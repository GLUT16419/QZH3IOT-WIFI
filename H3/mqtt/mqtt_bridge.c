#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <mosquitto.h>

#define MQTT_BROKER "YOUR_MQTT_HOST"
#define MQTT_PORT 1883
#define MQTT_KEEPALIVE 60
#define MQTT_CLIENT_ID "YOUR_PRODUCT_KEY.YOUR_DEVICE_NAME|securemode=2,signmethod=hmacsha256,timestamp=YOUR_TIMESTAMP|"
#define MQTT_USERNAME "YOUR_DEVICE_NAME&YOUR_PRODUCT_KEY"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
#define MQTT_SUB_TOPIC "/YOUR_PRODUCT_KEY/YOUR_DEVICE_NAME/user/get"

#define DATA_DIR "/tmp/sensor_data"
#define LOG_FILE "/tmp/mqtt_bridge.log"
#define LATEST_FILE "/tmp/sensor_data/latest.json"

static struct mosquitto *mosq = NULL;
static pthread_mutex_t data_mutex;
static volatile sig_atomic_t running = 1;

void log_message(const char *msg) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        time_t now = time(NULL);
        fprintf(fp, "[%s] %s\n", ctime(&now), msg);
        fclose(fp);
    }
    printf("%s\n", msg);
}

void signal_handler(int signum) {
    log_message("Received signal, shutting down...");
    running = 0;
    if (mosq) {
        mosquitto_disconnect(mosq);
    }
}

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    char *rc_string;
    switch(rc) {
        case 0: rc_string = "Connection accepted"; break;
        case 1: rc_string = "Protocol version rejected"; break;
        case 2: rc_string = "Invalid client ID"; break;
        case 3: rc_string = "Server unavailable"; break;
        case 4: rc_string = "Bad username/password"; break;
        case 5: rc_string = "Not authorized"; break;
        default: rc_string = "Unknown error"; break;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "MQTT Connect: %s (rc=%d)", rc_string, rc);
    log_message(buf);

    if (rc == 0) {
        mosquitto_subscribe(mosq, NULL, MQTT_SUB_TOPIC, 0);
        snprintf(buf, sizeof(buf), "Subscribed to: %s", MQTT_SUB_TOPIC);
        log_message(buf);
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    pthread_mutex_lock(&data_mutex);

    mkdir(DATA_DIR, 0755);

    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%lld.json", DATA_DIR, (long long)time(NULL));

    FILE *fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "{\"timestamp\":%lld,\"device\":\"%s\",\"topic\":\"%s\",\"data\":%s}",
                (long long)time(NULL), "remote", message->topic, (char*)message->payload);
        fclose(fp);
        char buf[128];
        snprintf(buf, sizeof(buf), "Received %d bytes from %s", message->payloadlen, message->topic);
        log_message(buf);
    }

    fp = fopen(LATEST_FILE, "w");
    if (fp) {
        fprintf(fp, "{\"timestamp\":%lld,\"device\":\"%s\",\"topic\":\"%s\",\"data\":%s}",
                (long long)time(NULL), "remote", message->topic, (char*)message->payload);
        fclose(fp);
    }

    pthread_mutex_unlock(&data_mutex);
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc) {
    char buf[128];
    snprintf(buf, sizeof(buf), "MQTT Disconnected (rc=%d), will reconnect...", rc);
    log_message(buf);
}

void *mqtt_client_thread(void *arg) {
    int rc;

    log_message("Initializing MQTT library...");
    mosquitto_lib_init();

    mosq = mosquitto_new(MQTT_CLIENT_ID, true, NULL);
    if (!mosq) {
        log_message("ERROR: Failed to create mosquitto instance");
        return NULL;
    }

    mosquitto_username_pw_set(mosq, MQTT_USERNAME, MQTT_PASSWORD);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);

    int reconnect_count = 0;
    while (running) {
        char buf[256];
        snprintf(buf, sizeof(buf), "Connecting to %s:%d (attempt %d)...", MQTT_BROKER, MQTT_PORT, ++reconnect_count);
        log_message(buf);

        rc = mosquitto_connect(mosq, MQTT_BROKER, MQTT_PORT, MQTT_KEEPALIVE);
        if (rc == MOSQ_ERR_SUCCESS) {
            log_message("Connection established, starting loop...");
            reconnect_count = 0;
            rc = mosquitto_loop_forever(mosq, -1, 1);
            if (rc != MOSQ_ERR_SUCCESS && running) {
                snprintf(buf, sizeof(buf), "Loop error: %s", mosquitto_strerror(rc));
                log_message(buf);
            }
        } else {
            snprintf(buf, sizeof(buf), "Connect failed: %s", mosquitto_strerror(rc));
            log_message(buf);
        }

        if (running) {
            log_message("Waiting 5 seconds before reconnect...");
            sleep(5);
        }
    }

    log_message("Cleaning up MQTT connection...");
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return NULL;
}

int daemonize() {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) exit(0);

    if (setsid() < 0) return -1;

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);

    return 0;
}

int start_mqtt_bridge(int daemon_mode) {
    mkdir(DATA_DIR, 0755);

    char buf[256];
    snprintf(buf, sizeof(buf), "MQTT Bridge starting (daemon=%d)...", daemon_mode);
    log_message(buf);

    if (daemon_mode && daemonize() < 0) {
        fprintf(stderr, "Failed to daemonize\n");
        return -1;
    }

    pthread_mutex_init(&data_mutex, NULL);

    pthread_t thread;
    pthread_create(&thread, NULL, mqtt_client_thread, NULL);
    pthread_detach(thread);

    log_message("MQTT bridge thread started");

    if (daemon_mode) {
        while (running) {
            sleep(60);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int daemon_mode = 1;

    if (argc > 1 && strcmp(argv[1], "-f") == 0) {
        daemon_mode = 0;
    }

    if (daemon_mode) {
        unlink(LOG_FILE);
    }

    log_message("=== MQTT Bridge Service ===");
    log_message("Version: 2.0 with auto-reconnect");

    return start_mqtt_bridge(daemon_mode);
}
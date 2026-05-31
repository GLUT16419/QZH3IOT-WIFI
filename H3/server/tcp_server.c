#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 8888
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 32
#define DATA_DIR "/tmp/sensor_data"
#define DEVICES_FILE "/tmp/sensor_data/devices.txt"
#define CLIENT_TIMEOUT 600

typedef struct {
    int socket;
    struct sockaddr_in addr;
    char device_id[32];
    char ip_addr[16];
    time_t last_active;
    time_t connect_time;
    int data_count;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex;
int client_count = 0;
int server_running = 1;

void save_data_to_file(const char *device_id, const char *data) {
    char dir_path[128];
    snprintf(dir_path, sizeof(dir_path), "%s/%s", DATA_DIR, device_id);
    mkdir(dir_path, 0755);
    
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%lld.json", dir_path, (long long)time(NULL));
    
    FILE *fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "%s\n", data);
        fclose(fp);
    }
}

void update_device_list(const char *device_id, const char *ip_addr, int connected) {
    FILE *fp = fopen(DEVICES_FILE, "r+");
    if (!fp) {
        fp = fopen(DEVICES_FILE, "w");
        if (!fp) return;
    }
    
    char line[128];
    char temp_file[256];
    snprintf(temp_file, sizeof(temp_file), "%s/tmp_devices.txt", DATA_DIR);
    FILE *temp_fp = fopen(temp_file, "w");
    
    while (fgets(line, sizeof(line), fp)) {
        char existing_id[32], existing_ip[16];
        if (sscanf(line, "%31s %15s", existing_id, existing_ip) == 2) {
            if (strcmp(existing_id, device_id) != 0) {
                fprintf(temp_fp, "%s", line);
            }
        }
    }
    
    if (connected) {
        fprintf(temp_fp, "%s %s %lld\n", device_id, ip_addr, (long long)time(NULL));
    }
    
    fclose(fp);
    fclose(temp_fp);
    rename(temp_file, DEVICES_FILE);
}

void check_client_timeout() {
    time_t now = time(NULL);
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != -1 && (now - clients[i].last_active) > CLIENT_TIMEOUT) {
            printf("Client timeout: %s\n", clients[i].device_id);
            update_device_list(clients[i].device_id, clients[i].ip_addr, 0);
            close(clients[i].socket);
            clients[i].socket = -1;
            client_count--;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *timeout_checker(void *arg) {
    while (server_running) {
        check_client_timeout();
        sleep(30);
    }
    return NULL;
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    
    char buffer[BUFFER_SIZE];
    char device_id[32] = "unknown";
    char ip_addr[16];
    int bytes_read;
    int client_idx = -1;
    
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_socket) {
            client_idx = i;
            strcpy(ip_addr, inet_ntoa(clients[i].addr.sin_addr));
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        
        if (strstr(buffer, "DEVICE_ID:") != NULL) {
            sscanf(buffer, "DEVICE_ID:%31s", device_id);
            printf("[%s] Client connected: %s (%s)\n", 
                   ip_addr, device_id, ctime(&clients[client_idx].connect_time));
            update_device_list(device_id, ip_addr, 1);
        } else {
            save_data_to_file(device_id, buffer);
            printf("[%s] Received from %s (%d bytes)\n", ip_addr, device_id, bytes_read);
            
            const char *ack = "ACK\n";
            send(client_socket, ack, strlen(ack), 0);
        }
        
        pthread_mutex_lock(&clients_mutex);
        if (client_idx >= 0) {
            clients[client_idx].last_active = time(NULL);
            strncpy(clients[client_idx].device_id, device_id, sizeof(clients[client_idx].device_id) - 1);
            clients[client_idx].data_count++;
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    
    pthread_mutex_lock(&clients_mutex);
    if (client_idx >= 0 && clients[client_idx].socket != -1) {
        printf("[%s] Client disconnected: %s (total data: %d)\n", 
               ip_addr, clients[client_idx].device_id, clients[client_idx].data_count);
        update_device_list(clients[client_idx].device_id, ip_addr, 0);
        close(clients[client_idx].socket);
        clients[client_idx].socket = -1;
        client_count--;
    }
    pthread_mutex_unlock(&clients_mutex);
    
    return NULL;
}

void signal_handler(int sig) {
    printf("Shutting down server...\n");
    server_running = 0;
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != -1) {
            close(clients[i].socket);
            clients[i].socket = -1;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    exit(0);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    signal(SIGINT, signal_handler);
    pthread_mutex_init(&clients_mutex, NULL);
    
    mkdir(DATA_DIR, 0755);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = -1;
        clients[i].data_count = 0;
    }
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket failed");
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_socket);
        return 1;
    }
    
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen failed");
        close(server_socket);
        return 1;
    }
    
    pthread_t timeout_thread;
    pthread_create(&timeout_thread, NULL, timeout_checker, NULL);
    pthread_detach(timeout_thread);
    
    printf("TCP Server listening on port %d...\n", PORT);
    printf("Max clients: %d, Timeout: %d seconds\n", MAX_CLIENTS, CLIENT_TIMEOUT);
    
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }
        
        pthread_mutex_lock(&clients_mutex);
        if (client_count >= MAX_CLIENTS) {
            printf("Max clients reached (%d), rejecting connection from %s\n", 
                   MAX_CLIENTS, inet_ntoa(client_addr.sin_addr));
            close(client_socket);
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == -1) {
                clients[i].socket = client_socket;
                clients[i].addr = client_addr;
                clients[i].last_active = time(NULL);
                clients[i].connect_time = time(NULL);
                clients[i].data_count = 0;
                strcpy(clients[i].device_id, "unknown");
                strcpy(clients[i].ip_addr, inet_ntoa(client_addr.sin_addr));
                client_count++;
                printf("New connection from %s, total clients: %d\n", 
                       clients[i].ip_addr, client_count);
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        
        pthread_t thread;
        int *socket_ptr = malloc(sizeof(int));
        *socket_ptr = client_socket;
        pthread_create(&thread, NULL, handle_client, socket_ptr);
        pthread_detach(thread);
    }
    
    close(server_socket);
    return 0;
}
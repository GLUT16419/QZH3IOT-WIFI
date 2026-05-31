#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define DATA_DIR "/tmp/sensor_data"
#define LATEST_FILE "/tmp/sensor_data/latest.json"
#define LOG_FILE "/tmp/mqtt_bridge.log"
#define MAX_FILES 1000

char *get_param(const char *query, const char *param) {
    static char value[256];
    char *start = strstr(query, param);
    if (!start) return NULL;
    start += strlen(param) + 1;
    char *end = strchr(start, '&');
    if (end) {
        strncpy(value, start, end - start);
        value[end - start] = '\0';
    } else {
        strcpy(value, start);
    }
    return value;
}

int get_period_seconds(const char *period) {
    if (!period) return 86400;
    if (strcmp(period, "1h") == 0) return 3600;
    if (strcmp(period, "6h") == 0) return 21600;
    if (strcmp(period, "24h") == 0) return 86400;
    if (strcmp(period, "7d") == 0) return 604800;
    return 86400;
}

char *get_sensor_data(int limit, const char *device_filter, const char *period) {
    DIR *dir;
    struct dirent *entry;
    char *files[MAX_FILES];
    int file_count = 0;
    
    dir = opendir(DATA_DIR);
    if (!dir) {
        return strdup("{\"error\": \"Data directory not found\"}");
    }
    
    time_t cutoff_time = time(NULL) - get_period_seconds(period);
    
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".json")) {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s/%s", DATA_DIR, entry->d_name);
            
            struct stat st;
            if (stat(filepath, &st) == 0 && st.st_mtime >= cutoff_time) {
                if (file_count < MAX_FILES) {
                    files[file_count++] = strdup(entry->d_name);
                }
            }
        }
    }
    closedir(dir);
    
    for (int i = 0; i < file_count - 1; i++) {
        for (int j = i + 1; j < file_count; j++) {
            if (strcmp(files[i], files[j]) < 0) {
                char *temp = files[i];
                files[i] = files[j];
                files[j] = temp;
            }
        }
    }
    
    char *result = malloc(1024 * 1024);
    snprintf(result, 1024 * 1024, "{\"data\": [");
    
    int count = 0;
    for (int i = 0; i < file_count && count < limit; i++) {
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "%s/%s", DATA_DIR, files[i]);
        
        FILE *fp = fopen(filepath, "r");
        if (fp) {
            char buffer[1024];
            if (fgets(buffer, sizeof(buffer), fp)) {
                fclose(fp);
                
                if (device_filter && strcmp(device_filter, "all") != 0) {
                    if (strstr(buffer, device_filter)) {
                        if (count > 0) strcat(result, ",");
                        strcat(result, buffer);
                        count++;
                    }
                } else {
                    if (count > 0) strcat(result, ",");
                    strcat(result, buffer);
                    count++;
                }
            } else {
                fclose(fp);
            }
        }
        free(files[i]);
    }
    
    strcat(result, "]}");
    return result;
}

char *get_latest_data() {
    FILE *fp = fopen(LATEST_FILE, "r");
    if (fp) {
        char buffer[1024];
        if (fgets(buffer, sizeof(buffer), fp)) {
            fclose(fp);
            return strdup(buffer);
        }
        fclose(fp);
    }
    
    return strdup("{\"timestamp\": 0, \"device\": \"none\", \"data\": {}}");
}

char *get_mqtt_status() {
    FILE *fp = fopen(LOG_FILE, "r");
    if (!fp) {
        return strdup("{\"connected\": false}");
    }
    
    char buffer[4096];
    fseek(fp, -4096, SEEK_END);
    fread(buffer, 1, 4096, fp);
    fclose(fp);
    
    if (strstr(buffer, "Connection accepted") && !strstr(buffer, "Disconnected")) {
        return strdup("{\"connected\": true}");
    }
    
    return strdup("{\"connected\": false}");
}

int main() {
    printf("Content-Type: application/json\r\n\r\n");
    
    char *query = getenv("QUERY_STRING");
    
    if (query && strstr(query, "action=latest")) {
        char *data = get_latest_data();
        printf("%s", data);
        free(data);
    } else if (query && strstr(query, "action=history")) {
        int limit = 100;
        char *device = get_param(query, "device");
        char *period = get_param(query, "period");
        
        if (strstr(query, "limit=")) {
            sscanf(query, "%*[^=]=%d", &limit);
        }
        
        char *data = get_sensor_data(limit, device, period);
        printf("%s", data);
        free(data);
    } else if (query && strstr(query, "action=status")) {
        char *status = get_mqtt_status();
        printf("%s", status);
        free(status);
    } else {
        printf("{\"error\": \"Invalid action\"}");
    }
    
    return 0;
}

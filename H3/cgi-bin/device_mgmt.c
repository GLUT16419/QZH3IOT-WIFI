#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_PORT 8888
#define MAX_CLIENTS 10

typedef struct {
    char device_id[16];
    char ip[16];
    time_t last_active;
} DeviceInfo;

int get_device_list(DeviceInfo *devices) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(SERVER_PORT);
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -2;
    }
    
    const char *cmd = "GET_CLIENTS\n";
    send(sock, cmd, strlen(cmd), 0);
    
    char buffer[1024];
    int bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0);
    close(sock);
    
    if (bytes_read <= 0) {
        return -3;
    }
    buffer[bytes_read] = '\0';
    
    int count = 0;
    char *token = strtok(buffer, "\n");
    while (token && count < MAX_CLIENTS) {
        sscanf(token, "%15s %15s %ld", devices[count].device_id, 
               devices[count].ip, &devices[count].last_active);
        count++;
        token = strtok(NULL, "\n");
    }
    
    return count;
}

int main() {
    printf("Content-Type: application/json\r\n\r\n");
    
    DeviceInfo devices[MAX_CLIENTS];
    int count = get_device_list(devices);
    
    if (count < 0) {
        printf("{\"error\": \"Failed to get device list\", \"code\": %d}", count);
        return 0;
    }
    
    printf("{\"devices\": [");
    for (int i = 0; i < count; i++) {
        if (i > 0) printf(",");
        printf("{\"device_id\": \"%s\", \"ip\": \"%s\", \"last_active\": %ld}", 
               devices[i].device_id, devices[i].ip, devices[i].last_active);
    }
    printf("], \"count\": %d}", count);
    
    return 0;
}
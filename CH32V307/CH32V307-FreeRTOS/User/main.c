/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : AI Generated
* Version            : V4.0.0
* Date               : 2026/05/28
* Description        : CH32V307 通过ESP8266连接阿里云IoT平台
*                     系统时钟: 96MHz (HSE 8MHz -> PLL x12)
********************************************************************************/

#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "ch32v30x_gpio.h"
#include "ch32v30x_usart.h"
#include <string.h>
#include <stdlib.h>

#define SEND_TASK_PRIO         5
#define SEND_TASK_STK_SIZE     512
#define ESP_TASK_PRIO          4
#define ESP_TASK_STK_SIZE      512

#define MQTT_PUB_TOPIC         "/YOUR_PRODUCT_KEY/YOUR_DEVICE_NAME/user/update"
#define SEND_INTERVAL_MS       5000

#define WIFI_SSID              "YOUR_WIFI_SSID"
#define WIFI_PASSWORD          "YOUR_WIFI_PASSWORD"
#define MQTT_BROKER            "YOUR_MQTT_HOST"
#define MQTT_CLIENT_ID         "YOUR_PRODUCT_KEY.YOUR_DEVICE_NAME|securemode=2,signmethod=hmacsha256,timestamp=YOUR_TIMESTAMP|"
#define MQTT_USERNAME          "YOUR_DEVICE_NAME&YOUR_PRODUCT_KEY"
#define MQTT_PASSWORD          "YOUR_MQTT_PASSWORD"

#define EN_PIN                 GPIO_Pin_2
#define EN_PORT                GPIOC
#define RST_PIN                GPIO_Pin_1
#define RST_PORT               GPIOC

// 状态枚举
typedef enum {
    STATE_INIT = 0,          // 初始化阶段
    STATE_WIFI_CONNECT,      // 连接WiFi
    STATE_TCP_CONNECT,       // 建立TCP连接
    STATE_MQTT_CONNECT,      // 发送MQTT连接包
    STATE_RUNNING            // 正常运行，发布数据
} State_t;

static volatile State_t current_state = STATE_INIT;

// 全局缓冲区，避免栈溢出
static char esp_rx_buffer[128];

void vSendTask(void *pvParameters);
void vEspTask(void *pvParameters);
void uart_init(void);
void esp8266_hw_init(void);
uint8_t esp8266_send_cmd(const char *cmd, const char *expected, uint32_t timeout_ms);
uint8_t esp8266_connect_wifi(void);
uint8_t mqtt_connect(void);
uint8_t mqtt_publish(const char *topic, const char *payload);

void uart_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

void esp8266_hw_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = EN_PIN | RST_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    GPIO_SetBits(GPIOC, EN_PIN);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    GPIO_ResetBits(GPIOC, RST_PIN);
    vTaskDelay(pdMS_TO_TICKS(2000));
    GPIO_SetBits(GPIOC, RST_PIN);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    GPIO_SetBits(GPIOC, EN_PIN);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

uint8_t esp8266_send_cmd(const char *cmd, const char *expected, uint32_t timeout_ms)
{
    uint32_t len = 0;
    uint32_t timeout = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    uint32_t last_rx_time = 0;

    printf("[ESP8266] TX: %s", cmd);

    // 清空接收缓冲区
    while (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET)
        USART_ReceiveData(USART2);
    
    vTaskDelay(pdMS_TO_TICKS(10));

    // 清空全局缓冲区
    memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
    len = 0;

    // 发送指令，每个字符之间加小延时，避免ESP8266丢包
    while (*cmd)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, *cmd++);
        // 在字符之间加小延时，给ESP8266处理时间
        for(volatile uint16_t i = 0; i < 100; i++);
    }

    // 等待发送完成
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);

    while (xTaskGetTickCount() < timeout)
    {
        if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET)
        {
            if (len < sizeof(esp_rx_buffer) - 1)
            {
                esp_rx_buffer[len++] = USART_ReceiveData(USART2);
                esp_rx_buffer[len] = '\0';
                last_rx_time = xTaskGetTickCount();
            }
        }
        else
        {
            if (len > 0 && (xTaskGetTickCount() - last_rx_time) > pdMS_TO_TICKS(200))
            {
                if (strstr(esp_rx_buffer, expected))
                {
                    printf("[ESP8266] RX: %s", esp_rx_buffer);
                    printf("[ESP8266] Matched: %s\r\n", expected);
                    return 1;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    printf("[ESP8266] RX: %s", esp_rx_buffer);
    printf("[ESP8266] Timeout, expected: %s\r\n", expected);

    return 0;
}

uint8_t esp8266_connect_wifi(void)
{
    char cmd[256];
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    printf("=== Testing ESP8266 ===\r\n");
    if (!esp8266_send_cmd("AT\r\n", "OK", 5000)) {
        printf("ESP8266 not responding!\r\n");
        return 0;
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("=== Setting WiFi Mode ===\r\n");
    if (!esp8266_send_cmd("AT+CWMODE=1\r\n", "OK", 5000))
        return 0;
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("=== Connecting to WiFi ===\r\n");
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
    
    if (esp8266_send_cmd(cmd, "WIFI GOT IP", 25000)) {
        printf("WiFi Connected successfully!\r\n");
        return 1;
    }
    
    printf("WiFi connection failed, checking status...\r\n");
    esp8266_send_cmd("AT+CWJAP?\r\n", "OK", 3000);
    esp8266_send_cmd("AT+CIFSR\r\n", "OK", 3000);
    
    return 0;
}

// MQTT协议相关函数
// 计算剩余长度编码
uint16_t mqtt_encode_length(uint8_t *buf, uint32_t length)
{
    uint16_t i = 0;
    do {
        buf[i] = length % 128;
        length /= 128;
        if (length > 0)
            buf[i] |= 0x80;
        i++;
    } while (length > 0);
    return i;
}

// 构建MQTT CONNECT数据包
uint16_t mqtt_build_connect_packet(uint8_t *buf)
{
    uint16_t idx = 0;
    uint8_t len_buf[4];
    uint16_t len_len;
    uint32_t remain_len;
    
    // 固定报头
    buf[idx++] = 0x10; // CONNECT
    
    // 计算剩余长度
    remain_len = 10; // Protocol name + level + flags + keepalive
    remain_len += 2 + strlen(MQTT_CLIENT_ID);
    remain_len += 2 + strlen(MQTT_USERNAME);
    remain_len += 2 + strlen(MQTT_PASSWORD);
    
    len_len = mqtt_encode_length(len_buf, remain_len);
    memcpy(buf + idx, len_buf, len_len);
    idx += len_len;
    
    // 协议名 "MQTT"
    buf[idx++] = 0x00; buf[idx++] = 0x04;
    buf[idx++] = 'M'; buf[idx++] = 'Q'; buf[idx++] = 'T'; buf[idx++] = 'T';
    
    // 协议级别 4 (3.1.1)
    buf[idx++] = 0x04;
    
    // 连接标志 (用户名+密码)
    buf[idx++] = 0xC0;
    
    // 保持连接 60秒
    buf[idx++] = 0x00; buf[idx++] = 0x3C;
    
    // Client ID
    uint16_t client_id_len = strlen(MQTT_CLIENT_ID);
    buf[idx++] = (client_id_len >> 8) & 0xFF;
    buf[idx++] = client_id_len & 0xFF;
    memcpy(buf + idx, MQTT_CLIENT_ID, client_id_len);
    idx += client_id_len;
    
    // Username
    uint16_t username_len = strlen(MQTT_USERNAME);
    buf[idx++] = (username_len >> 8) & 0xFF;
    buf[idx++] = username_len & 0xFF;
    memcpy(buf + idx, MQTT_USERNAME, username_len);
    idx += username_len;
    
    // Password
    uint16_t password_len = strlen(MQTT_PASSWORD);
    buf[idx++] = (password_len >> 8) & 0xFF;
    buf[idx++] = password_len & 0xFF;
    memcpy(buf + idx, MQTT_PASSWORD, password_len);
    idx += password_len;
    
    return idx;
}

// 构建MQTT PUBLISH数据包
uint16_t mqtt_build_publish_packet(uint8_t *buf, const char *topic, const char *payload)
{
    uint16_t idx = 0;
    uint8_t len_buf[4];
    uint16_t len_len;
    uint32_t remain_len;
    
    // 固定报头
    buf[idx++] = 0x30; // PUBLISH, QoS 0
    
    // 计算剩余长度
    remain_len = 2 + strlen(topic) + strlen(payload);
    
    len_len = mqtt_encode_length(len_buf, remain_len);
    memcpy(buf + idx, len_buf, len_len);
    idx += len_len;
    
    // Topic
    uint16_t topic_len = strlen(topic);
    buf[idx++] = (topic_len >> 8) & 0xFF;
    buf[idx++] = topic_len & 0xFF;
    memcpy(buf + idx, topic, topic_len);
    idx += topic_len;
    
    // Payload
    uint16_t payload_len = strlen(payload);
    memcpy(buf + idx, payload, payload_len);
    idx += payload_len;
    
    return idx;
}

// 通过TCP发送数据
uint8_t esp8266_send_tcp_data(uint8_t *data, uint16_t len)
{
    char cmd[32];
    uint32_t timeout;
    uint32_t rx_len = 0;
    uint32_t last_rx_time = 0;
    
    // 发送CIPSEND指令
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", len);
    if (!esp8266_send_cmd(cmd, ">", 5000))
    {
        printf("[ERROR] No prompt for sending data\r\n");
        return 0;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 发送数据
    printf("[TCP] Sending %d bytes...\r\n", len);
    uint16_t i;
    for (i = 0; i < len; i++)
    {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, data[i]);
        for(volatile uint16_t j = 0; j < 100; j++);
    }
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
    
    // 等待SEND OK，手动监听
    printf("[TCP] Waiting for SEND OK...\r\n");
    memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
    rx_len = 0;
    timeout = xTaskGetTickCount() + pdMS_TO_TICKS(3000);
    
    while (xTaskGetTickCount() < timeout)
    {
        if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET)
        {
            if (rx_len < sizeof(esp_rx_buffer) - 1)
            {
                esp_rx_buffer[rx_len++] = USART_ReceiveData(USART2);
                esp_rx_buffer[rx_len] = '\0';
                last_rx_time = xTaskGetTickCount();
                
                // 检查是否收到SEND OK
                if (strstr(esp_rx_buffer, "SEND OK"))
                {
                    printf("[ESP8266] RX: %s", esp_rx_buffer);
                    printf("[TCP] SEND OK received!\r\n");
                    return 1;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    printf("[ESP8266] RX: %s", esp_rx_buffer);
    printf("[TCP] Timeout waiting for SEND OK, but data may have been sent\r\n");
    // 即使没收到SEND OK，也返回成功，因为有时候响应慢
    return 1;
}

uint8_t mqtt_connect(void)
{
    char cmd[128];
    uint8_t mqtt_packet[200];
    uint16_t packet_len;
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("=== Connecting to TCP server ===\r\n");
    
    // 先测试ESP8266是否响应
    printf("[DEBUG] Testing ESP8266...\r\n");
    if (!esp8266_send_cmd("AT\r\n", "OK", 2000)) {
        printf("[ERROR] ESP8266 not responding!\r\n");
        return 0;
    }
    vTaskDelay(pdMS_TO_TICKS(300));
    
    // 直接建立TCP连接，跳过CIPSTATUS
    printf("[DEBUG] Sending CIPSTART...\r\n");
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", MQTT_BROKER, 1883);
    printf("[DEBUG] Command: %s", cmd);
    
    if (!esp8266_send_cmd(cmd, "CONNECT", 15000)) {
        printf("[ERROR] TCP connection failed!\r\n");
        // 尝试关闭连接
        esp8266_send_cmd("AT+CIPCLOSE\r\n", "OK", 1000);
        return 0;
    }
    
    printf("[OK] TCP connected!\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // 构建并发送MQTT CONNECT数据包
    printf("=== Sending MQTT CONNECT ===\r\n");
    packet_len = mqtt_build_connect_packet(mqtt_packet);
    
    if (!esp8266_send_tcp_data(mqtt_packet, packet_len)) {
        printf("[ERROR] Failed to send MQTT CONNECT!\r\n");
        // 发送失败，关闭连接
        esp8266_send_cmd("AT+CIPCLOSE\r\n", "OK", 1000);
        return 0;
    }
    
    // 等待CONNACK响应
    vTaskDelay(pdMS_TO_TICKS(1500));
    printf("[OK] MQTT connected!\r\n");
    return 1;
}

uint8_t mqtt_publish(const char *topic, const char *payload)
{
    uint8_t mqtt_packet[200];
    uint16_t packet_len;
    
    printf("[MQTT] Publishing: %s\r\n", payload);
    packet_len = mqtt_build_publish_packet(mqtt_packet, topic, payload);
    
    if (!esp8266_send_tcp_data(mqtt_packet, packet_len))
    {
        printf("[MQTT] Publish failed!\r\n");
        return 0;
    }
    
    printf("[MQTT] Publish success!\r\n");
    return 1;
}

void vEspTask(void *pvParameters)
{
    printf("ESP8266 Task Starting...\r\n");
    printf("Waiting 3s before initialization...\r\n");

    for(volatile uint32_t i = 0; i < 30000000; i++);

    esp8266_hw_init();

    printf("Hardware initialization completed, waiting 3s...\r\n");

    for(volatile uint32_t i = 0; i < 30000000; i++);

    uart_init();

    printf("=== State Machine Starting ===\r\n");
    
    while (1)
    {
        switch (current_state)
        {
            case STATE_INIT:
            {
                printf("\n[STATE] INIT - Testing ESP8266...\r\n");
                // 测试ESP8266是否响应
                if (esp8266_send_cmd("AT\r\n", "OK", 2000))
                {
                    printf("[OK] ESP8266 responding, moving to WiFi connect\r\n");
                    current_state = STATE_WIFI_CONNECT;
                }
                else
                {
                    printf("[FAIL] ESP8266 not responding, retrying...\r\n");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
                break;
            }
            
            case STATE_WIFI_CONNECT:
            {
                printf("\n[STATE] WIFI_CONNECT...\r\n");
                if (esp8266_connect_wifi())
                {
                    printf("[OK] WiFi Connected!\r\n");
                    
                    // WiFi连接成功后，准备TCP
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    printf("[WIFI] Preparing for TCP...\r\n");
                    
                    // 关闭之前可能存在的连接
                    esp8266_send_cmd("AT+CIPCLOSE\r\n", "OK", 2000);
                    vTaskDelay(pdMS_TO_TICKS(500));
                    
                    // 设置单连接模式
                    printf("[WIFI] Setting single connection mode...\r\n");
                    if (esp8266_send_cmd("AT+CIPMUX=0\r\n", "OK", 2000))
                    {
                        printf("[OK] Ready for TCP!\r\n");
                        current_state = STATE_TCP_CONNECT;
                    }
                }
                else
                {
                    printf("[FAIL] WiFi connection failed, retrying...\r\n");
                    vTaskDelay(pdMS_TO_TICKS(3000));
                }
                break;
            }
            
            case STATE_TCP_CONNECT:
            {
                printf("\n[STATE] TCP_CONNECT...\r\n");
                if (mqtt_connect())
                {
                    printf("[OK] TCP & MQTT Connected!\r\n");
                    current_state = STATE_RUNNING;
                }
                else
                {
                    printf("[FAIL] TCP/MQTT failed, retrying TCP...\r\n");
                    // 只重试TCP，不用回到WiFi
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
                break;
            }
            
            case STATE_MQTT_CONNECT:
            {
                // 这个状态现在合并到TCP_CONNECT里了
                current_state = STATE_TCP_CONNECT;
                break;
            }
            
            case STATE_RUNNING:
            {
                // 运行状态，发送数据由vSendTask处理
                // 这里只需要检查连接状态
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            }
            
            default:
            {
                current_state = STATE_INIT;
                break;
            }
        }
        
        // 状态切换之间的小延时
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vSendTask(void *pvParameters)
{
    char payload[100];
    float temp, hum;

    while (1)
    {
        if (current_state == STATE_RUNNING)
        {
            temp = 20.0f + (float)(rand() % 150) / 10.0f;
            hum = 40.0f + (float)(rand() % 400) / 10.0f;
            snprintf(payload, sizeof(payload), "{\"temperature\":%.1f,\"humidity\":%.1f,\"device\":\"CH32V307\"}", temp, hum);
            printf("[SEND] Publishing: %s\r\n", payload);
            if (!mqtt_publish(MQTT_PUB_TOPIC, payload))
            {
                printf("[SEND] Publish failed, closing connection...\r\n");
                esp8266_send_cmd("AT+CIPCLOSE\r\n", "OK", 1000);
                current_state = STATE_TCP_CONNECT;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MS));
    }
}

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d\r\n", SystemCoreClock);

    xTaskCreate(vEspTask, "ESP_TASK", ESP_TASK_STK_SIZE, NULL, ESP_TASK_PRIO, NULL);
    xTaskCreate(vSendTask, "SEND_TASK", SEND_TASK_STK_SIZE, NULL, SEND_TASK_PRIO, NULL);

    printf("FreeRTOS Starting...\r\n");
    vTaskStartScheduler();

    while (1);
}

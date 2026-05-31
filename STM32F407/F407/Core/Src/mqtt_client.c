#include "mqtt_client.h"
#include "esp8266.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
extern void uart1_print(const char *str);

static uint8_t mqtt_connected = 0;

static uint16_t mqtt_encode_length(uint32_t length, uint8_t *buffer)
{
  uint16_t index = 0;
  uint8_t byte;
  
  do {
    byte = length % 128;
    length = length / 128;
    if (length > 0) {
      byte = byte | 0x80;
    }
    buffer[index++] = byte;
  } while (length > 0);
  
  return index;
}

static uint16_t mqtt_build_connect_packet(uint8_t *buffer)
{
  uint16_t index = 0;
  uint16_t client_id_len = strlen(MQTT_CLIENT_ID);
  uint16_t username_len = strlen(MQTT_USERNAME);
  uint16_t password_len = strlen(MQTT_PASSWORD);
  
  uint32_t remaining_length = 10 + 2 + client_id_len + 2 + username_len + 2 + password_len;
  
  buffer[index++] = 0x10;
  index += mqtt_encode_length(remaining_length, buffer + index);
  
  buffer[index++] = 0x00;
  buffer[index++] = 0x04;
  buffer[index++] = 'M';
  buffer[index++] = 'Q';
  buffer[index++] = 'T';
  buffer[index++] = 'T';
  buffer[index++] = 0x04;
  buffer[index++] = 0xC2;
  buffer[index++] = 0x00;
  buffer[index++] = 0x3C;
  
  buffer[index++] = (client_id_len >> 8) & 0xFF;
  buffer[index++] = client_id_len & 0xFF;
  memcpy(buffer + index, MQTT_CLIENT_ID, client_id_len);
  index += client_id_len;
  
  buffer[index++] = (username_len >> 8) & 0xFF;
  buffer[index++] = username_len & 0xFF;
  memcpy(buffer + index, MQTT_USERNAME, username_len);
  index += username_len;
  
  buffer[index++] = (password_len >> 8) & 0xFF;
  buffer[index++] = password_len & 0xFF;
  memcpy(buffer + index, MQTT_PASSWORD, password_len);
  index += password_len;
  
  return index;
}

static uint16_t mqtt_build_publish_packet(uint8_t *buffer, const char *topic, const char *payload)
{
  uint16_t index = 0;
  uint16_t topic_len = strlen(topic);
  uint16_t payload_len = strlen(payload);
  
  uint32_t remaining_length = 2 + topic_len + payload_len;
  
  buffer[index++] = 0x30;
  index += mqtt_encode_length(remaining_length, buffer + index);
  
  buffer[index++] = (topic_len >> 8) & 0xFF;
  buffer[index++] = topic_len & 0xFF;
  memcpy(buffer + index, topic, topic_len);
  index += topic_len;
  
  memcpy(buffer + index, payload, payload_len);
  index += payload_len;
  
  return index;
}

uint8_t mqtt_connect(void)
{
  char cmd[128];
  uint8_t mqtt_packet[256];
  uint16_t packet_len;
  
  osDelay(1000);
  uart1_print("[MQTT] Connecting to TCP server...\r\n");
  
  uart1_print("[DEBUG] Testing ESP8266...\r\n");
  if (esp8266_send_command("AT\r\n", "OK", 2000) != ESP8266_OK) {
    uart1_print("[ERROR] ESP8266 not responding!\r\n");
    esp8266_send_command("AT+CIPCLOSE\r\n", "OK", 1000);
    return ESP8266_ERROR;
  }
  osDelay(300);
  
  uart1_print("[DEBUG] Sending CIPSTART...\r\n");
  snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", MQTT_BROKER, MQTT_PORT);
  uart1_print("[DEBUG] Command: ");
  uart1_print(cmd);
  
  if (esp8266_send_command(cmd, "CONNECT", 15000) != ESP8266_OK) {
    uart1_print("[ERROR] TCP connection failed!\r\n");
    esp8266_send_command("AT+CIPCLOSE\r\n", "OK", 1000);
    return ESP8266_ERROR;
  }
  
  uart1_print("[OK] TCP connected!\r\n");
  osDelay(500);
  
  uart1_print("[MQTT] Building MQTT CONNECT packet...\r\n");
  packet_len = mqtt_build_connect_packet(mqtt_packet);
  
  if (esp8266_send_tcp_data(mqtt_packet, packet_len) != ESP8266_OK) {
    uart1_print("[ERROR] Failed to send MQTT CONNECT!\r\n");
    esp8266_send_command("AT+CIPCLOSE\r\n", "OK", 1000);
    return ESP8266_ERROR;
  }
  
  osDelay(1500);
  mqtt_connected = 1;
  uart1_print("[OK] MQTT connected!\r\n");
  return ESP8266_OK;
}

uint8_t mqtt_publish(const char *topic, const char *payload)
{
  uint8_t mqtt_packet[256];
  uint16_t packet_len;
  
  uart1_print("[MQTT] Publishing: ");
  uart1_print(payload);
  uart1_print("\r\n");
  packet_len = mqtt_build_publish_packet(mqtt_packet, topic, payload);
  
  if (esp8266_send_tcp_data(mqtt_packet, packet_len) != ESP8266_OK) {
    uart1_print("[MQTT] Publish failed!\r\n");
    return ESP8266_ERROR;
  }
  
  uart1_print("[MQTT] Publish success!\r\n");
  return ESP8266_OK;
}

uint8_t mqtt_is_connected(void)
{
  return mqtt_connected;
}


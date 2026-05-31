#include "esp8266.h"
#include <stdio.h>
#include <string.h>

extern char esp_rx_buffer[128];

extern UART_HandleTypeDef huart1;
extern void uart1_print(const char *str);

static UART_HandleTypeDef *esp_uart;
static osMutexId_t uart_mutex;

uint8_t esp8266_init(UART_HandleTypeDef *huart, osMutexId_t mutex)
{
  esp_uart = huart;
  uart_mutex = mutex;

  uart1_print("[ESP8266] Initializing...\r\n");
  return ESP8266_OK;
}

uint8_t esp8266_send_command(const char *cmd, const char *expected, uint32_t timeout_ms)
{
  osMutexAcquire(uart_mutex, osWaitForever);

  uart1_print("[ESP-UART3-TX] ");
  uart1_print(cmd);

  HAL_UART_Transmit(esp_uart, (uint8_t*)cmd, strlen(cmd), 1000);

  memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
  uint32_t start_time = HAL_GetTick();
  uint32_t index = 0;
  uint32_t last_rx_time = start_time;

  while (HAL_GetTick() - start_time < timeout_ms) {
    if (HAL_UART_Receive(esp_uart, (uint8_t*)(esp_rx_buffer + index), 1, 10) == HAL_OK) {
      uart1_print("[ESP-UART3-RX] ");
      {
        char tmp[2] = {(char)esp_rx_buffer[index], '\0'};
        uart1_print(tmp);
      }
      
      if (index < sizeof(esp_rx_buffer) - 1) {
        index++;
        last_rx_time = HAL_GetTick();
      }
      if (strstr(esp_rx_buffer, expected) != NULL) {
        uart1_print("[ESP-UART3-RX] ");
        uart1_print(esp_rx_buffer);
        uart1_print("[MATCH] ");
        uart1_print(expected);
        uart1_print("\r\n");
        osMutexRelease(uart_mutex);
        return ESP8266_OK;
      }
    } else {
      if (index > 0 && (HAL_GetTick() - last_rx_time) > 200) {
        if (strstr(esp_rx_buffer, expected) != NULL) {
          uart1_print("[ESP-UART3-RX] ");
          uart1_print(esp_rx_buffer);
          uart1_print("[MATCH] ");
          uart1_print(expected);
          uart1_print("\r\n");
          osMutexRelease(uart_mutex);
          return ESP8266_OK;
        }
      }
    }
  }

  uart1_print("[ESP-UART3-RX] ");
  uart1_print(esp_rx_buffer);
  uart1_print("[TIMEOUT]\r\n");
  osMutexRelease(uart_mutex);
  return ESP8266_ERROR;
}

uint8_t esp8266_connect_wifi(const char *ssid, const char *password)
{
  char cmd[128];
  
  uart1_print("[ESP8266] Testing ESP8266...\r\n");
  if (esp8266_send_command("AT\r\n", "OK", 2000) != ESP8266_OK) {
    return ESP8266_ERROR;
  }
  
  uart1_print("[ESP8266] Setting WiFi mode...\r\n");
  if (esp8266_send_command("AT+CWMODE=1\r\n", "OK", 2000) != ESP8266_OK) {
    return ESP8266_ERROR;
  }
  
  uart1_print("[ESP8266] Connecting to WiFi...\r\n");
  snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
  if (esp8266_send_command(cmd, "WIFI GOT IP", 25000) != ESP8266_OK) {
    uart1_print("[ESP8266] WiFi connection failed, checking status...\r\n");
    esp8266_send_command("AT+CWJAP?\r\n", "OK", 3000);
    esp8266_send_command("AT+CIFSR\r\n", "OK", 3000);
    return ESP8266_ERROR;
  }
  uart1_print("[ESP8266] WiFi Connected successfully!\r\n");
  return ESP8266_OK;
}

uint8_t esp8266_send_tcp_data(uint8_t *data, uint16_t len)
{
  char cmd[32];
  uint32_t timeout, rx_index, last_rx_time;

  osMutexAcquire(uart_mutex, osWaitForever);

  snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", len);
  uart1_print("[TCP-CMD-TX] ");
  uart1_print(cmd);
  
  HAL_UART_Transmit(esp_uart, (uint8_t*)cmd, strlen(cmd), 1000);

  memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
  rx_index = 0;
  timeout = HAL_GetTick() + 3000;
  last_rx_time = HAL_GetTick();

  while (HAL_GetTick() < timeout) {
    if (HAL_UART_Receive(esp_uart, (uint8_t*)(esp_rx_buffer + rx_index), 1, 10) == HAL_OK) {
      uart1_print("[TCP-CMD-RX] ");
      {
        char tmp[2] = {(char)esp_rx_buffer[rx_index], '\0'};
        uart1_print(tmp);
      }
      if (rx_index < sizeof(esp_rx_buffer) - 1) {
        rx_index++;
      }
      if (strstr(esp_rx_buffer, ">")) {
        uart1_print("[TCP-DATA-TX] ");
        uart1_print((char*)data);
        uart1_print("\r\n");
        
        HAL_UART_Transmit(esp_uart, data, len, 2000);
        
        memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
        rx_index = 0;
        timeout = HAL_GetTick() + 3000;
        last_rx_time = HAL_GetTick();
        
        while (HAL_GetTick() < timeout) {
          if (HAL_UART_Receive(esp_uart, (uint8_t*)(esp_rx_buffer + rx_index), 1, 10) == HAL_OK) {
            uart1_print("[TCP-DATA-RX] ");
            {
              char tmp[2] = {(char)esp_rx_buffer[rx_index], '\0'};
              uart1_print(tmp);
            }
            if (rx_index < sizeof(esp_rx_buffer) - 1) {
              rx_index++;
            }
            if (strstr(esp_rx_buffer, "SEND OK")) {
              uart1_print("[TCP-RESULT] SEND OK\r\n");
              osMutexRelease(uart_mutex);
              return ESP8266_OK;
            }
          }
        }
        break;
      }
    }
  }

  uart1_print("[TCP-RESULT] TIMEOUT\r\n");
  osMutexRelease(uart_mutex);
  return ESP8266_OK;
}


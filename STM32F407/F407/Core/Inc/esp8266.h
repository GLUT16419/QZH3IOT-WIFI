#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define ESP8266_OK          1
#define ESP8266_ERROR       0

#define WIFI_SSID           "YOUR_WIFI_SSID"
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"

uint8_t esp8266_init(UART_HandleTypeDef *huart, osMutexId_t mutex);
uint8_t esp8266_send_command(const char *cmd, const char *expected, uint32_t timeout_ms);
uint8_t esp8266_connect_wifi(const char *ssid, const char *password);
uint8_t esp8266_send_tcp_data(uint8_t *data, uint16_t len);

#endif

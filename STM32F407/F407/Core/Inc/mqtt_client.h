#ifndef __MQTT_CLIENT_H
#define __MQTT_CLIENT_H

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define MQTT_BROKER         "YOUR_MQTT_HOST"
#define MQTT_PORT           1883
#define MQTT_CLIENT_ID      "YOUR_PRODUCT_KEY.YOUR_DEVICE_NAME|securemode=2,signmethod=hmacsha256,timestamp=YOUR_TIMESTAMP|"
#define MQTT_USERNAME       "YOUR_DEVICE_NAME&YOUR_PRODUCT_KEY"
#define MQTT_PASSWORD       "YOUR_MQTT_PASSWORD"

uint8_t mqtt_connect(void);
uint8_t mqtt_publish(const char *topic, const char *payload);
uint8_t mqtt_is_connected(void);

#endif
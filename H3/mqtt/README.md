# MQTT Bridge for H3 IoT Gateway

## Overview

MQTT bridge service for connecting IoT devices that may not be always online.

## Features

- Support for MQTT v3.1/v3.1.1
- Persistent message storage
- Offline message queuing
- Support for up to 50 concurrent connections

## Installation

```bash
# Install mosquitto server
apt-get update && apt-get install -y mosquitto mosquitto-clients libmosquitto-dev

# Copy configuration
mkdir -p /etc/mosquitto/conf.d
cp mosquitto.conf /etc/mosquitto/conf.d/h3.conf

# Build and install MQTT bridge
gcc -o mqtt_bridge mqtt_bridge.c -lmosquitto -lpthread
cp mqtt_bridge /usr/local/bin/
```

## Configuration

### MQTT Broker
- Host: localhost
- Port: 1883
- Protocol: MQTT v3.1/v3.1.1
- Authentication: Disabled (allow_anonymous)

### Topics

| Topic | Direction | Description |
|-------|-----------|-------------|
| sensor/data/<device_id> | Device → Gateway | Sensor data from devices |
| sensor/control/<device_id> | Gateway → Device | Control commands to devices |

## Usage

### Start MQTT Service

```bash
# Start mosquitto server
systemctl start mosquitto

# Start MQTT bridge
/usr/local/bin/mqtt_bridge &
```

### Test MQTT

```bash
# Publish test message
mosquitto_pub -t "sensor/data/STM32_001" -m '{"temp":25.5,"humidity":60.0}'

# Subscribe to all sensor data
mosquitto_sub -t "sensor/data/#"
```

## Device MQTT Client Example

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

#define MQTT_BROKER "192.168.4.1"
#define MQTT_PORT 1883
#define DEVICE_ID "STM32_001"

void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    printf("Connected to MQTT broker\n");
}

void publish_sensor_data(struct mosquitto *mosq, float temp, float humidity) {
    char topic[64];
    char payload[128];
    
    snprintf(topic, sizeof(topic), "sensor/data/%s", DEVICE_ID);
    snprintf(payload, sizeof(payload), "{\"temp\":%.1f,\"humidity\":%.1f,\"timestamp\":%lld}", 
             temp, humidity, (long long)time(NULL));
    
    mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
    printf("Published: %s -> %s\n", topic, payload);
}

int main() {
    struct mosquitto *mosq;
    int rc;
    
    mosquitto_lib_init();
    mosq = mosquitto_new(DEVICE_ID, true, NULL);
    
    mosquitto_connect_callback_set(mosq, on_connect);
    
    rc = mosquitto_connect(mosq, MQTT_BROKER, MQTT_PORT, 60);
    if (rc != 0) {
        fprintf(stderr, "Connection failed\n");
        return 1;
    }
    
    publish_sensor_data(mosq, 25.5, 60.0);
    
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    
    return 0;
}
```

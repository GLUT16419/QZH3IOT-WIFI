#!/bin/bash
# Test MQTT connection
echo "Testing MQTT connection..."
mosquitto_sub -d -h "YOUR_MQTT_HOST" -p 1883 -i "YOUR_PRODUCT_KEY.YOUR_DEVICE_NAME|securemode=2,signmethod=hmacsha256,timestamp=YOUR_TIMESTAMP|" -u "YOUR_DEVICE_NAME&YOUR_PRODUCT_KEY" -P "YOUR_MQTT_PASSWORD" -t "/YOUR_PRODUCT_KEY/YOUR_DEVICE_NAME/user/get" 2>&1
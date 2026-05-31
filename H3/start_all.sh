#!/bin/bash

echo "Starting H3 IoT Gateway services..."

echo "1. Setting up WiFi AP..."
/root/H3/wifi/setup_wifi.sh

echo "2. Starting MQTT bridge..."
/root/H3/mqtt/mqtt_bridge &

echo "3. Starting web server..."
lighttpd -f /etc/lighttpd/lighttpd.conf &

echo ""
echo "=== Services Status ==="
echo "WiFi AP: SSID=H3-WIFI, IP=192.168.10.1"
echo "MQTT Bridge: Running"
echo "Web Server: Running on port 80"
echo ""
echo "All services started successfully!"

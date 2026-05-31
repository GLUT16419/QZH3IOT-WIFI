#!/bin/bash

SSID="H3-WIFI"
PASSWORD="password123"
INTERFACE="wlan0"

echo "Configuring WiFi access point..."

ip link set $INTERFACE down
ip addr flush dev $INTERFACE
ip addr add 192.168.10.1/24 dev $INTERFACE
ip link set $INTERFACE up

hostapd -B /root/H3/wifi/hostapd.conf

dnsmasq --interface=$INTERFACE --dhcp-range=192.168.10.10,192.168.10.100,255.255.255.0,24h

echo "WiFi AP started: $SSID"
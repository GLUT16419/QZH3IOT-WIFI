# H3 IoT Gateway Setup Guide

## Prerequisites
1. H3开发板已连接到网络或可以通过串口访问
2. 已安装必要的依赖：hostapd, dnsmasq

## Build and Install

```bash
cd /path/to/IOT-WIFI/H3
make all
make install
```

## Start Services Manually

### Method 1: Run the start script
```bash
/usr/local/bin/start_all.sh
```

### Method 2: Manual start

1. **Configure Ethernet interface:**
```bash
ifconfig eth0 down
ifconfig eth0 192.168.1.1 netmask 255.255.255.0
ifconfig eth0 up
```

2. **Configure WiFi interface:**
```bash
ifconfig wlan0 down
ifconfig wlan0 192.168.4.1 netmask 255.255.255.0
ifconfig wlan0 up
```

3. **Start hostapd:**
```bash
hostapd /etc/hostapd/hostapd.conf &
```

4. **Start dnsmasq:**
```bash
dnsmasq --interface=wlan0 --dhcp-range=192.168.4.10,192.168.4.50,255.255.255.0,24h &
```

5. **Start TCP server:**
```bash
/usr/local/bin/tcp_server &
```

## Network Configuration

### Ethernet (用于SSH管理)
- Gateway IP: 192.168.10.100
- Subnet Mask: 255.255.255.0

### WiFi (用于设备连接)
- SSID: H3_IoT_Gateway
- Password: gateway123
- Gateway IP: 192.168.4.1
- Subnet Mask: 255.255.255.0

### Common
- TCP Server Port: 8888
- MQTT Broker Port: 1883
- Max Clients: 32 (TCP) / 50 (MQTT)
- Client Timeout: 10 minutes

## MQTT Support (Recommended)

MQTT is recommended for devices that may not be always online.

### Features:
- **Publish/Subscribe pattern**: Efficient message routing
- **Persistent storage**: Messages are stored when devices are offline
- **QoS levels**: Support for reliable message delivery
- **Last Will and Testament**: Detect device disconnection

### MQTT Topics

| Topic | Direction | Description |
|-------|-----------|-------------|
| sensor/data/<device_id> | Device → Gateway | Sensor data from devices |
| sensor/control/<device_id> | Gateway → Device | Control commands to devices |

### MQTT Configuration
- Broker IP: 192.168.4.1 (WiFi) / 192.168.10.100 (Ethernet)
- Port: 1883
- Protocol: MQTT v3.1/v3.1.1
- Authentication: Disabled

### Install MQTT

```bash
# Install mosquitto server
apt-get update && apt-get install -y mosquitto mosquitto-clients libmosquitto-dev

# Copy configuration
mkdir -p /etc/mosquitto/conf.d
cp /root/H3/mqtt/mosquitto.conf /etc/mosquitto/conf.d/h3.conf

# Build MQTT bridge
gcc -o /usr/local/bin/mqtt_bridge /root/H3/mqtt/mqtt_bridge.c -lmosquitto -lpthread
```

## Multi-Device Support

H3网关支持同时连接多个STM32设备：

### Features:
- **Maximum 32 devices**: Support up to 32 concurrent connections
- **Device identification**: Each device sends a unique DEVICE_ID upon connection
- **Data isolation**: Data from each device is stored in separate directories under `/tmp/sensor_data/<device_id>/`
- **Device tracking**: Connected devices are tracked in `/tmp/sensor_data/devices.txt`
- **Automatic timeout**: Inactive clients (no data for 10 minutes) are automatically disconnected

### Device Registration

When a device connects, it must send its device ID first:

```
DEVICE_ID:STM32_001
```

### Data Storage Structure

```
/tmp/sensor_data/
├── devices.txt          # List of connected devices
├── STM32_001/
│   ├── 1716789000.json  # Timestamp-based data files
│   ├── 1716789005.json
│   └── ...
├── STM32_002/
│   └── ...
└── ...
```

## Verify Services

```bash
# Check Ethernet interface
ifconfig eth0

# Check WiFi interface
ifconfig wlan0

# Check running services
ps aux | grep -E '(hostapd|dnsmasq|tcp_server)'

# Check network connectivity
ping 192.168.1.1
ping 192.168.4.1
```

## Auto-start on Boot

Add the following to /etc/rc.local before `exit 0`:

```bash
/usr/local/bin/start_all.sh
```

## Access Web Interface

通过浏览器访问：
- Ethernet: http://192.168.1.1
- WiFi: http://192.168.4.1

## Troubleshooting

### Ethernet not working
1. Check Ethernet cable connection
2. Check Ethernet interface status: `ifconfig eth0`
3. Check if Ethernet driver is loaded: `lsmod | grep ethernet`

### WiFi AP not starting
1. Check if hostapd is installed: `which hostapd`
2. Check WiFi driver: `lsmod | grep xradio`
3. Check hostapd logs: `hostapd /etc/hostapd/hostapd.conf` (run in foreground)

### TCP server not starting
1. Check if tcp_server binary exists: `ls -la /usr/local/bin/tcp_server`
2. Check port 8888: `netstat -tlnp | grep 8888`

### STM32 cannot connect
1. Verify STM32 network configuration matches gateway IP
2. Check Ethernet/WiFi connection status
3. Verify TCP server is listening on port 8888
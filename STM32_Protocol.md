# STM32 与 H3网关 通信协议文档

## 1. 概述

本文档描述STM32数据采集终端与H3物联网网关之间的通信协议，支持TCP和MQTT两种协议，包括网络连接、数据帧格式、数据类型定义等内容。MQTT协议特别适用于设备不一定实时在线的场景。

---

## 2. 网络连接

### 2.1 连接方式

STM32支持两种连接方式，可以通过以太网或WiFi连接到H3网关：

**方式一：WiFi连接（推荐）**

| 配置项 | 值 |
|--------|----|
| WiFi模式 | Station模式 |
| 目标SSID | H3_IoT_Gateway |
| WiFi密码 | gateway123 |
| H3网关IP | 192.168.4.1 |
| 子网掩码 | 255.255.255.0 |
| TCP端口 | 8888 |

**方式二：以太网连接**

| 配置项 | 值 |
|--------|----|
| 连接方式 | 以太网直连 |
| STM32 IP地址 | 192.168.10.x (x范围: 2-254) |
| H3网关IP | 192.168.10.100 |
| 子网掩码 | 255.255.255.0 |
| TCP端口 | 8888 |

**方式三：MQTT连接（推荐，支持离线设备）**

| 配置项 | 值 |
|--------|----|
| 协议类型 | MQTT 3.1.1 |
| H3网关IP | 192.168.4.1 (WiFi) 或 192.168.10.100 (以太网) |
| MQTT端口 | 1883 |
| QoS级别 | 0/1/2（建议使用QoS 1） |
| 客户端ID | STM32_XXX（与设备ID一致） |

### 2.2 连接流程

```
STM32                              H3 Gateway
  │                                      │
  │─── 1. 初始化以太网接口 ─────────────▶│
  │                                      │
  │─── 2. TCP连接请求 (SYN) ───────────▶│
  │                                      │
  │◀─── 3. TCP连接确认 (SYN+ACK) ───────│
  │                                      │
  │─── 4. 发送设备标识 ─────────────────▶│
  │     "DEVICE_ID:STM32_XXX"           │
  │                                      │
  │─── 5. 定时发送传感器数据 ───────────▶│
  │                                      │
  │◀─── 6. 发送确认帧 (ACK) ────────────│
```

### 2.3 重连机制

- 连接断开后自动重试，重试间隔：1秒 → 2秒 → 4秒 → 8秒（最大8秒）
- 数据缓存：断开期间数据缓存到本地，恢复连接后补发

---

## 3. 数据帧格式

### 3.1 帧结构（STM32 → H3）

| 字段 | 字节数 | 说明 | 值 |
|------|--------|------|----|
| 帧头 | 2 | 帧起始标记 | 0xAA 0xBB |
| 设备ID | 4 | 唯一设备标识 | 32位整数 |
| 数据类型 | 1 | 传感器类型 | 见表3.2 |
| 数据长度 | 2 | 后续数据字节数 | 大端序 |
| 数据内容 | N | 传感器数据 | 具体格式见表3.2 |
| 校验和 | 2 | CRC16校验 | 大端序 |
| 帧尾 | 2 | 帧结束标记 | 0xCC 0xDD |

### 3.2 数据类型定义

| 类型码 | 传感器类型 | 数据格式 | 数据长度 | 示例 |
|-------|-----------|---------|---------|------|
| 0x01 | 温湿度 | float temp, float humidity | 8字节 | 25.5°C, 60% |
| 0x02 | 光照强度 | uint32 lux | 4字节 | 1000 lx |
| 0x03 | 气压 | float hPa | 4字节 | 1013.25 hPa |
| 0x04 | 空气质量 | uint8 level | 1字节 | 0-5级 |
| 0x05 | 继电器控制 | uint8 state | 1字节 | 0=关, 1=开 |
| 0x06 | 自定义数据 | 可变长度 | N字节 | 预留 |

### 3.3 帧格式示例

**温湿度数据帧（类型0x01）**：
```
字节: 0xAA 0xBB 0x00 0x00 0x00 0x01 0x01 0x00 0x08 0x41 0xC8 0x00 0x00 0x42 0x70 0x00 0x00 [CRC] 0xCC 0xDD
      ↑    ↑    ↑         ↑      ↑    ↑    ↑         ↑                    ↑         ↑
    帧头   设备ID(0x00000001) 类型 长度  温度(25.5)   湿度(60.0)      校验和   帧尾
```

### 3.4 CRC16算法

采用标准CRC16-CCITT算法：
- 初始值：0xFFFF
- 多项式：0xA001
- 校验范围：设备ID + 数据类型 + 数据长度 + 数据内容

```c
uint16_t crc16(const uint8_t *data, int length) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (crc & 1 ? 0xA001 : 0);
        }
    }
    return crc;
}
```

---

## 4. 数据格式详解

### 4.1 温湿度数据（类型0x01）

| 字节偏移 | 字段 | 类型 | 说明 |
|---------|------|------|------|
| 0-3 | 温度 | float (IEEE 754) | 单位：摄氏度 |
| 4-7 | 湿度 | float (IEEE 754) | 单位：百分比 |

### 4.2 光照强度（类型0x02）

| 字节偏移 | 字段 | 类型 | 说明 |
|---------|------|------|------|
| 0-3 | 光照值 | uint32 | 单位：lux，大端序 |

### 4.3 气压数据（类型0x03）

| 字节偏移 | 字段 | 类型 | 说明 |
|---------|------|------|------|
| 0-3 | 气压值 | float (IEEE 754) | 单位：hPa |

### 4.4 空气质量（类型0x04）

| 字节偏移 | 字段 | 类型 | 说明 |
|---------|------|------|------|
| 0 | 等级 | uint8 | 0-5级，0=优，5=严重污染 |

### 4.5 继电器控制（类型0x05）

| 字节偏移 | 字段 | 类型 | 说明 |
|---------|------|------|------|
| 0 | 状态 | uint8 | 0=关闭，1=打开 |

---

## 5. 通信协议示例代码

### 5.1 STM32端数据打包函数

```c
#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t device_id;
    uint8_t data_type;
    uint16_t data_length;
    uint8_t data[64];
} SensorData;

uint16_t crc16(const uint8_t *data, int length) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (crc & 1 ? 0xA001 : 0);
        }
    }
    return crc;
}

int pack_frame(SensorData *data, uint8_t *buffer, int max_len) {
    if (max_len < 11 + data->data_length) return -1;
    
    int offset = 0;
    buffer[offset++] = 0xAA;
    buffer[offset++] = 0xBB;
    
    buffer[offset++] = (data->device_id >> 24) & 0xFF;
    buffer[offset++] = (data->device_id >> 16) & 0xFF;
    buffer[offset++] = (data->device_id >> 8) & 0xFF;
    buffer[offset++] = data->device_id & 0xFF;
    
    buffer[offset++] = data->data_type;
    
    buffer[offset++] = (data->data_length >> 8) & 0xFF;
    buffer[offset++] = data->data_length & 0xFF;
    
    memcpy(buffer + offset, data->data, data->data_length);
    offset += data->data_length;
    
    uint16_t crc = crc16(buffer + 2, offset - 2);
    buffer[offset++] = (crc >> 8) & 0xFF;
    buffer[offset++] = crc & 0xFF;
    
    buffer[offset++] = 0xCC;
    buffer[offset++] = 0xDD;
    
    return offset;
}

void send_temperature_humidity(int sock, uint32_t device_id, float temp, float humidity) {
    SensorData data;
    uint8_t buffer[128];
    
    data.device_id = device_id;
    data.data_type = 0x01;
    data.data_length = 8;
    
    memcpy(data.data, &temp, 4);
    memcpy(data.data + 4, &humidity, 4);
    
    int len = pack_frame(&data, buffer, sizeof(buffer));
    send(sock, buffer, len, 0);
}
```

### 5.2 以太网初始化

```c
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"

struct netif gnetif;

void ethernet_init(void) {
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;
    
    IP4_ADDR(&ipaddr, 192, 168, 1, 10);     // STM32 IP地址
    IP4_ADDR(&netmask, 255, 255, 255, 0);    // 子网掩码
    IP4_ADDR(&gw, 192, 168, 1, 1);           // 网关IP
    
    netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
    netif_set_default(&gnetif);
    netif_set_up(&gnetif);
}
```

### 5.3 设备ID注册

连接成功后，STM32需要发送设备标识：

```c
void send_device_id(int sock, const char *device_id) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "DEVICE_ID:%s\n", device_id);
    send(sock, buffer, strlen(buffer), 0);
}
```

---

## 6. H3端响应协议

### 6.1 确认帧格式

H3网关收到数据后，返回简单的ACK确认：

| 响应类型 | 格式 | 说明 |
|---------|------|------|
| 数据确认 | `ACK\n` | 数据接收成功 |
| 错误响应 | `ERROR:XXX\n` | 数据解析失败 |

### 6.2 响应示例

```
STM32发送数据帧 ─────────────▶ H3
                             │
H3返回 ─────────────────────▶ STM32
"ACK\n"
```

---

## 7. 数据采集周期

| 传感器类型 | 默认周期 | 可配置范围 |
|-----------|---------|-----------|
| 温湿度 | 5秒 | 1-60秒 |
| 光照强度 | 10秒 | 1-60秒 |
| 气压 | 30秒 | 5-300秒 |
| 空气质量 | 60秒 | 10-600秒 |

---

## 8. 错误处理

### 8.1 帧错误类型

| 错误码 | 错误类型 | 说明 |
|-------|---------|------|
| -1 | 帧长度不足 | 接收到的数据长度小于最小帧长(11字节) |
| -2 | 帧头错误 | 帧头不是0xAA 0xBB |
| -3 | 帧尾错误 | 帧尾不是0xCC 0xDD |
| -4 | CRC校验失败 | 校验和不匹配 |
| -5 | 数据类型未知 | 数据类型码不在定义范围内 |

### 8.2 错误处理策略

1. **CRC校验失败**：丢弃该帧，不发送ACK
2. **帧格式错误**：记录日志，等待下一帧
3. **连接超时**：超过30秒无数据，断开连接

---

## 9. 安全性

### 9.1 数据完整性

- 采用CRC16校验确保数据完整性
- 帧头帧尾标识防止数据错位

### 9.2 设备识别

- 每个STM32终端分配唯一设备ID
- 设备ID在连接时注册

---

## 10. 附录

### 10.1 字节序说明

所有多字节数据采用**大端序（Big-Endian）**：
- uint16: 高字节在前，低字节在后
- uint32: 最高字节在前，最低字节在后
- float: IEEE 754标准，大端序

### 10.2 设备ID分配规则

设备ID采用32位整数，建议分配规则：
- STM32_001 → 0x00000001
- STM32_002 → 0x00000002
- ...

---

## 11. MQTT协议规范

### 11.1 MQTT主题定义

| 主题 | 方向 | QoS | 说明 |
|------|------|-----|------|
| sensor/data/<device_id> | 设备→网关 | 1 | 传感器数据上报 |
| sensor/control/<device_id> | 网关→设备 | 1 | 设备控制命令 |
| sensor/status/<device_id> | 设备→网关 | 0 | 设备状态上报 |

### 11.2 MQTT数据格式

传感器数据JSON格式：
```json
{
    "device_id": "STM32_001",
    "timestamp": 1779864678,
    "data_type": 0x01,
    "data": {
        "temp": 25.5,
        "humidity": 60.0
    }
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| device_id | string | 设备标识 |
| timestamp | uint32 | 时间戳（UNIX时间） |
| data_type | uint8 | 数据类型（同3.2节） |
| data | object | 传感器数据 |

### 11.3 MQTT控制命令格式

```json
{
    "command": "relay_control",
    "channel": 1,
    "state": 1
}
```

| 命令 | 参数 | 说明 |
|------|------|------|
| relay_control | channel, state | 继电器控制，state=0关闭，state=1打开 |
| set_interval | sensor_type, interval | 设置采集周期（秒） |
| get_status | - | 获取设备状态 |

### 11.4 MQTT优势

| 特性 | TCP | MQTT |
|------|-----|------|
| 离线支持 | 无 | ✅ 支持（QoS 1/2） |
| 消息持久化 | 无 | ✅ 支持 |
| 带宽占用 | 高 | 低 |
| 发布/订阅 | 无 | ✅ 支持 |
| 一对多通信 | 需实现 | ✅ 原生支持 |

### 11.5 MQTT示例代码

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mqtt_client.h"

#define MQTT_BROKER "192.168.4.1"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "STM32_001"

void on_mqtt_connect(int rc) {
    printf("MQTT Connected: %s\n", rc == 0 ? "success" : "failed");
}

void on_mqtt_message(char* topic, char* payload) {
    printf("Received message: %s -> %s\n", topic, payload);
}

void mqtt_send_sensor_data(float temp, float humidity) {
    char topic[64];
    char payload[128];
    
    snprintf(topic, sizeof(topic), "sensor/data/%s", MQTT_CLIENT_ID);
    snprintf(payload, sizeof(payload), 
             "{\"device_id\":\"%s\",\"timestamp\":%lld,\"data_type\":1,\"data\":{\"temp\":%.1f,\"humidity\":%.1f}}",
             MQTT_CLIENT_ID, (long long)time(NULL), temp, humidity);
    
    mqtt_publish(topic, payload, strlen(payload), 1, 0);
}

int main() {
    mqtt_client_t client;
    
    mqtt_client_init(&client, MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);
    mqtt_set_callback(&client, on_mqtt_connect, on_mqtt_message);
    mqtt_connect(&client);
    
    while (1) {
        float temp = 25.5;
        float humidity = 60.0;
        
        mqtt_send_sensor_data(temp, humidity);
        mqtt_loop(&client);
        sleep(5);
    }
    
    return 0;
}
```

---

**文档版本**: v1.2  
**创建日期**: 2026-05-22  
**更新日期**: 2026-05-27  
**更新内容**: 添加MQTT协议支持，支持离线设备通信  
**适用项目**: H3物联网网关项目
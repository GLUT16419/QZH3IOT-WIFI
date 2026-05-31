# IOT-WIFI - 嵌入式物联网网关系统

基于嵌入式Linux的物联网网关项目，由H3网关、CH32V307和STM32F407采集终端组成，设备通过ESP8266 WiFi模块连接阿里云IoT平台。

## 项目概述

本项目实现了完整的物联网数据采集、存储、可视化系统：

- **H3网关**：作为数据汇聚点，运行MQTT桥接程序和Web服务
- **CH32V307采集终端**：基于FreeRTOS，通过ESP8266连接WiFi
- **STM32F407采集终端**：基于FreeRTOS和HAL库，通过ESP8266连接WiFi
- **阿里云IoT平台**：云端数据中转和设备管理

## 项目结构

```
IOT-WIFI/
├── H3/                           # H3网关端
│   ├── cgi-bin/                 # CGI程序
│   │   ├── sensor_data.c        # 传感器数据处理CGI
│   │   └── device_mgmt.c        # 设备管理CGI
│   ├── html/                    # Web界面
│   │   ├── index.html           # 主页面
│   │   ├── style.css            # 样式文件
│   │   └── main.js              # 前端逻辑
│   ├── mqtt/                    # MQTT桥接程序
│   │   ├── mqtt_bridge.c        # MQTT桥接器
│   │   ├── mosquitto.conf       # Mosquitto配置
│   │   └── README.md            # MQTT说明
│   ├── Makefile                 # 编译脚本
│   └── README.md                # H3网关说明
├── CH32V307/                    # CH32V307采集终端
│   └── CH32V307-FreeRTOS/       # FreeRTOS固件
│       └── User/main.c          # 主程序
├── STM32F407/                   # STM32F407采集终端
│   └── F407/                    # 固件项目
│       ├── Core/
│       │   ├── Inc/             # 头文件
│       │   │   ├── mqtt_client.h
│       │   │   └── esp8266.h
│       │   └── Src/             # 源文件
│       │       ├── main.c
│       │       ├── mqtt_client.c
│       │       └── esp8266.c
├── STM32_Protocol.md           # 通信协议文档
├── README.md                   # 项目说明
└── test_mqtt.sh                # MQTT测试脚本
```

## 功能特性

- **多终端数据采集**：支持CH32V307和STM32F407两个采集终端
- **ESP8266 WiFi通信**：通过AT指令控制ESP8266模块连接WiFi
- **MQTT协议对接阿里云**：设备直接连接阿里云IoT平台
- **实时数据可视化**：Web界面展示实时传感器数据
- **设备管理**：自动发现新设备，在线状态监控
- **历史数据查询**：按设备和时间范围查询历史数据
- **数据统计图表**：温度、湿度等数据趋势图
- **自动重连机制**：网络断线后自动恢复连接

## 技术栈

| 分类 | 技术/组件 |
|------|-----------|
| 网关芯片 | Allwinner H3 (ARM Cortex-A7) |
| MCU芯片 | CH32V307VCT6 (RISC-V) / STM32F407 (ARM Cortex-M4) |
| 网关系统 | 嵌入式Linux (Buildroot) |
| MCU RTOS | FreeRTOS |
| WiFi模块 | ESP8266 |
| Web服务器 | lighttpd + CGI |
| MQTT代理 | Mosquitto |
| 云平台 | 阿里云IoT平台 |
| 编程语言 | C语言 |
| 前端技术 | HTML5 + CSS3 + JavaScript |

## 配置说明

使用前需要修改以下配置文件中的占位符为实际值：

### H3网关端

**H3/mqtt/mqtt_bridge.c**:
```c
#define MQTT_BROKER "YOUR_MQTT_HOST"
#define MQTT_CLIENT_ID "YOUR_PRODUCT_KEY.YOUR_DEVICE_NAME|securemode=2,signmethod=hmacsha256,timestamp=YOUR_TIMESTAMP|"
#define MQTT_USERNAME "YOUR_DEVICE_NAME&YOUR_PRODUCT_KEY"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
```

### CH32V307终端

**CH32V307/CH32V307-FreeRTOS/User/main.c**:
```c
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define MQTT_BROKER "YOUR_MQTT_HOST"
#define MQTT_CLIENT_ID "YOUR_PRODUCT_KEY.YOUR_DEVICE_NAME|securemode=2,signmethod=hmacsha256,timestamp=YOUR_TIMESTAMP|"
#define MQTT_USERNAME "YOUR_DEVICE_NAME&YOUR_PRODUCT_KEY"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
```

### STM32F407终端

**STM32F407/F407/Core/Inc/esp8266.h**:
```c
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

**STM32F407/F407/Core/Inc/mqtt_client.h**:
```c
#define MQTT_BROKER "YOUR_MQTT_HOST"
#define MQTT_CLIENT_ID "YOUR_PRODUCT_KEY.YOUR_DEVICE_NAME|securemode=2,signmethod=hmacsha256,timestamp=YOUR_TIMESTAMP|"
#define MQTT_USERNAME "YOUR_DEVICE_NAME&YOUR_PRODUCT_KEY"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
```

## 编译安装

### H3网关

```bash
cd H3
make
make install
make clean
```

### CH32V307

使用MounRiver Studio打开项目 `CH32V307/CH32V307-FreeRTOS/` 进行编译。

### STM32F407

使用Keil MDK打开项目 `STM32F407/F407/F407.uvprojx` 进行编译。

## 启动服务

### H3网关

```bash
# 编译MQTT桥接程序
cd H3/mqtt
gcc mqtt_bridge.c -o mqtt_bridge -lmosquitto -lpthread

# 后台运行MQTT桥接器
./mqtt_bridge -f &

# 启动lighttpd Web服务
/etc/init.d/lighttpd start
```

### MCU终端

烧录固件后，模块会自动：
1. 初始化ESP8266
2. 连接配置的WiFi热点
3. 连接阿里云IoT平台
4. 周期采集并发布传感器数据

## Web访问

1. 确保H3网关连接到局域网
2. 在浏览器中访问网关IP地址
3. 查看实时传感器数据和设备状态

## MQTT数据格式

设备发布数据格式：
```json
{
  "temperature": 25.5,
  "humidity": 60.2,
  "device": "CH32V307"
}
```

## 阿里云IoT配置

1. 在阿里云IoT平台创建产品和设备
2. 获取设备三元组（ProductKey、DeviceName、DeviceSecret）
3. 使用阿里云官方工具生成MQTT连接参数
4. 将参数填入各设备的配置文件

## 注意事项

- ESP8266模块初始化需要额外的延迟，确保稳定启动
- WiFi热点需要支持2.4GHz频段
- 阿里云MQTT连接密码具有时效性，需要定期更新
- 建议使用守护进程方式运行MQTT桥接程序

## 硬件连接

### ESP8266引脚配置

| ESP8266 | MCU |
|---------|-----|
| TX | USART RX |
| RX | USART TX |
| VCC | 3.3V |
| GND | GND |

## 许可证

本项目仅供学习和研究使用。

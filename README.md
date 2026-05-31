# H3 IoT Gateway - 本地WiFi直连版

基于嵌入式Linux的物联网网关项目，无需云端平台中转，用户直接连接H3网关的WiFi即可访问设备和查看数据。

## 项目结构

```
IOT-WIFI/
├── cgi-bin/          # CGI程序
│   ├── sensor_data.c # 传感器数据处理CGI
│   └── device_mgmt.c # 设备管理CGI
├── html/             # Web界面
│   ├── index.html    # 主页面
│   ├── style.css     # 样式文件
│   └── main.js       # 前端逻辑
├── server/           # TCP服务端
│   ├── tcp_server.c  # TCP服务程序
│   └── data_parser.c # 数据解析模块
├── wifi/             # WiFi配置
│   ├── hostapd.conf  # hostapd配置文件
│   └── setup_wifi.sh # WiFi设置脚本
├── Makefile          # 编译脚本
└── README.md         # 项目说明
```

## 功能特性

- **实时数据采集**：支持温湿度、光照、气压、空气质量等传感器
- **TCP数据接收**：多客户端并发连接，最大支持10个STM32终端
- **本地数据存储**：JSON格式存储，自动清理7天前历史数据
- **Web管理界面**：实时数据展示、设备管理、历史数据查询
- **WiFi直连**：H3网关作为AP热点，无需互联网即可使用

## 默认配置

| 配置项 | 默认值 |
|--------|--------|
| WiFi SSID | H3_IoT_Gateway |
| WiFi密码 | gateway123 |
| 网关IP | 192.168.1.1 |
| Web端口 | 80 |
| TCP端口 | 8888 |

## 编译安装

```bash
# 交叉编译
make

# 安装到开发板
make install

# 清理编译文件
make clean
```

## 启动服务

```bash
# 启动TCP服务（后台运行）
tcp_server &

# 启动WiFi AP
setup_wifi.sh

# 启动lighttpd（如未自动启动）
/etc/init.d/lighttpd start
```

## 连接方式

1. 启动H3网关，等待WiFi热点启动
2. 用户终端（手机/电脑）搜索并连接WiFi：`H3_IoT_Gateway`
3. 在浏览器中访问：`http://192.168.1.1`
4. 查看实时传感器数据

## 数据格式

传感器数据JSON格式：

```json
{
  "device_id": "STM32_001",
  "timestamp": 1699999999,
  "sensors": [
    {
      "type": "temperature",
      "value": 25.5,
      "unit": "°C"
    },
    {
      "type": "humidity",
      "value": 60.0,
      "unit": "%"
    }
  ]
}
```

## 通信协议

数据帧格式（STM32 → H3）：

| 字段 | 字节数 | 说明 |
|------|--------|------|
| 帧头 | 2 | 0xAA 0xBB |
| 设备ID | 4 | 唯一标识 |
| 数据类型 | 1 | 传感器类型 |
| 数据长度 | 2 | 后续数据字节数 |
| 数据内容 | N | 传感器数据 |
| 校验和 | 2 | CRC16 |
| 帧尾 | 2 | 0xCC 0xDD |

数据类型定义：

| 类型码 | 传感器类型 |
|-------|-----------|
| 0x01 | 温湿度 |
| 0x02 | 光照强度 |
| 0x03 | 气压 |
| 0x04 | 空气质量 |

## 技术栈

- **硬件**：Allwinner H3 (ARM Cortex-A7)
- **操作系统**：Linux (Buildroot)
- **Web服务器**：lighttpd + CGI
- **编程语言**：C语言
- **网络协议**：TCP/IP, WiFi AP模式
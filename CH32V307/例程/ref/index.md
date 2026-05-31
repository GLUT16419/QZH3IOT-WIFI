# CH32V307 外设参考文档索引

根据所需功能选择对应模块文档，提供给AI编写代码。

## 模块列表

| 文档 | 功能 | 适用场景 |
|------|------|----------|
| [01_基础系统.md](01_基础系统.md) | 芯片定义、基础配置、debug、NVIC、IRQ号 | 任何项目的基础初始化 |
| [02_RCC时钟.md](02_RCC时钟.md) | 时钟控制(HSE/PLL/外设时钟) | 配置系统时钟、使能外设时钟 |
| [03_GPIO.md](03_GPIO.md) | GPIO完整API | LED、按键、普通IO控制 |
| [04_USART.md](04_USART.md) | 串口通信 | 串口收发、printf输出 |
| [05_TIM定时器.md](05_TIM定时器.md) | 定时器/PWM/输入捕获 | 定时任务、PWM输出、频率测量 |
| [06_EXTI外部中断.md](06_EXTI外部中断.md) | 外部中断 | 按键中断、边沿触发 |
| [07_DMA.md](07_DMA.md) | DMA传输 | 内存拷贝、外设数据批量传输 |
| [08_ADC.md](08_ADC.md) | 模数转换 | 模拟信号采集 |
| [09_SPI.md](09_SPI.md) | SPI总线 | 显示屏、FLASH、传感器 |
| [10_I2C.md](10_I2C.md) | I2C总线 | EEPROM、传感器 |
| [11_ETH以太网.md](11_ETH以太网.md) | 以太网+WCHNET协议栈 | TCP/UDP通信、MQTT、WebServer |
| [12_其他外设.md](12_其他外设.md) | PWR/RTC/IWDG/BKP/CRC/RNG/FPU/FLASH | 低功耗、RTC、看门狗、CRC等 |
| [13_引脚复用.md](13_引脚复用.md) | 引脚复用表、芯片型号差异 | 硬件设计时查询引脚功能 |

## 使用方法

```markdown
向 AI 提问时附带对应模块文件的完整内容。
例如：要实现 TCP 服务器，提供 11_ETH以太网.md 给 AI。
```

---

*芯片：CH32V307VCT6 (RISC-V RV32IMAC, 144MHz)*  
*编译选项：`-march=rv32imac -mabi=ilp32` 或 MounRiverStudio 默认配置*

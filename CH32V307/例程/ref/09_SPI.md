# SPI 总线模块

包含：数据结构、主从模式/时钟极性/波特率宏、函数原型、典型用法。

---

## 1. 头文件

```c
#include "ch32v30x_spi.h"   // 文件: Peripheral/inc/ch32v30x_spi.h
```

## 2. 数据结构

```c
typedef struct {
    uint16_t SPI_Direction;         // 双/单工
    uint16_t SPI_Mode;              // 主/从
    uint16_t SPI_DataSize;          // 8/16位
    uint16_t SPI_CPOL;              // 时钟极性
    uint16_t SPI_CPHA;              // 时钟相位
    uint16_t SPI_NSS;               // NSS管理
    uint16_t SPI_BaudRatePrescaler; // 波特率预分频
    uint16_t SPI_FirstBit;          // MSB/LSB
    uint16_t SPI_CRCPolynomial;     // CRC多项式
} SPI_InitTypeDef;
```

## 3. 关键宏

```c
// 方向
#define SPI_Direction_2Lines_FullDuplex ((uint16_t)0x0000)  // 双线全双工
#define SPI_Direction_2Lines_RxOnly     ((uint16_t)0x0400)  // 双线只接收
#define SPI_Direction_1Line_Rx          ((uint16_t)0x8000)  // 单线接收
#define SPI_Direction_1Line_Tx          ((uint16_t)0xC000)  // 单线发送

// 主从
#define SPI_Mode_Master ((uint16_t)0x0104)
#define SPI_Mode_Slave  ((uint16_t)0x0000)

// 数据大小
#define SPI_DataSize_16b ((uint16_t)0x0800)
#define SPI_DataSize_8b  ((uint16_t)0x0000)

// 时钟极性 (CPOL)
#define SPI_CPOL_Low  ((uint16_t)0x0000)
#define SPI_CPOL_High ((uint16_t)0x0002)

// 时钟相位 (CPHA)
#define SPI_CPHA_1Edge ((uint16_t)0x0000)  // 第一个边沿采样
#define SPI_CPHA_2Edge ((uint16_t)0x0001)  // 第二个边沿采样

// NSS管理
#define SPI_NSS_Soft ((uint16_t)0x0200)  // 软件NSS
#define SPI_NSS_Hard ((uint16_t)0x0000)  // 硬件NSS

// 波特率预分频
#define SPI_BaudRatePrescaler_2   ((uint16_t)0x0000)
#define SPI_BaudRatePrescaler_4   ((uint16_t)0x0008)
#define SPI_BaudRatePrescaler_8   ((uint16_t)0x0010)
#define SPI_BaudRatePrescaler_16  ((uint16_t)0x0018)
#define SPI_BaudRatePrescaler_32  ((uint16_t)0x0020)
#define SPI_BaudRatePrescaler_64  ((uint16_t)0x0028)
#define SPI_BaudRatePrescaler_128 ((uint16_t)0x0030)
#define SPI_BaudRatePrescaler_256 ((uint16_t)0x0038)

// 位顺序
#define SPI_FirstBit_MSB ((uint16_t)0x0000)
#define SPI_FirstBit_LSB ((uint16_t)0x0080)

// 标志
#define SPI_FLAG_TXE ((uint16_t)0x0002)  // 发送缓冲空
#define SPI_FLAG_RXNE ((uint16_t)0x0001) // 接收缓冲非空
#define SPI_FLAG_BSY ((uint16_t)0x0080)  // 忙
```

## 4. SPI端口

```c
SPI_TypeDef *SPI1;  // PA5(SCK), PA6(MISO), PA7(MOSI), PA4(NSS)
SPI_TypeDef *SPI2;  // PB13(SCK), PB14(MISO), PB15(MOSI)
```

## 5. 函数原型

```c
void SPI_DeInit(SPI_TypeDef* SPIx);
void SPI_Init(SPI_TypeDef* SPIx, SPI_InitTypeDef* SPI_InitStruct);
void SPI_Cmd(SPI_TypeDef* SPIx, FunctionalState NewState);
void SPI_ITConfig(SPI_TypeDef* SPIx, uint8_t SPI_IT, FunctionalState NewState);
uint16_t SPI_ReadWriteData(SPI_TypeDef* SPIx, uint16_t Data);
void SPI_ReadDataR8(SPI_TypeDef* SPIx, void* buf, uint32_t len);
void SPI_WriteDataR8(SPI_TypeDef* SPIx, void* buf, uint32_t len);
void SPI_ReadDataR16(SPI_TypeDef* SPIx, void* buf, uint32_t len);
void SPI_WriteDataR16(SPI_TypeDef* SPIx, void* buf, uint32_t len);
FlagStatus SPI_GetFlagStatus(SPI_TypeDef* SPIx, uint16_t SPI_FLAG);
void SPI_ClearFlag(SPI_TypeDef* SPIx, uint16_t SPI_FLAG);
```

## 6. 典型用法 - SPI主模式

```c
void SPI1_Master_Init(void) {
    SPI_InitTypeDef SPI_InitStructure = {0};
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitTypeDef GPIO_NSS_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // SCK(PA5), MOSI(PA7) 复用推挽
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // MISO(PA6) 浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // NSS(PA4) 推挽输出 (软件NSS)
    GPIO_NSS_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_NSS_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_NSS_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_NSS_InitStructure);
    GPIO_SetBits(GPIOA, GPIO_Pin_4);  // NSS高=空闲

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);
}

uint8_t SPI_ReadWriteByte(uint8_t data) {
    while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);
    SPI_ReadWriteData(SPI1, data);
    while (SPI_GetFlagStatus(SPI1, SPI_FLAG_RXNE) == RESET);
    return (uint8_t)SPI_ReadWriteData(SPI1, 0);
}

// 片选控制
#define SPI_CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define SPI_CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)
```

---

*对应文件路径: Peripheral/inc/ch32v30x_spi.h*

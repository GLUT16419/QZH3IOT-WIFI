# USART 串口模块

包含：完整数据结构、波特率/校验/停止位宏、函数原型、典型用法。

---

## 1. 头文件

```c
#include "ch32v30x_usart.h"   // 文件: Peripheral/inc/ch32v30x_usart.h
```

## 2. 数据结构

```c
typedef struct {
    uint32_t USART_BaudRate;             // 波特率 (1200 ~ 921600)
    uint16_t USART_WordLength;           // 数据位: 8b 或 9b
    uint16_t USART_StopBits;             // 停止位: 1, 0.5, 2, 1.5
    uint16_t USART_Parity;               // 校验: No, Even, Odd
    uint16_t USART_Mode;                 // 模式: Rx, Tx
    uint16_t USART_HardwareFlowControl;  // 流控: None, RTS, CTS, RTS_CTS
} USART_InitTypeDef;

typedef struct {
    uint16_t USART_Clock;    // 时钟使能
    uint16_t USART_CPOL;     // 时钟极性
    uint16_t USART_CPHA;     // 时钟相位
    uint16_t USART_LastBit;  // 最后一位时钟
} USART_ClockInitTypeDef;
```

## 3. 关键宏

```c
// 数据位
#define USART_WordLength_8b  ((uint16_t)0x0000)
#define USART_WordLength_9b  ((uint16_t)0x1000)

// 停止位
#define USART_StopBits_1    ((uint16_t)0x0000)
#define USART_StopBits_0_5  ((uint16_t)0x1000)
#define USART_StopBits_2    ((uint16_t)0x2000)
#define USART_StopBits_1_5  ((uint16_t)0x3000)

// 校验
#define USART_Parity_No    ((uint16_t)0x0000)
#define USART_Parity_Even  ((uint16_t)0x0400)
#define USART_Parity_Odd   ((uint16_t)0x0600)

// 模式
#define USART_Mode_Rx  ((uint16_t)0x0004)
#define USART_Mode_Tx  ((uint16_t)0x0008)

// 流控
#define USART_HardwareFlowControl_None    ((uint16_t)0x0000)
#define USART_HardwareFlowControl_RTS     ((uint16_t)0x0100)
#define USART_HardwareFlowControl_CTS     ((uint16_t)0x0200)
#define USART_HardwareFlowControl_RTS_CTS ((uint16_t)0x0300)

// 中断标志
#define USART_IT_PE    ((uint16_t)0x0028)
#define USART_IT_TXE   ((uint16_t)0x0727)
#define USART_IT_TC    ((uint16_t)0x0626)
#define USART_IT_RXNE  ((uint16_t)0x0525)
#define USART_IT_IDLE  ((uint16_t)0x0424)

// 状态标志
#define USART_FLAG_CTS  ((uint16_t)0x0200)
#define USART_FLAG_TXE  ((uint16_t)0x0080)
#define USART_FLAG_TC   ((uint16_t)0x0040)
#define USART_FLAG_RXNE ((uint16_t)0x0020)
#define USART_FLAG_IDLE ((uint16_t)0x0010)
#define USART_FLAG_ORE  ((uint16_t)0x0008)
```

## 4. USART端口

```c
USART_TypeDef *USART1;  // 基地址 0x40013800, PA9(TX)/PA10(RX)
USART_TypeDef *USART2;  // 基地址 0x40004400
USART_TypeDef *USART3;  // 基地址 0x40004800
```

## 5. 函数原型

```c
void USART_DeInit(USART_TypeDef* USARTx);
void USART_Init(USART_TypeDef* USARTx, USART_InitTypeDef* USART_InitStruct);
void USART_StructInit(USART_InitTypeDef* USART_InitStruct);
void USART_Cmd(USART_TypeDef* USARTx, FunctionalState NewState);
void USART_ITConfig(USART_TypeDef* USARTx, uint16_t USART_IT, FunctionalState NewState);
void USART_DMACmd(USART_TypeDef* USARTx, uint16_t USART_DMAReq, FunctionalState NewState);
void USART_SendData(USART_TypeDef* USARTx, uint16_t Data);
uint16_t USART_ReceiveData(USART_TypeDef* USARTx);
FlagStatus USART_GetFlagStatus(USART_TypeDef* USARTx, uint16_t USART_FLAG);
void USART_ClearFlag(USART_TypeDef* USARTx, uint16_t USART_FLAG);
ITStatus USART_GetITStatus(USART_TypeDef* USARTx, uint16_t USART_IT);
void USART_ClearITPendingBit(USART_TypeDef* USARTx, uint16_t USART_IT);
```

## 6. 典型用法

### 基本初始化 (USART1, PA9/PA10)

```c
void USART1_Init(void) {
    USART_InitTypeDef USART_InitStructure = {0};
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // PA9 TX (复用推挽)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA10 RX (浮空输入)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}
```

### 中断接收

```c
void USART1_Init_IT(void) {
    USART1_Init();                           // 先调用基本初始化
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 2);
}

void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART1);
        // 处理接收到的数据
        USART_SendData(USART1, data);         // 回显
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
```

### 轮询发送

```c
void USART_SendByte(USART_TypeDef* USARTx, uint8_t data) {
    USART_SendData(USARTx, data);
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
}

void USART_SendString(USART_TypeDef* USARTx, const char* str) {
    while (*str) {
        USART_SendByte(USARTx, *str++);
    }
}
```

---

*对应文件路径: Peripheral/inc/ch32v30x_usart.h*

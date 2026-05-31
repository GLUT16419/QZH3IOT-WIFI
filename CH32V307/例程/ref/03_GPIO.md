# GPIO 模块

包含：完整的GPIO数据结构、引脚宏、函数原型、典型用法。

---

## 1. 头文件

```c
#include "ch32v30x_gpio.h"   // 文件: Peripheral/inc/ch32v30x_gpio.h
```

## 2. 数据结构

```c
typedef enum {
    GPIO_Speed_10MHz = 1,
    GPIO_Speed_2MHz,
    GPIO_Speed_50MHz
} GPIOSpeed_TypeDef;

typedef enum {
    GPIO_Mode_AIN         = 0x0,   // 模拟输入
    GPIO_Mode_IN_FLOATING = 0x04,  // 浮空输入
    GPIO_Mode_IPD         = 0x28,  // 下拉输入
    GPIO_Mode_IPU         = 0x48,  // 上拉输入
    GPIO_Mode_Out_OD      = 0x14,  // 开漏输出
    GPIO_Mode_Out_PP      = 0x10,  // 推挽输出
    GPIO_Mode_AF_OD       = 0x1C,  // 复用开漏
    GPIO_Mode_AF_PP       = 0x18,  // 复用推挽
} GPIOMode_TypeDef;

typedef struct {
    uint16_t GPIO_Pin;              // @GPIO_pins_define
    GPIOSpeed_TypeDef GPIO_Speed;   // 输出速率
    GPIOMode_TypeDef GPIO_Mode;     // 模式
} GPIO_InitTypeDef;

typedef enum { Bit_RESET = 0, Bit_SET } BitAction;
```

## 3. GPIO引脚宏

```c
#define GPIO_Pin_0    ((uint16_t)0x0001)
#define GPIO_Pin_1    ((uint16_t)0x0002)
#define GPIO_Pin_2    ((uint16_t)0x0004)
#define GPIO_Pin_3    ((uint16_t)0x0008)
#define GPIO_Pin_4    ((uint16_t)0x0010)
#define GPIO_Pin_5    ((uint16_t)0x0020)
#define GPIO_Pin_6    ((uint16_t)0x0040)
#define GPIO_Pin_7    ((uint16_t)0x0080)
#define GPIO_Pin_8    ((uint16_t)0x0100)
#define GPIO_Pin_9    ((uint16_t)0x0200)
#define GPIO_Pin_10   ((uint16_t)0x0400)
#define GPIO_Pin_11   ((uint16_t)0x0800)
#define GPIO_Pin_12   ((uint16_t)0x1000)
#define GPIO_Pin_13   ((uint16_t)0x2000)
#define GPIO_Pin_14   ((uint16_t)0x4000)
#define GPIO_Pin_15   ((uint16_t)0x8000)
#define GPIO_Pin_All  ((uint16_t)0xFFFF)
```

## 4. GPIO端口

```c
GPIO_TypeDef *GPIOA;  // 基地址 0x40010800
GPIO_TypeDef *GPIOB;  // 基地址 0x40010C00
GPIO_TypeDef *GPIOC;  // 基地址 0x40011000
GPIO_TypeDef *GPIOD;  // 基地址 0x40011400
GPIO_TypeDef *GPIOE;  // 基地址 0x40011800
```

## 5. 函数原型

```c
void GPIO_DeInit(GPIO_TypeDef* GPIOx);
void GPIO_AFIODeInit(void);
void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct);
void GPIO_StructInit(GPIO_InitTypeDef* GPIO_InitStruct);
uint8_t  GPIO_ReadInputDataBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
uint16_t GPIO_ReadInputData(GPIO_TypeDef* GPIOx);
uint8_t  GPIO_ReadOutputDataBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
uint16_t GPIO_ReadOutputData(GPIO_TypeDef* GPIOx);
void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void GPIO_WriteBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, BitAction BitVal);
void GPIO_Write(GPIO_TypeDef* GPIOx, uint16_t PortVal);
void GPIO_PinLockConfig(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void GPIO_EventOutputConfig(uint8_t GPIO_PortSource, uint8_t GPIO_PinSource);
void GPIO_EventOutputCmd(FunctionalState NewState);
void GPIO_PinRemapConfig(uint32_t GPIO_Remap, FunctionalState NewState);
void GPIO_EXTILineConfig(uint8_t GPIO_PortSource, uint8_t GPIO_PinSource);
void GPIO_ETH_MediaInterfaceConfig(uint32_t GPIO_ETH_MediaInterface);
```

## 6. 重映射宏

```c
#define GPIO_Remap_SPI1            ((uint32_t)0x00000001)
#define GPIO_Remap_I2C1            ((uint32_t)0x00000002)
#define GPIO_Remap_USART1          ((uint32_t)0x00000004)
#define GPIO_Remap_USART2          ((uint32_t)0x00000008)
#define GPIO_PartialRemap_USART3   ((uint32_t)0x00140010)
#define GPIO_FullRemap_USART3      ((uint32_t)0x00140030)
#define GPIO_PartialRemap_TIM1     ((uint32_t)0x00160040)
#define GPIO_FullRemap_TIM1        ((uint32_t)0x001600C0)
#define GPIO_PartialRemap1_TIM2    ((uint32_t)0x00180100)
#define GPIO_FullRemap_TIM2        ((uint32_t)0x00180300)
#define GPIO_Remap_ETH             ((uint32_t)0x00200020)
#define GPIO_Remap_MII_RMII_SEL    ((uint32_t)0x00200080)
```

## 7. 典型用法

### 推挽输出

```c
void GPIO_Output_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA, GPIO_Pin_0);     // PA0 输出高
    GPIO_ResetBits(GPIOA, GPIO_Pin_1);   // PA1 输出低
}
```

### 上拉输入

```c
void GPIO_Input_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

uint8_t ReadKey(void) {
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);  // 0=按下, 1=未按
}
```

### GPIO翻转 (LED闪烁)

```c
void GPIO_Toggle_INIT(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void Loop(void) {
    while(1) {
        GPIO_WriteBit(GPIOA, GPIO_Pin_0,
            (i == 0) ? (i = Bit_SET) : (i = Bit_RESET));
        Delay_Ms(250);
    }
}
```

---

*对应文件路径: Peripheral/inc/ch32v30x_gpio.h*

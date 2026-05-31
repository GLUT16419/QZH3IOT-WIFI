# EXTI 外部中断模块

包含：数据结构、触发方式宏、函数原型、典型用法。

---

## 1. 头文件

```c
#include "ch32v30x_exti.h"   // 文件: Peripheral/inc/ch32v30x_exti.h
```

## 2. 数据结构

```c
typedef enum {
    EXTI_Mode_Interrupt = 0x00,
    EXTI_Mode_Event = 0x04
} EXTIMode_TypeDef;

typedef enum {
    EXTI_Trigger_Rising  = 0x08,
    EXTI_Trigger_Falling = 0x0C,
    EXTI_Trigger_Rising_Falling = 0x10
} EXTITrigger_TypeDef;

typedef struct {
    uint32_t EXTI_Line;               // @EXTI_Lines
    EXTIMode_TypeDef EXTI_Mode;       // 中断或事件
    EXTITrigger_TypeDef EXTI_Trigger; // 触发边沿
    FunctionalState EXTI_LineCmd;     // ENABLE/DISABLE
} EXTI_InitTypeDef;
```

## 3. EXTI 线宏

```c
#define EXTI_Line0   ((uint32_t)0x00001)
#define EXTI_Line1   ((uint32_t)0x00002)
// ... 到 EXTI_Line15 ((uint32_t)0x08000)
#define EXTI_Line16  ((uint32_t)0x10000)  // PVD输出
#define EXTI_Line17  ((uint32_t)0x20000)  // RTC闹钟
#define EXTI_Line18  ((uint32_t)0x40000)  // USB唤醒
#define EXTI_Line19  ((uint32_t)0x80000)  // 以太网唤醒
#define EXTI_Line20  ((uint32_t)0x100000) // USBHS唤醒
```

## 4. GPIO端口源和引脚源 (用于GPIO_EXTILineConfig)

```c
// 端口源
#define GPIO_PortSourceGPIOA  ((uint8_t)0x00)
#define GPIO_PortSourceGPIOB  ((uint8_t)0x01)
#define GPIO_PortSourceGPIOC  ((uint8_t)0x02)
#define GPIO_PortSourceGPIOD  ((uint8_t)0x03)
#define GPIO_PortSourceGPIOE  ((uint8_t)0x04)

// 引脚源
#define GPIO_PinSource0   ((uint8_t)0x00)
#define GPIO_PinSource1   ((uint8_t)0x01)
// ... 到 GPIO_PinSource15 ((uint8_t)0x0F)
```

## 5. 函数原型

```c
void EXTI_DeInit(void);
void EXTI_Init(EXTI_InitTypeDef* EXTI_InitStruct);
void EXTI_StructInit(EXTI_InitTypeDef* EXTI_InitStruct);
void EXTI_GenerateSWInterrupt(uint32_t EXTI_Line);
FlagStatus EXTI_GetFlagStatus(uint32_t EXTI_Line);
void EXTI_ClearFlag(uint32_t EXTI_Line);
ITStatus EXTI_GetITStatus(uint32_t EXTI_Line);
void EXTI_ClearITPendingBit(uint32_t EXTI_Line);
```

## 6. 典型用法 - PA0 下降沿中断

```c
void EXTI0_Init(void) {
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;      // 上拉输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);

    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_SetPriority(EXTI0_IRQn, 2);
}

void EXTI0_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        // 处理外部中断
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}
```

---

*对应文件路径: Peripheral/inc/ch32v30x_exti.h*

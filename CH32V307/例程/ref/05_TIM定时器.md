# TIM 定时器模块

包含：定时器时基配置、PWM输出、输入捕获、完整宏定义和函数原型。

---

## 1. 头文件

```c
#include "ch32v30x_tim.h"   // 文件: Peripheral/inc/ch32v30x_tim.h
```

## 2. 数据结构

```c
// 时基配置
typedef struct {
    uint16_t TIM_Prescaler;         // 预分频器 (0-65535)
    uint16_t TIM_CounterMode;       // 计数模式
    uint16_t TIM_Period;            // 自动重装值 (0-65535)
    uint16_t TIM_ClockDivision;     // 时钟分频
    uint8_t  TIM_RepetitionCounter; // 重复计数器 (TIM1/8)
} TIM_TimeBaseInitTypeDef;

// 输出比较 (PWM)
typedef struct {
    uint16_t TIM_OCMode;       // @TIM_Output_Compare_and_PWM_modes
    uint16_t TIM_OutputState;  // 输出使能
    uint16_t TIM_OutputNState; // 互补输出使能
    uint16_t TIM_Pulse;        // 脉冲宽度 (比较值)
    uint16_t TIM_OCPolarity;   // 输出极性
    uint16_t TIM_OCNPolarity;  // 互补输出极性
    uint16_t TIM_OCIdleState;  // 空闲状态
    uint16_t TIM_OCNIdleState;
} TIM_OCInitTypeDef;

// 输入捕获
typedef struct {
    uint16_t TIM_Channel;      // 通道
    uint16_t TIM_ICPolarity;   // 捕获极性
    uint16_t TIM_ICSelection;  // 输入选择
    uint16_t TIM_ICPrescaler;  // 捕获预分频
    uint16_t TIM_ICFilter;     // 滤波器 (0-15)
} TIM_ICInitTypeDef;
```

## 3. 关键宏

```c
// 计数模式
#define TIM_CounterMode_Up             ((uint16_t)0x0000)
#define TIM_CounterMode_Down           ((uint16_t)0x0010)
#define TIM_CounterMode_CenterAligned1 ((uint16_t)0x0020)
#define TIM_CounterMode_CenterAligned2 ((uint16_t)0x0040)
#define TIM_CounterMode_CenterAligned3 ((uint16_t)0x0060)

// PWM模式
#define TIM_OCMode_Timing  ((uint16_t)0x0000)  // 定时
#define TIM_OCMode_Active  ((uint16_t)0x0010)  // 匹配时置有效
#define TIM_OCMode_Inactive ((uint16_t)0x0020) // 匹配时置无效
#define TIM_OCMode_PWM1    ((uint16_t)0x0060)  // PWM1
#define TIM_OCMode_PWM2    ((uint16_t)0x0070)  // PWM2

// 通道
#define TIM_Channel_1  ((uint16_t)0x0000)
#define TIM_Channel_2  ((uint16_t)0x0004)
#define TIM_Channel_3  ((uint16_t)0x0008)
#define TIM_Channel_4  ((uint16_t)0x000C)

// 输出极性
#define TIM_OCPolarity_High ((uint16_t)0x0000)
#define TIM_OCPolarity_Low  ((uint16_t)0x0002)

// 输出状态
#define TIM_OutputState_Disable ((uint16_t)0x0000)
#define TIM_OutputState_Enable  ((uint16_t)0x0001)

// 中断类型
#define TIM_IT_Update  ((uint16_t)0x0001)
#define TIM_IT_CC1     ((uint16_t)0x0002)
#define TIM_IT_CC2     ((uint16_t)0x0004)
#define TIM_IT_CC3     ((uint16_t)0x0008)
#define TIM_IT_CC4     ((uint16_t)0x0010)

// 捕获极性
#define TIM_ICPolarity_Rising   ((uint16_t)0x0000)
#define TIM_ICPolarity_Falling  ((uint16_t)0x0002)
#define TIM_ICPolarity_BothEdge ((uint16_t)0x000A)

// 输入选择
#define TIM_ICSelection_DirectTI ((uint16_t)0x0001)
#define TIM_ICSelection_IndirectTI ((uint16_t)0x0002)
#define TIM_ICSelection_TRC ((uint16_t)0x0003)

// 捕获预分频
#define TIM_ICPSC_DIV1  ((uint16_t)0x0000)
#define TIM_ICPSC_DIV2  ((uint16_t)0x0004)
#define TIM_ICPSC_DIV4  ((uint16_t)0x0008)
#define TIM_ICPSC_DIV8  ((uint16_t)0x000C)

// 时钟分频
#define TIM_CKD_DIV1  ((uint16_t)0x0000)
#define TIM_CKD_DIV2  ((uint16_t)0x0100)
#define TIM_CKD_DIV4  ((uint16_t)0x0200)
```

## 4. 函数原型

```c
void TIM_DeInit(TIM_TypeDef* TIMx);
void TIM_TimeBaseInit(TIM_TypeDef* TIMx, TIM_TimeBaseInitTypeDef* TIM_TimeBaseInitStruct);
void TIM_OC1Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct);
void TIM_OC2Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct);
void TIM_OC3Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct);
void TIM_OC4Init(TIM_TypeDef* TIMx, TIM_OCInitTypeDef* TIM_OCInitStruct);
void TIM_ICInit(TIM_TypeDef* TIMx, TIM_ICInitTypeDef* TIM_ICInitStruct);
void TIM_BDTRConfig(TIM_TypeDef* TIMx, TIM_BDTRInitTypeDef *TIM_BDTRInitStruct);
void TIM_Cmd(TIM_TypeDef* TIMx, FunctionalState NewState);
void TIM_ITConfig(TIM_TypeDef* TIMx, uint16_t TIM_IT, FunctionalState NewState);
void TIM_CtrlPWMOutputs(TIM_TypeDef* TIMx, FunctionalState NewState);

void TIM_SetCompare1(TIM_TypeDef* TIMx, uint16_t Compare1);
void TIM_SetCompare2(TIM_TypeDef* TIMx, uint16_t Compare2);
uint32_t TIM_GetCapture1(TIM_TypeDef* TIMx);
uint32_t TIM_GetCapture2(TIM_TypeDef* TIMx);

FlagStatus TIM_GetFlagStatus(TIM_TypeDef* TIMx, uint16_t TIM_FLAG);
void TIM_ClearFlag(TIM_TypeDef* TIMx, uint16_t TIM_FLAG);
ITStatus TIM_GetITStatus(TIM_TypeDef* TIMx, uint16_t TIM_IT);
void TIM_ClearITPendingBit(TIM_TypeDef* TIMx, uint16_t TIM_IT);
```

## 5. 定时器分组

| 定时器 | 总线 | 特性 |
|--------|------|------|
| TIM1 | APB2 | 高级定时器 (16位, 带互补输出/刹车/死区) |
| TIM2-5 | APB1 | 通用定时器 (16位) |
| TIM6-7 | APB1 | 基本定时器 (16位, 无外部引脚) |
| TIM8 | APB2 | 高级定时器 |

## 6. 典型用法

### 定时器中断 (1ms)

```c
void TIM2_Init_1ms(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock / 1000000 - 1;  // 1us
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;   // 1000us = 1ms
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        // 每1ms执行一次
    }
}
```

### PWM输出 (TIM1_CH1, PA8, 1KHz, 50%占空比)

```c
void TIM1_PWM_Init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA, ENABLE);

    // PA8 TIM1_CH1 复用推挽
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    TIM_TimeBaseStructure.TIM_Prescaler = 144 - 1;       // 144MHz/144 = 1MHz
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;         // 1MHz/1000 = 1KHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 500;                 // 占空比 50%
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);                    // 主输出使能
    TIM_Cmd(TIM1, ENABLE);
}
```

---

*对应文件路径: Peripheral/inc/ch32v30x_tim.h*

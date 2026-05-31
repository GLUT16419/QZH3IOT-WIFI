# RCC 时钟控制模块

包含：时钟源配置、PLL倍频、分频系数、外设时钟使能、代码模板。

---

## 1. 头文件

```c
#include "ch32v30x_rcc.h"   // 文件: Peripheral/inc/ch32v30x_rcc.h
```

## 2. 数据结构

```c
typedef struct {
    uint32_t SYSCLK_Frequency;  // SYSCLK 频率(Hz)
    uint32_t HCLK_Frequency;    // HCLK 频率(Hz)
    uint32_t PCLK1_Frequency;   // PCLK1 (APB1, 定时器2-4/USART2-3等)
    uint32_t PCLK2_Frequency;   // PCLK2 (APB2, USART1/SPI1/TIM1等)
    uint32_t ADCCLK_Frequency;  // ADCCLK (ADC时钟)
} RCC_ClocksTypeDef;
```

## 3. 关键宏 (CH32V30x_D8C)

### HSE配置

```c
#define RCC_HSE_OFF         ((uint32_t)0x00000000)
#define RCC_HSE_ON          ((uint32_t)0x00010000)
#define RCC_HSE_Bypass      ((uint32_t)0x00040000)
```

### PLL时钟源

```c
#define RCC_PLLSource_HSI_Div2   ((uint32_t)0x00000000)   // HSI/2
#define RCC_PLLSource_PREDIV1    ((uint32_t)0x00010000)   // PREDIV1 输出
```

### PLL倍频 (D8C版本)

```c
#define RCC_PLLMul_18_EXTEN  ((uint32_t)0x00000000)  // 18倍
#define RCC_PLLMul_3_EXTEN   ((uint32_t)0x00040000)
#define RCC_PLLMul_4_EXTEN   ((uint32_t)0x00080000)
#define RCC_PLLMul_5_EXTEN   ((uint32_t)0x000C0000)
#define RCC_PLLMul_6_EXTEN   ((uint32_t)0x00100000)
#define RCC_PLLMul_7_EXTEN   ((uint32_t)0x00140000)
#define RCC_PLLMul_8_EXTEN   ((uint32_t)0x00180000)
#define RCC_PLLMul_9_EXTEN   ((uint32_t)0x001C0000)
#define RCC_PLLMul_10_EXTEN  ((uint32_t)0x00200000)
#define RCC_PLLMul_11_EXTEN  ((uint32_t)0x00240000)
#define RCC_PLLMul_12_EXTEN  ((uint32_t)0x00280000)
#define RCC_PLLMul_13_EXTEN  ((uint32_t)0x002C0000)
#define RCC_PLLMul_14_EXTEN  ((uint32_t)0x00300000)
#define RCC_PLLMul_6_5_EXTEN ((uint32_t)0x00340000)
#define RCC_PLLMul_15_EXTEN  ((uint32_t)0x00380000)
#define RCC_PLLMul_16_EXTEN  ((uint32_t)0x003C0000)
```

### PREDIV1 (D8C)

```c
#define RCC_PREDIV1_Div1  ((uint32_t)0x00000000)  // 到 Div16
#define RCC_PREDIV1_Div16 ((uint32_t)0x0000000F)
#define RCC_PREDIV1_Source_HSE  ((uint32_t)0x00000000)
#define RCC_PREDIV1_Source_PLL2 ((uint32_t)0x00010000)
```

### 系统时钟源

```c
#define RCC_SYSCLKSource_HSI    ((uint32_t)0x00000000)
#define RCC_SYSCLKSource_HSE    ((uint32_t)0x00000001)
#define RCC_SYSCLKSource_PLLCLK ((uint32_t)0x00000002)
```

### AHB/APB 分频

```c
#define RCC_SYSCLK_Div1    ((uint32_t)0x00000000)
#define RCC_SYSCLK_Div2    ((uint32_t)0x00000080)
#define RCC_SYSCLK_Div4    ((uint32_t)0x00000090)
#define RCC_SYSCLK_Div8    ((uint32_t)0x000000A0)
#define RCC_SYSCLK_Div16   ((uint32_t)0x000000B0)
#define RCC_SYSCLK_Div64   ((uint32_t)0x000000C0)
#define RCC_SYSCLK_Div128  ((uint32_t)0x000000D0)
#define RCC_SYSCLK_Div256  ((uint32_t)0x000000E0)
#define RCC_SYSCLK_Div512  ((uint32_t)0x000000F0)

#define RCC_HCLK_Div1  ((uint32_t)0x00000000)
#define RCC_HCLK_Div2  ((uint32_t)0x00000400)
#define RCC_HCLK_Div4  ((uint32_t)0x00000500)
#define RCC_HCLK_Div8  ((uint32_t)0x00000600)
#define RCC_HCLK_Div16 ((uint32_t)0x00000700)
```

## 4. 外设时钟使能

```c
// APB2 外设 (挂载高速外设: GPIO, USART1, SPI1, TIM1, ADC等)
void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, FunctionalState NewState);

#define RCC_APB2Periph_GPIOA    ((uint32_t)0x00000004)
#define RCC_APB2Periph_GPIOB    ((uint32_t)0x00000008)
#define RCC_APB2Periph_GPIOC    ((uint32_t)0x00000010)
#define RCC_APB2Periph_GPIOD    ((uint32_t)0x00000020)
#define RCC_APB2Periph_GPIOE    ((uint32_t)0x00000040)
#define RCC_APB2Periph_USART1   ((uint32_t)0x00004000)
#define RCC_APB2Periph_ADC1     ((uint32_t)0x00000200)
#define RCC_APB2Periph_ADC2     ((uint32_t)0x00000400)
#define RCC_APB2Periph_TIM1     ((uint32_t)0x00000800)
#define RCC_APB2Periph_SPI1     ((uint32_t)0x00001000)
#define RCC_APB2Periph_AFIO     ((uint32_t)0x00000001)

// APB1 外设 (挂载低速外设: USART2/3, TIM2-7, I2C1/2, SPI2等)
void RCC_APB1PeriphClockCmd(uint32_t RCC_APB1Periph, FunctionalState NewState);

#define RCC_APB1Periph_TIM2     ((uint32_t)0x00000001)
#define RCC_APB1Periph_TIM3     ((uint32_t)0x00000002)
#define RCC_APB1Periph_TIM4     ((uint32_t)0x00000004)
#define RCC_APB1Periph_USART2   ((uint32_t)0x00020000)
#define RCC_APB1Periph_USART3   ((uint32_t)0x00040000)
#define RCC_APB1Periph_I2C1     ((uint32_t)0x00200000)
#define RCC_APB1Periph_I2C2     ((uint32_t)0x00400000)
#define RCC_APB1Periph_SPI2     ((uint32_t)0x00004000)
#define RCC_APB1Periph_CAN1     ((uint32_t)0x02000000)
#define RCC_APB1Periph_DAC      ((uint32_t)0x20000000)
#define RCC_APB1Periph_PWR      ((uint32_t)0x10000000)
#define RCC_APB1Periph_BKP      ((uint32_t)0x08000000)
```

## 5. 函数原型

```c
void RCC_DeInit(void);
void RCC_HSEConfig(uint32_t RCC_HSE);           // RCC_HSE_ON/OFF/Bypass
uint32_t RCC_WaitForHSEStartUp(void);           // 返回 SUCCESS 或 ERROR
void RCC_HSICmd(FunctionalState NewState);
void RCC_PLLCmd(FunctionalState NewState);
void RCC_PLLConfig(uint32_t RCC_PLLSource, uint32_t RCC_PLLMul);
void RCC_PREDIV1Config(uint32_t RCC_PREDIV1_Source, uint32_t RCC_PREDIV1_Div);
void RCC_SYSCLKConfig(uint32_t RCC_SYSCLKSource);
uint8_t RCC_GetSYSCLKSource(void);
void RCC_HCLKConfig(uint32_t RCC_SYSCLK);
void RCC_PCLK1Config(uint32_t RCC_HCLK);
void RCC_PCLK2Config(uint32_t RCC_HCLK);
void RCC_GetClocksFreq(RCC_ClocksTypeDef* RCC_Clocks);
void SystemCoreClockUpdate(void);               // 更新 SystemCoreClock 变量
FlagStatus RCC_GetFlagStatus(uint8_t RCC_FLAG);
```

## 6. 代码模板：HSE 8MHz -> PLL -> 144MHz

```c
void HSE_SetSysClk(uint32_t pllmul) {
    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    if (RCC_WaitForHSEStartUp() == SUCCESS) {
        RCC_HCLKConfig(RCC_SYSCLK_Div1);        // AHB = SYSCLK
        RCC_PCLK2Config(RCC_HCLK_Div1);         // APB2 = AHB
        RCC_PCLK1Config(RCC_HCLK_Div2);         // APB1 = AHB/2
        RCC_PLLConfig(RCC_PLLSource_PREDIV1, pllmul);
        RCC_PLLCmd(ENABLE);
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        while (RCC_GetSYSCLKSource() != 0x02);
    }
}
// 调用: HSE_SetSysClk(RCC_PLLMul_18_EXTEN);  // 8MHz * 18 = 144MHz
```

---

*对应文件路径: Peripheral/inc/ch32v30x_rcc.h*

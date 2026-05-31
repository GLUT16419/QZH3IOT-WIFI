# ADC 模数转换模块

包含：数据结构、采样时间/PGA/对齐宏、函数原型、典型用法。

---

## 1. 头文件

```c
#include "ch32v30x_adc.h"   // 文件: Peripheral/inc/ch32v30x_adc.h
```

## 2. 数据结构

```c
typedef struct {
    uint32_t ADC_Mode;               // 模式
    FunctionalState ADC_ScanConvMode;       // 扫描模式
    FunctionalState ADC_ContinuousConvMode; // 连续转换
    uint32_t ADC_ExternalTrigConv;   // 外部触发
    uint32_t ADC_DataAlign;          // 对齐
    uint8_t  ADC_NbrOfChannel;       // 转换通道数 (1-16)
    uint32_t ADC_OutputBuffer;       // 输出缓冲
    uint32_t ADC_Pga;                // PGA增益
} ADC_InitTypeDef;
```

## 3. 关键宏

```c
// ADC模式
#define ADC_Mode_Independent ((uint32_t)0x00000000)

// 外部触发 (软件触发)
#define ADC_ExternalTrigConv_None ((uint32_t)0x00000000)

// 数据对齐
#define ADC_DataAlign_Right ((uint32_t)0x00000000)
#define ADC_DataAlign_Left  ((uint32_t)0x00000800)

// 采样时间 (通道独立配置)
#define ADC_SampleTime_1Cycles5   ((uint8_t)0x00)
#define ADC_SampleTime_7Cycles5   ((uint8_t)0x01)
#define ADC_SampleTime_13Cycles5  ((uint8_t)0x02)
#define ADC_SampleTime_28Cycles5  ((uint8_t)0x03)
#define ADC_SampleTime_41Cycles5  ((uint8_t)0x04)
#define ADC_SampleTime_55Cycles5  ((uint8_t)0x05)
#define ADC_SampleTime_71Cycles5  ((uint8_t)0x06)
#define ADC_SampleTime_239Cycles5 ((uint8_t)0x07)

// 输出缓冲
#define ADC_OutputBuffer_Enable  ((uint32_t)0x04000000)
#define ADC_OutputBuffer_Disable ((uint32_t)0x00000000)

// PGA增益
#define ADC_Pga_1  ((uint32_t)0x00000000)
#define ADC_Pga_4  ((uint32_t)0x08000000)
#define ADC_Pga_16 ((uint32_t)0x10000000)
#define ADC_Pga_64 ((uint32_t)0x18000000)

// 标志
#define ADC_FLAG_EOC ((uint8_t)0x02)   // 转换结束
#define ADC_FLAG_AWD ((uint8_t)0x01)   // 模拟看门狗

// 通道
#define ADC_Channel_0   ((uint8_t)0x00)
#define ADC_Channel_1   ((uint8_t)0x01)
// ... 到 ADC_Channel_15 ((uint8_t)0x0F)
```

## 4. 函数原型

```c
void ADC_DeInit(void);
void ADC_Init(ADC_TypeDef* ADCx, ADC_InitTypeDef* ADC_InitStruct);
void ADC_StructInit(ADC_InitTypeDef* ADC_InitStruct);
void ADC_Cmd(ADC_TypeDef* ADCx, FunctionalState NewState);
void ADC_RegularChannelConfig(ADC_TypeDef* ADCx, uint8_t ADC_Channel, uint8_t Rank, uint8_t ADC_SampleTime);
void ADC_SoftwareStartConvCmd(ADC_TypeDef* ADCx, FunctionalState NewState);
FlagStatus ADC_GetFlagStatus(ADC_TypeDef* ADCx, uint8_t ADC_FLAG);
uint16_t ADC_GetConversionValue(ADC_TypeDef* ADCx);
void ADC_ResetCalibration(ADC_TypeDef* ADCx);
FlagStatus ADC_GetResetCalibrationStatus(ADC_TypeDef* ADCx);
void ADC_StartCalibration(ADC_TypeDef* ADCx);
FlagStatus ADC_GetCalibrationStatus(ADC_TypeDef* ADCx);
void ADC_ITConfig(ADC_TypeDef* ADCx, uint16_t ADC_IT, FunctionalState NewState);
```

## 5. 典型用法 - 单通道连续转换 (PA0)

```c
void ADC1_Init(void) {
    ADC_InitTypeDef ADC_InitStructure = {0};
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // PA0 模拟输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_InitStructure.ADC_OutputBuffer = ADC_OutputBuffer_Disable;
    ADC_InitStructure.ADC_Pga = ADC_Pga_1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_7Cycles5);
    ADC_Cmd(ADC1, ENABLE);

    // 校准
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1));

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

uint16_t ADC_Read(void) {
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);  // 12位分辨率, 0~4095
}
```

---

*对应文件路径: Peripheral/inc/ch32v30x_adc.h*

# DMA 模块

包含：完整数据结构、传输方向/地址增量/数据宽度宏、函数原型、典型用法。

---

## 1. 头文件

```c
#include "ch32v30x_dma.h"   // 文件: Peripheral/inc/ch32v30x_dma.h
```

## 2. 数据结构

```c
typedef struct {
    uint32_t DMA_PeripheralBaseAddr;   // 外设地址
    uint32_t DMA_MemoryBaseAddr;       // 内存地址
    uint32_t DMA_DIR;                  // 方向
    uint32_t DMA_BufferSize;           // 传输数据量
    uint32_t DMA_PeripheralInc;        // 外设地址增量
    uint32_t DMA_MemoryInc;            // 内存地址增量
    uint32_t DMA_PeripheralDataSize;   // 外设数据宽度
    uint32_t DMA_MemoryDataSize;       // 内存数据宽度
    uint32_t DMA_Mode;                 // 模式: Normal/Circular
    uint32_t DMA_Priority;             // 优先级
    uint32_t DMA_M2M;                  // 内存到内存
} DMA_InitTypeDef;
```

## 3. 关键宏

```c
// 传输方向
#define DMA_DIR_PeripheralDST ((uint32_t)0x00000010)  // 外设是目标 (读内存->写外设)
#define DMA_DIR_PeripheralSRC ((uint32_t)0x00000000)  // 外设是源 (读外设->写内存)

// 地址增量
#define DMA_PeripheralInc_Enable  ((uint32_t)0x00000040)
#define DMA_PeripheralInc_Disable ((uint32_t)0x00000000)
#define DMA_MemoryInc_Enable      ((uint32_t)0x00000080)
#define DMA_MemoryInc_Disable     ((uint32_t)0x00000000)

// 数据宽度
#define DMA_PeripheralDataSize_Byte     ((uint32_t)0x00000000)
#define DMA_PeripheralDataSize_HalfWord ((uint32_t)0x00000100)
#define DMA_PeripheralDataSize_Word     ((uint32_t)0x00000200)
#define DMA_MemoryDataSize_Byte         ((uint32_t)0x00000000)
#define DMA_MemoryDataSize_HalfWord     ((uint32_t)0x00000400)
#define DMA_MemoryDataSize_Word         ((uint32_t)0x00000800)

// 模式
#define DMA_Mode_Circular ((uint32_t)0x00000020)  // 循环模式
#define DMA_Mode_Normal   ((uint32_t)0x00000000)  // 单次模式

// 优先级
#define DMA_Priority_VeryHigh ((uint32_t)0x00003000)
#define DMA_Priority_High     ((uint32_t)0x00002000)
#define DMA_Priority_Medium   ((uint32_t)0x00001000)
#define DMA_Priority_Low      ((uint32_t)0x00000000)

// 内存到内存
#define DMA_M2M_Enable  ((uint32_t)0x00004000)
#define DMA_M2M_Disable ((uint32_t)0x00000000)
```

## 4. DMA通道

```c
DMA_Channel_TypeDef *DMA1_Channel1;  // DMA1通道1
DMA_Channel_TypeDef *DMA1_Channel2;
// ... 到 DMA2_Channel11
```

## 5. 函数原型

```c
void DMA_DeInit(DMA_Channel_TypeDef* DMAy_Channelx);
void DMA_Init(DMA_Channel_TypeDef* DMAy_Channelx, DMA_InitTypeDef* DMA_InitStruct);
void DMA_Cmd(DMA_Channel_TypeDef* DMAy_Channelx, FunctionalState NewState);
void DMA_ITConfig(DMA_Channel_TypeDef* DMAy_Channelx, uint32_t DMA_IT, FunctionalState NewState);
FlagStatus DMA_GetFlagStatus(uint32_t DMA_FLAG);
void DMA_ClearFlag(uint32_t DMA_FLAG);
ITStatus DMA_GetITStatus(uint32_t DMA_IT);
void DMA_ClearITPendingBit(uint32_t DMA_IT);
```

## 6. 典型用法

### 内存到内存传输

```c
void DMA_MemToMem_Example(void) {
    uint32_t src[100] = {1,2,3,4,5};
    uint32_t dst[100] = {0};
    DMA_InitTypeDef DMA_InitStructure = {0};

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)src;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)dst;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = 100;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    DMA_Cmd(DMA1_Channel1, ENABLE);
}
```

---

*对应文件路径: Peripheral/inc/ch32v30x_dma.h*

# CH32V307-FreeRTOS 参考手册（AI版）

> 本文档包含 CH32V307 上 FreeRTOS 移植的完整参考信息，包括精确的 API 函数原型、数据结构定义、FreeRTOSConfig.h 配置、RISC-V 移植层细节、中断安全调用规则等。AI 可据此直接编写 FreeRTOS 应用程序代码，无需查看实际源文件。

---

## 1. 项目概述

**芯片**: CH32V307VCT6 (RISC-V RV32IMAC 内核, 144MHz)  
**RTOS**: FreeRTOS V10.4.6 (移植)  
**内存管理**: heap_4.c (支持 free 和碎片合并)  
**Tick 频率**: 500Hz (2ms 一个 Tick)  
**任务调度**: 抢占式 (configUSE_PREEMPTION = 1)

---

## 2. 文件路径与头文件包含

### 2.1 目录结构简图（包含路径）

```
CH32V307-FreeRTOS/
├── Core/
│   ├── core_riscv.c
│   └── core_riscv.h            # RISC-V 核心功能
├── Debug/
│   ├── debug.c
│   └── debug.h                 # 延时 + printf 初始化
├── FreeRTOS/
│   ├── tasks.c                 # 任务管理
│   ├── queue.c                 # 队列/IPC
│   ├── timers.c                # 软件定时器
│   ├── event_groups.c          # 事件组
│   ├── list.c                  # 链表
│   ├── stream_buffer.c         # 流缓冲区
│   ├── croutine.c              # 协程
│   ├── include/                # FreeRTOS 头文件目录
│   │   ├── FreeRTOS.h          # 主头文件
│   │   ├── task.h              # 任务 API
│   │   ├── queue.h             # 队列/信号量 API
│   │   ├── semphr.h            # 信号量/互斥锁 API
│   │   ├── timers.h            # 定时器 API
│   │   ├── event_groups.h      # 事件组 API
│   │   ├── stream_buffer.h     # 流缓冲区 API
│   │   ├── message_buffer.h    # 消息缓冲区 API
│   │   ├── portable.h          # 移植抽象层
│   │   └── projdefs.h          # 项目定义
│   └── portable/
│       └── GCC/RISC-V/         # RISC-V 移植层
│           ├── port.c          # 上下文切换 C 实现
│           ├── portASM.S       # 上下文切换 汇编实现
│           ├── portmacro.h     # 平台相关宏/类型定义
│           └── chip_specific_extensions/
│               └── RV32I_PFIC_no_extensions/
│                   └── freertos_risc_v_chip_specific_extensions.h
├── Peripheral/                  # CH32V307 外设驱动
│   ├── inc/ch32v30x_*.h        # 外设头文件
│   └── src/ch32v30x_*.c        # 外设源文件
├── Startup/
│   ├── startup_ch32v30x_D8.S   # CH32V303 启动文件
│   └── startup_ch32v30x_D8C.S  # CH32V305/307 启动文件（默认）
├── User/
│   ├── main.c                  # 用户主程序
│   ├── FreeRTOSConfig.h        # FreeRTOS 配置
│   ├── ch32v30x_conf.h         # 外设配置（包含所有外设头文件）
│   ├── ch32v30x_it.c/h         # 中断处理
│   └── system_ch32v30x.c/h     # 系统时钟配置
└── Ld/
    └── Link.ld                 # 链接脚本
```

### 2.2 必须包含的头文件

```c
// 基础（必须）
#include "debug.h"                    // Debug/debug.h - Delay_Init, USART_Printf_Init
#include "FreeRTOS.h"                 // FreeRTOS.h - BaseType_t, TickType_t, 配置
#include "task.h"                     // task.h     - xTaskCreate, vTaskDelay 等
#include "ch32v30x_gpio.h"            // GPIO 操作
#include "ch32v30x_rcc.h"             // 时钟控制

// 根据需要使用
#include "queue.h"                    // 队列 API (xQueueCreate 等)
#include "semphr.h"                   // 信号量/互斥锁 API
#include "timers.h"                   // 软件定时器 API
#include "event_groups.h"             // 事件组 API
#include "stream_buffer.h"            // 流缓冲区 API
```

---

## 3. FreeRTOSConfig.h 完整配置

```c
// 文件: User/FreeRTOSConfig.h

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "ch32v30x.h"             // 芯片定义, 提供 SystemCoreClock

// ── RISC-V 特定配置 ──
#define configMTIME_BASE_ADDRESS      0
#define configMTIMECMP_BASE_ADDRESS   0
// !!! CH32V307 不使用标准 RISC-V 的 mtime/mtimecmp 寄存器。
// ！  定时器中断由 PFIC (类似 NVIC) 管理。这两个宏必须设为 0。

// ── 调度器 ──
#define configUSE_PREEMPTION          1   // 抢占式调度
#define configUSE_IDLE_HOOK           0   // 空闲钩子
#define configUSE_TICK_HOOK           0   // Tick 钩子
#define configUSE_TICKLESS_IDLE       0   // Tickless 模式
#define configTICK_SOURCE             0   // Tick 时钟源

// ── 系统时钟 ──
#define configCPU_CLOCK_HZ            SystemCoreClock  // 144000000UL (144MHz)
#define configTICK_RATE_HZ            ( ( TickType_t ) 500 )  // 500Hz = 2ms/Tick

// ── 任务与堆 ──
#define configMAX_PRIORITIES          ( 15 )           // 优先级 0-14 (0最低, 14最高)
#define configMINIMAL_STACK_SIZE      ( ( unsigned short ) 256 )  // 最小栈(字)
#define configTOTAL_HEAP_SIZE         ( ( size_t ) ( 12 * 1024 ) ) // 12KB
#define configMAX_TASK_NAME_LEN       ( 16 )
#define configUSE_16_BIT_TICKS        0   // 32位 Tick 计数器

// ── 内核功能 ──
#define configUSE_MUTEXES             1   // 互斥锁
#define configUSE_RECURSIVE_MUTEXES   1   // 递归互斥锁
#define configUSE_COUNTING_SEMAPHORES 1   // 计数信号量
#define configUSE_TIMERS              1   // 软件定时器
#define configUSE_CO_ROUTINES         0   // 协程
#define configUSE_QUEUE_SETS          0   // 队列集
#define configUSE_TASK_NOTIFICATIONS  1   // 任务通知

// ── 定时器任务 ──
#define configTIMER_TASK_PRIORITY     ( configMAX_PRIORITIES - 1 )  // 最高优先
#define configTIMER_QUEUE_LENGTH      10
#define configTIMER_TASK_STACK_DEPTH  ( configMINIMAL_STACK_SIZE * 2 )

// ── 空闲任务 ──
#define configIDLE_SHOULD_YIELD       1

// ── 栈溢出检测 ──
#define configCHECK_FOR_STACK_OVERFLOW         1   // 启用栈溢出检测
#define configQUEUE_REGISTRY_SIZE              10

// ── API 函数可用性 ──
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1   // 任务删除
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1   // 挂起/恢复
#define INCLUDE_vTaskDelayUntil                 1   // 绝对延时
#define INCLUDE_vTaskDelay                      1   // 相对延时
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   0
#define INCLUDE_xEventGroupSetBitFromISR        1
#define INCLUDE_xTimerPendFunctionCall          0
#define INCLUDE_xTaskAbortDelay                 0
#define INCLUDE_xTaskGetHandle                  0

// ── 断言 ──
#define configASSERT( x )   if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

// ── 中断优先级映射 ──
#define configPRIO_BITS                2       // 2位优先级
#define configKERNEL_INTERRUPT_PRIORITY 0xFF   // 内核中断优先级(最低)
#define configMAX_API_CALL_INTERRUPT_PRIORITY   0   // API调用最高中断优先级

#endif /* FREERTOS_CONFIG_H */
```

### 3.1 配置关键说明

| 配置项 | 值 | 说明 |
|--------|-----|------|
| Tick 频率 | 500Hz | 每个 Tick = 2ms，比标准的 1000Hz 低，减少上下文切换开销 |
| 最大优先级 | 15 | 优先级 0~14，14 为最高（建议定时器任务用 14） |
| 堆大小 | 12KB | 任务栈、队列等动态分配的总内存 |
| 最小栈 | 256 字 | 即 1024 字节（CH32V307 是 32位，1 字 = 4 字节） |
| 协议前缀 | v/x/pd | v=void 返回, x=BaseType_t 返回, pd=宏定义 |

---

## 4. 基础数据类型

```c
// 定义在 portable/GCC/RISC-V/portmacro.h

typedef signed char     int8_t;
typedef signed short    int16_t;
typedef signed long     int32_t;
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned long   uint32_t;

// FreeRTOS 核心类型
typedef uint32_t    StackType_t;        // 栈帧类型 (32位)
typedef int32_t     BaseType_t;         // 基础类型 (int)
typedef uint32_t    TickType_t;         // Tick 类型 (32位, 因 configUSE_16_BIT_TICKS=0)

// 返回值
#define pdTRUE      ((BaseType_t)1)
#define pdFALSE     ((BaseType_t)0)
#define pdPASS      (pdTRUE)
#define pdFAIL      (pdFALSE)
#define pdPASS      ((BaseType_t)1)
#define pdFAIL      ((BaseType_t)0)

// 延时
#define portMAX_DELAY           (TickType_t)0xFFFFFFFFUL   // 无限等待
#define portNO_DELAY            (TickType_t)0              // 不等待

// Tick 转换
#define pdMS_TO_TICKS(xTimeMs)  ((TickType_t)(((uint32_t)(xTimeMs)) / portTICK_PERIOD_MS))
#define portTICK_PERIOD_MS      ((TickType_t)2)            // 每个 Tick 2ms

// 中断控制
#define portDISABLE_INTERRUPTS()   __asm volatile("csrc mstatus, 8")
#define portENABLE_INTERRUPTS()    __asm volatile("csrs mstatus, 8")
#define portSET_INTERRUPT_MASK_FROM_ISR()    0
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)  (void)x
```

---

## 5. 任务管理 API（完整签名）

### 5.1 创建任务

```c
BaseType_t xTaskCreate(
    TaskFunction_t   pvTaskCode,      // 任务函数指针: void task(void *param)
    const char * const pcName,        // 任务名称 (字符串)
    const uint16_t   usStackDepth,    // 栈大小 (单位: 字/StackType_t, 非字节)
    void * const     pvParameters,    // 任务参数 (void*)
    UBaseType_t      uxPriority,      // 优先级 (0 ~ configMAX_PRIORITIES-1)
    TaskHandle_t * const pxCreatedTask  // [OUT] 任务句柄 (可传NULL)
);
// 返回值: pdPASS(成功) / errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY(失败)

// 更高效的版本 (栈由调用者分配)
BaseType_t xTaskCreateStatic(
    TaskFunction_t pvTaskCode,
    const char * const pcName,
    const uint32_t ulStackDepth,
    void * const pvParameters,
    UBaseType_t uxPriority,
    StackType_t * const puxStackBuffer,   // 预分配的栈缓冲区
    StaticTask_t * const pxTaskBuffer,    // 预分配的任务控制块
    TaskHandle_t * const pxCreatedTask
);
```

### 5.2 删除任务

```c
void vTaskDelete(TaskHandle_t xTask);
// xTask = NULL 表示删除自身
```

### 5.3 延时

```c
void vTaskDelay(const TickType_t xTicksToDelay);
// 相对延时: 延时 xTicksToDelay 个 Tick
// 例: vTaskDelay(500) 延时 500*2ms = 1000ms

void vTaskDelayUntil(TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement);
// 绝对延时: 保持固定周期
// 用法:
// TickType_t xLastWakeTime = xTaskGetTickCount();
// while(1) {
//     vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
//     // 每100ms执行一次
// }
```

### 5.4 挂起/恢复

```c
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
void vTaskResume(TaskHandle_t xTaskToResume);
BaseType_t xTaskResumeFromISR(TaskHandle_t xTaskToResume);
```

### 5.5 任务控制

```c
void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
UBaseType_t uxTaskPriorityGet(const TaskHandle_t xTask);
UBaseType_t uxTaskPriorityGetFromISR(const TaskHandle_t xTask);

TickType_t xTaskGetTickCount(void);
TickType_t xTaskGetTickCountFromISR(void);

UBaseType_t uxTaskGetNumberOfTasks(void);
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask);  // 栈剩余量(字)

void vTaskStartScheduler(void);             // 启动调度器
void vTaskEndScheduler(void);               // 停止调度器
BaseType_t xTaskGetSchedulerState(void);     // 调度器状态

TaskHandle_t xTaskGetCurrentTaskHandle(void);
```

---

## 6. 队列 API（完整签名）

### 6.1 创建队列

```c
QueueHandle_t xQueueCreate(
    UBaseType_t uxQueueLength,      // 队列长度 (元素个数)
    UBaseType_t uxItemSize          // 每个元素大小 (字节)
);
// 返回: NULL(失败) 或 QueueHandle_t

QueueHandle_t xQueueCreateStatic(
    UBaseType_t uxQueueLength,
    UBaseType_t uxItemSize,
    uint8_t *pucQueueStorageBuffer,
    StaticQueue_t *pxQueueBuffer
);
```

### 6.2 发送数据

```c
// 队列尾部发送
BaseType_t xQueueSend(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,        // 指向要发送数据的指针
    TickType_t xTicksToWait           // 等待时间 (portMAX_DELAY 无限等)
);

// 队列头部发送
BaseType_t xQueueSendToFront(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    TickType_t xTicksToWait
);

// 队列尾部发送 (等价于 xQueueSend)
BaseType_t xQueueSendToBack(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    TickType_t xTicksToWait
);

// 中断中发送 (不能在中断中阻塞，所以无超时参数)
BaseType_t xQueueSendFromISR(
    QueueHandle_t xQueue,
    const void *pvItemToQueue,
    BaseType_t *pxHigherPriorityTaskWoken
);
```

### 6.3 接收数据

```c
BaseType_t xQueueReceive(
    QueueHandle_t xQueue,
    void *pvBuffer,                   // 接收缓冲区指针
    TickType_t xTicksToWait           // 等待时间
);

BaseType_t xQueueReceiveFromISR(
    QueueHandle_t xQueue,
    void *pvBuffer,
    BaseType_t *pxHigherPriorityTaskWoken
);

BaseType_t xQueuePeek(
    QueueHandle_t xQueue,
    void *pvBuffer,
    TickType_t xTicksToWait
);
```

### 6.4 队列控制

```c
UBaseType_t uxQueueMessagesWaiting(const QueueHandle_t xQueue);
UBaseType_t uxQueueMessagesWaitingFromISR(const QueueHandle_t xQueue);
void vQueueDelete(QueueHandle_t xQueue);
```

### 6.5 队列典型用法

```c
// 创建
QueueHandle_t xQueue = xQueueCreate(10, sizeof(uint32_t));
configASSERT(xQueue);

// 发送任务
void vSenderTask(void *pvParameters) {
    uint32_t data = 0;
    while(1) {
        xQueueSend(xQueue, &data, portMAX_DELAY);
        data++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 接收任务
void vReceiverTask(void *pvParameters) {
    uint32_t recv;
    while(1) {
        if (xQueueReceive(xQueue, &recv, portMAX_DELAY) == pdPASS) {
            printf("Recv: %lu\r\n", recv);
        }
    }
}
```

---

## 7. 信号量与互斥锁 API

> 使用前需包含 `#include "semphr.h"`  
> `semphr.h` 本身不定义类型，通过宏定义映射到队列 API

### 7.1 创建

```c
// ── 二进制信号量 ──
SemaphoreHandle_t xSemaphoreCreateBinary(void);

// ── 计数信号量 (需要 configUSE_COUNTING_SEMAPHORES=1) ──
SemaphoreHandle_t xSemaphoreCreateCounting(
    UBaseType_t uxMaxCount,      // 最大计数
    UBaseType_t uxInitialCount   // 初始计数
);

// ── 互斥锁 (需要 configUSE_MUTEXES=1) ──
SemaphoreHandle_t xSemaphoreCreateMutex(void);

// ── 递归互斥锁 (需要 configUSE_RECURSIVE_MUTEXES=1) ──
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void);

// ── 静态版本 ──
SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *pxSemaphoreBuffer);
SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *pxMutexBuffer);
```

### 7.2 获取/释放

```c
// ── 获取 (Pend/Wait) ──
BaseType_t xSemaphoreTake(
    SemaphoreHandle_t xSemaphore,
    TickType_t xTicksToWait
);

// ── 释放 (Post/Signal) ──
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);

// ── 中断中释放 ──
BaseType_t xSemaphoreGiveFromISR(
    SemaphoreHandle_t xSemaphore,
    BaseType_t *pxHigherPriorityTaskWoken
);

// ── 递归互斥锁 ──
BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t xMutex);
BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t xMutex);
```

### 7.3 互斥锁典型用法

```c
SemaphoreHandle_t xMutex;

void vTaskWithResource(void *pvParameters) {
    while(1) {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        // -- 临界区 --
        shared_resource++;
        // -- 临界区结束 --
        xSemaphoreGive(xMutex);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

---

## 8. 软件定时器 API

> 需要 configUSE_TIMERS = 1

### 8.1 创建

```c
TimerHandle_t xTimerCreate(
    const char * const pcTimerName,       // 定时器名称
    const TickType_t xTimerPeriodInTicks, // 周期 (Tick 单位)
    const UBaseType_t uxAutoReload,       // pdTRUE=自动重载, pdFALSE=单次
    void * const pvTimerID,              // 定时器 ID
    TimerCallbackFunction_t pxCallbackFunction  // 回调函数: void vCallback(TimerHandle_t xTimer)
);
```

### 8.2 控制

```c
BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xBlockTime);
BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xBlockTime);
BaseType_t xTimerReset(TimerHandle_t xTimer, TickType_t xBlockTime);
BaseType_t xTimerChangePeriod(
    TimerHandle_t xTimer,
    TickType_t xNewPeriod,
    TickType_t xBlockTime
);
BaseType_t xTimerDelete(TimerHandle_t xTimer, TickType_t xBlockTime);

// 中断版本
BaseType_t xTimerStartFromISR(TimerHandle_t xTimer, BaseType_t *pxHigherPriorityTaskWoken);
BaseType_t xTimerStopFromISR(TimerHandle_t xTimer, BaseType_t *pxHigherPriorityTaskWoken);
BaseType_t xTimerResetFromISR(TimerHandle_t xTimer, BaseType_t *pxHigherPriorityTaskWoken);
```

### 8.3 创建后立即启动（静态创建宏）

```c
TimerHandle_t xTimer = xTimerCreate(
    "Timer", pdMS_TO_TICKS(1000), pdTRUE, NULL, vTimerCallback
);
xTimerStart(xTimer, 0);  // 启动
```

---

## 9. 事件组 API

> 需要 configUSE_EVENT_GROUPS = 1 (本项目已启用)

### 9.1 创建

```c
EventGroupHandle_t xEventGroupCreate(void);
EventGroupHandle_t xEventGroupCreateStatic(StaticEventGroup_t *pxEventGroupBuffer);
```

### 9.2 设置/等待

```c
// 设置位
EventBits_t xEventGroupSetBits(
    EventGroupHandle_t xEventGroup,
    const EventBits_t uxBitsToSet
);
BaseType_t xEventGroupSetBitsFromISR(
    EventGroupHandle_t xEventGroup,
    const EventBits_t uxBitsToSet,
    BaseType_t *pxHigherPriorityTaskWoken
);

// 清除位
EventBits_t xEventGroupClearBits(
    EventGroupHandle_t xEventGroup,
    const EventBits_t uxBitsToClear
);
EventBits_t xEventGroupClearBitsFromISR(
    EventGroupHandle_t xEventGroup,
    const EventBits_t uxBitsToClear
);

// 等待位
EventBits_t xEventGroupWaitBits(
    EventGroupHandle_t xEventGroup,
    const EventBits_t uxBitsToWaitFor,   // 要等的位
    const BaseType_t xClearOnExit,       // pdTRUE: 满足后自动清除
    const BaseType_t xWaitForAllBits,    // pdTRUE: 所有位, pdFALSE: 任一
    TickType_t xTicksToWait              // 超时
);

// 同步（同时设置和等待）
EventBits_t xEventGroupSync(
    EventGroupHandle_t xEventGroup,
    const EventBits_t uxBitsToSet,
    const EventBits_t uxBitsToWaitFor,
    TickType_t xTicksToWait
);
```

### 9.3 事件位定义

```c
#define BIT_0  (1 << 0)
#define BIT_1  (1 << 1)
#define BIT_2  (1 << 2)
#define BIT_3  (1 << 3)
#define BIT_4  (1 << 4)
// ...
```

---

## 10. 任务通知 API

> 无需额外内存分配，每个任务自带 32 位通知值

### 10.1 发送通知

```c
// 发送通知（可设置值、增加位、覆写等）
BaseType_t xTaskNotify(
    TaskHandle_t xTaskToNotify,
    uint32_t ulValue,
    eNotifyAction eAction        // eNoAction, eSetBits, eIncrement,
);                               // eSetValueWithOverwrite, eSetValueWithoutOverwrite

// 带中断机制的发送
BaseType_t xTaskNotifyFromISR(
    TaskHandle_t xTaskToNotify,
    uint32_t ulValue,
    eNotifyAction eAction,
    BaseType_t *pxHigherPriorityTaskWoken
);

// 简化版：通知并递增通知值
BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify);
BaseType_t xTaskNotifyGiveFromISR(
    TaskHandle_t xTaskHandle,
    BaseType_t *pxHigherPriorityTaskWoken
);
```

### 10.2 接收通知

```c
// 阻塞等待通知
BaseType_t xTaskNotifyWait(
    uint32_t ulBitsToClearOnEntry,     // 进入时清除的位
    uint32_t ulBitsToClearOnExit,      // 退出时清除的位
    uint32_t *pulNotificationValue,    // [OUT] 接收到的值
    TickType_t xTicksToWait            // 超时
);

// 简化版
uint32_t ulTaskNotifyTake(
    BaseType_t xClearCountOnExit,      // pdTRUE: 清零, pdFALSE: 递减
    TickType_t xTicksToWait            // 超时
);
```

---

## 11. CH32V307 中断与 FreeRTOS 集成

### 11.1 CH32V307 中断控制器 (PFIC)

CH32V307 使用 PFIC (Programmable Fast Interrupt Controller) 而非标准 RISC-V 的 PLIC。  
PFIC 的编程接口与 ARM NVIC 高度兼容，因此在 `portmacro.h` 中提供了一组宏来操作中断。

### 11.2 中断优先级

```c
// 在 FreeRTOSConfig.h 中:
#define configPRIO_BITS                  2   // 2位优先级 (0-3)
#define configKERNEL_INTERRUPT_PRIORITY  0xFF  // 内核临界区优先级
#define configMAX_API_CALL_INTERRUPT_PRIORITY  0  // 可使用 FreeRTOS API 的最高中断优先级

// 注意: CH32V307 的 PFIC 优先级值越小，优先级越高
// 0 = 最高优先级, 3 = 最低优先级
```

### 11.3 中断安全API规则

**在中断服务函数中，必须使用 FromISR 版本的 API**，且必须检查是否需要上下文切换：

```c
void USART1_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART1);

        // 只能使用 FromISR 版本
        xQueueSendFromISR(xQueue, &data, &xHigherPriorityTaskWoken);
        // 或
        xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);
        // 或
        vTaskNotifyGiveFromISR(xTaskHandle, &xHigherPriorityTaskWoken);
    }

    // 如果通知更高优先级任务，请求上下文切换
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
// 其中 portYIELD_FROM_ISR() 展开为:
// #define portYIELD_FROM_ISR(x)  if(x) { vTaskSwitchContext(); }
```

### 11.4 FreeRTOS Tick 中断

```c
// FreeRTOS 的 tick 中断由 TIM2 或 SysTick 提供（本项目使用 TIM2）
// 在 ch32v30x_it.c 中:

void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            // 务必先清除标志再调用 xPortSysTickHandler
            xPortSysTickHandler();
        }
    }
}

// 注意: xPortSysTickHandler 是 FreeRTOS 的 Tick 入口函数
// 定义在 FreeRTOS/portable/GCC/RISC-V/port.c
```

### 11.5 不可使用 FreeRTOS API 的中断

```c
// 优先级高于 configMAX_API_CALL_INTERRUPT_PRIORITY 的中断
// 绝对不能调用任何 FreeRTOS API 函数
// 这些中断通常用于硬实时场景
```

### 11.6 CH32V307 中断服务函数命名（必须精确匹配）

```c
// 在使用 FreeRTOS 时，以下中断函数名是启动文件中固定的：
void NMI_Handler(void);               // NMI
void HardFault_Handler(void);         // 硬错误
void SysTick_Handler(void);           // SysTick (本项目使用 TIM2 替代)
void TAMPER_IRQHandler(void);         // 篡改检测
void TIM2_IRQHandler(void);           // TIM2 全局中断 (FreeRTOS Tick)
void TIM3_IRQHandler(void);           // TIM3 全局中断
void USART1_IRQHandler(void);         // USART1
void USART2_IRQHandler(void);         // USART2
void EXTI0_IRQHandler(void);          // EXTI0
void EXTI1_IRQHandler(void);          // EXTI1
void DMA1_Channel1_IRQHandler(void);  // DMA1 通道1
// ... 所有中断函数名 = IRQn 枚举名 + _IRQHandler
```

### 11.7 临界区

```c
// 方法1: 全局中断控制（最简单）
taskENTER_CRITICAL();       // 关闭中断
// ... 临界区代码 ...
taskEXIT_CRITICAL();        // 恢复中断

// 方法2: 挂起调度器（不关中断，其他任务不会切换）
vTaskSuspendAll();
// ... 临界区代码（可以被中断，但不会被其他任务打断）...
xTaskResumeAll();
```

---

## 12. CH32V307 外设与 FreeRTOS 整合示例

### 12.1 完整的双任务 GPIO 翻转

```c
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"

#define TASK1_PRIO     5
#define TASK1_STK_SIZE 256
#define TASK2_PRIO     5
#define TASK2_STK_SIZE 256

TaskHandle_t xTask1Handle;
TaskHandle_t xTask2Handle;

void vTask1(void *pvParameters) {
    while(1) {
        printf("task1 entry\r\n");
        GPIO_SetBits(GPIOA, GPIO_Pin_0);      // PA0 高
        vTaskDelay(250);                       // 延时 250 Ticks = 500ms
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);     // PA0 低
        vTaskDelay(250);
    }
}

void vTask2(void *pvParameters) {
    while(1) {
        printf("task2 entry\r\n");
        GPIO_ResetBits(GPIOA, GPIO_Pin_1);     // PA1 低
        vTaskDelay(500);                       // 延时 500 Ticks = 1000ms
        GPIO_SetBits(GPIOA, GPIO_Pin_1);       // PA1 高
        vTaskDelay(500);
    }
}

void GPIO_Toggle_INIT(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

int main(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    printf("FreeRTOS Kernel Version:%s\r\n", tskKERNEL_VERSION_NUMBER);

    GPIO_Toggle_INIT();

    xTaskCreate(vTask1, "task1", TASK1_STK_SIZE, NULL, TASK1_PRIO, &xTask1Handle);
    xTaskCreate(vTask2, "task2", TASK2_STK_SIZE, NULL, TASK2_PRIO, &xTask2Handle);

    vTaskStartScheduler();  // 启动调度器，不会返回

    while(1) {
        printf("shouldn't run at here!!\n");
    }
}
```

### 12.2 串口 DMA + FreeRTOS 队列

```c
#include "ch32v30x_usart.h"
#include "ch32v30x_dma.h"

QueueHandle_t xUartRxQueue;
#define UART_QUEUE_LEN  128

void USART1_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART1);
        xQueueSendFromISR(xUartRxQueue, &data, &xHigherPriorityTaskWoken);
    }

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void vUartRxTask(void *pvParameters) {
    uint8_t data;
    while(1) {
        if (xQueueReceive(xUartRxQueue, &data, portMAX_DELAY) == pdPASS) {
            printf("RX: 0x%02X '%c'\r\n", data, data);
        }
    }
}

void vUartInitTask(void *pvParameters) {
    // 初始化串口
    USART_InitTypeDef USART_InitStructure = {0};
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    // PA9 TX, PA10 RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

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

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);

    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 2);

    // 创建队列
    xUartRxQueue = xQueueCreate(UART_QUEUE_LEN, sizeof(uint8_t));

    // 创建接收任务
    xTaskCreate(vUartRxTask, "uart_rx", 256, NULL, 3, NULL);

    vTaskDelete(NULL);  // 初始化完成，删除自身
}
```

### 12.3 使用信号量同步外设操作

```c
SemaphoreHandle_t xAdcSemaphore;

void ADC_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // ADC 转换完成中断
    xSemaphoreGiveFromISR(xAdcSemaphore, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void vAdcTask(void *pvParameters) {
    xAdcSemaphore = xSemaphoreCreateBinary();

    // 初始化 ADC ...
    // ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
    // NVIC_EnableIRQ(ADC_IRQn);

    while(1) {
        // 启动 ADC 转换
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);

        // 等待 ADC 中断给出信号量
        if (xSemaphoreTake(xAdcSemaphore, pdMS_TO_TICKS(100)) == pdPASS) {
            uint16_t value = ADC_GetConversionValue(ADC1);
            printf("ADC: %d\r\n", value);
        }
    }
}
```

---

## 13. 内存管理详解

### 13.1 堆内存布局

```c
// configTOTAL_HEAP_SIZE = 12 * 1024 = 12288 字节
// heap_4.c 在一个大数组中管理内存:
// static uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];

// heap_4 特点:
// 1. 支持 malloc/free (pvPortMalloc / vPortFree)
// 2. 支持合并相邻空闲块（减少碎片）
// 3. 首次适应分配策略
// 4. 线程安全（分配/释放时关中断）
```

### 13.2 栈大小计算

```c
// 任务栈单位: 字 (StackType_t = uint32_t = 4字节)
// 所以:
// configMINIMAL_STACK_SIZE = 256 字 = 1024 字节
// configTIMER_TASK_STACK_DEPTH = 512 字 = 2048 字节

// 评估栈用量:
// 基础任务（无局部大数组）: 约 100-200 字
// 含 printf 调用: +100 字
// 含大量局部变量: 按需增加

// 查看栈剩余:
// UBaseType_t free = uxTaskGetStackHighWaterMark(xTaskHandle);
// free 返回值单位: 字。值越小越接近溢出。
```

### 13.3 总内存占用估算

```c
// 固定消耗:
// FreeRTOS 内核数据结构: ~2KB
// 空闲任务栈: 256 字 = 1KB
// 定时器任务栈: 512 字 = 2KB
// 定时器队列: 10项 * sizeof(void*) * 2 ≈ 80B

// 每个任务额外消耗:
// TCB (任务控制块): ~80 字节
// 任务栈: 栈大小(字) * 4

// 例如: 创建 3 个 256 字任务
// 总堆消耗 ≈ 2KB + 1KB + 2KB + 3*(80 + 1KB) ≈ 8.2KB
// 剩余堆: 12KB - 8.2KB ≈ 3.8KB (用于队列、信号量等)
```

---

## 14. 移植层关键细节

### 14.1 上下文切换

```c
// 文件: FreeRTOS/portable/GCC/RISC-V/port.c
// 文件: FreeRTOS/portable/GCC/RISC-V/portASM.S

// CH32V307 上下文切换流程:
// 1. portYIELD() -> 触发软件中断 (ecall)
// 2. 在 ecall 异常处理中保存当前任务寄存器
// 3. 调用 vTaskSwitchContext() 选择下一个任务
// 4. 恢复新任务的寄存器
// 5. mret 返回新任务

// 保存的寄存器: ra, sp, gp, tp, s0-s11 (Callee-saved registers)

// 特别注意:
// CH32V307 不支持标准 RISC-V 的 mtime/mtimecmp (configMTIME_BASE_ADDRESS=0)
// Tick 中断通过 TIM2 实现，而非机器定时器
```

### 14.2 启动文件中的中断向量表

```c
// 文件: Startup/startup_ch32v30x_D8C.S
// 启动文件中预设的中断处理函数名（钩子）:
// .weak 表示可以被用户重写

.weak NMI_Handler
.weak HardFault_Handler
.weak TIM2_IRQHandler      // FreeRTOS Tick 使用
.weak SysTick_Handler      // 备用 Tick 源
// ...
```

---

## 15. 常见问题与解决方案

### 问题1: 任务创建失败

```c
// 检查点:
// 1. configTOTAL_HEAP_SIZE 是否足够
// 2. 任务栈大小是否合理 (最小 256 字)
// 3. 返回值: xTaskCreate 返回 errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY
// 解决: 增大 configTOTAL_HEAP_SIZE 或减小栈大小
```

### 问题2: HardFault / 栈溢出

```c
// 检查点:
// 1. 是否启用 configCHECK_FOR_STACK_OVERFLOW
// 2. 检查 uxTaskGetStackHighWaterMark 返回值
// 3. 检查中断嵌套是否过深
// 解决:
//   - 增大任务栈
//   - 使用 vApplicationStackOverflowHook() 定位溢出任务
// void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
//     printf("Stack Overflow! Task: %s\r\n", pcTaskName);
//     while(1);
// }
```

### 问题3: 队列/信号量操作在中断中无效

```c
// 原因: 在中断中使用了非 FromISR 版本 API
// 解决: 确保使用 xQueueSendFromISR / xSemaphoreGiveFromISR 等
```

### 问题4: vTaskDelay 不准确

```c
// 原因: configTICK_RATE_HZ = 500, 所以每个 Tick = 2ms
// vTaskDelay(1) 延时 2ms, 不是 1ms

// 精确延时请使用绝对延时:
// TickType_t xLastWakeTime = xTaskGetTickCount();
// while(1) {
//     vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));  // 精确 10ms
// }
```

### 问题5: 多个任务同时使用 printf 导致输出混乱

```c
// 原因: printf 不是线程安全的
// 解决: 使用互斥锁保护
// xSemaphoreTake(xPrintfMutex, portMAX_DELAY);
// printf("message\r\n");
// xSemaphoreGive(xPrintfMutex);
```

---

## 16. 完整的项目模板

```c
// main.c - FreeRTOS + CH32V307 项目模板

#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"

// ── 任务参数 ──
#define TASK_A_PRIO     5
#define TASK_A_STK      256
#define TASK_B_PRIO     4
#define TASK_B_STK      256

TaskHandle_t xTaskAHandle;
TaskHandle_t xTaskBHandle;

// ── 外设初始化 ──
static void prvGPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

// ── 任务实现 ──
static void prvTaskA(void *pvParameters) {
    while(1) {
        GPIO_SetBits(GPIOA, GPIO_Pin_0);
        vTaskDelay(pdMS_TO_TICKS(500));
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void prvTaskB(void *pvParameters) {
    while(1) {
        GPIO_SetBits(GPIOA, GPIO_Pin_1);
        vTaskDelay(pdMS_TO_TICKS(200));
        GPIO_ResetBits(GPIOA, GPIO_Pin_1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ── 主函数 ──
int main(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);

    printf("FreeRTOS CH32V307 Template\r\n");
    printf("SysClk: %d Hz\r\n", SystemCoreClock);

    prvGPIO_Init();

    xTaskCreate(prvTaskA, "TaskA", TASK_A_STK, NULL, TASK_A_PRIO, &xTaskAHandle);
    xTaskCreate(prvTaskB, "TaskB", TASK_B_STK, NULL, TASK_B_PRIO, &xTaskBHandle);

    vTaskStartScheduler();

    while(1);
}
```

---

## 17. FreeRTOS 版本信息

```c
// FreeRTOS 内核版本字符串
// 可用: printf("Version: %s\r\n", tskKERNEL_VERSION_NUMBER);
// 输出: "V10.4.6"

// 配置参数汇总速查:
// Tick 频率:       500 Hz (2ms)
// 最大优先级:      15 (0-14)
// 堆大小:          12 KB
// 最小栈:          256 字 (1 KB)
// 抢占模式:        是
// 互斥锁:           是
// 递归互斥锁:      是
// 计数信号量:      是
// 软件定时器:      是
// 事件组:           是
// 任务通知:        是
// 栈溢出检测:      是
```

---

## 附录A: 链接脚本关键段定义

```c
// 文件: Ld/Link.ld

// FLASH: 0x00000000, 大小 512KB (CH32V307)
// RAM:   0x20000000, 大小 64KB

// 关键段:
// .text   - 代码段 (FLASH)
// .rodata - 只读数据 (FLASH)
// .data   - 初始化数据 (FLASH->RAM)
// .bss    - 未初始化数据 (RAM)
// _heap   - 堆起始地址
// _eheap  - 堆结束地址 (由 _heap + configTOTAL_HEAP_SIZE 决定)
```

## 附录B: API 速查表

| 功能 | 关键函数 | 头文件 |
|------|----------|--------|
| 创建任务 | `xTaskCreate()` | task.h |
| 删除任务 | `vTaskDelete()` | task.h |
| 相对延时 | `vTaskDelay(Ticks)` | task.h |
| 绝对延时 | `vTaskDelayUntil()` | task.h |
| 挂起/恢复 | `vTaskSuspend/Resume()` | task.h |
| 创建队列 | `xQueueCreate()` | queue.h |
| 发送队列 | `xQueueSend()` | queue.h |
| 接收队列 | `xQueueReceive()` | queue.h |
| 创建互斥锁 | `xSemaphoreCreateMutex()` | semphr.h |
| 二进制信号量 | `xSemaphoreCreateBinary()` | semphr.h |
| 计数信号量 | `xSemaphoreCreateCounting()` | semphr.h |
| 获取信号量 | `xSemaphoreTake()` | semphr.h |
| 释放信号量 | `xSemaphoreGive()` | semphr.h |
| 创建定时器 | `xTimerCreate()` | timers.h |
| 启动定时器 | `xTimerStart()` | timers.h |
| 事件组 | `xEventGroupCreate()` | event_groups.h |
| 设置事件位 | `xEventGroupSetBits()` | event_groups.h |
| 等待事件位 | `xEventGroupWaitBits()` | event_groups.h |
| 任务通知 | `xTaskNotifyGive()` | task.h |
| 等待通知 | `ulTaskNotifyTake()` | task.h |
| 临界区 | `taskENTER_CRITICAL()` | FreeRTOS.h |
| 中断FromISR | `xxxFromISR()` | 对应头文件 |

---

*文档版本：V2.0 (AI增强版)*  
*生成日期：2024年*  
*目标：AI可直接根据本文档编写正确的 CH32V307-FreeRTOS 代码，无需查看实际源文件*
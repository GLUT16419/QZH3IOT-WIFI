# FreeRTOS 进程间通信(IPC)模块

包含：队列、二进制信号量、计数信号量、互斥锁、递归互斥锁。

---

## 1. 头文件

```c
#include "queue.h"      // 队列API
#include "semphr.h"     // 信号量/互斥锁API (注意顺序: include semphr.h后自动引入queue.h)
```

## 2. 队列 API

### 创建

```c
QueueHandle_t xQueueCreate(
    UBaseType_t uxQueueLength,   // 元素个数
    UBaseType_t uxItemSize       // 每个元素大小(字节)
);
// 成功返回句柄，失败返回NULL
```

### 发送

```c
// 尾部发送 (可阻塞)
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);

// 头部发送
BaseType_t xQueueSendToFront(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);

// 尾部发送 (等价于 xQueueSend)
BaseType_t xQueueSendToBack(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);

// 中断中发送 (无阻塞)
BaseType_t xQueueSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken);
```

### 接收

```c
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
BaseType_t xQueueReceiveFromISR(QueueHandle_t xQueue, void *pvBuffer, BaseType_t *pxHigherPriorityTaskWoken);
BaseType_t xQueuePeek(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait); // 查看不移除
```

### 队列控制

```c
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue);                // 队列中消息数
UBaseType_t uxQueueMessagesWaitingFromISR(QueueHandle_t xQueue);
void vQueueDelete(QueueHandle_t xQueue);
```

### 队列使用示例

```c
QueueHandle_t xQueue;

// 发送任务
void vSender(void *pvParameters) {
    uint32_t data = 0;
    while(1) {
        xQueueSend(xQueue, &data, portMAX_DELAY);
        data++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 接收任务
void vReceiver(void *pvParameters) {
    uint32_t recv;
    while(1) {
        if (xQueueReceive(xQueue, &recv, portMAX_DELAY) == pdPASS) {
            printf("Recv: %lu\r\n", recv);
        }
    }
}

// 初始化
void vInit(void) {
    xQueue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(vSender, "Sender", 256, NULL, 3, NULL);
    xTaskCreate(vReceiver, "Receiver", 256, NULL, 3, NULL);
}
```

## 3. 信号量 API

```c
// ── 二进制信号量 ──
SemaphoreHandle_t xSemaphoreCreateBinary(void);

// ── 计数信号量 (需要 configUSE_COUNTING_SEMAPHORES=1) ──
SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount);
```

## 4. 互斥锁 API

```c
// ── 互斥锁 (需要 configUSE_MUTEXES=1) ──
SemaphoreHandle_t xSemaphoreCreateMutex(void);           // 创建互斥锁
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void);  // 创建递归互斥锁
```

## 5. 获取和释放

```c
// ── 获取 (Pend/Wait) ──
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait);
// 中断中获取: 不允许，因为可能阻塞

// ── 释放 (Give) ──
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);

// 中断中释放
BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken);

// 递归互斥锁
BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t xMutex);
BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t xMutex);
```

## 6. 典型用法

### 互斥锁保护共享资源

```c
SemaphoreHandle_t xMutex;

void vSharedTask(void *pvParameters) {
    while(1) {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        // 临界区: 访问共享资源
        shared_value++;
        // 临界区结束
        xSemaphoreGive(xMutex);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 初始化
xMutex = xSemaphoreCreateMutex();
```

### 二进制信号量同步 (从ISR到任务)

```c
SemaphoreHandle_t xSemBinary;

void vSyncTask(void *pvParameters) {
    while(1) {
        // 等待中断触发
        xSemaphoreTake(xSemBinary, portMAX_DELAY);
        printf("ISR triggered task\r\n");
    }
}

void EXTI0_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (EXTI_GetITStatus(EXTI_Line0) != RESET) {
        xSemaphoreGiveFromISR(xSemBinary, &xHigherPriorityTaskWoken);
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// 初始化
xSemBinary = xSemaphoreCreateBinary();
```

---

*注意: 创建信号量/互斥锁会消耗堆内存。每次 xSemaphoreCreateBinary 大约消耗 80 字节。*

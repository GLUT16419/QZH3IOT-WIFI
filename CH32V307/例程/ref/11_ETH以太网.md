# ETH 以太网 + WCHNET 协议栈模块

包含：网络配置(net_config.h)、关键数据结构、完整API函数原型、TCP服务器完整代码示例。

---

## 1. 头文件

```c
#include "eth_driver.h"     // 文件: ETH/NetLib/eth_driver.h - MAC驱动
#include "wchnet.h"         // 文件: ETH/NetLib/wchnet.h - 协议栈API
#include "net_config.h"     // 文件: User/net_config.h - 协议栈配置
#include "string.h"         // memset 等
```

## 2. 网络配置文件 (net_config.h)

```c
#define WCHNET_NUM_IPRAW        0   // IPRAW连接数
#define WCHNET_NUM_UDP          0   // UDP连接数
#define WCHNET_NUM_TCP          1   // TCP连接数
#define WCHNET_NUM_TCP_LISTEN   0   // TCP监听数
#define WCHNET_MAX_SOCKET_NUM  (WCHNET_NUM_IPRAW+WCHNET_NUM_UDP+WCHNET_NUM_TCP+WCHNET_NUM_TCP_LISTEN)
#define WCHNET_TCP_MSS          1460  // TCP MSS 大小

#define ETH_TXBUFNB             2    // MAC发送描述符数
#define ETH_RXBUFNB             4    // MAC接收描述符数
#define ETH_RX_BUF_SZE          1520 // MAC接收缓冲区长度
#define ETH_TX_BUF_SZE          1520 // MAC发送缓冲区长度

#define WCHNET_PING_ENABLE      1    // 启用PING
#define TCP_RETRY_COUNT         20   // TCP重传次数
#define TCP_RETRY_PERIOD        10   // TCP重传周期(50ms单位)

#define RECE_BUF_LEN            (WCHNET_TCP_MSS*2)
#define WCHNET_NUM_TCP_SEG      (WCHNET_NUM_TCP*2)
#define WCHNET_MEM_HEAP_SIZE    (((WCHNET_TCP_MSS+0x10+54)*WCHNET_NUM_TCP_SEG)+ETH_TX_BUF_SZE+64)
#define WCHNET_NUM_ARP_TABLE    50
#define WCHNET_NUM_PBUF         (WCHNET_MAX_SOCKET_NUM+WCHNET_NUM_TCP)
```

## 3. 关键数据结构

```c
// Socket信息
typedef struct _SOCK_INF {
    uint32_t IntStatus;        // 中断状态
    uint32_t SockIndex;        // Socket索引
    uint32_t RecvStartPoint;   // 接收缓冲区起始
    uint32_t RecvBufLen;       // 接收缓冲区长度
    uint32_t RecvCurPoint;     // 当前指针
    uint32_t RecvReadPoint;    // 读指针
    uint32_t RecvRemLen;       // 剩余长度
    uint32_t ProtoType;        // 协议类型
    uint32_t SockStatus;       // Socket状态
    uint32_t DesPort;          // 目标端口
    uint32_t SourPort;         // 源端口
    uint8_t  IPAddr[4];        // 目标IP
    void *Resv1;
    void *Resv2;
    pSockRecv AppCallBack;     // 接收回调
} SOCK_INF;

// Keep Alive 配置
struct _KEEP_CFG {
    uint32_t KLIdle;    // 空闲时间(ms)
    uint32_t KLIntvl;   // 探测间隔(ms)
    uint32_t KLCount;   // 探测次数
};
```

## 4. 关键宏定义

```c
// 协议类型
#define PROTO_TYPE_IP_RAW  0
#define PROTO_TYPE_UDP     2
#define PROTO_TYPE_TCP     3

// 全局中断
#define GINT_STAT_UNREACH      (1 << 0)
#define GINT_STAT_IP_CONFLI    (1 << 1)
#define GINT_STAT_PHY_CHANGE   (1 << 2)
#define GINT_STAT_SOCKET       (1 << 4)

// Socket中断
#define SINT_STAT_RECV        (1 << 2)   // 收到数据
#define SINT_STAT_CONNECT     (1 << 3)   // 连接成功
#define SINT_STAT_DISCONNECT  (1 << 4)   // 断开

// TCP状态
#define TCP_CLOSED      0
#define TCP_LISTEN      1
#define TCP_ESTABLISHED 4
```

## 5. 完整API函数原型

```c
// 初始化
uint8_t WCHNET_Init(const uint8_t *ip, const uint8_t *gwip, const uint8_t *mask, const uint8_t *macaddr);
uint8_t ETH_LibInit(uint8_t *ip, uint8_t *gwip, uint8_t *mask, uint8_t *macaddr);
void    WCHNET_GetMacAddr(uint8_t *macaddr);
uint8_t WCHNET_ConfigLIB(struct _WCH_CFG *cfg);
void    WCHNET_ConfigKeepLive(struct _KEEP_CFG *cfg);
uint32_t WCHNET_GetPHYStatus(void);

// 主循环
void WCHNET_PeriodicHandle(void);   // 定时调用 (通常TIM2中断中)
void WCHNET_MainTask(void);         // 主循环调用

// Socket管理
uint8_t WCHNET_SocketCreat(uint8_t *id, SOCK_INF *p);
uint8_t WCHNET_SocketListen(uint8_t id);
uint8_t WCHNET_SocketConnect(uint8_t id);
uint8_t WCHNET_SocketClose(uint8_t id, uint8_t mode);

// 数据收发
uint8_t WCHNET_SocketSend(uint8_t id, uint8_t *buf, uint32_t *len);
uint8_t WCHNET_SocketRecv(uint8_t id, uint8_t *buf, uint32_t *len);
uint32_t WCHNET_SocketRecvLen(uint8_t id, uint32_t *remain);

// 中断查询
uint8_t WCHNET_QueryGlobalInt(void);
uint8_t WCHNET_GetGlobalInt(void);
uint8_t WCHNET_GetSocketInt(uint8_t id);
void    WCHNET_SetSocketInt(uint8_t id, uint8_t s);
```

## 6. RMII引脚连接 (CH32V307)

| 引脚 | 功能 |
|------|------|
| PA1 | ETH_CRS_DV |
| PA2 | ETH_TXD0 |
| PA3 | ETH_TXD1 |
| PA7 | ETH_RXDV |
| PC1 | ETH_MDC |
| PC2 | ETH_MDIO |
| PB13 | ETH_TX_CLK |
| PD8 | ETH_RXD0 |
| PD9 | ETH_RXD1 |
| PD10 | ETH_RX_CLK |

## 7. TCP服务器完整代码

```c
#include "debug.h"
#include "string.h"
#include "wchnet.h"
#include "eth_driver.h"

u8 MACAddr[6];
u8 IPAddr[4] = { 192, 168, 1, 10 };
u8 GWIPAddr[4] = { 192, 168, 1, 1 };
u8 IPMask[4] = { 255, 255, 255, 0 };
u16 srcport = 1000;
u8 SocketIdForListen;

void TIM2_Init(void) {
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseStructure.TIM_Period = SystemCoreClock / 1000000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 50 * 1000 - 1;   // 50ms
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    NVIC_EnableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void) {
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        WCHNET_PeriodicHandle();  // 50ms定时调用
    }
}

int main(void) {
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("TcpServer Test\r\n");

    WCHNET_GetMacAddr(MACAddr);
    TIM2_Init();
    ETH_LibInit(IPAddr, GWIPAddr, IPMask, MACAddr);

    // 创建TCP监听Socket
    SOCK_INF TmpSocketInf = {0};
    TmpSocketInf.SourPort = srcport;
    TmpSocketInf.ProtoType = PROTO_TYPE_TCP;
    WCHNET_SocketCreat(&SocketIdForListen, &TmpSocketInf);
    WCHNET_SocketListen(SocketIdForListen);

    while(1) {
        WCHNET_MainTask();
        if (WCHNET_QueryGlobalInt()) {
            // 逐个检查Socket中断
            for (u16 i = 0; i < WCHNET_MAX_SOCKET_NUM; i++) {
                u8 socketint = WCHNET_GetSocketInt(i);
                if (socketint & SINT_STAT_RECV) {
                    u32 remain;
                    u32 len = WCHNET_SocketRecvLen(i, &remain);
                    u8 *buf = pvPortMalloc(len);
                    if (buf) {
                        WCHNET_SocketRecv(i, buf, &len);
                        WCHNET_SocketSend(i, buf, &len);  // 回显
                        vPortFree(buf);
                    }
                }
                if (socketint & SINT_STAT_DISCONNECT) {
                    printf("Socket %d disconnected\r\n", i);
                }
            }
        }
    }
}
```

---

*对应文件: ETH/NetLib/wchnet.h, ETH/NetLib/eth_driver.h, User/net_config.h*

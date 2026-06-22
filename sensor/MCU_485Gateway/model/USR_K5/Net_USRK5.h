#ifndef __NET_USRK5_H
#define __NET_USRK5_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"


// 是否设置静态IP地址: 1=静态IP, 0=DHCP
#define  USRK5_STAT_IP_MODE       1

#if USRK5_STAT_IP_MODE            // 设置的静态IP地址
#define USRK5_STAT_IP_ADDR        "192.168.31.240"
#define USRK5_STAT_IP_MASK        "255.255.255.0"
#define USRK5_STAT_IP_GATEWAY     "192.168.31.1"
#endif

// 模块串口参数 (与MCU侧USART2保持一致)
#define USRK5_UART_BAUD         115200u            /* USR-K5 模块串口波特率 */
#define USRK5_UART_DATA_BITS    8u               /* 数据位 */
#define USRK5_UART_STOP_BITS    1u               /* 停止位 */
#define USRK5_UART_PARITY       0u               /* 校验位: 0=无, 1=奇, 2=偶 */

// TCPC客户端--远程服务器的IP地址和端口
// #define SERVER_IP       "192.168.31.213"
// #define SERVER_PORT     1883


/* ==================== 引脚宏定义 ==================== */

/**
  * @brief  USR-K5 控制引脚定义
  *         PA5 -- Reload(6)  低电平持续 3s 恢复出厂设置
  *         PA6 -- RST(4)     低电平持续 300ms 硬件复位
  */
#define USRK5_CTRL_PORT       GPIOA               /* 控制端口 */
#define USRK5_RELOAD_PIN      GPIO_Pin_5          /* Reload 引脚: PA5 */
#define USRK5_RST_PIN         GPIO_Pin_6          /* RST 引脚:    PA6 */

/**
  * @brief  USR-K5 通信串口 (USART2)
  *         TXD(9)  -- PA3 (USART2_RX)  模块发送→MCU接收
  *         RXD(8)  -- PA2 (USART2_TX)  MCU发送→模块接收
  */
#define USRK5_USART           USART2              /* 使用 USART2 与模块通信 */
#define USRK5_UART_DEV        g_uart2_dev         /* USART2 设备实例 */

/* ==================== 网络模式枚举 ==================== */

typedef enum {
    USRK5_MODE_TCP_CLIENT = 0,                   /* TCP 客户端模式 */
    USRK5_MODE_TCP_SERVER = 1,                   /* TCP 服务器模式 */
    USRK5_MODE_UDP_CLIENT = 2,                   /* UDP 客户端模式 */
    USRK5_MODE_UDP_SERVER = 3                    /* UDP 服务器模式 */
} usrk5_net_mode_t;

/* ==================== 设备状态枚举 ==================== */

typedef enum {
    USRK5_STATE_INIT = 0,                        /* 初始化状态 */
    USRK5_STATE_READY,                           /* 就绪（已进入 AT 模式或工作模式） */
    USRK5_STATE_ERROR                            /* 错误状态 */
} usrk5_state_t;

/* ==================== 函数声明 ==================== */

void usrk5_init(void);                                             /* 初始化 USR-K5：PA5/PA6 + USART2 9600bps */
uint8_t usrk5_Net_init(void);                                    /* 初始化 K5 模块并配置网络参数，返回 0 成功 */
void usrk5_hw_reset(void);                                         /* 硬件复位：PA6 拉低 300ms */
void usrk5_reload_default(void);                                   /* 恢复出厂：PA5 拉低 3s */

uint8_t usrk5_enter_at_mode(void);                                 /* 进入 AT 指令模式，返回 0 成功 */
uint8_t usrk5_exit_at_mode(void);                                  /* 退出 AT 指令模式，返回 0 成功 */
uint8_t usrk5_send_at_cmd(const char *cmd, char *resp, uint16_t resp_size, uint32_t timeout_ms); /* 发送 AT 指令并等待应答 */

uint8_t usrk5_set_net_mode(usrk5_net_mode_t mode);                 /* 设置网络模式: TCP/UDP Client/Server */
uint8_t usrk5_set_remote_ip(const char *ip, uint16_t port);        /* 设置远端 IP 和端口 */
uint8_t usrk5_set_local_port(uint16_t port);                       /* 设置本地端口 */
uint8_t usrk5_set_static_ip(const char *ip, const char *mask, const char *gateway); /* 设置模块静态 IP */
uint8_t usrk5_set_dhcp(void);                                      /* 设置模块 DHCP 动态获取 IP */
uint8_t usrk5_set_uart(uint32_t baud, uint8_t data_bits, uint8_t stop_bits, uint8_t parity); /* 设置模块串口参数 */
uint8_t usrk5_query_ip(char *ip_buf, uint16_t buf_size);           /* 查询模块 IP 地址 */

uint8_t usrk5_net_send(const uint8_t *data, uint16_t len);         /* 通过网络发送数据 */
uint8_t usrk5_net_is_connected(void);                              /* 检查网络是否连接，返回 1 已连接 */
uint8_t usrk5_net_check_connect(void);                             /* 检查 TCP 连接状态，0=已连接 */

#ifdef __cplusplus
}
#endif

#endif /* __NET_USRK5_H */





/*
K5模块AT指令合集：

从透传模式进入AT指令模式： 
串口连续发送：+->+->+
模块收到300ms内向串口回复 "a"
串口收到后3S内给模块回复 "a"
模块在接收到‘a’后，给串口发送“+OK”
进入“AT 指令模式”
串口接收到“+OK”后，知道模块已进入“AT 指令模式”，可以向其发送 AT 指令
退出AT指令模式：AT+ENTM




AT+WANN:  设置和查询k5的IP获取方式，IP/子网掩码/网关参数
AT+DNS: 设置和查询K5的DNS服务器地址
AT+RELD: 恢复出厂设置
AT+SOCK: 设置K5 Socket工作方式/目标IP/目标端口


*/








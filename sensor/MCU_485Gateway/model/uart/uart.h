#ifndef __UART_H
#define __UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

/* ==================== 宏定义 ==================== */
#define UART_TX_BUF_SIZE  128  /* 发送环形缓冲区容量（字节） */
#define UART_RX_BUF_SIZE  256  /* 接收环形缓冲区容量（字节） */



#define UART1_BAUDRATE  9600    /* U1波特率设置 */
#define UART2_BAUDRATE  115200    /* U1波特率设置 */
#define UART3_BAUDRATE  115200    /* U1波特率设置 */

/* ==================== 结构体定义 ==================== */

/**
  * @brief  UART 设备管理结构体
  * @note   每个 USART 外设对应一个全局实例，所有发送/接收状态集中管理
  *         tx_buf/rx_buf 为环形缓冲区，通过 head/tail 指针管理读写位置
  */
typedef struct {
    uint8_t  tx_buf[UART_TX_BUF_SIZE];     /* 发送环形缓冲区 */
    volatile uint16_t tx_head;             /* 发送缓冲区头指针（ISR 读取位置） */
    volatile uint16_t tx_tail;             /* 发送缓冲区尾指针（用户写入位置） */
    volatile uint8_t  tx_busy;             /* 发送忙标志：0=空闲，1=发送中 */

    uint8_t  rx_buf[UART_RX_BUF_SIZE];     /* 接收环形缓冲区 */
    volatile uint16_t rx_head;             /* 接收缓冲区头指针（用户读取位置） */
    volatile uint16_t rx_tail;             /* 接收缓冲区尾指针（ISR 写入位置） */
    volatile uint8_t  rx_complete;         /* 接收帧完成标志：0=未完成，1=一帧接收完毕 */
    volatile uint16_t rx_frame_len;        /* 当前帧接收的字节数 */
} uart_dev_t;


/* ==================== 串口选择枚举 ==================== */

/**
  * @brief  串口选择枚举，用于统一发送/接收接口的参数化
  */
typedef enum {
    uart1 = 1,                             /* USART1: TX=PA9, RX=PA10 */
    uart2 = 2,                             /* USART2: TX=PA2, RX=PA3  */
    uart3 = 3                              /* USART3: TX=PB10, RX=PB11 */
} uart_port_t;

/* ==================== 全局实例声明 ==================== */
extern uart_dev_t g_uart1_dev;             /* USART1 全局设备实例 */
extern uart_dev_t g_uart2_dev;             /* USART2 全局设备实例 */
extern uart_dev_t g_uart3_dev;             /* USART3 全局设备实例 */

/* ==================== USART1 函数声明 ==================== */
/* 引脚: TX=PA9, RX=PA10  |  时钟: APB2 72MHz */

void     uart1_init(void);                                           /* USART1 初始化（9600-8-N-1，TX+中断RX） */
void     uart1_send_string(const char *str);                         /* 发送字符串 */
void     uart1_send_hex(const uint8_t *data, uint16_t len);          /* 发送十六进制数据 */
uint16_t uart1_recv_data(uint8_t *buf, uint16_t buf_size);           /* 从接收缓冲区取出数据 */
void     uart1_clear_rx_flag(void);                                  /* 清除接收完成标志 */

/* ==================== USART2 函数声明 ==================== */
/* 引脚: TX=PA2, RX=PA3  |  时钟: APB1 36MHz */

void     uart2_init(void);                                           /* USART2 初始化（9600-8-N-1，TX+中断RX） */
void     uart2_send_string(const char *str);                         /* 发送字符串 */
void     uart2_send_hex(const uint8_t *data, uint16_t len);          /* 发送十六进制数据 */
uint16_t uart2_recv_data(uint8_t *buf, uint16_t buf_size);           /* 从接收缓冲区取出数据 */
void     uart2_clear_rx_flag(void);                                  /* 清除接收完成标志 */

/* ==================== USART3 函数声明 ==================== */
/* 引脚: TX=PB10, RX=PB11  |  时钟: APB1 36MHz */

void     uart3_init(void);                                           /* USART3 初始化（9600-8-N-1，TX+中断RX） */
void     uart3_send_string(const char *str);                         /* 发送字符串 */
void     uart3_send_hex(const uint8_t *data, uint16_t len);          /* 发送十六进制数据 */
uint16_t     uart3_recv_data(uint8_t *buf, uint16_t buf_size);           /* 从接收缓冲区取出数据 */
void     uart3_clear_rx_flag(void);                                  /* 清除接收完成标志 */


uint8_t Serial_transmission_Selection(uart_port_t port, const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H */

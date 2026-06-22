#include "uart.h"
#include <string.h>

/* ==================== 全局变量 ==================== */
uart_dev_t g_uart1_dev; /* USART1 设备全局实例 */
uart_dev_t g_uart2_dev; /* USART2 设备全局实例 */
uart_dev_t g_uart3_dev; /* USART3 设备全局实例 */

/* ==================== 内部宏定义 ==================== */

/**
  * @brief  环形缓冲区索引自增宏
  */
#define RING_BUF_NEXT(idx, size)  (((idx) + 1u) % (size))

/* ==================== 内部函数声明 ==================== */
static void _uart_tx_write(USART_TypeDef *usart, uart_dev_t *dev, const uint8_t *data, uint16_t len);
static void _uart_isr_handler(USART_TypeDef *usart, uart_dev_t *dev);

/* ======================================================================== */
/*                           USART1 相关函数                                  */
/*                 引脚: TX=PA9, RX=PA10  |  APB2 72MHz                      */
/* ======================================================================== */

/**
  * @brief  USART1 初始化（9600-8-N-1，TX+中断RX）
  */
void uart1_init(void)
{
    GPIO_InitTypeDef  gpio_init;
    USART_InitTypeDef usart_init;
    NVIC_InitTypeDef  nvic_init;

    /* ---------- 1. 使能时钟 ---------- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    /* ---------- 2. 配置 GPIO ---------- */
    gpio_init.GPIO_Pin   = GPIO_Pin_9;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;           /* PA9  -- USART1_TX 复用推挽输出 */
    GPIO_Init(GPIOA, &gpio_init);

    gpio_init.GPIO_Pin   = GPIO_Pin_10;
    gpio_init.GPIO_Mode  = GPIO_Mode_IN_FLOATING;      /* PA10 -- USART1_RX 浮空输入 */
    GPIO_Init(GPIOA, &gpio_init);

    /* ---------- 3. 配置 USART ---------- */
    usart_init.USART_BaudRate            = UART1_BAUDRATE;
    usart_init.USART_WordLength          = USART_WordLength_8b;
    usart_init.USART_StopBits            = USART_StopBits_1;
    usart_init.USART_Parity              = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart_init);

    /* ---------- 4. 配置 NVIC 中断 ---------- */
    nvic_init.NVIC_IRQChannel                   = USART1_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init.NVIC_IRQChannelSubPriority        = 0;
    nvic_init.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic_init);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);    /* 接收非空中断 */
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);    /* 总线空闲中断 */

    /* ---------- 5. 使能 USART1 ---------- */
    USART_Cmd(USART1, ENABLE);
}

void uart1_send_string(const char *str)
{
    uint16_t len = (uint16_t)strlen(str);
    if (len > 0u) {
        _uart_tx_write(USART1, &g_uart1_dev, (const uint8_t *)str, len);
    }
}

void uart1_send_hex(const uint8_t *data, uint16_t len)
{
    if ((data != NULL) && (len > 0u)) {
        _uart_tx_write(USART1, &g_uart1_dev, data, len);
    }
}

uint16_t uart1_recv_data(uint8_t *buf, uint16_t buf_size)
{
    uint16_t count = 0u;

    if ((buf == NULL) || (buf_size == 0u)) {
        return 0u;
    }

    while ((g_uart1_dev.rx_head != g_uart1_dev.rx_tail) && (count < buf_size)) {
        buf[count] = g_uart1_dev.rx_buf[g_uart1_dev.rx_head];
        g_uart1_dev.rx_head = RING_BUF_NEXT(g_uart1_dev.rx_head, UART_RX_BUF_SIZE);
        count++;
    }

    return count;
}

void uart1_clear_rx_flag(void)
{
    g_uart1_dev.rx_complete  = 0u;
    g_uart1_dev.rx_frame_len = 0u;
}

/* ======================================================================== */
/*                           USART2 相关函数                                  */
/*                 引脚: TX=PA2, RX=PA3  |  APB1 36MHz                      */
/* ======================================================================== */

/**
  * @brief  USART2 初始化（9600-8-N-1，TX+中断RX）
  */
void uart2_init(void)
{
    GPIO_InitTypeDef  gpio_init;
    USART_InitTypeDef usart_init;
    NVIC_InitTypeDef  nvic_init;

    /* ---------- 1. 使能时钟 ---------- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);       /* GPIOA 挂 APB2 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);      /* USART2 挂 APB1 */

    /* ---------- 2. 配置 GPIO ---------- */
    gpio_init.GPIO_Pin   = GPIO_Pin_2;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;           /* PA2 -- USART2_TX 复用推挽输出 */
    GPIO_Init(GPIOA, &gpio_init);

    gpio_init.GPIO_Pin   = GPIO_Pin_3;
    gpio_init.GPIO_Mode  = GPIO_Mode_IN_FLOATING;      /* PA3 -- USART2_RX 浮空输入 */
    GPIO_Init(GPIOA, &gpio_init);

    /* ---------- 3. 配置 USART ---------- */
    usart_init.USART_BaudRate            = UART2_BAUDRATE;    // 波特率设置
    usart_init.USART_WordLength          = USART_WordLength_8b;
    usart_init.USART_StopBits            = USART_StopBits_1;
    usart_init.USART_Parity              = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &usart_init);

    /* ---------- 4. 配置 NVIC 中断 ---------- */
    nvic_init.NVIC_IRQChannel                   = USART2_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init.NVIC_IRQChannelSubPriority        = 0;
    nvic_init.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic_init);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);    /* 接收非空中断 */
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);    /* 总线空闲中断 */

    /* ---------- 5. 使能 USART2 ---------- */
    USART_Cmd(USART2, ENABLE);
}

void uart2_send_string(const char *str)
{
    uint16_t len = (uint16_t)strlen(str);
    if (len > 0u) {
        _uart_tx_write(USART2, &g_uart2_dev, (const uint8_t *)str, len);
    }
}

void uart2_send_hex(const uint8_t *data, uint16_t len)
{
    if ((data != NULL) && (len > 0u)) {
        _uart_tx_write(USART2, &g_uart2_dev, data, len);
    }
}

uint16_t uart2_recv_data(uint8_t *buf, uint16_t buf_size)
{
    uint16_t count = 0u;

    if ((buf == NULL) || (buf_size == 0u)) {
        return 0u;
    }

    while ((g_uart2_dev.rx_head != g_uart2_dev.rx_tail) && (count < buf_size)) {
        buf[count] = g_uart2_dev.rx_buf[g_uart2_dev.rx_head];
        g_uart2_dev.rx_head = RING_BUF_NEXT(g_uart2_dev.rx_head, UART_RX_BUF_SIZE);
        count++;
    }

    return count;
}

void uart2_clear_rx_flag(void)
{
    g_uart2_dev.rx_complete  = 0u;
    g_uart2_dev.rx_frame_len = 0u;
}

/* ======================================================================== */
/*                           USART3 相关函数                                  */
/*                 引脚: TX=PB10, RX=PB11  |  APB1 36MHz                    */
/* ======================================================================== */

/**
  * @brief  USART3 初始化（9600-8-N-1，TX+中断RX）
  */
void uart3_init(void)
{
    GPIO_InitTypeDef  gpio_init;
    USART_InitTypeDef usart_init;
    NVIC_InitTypeDef  nvic_init;

    /* ---------- 1. 使能时钟 ---------- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);       /* GPIOB 挂 APB2 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);      /* USART3 挂 APB1 */

    /* ---------- 2. 配置 GPIO ---------- */
    gpio_init.GPIO_Pin   = GPIO_Pin_10;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;           /* PB10 -- USART3_TX 复用推挽输出 */
    GPIO_Init(GPIOB, &gpio_init);

    gpio_init.GPIO_Pin   = GPIO_Pin_11;
    gpio_init.GPIO_Mode  = GPIO_Mode_IN_FLOATING;      /* PB11 -- USART3_RX 浮空输入 */
    GPIO_Init(GPIOB, &gpio_init);

    /* ---------- 3. 配置 USART ---------- */
    usart_init.USART_BaudRate            = UART3_BAUDRATE;
    usart_init.USART_WordLength          = USART_WordLength_8b;
    usart_init.USART_StopBits            = USART_StopBits_1;
    usart_init.USART_Parity              = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &usart_init);

    /* ---------- 4. 配置 NVIC 中断 ---------- */
    nvic_init.NVIC_IRQChannel                   = USART3_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init.NVIC_IRQChannelSubPriority        = 0;
    nvic_init.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic_init);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);    /* 接收非空中断 */
    USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);    /* 总线空闲中断 */

    /* ---------- 5. 使能 USART3 ---------- */
    USART_Cmd(USART3, ENABLE);
}

void uart3_send_string(const char *str)
{
    uint16_t len = (uint16_t)strlen(str);
    if (len > 0u) {
        _uart_tx_write(USART3, &g_uart3_dev, (const uint8_t *)str, len);
    }
}

void uart3_send_hex(const uint8_t *data, uint16_t len)
{
    if ((data != NULL) && (len > 0u)) {
        _uart_tx_write(USART3, &g_uart3_dev, data, len);
    }
}

uint16_t uart3_recv_data(uint8_t *buf, uint16_t buf_size)
{
    uint16_t count = 0u;

    if ((buf == NULL) || (buf_size == 0u)) {
        return 0u;
    }

    while ((g_uart3_dev.rx_head != g_uart3_dev.rx_tail) && (count < buf_size)) {
        buf[count] = g_uart3_dev.rx_buf[g_uart3_dev.rx_head];
        g_uart3_dev.rx_head = RING_BUF_NEXT(g_uart3_dev.rx_head, UART_RX_BUF_SIZE);
        count++;
    }

    return count;
}

void uart3_clear_rx_flag(void)
{
    g_uart3_dev.rx_complete  = 0u;
    g_uart3_dev.rx_frame_len = 0u;
}

/* ======================================================================== */
/*                           中断服务函数 (ISR)                               */
/* ======================================================================== */

/**
  * @brief  USART1 中断服务函数
  */
void USART1_IRQHandler(void)
{
    _uart_isr_handler(USART1, &g_uart1_dev);
}

/**
  * @brief  USART2 中断服务函数
  */
void USART2_IRQHandler(void)
{
    _uart_isr_handler(USART2, &g_uart2_dev);
}

/**
  * @brief  USART3 中断服务函数
  */
void USART3_IRQHandler(void)
{
    _uart_isr_handler(USART3, &g_uart3_dev);
}

/* ======================================================================== */
/*                        内部参数化函数（公有 ISR / 公有 API 共用）           */
/* ======================================================================== */

/**
  * @brief  向指定串口的发送缓冲区写入数据（内部函数）
  * @param  usart : USART 外设基地址 (USART1 / USART2 / USART3)
  * @param  dev   : 对应设备结构体指针
  * @param  data  : 数据指针
  * @param  len   : 数据长度
  */
static void _uart_tx_write(USART_TypeDef *usart, uart_dev_t *dev,
                           const uint8_t *data, uint16_t len)
{
    uint16_t i;

    for (i = 0u; i < len; i++) {
        uint16_t next_tail = RING_BUF_NEXT(dev->tx_tail, UART_TX_BUF_SIZE);

        /* 等待缓冲区有空位；若发送器空闲则先启动 TXE 中断排空 */
        while (next_tail == dev->tx_head) {
            if (dev->tx_busy == 0u) {
                dev->tx_busy = 1u;
                USART_ITConfig(usart, USART_IT_TXE, ENABLE);
            }
        }

        dev->tx_buf[dev->tx_tail] = data[i];
        dev->tx_tail = next_tail;
    }

    /* 如果当前不处于发送中，启动发送 */
    if (dev->tx_busy == 0u) {
        dev->tx_busy = 1u;
        USART_ITConfig(usart, USART_IT_TXE, ENABLE);
    }
}

/**
  * @brief  通用 UART 中断处理（内部函数）
  * @param  usart : USART 外设基地址
  * @param  dev   : 对应设备结构体指针
  * @note   处理 RXNE / IDLE / TXE 三类中断
  */
static void _uart_isr_handler(USART_TypeDef *usart, uart_dev_t *dev)
{
    /* --- 接收中断 RXNE --- */
    if (USART_GetITStatus(usart, USART_IT_RXNE) != RESET) {
        uint8_t  rx_byte;
        uint16_t next_tail;

        rx_byte   = (uint8_t)(usart->DR & 0xFFu);
        next_tail = RING_BUF_NEXT(dev->rx_tail, UART_RX_BUF_SIZE);

        /* 缓冲区未满时才写入 */
        if (next_tail != dev->rx_head) {
            dev->rx_buf[dev->rx_tail] = rx_byte;
            dev->rx_tail = next_tail;
        }
    }

    /* --- 空闲中断 IDLE --- */
    if (USART_GetITStatus(usart, USART_IT_IDLE) != RESET) {
        (void)(usart->SR);   /* 读 SR 再读 DR 清除 IDLE 标志 */
        (void)(usart->DR);

        /* 计算本帧接收长度 */
        if (dev->rx_tail >= dev->rx_head) {
            dev->rx_frame_len = dev->rx_tail - dev->rx_head;
        } else {
            dev->rx_frame_len = UART_RX_BUF_SIZE - dev->rx_head + dev->rx_tail;
        }

        dev->rx_complete = 1u;
    }

    /* --- 发送中断 TXE --- */
    if (USART_GetITStatus(usart, USART_IT_TXE) != RESET) {
        if (dev->tx_head != dev->tx_tail) {
            usart->DR = dev->tx_buf[dev->tx_head];
            dev->tx_head = RING_BUF_NEXT(dev->tx_head, UART_TX_BUF_SIZE);
        } else {
            USART_ITConfig(usart, USART_IT_TXE, DISABLE);
            dev->tx_busy = 0u;
        }
    }
}

/**
  * @brief  串口选择发送函数（统一发送接口）
  * @param  port : 串口选择，取值 uart1 / uart2 / uart3
  * @param  data : 待发送数据指针
  * @param  len  : 数据长度（字节）
  * @retval 0: 发送成功  1: 参数错误（端口非法或数据为空）
  */
uint8_t Serial_transmission_Selection(uart_port_t port, const uint8_t *data, uint16_t len){
    if ((data == NULL) || (len == 0u)) {
        return 1u;                                   /* 数据指针为空或长度为零，返回错误 */
    }

    switch (port) {
        case uart1:
            _uart_tx_write(USART1, &g_uart1_dev, data, len);
            break;
        case uart2:
            _uart_tx_write(USART2, &g_uart2_dev, data, len);
            break;
        case uart3:
            _uart_tx_write(USART3, &g_uart3_dev, data, len);
            break;
        default:
            return 1u;                               /* 无效端口号，返回错误 */
    }

    return 0u;                                       /* 发送成功 */
}


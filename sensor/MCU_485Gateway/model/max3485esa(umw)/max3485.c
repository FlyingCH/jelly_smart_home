#include "max3485.h"
#include "delay.h"
#include "uart.h"


/**
  * @brief  MAX3485 控制引脚初始化
  * @note   PA11 配置为通用推挽输出 (GPIO_Mode_Out_PP)，默认拉低（接收模式）
  *         MAX3485 的 DE 和 RE 在硬件上短接，由同一 GPIO 控制方向
  */
void max3485_init(void)
{
    GPIO_InitTypeDef gpio_init;

    /* ---------- 1. 使能 GPIOA 时钟 ---------- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* ---------- 2. 配置 PA11 为通用推挽输出 ---------- */
    gpio_init.GPIO_Pin   = MAX3485_CTRL_PIN;       /* PA11 */
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;        /* 50MHz 驱动速度 */
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;        /* 通用推挽输出 */
    GPIO_Init(MAX3485_CTRL_PORT, &gpio_init);

    /* ---------- 3. 默认拉低，进入接收模式 ---------- */
    GPIO_ResetBits(MAX3485_CTRL_PORT, MAX3485_CTRL_PIN); /* PA11 = 0, 接收模式 */
}

void max3485_U3_init(void)
{
    GPIO_InitTypeDef gpio_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio_init.GPIO_Pin   = MAX3485_U3_CTRL_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(MAX3485_U3_CTRL_PORT, &gpio_init);

    GPIO_ResetBits(MAX3485_U3_CTRL_PORT, MAX3485_U3_CTRL_PIN);
}







/**
  * @brief  设置 MAX3485 收发模式
  * @param  mode : MAX3485_TX_MODE(1u) 发送模式 / MAX3485_RX_MODE(0u) 接收模式
  * @note   发送前需置为 TX_MODE，发送完毕后及时切回 RX_MODE
  */
void max3485_set_mode(uint8_t mode)
{
    if (mode == MAX3485_TX_MODE) {
        GPIO_SetBits(MAX3485_CTRL_PORT, MAX3485_CTRL_PIN);    /* PA11 拉高 → 发送模式 */
    } else {
        GPIO_ResetBits(MAX3485_CTRL_PORT, MAX3485_CTRL_PIN);  /* PA11 拉低 → 接收模式 */
    }
}

// 串口3的发送
void max3485_U3_set_mode(uint8_t mode)
{
    if (mode == MAX3485_TX_MODE) {
        GPIO_SetBits(MAX3485_U3_CTRL_PORT, MAX3485_U3_CTRL_PIN);    /* PA11 拉高 → 发送模式 */
    } else {
        GPIO_ResetBits(MAX3485_U3_CTRL_PORT, MAX3485_U3_CTRL_PIN);  /* PA11 拉低 → 接收模式 */
    }
}




/**
  * @brief  MAX3485 RS-485 发送函数
  * @param  data : 待发送数据指针
  * @param  len  : 数据长度（字节）
  * @note   自动控制 PA11 方向：
  *         1. 拉高 PA11 → 发送模式
  *         2. 通过 USART1 发出全部数据
  *         3. 等待 USART1 TC(发送完成) 标志，确保移位寄存器完全移出（带超时保护）
  *         4. 拉低 PA11 → 接收模式
  *         此函数为阻塞调用，发送完毕后才返回
  * @retval 0 : 发送成功
  * @retval 1 : 参数无效
  * @retval 2 : TC 等待超时
  */
uint8_t max3485_send_data(const uint8_t *data, uint16_t len){
    uint32_t tx_timeout;
    uint32_t tc_timeout;

    if ((data == (const uint8_t *)0) || (len == 0u)) {
        return 1u;
    }

    max3485_set_mode(MAX3485_TX_MODE);
    Serial_transmission_Selection(uart1, data, len);

    /* 等待软件发送缓冲区完全送入 USART 数据寄存器 */
    tx_timeout = 0u;
    while (g_uart1_dev.tx_busy != 0u) {
        if (tx_timeout >= 1000000u) {
            max3485_set_mode(MAX3485_RX_MODE);
            return 2u;
        }
        tx_timeout++;
    }

    /* 再等待最后一个字节从移位寄存器真正发完 */
    tc_timeout = 0u;
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {
        if (tc_timeout >= 100000u) {
            max3485_set_mode(MAX3485_RX_MODE);
            return 2u;
        }
        tc_timeout++;
    }

    max3485_set_mode(MAX3485_RX_MODE);
    return 0u;
}

uint8_t max3485_U3_send_data(const uint8_t *data, uint16_t len){
    uint32_t tx_timeout;
    uint32_t tc_timeout;

    if ((data == (const uint8_t *)0) || (len == 0u)) {
        return 1u;
    }

    max3485_U3_set_mode(MAX3485_TX_MODE);
    Serial_transmission_Selection(uart3, data, len);

    /* 等待软件发送缓冲区完全送入 USART 数据寄存器 */
    tx_timeout = 0u;
    while (g_uart3_dev.tx_busy != 0u) {
        if (tx_timeout >= 1000000u) {
            max3485_U3_set_mode(MAX3485_RX_MODE);
            return 2u;
        }
        tx_timeout++;
    }

    /* 再等待最后一个字节从移位寄存器真正发完 */
    tc_timeout = 0u;
    while (USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET) {
        if (tc_timeout >= 100000u) {
            max3485_U3_set_mode(MAX3485_RX_MODE);
            return 2u;
        }
        tc_timeout++;
    }

    max3485_U3_set_mode(MAX3485_RX_MODE);
    return 0u;
}










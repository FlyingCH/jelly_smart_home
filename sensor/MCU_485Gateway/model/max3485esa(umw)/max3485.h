#ifndef __MAX3485_H
#define __MAX3485_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

/* ==================== 宏定义 ==================== */

/**
  * @brief  MAX3485 DE/RE 控制引脚定义控制UART1的发送引脚
  *         PA11 连接 MAX3485 的 DE(驱动器使能) 和 RE(接收器使能) 引脚
  *         DE 与 RE 在硬件上短接，由同一 GPIO 控制：
  *         高电平 → 发送模式（DE=1, RE=1，驱动器使能，接收器禁用）
  *         低电平 → 接收模式（DE=0, RE=0，驱动器禁用，接收器使能）
  */
#define MAX3485_CTRL_PORT      GPIOA               /* 控制端口 */
#define MAX3485_CTRL_PIN       GPIO_Pin_11         /* 控制引脚: PA11 */



// 控制UART3的引脚
#define MAX3485_U3_CTRL_PORT      GPIOB               /* 控制端口 */
#define MAX3485_U3_CTRL_PIN       GPIO_Pin_5         /* 控制引脚: PB5 */




#define MAX3485_TX_MODE        1u                  /* 发送模式: 引脚拉高 */
#define MAX3485_RX_MODE        0u                  /* 接收模式: 引脚拉低 */

/* ==================== 函数声明 ==================== */

void max3485_init(void);                                             /* 初始化 PA11 为通用推挽输出，默认接收模式 */
void max3485_U3_init(void);
void max3485_set_mode(uint8_t mode);                                 /* 设置收发模式: MAX3485_TX_MODE / MAX3485_RX_MODE */
void max3485_U3_set_mode(uint8_t mode);
uint8_t max3485_send_data(const uint8_t *data, uint16_t len);           /* RS-485 发送数据：自动切换收发方向 + 等待 TC 完成 */
uint8_t max3485_U3_send_data(const uint8_t *data, uint16_t len);
#ifdef __cplusplus
}
#endif

#endif /* __MAX3485_H */

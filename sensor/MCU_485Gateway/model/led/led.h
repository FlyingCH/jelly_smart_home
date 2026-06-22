#ifndef __LED_H
#define __LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

/* ==================== 引脚宏定义 ==================== */

/**
  * @brief  LED 引脚定义
  *         PA0 -- STATE LED（状态指示灯）
  *         PA1 -- DATA LED （数据通信指示灯）
  *         引脚为通用推挽输出，高电平点亮，低电平熄灭
  */
#define LED_PORT              GPIOA               /* LED 控制端口 */
#define LED_STATE_PIN         GPIO_Pin_0          /* STATE LED: PA0 */
#define LED_DATA_PIN          GPIO_Pin_1          /* DATA LED:  PA1 */

/* ==================== LED 选择枚举 ==================== */

typedef enum {
    LED_STATE = 0,                               /* 状态指示灯 PA0 */
    LED_DATA  = 1                                /* 数据指示灯 PA1 */
} led_id_t;

/* ==================== LED 状态宏 ==================== */
#define LED_ON                1u                  /* 点亮：引脚拉高 */
#define LED_OFF               0u                  /* 熄灭：引脚拉低 */

/* ==================== 函数声明 ==================== */

void led_init(void);                                             /* 初始化 PA0/PA1 为通用推挽输出，默认全灭 */
void led_set(led_id_t id, uint8_t state);                        /* 控制指定 LED: LED_ON / LED_OFF */
void led_toggle(led_id_t id);                                    /* 翻转指定 LED 的电平 */

#ifdef __cplusplus
}
#endif

#endif /* __LED_H */

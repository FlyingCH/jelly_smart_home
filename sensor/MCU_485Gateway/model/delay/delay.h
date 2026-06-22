#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

void     delay_init(void);              /* 初始化延时模块：SysTick 1ms 中断 + DWT 周期计数器 */
void     delay_us(uint32_t us);         /* 微秒级阻塞延时 */
void     delay_ms(uint32_t ms);         /* 毫秒级阻塞延时 */
void     delay_s(uint32_t s);           /* 秒级阻塞延时 */

uint32_t get_tick_ms(void);             /* 获取系统毫秒计数器（由 SysTick_Handler 每 1ms 递增） */

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */

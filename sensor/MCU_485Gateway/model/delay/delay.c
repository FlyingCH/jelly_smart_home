#include "delay.h"

/* ==================== DWT 寄存器地址定义 ==================== */
/* Cortex-M3 调试架构中 DWT (Data Watchpoint and Trace) 的固定地址映射
   旧版 CMSIS (start/core_cm3.h) 未定义 DWT 结构体，此处直接使用地址操作 */
#define DWT_CTRL_REG        (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG      (*(volatile uint32_t *)0xE0001004)
#define DWT_CTRL_CYCCNTENA  (1UL << 0)

/* CoreDebug DEMCR 寄存器地址 */
#define CoreDebug_DEMCR_REG (*(volatile uint32_t *)0xE000EDFC)
#define DEMCR_TRCENA        (1UL << 24)

/* ==================== 内部变量 ==================== */
static volatile uint32_t s_ms_tick;                 /* SysTick 毫秒累加计数器，由 SysTick_Handler 每 1ms 递增 */

/**
  * @brief  SysTick 中断服务函数
  * @note   每 1ms 触发一次，累加 s_ms_tick 供 delay_ms / delay_s 使用
  */
void SysTick_Handler(void)
{
    s_ms_tick++;
}

/**
  * @brief  延时模块初始化
  * @note   1. 配置 SysTick 为 1ms 中断 (SystemCoreClock/1000 = 72000)
  *         2. 使能 DWT 周期计数器，用于 us 级延时
  *         Hardware: HSE = 8MHz, PLL ×9 = SYSCLK 72MHz
  */
void delay_init(void)
{
    if (SysTick_Config(SystemCoreClock / 1000u)) {
        while (1);                                  /* SysTick 配置失败，死循环等待（正常不应到达） */
    }

    CoreDebug_DEMCR_REG |= DEMCR_TRCENA;            /* 使能 DWT 调试跟踪 */
    DWT_CTRL_REG |= DWT_CTRL_CYCCNTENA;             /* 使能 DWT 周期计数 CYCCNT */
    DWT_CYCCNT_REG = 0u;                            /* CYCCNT 从 0 开始计数 */
}

/**
  * @brief  微秒级阻塞延时
  * @param  us : 延时微秒数 (0 ~ 59,652,323 us，约 59.6 秒)
  * @note   使用 DWT 的 CYCCNT 周期计数器实现
  *         CPU = 72MHz → 1 us = 72 个时钟周期
  *         超过约 59.6 秒的延时请使用 delay_ms() 或 delay_s()
  */
void delay_us(uint32_t us)
{
    uint32_t start_val = DWT_CYCCNT_REG;            /* 记录起始计数值 */
    uint32_t ticks     = us * (SystemCoreClock / 1000000u); /* 计算所需时钟周期数 */
                                                     /* 72MHz 下：ticks = us × 72 */
    while ((DWT_CYCCNT_REG - start_val) < ticks);   /* 等待 CYCCNT 差值达到目标 */
                                                     /* 32bit 减法自动处理回绕 */
}

/**
  * @brief  毫秒级阻塞延时
  * @param  ms : 延时毫秒数 (最大值 4,294,967,295 ms，约 49.7 天)
  * @note   基于 SysTick 1ms 中断的 s_ms_tick 累加器实现
  */
void delay_ms(uint32_t ms)
{
    uint32_t start = s_ms_tick;                     /* 记录起始 tick */
    while ((s_ms_tick - start) < ms);               /* 等待差值达到目标 ms */
                                                     /* s_ms_tick 每 1ms 递增一次 */
}

/**
  * @brief  秒级阻塞延时
  * @param  s : 延时秒数
  * @note   由 delay_ms() 循环累加实现
  */
void delay_s(uint32_t s)
{
    uint32_t i;
    for (i = 0u; i < s; i++) {
        delay_ms(1000u);                            /* 每次延时 1 秒 */
    }
}

/**
  * @brief  获取系统毫秒计数器
  * @retval 当前毫秒计数值（由 SysTick_Handler 每 1ms 递增）
  * @note   32 位无符号数，约 49.7 天溢出回绕，减法运算天然兼容回绕
  */
uint32_t get_tick_ms(void)
{
    return s_ms_tick;
}

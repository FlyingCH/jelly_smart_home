#include "led.h"

/* 引脚查找表：将 led_id_t 枚举映射到 GPIO_Pin */
static const uint16_t s_led_pin_table[] = {
    LED_STATE_PIN,                               /* LED_STATE → PA0 */
    LED_DATA_PIN                                 /* LED_DATA  → PA1 */
};

/**
  * @brief  LED 初始化
  * @note   PA0 配置为 STATE LED，PA1 配置为 DATA LED
  *         均为通用推挽输出，默认熄灭
  */
void led_init(void)
{
    GPIO_InitTypeDef gpio_init;

    /* ---------- 1. 使能 GPIOA 时钟 ---------- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* ---------- 2. 配置 PA0(STATUS) 和 PA1(DATA) 为通用推挽输出 ---------- */
    gpio_init.GPIO_Pin   = LED_STATE_PIN | LED_DATA_PIN; /* PA0 | PA1 */
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;              /* 50MHz 驱动速度 */
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;              /* 通用推挽输出 */
    GPIO_Init(LED_PORT, &gpio_init);

    /* ---------- 3. 默认全灭 ---------- */
    GPIO_ResetBits(LED_PORT, LED_STATE_PIN | LED_DATA_PIN); /* PA0=0, PA1=0 */
}

/**
  * @brief  控制指定 LED 的亮灭
  * @param  id    : LED 选择: LED_STATE(PA0) / LED_DATA(PA1)
  * @param  state : 目标状态: LED_ON(1u) 点亮 / LED_OFF(0u) 熄灭
  */
void led_set(led_id_t id, uint8_t state)
{
    uint16_t pin;

    if ((id != LED_STATE) && (id != LED_DATA)) {
        return;                                  /* 无效 LED ID，直接返回 */
    }

    pin = s_led_pin_table[id];                   /* 通过查找表获取对应的引脚 */

    if (state == LED_ON) {
        GPIO_SetBits(LED_PORT, pin);             /* 引脚拉高 → LED 点亮 */
    } else {
        GPIO_ResetBits(LED_PORT, pin);           /* 引脚拉低 → LED 熄灭 */
    }
}

/**
  * @brief  翻转指定 LED 的电平
  * @param  id : LED 选择: LED_STATE(PA0) / LED_DATA(PA1)
  * @note   读取当前输出电平，取反后写回
  */
void led_toggle(led_id_t id)
{
    uint16_t pin;

    if ((id != LED_STATE) && (id != LED_DATA)) {
        return;                                  /* 无效 LED ID，直接返回 */
    }

    pin = s_led_pin_table[id];                   /* 通过查找表获取对应的引脚 */

    /* 读取当前输出电平，取反写回 */
    if (GPIO_ReadOutputDataBit(LED_PORT, pin) == Bit_RESET) {
        GPIO_SetBits(LED_PORT, pin);              /* 当前低 → 拉高 */
    } else {
        GPIO_ResetBits(LED_PORT, pin);            /* 当前高 → 拉低 */
    }
}

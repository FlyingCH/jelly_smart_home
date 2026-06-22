#include "Net_USRK5.h"
#include "uart.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include "mqtt.h"

/* ==================== 内部宏定义 ==================== */

#define USRK5_AT_RETRY_MAX      3u               /* AT 指令最大重试次数 */
#define USRK5_AT_TIMEOUT_DEF    2000u             /* AT 指令默认超时 ms */
#define USRK5_RST_HOLD_MS       300u              /* RST 低电平保持时间 ms */
#define USRK5_RELOAD_HOLD_MS    3000u             /* Reload 低电平保持时间 ms */
#define USRK5_BOOT_WAIT_MS      2000u             /* 上电/复位后等待模块启动 ms */

/* ==================== 内部函数声明 ==================== */
static uint8_t _usrk5_wait_response(const char *expect, uint32_t timeout_ms);
static uint8_t _usrk5_read_until(char *buf, uint16_t buf_size, uint32_t timeout_ms);
static char _usrk5_to_lower(char c);
static const char *_usrk5_stristr(const char *haystack, const char *needle);
static uint8_t _usrk5_check_ok(const char *buf);

/* ==================== 初始化与硬件控制 ==================== */

/**
  * @brief  USR-K5 模块初始化（底层 GPIO + USART2 硬件初始化）
  * @note   1. 配置 PA5(Reload)、PA6(RST) 为推挽输出，默认拉高
  *         2. 硬件复位模块，等待启动完成
  *         3. 注意: 模块工厂默认波特率 = 115200，配置成功后切换到目标波特率
  */
void usrk5_init(void)
{
    GPIO_InitTypeDef gpio_init;

    /* ---------- 1. 使能 GPIOA 时钟 ---------- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* ---------- 2. 配置 PA5(Reload) 和 PA6(RST) 为推挽输出 ---------- */
    gpio_init.GPIO_Pin   = USRK5_RELOAD_PIN | USRK5_RST_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(USRK5_CTRL_PORT, &gpio_init);

    GPIO_SetBits(USRK5_CTRL_PORT, USRK5_RELOAD_PIN | USRK5_RST_PIN);

    /* ---------- 3. 硬件复位模块 ---------- */
    usrk5_hw_reset();
}

/**
  * @brief  初始化 USART2 通信接口
  * @param  baud : 目标波特率
  * @note   内部直接操作 USART2 外设寄存器，不依赖 uart.h 的固定 9600 配置
  *         用于 AT 会话时的 115200 与工作模式下的目标波特率之间切换
  */
static void _usrk5_uart_init(uint32_t baud)
{
    GPIO_InitTypeDef  gpio_init;
    USART_InitTypeDef usart_init;
    NVIC_InitTypeDef  nvic_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    gpio_init.GPIO_Pin   = GPIO_Pin_2;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode  = GPIO_Mode_AF_PP;           /* PA2 -- USART2_TX */
    GPIO_Init(GPIOA, &gpio_init);

    gpio_init.GPIO_Pin   = GPIO_Pin_3;
    gpio_init.GPIO_Mode  = GPIO_Mode_IN_FLOATING;      /* PA3 -- USART2_RX */
    GPIO_Init(GPIOA, &gpio_init);

    usart_init.USART_BaudRate            = baud;
    usart_init.USART_WordLength          = USART_WordLength_8b;
    usart_init.USART_StopBits            = USART_StopBits_1;
    usart_init.USART_Parity              = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &usart_init);

    nvic_init.NVIC_IRQChannel                   = USART2_IRQn;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_init.NVIC_IRQChannelSubPriority        = 0;
    nvic_init.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic_init);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);    /* 接收中断 */
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);    /* 空闲中断 */

    USART_Cmd(USART2, ENABLE);
}

/**
  * @brief  硬件复位 USR-K5 模块
  * @note   PA6(RST) 拉低 300ms → 拉高 → 等待 2s 模块启动
  */
void usrk5_hw_reset(void)
{
    GPIO_ResetBits(USRK5_CTRL_PORT, USRK5_RST_PIN);  /* RST = 0 */
    delay_ms(USRK5_RST_HOLD_MS);                     /* 保持 300ms */
    GPIO_SetBits(USRK5_CTRL_PORT, USRK5_RST_PIN);    /* RST = 1 */
    delay_ms(USRK5_BOOT_WAIT_MS);                    /* 等待模块启动 */
}

/**
  * @brief  恢复 USR-K5 出厂设置
  * @note   PA5(Reload) 拉低 3s → 拉高 → 等待 2s 模块重启
  */
void usrk5_reload_default(void)
{
    GPIO_ResetBits(USRK5_CTRL_PORT, USRK5_RELOAD_PIN); /* Reload = 0 */
    delay_ms(USRK5_RELOAD_HOLD_MS);                     /* 保持 3s */
    GPIO_SetBits(USRK5_CTRL_PORT, USRK5_RELOAD_PIN);    /* Reload = 1 */
    delay_ms(USRK5_BOOT_WAIT_MS);                       /* 等待模块重启 */
}

/* ==================== AT 指令模式进出 ==================== */

/**
  * @brief  进入 AT 指令模式（透传 → 临时指令模式）
  * @retval 0: 成功  1: 失败
  * @note   严格按照 USR-K5 时序:
  *          1. T1: 串口静默 > 打包间隔，确保模块识别数据断流
  *          2. T2 < 300ms: 连续发送 "+++"，字符间隔需在 300ms 内
  *          3. T3 < 300ms: 模块收到 "+++" 后回复 "a"
  *          4. T5 < 3s:   MCU 收到 "a" 后回复 "a"
  *          5. 模块回复 "+OK" → 进入 AT 指令模式
  */
uint8_t usrk5_enter_at_mode(void)
{
    uint8_t retry;

    for (retry = 0u; retry < USRK5_AT_RETRY_MAX; retry++) {
        /* 清除接收缓冲区中可能的残留数据 */
        uart2_clear_rx_flag();
        g_uart2_dev.rx_head = g_uart2_dev.rx_tail;

        /* ---- T1: 串口静默，等待发送完成 + 打包间隔 ---- */
        while (USART_GetFlagStatus(USRK5_USART, USART_FLAG_TC) == RESET);
        delay_ms(500);

        /* ---- T2: 连续发送 "+++"（字符间隔由 TXE 中断保证 << 300ms） ---- */
        uart2_send_string("+++");
        while (USART_GetFlagStatus(USRK5_USART, USART_FLAG_TC) == RESET);

        /* ---- T3: 等待模块在 300ms 内回复 "a" ---- */
        if (_usrk5_wait_response("a", 300u) != 0u) {
            continue;
        }

        /* ---- T5: 收到 "a" 后 3s 内回复 "a" ---- */
        uart2_send_string("a");
        while (USART_GetFlagStatus(USRK5_USART, USART_FLAG_TC) == RESET);

        /* ---- 等待模块回复 "+OK" ---- */
        if (_usrk5_wait_response("+ok", 3000u) == 0u) {
            return 0u;
        }
    }

    return 1u;
}

/**
  * @brief  退出 AT 指令模式，返回透传工作模式
  * @retval 0: 成功  1: 失败
  */
uint8_t usrk5_exit_at_mode(void)
{
    return usrk5_send_at_cmd("AT+ENTM", (char *)0, 0u, USRK5_AT_TIMEOUT_DEF);
}

/* ==================== AT 指令基础接口 ==================== */

/**
  * @brief  发送 AT 指令并等待应答
  * @param  cmd       : AT 指令字符串(不含结尾 CRLF，本函数自动追加)
  * @param  resp      : 响应缓冲区指针(NULL = 不保存响应)
  * @param  resp_size : 响应缓冲区大小
  * @param  timeout_ms: 等待应答超时 ms
  * @retval 0: 成功  1: 超时或失败
  */
uint8_t usrk5_send_at_cmd(const char *cmd, char *resp, uint16_t resp_size, uint32_t timeout_ms)
{
    if ((cmd == (const char *)0) || (cmd[0] == '\0')) {
        return 1u;
    }

    /* 清除接收缓冲区 */
    uart2_clear_rx_flag();
    g_uart2_dev.rx_head = g_uart2_dev.rx_tail;       /* 丢弃环形缓冲中残留数据 */

    /* 发送 AT 指令 + CRLF */
    uart2_send_string(cmd);
    uart2_send_string("\r\n");

    /* 等待 TC 完成 */
    while (USART_GetFlagStatus(USRK5_USART, USART_FLAG_TC) == RESET);

    /* 等待并读取响应 */
    if (resp != (char *)0 && resp_size > 0u) {
        return _usrk5_read_until(resp, resp_size, timeout_ms);
    }

    return 0u;
}

/* ==================== 网络配置函数 ==================== */

/**
  * @brief  设置网络工作模式
  * @param  mode : USRK5_MODE_TCP_CLIENT / TCP_SERVER / UDP_CLIENT / UDP_SERVER
  * @retval 0: 成功  1: 失败
  */
uint8_t usrk5_set_net_mode(usrk5_net_mode_t mode)
{
    char cmd_buf[32];

    switch (mode) {
        case USRK5_MODE_TCP_CLIENT:
            (void)snprintf(cmd_buf, sizeof(cmd_buf), "AT+NETP=TCP,CLIENT,0");
            break;
        case USRK5_MODE_TCP_SERVER:
            (void)snprintf(cmd_buf, sizeof(cmd_buf), "AT+NETP=TCP,SERVER,0");
            break;
        case USRK5_MODE_UDP_CLIENT:
            (void)snprintf(cmd_buf, sizeof(cmd_buf), "AT+NETP=UDP,CLIENT,0");
            break;
        case USRK5_MODE_UDP_SERVER:
            (void)snprintf(cmd_buf, sizeof(cmd_buf), "AT+NETP=UDP,SERVER,0");
            break;
        default:
            return 1u;
    }

    return usrk5_send_at_cmd(cmd_buf, (char *)0, 0u, USRK5_AT_TIMEOUT_DEF);
}

/**
  * @brief  设置远端 IP 和端口
  * @param  ip   : 远端 IP 地址字符串，如 "192.168.1.100"
  * @param  port : 远端端口号，如 8234
  * @retval 0: 成功  1: 失败
  */
uint8_t usrk5_set_remote_ip(const char *ip, uint16_t port)
{
    char cmd_buf[64];

    if (ip == (const char *)0) {
        return 1u;
    }

    (void)snprintf(cmd_buf, sizeof(cmd_buf), "AT+TCPLK=%s,%u", ip, (unsigned int)port);
    return usrk5_send_at_cmd(cmd_buf, (char *)0, 0u, USRK5_AT_TIMEOUT_DEF);
}

/**
  * @brief  设置本地监听端口（预留接口）
  * @param  port : 本地端口号
  * @retval 0: 成功  1: 失败
  * @note   TCP Client 模式下本地端口由模块自动分配，一般无需手动设置
  *         如需在 Server 模式下监听指定端口，可启用此函数
  */
uint8_t usrk5_set_local_port(uint16_t port)
{
    char cmd_buf[32];

    (void)snprintf(cmd_buf, sizeof(cmd_buf), "AT+LANN=%u", (unsigned int)port);
    return usrk5_send_at_cmd(cmd_buf, (char *)0, 0u, USRK5_AT_TIMEOUT_DEF);
}

/**
  * @brief  设置模块 UART 参数
  * @param  baud      : 波特率
  * @param  data_bits : 数据位
  * @param  stop_bits : 停止位
  * @param  parity    : 校验位: 0=无, 1=奇, 2=偶
  * @retval 0: 成功  1: 失败
  * @note   修改后需重启模块才能生效
  */
uint8_t usrk5_set_uart(uint32_t baud, uint8_t data_bits, uint8_t stop_bits, uint8_t parity)
{
    char cmd_buf[48];

    (void)snprintf(cmd_buf, sizeof(cmd_buf), "AT+UART=%lu,%u,%u,%u",
                   (unsigned long)baud, (unsigned int)data_bits,
                   (unsigned int)stop_bits, (unsigned int)parity);
    return usrk5_send_at_cmd(cmd_buf, (char *)0, 0u, USRK5_AT_TIMEOUT_DEF);
}

/**
  * @brief  设置模块静态 IP
  * @param  ip      : IP 地址字符串，如 "192.168.1.200"
  * @param  mask    : 子网掩码字符串，如 "255.255.255.0"
  * @param  gateway : 网关地址字符串，如 "192.168.1.1"
  * @retval 0: 成功  1: 失败
  * @note   修改后需重启模块才能生效
  */
uint8_t usrk5_set_static_ip(const char *ip, const char *mask, const char *gateway)
{
    char cmd_buf[64];

    if ((ip == (const char *)0) || (mask == (const char *)0) || (gateway == (const char *)0)) {
        return 1u;
    }

    (void)snprintf(cmd_buf, sizeof(cmd_buf), "AT+WANN=STATIC,%s,%s,%s", ip, mask, gateway);
    return usrk5_send_at_cmd(cmd_buf, (char *)0, 0u, USRK5_AT_TIMEOUT_DEF);
}

/**
  * @brief  设置模块 DHCP 动态获取 IP
  * @retval 0: 成功  1: 失败
  * @note   修改后需重启模块才能生效
  */
uint8_t usrk5_set_dhcp(void)
{
    return usrk5_send_at_cmd("AT+WANN=DHCP,0,0,0", (char *)0, 0u, USRK5_AT_TIMEOUT_DEF);
}

/**
  * @brief  查询模块 IP 地址
  * @param  ip_buf   : 输出 IP 字符串缓冲区
  * @param  buf_size : 缓冲区大小
  * @retval 0: 成功  1: 失败
  */
uint8_t usrk5_query_ip(char *ip_buf, uint16_t buf_size)
{
    return usrk5_send_at_cmd("AT+WANN", ip_buf, buf_size, USRK5_AT_TIMEOUT_DEF);
}

/* ==================== 网络数据收发 ==================== */

/**
  * @brief  通过 USR-K5 网络发送数据
  * @param  data : 待发送数据
  * @param  len  : 数据长度
  * @retval 0: 成功  1: 失败
  * @note   在 TCP Client 透传模式下，直接通过 USART2 发送即可
  *         模块会自动将串口数据打包发送到远端服务器
  */
uint8_t usrk5_net_send(const uint8_t *data, uint16_t len)
{
    if ((data == (const uint8_t *)0) || (len == 0u)) {
        return 1u;
    }

    uart2_send_hex(data, len);                   /* 通过 USART2 发给模块 */
    return 0u;
}

/**
  * @brief  检查模块网络连接状态
  * @retval 0: 未连接  1: 已连接
  * @note   必须进入 AT 模式查询，查询完毕返回透传模式
  *         调用此函数会短暂中断数据透传（约 3~5 秒），请勿频繁调用
  */
uint8_t usrk5_net_is_connected(void)
{
    char buf[32];
    uint8_t connected = 0u;

    if (usrk5_enter_at_mode() == 0) {
        if (usrk5_send_at_cmd("AT+TCPDIS", buf, sizeof(buf), USRK5_AT_TIMEOUT_DEF) == 0u) {
            if (strstr(buf, ".") != (char *)0) {
                connected = 1u;                  /* 返回含 IP 地址 → 已连接 */
            }
        }
        (void)usrk5_exit_at_mode();             /* 无论查询结果如何，退出 AT 返回透传 */
    }

    return connected;
}

/**
  * @brief  检查模块与服务器的 TCP 连接状态
  * @retval 0 : 已连接
  * @retval 1 : 未连接或查询失败（进入 AT 模式失败 / 超时 / 无有效响应）
  * @note   进入 AT 模式 → AT+SOCK 查询 socket 状态 → 退出 AT 模式 → 返回结果
  *         调用会短暂中断透传（约 3~5 秒），不宜在透传发送循环中频繁调用
  *         判断依据：
  *           - 响应中含 "CONNECT" 或 "ESTABLISH" → 已连接
  *           - 响应中含 "."（IP 格式地址）         → 已连接
  */
uint8_t usrk5_net_check_connect(void)
{
    char buf[64];
    uint8_t ret = 1u;
    if (usrk5_enter_at_mode() != 0u) {
        return 1u;
    }
    if (usrk5_send_at_cmd("AT+SOCK", buf, sizeof(buf), 2000u) == 0u) {
        if (_usrk5_stristr(buf, "CONNECT") != (const char *)0
            || _usrk5_stristr(buf, "ESTABLISH") != (const char *)0
            || strstr(buf, ".") != (char *)0) {
            ret = 0u;
        }
    }
    (void)usrk5_exit_at_mode();
    return ret;
}

/* ==================== 内部函数 ==================== */

/**
  * @brief  等待接收缓冲区中出现期望字符串
  * @param  expect     : 期望的响应字符串
  * @param  timeout_ms : 超时时间 ms
  * @retval 0: 匹配成功  1: 超时
  */
static uint8_t _usrk5_wait_response(const char *expect, uint32_t timeout_ms)
{
    char    buf[128];
    uint8_t found = 0u;
    uint32_t waited = 0u;

    while (waited < timeout_ms) {
        if (g_uart2_dev.rx_complete) {
            uint8_t raw[128];
            uint16_t raw_len = uart2_recv_data(raw, sizeof(raw));
            uart2_clear_rx_flag();

            if (raw_len > 0u && raw_len < sizeof(buf)) {
                uint16_t i;
                for (i = 0u; i < raw_len; i++) {
                    buf[i] = (char)raw[i];
                }
                buf[raw_len] = '\0';

                if (_usrk5_stristr(buf, expect) != (const char *)0) {
                    found = 1u;
                    break;
                }
            }
        }

        delay_ms(10);
        waited += 10u;
    }

    return found ? 0u : 1u;
}

/**
  * @brief  读取响应直到超时，保存到缓冲区
  * @param  buf        : 输出缓冲区
  * @param  buf_size   : 缓冲区大小
  * @param  timeout_ms : 超时时间 ms
  * @retval 0: 成功  1: 超时无数据
  */
static uint8_t _usrk5_read_until(char *buf, uint16_t buf_size, uint32_t timeout_ms)
{
    uint16_t total = 0u;
    uint32_t waited = 0u;
    uint8_t  has_data = 0u;

    if (buf == (char *)0 || buf_size < 2u) {
        return 1u;
    }

    buf[0] = '\0';

    while (waited < timeout_ms) {
        if (g_uart2_dev.rx_complete) {
            uint8_t raw[128];
            uint16_t raw_len = uart2_recv_data(raw, sizeof(raw));
            uart2_clear_rx_flag();

            if (raw_len > 0u) {
                uint16_t i;
                for (i = 0u; i < raw_len && total < (buf_size - 1u); i++) {
                    buf[total++] = (char)raw[i];
                }
                buf[total] = '\0';
                has_data = 1u;

                if (_usrk5_stristr(buf, "+ok") != (const char *)0
                    || _usrk5_stristr(buf, "+ERR") != (const char *)0) {
                    break;
                }
            }
        }

        /* 也检查未帧完成的数据 */
        {
            uint16_t pending = 0u;
            if (g_uart2_dev.rx_tail >= g_uart2_dev.rx_head) {
                pending = g_uart2_dev.rx_tail - g_uart2_dev.rx_head;
            } else {
                pending = UART_RX_BUF_SIZE - g_uart2_dev.rx_head + g_uart2_dev.rx_tail;
            }
            if (pending > 0u && !g_uart2_dev.rx_complete) {
                uint8_t raw[128];
                uint16_t raw_len = uart2_recv_data(raw, pending > sizeof(raw) ? sizeof(raw) : pending);
                if (raw_len > 0u) {
                    uint16_t i;
                    for (i = 0u; i < raw_len && total < (buf_size - 1u); i++) {
                        buf[total++] = (char)raw[i];
                    }
                    buf[total] = '\0';
                    has_data = 1u;
                }
            }
        }

        delay_ms(50);
        waited += 50u;
    }

    return has_data ? 0u : 1u;
}

static char _usrk5_to_lower(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + 32);
    }
    return c;
}

static const char *_usrk5_stristr(const char *haystack, const char *needle)
{
    size_t needle_len;

    if (haystack == (const char *)0 || needle == (const char *)0) {
        return (const char *)0;
    }

    needle_len = strlen(needle);
    if (needle_len == 0u) {
        return haystack;
    }

    while (*haystack) {
        size_t j;
        uint8_t match = 1u;

        for (j = 0u; j < needle_len; j++) {
            if (haystack[j] == '\0') {
                match = 0u;
                break;
            }
            if (_usrk5_to_lower(haystack[j]) != _usrk5_to_lower(needle[j])) {
                match = 0u;
                break;
            }
        }

        if (match) {
            return haystack;
        }

        haystack++;
    }

    return (const char *)0;
}

static uint8_t _usrk5_check_ok(const char *buf)
{
    if (buf == (const char *)0) {
        return 1u;
    }
    if (_usrk5_stristr(buf, "+ok") != (const char *)0) {
        return 0u;
    }
    return 1u;
}

/**
  * @brief  发送 AT 指令并确认返回 +OK
  * @param  cmd : AT 指令字符串（不含末尾 CRLF）
  * @retval 0 : 指令执行成功（收到 +OK）
  * @retval 1 : 发送超时或模块回应 +ERR
  * @note   AT 指令的 CRLF 由 usrk5_send_at_cmd 内部追加
  *         收到的响应通过 _usrk5_check_ok 检查是否含 "+ok"（大小写不敏感）
  */
static uint8_t _usrk5_send_and_confirm(const char *cmd)
{
    char buf[64];
    if (usrk5_send_at_cmd(cmd, buf, sizeof(buf), USRK5_AT_TIMEOUT_DEF) != 0u) {
        return 1u;
    }
    return _usrk5_check_ok(buf);
}

uint8_t usrk5_Net_init(void)
{
    char sock_cmd[64];
    usrk5_init();
    _usrk5_uart_init(USRK5_UART_BAUD);

    if (usrk5_enter_at_mode() != 0u) {       // 进入 AT 配置模式
        uart3_send_string("[USRK5] enter AT mode FAIL\r\n");
        return 1u;     // 进入失败
    }

    // 查询模块的固件版本：AT+VER
    {
        char ver_buf[64];
        (void)usrk5_send_at_cmd("AT+VER", ver_buf, sizeof(ver_buf), 1000u);
        uart3_send_string("[USRK5] FW ver: ");
        uart3_send_string(ver_buf);
        uart3_send_string("\r\n");
    }
    // 查询模块的MAC地址：AT+MAC
    {
        char mac_buf[64];
        usrk5_send_at_cmd("AT+MAC", mac_buf, sizeof(mac_buf), 1000u);
        uart3_send_string("[USRK5] MAC: ");
        uart3_send_string(mac_buf);
        uart3_send_string("\r\n");
    }

#if USRK5_STAT_IP_MODE     // 编译如果使用静态IP方式
    if (_usrk5_send_and_confirm("AT+WANN=STATIC,"USRK5_STAT_IP_ADDR ","USRK5_STAT_IP_MASK ","USRK5_STAT_IP_GATEWAY) != 0u) {
        uart3_send_string("[USRK5] set static IP FAIL\r\n");
        return 1u;     // 静态IP设置失败了
    }
#else
    (void)_usrk5_send_and_confirm("AT+WANN=DHCP,0,0,0");    // 如果使用DHCP方式
#endif
    delay_ms(200);
    // 设置服务区IP+端口
    snprintf(sock_cmd, sizeof(sock_cmd),"AT+SOCK=TCPC,%s,%u",MQTT_BROKER_IP, (unsigned int)MQTT_BROKER_PORT);
    if(_usrk5_send_and_confirm(sock_cmd) != 0u){   
        uart3_send_string("[USRK5] set TCP client remote FAIL\r\n");
        return 1u;   // TCP Client客户端设置失败
    }
    delay_ms(200);


    (void)_usrk5_send_and_confirm("AT+ENTM");   // 退出AT命令模式

    usrk5_hw_reset();                      // 硬件复位 USR-K5 模块
    _usrk5_uart_init(USRK5_UART_BAUD);     // 配置本地串口和USR-K5参数一直

    uart3_send_string("[USRK5] init OK\r\n");
    return 0u;                            // 成功
}






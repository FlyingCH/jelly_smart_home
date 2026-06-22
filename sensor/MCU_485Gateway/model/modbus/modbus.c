/*多类型Modbus传感器协议数据包组合*/ 
#include "modbus.h"
#include "max3485.h"
#include "uart.h"
#include "delay.h"
#include "led.h"
#include <stdio.h>
#include <string.h>
#include "mqtt.h"
#include "public.h"
#include "crc.h"




#if SMOKE_SENSOR
/* ==================== 接收数据全局缓冲区 ==================== */
uint8_t g_smoke_recv_buf[MAX_MODBUS_RECV_BUF];
uint8_t g_smoke_recv_len = 0u;
#endif

#if TEMP_HUMI_SERSOR
uint8_t g_temphumi_recv_buf[MAX_MODBUS_RECV_BUF];
uint8_t g_temphumi_recv_len = 0u;
#endif

#if CIRCUIT_SERSOR
uint8_t g_circuit_recv_buf[MAX_MODBUS_RECV_BUF];
uint8_t g_circuit_recv_len = 0u;
#endif



multi_data_t g_multi_data;   // 多类型传感器数据

#if CIRCUIT_SERSOR
uint8_t g_electric_per_done[CIRCUIT_SERSOR_NUM] = {0};
sensor_poll_ctx_t g_poll_ctx = { .step = SM_IDLE, .dev_index = 0, .wait_start_ms = 0 };
#endif
 

// 温湿度传感器发包数据接口
#if TEMP_HUMI_SERSOR 
uint8_t temp_humi_send_data(const uint8_t addr){
    if(Multiple_modbus_package(addr, MODBUS_HOLDING_REG_READ, TEMP_HUM_READ_START_ADDR, TEMP_HUM_READ_READ_NUM) != 0){
        return 1;   // 发送命令失败
    }
    return 0;
}

/**
  * @brief  等待温湿度传感器 RS-485 回复
  * @retval 0 : 成功，数据存入 g_temphumi_recv_buf
  * @retval 1 : 超时
  */
uint8_t temp_humi_recv_data(void){
    uint32_t waited = 0u;
    g_temphumi_recv_len = 0u;
    while (waited < UART_RECV_TIMEOUT_MS) {
        if (g_uart1_dev.rx_complete) {
            uint16_t raw_len = uart1_recv_data(g_temphumi_recv_buf, MAX_MODBUS_RECV_BUF - 1u);
            uart1_clear_rx_flag();
            if (raw_len > 0u) {
                g_temphumi_recv_buf[raw_len] = 0u;
                g_temphumi_recv_len = (uint8_t)raw_len;
                return 0u;
            }
        }
        delay_ms(5);
        waited += 5u;
    }
    return 1u;
}


#endif


// 智能断路器发送数据接口
#if CIRCUIT_SERSOR  
uint8_t circuit_send_data(const uint8_t addr){
    if(Multiple_modbus_package(addr, MODBUS_HOLDING_REG_READ, SWITCH_READ_START_ADDR, SWITCH_REG_READ_NUM) != 0){
        return 1;   // 发送命令失败
    }
    return 0;
}

/**
  * @brief  等待断路器 RS-485 回复（33 个寄存器，数据量大）
  * @retval 0 : 成功，数据存入 g_circuit_recv_buf
  * @retval 1 : 超时
  */
uint8_t circuit_recv_data(void){
    uint32_t waited = 0u;
    g_circuit_recv_len = 0u;
    while (waited < UART_RECV_TIMEOUT_MS) {
        if (g_uart1_dev.rx_complete) {
            uint16_t raw_len = uart1_recv_data(g_circuit_recv_buf, MAX_MODBUS_RECV_BUF - 1u);
            uart1_clear_rx_flag();
            if (raw_len > 0u) {
                g_circuit_recv_buf[raw_len] = 0u;
                g_circuit_recv_len = (uint8_t)raw_len;
                return 0u;
            }
        }
        delay_ms(5);
        waited += 5u;
    }
    return 1u;
}

#endif




// 烟雾-温度-湿度传感器发送数据接口(适配LT-001-M1920-00Z产品)
// 询问方式-例：0x01 0x03 0x00 0x00 0x00 0x03 0x05 0xCB
#if SMOKE_SENSOR 
uint8_t smoke_send_data(const uint8_t addr){
    if(Multiple_modbus_package(addr, MODBUS_HOLDING_REG_READ, SMOKE_READ_START_ADDR, SMOKE_READ_READ_NUM) != 0){
        return 1;   // 发送命令失败
    }
    return 0;
}

/**
  * @brief  等待 RS-485 从机回复数据，存入全局缓冲区
  * @retval 0 : 成功收到数据
  * @retval 1 : 超时未收到回复
  * @note   调用前需先通过 smoke_send_data / Multiple_modbus_package 发出查询命令
  *         收到数据后，g_smoke_recv_buf 存放原始字节，g_smoke_recv_len 存放有效长度
  *         最大接收长度受 MAX_MODBUS_RECV_BUF 限制
  */
uint8_t smoke_recv_data(void)
{
    uint32_t waited = 0u;
    g_smoke_recv_len = 0u;
    while (waited < UART_RECV_TIMEOUT_MS) {
        if (g_uart1_dev.rx_complete) {
            uint16_t raw_len = uart1_recv_data(g_smoke_recv_buf, MAX_MODBUS_RECV_BUF - 1u);
            uart1_clear_rx_flag();
            if (raw_len > 0u) {
                g_smoke_recv_buf[raw_len] = 0u;
                g_smoke_recv_len = (uint8_t)raw_len;
                return 0u;
            }
        }
        delay_ms(5);
        waited += 5u;
    }
    return 1u;
}

#endif



/**
  * @brief  通用 Modbus RTU 查询帧打包发送接口
  * @param  addr      : 从机地址（合法范围 1~255，0 为广播地址将被拒绝）
  * @param  func      : Modbus 功能码（如 0x03 读保持寄存器）
  * @param  start_addr: 寄存器起始地址（16 位，大端序填入报文）
  * @param  num       : 读取/写入的寄存器数量（16 位，大端序填入报文）
  * @retval 0 : 帧发送成功
  * @retval 1 : 地址非法（addr == 0）或 max3485 发送失败
  * @note   内部调用 max3485_send_data 经 RS-485 总线发出
  *         CRC 仅对前 6 字节（地址 + 功能码 + 起始地址 + 数量）计算
  *         CRC 低字节先发，高字节后发，符合 Modbus RTU 字节序
  */
uint8_t Multiple_modbus_package(const uint8_t addr, uint8_t func, uint16_t start_addr, uint16_t num){
    uint16_t crc = 0;
    uint8_t msage_buf[8] = {0};
    /* Modbus 地址合法范围 1~255，0 为广播地址不可用于单播查询 */
    if (addr == 0u) 
        return 1u;
    msage_buf[0] = addr;                                          // 从机地址
    msage_buf[1] = func;                                          // 功能码
    msage_buf[2] = (start_addr >> 8) & 0xff;                      // 寄存器起始地址高位
    msage_buf[3] = (start_addr >> 0) & 0xff;                      // 寄存器起始地址低位
    msage_buf[4] = (num >> 8) & 0xff;                             // 读取数量高位
    msage_buf[5] = (num >> 0) & 0xff;                             // 读取数量低位
    crc = uiCrcCalFunc(msage_buf, 6);                             // CRC 仅覆盖前 6 字节
    msage_buf[6] = (uint8_t)((crc >> 0) & 0xFF);                  // CRC 低字节（先发）
    msage_buf[7] = (uint8_t)((crc >> 8) & 0xFF);                  // CRC 高字节（后发）
    if (max3485_send_data(msage_buf, 8) != 0)
        return 1;


// 串口发送数据打印        
#if PRINT_DATA == 1 
    {
        static const char hex_tbl[] = "0123456789ABCDEF";
        uint8_t hex_buf[24];  /* 8字节 × 3字符("XX ") = 24 */
        uint8_t i;
        max3485_U3_send_data((const uint8_t *)"[MODBUS] send: ", 15u);
        for (i = 0; i < 8; i++) {
            hex_buf[i * 3 + 0] = (uint8_t)hex_tbl[(msage_buf[i] >> 4) & 0x0F];
            hex_buf[i * 3 + 1] = (uint8_t)hex_tbl[(msage_buf[i] >> 0) & 0x0F];
            hex_buf[i * 3 + 2] = ' ';
        }
        max3485_U3_send_data(hex_buf, 24u);
        max3485_U3_send_data((const uint8_t *)"\r\n", 2u);
    }
#endif



    return 0;
}


// 温湿度传感器发送命令：35 03 00 00 00 02 C0 7F 
// 温湿度传感器回复示例：35 03 04 02 99(湿度) 01 07(温度) 1E 35


/* ── 轮询状态机：初始化 ── */
void sensor_poll_init(void) {
    g_poll_ctx.step      = SM_IDLE;
    g_poll_ctx.dev_index = 0;
    g_poll_ctx.wait_start_ms = 0;
    memset(g_electric_per_done, 0, sizeof(g_electric_per_done));
}

/* ── 轮询状态机：每主循环推进一步 ── */
void sensor_poll_step(void){
    uint8_t addr;   // 当前轮询地址
    uint32_t now;   // 当前时间戳
    uint32_t e_raw;  // 原始能量值

    switch (g_poll_ctx.step) {
    case SM_IDLE:     /* ── 空闲 → 开始轮询第一个设备 ── */
        g_poll_ctx.dev_index = 0;
        g_poll_ctx.step = SM_SEND;
        break;

    
    case SM_SEND:    /* ── 发送查询帧 ── */
        led_toggle(LED_STATE);
        addr = circuit_device_id[g_poll_ctx.dev_index];
        /* 发送前清除接收缓冲区和标志，避免读到旧数据 */
        uart1_clear_rx_flag();
        g_uart1_dev.rx_head = g_uart1_dev.rx_tail;
        if (circuit_send_data(addr) == 0) {
            g_poll_ctx.wait_start_ms = get_tick_ms();
            g_poll_ctx.step = SM_WAIT_RECV;

        } else {   // 消息发送失败
            max3485_U3_send_data((const uint8_t *)"[CKT] send fail\r\n", 17u);
            /* send 失败：跳过当前，转到下一个 */
            g_poll_ctx.dev_index++;
            if (g_poll_ctx.dev_index >= CIRCUIT_SERSOR_NUM) {
                g_poll_ctx.dev_index = 0;
            }
            /* 保持 SM_SEND 状态，下一次调用继续发下一个 */
        }
        break;

    /* ── 等待回复（非阻塞） ── */
    case SM_WAIT_RECV:
        led_toggle(LED_STATE);
        if (g_uart1_dev.rx_complete) {     /* 先检查是否收到完整帧 */
            uint16_t raw_len = uart1_recv_data(g_circuit_recv_buf, MAX_MODBUS_RECV_BUF - 1u);
            uart1_clear_rx_flag();
            if (raw_len > 0u) {
                g_circuit_recv_buf[raw_len] = 0u;
                g_circuit_recv_len = (uint8_t)raw_len;
#if PRINT_DATA == 1
                {
                    static const char hx[] = "0123456789ABCDEF";
                    static uint8_t hex_buf[96]; /* 32字节 × 3字符 = 96，分片发送避免冲爆TX缓冲区 */
                    uint16_t i, chunk_start;
                    max3485_U3_send_data((const uint8_t *)"[MODBUS] recv: ", 15u);
                    for (chunk_start = 0; chunk_start < raw_len; chunk_start += 32u) {
                        uint16_t pos = 0;
                        for (i = chunk_start; i < raw_len && i < chunk_start + 32u && pos + 3u <= sizeof(hex_buf); i++) {
                            hex_buf[pos++] = (uint8_t)hx[(g_circuit_recv_buf[i] >> 4) & 0x0F];
                            hex_buf[pos++] = (uint8_t)hx[(g_circuit_recv_buf[i] >> 0) & 0x0F];
                            hex_buf[pos++] = ' ';
                        }
                        max3485_U3_send_data(hex_buf, pos);
                    }
                    max3485_U3_send_data((const uint8_t *)"\r\n", 2u);
                }
#endif

                /* 剥离 RS-485 总线噪声字节：在缓冲区前部搜索地址字节 */
                {
                    uint8_t target_addr = circuit_device_id[g_poll_ctx.dev_index];
                    uint16_t leading = 0;
                    while (leading + 4u < raw_len && g_circuit_recv_buf[leading] != target_addr) {
                        leading++;
                    }
                    if (leading > 0u && g_circuit_recv_buf[leading] == target_addr) {
                        /* 找到地址字节，将数据前移覆盖噪声 */
                        uint16_t j;
                        raw_len -= leading;
                        for (j = 0; j < raw_len; j++) {
                            g_circuit_recv_buf[j] = g_circuit_recv_buf[j + leading];
                        }
                    }
                }
                /* 验证接收到的Modbus响应帧长度是否足够 */
                /* 完整响应 = 1(地址) + 1(功能码) + 1(字节数) + 66(数据) + 2(CRC) = 71字节 */
                if (raw_len < 69u) {  /* 至少需要69字节才能安全访问到[68] */
                    max3485_U3_send_data((const uint8_t *)"[CKT] recv too short: ", 22u);
                    /* 打印实际长度 */
                    {
                        uint8_t len_buf[4];
                        len_buf[0] = (uint8_t)('0' + (raw_len / 10) % 10);
                        len_buf[1] = (uint8_t)('0' + raw_len % 10);
                        len_buf[2] = 'B';
                        max3485_U3_send_data(len_buf, 3u);
                    }
                    max3485_U3_send_data((const uint8_t *)"\r\n", 2u);
                    /* 跳过当前设备，继续轮询 */
                    g_poll_ctx.dev_index = (g_poll_ctx.dev_index + 1) % CIRCUIT_SERSOR_NUM;
                    g_poll_ctx.step = SM_SEND;
                    break;
                }
                g_poll_ctx.step = SM_PARSE;
                break;
            }
        }

        /* 检查超时（500ms） */
        now = get_tick_ms();
        if ((now - g_poll_ctx.wait_start_ms) >= UART_RECV_TIMEOUT_MS) {
            max3485_U3_send_data((const uint8_t *)"[CKT] recv timeout\r\n", 20u);
            /* 超时：跳过当前，转到下一个 */
            g_poll_ctx.dev_index = (g_poll_ctx.dev_index + 1) % CIRCUIT_SERSOR_NUM;
            g_poll_ctx.step = SM_SEND;
        }
        break;

    /* ── 解析回复数据 ── */
    case SM_PARSE:
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].addr = circuit_device_id[g_poll_ctx.dev_index];
        if (g_circuit_recv_buf[4] == 0x00){    // 闭合状态
            g_multi_data.electric_sensor[g_poll_ctx.dev_index].sw = 1;
        }
        else{                                   // 断开状态
            g_multi_data.electric_sensor[g_poll_ctx.dev_index].sw = 0;
        }
        // 电流
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].current = (g_circuit_recv_buf[5] * 256 + g_circuit_recv_buf[6]) * 0.1; // 单位A                               
        //g_multi_data.electric_sensor[g_poll_ctx.dev_index].current = ((g_circuit_recv_buf[5] << 8) | g_circuit_recv_buf[6]) * 0.01f;  
        // 电压
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].voltage = ((g_circuit_recv_buf[13] << 8) | g_circuit_recv_buf[14]) * 0.1f;
        // 功率
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].power = (g_circuit_recv_buf[55] * 256 + g_circuit_recv_buf[56]) * 1;    // 单位 W                       
        //g_multi_data.electric_sensor[g_poll_ctx.dev_index].power = (float)((g_circuit_recv_buf[55] << 8) | g_circuit_recv_buf[56]) * 0.01f;   // 单位kw
        // 温度
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].temp = (g_circuit_recv_buf[21] * 256 + g_circuit_recv_buf[22]) * 1.0;         // 单位℃                
        //g_multi_data.electric_sensor[g_poll_ctx.dev_index].temp = ((g_circuit_recv_buf[21] << 8) | g_circuit_recv_buf[22]) * 0.01f;
        // 功率因数
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].factor  = (uint8_t)(((g_circuit_recv_buf[63] << 8) | g_circuit_recv_buf[64]) & 0xFF) * 0.01f;   
        // 电能
        // e_raw = ((uint32_t)(g_circuit_recv_buf[65] << 8) | g_circuit_recv_buf[66]) << 16
        //       | ((uint32_t)(g_circuit_recv_buf[67] << 8) | g_circuit_recv_buf[68]);
        e_raw = (g_circuit_recv_buf[65] << 24) + (g_circuit_recv_buf[66] << 16) + (g_circuit_recv_buf[67] << 8) + g_circuit_recv_buf[68];
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].energy = e_raw * 0.0001; // 单位 Wh

#if PRINT_DATA == 1
        {
            /* 手动将整数转为十进制字符串，返回写入长度 */
            #define ITOA_BUF_LEN  12
            static const char hx[] = "0123456789";
            electric_data_t *es = &g_multi_data.electric_sensor[g_poll_ctx.dev_index];
            uint8_t idx = g_poll_ctx.dev_index;
            uint8_t int_buf[ITOA_BUF_LEN];
            uint8_t pos, len;
            int32_t val;
            uint32_t uval;
            uint32_t frac;

            /* ── 打印标题 ── */
            max3485_U3_send_data((const uint8_t *)"[PARSE] dev=", 12u);
            /* 打印设备索引 */
            int_buf[0] = (uint8_t)('0' + idx);
            max3485_U3_send_data(int_buf, 1u);
            max3485_U3_send_data((const uint8_t *)" addr=", 6u);
            /* 打印地址 (hex) */
            int_buf[0] = (uint8_t)hx[(es->addr >> 4) & 0x0F];
            int_buf[1] = (uint8_t)hx[(es->addr >> 0) & 0x0F];
            max3485_U3_send_data(int_buf, 2u);

            /* ── 打印开关状态 ── */
            max3485_U3_send_data((const uint8_t *)" sw=", 4u);
            int_buf[0] = (uint8_t)('0' + es->sw);
            max3485_U3_send_data(int_buf, 1u);

            /* ── 打印电压 (xx.x V) ── */
            max3485_U3_send_data((const uint8_t *)" V=", 3u);
            val = (int32_t)(es->voltage * 10.0f);
            /* 打印整数部分 */
            pos = ITOA_BUF_LEN;
            uval = (uint32_t)(val / 10);
            if (uval == 0u) { int_buf[--pos] = '0'; }
            else { while (uval > 0u) { int_buf[--pos] = (uint8_t)('0' + uval % 10u); uval /= 10u; } }
            len = (uint8_t)(ITOA_BUF_LEN - pos);
            max3485_U3_send_data(&int_buf[pos], len);
            /* 小数点 + 小数第一位 */
            frac = (uint32_t)(val % 10);
            if (frac > 9u) frac = 0u;  /* 负数保护 */
            int_buf[0] = '.';
            int_buf[1] = (uint8_t)('0' + frac);
            max3485_U3_send_data(int_buf, 2u);

            /* ── 打印电流 (xx.xx A) ── */
            max3485_U3_send_data((const uint8_t *)" A=", 3u);
            val = (int32_t)(es->current * 100.0f);
            pos = ITOA_BUF_LEN;
            uval = (uint32_t)(val / 100);
            if (uval == 0u) { int_buf[--pos] = '0'; }
            else { while (uval > 0u) { int_buf[--pos] = (uint8_t)('0' + uval % 10u); uval /= 10u; } }
            len = (uint8_t)(ITOA_BUF_LEN - pos);
            max3485_U3_send_data(&int_buf[pos], len);
            frac = (uint32_t)(val % 100);
            if (frac > 99u) frac = 0u;
            int_buf[0] = '.';
            int_buf[1] = (uint8_t)('0' + frac / 10u);
            int_buf[2] = (uint8_t)('0' + frac % 10u);
            max3485_U3_send_data(int_buf, 3u);

            /* ── 打印功率 (xx.xx W) ── */
            max3485_U3_send_data((const uint8_t *)" W=", 3u);
            val = (int32_t)(es->power * 100.0f);
            pos = ITOA_BUF_LEN;
            uval = (uint32_t)(val / 100);
            if (uval == 0u) { int_buf[--pos] = '0'; }
            else { while (uval > 0u) { int_buf[--pos] = (uint8_t)('0' + uval % 10u); uval /= 10u; } }
            len = (uint8_t)(ITOA_BUF_LEN - pos);
            max3485_U3_send_data(&int_buf[pos], len);
            frac = (uint32_t)(val % 100);
            if (frac > 99u) frac = 0u;
            int_buf[0] = '.';
            int_buf[1] = (uint8_t)('0' + frac / 10u);
            int_buf[2] = (uint8_t)('0' + frac % 10u);
            max3485_U3_send_data(int_buf, 3u);

            /* ── 打印温度 (xx.x C) ── */
            max3485_U3_send_data((const uint8_t *)" T=", 3u);
            val = (int32_t)(es->temp * 10.0f);
            pos = ITOA_BUF_LEN;
            uval = (uint32_t)(val / 10);
            if (uval == 0u) { int_buf[--pos] = '0'; }
            else { while (uval > 0u) { int_buf[--pos] = (uint8_t)('0' + uval % 10u); uval /= 10u; } }
            len = (uint8_t)(ITOA_BUF_LEN - pos);
            max3485_U3_send_data(&int_buf[pos], len);
            frac = (uint32_t)(val % 10);
            if (frac > 9u) frac = 0u;
            int_buf[0] = '.';
            int_buf[1] = (uint8_t)('0' + frac);
            max3485_U3_send_data(int_buf, 2u);

            /* ── 打印功率因数 (整数) ── */
            max3485_U3_send_data((const uint8_t *)" pf=", 4u);
            pos = ITOA_BUF_LEN;
            uval = (uint32_t)es->factor;
            if (uval == 0u) { int_buf[--pos] = '0'; }
            else { while (uval > 0u) { int_buf[--pos] = (uint8_t)('0' + uval % 10u); uval /= 10u; } }
            len = (uint8_t)(ITOA_BUF_LEN - pos);
            max3485_U3_send_data(&int_buf[pos], len);

            /* ── 打印电能 (整数, 0.01kWh) ── */
            max3485_U3_send_data((const uint8_t *)" E=", 3u);
            pos = ITOA_BUF_LEN;
            uval = es->energy;
            if (uval == 0u) { int_buf[--pos] = '0'; }
            else { while (uval > 0u) { int_buf[--pos] = (uint8_t)('0' + uval % 10u); uval /= 10u; } }
            len = (uint8_t)(ITOA_BUF_LEN - pos);
            max3485_U3_send_data(&int_buf[pos], len);

            max3485_U3_send_data((const uint8_t *)"\r\n", 2u);
        }
#endif

        /* 标记当前设备数据就绪，MQTT 层稍后发布 */
        g_electric_per_done[g_poll_ctx.dev_index] = 1;

        /* 前进到下一个设备 */
        g_poll_ctx.dev_index = (g_poll_ctx.dev_index + 1) % CIRCUIT_SERSOR_NUM;
        if (g_poll_ctx.dev_index == 0) {
            /* 完成整轮轮询 → 回到 IDLE，给 mqtt_loop 留更多时间 */
            g_poll_ctx.step = SM_IDLE;
        } else {
            g_poll_ctx.step = SM_SEND;
        }
        break;

    default:
        g_poll_ctx.step = SM_IDLE;
        break;
    }
}










// 断路器(空开)设备开关的预令与动令发送
#if CIRCUIT_SERSOR

static uint8_t circuit_write_single_reg(const uint8_t addr, uint16_t reg, uint16_t value)
{
    if (Multiple_modbus_package(addr, MODBUS_SINGLE_REG_WRITE, reg, value) != 0u) {
        return 1u;
    }
    if (circuit_recv_data() != 0u) {
        return 2u;
    }
    return 0u;
}

uint8_t circuit_switch_close(const uint8_t addr)
{
    if (circuit_write_single_reg(addr, CIRCUIT_REMOTE_PRE_REG, CIRCUIT_CMD_CLOSE_PRE) != 0u) {
        return 1u;
    }
    if (circuit_write_single_reg(addr, CIRCUIT_REMOTE_ACT_REG, CIRCUIT_CMD_CLOSE_ACT) != 0u) {
        return 2u;
    }
    return 0u;
}

uint8_t circuit_switch_open(const uint8_t addr)
{
    if (circuit_write_single_reg(addr, CIRCUIT_REMOTE_PRE_REG, CIRCUIT_CMD_OPEN_PRE) != 0u) {
        return 1u;
    }
    if (circuit_write_single_reg(addr, CIRCUIT_REMOTE_ACT_REG, CIRCUIT_CMD_OPEN_ACT) != 0u) {
        return 2u;
    }
    return 0u;
}

#endif












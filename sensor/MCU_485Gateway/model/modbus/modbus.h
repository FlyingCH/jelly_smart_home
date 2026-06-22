#ifndef __MODBUS_H
#define __MODBUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"
#include "public.h"         // 传感器启用在public.h中定义的宏

/*Modbus功能码*/
#define MODBUS_HOLDING_REG_READ    0x03    // 读保持寄存器
#define MODBUS_SINGLE_REG_WRITE    0x06    // 预置单寄存器



/*Modbus 接收超时时间 ms（等待从机回复的最大时间）*/
#define UART_RECV_TIMEOUT_MS      500u
#define MAX_MODBUS_RECV_BUF       256



/******Modbus RTU 传感器收发接口******/
uint8_t Multiple_modbus_package(const uint8_t addr, uint8_t func, uint16_t start_addr, uint16_t num);


#if TEMP_HUMI_SERSOR
/*杂牌温湿度传感器(带数显产品)*/
#define TEMP_HUM_READ_START_ADDR    0x0000    // 读取起始地址
#define TEMP_HUM_READ_READ_NUM      2         // 杂牌温湿度传感器读取数量（读 2 个寄存器）
#define TEMP_HUMI_JSON_BUF_SIZE     128       // MQTT JSON 发布缓冲区

extern uint8_t  g_temphumi_recv_buf[MAX_MODBUS_RECV_BUF];
extern uint8_t  g_temphumi_recv_len;
uint8_t temp_humi_send_data(const uint8_t addr);
uint8_t temp_humi_recv_data(void);
void MultiType_sensor_data_poll(void);

/* 温湿度传感器 Modbus 地址表（按实际挂载顺序排列） */
static const uint8_t temp_humi_device_id[TEMP_HUMI_SERSOR_NUM] = {0x35};
#endif


/*佳岚断路器Modbus寄存器读取*/
#if CIRCUIT_SERSOR
#define SWITCH_READ_START_ADDR    0x0010    // 断路器读取开始地址
#define SWITCH_REG_READ_NUM       33        // 断路器读取数量（读 33 个寄存器）

/* 断路器远程控制命令寄存器 */
#define CIRCUIT_REMOTE_PRE_REG   0x0080    // 遥控预令寄存器
#define CIRCUIT_REMOTE_ACT_REG   0x0081    // 遥控动令寄存器
#define CIRCUIT_CMD_CLOSE_PRE    0x0155    // 合闸预令
#define CIRCUIT_CMD_CLOSE_ACT    0x01AA    // 合闸动令
#define CIRCUIT_CMD_OPEN_PRE     0x0255    // 分闸预令
#define CIRCUIT_CMD_OPEN_ACT     0x02AA    // 分闸动令

extern uint8_t  g_circuit_recv_buf[MAX_MODBUS_RECV_BUF];
extern uint8_t  g_circuit_recv_len;
uint8_t circuit_send_data(const uint8_t addr);
uint8_t circuit_recv_data(void);
uint8_t circuit_switch_close(const uint8_t addr);
uint8_t circuit_switch_open(const uint8_t addr);

/* 断路器 Modbus 地址表（按实际挂载顺序排列） */
static const uint8_t circuit_device_id[CIRCUIT_SERSOR_NUM] = {0x21, 0x22, 0x23, 0x83, 0x96};
#endif





/*龙腾伟业烟雾-温湿度传感器*/
#if SMOKE_SENSOR
#define SMOKE_READ_START_ADDR     0x0000
#define SMOKE_READ_READ_NUM       3        // 烟雾-温湿度传感器读取数量（读 3 个寄存器）

extern uint8_t  g_smoke_recv_buf[MAX_MODBUS_RECV_BUF];  // 接收到的数据
extern uint8_t  g_smoke_recv_len;                       // 接收数据的长度

uint8_t smoke_send_data(const uint8_t addr);
uint8_t smoke_recv_data(void);  // 烟雾传感器温湿度数据接收接口

/* 烟雾传感器 Modbus 地址表（按实际挂载顺序排列） */
static const uint8_t smoke_device_id[SMOKE_SENSOR_NUM] = {0x01};
#endif



/* ── 轮询状态机 ── */
#if CIRCUIT_SERSOR
typedef enum {
    SM_IDLE = 0,
    SM_SEND,
    SM_WAIT_RECV,
    SM_PARSE,
} sensor_step_t;

typedef struct {
    sensor_step_t step;
    uint8_t      dev_index;
    uint32_t     wait_start_ms;
} sensor_poll_ctx_t;

extern uint8_t g_electric_per_done[CIRCUIT_SERSOR_NUM];
extern sensor_poll_ctx_t g_poll_ctx;

void sensor_poll_init(void);
void sensor_poll_step(void);
#endif


// 温湿度传感器的
typedef struct{
    uint8_t  addr;     // Modbus 从机物理地址
    float temp;
    float humi;
}temp_humi_data_t;

// 断路器的
typedef struct{
    uint8_t  addr;     // Modbus 从机物理地址
    uint8_t sw;     // 开关状态
    float current;   // 电流
    float voltage;   // 电压
    float power;     // 功率
    float temp;      // 温度
    uint8_t factor;  // 功率因素
    uint32_t energy; // 电能
}electric_data_t;

// 烟雾传感器的
typedef struct {
    uint8_t  addr;     // Modbus 从机物理地址
    float temp;
    float humi;
    uint16_t smoke; // 烟雾浓度
} smoke_data_t;


// 多类型传感器
typedef struct {
    temp_humi_data_t temphumi_sensor[TEMP_HUMI_SERSOR_NUM];
    smoke_data_t smoke_sensor[SMOKE_SENSOR_NUM];
    electric_data_t electric_sensor[CIRCUIT_SERSOR_NUM];
    uint8_t tempHumi_done;   // 温湿度全部读取完成
    uint8_t electric_done;   // 电力全部读取完成
    uint8_t smoke_done;      // 烟雾传感器全部读取完成
    uint8_t newData;         // 新数据标志位
} multi_data_t;


extern multi_data_t g_multi_data;


#ifdef __cplusplus
}
#endif

#endif




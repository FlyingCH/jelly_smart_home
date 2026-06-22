# MCU_485Gateway 数据采集轮询升级方案

## 项目定位

STM32F103C8T6 裸机固件，工具链 `arm-none-eabi-gcc` (nano.specs)，无 RTOS，代码路径 `/home/jelly/project/smarthome/sensor/MCU_485Gateway/`。当前仅启用断路器传感器 (`public.h` 中 `CIRCUIT_SERSOR=1`)，但方案须兼容未来温湿度/烟雾传感器扩展。

---

## 一、现状问题

当前 `dlq_data_acquisition()` 在单个主循环内**阻塞轮询全部 5 个断路器**：

```
while(1):
    轮询断路器1 (send + 阻塞忙等500ms + parse)
    轮询断路器2 (send + 阻塞忙等500ms + parse)
    ... ×5
    mqtt_loop()          ← MQTT 下行命令在 5 轮全部完成后才处理
    delay_ms(500)        ← 空转
```

**核心缺陷**：
1. 每个 `circuit_recv_data()` 以 `delay_ms(5)` 步进忙等，5 设备最坏阻塞 2.5s
2. MQTT 订阅回调（远程开关命令）在此期间得不到处理
3. `delay_ms(500)` 纯浪费 CPU 时间
4. `dlq_data_acquisition()` 在 send 失败时直接 `return`，但 `electric_done` 已清零，本轮数据全丢；而 `MultiType_sensor_data_poll()` 正确继续下一个
5. `dlq_data_acquisition()` 与 `MultiType_sensor_data_poll()` 断路器部分代码重复

---

## 二、目标架构：单设备步进轮询模型

**核心思路**：每次主循环只轮询 **1 个设备**，用状态机在循环间保持进度，MQTT 每次循环都能执行。

```
while(1):
    sensor_poll_step()       ← 仅推进一步（send / check_recv / parse / next）
    mqtt_loop()              ← 每轮都执行，延迟 < 500ms
    // 无需 delay_ms，或极小值(10ms)用于调度
```

### 2.1 状态机设计

```c
typedef enum {
    SM_IDLE,          // 空闲
    SM_SEND,          // 发送查询帧
    SM_WAIT_RECV,     // 等待回复（非阻塞，基于 tick）
    SM_PARSE,         // 解析数据 → 标记该设备 done
    SM_PUBLISH,       // MQTT 发布单个设备数据
} sensor_step_t;
```

配合一个全局上下文追踪当前进度：

```c
typedef struct {
    sensor_step_t step;                 // 当前状态
    uint8_t      dev_index;             // 当前在轮询第几个设备（0 ~ N-1）
    uint32_t     wait_start_ms;         // WAIT_RECV 的起始 tick (get_tick_ms())
} sensor_poll_ctx_t;
```

### 2.2 状态迁移图

```
                          ┌─────────────────┐
                          │     SM_IDLE     │
                          └────────┬────────┘
                                   │ 无条件
                                   ▼
                          ┌─────────────────┐
               ◄──────────│    SM_SEND       │
               │          └────────┬────────┘
               │                   │ send OK → entry WAIT, 记录 wait_start_ms
               │  send 失败        ▼
               │  (跳过当前)    ┌─────────────────┐
               │               │  SM_WAIT_RECV    │
               │               └────────┬────────┘
               │                        │ rx_complete=1 → 收到数据 → SM_PARSE
               │                        │ 超时(now - start > 500ms) → 打印超时 → next device
               │                        ▼
               │               ┌─────────────────┐
               │               │    SM_PARSE      │
               │               └────────┬────────┘
               │                        │ 解析成功 → 置位 per_device_done[dev_index] = 1
               │                        │ dev_index = (dev_index+1) % N
               │                        │ if dev_index == 0: all round complete
               │                        ▼
               └────────────────────────┘  next device → SM_SEND
```

### 2.3 单设备数据上报

```c
// mqtt_loop() 中，遍历检查每设备标志
uint8_t per_device_done[CIRCUIT_SERSOR_NUM];    // 每设备独立 done 标志
uint32_t device_last_poll_ms[CIRCUIT_SERSOR_NUM]; // 每设备上次成功时间

// 每个主循环：收集到哪个设备的新数据就立即发布哪个
for (i = 0; i < CIRCUIT_SERSOR_NUM; i++) {
    if (per_device_done[i]) {
        mqtt_publish_single_device(i);   // 只发这一个的 JSON
        per_device_done[i] = 0;
        device_last_poll_ms[i] = get_tick_ms();
    }
}
```

---

## 三、需要修改的文件及详细改动

### 3.1 `model/modbus/modbus.h` — 新增类型和声明

**新增内容**：

```c
// 在文件末尾 #endif 之前添加

/* ── 轮询状态机 ── */
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

extern uint8_t g_electric_per_done[CIRCUIT_SERSOR_NUM];    // 单设备 done 标志
extern sensor_poll_ctx_t g_poll_ctx;                         // 轮询上下文

void sensor_poll_init(void);                                // 重置状态机 (bsp_Init 中调用)
void sensor_poll_step(void);                                // 推进一步 (main 循环中调用)
```

### 3.2 `model/modbus/modbus.c` — 核心改造

**保留不变的函数**（不修改）：
- `Multiple_modbus_package()` — 通用 Modbus 帧打包
- `circuit_send_data()` — 发送断路器查询
- `temp_humi_send_data()`, `temp_humi_recv_data()`, `smoke_send_data()`, `smoke_recv_data()` — 不修改（未来用同样模式扩展）

**新增全局变量**（在 `g_multi_data` 声明附近）：

```c
uint8_t g_electric_per_done[CIRCUIT_SERSOR_NUM] = {0};
sensor_poll_ctx_t g_poll_ctx = { .step = SM_IDLE, .dev_index = 0, .wait_start_ms = 0 };
```

**新增函数 1：`sensor_poll_init()`**

```c
void sensor_poll_init(void) {
    g_poll_ctx.step      = SM_IDLE;
    g_poll_ctx.dev_index = 0;
    g_poll_ctx.wait_start_ms = 0;
    memset(g_electric_per_done, 0, sizeof(g_electric_per_done));
}
```

**新增函数 2：`sensor_poll_step()`**（每主循环调用一次）

```c
void sensor_poll_step(void){
    uint8_t addr;
    uint32_t now;
    uint32_t e_raw;

    switch (g_poll_ctx.step) {
    /* ── 空闲 → 开始轮询第一个设备 ── */
    case SM_IDLE:
        g_poll_ctx.dev_index = 0;
        g_poll_ctx.step = SM_SEND;
        break;

    /* ── 发送查询帧 ── */
    case SM_SEND:
        addr = circuit_device_id[g_poll_ctx.dev_index];
        if (circuit_send_data(addr) == 0) {
            g_poll_ctx.wait_start_ms = get_tick_ms();
            g_poll_ctx.step = SM_WAIT_RECV;
        } else {
            max3485_U3_send_data((const uint8_t *)"[CKT] send fail\r\n", 16u);
            // send 失败：跳过当前，转到下一个
            g_poll_ctx.dev_index++;
            if (g_poll_ctx.dev_index >= CIRCUIT_SERSOR_NUM) {
                g_poll_ctx.dev_index = 0;
            }
            /* 保持 SM_SEND 状态，下一次调用继续发下一个 */
        }
        break;

    /* ── 等待回复（非阻塞） ── */
    case SM_WAIT_RECV:
        /* 先检查是否收到完整帧 */
        if (g_uart1_dev.rx_complete) {
            uint16_t raw_len = uart1_recv_data(g_circuit_recv_buf, MAX_MODBUS_RECV_BUF - 1u);
            uart1_clear_rx_flag();
            if (raw_len > 0u) {
                g_circuit_recv_buf[raw_len] = 0u;
                g_circuit_recv_len = (uint8_t)raw_len;
                g_poll_ctx.step = SM_PARSE;
                break;
            }
        }
        /* 检查超时（500ms） */
        now = get_tick_ms();
        if ((now - g_poll_ctx.wait_start_ms) >= UART_RECV_TIMEOUT_MS) {
            max3485_U3_send_data((const uint8_t *)"[CKT] recv timeout\r\n", 20u);
            // 超时：跳过当前，转到下一个
            g_poll_ctx.dev_index = (g_poll_ctx.dev_index + 1) % CIRCUIT_SERSOR_NUM;
            g_poll_ctx.step = SM_SEND;
        }
        break;

    /* ── 解析回复数据 ── */
    case SM_PARSE:
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].addr = circuit_device_id[g_poll_ctx.dev_index];
        if (g_circuit_recv_buf[4] == 0x00)
            g_multi_data.electric_sensor[g_poll_ctx.dev_index].sw = 1;
        else
            g_multi_data.electric_sensor[g_poll_ctx.dev_index].sw = 0;

        g_multi_data.electric_sensor[g_poll_ctx.dev_index].current = ((g_circuit_recv_buf[5] << 8) | g_circuit_recv_buf[6]) * 0.01f;
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].voltage = ((g_circuit_recv_buf[13] << 8) | g_circuit_recv_buf[14]) * 0.1f;
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].power   = (float)((g_circuit_recv_buf[55] << 8) | g_circuit_recv_buf[56]) * 0.01f;
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].temp    = ((g_circuit_recv_buf[21] << 8) | g_circuit_recv_buf[22]) * 0.01f;
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].factor  = (uint8_t)((g_circuit_recv_buf[63] << 8) | g_circuit_recv_buf[64]) & 0xFF;
        e_raw = ((uint32_t)(g_circuit_recv_buf[65] << 8) | g_circuit_recv_buf[66]) << 16
              | ((uint32_t)(g_circuit_recv_buf[67] << 8) | g_circuit_recv_buf[68]);
        g_multi_data.electric_sensor[g_poll_ctx.dev_index].energy = e_raw;

        /* 标记当前设备数据就绪，MQTT 层稍后发布 */
        g_electric_per_done[g_poll_ctx.dev_index] = 1;

        /* 前进到下一个设备 */
        g_poll_ctx.dev_index = (g_poll_ctx.dev_index + 1) % CIRCUIT_SERSOR_NUM;
        if (g_poll_ctx.dev_index == 0) {
            /* 完成整轮轮询 → 回到 IDLE，给 mqtt_loop 留更多时间 */
            g_poll_ctx.step = SM_IDLE;
        } else {
            g_poll_ctx.step = SM_SEND;
            /* 可选：加一个设备之间的最小间隔避免总线冲突 */
            g_poll_ctx.wait_start_ms = get_tick_ms(); /* 复用为间隔计时 */
        }
        break;
    }
}
```

**删除内容**：
- 删除 `dlq_data_acquisition()` 函数（第 267-299 行），或保留并标记 `// deprecated, use sensor_poll_step()`

### 3.3 `model/mqtt/mqtt.c` — MQTT 上报改为逐设备发布

**`mqtt_loop()` 函数改造**（约第 474-559 行）：

将原来检查 `g_multi_data.electric_done` 然后批量发布 5 个设备的代码块，替换为逐设备检查 `g_electric_per_done[i]`：

```c
void mqtt_loop(void){
    static uint32_t last_ping_tick = 0u;
    uint8_t i;

    mqtt_yield();

    /* ── 断路器/电力传感器数据上报：逐设备检查，有就发 ── */
    if (mqtt_get_state() == MQTT_STATE_CONNECTED) {
        for (i = 0; i < CIRCUIT_SERSOR_NUM; i++) {
            if (g_electric_per_done[i]) {
                char json[256];
                int len = sprintf(json,
                    "{\"devcd\":\"%s\",\"devtype\":\"electricity_sensor\","
                    "\"nodeid\":\"elsensor_%u\","
                    "\"data\":{\"sw\":%u,\"voltage\":%d.%01d,\"current\":%d.%02d,"
                    "\"power\":%d.%02d,\"temp\":%d.%01d,\"factor\":%u,\"energy\":%lu}}",
                    MQTT_CLIENT_ID,
                    (unsigned int)g_multi_data.electric_sensor[i].addr,
                    (unsigned int)g_multi_data.electric_sensor[i].sw,
                    FLOAT_TO_INTS(g_multi_data.electric_sensor[i].voltage, 10),
                    FLOAT_TO_INTS(g_multi_data.electric_sensor[i].current, 100),
                    FLOAT_TO_INTS(g_multi_data.electric_sensor[i].power, 100),
                    FLOAT_TO_INTS(g_multi_data.electric_sensor[i].temp, 10),
                    (unsigned int)g_multi_data.electric_sensor[i].factor,
                    (unsigned long)g_multi_data.electric_sensor[i].energy);
                if (len > 0) {
                    mqtt_publish(MQTT_TOPIC_PUBLISH,
                        (const uint8_t *)json, (uint16_t)len, QOS_LEVEL, 0u);
                    led_toggle(LED_DATA);   /* 数据发送指示 */
                }
                g_electric_per_done[i] = 0;   /* 清除本设备 done 标志 */
            }
        }
    }

    /* ── 心跳保活（不变） ── */
    if (mqtt_get_state() == MQTT_STATE_CONNECTED) {
        uint32_t now = get_tick_ms();
        if ((now - last_ping_tick) >= (MQTT_KEEPALIVE_INTERVAL * 500u)) {
            mqtt_ping();
            last_ping_tick = now;
        }
    }
}
```

**添加外部变量声明**（如果 mqtt.c 中没有引入 modbus.h，需添加）：

```c
#include "modbus.h"    // 确保能访问 g_electric_per_done 和 CIRCUIT_SERSOR_NUM
```

> 注意：需删除 `g_multi_data.electric_done` 相关的旧逻辑和注释掉的温湿度/烟雾批量发布代码。

### 3.4 `user/main.c` — 主循环简化

```c
#include "stm32f10x.h"
#include "uart.h"
#include "delay.h"
#include "led.h"
#include "Net_USRK5.h"
#include <string.h>
#include "mqtt.h"
#include "max3485.h"
#include "modbus.h"
#include "public.h"


void on_mqtt_message(const char *topic, uint16_t topic_len,
                     const uint8_t *payload, uint16_t payload_len)
{
    max3485_U3_send_data((const uint8_t *)"[MQTT] topic: ", (uint16_t)strlen("[MQTT] topic: "));
    max3485_U3_send_data((const uint8_t *)topic, (uint16_t)strlen(topic));
    max3485_U3_send_data((const uint8_t *)"\r\n", 2u);
}


void bsp_Init(void) {
    delay_init();
    uart1_init();
    led_init();
    max3485_init();
    uart3_init();
    max3485_U3_init();

    if (usrk5_Net_init() != 0) {
        while (1) {
            max3485_U3_send_data("[USR-K5] init FAIL\r\n", strlen("[USR-K5] init FAIL\r\n"));
            led_toggle(LED_STATE);
            led_toggle(LED_DATA);
            delay_ms(2000);
        }
    }

    mqtt_init();
    mqtt_set_message_callback(on_mqtt_message);

    /* NEW: 初始化轮询状态机 */
    sensor_poll_init();

    delay_ms(500);
    led_set(LED_STATE, LED_OFF);
    led_set(LED_DATA, LED_OFF);
}


int main(void) {
    bsp_Init();

    if (mqtt_connect() == 0) {
        max3485_U3_send_data((const uint8_t *)"[MQTT] connected\r\n",
                             (uint16_t)strlen("[MQTT] connected\r\n"));
        mqtt_subscribe(MQTT_TOPIC_SUBSCRIBE, QOS_LEVEL);
    }

    while (1) {
#if CIRCUIT_SERSOR
        sensor_poll_step();     /* 推进一步：send / wait / parse / next */
#endif
        mqtt_loop();            /* 每轮都处理 MQTT + 逐设备上报 */

        /* 极小延时：兼顾其他 ISR 和 CPU 负载（可调 10~50ms） */
        delay_ms(10);
    }
}
```

**删除内容**：
- 删除 `#if TEMP_HUMI_SERSOR` 和 `#if SMOKING_SERSOR` 的空块（未来扩展时用状态机统一处理）
- `delay_ms(500)` 改为 `delay_ms(10)`

---

## 四、扩展性：未来加入温湿度/烟雾传感器

状态机框架天然支持多类型传感器。扩展时只需：

1. 在 `sensor_step_t` 中加 `SM_xxx_SEND`、`SM_xxx_WAIT_RECV` 等状态
2. 在 `sensor_poll_step()` 的 switch 中加对应处理分支
3. 轮询顺序：断路器 → 温湿度 → 烟雾，轮转

```c
// 扩展状态示例
typedef enum {
    SM_IDLE,
    SM_CKT_SEND, SM_CKT_WAIT, SM_CKT_PARSE,
    SM_TH_SEND,  SM_TH_WAIT,  SM_TH_PARSE,
    SM_SMK_SEND, SM_SMK_WAIT, SM_SMK_PARSE,
} sensor_step_t;
```

---

## 五、测试验证清单

| # | 验证项 | 方法 |
|---|--------|------|
| 1 | 编译通过 | `make clean && make` |
| 2 | 5 个断路器数据正常上报 | 接实际设备，观察 MQTT topic 每 ~3s 收到 5 条 JSON |
| 3 | 设备全部离线 | 串口输出 timeout 日志，MQTT 无数据但主循环不卡死 |
| 4 | 部分设备离线 | 在线设备数据正常上报，离线设备打印 timeout 后跳过 |
| 5 | MQTT 断开后重连 | 数据暂存（不丢），重连后继续上报 |
| 6 | 心跳保活正常 | 30s 间隔 PINGREQ/PINGRESP 正常 |
| 7 | LED 指示正常 | STATE LED 心跳（可加在主循环中 toggle），DATA LED 数据发送时闪 |

---

## 六、对另一个 AI 执行时的提示

1. **不使用 `%f` 格式化**：项目用 `nano.specs`，浮点数必须用 `FLOAT_TO_INTS(val, dec)` 宏拆分为整数对
2. **不引入动态内存**：所有缓冲区静态分配
3. **不修改 `library/` 和 `start/`**：这些是硬件抽象层，保持原样
4. **`modbus.h` 中 `static const` 数组**：如果编译器报重复定义，改为 `static const` 加 `#ifndef MODBUS_C` 条件，或在 `modbus.c` 中定义、`.h` 中用 `extern`
5. **构建验证**：`make -C /home/jelly/project/smarthome/sensor/MCU_485Gateway`（使用 `workdir` 参数）

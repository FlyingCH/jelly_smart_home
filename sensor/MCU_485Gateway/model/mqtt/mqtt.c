#include "mqtt.h"
#include "MQTTPacket.h"
#include "uart.h"
#include "delay.h"
#include "led.h"
#include <string.h>
#include <stdio.h>
#include "modbus.h"

/* ==================== 浮点转整数拆分宏 ==================== */
/* nano.specs 不支持 %f，用整数拆分实现定点数输出 */
#define FLOAT_TO_INTS(val, dec) \
    (int)(val), ((int)((val) * (float)(dec)) % (dec))

/* ==================== 内部宏定义 ==================== */

#define MQTT_TRANSPORT_TIMEOUT_MS   5000u
#define MQTT_RECV_POLL_MS           10u
#define MQTT_PACKET_ID_MAX          65535u

/* ==================== 内部类型 ==================== */

typedef enum {
    MQTT_PKT_CONNACK = 2,
    MQTT_PKT_PUBLISH = 3,
    MQTT_PKT_PUBACK   = 4,
    MQTT_PKT_SUBACK   = 9,
    MQTT_PKT_PINGRESP = 13
} mqtt_pkt_type_t;

/* ==================== 内部全局变量 ==================== */

static mqtt_state_t g_mqtt_state = MQTT_STATE_IDLE;
static mqtt_on_message_cb_t g_on_message_cb = (mqtt_on_message_cb_t)0;

static uint8_t  g_send_buf[MQTT_SEND_BUF_SIZE];
static uint8_t  g_recv_buf[MQTT_RECV_BUF_SIZE];
static char     g_publish_json_buf[256];
static uint16_t g_packet_id = 1u;

/* ==================== 内部函数声明 ==================== */

static int      _mqtt_transport_read(unsigned char *buf, int count);
static uint8_t  _mqtt_wait_first_byte(uint8_t expected_type, uint32_t timeout_ms);
static uint16_t _mqtt_get_next_packet_id(void);
static uint8_t  _mqtt_recv_flush(void);

/* ==================== 传输层实现 ==================== */

/**
  * @brief  从 USART2 环形缓冲区读取指定字节数（阻塞 + 超时）
  * @param  buf   : 输出缓冲区
  * @param  count : 期望读取字节数
  * @retval >0 : 实际读取字节数
  * @retval -1 : 超时或错误
  * @note   此函数作为 MQTTPacket_read 的 transport callback
  *         USR-K5 透传模式：串口数据 ← → 网络数据
  */
static int _mqtt_transport_read(unsigned char *buf, int count)
{
    uint16_t total = 0u;
    uint32_t waited = 0u;

    while (total < (uint16_t)count) {
        if (g_uart2_dev.rx_complete) {
            uint8_t raw[128];
            uint16_t raw_len = uart2_recv_data(raw, sizeof(raw));
            uart2_clear_rx_flag();

            if (raw_len > 0u) {
                uint16_t i;
                for (i = 0u; i < raw_len && total < (uint16_t)count; i++) {
                    buf[total++] = raw[i];
                }
            }
        }

        if (g_uart2_dev.rx_head != g_uart2_dev.rx_tail) {
            while (g_uart2_dev.rx_head != g_uart2_dev.rx_tail
                   && total < (uint16_t)count) {
                buf[total++] = g_uart2_dev.rx_buf[g_uart2_dev.rx_head];
                g_uart2_dev.rx_head = (g_uart2_dev.rx_head + 1u) % UART_RX_BUF_SIZE;
            }
        }

        if (total >= (uint16_t)count) {
            break;
        }

        delay_ms(MQTT_RECV_POLL_MS);
        waited += MQTT_RECV_POLL_MS;

        if (waited >= MQTT_TRANSPORT_TIMEOUT_MS) {
            return -1;
        }
    }

    return (int)total;
}

/**
  * @brief  清空 UART2 接收缓冲区中残留数据
  */
static uint8_t _mqtt_recv_flush(void)
{
    uart2_clear_rx_flag();
    g_uart2_dev.rx_head = g_uart2_dev.rx_tail;
    return 0u;
}

/* ==================== 包 ID 管理 ==================== */
static uint16_t _mqtt_get_next_packet_id(void)
{
    uint16_t id = g_packet_id;
    g_packet_id++;
    if (g_packet_id > MQTT_PACKET_ID_MAX) {
        g_packet_id = 1u;
    }
    return id;
}

/* ==================== 内部：等待指定类型 MQTT 报文 ==================== */

/**
  * @brief  等待 UART2 接收到指定 MQTT 报文类型的首字节
  * @param  expected_type : 期望的报文首字节 (CONNACK=0x20, SUBACK=0x90 等)
  * @param  timeout_ms    : 超时 ms
  * @retval 0 : 收到期望首字节
  * @retval 1 : 超时或类型不匹配
  * @note   此函数不清除接收缓冲区，后续由 MQTTPacket_read 完整读取报文
  */
static uint8_t _mqtt_wait_first_byte(uint8_t expected_type, uint32_t timeout_ms)
{
    uint8_t expected_header = expected_type << 4u;
    uint32_t waited = 0u;

    while (waited < timeout_ms) {
        if (g_uart2_dev.rx_complete) {
            if (g_uart2_dev.rx_head != g_uart2_dev.rx_tail) {
                uint8_t first_byte = g_uart2_dev.rx_buf[g_uart2_dev.rx_head];

                if (first_byte == expected_header) {
                    return 0u;
                }
                uart2_clear_rx_flag();
                g_uart2_dev.rx_head = g_uart2_dev.rx_tail;
            }
        }

        if (g_uart2_dev.rx_head != g_uart2_dev.rx_tail) {
            uint8_t first_byte = g_uart2_dev.rx_buf[g_uart2_dev.rx_head];

            if (first_byte == expected_header) {
                return 0u;
            }
            g_uart2_dev.rx_head = (g_uart2_dev.rx_head + 1u) % UART_RX_BUF_SIZE;
        }

        delay_ms(MQTT_RECV_POLL_MS);
        waited += MQTT_RECV_POLL_MS;
    }

    return 1u;
}

/* ==================== 对外接口 ==================== */

/**
  * @brief  初始化 MQTT 客户端状态
  */
void mqtt_init(void)
{
    g_mqtt_state    = MQTT_STATE_IDLE;
    g_on_message_cb = (mqtt_on_message_cb_t)0;
    g_packet_id     = 1u;

    (void)memset(g_send_buf, 0, sizeof(g_send_buf));
    (void)memset(g_recv_buf, 0, sizeof(g_recv_buf));
}

/**
  * @brief  连接 MQTT Broker
  * @retval 0 : 连接成功
  * @retval 1 : 连接失败（序列化错误 / 发送失败 / CONNACK 超时 / Broker 拒绝）
  */
uint8_t mqtt_connect(void)
{
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;
    int serialized_len;
    int packet_len;
    unsigned char session_present = 0u;
    unsigned char connack_rc = 0u;
    int deserialize_rc;

    g_mqtt_state = MQTT_STATE_CONNECTING;

    conn_data.MQTTVersion       = 4u;
    conn_data.clientID.cstring  = (char *)MQTT_CLIENT_ID;    // 客户端名称
    conn_data.keepAliveInterval  = MQTT_KEEPALIVE_INTERVAL;  // 心跳
    conn_data.cleansession      = 1u;                           
    conn_data.willFlag          = 0u;

#if MQTT_USE_USERNAME          // 如果登录需要使用用户名
    conn_data.username.cstring = (char *)MQTT_USERNAME;
    conn_data.password.cstring = (char *)MQTT_PASSWORD;
#endif
    // 序列化连接报文
    serialized_len = MQTTSerialize_connect(g_send_buf, (int)sizeof(g_send_buf), &conn_data);
    if (serialized_len <= 0) {     // 连接失败
        g_mqtt_state = MQTT_STATE_ERROR;
        return 1u;
    }

    _mqtt_recv_flush();   // 串口接收缓冲区清空
    uart2_send_hex(g_send_buf, (uint16_t)serialized_len);  // 发送连接报文

    if (_mqtt_wait_first_byte((uint8_t)MQTT_PKT_CONNACK, MQTT_TRANSPORT_TIMEOUT_MS) != 0u) {
        g_mqtt_state = MQTT_STATE_ERROR;
        return 1u;
    }

    packet_len = MQTTPacket_read(g_recv_buf, (int)sizeof(g_recv_buf),
                                  _mqtt_transport_read);
    if (packet_len <= 0) {
        g_mqtt_state = MQTT_STATE_ERROR;
        return 1u;
    }

    deserialize_rc = MQTTDeserialize_connack(
        &session_present, &connack_rc,
        g_recv_buf, packet_len);
    if (deserialize_rc != 1 || connack_rc != 0u) {
        g_mqtt_state = MQTT_STATE_ERROR;
        return 1u;
    }

    g_mqtt_state = MQTT_STATE_CONNECTED;
    return 0u;
}

/**
  * @brief  断开 MQTT 连接
  * @retval 0 : 成功
  * @retval 1 : 失败
  */
uint8_t mqtt_disconnect(void)
{
    int serialized_len;

    serialized_len = MQTTSerialize_disconnect(g_send_buf, (int)sizeof(g_send_buf));
    if (serialized_len <= 0) {
        return 1u;
    }

    uart2_send_hex(g_send_buf, (uint16_t)serialized_len);
    g_mqtt_state = MQTT_STATE_DISCONNECTED;
    return 0u;
}

/**
  * @brief  发布消息到指定 Topic
  * @param  topic       : Topic 字符串
  * @param  payload     : 消息内容
  * @param  payload_len : 消息长度
  * @param  qos         : QoS 等级 (0 或 1)
  * @param  retained    : 保留标志
  * @retval 0 : 成功
  * @retval 1 : 失败
  */
uint8_t mqtt_publish(const char *topic, const uint8_t *payload, uint16_t payload_len,
                     uint8_t qos, uint8_t retained)
{
    MQTTString mqtt_topic = MQTTString_initializer;
    int serialized_len;
    uint16_t packet_id = 0u;

    if ((topic == (const char *)0) || (payload == (const uint8_t *)0)) {
        return 1u;
    }
    if (g_mqtt_state != MQTT_STATE_CONNECTED) {
        return 1u;
    }

    mqtt_topic.cstring = (char *)topic;

    if (qos > 0u) {
        packet_id = _mqtt_get_next_packet_id();
    }

    serialized_len = MQTTSerialize_publish(
        g_send_buf, (int)sizeof(g_send_buf),
        0u, (int)qos, retained, packet_id,
        mqtt_topic,
        (unsigned char *)payload, (int)payload_len);

    if (serialized_len <= 0) {
        return 1u;
    }

    uart2_send_hex(g_send_buf, (uint16_t)serialized_len);
    return 0u;
}

/**
  * @brief  订阅 Topic
  * @param  topic : Topic 字符串
  * @param  qos   : 期望 QoS 等级
  * @retval 0 : 成功
  * @retval 1 : 失败
  */
uint8_t mqtt_subscribe(const char *topic, uint8_t qos)
{
    MQTTString mqtt_topic = MQTTString_initializer;
    int serialized_len;
    int requested_qos = (int)qos;
    uint16_t packet_id;
    int packet_len;
    unsigned short suback_pkt_id = 0u;
    int suback_count = 0;
    int granted_qos[1];

    if (topic == (const char *)0) {
        return 1u;
    }
    if (g_mqtt_state != MQTT_STATE_CONNECTED) {
        return 1u;
    }

    mqtt_topic.cstring = (char *)topic;
    packet_id = _mqtt_get_next_packet_id();

    serialized_len = MQTTSerialize_subscribe(
        g_send_buf, (int)sizeof(g_send_buf),
        0u, packet_id, 1,
        &mqtt_topic, &requested_qos);

    if (serialized_len <= 0) {
        return 1u;
    }

    _mqtt_recv_flush();
    uart2_send_hex(g_send_buf, (uint16_t)serialized_len);

    if (_mqtt_wait_first_byte((uint8_t)MQTT_PKT_SUBACK, MQTT_TRANSPORT_TIMEOUT_MS) != 0u) {
        return 1u;
    }

    packet_len = MQTTPacket_read(g_recv_buf, (int)sizeof(g_recv_buf),
                                  _mqtt_transport_read);
    if (packet_len <= 0) {
        return 1u;
    }
    if (MQTTDeserialize_suback(&suback_pkt_id, 1, &suback_count,
                                granted_qos, g_recv_buf, packet_len) != 1) {
        return 1u;
    }
    if (suback_count < 1 || granted_qos[0] == 0x80) {
        return 1u;
    }
    return 0u;
}

/**
  * @brief  发送 MQTT PINGREQ 保活心跳
  * @retval 0 : 成功收到 PINGRESP
  * @retval 1 : 失败（超时或收到非 PINGRESP 报文）
  * @note   直接使用 MQTTPacket_read 读取完整报文，避免 _mqtt_wait_first_byte
  *         破坏环形缓冲区中的其他数据
  */
uint8_t mqtt_ping(void)
{
    int serialized_len;

    if (g_mqtt_state != MQTT_STATE_CONNECTED) {
        return 1u;
    }

    serialized_len = MQTTSerialize_pingreq(g_send_buf, (int)sizeof(g_send_buf));
    if (serialized_len <= 0) {
        return 1u;
    }

    uart2_send_hex(g_send_buf, (uint16_t)serialized_len);

    /* 非阻塞：发送 PINGREQ 后立即返回，PINGRESP 由 mqtt_yield() 异步处理 */
    return 0u;
}

/**
  * @brief  MQTT 主循环调度：处理接收到的 PUBLISH 报文并分派回调
  * @note   需在主循环中定期调用（或收到 rx_complete 后调用）
  *         收到 PUBLISH 后自动发送 PUBACK (QoS 1)
  */
void mqtt_yield(void)
{
    int packet_len;
    int deserialize_rc;

    if (g_mqtt_state != MQTT_STATE_CONNECTED) {
        return;
    }

    if (!g_uart2_dev.rx_complete && g_uart2_dev.rx_head == g_uart2_dev.rx_tail) {
        return;
    }

    /* 只处理 PUBLISH 报文，其他类型（PINGRESP 等）留给 mqtt_ping() 处理 */
    if (g_uart2_dev.rx_buf[g_uart2_dev.rx_head] != ((uint8_t)MQTT_PKT_PUBLISH << 4u)) {
        return;   /* 不清除缓冲区，避免吞掉 PINGRESP */
    }

    packet_len = MQTTPacket_read(g_recv_buf, (int)sizeof(g_recv_buf),
                                  _mqtt_transport_read);

    if (packet_len > 0) {
        unsigned char dup = 0u;
        int qos = 0;
        unsigned char retained = 0u;
        unsigned short packet_id = 0u;
        MQTTString topic_name = MQTTString_initializer;
        unsigned char *payload = (unsigned char *)0;
        int payload_len = 0;

        deserialize_rc = MQTTDeserialize_publish(
            &dup, &qos, &retained, &packet_id,
            &topic_name, &payload, &payload_len,
            g_recv_buf, packet_len);

        if (deserialize_rc == 1) {
            if (qos == 1) {
                int puback_len = MQTTSerialize_puback(
                    g_send_buf, (int)sizeof(g_send_buf), packet_id);
                if (puback_len > 0) {
                    uart2_send_hex(g_send_buf, (uint16_t)puback_len);
                }
            }

            if (g_on_message_cb != (mqtt_on_message_cb_t)0) {
                g_on_message_cb(topic_name.cstring,
                                (uint16_t)MQTTstrlen(topic_name),
                                payload, (uint16_t)payload_len);
            }
        }
    }
}

/**
  * @brief  注册消息回调函数
  * @param  cb : 回调函数指针
  */
void mqtt_set_message_callback(mqtt_on_message_cb_t cb)
{
    g_on_message_cb = cb;
}

/**
  * @brief  获取当前 MQTT 客户端状态
  * @retval 当前状态枚举值
  */
mqtt_state_t mqtt_get_state(void){
    return g_mqtt_state;
}

// MQTT 主循环
void mqtt_loop(void){
    static uint32_t last_ping_tick = 0u;
    mqtt_yield();   /* 处理MQTT收发缓冲区,读取/分发入站报文 */

    /* 断路器/电力传感器数据上报：逐设备检查，有就发 */
    if (mqtt_get_state() == MQTT_STATE_CONNECTED) {
        uint8_t i;              
        for (i = 0; i < CIRCUIT_SERSOR_NUM; i++) {     // g_multi_data.electric_sensor[g_poll_ctx.dev_index]
            if (g_electric_per_done[i]) {
                int len = snprintf(g_publish_json_buf, sizeof(g_publish_json_buf),
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
                if (len > 0 && len < (int)sizeof(g_publish_json_buf)) {
                    mqtt_publish(MQTT_TOPIC_PUBLISH,
                        (const uint8_t *)g_publish_json_buf, (uint16_t)len, QOS_LEVEL, 0u);
                    led_toggle(LED_DATA);
                }
                g_electric_per_done[i] = 0;
            }
        }
    }

    /* 心跳保活：按实际毫秒计时，keepalive 间隔的一半发送 PINGREQ */
    if (mqtt_get_state() == MQTT_STATE_CONNECTED) {
        uint32_t now = get_tick_ms();
        if ((now - last_ping_tick) >= (MQTT_KEEPALIVE_INTERVAL * 500u)) {
            mqtt_ping();
            last_ping_tick = now;
        }
    }
}







#ifndef __MQTT_H
#define __MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

/* ==================== MQTT Broker 配置 ==================== */

#define MQTT_BROKER_IP         "192.168.31.213"    // MQTT服务器地址
#define MQTT_BROKER_PORT       1883               // MQTT端口
#define MQTT_CLIENT_ID         "STM32ELE_Gateway"  // MQTT客户端ID
#define MQTT_KEEPALIVE_INTERVAL 60u                // 心跳间隔，单位秒
#define MQTT_USE_USERNAME      0                   // 是否使用用户名密码认证
#define MQTT_USERNAME          ""                  // MQTT登录用户名   
#define MQTT_PASSWORD          ""                  // MQTT登录密码 
#define QOS_LEVEL              0


/* ==================== MQTT 发布和订阅的主题 ==================== */
/* 利用 C 语言相邻字符串字面量自动拼接的特性，使主题跟着 MQTT_CLIENT_ID 联动 */
#define _MQTT_TOPIC_PREFIX     "/Multisensor/"    // 统一设备前缀
#define _MQTT_TOPIC_DATA       "/data"            // 发布主题后缀
#define _MQTT_TOPIC_FUNC        "/func"           // 订阅主题后缀 
   

#define MQTT_TOPIC_PUBLISH     _MQTT_TOPIC_PREFIX MQTT_CLIENT_ID _MQTT_TOPIC_DATA   // 发布主题
#define MQTT_TOPIC_SUBSCRIBE   _MQTT_TOPIC_PREFIX MQTT_CLIENT_ID _MQTT_TOPIC_FUNC   // 订阅主题

/* ==================== MQTT 内部缓冲区 ==================== */

#define MQTT_SEND_BUF_SIZE     512u              // MQTT发送消息缓冲区
#define MQTT_RECV_BUF_SIZE     512u              // MQTT接收消息缓冲区
#define MQTT_TOPIC_MAX_LEN     64u               // MQTT最大主题长度
#define MQTT_PAYLOAD_MAX_LEN   256u              // MQTT最大有效负荷

/* ==================== MQTT 客户端状态 ==================== */

typedef enum {
    MQTT_STATE_IDLE = 0,
    MQTT_STATE_CONNECTING,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_ERROR
} mqtt_state_t;

/* ==================== 回调函数类型 ==================== */

typedef void (*mqtt_on_message_cb_t)(const char *topic, uint16_t topic_len,
                                     const uint8_t *payload, uint16_t payload_len);

/* ==================== 函数声明 ==================== */

void     mqtt_init(void);
uint8_t  mqtt_connect(void);
uint8_t  mqtt_disconnect(void);
uint8_t  mqtt_publish(const char *topic, const uint8_t *payload, uint16_t payload_len,
                      uint8_t qos, uint8_t retained);
uint8_t  mqtt_subscribe(const char *topic, uint8_t qos);
uint8_t  mqtt_ping(void);
void     mqtt_yield(void);

void     mqtt_set_message_callback(mqtt_on_message_cb_t cb);
mqtt_state_t mqtt_get_state(void);

void mqtt_loop(void);


#ifdef __cplusplus
}
#endif

#endif /* __MQTT_H */

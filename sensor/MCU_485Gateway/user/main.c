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

// 板级初始化
void bsp_Init(void){
  delay_init();              /* 初始化延时模块：SysTick 1ms + DWT us */
  uart1_init();             /* 初始化 USART1：9600-8-N-1，调试串口 */
  led_init();               /* 初始化 PA0/PA1 LED */
  max3485_init();          /* MAX3485发送控制引脚 */
  uart3_init();
  max3485_U3_init();
  
  if(usrk5_Net_init() != 0){    // USR-K5初始化
    while(1){ 
      max3485_U3_send_data((char*)"[USR-K5] init FAIL\r\n",strlen("[USR-K5] init FAIL\r\n"));
      led_toggle(LED_STATE);
      led_toggle(LED_DATA);
      delay_ms(2000);
    }
  }
  mqtt_init();  
  mqtt_set_message_callback(on_mqtt_message);

  /* 初始化轮询状态机 */
  sensor_poll_init();

  delay_ms(500);
  led_set(LED_STATE, LED_OFF);
  led_set(LED_DATA, LED_OFF);
}


/**
  * @brief  主函数入口
  * @note   初始化延时模块和 UART1，主循环每 5s 发送一次 TCP 数据包
  */
int main(void){    
    bsp_Init();
    // 初始化连接MQTT服务器
    if (mqtt_connect() == 0) {   
        max3485_U3_send_data((const uint8_t *)"[MQTT] connected\r\n", (uint16_t)strlen("[MQTT] connected\r\n"));
        mqtt_subscribe(MQTT_TOPIC_SUBSCRIBE, QOS_LEVEL);     // 订阅主题
    }

    while (1) {
#if CIRCUIT_SERSOR
      sensor_poll_step();     /* 推进一步：send / wait / parse / next */
#endif

      mqtt_loop();            /* 每轮都处理 MQTT + 逐设备上报 */
      
      /* 断线重连：MQTT 连接断开后尝试重新连接 */
      if (mqtt_get_state() != MQTT_STATE_CONNECTED) {
        max3485_U3_send_data((const uint8_t *)"[MQTT] reconnecting...\r\n", 23u);
          if (mqtt_connect() == 0) {
            mqtt_subscribe(MQTT_TOPIC_SUBSCRIBE, QOS_LEVEL);
            max3485_U3_send_data((const uint8_t *)"[MQTT] reconnected\r\n", 20u);
          }
      }
      delay_ms(500);
     }
}








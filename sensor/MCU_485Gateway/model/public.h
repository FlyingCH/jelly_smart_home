#ifndef __PUBLIC_H
#define __PUBLIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

#define   PRINT_DATA     1    // 串口3是否打印开启


#define    MULTI_SENSOR_SERSOR_NUM      50       // 传感器的最多数量

/**************打开网关使用的 传感器类型**************/ 
#define    TEMP_HUMI_SERSOR        0       // 温湿度传感器
#define    CIRCUIT_SERSOR          1       // 断路器(空开)设备
#define    SMOKE_SENSOR            0       // 烟雾传感器




/**************混合传感器各自的数量**************/
#if TEMP_HUMI_SERSOR
    #define TEMP_HUMI_SERSOR_NUM  NULL     // 温湿度传感器数量
#endif

#if CIRCUIT_SERSOR
    #define CIRCUIT_SERSOR_NUM      5       // 断路器数量
#endif

#if SMOKE_SENSOR
    #define SMOKE_SENSOR_NUM    NULL   // 烟雾传感器数量
#endif

/* 回退值：结构体 multi_data_t 中的数组维度需要这些宏始终有定义 */
#ifndef TEMP_HUMI_SERSOR_NUM
    #define TEMP_HUMI_SERSOR_NUM  1
#endif
#ifndef CIRCUIT_SERSOR_NUM
    #define CIRCUIT_SERSOR_NUM    5    // 断路器的数量
#endif
#ifndef SMOKE_SENSOR_NUM
    #define SMOKE_SENSOR_NUM      1
#endif






#ifdef __cplusplus
}
#endif

#endif 


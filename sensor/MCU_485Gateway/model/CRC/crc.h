#ifndef __CRC_H
#define __CRC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"


/******CRC校验计算******/
uint8_t  calCheckSumFunc(uint8_t const *pMsg, uint16_t uiDataLen);
uint16_t uiCrcCalFunc(uint8_t const *pMsg, uint16_t uiDataLen);
uint8_t  uCrc8Cal_Func(uint8_t *pBuffer, uint8_t ucLen);




#ifdef __cplusplus
}
#endif

#endif




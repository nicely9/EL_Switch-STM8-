/******************** (C) COPYRIGHT 2013 STMicroelectronics ********************/
/**
 * @file    : host_if.h
 * @brief   : HOST communication demonstration with private protocol
 * @author  : Smart Metering Team, IPG, GC&SA
 * @version : V1.0
 * @date    : 2013.MAY
 *
 **********************************************************************************
 *              (C) COPYRIGHT 2013 STMicroelectronics                        <br>
 * THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS  
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT, 
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE 
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING?
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *********************************************************************************/

#ifndef __ST_PRIME_HOST_IF_H_
#define __ST_PRIME_HOST_IF_H_

#ifdef __cplusplus
extern "C"{
#endif 

#include "st_hal_api.h"

typedef enum{
    STPM3x_FUNC_RESET = 0,
    STPM3x_FUNC_LATCH,
    STPM3x_FUNC_DIO,
    STPM3x_FUNC_READ,
    STPM3x_FUNC_WRITE,
    STPM3x_FUNC_SAVE_CALIB,
    STPM3x_FUNC_CLR_CNTR,
    STPM3x_FUNC_SAVE_ALL,

    STPM3x_FUNC_MAX
}ESTPM3xFunc;

typedef enum{
    STPM3x_TAR_STPM34  = 1,
    STPM3x_TAR_STPM33  = 2,
    STPM3x_TAR_BOTH    = 3,
    STPM3x_TAR_EEPROM  = 4,
    STPM3x_TAR_V3P     = 8,
    STPM3x_TAR_MEAS    = 12
}ESTPM3xTar;

//
// STPM application mode masks
//
#define STPM3x_MODE_DBG_MASK    (0x04)
#define STPM3x_MODE_DUMMY_MASK  (0x02)
#define STPM3x_MODE_SPI_MASK    (0x01)
#define STPM3x_MODE_TRANS_MASK  (0x01)
#define STPM3x_MODE_INVALID     (0xFF)

typedef enum{
    STPM3x_MODE_NORMAL          = 0,
    STPM3x_MODE_NORMAL_DBG      = STPM3x_MODE_DBG_MASK | STPM3x_MODE_NORMAL,    // 0x04
    STPM3x_MODE_TRANS           = STPM3x_MODE_TRANS_MASK,                       // 0x01
    STPM3x_MODE_TRANS_DBG       = STPM3x_MODE_DBG_MASK | STPM3x_MODE_TRANS,     // 0x05
    STPM3x_MODE_DUMMY_UART      = STPM3x_MODE_DUMMY_MASK,                       // 0x02
    STPM3x_MODE_DUMMY_UART_DBG  = STPM3x_MODE_DBG_MASK | STPM3x_MODE_DUMMY_UART,// 0x06
    STPM3x_MODE_DUMMY_SPI       = STPM3x_MODE_DUMMY_MASK | STPM3x_MODE_SPI_MASK,// 0x03
    STPM3x_MODE_DUMMY_SPI_DBG   = STPM3x_MODE_DBG_MASK | STPM3x_MODE_DUMMY_SPI  // 0x07
}ESTPM3xMode;

typedef enum{
    STPM3x_ERR_SUCC = 0,
    STPM3x_ERR_BAD_PARAM,
    STPM3x_ERR_IO_FAILED,
    STPM3x_ERR_NOT_SUPPORTED,
    STPM3x_ERR_IN_DUMMY,

    STPM3x_ERR_ERROR
}ESTPM3xError;

void  HOST_Init(void);
void  HOST_Fsm(void);

#ifdef __cplusplus
}
#endif 

#endif // __ST_PRIME_HOST_IF_H_

/************* (C) COPYRIGHT 2013 STMicroelectronics ***** END OF FILE *******/

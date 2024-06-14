#ifndef __RS_485_H
#define __RS_485_H

#ifdef	_HYUNDAI_PROTOCOL_
#include	"RS-485_HD.h"
#endif

#ifdef	_CVNET_PROTOCOL_
#include	"RS-485_CVNET.h"
#endif

#ifdef _KOCOM_PROTOCOL_
#include	"RS-485_KOCOM.h"
#endif

#ifdef _COMMAX_PROTOCOL_
#include	"RS-485_COMMAX.h"
#endif

#ifdef _SAMSUNG_PROTOCOL_
#include	"RS-485_SAMSUNG.h"
#endif

#ifdef _KDW_PROTOCOL_
#include	"RS-485_KDW.h"
#endif

#ifdef _HW_PROTOCOL_
#include	"RS-485_HW.h"
#endif

#ifdef _HDC_PROTOCOL_
#include	"RS-485_HDC.h"
#endif

#ifdef _NO_PROTOCOL_
#include	"RS-485_NO_PROTOCOL.h"
#endif

#define NIS_RX		0
#define NIS_TX		1
#define NIS_XOR_CRC	7
#define NIS_SUM_CRC 8

typedef enum
{
	NO_PROTOCOL				= 0x0,	// 프로토콜 없음
	HYUNDAI_PROTOCOL		= 0x1,	// 현대
	CVNET_PROTOCOL			= 0x2,	// CVNET
	KOCOM_PROTOCOL			= 0x3,	// KOCOM
	COMMAX_PROTOCOL			= 0x4,	// COMMAX
	SAMSUNG_PROTOCOL		= 0x5,	//SAMSUNG SDS
	KDW_PROTOCOL			= 0x6,	//경동원
	HW_PROTOCOL				= 0x7,	//하이퍼월
	HDC_PROTOCOL			= 0x8,	//HDC
	PROTOCOL_MAX
}_protocol_type_;

typedef enum
{
	ELEVATOR_NON	= 0,	// 0
	ELEVATOR_CALL,			// 호출
	ELEVATOR_CANCEL,		// 취소
	ELEVATOR_ARRIVE			// 도착
}_elevator_flag_;


extern __IO uint8_t	Gu8_RS_485_Rx_Tmr;
extern __IO uint8_t	Gu8_RS_485_Tx_Tmr;

extern uint8_t		Gu8_RS485_TX_Enable;

extern uint8_t	Gu8_Gas_Flag;
extern uint16_t	Gu16_GAS_Off_Tmr;

extern uint8_t	Gu8_ELEVATOR_Arrive_Flag;
extern uint16_t	Gu16_Elevator_Tmr;

extern uint8_t	Gu8_WallPad_Elevator_Call;		//엘리베이터 상태
extern uint8_t	Gu8_WallPad_Elevator_Floor;		//엘리베이터 현재 층
extern uint8_t	Gu8_WallPad_Elevator_Number;	//엘리베이터 호수

extern void irq_RS485_TX(void);
extern void irq_RS485_RX(void);
extern void RS485_Init(void);
extern void RS485_Process(void);


#endif

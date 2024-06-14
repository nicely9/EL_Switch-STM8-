#ifndef __RS_485_NO_H
#define __RS_485_NO_H

#define NO_MAX_BUF 10

#define NIS_LIGHT_ID_COMM_1		        0xDD		//생산시 프로토콜, ID 확인용 커맨드
#define NIS_LIGHT_ID_COMM_2             0xBB
#define NO_STX_1	0xF9
#define NO_STX_2	0x45
#define NO_ETX		0xEE

// #define THREEWAY_TRANS
#ifdef THREEWAY_TRANS
#define	NO_RES_DELAY_TIME	31		// 30ms 지나고 300ms 이내에 응답(제어 후 응답)
#define	NO_INTERVAL_TIME	6		// 5ms 지만 6ms로 설정

#define NO_REQ		0x01
#define NO_RES		0x04

#define NO_ON		0x01
#define NO_OFF		0x02

#define SET__THREEWAY_ON	0
#define SET__THREEWAY_OFF	1

extern uint16_t	Gu16_Noprotocol_Req_Tmr;

#endif

typedef enum
{
	NO_F_D_STX_1 	= 0,
	NO_F_D_STX_2,
	NO_F_D_0,				//REQ, RES
	NO_F_D_1,				//THREE STATE
	NO_F_D_2,
	NO_F_D_3,
	NO_F_D_4,
	NO_F_D_5,
	NO_F_FCC,
	NO_F_ETX
}_no_rx_frame_;

typedef struct
{
	uint8_t		buf[NO_MAX_BUF];
	uint16_t	count;
	uint8_t		send_flag;
}NO_BUF;

// ----------------------------------------------------------------------------------------

extern void Protocol_Data_Init(void);
extern void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr);
extern void RS485_Tx_Process(void);
extern void Protocol_Process(uint8_t data);


#endif
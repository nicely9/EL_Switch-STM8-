#ifndef __RS_485_HW_H
#define __RS_485_HW_H

// HW ----------------------------------------------------------------------------------
#define	HW_RES_DELAY_TIME	11				// 10ms 지나고 20ms 이내에 응답
#define	HW_INTERVAL_TIME	6				// 5ms 지만 6ms로 설정
//-----------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1					0xDD		//생산시 프로토콜, ID 확인용 커맨드
#define NIS_LIGHT_ID_COMM_2					0xBB
//--------------------------------------------------------------------------------------------
#define HW_MAX_BUF 255

typedef struct
{
	uint8_t		buf[HW_MAX_BUF];
	uint16_t	count;
	uint8_t		send_flag;
}HW_BUF;
// ----------------------------------------------------------------------------------------

extern void Protocol_Data_Init(void);
extern void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr);
extern void RS485_Tx_Process(void);
extern void Protocol_Process(uint8_t data);

//extern int Debug_SET_Switch_ID(int argc, char *argv[]);
extern uint8_t Get_485_ID(void);

#endif


#ifndef __RS_485_COMMAX_H
#define __RS_485_COMMAX_H

// 코맥스----------------------------------------------------------------------------------------
#define	COMMAX_RES_DELAY_TIME			36		// 최소 30ms ~ 최대 50ms
#define	COMMAX_INTERVAL_TIME			25		// 기준없음
//--------------------------------------------------------------------------------------------
#define COMMAX_OFF_FLAG					0x00
#define COMMAX_ON_FLAG					0x01
#define COMMAX_UNKNOWN_FLAG				0xFF
#define COMMAX_SET_COLOR_TEMP			0x02
#define COMMAX_SET_DIMMING				0x04
//--------------------------------------------------------------------------------------------
#define COMMAX_ON_n_OFF				0x01
#define COMMAX_DIMMING				0x02
#define COMMAX_COLOR_TEMP			0x04
#define COMMAX_MAX_DIM_LEVEL		0x08

#define COMMAX_BLOCK_MODE			0x02
#define COMMAX_STANDBY_VALUE_SAVE	0x03

#define COMMAX_AUTO_FLAG			0x01
#define COMMAX_MANUAL_FLAG			0x00

#define TYPE_USAGE					0x01
#define TYPE_STANDBY_VALUE_SAVE 	0x02
#define TYPE_USAGE_ONE_HOURS		0x03

#define COMMAX_GAS_STATE			0x01
#define COMMAX_3WAY_STATE			0x04
#define COMMAX_3WAY_INSTALL			0x08
//-------------------------------------------------------------------------------------------- Command 22의 동작요구
#define COMMAX_ELEVATOR_FAIL		0x80
#define COMMAX_ELEVATOR_SUCCESS		0x40
#define COMMAX_GAS_CLOSE_FAIL		0x20
#define COMMAX_GAS_CLOSE_SUCCESS	0x10
#define COMMAX_BATCH_LIGHT_ON		0x01
#define COMMAX_BATCH_LIGHT_OFF		0x00
//--------------------------------------------------------------------------------------------
#define	NIS_LIGHT_ID_COMM_1			0xDD		//생산시 프로토콜, ID 확인용 커맨드
#define NIS_LIGHT_ID_COMM_2			0xBB
//--------------------------------------------------------------------------------------------
//D1 
//층수는 최상위 비트에 따라 지상(0), 지하(1)로 구분. 나머지 비트는 현재 층수로 16진수로 표기함.
//지상 28층 -> 0x1C, 지하 28층 -> 0x9C. 즉, 지상은 16진수 바로 표기, 지하는 (1 << 7 | 16진수) -> (1 << 7 | 0x1C) = 0x9C

typedef enum
{
    COMMAX_GAS_COMM_REQUEST				=   0x10,		//사용안함
    COMMAX_GAS_COMM_CONTROL				=   0x11,		//사용안함
    
	COMMAX_LIGHT_COMM_REQUEST			=   0x30,
    COMMAX_LIGHT_COMM_CONTROL			=   0x31,
    COMMAX_LIGHT_COMM_GROUP_CONTROL		=   0x3F,
	
	COMMAX_ELEC_COMM_REQUEST			=	0x79,
	COMMAX_ELEC_COMM_CONTROL			=	0x7A,
	COMMAX_ELEC_COMM_GROUP_CONTROL		=	0x7B,

	COMMAX_BATCH_BLOCK_COMM_REQUEST		=	0x20,
	COMMAX_BATCH_LIGHT_COMM_CONTROL		=	0x21,		//기존 설치된 일괄 소등 스위치 용도로 사용
	COMMAX_BATCH_BLOCK_COMM_CONTROL		=	0x22,		//엘리베이터, 가스, 외출설정, 대기전력, 일괄소등
	COMMAX_ELEVATOR_COMM_CONTROL		=	0x23,		//엘리베이터	>> 엘리베이터 층 정보표시 기능 미지원 일 때 사용
	// COMMAX_BATCH_ELEVATOR_COMM_CONTROL_		=	0x26,	//엘리베이터	>> 엘리베이터 층 정보표시 기능 지원 일 때 사용
	COMMAX_BATCH_BLOCK_COMM_GROUP_CONTROL	=	0x2F	//일괄소등
	// COMMAX_BATCH_BLOCK_COMM_GROUP_CONTROL	=	0x2D,	//대기전력

}_MASTER_COMM_;
/*
<[01:11:23.286]20 01 00 00 00 00 00 21
<[01:11:23.324]A0 00 01 00 00 05 00 A6(스위치에서 일괄 소등 눌렀을 경우 응답) -> 월패드에서 그룹 제어로 전등 소등 byte_01
>> 스위치에서 일괄 소등을 누르면 월패드에서 전등 그룹제어로 일괄 소등. 그렇다면 그룹제어로 소등이 왔을 때 전등 상태 저장.

<[01:11:39.209]20 01 00 00 00 00 00 21
<[01:11:39.255]A0 01 01 00 00 05 00 A7(스위치에서 일괄 점등 눌렀을 경우 응답)
>> A0응답으로 일괄 점등으로 월패드에 데이터 보낼때 전등 스위치 일괄 점등.

<[01:14:44.504]22 01 01 01 00 00 00 25(월패드에서 일괄 점등 눌렀을 경우)
<[01:14:44.550]A2 01 01 00 00 00 00 A4
>> 22커맨드로 일괄 점등이 왔을때 전등 스위치 소등 전 저장된 상태로 일괄 점등.

<[01:15:09.239]22 01 00 01 00 00 00 24(월패드에서 일괄 소등 눌렀을 경우)	-> 월패드에서 그룹 제어로 전등 소등
<[01:15:09.270]A2 00 01 00 00 00 00 A3
>> 이 경우도 그룹제어로 소등이 왔을때 전등 상태 저장.
*/
//ACK Command = Request Command + 0x80
typedef enum
{
	COMMAX_GAS_COMM_REQUEST_RES			=	0x90,	//COMMAX_GAS_COMM_REQUEST + 0x80
	COMMAX_GAS_COMM_CONTROL_RES			=	0x91,	//COMMAX_GAS_COMM_CONTROL + 0x80
	COMMAX_LIGHT_COMM_REQUEST_RES		=	0xB0,	//COMMAX_LIGHT_COMM_REQUEST + 0x80
	COMMAX_LIGHT_COMM_CONTROL_RES		=	0xB1,	//COMMAX_LIGHT_COMM_CONTROL + 0x80
	COMMAX_ELEC_COMM_REQUEST_RES		=	0xF9,	//COMMAX_ELEC_COMM_REQUEST + 0x80
	COMMAX_ELEC_COMM_CONTROL_RES		=	0xFA,	//COMMAX_ELEC_COMM_CONTROL + 0x80

	COMMAX_BATCH_BLOCK_COMM_REQUEST_RES	=	0xA0,	//COMMAX_MULTISWITCH_COMM_REQUEST + 0x80
	COMMAX_BATCH_BLOCK_COMM_CONTROL_RES	=	0xA2	//COMMAX_BATCH_BLOCK_COMM_CONTROL + 0x80

}_DEVICE_COMM_;

typedef enum
{
	COMMAX_F_COMMAND 	= 0,
	COMMAX_F_BYTE_01,
	COMMAX_F_BYTE_02,
	COMMAX_F_BYTE_03,
	COMMAX_F_BYTE_04,
	COMMAX_F_BYTE_05,
	COMMAX_F_BYTE_06,
	COMMAX_F_CHECK_SUM,
	COMMAX_MAX_BUF
}_COMMAX_rx_frame_;

typedef enum								//Command 22 제어 요구에서 Byte03 제어의 종류
{
	COMMAX_VERIATY_BATCHLIGHT 				= 0x01,		//일괄 소등 On/Off 변경 요구
	COMMAX_VERIATY_ELEC,								//대기전력 On/Off 변경 요구
	COMMAX_VERIATY_OUTING_SUCCESS,						//외출 설정모드 진입 허가
	COMMAX_VERIATY_OUTING_FAIL,							//외출 진입 불가
	COMMAX_VERIATY_GAS_CLOSE_SUCCESS,					//가스 차단
	COMMAX_VERIATY_GAS_CLOSE_FAIL,						//가스 차단 불가
	COMMAX_VERIATY_ELEVATOR_CALL_SUCCESS,				//엘리베이터 호출 성공
	COMMAX_VERIATY_ELEVATOR_CALL_FAIL					//엘리베이터 호출 실패
}_COMMAX_control_veriaty_;

typedef struct
{
	uint8_t		buf[COMMAX_MAX_BUF+1];		//통신 프로그램 사용위해 임의로 버퍼 크기 +1
	uint16_t	count;
	uint8_t		send_flag;
}COMMAX_BUF;

// ----------------------------------------------------------------------------------------
#define	SET__GAS_CLOSE_REQUEST	1
#define	SET__GAS_CLOSE_STATE	2
#define	SET__GAS_OPEN_STATE		3
// #define	SET__ELEVATOR_BUTTON	4
#define SET__ELEVATOR_REQUEST	4
#define	SET__ELEVATOR_CALL		5
#define SET__ELEVATOR_ARRIVE	6
#define SET__ELEVATOR_CALL_FAIL	7
#define SET__BATCHLIGHT_OFF		8
#define SET__BATCHLIGHT_ON		9

extern void BATCH_BLOCK_Control(uint8_t control);

extern void Protocol_Data_Init(void);
extern void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr);
extern void RS485_Tx_Process(void);
extern void Protocol_Process(uint8_t data);

extern void Elevator_Process(void);
extern void ELEVATOR_Call(void);
extern void ELEVATOR_Cancel(void);
extern void BATCH_BLOCK_STATE_Process(void);
extern uint8_t Get_485_ID(void);

extern uint8_t Gu8_Batch_Toggle_Tmr;

// extern uint8_t Batch_Light_Use_Tmr;


// extern void COMMAX_Switch_Control_Process(void);

//-----------------------------------------------------------------------------------------

#endif
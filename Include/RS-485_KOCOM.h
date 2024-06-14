#ifndef __RS_485_KOCOM_H
#define __RS_485_KOCOM_H

// 코콤----------------------------------------------------------------------------------------
#define KOCOM_PREAMBLE_1		0xAA		//START CODE 1
#define KOCOM_PREAMBLE_2		0x55		//START CODE 2

#define KOCOM_HD				0x30		//HEADER

#define KOCOM_EOT_1				0x0D		//
#define KOCOM_EOT_2				0x0D		//

#define KOCOM_WALLPAD			0x01
#define KOCOM_WALLPAD_ID		0x00

#define KOCOM_LIGHT_DEVICE		0x0E		//전등스위치
#define KOCOM_ELEC_DEVICE		0x3B
#define KOCOM_USS_DEVICE		0x54

#define KOCOM_BATCH_DEVICE		0x90		//일괄스위치
#define KOCOM_GAS_DEVICE		0x2C		//일괄스위치 가스 
#define KOCOM_COOK_DEVICE		0x2D		//일괄스위치 쿡탑
#define KOCOM_ELEVATOR_DEVICE	0x44		//일괄스위치 엘리베이터 호출할때

#define KOCOM_MULTI_DEVICE		0x82		//다기능 스위치

#define KOCOM_3WAY_LIGHT_DEVICE	0x61		//전등 스위치 3로
#define KOCOM_3WAY_BATCH_DEVICE	0x62		//일괄 스위치 3로
/*
3로 스위치의 경우 세대기(0x01)에서 관리하지 않으며, 해당 장치기 간의 연동에만 사용됨
따라서 상태를 세대기에서 요청하는 경우가 없음.
CC값은 0x9C로만 처리하고 제어 요청만 처리함.
실제 릴레이 연결이 어느 쪽에 연결되어 있는지 관여하지 않으며, 해당 제어가 발생한 경우나
요청을 받은 경우 둘 다 다른 장치 쪽에 값을 전달 해야 한다.
*/
//--------------------------------------------------------------------------------------------
#define	KOCOM_RES_DELAY_TIME			11		// 10ms 지나고 250ms 이내에 응답
#define	KOCOM_INTERVAL_TIME				6		// 기준없음
#define	KOCOM_RESULT_SEND_TIME			251		// 개별 결과 전송시간
#define	KOCOM_GROUP_RESULT_SEND_TIME	251		// 그룹 결과 전송시간
#define	KOCOM_SINGLE_ID					0x01	// 개별제어
//--------------------------------------------------------------------------------------------
// LIGHT_PROTOCOL_VERSION	2.43	전등
#define	KOCOM_LIGHT_PROTOCOL_VERSION_H	2
#define	KOCOM_LIGHT_PROTOCOL_VERSION_L	43
// ELEC_PROTOCOL_VERSION	2.23	전열
#define KOCOM_ELEC_PROTOCOL_VERSION_H	2	//임시값
#define KOCOM_ELEC_PROTOCOL_VERSION_L	23	//임시값
// BATCH_PROTOCOL_VERSION	2.01	일괄
#define KOCOM_BATCH_PROTOCOL_VERSION_H	2
#define KOCOM_BATCH_PROTOCOL_VERSION_L	1
// MULTI_PROTOCOL_VERSION	2.43	다기능
#define KOCOM_MULTI_PROTOCOL_VERSION_H	1
#define KOCOM_MULTI_PROTOCOL_VERSION_L	6
//--------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1		0xDD		//생산시 프로토콜, ID 확인용 커맨드
#define NIS_LIGHT_ID_COMM_2		0xBB
//--------------------------------------------------------------------------------------------
#define	KOCOM_LOWEST_ID			0x00		// 장치기가 설정할 수 있는 가장 낮은 ID
#define	KOCOM_GROUP_ID			0xFF		// 그룹제어
#define	KOCOM_CC_NOT_ACK		0x9C		// 
#define	KOCOM_CC_REQ			0xB0		// CC REQ	(0xBC, 0xBD, 0xBE)
#define	KOCOM_CC_ACK			0xD0		// CC ACK	(0xDC, 0xDD, 0xDE)
#define	KOCOM_CC_RESULT			0xBC		/* CC Result(0xBC, 0xBD, 0xBE) */

#define	KOCOM_OFF_FLAG			0x00
#define	KOCOM_ON_FLAG			0xFF
#define KOCOM_UNKOWN_FLAG		0x01

#define KOCOM_NO_CIRCUIT		0x00		//회로 없을때
#define KOCOM_LIGHT_CIRCUIT		0x01		//단순 ON/OFF 회로일때

#define KOCOM_BATCH_OFF_STATE	0x63		//일괄소등 상태
#define KOCOM_BATCH_ON_STATE	0x64		//일괄점등 상태

/*#define KOCOM_ELEC_OFF_STATE	0x65		//콘센트 일괄 OFF 상태
#define KOCOM_ELEC_ON_STATE		0x66		//콘센트 일괄 ON 상태*/

#define KOCOM_BLOCK_FLAG			0x01	//차단(가스, 쿡탑)
#define KOCOM_BLOCK_RELEASE_FLAG	0x00	//차단해제(가스, 쿡탑)

//D0
#define KOCOM_ELEVATOR_STOP			0x00	//엘리베이터 정지 상태
#define KOCOM_ELEVATOR_DOWNWARD		0x01	//엘리베이터 하향
#define KOCOM_ELEVATOR_UPWARD		0x02	//엘리베이터 상향
#define KOCOM_ELEVATOR_ARRIVE		0x03	//엘리베이터 도착

//D1 
//층수는 최상위 비트에 따라 지상(0), 지하(1)로 구분. 나머지 비트는 현재 층수로 16진수로 표기함.
//지상 28층 -> 0x1C, 지하 28층 -> 0x9C. 즉, 지상은 16진수 바로 표기, 지하는 (1 << 7 | 16진수) -> (1 << 7 | 0x1C) = 0x9C

typedef enum
{
	KOCOM_OP_LIGHT_CONTROL			=	0x00,	//제어
	KOCOM_OP_3WAY_CONTROL			=	0x00,
	KOCOM_OP_STATE_REQ				=	0x3A,	//상태요구
	KOCOM_OP_BATCH_LIGHT_REQ		=	0x62,	//일괄제어 후 상태요구
	KOCOM_OP_BATCH_LIGHT_STATE_OFF	=	0x63,	//일괄소등 상태 값
	KOCOM_OP_BATCH_LIGHT_STATE_ON	=	0x64,	//일괄점등 상태 값
	KOCOM_OP_BATCH_LIGHT_OFF		=	0x65,	//일괄소등 명령
	KOCOM_OP_BATCH_LIGHT_ON			=	0x66,	//일괄점등 명령
	
	KOCOM_OP_PROTOCOL_VERSION		=	0x4A,
	KOCOM_OP_DIM_LEVEL				=	0x5A,		//조명 장치의 디밍 단계를 조회
	KOCOM_OP_COLOR_TEMP_CONTROL		=	0x01,		//색온도 제어
	KOCOM_OP_COLOR_TEMP_REQ			=	0x3B,		//색온도 상태조회
	KOCOM_OP_COLOR_TEMP_LEVEL		=	0x5B,		//조명 장치의 색온도 최고 단계를 조회

	KOCOM_OP_GAS_STATE_REQ			=	0x3A,
	KOCOM_OP_GAS_BLOCK				=	0x02,
	KOCOM_OP_GAS_BLOCK_RELEASE		=	0x01,
	KOCOM_OP_COOK_STATE_REQ			=	0x3A,
	KOCOM_OP_COOK_BLOCK				=	0x02,
	KOCOM_OP_COOK_BLOCK_RELEASE		=	0x01,

	KOCOM_OP_ELEVATOR_CALL			=	0x01,
	KOCOM_OP_ELEVATOR_CALL_FAIL		=	0xFF,
	//------------------------------------------------------------콘센트
	KOCOM_OP_BATCH_ELEC_OFF			=	0x65,
	KOCOM_OP_BATCH_ELEC_ON			=	0x66,
	KOCOM_OP_ELEC_STATE_REQ			=	0x3A,
	KOCOM_OP_ELEC_CONTROL			=	0x00,
	//------------------------------------------------------------다기능스위치
	KOCOM_OP_IMPLEMENT_INFO			=	0x6A,		//구현기능 정보
	KOCOM_OP_WEATHER_INFO			=	0x01,		//날씨 정보
	KOCOM_OP_ENERGY_USAGE			=	0x02,		//에너지 사용량
	KOCOM_OP_MULTI_SWITCH_CONTROL	=	0x03,		//다기능 스위치 제어
	KOCOM_OP_PARCEL					=	0x04,		//택배
	KOCOM_OP_PARKING_INFO			=	0x05,		//주차 정보
	KOCOM_OP_OUTING_n_SECURITY		=	0x06,		//외출/보안
	KOCOM_OP_ENERGY_SPECIES			=	0x07,		//에너지 종수
	KOCOM_OP_DUST_INFO				=	0x08,		//미세먼지 정보
	KOCOM_OP_PARKING_DETAIL_INFO	=	0x50		//주차 상세 정보
}_OPERATION_CODE_;

typedef enum
{
	KOCOM_CMD_REQ_BC				=	0xBC,
	KOCOM_CMD_REQ_BD				=	0xBD,
	KOCOM_CMD_REQ_BE				=	0xBE,
	KOCOM_CMD_RES					=	0xDC,
	KOCOM_CMD_NON_ACK				=	0x9C		//사용하면 ACK 없이 결과 데이터만 전송.
}_KOCOM_CMD_;

typedef enum
{
	KOCOM_F_PRE_1 	= 0,
	KOCOM_F_PRE_2,
	KOCOM_F_HD,
	KOCOM_F_CC,
	KOCOM_F_PCNT,
	KOCOM_F_ADH,
	KOCOM_F_ADL,
	KOCOM_F_ASH,
	KOCOM_F_ASL,
	KOCOM_F_OPCODE,
	KOCOM_F_DATA_0,
	KOCOM_F_DATA_1,
	KOCOM_F_DATA_2,
	KOCOM_F_DATA_3,
	KOCOM_F_DATA_4,
	KOCOM_F_DATA_5,
	KOCOM_F_DATA_6,
	KOCOM_F_DATA_7,
	KOCOM_F_FCC,
	KOCOM_F_EOT_1,
	KOCOM_F_EOT_2,
	KOCOM_MAX_BUF
}_kocom_rx_frame_;

typedef struct
{
	uint8_t		buf[KOCOM_MAX_BUF];
	uint16_t	count;
	uint8_t		SEND_Flag;
	uint8_t		Result_SEND_Flag;
	uint8_t		Result_SEND_GROUP_Flag;
	uint8_t		Result_SEND_OP_Code;
	uint8_t		Result_SEND_CC_Count;
	uint8_t		Result_SEND_Event_Flag;	
}KOCOM_BUF;

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

extern uint8_t Batch_Light_Use_Tmr;
extern uint8_t Touch_Use_Tmr;
extern _BLOCK_FLAG_ Block_Active_Flag;
extern uint8_t Gu8_3Way_Control_Flag;
extern uint16_t Gu8_RS_485_Tx_Add_Tmr;
/*
a) Preamble : 첫 번째(0xAA), 두 번째(0x55) 패킷 시작을 알리는 Start Code
b) HD : 프로토콜의 용도 구분
c) CC : Control Command 재전송 카운트 및 명령 또는 ACK에 대한 구분
d) PCNT : 연속데이터 전송시, 남아있는 패킷 수
e) ADH / ADL : 목적지 상/하 바이트
f) ASH / ASL : 출발지 상/하 바이트
g) OP : Operation Code 명령 코드
h) FCC : Check Sum 체크섬(HD + ……… + D7 결과의 하위 1바이트 값)
i) EOT : 마지막 2바이트(0x0D) 패킷 마지막을 End Code

세대기에서 조명을 제어할 때, OP-Code 가 0x00 이면 모든 회로가 제어되어야 한다.
제어가 완료되어 조명이 세대기에 결과값을 전송할 때 OP-Code 는 0x00 로 하여 D0~D7 에 전체 회로 정보를 반영하여 전송한다.

# 30bc(요청) + 003a(조회) = 조회 명령어
# 30bc(요청) + 0000(제어) = 제어 명령어 (turn of/on)
# 30dc(응답) + 0000(제어) = 제어 명령어 대한 응답
# 30dc(응답) + 003a(조회) = 조회 명령어에 대한 응답

1. 조명 제어 (거실 조명 1번 켜기)
AA 55 30 BC 00 0E 00 01 00 00 FF 00 00 00 00 00 00 00 FA 0D 0D 커맨드(사용자 입력) (월패드->거실(0번방)의 조명1을 켜라(ff)보냄
AA 55 30 DC 00 01 00 0E 00 00 FF 00 00 00 00 00 00 00 1A 0D 0D 응답(기기간 자동) (거실(0번방) -> 월패드 조명1을 켜라 신호 받았음 ACK
AA 55 30 BC 00 01 00 0E 00 00 FF 00 00 00 00 00 00 00 FA 0D 0D 응답(기기간 자동) (거실(0번방) -> 월패드 조명1 켜졌음 상태 보냄
AA 55 30 DC 00 0E 00 01 00 00 FF 00 00 00 00 00 00 00 1A 0D 0D 응답(기기간 자동) (월패드->거실(0번방)의 조명1을 켜졌다 상태 받았음 ACK

2. 상태 확인 (1Byte 데이터)
AA 55 30 BC 00 2C 00 01 00 3A 00 00 00 00 00 00 00 00 53 0D 0D 커맨드(사용자 입력) (월패드->가스밸브제어기) 상태요청(3A) 보냄
AA 55 30 DC 00 01 00 2C 00 3A 00 00 00 00 00 00 00 00 73 0D 0D 응답(기기간 자동) (가스밸브제어기->월패드) 상태요청 받았음 ACK
AA 55 30 BC 00 01 00 2C 00 02 00 00 00 00 00 00 00 00 1B 0D 0D 응답(기기간 자동) (가스밸브제어기->월패드) '닫힘'(02) 보냄
AA 55 30 DC 00 2C 00 01 00 02 00 00 00 00 00 00 00 00 3B 0D 0D 응답(기기간 자동) (월패드->가스밸브제어기) '닫힘'상태 받았음 ACK

*/
//-----------------------------------------------------------------------------------------

#endif
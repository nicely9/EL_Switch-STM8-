#ifndef __RS_485_HD_H
#define __RS_485_HD_H

// 현대통신 ---------------------------------------------------------------------------------
#define	HYUNDAI_RES_DELAY_TIME	31		// 30ms 지나고 300ms 이내에 응답(제어 후 응답)
#define	HYUNDAI_INTERVAL_TIME	6		// 5ms 지만 6ms로 설정

#define HYUNDAI_WALLPAD_ID		0x01
#define HYUNDAI_STX				0xF7
#define HYUNDAI_VEN				0x01
#define HYUNDAI_ETX				0xEE
#define HD_MAX_BUF				255
#ifdef _HYUNDAI_PROTOCOL_
#define SWITCH_NUM				pG_Config->RS485_ID
#endif
//--------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1		0xDD		//생산시 프로토콜, ID 확인용 커맨드
#define NIS_LIGHT_ID_COMM_2		0xBB
//--------------------------------------------------------------------------------------------
#define HYUNDAI_ON_FLAG				0x01
#define HYUNDAI_OFF_FLAG			0x02
#define HYUNDAI_ON_n_OFF			0x01
#define HYUNDAI_DIMMING				0x02

#define HYUNDAI_AUTO_MODE			0x04
#define WATT_MSB					0x01
#define WATT_LSB					0x02

typedef enum
{
	HD_DEV_USS					= 0x2A,		//일괄차단
	HD_DEV_LIGHT				= 0x19,		//일괄소등
	HD_DEV_LIGHT_DIMMING		= 0x1A,		//디밍
	HD_DEV_STANDBY_POWER		= 0x2C,		//대기전력
	HD_DEV_ELETRICITY			= 0x1F,		//콘센트
	HD_DEV_GAS					= 0x1B,		//가스
	HD_DEV_HEATER				= 0x18,		//난방기
	HD_DEV_OUTING				= 0x17,		//외출
	HD_DEV_SECURITY				= 0x16,		//방범
	HD_DEV_ELEVATOR_CALL		= 0x34,
	HD_DEV_EXTERNAL_CONTACT		= 0x48,

	HD_SRV_POWER				= 0x40,		//전원
	HD_SRV_DIMMING				= 0x42,		//밝기조절
	HD_SRV_BATCHLIGHT			= 0x70,		//일괄제어
	HD_SRV_OPEN_n_CLOSE			= 0x43,		//개폐
	HD_SRV_POWERSAVING_MODE 	= 0x57,
	HD_SRV_ELEVATOR_UP_n_DOWN 	= 0x41
	// HD_SRV_HEATER			= 0x46,		//난방운전 설정
	// HD_SRV_SECURITY			= 0x60,		//방범 설정
}_hyundai_packet_;

typedef enum
{
	HD_TYPE_STATE_REQ		= 0x01,
	HD_TYPE_CONTROL_REQ		= 0x02,
	HD_TYPE_RES_SUCCESS		= 0x04
}_hyundai_res_;

typedef enum
{
	HD_CMD_ON				= 0x01,
	HD_CMD_OFF				= 0x02,
	HD_CMD_BLOCK			= 0x01,		//차단
	HD_CMD_UNBLOCK			= 0x02,		//차단해제
	HD_CMD_CLOSE			= 0x03,		//가스
	HD_CMD_OPEN				= 0x04,		//가스
	HD_CMD_UP				= 0x05,		//엘리베이터
	HD_CMD_DOWN				= 0x06		//엘리베이터

	// HD_CMD_HEATER_ON		= 0x01,		//난방기 가동
	// HD_CMD_HEATER_OFF		= 0x04,		//난방기 정지
	// HD_CMD_SECURITY_ON		= 0x07,		//방범 설정
	// HD_CMD_SECURITY_OFF		= 0x08,		//방범 해제
	// HD_CMD_OUTING_DELAY		= 0x0A,		//외출 지연
	// HD_CMD_ENTRANCE_DELAY	= 0x0B,		//입장 지연	
}_hyundai_cmd_;


typedef enum
{
	HD_ARG_NORMAL					= 0x00,
	HD_ARG_ARRIVE					= 0x01,
	HD_ARG_ERR						= 0x02,
	HD_ARG_CALL						= 0x03,		//사용 안함
	HD_ARG_CALL_UP					= 0x05,		//상향 호출
	HD_ARG_CALL_DOWN				= 0x06,		//하향 호출
	HD_ARG_RUNNING_UP				= 0xA0,		//상향 운행
	HD_ARG_RUNNING_DOWN				= 0xB0,		//하향 운행
	HD_ARG_RUNNING_UP_n_CALL_UP		= 0xA5,		//상향운행 상향 호출
	HD_ARG_RUNNING_UP_n_CALL_DOWN	= 0xA6,		//상향운행 하향 호출
	HD_ARG_RUNNING_DOWN_n_CALL_UP 	= 0xB5,		//하향운행 상향 호출
	HD_ARG_RUNNING_DOWN_n_CALL_DOWN = 0xB6,		//하향운행 하향 호출
	HD_ARG_EXTERNAL_CONTACT			= 0x71
	// 층수 지하 : -1(0xB1) ~ -15(0xBF)
	// 층수 지상 : 0(0x00) ~ 99(0x99)
	// 호기 : 0x01 ~ 0xFF, 0x00(미지정)
}_hyundai_elevator_;

typedef enum
{
	HD_LOC_CIRCUIT_GROUP_ALL	= 0x00,
	HD_LOC_CIRCUIT_ALL			= 0,		//1번 스위치 전체 제어
	HD_LOC_CIRCUIT_1,						//1번 스위치 1 회로 제어
	HD_LOC_CIRCUIT_2,						//1번 스위치 2 회로 제어
	HD_LOC_CIRCUIT_3,						//1번 스위치 3 회로 제어
	HD_LOC_CIRCUIT_4,						//1번 스위치 4 회로 제어
	HD_LOC_CIRCUIT_5,
	HD_LOC_CIRCUIT_6
}_hyundai_req_;

typedef enum
{
	HD_F_STX 	= 0,
	HD_F_LEN,
	HD_F_VEN,
	HD_F_DEV,
	HD_F_TYPE,
	HD_F_SRV,
	HD_F_LOC,
	HD_F_CMD,
	HD_F_ARG_1,
	HD_F_ARG_2,
	HD_F_ARG_3,
	HD_F_ARG_4,	
	HD_F_ARG_5,
	HD_F_ARG_6,
	HD_F_ARG_7,
	HD_F_ARG_8,
	HD_F_ARG_9,
	HD_F_ARG_10,
	HD_F_ARG_11,
	HD_F_ARG_12,
	HD_F_ARG_13,
	HD_F_ARG_14,
	HD_F_ARG_15,
	HD_F_ARG_16,
	HD_F_ARG_17,
	HD_F_ARG_18
}_hyundai_rx_frame_;

typedef enum
{
	HD_T_CRC_9	= 9,
	HD_T_ETX_10,
	HD_T_MAX_11,

	HD_T_CRC_10 = 10,
	HD_T_ETX_11,
	HD_T_MAX_12,

	HD_T_CRC_11 = 11,
	HD_T_ETX_12,
	HD_T_MAX_13,

	HD_T_CRC_12 = 12,
	HD_T_ETX_13,
	HD_T_MAX_14,

	HD_T_CRC_13 = 13,
	HD_T_ETX_14,
	HD_T_MAX_15,

	HD_T_CRC_14 = 14,
	HD_T_ETX_15,
	HD_T_MAX_16,

	HD_T_CRC_15 = 15,
	HD_T_ETX_16,
	HD_T_MAX_17,

	HD_T_CRC_16 = 16,
	HD_T_ETX_17,
	HD_T_MAX_18,

	HD_T_CRC_17 = 17,
	HD_T_ETX_18,
	HD_T_MAX_19,

	HD_T_CRC_20 = 20,
	HD_T_ETX_21,
	HD_T_MAX_22,

	HD_T_CRC_26 = 26,
	HD_T_ETX_27,
	HD_T_MAX_28
}_hyundai_tx_frame_;

typedef struct
{
	// uint8_t word;
	uint8_t value;
}_length_;


typedef struct
{
	uint8_t		buf[HD_MAX_BUF];//수정
	uint16_t	count;
	_length_	length;
	uint8_t		send_flag;
}HYUNDAI_BUF;

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
extern uint8_t Gu8_External_Flag;
// extern void Gas_Process(void);

// HYUNDAI ----------------------------------------------------------------------------------
/*
	* 30ms 지나고 300ms 이내에 응답
	* Interval time은 5ms
	------------------------------------------------------------------------------------------
	STX | LEN | VEN | DEV | TYPE | SRV | LOC | CMD | ARG1 ~ ARGn | CRC | ETX
	------------------------------------------------------------------------------------------
    STX
    0xF7로 고정

    LEN
    STX ~ ETX까지의 Packet 길이
    
    VEN
    0x01

    DEV
    일반 조명       0x19
    디밍 조명       0x1A

    TYPE
    제어 요청에 대한 응답은 제어 기능 수행 이후 응답한다(단 ,응답시간이 300ms를 초과하여서는 안됨)
    상태요구             0x01       (명령사용 : 현대통신)
    제어명령             0x02       (명령사용 : 현대통신)
    Response Success    0x04       (명령사용 : 기기업체)
    
    SRV
    전원            0x40
    밝기 조절       0x42
    일괄제어        0x70

    LOC
    상위 니블 : 그룹 넘버, 하위 니블 : 그룹 내 기기ID
    상위 니블은 물리적인 조명 스위치를 의미
    하위 니블은 해당 조명 스위치의 회로를 의미
    각 니블은 1 ~ 6 까지 사용
    
    전체 그룹 조명 제어 시 : 상위 니블인 그룹 넘버를 0으로 제어
    해당 그룹 전체 제어 시 : 하위 니블인 기기 ID를 0으로 제어
    * 예시 0x11(1번  스위치의 1번 회로 : 11, 2번 회로 : 12)

	CMD
    ON              0x01
    OFF             0x02
    NUMBER          0 ~ 7

    ARG
    전체 상태 응답 시 해당 구수 별로 ARG 개수 가변
    ARG는 BYTE로 회로 상태 표시.
    ON : 0x01, OFF : 0x02
    1구일 경우 전체 상태 응답시 1BYTE, 6구일 경우 전체 상태 응답 시 6BYTE
    상태 응답 시 현재 상태 표시

    CRC
    STX ~ CRC전까지의 Exclusive OR

    ETX
    0xEE로 고정
*/

#endif


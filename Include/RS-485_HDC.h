#ifndef __RS_485_HDC_H
#define __RS_485_HDC_H

// HDC ---------------------------------------------------------------------------------
#define	HDC_RES_DELAY_TIME					11
#define	HDC_INTERVAL_TIME					6

#define HDC_STX								0x02

#define HDC_MAX_LEN							64

#define HDC_BASE_REQ_LEN					0x06
#define HDC_STATE_REQ_LEN					0x06

#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
#define HDC_CMD_STATE_REQ					0x00
#define HDC_CMD_STATE_RELEASE_REQ			0x01
#define HDC_CMD_MODE_CONTROL_REQ			0x04
#define HDC_CMD_GAS_STATE_REQ				0x06
#define HDC_CMD_ELEVATOR_STATE_REQ			0x07
#define HDC_CMD_VERSION_INFO_REQ			0x21

#define PROTOCOL_VERSION_MSB				0x02
#define PROTOCOL_VERSION_LSB				0x04
#else
#define HDC_CMD_BASE_REQ					0x01	//기본 정보
#define HDC_CMD_STATE_REQ					0x11	//상태 정보
#define HDC_CMD_CONTROL_REQ					0x12	//상태 제어
#define HDC_CMD_TIME_INFO_REQ				0x21	//시간 정보 알림

#define HDC_CMD_BASE_V2_REQ					0x31
#define HDC_CMD_STATE_V2_REQ				0x32
#define HDC_CMD_CONTROL_RECEIVCE_V2_REQ		0x33
#define HDC_CMD_CONTROL_V2_REQ				0x34

#define PROTOCOL_VERSION_MSB				0x01
#define PROTOCOL_VERSION_LSB				0x09

#endif

#define HDC_MSB								0
#define HDC_LSB								1

#define HDC_DATE_YEAR						0
#define HDC_DATE_MONTH						1
#define HDC_DATE_DAY						2

#define HDC_ON_FLAG							1
#define HDC_OFF_FLAG						0

#define HDC_BATCHLIGHT_UNKNOWN				0x00
#define HDC_BATCHLIGHT_BLOCK				0x01	//전등용
#define HDC_BATCHLIGHT_RELEASE				0x02	//전등용

#define HDC_ELEC_ON							0x01
#define HDC_ELEC_OFF						0x02
#define HDC_ELEC_AUTO						0x10
#define HDC_ELEC_NORMAL						0x20
#define HDC_ELEC_BLOCK_VALUE				0x40

#define HDC_ELEC_ON_OFF_BIT					(HDC_ELEC_ON|HDC_ELEC_OFF)
#define HDC_ELEC_MODE_BIT					(HDC_ELEC_AUTO|HDC_ELEC_NORMAL)

#define HDC_BATCH_LIGHT_OFF					0x00
#define HDC_BATCH_LIGHT_ON					0x01
// #define HDC_LIGHT_EXPANSION					0x02
#define HDC_REVERSED						(0x02|0x04|0x08|0x10)	//조명확장(0x02), 대기전력(0x10), X(0x04, 0x08)
// #define HDC_STANDBY_POWER					0x10			
#define HDC_GAS_CLOSE						0x20
#define HDC_ELEVATOR_DOWN					0x40
#define HDC_ELEVATOR_UP						0x80
#define HDC_ELEVATOR_CALL					(HDC_ELEVATOR_UP|HDC_ELEVATOR_DOWN)

#define FROM_SWITCH							0x01
#define FROM_WALLPAD						0x02

#define NIS_LIGHT_ID_COMM_1					0xDD
#define NIS_LIGHT_ID_COMM_2					0xBB
#define CRC_SUM								0
#define CRC_XOR								1

#define HDC_MAX_BUF				255




typedef enum
{
    BATCH_BLOCK_ID          = 0x15, //일괄
    BATCH_BLOCK_n_GAS_ID    = 0x15, //일괄, 가스
    BATCH_BLOCK_n_GAS_EV_ID = 0x17, //일괄, 가스, 엘베
    MULTI_SWITCH_ID         = 0x50 //전등, 대기 등 0x51 ~ 
}_hdc_id_;

#if defined _ONE_SIZE_BATCH_BLOCK_MODEL_
typedef enum
{
	HDC_F_STX 	= 0,	//프레임 시작
	HDC_F_ID,			//디바이스 ID
    HDC_F_CMD,			//패킷 종류 구분
	HDC_F_SEQ,			//패킷의 일련번호 월패드에서 부여
	HDC_F_D1,			//가변  길이의 데이터 필드.
    HDC_F_D2,			//가변  길이의 데이터 필드.
    HDC_F_D3,			//가변  길이의 데이터 필드.
    HDC_F_D4,			//가변  길이의 데이터 필드.
    HDC_F_D5,			//가변  길이의 데이터 필드.
    HDC_F_CS,			//CheckSum
	HDC_F_MAX_BUF
}_hdc_batchblock_frame_;
#else
typedef enum
{
	HDC_F_STX 	= 0,	//프레임 시작
	HDC_F_ID,			//디바이스 ID
	HDC_F_LEN,			//전체 패킷 길이
    HDC_F_CMD,			//패킷 종류 구분
	HDC_F_SEQ,			//패킷의 일련번호 월패드에서 부여
	HDC_F_D1,			//가변  길이의 데이터 필드.
	HDC_F_D2,			//가변  길이의 데이터 필드.
	HDC_F_D3,			//가변  길이의 데이터 필드.
	HDC_F_D4,			//가변  길이의 데이터 필드.
	HDC_F_D5			//가변  길이의 데이터 필드.
}_hdc_multiswitch_frame_;
#endif



typedef struct
{
	uint8_t word;
	uint8_t value;
}_length_;

typedef struct
{
	uint8_t		buf[HDC_MAX_BUF];
	uint16_t	count;
	_length_	length;
	uint8_t		send_flag;
}HDC_BUF;

#define	SET__GAS_CLOSE_REQUEST	1
#define	SET__GAS_CLOSE_STATE	2
#define	SET__GAS_OPEN_STATE		3
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
extern void BATCH_BLOCK_STATE_Process(void);
extern uint8_t Gu8_HDC_Error_Code;
extern uint8_t Gu8_Elec_Auto_OFF[2];
extern uint8_t Gu8_Elec_Overload_OFF[2];
// extern uint8_t Temp2Hex(float temp);


#endif


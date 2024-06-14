#ifndef __RS_485_KDW_H
#define __RS_485_KDW_H

// KDW ----------------------------------------------------------------------------------
#define	KDW_RES_DELAY_TIME	11				// 10ms 지나고 20ms 이내에 응답
#define	KDW_INTERVAL_TIME	6				// 5ms 지만 6ms로 설정
//-----------------------------------------------------------------------------------------------
#define	KDW_HEADER					0xF7	// 장치기 프로토콜은 0x20 고정사용

#define KDW_STATE_REQ				    0x01	//상태 요구, 상태요구 코드 범위(0x01 ~ 0x3F)
#define KDW_CHARACTER_REQ			    0x0F	//특성 요구
#define KDW_SELECT_CONTROL_REQ		    0x41	//개별 동작 제어 요구, 동작 제어 요구 코드 범위(0x40 ~ 0x7F)
#define KDW_ALL_CONTROL_REQ			    0x42	//전체 동작 제어 요구, 응답 없음
#define KDW_BATCHLIGHT_CONTROL_REQ	    0x43	//일괄 제어(전등)
#define KDW_3WAY_CONTROL_REQ		    0x51	//3로 동작 제어 요구
#define KDW_DIMMING_CONTROL_REQ		    0x52	//디밍 조명 개별 동작 제어 요구
#define KDW_COLOR_TEMP_CONTROL_REQ	    0x54	//색온도 조명 제어 요구

#define KDW_AUTO_BLOCK_VALUE_REQ	    0x31	//차단 설정 값 요구 
#define KDW_AUTO_BLOCK_VALUE_SET_REQ	0x43	//차단 설정값 설정 요구(대기전력)

#define KDW_USER_REQ_RESULT             0x43
#define KDW_ELEVATOR_FLOOR_REQ		    0x44	//엘리베이터 층 표시 요구
#define KDW_THREE_WAY_STATE_NOTICE      0x52
#define KDW_COOKTOP_STATE_NOTICE        0x55
#define KDW_COOKTOP_CONTROL_RESULT      0x56
#define KDW_ELEVATOR_ARRIVE             0x57
//-----------------------------------------------------------------------------------------------
#define	KDW_LIGHT_DEVICE_ID			    0x0E	// 전등 ID
#define	KDW_ELEC_DEVICE_ID			    0x39	// 전열 ID
#define	KDW_BATCH_BLOCK_DEVICE_ID	    0x33	// 일괄차단 ID
#define KDW_GAS_DEVICE_ID			    0x12	// 가스차단기 ID
//-----------------------------------------------------------------------------------------------
#define KDW_BATCH_LIGHT_DEVICE_SUB_ID	0xFF    //전등 기능 있는 스위치에서 일괄 소등/복귀 시 사용
//-----------------------------------------------------------------------------------------------
#define	KDW_OFF_FLAG				0
#define	KDW_ON_FLAG					1

#define KDW_DIMMING_UP				0x02
#define KDW_DIMMING_DOWN			0x01

#define KDW_LIGHT_ON_n_OFF          0
#define KDW_LIGHT_DIMMING           1

#define KDW_ELEC_1                  1
#define KDW_ELEC_2                  2
#define KDW_ELEC_ALL                0x0F

#define THOUSAND					0x10
#define HUNDRED						0x08
#define TEN							0x04
#define ONE							0x02
#define DECIMAL						0x01

#define WATT_TYPE_LCD               0x01
#define WATT_TYPE_LIMIT             0x02

#define	KDW_BATCH_LIGHT_ON			1				// 일괄조명차단 설정(LED ON)
#define	KDW_BATCH_LIGHT_OFF		    0				// 일괄조명차단 해제(LED OFF)
#define	KDW_GAS_CLOSE				0				// 가스밸브	 닫힘(LED ON)
#define	KDW_GAS_OPEN				1				// 가스밸브 열림(LED OFF)
#define KDW_REQUEST                 1
#define KDW_NOT_REQUEST             0
//--------------------------------------------------------------------------------------------
#define XOR_SUM						1
#define ADD_SUM						2
//--------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1					0xDD		//생산시 프로토콜, ID 확인용 커맨드
#define NIS_LIGHT_ID_COMM_2					0xBB
//--------------------------------------------------------------------------------------------
#define KDW_MAX_BUF 255

typedef enum
{
	KDW_FRAME_HD    = 0,
    KDW_FRAME_DEV_ID,
    KDW_FRAME_DEV_SUB_ID,
    KDW_FRAME_CMD_TYPE,
    KDW_FRAME_LENGTH,
    KDW_FRAME_DATA_0,
    KDW_FRAME_DATA_1,
    KDW_FRAME_DATA_2,
    KDW_FRAME_DATA_3

	// KDW_MAX_BUF			// 15
}_KDW_frame_;

typedef struct
{
	// uint8_t word;
	uint8_t value;
}_length_;

typedef struct
{
	uint8_t		buf[KDW_MAX_BUF];
	uint16_t	count;
	uint8_t		send_flag;
    _length_	length;
}KDW_BUF;
// ----------------------------------------------------------------------------------------
#define	SET__GAS_CLOSE_REQUEST	1
#define	SET__GAS_CLOSE_STATE	2
#define	SET__GAS_OPEN_STATE		3
#define SET__ELEVATOR_REQUEST	4
#define	SET__ELEVATOR_CALL		5
#define SET__ELEVATOR_ARRIVE	6
#define SET__ELEVATOR_CALL_FAIL	7
#define SET__BATCHLIGHT_OFF		8
#define SET__BATCHLIGHT_ON		9
#define SET__COOK_CLOSE_REQUEST 10
#define SET__COOK_CLOSE_STATE   11
#define SET__COOK_OPEN_STATE    12
// #define SET__THREEWAY_ON        13
// #define SET__THREEWAY_OFF       14

extern void BATCH_BLOCK_Control(uint8_t control);
extern void Protocol_Data_Init(void);
extern void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr);
extern void RS485_Tx_Process(void);
extern void Protocol_Process(uint8_t data);

extern void Elevator_Process(void);
extern void ELEVATOR_Call(void);
extern void ELEVATOR_Cancel(void);

//extern int Debug_SET_Switch_ID(int argc, char *argv[]);
extern uint8_t Get_485_ID(void);
extern uint8_t Get_Elevator_485_ID(void);
extern uint8_t Gu8_3Way_Flag;
extern uint8_t Gu8_OFF_Repeat_Tmr;
extern uint8_t Gu8_ON_Repeat_Tmr;
extern uint8_t Gu8_Elec_OFF_Repeat_Tmr;
extern uint8_t Gu8_Elec_ON_Repeat_Tmr;

#endif


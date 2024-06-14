#ifndef __RS_485_SAMSUNG_H
#define __RS_485_SAMSUNG_H

// 삼성SDS----------------------------------------------------------------------------------------
#define	SAMSUNG_RES_DELAY_TIME			4		// 최소 3ms
#define	SAMSUNG_INTERVAL_TIME			25		// 기준없음
//--------------------------------------------------------------------------------------------
#define SAMSUNG_LIGHT_HEADER			    0xAC
#define SAMSUNG_BATCH_BLOCK_HEADER		    0xAD
#define SAMSUNG_BATCH_BLOCK_2_HEADER        0xA9        //2번 일괄 스위치
#define SAMSUNG_ALL_DEVICE_ACK              0xB0
//--------------------------------------------------------------------------------------------
#define SAMSUNG_ON_FLAG					0x01
#define SAMSUNG_OFF_FLAG				0x00
//--------------------------------------------------------------------------------------------
#define SAMSUNG_GAS_BLOCK               0x01
//--------------------------------------------------------------------------------------------
#define SAMSUNG_ELEVATOR_CALL           0x01
//--------------------------------------------------------------------------------------------
#define SAMSUNG_ON_n_OFF				0x01
#define SAMSUNG_DIMMING					0x02
#define SAMSUNG_COLOR_TEMP				0x04
//--------------------------------------------------------------------------------------------
#define SAMSUNG_AUTO_FLAG               0x01
//--------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1		        0xDD		//생산시 프로토콜, ID 확인용 커맨드
#define NIS_LIGHT_ID_COMM_2             0xBB
//--------------------------------------------------------------------------------------------

typedef enum
{
    SAMSUNG_POWER_ON				            = 0x5A,     //POWER ON 시
    SAMSUNG_LIGHT_STATE_REQUEST		            = 0x79,     //조명 상태
    SAMSUNG_LIGHT_STATE_17circuit_REQUEST       = 0x75,     //조명 상태(해당 모듈에 전등 ID 17번이상이 포함되었을때)
    SAMSUNG_LIGHT_CONTROL			            = 0x7A,     //조명 제어
    SAMSUNG_LIGHT_GROUP_CONTROL                 = 0x1A,
    SAMSUNG_DIMMING_LIGHT_CONTROL	            = 0x7B,     //디밍 제어
    SAMSUNG_DIMMING_LIGHT_REQUEST               = 0x1E,

    SAMSUNG_BATCH_BLOCK_GAS_REQUEST             = 0x50,     //가스 밸브 상태 요청(세대기 -> 일괄) 20240417 문의 시 실제로 해당 데이터는 사용하지 않는다고 함. 일괄 스위치에 가스차단기가 직접 연결 되어 있는 경우에 사용하는 데이터. 응답을 안해도 되고, 아마도 데이터 보내지 않을거라고 함.
    SAMSUNG_BATCH_BLOCK_GAS_CONTROL             = 0x51,     //가스 밸브 제어(세대기 -> 일괄)
    SAMSUNG_BATCH_BLOCK_REQUEST                 = 0x52,     //일괄 상태 요구
    SAMSUNG_BATCH_BLOCK_CONTROL                 = 0x53,     //일괄 제어
    
    SAMSUNG_BATCH_BLOCK_SWITCH_CONTROL          = 0x54,     //스위치에서 일괄소등(점등) 제어 했을때
    SAMSUNG_BATCH_BLOCK_SWITCH_GAS_CONTROL      = 0x55,     //스위치에서 가스 제어 했을때
    SAMSUNG_BATCH_BLOCK_SWITCH_GAS_REQUEST      = 0x56,     //스위치에서 가스 상태 요청...???
    SAMSUNG_BATCH_BLOCK_SWITCH_ELEVATOR_CONTROL = 0x2F,     //스위치에서 엘리베이터 요청
    SAMSUNG_STATUS_DATA                         = 0x41

}_MASTER_COMM_;

typedef enum
{
	SAMSUNG_F_HEADER		= 0,
	SAMSUNG_F_COMMAND,
	SAMSUNG_F_SUB_DATA_1,
    SAMSUNG_F_SUB_DATA_2,
    SAMSUNG_F_BATCH_BLOCK_CHECK_SUM = 3,
	SAMSUNG_F_LIGHT_CHECK_SUM,
	SAMSUNG_BATCH_BLOCK_MAX_BUF = 4
}_SAMSUNG_light_rx_frame_;

typedef enum
{
    SAMSUNG_F_SUB_DATA_3    = 4,
    SAMSUNG_F_SUB_DATA_4,
    SAMSUNG_F_SUB_DATA_5,
    SAMSUNG_F_SUB_DATA_6,
    SAMSUNG_F_SUB_DATA_7,
    SAMSUNG_F_SUB_DATA_8,
    SAMSUNG_F_SUB_DATA_9,
    SAMSUNG_F_DIM_CHECK_SUM,
    SAMSUNG_MAX_BUF
}_SAMSUNG_dimming_tx_frame_;

typedef struct
{
	uint8_t		buf[SAMSUNG_MAX_BUF];
	uint16_t	count;
	uint8_t		send_flag;
}SAMSUNG_BUF;

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

// extern uint8_t Batch_Light_Use_Tmr;


// extern void SAMSUNG_Switch_Control_Process(void);

//-----------------------------------------------------------------------------------------

#endif
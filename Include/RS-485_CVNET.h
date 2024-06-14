#ifndef __RS_485_CVNET_H
#define __RS_485_CVNET_H

// CVNET ----------------------------------------------------------------------------------
#define	CVNET_RES_DELAY_TIME	11				// 10ms 지나고 20ms 이내에 응답
#define	CVNET_INTERVAL_TIME		6				// 5ms 지만 6ms로 설정

#define	CVNET_STX						0xF7	// Preamble
#define	CVNET_HEADER					0x20	// 장치기 프로토콜은 0x20 고정사용
#define	CVNET_WALLPAD_ID				0x01	// 월패드 ID, 조명장치(0x21 ~ 0x2E), 대기전력스위치(0xC1 ~ 0xCE)
#define	CVNET_ETX						0xAA	// End of Packet

#define	CVNET_GROUP_ID					0x0F	// 그룹제어 어드레스 ID
#define	CVNET_RES_FLAG					0x80	// 응답 플래그

#define	CVNET_LIGHT_DEVICE_ID			0x20	// 전등 ID
#define	CVNET_LIGHT_ELEC_DEVICE_ID		0xB0	// 전등+전열 ID
#define	CVNET_BATCH_BLOCK_DEVICE_ID		0x80	// 일괄차단 ID
#define CVNET_TOTAL_SWITCH_DEVICE_ID	0xE1	// 통합스위치
#define CVNET_THREEWAY_DEVICE_ID		0x90	// 3로 스위치 ID
//-----------------------------------------------------------------------------------------------
//스위치 아이디는 1번 부터 시작(예를들어 전등스위치라면, 거실(0x21), 방1(0x22))
//-----------------------------------------------------------------------------------------------
#define	CVNET_OFF_FLAG				0
#define	CVNET_ON_FLAG				1
#define	CVNET_MANUAL_FLAG			0
#define	CVNET_AUTO_FLAG				1

#define	CVNET_BATCH_LIGHT_SET			0				// 일괄조명차단 설정(LED ON)
#define	CVNET_BATCH_LIGHT_RELEASE		1				// 일괄조명차단 해제(LED OFF)
#define	CVNET_GAS_CLOSE					0				// 가스밸브	 닫힘(LED ON)
#define	CVNET_GAS_OPEN					1				// 가스밸브 열림(LED OFF)
#define	CVNET_ELEVATOR_NORMAL			0x00
#define	CVNET_ELEVATOR_ARRIVE			0x80
#define	CVNET_ELEVATOR_UP				0x04
#define	CVNET_ELEVATOR_DOWN				0x08
#define	CVNET_ELEVATOR_TRANS			0x0C
#define	CVNET_ELEVATOR_UP_REQ			0x01
#define	CVNET_ELEVATOR_DOWN_REQ			0x02
//-----------------------------------------------------------------------------------------------
#define	CMD_BATCHLIGHT_n_ELEVATOR_CONTROL_REQ					0x11			// 일괄조명 제어				일괄차단 모델
#define	CMD_BATCHLIGHT_CONTROL_RES								0x9F			// 일괄조명 제어				일괄차단 모델
#define	CMD_GAS_CONTROL_REQ										0x12			// 가스 제어				일괄차단 모델
#define	CMD_GAS_CONTROL_RES										0x9F			// 가스 제어				일괄차단 모델
// #define	CMD_ELEVATOR_CONTROL_REQ							0x11			// 엘리베이터 제어 요구
#define	CMD_ELEVATOR_CONTROL_RES								0x91			// 엘리베이터 제어 응답
//-----------------------------------------------------------------------------------------------
#define	CMD_TOTAL_CONTROL_REQ						0x1F		// 전체제어			조명 모델, 일괄차단 모델
#define	CMD_TOTAL_CONTROL_RES						0x9F		// 전체제어응답		조명 모델, 일괄차단 모델
#define	CMD_GROUP_CONTROL_REQ						0x3F		// 조명 그룹제어 	- 응답없음
	//-----------------------------------------------------------------------------------------------
#define	CMD_SELECTIVE_CONTROL_1_REQ			0x11		// 조명 선택제어1		조명 모델
#define	CMD_SELECTIVE_CONTROL_2_REQ			0x12		// 조명 선택제어2		조명 모델
#define	CMD_SELECTIVE_CONTROL_3_REQ			0x13		// 조명 선택제어3		조명 모델
#define	CMD_SELECTIVE_CONTROL_4_REQ			0x14		// 조명 선택제어4		조명 모델
#define	CMD_SELECTIVE_CONTROL_5_REQ			0x15		// 조명 선택제어5		조명 모델
#define	CMD_SELECTIVE_CONTROL_6_REQ			0x16		// 조명 선택제어6		조명 모델
#define	CMD_SELECTIVE_CONTROL_7_REQ			0x17		// 조명 선택제어7		조명 모델
#define	CMD_SELECTIVE_CONTROL_8_REQ			0x18		// 조명 선택제어8		조명 모델
#define	CMD_SELECTIVE_CONTROL_1_8_RES		0x9F		// 조명 선택제어1~8응답	조명 모델
//-----------------------------------------------------------------------------------------------
#define	CMD_STATE_REQ						0x01		// 상태요구		조명 모델, 조명+대기전력 모델, 일괄차단 모델
#define	CMD_STATE_RES						0x81		// 상태응답		조명 모델, 조명+대기전력 모델, 일괄차단 모델
//-----------------------------------------------------------------------------------------------
#define	CMD_LIGHT_TOTAL_CONTROL_REQ			0x1E		// 조명 전체제어			조명+대기전력 모델
#define	CMD_LIGHT_TOTAL_CONTROL_RES			0x9E		// 조명 전체제어응답		조명+대기전력 모델

#define	CMD_ELEC_TOTAL_CONTROL_REQ			0x1F		// 대기전력 전체제어		조명+대기전력 모델
#define	CMD_ELEC_TOTAL_CONTROL_RES			0x9F		// 대기전력 전체제어응답	조명+대기전력 모델

#define	CMD_LIGHT_SELECTIVE_CONTROL_1_REQ	0x11		// 조명 선택제어1		조명+대기전력 모델
#define	CMD_LIGHT_SELECTIVE_CONTROL_1_RES	0x91		// 조명 선택제어1응답	조명+대기전력 모델
#define	CMD_LIGHT_SELECTIVE_CONTROL_2_REQ	0x12		// 조명 선택제어2		조명+대기전력 모델
#define	CMD_LIGHT_SELECTIVE_CONTROL_2_RES	0x92		// 조명 선택제어2응답	조명+대기전력 모델
#define	CMD_LIGHT_SELECTIVE_CONTROL_3_REQ	0x13		// 조명 선택제어3		조명+대기전력 모델
#define	CMD_LIGHT_SELECTIVE_CONTROL_3_RES	0x93		// 조명 선택제어3응답	조명+대기전력 모델
#define	CMD_LIGHT_SELECTIVE_CONTROL_4_REQ	0x14		// 조명 선택제어4		조명+대기전력 모델
#define	CMD_LIGHT_SELECTIVE_CONTROL_4_RES	0x94		// 조명 선택제어4응답	조명+대기전력 모델

#define	CMD_ELEC_SELECTIVE_CONTROL_1_REQ	0x17		// 대기 선택제어1		조명+대기전력 모델
#define	CMD_ELEC_SELECTIVE_CONTROL_1_RES	0x97		// 대기 선택제어1응답	조명+대기전력 모델
#define	CMD_ELEC_SELECTIVE_CONTROL_2_REQ	0x18		// 대기 선택제어2		조명+대기전력 모델
#define	CMD_ELEC_SELECTIVE_CONTROL_2_RES	0x98		// 대기 선택제어2응답	조명+대기전력 모델
#define	CMD_ELEC_SELECTIVE_CONTROL_3_REQ	0x19		// 대기 선택제어3		조명+대기전력 모델
#define	CMD_ELEC_SELECTIVE_CONTROL_3_RES	0x99		// 대기 선택제어3응답	조명+대기전력 모델
#define	CMD_ELEC_SELECTIVE_CONTROL_4_REQ	0x1A		// 대기 선택제어4		조명+대기전력 모델
#define	CMD_ELEC_SELECTIVE_CONTROL_4_RES	0x9A		// 대기 선택제어4응답	조명+대기전력 모델

#define CMD_LIGHT_CHARACTER_REQ				0x21
#define CMD_LIGHT_CHARACTER_RES				0xA1
//--------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1					0xDD		//생산시 프로토콜, ID 확인용 커맨드
#define NIS_LIGHT_ID_COMM_2					0xBB
//--------------------------------------------------------------------------------------------

typedef enum
{
	CVNET_FRAME_PRE	= 0,
	CVNET_FRAME_HD,
	CVNET_FRAME_DA,
	CVNET_FRAME_SA,
	CVNET_FRAME_CMD,
	CVNET_FRAME_D0,
	CVNET_FRAME_D1,
	CVNET_FRAME_D2,
	CVNET_FRAME_D3,
	CVNET_FRAME_D4,
	CVNET_FRAME_D5,
	CVNET_FRAME_D6,
	CVNET_FRAME_D7,
	CVNET_FRAME_FEC,
	CVNET_FRAME_EOP,
	CVNET_MAX_BUF			// 15
}_cvnet_frame_;

typedef struct
{
	uint8_t		buf[CVNET_MAX_BUF];
	uint16_t	count;
	uint8_t		send_flag;
}CVNET_BUF;
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
#define SET__THREEWAY_ON		10
#define SET__THREEWAY_OFF		11

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
extern void BATCH_BLOCK_STATE_Process(void);
extern uint8_t Get_Batch_Block_485_ID(void);
extern uint8_t Gu8_3Way_Flag;

#endif


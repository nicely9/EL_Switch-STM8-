#ifndef __RS_485_KDW_H
#define __RS_485_KDW_H

// KDW ----------------------------------------------------------------------------------
#define	KDW_RES_DELAY_TIME	11				// 10ms ������ 20ms �̳��� ����
#define	KDW_INTERVAL_TIME	6				// 5ms ���� 6ms�� ����
//-----------------------------------------------------------------------------------------------
#define	KDW_HEADER					0xF7	// ��ġ�� ���������� 0x20 �������

#define KDW_STATE_REQ				    0x01	//���� �䱸, ���¿䱸 �ڵ� ����(0x01 ~ 0x3F)
#define KDW_CHARACTER_REQ			    0x0F	//Ư�� �䱸
#define KDW_SELECT_CONTROL_REQ		    0x41	//���� ���� ���� �䱸, ���� ���� �䱸 �ڵ� ����(0x40 ~ 0x7F)
#define KDW_ALL_CONTROL_REQ			    0x42	//��ü ���� ���� �䱸, ���� ����
#define KDW_BATCHLIGHT_CONTROL_REQ	    0x43	//�ϰ� ����(����)
#define KDW_3WAY_CONTROL_REQ		    0x51	//3�� ���� ���� �䱸
#define KDW_DIMMING_CONTROL_REQ		    0x52	//��� ���� ���� ���� ���� �䱸
#define KDW_COLOR_TEMP_CONTROL_REQ	    0x54	//���µ� ���� ���� �䱸

#define KDW_AUTO_BLOCK_VALUE_REQ	    0x31	//���� ���� �� �䱸 
#define KDW_AUTO_BLOCK_VALUE_SET_REQ	0x43	//���� ������ ���� �䱸(�������)

#define KDW_USER_REQ_RESULT             0x43
#define KDW_ELEVATOR_FLOOR_REQ		    0x44	//���������� �� ǥ�� �䱸
#define KDW_THREE_WAY_STATE_NOTICE      0x52
#define KDW_COOKTOP_STATE_NOTICE        0x55
#define KDW_COOKTOP_CONTROL_RESULT      0x56
#define KDW_ELEVATOR_ARRIVE             0x57
//-----------------------------------------------------------------------------------------------
#define	KDW_LIGHT_DEVICE_ID			    0x0E	// ���� ID
#define	KDW_ELEC_DEVICE_ID			    0x39	// ���� ID
#define	KDW_BATCH_BLOCK_DEVICE_ID	    0x33	// �ϰ����� ID
#define KDW_GAS_DEVICE_ID			    0x12	// �������ܱ� ID
//-----------------------------------------------------------------------------------------------
#define KDW_BATCH_LIGHT_DEVICE_SUB_ID	0xFF    //���� ��� �ִ� ����ġ���� �ϰ� �ҵ�/���� �� ���
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

#define	KDW_BATCH_LIGHT_ON			1				// �ϰ��������� ����(LED ON)
#define	KDW_BATCH_LIGHT_OFF		    0				// �ϰ��������� ����(LED OFF)
#define	KDW_GAS_CLOSE				0				// �������	 ����(LED ON)
#define	KDW_GAS_OPEN				1				// ������� ����(LED OFF)
#define KDW_REQUEST                 1
#define KDW_NOT_REQUEST             0
//--------------------------------------------------------------------------------------------
#define XOR_SUM						1
#define ADD_SUM						2
//--------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1					0xDD		//����� ��������, ID Ȯ�ο� Ŀ�ǵ�
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


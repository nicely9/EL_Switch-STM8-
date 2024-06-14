#ifndef __RS_485_CVNET_H
#define __RS_485_CVNET_H

// CVNET ----------------------------------------------------------------------------------
#define	CVNET_RES_DELAY_TIME	11				// 10ms ������ 20ms �̳��� ����
#define	CVNET_INTERVAL_TIME		6				// 5ms ���� 6ms�� ����

#define	CVNET_STX						0xF7	// Preamble
#define	CVNET_HEADER					0x20	// ��ġ�� ���������� 0x20 �������
#define	CVNET_WALLPAD_ID				0x01	// ���е� ID, ������ġ(0x21 ~ 0x2E), ������½���ġ(0xC1 ~ 0xCE)
#define	CVNET_ETX						0xAA	// End of Packet

#define	CVNET_GROUP_ID					0x0F	// �׷����� ��巹�� ID
#define	CVNET_RES_FLAG					0x80	// ���� �÷���

#define	CVNET_LIGHT_DEVICE_ID			0x20	// ���� ID
#define	CVNET_LIGHT_ELEC_DEVICE_ID		0xB0	// ����+���� ID
#define	CVNET_BATCH_BLOCK_DEVICE_ID		0x80	// �ϰ����� ID
#define CVNET_TOTAL_SWITCH_DEVICE_ID	0xE1	// ���ս���ġ
#define CVNET_THREEWAY_DEVICE_ID		0x90	// 3�� ����ġ ID
//-----------------------------------------------------------------------------------------------
//����ġ ���̵�� 1�� ���� ����(������� �����ġ���, �Ž�(0x21), ��1(0x22))
//-----------------------------------------------------------------------------------------------
#define	CVNET_OFF_FLAG				0
#define	CVNET_ON_FLAG				1
#define	CVNET_MANUAL_FLAG			0
#define	CVNET_AUTO_FLAG				1

#define	CVNET_BATCH_LIGHT_SET			0				// �ϰ��������� ����(LED ON)
#define	CVNET_BATCH_LIGHT_RELEASE		1				// �ϰ��������� ����(LED OFF)
#define	CVNET_GAS_CLOSE					0				// �������	 ����(LED ON)
#define	CVNET_GAS_OPEN					1				// ������� ����(LED OFF)
#define	CVNET_ELEVATOR_NORMAL			0x00
#define	CVNET_ELEVATOR_ARRIVE			0x80
#define	CVNET_ELEVATOR_UP				0x04
#define	CVNET_ELEVATOR_DOWN				0x08
#define	CVNET_ELEVATOR_TRANS			0x0C
#define	CVNET_ELEVATOR_UP_REQ			0x01
#define	CVNET_ELEVATOR_DOWN_REQ			0x02
//-----------------------------------------------------------------------------------------------
#define	CMD_BATCHLIGHT_n_ELEVATOR_CONTROL_REQ					0x11			// �ϰ����� ����				�ϰ����� ��
#define	CMD_BATCHLIGHT_CONTROL_RES								0x9F			// �ϰ����� ����				�ϰ����� ��
#define	CMD_GAS_CONTROL_REQ										0x12			// ���� ����				�ϰ����� ��
#define	CMD_GAS_CONTROL_RES										0x9F			// ���� ����				�ϰ����� ��
// #define	CMD_ELEVATOR_CONTROL_REQ							0x11			// ���������� ���� �䱸
#define	CMD_ELEVATOR_CONTROL_RES								0x91			// ���������� ���� ����
//-----------------------------------------------------------------------------------------------
#define	CMD_TOTAL_CONTROL_REQ						0x1F		// ��ü����			���� ��, �ϰ����� ��
#define	CMD_TOTAL_CONTROL_RES						0x9F		// ��ü��������		���� ��, �ϰ����� ��
#define	CMD_GROUP_CONTROL_REQ						0x3F		// ���� �׷����� 	- �������
	//-----------------------------------------------------------------------------------------------
#define	CMD_SELECTIVE_CONTROL_1_REQ			0x11		// ���� ��������1		���� ��
#define	CMD_SELECTIVE_CONTROL_2_REQ			0x12		// ���� ��������2		���� ��
#define	CMD_SELECTIVE_CONTROL_3_REQ			0x13		// ���� ��������3		���� ��
#define	CMD_SELECTIVE_CONTROL_4_REQ			0x14		// ���� ��������4		���� ��
#define	CMD_SELECTIVE_CONTROL_5_REQ			0x15		// ���� ��������5		���� ��
#define	CMD_SELECTIVE_CONTROL_6_REQ			0x16		// ���� ��������6		���� ��
#define	CMD_SELECTIVE_CONTROL_7_REQ			0x17		// ���� ��������7		���� ��
#define	CMD_SELECTIVE_CONTROL_8_REQ			0x18		// ���� ��������8		���� ��
#define	CMD_SELECTIVE_CONTROL_1_8_RES		0x9F		// ���� ��������1~8����	���� ��
//-----------------------------------------------------------------------------------------------
#define	CMD_STATE_REQ						0x01		// ���¿䱸		���� ��, ����+������� ��, �ϰ����� ��
#define	CMD_STATE_RES						0x81		// ��������		���� ��, ����+������� ��, �ϰ����� ��
//-----------------------------------------------------------------------------------------------
#define	CMD_LIGHT_TOTAL_CONTROL_REQ			0x1E		// ���� ��ü����			����+������� ��
#define	CMD_LIGHT_TOTAL_CONTROL_RES			0x9E		// ���� ��ü��������		����+������� ��

#define	CMD_ELEC_TOTAL_CONTROL_REQ			0x1F		// ������� ��ü����		����+������� ��
#define	CMD_ELEC_TOTAL_CONTROL_RES			0x9F		// ������� ��ü��������	����+������� ��

#define	CMD_LIGHT_SELECTIVE_CONTROL_1_REQ	0x11		// ���� ��������1		����+������� ��
#define	CMD_LIGHT_SELECTIVE_CONTROL_1_RES	0x91		// ���� ��������1����	����+������� ��
#define	CMD_LIGHT_SELECTIVE_CONTROL_2_REQ	0x12		// ���� ��������2		����+������� ��
#define	CMD_LIGHT_SELECTIVE_CONTROL_2_RES	0x92		// ���� ��������2����	����+������� ��
#define	CMD_LIGHT_SELECTIVE_CONTROL_3_REQ	0x13		// ���� ��������3		����+������� ��
#define	CMD_LIGHT_SELECTIVE_CONTROL_3_RES	0x93		// ���� ��������3����	����+������� ��
#define	CMD_LIGHT_SELECTIVE_CONTROL_4_REQ	0x14		// ���� ��������4		����+������� ��
#define	CMD_LIGHT_SELECTIVE_CONTROL_4_RES	0x94		// ���� ��������4����	����+������� ��

#define	CMD_ELEC_SELECTIVE_CONTROL_1_REQ	0x17		// ��� ��������1		����+������� ��
#define	CMD_ELEC_SELECTIVE_CONTROL_1_RES	0x97		// ��� ��������1����	����+������� ��
#define	CMD_ELEC_SELECTIVE_CONTROL_2_REQ	0x18		// ��� ��������2		����+������� ��
#define	CMD_ELEC_SELECTIVE_CONTROL_2_RES	0x98		// ��� ��������2����	����+������� ��
#define	CMD_ELEC_SELECTIVE_CONTROL_3_REQ	0x19		// ��� ��������3		����+������� ��
#define	CMD_ELEC_SELECTIVE_CONTROL_3_RES	0x99		// ��� ��������3����	����+������� ��
#define	CMD_ELEC_SELECTIVE_CONTROL_4_REQ	0x1A		// ��� ��������4		����+������� ��
#define	CMD_ELEC_SELECTIVE_CONTROL_4_RES	0x9A		// ��� ��������4����	����+������� ��

#define CMD_LIGHT_CHARACTER_REQ				0x21
#define CMD_LIGHT_CHARACTER_RES				0xA1
//--------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1					0xDD		//����� ��������, ID Ȯ�ο� Ŀ�ǵ�
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


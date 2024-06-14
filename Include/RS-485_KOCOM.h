#ifndef __RS_485_KOCOM_H
#define __RS_485_KOCOM_H

// ����----------------------------------------------------------------------------------------
#define KOCOM_PREAMBLE_1		0xAA		//START CODE 1
#define KOCOM_PREAMBLE_2		0x55		//START CODE 2

#define KOCOM_HD				0x30		//HEADER

#define KOCOM_EOT_1				0x0D		//
#define KOCOM_EOT_2				0x0D		//

#define KOCOM_WALLPAD			0x01
#define KOCOM_WALLPAD_ID		0x00

#define KOCOM_LIGHT_DEVICE		0x0E		//�����ġ
#define KOCOM_ELEC_DEVICE		0x3B
#define KOCOM_USS_DEVICE		0x54

#define KOCOM_BATCH_DEVICE		0x90		//�ϰ�����ġ
#define KOCOM_GAS_DEVICE		0x2C		//�ϰ�����ġ ���� 
#define KOCOM_COOK_DEVICE		0x2D		//�ϰ�����ġ ��ž
#define KOCOM_ELEVATOR_DEVICE	0x44		//�ϰ�����ġ ���������� ȣ���Ҷ�

#define KOCOM_MULTI_DEVICE		0x82		//�ٱ�� ����ġ

#define KOCOM_3WAY_LIGHT_DEVICE	0x61		//���� ����ġ 3��
#define KOCOM_3WAY_BATCH_DEVICE	0x62		//�ϰ� ����ġ 3��
/*
3�� ����ġ�� ��� �����(0x01)���� �������� ������, �ش� ��ġ�� ���� �������� ����
���� ���¸� ����⿡�� ��û�ϴ� ��찡 ����.
CC���� 0x9C�θ� ó���ϰ� ���� ��û�� ó����.
���� ������ ������ ��� �ʿ� ����Ǿ� �ִ��� �������� ������, �ش� ��� �߻��� ��쳪
��û�� ���� ��� �� �� �ٸ� ��ġ �ʿ� ���� ���� �ؾ� �Ѵ�.
*/
//--------------------------------------------------------------------------------------------
#define	KOCOM_RES_DELAY_TIME			11		// 10ms ������ 250ms �̳��� ����
#define	KOCOM_INTERVAL_TIME				6		// ���ؾ���
#define	KOCOM_RESULT_SEND_TIME			251		// ���� ��� ���۽ð�
#define	KOCOM_GROUP_RESULT_SEND_TIME	251		// �׷� ��� ���۽ð�
#define	KOCOM_SINGLE_ID					0x01	// ��������
//--------------------------------------------------------------------------------------------
// LIGHT_PROTOCOL_VERSION	2.43	����
#define	KOCOM_LIGHT_PROTOCOL_VERSION_H	2
#define	KOCOM_LIGHT_PROTOCOL_VERSION_L	43
// ELEC_PROTOCOL_VERSION	2.23	����
#define KOCOM_ELEC_PROTOCOL_VERSION_H	2	//�ӽð�
#define KOCOM_ELEC_PROTOCOL_VERSION_L	23	//�ӽð�
// BATCH_PROTOCOL_VERSION	2.01	�ϰ�
#define KOCOM_BATCH_PROTOCOL_VERSION_H	2
#define KOCOM_BATCH_PROTOCOL_VERSION_L	1
// MULTI_PROTOCOL_VERSION	2.43	�ٱ��
#define KOCOM_MULTI_PROTOCOL_VERSION_H	1
#define KOCOM_MULTI_PROTOCOL_VERSION_L	6
//--------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1		0xDD		//����� ��������, ID Ȯ�ο� Ŀ�ǵ�
#define NIS_LIGHT_ID_COMM_2		0xBB
//--------------------------------------------------------------------------------------------
#define	KOCOM_LOWEST_ID			0x00		// ��ġ�Ⱑ ������ �� �ִ� ���� ���� ID
#define	KOCOM_GROUP_ID			0xFF		// �׷�����
#define	KOCOM_CC_NOT_ACK		0x9C		// 
#define	KOCOM_CC_REQ			0xB0		// CC REQ	(0xBC, 0xBD, 0xBE)
#define	KOCOM_CC_ACK			0xD0		// CC ACK	(0xDC, 0xDD, 0xDE)
#define	KOCOM_CC_RESULT			0xBC		/* CC Result(0xBC, 0xBD, 0xBE) */

#define	KOCOM_OFF_FLAG			0x00
#define	KOCOM_ON_FLAG			0xFF
#define KOCOM_UNKOWN_FLAG		0x01

#define KOCOM_NO_CIRCUIT		0x00		//ȸ�� ������
#define KOCOM_LIGHT_CIRCUIT		0x01		//�ܼ� ON/OFF ȸ���϶�

#define KOCOM_BATCH_OFF_STATE	0x63		//�ϰ��ҵ� ����
#define KOCOM_BATCH_ON_STATE	0x64		//�ϰ����� ����

/*#define KOCOM_ELEC_OFF_STATE	0x65		//�ܼ�Ʈ �ϰ� OFF ����
#define KOCOM_ELEC_ON_STATE		0x66		//�ܼ�Ʈ �ϰ� ON ����*/

#define KOCOM_BLOCK_FLAG			0x01	//����(����, ��ž)
#define KOCOM_BLOCK_RELEASE_FLAG	0x00	//��������(����, ��ž)

//D0
#define KOCOM_ELEVATOR_STOP			0x00	//���������� ���� ����
#define KOCOM_ELEVATOR_DOWNWARD		0x01	//���������� ����
#define KOCOM_ELEVATOR_UPWARD		0x02	//���������� ����
#define KOCOM_ELEVATOR_ARRIVE		0x03	//���������� ����

//D1 
//������ �ֻ��� ��Ʈ�� ���� ����(0), ����(1)�� ����. ������ ��Ʈ�� ���� ������ 16������ ǥ����.
//���� 28�� -> 0x1C, ���� 28�� -> 0x9C. ��, ������ 16���� �ٷ� ǥ��, ���ϴ� (1 << 7 | 16����) -> (1 << 7 | 0x1C) = 0x9C

typedef enum
{
	KOCOM_OP_LIGHT_CONTROL			=	0x00,	//����
	KOCOM_OP_3WAY_CONTROL			=	0x00,
	KOCOM_OP_STATE_REQ				=	0x3A,	//���¿䱸
	KOCOM_OP_BATCH_LIGHT_REQ		=	0x62,	//�ϰ����� �� ���¿䱸
	KOCOM_OP_BATCH_LIGHT_STATE_OFF	=	0x63,	//�ϰ��ҵ� ���� ��
	KOCOM_OP_BATCH_LIGHT_STATE_ON	=	0x64,	//�ϰ����� ���� ��
	KOCOM_OP_BATCH_LIGHT_OFF		=	0x65,	//�ϰ��ҵ� ���
	KOCOM_OP_BATCH_LIGHT_ON			=	0x66,	//�ϰ����� ���
	
	KOCOM_OP_PROTOCOL_VERSION		=	0x4A,
	KOCOM_OP_DIM_LEVEL				=	0x5A,		//���� ��ġ�� ��� �ܰ踦 ��ȸ
	KOCOM_OP_COLOR_TEMP_CONTROL		=	0x01,		//���µ� ����
	KOCOM_OP_COLOR_TEMP_REQ			=	0x3B,		//���µ� ������ȸ
	KOCOM_OP_COLOR_TEMP_LEVEL		=	0x5B,		//���� ��ġ�� ���µ� �ְ� �ܰ踦 ��ȸ

	KOCOM_OP_GAS_STATE_REQ			=	0x3A,
	KOCOM_OP_GAS_BLOCK				=	0x02,
	KOCOM_OP_GAS_BLOCK_RELEASE		=	0x01,
	KOCOM_OP_COOK_STATE_REQ			=	0x3A,
	KOCOM_OP_COOK_BLOCK				=	0x02,
	KOCOM_OP_COOK_BLOCK_RELEASE		=	0x01,

	KOCOM_OP_ELEVATOR_CALL			=	0x01,
	KOCOM_OP_ELEVATOR_CALL_FAIL		=	0xFF,
	//------------------------------------------------------------�ܼ�Ʈ
	KOCOM_OP_BATCH_ELEC_OFF			=	0x65,
	KOCOM_OP_BATCH_ELEC_ON			=	0x66,
	KOCOM_OP_ELEC_STATE_REQ			=	0x3A,
	KOCOM_OP_ELEC_CONTROL			=	0x00,
	//------------------------------------------------------------�ٱ�ɽ���ġ
	KOCOM_OP_IMPLEMENT_INFO			=	0x6A,		//������� ����
	KOCOM_OP_WEATHER_INFO			=	0x01,		//���� ����
	KOCOM_OP_ENERGY_USAGE			=	0x02,		//������ ��뷮
	KOCOM_OP_MULTI_SWITCH_CONTROL	=	0x03,		//�ٱ�� ����ġ ����
	KOCOM_OP_PARCEL					=	0x04,		//�ù�
	KOCOM_OP_PARKING_INFO			=	0x05,		//���� ����
	KOCOM_OP_OUTING_n_SECURITY		=	0x06,		//����/����
	KOCOM_OP_ENERGY_SPECIES			=	0x07,		//������ ����
	KOCOM_OP_DUST_INFO				=	0x08,		//�̼����� ����
	KOCOM_OP_PARKING_DETAIL_INFO	=	0x50		//���� �� ����
}_OPERATION_CODE_;

typedef enum
{
	KOCOM_CMD_REQ_BC				=	0xBC,
	KOCOM_CMD_REQ_BD				=	0xBD,
	KOCOM_CMD_REQ_BE				=	0xBE,
	KOCOM_CMD_RES					=	0xDC,
	KOCOM_CMD_NON_ACK				=	0x9C		//����ϸ� ACK ���� ��� �����͸� ����.
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
a) Preamble : ù ��°(0xAA), �� ��°(0x55) ��Ŷ ������ �˸��� Start Code
b) HD : ���������� �뵵 ����
c) CC : Control Command ������ ī��Ʈ �� ��� �Ǵ� ACK�� ���� ����
d) PCNT : ���ӵ����� ���۽�, �����ִ� ��Ŷ ��
e) ADH / ADL : ������ ��/�� ����Ʈ
f) ASH / ASL : ����� ��/�� ����Ʈ
g) OP : Operation Code ��� �ڵ�
h) FCC : Check Sum üũ��(HD + ������ + D7 ����� ���� 1����Ʈ ��)
i) EOT : ������ 2����Ʈ(0x0D) ��Ŷ �������� End Code

����⿡�� ������ ������ ��, OP-Code �� 0x00 �̸� ��� ȸ�ΰ� ����Ǿ�� �Ѵ�.
��� �Ϸ�Ǿ� ������ ����⿡ ������� ������ �� OP-Code �� 0x00 �� �Ͽ� D0~D7 �� ��ü ȸ�� ������ �ݿ��Ͽ� �����Ѵ�.

# 30bc(��û) + 003a(��ȸ) = ��ȸ ��ɾ�
# 30bc(��û) + 0000(����) = ���� ��ɾ� (turn of/on)
# 30dc(����) + 0000(����) = ���� ��ɾ� ���� ����
# 30dc(����) + 003a(��ȸ) = ��ȸ ��ɾ ���� ����

1. ���� ���� (�Ž� ���� 1�� �ѱ�)
AA 55 30 BC 00 0E 00 01 00 00 FF 00 00 00 00 00 00 00 FA 0D 0D Ŀ�ǵ�(����� �Է�) (���е�->�Ž�(0����)�� ����1�� �Ѷ�(ff)����
AA 55 30 DC 00 01 00 0E 00 00 FF 00 00 00 00 00 00 00 1A 0D 0D ����(��Ⱓ �ڵ�) (�Ž�(0����) -> ���е� ����1�� �Ѷ� ��ȣ �޾��� ACK
AA 55 30 BC 00 01 00 0E 00 00 FF 00 00 00 00 00 00 00 FA 0D 0D ����(��Ⱓ �ڵ�) (�Ž�(0����) -> ���е� ����1 ������ ���� ����
AA 55 30 DC 00 0E 00 01 00 00 FF 00 00 00 00 00 00 00 1A 0D 0D ����(��Ⱓ �ڵ�) (���е�->�Ž�(0����)�� ����1�� ������ ���� �޾��� ACK

2. ���� Ȯ�� (1Byte ������)
AA 55 30 BC 00 2C 00 01 00 3A 00 00 00 00 00 00 00 00 53 0D 0D Ŀ�ǵ�(����� �Է�) (���е�->������������) ���¿�û(3A) ����
AA 55 30 DC 00 01 00 2C 00 3A 00 00 00 00 00 00 00 00 73 0D 0D ����(��Ⱓ �ڵ�) (������������->���е�) ���¿�û �޾��� ACK
AA 55 30 BC 00 01 00 2C 00 02 00 00 00 00 00 00 00 00 1B 0D 0D ����(��Ⱓ �ڵ�) (������������->���е�) '����'(02) ����
AA 55 30 DC 00 2C 00 01 00 02 00 00 00 00 00 00 00 00 3B 0D 0D ����(��Ⱓ �ڵ�) (���е�->������������) '����'���� �޾��� ACK

*/
//-----------------------------------------------------------------------------------------

#endif
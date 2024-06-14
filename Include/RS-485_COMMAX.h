#ifndef __RS_485_COMMAX_H
#define __RS_485_COMMAX_H

// �ڸƽ�----------------------------------------------------------------------------------------
#define	COMMAX_RES_DELAY_TIME			36		// �ּ� 30ms ~ �ִ� 50ms
#define	COMMAX_INTERVAL_TIME			25		// ���ؾ���
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
//-------------------------------------------------------------------------------------------- Command 22�� ���ۿ䱸
#define COMMAX_ELEVATOR_FAIL		0x80
#define COMMAX_ELEVATOR_SUCCESS		0x40
#define COMMAX_GAS_CLOSE_FAIL		0x20
#define COMMAX_GAS_CLOSE_SUCCESS	0x10
#define COMMAX_BATCH_LIGHT_ON		0x01
#define COMMAX_BATCH_LIGHT_OFF		0x00
//--------------------------------------------------------------------------------------------
#define	NIS_LIGHT_ID_COMM_1			0xDD		//����� ��������, ID Ȯ�ο� Ŀ�ǵ�
#define NIS_LIGHT_ID_COMM_2			0xBB
//--------------------------------------------------------------------------------------------
//D1 
//������ �ֻ��� ��Ʈ�� ���� ����(0), ����(1)�� ����. ������ ��Ʈ�� ���� ������ 16������ ǥ����.
//���� 28�� -> 0x1C, ���� 28�� -> 0x9C. ��, ������ 16���� �ٷ� ǥ��, ���ϴ� (1 << 7 | 16����) -> (1 << 7 | 0x1C) = 0x9C

typedef enum
{
    COMMAX_GAS_COMM_REQUEST				=   0x10,		//������
    COMMAX_GAS_COMM_CONTROL				=   0x11,		//������
    
	COMMAX_LIGHT_COMM_REQUEST			=   0x30,
    COMMAX_LIGHT_COMM_CONTROL			=   0x31,
    COMMAX_LIGHT_COMM_GROUP_CONTROL		=   0x3F,
	
	COMMAX_ELEC_COMM_REQUEST			=	0x79,
	COMMAX_ELEC_COMM_CONTROL			=	0x7A,
	COMMAX_ELEC_COMM_GROUP_CONTROL		=	0x7B,

	COMMAX_BATCH_BLOCK_COMM_REQUEST		=	0x20,
	COMMAX_BATCH_LIGHT_COMM_CONTROL		=	0x21,		//���� ��ġ�� �ϰ� �ҵ� ����ġ �뵵�� ���
	COMMAX_BATCH_BLOCK_COMM_CONTROL		=	0x22,		//����������, ����, ���⼳��, �������, �ϰ��ҵ�
	COMMAX_ELEVATOR_COMM_CONTROL		=	0x23,		//����������	>> ���������� �� ����ǥ�� ��� ������ �� �� ���
	// COMMAX_BATCH_ELEVATOR_COMM_CONTROL_		=	0x26,	//����������	>> ���������� �� ����ǥ�� ��� ���� �� �� ���
	COMMAX_BATCH_BLOCK_COMM_GROUP_CONTROL	=	0x2F	//�ϰ��ҵ�
	// COMMAX_BATCH_BLOCK_COMM_GROUP_CONTROL	=	0x2D,	//�������

}_MASTER_COMM_;
/*
<[01:11:23.286]20 01 00 00 00 00 00 21
<[01:11:23.324]A0 00 01 00 00 05 00 A6(����ġ���� �ϰ� �ҵ� ������ ��� ����) -> ���е忡�� �׷� ����� ���� �ҵ� byte_01
>> ����ġ���� �ϰ� �ҵ��� ������ ���е忡�� ���� �׷������ �ϰ� �ҵ�. �׷��ٸ� �׷������ �ҵ��� ���� �� ���� ���� ����.

<[01:11:39.209]20 01 00 00 00 00 00 21
<[01:11:39.255]A0 01 01 00 00 05 00 A7(����ġ���� �ϰ� ���� ������ ��� ����)
>> A0�������� �ϰ� �������� ���е忡 ������ ������ ���� ����ġ �ϰ� ����.

<[01:14:44.504]22 01 01 01 00 00 00 25(���е忡�� �ϰ� ���� ������ ���)
<[01:14:44.550]A2 01 01 00 00 00 00 A4
>> 22Ŀ�ǵ�� �ϰ� ������ ������ ���� ����ġ �ҵ� �� ����� ���·� �ϰ� ����.

<[01:15:09.239]22 01 00 01 00 00 00 24(���е忡�� �ϰ� �ҵ� ������ ���)	-> ���е忡�� �׷� ����� ���� �ҵ�
<[01:15:09.270]A2 00 01 00 00 00 00 A3
>> �� ��쵵 �׷������ �ҵ��� ������ ���� ���� ����.
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

typedef enum								//Command 22 ���� �䱸���� Byte03 ������ ����
{
	COMMAX_VERIATY_BATCHLIGHT 				= 0x01,		//�ϰ� �ҵ� On/Off ���� �䱸
	COMMAX_VERIATY_ELEC,								//������� On/Off ���� �䱸
	COMMAX_VERIATY_OUTING_SUCCESS,						//���� ������� ���� �㰡
	COMMAX_VERIATY_OUTING_FAIL,							//���� ���� �Ұ�
	COMMAX_VERIATY_GAS_CLOSE_SUCCESS,					//���� ����
	COMMAX_VERIATY_GAS_CLOSE_FAIL,						//���� ���� �Ұ�
	COMMAX_VERIATY_ELEVATOR_CALL_SUCCESS,				//���������� ȣ�� ����
	COMMAX_VERIATY_ELEVATOR_CALL_FAIL					//���������� ȣ�� ����
}_COMMAX_control_veriaty_;

typedef struct
{
	uint8_t		buf[COMMAX_MAX_BUF+1];		//��� ���α׷� ������� ���Ƿ� ���� ũ�� +1
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
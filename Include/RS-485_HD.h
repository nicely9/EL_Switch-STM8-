#ifndef __RS_485_HD_H
#define __RS_485_HD_H

// ������� ---------------------------------------------------------------------------------
#define	HYUNDAI_RES_DELAY_TIME	31		// 30ms ������ 300ms �̳��� ����(���� �� ����)
#define	HYUNDAI_INTERVAL_TIME	6		// 5ms ���� 6ms�� ����

#define HYUNDAI_WALLPAD_ID		0x01
#define HYUNDAI_STX				0xF7
#define HYUNDAI_VEN				0x01
#define HYUNDAI_ETX				0xEE
#define HD_MAX_BUF				255
#ifdef _HYUNDAI_PROTOCOL_
#define SWITCH_NUM				pG_Config->RS485_ID
#endif
//--------------------------------------------------------------------------------------------
#define NIS_LIGHT_ID_COMM_1		0xDD		//����� ��������, ID Ȯ�ο� Ŀ�ǵ�
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
	HD_DEV_USS					= 0x2A,		//�ϰ�����
	HD_DEV_LIGHT				= 0x19,		//�ϰ��ҵ�
	HD_DEV_LIGHT_DIMMING		= 0x1A,		//���
	HD_DEV_STANDBY_POWER		= 0x2C,		//�������
	HD_DEV_ELETRICITY			= 0x1F,		//�ܼ�Ʈ
	HD_DEV_GAS					= 0x1B,		//����
	HD_DEV_HEATER				= 0x18,		//�����
	HD_DEV_OUTING				= 0x17,		//����
	HD_DEV_SECURITY				= 0x16,		//���
	HD_DEV_ELEVATOR_CALL		= 0x34,
	HD_DEV_EXTERNAL_CONTACT		= 0x48,

	HD_SRV_POWER				= 0x40,		//����
	HD_SRV_DIMMING				= 0x42,		//�������
	HD_SRV_BATCHLIGHT			= 0x70,		//�ϰ�����
	HD_SRV_OPEN_n_CLOSE			= 0x43,		//����
	HD_SRV_POWERSAVING_MODE 	= 0x57,
	HD_SRV_ELEVATOR_UP_n_DOWN 	= 0x41
	// HD_SRV_HEATER			= 0x46,		//������� ����
	// HD_SRV_SECURITY			= 0x60,		//��� ����
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
	HD_CMD_BLOCK			= 0x01,		//����
	HD_CMD_UNBLOCK			= 0x02,		//��������
	HD_CMD_CLOSE			= 0x03,		//����
	HD_CMD_OPEN				= 0x04,		//����
	HD_CMD_UP				= 0x05,		//����������
	HD_CMD_DOWN				= 0x06		//����������

	// HD_CMD_HEATER_ON		= 0x01,		//����� ����
	// HD_CMD_HEATER_OFF		= 0x04,		//����� ����
	// HD_CMD_SECURITY_ON		= 0x07,		//��� ����
	// HD_CMD_SECURITY_OFF		= 0x08,		//��� ����
	// HD_CMD_OUTING_DELAY		= 0x0A,		//���� ����
	// HD_CMD_ENTRANCE_DELAY	= 0x0B,		//���� ����	
}_hyundai_cmd_;


typedef enum
{
	HD_ARG_NORMAL					= 0x00,
	HD_ARG_ARRIVE					= 0x01,
	HD_ARG_ERR						= 0x02,
	HD_ARG_CALL						= 0x03,		//��� ����
	HD_ARG_CALL_UP					= 0x05,		//���� ȣ��
	HD_ARG_CALL_DOWN				= 0x06,		//���� ȣ��
	HD_ARG_RUNNING_UP				= 0xA0,		//���� ����
	HD_ARG_RUNNING_DOWN				= 0xB0,		//���� ����
	HD_ARG_RUNNING_UP_n_CALL_UP		= 0xA5,		//������� ���� ȣ��
	HD_ARG_RUNNING_UP_n_CALL_DOWN	= 0xA6,		//������� ���� ȣ��
	HD_ARG_RUNNING_DOWN_n_CALL_UP 	= 0xB5,		//������� ���� ȣ��
	HD_ARG_RUNNING_DOWN_n_CALL_DOWN = 0xB6,		//������� ���� ȣ��
	HD_ARG_EXTERNAL_CONTACT			= 0x71
	// ���� ���� : -1(0xB1) ~ -15(0xBF)
	// ���� ���� : 0(0x00) ~ 99(0x99)
	// ȣ�� : 0x01 ~ 0xFF, 0x00(������)
}_hyundai_elevator_;

typedef enum
{
	HD_LOC_CIRCUIT_GROUP_ALL	= 0x00,
	HD_LOC_CIRCUIT_ALL			= 0,		//1�� ����ġ ��ü ����
	HD_LOC_CIRCUIT_1,						//1�� ����ġ 1 ȸ�� ����
	HD_LOC_CIRCUIT_2,						//1�� ����ġ 2 ȸ�� ����
	HD_LOC_CIRCUIT_3,						//1�� ����ġ 3 ȸ�� ����
	HD_LOC_CIRCUIT_4,						//1�� ����ġ 4 ȸ�� ����
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
	uint8_t		buf[HD_MAX_BUF];//����
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
	* 30ms ������ 300ms �̳��� ����
	* Interval time�� 5ms
	------------------------------------------------------------------------------------------
	STX | LEN | VEN | DEV | TYPE | SRV | LOC | CMD | ARG1 ~ ARGn | CRC | ETX
	------------------------------------------------------------------------------------------
    STX
    0xF7�� ����

    LEN
    STX ~ ETX������ Packet ����
    
    VEN
    0x01

    DEV
    �Ϲ� ����       0x19
    ��� ����       0x1A

    TYPE
    ���� ��û�� ���� ������ ���� ��� ���� ���� �����Ѵ�(�� ,����ð��� 300ms�� �ʰ��Ͽ����� �ȵ�)
    ���¿䱸             0x01       (��ɻ�� : �������)
    ������             0x02       (��ɻ�� : �������)
    Response Success    0x04       (��ɻ�� : ����ü)
    
    SRV
    ����            0x40
    ��� ����       0x42
    �ϰ�����        0x70

    LOC
    ���� �Ϻ� : �׷� �ѹ�, ���� �Ϻ� : �׷� �� ���ID
    ���� �Ϻ��� �������� ���� ����ġ�� �ǹ�
    ���� �Ϻ��� �ش� ���� ����ġ�� ȸ�θ� �ǹ�
    �� �Ϻ��� 1 ~ 6 ���� ���
    
    ��ü �׷� ���� ���� �� : ���� �Ϻ��� �׷� �ѹ��� 0���� ����
    �ش� �׷� ��ü ���� �� : ���� �Ϻ��� ��� ID�� 0���� ����
    * ���� 0x11(1��  ����ġ�� 1�� ȸ�� : 11, 2�� ȸ�� : 12)

	CMD
    ON              0x01
    OFF             0x02
    NUMBER          0 ~ 7

    ARG
    ��ü ���� ���� �� �ش� ���� ���� ARG ���� ����
    ARG�� BYTE�� ȸ�� ���� ǥ��.
    ON : 0x01, OFF : 0x02
    1���� ��� ��ü ���� ����� 1BYTE, 6���� ��� ��ü ���� ���� �� 6BYTE
    ���� ���� �� ���� ���� ǥ��

    CRC
    STX ~ CRC�������� Exclusive OR

    ETX
    0xEE�� ����
*/

#endif


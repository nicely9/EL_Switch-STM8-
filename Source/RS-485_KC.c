/************************************************************************************
	Project		: ���ڽĽ���ġ
	File Name	: RS-485_KOCOM.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2021/07/13
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "rs-485.h"
#include "el_switch.h"
#include "Debug.h"
#include "Timer.h"
#include "led.h"
#include "WDGnBeep.h"
#include "LCD.h"
#include "STPM3x_opt.h"

#ifdef _KOCOM_PROTOCOL_

void KOCOM_Data_Process(KOCOM_BUF	*pRx);
// void BATCH_BLOCK_STATE_Process(void);
uint8_t KOCOM_Crc(KOCOM_BUF *pTRx);

uint8_t KOCOM_LIGHT_State_Data_Conversion(uint8_t item);
uint8_t KOCOM_Dimming_Color_Data_Conversion(uint8_t item, KOCOM_BUF *TxBuf);
uint8_t KOCOM_BATCHLIGHT_State_Data_Conversion(uint8_t item);
uint8_t KOCOM_ELEC_State_Data_Conversion(uint8_t item);

uint8_t KOCOM_Batch_Light_State(uint8_t item);
uint8_t	KOCOM_Batch_Elec_State(uint8_t item);

void BATCH_BLOCK_Control(uint8_t control);
uint8_t Get_485_Elec_ID(void);
// ----------------------------------------------------------------------------------------
static	KOCOM_BUF		RxBuf, TxBuf;
	
#define	MAX_KOCOM_DATA_SEQUENCE		8
#define MAX_KOCOM_ELEC_DATA_SEQUENCE 2

uint8_t	KOCOM_LIGHT_ITEM_Sequence[MAX_KOCOM_DATA_SEQUENCE];
uint8_t	KOCOM_ELEC_ITEM_Sequence[MAX_KOCOM_ELEC_DATA_SEQUENCE];

uint8_t	old_LIGHT_State[MAX_KOCOM_DATA_SEQUENCE];
uint8_t	old_ELEC_State[MAX_KOCOM_ELEC_DATA_SEQUENCE];

uint8_t Store_Light_State[MAX_KOCOM_DATA_SEQUENCE];			//�ϰ� �ҵ� �� ���� ���¸� ����
uint8_t Store_Elec_State[MAX_KOCOM_ELEC_DATA_SEQUENCE];		//�ϰ� �ҵ�(����) �� ���� ���¸� ����

_DIMMING_BIT_		old_Dimming_Level;						// ��� ��
_Color_Temp_BIT_	old_Color_Temp_Level;					// ����� ��
_BLOCK_FLAG_		Block_Event_Flag;
_BLOCK_FLAG_		Block_Active_Flag;

uint8_t	Gu8_Batch_Light_State;									//�ϰ� �ҵ� �������� �ϰ� ���� �������� ����.
uint8_t Gu8_Batch_Elec_State;									//�ϰ� �ҵ�(����) �������� �ϰ� ����(����) �������� ����

uint8_t Gu8_BATCH_OPCODE;

uint8_t Gu8_3Way_Control_Flag;
uint8_t Gu8_3Way_Send_Flag;
uint8_t Gu8_Old_3Way_State;
uint8_t Gu8_3Way_B2L_Control_Flag;	//0x61(�ϰ�)->0x62(����) ���� �� ��� �÷���
uint8_t Gu8_Wallpad_3Way_Control_Flag;	//���е忡�� 3�� ���� ���� �ÿ� 3�� ��Ŷ �ϰ��� ���� �� �� �����

uint8_t Gu8_Light_Diff_Flag = 0;	//������¸𵨿��� ��ġ�� �̺�Ʈ �߻������� ����� ������ ����
uint8_t Gu8_Elec_Diff_Flag = 0;		//������¸𵨿��� ��ġ�� �̺�Ʈ �߻������� ����� ������ ����

uint8_t Batch_Light_Use_Tmr = 0;	//����ġ���� �ϰ� �ҵ� or ���� ���� �� ��ġ��� ���ޱ��� 3 ~ 5�� �ɸ��Ƿ� �ּ� 3�� �̳� �ϰ� �ҵ�/���� ����.
uint8_t Touch_Use_Tmr = 0;
uint16_t Gu8_RS_485_Tx_Add_Tmr = 0;

#define OPEN	0
#define CLOSE	1
#define REQUEST	2


uint8_t Gu8_Gas_State = 0;
uint8_t Gu8_Cook_State = 0;

// ----------------------------------------------------------------------------------------
void SET_KOCOM_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_KOCOM_DATA_SEQUENCE)
	{
		KOCOM_LIGHT_ITEM_Sequence[count]	= item;
	}
}

void SET_KOCOM_ELEC_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_KOCOM_ELEC_DATA_SEQUENCE)
	{
		KOCOM_ELEC_ITEM_Sequence[count]	= item;
	}
}

void Protocol_Data_Init(void)
{
	uint8_t	count, i;
	
	memset((void*)&RxBuf,		0,	sizeof(KOCOM_BUF));
	memset((void*)&TxBuf,		0,	sizeof(KOCOM_BUF));
	
	Gu16_GAS_Off_Tmr				= 0;
	
	Gu16_Elevator_Tmr			= 0;
	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_NON;
	
	Gu8_Batch_Light_State				= 0;				//�ϰ����� ��û ���� ��, �����ϱ� ����... �ٵ� �ʱⰪ���� �ϰ�������·� �ص� �Ǵ���..?
	Gu8_Batch_Elec_State				= 0;

	memset(KOCOM_LIGHT_ITEM_Sequence, 0, MAX_KOCOM_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	memset(KOCOM_ELEC_ITEM_Sequence, 0, MAX_KOCOM_ELEC_DATA_SEQUENCE);	// 2�� �׸� Ŭ����
	
	// �������� ������ �׸� ����
	// �����	 �ִ��׸�	: ���� 6��, ���� 4�� + ��� 2��
	// ����+��� �ִ��׸�	: ���� 4�� + ��� 2��, ����2�� + ���2�� + ���2��
	// ex) ���� 3��, ��� 2�� = ����1,����2,����3,���1,���2,0,0,0
	// ex) ���� 1��, ��� 1�� = ����1,���1,0,0,0,0,0,0
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// �������� ���� ����, ������� ������
	count	= 0;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
#endif
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// �������� ���� �������, ���� ������
	count	= 0;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_KOCOM_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
#endif

	for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
	{
		// old_LIGHT_State[i]		= KOCOM_LIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[i]);
		old_LIGHT_State[i]		= KOCOM_UNKOWN_FLAG;		//0x00, 0xFF�� ���� �÷��׸� �����ϱ� ?���� �ʱⰪ�� �� ���� ������� 0x01�� �Ѵ�.
		Store_Light_State[i]	= (uint8_t)KOCOM_Batch_Light_State(KOCOM_LIGHT_ITEM_Sequence[i]);
	}
	old_Dimming_Level.Dimming1			= pG_State->Dimming_Level.Dimming1;
	old_Dimming_Level.Dimming2			= pG_State->Dimming_Level.Dimming2;
	old_Color_Temp_Level.Color_Temp1	= pG_State->Color_Temp_Level.Color_Temp1;
	old_Color_Temp_Level.Color_Temp2	= pG_State->Color_Temp_Level.Color_Temp2;
	
	if(item2tsn(mapping_ITEM_3WAY_1))	Gu8_Old_3Way_State = 0xFF;

#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// ???����???����? ?????���� 
	count	= 0;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_KOCOM_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_KOCOM_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
	
	for(i = 0; i < MAX_KOCOM_ELEC_DATA_SEQUENCE; i++)
	{
		// old_ELEC_State[i]		= KOCOM_ELEC_State_Data_Conversion(KOCOM_ELEC_ITEM_Sequence[i]);
		old_ELEC_State[i]		= KOCOM_UNKOWN_FLAG;		//0x00, 0xFF�� ���� �÷��׸� �����ϱ� ?���� �ʱⰪ�� �� ���� ������� 0x01�� �Ѵ�.
		Store_Elec_State[i]		= KOCOM_Batch_Elec_State(KOCOM_ELEC_ITEM_Sequence[i]);
	}
#else
	count = 0;
	SET_KOCOM_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	SET_KOCOM_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
	/*
	���� ��� ���� �ƴ϶�, �������� �� �������� ��ü ��û�� ������, ���� ��(���� ���� �ƴϴ���)���� ������ ����� ���� ���� ����  ������ �Ǳ� ������
	���� ��������ʴ� ���̴��� �ӽ÷� ������.
	*/
#endif
}
// ----------------------------------------------------------------------------------------
void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr)
{
	if(tmr == 0)		// ������ ������ ���� �� X ms �ʰ��ϸ� ������ Ŭ����
	{
		RxBuf.buf[0]	= 0;
		RxBuf.count	    = 0;
	}
}
// ----------------------------------------------------------------------------------------
void Control_Diff_Check(void)
{
	int i;
	uint8_t touch_switch = 0;
	uint8_t	current_state[MAX_KOCOM_DATA_SEQUENCE];
	uint8_t current_elec_state[MAX_KOCOM_DATA_SEQUENCE];
	uint8_t current_3way_state;
	if(Gu8_Light_n_ETC_Touch_Flag)		//��ġ������ �÷��� �߻�
	{
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_	
		// printf("Batch %d, Gas %d, Cook %d, Elevator %d, Threeway %d\r\n", (uint16_t)Block_Active_Flag.Batch, (uint16_t)Block_Active_Flag.Gas, (uint16_t)Block_Active_Flag.Cook, (uint16_t)Block_Active_Flag.Elevator, (uint16_t)Block_Active_Flag.Threeway);
		if(Block_Active_Flag.Batch && Block_Active_Flag.Gas == 0 && Block_Active_Flag.Cook == 0)
		{
			if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF))			touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))	touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS);
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))	touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK);
			
			if(GET_Switch_State(touch_switch) == 0)				Gu8_BATCH_OPCODE = KOCOM_OP_BATCH_LIGHT_OFF;
			else												Gu8_BATCH_OPCODE = KOCOM_OP_BATCH_LIGHT_ON;	
			
			TxBuf.Result_SEND_Event_Flag	= 1;
			Block_Event_Flag.Batch 			= 1;
			Block_Active_Flag.Batch 		= 0;
			// printf("Batch %d, OPCODE %x\r\n", (uint16_t)GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS)), (uint16_t)Gu8_BATCH_OPCODE);
		}
		else if(Block_Active_Flag.Gas)	//���� ���� ���¿��� �������� �������� 
		{
			TxBuf.Result_SEND_Event_Flag	= 1;
			Block_Event_Flag.Gas 			= 1;
			Block_Active_Flag.Gas 			= 0;
		}
		else if(Block_Active_Flag.Cook)
		{
			TxBuf.Result_SEND_Event_Flag	= 1;
			Block_Event_Flag.Cook 			= 1;
			Block_Active_Flag.Cook 			= 0;
		}
		else if(Block_Active_Flag.Elevator)
		{
			printf("elevator call\r\n");
			TxBuf.Result_SEND_Event_Flag	= 1;
			Block_Event_Flag.Elevator		= 1;
			Block_Active_Flag.Elevator 		= 0;
		}
		else if(Block_Active_Flag.Threeway)
		{
			TxBuf.Result_SEND_Event_Flag	= 1;
			Block_Event_Flag.Threeway		= 1;
			Block_Active_Flag.Threeway 		= 0;
		}
#else		
		for(i = 0; i <MAX_KOCOM_DATA_SEQUENCE; i++)
		{
			current_state[i]		= KOCOM_LIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[i]);
			if(old_LIGHT_State[i] != current_state[i])
			{
				old_LIGHT_State[i] = current_state[i];
				TxBuf.Result_SEND_Event_Flag	= 1;
				Gu8_Light_Diff_Flag = 1;
				// if(Gu8_Elec_Diff_Flag)
				// {
				Gu8_Elec_Diff_Flag = 0;		//���е忡�� ������ ���� ��, ���� ���� ������ ���� �� �ܼ�Ʈ ���� �����Ͱ� ���� ��찡 �־ �߰���
				TxBuf.Result_SEND_CC_Count = 0;	//�ش� ������ ������ �߿� �ٸ� ����ġ ������ �������� ��, CC���� BC�� �ʱ�ȭ ���� �ʰ� BD, BE�� �ٸ� ���� �����Ͱ� ���۵Ǿ� �߰���.
				// }
			}		
		}
		if(pG_Config->Enable_Flag.PWM_Dimming == ENABLE_BIT_DIMMING_1)
		{
			if(old_Dimming_Level.Dimming1 != pG_State->Dimming_Level.Dimming1)
			{
				old_Dimming_Level.Dimming1 = pG_State->Dimming_Level.Dimming1;
				TxBuf.Result_SEND_Event_Flag	= 1;
			}
		}
		if(pG_Config->Enable_Flag.PWM_Dimming == ENABLE_BIT_DIMMING_2)
		{
			if(old_Dimming_Level.Dimming2 != pG_State->Dimming_Level.Dimming2)
			{
				old_Dimming_Level.Dimming2 = pG_State->Dimming_Level.Dimming2;
				TxBuf.Result_SEND_Event_Flag	= 1;
			}
		}
		if(pG_Config->Enable_Flag.PWM_Color_Temp == ENABLE_BIT_COLOR_TEMP_1)
		{
			if(old_Color_Temp_Level.Color_Temp1 != pG_State->Color_Temp_Level.Color_Temp1)
			{
				old_Color_Temp_Level.Color_Temp1 = pG_State->Color_Temp_Level.Color_Temp1;
				TxBuf.Result_SEND_Event_Flag	= 1;
			}
		}
		if(pG_Config->Enable_Flag.PWM_Color_Temp == ENABLE_BIT_COLOR_TEMP_1)
		{
			if(old_Color_Temp_Level.Color_Temp2 != pG_State->Color_Temp_Level.Color_Temp2)
			{
				old_Color_Temp_Level.Color_Temp2 = pG_State->Color_Temp_Level.Color_Temp2;
				TxBuf.Result_SEND_Event_Flag	= 1;
			}
		}
		if(item2tsn(mapping_ITEM_3WAY_1))
		{
			if(Gu8_3Way_Control_Flag)
			{
				current_3way_state = GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
				if(Gu8_Old_3Way_State != current_3way_state)
				{
					if(Gu8_3Way_B2L_Control_Flag)
					{
						Gu8_Light_Diff_Flag			= 1;
						Gu8_3Way_B2L_Control_Flag	= 0;
						// printf("1-1-1, %d\r\n", (uint16_t)Gu8_3Way_B2L_Control_Flag);
					}
					else
					{
						// printf("1-1-2, %d\r\n", (uint16_t)Gu8_3Way_B2L_Control_Flag);
					}
					Gu8_3Way_Send_Fla				= 1;
					TxBuf.Result_SEND_Event_Flag	= 1;
					Gu8_Old_3Way_State = GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
					
				}
				else	//�ϰ� ����ġ���� ���� ��Ŷ ���� �� ������ �� ���°� ���� ���� ���� �� ������ �۽����� �ʴ� ��� �־ �߰�
				{
					if(Gu8_3Way_B2L_Control_Flag)
					{
						Gu8_Light_Diff_Flag				= 1;
						Gu8_3Way_Send_Flag				= 1;
						TxBuf.Result_SEND_Event_Flag	= 1;
						Gu8_3Way_B2L_Control_Flag		= 0;
						// printf("1-2-1, %d\r\n", (uint16_t)Gu8_3Way_B2L_Control_Flag);
					}
					else
					{
						Gu8_Light_Diff_Flag				= 1;
						Gu8_3Way_Send_Flag				= 1;
						TxBuf.Result_SEND_Event_Flag	= 1;
						// printf("1-2-2, %d\r\n", (uint16_t)Gu8_3Way_B2L_Control_Flag);
					}
					Gu8_Old_3Way_State = GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
				}
			}
		}
#endif	
		Gu8_Light_n_ETC_Touch_Flag = 0;
	}
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// ???����???����? ?????����
	if(Gu8_Elec_Touch_Flag)
	{
		for(i = 0; i < MAX_KOCOM_ELEC_DATA_SEQUENCE; i++)
		{
			current_elec_state[i]	= KOCOM_ELEC_State_Data_Conversion(KOCOM_ELEC_ITEM_Sequence[i]);

			if(old_ELEC_State[i] != current_elec_state[i])
			{
				old_ELEC_State[i] = current_elec_state[i];
				TxBuf.Result_SEND_Event_Flag	= 1;
				Gu8_Elec_Diff_Flag				= 1;
				// if(Gu8_Light_Diff_Flag)
				// {
				Gu8_Light_Diff_Flag				= 0;
				TxBuf.Result_SEND_CC_Count		= 0;	//�ش� ������ ������ �߿� �ٸ� ����ġ ������ �������� ��, CC���� BC�� �ʱ�ȭ ���� �ʰ� BD, BE�� �ٸ� ���� �����Ͱ� ���۵Ǿ� �߰���.
				// }
			}
		}
		Gu8_Elec_Touch_Flag = 0;
	}
#endif
}

void Send_After_3_Transmission(KOCOM_BUF *pTx)
{
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
	if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
	{
		if(pTx->buf[KOCOM_F_ADH] == KOCOM_GAS_DEVICE && pTx->buf[KOCOM_F_CC] == 0xBE)
		{
			if(Gu8_Direct_Control)
			{
				Gu8_Light_n_ETC_Touch_Flag	= 1;	//���� ��Ŷ 3ȸ ���� �� �ϰ� ��Ŷ �۽��� ���� ���
				Block_Active_Flag.Batch		= 1;
				Gu8_RS_485_Tx_Add_Tmr		= 50;
			}
			if(Gu8_Gas_State == REQUEST)
			{
				Gu16_GAS_Off_Tmr = 10;
			}
		}
	}
	else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
	{
		if(pTx->buf[KOCOM_F_ADH] == KOCOM_COOK_DEVICE && pTx->buf[KOCOM_F_CC] == 0xBE)
		{
			if(Gu8_Direct_Control)
			{
				Gu8_Light_n_ETC_Touch_Flag = 1;
				Block_Active_Flag.Batch = 1;
				Gu8_RS_485_Tx_Add_Tmr = 50;
			}
			if(Gu8_Cook_State == REQUEST)
			{
				Gu16_GAS_Off_Tmr = 10;
			}
		}
	}
	else if(item2tsn(mapping_ITEM_GAS_n_COOK))
	{
		if(pTx->buf[KOCOM_F_ADH] == KOCOM_GAS_DEVICE && pTx->buf[KOCOM_F_CC] == 0xBE)
		{
			if(Gu8_Direct_Control)
			{
				Gu8_Light_n_ETC_Touch_Flag = 1;
				Block_Active_Flag.Cook = 1;
				Gu16_GAS_Off_Tmr = 10;
			}
		}
		if(pTx->buf[KOCOM_F_ADH] == KOCOM_COOK_DEVICE && pTx->buf[KOCOM_F_CC] == 0xBE)
		{
			Gu16_GAS_Off_Tmr = 10;
		}
	}
	
	if(item2tsn(mapping_ITEM_GAS))
	{
		if(pTx->buf[KOCOM_F_ADH] == KOCOM_GAS_DEVICE && pTx->buf[KOCOM_F_CC] == 0xBE)
		{
			if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)
			{
				Gu16_GAS_Off_Tmr = 10;
			}
		}
	}
	else if(item2tsn(mapping_ITEM_COOK))
	{
		if(pTx->buf[KOCOM_F_ADH] == KOCOM_COOK_DEVICE && pTx->buf[KOCOM_F_CC] == 0xBE)
		{
			if(GET_LED_State(item2tsn(mapping_ITEM_COOK)) == LED_FLASHING)
			{
				Gu16_GAS_Off_Tmr = 10;
			}
		}
	}

	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(pTx->buf[KOCOM_F_ASH] == KOCOM_ELEVATOR_DEVICE && pTx->buf[KOCOM_F_CC] == 0xBE)
		{
			if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)
			{
				Gu16_Elevator_Tmr = 10;
			}
		}
	}
#else	//�������, ��Ʈ��ũ ����ġ �� ���
	if(item2tsn(mapping_ITEM_3WAY_1))
	{
		if(Gu8_3Way_Control_Flag)
		{
			if(pTx->buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE && pTx->buf[KOCOM_F_ASL] == Get_485_ID() && pTx->buf[KOCOM_F_CC] == 0xBE)	//�����(ASH) �����ġ, �����(ASL) ��� ID, 3��° ������ �� ��
			{
				if(pTx->buf[KOCOM_F_OPCODE] == KOCOM_OP_3WAY_CONTROL)	//OPCODE�� ���� �� ���
				{
					if(Gu8_3Way_Send_Flag)
					{
						// Gu8_3Way_Control_Flag = 1;
						TxBuf.Result_SEND_Event_Flag	= 1;
						// printf("4\r\n");
					}
				}
			}
		}
	}
#endif
}

void RS485_Tx_Process(void)
{
	int i;
	uint8_t	item;
	KOCOM_BUF	*pTx;
	
	pTx = &TxBuf;
	
	Control_Diff_Check();	// ��ġ�� ���� ���ۿ����� �̺�Ʈ �����ؾ� ��(���� ������� �ʵ��� ������ ��)
	if(TxBuf.Result_SEND_Event_Flag)		//��ġ�� �̺�Ʈ �߻�������
	{
		if(TxBuf.SEND_Flag == 0 && TxBuf.Result_SEND_Flag == 0)		//
		{
			Gu8_RS_485_Tx_Tmr				= KOCOM_RES_DELAY_TIME;		// �� 250ms ���� ��� ���� KOCOM_RESULT_SEND_TIME���� ������. ��ġ�� �̺�Ʈ �߻��� 250ms ���� �ʿ����.
			TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
			if(Block_Event_Flag.Batch)			TxBuf.Result_SEND_OP_Code		= Gu8_BATCH_OPCODE;
			else if(Block_Event_Flag.Gas)		TxBuf.Result_SEND_OP_Code		= KOCOM_OP_GAS_BLOCK;
			else if(Block_Event_Flag.Elevator)	TxBuf.Result_SEND_OP_Code		= KOCOM_OP_ELEVATOR_CALL;
			else if(Block_Event_Flag.Cook)		TxBuf.Result_SEND_OP_Code		= (uint8_t)(KOCOM_OP_COOK_BLOCK | KOCOM_COOK_DEVICE);	//������ ��ž opcode ������ ���ؼ� �����.
			else if(Block_Event_Flag.Threeway)
			{
				TxBuf.Result_SEND_OP_Code		= KOCOM_OP_3WAY_CONTROL;
				// printf("Threeway\r\n");
			}
#endif
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
			TxBuf.Result_SEND_OP_Code		= KOCOM_OP_LIGHT_CONTROL;		// 0x00 ����
#endif
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
			if(Gu8_Light_Diff_Flag)		TxBuf.Result_SEND_OP_Code		= KOCOM_OP_LIGHT_CONTROL;				//�����ϱ� ���ؼ� ���������� OP_CONTROL�� OP_ELEC_CONTROL �� ��ü�� ������.
			else if(Gu8_Elec_Diff_Flag)	TxBuf.Result_SEND_OP_Code		= KOCOM_OP_ELEC_CONTROL;
#endif
    		TxBuf.Result_SEND_CC_Count		= 0;
		}
	}
#if defined _ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_ || defined _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_n_3WAY_	//�ϰ�/����, �ϰ�/��ž�� ��츸
	if(Gu8_RS_485_Tx_Tmr == 0 && Gu8_RS_485_Tx_Add_Tmr == 0 && (Gu8_RS_485_Rx_Tmr < pG_Config->Protocol_IntervalTime))		// ������ ���� �� 10ms(�Ǵ� 250ms) ���� & ������ ���� �� 1ms ������ ACK ����
#else
	if(Gu8_RS_485_Tx_Tmr == 0 && (Gu8_RS_485_Rx_Tmr < pG_Config->Protocol_IntervalTime))		// ������ ���� �� 10ms(�Ǵ� 250ms) ���� & ������ ���� �� 1ms ������ ACK ����
#endif
	{
		if(TxBuf.SEND_Flag)
		{
			TX_Queue_Clear(RS_485_PORT);
			Uart_PutsTxQ(RS_485_PORT, TxBuf.buf, TxBuf.count);
			
			if(G_Debug == DEBUG_HOST)
			{
				printf("TX(KOCOM) : ");
				for(i=0;i<TxBuf.count;i++)
				{
					printf("%02X ", (uint16_t)TxBuf.buf[i]);
				}
				printf("\n");
			}
			TxBuf.count		= 0;
			TxBuf.SEND_Flag	= 0;

			if(TxBuf.Result_SEND_Flag == KOCOM_SINGLE_ID)			// ������ ����� ������
			{
				Gu8_RS_485_Tx_Tmr		= 25;	// �� 250ms ���� ��� ���� 0xbc ack �� ���, ������
				
			}		//������ or 0xBC�� ��û���� ��� ������۽� �� 25ms �� ���� ��ŵǴ� ��찡 �־ �߰���. ���� ������ 250ms.
			if(TxBuf.Result_SEND_OP_Code == KOCOM_OP_BATCH_LIGHT_ON || TxBuf.Result_SEND_OP_Code == KOCOM_OP_BATCH_LIGHT_OFF)
			{
				TxBuf.SEND_Flag				= 0;
				TxBuf.Result_SEND_Flag		= 0;
				TxBuf.Result_SEND_OP_Code	= 0;
				TxBuf.Result_SEND_CC_Count	= 0;
				Gu8_Direct_Control = 0;
			}
			else if(TxBuf.Result_SEND_OP_Code == KOCOM_OP_LIGHT_CONTROL)	//20240425
			{
				if((TxBuf.buf[KOCOM_F_ADH] == KOCOM_3WAY_LIGHT_DEVICE && TxBuf.buf[KOCOM_F_ASH] == KOCOM_3WAY_BATCH_DEVICE) \
				|| (TxBuf.buf[KOCOM_F_ADH] == KOCOM_3WAY_BATCH_DEVICE && TxBuf.buf[KOCOM_F_ASH] == KOCOM_3WAY_LIGHT_DEVICE))
				{
					TxBuf.SEND_Flag				= 0;
					TxBuf.Result_SEND_Flag		= 0;
					TxBuf.Result_SEND_OP_Code	= 0;
					TxBuf.Result_SEND_CC_Count	= 0;
				}
			}
		}
		else if(TxBuf.Result_SEND_Flag == KOCOM_SINGLE_ID)			// ������ ��� �����Ͱ� ������	//else if - > if
		{
			Gu8_RS_485_Tx_Tmr		= KOCOM_RESULT_SEND_TIME;		//���� �� 250ms ���� ��� ����, ack ������ ù ����, �ٵ� �����ۿ��� ������ ��. KOCOM_RESULT_SEND_TIME�ϸ� �������϶� 450ms �̻� ����.
			if(TxBuf.Result_SEND_CC_Count >= 3)		// 3ȸ || ��ġ �̺�Ʈ �߻� ��, �ٸ� ��ġ �̺�Ʈ �߻� �� ������ 3ȸ �̻��� �Ǿ ��� �������� �Ǵ� �����־� �߰���.
			{
				Send_After_3_Transmission(pTx);
				TxBuf.SEND_Flag				= 0;
				TxBuf.Result_SEND_Flag		= 0;
				TxBuf.Result_SEND_OP_Code	= 0;
				TxBuf.Result_SEND_CC_Count	= 0;
				Gu8_Light_Diff_Flag 		= 0;	//��ġ�� ���ϴ� �̺�Ʈ �÷��� ������ 3ȸ �Ϸ� �� �ʱ�ȭ
				Gu8_Elec_Diff_Flag 			= 0;	//��ġ�� �߻��ϴ� �̺�Ʈ �÷��� ������ 3ȸ �Ϸ� �� �ʱ�ȭ
			}
			else									// ????��?? 3??? ???????????����,
			{
				// ������ ���� �� ���� ���μ������� ����
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
				pTx->count	= 0;
				pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_1;
				pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_2;
				pTx->buf[pTx->count++]	= KOCOM_HD;
				if(TxBuf.Result_SEND_OP_Code == KOCOM_OP_BATCH_LIGHT_ON || TxBuf.Result_SEND_OP_Code == KOCOM_OP_BATCH_LIGHT_OFF)
				{				
					pTx->buf[pTx->count++]	= KOCOM_CC_NOT_ACK;																//����� ����
					pTx->buf[pTx->count++]	= 0x00;																			// PCNT
					pTx->buf[pTx->count++]	= KOCOM_LIGHT_DEVICE;															// KOCOM_WALLPAD, 		ADH ���� -> ���е�
					pTx->buf[pTx->count++]	= KOCOM_GROUP_ID;																// KOCOM_WALLPAD_ID,	ADL
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;
					pTx->buf[pTx->count++]	= TxBuf.Result_SEND_OP_Code;													//OP-CODE;	
				}
			//ASL
				else if(TxBuf.Result_SEND_OP_Code == KOCOM_OP_GAS_BLOCK)
				{
					pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);						//����� ����
					pTx->buf[pTx->count++]	= 0x00;																			// PCNT
					pTx->buf[pTx->count++]	= KOCOM_GAS_DEVICE;															// KOCOM_WALLPAD, 		ADH ���� -> ���е�
					pTx->buf[pTx->count++]	= Get_485_ID();																	// KOCOM_WALLPAD_ID,	ADL
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;
					pTx->buf[pTx->count++]	= TxBuf.Result_SEND_OP_Code;													//OP-CODE;	
				}
				else if(TxBuf.Result_SEND_OP_Code == (KOCOM_OP_COOK_BLOCK | KOCOM_COOK_DEVICE))
				{
					pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);						//����� ����
					pTx->buf[pTx->count++]	= 0x00;																			// PCNT
					pTx->buf[pTx->count++]	= KOCOM_COOK_DEVICE;															// KOCOM_WALLPAD, 		ADH ���� -> ���е�
					pTx->buf[pTx->count++]	= Get_485_ID();																	// KOCOM_WALLPAD_ID,	ADL
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;
					pTx->buf[pTx->count++]	= (uint8_t)(TxBuf.Result_SEND_OP_Code ^ KOCOM_COOK_DEVICE);													//OP-CODE;	
				}
				else if(TxBuf.Result_SEND_OP_Code == KOCOM_OP_ELEVATOR_CALL)
				{
					pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);						//����� ����
					pTx->buf[pTx->count++]	= 0x00;																			// PCNT
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;															// KOCOM_WALLPAD, 		ADH ���� -> ���е�
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;																// KOCOM_WALLPAD_ID,	ADL
					pTx->buf[pTx->count++]	= KOCOM_ELEVATOR_DEVICE;
					pTx->buf[pTx->count++]	= Get_485_ID();
					pTx->buf[pTx->count++]	= TxBuf.Result_SEND_OP_Code;													//OP-CODE;	
				}
				else if(TxBuf.Result_SEND_OP_Code == KOCOM_OP_STATE_REQ || TxBuf.Result_SEND_OP_Code == KOCOM_OP_LIGHT_CONTROL)
				{
					if(Gu8_3Way_Control_Flag)
					{
						// printf("opcode 00\r\n");
						pTx->buf[pTx->count++]	= KOCOM_CC_NOT_ACK;			//CC
						pTx->buf[pTx->count++]	= 0x00;						//PCNT
						pTx->buf[pTx->count++]	= KOCOM_3WAY_LIGHT_DEVICE;	//���� ����ġ 3�� �ּ�
						pTx->buf[pTx->count++]	= 0x00;						//3�� ��ȣ
						pTx->buf[pTx->count++]	= KOCOM_3WAY_BATCH_DEVICE;	//�ϰ� ����ġ 3�� �ּ�
						pTx->buf[pTx->count++]	= Get_485_ID();				//����� ID
						pTx->buf[pTx->count++]	= KOCOM_OP_LIGHT_CONTROL;	//OPCODE ����
						Gu8_3Way_Control_Flag = 0;
					}
					else
					{
						pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);						//����� ����
						pTx->buf[pTx->count++]	= 0x00;																			// PCNT
						pTx->buf[pTx->count++]	= KOCOM_WALLPAD;																// KOCOM_WALLPAD, 		ADH ���� -> ���е�
						pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;																// KOCOM_WALLPAD_ID,	ADL
						pTx->buf[pTx->count++]	= KOCOM_BATCH_DEVICE;
						pTx->buf[pTx->count++]	= Get_485_ID();
						pTx->buf[pTx->count++]	= TxBuf.Result_SEND_OP_Code;													//OP-CODE;
					}
				}
				else
				{
					pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);					//����� ����
					pTx->buf[pTx->count++]	= 0x00;																		// PCNT
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;															// KOCOM_WALLPAD, 		ADH ���� -> ���е�
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;															// KOCOM_WALLPAD_ID,	ADL
					pTx->buf[pTx->count++]	= KOCOM_BATCH_DEVICE;
					pTx->buf[pTx->count++]	= Get_485_ID();
					pTx->buf[pTx->count++]	= TxBuf.Result_SEND_OP_Code;	//OP-CODE;
					pTx->buf[pTx->count++]	= 0;							//DATA_0
					pTx->buf[pTx->count++]	= 0;							//DATA_1
					pTx->buf[pTx->count++]	= 0;							//DATA_2
					pTx->buf[pTx->count++]	= 0;							//DATA_3
					pTx->buf[pTx->count++]	= 0;							//DATA_4
					pTx->buf[pTx->count++]	= 0;							//DATA_5
					pTx->buf[pTx->count++]	= 0;							//DATA_6
					pTx->buf[pTx->count++]	= 0;							//�߰� DATA_7
				}

				switch(TxBuf.Result_SEND_OP_Code)		// ??��???? ����?����?����?�� ????????
				{
					default:
						printf("dafault\r\n");
						break;
#ifdef COMM_THREEWAY
					case KOCOM_OP_3WAY_CONTROL:
						if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
						{
							pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//DATA_0
						}
						else
						{
							pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_0
						}
						pTx->buf[pTx->count++]			= 0;								//DATA_1
						pTx->buf[pTx->count++]			= 0;								//DATA_2
						pTx->buf[pTx->count++]			= 0;								//DATA_3
						pTx->buf[pTx->count++]			= 0;								//DATA_4
						pTx->buf[pTx->count++]			= 0;								//DATA_5
						pTx->buf[pTx->count++]			= 0;								//DATA_6
						pTx->buf[pTx->count++]			= 0;								//�߰� DATA_7
						Block_Event_Flag.Threeway		= 0;					
						TxBuf.Result_SEND_Event_Flag	= 0;
						break;
#endif			
					case KOCOM_OP_BATCH_LIGHT_ON:
						pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//DATA_0
						pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//DATA_1
						pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//DATA_2
						pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//DATA_3
						pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//DATA_4
						pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//DATA_5
						pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//DATA_6
						pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//�߰� DATA_7
						Block_Event_Flag.Batch			= 0;					
						TxBuf.Result_SEND_Event_Flag	= 0;									// �÷��װ� ������ �� ��� ���� �����͸� �����Ͽ����� Ŭ����
						break;
					case KOCOM_OP_BATCH_LIGHT_OFF:
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_0
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_1
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_2
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_3
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_4
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_5
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_6
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//�߰� DATA_7
						Block_Event_Flag.Batch			= 0;
						TxBuf.Result_SEND_Event_Flag	= 0;									// �÷��װ� ������ �� ��� ���� �����͸� �����Ͽ����� Ŭ����
						break;
					case KOCOM_OP_GAS_BLOCK:
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_0
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_1
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_2
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_3
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_4
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_5
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_6
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//�߰� DATA_7
						Block_Event_Flag.Gas			= 0;
						TxBuf.Result_SEND_Event_Flag	= 0;									// �÷��װ� ������ �� ��� ���� �����͸� �����Ͽ����� Ŭ����
						TxBuf.SEND_Flag	= 1;	//240419�ʿ����� üũ
						break;
					case KOCOM_OP_COOK_BLOCK | KOCOM_COOK_DEVICE:
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_0
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_1
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_2
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_3
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_4
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_5
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_6
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//�߰� DATA_7
						Block_Event_Flag.Cook			= 0;
						TxBuf.Result_SEND_Event_Flag	= 0;									// �÷��װ� ������ �� ��� ���� �����͸� �����Ͽ����� Ŭ����
						break;
					case KOCOM_OP_ELEVATOR_CALL:
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_0
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_1
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_2
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_3
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_4
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_5
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_6
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//�߰� DATA_7
						Block_Event_Flag.Elevator		= 0;
						TxBuf.Result_SEND_Event_Flag	= 0;			// �÷��װ� ������ �� ��� ���� �����͸� �����Ͽ����� Ŭ����	
						break;
					case KOCOM_OP_PROTOCOL_VERSION:						// 0x4A	�������� ������û
			    		pTx->buf[KOCOM_F_DATA_0]		= KOCOM_BATCH_PROTOCOL_VERSION_H;	// ���� ����
						pTx->buf[KOCOM_F_DATA_1]		= KOCOM_BATCH_PROTOCOL_VERSION_L;	// ���� ����
						break;
					case KOCOM_OP_STATE_REQ:
						pTx->buf[KOCOM_F_OPCODE]	= 0x00;
						if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	//�ϰ�������¸�
						{
							pTx->buf[pTx->count++] = 0x02;
						}
						else if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == 0)	//�ϰ��ҵ���¸�
						{
							pTx->buf[pTx->count++] = 0x01;
						}
						pTx->buf[pTx->count++]	= 0;							//DATA_1
						pTx->buf[pTx->count++]	= 0;							//DATA_2
						pTx->buf[pTx->count++]	= 0;							//DATA_3
						pTx->buf[pTx->count++]	= 0;							//DATA_4
						pTx->buf[pTx->count++]	= 0;							//DATA_5
						pTx->buf[pTx->count++]	= 0;							//DATA_6
						pTx->buf[pTx->count++]	= 0;							//�߰� DATA_7
						break;
				}
#endif
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_) || defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
				pTx->count	= 0;
				pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_1;
				pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_2;
				pTx->buf[pTx->count++]	= KOCOM_HD;
				pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);	//����� ����
				pTx->buf[pTx->count++]	= 0x00;							// PCNT
				pTx->buf[pTx->count++]	= KOCOM_WALLPAD;				// KOCOM_WALLPAD, 		ADH ���� -> ���е�
				pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;				// KOCOM_WALLPAD_ID,	ADL
				if((RxBuf.buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)		//������ CC ���� ��û�϶�()
				{
					if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE)		//���е� -> ���� ����ġ ��û
					{
						pTx->buf[pTx->count++] = KOCOM_LIGHT_DEVICE;		//ASH = LIGHT_DEVICE
					}
					else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE)	//���е� -> ���� ����ġ ��û
					{
						pTx->buf[pTx->count++] = KOCOM_ELEC_DEVICE;			//ASH = ELEC_DEVICE
					}
					else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_WALLPAD)		//�������� ���е��϶�, -> CC�� 0xBC, ADH = WALLPAD�� ����ġ�� ��� ���� or ��� ������
					{
						pTx->buf[pTx->count++] = RxBuf.buf[KOCOM_F_ASH];		//ASH�� ������ ASH �� ����
					}
				}
				else if((RxBuf.buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		//������ CC ���� ACK �϶�
				{
					if(RxBuf.buf[KOCOM_F_ASH] == KOCOM_WALLPAD)						//������� ���е��϶�, -> CC�� 0xDC, ASH = WALLPAD�� ���е忡�� ������ ����� ���� ACK
					{
						pTx->buf[pTx->count++]	= RxBuf.buf[KOCOM_F_ADH];		//ASH ���� or ���� �������϶�, ���е� ���� ����...
					}
					// else if(RxBuf.buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE)
					// {
					// 	pTx->buf[pTx->count++]	= KOCOM_LIGHT_DEVICE;
					// }
					// else if(RxBuf.buf[KOCOM_F_ASH] == KOCOM_ELEC_DEVICE)
					// {
					// 	pTx->buf[pTx->count++]	= KOCOM_ELEC_DEVICE;
					// }
				}
				else															//�׷��û, �׿�(?) �϶�, ����� -> ����ġ or �ϰ������ �ϰ� ����ġ -> ����ġ�� ������
				{
					pTx->buf[pTx->count++]	= RxBuf.buf[KOCOM_F_ADH];
					// printf("else\r\n");
				}
				pTx->buf[pTx->count++]	= Get_485_ID();					//ASL
				pTx->buf[pTx->count++]	= TxBuf.Result_SEND_OP_Code;	//OP-CODE;
				pTx->buf[pTx->count++]	= 0;							//DATA_0
				pTx->buf[pTx->count++]	= 0;							//DATA_1
				pTx->buf[pTx->count++]	= 0;							//DATA_2
				pTx->buf[pTx->count++]	= 0;							//DATA_3
				pTx->buf[pTx->count++]	= 0;							//DATA_4
				pTx->buf[pTx->count++]	= 0;							//DATA_5
				pTx->buf[pTx->count++]	= 0;							//DATA_6
				pTx->buf[pTx->count++]	= 0;							//�߰� DATA_7

				switch(TxBuf.Result_SEND_OP_Code)		// ������ ����� ������
				{
					default:
						break;
					case KOCOM_OP_LIGHT_CONTROL:								// 0x00 ����
						// printf("control\r\n");
						if(Gu8_Light_Diff_Flag && Gu8_Elec_Diff_Flag == 0)		//����ġ���� ���� ���� ����
						{
							if(G_Trace)	printf("Light Diff\r\n");
							for(item=0;item<MAX_KOCOM_DATA_SEQUENCE;item++)
							{
								pTx->buf[KOCOM_F_DATA_0+item]	= KOCOM_LIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item]);
							}
							if(GET_LED_State(item2tsn(mapping_ITEM_LIGHT_1)) == LED_FLASHING)
							{
								pTx->buf[KOCOM_F_DATA_0] = 0x00;		//LED ������ ����ġ�� ��ü ���� OFF�� ���� ����ε�, �� ��� ����ġ ���°� ON�̱� ������ �����Ͱ� FF 00 00 .. ���� ���۵�
							}
																		//�׷��� ���е忡���� 1�������� ON���� ���� �����̱� ������ 1�� ������ LED �������̸� 00���� ������ ����.
							pTx->buf[KOCOM_F_ASH]		= KOCOM_LIGHT_DEVICE;
							TxBuf.Result_SEND_Event_Flag	= 0;			// �÷��װ� ������ �� ��� ���� �����͸� �����Ͽ����� Ŭ����
							TxBuf.SEND_Flag	= 1;
						}
						else if(Gu8_Elec_Diff_Flag && Gu8_Light_Diff_Flag == 0)	//����ġ���� ���� ���� ����
						{
							if(G_Trace)	printf("Elec Diff\r\n");
							for(item = 0; item < MAX_KOCOM_ELEC_DATA_SEQUENCE; item++)
							{
								pTx->buf[KOCOM_F_DATA_0 + item]		= KOCOM_ELEC_State_Data_Conversion(KOCOM_ELEC_ITEM_Sequence[item]);
							}							
							pTx->buf[KOCOM_F_ASH]		= KOCOM_ELEC_DEVICE;
							TxBuf.Result_SEND_Event_Flag	= 0;			// �÷��װ� ������ �� ��� ���� �����͸� �����Ͽ����� Ŭ����
							TxBuf.SEND_Flag	= 1;
						}					
						else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE && Gu8_Light_Diff_Flag == 0 && Gu8_Elec_Diff_Flag == 0)		//�׷� ����� ���е� -> ����ġ���� ACK �϶�. ���� ����ô� ���е� -> ����ġ���� ��û
						{
							if(G_Trace)	printf("Rx Light\r\n");
							for(item=0;item<MAX_KOCOM_DATA_SEQUENCE;item++)
							{
								pTx->buf[KOCOM_F_DATA_0+item]	= KOCOM_LIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item]);
							}
							pTx->buf[KOCOM_F_OPCODE]	= KOCOM_OP_LIGHT_CONTROL;
							TxBuf.Result_SEND_Event_Flag	= 0;
							TxBuf.SEND_Flag	= 1;
						}
						else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE && Gu8_Light_Diff_Flag == 0 && Gu8_Elec_Diff_Flag == 0)		//�׷� ����� ���е� -> ����ġ���� ACK �϶�. ���� ����ô� ���е� -> ����ġ���� ��û
						{
							if(G_Trace)	printf("Rx Elec\r\n");
							for(item = 0; item < MAX_KOCOM_ELEC_DATA_SEQUENCE; item++)
							{
								pTx->buf[KOCOM_F_DATA_0 + item]		= KOCOM_ELEC_State_Data_Conversion(KOCOM_ELEC_ITEM_Sequence[item]);
							}
							pTx->buf[KOCOM_F_OPCODE]	= KOCOM_OP_ELEC_CONTROL;
							TxBuf.Result_SEND_Event_Flag	= 0;
							TxBuf.SEND_Flag	= 1;
						}
						else if((RxBuf.buf[KOCOM_F_ADH] == KOCOM_WALLPAD) && (RxBuf.buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE))	//����ġ -> ���е���� ��� ����(������) �����͸� ������, �׷� ����� ���� ����ġ �����͸� ����.
						{
							if((RxBuf.buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
							{
								for(item=0;item<MAX_KOCOM_DATA_SEQUENCE;item++)
								{
									pTx->buf[KOCOM_F_DATA_0+item]	= KOCOM_LIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item]);
								}
								pTx->buf[KOCOM_F_OPCODE]	= KOCOM_OP_LIGHT_CONTROL;
								TxBuf.Result_SEND_Event_Flag	= 0;
								TxBuf.SEND_Flag	= 1;																
								// printf("ADH = WALLPAD, ASH = LIGHT, CC =%x\r\n", (uint16_t)RxBuf.buf[KOCOM_F_CC]);
							}
						}
						else if((RxBuf.buf[KOCOM_F_ADH] == KOCOM_WALLPAD) && (RxBuf.buf[KOCOM_F_ASH] == KOCOM_ELEC_DEVICE))	//����ġ -> ���е���� ��� ����(������) �����͸� ������, �׷� ����� ���� ����ġ �����͸� ����.
						{
							if((RxBuf.buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
							{
								for(item = 0; item < MAX_KOCOM_ELEC_DATA_SEQUENCE; item++)
								{
									pTx->buf[KOCOM_F_DATA_0 + item]		= KOCOM_ELEC_State_Data_Conversion(KOCOM_ELEC_ITEM_Sequence[item]);
								}
								pTx->buf[KOCOM_F_OPCODE]	= KOCOM_OP_ELEC_CONTROL;
								TxBuf.Result_SEND_Event_Flag	= 0;
								TxBuf.SEND_Flag	= 1;
								// printf("ADH = WALLPAD, ASH = ELEC, CC =%x\r\n", (uint16_t)RxBuf.buf[KOCOM_F_CC]);
							}
						}
						if(Gu8_3Way_Control_Flag && Gu8_3Way_Send_Flag && Gu8_Light_Diff_Flag == 0)
						{
							pTx->buf[KOCOM_F_CC]		= KOCOM_CC_NOT_ACK;
							pTx->buf[KOCOM_F_ADH]		= KOCOM_3WAY_BATCH_DEVICE;	//0x62
							pTx->buf[KOCOM_F_ADL]		= 0x00;
							pTx->buf[KOCOM_F_ASH]		= KOCOM_3WAY_LIGHT_DEVICE;	//0x61
							pTx->buf[KOCOM_F_ASL]		= Get_485_ID();
							pTx->buf[KOCOM_F_OPCODE]	= 0x00;
							for(item = 0; item < MAX_KOCOM_DATA_SEQUENCE; item++)
							{
								pTx->buf[KOCOM_F_DATA_0 + item]		= 0;
							}
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))	pTx->buf[KOCOM_F_DATA_0]	= KOCOM_ON_FLAG;
							else												pTx->buf[KOCOM_F_DATA_0]	= KOCOM_OFF_FLAG;
							TxBuf.Result_SEND_Event_Flag	= 0;
							TxBuf.SEND_Flag	= 1;
							Gu8_3Way_Send_Flag = 0;
							// printf("3\r\n");
						}
						break;
					case KOCOM_OP_COLOR_TEMP_CONTROL:					// 0x01 ���µ� ����
						pTx->buf[KOCOM_F_OPCODE]		= KOCOM_OP_COLOR_TEMP_CONTROL;
						for(item = 0; item < MAX_KOCOM_DATA_SEQUENCE; item++)
						{
							pTx->buf[KOCOM_F_DATA_0+item]	= KOCOM_Dimming_Color_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item], &TxBuf);
						}
						TxBuf.SEND_Flag	= 1;
						break;
					case KOCOM_OP_BATCH_LIGHT_REQ:						// 0x62 �ϰ����� �� ���¿䱸
						pTx->buf[KOCOM_F_OPCODE]		= Gu8_Batch_Light_State;
						for(item = 0; item < MAX_KOCOM_DATA_SEQUENCE; item++)
						{
							pTx->buf[KOCOM_F_DATA_0 + item]		= KOCOM_BATCHLIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item]);
						}
						TxBuf.SEND_Flag	= 1;
						break;
					case KOCOM_OP_PROTOCOL_VERSION:						// 0x4A	�������� ������û
						pTx->buf[KOCOM_F_OPCODE]		= KOCOM_OP_PROTOCOL_VERSION;
						if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE || RxBuf.buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE)
						{
							pTx->buf[KOCOM_F_DATA_0]		= KOCOM_LIGHT_PROTOCOL_VERSION_H;	// ���� ����
							pTx->buf[KOCOM_F_DATA_1]		= KOCOM_LIGHT_PROTOCOL_VERSION_L;	// ���� ����
						}
						else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE || RxBuf.buf[KOCOM_F_ASH] == KOCOM_ELEC_DEVICE)
						{
							pTx->buf[KOCOM_F_DATA_0]		= KOCOM_ELEC_PROTOCOL_VERSION_H;	// ���� ����
							pTx->buf[KOCOM_F_DATA_1]		= KOCOM_ELEC_PROTOCOL_VERSION_L;	// ���� ����							
						}
						TxBuf.SEND_Flag	= 1;
						break;
					case KOCOM_OP_DIM_LEVEL:							// 0x5A	��� �ִ� �ܰ���ȸ
					case KOCOM_OP_COLOR_TEMP_LEVEL:						// 0x5B	���µ� �ִ� �ܰ���ȸ, ȸ�� ����X : 0x00, �ܼ� ON/OFF : 0x01, ���µ� �ִ� �ܰ� �� : 0x02 ~ 0xFF
					case KOCOM_OP_COLOR_TEMP_REQ:						// 0x3B ���µ� ������ȸ, OFF : 0x00, ON : 0x01 ~ 0xFF
						if(pTx->buf[KOCOM_F_OPCODE] == KOCOM_OP_COLOR_TEMP_REQ)
						{
							pTx->buf[KOCOM_F_OPCODE]		= 0x01;			//���µ� ������ȸ�� ��� ����� OPCODe 0x01�� ����.
						}
						for(item = 0; item < MAX_KOCOM_DATA_SEQUENCE; item++)
						{
							pTx->buf[KOCOM_F_DATA_0 + item]	= KOCOM_Dimming_Color_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item], &TxBuf);
						}
						TxBuf.SEND_Flag	= 1;
						break;
					case KOCOM_OP_STATE_REQ:								// 0x3A ���� ��ȸ, ȸ�� OFF : 0x00, ȸ�� ON : 0xFF, ��� ȸ�� : 0x01 ~ 0xFE
						// printf("STATE REQ\r\n");
						pTx->buf[KOCOM_F_OPCODE]		= 0x00;				// ���� ��ȸ ����� opcode 0x00
						if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE)
						{
							for(item = 0; item < MAX_KOCOM_DATA_SEQUENCE; item++)
							{
								pTx->buf[KOCOM_F_DATA_0 + item]	= KOCOM_LIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item]);
							}
							pTx->buf[KOCOM_F_ASH] = KOCOM_LIGHT_DEVICE;
						}
						else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE)
						{
							for(item = 0; item < MAX_KOCOM_ELEC_DATA_SEQUENCE; item++)
							{
								pTx->buf[KOCOM_F_DATA_0 + item]		= KOCOM_ELEC_State_Data_Conversion(KOCOM_ELEC_ITEM_Sequence[item]);
							}
							pTx->buf[KOCOM_F_ASH] = KOCOM_ELEC_DEVICE;
						}
						else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_WALLPAD && RxBuf.buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE)
						{
							if((RxBuf.buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
							{
								for(item = 0; item < MAX_KOCOM_DATA_SEQUENCE; item++)
								{
									pTx->buf[KOCOM_F_DATA_0 + item]	= KOCOM_LIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item]);
								}
								pTx->buf[KOCOM_F_ASH] = KOCOM_LIGHT_DEVICE;
							}
						}
						else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_WALLPAD && RxBuf.buf[KOCOM_F_ASH] == KOCOM_ELEC_DEVICE)
						{
							if((RxBuf.buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
							{
								for(item = 0; item < MAX_KOCOM_ELEC_DATA_SEQUENCE; item++)
								{
									pTx->buf[KOCOM_F_DATA_0 + item]		= KOCOM_ELEC_State_Data_Conversion(KOCOM_ELEC_ITEM_Sequence[item]);
								}
								pTx->buf[KOCOM_F_ASH] = KOCOM_ELEC_DEVICE;								
							}
						}
						TxBuf.SEND_Flag	= 1;
						break;
				}
#endif
				pTx->buf[pTx->count++]	= KOCOM_Crc(pTx);
				pTx->buf[pTx->count++]	= KOCOM_EOT_1;
				pTx->buf[pTx->count++]	= KOCOM_EOT_2;
				
				TxBuf.Result_SEND_CC_Count++;
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
				TxBuf.SEND_Flag	= 1;
#endif
			}
		}
	}
}
// ----------------------------------------------------------------------------------------
uint8_t KOCOM_Crc(KOCOM_BUF *pTRx)
{
	uint8_t i, crc = 0;
	
	for(i = KOCOM_F_HD; i <= KOCOM_F_DATA_7; i++)
	{
		crc += pTRx->buf[i];
	}
	
	return crc;
}
uint8_t NIS_Crc(KOCOM_BUF *pTRx, uint8_t cal, uint8_t sel)		//����� 485��� �׽�Ʈ�� ���ؼ� �߰���.
{
	uint8_t i, crc = 0;
	uint8_t j = 0;

	if(sel == NIS_RX)		j = 1;
	else if(sel == NIS_TX)	j = 0;

	if(cal == 0)
	{
		for(i = 0; i < (uint8_t)(NIS_SUM_CRC - j); i++)
		{
			crc += pTRx->buf[i];
		}
	}
	else if(cal == 1)
	{
		for(i = 0; i < (uint8_t)(NIS_XOR_CRC - j); i++)
		{
			crc ^= pTRx->buf[i];
		}
	}
	return crc;
}
//
void RS_485_ID_RES(void)
{
	uint8_t Elec_Watt_MSB, Elec_Watt_LSB, prime_num_1, prime_num_2 = 0;
	KOCOM_BUF	*pTx;
	pTx = &TxBuf;
	
	Elec_Watt_MSB = (uint8_t)((int)Gu16_Elec_1_Watt + (int)Gu16_Elec_2_Watt);
	printf("Watt1 = %f Watt2 = %f\r\n", (double)Gu16_Elec_1_Watt, (double)Gu16_Elec_2_Watt);
	
	prime_num_1 = (uint8_t)((Gu16_Elec_1_Watt - (int)Gu16_Elec_1_Watt) * 10);
	prime_num_2	= (uint8_t)((Gu16_Elec_2_Watt - (int)Gu16_Elec_2_Watt) * 10);
	
	Elec_Watt_MSB = (uint8_t)((int)Gu16_Elec_1_Watt + (int)Gu16_Elec_2_Watt);
	Elec_Watt_LSB = (uint8_t)(prime_num_1 + prime_num_2);

	if(10 <= (prime_num_1 + prime_num_2))
	{
		Elec_Watt_MSB += 1;
		Elec_Watt_LSB -+ 10;
	}

	pTx->buf[pTx->count++]	= 0xCC;
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//�������� Ÿ��
	pTx->buf[pTx->count++]	= Get_485_ID();					//ID ��ȣ
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
	pTx->buf[pTx->count++]	= Get_485_Elec_ID();
#else
	pTx->buf[pTx->count++]	= 0;
#endif
	

	pTx->buf[pTx->count++]	= Elec_Watt_MSB;				//����1, 2�� ���� ��(����)
	pTx->buf[pTx->count++]	= Elec_Watt_LSB;				//����1, 2�� ���� ��(�Ҽ�)

	pTx->buf[pTx->count++]	= Gu8_ZeroCrossing_Err_Flag;	//����ũ�ν� ���� ���� 1�̸� Err, 0�̸� Pass

	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 1, NIS_TX);
	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 0, NIS_TX);
	TxBuf.SEND_Flag	= 1;
}
// ----------------------------------------------------------------------------------------
void Protocol_Process(uint8_t data)
{
	KOCOM_BUF	*pRx;
	uint8_t		crc, crc_xor = 0;
	uint8_t     cnt = 0;
	int i;
	
	pRx = &RxBuf;
	
	if(G_Debug == DEBUG_HOST_REALDATA)
	{
		if(data == KOCOM_PREAMBLE_1)	printf("\n");
		printf("%02X ", (uint16_t)data);
	}

	switch(pRx->count)
	{
		default:
			if((pRx->buf[KOCOM_F_PRE_1] != KOCOM_PREAMBLE_1) && (data == KOCOM_PREAMBLE_1))
			{
				pRx->count = 0;
			}
			break;
		case 1:		// STX1
			if((pRx->buf[KOCOM_F_PRE_1] != KOCOM_PREAMBLE_1) && (pRx->buf[KOCOM_F_PRE_1] != NIS_LIGHT_ID_COMM_1))
			{
				pRx->count = 0;
			}
			break;
		case 2:		// STX2
			if((pRx->buf[KOCOM_F_PRE_2] != KOCOM_PREAMBLE_2) && (pRx->buf[KOCOM_F_PRE_2] != NIS_LIGHT_ID_COMM_2))
			{
				pRx->count = 0;
			}
			break;
		case 3:			// Header
			if(pRx->buf[KOCOM_F_HD] != KOCOM_HD)
			{
				if((pRx->buf[KOCOM_F_PRE_1] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[KOCOM_F_PRE_2] != NIS_LIGHT_ID_COMM_2))
				{
					pRx->count = 0;
				}
			}
			break;
	}
	pRx->buf[pRx->count++] = data;

	if((pRx->buf[KOCOM_F_PRE_1] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[KOCOM_F_PRE_2] == NIS_LIGHT_ID_COMM_2))
	{
		if(pRx->count >= 8)
		{
			crc = NIS_Crc(pRx, 0, NIS_RX);
			crc_xor = NIS_Crc(pRx, 1, NIS_RX);
			if(crc == pRx->buf[7] && crc_xor == pRx->buf[6])
			{
				if(G_Debug == DEBUG_HOST)
				{
					printf("\nRX(KOCOM) : ");
					for(i=0;i<pRx->count;i++)
					{
						printf("%02X ", (uint16_t)pRx->buf[i]);
					}
					printf("\n");
				}
				KOCOM_Data_Process(pRx);				
			}
			else
			{
				if(crc_xor != pRx->buf[6])	printf("cal crc_xor[0x%02X] != buf crc_xor[0x%02X]", (uint16_t)crc_xor, (uint16_t)pRx->buf[6]);
				if(crc != pRx->buf[7])		printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[7]);
			}
			pRx->buf[0] = 0;
			pRx->count = 0;			
		}
	}
	else
	{
		if(pRx->count >= KOCOM_MAX_BUF)
		{
			if(pRx->buf[KOCOM_F_EOT_2] == KOCOM_EOT_2)
			{
				crc = KOCOM_Crc(pRx);
				
				if(crc == pRx->buf[KOCOM_F_FCC])
				{
					if(G_Debug == DEBUG_HOST)
					{
						printf("RX(KOCOM) : ");
						for(i=0;i<pRx->count;i++)
						{
							printf("%02X ", (uint16_t)pRx->buf[i]);
						}
						printf("\n");
					}
					KOCOM_Data_Process(pRx);
				}
				else
				{
					printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[KOCOM_F_FCC]);
				}
			}
			pRx->buf[0] = 0;
			pRx->count = 0;
		}
	}
}

void SET_DimmingLevel(uint8_t item, uint8_t Dimming_Level)		// ��ַ���
{
	switch(item)
	{
		case mapping_ITEM_DIMMING_LIGHT_1:
			if((uint8_t)Dimming_Level > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
			}
			else
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)Dimming_Level;
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if((uint8_t)Dimming_Level > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
			}
			else
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)Dimming_Level;
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
			break;
	}
}

void SET_DimmingColorLevel(uint8_t item, uint8_t Dimming_Level)		// ���µ�
{
	switch(item)
	{
		case mapping_ITEM_DIMMING_LIGHT_1:
			if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)		// ���µ� ��� ���� ���
			{
				if((uint8_t)Dimming_Level > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
				{
					pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// �ִ� ũ��� ����
				}
				else
				{
					pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)Dimming_Level;
				}
				/*if(GET_Switch_State(item2tsn(item)) == 0)		//���µ� ���� �����Ҷ� ���� ���������� ON
				{
					EventCtrl(item2tsn(item), ON);
				}*/
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)		// ���µ� ��� ���� ���
			{
				if((uint8_t)Dimming_Level > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
				{
					pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// �ִ� ũ��� ����
				}
				else
				{
					pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)Dimming_Level;
				}
				/*if(GET_Switch_State(item2tsn(item)) == 0)		//���µ� ���� �����Ҷ� ���� ���������� ON
				{
					EventCtrl(item2tsn(item), ON);
				}*/
			}
			break;
	}
}

uint8_t KOCOM_Batch_Light_State(uint8_t item)		//�ϰ� �ҵ� �� ���� ���� ���� ����.
{
	uint8_t ret = 0;
	switch(item)
	{
		default:
			ret = KOCOM_OFF_FLAG;
			break;		
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(GET_Switch_State(item2tsn(item)))
			{
				ret = KOCOM_ON_FLAG;
			}
			else
			{
				ret = KOCOM_OFF_FLAG;
			}
			break;
	}
	return ret;
}
uint8_t KOCOM_Batch_Elec_State(uint8_t item)
{
	uint8_t ret = 0;
	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn(item)))
			{
				ret = KOCOM_ON_FLAG;
			}
			else
			{
				ret = KOCOM_OFF_FLAG;
			}
			break;
	}
	return ret;
}
void KOCOM_BatchLight_Control(uint8_t item, uint8_t control_value)
{
	uint8_t tmr;
	tmr = Gu8_PowerSaving_Tmr;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	switch(item)
	{
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(control_value == KOCOM_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);						//���� OFF
			}
			else if(control_value == KOCOM_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);						//���� ON
			}
			break;
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// ����
			break;
	}
}

void KOCOM_Control(uint8_t item, uint8_t control_value)
{
	uint8_t	Flag = OFF;
	uint8_t tmr, touch_switch = 0;

	tmr	= Gu8_PowerSaving_Tmr;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	
	switch(item)	// ������ ������ ������
	{
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			touch_switch = item2tsn(item);
			if(control_value == KOCOM_ON_FLAG)
			{
				if(GET_Switch_State(touch_switch) == 0)		//����ġ OFF��
				{
					EventCtrl(item2tsn(item), ON);
					if(item == mapping_ITEM_3WAY_1)
					{
						Gu8_Wallpad_3Way_Control_Flag = 1;
					}
				}
			}
			else if(control_value == KOCOM_OFF_FLAG)
			{
				if(GET_Switch_State(touch_switch))			//����ġ ON�̸�
				{
					EventCtrl(item2tsn(item), OFF);
					if(item == mapping_ITEM_3WAY_1)
					{
						Gu8_Wallpad_3Way_Control_Flag = 1;
					}
				}
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
			touch_switch = item2tsn(item);
			if(control_value == KOCOM_ON_FLAG)	// OFF�� ���µ�&���� �������� �ҵ�
			{
				if(GET_Switch_State(touch_switch) == 0)		//����ġ OFF��
				{
					EventCtrl(item2tsn(item), ON);
				}
			}
			else if(control_value == KOCOM_OFF_FLAG)	// ����� ������ ON
			{
				if(GET_Switch_State(touch_switch))			//����ġ ON�̸�
				{
					EventCtrl(item2tsn(item), OFF);
				}
			}
			else if(control_value)					// �������� ��ַ���
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	// �����ִ� ���¿����� ������ ������ �� ����(�� �� ���޿��� ����� ����Ǹ� ������ ��)
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;		// LCD�� ��� �����ֱ� ����
					Gu8_LCD_DIM_Tmr					= 20;		// 2s ���� LCD ǥ��
					SET_DimmingLevel(item, control_value);		//�����ִ� ���¸� ��� ������ ����.
				}
				else if(GET_Switch_State(item2tsn((uint8_t)item)) == 0)	//�����ִ� ���¸�
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;		// LCD�� ��� �����ֱ� ����
					Gu8_LCD_DIM_Tmr					= 20;		// 2s ���� LCD ǥ��
					SET_DimmingLevel(item, control_value);
					EventCtrl(item2tsn(item), ON);				//�����ִ� ���¸� ��� ���� ���� �� ����
				}
			}
			break;
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// ����
			break;
	}
}

void KOCOM_ELEC_CONTROL(uint8_t item, uint8_t control_value)
{
	uint8_t tmr, touch_switch, Flag;

	tmr	= Gu8_PowerSaving_Tmr;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	
	switch(item)	// ������ ������ ������
	{
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_ELECTRICITY_1:		// ���1 ����ġ(PWM ����)
		case mapping_ITEM_ELECTRICITY_2:		// ���2 ����ġ(PWM ����)
			touch_switch = item2tsn(item);
			if(control_value == KOCOM_OFF_FLAG)
			{
				Flag = OFF;
				if(GET_Switch_State(touch_switch))	//����ġ ON�̸�
				{
					// EventCtrl(item2tsn(item), OFF);		//���е忡�� ����� ���� ���� �ϱ����ؼ� EventCtrl �����������.
					SET_Switch_State(touch_switch, Flag);
					SET_LED_State(touch_switch, Flag);
					PUT_RelayCtrl(item2ctrl(item), Flag);
					ALL_Electricity_Switch_LED_Ctrl();
					Beep(Flag);
				}
			}
			else if(control_value == KOCOM_ON_FLAG)
			{
				Flag = ON;
				if(GET_Switch_State(touch_switch) == 0)	//����ġ OFF�̸�
				{
					// EventCtrl(item2tsn(item), ON);
					SET_Switch_State(touch_switch, Flag);
					SET_LED_State(touch_switch, Flag);
					PUT_RelayCtrl(item2ctrl(item), Flag);
					ALL_Electricity_Switch_LED_Ctrl();
					Beep(Flag);				
				}
			}
			break;
#else		//���� ������� �ʴ� ���� �ƹ� ���� ����.
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			break;
#endif			
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// ����
			break;
	}
}

uint8_t KOCOM_BATCHLIGHT_State_Data_Conversion(uint8_t item)		//�ϰ� ���´� ���� ON/OFF�θ� ����
{
	uint8_t ret;

	switch(item)
	{
		default:
			ret = KOCOM_OFF_FLAG;
			break;
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				ret = KOCOM_ON_FLAG;
			}
			else
			{
				ret = KOCOM_OFF_FLAG;
			}
			break;
	}
	return ret;
}

uint8_t KOCOM_LIGHT_State_Data_Conversion(uint8_t item)
{
	uint8_t	ret;
	
	switch(item)
	{
		default:
			ret = KOCOM_OFF_FLAG;
			break;
			
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))		//���� ����ġ ���¸� ����
			{
				if(GET_LED_State(item2tsn(mapping_ITEM_LIGHT_1)) == LED_FLASHING)	//��ü ����OFF �� �� ����1�� ���� �ҵ� �� ���� ������ OFF��
				{
					ret = KOCOM_OFF_FLAG;
				}
				else
				{
					ret = KOCOM_ON_FLAG;
				}
				
			}
			else
			{
				ret = KOCOM_OFF_FLAG;
			}
			break;
			
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			if(GET_Switch_State(item2tsn((uint8_t)item)))		//���� ��� ����ġ ���������� ��� ������ ����
			{
				ret = (uint8_t)(pG_State->Dimming_Level.Dimming1);
			}
			else												//���� ��� ����ġ ���������� 0x00 ����
			{
				ret = KOCOM_OFF_FLAG;
			}
			break;
			
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				ret = (uint8_t)(pG_State->Dimming_Level.Dimming2);
			}
			else
			{
				ret = KOCOM_OFF_FLAG;
			}
			break;
	}
	
	return	ret;
}

uint8_t KOCOM_Dimming_Color_Data_Conversion(uint8_t item, KOCOM_BUF *TxBuf)
{
	uint8_t	ret = 0;
	switch(TxBuf->Result_SEND_OP_Code)
	{
		case KOCOM_OP_COLOR_TEMP_CONTROL:		//0x00 : ȸ�� OFF, 0x01 ~ 0xFF : ���µ� �ܰ�
		case KOCOM_OP_COLOR_TEMP_REQ:			//0x00 : ȸ�� OFF, 0x01 ~ 0xFF : ���µ� �ܰ�
			switch(item)
			{
				default:
					ret = KOCOM_OFF_FLAG;		//�Ϲ����� ON/OFF �϶� 0x00�� ǥ��???
					break;
					
				case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
					if(GET_Switch_State(item2tsn((uint8_t)item)))
					{
						if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)						//���µ� ��� ���̸�(��ָ� ����ϴ� �𵨿��� ��ȸ�Ǵ°� ����)
						{
							ret = (uint8_t)(pG_State->Color_Temp_Level.Color_Temp1);	//���� ���µ� ������ ����
						}
						else															//���µ� ��� ���� �ƴϸ�
						{
							ret = KOCOM_OFF_FLAG;										//��� ����ġ�� ON�̰�, ���µ��� ������� ������, ��, ��ָ� ����ϴ� �����϶� ��� ǥ��??? 
						}
					}
					else
					{
						ret = KOCOM_OFF_FLAG;											//�Ϲ������� OFF???
					}
					break;
					
				case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
					if(GET_Switch_State(item2tsn((uint8_t)item)))
					{
						if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)
						{
							ret = (uint8_t)(pG_State->Color_Temp_Level.Color_Temp2);		//���� ���µ� ������ ����
						}
						else															//���µ� ��� ���� �ƴϸ�
						{
							ret = KOCOM_OFF_FLAG;										//��� ����ġ�� ON�̰�, ���µ��� ������� ������, ��, ��ָ� ����ϴ� �����϶� ��� ǥ��??? 
						}
					}
					else
					{
						ret = KOCOM_OFF_FLAG;											//�Ϲ������� OFF???
					}
					break;
			}		
			break;
		case KOCOM_OP_COLOR_TEMP_LEVEL:			//0x00 : ȸ�� ����X, 0x01 : ȸ�ΰ� �ܼ� ON/OFF, 0x02 ~ 0xFF : ���µ� ����� �ִ°��, ���µ� �ִ� �ܰ� ��
		case KOCOM_OP_DIM_LEVEL:				//0x00 : ȸ�� ����X, 0x01 : ȸ�ΰ� �ܼ� ON/OFF, 0x02 ~ 0xFE : ��� ����� �ִ� ��� ��� �ִ� �ܰ� ��		
			switch(item)
			{
				default:
					ret = KOCOM_NO_CIRCUIT;		//�ش� ȸ�ΰ� �������� ������
					break;
				case mapping_ITEM_LIGHT_1:
				case mapping_ITEM_LIGHT_2:
				case mapping_ITEM_LIGHT_3:
				case mapping_ITEM_LIGHT_4:
				case mapping_ITEM_LIGHT_5:
				case mapping_ITEM_LIGHT_6:
				case mapping_ITEM_3WAY_1:
				case mapping_ITEM_3WAY_2:
					ret = KOCOM_LIGHT_CIRCUIT;		//�ش� ȸ�ΰ� �ܼ� ON / OFF �϶�
					break;
				case mapping_ITEM_DIMMING_LIGHT_1:
				case mapping_ITEM_DIMMING_LIGHT_2:
					if(TxBuf->Result_SEND_OP_Code == KOCOM_OP_COLOR_TEMP_LEVEL)
					{
						if(pG_Config->Enable_Flag.PWM_Color_Temp)		//�ش� ȸ�ΰ� ���µ� ����� �ִ� ���.
						{
							ret = pG_Config->Color_Temp_MAX_Level;		//���µ� �ִ� �ܰ� ��
						}
					}
					else if(TxBuf->Result_SEND_OP_Code == KOCOM_OP_DIM_LEVEL)
					{
						if(pG_Config->Enable_Flag.PWM_Dimming)
						{
							ret = pG_Config->Dimming_MAX_Level;
						}
					}
					else											//�ش� ȸ�ΰ� ���µ� ����� ���� ���. ��, ��� ������ ���.
					{
						ret = KOCOM_LIGHT_CIRCUIT;					//���µ� ���� ��� ��� ����� ���� �ܼ� ON/OFF�� ����. �������� ������ ���µ� ��û�϶� ������ ���� ���� ����.
					}
					break;
			}
			break;
	}
	
	return	ret;
}

uint8_t KOCOM_ELEC_State_Data_Conversion(uint8_t item)		//���� �������� ������ ���� �ʿ�
{
	uint8_t	ret = 0;
	
	switch(item)
	{
		default:
			ret = KOCOM_OFF_FLAG;
			break;
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_ELECTRICITY_1:
			if(GET_Switch_State(item2tsn(item)) && GET_LED_State(item2tsn(item)) == LED_ON)			//���� ON ����
			{
				// printf("ELEC1 SWITCH ON, LED OFF\r\n");
				ret		= KOCOM_ON_FLAG;				
			}
			else if(GET_Switch_State(item2tsn(item)) && GET_LED_State(item2tsn(item)) == LED_FLASHING)		//���� OFF�� �������� ����
			{
				// printf("ELEC1 SWITCH ON, LED FLASHING\r\n");
				ret		= KOCOM_OFF_FLAG;
			}
			else if(GET_Switch_State(item2tsn(item)) == 0 && GET_LED_State(item2tsn(item)) == LED_OFF)		//���� OFF ����
			{
				// printf("ELEC1 SWITCH OFF, LED ON\r\n");
				ret		= KOCOM_OFF_FLAG;				
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn(item)) && GET_LED_State(item2tsn(item)) == LED_ON)			//���� ON�� ����
			{
				// printf("ELEC2 SWITCH ON, LED OFF\r\n");
				ret		= KOCOM_ON_FLAG;				
			}
			else if(GET_Switch_State(item2tsn(item)) && GET_LED_State(item2tsn(item)) == LED_FLASHING)		//���� OFF�� �������� ����
			{
				// printf("ELEC2 SWITCH ON, LED FLASHING\r\n");
				ret		= KOCOM_OFF_FLAG;
			}
			else if(GET_Switch_State(item2tsn(item)) == 0 && GET_LED_State(item2tsn(item)) == LED_OFF)		//���� OFF ����
			{
				// printf("ELEC2 SWITCH OFF, LED ON\r\n");
				ret		= KOCOM_OFF_FLAG;				
			}
			break;
#else		//���� ������� �ʴ� ���� OFF Flag ����.
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			ret		=	KOCOM_OFF_FLAG;
			break;
#endif			
	}
	return	ret;
}
//------------------------------------------------------------------------------------------------------------------------------------------
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_) || defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)	// 2����?��???	- ????+????��??????����
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}
uint8_t Get_485_Elec_ID(void)
{
	return (uint8_t)(pG_Config->RS485_Elec_ID);
}
void KOCOM_ELEC_Model_ACK(KOCOM_BUF *pRx)
{
	uint8_t	i;
	KOCOM_BUF	*pTx;
	pTx = &TxBuf;
	
	pTx->count	= 0;
	pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_1;
	pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_2;
	pTx->buf[pTx->count++]	= KOCOM_HD;
	pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_ACK | (pRx->buf[KOCOM_F_CC] & 0x0F));		// ���ŵ� CC���� ���� ���� CC�� �޶���
	pTx->buf[pTx->count++]	= 0x00;															// PCNT
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASH];										// KOCOM_WALLPAD
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASL];										// KOCOM_WALLPAD_ID
	pTx->buf[pTx->count++]	= KOCOM_ELEC_DEVICE;
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_OPCODE];										// ���ŵ� OP-CODE���� ����
	for(i=KOCOM_F_DATA_0;i<=KOCOM_F_DATA_7;i++)
	{
		pTx->buf[pTx->count++]	= pRx->buf[i];												// ���ŵ� �����ͷ� ����
	}
	pTx->buf[pTx->count++]	= KOCOM_Crc(pTx);
	pTx->buf[pTx->count++]	= KOCOM_EOT_1;
	pTx->buf[pTx->count++]	= KOCOM_EOT_2;
	TxBuf.SEND_Flag	= 1;	
}

void KOCOM_LIGHT_Model_ACK(KOCOM_BUF *pRx)
{
	uint8_t	i;
	KOCOM_BUF	*pTx;
	pTx = &TxBuf;
	
	pTx->count	= 0;
	pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_1;
	pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_2;
	pTx->buf[pTx->count++]	= KOCOM_HD;
	pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_ACK | (pRx->buf[KOCOM_F_CC] & 0x0F));		// ���ŵ� CC���� ���� ���� CC�� �޶���
	pTx->buf[pTx->count++]	= 0x00;															// PCNT
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASH];										// KOCOM_WALLPAD
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASL];										// KOCOM_WALLPAD_ID
	pTx->buf[pTx->count++]	= KOCOM_LIGHT_DEVICE;
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_OPCODE];										// ���ŵ� OP-CODE���� ����
	for(i=KOCOM_F_DATA_0;i<=KOCOM_F_DATA_7;i++)
	{
		pTx->buf[pTx->count++]	= pRx->buf[i];												// ���ŵ� �����ͷ� ����
	}
	pTx->buf[pTx->count++]	= KOCOM_Crc(pTx);
	pTx->buf[pTx->count++]	= KOCOM_EOT_1;
	pTx->buf[pTx->count++]	= KOCOM_EOT_2;
	TxBuf.SEND_Flag	= 1;
}

void KOCOM_Data_Process(KOCOM_BUF	*pRx)
{
	uint16_t	i, cnt = 0;
	// uint8_t		item, touch_switch = 0;
	if((pRx->buf[KOCOM_F_PRE_1] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[KOCOM_F_PRE_2] != NIS_LIGHT_ID_COMM_2))		//485��� �׽�Ʈ�� ���� �����Ͱ� �ƴҶ�
	{
		if((pRx->buf[KOCOM_F_ADH] != KOCOM_LIGHT_DEVICE && pRx->buf[KOCOM_F_ADH] != KOCOM_ELEC_DEVICE && pRx->buf[KOCOM_F_ADH] != KOCOM_3WAY_LIGHT_DEVICE) || (pRx->buf[KOCOM_F_ADL] != Get_485_ID() && pRx->buf[KOCOM_F_ADL] != KOCOM_GROUP_ID))
		{
			if(TxBuf.Result_SEND_Flag == KOCOM_GROUP_ID)		// �׷������� ������
			{
				if(Get_485_ID() > KOCOM_LOWEST_ID)				// ���� ���� ID���� ũ��
				{
					if((pRx->buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE || pRx->buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE) && pRx->buf[KOCOM_F_ADL] == (Get_485_ID() - 1))	// ���е忡�� �ٷ� �� ��ġ�� ID�� ACK�� ������
					{
						if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)	// ����� ���� �����̸�	(0xDC, 0xDD, 0xDE)
						{
							TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
							TxBuf.Result_SEND_CC_Count		= 0;
						}
					}
					else if(pRx->buf[KOCOM_F_ADH] == KOCOM_WALLPAD && pRx->buf[KOCOM_F_ADL] == KOCOM_WALLPAD_ID)		// �������� ���е� ID�� ��� ��, �ٸ� ����ġ���� ���е�� ������ �۽� �� ���.
					{
						// printf("SWITCH -> WALLPAD\r\n");
						if((pRx->buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE || pRx->buf[KOCOM_F_ASH] == KOCOM_ELEC_DEVICE) && pRx->buf[KOCOM_F_ASL] == (Get_485_ID() - 1))	// �ٷ� �� ��ġ�Ⱑ
						{
							// printf("LIGHT -> WALLPAD, ID = %d\r\n", (uint16_t)Get_485_ID());
							if((pRx->buf[KOCOM_F_CC]) == 0xBE)																// 3ȸ ����������	(0xBC, 0xBD, 0xBE)
							{
								// printf("GROUP, ID = %d\r\n", (uint16_t)Get_485_ID());
								TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
								TxBuf.Result_SEND_CC_Count		= 0;
							}
						}
					}
				}
			}
			return;
		}
	}

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	if(pRx->buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE || pRx->buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE)
	{
		switch(pRx->buf[KOCOM_F_OPCODE])	// OP-CODE
		{
			//230303 �ϰ��ҵ� ���� ��Ŷ �����Ϸ��� �ּ� ó�� �ؾ���.
			//�ϰ� �ҵ� �� ��Ʈ��ũ ����ġ ������ ������ ����. 
			//��Ŷ���� �ϰ� �ҵ� �� ������ ������ �Ǿ� ���� OFF ���·� ���� ������ �Ǿ� �ϰ� �ҵ�/���� �� ���� ���� ȿ��?�� ���� ����.
			//���Ƿ� �ϰ� �ҵ� ���� ��Ŷ�� ������� �ʰ� ���� ���� On/Off�� ���� �������� �ϰ� �ҵ�/���� ���� �� ���� ���� ȿ���� �� �� ����.
			case KOCOM_OP_BATCH_LIGHT_OFF:						// 0x65 �ϰ��ҵ� ���(�������)
				if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)
				{
					if(Gu8_Batch_Light_State != KOCOM_BATCH_OFF_STATE)
					{
						for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
						{
							Store_Light_State[i] = KOCOM_Batch_Light_State(KOCOM_LIGHT_ITEM_Sequence[i]);								//���� ���� ����
							// KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);	// �׸�, ����
							KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], KOCOM_OFF_FLAG);	// �׸�, ����
						}
					}
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
					Gu8_Batch_Light_State = KOCOM_BATCH_OFF_STATE;					
				}
				break;
			case KOCOM_OP_BATCH_LIGHT_ON:						// 0x66 �ϰ����� ���(�������)
				if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)
				{
					for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
					{
						if(Gu8_Batch_Light_State == KOCOM_BATCH_ON_STATE)		//����ġ ó�� ���� or ���� �ϰ����� ������ ��, ���� ����� ���� ��� ���� �� ON. �ϰ� ���� ���¿��� �ϰ� ���� ����� ���� ��� �����.
						{
							// KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);	// �׸�, ����
							// KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], KOCOM_ON_FLAG);	// �׸�, ����
						}
						else if(Gu8_Batch_Light_State != KOCOM_BATCH_ON_STATE)											//�ϰ��ҵ� ������ ���� �ҵ� �� ���·� ����.
						{	
							// if(Store_Light_State[i] == (pRx->buf[KOCOM_F_DATA_0 + i]))		//�ϰ��ҵ� �� ����� ���� ���°� ON�̰� DATA ��û�� ����ON�̸�, �ҵ� �� ���·� ����
							// {
							// 	KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);	// �׸�, ����
							// }
							if(Gu8_Batch_Light_State == KOCOM_BATCH_OFF_STATE)
							{
								if(Store_Light_State[i] == KOCOM_ON_FLAG)						//�ϰ� �ҵ� �� ���� ���°� ON�� ��� ����. �ϰ��ҵ� ���¿��� ���������� ���� ON ������ ������ ���� ������.
								{
									KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], KOCOM_ON_FLAG);
								}
							}
							else		//���� or ó�� ���� �� �ϰ� ���� 0 ���� �ʱ�ȭ �� ���¿����� �ϰ� �ҵ� ���� ������ �޾����� ��� ���� ON
							{
								KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], KOCOM_ON_FLAG);
							}
							// else	//�ҵ� �� ����� ���� ���°� OFF �϶�
							// {
							// 	cnt++;
							// }
						}
					}
					// if(cnt >= MAX_KOCOM_DATA_SEQUENCE)		//�ϰ� �ҵ� �� ����� ������ ���°� ��� ���� OFF��, �ϰ� ����� ��� ���� ����.
					// {
					// 	for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
					// 	{
					// 		KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], KOCOM_ON_FLAG);
					// 	}
					// 	cnt = 0;
					// }
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
					Gu8_Batch_Light_State = KOCOM_BATCH_ON_STATE;					
				}				
				break;
			case KOCOM_OP_LIGHT_CONTROL:						// 0x00 ����
			case KOCOM_OP_COLOR_TEMP_CONTROL:					// 0x01 ���µ� ����
			case KOCOM_OP_BATCH_LIGHT_REQ:						// 0x62 �ϰ����� �� ���¿䱸
			case KOCOM_OP_BATCH_LIGHT_STATE_OFF:
			case KOCOM_OP_BATCH_LIGHT_STATE_ON:
			case KOCOM_OP_PROTOCOL_VERSION:						// 0x4A	�������� ������û
			case KOCOM_OP_DIM_LEVEL:							// 0x5A	��� �ܰ���ȸ
			case KOCOM_OP_COLOR_TEMP_LEVEL:						// 0x5B	���µ� �ܰ���ȸ
			case KOCOM_OP_STATE_REQ:							// 0x3A ��ü���� ��ȸ
			case KOCOM_OP_COLOR_TEMP_REQ:						// 0x3B ���µ� ������ȸ
				if(pRx->buf[KOCOM_F_OPCODE] == KOCOM_OP_LIGHT_CONTROL)					// 0x00 ������ ���
				{
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) != KOCOM_CC_ACK)			// ACK ������ ������ �絿���� �Ǵ� ������ �־ ��û �����Ͱ� �������� �����ϵ��� �߰���.
					{
						for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
						{
							KOCOM_Control(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);	// �׸�, ����
						}
					}
				}
				else if(pRx->buf[KOCOM_F_OPCODE] == KOCOM_OP_COLOR_TEMP_CONTROL)	// 0x01 ���µ� ������ ���
				{
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) != KOCOM_CC_ACK)
					{
						for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
						{
							SET_DimmingColorLevel(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);
						}
					}
				}
				
				if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// ������ ��û�̸�	(0xBC, 0xBD, 0xBE)
				{
					KOCOM_LIGHT_Model_ACK(pRx);		// OP-CODE	0x4A�� ����???? �����ġ ACK ����
					TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
					TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];		// ��������� OP-CODE
					TxBuf.Result_SEND_CC_Count		= 0;
				}
				else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)	// ����� ���� �����̸�	(0xDC, 0xDD, 0xDE)
				{
					if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())
					{
						TxBuf.SEND_Flag				= 0;
					}				
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_OP_Code		= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
					Gu8_Light_Diff_Flag 			= 0;	//��ġ�� �߻��ϴ� �̺�Ʈ �÷��� ������� 0���� �ʱ�ȭ
#ifdef COMM_THREEWAY
					if(pRx->buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE)	//ADH �����ġ, ACK ���� �� �� 
					{
						if(item2tsn(mapping_ITEM_3WAY_1))
						{
							if(Gu8_3Way_Send_Flag)
							{
								if(Gu8_3Way_B2L_Control_Flag == 0)	//�ϰ� ����ġ ���ؼ� ��� �ƴ� ��츸
								{
									Gu8_3Way_Control_Flag = 1;
									TxBuf.Result_SEND_Event_Flag = 1;
									// printf("2-1, %d\r\n", (uint16_t)Gu8_3Way_B2L_Control_Flag);
								}
								else
								{
									// printf("2-2, %d\r\n", (uint16_t)Gu8_3Way_B2L_Control_Flag);
									Gu8_3Way_B2L_Control_Flag = 0;
								}
							}
							else
							{
								if(Gu8_3Way_B2L_Control_Flag == 0)
								{
									if(Gu8_Wallpad_3Way_Control_Flag)
									{
										Gu8_3Way_Send_Flag = 1;
										Gu8_3Way_Control_Flag = 1;
										TxBuf.Result_SEND_Event_Flag = 1;
										Gu8_Wallpad_3Way_Control_Flag = 0;
										//���е忡�� 3�� ���� ���� �� �ϰ� ����ġ�� 3�� ��Ŷ�� ���� �ϱ� ����
										// printf("2-3, %d\r\n", (uint16_t)Gu8_3Way_B2L_Control_Flag);
									}
								}
								else
								{
									// printf("B > L\r\n");
									Gu8_3Way_B2L_Control_Flag = 0;
								}
							}
						}
					}
#endif
				}
				else if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)				// ACK ����
				{
					if(pRx->buf[KOCOM_F_ADL] == KOCOM_GROUP_ID)			// �׷� ID
					{
						if(Get_485_ID() == KOCOM_LOWEST_ID)							// ù��°�� �����ؾ� �� ID�̸�
						{
							Gu8_RS_485_Tx_Tmr				= 0;
							TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];
							TxBuf.Result_SEND_CC_Count		= 0;
						}
						else														// ���߿� �����ؾ� �� ID�̸�
						{
							TxBuf.Result_SEND_Flag			= KOCOM_GROUP_ID;
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];
							TxBuf.Result_SEND_CC_Count		= 0;
						}
					}
					else
					{
						if(Get_485_ID() == pRx->buf[KOCOM_F_ADL])
						{
							Gu8_RS_485_Tx_Tmr				= 0;
							TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];
							TxBuf.Result_SEND_CC_Count		= 0;							
						}
					}
				}
				break;
		}
	}
	
	else if(pRx->buf[KOCOM_F_ADH] == KOCOM_3WAY_LIGHT_DEVICE && pRx->buf[KOCOM_F_ASH] == KOCOM_3WAY_BATCH_DEVICE)	//20240429
	{
		if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)
		{
			if(item2tsn(mapping_ITEM_3WAY_1))
			{
				if(pRx->buf[KOCOM_F_DATA_0] == KOCOM_ON_FLAG)
				{
					if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == 0)
					{
						EventCtrl(item2tsn(mapping_ITEM_3WAY_1), ON);
					}
				}
				else if(pRx->buf[KOCOM_F_DATA_0] == KOCOM_OFF_FLAG)
				{
					if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
					{
						EventCtrl(item2tsn(mapping_ITEM_3WAY_1), OFF);
					}
				}
				TxBuf.Result_SEND_Event_Flag = 1;	//�ϰ�->�����ġ�� 3�� ���� ������ ���� �� ���� ��Ŷ ���е�� �����ϱ� ���ؼ� �߰���
				Gu8_Light_Diff_Flag = 1;	//�ϰ�->�����ġ�� 3�� ���� ������ ���� �� ���� ��Ŷ ���е�� �����ϱ� ���ؼ� �߰���
				Gu8_3Way_B2L_Control_Flag = 1;	//�ϰ� ����ġ ���ؼ� ������ ��츸
				// printf("B2L 1\r\n");
			}
		}
	}
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
	else if(pRx->buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE || pRx->buf[KOCOM_F_ASH] == KOCOM_ELEC_DEVICE)
	{
		switch(pRx->buf[KOCOM_F_OPCODE])
		{
			case KOCOM_OP_BATCH_ELEC_OFF:
				if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)
				{
					if(Gu8_Batch_Elec_State == KOCOM_BATCH_ON_STATE)
					{	
						for(i = 0; i < MAX_KOCOM_ELEC_DATA_SEQUENCE; i++)
						{
							Store_Elec_State[i] = KOCOM_Batch_Elec_State(KOCOM_ELEC_ITEM_Sequence[i]);
							// KOCOM_ELEC_CONTROL(KOCOM_ELEC_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);
							KOCOM_ELEC_CONTROL(KOCOM_ELEC_ITEM_Sequence[i], KOCOM_OFF_FLAG);
						}
					}
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
					Gu8_Batch_Elec_State = KOCOM_BATCH_OFF_STATE;
				}
				break;
			case KOCOM_OP_BATCH_ELEC_ON:
				if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)
				{
					for(i = 0; i < MAX_KOCOM_ELEC_DATA_SEQUENCE; i++)
					{
						if(Gu8_Batch_Elec_State == KOCOM_BATCH_ON_STATE)
						{
							// KOCOM_ELEC_CONTROL(KOCOM_ELEC_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);
							// KOCOM_ELEC_CONTROL(KOCOM_ELEC_ITEM_Sequence[i], KOCOM_ON_FLAG);
						}
						else if(Gu8_Batch_Elec_State != KOCOM_BATCH_ON_STATE)	//���� �ϰ��ҵ� ���¸� �ҵ� �� ���·� ����
						{
							// if(Store_Elec_State[i] == (pRx->buf[KOCOM_F_DATA_0 + i]))
							if(Gu8_Batch_Elec_State == KOCOM_BATCH_OFF_STATE)
							{
								if(Store_Elec_State[i] == KOCOM_ON_FLAG)
								{
									KOCOM_ELEC_CONTROL(KOCOM_ELEC_ITEM_Sequence[i], KOCOM_ON_FLAG);
								}
							}
							else		//���� or ó�� ���� �� �ϰ� ���� 0 ���� �ʱ�ȭ �� ���¿����� �ϰ� �ҵ� ���� ������ �޾����� ��� ���� ON
							{
								KOCOM_ELEC_CONTROL(KOCOM_ELEC_ITEM_Sequence[i], KOCOM_ON_FLAG);
							}							
						}
					}

					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
					Gu8_Batch_Elec_State = KOCOM_BATCH_ON_STATE;
				}
				break;
			case KOCOM_OP_ELEC_STATE_REQ:
			case KOCOM_OP_ELEC_CONTROL:
			case KOCOM_OP_PROTOCOL_VERSION:
				if(pRx->buf[KOCOM_F_OPCODE] == KOCOM_OP_ELEC_CONTROL)
				{
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) != KOCOM_CC_ACK)			// ACK ������ ������ �絿���� �Ǵ� ������ �־ ��û �����Ͱ� �������� �����ϵ��� �߰���.
					{
						for(i = 0; i < MAX_KOCOM_ELEC_DATA_SEQUENCE; i++)
						{
							// touch_switch = item2tsn(KOCOM_ELEC_ITEM_Sequence[i]);
							KOCOM_ELEC_CONTROL(KOCOM_ELEC_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);
						}
					}
				}
				if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// ������ ��û�̸�	(0xBC, 0xBD, 0xBE)
				{
					KOCOM_ELEC_Model_ACK(pRx);		// OP-CODE	0x4A�� ����???? �����ġ ACK ����
					TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
					TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];		// ��������� OP-CODE
					TxBuf.Result_SEND_CC_Count		= 0;
				}
				else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		// ����� ���� �����̸�	(0xDC, 0xDD, 0xDE)
				{
					if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())
					{
						TxBuf.SEND_Flag				= 0;
					}				
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_OP_Code		= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
					Gu8_Elec_Diff_Flag 				= 0;		//��ġ�� �߻��ϴ� �̺�Ʈ �÷��� ������� 0���� �ʱ�ȭ	
				}
				else if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)				// ACK ����
				{
					if(pRx->buf[KOCOM_F_ADL] == KOCOM_GROUP_ID)					// �׷� ID
					{
						if(Get_485_ID() == KOCOM_LOWEST_ELEC_ID)				// ù��°�� �����ؾ� �� ID�̸�
						{
							// Gu8_RS_485_Tx_Tmr				= pG_Config->Protocol_RES_DelayTime;
							Gu8_RS_485_Tx_Tmr				= 0;
							TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];
							TxBuf.Result_SEND_CC_Count		= 0;
						}
						else														// ���߿� �����ؾ� �� ID�̸�
						{
							TxBuf.Result_SEND_Flag			= KOCOM_GROUP_ID;
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];
							TxBuf.Result_SEND_CC_Count		= 0;
						}
					}
					else
					{
						if(Get_485_ID() == pRx->buf[KOCOM_F_ADL])
						{
							Gu8_RS_485_Tx_Tmr				= 0;
							TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];
							TxBuf.Result_SEND_CC_Count		= 0;							
						}
					}					
				}
				break;
		}
	}
#endif
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{	
		/*
		if(pRx->buf[2] != pG_Config->RS485_ID)
		{
			pG_Config->RS485_ID = pRx->buf[2];
			printf("Switch ID Change\r\n");
			Store_CurrentConfig();
		}*/
		RS_485_ID_RES();
	}
}
#endif	// defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)

#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}
void KOCOM_BATCH_BLOCK_Model_ACK(KOCOM_BUF *pRx)
{
	uint8_t	i;
	KOCOM_BUF	*pTx;
	pTx = &TxBuf;
	
	pTx->count	= 0;
	pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_1;
	pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_2;
	pTx->buf[pTx->count++]	= KOCOM_HD;
	pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_ACK | (pRx->buf[KOCOM_F_CC] & 0x0F));		// ���ŵ� CC���� ���� ���� CC�� �޶���
	pTx->buf[pTx->count++]	= 0x00;					// PCNT
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASH];		// KOCOM_WALLPAD
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASL];		// KOCOM_WALLPAD_ID
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ADH];
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_OPCODE];							// ���ŵ� OP-CODE���� ����
	for(i=KOCOM_F_DATA_0;i<=KOCOM_F_DATA_7;i++)
	{
		pTx->buf[pTx->count++]	= pRx->buf[i];										// ���ŵ� �����ͷ� ����
	}
	pTx->buf[pTx->count++]	= KOCOM_Crc(pTx);
	pTx->buf[pTx->count++]	= KOCOM_EOT_1;
	pTx->buf[pTx->count++]	= KOCOM_EOT_2;
	TxBuf.SEND_Flag	= 1;
}

void KOCOM_Data_Process(KOCOM_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item, floor, ground = 0;
	
	/*if(G_Debug  == DEBUG_HOST_REALDATA)
	{
		for(i = 0; i < KOCOM_MAX_BUF; i++)
		{
			printf("%02X ", (uint16_t)pRx->buf[i]);
		}
		printf("\n");
	}*/

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
	if(pRx->buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE && pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK && pRx->buf[KOCOM_F_ADL] == KOCOM_GROUP_ID)
	{
		switch(pRx->buf[KOCOM_F_OPCODE])
		{
			case KOCOM_OP_BATCH_LIGHT_OFF:						// 0x65 �ϰ��ҵ� ���(�������)
				BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
				TxBuf.Result_SEND_Flag			= 0;
				TxBuf.Result_SEND_CC_Count		= 0;
				Gu8_Batch_Light_State = KOCOM_BATCH_OFF_STATE;			
				break;
			case KOCOM_OP_BATCH_LIGHT_ON:						// 0x66 �ϰ����� ���(�������)
				BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);
				TxBuf.Result_SEND_Flag			= 0;
				TxBuf.Result_SEND_CC_Count		= 0;
				Gu8_Batch_Light_State = KOCOM_BATCH_ON_STATE;			
				break;
		}
	}
	else if(pRx->buf[KOCOM_F_ADH] == KOCOM_GAS_DEVICE || pRx->buf[KOCOM_F_ASH] == KOCOM_GAS_DEVICE)
	{
		if(item2tsn(mapping_ITEM_GAS) || item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS) || item2tsn(mapping_ITEM_GAS_n_COOK))
		{
			switch(pRx->buf[KOCOM_F_OPCODE])
			{
				case KOCOM_OP_GAS_BLOCK:
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
					{
						Gu8_Gas_State = CLOSE;
						BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//���е忡�� �������� ���� ��� ���� ���°� �ǰ�, ����ġ���� ���� ��û���̰�, ���� �����Ϳ��� ����.
						TxBuf.SEND_Flag					= 0;
						TxBuf.Result_SEND_Flag			= 0;			//���� �����ϰ� �������� ����. ������ ����⿡�� ��.
						TxBuf.Result_SEND_CC_Count		= 0;		
					}
					else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		//211117 �������� NG�� �߰���. (����ġ���� ���� ���� ���� �� ���е忡�� ����͵� �������� ���ܼ�)
					{
						if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())						//������ ������ ����.
						{
							TxBuf.SEND_Flag				= 0;
						}	
						TxBuf.Result_SEND_Flag			= 0;
						TxBuf.Result_SEND_OP_Code		= 0;
						TxBuf.Result_SEND_CC_Count		= 0;

						if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
						{
							if(pRx->buf[KOCOM_F_ADH] == KOCOM_GAS_DEVICE)	//AA 55 30 DC 00 2C 00 01 00...(���е�->�������ܱ�)
							{
								if(Gu8_Direct_Control)	//���� ��� ���� ���信�� �ϰ� �ҵ� ������ ����
								{
									Block_Active_Flag.Batch = 1;
									Gu8_Light_n_ETC_Touch_Flag = 1;
									//���� ������ �ó����� ���� �� �ϰ� �ҵ� ������ ����.
									Gu8_Direct_Control = 0;
									Gu8_RS_485_Tx_Add_Tmr = 50;
								}
							}			
						}
						else if(item2tsn(mapping_ITEM_GAS_n_COOK))
						{
							if(pRx->buf[KOCOM_F_ADH] == KOCOM_GAS_DEVICE)	//���� ������ ���� �� ��ž ������ ������.
							{
								if(Gu8_Direct_Control)
								{
									Block_Active_Flag.Cook = 1;
									Gu8_Light_n_ETC_Touch_Flag = 1;
									//��ž ������ �߰�
								}
							}
						} //����/��ž�� ���� ������ �޾��� �� ��ž�� ������� �����Ƿ� ���� ���� ������ ������ �������� �ʵ��� ��.
					}
					break;
				case KOCOM_OP_GAS_BLOCK_RELEASE:
					if(pRx->buf[KOCOM_F_ADH] == KOCOM_WALLPAD && pRx->buf[KOCOM_F_ASH] == KOCOM_GAS_DEVICE)		//���� ��������| �������ܱ� -> ���е�(����ġ�� ���е� �ּ� ���)
					{
						if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
						{
							if(item2tsn(mapping_ITEM_GAS_n_COOK))
							{
								Gu8_Gas_State = OPEN;
							}
							BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);	//���е忡�� ������������ ���� ��� �������� ���°� �ǰ�, ����ġ������ ���� �������� �Ұ���.
						}
						TxBuf.Result_SEND_Flag			= 0;
						TxBuf.Result_SEND_CC_Count		= 0;				
					}
					break;
				default:
				case KOCOM_OP_GAS_STATE_REQ:
					Gu8_Direct_Control = 0;
					break;
			}
		}
	}
	else if(pRx->buf[KOCOM_F_ADH] == KOCOM_COOK_DEVICE || pRx->buf[KOCOM_F_ASH] == KOCOM_COOK_DEVICE)
	{
		if(item2tsn(mapping_ITEM_COOK) || item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK) || item2tsn(mapping_ITEM_GAS_n_COOK))
		{
			switch(pRx->buf[KOCOM_F_OPCODE])
			{
				case KOCOM_OP_COOK_BLOCK:	//OP ������ ����
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
					{
						Gu8_Cook_State = CLOSE;
						BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//���е忡�� �������� ���� ��� ���� ���°� �ǰ�, ����ġ���� ���� ��û���̰�, ���� �����Ϳ��� ����.
						TxBuf.SEND_Flag				= 0;
						TxBuf.Result_SEND_Flag			= 0;			//���� �����ϰ� �������� ����. ������ ����⿡�� ��.
						TxBuf.Result_SEND_CC_Count		= 0;		
					}
					else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		//211117 �������� NG�� �߰���. (����ġ���� ���� ���� ���� �� ���е忡�� ����͵� �������� ���ܼ�)
					{
						if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())						//������ ������ ����.
						{
							TxBuf.SEND_Flag				= 0;
						}	
						TxBuf.Result_SEND_Flag			= 0;
						TxBuf.Result_SEND_OP_Code		= 0;
						TxBuf.Result_SEND_CC_Count		= 0;

						if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
						{
							if(pRx->buf[KOCOM_F_ADH] == KOCOM_COOK_DEVICE)	//AA 55 30 DC 00 2C 00 01 00...(���е�->�������ܱ�)
							{
								if(Gu8_Direct_Control)	//���� ��� ���� ���信�� �ϰ� �ҵ� ������ ����
								{
									Block_Active_Flag.Batch = 1;
									Gu8_Light_n_ETC_Touch_Flag = 1;
									Gu8_Direct_Control = 0;
									Gu8_RS_485_Tx_Add_Tmr = 50;
									//��ž ������ �ó����� ���� �� �ϰ� �ҵ� ������ ����.
								}
							}			
						}
					}
					break;
				case KOCOM_OP_COOK_BLOCK_RELEASE:	//OP ������ ����
					if(pRx->buf[KOCOM_F_ADH] == KOCOM_WALLPAD && pRx->buf[KOCOM_F_ASH] == KOCOM_COOK_DEVICE)		//���� ��������| �������ܱ� -> ���е�(����ġ�� ���е� �ּ� ���)
					{
						if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
						{
							if(item2tsn(mapping_ITEM_GAS_n_COOK))
							{
								Gu8_Cook_State = OPEN;
							}
							BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);	//���е忡�� ������������ ���� ��� �������� ���°� �ǰ�, ����ġ������ ���� �������� �Ұ���.
						}
						TxBuf.Result_SEND_Flag			= 0;
						TxBuf.Result_SEND_CC_Count		= 0;				
					}
					break;
				default:
				case KOCOM_OP_COOK_STATE_REQ:
					Gu8_Direct_Control = 0;
					break;
			}
		}
	}
	else if(pRx->buf[KOCOM_F_ADH] == KOCOM_ELEVATOR_DEVICE || pRx->buf[KOCOM_F_ASH] == KOCOM_ELEVATOR_DEVICE)
	{
		if(item2tsn(mapping_ITEM_ELEVATOR))
		{
			switch(pRx->buf[KOCOM_F_OPCODE])
			{
				case KOCOM_OP_ELEVATOR_CALL:
					if(pRx->buf[KOCOM_F_ADH] == KOCOM_ELEVATOR_DEVICE && pRx->buf[KOCOM_F_ASH] == KOCOM_WALLPAD)	//���������� ȣ��| ����� -> ����ġ
					{
						if(pRx->buf[KOCOM_F_DATA_0] != KOCOM_ELEVATOR_ARRIVE)
						{
							if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
							{
								BATCH_BLOCK_Control(SET__ELEVATOR_CALL);
								if(pRx->buf[KOCOM_F_DATA_0] == 0x00)		printf("ELEVATOR STATE STOP\r\n");
								else if(pRx->buf[KOCOM_F_DATA_0] == 0x01)	printf("ELEVATOR STATE DOWNWARD\r\n");
								else if(pRx->buf[KOCOM_F_DATA_0] == 0x02)	printf("ELEVATOR STATE UPWARD\r\n");
								if(pRx->buf[KOCOM_F_DATA_1])
								{
									ground = (uint8_t)((pRx->buf[KOCOM_F_DATA_1] & 0x80) >> 7);
									floor = (uint8_t)(pRx->buf[KOCOM_F_DATA_1] & 0x7F);
									if(ground)	printf("B%d FLOOR\r\n", (uint16_t)floor);
									else		printf("%d FLOOR\r\n",(uint16_t)floor);
								}
							}
							/*else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)	// ����� ���� �����̸�	(0xDC, 0xDD, 0xDE)
							{
								printf("DC\r\n");
								if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())
								{
									TxBuf.SEND_Flag				= 0;
								}							
								TxBuf.Result_SEND_Flag			= 0;
								TxBuf.Result_SEND_OP_Code		= 0;
								TxBuf.Result_SEND_CC_Count		= 0;
							}*/						
						}
						else if(pRx->buf[KOCOM_F_DATA_0] == KOCOM_ELEVATOR_ARRIVE)		//����
						{
							if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
							{
								BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);
							}
						}
					// pRx->buf[KOCOM_F_DATA_0] 0x00 : ����, 0x01 : ����, 0x02 :  ����, 0x03 : ����
					// pRx->buf[KOCOM_F_DATA_1]	0x00 ~ 0xFF : ����(�ֻ��� ��Ʈ 0 : ����, 1: ����), ���ڽ� ����ġ������ �ʿ����.
						if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// ������ ��û�̸�	(0xBC, 0xBD, 0xBE)
						{
							KOCOM_BATCH_BLOCK_Model_ACK(pRx);		// OP-CODE	0x4A�� ����???? �����ġ ACK ����
							TxBuf.Result_SEND_Flag			= 0;							// �������������� ACK �� ��� ���亸���� ����.
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];		// ��������� OP-CODE
							TxBuf.Result_SEND_CC_Count		= 0;
						}
						else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		//�ϰ�����ġ -> �������� ���������� �� ��û�� ���� ACK �϶�.
						{
							if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	//��û�� ���� �����̹Ƿ� ��û �� �� ���� ȣ�� �������� ó��
							{
								BATCH_BLOCK_Control(SET__ELEVATOR_CALL);						//���� ���������� �� (LED ON)
							}
							if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())						//������ ������ ����.
							{
								TxBuf.SEND_Flag				= 0;
							}							
							TxBuf.Result_SEND_Flag			= 0;
							TxBuf.Result_SEND_OP_Code		= 0;
							TxBuf.Result_SEND_CC_Count		= 0;						
						}
					}
					break;		
				case KOCOM_OP_ELEVATOR_CALL_FAIL:
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// ���е� -> ����ġ���� ��� �����̸�
					{
						BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);					// ���е� -> �ϰ�����ġ�� ȣ�� ���� ����. LED OFF
						KOCOM_BATCH_BLOCK_Model_ACK(pRx);
						TxBuf.Result_SEND_Flag			= 0;
						TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];;
						TxBuf.Result_SEND_CC_Count		= 0;
					}			
					break;
			}
		}
	}
	else if(pRx->buf[KOCOM_F_ADH] == KOCOM_BATCH_DEVICE || pRx->buf[KOCOM_F_ASH] == KOCOM_BATCH_DEVICE)
	{
		switch(pRx->buf[KOCOM_F_OPCODE])
		{
			case KOCOM_OP_PROTOCOL_VERSION:						// 0x4A	�������� ������û
			case KOCOM_OP_STATE_REQ:							//�ϰ� ����ġ������ �ϰ� ������¸� ��ȸ��.
			case KOCOM_OP_LIGHT_CONTROL:						//0x3A�� ��û������, ��� ������ OPCODE�� 0x00���� �ٲٰ� ��. �׶� ������ ������ �ް� ����� �� ������ ��Ȳ�� ���ܼ� �߰���
				if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// ������ ��û�̸�	(0xBC, 0xBD, 0xBE)
				{
					KOCOM_BATCH_BLOCK_Model_ACK(pRx);		// OP-CODE	0x4A�� ����???? �����ġ ACK ����
					TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
					TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];		// ��������� OP-CODE
					TxBuf.Result_SEND_CC_Count		= 0;
				}
				else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)	// ����� ���� �����̸�	(0xDC, 0xDD, 0xDE)
				{
					if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())
					{
						TxBuf.SEND_Flag				= 0;
					}					
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_OP_Code		= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
				}
				else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_NOT_ACK) == KOCOM_CC_NOT_ACK)		//211117 0x9C ���� �������� NG ���� �߰���.
				{
					if(pRx->buf[KOCOM_F_OPCODE] == KOCOM_OP_STATE_REQ)						//0x3A�� ���� ��û
					{
						if(Get_485_ID() == KOCOM_LOWEST_ID)							// ù��°�� �����ؾ� �� ID�̸�
						{
							Gu8_RS_485_Tx_Tmr				= 0;
							TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];
							TxBuf.Result_SEND_CC_Count		= 0;
						}						
					}
				}
				break;			
		}
	}
#ifdef COMM_THREEWAY
	else if(pRx->buf[KOCOM_F_ADH] == KOCOM_3WAY_BATCH_DEVICE && pRx->buf[KOCOM_F_ASH] == KOCOM_3WAY_LIGHT_DEVICE)
	{
		if(item2tsn(mapping_ITEM_3WAY_1))
		{
			if(pRx->buf[KOCOM_F_OPCODE] == KOCOM_OP_3WAY_CONTROL)
			{
				if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)
				{
					if(pRx->buf[KOCOM_F_DATA_0] == KOCOM_ON_FLAG)
					{
						if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == 0)
						{
							// EventCtrl(item2tsn(mapping_ITEM_3WAY_1), ON);
							SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), ON);	//EventCtrl ��� �� Block_Active_Flag.Threeway�� 1�� �Ǿ� ������ ��
							SET_LED_State(item2tsn(mapping_ITEM_3WAY_1), ON);
							Beep(ON);
						}
					}
					else if(pRx->buf[KOCOM_F_DATA_0] == KOCOM_OFF_FLAG)
					{
						if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
						{
							// EventCtrl(item2tsn(mapping_ITEM_3WAY_1) ,OFF);
							SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
							SET_LED_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
							Beep(OFF);
						}
					}
				}
			}
		}
	}
#endif
	if((pRx->buf[KOCOM_F_PRE_1] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[KOCOM_F_PRE_2] == NIS_LIGHT_ID_COMM_2))
	{	
		RS_485_ID_RES();
	}	
}

#endif	// def _ONE_SIZE_BATCH_BLOCK_MODEL_
//------------------------------------------------------------------------------------------------------------------------------------------
void BATCH_BLOCK_STATE_Process(void)
{
	if(item2tsn(mapping_ITEM_GAS))
	{
		if(Gu16_GAS_Off_Tmr == 0)
		{
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_GAS)) == LED_FLASHING)		// Ÿ�̸Ӱ� 0�̰� ���� ���� ��û���̸�
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);		//���� ��û ����ϰ� ���� ���� ��������. 
			}
		}
	}
	else if(item2tsn(mapping_ITEM_COOK))
	{
		if(Gu16_GAS_Off_Tmr == 0)
		{
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_COOK)) == LED_FLASHING)		// Ÿ�̸Ӱ� 0�̰� ���� ���� ��û���̸�
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);		//���� ��û ����ϰ� ���� ���� ��������. 
			}
		}
	}
	else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
	{
		if(Gu16_GAS_Off_Tmr == 0)
		{
			if(Gu8_Gas_State == REQUEST)
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
			}
		}
	}
	else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
	{
		if(Gu16_GAS_Off_Tmr == 0)
		{
			if(Gu8_Cook_State == REQUEST)
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
			}
		}
	}
	else if(item2tsn(mapping_ITEM_GAS_n_COOK))
	{
		if(Gu16_GAS_Off_Tmr == 0)
		{
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_GAS_n_COOK)) == LED_FLASHING)		// Ÿ�̸Ӱ� 0�̰� ���� ���� ��û���̸�
			{
				Gu8_Gas_State = 0;
				Gu8_Cook_State = 0;
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);		//���� ��û ����ϰ� ���� ���� ��������. 
			}
		}
	}

	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(Gu16_Elevator_Tmr == 0)
		{
			if((GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING) || (GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR))))		//LED Flashing(��û) ���̰ų�, ȣ�� ������ �����϶�, ���� �ð��� ������
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);		//�������·� ����.
			}
		}
	}
}

void BATCH_BLOCK_Control(uint8_t control)
{
	uint8_t	Flag;
	uint8_t	touch_switch = 0;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec

	switch(control)
	{
		case SET__BATCHLIGHT_OFF:
			if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF))
			{
				touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
				if(GET_Switch_State(touch_switch))	EventCtrl(touch_switch, OFF);
			}
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
			{
				touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS);
				if(GET_Switch_State(touch_switch))
				{
					if(Batch_Light_Use_Tmr == 0)
					{
						SET_Switch_State(touch_switch, OFF);
						SET_LED_State(touch_switch, OFF);
						Beep(OFF);
						PUT_RelayCtrl(item2ctrl(mapping_ITEM_BATCH_LIGHT_n_GAS), OFF);		// �׸���� ����
					}
				}
			}
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
			{
				touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK);
				if(GET_Switch_State(touch_switch))
				{
					if(Batch_Light_Use_Tmr == 0)
					{
						SET_Switch_State(touch_switch, OFF);
						SET_LED_State(touch_switch, OFF);
						Beep(OFF);
						PUT_RelayCtrl(item2ctrl(mapping_ITEM_BATCH_LIGHT_n_COOK), OFF);		// �׸���� ����
					}
				}
			}
			break;
		case SET__BATCHLIGHT_ON:
			if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF))
			{
				touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
				if(GET_Switch_State(touch_switch) == 0)	EventCtrl(touch_switch, ON);
			}
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
			{
				touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS);
				if(GET_Switch_State(touch_switch) == 0)
				{
					EventCtrl(touch_switch, ON);
				}
			}
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
			{
				touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK);
				if(GET_Switch_State(touch_switch) == 0)
				{
					EventCtrl(touch_switch, ON);
				}
			}
			break;
		case SET__GAS_CLOSE_REQUEST:		// ���������� ����ġ ���°� ���� �־�� ������ ������ �����̴�. ���� ���¶� ���� ����� �����ؾ� ��(�������ݿ� ����).
			if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
			{
				//������ �������� ���� ���� ��û��.
				Gu16_GAS_Off_Tmr	= 60;
				Gu8_Gas_State = REQUEST;
				// Beep(ON);
				if(G_Trace)	printf("CMD : REQUEST, State : !CLOSE -> REQUEST\r\n");
			}
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
			{
				Gu16_GAS_Off_Tmr	= 60;
				Gu8_Cook_State = REQUEST;
				if(G_Trace)	printf("CMD : REQUEST, State : !CLOSE -> REQUEST\r\n");
			}
			else
			{
				if(item2tsn(mapping_ITEM_GAS))				touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
				else if(item2tsn(mapping_ITEM_COOK))		touch_switch = item2tsn((uint8_t)mapping_ITEM_COOK);
				else if(item2tsn(mapping_ITEM_GAS_n_COOK))	touch_switch = item2tsn(mapping_ITEM_GAS_n_COOK);

				if(GET_Switch_State(touch_switch))		//���� �������϶�
				{
					Gu16_GAS_Off_Tmr	= 60;						// 60�� ��� �� LED �ҵ�(�� ���е忡�� close/open �����Ͱ� ���ŵǸ� �ش� ���·� ��ȯ), 
					if(item2tsn(mapping_ITEM_GAS))				Block_Active_Flag.Gas	= 1;
					else if(item2tsn(mapping_ITEM_COOK))		Block_Active_Flag.Cook	= 1;
					else if(item2tsn(mapping_ITEM_GAS_n_COOK))	Block_Active_Flag.Gas	= 1;

					Beep(ON);
					if(G_Trace)	printf("Gas/Cook State Close. Re-REQUEST\r\n");			//�������̶� ���û.
				}
				else													//���� �������� �ƴҶ�
				{
					Gu16_GAS_Off_Tmr	= 60;						// 60�� ��� �� LED �ҵ�(�� ���е忡�� close/open �����Ͱ� ���ŵǸ� �ش� ���·� ��ȯ), 
					if(item2tsn(mapping_ITEM_GAS))				Block_Active_Flag.Gas	= 1;
					else if(item2tsn(mapping_ITEM_COOK))		Block_Active_Flag.Cook	= 1;
					else if(item2tsn(mapping_ITEM_GAS_n_COOK))	Block_Active_Flag.Gas	= 1;

					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, LED_FLASHING);	// LED FLASHING
					Beep(ON);
					if(G_Trace)	printf("Gas/Cook REQUEST\r\n");
				}
			}
			break;
		case SET__GAS_CLOSE_STATE:
			if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
			{
				if(Gu8_Gas_State != CLOSE)	//���� ���� ���� �ƴ� ��(����, ��û)
				{
					Gu8_Gas_State = CLOSE;
					Beep(ON);
					if(G_Trace)	printf("CMD : CLOSE, State : !CLOSE -> CLOSE\r\n");
				}
				else
				{
					if(G_Trace)	printf("Gas/Cook Close\r\n");
				}
			}
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
			{
				if(Gu8_Cook_State != CLOSE)	//���� ���� ���� �ƴ� ��(����, ��û)
				{
					Gu8_Cook_State = CLOSE;
					Beep(ON);
					if(G_Trace)	printf("CMD : CLOSE, State : !CLOSE -> CLOSE\r\n");
				}
			}
			else
			{
				if(item2tsn(mapping_ITEM_GAS))				touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
				else if(item2tsn(mapping_ITEM_COOK))		touch_switch = item2tsn((uint8_t)mapping_ITEM_COOK);
				else if(item2tsn(mapping_ITEM_GAS_n_COOK))	touch_switch = item2tsn(mapping_ITEM_GAS_n_COOK);

				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == OFF)		//������� ���� ���°� �ƴϰų� �������� ��û��(LED ������)
				{
					if(item2tsn(mapping_ITEM_GAS_n_COOK))
					{
						if(Gu8_Gas_State == CLOSE || Gu8_Cook_State == CLOSE)	//�ϳ��� �����̸� LED�� ON
						{
							SET_LED_State(touch_switch, OFF);		// �����δ� LED ����
						}
						if(Gu8_Gas_State == CLOSE && Gu8_Cook_State == CLOSE)
						{
							SET_Switch_State(touch_switch, ON);	//��� ���� �Ǹ� ����ġ ���� ���·�.
						}
					}
					else
					{
						SET_Switch_State(touch_switch, ON);		// ������� ����(����)
						SET_LED_State(touch_switch, OFF);		// �����δ� LED ����
					}
					Beep(ON);
					if(G_Trace)	printf("GAS/Cook CLOSE\r\n");
				}
			}
			break;
		case SET__GAS_OPEN_STATE:
			if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
			{
				if(Gu8_Gas_State != OPEN)	//���� ���� ���� �ƴ� ��(����, ��û)
				{
					Gu8_Gas_State = OPEN;
					Beep(ON);
					if(G_Trace)	printf("CMD : OPEN, State : !OPEN -> OPEN\r\n");
				}
			}
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
			{
				if(Gu8_Cook_State != OPEN)	//���� ���� ���� �ƴ� ��(����, ��û)
				{
					Gu8_Cook_State = OPEN;
					Beep(ON);
					if(G_Trace)	printf("CMD : OPEN, State : !OPEN -> OPEN\r\n");
				}
			}
			else
			{
				if(item2tsn(mapping_ITEM_GAS))				touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
				else if(item2tsn(mapping_ITEM_COOK))		touch_switch = item2tsn((uint8_t)mapping_ITEM_COOK);
				else if(item2tsn(mapping_ITEM_GAS_n_COOK))	touch_switch = item2tsn(mapping_ITEM_GAS_n_COOK);

				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == ON)		//���� ��� �������̰ų� ���� ���� ��û��(LED ������)
				{
					if(item2tsn(mapping_ITEM_GAS_n_COOK))
					{
						if(Gu8_Gas_State != CLOSE && Gu8_Cook_State != CLOSE)	//������ ��ž ��� �������°� �ƴϸ� -> �������� ���� �� �ش� ���´� �̸� ó����.
						{
							SET_Switch_State(touch_switch, OFF);	// ������� ����
							SET_LED_State(touch_switch, ON);		// �����δ� LED ����
							if(G_Trace) printf("Gas, Cook All OPEN\r\n");
						}
					}
					else
					{
						SET_Switch_State(touch_switch, OFF);	// ������� ����
						SET_LED_State(touch_switch, ON);		// �����δ� LED ����
						if(G_Trace)	printf("GAS OPEN\r\n");
					}
					Beep(ON);
				}
			}
			break;
		case SET__ELEVATOR_REQUEST:																			//����ġ���� ���������� ��
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))		//����ġ OFF �ų� LED ���� Flashing�̸�
			// if(GET_Switch_State(touch_switch) == OFF)														//����ġ OFF��
			{
				Gu16_Elevator_Tmr = 60;																		//��û Ÿ�̸� 60�� �ʱ�ȭ. 0�̵Ǹ� ��û ��ҵǰ� LED OFF��.
				Block_Active_Flag.Elevator = 1;
				SET_Switch_State(touch_switch, OFF);															//����ġ OFF
				SET_LED_State(touch_switch, LED_FLASHING);													//LED Flashing
				// SET_LED_State(touch_switch, LED_OFF);														//LED ON
				Beep(ON);
				if(G_Trace)	printf("ELEVATOR REQEUST\r\n");
			}
			break;
		case SET__ELEVATOR_CALL:																			//����⿡�� ���������� ��
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == 0)
			{
				Gu16_Elevator_Tmr = 60;																			//�� ���°� �Ǹ� Ÿ�̸� 60�� �ʱ�ȭ. Ÿ�̸Ӱ� 0�Ǹ� ���� ������� ���ư�.
				touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
				SET_Switch_State(touch_switch, ON);																//����ġ ON
				SET_LED_State(touch_switch, OFF);																//LED ON
				Beep(ON);
				if(G_Trace)	printf("ELEVATOR CALL\r\n");
			}
			break;
		case SET__ELEVATOR_ARRIVE:
		case SET__ELEVATOR_CALL_FAIL:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			/*if(control == SET__ELEVATOR_ARRIVE)
			{
				if(GET_Switch_State(touch_switch))			//ȣ�� �����϶�
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, ON);				//���� or ȣ�� ���н� LED OFF					
					Beep(ON);
					// printf("ELEVATOR ARRIVE\r\n");
				}
			}
			else if(control == SET__ELEVATOR_CALL_FAIL)
			{
				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch))	//ȣ�� ��û ���°ų�, ȣ����� �� ��
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, ON);				//���� or ȣ�� ���н� LED OFF				
					// Beep(ON);									//���������� ���������� �����׽�Ʈ ���� �ʾ�, ��� �����ϴ��� �𸣹Ƿ� �ϴ� ������ ������.
				}
				// printf("ELEVATOR FAIL\r\n");
			}*/
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch))	//ȣ�� ��û ���°ų�, ȣ����� �� ��
			{
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, ON);				//���� or ȣ�� ���н� LED OFF
#ifdef _COMMAX_PROTOCOL_
				Beep(ON);										//�ڸƽ��� ���������� ���� ������ ������� �ʴ´ٰ� �Ͽ� �Ѵ� ������ �����.
#else
				if(control == SET__ELEVATOR_ARRIVE)				//�������� ������ ����. 
				{
					Beep(ON);
					if(G_Trace)	printf("ELEVATOR ARRIVE\r\n");
				}
#endif
			}
			break;
	}
}

//------------------------------------------------------------------------------------------------------------------------------------------
void Elevator_Process(void)
{
	;
}
//------------------------------------------------------------------------------------------------------------------------------------------
void ELEVATOR_Call(void)
{
    ;
}
//------------------------------------------------------------------------------------------------------------------------------------------
void ELEVATOR_Cancel(void)
{
   ;
}
// ----------------------------------------------------------------------------------------
#endif	// _KOCOM_PROTOCOL_

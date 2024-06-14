/************************************************************************************
	Project		: 전자식스위치
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

uint8_t Store_Light_State[MAX_KOCOM_DATA_SEQUENCE];			//일괄 소등 전 전등 상태를 저장
uint8_t Store_Elec_State[MAX_KOCOM_ELEC_DATA_SEQUENCE];		//일괄 소등(전열) 전 전열 상태를 저장

_DIMMING_BIT_		old_Dimming_Level;						// 디밍 값
_Color_Temp_BIT_	old_Color_Temp_Level;					// 전등색 값
_BLOCK_FLAG_		Block_Event_Flag;
_BLOCK_FLAG_		Block_Active_Flag;

uint8_t	Gu8_Batch_Light_State;									//일괄 소등 상태인지 일괄 점등 상태인지 구분.
uint8_t Gu8_Batch_Elec_State;									//일괄 소등(전열) 상태인지 일괄 점등(전열) 상태인지 구분

uint8_t Gu8_BATCH_OPCODE;

uint8_t Gu8_3Way_Control_Flag;
uint8_t Gu8_3Way_Send_Flag;
uint8_t Gu8_Old_3Way_State;
uint8_t Gu8_3Way_B2L_Control_Flag;	//0x61(일괄)->0x62(전등) 제어 시 사용 플래그
uint8_t Gu8_Wallpad_3Way_Control_Flag;	//월패드에서 3로 전등 제어 시에 3로 패킷 일괄로 전송 할 때 사용함

uint8_t Gu8_Light_Diff_Flag = 0;	//대기전력모델에서 터치로 이벤트 발생했을때 전등과 전열을 구분
uint8_t Gu8_Elec_Diff_Flag = 0;		//대기전력모델에서 터치로 이벤트 발생했을때 전등과 전열을 구분

uint8_t Batch_Light_Use_Tmr = 0;	//스위치에서 일괄 소등 or 점등 제어 후 장치기로 전달까지 3 ~ 5초 걸리므로 최소 3초 이내 일괄 소등/점등 금지.
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
	
	Gu8_Batch_Light_State				= 0;				//일괄상태 요청 했을 때, 응답하기 위함... 근데 초기값으로 일괄점등상태로 해도 되는지..?
	Gu8_Batch_Elec_State				= 0;

	memset(KOCOM_LIGHT_ITEM_Sequence, 0, MAX_KOCOM_DATA_SEQUENCE);	// 8개 항목 클리어
	memset(KOCOM_ELEC_ITEM_Sequence, 0, MAX_KOCOM_ELEC_DATA_SEQUENCE);	// 2개 항목 클리어
	
	// 프로토콜 데이터 항목 순서
	// 전등모델	 최대항목	: 전등 6개, 전등 4개 + 디밍 2개
	// 전등+대기 최대항목	: 전등 4개 + 대기 2개, 전등2개 + 디밍2개 + 대기2개
	// ex) 조명 3개, 디밍 2개 = 조명1,조명2,조명3,디밍1,디밍2,0,0,0
	// ex) 조명 1개, 디밍 1개 = 조명1,디밍1,0,0,0,0,0,0
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// 프로토콜 순서 조명, 디밍전등 순으로
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
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// 프로토콜 순서 디밍전등, 조명 순으로
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
		old_LIGHT_State[i]		= KOCOM_UNKOWN_FLAG;		//0x00, 0xFF로 상태 플래그를 설정하기 ?문에 초기값은 두 값과 관계없는 0x01로 한다.
		Store_Light_State[i]	= (uint8_t)KOCOM_Batch_Light_State(KOCOM_LIGHT_ITEM_Sequence[i]);
	}
	old_Dimming_Level.Dimming1			= pG_State->Dimming_Level.Dimming1;
	old_Dimming_Level.Dimming2			= pG_State->Dimming_Level.Dimming2;
	old_Color_Temp_Level.Color_Temp1	= pG_State->Color_Temp_Level.Color_Temp1;
	old_Color_Temp_Level.Color_Temp2	= pG_State->Color_Temp_Level.Color_Temp2;
	
	if(item2tsn(mapping_ITEM_3WAY_1))	Gu8_Old_3Way_State = 0xFF;

#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// ???¡¾???¡¤? ?????¡§ 
	count	= 0;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_KOCOM_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_KOCOM_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
	
	for(i = 0; i < MAX_KOCOM_ELEC_DATA_SEQUENCE; i++)
	{
		// old_ELEC_State[i]		= KOCOM_ELEC_State_Data_Conversion(KOCOM_ELEC_ITEM_Sequence[i]);
		old_ELEC_State[i]		= KOCOM_UNKOWN_FLAG;		//0x00, 0xFF로 상태 플래그를 설정하기 ?문에 초기값은 두 값과 관계없는 0x01로 한다.
		Store_Elec_State[i]		= KOCOM_Batch_Elec_State(KOCOM_ELEC_ITEM_Sequence[i]);
	}
#else
	count = 0;
	SET_KOCOM_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	SET_KOCOM_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
	/*
	전열 사용 모델이 아니라도, 프로토콜 상 전열모델을 전체 요청이 왔을때, 앞의 모델(전열 모델이 아니더라도)에서 응답을 해줘야 뒤의 전열 모델이  응답이 되기 때문에
	전열 사용하지않는 모델이더라도 임시로 적용함.
	*/
#endif
}
// ----------------------------------------------------------------------------------------
void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr)
{
	if(tmr == 0)		// 마지막 데이터 수신 후 X ms 초과하면 데이터 클리어
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
	if(Gu8_Light_n_ETC_Touch_Flag)		//터치했을때 플래그 발생
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
		else if(Block_Active_Flag.Gas)	//가스 해제 상태에서 차단으로 변했을때 
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
				Gu8_Elec_Diff_Flag = 0;		//월패드에서 응답이 없을 때, 전등 제어 데이터 전송 시 콘센트 제어 데이터가 가는 경우가 있어서 추가함
				TxBuf.Result_SEND_CC_Count = 0;	//해당 데이터 재전송 중에 다른 스위치 눌러서 제어했을 때, CC값이 BC로 초기화 되지 않고 BD, BE로 다른 제어 데이터가 전송되어 추가함.
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
				else	//일괄 스위치에서 제어 패킷 받은 후 제어한 뒤 상태가 같아 다음 제어 시 데이터 송신하지 않는 경우 있어서 추가
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
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// ???¡¾???¡¤? ?????¡§
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
				TxBuf.Result_SEND_CC_Count		= 0;	//해당 데이터 재전송 중에 다른 스위치 눌러서 제어했을 때, CC값이 BC로 초기화 되지 않고 BD, BE로 다른 제어 데이터가 전송되어 추가함.
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
				Gu8_Light_n_ETC_Touch_Flag	= 1;	//가스 패킷 3회 전달 후 일괄 패킷 송신을 위해 사용
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
#else	//대기전력, 네트워크 스위치 인 경우
	if(item2tsn(mapping_ITEM_3WAY_1))
	{
		if(Gu8_3Way_Control_Flag)
		{
			if(pTx->buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE && pTx->buf[KOCOM_F_ASL] == Get_485_ID() && pTx->buf[KOCOM_F_CC] == 0xBE)	//출발지(ASH) 전등스위치, 출발지(ASL) 기기 ID, 3번째 재전송 일 때
			{
				if(pTx->buf[KOCOM_F_OPCODE] == KOCOM_OP_3WAY_CONTROL)	//OPCODE는 제어 일 경우
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
	
	Control_Diff_Check();	// 터치에 의한 동작에서만 이벤트 생성해야 함(자주 실행되지 않도록 적용할 것)
	if(TxBuf.Result_SEND_Event_Flag)		//터치로 이벤트 발생했을때
	{
		if(TxBuf.SEND_Flag == 0 && TxBuf.Result_SEND_Flag == 0)		//
		{
			Gu8_RS_485_Tx_Tmr				= KOCOM_RES_DELAY_TIME;		// 약 250ms 이후 결과 전송 KOCOM_RESULT_SEND_TIME에서 변경함. 터치로 이벤트 발생시 250ms 지연 필요없음.
			TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
			if(Block_Event_Flag.Batch)			TxBuf.Result_SEND_OP_Code		= Gu8_BATCH_OPCODE;
			else if(Block_Event_Flag.Gas)		TxBuf.Result_SEND_OP_Code		= KOCOM_OP_GAS_BLOCK;
			else if(Block_Event_Flag.Elevator)	TxBuf.Result_SEND_OP_Code		= KOCOM_OP_ELEVATOR_CALL;
			else if(Block_Event_Flag.Cook)		TxBuf.Result_SEND_OP_Code		= (uint8_t)(KOCOM_OP_COOK_BLOCK | KOCOM_COOK_DEVICE);	//가스와 쿡탑 opcode 구분을 위해서 사용함.
			else if(Block_Event_Flag.Threeway)
			{
				TxBuf.Result_SEND_OP_Code		= KOCOM_OP_3WAY_CONTROL;
				// printf("Threeway\r\n");
			}
#endif
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
			TxBuf.Result_SEND_OP_Code		= KOCOM_OP_LIGHT_CONTROL;		// 0x00 제어
#endif
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
			if(Gu8_Light_Diff_Flag)		TxBuf.Result_SEND_OP_Code		= KOCOM_OP_LIGHT_CONTROL;				//구분하기 위해서 나눠놨지만 OP_CONTROL과 OP_ELEC_CONTROL 값 자체는 동일함.
			else if(Gu8_Elec_Diff_Flag)	TxBuf.Result_SEND_OP_Code		= KOCOM_OP_ELEC_CONTROL;
#endif
    		TxBuf.Result_SEND_CC_Count		= 0;
		}
	}
#if defined _ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_ || defined _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_n_3WAY_	//일괄/가스, 일괄/쿡탑의 경우만
	if(Gu8_RS_485_Tx_Tmr == 0 && Gu8_RS_485_Tx_Add_Tmr == 0 && (Gu8_RS_485_Rx_Tmr < pG_Config->Protocol_IntervalTime))		// 데이터 수신 후 10ms(또는 250ms) 이후 & 데이터 수신 후 1ms 지나면 ACK 전송
#else
	if(Gu8_RS_485_Tx_Tmr == 0 && (Gu8_RS_485_Rx_Tmr < pG_Config->Protocol_IntervalTime))		// 데이터 수신 후 10ms(또는 250ms) 이후 & 데이터 수신 후 1ms 지나면 ACK 전송
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

			if(TxBuf.Result_SEND_Flag == KOCOM_SINGLE_ID)			// 전송할 결과가 있으면
			{
				Gu8_RS_485_Tx_Tmr		= 25;	// 약 250ms 이후 결과 전송 0xbc ack 후 결과, 재전송
				
			}		//재전송 or 0xBC로 요청왔을 경우 결과전송시 약 25ms 가 빨리 통신되는 경우가 있어서 추가함. 기존 설정은 250ms.
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
		else if(TxBuf.Result_SEND_Flag == KOCOM_SINGLE_ID)			// 전송할 결과 데이터가 있으면	//else if - > if
		{
			Gu8_RS_485_Tx_Tmr		= KOCOM_RESULT_SEND_TIME;		//수정 약 250ms 이후 결과 전송, ack 없을때 첫 전송, 근데 재전송에도 영향을 줌. KOCOM_RESULT_SEND_TIME하면 재전송일때 450ms 이상 나옴.
			if(TxBuf.Result_SEND_CC_Count >= 3)		// 3회 || 터치 이벤트 발생 중, 다른 터치 이벤트 발생 시 재전송 3회 이상이 되어도 계속 재전송이 되는 현상있어 추가함.
			{
				Send_After_3_Transmission(pTx);
				TxBuf.SEND_Flag				= 0;
				TxBuf.Result_SEND_Flag		= 0;
				TxBuf.Result_SEND_OP_Code	= 0;
				TxBuf.Result_SEND_CC_Count	= 0;
				Gu8_Light_Diff_Flag 		= 0;	//터치시 생하는 이벤트 플래그 재전송 3회 완료 후 초기화
				Gu8_Elec_Diff_Flag 			= 0;	//터치시 발생하는 이벤트 플래그 재전송 3회 완료 후 초기화
			}
			else									// ????¨?? 3??? ???????????¡×,
			{
				// 데이터 생성 및 다음 프로세스에서 전송
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
				pTx->count	= 0;
				pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_1;
				pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_2;
				pTx->buf[pTx->count++]	= KOCOM_HD;
				if(TxBuf.Result_SEND_OP_Code == KOCOM_OP_BATCH_LIGHT_ON || TxBuf.Result_SEND_OP_Code == KOCOM_OP_BATCH_LIGHT_OFF)
				{				
					pTx->buf[pTx->count++]	= KOCOM_CC_NOT_ACK;																//결과값 전송
					pTx->buf[pTx->count++]	= 0x00;																			// PCNT
					pTx->buf[pTx->count++]	= KOCOM_LIGHT_DEVICE;															// KOCOM_WALLPAD, 		ADH 전등 -> 월패드
					pTx->buf[pTx->count++]	= KOCOM_GROUP_ID;																// KOCOM_WALLPAD_ID,	ADL
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;
					pTx->buf[pTx->count++]	= TxBuf.Result_SEND_OP_Code;													//OP-CODE;	
				}
			//ASL
				else if(TxBuf.Result_SEND_OP_Code == KOCOM_OP_GAS_BLOCK)
				{
					pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);						//결과값 전송
					pTx->buf[pTx->count++]	= 0x00;																			// PCNT
					pTx->buf[pTx->count++]	= KOCOM_GAS_DEVICE;															// KOCOM_WALLPAD, 		ADH 전등 -> 월패드
					pTx->buf[pTx->count++]	= Get_485_ID();																	// KOCOM_WALLPAD_ID,	ADL
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;
					pTx->buf[pTx->count++]	= TxBuf.Result_SEND_OP_Code;													//OP-CODE;	
				}
				else if(TxBuf.Result_SEND_OP_Code == (KOCOM_OP_COOK_BLOCK | KOCOM_COOK_DEVICE))
				{
					pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);						//결과값 전송
					pTx->buf[pTx->count++]	= 0x00;																			// PCNT
					pTx->buf[pTx->count++]	= KOCOM_COOK_DEVICE;															// KOCOM_WALLPAD, 		ADH 전등 -> 월패드
					pTx->buf[pTx->count++]	= Get_485_ID();																	// KOCOM_WALLPAD_ID,	ADL
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;
					pTx->buf[pTx->count++]	= (uint8_t)(TxBuf.Result_SEND_OP_Code ^ KOCOM_COOK_DEVICE);													//OP-CODE;	
				}
				else if(TxBuf.Result_SEND_OP_Code == KOCOM_OP_ELEVATOR_CALL)
				{
					pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);						//결과값 전송
					pTx->buf[pTx->count++]	= 0x00;																			// PCNT
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;															// KOCOM_WALLPAD, 		ADH 전등 -> 월패드
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
						pTx->buf[pTx->count++]	= KOCOM_3WAY_LIGHT_DEVICE;	//전등 스위치 3로 주소
						pTx->buf[pTx->count++]	= 0x00;						//3로 번호
						pTx->buf[pTx->count++]	= KOCOM_3WAY_BATCH_DEVICE;	//일괄 스위치 3로 주소
						pTx->buf[pTx->count++]	= Get_485_ID();				//출발지 ID
						pTx->buf[pTx->count++]	= KOCOM_OP_LIGHT_CONTROL;	//OPCODE 제어
						Gu8_3Way_Control_Flag = 0;
					}
					else
					{
						pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);						//결과값 전송
						pTx->buf[pTx->count++]	= 0x00;																			// PCNT
						pTx->buf[pTx->count++]	= KOCOM_WALLPAD;																// KOCOM_WALLPAD, 		ADH 전등 -> 월패드
						pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;																// KOCOM_WALLPAD_ID,	ADL
						pTx->buf[pTx->count++]	= KOCOM_BATCH_DEVICE;
						pTx->buf[pTx->count++]	= Get_485_ID();
						pTx->buf[pTx->count++]	= TxBuf.Result_SEND_OP_Code;													//OP-CODE;
					}
				}
				else
				{
					pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);					//결과값 전송
					pTx->buf[pTx->count++]	= 0x00;																		// PCNT
					pTx->buf[pTx->count++]	= KOCOM_WALLPAD;															// KOCOM_WALLPAD, 		ADH 전등 -> 월패드
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
					pTx->buf[pTx->count++]	= 0;							//추가 DATA_7
				}

				switch(TxBuf.Result_SEND_OP_Code)		// ??¨???? ¡Æ?¡Æ?¡Æ?® ????????
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
						pTx->buf[pTx->count++]			= 0;								//추가 DATA_7
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
						pTx->buf[pTx->count++]	= KOCOM_ON_FLAG;								//추가 DATA_7
						Block_Event_Flag.Batch			= 0;					
						TxBuf.Result_SEND_Event_Flag	= 0;									// 플래그가 설정된 후 제어에 대한 데이터를 전송하였으면 클리어
						break;
					case KOCOM_OP_BATCH_LIGHT_OFF:
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_0
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_1
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_2
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_3
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_4
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_5
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_6
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//추가 DATA_7
						Block_Event_Flag.Batch			= 0;
						TxBuf.Result_SEND_Event_Flag	= 0;									// 플래그가 설정된 후 제어에 대한 데이터를 전송하였으면 클리어
						break;
					case KOCOM_OP_GAS_BLOCK:
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_0
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_1
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_2
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_3
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_4
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_5
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_6
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//추가 DATA_7
						Block_Event_Flag.Gas			= 0;
						TxBuf.Result_SEND_Event_Flag	= 0;									// 플래그가 설정된 후 제어에 대한 데이터를 전송하였으면 클리어
						TxBuf.SEND_Flag	= 1;	//240419필요한지 체크
						break;
					case KOCOM_OP_COOK_BLOCK | KOCOM_COOK_DEVICE:
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_0
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_1
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_2
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_3
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_4
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_5
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_6
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//추가 DATA_7
						Block_Event_Flag.Cook			= 0;
						TxBuf.Result_SEND_Event_Flag	= 0;									// 플래그가 설정된 후 제어에 대한 데이터를 전송하였으면 클리어
						break;
					case KOCOM_OP_ELEVATOR_CALL:
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_0
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_1
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_2
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_3
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_4
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_5
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//DATA_6
						pTx->buf[pTx->count++]	= KOCOM_OFF_FLAG;								//추가 DATA_7
						Block_Event_Flag.Elevator		= 0;
						TxBuf.Result_SEND_Event_Flag	= 0;			// 플래그가 설정된 후 제어에 대한 데이터를 전송하였으면 클리어	
						break;
					case KOCOM_OP_PROTOCOL_VERSION:						// 0x4A	프로토콜 버젼요청
			    		pTx->buf[KOCOM_F_DATA_0]		= KOCOM_BATCH_PROTOCOL_VERSION_H;	// 버젼 상위
						pTx->buf[KOCOM_F_DATA_1]		= KOCOM_BATCH_PROTOCOL_VERSION_L;	// 버젼 하위
						break;
					case KOCOM_OP_STATE_REQ:
						pTx->buf[KOCOM_F_OPCODE]	= 0x00;
						if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	//일괄점등상태면
						{
							pTx->buf[pTx->count++] = 0x02;
						}
						else if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == 0)	//일괄소등상태면
						{
							pTx->buf[pTx->count++] = 0x01;
						}
						pTx->buf[pTx->count++]	= 0;							//DATA_1
						pTx->buf[pTx->count++]	= 0;							//DATA_2
						pTx->buf[pTx->count++]	= 0;							//DATA_3
						pTx->buf[pTx->count++]	= 0;							//DATA_4
						pTx->buf[pTx->count++]	= 0;							//DATA_5
						pTx->buf[pTx->count++]	= 0;							//DATA_6
						pTx->buf[pTx->count++]	= 0;							//추가 DATA_7
						break;
				}
#endif
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_) || defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
				pTx->count	= 0;
				pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_1;
				pTx->buf[pTx->count++]	= KOCOM_PREAMBLE_2;
				pTx->buf[pTx->count++]	= KOCOM_HD;
				pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_RESULT + pTx->Result_SEND_CC_Count);	//결과값 전송
				pTx->buf[pTx->count++]	= 0x00;							// PCNT
				pTx->buf[pTx->count++]	= KOCOM_WALLPAD;				// KOCOM_WALLPAD, 		ADH 전등 -> 월패드
				pTx->buf[pTx->count++]	= KOCOM_WALLPAD_ID;				// KOCOM_WALLPAD_ID,	ADL
				if((RxBuf.buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)		//수신한 CC 값이 요청일때()
				{
					if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE)		//월패드 -> 전등 스위치 요청
					{
						pTx->buf[pTx->count++] = KOCOM_LIGHT_DEVICE;		//ASH = LIGHT_DEVICE
					}
					else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE)	//월패드 -> 전열 스위치 요청
					{
						pTx->buf[pTx->count++] = KOCOM_ELEC_DEVICE;			//ASH = ELEC_DEVICE
					}
					else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_WALLPAD)		//목적지가 월패드일때, -> CC가 0xBC, ADH = WALLPAD면 스위치의 결과 응답 or 결과 재전송
					{
						pTx->buf[pTx->count++] = RxBuf.buf[KOCOM_F_ASH];		//ASH에 수신한 ASH 값 대입
					}
				}
				else if((RxBuf.buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		//수신한 CC 값이 ACK 일때
				{
					if(RxBuf.buf[KOCOM_F_ASH] == KOCOM_WALLPAD)						//출발지가 월패드일때, -> CC가 0xDC, ASH = WALLPAD면 월패드에서 보내는 결과에 대한 ACK
					{
						pTx->buf[pTx->count++]	= RxBuf.buf[KOCOM_F_ADH];		//ASH 전등 or 전열 재전송일때, 월패드 값이 들어옴...
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
				else															//그룹요청, 그외(?) 일때, 세대기 -> 스위치 or 일괄제어시 일괄 스위치 -> 스위치로 보낼때
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
				pTx->buf[pTx->count++]	= 0;							//추가 DATA_7

				switch(TxBuf.Result_SEND_OP_Code)		// 전송할 결과가 있으면
				{
					default:
						break;
					case KOCOM_OP_LIGHT_CONTROL:								// 0x00 제어
						// printf("control\r\n");
						if(Gu8_Light_Diff_Flag && Gu8_Elec_Diff_Flag == 0)		//스위치에서 직접 전등 제어
						{
							if(G_Trace)	printf("Light Diff\r\n");
							for(item=0;item<MAX_KOCOM_DATA_SEQUENCE;item++)
							{
								pTx->buf[KOCOM_F_DATA_0+item]	= KOCOM_LIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item]);
							}
							if(GET_LED_State(item2tsn(mapping_ITEM_LIGHT_1)) == LED_FLASHING)
							{
								pTx->buf[KOCOM_F_DATA_0] = 0x00;		//LED 점멸은 스위치의 전체 전등 OFF를 했을 경우인데, 이 경우 스위치 상태가 ON이기 때문에 데이터가 FF 00 00 .. 으로 전송됨
							}
																		//그러면 월패드에서는 1번전등은 ON으로 받은 상태이기 때문에 1번 전등이 LED 점멸중이면 00으로 데이터 보냄.
							pTx->buf[KOCOM_F_ASH]		= KOCOM_LIGHT_DEVICE;
							TxBuf.Result_SEND_Event_Flag	= 0;			// 플래그가 설정된 후 제어에 대한 데이터를 전송하였으면 클리어
							TxBuf.SEND_Flag	= 1;
						}
						else if(Gu8_Elec_Diff_Flag && Gu8_Light_Diff_Flag == 0)	//스위치에서 직접 전열 제어
						{
							if(G_Trace)	printf("Elec Diff\r\n");
							for(item = 0; item < MAX_KOCOM_ELEC_DATA_SEQUENCE; item++)
							{
								pTx->buf[KOCOM_F_DATA_0 + item]		= KOCOM_ELEC_State_Data_Conversion(KOCOM_ELEC_ITEM_Sequence[item]);
							}							
							pTx->buf[KOCOM_F_ASH]		= KOCOM_ELEC_DEVICE;
							TxBuf.Result_SEND_Event_Flag	= 0;			// 플래그가 설정된 후 제어에 대한 데이터를 전송하였으면 클리어
							TxBuf.SEND_Flag	= 1;
						}					
						else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE && Gu8_Light_Diff_Flag == 0 && Gu8_Elec_Diff_Flag == 0)		//그룹 제어시 월패드 -> 스위치로의 ACK 일때. 개별 제어시는 월패드 -> 스위치로의 요청
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
						else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE && Gu8_Light_Diff_Flag == 0 && Gu8_Elec_Diff_Flag == 0)		//그룹 제어시 월패드 -> 스위치로의 ACK 일때. 개별 제어시는 월패드 -> 스위치로의 요청
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
						else if((RxBuf.buf[KOCOM_F_ADH] == KOCOM_WALLPAD) && (RxBuf.buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE))	//스위치 -> 월패드로의 결과 전송(재전송) 데이터를 받으면, 그룹 제어에서 다음 스위치 데이터를 위함.
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
						else if((RxBuf.buf[KOCOM_F_ADH] == KOCOM_WALLPAD) && (RxBuf.buf[KOCOM_F_ASH] == KOCOM_ELEC_DEVICE))	//스위치 -> 월패드로의 결과 전송(재전송) 데이터를 받으면, 그룹 제어에서 다음 스위치 데이터를 위함.
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
					case KOCOM_OP_COLOR_TEMP_CONTROL:					// 0x01 색온도 제어
						pTx->buf[KOCOM_F_OPCODE]		= KOCOM_OP_COLOR_TEMP_CONTROL;
						for(item = 0; item < MAX_KOCOM_DATA_SEQUENCE; item++)
						{
							pTx->buf[KOCOM_F_DATA_0+item]	= KOCOM_Dimming_Color_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item], &TxBuf);
						}
						TxBuf.SEND_Flag	= 1;
						break;
					case KOCOM_OP_BATCH_LIGHT_REQ:						// 0x62 일괄제어 후 상태요구
						pTx->buf[KOCOM_F_OPCODE]		= Gu8_Batch_Light_State;
						for(item = 0; item < MAX_KOCOM_DATA_SEQUENCE; item++)
						{
							pTx->buf[KOCOM_F_DATA_0 + item]		= KOCOM_BATCHLIGHT_State_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item]);
						}
						TxBuf.SEND_Flag	= 1;
						break;
					case KOCOM_OP_PROTOCOL_VERSION:						// 0x4A	프로토콜 버젼요청
						pTx->buf[KOCOM_F_OPCODE]		= KOCOM_OP_PROTOCOL_VERSION;
						if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE || RxBuf.buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE)
						{
							pTx->buf[KOCOM_F_DATA_0]		= KOCOM_LIGHT_PROTOCOL_VERSION_H;	// 버젼 상위
							pTx->buf[KOCOM_F_DATA_1]		= KOCOM_LIGHT_PROTOCOL_VERSION_L;	// 버젼 하위
						}
						else if(RxBuf.buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE || RxBuf.buf[KOCOM_F_ASH] == KOCOM_ELEC_DEVICE)
						{
							pTx->buf[KOCOM_F_DATA_0]		= KOCOM_ELEC_PROTOCOL_VERSION_H;	// 버젼 상위
							pTx->buf[KOCOM_F_DATA_1]		= KOCOM_ELEC_PROTOCOL_VERSION_L;	// 버젼 하위							
						}
						TxBuf.SEND_Flag	= 1;
						break;
					case KOCOM_OP_DIM_LEVEL:							// 0x5A	디밍 최대 단계조회
					case KOCOM_OP_COLOR_TEMP_LEVEL:						// 0x5B	색온도 최대 단계조회, 회로 존재X : 0x00, 단순 ON/OFF : 0x01, 색온도 최대 단계 값 : 0x02 ~ 0xFF
					case KOCOM_OP_COLOR_TEMP_REQ:						// 0x3B 색온도 상태조회, OFF : 0x00, ON : 0x01 ~ 0xFF
						if(pTx->buf[KOCOM_F_OPCODE] == KOCOM_OP_COLOR_TEMP_REQ)
						{
							pTx->buf[KOCOM_F_OPCODE]		= 0x01;			//색온도 상태조회는 결과 응답시 OPCODe 0x01로 응답.
						}
						for(item = 0; item < MAX_KOCOM_DATA_SEQUENCE; item++)
						{
							pTx->buf[KOCOM_F_DATA_0 + item]	= KOCOM_Dimming_Color_Data_Conversion(KOCOM_LIGHT_ITEM_Sequence[item], &TxBuf);
						}
						TxBuf.SEND_Flag	= 1;
						break;
					case KOCOM_OP_STATE_REQ:								// 0x3A 상태 조회, 회로 OFF : 0x00, 회로 ON : 0xFF, 디밍 회로 : 0x01 ~ 0xFE
						// printf("STATE REQ\r\n");
						pTx->buf[KOCOM_F_OPCODE]		= 0x00;				// 상태 조회 응답시 opcode 0x00
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
uint8_t NIS_Crc(KOCOM_BUF *pTRx, uint8_t cal, uint8_t sel)		//생산시 485통신 테스트를 위해서 추가함.
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
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//프로토콜 타입
	pTx->buf[pTx->count++]	= Get_485_ID();					//ID 번호
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
	pTx->buf[pTx->count++]	= Get_485_Elec_ID();
#else
	pTx->buf[pTx->count++]	= 0;
#endif
	

	pTx->buf[pTx->count++]	= Elec_Watt_MSB;				//전열1, 2의 전력 합(정수)
	pTx->buf[pTx->count++]	= Elec_Watt_LSB;				//전열1, 2의 전력 합(소수)

	pTx->buf[pTx->count++]	= Gu8_ZeroCrossing_Err_Flag;	//제로크로싱 동작 상태 1이면 Err, 0이면 Pass

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

void SET_DimmingLevel(uint8_t item, uint8_t Dimming_Level)		// 디밍레벨
{
	switch(item)
	{
		case mapping_ITEM_DIMMING_LIGHT_1:
			if((uint8_t)Dimming_Level > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
			}
			else
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)Dimming_Level;
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if((uint8_t)Dimming_Level > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
			}
			else
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)Dimming_Level;
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
			break;
	}
}

void SET_DimmingColorLevel(uint8_t item, uint8_t Dimming_Level)		// 색온도
{
	switch(item)
	{
		case mapping_ITEM_DIMMING_LIGHT_1:
			if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)		// 색온도 사용 모델인 경우
			{
				if((uint8_t)Dimming_Level > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
				{
					pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// 최대 크기로 설정
				}
				else
				{
					pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)Dimming_Level;
				}
				/*if(GET_Switch_State(item2tsn(item)) == 0)		//색온도 레벨 제어할때 전등 꺼져있으면 ON
				{
					EventCtrl(item2tsn(item), ON);
				}*/
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)		// 색온도 사용 모델인 경우
			{
				if((uint8_t)Dimming_Level > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
				{
					pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// 최대 크기로 설정
				}
				else
				{
					pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)Dimming_Level;
				}
				/*if(GET_Switch_State(item2tsn(item)) == 0)		//색온도 레벨 제어할때 전등 꺼져있으면 ON
				{
					EventCtrl(item2tsn(item), ON);
				}*/
			}
			break;
	}
}

uint8_t KOCOM_Batch_Light_State(uint8_t item)		//일괄 소등 전 현재 전등 상태 저장.
{
	uint8_t ret = 0;
	switch(item)
	{
		default:
			ret = KOCOM_OFF_FLAG;
			break;		
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
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
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(control_value == KOCOM_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);						//전등 OFF
			}
			else if(control_value == KOCOM_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);						//전등 ON
			}
			break;
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// 복구
			break;
	}
}

void KOCOM_Control(uint8_t item, uint8_t control_value)
{
	uint8_t	Flag = OFF;
	uint8_t tmr, touch_switch = 0;

	tmr	= Gu8_PowerSaving_Tmr;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	
	switch(item)	// 설정된 조명이 있으면
	{
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			touch_switch = item2tsn(item);
			if(control_value == KOCOM_ON_FLAG)
			{
				if(GET_Switch_State(touch_switch) == 0)		//스위치 OFF면
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
				if(GET_Switch_State(touch_switch))			//스위치 ON이면
				{
					EventCtrl(item2tsn(item), OFF);
					if(item == mapping_ITEM_3WAY_1)
					{
						Gu8_Wallpad_3Way_Control_Flag = 1;
					}
				}
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
			touch_switch = item2tsn(item);
			if(control_value == KOCOM_ON_FLAG)	// OFF는 색온도&레벨 설정없이 소등
			{
				if(GET_Switch_State(touch_switch) == 0)		//스위치 OFF면
				{
					EventCtrl(item2tsn(item), ON);
				}
			}
			else if(control_value == KOCOM_OFF_FLAG)	// 저장된 값으로 ON
			{
				if(GET_Switch_State(touch_switch))			//스위치 ON이면
				{
					EventCtrl(item2tsn(item), OFF);
				}
			}
			else if(control_value)					// 나머지는 디밍레벨
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	// 켜져있는 상태에서만 레벨을 설정할 수 있음(차 후 코콤에서 기능이 변경되면 수정할 것)
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;		// LCD에 즉시 보여주기 위해
					Gu8_LCD_DIM_Tmr					= 20;		// 2s 동안 LCD 표시
					SET_DimmingLevel(item, control_value);		//켜져있는 상태면 디밍 레벨만 변경.
				}
				else if(GET_Switch_State(item2tsn((uint8_t)item)) == 0)	//꺼져있는 상태면
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;		// LCD에 즉시 보여주기 위해
					Gu8_LCD_DIM_Tmr					= 20;		// 2s 동안 LCD 표시
					SET_DimmingLevel(item, control_value);
					EventCtrl(item2tsn(item), ON);				//꺼져있는 상태면 디밍 레벨 변경 후 점등
				}
			}
			break;
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// 복구
			break;
	}
}

void KOCOM_ELEC_CONTROL(uint8_t item, uint8_t control_value)
{
	uint8_t tmr, touch_switch, Flag;

	tmr	= Gu8_PowerSaving_Tmr;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	
	switch(item)	// 설정된 조명이 있으면
	{
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_ELECTRICITY_1:		// 디밍1 스위치(PWM 제어)
		case mapping_ITEM_ELECTRICITY_2:		// 디밍2 스위치(PWM 제어)
			touch_switch = item2tsn(item);
			if(control_value == KOCOM_OFF_FLAG)
			{
				Flag = OFF;
				if(GET_Switch_State(touch_switch))	//스위치 ON이면
				{
					// EventCtrl(item2tsn(item), OFF);		//월패드에서 제어시 지연 없게 하기위해서 EventCtrl 사용하지않음.
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
				if(GET_Switch_State(touch_switch) == 0)	//스위치 OFF이면
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
#else		//전열 사용하지 않는 모델은 아무 동작 없음.
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			break;
#endif			
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// 복구
			break;
	}
}

uint8_t KOCOM_BATCHLIGHT_State_Data_Conversion(uint8_t item)		//일괄 상태는 전등 ON/OFF로만 구분
{
	uint8_t ret;

	switch(item)
	{
		default:
			ret = KOCOM_OFF_FLAG;
			break;
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
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
			
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))		//현재 스위치 상태를 리턴
			{
				if(GET_LED_State(item2tsn(mapping_ITEM_LIGHT_1)) == LED_FLASHING)	//전체 전등OFF 일 때 전등1이 지연 소등 시 상태 데이터 OFF로
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
			
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
			if(GET_Switch_State(item2tsn((uint8_t)item)))		//현재 디밍 스위치 켜져있으면 디밍 레벨을 리턴
			{
				ret = (uint8_t)(pG_State->Dimming_Level.Dimming1);
			}
			else												//현재 디밍 스위치 꺼져있으면 0x00 리턴
			{
				ret = KOCOM_OFF_FLAG;
			}
			break;
			
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
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
		case KOCOM_OP_COLOR_TEMP_CONTROL:		//0x00 : 회로 OFF, 0x01 ~ 0xFF : 색온도 단계
		case KOCOM_OP_COLOR_TEMP_REQ:			//0x00 : 회로 OFF, 0x01 ~ 0xFF : 색온도 단계
			switch(item)
			{
				default:
					ret = KOCOM_OFF_FLAG;		//일반전등 ON/OFF 일때 0x00로 표현???
					break;
					
				case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
					if(GET_Switch_State(item2tsn((uint8_t)item)))
					{
						if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)						//색온도 사용 모델이면(디밍만 사용하는 모델에서 조회되는걸 방지)
						{
							ret = (uint8_t)(pG_State->Color_Temp_Level.Color_Temp1);	//현재 색온도 레벨을 리턴
						}
						else															//색온도 사용 모델이 아니면
						{
							ret = KOCOM_OFF_FLAG;										//디밍 스위치는 ON이고, 색온도는 사용하지 않을때, 즉, 디밍만 사용하는 전등일때 어떻게 표현??? 
						}
					}
					else
					{
						ret = KOCOM_OFF_FLAG;											//일반전등은 OFF???
					}
					break;
					
				case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
					if(GET_Switch_State(item2tsn((uint8_t)item)))
					{
						if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)
						{
							ret = (uint8_t)(pG_State->Color_Temp_Level.Color_Temp2);		//현재 색온도 레벨을 리턴
						}
						else															//색온도 사용 모델이 아니면
						{
							ret = KOCOM_OFF_FLAG;										//디밍 스위치는 ON이고, 색온도는 사용하지 않을때, 즉, 디밍만 사용하는 전등일때 어떻게 표현??? 
						}
					}
					else
					{
						ret = KOCOM_OFF_FLAG;											//일반전등은 OFF???
					}
					break;
			}		
			break;
		case KOCOM_OP_COLOR_TEMP_LEVEL:			//0x00 : 회로 존재X, 0x01 : 회로가 단순 ON/OFF, 0x02 ~ 0xFF : 색온도 기능이 있는경우, 색온도 최대 단계 값
		case KOCOM_OP_DIM_LEVEL:				//0x00 : 회로 존재X, 0x01 : 회로가 단순 ON/OFF, 0x02 ~ 0xFE : 디밍 기능이 있는 경우 디밍 최대 단계 값		
			switch(item)
			{
				default:
					ret = KOCOM_NO_CIRCUIT;		//해당 회로가 존재하지 않을때
					break;
				case mapping_ITEM_LIGHT_1:
				case mapping_ITEM_LIGHT_2:
				case mapping_ITEM_LIGHT_3:
				case mapping_ITEM_LIGHT_4:
				case mapping_ITEM_LIGHT_5:
				case mapping_ITEM_LIGHT_6:
				case mapping_ITEM_3WAY_1:
				case mapping_ITEM_3WAY_2:
					ret = KOCOM_LIGHT_CIRCUIT;		//해당 회로가 단순 ON / OFF 일때
					break;
				case mapping_ITEM_DIMMING_LIGHT_1:
				case mapping_ITEM_DIMMING_LIGHT_2:
					if(TxBuf->Result_SEND_OP_Code == KOCOM_OP_COLOR_TEMP_LEVEL)
					{
						if(pG_Config->Enable_Flag.PWM_Color_Temp)		//해당 회로가 색온도 기능이 있는 경우.
						{
							ret = pG_Config->Color_Temp_MAX_Level;		//색온도 최대 단계 값
						}
					}
					else if(TxBuf->Result_SEND_OP_Code == KOCOM_OP_DIM_LEVEL)
					{
						if(pG_Config->Enable_Flag.PWM_Dimming)
						{
							ret = pG_Config->Dimming_MAX_Level;
						}
					}
					else											//해당 회로가 색온도 기능이 없는 경우. 즉, 디밍 전등인 경우.
					{
						ret = KOCOM_LIGHT_CIRCUIT;					//색온도 없이 디밍 전등만 사용할 때는 단순 ON/OFF로 전달. 프로토콜 문서에 색온도 요청일때 디밍전등에 대한 내용 없음.
					}
					break;
			}
			break;
	}
	
	return	ret;
}

uint8_t KOCOM_ELEC_State_Data_Conversion(uint8_t item)		//전열 프로토콜 받으면 수정 필요
{
	uint8_t	ret = 0;
	
	switch(item)
	{
		default:
			ret = KOCOM_OFF_FLAG;
			break;
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_ELECTRICITY_1:
			if(GET_Switch_State(item2tsn(item)) && GET_LED_State(item2tsn(item)) == LED_ON)			//전열 ON 상태
			{
				// printf("ELEC1 SWITCH ON, LED OFF\r\n");
				ret		= KOCOM_ON_FLAG;				
			}
			else if(GET_Switch_State(item2tsn(item)) && GET_LED_State(item2tsn(item)) == LED_FLASHING)		//전열 OFF로 동작중인 상태
			{
				// printf("ELEC1 SWITCH ON, LED FLASHING\r\n");
				ret		= KOCOM_OFF_FLAG;
			}
			else if(GET_Switch_State(item2tsn(item)) == 0 && GET_LED_State(item2tsn(item)) == LED_OFF)		//전열 OFF 상태
			{
				// printf("ELEC1 SWITCH OFF, LED ON\r\n");
				ret		= KOCOM_OFF_FLAG;				
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn(item)) && GET_LED_State(item2tsn(item)) == LED_ON)			//전열 ON인 상태
			{
				// printf("ELEC2 SWITCH ON, LED OFF\r\n");
				ret		= KOCOM_ON_FLAG;				
			}
			else if(GET_Switch_State(item2tsn(item)) && GET_LED_State(item2tsn(item)) == LED_FLASHING)		//전열 OFF로 동작중인 상태
			{
				// printf("ELEC2 SWITCH ON, LED FLASHING\r\n");
				ret		= KOCOM_OFF_FLAG;
			}
			else if(GET_Switch_State(item2tsn(item)) == 0 && GET_LED_State(item2tsn(item)) == LED_OFF)		//전열 OFF 상태
			{
				// printf("ELEC2 SWITCH OFF, LED ON\r\n");
				ret		= KOCOM_OFF_FLAG;				
			}
			break;
#else		//전열 사용하지 않는 모델은 OFF Flag 리턴.
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			ret		=	KOCOM_OFF_FLAG;
			break;
#endif			
	}
	return	ret;
}
//------------------------------------------------------------------------------------------------------------------------------------------
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_) || defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)	// 2¡Æ?ø???	- ????+????¡??????¡§
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
	pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_ACK | (pRx->buf[KOCOM_F_CC] & 0x0F));		// 수신된 CC값에 따라 응답 CC값 달라짐
	pTx->buf[pTx->count++]	= 0x00;															// PCNT
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASH];										// KOCOM_WALLPAD
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASL];										// KOCOM_WALLPAD_ID
	pTx->buf[pTx->count++]	= KOCOM_ELEC_DEVICE;
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_OPCODE];										// 수신된 OP-CODE으로 전송
	for(i=KOCOM_F_DATA_0;i<=KOCOM_F_DATA_7;i++)
	{
		pTx->buf[pTx->count++]	= pRx->buf[i];												// 수신된 데이터로 전송
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
	pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_ACK | (pRx->buf[KOCOM_F_CC] & 0x0F));		// 수신된 CC값에 따라 응답 CC값 달라짐
	pTx->buf[pTx->count++]	= 0x00;															// PCNT
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASH];										// KOCOM_WALLPAD
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASL];										// KOCOM_WALLPAD_ID
	pTx->buf[pTx->count++]	= KOCOM_LIGHT_DEVICE;
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_OPCODE];										// 수신된 OP-CODE으로 전송
	for(i=KOCOM_F_DATA_0;i<=KOCOM_F_DATA_7;i++)
	{
		pTx->buf[pTx->count++]	= pRx->buf[i];												// 수신된 데이터로 전송
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
	if((pRx->buf[KOCOM_F_PRE_1] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[KOCOM_F_PRE_2] != NIS_LIGHT_ID_COMM_2))		//485통신 테스트를 위한 데이터가 아닐때
	{
		if((pRx->buf[KOCOM_F_ADH] != KOCOM_LIGHT_DEVICE && pRx->buf[KOCOM_F_ADH] != KOCOM_ELEC_DEVICE && pRx->buf[KOCOM_F_ADH] != KOCOM_3WAY_LIGHT_DEVICE) || (pRx->buf[KOCOM_F_ADL] != Get_485_ID() && pRx->buf[KOCOM_F_ADL] != KOCOM_GROUP_ID))
		{
			if(TxBuf.Result_SEND_Flag == KOCOM_GROUP_ID)		// 그룹전송이 있으면
			{
				if(Get_485_ID() > KOCOM_LOWEST_ID)				// 제일 낮은 ID보다 크고
				{
					if((pRx->buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE || pRx->buf[KOCOM_F_ADH] == KOCOM_ELEC_DEVICE) && pRx->buf[KOCOM_F_ADL] == (Get_485_ID() - 1))	// 월패드에서 바로 전 장치기 ID로 ACK가 있으면
					{
						if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)	// 결과에 대한 응답이면	(0xDC, 0xDD, 0xDE)
						{
							TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
							TxBuf.Result_SEND_CC_Count		= 0;
						}
					}
					else if(pRx->buf[KOCOM_F_ADH] == KOCOM_WALLPAD && pRx->buf[KOCOM_F_ADL] == KOCOM_WALLPAD_ID)		// 목적지가 월패드 ID일 경우 즉, 다른 스위치에서 월패드로 데이터 송신 할 경우.
					{
						// printf("SWITCH -> WALLPAD\r\n");
						if((pRx->buf[KOCOM_F_ASH] == KOCOM_LIGHT_DEVICE || pRx->buf[KOCOM_F_ASH] == KOCOM_ELEC_DEVICE) && pRx->buf[KOCOM_F_ASL] == (Get_485_ID() - 1))	// 바로 전 장치기가
						{
							// printf("LIGHT -> WALLPAD, ID = %d\r\n", (uint16_t)Get_485_ID());
							if((pRx->buf[KOCOM_F_CC]) == 0xBE)																// 3회 전송했으면	(0xBC, 0xBD, 0xBE)
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
			//230303 일괄소등 관련 패킷 무시하려면 주석 처리 해야함.
			//일괄 소등 시 네트워크 스위치 전원이 꺼지는 현장. 
			//패킷으로 일괄 소등 후 전원이 꺼지게 되어 전등 OFF 상태로 정전 보상이 되어 일괄 소등/해제 시 상태 복구 효과?를 볼수 없음.
			//임의로 일괄 소등 관련 패킷은 사용하지 않고 전등 전원 On/Off로 정전 보상으로 일괄 소등/해제 했을 때 상태 복구 효과를 볼 수 있음.
			case KOCOM_OP_BATCH_LIGHT_OFF:						// 0x65 일괄소등 명령(응답없음)
				if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)
				{
					if(Gu8_Batch_Light_State != KOCOM_BATCH_OFF_STATE)
					{
						for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
						{
							Store_Light_State[i] = KOCOM_Batch_Light_State(KOCOM_LIGHT_ITEM_Sequence[i]);								//현재 상태 저장
							// KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);	// 항목, 제어
							KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], KOCOM_OFF_FLAG);	// 항목, 제어
						}
					}
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
					Gu8_Batch_Light_State = KOCOM_BATCH_OFF_STATE;					
				}
				break;
			case KOCOM_OP_BATCH_LIGHT_ON:						// 0x66 일괄점등 명령(응답없음)
				if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)
				{
					for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
					{
						if(Gu8_Batch_Light_State == KOCOM_BATCH_ON_STATE)		//스위치 처음 상태 or 현재 일괄점등 상태일 때, 점등 명령이 오면 모든 전등 다 ON. 일괄 점등 상태에서 일괄 점등 명령이 오면 모두 점등됨.
						{
							// KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);	// 항목, 제어
							// KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], KOCOM_ON_FLAG);	// 항목, 제어
						}
						else if(Gu8_Batch_Light_State != KOCOM_BATCH_ON_STATE)											//일괄소등 상태일 때는 소등 전 상태로 복귀.
						{	
							// if(Store_Light_State[i] == (pRx->buf[KOCOM_F_DATA_0 + i]))		//일괄소등 전 저장된 전등 상태가 ON이고 DATA 요청도 전등ON이면, 소등 전 상태로 복귀
							// {
							// 	KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);	// 항목, 제어
							// }
							if(Gu8_Batch_Light_State == KOCOM_BATCH_OFF_STATE)
							{
								if(Store_Light_State[i] == KOCOM_ON_FLAG)						//일괄 소등 전 전등 상태가 ON인 전등만 복구. 일괄소등 상태에서 개별적으로 전등 ON 제어한 전등은 상태 유지함.
								{
									KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], KOCOM_ON_FLAG);
								}
							}
							else		//리셋 or 처음 시작 시 일괄 상태 0 으로 초기화 된 상태에서는 일괄 소등 해제 데이터 받았으면 모든 전등 ON
							{
								KOCOM_BatchLight_Control(KOCOM_LIGHT_ITEM_Sequence[i], KOCOM_ON_FLAG);
							}
							// else	//소등 전 저장된 전등 상태가 OFF 일때
							// {
							// 	cnt++;
							// }
						}
					}
					// if(cnt >= MAX_KOCOM_DATA_SEQUENCE)		//일괄 소등 전 저장된 전등의 상태가 모두 전등 OFF면, 일괄 점등시 모든 전등 점등.
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
			case KOCOM_OP_LIGHT_CONTROL:						// 0x00 제어
			case KOCOM_OP_COLOR_TEMP_CONTROL:					// 0x01 색온도 제어
			case KOCOM_OP_BATCH_LIGHT_REQ:						// 0x62 일괄제어 후 상태요구
			case KOCOM_OP_BATCH_LIGHT_STATE_OFF:
			case KOCOM_OP_BATCH_LIGHT_STATE_ON:
			case KOCOM_OP_PROTOCOL_VERSION:						// 0x4A	프로토콜 버젼요청
			case KOCOM_OP_DIM_LEVEL:							// 0x5A	디밍 단계조회
			case KOCOM_OP_COLOR_TEMP_LEVEL:						// 0x5B	색온도 단계조회
			case KOCOM_OP_STATE_REQ:							// 0x3A 전체상태 조회
			case KOCOM_OP_COLOR_TEMP_REQ:						// 0x3B 색온도 상태조회
				if(pRx->buf[KOCOM_F_OPCODE] == KOCOM_OP_LIGHT_CONTROL)					// 0x00 제어일 경우
				{
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) != KOCOM_CC_ACK)			// ACK 응답을 받으면 재동작이 되는 현상이 있어서 요청 데이터가 왔을때만 동작하도록 추가함.
					{
						for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
						{
							KOCOM_Control(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);	// 항목, 제어
						}
					}
				}
				else if(pRx->buf[KOCOM_F_OPCODE] == KOCOM_OP_COLOR_TEMP_CONTROL)	// 0x01 색온도 제어일 경우
				{
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) != KOCOM_CC_ACK)
					{
						for(i = 0; i < MAX_KOCOM_DATA_SEQUENCE; i++)
						{
							SET_DimmingColorLevel(KOCOM_LIGHT_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);
						}
					}
				}
				
				if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// 데이터 요청이면	(0xBC, 0xBD, 0xBE)
				{
					KOCOM_LIGHT_Model_ACK(pRx);		// OP-CODE	0x4A로 응답???? 전등스위치 ACK 응답
					TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
					TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];		// 결과데이터 OP-CODE
					TxBuf.Result_SEND_CC_Count		= 0;
				}
				else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)	// 결과에 대한 응답이면	(0xDC, 0xDD, 0xDE)
				{
					if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())
					{
						TxBuf.SEND_Flag				= 0;
					}				
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_OP_Code		= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
					Gu8_Light_Diff_Flag 			= 0;	//터치시 발생하는 이벤트 플래그 응답오면 0으로 초기화
#ifdef COMM_THREEWAY
					if(pRx->buf[KOCOM_F_ADH] == KOCOM_LIGHT_DEVICE)	//ADH 전등스위치, ACK 응답 일 때 
					{
						if(item2tsn(mapping_ITEM_3WAY_1))
						{
							if(Gu8_3Way_Send_Flag)
							{
								if(Gu8_3Way_B2L_Control_Flag == 0)	//일괄 스위치 통해서 제어가 아닐 경우만
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
										//월패드에서 3로 전등 제어 시 일괄 스위치에 3로 패킷을 전달 하기 위해
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
				else if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)				// ACK 없음
				{
					if(pRx->buf[KOCOM_F_ADL] == KOCOM_GROUP_ID)			// 그룹 ID
					{
						if(Get_485_ID() == KOCOM_LOWEST_ID)							// 첫번째로 전송해야 할 ID이면
						{
							Gu8_RS_485_Tx_Tmr				= 0;
							TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];
							TxBuf.Result_SEND_CC_Count		= 0;
						}
						else														// 나중에 전송해야 할 ID이면
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
				TxBuf.Result_SEND_Event_Flag = 1;	//일괄->전등스위치로 3로 제어 데이터 받은 뒤 전등 패킷 월패드로 전달하기 위해서 추가함
				Gu8_Light_Diff_Flag = 1;	//일괄->전등스위치로 3로 제어 데이터 받은 뒤 전등 패킷 월패드로 전달하기 위해서 추가함
				Gu8_3Way_B2L_Control_Flag = 1;	//일괄 스위치 통해서 제어인 경우만
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
						else if(Gu8_Batch_Elec_State != KOCOM_BATCH_ON_STATE)	//현재 일괄소등 상태면 소등 전 상태로 복귀
						{
							// if(Store_Elec_State[i] == (pRx->buf[KOCOM_F_DATA_0 + i]))
							if(Gu8_Batch_Elec_State == KOCOM_BATCH_OFF_STATE)
							{
								if(Store_Elec_State[i] == KOCOM_ON_FLAG)
								{
									KOCOM_ELEC_CONTROL(KOCOM_ELEC_ITEM_Sequence[i], KOCOM_ON_FLAG);
								}
							}
							else		//리셋 or 처음 시작 시 일괄 상태 0 으로 초기화 된 상태에서는 일괄 소등 해제 데이터 받았으면 모든 전열 ON
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
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) != KOCOM_CC_ACK)			// ACK 응답을 받으면 재동작이 되는 현상이 있어서 요청 데이터가 왔을때만 동작하도록 추가함.
					{
						for(i = 0; i < MAX_KOCOM_ELEC_DATA_SEQUENCE; i++)
						{
							// touch_switch = item2tsn(KOCOM_ELEC_ITEM_Sequence[i]);
							KOCOM_ELEC_CONTROL(KOCOM_ELEC_ITEM_Sequence[i], pRx->buf[KOCOM_F_DATA_0 + i]);
						}
					}
				}
				if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// 데이터 요청이면	(0xBC, 0xBD, 0xBE)
				{
					KOCOM_ELEC_Model_ACK(pRx);		// OP-CODE	0x4A로 응답???? 전등스위치 ACK 응답
					TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
					TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];		// 결과데이터 OP-CODE
					TxBuf.Result_SEND_CC_Count		= 0;
				}
				else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		// 결과에 대한 응답이면	(0xDC, 0xDD, 0xDE)
				{
					if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())
					{
						TxBuf.SEND_Flag				= 0;
					}				
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_OP_Code		= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
					Gu8_Elec_Diff_Flag 				= 0;		//터치시 발생하는 이벤트 플래그 응답오면 0으로 초기화	
				}
				else if(pRx->buf[KOCOM_F_CC] == KOCOM_CC_NOT_ACK)				// ACK 없음
				{
					if(pRx->buf[KOCOM_F_ADL] == KOCOM_GROUP_ID)					// 그룹 ID
					{
						if(Get_485_ID() == KOCOM_LOWEST_ELEC_ID)				// 첫번째로 전송해야 할 ID이면
						{
							// Gu8_RS_485_Tx_Tmr				= pG_Config->Protocol_RES_DelayTime;
							Gu8_RS_485_Tx_Tmr				= 0;
							TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];
							TxBuf.Result_SEND_CC_Count		= 0;
						}
						else														// 나중에 전송해야 할 ID이면
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
	pTx->buf[pTx->count++]	= (uint8_t)(KOCOM_CC_ACK | (pRx->buf[KOCOM_F_CC] & 0x0F));		// 수신된 CC값에 따라 응답 CC값 달라짐
	pTx->buf[pTx->count++]	= 0x00;					// PCNT
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASH];		// KOCOM_WALLPAD
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ASL];		// KOCOM_WALLPAD_ID
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_ADH];
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= pRx->buf[KOCOM_F_OPCODE];							// 수신된 OP-CODE으로 전송
	for(i=KOCOM_F_DATA_0;i<=KOCOM_F_DATA_7;i++)
	{
		pTx->buf[pTx->count++]	= pRx->buf[i];										// 수신된 데이터로 전송
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
			case KOCOM_OP_BATCH_LIGHT_OFF:						// 0x65 일괄소등 명령(응답없음)
				BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
				TxBuf.Result_SEND_Flag			= 0;
				TxBuf.Result_SEND_CC_Count		= 0;
				Gu8_Batch_Light_State = KOCOM_BATCH_OFF_STATE;			
				break;
			case KOCOM_OP_BATCH_LIGHT_ON:						// 0x66 일괄점등 명령(응답없음)
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
						BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//월패드에서 가스차단 오면 즉시 차단 상태가 되고, 스위치에서 차단 요청중이고, 차단 데이터오면 차단.
						TxBuf.SEND_Flag					= 0;
						TxBuf.Result_SEND_Flag			= 0;			//차단 제어하고 응답하지 않음. 응답은 세대기에서 함.
						TxBuf.Result_SEND_CC_Count		= 0;		
					}
					else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		//211117 프로토콜 NG로 추가함. (스위치에서 가스 차단 제어 시 월패드에서 응답와도 재전송이 생겨서)
					{
						if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())						//응답은 보내지 않음.
						{
							TxBuf.SEND_Flag				= 0;
						}	
						TxBuf.Result_SEND_Flag			= 0;
						TxBuf.Result_SEND_OP_Code		= 0;
						TxBuf.Result_SEND_CC_Count		= 0;

						if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
						{
							if(pRx->buf[KOCOM_F_ADH] == KOCOM_GAS_DEVICE)	//AA 55 30 DC 00 2C 00 01 00...(월패드->가스차단기)
							{
								if(Gu8_Direct_Control)	//직접 제어에 대한 응답에만 일괄 소등 데이터 전달
								{
									Block_Active_Flag.Batch = 1;
									Gu8_Light_n_ETC_Touch_Flag = 1;
									//가스 데이터 시나리오 종료 후 일괄 소등 데이터 전송.
									Gu8_Direct_Control = 0;
									Gu8_RS_485_Tx_Add_Tmr = 50;
								}
							}			
						}
						else if(item2tsn(mapping_ITEM_GAS_n_COOK))
						{
							if(pRx->buf[KOCOM_F_ADH] == KOCOM_GAS_DEVICE)	//가스 데이터 전송 후 쿡탑 데이터 전송함.
							{
								if(Gu8_Direct_Control)
								{
									Block_Active_Flag.Cook = 1;
									Gu8_Light_n_ETC_Touch_Flag = 1;
									//쿡탑 데이터 추가
								}
							}
						} //가스/쿡탑은 가스 응답을 받았을 시 쿡탑은 사용하지 않으므로 가스 응답 데이터 받으면 전송하지 않도록 함.
					}
					break;
				case KOCOM_OP_GAS_BLOCK_RELEASE:
					if(pRx->buf[KOCOM_F_ADH] == KOCOM_WALLPAD && pRx->buf[KOCOM_F_ASH] == KOCOM_GAS_DEVICE)		//가스 차단해제| 가스차단기 -> 월패드(스위치도 월패드 주소 사용)
					{
						if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
						{
							if(item2tsn(mapping_ITEM_GAS_n_COOK))
							{
								Gu8_Gas_State = OPEN;
							}
							BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);	//월패드에서 가스차단해제 오면 즉시 차단해제 상태가 되고, 스위치에서는 가스 차단해제 불가능.
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
				case KOCOM_OP_COOK_BLOCK:	//OP 가스랑 동일
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
					{
						Gu8_Cook_State = CLOSE;
						BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//월패드에서 가스차단 오면 즉시 차단 상태가 되고, 스위치에서 차단 요청중이고, 차단 데이터오면 차단.
						TxBuf.SEND_Flag				= 0;
						TxBuf.Result_SEND_Flag			= 0;			//차단 제어하고 응답하지 않음. 응답은 세대기에서 함.
						TxBuf.Result_SEND_CC_Count		= 0;		
					}
					else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		//211117 프로토콜 NG로 추가함. (스위치에서 가스 차단 제어 시 월패드에서 응답와도 재전송이 생겨서)
					{
						if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())						//응답은 보내지 않음.
						{
							TxBuf.SEND_Flag				= 0;
						}	
						TxBuf.Result_SEND_Flag			= 0;
						TxBuf.Result_SEND_OP_Code		= 0;
						TxBuf.Result_SEND_CC_Count		= 0;

						if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
						{
							if(pRx->buf[KOCOM_F_ADH] == KOCOM_COOK_DEVICE)	//AA 55 30 DC 00 2C 00 01 00...(월패드->가스차단기)
							{
								if(Gu8_Direct_Control)	//직접 제어에 대한 응답에만 일괄 소등 데이터 전달
								{
									Block_Active_Flag.Batch = 1;
									Gu8_Light_n_ETC_Touch_Flag = 1;
									Gu8_Direct_Control = 0;
									Gu8_RS_485_Tx_Add_Tmr = 50;
									//쿡탑 데이터 시나리오 종료 후 일괄 소등 데이터 전송.
								}
							}			
						}
					}
					break;
				case KOCOM_OP_COOK_BLOCK_RELEASE:	//OP 가스랑 동일
					if(pRx->buf[KOCOM_F_ADH] == KOCOM_WALLPAD && pRx->buf[KOCOM_F_ASH] == KOCOM_COOK_DEVICE)		//가스 차단해제| 가스차단기 -> 월패드(스위치도 월패드 주소 사용)
					{
						if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
						{
							if(item2tsn(mapping_ITEM_GAS_n_COOK))
							{
								Gu8_Cook_State = OPEN;
							}
							BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);	//월패드에서 가스차단해제 오면 즉시 차단해제 상태가 되고, 스위치에서는 가스 차단해제 불가능.
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
					if(pRx->buf[KOCOM_F_ADH] == KOCOM_ELEVATOR_DEVICE && pRx->buf[KOCOM_F_ASH] == KOCOM_WALLPAD)	//엘리베이터 호출| 세대기 -> 스위치
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
							/*else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)	// 결과에 대한 응답이면	(0xDC, 0xDD, 0xDE)
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
						else if(pRx->buf[KOCOM_F_DATA_0] == KOCOM_ELEVATOR_ARRIVE)		//도착
						{
							if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)
							{
								BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);
							}
						}
					// pRx->buf[KOCOM_F_DATA_0] 0x00 : 정지, 0x01 : 하향, 0x02 :  상향, 0x03 : 도착
					// pRx->buf[KOCOM_F_DATA_1]	0x00 ~ 0xFF : 층수(최상위 비트 0 : 지상, 1: 지하), 전자식 스위치에서는 필요없음.
						if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// 데이터 요청이면	(0xBC, 0xBD, 0xBE)
						{
							KOCOM_BATCH_BLOCK_Model_ACK(pRx);		// OP-CODE	0x4A로 응답???? 전등스위치 ACK 응답
							TxBuf.Result_SEND_Flag			= 0;							// 엘리베이터쪽은 ACK 후 결과 응답보내지 않음.
							TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];		// 결과데이터 OP-CODE
							TxBuf.Result_SEND_CC_Count		= 0;
						}
						else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)		//일괄스위치 -> 세대기로의 엘리베이터 콜 요청에 대한 ACK 일때.
						{
							if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	//요청에 대한 응답이므로 요청 중 일 때만 호출 성공으로 처리
							{
								BATCH_BLOCK_Control(SET__ELEVATOR_CALL);						//상태 엘리베이터 콜 (LED ON)
							}
							if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())						//응답은 보내지 않음.
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
					if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// 월패드 -> 스위치로의 결과 전송이면
					{
						BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);					// 월패드 -> 일괄스위치로 호출 실패 전송. LED OFF
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
			case KOCOM_OP_PROTOCOL_VERSION:						// 0x4A	프로토콜 버젼요청
			case KOCOM_OP_STATE_REQ:							//일괄 스위치에서는 일괄 조명상태를 조회함.
			case KOCOM_OP_LIGHT_CONTROL:						//0x3A로 요청했을때, 결과 응답의 OPCODE를 0x00으로 바꾸게 됨. 그때 마지막 응답을 받고도 결과를 더 보내는 상황이 생겨서 추가함
				if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_REQ) == KOCOM_CC_REQ)			// 데이터 요청이면	(0xBC, 0xBD, 0xBE)
				{
					KOCOM_BATCH_BLOCK_Model_ACK(pRx);		// OP-CODE	0x4A로 응답???? 전등스위치 ACK 응답
					TxBuf.Result_SEND_Flag			= KOCOM_SINGLE_ID;
					TxBuf.Result_SEND_OP_Code		= pRx->buf[KOCOM_F_OPCODE];		// 결과데이터 OP-CODE
					TxBuf.Result_SEND_CC_Count		= 0;
				}
				else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_ACK) == KOCOM_CC_ACK)	// 결과에 대한 응답이면	(0xDC, 0xDD, 0xDE)
				{
					if(pRx->buf[KOCOM_F_ADL] == Get_485_ID())
					{
						TxBuf.SEND_Flag				= 0;
					}					
					TxBuf.Result_SEND_Flag			= 0;
					TxBuf.Result_SEND_OP_Code		= 0;
					TxBuf.Result_SEND_CC_Count		= 0;
				}
				else if((pRx->buf[KOCOM_F_CC] & KOCOM_CC_NOT_ACK) == KOCOM_CC_NOT_ACK)		//211117 0x9C 응답 없음으로 NG 나서 추가함.
				{
					if(pRx->buf[KOCOM_F_OPCODE] == KOCOM_OP_STATE_REQ)						//0x3A로 상태 요청
					{
						if(Get_485_ID() == KOCOM_LOWEST_ID)							// 첫번째로 전송해야 할 ID이면
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
							SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), ON);	//EventCtrl 사용 시 Block_Active_Flag.Threeway가 1로 되어 문제가 됨
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
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_GAS)) == LED_FLASHING)		// 타이머가 0이고 가스 차단 요청중이면
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);		//가스 요청 취소하고 가스 차단 해제상태. 
			}
		}
	}
	else if(item2tsn(mapping_ITEM_COOK))
	{
		if(Gu16_GAS_Off_Tmr == 0)
		{
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_COOK)) == LED_FLASHING)		// 타이머가 0이고 가스 차단 요청중이면
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);		//가스 요청 취소하고 가스 차단 해제상태. 
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
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_GAS_n_COOK)) == LED_FLASHING)		// 타이머가 0이고 가스 차단 요청중이면
			{
				Gu8_Gas_State = 0;
				Gu8_Cook_State = 0;
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);		//가스 요청 취소하고 가스 차단 해제상태. 
			}
		}
	}

	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(Gu16_Elevator_Tmr == 0)
		{
			if((GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING) || (GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR))))		//LED Flashing(요청) 중이거나, 호출 성공인 상태일때, 일정 시간이 지나면
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);		//원래상태로 복귀.
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
						PUT_RelayCtrl(item2ctrl(mapping_ITEM_BATCH_LIGHT_n_GAS), OFF);		// 항목기준 제어
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
						PUT_RelayCtrl(item2ctrl(mapping_ITEM_BATCH_LIGHT_n_COOK), OFF);		// 항목기준 제어
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
		case SET__GAS_CLOSE_REQUEST:		// 가스차단은 스위치 상태가 켜져 있어야 차단이 설정된 상태이다. 차단 상태라도 차단 명령을 전송해야 함(프로토콜에 나옴).
			if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
			{
				//코콤은 차단중일 때도 차단 요청함.
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

				if(GET_Switch_State(touch_switch))		//가스 차단중일때
				{
					Gu16_GAS_Off_Tmr	= 60;						// 60초 경과 후 LED 소등(단 월패드에서 close/open 데이터가 수신되면 해당 상태로 전환), 
					if(item2tsn(mapping_ITEM_GAS))				Block_Active_Flag.Gas	= 1;
					else if(item2tsn(mapping_ITEM_COOK))		Block_Active_Flag.Cook	= 1;
					else if(item2tsn(mapping_ITEM_GAS_n_COOK))	Block_Active_Flag.Gas	= 1;

					Beep(ON);
					if(G_Trace)	printf("Gas/Cook State Close. Re-REQUEST\r\n");			//차단중이라도 재요청.
				}
				else													//가스 차단중이 아닐때
				{
					Gu16_GAS_Off_Tmr	= 60;						// 60초 경과 후 LED 소등(단 월패드에서 close/open 데이터가 수신되면 해당 상태로 전환), 
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
				if(Gu8_Gas_State != CLOSE)	//가스 차단 상태 아닐 때(열림, 요청)
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
				if(Gu8_Cook_State != CLOSE)	//가스 차단 상태 아닐 때(열림, 요청)
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

				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == OFF)		//가스밸브 차단 상태가 아니거나 가스차단 요청중(LED 점멸중)
				{
					if(item2tsn(mapping_ITEM_GAS_n_COOK))
					{
						if(Gu8_Gas_State == CLOSE || Gu8_Cook_State == CLOSE)	//하나라도 차단이면 LED는 ON
						{
							SET_LED_State(touch_switch, OFF);		// 실제로는 LED 켜짐
						}
						if(Gu8_Gas_State == CLOSE && Gu8_Cook_State == CLOSE)
						{
							SET_Switch_State(touch_switch, ON);	//모두 차단 되면 스위치 차단 상태로.
						}
					}
					else
					{
						SET_Switch_State(touch_switch, ON);		// 가스밸브 차단(닫힘)
						SET_LED_State(touch_switch, OFF);		// 실제로는 LED 켜짐
					}
					Beep(ON);
					if(G_Trace)	printf("GAS/Cook CLOSE\r\n");
				}
			}
			break;
		case SET__GAS_OPEN_STATE:
			if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_GAS))
			{
				if(Gu8_Gas_State != OPEN)	//가스 열림 상태 아닐 때(차단, 요청)
				{
					Gu8_Gas_State = OPEN;
					Beep(ON);
					if(G_Trace)	printf("CMD : OPEN, State : !OPEN -> OPEN\r\n");
				}
			}
			else if(item2tsn(mapping_ITEM_BATCH_LIGHT_n_COOK))
			{
				if(Gu8_Cook_State != OPEN)	//가스 열림 상태 아닐 때(차단, 요청)
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

				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == ON)		//가스 밸브 차단중이거나 가스 차단 요청중(LED 점멸중)
				{
					if(item2tsn(mapping_ITEM_GAS_n_COOK))
					{
						if(Gu8_Gas_State != CLOSE && Gu8_Cook_State != CLOSE)	//가스와 쿡탑 모두 닫힘상태가 아니면 -> 프로토콜 수신 시 해당 상태는 미리 처리함.
						{
							SET_Switch_State(touch_switch, OFF);	// 가스밸브 열림
							SET_LED_State(touch_switch, ON);		// 실제로는 LED 꺼짐
							if(G_Trace) printf("Gas, Cook All OPEN\r\n");
						}
					}
					else
					{
						SET_Switch_State(touch_switch, OFF);	// 가스밸브 열림
						SET_LED_State(touch_switch, ON);		// 실제로는 LED 꺼짐
						if(G_Trace)	printf("GAS OPEN\r\n");
					}
					Beep(ON);
				}
			}
			break;
		case SET__ELEVATOR_REQUEST:																			//스위치에서 엘리베이터 콜
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))		//스위치 OFF 거나 LED 상태 Flashing이면
			// if(GET_Switch_State(touch_switch) == OFF)														//스위치 OFF면
			{
				Gu16_Elevator_Tmr = 60;																		//요청 타이머 60초 초기화. 0이되면 요청 취소되고 LED OFF됨.
				Block_Active_Flag.Elevator = 1;
				SET_Switch_State(touch_switch, OFF);															//스위치 OFF
				SET_LED_State(touch_switch, LED_FLASHING);													//LED Flashing
				// SET_LED_State(touch_switch, LED_OFF);														//LED ON
				Beep(ON);
				if(G_Trace)	printf("ELEVATOR REQEUST\r\n");
			}
			break;
		case SET__ELEVATOR_CALL:																			//세대기에서 엘리베이터 콜
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == 0)
			{
				Gu16_Elevator_Tmr = 60;																			//콜 상태가 되면 타이머 60초 초기화. 타이머가 0되면 상태 원래대로 돌아감.
				touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
				SET_Switch_State(touch_switch, ON);																//스위치 ON
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
				if(GET_Switch_State(touch_switch))			//호출 상태일때
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, ON);				//도착 or 호출 실패시 LED OFF					
					Beep(ON);
					// printf("ELEVATOR ARRIVE\r\n");
				}
			}
			else if(control == SET__ELEVATOR_CALL_FAIL)
			{
				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch))	//호출 요청 상태거나, 호출상태 일 때
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, ON);				//도착 or 호출 실패시 LED OFF				
					// Beep(ON);									//엘리베이터 프로토콜은 연동테스트 되지 않아, 어떻게 동작하는지 모르므로 일단 부저는 삭제함.
				}
				// printf("ELEVATOR FAIL\r\n");
			}*/
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch))	//호출 요청 상태거나, 호출상태 일 때
			{
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, ON);				//도착 or 호출 실패시 LED OFF
#ifdef _COMMAX_PROTOCOL_
				Beep(ON);										//코맥스는 실질적으로 도착 데이터 사용하지 않는다고 하여 둘다 부저음 사용함.
#else
				if(control == SET__ELEVATOR_ARRIVE)				//도착에만 부저음 있음. 
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

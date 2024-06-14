/************************************************************************************
	Project		: ���ڽĽ���ġ
	File Name	: EL_Switch.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/07/10
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "eeprom.h"
#include "touch.h"
#include "el_switch.h"
#include "led.h"
#include "lcd.h"
#include "WDGnBeep.h"
#include "dimming.h"
#include "Relay.h"
#include "Debug.h"
#include "STPM3x_opt.h"
#include "rs-485.h"

#define		ELECTRICITY_DELAY_TIME		50		// 5sec
#define		LIGHT_DELAY_TIME			50		// 5sec
#define		GAS_DELAY_TIME				50		// 5sec

#define		LIGHT_SLEEP_1MINUTES		54		// 1minutes		60
#define		LIGHT_SLEEP_3MINUTES		162		// 3minutes		180
#define		LIGHT_SLEEP_5MINUTES		270		// 5minutes		300
#define		LIGHT_SLEEP_30MINUTES		1620	// 30minutes	1800
#define		LIGHT_SLEEP_60MINUTES		3240	// 60minutes	3600
#define		LIGHT_SLEEP_90MINUTES		4860	// 90minutes	4860

uint8_t				Gu8_PowerSaving_Tmr;
uint16_t			Gu16_Touch_Err_Tmr = 0;
uint8_t				Gu8_Touch_Err_Flag = 0;
uint8_t				Gu8_Touch_Err_Cnt = 0;
EVNET_BUF			Relay_CtrlBuff, PWM_CtrlBuff;

uint8_t				Gu8_SWITCH_Delay_Flag[mapping_ITEM_MAX];
uint8_t				Gu8_SWITCH_Delay_OFF_Tmr[mapping_ITEM_MAX];
uint8_t				Gu8_Special_Function_Key		= 0;
uint8_t				Gu8_Special_Function_Key_Tmr	= 0;

uint8_t				Gu8_Sleep_Flag			= 0;
uint16_t			Gu16_Light_Sleep_tmr	= 0;
uint8_t 			Gu8_Sleep_cnt			= 0;
uint8_t				Gu8_Sleep_Set_Tmr		= 0;

uint8_t				Gu8_Color_Temp_Flag = 0;
uint8_t				Gu8_Dim_Flag = 0;
uint8_t				Gu8_Light_n_ETC_Touch_Flag = 0;
uint8_t				Gu8_Elec_Touch_Flag  = 0;
uint8_t				Gu8_Direct_Control = 0;
void Touch_Error(uint8_t touch_switch, uint8_t tmr, uint8_t Flag);
//-------------------------------------------------------------------------------------------------------------------------

void EL_Switch_Init(void)
{
	uint8_t	i;
	
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	
	memset((void*)&Relay_CtrlBuff, 0, sizeof(EVNET_BUF));
	memset((void*)&PWM_CtrlBuff, 0, sizeof(EVNET_BUF));
	
	for(i=0;i<mapping_ITEM_MAX;i++)
	{
		Gu8_SWITCH_Delay_OFF_Tmr[i]	= 0;
		Gu8_SWITCH_Delay_Flag[i]	= 0;
	}
}

void EventCtrl(uint8_t touch_switch, uint8_t Flag)	// ����ġ
{
	uint8_t	i, item, tmp;
	static uint8_t old_touch_switch = 0;
// #if defined _ONE_SIZE_BATCH_BLOCK_MODEL_
	static uint8_t ThreeWay_State = 0;
// #endif
#ifdef _KOCOM_PROTOCOL_
	if(Touch_Use_Tmr)	return;
#endif
	if(touch_switch && touch_switch < mapping_SWITCH_MAX)
	{
		item	= tsn2item(touch_switch);
		switch(item)
		{
			case mapping_ITEM_LIGHT_GROUP:			// �׷콺��ġ
				if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_ALL)))		// ��ü ���� ���� ������
				{
					SET_Switch_State(item2tsn(mapping_ITEM_LIGHT_ALL), OFF);
					SET_LED_State(item2tsn(mapping_ITEM_LIGHT_ALL), OFF);	// ��ü�� LED �ҵ�
					ALL_Light_Switch_Ctrl((uint8_t)(pG_Config->Factory_Mapping_ALL_Light ^ pG_State->User_Mapping_ALL_Light), OFF, DELAY_OFF_PASS);		// ��ü��� �׷������ ������ ���� �� �׷���� �ƴ� ���� �ҵ�
				}
				Flag	= SET_Switch_State(touch_switch, Flag);				// Switch ���� �����ϰ� Switch ���·� Flag �缳��
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				if(Flag == OFF)
				{
					ALL_Light_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Light, Flag, DELAY_OFF_PASS);	// �������� �� � �������ؼ�
				}
				else
				{
					ALL_Light_Switch_Ctrl(pG_State->User_Mapping_ALL_Light, Flag, DELAY_OFF_PASS);
				}
				break;
			case mapping_ITEM_LIGHT_ALL:		// ���� �� ����ġ
				if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_GROUP)))	// �׷���� ���� ������
				{
					SET_Switch_State(item2tsn(mapping_ITEM_LIGHT_GROUP), OFF);
					SET_LED_State(item2tsn(mapping_ITEM_LIGHT_GROUP), OFF);	// �׷�� LED �ҵ�
					ALL_Light_Switch_Ctrl((uint8_t)(pG_Config->Factory_Mapping_ALL_Light ^ pG_State->User_Mapping_ALL_Light), OFF, DELAY_OFF_PASS);	// �׷��� ��ü������ ������ ���� �� ��ü���� �ƴ� ���� �ҵ�
				}
				Flag	= SET_Switch_State(touch_switch, Flag);				// Switch ���� �����ϰ� Switch ���·� Flag �缳��
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				ALL_Light_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Light, Flag, DELAY_OFF_CHECK);	//���� �ҵ� ����Ϸ��� ���
				// ALL_Light_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Light, Flag, DELAY_OFF_PASS);	//�����ҵ� �����Ϸ��� ���
				break;
			case mapping_ITEM_ELECTRICITY_ALL:	// ��ü ���� ����ġ
				if(Flag == INVERSE)	Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
				
				if(Flag == OFF)
				{
					//Beep(BEEP_FLASHING);
					SET_LED_State(touch_switch, LED_FLASHING);
					SET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_ELECTRICITY_ALL, ELECTRICITY_DELAY_TIME);				// ��ü ���� �����ҵ� ����
					if(pG_Config->Factory_Mapping_ALL_Electricity & CONTROL_BIT_RELAY_LATCH_1)
					{
						SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_1), LED_FLASHING);
						SET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_ELECTRICITY_1, ELECTRICITY_DELAY_TIME);				// ����1 �����ҵ� ����
					}
					if(pG_Config->Factory_Mapping_ALL_Electricity & CONTROL_BIT_RELAY_LATCH_2)
					{
						SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_2), LED_FLASHING);		// ����2 �����ҵ� ����
						SET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_ELECTRICITY_2, ELECTRICITY_DELAY_TIME);
					}
				}
				else
				{
					Flag	= SET_Switch_State(touch_switch, Flag);					// Switch ���� �����ϰ� Switch ���·� Flag �缳��
					SET_LED_State(touch_switch, Flag);
					Beep(Flag);
					//Beep(BEEP_MEL);
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);				// ��ü ���� �����ҵ� Flag Ŭ����
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_1, 0);					// ����1 �����ҵ� Flag Ŭ����
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_2, 0);					// ����2 �����ҵ� Flag Ŭ����
					ALL_Electricity_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Electricity, Flag);	// ��ü ���� ����
				}
				break;
			case mapping_ITEM_3WAY_1:
			case mapping_ITEM_3WAY_2:
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_		//�ϰ�����ġ���� ��� ��� ���� ��
#ifdef COMM_THREEWAY
	#ifdef _HYUNDAI_PROTOCOL_		//���� ���������̸�, ���� ���̾� �����ϴ� ����� �ƴ϶� ���е� ��� ������ ��츸 LED ������.
				Gu8_External_Flag = 1;
				SET_Switch_State(touch_switch, INVERSE);
				SET_LED_State(touch_switch, LED_DETECT_ON);
				Beep(ON);
	#elif defined _CVNET_PROTOCOL_
				Flag	= SET_Switch_State(touch_switch, Flag);
				SET_LED_State(touch_switch, Flag);
				if(Flag == ON)	Gu8_3Way_Flag = 0x01;
				else			Gu8_3Way_Flag = 0x02;
				Beep(Flag);
	#elif defined _KOCOM_PROTOCOL_
				Flag	= SET_Switch_State(touch_switch, Flag);
				SET_LED_State(touch_switch, Flag);
				Block_Active_Flag.Threeway = 1;
				Beep(Flag);
				Touch_Use_Tmr = 5;	//500ms
	#else		//����, �����̳� ���� ��������
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch ���� �����ϰ� Switch ���·� Flag �缳��
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);	// �׸���� ����
	#endif
#endif	//COMM_THREEWAY
#ifdef WIRING_THREEWAY
#ifdef _KOCOM_PROTOCOL_
				Flag	= SET_Switch_State(touch_switch, Flag);
				SET_LED_State(touch_switch, LED_DETECT_ON);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);	// �׸���� ����
				Touch_Use_Tmr = 5;	//500ms
#else
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch ���� �����ϰ� Switch ���·� Flag �缳��
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);	// �׸���� ����
#endif	//_KOCOM_PROTOCO:_
#endif	//WIRING THREEWAY
#else	//�ϰ� ����ġ �ƴ� ��
	#if defined _NO_PROTOCOL_ && defined THREEWAY_TRANS
				Gu8_Direct_Control = 1;
	#endif
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch ���� �����ϰ� Switch ���·� Flag �缳��
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);	// �׸���� ����
				ALL_n_Group_Light_Switch_LED_Ctrl();		//210708 ��ü�� & �׷�� LED ����
#endif

				break;
			case mapping_ITEM_LIGHT_1:
			case mapping_ITEM_LIGHT_2:
			case mapping_ITEM_LIGHT_3:
			case mapping_ITEM_LIGHT_4:
			case mapping_ITEM_LIGHT_5:
			case mapping_ITEM_LIGHT_6:
				if(Flag == INVERSE)
				{
					Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
				}
				/*	// ���� ����� �����ҵ� ���ٰ� ��(�����)
				if(pG_State->ETC.DelayOFF)
				{
					if(GET_Light_ON_Count() == 1)	// ���� ���� ������ ���̸�
					{
						if(Flag == OFF)
						{
							SET_LED_State(touch_switch, LED_FLASHING);
							Beep(BEEP_ONE);
							SET_SWITCH_Delay_OFF_Tmr(item, LIGHT_DELAY_TIME);
							break;
						}
					}
				}
				*/
				
#if 0
				// �����ҵ� �� �ٽ� ������ �� �ٷ� ������ �ϸ� �� �κ� ������ ��
				if(GET_LED_State(touch_switch) == LED_FLASHING)
				{
					SET_SWITCH_Delay_OFF_Flag(item, 0);
					Flag	= OFF;
				}
#endif
				
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch ���� �����ϰ� Switch ���·� Flag �缳��
				if(Flag == ON)
				{
					for(i=mapping_ITEM_LIGHT_1;i<mapping_ITEM_LIGHT_6;i++)
					{
						SET_SWITCH_Delay_OFF_Flag(i, 0);
						if(tmp	= item2tsn(i))
						{
							if(GET_LED_State(tmp) == LED_FLASHING)	SET_LED_State(tmp, LED_ON);
						}
					}
				}
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);		// �׸���� ����
				ALL_n_Group_Light_Switch_LED_Ctrl();		// ��ü�� & �׷�� LED ����
				break;
			case mapping_ITEM_ELECTRICITY_1:
			case mapping_ITEM_ELECTRICITY_2:
				//printf("mapping_ITEM_SETUP %d\n", GET_Switch_State(item2tsn(mapping_ITEM_SETUP)));
				if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))	// ���� ���� ���̸�
				{
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);				// ��ü ���� �����ҵ� Flag Ŭ����
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_1, 0);					// ����1 �����ҵ� Flag Ŭ����
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_2, 0);					// ����2 �����ҵ� Flag Ŭ����
					if(item == mapping_ITEM_ELECTRICITY_1)
					{
						pG_State->ETC.Auto1	= pG_State->ETC.Auto1 ? 0 : 1;
					}
					else
					{
						pG_State->ETC.Auto2	= pG_State->ETC.Auto2 ? 0 : 1;
					}
					if(pG_Config->Enable_Flag.LCD_Type == 0)
					{
						if(pG_State->ETC.Auto1)	SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_1), LED_FLASHING);
						else					SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_1), ON);
						if(pG_State->ETC.Auto2)	SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_2), LED_FLASHING);
						else					SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_2), ON);
					}
					Beep(ON);
				}
				else
				{
					if(Flag == INVERSE)
					{
						Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
					}
					
					if(Flag == OFF)
					{
						SET_LED_State(touch_switch, LED_FLASHING);
						//Beep(BEEP_FLASHING);
						SET_SWITCH_Delay_OFF_Tmr(item, ELECTRICITY_DELAY_TIME);
					}
					else
					{
						Flag	= SET_Switch_State(touch_switch, Flag);			// Switch ���� �����ϰ� Switch ���·� Flag �缳��
						SET_LED_State(touch_switch, Flag);
						Beep(Flag);
						//Beep(BEEP_MEL);
						PUT_RelayCtrl(item2ctrl(item), Flag);	// �׸���� ����
						SET_SWITCH_Delay_OFF_Flag(item, 0);
						SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
						//----------------------------------------------------------------------------------------------------------
						ALL_Electricity_Switch_LED_Ctrl();
						//----------------------------------------------------------------------------------------------------------
#ifdef _HDC_PROTOCOL_	//HDC ���������� ������ ���� �� �ڵ� ������ ��� �����͸� �����ϴµ� EventCtrl�� ���� �� ��� �ش� ������ �ʱ�ȭ
						if(item == mapping_ITEM_ELECTRICITY_1)
						{
							Gu8_Elec_Auto_OFF[0]		= 0;
							Gu8_Elec_Overload_OFF[0]	= 0;
						}
						else
						{
							Gu8_Elec_Auto_OFF[1]		= 0;
							Gu8_Elec_Overload_OFF[1]	= 0;
						}
#endif
					}
				}
				break;
			case mapping_ITEM_DIMMING_LIGHT_1:
			case mapping_ITEM_DIMMING_LIGHT_2:
				if(Flag == INVERSE)
				{
					Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
				}
				/*	// ���� ����� �����ҵ� ���ٰ� ��(�����)
				if(pG_State->ETC.DelayOFF)
				{
					if(GET_Light_ON_Count() == 1)	// ���� ���� ������ ���̸�
					{
						if(Flag == OFF)
						{
							SET_LED_State(touch_switch, LED_FLASHING);
							Beep(BEEP_ONE);
							SET_SWITCH_Delay_OFF_Tmr(item, LIGHT_DELAY_TIME);
							break;
						}
					}
				}
				*/
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch ���� �����ϰ� Switch ���·� Flag �缳��
				if(Flag == ON)
				{
					for(i=mapping_ITEM_DIMMING_LIGHT_1;i<mapping_ITEM_DIMMING_LIGHT_2;i++)
					{
						SET_SWITCH_Delay_OFF_Flag(i, 0);
						if(tmp	= item2tsn(i))
						{
							if(GET_LED_State(tmp) == LED_FLASHING)	SET_LED_State(tmp, LED_ON);
						}
					}
				}
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				PUT_PWMCtrl(item2ctrl(item), Flag);			// �׸���� ����
				PUT_RelayCtrl(item2ctrl(item), Flag);		// 20210604 ������ ������ ���� ���� �ϵ��� �߰�
				ALL_n_Group_Light_Switch_LED_Ctrl();		// 210621 ��������� ��ü�� LED�� ����ȹ޵���
				break;
			case mapping_ITEM_DIMMING_UP:
				Gu8_LCD_DIM_Tmr					= 50;		// 5s ���� LCD ǥ��
				Gu8_LCD_ElecLimitCurrent_Tmr	= 0;

				SET_LED_State(touch_switch, LED_DETECT_ON);
				Beep(ON);
				if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))
				{
					Gu8_Dim_Flag = 0;		//210623
					Gu8_Color_Temp_Flag = 1;
				}
				else
				{
					Gu8_Dim_Flag = 1;		//210623
					Gu8_Color_Temp_Flag = 0;
				}
				if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)) && GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)) )	// ��� ���� ��� ���� ������
				{
					PWM_Level_Set((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), LEVEL_UP);
					PUT_PWMCtrl((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	// ���1 ���� ���� ������
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_1, LEVEL_UP);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	// ���2 ���� ���� ������
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_2, LEVEL_UP);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
				}
				break;
			case mapping_ITEM_DIMMING_DN:
				Gu8_LCD_DIM_Tmr					= 50;		// 5s ���� LCD ǥ��
				Gu8_LCD_ElecLimitCurrent_Tmr	= 0;

				SET_LED_State(touch_switch, LED_DETECT_ON);
				Beep(ON);
				if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))
				{
					Gu8_Dim_Flag = 0;		//210623
					Gu8_Color_Temp_Flag = 1;
				}
				else
				{
					Gu8_Dim_Flag = 1;		//210623
					Gu8_Color_Temp_Flag = 0;
				}
				if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)) && GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)) )	// ��� ���� ��� ���� ������
				{
					PWM_Level_Set((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), LEVEL_DN);
					PUT_PWMCtrl((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	// ���1 ���� ���� ������
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_1, LEVEL_DN);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	// ���2 ���� ���� ������
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_2, LEVEL_DN);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
				}
				break;
			case mapping_ITEM_COLOR_TEMP_UP:
				/*Gu8_LCD_DIM_Tmr					= 50;		// 5s ���� LCD ǥ��
				Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
				Gu8_Color_Temp_Flag = 1;
				Gu8_Dim_Flag = 0;
				SET_LED_State(touch_switch, LED_DETECT_ON);
				Beep(ON);
				if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)) && GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)) )	// ��� ���� ��� ���� ������
				{
					PWM_Level_Set((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), LEVEL_UP);
					PUT_PWMCtrl((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), ON);
				}				
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	// ���1 ���� ���� ������
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_1, LEVEL_UP);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	// ���2 ���� ���� ������
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_2, LEVEL_UP);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
				 }*/
				break;
			case mapping_ITEM_COLOR_TEMP_DN:
				/*Gu8_LCD_DIM_Tmr					= 50;		// 5s ���� LCD ǥ��
				Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
				Gu8_Color_Temp_Flag = 1;
				Gu8_Dim_Flag = 0;
				SET_LED_State(touch_switch, LED_DETECT_ON);
				Beep(ON);
				if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)) && GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)) )	// ��� ���� ��� ���� ������
				{
					PWM_Level_Set((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), LEVEL_DN);
					PUT_PWMCtrl((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), ON);
				}				
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	// ���1 ���� ���� ������
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_1, LEVEL_DN);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
				}				
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	// ���2 ���� ���� ������
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_2, LEVEL_DN);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
				}*/
				break;
			case mapping_ITEM_SETUP:
				Flag	= SET_Switch_State(touch_switch, Flag);
				if(Flag == ON)
				{
					SET_LED_State(touch_switch, LED_FLASHING);
					if(pG_Config->Enable_Flag.LCD_Type == 0)
					{
						if(pG_State->ETC.Auto1)	SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_1), LED_FLASHING);
						else					SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_1), ON);	//LED OFF
						if(pG_State->ETC.Auto2)	SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_2), LED_FLASHING);
						else					SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_2), ON);	//LED OFF
					}
				}
				else
				{
					SET_LED_State(touch_switch, OFF);
					if(pG_Config->Enable_Flag.LCD_Type == 0)
					{
						if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1)))	SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_1), ON);	//LED OFF
						else														SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_1), OFF);
						if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2)))	SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_2), ON);	//LED OFF
						else														SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_2), OFF);
					}
				}
				Beep(Flag);
				break;

			case mapping_ITEM_SLEEP:		//20210402
				if(Flag == ON)
				{
					if(GET_Switch_State(touch_switch) == 0)
					{
						if(GET_LED_State(touch_switch) != LED_FLASHING)
						{
							Gu8_Sleep_Flag = 0;			//�������̶� �ٽ� ��ġ�ϸ� �÷��� �ʱ�ȭ
							Gu8_Sleep_cnt = 0;				//�������̶� �ٽ� ��ġ�ϸ� cnt �ʱ�ȭ		
							Gu8_Sleep_Set_Tmr = 5;			
							SET_Switch_State(touch_switch, ON);
							SET_LED_State(touch_switch, LED_FLASHING);
							Beep(ON);
							printf("SLEEP SET ON\r\n");
						}
					}
				}
				else
				{
					if(GET_Switch_State(touch_switch))
					{
						Gu16_Light_Sleep_tmr = 0;
						Gu8_Sleep_Flag = 0;
						Gu8_Sleep_cnt = 0;
						SET_Switch_State(touch_switch, OFF);
						SET_LED_State(touch_switch, ON);
						Beep(ON);
						printf("SLEEP SET OFF\r\n");
					}
				}
				/*
				Flag 	= SET_Switch_State(touch_switch, Flag);
				if(Flag == ON)
				{
					Gu8_Sleep_Flag = 0;			//�������̶� �ٽ� ��ġ�ϸ� �÷��� �ʱ�ȭ
					Gu8_Sleep_cnt = 0;				//�������̶� �ٽ� ��ġ�ϸ� cnt �ʱ�ȭ
					SET_LED_State(touch_switch, LED_FLASHING);
					printf("SLEEP SET START\r\n");
					Beep(Flag);
				}
				else
				{
					SET_LED_State(touch_switch, ON);
					printf("SLEEP SET\r\n");
					// Beep(Flag);
				}
				*/
				break;

			case mapping_ITEM_BATCH_LIGHT_OFF:
// #ifndef _HYUNDAI_PROTOCOL_
#if 0
				//if(pG_State->ETC.DelayOFF)		// jsyoon 20200925
				{
					if(Flag == OFF)
					{
						if(GET_LED_State(touch_switch) == LED_FLASHING)
						{
							SET_SWITCH_Delay_OFF_Flag(item, 0);
						}
						else
						{
							SET_LED_State(touch_switch, LED_FLASHING);
							Beep(BEEP_ONE);
							SET_SWITCH_Delay_OFF_Tmr(item, LIGHT_DELAY_TIME);
							// Gu8_BatchLight_Flag = 1;		//210617
							break;
						}
					}		//�ϰ��ҵ� �� �����ҵ��� �ʿ������ IF�� �ּ� ó��.
				}
#endif				
#ifdef _KOCOM_PROTOCOL_
				if(Batch_Light_Use_Tmr == 0)
				{
					Flag	= SET_Switch_State(touch_switch, Flag);			// Switch ���� �����ϰ� Switch ���·� Flag �缳��
					SET_LED_State(touch_switch, Flag);
					Beep(Flag);
					// �ϰ��ҵ��� �����ٴ� ���� ������ OFF �������� ������ ������ ���ٴ� �ǹ���
					// �ϰ��ҵ��� �����ٴ� ���� ������ ON �������� ������ ��� ������ ���޵Ǿ� ��� �� �� �ִٴ� �ǹ���
					PUT_RelayCtrl(item2ctrl(item), Flag);		// �׸���� ����

					Block_Active_Flag.Batch = 1;
					Batch_Light_Use_Tmr = 3;
#ifdef COMM_THREEWAY
					if(item2tsn(mapping_ITEM_3WAY_1))
					{
						if(Flag == OFF)	//�ϰ� �ҵ� ��
						{
							ThreeWay_State = GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
							{
								SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
								SET_LED_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
								PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_1), OFF);
							}
						}
						else	//�ϰ� �ҵ� ���� ��
						{
							if(ThreeWay_State)	//�ҵ� ���� ���°� ON�̸� �ҵ� ������ �Բ� ON���� ����
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == OFF)
								{
									SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), ON);
									SET_LED_State(item2tsn(mapping_ITEM_3WAY_1), ON);
									PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_1), ON);
								}
							}
						}
					}
#endif
					Touch_Use_Tmr = 5;	//500ms
				}
#else

				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch ���� �����ϰ� Switch ���·� Flag �缳��
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				// �ϰ��ҵ��� �����ٴ� ���� ������ OFF �������� ������ ������ ���ٴ� �ǹ���
				// �ϰ��ҵ��� �����ٴ� ���� ������ ON �������� ������ ��� ������ ���޵Ǿ� ��� �� �� �ִٴ� �ǹ���
				PUT_RelayCtrl(item2ctrl(item), Flag);		// �׸���� ����
/*#ifdef _CVNET_PROTOCOL_
				if(item2tsn(mapping_ITEM_3WAY_1))
				{
					if(Flag == OFF)
					{
						ThreeWay_State = GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
						if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
						{
							EventCtrl(item2tsn(mapping_ITEM_3WAY_1), OFF);
						}
					}
					else
					{
						if(ThreeWay_State)
						{
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == OFF)
							{
								EventCtrl(item2tsn(mapping_ITEM_3WAY_1), ON);
							}
						}
					}
				}
#endif	//_CVNET_PROTOCOL_*/
#endif
				break;
			case mapping_ITEM_BATCH_LIGHT_n_GAS:	//240307
			case mapping_ITEM_BATCH_LIGHT_n_COOK:
#ifdef _KOCOM_PROTOCOL_
				if(Batch_Light_Use_Tmr == 0)
				{
					// printf("Switch %d, Flag %d\r\n", (uint16_t)GET_Switch_State(item2tsn(item)), (uint16_t)Flag);
					Flag	= SET_Switch_State(touch_switch, Flag);
					SET_LED_State(touch_switch, Flag);
					Beep(Flag);
					PUT_RelayCtrl(item2ctrl(item), Flag);		// �׸���� ����
					if(Flag == OFF)
					{
						BATCH_BLOCK_Control(SET__GAS_CLOSE_REQUEST);
						if(item == mapping_ITEM_BATCH_LIGHT_n_GAS)			Block_Active_Flag.Gas = 1;	//���� ���� ��û�� �ϰ� �ҵ�ÿ��� ����
						else if(item == mapping_ITEM_BATCH_LIGHT_n_COOK)	Block_Active_Flag.Cook = 1;	//��ž ���� ��û�� �ϰ� �ҵ�ÿ��� ����
						Gu8_Direct_Control = 1;
					}
					else	//�ϰ� �ҵ� �������� �ϰ� �����͸� ��� ����
					{
						Block_Active_Flag.Batch = 1;
					}
					Batch_Light_Use_Tmr = 3;
#ifdef COMM_THREEWAY
					if(item2tsn(mapping_ITEM_3WAY_1))
					{
						if(Flag == OFF)	//�ϰ� �ҵ� ��
						{
							ThreeWay_State = GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
							{
								SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
								SET_LED_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
								PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_1), OFF);
							}
						}
						else	//�ϰ� �ҵ� ���� ��
						{
							if(ThreeWay_State)	//�ҵ� ���� ���°� ON�̸� �ҵ� ������ �Բ� ON���� ����
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == OFF)
								{
									SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), ON);
									SET_LED_State(item2tsn(mapping_ITEM_3WAY_1), ON);
									PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_1), ON);
								}
							}
							else	//�ҵ� ���� ���°� OFF�� �ҵ� ���� �� ���� ���� ������.
							{
								;
							}
						}
					}
#endif
					Touch_Use_Tmr = 5;	//500ms
				}
#else
#ifndef _NO_PROTOCOL_
				Flag	= SET_Switch_State(touch_switch, Flag);
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);		// �׸���� ����
				if(Flag == OFF)
				{
					BATCH_BLOCK_Control(SET__GAS_CLOSE_REQUEST);
				}
#endif
#endif
				break;
			case mapping_ITEM_GAS:		// ������ ���ܸ� ����
			case mapping_ITEM_COOK:
#ifndef _NO_PROTOCOL_
	#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
				BATCH_BLOCK_Control(SET__GAS_CLOSE_REQUEST);
#ifdef _KOCOM_PROTOCOL_
				Touch_Use_Tmr = 5;	//500ms
#endif
	#endif	//_ONE_SIZE_BATCH_BLOCK_MODEL_
#else		//NO PROTOCOL �϶� ���� ���� ����.
				Flag	= SET_Switch_State(touch_switch, ON);
				SET_LED_State(touch_switch, LED_FLASHING);
				SET_SWITCH_Delay_OFF_Tmr(item, GAS_DELAY_TIME);
				Beep(ON);
			
#endif
				break;
			case mapping_ITEM_GAS_n_COOK:
#ifdef _KOCOM_PROTOCOL_
				Gu8_Direct_Control = 1;
				Touch_Use_Tmr = 15;
				BATCH_BLOCK_Control(SET__GAS_CLOSE_REQUEST);
#endif
				break;
			case mapping_ITEM_ELEVATOR:
#ifdef _NO_PROTOCOL_
				Flag	= SET_Switch_State(touch_switch, Flag);
				if(Flag == ON)
				{
					SET_LED_State(touch_switch, LED_FLASHING);
					Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_CALL;
					Beep(ON);
					// ELEVATOR_Call();
				}
				else
				{
					SET_LED_State(touch_switch, ON);
					Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_CANCEL;
					Beep(ON);
					// ELEVATOR_Cancel();
				}			
#endif

// #if defined(_KOCOM_PROTOCOL_) || defined(_COMMAX_PROTOCOL_) || defined(_HYUNDAI_PROTOCOL_) || defined(_CVNET_PROTOCOL_) || defined(_KDW_PROTOCOL_) || defined(_HW_PROTOCOL_) || defined(_HDC_PROTOCOL_)
#ifndef _NO_PROTOCOL_
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_
				BATCH_BLOCK_Control(SET__ELEVATOR_REQUEST);
#ifdef _KOCOM_PROTOCOL_
				Touch_Use_Tmr = 5;	//500ms
#endif
#endif	//_ONE_SIZE_BATCH_BLOCK_MODEL_
#endif
				break;
// #endif
		}
		Touch_Error(touch_switch, 0, 1);
	}
}
//--------------------------------------------------------------------------------------------------------------
void SWITCH_Delay_OFF_Process(void)
{
	uint8_t item;
	uint8_t touch_switch;
	uint8_t	ctrl;
	uint8_t	Flag	= OFF;
	
	for(item=mapping_ITEM_LIGHT_1;item<=mapping_ITEM_DIMMING_LIGHT_2;item++)
	{
		if(GET_SWITCH_Delay_OFF_Flag(item))
		{
			//printf("(%d)\n", item);	// TEST
			if(GET_SWITCH_Delay_OFF_Tmr(item) == 0)
			{				
				touch_switch	= item2tsn(item);
				SET_Switch_State(touch_switch, Flag);
				//SET_LED_State(touch_switch, Flag);
				SET_LED_State(touch_switch, OFF);
				Beep(Flag);
								
				ctrl	= item2ctrl(item);
				
				//printf("[%d%X]\n", touch_switch, ctrl);	// TEST
				if(ctrl & (uint8_t)(CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2) )
				{
					//printf("p\n");	// TEST
					PUT_RelayCtrl(ctrl, Flag);	//240229
					PUT_PWMCtrl(ctrl, Flag);
				}
				else
				{
					//printf("P");	// TEST
					PUT_RelayCtrl(ctrl, Flag);
				}
				ALL_n_Group_Light_Switch_LED_Ctrl();		// ��ü�� & �׷�� LED ����
				SET_SWITCH_Delay_OFF_Flag(item, 0);
			}
		}
	}
	
	if(GET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL))
	{
		if(GET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_ELECTRICITY_ALL) == 0)
		{
			touch_switch	= item2tsn(mapping_ITEM_ELECTRICITY_ALL);
			SET_Switch_State(touch_switch, OFF);
			SET_LED_State(touch_switch, OFF);
			//if(beep == 0)
			{
				Beep(OFF);
				//beep = 1;
			}
			ALL_Electricity_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Electricity, OFF);	// ��ü�� �ҵ�
			
			SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);
		}
	}
	
	for(item=mapping_ITEM_ELECTRICITY_1;item<=mapping_ITEM_ELECTRICITY_2;item++)
	{
		if(GET_SWITCH_Delay_OFF_Flag(item))
		{
			if(GET_SWITCH_Delay_OFF_Tmr(item) == 0)
			{
				touch_switch	= item2tsn(item);
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, OFF);
				//if(beep == 0)
				{
					Beep(OFF);
					//beep = 1;
				}
				PUT_RelayCtrl(item2ctrl(item), OFF);	// �׸���� ����
				
				SET_SWITCH_Delay_OFF_Flag(item, 0);
				
				ALL_Electricity_Switch_LED_Ctrl();
			}
		}
	}
/*#ifdef _HYUNDAI_PROTOCOL_		//���� ��������, 5�� ������ �������� LED ON
	item = mapping_ITEM_GAS;
	if(GET_SWITCH_Delay_OFF_Flag(item))
	{
		if(GET_SWITCH_Delay_OFF_Tmr(item) == 0)
		{				
			touch_switch	= item2tsn(item);
			SET_Switch_State(touch_switch, OFF);
			SET_LED_State(touch_switch, LED_OFF);		//������ ���� �� LED ON
			Beep(ON);
			Gu8_Gas_Flag = 1;
			SET_SWITCH_Delay_OFF_Flag(item, 0);
		}
	}
#endif*/
#ifdef _NO_PROTOCOL_
	if(GET_SWITCH_Delay_OFF_Flag(mapping_ITEM_GAS))
	{
		if(GET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_GAS) == 0)
		{
			if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)
			{
				touch_switch	= item2tsn(mapping_ITEM_GAS);
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, ON);
				SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_GAS, 0);
			}
		}
	}
#endif	
}
//--------------------------------------------------------------------------------------------------------------
void SET_SWITCH_Delay_OFF_Tmr(uint8_t item, uint8_t tmr)
{
	if(item > mapping_ITEM_DISABLE && item < mapping_ITEM_MAX)
	{
		SET_SWITCH_Delay_OFF_Flag(item, 1);
		Gu8_SWITCH_Delay_OFF_Tmr[item]	= tmr;		// 5sec �� �ҵ�
	}
	else
	{
		printf("ERR : set d-off \n");
	}
}

uint8_t GET_SWITCH_Delay_OFF_Tmr(uint8_t item)
{
	uint8_t	ret	 = 0;
	
	if(item > mapping_ITEM_DISABLE && item < mapping_ITEM_MAX)
	{
		ret	= Gu8_SWITCH_Delay_OFF_Tmr[item];
	}
	else
	{
		//ret	= 0xFF;
		printf("ERR : get d-off\n");
	}
	
	return ret;
}

void SET_SWITCH_Delay_OFF_Flag(uint8_t item, uint8_t Flag)
{
	if(item > mapping_ITEM_DISABLE && item < mapping_ITEM_MAX)
	{
		Gu8_SWITCH_Delay_Flag[item]		= Flag;
	}
	else
	{
		//ret	= 0xFF;
		printf("ERR : set d-off flag\n");
	}
}

uint8_t GET_SWITCH_Delay_OFF_Flag(uint8_t item)
{
	uint8_t	ret	 = 0;
	
	if(item > mapping_ITEM_DISABLE && item < mapping_ITEM_MAX)
	{
		ret	= Gu8_SWITCH_Delay_Flag[item];
	}
	else
	{
		//ret	= 0xFF;
		printf("ERR : get d-off flag\n");
	}
	
	return ret;
}
//--------------------------------------------------------------------------------------------------------------
void PUT_RelayCtrl(uint8_t Ctrl, uint8_t Flag)
{
	Relay_CtrlBuff.Ctrl[Relay_CtrlBuff.PUT_Event_Cnt]	= Ctrl;
	Relay_CtrlBuff.Flag[Relay_CtrlBuff.PUT_Event_Cnt]	= Flag;
	Relay_CtrlBuff.PUT_Event_Cnt++;
	if(Relay_CtrlBuff.PUT_Event_Cnt >= MAX_EVENT_BUF)	Relay_CtrlBuff.PUT_Event_Cnt	= 0;
}

void PUT_PWMCtrl(uint8_t Ctrl, uint8_t Flag)
{
	PWM_CtrlBuff.Ctrl[PWM_CtrlBuff.PUT_Event_Cnt]	= Ctrl;
	PWM_CtrlBuff.Flag[PWM_CtrlBuff.PUT_Event_Cnt]	= Flag;
	PWM_CtrlBuff.PUT_Event_Cnt++;
	if(PWM_CtrlBuff.PUT_Event_Cnt >= MAX_EVENT_BUF)	PWM_CtrlBuff.PUT_Event_Cnt	= 0;
}

uint8_t Event_Empt(void)
{
	uint8_t	ret	= 0;
	
	if(Relay_CtrlBuff.PUT_Event_Cnt == Relay_CtrlBuff.GET_Event_Cnt)
	{
		if(PWM_CtrlBuff.PUT_Event_Cnt == PWM_CtrlBuff.GET_Event_Cnt)
		{
			ret	= 1;
		}
	}
	
	return ret;
}

void GET_Event(void)	// Zorocrossing ���¿��� ������ �� PWM�� ����
{
	if(Relay_CtrlBuff.PUT_Event_Cnt != Relay_CtrlBuff.GET_Event_Cnt)
	{
		Relay_Ctrl(Relay_CtrlBuff.Ctrl[Relay_CtrlBuff.GET_Event_Cnt], Relay_CtrlBuff.Flag[Relay_CtrlBuff.GET_Event_Cnt]);	// ������ ����
		Relay_CtrlBuff.GET_Event_Cnt++;
		if(Relay_CtrlBuff.GET_Event_Cnt >= MAX_EVENT_BUF)	Relay_CtrlBuff.GET_Event_Cnt	= 0;
	}
	else if(PWM_CtrlBuff.PUT_Event_Cnt != PWM_CtrlBuff.GET_Event_Cnt)
	{
		PWM_Ctrl(PWM_CtrlBuff.Ctrl[PWM_CtrlBuff.GET_Event_Cnt], PWM_CtrlBuff.Flag[PWM_CtrlBuff.GET_Event_Cnt]);				// PWM ����
		PWM_CtrlBuff.GET_Event_Cnt++;
		if(PWM_CtrlBuff.GET_Event_Cnt >= MAX_EVENT_BUF)	PWM_CtrlBuff.GET_Event_Cnt	= 0;
	}
}
//--------------------------------------------------------------------------------------------------------------
uint8_t SET_Switch_State(uint8_t touch_switch, uint8_t Flag)
{
	uint8_t	Led_Flag;
	
	if(Flag == INVERSE)
	{
		Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
	}
	
	switch(touch_switch)
	{
		case 1:		pG_State->SW_State.Bit.N1	= Flag;	break;
		case 2:		pG_State->SW_State.Bit.N2	= Flag;	break;
		case 3:		pG_State->SW_State.Bit.N3	= Flag;	break;
		case 4:		pG_State->SW_State.Bit.N4	= Flag;	break;
		case 5:		pG_State->SW_State.Bit.N5	= Flag;	break;
		case 6:		pG_State->SW_State.Bit.N6	= Flag;	break;
		case 7:		pG_State->SW_State.Bit.N7	= Flag;	break;
		case 8:		pG_State->SW_State.Bit.N8	= Flag;	break;
		case 9:		pG_State->SW_State.Bit.N9	= Flag;	break;
		case 10:	pG_State->SW_State.Bit.N10	= Flag;	break;
		case 11:	pG_State->SW_State.Bit.N11	= Flag;	break;
		case 12:	pG_State->SW_State.Bit.N12	= Flag;	break;
		case 13:	pG_State->SW_State.Bit.N13	= Flag;	break;
		case 14:	pG_State->SW_State.Bit.N14	= Flag;	break;
		case 15:	pG_State->SW_State.Bit.N15	= Flag;	break;
		case 16:	pG_State->SW_State.Bit.N16	= Flag;	break;
		default:	Flag	= 0;	break;
	}
	
	return Flag;
}

uint8_t GET_Switch_State(uint8_t touch_switch)
{
	uint8_t	ret = 0;
	switch(touch_switch)
	{
		case 1:		ret	= pG_State->SW_State.Bit.N1;	break;
		case 2:		ret	= pG_State->SW_State.Bit.N2;	break;
		case 3:		ret	= pG_State->SW_State.Bit.N3;	break;
		case 4:		ret	= pG_State->SW_State.Bit.N4;	break;
		case 5:		ret	= pG_State->SW_State.Bit.N5;	break;
		case 6:		ret	= pG_State->SW_State.Bit.N6;	break;
		case 7:		ret	= pG_State->SW_State.Bit.N7;	break;
		case 8:		ret	= pG_State->SW_State.Bit.N8;	break;
		case 9:		ret	= pG_State->SW_State.Bit.N9;	break;
		case 10:	ret	= pG_State->SW_State.Bit.N10;	break;
		case 11:	ret	= pG_State->SW_State.Bit.N11;	break;
		case 12:	ret	= pG_State->SW_State.Bit.N12;	break;
		case 13:	ret	= pG_State->SW_State.Bit.N13;	break;
		case 14:	ret	= pG_State->SW_State.Bit.N14;	break;
		case 15:	ret	= pG_State->SW_State.Bit.N15;	break;
		case 16:	ret	= pG_State->SW_State.Bit.N16;	break;
		default:	ret	= 0;	break;
	}
	return ret;
}

uint8_t	GET_Light_State(void)		// ����
{
	uint8_t	ret	= 0;
	
	if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))				ret	|= item2ctrl(mapping_ITEM_3WAY_1);
	if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_2)))				ret	|= item2ctrl(mapping_ITEM_3WAY_2);
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_1)))			ret	|= item2ctrl(mapping_ITEM_LIGHT_1);
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_2)))			ret	|= item2ctrl(mapping_ITEM_LIGHT_2);
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_3)))			ret	|= item2ctrl(mapping_ITEM_LIGHT_3);
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_4)))			ret	|= item2ctrl(mapping_ITEM_LIGHT_4);
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_5)))			ret	|= item2ctrl(mapping_ITEM_LIGHT_5);
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_6))) 			ret	|= item2ctrl(mapping_ITEM_LIGHT_6);
	if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	ret	|= item2ctrl(mapping_ITEM_DIMMING_LIGHT_1);		
	if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	ret	|= item2ctrl(mapping_ITEM_DIMMING_LIGHT_2);		
	
	if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	ret	|= item2ctrl(mapping_ITEM_BATCH_LIGHT_OFF);
	
	return ret;
}

uint8_t	GET_Light_ON_Count(void)		// �����ִ� ���� ����
{
	uint8_t	ret	= 0;
	
	if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))				ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_2)))				ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_1)))			ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_2)))			ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_3)))			ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_4)))			ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_5)))			ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_6))) 			ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	ret++;
	
	return ret;
}

uint8_t	GET_Electricity_State(void)		// ����
{
	uint8_t	ret	= 0;
	
	if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1)))		ret	|= item2ctrl(mapping_ITEM_ELECTRICITY_1);
	if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2)))		ret	|= item2ctrl(mapping_ITEM_ELECTRICITY_2);
	
	return ret;
}

uint8_t GET_Electricity_Count(void)
{
	uint8_t ret = 0;

	if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1)))		ret++;
	if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2)))		ret++;
	
	return ret;
}

void ALL_Light_Switch_Ctrl(uint8_t ctrl, uint8_t Flag, uint8_t DelayOFF_Flag)
{
	uint8_t	touch_switch;
	uint8_t	item;
	
	if(pG_Config->Enable_Flag.PWM_Dimming)
	{
		for(item=mapping_ITEM_LIGHT_1; item < mapping_ITEM_MAX; item++)
		{
			if(item2ctrl(item) & ctrl)
			{
				touch_switch	= item2tsn(item);
				if(Flag == INVERSE)
				{
					Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
				}
				
				if(pG_State->ETC.DelayOFF && DelayOFF_Flag == DELAY_OFF_CHECK)
				{
					if(GET_Light_ON_Count() == 1 && GET_Switch_State(touch_switch))	// ������� ���� �ְ� ������ ���̸�
					{
						if(Flag == OFF)
						{
							SET_LED_State(touch_switch, LED_FLASHING);
							SET_SWITCH_Delay_OFF_Tmr(item, LIGHT_DELAY_TIME);
							continue;
						}
					}
					//SET_SWITCH_Delay_OFF_Flag(item, 0);
				}
				
				SET_Switch_State(touch_switch, Flag);
				if(Flag == ON)
				{
					//if(GET_SWITCH_Delay_OFF_Flag(item))
					{
						SET_SWITCH_Delay_OFF_Flag(item, 0);
					}
				}
				
				SET_LED_State(touch_switch, Flag);
		
				if(item2ctrl(item) & (uint8_t)(CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2) )
				{
					PUT_RelayCtrl(item2ctrl(item), Flag);		//210708 ������ ��ü����� ������ ���� �����ϵ���
					PUT_PWMCtrl(item2ctrl(item), Flag);
				}
				else
				{
					PUT_RelayCtrl(item2ctrl(item), Flag);
				}
			}
		}
	}
	else
	{
		for(item=mapping_ITEM_MAX-1; item > mapping_ITEM_DISABLE; item--)	// 20200709	����� ��û���� ��� ������(���̻�, ������), ù��° �� �����ҵ�
		{
			if(item2ctrl(item) & ctrl)
			{
				touch_switch	= item2tsn(item);
				if(Flag == INVERSE)
				{
					Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
				}
				
				if(pG_State->ETC.DelayOFF && DelayOFF_Flag == DELAY_OFF_CHECK)
				{
					if(GET_Light_ON_Count() == 1 && GET_Switch_State(touch_switch))	// ������� ���� �ְ� ������ ���̸�
					{
						if(Flag == OFF)
						{
							SET_LED_State(touch_switch, LED_FLASHING);
							SET_SWITCH_Delay_OFF_Tmr(item, LIGHT_DELAY_TIME);
							continue;
						}
					}
					//SET_SWITCH_Delay_OFF_Flag(item, 0);
				}
				
				SET_Switch_State(touch_switch, Flag);
				if(Flag == ON)
				{
					//if(GET_SWITCH_Delay_OFF_Flag(item))
					{
						SET_SWITCH_Delay_OFF_Flag(item, 0);
					}
				}
				
				SET_LED_State(touch_switch, Flag);
		
				if(item2ctrl(item) & (uint8_t)(CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2) )
				{
					PUT_RelayCtrl(item2ctrl(item), Flag);		//210708 ������ ��ü����� ������ ���� �����ϵ���
					PUT_PWMCtrl(item2ctrl(item), Flag);
				}
				else
				{
					PUT_RelayCtrl(item2ctrl(item), Flag);
				}
			}
		}
	}
}

void ALL_Electricity_Switch_Ctrl(uint8_t ctrl, uint8_t Flag)
{
	uint8_t	touch_switch;
	uint8_t	item;
	
	if(ctrl & CONTROL_BIT_RELAY_LATCH_1)
	{
		item	= ctrl2item(CONTROL_BIT_RELAY_LATCH_1);
		if(item)
		{
			touch_switch	= item2tsn(item);
			SET_Switch_State(touch_switch, Flag);
			SET_LED_State(touch_switch, Flag);
			PUT_RelayCtrl(item2ctrl(item), Flag);
		}
	}
	if(ctrl & CONTROL_BIT_RELAY_LATCH_2)
	{
		item	= ctrl2item(CONTROL_BIT_RELAY_LATCH_2);
		if(item)
		{
			touch_switch	= item2tsn(item);
			SET_Switch_State(touch_switch, Flag);
			SET_LED_State(touch_switch, Flag);
			PUT_RelayCtrl(item2ctrl(item), Flag);
		}
	}
}
//--------------------------------------------------------------------------------------------------------------

void Touch_Evente_Control(uint8_t touch_switch, uint8_t tmr)
{
	uint8_t Flag;
	uint8_t item;
	Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
	item	= tsn2item(touch_switch);
	if(GET_SWITCH_Delay_OFF_Flag(item))
	{
		if(item == mapping_ITEM_BATCH_LIGHT_OFF)
		{ 
			Flag = OFF;									// �ϰ��ҵ��� �������� ����߿� �ٽ� ��� �ϸ� ������ �ƴ� ��� ����
		}
		else if(item == mapping_ITEM_BATCH_LIGHT_n_GAS)	//Flag ���°� ��ȭ�� �ʾƼ� �߰���
		{
			;
		}
		else
		{
			Flag = ON;									// �������� ������̸� ������� ����
		}
	} 
	if(Flag)
	{
		if(pG_Config->Mapping_TSN_OnTmr[touch_switch-1] <= tmr)
		{
			//Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 0;
			//Gu8_PowerSaving_Tmr	= POWER_SAVING_TMR;
			Gu8_ElecLimitCurrent_1_Tmr	= ELEC_LIMIT_CURRENT_TIME;		// 120sec
			Gu8_ElecLimitCurrent_2_Tmr	= ELEC_LIMIT_CURRENT_TIME;		// 120sec
			EventCtrl(touch_switch, ON);
			SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);		// ����ġ�� ��� ������ ������ �ݺ� ������ �ϰԵǴµ� �̸� �����ϱ� ����
#ifdef _KOCOM_PROTOCOL_							
			if(Gu8_PowerSaving_Tmr != 0)								//KOCOM �������ݿ��� ��ġ �̺�Ʈ �߻��� ���е�� ����.
			{
				if(tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_1 || tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_2 || tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_ALL)
				{
					if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)) == 0)	//���� ������尡 �ƴ� ����
					{
						Gu8_Elec_Touch_Flag = 1;		//������������ ���� ��ġ �÷���
					}
				}
				else
				{
					Gu8_Light_n_ETC_Touch_Flag = 1;	//������������ ���� �� �� �� ��ġ �÷���
				}
				
			}
// #ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
			if(item2tsn(mapping_ITEM_3WAY_1))
			{
				if(item == mapping_ITEM_3WAY_1 || item == mapping_ITEM_LIGHT_GROUP || item == mapping_ITEM_LIGHT_ALL)
				{
					Gu8_3Way_Control_Flag = 1;
				}
				else
				{
					Gu8_3Way_Control_Flag = 0;
				}
			}
// #endif
#endif						
		}
	}
	else
	{
		if(pG_Config->Mapping_TSN_OffTmr[touch_switch-1] <= tmr)
		{
			//Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 0;
			//Gu8_PowerSaving_Tmr	= POWER_SAVING_TMR;
			Gu8_ElecLimitCurrent_1_Tmr	= ELEC_LIMIT_CURRENT_TIME;		// 120sec
			Gu8_ElecLimitCurrent_2_Tmr	= ELEC_LIMIT_CURRENT_TIME;		// 120sec
			EventCtrl(touch_switch, OFF);
			SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);		// ����ġ�� ��� ������ ������ �ݺ� ������ �ϰԵǴµ� �̸� �����ϱ� ����
#ifdef _KOCOM_PROTOCOL_							
			if(Gu8_PowerSaving_Tmr != 0)								//KOCOM �������ݿ��� ��ġ �̺�Ʈ �߻��� ���е�� ����.
			{
				if(tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_1 || tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_2 || tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_ALL)
				{
					if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)) == 0)	//���� ������尡 �ƴ� ����
					{
						Gu8_Elec_Touch_Flag = 1;		//������������ ���� ��ġ �÷���
					}
				}
				else
				{
					Gu8_Light_n_ETC_Touch_Flag = 1;
				}
			}
// #ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
			if(item2tsn(mapping_ITEM_3WAY_1))
			{
				if(item == mapping_ITEM_3WAY_1 || item == mapping_ITEM_LIGHT_GROUP || item == mapping_ITEM_LIGHT_ALL)
				{
					Gu8_3Way_Control_Flag = 1;
				}
				else
				{
					Gu8_3Way_Control_Flag = 0;
				}
			}
// #endif
#endif			
		}
	}
}

void Group_Light_Setup(uint8_t touch_switch, uint8_t tmr)
{
	uint8_t	i;
	
	switch(tsn2item(touch_switch))
	{
		case mapping_ITEM_LIGHT_GROUP:
			if(GET_Touch_Ignore_Flag(touch_switch) != SET_SPECIAL_SWITCH)
			{
				SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);
				pG_State->User_Mapping_ALL_Light	= GET_Light_State();
				Beep(OFF);
				Gu8_LightGroup_SET_Flag	= 0;		// ���� �Ϸ�
				if(GET_Switch_State(touch_switch))
				{
					SET_LED_State(touch_switch, LED_ON);
				}
				else
				{
					SET_LED_State(touch_switch, LED_OFF);
				}
				// SET_LED_State(touch_switch, ON);		//LED OFF
				// SET_Switch_State(touch_switch, ON);
			}
			break;
		case mapping_ITEM_LIGHT_1:
		case mapping_ITEM_LIGHT_2:
		case mapping_ITEM_LIGHT_3:
		case mapping_ITEM_LIGHT_4:
		case mapping_ITEM_LIGHT_5:
		case mapping_ITEM_LIGHT_6:
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			Touch_Evente_Control(touch_switch, tmr);	// ����ġ ���۽ð� üũ �� ����
			break;
	}
}

void Light_Sleep_Process(void)
{
	uint8_t touch_switch, item = 0;
	if(item2tsn(mapping_ITEM_SLEEP))
	{
		if(Gu16_Light_Sleep_tmr == 0)
		{
			if(Gu8_Sleep_Flag)
			{
				for(item = mapping_ITEM_LIGHT_1; item <= mapping_ITEM_LIGHT_2; item++)
				{
					touch_switch = item2tsn(item);
					if(GET_Switch_State(touch_switch))
					{
						SET_Switch_State(touch_switch, OFF);
						PUT_RelayCtrl(item2ctrl(item), OFF);
						SET_LED_State(touch_switch, OFF);
					}
				}
				Gu8_Sleep_Flag = 0;
				SET_Switch_State(item2tsn(mapping_ITEM_SLEEP), OFF);
				SET_LED_State(item2tsn(mapping_ITEM_SLEEP), ON);
			}
			else
			{
				if((GET_LED_State(item2tsn(mapping_ITEM_SLEEP)) == LED_FLASHING) && (Gu8_Sleep_Set_Tmr == 0))	//���� ������� �������̰�, ���� �ð��� 0�� �Ǹ�
				{
					EventCtrl(item2tsn(mapping_ITEM_SLEEP), OFF);
				}
			}
		}
	}
}
void Special_Function_Key_Process(uint8_t touch_switch, uint8_t tmr)
{
	// static uint8_t cnt = 0;

	if(touch_switch && touch_switch < mapping_SWITCH_MAX)
	{
		SET_Touch_Ignore_Flag(touch_switch, SET_SPECIAL_SWITCH);

		if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))	// ����Ű�� ���� ���̸�
		{
			if(SETTING_1S_TMR <= tmr)		// 1sec
			{
				switch(tsn2item(touch_switch))
				{
					case mapping_ITEM_ELECTRICITY_1:
						if(((double)Gu16_LCD_Watt_1 * 0.8) < 0.0)	Gu16_ElecLimitCurrent_1	= 0;
						else										Gu16_ElecLimitCurrent_1	= (uint16_t)((double)Gu16_LCD_Watt_1 * 0.8);	// ���� ���� 80%�� ����
						pG_State->ETC.Auto1		= 1;
						Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 1;
						Gu8_LCD_ElecLimitCurrent_Tmr			= 0;
						Store_ElecLimitCurrent();
						Beep(BEEP_MEL);
						//Beep(BEEP_TWO);
						break;
						
					case mapping_ITEM_ELECTRICITY_2:
						if(((double)Gu16_LCD_Watt_2 * 0.8) < 0.0)	Gu16_ElecLimitCurrent_2	= 0;
						else										Gu16_ElecLimitCurrent_2	= (uint16_t)((double)Gu16_LCD_Watt_2 * 0.8);	// ���� ���� 80%�� ����
						pG_State->ETC.Auto2		= 1;
						Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 11;
						Gu8_LCD_ElecLimitCurrent_Tmr			= 0;
						Store_ElecLimitCurrent();
						Beep(BEEP_MEL);
						break;
				}
			}
		}
		else												// ����Ű �������� �ƴϸ�
		{
#if 0
			if(SETTING_10S_TMR <= tmr)						// 10sec �̻� ��������
			{
				;
			}
			else if(SETTING_5S_TMR <= tmr)					// 5sec �̻� ��������
			{
				/*	20200709	����� ��û���� ��� ������(���̻�, ������)
				switch(tsn2item(touch_switch))
				{
					case mapping_ITEM_LIGHT_1:
					case mapping_ITEM_BATCH_LIGHT_OFF:		// �ϰ��ҵ�
						
						if(pG_State->ETC.BeepMode)
						{
							Beep(BEEP_MEL);
							pG_State->ETC.BeepMode	= 0;
						}
						else
						{
							pG_State->ETC.BeepMode	= 1;
							Beep(BEEP_MEL);
						}
						//printf("beep mode %d\n", (uint16_t)pG_State->ETC.BeepMode);
						break;
				}
				*/
			}
#endif
			if(SETTING_2S_TMR <= tmr)					// 2sec �̻� ��������
			{
				switch(tsn2item(touch_switch))
				{
					/*	20200709	����� ��û���� ��� ������(���̻�, ������)
					case mapping_ITEM_LIGHT_1:
					case mapping_ITEM_BATCH_LIGHT_OFF:		// �ϰ��ҵ�
						Beep(BEEP_MEL);
						if(pG_State->ETC.DelayOFF)	pG_State->ETC.DelayOFF	= 0;
						else						pG_State->ETC.DelayOFF	= 1;
						//printf("delay off %d\n", (uint16_t)pG_State->ETC.DelayOFF);
						break;
					*/
					case mapping_ITEM_LIGHT_GROUP:
						Beep(ON);
						Gu8_LightGroup_SET_Flag	= 1;
						if(GET_Switch_State(touch_switch) == 0)				// �׷������� ���� ������
						{
							Touch_Evente_Control(touch_switch, tmr);		// ����ġ ����
						}
						SET_LED_State(touch_switch, LED_FLASHING);			//��ġ�� if�� ������ �Ʒ��� �����Ͽ� �׷� ������ �÷��� ���� �ذ�
						break;
#if 0
					case mapping_ITEM_LIGHT_1:
						if(SETTING_5S_TMR <= tmr)
						{
							// Touch_Evente_Control(touch_switch, tmr);
							// EventCtrl(touch_switch, INVERSE);	//���� �� ����ġ ���� �ٲ�����Ƿ� �ٽ� ������ ���� ���.
							SET_Defaultdata();
							while(1);
						}
						break;
#endif
				}
			}
		}
	}
	Gu8_Special_Function_Key		= 0;
	Gu8_Special_Function_Key_Tmr	= 0;
}
#if defined (_ONE_SIZE_LIGHT_1_n_SLEEP_) || defined (_ONE_SIZE_LIGHT_2_n_SLEEP_)
void Sleep_Level(uint8_t touch_switch)
{
	switch(tsn2item(touch_switch))
	{
		case mapping_ITEM_SLEEP:
			if((GET_Switch_State(touch_switch)) && (GET_LED_State(touch_switch) == LED_FLASHING))
			{
				switch(Gu8_Sleep_cnt)
				{
					case 0:
						Gu8_Sleep_Flag = 1;
						Gu16_Light_Sleep_tmr = LIGHT_SLEEP_30MINUTES;		//10s
						Gu8_Sleep_cnt++;
						SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);
						Beep(BEEP_30MINUTES);
						printf("30min later\r\n");
						break;
					case 1:
						Gu8_Sleep_Flag = 1;
						Gu16_Light_Sleep_tmr = LIGHT_SLEEP_60MINUTES;		//10s
						Gu8_Sleep_cnt++;
						SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);
						Beep(BEEP_60MINUTES);
						printf("60min later\r\n");
						break;
					case 2:
						Gu8_Sleep_Flag = 1;
						Gu16_Light_Sleep_tmr = LIGHT_SLEEP_90MINUTES;
						Gu8_Sleep_cnt++;
						SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);
						Beep(BEEP_90MINUTES);
						printf("90min later\r\n");
						break;
					case 3:
						Gu8_Sleep_Flag = 0;
						Gu8_Sleep_cnt = 0;
						Gu16_Light_Sleep_tmr = 0;
						// EventCtrl(touch_switch, OFF);
						SET_Switch_State(touch_switch, OFF);
						SET_LED_State(touch_switch, ON);
						SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);
						Beep(OFF);
						printf("TIMER INIT\r\n");
						break;
					default:
						Gu8_Sleep_Flag = 0;
						Gu8_Sleep_cnt = 0;
						Beep(OFF);
						printf("default\r\n");
						break;
				}
			}
			break;
	}
}
#endif

/*void Sleep_Evente_Control(uint8_t touch_switch, uint8_t tmr)
{
	uint8_t Flag;
	uint8_t item;
	
	Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
	item	= tsn2item(touch_switch);

	if(Flag)
	{
		if(pG_Config->Mapping_TSN_OnTmr[touch_switch-1] <= tmr)
		{
			// EventCtrl(touch_switch, ON);
			SET_Switch_State(touch_switch, ON);
			SET_LED_State(touch_switch, LED_FLASHING);
			SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);
		}
	}
	else
	{
		if(pG_Config->Mapping_TSN_OffTmr[touch_switch-1] <= tmr)
		{
			// EventCtrl(touch_switch, OFF);
			SET_Switch_State(touch_switch, OFF);
			SET_LED_State(touch_switch, ON);
			SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);		// ����ġ�� ��� ������ ������ �ݺ� ������ �ϰԵǴµ� �̸� �����ϱ� ����
		}

	}
}*/

void TouchSwitch_Action_Check(uint8_t touch_switch, uint8_t tmr)
{
	static uint16_t cnt = 0;
	if(TouchConfig.MAX_TouchChip >= 1)	// GT308L
	{
		if(touch_switch && touch_switch < mapping_SWITCH_MAX)
		{
		//   printf("touch switch %d\r\n", (uint16_t)touch_switch);
			Touch_Error(touch_switch, tmr, 0);
			switch(pG_State->ETC.LED_Mode)
			{
				case LED_OFF__LIGHT_IS_ON:
					if(GET_Touch_Ignore_Flag(touch_switch) == IGNORE_CLEAR)				// ����ġ ���ð� �ƴϸ�
					{
#if defined (_ONE_SIZE_LIGHT_1_n_SLEEP_) || defined (_ONE_SIZE_LIGHT_2_n_SLEEP_)
						if(GET_Switch_State(item2tsn(mapping_ITEM_SLEEP)))
						{
							switch(tsn2item(touch_switch))
							{
								case mapping_ITEM_SLEEP:
									Sleep_Level(touch_switch);
									break;
							}
						}
#else
						;
#endif
					}
					else if(GET_Touch_Ignore_Flag(touch_switch) != SET_SPECIAL_SWITCH)	// Ư��Ű ���ð� �ƴϸ�
					{
						if(SETTING_10S_TMR <= tmr)									// 10sec �̻� ��������
						{
							memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));	// ��ġĨ �ʱ�ȭ
						}
					}
					else if(GET_Touch_Ignore_Flag(touch_switch) == SET_SPECIAL_SWITCH)		// Ư��Ű ���� 20201113
					{
						if(SETTING_10S_TMR <= tmr)
						{
							memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
						}
					}		//20201113

					/*else
					{
						printf("LED_MODE1 != IGNORE_CLEAR\r\n");
						if(GET_Touch_Ignore_Flag(touch_switch) != SET_SPECIAL_SWITCH)
						{
							printf("LED_MODE1 != SET_SPECIAL_SWITCH\r\n");
						}
						
						else if(GET_Touch_Ignore_Flag(touch_switch) == SET_SPECIAL_SWITCH)
						{
							printf("LED_MODE1 == SET_SPECIAL_SWITCH\r\n");
						}
						
						if(SETTING_10S_TMR <= tmr)
						{
							memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
						}
					}*/

					if(Gu8_PowerSaving_Tmr == 0)	// ������ ������ LED OFF, ������ LED ON	 and ������忴�� ��� 
					{
						Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;								// 5sec ���� �� wakeup (��ġ�� ���� ���۾���)
						SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);
						break;
					}
				case LED_OFF__LIGHT_IS_ON_2:
				default:
					Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
					Gu8_INFO_Disp_Tmr			= 0xFF;									// ���� ���� �� �ѹ��̶� ��ġ�� ������ LCD ������� ����
					if(GET_Touch_Ignore_Flag(touch_switch) == IGNORE_CLEAR)				// ����ġ ���ð� �ƴϸ�
					{
						if(Gu8_LightGroup_SET_Flag)										// �׷����� ���� ����
						{
							Group_Light_Setup(touch_switch, tmr);
						}
						else if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))			// ���� ���� ����
						{
							switch(tsn2item(touch_switch))
							{
								case mapping_ITEM_ELECTRICITY_1:
								case mapping_ITEM_ELECTRICITY_2:
									Touch_Evente_Control(touch_switch, 255);			// OFF �ð��� ������� ��� ����
									break;
								default:
									Touch_Evente_Control(touch_switch, tmr);			// ���� �ܴ̿� ����ġ ���۽ð� üũ �� ����
									break;
							}
						}
#if defined(_ONE_SIZE_LIGHT_1_n_SLEEP_) || defined(_ONE_SIZE_LIGHT_2_n_SLEEP_)			
						else if(GET_LED_State(item2tsn(mapping_ITEM_SLEEP)) == LED_FLASHING)
						{
							switch(tsn2item(touch_switch))
							{
								case mapping_ITEM_SLEEP:
									Sleep_Level(touch_switch);
									break;
								default:
									Touch_Evente_Control(touch_switch, tmr);
									break;
							}
						}
#endif						
						else															// �Ϲݵ���
						{
							Touch_Evente_Control(touch_switch, tmr);					// ����ġ ���۽ð� üũ �� ����				
						}
					}
					else if(GET_Touch_Ignore_Flag(touch_switch) != SET_SPECIAL_SWITCH)	// Ư��Ű ���ð� �ƴϸ�
					{
						Gu8_Special_Function_Key		= touch_switch;
						Gu8_Special_Function_Key_Tmr	= tmr;
						if(SETTING_10S_TMR <= tmr)						// 10sec �̻� ��������
						{
							memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));	// ��ġĨ �ʱ�ȭ
						}
						else if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))		// ����Ű�� ���� ���̸�
						{
							if(SETTING_1S_TMR <= Gu8_Special_Function_Key_Tmr)	// ������ �� �ִ� �ִ�ð��� ������ Ư��Ű ��ý���
							{
								Special_Function_Key_Process(Gu8_Special_Function_Key, Gu8_Special_Function_Key_Tmr);		// ��ý����ϱ� ����
							}
						}
						else
						{
							//if(SETTING_10S_TMR <= Gu8_Special_Function_Key_Tmr)	// ������ �� �ִ� �ִ�ð��� ������ Ư��Ű ��ý���
							if(SETTING_2S_TMR <= Gu8_Special_Function_Key_Tmr)	// ������ �� �ִ� �ִ�ð��� ������ Ư��Ű ��ý���
							{
								Special_Function_Key_Process(Gu8_Special_Function_Key, Gu8_Special_Function_Key_Tmr);
							}
						}
					}
					else if(GET_Touch_Ignore_Flag(touch_switch) == SET_SPECIAL_SWITCH)
					{
						if(SETTING_10S_TMR <= tmr)
						{
							memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
						}
#if 0
						else if(SETTING_5S_TMR <= tmr)
						{
							Special_Function_Key_Process(touch_switch, tmr);
						}
#endif
					}

					/*else
					{
						printf("LED_MODE2 != IGNORE CLEAR\r\n");
						if(GET_Touch_Ignore_Flag(touch_switch) != SET_SPECIAL_SWITCH)
						{
							printf("LED_MODE2 != SET_SPECIAL_SWITCH\r\n");
							Gu8_Special_Function_Key		= touch_switch;
							Gu8_Special_Function_Key_Tmr	= tmr;
						
							if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))
							{
								if(SETTING_1S_TMR <= Gu8_Special_Function_Key_Tmr)
								{
									Special_Function_Key_Process(Gu8_Special_Function_Key, Gu8_Special_Function_Key_Tmr);
								}
							}
							else
							{
								if(SETTING_10S_TMR <= Gu8_Special_Function_Key_Tmr)
								{
									Special_Function_Key_Process(Gu8_Special_Function_Key, Gu8_Special_Function_Key_Tmr);
								}
							}
						}
						else if(GET_Touch_Ignore_Flag(touch_switch) == SET_SPECIAL_SWITCH)
						{
							printf("LED_MODE2 == SET_SPECIAL_SWITCH\r\n");
						}
						if(SETTING_10S_TMR <= tmr)
						{
							memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
						}
					}*/

					break;
			}
		}
	}
}
//---------------------------------------------------------------------------------------------------------------
extern const uint8_t *str_mapping[mapping_ITEM_MAX];
void Control_Recovery_Init(void)
{
	uint8_t i;
	uint8_t flag = 0;
	
	printf("Control Recovery Init\n");
	
	// ��ü, �׷� ����ġ�� ���� �ִ��� Ȯ��
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		if(GET_Switch_State(i))				// ���� �־����� Ȯ��
		{
			switch(tsn2item(i))
			{
				case mapping_ITEM_LIGHT_ALL:		// ���� ALL ����ġ
					flag	|= 0x01;
					break;			
				case mapping_ITEM_ELECTRICITY_ALL:	// ���� ALL ����ġ
					flag	|= 0x02;				// ��ü������ ���� �־����� Ȯ��
					break;
				case mapping_ITEM_LIGHT_GROUP:			// ���� GROUP ����ġ
					flag	|= 0x04;				// �׷� ������ ���� �־����� Ȯ��
					break;
			}
		}
	}
	
	// ��ü, �׷� ����ġ�� ���� ������ �׿� �ش��ϴ� ������ OFF�� ����
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		switch(tsn2item(i))
		{
			case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
			case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
			case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
			case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
			case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
			case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
			case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
			case mapping_ITEM_3WAY_1:				// 3�� 1 (������ ����) case mapping_ITEM_ELEVATOR �Ʒ��� �־��µ�, ���� �� ���� ���� ���� �ʾ� �ű�.
			case mapping_ITEM_3WAY_2:				// 3�� 2 (������ ����)
				if(flag & 0x01)			// ��ü���� ���� ������
				{
					SET_Switch_State(i, OFF);		// ��ü���� ������ �Ϲ� �� ����ġ ���� Ŭ����
				}
				else if(flag & 0x04)	// �׷���� ���� ������
				{
					SET_Switch_State(i, OFF);		// �׷���� ������ �Ϲ� �� ����ġ ���� Ŭ����
				}
				break;
			case mapping_ITEM_ELECTRICITY_1:		// ���� 1 (������ ����)
			case mapping_ITEM_ELECTRICITY_2:		// ���� 2 (������ ����)
				if(flag & 0x02)			// ��ü������ ���� ������
				{
					SET_Switch_State(i, OFF);		// ��ü���� ������ �Ϲ� �� ����ġ ���� Ŭ����
				}
				break;

			//case mapping_ITEM_GAS:					// ��������
			//case mapping_ITEM_ELEVATOR:				

			case mapping_ITEM_ELECTRICITY_ALL:		// ���� ALL ����ġ
			case mapping_ITEM_BATCH_LIGHT_OFF:
			case mapping_ITEM_BATCH_LIGHT_n_GAS:
			case mapping_ITEM_BATCH_LIGHT_n_COOK:
			case mapping_ITEM_LIGHT_ALL:			// ���� ALL ����ġ
			case mapping_ITEM_LIGHT_GROUP:			// ���� GROUP ����ġ
				break;
			default:
				SET_Switch_State(i, OFF);			// �Ϲ� ����ġ ���´� Ŭ����
				break;
		}
	}
	/*
	printf("\n\n");
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		if(GET_Switch_State(i))	printf("%d\n", (uint16_t)i);
	}
	printf("\n\n");
	*/
}

void Control_Recovery(void)
{
	uint8_t i, item;
	
	// ��ü, �׷��� ���� ������ ������, ���������� �����ϰ�,	��ü, �׷��� ���� ������ ��ü, �׷� ����
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		item = tsn2item(i);
		if(GET_Switch_State(i) != 0)	//����ġ ���°� ON�̸�
		{
			switch(item)
			{
				case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
				case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
				case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
				case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
				case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
				case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
				case mapping_ITEM_ELECTRICITY_1:		// ���� 1 (������ ����)
				case mapping_ITEM_ELECTRICITY_2:		// ���� 2 (������ ����)

				case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
				case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)	
				case mapping_ITEM_BATCH_LIGHT_OFF:
				case mapping_ITEM_BATCH_LIGHT_n_GAS:
				case mapping_ITEM_BATCH_LIGHT_n_COOK:
				case mapping_ITEM_ELECTRICITY_ALL:		// ���� ALL ����ġ				
					// EventCtrl(i, ON);				// ����� �����ͷ� ����, EventCtrl�� �����ϸ� ������ �����Ǹ鼭 �׷� ����ġ�� ��ü���� ����ġ�� LED ���°� �޶����� �Ʒ��� ���� LED�� ������.
					// SET_LED_State(i, ON);				// ����� �����ͷ� ����
					// break;
				case mapping_ITEM_LIGHT_GROUP:			// ���� GROUP ����ġ					
				case mapping_ITEM_LIGHT_ALL:			// ���� ALL ����ġ
					EventCtrl(i, ON);
					break;
				case mapping_ITEM_3WAY_1:				// 3�� 1 (������ ����)
				case mapping_ITEM_3WAY_2:				// 3�� 2 (������ ����)
#if defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) && defined(WIRING_THREEWAY)	//�ϰ�����ġ ���̰� ��� ������ ���� ���¸� �𸣴� ��쿡�� LED OFF�� ����
					// printf("--1 switch state %d\r\n", (uint16_t)GET_Switch_State(i));
					SET_LED_State(i, ON);										//���� ���ۿ����� �ش� ��ư ���� �� LED ������
					PUT_RelayCtrl(item2ctrl(tsn2item(i)), ON);
#else
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
					SET_LED_State(i, ON);
#else
					EventCtrl(i, ON);
#endif			
#endif
					break;
			}
		}
		else
		{
			switch(item)
			{
				case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
				case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
				case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
				case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
				case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
				case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)

				case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)		//��� ���� ���� ������ ��, ������ ����ġ ���¿� ���� �ʾƼ� �߰���.
				case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)		//��� ���� ���� ������ ��, ������ ����ġ ���¿� ���� �ʾƼ� �߰���.		
				case mapping_ITEM_ELECTRICITY_1:		// ���� 1 (������ ����)
				case mapping_ITEM_ELECTRICITY_2:		// ���� 2 (������ ����)
				case mapping_ITEM_BATCH_LIGHT_OFF:
				case mapping_ITEM_BATCH_LIGHT_n_GAS:
				case mapping_ITEM_BATCH_LIGHT_n_COOK:
					SET_LED_State(i, OFF);
					SET_SWITCH_Delay_OFF_Tmr(item, 0);	// ������ ������ ���
					break;
				case mapping_ITEM_3WAY_1:				// 3�� 1 (������ ����)
				case mapping_ITEM_3WAY_2:				// 3�� 2 (������ ����)
#if defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) && defined(WIRING_THREEWAY)	//�ϰ�����ġ ���̰� ��� ������ ���� ���¸� �𸣴� ��쿡�� LED OFF�� ����
					// printf("--0 switch state %d\r\n", (uint16_t)GET_Switch_State(i));
					SET_LED_State(i, OFF);
#elif defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) && defined(_HYUNDAI_PROTOCOL_) && defined(COMM_THREEWAY)
					SET_LED_State(i, ON);	//��������� ��� 3���� ��� ���� �� ���� ������. �׷��� LED OFF��.
#else
					SET_LED_State(i, OFF);
					// SET_SWITCH_Delay_OFF_Tmr(item, 0);	// ������ ������ ���
#endif
					break;
				case mapping_ITEM_GAS:					// ��������
				case mapping_ITEM_COOK:
				case mapping_ITEM_GAS_n_COOK:
				case mapping_ITEM_ELEVATOR:				// ���������� ȣ���� �������� ����(����Ʈ ��ü�� ���� �� �����Ǿ��� �� ��� ���밡 ȣ��...)
				case mapping_ITEM_SLEEP:
					SET_LED_State(i, ON);
					break;
			}
		}
	}	
}

void Touch_Error(uint8_t touch_switch, uint8_t tmr, uint8_t Flag)		//20210524 Flag�� 1�̸�, EventCtrl, Flag 0�̸�, ActionCheck
{
	static uint16_t cnt = 0;
	static uint8_t old_touch_switch;
	if(tsn2item(touch_switch) != mapping_ITEM_LIGHT_GROUP)
	{
		if((Flag == 0) && (Gu8_Touch_Err_Flag == 0) && (Gu16_Touch_Err_Tmr == 0))		//��ġ ���� üũ. (��ġ 2���̻� ������ ��� 120�� ���� ��ġ���� Ȯ��)
		{
			if((tmr >= 20) && (tmr < 90))		//2�� �̻�
			{
				Gu16_Touch_Err_Tmr = 120;
				Gu8_Touch_Err_Flag = 1;
				cnt = 0;
				printf("Temp_Tmr = 120\r\n");
			}
		}
	}
	else		//GROUP Switch ��ġ�� ���
	{
		if((Flag == 0) && (Gu8_Touch_Err_Flag == 0) && (Gu16_Touch_Err_Tmr == 0))		//��ġ ���� üũ. (��ġ 2���̻� ������ ��� 120�� ���� ��ġ���� Ȯ��)
		{
			if((tmr >= 30) && (tmr < 90))		//3�� �̻�
			{
				Gu16_Touch_Err_Tmr = 120;
				Gu8_Touch_Err_Flag = 1;
				cnt = 0;
				// printf("Temp_Tmr = 120\r\n");
			}
		}		
	}

	if((Flag == 0) && (Gu8_Touch_Err_Flag == 1))
	{
		if(cnt % 200 == 0)
		{
			// printf("touch switch = %d, cnt = %d, tmr = %d\r\n", (uint16_t)touch_switch, cnt, (uint16_t)tmr);
			if(Gu16_Touch_Err_Tmr)
			{
				if(cnt >= 30000)		//���� üũ ���� �� 120�ʵ��� ��ġ�� ���� �̻� cnt�� �� ��� ��ġĨ �ʱ�ȭ.
				{
					memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
					Gu16_Touch_Err_Tmr = 5;
					Gu8_Touch_Err_Flag = 0;
					Gu8_Touch_Err_Cnt = 0;					
					cnt = 0;
				}
			}
		}
		if((tmr >= 99))					//���� üũ ���� �� �ϳ��� ��ġ ����ġ�� 10�� �̻� ��ġ �� ��� ��ġĨ �ʱ�ȭ�� �ǹǷ� ������ ������ �ʱ�ȭ.
		{
			printf("CLEAR\r\n");
			Gu16_Touch_Err_Tmr = 5;
			Gu8_Touch_Err_Flag = 0;
			// Flag = 0;
			Gu8_Touch_Err_Cnt = 0;
			cnt = 0;
			// tmr = 0;		//��ġ 10�� �̻�����  �ʱ�ȭ �� �� �ٷ� ��ġ ���� üũ�� ���� ��� ����.
		}
	}
	else if((Flag == 1) && (Gu8_Touch_Err_Flag == 1))		//EventCtrl�� ���� ���۰� ���� ������ ���� ��� ī��Ʈ.
	{
		if((Gu16_Touch_Err_Tmr))
		{
			if(touch_switch == old_touch_switch)	Gu8_Touch_Err_Cnt++;
			printf("Cnt = %d, Tmr = %d\r\n", (uint16_t)Gu8_Touch_Err_Cnt, Gu16_Touch_Err_Tmr);
			printf("old = %d, pre = %d\r\n", (uint16_t)old_touch_switch, (uint16_t)touch_switch);
		}
		else
		{
			// printf("Temp Tmr, Cnt reset\r\n");			//120�� Ÿ�̸Ӱ� 0�� �Ǹ� �� ������ �ʱ�ȭ
			// Gu8_Touch_Err_Flag = 0;
			// Flag = 0;
			// Gu8_Touch_Err_Cnt = 0;
			// cnt = 0;
		}
		if(Gu8_Touch_Err_Cnt >= 10)							//EventCtrl�� ���� ���۰� ���� ������ ���� ��� ī��Ʈ�Ͽ� 10 �̻�Ǹ� ��ġĨ �ʱ�ȭ
		{
			printf("EventCtrl touch reset\r\n");
			memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
			Gu16_Touch_Err_Tmr = 5;
			Gu8_Touch_Err_Flag = 0;
			// Flag = 0;
			Gu8_Touch_Err_Cnt = 0;
			cnt = 0;
		}
		old_touch_switch = touch_switch;
	}
	cnt++;
}

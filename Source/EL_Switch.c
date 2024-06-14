/************************************************************************************
	Project		: 전자식스위치
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

void EventCtrl(uint8_t touch_switch, uint8_t Flag)	// 스위치
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
			case mapping_ITEM_LIGHT_GROUP:			// 그룹스위치
				if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_ALL)))		// 전체 등이 켜져 있으면
				{
					SET_Switch_State(item2tsn(mapping_ITEM_LIGHT_ALL), OFF);
					SET_LED_State(item2tsn(mapping_ITEM_LIGHT_ALL), OFF);	// 전체등 LED 소등
					ALL_Light_Switch_Ctrl((uint8_t)(pG_Config->Factory_Mapping_ALL_Light ^ pG_State->User_Mapping_ALL_Light), OFF, DELAY_OFF_PASS);		// 전체등와 그룹등으로 설정된 전등 중 그룹등이 아닌 것은 소등
				}
				Flag	= SET_Switch_State(touch_switch, Flag);				// Switch 상태 설정하고 Switch 상태로 Flag 재설정
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				if(Flag == OFF)
				{
					ALL_Light_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Light, Flag, DELAY_OFF_PASS);	// 수동으로 켠 등도 끄기위해서
				}
				else
				{
					ALL_Light_Switch_Ctrl(pG_State->User_Mapping_ALL_Light, Flag, DELAY_OFF_PASS);
				}
				break;
			case mapping_ITEM_LIGHT_ALL:		// 전제 등 스위치
				if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_GROUP)))	// 그룹등이 켜져 있으면
				{
					SET_Switch_State(item2tsn(mapping_ITEM_LIGHT_GROUP), OFF);
					SET_LED_State(item2tsn(mapping_ITEM_LIGHT_GROUP), OFF);	// 그룹등 LED 소등
					ALL_Light_Switch_Ctrl((uint8_t)(pG_Config->Factory_Mapping_ALL_Light ^ pG_State->User_Mapping_ALL_Light), OFF, DELAY_OFF_PASS);	// 그룹등와 전체등으로 설정된 전등 중 전체등이 아닌 것은 소등
				}
				Flag	= SET_Switch_State(touch_switch, Flag);				// Switch 상태 설정하고 Switch 상태로 Flag 재설정
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				ALL_Light_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Light, Flag, DELAY_OFF_CHECK);	//지연 소등 사용하려면 사용
				// ALL_Light_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Light, Flag, DELAY_OFF_PASS);	//지연소등 제거하려면 사용
				break;
			case mapping_ITEM_ELECTRICITY_ALL:	// 전체 전열 스위치
				if(Flag == INVERSE)	Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
				
				if(Flag == OFF)
				{
					//Beep(BEEP_FLASHING);
					SET_LED_State(touch_switch, LED_FLASHING);
					SET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_ELECTRICITY_ALL, ELECTRICITY_DELAY_TIME);				// 전체 전열 지연소등 설정
					if(pG_Config->Factory_Mapping_ALL_Electricity & CONTROL_BIT_RELAY_LATCH_1)
					{
						SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_1), LED_FLASHING);
						SET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_ELECTRICITY_1, ELECTRICITY_DELAY_TIME);				// 전열1 지연소등 설정
					}
					if(pG_Config->Factory_Mapping_ALL_Electricity & CONTROL_BIT_RELAY_LATCH_2)
					{
						SET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_2), LED_FLASHING);		// 전열2 지연소등 설정
						SET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_ELECTRICITY_2, ELECTRICITY_DELAY_TIME);
					}
				}
				else
				{
					Flag	= SET_Switch_State(touch_switch, Flag);					// Switch 상태 설정하고 Switch 상태로 Flag 재설정
					SET_LED_State(touch_switch, Flag);
					Beep(Flag);
					//Beep(BEEP_MEL);
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);				// 전체 전열 지연소등 Flag 클리어
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_1, 0);					// 전열1 지연소등 Flag 클리어
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_2, 0);					// 전열2 지연소등 Flag 클리어
					ALL_Electricity_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Electricity, Flag);	// 전체 전열 점등
				}
				break;
			case mapping_ITEM_3WAY_1:
			case mapping_ITEM_3WAY_2:
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_		//일괄스위치에서 삼로 기능 있을 때
#ifdef COMM_THREEWAY
	#ifdef _HYUNDAI_PROTOCOL_		//현대 프로토콜이면, 실제 와이어 연결하는 방식이 아니라 월패드 통신 연동일 경우만 LED 점멸함.
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
	#else		//현대, 씨브이넷 제외 프로토콜
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);	// 항목기준 제어
	#endif
#endif	//COMM_THREEWAY
#ifdef WIRING_THREEWAY
#ifdef _KOCOM_PROTOCOL_
				Flag	= SET_Switch_State(touch_switch, Flag);
				SET_LED_State(touch_switch, LED_DETECT_ON);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);	// 항목기준 제어
				Touch_Use_Tmr = 5;	//500ms
#else
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);	// 항목기준 제어
#endif	//_KOCOM_PROTOCO:_
#endif	//WIRING THREEWAY
#else	//일괄 스위치 아닐 때
	#if defined _NO_PROTOCOL_ && defined THREEWAY_TRANS
				Gu8_Direct_Control = 1;
	#endif
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				PUT_RelayCtrl(item2ctrl(item), Flag);	// 항목기준 제어
				ALL_n_Group_Light_Switch_LED_Ctrl();		//210708 전체등 & 그룹등 LED 제어
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
				/*	// 개별 제어시 지연소등 없다고 함(사업부)
				if(pG_State->ETC.DelayOFF)
				{
					if(GET_Light_ON_Count() == 1)	// 현재 등이 마지막 등이면
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
				// 지연소등 중 다시 눌렀을 때 바로 꺼져야 하면 이 부분 해제할 것
				if(GET_LED_State(touch_switch) == LED_FLASHING)
				{
					SET_SWITCH_Delay_OFF_Flag(item, 0);
					Flag	= OFF;
				}
#endif
				
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
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
				PUT_RelayCtrl(item2ctrl(item), Flag);		// 항목기준 제어
				ALL_n_Group_Light_Switch_LED_Ctrl();		// 전체등 & 그룹등 LED 제어
				break;
			case mapping_ITEM_ELECTRICITY_1:
			case mapping_ITEM_ELECTRICITY_2:
				//printf("mapping_ITEM_SETUP %d\n", GET_Switch_State(item2tsn(mapping_ITEM_SETUP)));
				if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))	// 전열 설정 중이면
				{
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);				// 전체 전열 지연소등 Flag 클리어
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_1, 0);					// 전열1 지연소등 Flag 클리어
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_2, 0);					// 전열2 지연소등 Flag 클리어
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
						Flag	= SET_Switch_State(touch_switch, Flag);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
						SET_LED_State(touch_switch, Flag);
						Beep(Flag);
						//Beep(BEEP_MEL);
						PUT_RelayCtrl(item2ctrl(item), Flag);	// 항목기준 제어
						SET_SWITCH_Delay_OFF_Flag(item, 0);
						SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
						//----------------------------------------------------------------------------------------------------------
						ALL_Electricity_Switch_LED_Ctrl();
						//----------------------------------------------------------------------------------------------------------
#ifdef _HDC_PROTOCOL_	//HDC 프로토콜은 과부하 차단 및 자동 차단일 경우 데이터를 전달하는데 EventCtrl로 제어 할 경우 해당 데이터 초기화
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
				/*	// 개별 제어시 지연소등 없다고 함(사업부)
				if(pG_State->ETC.DelayOFF)
				{
					if(GET_Light_ON_Count() == 1)	// 현재 등이 마지막 등이면
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
				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
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
				PUT_PWMCtrl(item2ctrl(item), Flag);			// 항목기준 제어
				PUT_RelayCtrl(item2ctrl(item), Flag);		// 20210604 디밍전등도 릴레어 제어 가능 하도록 추가
				ALL_n_Group_Light_Switch_LED_Ctrl();		// 210621 디밍전등은 전체등 LED에 영향안받도록
				break;
			case mapping_ITEM_DIMMING_UP:
				Gu8_LCD_DIM_Tmr					= 50;		// 5s 동안 LCD 표시
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
				if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)) && GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)) )	// 디밍 등이 모두 켜져 있으면
				{
					PWM_Level_Set((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), LEVEL_UP);
					PUT_PWMCtrl((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	// 디밍1 등이 켜져 있으면
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_1, LEVEL_UP);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	// 디밍2 등이 켜져 있으면
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_2, LEVEL_UP);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
				}
				break;
			case mapping_ITEM_DIMMING_DN:
				Gu8_LCD_DIM_Tmr					= 50;		// 5s 동안 LCD 표시
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
				if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)) && GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)) )	// 디밍 등이 모두 켜져 있으면
				{
					PWM_Level_Set((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), LEVEL_DN);
					PUT_PWMCtrl((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	// 디밍1 등이 켜져 있으면
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_1, LEVEL_DN);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	// 디밍2 등이 켜져 있으면
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_2, LEVEL_DN);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
				}
				break;
			case mapping_ITEM_COLOR_TEMP_UP:
				/*Gu8_LCD_DIM_Tmr					= 50;		// 5s 동안 LCD 표시
				Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
				Gu8_Color_Temp_Flag = 1;
				Gu8_Dim_Flag = 0;
				SET_LED_State(touch_switch, LED_DETECT_ON);
				Beep(ON);
				if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)) && GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)) )	// 디밍 등이 모두 켜져 있으면
				{
					PWM_Level_Set((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), LEVEL_UP);
					PUT_PWMCtrl((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), ON);
				}				
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	// 디밍1 등이 켜져 있으면
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_1, LEVEL_UP);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
				}
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	// 디밍2 등이 켜져 있으면
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_2, LEVEL_UP);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
				 }*/
				break;
			case mapping_ITEM_COLOR_TEMP_DN:
				/*Gu8_LCD_DIM_Tmr					= 50;		// 5s 동안 LCD 표시
				Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
				Gu8_Color_Temp_Flag = 1;
				Gu8_Dim_Flag = 0;
				SET_LED_State(touch_switch, LED_DETECT_ON);
				Beep(ON);
				if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)) && GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)) )	// 디밍 등이 모두 켜져 있으면
				{
					PWM_Level_Set((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), LEVEL_DN);
					PUT_PWMCtrl((CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2), ON);
				}				
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	// 디밍1 등이 켜져 있으면
				{
					PWM_Level_Set(CONTROL_BIT_DIMMING_1, LEVEL_DN);
					PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
				}				
				else if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	// 디밍2 등이 켜져 있으면
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
							Gu8_Sleep_Flag = 0;			//설정중이라도 다시 터치하면 플래그 초기화
							Gu8_Sleep_cnt = 0;				//설정중이라도 다시 터치하면 cnt 초기화		
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
					Gu8_Sleep_Flag = 0;			//설정중이라도 다시 터치하면 플래그 초기화
					Gu8_Sleep_cnt = 0;				//설정중이라도 다시 터치하면 cnt 초기화
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
					}		//일괄소등 중 지연소등이 필요없으면 IF문 주석 처리.
				}
#endif				
#ifdef _KOCOM_PROTOCOL_
				if(Batch_Light_Use_Tmr == 0)
				{
					Flag	= SET_Switch_State(touch_switch, Flag);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
					SET_LED_State(touch_switch, Flag);
					Beep(Flag);
					// 일괄소등이 켜졌다는 것은 릴레이 OFF 동작으로 집안의 모든등을 끈다는 의미임
					// 일괄소등이 꺼졌다는 것은 릴레이 ON 동작으로 집안의 모든등에 전원이 공급되어 제어를 할 수 있다는 의미임
					PUT_RelayCtrl(item2ctrl(item), Flag);		// 항목기준 제어

					Block_Active_Flag.Batch = 1;
					Batch_Light_Use_Tmr = 3;
#ifdef COMM_THREEWAY
					if(item2tsn(mapping_ITEM_3WAY_1))
					{
						if(Flag == OFF)	//일괄 소등 시
						{
							ThreeWay_State = GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
							{
								SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
								SET_LED_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
								PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_1), OFF);
							}
						}
						else	//일괄 소등 해제 시
						{
							if(ThreeWay_State)	//소등 이전 상태가 ON이면 소등 해제와 함께 ON으로 제어
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

				Flag	= SET_Switch_State(touch_switch, Flag);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
				SET_LED_State(touch_switch, Flag);
				Beep(Flag);
				// 일괄소등이 켜졌다는 것은 릴레이 OFF 동작으로 집안의 모든등을 끈다는 의미임
				// 일괄소등이 꺼졌다는 것은 릴레이 ON 동작으로 집안의 모든등에 전원이 공급되어 제어를 할 수 있다는 의미임
				PUT_RelayCtrl(item2ctrl(item), Flag);		// 항목기준 제어
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
					PUT_RelayCtrl(item2ctrl(item), Flag);		// 항목기준 제어
					if(Flag == OFF)
					{
						BATCH_BLOCK_Control(SET__GAS_CLOSE_REQUEST);
						if(item == mapping_ITEM_BATCH_LIGHT_n_GAS)			Block_Active_Flag.Gas = 1;	//가스 차단 요청은 일괄 소등시에만 동작
						else if(item == mapping_ITEM_BATCH_LIGHT_n_COOK)	Block_Active_Flag.Cook = 1;	//쿡탑 차단 요청은 일괄 소등시에만 동작
						Gu8_Direct_Control = 1;
					}
					else	//일괄 소등 해제에는 일괄 데이터만 즉시 전송
					{
						Block_Active_Flag.Batch = 1;
					}
					Batch_Light_Use_Tmr = 3;
#ifdef COMM_THREEWAY
					if(item2tsn(mapping_ITEM_3WAY_1))
					{
						if(Flag == OFF)	//일괄 소등 시
						{
							ThreeWay_State = GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
							{
								SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
								SET_LED_State(item2tsn(mapping_ITEM_3WAY_1), OFF);
								PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_1), OFF);
							}
						}
						else	//일괄 소등 해제 시
						{
							if(ThreeWay_State)	//소등 이전 상태가 ON이면 소등 해제와 함께 ON으로 제어
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == OFF)
								{
									SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), ON);
									SET_LED_State(item2tsn(mapping_ITEM_3WAY_1), ON);
									PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_1), ON);
								}
							}
							else	//소등 이전 상태가 OFF면 소등 해제 시 에도 상태 유지함.
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
				PUT_RelayCtrl(item2ctrl(item), Flag);		// 항목기준 제어
				if(Flag == OFF)
				{
					BATCH_BLOCK_Control(SET__GAS_CLOSE_REQUEST);
				}
#endif
#endif
				break;
			case mapping_ITEM_GAS:		// 무조건 차단만 실행
			case mapping_ITEM_COOK:
#ifndef _NO_PROTOCOL_
	#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
				BATCH_BLOCK_Control(SET__GAS_CLOSE_REQUEST);
#ifdef _KOCOM_PROTOCOL_
				Touch_Use_Tmr = 5;	//500ms
#endif
	#endif	//_ONE_SIZE_BATCH_BLOCK_MODEL_
#else		//NO PROTOCOL 일때 가스 차단 동작.
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
				ALL_n_Group_Light_Switch_LED_Ctrl();		// 전체등 & 그룹등 LED 제어
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
			ALL_Electricity_Switch_Ctrl(pG_Config->Factory_Mapping_ALL_Electricity, OFF);	// 전체등 소등
			
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
				PUT_RelayCtrl(item2ctrl(item), OFF);	// 항목기준 제어
				
				SET_SWITCH_Delay_OFF_Flag(item, 0);
				
				ALL_Electricity_Switch_LED_Ctrl();
			}
		}
	}
/*#ifdef _HYUNDAI_PROTOCOL_		//현대 프로토콜, 5초 지연후 가스차단 LED ON
	item = mapping_ITEM_GAS;
	if(GET_SWITCH_Delay_OFF_Flag(item))
	{
		if(GET_SWITCH_Delay_OFF_Tmr(item) == 0)
		{				
			touch_switch	= item2tsn(item);
			SET_Switch_State(touch_switch, OFF);
			SET_LED_State(touch_switch, LED_OFF);		//가스는 지연 후 LED ON
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
		Gu8_SWITCH_Delay_OFF_Tmr[item]	= tmr;		// 5sec 후 소등
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

void GET_Event(void)	// Zorocrossing 상태에서 릴리이 및 PWM이 동작
{
	if(Relay_CtrlBuff.PUT_Event_Cnt != Relay_CtrlBuff.GET_Event_Cnt)
	{
		Relay_Ctrl(Relay_CtrlBuff.Ctrl[Relay_CtrlBuff.GET_Event_Cnt], Relay_CtrlBuff.Flag[Relay_CtrlBuff.GET_Event_Cnt]);	// 릴레이 제어
		Relay_CtrlBuff.GET_Event_Cnt++;
		if(Relay_CtrlBuff.GET_Event_Cnt >= MAX_EVENT_BUF)	Relay_CtrlBuff.GET_Event_Cnt	= 0;
	}
	else if(PWM_CtrlBuff.PUT_Event_Cnt != PWM_CtrlBuff.GET_Event_Cnt)
	{
		PWM_Ctrl(PWM_CtrlBuff.Ctrl[PWM_CtrlBuff.GET_Event_Cnt], PWM_CtrlBuff.Flag[PWM_CtrlBuff.GET_Event_Cnt]);				// PWM 제어
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

uint8_t	GET_Light_State(void)		// 리턴
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

uint8_t	GET_Light_ON_Count(void)		// 켜져있는 등의 갯수
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

uint8_t	GET_Electricity_State(void)		// 리턴
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
					if(GET_Light_ON_Count() == 1 && GET_Switch_State(touch_switch))	// 현재등이 켜져 있고 마지막 등이면
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
					PUT_RelayCtrl(item2ctrl(item), Flag);		//210708 디밍전등도 전체제어로 릴레이 제어 가능하도록
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
		for(item=mapping_ITEM_MAX-1; item > mapping_ITEM_DISABLE; item--)	// 20200709	사업팀 요청으로 기능 수정함(추이사, 심차장), 첫번째 등 지연소등
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
					if(GET_Light_ON_Count() == 1 && GET_Switch_State(touch_switch))	// 현재등이 켜져 있고 마지막 등이면
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
					PUT_RelayCtrl(item2ctrl(item), Flag);		//210708 디밍전등도 전체제어로 릴레이 제어 가능하도록
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
			Flag = OFF;									// 일괄소등은 지연오프 대기중에 다시 제어를 하면 복구가 아닌 즉시 제어
		}
		else if(item == mapping_ITEM_BATCH_LIGHT_n_GAS)	//Flag 상태가 변화지 않아서 추가함
		{
			;
		}
		else
		{
			Flag = ON;									// 지연오프 대기중이면 원래대로 복구
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
			SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);		// 스위치를 계속 누르고 있으면 반복 동작을 하게되는데 이를 방지하기 위해
#ifdef _KOCOM_PROTOCOL_							
			if(Gu8_PowerSaving_Tmr != 0)								//KOCOM 프로토콜에서 터치 이벤트 발생시 월패드로 전달.
			{
				if(tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_1 || tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_2 || tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_ALL)
				{
					if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)) == 0)	//전열 설정모드가 아닐 떄만
					{
						Gu8_Elec_Touch_Flag = 1;		//코콤프로토콜 전열 터치 플래그
					}
				}
				else
				{
					Gu8_Light_n_ETC_Touch_Flag = 1;	//코콤프로토콜 전등 및 그 외 터치 플래그
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
			SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);		// 스위치를 계속 누르고 있으면 반복 동작을 하게되는데 이를 방지하기 위해
#ifdef _KOCOM_PROTOCOL_							
			if(Gu8_PowerSaving_Tmr != 0)								//KOCOM 프로토콜에서 터치 이벤트 발생시 월패드로 전달.
			{
				if(tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_1 || tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_2 || tsn2item(touch_switch) == mapping_ITEM_ELECTRICITY_ALL)
				{
					if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)) == 0)	//전열 설정모드가 아닐 떄만
					{
						Gu8_Elec_Touch_Flag = 1;		//코콤프로토콜 전열 터치 플래그
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
				Gu8_LightGroup_SET_Flag	= 0;		// 설정 완료
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
			Touch_Evente_Control(touch_switch, tmr);	// 스위치 동작시간 체크 후 제어
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
				if((GET_LED_State(item2tsn(mapping_ITEM_SLEEP)) == LED_FLASHING) && (Gu8_Sleep_Set_Tmr == 0))	//슬립 설정모드 진입중이고, 설정 시간이 0이 되면
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

		if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))	// 설정키가 동작 중이면
		{
			if(SETTING_1S_TMR <= tmr)		// 1sec
			{
				switch(tsn2item(touch_switch))
				{
					case mapping_ITEM_ELECTRICITY_1:
						if(((double)Gu16_LCD_Watt_1 * 0.8) < 0.0)	Gu16_ElecLimitCurrent_1	= 0;
						else										Gu16_ElecLimitCurrent_1	= (uint16_t)((double)Gu16_LCD_Watt_1 * 0.8);	// 현재 값의 80%로 저장
						pG_State->ETC.Auto1		= 1;
						Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 1;
						Gu8_LCD_ElecLimitCurrent_Tmr			= 0;
						Store_ElecLimitCurrent();
						Beep(BEEP_MEL);
						//Beep(BEEP_TWO);
						break;
						
					case mapping_ITEM_ELECTRICITY_2:
						if(((double)Gu16_LCD_Watt_2 * 0.8) < 0.0)	Gu16_ElecLimitCurrent_2	= 0;
						else										Gu16_ElecLimitCurrent_2	= (uint16_t)((double)Gu16_LCD_Watt_2 * 0.8);	// 현재 값의 80%로 저장
						pG_State->ETC.Auto2		= 1;
						Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 11;
						Gu8_LCD_ElecLimitCurrent_Tmr			= 0;
						Store_ElecLimitCurrent();
						Beep(BEEP_MEL);
						break;
				}
			}
		}
		else												// 설정키 동작중이 아니면
		{
#if 0
			if(SETTING_10S_TMR <= tmr)						// 10sec 이상 눌렀으면
			{
				;
			}
			else if(SETTING_5S_TMR <= tmr)					// 5sec 이상 눌렀으면
			{
				/*	20200709	사업팀 요청으로 기능 삭제함(추이사, 심차장)
				switch(tsn2item(touch_switch))
				{
					case mapping_ITEM_LIGHT_1:
					case mapping_ITEM_BATCH_LIGHT_OFF:		// 일괄소등
						
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
			if(SETTING_2S_TMR <= tmr)					// 2sec 이상 눌렀으면
			{
				switch(tsn2item(touch_switch))
				{
					/*	20200709	사업팀 요청으로 기능 삭제함(추이사, 심차장)
					case mapping_ITEM_LIGHT_1:
					case mapping_ITEM_BATCH_LIGHT_OFF:		// 일괄소등
						Beep(BEEP_MEL);
						if(pG_State->ETC.DelayOFF)	pG_State->ETC.DelayOFF	= 0;
						else						pG_State->ETC.DelayOFF	= 1;
						//printf("delay off %d\n", (uint16_t)pG_State->ETC.DelayOFF);
						break;
					*/
					case mapping_ITEM_LIGHT_GROUP:
						Beep(ON);
						Gu8_LightGroup_SET_Flag	= 1;
						if(GET_Switch_State(touch_switch) == 0)				// 그룹조명이 꺼져 있으면
						{
							Touch_Evente_Control(touch_switch, tmr);		// 스위치 제어
						}
						SET_LED_State(touch_switch, LED_FLASHING);			//위치를 if문 위에서 아래로 수정하여 그룹 설정시 플래싱 오류 해결
						break;
#if 0
					case mapping_ITEM_LIGHT_1:
						if(SETTING_5S_TMR <= tmr)
						{
							// Touch_Evente_Control(touch_switch, tmr);
							// EventCtrl(touch_switch, INVERSE);	//누를 때 스위치 상태 바뀌었으므로 다시 복구를 위해 사용.
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
			SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);		// 스위치를 계속 누르고 있으면 반복 동작을 하게되는데 이를 방지하기 위해
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
					if(GET_Touch_Ignore_Flag(touch_switch) == IGNORE_CLEAR)				// 스위치 무시가 아니면
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
					else if(GET_Touch_Ignore_Flag(touch_switch) != SET_SPECIAL_SWITCH)	// 특수키 무시가 아니면
					{
						if(SETTING_10S_TMR <= tmr)									// 10sec 이상 눌렀으면
						{
							memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));	// 터치칩 초기화
						}
					}
					else if(GET_Touch_Ignore_Flag(touch_switch) == SET_SPECIAL_SWITCH)		// 특수키 무시 20201113
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

					if(Gu8_PowerSaving_Tmr == 0)	// 전등이 켜지면 LED OFF, 꺼지면 LED ON	 and 슬립모드였을 경우 
					{
						Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;								// 5sec 설정 및 wakeup (터치에 대한 동작없음)
						SET_Touch_Ignore_Flag(touch_switch, SET_IGNORE_SWITCH);
						break;
					}
				case LED_OFF__LIGHT_IS_ON_2:
				default:
					Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
					Gu8_INFO_Disp_Tmr			= 0xFF;									// 최초 부팅 후 한번이라도 터치가 있으면 LCD 정보출력 중지
					if(GET_Touch_Ignore_Flag(touch_switch) == IGNORE_CLEAR)				// 스위치 무시가 아니면
					{
						if(Gu8_LightGroup_SET_Flag)										// 그룹조명 설정 동작
						{
							Group_Light_Setup(touch_switch, tmr);
						}
						else if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))			// 전열 설정 동작
						{
							switch(tsn2item(touch_switch))
							{
								case mapping_ITEM_ELECTRICITY_1:
								case mapping_ITEM_ELECTRICITY_2:
									Touch_Evente_Control(touch_switch, 255);			// OFF 시간과 관계없이 즉시 전달
									break;
								default:
									Touch_Evente_Control(touch_switch, tmr);			// 전열 이외는 스위치 동작시간 체크 후 제어
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
						else															// 일반동작
						{
							Touch_Evente_Control(touch_switch, tmr);					// 스위치 동작시간 체크 후 제어				
						}
					}
					else if(GET_Touch_Ignore_Flag(touch_switch) != SET_SPECIAL_SWITCH)	// 특수키 무시가 아니면
					{
						Gu8_Special_Function_Key		= touch_switch;
						Gu8_Special_Function_Key_Tmr	= tmr;
						if(SETTING_10S_TMR <= tmr)						// 10sec 이상 눌렀으면
						{
							memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));	// 터치칩 초기화
						}
						else if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))		// 설정키가 동작 중이면
						{
							if(SETTING_1S_TMR <= Gu8_Special_Function_Key_Tmr)	// 설정할 수 있는 최대시간을 넘으면 특수키 즉시실행
							{
								Special_Function_Key_Process(Gu8_Special_Function_Key, Gu8_Special_Function_Key_Tmr);		// 즉시실행하기 위해
							}
						}
						else
						{
							//if(SETTING_10S_TMR <= Gu8_Special_Function_Key_Tmr)	// 설정할 수 있는 최대시간을 넘으면 특수키 즉시실행
							if(SETTING_2S_TMR <= Gu8_Special_Function_Key_Tmr)	// 설정할 수 있는 최대시간을 넘으면 특수키 즉시실행
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
	
	// 전체, 그룹 스위치가 켜져 있는지 확인
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		if(GET_Switch_State(i))				// 켜져 있었는지 확인
		{
			switch(tsn2item(i))
			{
				case mapping_ITEM_LIGHT_ALL:		// 전등 ALL 스위치
					flag	|= 0x01;
					break;			
				case mapping_ITEM_ELECTRICITY_ALL:	// 전열 ALL 스위치
					flag	|= 0x02;				// 전체전열이 켜져 있었는지 확인
					break;
				case mapping_ITEM_LIGHT_GROUP:			// 전등 GROUP 스위치
					flag	|= 0x04;				// 그룹 전등이 켜져 있었는지 확인
					break;
			}
		}
	}
	
	// 전체, 그룹 스위치가 켜져 있으면 그에 해당하는 동작은 OFF로 설정
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		switch(tsn2item(i))
		{
			case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
			case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
			case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
			case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
			case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
			case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
			case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
			case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
			case mapping_ITEM_3WAY_1:				// 3로 1 (릴레이 제어) case mapping_ITEM_ELEVATOR 아래에 있었는데, 리셋 전 상태 복구 되지 않아 옮김.
			case mapping_ITEM_3WAY_2:				// 3로 2 (릴레이 제어)
				if(flag & 0x01)			// 전체등이 켜져 있으면
				{
					SET_Switch_State(i, OFF);		// 전체등이 있으면 일반 등 스위치 상태 클리어
				}
				else if(flag & 0x04)	// 그룹등이 켜져 있으면
				{
					SET_Switch_State(i, OFF);		// 그룹등이 있으면 일반 등 스위치 상태 클리어
				}
				break;
			case mapping_ITEM_ELECTRICITY_1:		// 전열 1 (릴레이 제어)
			case mapping_ITEM_ELECTRICITY_2:		// 전열 2 (릴레이 제어)
				if(flag & 0x02)			// 전체전열이 켜져 있으면
				{
					SET_Switch_State(i, OFF);		// 전체등이 있으면 일반 등 스위치 상태 클리어
				}
				break;

			//case mapping_ITEM_GAS:					// 가스차단
			//case mapping_ITEM_ELEVATOR:				

			case mapping_ITEM_ELECTRICITY_ALL:		// 전열 ALL 스위치
			case mapping_ITEM_BATCH_LIGHT_OFF:
			case mapping_ITEM_BATCH_LIGHT_n_GAS:
			case mapping_ITEM_BATCH_LIGHT_n_COOK:
			case mapping_ITEM_LIGHT_ALL:			// 전등 ALL 스위치
			case mapping_ITEM_LIGHT_GROUP:			// 전등 GROUP 스위치
				break;
			default:
				SET_Switch_State(i, OFF);			// 일반 스위치 상태는 클리어
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
	
	// 전체, 그룹이 꺼져 있으면 개별등, 개별전열을 복구하고,	전체, 그룹이 켜져 있으면 전체, 그룹 복구
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		item = tsn2item(i);
		if(GET_Switch_State(i) != 0)	//스위치 상태가 ON이면
		{
			switch(item)
			{
				case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
				case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
				case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
				case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
				case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
				case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
				case mapping_ITEM_ELECTRICITY_1:		// 전열 1 (릴레이 제어)
				case mapping_ITEM_ELECTRICITY_2:		// 전열 2 (릴레이 제어)

				case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
				case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)	
				case mapping_ITEM_BATCH_LIGHT_OFF:
				case mapping_ITEM_BATCH_LIGHT_n_GAS:
				case mapping_ITEM_BATCH_LIGHT_n_COOK:
				case mapping_ITEM_ELECTRICITY_ALL:		// 전열 ALL 스위치				
					// EventCtrl(i, ON);				// 저장된 데이터로 복구, EventCtrl로 복구하면 전등이 복구되면서 그룹 스위치와 전체전등 스위치의 LED 상태가 달라져서 아래와 같이 LED만 복구함.
					// SET_LED_State(i, ON);				// 저장된 데이터로 복구
					// break;
				case mapping_ITEM_LIGHT_GROUP:			// 전등 GROUP 스위치					
				case mapping_ITEM_LIGHT_ALL:			// 전등 ALL 스위치
					EventCtrl(i, ON);
					break;
				case mapping_ITEM_3WAY_1:				// 3로 1 (릴레이 제어)
				case mapping_ITEM_3WAY_2:				// 3로 2 (릴레이 제어)
#if defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) && defined(WIRING_THREEWAY)	//일괄스위치 모델이고 삼로 전등의 현재 상태를 모르는 경우에는 LED OFF로 복구
					// printf("--1 switch state %d\r\n", (uint16_t)GET_Switch_State(i));
					SET_LED_State(i, ON);										//실제 동작에서는 해당 버튼 누를 때 LED 점멸함
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
				case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
				case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
				case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
				case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
				case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
				case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)

				case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)		//디밍 전등 꺼진 상태일 때, 복구시 스위치 상태와 맞지 않아서 추가함.
				case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)		//디밍 전등 꺼진 상태일 때, 복구시 스위치 상태와 맞지 않아서 추가함.		
				case mapping_ITEM_ELECTRICITY_1:		// 전열 1 (릴레이 제어)
				case mapping_ITEM_ELECTRICITY_2:		// 전열 2 (릴레이 제어)
				case mapping_ITEM_BATCH_LIGHT_OFF:
				case mapping_ITEM_BATCH_LIGHT_n_GAS:
				case mapping_ITEM_BATCH_LIGHT_n_COOK:
					SET_LED_State(i, OFF);
					SET_SWITCH_Delay_OFF_Tmr(item, 0);	// 복구할 때에는 즉시
					break;
				case mapping_ITEM_3WAY_1:				// 3로 1 (릴레이 제어)
				case mapping_ITEM_3WAY_2:				// 3로 2 (릴레이 제어)
#if defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) && defined(WIRING_THREEWAY)	//일괄스위치 모델이고 삼로 전등의 현재 상태를 모르는 경우에는 LED OFF로 복구
					// printf("--0 switch state %d\r\n", (uint16_t)GET_Switch_State(i));
					SET_LED_State(i, OFF);
#elif defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) && defined(_HYUNDAI_PROTOCOL_) && defined(COMM_THREEWAY)
					SET_LED_State(i, ON);	//현대통신의 통신 3로의 경우 누를 때 마다 점멸함. 그래서 LED OFF로.
#else
					SET_LED_State(i, OFF);
					// SET_SWITCH_Delay_OFF_Tmr(item, 0);	// 복구할 때에는 즉시
#endif
					break;
				case mapping_ITEM_GAS:					// 가스차단
				case mapping_ITEM_COOK:
				case mapping_ITEM_GAS_n_COOK:
				case mapping_ITEM_ELEVATOR:				// 엘리베이터 호츨은 복구에서 제외(아파트 전체가 정전 후 복구되었을 때 모든 세대가 호출...)
				case mapping_ITEM_SLEEP:
					SET_LED_State(i, ON);
					break;
			}
		}
	}	
}

void Touch_Error(uint8_t touch_switch, uint8_t tmr, uint8_t Flag)		//20210524 Flag가 1이면, EventCtrl, Flag 0이면, ActionCheck
{
	static uint16_t cnt = 0;
	static uint8_t old_touch_switch;
	if(tsn2item(touch_switch) != mapping_ITEM_LIGHT_GROUP)
	{
		if((Flag == 0) && (Gu8_Touch_Err_Flag == 0) && (Gu16_Touch_Err_Tmr == 0))		//터치 에러 체크. (터치 2초이상 눌렸을 경우 120초 동안 터치동작 확인)
		{
			if((tmr >= 20) && (tmr < 90))		//2초 이상
			{
				Gu16_Touch_Err_Tmr = 120;
				Gu8_Touch_Err_Flag = 1;
				cnt = 0;
				printf("Temp_Tmr = 120\r\n");
			}
		}
	}
	else		//GROUP Switch 터치인 경우
	{
		if((Flag == 0) && (Gu8_Touch_Err_Flag == 0) && (Gu16_Touch_Err_Tmr == 0))		//터치 에러 체크. (터치 2초이상 눌렸을 경우 120초 동안 터치동작 확인)
		{
			if((tmr >= 30) && (tmr < 90))		//3초 이상
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
				if(cnt >= 30000)		//에러 체크 진입 후 120초동안 터치가 기준 이상 cnt가 된 경우 터치칩 초기화.
				{
					memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
					Gu16_Touch_Err_Tmr = 5;
					Gu8_Touch_Err_Flag = 0;
					Gu8_Touch_Err_Cnt = 0;					
					cnt = 0;
				}
			}
		}
		if((tmr >= 99))					//에러 체크 진입 후 하나의 터치 스위치를 10초 이상 터치 한 경우 터치칩 초기화가 되므로 나머지 변수들 초기화.
		{
			printf("CLEAR\r\n");
			Gu16_Touch_Err_Tmr = 5;
			Gu8_Touch_Err_Flag = 0;
			// Flag = 0;
			Gu8_Touch_Err_Cnt = 0;
			cnt = 0;
			// tmr = 0;		//터치 10초 이상으로  초기화 할 때 바로 터치 에러 체크로 들어가는 경우 방지.
		}
	}
	else if((Flag == 1) && (Gu8_Touch_Err_Flag == 1))		//EventCtrl로 이전 동작과 현재 동작이 같은 경우 카운트.
	{
		if((Gu16_Touch_Err_Tmr))
		{
			if(touch_switch == old_touch_switch)	Gu8_Touch_Err_Cnt++;
			printf("Cnt = %d, Tmr = %d\r\n", (uint16_t)Gu8_Touch_Err_Cnt, Gu16_Touch_Err_Tmr);
			printf("old = %d, pre = %d\r\n", (uint16_t)old_touch_switch, (uint16_t)touch_switch);
		}
		else
		{
			// printf("Temp Tmr, Cnt reset\r\n");			//120초 타이머가 0이 되면 각 변수들 초기화
			// Gu8_Touch_Err_Flag = 0;
			// Flag = 0;
			// Gu8_Touch_Err_Cnt = 0;
			// cnt = 0;
		}
		if(Gu8_Touch_Err_Cnt >= 10)							//EventCtrl로 이전 동작과 현재 동작이 같은 경우 카운트하여 10 이상되면 터치칩 초기화
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

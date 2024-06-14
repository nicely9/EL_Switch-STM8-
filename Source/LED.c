/************************************************************************************
	Project		: 전자식스위치
	File Name	: LED.C
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
#include "WDGnBeep.h"

void led_output(uint8_t switch_led, uint8_t bright);
void LedOn(uint8_t	switch_led);
void LedOff(uint8_t	switch_led);
void set_led_flashing_flag(uint8_t switch_led, uint8_t state);
uint8_t get_led_flashing_flag(uint8_t switch_led);
//-------------------------------------------------------------------------------------------------------------------------
uint8_t				LED_State_Flag[MAX_SWITCH];
uint8_t				LED_Flashing_Flag[MAX_SWITCH];
uint8_t				Gu8_LED_Flashing_Tmr;
uint8_t				Gu8_LED_TEST_Tmr = 5;		//210106
// #define OFFICE_SET		//210325, 사무실 설정
//-------------------------------------------------------------------------------------------------------------------------

void LED_Init(void)
{
	uint8_t	i;
	
	Gu8_LED_Flashing_Tmr			= 0;
	memset((void*)&LED_Flashing_Flag, 0, MAX_SWITCH);
	
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		SET_LED_State(i, LED_UNDEFINED);
	}
}

void LED_Output_Process(void)
{
	//uint8_t	beep = 0;
	uint8_t	switch_led, item;
	uint8_t	flashing_flag = 0;
	static uint8_t flag = 1;
	
	for(switch_led=mapping_SWITCH_1;switch_led<mapping_SWITCH_MAX;switch_led++)
	{
		switch(GET_LED_State(switch_led))
		{
			case LED_DETECT_ON:
				if(pG_State->ETC.LED_Mode == LED_LOW_LEVEL__LIGHT_IS_OFF)	// LED 모드가 전등이 켜지면 LED ON, 전등이 꺼지면 LED Low level on
				{
					led_output(switch_led, LED_ON_NORMAL_LEVEL);
				}
				else if(pG_State->ETC.LED_Mode == LED_OFF__LIGHT_IS_ON)		// LED 모드가 전등이 켜지면 LED OFF, 꺼지면 LED ON
				{
					//led_output(switch_led, LED_ON_LEVEL);
					led_output(switch_led, LED_OFF_LEVEL);
				}
				else														// 전등이 켜지면 LED ON, 꺼지면 LED OFF(일괄소등 반대로 동작)
				{
					led_output(switch_led, LED_ON_LEVEL);
				}
				break;
				
			case LED_OFF:
				set_led_flashing_flag(switch_led, 0);
				LedOff(switch_led);
				break;
				
			case LED_ON:
				set_led_flashing_flag(switch_led, 0);
				LedOn(switch_led);
				break;
				
			case LED_FLASHING:
				item	= tsn2item(switch_led);
				switch(item)
				{
					case mapping_ITEM_ELECTRICITY_ALL:
					case mapping_ITEM_ELECTRICITY_1:
					case mapping_ITEM_ELECTRICITY_2:
						if(GET_SWITCH_Delay_OFF_Tmr(item) <= 3)	// 마지막 LCD 및 Beep의 동작이 간혹 불협이 되는 경우가 있어 적용
						{
// #if defined(_TWO_SIZE_LIGHT_1_n_ELEC_2_) || defined(_TWO_SIZE_LIGHT_2_n_ELEC_2_) || defined(_TWO_SIZE_LIGHT_3_n_ELEC_2_) || defined(_TWO_SIZE_LIGHT_4_n_ELEC_2_)\
// || defined(_TWO_SIZE_LIGHT_1_n_ELEC_2_n_3WAY_1) || defined(_TWO_SIZE_LIGHT_2_n_ELEC_2_n_3WAY_1) || defined(_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1) || defined(_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1)
							if(pG_Config->Enable_Flag.LCD_Type == 0)
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)) == 0)	//220405, LCD 없는 모델은 설정 진입이 아닐 때만
								{
									break;
								}
							}
							else
							{
								break;
							}
// #else
// #endif
						}
					default:
						if(Gu8_LED_Flashing_Tmr == 0 || flashing_flag)
						{
							if(flashing_flag == 0)	// 여러개의 Flashing LED가 있을 수 있으므로 한번만 설정하기위해서
							{
								Gu8_LED_Flashing_Tmr	= 5;			// 0.5sec
								flag	= (uint8_t)(flag ? 0 : 1);
							}
							flashing_flag	= 1;
							
							if(flag)	set_led_flashing_flag(switch_led, 1);
							else		set_led_flashing_flag(switch_led, 0);
							
							if(get_led_flashing_flag(switch_led))
							{
								switch(item)
								{
									case mapping_ITEM_ELECTRICITY_ALL:
									case mapping_ITEM_ELECTRICITY_1:
									case mapping_ITEM_ELECTRICITY_2:
										//if(beep == 0)
										{
// #if defined(_TWO_SIZE_LIGHT_1_n_ELEC_2_) || defined(_TWO_SIZE_LIGHT_2_n_ELEC_2_) || defined(_TWO_SIZE_LIGHT_3_n_ELEC_2_) || defined(_TWO_SIZE_LIGHT_4_n_ELEC_2_)\
// || defined(_TWO_SIZE_LIGHT_1_n_ELEC_2_n_3WAY_1) || defined(_TWO_SIZE_LIGHT_2_n_ELEC_2_n_3WAY_1) || defined(_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1) || defined(_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1)
											if(pG_Config->Enable_Flag.LCD_Type == 0)
											{
												if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)) == 0)	//220405, LCD 없는 모델은 설정 진입이 아닐 때만
												{
													Beep(ON);		// LED가 Flashing일때 Beep
												}
											}
											else
											{
												Beep(ON);
											}
// #else
// #endif
											//beep	= 1;
										}
										break;
										
									default:
										if(pG_State->ETC.LED_Mode == LED_OFF__LIGHT_IS_ON_2)
										{
#if defined(_ONE_SIZE_LIGHT_1_n_SLEEP_) || defined (_ONE_SIZE_LIGHT_2_n_SLEEP_)
#else
											Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;		// LED 점멸중에는 절전모드로 진입하지 않음
#endif
										}
										/*else if(pG_State->ETC.LED_Mode == LED_OFF__LIGHT_IS_ON)
										{
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_											
											Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;		// LED 점멸중에는 절전모드로 진입하지 않음
#endif
										}*/								
										break;
								}
								LedOn(switch_led);
							}
							else
							{
#ifndef OFFICE_SET
								LedOff(switch_led);	
#else
								led_output(switch_led, LED_OFF_LEVEL);		//210325, 사무실 세팅용 flashing 동작이 되도록, 없으면 LED Flashing이 밝기10->밝기4로 Flashing 됨.
#endif
							}
						}
						break;
				}
				break;
		}
	}
}

void ALL_Electricity_Switch_LED_Ctrl(void)
{
	uint8_t touch_switch;
	//----------------------------------------------------------------------------------------------------------
	// 전체젼열 LED 제어(전열 중 하나라도 꺼지면 전체전열 LED는 꺼지고 둘다 켜져 있으면 전체전열 LED는 켜준다
	if(GET_Electricity_State() == pG_Config->Factory_Mapping_ALL_Electricity)
	{
		touch_switch	= item2tsn(mapping_ITEM_ELECTRICITY_ALL);
		SET_Switch_State(touch_switch, ON);					// 전체가 점등되어 있으면 ALL Switch는 ON 상태로
		SET_LED_State(touch_switch, ON);
		
		//SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);
	}
	else
	{
		touch_switch	= item2tsn(mapping_ITEM_ELECTRICITY_ALL);
		SET_Switch_State(touch_switch, OFF);	// 하나라도 꺼져있으면 ALL Switch는 OFF 상태로
		SET_LED_State(touch_switch, OFF);
	}
	//----------------------------------------------------------------------------------------------------------
}

void ALL_n_Group_Light_Switch_LED_Ctrl(void)
{
	uint8_t touch_switch;
	
	if(Gu8_LightGroup_SET_Flag == 0)		// 그룹조명 설정 동작이 아닐때
	{
		if(item2tsn(mapping_ITEM_LIGHT_GROUP))		// 그룹등이 있으면
		{
			if((GET_Light_State() & pG_State->User_Mapping_ALL_Light) == pG_State->User_Mapping_ALL_Light)
			{
				touch_switch	= item2tsn(mapping_ITEM_LIGHT_GROUP);
				SET_Switch_State(touch_switch, ON);					// 전체가 점등되어 있으면 GROUP Switch는 ON 상태로
				SET_LED_State(touch_switch, ON);
			}
			else
			{
				touch_switch	= item2tsn(mapping_ITEM_LIGHT_GROUP);
				SET_Switch_State(touch_switch, OFF);				// 하나라도 꺼져있으면 GROUP Switch는 OFF 상태로
				SET_LED_State(touch_switch, OFF);
			}
		}
		if(item2tsn(mapping_ITEM_LIGHT_ALL))		// 전체등 설정이 있으면
		{
			if((GET_Light_State() & pG_Config->Factory_Mapping_ALL_Light) == pG_Config->Factory_Mapping_ALL_Light)
			{
				touch_switch	= item2tsn(mapping_ITEM_LIGHT_GROUP);
				if(touch_switch)
				{
					SET_Switch_State(touch_switch, OFF);				// 전체등이 켜지면 GROUP Switch는 OFF 상태로
					SET_LED_State(touch_switch, OFF);
				}
				
				touch_switch	= item2tsn(mapping_ITEM_LIGHT_ALL);
				SET_Switch_State(touch_switch, ON);					// 전체가 점등되어 있으면 ALL Switch는 ON 상태로
				SET_LED_State(touch_switch, ON);
			}
			else
			{
				touch_switch	= item2tsn(mapping_ITEM_LIGHT_ALL);
				SET_Switch_State(touch_switch, OFF);				// 하나라도 꺼져있으면 ALL Switch는 OFF 상태로
				SET_LED_State(touch_switch, OFF);
			}
		}
	}
}

void led_output(uint8_t switch_led, uint8_t bright)
{
	if(TouchConfig.MAX_TouchChip >= 1)	// GT308L
	{
		switch(switch_led)
		{
#ifdef	_TWO_SIZE_LIGHT_MODEL_
			case 1:		TouchConfig.GT308L[0].PWM_Duty.Bit.N1	= bright;	break;
			case 2:		TouchConfig.GT308L[0].PWM_Duty.Bit.N2	= bright;	break;
			case 3:		TouchConfig.GT308L[0].PWM_Duty.Bit.N3	= bright;	break;
			case 4:		TouchConfig.GT308L[0].PWM_Duty.Bit.N4	= bright;	break;
			case 5:		TouchConfig.GT308L[0].PWM_Duty.Bit.N5	= bright;	break;
			case 6:		TouchConfig.GT308L[0].PWM_Duty.Bit.N6	= bright;	break;
			case 7:		TouchConfig.GT308L[0].PWM_Duty.Bit.N7	= bright;	break;
			case 8:		TouchConfig.GT308L[0].PWM_Duty.Bit.N8	= bright;	break;
#else
			case 1:		TouchConfig.GT308L[0].PWM_Duty.Bit.N1	= bright;	break;
			case 2:		TouchConfig.GT308L[0].PWM_Duty.Bit.N2	= bright;	break;
			case 3:		TouchConfig.GT308L[0].PWM_Duty.Bit.N3	= bright;	break;
			case 4:		TouchConfig.GT308L[0].PWM_Duty.Bit.N4	= bright;	break;
			case 5:		TouchConfig.GT308L[0].PWM_Duty.Bit.N5	= bright;	break;
			case 6:		TouchConfig.GT308L[0].PWM_Duty.Bit.N6	= bright;	break;
			case 7:		TouchConfig.GT308L[0].PWM_Duty.Bit.N7	= bright;	break;
			case 8:		TouchConfig.GT308L[0].PWM_Duty.Bit.N8	= bright;	break;
#endif
#if defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
			case 9:		TouchConfig.GT308L[1].PWM_Duty.Bit.N1	= bright;	break;
			case 10:	TouchConfig.GT308L[1].PWM_Duty.Bit.N2	= bright;	break;
			case 11:	TouchConfig.GT308L[1].PWM_Duty.Bit.N3	= bright;	break;
			case 12:	TouchConfig.GT308L[1].PWM_Duty.Bit.N4	= bright;	break;
			case 13:	TouchConfig.GT308L[1].PWM_Duty.Bit.N5	= bright;	break;
			case 14:	TouchConfig.GT308L[1].PWM_Duty.Bit.N6	= bright;	break;
			case 15:	TouchConfig.GT308L[1].PWM_Duty.Bit.N7	= bright;	break;
			case 16:	TouchConfig.GT308L[1].PWM_Duty.Bit.N8	= bright;	break;
#else
			case 9:		TouchConfig.GT308L[1].PWM_Duty.Bit.N1	= bright;	break;
			case 10:	TouchConfig.GT308L[1].PWM_Duty.Bit.N2	= bright;	break;
			case 11:	TouchConfig.GT308L[1].PWM_Duty.Bit.N3	= bright;	break;
			case 12:	TouchConfig.GT308L[1].PWM_Duty.Bit.N4	= bright;	break;
			case 13:	TouchConfig.GT308L[1].PWM_Duty.Bit.N5	= bright;	break;
			case 14:	TouchConfig.GT308L[1].PWM_Duty.Bit.N6	= bright;	break;
			case 15:	TouchConfig.GT308L[1].PWM_Duty.Bit.N7	= bright;	break;
			case 16:	TouchConfig.GT308L[1].PWM_Duty.Bit.N8	= bright;	break;
#endif
		}
	}
}
/*
uint8_t get_led_output(uint8_t switch_led)
{
	uint8_t	ret = 0;
	
	if(TouchConfig.MAX_TouchChip >= 1)	// GT308L
	{
		switch(switch_led)
		{
			case 1:		ret	= TouchConfig.GT308L[0].PWM_Duty.Bit.N1;	break;
			case 2:		ret	= TouchConfig.GT308L[0].PWM_Duty.Bit.N2;	break;
			case 3:		ret	= TouchConfig.GT308L[0].PWM_Duty.Bit.N3;	break;
			case 4:		ret	= TouchConfig.GT308L[0].PWM_Duty.Bit.N4;	break;
			case 5:		ret	= TouchConfig.GT308L[0].PWM_Duty.Bit.N5;	break;
			case 6:		ret	= TouchConfig.GT308L[0].PWM_Duty.Bit.N6;	break;
			case 7:		ret	= TouchConfig.GT308L[0].PWM_Duty.Bit.N7;	break;
			case 8:		ret	= TouchConfig.GT308L[0].PWM_Duty.Bit.N8;	break;
				
			case 9:		ret	= TouchConfig.GT308L[1].PWM_Duty.Bit.N1;	break;
			case 10:	ret	= TouchConfig.GT308L[1].PWM_Duty.Bit.N2;	break;
			case 11:	ret	= TouchConfig.GT308L[1].PWM_Duty.Bit.N3;	break;
			case 12:	ret	= TouchConfig.GT308L[1].PWM_Duty.Bit.N4;	break;
			case 13:	ret	= TouchConfig.GT308L[1].PWM_Duty.Bit.N5;	break;
			case 14:	ret	= TouchConfig.GT308L[1].PWM_Duty.Bit.N6;	break;
			case 15:	ret	= TouchConfig.GT308L[1].PWM_Duty.Bit.N7;	break;
			case 16:	ret	= TouchConfig.GT308L[1].PWM_Duty.Bit.N8;	break;
		}
	}
	
	return ret;
}
*/

void LedOn(uint8_t	switch_led)
{
	if(pG_State->ETC.LED_Mode == LED_LOW_LEVEL__LIGHT_IS_OFF)		// 전등이 켜지면 LED ON, 전등이 꺼지면 LED 미등(Low level on)
	{
// #if	defined(_ONE_SIZE_BATCH_LIGHT_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_)
		// led_output(switch_led, LED_OFF_POWERSAVE_LEVEL);			//LED_OFF_POWERSAVE_LEVEL->1(미등)
// #else
		if(Gu8_PowerSaving_Tmr != 0)
		{
			led_output(switch_led, LED_ON_NORMAL_LEVEL);			//LED_ON_NORMAL_LEVEL->10
		}
		else
		{
			led_output(switch_led, LED_ON_POWERSAVE_LEVEL);			//LED_ON_POWERSAVE_LEVEL->6
		}
// #endif
	}
	else if(pG_State->ETC.LED_Mode == LED_OFF__LIGHT_IS_ON)			// 전등이 켜지면 LED OFF, 꺼지면 LED ON
	{
		if(Gu8_PowerSaving_Tmr != 0)
		{
			led_output(switch_led, LED_OFF_LEVEL);					//LED_OFF_LEVEL->0
		}
		else									// 절전모드
		{
			led_output(switch_led, LED_OFF_LEVEL);					//LED_OFF_LEVEL->0
		}
	}
	else															// 전등이 켜지면 LED OFF, 꺼지면 LED ON(절전모드에서 미등) LED_OFF__LIGHT_IS_ON_2(일괄소등모델)
	{
#ifdef OFFICE_SET
		led_output(switch_led, LED_ON_LEVEL);					//사무실 설정
#else
		if(Gu8_PowerSaving_Tmr != 0)
		{
			led_output(switch_led, LED_OFF_LEVEL);					//LED_OFF_LEVEL->0
		}
		else									// 절전모드
		{
			led_output(switch_led, LED_OFF_LEVEL);					//LED_OFF_LEVEL->0
		}
#endif
	}
}

void LedOff(uint8_t	switch_led)
{
	if(pG_State->ETC.LED_Mode == LED_LOW_LEVEL__LIGHT_IS_OFF)		// 전등이 켜지면 LED ON, 전등이 꺼지면 LED 미등(Low level on)
	{
// #if	defined(_ONE_SIZE_BATCH_LIGHT_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_)
		// if(Gu8_PowerSaving_Tmr != 0)
		// {
			// led_output(switch_led, LED_ON_NORMAL_LEVEL);			//LED_ON_NORMAL_LEVEL->10
		// }
		// else
		// {
			// led_output(switch_led, LED_ON_POWERSAVE_LEVEL);			//LED_ON_POWERSAVE_LEVEL->6
		// }
// #else
		led_output(switch_led, LED_OFF_POWERSAVE_LEVEL);
// #endif
	}
	else if(pG_State->ETC.LED_Mode == LED_OFF__LIGHT_IS_ON)			// 전등이 켜지면 LED OFF, 꺼지면 LED ON
	{
		if(Gu8_PowerSaving_Tmr != 0)
		{
			led_output(switch_led, LED_ON_LEVEL);					//LED_ON_LEVEL->6 기본 세팅
		}
		else									// 절전모드
		{
			led_output(switch_led, LED_OFF_LEVEL);					//LED_OFF_LEVEL->0
		}
	}
	else															// 전등이 켜지면 LED OFF, 꺼지면 LED ON(절전모드에서 미등) LED_OFF__LIGHT_IS_ON_2(일괄소등모델)
	{
#ifdef OFFICE_SET
		led_output(switch_led, LED_OFF_POWERSAVE_LEVEL);
#else
		if(Gu8_PowerSaving_Tmr != 0)
		{
			led_output(switch_led, LED_ON_LEVEL);					//LED_ON_LEVEL->6
		}
		else									// 절전모드
		{
			led_output(switch_led, LED_ON_LEVEL);					//LED_ON_LEVEL->6		//일괄, 네트워크 스위치 절전모드 없고, 미등없음.
			// led_output(switch_led, LED_ON_POWERSAVE_LOW_LEVEL);		//LED_ON_POWERSAVE_LOW_LEVEL->1
		}
#endif
	}
}

void set_led_flashing_flag(uint8_t switch_led, uint8_t state)
{
	if(switch_led && switch_led < mapping_SWITCH_MAX)
	{
		LED_Flashing_Flag[switch_led-1]	= state;
	}
	else
	{
		printf("ERR : set_led_flashing_flag\n");
	}
}

uint8_t get_led_flashing_flag(uint8_t switch_led)
{
	uint8_t	ret = 0;
	
	if(switch_led && switch_led < mapping_SWITCH_MAX)
	{
		ret	= LED_Flashing_Flag[switch_led-1];
	}
	else
	{
		printf("ERR : get_led_flashing_flag\n");
	}
	
	return ret;
}

// LED 설정
void SET_LED_State(uint8_t switch_led, uint8_t state)
{
	if(switch_led && switch_led < mapping_SWITCH_MAX)
	{
		if(state == OFF || state == LED_OFF)
		{
			/*
			switch(tsn2item(switch_led) 
			{
				mapping_ITEM_BATCH_LIGHT_OFF
				mapping_ITEM_GAS
				mapping_ITEM_ELEVATOR
			}
			*/
			LED_State_Flag[switch_led-1]	= LED_OFF;
		}
		else if(state == ON || state == LED_ON)
		{
			LED_State_Flag[switch_led-1]	= LED_ON;
		}
		else
		{
			LED_State_Flag[switch_led-1]	= state;
		}
	}
	else
	{
		printf("ERR : SET_LED_State(%d)\n", (uint16_t)switch_led);
	}
}

uint8_t GET_LED_State(uint8_t switch_led)
{
	uint8_t	ret = 0;
	
	if(switch_led && switch_led < mapping_SWITCH_MAX)
	{
		ret	= LED_State_Flag[switch_led-1];
	}
	else
	{
		printf("ERR : GET_LED_State %d\n", (uint16_t)switch_led);
	}
	
	return	ret;
}

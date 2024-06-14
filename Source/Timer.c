/************************************************************************************
	Project		: 전자식스위치
	File Name	: Timer.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "Timer.h"
#include "el_switch.h"
#include "led.h"
#include "lcd.h"
#include "Relay.h"
#include "ZeroCrossing.h"
#include "ThreeWay.h"
#include "WDGnBeep.h"
#include "dimming.h"
#include "rs-485.h"
#include "i2c.h"
#include "adc.h"
#include "STPM3x_opt.h"

__IO uint16_t	Gu16_Check_Tmr		= 0;
__IO uint16_t	Gu16_1ms_delay_tmr		= 0;
__IO uint8_t	Gu8_10ms_Tmr			= 0;
__IO uint8_t	Gu8_100ms_Tmr			= 0;
__IO uint8_t	Gu8_1000ms_Tmr			= 0;
__IO uint8_t	Gu8_10ms_Flag			= 0;
//-------------------------------------------------------------------------------------------------------------------------
// 1ms timer
void irq_TIM4_Timer(void)
{
	Gu16_Check_Tmr++;
	
	if(Gu16_1ms_delay_tmr)	Gu16_1ms_delay_tmr--;
	if(Gu8_10ms_Tmr)		Gu8_10ms_Tmr--;
	else
	{
		Gu8_10ms_Tmr	= 10;
		Gu8_10ms_Flag++;
	}
	
	//if(Gu8_ZeroCrossing_Tmr < (uint8_t)255)	Gu8_ZeroCrossing_Tmr++;	// 255ms 까지만 측정
	if(Gu8_ZeroCrossing_Tmr)	Gu8_ZeroCrossing_Tmr--;
	if(Gu8_I2C_1ms_Tmr)
	{
		if(--Gu8_I2C_1ms_Tmr == 0)
		{
#if 1
			//_asm("nop");
			ErrProc(0xFF);
			//Touch_Init();
#endif
		}
	}
	
	//if(Gu16_TMP_Inc_Tmr < (uint16_t)65535)	Gu16_TMP_Inc_Tmr++;
	
	if(Gu8_RS_485_Rx_Tmr)	Gu8_RS_485_Rx_Tmr--;
	if(Gu8_RS_485_Tx_Tmr)	Gu8_RS_485_Tx_Tmr--;
	
	TIM4_ClearITPendingBit(TIM4_IT_Update);
}

extern uint8_t	Gu8_W_Tmr;

void Timer10ms(void)
{
	uint8_t	i, cnt;
	
	static uint16_t c = 0;
	//if(Gu8_10ms_Tmr == 0)
	if(Gu8_10ms_Flag)
	{
		//Gu8_10ms_Tmr	= 10;
		cnt	= Gu8_10ms_Flag;
		Gu8_10ms_Flag	= 0;
		
		for(i=0;i<cnt;i++)
		{
			if(Gu8_100ms_Tmr)					Gu8_100ms_Tmr--;
			else
			{
				Gu8_100ms_Tmr	= 10;
				Timer100ms();
			}
			if(Gu8_1000ms_Tmr)					Gu8_1000ms_Tmr--;
			else
			{
				Gu8_1000ms_Tmr	= 100;
				Timer1000ms();
			}
			if(Gu8_Beep_Tmr)					Gu8_Beep_Tmr--;
#ifdef	_BEEP_STEP_DOWN_
			if(Gu8_Beep_Step_Down_Tmr)			Gu8_Beep_Step_Down_Tmr--;
#endif
			if(Gu8_Beep_Put_Tmr)				Gu8_Beep_Put_Tmr--;
			
			if(Gu8_ThreeWay_ON_Tmr[0])			Gu8_ThreeWay_ON_Tmr[0]--;
			if(Gu8_ThreeWay_OFF_Tmr[0])			Gu8_ThreeWay_OFF_Tmr[0]--;
			if(Gu8_ThreeWay_ON_Tmr[1])			Gu8_ThreeWay_ON_Tmr[1]--;
			if(Gu8_ThreeWay_OFF_Tmr[1])			Gu8_ThreeWay_OFF_Tmr[1]--;
			if(Gu8_LatchRelay_1_Floating_Tmr)	Gu8_LatchRelay_1_Floating_Tmr--;
			if(Gu8_LatchRelay_2_Floating_Tmr)	Gu8_LatchRelay_2_Floating_Tmr--;
			
			if(Gu8_STPM3x_RW_Process_Tmr)		Gu8_STPM3x_RW_Process_Tmr--;
			if(Gu8_STPM3x_Read_Tmr)				Gu8_STPM3x_Read_Tmr--;
			if(Gu8_STPM3x_COMM_Error_Tmr)		Gu8_STPM3x_COMM_Error_Tmr--;
			
			if(Gu8_ZeroCrossing_Clr_Tmr)		Gu8_ZeroCrossing_Clr_Tmr--;
			//if(Gu8_STPM3x_Average_Tmr)			Gu8_STPM3x_Average_Tmr--;
			
			if(Gu8_W_Tmr)						Gu8_W_Tmr--;
#ifdef _KOCOM_PROTOCOL_
			if(Gu8_RS_485_Tx_Add_Tmr)			Gu8_RS_485_Tx_Add_Tmr--;
#endif
			// Dimming_Smooth_Process();
		}
	}
}

void Timer100ms(void)
{
	int i;
		
	for(i=0;i<2;i++)
	{
		if(Gu8_TouchChip_INT[i])
		{
			if(Gu8_TouchChip_INT_Tmr[i] < SETTING_10S_TMR)
			{
				Gu8_TouchChip_INT_Tmr[i]++;
			}
		}
	}
	
	if(Gu8_LED_Flashing_Tmr)	Gu8_LED_Flashing_Tmr--;
	if(Gu8_PowerSaving_Tmr)		Gu8_PowerSaving_Tmr--;
	
	for(i=1;i<mapping_ITEM_MAX;i++)
	{
		if(Gu8_SWITCH_Delay_OFF_Tmr[i])	Gu8_SWITCH_Delay_OFF_Tmr[i]--;
	}
	
	if(Gu8_LCD_ElecLimitCurrent_Tmr)	Gu8_LCD_ElecLimitCurrent_Tmr--;
	if(Gu8_LCD_DIM_Tmr)					Gu8_LCD_DIM_Tmr--;
	if(Gu8_INFO_Disp_Tmr < 255)			Gu8_INFO_Disp_Tmr++;
	
	//if(Gu8_AC_Warring_Tmr)			Gu8_AC_Warring_Tmr--;
	if(Gu8_LED_TEST_Tmr)				Gu8_LED_TEST_Tmr--;
#if defined _NO_PROTOCOL_	 && defined THREEWAY_TRANS
	if(Gu16_Noprotocol_Req_Tmr)			Gu16_Noprotocol_Req_Tmr--;
#endif
#ifdef _KDW_PROTOCOL_
	if(Gu8_ON_Repeat_Tmr)		Gu8_ON_Repeat_Tmr--;
	if(Gu8_OFF_Repeat_Tmr)		Gu8_OFF_Repeat_Tmr--;
	if(Gu8_Elec_ON_Repeat_Tmr)	Gu8_Elec_ON_Repeat_Tmr--;
	if(Gu8_Elec_OFF_Repeat_Tmr)	Gu8_Elec_OFF_Repeat_Tmr--;
#endif
	// if(Gu8_eeprom_diff_store_Tmr)		Gu8_eeprom_diff_store_Tmr--;
#ifdef _KOCOM_PROTOCOL_
	if(Touch_Use_Tmr)				Touch_Use_Tmr--;
#endif
}

void Timer1000ms(void)
{
	if(Gu8_Touch_Check_Tmr)			Gu8_Touch_Check_Tmr--;
	if(Gu8_ElecLimitCurrent_1_Tmr)	Gu8_ElecLimitCurrent_1_Tmr--;
	if(Gu8_ElecLimitCurrent_2_Tmr)	Gu8_ElecLimitCurrent_2_Tmr--;
	if(Gu16_Light_Sleep_tmr)		Gu16_Light_Sleep_tmr--;
	if(Gu16_Touch_Err_Tmr)			Gu16_Touch_Err_Tmr--;
	if(Gu16_Elevator_Tmr)			Gu16_Elevator_Tmr--;
	if(Gu16_GAS_Off_Tmr)			Gu16_GAS_Off_Tmr--;
	if(Gu8_Sleep_Set_Tmr)			Gu8_Sleep_Set_Tmr--;
#ifdef _COMMAX_PROTOCOL_
	if(Gu8_Batch_Toggle_Tmr)		Gu8_Batch_Toggle_Tmr--;
#endif
#ifdef _KOCOM_PROTOCOL_
	if(Batch_Light_Use_Tmr)			Batch_Light_Use_Tmr--;
#endif
#ifdef	_STATE_DIFF_SAVE__	
	eeprom_diff_store_Process();
#endif
}

#define TIM4_PERIOD       124
void TIM_Init(void)
{
	/* TIM4 configuration:
	- TIM4CLK is set to 16 MHz, the TIM4 Prescaler is equal to 128 so the TIM1 counter clock used is 16 MHz / 128 = 125,000 Hz
	- With 125 000 Hz we can generate time base:
	  max time base is 2.048 ms if TIM4_PERIOD = 255 --> (255 + 1) / 125000 = 2.048 ms
	  min time base is 0.016 ms if TIM4_PERIOD = 1   --> (  1 + 1) / 125000 = 0.016 ms
	- In this example we need to generate a time base equal to 1 ms so TIM4_PERIOD = (0.001 * 125000 - 1) = 124 
	- 16MHz / Prescaler / PERIOD = Hz
	  16000000 / 128 / (124+1) = 1000Hz
	*/
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
	
	TIM4_TimeBaseInit(TIM4_Prescaler_128, TIM4_PERIOD);		// fixed CounterMode_Up
	TIM4_ClearFlag(TIM4_FLAG_Update);
	TIM4_ITConfig(TIM4_IT_Update, ENABLE);	// Enable update interrupt
	TIM4_Cmd(ENABLE);
	
#ifdef		_ADC_
	컴파일 error 표시를 위해...(ADC 에서 타이머 2 사용)
#else
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);
#ifdef _SAMSUNG_PROTOCOL_
	TIM2_TimeBaseInit(TIM2_Prescaler_128, TIM2_CounterMode_Up, 219);		// 219, 1.76ms
#else
#ifndef _KOCOM_PROTOCOL_
	TIM2_TimeBaseInit(TIM2_Prescaler_128, TIM2_CounterMode_Up, 62);		//  62, 0.504ms
#endif
#endif
#ifndef _KOCOM_PROTOCOL_
	TIM2_ClearFlag(TIM2_FLAG_Update);
	TIM2_ITConfig(TIM2_IT_Update, ENABLE);	// Enable update interrupt
	//TIM2_Cmd(ENABLE);
#endif
#endif

	printf("TIM Init\n");
}
#ifndef _KOCOM_PROTOCOL_
__IO uint8_t Gu8_RS_485_Enable_Tmr = 0;

void irq_TIM2_Timer(void)
{
	if(TIM2_GetITStatus(TIM2_IT_Update) != RESET)
	{
		Gu8_RS_485_Enable_Tmr	= 0;
		TIM2_Cmd(DISABLE);
		TIM2_ClearITPendingBit(TIM2_IT_Update);
	}
}
#endif
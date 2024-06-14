/************************************************************************************
	Project		: 전자식스위치
	File Name	: WDGnBeep.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "WDGnBeep.h"
#include "timer.h"
#include "Dimming.h"

//#define		_BEEP_STEP_DOWN_		// 실로폰소리 효과

#define		BEEP_PWM_PORT			GPIOD
#define		BEEP_PWM_PIN			GPIO_Pin_0
#define		BEEP_STEP_DONW_PORT		GPIOF
#define		BEEP_STEP_DONW_PIN		GPIO_Pin_7

#define	TIM3_PRESCALER				4
__IO uint16_t Giou16_Beep_Period;
//-----------------------------------------------------------
uint8_t		Gu8_BeepDuty			= 90;
uint16_t	Gu16_BeepFreq			= 2000;
uint8_t		Gu8_BEEP_ON_TIME		= 20;
#ifdef	_BEEP_STEP_DOWN_
uint8_t		Gu8_BEEP_STEP_DOWN_TIME	= 10;
#endif
//-----------------------------------------------------------
#ifdef	_BEEP_STEP_DOWN_
uint8_t	Gu8_Beep_Step_Down_Tmr	= 0;
#endif
uint8_t	Gu8_Beep_Tmr			= 0;
uint8_t	Gu8_Beep_Cnt			= 0;
uint8_t	Gu8_WDG_Boot			= 0;
uint8_t	Gu8_Beep_Put_Tmr		= 0;

BEEP_QUEUE	BeepQueue;
//-------------------------------------------------------------------------------------------------------------------------
uint32_t LSIMeasurment(void);
//-------------------------------------------------------------------------------------------------------------------------
void Beep_WDG_Init(void)
{
	uint32_t	LsiFreq;
	
	CLK_LSICmd(ENABLE);		// Enable LSI clock for Beep & IWDG
	while (CLK_GetFlagStatus(CLK_FLAG_LSIRDY) == RESET);		// Wait for LSI clock to be ready

	BeepQueue.rptr	= 0;
	BeepQueue.wptr	= 0;
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM3, ENABLE);
	
	Giou16_Beep_Period	= PWM_Freq2Period(Gu16_BeepFreq, TIM3_PRESCALER);	// 2Khz
	
	TIM3_TimeBaseInit(TIM3_Prescaler_4, TIM3_CounterMode_Up, Giou16_Beep_Period);
	
	GPIO_Init(BEEP_PWM_PORT, BEEP_PWM_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
	GPIO_Init(BEEP_STEP_DONW_PORT, BEEP_STEP_DONW_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
	//GPIO_ResetBits(BEEP_STEP_DONW_PORT, BEEP_STEP_DONW_PIN);
	
	//TIM3_OC2Init(TIM3_OCMode_PWM2, TIM3_OutputState_Enable, Duty2CCR(50, Giou16_Beep_Period), TIM3_OCPolarity_High, TIM3_OCIdleState_Set);
	TIM3_OC2Init(TIM3_OCMode_PWM2, TIM3_OutputState_Enable, Duty2CCR(Gu8_BeepDuty, Giou16_Beep_Period), TIM3_OCPolarity_Low, TIM3_OCIdleState_Set);
	
	TIM3_CtrlPWMOutputs(ENABLE);
	TIM3_SetCompare2(0);
	TIM3_Cmd(ENABLE);
	
	printf("Beep PWM Init\n");
}
//-----------------------------------------------------------------------------------------------------------
void WWDG_Disable(void)
{
#ifdef	_WWDG_
  WWDG->CR = (uint8_t)~WWDG_CR_WDGA;
#endif
}

void WDG_Config_Set(void)
{
#ifdef	_WWDG_
	/*	Watchdog Window = (WWDG_COUNTER_INIT - 63) * 1 step
	                 = (127 - 63) * (12288 / 16Mhz)
	                 = 49.152 ms	
		Non Allowed Window = (WWDG_COUNTER_INIT - WWDG_Window_Value) * 1 step
	                 =  (127 - 127) * (12288 / 16Mhz) 
	                 =  0.0 ms
	*/
	// 설정가능한 0.0ms ~ 49.152ms
	
	WWDG_Init(WWDG_COUNTER_INIT, WWDG_WINDOW_VALUE);
#else
	IWDG_Enable(); 									// Enable IWDG (the LSI oscillator will be enabled by hardware)
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);	// IWDG timeout equal to 214 ms (the timeout may varies due to LSI frequency dispersion)
	IWDG_SetPrescaler(IWDG_Prescaler_32);			// IWDG configuration: IWDG is clocked by LSI = 38KHz,		4, 8, 16, 32, 64, 128, 256
	
	// IWDG timeout equal to 214.7 ms (the timeout may varies due to LSI frequency dispersion) */
	// IWDG timeout = (RELOAD_VALUE + 1) * Prescaler / LSI  = (254 + 1) * 32 / 38000 = 214.7 ms, //(254 + 1) * 64 / 38000 = 858.9 ms
	IWDG_SetReload((uint8_t)RELOAD_VALUE);	
	
	IWDG_ReloadCounter();							// Reload IWDG counter
#endif
	printf("SET WDG Config\n");
}

void WDG_SetCounter(void)
{
#ifdef	_WWDG_
	if(WWDG->CR&WWDG_CR_WDGA)
	{
		if((WWDG_GetCounter() & 0x7F) < WWDG_WINDOW_VALUE)		// 127
		{
			WWDG_SetCounter(WWDG_COUNTER_INIT);
		}
	}
#else
	IWDG_ReloadCounter();  		// Reload IWDG(Independent watchdog) counter
#endif
}

void WDG_BootModeCheck(void)
{
#ifdef	_WWDG_
	// Check if the MCU has resumed from WWDG reset
	if (RST_GetFlagStatus(RST_FLAG_WWDGF) != RESET)		// 와치독 리셋 후 부팅됨
	{
		Gu8_WDG_Boot	= 1;
		RST_ClearFlag(RST_FLAG_WWDGF);					// Clear WWDGF Flag
		printf("\n\n\nWWDG Reset\n\n\n");
	}
#else
	if (RST_GetFlagStatus(RST_FLAG_IWDGF) != RESET)		// 와치독 리셋 후 부팅됨
	{
		Gu8_WDG_Boot	= 1;
		RST_ClearFlag(RST_FLAG_IWDGF);					// Clear IWDGF Flag
		printf("\n\n\nIWDG Reset\n\n\n");
	}
#endif
}
//-----------------------------------------------------------------------------------------------------------
void SET_BEEP(uint16_t freq, uint8_t duty, uint8_t ontime, uint8_t downtime)
{
	Gu16_BeepFreq			= freq;
	Gu8_BeepDuty			= duty;
	Gu8_BEEP_ON_TIME		= ontime;
#ifdef	_BEEP_STEP_DOWN_
	Gu8_BEEP_STEP_DOWN_TIME	= downtime;
#endif
	Giou16_Beep_Period	= PWM_Freq2Period(Gu16_BeepFreq, TIM3_PRESCALER);
	TIM3_SetAutoreload(Giou16_Beep_Period);
	//TIM3_SetCompare2(Duty2CCR(Gu8_BeepDuty, Giou16_Beep_Period));
}

uint8_t Beep_EmptyQ(void)
{
	uint8_t	ret	= 1;
	if(BeepQueue.wptr != BeepQueue.rptr)	ret	= 0;
	return ret;
}

uint8_t Beep_GetQ(void)
{
	uint8_t	ret	= 0;
	//if(BeepQueue.wptr != BeepQueue.rptr)
	{
		ret	= BeepQueue.buff[BeepQueue.rptr++];
		if(BeepQueue.rptr == BEEP_MAXQ)	BeepQueue.rptr	= 0;
	}
	return ret;
}

void Beep_PutQ(uint8_t data)
{
	if(Gu8_Beep_Put_Tmr	== 0)		// 50ms 이내의 부져는 무시
	{
		BeepQueue.buff[BeepQueue.wptr++]	= data;
		if(BeepQueue.wptr == BEEP_MAXQ)	BeepQueue.wptr	= 0;
		Gu8_Beep_Put_Tmr	= 1;		// 10ms
	}
}

void Beep(uint8_t Flag)
{
	if(Gu8_SystemInit || Flag == FORCE_BEEP)
	{
		if(pG_State->ETC.BeepMode)
		{
			Beep_PutQ(Flag);
		}
	}
}

void BEEP_ON(void)
{
	Gu8_Beep_Tmr			= Gu8_BEEP_ON_TIME;
#ifdef	_BEEP_STEP_DOWN_
	Gu8_Beep_Step_Down_Tmr	= Gu8_BEEP_STEP_DOWN_TIME;
#endif
	GPIO_SetBits(BEEP_STEP_DONW_PORT, BEEP_STEP_DONW_PIN);
	TIM3_SetCompare2(Duty2CCR(Gu8_BeepDuty, Giou16_Beep_Period));
}

void BEEP_OFF(void)
{
	GPIO_ResetBits(BEEP_STEP_DONW_PORT, BEEP_STEP_DONW_PIN);
	TIM3_SetCompare2(0);
}

void Beep_Process(void)
{
	static uint8_t		mode	= 0xFF;
	static uint8_t		beep_mode	= 0;
	
#ifdef	_BEEP_STEP_DOWN_
	if(Gu8_Beep_Step_Down_Tmr == 0)
	{
		GPIO_ResetBits(BEEP_STEP_DONW_PORT, BEEP_STEP_DONW_PIN);
	}
#endif
	
	if(Gu8_Beep_Tmr == 0)
	{
		switch(mode)
		{
			case FORCE_BEEP:
			case BEEP_ONE:
			case ON:
				switch(beep_mode)
				{
					case 0:
						SET_BEEP(833, 50, 5, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 1:
						BEEP_OFF();
						beep_mode = 0;
						mode	= 0xFF;
						break;
				}
				break;
			case BEEP_AC_WARRING:
			case BEEP_TWO:
			case OFF:
				switch(beep_mode)
				{
					case 0:
						SET_BEEP(833, 50, 5, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 1:
						BEEP_OFF();
						beep_mode = 0;
						mode	= 0xFF;
						break;
				}
				break;
			case BEEP_MEL:
				switch(beep_mode)
				{
					/*case 0:
						SET_BEEP(625, 50, 20, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 1:
						SET_BEEP(833, 50, 50, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 2:
						SET_BEEP(263, 50, 130, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;*/
						//도레미 523 587 659
						//도미솔 523 659 784
					case 0:
						SET_BEEP(523, 50, 20, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 1:
						SET_BEEP(659, 50, 20, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 2:
						SET_BEEP(784, 50, 30, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;						
					case 3:
						BEEP_OFF();
						beep_mode = 0;
						mode	= 0xFF;
						break;
				}
				break;
			case BEEP_30MINUTES:
				switch(beep_mode)
				{
					case 0:
						SET_BEEP(833, 50, 5, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 1:
						BEEP_OFF();
						beep_mode = 0;
						mode	= 0xFF;
						break;
				}			
				break;
			case BEEP_60MINUTES:
				switch(beep_mode)
				{
					case 0:
						SET_BEEP(833, 50, 5, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 1:
						SET_BEEP(0, 0, 4, 0);
						BEEP_ON();
						beep_mode++;
						break;						
					case 2:
						SET_BEEP(833, 50, 5, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;						
					case 3:
						BEEP_OFF();
						beep_mode = 0;
						mode	= 0xFF;
						break;
				}			
				break;
			case BEEP_90MINUTES:
				switch(beep_mode)
				{
					case 0:
						SET_BEEP(833, 50, 5, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 1:
						SET_BEEP(0, 0, 4, 0);
						BEEP_ON();
						beep_mode++;
						break;						
					case 2:
						SET_BEEP(833, 50, 5, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 3:
						SET_BEEP(0, 0, 4, 0);
						BEEP_ON();
						beep_mode++;
						break;						
					case 4:
						SET_BEEP(833, 50, 5, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;												
					case 5:
						BEEP_OFF();
						beep_mode = 0;
						mode	= 0xFF;
						break;
				}			
				break;								
				/*
			case BEEP_LONG:
				switch(beep_mode)
				{
					case 0:
						SET_BEEP(833, 50, 100, 0);	// freq, duty, ontime, downtime)
						BEEP_ON();
						beep_mode++;
						break;
					case 1:
						BEEP_OFF();
						beep_mode = 0;
						mode	= 0xFF;
						break;
				}
				break;
				*/
			default:
				if(Beep_EmptyQ() == 0)
				{
					mode	= Beep_GetQ();
				}
				break;
		}
		
	}
}
//-----------------------------------------------------------------------------------------------------------------------------
#ifdef	_LSI_MEASURMENT_

#define LSI_PERIOD_NUMBERS		10
uint16_t IC1ReadValue1 = 0, IC1ReadValue2 = 0;
__IO uint16_t CaptureState	= 0;
__IO uint32_t Capture	= 0;

uint32_t LSIMeasurment(void)
{
	uint8_t icfilter = 0;
	uint32_t LSICurrentPeriod = 0;
	uint32_t LSIMeasuredFrequencyCumul = 0;
	uint32_t LSIMeasuredFrequency = 0;
	uint8_t LSIPeriodCounter = 0;
	
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);
	enableInterrupts();		// Enable Interrupts
	//GPIO_Init(GPIOB, GPIO_Pin_0, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
	
	/* Enable the LSI measurement: LSI clock connected to timer Input Capture 1 */
	BEEP_LSClockToTIMConnectCmd(ENABLE);
	
	/* TIM2 configuration: Input Capture mode */
	/* Configure TIM2 channel 1 in input capture mode */
	/* The signal in input is divided and not filtered */
	/* The capture occurs when a rising edge is detected on TIM2 channel 1 */
	TIM2_ICInit(TIM2_Channel_1, TIM2_ICPolarity_Rising, TIM2_ICSelection_DirectTI, TIM2_ICPSC_DIV8, icfilter);
	
	LSIPeriodCounter = 0;
	/**************************** START of LSI Measurement **********************/
	while (LSIPeriodCounter <= LSI_PERIOD_NUMBERS)
	{
		CaptureState = 1;
		/* Clear all TM2 flags */
		TIM2_GenerateEvent(TIM2_EventSource_Update);
		TIM2->SR1 = 0;
		TIM2->SR2 = 0;
		/* Enable capture 1 interrupt */
		TIM2_ITConfig(TIM2_IT_CC1, ENABLE);
		/* Enable TIM2 */
		TIM2_Cmd(ENABLE);
		
		while (CaptureState != 255);
		
		if (LSIPeriodCounter != 0)
		{
			/* Compute the frequency value */
			LSICurrentPeriod = (uint32_t) 8 * (HSI_VALUE / Capture);
			/* Add the current frequency to previous cumulation */
			LSIMeasuredFrequencyCumul = LSIMeasuredFrequencyCumul + LSICurrentPeriod;
		}
		LSIPeriodCounter++;
	}
	/**************************** END of LSI Measurement ************************/
	
	/* Compute the average of LSI frequency value */
	LSIMeasuredFrequency = LSIMeasuredFrequencyCumul / LSI_PERIOD_NUMBERS;
	
	/* Disconnect LSI clock from Timer 2 channel 1 */
	BEEP_LSClockToTIMConnectCmd(DISABLE);
	disableInterrupts();						// Disable Interrupts
	
	//printf("%ld  %ld\n", LSIMeasuredFrequency, LSIMeasuredFrequencyCumul / LSI_PERIOD_NUMBERS);
	
	TIM2_DeInit();
	
	/* Return the LSI frequency */
	//return (uint16_t)(LSIMeasuredFrequency);
	return LSIMeasuredFrequency;
}

void irq_TIM2_LSIMeasurment(void)
{
	if (TIM2_GetITStatus(TIM2_IT_CC1) != RESET)
	{
		// Clear TIM2 Capture Compare 1 interrupt pending bit
		TIM2_ClearITPendingBit(TIM2_IT_CC1);
		if (CaptureState == 1)
		{
			// Get the Input Capture value
			IC1ReadValue1 = TIM2_GetCapture1();
			CaptureState = 2;
		}
		else if (CaptureState == 2)
		{
			// Get the Input Capture value
			IC1ReadValue2 = TIM2_GetCapture1();
			// Disable TIM2
			TIM2_Cmd(DISABLE);
			TIM2_ITConfig(TIM2_IT_CC1, DISABLE);
			
			// Capture computation
			if (IC1ReadValue2 > IC1ReadValue1)
			{
				Capture = (IC1ReadValue2 - IC1ReadValue1);
			}
			else
			{
				Capture = ((0xFFFF - IC1ReadValue1) + IC1ReadValue2);
			}
			// capture of two values is done
			CaptureState = 255;
		}
	}
}

#endif






/*
// 온음표 2000ms, 2분음표 1000ms, 4분음표 500ms, 8분음표 250ms, 16분음표 125ms
// 음표에 따른 지연시간(ms) = 2000 / x분음표,	즉 2분음표 지연시간 1000(ms) = 2000 / 2

#ifdef	_BEEP_PWM_TEST_

#define	NOTE_C6		1046
#define	NOTE_A6		1760
#define	NOTE_G6		1567
#define	NOTE_F6		1396
#define	NOTE_D6		1244
#define	NOTE_AS6	1864
#define	NOTE_E6		1318
#define	NOTE_C7		2217
#define	NOTE_F6		1396
	
#define	NOTE_REST	0
#define	NOTE_C5		523	//523.2511	5옥타브	C(도)
#define	NOTE_Cs5	554	//554.3653			C#
#define	NOTE_D5		587	//587.3295			D(레)
#define	NOTE_Ds5	622	//622.2540			D#
#define	NOTE_E5		659	//659.2551			E(미)
#define	NOTE_F5		698	//698.4565			F(파)
#define	NOTE_Fs5	740	//739.9888			F#
#define	NOTE_G5		784	//783.9909			G(솔)
#define	NOTE_Gs5	831	//830.6094			G#
#define	NOTE_A5		880	//880.0000			A(라)
#define	NOTE_As5	932	//932.3275			A#
#define	NOTE_B5		988	//987.7666			B(시)

static uint16_t melody[] = 
{
	NOTE_C5, NOTE_D5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_REST,
	NOTE_G5, NOTE_E5, NOTE_D5, NOTE_E5, NOTE_REST,
	NOTE_C5, NOTE_D5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_REST,
	NOTE_G5, NOTE_E5, NOTE_D5, NOTE_E5, NOTE_REST,
	NOTE_D5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_D5,
	NOTE_D5, NOTE_E5, NOTE_D5, NOTE_D5,
	NOTE_G5, NOTE_G5, NOTE_F5, NOTE_F5, NOTE_E5, NOTE_D5,
	NOTE_E5, NOTE_C5, NOTE_C5, NOTE_C5,
	NOTE_G5, NOTE_G5, NOTE_G5,
	NOTE_A5, NOTE_A5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_REST,
	NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_C5, NOTE_D5,
	NOTE_D5, NOTE_REST,
	NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5,
	NOTE_A5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_REST,
	NOTE_E5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_D5,
	NOTE_D5, NOTE_REST,
	NOTE_C5, NOTE_D5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_REST,
	NOTE_G5, NOTE_E5, NOTE_D5, NOTE_E5, NOTE_REST,
	NOTE_C5, NOTE_D5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_E5, NOTE_REST,
	NOTE_G5, NOTE_E5, NOTE_D5, NOTE_E5, NOTE_REST,
	NOTE_D5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_D5,
	NOTE_D5, NOTE_E5, NOTE_D5, NOTE_D5,
	NOTE_G5, NOTE_G5, NOTE_F5, NOTE_F5, NOTE_E5, NOTE_D5,
	NOTE_E5, NOTE_C5, NOTE_C5, NOTE_C5
};

// 온음표 2000ms, 2분음표 1000ms, 4분음표 500ms, 8분음표 250ms, 16분음표 125ms
static uint8_t duration[] =
{
	8, 8, 8, 8, 8, 8, 4,
	4, 4, 8, 8, 4,
	8, 8, 8, 8, 8, 8, 4,
	4, 4, 8, 8, 4,
	8, 8, 8, 8, 2,
	4, 8, 8, 2,
	8, 8, 8, 8, 4, 4,
	8, 4, 8, 2,
	2, 4, 4,
	4, 8, 8, 8, 8, 4,
	8, 8, 8, 8, 8, 8, 8, 8,
	3, 4,
	4, 4, 8, 4, 8,
	4, 4, 8, 8, 4,
	4, 4, 8, 8, 8, 8,
	3, 4,
	8, 8, 8, 8, 8, 8, 4,
	4, 4, 8, 8, 4,
	8, 8, 8, 8, 8, 8, 4,
	4, 4, 8, 8, 4,
	8, 8, 8, 8, 2,
	4, 8, 8, 2,
	8, 8, 8, 8, 4, 4,
	8, 4, 8, 2
};

#define	STEP_DOWN_RATE		0.9		// 90%

#endif

#ifdef	_BEEP_PWM_TEST_
	if(strcmp(argv[1],"mel") == 0)
	{
		for(i=0;i<sizeof(duration);i++)
		{
			if(melody[i] == NOTE_REST)
			{
				BEEP_OFF();
			}
			else
			{
				Giou16_Beep_Period	= PWM_Freq2Period(melody[i], TIM3_PRESCALER);
				TIM3_SetAutoreload(Giou16_Beep_Period);
				
				TIM3_SetCompare2(Duty2CCR(50, Giou16_Beep_Period));
				GPIO_SetBits(BEEP_STEP_DONW_PORT, BEEP_STEP_DONW_PIN);
			}
			
			Gu16_1ms_delay_tmr	= 2000 / duration[i];
			tmp	= (uint16_t)((double)Gu16_1ms_delay_tmr * STEP_DOWN_RATE);
			
			while(Gu16_1ms_delay_tmr)
			{
				if(Gu16_1ms_delay_tmr <= tmp)
				{
					GPIO_ResetBits(BEEP_STEP_DONW_PORT, BEEP_STEP_DONW_PIN);
				}
				WDG_SetCounter();
			}
		}
		GPIO_ResetBits(BEEP_STEP_DONW_PORT, BEEP_STEP_DONW_PIN);
		TIM3_SetCompare2(0);
	}
	else 
#endif

#ifdef	_BEEP_PWM_TEST_
void Beep_Mode(void)
{
	static uint8_t	old_beep_mode = 0;
	
	switch(pG_State->ETC.BeepMode)
	{
		case 0:		// 종소리
			SET_BEEP(635, 95, 43, 5, 10);	// freq, duty, ontime, offtime, downtime)
			break;
		case 1:		// 
			SET_BEEP(635, 95, 43, 5, 10);	// freq, duty, ontime, offtime, downtime)
			break;
		case 2:		// 
			SET_BEEP(1000, 97, 20, 5, 5);
			break;
		case 3:		// 
			SET_BEEP(600, 90, 15, 5, 10);
			break;
		case 4:		// 
			SET_BEEP(0, 90, 15, 5, 10);
			break;
		case 5:		// 테스트 모드(최종 설정모드로 동작)
			break;
		default:
			pG_State->ETC.BeepMode	= old_beep_mode;
			break;
	}
	old_beep_mode	= pG_State->ETC.BeepMode;
}
#endif

void Beep_Process(void)
{
	static uint8_t		mode	= 0xFF;
	static uint8_t		beep_1_mode	= 0;
	static uint8_t		beep_2_mode	= 0;
	static uint8_t		beep_3_mode	= 0;
	static uint8_t		beep_ac_mode	= 0;
	
	if(Gu8_Beep_Step_Down_Tmr == 0)
	{
		GPIO_ResetBits(BEEP_STEP_DONW_PORT, BEEP_STEP_DONW_PIN);
	}
	if(Gu8_Beep_Tmr == 0)
	{
		switch(mode)
		{
			case BEEP_ONE:
			case FORCE_BEEP:
			case ON:
				switch(beep_1_mode)
				{
					case 0:
#ifdef	_BEEP_PWM_TEST_
						Beep_Mode();
#else
						SET_BEEP(635, 95, 43, 5, 10);
#endif
						BEEP_ON();
						beep_1_mode++;
						break;
					case 1:
						BEEP_OFF();
						beep_1_mode = 0;
						mode	= 0xFF;
						break;
				}
				break;
				
			case BEEP_TWO:
			case OFF:
				switch(beep_2_mode)
				{
					case 0:
#ifdef	_BEEP_PWM_TEST_
						Beep_Mode();
						if(Gu8_BEEP_ON_TIME > 20)
						{
							Gu8_BEEP_ON_TIME		= 20;	// 첫음은 200ms로 제한
							Gu8_BEEP_STEP_DOWN_TIME	= 10;
						}
#else
						SET_BEEP(635, 95, 20, 5, 10);
#endif
						BEEP_ON();
						beep_2_mode++;
						break;
					case 1:
#ifdef	_BEEP_PWM_TEST_
						Beep_Mode();
#else
						SET_BEEP(635, 95, 43, 5, 10);
#endif
						BEEP_ON();
						beep_2_mode++;
						break;
					case 2:
						BEEP_OFF();
						beep_2_mode = 0;
						mode	= 0xFF;
						break;
				}
				break;
				
			case BEEP_MEL:
				switch(beep_3_mode)
				{
					case 0:
						if(pG_State->ETC.BeepMode == 1)		SET_BEEP(440, 95, 15, 5, 10);	// freq, duty, ontime, offtime, downtime)
						else								SET_BEEP(880, 95, 15, 5, 10);
						BEEP_ON();
						beep_3_mode++;
						break;
					case 1:
						if(pG_State->ETC.BeepMode == 1)		SET_BEEP(554, 95, 15, 5, 10);
						else								SET_BEEP(1108, 95, 15, 5, 10);
						BEEP_ON();
						beep_3_mode++;
						break;
					case 2:
						if(pG_State->ETC.BeepMode == 1)		SET_BEEP(659, 95, 43, 5, 10);
						else								SET_BEEP(1318, 95, 15, 5, 10);
						BEEP_ON();
						beep_3_mode++;
						break;
					case 3:
						BEEP_OFF();
						beep_3_mode = 0;
						mode	= 0xFF;
						break;
				}
				break;
			
			case BEEP_AC_WARRING:
				switch(beep_ac_mode)
				{
					case 0:
						SET_BEEP(500, 95, 1, 0, 10);
						BEEP_ON();
						beep_ac_mode++;
						break;
					case 1:
					case 3:
					case 5:
						SET_BEEP(Gu16_BeepFreq+100, 95, 1, 1, 10);
						BEEP_ON();
						if(Gu16_BeepFreq > 1800)
						{
							beep_ac_mode++;
						}
						break;
					case 2:
					case 4:
					case 6:
						SET_BEEP(Gu16_BeepFreq-100, 95, 1, 1, 10);
						BEEP_ON();
						if(Gu16_BeepFreq <= 500)
						{
							beep_ac_mode++;
						}
						break;
					case 7:
						BEEP_OFF();
						beep_ac_mode = 0;
						mode	= 0xFF;
						break;
				}
				break;
				
			default:
				if(Beep_EmptyQ() == 0)
				{
					mode	= Beep_GetQ();
					if(Beep_EmptyQ() == 0)			// 출력할 비프가 있으면
					{
						if(Gu8_Beep_Tmr > 20)		// 비프음을 200ms 이하로 제한
						{
							Gu8_Beep_Tmr	= 20;
						}
					}
				}
				break;
		}
		
	}
}
*/
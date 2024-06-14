
#include "header.h"
#include "Beep.h"
#include "WDGnBeep.h"
#include "timer.h"
#include "dimming.h"

#ifdef	_BEEP_PWM_

#define	TIM3_PRESCALER				4
#define	TIM3_REPTETION_COUNTER		0


static uint16_t melody[] = {
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
    static uint8_t duration[] = {
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


__IO uint16_t Giou16_Beep_Period;

uint16_t BEEP_PWM_Freq2Period(uint32_t freq)
{
	uint32_t	ret = 0;
	
	if(freq < 65 || freq > 100000)
	{
		printf("ERR : SET FREQ Value (65Hz ~ 100000Hz), %ld\n", freq);
	}
	else
	{
		ret	= (uint16_t)((16000000/(freq*(TIM3_PRESCALER)))-1);
		//printf("freq %lu period %ld\n", freq, ret);
	}
	return (uint16_t)ret;
}

uint16_t BEEP_Duty2CCR(uint16_t Duty)
{
	uint16_t	ret = 0;
	
	if(Duty > 100)
	{
		printf("ERR : SET Duty Value (0%% ~ 100%%)\n");
	}
	else
	{
		ret	= (uint16_t)((Giou16_Beep_Period)*(Duty*0.01));
		//ret	= ((Giou16_Period+1)*(Duty*0.01))+1;
		//printf("duty %d ccr %d\n", Duty, ret);
	}
	return ret;
}

void BEEP_PWM_Init(void)
{
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM3, ENABLE);
	
	Giou16_Beep_Period	= BEEP_PWM_Freq2Period(1000);	// 1Khz
	
	TIM3_TimeBaseInit(TIM3_Prescaler_4, TIM3_CounterMode_Up, Giou16_Beep_Period);
	
	GPIO_Init(GPIOD, GPIO_Pin_0, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
	GPIO_Init(GPIOE, GPIO_Pin_5, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
	//GPIO_ResetBits(GPIOE, GPIO_Pin_5);
	
	TIM3_OC2Init(TIM3_OCMode_PWM1, TIM3_OutputState_Enable, BEEP_Duty2CCR(50), TIM3_OCPolarity_High, TIM3_OCIdleState_Set);
	
	TIM3_CtrlPWMOutputs(ENABLE);
	TIM3_Cmd(DISABLE);
	
	// printf("Beep PWM Init\n");		//210621
}

#define	STEP_DOWN_RATE		0.5		// 50%

/*
void Beep(uint8_t Flag)
{
	if(Gu8_SystemInit || Flag == FORCE_BEEP)
	{
		switch(Flag)
		{
			case BEEP_THREE:
			case BEEP_AC_WARRING:
				Gu8_Beep_Tmr			= (uint8_t)(BEEP_EN_TMR);
				Gu8_Beep_Step_Down_Tmr	= (uint8_t)((double)BEEP_EN_TMR * STEP_DOWN_RATE);
				Gu8_Beep_Cnt			= BEEP_3;					// beep 3회
				GPIO_SetBits(GPIOE, GPIO_Pin_5);
				TIM3_Cmd(ENABLE);
				break;
				
			case BEEP_ONE:
			case FORCE_BEEP:
			case ON:
				Gu8_Beep_Tmr			= (uint8_t)(BEEP_EN_TMR);
				Gu8_Beep_Step_Down_Tmr	= (uint8_t)((double)BEEP_EN_TMR * STEP_DOWN_RATE);
				Gu8_Beep_Cnt			= BEEP_1;					// beep 1회
				GPIO_SetBits(GPIOE, GPIO_Pin_5);
				TIM3_Cmd(ENABLE);
				break;
				
			case BEEP_TWO:
			case OFF:
				Gu8_Beep_Tmr			= (uint8_t)(BEEP_EN_TMR);
				Gu8_Beep_Step_Down_Tmr	= (uint8_t)((double)BEEP_EN_TMR * STEP_DOWN_RATE);
				Gu8_Beep_Cnt			= BEEP_2;					// beep 2회
				GPIO_SetBits(GPIOE, GPIO_Pin_5);
				TIM3_Cmd(ENABLE);
				break;
		}
	}
}

void Beep_Process(void)
{
	if(Gu8_Beep_Tmr == 0)
	{
		if(Gu8_Beep_Cnt)	Gu8_Beep_Cnt--;
		
		if(Gu8_Beep_Cnt == 0)
		{
			if(BEEP->CSR2&BEEP_CSR2_BEEPEN)
			{
				GPIO_ResetBits(GPIOE, GPIO_Pin_5);
				TIM3_Cmd(DISABLE);
			}
		}
		else if((Gu8_Beep_Cnt%2) == 0)
		{
			Gu8_Beep_Tmr	=	BEEP_DEN_TMR;
			GPIO_ResetBits(GPIOE, GPIO_Pin_5);
			TIM3_Cmd(DISABLE);
		}
		else
		{
			Gu8_Beep_Tmr	= 	BEEP_EN_TMR;
			GPIO_SetBits(GPIOE, GPIO_Pin_5);
			TIM3_Cmd(ENABLE);
		}
	}
	if(Gu8_Beep_Step_Down_Tmr == 0)
	{
		GPIO_ResetBits(GPIOE, GPIO_Pin_5);
	}
}
*/

int Beep_PWM(int argc, char *argv[])
{
	uint16_t	i, vol, j, tmp;
	
	if(strcmp(argv[1],"hi") == 0)
	{
		GPIO_SetBits(GPIOE, GPIO_Pin_5);
	}
	else if(strcmp(argv[1],"lo") == 0)
	{
		GPIO_ResetBits(GPIOE, GPIO_Pin_5);
	}
	else if(strcmp(argv[1],"mel") == 0)
	{
		j = (uint8_t)atoi(argv[2]);
		
		printf("melody, vol %d\n", j);
		
		GPIO_SetBits(GPIOE, GPIO_Pin_5);
		TIM3_Cmd(ENABLE);
		
		for(i=0;i<sizeof(duration);i++)
		{
			if(melody[i] == NOTE_REST)
			{
				TIM3_SetCompare2(0);
			}
			else
			{
				vol = j;
				Giou16_Beep_Period	= BEEP_PWM_Freq2Period(melody[i]);
				TIM3_SetAutoreload(Giou16_Beep_Period);
				TIM3_SetCompare2(BEEP_Duty2CCR(vol));
			}
			
			Gu16_1ms_delay_tmr	= 2000 / duration[i];
			tmp	= (uint16_t)((double)Gu16_1ms_delay_tmr * STEP_DOWN_RATE);
			
			while(Gu16_1ms_delay_tmr)
			{
				if(Gu16_1ms_delay_tmr <= tmp)
				{
					GPIO_ResetBits(GPIOE, GPIO_Pin_5);
				}
				WDG_SetCounter();
			}
		}
		GPIO_ResetBits(GPIOE, GPIO_Pin_5);
		TIM3_Cmd(DISABLE);
	}
	else
	{
		i = (uint8_t)atoi(argv[1]);
		vol = (uint8_t)atoi(argv[2]);
		
		printf("%d, vol %d\n", i, vol);
		
		GPIO_SetBits(GPIOE, GPIO_Pin_5);
		TIM3_Cmd(ENABLE);
		
		Giou16_Beep_Period	= BEEP_PWM_Freq2Period(melody[i]);
		TIM3_SetAutoreload(Giou16_Beep_Period);
		
		if(melody[i] == NOTE_REST)	TIM3_SetCompare2(0);
		else						TIM3_SetCompare2(BEEP_Duty2CCR(vol));
		
		Gu16_1ms_delay_tmr	= 100;
		while(Gu16_1ms_delay_tmr)
		{
			if(Gu16_1ms_delay_tmr <= 50)	GPIO_ResetBits(GPIOE, GPIO_Pin_5);
			WDG_SetCounter();
		}
		GPIO_ResetBits(GPIOE, GPIO_Pin_5);
		TIM3_Cmd(DISABLE);
	}
	
	return 1;
}

#endif


/************************************************************************************
	Project		: ���ڽĽ���ġ
	File Name	: Dimming.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "eeprom.h"
#include "touch.h"
#include "el_switch.h"
#include "dimming.h"
#include "debug.h"

// ȸ�ε� ���� fc = 1/(2��*R*C)	R=1K, C0=1uF
// fc 1591.5497Hz

#define	TIM1_PRESCALER				3
#define	TIM1_REPTETION_COUNTER		0

__IO uint16_t Giou16_Dimming_Period	= 0;		// Dimming
int16_t	Gs16_Dimming1_Setup_Value;
int16_t	Gs16_Dimming1_Currnet_Value;
int16_t	Gs16_Dimming2_Setup_Value;
int16_t	Gs16_Dimming2_Currnet_Value;
int16_t	Gs16_Color_Temp1_Setup_Value;			//210622
int16_t	Gs16_Color_Temp1_Currnet_Value;		//210622
int16_t	Gs16_Color_Temp2_Setup_Value;			//210622
int16_t	Gs16_Color_Temp2_Currnet_Value;		//210622
//-------------------------------------------------------------------------------------------------------------------------
//uint16_t Capture = 0;
void irq_Dimming(void)
{
	;
}
//-------------------------------------------------------------------------------------------------------------------------
void Dimming_Init(void)
{
	if(pG_Config->Enable_Flag.PWM_Dimming)
	{
		CLK_PeripheralClockConfig(CLK_Peripheral_TIM1, ENABLE);
		
		Giou16_Dimming_Period	= PWM_Freq2Period(1000, TIM1_PRESCALER+1);	// 1Khz
		
		// TIM1CLK = 16 MHz
		// TIM1 counter clock = TIM1CLK / TIM1_PRESCALER+1 = 16 MHz/3+1 = 4 MHz
		TIM1_TimeBaseInit(TIM1_PRESCALER, TIM1_CounterMode_Up, Giou16_Dimming_Period, TIM1_REPTETION_COUNTER);
		
		if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_1)
		{
			GPIO_Init(DIMMING_OUT_1_PORT, DIMMING_OUT_1_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
			// TIM1 Channel 1 output frequency = TIM1 counter clock / (TIM1_PERIOD + 1) = 4000000 / (3999 +1) = 1000.0 Hz
			// TIM1 Channel 1 duty cycle = 100 * CCR1_VAL / TIM1_PERIOD + 1 = (2000 * 100) / (3999 + 1) = 50%
			TIM1_OC2Init(TIM1_OCMode_PWM2, TIM1_OutputState_Enable, TIM1_OutputNState_Disable, Duty2CCR(0, Giou16_Dimming_Period+1), 
						TIM1_OCPolarity_Low, TIM1_OCNPolarity_Low, TIM1_OCIdleState_Set, TIM1_OCNIdleState_Set);
		}
		
		if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_2 || pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)
		{
			GPIO_Init(DIMMING_OUT_2_PORT, DIMMING_OUT_2_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
			TIM1_OC3Init(TIM1_OCMode_PWM2, TIM1_OutputState_Enable, TIM1_OutputNState_Disable, Duty2CCR(0, Giou16_Dimming_Period+1), 
					TIM1_OCPolarity_Low, TIM1_OCNPolarity_Low, TIM1_OCIdleState_Set, TIM1_OCNIdleState_Set);
		}
		
		TIM1_CtrlPWMOutputs(ENABLE);	// Enable TIM1 outputs
		TIM1_Cmd(ENABLE);				// TIM1 enable counter
		if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_1)
		{
			Gs16_Dimming1_Setup_Value	= Duty2CCR(DimmingLevelExchange(pG_State->Dimming_Level.Dimming1), Giou16_Dimming_Period+1);						// ������
			// Gs16_Dimming1_Currnet_Value	= Gs16_Dimming1_Setup_Value;			// �ش� ���� ����ϸ� ���� ���� �� ��� ������ ���� ����.
		}
		if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_2)
		{
			Gs16_Dimming2_Setup_Value	= Duty2CCR(DimmingLevelExchange(pG_State->Dimming_Level.Dimming2), Giou16_Dimming_Period+1);						// ������
			// Gs16_Dimming2_Currnet_Value	= Gs16_Dimming2_Setup_Value;			// ���簪
		}
		if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)
		{
			Gs16_Color_Temp1_Setup_Value 	= Duty2CCR(ColorTempLevelExchange(pG_State->Color_Temp_Level.Color_Temp1), Giou16_Dimming_Period+1);				//210622 ���� ���� ������ �ʰ� Giou16_Dimming_Period�� �״�� ���
			// Gs16_Color_Temp1_Currnet_Value	= Gs16_Color_Temp1_Setup_Value;		//210622
		}
		/*if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)
		{
			Gs16_Color_Temp2_Setup_Value 	= Duty2CCR(ColorTempLevelExchange(pG_State->Color_Temp_Level.Color_Temp2), Giou16_Dimming_Period+1);				//210622 ���µ�2�� ��뿩�ΰ� ��Ȯ��
			// Gs16_Color_Temp2_Currnet_Value = Gs16_Color_Temp2_Setup_Value;		//210622
		}*/
		// printf("Dimming Init\n");		//210621
	}
}
//-------------------------------------------------------------------------------------------------------------------------
#define	DIMMING_SMOOTH_STEP		20		// 10ms Ÿ�̸�,	MAX 2���̳� �Ϸ�(�ּ� -> �ִ�)
uint8_t Dimming_Smooth_Step_Func(int16_t Current_Value, int16_t Setup_Value)
{
	int16_t Diff = 0;
	uint8_t Step = 0;
	//1v�� Diff 400 ���̳� ��. 1v - 10v���� �̱� ������ �ִ� 4000 ����
	Diff = abs(Setup_Value - Current_Value);
	Step = (uint8_t)(Diff / 100);
	return Step;
}

void Dimming_Direct_Process(void)
{
	if(Gs16_Dimming1_Setup_Value != Gs16_Dimming1_Currnet_Value)
	{
		Gs16_Dimming1_Currnet_Value = Gs16_Dimming1_Setup_Value;
		TIM1_SetCompare2(Gs16_Dimming1_Currnet_Value);
	}
	if(Gs16_Dimming2_Setup_Value != Gs16_Dimming2_Currnet_Value)
	{
		Gs16_Dimming2_Currnet_Value = Gs16_Dimming2_Setup_Value;
		TIM1_SetCompare3(Gs16_Dimming2_Currnet_Value);
	}

	if(Gs16_Color_Temp1_Setup_Value != Gs16_Color_Temp1_Currnet_Value)
	{
		Gs16_Color_Temp1_Currnet_Value = Gs16_Color_Temp1_Setup_Value;
		TIM1_SetCompare3(Gs16_Color_Temp1_Currnet_Value);
	}
	/*if(Gs16_Color_Temp2_Setup_Value != Gs16_Color_Temp2_Currnet_Value)	//�����δ� ��ְ� ���µ��� �Բ� ���ǹǷ� ���µ��� 2�� ������ �ʴ´ٰ� ��.
	{
		Gs16_Color_Temp2_Currnet_Value = Gs16_Color_Temp2_Setup_Value;
		TIM1_SetCompare3(Gs16_Color_Temp1_Currnet_Value);
	}*/
}

void Dimming_Smooth_Process(void)
{
	uint8_t Step = 40;
	
	if(Gs16_Dimming1_Setup_Value != Gs16_Dimming1_Currnet_Value)	// �������� ���簪�� �ٸ���
	{
		if(Gs16_Dimming1_Setup_Value >= Gs16_Dimming1_Currnet_Value + Step)
		{
			Gs16_Dimming1_Currnet_Value += Step;
		}
		else if(Gs16_Dimming1_Setup_Value <= Gs16_Dimming1_Currnet_Value - Step)
		{
			if(Gs16_Dimming1_Setup_Value == 0)	Gs16_Dimming1_Currnet_Value = 0;
			else
			{
				Gs16_Dimming1_Currnet_Value -= Step;
			}
		}
		else
		{
			Gs16_Dimming1_Currnet_Value	= Gs16_Dimming1_Setup_Value;
		}
		TIM1_SetCompare2(Gs16_Dimming1_Currnet_Value);
		if(G_Trace)	printf("Dimming1_Setup %d, Currnet%d\n", Gs16_Dimming1_Setup_Value, Gs16_Dimming1_Currnet_Value);
	}
	// if(G_Trace)	printf("Dimming1_Setup %d, Currnet%d\n", Gs16_Dimming1_Setup_Value, Gs16_Dimming1_Currnet_Value);
	
	if(Gs16_Dimming2_Setup_Value != Gs16_Dimming2_Currnet_Value)
	{
		if(Gs16_Dimming2_Setup_Value >= Gs16_Dimming2_Currnet_Value + Step)
		{
			Gs16_Dimming2_Currnet_Value += Step;
		}
		else if(Gs16_Dimming2_Setup_Value <= Gs16_Dimming2_Currnet_Value - Step)
		{
			Gs16_Dimming2_Currnet_Value -= Step;
		}
		else
		{
			Gs16_Dimming2_Currnet_Value	= Gs16_Dimming2_Setup_Value;
		}
		TIM1_SetCompare3(Gs16_Dimming2_Currnet_Value);

		if(G_Trace)	printf("Dimming2_Setup %d, Currnet%d\n", Gs16_Dimming2_Setup_Value, Gs16_Dimming2_Currnet_Value);
	}
	
	if(Gs16_Color_Temp1_Setup_Value != Gs16_Color_Temp1_Currnet_Value)	//210622 ���� ���µ� �߰�
	{
		if(Gs16_Color_Temp1_Setup_Value >= Gs16_Color_Temp1_Currnet_Value + Step)
		{
			Gs16_Color_Temp1_Currnet_Value += Step;
		}
		else if(Gs16_Color_Temp1_Setup_Value <= Gs16_Color_Temp1_Currnet_Value - Step)
		{
			Gs16_Color_Temp1_Currnet_Value -= Step;
		}
		else
		{
			Gs16_Color_Temp1_Currnet_Value	= Gs16_Color_Temp1_Setup_Value;
		}
		TIM1_SetCompare3(Gs16_Color_Temp1_Currnet_Value);
		
		// if(G_Trace)	printf("Color_Temp1_Setup %d, Currnet%d\n", Gs16_Color_Temp1_Setup_Value, Gs16_Color_Temp1_Currnet_Value);
	}
	
	/*if(Gs16_Color_Temp2_Setup_Value != Gs16_Color_Temp2_Currnet_Value)
	{
		if(Gs16_Color_Temp2_Setup_Value >= Gs16_Color_Temp2_Currnet_Value + Step)
		{
			Gs16_Color_Temp2_Currnet_Value += Step;
		}
		else if(Gs16_Color_Temp2_Setup_Value <= Gs16_Color_Temp2_Currnet_Value - Step)
		{
			Gs16_Color_Temp2_Currnet_Value -= Step;
		}
		else
		{
			Gs16_Color_Temp2_Currnet_Value	= Gs16_Color_Temp2_Setup_Value;
		}
		TIM1_SetCompare3(Gs16_Color_Temp2_Currnet_Value);
		
		// if(G_Trace)	printf("Color_Temp2_Setup %d, Currnet%d\n", Gs16_Color_Temp2_Setup_Value, Gs16_Color_Temp2_Currnet_Value);
	}*/	
}

uint8_t	DimmingLevelExchange(uint8_t level)
{
	return (uint8_t)(level * 100 / pG_Config->Dimming_MAX_Level);
}


uint8_t ColorTempLevelExchange(uint8_t level)
{
	return (uint8_t)(level * 100 / pG_Config->Color_Temp_MAX_Level);		//210622 �ӽ÷� 5�ܰ� ����
}
//-------------------------------------------------------------------------------------------------------------------------
void PWM_Ctrl(uint8_t pwm, uint8_t Flag)
{
	uint8_t	Level;
	uint8_t	item;
	
	if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_1)
	{
		if(pwm&CONTROL_BIT_DIMMING_1)
		{
			if(Flag == INVERSE)
			{
				item	= ctrl2item(CONTROL_BIT_DIMMING_1);
				Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
			}
			/*
			Level	= DimmingLevelExchange(pG_State->Dimming_Level.Dimming1);
			if(Flag)	TIM1_SetCompare2(Duty2CCR(Level, Giou16_Dimming_Period+1));	// ON
			else		TIM1_SetCompare2(Duty2CCR(0, Giou16_Dimming_Period+1));		// OFF
			*/
			if(Flag)					// ON
			{
				//if(Gs16_Dimming2_Setup_Value != Gs16_Dimming2_Currnet_Value)	Gs16_Dimming1_Currnet_Value	= Gs16_Dimming1_Setup_Value;			// ���簪
				Level	= DimmingLevelExchange(pG_State->Dimming_Level.Dimming1);
				Gs16_Dimming1_Setup_Value	= Duty2CCR(Level, Giou16_Dimming_Period+1);						// ������
			}
			else						// OFF
			{
				//if(Gs16_Dimming2_Setup_Value != Gs16_Dimming2_Currnet_Value)	Gs16_Dimming1_Currnet_Value	= Gs16_Dimming1_Setup_Value;			// ���簪
				Gs16_Dimming1_Setup_Value	= 0;									// ������
			}
		}
	}
	if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_2)
	{
		if(pwm&CONTROL_BIT_DIMMING_2)
		{
			if(Flag == INVERSE)
			{
				item	= ctrl2item(CONTROL_BIT_DIMMING_2);
				Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
			}
			/*
			Level	= DimmingLevelExchange(pG_State->Dimming_Level.Dimming2);
			if(Flag)	TIM1_SetCompare3(Duty2CCR(Level, Giou16_Dimming_Period+1));	// ON
			else		TIM1_SetCompare3(Duty2CCR(0, Giou16_Dimming_Period+1));		// OFF
			*/
			if(Flag)					// ON
			{
				//if(Gs16_Dimming2_Setup_Value != Gs16_Dimming2_Currnet_Value)	Gs16_Dimming2_Currnet_Value	= Gs16_Dimming2_Setup_Value;			// ���簪
				Level	= DimmingLevelExchange(pG_State->Dimming_Level.Dimming2);
				Gs16_Dimming2_Setup_Value	= Duty2CCR(Level, Giou16_Dimming_Period+1);						// ������
			}
			else						// OFF
			{
				//if(Gs16_Dimming2_Setup_Value != Gs16_Dimming2_Currnet_Value)	Gs16_Dimming2_Currnet_Value	= Gs16_Dimming2_Setup_Value;			// ���簪
				Gs16_Dimming2_Setup_Value	= 0;									// ������
			}
		}
	}
	if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)
	{
		if(pwm&CONTROL_BIT_DIMMING_1)
		{
			if(Flag == INVERSE)
			{
				item	= ctrl2item(CONTROL_BIT_DIMMING_2);
				Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
			}
			if(Flag)					// ON
			{
				Level	= ColorTempLevelExchange(pG_State->Color_Temp_Level.Color_Temp1);
				Gs16_Color_Temp1_Setup_Value	= Duty2CCR(Level, Giou16_Dimming_Period+1);						// ������
			}
			else						// OFF
			{
				Gs16_Color_Temp1_Setup_Value	= 0;									// ������
			}
		}
	}
	/*if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)
	{
		if(pwm&CONTROL_BIT_DIMMING_2)
		{
			if(Flag == INVERSE)
			{
				item	= ctrl2item(CONTROL_BIT_DIMMING_2);
				Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
			}
			if(Flag)					// ON
			{
				Level	= ColorTempLevelExchange(pG_State->Color_Temp_Level.Color_Temp2);
				Gs16_Color_Temp2_Setup_Value	= Duty2CCR(Level, Giou16_Dimming_Period+1);						// ������
			}
			else						// OFF
			{
				Gs16_Color_Temp2_Setup_Value	= 0;									// ������
			}
		}
	}*/
	Dimming_Direct_Process();
}

void PWM_Level_Set(uint8_t pwm, uint8_t level_ctrl)
{
	if(((pwm & (CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2)) == (CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2)) && level_ctrl != LEVEL_EQUALL)			// �ΰ� ���ÿ� ������ ���
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)) && GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))
		{
			if(pG_State->Dimming_Level.Dimming1 > pG_State->Dimming_Level.Dimming2)			// �ΰ��� ������ ū���� �������� ����
			{
				pG_State->Dimming_Level.Dimming2	= pG_State->Dimming_Level.Dimming1;
			}
			else
			{
				pG_State->Dimming_Level.Dimming1	= pG_State->Dimming_Level.Dimming2;
			}
		}
	}
	if(pwm & CONTROL_BIT_DIMMING_1)
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))	// ���1 ���� ���� ������
		{
			if(Gu8_Dim_Flag && Gu8_Color_Temp_Flag == 0)				//??
			{
				if(level_ctrl == LEVEL_UP)
				{
					if(pG_State->Dimming_Level.Dimming1 == pG_Config->Dimming_MAX_Level)
					{
						pG_State->Dimming_Level.Dimming1	= (uint8_t)1;
					}
					else
					{
						pG_State->Dimming_Level.Dimming1++;
					}
					/*pG_State->Dimming_Level.Dimming1++;
					if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)
					{
						pG_State->Dimming_Level.Dimming1	= (uint8_t)1;
					}*/
				}
				else if(level_ctrl == LEVEL_DN)
				{
					if(pG_State->Dimming_Level.Dimming1)	pG_State->Dimming_Level.Dimming1--;
					if((uint8_t)pG_State->Dimming_Level.Dimming1 == (uint8_t)0)
					{
						pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;
					}
				}
			}
			else if(Gu8_Color_Temp_Flag && Gu8_Dim_Flag == 0)				//???
			{
				if(level_ctrl == LEVEL_UP)
				{
					if(pG_State->Color_Temp_Level.Color_Temp1 == pG_Config->Color_Temp_MAX_Level)
					{
						pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)1;
					}
					else
					{
						pG_State->Color_Temp_Level.Color_Temp1++;
					}
				}
				else if(level_ctrl == LEVEL_DN)
				{
					if(pG_State->Color_Temp_Level.Color_Temp1)	pG_State->Color_Temp_Level.Color_Temp1--;
					if((uint8_t)pG_State->Color_Temp_Level.Color_Temp1 == (uint8_t)0)
					{
						pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;
					}
					/*if((uint8_t)pG_State->Color_Temp_Level.Color_Temp1 < (uint8_t)1)
					{
						pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;
					}
					else if((uint8_t)pG_State->Color_Temp_Level.Color_Temp1 > (uint8_t)pG_Config->Color_Temp_MAX_Level)		//1���� �ٿ�� ���� 15�� ���� ��� ?���� �߰�
					{
						pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;
					}*/
				}
			}
		}
	}
	if(pwm & CONTROL_BIT_DIMMING_2)
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))	// ���2 ���� ���� ������
		{
			if(Gu8_Dim_Flag && Gu8_Color_Temp_Flag == 0)				//??
			{
				if(level_ctrl == LEVEL_UP)
				{
					if(pG_State->Dimming_Level.Dimming2 == pG_Config->Dimming_MAX_Level)
					{
						pG_State->Dimming_Level.Dimming2	= (uint8_t)1;
					}
					else
					{
						pG_State->Dimming_Level.Dimming2++;
					}
					//Dimming_Level.Dimming�� ũ�Ⱑ bit4�̹Ƿ� �ִ� 15. �׷��� Dimming_MAX_Level�� 15�̻��� ���, �Ʒ� �ڵ�� ���� 15->16�� �Ǿ� 1�� �Ǵ°��� �ƴ϶�
					//15->0���� �ǰ�, �ѹ� �� up�ؾ� ������ 1�� �Ǿ� ���� ���� ������.
					/*pG_State->Dimming_Level.Dimming2++;
					if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)
					{
						pG_State->Dimming_Level.Dimming2	= (uint8_t)1;
					}*/
				}
				else if(level_ctrl == LEVEL_DN)
				{
					if(pG_State->Dimming_Level.Dimming2)	pG_State->Dimming_Level.Dimming2--;
					if((uint8_t)pG_State->Dimming_Level.Dimming2 == (uint8_t)0)
					{
						pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;
					}
				}
			}
			/*else if(Gu8_Color_Temp_Flag && Gu8_Dim_Flag == 0)				//???
			{
				if(level_ctrl == LEVEL_UP)
				{
					pG_State->Color_Temp_Level.Color_Temp2++;
					pG_State->Color_Temp_Level.Color_Temp2++;
					if((uint8_t)pG_State->Color_Temp_Level.Color_Temp2 > (uint8_t)pG_Config->Color_Temp_MAX_Level)
					{
						pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)1;
					}
				}
				else if(level_ctrl == LEVEL_DN)
				{
					if(pG_State->Color_Temp_Level.Color_Temp2)
					{	
						pG_State->Color_Temp_Level.Color_Temp2--;
						pG_State->Color_Temp_Level.Color_Temp2--;
					}
					if((uint8_t)pG_State->Color_Temp_Level.Color_Temp2 < (uint8_t)1)
					{
						pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)pG_Config->Color_Temp_MAX_Level;
					}
					else if((uint8_t)pG_State->Color_Temp_Level.Color_Temp2 > (uint8_t)pG_Config->Color_Temp_MAX_Level)		//1���� �ٿ�� ���� 15�� ���� ��� ?���� �߰�
					{
						pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)pG_Config->Color_Temp_MAX_Level;
					}
				}				
			}*/
		}
	}

}
//-------------------------------------------------------------------------------------------------------------------------
uint16_t PWM_Freq2Period(uint32_t freq, uint32_t prescaler)
{
	uint32_t	ret = 0;
	
	/*
	if(freq < 65 || freq > 100000)
	{
		printf("ERR : SET FREQ Value (65Hz ~ 100000Hz)\n");
	}
	else
	*/
	{
		//ret	= (uint16_t)((16000000/(freq*(prescaler)))-1);
		ret	= (uint16_t)((16000000/(freq*(prescaler))));
		//printf("freq %lu period %ld\n", freq, ret);
	}
	//Freq = 1 / period
	//period = 16000000 / 1000 * 4 = 4000
	//Freq = 1 / 4000 = 0.00025

	return (uint16_t)ret;
}

uint16_t Duty2CCR(uint16_t Duty, uint16_t period)
{
	uint16_t	ret = 0;
	
	if(Duty > 100)
	{
		printf("ERR : SET Duty Value (0%% ~ 100%%)\n");
	}
	else
	{
		ret	= (uint16_t)((period)*(Duty*0.01));	//Duty2CCR(level, preiod), level = 10(���� ���� 1), preiod = 4000�̸�, 4000 * 10 * 0.01 = 400. level = 100(���� ����10), period = 4000�̸�, 4000 * 100 * 0.01 = 4000.
		/*if(Duty < 20)	//level * 10�� ���� 20���� �Ʒ��� �� 
		{
			ret = 500; //1.25V ����
			//1V ���� ret = 400.
		}
		else
		{
			ret	= (uint16_t)((period)*(Duty*0.01));
		}*/
		/*
		if((10 / maxlevel) < 1.25)
		{
			value = 10 / maxlevel * level;
			if(value < 1.25)
			{
				ret = (uint16_t)((period)*(Duty*0.01) + );
			}
		}
		*/
		//ret	= ((Giou16_Dimming_Period+1)*(Duty*0.01))+1;
		//printf("duty %d ccr %d\n", Duty, ret);
	}
	return ret;
}
//DimmingLevelExchange(level)���� level = 1�̸� return�� 10. level = 10�̸�, return�� 100(��, �ƽ� ������ 10�� ���)
#ifndef	_OPTIMIZE_
int Debug_Dimming_Freq(int argc, char *argv[])
{
	uint32_t i = atoi(argv[1]);
	
	if(i)
	{
		if(argv[2][0] == 'k')	i*=1000;
		else if(argv[2][0] == 'm')	i*=1000000;
		Giou16_Dimming_Period	= PWM_Freq2Period(i, TIM1_PRESCALER+1);
		printf("freq %lu period %ld\n", i, Giou16_Dimming_Period);
		TIM1_SetAutoreload(Giou16_Dimming_Period);		// ���ļ� ����
		TIM1_SetCompare2(Duty2CCR(50, Giou16_Dimming_Period+1));			// ��Ƽ ����
	}
	
	return 1;
}
#endif

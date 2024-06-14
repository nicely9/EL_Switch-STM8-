/************************************************************************************
	Project		: 전자식스위치
	File Name	: Touch.C
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
#include "el_switch.h"
#include "led.h"
#include "lcd.h"
#include "WDGnBeep.h"
#include "Relay.h"
#include "Timer.h"
#include "Debug.h"
#include "i2c.h"
#include "STPM3x_opt.h"


TOUCH_CFG			TouchConfig; 
_TOUCH_16_CHANNEL_	TouchState;					// 2AH	ReadOnly
//_TOUCH_16_CHANNEL_	oldTouchState;
uint8_t				Touch_Ignore_Flag[MAX_SWITCH];

TOUCH_CFG			cmpTouchConfig;

uint8_t				Gu8_TouchChip_INT[2];
uint8_t				Gu8_TouchChip_INT_Tmr[2];
uint8_t				Gu8_Sensitivity[8];

uint8_t				Gu8_LightGroup_SET_Flag	= 0;
uint8_t				Gu8_Touch_Check_Tmr	= 3;	// 3sec
uint8_t				Gu8_Touch_SENSITIVITY_Check_Flag[2];
//-------------------------------------------------------------------------------------------------------------------------
void Touch_Chip_Reset(void)
{
	GPIO_SetBits(TOUCH_RESET_PORT, TOUCH_RESET_PIN);
	delay(10);		// 2ms -> 20201111 10ms
	GPIO_ResetBits(TOUCH_RESET_PORT, TOUCH_RESET_PIN);
	delay(10);		// 2ms -> 20201111 10ms
}

void Touch_GPIO_Init(void)
{
#if 1
	GPIO_Init(TOUCH_INT_1_PORT, TOUCH_INT_1_PIN, GPIO_Mode_In_PU_IT);						// Input pull-up, external interrupt
	GPIO_Init(TOUCH_INT_2_PORT, TOUCH_INT_2_PIN, GPIO_Mode_In_PU_IT);						// Input pull-up, external interrupt
	EXTI_SetPinSensitivity((EXTI_Pin_TypeDef)TOUCH_INT_1_EXTI, EXTI_Trigger_Falling);		// EXTI Interrupt
	EXTI_SetPinSensitivity((EXTI_Pin_TypeDef)TOUCH_INT_2_EXTI, EXTI_Trigger_Falling);		// EXTI Interrupt
	//EXTI_SetPinSensitivity((EXTI_Pin_TypeDef)TOUCH_INT_1_EXTI, EXTI_Trigger_Rising_Falling);		// EXTI Interrupt
	//EXTI_SetPinSensitivity((EXTI_Pin_TypeDef)TOUCH_INT_2_EXTI, EXTI_Trigger_Rising_Falling);		// EXTI Interrupt
#else
	//GPIO_Init(TOUCH_INT_1_PORT, TOUCH_INT_1_PIN, GPIO_Mode_In_FL_No_IT);					// Input floating, no external interrupt
	//GPIO_Init(TOUCH_INT_2_PORT, TOUCH_INT_2_PIN, GPIO_Mode_In_FL_No_IT);					// Input floating, no external interrupt
	GPIO_Init(TOUCH_INT_1_PORT, TOUCH_INT_1_PIN, GPIO_Mode_In_PU_No_IT);					// Input pull-up, no external interrupt
	GPIO_Init(TOUCH_INT_2_PORT, TOUCH_INT_2_PIN, GPIO_Mode_In_PU_No_IT);					// Input pull-up, no external interrupt
#endif
	GPIO_Init(TOUCH_RESET_PORT, TOUCH_RESET_PIN, GPIO_Mode_Out_PP_High_Fast);				// Output push-pull, high level, 10MHz
	//GPIO_Init(TOUCH_RESET_PORT, TOUCH_RESET_PIN, GPIO_Mode_Out_PP_Low_Fast);				// Output push-pull, low level, 10MHz
	
	//define SDA, SCL outputs, HiZ, Open drain, Fast
	GPIOC->ODR |= 0x03;               
	GPIOC->DDR |= 0x03;
	GPIOC->CR2 |= 0x03;
	
	Gu8_I2C_1ms_Tmr	= 0;
	enableInterrupts();			// Enable Interrupts
	Touch_Chip_Reset();
	disableInterrupts();		// Disable Interrupts
	
	printf("Touch GPIO Init\n");
}

void Touch_Init(void)
{
	uint16_t i, j;
	uint8_t	switch_enable[2] = {0, 0};
	
	if(pG_Config->Enable_Flag.TOUCH_Chip_Type == 1 || pG_Config->Enable_Flag.TOUCH_Chip_Type == 2)	// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
	{
		memset((void*)&TouchConfig, 0, sizeof(TOUCH_CFG));
		
		TouchConfig.MAX_TouchChip	= pG_Config->Enable_Flag.TOUCH_Chip_Type;
		TouchConfig.Address[0]		= TOUCH_1_ADDR;
		TouchConfig.Address[1]		= TOUCH_2_ADDR;
		
		for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
		{
			if(i <= mapping_SWITCH_8)
			{
				if(tsn2item((uint8_t)i))
				{
					switch_enable[0]	|= (uint8_t)(1<<(i-mapping_SWITCH_1));
					SET_LED_State((uint8_t)i, LED_OFF);
					//printf("1o %d %02x\n", i, (uint16_t)switch_enable[0]);
				}
				//else printf("1x %d %02x\n", i, (uint16_t)switch_enable[0]);
			}
			else
			{
				if(tsn2item((uint8_t)i))
				{
					switch_enable[1]	|= (uint8_t)(1<<(i-mapping_SWITCH_9));
					SET_LED_State((uint8_t)i, LED_OFF);
					//printf("2o %d %02x\n", i, (uint16_t)switch_enable[1]);
				}
				//else printf("2x %d %02x\n", i, (uint16_t)switch_enable[1]);
			}
		}
		
		//printf("\n\n%02x %02x\n\n", (uint16_t)switch_enable[0], (uint16_t)switch_enable[1]);
		
		for(i=0;i<TouchConfig.MAX_TouchChip;i++)
		{
			//---------------------------------------------------------------------------------
			// 기본설정 값
			//TouchConfig.GT308L[i].TS_Enable.Byte	= 0xFF;		// 03H	0xFF	0 = Disable, 1 = Enable
			TouchConfig.GT308L[i].TS_Enable.Byte	= switch_enable[i];		// 03H	0xFF	0 = Disable, 1 = Enable
			TouchConfig.GT308L[i].TouchMode.Byte	= 0x51;		// 04H	0x51	Single touch Mode
			//TouchConfig.GT308L[i].TouchMode.Byte	= 0x55;		// 04H	0x51	Multi touch Mode
			TouchConfig.GT308L[i].PWM_Enable.Byte	= 0xFF;		// 05H	0xFF	0 = Output Mode, 1 = PWM Mode
			
			//TouchConfig.GT308L[i].H07.Byte			= 0x27;		// 07H	0x27	TouchPeriod 3, CalibrationTime 7
			TouchConfig.GT308L[i].H07.Byte			= 0x47;		// 07H	0x47	TouchPeriod 5, CalibrationTime 7		jsyoon 20201012
			
			TouchConfig.GT308L[i].H3A.Byte			= 0x05;		// 3AH	0x05	Pulse Mode, Active Low
			TouchConfig.GT308L[i].H3B.Byte			= 0x30;		// 3BH	0x30	IDLE_Time 30ms, PWM Generation Enable
			
			for(j=0;j<8;j++)
			{
				TouchConfig.GT308L[i].Sensitivity[j]	= pG_Config->GT308L[i].Sensitivity[j];	// 40H ~ 47H	Default High 0x0C, Middle 0x0F, Low 0x18
			}
			for(j=0;j<4;j++)
			{
				TouchConfig.GT308L[i].PWM_Duty.Byte[j]	= 0;	// 48H ~ 4BH
			}
			
			TouchConfig.GT308L[i].H4F.Byte			= 0x20;		// 4FH	0x20
			//---------------------------------------------------------------------------------
			// 변경 할 내용
			TouchConfig.GT308L[i].H3A.Bit.INT_Mode	= 1;		// Interrupt Level Mode
		}
	}
	
	//----------------------------------------------------------------------------
	Gu8_TouchChip_INT[0]		= 0;
	Gu8_TouchChip_INT[1]		= 0;
	Gu8_TouchChip_INT_Tmr[0]	= 0;
	Gu8_TouchChip_INT_Tmr[1]	= 0;
	Gu8_Touch_SENSITIVITY_Check_Flag[0]	= 0;
	Gu8_Touch_SENSITIVITY_Check_Flag[1]	= 0;	
	memset((void*)&TouchState, 0, sizeof(_TOUCH_16_CHANNEL_));
	//memset((void*)&oldTouchState, 0, sizeof(_TOUCH_16_CHANNEL_));
	memset((void*)&Touch_Ignore_Flag, 0, MAX_SWITCH);
	memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
#ifndef	_TOUCH_SENSITIVITY_TEST_
	Touch_Chip_Init();
#endif
	//----------------------------------------------------------------------------
	
   	printf("Touch Init\n");
}

void Touch_Chip_Init(void)
{
	uint16_t i, retry = 0;
	
	if(pG_Config->Enable_Flag.TOUCH_Chip_Type == 1 || pG_Config->Enable_Flag.TOUCH_Chip_Type == 2)	// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
	{
		for(i=0;i<TouchConfig.MAX_TouchChip;i++)
		{
			if(TouchConfigCmp((void*)&cmpTouchConfig.GT308L[i], (void *)&TouchConfig.GT308L[i]) != 0)		// 데이터가 다르면
			{
				retry	= 1;
			}
		}
		if(retry)
		{
			printf("touch chip reset\n");
			Touch_Chip_Reset();
		}
		else
		{
			return;
		}
		

		for(i=0;i<TouchConfig.MAX_TouchChip;i++)
		{
			retry	= 0;
			while(1)
			{
				if(TouchConfigCmp((void*)&cmpTouchConfig.GT308L[i], (void *)&TouchConfig.GT308L[i]) != 0)		// 데이터가 다르면
				{

					while(SET_Touch_Config_Send((void*)&cmpTouchConfig, i) == 0)
					{
						if(Gu16_ERR_Flag & ERR_FLAG_TOUCH_TIMEOUT)
						{
							printf("ERR : touch chip[%d] init Timeout\n", (uint16_t)(i+1));
							break;
						}
					}
					
					if(retry)
					{
						printf("retry ");
						printf("touch chip[%d] init. %d\n", (uint16_t)(i+1), (uint16_t)retry);
						printf("ERR : touch init cmp 0x%04x\n", TouchConfigCmp((void*)&cmpTouchConfig.GT308L[i], (void *)&TouchConfig.GT308L[i]));
						printf("cmp_config = %x, config = %x\n", (uint16_t)cmpTouchConfig.GT308L[i].H3B.Byte,(uint16_t)TouchConfig.GT308L[i].H3B.Byte);
					}
					else
					{
						printf("\ntouch chip[%d] init\n", (uint16_t)(i+1));
					}
					
					retry++;
					if(retry > 5)
					{
						printf("ERR : touch chip[%d] init\n", (uint16_t)(i+1));
						if(i==0)	Gu16_ERR_Flag	|= ERR_FLAG_TOUCH_CHIP_1;		// 통신은 되는데 chip 설정안됨
						if(i==1)	Gu16_ERR_Flag	|= ERR_FLAG_TOUCH_CHIP_2;		// 통신은 되는데 chip 설정안됨
						break;
					}
				}
				else
				{
					Gu16_ERR_Flag	&= (uint16_t)~ERR_FLAG_TOUCH_CHIP_1;
					Gu16_ERR_Flag	&= (uint16_t)~ERR_FLAG_TOUCH_CHIP_2;
					Gu16_ERR_Flag	&= (uint16_t)~ERR_FLAG_TOUCH_TIMEOUT;
					printf("OK\n");
					break;
				}
			}
		}
	}
}

void Touch_Check(void)
{
	if(Gu16_ERR_Flag & ERR_FLAG_TOUCH_CHIP_1 || Gu16_ERR_Flag & ERR_FLAG_TOUCH_CHIP_2 || Gu16_ERR_Flag & ERR_FLAG_TOUCH_TIMEOUT)
	{
		Gu16_ERR_Flag	&= (uint16_t)~ERR_FLAG_TOUCH_CHIP_1;
		Gu16_ERR_Flag	&= (uint16_t)~ERR_FLAG_TOUCH_CHIP_2;
		Gu16_ERR_Flag	&= (uint16_t)~ERR_FLAG_TOUCH_TIMEOUT;
		
		memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
		Touch_Chip_Init();
		if(Gu16_ERR_Flag & ERR_FLAG_TOUCH_CHIP_1 || Gu16_ERR_Flag & ERR_FLAG_TOUCH_CHIP_2 || Gu16_ERR_Flag & ERR_FLAG_TOUCH_TIMEOUT)
		{
			Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
			printf("ERR : Touch_Chip_Init retry(soft reset)\n");
			Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
			while(1);		// reset
		}
	}
	else if(Gu8_Touch_Check_Tmr == 0)
	{
		Gu8_Touch_SENSITIVITY_Check_Flag[0]	= 1;
		Gu8_Touch_SENSITIVITY_Check_Flag[1]	= 1;		
		Gu8_Touch_Check_Tmr	= 3;	// 3sec
		Touch_Chip_Init();
	}
}

// 타임아웃 및 리셋 등 적용할 것(동작 중 터치가 10초 이상 눌려있을 경우 동작될 수 있도록)
uint8_t SET_Touch_Config_Send(TOUCH_CFG *cmpTouchConfig, uint16_t chip)
{
	static uint16_t	mode = 0;
	uint8_t	ret = 0;

#if 1	
 	static uint16_t old_mode = 0xFF;

	if(mode != old_mode)
	{
		WDG_SetCounter();                   // Reload IWDG(Independent watchdog) counter
		printf("%d, ", mode);
	}
	old_mode         = mode;
#endif
	// WDG_SetCounter();  		// Reload IWDG(Independent watchdog) counter
	
	switch(mode)
	{
		case 0:		if(I2C_OPT_WriteRegister(TouchConfig.Address[chip], 0x03, (void *)&TouchConfig.GT308L[chip].TS_Enable.Byte, 3))		mode++;
					break;
		case 1:		if(I2C_OPT_WriteRegister(TouchConfig.Address[chip], 0x07, (void *)&TouchConfig.GT308L[chip].H07.Byte, 1))			mode++;
					break;
		case 2:		if(I2C_OPT_WriteRegister(TouchConfig.Address[chip], 0x3A, (void *)&TouchConfig.GT308L[chip].H3A.Byte, 2))			mode++;
					break;
		case 3:		if(I2C_OPT_WriteRegister(TouchConfig.Address[chip], 0x40, (void *)&TouchConfig.GT308L[chip].Sensitivity[0], 12))	mode++;
					break;
		case 4:		if(I2C_OPT_WriteRegister(TouchConfig.Address[chip], 0x4F, (void *)&TouchConfig.GT308L[chip].H4F.Byte, 1))			mode++;
					break;
		case 5:		if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[chip], 0x03))													mode++;
					break;
		case 6:		if(I2C_OPT_ReadRegister(TouchConfig.Address[chip], (void *)&cmpTouchConfig->GT308L[chip].TS_Enable.Byte, 3))		mode++;
					break;
		case 7:		if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[chip], 0x07))													mode++;
					break;
		case 8:		if(I2C_OPT_ReadRegister(TouchConfig.Address[chip], (void *)&cmpTouchConfig->GT308L[chip].H07.Byte, 1))				mode++;
					break;
		case 9:		if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[chip], 0x3A))													mode++;
					break;
//---------------------------------------------------------------------------------------------------------------------------------------------- 수정한 부분.... 테스트..
		case 10:	if(I2C_OPT_ReadRegister(TouchConfig.Address[chip], (void *)&cmpTouchConfig->GT308L[chip].H3A.Byte, 1))				mode++;
					break;		
		case 11:	if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[chip], 0x3B))													mode++;
					break;
		case 12:	if(I2C_OPT_ReadRegister(TouchConfig.Address[chip], (void *)&cmpTouchConfig->GT308L[chip].H3B.Byte, 1))				mode++;
					break;					
		case 13:	if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[chip], 0x40))													mode++;
					break;					
		case 14:	if(I2C_OPT_ReadRegister(TouchConfig.Address[chip], (void *)&cmpTouchConfig->GT308L[chip].Sensitivity[0], 12))		mode++;
					break;
		case 15:	if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[chip], 0x4F))													mode++;
					break;
		case 16:	if(I2C_OPT_ReadRegister(TouchConfig.Address[chip], (void *)&cmpTouchConfig->GT308L[chip].H4F.Byte, 1))				mode++;
					break;
		case 17:	if(!I2C_OPT_Busy())
					{
						mode	= 0;
						ret	= 1;
					}
					break;
//---------------------------------------------------------------------------------------------------------------------------------------------기존 코드는 아래 주석처리한 부분
		// case 10:	if(I2C_OPT_ReadRegister(TouchConfig.Address[chip], (void *)&cmpTouchConfig->GT308L[chip].H3A.Byte, 2))				mode++;
					// break;					
		// case 11:	if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[chip], 0x40))													mode++;
					// break;
		// case 12:	if(I2C_OPT_ReadRegister(TouchConfig.Address[chip], (void *)&cmpTouchConfig->GT308L[chip].Sensitivity[0], 12))		mode++;
					// break;
		// case 13:	if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[chip], 0x4F))													mode++;
					// break;
		// case 14:	if(I2C_OPT_ReadRegister(TouchConfig.Address[chip], (void *)&cmpTouchConfig->GT308L[chip].H4F.Byte, 1))				mode++;
		// 			break;
		// case 15:	if(!I2C_OPT_Busy())
		// 			{
		// 				mode	= 0;
		// 				ret	= 1;
		// 			}
		// 			break;
		default:	mode	= 0;	break;
	}
	
	return ret;
}

uint16_t TouchConfigCmp(_GT308L_ *cmp1, _GT308L_ *cmp2)
{
	uint16_t	ret = 0;
	
	if(cmp1->TS_Enable.Byte != cmp2->TS_Enable.Byte)		ret	|= 0x0001;
	if(cmp1->TouchMode.Byte != cmp2->TouchMode.Byte)		ret	|= 0x0002;
	if(cmp1->PWM_Enable.Byte != cmp2->PWM_Enable.Byte)		ret	|= 0x0004;
	if(cmp1->H07.Byte != cmp2->H07.Byte)					ret	|= 0x0008;
	if(cmp1->H3A.Byte != cmp2->H3A.Byte)					ret	|= 0x0010;
	if(cmp1->H3B.Byte != cmp2->H3B.Byte)					ret	|= 0x0020;
	if(cmp1->Sensitivity[0] != cmp2->Sensitivity[0])		ret	|= 0x0040;
	if(cmp1->Sensitivity[1] != cmp2->Sensitivity[1])		ret	|= 0x0080;
	if(cmp1->Sensitivity[2] != cmp2->Sensitivity[2])		ret	|= 0x0100;
	if(cmp1->Sensitivity[3] != cmp2->Sensitivity[3])		ret	|= 0x0200;
	if(cmp1->Sensitivity[4] != cmp2->Sensitivity[4])		ret	|= 0x0400;
	if(cmp1->Sensitivity[5] != cmp2->Sensitivity[5])		ret	|= 0x0800;
	if(cmp1->Sensitivity[6] != cmp2->Sensitivity[6])		ret	|= 0x1000;
	if(cmp1->Sensitivity[7] != cmp2->Sensitivity[7])		ret	|= 0x2000;
	/*
	if(cmp1->PWM_Duty.Byte[0] != cmp2->PWM_Duty.Byte[0])	ret	|= 0x4000;
	if(cmp1->PWM_Duty.Byte[1] != cmp2->PWM_Duty.Byte[1])	ret	|= 0x4000;
	if(cmp1->PWM_Duty.Byte[2] != cmp2->PWM_Duty.Byte[2])	ret	|= 0x4000;
	if(cmp1->PWM_Duty.Byte[3] != cmp2->PWM_Duty.Byte[3])	ret	|= 0x4000;
	*/
	//if(cmp1->H4F.Byte != cmp2->H4F.Byte)					ret	|= 0x8000;
	
	return	ret;
}

void TouchSwitch_Event_Ctrl(_TOUCH_16_CHANNEL_ *touch_state, uint8_t tmr)
{
	if(touch_state->Bit.TS1)		TouchSwitch_Action_Check(mapping_SWITCH_1, tmr);
	else if(touch_state->Bit.TS2)	TouchSwitch_Action_Check(mapping_SWITCH_2, tmr);
	else if(touch_state->Bit.TS3)	TouchSwitch_Action_Check(mapping_SWITCH_3, tmr);
	else if(touch_state->Bit.TS4)	TouchSwitch_Action_Check(mapping_SWITCH_4, tmr);
	else if(touch_state->Bit.TS5)	TouchSwitch_Action_Check(mapping_SWITCH_5, tmr);
	else if(touch_state->Bit.TS6)	TouchSwitch_Action_Check(mapping_SWITCH_6, tmr);
	else if(touch_state->Bit.TS7)	TouchSwitch_Action_Check(mapping_SWITCH_7, tmr);
	else if(touch_state->Bit.TS8)	TouchSwitch_Action_Check(mapping_SWITCH_8, tmr);
	else if(touch_state->Bit.TS9)	TouchSwitch_Action_Check(mapping_SWITCH_9, tmr);
	else if(touch_state->Bit.TS10)	TouchSwitch_Action_Check(mapping_SWITCH_10, tmr);
	else if(touch_state->Bit.TS11)	TouchSwitch_Action_Check(mapping_SWITCH_11, tmr);
	else if(touch_state->Bit.TS12)	TouchSwitch_Action_Check(mapping_SWITCH_12, tmr);
	else if(touch_state->Bit.TS13)	TouchSwitch_Action_Check(mapping_SWITCH_13, tmr);
	else if(touch_state->Bit.TS14)	TouchSwitch_Action_Check(mapping_SWITCH_14, tmr);
	else if(touch_state->Bit.TS15)	TouchSwitch_Action_Check(mapping_SWITCH_15, tmr);
	else if(touch_state->Bit.TS16)	TouchSwitch_Action_Check(mapping_SWITCH_16, tmr);
	//else							TouchSwitch_Action_Check(mapping_SWITCH_DISABLE, tmr);

}

void Touch_and_LED_Process(void)
{
	static	uint8_t	i = 0;
	uint8_t	j;
	static _H48_H4B_	old_PWM_Duty[2]	= {0};
	static uint8_t		mode[2]			= {0,0};
	static uint8_t		old_mode[2]		= {0xFF,0xFF};
	static uint8_t		mode_cnt[2]		= {0,0};
	
	Relay_Ctrl_Retry();		// jsyoon 20200925
	Touch_Check();
	
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
	if(pG_State->ETC.LED_Mode != LED_OFF__LIGHT_IS_ON_2)
	{
		Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;		// 5sec,	절전모드 없음
	}
#endif
	
	if(TouchConfig.MAX_TouchChip == 1 || TouchConfig.MAX_TouchChip == 2)	// GT308L
	{
		if(i >= TouchConfig.MAX_TouchChip)	i = 0;
		
		if(old_mode[i] == mode[i])
		{
			mode_cnt[i]++;
			if(mode_cnt[i] >= 200)
			{
				mode_cnt[i]	= 0;
				if(i == 0)	Gu16_ERR_Flag |= ERR_FLAG_TOUCH_CHIP_1;
				else		Gu16_ERR_Flag |= ERR_FLAG_TOUCH_CHIP_2;
				printf("ERR touch mode[%d] %d\n", (uint16_t)i, (uint16_t)mode_cnt[i]);
			}
		}
		else
		{
			mode_cnt[i]	= 0;
		}
		old_mode[i] = mode[i];
		
		switch(mode[i])
		{
			default:
				i	= 0;
				mode[i]	= TOUCH_MODE;
				break;
			//---------------------------------------------------------------------------------------------------------------
			case SENSITIVITY_MODE:
				if(Gu8_TouchChip_INT[i])		// 터치 이벤트가 있으면 터치상태 읽기모드로
				{
					mode[i]	= TOUCH_MODE;
				}
				else if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[i], 0x40))
				{
					mode[i]++;
				}
				break;
			case SENSITIVITY_MODE+1:
				memset((void *)&cmpTouchConfig.GT308L[i].Sensitivity[0], 0, 12);
				//if(I2C_OPT_ReadRegister(TouchConfig.Address[i], (void *)&Gu8_Sensitivity[0], 8))
				if(I2C_OPT_ReadRegister(TouchConfig.Address[i], (void *)&cmpTouchConfig.GT308L[i].Sensitivity[0], 12))
				{
					mode[i]++;
				}
				break;
			case SENSITIVITY_MODE+2:
				if(I2C_OPT_Busy() == 0)		// 데이터 수신완료
				{
					mode[i]++;
					//mode[i]	= TOUCH_MODE;
				}
				break;
			//---------------------------------------------------------------------------------------------------------------
			case SENSITIVITY_MODE+3:
				if(Gu8_TouchChip_INT[i])		// 터치 이벤트가 있으면 터치상태 읽기모드로
				{
					mode[i]	= TOUCH_MODE;
				}
				else if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[i], 0x03))
				{
					mode[i]++;
				}
				break;
			case SENSITIVITY_MODE+4:
				memset((void *)&cmpTouchConfig.GT308L[i].TS_Enable.Byte, 0, 3);
				if(I2C_OPT_ReadRegister(TouchConfig.Address[i], (void *)&cmpTouchConfig.GT308L[i].TS_Enable.Byte, 3))
				{
					mode[i]++;
				}
				break;
			case SENSITIVITY_MODE+5:
				if(I2C_OPT_Busy() == 0)		// 데이터 수신완료
				{
					mode[i]	= TOUCH_MODE;		// 필요시 비교하는 데이터 모두 체크하도록 적용할 것
				}
				break;
			//---------------------------------------------------------------------------------------------------------------
			case TOUCH_MODE:
				if(Gu8_TouchChip_INT[i])		// 터치 이벤트가 있으면 터치상태 읽기모드로
				{
					if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[i], 0x2A))
					{
						mode[i]++;
					}
				}
				else	mode[i]	= LED_PWM_MODE;		// 이벤트가 없으면 LED 제어모드로
				break;
			case TOUCH_MODE+1:
				if(I2C_OPT_ReadRegister(TouchConfig.Address[i], (void *)&TouchState.Byte[i], 1))
				{
					mode[i]++;
				}
				break;
			case TOUCH_MODE+2:
				if(I2C_OPT_Busy() == 0)		// 데이터 수신완료
				{
					//--------------------------------------------------------------------------------
					TouchSwitch_Event_Ctrl((void *)&TouchState.Byte[0], Gu8_TouchChip_INT_Tmr[i]);	// 제어
					//printf("%02x %02x\n", (uint16_t)TouchState.Byte[0], (uint16_t)TouchState.Byte[1]);
					//--------------------------------------------------------------------------------
					
					if(i == 0)
					{
						if(GPIO_ReadInputDataBit(TOUCH_INT_1_PORT, (GPIO_Pin_TypeDef)TOUCH_INT_1_PIN) == TOUCH_INT_1_PIN)		// 터치 인터럽트가 해제되었으면(High)
						{
							// 즉시 실행이 아닐경우 특수기 실행시간이 지난후에는 터치 정보가 사라지므로 TouchSwitch_Action_Check()가 실행되지 않으므로 터치 종료 후 실행
							Special_Function_Key_Process(Gu8_Special_Function_Key, Gu8_Special_Function_Key_Tmr);
							for(j=mapping_SWITCH_1;j<=mapping_SWITCH_8;j++)
							{
								SET_Touch_Ignore_Flag(j, IGNORE_CLEAR);
								if(GET_LED_State(j) == LED_DETECT_ON)
								{
#if defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) && defined(COMM_THREEWAY) && defined(_HYUNDAI_PROTOCOL_)
									if(item2tsn(mapping_ITEM_3WAY_1))	SET_LED_State(j, LED_ON);	//일괄스위치 현대 프로토콜에서 삼로 기능 사용할 때는 DETECT_ON은 평소 OFF 였다가 누를 때 켜짐.
									else								SET_LED_State(j, LED_OFF);	//그 외의 경우는 다른 모델, 기능과 동일하게 동작. 평소 LED ON, 누르면 LED OFF
#elif defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) && defined(_KOCOM_PROTOCOL_) && defined(WIRING_THREEWAY)
									if(item2tsn(mapping_ITEM_3WAY_1))	SET_LED_State(j, LED_ON);
									else								SET_LED_State(j, LED_OFF);
#else
									SET_LED_State(j, LED_OFF);
#endif
								}
							}
							Gu8_TouchChip_INT_Tmr[i]	= 0;
							Gu8_TouchChip_INT[i]		= 0;		// 터치 인터럽트 후 최소 한번의 데이터를 읽은 후 클리어
							TouchState.Byte[i]			= 0;
						}
					}
					else if(GPIO_ReadInputDataBit(TOUCH_INT_2_PORT, (GPIO_Pin_TypeDef)TOUCH_INT_2_PIN) == TOUCH_INT_2_PIN)	// 터치 인터럽트가 해제되었으면(High)
					{
						// 즉시 실행이 아닐경우 특수기 실행시간이 지난후에는 터치 정보가 사라지므로 TouchSwitch_Action_Check()가 실행되지 않으므로 터치 종료 후 실행
						Special_Function_Key_Process(Gu8_Special_Function_Key, Gu8_Special_Function_Key_Tmr);
						
						for(j=mapping_SWITCH_9;j<=mapping_SWITCH_16;j++)
						{
							SET_Touch_Ignore_Flag(j, IGNORE_CLEAR);
							if(GET_LED_State(j) == LED_DETECT_ON)
							{
								SET_LED_State(j, LED_OFF);
							}
						}
						Gu8_TouchChip_INT_Tmr[i]	= 0;
						Gu8_TouchChip_INT[i]		= 0;		// 터치 인터럽트 후 최소 한번의 데이터를 읽은 후 클리어
						TouchState.Byte[i]			= 0;
					}
					mode[i]	= LED_PWM_MODE;		// 터치가 감지되고 있는 동안은 계속
				}
				break;
			//---------------------------------------------------------------------------------------------------------------
			case LED_PWM_MODE:
				//--------------------------------------------------------------------------------
				LED_Output_Process();
				//--------------------------------------------------------------------------------
				if(memcmp((void*)&old_PWM_Duty[i].Byte[0],	(void *)&TouchConfig.GT308L[i].PWM_Duty.Byte[0],	4) != 0)		// 데이터가 다르면
				{
					if(I2C_OPT_WriteRegister(TouchConfig.Address[i], 0x48, (void *)&TouchConfig.GT308L[i].PWM_Duty.Byte[0], 4))
					{
						mode[i]++;
					}
				}
				else
				{
					if(Gu8_Touch_SENSITIVITY_Check_Flag[i] == 1)
					{
						Gu8_Touch_SENSITIVITY_Check_Flag[i]	= 0;
						mode[i]	= SENSITIVITY_MODE;
					}
					else
					{
						mode[i]	= TOUCH_MODE;
					}
					i++;
				}
				break;
			case LED_PWM_MODE+1:
				if(I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[i], 0x48))
				{
					mode[i]++;
				}
				break;
			case LED_PWM_MODE+2:
				if(I2C_OPT_ReadRegister(TouchConfig.Address[i], (void *)&old_PWM_Duty[i], 4))
				{
					mode[i]++;
				}
				break;
			case LED_PWM_MODE+3:
				if(I2C_OPT_Busy() == 0)		// 데이터 수신완료
				{
					if(Gu8_Touch_SENSITIVITY_Check_Flag[i] == 1)
					{
						Gu8_Touch_SENSITIVITY_Check_Flag[i]	= 0;
						mode[i]	= SENSITIVITY_MODE;
					}
					else
					{
						mode[i]	= TOUCH_MODE;
					}
					i++;
				}
				break;
			//---------------------------------------------------------------------------------------------------------------
		}
	}
	//---------------------------------------------------------------------------------------------------------------
	if(Gu8_PowerSaving_Tmr == 0)			// 절전모드로 진입하면
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_SETUP)))	// 설정 중이면
		{
			//SET_LED_State(item2tsn(mapping_ITEM_SETUP), LED_OFF);
			EventCtrl(item2tsn(mapping_ITEM_SETUP), OFF);
		}
#ifdef _TWO_SIZE_LIGHT_MODEL_		
		if(Gu8_LightGroup_SET_Flag)			// 설정중이던 그룹 해제
		{
			if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_GROUP)))
			{
				SET_LED_State(item2tsn(mapping_ITEM_LIGHT_GROUP), LED_ON);
				Beep(BEEP_TWO);
			}
			else
			{
				SET_LED_State(item2tsn(mapping_ITEM_LIGHT_GROUP), LED_OFF);
			}
		}
#endif
#if defined(_ONE_SIZE_LIGHT_1_n_SLEEP_) || defined(_ONE_SIZE_LIGHT_2_n_SLEEP_)	
		if(GET_LED_State(item2tsn(mapping_ITEM_SLEEP)) == LED_FLASHING)		//기상/취침 설정중이면
		{
			// EventCtrl(item2tsn(mapping_ITEM_SLEEP), OFF);
			SET_LED_State(item2tsn(mapping_ITEM_SLEEP), OFF);					//LED ON
			Beep(BEEP_MEL);
		}
#endif		
		Gu8_LightGroup_SET_Flag	= 0;		// 설정 중이던 모드 해제
	}
	//---------------------------------------------------------------------------------------------------------------
	if((Gu16_Touch_Err_Tmr == 0) && (Gu8_Touch_Err_Flag == 1))
	{
		printf("Touch_Err Tmr, Cnt reset\r\n");
		Gu8_Touch_Err_Flag = 0;
		Gu8_Touch_Err_Cnt = 0;
	}
}
//--------------------------------------------------------------------------------------------------------------
void SET_Touch_Ignore_Flag(uint8_t switch_led, uint8_t state)
{
	if(switch_led && switch_led < mapping_SWITCH_MAX)
	{
		Touch_Ignore_Flag[switch_led-1]	= state;
	}
	else
	{
		printf("ERR : SET_Touch_Ignore_Flag\n");
	}
}

uint8_t GET_Touch_Ignore_Flag(uint8_t switch_led)
{
	uint8_t	ret = 0;
	
	if(switch_led && switch_led < mapping_SWITCH_MAX)
	{
		ret	= Touch_Ignore_Flag[switch_led-1];
	}
	else
	{
		printf("ERR : GET_Touch_Ignore_Flag\n");
	}
	
	return ret;
}
//--------------------------------------------------------------------------------------------------------------
void irq_Touch_1(void)				// EXTI_Pin_2
{
	//if(GPIO_ReadInputDataBit(TOUCH_INT_1_PORT, (GPIO_Pin_TypeDef)TOUCH_INT_1_PIN) == 0)	// GPIO Read
	{
		//Debug_Put('1');
		
		Gu8_TouchChip_INT[0]	= 1;
		EXTI_ClearITPendingBit(EXTI_IT_Pin2);
	}
}

void irq_Touch_2(void)				// EXTI_Pin_3
{
	//if(GPIO_ReadInputDataBit(TOUCH_INT_2_PORT, (GPIO_Pin_TypeDef)TOUCH_INT_2_PIN) == 0)	// GPIO Read
	{
		//Debug_Put('2');
		
		Gu8_TouchChip_INT[1]	= 1;
		EXTI_ClearITPendingBit(EXTI_IT_Pin3);
	}
}
//--------------------------------------------------------------------------------------------------------------


/*
void touchchiptest(void){
	memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
}		//20201112

void touchsend(void){
	uint8_t chipcnt;
	for(chipcnt = 0; chipcnt < 2; chipcnt++){
		while(SET_Touch_Config_Send((void*)&cmpTouchConfig, chipcnt) == 0);
	}
}		//20201112
*/
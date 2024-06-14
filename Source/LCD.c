/************************************************************************************
	Project		: 전자식스위치
	File Name	: LCD.C
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
#include "lcd.h"
#include "RS-485.h"
#include "el_switch.h"
#include "STPM3x_opt.h"
#include "WDGnBeep.h"

uint8_t	Gu8_INFO_Disp_Tmr	= 0;

uint8_t	Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 0;
uint8_t	Gu8_LCD_ElecLimitCurrent_Tmr			= 0;
uint8_t	Gu8_LCD_DIM_Tmr			= 0;
uint8_t Gu8_Back_Light_Flag;
_TN_LCD_SEG_	TN_Lcd;
uint16_t		Gu16_LCD_Watt_1	= 0;
uint16_t		Gu16_LCD_Watt_2	= 0;

#ifdef _ONE_SIZE_LIGHT_n_ELEC_MODEL_
	#define MAX_LCD_BUF 8
#else
	#define MAX_LCD_BUF 8
#endif

// #define LCD_SEG_TEST		//1개용 대기스위치 SEG 테스트용도

void TN_LCD_Write(void);
void BACK_LIGHT_Init(void);

void TN_LCD_Test(void)
{
	uint16_t	i, k, m = 0;
	uint8_t		buff[6] = {0,0,0,0,0,0};
	
	if(pG_Config->Enable_Flag.LCD_Type)
	{
		for(i=0;i<MAX_LCD_BUF;i++)	TN_Lcd.TN_LCD_Buff[i]	= 0;
		if(Gu8_WDG_Boot == 0)
		{
			k = 0x9999;
			for(i=0;i<10;i++)
			{
				sprintf(buff, "%4x", k);
				LCD_GLASS_DisplayString(buff);
				
				//m = m ? 0 : 1;
				m	= 1;
				TN_Lcd.Bit.Icon_ELEC_1	= m;
				TN_Lcd.Bit.Icon_ELEC_2	= m;
				TN_Lcd.Bit.Icon_Auto1	= m;
				TN_Lcd.Bit.Icon_Auto2	= m;
				TN_Lcd.Bit.Icon_W		= m;
#ifndef	_ONE_SIZE_LIGHT_n_ELEC_MODEL_
				TN_Lcd.Bit.Icon_DIM		= m;
#endif
				
				TN_LCD_Write();
				
				delay(200);
				k	-= 0x1111;
			}
			
			for(i=0;i<8;i++)	TN_Lcd.TN_LCD_Buff[i]	= 0;
		}
	}
}
void TN_LCD_Init(void)
{
	if(pG_Config->Enable_Flag.LCD_Type)
	{
		CLK_PeripheralClockConfig(CLK_Peripheral_RTC, ENABLE);
		CLK_PeripheralClockConfig(CLK_Peripheral_LCD, ENABLE);
		CLK_RTCClockConfig(CLK_RTCCLKSource_LSI, CLK_RTCCLKDiv_1);
		
		// LCD frequency = CLKprescaler/31
		LCD_Init(LCD_Prescaler_1, LCD_Divider_31, LCD_Duty_1_4, LCD_Bias_1_3, LCD_VoltageSource_Internal);
		
		//LCD_PortMaskConfig(LCD_PortMaskRegister_0,	0x00);	// SEG00 ~ SEG07
		LCD_PortMaskConfig(LCD_PortMaskRegister_1,	0xFF);		// SEG15 ~ SEG08	Enable(11111111)
		LCD_PortMaskConfig(LCD_PortMaskRegister_2,	0x01);		// SEG23 ~ SEG16	Enable(00000001)
		//현재 회로의 포트와 핀이 연결된 SEG는 SEG8 ~ SEG16(회로도 상에는 SEG1 ~ SEG9로 표기)
#ifdef _ONE_SIZE_LIGHT_n_ELEC_MODEL_
		LCD_ContrastConfig(LCD_Contrast_3V0);					// To set contrast to mean value(2V6 ~ 3V3)
#else
		LCD_ContrastConfig(LCD_Contrast_3V0);					// To set contrast to mean value(2V6 ~ 3V3)
#endif
		
		LCD_DeadTimeConfig(LCD_DeadTime_0);						// 0 ~ 7phase periods dead time
		LCD_PulseOnDurationConfig(LCD_PulseOnDuration_1);		// Pulse ON Duration(0~7)
		
		LCD_Cmd(ENABLE);
		
		printf("LCD Init\n");		//210621
	}
}

void BACK_LIGHT_Init(void)
{
	if(pG_Config->Enable_Flag.Back_Light)
	{
		GPIO_Init(BACK_LIGHT_PORT, BACK_LIGHT_PIN, GPIO_Mode_Out_PP_High_Fast);
		Gu8_Back_Light_Flag = 1;
		printf("BACK LIGHT Init\r\n");
	}
}

void BACK_LIGHT_Process(void)
{
	if(pG_Config->Enable_Flag.Back_Light)
	{
		if(Gu8_PowerSaving_Tmr)
		{
			if(Gu8_Back_Light_Flag)
			{
				GPIO_SetBits(BACK_LIGHT_PORT, BACK_LIGHT_PIN);
				Gu8_Back_Light_Flag = 0;
				// printf("B_Light ON\r\n");
			}
		}
		else
		{
			if(Gu8_Back_Light_Flag == 0)
			{
				GPIO_ResetBits(BACK_LIGHT_PORT, BACK_LIGHT_PIN);
				Gu8_Back_Light_Flag = 1;
				// printf("B_Light OFF\r\n");
			}
		}
	}
	// GPIO_ResetBits(BACK_LIGHT_PORT, BACK_LIGHT_PIN);	//백라이트 제거 필요하면 위 if문 주석처리하고 해당 코드 주석 해제.
}

void TN_LCD_Write(void)
{
#ifdef _ONE_SIZE_LIGHT_n_ELEC_MODEL_
	LCD->RAM[LCD_RAMRegister_1]		= TN_Lcd.TN_LCD_Buff[0];		// COM0 SEG15 ~ SEG08	Enable(11111111)
	LCD->RAM[LCD_RAMRegister_2]		= TN_Lcd.TN_LCD_Buff[1];		// COM0 SEG23 ~ SEG16	Enable(00000001)
	LCD->RAM[LCD_RAMRegister_4]		= TN_Lcd.TN_LCD_Buff[2];		// COM1 SEG11 ~ SEG04	Enable(11110000)
	LCD->RAM[LCD_RAMRegister_5]		= TN_Lcd.TN_LCD_Buff[3];		// COM1 SEG19 ~ SEG12	Enable(00001111)
	LCD->RAM[LCD_RAMRegister_8]		= TN_Lcd.TN_LCD_Buff[4];		// COM2 SEG15 ~ SEG08	Enable(11111111)
	// LCD->RAM[LCD_RAMRegister_9]		= TN_Lcd.TN_LCD_Buff[5];		// COM2 SEG23 ~ SEG16	Enable(00000000)
	LCD->RAM[LCD_RAMRegister_11]	= TN_Lcd.TN_LCD_Buff[5];		// COM3 SEG11 ~ SEG04	Enable(11110000)
	LCD->RAM[LCD_RAMRegister_12]	= TN_Lcd.TN_LCD_Buff[6];		// COM3 SEG19 ~ SEG12	Enable(00001111)
#else
	LCD->RAM[LCD_RAMRegister_1]		= TN_Lcd.TN_LCD_Buff[0];		// COM0 SEG15 ~ SEG08	Enable(11111111)
	LCD->RAM[LCD_RAMRegister_2]		= TN_Lcd.TN_LCD_Buff[1];		// COM0 SEG23 ~ SEG16	Enable(00000001)
	LCD->RAM[LCD_RAMRegister_4]		= TN_Lcd.TN_LCD_Buff[2];		// COM1 SEG11 ~ SEG04	Enable(11110000)
	LCD->RAM[LCD_RAMRegister_5]		= TN_Lcd.TN_LCD_Buff[3];		// COM1 SEG19 ~ SEG12	Enable(00011111)
	LCD->RAM[LCD_RAMRegister_8]		= TN_Lcd.TN_LCD_Buff[4];		// COM2 SEG15 ~ SEG08	Enable(11111111)
	LCD->RAM[LCD_RAMRegister_9]		= TN_Lcd.TN_LCD_Buff[5];		// COM2 SEG23 ~ SEG16	Enable(00000001)
	LCD->RAM[LCD_RAMRegister_11]	= TN_Lcd.TN_LCD_Buff[6];		// COM3 SEG11 ~ SEG04	Enable(11110000)
	LCD->RAM[LCD_RAMRegister_12]	= TN_Lcd.TN_LCD_Buff[7];		// COM3 SEG19 ~ SEG12	Enable(00011111)
#endif
}

void LCD_TEST(int argc, char *argv[])
{
	uint16_t k = 0;
	uint8_t buff[4] = {0, 0, 0, 0};
	printf("argv = %x\r\n", (uint16_t)atoi(argv[1]));
	k = atoi(argv[1]);
	sprintf(buff, "%4d", k);
	// TN_Lcd.Bit.Icon_ELEC_1	= 1;
	// TN_Lcd.Bit.Icon_ELEC_2	= 1;
	// TN_Lcd.Bit.Icon_Auto1	= 1;
	// TN_Lcd.Bit.Icon_Auto2	= 1;
	// TN_Lcd.Bit.Icon_W		= 1;	
	LCD_GLASS_DisplayString(buff);
	TN_LCD_Write();
}

void TN_LCD_Process(void)
{
	uint16_t	LCD_Watt = 0;
	uint8_t		buff[6] = {0,0,0,0,0,0};
	uint8_t		item = 0;
	static uint16_t value = 0;

	if(pG_Config->Enable_Flag.LCD_Type)
	{
		BACK_LIGHT_Process();
		if(Gu8_PowerSaving_Tmr == 0)
		{
			switch(pG_State->ETC.LED_Mode)
			{
				case LED_OFF__LIGHT_IS_ON:
				case LED_OFF__LIGHT_IS_ON_2:
					LCD_Cmd(DISABLE);
					break;
			}
		}
		else if((LCD->CR3 & LCD_CR3_LCDEN) != LCD_CR3_LCDEN)
		{
			//TN_LCD_Init();
			LCD_Cmd(ENABLE);
		}
		//if(1)	// 변화가 있으면
		{
			//memset(TN_Lcd, 0, sizeof(_TN_LCD_SEG_));
			
			TN_Lcd.Bit.Icon_ELEC_1	= GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1));
			TN_Lcd.Bit.Icon_ELEC_2	= GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2));
			TN_Lcd.Bit.Icon_Auto1	= pG_State->ETC.Auto1;
			TN_Lcd.Bit.Icon_Auto2	= pG_State->ETC.Auto2;
			
			if(Gu8_LCD_ElecLimitCurrent_Tmr == 0)
			{
				if(Gu8_Special_Function_Key)	item	= tsn2item(Gu8_Special_Function_Key);
				
				switch(Gu8_LCD_ElecLimitCurrent_Flashing_Flag)
				{
					case 1:		case 3:		case 5:
					case 11:	case 13:	case 15:
						if(Gu8_LCD_ElecLimitCurrent_Flashing_Flag < 10)
						{
							//sprintf(buff, "%4d", Gu16_ElecLimitCurrent_1);
							sprintf(buff, "%4d", Gu16_LCD_Watt_1);
						}
						else
						{
							//sprintf(buff, "%4d", Gu16_ElecLimitCurrent_2);
							sprintf(buff, "%4d", Gu16_LCD_Watt_2);
						}
						
						LCD_GLASS_DisplayString((void*)&buff);
						Gu8_LCD_ElecLimitCurrent_Tmr	= 5;	//500ms ON
						Gu8_LCD_ElecLimitCurrent_Flashing_Flag++;
						if(Gu8_LCD_ElecLimitCurrent_Flashing_Flag == 6 || Gu8_LCD_ElecLimitCurrent_Flashing_Flag == 16)
						{
							Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 0;
						}
						break;
					case 2:		case 4:
					case 12:	case 14:
						LCD_GLASS_DisplayString(0);
						Gu8_LCD_ElecLimitCurrent_Tmr	= 5;	//500ms OFF
						Gu8_LCD_ElecLimitCurrent_Flashing_Flag++;
						break;
						
					default:
						if(Gu8_INFO_Disp_Tmr <= 20)				// 부팅 후 2초간 버젼표시
						{
							sprintf(buff, "V%3d", VERSION);
							Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
						}
						else if(Gu8_INFO_Disp_Tmr <= 40)		// 부팅 후 2초간 RS-485 ID 출력
						{
							sprintf(buff, "A%3d", (uint16_t)pG_Config->RS485_ID);
							//sprintf(buff, "%02x %1d", (uint16_t)Get_485_ID(), (uint16_t)pG_Config->RS485_ID);	// 0xABCD(AB = 실제 어드레스 0x00~0xFF, C = null, D = 설정된 ID 0x1~0xF)
							Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
						}
						else if(Gu8_INFO_Disp_Tmr <= 60)		// 부팅 후 2초간 프로토콜 타입 출력
						{
							sprintf(buff, "P%3d", (uint16_t)pG_Config->Protocol_Type);
							Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
						}
						else if((Gu8_Special_Function_Key_Tmr >= 20) && ((item == mapping_ITEM_LIGHT_1) || (item == mapping_ITEM_BATCH_LIGHT_OFF) || (item == mapping_ITEM_LIGHT_GROUP)))
						{
#ifndef _ONE_SIZE_LIGHT_n_ELEC_MODEL_
							TN_Lcd.Bit.Icon_DIM		= 0;
#endif
							TN_Lcd.Bit.Icon_W		= 0;
							sprintf(buff, "%1dSEC", (uint16_t)(Gu8_Special_Function_Key_Tmr / 10));
						}
						else if(Gu8_LCD_DIM_Tmr)
						{
#ifndef _ONE_SIZE_LIGHT_n_ELEC_MODEL_							
							TN_Lcd.Bit.Icon_DIM		= 1;
#endif
							TN_Lcd.Bit.Icon_W		= 0;
							
							if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_1 && pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_2)
							{
								sprintf(buff, "%2d%2d", (uint16_t)(pG_State->Dimming_Level.Dimming1), (uint16_t)(pG_State->Dimming_Level.Dimming2));		//20210604 위 내용을 추가하고 주석처리
							}
							else if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_1 && pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)		//210623 디밍1, 색온도1 가능 모델일 경우
							{
								sprintf(buff, "%2d%2d", (uint16_t)(pG_State->Dimming_Level.Dimming1), (uint16_t)(pG_State->Color_Temp_Level.Color_Temp1));
							}							
							else if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_1)
							{
								sprintf(buff, "%4d", (uint16_t)(pG_State->Dimming_Level.Dimming1));
							}
							else if(pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_2)
							{
								sprintf(buff, "%4d", (uint16_t)(pG_State->Dimming_Level.Dimming2));
							}
							else if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)		//210622 전등색1 모델
							{
								sprintf(buff, "%4d", (uint16_t)(pG_State->Color_Temp_Level.Color_Temp1));
							}
								// sprintf(buff, "%4d", (uint16_t)(pG_State->Dimming_Level.Dimming1));		20210604 디밍2의 수치 LCD에서 보기 위해서 주석 처리

							// if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1 && pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)		//210622 전등색1, 2 모델일때
							// {
							// 	sprintf(buff, "%2d%2d", (uint16_t)(pG_State->Color_Temp_Level.Color_Temp1), (uint16_t)(pG_State->Color_Temp_Level.Color_Temp2));
							// }

							// else if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)		//210622 전등색2 모델
							// {
							// 	sprintf(buff, "%4d", (uint16_t)(pG_State->Color_Temp_Level.Color_Temp2));
							// }
						}
						else
						{
							TN_Lcd.Bit.Icon_W		= 1;
#ifndef _ONE_SIZE_LIGHT_n_ELEC_MODEL_							
							TN_Lcd.Bit.Icon_DIM		= 0;
#endif
							Gu8_Dim_Flag = 0;					//210623
							Gu8_Color_Temp_Flag = 0;			//230623
#ifndef LCD_SEG_TEST
							if(TN_Lcd.Bit.Icon_ELEC_1 && TN_Lcd.Bit.Icon_ELEC_2)	LCD_Watt	= Gu16_LCD_Watt_1 + Gu16_LCD_Watt_2;
							else if(TN_Lcd.Bit.Icon_ELEC_1)							LCD_Watt	= Gu16_LCD_Watt_1;
							else if(TN_Lcd.Bit.Icon_ELEC_2)							LCD_Watt	= Gu16_LCD_Watt_2;
							else													LCD_Watt	= 0;
#else
#ifdef _ONE_SIZE_LIGHT_n_ELEC_MODEL_	//1개용 대기스위치만
							LCD_Watt	= value;
							value += 1111;
							if(value > 9999)	value = 0;
#endif
#endif
							sprintf(buff, "%4d", LCD_Watt);
						}
						
						LCD_GLASS_DisplayString((void*)&buff);
						//Gu8_LCD_ElecLimitCurrent_Tmr	= 5;	//500ms 마다 갱신
						Gu8_LCD_ElecLimitCurrent_Tmr	= 10;	//1s 마다 갱신
						break;
				}
			}
			TN_LCD_Write();
		}
	}
}

void DIGIT_1_DisplayString(uint8_t ch)
{
	switch(ch)
	{
		case '0':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 0;	break;
		case '1':	TN_Lcd.Bit.A1	= 0;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 0;	TN_Lcd.Bit.E1	= 0;	TN_Lcd.Bit.F1	= 0;	TN_Lcd.Bit.G1	= 0;	break;
		case '2':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 0;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 0;	TN_Lcd.Bit.G1	= 1;	break;
		case '3':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 0;	TN_Lcd.Bit.F1	= 0;	TN_Lcd.Bit.G1	= 1;	break;
		case '4':	TN_Lcd.Bit.A1	= 0;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 0;	TN_Lcd.Bit.E1	= 0;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		case '5':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 0;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 0;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		case '6':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 0;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		case '7':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 0;	TN_Lcd.Bit.E1	= 0;	TN_Lcd.Bit.F1	= 0;	TN_Lcd.Bit.G1	= 0;	break;
		case '8':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		case '9':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 0;	TN_Lcd.Bit.E1	= 0;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		case 'V':	TN_Lcd.Bit.A1	= 0;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 0;	break;
		case 'P':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 0;	TN_Lcd.Bit.D1	= 0;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		case 'a':	// 'A'
		case 'A':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 0;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		case 'b':	// 'b'
		case 'B':	TN_Lcd.Bit.A1	= 0;	TN_Lcd.Bit.B1	= 0;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		case 'c':	// 'C'
		case 'C':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 0;	TN_Lcd.Bit.C1	= 0;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 0;	break;
		case 'd':	// 'd'
		case 'D':	TN_Lcd.Bit.A1	= 0;	TN_Lcd.Bit.B1	= 1;	TN_Lcd.Bit.C1	= 1;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 0;	TN_Lcd.Bit.G1	= 1;	break;
		case 'e':	// 'E'
		case 'E':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 0;	TN_Lcd.Bit.C1	= 0;	TN_Lcd.Bit.D1	= 1;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		case 'f':	// 'F'
		case 'F':	TN_Lcd.Bit.A1	= 1;	TN_Lcd.Bit.B1	= 0;	TN_Lcd.Bit.C1	= 0;	TN_Lcd.Bit.D1	= 0;	TN_Lcd.Bit.E1	= 1;	TN_Lcd.Bit.F1	= 1;	TN_Lcd.Bit.G1	= 1;	break;
		default:	TN_Lcd.Bit.A1	= 0;	TN_Lcd.Bit.B1	= 0;	TN_Lcd.Bit.C1	= 0;	TN_Lcd.Bit.D1	= 0;	TN_Lcd.Bit.E1	= 0;	TN_Lcd.Bit.F1	= 0;	TN_Lcd.Bit.G1	= 0;	break;
	}
}

void DIGIT_2_DisplayString(uint8_t ch)
{
	switch(ch)
	{
		case '0':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 0;	break;
		case '1':	TN_Lcd.Bit.A2	= 0;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 0;	TN_Lcd.Bit.E2	= 0;	TN_Lcd.Bit.F2	= 0;	TN_Lcd.Bit.G2	= 0;	break;
		case '2':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 0;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 0;	TN_Lcd.Bit.G2	= 1;	break;
		case '3':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 0;	TN_Lcd.Bit.F2	= 0;	TN_Lcd.Bit.G2	= 1;	break;
		case '4':	TN_Lcd.Bit.A2	= 0;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 0;	TN_Lcd.Bit.E2	= 0;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 1;	break;
		case 'S':
		case '5':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 0;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 0;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 1;	break;
		case '6':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 0;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 1;	break;
		case '7':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 0;	TN_Lcd.Bit.E2	= 0;	TN_Lcd.Bit.F2	= 0;	TN_Lcd.Bit.G2	= 0;	break;
		case '8':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 1;	break;
		case '9':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 0;	TN_Lcd.Bit.E2	= 0;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 1;	break;
		case 'a':	// 'A'
		case 'A':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 0;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 1;	break;
		case 'b':	// 'b'
		case 'B':	TN_Lcd.Bit.A2	= 0;	TN_Lcd.Bit.B2	= 0;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 1;	break;
		case 'c':	// 'C'
		case 'C':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 0;	TN_Lcd.Bit.C2	= 0;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 0;	break;
		case 'd':	// 'd'
		case 'D':	TN_Lcd.Bit.A2	= 0;	TN_Lcd.Bit.B2	= 1;	TN_Lcd.Bit.C2	= 1;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 0;	TN_Lcd.Bit.G2	= 1;	break;
		case 'e':	// 'E'
		case 'E':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 0;	TN_Lcd.Bit.C2	= 0;	TN_Lcd.Bit.D2	= 1;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 1;	break;
		case 'f':	// 'F'
		case 'F':	TN_Lcd.Bit.A2	= 1;	TN_Lcd.Bit.B2	= 0;	TN_Lcd.Bit.C2	= 0;	TN_Lcd.Bit.D2	= 0;	TN_Lcd.Bit.E2	= 1;	TN_Lcd.Bit.F2	= 1;	TN_Lcd.Bit.G2	= 1;	break;
		default:	TN_Lcd.Bit.A2	= 0;	TN_Lcd.Bit.B2	= 0;	TN_Lcd.Bit.C2	= 0;	TN_Lcd.Bit.D2	= 0;	TN_Lcd.Bit.E2	= 0;	TN_Lcd.Bit.F2	= 0;	TN_Lcd.Bit.G2	= 0;	break;
	}
}

void DIGIT_3_DisplayString(uint8_t ch)
{
	switch(ch)
	{
		case '0':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 0;	break;
		case '1':	TN_Lcd.Bit.A3	= 0;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 0;	TN_Lcd.Bit.E3	= 0;	TN_Lcd.Bit.F3	= 0;	TN_Lcd.Bit.G3	= 0;	break;
		case '2':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 0;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 0;	TN_Lcd.Bit.G3	= 1;	break;
		case '3':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 0;	TN_Lcd.Bit.F3	= 0;	TN_Lcd.Bit.G3	= 1;	break;
		case '4':	TN_Lcd.Bit.A3	= 0;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 0;	TN_Lcd.Bit.E3	= 0;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 1;	break;
		case '5':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 0;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 0;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 1;	break;
		case '6':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 0;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 1;	break;
		case '7':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 0;	TN_Lcd.Bit.E3	= 0;	TN_Lcd.Bit.F3	= 0;	TN_Lcd.Bit.G3	= 0;	break;
		case '8':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 1;	break;
		case '9':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 0;	TN_Lcd.Bit.E3	= 0;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 1;	break;
		case 'a':	// 'A'
		case 'A':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 0;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 1;	break;
		case 'b':	// 'b'
		case 'B':	TN_Lcd.Bit.A3	= 0;	TN_Lcd.Bit.B3	= 0;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 1;	break;
		case 'c':	// 'C'
		case 'C':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 0;	TN_Lcd.Bit.C3	= 0;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 0;	break;
		case 'd':	// 'd'
		case 'D':	TN_Lcd.Bit.A3	= 0;	TN_Lcd.Bit.B3	= 1;	TN_Lcd.Bit.C3	= 1;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 0;	TN_Lcd.Bit.G3	= 1;	break;
		case 'e':	// 'E'
		case 'E':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 0;	TN_Lcd.Bit.C3	= 0;	TN_Lcd.Bit.D3	= 1;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 1;	break;
		case 'f':	// 'F'
		case 'F':	TN_Lcd.Bit.A3	= 1;	TN_Lcd.Bit.B3	= 0;	TN_Lcd.Bit.C3	= 0;	TN_Lcd.Bit.D3	= 0;	TN_Lcd.Bit.E3	= 1;	TN_Lcd.Bit.F3	= 1;	TN_Lcd.Bit.G3	= 1;	break;
		default:	TN_Lcd.Bit.A3	= 0;	TN_Lcd.Bit.B3	= 0;	TN_Lcd.Bit.C3	= 0;	TN_Lcd.Bit.D3	= 0;	TN_Lcd.Bit.E3	= 0;	TN_Lcd.Bit.F3	= 0;	TN_Lcd.Bit.G3	= 0;	break;
	}
}

void DIGIT_4_DisplayString(uint8_t ch)
{
	switch(ch)
	{
		case '0':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 0;	break;
		case '1':	TN_Lcd.Bit.A4	= 0;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 0;	TN_Lcd.Bit.E4	= 0;	TN_Lcd.Bit.F4	= 0;	TN_Lcd.Bit.G4	= 0;	break;
		case '2':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 0;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 0;	TN_Lcd.Bit.G4	= 1;	break;
		case '3':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 0;	TN_Lcd.Bit.F4	= 0;	TN_Lcd.Bit.G4	= 1;	break;
		case '4':	TN_Lcd.Bit.A4	= 0;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 0;	TN_Lcd.Bit.E4	= 0;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 1;	break;
		case '5':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 0;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 0;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 1;	break;
		case '6':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 0;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 1;	break;
		case '7':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 0;	TN_Lcd.Bit.E4	= 0;	TN_Lcd.Bit.F4	= 0;	TN_Lcd.Bit.G4	= 0;	break;
		case '8':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 1;	break;
		case '9':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 0;	TN_Lcd.Bit.E4	= 0;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 1;	break;
		case 'a':	// 'A'
		case 'A':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 0;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 1;	break;
		case 'b':	// 'b'
		case 'B':	TN_Lcd.Bit.A4	= 0;	TN_Lcd.Bit.B4	= 0;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 1;	break;
		case 'c':	// 'C'
		case 'C':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 0;	TN_Lcd.Bit.C4	= 0;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 0;	break;
		case 'd':	// 'd'
		case 'D':	TN_Lcd.Bit.A4	= 0;	TN_Lcd.Bit.B4	= 1;	TN_Lcd.Bit.C4	= 1;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 0;	TN_Lcd.Bit.G4	= 1;	break;
		case 'e':	// 'E'
		case 'E':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 0;	TN_Lcd.Bit.C4	= 0;	TN_Lcd.Bit.D4	= 1;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 1;	break;
		case 'f':	// 'F'
		case 'F':	TN_Lcd.Bit.A4	= 1;	TN_Lcd.Bit.B4	= 0;	TN_Lcd.Bit.C4	= 0;	TN_Lcd.Bit.D4	= 0;	TN_Lcd.Bit.E4	= 1;	TN_Lcd.Bit.F4	= 1;	TN_Lcd.Bit.G4	= 1;	break;
		default:	TN_Lcd.Bit.A4	= 0;	TN_Lcd.Bit.B4	= 0;	TN_Lcd.Bit.C4	= 0;	TN_Lcd.Bit.D4	= 0;	TN_Lcd.Bit.E4	= 0;	TN_Lcd.Bit.F4	= 0;	TN_Lcd.Bit.G4	= 0;	break;
	}
}

void LCD_GLASS_DisplayString(uint8_t* ptr)
{
	DIGIT_1_DisplayString(ptr[0]);
	DIGIT_2_DisplayString(ptr[1]);
	DIGIT_3_DisplayString(ptr[2]);
	DIGIT_4_DisplayString(ptr[3]);
}
/************************************************************************************
	Project		: 전자식스위치
	File Name	: MAIN.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/


#include "header.h"
#include "el_switch.h"
#include "led.h"
#include "WDGnBeep.h"
#include "dimming.h"
#include "ThreeWay.h"
#include "IR.h"
#include "ZeroCrossing.h"
#include "Timer.h"
#include "Debug.h"
#include "ADC.h"
#include "Relay.h"
#include "STPM3x_opt.h"
#include "rs-485.h"
#include "led.h"
#include "lcd.h"
#include "BigInteger.h"
#include "Beep.h"

//-------------------------------------------------------------------------------------------------------------------------
uint16_t	Gu16_ERR_Flag = 0;
uint8_t		Gu8_SystemInit	= 0;
uint8_t		touch_switch;
uint8_t		Flag = 0;
//-------------------------------------------------------------------------------------------------------------------------
// IR(TIM5),	1mTMR(TIM4),	ADC(TIM2),	PWM(TIM1)
//-------------------------------------------------------------------------------------------------------------------------
void main(void)
{
	uint16_t i;
	
	Gu16_ERR_Flag	= 0;
	G_Debug	= 0;
	G_Trace	= 0;
	
	//_asm("nop");
	
	CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);		// High speed internal clock prescaler: 1	16MHz
	
	Queue_Init();
	Debug_Init();
	// G_Debug = DEBUG_STPM3x_REALDATA;
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	printf("\n\nEL Switch Ver(%03d)\n\n", VERSION);
	TIM_Init();				// 1ms timer
	//----------------------------------------------------------------------------------
	enableInterrupts();		// Enable Interrupts
	delay(4);				// 4ms delay
	disableInterrupts();	// Disable Interrupts
	eeprom_Init();
	Dimming_Init();			// PWM
	RS485_ID_Init();
	// SET_Defaultdata();
	// printf("pG_State->KEY = 0x%x\r\n", (uint16_t)pG_State->KEY);
	//----------------------------------------------------------------------------------
	WDG_BootModeCheck();	// WDG Reset 체크
	Control_Recovery_Init();	// 정전전 상태 복구 초기화

	Relay_Init();			// Relay Output
	// Dimming_Init();			// PWM
#ifdef WIRING_THREEWAY		//결선 삼로만 해당 프로세스 사용함
#ifndef _ONE_SIZE_BATCH_BLOCK_MODEL_
	ThreeWay_Init();		// 3로
#endif
#endif
	ZeroCrossing_Init();
	
	EL_Switch_Init();
	Touch_GPIO_Init();
	//----------------------------------------------------------------------------------
	CLK_PeripheralClockConfig(CLK_Peripheral_I2C1, ENABLE);
	I2C_Init(I2C1, 300000, 0xA0, I2C_Mode_I2C, I2C_DutyCycle_2, I2C_Ack_Enable, I2C_AcknowledgedAddress_7bit);
	I2C_Cmd(I2C1, ENABLE);
	ITC_SetSoftwarePriority(I2C1_SPI2_IRQn, ITC_PriorityLevel_3);
	//----------------------------------------------------------------------------------
	enableInterrupts();		// Enable Interrupts
	LED_Init();
	
	Beep_WDG_Init();
	WDG_Config_Set();		// 와치독 설정
	WDG_SetCounter();

	Touch_Init();				// Touch(I2C)
	WDG_SetCounter();
	
	STPM3x_GPIO_Init();
	disableInterrupts();	// Disable Interrupts
	//----------------------------------------------------------------------------------
	
#if 1
	if(FLASH_ReadByte(0x480B) == 0x55)
	{
		if(FLASH_ReadByte(0x480C) == 0xAA)
		{
			USART_Bootloader_Disable();
		}
	}
#endif
	
	//printf("%02x\n", (uint16_t)FLASH_ReadByte(0x480A));
	if(FLASH_ReadByte(0x480A) != (Brownout_Reset_Threshold_4_2V90 | Brownout_Reset_ENABLE))
	{
		printf("SET Brownout_Reset\n");
		FLASH_Unlock(FLASH_MemType_Data);
		FLASH_ProgramOptionByte(0x480A, (Brownout_Reset_Threshold_4_2V90 | Brownout_Reset_ENABLE));	// Rising edge 2.90V, Falling edge 2.80V
		FLASH_Lock(FLASH_MemType_Data);
	}
	
	// Beep_WDG_Init();
	// WDG_Config_Set();		// 와치독 설정
	//----------------------------------------------------------------------------------
	
	//----------------------------------------------------------------------------------
	RS485_Init();			// RS-485

#ifdef	_ADC_
	ADC_Sensor_Init();		// ADC
#endif
	
	IR_Init();
	//----------------------------------------------------------------------------------
	enableInterrupts();		// Enable Interrupts
	//----------------------------------------------------------------------------------
	STPM3x_Init();			// RS-232(STPM3x)
	TN_LCD_Init();			// LCD
	BACK_LIGHT_Init();		// Back Light
	TN_LCD_Test();
	Control_Recovery();		// 정전전 상태 복구
	//----------------------------------------------------------------------------------
	if(Gu8_WDG_Boot	== 0)	Beep(FORCE_BEEP);
	//----------------------------------------------------------------------------------	
	Gu8_10ms_Tmr			= 10;
	Gu8_100ms_Tmr			= 10;
	Gu8_1000ms_Tmr			= 10;
	Gu8_SystemInit			= 1;
	
	// Gu8_IR_PutCnt			= 0;
	// Gu8_IR_GetCnt			= 0;
	
	Gu8_10ms_Flag			= 0;
	Gu8_ZeroCrossing_Tmr	= 255;	// 50ms
	Gu8_STPM3x_COMM_Error_Tmr	= 100;		// 1s
	
	STPM3x_UartData_Init();
	
	if(Gu8_WDG_Boot	== 1)
	{
		Gu8_PowerSaving_Tmr	= 0;
		Gu8_INFO_Disp_Tmr	= 0xFF;		// LCD 정보출력 중지
		Gu8_Back_Light_Flag = 0;
	}
	while (1)
	{
		//----------------------------------------------------------------------------------
		WDG_SetCounter();
		//printf(".");
		Timer10ms();
		
		Beep_Process();
		Debug_Process();
#ifdef	_ADC_
		ADC_Process();							// ADC 데이터 처리(온도, ACV, CT1, CT2)
#endif
		STPM3x_Process();
		BlackOut_Process();						// 정전발생시 데이터 1회 저장
#if defined _NO_PROTOCOL_ && defined THREEWAY_TRANS
#else
#ifdef WIRING_THREEWAY
#ifndef _ONE_SIZE_BATCH_BLOCK_MODEL_		//결선 삼로만 해당 프로세스 사용함
		ThreeWay_EXT_Switch_State_Process();	// 3로 외부스위치 처리, 통신 3로 사용이면 해당 기능 사용할 필요 없음.
#endif
#endif
#endif
		IR_Process();

#ifndef	_TOUCH_SENSITIVITY_TEST_
		Touch_and_LED_Process();
		SWITCH_Delay_OFF_Process();
		Light_Sleep_Process();
#endif
		RS485_Process();
		TN_LCD_Process();
		BACK_LIGHT_Process();
		// Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;	//210422 test
		//----------------------------------------------------------------------------------
	}
}

#ifdef  USE_FULL_ASSERT
  /**
    * @brief  Reports the name of the source file and the source line number
    *   where the assert_param error has occurred.
    * @param  file: pointer to the source file name
    * @param  line: assert_param error line source number
    * @retval None
    */
  void assert_failed(uint8_t* file, uint32_t line)
  {
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    	printf("assert_failed\n\r");
    }
  }
#endif

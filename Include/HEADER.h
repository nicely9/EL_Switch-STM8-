
#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stm8l15x.h"
#include "queue.h"
#include "eeprom.h"
#include "touch.h"
#include "ZeroCrossing.h"



//-------------------------------------------------------------------------------------------------------------------------
#define	VERSION		(uint16_t)(26)
//=========================================================================================================================
// #define		_TOUCH_SENSITIVITY_TEST_			// 터치 chip SENSITIVITY 설정 프로그램 테스트 모드, release 버젼에서는 반드시 해제해야 함
//=========================================================================================================================
#define		_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1개용 일괄차단 모델
// #define		_ONE_SIZE_LIGHT_MODEL_						// 1개용 전등 모델
// #define		_ONE_SIZE_LIGHT_n_ELEC_MODEL_				// 1개용 대기전력 모델
// #define		_TWO_SIZE_LIGHT_MODEL_					    // 2개용 전등 모델
// #define		_TWO_SIZE_LIGHT_n_ELEC_MODEL_				// 2개용 전등,전열 모델
// #define      _TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_  // 2개용 전등, 전열 + 디밍 모델
//=========================================================================================================================
// #define IR_ENABLE
#define IR_DISABLE
//=========================================================================================================================
#define COMM_THREEWAY  				//통신 3로 사용
// #define WIRING_THREEWAY			    //결선 삼로 시 사용
/*
일괄 스위치(클라이언트)<->전등 스위치(와이어) -> 둘다 WIRING_THREEWAY
 - 코콤 프로토콜일 경우 전등 스위치는 WIRING_THREEWAY라도 485로 3로 데이터 사용함.
일괄 스위치(통신)<->전등 스위치(통신) -> 둘다 COMM_THREEWAY
전등 스위치(와이어) <-> 텀블러 : WIRING_THREEWAY
*/
//=========================================================================================================================
// #define _NO_PROTOCOL_
// #define _HYUNDAI_PROTOCOL_                               //HYUNDAI PROTOCOL
// #define _CVNET_PROTOCOL_									//CVNET PROTOCOL
// #define _KOCOM_PROTOCOL_									//KOCOM PROTOCOL
// #define _COMMAX_PROTOCOL_								//COMMAX_PROTOCOL
#define _SAMSUNG_PROTOCOL_								//SAMSUNG SDS PROTOCOL
// #define _KDW_PROTOCOL_									//KYUNGDONGWON PROTOCOL
// #define _HW_PROTOCOL_									//HYPERWALL PROTOCOL
// #define _HDC_PROTOCOL_									//HDC LABS PROTOCOL
//=========================================================================================================================
#ifdef _CVNET_PROTOCOL_   //Cvnet 연동일 경우
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
// #define _BATCH_BLOCK_SWITCH_PROTOCOL_   //기본 일괄 스위치 모델 사용 시 
#define _TOTAL_SITWCH_PROTOCOL_             //3로 등 특이 기능 사용 시
#endif

#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined (_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined (_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
// #define _LIGHT_SWITCH_PROTOCOL_USE_ //대기전력 스위치를 전등 프로토콜 사용
#endif
#endif
//-------------------------------------------------------------------------------------------------------------------------
#define ID_SETTING
#ifdef ID_SETTING
#define RS_ID 1
#define RS_ELEC_ID 1        //COMMAX 사용
#endif

#ifdef _KOCOM_PROTOCOL_
#define KOCOM_LOWEST_ELEC_ID	0x00		// 장치기가 설정할 수 있는 가장 낮은 ELEC ID, 가장 낮은 대기전력 스위치 ID값
#endif
//=========================================================================================================================
// #define	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// 프로토콜 순서 조명, 디밍전등 순으로
#define	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// 디밍 전등 사용 시 반드시 사용. 프로토콜 순서 디밍전등, 조명 순으로
//=========================================================================================================================
#ifdef		_TWO_SIZE_LIGHT_MODEL_
//-------------------------------------------------------------------------------------------------------------------------
// #define		_TWO_SIZE_LIGHT_1_
// #define		_TWO_SIZE_LIGHT_2_                      // 현재 해당 모델 없음
// #define		_TWO_SIZE_LIGHT_3_						// 2개용 전등3
// #define		_TWO_SIZE_LIGHT_4_						// 2개용 전등4
// #define		_TWO_SIZE_LIGHT_5_						// 2개용 전등5
// #define		_TWO_SIZE_LIGHT_6_						// 2개용 전등6
// #define		_TWO_SIZE_LIGHT_2_ETC_                  // 완주이서 현장용 전등2구 특이 모델
// #define		_TWO_SIZE_LIGHT_3_ETC_                  // 완주이서 현장용 전등3구 특이 모델
// #define		_TWO_SIZE_LIGHT_3_n_3WAY_1_				// 1로 전등2, 3로 전등1
// #define		_TWO_SIZE_LIGHT_4_n_3WAY_1_				// 1로 전등3, 3로 전등1
#define      _TWO_SIZE_LIGHT_5_n_3WAY_1_             // 1로 전등4, 3로 전등1
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
#ifdef		_TWO_SIZE_LIGHT_n_ELEC_MODEL_
//-------------------------------------------------------------------------------------------------------------------------
// #define		_TWO_SIZE_LIGHT_1_n_ELEC_2_					// 2개용 전등1, 전열2
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_					// 2개용 전등2, 전열2
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_					// 2개용 전등3, 전열2
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_					// 2개용 전등4, 전열2
// #define		_TWO_SIZE_LIGHT_1_n_ELEC_2_n_3WAY_1			// 2개용 전등1(3로), 전열2, LCD X. 삼로는 전등 갯수에 포함됨
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_3WAY_1			// 2개용 전등2(3로), 전열2, LCD X. 삼로는 전등 갯수에 포함됨
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1			// 2개용 전등3(3로), 전열2, LCD X. 삼로는 전등 갯수에 포함됨
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1			// 2개용 전등4(3로), 전열2, LCD X. 삼로는 전등 갯수에 포함됨

// #define      _TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_           // 2개용 전등1, 전열2, LCD
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_           // 2개용 전등2, 전열2, LCD
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_           // 2개용 전등3, 전열2, LCD
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_           // 2개용 전등4, 전열2, LCD
#define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1_n_LCD_	// 2개용 전등3, 전열2, LCD. 삼로는 전등 갯수에 포함됨(일반전등2, 삼로전등1)
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1_n_LCD_	// 2개용 전등4, 전열2, LCD. 삼로는 전등 갯수에 포함됨(일반전등3, 삼로전등1)
// #define		_TWO_SIZE_LIGHT_1_n_ELEC_X_                 //네트워크 스위치 모델인데, 건설사에서 원하는 아이콘 위치가 대기전력 스위치 전등 아이콘 위치와 동일하여 적용
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_X_
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
#ifdef      _TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_            // 2개용 전등, 전열, 디밍, 전등색 모델
//-------------------------------------------------------------------------------------------------------------------------
// #define		_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_1_               // 2개용 전등, 전열, 디밍, 전등색 모델
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_               // 2개용 전등1, 전열2, LCD, Dimming1, (eeprom에 아직 매핑 안되어있음)
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_n_DIM_1_               // 2개용 전등2, 전열2, LCD, Dimming1, (eeprom에 아직 매핑 안되어있음)
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_n_DIM_1_	            // 2개용 전등3, 전열2, LCD, Dimming1, (eeprom에 아직 매핑 안되어있음)

// #define		_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_2_               // 2개용 전등3, 전열2, LCD, Dimming1, (eeprom에 아직 매핑 안되어있음), 디밍2이상 모델 사용 시 Relay_Ctrl 수정필요함.
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_2_               // 2개용 전등1, 전열2, LCD, Dimming2, (eeprom에 아직 매핑 안되어있음)

// #define      _TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_     // 2개용 전등1, 전열2, LCD, 디밍1, 색온도1
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_     // 2개용 전등2, 전열2, LCD, Dimming1, Color_Temp1 디밍전등 1개에 디밍, 색온도조절 함께 함
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_     // 2개용 전등3, 전열2, LCD, Dimming1, Color_Temp1
#define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_     // 2개용 전등4, 전열2, LCD, Dimming1, Color_Temp1,
// #define      _TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_n_3WAY_1_
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
#ifdef      _ONE_SIZE_BATCH_BLOCK_MODEL_
//-------------------------------------------------------------------------------------------------------------------------
// #define		_ONE_SIZE_BATCH_LIGHT_					            //일괄소등
#define		_ONE_SIZE_BATCH_LIGHT_n_GAS_			            //일괄소등, 가스차단
// #define      _ONE_SIZE_BATCH_LIGHT_n_ELEVATOR_                   //일괄소등, 엘리베이터
// #define      _ONE_SIZE_BATCH_LIGHT_n_3WAY_                       //일괄소등, 3로
// #define		_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_	            //일괄소등, 가스차단, 엘리베이터 호출
// #define		_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_		            //일괄소등, 가스차단, 3로
// #define      _ONE_SIZE_BATCH_LIGHT_n_ELEVATOR_n_3WAY_            //일괄소등, 엘리베이터, 3로
// #define		_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_	    //일괄소등/가스차단, 엘리베이터 호출, 3로(일괄소등, 가스차단을 하나의 버튼으로 사용)
// #define      _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_n_3WAY_     //일괄소등/쿡탑차단, 엘리베이터, 3로(일괄소등, 쿡탑차단을 하나의 버튼으로 사용)
// #define      _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_            //일괄소등, 쿡탑, 엘리베이터
// #define      _ONE_SIZE_BATCH_LIGHT_n_GAS_n_COOK_n_ELEVATOR_      //일괄소등, 가스/쿡탑 차단, 엘리베이터(가스차단, 쿡탑차단 한 버튼으로 사용)
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
#ifdef		_ONE_SIZE_LIGHT_MODEL_		
//-------------------------------------------------------------------------------------------------------------------------
// #define		_ONE_SIZE_LIGHT_1_						//1개용 전등1
// #define		_ONE_SIZE_LIGHT_2_						//1개용 전등2
#define		_ONE_SIZE_LIGHT_3_						//1개용 전등3
// #define      _ONE_SIZE_LIGHT_1_n_SLEEP_				//1개용 전등1, 취침
// #define      _ONE_SIZE_LIGHT_2_n_SLEEP_    			//1개용 전등2, 취침
// #define		_ONE_SIZE_LIGHT_1_n_3WAY_1_				//1개용 전등1, 삼로1
// #define		_ONE_SIZE_LIGHT_2_n_3WAY_1_				//1개용 전등2, 삼로1
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
#ifdef		_ONE_SIZE_LIGHT_n_ELEC_MODEL_
//-------------------------------------------------------------------------------------------------------------------------
#define		_ONE_SIZE_LIGHT_1_n_ELEC_2_n_LCD_
// #define		_ONE_SIZE_LIGHT_2_n_ELEC_2_n_LCD_
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
//#define	_ADC_
#ifdef		_ADC_
#define	_LSI_MEASURMENT_		// LSI Calibration
#endif
//#define	_WWDG_
//-------------------------------------------------------------------------------------------------------------------------
// #define	_STATE_DIFF_SAVE__
//-------------------------------------------------------------------------------------------------------------------------
#define	STPM3x_POWER
//#define	STPM3x_ENERGY
#define	STPM3x_OFFSET_SETTING
//#define	STPM3x_READ_SR
#define	_STPM3x_AUTO_LATCH_
#define		_STPM3x_MIN_READ_
//#define		_STPM3x_UART_READ_		// 20210428
//-------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------
#define	ERR_FLAG_TOUCH_CHIP_1	0x0001
#define	ERR_FLAG_TOUCH_CHIP_2	0x0002
#define	ERR_FLAG_TOUCH_TIMEOUT	0x0004

extern uint16_t	Gu16_ERR_Flag;
extern uint8_t	Gu8_SystemInit;

//-------------------------------------------------------------------------------------------------------------------------
//extern double sqrt(double x);

//-------------------------------------------------------------------------------------------------------------------------
#define delay(a)          {	Gu16_1ms_delay_tmr = a;	while(Gu16_1ms_delay_tmr){WDG_SetCounter();} }

// STM8L052R8
#define	DEBUG_USART					USART2
#define	DEBUG_PORT_CLK				CLK_Peripheral_USART2
#define	DEBUG_RX_PORT				GPIOE
#define	DEBUG_TX_PORT				GPIOE
#define	DEBUG_RX_PIN				GPIO_Pin_3
#define	DEBUG_TX_PIN				GPIO_Pin_4

#define	RS_485_USART				USART3
#define	RS_485_PORT_CLK				CLK_Peripheral_USART3
#define	RS_485_RX_PORT				GPIOG
#define	RS_485_TX_PORT				GPIOG
#define	RS_485_RX_PIN				GPIO_Pin_0
#define	RS_485_TX_PIN				GPIO_Pin_1
#define	RS_485_DE_PORT				GPIOG
#define	RS_485_DE_PIN				GPIO_Pin_2
//----------------------------------------------------------------------------------
#define	STPM3x_USART				USART1
#define	STPM3x_PORT_CLK				CLK_Peripheral_USART1
#define	STPM3x_RX_PORT				GPIOA
#define	STPM3x_TX_PORT				GPIOA
#define	STPM3x_RX_PIN				GPIO_Pin_3
#define	STPM3x_TX_PIN				GPIO_Pin_2

#define	STPM3x_SYN_PORT				GPIOF
#define	STPM3x_SYN_PIN				GPIO_Pin_7

#define	ADuM5010_PDIS_PORT			GPIOF
#define	ADuM5010_PDIS_PIN			GPIO_Pin_6
//----------------------------------------------------------------------------------
#define	IR_RECEVIE_PORT				GPIOA			// TIM5 CH1
#define	IR_RECEVIE_PIN				GPIO_Pin_7
//----------------------------------------------------------------------------------
#define	TEMPERATURE_ADC_CH			ADC_Channel_4	// GPIOC, GPIO_Pin_4		Slow
#define	AC_VOLTAGE_ADC_CH			ADC_Channel_11	// GPIOB, GPIO_Pin_7		Slow
#define	VREFINT_5_0V_CH				ADC_Channel_22	// 
#define	CT_SENSOR_1_ADC_CH			ADC_Channel_24	// GPIOF, GPIO_Pin_0		Fast
#define	CT_SENSOR_2_ADC_CH			ADC_Channel_25	// GPIOF, GPIO_Pin_1		Fast
#define	VREFINT_ADC_CH				ADC_Channel_Vrefint
//----------------------------------------------------------------------------------
#define	ENABLE_BIT_DIMMING_1		0x01
#define	ENABLE_BIT_DIMMING_2		0x02
#define ENABLE_BIT_COLOR_TEMP_1     0x01        //210622
#define ENABLE_BIT_COLOR_TEMP_2     0x02        //210622
#define	ENABLE_BIT_THREE_WAY_1		0x01
#define	ENABLE_BIT_THREE_WAY_2		0x02

#define	DIMMING_OUT_1_PORT			GPIOD
#define	DIMMING_OUT_1_PIN			GPIO_Pin_4
#define	DIMMING_OUT_2_PORT			GPIOD
#define	DIMMING_OUT_2_PIN			GPIO_Pin_5

// OR
#define	THREE_WAY_IN_1_PORT			GPIOD
#define	THREE_WAY_IN_1_PIN			GPIO_Pin_4
#define	THREE_WAY_IN_1_EXTI			EXTI_Pin_4
#define	THREE_WAY_IN_2_PORT			GPIOD
#define	THREE_WAY_IN_2_PIN			GPIO_Pin_5
#define	THREE_WAY_IN_2_EXTI			EXTI_Pin_5
//----------------------------------------------------------------------------------
#define	RELAY_1_PORT				GPIOD
#define	RELAY_1_PIN					GPIO_Pin_6
#define	RELAY_2_PORT				GPIOD
#define	RELAY_2_PIN					GPIO_Pin_7
#define	RELAY_3_PORT				GPIOC
#define	RELAY_3_PIN					GPIO_Pin_5
#define	RELAY_4_PORT				GPIOC
#define	RELAY_4_PIN					GPIO_Pin_6

#define	RELAY_5_PORT				GPIOG
#define	RELAY_5_PIN					GPIO_Pin_4
#define	RELAY_6_PORT				GPIOG
#define	RELAY_6_PIN					GPIO_Pin_6

#define	LATCH_RELAY_1_1_PORT		GPIOG
#define	LATCH_RELAY_1_1_PIN			GPIO_Pin_4
#define	LATCH_RELAY_1_2_PORT		GPIOG
#define	LATCH_RELAY_1_2_PIN			GPIO_Pin_5
#define	LATCH_RELAY_2_1_PORT		GPIOG
#define	LATCH_RELAY_2_1_PIN			GPIO_Pin_6
#define	LATCH_RELAY_2_2_PORT		GPIOG
#define	LATCH_RELAY_2_2_PIN			GPIO_Pin_7
//----------------------------------------------------------------------------------
#define	TOUCH_RESET_PORT			GPIOE
#define	TOUCH_RESET_PIN				GPIO_Pin_7

#define	TOUCH_INT_1_PORT			GPIOC
#define	TOUCH_INT_1_PIN				GPIO_Pin_2
#define TOUCH_INT_1_EXTI			EXTI_Pin_2

#define	TOUCH_INT_2_PORT			GPIOC
#define	TOUCH_INT_2_PIN				GPIO_Pin_3
#define TOUCH_INT_2_EXTI			EXTI_Pin_3
//----------------------------------------------------------------------------------
#define	ZERO_CROSSING_PORT			GPIOC
#define	ZERO_CROSSING_PIN			GPIO_Pin_7
#define ZERO_CROSSING_EXTI			EXTI_Pin_7

#define	SYSTEM_PVD_PORT				GPIOE
#define	SYSTEM_PVD_PIN				GPIO_Pin_6
//----------------------------------------------------------------------------------
#define BACK_LIGHT_PORT             GPIOD
#define BACK_LIGHT_PIN              GPIO_Pin_5
//----------------------------------------------------------------------------------
#define	Brownout_Reset_ENABLE			0x01
#define	Brownout_Reset_DISABLE			0x00
#define	Brownout_Reset_Threshold_0_2V75	(0x00)
#define	Brownout_Reset_Threshold_1_2V04	(0x01<<1)
#define	Brownout_Reset_Threshold_2_2V41	(0x02<<1)
#define	Brownout_Reset_Threshold_3_2V66	(0x03<<1)
#define	Brownout_Reset_Threshold_4_2V90	(0x04<<1)

#endif

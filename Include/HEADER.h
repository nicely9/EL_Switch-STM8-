
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
// #define		_TOUCH_SENSITIVITY_TEST_			// ��ġ chip SENSITIVITY ���� ���α׷� �׽�Ʈ ���, release ���������� �ݵ�� �����ؾ� ��
//=========================================================================================================================
#define		_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1���� �ϰ����� ��
// #define		_ONE_SIZE_LIGHT_MODEL_						// 1���� ���� ��
// #define		_ONE_SIZE_LIGHT_n_ELEC_MODEL_				// 1���� ������� ��
// #define		_TWO_SIZE_LIGHT_MODEL_					    // 2���� ���� ��
// #define		_TWO_SIZE_LIGHT_n_ELEC_MODEL_				// 2���� ����,���� ��
// #define      _TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_  // 2���� ����, ���� + ��� ��
//=========================================================================================================================
// #define IR_ENABLE
#define IR_DISABLE
//=========================================================================================================================
#define COMM_THREEWAY  				//��� 3�� ���
// #define WIRING_THREEWAY			    //�ἱ ��� �� ���
/*
�ϰ� ����ġ(Ŭ���̾�Ʈ)<->���� ����ġ(���̾�) -> �Ѵ� WIRING_THREEWAY
 - ���� ���������� ��� ���� ����ġ�� WIRING_THREEWAY�� 485�� 3�� ������ �����.
�ϰ� ����ġ(���)<->���� ����ġ(���) -> �Ѵ� COMM_THREEWAY
���� ����ġ(���̾�) <-> �Һ� : WIRING_THREEWAY
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
#ifdef _CVNET_PROTOCOL_   //Cvnet ������ ���
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
// #define _BATCH_BLOCK_SWITCH_PROTOCOL_   //�⺻ �ϰ� ����ġ �� ��� �� 
#define _TOTAL_SITWCH_PROTOCOL_             //3�� �� Ư�� ��� ��� ��
#endif

#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined (_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined (_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
// #define _LIGHT_SWITCH_PROTOCOL_USE_ //������� ����ġ�� ���� �������� ���
#endif
#endif
//-------------------------------------------------------------------------------------------------------------------------
#define ID_SETTING
#ifdef ID_SETTING
#define RS_ID 1
#define RS_ELEC_ID 1        //COMMAX ���
#endif

#ifdef _KOCOM_PROTOCOL_
#define KOCOM_LOWEST_ELEC_ID	0x00		// ��ġ�Ⱑ ������ �� �ִ� ���� ���� ELEC ID, ���� ���� ������� ����ġ ID��
#endif
//=========================================================================================================================
// #define	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// �������� ���� ����, ������� ������
#define	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// ��� ���� ��� �� �ݵ�� ���. �������� ���� �������, ���� ������
//=========================================================================================================================
#ifdef		_TWO_SIZE_LIGHT_MODEL_
//-------------------------------------------------------------------------------------------------------------------------
// #define		_TWO_SIZE_LIGHT_1_
// #define		_TWO_SIZE_LIGHT_2_                      // ���� �ش� �� ����
// #define		_TWO_SIZE_LIGHT_3_						// 2���� ����3
// #define		_TWO_SIZE_LIGHT_4_						// 2���� ����4
// #define		_TWO_SIZE_LIGHT_5_						// 2���� ����5
// #define		_TWO_SIZE_LIGHT_6_						// 2���� ����6
// #define		_TWO_SIZE_LIGHT_2_ETC_                  // �����̼� ����� ����2�� Ư�� ��
// #define		_TWO_SIZE_LIGHT_3_ETC_                  // �����̼� ����� ����3�� Ư�� ��
// #define		_TWO_SIZE_LIGHT_3_n_3WAY_1_				// 1�� ����2, 3�� ����1
// #define		_TWO_SIZE_LIGHT_4_n_3WAY_1_				// 1�� ����3, 3�� ����1
#define      _TWO_SIZE_LIGHT_5_n_3WAY_1_             // 1�� ����4, 3�� ����1
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
#ifdef		_TWO_SIZE_LIGHT_n_ELEC_MODEL_
//-------------------------------------------------------------------------------------------------------------------------
// #define		_TWO_SIZE_LIGHT_1_n_ELEC_2_					// 2���� ����1, ����2
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_					// 2���� ����2, ����2
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_					// 2���� ����3, ����2
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_					// 2���� ����4, ����2
// #define		_TWO_SIZE_LIGHT_1_n_ELEC_2_n_3WAY_1			// 2���� ����1(3��), ����2, LCD X. ��δ� ���� ������ ���Ե�
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_3WAY_1			// 2���� ����2(3��), ����2, LCD X. ��δ� ���� ������ ���Ե�
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1			// 2���� ����3(3��), ����2, LCD X. ��δ� ���� ������ ���Ե�
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1			// 2���� ����4(3��), ����2, LCD X. ��δ� ���� ������ ���Ե�

// #define      _TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_           // 2���� ����1, ����2, LCD
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_           // 2���� ����2, ����2, LCD
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_           // 2���� ����3, ����2, LCD
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_           // 2���� ����4, ����2, LCD
#define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1_n_LCD_	// 2���� ����3, ����2, LCD. ��δ� ���� ������ ���Ե�(�Ϲ�����2, �������1)
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1_n_LCD_	// 2���� ����4, ����2, LCD. ��δ� ���� ������ ���Ե�(�Ϲ�����3, �������1)
// #define		_TWO_SIZE_LIGHT_1_n_ELEC_X_                 //��Ʈ��ũ ����ġ ���ε�, �Ǽ��翡�� ���ϴ� ������ ��ġ�� ������� ����ġ ���� ������ ��ġ�� �����Ͽ� ����
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_X_
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
#ifdef      _TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_            // 2���� ����, ����, ���, ����� ��
//-------------------------------------------------------------------------------------------------------------------------
// #define		_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_1_               // 2���� ����, ����, ���, ����� ��
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_               // 2���� ����1, ����2, LCD, Dimming1, (eeprom�� ���� ���� �ȵǾ�����)
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_n_DIM_1_               // 2���� ����2, ����2, LCD, Dimming1, (eeprom�� ���� ���� �ȵǾ�����)
// #define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_n_DIM_1_	            // 2���� ����3, ����2, LCD, Dimming1, (eeprom�� ���� ���� �ȵǾ�����)

// #define		_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_2_               // 2���� ����3, ����2, LCD, Dimming1, (eeprom�� ���� ���� �ȵǾ�����), ���2�̻� �� ��� �� Relay_Ctrl �����ʿ���.
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_2_               // 2���� ����1, ����2, LCD, Dimming2, (eeprom�� ���� ���� �ȵǾ�����)

// #define      _TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_     // 2���� ����1, ����2, LCD, ���1, ���µ�1
// #define		_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_     // 2���� ����2, ����2, LCD, Dimming1, Color_Temp1 ������� 1���� ���, ���µ����� �Բ� ��
// #define		_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_     // 2���� ����3, ����2, LCD, Dimming1, Color_Temp1
#define		_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_     // 2���� ����4, ����2, LCD, Dimming1, Color_Temp1,
// #define      _TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_n_3WAY_1_
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
#ifdef      _ONE_SIZE_BATCH_BLOCK_MODEL_
//-------------------------------------------------------------------------------------------------------------------------
// #define		_ONE_SIZE_BATCH_LIGHT_					            //�ϰ��ҵ�
#define		_ONE_SIZE_BATCH_LIGHT_n_GAS_			            //�ϰ��ҵ�, ��������
// #define      _ONE_SIZE_BATCH_LIGHT_n_ELEVATOR_                   //�ϰ��ҵ�, ����������
// #define      _ONE_SIZE_BATCH_LIGHT_n_3WAY_                       //�ϰ��ҵ�, 3��
// #define		_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_	            //�ϰ��ҵ�, ��������, ���������� ȣ��
// #define		_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_		            //�ϰ��ҵ�, ��������, 3��
// #define      _ONE_SIZE_BATCH_LIGHT_n_ELEVATOR_n_3WAY_            //�ϰ��ҵ�, ����������, 3��
// #define		_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_	    //�ϰ��ҵ�/��������, ���������� ȣ��, 3��(�ϰ��ҵ�, ���������� �ϳ��� ��ư���� ���)
// #define      _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_n_3WAY_     //�ϰ��ҵ�/��ž����, ����������, 3��(�ϰ��ҵ�, ��ž������ �ϳ��� ��ư���� ���)
// #define      _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_            //�ϰ��ҵ�, ��ž, ����������
// #define      _ONE_SIZE_BATCH_LIGHT_n_GAS_n_COOK_n_ELEVATOR_      //�ϰ��ҵ�, ����/��ž ����, ����������(��������, ��ž���� �� ��ư���� ���)
//-------------------------------------------------------------------------------------------------------------------------
#endif
//=========================================================================================================================
#ifdef		_ONE_SIZE_LIGHT_MODEL_		
//-------------------------------------------------------------------------------------------------------------------------
// #define		_ONE_SIZE_LIGHT_1_						//1���� ����1
// #define		_ONE_SIZE_LIGHT_2_						//1���� ����2
#define		_ONE_SIZE_LIGHT_3_						//1���� ����3
// #define      _ONE_SIZE_LIGHT_1_n_SLEEP_				//1���� ����1, ��ħ
// #define      _ONE_SIZE_LIGHT_2_n_SLEEP_    			//1���� ����2, ��ħ
// #define		_ONE_SIZE_LIGHT_1_n_3WAY_1_				//1���� ����1, ���1
// #define		_ONE_SIZE_LIGHT_2_n_3WAY_1_				//1���� ����2, ���1
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

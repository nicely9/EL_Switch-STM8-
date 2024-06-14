
#include "stm8l15x_it.h"
#include "header.h"
#include "adc.h"
#include "WDGnBeep.h"
#include "dimming.h"
#include "ZeroCrossing.h"
#include "Debug.h"
#include "IR.h"
#include "Timer.h"
#include "STPM3x_opt.h"
#include "rs-485.h"
#include "i2c.h"

/*
double sqrt(double x)
{
	extern double newton(double);
	int n;
	x = frexp(x, &n);
	x = newton(x);
	if (n & 1)
	x *= SQRT2;
	return (ldexp(x, n / 2));
}
*/
#ifdef _COSMIC_
// @brief Dummy interrupt routine
INTERRUPT_HANDLER(NonHandledInterrupt, 0)
{
	
}
#endif

// @brief TRAP interrupt routine
INTERRUPT_HANDLER_TRAP(TRAP_IRQHandler)
{
	
}
// @brief FLASH Interrupt routine.
INTERRUPT_HANDLER(FLASH_IRQHandler, 1)
{
	
}

// @brief DMA1 channel0 and channel1 Interrupt routine.
/*
@svlreg INTERRUPT_HANDLER(DMA1_CHANNEL0_1_IRQHandler, 2)
{
	irq_ADC_DMA();
}
*/
INTERRUPT_HANDLER(DMA1_CHANNEL0_1_IRQHandler, 2)
{
#ifdef	_ADC_
	
	int i, cnt = 0;
	
	//if(DMA_GetITStatus(DMA1_IT_HT0))	// Half Transaction Interrupt Channel 0
	if(DMA_GetITStatus(DMA1_IT_TC0))	// Transaction Complete Interrupt Channel 0
	{
		if(pG_Config->Enable_Flag.ADC_Temperature)
		{
			G_ADC_Sum_Buffer[ADC_BUFF_TEMPERATURE]	+=	G_ADC_DMA_Buffer[cnt++];
		}
		if(pG_Config->Enable_Flag.ADC_AC_Voltage)
		{
			if((uint32_t)(Gu16_MAX_ACV_Adc_tmp[0]) < (uint32_t)(G_ADC_DMA_Buffer[cnt]))
			{
				for(i=ADC_MAX_MIN_CNT-1;i>0;i--)
				{
					Gu16_MAX_ACV_Adc_tmp[i]	= Gu16_MAX_ACV_Adc_tmp[i-1];
				}
				Gu16_MAX_ACV_Adc_tmp[0]	= G_ADC_DMA_Buffer[cnt];
			}
			if((uint32_t)(Gu16_MIN_ACV_Adc_tmp[0]) > (uint32_t)(G_ADC_DMA_Buffer[cnt]))
			{
				for(i=ADC_MAX_MIN_CNT-1;i>0;i--)
				{
					Gu16_MIN_ACV_Adc_tmp[i]	= Gu16_MIN_ACV_Adc_tmp[i-1];
				}
				Gu16_MIN_ACV_Adc_tmp[0]	= G_ADC_DMA_Buffer[cnt];
			}
			cnt++;
		}
		if(pG_Config->Enable_Flag.ADC_5_0V_Ref)
		{
			G_ADC_Sum_Buffer[ADC_BUFF_5V_VREF]		+=	G_ADC_DMA_Buffer[cnt++];
		}
		if(pG_Config->Enable_Flag.ADC_CT_Sensor1)
		{
			//G_ADC_Sum_Buffer[ADC_BUFF_CT_SENSOR_1]	+=	G_ADC_DMA_Buffer[cnt++];
			if((uint32_t)(Gu16_MAX_CT1_Adc_tmp[0]) < (uint32_t)(G_ADC_DMA_Buffer[cnt]))
			{
				for(i=ADC_MAX_MIN_CNT-1;i>0;i--)
				{
					Gu16_MAX_CT1_Adc_tmp[i]	= Gu16_MAX_CT1_Adc_tmp[i-1];
				}
				Gu16_MAX_CT1_Adc_tmp[0]	= G_ADC_DMA_Buffer[cnt];
			}
			if((uint32_t)(Gu16_MIN_CT1_Adc_tmp[0]) > (uint32_t)(G_ADC_DMA_Buffer[cnt]))
			{
				for(i=ADC_MAX_MIN_CNT-1;i>0;i--)
				{
					Gu16_MIN_CT1_Adc_tmp[i]	= Gu16_MIN_CT1_Adc_tmp[i-1];
				}
				Gu16_MIN_CT1_Adc_tmp[0]	= G_ADC_DMA_Buffer[cnt];
			}
			cnt++;
		}
		if(pG_Config->Enable_Flag.ADC_CT_Sensor2)
		{
			//G_ADC_Sum_Buffer[ADC_BUFF_CT_SENSOR_2]	+=	G_ADC_DMA_Buffer[cnt++];
			if((uint32_t)(Gu16_MAX_CT2_Adc_tmp[0]) < (uint32_t)(G_ADC_DMA_Buffer[cnt]))
			{
				for(i=ADC_MAX_MIN_CNT-1;i>0;i--)
				{
					Gu16_MAX_CT2_Adc_tmp[i]	= Gu16_MAX_CT2_Adc_tmp[i-1];
				}
				Gu16_MAX_CT2_Adc_tmp[0]	= G_ADC_DMA_Buffer[cnt];
			}
			if((uint32_t)(Gu16_MIN_CT2_Adc_tmp[0]) > (uint32_t)(G_ADC_DMA_Buffer[cnt]))
			{
				for(i=ADC_MAX_MIN_CNT-1;i>0;i--)
				{
					Gu16_MIN_CT2_Adc_tmp[i]	= Gu16_MIN_CT2_Adc_tmp[i-1];
				}
				Gu16_MIN_CT2_Adc_tmp[0]	= G_ADC_DMA_Buffer[cnt];
			}
			cnt++;
		}
		if(pG_Config->Enable_Flag.ADC_3_3V_Ref)
		{
			G_ADC_Sum_Buffer[ADC_BUFF_VREFINT]	+=	G_ADC_DMA_Buffer[cnt++];	// VrefINT
		}
		
		Gu16_ADC_DMA_Cnt++;
		if(Gu16_ADC_DMA_Cnt  >= MAX_ADC_CNT)
		{
			for(i=0;i<ADC_MAX_MIN_CNT;i++)
			{
				Gu16_MAX_ACV_Adc[i]	= Gu16_MAX_ACV_Adc_tmp[i];
				Gu16_MAX_ACV_Adc_tmp[i]	= 0;
				Gu16_MAX_CT1_Adc[i]	= Gu16_MAX_CT1_Adc_tmp[i];
				Gu16_MAX_CT1_Adc_tmp[i]	= 0;
				Gu16_MAX_CT2_Adc[i]	= Gu16_MAX_CT2_Adc_tmp[i];
				Gu16_MAX_CT2_Adc_tmp[i]	= 0;
			}
			for(i=0;i<ADC_MAX_MIN_CNT;i++)
			{
				Gu16_MIN_ACV_Adc[i]	= Gu16_MIN_ACV_Adc_tmp[i];
				Gu16_MIN_ACV_Adc_tmp[i]	= 0xFFFF;
				Gu16_MIN_CT1_Adc[i]	= Gu16_MIN_CT1_Adc_tmp[i];
				Gu16_MIN_CT1_Adc_tmp[i]	= 0xFFFF;
				Gu16_MIN_CT2_Adc[i]	= Gu16_MIN_CT2_Adc_tmp[i];
				Gu16_MIN_CT2_Adc_tmp[i]	= 0xFFFF;
			}
			
			Gdbl_ADC_Avr_Buffer[ADC_BUFF_5V_VREF]		= (double)((double)G_ADC_Sum_Buffer[ADC_BUFF_5V_VREF] / (double)Gu16_ADC_DMA_Cnt);
			Gdbl_ADC_Avr_Buffer[ADC_BUFF_TEMPERATURE]	= (double)((double)G_ADC_Sum_Buffer[ADC_BUFF_TEMPERATURE] / (double)Gu16_ADC_DMA_Cnt);
			//Gdbl_ADC_Avr_Buffer[ADC_BUFF_CT_SENSOR_1]	= (double)((double)G_ADC_Sum_Buffer[ADC_BUFF_CT_SENSOR_1] / (double)Gu16_ADC_DMA_Cnt);
			//Gdbl_ADC_Avr_Buffer[ADC_BUFF_CT_SENSOR_2]	= (double)((double)G_ADC_Sum_Buffer[ADC_BUFF_CT_SENSOR_2] / (double)Gu16_ADC_DMA_Cnt);
			
			Gdbl_ADC_Avr_Buffer[ADC_BUFF_VREFINT]		= (double)((double)G_ADC_Sum_Buffer[ADC_BUFF_VREFINT] / (double)Gu16_ADC_DMA_Cnt);
			
			for(i=0;i<MAX_ADC_BUFF;i++)
			{
				G_ADC_Sum_Buffer[i]	= 0;
			}
			
			Gu16_ADC_DMA_Cnt			= 0;
			G_DMA_Flag				|= ADC_COMPLETE;
		}
		DMA_ClearITPendingBit(DMA1_IT_TC0);			// Clear IT Pending Bit
	}
	if(DMA_GetITStatus(DMA1_IT_TC1))	// Transaction Complete Interrupt Channel 1
	{
		DMA_ClearITPendingBit(DMA1_IT_TC1);			// Clear IT Pending Bit
	}
#endif
}
// @brief DMA1 channel2 and channel3 Interrupt routine.
INTERRUPT_HANDLER(DMA1_CHANNEL2_3_IRQHandler, 3)
{
	/*
	if(DMA_GetITStatus(DMA1_IT_TC2))			// Transaction Complete Interrupt Channel 2
	{
		DMA_ClearITPendingBit(DMA1_IT_TC2);		// Clear IT Pending Bit
	}
	if(DMA_GetITStatus(DMA1_IT_TC3))			// Transaction Complete Interrupt Channel 2
	{
		DMA_ClearITPendingBit(DMA1_IT_TC3);		// Clear IT Pending Bit
	}
	*/
}
// @brief RTC / CSS_LSE Interrupt routine.
INTERRUPT_HANDLER(RTC_CSSLSE_IRQHandler, 4)
{
	
}
// @brief External IT PORTE/F and PVD Interrupt routine.
INTERRUPT_HANDLER(EXTIE_F_PVD_IRQHandler, 5)
{
	
}

// @brief External IT PORTB / PORTG Interrupt routine.
INTERRUPT_HANDLER(EXTIB_G_IRQHandler, 6)
{
	
}

// @brief External IT PORTD /PORTH Interrupt routine.
INTERRUPT_HANDLER(EXTID_H_IRQHandler, 7)
{
	
}

//* @brief External IT PIN0 Interrupt routine.
INTERRUPT_HANDLER(EXTI0_IRQHandler, 8)
{
	
}

// @brief External IT PIN1 Interrupt routine.
INTERRUPT_HANDLER(EXTI1_IRQHandler, 9)
{
	
}

// @brief External IT PIN2 Interrupt routine.
@svlreg INTERRUPT_HANDLER(EXTI2_IRQHandler, 10)									// Touch 1
{
	irq_Touch_1();
}

// @brief External IT PIN3 Interrupt routine.
@svlreg INTERRUPT_HANDLER(EXTI3_IRQHandler, 11)									// Touch 2
{
	irq_Touch_2();
}

// @brief External IT PIN4 Interrupt routine.
INTERRUPT_HANDLER(EXTI4_IRQHandler, 12)
{
	
}

// @brief External IT PIN5 Interrupt routine.
INTERRUPT_HANDLER(EXTI5_IRQHandler, 13)
{
	
}

// @brief External IT PIN6 Interrupt routine.
INTERRUPT_HANDLER(EXTI6_IRQHandler, 14)
{
	
}

// @brief External IT PIN7 Interrupt routine.

@svlreg INTERRUPT_HANDLER(EXTI7_IRQHandler, 15)									// Zerocrossing
{
	irq_ZeroCrossing();
}

// @brief LCD /AES Interrupt routine.
INTERRUPT_HANDLER(LCD_AES_IRQHandler, 16)
{
	
}
// @brief CLK switch/CSS/TIM1 break Interrupt routine.
INTERRUPT_HANDLER(SWITCH_CSS_BREAK_DAC_IRQHandler, 17)
{
	
}

// @brief ADC1/Comparator Interrupt routine.
INTERRUPT_HANDLER(ADC1_COMP_IRQHandler, 18)
{
	/*
	if(ADC_GetITStatus(ADC1, ADC_IT_OVER))
	{
		G_Debug |= 0x01;
		ADC_ClearITPendingBit(ADC1, ADC_IT_OVER);
	}
	else if(ADC_GetITStatus(ADC1, ADC_IT_EOC))
	{
		G_Debug |= 0x02;
		ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
	}
	*/
}

// @brief TIM2 Update/Overflow/Trigger/Break /USART2 TX Interrupt routine.
INTERRUPT_HANDLER(TIM2_UPD_OVF_TRG_BRK_USART2_TX_IRQHandler, 19)		// DEBUG TX
{
	//TIM2_ClearITPendingBit(TIM2_IT_Update);
	irq_Debug_TX();
#ifndef _KOCOM_PROTOCOL_	
	irq_TIM2_Timer();
#endif
}

// @brief Timer2 Capture/Compare / USART2 RX Interrupt routine.
@svlreg INTERRUPT_HANDLER(TIM2_CC_USART2_RX_IRQHandler, 20)						// DEBUG RX
{
	irq_Debug_RX();
#ifdef	_LSI_MEASURMENT_
	irq_TIM2_LSIMeasurment();
#endif
}

// @brief Timer3 Update/Overflow/Trigger/Break Interrupt routine.
@svlreg INTERRUPT_HANDLER(TIM3_UPD_OVF_TRG_BRK_USART3_TX_IRQHandler, 21)		// RS_485 TX
{
	irq_RS485_TX();
	/*
	if(TIM3_GetITStatus(TIM3_IT_Update) != RESET)
	{
		
	}
	*/
}

// @brief Timer3 Capture/Compare /USART3 RX Interrupt routine.
INTERRUPT_HANDLER(TIM3_CC_USART3_RX_IRQHandler, 22)						// RS_485 RX
{
	irq_RS485_RX();
}

// @brief TIM1 Update/Overflow/Trigger/Commutation Interrupt routine.
INTERRUPT_HANDLER(TIM1_UPD_OVF_TRG_COM_IRQHandler, 23)
{
	
}
// @brief TIM1 Capture/Compare Interrupt routine.
INTERRUPT_HANDLER(TIM1_CC_IRQHandler, 24)								// PWM
{
	irq_Dimming();
}

// @brief TIM4 Update/Overflow/Trigger Interrupt routine.
@svlreg INTERRUPT_HANDLER(TIM4_UPD_OVF_TRG_IRQHandler, 25)						// 1ms Timer
{
  irq_TIM4_Timer();
}

// @brief SPI1 Interrupt routine.
INTERRUPT_HANDLER(SPI1_IRQHandler, 26)
{
	
}

// @brief USART1 TX / TIM5 Update/Overflow/Trigger/Break Interrupt  routine.
INTERRUPT_HANDLER(USART1_TX_TIM5_UPD_OVF_TRG_BRK_IRQHandler, 27)		// STPM3x TX
{
	irq_STPM3x_TX();
	//irq_Debug_TX();
}

// @brief USART1 RX / Timer5 Capture/Compare Interrupt routine.
@svlreg INTERRUPT_HANDLER(USART1_RX_TIM5_CC_IRQHandler, 28)						// STPM3x RX, IR
{
	irq_STPM3x_RX();
	//irq_Debug_RX();
	irq_IR();
}

// @brief I2C1 / SPI2 Interrupt routine.
@svlreg INTERRUPT_HANDLER(I2C1_SPI2_IRQHandler, 29)								// I2C
{
	uint16_t	event;
	
	if (I2C_ReadRegister(I2C1, I2C_Register_SR2))
	{
		I2C1->SR2 = 0;
	}
	
	event	= I2C_GetLastEvent(I2C1);
	switch (event)
	{
		// EV5	Slave Address 전송
		case I2C_EVENT_MASTER_MODE_SELECT :		//0x301 -> Start Bit, Master Mode, Bus busy
			/* Send slave Address for write */
			if(u8_Direction == WRITE)
			{
				I2C_Send7bitAddress(I2C1, (u8)(u16_SlaveAdd_cpy << 1), I2C_Direction_Transmitter);
			}
			else if(u8_Direction == READ)
			{
				I2C_Send7bitAddress(I2C1, (u8)((u16_SlaveAdd_cpy << 1)|1), I2C_Direction_Receiver);
			}
			else
			{
				I2C_GenerateSTOP(I2C1, ENABLE);
				I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_ERR | I2C_IT_BUF) , DISABLE);
				set_tout_ms(0);
			}
			break;
		//-----------------------------------------------------------------------------------------------
		// EV6(Address Acknowledge)			EV5 전송 후 Slave에서 ACK가 수신되면
		case I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED:		// 7bit Receiver
			// EV6_1
			if (u8_NumByte_cpy == 2)		// EV5 -> EV6_1, EV7_3
			{
				I2C_AckPositionConfig(I2C1, I2C_AckPosition_Next);	//I2C1->CR2 |= I2C_CR2_POS;	// set POS bit
				I2C_AcknowledgeConfig(I2C1, DISABLE);				//I2C1->CR2 &= (uint8_t)(~I2C_CR2_ACK);
				break;
			}
			// EV6_3
			if (u8_NumByte_cpy == 1)		// EV5 -> EV6_3, EV7
			{
				I2C_AcknowledgeConfig(I2C1, DISABLE);						//I2C1->CR2 &= (uint8_t)(~I2C_CR2_ACK);
				I2C_GenerateSTOP(I2C1, ENABLE);								//I2C1->CR2 |= I2C_CR2_STOP;
				I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_BUF) , ENABLE);	//I2C1->ITR |= I2C_ITR_ITBUFEN;
			}
			break;
			
		// EV7
		case I2C_SR1_RXNE:		// 0x0040,	마지막 1바이트 read
			//Debug_Put('@');
			if (u8_NumByte_cpy == 1)
			{
				*pu8_DataBuffer_cpy++	= I2C_ReceiveData(I2C1);
				u8_NumByte_cpy--;
			}
			if (u8_NumByte_cpy == 0)
			{
				I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_ERR | I2C_IT_BUF) , DISABLE);
				set_tout_ms(0);
			}
			break;
			
		
		case I2C_EVENT_MASTER_BYTE_RECEIVED | I2C_SR1_BTF:		// 0x0344, 마지막 2바이트 read
			//Debug_Put('#');
			
			if (u8_NumByte_cpy >= 4)
			{
				*pu8_DataBuffer_cpy++	= I2C_ReceiveData(I2C1);
				u8_NumByte_cpy--;
				break;
			}
			// EV7_2
			if (u8_NumByte_cpy == 3)
			{
				I2C_AcknowledgeConfig(I2C1, DISABLE);	//I2C1->CR2 &= (uint8_t)(~I2C_CR2_ACK);		// Set NACK (ACK=0)
				*pu8_DataBuffer_cpy++	= I2C_ReceiveData(I2C1);
				u8_NumByte_cpy--;
				I2C_GenerateSTOP(I2C1, ENABLE);			//I2C1->CR2 |= I2C_CR2_STOP;
				*pu8_DataBuffer_cpy++	= I2C_ReceiveData(I2C1);
				u8_NumByte_cpy--;
				I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_BUF) , ENABLE);	//I2C1->ITR |= I2C_ITR_ITBUFEN;
				break;
			}
			// EV7_3
			if (u8_NumByte_cpy == 2)
			{
				I2C_GenerateSTOP(I2C1, ENABLE);			//I2C1->CR2 |= I2C_CR2_STOP;
				*pu8_DataBuffer_cpy++	= I2C_ReceiveData(I2C1);
				u8_NumByte_cpy--;
				*pu8_DataBuffer_cpy++	= I2C_ReceiveData(I2C1);
				u8_NumByte_cpy--;
				I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_ERR | I2C_IT_BUF) , DISABLE);
				set_tout_ms(0);
				break;
			}
			break;
			
		// --EV7(Communication events)
		//case I2C_EVENT_MASTER_BYTE_RECEIVED:	// 0x0340
		//-----------------------------------------------------------------------------------------------
		// EV6(Address Acknowledge)			EV5 전송 후 Slave에서 ACK가 수신되면
		case I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED:	// 7bit Transmitter
			//Debug_Put('^');
			if (u8_NumByte_cpy != 0)
			{
				I2C_SendData(I2C1, *pu8_DataBuffer_cpy++);	// Send the first Data
				u8_NumByte_cpy--;
			}
			break;
		
		// EV8(Communication events)		데이터가 DR에 기록되었으며 출력되고 있음
		case I2C_EVENT_MASTER_BYTE_TRANSMITTING:		// 적용하면 속도가 빨라짐
			//Debug_Put('&');
			/*
			if (u8_NumByte_cpy != 0)
			{
				I2C_SendData(I2C1, *pu8_DataBuffer_cpy++);	// Transmit Data
				u8_NumByte_cpy--;
			}
			*/
			/*
			if (u8_NumByte_cpy == 0)
			{
				I2C_GenerateSTOP(I2C1, ENABLE);
				I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_ERR | I2C_IT_BUF) , DISABLE);
				set_tout_ms(0);
			}
			*/
			break;
		
		// EV8_2(Communication events)		데이터가 하드웨어적으로 출력됨
		case I2C_EVENT_MASTER_BYTE_TRANSMITTED:			// 좀 더 안정적으로 동작	0x0784 TXE, BTF, MSL, BUSY, TRA
			//Debug_Put('*');
			I2C1->DR;						// Read DR register to clear BTF Flag
			if (u8_NumByte_cpy != 0)
			{
				I2C_SendData(I2C1, *pu8_DataBuffer_cpy++);	// Transmit Data
				u8_NumByte_cpy--;
			}
			if (u8_NumByte_cpy == 0)
			{
				I2C_GenerateSTOP(I2C1, ENABLE);
				I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_ERR | I2C_IT_BUF) , DISABLE);
				set_tout_ms(0);
			}
			break;
		//-----------------------------------------------------------------------------------------------
		default:
			ErrProc(250);
			I2C_GenerateSTOP(I2C1, ENABLE);
			I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_ERR | I2C_IT_BUF) , DISABLE);
			/*
			Debug_Put((u8)(0xFF));
			Debug_Put((u8)(event>>8));
			Debug_Put((u8)(event&0x00ff));
			Debug_Put((u8)(0xFF));
			*/
			break;
	}
}

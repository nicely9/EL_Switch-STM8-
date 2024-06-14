/************************************************************************************
	Project		: 전자식스위치
	File Name	: IoT.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "IoT.h"
#include "Timer.h"
#include "Debug.h"

//-------------------------------------------------------------------------------------------------------------------------
void irq_IoT_TX(void)
{
	uint8_t data;
	
	if(USART_GetITStatus(IoT_USART, USART_IT_TC) != RESET)
	{
		if(Uart_GetTxQ(IoT_PORT, &data) != 0)
		{
			USART_SendData8(IoT_USART, data);
			USART_ClearITPendingBit(IoT_USART, USART_IT_TC);
		}
		else
		{
			USART_ITConfig(IoT_USART, USART_IT_TC, DISABLE);	// Disable the USART Transmit Complete interrupt
			//USART_ClearITPendingBit(IoT_USART, USART_IT_TC);	// ???
		}
	}
}

void irq_IoT_RX(void)
{
	uint8_t data;
	
	if(USART_GetITStatus(IoT_USART, USART_IT_RXNE) != RESET)
	{
		data = USART_ReceiveData8(IoT_USART);
		Uart_PutRxQ(IoT_PORT, data);							// Read one byte from the receive data register
	}
}
//-------------------------------------------------------------------------------------------------------------------------
void IoT_Init(void)
{
	if(pG_Config->Enable_Flag.IoT_RS232)
	{
		CLK_PeripheralClockConfig((CLK_Peripheral_TypeDef)IoT_PORT_CLK, ENABLE);	// Enable IoT USART clock
		GPIO_ExternalPullUpConfig(IoT_TX_PORT, IoT_TX_PIN, ENABLE);					// IoT Tx as alternate function push-pull
		GPIO_ExternalPullUpConfig(IoT_RX_PORT, IoT_RX_PIN, ENABLE);					// IoT Rx as alternate function push-pull
		USART_Init(IoT_USART, 115200, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
		//-------------------------------------------------------------------------------------------------------------------------
		USART_ITConfig(IoT_USART, USART_IT_RXNE, ENABLE);
		//USART_ITConfig(IoT_USART, USART_IT_TC, ENABLE);		// Enable the USART Transmit complete interrupt
		USART_Cmd(IoT_USART, ENABLE);
		//-------------------------------------------------------------------------------------------------------------------------
		printf("IoT Init\n");
	}
}

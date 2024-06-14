/************************************************************************************
	Project		: 전자식스위치
	File Name	: Queue.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "rs-485.h"

USART_TypeDef* COM_USART[MAX_PORT]		= {STPM3x_USART, DEBUG_USART, RS_485_USART}; 

//@tiny UART_QUEUE	Uart_RxQ[MAX_PORT], Uart_TxQ[MAX_PORT];
UART_QUEUE	Uart_RxQ[MAX_PORT], Uart_TxQ[MAX_PORT];


void Queue_Init(void)
{
	int i;
	
	for(i=0;i<MAX_PORT;i++)
	{
		Queue_Clear(i);
	}
}

void Queue_Clear(uint16_t port)
{
	RX_Queue_Clear(port);
	TX_Queue_Clear(port);
}

void RX_Queue_Clear(uint16_t port)
{
	Uart_RxQ[port].rptr	= 0;
	Uart_RxQ[port].wptr	= 0;
	Uart_RxQ[port].buff[0]	= 0;
}

void TX_Queue_Clear(uint16_t port)
{
	Uart_TxQ[port].rptr	= 0;
	Uart_TxQ[port].wptr	= 0;
	Uart_TxQ[port].buff[0]	= 0;
}

/* Receive Que read function */
uint8_t Uart_GetRxQ(uint16_t port, uint8_t *data)
{
	if(Uart_RxQ[port].rptr != Uart_RxQ[port].wptr)
	{
		*data = Uart_RxQ[port].buff[Uart_RxQ[port].rptr++];
		if(Uart_RxQ[port].rptr == UART_MAXQ)
		{
			Uart_RxQ[port].rptr = 0;	// loop back
		}
		return (1);
	}
	else 
	{
		return(0);
	}
}

/* Receive Que write function */
void Uart_PutRxQ(uint16_t port, uint8_t data)
{
	Uart_RxQ[port].buff[Uart_RxQ[port].wptr++] = data;
	if(Uart_RxQ[port].wptr == UART_MAXQ)
	{
		Uart_RxQ[port].wptr = 0;		// loop back
	}
}

/* Tx Que read function */
uint8_t Uart_GetTxQ(uint16_t port, uint8_t *data)
{
	if(Uart_TxQ[port].rptr != Uart_TxQ[port].wptr)
	{
		*data = Uart_TxQ[port].buff[Uart_TxQ[port].rptr++];
		if(Uart_TxQ[port].rptr == UART_MAXQ)
		{
			Uart_TxQ[port].rptr = 0;	//loop back
		}
		return (1);
	}
	else
	{
		return(0);
	}
}

uint8_t Uart_EmptyTxQ(uint16_t port)
{
	if(Uart_TxQ[port].rptr != Uart_TxQ[port].wptr)
	{
		return (0);
	}
	else
	{
		return(1);
	}
}

/* Tx Que write function */
void Uart_PutTxQ(uint16_t port, uint8_t data)
{
	Uart_TxQ[port].buff[Uart_TxQ[port].wptr] = data;
	Uart_TxQ[port].wptr++;
	//printf("%02x, ", data);
	if(Uart_TxQ[port].wptr == UART_MAXQ) 
	{
		Uart_TxQ[port].wptr = 0;	// loop back
	}
}

FlagStatus USART_ITConfigState(USART_TypeDef* USARTx, USART_IT_TypeDef USART_IT)
{
	FlagStatus status = DISABLE;
	
	uint8_t usartreg, itpos = 0x00;
	assert_param(IS_USART_CONFIG_IT(USART_IT));
	
	/* Get the USART IT index */
	itpos = (uint8_t)((uint8_t)1 << (uint8_t)((uint8_t)USART_IT & (uint8_t)0x0F));
	
	if((USARTx->CR2 & itpos) != (uint8_t)0x00)
	{
		status = ENABLE;
    }
    else
    {
		status = DISABLE;
    }
    return status;
}

void Uart_PutsTxQ(uint16_t port, uint8_t *buf, int len)
{
	int i;
	
	for(i=0; i<len; i++)
	{
		Uart_PutTxQ(port, buf[i]);
	}
	
	if(USART_ITConfigState(COM_USART[port], USART_IT_TC) == DISABLE)
	{
		if(port == RS_485_PORT)
		{
			Gu8_RS485_TX_Enable	= 1;
			GPIO_SetBits(RS_485_DE_PORT, RS_485_DE_PIN);			// Tx Active High
		}
		USART_ITConfig(COM_USART[port], USART_IT_TC, ENABLE);		// Enable the USART Transmit complete interrupt
	}
}

#ifndef __QUEUE_H
#define __QUEUE_H

#define UART_MAXQ	100		/* xx byte */

typedef enum 
{
  STPM3x_PORT = 0,
  DEBUG_PORT,
  RS_485_PORT,
  MAX_PORT
} com_port;   

/*
#define	IoT_PORT	0
#define	RS_485_PORT	1
#define	DEBUG_PORT	2
#define	MAX_PORT	3
*/
typedef struct {
    uint8_t  buff[UART_MAXQ];		/* data buffer */
    uint16_t           wptr;		/* write pointer */
    uint16_t           rptr;		/* read pointer */
}UART_QUEUE;

extern uint8_t Uart_GetRxQ(uint16_t port, uint8_t *data);
extern void Uart_PutRxQ(uint16_t port, uint8_t data);
extern uint8_t Uart_GetTxQ(uint16_t port, uint8_t *data);
extern void Uart_PutTxQ(uint16_t port, uint8_t data);
extern void Uart_PutsTxQ(uint16_t port, uint8_t *buf, int len);
extern uint8_t Uart_EmptyTxQ(uint16_t port);

extern void Queue_Init(void);
extern void Queue_Clear(uint16_t port);
extern void RX_Queue_Clear(uint16_t port);
extern void TX_Queue_Clear(uint16_t port);

extern USART_TypeDef* COM_USART[MAX_PORT];
extern FlagStatus USART_ITConfigState(USART_TypeDef* USARTx, USART_IT_TypeDef USART_IT);
//extern UART_QUEUE	Uart_RxQ[MAX_PORT], Uart_TxQ[MAX_PORT];

#endif
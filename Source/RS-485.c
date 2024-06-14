/************************************************************************************
	Project		: 전자식스위치
	File Name	: RS-485.C
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
#include "el_switch.h"
#include "Debug.h"
#include "Timer.h"
#include "led.h"
#include "WDGnBeep.h"

__IO uint8_t	Gu8_RS_485_Rx_Tmr	= 0;
__IO uint8_t	Gu8_RS_485_Tx_Tmr	= 0;

uint8_t			Gu8_RS485_TX_Enable	= 0;


uint16_t		Gu16_GAS_Off_Tmr;
uint8_t 		Gu8_Gas_Flag;

uint16_t		Gu16_Elevator_Tmr;
uint8_t			Gu8_ELEVATOR_Arrive_Flag;

uint8_t         Gu8_WallPad_Elevator_Call;					//엘리베이터 상태(0x00 : 평상시, 0x01 : 도착, 0x02 : Err 등)
uint8_t         Gu8_WallPad_Elevator_Floor;					//엘리베이터 현재 층(지상 0 ~ 99층(0x00 ~ 0x99), 지하 : B1 ~ B16(0xB1 ~ 0xBF))
uint8_t         Gu8_WallPad_Elevator_Number;				//엘리베이터 호수(0x00 ~ 0xFF)

__IO uint8_t	Gu8_RS_485_Result_Tx_Tmr = 0;

void CVNET_Process(uint8_t data);
//-------------------------------------------------------------------------------------------------------------------------
void irq_RS485_TX(void)
{
	uint8_t data;
	
	if(USART_GetITStatus(RS_485_USART, USART_IT_TC) != RESET)
	{
		if(Uart_GetTxQ(RS_485_PORT, &data) != 0)
		{
#ifdef _SAMSUNG_PROTOCOL_
			USART_SendData9(RS_485_USART, data);
#else
			USART_SendData8(RS_485_USART, data);
#endif
			USART_ClearITPendingBit(RS_485_USART, USART_IT_TC);
		}
		else
		{
			//Gu16_Check_Tmr	= 0;
			// Gu8_RS485_TX_Enable	= 0;
/*
			// Gu8_HD_RS485_TX_Enable = 0;
			GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);			// Receiver Active Low
*/
#ifdef	_KOCOM_PROTOCOL_		//211124
			Gu8_RS485_TX_Enable	= 0;
			GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);
#endif
			USART_ITConfig(RS_485_USART, USART_IT_TC, DISABLE);		// Disable the USART Transmit Complete interrupt
			//USART_ClearITPendingBit(RS_485_USART, USART_IT_TC);	// ???
		}
	}
}

void irq_RS485_RX(void)
{
	uint8_t data;
	
	if(USART_GetITStatus(RS_485_USART, USART_IT_RXNE) != RESET)
	{
		Gu8_RS_485_Rx_Tmr	= pG_Config->Protocol_IntervalTime;		// 6ms
#ifdef _SAMSUNG_PROTOCOL_
		data = (uint8_t)USART_ReceiveData9(RS_485_USART);
#else
		data = USART_ReceiveData8(RS_485_USART);
#endif
		Uart_PutRxQ(RS_485_PORT, data);								// Read one byte from the receive data register
	}
}
//-------------------------------------------------------------------------------------------------------------------------
void RS485_Init(void)
{
	CLK_PeripheralClockConfig((CLK_Peripheral_TypeDef)RS_485_PORT_CLK, ENABLE);	// Enable RS_485 USART clock
	
	SYSCFG_REMAPPinConfig(REMAP_Pin_USART3TxRxPortF, DISABLE);		// USART3_TX mapped on PG1 and USART3_RX mapped on PG0
	//SYSCFG_REMAPPinConfig(REMAP_Pin_USART3TxRxPortF, ENABLE);		// USART3_TX mapped on PF0 and USART3_RX mapped on PF1
	
	GPIO_ExternalPullUpConfig(RS_485_TX_PORT, RS_485_TX_PIN, ENABLE);			// RS_485 Tx as alternate function push-pull
	GPIO_ExternalPullUpConfig(RS_485_RX_PORT, RS_485_RX_PIN, ENABLE);			// RS_485 Rx as alternate function push-pull
#ifdef	_SAMSUNG_PROTOCOL_
	USART_Init(RS_485_USART, 9600, USART_WordLength_9b, USART_StopBits_1, USART_Parity_Even, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
#else
	USART_Init(RS_485_USART, 9600, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
#endif
	//-------------------------------------------------------------------------------------------------------------------------
	GPIO_Init(RS_485_DE_PORT, RS_485_DE_PIN, GPIO_Mode_Out_PP_Low_Fast);		// Output
	GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);		// Low
	//-------------------------------------------------------------------------------------------------------------------------
	USART_ITConfig(RS_485_USART, USART_IT_RXNE, ENABLE);
	//USART_ITConfig(RS_485_USART, USART_IT_TC, ENABLE);		// Enable the USART Transmit complete interrupt
	
	USART_Cmd(RS_485_USART, ENABLE);
	//-------------------------------------------------------------------------------------------------------------------------
	/*
	CLK_PeripheralClockConfig(CLK_Peripheral_DMA1, ENABLE);
	//DMA_DeInit(DMA1_Channel2);	// USART3_RX	RS_485
	DMA_Init(DMA1_Channel2, RS_485_DR_ADDRESS, RS_485_DR_ADDRESS, RS_485_BUFFER_SIZE,
			DMA_DIR_PeripheralToMemory,
			DMA_Mode_Circular,
			DMA_MemoryIncMode_Inc,
			DMA_Priority_High,
			DMA_MemoryDataSize_Byte );
	DMA_Cmd(DMA1_Channel2, ENABLE);
	DMA_GlobalCmd(ENABLE);					// DMA enable
	*/
	printf("RS-485 Init\n");
	
	//-------------------------------------------------------------------------------------------------------------------------
	Protocol_Data_Init();
}

void RS485_Process(void)
{
	uint16_t	i;
	uint8_t		data;
	uint8_t		touch_switch;
	//---------------------------------------------------------------------------------------------------------------------------------------
// #if defined	_KOCOM_PROTOCOL_ || defined _HDC_PROTOCOL_
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_
#ifndef _NO_PROTOCOL_	//연동 없을 때는 사용하지 않음
	BATCH_BLOCK_STATE_Process();		//코콤은 주기적으로 데이터를 받는지 알 수 없어서, CVNET과 달리 위치를 옮김.
#endif
#endif	//_ONE_SIZE_BATCH_BLOCK_MODEL_
// #endif	//_KOCOM_PROTOCOL_
	//---------------------------------------------------------------------------------------------------------------------------------------
	if(Gu8_RS485_TX_Enable == 0)		// 5000~10000번에 한번식 High로 유지되는 경우가 있어 적용함(많을 경우 1000번에 한번, Low 시키는 루틴은 정상적으로 실행됨)
	{
		if(GPIO_ReadInputDataBit(RS_485_DE_PORT, (GPIO_Pin_TypeDef)RS_485_DE_PIN))	// GPIO Read
		{
			GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);			// Receiver Active Low
		}
	}
	while(Uart_GetRxQ(RS_485_PORT, &data) != 0)
	{
		Protocol_Process(data);
	}
	//------------------------------------------------------------------------------------------
	RS485_IntervalTimeOver_RxdataClear(Gu8_RS_485_Rx_Tmr);	// // 마지막 데이터 수신 후 X ms 초과하면 데이터 클리어
	RS485_Tx_Process();
	//------------------------------------------------------------------------------------------
}


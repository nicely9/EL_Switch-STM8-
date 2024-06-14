/************************************************************************************
	Project		: 전자식스위치
	File Name	: RS-485_CVNET.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2021/07/13
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
#include "LCD.h"
#include "STPM3x_opt.h"

#ifdef _CVNET_PROTOCOL_

/*CVNET은 대기 연동 없이 전등 연동으로만 대기전력 스위치를 사용할 경우 조명+대기전력 프로토콜 사용하지 않고 조명 프로토콜 사용할 수도 있다.
예를들어 거실(ID1)이 네트워크 스위치이고 안방(ID2)가 대기전력 스위치라면 조명 대기전력 스위치를 조명 프로토콜 사용 해야한다.
물론 현장에 네트워크 스위치가 들어가지 않고, 모두 대기전력 스위치만 사용한다면 해당 경우는 어떤 프로토콜을 적용할 지 현장 혹은 CVNET 연구소 문의 해야한다.*/
/*

버전특성요구(0x22)				해당 기능 적용되지 않음. 구현 하지 않아도 됨.
그룹 제어(0x31 ~ 0x3E, 0x3F)	해당 기능 적용되지 않음. 구현 하지 않아도 됨.
CVNET은 월패드 옵션으로 일괄 소등 시 조명 상태를 저장하고 소등 해제 시 저장 된 조명 상태로 복구함.
일괄 스위치의 경우
일괄 차단 스위치 프로토콜에 없는 기능(ex 삼로 등)을 사용하지 않으면 일괄 차단 스위치 프로토콜을 사용해야함.
 >> 통합 스위치 프로토콜 사용하면 안됨.
통합 스위치 프로토콜에서 사용하지 않는 기능의 데이터면 0으로 처리해서 응답.
3로 전등의 경우 조명 프로토콜에서 응답도 제어도 하지 않는다. 그룹 제어에서도 하지 않는다. 김태운 프로 답변
*/

void CVNET_Data_Process(CVNET_BUF	*pRx);
// ----------------------------------------------------------------------------------------
static	CVNET_BUF		RxBuf, TxBuf;
#define ELEVATOR_TMR	162		//(180-18)3분 이상 엘리베이터 도착신호가 오지 않을 경우 버튼 상태를 초기화.
#define	MAX_CVNET_DATA_SEQUENCE		8
uint8_t	CVNET_LIGHT_ITEM_Sequence[MAX_CVNET_DATA_SEQUENCE];
uint8_t	CVNET_ELEC_ITEM_Sequence[MAX_CVNET_DATA_SEQUENCE];
uint8_t Gu8_3Way_Flag;
// ----------------------------------------------------------------------------------------
void SET_CVNET_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_CVNET_DATA_SEQUENCE)
	{
		CVNET_LIGHT_ITEM_Sequence[count]	= item;
	}
}

void SET_CVNET_ELEC_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_CVNET_DATA_SEQUENCE)
	{
		CVNET_ELEC_ITEM_Sequence[count]	= item;
	}
}

void Protocol_Data_Init(void)
{
	uint8_t	count;
	
	memset((void*)&RxBuf,		0,	sizeof(CVNET_BUF));
	memset((void*)&TxBuf,		0,	sizeof(CVNET_BUF));
	
	Gu16_GAS_Off_Tmr				= 0;
	
	Gu16_Elevator_Tmr			= 0;
	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_NON;
	
	memset(CVNET_LIGHT_ITEM_Sequence, 0, MAX_CVNET_DATA_SEQUENCE);	// 8개 항목 클리어
	memset(CVNET_ELEC_ITEM_Sequence, 0, MAX_CVNET_DATA_SEQUENCE);	// 8개 항목 클리어
	
	// 프로토콜 데이터 항목 순서
	// 전등모델	 최대항목	: 전등 6개, 전등 4개 + 디밍 2개
	// 전등+대기 최대항목	: 전등 4개 + 대기 2개, 전등2개 + 디밍2개 + 대기2개
	// ex) 조명 3개, 디밍 2개 = 조명1,조명2,조명3,디밍1,디밍2,0,0,0
	// ex) 조명 1개, 디밍 1개 = 조명1,디밍1,0,0,0,0,0,0
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// 프로토콜 순서 조명, 디밍전등 순으로
	count	= 0;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	// if(item2tsn(mapping_ITEM_3WAY_1))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);	//CVNET은 3로 프로토콜은 별도로 사용하므로 해당 함수로 사용 안함
	// if(item2tsn(mapping_ITEM_3WAY_2))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
#endif
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// 프로토콜 순서 디밍전등, 조명 순으로
	count	= 0;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	// if(item2tsn(mapping_ITEM_3WAY_1))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	// if(item2tsn(mapping_ITEM_3WAY_2))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
#endif
	
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// 대기전력 모델 
	count	= 0;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_CVNET_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_CVNET_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
#endif
}
// ----------------------------------------------------------------------------------------
void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr)
{
	if(tmr == 0)		// 마지막 데이터 수신 후 X ms 초과하면 데이터 클리어
	{
		RxBuf.buf[0]	= 0;
		RxBuf.count	    = 0;
	}
}
// ----------------------------------------------------------------------------------------
void RS485_Tx_Process(void)
{
	static uint8_t	_mode_	= 0;
	int i;
	
	/*if(Gu8_RS_485_Tx_Tmr == 0)		// 데이터 수신 후 10ms 이후 데이터 전송
	{
		if(TxBuf.send_flag)
		{
			TX_Queue_Clear(RS_485_PORT);
			Uart_PutsTxQ(RS_485_PORT, TxBuf.buf, TxBuf.count);
			
			if(G_Debug == DEBUG_HOST)
			{
				printf("TX(CVNET) : ");
				//printf("\nTX(CVNET %d) : ", (uint16_t)TxBuf.count);
				for(i=0;i<TxBuf.count;i++)
				{
					printf("%02X ", (uint16_t)TxBuf.buf[i]);
				}
				printf("\n");
			}
			TxBuf.count	= 0;
			TxBuf.send_flag	= 0;
		}
	}*/
	if(Gu8_RS_485_Tx_Tmr == 0 && Gu8_RS_485_Enable_Tmr == 0)
	{
		switch(_mode_)
		{
			case 0:
				if(TxBuf.send_flag)
				{
					Gu8_RS485_TX_Enable = 1;
					GPIO_SetBits(RS_485_DE_PORT, RS_485_DE_PIN);
					Gu8_RS_485_Enable_Tmr = 1;
					TIM2_Cmd(ENABLE);
					_mode_++;
				}
				break;
			case 1:
				TX_Queue_Clear(RS_485_PORT);
				Uart_PutsTxQ(RS_485_PORT, TxBuf.buf, TxBuf.count);
				if(G_Debug == DEBUG_HOST)
				{
					printf("TX(CVNET) : ");
					for(i=0;i<TxBuf.count;i++)
					{
						printf("%02X ", (uint16_t)TxBuf.buf[i]);
					}
					printf("\n");
				}
				Gu8_RS_485_Tx_Tmr	= 2;		// 약 2ms 이후에 USART_IT_TC 체크
				_mode_++;
				break;
			case 2:
				if(USART_ITConfigState(COM_USART[RS_485_PORT], USART_IT_TC) == DISABLE)
				{
					Gu8_RS_485_Enable_Tmr = 1;
					TIM2_Cmd(ENABLE);
					_mode_++;
				}
				break;
			case 3:
			default:
				Gu8_RS485_TX_Enable	= 0;
				GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);			// Receiver Active Low
				TxBuf.count	= 0;
				TxBuf.send_flag	= 0;
				_mode_	= 0;
				break;
		}
	}
}
// ----------------------------------------------------------------------------------------
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID | CVNET_LIGHT_DEVICE_ID);
}
#endif

#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
#ifdef _LIGHT_SWITCH_PROTOCOL_USE_
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID | CVNET_LIGHT_DEVICE_ID);
}
#else
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID | CVNET_LIGHT_ELEC_DEVICE_ID);
}
#endif
#endif

uint8_t Get_Batch_Block_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID | CVNET_BATCH_BLOCK_DEVICE_ID);	// 어드레스 설정시 : ID 11(0xB), ID 12(0xC), ID 13(0xD), ID 14(0xE)로 설정해야 함
}

uint8_t Get_Elevator_485_ID(void)
{
	return (uint8_t)((pG_Config->RS485_ID - 0x0A) | CVNET_BATCH_BLOCK_DEVICE_ID);	//엘리베이터 호출 관련은 ID가 0x0B부터 시작하지 않고 0x01부터 시작함.
}
uint8_t Get_TotalSwitch_485_ID(void)
{
	return (uint8_t)(CVNET_TOTAL_SWITCH_DEVICE_ID);
}
uint8_t Get_Threeway_485_ID(void)
{
	return (uint8_t)(pG_Config->RS485_ID | CVNET_THREEWAY_DEVICE_ID);
}
// ----------------------------------------------------------------------------------------
uint8_t CVNET_Crc(CVNET_BUF *pTRx, uint16_t len)
{
	uint8_t i, crc = 0;
	
	for(i = CVNET_FRAME_HD; i < len; i++)
	{
		crc += pTRx->buf[i];
	}
	
	return crc;
}
uint8_t NIS_Crc(CVNET_BUF *pTRx, uint8_t cal, uint8_t sel)		//생산시 485통신 테스트를 위해서 추가함.
{
	uint8_t i, crc = 0;
	uint8_t j = 0;

	if(sel == NIS_RX)		j = 1;
	else if(sel == NIS_TX)	j = 0;

	if(cal == 0)
	{
		for(i = 0; i < (uint8_t)(NIS_SUM_CRC - j); i++)
		{
			crc += pTRx->buf[i];
		}
	}
	else if(cal == 1)
	{
		for(i = 0; i < (uint8_t)(NIS_XOR_CRC - j); i++)
		{
			crc ^= pTRx->buf[i];
		}
	}
	return crc;
}
// ----------------------------------------------------------------------------------------
void RS_485_ID_RES(void)
{
	uint8_t Elec_Watt_MSB, Elec_Watt_LSB, prime_num_1, prime_num_2 = 0;
	CVNET_BUF	*pTx;
	pTx = &TxBuf;
	
	// Elec_Watt_MSB = (uint8_t)((int)Gu16_Elec_1_Watt + (int)Gu16_Elec_2_Watt);
	// if(G_Trace)	printf("Watt1 = %f Watt2 = %f\r\n", (double)Gu16_Elec_1_Watt, (double)Gu16_Elec_2_Watt);
	
	prime_num_1 = (uint8_t)((Gu16_Elec_1_Watt - (int)Gu16_Elec_1_Watt) * 10);
	prime_num_2	= (uint8_t)((Gu16_Elec_2_Watt - (int)Gu16_Elec_2_Watt) * 10);
	
	Elec_Watt_MSB = (uint8_t)((int)Gu16_Elec_1_Watt + (int)Gu16_Elec_2_Watt);
	Elec_Watt_LSB = (uint8_t)(prime_num_1 + prime_num_2);

	if(10 <= (prime_num_1 + prime_num_2))
	{
		Elec_Watt_MSB += 1;
		Elec_Watt_LSB -= 10;
	}

	pTx->buf[pTx->count++]	= 0xCC;
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//프로토콜 타입
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
	pTx->buf[pTx->count++]	= (uint8_t)(Get_485_ID() ^ CVNET_LIGHT_DEVICE_ID);					//ID 번호
#elif defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
	#ifdef _LIGHT_SWITCH_PROTOCOL_USE_
	pTx->buf[pTx->count++]	= (uint8_t)(Get_485_ID() ^ CVNET_LIGHT_DEVICE_ID);					//ID 번호
	#else
	pTx->buf[pTx->count++]	= (uint8_t)(Get_485_ID() ^ CVNET_LIGHT_ELEC_DEVICE_ID);				//ID 번호
	#endif
#elif defined(_ONE_SIZE_BATCH_BLOCK_MODEL_)
#ifdef _BATCH_BLOCK_SWITCH_PROTOCOL_
	pTx->buf[pTx->count++]	= (uint8_t)((Get_Batch_Block_485_ID() ^ CVNET_BATCH_BLOCK_DEVICE_ID) - 0x0A);	//ID 번호, 일괄스위치의 경우 ID가 0x8B부터 시작하므로 프로그램 응답을 위해 -0x0A함.
#else	//_TOTAL_SITWCH_PROTOCOL_
	pTx->buf[pTx->count++]	= (uint8_t)(pG_Config->RS485_ID);	//Total 스위치는 0xE1 하나이므로 바로 RS ID 값 리턴
#endif
#endif
	pTx->buf[pTx->count++]	= 0;

	pTx->buf[pTx->count++]	= Elec_Watt_MSB;				//전열1, 2의 전력 합(정수)
	pTx->buf[pTx->count++]	= Elec_Watt_LSB;				//전열1, 2의 전력 합(소수)

	pTx->buf[pTx->count++]	= Gu8_ZeroCrossing_Err_Flag;	//제로크로싱 동작 상태 1이면 Err, 0이면 Pass

	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 1, NIS_TX);
	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 0, NIS_TX);
	TxBuf.send_flag	= 1;
}
// ----------------------------------------------------------------------------------------
void Protocol_Process(uint8_t data)
{
	CVNET_BUF	*pRx;
	uint8_t		crc, crc_xor = 0;
	uint8_t     cnt = 0;
	int i;
	
	pRx = &RxBuf;
	
	if(G_Debug == DEBUG_HOST_REALDATA)
	{
		if(data == CVNET_STX)	printf("\n");
		printf("%02X ", (uint16_t)data);
	}

// #ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1개용 - 일괄차단스위치, 일괄만 있는 모델에서 실행되어 LED 에러가 출력되어 아래로 변경함.
/*#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
	BATCH_BLOCK_STATE_Process();		// 통신이 전혀 안되는 경우 이 함수는 실행되지 않음(즉 LED 상태를 전활할 수 없음
#endif*/
	
	switch(pRx->count)
	{
		default:
			if((pRx->buf[CVNET_FRAME_PRE] != CVNET_STX) && (data == CVNET_STX))
			{
				pRx->count = 0;
			}
			break;
		case 1:		// STX
			if((pRx->buf[CVNET_FRAME_PRE] != CVNET_STX) && (pRx->buf[CVNET_FRAME_PRE] != NIS_LIGHT_ID_COMM_1))
			{
				// printf("PRE\r\n");
				pRx->count = 0;
			}
			break;
		case 2:		// Header
			if((pRx->buf[CVNET_FRAME_HD] != CVNET_HEADER) && (pRx->buf[CVNET_FRAME_HD] != NIS_LIGHT_ID_COMM_2))
			{
				// printf("HD\r\n");
				pRx->count = 0;
			}
			break;
		case 3:		// DA
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
			if((pRx->buf[CVNET_FRAME_DA] != Get_485_ID()) && (pRx->buf[CVNET_FRAME_DA] != (Get_485_ID() | CVNET_GROUP_ID)) && (pRx->buf[CVNET_FRAME_DA] != Get_Threeway_485_ID()))
#endif
#if defined(_ONE_SIZE_BATCH_BLOCK_MODEL_)
			if((pRx->buf[CVNET_FRAME_DA] != Get_Batch_Block_485_ID()) && (pRx->buf[CVNET_FRAME_DA] != Get_Elevator_485_ID()) && (pRx->buf[CVNET_FRAME_DA] != Get_TotalSwitch_485_ID()))
#endif
			// if((pRx->buf[CVNET_FRAME_DA] != Get_485_ID()) && (pRx->buf[CVNET_FRAME_DA] != Get_Elevator_485_ID()) && (pRx->buf[CVNET_FRAME_DA] != (Get_485_ID() | CVNET_GROUP_ID)) \
			// && (pRx->buf[CVNET_FRAME_DA] != Get_TotalSwitch_485_ID()) && (pRx->buf[CVNET_FRAME_DA] != Get_Threeway_485_ID()))	// 내 주소가 아니고 그룹제어 주소도 아니면
			{
				if((pRx->buf[CVNET_FRAME_PRE] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[CVNET_FRAME_HD] != NIS_LIGHT_ID_COMM_2))
				{
					// printf("DA\r\n");
					//네트워크, Get_485_ID 21, 
					//일괄, Get_485_ID 8B, Elevator ID 81, Group 8F, Total E1, Threeway 91
					pRx->count = 0;
				}
			}
			break;
		case 4:		// SA
			if(pRx->buf[CVNET_FRAME_SA] != CVNET_WALLPAD_ID)		// 월패드 주소가 아니면
			{
				if((pRx->buf[CVNET_FRAME_PRE] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[CVNET_FRAME_HD] != NIS_LIGHT_ID_COMM_2))
				{
					// printf("SA\r\n");
					pRx->count = 0;
				}
			}
			break;
	}
	pRx->buf[pRx->count++] = data;
	
	if((pRx->buf[CVNET_FRAME_PRE] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[CVNET_FRAME_HD] == NIS_LIGHT_ID_COMM_2))
	{
		// printf("NIS ");
		if(pRx->count >= 8)
		{
			// printf("MAX BUF\r\n");
			crc = NIS_Crc(pRx, 0, NIS_RX);
			crc_xor = NIS_Crc(pRx, 1,NIS_RX);
			if(crc == pRx->buf[7] && crc_xor == pRx->buf[6])
			{
				if(G_Debug == DEBUG_HOST)
				{
					printf("\nRX(CVNET) : ");
					for(i=0;i<pRx->count;i++)
					{
						printf("%02X ", (uint16_t)pRx->buf[i]);
					}
					printf("\n");
				}
				CVNET_Data_Process(pRx);				
			}
			else
			{
				if(crc_xor != pRx->buf[6])	printf("cal crc_xor[0x%02X] != buf crc_xor[0x%02X]", (uint16_t)crc_xor, (uint16_t)pRx->buf[6]);
				if(crc != pRx->buf[7])		printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[7]);
			}
			pRx->buf[0] = 0;
			pRx->count = 0;			
		}
	}
	else
	{
		// printf("CVNET count = %d", (uint16_t)pRx->count);
		if(pRx->count >= CVNET_MAX_BUF)
		{
			// printf("MAX BUF\r\n");
			if(data == CVNET_ETX)
			{
				crc = CVNET_Crc(pRx, pRx->count-2);
				
				if(crc == pRx->buf[pRx->count-2])
				{
					if(G_Debug == DEBUG_HOST)
					{
						printf("\nRX(CVNET) : ");
						for(i=0;i<pRx->count;i++)
						{
							printf("%02X ", (uint16_t)pRx->buf[i]);
						}
						printf("\n");
					}
					CVNET_Data_Process(pRx);
				}
				else
				{
					printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[pRx->count-2]);
				}
			}
			pRx->buf[0] = 0;
			pRx->count = 0;
		}
	}
}

void SET_DimmingLevel(uint8_t item, uint8_t Dimming_Level)
{
	switch(item)
	{
		case mapping_ITEM_DIMMING_LIGHT_1:
			pG_State->Dimming_Level.Dimming1		= (uint8_t)Dimming_Level&0x0F;
			pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)(Dimming_Level>>4)&0x0F;
			if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
			}
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp1 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// 최대 크기로 설정
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			pG_State->Dimming_Level.Dimming2		= (uint8_t)Dimming_Level&0x0F;
			pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)(Dimming_Level>>4)&0x0F;
			if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
			}
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp2 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// 최대 크기로 설정
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
			break;
	}
	
	//if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)
}

void SET_Elec_Auto_Manual(uint8_t item, uint8_t flag)
{
	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
			if(flag == CVNET_AUTO_FLAG)	pG_State->ETC.Auto1	= 1;
			else						pG_State->ETC.Auto1	= 0;
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(flag == CVNET_AUTO_FLAG)	pG_State->ETC.Auto2	= 1;
			else						pG_State->ETC.Auto2	= 0;
			break;
	}
}

void CVNET_Control(uint8_t item, uint8_t control_value)
{
	uint8_t	Flag = OFF, touch_switch = 0, tmr;
	
	tmr	= Gu8_PowerSaving_Tmr;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	
	switch(item)	// 설정된 조명이 있으면
	{
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1개용 - 일괄차단스위치
		case mapping_ITEM_BATCH_LIGHT_OFF:		// 일괄소등
			touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
			if(control_value == CVNET_BATCH_LIGHT_SET)	// 0 일괄조명차단 설정(LED ON)
			{
				if(GET_Switch_State(touch_switch))	EventCtrl(touch_switch, OFF);	// 일괄조명 차단설정
			}
			else
			{
				if(GET_Switch_State(touch_switch) == 0)	EventCtrl(touch_switch, ON);	// 일괄조명 차단해제
			}
			break;
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_)
		case mapping_ITEM_GAS:					// 가스차단, 가스차단은 스위치 상태가 켜져 있어야 차단이 설정된 상태이다
			touch_switch	= item2tsn((uint8_t)mapping_ITEM_GAS);
			if(control_value == CVNET_GAS_CLOSE)		// 0 가스밸브	 닫힘(LED ON)
			{
				BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
			}
			else
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
			}
			break;
#endif	//#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_)			
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined (_ONE_SIZE_BATCH_LIGHT_n_ELEVATOR_n_3WAY_)
		case mapping_ITEM_ELEVATOR:
			touch_switch	= item2tsn(mapping_ITEM_ELEVATOR);
			if(control_value == CVNET_ELEVATOR_UP || control_value == CVNET_ELEVATOR_DOWN)
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL);
			}
			else if(control_value == CVNET_ELEVATOR_ARRIVE)
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);
			}
			else	//통신시작(0x0C), 평상시(0x00)
			{
				;
			}
			break;
#endif
#endif
#if defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		// case mapping_ITEM_3WAY_1:
		// case mapping_ITEM_3WAY_2:
			if(control_value == CVNET_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);
			}
			else if(control_value == CVNET_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);
			}
			break;
			
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
			if((control_value & 0x0F) == CVNET_OFF_FLAG)	// OFF는 색온도&레벨 설정없이 소등
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);
			}
			else if(control_value & 0x0F)
			{
				Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
				Gu8_LCD_DIM_Tmr					= 20;		// 2s 동안 LCD 표시
				Gu8_Dim_Flag = 1;
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);
				SET_DimmingLevel(item, control_value);
			}
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			if(control_value & 0x02)	// 자동,수동
			{
				SET_Elec_Auto_Manual(item, CVNET_AUTO_FLAG);
			}
			else
			{
				SET_Elec_Auto_Manual(item, CVNET_MANUAL_FLAG);
			}
			
			if(control_value & 0x01)	// ON,OFF
			{
				if(GET_Switch_State(item2tsn(item)) == 0)
				{
					touch_switch	= item2tsn(item);
					Flag	= ON;
					SET_Switch_State(touch_switch, Flag);
					SET_LED_State(touch_switch, Flag);
					Beep(Flag);
					PUT_RelayCtrl(item2ctrl(item), Flag);	// 항목기준 제어				
				}
			}
			else
			{
				if(GET_Switch_State(item2tsn(item)))
				{
					touch_switch	= item2tsn(item);
					Flag	= OFF;
					SET_Switch_State(touch_switch, Flag);
					SET_LED_State(touch_switch, Flag);
					Beep(Flag);
					PUT_RelayCtrl(item2ctrl(item), Flag);	// 항목기준 제어
				}
			}
			
			SET_SWITCH_Delay_OFF_Flag(item, 0);
			SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
			//----------------------------------------------------------------------------------------------------------
			ALL_Electricity_Switch_LED_Ctrl();
			//----------------------------------------------------------------------------------------------------------
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_MODEL_)
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
			if(control_value == CVNET_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);
			}
			else if(control_value == CVNET_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))	EventCtrl(item2tsn(item), OFF);
			}
			break;		
#endif

		default:
			Gu8_PowerSaving_Tmr			= tmr;	// 복구
			break;
	}
}

uint8_t CVNET_LIGHT_Data_Conversion(uint8_t item)
{
	uint8_t	ret;
	
	switch(item)
	{
		default:
			ret = CVNET_OFF_FLAG;
			break;
			
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		// case mapping_ITEM_3WAY_1:			//3로의 경우 3로 프로토콜을 통해서 따로 응답
		// case mapping_ITEM_3WAY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				ret = CVNET_ON_FLAG;
			}
			else
			{
				ret = CVNET_OFF_FLAG;
			}
			break;
			
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				if(pG_Config->Enable_Flag.PWM_Color_Temp)	//색 온도 사용할 때는 응답 시 색온도 레벨 응답하고, 색 온도 사용하지 않을 때에는 디밍 레벨만 응답함
				{
					ret = (uint8_t)(pG_State->Dimming_Level.Dimming1 | (pG_State->Color_Temp_Level.Color_Temp1<<4));
				}
				else
				{
					ret = (uint8_t)(pG_State->Dimming_Level.Dimming1);
				}
			}
			else
			{
				ret = CVNET_OFF_FLAG;
			}
			break;
			
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				if(pG_Config->Enable_Flag.PWM_Color_Temp)
				{
					ret = (uint8_t)(pG_State->Dimming_Level.Dimming2 | (pG_State->Color_Temp_Level.Color_Temp2<<4));
				}
				else
				{
					ret = (uint8_t)(pG_State->Dimming_Level.Dimming2);
				}
			}
			else
			{
				ret = CVNET_OFF_FLAG;
			}
			break;
	}
	
	return	ret;
}

uint8_t CVNET_ELEC_Data_Conversion(uint8_t item)
{
	uint8_t	ret;
	
	switch(item)
	{
		default:
			ret = 0x00;
			break;
			
		case mapping_ITEM_ELECTRICITY_1:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit1 자동/수동, bit0 전원 ON/OFF
			{
				ret	= (uint8_t)(CVNET_ON_FLAG | (pG_State->ETC.Auto1<<1));
			}
			else
			{
				ret	= (uint8_t)(CVNET_OFF_FLAG | (pG_State->ETC.Auto1<<1));	// CVNET_OFF_FLAG는 0이어서 의미는 없으나...
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit1 자동/수동, bit0 전원 ON/OFF
			{
				ret	= (uint8_t)(CVNET_ON_FLAG | (pG_State->ETC.Auto2<<1));
			}
			else
			{
				ret	= (uint8_t)(CVNET_OFF_FLAG | (pG_State->ETC.Auto2<<1));
			}
			break;
	}
	
	return	ret;
}
//------------------------------------------------------------------------------------------------------------------------------------------
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)		// 1개용, 2개용	- 전등모델

void CVNET_LIGHT_Model_Data_Res(CVNET_BUF *pRx, uint8_t CMD)
{
	uint8_t	item;
	CVNET_BUF	*pTx;
	pTx = &TxBuf;
	
	pTx->count	= CVNET_FRAME_PRE;
	pTx->buf[pTx->count++]	= CVNET_STX;
	pTx->buf[pTx->count++]	= CVNET_HEADER;
	pTx->buf[pTx->count++]	= CVNET_WALLPAD_ID;
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= CMD;
	
	if(CMD == CMD_LIGHT_CHARACTER_RES)
	{
		for(item = 0; item < MAX_CVNET_DATA_SEQUENCE; item++)
		{
			switch(CVNET_LIGHT_ITEM_Sequence[item])
			{
				case mapping_ITEM_LIGHT_1:
				case mapping_ITEM_LIGHT_2:
				case mapping_ITEM_LIGHT_3:
				case mapping_ITEM_LIGHT_4:
				case mapping_ITEM_LIGHT_5:
				case mapping_ITEM_LIGHT_6:
				// case mapping_ITEM_3WAY_1:	//3로의 경우 3로 프로토콜을 통해서 따로 응답
				// case mapping_ITEM_3WAY_2:
					pTx->buf[pTx->count++] = 0x01;
					break;
				case mapping_ITEM_DIMMING_LIGHT_1:
				case mapping_ITEM_DIMMING_LIGHT_2:
					pTx->buf[pTx->count++] = pG_Config->Dimming_MAX_Level;
					break;
				default:
					pTx->buf[pTx->count++] = 0x00;
					break;
			}
		}		
	}
	else
	{
		for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
		{
			pTx->buf[pTx->count++]	= CVNET_LIGHT_Data_Conversion(CVNET_LIGHT_ITEM_Sequence[item]);
		}
	}
	
	pTx->buf[pTx->count++]	= CVNET_Crc(pTx, pTx->count);
	pTx->buf[pTx->count++]	= CVNET_ETX;
	TxBuf.send_flag	= 1;
}

void ThreeWay_Data_Res(CVNET_BUF *pRx)
{
	CVNET_BUF	*pTx;
	pTx = &TxBuf;
	if(pRx->buf[CVNET_FRAME_D0] == CVNET_ON_FLAG || pRx->buf[CVNET_FRAME_D0] == CVNET_OFF_FLAG)	//D0 데이터가 ON, OFF 일 때만 응답. 토글 데이터 일 떄는 응답하지 않도록 함.
	{
		pTx->count	= CVNET_FRAME_PRE;
		pTx->buf[pTx->count++]	= CVNET_STX;
		pTx->buf[pTx->count++]	= CVNET_HEADER;
		pTx->buf[pTx->count++]	= CVNET_WALLPAD_ID;
		pTx->buf[pTx->count++]	= Get_Threeway_485_ID();
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | 0x80);

		pTx->buf[pTx->count++]	= GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;

		pTx->buf[pTx->count++]	= CVNET_Crc(pTx, pTx->count);
		pTx->buf[pTx->count++]	= CVNET_ETX;
		TxBuf.send_flag	= 1;
	}
}

void CVNET_Data_Process(CVNET_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item = 0;
	
    if(pRx->buf[CVNET_FRAME_DA] == (Get_485_ID() | CVNET_GROUP_ID))	// 그룹제어 어드레스(1개용, 2개용 전등모델만 그룹제어 있음)
    {
    	if(pRx->buf[CVNET_FRAME_CMD] != CMD_GROUP_CONTROL_REQ)				// 그룹제어 CMD
    	{
    		return;	// 전등모델이라도 그룹제어 CMD가 아니면 그룹제어는 없다
    	}
    }
	if(pRx->buf[CVNET_FRAME_PRE] == CVNET_FRAME_PRE)	//첫 패킷이 0xF7일 때만
	{
		if((pRx->buf[CVNET_FRAME_DA] & CVNET_LIGHT_DEVICE_ID) != CVNET_LIGHT_DEVICE_ID)	return;	//240322
	}

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	switch(pRx->buf[CVNET_FRAME_CMD])	// CMD
    {
    	case CMD_GROUP_CONTROL_REQ:								// 0x3F	그룹제어	- 응답없음
    		for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// 항목, 제어
    		}
    		break;
    		
    	case CMD_STATE_REQ:										// 0x01	상태요청
			if(pRx->buf[CVNET_FRAME_DA] == Get_485_ID())
			{
				CVNET_LIGHT_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// 상태요청 및 설정에 대한 응답 CMD(0x81)
			}
			else if(pRx->buf[CVNET_FRAME_DA] == Get_Threeway_485_ID())
			{
				if(item2tsn(mapping_ITEM_3WAY_1))
				{
					ThreeWay_Data_Res(pRx);
				}
				//3로 상태 응답
			}
			break;
			
    	case CMD_TOTAL_CONTROL_REQ:								// 0x1F	조명전체 제어
			for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
			{
				CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// 항목, 제어
			}
			CVNET_LIGHT_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// 상태요청 및 설정에 대한 응답 CMD(0x9F)
			break;
			
		case CMD_SELECTIVE_CONTROL_1_REQ:	// 0x11	조명 선택제어1		조명 모델
		case CMD_SELECTIVE_CONTROL_2_REQ:	// 0x12	조명 선택제어2		조명 모델
		case CMD_SELECTIVE_CONTROL_3_REQ:	// 0x13	조명 선택제어3		조명 모델
		case CMD_SELECTIVE_CONTROL_4_REQ:	// 0x14	조명 선택제어4		조명 모델
		case CMD_SELECTIVE_CONTROL_5_REQ:	// 0x15	조명 선택제어5		조명 모델
		case CMD_SELECTIVE_CONTROL_6_REQ:	// 0x16	조명 선택제어6		조명 모델
		case CMD_SELECTIVE_CONTROL_7_REQ:	// 0x17	조명 선택제어7		조명 모델
		case CMD_SELECTIVE_CONTROL_8_REQ:	// 0x18	조명 선택제어8		조명 모델
			if(pRx->buf[CVNET_FRAME_DA] == Get_485_ID())
			{
				item = CVNET_LIGHT_ITEM_Sequence[pRx->buf[CVNET_FRAME_CMD] - CMD_SELECTIVE_CONTROL_1_REQ];
				if(item)
				{
					CVNET_Control(item, pRx->buf[CVNET_FRAME_D0]);		// 항목, 제어
					CVNET_LIGHT_Model_Data_Res(pRx, CMD_SELECTIVE_CONTROL_1_8_RES);	// 전체응답
				}
			}
			else if(pRx->buf[CVNET_FRAME_DA] == Get_Threeway_485_ID())
			{
				if(pRx->buf[CVNET_FRAME_CMD] == CMD_SELECTIVE_CONTROL_1_REQ)
				{
					if(item2tsn(mapping_ITEM_3WAY_1))
					{
						if(pRx->buf[CVNET_FRAME_D0] == CVNET_ON_FLAG)
						{
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == 0)	EventCtrl(item2tsn(mapping_ITEM_3WAY_1), ON);
						}
						else if(pRx->buf[CVNET_FRAME_D0] == CVNET_OFF_FLAG)
						{
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))			EventCtrl(item2tsn(mapping_ITEM_3WAY_1), OFF);
						}
						ThreeWay_Data_Res(pRx);
						//3로 제어
					}
				}
			}
			break;
		case CMD_LIGHT_CHARACTER_REQ:
			CVNET_LIGHT_Model_Data_Res(pRx, CMD_LIGHT_CHARACTER_RES);
			break;			
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
			Store_CurrentConfig();
			printf("Switch ID Change\r\n");
		}*/
		RS_485_ID_RES();
	}
}
#endif	// defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)	// 2개용	- 전등+전열모델
#ifdef _LIGHT_SWITCH_PROTOCOL_USE_	//해당 define 사용 시 대기전력 스위치는 전등 프로토콜 사용함
void CVNET_LIGHT_Model_Data_Res(CVNET_BUF *pRx, uint8_t CMD)
{
	uint8_t	item;
	CVNET_BUF	*pTx;
	pTx = &TxBuf;
	
	pTx->count	= CVNET_FRAME_PRE;
	pTx->buf[pTx->count++]	= CVNET_STX;
	pTx->buf[pTx->count++]	= CVNET_HEADER;
	pTx->buf[pTx->count++]	= CVNET_WALLPAD_ID;
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= CMD;
	
	if(CMD == CMD_LIGHT_CHARACTER_RES)
	{
		for(item = 0; item < MAX_CVNET_DATA_SEQUENCE; item++)
		{
			switch(CVNET_LIGHT_ITEM_Sequence[item])
			{
				case mapping_ITEM_LIGHT_1:
				case mapping_ITEM_LIGHT_2:
				case mapping_ITEM_LIGHT_3:
				case mapping_ITEM_LIGHT_4:
				case mapping_ITEM_LIGHT_5:
				case mapping_ITEM_LIGHT_6:
				// case mapping_ITEM_3WAY_1:	//3로의 경우 3로 프로토콜을 통해서 따로 응답
				// case mapping_ITEM_3WAY_2:
					pTx->buf[pTx->count++] = 0x01;
					break;
				case mapping_ITEM_DIMMING_LIGHT_1:
				case mapping_ITEM_DIMMING_LIGHT_2:
					pTx->buf[pTx->count++] = pG_Config->Dimming_MAX_Level;
					break;
				default:
					pTx->buf[pTx->count++] = 0x00;
					break;
			}
		}		
	}
	else
	{
		for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
		{
			pTx->buf[pTx->count++]	= CVNET_LIGHT_Data_Conversion(CVNET_LIGHT_ITEM_Sequence[item]);
		}
	}
	
	pTx->buf[pTx->count++]	= CVNET_Crc(pTx, pTx->count);
	pTx->buf[pTx->count++]	= CVNET_ETX;
	TxBuf.send_flag	= 1;
}

void ThreeWay_Data_Res(CVNET_BUF *pRx)
{
	CVNET_BUF	*pTx;
	pTx = &TxBuf;
	if(pRx->buf[CVNET_FRAME_D0] == CVNET_ON_FLAG || pRx->buf[CVNET_FRAME_D0] == CVNET_OFF_FLAG)	//D0 데이터가 ON, OFF 일 때만 응답. 토글 데이터 일 떄는 응답하지 않도록 함.
	{
		pTx->count	= CVNET_FRAME_PRE;
		pTx->buf[pTx->count++]	= CVNET_STX;
		pTx->buf[pTx->count++]	= CVNET_HEADER;
		pTx->buf[pTx->count++]	= CVNET_WALLPAD_ID;
		pTx->buf[pTx->count++]	= Get_Threeway_485_ID();
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | 0x80);

		pTx->buf[pTx->count++]	= GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;
		pTx->buf[pTx->count++]	= 0;

		pTx->buf[pTx->count++]	= CVNET_Crc(pTx, pTx->count);
		pTx->buf[pTx->count++]	= CVNET_ETX;
		TxBuf.send_flag	= 1;
	}
}

void CVNET_Data_Process(CVNET_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item = 0;
	
    if(pRx->buf[CVNET_FRAME_DA] == (Get_485_ID() | CVNET_GROUP_ID))	// 그룹제어 어드레스(1개용, 2개용 전등모델만 그룹제어 있음)
    {
    	if(pRx->buf[CVNET_FRAME_CMD] != CMD_GROUP_CONTROL_REQ)				// 그룹제어 CMD
    	{
    		return;	// 전등모델이라도 그룹제어 CMD가 아니면 그룹제어는 없다
    	}
    }
	if(pRx->buf[CVNET_FRAME_PRE] == CVNET_FRAME_PRE)	//첫 패킷이 0xF7일 때만
	{
		if((pRx->buf[CVNET_FRAME_DA] & CVNET_LIGHT_DEVICE_ID) != CVNET_LIGHT_DEVICE_ID)	return;	//240322
	}

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	switch(pRx->buf[CVNET_FRAME_CMD])	// CMD
    {
    	case CMD_GROUP_CONTROL_REQ:								// 0x3F	그룹제어	- 응답없음
    		for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// 항목, 제어
    		}
    		break;
    		
    	case CMD_STATE_REQ:										// 0x01	상태요청
			if(pRx->buf[CVNET_FRAME_DA] == Get_485_ID())
			{
				CVNET_LIGHT_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// 상태요청 및 설정에 대한 응답 CMD(0x81)
			}
			else if(pRx->buf[CVNET_FRAME_DA] == Get_Threeway_485_ID())
			{
				if(item2tsn(mapping_ITEM_3WAY_1))
				{
					ThreeWay_Data_Res(pRx);
				}
				//3로 상태 응답
			}
			break;
			
    	case CMD_TOTAL_CONTROL_REQ:								// 0x1F	조명전체 제어
			for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
			{
				CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// 항목, 제어
			}
			CVNET_LIGHT_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// 상태요청 및 설정에 대한 응답 CMD(0x9F)
			break;
			
		case CMD_SELECTIVE_CONTROL_1_REQ:	// 0x11	조명 선택제어1		조명 모델
		case CMD_SELECTIVE_CONTROL_2_REQ:	// 0x12	조명 선택제어2		조명 모델
		case CMD_SELECTIVE_CONTROL_3_REQ:	// 0x13	조명 선택제어3		조명 모델
		case CMD_SELECTIVE_CONTROL_4_REQ:	// 0x14	조명 선택제어4		조명 모델
		case CMD_SELECTIVE_CONTROL_5_REQ:	// 0x15	조명 선택제어5		조명 모델
		case CMD_SELECTIVE_CONTROL_6_REQ:	// 0x16	조명 선택제어6		조명 모델
		case CMD_SELECTIVE_CONTROL_7_REQ:	// 0x17	조명 선택제어7		조명 모델
		case CMD_SELECTIVE_CONTROL_8_REQ:	// 0x18	조명 선택제어8		조명 모델
			if(pRx->buf[CVNET_FRAME_DA] == Get_485_ID())
			{
				item = CVNET_LIGHT_ITEM_Sequence[pRx->buf[CVNET_FRAME_CMD] - CMD_SELECTIVE_CONTROL_1_REQ];
				if(item)
				{
					CVNET_Control(item, pRx->buf[CVNET_FRAME_D0]);		// 항목, 제어
					CVNET_LIGHT_Model_Data_Res(pRx, CMD_SELECTIVE_CONTROL_1_8_RES);	// 전체응답
				}
			}
			else if(pRx->buf[CVNET_FRAME_DA] == Get_Threeway_485_ID())
			{
				if(pRx->buf[CVNET_FRAME_CMD] == CMD_SELECTIVE_CONTROL_1_REQ)
				{
					if(item2tsn(mapping_ITEM_3WAY_1))
					{
						if(pRx->buf[CVNET_FRAME_D0] == CVNET_ON_FLAG)
						{
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == 0)	EventCtrl(item2tsn(mapping_ITEM_3WAY_1), ON);
						}
						else if(pRx->buf[CVNET_FRAME_D0] == CVNET_OFF_FLAG)
						{
							if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))			EventCtrl(item2tsn(mapping_ITEM_3WAY_1), OFF);
						}
						ThreeWay_Data_Res(pRx);
						//3로 제어
					}
				}
			}
			break;
		case CMD_LIGHT_CHARACTER_REQ:
			CVNET_LIGHT_Model_Data_Res(pRx, CMD_LIGHT_CHARACTER_RES);
			break;			
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
			Store_CurrentConfig();
			printf("Switch ID Change\r\n");
		}*/
		RS_485_ID_RES();
	}
}
#else
void CVNET_LIGHT_ELEC_Model_Data_Res(CVNET_BUF *pRx, uint8_t CMD)
{
	uint8_t	item;
	CVNET_BUF	*pTx;
	pTx = &TxBuf;
	
	pTx->count	= CVNET_FRAME_PRE;
	pTx->buf[pTx->count++]	= CVNET_STX;
	pTx->buf[pTx->count++]	= CVNET_HEADER;
	pTx->buf[pTx->count++]	= CVNET_WALLPAD_ID;
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= CMD;
	
	switch(CMD)
	{
		case CMD_STATE_RES:						// 0x81	상태응답(전등1, 전등2, 전등3, 전등4, 00, 00, 전열1~4, 00
		case CMD_LIGHT_TOTAL_CONTROL_RES:		// 0x9E	조명 전체제어 응답(전등1, 전등2, 전등3, 전등4, 00, 00, 00, 00)
			for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
			{
				pTx->buf[pTx->count++]	= CVNET_LIGHT_Data_Conversion(CVNET_LIGHT_ITEM_Sequence[item]);
			}
			if(CMD == CMD_STATE_RES)			// 스위치 상태 요청일 경우
			{
				pTx->buf[CVNET_FRAME_D6]	= 0;
				//for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
				for(item=0;item<pG_Config->ElectricityCount;item++)
				{
					//if(CVNET_ELEC_ITEM_Sequence[item])
					{
						pTx->buf[CVNET_FRAME_D6]	|= (uint8_t)( CVNET_ELEC_Data_Conversion(CVNET_ELEC_ITEM_Sequence[item]) << (uint8_t)(2*item) );
					}
				}
			}
			break;
			
		case CMD_ELEC_TOTAL_CONTROL_RES:		// 0x9F	대기 전체제어 응답(대기1, 대기2, 대기3, 대기4, 00, 00, 00, 00)
			for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
			{
				pTx->buf[pTx->count++]	= CVNET_ELEC_Data_Conversion(CVNET_ELEC_ITEM_Sequence[item]);
			}
			break;
		
		case CMD_LIGHT_SELECTIVE_CONTROL_1_RES:	// 0x91~0x94 조명 선택제어 응답(조명, 00, 00, 00, 00, 00, 00, 00)
		case CMD_LIGHT_SELECTIVE_CONTROL_2_RES:
		case CMD_LIGHT_SELECTIVE_CONTROL_3_RES:
		case CMD_LIGHT_SELECTIVE_CONTROL_4_RES:
			for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
			{
				pTx->buf[pTx->count++]	= 0x00;
			}
			pTx->buf[CVNET_FRAME_D0]	= CVNET_ELEC_Data_Conversion(CVNET_LIGHT_ITEM_Sequence[CMD - CMD_LIGHT_SELECTIVE_CONTROL_1_RES]);
			break;
		
		case CMD_ELEC_SELECTIVE_CONTROL_1_RES:	// 0x95~0x98 대기 선택제어 응답(대기, 00, 00, 00, 00, 00, 00, 00)
		case CMD_ELEC_SELECTIVE_CONTROL_2_RES:
		case CMD_ELEC_SELECTIVE_CONTROL_3_RES:
		case CMD_ELEC_SELECTIVE_CONTROL_4_RES:
			for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
			{
				pTx->buf[pTx->count++]	= 0x00;
			}
			pTx->buf[CVNET_FRAME_D0]	= CVNET_ELEC_Data_Conversion(CVNET_ELEC_ITEM_Sequence[CMD - CMD_ELEC_SELECTIVE_CONTROL_1_RES]);
			break;
		case CMD_LIGHT_CHARACTER_RES:
			for(item = 0; item < MAX_CVNET_DATA_SEQUENCE; item++)
			{
				switch(CVNET_LIGHT_ITEM_Sequence[item])
				{
					case mapping_ITEM_LIGHT_1:
					case mapping_ITEM_LIGHT_2:
					case mapping_ITEM_LIGHT_3:
					case mapping_ITEM_LIGHT_4:
					case mapping_ITEM_LIGHT_5:
					case mapping_ITEM_LIGHT_6:
					// case mapping_ITEM_3WAY_1:
					// case mapping_ITEM_3WAY_2:
						pTx->buf[pTx->count++] = 0x01;
						break;
					case mapping_ITEM_DIMMING_LIGHT_1:
					case mapping_ITEM_DIMMING_LIGHT_2:
						pTx->buf[pTx->count++] = pG_Config->Dimming_MAX_Level;
						break;
				default:
					pTx->buf[pTx->count++] = 0x00;
					break;						
				}
			}
			break;
	}
	
	pTx->buf[pTx->count++]	= CVNET_Crc(pTx, pTx->count);
	pTx->buf[pTx->count++]	= CVNET_ETX;
	TxBuf.send_flag	= 1;
}

void CVNET_Data_Process(CVNET_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item = 0;
    
	if(pRx->buf[CVNET_FRAME_PRE] == CVNET_FRAME_PRE)	//첫 패킷이 0xF7일 때만
	{
		if((pRx->buf[CVNET_FRAME_DA] & CVNET_LIGHT_ELEC_DEVICE_ID) != CVNET_LIGHT_ELEC_DEVICE_ID)	return;
	}

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
	switch(pRx->buf[CVNET_FRAME_CMD])	// CMD
    {
    	/*
    	case CMD_GROUP_CONTROL_REQ:								// 그룹제어	- 응답없음
    		for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// 항목, 제어
    		}
    		break;
    	*/
    	case CMD_STATE_REQ:										// 0x01	상태요청
			CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// 상태요청 응답 CMD(0x81)
			break;
			
    	case CMD_LIGHT_TOTAL_CONTROL_REQ:						// 0x1E	조명 전체제어
    		for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// 항목, 제어
    		}
    		CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG));
			break;
			
		case CMD_ELEC_TOTAL_CONTROL_REQ:						// 0x1F	대기전력 전체제어
			for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_ELEC_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);		// 항목, 제어
    		}
    		CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG));
    		break;
    	
		case CMD_LIGHT_SELECTIVE_CONTROL_1_REQ:		// 0x11	조명 선택제어1		조명+대기전력 모델
		case CMD_LIGHT_SELECTIVE_CONTROL_2_REQ:		// 0x12	조명 선택제어2		조명+대기전력 모델
		case CMD_LIGHT_SELECTIVE_CONTROL_3_REQ:		// 0x13	조명 선택제어3		조명+대기전력 모델
		case CMD_LIGHT_SELECTIVE_CONTROL_4_REQ:		// 0x14	조명 선택제어4		조명+대기전력 모델
			item = CVNET_LIGHT_ITEM_Sequence[pRx->buf[CVNET_FRAME_CMD] - CMD_LIGHT_SELECTIVE_CONTROL_1_REQ];
			if(item)
			{
				CVNET_Control(item, pRx->buf[CVNET_FRAME_D0]);		// 항목, 제어
				CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG));
			}
			break;
			
		case CMD_ELEC_SELECTIVE_CONTROL_1_REQ:		// 0x15	대기 선택제어1		조명+대기전력 모델
		case CMD_ELEC_SELECTIVE_CONTROL_2_REQ:		// 0x16	대기 선택제어2		조명+대기전력 모델
		case CMD_ELEC_SELECTIVE_CONTROL_3_REQ:		// 0x17	대기 선택제어3		조명+대기전력 모델
		case CMD_ELEC_SELECTIVE_CONTROL_4_REQ:		// 0x18	대기 선택제어4		조명+대기전력 모델
			item = CVNET_ELEC_ITEM_Sequence[pRx->buf[CVNET_FRAME_CMD] - CMD_ELEC_SELECTIVE_CONTROL_1_REQ];
			if(item)
			{
				CVNET_Control(item, pRx->buf[CVNET_FRAME_D0]);		// 항목, 제어
				CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG));
			}
			break;
		case CMD_LIGHT_CHARACTER_REQ:
			CVNET_LIGHT_ELEC_Model_Data_Res(pRx, CMD_LIGHT_CHARACTER_RES);
			break;
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
			Store_CurrentConfig();
			printf("Switch ID Change\r\n");
		}*/
		RS_485_ID_RES();
	}
}
#endif	// defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
#endif
//------------------------------------------------------------------------------------------------------------------------------------------
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1개용 - 일괄차단스위치
//--------------------------------------일괄차단 스위치 프로토콜-----------------------------------------------------------------------------------------------
#ifdef _BATCH_BLOCK_SWITCH_PROTOCOL_	//일괄차단 스위치 프로토콜 사용할 때
//일괄차단 스위치와 통합 스위치는 다른 종류의 스위치로 인식하므로, 3로 같은 특이 사항이 있지 않으면 일괄 차단 스위치 프로토콜 사용 하도록 권장함(CVNET 연구소).
//Cvnet도 세대와 동일층에서 엘리베이터 호출 했을 경우 도착 데이터 받지 못하는 경우 있는것으로 보임.
void CVNET_BATCH_BLOCK_Data_Res(CVNET_BUF *pRx, uint8_t CMD)
{
	uint8_t	item, touch_switch;
	CVNET_BUF	*pTx;
	pTx = &TxBuf;
	
	pTx->count	= CVNET_FRAME_PRE;
	pTx->buf[pTx->count++]	= CVNET_STX;
	pTx->buf[pTx->count++]	= CVNET_HEADER;
	pTx->buf[pTx->count++]	= CVNET_WALLPAD_ID;
	pTx->buf[pTx->count++]	= Get_Batch_Block_485_ID();
	pTx->buf[pTx->count++]	= CMD;								// 상태응답(0x81), 	전체제어응답(0x9F), 선택제어응답(0x9F)
	
	// DATA0~7 = 일괄조명, 가스밸브, 00, 00, 00, 00, 00, 00
	touch_switch	= item2tsn((uint8_t)mapping_ITEM_BATCH_LIGHT_OFF);
	if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))	// 일괄소등은 스위치 상태가 꺼져 있어야 차단이 설정된 상태이다
	{
		pTx->buf[pTx->count++]	 = CVNET_BATCH_LIGHT_SET;		// 0 일괄조명차단 설정(LED ON);
	}
	else
	{
		pTx->buf[pTx->count++]	 = CVNET_BATCH_LIGHT_RELEASE;	// 1 일괄조명차단 해제(LED OFF)
	}
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_)
	touch_switch	= item2tsn((uint8_t)mapping_ITEM_GAS);
	if(GET_Switch_State(touch_switch) == ON || (GET_LED_State(touch_switch) == LED_FLASHING))	// 가스차단은 스위치 상태가 켜져 있어야 차단이 설정된 상태이다
	{
		pTx->buf[pTx->count++]	 = CVNET_GAS_CLOSE;				// 0 가스밸브	 닫힘(LED ON)
	}
	else
	{
		pTx->buf[pTx->count++]	 = CVNET_GAS_OPEN;				// 1 가스밸브 열림(LED OFF)
	}
#else		//일괄 스위치 모델 중 가스 없는 모델 일 때(ex) 일괄소등 1 모델) pTx 버퍼 카운트 수가 1 부족하므로 카운트 1 증가 처리
	pTx->buf[pTx->count++] = 0;	// DATA1
#endif		//#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_)
	pTx->buf[pTx->count++]	= 0;	// DATA2
	pTx->buf[pTx->count++]	= 0;	// DATA3
	pTx->buf[pTx->count++]	= 0;	// DATA4
	pTx->buf[pTx->count++]	= 0;	// DATA5
	pTx->buf[pTx->count++]	= 0;	// DATA6
	pTx->buf[pTx->count++]	= 0;	// DATA7
	pTx->buf[pTx->count++]	= CVNET_Crc(pTx, pTx->count);
	pTx->buf[pTx->count++]	= CVNET_ETX;
	TxBuf.send_flag	= 1;
}

void CVNET_Elevator_Data_Res(CVNET_BUF *pRx, uint8_t CMD)
{
	uint8_t	touch_switch;
	CVNET_BUF	*pTx;
	pTx = &TxBuf;
	touch_switch = item2tsn(mapping_ITEM_ELEVATOR);

	pTx->count	= CVNET_FRAME_PRE;
	pTx->buf[pTx->count++]	= CVNET_STX;
	pTx->buf[pTx->count++]	= CVNET_HEADER;
	pTx->buf[pTx->count++]	= CVNET_WALLPAD_ID;
	pTx->buf[pTx->count++]	= Get_Elevator_485_ID();
	pTx->buf[pTx->count++]	= CMD;								// 상태응답(0x81), 	전체제어응답(0x9F), 선택제어응답(0x9F)

	if(pRx->buf[CVNET_FRAME_CMD] == CMD_STATE_REQ)
	{
		if(GET_LED_State(touch_switch) == LED_FLASHING)	//호출 요청중 or 호출중이면
		{
			pTx->buf[pTx->count++]	= CVNET_ELEVATOR_DOWN_REQ;	// DATA0
		}
		else
		{
			if(GET_Switch_State(touch_switch))
			{
				pTx->buf[pTx->count++] = pRx->buf[CVNET_FRAME_D5];
			}
			else
			{
				pTx->buf[pTx->count++]	= CVNET_ELEVATOR_NORMAL;	// DATA0
			}
		}
	}
	else if(pRx->buf[CVNET_FRAME_CMD] == CMD_BATCHLIGHT_n_ELEVATOR_CONTROL_REQ)	//제어요구는 스위치에서 엘리베이터 콜 했을 경우에만 월패드에서 전달함. DA, SA, CMD 제외한 데이터는 똑같이 전송하면 된다고 함.
	{
		pTx->buf[pTx->count++]		= pRx->buf[CVNET_FRAME_D0];
	}
	pTx->buf[pTx->count++]	= pRx->buf[CVNET_FRAME_D1];	// DATA1
	pTx->buf[pTx->count++]	= pRx->buf[CVNET_FRAME_D2];	// DATA2
	pTx->buf[pTx->count++]	= pRx->buf[CVNET_FRAME_D3];	// DATA3
	pTx->buf[pTx->count++]	= pRx->buf[CVNET_FRAME_D4];	// DATA4
	pTx->buf[pTx->count++]	= pRx->buf[CVNET_FRAME_D5];	// DATA5
	pTx->buf[pTx->count++]	= pRx->buf[CVNET_FRAME_D6];	// DATA6
	pTx->buf[pTx->count++]	= pRx->buf[CVNET_FRAME_D7];	// DATA7
	pTx->buf[pTx->count++]	= CVNET_Crc(pTx, pTx->count);
	pTx->buf[pTx->count++]	= CVNET_ETX;
	TxBuf.send_flag	= 1;
}

void CVNET_Data_Process(CVNET_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item = 0;
    
	if(pRx->buf[CVNET_FRAME_PRE] == CVNET_FRAME_PRE)	//첫 패킷이 0xF7일 때만
	{
		if(pRx->buf[CVNET_FRAME_DA] != Get_Batch_Block_485_ID() && pRx->buf[CVNET_FRAME_DA] != Get_Elevator_485_ID())	return;
	}

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	switch(pRx->buf[CVNET_FRAME_CMD])	// CMD
    {	
    	case CMD_STATE_REQ:												// 0x01	상태요청
			if(pRx->buf[CVNET_FRAME_DA] == Get_Elevator_485_ID())
			{
				if(item2tsn(mapping_ITEM_ELEVATOR))
				{
					CVNET_Elevator_Data_Res(pRx, CMD_STATE_RES);
				}
			}
			else
			{
				if(item2tsn(mapping_ITEM_GAS))
				{
					if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)	CVNET_Control(mapping_ITEM_GAS, CVNET_GAS_CLOSE);
				}
				//윗줄 20220310 추가함. 스위치에서 가스 차단 요청 후, 월패드에서 가스차단이 성공했다는 데이터를 보내주는 것이 존재하지 않음. 임시로 응답 후 가스 차단으로 변하도록 함.
				//CVNET은 가스 상태에 대한 데이터는 선택 제어나 전체 제어에서만 있는것으로 프로토콜 문서에 나옴. 만약 가스 차단이 실패하면 어떻게 데이터를 받아야할지..
				CVNET_BATCH_BLOCK_Data_Res(pRx, CMD_STATE_RES);				// 0x81	상태요청 응답 CMD(0x81)
			}
			break;
			
    	case CMD_TOTAL_CONTROL_REQ:										// 0x1F	전체제어
			if(pRx->buf[CVNET_FRAME_DA] == Get_Batch_Block_485_ID())
			{
				CVNET_Control(mapping_ITEM_BATCH_LIGHT_OFF, pRx->buf[CVNET_FRAME_D0]);	// 일괄차단
				if(item2tsn(mapping_ITEM_GAS))
				{
					CVNET_Control(mapping_ITEM_GAS, pRx->buf[CVNET_FRAME_D1]);				// 가스차단
				}
				CVNET_BATCH_BLOCK_Data_Res(pRx, CMD_TOTAL_CONTROL_RES);		// 0x9F	전체제어 응답
			}
			break;
			
		case CMD_BATCHLIGHT_n_ELEVATOR_CONTROL_REQ:								// 0x11	일괄조명 제어		일괄차단 모델
		// case CMD_ELEVATOR_CONTROL_REQ:									// 0x11 엘리베이터 제어
			if(pRx->buf[CVNET_FRAME_DA] == Get_Elevator_485_ID())
			{
				if(item2tsn(mapping_ITEM_ELEVATOR))
				{
					// CVNET_Control(mapping_ITEM_ELEVATOR, pRx->buf[CVNET_FRAME_D0]);	//제어 데이터는 스위치에서 엘리베이터 호출했을 경우만 전송하므로 제어 데이터 받았을 때 엘리베이터 스위치 동작 할 필요 없음
					if(pRx->buf[CVNET_FRAME_D0] == CVNET_ELEVATOR_ARRIVE)			//도착 데이터면 엘리베이터 도착상태로
					{
						BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);					//호출 상태일 때만 도착 동작함
					}
					else if(pRx->buf[CVNET_FRAME_D0] & CVNET_ELEVATOR_TRANS)		//0x0C 운행정보 전달, 통신시작 데이터면 엘리베이터 콜 상태
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)
						{
							BATCH_BLOCK_Control(SET__ELEVATOR_CALL);						//제어 데이터를 받으면 스위치 -> 월패드로의 엘리베이터 호출 성공이므로 LED 상태 변경
						}
					}
					/*else if(pRx->buf[CVNET_FRAME_D0] == CVNET_ELEVATOR_DOWN)		//0x08 운행정보가 DOWN일 때만
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING || GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR)))				//호출 요청 후 통신 확인 뒤 호출중일때만 동작. 다만, 스위치는 층정보 표시 필요는 없음.
						{
							BATCH_BLOCK_Control(SET__ELEVATOR_CALL);						//제어 데이터를 받으면 스위치 -> 월패드로의 엘리베이터 호출 성공이므로 LED 상태 변경
						}
					}*/
					CVNET_Elevator_Data_Res(pRx, CMD_ELEVATOR_CONTROL_RES);
				}
			}
			else
			{
				if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF))
				{
					CVNET_Control(mapping_ITEM_BATCH_LIGHT_OFF, pRx->buf[CVNET_FRAME_D0]);
					CVNET_BATCH_BLOCK_Data_Res(pRx, CMD_TOTAL_CONTROL_RES);		// 0x9F	전체제어 응답
				}
			}
    		break;

		case CMD_GAS_CONTROL_REQ:										// 0x12	가스 제어		일괄차단 모델
			if(pRx->buf[CVNET_FRAME_DA] == Get_Batch_Block_485_ID())
			{
				if(item2tsn(mapping_ITEM_GAS))
				{
					CVNET_Control(mapping_ITEM_GAS, pRx->buf[CVNET_FRAME_D0]);
					CVNET_BATCH_BLOCK_Data_Res(pRx, CMD_TOTAL_CONTROL_RES);		// 0x9F	전체제어 응답
				}
			}
			break;
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
		/*if(pRx->buf[2] == 1)
		{
			if(pG_Config->RS485_ID != 0x0B)
			{
				pG_Config->RS485_ID = 0x0B;
				Store_CurrentConfig();
				printf("Switch ID Change\r\n");					
			}
		}
		else if(pRx->buf[2] == 2)
		{
			if(pG_Config->RS485_ID != 0x0C)
			{
				pG_Config->RS485_ID = 0x0C;
				Store_CurrentConfig();
				printf("Switch ID Change\r\n");					
			}
		}
		else if(pRx->buf[2] == 3)
		{
			if(pG_Config->RS485_ID != 0x0D)
			{
				pG_Config->RS485_ID = 0x0D;
				Store_CurrentConfig();
				printf("Switch ID Change\r\n");					
			}
		}
		else
		{
			if(pG_Config->RS485_ID != 0x0B)
			{
				pG_Config->RS485_ID = 0x0B;
				Store_CurrentConfig();
				printf("Switch ID Change\r\n");					
			}
		}*/
		RS_485_ID_RES();
	}
}
#endif
//--------------------------------------통합스위치 프로토콜-----------------------------------------------------------------------------------------------
#ifdef _TOTAL_SITWCH_PROTOCOL_
uint8_t CVNET_Total_Switch_State(uint8_t Data)
{
	uint8_t ret = 0;

	if(Data == CVNET_FRAME_D1)
	{
		ret = 0;
	}
	else if(Data == CVNET_FRAME_D2)
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == 0)	ret |= (1 << 4);
		else																ret |= (0 << 4);

		if(item2tsn(mapping_ITEM_GAS))
		{
			if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)) == ON || (GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING))		ret |= (1 << 0);
		else																															ret |= (0 << 0);
		}
		else
		{
			ret |= (0 << 0);
		}

		if(item2tsn(mapping_ITEM_3WAY_1))
		{
			if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))				ret |= (1 << 1);	//3로 ON 상태
			else															ret |= (0 << 1);	//3로 OFF 상태
		}
		else																ret |= (0 << 1);	
	}
	return ret;
}

void CVNET_Total_Switch_ACK(CVNET_BUF *pRx)
{
	CVNET_BUF *pTx;
	pTx = &TxBuf;

	pTx->count = CVNET_FRAME_PRE;
	pTx->buf[pTx->count++] = CVNET_STX;										//PRE
	pTx->buf[pTx->count++] = CVNET_HEADER;									//HD
	pTx->buf[pTx->count++] = CVNET_WALLPAD_ID;								//DA
	pTx->buf[pTx->count++] = Get_TotalSwitch_485_ID();						//SA
	pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | 0x80);	//CMD
	//------------------------------------------------------------D0, 보안------------------------------------------------------------------------
	pTx->buf[pTx->count++] = 0;
	//------------------------------------------------------------D1, 외출/주차정보 요청------------------------------------------------------------
	pTx->buf[pTx->count++] = CVNET_Total_Switch_State(CVNET_FRAME_D1);
	//------------------------------------------------------------D2, 일괄소등, 가스차단, 삼로--------------------------------------------------------------
	pTx->buf[pTx->count++] = CVNET_Total_Switch_State(CVNET_FRAME_D2);
	//------------------------------------------------------------D3, 난방-------------------------------------------------------------------------
	pTx->buf[pTx->count++] = 0;
	//------------------------------------------------------------D4, 대기전력 콘센트/스위치---------------------------------------------------------
	pTx->buf[pTx->count++] = 0;
	//------------------------------------------------------------D5, D6, D7 엘리베이터-------------------------------------------------------------
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	//
		{
			pTx->buf[pTx->count++] = CVNET_ELEVATOR_DOWN_REQ;
		}
		else																	
		{
			pTx->buf[pTx->count++] = pRx->buf[CVNET_FRAME_D5];
		}
		pTx->buf[pTx->count++] = pRx->buf[CVNET_FRAME_D6];						//D6, 엘리베이터
		pTx->buf[pTx->count++] = pRx->buf[CVNET_FRAME_D7];						//D7, 엘리베이터
	}
	//패킷 체크 해본 결과 월패드에서 엘베 호출 했을 경우에는 D5(엘리베이터 상태) 데이터 0으로 옴.
	else	//230802, 엘리베이터 사용하지 않는 모델에서는 받은 데이터 그대로 응답(임시)
	{
		pTx->buf[pTx->count++] = pRx->buf[CVNET_FRAME_D5];
		pTx->buf[pTx->count++] = pRx->buf[CVNET_FRAME_D6];
		pTx->buf[pTx->count++] = pRx->buf[CVNET_FRAME_D7];
	}
	////--------------------------------------------------------------------------------------------------------------------------------------------
	pTx->buf[pTx->count++] = 0;												//FEC
	pTx->buf[pTx->count++] = CVNET_ETX;										//ETX	
	pTx->buf[CVNET_FRAME_FEC] = CVNET_Crc(pTx, pTx->count-2);
	TxBuf.send_flag	= 1;
}

void CVNET_Data_Process(CVNET_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		touch_switch, item = 0;

	if(pRx->buf[CVNET_FRAME_DA] == CVNET_TOTAL_SWITCH_DEVICE_ID)
	{
		Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
		
		if((pRx->buf[CVNET_FRAME_D2] & 0x80) == 0x80)		//
		{
			touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
			if((pRx->buf[CVNET_FRAME_D2] & 0x10) == 0x10)
			{
				if(GET_Switch_State(touch_switch))	EventCtrl(touch_switch, OFF);	// 일괄조명 차단설정
				// BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);	//1001 0000
			}
			else
			{
				if(GET_Switch_State(touch_switch) == 0)	EventCtrl(touch_switch, ON);	// 일괄조명 차단설정
				// BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);	//1000 0000
			}
		}
	
		if(item2tsn(mapping_ITEM_3WAY_1))
		{
			if(Gu8_3Way_Flag)	//직접 스위치 제어 했을 때는 제어 없음.
			{
				/*if(Gu8_3Way_Flag == 0x02 && (pRx->buf[CVNET_FRAME_D2] & 0x02) == 0x02)	//직접 제어해서 OFF인 상태, 월패드에서 ON 제어 오면
				{
					BATCH_BLOCK_Control(SET__THREEWAY_ON);
					printf("OFF -> ON\r\n");
				}
				else if(Gu8_3Way_Flag == 0x01 && (pRx->buf[CVNET_FRAME_D2] & 0x02) == 0x00)	//직접 제어해서 ON인 상태, 월패드에서 OFF 제어 오면
				{
					BATCH_BLOCK_Control(SET__THREEWAY_OFF);
					printf("ON -> OFF\r\n");
				}*/	//240418
				Gu8_3Way_Flag = 0;
			}
			else	//직접 스위치 제어 하지 않음
			{
				if((pRx->buf[CVNET_FRAME_D2] & 0x02) == 0x02)
				{
					printf("OFF -> ON\r\n");
					BATCH_BLOCK_Control(SET__THREEWAY_ON);
				}
				else
				{
					printf("ON -> OFF\r\n");
					BATCH_BLOCK_Control(SET__THREEWAY_OFF);
				}
			}
		}
		if(item2tsn(mapping_ITEM_GAS))
		{
			if((pRx->buf[CVNET_FRAME_D2] & 0x01) == 0x01)
			{
				if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) != LED_FLASHING)
				{
					BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
				}
			}
			else
			{
				BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
			}
		}
		if(item2tsn(mapping_ITEM_ELEVATOR))
		{
			if((pRx->buf[CVNET_FRAME_D5] & 0x80) == CVNET_ELEVATOR_ARRIVE)
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);
			}
			else if(pRx->buf[CVNET_FRAME_D5] & CVNET_ELEVATOR_TRANS)
			{
				if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)
				{
					BATCH_BLOCK_Control(SET__ELEVATOR_CALL);
				}
			}
		}
		CVNET_Total_Switch_ACK(pRx);
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
			Store_CurrentConfig();
			printf("Switch ID Change\r\n");
		}*/
		RS_485_ID_RES();
	}	
}
#endif
//------------------------------------------------------------------------------------------------------------------------------------------
void BATCH_BLOCK_STATE_Process(void)
{
	if(item2tsn(mapping_ITEM_GAS))
	{
		if(Gu16_GAS_Off_Tmr == 0)
		{
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_GAS)) == LED_FLASHING)		// 프로토콜에 대해 설정이 없으면 OPEN 상태로 전환
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
			}
		}
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))		//호출 요청 or 호출 중일 때
	{
		if(Gu16_Elevator_Tmr == 0)	//CVNET은 호출 후 3분 이내 도착 데이터가 오지 않아도 상태 초기화 해야함.
		{
			if((GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING) || (GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR))))
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);
			}
		}
	}	
}
#endif	// _ONE_SIZE_BATCH_BLOCK_MODEL_

void BATCH_BLOCK_Control(uint8_t control)
{
	uint8_t	Flag;
	uint8_t	touch_switch;
	
	switch(control)
	{
		case SET__GAS_CLOSE_REQUEST:		// 가스차단은 스위치 상태가 켜져 있어야 차단이 설정된 상태이다
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) == OFF || GET_LED_State(touch_switch) == LED_FLASHING)	// 가스벨브가 열려(OFF)있거나, 차단 요청(LED_FLASHING)중일 경우
			{
				Gu16_GAS_Off_Tmr	= 60;		// 60초 경과 후 LED 소등(단 월패드에서 close/open 데이터가 수신되면 해당 상태로 전환)
				SET_Switch_State(touch_switch, OFF);	//20220310 요청시에는 스위치 OFF로 바꿈.
				SET_LED_State(touch_switch, LED_FLASHING);
				Beep(ON);
			}
			else
			{
				Beep(OFF);
			}
			break;
			
		case SET__GAS_CLOSE_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) == 0)		//요청중이거나, 차단 해제  상태일때만,
			{
				SET_Switch_State(touch_switch, ON);		// 가스밸브 차단(닫힘)
				SET_LED_State(touch_switch, OFF);		// 실제로는 LED 켜짐
				Beep(ON);
			}
			break;
			
		case SET__GAS_OPEN_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) || (GET_LED_State(touch_switch) == LED_FLASHING))			//차단중일때만
			{
				SET_Switch_State(touch_switch, OFF);	// 가스밸브 열림
				SET_LED_State(touch_switch, ON);		// 실제로는 LED 꺼짐
				Beep(OFF);
			}
			break;
		case SET__ELEVATOR_REQUEST:																			//스위치에서 엘리베이터 콜
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))		//스위치 OFF 거나 LED 상태 Flashing이면
			{
				Gu16_Elevator_Tmr = ELEVATOR_TMR;															//CVNET은 호출 후 3분 이내 도착 신호를 받지 못하면 스위치 초기화 
				SET_Switch_State(touch_switch, OFF);														//스위치 OFF
				SET_LED_State(touch_switch, LED_FLASHING);													//LED Flashing
				Beep(ON);
				// printf("ELEVATOR REQEUST\r\n");
			}
			break;			
		case SET__ELEVATOR_CALL:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == 0)			//호출 요청 상태거나, 호출상태 아닐 때.
			{
				Gu16_Elevator_Tmr = ELEVATOR_TMR;															//CVNET은 호출 후 3분 이내 도착 신호를 받지 못하면 스위치 초기화 함.
				SET_Switch_State(touch_switch, ON);															//스위치 ON
				SET_LED_State(touch_switch, OFF);															//LED ON
				Beep(ON);
			}
			break;
		case SET__ELEVATOR_ARRIVE:
		case SET__ELEVATOR_CALL_FAIL:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			/*if(control == SET__ELEVATOR_ARRIVE)
			{
				if(GET_Switch_State(touch_switch))			//호출 상태일때
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, ON);				//도착 or 호출 실패시 LED OFF					
					Beep(ON);
					// printf("ELEVATOR ARRIVE\r\n");
				}
			}
			else if(control == SET__ELEVATOR_CALL_FAIL)
			{
				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch))	//호출 요청 상태거나, 호출상태 일 때
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, ON);				//도착 or 호출 실패시 LED OFF				
					// Beep(ON);									//엘리베이터 프로토콜은 연동테스트 되지 않아, 어떻게 동작하는지 모르므로 일단 부저는 삭제함.
				}
				// printf("ELEVATOR FAIL\r\n");
			}*/
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch))	//호출 요청 상태거나, 호출상태 일 때
			{
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, ON);				//도착 or 호출 실패시 LED OFF
#ifdef _COMMAX_PROTOCOL_
				Beep(ON);										//코맥스는 실질적으로 도착 데이터 사용하지 않는다고 하여 둘다 부저음 사용함.
#else
				if(control == SET__ELEVATOR_ARRIVE)				//도착에만 부저음 있음. 
				{
					Beep(ON);
				}
#endif
			}
			break;
		case SET__THREEWAY_ON:
		case SET__THREEWAY_OFF:
			touch_switch = item2tsn(mapping_ITEM_3WAY_1);
#ifdef COMM_THREEWAY
			if(control == SET__THREEWAY_ON)
			{
				if(GET_Switch_State(touch_switch) == 0)
				{
					SET_Switch_State(touch_switch, ON);
					SET_LED_State(touch_switch, ON);
					Beep(ON);
				}				
			}
			else
			{
				if(GET_Switch_State(touch_switch))
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, OFF);
					Beep(OFF);
				}				
			}
#else		//통신삼로X, 입선삼로O, 스위치 끼리 삼로 와이어 연결 시 사용
			SET_LED_State(touch_switch, LED_DETECT_ON);
			Flag = SET_Switch_State(touch_switch, INVERSE);
			PUT_RelayCtrl(item2ctrl(tsn2item(touch_switch)), Flag);
#endif
			break;		
	}
}
//------------------------------------------------------------------------------------------------------------------------------------------
void Elevator_Process(void)
{
	;
}
//------------------------------------------------------------------------------------------------------------------------------------------
void ELEVATOR_Call(void)
{
    ;
}
//------------------------------------------------------------------------------------------------------------------------------------------
void ELEVATOR_Cancel(void)
{
   ;
}
// ----------------------------------------------------------------------------------------


#endif	// _CVNET_PROTOCOL_

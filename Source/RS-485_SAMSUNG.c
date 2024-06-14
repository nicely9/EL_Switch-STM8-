/************************************************************************************
	Project		: 전자식스위치
	File Name	: RS-485_SAMSUNG.C
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

#ifdef _SAMSUNG_PROTOCOL_

/*#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1개용 - 일괄차단스위치
void BATCH_BLOCK_STATE_Process(void);
#endif*/
uint8_t SAMSUNG_Batch_Light_State(uint8_t item);
void SAMSUNG_Data_Process(SAMSUNG_BUF	*pRx);
void BATCH_BLOCK_Control(uint8_t control);
// ----------------------------------------------------------------------------------------
static	SAMSUNG_BUF		RxBuf, TxBuf;

#define	MAX_SAMSUNG_DATA_SEQUENCE		9		//전등 종류 수는 최대 8이지만 SAMSUNG의 경우 전등 no를 1부터 시작하므로 8이아니라 최대 9로 설정함.
uint8_t	SAMSUNG_LIGHT_ITEM_Sequence[MAX_SAMSUNG_DATA_SEQUENCE];
uint8_t	SAMSUNG_ELEC_ITEM_Sequence[MAX_SAMSUNG_DATA_SEQUENCE];
uint8_t SAMSUNG_DIMMING_LIGHT_ITEM_Sequence[MAX_SAMSUNG_DATA_SEQUENCE];
uint8_t Store_Light_State[MAX_SAMSUNG_DATA_SEQUENCE];		//일괄 소등시 상태 저장
uint8_t old_batch_state, old_gas_state, old_elevator_state, batch_state, gas_state, elevator_state;
uint8_t Gu8_BatchLight_OFF_Flag, Gu8_BatchLight_OFF_Flag_2;
uint8_t Gu8_Gas_Request_Flag = 0;
// ----------------------------------------------------------------------------------------
void SET_SAMSUNG_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)				//일반전등 디밍전등 모두 포함
{
	if(count < MAX_SAMSUNG_DATA_SEQUENCE)
	{
		SAMSUNG_LIGHT_ITEM_Sequence[count]	= item;
	}
}

void SET_SAMSUNG_ELEC_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_SAMSUNG_DATA_SEQUENCE)
	{
		SAMSUNG_ELEC_ITEM_Sequence[count]	= item;
	}
}
void SET_SAMSUNG_DIMMING_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)		//디밍 전등만 따로 상태 값 사용할 부분이 있어서 추가함.
{
	if(count < MAX_SAMSUNG_DATA_SEQUENCE)
	{
		SAMSUNG_DIMMING_LIGHT_ITEM_Sequence[count]	= item;
	}
}
void Protocol_Data_Init(void)
{
	uint8_t	count, i;
	
	memset((void*)&RxBuf,		0,	sizeof(SAMSUNG_BUF));
	memset((void*)&TxBuf,		0,	sizeof(SAMSUNG_BUF));
	
	Gu16_GAS_Off_Tmr				= 0;
	
	Gu16_Elevator_Tmr			= 0;
	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_NON;

	Gu8_BatchLight_OFF_Flag  = 0;
	Gu8_BatchLight_OFF_Flag_2 = 0;

#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
	old_batch_state		= GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF));
	if(item2tsn(mapping_ITEM_GAS))		old_gas_state		= GET_LED_State(item2tsn(mapping_ITEM_GAS));		//요청 : FLASHING, 차단 : LED ON, 열림 : LED_OFF
	if(item2tsn(mapping_ITEM_ELEVATOR))	old_elevator_state	= GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR));	//요청 : FLASHING, 차단 : LED ON, 열림 : LED_OFF
#endif
	
	memset(SAMSUNG_LIGHT_ITEM_Sequence, 0, MAX_SAMSUNG_DATA_SEQUENCE);	// 8개 항목 클리어
	memset(SAMSUNG_ELEC_ITEM_Sequence, 0, MAX_SAMSUNG_DATA_SEQUENCE);	// 8개 항목 클리어
	
	// 프로토콜 데이터 항목 순서
	// 전등모델	 최대항목	: 전등 6개, 전등 4개 + 디밍 2개
	// 전등+대기 최대항목	: 전등 4개 + 대기 2개, 전등2개 + 디밍2개 + 대기2개
	// ex) 조명 3개, 디밍 2개 = 조명1,조명2,조명3,디밍1,디밍2,0,0,0
	// ex) 조명 1개, 디밍 1개 = 조명1,디밍1,0,0,0,0,0,0
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// 프로토콜 순서 조명, 디밍전등 순으로
	count	= 1;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
#endif
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// 프로토콜 순서 디밍전등, 조명 순으로
	count	= 1;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_SAMSUNG_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
#endif
	
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// 대기전력 모델 
	count	= 1;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_SAMSUNG_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_SAMSUNG_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
	count = 1;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_SAMSUNG_DIMMING_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_SAMSUNG_DIMMING_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);	
#endif

	for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
	{
		Store_Light_State[i]	= SAMSUNG_Batch_Light_State(SAMSUNG_LIGHT_ITEM_Sequence[i]);
	}
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
				printf("TX(SAMSUNG) : ");
				//printf("\nTX(SAMSUNG %d) : ", (uint16_t)TxBuf.count);
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
					printf("TX(SAMSUNG) : ");
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
uint8_t SAMSUNG_Crc(SAMSUNG_BUF *pTRx, uint8_t len)
{
	uint8_t i, crc = 0;
	
	for(i = SAMSUNG_F_HEADER; i < len; i++)		//header부터 checksum 앞까지 xor 연산
	{
		crc ^= pTRx->buf[i];
	}
	// crc ^= 0x80;		//7bit는 항상 0
	crc &= 0x7F;

	return crc;
}
uint8_t NIS_Crc(SAMSUNG_BUF *pTRx, uint8_t cal, uint8_t sel)		//생산시 485통신 테스트를 위해서 추가함.
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
void BCD_CODE(uint16_t input, uint8_t *MSB, uint8_t *LSB)
{
	int shift;
	uint16_t bcd = 0;
	// printf("input = 0x%x, %d\r\n", input, input);

	for(shift = 0; input > 0; shift++, input /= 10)
	{
		bcd |= input % 10 << (shift << 2);
	}
	*MSB = (uint8_t)(bcd >> 8);
	*LSB = (uint8_t)(bcd & 0xFF);
	printf("IN : 0x%x\r\n", (uint16_t)input);
	printf("BCD : 0x%x\r\n", (uint16_t)bcd);
    printf("MSB : 0x%x\r\n", (uint16_t)*MSB);
    printf("LSB : 0x%x\r\n", (uint16_t)*LSB);
}
// ----------------------------------------------------------------------------------------
uint8_t Light_Cnt(void)		//전등 수 카운트
{
	uint8_t count = 0;
	
	if(item2tsn(mapping_ITEM_LIGHT_1))	count++;
	if(item2tsn(mapping_ITEM_LIGHT_2))	count++;
	if(item2tsn(mapping_ITEM_LIGHT_3))	count++;
	if(item2tsn(mapping_ITEM_LIGHT_4))	count++;
	if(item2tsn(mapping_ITEM_LIGHT_5))	count++;
	if(item2tsn(mapping_ITEM_LIGHT_6))	count++;
	if(item2tsn(mapping_ITEM_3WAY_1))	count++;
	if(item2tsn(mapping_ITEM_3WAY_2))	count++;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	count++;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	count++;
	// printf("light count = %d\r\n", (uint16_t)count);
	return count;
}

uint8_t Elec_Cnt(void)		//전열 수 카운트
{
	uint8_t count = 0;
	
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	count++;
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	count++;
	
	return count;
}

void RS_485_ID_RES(void)
{
	uint8_t Elec_Watt_MSB, Elec_Watt_LSB, prime_num_1, prime_num_2 = 0;
	SAMSUNG_BUF	*pTx;
	pTx = &TxBuf;
	
	Elec_Watt_MSB = (uint8_t)((int)Gu16_Elec_1_Watt + (int)Gu16_Elec_2_Watt);
	printf("Watt1 = %f Watt2 = %f\r\n", (double)Gu16_Elec_1_Watt, (double)Gu16_Elec_2_Watt);
	
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
	pTx->buf[pTx->count++]	= Get_485_ID();					//ID 번호
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
	SAMSUNG_BUF	*pRx;
	uint8_t		crc, crc_xor = 0;
	uint8_t     cnt = 0;
	int i;
	
	pRx = &RxBuf;
	
	if(G_Debug == DEBUG_HOST_REALDATA)
	{
		// if(data == 0xAC)	printf("\n");
		if((data & 0xA0) == 0xA0 || (data == 0x5A && pRx->count == 0))	printf("\n");	//240411
		printf("%02X ", (uint16_t)data);
	}

/*#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1개용 - 일괄차단스위치
	BATCH_BLOCK_STATE_Process();		// 통신이 전혀 안되는 경우 이 함수는 실행되지 않음(즉 LED 상태를 전환할 수 없음
#endif*/
	
	switch(pRx->count)
	{
		default:
			if(((pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_LIGHT_HEADER) && (data == SAMSUNG_LIGHT_HEADER)) && ((pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_BATCH_BLOCK_HEADER) && (data == SAMSUNG_BATCH_BLOCK_HEADER))
			&& ((pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_BATCH_BLOCK_2_HEADER) && (data == SAMSUNG_BATCH_BLOCK_2_HEADER)))
			{
				pRx->count = 0;
			}
			break;
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
		case 1:
			if((pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_BATCH_BLOCK_HEADER) && (pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_BATCH_BLOCK_2_HEADER) && (pRx->buf[SAMSUNG_F_HEADER] != NIS_LIGHT_ID_COMM_1))
			{
				pRx->count = 0;
			}
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
		case 1:
			if((pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_LIGHT_HEADER) && (pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_ALL_DEVICE_ACK) 
			&& (pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_BATCH_BLOCK_HEADER) && (pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_BATCH_BLOCK_2_HEADER) && (pRx->buf[SAMSUNG_F_HEADER] != NIS_LIGHT_ID_COMM_1))
			{
				pRx->count = 0;
			}
			break;
		case 2:
			if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_ALL_DEVICE_ACK)	//HEADER : B0이고 COMMAND : 0x52이 아니면 
			{
				if(pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_REQUEST && pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_CONTROL)
				{
					pRx->count = 0;
				}
			}
			/*else if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER || pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_2_HEADER)	//HEADER : AD, A9일 때
			{
				if(pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_CONTROL)	//COMMAND : 0x53이 아니면
				{
					pRx->count = 0;
				}
			}*/
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined (_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case 1:
			if((pRx->buf[SAMSUNG_F_HEADER] != SAMSUNG_LIGHT_HEADER) && (pRx->buf[SAMSUNG_F_HEADER] != NIS_LIGHT_ID_COMM_1))
			{
				pRx->count = 0;
			}
			break;
		case 2:
			if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_ALL_DEVICE_ACK && pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_REQUEST)	//HEADER : B0이고 COMMAND : 0x52이 아니면 
			{
				pRx->count = 0;
			}
			else if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER || pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_2_HEADER)	//HEADER : AD, A9일 때
			{
				if(pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_CONTROL)	//COMMAND : 0x53이 아니면
				{
					pRx->count = 0;
				}
			}
			break;			
#endif
	}
	pRx->buf[pRx->count++] = data;

	if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_LIGHT_HEADER)
	{
		if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_POWER_ON || pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_STATUS_DATA)
		{
			// printf("LIGHT_POWER_ON\r\n");
			if(pRx->count >= 4)
			{
				// printf("power on max\r\n");
				crc = SAMSUNG_Crc(pRx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
				
				if(crc == pRx->buf[SAMSUNG_F_BATCH_BLOCK_CHECK_SUM])
				{
					if(G_Debug == DEBUG_HOST)
					{
						printf("\nRX(SAMSUNG) : ");
						for(i=0;i<pRx->count;i++)
						{
							printf("%02X ", (uint16_t)pRx->buf[i]);
						}
						printf("\n");
					}
					SAMSUNG_Data_Process(pRx);
				}
				else
				{
					printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[SAMSUNG_F_BATCH_BLOCK_CHECK_SUM]);
				}
				pRx->buf[0] = 0;
				pRx->count = 0;
			}
		}
		else
		{
			// printf("LIGHT\r\n");
			if(pRx->count >= 5)
			{
				// printf("light max\r\n");
				crc = SAMSUNG_Crc(pRx, SAMSUNG_F_LIGHT_CHECK_SUM);
				
				if(crc == pRx->buf[SAMSUNG_F_LIGHT_CHECK_SUM])
				{
					if(G_Debug == DEBUG_HOST)
					{
						printf("\nRX(SAMSUNG) : ");
						for(i=0;i<pRx->count;i++)
						{
							printf("%02X ", (uint16_t)pRx->buf[i]);
						}
						printf("\n");
					}
					SAMSUNG_Data_Process(pRx);
				}
				else
				{
					printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[SAMSUNG_F_LIGHT_CHECK_SUM]);
				}
				pRx->buf[0] = 0;
				pRx->count = 0;
			}
		}
	}
	else if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER || pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_2_HEADER || pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_ALL_DEVICE_ACK)
	{
		if(pRx->count >= SAMSUNG_BATCH_BLOCK_MAX_BUF)
		{
			crc = SAMSUNG_Crc(pRx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
			
			if(crc == pRx->buf[SAMSUNG_F_BATCH_BLOCK_CHECK_SUM])
			{
				if(G_Debug == DEBUG_HOST)
				{
					printf("\nRX(SAMSUNG) : ");
					for(i=0;i<pRx->count;i++)
					{
						printf("%02X ", (uint16_t)pRx->buf[i]);
					}
					printf("\n");
				}
				SAMSUNG_Data_Process(pRx);
			}
			else
			{
				printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[SAMSUNG_F_BATCH_BLOCK_CHECK_SUM]);
			}
			pRx->buf[0] = 0;
			pRx->count = 0;			
		}
	}
	else
	{
		if(pRx->buf[SAMSUNG_F_HEADER] == NIS_LIGHT_ID_COMM_1 && pRx->buf[SAMSUNG_F_COMMAND] == NIS_LIGHT_ID_COMM_2)
		{
			if(pRx->count >= 8)
			{
				// printf("rs485 nis max\r\n");
				crc = NIS_Crc(pRx, 0, NIS_RX);
				crc_xor = NIS_Crc(pRx, 1, NIS_RX);
				if(crc == pRx->buf[7] && crc_xor == pRx->buf[6])
				{
					if(G_Debug == DEBUG_HOST)
					{
						printf("\nRX(SAMSUNG) : ");
						for(i=0;i<pRx->count;i++)
						{
							printf("%02X ", (uint16_t)pRx->buf[i]);
						}
						printf("\n");
					}
					SAMSUNG_Data_Process(pRx);
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
	}
}

void SET_DimmingLevel(uint8_t item, uint8_t Dimming_Level)
{
	switch(item)
	{
		case mapping_ITEM_DIMMING_LIGHT_1:
			if(GET_Switch_State(item2tsn(item)))		//전등 켜져있을때만.
			{
				if(pG_State->Dimming_Level.Dimming1	!= (uint8_t)Dimming_Level)	Beep(ON);			// 디밍제어시 부저음 추가.
				pG_State->Dimming_Level.Dimming1		= (uint8_t)(Dimming_Level + 1);				// 삼성 SDS 디밍 레벨은 0이 디밍 1단계, 5가 디밍 6단계 이므로 + 1
				if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
				{
					pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
				}
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(GET_Switch_State(item2tsn(item)))
			{
				if(pG_State->Dimming_Level.Dimming2	!= (uint8_t)Dimming_Level)	Beep(ON);
				pG_State->Dimming_Level.Dimming2		= (uint8_t)(Dimming_Level + 1);
				if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
				{
					pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
				}
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
			break;
	}
}
void SET_DimmingColorLevel(uint8_t item, uint8_t Color_Level)
{
	switch(item)
	{
		case mapping_ITEM_DIMMING_LIGHT_1:
			pG_State->Color_Temp_Level.Color_Temp1	= Color_Level;
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp1 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// 최대 크기로 설정
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			pG_State->Color_Temp_Level.Color_Temp2	= Color_Level;
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp2 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// 최대 크기로 설정
			}		
			break;
	}
}
void SET_Elec_Auto_Manual(uint8_t item, uint8_t flag)
{
	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
			if(flag == SAMSUNG_AUTO_FLAG)	pG_State->ETC.Auto1	= 1;
			else							pG_State->ETC.Auto1	= 0;
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(flag == SAMSUNG_AUTO_FLAG)	pG_State->ETC.Auto2	= 1;
			else							pG_State->ETC.Auto2	= 0;
			break;
	}
}

void ElecLimit_Save(uint8_t item)
{
	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
			if(((double)Gu16_LCD_Watt_1 * 0.8) < 0.0)	Gu16_ElecLimitCurrent_1	= 0;
			else										Gu16_ElecLimitCurrent_1	= (uint16_t)((double)Gu16_LCD_Watt_1 * 0.8);	// 현재 값의 80%로 저장
			Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 1;
			Store_ElecLimitCurrent();		
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(((double)Gu16_LCD_Watt_2 * 0.8) < 0.0)	Gu16_ElecLimitCurrent_2	= 0;
			else										Gu16_ElecLimitCurrent_2	= (uint16_t)((double)Gu16_LCD_Watt_2 * 0.8);	// 현재 값의 80%로 저장
			Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 11;
			Store_ElecLimitCurrent();
			break;
	}
}

void SAMSUNG_Control(SAMSUNG_BUF *pRx, uint8_t item)
{
	uint8_t	Flag = OFF, touch_switch, tmr, control;
	control = pRx->buf[SAMSUNG_F_SUB_DATA_2];
	tmr	= Gu8_PowerSaving_Tmr;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	
	switch(item)	// 설정된 조명이 있으면
	{
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
			if(control == SAMSUNG_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)
				{
					EventCtrl(item2tsn(item), ON);
				}
			}
			else if(control == SAMSUNG_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))
				{
					EventCtrl(item2tsn(item), OFF);
				}
			}
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			if(control == SAMSUNG_ON_FLAG)		//ON
			{
				if(GET_Switch_State(item2tsn(item)) == 0)
				{
					// Flag = ON;
					touch_switch	= item2tsn(item);
					SET_Switch_State(touch_switch, ON);
					SET_LED_State(touch_switch, ON);
					Beep(ON);
					PUT_RelayCtrl(item2ctrl(item), ON);	// 항목기준 제어
					SET_SWITCH_Delay_OFF_Flag(item, 0);
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
					ALL_Electricity_Switch_LED_Ctrl();
				}
			}
			else								//OFF
			{
				// Flag = OFF;
				touch_switch	= item2tsn(item);
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, OFF);
				Beep(OFF);
				PUT_RelayCtrl(item2ctrl(item), OFF);	// 항목기준 제어
				SET_SWITCH_Delay_OFF_Flag(item, 0);
				SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
				ALL_Electricity_Switch_LED_Ctrl();
			}

#endif
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// 복구
			break;
	}
}

void SAMSUNG_Light_GROUP_Control(SAMSUNG_BUF *pRx)
{
	uint8_t i, switch_id, item, control;
	uint8_t Flag = 0, cnt = 1, k = 1, j = 0;
	
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	/*if(Get_485_ID() >= 16)
	{
		switch_id = pRx->buf[SAMSUNG_F_SUB_DATA_1] << 4 | pRx->buf[SAMSUNG_F_SUB_DATA_2];
		control = pRx->buf
	}
	else
	{
		switch_id = pRx->buf[SAMSUNG_F_SUB_DATA_1];
	}*/
	// switch_id = pRx->buf[SAMSUNG_F_SUB_DATA_1];
	control = pRx->buf[SAMSUNG_F_SUB_DATA_2];	//프로토콜 문서상 현장에 따른 옵션 기능이라고 언급 되어 있으나, 일단 추가함. SUB_DATA_2가 XX로 표시되어 있는데 정확히 언급이 없어 기존 제어에 맞춰서 1이면 ON, 0이면 OFF로 설정함. 
												//아니면 각 비트로 제어?? 그룹제어라 그렇게는 안될거같은데..
	for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
	{
		item = SAMSUNG_LIGHT_ITEM_Sequence[i];
		switch(item)
		{
			case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
			case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
			case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
			case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
			case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
			case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
			case mapping_ITEM_3WAY_1:
			case mapping_ITEM_3WAY_2:
			case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
			case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
				if(control == SAMSUNG_ON_FLAG)
				{
					if(GET_Switch_State(item2tsn(item)) == 0)
					{
						EventCtrl(item2tsn(item), ON);
					}
				}
				else
				{
					if(GET_Switch_State(item2tsn(item)))
					{
						EventCtrl(item2tsn(item), OFF);
					}
				}
				break;
		}
	}
}

uint8_t SAMSUNG_DIMMING_LIGHT_Data_Conversion(uint8_t item)
{
	uint8_t ret = 0;

	switch(item)
	{
		default:
			ret = SAMSUNG_OFF_FLAG;
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)(pG_State->Dimming_Level.Dimming1 - 1);		//삼성SDS 는 디밍 레벨이 1을 0으로 표시함. 그래서 디밍 레벨을 표현할 때 -1로 함
			else											ret = SAMSUNG_OFF_FLAG;			
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)(pG_State->Dimming_Level.Dimming2 - 1);		//삼성SDS 는 디밍 레벨이 1을 0으로 표시함. 그래서 디밍 레벨을 표현할 때 -1로 함
			else											ret = SAMSUNG_OFF_FLAG;		
			break;
	}
	return ret;
}

uint8_t SAMSUNG_LIGHT_Data_Conversion(uint8_t item, uint8_t control)
{
	uint8_t	ret = 0;
	
	switch(item)
	{
		default:
			ret = SAMSUNG_OFF_FLAG;
			break;
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = SAMSUNG_ON_FLAG;
			else											ret = SAMSUNG_OFF_FLAG;
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
			if(control == SAMSUNG_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = SAMSUNG_ON_FLAG;
				else											ret = SAMSUNG_OFF_FLAG;
			}
			else if(control == SAMSUNG_DIMMING)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)(pG_State->Dimming_Level.Dimming1 - 1);		//삼성SDS 는 디밍 레벨이 1을 0으로 표시함. 그래서 디밍 레벨을 표현할 때 -1로 함
				else											ret = SAMSUNG_OFF_FLAG;
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
			if(control == SAMSUNG_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = SAMSUNG_ON_FLAG;
				else											ret = SAMSUNG_OFF_FLAG;				
			}
			else if(control == SAMSUNG_DIMMING)
			{		
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)(pG_State->Dimming_Level.Dimming2 - 1);		//삼성SDS 는 디밍 레벨이 1을 0으로 표시함. 그래서 디밍 레벨을 표현할 때 -1로 함
				else											ret = SAMSUNG_OFF_FLAG;
			}
			break;
	}
	return	ret;
}

uint8_t SAMSUNG_ELEC_Data_Conversion(uint8_t item)
{
	uint8_t	ret;
	
	switch(item)
	{
		default:
			ret = 0x00;
			break;
			
		case mapping_ITEM_ELECTRICITY_1:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit4 자동/수동, bit0 전원 ON/OFF
			{
				ret	= (uint8_t)(SAMSUNG_ON_FLAG | (pG_State->ETC.Auto1 << 4));
			}
			else
			{
				ret	= (uint8_t)(SAMSUNG_OFF_FLAG | (pG_State->ETC.Auto1 << 4));
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit4 자동/수동, bit0 전원 ON/OFF
			{
				ret	= (uint8_t)(SAMSUNG_ON_FLAG | (pG_State->ETC.Auto2 << 4));
			}
			else
			{
				ret	= (uint8_t)(SAMSUNG_OFF_FLAG | (pG_State->ETC.Auto2 << 4));
			}
			break;
	}
	
	return	ret;
}

uint8_t SAMSUNG_Batch_Light_State(uint8_t item)		//일괄 소등 전 현재 전등 상태 저장.
{
	uint8_t ret = 0;
	switch(item)
	{
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			// printf("sta ");
			if(GET_Switch_State(item2tsn(item)))
			{
				// printf("ON ");
				ret = SAMSUNG_ON_FLAG;
			}
			else
			{
				// printf("OFF ");
				ret = SAMSUNG_OFF_FLAG;
			}
			// printf("%d\n\r", (uint16_t)ret);
			break;
	}
	return ret;
}

void SAMSUNG_BatchLight_Control(uint8_t item, uint8_t control_value)
{
	uint8_t tmr;
	tmr = Gu8_PowerSaving_Tmr;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
	switch(item)
	{
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(control_value == SAMSUNG_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))
				{
					EventCtrl(item2tsn(item), OFF);						//전등 OFF
				}
			}
			else if(control_value == SAMSUNG_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)
				{
					EventCtrl(item2tsn(item), ON);						//전등 ON
				}
			}
			break;
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// 복구
			break;
	}
}


//------------------------------------------------------------------------------------------------------------------------------------------
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)		// 1개용, 2개용	- 전등모델
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}
void SAMSUNG_LIGHT_POWER_ON_Res(SAMSUNG_BUF *pRx)
{
	SAMSUNG_BUF *pTx;
	pTx = &TxBuf;

	pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_SUB_DATA_1];
	pTx->buf[pTx->count++]	= SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
	TxBuf.send_flag	= 1;
}

void SAMSUNG_Status_Data_Res(void)
{
	SAMSUNG_BUF	*pTx;
	pTx = &TxBuf;
	pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;
	pTx->buf[pTx->count++] = SAMSUNG_STATUS_DATA;
	pTx->buf[pTx->count++] = 0;
	pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
	TxBuf.send_flag	= 1;
}

void SAMSUNG_LIGHT_Model_Res(SAMSUNG_BUF *pRx)				//개별전등 상태 요구에 대한 응답
{
	uint8_t	i, item, light_start_id, light_id = 0;
	uint8_t	on_n_off = 0x01, dimming = 0x02;
	SAMSUNG_BUF	*pTx;
	pTx = &TxBuf;

	pTx->count = SAMSUNG_F_HEADER;
	if(Get_485_ID() <= 1)	item = pRx->buf[SAMSUNG_F_SUB_DATA_1];
	else					item = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] - Get_485_ID() + 1);
	
	if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_LIGHT_STATE_REQUEST)
	{
		if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x00)													//No Action일때 전등상태 요구(해당 모듈의 전체 전등 상태를 보낸다)
		{
			light_start_id	= pRx->buf[SAMSUNG_F_SUB_DATA_2];										//해당 스위치의 첫 전등 id이자, 스위치 id.
			if(light_start_id != SAMSUNG_DIMMING_LIGHT_REQUEST)
			{
				if((light_start_id == Get_485_ID()) && ((uint8_t)(Get_485_ID() + Light_Cnt() - 1) < 17))	//sub data2가 485id 값과 동일할 때)
				{
					pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;									//buf 0
					pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];								//buf 1				
					pTx->buf[pTx->count++]	= (uint8_t)(((Light_Cnt() << 4) | light_start_id) & 0x7F);			//buf 2

					pTx->buf[pTx->count] = 0;		//for문의 데이터가 다음 동일 요청 왔을때 응답의 값이 지워지지 않아서 초기화. (전 응답시 모든 전등 ON으로 0x0F, 현재 상태가 바뀌어도 0x0F로 나오는 현상)
					for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
					{
						pTx->buf[pTx->count] |= (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[i], on_n_off) << (i - 1));	//전등 id 17이상 포함되는 스위치의 경우 buf4, 아니면 buf 3
					}
					pTx->buf[pTx->count++] &= 0x7F;		//7bit 는 0
					pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);					//전등 id 17이상 포함되는 스위치의 경우 buf5, 아니면 buf 4
					TxBuf.send_flag	= 1;
				}
			}
			else																					//NO Action에서 디밍상태 조회시
			{
				if(pG_Config->Enable_Flag.PWM_Dimming)												//디밍은 한 가구에 디밍스위치 하나만 가능.
				{
					pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;
					pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];
					pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_2] & 0x7F);							//0x1E
					for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
					{
						pTx->buf[pTx->count++]	= SAMSUNG_DIMMING_LIGHT_Data_Conversion(SAMSUNG_DIMMING_LIGHT_ITEM_Sequence[i]);
					}
					pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_DIM_CHECK_SUM);
					TxBuf.send_flag = 1;
				}
			}
		}
		else																					//전등 상태 요구(해당하는 전등ID의 전등의 상태만 보낸다)
		{
			light_id		= pRx->buf[SAMSUNG_F_SUB_DATA_1];
			if((light_id >= Get_485_ID()) && (light_id < (uint8_t)(Get_485_ID() + Light_Cnt())) && ((uint8_t)(Get_485_ID() + Light_Cnt() - 1) < 17))	//light_id가 해당 스위치의 아이디와 같거나 크고, id + light_cnt 값 보다 작을때, 17회로 이하일때 ex)switch id = 1, light_cnt = 4라면 light_id 1 ~ 4까지 
			{
				pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;
				pTx->buf[pTx->count++] = pRx->buf[SAMSUNG_F_COMMAND];
				pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] & 0x7F);
				pTx->buf[pTx->count++] = (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[item], dimming) & 0x7F);
				pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);
				TxBuf.send_flag	= 1;
			}
		}
	}
	else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_LIGHT_STATE_17circuit_REQUEST)
	{
		light_start_id	= pRx->buf[SAMSUNG_F_SUB_DATA_2];
		if(light_start_id == Get_485_ID())
		{
			if((Get_485_ID() + Light_Cnt() - 1) >= 17)										//17회로 이상, 해당 모듈에 전등 ID 17번 이상이 포함될때 ex) ID : 15, 전등 4구(15,16, 17, 18). 15 + 4 = 19. 19 - 1 = 18
			{
				pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;							//buf 0
				pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];						//buf 1						
				pTx->buf[pTx->count++]	= (uint8_t)(Light_Cnt() & 0x7F);					//buf 2, 전등 수
				pTx->buf[pTx->count++]	= (uint8_t)(light_start_id & 0x7F);					//buf 3, 시작 전등 ID
				pTx->buf[pTx->count] = 0;
				for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
				{
					pTx->buf[pTx->count] |= (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[i], on_n_off) << (i - 1));	//전등 id 17이상 포함되는 스위치의 경우 buf4, 아니면 buf 3
				}
				pTx->buf[pTx->count++] &= 0x7F;
				pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);					//전등 id 17이상 포함되는 스위치의 경우 buf5, 아니면 buf 4
				TxBuf.send_flag	= 1;
			}
		}
	}
	else																				//조명 제어, 디밍 제어
	{
		light_id		= pRx->buf[SAMSUNG_F_SUB_DATA_1];
		if((light_id >= Get_485_ID()) && (light_id < (uint8_t)(Get_485_ID() + Light_Cnt())))	//light_id가 해당 스위치의 아이디와 같거나 크고, id + light_cnt 값 보다 작을때 ex)switch id = 1, light_cnt = 4라면 light_id 1 ~ 4까지 
		{			
			pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;								//ACK
			pTx->buf[pTx->count++] = pRx->buf[SAMSUNG_F_COMMAND];							//CMD
			pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] & 0x7F);						//전등ID
			if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_LIGHT_CONTROL)
			{	
				pTx->buf[pTx->count++] = (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[item], on_n_off) & 0x7F);
			}
			else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_LIGHT_GROUP_CONTROL)
			{
				pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_2] & 0x7F);
			}
			else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_DIMMING_LIGHT_CONTROL)
			{
				pTx->buf[pTx->count++] = (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[item], dimming) & 0x7F);
			}
			pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);
			TxBuf.send_flag	= 1;
		}
	}
}

void SAMSUNG_All_Light_Res(SAMSUNG_BUF *pRx)
{
	SAMSUNG_BUF	*pTx;
	pTx = &TxBuf;

	pTx->count = SAMSUNG_F_HEADER;
	pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_SUB_DATA_1];
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_SUB_DATA_2];
	pTx->buf[pTx->count++]	= SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);
	TxBuf.send_flag	= 1;
}

void SAMSUNG_Data_Process(SAMSUNG_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		light_no, item = 0;
	uint8_t		switch_id;

	switch_id = pRx->buf[SAMSUNG_F_SUB_DATA_1];
	Gu8_RS_485_Enable_Tmr	= 1;
	TIM2_Cmd(ENABLE);
	// Gu8_RS_485_Tx_Tmr		= 0;
	
	item = SAMSUNG_LIGHT_ITEM_Sequence[(uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] - (Get_485_ID() - 1))];	//ex)전등(9 ~ 12)인 모듈. 4번째 전등(ID : 12)을 제어 하려면 12 - (9 - 1) = 4
	
	if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_LIGHT_HEADER)		//0xAC
	{
		switch(pRx->buf[SAMSUNG_F_COMMAND])
		{
			case SAMSUNG_POWER_ON:								//0x5A
				if(Get_485_ID() == 0x01)	SAMSUNG_LIGHT_POWER_ON_Res(pRx);		//대표 전등 하나가 응답.
				break;
			case SAMSUNG_STATUS_DATA:							//0x41
				if(Get_485_ID() == 0x01)	SAMSUNG_Status_Data_Res();			//대표 전등 하나가 응답.
				break;
			case SAMSUNG_LIGHT_STATE_REQUEST:					//0x79
			case SAMSUNG_LIGHT_STATE_17circuit_REQUEST:			//0x75
				SAMSUNG_LIGHT_Model_Res(pRx);
				break;
			case SAMSUNG_LIGHT_CONTROL:					//0x7A
				if(switch_id == 0)						//ID가 0번이면 전체 모듈의 전체 전등 제어.
				{
					/*if(Gu8_BatchLight_OFF_Flag == 0)
					{
						if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_OFF_FLAG)		//전체 전등 OFF 일때(일괄 소등)만 상태 저장.
						{
							for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
							{
								Store_Light_State[i] = SAMSUNG_Batch_Light_State(SAMSUNG_LIGHT_ITEM_Sequence[i]);	//소등 전 현재 전등 상태 저장.
							}
							Gu8_BatchLight_OFF_Flag = 1;
						}
						SAMSUNG_Light_GROUP_Control(pRx);	//전체 모듈 제어.
					}
					if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_ON_FLAG)		//전체 켜기(일괄X)
					{
						SAMSUNG_Light_GROUP_Control(pRx);
					}*/

					/*if(Gu8_BatchLight_OFF_Flag == 0)	//일괄 소등 상태가 아니고, 
					{
						if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_OFF_FLAG)		//전체 전등 OFF 일때(일괄 소등)만 상태 저장.
						{
							for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
							{
								Store_Light_State[i] = SAMSUNG_Batch_Light_State(SAMSUNG_LIGHT_ITEM_Sequence[i]);	//소등 전 현재 전등 상태 저장.
							}
							// Gu8_BatchLight_OFF_Flag = 1;
						}
						SAMSUNG_Light_GROUP_Control(pRx);	//전체 모듈 제어.							
					}
					else	//전체 켜기, 끄기 일 때(플래그 없음), 데이터 저장 없음.
					{
						SAMSUNG_Light_GROUP_Control(pRx);	//SUB_DATA_2에 따라서 전체 ON / OFF
					}*/

					if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_OFF_FLAG)
					{
						if(Gu8_BatchLight_OFF_Flag == 0)
						{
							for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
							{
								Store_Light_State[i] = SAMSUNG_Batch_Light_State(SAMSUNG_LIGHT_ITEM_Sequence[i]);	//소등 전 현재 전등 상태 저장.
							}
							Gu8_BatchLight_OFF_Flag = 1;
							SAMSUNG_Light_GROUP_Control(pRx);							
						}
					}
					else if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_ON_FLAG)
					{
						Gu8_BatchLight_OFF_Flag = 0;	//만약 일괄소등 상태에서 전체 전등 ON 하면 일괄 점등 했을때 복귀가 안되는데..
						SAMSUNG_Light_GROUP_Control(pRx);
					}

					if(Get_485_ID() == 0x01)			//첫번째 모듈만 대표로 응답.
					{
						SAMSUNG_All_Light_Res(pRx);
					}
				}
				else
				{
					Gu8_BatchLight_OFF_Flag = 0;	//만약 일괄소등 상태에서 개별 전등 ON 하면 일괄 점등 했을때 복귀가 안되는데..
					SAMSUNG_Control(pRx, item);
					SAMSUNG_LIGHT_Model_Res(pRx);
				}
				break;
			case SAMSUNG_LIGHT_GROUP_CONTROL:
				if(switch_id == Get_485_ID())
				{
					SAMSUNG_Light_GROUP_Control(pRx);
					SAMSUNG_LIGHT_Model_Res(pRx);
				}
				break;
		}
	}
	else if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_ALL_DEVICE_ACK)
	{
		// printf("0xB0 ");
		// if((pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER && pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_CONTROL)
		// || (pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_2_HEADER && pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_CONTROL))
		if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_CONTROL)
		{
			// printf("0x53 ");
			/*if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == SAMSUNG_OFF_FLAG)
			{	//B0 53 00 XX	일괄소등 제어에 대한 일괄 스위치의 응답.
				Gu8_BatchLight_OFF_Flag_2 = 1;	//일괄 소등 상태 플래그
			}
			else if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == SAMSUNG_ON_FLAG)
			{	//B0 53 01 XX	일괄점등 제어에 대한 일괄 스위치의 응답.
				if(Gu8_BatchLight_OFF_Flag && Gu8_BatchLight_OFF_Flag_2)
				{
					// printf("flag 1\r\n");
					for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
					{
						if(Store_Light_State[i] == SAMSUNG_ON_FLAG)		//저장된 전등 상태가 ON인 전등만
						{
							SAMSUNG_BatchLight_Control(SAMSUNG_LIGHT_ITEM_Sequence[i], SAMSUNG_ON_FLAG);	//전등 ON
						}
					}
					Gu8_BatchLight_OFF_Flag = 0;
					Gu8_BatchLight_OFF_Flag_2 = 0;
				}				
			}*/
		}
		else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_REQUEST)
		{
			// printf("0x52 ");
			if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == SAMSUNG_OFF_FLAG)
			{
				Gu8_BatchLight_OFF_Flag_2 = 1;
			}
			else if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == SAMSUNG_ON_FLAG)
			{
				// printf("data1 1 ");
				if(Gu8_BatchLight_OFF_Flag && Gu8_BatchLight_OFF_Flag_2)
				{
					// printf("flag 1\r\n");
					for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
					{
						// SAMSUNG_BatchLight_Control(SAMSUNG_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);	//전등 ON
						if(Store_Light_State[i] == SAMSUNG_ON_FLAG)		//저장된 전등 상태가 ON인 전등만
						{
							SAMSUNG_BatchLight_Control(SAMSUNG_LIGHT_ITEM_Sequence[i], SAMSUNG_ON_FLAG);	//전등 ON
						}
					}
					// printf("\r\n");				
					Gu8_BatchLight_OFF_Flag = 0;
					Gu8_BatchLight_OFF_Flag_2 = 0;
				}				
			}
		}
	}
	else if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
		/*
		if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
			printf("Switch ID Change\r\n");
		}
		Store_CurrentConfig();
		*/
		RS_485_ID_RES();
	}
}
#endif	// defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)	// 2개용	- 전등+전열모델
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}
void SAMSUNG_LIGHT_POWER_ON_Res(SAMSUNG_BUF *pRx)
{
	SAMSUNG_BUF *pTx;
	pTx = &TxBuf;

	pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_SUB_DATA_1];
	pTx->buf[pTx->count++]	= SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
	TxBuf.send_flag	= 1;
}

void SAMSUNG_LIGHT_Model_Res(SAMSUNG_BUF *pRx)				//개별전등 상태 요구에 대한 응답
{
	uint8_t	i, item, light_start_id, light_id = 0;
	uint8_t	on_n_off = 0x01, dimming = 0x02;
	SAMSUNG_BUF	*pTx;
	pTx = &TxBuf;

	pTx->count = SAMSUNG_F_HEADER;
	if(Get_485_ID() <= 1)	item = pRx->buf[SAMSUNG_F_SUB_DATA_1];
	else					item = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] - Get_485_ID() + 1);
	
	if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_LIGHT_STATE_REQUEST)
	{
		if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x00)													//No Action일때 전등상태 요구(해당 모듈의 전체 전등 상태를 보낸다)
		{
			light_start_id	= pRx->buf[SAMSUNG_F_SUB_DATA_2];										//해당 스위치의 첫 전등 id이자, 스위치 id.
			if(light_start_id != SAMSUNG_DIMMING_LIGHT_REQUEST)
			{
				if((light_start_id == Get_485_ID()) && ((uint8_t)(Get_485_ID() + Light_Cnt() - 1) < 17))	//sub data2가 485id 값과 동일할 때)
				{
					pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;									//buf 0
					pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];								//buf 1				
					pTx->buf[pTx->count++]	= (uint8_t)(((Light_Cnt() << 4) | light_start_id) & 0x7F);			//buf 2

					pTx->buf[pTx->count] = 0;		//for문의 데이터가 다음 동일 요청 왔을때 응답의 값이 지워지지 않아서 초기화. (전 응답시 모든 전등 ON으로 0x0F, 현재 상태가 바뀌어도 0x0F로 나오는 현상)
					for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
					{
						pTx->buf[pTx->count] |= (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[i], on_n_off) << (i - 1));	//전등 id 17이상 포함되는 스위치의 경우 buf4, 아니면 buf 3
					}
					pTx->buf[pTx->count++] &= 0x7F;		//7bit 는 0
					pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);					//전등 id 17이상 포함되는 스위치의 경우 buf5, 아니면 buf 4
					TxBuf.send_flag	= 1;
				}
			}
			else																					//NO Action에서 디밍상태 조회시
			{
				if(pG_Config->Enable_Flag.PWM_Dimming)												//디밍은 한 가구에 디밍스위치 하나만 가능.
				{
					pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;
					pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];
					pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_2] & 0x7F);							//0x1E
					for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
					{
						pTx->buf[pTx->count++]	= SAMSUNG_DIMMING_LIGHT_Data_Conversion(SAMSUNG_DIMMING_LIGHT_ITEM_Sequence[i]);
					}
					pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_DIM_CHECK_SUM);
					TxBuf.send_flag = 1;
				}
			}
		}
		else																					//전등 상태 요구(해당하는 전등ID의 전등의 상태만 보낸다)
		{
			light_id		= pRx->buf[SAMSUNG_F_SUB_DATA_1];
			if((light_id >= Get_485_ID()) && (light_id < (uint8_t)(Get_485_ID() + Light_Cnt())) && ((uint8_t)(Get_485_ID() + Light_Cnt() - 1) < 17))	//light_id가 해당 스위치의 아이디와 같거나 크고, id + light_cnt 값 보다 작을때, 17회로 이하일때 ex)switch id = 1, light_cnt = 4라면 light_id 1 ~ 4까지 
			{
				pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;
				pTx->buf[pTx->count++] = pRx->buf[SAMSUNG_F_COMMAND];
				pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] & 0x7F);
				pTx->buf[pTx->count++] = (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[item], dimming) & 0x7F);
				pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);
				TxBuf.send_flag	= 1;
			}
		}
	}
	else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_LIGHT_STATE_17circuit_REQUEST)
	{
		light_start_id	= pRx->buf[SAMSUNG_F_SUB_DATA_2];
		if(light_start_id == Get_485_ID())
		{
			if((Get_485_ID() + Light_Cnt() - 1) >= 17)										//17회로 이상, 해당 모듈에 전등 ID 17번 이상이 포함될때 ex) ID : 15, 전등 4구(15,16, 17, 18). 15 + 4 = 19. 19 - 1 = 18
			{
				pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;							//buf 0
				pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];						//buf 1						
				pTx->buf[pTx->count++]	= (uint8_t)(Light_Cnt() & 0x7F);										//buf 2, 전등 수
				pTx->buf[pTx->count++]	= (uint8_t)(light_start_id & 0x7F);									//buf 3, 시작 전등 ID
				pTx->buf[pTx->count] = 0;
				for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
				{
					pTx->buf[pTx->count] |= (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[i], on_n_off) << (i - 1));	//전등 id 17이상 포함되는 스위치의 경우 buf4, 아니면 buf 3
				}
				pTx->buf[pTx->count++] &= 0x7F;
				pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);					//전등 id 17이상 포함되는 스위치의 경우 buf5, 아니면 buf 4
				TxBuf.send_flag	= 1;
			}
		}
	}
	else																				//조명 제어, 디밍 제어
	{
		light_id		= pRx->buf[SAMSUNG_F_SUB_DATA_1];
		if((light_id >= Get_485_ID()) && (light_id < (uint8_t)(Get_485_ID() + Light_Cnt())))	//light_id가 해당 스위치의 아이디와 같거나 크고, id + light_cnt 값 보다 작을때 ex)switch id = 1, light_cnt = 4라면 light_id 1 ~ 4까지 
		{			
			pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;								//ACK
			pTx->buf[pTx->count++] = pRx->buf[SAMSUNG_F_COMMAND];							//CMD
			pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] & 0x7F);						//전등ID
			if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_LIGHT_CONTROL)
			{	
				pTx->buf[pTx->count++] = (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[item], on_n_off) & 0x7F);
			}
			else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_LIGHT_GROUP_CONTROL)
			{
				pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_2] & 0x7F);
			}
			else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_DIMMING_LIGHT_CONTROL)
			{
				pTx->buf[pTx->count++] = (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[item], dimming) & 0x7F);
			}
			pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);
			TxBuf.send_flag	= 1;
		}
	}
}

void SAMSUNG_Data_Process(SAMSUNG_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		light_no, item = 0;

	Gu8_RS_485_Enable_Tmr	= 1;
	TIM2_Cmd(ENABLE);
	// Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
	switch_id = pRx->buf[SAMSUNG_F_SUB_DATA_1];

	item = SAMSUNG_LIGHT_ITEM_Sequence[(uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] - (Get_485_ID() - 1))];	//ex)전등(9 ~ 12)인 모듈. 4번째 전등(ID : 12)을 제어 하려면 12 - (9 - 1) = 4
	
	if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_LIGHT_HEADER)		//0xAC
	{
		switch(pRx->buf[SAMSUNG_F_COMMAND])
		{
			case SAMSUNG_POWER_ON:								//0x5A
				if(Get_485_ID() == 0x01)	SAMSUNG_LIGHT_POWER_ON_Res(pRx);		//대표 전등 하나가 응답.
				break;
			case SAMSUNG_STATUS_DATA:							//0x41
				if(Get_485_ID() == 0x01)	SAMSUNG_Status_Data_Res(pRx);			//대표 전등 하나가 응답.
				break;				
			case SAMSUNG_LIGHT_STATE_REQUEST:					//0x79
			case SAMSUNG_LIGHT_STATE_17circuit_REQUEST:			//0x75
			case SAMSUNG_DIMMING_LIGHT_REQUEST:
				SAMSUNG_LIGHT_Model_Res(pRx);
				break;
			case SAMSUNG_LIGHT_CONTROL:					//0x7A
				if(switch_id == 0)						//ID가 0번이면 전체 모듈의 전체 전등 제어.
				{
					if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_OFF_FLAG)		//전체 전등 OFF 일때(일괄 소등)만 상태 저장.
					{
						for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
						{
							Store_Light_State[i] = SAMSUNG_Batch_Light_State(SAMSUNG_LIGHT_ITEM_Sequence[i]);	//소등 전 현재 전등 상태 저장.
						}
					}
					SAMSUNG_Light_GROUP_Control(pRx);	//전체 모듈 제어.
					if(Get_485_ID() == 0x01)			//첫번째 모듈만 대표로 응답.
					{
						SAMSUNG_All_Light_Res(pRx);
					}
				}
				else										//ID가 0번이 아닐때는 해당 전등만 제어.
				{
					SAMSUNG_Control(pRx, item);
					SAMSUNG_LIGHT_Model_Res(pRx);
				}
				break;
			case SAMSUNG_LIGHT_GROUP_CONTROL:
				SAMSUNG_Light_GROUP_Control(pRx);
				SAMSUNG_LIGHT_Model_Res(pRx);
				break;
			case SAMSUNG_DIMMING_LIGHT_CONTROL:					//0x7B 현재는 네트워크 스위치에 디밍 기능 없음
				SET_DimmingLevel(item, pRx->buf[SAMSUNG_F_SUB_DATA_2]);
				SAMSUNG_LIGHT_Model_Res(pRx);
				break;
		}
	}
	else else if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.

		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
			printf("Switch ID Change\r\n");
		}
		Store_CurrentConfig();*/
		RS_485_ID_RES();
	}
}
#endif	// defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1개용 - 일괄차단스위치

uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}
void SAMSUNG_LIGHT_POWER_ON_Res(SAMSUNG_BUF *pRx)
{
	SAMSUNG_BUF *pTx;
	pTx = &TxBuf;

	pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_SUB_DATA_1];
	pTx->buf[pTx->count++]	= SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
	TxBuf.send_flag	= 1;
}

void SAMSUNG_BATCH_BLOCK_Data_Res(SAMSUNG_BUF *pRx)
{
	uint8_t	item, touch_switch;
	SAMSUNG_BUF	*pTx;
	pTx = &TxBuf;
	
	pTx->count = SAMSUNG_F_HEADER;
	
	// if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER)		//일괄 조명
	// {
	pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;
	pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];
	if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_REQUEST || pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_CONTROL)
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	pTx->buf[pTx->count++]	= (uint8_t)(SAMSUNG_ON_FLAG & 0x7F);
		else															pTx->buf[pTx->count++]	= (uint8_t)(SAMSUNG_OFF_FLAG & 0x7F);
	}
	/*else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_GAS_REQUEST || pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_GAS_CONTROL)
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))				pTx->buf[pTx->count++]	= (uint8_t)(SAMSUNG_ON_FLAG & 0x7F);
		else															pTx->buf[pTx->count++]	= (uint8_t)(SAMSUNG_OFF_FLAG & 0x7F);
	}*/		//240417 0x50, 0x51은 일괄스위치에 가스차단기가 직접 연결이 되어 있는 경우 사용하여 응답하지 않아도 된다고 함.
	// }
	pTx->buf[pTx->count++]	= SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);							//Byte07 Check Sum
	TxBuf.send_flag	= 1;
}

void SAMSUNG_Status_Data_Res(void)
{
	SAMSUNG_BUF	*pTx;
	pTx = &TxBuf;
	pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;
	pTx->buf[pTx->count++] = SAMSUNG_STATUS_DATA;
	pTx->buf[pTx->count++] = 0;
	pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
	TxBuf.send_flag	= 1;
}

void SAMSUNG_Data_Process(SAMSUNG_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item, touch_switch = 0;
    uint8_t static old_state = 0, state = 0, mode = 0, mode2 = 0, step = 0;

	SAMSUNG_BUF	*pTx;
	pTx = &TxBuf;

	Gu8_RS_485_Enable_Tmr	= 1;
	TIM2_Cmd(ENABLE);
	// Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
	
	if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER || pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_2_HEADER)
	{
		if(Get_485_ID() == 1 && pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_2_HEADER)		return;	//wsm 240412
		else if(Get_485_ID() == 2 && pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER)	return;	//wsm 240412

		switch(pRx->buf[SAMSUNG_F_COMMAND])
		{
			case SAMSUNG_POWER_ON:
				SAMSUNG_LIGHT_POWER_ON_Res(pRx);
				break;
			case SAMSUNG_BATCH_BLOCK_REQUEST:		//0x52 일괄 상태 요구(월패드 -> 일괄)
			case SAMSUNG_BATCH_BLOCK_CONTROL:		//0x53 일괄 점등, 소등 요구(월패드 -> 일괄)
				if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_CONTROL)
				{
					if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == SAMSUNG_ON_FLAG)			BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);		//일괄점등
					else if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == SAMSUNG_OFF_FLAG)		BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);		//일괄소등
				}
				SAMSUNG_BATCH_BLOCK_Data_Res(pRx);
				break;
			/*case SAMSUNG_BATCH_BLOCK_GAS_REQUEST:	//0x50 가스 밸브 상태 요구(월패드 -> 일괄)
			case SAMSUNG_BATCH_BLOCK_GAS_CONTROL:	//0x51 가스 차단 요구(월패드 -> 일괄)
				if(item2tsn(mapping_ITEM_GAS))
				{
					if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_GAS_CONTROL)
					{
						if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x01)						BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//가스 차단
					}
					SAMSUNG_BATCH_BLOCK_Data_Res(pRx);
				}
				break;*/	//240417 0x50, 0x51은 일괄스위치에 가스차단기가 직접 연결이 되어 있는 경우 사용하여 응답하지 않아도 된다고 함.
			case SAMSUNG_BATCH_BLOCK_SWITCH_CONTROL:				//스위치에서 제어 후 월패드에서 일괄소등(점등)에 대한 ACK
			case SAMSUNG_BATCH_BLOCK_SWITCH_GAS_CONTROL:			//스위치에서 가스 차단 요청 후 월패드에서 가스 차단 요청에 대한 ACK
			case SAMSUNG_BATCH_BLOCK_SWITCH_GAS_REQUEST:			//스위치에서 가스 상태요구 후 월패드에서 가스 상태요구에 대한 ACK
				if(item2tsn(mapping_ITEM_GAS))
				{
					if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_SWITCH_GAS_CONTROL)	//0x55
					{
						// if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)		//가스 차단 요청중일때만,
						// {
							// if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x01)	BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//응답오면 스위치에서 가스 차단 상태로 변경.
							Gu8_Gas_Request_Flag = 0;	//해닫힘 요구에 대한 Ack 받으면 요청 플래그 초기화.
						// }
					}
					else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_SWITCH_GAS_REQUEST)	//0x56
					{
						if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x00)
						{
							if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) != LED_FLASHING)	//가스 차단 요청중이 아닌 경우만 차단 해제함.
							{
								BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);	//가스 차단 해제
							}
						}
						else if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x01)		BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);	//가스 차단
					}
					SAMSUNG_Status_Data_Res();
				}
				break;
			case SAMSUNG_BATCH_BLOCK_SWITCH_ELEVATOR_CONTROL:		//스위치에서 엘리베이터 콜 요청 후 월패드에서 엘리베이터 콜에 대한 ACK
				if(item2tsn(mapping_ITEM_ELEVATOR))
				{
					if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_SWITCH_ELEVATOR_CONTROL)
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	//240409 엘리베이터 요청중일 때만
						{
							BATCH_BLOCK_Control(SET__ELEVATOR_CALL);
						}
					}
					SAMSUNG_Status_Data_Res();
				}
				// SAMSUNG_Status_Data_Res();	//해당 기능 사용할 경우만 응답하도록 수정
				break;
			case SAMSUNG_STATUS_DATA:
				batch_state = GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF));
				if(old_batch_state != batch_state)													state |= 0x01;
				
				if(item2tsn(mapping_ITEM_GAS))
				{
					// gas_state = GET_LED_State(item2tsn(mapping_ITEM_GAS));							//요청 : FLASHING, 차단 : LED ON, 열림 : LED_OFF
					// if((old_gas_state != gas_state) && gas_state == LED_FLASHING)					state |= 0x02;				//11 OFF, 12 FLASING,
					if((state & 0x02) == 0)
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING && Gu8_Gas_Request_Flag)	state |= 0x02;	//가스 차단 요청중(LED FLASHING)과 가스 차단 요청 플래그 값이 있으면 
					}
				}
				if(item2tsn(mapping_ITEM_ELEVATOR))
				{
					// elevator_state = GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR));				//요청 : FLASHING, 차단 : LED ON, 열림 : LED_OFF
					// if((old_elevator_state != elevator_state) && elevator_state == LED_FLASHING)	state |= 0x04;
					if((state & 0x04) == 0)
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	state |= 0x04;
					}
				}
				// printf("old = %d, current = %d\r\n", (uint16_t)old_gas_state, (uint16_t)gas_state);
				if(state != 0)		//일괄 스위치의 상태 변화가있을때
				{
					// printf("state = %x ", (uint16_t)state);
					// if(state == 0x01 || state == 0x03 || state == 0x05 || state == 0x07)		//0x01 : 일괄, 0x03 : 일괄, 가스, 0x05 : 일괄, 엘베, 0x07 : 일괄, 가스, 엘베
					if((state & 0x01) == 0x01)
					{
						pTx->count = SAMSUNG_F_HEADER;
						pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;
						pTx->buf[pTx->count++] = SAMSUNG_BATCH_BLOCK_SWITCH_CONTROL;
						if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	pTx->buf[pTx->count++]	= (uint8_t)(SAMSUNG_ON_FLAG & 0x7F);		//점등 1
						else															pTx->buf[pTx->count++]	= (uint8_t)(SAMSUNG_OFF_FLAG & 0x7F);		//소등 0
						pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
						TxBuf.send_flag	= 1;								
						state = (uint8_t)(state ^ 0x01);
						old_batch_state = batch_state;						
					}
					// else if(state == 0x02 || state == 0x06)										//0x02 : 가스, 0x06 : 가스, 엘베

					else if((state & 0x02) == 0x02 || (state & 0x04) == 0x04)
					{
						pTx->count = SAMSUNG_F_HEADER;
						pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;
						if((state & 0x06) == 0x06)
						{
							switch(step)
							{
								case 0:
									pTx->buf[pTx->count++] = SAMSUNG_BATCH_BLOCK_SWITCH_GAS_CONTROL;
									pTx->buf[pTx->count++] = SAMSUNG_GAS_BLOCK;
									state = (uint8_t)(state ^ 0x02);
									step++;
									break;
								case 1:
									pTx->buf[pTx->count++] = SAMSUNG_BATCH_BLOCK_SWITCH_ELEVATOR_CONTROL;
									pTx->buf[pTx->count++] = SAMSUNG_ELEVATOR_CALL;
									state = (uint8_t)(state ^ 0x04);
									step = 0;
									break;
							}
						}
						else if((state & 0x02) == 0x02)
						{
							pTx->buf[pTx->count++] = SAMSUNG_BATCH_BLOCK_SWITCH_GAS_CONTROL;
							pTx->buf[pTx->count++] = SAMSUNG_GAS_BLOCK;
							state = (uint8_t)(state ^ 0x02);
						}
						else if((state & 0x04) == 0x04)
						{
							pTx->buf[pTx->count++] = SAMSUNG_BATCH_BLOCK_SWITCH_ELEVATOR_CONTROL;
							pTx->buf[pTx->count++] = SAMSUNG_ELEVATOR_CALL;
							state = (uint8_t)(state ^ 0x04);
						}
						pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
						TxBuf.send_flag	= 1;
					}
				}
				else
				{
					// printf("state equal ");
					switch(mode2)
					{
						case 0:
							// printf("status res\r\n");
							SAMSUNG_Status_Data_Res();
							mode2++;
							break;
						case 1:
							// printf("gas req\r\n");
							pTx->count = SAMSUNG_F_HEADER;
							pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;
							pTx->buf[pTx->count++] = SAMSUNG_BATCH_BLOCK_SWITCH_GAS_REQUEST;
							pTx->buf[pTx->count++] = 0;
							pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
							TxBuf.send_flag	= 1;
							mode2 = 0;
							break;
						default:
							mode2 = 0;
							break;
					}
					old_batch_state = batch_state;
					// old_gas_state = gas_state;
					// old_elevator_state = elevator_state;
				}
				break;
		}
	}
	else if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
			printf("Switch ID Change\r\n");
		}
		Store_CurrentConfig();	*/
		RS_485_ID_RES();
	}
	
}
#endif	// _ONE_SIZE_BATCH_BLOCK_MODEL_
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
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(Gu16_Elevator_Tmr == 0)
		{
			if((GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING) || (GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR))))			//호출 요청중일때 60초 지나면 LED 원래대로
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);
			}
		}
	}
}

void BATCH_BLOCK_Control(uint8_t control)
{
	uint8_t	touch_switch = 0;
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec

	switch(control)
	{
		case SET__BATCHLIGHT_OFF:
			if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))
			{
				EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), OFF);
			}
			break;
		case SET__BATCHLIGHT_ON:
			if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == 0)
			{
				EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), ON);
			}
			break;

		case SET__GAS_CLOSE_REQUEST:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch))		//가스 차단중일때
			{
				Beep(ON);
				// printf("Gas State Close. Re-REQUEST\r\n");			//차단중이라도 재요청.
			}
			else													//가스 차단중이 아닐때
			{
				Gu16_GAS_Off_Tmr	= 30;						// 5분(300->270), 60초(임시) 경과 후 LED 소등(단 월패드에서 close/open 데이터가 수신되면 해당 상태로 전환), 
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, LED_FLASHING);	// LED FLASHING
				Beep(OFF);
				Gu8_Gas_Request_Flag = 1;
				// printf("Gas REQUEST\r\n");
			}

			break;
		case SET__GAS_CLOSE_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) == 0)		//가스 열림 상태일때만 차단 동작.
			{	
				SET_Switch_State(touch_switch, ON);		// 가스밸브 차단(닫힘)
				SET_LED_State(touch_switch, OFF);		// 실제로는 LED 켜짐
				Beep(ON);
			}
			// printf("GAS CLOSE\r\n");
			break;
		case SET__GAS_OPEN_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) || GET_LED_State(touch_switch) == LED_FLASHING)			//가스 닫힘 상태일때만 열림 동작.
			{
				SET_Switch_State(touch_switch, OFF);	// 가스밸브 열림
				SET_LED_State(touch_switch, ON);		// 실제로는 LED 꺼짐
				Beep(ON);
			}
			// printf("GAS OPEN\r\n");
			break;
		
		case SET__ELEVATOR_REQUEST:																			//스위치에서 엘리베이터 콜
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))		//스위치 OFF 거나 LED 상태 Flashing이면
			{
				Gu16_Elevator_Tmr = 60;																		//요청 타이머 60초 초기화. 0이되면 요청 취소되고 LED OFF됨.
				SET_Switch_State(touch_switch, OFF);															//스위치 OFF
				SET_LED_State(touch_switch, LED_FLASHING);													//LED Flashing
				Beep(ON);
				// printf("ELEVATOR REQUEST\r\n");
			}
			break;
		case SET__ELEVATOR_CALL:																			//세대기에서 엘리베이터 콜
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_Switch_State(touch_switch) == OFF)
			{
				Gu16_Elevator_Tmr = 10;																			//콜 상태가 되면 타이머 60초 초기화. 타이머가 0되면 상태 원래대로 돌아감.
				//240411 해당 프로토콜은 도착 데이터 없으므로. 엘리베이터 콜 성공 후 10초 뒤 노멀 모드로 전환
				SET_Switch_State(touch_switch, ON);																//스위치 ON
				SET_LED_State(touch_switch, OFF);																//LED ON
				Beep(ON);
			}
			// printf("ELEVATOR CALL\r\n");
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


#endif	// _SAMSUNG_PROTOCOL_
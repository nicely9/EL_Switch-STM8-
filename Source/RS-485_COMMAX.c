/************************************************************************************
	Project		: 전자식스위치
	File Name	: RS-485_COMMAX.C
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
#include "ThreeWay.h"

#ifdef _COMMAX_PROTOCOL_

/*#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1개용 - 일괄차단스위치
void BATCH_BLOCK_STATE_Process(void);
#else
void Batch_Light_Process(void);
#endif*/

void COMMAX_Data_Process(COMMAX_BUF	*pRx);
void BATCH_BLOCK_Control(uint8_t control);
uint8_t Get_485_Elec_ID(void);
uint8_t COMMAX_Batch_Light_State(uint8_t item);
// ----------------------------------------------------------------------------------------
static	COMMAX_BUF		RxBuf, TxBuf;

#define	MAX_COMMAX_DATA_SEQUENCE		9		//전등 종류 수는 최대 8이지만 COMMAX의 경우 전등 no를 1부터 시작하므로 8이아니라 최대 9로 설정함.
uint8_t	COMMAX_LIGHT_ITEM_Sequence[MAX_COMMAX_DATA_SEQUENCE];
uint8_t	COMMAX_ELEC_ITEM_Sequence[MAX_COMMAX_DATA_SEQUENCE];

uint8_t Store_Light_State[MAX_COMMAX_DATA_SEQUENCE];
uint8_t Gu8_Batch_Toggle_Tmr		= 0 ;
// uint8_t Gu8_Batch_Light_Flag;
// ----------------------------------------------------------------------------------------
void SET_COMMAX_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_COMMAX_DATA_SEQUENCE)
	{
		COMMAX_LIGHT_ITEM_Sequence[count]	= item;
	}
}

void SET_COMMAX_ELEC_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_COMMAX_DATA_SEQUENCE)
	{
		COMMAX_ELEC_ITEM_Sequence[count]	= item;
	}
}

void Protocol_Data_Init(void)
{
	uint8_t	count, i;
	
	memset((void*)&RxBuf,		0,	sizeof(COMMAX_BUF));
	memset((void*)&TxBuf,		0,	sizeof(COMMAX_BUF));
	
	Gu16_GAS_Off_Tmr				= 0;
	
	Gu16_Elevator_Tmr			= 0;
	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_NON;	

	memset(COMMAX_LIGHT_ITEM_Sequence, 0, MAX_COMMAX_DATA_SEQUENCE);	// 8개 항목 클리어
	memset(COMMAX_ELEC_ITEM_Sequence, 0, MAX_COMMAX_DATA_SEQUENCE);	// 8개 항목 클리어
	
	// 프로토콜 데이터 항목 순서
	// 전등모델	 최대항목	: 전등 6개, 전등 4개 + 디밍 2개
	// 전등+대기 최대항목	: 전등 4개 + 대기 2개, 전등2개 + 디밍2개 + 대기2개
	// ex) 조명 3개, 디밍 2개 = 조명1,조명2,조명3,디밍1,디밍2,0,0,0
	// ex) 조명 1개, 디밍 1개 = 조명1,디밍1,0,0,0,0,0,0
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// 프로토콜 순서 조명, 디밍전등 순으로
	count	= 1;		//COMMAX는 전등 1번 부터 사용하므로 편의상 1부터 시작
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
#endif
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// 프로토콜 순서 디밍전등, 조명 순으로
	count	= 1;		//COMMAX는 전등 1번 부터 사용하므로 편의상 1부터 시작
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_COMMAX_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
#endif
	
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// 대기전력 모델 
	count	= 1;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_COMMAX_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_COMMAX_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
#endif

	for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
	{
		// Store_Light_State[i]	= COMMAX_Batch_Light_State(COMMAX_LIGHT_ITEM_Sequence[i]);
		Store_Light_State[i]	= COMMAX_ON_FLAG;
	}
	// Gu8_Batch_Light_Flag = COMMAX_UNKNOWN_FLAG;	//ON FLAG 일 시, 일괄 스위치와 함께 사용하지 않는 경우에 Batch_Light_Process가 쓸데 없이 실행되므로 UNKNOWN FLAG로 함.
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
				printf("TX(COMMAX) : ");
				//printf("\nTX(COMMAX %d) : ", (uint16_t)TxBuf.count);
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
					printf("TX(COMMAX) : ");
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
uint8_t COMMAX_Crc(COMMAX_BUF *pTRx)
{
	uint8_t i, crc = 0;
	
	for(i = COMMAX_F_COMMAND; i < COMMAX_F_CHECK_SUM; i++)
	{
		crc += pTRx->buf[i];
	}
	
	return crc;
}
uint8_t NIS_Crc(COMMAX_BUF *pTRx, uint8_t cal, uint8_t sel)		//생산시 485통신 테스트를 위해서 추가함.
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
	// printf("input = 0x%x, %d\r\n", (uint16_t)input, (uint16_t)input);

	for(shift = 0; input > 0; shift++, input /= 10)
	{
		bcd |= input % 10 << (shift << 2);
	}
	*MSB = (uint8_t)(bcd >> 8);
	*LSB = (uint8_t)(bcd & 0xFF);
	// printf("IN : 0x%x\r\n", (uint16_t)input);
	// printf("BCD : 0x%x\r\n", (uint16_t)bcd);
    // printf("MSB : 0x%x\r\n", (uint16_t)*MSB);
    // printf("LSB : 0x%x\r\n", (uint16_t)*LSB);
}

uint16_t BIN_CODE(uint16_t input)
{
	uint16_t ret, binary = 0;
	int i;

	printf("IN : 0x%x\r\n", (uint16_t)input);

	for(i = 1; input != 0x00; input = (uint16_t)(input >> 4), i *= 10)
	{
		binary += (uint16_t)(i * (input & 0x0F));
		// printf("binary = 0x%x\r\n", (uint16_t)binary);
	}
	
	printf("binary = 0x%x\r\n", (uint16_t)binary);
	
	ret = binary;
	
	return ret;
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
	COMMAX_BUF	*pTx;
	pTx = &TxBuf;
	
	// Elec_Watt_MSB = (uint8_t)((int)Gu16_Elec_1_Watt + (int)Gu16_Elec_2_Watt);
	// printf("Watt1 = %f Watt2 = %f\r\n", (double)Gu16_Elec_1_Watt, (double)Gu16_Elec_2_Watt);
	
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
	pTx->buf[pTx->count++]	= Get_485_Elec_ID();

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
	COMMAX_BUF	*pRx;
	uint8_t		crc, crc_xor = 0;
	uint8_t     cnt = 0;
	int i;
	
	pRx = &RxBuf;
	
	if(G_Debug == DEBUG_HOST_REALDATA)
	{
		printf("\n");
		printf("%02X ", (uint16_t)data);
	}
	
	switch(pRx->count)
	{
		default:
			break;
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
		case 1:
			if((pRx->buf[COMMAX_F_COMMAND] != COMMAX_BATCH_BLOCK_COMM_REQUEST) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_BATCH_BLOCK_COMM_CONTROL) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_BATCH_BLOCK_COMM_GROUP_CONTROL) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_ELEVATOR_COMM_CONTROL) && (pRx->buf[COMMAX_F_COMMAND] != NIS_LIGHT_ID_COMM_1))
			{
				pRx->count = 0;
			}
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
		case 1:
			if((pRx->buf[COMMAX_F_COMMAND] != COMMAX_LIGHT_COMM_REQUEST) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_LIGHT_COMM_CONTROL) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_LIGHT_COMM_GROUP_CONTROL) && (pRx->buf[COMMAX_F_COMMAND] != NIS_LIGHT_ID_COMM_1)\
			&& (pRx->buf[COMMAX_F_COMMAND] != COMMAX_BATCH_BLOCK_COMM_REQUEST_RES) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_BATCH_BLOCK_COMM_CONTROL_RES))
			{
				pRx->count = 0;
			}
			break;
		case 2:
			if((pRx->buf[COMMAX_F_COMMAND] == COMMAX_LIGHT_COMM_REQUEST) || (pRx->buf[COMMAX_F_COMMAND] == COMMAX_LIGHT_COMM_CONTROL))
			{
				if((pRx->buf[COMMAX_F_BYTE_01] < Get_485_ID()) || (pRx->buf[COMMAX_F_BYTE_01] >= (uint8_t)(Get_485_ID() + Light_Cnt())))
				{
					pRx->count = 0;
				}
			}
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined (_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case 1:
			if((pRx->buf[COMMAX_F_COMMAND] != COMMAX_LIGHT_COMM_REQUEST) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_LIGHT_COMM_CONTROL) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_LIGHT_COMM_GROUP_CONTROL)\
			&& (pRx->buf[COMMAX_F_COMMAND] != COMMAX_ELEC_COMM_REQUEST) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_ELEC_COMM_CONTROL) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_ELEC_COMM_GROUP_CONTROL) && (pRx->buf[COMMAX_F_COMMAND] != NIS_LIGHT_ID_COMM_1)\
			&& (pRx->buf[COMMAX_F_COMMAND] != COMMAX_BATCH_BLOCK_COMM_REQUEST_RES) && (pRx->buf[COMMAX_F_COMMAND] != COMMAX_BATCH_BLOCK_COMM_CONTROL_RES))
			{
				pRx->count = 0;
			}		
			break;
		case 2:
			if((pRx->buf[COMMAX_F_COMMAND] == COMMAX_LIGHT_COMM_REQUEST) || (pRx->buf[COMMAX_F_COMMAND] == COMMAX_LIGHT_COMM_CONTROL))
			{
				if((pRx->buf[COMMAX_F_BYTE_01] < Get_485_ID()) || (pRx->buf[COMMAX_F_BYTE_01] >= (uint8_t)(Get_485_ID() + Light_Cnt())))
				{
					pRx->count = 0;
				}
			}
			else if((pRx->buf[COMMAX_F_COMMAND] == COMMAX_ELEC_COMM_REQUEST) || (pRx->buf[COMMAX_F_COMMAND] == COMMAX_ELEC_COMM_CONTROL))
			{
				if((pRx->buf[COMMAX_F_BYTE_01] < Get_485_Elec_ID()) || (pRx->buf[COMMAX_F_BYTE_01] >= (uint8_t)(Get_485_Elec_ID() + Elec_Cnt())))
				{
					pRx->count = 0;
				}
			}
			break;			
#endif
	}
	pRx->buf[pRx->count++] = data;
	
	if(pRx->count >= COMMAX_MAX_BUF)
	{
		if(pRx->buf[COMMAX_F_COMMAND] == NIS_LIGHT_ID_COMM_1)		//485 통신 테스트를 위해서 추가함
		{
			crc = NIS_Crc(pRx, 0, NIS_RX);
			crc_xor = NIS_Crc(pRx, 1, NIS_RX);
			if(crc == pRx->buf[COMMAX_F_CHECK_SUM] && crc_xor == pRx->buf[COMMAX_F_BYTE_06])
			{
				if(G_Debug == DEBUG_HOST)
				{
					printf("\nRX(COMMAX) : ");
					for(i=0;i<pRx->count;i++)
					{
						printf("%02X ", (uint16_t)pRx->buf[i]);
					}
					printf("\n");
				}
				COMMAX_Data_Process(pRx);
			}
			else
			{
				if(crc_xor != pRx->buf[COMMAX_F_BYTE_06])	printf("cal crc_xor[0x%02X] != buf crc_xor[0x%02X]", (uint16_t)crc_xor, (uint16_t)pRx->buf[COMMAX_F_BYTE_06]);
				if(crc != pRx->buf[COMMAX_F_CHECK_SUM])		printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[COMMAX_F_CHECK_SUM]);
			}
		}
		else
		{
			crc = COMMAX_Crc(pRx);
			if(crc == pRx->buf[COMMAX_F_CHECK_SUM])
			{
				if(G_Debug == DEBUG_HOST)
				{
					printf("\nRX(COMMAX) : ");
					for(i=0;i<pRx->count;i++)
					{
						printf("%02X ", (uint16_t)pRx->buf[i]);
					}
					printf("\n");
				}
				COMMAX_Data_Process(pRx);
			}
			else
			{
				printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[COMMAX_F_CHECK_SUM]);
			}
		}
		pRx->buf[0] = 0;
		pRx->count = 0;
	}
}

void SET_DimmingLevel(uint8_t item, uint8_t Dimming_Level)
{
	switch(item)
	{
		case mapping_ITEM_DIMMING_LIGHT_1:
			if(pG_State->Dimming_Level.Dimming1	!= (uint8_t)Dimming_Level)	Beep(ON);				// 월패드에서 제어했을때 부저음으로 제어 됐는지 구분이 쉽도록
			pG_State->Dimming_Level.Dimming1		= (uint8_t)Dimming_Level;
			if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:			
			if(pG_State->Dimming_Level.Dimming2	!= (uint8_t)Dimming_Level)	Beep(ON);				// 월패드에서 제어했을때 부저음으로 제어 됐는지 구분이 쉽도록
			pG_State->Dimming_Level.Dimming2		= (uint8_t)Dimming_Level;
			if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
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
			if(pG_State->Color_Temp_Level.Color_Temp1	!= (uint8_t)Color_Level)	Beep(ON);				// 월패드에서 제어했을때 부저음으로 제어 됐는지 구분이 쉽도록
			pG_State->Color_Temp_Level.Color_Temp1	= Color_Level;
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp1 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// 최대 크기로 설정
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(pG_State->Color_Temp_Level.Color_Temp2	!= (uint8_t)Color_Level)	Beep(ON);				// 월패드에서 제어했을때 부저음으로 제어 됐는지 구분이 쉽도록
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
			if(pG_State->ETC.Auto1 != flag)
			{
				if(flag == COMMAX_AUTO_FLAG)	pG_State->ETC.Auto1	= 1;
				else							pG_State->ETC.Auto1	= 0; 
				Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
				Beep(ON);
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(pG_State->ETC.Auto2 != flag)
			{				
				if(flag == COMMAX_AUTO_FLAG)	pG_State->ETC.Auto2	= 1;
				else							pG_State->ETC.Auto2	= 0;
				Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
				Beep(ON);
			}
			break;
	}
}

void ElecLimit_Save(uint8_t item, COMMAX_BUF *pRx)
{
	uint16_t Elec_Current_H, Elec_Current_L;

	Elec_Current_H = (uint16_t)(pRx->buf[COMMAX_F_BYTE_03] << 8);
	Elec_Current_L = (uint16_t)pRx->buf[COMMAX_F_BYTE_04];

	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
			if((pRx->buf[COMMAX_F_BYTE_05] & 0x0F) == 0x00)		Gu16_ElecLimitCurrent_1 = (uint16_t)(BIN_CODE(Elec_Current_H) + BIN_CODE(Elec_Current_L));
			else if((pRx->buf[COMMAX_F_BYTE_05] & 0x0F) == 0x01)Gu16_ElecLimitCurrent_1 = (uint16_t)((BIN_CODE(Elec_Current_H) + BIN_CODE(Elec_Current_L)) / 10);
			else if((pRx->buf[COMMAX_F_BYTE_05] & 0x0F) == 0x02)Gu16_ElecLimitCurrent_1 = (uint16_t)((BIN_CODE(Elec_Current_H) + BIN_CODE(Elec_Current_L)) / 100);
			else if((pRx->buf[COMMAX_F_BYTE_05] & 0x0F) == 0x03)Gu16_ElecLimitCurrent_1 = (uint16_t)((BIN_CODE(Elec_Current_H) + BIN_CODE(Elec_Current_L)) / 1000);
			Store_ElecLimitCurrent();		
			break;	
		case mapping_ITEM_ELECTRICITY_2:
			if((pRx->buf[COMMAX_F_BYTE_05] & 0x0F) == 0x00)		Gu16_ElecLimitCurrent_2 = (uint16_t)(BIN_CODE(Elec_Current_H) + BIN_CODE(Elec_Current_L));
			else if((pRx->buf[COMMAX_F_BYTE_05] & 0x0F) == 0x01)Gu16_ElecLimitCurrent_2 = (uint16_t)((BIN_CODE(Elec_Current_H) + BIN_CODE(Elec_Current_L)) / 10);
			else if((pRx->buf[COMMAX_F_BYTE_05] & 0x0F) == 0x02)Gu16_ElecLimitCurrent_2 = (uint16_t)((BIN_CODE(Elec_Current_H) + BIN_CODE(Elec_Current_L)) / 100);
			else if((pRx->buf[COMMAX_F_BYTE_05] & 0x0F) == 0x03)Gu16_ElecLimitCurrent_2 = (uint16_t)((BIN_CODE(Elec_Current_H) + BIN_CODE(Elec_Current_L)) / 1000);			
			Store_ElecLimitCurrent();
			break;
	}
}

uint8_t COMMAX_Batch_Light_State(uint8_t item)		//일괄 소등 전 현재 전등 상태 저장.
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
			if(GET_Switch_State(item2tsn(item)))
			{
				ret = COMMAX_ON_FLAG;
			}
			else
			{
				ret = COMMAX_OFF_FLAG;
			}
			break;
	}
	return ret;
}

void COMMAX_BatchLight_Control(uint8_t item, uint8_t control_value)
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
			if(control_value == COMMAX_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == COMMAX_ON_FLAG)
				{
					EventCtrl(item2tsn(item), OFF);						//전등 OFF
				}
			}
			else if(control_value == COMMAX_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == COMMAX_OFF_FLAG)
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

void COMMAX_Control(COMMAX_BUF *pRx, uint8_t item)
{
	uint8_t	Flag = OFF, touch_switch, tmr, control;
	uint8_t	Color_Level, Dimming_Level;

	Color_Level		= pRx->buf[COMMAX_F_BYTE_05];
	Dimming_Level	= pRx->buf[COMMAX_F_BYTE_06];
	control			= pRx->buf[COMMAX_F_BYTE_02];
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
			if((control & COMMAX_ON_FLAG) == COMMAX_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)
				{
					EventCtrl(item2tsn(item), ON);
				}
			}
			else if((control & COMMAX_OFF_FLAG) == COMMAX_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))
				{
					EventCtrl(item2tsn(item), OFF);
				}
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
			if((control & COMMAX_ON_FLAG) == COMMAX_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)
				{
					EventCtrl(item2tsn(item), ON);
				}
				else		//월패드에서 디밍 제어시에도 byte_02의 값이 0x01(ON/OFF)로 와서 추가함.
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
					Gu8_LCD_DIM_Tmr					= 20;		// 2s 동안 LCD 표시					
					SET_DimmingLevel(item, Dimming_Level);			//월패드에서 디밍 레벨 변경시 0x02으로 오지않고 0x01로 옴....
					SET_DimmingColorLevel(item, Color_Level);		//월패드에서 색온도 레벨 변경시 0x04으로 오지않고 0x01로 옴....
				}
				if((control & COMMAX_DIMMING) == COMMAX_DIMMING)
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
					Gu8_LCD_DIM_Tmr					= 20;		// 2s 동안 LCD 표시
					SET_DimmingLevel(item, Dimming_Level);
				}
				if((control & COMMAX_COLOR_TEMP) == COMMAX_COLOR_TEMP)
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
					Gu8_LCD_DIM_Tmr					= 20;		// 2s 동안 LCD 표시					
					SET_DimmingColorLevel(item, Color_Level);
				}
			}
			else if((control & COMMAX_OFF_FLAG) == COMMAX_OFF_FLAG)
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
			if(control == COMMAX_ON_n_OFF)
			{
				if(pRx->buf[COMMAX_F_BYTE_03])		//ON
				{
					if(GET_Switch_State(item2tsn(item)) == 0)
					{
						touch_switch	= item2tsn(item);
						SET_Switch_State(touch_switch, ON);
						SET_LED_State(touch_switch, ON);
						PUT_RelayCtrl(item2ctrl(item), ON);	// 항목기준 제어
						SET_SWITCH_Delay_OFF_Flag(item, 0);
						SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
						ALL_Electricity_Switch_LED_Ctrl();						
						Beep(ON);
					}
				}
				else								//OFF
				{
					if(GET_Switch_State(item2tsn(item)))
					{
						touch_switch	= item2tsn(item);
						SET_Switch_State(touch_switch, OFF);
						SET_LED_State(touch_switch, OFF);
						PUT_RelayCtrl(item2ctrl(item), OFF);	// 항목기준 제어
						SET_SWITCH_Delay_OFF_Flag(item, 0);
						SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
						ALL_Electricity_Switch_LED_Ctrl();						
						Beep(OFF);
					}
				}

			}
			else if(control == COMMAX_BLOCK_MODE)
			{
				if(pRx->buf[COMMAX_F_BYTE_03])		//차단 모드 자동
				{
					SET_Elec_Auto_Manual(item, COMMAX_AUTO_FLAG);
				}
				else								//차단 모드 수동
				{
					SET_Elec_Auto_Manual(item, COMMAX_MANUAL_FLAG);
				}

			}
			else if(control == COMMAX_STANDBY_VALUE_SAVE)		//대기전력 값 저장
			{													//대기전력 값 저장 제어가 오면 byte 03, byte 04까지 대기전력 값을 저장해야 한다.
				ElecLimit_Save(item, pRx);
			}

			break;
#endif
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// 복구
			break;
	}
}

void COMMAX_Light_GROUP_Control(COMMAX_BUF *pRx)
{
	uint8_t i, Sub_No, item;
	uint8_t Flag = 0, cnt = 1, k = 1, j = 0;
	uint8_t dim_level;

	if(pRx->buf[COMMAX_F_COMMAND] == COMMAX_LIGHT_COMM_GROUP_CONTROL)
	{
		dim_level = pRx->buf[COMMAX_F_BYTE_06];
		for(j = COMMAX_F_BYTE_01; j < COMMAX_F_BYTE_06; j++)
		{
			for(i = 1; i <= 8; i++)
			{
				Sub_No = (uint8_t)(pRx->buf[j] >> (i - 1));
				if((Get_485_ID() <= cnt) && (cnt < (Get_485_ID() + Light_Cnt())))
				{
					item = COMMAX_LIGHT_ITEM_Sequence[k++];
					if((Sub_No & 0x01) == 0x01)		Flag = 1;
					else							Flag = 0;

					switch(item)
					{
						case mapping_ITEM_LIGHT_1:
						case mapping_ITEM_LIGHT_2:
						case mapping_ITEM_LIGHT_3:
						case mapping_ITEM_LIGHT_4:
						case mapping_ITEM_LIGHT_5:
						case mapping_ITEM_LIGHT_6:
						case mapping_ITEM_3WAY_1:
						case mapping_ITEM_3WAY_2:
						case mapping_ITEM_DIMMING_LIGHT_1:
						case mapping_ITEM_DIMMING_LIGHT_2:
							if(item == mapping_ITEM_DIMMING_LIGHT_1 || item == mapping_ITEM_DIMMING_LIGHT_2)
							{
								if((dim_level >= 1) && (dim_level <= 8))
								{
									Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
									SET_DimmingLevel(item, dim_level);		//Byte_06이 0이면 기존 디밍레벨 유지, 1 ~ 8 은 디밍레벨로 On한다.
								}
							}
							if((GET_Switch_State(item2tsn(item)) == COMMAX_ON_FLAG) && (Flag == COMMAX_OFF_FLAG))
							{
								Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
								EventCtrl(item2tsn(item), COMMAX_OFF_FLAG);
							}
							else if((GET_Switch_State(item2tsn(item)) == COMMAX_OFF_FLAG) && (Flag == COMMAX_ON_FLAG))
							{
								Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
								EventCtrl(item2tsn(item), COMMAX_ON_FLAG);
							}
							
							break;
					}					
				}
				else
				{
					k = 1;		//k값은 해당 스위치에 범위에 맞는 cnt 값이 되었을때 증가하고, 범위를 벗어 난 경우 1로 초기화 함.
				}
				cnt++;
			}

		}
	}

}

void COMMAX_Elec_GROUP_Control(COMMAX_BUF *pRx)
{
	uint8_t i, Sub_No, item, Control_Type, Control_Data;
	uint8_t Flag = 0, touch_switch = 0, cnt = 1, k = 1;

	if(pRx->buf[COMMAX_F_COMMAND] == COMMAX_ELEC_COMM_GROUP_CONTROL)
	{
		Control_Type	= pRx->buf[COMMAX_F_BYTE_02];
		Control_Data	= pRx->buf[COMMAX_F_BYTE_03];

		for(i = 1; i <= 8; i++)
		{
			Sub_No = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] >> (i - 1));
			if((Get_485_Elec_ID() <= cnt) && (cnt < (uint8_t)(Get_485_Elec_ID() + Elec_Cnt())))
			{
				if((Sub_No & 0x01) == 0x01)
				{
					item = COMMAX_ELEC_ITEM_Sequence[k++];
					// printf("buf[%d] = %x, Sub_No = %x, cnt = %d, itme = %d\r\n", (uint16_t)i, (uint16_t)pRx->buf[i], (uint16_t)Sub_No, (uint16_t)cnt, (uint16_t)item);
					switch(item)
					{
						case mapping_ITEM_ELECTRICITY_1:
						case mapping_ITEM_ELECTRICITY_2:
							if(Control_Type == COMMAX_ON_n_OFF)
							{
								if(GET_Switch_State(item2tsn(item)) == 0)
								{
									if(Control_Data == COMMAX_ON_FLAG)
									{
										Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
										touch_switch = item2tsn(item);
										SET_Switch_State(touch_switch, ON);
										SET_LED_State(touch_switch, ON);
										Beep(ON);
										PUT_RelayCtrl(item2ctrl(item), ON);
										SET_SWITCH_Delay_OFF_Flag(item, 0);
										SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);
										ALL_Electricity_Switch_LED_Ctrl();									
									}
								}
								else
								{
									if(Control_Data	== COMMAX_OFF_FLAG)
									{
										Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
										touch_switch = item2tsn(item);
										SET_Switch_State(touch_switch, OFF);
										SET_LED_State(touch_switch, OFF);
										Beep(OFF);
										PUT_RelayCtrl(item2ctrl(item), OFF);
										SET_SWITCH_Delay_OFF_Flag(item, 0);
										SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);
										ALL_Electricity_Switch_LED_Ctrl();	
									}
								}

							}
							else if(Control_Type == COMMAX_BLOCK_MODE)
							{
								SET_Elec_Auto_Manual(item, Flag);
							}
							else if(Control_Type == COMMAX_STANDBY_VALUE_SAVE)
							{
								ElecLimit_Save(item, pRx);
							}					
							break;
					}
				}
				else
				{
					item = COMMAX_ELEC_ITEM_Sequence[k++];
				}
			}
			else
			{
				k = 1;
			}
			cnt++;
		}
	}
}

uint8_t COMMAX_LIGHT_Data_Conversion(uint8_t item, uint8_t ack)
{
	uint8_t	ret = 0;
	
	switch(item)
	{
		default:
			ret = COMMAX_OFF_FLAG;
			break;
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			if(ack == COMMAX_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = COMMAX_ON_FLAG;
				else											ret = COMMAX_OFF_FLAG;
			}
			else												ret = COMMAX_OFF_FLAG;
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
			if(ack == COMMAX_DIMMING)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = pG_State->Dimming_Level.Dimming1;
				else											ret = COMMAX_OFF_FLAG;
			}
			else if(ack == COMMAX_COLOR_TEMP)
			{
				if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)		//색온도 조명 사용하지 않는 디밍전등에서 색온도 값을 전달하지 않기 위해서 추가함
				{
					if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)((pG_Config->Color_Temp_MAX_Level << 4) | pG_State->Color_Temp_Level.Color_Temp1);
					else											ret = COMMAX_OFF_FLAG;
				}
				else												ret = COMMAX_OFF_FLAG;
			}
			else if(ack == COMMAX_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))		ret = COMMAX_ON_FLAG;
				else												ret = COMMAX_OFF_FLAG;
			}
			else if(ack == COMMAX_MAX_DIM_LEVEL)					ret = pG_Config->Dimming_MAX_Level;
			
			else													ret = COMMAX_OFF_FLAG;
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
			if(ack == COMMAX_DIMMING)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))		ret = pG_State->Dimming_Level.Dimming2;
				else												ret = COMMAX_OFF_FLAG;
			}
			else if(ack == COMMAX_COLOR_TEMP)
			{
				if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)		//색온도 조명 사용하지 않는 디밍전등에서 색온도 값을 전달하지 않기 위해서 추가함
				{
					if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)((pG_Config->Color_Temp_MAX_Level << 4) | pG_State->Color_Temp_Level.Color_Temp2);
					else											ret = COMMAX_OFF_FLAG;
				}
				else												ret = COMMAX_OFF_FLAG;
			}
			else if(ack == COMMAX_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))		ret = COMMAX_ON_FLAG;
				else												ret = COMMAX_OFF_FLAG;
			}
			else if(ack == COMMAX_MAX_DIM_LEVEL)					ret = pG_Config->Dimming_MAX_Level;
			
			else													ret = COMMAX_OFF_FLAG;
			break;
	}
	return	ret;
}

uint8_t COMMAX_ELEC_Data_Conversion(uint8_t item)
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
				ret	= (uint8_t)(COMMAX_ON_FLAG | (pG_State->ETC.Auto1 << 4));
			}
			else
			{
				ret	= (uint8_t)(COMMAX_OFF_FLAG | (pG_State->ETC.Auto1 << 4));
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit4 자동/수동, bit0 전원 ON/OFF
			{
				ret	= (uint8_t)(COMMAX_ON_FLAG | (pG_State->ETC.Auto2 << 4));
			}
			else
			{
				ret	= (uint8_t)(COMMAX_OFF_FLAG | (pG_State->ETC.Auto2 << 4));
			}
			break;
	}
	
	return	ret;
}

/*void Batch_Light_Process(void)						//일괄점등 상태에서 현재 전등 상태를 저장하여, 일괄소등->일괄점등 때 저장된 상태로 복귀.
{
	uint8_t i = 1;
	static uint8_t cnt = 0;
	
	if(Gu8_Batch_Light_Flag != COMMAX_ON_FLAG)	return;

	if(cnt == 0)
	{
		if(Gu8_Batch_Light_Flag == COMMAX_ON_FLAG)		//일괄 점등 상태면 현재 전등 상태 저장.
		{
			for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
			{
				Store_Light_State[i] = COMMAX_Batch_Light_State(COMMAX_LIGHT_ITEM_Sequence[i]);
				printf("%d ", (uint16_t)Store_Light_State[i]);
			}
			printf("\n");
		}
		cnt++;
	}
	else
	{
		if(cnt >= 6)	cnt = 0;
		else			cnt++;
	}
}*/
//------------------------------------------------------------------------------------------------------------------------------------------
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)		// 1개용, 2개용	- 전등모델
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}

uint8_t Get_485_Elec_ID(void)
{
	return (uint8_t)(pG_Config->RS485_Elec_ID);
}

void COMMAX_LIGHT_COMM_Request_Res(COMMAX_BUF *pRx)				//개별전등 상태 요구에 대한 응답
{
	uint8_t	light_no, item = 0;
	COMMAX_BUF	*pTx;
	pTx = &TxBuf;
	
	if(Get_485_ID() <= 1)	light_no = pRx->buf[COMMAX_F_BYTE_01];
	else					light_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_ID() + 1);

	pTx->count	= COMMAX_F_COMMAND;

	pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[COMMAX_F_COMMAND] + 0x80);		//ACK의 Command는 요구 Command + 0x80 이다.
	pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(COMMAX_LIGHT_ITEM_Sequence[light_no], COMMAX_ON_n_OFF);		//ON/OFF  상태
	pTx->buf[pTx->count++]	= pRx->buf[COMMAX_F_BYTE_01];																//ACK 전등번호
	pTx->buf[pTx->count++]	= 0x00;
	pTx->buf[pTx->count++]	= 0x00;																						//조명색 사용하지 않으므로 조명색단계 없음
	pTx->buf[pTx->count++]	= 0x00;																						//디밍 사용하지 않으므로 현재 디밍단계 없음
	pTx->buf[pTx->count++]	= 0x00;																						//디밍 사용하지 않으므로 디밍단계 최대 값 없음
	/*	전등모델에 디밍, 색온도 옵션 추가되면 사용
	pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(COMMAX_LIGHT_ITEM_Sequence[light_no], COMMAX_COLOR_TEMP);	//조명색단계		전드모델은 현재 색온도 옵션 없음
	pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(COMMAX_LIGHT_ITEM_Sequence[light_no], COMMAX_DIMMING);		//현재의 디밍단계	전등모델은 현재 디밍 옵션 없음
	pTx->buf[pTx->count++]	= pG_Config->Dimming_MAX_Level;																//디밍단계 최대값	전등모델은 현재 디밍 옵션 없음
	*/
	pTx->buf[pTx->count++]	= COMMAX_Crc(pTx);																			//Checksum
	TxBuf.send_flag	= 1;
}

void COMMAX_Data_Process(COMMAX_BUF	*pRx)
{
	uint16_t	i = 0, cnt = 1;
	uint8_t		light_no, item = 0;
	static uint8_t Batch_Old_Flag = 0;
	static uint8_t Batch_Toggle_Flag = 0;

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	switch(pRx->buf[COMMAX_F_COMMAND])
	{
		case COMMAX_LIGHT_COMM_REQUEST:
			if((Get_485_ID() + Light_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])		//Sub no가 내 아이디 + 전등 수 보다 작을때, ex) id = 1, light_cnt = 4, id + light = 5, 5 > sub no, 다음 스위치 id는 5부터 시작
			{
				COMMAX_LIGHT_COMM_Request_Res(pRx);
			}
			break;
		case COMMAX_LIGHT_COMM_CONTROL:
			if((Get_485_ID() + Light_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])
			{
				if(Get_485_ID() <= 1)		//ID가 1이하면, 코맥스에서 0은 쓰지않으나 id가 0이면 동작이 달라져서 예외처리 필요할지도
				{
					light_no = pRx->buf[COMMAX_F_BYTE_01];
					COMMAX_Control(pRx, COMMAX_LIGHT_ITEM_Sequence[light_no]);
					COMMAX_LIGHT_COMM_Request_Res(pRx);
				}
				else					//ID가 1보다 클 때
				{
					light_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_ID() + 1);		//ex) id = 5, light_cnt = 4, byte_01 = 5, 6, 7, 8 즉, byte_01 - id + 1
																								//ex) id = 7, light_cnt = 4, byte_01 = 7, 8, 9, 10 즉, byte_01 - id +1
					COMMAX_Control(pRx, COMMAX_LIGHT_ITEM_Sequence[light_no]);
					COMMAX_LIGHT_COMM_Request_Res(pRx);
				}
			}
			break;		
		case COMMAX_LIGHT_COMM_GROUP_CONTROL:
			if((pRx->buf[COMMAX_F_BYTE_01] | pRx->buf[COMMAX_F_BYTE_02] | pRx->buf[COMMAX_F_BYTE_03] | pRx->buf[COMMAX_F_BYTE_04] | pRx->buf[COMMAX_F_BYTE_05] | pRx->buf[COMMAX_F_BYTE_06]) == 0)
			{
				if(Batch_Toggle_Flag)
				{
					for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)	//221114
					{
						Store_Light_State[i] = COMMAX_Batch_Light_State(COMMAX_LIGHT_ITEM_Sequence[i]);	//현재 전등 스위치 상태 저장. 제어는 하지 않음. 제어는 월패드에서 그룹 제어로 함.
						printf("%d ", (uint16_t)Store_Light_State[i]);
					}
					Batch_Toggle_Flag = 0;
				}
			}
			COMMAX_Light_GROUP_Control(pRx);		//ACK 없음
			break;
		case COMMAX_BATCH_BLOCK_COMM_REQUEST_RES:					//(0xA0)일괄 스위치 상태 요청에 대한 응답. 응답에서 일괄 소등 상태를 파악.
			if(Batch_Old_Flag == COMMAX_OFF_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_ON_FLAG)
			{
				for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
				{
					// COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);	//소등 후 전등 제어했을 때, 복귀 시 제어한 전등도 소등 전 상태로 복귀
					if(Store_Light_State[i] == COMMAX_ON_FLAG)		//ON만 하게되면 일괄 소등 이후 전등 제어 했을 때, 복귀 시 제어 한 전등은 그대로 켜진 상태로 꺼진 전등만 복구됨
					{
						COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], COMMAX_ON_FLAG);			//소등 이전 상태로 전등ON + 소등 후 개별 제어로 ON한 전등은 유지
					}
				}
			}
			else if(Batch_Old_Flag == COMMAX_ON_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_OFF_FLAG)
			{
				Gu8_Batch_Toggle_Tmr = 5;
				Batch_Toggle_Flag = 1;
			}

			if(Batch_Toggle_Flag)	//플래그 있을 때, 타이머가 0이되면 플래그 0으로,(일괄 소등해제 -> 일괄 소등 되었을 때 플래그 세워짐)
			{
				if(Gu8_Batch_Toggle_Tmr == 0)		//해당 플래그는 일괄 소등 받아 그룹제어로 전등 상태 저장 시 사용함. 타이머는 일정 시간 지났을 경우
				{									//전등 상태 저장 하지 않도록 하기 위해서
					Batch_Toggle_Flag = 0;
				}
			}
			Batch_Old_Flag = pRx->buf[COMMAX_F_BYTE_01];
			break;
		case COMMAX_BATCH_BLOCK_COMM_CONTROL_RES:				//월패드에서 일괄 제어 할때 일괄 스위치의 응답.
			if(Batch_Old_Flag == COMMAX_OFF_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_ON_FLAG)
			{
				for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
				{
					COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);
					/*if(Store_Light_State[i] == COMMAX_ON_FLAG)
					{
						COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], COMMAX_ON_FLAG);			//소등 이전 상태로 전등ON + 소등 후 개별 제어로 ON한 전등은 유지
					}*/
				}
			}
			else if(Batch_Old_Flag == COMMAX_ON_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_OFF_FLAG)
			{
				Gu8_Batch_Toggle_Tmr = 5;
				Batch_Toggle_Flag = 1;
			}			
			Batch_Old_Flag = pRx->buf[COMMAX_F_BYTE_01];
			break;
		case NIS_LIGHT_ID_COMM_1:				//생산시 프로토콜 타입과 아이디를 조회하기 위해서 추가함.
			if(pRx->buf[1] == NIS_LIGHT_ID_COMM_2)
			{
				//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
				/*if(pRx->buf[2] != pG_Config->RS485_ID || pRx->buf[3] != pG_Config->RS485_Elec_ID)
				{
					if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
					{
						pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
						printf("Switch ID Change\r\n");
					}
					if(pRx->buf[3] != pG_Config->RS485_Elec_ID)
					{
						pG_Config->RS485_Elec_ID = pRx->buf[3];
						printf("Elec ID Change\r\n");
					}
					Store_CurrentConfig();
				}*/
				RS_485_ID_RES();
			}
			break;
	}
}
#endif	// defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)	// 2개용	- 전등+전열모델
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}

uint8_t Get_485_Elec_ID(void)
{
	return (uint8_t)(pG_Config->RS485_Elec_ID);
}

void COMMAX_Light_n_ELEC_COMM_Request_Res(COMMAX_BUF *pRx)				//개별전등 상태 요구에 대한 응답
{
	uint8_t	light_no, elec_no, item;
	uint8_t		Watt_MSB, Watt_LSB = 0;
	COMMAX_BUF	*pTx;
	pTx = &TxBuf;

	switch(pRx->buf[COMMAX_F_COMMAND])
	{
		case COMMAX_LIGHT_COMM_REQUEST:
		case COMMAX_LIGHT_COMM_CONTROL:
			if(Get_485_ID() <= 1)	light_no = pRx->buf[COMMAX_F_BYTE_01];
			else					light_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_ID() + 1);
			
			item = COMMAX_LIGHT_ITEM_Sequence[light_no];
			pTx->count				= COMMAX_F_COMMAND;

			pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[COMMAX_F_COMMAND] + 0x80);				//ACK의 Command는 요구 Command + 0x80 이다.
			pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(item, COMMAX_ON_n_OFF);		//ON/OFF  상태
			pTx->buf[pTx->count++]	= pRx->buf[COMMAX_F_BYTE_01];								//ACK 전등번호
			pTx->buf[pTx->count++]	= 0x00;
			pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(item, COMMAX_COLOR_TEMP);	//조명색단계
			pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(item, COMMAX_DIMMING);		//현재의 디밍단계
			pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(item, COMMAX_MAX_DIM_LEVEL);	//디밍단계 최대값
			pTx->buf[pTx->count++]	= COMMAX_Crc(pTx);											//Checksum
			TxBuf.send_flag	= 1;
			break;
		case COMMAX_ELEC_COMM_REQUEST:
		case COMMAX_ELEC_COMM_CONTROL:
			if(Get_485_Elec_ID() <= 1)	elec_no = pRx->buf[COMMAX_F_BYTE_01];
			else						elec_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_Elec_ID() + 1);

			item = COMMAX_ELEC_ITEM_Sequence[elec_no];
			pTx->count				= COMMAX_F_COMMAND;

			pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[COMMAX_F_COMMAND] + 0x80);
			pTx->buf[pTx->count++]	= COMMAX_ELEC_Data_Conversion(item);							//ON/OFF  | 자동/수동
			pTx->buf[pTx->count++]	= pRx->buf[COMMAX_F_BYTE_01];									//ACK 전원제어번호
			pTx->buf[pTx->count++]	= (uint8_t)((pRx->buf[COMMAX_F_BYTE_02] << 4) | 0);				//Type | 소수점 자릿수(현재 스위치에 전력값은 소수점 표현이 없으므로 소숫점 자리 없음으로 표시.)
			
			if(pRx->buf[COMMAX_F_COMMAND] == COMMAX_ELEC_COMM_REQUEST)
			{
				switch(pRx->buf[COMMAX_F_BYTE_02])
				{
					case TYPE_USAGE:		//사용량(순시치)
						if(item == mapping_ITEM_ELECTRICITY_1)		BCD_CODE(Gu16_LCD_Watt_1, &Watt_MSB, &Watt_LSB);
						else if(item == mapping_ITEM_ELECTRICITY_2) BCD_CODE(Gu16_LCD_Watt_2, &Watt_MSB, &Watt_LSB);

						pTx->buf[pTx->count++]	= 0;				//byte4
						pTx->buf[pTx->count++]	= Watt_MSB;			//byte5
						pTx->buf[pTx->count++]	= Watt_LSB;			//byte6
						break;
					case TYPE_STANDBY_VALUE_SAVE:		//대기전력 값 저장??
						if(item == mapping_ITEM_ELECTRICITY_1)		BCD_CODE(Gu16_ElecLimitCurrent_1, &Watt_MSB, &Watt_LSB);
						else if(item == mapping_ITEM_ELECTRICITY_2)	BCD_CODE(Gu16_ElecLimitCurrent_2, &Watt_MSB, &Watt_LSB);
						pTx->buf[pTx->count++]	= 0;				//byte4
						pTx->buf[pTx->count++]	= Watt_MSB;			//byte5
						pTx->buf[pTx->count++]	= Watt_LSB;			//byte6
						break;
					case TYPE_USAGE_ONE_HOURS:		//사용량(선택사항, 1시간 누적치)
						pTx->buf[pTx->count++] = 0;
						pTx->buf[pTx->count++] = 0;
						pTx->buf[pTx->count++] = 0;
						break;
				}
			}
			else if(pRx->buf[COMMAX_F_COMMAND] == COMMAX_ELEC_COMM_CONTROL)
			{
				switch(pRx->buf[COMMAX_F_BYTE_02])
				{
					case COMMAX_ON_n_OFF:		//사용량(순시치)
					case COMMAX_BLOCK_MODE:
						if(item == mapping_ITEM_ELECTRICITY_1)		BCD_CODE(Gu16_LCD_Watt_1, &Watt_MSB, &Watt_LSB);
						else if(item == mapping_ITEM_ELECTRICITY_2) BCD_CODE(Gu16_LCD_Watt_2, &Watt_MSB, &Watt_LSB);
						pTx->buf[pTx->count++]	= 0;				//byte4
						pTx->buf[pTx->count++]	= Watt_MSB;			//byte5
						pTx->buf[pTx->count++]	= Watt_LSB;			//byte6
						break;
					case COMMAX_STANDBY_VALUE_SAVE:		//대기전력 값 저장
						if(item == mapping_ITEM_ELECTRICITY_1)		BCD_CODE(Gu16_ElecLimitCurrent_1, &Watt_MSB, &Watt_LSB);
						else if(item == mapping_ITEM_ELECTRICITY_2)	BCD_CODE(Gu16_ElecLimitCurrent_2, &Watt_MSB, &Watt_LSB);
						pTx->buf[pTx->count++]	= 0;				//byte4
						pTx->buf[pTx->count++]	= Watt_MSB;			//byte5
						pTx->buf[pTx->count++]	= Watt_LSB;			//byte6				
						break;
				}
			}
			pTx->buf[pTx->count++]	= COMMAX_Crc(pTx);							//Checksum
			TxBuf.send_flag	= 1;			
			break;
	}
}

void COMMAX_Data_Process(COMMAX_BUF	*pRx)
{
	uint16_t	i = 0, cnt = 1;
	uint8_t		light_no = 0, elec_no = 0, item = 0;
	static uint8_t Batch_Old_Flag = 0;
	static uint8_t Batch_Toggle_Flag = 0;

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	switch(pRx->buf[COMMAX_F_COMMAND])
	{
		case COMMAX_LIGHT_COMM_REQUEST:
			if((Get_485_ID() + Light_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])		//Sub no가 내 아이디 + 전등 수 보다 작을때, ex) id = 1, light_cnt = 4, id + light = 5, 5 > sub no, 다음 스위치 id는 5부터 시작
			{
				COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
			}
			break;
		case COMMAX_ELEC_COMM_REQUEST:
			if((Get_485_Elec_ID() + Elec_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])		//Sub no가 내 아이디 + 전등 수 보다 작을때, ex) id = 1, light_cnt = 4, id + light = 5, 5 > sub no, 다음 스위치 id는 5부터 시작
			{
				COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
			}
			break;
		case COMMAX_LIGHT_COMM_CONTROL:
			if((Get_485_ID() + Light_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])
			{
				if(Get_485_ID() <= 1)		//ID가 1이하면
				{
					light_no = pRx->buf[COMMAX_F_BYTE_01];
					COMMAX_Control(pRx, COMMAX_LIGHT_ITEM_Sequence[light_no]);
					COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
				}
				else					//ID가 1보다 클 때
				{
					light_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_ID() + 1);		//ex) id = 5, light_cnt = 4, byte_01 = 5, 6, 7, 8 즉, byte_01 - id + 1
																								//ex) id = 7, light_cnt = 4, byte_01 = 7, 8, 9, 10 즉, byte_01 - id +1
					COMMAX_Control(pRx, COMMAX_LIGHT_ITEM_Sequence[light_no]);
					COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
				}
			}
			break;
		case COMMAX_ELEC_COMM_CONTROL:
			if((Get_485_Elec_ID() + Elec_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])
			{
				if(Get_485_Elec_ID() <= 1)		//ID가 1이하면
				{
					elec_no = pRx->buf[COMMAX_F_BYTE_01];
					COMMAX_Control(pRx, COMMAX_ELEC_ITEM_Sequence[elec_no]);
					COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
				}
				else					//ID가 1보다 클 때
				{
					elec_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_Elec_ID() + 1);		//ex) id = 5, light_cnt = 4, byte_01 = 5, 6, 7, 8 즉, byte_01 - id + 1
																								//ex) id = 7, light_cnt = 4, byte_01 = 7, 8, 9, 10 즉, byte_01 - id +1
					COMMAX_Control(pRx, COMMAX_ELEC_ITEM_Sequence[elec_no]);
					COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
				}
			}
			break;		
		case COMMAX_LIGHT_COMM_GROUP_CONTROL:
			if((pRx->buf[COMMAX_F_BYTE_01] | pRx->buf[COMMAX_F_BYTE_02] | pRx->buf[COMMAX_F_BYTE_03] | pRx->buf[COMMAX_F_BYTE_04] | pRx->buf[COMMAX_F_BYTE_05] | pRx->buf[COMMAX_F_BYTE_06]) == 0)
			{
				if(Batch_Toggle_Flag)
				{
					for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)	//221114
					{
						Store_Light_State[i] = COMMAX_Batch_Light_State(COMMAX_LIGHT_ITEM_Sequence[i]);	//현재 전등 스위치 상태 저장. 제어는 하지 않음. 제어는 월패드에서 그룹 제어로 함.
						printf("%d ", (uint16_t)Store_Light_State[i]);
					}
					Batch_Toggle_Flag = 0;
				}
			}
			COMMAX_Light_GROUP_Control(pRx);		//ACK 없음
			break;
		case COMMAX_ELEC_COMM_GROUP_CONTROL:
			COMMAX_Elec_GROUP_Control(pRx);
			break;
		case COMMAX_BATCH_BLOCK_COMM_REQUEST_RES:					//(0xA0)일괄 스위치 상태 요청에 대한 응답. 응답에서 일괄 소등 상태를 파악.
			if(Batch_Old_Flag == COMMAX_OFF_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_ON_FLAG)
			{
				for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
				{
					COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);
					/*if(Store_Light_State[i] == COMMAX_ON_FLAG)
					{
						COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], COMMAX_ON_FLAG);			//소등 이전 상태로 전등ON + 소등 후 개별 제어로 ON한 전등은 유지
					}*/
				}
			}
			else if(Batch_Old_Flag == COMMAX_ON_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_OFF_FLAG)
			{
				Gu8_Batch_Toggle_Tmr = 5;
				Batch_Toggle_Flag = 1;
			}

			if(Batch_Toggle_Flag)	//플래그 있을 때, 타이머가 0이되면 플래그 0으로,(일괄 소등해제 -> 일괄 소등 되었을 때 플래그 세워짐)
			{
				if(Gu8_Batch_Toggle_Tmr == 0)		//해당 플래그는 일괄 소등 받아 그룹제어로 전등 상태 저장 시 사용함. 타이머는 일정 시간 지났을 경우
				{									//전등 상태 저장 하지 않도록 하기 위해서
					Batch_Toggle_Flag = 0;
				}
			}
			Batch_Old_Flag = pRx->buf[COMMAX_F_BYTE_01];
			break;
		case COMMAX_BATCH_BLOCK_COMM_CONTROL_RES:				//월패드에서 일괄 제어 할때 일괄 스위치의 응답.
			if(Batch_Old_Flag == COMMAX_OFF_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_ON_FLAG)
			{
				for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
				{
					COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);
					/*if(Store_Light_State[i] == COMMAX_ON_FLAG)
					{
						COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], COMMAX_ON_FLAG);			//소등 이전 상태로 전등ON + 소등 후 개별 제어로 ON한 전등은 유지
					}*/
				}
			}
			else if(Batch_Old_Flag == COMMAX_ON_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_OFF_FLAG)
			{
				Gu8_Batch_Toggle_Tmr = 5;
				Batch_Toggle_Flag = 1;
			}			
			Batch_Old_Flag = pRx->buf[COMMAX_F_BYTE_01];
			break;
		case NIS_LIGHT_ID_COMM_1:				//생산시 프로토콜 타입과 아이디를 조회하기 위해서 추가함.
			if(pRx->buf[1] == NIS_LIGHT_ID_COMM_2)
			{
				//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
				/*if(pRx->buf[2] != pG_Config->RS485_ID || pRx->buf[3] != pG_Config->RS485_Elec_ID)
				{
					if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
					{
						pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
						printf("Switch ID Change\r\n");
					}
					if(pRx->buf[3] != pG_Config->RS485_Elec_ID)
					{
						pG_Config->RS485_Elec_ID = pRx->buf[3];
						printf("Elec ID Change\r\n");
					}
					Store_CurrentConfig();
				}*/
				RS_485_ID_RES();
			}
			break;
	}
}
#endif	// defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1개용 - 일괄차단스위치

uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}

uint8_t Get_485_Elec_ID(void)
{
	return (uint8_t)(pG_Config->RS485_Elec_ID);
}

uint8_t Feature_Request(void)
{
	static uint8_t old_toggle_flag = 0;
	uint8_t feature_req = 0;
	
	if(item2tsn(mapping_ITEM_GAS))
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)		feature_req |= (1 << 0);	//bit 0
		else																feature_req |= (0 << 0);
	}
	else
	{
		feature_req |= (0 << 0);
	}

	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	feature_req |= (1 << 3);	//bit 3
		else																feature_req |= (0 << 3);
	}
	else
	{
		feature_req |= (0 << 3);
	}

	if(item2tsn(mapping_ITEM_3WAY_1))
	{
		if(old_toggle_flag != Gu8_ThreeWay_Toggle_Flag[0])					feature_req |= (1 << 4);	//bit 4, 3로는 토글 플래그 상태가 변화하면 요청.
		else																feature_req |= (1 << 4);
		old_toggle_flag = Gu8_ThreeWay_Toggle_Flag[0];		
	}
	else
	{
		feature_req |= (0 << 4);
	}

	return feature_req;
}

uint8_t Feature_Presence(void)
{
	uint8_t feature_pre = 0;

	if(item2tsn(mapping_ITEM_GAS))							feature_pre |= (1 << 0);
	else													feature_pre |= (0 << 0);

	if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF))				feature_pre |= (1 << 2);
	else													feature_pre |= (0 << 2);

	if(item2tsn(mapping_ITEM_ELEVATOR))						feature_pre |= (1 << 4);
	else													feature_pre |= (0 << 4);

	if(item2tsn(mapping_ITEM_3WAY_1))						feature_pre |= (1 << 5);
	else													feature_pre |= (0 << 5);

	feature_pre |= (0 << 7);	//protocol V2.86이상 연동되는 메인스위치 중 엘리베이터 층 정보 표시 기능 지원하는 경우 1로 설정, 지원 하지 않는 경우 0으로 설정.
								//전자식 스위치는 층 정보 표시 기능 지원하지 않으므로 0으로 설정.
	return feature_pre;
}

void COMMAX_BATCH_BLOCK_GROUP_Control(COMMAX_BUF *pRx)
{
	uint8_t Sub_No, cnt = 0, i = 0;
	for(i = 0; i < 8; i++)
	{
		Sub_No = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] >> i);
		cnt++;
		if((Sub_No & 0x01) == 0x01)
		{	
			if(Get_485_ID() == cnt)	BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);
		}
		else if((Sub_No & 0x01) == 0x00)
		{
			if(Get_485_ID() == cnt)	BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
		}
		printf("ID = %d, cnt = %d\r\n", (uint16_t)Get_485_ID(), (uint16_t)cnt);
	}
}

void COMMAX_BATCH_BLOCK_Data_Res(COMMAX_BUF *pRx)
{
	uint8_t	item, touch_switch;
	COMMAX_BUF	*pTx;
	pTx = &TxBuf;

	pTx->count	= COMMAX_F_COMMAND;
	pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[COMMAX_F_COMMAND] + 0x80);		//ACK의 Command는 요구 Command + 0x80 이다.

	if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	pTx->buf[pTx->count++]	= COMMAX_ON_FLAG;			//Byte01은 일괄소등 상태
	else															pTx->buf[pTx->count++]	= COMMAX_OFF_FLAG;
	
	pTx->buf[pTx->count++]	= pRx->buf[COMMAX_F_BYTE_01];					//Byte02 Ack Sub번호
	
	if(pRx->buf[COMMAX_F_COMMAND] == COMMAX_BATCH_BLOCK_COMM_REQUEST)		//요구 Command가 상태요구(0x20)면
	{	
		pTx->buf[pTx->count++]	= 0;										//Byte03 대기전력 On/Off 상태(일괄 스위치에 기능 없음)
		pTx->buf[pTx->count++]	= Feature_Request();						//Byte04 기능요구(B0 : 가스밸브, B2 : 엘리베이터 Up 요구, B3 : 엘리베이터 Down 요구, B4 : 삼로 스위치 상태변경 요구)
		pTx->buf[pTx->count++]	= Feature_Presence();						//Byte05 기능유무(B0 : 가스닫힘 기능, B2 : 일괄소등 기능, B4 : 엘리베이터 기능, B5 : 삼로 기능)
		pTx->buf[pTx->count++]	= 0;										//Byte06 기능요구2(일괄 스위치에 기능 없음)
	}
	else																	//요구 Command가 그 외면
	{
		pTx->buf[pTx->count++] = 0;											//Byte03 X
		pTx->buf[pTx->count++] = 0;											//Byte04 X
		pTx->buf[pTx->count++] = 0;											//Byte05 X
		pTx->buf[pTx->count++] = 0;											//Byte06 X
	}
	pTx->buf[pTx->count++]	= COMMAX_Crc(pTx);							//Byte07 Check Sum

	TxBuf.send_flag	= 1;
}

void COMMAX_Data_Process(COMMAX_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		LED_State, item, touch_switch = 0;
    
	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	if(pRx->buf[COMMAX_F_COMMAND] == COMMAX_BATCH_BLOCK_COMM_GROUP_CONTROL)
	{
		COMMAX_BATCH_BLOCK_GROUP_Control(pRx);
	}
	else
	{
		if(pRx->buf[COMMAX_F_BYTE_01] == Get_485_ID())
		{
			switch(pRx->buf[COMMAX_F_COMMAND])
			{
				case COMMAX_BATCH_BLOCK_COMM_REQUEST:
					LED_State = pRx->buf[COMMAX_F_BYTE_02];
					if(item2tsn(mapping_ITEM_GAS))
					{
						if(LED_State & COMMAX_GAS_STATE)						//bit0 가스 밸브 상태 OPEN
						{
							touch_switch = item2tsn(mapping_ITEM_GAS);

							if(GET_LED_State(touch_switch) == LED_FLASHING)		//가스 차단 요청중일때는 0x22에서 데이터 받아서 제어.
							{
								// BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//가스 차단
							}
							else if(GET_Switch_State(touch_switch))				//현재 가스 차단중이면
							{
								BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);		//가스 차단해제
							}
						}
						else if((LED_State & COMMAX_GAS_STATE) == 0)			//월패드에서 가스밸브 상태 CLOSE로 오면
						{
							touch_switch = item2tsn(mapping_ITEM_GAS);
							if(GET_LED_State(touch_switch) == LED_FLASHING)
							{
								;
							}
							else if(GET_Switch_State(touch_switch) == 0)				//가스 차단 해제 상태일때, 가스차단 요청중일때
							{
								BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
							}
						}
					}
					if(item2tsn(mapping_ITEM_3WAY_1))
					{
						if(LED_State & COMMAX_3WAY_INSTALL)						//bit3 삼로 스위치 설치
						{
							if(LED_State & COMMAX_3WAY_STATE)					//bit2 삼로 스위치 상태 On
							{
								EventCtrl(item2tsn(mapping_ITEM_3WAY_1), ON);
							}
							else if((LED_State & COMMAX_3WAY_STATE) == 0)								//삼로 스위치 상태 Off
							{
								EventCtrl(item2tsn(mapping_ITEM_3WAY_1), OFF);
							}
						}
					}
					COMMAX_BATCH_BLOCK_Data_Res(pRx);
					break;
				/*case COMMAX_BATCH_LIGHT_COMM_CONTROL:
					if(pRx->buf[COMMAX_F_BYTE_02] == COMMAX_ON_FLAG)			BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);
					else if(pRx->buf[COMMAX_F_BYTE_02] == COMMAX_OFF_FLAG)		BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
					COMMAX_BATCH_BLOCK_Data_Res(pRx);
					break;*/		//Command 21 사용안해도 된다고 함. Command 22 사용.
				case COMMAX_BATCH_BLOCK_COMM_CONTROL:	//삳태요구 응답[A0]의 기능요구에 대해 월패드는 완료/ 실패에 따른 제어의 종류를 구별하기 위한 용도로 사용. 일괄 소등 제외.
					if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_BATCHLIGHT)
					{
						if((pRx->buf[COMMAX_F_BYTE_02] & 0x01) == COMMAX_BATCH_LIGHT_ON)		BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);	//일괄점등
						else if((pRx->buf[COMMAX_F_BYTE_02] & 0x01) == COMMAX_BATCH_LIGHT_OFF)	BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);	//일괄소등
					}
					if(item2tsn(mapping_ITEM_GAS))
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)		//가스 차단 요청중일때
						{
							if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_GAS_CLOSE_SUCCESS)		//가스 닫힘
							{
								if((pRx->buf[COMMAX_F_BYTE_02] & COMMAX_GAS_CLOSE_SUCCESS) == COMMAX_GAS_CLOSE_SUCCESS)	BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);	//가스 차단 성공하면 차단 상태로
							}
							else if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_GAS_CLOSE_FAIL)	//가스 닫힘 불가
							{
								if((pRx->buf[COMMAX_F_BYTE_02] & COMMAX_GAS_CLOSE_FAIL) == COMMAX_GAS_CLOSE_FAIL)		BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);	//가스 차단 실패하면 해제 상태로
							}
						}
					}
					if(item2tsn(mapping_ITEM_ELEVATOR))
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	//엘리베이터 호출 요청중일때
						{
							if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_ELEVATOR_CALL_SUCCESS)
							{
								if((pRx->buf[COMMAX_F_BYTE_02] & COMMAX_ELEVATOR_SUCCESS) == COMMAX_ELEVATOR_SUCCESS)	BATCH_BLOCK_Control(SET__ELEVATOR_CALL);	//엘리베이터 호출 성공하면 호출 상태로
							}
							else if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_ELEVATOR_CALL_FAIL)
							{
								if((pRx->buf[COMMAX_F_BYTE_02] & COMMAX_ELEVATOR_FAIL) == COMMAX_ELEVATOR_FAIL)			BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);	//엘리베이터 호출 실패하면 기본 상태로
							}
						}
					}
						//문의 했을때 사용하지 않는다고 함.......... 다시 확인해야함.
					COMMAX_BATCH_BLOCK_Data_Res(pRx);
					break;
				case COMMAX_ELEVATOR_COMM_CONTROL:
					if(item2tsn(mapping_ITEM_ELEVATOR))
					{
						if(Get_485_ID() == pRx->buf[COMMAX_F_BYTE_01])
						{
							BATCH_BLOCK_Control(SET__ELEVATOR_CALL);
							COMMAX_BATCH_BLOCK_Data_Res(pRx);
						}
					}
					break;		
			}
		}
		else
		{
			if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))     //485통신 테스트를 위해서 추가함 
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
			if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)			//호출 요청중일때 60초 지나면 LED 원래대로
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);
			}
			else if(GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR)))					//호출 성공했을때, 60초 지나면 LED 원래대로
			{
				SET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR), OFF);
				SET_LED_State(item2tsn(mapping_ITEM_ELEVATOR), ON);
				// BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);
			}
		}
	}
}

void BATCH_BLOCK_Control(uint8_t control)
{
	uint8_t	Flag;
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
			}
			else													//가스 차단중이 아닐때
			{
				Gu16_GAS_Off_Tmr	= 60;						// 60초 경과 후 LED 소등(단 월패드에서 close/open 데이터가 수신되면 해당 상태로 전환), 
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, LED_FLASHING);	// LED FLASHING
				Beep(ON);
				// printf("Gas REQUEST\r\n");
			}

			break;
		case SET__GAS_CLOSE_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) == 0)		//차단  상태 아닐 때
			{
				SET_Switch_State(touch_switch, ON);		// 가스밸브 차단(닫힘)
				SET_LED_State(touch_switch, OFF);		// 실제로는 LED 켜짐
				Beep(ON);
			}
			// printf("GAS CLOSE\r\n");
			break;
		case SET__GAS_OPEN_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) || (GET_LED_State(touch_switch) == LED_FLASHING))
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
				// printf("ELEVATOR REQEUST\r\n");
			}
			break;
		case SET__ELEVATOR_CALL:																			//세대기에서 엘리베이터 콜
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == 0)			//호출 요청 상태거나, 호출상태 아닐 때.
			{
				Gu16_Elevator_Tmr = 60;																			//콜 상태가 되면 타이머 60초 초기화. 타이머가 0되면 상태 원래대로 돌아감.
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
					Beep(ON);									//엘리베이터 프로토콜은 연동테스트 되지 않아, 어떻게 동작하는지 모르므로 일단 부저는 삭제함.
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


#endif	// _COMMAX_PROTOCOL_

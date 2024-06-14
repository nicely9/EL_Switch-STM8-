/************************************************************************************
	Project		: ���ڽĽ���ġ
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

/*#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1���� - �ϰ����ܽ���ġ
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

#define	MAX_COMMAX_DATA_SEQUENCE		9		//���� ���� ���� �ִ� 8������ COMMAX�� ��� ���� no�� 1���� �����ϹǷ� 8�̾ƴ϶� �ִ� 9�� ������.
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

	memset(COMMAX_LIGHT_ITEM_Sequence, 0, MAX_COMMAX_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	memset(COMMAX_ELEC_ITEM_Sequence, 0, MAX_COMMAX_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	
	// �������� ������ �׸� ����
	// �����	 �ִ��׸�	: ���� 6��, ���� 4�� + ��� 2��
	// ����+��� �ִ��׸�	: ���� 4�� + ��� 2��, ����2�� + ���2�� + ���2��
	// ex) ���� 3��, ��� 2�� = ����1,����2,����3,���1,���2,0,0,0
	// ex) ���� 1��, ��� 1�� = ����1,���1,0,0,0,0,0,0
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// �������� ���� ����, ������� ������
	count	= 1;		//COMMAX�� ���� 1�� ���� ����ϹǷ� ���ǻ� 1���� ����
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
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// �������� ���� �������, ���� ������
	count	= 1;		//COMMAX�� ���� 1�� ���� ����ϹǷ� ���ǻ� 1���� ����
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
	
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// ������� �� 
	count	= 1;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_COMMAX_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_COMMAX_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
#endif

	for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
	{
		// Store_Light_State[i]	= COMMAX_Batch_Light_State(COMMAX_LIGHT_ITEM_Sequence[i]);
		Store_Light_State[i]	= COMMAX_ON_FLAG;
	}
	// Gu8_Batch_Light_Flag = COMMAX_UNKNOWN_FLAG;	//ON FLAG �� ��, �ϰ� ����ġ�� �Բ� ������� �ʴ� ��쿡 Batch_Light_Process�� ���� ���� ����ǹǷ� UNKNOWN FLAG�� ��.
}
// ----------------------------------------------------------------------------------------
void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr)
{
	if(tmr == 0)		// ������ ������ ���� �� X ms �ʰ��ϸ� ������ Ŭ����
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
	
	/*if(Gu8_RS_485_Tx_Tmr == 0)		// ������ ���� �� 10ms ���� ������ ����
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
				Gu8_RS_485_Tx_Tmr	= 2;		// �� 2ms ���Ŀ� USART_IT_TC üũ
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
uint8_t NIS_Crc(COMMAX_BUF *pTRx, uint8_t cal, uint8_t sel)		//����� 485��� �׽�Ʈ�� ���ؼ� �߰���.
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
uint8_t Light_Cnt(void)		//���� �� ī��Ʈ
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

uint8_t Elec_Cnt(void)		//���� �� ī��Ʈ
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
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//�������� Ÿ��
	pTx->buf[pTx->count++]	= Get_485_ID();					//ID ��ȣ
	pTx->buf[pTx->count++]	= Get_485_Elec_ID();

	pTx->buf[pTx->count++]	= Elec_Watt_MSB;				//����1, 2�� ���� ��(����)
	pTx->buf[pTx->count++]	= Elec_Watt_LSB;				//����1, 2�� ���� ��(�Ҽ�)
	
	pTx->buf[pTx->count++]	= Gu8_ZeroCrossing_Err_Flag;	//����ũ�ν� ���� ���� 1�̸� Err, 0�̸� Pass
	
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
		if(pRx->buf[COMMAX_F_COMMAND] == NIS_LIGHT_ID_COMM_1)		//485 ��� �׽�Ʈ�� ���ؼ� �߰���
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
			if(pG_State->Dimming_Level.Dimming1	!= (uint8_t)Dimming_Level)	Beep(ON);				// ���е忡�� ���������� ���������� ���� �ƴ��� ������ ������
			pG_State->Dimming_Level.Dimming1		= (uint8_t)Dimming_Level;
			if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:			
			if(pG_State->Dimming_Level.Dimming2	!= (uint8_t)Dimming_Level)	Beep(ON);				// ���е忡�� ���������� ���������� ���� �ƴ��� ������ ������
			pG_State->Dimming_Level.Dimming2		= (uint8_t)Dimming_Level;
			if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
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
			if(pG_State->Color_Temp_Level.Color_Temp1	!= (uint8_t)Color_Level)	Beep(ON);				// ���е忡�� ���������� ���������� ���� �ƴ��� ������ ������
			pG_State->Color_Temp_Level.Color_Temp1	= Color_Level;
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp1 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// �ִ� ũ��� ����
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(pG_State->Color_Temp_Level.Color_Temp2	!= (uint8_t)Color_Level)	Beep(ON);				// ���е忡�� ���������� ���������� ���� �ƴ��� ������ ������
			pG_State->Color_Temp_Level.Color_Temp2	= Color_Level;
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp2 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// �ִ� ũ��� ����
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

uint8_t COMMAX_Batch_Light_State(uint8_t item)		//�ϰ� �ҵ� �� ���� ���� ���� ����.
{
	uint8_t ret = 0;
	switch(item)
	{
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
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
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(control_value == COMMAX_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == COMMAX_ON_FLAG)
				{
					EventCtrl(item2tsn(item), OFF);						//���� OFF
				}
			}
			else if(control_value == COMMAX_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == COMMAX_OFF_FLAG)
				{
					EventCtrl(item2tsn(item), ON);						//���� ON
				}
			}
			break;
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// ����
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
	
	switch(item)	// ������ ������ ������
	{
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
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
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
			if((control & COMMAX_ON_FLAG) == COMMAX_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)
				{
					EventCtrl(item2tsn(item), ON);
				}
				else		//���е忡�� ��� ����ÿ��� byte_02�� ���� 0x01(ON/OFF)�� �ͼ� �߰���.
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
					Gu8_LCD_DIM_Tmr					= 20;		// 2s ���� LCD ǥ��					
					SET_DimmingLevel(item, Dimming_Level);			//���е忡�� ��� ���� ����� 0x02���� �����ʰ� 0x01�� ��....
					SET_DimmingColorLevel(item, Color_Level);		//���е忡�� ���µ� ���� ����� 0x04���� �����ʰ� 0x01�� ��....
				}
				if((control & COMMAX_DIMMING) == COMMAX_DIMMING)
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
					Gu8_LCD_DIM_Tmr					= 20;		// 2s ���� LCD ǥ��
					SET_DimmingLevel(item, Dimming_Level);
				}
				if((control & COMMAX_COLOR_TEMP) == COMMAX_COLOR_TEMP)
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
					Gu8_LCD_DIM_Tmr					= 20;		// 2s ���� LCD ǥ��					
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
						PUT_RelayCtrl(item2ctrl(item), ON);	// �׸���� ����
						SET_SWITCH_Delay_OFF_Flag(item, 0);
						SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
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
						PUT_RelayCtrl(item2ctrl(item), OFF);	// �׸���� ����
						SET_SWITCH_Delay_OFF_Flag(item, 0);
						SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
						ALL_Electricity_Switch_LED_Ctrl();						
						Beep(OFF);
					}
				}

			}
			else if(control == COMMAX_BLOCK_MODE)
			{
				if(pRx->buf[COMMAX_F_BYTE_03])		//���� ��� �ڵ�
				{
					SET_Elec_Auto_Manual(item, COMMAX_AUTO_FLAG);
				}
				else								//���� ��� ����
				{
					SET_Elec_Auto_Manual(item, COMMAX_MANUAL_FLAG);
				}

			}
			else if(control == COMMAX_STANDBY_VALUE_SAVE)		//������� �� ����
			{													//������� �� ���� ��� ���� byte 03, byte 04���� ������� ���� �����ؾ� �Ѵ�.
				ElecLimit_Save(item, pRx);
			}

			break;
#endif
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// ����
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
									SET_DimmingLevel(item, dim_level);		//Byte_06�� 0�̸� ���� ��ַ��� ����, 1 ~ 8 �� ��ַ����� On�Ѵ�.
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
					k = 1;		//k���� �ش� ����ġ�� ������ �´� cnt ���� �Ǿ����� �����ϰ�, ������ ���� �� ��� 1�� �ʱ�ȭ ��.
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
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			if(ack == COMMAX_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = COMMAX_ON_FLAG;
				else											ret = COMMAX_OFF_FLAG;
			}
			else												ret = COMMAX_OFF_FLAG;
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			if(ack == COMMAX_DIMMING)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = pG_State->Dimming_Level.Dimming1;
				else											ret = COMMAX_OFF_FLAG;
			}
			else if(ack == COMMAX_COLOR_TEMP)
			{
				if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)		//���µ� ���� ������� �ʴ� �������� ���µ� ���� �������� �ʱ� ���ؼ� �߰���
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
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
			if(ack == COMMAX_DIMMING)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))		ret = pG_State->Dimming_Level.Dimming2;
				else												ret = COMMAX_OFF_FLAG;
			}
			else if(ack == COMMAX_COLOR_TEMP)
			{
				if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)		//���µ� ���� ������� �ʴ� �������� ���µ� ���� �������� �ʱ� ���ؼ� �߰���
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
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit4 �ڵ�/����, bit0 ���� ON/OFF
			{
				ret	= (uint8_t)(COMMAX_ON_FLAG | (pG_State->ETC.Auto1 << 4));
			}
			else
			{
				ret	= (uint8_t)(COMMAX_OFF_FLAG | (pG_State->ETC.Auto1 << 4));
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit4 �ڵ�/����, bit0 ���� ON/OFF
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

/*void Batch_Light_Process(void)						//�ϰ����� ���¿��� ���� ���� ���¸� �����Ͽ�, �ϰ��ҵ�->�ϰ����� �� ����� ���·� ����.
{
	uint8_t i = 1;
	static uint8_t cnt = 0;
	
	if(Gu8_Batch_Light_Flag != COMMAX_ON_FLAG)	return;

	if(cnt == 0)
	{
		if(Gu8_Batch_Light_Flag == COMMAX_ON_FLAG)		//�ϰ� ���� ���¸� ���� ���� ���� ����.
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
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)		// 1����, 2����	- �����
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}

uint8_t Get_485_Elec_ID(void)
{
	return (uint8_t)(pG_Config->RS485_Elec_ID);
}

void COMMAX_LIGHT_COMM_Request_Res(COMMAX_BUF *pRx)				//�������� ���� �䱸�� ���� ����
{
	uint8_t	light_no, item = 0;
	COMMAX_BUF	*pTx;
	pTx = &TxBuf;
	
	if(Get_485_ID() <= 1)	light_no = pRx->buf[COMMAX_F_BYTE_01];
	else					light_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_ID() + 1);

	pTx->count	= COMMAX_F_COMMAND;

	pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[COMMAX_F_COMMAND] + 0x80);		//ACK�� Command�� �䱸 Command + 0x80 �̴�.
	pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(COMMAX_LIGHT_ITEM_Sequence[light_no], COMMAX_ON_n_OFF);		//ON/OFF  ����
	pTx->buf[pTx->count++]	= pRx->buf[COMMAX_F_BYTE_01];																//ACK �����ȣ
	pTx->buf[pTx->count++]	= 0x00;
	pTx->buf[pTx->count++]	= 0x00;																						//����� ������� �����Ƿ� ������ܰ� ����
	pTx->buf[pTx->count++]	= 0x00;																						//��� ������� �����Ƿ� ���� ��ִܰ� ����
	pTx->buf[pTx->count++]	= 0x00;																						//��� ������� �����Ƿ� ��ִܰ� �ִ� �� ����
	/*	����𵨿� ���, ���µ� �ɼ� �߰��Ǹ� ���
	pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(COMMAX_LIGHT_ITEM_Sequence[light_no], COMMAX_COLOR_TEMP);	//������ܰ�		������� ���� ���µ� �ɼ� ����
	pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(COMMAX_LIGHT_ITEM_Sequence[light_no], COMMAX_DIMMING);		//������ ��ִܰ�	������� ���� ��� �ɼ� ����
	pTx->buf[pTx->count++]	= pG_Config->Dimming_MAX_Level;																//��ִܰ� �ִ밪	������� ���� ��� �ɼ� ����
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
			if((Get_485_ID() + Light_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])		//Sub no�� �� ���̵� + ���� �� ���� ������, ex) id = 1, light_cnt = 4, id + light = 5, 5 > sub no, ���� ����ġ id�� 5���� ����
			{
				COMMAX_LIGHT_COMM_Request_Res(pRx);
			}
			break;
		case COMMAX_LIGHT_COMM_CONTROL:
			if((Get_485_ID() + Light_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])
			{
				if(Get_485_ID() <= 1)		//ID�� 1���ϸ�, �ڸƽ����� 0�� ���������� id�� 0�̸� ������ �޶����� ����ó�� �ʿ�������
				{
					light_no = pRx->buf[COMMAX_F_BYTE_01];
					COMMAX_Control(pRx, COMMAX_LIGHT_ITEM_Sequence[light_no]);
					COMMAX_LIGHT_COMM_Request_Res(pRx);
				}
				else					//ID�� 1���� Ŭ ��
				{
					light_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_ID() + 1);		//ex) id = 5, light_cnt = 4, byte_01 = 5, 6, 7, 8 ��, byte_01 - id + 1
																								//ex) id = 7, light_cnt = 4, byte_01 = 7, 8, 9, 10 ��, byte_01 - id +1
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
						Store_Light_State[i] = COMMAX_Batch_Light_State(COMMAX_LIGHT_ITEM_Sequence[i]);	//���� ���� ����ġ ���� ����. ����� ���� ����. ����� ���е忡�� �׷� ����� ��.
						printf("%d ", (uint16_t)Store_Light_State[i]);
					}
					Batch_Toggle_Flag = 0;
				}
			}
			COMMAX_Light_GROUP_Control(pRx);		//ACK ����
			break;
		case COMMAX_BATCH_BLOCK_COMM_REQUEST_RES:					//(0xA0)�ϰ� ����ġ ���� ��û�� ���� ����. ���信�� �ϰ� �ҵ� ���¸� �ľ�.
			if(Batch_Old_Flag == COMMAX_OFF_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_ON_FLAG)
			{
				for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
				{
					// COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);	//�ҵ� �� ���� �������� ��, ���� �� ������ ��� �ҵ� �� ���·� ����
					if(Store_Light_State[i] == COMMAX_ON_FLAG)		//ON�� �ϰԵǸ� �ϰ� �ҵ� ���� ���� ���� ���� ��, ���� �� ���� �� ������ �״�� ���� ���·� ���� ��� ������
					{
						COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], COMMAX_ON_FLAG);			//�ҵ� ���� ���·� ����ON + �ҵ� �� ���� ����� ON�� ������ ����
					}
				}
			}
			else if(Batch_Old_Flag == COMMAX_ON_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_OFF_FLAG)
			{
				Gu8_Batch_Toggle_Tmr = 5;
				Batch_Toggle_Flag = 1;
			}

			if(Batch_Toggle_Flag)	//�÷��� ���� ��, Ÿ�̸Ӱ� 0�̵Ǹ� �÷��� 0����,(�ϰ� �ҵ����� -> �ϰ� �ҵ� �Ǿ��� �� �÷��� ������)
			{
				if(Gu8_Batch_Toggle_Tmr == 0)		//�ش� �÷��״� �ϰ� �ҵ� �޾� �׷������ ���� ���� ���� �� �����. Ÿ�̸Ӵ� ���� �ð� ������ ���
				{									//���� ���� ���� ���� �ʵ��� �ϱ� ���ؼ�
					Batch_Toggle_Flag = 0;
				}
			}
			Batch_Old_Flag = pRx->buf[COMMAX_F_BYTE_01];
			break;
		case COMMAX_BATCH_BLOCK_COMM_CONTROL_RES:				//���е忡�� �ϰ� ���� �Ҷ� �ϰ� ����ġ�� ����.
			if(Batch_Old_Flag == COMMAX_OFF_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_ON_FLAG)
			{
				for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
				{
					COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);
					/*if(Store_Light_State[i] == COMMAX_ON_FLAG)
					{
						COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], COMMAX_ON_FLAG);			//�ҵ� ���� ���·� ����ON + �ҵ� �� ���� ����� ON�� ������ ����
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
		case NIS_LIGHT_ID_COMM_1:				//����� �������� Ÿ�԰� ���̵� ��ȸ�ϱ� ���ؼ� �߰���.
			if(pRx->buf[1] == NIS_LIGHT_ID_COMM_2)
			{
				//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.
				/*if(pRx->buf[2] != pG_Config->RS485_ID || pRx->buf[3] != pG_Config->RS485_Elec_ID)
				{
					if(pRx->buf[2] != pG_Config->RS485_ID)			//���α׷����� ������ ����ġ ID�� ����ġ�� ID�� �ٸ���
					{
						pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
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
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)	// 2����	- ����+������
uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}

uint8_t Get_485_Elec_ID(void)
{
	return (uint8_t)(pG_Config->RS485_Elec_ID);
}

void COMMAX_Light_n_ELEC_COMM_Request_Res(COMMAX_BUF *pRx)				//�������� ���� �䱸�� ���� ����
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

			pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[COMMAX_F_COMMAND] + 0x80);				//ACK�� Command�� �䱸 Command + 0x80 �̴�.
			pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(item, COMMAX_ON_n_OFF);		//ON/OFF  ����
			pTx->buf[pTx->count++]	= pRx->buf[COMMAX_F_BYTE_01];								//ACK �����ȣ
			pTx->buf[pTx->count++]	= 0x00;
			pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(item, COMMAX_COLOR_TEMP);	//������ܰ�
			pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(item, COMMAX_DIMMING);		//������ ��ִܰ�
			pTx->buf[pTx->count++]	= COMMAX_LIGHT_Data_Conversion(item, COMMAX_MAX_DIM_LEVEL);	//��ִܰ� �ִ밪
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
			pTx->buf[pTx->count++]	= COMMAX_ELEC_Data_Conversion(item);							//ON/OFF  | �ڵ�/����
			pTx->buf[pTx->count++]	= pRx->buf[COMMAX_F_BYTE_01];									//ACK ���������ȣ
			pTx->buf[pTx->count++]	= (uint8_t)((pRx->buf[COMMAX_F_BYTE_02] << 4) | 0);				//Type | �Ҽ��� �ڸ���(���� ����ġ�� ���°��� �Ҽ��� ǥ���� �����Ƿ� �Ҽ��� �ڸ� �������� ǥ��.)
			
			if(pRx->buf[COMMAX_F_COMMAND] == COMMAX_ELEC_COMM_REQUEST)
			{
				switch(pRx->buf[COMMAX_F_BYTE_02])
				{
					case TYPE_USAGE:		//��뷮(����ġ)
						if(item == mapping_ITEM_ELECTRICITY_1)		BCD_CODE(Gu16_LCD_Watt_1, &Watt_MSB, &Watt_LSB);
						else if(item == mapping_ITEM_ELECTRICITY_2) BCD_CODE(Gu16_LCD_Watt_2, &Watt_MSB, &Watt_LSB);

						pTx->buf[pTx->count++]	= 0;				//byte4
						pTx->buf[pTx->count++]	= Watt_MSB;			//byte5
						pTx->buf[pTx->count++]	= Watt_LSB;			//byte6
						break;
					case TYPE_STANDBY_VALUE_SAVE:		//������� �� ����??
						if(item == mapping_ITEM_ELECTRICITY_1)		BCD_CODE(Gu16_ElecLimitCurrent_1, &Watt_MSB, &Watt_LSB);
						else if(item == mapping_ITEM_ELECTRICITY_2)	BCD_CODE(Gu16_ElecLimitCurrent_2, &Watt_MSB, &Watt_LSB);
						pTx->buf[pTx->count++]	= 0;				//byte4
						pTx->buf[pTx->count++]	= Watt_MSB;			//byte5
						pTx->buf[pTx->count++]	= Watt_LSB;			//byte6
						break;
					case TYPE_USAGE_ONE_HOURS:		//��뷮(���û���, 1�ð� ����ġ)
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
					case COMMAX_ON_n_OFF:		//��뷮(����ġ)
					case COMMAX_BLOCK_MODE:
						if(item == mapping_ITEM_ELECTRICITY_1)		BCD_CODE(Gu16_LCD_Watt_1, &Watt_MSB, &Watt_LSB);
						else if(item == mapping_ITEM_ELECTRICITY_2) BCD_CODE(Gu16_LCD_Watt_2, &Watt_MSB, &Watt_LSB);
						pTx->buf[pTx->count++]	= 0;				//byte4
						pTx->buf[pTx->count++]	= Watt_MSB;			//byte5
						pTx->buf[pTx->count++]	= Watt_LSB;			//byte6
						break;
					case COMMAX_STANDBY_VALUE_SAVE:		//������� �� ����
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
			if((Get_485_ID() + Light_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])		//Sub no�� �� ���̵� + ���� �� ���� ������, ex) id = 1, light_cnt = 4, id + light = 5, 5 > sub no, ���� ����ġ id�� 5���� ����
			{
				COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
			}
			break;
		case COMMAX_ELEC_COMM_REQUEST:
			if((Get_485_Elec_ID() + Elec_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])		//Sub no�� �� ���̵� + ���� �� ���� ������, ex) id = 1, light_cnt = 4, id + light = 5, 5 > sub no, ���� ����ġ id�� 5���� ����
			{
				COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
			}
			break;
		case COMMAX_LIGHT_COMM_CONTROL:
			if((Get_485_ID() + Light_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])
			{
				if(Get_485_ID() <= 1)		//ID�� 1���ϸ�
				{
					light_no = pRx->buf[COMMAX_F_BYTE_01];
					COMMAX_Control(pRx, COMMAX_LIGHT_ITEM_Sequence[light_no]);
					COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
				}
				else					//ID�� 1���� Ŭ ��
				{
					light_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_ID() + 1);		//ex) id = 5, light_cnt = 4, byte_01 = 5, 6, 7, 8 ��, byte_01 - id + 1
																								//ex) id = 7, light_cnt = 4, byte_01 = 7, 8, 9, 10 ��, byte_01 - id +1
					COMMAX_Control(pRx, COMMAX_LIGHT_ITEM_Sequence[light_no]);
					COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
				}
			}
			break;
		case COMMAX_ELEC_COMM_CONTROL:
			if((Get_485_Elec_ID() + Elec_Cnt()) > pRx->buf[COMMAX_F_BYTE_01])
			{
				if(Get_485_Elec_ID() <= 1)		//ID�� 1���ϸ�
				{
					elec_no = pRx->buf[COMMAX_F_BYTE_01];
					COMMAX_Control(pRx, COMMAX_ELEC_ITEM_Sequence[elec_no]);
					COMMAX_Light_n_ELEC_COMM_Request_Res(pRx);
				}
				else					//ID�� 1���� Ŭ ��
				{
					elec_no = (uint8_t)(pRx->buf[COMMAX_F_BYTE_01] - Get_485_Elec_ID() + 1);		//ex) id = 5, light_cnt = 4, byte_01 = 5, 6, 7, 8 ��, byte_01 - id + 1
																								//ex) id = 7, light_cnt = 4, byte_01 = 7, 8, 9, 10 ��, byte_01 - id +1
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
						Store_Light_State[i] = COMMAX_Batch_Light_State(COMMAX_LIGHT_ITEM_Sequence[i]);	//���� ���� ����ġ ���� ����. ����� ���� ����. ����� ���е忡�� �׷� ����� ��.
						printf("%d ", (uint16_t)Store_Light_State[i]);
					}
					Batch_Toggle_Flag = 0;
				}
			}
			COMMAX_Light_GROUP_Control(pRx);		//ACK ����
			break;
		case COMMAX_ELEC_COMM_GROUP_CONTROL:
			COMMAX_Elec_GROUP_Control(pRx);
			break;
		case COMMAX_BATCH_BLOCK_COMM_REQUEST_RES:					//(0xA0)�ϰ� ����ġ ���� ��û�� ���� ����. ���信�� �ϰ� �ҵ� ���¸� �ľ�.
			if(Batch_Old_Flag == COMMAX_OFF_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_ON_FLAG)
			{
				for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
				{
					COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);
					/*if(Store_Light_State[i] == COMMAX_ON_FLAG)
					{
						COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], COMMAX_ON_FLAG);			//�ҵ� ���� ���·� ����ON + �ҵ� �� ���� ����� ON�� ������ ����
					}*/
				}
			}
			else if(Batch_Old_Flag == COMMAX_ON_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_OFF_FLAG)
			{
				Gu8_Batch_Toggle_Tmr = 5;
				Batch_Toggle_Flag = 1;
			}

			if(Batch_Toggle_Flag)	//�÷��� ���� ��, Ÿ�̸Ӱ� 0�̵Ǹ� �÷��� 0����,(�ϰ� �ҵ����� -> �ϰ� �ҵ� �Ǿ��� �� �÷��� ������)
			{
				if(Gu8_Batch_Toggle_Tmr == 0)		//�ش� �÷��״� �ϰ� �ҵ� �޾� �׷������ ���� ���� ���� �� �����. Ÿ�̸Ӵ� ���� �ð� ������ ���
				{									//���� ���� ���� ���� �ʵ��� �ϱ� ���ؼ�
					Batch_Toggle_Flag = 0;
				}
			}
			Batch_Old_Flag = pRx->buf[COMMAX_F_BYTE_01];
			break;
		case COMMAX_BATCH_BLOCK_COMM_CONTROL_RES:				//���е忡�� �ϰ� ���� �Ҷ� �ϰ� ����ġ�� ����.
			if(Batch_Old_Flag == COMMAX_OFF_FLAG && pRx->buf[COMMAX_F_BYTE_01] == COMMAX_ON_FLAG)
			{
				for(i = 1; i < MAX_COMMAX_DATA_SEQUENCE; i++)
				{
					COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);
					/*if(Store_Light_State[i] == COMMAX_ON_FLAG)
					{
						COMMAX_BatchLight_Control(COMMAX_LIGHT_ITEM_Sequence[i], COMMAX_ON_FLAG);			//�ҵ� ���� ���·� ����ON + �ҵ� �� ���� ����� ON�� ������ ����
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
		case NIS_LIGHT_ID_COMM_1:				//����� �������� Ÿ�԰� ���̵� ��ȸ�ϱ� ���ؼ� �߰���.
			if(pRx->buf[1] == NIS_LIGHT_ID_COMM_2)
			{
				//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.
				/*if(pRx->buf[2] != pG_Config->RS485_ID || pRx->buf[3] != pG_Config->RS485_Elec_ID)
				{
					if(pRx->buf[2] != pG_Config->RS485_ID)			//���α׷����� ������ ����ġ ID�� ����ġ�� ID�� �ٸ���
					{
						pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
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
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1���� - �ϰ����ܽ���ġ

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
		if(old_toggle_flag != Gu8_ThreeWay_Toggle_Flag[0])					feature_req |= (1 << 4);	//bit 4, 3�δ� ��� �÷��� ���°� ��ȭ�ϸ� ��û.
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

	feature_pre |= (0 << 7);	//protocol V2.86�̻� �����Ǵ� ���ν���ġ �� ���������� �� ���� ǥ�� ��� �����ϴ� ��� 1�� ����, ���� ���� �ʴ� ��� 0���� ����.
								//���ڽ� ����ġ�� �� ���� ǥ�� ��� �������� �����Ƿ� 0���� ����.
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
	pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[COMMAX_F_COMMAND] + 0x80);		//ACK�� Command�� �䱸 Command + 0x80 �̴�.

	if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	pTx->buf[pTx->count++]	= COMMAX_ON_FLAG;			//Byte01�� �ϰ��ҵ� ����
	else															pTx->buf[pTx->count++]	= COMMAX_OFF_FLAG;
	
	pTx->buf[pTx->count++]	= pRx->buf[COMMAX_F_BYTE_01];					//Byte02 Ack Sub��ȣ
	
	if(pRx->buf[COMMAX_F_COMMAND] == COMMAX_BATCH_BLOCK_COMM_REQUEST)		//�䱸 Command�� ���¿䱸(0x20)��
	{	
		pTx->buf[pTx->count++]	= 0;										//Byte03 ������� On/Off ����(�ϰ� ����ġ�� ��� ����)
		pTx->buf[pTx->count++]	= Feature_Request();						//Byte04 ��ɿ䱸(B0 : �������, B2 : ���������� Up �䱸, B3 : ���������� Down �䱸, B4 : ��� ����ġ ���º��� �䱸)
		pTx->buf[pTx->count++]	= Feature_Presence();						//Byte05 �������(B0 : �������� ���, B2 : �ϰ��ҵ� ���, B4 : ���������� ���, B5 : ��� ���)
		pTx->buf[pTx->count++]	= 0;										//Byte06 ��ɿ䱸2(�ϰ� ����ġ�� ��� ����)
	}
	else																	//�䱸 Command�� �� �ܸ�
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
						if(LED_State & COMMAX_GAS_STATE)						//bit0 ���� ��� ���� OPEN
						{
							touch_switch = item2tsn(mapping_ITEM_GAS);

							if(GET_LED_State(touch_switch) == LED_FLASHING)		//���� ���� ��û���϶��� 0x22���� ������ �޾Ƽ� ����.
							{
								// BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//���� ����
							}
							else if(GET_Switch_State(touch_switch))				//���� ���� �������̸�
							{
								BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);		//���� ��������
							}
						}
						else if((LED_State & COMMAX_GAS_STATE) == 0)			//���е忡�� ������� ���� CLOSE�� ����
						{
							touch_switch = item2tsn(mapping_ITEM_GAS);
							if(GET_LED_State(touch_switch) == LED_FLASHING)
							{
								;
							}
							else if(GET_Switch_State(touch_switch) == 0)				//���� ���� ���� �����϶�, �������� ��û���϶�
							{
								BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
							}
						}
					}
					if(item2tsn(mapping_ITEM_3WAY_1))
					{
						if(LED_State & COMMAX_3WAY_INSTALL)						//bit3 ��� ����ġ ��ġ
						{
							if(LED_State & COMMAX_3WAY_STATE)					//bit2 ��� ����ġ ���� On
							{
								EventCtrl(item2tsn(mapping_ITEM_3WAY_1), ON);
							}
							else if((LED_State & COMMAX_3WAY_STATE) == 0)								//��� ����ġ ���� Off
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
					break;*/		//Command 21 �����ص� �ȴٰ� ��. Command 22 ���.
				case COMMAX_BATCH_BLOCK_COMM_CONTROL:	//���¿䱸 ����[A0]�� ��ɿ䱸�� ���� ���е�� �Ϸ�/ ���п� ���� ������ ������ �����ϱ� ���� �뵵�� ���. �ϰ� �ҵ� ����.
					if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_BATCHLIGHT)
					{
						if((pRx->buf[COMMAX_F_BYTE_02] & 0x01) == COMMAX_BATCH_LIGHT_ON)		BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);	//�ϰ�����
						else if((pRx->buf[COMMAX_F_BYTE_02] & 0x01) == COMMAX_BATCH_LIGHT_OFF)	BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);	//�ϰ��ҵ�
					}
					if(item2tsn(mapping_ITEM_GAS))
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)		//���� ���� ��û���϶�
						{
							if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_GAS_CLOSE_SUCCESS)		//���� ����
							{
								if((pRx->buf[COMMAX_F_BYTE_02] & COMMAX_GAS_CLOSE_SUCCESS) == COMMAX_GAS_CLOSE_SUCCESS)	BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);	//���� ���� �����ϸ� ���� ���·�
							}
							else if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_GAS_CLOSE_FAIL)	//���� ���� �Ұ�
							{
								if((pRx->buf[COMMAX_F_BYTE_02] & COMMAX_GAS_CLOSE_FAIL) == COMMAX_GAS_CLOSE_FAIL)		BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);	//���� ���� �����ϸ� ���� ���·�
							}
						}
					}
					if(item2tsn(mapping_ITEM_ELEVATOR))
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	//���������� ȣ�� ��û���϶�
						{
							if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_ELEVATOR_CALL_SUCCESS)
							{
								if((pRx->buf[COMMAX_F_BYTE_02] & COMMAX_ELEVATOR_SUCCESS) == COMMAX_ELEVATOR_SUCCESS)	BATCH_BLOCK_Control(SET__ELEVATOR_CALL);	//���������� ȣ�� �����ϸ� ȣ�� ���·�
							}
							else if(pRx->buf[COMMAX_F_BYTE_03] == COMMAX_VERIATY_ELEVATOR_CALL_FAIL)
							{
								if((pRx->buf[COMMAX_F_BYTE_02] & COMMAX_ELEVATOR_FAIL) == COMMAX_ELEVATOR_FAIL)			BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);	//���������� ȣ�� �����ϸ� �⺻ ���·�
							}
						}
					}
						//���� ������ ������� �ʴ´ٰ� ��.......... �ٽ� Ȯ���ؾ���.
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
			if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))     //485��� �׽�Ʈ�� ���ؼ� �߰��� 
			{
				//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.
				/*if(pRx->buf[2] != pG_Config->RS485_ID)			//���α׷����� ������ ����ġ ID�� ����ġ�� ID�� �ٸ���
				{
					pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
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
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_GAS)) == LED_FLASHING)		// �������ݿ� ���� ������ ������ OPEN ���·� ��ȯ
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
			}
		}
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(Gu16_Elevator_Tmr == 0)
		{
			if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)			//ȣ�� ��û���϶� 60�� ������ LED �������
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);
			}
			else if(GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR)))					//ȣ�� ����������, 60�� ������ LED �������
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
			if(GET_Switch_State(touch_switch))		//���� �������϶�
			{
				Beep(ON);
			}
			else													//���� �������� �ƴҶ�
			{
				Gu16_GAS_Off_Tmr	= 60;						// 60�� ��� �� LED �ҵ�(�� ���е忡�� close/open �����Ͱ� ���ŵǸ� �ش� ���·� ��ȯ), 
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, LED_FLASHING);	// LED FLASHING
				Beep(ON);
				// printf("Gas REQUEST\r\n");
			}

			break;
		case SET__GAS_CLOSE_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) == 0)		//����  ���� �ƴ� ��
			{
				SET_Switch_State(touch_switch, ON);		// ������� ����(����)
				SET_LED_State(touch_switch, OFF);		// �����δ� LED ����
				Beep(ON);
			}
			// printf("GAS CLOSE\r\n");
			break;
		case SET__GAS_OPEN_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) || (GET_LED_State(touch_switch) == LED_FLASHING))
			{
				SET_Switch_State(touch_switch, OFF);	// ������� ����
				SET_LED_State(touch_switch, ON);		// �����δ� LED ����
				Beep(ON);
			}
			// printf("GAS OPEN\r\n");
			break;
		
		case SET__ELEVATOR_REQUEST:																			//����ġ���� ���������� ��
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))		//����ġ OFF �ų� LED ���� Flashing�̸�
			{
				Gu16_Elevator_Tmr = 60;																		//��û Ÿ�̸� 60�� �ʱ�ȭ. 0�̵Ǹ� ��û ��ҵǰ� LED OFF��.
				SET_Switch_State(touch_switch, OFF);															//����ġ OFF
				SET_LED_State(touch_switch, LED_FLASHING);													//LED Flashing
				Beep(ON);
				// printf("ELEVATOR REQEUST\r\n");
			}
			break;
		case SET__ELEVATOR_CALL:																			//����⿡�� ���������� ��
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == 0)			//ȣ�� ��û ���°ų�, ȣ����� �ƴ� ��.
			{
				Gu16_Elevator_Tmr = 60;																			//�� ���°� �Ǹ� Ÿ�̸� 60�� �ʱ�ȭ. Ÿ�̸Ӱ� 0�Ǹ� ���� ������� ���ư�.
				SET_Switch_State(touch_switch, ON);																//����ġ ON
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
				if(GET_Switch_State(touch_switch))			//ȣ�� �����϶�
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, ON);				//���� or ȣ�� ���н� LED OFF					
					Beep(ON);
					// printf("ELEVATOR ARRIVE\r\n");
				}
			}
			else if(control == SET__ELEVATOR_CALL_FAIL)
			{
				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch))	//ȣ�� ��û ���°ų�, ȣ����� �� ��
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, ON);				//���� or ȣ�� ���н� LED OFF				
					Beep(ON);									//���������� ���������� �����׽�Ʈ ���� �ʾ�, ��� �����ϴ��� �𸣹Ƿ� �ϴ� ������ ������.
				}
				// printf("ELEVATOR FAIL\r\n");
			}*/
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch))	//ȣ�� ��û ���°ų�, ȣ����� �� ��
			{
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, ON);				//���� or ȣ�� ���н� LED OFF
#ifdef _COMMAX_PROTOCOL_
				Beep(ON);										//�ڸƽ��� ���������� ���� ������ ������� �ʴ´ٰ� �Ͽ� �Ѵ� ������ �����.
#else
				if(control == SET__ELEVATOR_ARRIVE)				//�������� ������ ����. 
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

/************************************************************************************
	Project		: ���ڽĽ���ġ
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

/*#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1���� - �ϰ����ܽ���ġ
void BATCH_BLOCK_STATE_Process(void);
#endif*/
uint8_t SAMSUNG_Batch_Light_State(uint8_t item);
void SAMSUNG_Data_Process(SAMSUNG_BUF	*pRx);
void BATCH_BLOCK_Control(uint8_t control);
// ----------------------------------------------------------------------------------------
static	SAMSUNG_BUF		RxBuf, TxBuf;

#define	MAX_SAMSUNG_DATA_SEQUENCE		9		//���� ���� ���� �ִ� 8������ SAMSUNG�� ��� ���� no�� 1���� �����ϹǷ� 8�̾ƴ϶� �ִ� 9�� ������.
uint8_t	SAMSUNG_LIGHT_ITEM_Sequence[MAX_SAMSUNG_DATA_SEQUENCE];
uint8_t	SAMSUNG_ELEC_ITEM_Sequence[MAX_SAMSUNG_DATA_SEQUENCE];
uint8_t SAMSUNG_DIMMING_LIGHT_ITEM_Sequence[MAX_SAMSUNG_DATA_SEQUENCE];
uint8_t Store_Light_State[MAX_SAMSUNG_DATA_SEQUENCE];		//�ϰ� �ҵ�� ���� ����
uint8_t old_batch_state, old_gas_state, old_elevator_state, batch_state, gas_state, elevator_state;
uint8_t Gu8_BatchLight_OFF_Flag, Gu8_BatchLight_OFF_Flag_2;
uint8_t Gu8_Gas_Request_Flag = 0;
// ----------------------------------------------------------------------------------------
void SET_SAMSUNG_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)				//�Ϲ����� ������� ��� ����
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
void SET_SAMSUNG_DIMMING_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)		//��� ��� ���� ���� �� ����� �κ��� �־ �߰���.
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
	if(item2tsn(mapping_ITEM_GAS))		old_gas_state		= GET_LED_State(item2tsn(mapping_ITEM_GAS));		//��û : FLASHING, ���� : LED ON, ���� : LED_OFF
	if(item2tsn(mapping_ITEM_ELEVATOR))	old_elevator_state	= GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR));	//��û : FLASHING, ���� : LED ON, ���� : LED_OFF
#endif
	
	memset(SAMSUNG_LIGHT_ITEM_Sequence, 0, MAX_SAMSUNG_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	memset(SAMSUNG_ELEC_ITEM_Sequence, 0, MAX_SAMSUNG_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	
	// �������� ������ �׸� ����
	// �����	 �ִ��׸�	: ���� 6��, ���� 4�� + ��� 2��
	// ����+��� �ִ��׸�	: ���� 4�� + ��� 2��, ����2�� + ���2�� + ���2��
	// ex) ���� 3��, ��� 2�� = ����1,����2,����3,���1,���2,0,0,0
	// ex) ���� 1��, ��� 1�� = ����1,���1,0,0,0,0,0,0
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// �������� ���� ����, ������� ������
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
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// �������� ���� �������, ���� ������
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
	
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// ������� �� 
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
uint8_t SAMSUNG_Crc(SAMSUNG_BUF *pTRx, uint8_t len)
{
	uint8_t i, crc = 0;
	
	for(i = SAMSUNG_F_HEADER; i < len; i++)		//header���� checksum �ձ��� xor ����
	{
		crc ^= pTRx->buf[i];
	}
	// crc ^= 0x80;		//7bit�� �׻� 0
	crc &= 0x7F;

	return crc;
}
uint8_t NIS_Crc(SAMSUNG_BUF *pTRx, uint8_t cal, uint8_t sel)		//����� 485��� �׽�Ʈ�� ���ؼ� �߰���.
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
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//�������� Ÿ��
	pTx->buf[pTx->count++]	= Get_485_ID();					//ID ��ȣ
	pTx->buf[pTx->count++]	= 0;

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

/*#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1���� - �ϰ����ܽ���ġ
	BATCH_BLOCK_STATE_Process();		// ����� ���� �ȵǴ� ��� �� �Լ��� ������� ����(�� LED ���¸� ��ȯ�� �� ����
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
			if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_ALL_DEVICE_ACK)	//HEADER : B0�̰� COMMAND : 0x52�� �ƴϸ� 
			{
				if(pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_REQUEST && pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_CONTROL)
				{
					pRx->count = 0;
				}
			}
			/*else if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER || pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_2_HEADER)	//HEADER : AD, A9�� ��
			{
				if(pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_CONTROL)	//COMMAND : 0x53�� �ƴϸ�
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
			if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_ALL_DEVICE_ACK && pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_REQUEST)	//HEADER : B0�̰� COMMAND : 0x52�� �ƴϸ� 
			{
				pRx->count = 0;
			}
			else if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER || pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_2_HEADER)	//HEADER : AD, A9�� ��
			{
				if(pRx->buf[SAMSUNG_F_COMMAND] != SAMSUNG_BATCH_BLOCK_CONTROL)	//COMMAND : 0x53�� �ƴϸ�
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
			if(GET_Switch_State(item2tsn(item)))		//���� ������������.
			{
				if(pG_State->Dimming_Level.Dimming1	!= (uint8_t)Dimming_Level)	Beep(ON);			// �������� ������ �߰�.
				pG_State->Dimming_Level.Dimming1		= (uint8_t)(Dimming_Level + 1);				// �Ｚ SDS ��� ������ 0�� ��� 1�ܰ�, 5�� ��� 6�ܰ� �̹Ƿ� + 1
				if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
				{
					pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
				}
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(GET_Switch_State(item2tsn(item)))
			{
				if(pG_State->Dimming_Level.Dimming2	!= (uint8_t)Dimming_Level)	Beep(ON);
				pG_State->Dimming_Level.Dimming2		= (uint8_t)(Dimming_Level + 1);
				if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
				{
					pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
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
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp1 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// �ִ� ũ��� ����
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
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
			else										Gu16_ElecLimitCurrent_1	= (uint16_t)((double)Gu16_LCD_Watt_1 * 0.8);	// ���� ���� 80%�� ����
			Gu8_LCD_ElecLimitCurrent_Flashing_Flag	= 1;
			Store_ElecLimitCurrent();		
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(((double)Gu16_LCD_Watt_2 * 0.8) < 0.0)	Gu16_ElecLimitCurrent_2	= 0;
			else										Gu16_ElecLimitCurrent_2	= (uint16_t)((double)Gu16_LCD_Watt_2 * 0.8);	// ���� ���� 80%�� ����
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
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
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
					PUT_RelayCtrl(item2ctrl(item), ON);	// �׸���� ����
					SET_SWITCH_Delay_OFF_Flag(item, 0);
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
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
				PUT_RelayCtrl(item2ctrl(item), OFF);	// �׸���� ����
				SET_SWITCH_Delay_OFF_Flag(item, 0);
				SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
				ALL_Electricity_Switch_LED_Ctrl();
			}

#endif
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// ����
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
	control = pRx->buf[SAMSUNG_F_SUB_DATA_2];	//�������� ������ ���忡 ���� �ɼ� ����̶�� ��� �Ǿ� ������, �ϴ� �߰���. SUB_DATA_2�� XX�� ǥ�õǾ� �ִµ� ��Ȯ�� ����� ���� ���� ��� ���缭 1�̸� ON, 0�̸� OFF�� ������. 
												//�ƴϸ� �� ��Ʈ�� ����?? �׷������ �׷��Դ� �ȵɰŰ�����..
	for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
	{
		item = SAMSUNG_LIGHT_ITEM_Sequence[i];
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
			case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
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
			if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)(pG_State->Dimming_Level.Dimming1 - 1);		//�ＺSDS �� ��� ������ 1�� 0���� ǥ����. �׷��� ��� ������ ǥ���� �� -1�� ��
			else											ret = SAMSUNG_OFF_FLAG;			
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)(pG_State->Dimming_Level.Dimming2 - 1);		//�ＺSDS �� ��� ������ 1�� 0���� ǥ����. �׷��� ��� ������ ǥ���� �� -1�� ��
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
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = SAMSUNG_ON_FLAG;
			else											ret = SAMSUNG_OFF_FLAG;
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			if(control == SAMSUNG_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = SAMSUNG_ON_FLAG;
				else											ret = SAMSUNG_OFF_FLAG;
			}
			else if(control == SAMSUNG_DIMMING)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)(pG_State->Dimming_Level.Dimming1 - 1);		//�ＺSDS �� ��� ������ 1�� 0���� ǥ����. �׷��� ��� ������ ǥ���� �� -1�� ��
				else											ret = SAMSUNG_OFF_FLAG;
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
			if(control == SAMSUNG_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = SAMSUNG_ON_FLAG;
				else											ret = SAMSUNG_OFF_FLAG;				
			}
			else if(control == SAMSUNG_DIMMING)
			{		
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret = (uint8_t)(pG_State->Dimming_Level.Dimming2 - 1);		//�ＺSDS �� ��� ������ 1�� 0���� ǥ����. �׷��� ��� ������ ǥ���� �� -1�� ��
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
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit4 �ڵ�/����, bit0 ���� ON/OFF
			{
				ret	= (uint8_t)(SAMSUNG_ON_FLAG | (pG_State->ETC.Auto1 << 4));
			}
			else
			{
				ret	= (uint8_t)(SAMSUNG_OFF_FLAG | (pG_State->ETC.Auto1 << 4));
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit4 �ڵ�/����, bit0 ���� ON/OFF
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

uint8_t SAMSUNG_Batch_Light_State(uint8_t item)		//�ϰ� �ҵ� �� ���� ���� ���� ����.
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
			if(control_value == SAMSUNG_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))
				{
					EventCtrl(item2tsn(item), OFF);						//���� OFF
				}
			}
			else if(control_value == SAMSUNG_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)
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


//------------------------------------------------------------------------------------------------------------------------------------------
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)		// 1����, 2����	- �����
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

void SAMSUNG_LIGHT_Model_Res(SAMSUNG_BUF *pRx)				//�������� ���� �䱸�� ���� ����
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
		if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x00)													//No Action�϶� ������� �䱸(�ش� ����� ��ü ���� ���¸� ������)
		{
			light_start_id	= pRx->buf[SAMSUNG_F_SUB_DATA_2];										//�ش� ����ġ�� ù ���� id����, ����ġ id.
			if(light_start_id != SAMSUNG_DIMMING_LIGHT_REQUEST)
			{
				if((light_start_id == Get_485_ID()) && ((uint8_t)(Get_485_ID() + Light_Cnt() - 1) < 17))	//sub data2�� 485id ���� ������ ��)
				{
					pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;									//buf 0
					pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];								//buf 1				
					pTx->buf[pTx->count++]	= (uint8_t)(((Light_Cnt() << 4) | light_start_id) & 0x7F);			//buf 2

					pTx->buf[pTx->count] = 0;		//for���� �����Ͱ� ���� ���� ��û ������ ������ ���� �������� �ʾƼ� �ʱ�ȭ. (�� ����� ��� ���� ON���� 0x0F, ���� ���°� �ٲ� 0x0F�� ������ ����)
					for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
					{
						pTx->buf[pTx->count] |= (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[i], on_n_off) << (i - 1));	//���� id 17�̻� ���ԵǴ� ����ġ�� ��� buf4, �ƴϸ� buf 3
					}
					pTx->buf[pTx->count++] &= 0x7F;		//7bit �� 0
					pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);					//���� id 17�̻� ���ԵǴ� ����ġ�� ��� buf5, �ƴϸ� buf 4
					TxBuf.send_flag	= 1;
				}
			}
			else																					//NO Action���� ��ֻ��� ��ȸ��
			{
				if(pG_Config->Enable_Flag.PWM_Dimming)												//����� �� ������ ��ֽ���ġ �ϳ��� ����.
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
		else																					//���� ���� �䱸(�ش��ϴ� ����ID�� ������ ���¸� ������)
		{
			light_id		= pRx->buf[SAMSUNG_F_SUB_DATA_1];
			if((light_id >= Get_485_ID()) && (light_id < (uint8_t)(Get_485_ID() + Light_Cnt())) && ((uint8_t)(Get_485_ID() + Light_Cnt() - 1) < 17))	//light_id�� �ش� ����ġ�� ���̵�� ���ų� ũ��, id + light_cnt �� ���� ������, 17ȸ�� �����϶� ex)switch id = 1, light_cnt = 4��� light_id 1 ~ 4���� 
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
			if((Get_485_ID() + Light_Cnt() - 1) >= 17)										//17ȸ�� �̻�, �ش� ��⿡ ���� ID 17�� �̻��� ���Եɶ� ex) ID : 15, ���� 4��(15,16, 17, 18). 15 + 4 = 19. 19 - 1 = 18
			{
				pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;							//buf 0
				pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];						//buf 1						
				pTx->buf[pTx->count++]	= (uint8_t)(Light_Cnt() & 0x7F);					//buf 2, ���� ��
				pTx->buf[pTx->count++]	= (uint8_t)(light_start_id & 0x7F);					//buf 3, ���� ���� ID
				pTx->buf[pTx->count] = 0;
				for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
				{
					pTx->buf[pTx->count] |= (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[i], on_n_off) << (i - 1));	//���� id 17�̻� ���ԵǴ� ����ġ�� ��� buf4, �ƴϸ� buf 3
				}
				pTx->buf[pTx->count++] &= 0x7F;
				pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);					//���� id 17�̻� ���ԵǴ� ����ġ�� ��� buf5, �ƴϸ� buf 4
				TxBuf.send_flag	= 1;
			}
		}
	}
	else																				//���� ����, ��� ����
	{
		light_id		= pRx->buf[SAMSUNG_F_SUB_DATA_1];
		if((light_id >= Get_485_ID()) && (light_id < (uint8_t)(Get_485_ID() + Light_Cnt())))	//light_id�� �ش� ����ġ�� ���̵�� ���ų� ũ��, id + light_cnt �� ���� ������ ex)switch id = 1, light_cnt = 4��� light_id 1 ~ 4���� 
		{			
			pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;								//ACK
			pTx->buf[pTx->count++] = pRx->buf[SAMSUNG_F_COMMAND];							//CMD
			pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] & 0x7F);						//����ID
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
	
	item = SAMSUNG_LIGHT_ITEM_Sequence[(uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] - (Get_485_ID() - 1))];	//ex)����(9 ~ 12)�� ���. 4��° ����(ID : 12)�� ���� �Ϸ��� 12 - (9 - 1) = 4
	
	if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_LIGHT_HEADER)		//0xAC
	{
		switch(pRx->buf[SAMSUNG_F_COMMAND])
		{
			case SAMSUNG_POWER_ON:								//0x5A
				if(Get_485_ID() == 0x01)	SAMSUNG_LIGHT_POWER_ON_Res(pRx);		//��ǥ ���� �ϳ��� ����.
				break;
			case SAMSUNG_STATUS_DATA:							//0x41
				if(Get_485_ID() == 0x01)	SAMSUNG_Status_Data_Res();			//��ǥ ���� �ϳ��� ����.
				break;
			case SAMSUNG_LIGHT_STATE_REQUEST:					//0x79
			case SAMSUNG_LIGHT_STATE_17circuit_REQUEST:			//0x75
				SAMSUNG_LIGHT_Model_Res(pRx);
				break;
			case SAMSUNG_LIGHT_CONTROL:					//0x7A
				if(switch_id == 0)						//ID�� 0���̸� ��ü ����� ��ü ���� ����.
				{
					/*if(Gu8_BatchLight_OFF_Flag == 0)
					{
						if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_OFF_FLAG)		//��ü ���� OFF �϶�(�ϰ� �ҵ�)�� ���� ����.
						{
							for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
							{
								Store_Light_State[i] = SAMSUNG_Batch_Light_State(SAMSUNG_LIGHT_ITEM_Sequence[i]);	//�ҵ� �� ���� ���� ���� ����.
							}
							Gu8_BatchLight_OFF_Flag = 1;
						}
						SAMSUNG_Light_GROUP_Control(pRx);	//��ü ��� ����.
					}
					if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_ON_FLAG)		//��ü �ѱ�(�ϰ�X)
					{
						SAMSUNG_Light_GROUP_Control(pRx);
					}*/

					/*if(Gu8_BatchLight_OFF_Flag == 0)	//�ϰ� �ҵ� ���°� �ƴϰ�, 
					{
						if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_OFF_FLAG)		//��ü ���� OFF �϶�(�ϰ� �ҵ�)�� ���� ����.
						{
							for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
							{
								Store_Light_State[i] = SAMSUNG_Batch_Light_State(SAMSUNG_LIGHT_ITEM_Sequence[i]);	//�ҵ� �� ���� ���� ���� ����.
							}
							// Gu8_BatchLight_OFF_Flag = 1;
						}
						SAMSUNG_Light_GROUP_Control(pRx);	//��ü ��� ����.							
					}
					else	//��ü �ѱ�, ���� �� ��(�÷��� ����), ������ ���� ����.
					{
						SAMSUNG_Light_GROUP_Control(pRx);	//SUB_DATA_2�� ���� ��ü ON / OFF
					}*/

					if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_OFF_FLAG)
					{
						if(Gu8_BatchLight_OFF_Flag == 0)
						{
							for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
							{
								Store_Light_State[i] = SAMSUNG_Batch_Light_State(SAMSUNG_LIGHT_ITEM_Sequence[i]);	//�ҵ� �� ���� ���� ���� ����.
							}
							Gu8_BatchLight_OFF_Flag = 1;
							SAMSUNG_Light_GROUP_Control(pRx);							
						}
					}
					else if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_ON_FLAG)
					{
						Gu8_BatchLight_OFF_Flag = 0;	//���� �ϰ��ҵ� ���¿��� ��ü ���� ON �ϸ� �ϰ� ���� ������ ���Ͱ� �ȵǴµ�..
						SAMSUNG_Light_GROUP_Control(pRx);
					}

					if(Get_485_ID() == 0x01)			//ù��° ��⸸ ��ǥ�� ����.
					{
						SAMSUNG_All_Light_Res(pRx);
					}
				}
				else
				{
					Gu8_BatchLight_OFF_Flag = 0;	//���� �ϰ��ҵ� ���¿��� ���� ���� ON �ϸ� �ϰ� ���� ������ ���Ͱ� �ȵǴµ�..
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
			{	//B0 53 00 XX	�ϰ��ҵ� ��� ���� �ϰ� ����ġ�� ����.
				Gu8_BatchLight_OFF_Flag_2 = 1;	//�ϰ� �ҵ� ���� �÷���
			}
			else if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == SAMSUNG_ON_FLAG)
			{	//B0 53 01 XX	�ϰ����� ��� ���� �ϰ� ����ġ�� ����.
				if(Gu8_BatchLight_OFF_Flag && Gu8_BatchLight_OFF_Flag_2)
				{
					// printf("flag 1\r\n");
					for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
					{
						if(Store_Light_State[i] == SAMSUNG_ON_FLAG)		//����� ���� ���°� ON�� ���
						{
							SAMSUNG_BatchLight_Control(SAMSUNG_LIGHT_ITEM_Sequence[i], SAMSUNG_ON_FLAG);	//���� ON
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
						// SAMSUNG_BatchLight_Control(SAMSUNG_LIGHT_ITEM_Sequence[i], Store_Light_State[i]);	//���� ON
						if(Store_Light_State[i] == SAMSUNG_ON_FLAG)		//����� ���� ���°� ON�� ���
						{
							SAMSUNG_BatchLight_Control(SAMSUNG_LIGHT_ITEM_Sequence[i], SAMSUNG_ON_FLAG);	//���� ON
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
		//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.
		/*
		if(pRx->buf[2] != pG_Config->RS485_ID)			//���α׷����� ������ ����ġ ID�� ����ġ�� ID�� �ٸ���
		{
			pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
			printf("Switch ID Change\r\n");
		}
		Store_CurrentConfig();
		*/
		RS_485_ID_RES();
	}
}
#endif	// defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)	// 2����	- ����+������
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

void SAMSUNG_LIGHT_Model_Res(SAMSUNG_BUF *pRx)				//�������� ���� �䱸�� ���� ����
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
		if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x00)													//No Action�϶� ������� �䱸(�ش� ����� ��ü ���� ���¸� ������)
		{
			light_start_id	= pRx->buf[SAMSUNG_F_SUB_DATA_2];										//�ش� ����ġ�� ù ���� id����, ����ġ id.
			if(light_start_id != SAMSUNG_DIMMING_LIGHT_REQUEST)
			{
				if((light_start_id == Get_485_ID()) && ((uint8_t)(Get_485_ID() + Light_Cnt() - 1) < 17))	//sub data2�� 485id ���� ������ ��)
				{
					pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;									//buf 0
					pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];								//buf 1				
					pTx->buf[pTx->count++]	= (uint8_t)(((Light_Cnt() << 4) | light_start_id) & 0x7F);			//buf 2

					pTx->buf[pTx->count] = 0;		//for���� �����Ͱ� ���� ���� ��û ������ ������ ���� �������� �ʾƼ� �ʱ�ȭ. (�� ����� ��� ���� ON���� 0x0F, ���� ���°� �ٲ� 0x0F�� ������ ����)
					for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
					{
						pTx->buf[pTx->count] |= (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[i], on_n_off) << (i - 1));	//���� id 17�̻� ���ԵǴ� ����ġ�� ��� buf4, �ƴϸ� buf 3
					}
					pTx->buf[pTx->count++] &= 0x7F;		//7bit �� 0
					pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);					//���� id 17�̻� ���ԵǴ� ����ġ�� ��� buf5, �ƴϸ� buf 4
					TxBuf.send_flag	= 1;
				}
			}
			else																					//NO Action���� ��ֻ��� ��ȸ��
			{
				if(pG_Config->Enable_Flag.PWM_Dimming)												//����� �� ������ ��ֽ���ġ �ϳ��� ����.
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
		else																					//���� ���� �䱸(�ش��ϴ� ����ID�� ������ ���¸� ������)
		{
			light_id		= pRx->buf[SAMSUNG_F_SUB_DATA_1];
			if((light_id >= Get_485_ID()) && (light_id < (uint8_t)(Get_485_ID() + Light_Cnt())) && ((uint8_t)(Get_485_ID() + Light_Cnt() - 1) < 17))	//light_id�� �ش� ����ġ�� ���̵�� ���ų� ũ��, id + light_cnt �� ���� ������, 17ȸ�� �����϶� ex)switch id = 1, light_cnt = 4��� light_id 1 ~ 4���� 
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
			if((Get_485_ID() + Light_Cnt() - 1) >= 17)										//17ȸ�� �̻�, �ش� ��⿡ ���� ID 17�� �̻��� ���Եɶ� ex) ID : 15, ���� 4��(15,16, 17, 18). 15 + 4 = 19. 19 - 1 = 18
			{
				pTx->buf[pTx->count++]	= SAMSUNG_ALL_DEVICE_ACK;							//buf 0
				pTx->buf[pTx->count++]	= pRx->buf[SAMSUNG_F_COMMAND];						//buf 1						
				pTx->buf[pTx->count++]	= (uint8_t)(Light_Cnt() & 0x7F);										//buf 2, ���� ��
				pTx->buf[pTx->count++]	= (uint8_t)(light_start_id & 0x7F);									//buf 3, ���� ���� ID
				pTx->buf[pTx->count] = 0;
				for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
				{
					pTx->buf[pTx->count] |= (uint8_t)(SAMSUNG_LIGHT_Data_Conversion(SAMSUNG_LIGHT_ITEM_Sequence[i], on_n_off) << (i - 1));	//���� id 17�̻� ���ԵǴ� ����ġ�� ��� buf4, �ƴϸ� buf 3
				}
				pTx->buf[pTx->count++] &= 0x7F;
				pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_LIGHT_CHECK_SUM);					//���� id 17�̻� ���ԵǴ� ����ġ�� ��� buf5, �ƴϸ� buf 4
				TxBuf.send_flag	= 1;
			}
		}
	}
	else																				//���� ����, ��� ����
	{
		light_id		= pRx->buf[SAMSUNG_F_SUB_DATA_1];
		if((light_id >= Get_485_ID()) && (light_id < (uint8_t)(Get_485_ID() + Light_Cnt())))	//light_id�� �ش� ����ġ�� ���̵�� ���ų� ũ��, id + light_cnt �� ���� ������ ex)switch id = 1, light_cnt = 4��� light_id 1 ~ 4���� 
		{			
			pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;								//ACK
			pTx->buf[pTx->count++] = pRx->buf[SAMSUNG_F_COMMAND];							//CMD
			pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] & 0x7F);						//����ID
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

	item = SAMSUNG_LIGHT_ITEM_Sequence[(uint8_t)(pRx->buf[SAMSUNG_F_SUB_DATA_1] - (Get_485_ID() - 1))];	//ex)����(9 ~ 12)�� ���. 4��° ����(ID : 12)�� ���� �Ϸ��� 12 - (9 - 1) = 4
	
	if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_LIGHT_HEADER)		//0xAC
	{
		switch(pRx->buf[SAMSUNG_F_COMMAND])
		{
			case SAMSUNG_POWER_ON:								//0x5A
				if(Get_485_ID() == 0x01)	SAMSUNG_LIGHT_POWER_ON_Res(pRx);		//��ǥ ���� �ϳ��� ����.
				break;
			case SAMSUNG_STATUS_DATA:							//0x41
				if(Get_485_ID() == 0x01)	SAMSUNG_Status_Data_Res(pRx);			//��ǥ ���� �ϳ��� ����.
				break;				
			case SAMSUNG_LIGHT_STATE_REQUEST:					//0x79
			case SAMSUNG_LIGHT_STATE_17circuit_REQUEST:			//0x75
			case SAMSUNG_DIMMING_LIGHT_REQUEST:
				SAMSUNG_LIGHT_Model_Res(pRx);
				break;
			case SAMSUNG_LIGHT_CONTROL:					//0x7A
				if(switch_id == 0)						//ID�� 0���̸� ��ü ����� ��ü ���� ����.
				{
					if(pRx->buf[SAMSUNG_F_SUB_DATA_2] == SAMSUNG_OFF_FLAG)		//��ü ���� OFF �϶�(�ϰ� �ҵ�)�� ���� ����.
					{
						for(i = 1; i < MAX_SAMSUNG_DATA_SEQUENCE; i++)
						{
							Store_Light_State[i] = SAMSUNG_Batch_Light_State(SAMSUNG_LIGHT_ITEM_Sequence[i]);	//�ҵ� �� ���� ���� ���� ����.
						}
					}
					SAMSUNG_Light_GROUP_Control(pRx);	//��ü ��� ����.
					if(Get_485_ID() == 0x01)			//ù��° ��⸸ ��ǥ�� ����.
					{
						SAMSUNG_All_Light_Res(pRx);
					}
				}
				else										//ID�� 0���� �ƴҶ��� �ش� ��� ����.
				{
					SAMSUNG_Control(pRx, item);
					SAMSUNG_LIGHT_Model_Res(pRx);
				}
				break;
			case SAMSUNG_LIGHT_GROUP_CONTROL:
				SAMSUNG_Light_GROUP_Control(pRx);
				SAMSUNG_LIGHT_Model_Res(pRx);
				break;
			case SAMSUNG_DIMMING_LIGHT_CONTROL:					//0x7B ����� ��Ʈ��ũ ����ġ�� ��� ��� ����
				SET_DimmingLevel(item, pRx->buf[SAMSUNG_F_SUB_DATA_2]);
				SAMSUNG_LIGHT_Model_Res(pRx);
				break;
		}
	}
	else else if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.

		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//���α׷����� ������ ����ġ ID�� ����ġ�� ID�� �ٸ���
		{
			pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
			printf("Switch ID Change\r\n");
		}
		Store_CurrentConfig();*/
		RS_485_ID_RES();
	}
}
#endif	// defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1���� - �ϰ����ܽ���ġ

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
	
	// if(pRx->buf[SAMSUNG_F_HEADER] == SAMSUNG_BATCH_BLOCK_HEADER)		//�ϰ� ����
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
	}*/		//240417 0x50, 0x51�� �ϰ�����ġ�� �������ܱⰡ ���� ������ �Ǿ� �ִ� ��� ����Ͽ� �������� �ʾƵ� �ȴٰ� ��.
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
			case SAMSUNG_BATCH_BLOCK_REQUEST:		//0x52 �ϰ� ���� �䱸(���е� -> �ϰ�)
			case SAMSUNG_BATCH_BLOCK_CONTROL:		//0x53 �ϰ� ����, �ҵ� �䱸(���е� -> �ϰ�)
				if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_CONTROL)
				{
					if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == SAMSUNG_ON_FLAG)			BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);		//�ϰ�����
					else if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == SAMSUNG_OFF_FLAG)		BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);		//�ϰ��ҵ�
				}
				SAMSUNG_BATCH_BLOCK_Data_Res(pRx);
				break;
			/*case SAMSUNG_BATCH_BLOCK_GAS_REQUEST:	//0x50 ���� ��� ���� �䱸(���е� -> �ϰ�)
			case SAMSUNG_BATCH_BLOCK_GAS_CONTROL:	//0x51 ���� ���� �䱸(���е� -> �ϰ�)
				if(item2tsn(mapping_ITEM_GAS))
				{
					if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_GAS_CONTROL)
					{
						if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x01)						BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//���� ����
					}
					SAMSUNG_BATCH_BLOCK_Data_Res(pRx);
				}
				break;*/	//240417 0x50, 0x51�� �ϰ�����ġ�� �������ܱⰡ ���� ������ �Ǿ� �ִ� ��� ����Ͽ� �������� �ʾƵ� �ȴٰ� ��.
			case SAMSUNG_BATCH_BLOCK_SWITCH_CONTROL:				//����ġ���� ���� �� ���е忡�� �ϰ��ҵ�(����)�� ���� ACK
			case SAMSUNG_BATCH_BLOCK_SWITCH_GAS_CONTROL:			//����ġ���� ���� ���� ��û �� ���е忡�� ���� ���� ��û�� ���� ACK
			case SAMSUNG_BATCH_BLOCK_SWITCH_GAS_REQUEST:			//����ġ���� ���� ���¿䱸 �� ���е忡�� ���� ���¿䱸�� ���� ACK
				if(item2tsn(mapping_ITEM_GAS))
				{
					if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_SWITCH_GAS_CONTROL)	//0x55
					{
						// if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)		//���� ���� ��û���϶���,
						// {
							// if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x01)	BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);		//������� ����ġ���� ���� ���� ���·� ����.
							Gu8_Gas_Request_Flag = 0;	//�ش��� �䱸�� ���� Ack ������ ��û �÷��� �ʱ�ȭ.
						// }
					}
					else if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_SWITCH_GAS_REQUEST)	//0x56
					{
						if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x00)
						{
							if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) != LED_FLASHING)	//���� ���� ��û���� �ƴ� ��츸 ���� ������.
							{
								BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);	//���� ���� ����
							}
						}
						else if(pRx->buf[SAMSUNG_F_SUB_DATA_1] == 0x01)		BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);	//���� ����
					}
					SAMSUNG_Status_Data_Res();
				}
				break;
			case SAMSUNG_BATCH_BLOCK_SWITCH_ELEVATOR_CONTROL:		//����ġ���� ���������� �� ��û �� ���е忡�� ���������� �ݿ� ���� ACK
				if(item2tsn(mapping_ITEM_ELEVATOR))
				{
					if(pRx->buf[SAMSUNG_F_COMMAND] == SAMSUNG_BATCH_BLOCK_SWITCH_ELEVATOR_CONTROL)
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	//240409 ���������� ��û���� ����
						{
							BATCH_BLOCK_Control(SET__ELEVATOR_CALL);
						}
					}
					SAMSUNG_Status_Data_Res();
				}
				// SAMSUNG_Status_Data_Res();	//�ش� ��� ����� ��츸 �����ϵ��� ����
				break;
			case SAMSUNG_STATUS_DATA:
				batch_state = GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF));
				if(old_batch_state != batch_state)													state |= 0x01;
				
				if(item2tsn(mapping_ITEM_GAS))
				{
					// gas_state = GET_LED_State(item2tsn(mapping_ITEM_GAS));							//��û : FLASHING, ���� : LED ON, ���� : LED_OFF
					// if((old_gas_state != gas_state) && gas_state == LED_FLASHING)					state |= 0x02;				//11 OFF, 12 FLASING,
					if((state & 0x02) == 0)
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING && Gu8_Gas_Request_Flag)	state |= 0x02;	//���� ���� ��û��(LED FLASHING)�� ���� ���� ��û �÷��� ���� ������ 
					}
				}
				if(item2tsn(mapping_ITEM_ELEVATOR))
				{
					// elevator_state = GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR));				//��û : FLASHING, ���� : LED ON, ���� : LED_OFF
					// if((old_elevator_state != elevator_state) && elevator_state == LED_FLASHING)	state |= 0x04;
					if((state & 0x04) == 0)
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)	state |= 0x04;
					}
				}
				// printf("old = %d, current = %d\r\n", (uint16_t)old_gas_state, (uint16_t)gas_state);
				if(state != 0)		//�ϰ� ����ġ�� ���� ��ȭ��������
				{
					// printf("state = %x ", (uint16_t)state);
					// if(state == 0x01 || state == 0x03 || state == 0x05 || state == 0x07)		//0x01 : �ϰ�, 0x03 : �ϰ�, ����, 0x05 : �ϰ�, ����, 0x07 : �ϰ�, ����, ����
					if((state & 0x01) == 0x01)
					{
						pTx->count = SAMSUNG_F_HEADER;
						pTx->buf[pTx->count++] = SAMSUNG_ALL_DEVICE_ACK;
						pTx->buf[pTx->count++] = SAMSUNG_BATCH_BLOCK_SWITCH_CONTROL;
						if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	pTx->buf[pTx->count++]	= (uint8_t)(SAMSUNG_ON_FLAG & 0x7F);		//���� 1
						else															pTx->buf[pTx->count++]	= (uint8_t)(SAMSUNG_OFF_FLAG & 0x7F);		//�ҵ� 0
						pTx->buf[pTx->count++] = SAMSUNG_Crc(pTx, SAMSUNG_F_BATCH_BLOCK_CHECK_SUM);
						TxBuf.send_flag	= 1;								
						state = (uint8_t)(state ^ 0x01);
						old_batch_state = batch_state;						
					}
					// else if(state == 0x02 || state == 0x06)										//0x02 : ����, 0x06 : ����, ����

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
		//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.
		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//���α׷����� ������ ����ġ ID�� ����ġ�� ID�� �ٸ���
		{
			pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
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
			if((GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING) || (GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR))))			//ȣ�� ��û���϶� 60�� ������ LED �������
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
			if(GET_Switch_State(touch_switch))		//���� �������϶�
			{
				Beep(ON);
				// printf("Gas State Close. Re-REQUEST\r\n");			//�������̶� ���û.
			}
			else													//���� �������� �ƴҶ�
			{
				Gu16_GAS_Off_Tmr	= 30;						// 5��(300->270), 60��(�ӽ�) ��� �� LED �ҵ�(�� ���е忡�� close/open �����Ͱ� ���ŵǸ� �ش� ���·� ��ȯ), 
				SET_Switch_State(touch_switch, OFF);
				SET_LED_State(touch_switch, LED_FLASHING);	// LED FLASHING
				Beep(OFF);
				Gu8_Gas_Request_Flag = 1;
				// printf("Gas REQUEST\r\n");
			}

			break;
		case SET__GAS_CLOSE_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) == 0)		//���� ���� �����϶��� ���� ����.
			{	
				SET_Switch_State(touch_switch, ON);		// ������� ����(����)
				SET_LED_State(touch_switch, OFF);		// �����δ� LED ����
				Beep(ON);
			}
			// printf("GAS CLOSE\r\n");
			break;
		case SET__GAS_OPEN_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) || GET_LED_State(touch_switch) == LED_FLASHING)			//���� ���� �����϶��� ���� ����.
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
				// printf("ELEVATOR REQUEST\r\n");
			}
			break;
		case SET__ELEVATOR_CALL:																			//����⿡�� ���������� ��
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_Switch_State(touch_switch) == OFF)
			{
				Gu16_Elevator_Tmr = 10;																			//�� ���°� �Ǹ� Ÿ�̸� 60�� �ʱ�ȭ. Ÿ�̸Ӱ� 0�Ǹ� ���� ������� ���ư�.
				//240411 �ش� ���������� ���� ������ �����Ƿ�. ���������� �� ���� �� 10�� �� ��� ���� ��ȯ
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
					// Beep(ON);									//���������� ���������� �����׽�Ʈ ���� �ʾ�, ��� �����ϴ��� �𸣹Ƿ� �ϴ� ������ ������.
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


#endif	// _SAMSUNG_PROTOCOL_
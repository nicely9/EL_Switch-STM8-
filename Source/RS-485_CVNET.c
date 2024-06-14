/************************************************************************************
	Project		: ���ڽĽ���ġ
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

/*CVNET�� ��� ���� ���� ���� �������θ� ������� ����ġ�� ����� ��� ����+������� �������� ������� �ʰ� ���� �������� ����� ���� �ִ�.
������� �Ž�(ID1)�� ��Ʈ��ũ ����ġ�̰� �ȹ�(ID2)�� ������� ����ġ��� ���� ������� ����ġ�� ���� �������� ��� �ؾ��Ѵ�.
���� ���忡 ��Ʈ��ũ ����ġ�� ���� �ʰ�, ��� ������� ����ġ�� ����Ѵٸ� �ش� ���� � ���������� ������ �� ���� Ȥ�� CVNET ������ ���� �ؾ��Ѵ�.*/
/*

����Ư���䱸(0x22)				�ش� ��� ������� ����. ���� ���� �ʾƵ� ��.
�׷� ����(0x31 ~ 0x3E, 0x3F)	�ش� ��� ������� ����. ���� ���� �ʾƵ� ��.
CVNET�� ���е� �ɼ����� �ϰ� �ҵ� �� ���� ���¸� �����ϰ� �ҵ� ���� �� ���� �� ���� ���·� ������.
�ϰ� ����ġ�� ���
�ϰ� ���� ����ġ �������ݿ� ���� ���(ex ��� ��)�� ������� ������ �ϰ� ���� ����ġ ���������� ����ؾ���.
 >> ���� ����ġ �������� ����ϸ� �ȵ�.
���� ����ġ �������ݿ��� ������� �ʴ� ����� �����͸� 0���� ó���ؼ� ����.
3�� ������ ��� ���� �������ݿ��� ���䵵 ��� ���� �ʴ´�. �׷� ������� ���� �ʴ´�. ���¿� ���� �亯
*/

void CVNET_Data_Process(CVNET_BUF	*pRx);
// ----------------------------------------------------------------------------------------
static	CVNET_BUF		RxBuf, TxBuf;
#define ELEVATOR_TMR	162		//(180-18)3�� �̻� ���������� ������ȣ�� ���� ���� ��� ��ư ���¸� �ʱ�ȭ.
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
	
	memset(CVNET_LIGHT_ITEM_Sequence, 0, MAX_CVNET_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	memset(CVNET_ELEC_ITEM_Sequence, 0, MAX_CVNET_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	
	// �������� ������ �׸� ����
	// �����	 �ִ��׸�	: ���� 6��, ���� 4�� + ��� 2��
	// ����+��� �ִ��׸�	: ���� 4�� + ��� 2��, ����2�� + ���2�� + ���2��
	// ex) ���� 3��, ��� 2�� = ����1,����2,����3,���1,���2,0,0,0
	// ex) ���� 1��, ��� 1�� = ����1,���1,0,0,0,0,0,0
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// �������� ���� ����, ������� ������
	count	= 0;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	// if(item2tsn(mapping_ITEM_3WAY_1))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);	//CVNET�� 3�� ���������� ������ ����ϹǷ� �ش� �Լ��� ��� ����
	// if(item2tsn(mapping_ITEM_3WAY_2))			SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_CVNET_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
#endif
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// �������� ���� �������, ���� ������
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
	
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// ������� �� 
	count	= 0;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_CVNET_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_CVNET_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
#endif
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
	return	(uint8_t)(pG_Config->RS485_ID | CVNET_BATCH_BLOCK_DEVICE_ID);	// ��巹�� ������ : ID 11(0xB), ID 12(0xC), ID 13(0xD), ID 14(0xE)�� �����ؾ� ��
}

uint8_t Get_Elevator_485_ID(void)
{
	return (uint8_t)((pG_Config->RS485_ID - 0x0A) | CVNET_BATCH_BLOCK_DEVICE_ID);	//���������� ȣ�� ������ ID�� 0x0B���� �������� �ʰ� 0x01���� ������.
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
uint8_t NIS_Crc(CVNET_BUF *pTRx, uint8_t cal, uint8_t sel)		//����� 485��� �׽�Ʈ�� ���ؼ� �߰���.
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
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//�������� Ÿ��
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
	pTx->buf[pTx->count++]	= (uint8_t)(Get_485_ID() ^ CVNET_LIGHT_DEVICE_ID);					//ID ��ȣ
#elif defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
	#ifdef _LIGHT_SWITCH_PROTOCOL_USE_
	pTx->buf[pTx->count++]	= (uint8_t)(Get_485_ID() ^ CVNET_LIGHT_DEVICE_ID);					//ID ��ȣ
	#else
	pTx->buf[pTx->count++]	= (uint8_t)(Get_485_ID() ^ CVNET_LIGHT_ELEC_DEVICE_ID);				//ID ��ȣ
	#endif
#elif defined(_ONE_SIZE_BATCH_BLOCK_MODEL_)
#ifdef _BATCH_BLOCK_SWITCH_PROTOCOL_
	pTx->buf[pTx->count++]	= (uint8_t)((Get_Batch_Block_485_ID() ^ CVNET_BATCH_BLOCK_DEVICE_ID) - 0x0A);	//ID ��ȣ, �ϰ�����ġ�� ��� ID�� 0x8B���� �����ϹǷ� ���α׷� ������ ���� -0x0A��.
#else	//_TOTAL_SITWCH_PROTOCOL_
	pTx->buf[pTx->count++]	= (uint8_t)(pG_Config->RS485_ID);	//Total ����ġ�� 0xE1 �ϳ��̹Ƿ� �ٷ� RS ID �� ����
#endif
#endif
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

// #ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1���� - �ϰ����ܽ���ġ, �ϰ��� �ִ� �𵨿��� ����Ǿ� LED ������ ��µǾ� �Ʒ��� ������.
/*#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
	BATCH_BLOCK_STATE_Process();		// ����� ���� �ȵǴ� ��� �� �Լ��� ������� ����(�� LED ���¸� ��Ȱ�� �� ����
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
			// && (pRx->buf[CVNET_FRAME_DA] != Get_TotalSwitch_485_ID()) && (pRx->buf[CVNET_FRAME_DA] != Get_Threeway_485_ID()))	// �� �ּҰ� �ƴϰ� �׷����� �ּҵ� �ƴϸ�
			{
				if((pRx->buf[CVNET_FRAME_PRE] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[CVNET_FRAME_HD] != NIS_LIGHT_ID_COMM_2))
				{
					// printf("DA\r\n");
					//��Ʈ��ũ, Get_485_ID 21, 
					//�ϰ�, Get_485_ID 8B, Elevator ID 81, Group 8F, Total E1, Threeway 91
					pRx->count = 0;
				}
			}
			break;
		case 4:		// SA
			if(pRx->buf[CVNET_FRAME_SA] != CVNET_WALLPAD_ID)		// ���е� �ּҰ� �ƴϸ�
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
			if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
			}
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp1 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Color_Temp_Level.Color_Temp1	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// �ִ� ũ��� ����
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			pG_State->Dimming_Level.Dimming2		= (uint8_t)Dimming_Level&0x0F;
			pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)(Dimming_Level>>4)&0x0F;
			if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
			}
			if((uint8_t)pG_State->Color_Temp_Level.Color_Temp2 > (uint8_t)pG_Config->Color_Temp_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Color_Temp_Level.Color_Temp2	= (uint8_t)pG_Config->Color_Temp_MAX_Level;			// �ִ� ũ��� ����
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
	
	switch(item)	// ������ ������ ������
	{
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 1���� - �ϰ����ܽ���ġ
		case mapping_ITEM_BATCH_LIGHT_OFF:		// �ϰ��ҵ�
			touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
			if(control_value == CVNET_BATCH_LIGHT_SET)	// 0 �ϰ��������� ����(LED ON)
			{
				if(GET_Switch_State(touch_switch))	EventCtrl(touch_switch, OFF);	// �ϰ����� ���ܼ���
			}
			else
			{
				if(GET_Switch_State(touch_switch) == 0)	EventCtrl(touch_switch, ON);	// �ϰ����� ��������
			}
			break;
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_)
		case mapping_ITEM_GAS:					// ��������, ���������� ����ġ ���°� ���� �־�� ������ ������ �����̴�
			touch_switch	= item2tsn((uint8_t)mapping_ITEM_GAS);
			if(control_value == CVNET_GAS_CLOSE)		// 0 �������	 ����(LED ON)
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
			else	//��Ž���(0x0C), ����(0x00)
			{
				;
			}
			break;
#endif
#endif
#if defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
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
			
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
			if((control_value & 0x0F) == CVNET_OFF_FLAG)	// OFF�� ���µ�&���� �������� �ҵ�
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);
			}
			else if(control_value & 0x0F)
			{
				Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
				Gu8_LCD_DIM_Tmr					= 20;		// 2s ���� LCD ǥ��
				Gu8_Dim_Flag = 1;
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);
				SET_DimmingLevel(item, control_value);
			}
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			if(control_value & 0x02)	// �ڵ�,����
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
					PUT_RelayCtrl(item2ctrl(item), Flag);	// �׸���� ����				
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
					PUT_RelayCtrl(item2ctrl(item), Flag);	// �׸���� ����
				}
			}
			
			SET_SWITCH_Delay_OFF_Flag(item, 0);
			SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
			//----------------------------------------------------------------------------------------------------------
			ALL_Electricity_Switch_LED_Ctrl();
			//----------------------------------------------------------------------------------------------------------
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_MODEL_)
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
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
			Gu8_PowerSaving_Tmr			= tmr;	// ����
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
			
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		// case mapping_ITEM_3WAY_1:			//3���� ��� 3�� ���������� ���ؼ� ���� ����
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
			
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				if(pG_Config->Enable_Flag.PWM_Color_Temp)	//�� �µ� ����� ���� ���� �� ���µ� ���� �����ϰ�, �� �µ� ������� ���� ������ ��� ������ ������
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
			
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
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
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit1 �ڵ�/����, bit0 ���� ON/OFF
			{
				ret	= (uint8_t)(CVNET_ON_FLAG | (pG_State->ETC.Auto1<<1));
			}
			else
			{
				ret	= (uint8_t)(CVNET_OFF_FLAG | (pG_State->ETC.Auto1<<1));	// CVNET_OFF_FLAG�� 0�̾ �ǹ̴� ������...
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit1 �ڵ�/����, bit0 ���� ON/OFF
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
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)		// 1����, 2����	- �����

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
				// case mapping_ITEM_3WAY_1:	//3���� ��� 3�� ���������� ���ؼ� ���� ����
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
	if(pRx->buf[CVNET_FRAME_D0] == CVNET_ON_FLAG || pRx->buf[CVNET_FRAME_D0] == CVNET_OFF_FLAG)	//D0 �����Ͱ� ON, OFF �� ���� ����. ��� ������ �� ���� �������� �ʵ��� ��.
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
	
    if(pRx->buf[CVNET_FRAME_DA] == (Get_485_ID() | CVNET_GROUP_ID))	// �׷����� ��巹��(1����, 2���� ����𵨸� �׷����� ����)
    {
    	if(pRx->buf[CVNET_FRAME_CMD] != CMD_GROUP_CONTROL_REQ)				// �׷����� CMD
    	{
    		return;	// ������̶� �׷����� CMD�� �ƴϸ� �׷������ ����
    	}
    }
	if(pRx->buf[CVNET_FRAME_PRE] == CVNET_FRAME_PRE)	//ù ��Ŷ�� 0xF7�� ����
	{
		if((pRx->buf[CVNET_FRAME_DA] & CVNET_LIGHT_DEVICE_ID) != CVNET_LIGHT_DEVICE_ID)	return;	//240322
	}

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	switch(pRx->buf[CVNET_FRAME_CMD])	// CMD
    {
    	case CMD_GROUP_CONTROL_REQ:								// 0x3F	�׷�����	- �������
    		for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// �׸�, ����
    		}
    		break;
    		
    	case CMD_STATE_REQ:										// 0x01	���¿�û
			if(pRx->buf[CVNET_FRAME_DA] == Get_485_ID())
			{
				CVNET_LIGHT_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// ���¿�û �� ������ ���� ���� CMD(0x81)
			}
			else if(pRx->buf[CVNET_FRAME_DA] == Get_Threeway_485_ID())
			{
				if(item2tsn(mapping_ITEM_3WAY_1))
				{
					ThreeWay_Data_Res(pRx);
				}
				//3�� ���� ����
			}
			break;
			
    	case CMD_TOTAL_CONTROL_REQ:								// 0x1F	������ü ����
			for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
			{
				CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// �׸�, ����
			}
			CVNET_LIGHT_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// ���¿�û �� ������ ���� ���� CMD(0x9F)
			break;
			
		case CMD_SELECTIVE_CONTROL_1_REQ:	// 0x11	���� ��������1		���� ��
		case CMD_SELECTIVE_CONTROL_2_REQ:	// 0x12	���� ��������2		���� ��
		case CMD_SELECTIVE_CONTROL_3_REQ:	// 0x13	���� ��������3		���� ��
		case CMD_SELECTIVE_CONTROL_4_REQ:	// 0x14	���� ��������4		���� ��
		case CMD_SELECTIVE_CONTROL_5_REQ:	// 0x15	���� ��������5		���� ��
		case CMD_SELECTIVE_CONTROL_6_REQ:	// 0x16	���� ��������6		���� ��
		case CMD_SELECTIVE_CONTROL_7_REQ:	// 0x17	���� ��������7		���� ��
		case CMD_SELECTIVE_CONTROL_8_REQ:	// 0x18	���� ��������8		���� ��
			if(pRx->buf[CVNET_FRAME_DA] == Get_485_ID())
			{
				item = CVNET_LIGHT_ITEM_Sequence[pRx->buf[CVNET_FRAME_CMD] - CMD_SELECTIVE_CONTROL_1_REQ];
				if(item)
				{
					CVNET_Control(item, pRx->buf[CVNET_FRAME_D0]);		// �׸�, ����
					CVNET_LIGHT_Model_Data_Res(pRx, CMD_SELECTIVE_CONTROL_1_8_RES);	// ��ü����
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
						//3�� ����
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
#endif	// defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)	// 2����	- ����+������
#ifdef _LIGHT_SWITCH_PROTOCOL_USE_	//�ش� define ��� �� ������� ����ġ�� ���� �������� �����
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
				// case mapping_ITEM_3WAY_1:	//3���� ��� 3�� ���������� ���ؼ� ���� ����
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
	if(pRx->buf[CVNET_FRAME_D0] == CVNET_ON_FLAG || pRx->buf[CVNET_FRAME_D0] == CVNET_OFF_FLAG)	//D0 �����Ͱ� ON, OFF �� ���� ����. ��� ������ �� ���� �������� �ʵ��� ��.
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
	
    if(pRx->buf[CVNET_FRAME_DA] == (Get_485_ID() | CVNET_GROUP_ID))	// �׷����� ��巹��(1����, 2���� ����𵨸� �׷����� ����)
    {
    	if(pRx->buf[CVNET_FRAME_CMD] != CMD_GROUP_CONTROL_REQ)				// �׷����� CMD
    	{
    		return;	// ������̶� �׷����� CMD�� �ƴϸ� �׷������ ����
    	}
    }
	if(pRx->buf[CVNET_FRAME_PRE] == CVNET_FRAME_PRE)	//ù ��Ŷ�� 0xF7�� ����
	{
		if((pRx->buf[CVNET_FRAME_DA] & CVNET_LIGHT_DEVICE_ID) != CVNET_LIGHT_DEVICE_ID)	return;	//240322
	}

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	switch(pRx->buf[CVNET_FRAME_CMD])	// CMD
    {
    	case CMD_GROUP_CONTROL_REQ:								// 0x3F	�׷�����	- �������
    		for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// �׸�, ����
    		}
    		break;
    		
    	case CMD_STATE_REQ:										// 0x01	���¿�û
			if(pRx->buf[CVNET_FRAME_DA] == Get_485_ID())
			{
				CVNET_LIGHT_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// ���¿�û �� ������ ���� ���� CMD(0x81)
			}
			else if(pRx->buf[CVNET_FRAME_DA] == Get_Threeway_485_ID())
			{
				if(item2tsn(mapping_ITEM_3WAY_1))
				{
					ThreeWay_Data_Res(pRx);
				}
				//3�� ���� ����
			}
			break;
			
    	case CMD_TOTAL_CONTROL_REQ:								// 0x1F	������ü ����
			for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
			{
				CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// �׸�, ����
			}
			CVNET_LIGHT_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// ���¿�û �� ������ ���� ���� CMD(0x9F)
			break;
			
		case CMD_SELECTIVE_CONTROL_1_REQ:	// 0x11	���� ��������1		���� ��
		case CMD_SELECTIVE_CONTROL_2_REQ:	// 0x12	���� ��������2		���� ��
		case CMD_SELECTIVE_CONTROL_3_REQ:	// 0x13	���� ��������3		���� ��
		case CMD_SELECTIVE_CONTROL_4_REQ:	// 0x14	���� ��������4		���� ��
		case CMD_SELECTIVE_CONTROL_5_REQ:	// 0x15	���� ��������5		���� ��
		case CMD_SELECTIVE_CONTROL_6_REQ:	// 0x16	���� ��������6		���� ��
		case CMD_SELECTIVE_CONTROL_7_REQ:	// 0x17	���� ��������7		���� ��
		case CMD_SELECTIVE_CONTROL_8_REQ:	// 0x18	���� ��������8		���� ��
			if(pRx->buf[CVNET_FRAME_DA] == Get_485_ID())
			{
				item = CVNET_LIGHT_ITEM_Sequence[pRx->buf[CVNET_FRAME_CMD] - CMD_SELECTIVE_CONTROL_1_REQ];
				if(item)
				{
					CVNET_Control(item, pRx->buf[CVNET_FRAME_D0]);		// �׸�, ����
					CVNET_LIGHT_Model_Data_Res(pRx, CMD_SELECTIVE_CONTROL_1_8_RES);	// ��ü����
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
						//3�� ����
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
		case CMD_STATE_RES:						// 0x81	��������(����1, ����2, ����3, ����4, 00, 00, ����1~4, 00
		case CMD_LIGHT_TOTAL_CONTROL_RES:		// 0x9E	���� ��ü���� ����(����1, ����2, ����3, ����4, 00, 00, 00, 00)
			for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
			{
				pTx->buf[pTx->count++]	= CVNET_LIGHT_Data_Conversion(CVNET_LIGHT_ITEM_Sequence[item]);
			}
			if(CMD == CMD_STATE_RES)			// ����ġ ���� ��û�� ���
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
			
		case CMD_ELEC_TOTAL_CONTROL_RES:		// 0x9F	��� ��ü���� ����(���1, ���2, ���3, ���4, 00, 00, 00, 00)
			for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
			{
				pTx->buf[pTx->count++]	= CVNET_ELEC_Data_Conversion(CVNET_ELEC_ITEM_Sequence[item]);
			}
			break;
		
		case CMD_LIGHT_SELECTIVE_CONTROL_1_RES:	// 0x91~0x94 ���� �������� ����(����, 00, 00, 00, 00, 00, 00, 00)
		case CMD_LIGHT_SELECTIVE_CONTROL_2_RES:
		case CMD_LIGHT_SELECTIVE_CONTROL_3_RES:
		case CMD_LIGHT_SELECTIVE_CONTROL_4_RES:
			for(item=0;item<MAX_CVNET_DATA_SEQUENCE;item++)
			{
				pTx->buf[pTx->count++]	= 0x00;
			}
			pTx->buf[CVNET_FRAME_D0]	= CVNET_ELEC_Data_Conversion(CVNET_LIGHT_ITEM_Sequence[CMD - CMD_LIGHT_SELECTIVE_CONTROL_1_RES]);
			break;
		
		case CMD_ELEC_SELECTIVE_CONTROL_1_RES:	// 0x95~0x98 ��� �������� ����(���, 00, 00, 00, 00, 00, 00, 00)
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
    
	if(pRx->buf[CVNET_FRAME_PRE] == CVNET_FRAME_PRE)	//ù ��Ŷ�� 0xF7�� ����
	{
		if((pRx->buf[CVNET_FRAME_DA] & CVNET_LIGHT_ELEC_DEVICE_ID) != CVNET_LIGHT_ELEC_DEVICE_ID)	return;
	}

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
	switch(pRx->buf[CVNET_FRAME_CMD])	// CMD
    {
    	/*
    	case CMD_GROUP_CONTROL_REQ:								// �׷�����	- �������
    		for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// �׸�, ����
    		}
    		break;
    	*/
    	case CMD_STATE_REQ:										// 0x01	���¿�û
			CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG) );	// ���¿�û ���� CMD(0x81)
			break;
			
    	case CMD_LIGHT_TOTAL_CONTROL_REQ:						// 0x1E	���� ��ü����
    		for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_LIGHT_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);	// �׸�, ����
    		}
    		CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG));
			break;
			
		case CMD_ELEC_TOTAL_CONTROL_REQ:						// 0x1F	������� ��ü����
			for(i=0;i<MAX_CVNET_DATA_SEQUENCE;i++)
    		{
    			CVNET_Control(CVNET_ELEC_ITEM_Sequence[i], pRx->buf[CVNET_FRAME_D0+i]);		// �׸�, ����
    		}
    		CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG));
    		break;
    	
		case CMD_LIGHT_SELECTIVE_CONTROL_1_REQ:		// 0x11	���� ��������1		����+������� ��
		case CMD_LIGHT_SELECTIVE_CONTROL_2_REQ:		// 0x12	���� ��������2		����+������� ��
		case CMD_LIGHT_SELECTIVE_CONTROL_3_REQ:		// 0x13	���� ��������3		����+������� ��
		case CMD_LIGHT_SELECTIVE_CONTROL_4_REQ:		// 0x14	���� ��������4		����+������� ��
			item = CVNET_LIGHT_ITEM_Sequence[pRx->buf[CVNET_FRAME_CMD] - CMD_LIGHT_SELECTIVE_CONTROL_1_REQ];
			if(item)
			{
				CVNET_Control(item, pRx->buf[CVNET_FRAME_D0]);		// �׸�, ����
				CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG));
			}
			break;
			
		case CMD_ELEC_SELECTIVE_CONTROL_1_REQ:		// 0x15	��� ��������1		����+������� ��
		case CMD_ELEC_SELECTIVE_CONTROL_2_REQ:		// 0x16	��� ��������2		����+������� ��
		case CMD_ELEC_SELECTIVE_CONTROL_3_REQ:		// 0x17	��� ��������3		����+������� ��
		case CMD_ELEC_SELECTIVE_CONTROL_4_REQ:		// 0x18	��� ��������4		����+������� ��
			item = CVNET_ELEC_ITEM_Sequence[pRx->buf[CVNET_FRAME_CMD] - CMD_ELEC_SELECTIVE_CONTROL_1_REQ];
			if(item)
			{
				CVNET_Control(item, pRx->buf[CVNET_FRAME_D0]);		// �׸�, ����
				CVNET_LIGHT_ELEC_Model_Data_Res(pRx, (uint8_t)(pRx->buf[CVNET_FRAME_CMD] | CVNET_RES_FLAG));
			}
			break;
		case CMD_LIGHT_CHARACTER_REQ:
			CVNET_LIGHT_ELEC_Model_Data_Res(pRx, CMD_LIGHT_CHARACTER_RES);
			break;
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
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
#endif	// defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
#endif
//------------------------------------------------------------------------------------------------------------------------------------------
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1���� - �ϰ����ܽ���ġ
//--------------------------------------�ϰ����� ����ġ ��������-----------------------------------------------------------------------------------------------
#ifdef _BATCH_BLOCK_SWITCH_PROTOCOL_	//�ϰ����� ����ġ �������� ����� ��
//�ϰ����� ����ġ�� ���� ����ġ�� �ٸ� ������ ����ġ�� �ν��ϹǷ�, 3�� ���� Ư�� ������ ���� ������ �ϰ� ���� ����ġ �������� ��� �ϵ��� ������(CVNET ������).
//Cvnet�� ����� ���������� ���������� ȣ�� ���� ��� ���� ������ ���� ���ϴ� ��� �ִ°����� ����.
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
	pTx->buf[pTx->count++]	= CMD;								// ��������(0x81), 	��ü��������(0x9F), ������������(0x9F)
	
	// DATA0~7 = �ϰ�����, �������, 00, 00, 00, 00, 00, 00
	touch_switch	= item2tsn((uint8_t)mapping_ITEM_BATCH_LIGHT_OFF);
	if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))	// �ϰ��ҵ��� ����ġ ���°� ���� �־�� ������ ������ �����̴�
	{
		pTx->buf[pTx->count++]	 = CVNET_BATCH_LIGHT_SET;		// 0 �ϰ��������� ����(LED ON);
	}
	else
	{
		pTx->buf[pTx->count++]	 = CVNET_BATCH_LIGHT_RELEASE;	// 1 �ϰ��������� ����(LED OFF)
	}
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_)
	touch_switch	= item2tsn((uint8_t)mapping_ITEM_GAS);
	if(GET_Switch_State(touch_switch) == ON || (GET_LED_State(touch_switch) == LED_FLASHING))	// ���������� ����ġ ���°� ���� �־�� ������ ������ �����̴�
	{
		pTx->buf[pTx->count++]	 = CVNET_GAS_CLOSE;				// 0 �������	 ����(LED ON)
	}
	else
	{
		pTx->buf[pTx->count++]	 = CVNET_GAS_OPEN;				// 1 ������� ����(LED OFF)
	}
#else		//�ϰ� ����ġ �� �� ���� ���� �� �� ��(ex) �ϰ��ҵ� 1 ��) pTx ���� ī��Ʈ ���� 1 �����ϹǷ� ī��Ʈ 1 ���� ó��
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
	pTx->buf[pTx->count++]	= CMD;								// ��������(0x81), 	��ü��������(0x9F), ������������(0x9F)

	if(pRx->buf[CVNET_FRAME_CMD] == CMD_STATE_REQ)
	{
		if(GET_LED_State(touch_switch) == LED_FLASHING)	//ȣ�� ��û�� or ȣ�����̸�
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
	else if(pRx->buf[CVNET_FRAME_CMD] == CMD_BATCHLIGHT_n_ELEVATOR_CONTROL_REQ)	//����䱸�� ����ġ���� ���������� �� ���� ��쿡�� ���е忡�� ������. DA, SA, CMD ������ �����ʹ� �Ȱ��� �����ϸ� �ȴٰ� ��.
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
    
	if(pRx->buf[CVNET_FRAME_PRE] == CVNET_FRAME_PRE)	//ù ��Ŷ�� 0xF7�� ����
	{
		if(pRx->buf[CVNET_FRAME_DA] != Get_Batch_Block_485_ID() && pRx->buf[CVNET_FRAME_DA] != Get_Elevator_485_ID())	return;
	}

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	switch(pRx->buf[CVNET_FRAME_CMD])	// CMD
    {	
    	case CMD_STATE_REQ:												// 0x01	���¿�û
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
				//���� 20220310 �߰���. ����ġ���� ���� ���� ��û ��, ���е忡�� ���������� �����ߴٴ� �����͸� �����ִ� ���� �������� ����. �ӽ÷� ���� �� ���� �������� ���ϵ��� ��.
				//CVNET�� ���� ���¿� ���� �����ʹ� ���� ��� ��ü ������� �ִ°����� �������� ������ ����. ���� ���� ������ �����ϸ� ��� �����͸� �޾ƾ�����..
				CVNET_BATCH_BLOCK_Data_Res(pRx, CMD_STATE_RES);				// 0x81	���¿�û ���� CMD(0x81)
			}
			break;
			
    	case CMD_TOTAL_CONTROL_REQ:										// 0x1F	��ü����
			if(pRx->buf[CVNET_FRAME_DA] == Get_Batch_Block_485_ID())
			{
				CVNET_Control(mapping_ITEM_BATCH_LIGHT_OFF, pRx->buf[CVNET_FRAME_D0]);	// �ϰ�����
				if(item2tsn(mapping_ITEM_GAS))
				{
					CVNET_Control(mapping_ITEM_GAS, pRx->buf[CVNET_FRAME_D1]);				// ��������
				}
				CVNET_BATCH_BLOCK_Data_Res(pRx, CMD_TOTAL_CONTROL_RES);		// 0x9F	��ü���� ����
			}
			break;
			
		case CMD_BATCHLIGHT_n_ELEVATOR_CONTROL_REQ:								// 0x11	�ϰ����� ����		�ϰ����� ��
		// case CMD_ELEVATOR_CONTROL_REQ:									// 0x11 ���������� ����
			if(pRx->buf[CVNET_FRAME_DA] == Get_Elevator_485_ID())
			{
				if(item2tsn(mapping_ITEM_ELEVATOR))
				{
					// CVNET_Control(mapping_ITEM_ELEVATOR, pRx->buf[CVNET_FRAME_D0]);	//���� �����ʹ� ����ġ���� ���������� ȣ������ ��츸 �����ϹǷ� ���� ������ �޾��� �� ���������� ����ġ ���� �� �ʿ� ����
					if(pRx->buf[CVNET_FRAME_D0] == CVNET_ELEVATOR_ARRIVE)			//���� �����͸� ���������� �������·�
					{
						BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);					//ȣ�� ������ ���� ���� ������
					}
					else if(pRx->buf[CVNET_FRAME_D0] & CVNET_ELEVATOR_TRANS)		//0x0C �������� ����, ��Ž��� �����͸� ���������� �� ����
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)
						{
							BATCH_BLOCK_Control(SET__ELEVATOR_CALL);						//���� �����͸� ������ ����ġ -> ���е���� ���������� ȣ�� �����̹Ƿ� LED ���� ����
						}
					}
					/*else if(pRx->buf[CVNET_FRAME_D0] == CVNET_ELEVATOR_DOWN)		//0x08 ���������� DOWN�� ����
					{
						if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING || GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR)))				//ȣ�� ��û �� ��� Ȯ�� �� ȣ�����϶��� ����. �ٸ�, ����ġ�� ������ ǥ�� �ʿ�� ����.
						{
							BATCH_BLOCK_Control(SET__ELEVATOR_CALL);						//���� �����͸� ������ ����ġ -> ���е���� ���������� ȣ�� �����̹Ƿ� LED ���� ����
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
					CVNET_BATCH_BLOCK_Data_Res(pRx, CMD_TOTAL_CONTROL_RES);		// 0x9F	��ü���� ����
				}
			}
    		break;

		case CMD_GAS_CONTROL_REQ:										// 0x12	���� ����		�ϰ����� ��
			if(pRx->buf[CVNET_FRAME_DA] == Get_Batch_Block_485_ID())
			{
				if(item2tsn(mapping_ITEM_GAS))
				{
					CVNET_Control(mapping_ITEM_GAS, pRx->buf[CVNET_FRAME_D0]);
					CVNET_BATCH_BLOCK_Data_Res(pRx, CMD_TOTAL_CONTROL_RES);		// 0x9F	��ü���� ����
				}
			}
			break;
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.
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
//--------------------------------------���ս���ġ ��������-----------------------------------------------------------------------------------------------
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
			if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))				ret |= (1 << 1);	//3�� ON ����
			else															ret |= (0 << 1);	//3�� OFF ����
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
	//------------------------------------------------------------D0, ����------------------------------------------------------------------------
	pTx->buf[pTx->count++] = 0;
	//------------------------------------------------------------D1, ����/�������� ��û------------------------------------------------------------
	pTx->buf[pTx->count++] = CVNET_Total_Switch_State(CVNET_FRAME_D1);
	//------------------------------------------------------------D2, �ϰ��ҵ�, ��������, ���--------------------------------------------------------------
	pTx->buf[pTx->count++] = CVNET_Total_Switch_State(CVNET_FRAME_D2);
	//------------------------------------------------------------D3, ����-------------------------------------------------------------------------
	pTx->buf[pTx->count++] = 0;
	//------------------------------------------------------------D4, ������� �ܼ�Ʈ/����ġ---------------------------------------------------------
	pTx->buf[pTx->count++] = 0;
	//------------------------------------------------------------D5, D6, D7 ����������-------------------------------------------------------------
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
		pTx->buf[pTx->count++] = pRx->buf[CVNET_FRAME_D6];						//D6, ����������
		pTx->buf[pTx->count++] = pRx->buf[CVNET_FRAME_D7];						//D7, ����������
	}
	//��Ŷ üũ �غ� ��� ���е忡�� ���� ȣ�� ���� ��쿡�� D5(���������� ����) ������ 0���� ��.
	else	//230802, ���������� ������� �ʴ� �𵨿����� ���� ������ �״�� ����(�ӽ�)
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
				if(GET_Switch_State(touch_switch))	EventCtrl(touch_switch, OFF);	// �ϰ����� ���ܼ���
				// BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);	//1001 0000
			}
			else
			{
				if(GET_Switch_State(touch_switch) == 0)	EventCtrl(touch_switch, ON);	// �ϰ����� ���ܼ���
				// BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);	//1000 0000
			}
		}
	
		if(item2tsn(mapping_ITEM_3WAY_1))
		{
			if(Gu8_3Way_Flag)	//���� ����ġ ���� ���� ���� ���� ����.
			{
				/*if(Gu8_3Way_Flag == 0x02 && (pRx->buf[CVNET_FRAME_D2] & 0x02) == 0x02)	//���� �����ؼ� OFF�� ����, ���е忡�� ON ���� ����
				{
					BATCH_BLOCK_Control(SET__THREEWAY_ON);
					printf("OFF -> ON\r\n");
				}
				else if(Gu8_3Way_Flag == 0x01 && (pRx->buf[CVNET_FRAME_D2] & 0x02) == 0x00)	//���� �����ؼ� ON�� ����, ���е忡�� OFF ���� ����
				{
					BATCH_BLOCK_Control(SET__THREEWAY_OFF);
					printf("ON -> OFF\r\n");
				}*/	//240418
				Gu8_3Way_Flag = 0;
			}
			else	//���� ����ġ ���� ���� ����
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
		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//���α׷����� ������ ����ġ ID�� ����ġ�� ID�� �ٸ���
		{
			pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
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
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_GAS)) == LED_FLASHING)		// �������ݿ� ���� ������ ������ OPEN ���·� ��ȯ
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
			}
		}
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))		//ȣ�� ��û or ȣ�� ���� ��
	{
		if(Gu16_Elevator_Tmr == 0)	//CVNET�� ȣ�� �� 3�� �̳� ���� �����Ͱ� ���� �ʾƵ� ���� �ʱ�ȭ �ؾ���.
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
		case SET__GAS_CLOSE_REQUEST:		// ���������� ����ġ ���°� ���� �־�� ������ ������ �����̴�
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) == OFF || GET_LED_State(touch_switch) == LED_FLASHING)	// �������갡 ����(OFF)�ְų�, ���� ��û(LED_FLASHING)���� ���
			{
				Gu16_GAS_Off_Tmr	= 60;		// 60�� ��� �� LED �ҵ�(�� ���е忡�� close/open �����Ͱ� ���ŵǸ� �ش� ���·� ��ȯ)
				SET_Switch_State(touch_switch, OFF);	//20220310 ��û�ÿ��� ����ġ OFF�� �ٲ�.
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
			if(GET_Switch_State(touch_switch) == 0)		//��û���̰ų�, ���� ����  �����϶���,
			{
				SET_Switch_State(touch_switch, ON);		// ������� ����(����)
				SET_LED_State(touch_switch, OFF);		// �����δ� LED ����
				Beep(ON);
			}
			break;
			
		case SET__GAS_OPEN_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(GET_Switch_State(touch_switch) || (GET_LED_State(touch_switch) == LED_FLASHING))			//�������϶���
			{
				SET_Switch_State(touch_switch, OFF);	// ������� ����
				SET_LED_State(touch_switch, ON);		// �����δ� LED ����
				Beep(OFF);
			}
			break;
		case SET__ELEVATOR_REQUEST:																			//����ġ���� ���������� ��
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))		//����ġ OFF �ų� LED ���� Flashing�̸�
			{
				Gu16_Elevator_Tmr = ELEVATOR_TMR;															//CVNET�� ȣ�� �� 3�� �̳� ���� ��ȣ�� ���� ���ϸ� ����ġ �ʱ�ȭ 
				SET_Switch_State(touch_switch, OFF);														//����ġ OFF
				SET_LED_State(touch_switch, LED_FLASHING);													//LED Flashing
				Beep(ON);
				// printf("ELEVATOR REQEUST\r\n");
			}
			break;			
		case SET__ELEVATOR_CALL:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == 0)			//ȣ�� ��û ���°ų�, ȣ����� �ƴ� ��.
			{
				Gu16_Elevator_Tmr = ELEVATOR_TMR;															//CVNET�� ȣ�� �� 3�� �̳� ���� ��ȣ�� ���� ���ϸ� ����ġ �ʱ�ȭ ��.
				SET_Switch_State(touch_switch, ON);															//����ġ ON
				SET_LED_State(touch_switch, OFF);															//LED ON
				Beep(ON);
			}
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
#else		//��Ż��X, �Լ����O, ����ġ ���� ��� ���̾� ���� �� ���
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

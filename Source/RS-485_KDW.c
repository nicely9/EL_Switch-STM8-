/************************************************************************************
	Project		: ���ڽ� ����ġ
	File Name	: RS-485_KDW.C
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

#ifdef _KDW_PROTOCOL_

/*#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// �ϰ����ܽ���ġ
void BATCH_BLOCK_STATE_Process(void);
#endif*/

void KDW_Data_Process(KDW_BUF	*pRx);
uint8_t KDW_Batch_Light_State(uint8_t item);
void BATCH_BLOCK_Control(uint8_t control);
// ----------------------------------------------------------------------------------------
static	KDW_BUF		RxBuf, TxBuf;
#define ELEVATOR_TMR	7		//
#define GAS_TIME_OUT 	7		 //�������� �� 5�ʰ� ǥ��������, 7�ʷ� ��.

#define	MAX_KDW_DATA_SEQUENCE		8
uint8_t	KDW_LIGHT_ITEM_Sequence[MAX_KDW_DATA_SEQUENCE];
uint8_t	KDW_ELEC_ITEM_Sequence[MAX_KDW_DATA_SEQUENCE];
uint8_t Store_Light_State[MAX_KDW_DATA_SEQUENCE];
// uint8_t Gu8_Batch_Light_Flag;
uint8_t Gu8_OFF_Repeat_Tmr = 0;
uint8_t Gu8_ON_Repeat_Tmr = 0;
uint8_t Gu8_Elec_OFF_Repeat_Tmr = 0;
uint8_t Gu8_Elec_ON_Repeat_Tmr = 0;
// ----------------------------------------------------------------------------------------
void SET_KDW_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_KDW_DATA_SEQUENCE)
	{
		KDW_LIGHT_ITEM_Sequence[count]	= item;
	}
}

void SET_KDW_ELEC_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_KDW_DATA_SEQUENCE)
	{
		KDW_ELEC_ITEM_Sequence[count]	= item;
	}
}

void Protocol_Data_Init(void)
{
	uint8_t	i, count;
	
	memset((void*)&RxBuf,		0,	sizeof(KDW_BUF));
	memset((void*)&TxBuf,		0,	sizeof(KDW_BUF));
	
	Gu16_GAS_Off_Tmr				= 0;
	
	Gu16_Elevator_Tmr			= 0;
	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_NON;
	// Gu8_Batch_Light_Flag		= 0;

	memset(KDW_LIGHT_ITEM_Sequence, 0, MAX_KDW_DATA_SEQUENCE);
	memset(KDW_ELEC_ITEM_Sequence, 0, MAX_KDW_DATA_SEQUENCE);
	
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_
	count	= 0;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
#endif
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_
	count	= 0;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_KDW_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
#endif
	for(i = 0; i < MAX_KDW_DATA_SEQUENCE; i++)
	{
		Store_Light_State[i]	= KDW_Batch_Light_State(KDW_LIGHT_ITEM_Sequence[i]);
	}	
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// ������� ����?�� 
	count	= 0;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_KDW_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_KDW_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
#endif
}
// ----------------------------------------------------------------------------------------
void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr)
{
	if(tmr == 0)		// ����??���� ??���� ��?��? ?? X ms �ϰ�??��? ??���� ??�����?
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
					printf("TX(KDW) : ");
					for(i=0;i<TxBuf.count;i++)
					{
						printf("%02X ", (uint16_t)TxBuf.buf[i]);
					}
					printf("\n");
				}
				Gu8_RS_485_Tx_Tmr	= 2;
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
uint8_t KDW_Crc(KDW_BUF *pTRx, uint16_t len, uint8_t select)
{
	uint8_t i, crc = 0;
	
    if(select == XOR_SUM)
    {
        for(i = KDW_FRAME_HD; i < len; i++)
        {
            crc ^= pTRx->buf[i];
        }
    }
    else if(select == ADD_SUM)
    {
        for(i = KDW_FRAME_HD; i < len; i++)
        {
            crc += pTRx->buf[i];
        }
    }
	return crc;
}
uint8_t NIS_Crc(KDW_BUF *pTRx, uint8_t cal, uint8_t sel)		//����� 485��� �׽�Ʈ�� ���ؼ� �߰���.
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

uint8_t Get_485_ID(void)
{
	return	(uint8_t)(pG_Config->RS485_ID);
}
// ----------------------------------------------------------------------------------------
void RS_485_ID_RES(void)
{
	uint8_t Elec_Watt_MSB, Elec_Watt_LSB, prime_num_1, prime_num_2 = 0;
	KDW_BUF	*pTx;
	pTx = &TxBuf;
	
	prime_num_1 = (uint8_t)((Gu16_Elec_1_Watt - (int)Gu16_Elec_1_Watt) * 10);
	prime_num_2	= (uint8_t)((Gu16_Elec_2_Watt - (int)Gu16_Elec_2_Watt) * 10);
	
	Elec_Watt_MSB = (uint8_t)((int)Gu16_Elec_1_Watt + (int)Gu16_Elec_2_Watt);
	Elec_Watt_LSB = (uint8_t)(prime_num_1 + prime_num_2);

	if(10 <= (prime_num_1 + prime_num_2))
	{
		Elec_Watt_MSB += 1;
		Elec_Watt_LSB -+ 10;
	}

	pTx->buf[pTx->count++]	= 0xCC;
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//�������� Ÿ��
    pTx->buf[pTx->count++]	= Get_485_ID();	
	pTx->buf[pTx->count++]	= 0;

	pTx->buf[pTx->count++]	= Elec_Watt_MSB;				//���°�
	pTx->buf[pTx->count++]	= Elec_Watt_LSB;				//���°�

	pTx->buf[pTx->count++]	= Gu8_ZeroCrossing_Err_Flag;	//����ũ�ν� ���� ���� 1�̸� Err, 0�̸� Pass

	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 1, NIS_TX);
	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 0, NIS_TX);
	TxBuf.send_flag	= 1;
}

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

uint8_t Color_Temp_Cnt(void)
{
	uint8_t count = 0;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1) || item2tsn(mapping_ITEM_DIMMING_LIGHT_2))
	{
		if(pG_Config->Enable_Flag.PWM_Color_Temp)	count++;
	}
	return count;
}

uint8_t Elec_Cnt(void)		//���� �� ī��Ʈ
{
	uint8_t count = 0;
	
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	count++;
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	count++;
	
	return count;
}
// ----------------------------------------------------------------------------------------
void Protocol_Process(uint8_t data)
{
	KDW_BUF	*pRx;
	uint8_t		crc, xor_crc = 0;
	uint8_t     cnt = 0;
	int i;
	
	pRx = &RxBuf;
	
	if(G_Debug == DEBUG_HOST_REALDATA)
	{
		if(data == KDW_HEADER)	printf("\n");
		printf("%02X ", (uint16_t)data);
	}
	
// #ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_
/*#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
	BATCH_BLOCK_STATE_Process();
#endif*/
	
	switch(pRx->count)
	{
		default:
			if((pRx->buf[KDW_FRAME_HD] != KDW_HEADER) && (data == KDW_HEADER))
			{
				pRx->count = 0;
			}
			break;
		case 1:		// HD
			if((pRx->buf[KDW_FRAME_HD] != KDW_HEADER) && (pRx->buf[KDW_FRAME_HD] != NIS_LIGHT_ID_COMM_1))
			{
				pRx->count = 0;
			}
			break;
		case 2:		// DEV_ID
			if((pRx->buf[KDW_FRAME_DEV_ID] != KDW_LIGHT_DEVICE_ID) && (pRx->buf[KDW_FRAME_DEV_ID] != KDW_ELEC_DEVICE_ID) && (pRx->buf[KDW_FRAME_DEV_ID] != KDW_BATCH_BLOCK_DEVICE_ID) && (pRx->buf[KDW_FRAME_DEV_ID] != NIS_LIGHT_ID_COMM_2))
			{
				pRx->count = 0;
			}
			break;
#if defined(_ONE_SIZE_BATCH_BLOCK_MODEL_)
		case 3:
			if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_BATCH_BLOCK_DEVICE_ID)
			{
				if(pRx->buf[KDW_FRAME_DEV_SUB_ID] != Get_485_ID() && (pRx->buf[KDW_FRAME_DEV_SUB_ID] != 0x0F) && (pRx->buf[KDW_FRAME_DEV_SUB_ID] != 0xFF))
				{
					if((pRx->buf[KDW_FRAME_HD] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[KDW_FRAME_DEV_ID] != NIS_LIGHT_ID_COMM_2))
					{
						pRx->count = 0;
					}
				}
			}
			else
			{
				if((pRx->buf[KDW_FRAME_HD] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[KDW_FRAME_DEV_ID] != NIS_LIGHT_ID_COMM_2))
				{
					pRx->count = 0;
				}
			}
			break;
		case 4:
			if(pRx->buf[KDW_FRAME_DEV_SUB_ID] == 0x0F || pRx->buf[KDW_FRAME_DEV_SUB_ID] == 0xFF)
			{
				if(pRx->buf[KDW_FRAME_CMD_TYPE] != KDW_ALL_CONTROL_REQ)
				{
					pRx->count = 0;	//�ϰ����� ��ü ���� Ŀ�ǵ尡 �ƴѰ�� ���� id�� 0x0F, 0xFF�� ī��Ʈ �ʱ�ȭ
				}
			}
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case 3:		// DEV_SUB_ID
			if(((pRx->buf[KDW_FRAME_DEV_SUB_ID] >> 4) != Get_485_ID()) && (pRx->buf[KDW_FRAME_DEV_SUB_ID] != 0xFF))	//�׷� ID�� ����ġ ID�� �ٸ� ��, �ϰ� �ҵ� ����� SUB ID�� �ƴ� ��
			{
				if((pRx->buf[KDW_FRAME_HD] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[KDW_FRAME_DEV_ID] != NIS_LIGHT_ID_COMM_2))
				{
					pRx->count = 0;
				}
			}
			else	//�׷� ID�� ����ġ ID�� �����ϰų� SUB_ID�� 0xFF�� ��
			{
				if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)
				{
					if(((pRx->buf[KDW_FRAME_DEV_SUB_ID] >> 4) == Get_485_ID()) && ((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) != 0x0F))
					{
						if(((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) > Light_Cnt()) || ((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == 0x00))	//�׷� ID�� ����ġ ID�� ������ �� ȸ�� ��ȣ�� �ش� ����ġ ���� �� ���� ������
						{
							pRx->count = 0;
						}
					}
				}
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
				else if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_ELEC_DEVICE_ID)
				{
					if(((pRx->buf[KDW_FRAME_DEV_SUB_ID] >> 4) == Get_485_ID()) && ((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) != 0x0F))
					{
						if(((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) > Elec_Cnt()) || ((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == 0x00))
						{
							pRx->count = 0;
						}
					}
				}
#endif
				else
				{
					if((pRx->buf[KDW_FRAME_HD] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[KDW_FRAME_DEV_ID] != NIS_LIGHT_ID_COMM_2))
					{
						pRx->count = 0;
					}
				}
			}
			break;
		case 4:
			if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)				//���� ����̽�
			{
				if(pRx->buf[KDW_FRAME_DEV_SUB_ID] == 0xFF)						//���� ID 0xFF�� ��
				{
					if((pRx->buf[KDW_FRAME_CMD_TYPE] != KDW_BATCHLIGHT_CONTROL_REQ) && (pRx->buf[KDW_FRAME_CMD_TYPE] != KDW_ALL_CONTROL_REQ))	//�ϰ� �ҵ�/���� ���� Ŀ�ǵ�, ��ü ���� ���� �䱸 Ŀ�ǵ尡 �ƴϸ� �ƴϸ�
					{
						pRx->count = 0;		
					}
				}
			}
			break;
#endif
	}
	pRx->buf[pRx->count++] = data;
	
	if((pRx->buf[KDW_FRAME_HD] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[KDW_FRAME_DEV_ID] == NIS_LIGHT_ID_COMM_2))
	{
		if(pRx->count >= 8)
		{
			crc = NIS_Crc(pRx, 0, NIS_RX);
			xor_crc = NIS_Crc(pRx, 1, NIS_RX);
			if(crc == pRx->buf[7] && xor_crc == pRx->buf[6])
			{
				if(G_Debug == DEBUG_HOST)
				{
					printf("\nRX(KDW) : ");
					for(i=0;i<pRx->count;i++)
					{
						printf("%02X ", (uint16_t)pRx->buf[i]);
					}
					printf("\n");
				}
				KDW_Data_Process(pRx);				
			}
			else
			{
				if(xor_crc != pRx->buf[6])	printf("cal xor[0x%02X] != buf xor[0x%02X]", (uint16_t)xor_crc, (uint16_t)pRx->buf[6]);
				if(crc != pRx->buf[7])		printf("cal sum[0x%02X] != buf sum[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[7]);
			}
			pRx->buf[0] = 0;
			pRx->count = 0;			
		}
	}
	else
	{
		// printf("KDW count = %d", (uint16_t)pRx->count);
		if(pRx->count >= KDW_MAX_BUF)
		{
			pRx->buf[0] = 0;
			pRx->count = 0;
        }
		if(pRx->count == KDW_FRAME_DATA_0)
		{
			pRx->length.value = pRx->buf[KDW_FRAME_LENGTH];
		}
		cnt = (uint8_t)(pRx->length.value + 7);

		if(pRx->count == cnt)
		{
			// printf("MAX BUF\r\n");
			xor_crc = KDW_Crc(pRx, pRx->count-2, XOR_SUM);	//xor ���ܻ��� xor �ƨ�
			crc		= KDW_Crc(pRx, pRx->count-1, ADD_SUM);	//xor��??? add �ƨ�
			if(crc == pRx->buf[pRx->count-1] && xor_crc == pRx->buf[pRx->count-2])
			{
				if(G_Debug == DEBUG_HOST)
				{
					printf("\nRX(KDW) : ");
					for(i=0;i<pRx->count;i++)
					{
						printf("%02X ", (uint16_t)pRx->buf[i]);
					}
					printf("\n");
				}
				KDW_Data_Process(pRx);
			}
			else
			{
				// printf("cal xor_crc[0x%02X] != buf xor_crc[0x%02X]", (uint16_t)xor_crc, (uint16_t)pRx->buf[pRx->count-2]);
				// printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[pRx->count-1]);
				if(xor_crc != pRx->buf[6] || crc != pRx->buf[7])
				{
					printf("xor[0x%02X], sum[0x%02X]\r\n", (uint16_t)xor_crc, (uint16_t)crc);
				}
			}
			pRx->buf[0] = 0;
			pRx->count = 0;
		}
	}
}

uint8_t KDW_Batch_Light_State(uint8_t item)		//�ϰ� �ҵ� �� ���� ���� ���� ����.
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
				ret = KDW_ON_FLAG;
			}
			else
			{
				ret = KDW_OFF_FLAG;
			}
			break;
	}
	return ret;
}

void KDW_BatchLight_Control(uint8_t item, uint8_t control_value)
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
			if(control_value == KDW_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == KDW_ON_FLAG)
				{
					EventCtrl(item2tsn(item), OFF);						//���� OFF
				}
			}
			else if(control_value == KDW_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == KDW_OFF_FLAG)
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

#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
void SET_DimmingLevel(uint8_t item, uint8_t Dimming_Level)	//���� ����(0x41)���� ����ϴ� ��� ����
{
	switch(item)
	{
		case mapping_ITEM_DIMMING_LIGHT_1:
			pG_State->Dimming_Level.Dimming1		= (uint8_t)Dimming_Level & 0x0F;
			if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_1), ON);
			Beep(ON);
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:
			pG_State->Dimming_Level.Dimming2		= (uint8_t)Dimming_Level & 0x0F;
			if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;
			}
			PUT_PWMCtrl(item2ctrl(mapping_ITEM_DIMMING_LIGHT_2), ON);
			Beep(ON);
			break;
	}
}

void KDW_Light_Control(uint8_t item, uint8_t control_value)
{
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
			if(control_value & 0x01)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);
			}
			else
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);
			}
			Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(control_value & 0x01)
			{
				if(control_value & 0xF0)	//bit7 ~ bit4�� ��� ����
				{
					if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);	//bit0�� 1�̸� ���� ����, 0�̸� ���� ����
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
					Gu8_LCD_DIM_Tmr					= 20;
					SET_DimmingLevel(item, (uint8_t)(control_value >> 4));
				}
				else
				{
					if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);	//bit0�� 1�̸� ���� ����, 0�̸� ���� ����
				}
			}
			else
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);
			}
			Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
			break;
	}
}

void KDW_Dimming_Control(uint8_t item, uint8_t data)	//��� ���� ����(0x52)���� ����ϴ� ��� ����
{
	uint8_t control = 0;

	control = (uint8_t)(data & 0x03);	//bit0, bit1����  ?����?
	if(GET_Switch_State(item2tsn(item)))	//�ش� ���� �������� ���� ����
	{
		if(item == mapping_ITEM_DIMMING_LIGHT_1)
		{
			Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
			if(control == KDW_DIMMING_UP)
			{
				if(pG_State->Dimming_Level.Dimming1 == pG_Config->Dimming_MAX_Level)
				{
					pG_State->Dimming_Level.Dimming1	= (uint8_t)1;
				}
				else
				{
					pG_State->Dimming_Level.Dimming1++;
				}
				Gu8_LCD_DIM_Tmr					= 50;		// 5s ���� LCD ǥ��
				Beep(ON);
			}
			else if(control == KDW_DIMMING_DOWN)
			{ 
				if(pG_State->Dimming_Level.Dimming1)	pG_State->Dimming_Level.Dimming1--;
				if((uint8_t)pG_State->Dimming_Level.Dimming1 == (uint8_t)0)
				{
					pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;
				}
				Gu8_LCD_DIM_Tmr					= 50;		// 5s ���� LCD ǥ��
				Beep(ON);
			}
		}
		else if(item == mapping_ITEM_DIMMING_LIGHT_2)
		{
			Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
			if(control == KDW_DIMMING_UP)
			{
				if(pG_State->Dimming_Level.Dimming2 == pG_Config->Dimming_MAX_Level)
				{
					pG_State->Dimming_Level.Dimming2	= (uint8_t)1;
				}
				else
				{
					pG_State->Dimming_Level.Dimming2++;
				}
				Gu8_LCD_DIM_Tmr					= 50;		// 5s ���� LCD ǥ��
				Beep(ON);
			}
			else if(control == KDW_DIMMING_DOWN)
			{ 
				if(pG_State->Dimming_Level.Dimming2)	pG_State->Dimming_Level.Dimming2--;
				if((uint8_t)pG_State->Dimming_Level.Dimming2 == (uint8_t)0)
				{
					pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;
				}
				Gu8_LCD_DIM_Tmr					= 50;		// 5s ���� LCD ǥ��
				Beep(ON);
			}
		}
	}
}

void KDW_Color_Temp_Control(uint8_t item, uint8_t control_value)
{
	if(pG_Config->Enable_Flag.PWM_Color_Temp)
	{
		if(GET_Switch_State(item2tsn(item)))
		{
			if(item == mapping_ITEM_DIMMING_LIGHT_1)
			{
				pG_State->Color_Temp_Level.Color_Temp1 = control_value;
				if(pG_State->Color_Temp_Level.Color_Temp1 > pG_Config->Color_Temp_MAX_Level)
				{
					pG_State->Color_Temp_Level.Color_Temp1 = pG_Config->Color_Temp_MAX_Level;
				}
				else if(pG_State->Color_Temp_Level.Color_Temp1 <= 0)
				{
					pG_State->Color_Temp_Level.Color_Temp1 = 1;
				}
				Beep(ON);
			}
			else if(item == mapping_ITEM_DIMMING_LIGHT_2)
			{
				pG_State->Color_Temp_Level.Color_Temp2 = control_value;
				if(pG_State->Color_Temp_Level.Color_Temp2 > pG_Config->Color_Temp_MAX_Level)
				{
					pG_State->Color_Temp_Level.Color_Temp2 = pG_Config->Color_Temp_MAX_Level;
				}
				else if(pG_State->Color_Temp_Level.Color_Temp2 <= 0)
				{
					pG_State->Color_Temp_Level.Color_Temp2 = 1;
				}
				Beep(ON);
			}
		}
	}
}

void KDW_Light_All_Control(KDW_BUF *pRx, uint8_t control_value)
{
	uint8_t i = 0;
	static uint8_t Batch_Light_Flag = 0;

	if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_ALL_CONTROL_REQ)
	{
		if(control_value == KDW_ON_FLAG)
		{
			for(i = 0; i < MAX_KDW_DATA_SEQUENCE; i++)
			{
				KDW_Light_Control(KDW_LIGHT_ITEM_Sequence[i], KDW_ON_FLAG);
			}
		}
		else if(control_value == KDW_OFF_FLAG)
		{
			for(i = 0; i < MAX_KDW_DATA_SEQUENCE; i++)
			{
				KDW_Light_Control(KDW_LIGHT_ITEM_Sequence[i], KDW_OFF_FLAG);
			}
		}
	}
	else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_BATCHLIGHT_CONTROL_REQ)
	{
		if(control_value == KDW_OFF_FLAG)			//
		{
			if(Batch_Light_Flag != 0x01)
			{
				if(Gu8_OFF_Repeat_Tmr == 0)
				{
					for(i = 0; i < MAX_KDW_DATA_SEQUENCE; i++)
					{
						Store_Light_State[i] = KDW_Batch_Light_State(KDW_LIGHT_ITEM_Sequence[i]);
						printf("%d ", (uint16_t)Store_Light_State[i]);
					}
					for(i = 0; i < MAX_KDW_DATA_SEQUENCE; i++)
					{
						KDW_Light_Control(KDW_LIGHT_ITEM_Sequence[i], KDW_OFF_FLAG);
					}
					printf("BATCH LIGHT OFF\r\n");
					Gu8_OFF_Repeat_Tmr = 5;
				}
				Batch_Light_Flag = 0x01;
			}
		}
		else if(control_value == KDW_ON_FLAG)		//
		{
			if(Batch_Light_Flag != 0x02)
			{
				if(Gu8_ON_Repeat_Tmr == 0)
				{
					for(i = 0; i < MAX_KDW_DATA_SEQUENCE; i++)
					{
						if(Store_Light_State[i] == KDW_ON_FLAG)
						{
							KDW_Light_Control(KDW_LIGHT_ITEM_Sequence[i], KDW_ON_FLAG);
						}
						/*else
						{
							KDW_Light_Control(KDW_LIGHT_ITEM_Sequence[i], KDW_OFF_FLAG);
						}*/
					}
					printf("BATCH LIGHT ON\r\n");
					Gu8_ON_Repeat_Tmr = 5;	//500ms ���� 3ȸ ���� �� �߰� ���� 2ȸ ���͹� �� 200ms��� ��. �˳��� 500ms
				}
				Batch_Light_Flag = 0x02;
			}
		}
	}
}

uint8_t KDW_Color_Temp_Check(void)
{
	uint8_t item, Sub_ID = 0;
	uint8_t ret = 0;

	for(item=0; item < MAX_KDW_DATA_SEQUENCE; item++)
	{
		switch(KDW_LIGHT_ITEM_Sequence[item])
		{
			default:
				ret = KDW_OFF_FLAG;
				break;
			case mapping_ITEM_LIGHT_1:
			case mapping_ITEM_LIGHT_2:
			case mapping_ITEM_LIGHT_3:
			case mapping_ITEM_LIGHT_4:
			case mapping_ITEM_LIGHT_5:
			case mapping_ITEM_LIGHT_6:
			case mapping_ITEM_3WAY_1:
			case mapping_ITEM_3WAY_2:
				if(pG_Config->Enable_Flag.PWM_Color_Temp)	Sub_ID++;
				break;
			case mapping_ITEM_DIMMING_LIGHT_1:
			case mapping_ITEM_DIMMING_LIGHT_2:
				if(pG_Config->Enable_Flag.PWM_Color_Temp)	Sub_ID++;
				return Sub_ID;
		}
	}
	return Sub_ID;
}

uint8_t KDW_3way_Check(void)
{
	uint8_t item, Sub_ID = 0;

	for(item=0; item < MAX_KDW_DATA_SEQUENCE; item++)
	{
		switch(KDW_LIGHT_ITEM_Sequence[item])
		{
			case mapping_ITEM_LIGHT_1:
			case mapping_ITEM_LIGHT_2:
			case mapping_ITEM_LIGHT_3:
			case mapping_ITEM_LIGHT_4:
			case mapping_ITEM_LIGHT_5:
			case mapping_ITEM_LIGHT_6:
			case mapping_ITEM_DIMMING_LIGHT_1:
			case mapping_ITEM_DIMMING_LIGHT_2:			
				if(pG_Config->Enable_Flag.ThreeWay)	Sub_ID++;
				break;
			case mapping_ITEM_3WAY_1:
			case mapping_ITEM_3WAY_2:
				if(pG_Config->Enable_Flag.ThreeWay)	Sub_ID++;
				return Sub_ID;
		}
	}
	return Sub_ID;	
}

uint8_t KDW_LIGHT_Character_Color_Temp_Flag(void)
{
	uint8_t i, shift = 0, ret = 0;
	
	for(i = 0; i < MAX_KDW_DATA_SEQUENCE; i++)
	{
		switch(KDW_LIGHT_ITEM_Sequence[i])
		{
			case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
			case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
			case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
			case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
			case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
			case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
			case mapping_ITEM_3WAY_1:
			case mapping_ITEM_3WAY_2:
				ret |= (uint8_t)(0 << shift);
				shift++;
				break;
			case mapping_ITEM_DIMMING_LIGHT_1:
			case mapping_ITEM_DIMMING_LIGHT_2:
				if(pG_Config->Enable_Flag.PWM_Color_Temp)
				{
					ret |= (uint8_t)(1 << shift);
				}
				else
				{
					ret |= (uint8_t)(0 << shift);
				}
				shift++;
				break;
		}
	}
	return ret;
}

uint8_t KDW_LIGHT_Character_Flag(void)
{
	uint8_t i, shift = 0, ret = 0;

	for(i = 0; i < MAX_KDW_DATA_SEQUENCE; i++)
	{
		switch(KDW_LIGHT_ITEM_Sequence[i])
		{
			case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
			case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
			case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
			case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
			case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
			case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
			case mapping_ITEM_3WAY_1:
			case mapping_ITEM_3WAY_2:
				ret |= (uint8_t)(0 << shift);
				shift++;
				break;
			case mapping_ITEM_DIMMING_LIGHT_1:
			case mapping_ITEM_DIMMING_LIGHT_2:
				ret |= (uint8_t)(1 << shift);
				shift++;
				break;
		}
	}
	return ret;
}

uint8_t KDW_LIGHT_Data_Conversion(uint8_t item)
{
	uint8_t	ret = 0;
	
	switch(item)
	{
		default:
			ret = KDW_OFF_FLAG;
			break;
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
		// case mapping_ITEM_DIMMING_LIGHT_1:
		// case mapping_ITEM_DIMMING_LIGHT_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				ret = KDW_ON_FLAG;
			}
			else
			{
				ret = KDW_OFF_FLAG;
			}
			break;
			
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				ret = (uint8_t)((pG_State->Dimming_Level.Dimming1 << 4) | 1);	//��� ��� �ְ� ON�̸� bit7 ~ bit4������ ��� ����, OFF�� 0.  bit0�� 1:����, 0:����
			}
			else
			{
				ret = KDW_OFF_FLAG;												//��� ��� �־ OFF���¸� bit7 ~ bit4�� 0x0.
			}
			ret |= (1 << 1);	//��� ���� ��� ����
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				ret = (uint8_t)((pG_State->Dimming_Level.Dimming2 << 4) | 1);
			}
			else
			{
				ret = KDW_OFF_FLAG;
			}
			ret |= (1 << 1);	//��� ���� ��� ����
			break;
	}
	return	ret;
}

uint8_t KDW_ELEC_Data_Conversion(uint8_t item)
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
				ret	= (uint8_t)(KDW_ON_FLAG | (pG_State->ETC.Auto1<<1));
			}
			else
			{
				ret	= (uint8_t)(KDW_OFF_FLAG | (pG_State->ETC.Auto1<<1));
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit1 �ڵ�/����, bit0 ���� ON/OFF
			{
				ret	= (uint8_t)(KDW_ON_FLAG | (pG_State->ETC.Auto2<<1));
			}
			else
			{
				ret	= (uint8_t)(KDW_OFF_FLAG | (pG_State->ETC.Auto2<<1));
			}
			break;
	}
	
	return	ret;
}

void KDW_LIGHT_Model_Data_Res(KDW_BUF *pRx)
{
	uint8_t	item;
	KDW_BUF	*pTx;
	pTx = &TxBuf;
	if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == 0x0F)
	{
		pTx->count	= KDW_FRAME_HD;											
		pTx->buf[pTx->count++]	= KDW_HEADER;								//HEADER
		pTx->buf[pTx->count++]	= KDW_LIGHT_DEVICE_ID;						//DEVICE ID
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];			//DEVICE SUB ID
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE
		
		pTx->buf[pTx->count++]	= (uint8_t)(1 + Light_Cnt() + Color_Temp_Cnt());	//LENGTH, ���� �� + ���µ� ���� �� + 1
		pTx->buf[pTx->count++]	= 0;	//DATA0
		for(item = 0; item < Light_Cnt(); item++)
		{
			pTx->buf[pTx->count++]	= KDW_LIGHT_Data_Conversion(KDW_LIGHT_ITEM_Sequence[item]);		//DATA1 ~ DATAn
		}
		
		if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_1)
		{
			pTx->buf[pTx->count++]	= (uint8_t)((pG_State->Color_Temp_Level.Color_Temp1 << 4) | KDW_Color_Temp_Check());	//DATAn+1
		}
		else if(pG_Config->Enable_Flag.PWM_Color_Temp & ENABLE_BIT_COLOR_TEMP_2)
		{
			pTx->buf[pTx->count++]	= (uint8_t)((pG_State->Color_Temp_Level.Color_Temp2 << 4) | KDW_Color_Temp_Check());
		}
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);		//XOR_SUM
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);		//ADD_SUM
		TxBuf.send_flag	= 1;
	}
}

void KDW_LIGHT_Model_Control_Res(KDW_BUF *pRx)
{
	uint8_t item, data = 0;
	KDW_BUF	*pTx;
	pTx = &TxBuf;

	pTx->count	= KDW_FRAME_HD;											
	pTx->buf[pTx->count++]	= KDW_HEADER;											//HEADER
	pTx->buf[pTx->count++]	= KDW_LIGHT_DEVICE_ID;									//DEVICE ID
	pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];						//DEVICE SUB ID
	pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE
	pTx->buf[pTx->count++]	= 0x02;													//LENGTH
	pTx->buf[pTx->count++]	= 0x00;													//DATA0, ���� ����
	
	item = KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1];
	if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_SELECT_CONTROL_REQ || pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_DIMMING_CONTROL_REQ)
	{
		switch(item)
		{
			default:
				data = 0;
				break;
			case mapping_ITEM_LIGHT_1:
			case mapping_ITEM_LIGHT_2:
			case mapping_ITEM_LIGHT_3:
			case mapping_ITEM_LIGHT_4:
			case mapping_ITEM_LIGHT_5:
			case mapping_ITEM_LIGHT_6:
			case mapping_ITEM_3WAY_1:
			case mapping_ITEM_3WAY_2:
				data = KDW_LIGHT_Data_Conversion(item);
				break;
			case mapping_ITEM_DIMMING_LIGHT_1:
				data |= (uint8_t)(pG_State->Dimming_Level.Dimming1 << 4);
				data |= (1 << 1);
				data |= KDW_LIGHT_Data_Conversion(item);
				break;
			case mapping_ITEM_DIMMING_LIGHT_2:	
				data |= (uint8_t)(pG_State->Dimming_Level.Dimming2 << 4);	//bit7 ~ bit4, ��� ����
				data |= (1 << 1);											//bit1, ��� ��� ����
				data |= KDW_LIGHT_Data_Conversion(item);						//���� On/Off ����
				break;
		}
	}
	else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_COLOR_TEMP_CONTROL_REQ)
	{
		if(item == mapping_ITEM_DIMMING_LIGHT_1)		data = pG_State->Color_Temp_Level.Color_Temp1;
		else if(item == mapping_ITEM_DIMMING_LIGHT_2)	data = pG_State->Color_Temp_Level.Color_Temp2;
	}
	pTx->buf[pTx->count++]	= data;										//DATA1
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);		//XOR_SUM
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);		//ADD_SUM
	TxBuf.send_flag	= 1;	
}

void KDW_Color_Temp_Control_Res(KDW_BUF *pRx)
{
	uint8_t item, data = 0;
	KDW_BUF	*pTx;
	pTx = &TxBuf;
	if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) ==  KDW_Color_Temp_Check())			//���µ� ���� ��ȣ�� ���� �� ���µ� ���� ���� ��ȣ�� �����Ҷ��� ����.
	{
		item = KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1];

		pTx->count	= KDW_FRAME_HD;											
		pTx->buf[pTx->count++]	= KDW_HEADER;											//HEADER
		pTx->buf[pTx->count++]	= KDW_LIGHT_DEVICE_ID;									//DEVICE ID
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];						//DEVICE SUB ID
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE
		pTx->buf[pTx->count++]	= 0x02;													//LENGTH
		pTx->buf[pTx->count++]	= 0x00;
		switch(item)
		{
			case mapping_ITEM_DIMMING_LIGHT_1:
				data = pG_State->Color_Temp_Level.Color_Temp1;
				break;
			case mapping_ITEM_DIMMING_LIGHT_2:
				data = pG_State->Color_Temp_Level.Color_Temp2;
				break;
		}
		pTx->buf[pTx->count++]	= data;										//DATA1
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);		//XOR_SUM
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);		//ADD_SUM
		TxBuf.send_flag	= 1;
	}
}

uint8_t KDW_LIGHT_Model_Select_Character_Data(uint8_t item, uint8_t Sel)
{
	uint8_t ret = 0;

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
			if(Sel == KDW_LIGHT_ON_n_OFF)
			{
				ret = 1;
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(Sel == KDW_LIGHT_DIMMING)
			{
				ret = 1;
			}
			break;
	}
	return ret;
}

void KDW_LIGHT_Model_Character_Data_Res(KDW_BUF *pRx)
{
	uint8_t item;
	KDW_BUF	*pTx;
	pTx = &TxBuf;
	
	item = KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1]; 

	pTx->count	= KDW_FRAME_HD;											
	pTx->buf[pTx->count++]	= KDW_HEADER;											//HEADER
	pTx->buf[pTx->count++]	= KDW_LIGHT_DEVICE_ID;									//DEVICE ID
	pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];						//DEVICE SUB ID
	pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE
	
	if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == 0x0F )	//�׷� Ư�� �䱸�� ��츸
	{
		pTx->buf[pTx->count++] = 0x0A;																			//LENGTH, DATA0 ~ DATA9
		pTx->buf[pTx->count++] = 0;																				//DATA0, ���� ����
		pTx->buf[pTx->count++] = (uint8_t)(pG_Config->LightCount + pG_Config->ThreeWayCount);					//DATA1, ���� ��
		pTx->buf[pTx->count++] = pG_Config->DimmingCount;														//DATA2, ��� ��
		pTx->buf[pTx->count++] = KDW_LIGHT_Character_Flag();													//DATA3, ���� ���� ����(1 ~ 8�� ����)
		pTx->buf[pTx->count++] = 0;																				//DATA4, ���� ���� ����(9 ~ E�� ����)
		pTx->buf[pTx->count++] = KDW_3way_Check();																//DATA5, 3�� ����ġ ���� �Ǵ� ��� �ش� ������ SUB ID(1 ~ 14), ������ 0x00
		if(pG_Config->Enable_Flag.PWM_Dimming)		pTx->buf[pTx->count++] = pG_Config->Dimming_MAX_Level;		//DATA6, ��� �ܰ� ���� ��� ���� �� �ִ� �ܰ� �Է�, ������ 0
		else										pTx->buf[pTx->count++] = 0;									//��� �ܰ� ���� ��� ���� �� �ִ� �ܰ� �Է�, ������ 0
		if(pG_Config->Enable_Flag.PWM_Color_Temp)	pTx->buf[pTx->count++] = pG_Config->Color_Temp_MAX_Level;	//DATA7, ���µ� �ܰ� ���� ��� ���� �� �ִ� �ܰ� �Է�, ������ 0
		else										pTx->buf[pTx->count++] = 0;									//���µ� �ܰ� ���� ��� ���� �� �ִ� �ܰ� �Է�, ������ 0	
		pTx->buf[pTx->count++] = KDW_LIGHT_Character_Color_Temp_Flag();											//DATA8, 1 ~ 8 ���� Ÿ��(bit1�̸� ���µ� ��� ����)
		pTx->buf[pTx->count++] = 0;																				//DATA9, 9 ~ E ���� Ÿ��(bit1�̸� ���µ� ��� ����)	
	}
	/*else if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) <= pG_Config->LightCount)	//DEV_SUB_ID�� 0x01 ~ 0x0E�� ��û ���
	{
		pTx->buf[pTx->count++] = 0x05;																//LENGTH
		pTx->buf[pTx->count++] = 0;																	//DATA0, ���� ����
		pTx->buf[pTx->count++] = KDW_LIGHT_Model_Select_Character_Data(item, KDW_LIGHT_ON_n_OFF);	//DATA1, ���� ��
		pTx->buf[pTx->count++] = KDW_LIGHT_Model_Select_Character_Data(item, KDW_LIGHT_DIMMING);	//DATA2, ��� ��
		pTx->buf[pTx->count++] = KDW_LIGHT_Character_Flag();										//DATA3, ���� ���� ����(1 ~ 8�� ����)
		pTx->buf[pTx->count++] = 0;																	//DATA4, ���� ���� ����(9 ~ E�� ����)		
	}*/
	//���� Ư�� ���� ������� ����.
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);
	TxBuf.send_flag	= 1;	
}
#endif	//#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)

void KDW_Data_Process(KDW_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item = 0;
	
	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
	// if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)
	// {
		// if(((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0xF0) >> 4) == Get_485_ID() || pRx->buf[KDW_FRAME_DEV_SUB_ID] == KDW_BATCH_LIGHT_DEVICE_SUB_ID)
		// {
			switch(pRx->buf[KDW_FRAME_CMD_TYPE])	// CMD
			{
				case KDW_STATE_REQ:	//���� �䱸
					KDW_LIGHT_Model_Data_Res(pRx);
					break;
				case KDW_CHARACTER_REQ:	//Ư�� �䱸
					KDW_LIGHT_Model_Character_Data_Res(pRx);
					break;
				case KDW_SELECT_CONTROL_REQ:	//���� ���� �䱸
					if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) != 0x0F)	//�׷� ID 1�ΰ�� 0x1F�� ������ ���� �� �����ϱ� ������ �߰�
					{
						KDW_Light_Control(KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1], pRx->buf[KDW_FRAME_DATA_0]);
						KDW_LIGHT_Model_Control_Res(pRx);
					}
					break;
				case KDW_ALL_CONTROL_REQ:		//��ü ���� �䱸
				case KDW_BATCHLIGHT_CONTROL_REQ:	//�ϰ� ���� �䱸
					KDW_Light_All_Control(pRx, pRx->buf[KDW_FRAME_DATA_0]);
					break;
				case KDW_3WAY_CONTROL_REQ:	//Ư����ȸ���� ID�� �����ϸ� ������� ����.
					break;
				case KDW_DIMMING_CONTROL_REQ:
					KDW_Dimming_Control(KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1], pRx->buf[KDW_FRAME_DATA_0]);
					KDW_LIGHT_Model_Control_Res(pRx);
					break;
				case KDW_COLOR_TEMP_CONTROL_REQ:
					KDW_Color_Temp_Control(KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1], pRx->buf[KDW_FRAME_DATA_0]);
					KDW_Color_Temp_Control_Res(pRx);
					break;
			}
		// }
	// }
	// if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) < (pG_Config->LightCount + pG_Config->DimmingCount))
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.
		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//??�Ρ����΢������� ������ ����ġ ID��? ����ġ?? ID�� ��?��?��?
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
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)	// 2�Ʃ���?	- ����+??��������?��

uint16_t BCD_CODE(uint16_t input)
{
	int shift;
	uint16_t bcd = 0;
	printf("input = %d", (uint16_t)input);
	for(shift = 0; input > 0; shift++, input /= 10)
	{
		bcd |= input % 10 << (shift << 2);
	}
	printf(" BCD = 0x%x\r\n", (uint16_t)bcd);
	return bcd;
}

uint16_t BIN_CODE(uint16_t input)
{
	uint16_t ret, binary = 0;
	int i;

	for(i = 1; input != 0x00; input = (uint16_t)(input >> 4), i *= 10)
	{
		binary += (uint16_t)(i * (input & 0x0F));
	}
	ret = binary;
	
	return ret;
}

uint8_t KDW_Watt2BCD(uint8_t item, uint8_t Sel, uint8_t Watt_Type)
{
	uint8_t state = 0;
	uint16_t Watt = 0, Watt_BCD = 0;
	uint16_t hundred, ten;

	if(Watt_Type == WATT_TYPE_LCD)
	{
		if(item == mapping_ITEM_ELECTRICITY_1)		Watt = Gu16_LCD_Watt_1;
		else if(item == mapping_ITEM_ELECTRICITY_2)	Watt = Gu16_LCD_Watt_2;
	}
	else if(Watt_Type == WATT_TYPE_LIMIT)
	{
		if(item == mapping_ITEM_ELECTRICITY_1)		Watt = Gu16_ElecLimitCurrent_1;
		else if(item == mapping_ITEM_ELECTRICITY_2)	Watt = Gu16_ElecLimitCurrent_2;		
	}

	if(Sel == THOUSAND)
	{
		Watt_BCD = BCD_CODE(Watt) & 0xF000;
		state = (uint8_t)(Watt_BCD >> 12);	//bit0 ~ 3
	}
	else if(Sel == HUNDRED)
	{
		Watt_BCD = BCD_CODE(Watt) & 0x0F00;
		state = (uint8_t)(Watt_BCD >> 4);	//bit 4 ~ 7
	}
	else if(Sel == TEN)
	{
		Watt_BCD = BCD_CODE(Watt) & 0x00F0;
		state = (uint8_t)(Watt_BCD >> 4);	//bit 0 ~ 3
	}
	else if(Sel == ONE)
	{
		Watt_BCD = BCD_CODE(Watt) & 0x000F;
		state = (uint8_t)(Watt_BCD << 4);	//bit 4 ~ 7
	}
	else if(Sel == DECIMAL)
	{
		state = (uint8_t)0;
	}

	return state;
}

uint16_t KDW_BCD2Watt(uint8_t Value_1, uint8_t Value_2)
{
	uint16_t Watt, Value;

	Value = (uint16_t)((Value_1 << 4) | (Value_2 >> 4));
	printf("input : %x\r\n", (uint16_t)Value);
	Watt = BIN_CODE(Value);
	printf("Watt : %x\r\n", (uint16_t)Watt);
	return Watt;
}

void KDW_Auto_Block_Value_Save(uint8_t item, uint16_t Watt)
{
	if(Watt <= 100)	//�ڵ� ���� ����?���ƨ�?? 100W �̳�?? �������� ���� �ƨ�?? ����. �ϰ�??��? ��??�� ����?���ƨ� ����.
	{
		if(item == mapping_ITEM_ELECTRICITY_1)
		{
			Gu16_ElecLimitCurrent_1 = Watt;
			Gu8_LCD_ElecLimitCurrent_Flashing_Flag = 1;
		}
		else if(item == mapping_ITEM_ELECTRICITY_2)
		{
			Gu16_ElecLimitCurrent_2 = Watt;
			Gu8_LCD_ElecLimitCurrent_Flashing_Flag = 11;
		}
		Store_ElecLimitCurrent();
	}
}

uint8_t KDW_ELEC_State_Res(KDW_BUF *pRx, uint8_t sel)
{
	uint8_t touch_switch, state = 0;

	if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_1)
	{
		touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_1);
		if(pG_State->ETC.Auto1)							state |= (1 << 7);
		else											state |= (0 << 7);

		if(Gu16_LCD_Watt_1 <= Gu16_ElecLimitCurrent_1)	state |= (1 << 6);
		else											state |= (0 << 6);

		if(Gu16_LCD_Watt_1 >= 3300)						state |= (1 << 5);	//������ ����
		else											state |= (0 << 5);

		if(GET_Switch_State(touch_switch) == OFF)		state |= (0 << 4);
		else											state |= (1 << 4);

		state |= KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, THOUSAND, WATT_TYPE_LCD);
	}
	else if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_2)
	{
		touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_2);
		if(pG_State->ETC.Auto2)							state |= (1 << 7);
		else											state |= (0 << 7);

		if(Gu16_LCD_Watt_2 <= Gu16_ElecLimitCurrent_2)	state |= (1 << 6);
		else											state |= (0 << 6);	

		if(Gu16_LCD_Watt_2 >= 3300)						state |= (1 << 5);	//������ ����
		else											state |= (0 << 5);

		if(GET_Switch_State(touch_switch) == OFF)		state |= (0 << 4);
		else											state |= (1 << 4);

		state |= KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, THOUSAND, WATT_TYPE_LCD);
	}
	else if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_ALL)
	{
		if(sel == KDW_ELEC_1)
		{
			touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_1);
			if(pG_State->ETC.Auto1)							state |= (1 << 7);
			else											state |= (0 << 7);

			if(Gu16_LCD_Watt_1 <= Gu16_ElecLimitCurrent_1)	state |= (1 << 6);
			else											state |= (0 << 6);

			if(Gu16_LCD_Watt_1 >= 3300)						state |= (1 << 5);	//������ ����
			else											state |= (0 << 5);

			if(GET_Switch_State(touch_switch) == OFF)		state |= (0 << 4);
			else											state |= (1 << 4);

			state |= KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, THOUSAND, WATT_TYPE_LCD);
		}
		else if(sel == KDW_ELEC_2)
		{
			touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_2);
			if(pG_State->ETC.Auto2)							state |= (1 << 7);
			else											state |= (0 << 7);

			if(Gu16_LCD_Watt_2 <= Gu16_ElecLimitCurrent_2)	state |= (1 << 6);
			else											state |= (0 << 6);

			if(Gu16_LCD_Watt_2 >= 3300)						state |= (1 << 5);	//������ ����
			else											state |= (0 << 5);			

			if(GET_Switch_State(touch_switch) == OFF)		state |= (0 << 4);
			else											state |= (1 << 4);

			state |= KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, THOUSAND, WATT_TYPE_LCD);
		}
	}
	return state;
}

uint8_t KDW_ELEC_Function_Info(void)
{
	uint8_t info = 0;

	info |= (1 << 7);	//bit7 : �ڵ����� ��� ����(ON : 1, OFF : 0)
	info |= (1 << 6);	//bit6 : ��ȿ���� ���� ��� ����(ON : 1, OFF : 0)
	info |= (1 << 5);	//bit5 : ������ ���� ��� ����(ON : 1, OFF : 0)

	return info;
}

void KDW_ELEC_Model_Data_Res(KDW_BUF *pRx)
{
	uint8_t	item, SUB_ID;
	KDW_BUF	*pTx;
	pTx = &TxBuf;
	SUB_ID = (uint8_t)(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F);

	if(SUB_ID == KDW_ELEC_ALL || SUB_ID == KDW_ELEC_1 || SUB_ID == KDW_ELEC_2)
	{
		pTx->count	= KDW_FRAME_HD;											
		pTx->buf[pTx->count++]	= KDW_HEADER;											//HEADER	
		pTx->buf[pTx->count++]	= KDW_ELEC_DEVICE_ID;									//DEVICE ID
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];						//DEVICE SUB ID
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE						
		switch(SUB_ID)
		{
			case KDW_ELEC_1:
			case KDW_ELEC_2:
				item = KDW_ELEC_ITEM_Sequence[SUB_ID - 1];
				pTx->buf[pTx->count++]	= 4;																								//LENGTH, 3 * ���� ä��(1) + 1
				pTx->buf[pTx->count++]	= 0;																								//DATA0, ���� ����
				pTx->buf[pTx->count++]	= KDW_ELEC_State_Res(pRx, 0);																		//DATA1, 1�� ä�� ����
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, HUNDRED, WATT_TYPE_LCD) | KDW_Watt2BCD(item, TEN, WATT_TYPE_LCD));	//DATA2, ä�� ����
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, ONE, WATT_TYPE_LCD) | KDW_Watt2BCD(item, DECIMAL, WATT_TYPE_LCD));	//DATA3, �ش�  ä�� ����
				break;
			case KDW_ELEC_ALL:
				pTx->buf[pTx->count++]	= 7;																								//LENGTH, 3 * ä�μ�(2) + 1, ������� ����ġ?? ä���� 2�� ����??.
				pTx->buf[pTx->count++]	= 0;																								//DATA0, ���� ����
				pTx->buf[pTx->count++]	= KDW_ELEC_State_Res(pRx, 1);																		//DATA1, 1�� ä�� ����
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, HUNDRED, WATT_TYPE_LCD)
				| KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, TEN, WATT_TYPE_LCD));															//DATA2, 1�� ä�� ����
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, ONE, WATT_TYPE_LCD)
				| KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, DECIMAL, WATT_TYPE_LCD));														//DATA3, 1�� ä�� ����
				pTx->buf[pTx->count++]	= KDW_ELEC_State_Res(pRx, 2);																		//DATA4, 2�� ä�� ����
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, HUNDRED, WATT_TYPE_LCD)
				| KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, TEN, WATT_TYPE_LCD));															//DATA5, 2�� ä�� ����
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, ONE, WATT_TYPE_LCD)
				| KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, DECIMAL, WATT_TYPE_LCD));														//DATA6, 2�� ä�� ����				
				break;
		}
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);		//XOR_SUM
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);		//ADD_SUM
		TxBuf.send_flag	= 1;
	}
}

void KDW_ELEC_Model_Character_Data_Res(KDW_BUF *pRx)
{
	KDW_BUF	*pTx;
	pTx = &TxBuf;

	if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == 0x0F)	//��ü ��û�� ��츸
	{
		pTx->count	= KDW_FRAME_HD;											
		pTx->buf[pTx->count++]	= KDW_HEADER;											//HEADER
		pTx->buf[pTx->count++]	= KDW_ELEC_DEVICE_ID;									//DEVICE ID
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];						//DEVICE SUB ID
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE
		pTx->buf[pTx->count++]	= 4;													//LENGTH, ä�μ�(n)+2
		pTx->buf[pTx->count++]	= 0;													//DATA0, ���� ����
		pTx->buf[pTx->count++]	= 2;													//DATA1, ������� ä�� ����(n)
		pTx->buf[pTx->count++]	= KDW_ELEC_Function_Info();								//DATA2, 1�� ������� ä�� ���
		pTx->buf[pTx->count++]	= KDW_ELEC_Function_Info();								//DATA3, 2�� ������� ä�� ���
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);
		TxBuf.send_flag	= 1;	
	}
	/*else	//DEV_SUB_ID�� ID|0x01 ~ ID|0x0E �� ��, �׷� ����ÿ��� 0x1F ~ 0xEF�� �����.
	{
		pTx->buf[pTx->count++] = 3;										//LENGTH, ä�μ�(n)+2
		pTx->buf[pTx->count++] = 0;										//DATA0, ���� ����
		pTx->buf[pTx->count++] = 1;										//DATA1, ������� ä�� ����(n) = 1
		pTx->buf[pTx->count++] = KDW_ELEC_Function_Info();				//DATA2, �ش� ä�� ����
	}*/	
}

uint8_t KDW_Elec_Model_Control_Data(uint8_t Sel)
{
	uint8_t touch_switch = 0, data = 0;

	if(Sel == KDW_ELEC_1)
	{
		touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_1);
		if(pG_State->ETC.Auto1)				data |= (1 << 1);	//�ڵ� ���� ���� ����, 1 : ����, 0 : ����
		else								data |= (0 << 1);
	}
	else if(Sel == KDW_ELEC_2)
	{
		touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_2);
		if(pG_State->ETC.Auto2)				data |= (1 << 1);	//�ڵ� ���� ���� ����, 1 : ����, 0 : ����
		else								data |= (0 << 1);
	}

	if(GET_Switch_State(touch_switch))	data |= (1 << 0);
	else								data |= (0 << 0);

	return data;
}

void KDW_Elec_Model_Control_Res(KDW_BUF *pRx)
{
	uint8_t SUB_ID;
	KDW_BUF	*pTx;
	pTx = &TxBuf;
	SUB_ID = (uint8_t)(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F);
	if(SUB_ID == KDW_ELEC_ALL || SUB_ID == KDW_ELEC_1 || SUB_ID == KDW_ELEC_2)
	{
		pTx->count	= KDW_FRAME_HD;											
		pTx->buf[pTx->count++]	= KDW_HEADER;											//HEADER
		pTx->buf[pTx->count++]	= KDW_ELEC_DEVICE_ID;									//DEVICE ID
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];						//DEVICE SUB ID
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_LENGTH] + 1);			//LENGTH, ä�� ��(n) + 1
		pTx->buf[pTx->count++]	= 0;													//DATA0, ���� ����

		switch(SUB_ID)
		{
			case KDW_ELEC_1:
			case KDW_ELEC_2:
				pTx->buf[pTx->count++]	= KDW_Elec_Model_Control_Data(SUB_ID);
				break;
			case KDW_ELEC_ALL:
				pTx->buf[pTx->count++]	= KDW_Elec_Model_Control_Data(KDW_ELEC_1);
				pTx->buf[pTx->count++]	= KDW_Elec_Model_Control_Data(KDW_ELEC_2);
				break;

		}
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);
		TxBuf.send_flag	= 1;
	}
}

void SET_Elec_Auto_Manual(uint8_t Sel, uint8_t Flag)
{
	Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
	if(Sel == KDW_ELEC_1)
	{
		if(Flag == KDW_OFF_FLAG)
		{
			if(pG_State->ETC.Auto1)	pG_State->ETC.Auto1 = KDW_OFF_FLAG;
			Beep(ON);
		}
		else if(Flag == KDW_ON_FLAG)
		{
			if(pG_State->ETC.Auto1 == 0)	pG_State->ETC.Auto1 = KDW_ON_FLAG;
			Beep(ON);
		}
	}
	else if(Sel == KDW_ELEC_2)
	{
		if(Flag == KDW_OFF_FLAG)
		{
			if(pG_State->ETC.Auto2)	pG_State->ETC.Auto2 = KDW_OFF_FLAG;
			Beep(ON);
		}
		else if(Flag == KDW_ON_FLAG)
		{
			if(pG_State->ETC.Auto2 == 0)	pG_State->ETC.Auto2 = KDW_ON_FLAG;
			Beep(ON);
		}
	}
}

void KDW_Elec_Control_Function(uint8_t item, uint8_t Flag)
{
	uint8_t touch_switch = 0;

	touch_switch = item2tsn(item);

	Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;

	if(Flag == OFF)
	{
		if(GET_Switch_State(touch_switch))
		{
			SET_Switch_State(touch_switch, Flag);
			SET_LED_State(touch_switch, Flag);
			Beep(Flag);
			PUT_RelayCtrl(item2ctrl(item), Flag);
			SET_SWITCH_Delay_OFF_Flag(item, 0);
			SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);
			ALL_Electricity_Switch_LED_Ctrl();
		}
	}
	else if(Flag == ON)
	{
		if(GET_Switch_State(item2tsn(item)) == 0)
		{
			SET_Switch_State(touch_switch, Flag);
			SET_LED_State(touch_switch, Flag);
			Beep(Flag);
			PUT_RelayCtrl(item2ctrl(item), Flag);
			SET_SWITCH_Delay_OFF_Flag(item, 0);
			SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);
			ALL_Electricity_Switch_LED_Ctrl();
		}
	}
}

void KDW_Elec_Control(KDW_BUF	*pRx)
{
	uint8_t i = 0, item = 0, Flag = 0;

	switch(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F)
	{
		case KDW_ELEC_1:
		case KDW_ELEC_2:
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x10) == 0x10)			//ä�� ���� ����
			{
				if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_1)			item = mapping_ITEM_ELECTRICITY_1;
				else if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_2)		item = mapping_ITEM_ELECTRICITY_2;
				
				if(pRx->buf[KDW_FRAME_DATA_0] & 0x01)				Flag = KDW_ON_FLAG;
				else												Flag = KDW_OFF_FLAG;

				KDW_Elec_Control_Function(item, Flag);
			}
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x20) == 0x20)			//�ڵ� ���ܱ�� ����
			{
				if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_1)
				{
					if(pRx->buf[KDW_FRAME_DATA_0] & 0x02)		SET_Elec_Auto_Manual(KDW_ELEC_1, KDW_ON_FLAG);	//�ڵ� ���� ����
					else										SET_Elec_Auto_Manual(KDW_ELEC_1, KDW_OFF_FLAG);	//�ڵ� ���� ����
				}
				else if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_2)
				{
					if(pRx->buf[KDW_FRAME_DATA_0] & 0x02)		SET_Elec_Auto_Manual(KDW_ELEC_2, KDW_ON_FLAG);	//�ڵ� ���� ����
					else										SET_Elec_Auto_Manual(KDW_ELEC_2, KDW_OFF_FLAG);	//�ڵ� ���� ����
				}
			}
			break;
		case KDW_ELEC_ALL:
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x10) == 0x10)
			{
				if(pRx->buf[KDW_FRAME_DATA_0+i] & 0x01)	Flag = ON;		//0x01 -> ������� ����X
				else									Flag = OFF;		//0x00 -> ������� ����O
				KDW_Elec_Control_Function(mapping_ITEM_ELECTRICITY_1, Flag);
				KDW_Elec_Control_Function(mapping_ITEM_ELECTRICITY_2, Flag);
			}
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x20) == 0x20)
			{
				if(pRx->buf[KDW_FRAME_DATA_0] & 0x02)
				{
					SET_Elec_Auto_Manual(KDW_ELEC_1, KDW_ON_FLAG);	//�ڵ� ���� ����
					SET_Elec_Auto_Manual(KDW_ELEC_2, KDW_ON_FLAG);	//�ڵ� ���� ����
				}
				else
				{
					SET_Elec_Auto_Manual(KDW_ELEC_1, KDW_OFF_FLAG);	//�ڵ� ���� ����
					SET_Elec_Auto_Manual(KDW_ELEC_2, KDW_OFF_FLAG);	//�ڵ� ���� ����
				}
			}
			break;
	}
}

void KDW_Elec_All_Control(uint8_t Flag)
{
	uint8_t i = 0;

	if(Flag == OFF)
	{
		if(Gu8_Elec_OFF_Repeat_Tmr == 0)
		{
			for(i = 0; i < pG_Config->ElectricityCount; i++)
			{
				KDW_Elec_Control_Function(KDW_ELEC_ITEM_Sequence[i], Flag);
			}
			Gu8_Elec_OFF_Repeat_Tmr = 5;
		}
	}
	else if(Flag == ON)
	{
		if(Gu8_Elec_ON_Repeat_Tmr == 0)
		{
			for(i = 0; i < pG_Config->ElectricityCount; i++)
			{
				KDW_Elec_Control_Function(KDW_ELEC_ITEM_Sequence[i], Flag);
			}
			Gu8_Elec_ON_Repeat_Tmr = 5;
		}
	}
}

void KDW_Auto_Block_Value_Res(KDW_BUF	*pRx)
{
	uint8_t item = 0;
	KDW_BUF	*pTx;
	pTx = &TxBuf;

	pTx->count	= KDW_FRAME_HD;											
	pTx->buf[pTx->count++]	= KDW_HEADER;								//HEADER
	pTx->buf[pTx->count++]	= KDW_ELEC_DEVICE_ID;						//DEVICE ID
	pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];			//DEVICE SUB ID
	pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE

	item = KDW_ELEC_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1];

	switch(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F)
	{
		case KDW_ELEC_1:
		case KDW_ELEC_2:
			pTx->buf[pTx->count++]	= 3;			//LENGTH, 2 * ä�� ��(n) + 1
			pTx->buf[pTx->count++]	= 0;			//DATA0, ���� ����
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(item, TEN, WATT_TYPE_LIMIT));	//bit7 ~ 4 : 100W �ڸ� BCD, bit3 ~ 0 : 10W �ڸ� BCD
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(item, DECIMAL, WATT_TYPE_LIMIT));		//bit7 ~ 4 : 1W �ڸ� BCD, bit3 ~ 0 : 0.1W �ڸ� BCD
			break;
		case KDW_ELEC_ALL:
			pTx->buf[pTx->count++]	= 5;			//LENGTH, 2 * ä�� ��(n) + 1
			pTx->buf[pTx->count++]	= 0;			//DATA0, ���� ����
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, TEN, WATT_TYPE_LIMIT));	//bit7 ~ 4 : 100W �ڸ� BCD, bit3 ~ 0 : 10W �ڸ� BCD
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, DECIMAL, WATT_TYPE_LIMIT));		//bit7 ~ 4 : 1W �ڸ� BCD, bit3 ~ 0 : 0.1W �ڸ� BCD
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, TEN, WATT_TYPE_LIMIT));	//bit7 ~ 4 : 100W �ڸ� BCD, bit3 ~ 0 : 10W �ڸ� BCD
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, DECIMAL, WATT_TYPE_LIMIT));		//bit7 ~ 4 : 1W �ڸ� BCD, bit3 ~ 0 : 0.1W �ڸ� BCD						
			break;
	}
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);
	TxBuf.send_flag	= 1;
}

void KDW_SET_Auto_Block_Value(KDW_BUF	*pRx)
{
	uint8_t item = 0;
	uint16_t Watt = 0;
	item = KDW_ELEC_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1];
	
	switch(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F)
	{
		case KDW_ELEC_1:
		case KDW_ELEC_2:
			Watt = KDW_BCD2Watt(pRx->buf[KDW_FRAME_DATA_0], pRx->buf[KDW_FRAME_DATA_1]);
			KDW_Auto_Block_Value_Save(item, Watt);
			break;
		case KDW_ELEC_ALL:
			Watt = KDW_BCD2Watt(pRx->buf[KDW_FRAME_DATA_0], pRx->buf[KDW_FRAME_DATA_1]);
			KDW_Auto_Block_Value_Save(mapping_ITEM_ELECTRICITY_1, Watt);
			Watt = KDW_BCD2Watt(pRx->buf[KDW_FRAME_DATA_2], pRx->buf[KDW_FRAME_DATA_3]);
			KDW_Auto_Block_Value_Save(mapping_ITEM_ELECTRICITY_2, Watt);
			break;
	}
}

void KDW_SET_Auto_Block_Value_Res(KDW_BUF	*pRx)
{
	uint8_t item = 0;

	KDW_BUF	*pTx;
	pTx = &TxBuf;

	pTx->count	= KDW_FRAME_HD;											
	pTx->buf[pTx->count++]	= KDW_HEADER;								//HEADER
	pTx->buf[pTx->count++]	= KDW_ELEC_DEVICE_ID;						//DEVICE ID
	pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];			//DEVICE SUB ID
	pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE
									//DATA0, ���� ����
	item = KDW_ELEC_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1];

	switch(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F)
	{
		case KDW_ELEC_1:
		case KDW_ELEC_2:
			pTx->buf[pTx->count++]	= 3;			//LENGTH, 2 * ä�� ��(n) + 1 
			pTx->buf[pTx->count++]	= 0;	
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(item, TEN, WATT_TYPE_LIMIT));
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(item, DECIMAL, WATT_TYPE_LIMIT));
			break;
		case KDW_ELEC_ALL:
			pTx->buf[pTx->count++]	= 5;			//LENGTH, 2 * ä�� ��(n) + 1 
			pTx->buf[pTx->count++]	= 0;	
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, TEN, WATT_TYPE_LIMIT));
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, DECIMAL, WATT_TYPE_LIMIT));
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, TEN, WATT_TYPE_LIMIT));
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, DECIMAL, WATT_TYPE_LIMIT));
			break;
	}
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);
	TxBuf.send_flag	= 1;	
}

void KDW_Data_Process(KDW_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item = 0;
	
	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
	if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID || pRx->buf[KDW_FRAME_DEV_ID] == KDW_ELEC_DEVICE_ID)
	{
		switch(pRx->buf[KDW_FRAME_CMD_TYPE])	// CMD
		{
			case KDW_STATE_REQ:	//���� �䱸
				if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)		KDW_LIGHT_Model_Data_Res(pRx);
				else if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_ELEC_DEVICE_ID)	KDW_ELEC_Model_Data_Res(pRx);
				break;
			case KDW_CHARACTER_REQ:	//Ư�� �䱸
				if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)		KDW_LIGHT_Model_Character_Data_Res(pRx);
				else if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_ELEC_DEVICE_ID)	KDW_ELEC_Model_Character_Data_Res(pRx);
				break;
			case KDW_SELECT_CONTROL_REQ:	//���� ���� �䱸
				if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)
				{
					KDW_Light_Control(KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1], pRx->buf[KDW_FRAME_DATA_0]);
					KDW_LIGHT_Model_Control_Res(pRx);
				}
				else if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_ELEC_DEVICE_ID)
				{
					KDW_Elec_Control(pRx);
					KDW_Elec_Model_Control_Res(pRx);
				}
				break;
			case KDW_ALL_CONTROL_REQ:		//��ü ���� �䱸
				if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)
				{
					KDW_Light_All_Control(pRx, pRx->buf[KDW_FRAME_DATA_0]);
				}
				else if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_ELEC_DEVICE_ID)
				{
					KDW_Elec_All_Control(pRx->buf[KDW_FRAME_DATA_0]);
				}
				break;
			case KDW_3WAY_CONTROL_REQ:
				break;
			case KDW_DIMMING_CONTROL_REQ:
				KDW_Dimming_Control(KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1], pRx->buf[KDW_FRAME_DATA_0]);
				KDW_LIGHT_Model_Control_Res(pRx);
				break;
			case KDW_COLOR_TEMP_CONTROL_REQ:
				KDW_Color_Temp_Control(KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1], pRx->buf[KDW_FRAME_DATA_0]);
				KDW_Color_Temp_Control_Res(pRx);
				break;
			case KDW_AUTO_BLOCK_VALUE_REQ:	//�ڵ� ���� ���� �� �䱸
				KDW_Auto_Block_Value_Res(pRx);
				break;
			case KDW_AUTO_BLOCK_VALUE_SET_REQ:	//�ڵ� ���� ���� �� ���� �䱸
			// case KDW_BATCHLIGHT_CONTROL_REQ:	//���� ������ ���� �䱸�� �ϰ� ���� �䱸�� Ŀ�ǵ� ���� ������.
				if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)
				{
					KDW_Light_All_Control(pRx, pRx->buf[KDW_FRAME_DATA_0]);
				}
				else if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_ELEC_DEVICE_ID)
				{
					KDW_SET_Auto_Block_Value(pRx);
					KDW_SET_Auto_Block_Value_Res(pRx);
				}
				break;
		}
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.
		/*
		if(pRx->buf[2] != pG_Config->RS485_ID)			//??�Ρ����΢������� ������ ����ġ ID��? ����ġ?? ID�� ��?��?��?
		{
			pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
			Store_CurrentConfig();
			printf("Switch ID Change\r\n");
		}*/
		RS_485_ID_RES();
	}
}
#endif	// defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1�Ʃ���? - �ϰ����ܽ���ġ
//--------------------------------------�ϰ����� ����ġ ??������?-----------------------------------------------------------------------------------------------
// #if 1	//�ϰ����� ����ġ ??�λ��� ?����? ��

uint8_t KDW_BATCH_BLOCK_User_Req(void)
{
	uint8_t touch_switch, data = 0;

	data |= (0 << 3);	//������� ���� ���� ON/OFF. 1 : ON, 0 : OFF
	data |= (0 << 1);	//���� ���� �䱸. 1 : �䱸, 0 : �䱸 ����

	if(item2tsn(mapping_ITEM_3WAY_1))
	{
		touch_switch = item2tsn(mapping_ITEM_3WAY_1);
		if(GET_Switch_State(touch_switch))				data |= (1 << 6);	//3�� ����ġ ���� �䱸 ���� ON/OFF ����. Ư��X : 1 : �䱸, 0 : �䱸 ���� // Ư��O : 1 : ON, 0 : OFF
		else											data |= (0 << 6);
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		touch_switch = item2tsn(mapping_ITEM_ELEVATOR);
		if(GET_LED_State(touch_switch) == LED_FLASHING)	data |= (1 << 5);	//���������� �ϰ� ȣ�� �䱸. 1 : �䱸, 0 : �䱸 ����
		else											data |= (0 << 5);

		data |= (0 << 4);	//���������� ���� ��� ȣ�� �䱸. ��� ȣ�� ��� 1 : �䱸, 0 : �䱸 ����.
	}
	if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF))
	{
		touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
		if(GET_Switch_State(touch_switch))				data |= (1 << 2);	//�ϰ� ���� ���� ������ ON/OFF ����. 1 : ON, 0 : OFF
		else											data |= (0 << 2);
	}
	if(item2tsn(mapping_ITEM_GAS))
	{
		touch_switch = item2tsn(mapping_ITEM_GAS);
		if(GET_LED_State(touch_switch) == LED_FLASHING)	data |= (1 << 0);	//���� ���� �䱸. 1 : �䱸, 0 : �䱸 ����
		else											data |= (0 << 0);
	}
	if(item2tsn(mapping_ITEM_COOK))
	{
		touch_switch = item2tsn(mapping_ITEM_COOK);
		if(GET_LED_State(touch_switch) == LED_FLASHING)	data |= (1 < 7);	//��ž ���� �䱸. 1 : �䱸, 0 : �䱸 ����
		else											data |= (0 < 7);
	}
	return data;
}

uint8_t KDW_BATCH_BLOCK_User_Function(void)
{
	uint8_t data = 0;
	
	data |= (0 << 7);	//������ġ ǥ�� ��� ����
	data |= (0 << 5);	//��Ȱ���� ���� ��� ����
	data |= (0 << 4);	//���������� �� ǥ�� ��� ����
	data |= (0 << 2);	//������� ���� ��� ����
	data |= (0 << 1);	//���� ���� ��� ����

	if(item2tsn(mapping_ITEM_3WAY_1))	data |= (1 << 6);	//3�� ����ġ ���� ���� ��� ����
	else								data |= (0 << 6);
	if(item2tsn(mapping_ITEM_ELEVATOR))	data |= (1 << 3);	//���������� ȣ�� ���� ��� ����
	else								data |= (0 << 3);
	if(item2tsn(mapping_ITEM_GAS))		data |= (1 << 0);	//���� ���� ��� ����
	else								data |= (0 << 0);

	return data;
}

void KDW_BATCH_BLOCK_Data_Res(KDW_BUF *pRx)
{
	KDW_BUF	*pTx;
	pTx = &TxBuf;
	
	pTx->count	= KDW_FRAME_HD;											
	pTx->buf[pTx->count++]	= KDW_HEADER;									//HEADER	
	pTx->buf[pTx->count++]	= KDW_BATCH_BLOCK_DEVICE_ID;					//DEVICE ID
	pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];				//DEVICE SUB ID
	pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE
	pTx->buf[pTx->count++]	= 0x03;											//LENGTH, 0x03 ����
	pTx->buf[pTx->count++]	= 0;											//DATA0, ���� ����

	if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_STATE_REQ || pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_SELECT_CONTROL_REQ)
	{
		pTx->buf[pTx->count++]	= KDW_BATCH_BLOCK_User_Req();				//DATA1, ����ġ ���� : ��ž, 3��, �������稬����?, �������, �ϰ�, ��û, ����
		pTx->buf[pTx->count++]	= 0;										//DATA2, ����ġ ���� : ��������(��û����??) ���� �䱸
	}
	else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_CHARACTER_REQ)
	{
		pTx->buf[pTx->count++]	= KDW_BATCH_BLOCK_User_Function();			//DATA1, ����ġ ��� ���� : ������ġ ǥ��, 3��, ��Ȱ���� ǥ��, ���������� �� ǥ�� ���, ���������� ȣ�� ���, �������, ���� ����, ���� ����
		pTx->buf[pTx->count++]	= 0;										//DATA2, ����ġ ��� ���� : �ơ���?����, ��ž ����, �������� ����
	}
	else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_USER_REQ_RESULT)
	{
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DATA_0];				//DATA1, ��� ������ DATA0�� ������ ����
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DATA_1];				//DATA2, ��� ������ DATA1�� ������ ����
	}
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);
	TxBuf.send_flag	= 1;	
}

void KDW_State_Req_Control(KDW_BUF *pRx)
{
	if(item2tsn(mapping_ITEM_GAS))	//���� ����ϴ� ���̸�
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) != LED_FLASHING)			//����ġ���� ���� ���� ��û���� �ƴ� ���
		{
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == KDW_GAS_OPEN)				//���� ���� ���·� ���е忡�� ���޿���
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
			}
			else if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == KDW_GAS_CLOSE)			//���� ���� ���·� ���е忡�� ���޿���
			{
				BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
			}
		}
	}	
}

void KDW_Select_Control(KDW_BUF *pRx)
{
	if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == 0x01)	BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);
	else											BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
}

void KDW_User_Req_Control(KDW_BUF *pRx)	//����� ��û ó�� ��� �� ����
{
	if(item2tsn(mapping_ITEM_GAS))
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)	//����ġ���� ���� ���� ��û���� ����
		{
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == 0x01)			//���� ��ݿ䱸 Ȯ��
			{
				BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);			//���� ����
			}
			else if((pRx->buf[KDW_FRAME_DATA_0] & 0x02) == 0x02)		//���� ��� ����
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);			//���� ���� ����
			}
		}
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)
		{
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x10) == 0x10)		//���������� ȣ�� Ȯ��
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL);		//���������� ȣ�� ����
			}
			else if((pRx->buf[KDW_FRAME_DATA_0] & 0x20) == 0x20)	//���������� ȣ�� ����
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);	//���������� ȣ�� ����
			}
		}
	}
	if(item2tsn(mapping_ITEM_3WAY_1))
	{
		;
	}
}

void KDW_THREEWAY_Control(KDW_BUF *pRx)
{
	uint8_t touch_switch = 0;

	if(item2tsn(mapping_ITEM_3WAY_1))
	{
		touch_switch = item2tsn(mapping_ITEM_3WAY_1);
		if(pRx->buf[KDW_FRAME_DATA_0] == KDW_ON_FLAG)
		{
			if(GET_Switch_State(touch_switch) == OFF)	EventCtrl(touch_switch, ON);//3�� ���� ON
		}
		else if(pRx->buf[KDW_FRAME_DATA_0] == KDW_OFF_FLAG)
		{
			if(GET_Switch_State(touch_switch))			EventCtrl(touch_switch, OFF);//3�� ���� OFF
		}
	}
}

void KDW_COOK_Control(KDW_BUF *pRx)
{
	if(item2tsn(mapping_ITEM_COOK))
	{
		if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_COOKTOP_STATE_NOTICE)
		{
			if(GET_Switch_State(item2tsn(mapping_ITEM_COOK)) != LED_FLASHING)	//��û ���°� �ƴ� ��
			{
				if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == 0x01)	BATCH_BLOCK_Control(SET__COOK_OPEN_STATE);
				else											BATCH_BLOCK_Control(SET__COOK_CLOSE_STATE);
			}
		}
		else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_COOKTOP_CONTROL_RESULT)
		{
			if(GET_Switch_State(item2tsn(mapping_ITEM_COOK)) == LED_FLASHING)	//��û ������ ��츸
			{
				if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == 0x01)			BATCH_BLOCK_Control(SET__COOK_CLOSE_STATE);	//��ž ���� �䱸 Ȯ�� 1 : Ȯ��, 0 : �ش� ����
				else if((pRx->buf[KDW_FRAME_DATA_0] & 0x02) == 0x02)	BATCH_BLOCK_Control(SET__COOK_OPEN_STATE);	//��ž ���� ���� 1 : ����, 0 : �ش� ����
			}
		}
	}
}

void KDW_Data_Process(KDW_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item = 0;
	
	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

	if(pRx->buf[KDW_FRAME_DEV_SUB_ID] == Get_485_ID())
	{
		switch(pRx->buf[KDW_FRAME_CMD_TYPE])	// CMD
		{
			case KDW_STATE_REQ:				//���� �䱸
			case KDW_SELECT_CONTROL_REQ:	//���� ���� �䱸
			case KDW_CHARACTER_REQ:			//Ư�� �䱸, KDW_CHARACTER_REQ�� ���� ���� ���丸
			case KDW_USER_REQ_RESULT:		//ó�� ��� ����
				if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_STATE_REQ)				KDW_State_Req_Control(pRx);
				else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_SELECT_CONTROL_REQ)	KDW_Select_Control(pRx);
				else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_USER_REQ_RESULT)	KDW_User_Req_Control(pRx);
				KDW_BATCH_BLOCK_Data_Res(pRx);
				break;
			case KDW_ELEVATOR_ARRIVE:		//���������� ���� ������. �������
				BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);
				break;					
			case KDW_THREE_WAY_STATE_NOTICE:	//3�� ���� ���� ����ġ ���� �˸�. �������
				KDW_THREEWAY_Control(pRx);
				break;
			case KDW_COOKTOP_STATE_NOTICE:		//��ž ��� ��� �� ����
			case KDW_COOKTOP_CONTROL_RESULT:	//��ž ��� ��� �� ����
				KDW_COOK_Control(pRx);
				break;
		}
	}
	else
	{
		if(pRx->buf[KDW_FRAME_DEV_SUB_ID] == 0x0F || pRx->buf[KDW_FRAME_DEV_SUB_ID] == 0xFF)
		{
			if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_ALL_CONTROL_REQ)	//�������
			{
				if((pRx->buf[KDW_FRAME_DATA_0] & (1 << (Get_485_ID() - 1))) == (1 << (Get_485_ID() - 1)))	//ex)ID : 1, bit0 : 1??��?, ON, ID : 2, bit1 : 1??��? ON
				{//0xff (1 << 2 - 1) == 1 << 2 - 1
					BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);	//�������ݿ��� ������ 1������ ���� �亯���δ� 1�� ON�̶�� ��.
				}
				else		//ex)ID : 1, bit0 : 0??��? OFF
				{
					BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
				}
			}
		}
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//���� ID�� �����ص� ������� �ʰ�, ID �˻�ø� ����Ǳ� ������ �ǹ̾�� �ּ� ó����.
		/*/
		if(pRx->buf[2] != pG_Config->RS485_ID)			//??�Ρ����΢������� ������ ����ġ ID��? ����ġ?? ID�� ��?��?��?
		{
			pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
			Store_CurrentConfig();
			printf("Switch ID Change\r\n");
		}*/
		RS_485_ID_RES();
	}
}

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
	else if (item2tsn(mapping_ITEM_COOK))
	{
		if (Gu16_GAS_Off_Tmr == 0)
		{
			if (GET_LED_State(item2tsn(mapping_ITEM_COOK)) == LED_FLASHING)
			{
				if (G_Trace)	printf("Cook Close Req Time out\r\n");
				BATCH_BLOCK_Control(SET__COOK_OPEN_STATE);
			}
		}
	}

	if(item2tsn(mapping_ITEM_ELEVATOR))		//ȣ�� ��û or ȣ�� ���� ��
	{
		if(Gu16_Elevator_Tmr == 0)	//KDW�� ���� ?? 3�� �̳� ���� �����Ͱ� ���� �ʾƵ� ���� �ʱ�ȭ �ؾ���.
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
	uint8_t	touch_switch = 0;
	
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
		case SET__GAS_CLOSE_REQUEST:		// ���������� ����ġ ���°� ���� �־�� ������ ������ �����̴�
		case SET__COOK_CLOSE_REQUEST:
			if(control == SET__GAS_CLOSE_REQUEST)		touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			else if(control == SET__COOK_CLOSE_REQUEST)	touch_switch = item2tsn((uint8_t)mapping_ITEM_COOK);

			if(GET_Switch_State(touch_switch) == OFF || GET_LED_State(touch_switch) == LED_FLASHING)	// �������갡 ����(OFF)�ְų�, ���� ��û(LED_FLASHING)���� ���
			{
				Gu16_GAS_Off_Tmr	= GAS_TIME_OUT;		// 60�� ��� �� LED �ҵ�(�� ���е忡�� close/open �����Ͱ� ���ŵǸ� �ش� ���·� ��ȯ)
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
		case SET__COOK_CLOSE_STATE:
			if(control == SET__GAS_CLOSE_STATE)			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			else if(control == SET__COOK_CLOSE_REQUEST)	touch_switch = item2tsn((uint8_t)mapping_ITEM_COOK);

			if(GET_Switch_State(touch_switch) == 0)		//��û���̰ų�, ���� ����  �����϶���,
			{
				SET_Switch_State(touch_switch, ON);		// ������� ����(����)
				SET_LED_State(touch_switch, OFF);		// �����δ� LED ����
				Beep(ON);
			}
			break;
			
		case SET__GAS_OPEN_STATE:
		case SET__COOK_OPEN_STATE:
			if(control == SET__GAS_OPEN_STATE)			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			else if(control == SET__COOK_CLOSE_REQUEST)	touch_switch = item2tsn((uint8_t)mapping_ITEM_COOK);

			if(GET_Switch_State(touch_switch) || (GET_LED_State(touch_switch) == LED_FLASHING))
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
				Gu16_Elevator_Tmr = ELEVATOR_TMR;															//KDW�� ȣ�� �� 5�� �̳� ��� ���� ������ ������.
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
				Gu16_Elevator_Tmr = 30;																		//�� ǥ�� ���� ��쿡�� ���Ƿ� 30�� ���� ȣ�� ���·� ��
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


#endif	// _KDW_PROTOCOL_

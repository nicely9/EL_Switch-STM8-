/************************************************************************************
	Project		: 전자식 스위치
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

/*#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_	// 일괄차단스위치
void BATCH_BLOCK_STATE_Process(void);
#endif*/

void KDW_Data_Process(KDW_BUF	*pRx);
uint8_t KDW_Batch_Light_State(uint8_t item);
void BATCH_BLOCK_Control(uint8_t control);
// ----------------------------------------------------------------------------------------
static	KDW_BUF		RxBuf, TxBuf;
#define ELEVATOR_TMR	7		//
#define GAS_TIME_OUT 	7		 //프로토콜 상 5초간 표시하지만, 7초로 함.

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
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// 대기전력 ¸ð?¨ 
	count	= 0;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_KDW_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_KDW_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
#endif
}
// ----------------------------------------------------------------------------------------
void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr)
{
	if(tmr == 0)		// ¸¶??¸· ??상태 ¼?½? ?? X ms 일괄??¸? ??상태 ??¸®¾?
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
uint8_t NIS_Crc(KDW_BUF *pTRx, uint8_t cal, uint8_t sel)		//생산시 485통신 테스트를 위해서 추가함.
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
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//프로토콜 타입
    pTx->buf[pTx->count++]	= Get_485_ID();	
	pTx->buf[pTx->count++]	= 0;

	pTx->buf[pTx->count++]	= Elec_Watt_MSB;				//전력값
	pTx->buf[pTx->count++]	= Elec_Watt_LSB;				//전력값

	pTx->buf[pTx->count++]	= Gu8_ZeroCrossing_Err_Flag;	//제로크로싱 동작 상태 1이면 Err, 0이면 Pass

	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 1, NIS_TX);
	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 0, NIS_TX);
	TxBuf.send_flag	= 1;
}

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

uint8_t Color_Temp_Cnt(void)
{
	uint8_t count = 0;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1) || item2tsn(mapping_ITEM_DIMMING_LIGHT_2))
	{
		if(pG_Config->Enable_Flag.PWM_Color_Temp)	count++;
	}
	return count;
}

uint8_t Elec_Cnt(void)		//전열 수 카운트
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
					pRx->count = 0;	//일괄차단 전체 동작 커맨드가 아닌경우 서브 id가 0x0F, 0xFF면 카운트 초기화
				}
			}
			break;
#endif
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
		case 3:		// DEV_SUB_ID
			if(((pRx->buf[KDW_FRAME_DEV_SUB_ID] >> 4) != Get_485_ID()) && (pRx->buf[KDW_FRAME_DEV_SUB_ID] != 0xFF))	//그룹 ID와 스위치 ID가 다를 때, 일괄 소등 제어용 SUB ID가 아닐 때
			{
				if((pRx->buf[KDW_FRAME_HD] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[KDW_FRAME_DEV_ID] != NIS_LIGHT_ID_COMM_2))
				{
					pRx->count = 0;
				}
			}
			else	//그룹 ID와 스위치 ID가 동일하거나 SUB_ID가 0xFF일 때
			{
				if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)
				{
					if(((pRx->buf[KDW_FRAME_DEV_SUB_ID] >> 4) == Get_485_ID()) && ((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) != 0x0F))
					{
						if(((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) > Light_Cnt()) || ((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == 0x00))	//그룹 ID와 스위치 ID가 동일할 때 회로 번호가 해당 스위치 전등 수 보다 많으면
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
			if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)				//전등 디바이스
			{
				if(pRx->buf[KDW_FRAME_DEV_SUB_ID] == 0xFF)						//서브 ID 0xFF일 때
				{
					if((pRx->buf[KDW_FRAME_CMD_TYPE] != KDW_BATCHLIGHT_CONTROL_REQ) && (pRx->buf[KDW_FRAME_CMD_TYPE] != KDW_ALL_CONTROL_REQ))	//일괄 소등/해제 제어 커맨드, 전체 전등 제어 요구 커맨드가 아니면 아니면
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
			xor_crc = KDW_Crc(pRx, pRx->count-2, XOR_SUM);	//xor 차단상태 xor °ª
			crc		= KDW_Crc(pRx, pRx->count-1, ADD_SUM);	//xor±??? add °ª
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

uint8_t KDW_Batch_Light_State(uint8_t item)		//일괄 소등 전 현재 전등 상태 저장.
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
			if(control_value == KDW_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == KDW_ON_FLAG)
				{
					EventCtrl(item2tsn(item), OFF);						//전등 OFF
				}
			}
			else if(control_value == KDW_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == KDW_OFF_FLAG)
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

#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
void SET_DimmingLevel(uint8_t item, uint8_t Dimming_Level)	//개별 제어(0x41)에서 사용하는 디밍 제어
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
				if(control_value & 0xF0)	//bit7 ~ bit4는 디밍 레벨
				{
					if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);	//bit0가 1이면 전등 켜짐, 0이면 전등 꺼짐
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
					Gu8_LCD_DIM_Tmr					= 20;
					SET_DimmingLevel(item, (uint8_t)(control_value >> 4));
				}
				else
				{
					if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);	//bit0가 1이면 전등 켜짐, 0이면 전등 꺼짐
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

void KDW_Dimming_Control(uint8_t item, uint8_t data)	//디밍 개별 제어(0x52)에서 사용하는 디밍 제어
{
	uint8_t control = 0;

	control = (uint8_t)(data & 0x03);	//bit0, bit1¸¸  ?제어?
	if(GET_Switch_State(item2tsn(item)))	//해당 전등 켜져있을 때만 동작
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
				Gu8_LCD_DIM_Tmr					= 50;		// 5s 동안 LCD 표시
				Beep(ON);
			}
			else if(control == KDW_DIMMING_DOWN)
			{ 
				if(pG_State->Dimming_Level.Dimming1)	pG_State->Dimming_Level.Dimming1--;
				if((uint8_t)pG_State->Dimming_Level.Dimming1 == (uint8_t)0)
				{
					pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;
				}
				Gu8_LCD_DIM_Tmr					= 50;		// 5s 동안 LCD 표시
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
				Gu8_LCD_DIM_Tmr					= 50;		// 5s 동안 LCD 표시
				Beep(ON);
			}
			else if(control == KDW_DIMMING_DOWN)
			{ 
				if(pG_State->Dimming_Level.Dimming2)	pG_State->Dimming_Level.Dimming2--;
				if((uint8_t)pG_State->Dimming_Level.Dimming2 == (uint8_t)0)
				{
					pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;
				}
				Gu8_LCD_DIM_Tmr					= 50;		// 5s 동안 LCD 표시
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
					Gu8_ON_Repeat_Tmr = 5;	//500ms 연속 3회 전송 중 추가 전송 2회 인터벌 약 200ms라고 함. 넉넉히 500ms
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
			case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
			case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
			case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
			case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
			case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
			case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
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
			case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
			case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
			case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
			case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
			case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
			case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
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
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
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
			
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				ret = (uint8_t)((pG_State->Dimming_Level.Dimming1 << 4) | 1);	//디밍 기능 있고 ON이면 bit7 ~ bit4까지는 디밍 레벨, OFF면 0.  bit0은 1:켜짐, 0:꺼짐
			}
			else
			{
				ret = KDW_OFF_FLAG;												//디밍 기능 있어도 OFF상태면 bit7 ~ bit4에 0x0.
			}
			ret |= (1 << 1);	//밝기 조절 기능 있음
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				ret = (uint8_t)((pG_State->Dimming_Level.Dimming2 << 4) | 1);
			}
			else
			{
				ret = KDW_OFF_FLAG;
			}
			ret |= (1 << 1);	//밝기 조절 기능 있음
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
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit1 자동/수동, bit0 전원 ON/OFF
			{
				ret	= (uint8_t)(KDW_ON_FLAG | (pG_State->ETC.Auto1<<1));
			}
			else
			{
				ret	= (uint8_t)(KDW_OFF_FLAG | (pG_State->ETC.Auto1<<1));
			}
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(GET_Switch_State(item2tsn((uint8_t)item)))	// bit1 자동/수동, bit0 전원 ON/OFF
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
		
		pTx->buf[pTx->count++]	= (uint8_t)(1 + Light_Cnt() + Color_Temp_Cnt());	//LENGTH, 전등 수 + 색온도 조명 수 + 1
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
	pTx->buf[pTx->count++]	= 0x00;													//DATA0, 에러 상태
	
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
				data |= (uint8_t)(pG_State->Dimming_Level.Dimming2 << 4);	//bit7 ~ bit4, 디밍 레벨
				data |= (1 << 1);											//bit1, 디밍 기능 유무
				data |= KDW_LIGHT_Data_Conversion(item);						//상태 On/Off 상태
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
	if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) ==  KDW_Color_Temp_Check())			//색온도 조명 번호와 수신 된 색온도 제어 전등 번호가 동일할때만 응답.
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
	
	if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == 0x0F )	//그룹 특성 요구일 경우만
	{
		pTx->buf[pTx->count++] = 0x0A;																			//LENGTH, DATA0 ~ DATA9
		pTx->buf[pTx->count++] = 0;																				//DATA0, 에러 상태
		pTx->buf[pTx->count++] = (uint8_t)(pG_Config->LightCount + pG_Config->ThreeWayCount);					//DATA1, 전등 수
		pTx->buf[pTx->count++] = pG_Config->DimmingCount;														//DATA2, 디밍 수
		pTx->buf[pTx->count++] = KDW_LIGHT_Character_Flag();													//DATA3, 전등 종류 구분(1 ~ 8번 상태)
		pTx->buf[pTx->count++] = 0;																				//DATA4, 전등 종류 구분(9 ~ E번 상태)
		pTx->buf[pTx->count++] = KDW_3way_Check();																//DATA5, 3로 스위치 연동 되는 경우 해당 조명의 SUB ID(1 ~ 14), 없으면 0x00
		if(pG_Config->Enable_Flag.PWM_Dimming)		pTx->buf[pTx->count++] = pG_Config->Dimming_MAX_Level;		//DATA6, 디밍 단계 제어 기능 지원 시 최대 단계 입력, 없으면 0
		else										pTx->buf[pTx->count++] = 0;									//디밍 단계 제어 기능 지원 시 최대 단계 입력, 없으면 0
		if(pG_Config->Enable_Flag.PWM_Color_Temp)	pTx->buf[pTx->count++] = pG_Config->Color_Temp_MAX_Level;	//DATA7, 색온도 단계 제어 기능 지원 시 최대 단계 입력, 없으면 0
		else										pTx->buf[pTx->count++] = 0;									//색온도 단계 제어 기능 지원 시 최대 단계 입력, 없으면 0	
		pTx->buf[pTx->count++] = KDW_LIGHT_Character_Color_Temp_Flag();											//DATA8, 1 ~ 8 조명 타입(bit1이면 색온도 기능 있음)
		pTx->buf[pTx->count++] = 0;																				//DATA9, 9 ~ E 조명 타입(bit1이면 색온도 기능 있음)	
	}
	/*else if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) <= pG_Config->LightCount)	//DEV_SUB_ID가 0x01 ~ 0x0E로 요청 경우
	{
		pTx->buf[pTx->count++] = 0x05;																//LENGTH
		pTx->buf[pTx->count++] = 0;																	//DATA0, 에러 상태
		pTx->buf[pTx->count++] = KDW_LIGHT_Model_Select_Character_Data(item, KDW_LIGHT_ON_n_OFF);	//DATA1, 전등 수
		pTx->buf[pTx->count++] = KDW_LIGHT_Model_Select_Character_Data(item, KDW_LIGHT_DIMMING);	//DATA2, 디밍 수
		pTx->buf[pTx->count++] = KDW_LIGHT_Character_Flag();										//DATA3, 전등 종류 구분(1 ~ 8번 상태)
		pTx->buf[pTx->count++] = 0;																	//DATA4, 전등 종류 구분(9 ~ E번 상태)		
	}*/
	//개별 특성 응답 사용하지 않음.
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
				case KDW_STATE_REQ:	//상태 요구
					KDW_LIGHT_Model_Data_Res(pRx);
					break;
				case KDW_CHARACTER_REQ:	//특성 요구
					KDW_LIGHT_Model_Character_Data_Res(pRx);
					break;
				case KDW_SELECT_CONTROL_REQ:	//개별 제어 요구
					if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) != 0x0F)	//그룹 ID 1인경우 0x1F로 데이터 왔을 때 응답하기 때문에 추가
					{
						KDW_Light_Control(KDW_LIGHT_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1], pRx->buf[KDW_FRAME_DATA_0]);
						KDW_LIGHT_Model_Control_Res(pRx);
					}
					break;
				case KDW_ALL_CONTROL_REQ:		//전체 제어 요구
				case KDW_BATCHLIGHT_CONTROL_REQ:	//일괄 제어 요구
					KDW_Light_All_Control(pRx, pRx->buf[KDW_FRAME_DATA_0]);
					break;
				case KDW_3WAY_CONTROL_REQ:	//특성조회에서 ID를 응답하면 사용하지 않음.
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
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
		/*if(pRx->buf[2] != pG_Config->RS485_ID)			//??로±×로¿¡¼­ 설정이 스위치 ID¿? 스위치?? ID가 ´?¸?¸?
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
#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)	// 2°³¿?	- 상태+??¿­¸ð?¨

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
	if(Watt <= 100)	//자동 차단 ¼³?¤°ª?? 100W 이내?? 때만¸¸ 설정 °ª?? 상태. 일괄??¸? ±??¸ ¼³?¤°ª 상태.
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

		if(Gu16_LCD_Watt_1 >= 3300)						state |= (1 << 5);	//과부하 상태
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

		if(Gu16_LCD_Watt_2 >= 3300)						state |= (1 << 5);	//과부하 상태
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

			if(Gu16_LCD_Watt_1 >= 3300)						state |= (1 << 5);	//과부하 상태
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

			if(Gu16_LCD_Watt_2 >= 3300)						state |= (1 << 5);	//과부하 상태
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

	info |= (1 << 7);	//bit7 : 자동차단 기능 유무(ON : 1, OFF : 0)
	info |= (1 << 6);	//bit6 : 유효전력 측정 기능 유무(ON : 1, OFF : 0)
	info |= (1 << 5);	//bit5 : 과부하 차단 기능 유무(ON : 1, OFF : 0)

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
				pTx->buf[pTx->count++]	= 4;																								//LENGTH, 3 * 닫힘 채널(1) + 1
				pTx->buf[pTx->count++]	= 0;																								//DATA0, 에러 상태
				pTx->buf[pTx->count++]	= KDW_ELEC_State_Res(pRx, 0);																		//DATA1, 1번 채널 상태
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, HUNDRED, WATT_TYPE_LCD) | KDW_Watt2BCD(item, TEN, WATT_TYPE_LCD));	//DATA2, 채널 상태
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, ONE, WATT_TYPE_LCD) | KDW_Watt2BCD(item, DECIMAL, WATT_TYPE_LCD));	//DATA3, 해당  채널 상태
				break;
			case KDW_ELEC_ALL:
				pTx->buf[pTx->count++]	= 7;																								//LENGTH, 3 * 채널수(2) + 1, 대기전력 스위치?? 채널은 2로 고정??.
				pTx->buf[pTx->count++]	= 0;																								//DATA0, 에러 상태
				pTx->buf[pTx->count++]	= KDW_ELEC_State_Res(pRx, 1);																		//DATA1, 1번 채널 상태
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, HUNDRED, WATT_TYPE_LCD)
				| KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, TEN, WATT_TYPE_LCD));															//DATA2, 1번 채널 상태
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, ONE, WATT_TYPE_LCD)
				| KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, DECIMAL, WATT_TYPE_LCD));														//DATA3, 1번 채널 상태
				pTx->buf[pTx->count++]	= KDW_ELEC_State_Res(pRx, 2);																		//DATA4, 2번 채널 상태
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, HUNDRED, WATT_TYPE_LCD)
				| KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, TEN, WATT_TYPE_LCD));															//DATA5, 2번 채널 상태
				pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, ONE, WATT_TYPE_LCD)
				| KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, DECIMAL, WATT_TYPE_LCD));														//DATA6, 2번 채널 상태				
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

	if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == 0x0F)	//전체 요청일 경우만
	{
		pTx->count	= KDW_FRAME_HD;											
		pTx->buf[pTx->count++]	= KDW_HEADER;											//HEADER
		pTx->buf[pTx->count++]	= KDW_ELEC_DEVICE_ID;									//DEVICE ID
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DEV_SUB_ID];						//DEVICE SUB ID
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_CMD_TYPE] | 0x80);		//COMMAND TYPE
		pTx->buf[pTx->count++]	= 4;													//LENGTH, 채널수(n)+2
		pTx->buf[pTx->count++]	= 0;													//DATA0, 에러 상태
		pTx->buf[pTx->count++]	= 2;													//DATA1, 대기전력 채널 개수(n)
		pTx->buf[pTx->count++]	= KDW_ELEC_Function_Info();								//DATA2, 1번 대기전력 채널 기능
		pTx->buf[pTx->count++]	= KDW_ELEC_Function_Info();								//DATA3, 2번 대기전력 채널 기능
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);
		pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);
		TxBuf.send_flag	= 1;	
	}
	/*else	//DEV_SUB_ID가 ID|0x01 ~ ID|0x0E 일 때, 그룹 제어시에는 0x1F ~ 0xEF만 사용함.
	{
		pTx->buf[pTx->count++] = 3;										//LENGTH, 채널수(n)+2
		pTx->buf[pTx->count++] = 0;										//DATA0, 에러 상태
		pTx->buf[pTx->count++] = 1;										//DATA1, 대기전력 채널 개수(n) = 1
		pTx->buf[pTx->count++] = KDW_ELEC_Function_Info();				//DATA2, 해당 채널 상태
	}*/	
}

uint8_t KDW_Elec_Model_Control_Data(uint8_t Sel)
{
	uint8_t touch_switch = 0, data = 0;

	if(Sel == KDW_ELEC_1)
	{
		touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_1);
		if(pG_State->ETC.Auto1)				data |= (1 << 1);	//자동 차단 설정 유무, 1 : 설정, 0 : 해제
		else								data |= (0 << 1);
	}
	else if(Sel == KDW_ELEC_2)
	{
		touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_2);
		if(pG_State->ETC.Auto2)				data |= (1 << 1);	//자동 차단 설정 유무, 1 : 설정, 0 : 해제
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
		pTx->buf[pTx->count++]	= (uint8_t)(pRx->buf[KDW_FRAME_LENGTH] + 1);			//LENGTH, 채널 수(n) + 1
		pTx->buf[pTx->count++]	= 0;													//DATA0, 에러 상태

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
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x10) == 0x10)			//채널 상태 변경
			{
				if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_1)			item = mapping_ITEM_ELECTRICITY_1;
				else if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_2)		item = mapping_ITEM_ELECTRICITY_2;
				
				if(pRx->buf[KDW_FRAME_DATA_0] & 0x01)				Flag = KDW_ON_FLAG;
				else												Flag = KDW_OFF_FLAG;

				KDW_Elec_Control_Function(item, Flag);
			}
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x20) == 0x20)			//자동 차단기능 변경
			{
				if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_1)
				{
					if(pRx->buf[KDW_FRAME_DATA_0] & 0x02)		SET_Elec_Auto_Manual(KDW_ELEC_1, KDW_ON_FLAG);	//자동 차단 설정
					else										SET_Elec_Auto_Manual(KDW_ELEC_1, KDW_OFF_FLAG);	//자동 차단 해제
				}
				else if((pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) == KDW_ELEC_2)
				{
					if(pRx->buf[KDW_FRAME_DATA_0] & 0x02)		SET_Elec_Auto_Manual(KDW_ELEC_2, KDW_ON_FLAG);	//자동 차단 설정
					else										SET_Elec_Auto_Manual(KDW_ELEC_2, KDW_OFF_FLAG);	//자동 차단 해제
				}
			}
			break;
		case KDW_ELEC_ALL:
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x10) == 0x10)
			{
				if(pRx->buf[KDW_FRAME_DATA_0+i] & 0x01)	Flag = ON;		//0x01 -> 대기전력 차단X
				else									Flag = OFF;		//0x00 -> 대기전력 차단O
				KDW_Elec_Control_Function(mapping_ITEM_ELECTRICITY_1, Flag);
				KDW_Elec_Control_Function(mapping_ITEM_ELECTRICITY_2, Flag);
			}
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x20) == 0x20)
			{
				if(pRx->buf[KDW_FRAME_DATA_0] & 0x02)
				{
					SET_Elec_Auto_Manual(KDW_ELEC_1, KDW_ON_FLAG);	//자동 차단 설정
					SET_Elec_Auto_Manual(KDW_ELEC_2, KDW_ON_FLAG);	//자동 차단 설정
				}
				else
				{
					SET_Elec_Auto_Manual(KDW_ELEC_1, KDW_OFF_FLAG);	//자동 차단 해제
					SET_Elec_Auto_Manual(KDW_ELEC_2, KDW_OFF_FLAG);	//자동 차단 해제
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
			pTx->buf[pTx->count++]	= 3;			//LENGTH, 2 * 채널 수(n) + 1
			pTx->buf[pTx->count++]	= 0;			//DATA0, 에러 상태
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(item, TEN, WATT_TYPE_LIMIT));	//bit7 ~ 4 : 100W 자리 BCD, bit3 ~ 0 : 10W 자리 BCD
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(item, DECIMAL, WATT_TYPE_LIMIT));		//bit7 ~ 4 : 1W 자리 BCD, bit3 ~ 0 : 0.1W 자리 BCD
			break;
		case KDW_ELEC_ALL:
			pTx->buf[pTx->count++]	= 5;			//LENGTH, 2 * 채널 수(n) + 1
			pTx->buf[pTx->count++]	= 0;			//DATA0, 에러 상태
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, TEN, WATT_TYPE_LIMIT));	//bit7 ~ 4 : 100W 자리 BCD, bit3 ~ 0 : 10W 자리 BCD
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_1, DECIMAL, WATT_TYPE_LIMIT));		//bit7 ~ 4 : 1W 자리 BCD, bit3 ~ 0 : 0.1W 자리 BCD
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, TEN, WATT_TYPE_LIMIT));	//bit7 ~ 4 : 100W 자리 BCD, bit3 ~ 0 : 10W 자리 BCD
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(mapping_ITEM_ELECTRICITY_2, DECIMAL, WATT_TYPE_LIMIT));		//bit7 ~ 4 : 1W 자리 BCD, bit3 ~ 0 : 0.1W 자리 BCD						
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
									//DATA0, 에러 상태
	item = KDW_ELEC_ITEM_Sequence[(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F) - 1];

	switch(pRx->buf[KDW_FRAME_DEV_SUB_ID] & 0x0F)
	{
		case KDW_ELEC_1:
		case KDW_ELEC_2:
			pTx->buf[pTx->count++]	= 3;			//LENGTH, 2 * 채널 수(n) + 1 
			pTx->buf[pTx->count++]	= 0;	
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, HUNDRED, WATT_TYPE_LIMIT) | KDW_Watt2BCD(item, TEN, WATT_TYPE_LIMIT));
			pTx->buf[pTx->count++]	= (uint8_t)(KDW_Watt2BCD(item, ONE, WATT_TYPE_LIMIT) | KDW_Watt2BCD(item, DECIMAL, WATT_TYPE_LIMIT));
			break;
		case KDW_ELEC_ALL:
			pTx->buf[pTx->count++]	= 5;			//LENGTH, 2 * 채널 수(n) + 1 
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
			case KDW_STATE_REQ:	//상태 요구
				if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)		KDW_LIGHT_Model_Data_Res(pRx);
				else if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_ELEC_DEVICE_ID)	KDW_ELEC_Model_Data_Res(pRx);
				break;
			case KDW_CHARACTER_REQ:	//특성 요구
				if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_LIGHT_DEVICE_ID)		KDW_LIGHT_Model_Character_Data_Res(pRx);
				else if(pRx->buf[KDW_FRAME_DEV_ID] == KDW_ELEC_DEVICE_ID)	KDW_ELEC_Model_Character_Data_Res(pRx);
				break;
			case KDW_SELECT_CONTROL_REQ:	//개별 제어 요구
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
			case KDW_ALL_CONTROL_REQ:		//전체 제어 요구
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
			case KDW_AUTO_BLOCK_VALUE_REQ:	//자동 차단 설정 값 요구
				KDW_Auto_Block_Value_Res(pRx);
				break;
			case KDW_AUTO_BLOCK_VALUE_SET_REQ:	//자동 차단 설정 값 설정 요구
			// case KDW_BATCHLIGHT_CONTROL_REQ:	//차단 설정값 설정 요구와 일괄 제어 요구의 커맨드 값이 동일함.
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
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
		/*
		if(pRx->buf[2] != pG_Config->RS485_ID)			//??로±×로¿¡¼­ 설정이 스위치 ID¿? 스위치?? ID가 ´?¸?¸?
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
			Store_CurrentConfig();
			printf("Switch ID Change\r\n");
		}*/
		RS_485_ID_RES();
	}
}
#endif	// defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
//------------------------------------------------------------------------------------------------------------------------------------------
#ifdef	_ONE_SIZE_BATCH_BLOCK_MODEL_				// 1°³¿? - 일괄차단스위치
//--------------------------------------일괄차단 스위치 ??·상태?-----------------------------------------------------------------------------------------------
// #if 1	//일괄차단 스위치 ??로상태 ?제어? 때

uint8_t KDW_BATCH_BLOCK_User_Req(void)
{
	uint8_t touch_switch, data = 0;

	data |= (0 << 3);	//대기전력 차단 상태 ON/OFF. 1 : ON, 0 : OFF
	data |= (0 << 1);	//외출 설정 요구. 1 : 요구, 0 : 요구 없음

	if(item2tsn(mapping_ITEM_3WAY_1))
	{
		touch_switch = item2tsn(mapping_ITEM_3WAY_1);
		if(GET_Switch_State(touch_switch))				data |= (1 << 6);	//3로 스위치 제어 요구 ¹× ON/OFF 상태. 특성X : 1 : 요구, 0 : 요구 없음 // 특성O : 1 : ON, 0 : OFF
		else											data |= (0 << 6);
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		touch_switch = item2tsn(mapping_ITEM_ELEVATOR);
		if(GET_LED_State(touch_switch) == LED_FLASHING)	data |= (1 << 5);	//엘리베이터 하강 호출 요구. 1 : 요구, 0 : 요구 없음
		else											data |= (0 << 5);

		data |= (0 << 4);	//엘리베이터 상태 상승 호출 요구. 상승 호출 기능 1 : 요구, 0 : 요구 없음.
	}
	if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF))
	{
		touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
		if(GET_Switch_State(touch_switch))				data |= (1 << 2);	//일괄 상태 차단 릴레이 ON/OFF 상태. 1 : ON, 0 : OFF
		else											data |= (0 << 2);
	}
	if(item2tsn(mapping_ITEM_GAS))
	{
		touch_switch = item2tsn(mapping_ITEM_GAS);
		if(GET_LED_State(touch_switch) == LED_FLASHING)	data |= (1 << 0);	//가스 차단 요구. 1 : 요구, 0 : 요구 없음
		else											data |= (0 << 0);
	}
	if(item2tsn(mapping_ITEM_COOK))
	{
		touch_switch = item2tsn(mapping_ITEM_COOK);
		if(GET_LED_State(touch_switch) == LED_FLASHING)	data |= (1 < 7);	//쿡탑 끄기 요구. 1 : 요구, 0 : 요구 없음
		else											data |= (0 < 7);
	}
	return data;
}

uint8_t KDW_BATCH_BLOCK_User_Function(void)
{
	uint8_t data = 0;
	
	data |= (0 << 7);	//주차위치 표기 기능 유무
	data |= (0 << 5);	//생활정보 차단 기능 유무
	data |= (0 << 4);	//엘리베이터 층 표시 기능 유무
	data |= (0 << 2);	//대기전력 제어 기능 유무
	data |= (0 << 1);	//외출 설정 기능 유무

	if(item2tsn(mapping_ITEM_3WAY_1))	data |= (1 << 6);	//3로 스위치 상태 연동 기능 유무
	else								data |= (0 << 6);
	if(item2tsn(mapping_ITEM_ELEVATOR))	data |= (1 << 3);	//엘리베이터 호출 상태 기능 유무
	else								data |= (0 << 3);
	if(item2tsn(mapping_ITEM_GAS))		data |= (1 << 0);	//가스 차단 기능 유무
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
	pTx->buf[pTx->count++]	= 0x03;											//LENGTH, 0x03 고정
	pTx->buf[pTx->count++]	= 0;											//DATA0, 에러 상태

	if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_STATE_REQ || pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_SELECT_CONTROL_REQ)
	{
		pTx->buf[pTx->count++]	= KDW_BATCH_BLOCK_User_Req();				//DATA1, 스위치 상태 : 쿡탑, 3로, ¿¤¸®º상태?, 대기전력, 일괄, 요청, 가스
		pTx->buf[pTx->count++]	= 0;										//DATA2, 스위치 상태 : ³­¹æ(요청¸ð??) 설정 요구
	}
	else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_CHARACTER_REQ)
	{
		pTx->buf[pTx->count++]	= KDW_BATCH_BLOCK_User_Function();			//DATA1, 스위치 기능 유무 : 주차위치 표기, 3로, 생활정보 표기, 엘리베이터 층 표시 기능, 엘리베이터 호출 기능, 대기전력, 외출 설정, 가스 차단
		pTx->buf[pTx->count++]	= 0;										//DATA2, 스위치 기능 유무 : °­¼?·®, 쿡탑 제어, ³­¹æ 제어
	}
	else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_USER_REQ_RESULT)
	{
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DATA_0];				//DATA1, 결과 전달의 DATA0과 데이터 동일
		pTx->buf[pTx->count++]	= pRx->buf[KDW_FRAME_DATA_1];				//DATA2, 결과 전달의 DATA1과 데이터 동일
	}
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, XOR_SUM);
	pTx->buf[pTx->count++]	= KDW_Crc(pTx, pTx->count, ADD_SUM);
	TxBuf.send_flag	= 1;	
}

void KDW_State_Req_Control(KDW_BUF *pRx)
{
	if(item2tsn(mapping_ITEM_GAS))	//가스 사용하는 모델이면
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) != LED_FLASHING)			//스위치에서 가스 차단 요청중이 아닐 경우
		{
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == KDW_GAS_OPEN)				//가스 열림 상태로 월패드에서 전달오면
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
			}
			else if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == KDW_GAS_CLOSE)			//가스 닫힘 상태로 월패드에서 전달오면
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

void KDW_User_Req_Control(KDW_BUF *pRx)	//사용자 요청 처리 결과 후 제어
{
	if(item2tsn(mapping_ITEM_GAS))
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)	//스위치에서 가스 차단 요청중일 때만
		{
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == 0x01)			//가스 잠금요구 확인
			{
				BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);			//가스 차단
			}
			else if((pRx->buf[KDW_FRAME_DATA_0] & 0x02) == 0x02)		//가스 잠금 실패
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);			//가스 차단 실패
			}
		}
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)
		{
			if((pRx->buf[KDW_FRAME_DATA_0] & 0x10) == 0x10)		//엘리베이터 호출 확인
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL);		//엘리베이터 호출 성공
			}
			else if((pRx->buf[KDW_FRAME_DATA_0] & 0x20) == 0x20)	//엘리베이터 호출 실패
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);	//엘리베이터 호출 실패
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
			if(GET_Switch_State(touch_switch) == OFF)	EventCtrl(touch_switch, ON);//3로 전등 ON
		}
		else if(pRx->buf[KDW_FRAME_DATA_0] == KDW_OFF_FLAG)
		{
			if(GET_Switch_State(touch_switch))			EventCtrl(touch_switch, OFF);//3로 전등 OFF
		}
	}
}

void KDW_COOK_Control(KDW_BUF *pRx)
{
	if(item2tsn(mapping_ITEM_COOK))
	{
		if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_COOKTOP_STATE_NOTICE)
		{
			if(GET_Switch_State(item2tsn(mapping_ITEM_COOK)) != LED_FLASHING)	//요청 상태가 아닐 때
			{
				if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == 0x01)	BATCH_BLOCK_Control(SET__COOK_OPEN_STATE);
				else											BATCH_BLOCK_Control(SET__COOK_CLOSE_STATE);
			}
		}
		else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_COOKTOP_CONTROL_RESULT)
		{
			if(GET_Switch_State(item2tsn(mapping_ITEM_COOK)) == LED_FLASHING)	//요청 상태일 경우만
			{
				if((pRx->buf[KDW_FRAME_DATA_0] & 0x01) == 0x01)			BATCH_BLOCK_Control(SET__COOK_CLOSE_STATE);	//쿡탑 끄기 요구 확인 1 : 확인, 0 : 해당 없음
				else if((pRx->buf[KDW_FRAME_DATA_0] & 0x02) == 0x02)	BATCH_BLOCK_Control(SET__COOK_OPEN_STATE);	//쿡탑 끄기 실패 1 : 실패, 0 : 해당 없음
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
			case KDW_STATE_REQ:				//상태 요구
			case KDW_SELECT_CONTROL_REQ:	//개별 제어 요구
			case KDW_CHARACTER_REQ:			//특성 요구, KDW_CHARACTER_REQ는 제어 없이 응답만
			case KDW_USER_REQ_RESULT:		//처리 결과 전달
				if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_STATE_REQ)				KDW_State_Req_Control(pRx);
				else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_SELECT_CONTROL_REQ)	KDW_Select_Control(pRx);
				else if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_USER_REQ_RESULT)	KDW_User_Req_Control(pRx);
				KDW_BATCH_BLOCK_Data_Res(pRx);
				break;
			case KDW_ELEVATOR_ARRIVE:		//엘리베이터 도착 데이터. 응답없음
				BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);
				break;					
			case KDW_THREE_WAY_STATE_NOTICE:	//3로 연동 조명 스위치 상태 알림. 응답없음
				KDW_THREEWAY_Control(pRx);
				break;
			case KDW_COOKTOP_STATE_NOTICE:		//쿡탑 기능 사용 시 적용
			case KDW_COOKTOP_CONTROL_RESULT:	//쿡탑 기능 사용 시 적용
				KDW_COOK_Control(pRx);
				break;
		}
	}
	else
	{
		if(pRx->buf[KDW_FRAME_DEV_SUB_ID] == 0x0F || pRx->buf[KDW_FRAME_DEV_SUB_ID] == 0xFF)
		{
			if(pRx->buf[KDW_FRAME_CMD_TYPE] == KDW_ALL_CONTROL_REQ)	//응답없음
			{
				if((pRx->buf[KDW_FRAME_DATA_0] & (1 << (Get_485_ID() - 1))) == (1 << (Get_485_ID() - 1)))	//ex)ID : 1, bit0 : 1??¸?, ON, ID : 2, bit1 : 1??¸? ON
				{//0xff (1 << 2 - 1) == 1 << 2 - 1
					BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);	//프로토콜에선 차단이 1이지만 문의 답변으로는 1이 ON이라고 함.
				}
				else		//ex)ID : 1, bit0 : 0??¸? OFF
				{
					BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
				}
			}
		}
	}
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		//현재 ID는 변경해도 저장되지 않고, ID 검사시만 변경되기 때문에 의미없어서 주석 처리함.
		/*/
		if(pRx->buf[2] != pG_Config->RS485_ID)			//??로±×로¿¡¼­ 설정이 스위치 ID¿? 스위치?? ID가 ´?¸?¸?
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
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
			if(GET_LED_State(item2tsn((uint8_t)mapping_ITEM_GAS)) == LED_FLASHING)		// 프로토콜에 대해 설정이 없으면 OPEN 상태로 전환
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

	if(item2tsn(mapping_ITEM_ELEVATOR))		//호출 요청 or 호출 중일 때
	{
		if(Gu16_Elevator_Tmr == 0)	//KDW은 상태 ?? 3분 이내 도착 데이터가 오지 않아도 상태 초기화 해야함.
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
		case SET__GAS_CLOSE_REQUEST:		// 가스차단은 스위치 상태가 켜져 있어야 차단이 설정된 상태이다
		case SET__COOK_CLOSE_REQUEST:
			if(control == SET__GAS_CLOSE_REQUEST)		touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			else if(control == SET__COOK_CLOSE_REQUEST)	touch_switch = item2tsn((uint8_t)mapping_ITEM_COOK);

			if(GET_Switch_State(touch_switch) == OFF || GET_LED_State(touch_switch) == LED_FLASHING)	// 가스벨브가 열려(OFF)있거나, 차단 요청(LED_FLASHING)중일 경우
			{
				Gu16_GAS_Off_Tmr	= GAS_TIME_OUT;		// 60초 경과 후 LED 소등(단 월패드에서 close/open 데이터가 수신되면 해당 상태로 전환)
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
		case SET__COOK_CLOSE_STATE:
			if(control == SET__GAS_CLOSE_STATE)			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			else if(control == SET__COOK_CLOSE_REQUEST)	touch_switch = item2tsn((uint8_t)mapping_ITEM_COOK);

			if(GET_Switch_State(touch_switch) == 0)		//요청중이거나, 차단 해제  상태일때만,
			{
				SET_Switch_State(touch_switch, ON);		// 가스밸브 차단(닫힘)
				SET_LED_State(touch_switch, OFF);		// 실제로는 LED 켜짐
				Beep(ON);
			}
			break;
			
		case SET__GAS_OPEN_STATE:
		case SET__COOK_OPEN_STATE:
			if(control == SET__GAS_OPEN_STATE)			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			else if(control == SET__COOK_CLOSE_REQUEST)	touch_switch = item2tsn((uint8_t)mapping_ITEM_COOK);

			if(GET_Switch_State(touch_switch) || (GET_LED_State(touch_switch) == LED_FLASHING))
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
				Gu16_Elevator_Tmr = ELEVATOR_TMR;															//KDW는 호출 시 5초 이내 결과 전송 없으면 해제함.
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
				Gu16_Elevator_Tmr = 30;																		//층 표시 없을 경우에는 임의로 30초 동안 호출 상태로 함
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

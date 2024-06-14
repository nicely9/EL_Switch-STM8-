/************************************************************************************
	Project		: 전자식 스위치
	File Name	: RS-485_NO.C
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

#ifdef _NO_PROTOCOL_


void Noprotocol_Data_Process(NO_BUF	*pRx);
void NO_Protocol_Control(uint8_t control);
void Noprotocol_REQ(void);
// ----------------------------------------------------------------------------------------
static	NO_BUF		RxBuf, TxBuf;
uint16_t Gu16_Noprotocol_Req_Tmr = 0;
uint8_t Gu8_Trans_Control = 0;
uint8_t Gu8_Direct_Control = 0;
// ----------------------------------------------------------------------------------------

void Protocol_Data_Init(void)
{
	memset((void*)&RxBuf,		0,	sizeof(NO_BUF));
	memset((void*)&TxBuf,		0,	sizeof(NO_BUF));
}
// ----------------------------------------------------------------------------------------
void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr)
{
	if(tmr == 0)
	{
		RxBuf.buf[0]	= 0;
		RxBuf.count	    = 0;
	}
}

uint8_t Get_485_ID(void)
{
	return	(uint8_t)pG_Config->RS485_ID;
}
uint8_t NIS_Crc(NO_BUF *pTRx, uint8_t cal, uint8_t sel)
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

void RS_485_ID_RES(void)
{
    uint8_t Elec_Watt_MSB, Elec_Watt_LSB, prime_num_1, prime_num_2 = 0;
	NO_BUF	*pTx;
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
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;
	pTx->buf[pTx->count++]	= Get_485_ID();
	pTx->buf[pTx->count++]	= 0;

	pTx->buf[pTx->count++]	= Elec_Watt_MSB;
	pTx->buf[pTx->count++]	= Elec_Watt_LSB;

	pTx->buf[pTx->count++]	= Gu8_ZeroCrossing_Err_Flag;

	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 1, NIS_TX);
	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 0, NIS_TX);
	// TxBuf.count	= 8;
    TxBuf.send_flag	= 1;    
}
// ----------------------------------------------------------------------------------------
void RS485_Tx_Process(void)
{
	int i;
	
    static uint8_t	_mode_	= 0;

	if(Gu8_RS_485_Tx_Tmr == 0 && Gu8_RS_485_Enable_Tmr == 0)
	{
#if defined THREEWAY_TRANS && defined _ONE_SIZE_BATCH_BLOCK_MODEL_
		if(_mode_ == 0)
		{
			if(GPIO_ReadInputDataBit(RS_485_DE_PORT, (GPIO_Pin_TypeDef)RS_485_DE_PIN) == 0)	// GPIO Read)
			{
				if(Gu16_Noprotocol_Req_Tmr == 0)
				{
					Noprotocol_REQ();
					Gu16_Noprotocol_Req_Tmr = 2;	//500ms
				}
			}
		}
#endif
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
					printf("TX(NO) : ");
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

// ----------------------------------------------------------------------------------------
void Protocol_Process(uint8_t data)
{
	NO_BUF	*pRx;
	uint8_t		crc, crc_xor = 0;
    uint8_t     cnt = 0;
    int i;

	pRx = &RxBuf;	
	if(G_Debug == DEBUG_HOST_REALDATA)
	{
		if(data == NO_STX_1)	printf("\n");
		printf("%02X ", (uint16_t)data);
	}
    switch(pRx->count)
    {
        default:
            if((pRx->buf[NO_F_D_STX_1] != NO_STX_1) && (pRx->buf[0] != NIS_LIGHT_ID_COMM_1) && (data == NIS_LIGHT_ID_COMM_1))
            {
                pRx->count = 0;
            }
            break;
        case 1:
            if((pRx->buf[NO_F_D_STX_1] != NO_STX_1) && pRx->buf[0] != NIS_LIGHT_ID_COMM_1)
            {
                pRx->count = 0;
            }
            break;
        case 2:
			if((pRx->buf[NO_F_D_STX_2] != NO_STX_2) && pRx->buf[1] != NIS_LIGHT_ID_COMM_2)
			{
				pRx->count = 0;
			}
            break;
    }
	pRx->buf[pRx->count++] = data;

    if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
    {
		if(pRx->count >= 8)
		{
			crc = NIS_Crc(pRx, 0, NIS_RX);
			crc_xor = NIS_Crc(pRx, 1, NIS_RX);
			if(crc == pRx->buf[7] && crc_xor == pRx->buf[6])
			{
                if(G_Debug == DEBUG_HOST)
                {
                    printf("\nRX(NO) : ");
                    for(i=0;i<pRx->count;i++)
                    {
                        printf("%02X ", (uint16_t)pRx->buf[i]);
                    }
                    printf("\n");
                }
				Noprotocol_Data_Process(pRx);				
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
		if(pRx->count >= NO_MAX_BUF)
		{
			if(pRx->buf[NO_F_ETX] == NO_ETX)
			{
				crc = NIS_Crc(pRx, 0, NIS_RX);
				
				if(crc == pRx->buf[NO_F_FCC])
				{
					if(G_Debug == DEBUG_HOST)
					{
						printf("\nRX(NO) : ");
						for(i=0;i<pRx->count;i++)
						{
							printf("%02X ", (uint16_t)pRx->buf[i]);
						}
						printf("\n");
					}
					Noprotocol_Data_Process(pRx);
				}
				else
				{
					printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[NO_F_FCC]);
				}
			}
			pRx->buf[0] = 0;
			pRx->count = 0;
		}		
	}
}

#ifdef THREEWAY_TRANS
void Noprotocol_REQ(void)
{
	static uint8_t old_state = 0xFF;
	uint8_t state = 0;

	NO_BUF	*pTx;
	pTx = &TxBuf;

	state = GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1));

	pTx->count = 0;
	pTx->buf[pTx->count++]	= NO_STX_1;
	pTx->buf[pTx->count++]	= NO_STX_2;
	pTx->buf[pTx->count++]	= 0x01;
	if(Gu8_Trans_Control == 0)
	{
		if(old_state != state)
		{
			if(state == NO_ON)	pTx->buf[pTx->count++]	= NO_ON;
			else				pTx->buf[pTx->count++]	= NO_OFF;
		}
		else
		{
			pTx->buf[pTx->count++]	= 0;
		}
	}
	else
	{
		pTx->buf[pTx->count++]	= 0;
		Gu8_Trans_Control = 0;
	}
	pTx->buf[pTx->count++]	= 0;
	pTx->buf[pTx->count++]	= 0;
	pTx->buf[pTx->count++]	= 0;
	pTx->buf[pTx->count++]	= 0;
	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 0, NIS_TX);
	pTx->buf[pTx->count++]	= NO_ETX;
	
	TxBuf.send_flag	= 1;
	old_state = state;
}

void Noprotocol_ACK(NO_BUF * pRx)
{
	NO_BUF	*pTx;
	pTx = &TxBuf;
	pTx->count = 0;
	pTx->buf[pTx->count++]	= NO_STX_1;
	pTx->buf[pTx->count++]	= NO_STX_2;
	pTx->buf[pTx->count++]	= 0x04;
	if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))	pTx->buf[pTx->count++]	= NO_ON;
	else												pTx->buf[pTx->count++]	= NO_OFF;
	pTx->buf[pTx->count++]	= 0;
	pTx->buf[pTx->count++]	= 0;
	pTx->buf[pTx->count++]	= 0;
	pTx->buf[pTx->count++]	= 0;
	pTx->buf[pTx->count++]	= NIS_Crc(pTx, 0, NIS_TX);
	pTx->buf[pTx->count++]	= NO_ETX;

	TxBuf.send_flag	= 1;
}
#endif

void Noprotocol_Data_Process(NO_BUF	*pRx)
{
	Gu8_RS_485_Tx_Tmr = pG_Config->Protocol_RES_DelayTime;
#ifdef THREEWAY_TRANS
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
	if(pRx->buf[NO_F_D_0] == NO_RES)
	{
		if(Gu8_Direct_Control == 0)
		{
			if(pRx->buf[NO_F_D_1] == NO_ON)
			{
				if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == OFF)
				{
					NO_Protocol_Control(SET__THREEWAY_ON);
					Gu8_Trans_Control = 1;
				}
			}
			else if(pRx->buf[NO_F_D_1] == NO_OFF)
			{
				if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
				{
					NO_Protocol_Control(SET__THREEWAY_OFF);
					Gu8_Trans_Control = 1;
				}
			}
		}
		else
		{
			Gu8_Direct_Control = 0;
		}
	}
#else
	if(pRx->buf[NO_F_D_0] == NO_REQ)
	{
		if(Gu8_Direct_Control == 0)
		{
			if(pRx->buf[NO_F_D_1] == NO_ON)
			{
				if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)) == OFF)
				{
					NO_Protocol_Control(SET__THREEWAY_ON);
				}
			}
			else if(pRx->buf[NO_F_D_1] == NO_OFF)
			{
				if(GET_Switch_State(item2tsn(mapping_ITEM_3WAY_1)))
				{
					NO_Protocol_Control(SET__THREEWAY_OFF);
				}
			}
		}
		else
		{
			Gu8_Direct_Control = 0;
		}		
		Noprotocol_ACK(pRx);
	}
#endif	//#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
#endif	//#ifdef THREEWAY_TRANS
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		
		/*
		if(pRx->buf[2] != pG_Config->RS485_ID)
		{
			pG_Config->RS485_ID = pRx->buf[2];
			Store_CurrentConfig();
			printf("Switch ID Change\r\n");
		}*/
		RS_485_ID_RES();
	}	
}

#ifdef THREEWAY_TRANS
void NO_Protocol_Control(uint8_t control)
{
	uint8_t touch_switch, item, Flag = 0;

	switch(control)
	{
		case SET__THREEWAY_ON:
		case SET__THREEWAY_OFF:
			if(control == SET__THREEWAY_ON)	Flag = ON;
			else							Flag = OFF;
			item = mapping_ITEM_3WAY_1;
			touch_switch = item2tsn(item);
			SET_Switch_State(touch_switch, Flag);
			SET_LED_State(touch_switch, Flag);			//LED OFF
			PUT_RelayCtrl(item2ctrl(item), Flag);
			ALL_n_Group_Light_Switch_LED_Ctrl();
			Beep(ON);
			break;
	}
}
#endif/	//THREEWAY_TRANS
#endif	// _NO_PROTOCOL_

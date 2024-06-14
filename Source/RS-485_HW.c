/************************************************************************************
	Project		: ���ڽ� ����ġ
	File Name	: RS-485_HW.C
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

#ifdef _HW_PROTOCOL_

void HW_Data_Process(HW_BUF	*pRx);

// ----------------------------------------------------------------------------------------
static	HW_BUF		RxBuf, TxBuf;
#define ELEVATOR_TMR	7		//
#define GAS_TIME_OUT 	7		 //�������� �� 5�ʰ� ǥ��������, 7�ʷ� ��.

#define	MAX_HW_DATA_SEQUENCE		8
uint8_t	HW_LIGHT_ITEM_Sequence[MAX_HW_DATA_SEQUENCE];
uint8_t	HW_ELEC_ITEM_Sequence[MAX_HW_DATA_SEQUENCE];

// ----------------------------------------------------------------------------------------
void SET_HW_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_HW_DATA_SEQUENCE)
	{
		HW_LIGHT_ITEM_Sequence[count]	= item;
	}
}

void SET_HW_ELEC_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_HW_DATA_SEQUENCE)
	{
		HW_ELEC_ITEM_Sequence[count]	= item;
	}
}

void Protocol_Data_Init(void)
{
	uint8_t	i, count;
	
	memset((void*)&RxBuf,		0,	sizeof(HW_BUF));
	memset((void*)&TxBuf,		0,	sizeof(HW_BUF));
	
	Gu16_GAS_Off_Tmr				= 0;
	
	Gu16_Elevator_Tmr			= 0;
	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_NON;
	
	memset(HW_LIGHT_ITEM_Sequence, 0, MAX_HW_DATA_SEQUENCE);
	memset(HW_ELEC_ITEM_Sequence, 0, MAX_HW_DATA_SEQUENCE);
	
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_
	count	= 0;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
#endif
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_
	count	= 0;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_HW_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
#endif

#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)		// ������� ����?�� 
	count	= 0;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_HW_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_HW_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
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
					printf("TX(HW) : ");
					for(i=0;i<TxBuf.count;i++)
					{
						printf("%02X ", (uint16_t)TxBuf.buf[i]);
					}
					printf("\n");
				}
				Gu8_RS_485_Tx_Tmr	= 2;		// ��? 2ms ���¢��� USART_IT_TC ?��??
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
uint8_t NIS_Crc(HW_BUF *pTRx, uint8_t cal, uint8_t sel)		//����� 485��� �׽�Ʈ�� ���ؼ� �߰���.
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
	HW_BUF	*pTx;
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
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//??�λ��� ?��??
    pTx->buf[pTx->count++]	= Get_485_ID();	
	pTx->buf[pTx->count++]	= 0;

	pTx->buf[pTx->count++]	= Elec_Watt_MSB;				//??����1, 2?? ??�� ??(?����?)
	pTx->buf[pTx->count++]	= Elec_Watt_LSB;				//??����1, 2?? ??�� ??(��?��?)

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
// ----------------------------------------------------------------------------------------
void Protocol_Process(uint8_t data)
{
	HW_BUF	*pRx;
	uint8_t		crc, xor_crc = 0;
	uint8_t     cnt = 0;
	int i;
	
	pRx = &RxBuf;
	
	if(G_Debug == DEBUG_HOST_REALDATA)
	{
		// if(data == HW_HEADER)	printf("\n");
		printf("%02X ", (uint16_t)data);
	}
	switch(pRx->count)
	{
		default:
			if((((pRx->buf[0] & 0xF0) != 0x30) && ((data & 0xF0) == 0x30)) && ((pRx->buf[0] != 0x50) && (data == 0x50)) && ((pRx->buf[0] != 0xFF) && (data == 0xFF)))
			{
				pRx->count = 0;
			}
			break;
		case 1:		// HD
			if(((pRx->buf[0] & 0xF0) != 0x30) && (pRx->buf[0] != 0x50) && (pRx->buf[0] != 0xFF) && (pRx->buf[0] != NIS_LIGHT_ID_COMM_1))
			{
				pRx->count = 0;
			}
			break;
		case 2:		// DEV_ID
			if((pRx->buf[1] != 0x3D) && (pRx->buf[1] != 0x4F) && (pRx->buf[1] != NIS_LIGHT_ID_COMM_2))
			{
				pRx->count = 0;
			}
			break;
		case 3:		// DEV_SUB_ID
            if((pRx->buf[0] & 0xF0) == 0x30)
            {
                if(pRx->buf[2] != 0x30 && pRx->buf[2] != 0x31)
                {
                    pRx->count = 0;
                }
            }
            else if(pRx->buf[0] == 0x50)
            {
                if(pRx->buf[2] != 0x4C)
                {
                    pRx->count = 0;
                }
            }
			break;
		case 4:
            if(((pRx->buf[0] & 0xF0) == 0x30) || pRx->buf[0] == 0xFF)
            {
                if(pRx->buf[3] != 0x21)				//���� ����̽�
                {
                    pRx->count = 0;
                }
            }
            else if(pRx->buf[0] == 0x50)
            {
                if(pRx->buf[3] != 0x4C)
                {
                    pRx->count = 0;
                }
            }
			break;
        case 5:
            if(pRx->buf[0] == 0x50)
            {
                if(pRx->buf[4] != 0x21)
                {
                    pRx->count = 0;
                }
            }
            break;
	}
	pRx->buf[pRx->count++] = data;
	
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
	{
		if(pRx->count >= 8)
		{
			crc = NIS_Crc(pRx, 0, NIS_RX);
			xor_crc = NIS_Crc(pRx, 1, NIS_RX);
			if(crc == pRx->buf[7] && xor_crc == pRx->buf[6])
			{
				if(G_Debug == DEBUG_HOST)
				{
					printf("\nRX(HW) : ");
					for(i=0;i<pRx->count;i++)
					{
						printf("%02X ", (uint16_t)pRx->buf[i]);
					}
					printf("\n");
				}
				HW_Data_Process(pRx);				
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
		// printf("HW count = %d", (uint16_t)pRx->count);
		if(pRx->count >= HW_MAX_BUF)
		{
			pRx->buf[0] = 0;
			pRx->count = 0;
        }
        if((pRx->buf[0] & 0xF0) == 0x30 || pRx->buf[0] == 0xFF)
        {
            if(pRx->count >= 5)
            {
                if(pRx->buf[4] == 0x0D)
                {
                    if(G_Debug == DEBUG_HOST)
                    {
                        printf("\nRX(HW) : ");
                        for(i=0;i<pRx->count;i++)
                        {
                            printf("%02X ", (uint16_t)pRx->buf[i]);
                        }
                        printf("\n");
                    }
                    HW_Data_Process(pRx);
                }
                pRx->buf[0] = 0;
                pRx->count = 0;
            }
        }
        else if(pRx->buf[0] == 0x50)
        {
            if(pRx->count >= 6)
            {
                if(pRx->buf[5] == 0x0D)
                {
                    if(G_Debug == DEBUG_HOST)
                    {
                        printf("\nRX(HW) : ");
                        for(i=0;i<pRx->count;i++)
                        {
                            printf("%02X ", (uint16_t)pRx->buf[i]);
                        }
                        printf("\n");
                    }
                    HW_Data_Process(pRx);
                }
                pRx->buf[0] = 0;
                pRx->count = 0;
            }
        }
	}
}

void HW_Light_Control(uint8_t item, uint8_t control_value)
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
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if((control_value & 0x0F) == 0x01)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);
			}
			else
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);
			}
			Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
			break;
	}
}


uint8_t HW_LIGHT_Data_Conversion(uint8_t item)
{
	uint8_t	ret = 0;
	
	switch(item)
	{
		default:
			ret = 0;
			break;
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
			if(GET_Switch_State(item2tsn((uint8_t)item)))
			{
				ret = 1;
			}
			else
			{
				ret = 0;
			}
			break;
    }
	return	ret;
}

void HW_LIGHT_Model_Data_Res(HW_BUF *pRx)
{
	uint8_t	item;
	HW_BUF	*pTx;
	pTx = &TxBuf;

    pTx->count              = 0;											
    pTx->buf[pTx->count++]	= 0x3F;								//HEADER
    pTx->buf[pTx->count++]	= 0x3D;						        //DEVICE ID
    for(item = 0; item < Light_Cnt(); item++)
    {
        pTx->buf[pTx->count++]	= HW_LIGHT_Data_Conversion(HW_LIGHT_ITEM_Sequence[item]);		//DATA1 ~ DATAn
    }
    pTx->buf[pTx->count++]	= 0x21;
    pTx->buf[pTx->count++]	= 0x0D;
    TxBuf.send_flag	= 1;
}
//------------------------------------------------------------------------------------------------------------------------------------------
#if	defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
void HW_Data_Process(HW_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item = 0;
	
	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

    switch(pRx->buf[0] & 0xF0)	// CMD
    {
        case 0x50:	//��ü ���� �䱸
            HW_LIGHT_Model_Data_Res(pRx);
            break;
        case 0x30:	//���� ���� �䱸
            if((pRx->buf[0] & 0x0F) <= Light_Cnt() && (pRx->buf[0] & 0x0F) > 0)   //31 ~ 36
            {
                HW_Light_Control(HW_LIGHT_ITEM_Sequence[(pRx->buf[0] & 0x0F) - 1], pRx->buf[2]);
            }
            break;
        case 0xF0:  //��ü ���� �䱸
            if(pRx->buf[0] == 0xFF)
            {
                for(i = 0; i < Light_Cnt(); i++)
                {
                    HW_Light_Control(HW_LIGHT_ITEM_Sequence[i], pRx->buf[2]);
                }
            }
            break;
    }
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
#endif	// _HW_PROTOCOL_

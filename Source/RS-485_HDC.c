

#include "header.h"
#include "rs-485.h"
#include "el_switch.h"
#include "Debug.h"
#include "Timer.h"
#include "led.h"
#include "WDGnBeep.h"
#include "LCD.h"
#include "STPM3x_opt.h"
// #define _HDC_PROTOCOL_
#ifdef _HDC_PROTOCOL_
HDC_BUF	RxBuf, TxBuf;
_BLOCK_FLAG_ Batch_State_Flag;
// uint8_t			Gu8_RS485_TX_Enable	= 0;

uint8_t		Gu8_HDC_Error_Code = 0;
uint8_t		Gu8_Date_Receive_Flag = 0;

#define	MAX_HDC_DATA_SEQUENCE		8 
uint8_t	HDC_LIGHT_ITEM_Sequence[MAX_HDC_DATA_SEQUENCE];
uint8_t	HDC_ELEC_ITEM_Sequence[MAX_HDC_DATA_SEQUENCE];
uint8_t HDC_DIMMING_LIGHT_ITEM_Sequence[MAX_HDC_DATA_SEQUENCE];
uint8_t Store_Light_State[MAX_HDC_DATA_SEQUENCE];

// uint8_t Gu8_Batch_Light_Flag;
uint8_t	Gu8_Call_From_Flag = 0;

uint8_t Gu8_Elec_Overload_OFF[2] = {0, };
uint8_t Gu8_Elec_Auto_OFF[2] = {0, };

void HDC_Data_Process(HDC_BUF	*pRx);
void HDC_Process(uint8_t data);
//-------------------------------------------------------------------------------------------------------------------------
void SET_HDC_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_HDC_DATA_SEQUENCE)
	{
		HDC_LIGHT_ITEM_Sequence[count]	= item;
	}
}
void SET_HDC_ELEC_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_HDC_DATA_SEQUENCE)
	{
		HDC_ELEC_ITEM_Sequence[count]	= item;
	}
}
void SET_HDC_DIMMING_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)		//��� ��� ���� ���� �� ����� �κ��� �־ �߰���.
{
	if(count < MAX_HDC_DATA_SEQUENCE)
	{
		HDC_DIMMING_LIGHT_ITEM_Sequence[count]	= item;
	}
}
void Protocol_Data_Init(void)
{
    uint8_t	count, i;

	memset((void*)&RxBuf,		0,	sizeof(HDC_BUF));
	memset((void*)&TxBuf,		0,	sizeof(HDC_BUF));
	
	Gu16_Elevator_Tmr			= 0;
	Gu16_GAS_Off_Tmr			= 0;
	// pG_State->ETC.BatchState		= HDC_BATCHLIGHT_UNKNOWN;
	memset(HDC_LIGHT_ITEM_Sequence, 0, MAX_HDC_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	memset(HDC_ELEC_ITEM_Sequence, 0, MAX_HDC_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	memset(HDC_DIMMING_LIGHT_ITEM_Sequence, 0, MAX_HDC_DATA_SEQUENCE);
	// �������� ������ �׸� ����
	// �����	 �ִ��׸�	: ���� 6��, ���� 4�� + ��� 2��
	// ����+��� �ִ��׸�	: ���� 4�� + ��� 2��, ����2�� + ���2�� + ���2��
	// ex) ���� 3��, ��� 2�� = ����1,����2,����3,���1,���2,0,0,0
	// ex) ���� 1��, ��� 1�� = ����1,���1,0,0,0,0,0,0
	
	count = 0;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_HDC_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);

#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
	count	= 0;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_HDC_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_HDC_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
	count	= 0;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_HDC_DIMMING_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_HDC_DIMMING_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);		
#endif
	for(i = 0; i < MAX_HDC_DATA_SEQUENCE; i++)
	{
		// Store_Light_State[i]	= HDC_Batch_Light_State(HDC_LIGHT_ITEM_Sequence[i]);
		Store_Light_State[i]	= HDC_ON_FLAG;	//�ʱ�ȭ ���� ���� ���� ON ���·� ����. 
		//�ϰ� �ҵ��� ���·� ������ ������ ��� �ϰ� ����ġ���� �ϰ� �ҵ� ������ ���� �� ��ü ������ ������ �ϱ� ����.
		//�� ó�� ���� ������ �ϰ� �ҵ��� ���·� ������ ������ ��� �ϰ� �ҵ� ������ �ϴ��� ������ ��� ���� ���°� ��.
	}
}

void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr)
{
    if(tmr == 0)		// ������ ������ ���� �� X ms �ʰ��ϸ� ������ Ŭ����
    {
        RxBuf.buf[0]    = 0;
        RxBuf.count     = 0;
    }
}

void RS485_Tx_Process(void)
{
    static uint8_t	_mode_	= 0;
    uint8_t i;

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
					printf("TX(HDC) : ");
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

void BATCH_BLOCK_STATE_Process(void)
{
	if(item2tsn(mapping_ITEM_GAS))
	{
		if(Gu16_GAS_Off_Tmr == 0)
		{
			if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)		// �������ݿ� ���� ������ ������ OPEN ���·� ��ȯ
			{
				BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
			}
		}
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(Gu16_Elevator_Tmr == 0)
		{
			if((GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING) || (GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR))))
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);
			}
		}
	}
}

uint8_t Get_485_ID(void)
{
	return	(uint8_t)pG_Config->RS485_ID;		// ��巹��
}

uint8_t BIN2BCD(uint8_t input)
{
	int shift, bcd = 0;
	// printf("input = 0x%x, ", input);
	for(shift = 0; input > 0; shift++, input /= 10)
	{
		bcd |= (uint8_t)(input % 10 << (shift << 2));
	}
	// printf("bcd = 0x%x\r\n", bcd);
	return (uint8_t)bcd;
}

uint8_t BCD2BIN(uint8_t input)
{
	uint8_t ret, binary = 0;
	int i;

	printf("IN : 0x%x\r\n", (uint16_t)input);

	for(i = 1; input != 0x00; input = (uint8_t)(input >> 4), i *= 10)
	{
		binary += (uint8_t)(i * (input & 0x0F));
		// printf("binary = 0x%x\r\n", (uint16_t)binary);
	}
	
	printf("binary = 0x%x\r\n", (uint16_t)binary);
	
	ret = binary;
	
	return ret;
}

uint8_t Data_Devide(uint16_t data, uint8_t Sel)
{
	uint8_t Data_MSB, Data_LSB, ret;

	Data_MSB = (uint8_t)((data & 0xFF00) >> 8);
	Data_LSB = (uint8_t)(data & 0x00FF);
	
	if(Sel == 0) 		ret = Data_MSB;
	else if(Sel == 1)	ret = Data_LSB;
	else ret = 0;

	return ret;
}

uint8_t HDC_Crc(HDC_BUF *pTRx, uint16_t len)
{
    uint8_t i, crc = 0;

    crc = HDC_STX;
	// printf("\r\n");
    for(i = HDC_F_ID; i < (len - 1); i++)
    {
        crc ^= pTRx->buf[i];
        crc++;
        // printf("%d %X ", (uint16_t)i, (uint16_t)pTRx->buf[i]);
    }
    // printf("crc = 0x%X\r\n", crc);
    return crc;
}

uint8_t NIS_Crc(HDC_BUF *pTRx, uint8_t cal, uint8_t sel)		//����� 485��� �׽�Ʈ�� ���ؼ� �߰���.
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
	HDC_BUF	*pTx;
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
	// TxBuf.count	= 9;
    TxBuf.send_flag	= 1;    
}

void Protocol_Process(uint8_t data)
{
	HDC_BUF	*pRx;
	uint8_t		crc, crc_xor = 0;
    uint8_t     cnt = 0;
    int i;

	pRx = &RxBuf;

	if(G_Debug == DEBUG_HOST_REALDATA)
	{
		if(pG_Config->Protocol_Type == HDC_PROTOCOL)
		{
			if((data == HDC_STX) && (pRx->count == 0))	printf("\n");
		}
		printf("%02X ", (uint16_t)data);
	}
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
    switch(pRx->count)
    {
        default:
            if((pRx->buf[HDC_F_STX] != HDC_STX) && (data == HDC_STX))
            {
                pRx->count = 0;
            }
            break;
        case 1:
            if((pRx->buf[HDC_F_STX] != HDC_STX) && (pRx->buf[HDC_F_STX] != NIS_LIGHT_ID_COMM_1))
            {
                pRx->count = 0;
            }
            break;
        case 2:
            if(pRx->buf[HDC_F_ID] != BATCH_BLOCK_ID && pRx->buf[HDC_F_ID] != BATCH_BLOCK_n_GAS_ID && pRx->buf[HDC_F_ID] != BATCH_BLOCK_n_GAS_EV_ID)
            {
                if(pRx->buf[HDC_F_ID] != NIS_LIGHT_ID_COMM_2)
                {
                    pRx->count = 0;
                }
            }
            break;
        case 3:
            if(pRx->buf[HDC_F_CMD] > 0x7F)  //0x00 <= CMD <= 0x7F �ش� ������ ���е� -> ������� �����ϴ� ��Ŷ
            {
				if(pRx->buf[HDC_F_STX] != NIS_LIGHT_ID_COMM_1)
				{
                	pRx->count = 0;
				}                
            }
            break;
		case 4:
			break;
		case 5:
			if(pRx->buf[HDC_F_D1] != 0)
			{
				if(pRx->buf[HDC_F_STX] != NIS_LIGHT_ID_COMM_1)
				{
				pRx->count = 0;
				}
			}
			break;
		case 6:
			if(pRx->buf[HDC_F_D2] != 0)
			{
				if(pRx->buf[HDC_F_STX] != NIS_LIGHT_ID_COMM_1)
				{
					pRx->count = 0;
				}
			}
			break;
		case 7:
			if(pRx->buf[HDC_F_D3] != 0)
			{
				if(pRx->buf[HDC_F_STX] != NIS_LIGHT_ID_COMM_1)
				{
					pRx->count = 0;
				}
			}
			break;
    }
#else
    switch(pRx->count)
        {
            default:
                if((pRx->buf[HDC_F_STX] != HDC_STX) && (data == HDC_STX))
                {
                    pRx->count = 0;
                }
                break;
            case 1:
                if((pRx->buf[HDC_F_STX] != HDC_STX) && (pRx->buf[HDC_F_STX] != NIS_LIGHT_ID_COMM_1))
                {
                    pRx->count = 0;
                }
                break;
            case 2:
                // if((pRx->buf[HDC_F_ID] & 0xF0) != MULTI_SWITCH_ID)		//��Ƽ ����ġ ID ������ �ƴϸ�
				if(pRx->buf[HDC_F_ID] != (MULTI_SWITCH_ID | Get_485_ID()))	//��Ƽ ����ġ ID�� 485 ID�� �ٸ���
                {
                    if(pRx->buf[HDC_F_ID] != NIS_LIGHT_ID_COMM_2)
                    {
                        pRx->count = 0;
						// printf("ID not Equal");
                    }
					else//id : 0xBB, stx : 0x02, count = 0
					{
						if(pRx->buf[HDC_F_STX] == HDC_STX)	pRx->count = 0;	//0x02 0xBB�� ������ ������ ��
					}
                }
                break;
            case 3:
                if(pRx->buf[HDC_F_LEN] > 127) //STX, ID, LEN, CMD, SEQ, CS �ּ� 6����Ʈ + DATA�� �ִ� 121����Ʈ = 127����Ʈ
                {
                    if(pRx->buf[HDC_F_STX] != NIS_LIGHT_ID_COMM_1)
                    {
                        pRx->count = 0;
                    }  
                }
                break;
            case 4:
                if(pRx->buf[HDC_F_CMD] > 0x7F)  //0x00 <= CMD <= 0x7F �ش� ������ ���е� -> ������� �����ϴ� ��Ŷ
                {
                    if(pRx->buf[HDC_F_STX] != NIS_LIGHT_ID_COMM_1)
                    {
                        pRx->count = 0;
                    }                
                }
                break;
        }
#endif
    pRx->buf[pRx->count++] = data;

    if((pRx->buf[HDC_F_STX] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[HDC_F_ID] == NIS_LIGHT_ID_COMM_2))
    {
		if(pRx->count >= 8)
		{
			crc = NIS_Crc(pRx, 0, NIS_RX);
			crc_xor = NIS_Crc(pRx, 1, NIS_RX);
			if(crc == pRx->buf[7] && crc_xor == pRx->buf[6])
			{
                if(G_Debug == DEBUG_HOST)
                {
                    printf("\nRX(HDC) : ");
                    for(i=0;i<pRx->count;i++)
                    {
                        printf("%02X ", (uint16_t)pRx->buf[i]);
                    }
                    printf("\n");
                }
				HDC_Data_Process(pRx);				
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
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
    else
    {
        if(pRx->count >= HDC_MAX_BUF)
        {
            pRx->buf[0] = 0;        //20210527
            pRx->count = 0;
        }
		if(pRx->count >= HDC_F_MAX_BUF)
		{
            crc = HDC_Crc(pRx, pRx->count);
            
            if(crc == pRx->buf[pRx->count-1])
            {
                if(G_Debug == DEBUG_HOST)
                {
                    printf("\nRX(HDC) : ");
                    for(i=0;i<pRx->count;i++)
                    {
                        printf("%02X ", (uint16_t)pRx->buf[i]);
                    }
                    printf("\n");
                }
                HDC_Data_Process(pRx);
            }
            else
            {
                printf("\r\ncal crc[0x%02X] != buf crc[0x%02X]\r\n", (uint16_t)crc, (uint16_t)pRx->buf[pRx->count-1]);
            }
            pRx->buf[0] = 0;
            pRx->count = 0;		
		}
    }
#else
    else
    {
        if(pRx->count >= HDC_MAX_BUF)
        {
            pRx->buf[0] = 0;        //20210527
            pRx->count = 0;
        }
		if(pRx->count >= HDC_F_D2) //ī��Ʈ�� D2��ŭ �Ǹ� ���� LEN ���� cnt�� ����
		{
			cnt = pRx->buf[HDC_F_LEN];
		}

        if(pRx->count == cnt)       //cnt�� LED(������ ����) ��, pRx->count�� ������ ���̿� ���� ��������
        {
            crc = HDC_Crc(pRx, pRx->count);
            
            if(crc == pRx->buf[pRx->count-1])
            {
                if(G_Debug == DEBUG_HOST)
                {
                    printf("\nRX(HDC) : ");
                    for(i=0;i<pRx->count;i++)
                    {
                        printf("%02X ", (uint16_t)pRx->buf[i]);
                    }
                    printf("\n");
                }
                HDC_Data_Process(pRx);
            }
            else
            {
                printf("\r\ncnt = %d, len = %d, cal crc[0x%02X] != buf crc[0x%02X]\r\n", (uint16_t)cnt, (uint16_t)pRx->buf[HDC_F_LEN], (uint16_t)crc, (uint16_t)pRx->buf[pRx->count-1]);
            }
            pRx->buf[0] = 0;
            pRx->count = 0;
        }
    }
#endif
}


int pre_macro_date_to_int(const char *str_pre_macro_date, int* year, int* month, int* day)
{
	const int comfile_date_len = 12;
	const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

	char s_month[5] = {0,};
	int iyear = 0, iday = 0;
	int imonth = 0;
	// error check
	if (NULL == str_pre_macro_date || comfile_date_len - 1 != strlen(str_pre_macro_date))
		return 0;

	sscanf(str_pre_macro_date, "%s %d %d", s_month, &iday, &iyear);
	imonth = (strstr(month_names, s_month) - month_names) / 3 + 1;
	*year = iyear-2000;
	*month = imonth;
	*day = iday;
	return 1;
	// 2010 12 01
	// return 10000 * iyear + 100 * imonth + iday;
}

uint8_t Get_Date(uint8_t date)
{
	uint8_t ret = 0;
	int year, month, day;

	pre_macro_date_to_int(__DATE__, &year, &month, &day);

	if(date == 0)		ret = (uint8_t)year;
	else if(date == 1)	ret = (uint8_t)month;
	else if(date == 2)	ret = (uint8_t)day;
    else ret = 1;
	
	return ret;

}
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
uint8_t Get_Light_Count(void)		//���� �� ī��Ʈ
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

uint8_t Get_Light_Count_BIT(void)
{
	uint8_t count = 0, bit = 0;
	if(item2tsn(mapping_ITEM_LIGHT_1))			bit |= (uint8_t)(1 << count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			bit |= (uint8_t)(1 << count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			bit |= (uint8_t)(1 << count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			bit |= (uint8_t)(1 << count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			bit |= (uint8_t)(1 << count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			bit |= (uint8_t)(1 << count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			bit |= (uint8_t)(1 << count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			bit |= (uint8_t)(1 << count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	bit |= (uint8_t)(1 << count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	bit |= (uint8_t)(1 << count++);
	return bit;
}

uint8_t HDC_Batch_Light_State(uint8_t item)		//�ϰ� �ҵ� �� ���� ���� ���� ����.
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
				ret = HDC_ON_FLAG;
			}
			else
			{
				ret = HDC_OFF_FLAG;
			}
			break;
	}
	return ret;
}

uint8_t Get_Elec_Count(void)
{
	uint8_t count = 0;
	
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	count++;
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	count++;
	
	return count;
}

uint8_t Base_D3(void)
{
    uint8_t ret = 0;

    ret |= (0 << 7);    //BIT7, �ð�����
    if(pG_Config->Enable_Flag.PWM_Dimming || pG_Config->Enable_Flag.PWM_Color_Temp)  ret |= (1 << 6);   //BIT6, V2CMD ����
    else                                                                             ret |= (0 << 6);
    ret |= 0x0E;    //BIT0~5, ������ ��ȣ ����

    return ret;
}

uint8_t Get_Light_State(void)
{
    uint8_t item, ret = 0;
	uint8_t i = 0;

    for(i = 0; i < Get_Light_Count(); i++)
    {
        item = HDC_LIGHT_ITEM_Sequence[i];
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
                if(GET_Switch_State(item2tsn(item)))	ret |= (uint8_t)(HDC_ON_FLAG << i);
                else									ret |= (uint8_t)(HDC_OFF_FLAG << i);
                break;
			default:
				ret |= 0;
				break;
        }
    }
	return ret;
}

uint8_t Get_Elec_State(uint8_t item)
{
    uint8_t ret = 0;
	uint8_t num = 0;

	if(Get_Elec_Count() != 0)
	{
		switch(item)
		{
			case mapping_ITEM_ELECTRICITY_1:
			case mapping_ITEM_ELECTRICITY_2:
				if(GET_Switch_State(item2tsn(item)))	ret = 0x01;	//ON
				else
				{
					if(item == mapping_ITEM_ELECTRICITY_1)	num = 0;
					else									num = 1;

					if(Gu8_Elec_Auto_OFF[num])				ret = 0x03;	//������� �ڵ����� ����
					else if(Gu8_Elec_Overload_OFF[num])		ret = 0x04;	//������� �����Ϸ� ����
					else									ret = 0x02;	//�Ϲ� OFF
				}
				if(item == mapping_ITEM_ELECTRICITY_1)
				{
					if(pG_State->ETC.Auto1)	ret |= (1 << 4);	//BIT4~5, 01 �ڵ�
					else					ret |= (1 << 5);	//BIT4~5, 10 ���
				}
				else if(item == mapping_ITEM_ELECTRICITY_2)
				{
					if(pG_State->ETC.Auto2)	ret |= (1 << 4);	//BIT4~5, 01 �ڵ�
					else					ret |= (1 << 5);	//BIT4~5, 10 ���
				}
				break;
			default:
				ret |= 0;
				break;
		}
	}


	return ret;
}

uint8_t Get_Elec_W(uint8_t item, uint8_t Sel)
{
	uint8_t Watt = 0;

	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
			Watt = Data_Devide((uint16_t)(Gu16_Elec_1_Watt*10), Sel);	//0.1W �������� ǥ���ϹǷ� �Һ�����(W) * 10 �� 16 ���� ������ ������
			break;
		case mapping_ITEM_ELECTRICITY_2:
			Watt = Data_Devide((uint16_t)(Gu16_Elec_2_Watt*10), Sel);
			break;
	}
	return Watt;
}

uint8_t Get_Elec_Limit(uint8_t item, uint8_t Sel)
{
	uint8_t Limit = 0;

	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
			Limit = Data_Devide((uint16_t)(Gu16_ElecLimitCurrent_1*10), Sel);
			break;
		case mapping_ITEM_ELECTRICITY_2:
			Limit = Data_Devide((uint16_t)(Gu16_ElecLimitCurrent_2*10), Sel);
			break;
	
	}
	return Limit;
}

uint8_t Batch_Light_State(void)
{
	uint8_t ret = 0;

	if(pG_State->ETC.BatchState == HDC_BATCHLIGHT_BLOCK)						ret = HDC_BATCHLIGHT_BLOCK;		//�ϰ� �ҵ� ����
	else if(pG_State->ETC.BatchState == HDC_BATCHLIGHT_RELEASE)					ret = HDC_BATCHLIGHT_RELEASE;	//�ϰ� �ҵ� ���� ����
	else																		ret = HDC_BATCHLIGHT_UNKNOWN;	//�ϰ� ���� ��
	return ret;
}

void HDC_Base_ACK(HDC_BUF *pRx)
{
	HDC_BUF	*pTx;
	pTx = &TxBuf;
    pTx->count = HDC_F_STX;

    pTx->buf[pTx->count++]  =   HDC_STX;                                    //STX
    pTx->buf[pTx->count++]  =   pRx->buf[HDC_F_ID];                         //ID
    pTx->buf[pTx->count++]  =   15;                                         //LEN
    pTx->buf[pTx->count++]  =   (uint8_t)(pRx->buf[HDC_F_CMD] + 0x80);      //CMD
    pTx->buf[pTx->count++]  =   pRx->buf[HDC_F_SEQ];                        //SEQ
	//----------------------------------------------------DATA ����-------------------------------------------------
    pTx->buf[pTx->count++]  =  	Get_Light_Count();				            //D1, ��ġ�� ���� ����
    pTx->buf[pTx->count++]  =   Get_Elec_Count();					        //D2, ��ġ�� ������� ���� �ܼ�Ʈ ����
    pTx->buf[pTx->count++]  =   Base_D3();									//D3, ������ ���� �� �ð� ���� ���� ����

    pTx->buf[pTx->count++]  =   Get_Date(HDC_DATE_YEAR);					//D4, ��
	pTx->buf[pTx->count++]  =   Get_Date(HDC_DATE_MONTH);					//D5, ��
	pTx->buf[pTx->count++]  =   Get_Date(HDC_DATE_DAY);						//D6, ��
	pTx->buf[pTx->count++]  =   0;											//D7, �Ϸù�ȣ

	pTx->buf[pTx->count++]  =   PROTOCOL_VERSION_MSB;						//D8, �������� ���� ����
	pTx->buf[pTx->count++]  =   PROTOCOL_VERSION_LSB;						//D9, �������� ���� ����

	pTx->buf[pTx->count++]  =   HDC_Crc(pTx, pTx->buf[HDC_F_LEN]);			//CRC
	TxBuf.send_flag	= 1; 
}

void HDC_State_ACK(HDC_BUF *pRx)
{
	uint8_t i = 0;
	HDC_BUF *pTx;
	pTx = &TxBuf;
	pTx->count = HDC_F_STX;

    pTx->buf[pTx->count++]  =   HDC_STX;								//STX
    pTx->buf[pTx->count++]  =   pRx->buf[HDC_F_ID];						//ID
    pTx->buf[pTx->count++]  =   0x00;									//LEN, �� �κп� �� ������.
    pTx->buf[pTx->count++]  =   (uint8_t)(pRx->buf[HDC_F_CMD] + 0x80);	//CMD
    pTx->buf[pTx->count++]  =   pRx->buf[HDC_F_SEQ];					//SEQ
	//-----------------------------------------------------���� ����--------------------------------------------
    pTx->buf[pTx->count++]  =   Get_Light_Count();									//D1, ��ġ�� ���� ����
	pTx->buf[pTx->count++]  =   Get_Light_State();									//D2, ���� ON/OFF ����
	pTx->buf[pTx->count++]  =   Batch_Light_State();								//D3, ���� �ϰ��ҵ� ����
	//-----------------------------------------------------�ܼ�Ʈ ����--------------------------------------------
	pTx->buf[pTx->count++]  =   Get_Elec_Count();									//D4, �ܼ�Ʈ ����
	if(Get_Elec_Count())
	{
		for(i = 0; i < 2; i++)
		{
			pTx->buf[pTx->count++]  =   Get_Elec_State(HDC_ELEC_ITEM_Sequence[i]);			//D5, D10, �ܼ�Ʈ1�� ����
			pTx->buf[pTx->count++]  =   Get_Elec_W(HDC_ELEC_ITEM_Sequence[i], HDC_MSB);		//D6, D11, �ܼ�Ʈ n�� �Һ�����(����)
			pTx->buf[pTx->count++]  =   Get_Elec_W(HDC_ELEC_ITEM_Sequence[i], HDC_LSB);		//D7, D12, �ܼ�Ʈ n�� �Һ�����(����)
			pTx->buf[pTx->count++]  =   Get_Elec_Limit(HDC_ELEC_ITEM_Sequence[i], HDC_MSB);	//D8, D13, �ܼ�Ʈ n�� ���� ���� ��(����)
			pTx->buf[pTx->count++]  =   Get_Elec_Limit(HDC_ELEC_ITEM_Sequence[i], HDC_LSB);	//D9, D14, �ܼ�Ʈ n�� ���� ���� ��(����)
		}
		pTx->buf[HDC_F_LEN] = 0x14;	//10 + (D4 * 5) = 10 + (2 * 5) = 20(������� ����ġ �ܼ�Ʈ �� �����̱� ����)
	}
	else
	{
		pTx->buf[HDC_F_LEN] = 10;	//�ܼ�Ʈ ���� ���� ��� ������ ���� 10(0x0A)
	}
	pTx->buf[pTx->count++]  =   HDC_Crc(pTx, pTx->buf[HDC_F_LEN]);					//CRC
	TxBuf.send_flag	= 1; 
}


void HDC_Light_Control(HDC_BUF *pRx)
{
	uint8_t i = 0;
	uint8_t item = 0xFF;
	uint8_t Light_no = pRx->buf[HDC_F_D2];
	uint8_t Flag = (uint8_t)(pRx->buf[HDC_F_D1] & 0x01);

	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec

	for(i = 0; i < MAX_HDC_DATA_SEQUENCE; i++)
	{
		if(Light_no & (1 << i))	item = HDC_LIGHT_ITEM_Sequence[i];
		else 					item = 0xFF;

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
				if(Flag == HDC_OFF_FLAG)
				{
					if(GET_Switch_State(item2tsn(item)))
					{
						EventCtrl(item2tsn(item), Flag);
					}
				}
				else if(Flag == HDC_ON_FLAG)
				{
					if(GET_Switch_State(item2tsn(item)) == OFF)
					{
						EventCtrl(item2tsn(item), Flag);
					}
				}
				break;
			default:
				break;
		}
	}
}

void HDC_BatchLight_Control(uint8_t item, uint8_t control_value)
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
			if(control_value == HDC_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))
				{
					EventCtrl(item2tsn(item), OFF);						//���� OFF
				}
			}
			else if(control_value == HDC_ON_FLAG)
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

void HDC_Batch_Light(HDC_BUF	*pRx)
{
	uint8_t i = 0;

	if(pRx->buf[HDC_F_D3] == HDC_BATCHLIGHT_BLOCK)
	{
		if(pG_State->ETC.BatchState != HDC_BATCHLIGHT_BLOCK)	//�ϰ� �ҵ� ���� �ƴ� ?
		{
			for(i = 0; i < MAX_HDC_DATA_SEQUENCE; i++)
			{
				Store_Light_State[i] = HDC_Batch_Light_State(HDC_LIGHT_ITEM_Sequence[i]);
				HDC_BatchLight_Control(HDC_LIGHT_ITEM_Sequence[i], HDC_OFF_FLAG);	//0xFF�� ��� ����
			}
			pG_State->ETC.BatchState = HDC_BATCHLIGHT_BLOCK;	//�ϰ��ҵ� �÷���
		}
	}
	else if(pRx->buf[HDC_F_D3] == HDC_BATCHLIGHT_RELEASE)
	{
		if(pG_State->ETC.BatchState != HDC_BATCHLIGHT_RELEASE)	//�ϰ� �ҵ� ���� ���°� �ƴ� ��
		{
			for(i = 0; i < MAX_HDC_DATA_SEQUENCE; i++)
			{
				if(Store_Light_State[i] == HDC_ON_FLAG)
				{
					HDC_BatchLight_Control(HDC_LIGHT_ITEM_Sequence[i], HDC_ON_FLAG);	//0xFF�� ��� ����
				}
			}
			pG_State->ETC.BatchState = HDC_BATCHLIGHT_RELEASE;	//�ϰ��ҵ� ���� �÷���
		}
	}
}

void SET_Elec_Auto_Manual(uint8_t item, uint8_t flag)
{
	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
			if(flag == HDC_ELEC_AUTO && pG_State->ETC.Auto1 == 0)
			{
				pG_State->ETC.Auto1	= 1;
				Beep(ON);
			}
			else if(flag == HDC_ELEC_NORMAL && pG_State->ETC.Auto1)
			{
				pG_State->ETC.Auto1	= 0;
				Beep(ON);
			}
			Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
			break;
		case mapping_ITEM_ELECTRICITY_2:
			if(flag == HDC_ELEC_AUTO && pG_State->ETC.Auto2 == 0)
			{
				pG_State->ETC.Auto2	= 1;
				Beep(ON);
			}
			else if(flag == HDC_ELEC_NORMAL && pG_State->ETC.Auto2)
			{
				pG_State->ETC.Auto2	= 0;
				Beep(ON);
			}
			Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
			break;
	}
}

void HDC_Elec_Control(uint8_t item, uint8_t control)
{
	uint8_t touch_switch = 0;
	uint8_t Flag = (uint8_t)(control & HDC_ELEC_MODE_BIT);
	printf("Elec_Control %x\r\n", (uint16_t)control);
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec

	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			touch_switch	= item2tsn(item);
			if((control & HDC_ELEC_ON_OFF_BIT) == HDC_ELEC_ON)
			{
				if(GET_Switch_State(touch_switch) == OFF)
				{
					SET_Switch_State(touch_switch, ON);
					SET_LED_State(touch_switch, ON);
					Beep(ON);
					PUT_RelayCtrl(item2ctrl(item), ON);	// �׸���� ����
					SET_SWITCH_Delay_OFF_Flag(item, 0);
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
					ALL_Electricity_Switch_LED_Ctrl();
				}
			}
			else if((control & HDC_ELEC_ON_OFF_BIT) == HDC_ELEC_OFF)
			{
				if(GET_Switch_State(touch_switch))
				{
					SET_Switch_State(touch_switch, OFF);
					SET_LED_State(touch_switch, OFF);
					Beep(OFF);
					PUT_RelayCtrl(item2ctrl(item), OFF);	// �׸���� ����
					SET_SWITCH_Delay_OFF_Flag(item, 0);
					SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
					ALL_Electricity_Switch_LED_Ctrl();
				}
			}
			//----------------------------------------------------------------------------------------------------------
			if(control & HDC_ELEC_MODE_BIT)
			{
				SET_Elec_Auto_Manual(item, Flag);
			}
			if(control & HDC_ELEC_BLOCK_VALUE)
			{
				if(item == mapping_ITEM_ELECTRICITY_1)
				{
					if(((double)Gu16_LCD_Watt_1 * 0.8) < 0.0)	Gu16_ElecLimitCurrent_1	= 0;
					else										Gu16_ElecLimitCurrent_1	= (uint16_t)((double)Gu16_LCD_Watt_1 * 0.8);	// ���� ���� 80%�� ����
					// Gu16_ElecLimitCurrent_1 = (uint16_t)Gu16_Elec_1_Watt;
				}
				else if(item == mapping_ITEM_ELECTRICITY_2)
				{
					if(((double)Gu16_LCD_Watt_2 * 0.8) < 0.0)	Gu16_ElecLimitCurrent_2	= 0;
					else										Gu16_ElecLimitCurrent_2	= (uint16_t)((double)Gu16_LCD_Watt_2 * 0.8);	// ���� ���� 80%�� ����
					// Gu16_ElecLimitCurrent_2 = (uint16_t)Gu16_Elec_2_Watt;
				}
				Store_ElecLimitCurrent();	//���� ��뷮�� ���� ���ذ����� ����
				printf("Store ElecLimit Current\r\n");
			}
			break;
	}
}

void HDC_Elec(HDC_BUF *pRx)
{
	uint8_t i = 0;
	uint8_t count = HDC_F_D5;
	uint8_t Elec_Count = pRx->buf[HDC_F_D4];
	uint8_t Elec_Num[2] = {0, };
	uint8_t Elec_Control[2] = {0, };

	for(i = 0; i < Elec_Count; i++)
	{
		Elec_Num[i] = pRx->buf[count++];		//D5, �ܼ�Ʈ ��ȣ
		Elec_Control[i] = pRx->buf[count++];	//D6, ����
		HDC_Elec_Control(HDC_ELEC_ITEM_Sequence[Elec_Num[i]-1], Elec_Control[i]);
	}
}

uint8_t HDC_Error_Data_Check(HDC_BUF *pRx)
{
	uint8_t HDC_CMD;
	uint8_t i = 0, k = 0, cnt = 0;
	HDC_CMD			= pRx->buf[HDC_F_CMD];

	if(pRx->buf[HDC_F_ID] != (MULTI_SWITCH_ID | Get_485_ID()))				return 1;
	if(HDC_CMD == HDC_CMD_BASE_REQ)		if(pRx->buf[HDC_F_LEN] != 6)		return 1;
	if(HDC_CMD == HDC_CMD_STATE_REQ)	if(pRx->buf[HDC_F_LEN] != 6)		return 1;
	if(HDC_CMD == HDC_CMD_CONTROL_REQ)
	{
		if(pRx->buf[HDC_F_D1] > 1)											return 1;	//���� ON/OFF �����̸� BIT0�� ����ϹǷ� 1���� ũ�� ����
		if(pRx->buf[HDC_F_D2])
		{
			// printf("D2\r\n");
			if(pRx->buf[HDC_F_D2] > Get_Light_Count_BIT() && pRx->buf[HDC_F_D2] != 0xFF)	return 1;

			for(i = 0; i < 8; i++)
			{
				if(pRx->buf[HDC_F_D2] >> i & 0x01) cnt++;
			}
			// printf("cnt %d, get light cnt %d\r\n", (uint16_t)cnt, (uint16_t)Get_Light_Count());
		}
		if(pRx->buf[HDC_F_D3] > 2)											return 1;	//�ϰ� �ҵ� ����. ����(00, 01, 10) ���̸� ����
		if(pRx->buf[HDC_F_D4])
		{
			// printf("D4\r\n");
			// printf("control elec cnt %d, Get elec cnt %d\r\n", (uint16_t)pRx->buf[HDC_F_D4], (uint16_t)Get_Elec_Count());
			if(((pRx->buf[HDC_F_D4] * 2) + 4) != (pRx->buf[HDC_F_LEN] - 6))	return 1;	//(1 * 2) + 4 != 0x0C - 6
			// if(pRx->buf[HDC_F_D4] > 0x08)									return 1;
			if(pRx->buf[HDC_F_D4] > Get_Elec_Count())						return 1;	//������ �ܼ�Ʈ ���� �ش� ����ġ�� �ܼ�Ʈ ������ ũ�� ����
			
			for(i = 0; i < pRx->buf[HDC_F_D4]; i++)
			{
				// printf("control elec num %d, Get elec cnt %d\r\n", (uint16_t)pRx->buf[HDC_F_D5+k], (uint16_t)Get_Elec_Count());
				if(pRx->buf[HDC_F_D5+k] > Get_Elec_Count())					return 1;	//������ �ܼ�Ʈ ��ȣ�� �ش� ����ġ�� �ܼ�Ʈ ������ ������ ����
				if(pRx->buf[HDC_F_D5+k] == 0)								return 1;	//D4(�ܼ�Ʈ ����)�� ������ �� D5 or D7(�ܼ�Ʈ ��ȣ)�� 0�̸� ����
				if(pRx->buf[HDC_F_D5+1+k] & 0x8C)							return 1;	//D6 or D8�� �̻�� ��Ʈ ��� �� ����. *���� �ʿ�*
				k++;
				k++;
			}
			// if(pRx->buf[HDC_F_D5] > 0x08)									return 1;	//D4(�ܼ�Ʈ ����)�� ������ �� D5(�ܼ�Ʈ ��ȣ)�� 0x08 �̻��̸� ����. �ܼ�Ʈ ���� �� ��ȣ�� �ִ� 8
		}
		else
		{
			if(pRx->buf[HDC_F_LEN] > 0x0A)									return 1;	//D4(�ܼ�Ʈ ����)�� 0�ε� ������ ���̰� 10���� ũ�� ����
		}
	}
	return 0;
}

void HDC_Data_Process(HDC_BUF	*pRx)
{
    uint8_t HDC_CMD, Elec_Count;
	uint8_t Batch_Control;

    HDC_CMD			= pRx->buf[HDC_F_CMD];

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
	
	if(pRx->buf[HDC_F_STX] == HDC_STX)
	{
		if(HDC_Error_Data_Check(pRx)) return;

		switch(HDC_CMD)
		{
			case HDC_CMD_BASE_REQ:
				HDC_Base_ACK(pRx);
				break;
			case HDC_CMD_STATE_REQ:
				HDC_State_ACK(pRx);
				break;
			case HDC_CMD_CONTROL_REQ:
				Batch_Control	= pRx->buf[HDC_F_D3];
				Elec_Count		= pRx->buf[HDC_F_D4];
				if(Batch_Control)	HDC_Batch_Light(pRx);
				else				HDC_Light_Control(pRx);

				if(Elec_Count)		HDC_Elec(pRx);	//���� ��û �� ����Ʈ ����
				HDC_State_ACK(pRx);
				break;
            case HDC_CMD_TIME_INFO_REQ:
                break;
            case HDC_CMD_BASE_V2_REQ:				//���, ���µ� ��� ��� �ƴϸ� ������ ó��. ���� �׽�ƮX
            case HDC_CMD_STATE_V2_REQ:				//���, ���µ� ��� ��� �ƴϸ� ������ ó��. ���� �׽�ƮX
            case HDC_CMD_CONTROL_RECEIVCE_V2_REQ:	//���, ���µ� ��� ��� �ƴϸ� ������ ó��. ���� �׽�ƮX
            case HDC_CMD_CONTROL_V2_REQ:			//���, ���µ� ��� ��� �ƴϸ� ������ ó��. ���� �׽�ƮX
                break;
		}
	}
    else if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
    {
        RS_485_ID_RES();
    }	
}

#elif defined(_ONE_SIZE_BATCH_BLOCK_MODEL_)
uint8_t Switch_State_BIT(void)
{
	uint8_t ret = 0;

	if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF))
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))				ret |= (0 << 0);	//�ϰ��ҵ� ���� ����
		else																		ret |= (1 << 0);	//�ϰ��ҵ� ���� ����
	}
	//ON : �ϰ��ҵ� ����, OFF : �ϰ��ҵ� ����
	//�ϰ��ҵ� ���� : 1, �ϰ��ҵ� ���� : 0
	if(item2tsn(mapping_ITEM_GAS))
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)				ret |= (1 << 5);
		else																		ret |= (0 << 5);
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)			ret |= (1 << 6);	//����ġ���� ���������� ȣ�� ��û ���϶��� ����.
		else																		ret |= (0 << 6);	//���� ���� Ŀ�ǵ� �޾��� ���� ����
	}
	return ret;
}

uint8_t Switch_Control_BIT(void)
{
	uint8_t ret = 0;

	if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF))
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))	ret |= (0 << 0);	//�ϰ��ҵ� ���� ����
		else															ret |= (1 << 0);	//�ϰ��ҵ� ���� ����
	}
	//ON : �ϰ��ҵ� ����, OFF : �ϰ��ҵ� ����
	//�ϰ��ҵ� ���� : 1, �ϰ��ҵ� ���� : 0
	//CMD0x06, 0x07���� D4 ������ �� 0x20, 0x40���� ���� ��� ���䵵 �����ϰ� �ش޶� ��û ����.
	//�� �����͸� ������ ���� ���� or ���������� ȣ�� �Ǳ� ������ ����ġ�� ������ ���������� ����ġ ���¿� ���� �����ϵ��� ��.
	if(item2tsn(mapping_ITEM_GAS))
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))				ret |= (1 << 5);
		else															ret |= (0 << 5);
	}
	if(item2tsn(mapping_ITEM_ELEVATOR))
	{
		if(GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR)))			ret |= (1 << 6);	//����ġ���� ���������� ȣ�� ��û ���϶��� ����.
		else															ret |= (0 << 6);	//���� ���� Ŀ�ǵ� �޾��� ���� ����
	}
	return ret;
}

void HDC_State_ACK(HDC_BUF *pRx)
{
	uint8_t i = 0;
	HDC_BUF *pTx;
	pTx = &TxBuf;
	pTx->count = HDC_F_STX;

    pTx->buf[pTx->count++]  =   HDC_STX;								//STX
    pTx->buf[pTx->count++]  =   pRx->buf[HDC_F_ID];						//ID
    pTx->buf[pTx->count++]  =   (uint8_t)(pRx->buf[HDC_F_CMD] + 0x80);	//CMD
    pTx->buf[pTx->count++]  =   pRx->buf[HDC_F_SEQ];					//SEQ
	pTx->buf[pTx->count++] =	0x80;									//D1, 0x80 ����
	
	switch(pRx->buf[HDC_F_CMD])
	{
		case HDC_CMD_STATE_REQ:
		case HDC_CMD_STATE_RELEASE_REQ:
		case HDC_CMD_MODE_CONTROL_REQ:
			pTx->buf[pTx->count++]  =   0x00;										//D2, �� ȸ���� ���� ��
			pTx->buf[pTx->count++]  =   0x00;										//D3, ���� Scene ��ȣ
			pTx->buf[pTx->count++]  =	Switch_State_BIT();							//D4, �ϰ��ҵ� ����ġ ����
			pTx->buf[pTx->count++]  =	0x00;										//D5, ��������� ��ü�� ����
			break;
		case HDC_CMD_GAS_STATE_REQ:													//0x20���� ������ 0x20���� ����. 0x40���� ���� �� 0x40���� ����. ������ ����� ������ ���� �� ȣ�� ����� ���� �� ���� �� �� 0x20, 0x40 ������.
		case HDC_CMD_ELEVATOR_STATE_REQ:											//���е忡�� �ֱ������� ���� ��. ����ȭ �����ε�.
			pTx->buf[pTx->count++]  =   0x00;										//D2, �� ȸ���� ���� ��
			pTx->buf[pTx->count++]  =   0x00;										//D3, ���� Scene ��ȣ
			pTx->buf[pTx->count++]  =	Switch_Control_BIT();						//D4, �ϰ��ҵ� ����ġ ����
			pTx->buf[pTx->count++]  =	0x00;										//D5, ��������� ��ü�� ����
			break;
		case HDC_CMD_VERSION_INFO_REQ:
			pTx->buf[pTx->count++] =	Get_Date(HDC_DATE_YEAR);					//D2, �߿��� ���� Date(Year)
			pTx->buf[pTx->count++] =	Get_Date(HDC_DATE_MONTH);					//D3, �߿��� ���� Date(Month)
			pTx->buf[pTx->count++] =	PROTOCOL_VERSION_MSB;						//D4, Protocol ���� ����(Main)
			pTx->buf[pTx->count++] =	PROTOCOL_VERSION_LSB;						//D5, Protocol ���� ����(Sub)
			break;
	}
	pTx->buf[pTx->count++]  =   HDC_Crc(pTx, 10);						//CRC
	TxBuf.send_flag	= 1;
}

void HDC_State_Release(HDC_BUF *pRx)
{
	uint8_t touch_switch = 0;
	
	touch_switch = item2tsn(mapping_ITEM_GAS);
	if(touch_switch)
	{
		if(pRx->buf[HDC_F_D4] & HDC_GAS_CLOSE)
		{
			if(GET_LED_State(touch_switch) == LED_FLASHING)		//���� ���� ��û ���϶�
			{
				BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
			}
		}
	}
	
	touch_switch = item2tsn(mapping_ITEM_ELEVATOR);
	if(touch_switch)
	{
		if(pRx->buf[HDC_F_D4] & HDC_ELEVATOR_DOWN)					//���� ȣ�� ���� �� ���� �����͸� ������ 
		{
			if(GET_LED_State(touch_switch) == LED_FLASHING)		//���������� ȣ�� ��û ���϶�
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL);		//���������� ȣ��� ���� �����ϰ� CMD 0x80 ���� �� ���� ������ �����Ѵ�.
			}
		}
	}
}
void HDC_Mode_Control(HDC_BUF *pRx)
{
	if((pRx->buf[HDC_F_D4] & 0x01) == HDC_BATCH_LIGHT_ON)			BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
	else if((pRx->buf[HDC_F_D4] & 0x01) == HDC_BATCH_LIGHT_OFF)		BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);
	//�������� ���� �� ON�� �ϰ��ҵ��� �ǹ�. ��, BATCH_BLOCK_Control �Լ������� OFF�� �ϰ� �ҵ��� �ǹ���
	//�������� ���� �� OFF�� �ϰ��ҵ� ������ �ǹ�. ��, BATCH_BLOCK_Control �Լ������� ON�� �ϰ� �ҵ� ������ �ǹ���
}

void HDC_Gas_Control(HDC_BUF *pRx)
{
	uint8_t touch_switch = 0;

	touch_switch = item2tsn(mapping_ITEM_GAS);

	if(touch_switch)
	{
		if(pRx->buf[HDC_F_D4] & HDC_GAS_CLOSE)		//�ش� �����ʹ� ���е� Or �������ܱ⿡�� ���� ���� ��츸 ���� �ȴٰ� ��.
		{
			BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
		}
		else
		{
			BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
		}
	}
}

void HDC_Elvator_Control(HDC_BUF *pRx)
{
	uint8_t touch_switch = 0;

	touch_switch = item2tsn(mapping_ITEM_ELEVATOR);

	if(touch_switch)
	{
		if(pRx->buf[HDC_F_D4] & HDC_ELEVATOR_DOWN)							//���� ȣ��
		{
			if(GET_Switch_State(touch_switch) == 0 && Gu8_Call_From_Flag != FROM_SWITCH)		//���������� ȣ�� ���°� �ƴ� ��. ��, �ش� �����ͷ� ���е忡�� ���������͸� ȣ�� �� ���.
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL);				//ȣ��
				Gu8_Call_From_Flag = FROM_WALLPAD;
			}
			else if(GET_Switch_State(touch_switch) && Gu8_Call_From_Flag == FROM_SWITCH)		//���������� ȣ�� ���� �� ��. ��, �ϰ� ����ġ���� ���������� ȣ�� �� ���.
			{
				// BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);				//�ش� �����Ͱ� ���� �������� ó����.
				//���� �����ʹ� D4�� 0�϶� �������� ó��.
				Gu8_Call_From_Flag = 0;
			}
		}
		else if(pRx->buf[HDC_F_D4] == 0)								//���е忡�� ȣ�� �� ������ ���� ������ 0
		{
			// if(GET_Switch_State(touch_switch) && Gu8_Call_From_Flag == FROM_WALLPAD)			//���������� ȣ�� ���� �� ��. ��, ���е忡�� ���������� ȣ�� �� ���.
			if(GET_Switch_State(touch_switch))	//D4�� 0�̸� ���е忡���� ȣ��, ����ġ���� ȣ�� ��� ���� ó��.
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);				//�ش� �����Ͱ� ���� �������� ó����.
				Gu8_Call_From_Flag = 0;
			}
		}
	}
}

uint8_t HDC_Error_Data_Check(HDC_BUF *pRx)
{
    uint8_t HDC_CMD;
    HDC_CMD			= pRx->buf[HDC_F_CMD];

	if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF) && item2tsn(mapping_ITEM_ELEVATOR))		//�ϰ�, ����, ���� or �ϰ�, ����
	{
		if(pRx->buf[HDC_F_ID] != BATCH_BLOCK_n_GAS_EV_ID)	return 1;
	}
	else if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF) && item2tsn(mapping_ITEM_GAS) && (item2tsn(mapping_ITEM_ELEVATOR) == 0))	//�ϰ�, ����
	{
		if(pRx->buf[HDC_F_ID] != BATCH_BLOCK_n_GAS_ID)		return 1;
	}
	else if(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF) && (item2tsn(mapping_ITEM_GAS) == 0) && item2tsn(mapping_ITEM_ELEVATOR) == 0)	//�ϰ�
	{
		if(pRx->buf[HDC_F_ID] != BATCH_BLOCK_ID)			return 1;
	}
	
	if(pRx->buf[HDC_F_D5] != 0)								return 1;
	
	if(HDC_CMD == HDC_CMD_STATE_REQ)
	{
		if(pRx->buf[HDC_F_D4] != 0)							return 1;	//CMD0x01���� D4�� 0�� �ƴϸ� ����
	}
	if(HDC_CMD == HDC_CMD_VERSION_INFO_REQ)
	{
		if(pRx->buf[HDC_F_D4] != 0)	return 1;
	}

	if(HDC_CMD == HDC_CMD_STATE_RELEASE_REQ)
	{
		if(pRx->buf[HDC_F_D4] & HDC_REVERSED)				return 1;	//��� ���� �������϶�(����Ȯ��(0x02), �������(0x10), X(0x04, 0x08))
		else
		{
			if(item2tsn(mapping_ITEM_GAS) == 0)
			{
				if(pRx->buf[HDC_F_D4] & HDC_GAS_CLOSE)		return 1;		//����ġ�� ���� ��� ���� �� D4�� ���� ���� ������ ���� ����
			}
			if(item2tsn(mapping_ITEM_ELEVATOR) == 0)
			{
				if(pRx->buf[HDC_F_D4] & HDC_ELEVATOR_CALL)	return 1;		//����ġ�� ���������� ��� ���� �� D4�� ���������� ���� ������ ���� ����
			}
		}
	}
	if(HDC_CMD == HDC_CMD_MODE_CONTROL_REQ)
	{
		if(pRx->buf[HDC_F_D4] & 0xFE)						return 1;
	}
	return 0;
}

void HDC_Data_Process(HDC_BUF	*pRx)
{
    uint8_t HDC_CMD;
    HDC_CMD			= pRx->buf[HDC_F_CMD];

	Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
	
	if(pRx->buf[HDC_F_STX] == HDC_STX)
	{
		if(HDC_Error_Data_Check(pRx))	return;

		switch(HDC_CMD)
		{
			case HDC_CMD_STATE_REQ:
			case HDC_CMD_STATE_RELEASE_REQ:
			case HDC_CMD_MODE_CONTROL_REQ:
			case HDC_CMD_GAS_STATE_REQ:
			case HDC_CMD_ELEVATOR_STATE_REQ:
			case HDC_CMD_VERSION_INFO_REQ:				//���, ���µ� ��� ��� �ƴϸ� ������ ó��. ���� �׽�ƮX
				if(HDC_CMD == HDC_CMD_STATE_RELEASE_REQ)
				{
					HDC_State_Release(pRx);
					HDC_State_ACK(pRx);
					//���е忡�� ���������� ȣ�� �� �ش� Ŀ�ǵ�� ���������� ���� �� ���� ���� ����
				}
				else if(HDC_CMD == HDC_CMD_MODE_CONTROL_REQ)
				{
					HDC_Mode_Control(pRx);
					HDC_State_ACK(pRx);
				}
				else if(HDC_CMD == HDC_CMD_GAS_STATE_REQ)
				{
					if(item2tsn(mapping_ITEM_GAS))
					{
						HDC_Gas_Control(pRx);
						HDC_State_ACK(pRx);
					}
					//�ϰ� ����ġ���� ���� ���� ��û ���� ��쿡�� ���� ���� ���� �ߴٴ� �� ��� �� �� �ִ���.
				}
				else if(HDC_CMD == HDC_CMD_ELEVATOR_STATE_REQ)
				{
					if(item2tsn(mapping_ITEM_ELEVATOR))
					{
						HDC_Elvator_Control(pRx);
						HDC_State_ACK(pRx);
					}
					//ȣ�� ������ ?�� �ش� Ŀ�ǵ� ���� �� �������� ó��(����ġ���� ȣ�� ���� ��)
					//ȣ�� ���°� �ƴ� ? �ش� Ŀ�ǵ� ������ ȣ��� ó��(���е忡�� ȣ�� ���� ?)
				}
				else
				{
					HDC_State_ACK(pRx);
				}
				break;
		}
	}
    else if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))
    {
        RS_485_ID_RES();
    }	
}
#endif
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
			if(touch_switch)
			{
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
			}

			break;
		case SET__GAS_CLOSE_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(touch_switch)
			{
				if(GET_Switch_State(touch_switch) == 0)		//����  ���� �ƴ� ��
				{
					SET_Switch_State(touch_switch, ON);		// ������� ����(����)
					SET_LED_State(touch_switch, OFF);		// �����δ� LED ����
					Beep(ON);
				}
			}
			// printf("GAS CLOSE\r\n");
			break;
		case SET__GAS_OPEN_STATE:
			touch_switch = item2tsn((uint8_t)mapping_ITEM_GAS);
			if(touch_switch)
			{
				if(GET_Switch_State(touch_switch) || (GET_LED_State(touch_switch) == LED_FLASHING))
				{
					SET_Switch_State(touch_switch, OFF);	// ������� ����
					SET_LED_State(touch_switch, ON);		// �����δ� LED ����
					Beep(ON);
				}
			}
			// printf("GAS OPEN\r\n");
			break;
		
		case SET__ELEVATOR_REQUEST:																			//����ġ���� ���������� ��
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(touch_switch)
			{
				if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))		//����ġ OFF �ų� LED ���� Flashing�̸�
				{
					Gu16_Elevator_Tmr = 60;																		//��û Ÿ�̸� 60�� �ʱ�ȭ. 0�̵Ǹ� ��û ��ҵǰ� LED OFF��.
					Gu8_Call_From_Flag = FROM_SWITCH;
					SET_Switch_State(touch_switch, OFF);															//����ġ OFF
					SET_LED_State(touch_switch, LED_FLASHING);													//LED Flashing
					Beep(ON);
					// printf("ELEVATOR REQEUST\r\n");
				}
			}
			break;
		case SET__ELEVATOR_CALL:																			//����⿡�� ���������� ��
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(touch_switch)
			{
				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == 0)			//ȣ�� ��û ���°ų�, ȣ����� �ƴ� ��.
				{
					Gu16_Elevator_Tmr = 60;																			//�� ���°� �Ǹ� Ÿ�̸� 60�� �ʱ�ȭ. Ÿ�̸Ӱ� 0�Ǹ� ���� ������� ���ư�.
					SET_Switch_State(touch_switch, ON);																//����ġ ON
					SET_LED_State(touch_switch, OFF);																//LED ON
					Beep(ON);
				}
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
			if(touch_switch)
			{
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
			}
			break;
	}
}


#endif
// ----------------------------------------------------------------------------------------



/************************************************************************************
	Project		: ���ڽĽ���ġ
	File Name	: RS-485.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
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

#ifdef _HYUNDAI_PROTOCOL_
#define	MAX_ADDRESS		15
HYUNDAI_BUF	RxBuf, TxBuf;

void HYUNDAI_Data_Process(HYUNDAI_BUF	*pRx);
void BATCH_BLOCK_STATE_Process(void);
void BATCH_BLOCK_Control(uint8_t);
uint8_t HYUNDAI_Batch_Light_State(uint8_t item);

#define	MAX_HYUNDAI_DATA_SEQUENCE		9

#define AP_REQ

uint8_t	HYUNDAI_LIGHT_ITEM_Sequence[MAX_HYUNDAI_DATA_SEQUENCE];
uint8_t	HYUNDAI_ELEC_ITEM_Sequence[MAX_HYUNDAI_DATA_SEQUENCE];
uint8_t HYUNDAI_DIMMING_LIGHT_ITEM_Sequence[MAX_HYUNDAI_DATA_SEQUENCE];
uint8_t HYUNDAI_ALL_LIGHT_ITEM_Sequence[MAX_HYUNDAI_DATA_SEQUENCE];			//���� ���������� �Ϲ� ����� ��� ������ �������� �־ �ϰ� �ҵ� �� ���� �� ���� ���� ���� �����.
uint8_t Store_Light_State[MAX_HYUNDAI_DATA_SEQUENCE];
uint8_t Gu8_Batch_Light_Flag;
uint8_t Gu8_External_Flag;
uint16_t Gu16_Req_Tmr = 0;
// ----------------------------------------------------------------------------------------
void SET_HYUNDAI_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_HYUNDAI_DATA_SEQUENCE)
	{
		HYUNDAI_LIGHT_ITEM_Sequence[count]	= item;
	}
}
void SET_HYUNDAI_ELEC_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_HYUNDAI_DATA_SEQUENCE)
	{
		HYUNDAI_ELEC_ITEM_Sequence[count]	= item;
	}
}
void SET_HYUNDAI_DIMMING_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)		//��� ��� ���� ���� �� ����� �κ��� �־ �߰���.
{
	if(count < MAX_HYUNDAI_DATA_SEQUENCE)
	{
		HYUNDAI_DIMMING_LIGHT_ITEM_Sequence[count]	= item;
	}
}
void SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)
{
	if(count < MAX_HYUNDAI_DATA_SEQUENCE)
	{
		HYUNDAI_ALL_LIGHT_ITEM_Sequence[count]	= item;
	}
}
//-------------------------------------------------------------------------------------------------------------------------
// HYUNDAI	��巹�� ���� 1 ~ 15(��巹�� 0�� ���Խ�Ŵ, 0���� �����Ǹ� ����X)

uint8_t Get_485_ID(void)
{
	return	(uint8_t)pG_Config->RS485_ID;		// ��巹��
}
void Protocol_Data_Init(void)
{
    uint8_t	count, i;

	memset((void*)&RxBuf,		0,	sizeof(HYUNDAI_BUF));
	memset((void*)&TxBuf,		0,	sizeof(HYUNDAI_BUF));
	
	Gu16_Elevator_Tmr			= 0;
	Gu16_GAS_Off_Tmr			= 0;
	Gu8_Batch_Light_Flag		= 0;
	memset(HYUNDAI_LIGHT_ITEM_Sequence, 0, MAX_HYUNDAI_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	memset(HYUNDAI_ELEC_ITEM_Sequence, 0, MAX_HYUNDAI_DATA_SEQUENCE);	// 8�� �׸� Ŭ����
	memset(HYUNDAI_DIMMING_LIGHT_ITEM_Sequence, 0, MAX_HYUNDAI_DATA_SEQUENCE);
	memset(HYUNDAI_ALL_LIGHT_ITEM_Sequence, 0, MAX_HYUNDAI_DATA_SEQUENCE);
	// �������� ������ �׸� ����
	// �����	 �ִ��׸�	: ���� 6��, ���� 4�� + ��� 2��
	// ����+��� �ִ��׸�	: ���� 4�� + ��� 2��, ����2�� + ���2�� + ���2��
	// ex) ���� 3��, ��� 2�� = ����1,����2,����3,���1,���2,0,0,0
	// ex) ���� 1��, ��� 1�� = ����1,���1,0,0,0,0,0,0
	
	count = 1;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// �������� ���� ����, ������� ������
	count	= 1;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
#endif
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// �������� ���� �������, ���� ������
	count	= 1;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_HYUNDAI_ALL_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
#endif
	for(i = 1; i < MAX_HYUNDAI_DATA_SEQUENCE; i++)
	{
		Store_Light_State[i]	= HYUNDAI_Batch_Light_State(HYUNDAI_ALL_LIGHT_ITEM_Sequence[i]);
	}	
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
	count	= 1;
	if(item2tsn(mapping_ITEM_ELECTRICITY_1))	SET_HYUNDAI_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_1, count++);
	if(item2tsn(mapping_ITEM_ELECTRICITY_2))	SET_HYUNDAI_ELEC_ITEM_Sequence(mapping_ITEM_ELECTRICITY_2, count++);
	count	= 1;
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_1))	SET_HYUNDAI_DIMMING_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_DIMMING_LIGHT_2))	SET_HYUNDAI_DIMMING_LIGHT_ITEM_Sequence(mapping_ITEM_DIMMING_LIGHT_2, count++);		
#endif    
}

void RS485_IntervalTimeOver_RxdataClear(uint8_t tmr)
{
    if(tmr == 0)		// ������ ������ ���� �� X ms �ʰ��ϸ� ������ Ŭ����
    {
        RxBuf.buf[0]    = 0;
        RxBuf.count     = 0;
    }
}

void Polling_Test(void)
{
	static uint8_t mode = 0;
	HYUNDAI_BUF	*pTx;
	pTx = &TxBuf;
	if(Gu16_Req_Tmr == 0)
	{
		pTx->count = 0;
		pTx->buf[pTx->count++] = 0xF7;
		pTx->buf[pTx->count++] = 0x0B;
		pTx->buf[pTx->count++] = 0x01;
		pTx->buf[pTx->count++] = 0x4D;
		pTx->buf[pTx->count++] = 0x01;
		pTx->buf[pTx->count++] = 0x1F;
		switch(mode)
		{
			case 0:	
				pTx->buf[pTx->count++] = 0x11;	//SRV
				mode++;
				break;
			case 1:
				pTx->buf[pTx->count++] = 0x21;
				mode = 0;
				break;
		}
		pTx->buf[pTx->count++] = 0x00;
		pTx->buf[pTx->count++] = 0x00;
		if(pTx->buf[HD_F_LOC] == 0x11)	pTx->buf[pTx->count++] = 0xBF;
		else							pTx->buf[pTx->count++] = 0x8F;
		pTx->buf[pTx->count++] = 0xEE;

		TxBuf.send_flag	= 1;
		Gu16_Req_Tmr = 30;
	}
}

void RS485_Tx_Process(void)
{
    static uint8_t	_mode_	= 0;
    uint8_t i;

	if(Gu8_RS_485_Tx_Tmr == 0 && Gu8_RS_485_Enable_Tmr == 0)
	{
#ifdef AP_REQ
		if(_mode_ == 0)
		{
			if(GPIO_ReadInputDataBit(RS_485_DE_PORT, (GPIO_Pin_TypeDef)RS_485_DE_PIN) == 0)	// GPIO Read)
			{
				Polling_Test();
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
					printf("TX(HYUNDAI) : ");
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

void HD_Group_Control(HYUNDAI_BUF *pRx, HYUNDAI_BUF *pTx)
{
	if(pRx->buf[HD_F_LOC] == HD_LOC_CIRCUIT_GROUP_ALL)      //��ü �׷� �����϶�
	{
		if(pG_Config->RS485_ID == 0x01)     //1�� ����
		{
			//���� �� 0x10 ���� ����
			pTx->buf[HD_F_LOC] = 0x10;
		}
		else
		{
			//���
			pTx->buf[0] = 0;
			pTx->count = 0;
            TxBuf.send_flag	= 0;
			// break;
		}
	}
	else if(pRx->buf[HD_F_LOC] != HD_LOC_CIRCUIT_GROUP_ALL)     //��ü�׷� ��� �ƴϸ�, ���� �� ����
	{
		pTx->buf[HD_F_LOC] = pRx->buf[HD_F_LOC];
		//���� �� �� ��⿡ �°� ����
	}	
}
// ----------------------------------------------------------------------------------------
uint8_t HYUNDAI_Crc(HYUNDAI_BUF *pTRx, uint16_t len)
{
    uint8_t i, crc = 0;

    for(i = HD_F_STX; i < len; i++)
    {
        crc ^= pTRx->buf[i];
    }
    // printf("crc = 0x%X\r\n\r\n", crc);
    // printf("HD_CRC = 0x%X\r\n\r\n", HYUNDAI_len.HD_CRC);
    return crc;
}

uint8_t HYUNDAI_TX_Crc(HYUNDAI_BUF *pTRx, uint16_t len)
{
    uint8_t i, crc = 0;

    for(i = HD_F_STX; i < len; i++)
    {
        crc ^= pTRx->buf[i];
		printf("buf[%d] = 0x%02X\r\n", (uint16_t)i, (uint16_t)pTRx->buf[i]);
    }
    // printf("crc = 0x%X\r\n\r\n", crc);
    // printf("HD_CRC = 0x%X\r\n\r\n", HYUNDAI_len.HD_CRC);
    return crc;
}
uint8_t NIS_Crc(HYUNDAI_BUF *pTRx, uint8_t cal, uint8_t sel)		//����� 485��� �׽�Ʈ�� ���ؼ� �߰���.
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
void External_ACK(HYUNDAI_BUF *pRx)
{
	HYUNDAI_BUF	*pTx;
	pTx = &TxBuf;

	if(pG_Config->Enable_Flag.ThreeWay == 0) 	return;

	pTx->count = HD_F_STX;
	pTx->buf[pTx->count++]	= HYUNDAI_STX;				//STX
	pTx->buf[pTx->count++]	= 0x0D;						//LEN
	pTx->buf[pTx->count++]	= HYUNDAI_VEN;				//VEN
	pTx->buf[pTx->count++]	= HD_DEV_EXTERNAL_CONTACT;	//DEV
	pTx->buf[pTx->count++]	= HD_TYPE_RES_SUCCESS;		//TYPE
	pTx->buf[pTx->count++]	= HD_SRV_POWER;				//SRV
	pTx->buf[pTx->count++]	= pRx->buf[HD_F_LOC];		//LOC
	pTx->buf[pTx->count++]	= 0x00;						//CMD
	pTx->buf[pTx->count++]	= HD_ARG_EXTERNAL_CONTACT;	//ARG1(aDEVID), �ܺ���������
	pTx->buf[pTx->count++]	= pRx->buf[HD_F_ARG_2];		//ARG2(aLOC), 1�� �׷��� 1�� �ܺ� ����
	if(Gu8_External_Flag)								//ThreeWay() ���� �� �÷��� 1�� ������. ���� ������ ��� 0x01�� ������ ���� �ϰ� ���Ĵ� 0x02�� ������.
	{
		pTx->buf[pTx->count++]	= HD_CMD_ON;							//ARG3(aCMD), 0x01(ON), 0x02(OFF) OR oxo3 ~ 0xFF(DATA)
		Gu8_External_Flag = 0;
	}
	else
	{
		pTx->buf[pTx->count++]	= HD_CMD_OFF;
	}
	pTx->buf[pTx->count++]	= HYUNDAI_Crc(pTx, pTx->count);	//�ܺ����� ���������� ������ ���̰� �����̹Ƿ� pTx->count-2 ���� �ʰ� �����.
	pTx->buf[pTx->count++]	= HYUNDAI_ETX;
	TxBuf.send_flag	= 1;  
}

void RS_485_ID_RES(void)
{
    uint8_t Elec_Watt_MSB, Elec_Watt_LSB, prime_num_1, prime_num_2 = 0;
	HYUNDAI_BUF	*pTx;
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
		Elec_Watt_LSB -+ 10;
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
// ----------------------------------------------------------------------------------------
void Protocol_Process(uint8_t data)
{
	HYUNDAI_BUF	*pRx;
	uint8_t		crc, crc_xor = 0;
    uint8_t     cnt = 0;
    int i;

	pRx = &RxBuf;

	if(G_Debug == DEBUG_HOST_REALDATA)
	{
		if(pG_Config->Protocol_Type == HYUNDAI_PROTOCOL)
		{
			if(data == HYUNDAI_STX)	printf("\n");
		}
		printf("%02X ", (uint16_t)data);
	}
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
	BATCH_BLOCK_STATE_Process();
#endif
    switch(pRx->count)
    {
        default:
            if((pRx->buf[HD_F_STX] != HYUNDAI_STX) && (data == HYUNDAI_STX))
            {
                pRx->count = 0;
            }
            break;
        case 1:
            if((pRx->buf[HD_F_STX] != HYUNDAI_STX) && (pRx->buf[HD_F_STX] != NIS_LIGHT_ID_COMM_1))
            {
                pRx->count = 0;
            }
            break;
        case 2:
            if(pRx->buf[HD_F_LEN] < 0x0B)
            {
                if((pRx->buf[HD_F_STX] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[HD_F_LEN] != NIS_LIGHT_ID_COMM_2))
                {
                    pRx->count = 0;
                }
            }
            break;
        case 3:
            if((pRx->buf[HD_F_VEN] != HYUNDAI_VEN))
            {
                if((pRx->buf[HD_F_STX] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[HD_F_LEN] != NIS_LIGHT_ID_COMM_2))
                {
                    pRx->count = 0;
                }
            }
            break;
        case 5:
            if((pRx->buf[HD_F_TYPE] == HD_TYPE_RES_SUCCESS))
            {
                if((pRx->buf[HD_F_STX] != NIS_LIGHT_ID_COMM_1) && (pRx->buf[HD_F_LEN] != NIS_LIGHT_ID_COMM_2) && (pRx->buf[HD_F_DEV] != HD_DEV_USS) && (pRx->buf[HD_F_DEV] != 0x4D))
                {
                    pRx->count = 0;
                }
            }
            break;
    }
    pRx->buf[pRx->count++] = data;

    if((pRx->buf[HD_F_STX] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[HD_F_LEN] == NIS_LIGHT_ID_COMM_2))
    {
		if(pRx->count >= 8)
		{
			crc = NIS_Crc(pRx, 0, NIS_RX);
			crc_xor = NIS_Crc(pRx, 1, NIS_RX);
			if(crc == pRx->buf[7] && crc_xor == pRx->buf[6])
			{
                if(G_Debug == DEBUG_HOST)
                {
                    printf("\nRX(HYUNDAI) : ");
                    for(i=0;i<pRx->count;i++)
                    {
                        printf("%02X ", (uint16_t)pRx->buf[i]);
                    }
                    printf("\n");
                }
				HYUNDAI_Data_Process(pRx);				
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
        if(pRx->count >= HD_MAX_BUF)
        {
            pRx->buf[0] = 0;        //20210527
            pRx->count = 0;
        }

        if(pRx->count == HD_F_SRV)
        {
            pRx->length.value = pRx->buf[HD_F_LEN];
        }
        cnt = pRx->length.value;

        if(pRx->count == cnt)       //cnt�� LED(������ ����) ��, pRx->count�� ������ ���̿� ���� ��������
        {
            if(data == HYUNDAI_ETX)
            {
                crc = HYUNDAI_Crc(pRx, pRx->count-2);
                
                if(crc == pRx->buf[pRx->count-2])
                {
					if(G_Debug == DEBUG_HOST)
					{
						printf("\nRX(HYUNDAI) : ");
						for(i=0;i<pRx->count;i++)
						{
							printf("%02X ", (uint16_t)pRx->buf[i]);
						}
						printf("\n");
					}
                    HYUNDAI_Data_Process(pRx);
                }
                else
                {
					Beep(ON);
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
			if(pG_State->Dimming_Level.Dimming1	!= (uint8_t)Dimming_Level)	Beep(ON);				// ���е忡�� ���������� ���������� ���� �ƴ��� ������ ������
			pG_State->Dimming_Level.Dimming1		= (uint8_t)Dimming_Level;
			if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:			
			if(pG_State->Dimming_Level.Dimming2	!= (uint8_t)Dimming_Level)	Beep(ON);				// ���е忡�� ���������� ���������� ���� �ƴ��� ������ ������
			pG_State->Dimming_Level.Dimming2		= (uint8_t)Dimming_Level;
			if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)	// ������ ������ �ִ� ũ�⸦ ������
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// �ִ� ũ��� ����
			}			
			break;
	}
}

uint8_t HYUNDAI_Batch_Light_State(uint8_t item)		//�ϰ� �ҵ� �� ���� ���� ���� ����.
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
				ret = HYUNDAI_ON_FLAG;
			}
			else
			{
				ret = HYUNDAI_OFF_FLAG;
			}
			break;
	}
	return ret;
}

void HYUNDAI_BatchLight_Control(uint8_t item, uint8_t control_value)
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
			if(control_value == HYUNDAI_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);						//���� OFF
			}
			else if(control_value == HYUNDAI_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);						//���� ON
			}
			break;
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// ����
			break;
	}
}

void HYUNDAI_Control(HYUNDAI_BUF *pRx, uint8_t item)
{
	uint8_t ret, control, touch_switch;
	control = pRx->buf[HD_F_CMD];

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
			if(control == HYUNDAI_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == OFF)	EventCtrl(item2tsn(item), ON);
			}
			else
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:
		case mapping_ITEM_DIMMING_LIGHT_2:
			if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
			{
				if(control == HYUNDAI_ON_FLAG)
				{
					if(GET_Switch_State(item2tsn(item)) == OFF)	EventCtrl(item2tsn(item), ON);
				}
				else
				{
					if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);
				}
			}
			else if(pRx->buf[HD_F_SRV] == HD_SRV_DIMMING)
			{
				if(GET_Switch_State(item2tsn(item)))
				{
					Gu8_LCD_ElecLimitCurrent_Tmr	= 0;
					Gu8_LCD_DIM_Tmr					= 20;
					SET_DimmingLevel(item, control);
				}
			}
			break;
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
			{
				if(control == HYUNDAI_ON_FLAG)
				{
					if(GET_Switch_State(item2tsn(item)) == OFF)
					{ 
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
				else		//HYUNDAI_OFF_FLAG
				{
					if(GET_Switch_State(item2tsn(item)))
					{
						touch_switch	= item2tsn(item);
						SET_Switch_State(touch_switch, OFF);
						SET_LED_State(touch_switch, OFF);
						Beep(OFF);
						PUT_RelayCtrl(item2ctrl(item), OFF);	// �׸���� ����
						SET_SWITCH_Delay_OFF_Flag(item, 0);
						SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
						ALL_Electricity_Switch_LED_Ctrl();
					}
				}
			}
			else if(pRx->buf[HD_F_SRV] == HD_SRV_POWERSAVING_MODE)
			{
				if(control == HYUNDAI_ON_FLAG)
				{
					if(item == mapping_ITEM_ELECTRICITY_1)			pG_State->ETC.Auto1	= 1;
					else if(item == mapping_ITEM_ELECTRICITY_2)		pG_State->ETC.Auto2 = 1;
				}
				else		//HYUNDAI_OFF_FLAG
				{
					if(item == mapping_ITEM_ELECTRICITY_1)			pG_State->ETC.Auto1	= 0;
					else if(item == mapping_ITEM_ELECTRICITY_2)		pG_State->ETC.Auto2 = 0;
				}
			}
			break;
	}
}

uint8_t HYUNDAI_LIGHT_Data_Conversion(uint8_t item, uint8_t ack)
{
	uint8_t	ret = 0;
	
	switch(item)
	{
		default:
			ret = HYUNDAI_OFF_FLAG;
			break;
		case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
		case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
		case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
		case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
		case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
		case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			if(ack == HYUNDAI_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn(item)))			ret = HYUNDAI_ON_FLAG;
				else											ret = HYUNDAI_OFF_FLAG;
			}
			else												ret = HYUNDAI_OFF_FLAG;
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			if(ack == HYUNDAI_DIMMING)
			{
				if(GET_Switch_State(item2tsn(item)))			ret = pG_State->Dimming_Level.Dimming1;
				else											ret = 0;
			}
			else if(ack == HYUNDAI_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn(item)))			ret = HYUNDAI_ON_FLAG;
				else											ret = HYUNDAI_OFF_FLAG;
			}
			else												ret = HYUNDAI_OFF_FLAG;
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
			if(ack == HYUNDAI_DIMMING)
			{
				if(GET_Switch_State(item2tsn(item)))			ret = pG_State->Dimming_Level.Dimming2;
				else											ret = 0;
			}
			else if(ack == HYUNDAI_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn(item)))			ret = HYUNDAI_ON_FLAG;
				else											ret = HYUNDAI_OFF_FLAG;
			}
			else												ret = HYUNDAI_OFF_FLAG;
			break;
	}
	return	ret;
}

#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_)
void HYUNDAI_ACK(HYUNDAI_BUF *pRx)
{
	uint8_t Light_Cnt, item, i = 0;
    uint8_t Loc_MSB, Loc_LSB;

	HYUNDAI_BUF	*pTx;
	pTx = &TxBuf;

    pTx->count	= HD_F_STX;
    pTx->buf[pTx->count++] = HYUNDAI_STX;
    pTx->buf[pTx->count++] = pRx->buf[HD_F_LEN];
    pTx->buf[pTx->count++] = HYUNDAI_VEN;
    pTx->buf[pTx->count++] = pRx->buf[HD_F_DEV];
    pTx->buf[pTx->count++] = HD_TYPE_RES_SUCCESS;
    pTx->buf[pTx->count++] = pRx->buf[HD_F_SRV];
    pTx->buf[pTx->count++] = pRx->buf[HD_F_LOC];
    pTx->buf[pTx->count++] = pRx->buf[HD_F_CMD];

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC ���� �Ϻ�
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC ���� �Ϻ�

    Light_Cnt	= pG_Config->LightCount;

	if(pRx->buf[HD_F_DEV] == HD_DEV_LIGHT)
	{
		item = HYUNDAI_LIGHT_ITEM_Sequence[Loc_LSB];
		if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
		{
			if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
			{
				for(i = mapping_ITEM_LIGHT_1; i < (uint8_t)(mapping_ITEM_LIGHT_1 + Light_Cnt); i++)
				{
					if(item2tsn(i))
					{
						pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(i, HYUNDAI_ON_n_OFF);
					}
				}
			}
			else
			{
				pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(item, HYUNDAI_ON_n_OFF);
			}
		}
		else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
		{
			if(Loc_LSB == HD_LOC_CIRCUIT_ALL)   pTx->buf[pTx->count++] = pRx->buf[HD_F_CMD];
			else                                pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(item, HYUNDAI_ON_n_OFF);
		}
		pTx->count++;		//CRC�� pTx->count üũ �� LEN ���� ������  ���̸� �缳���ϰ� ����ؾ��ϹǷ� ī��Ʈ��++��.
		pTx->buf[pTx->count++] = HYUNDAI_ETX;
		pTx->buf[HD_F_LEN] = (uint8_t)pTx->count;
		pTx->buf[pTx->count-2] = HYUNDAI_Crc(pTx, pTx->count-2);
		TxBuf.send_flag	= 1;
		HD_Group_Control(pRx, pTx);
	}
}

void HYUNDAI_Data_Process(HYUNDAI_BUF	*pRx)
{
    uint8_t     Loc_MSB, Loc_LSB, item;
	uint8_t 	Data_Cnt, i = 0, j = 0, k = 0;

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC ���� �Ϻ�
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC ���� �Ϻ�

    if(Loc_MSB == (uint8_t)pG_Config->RS485_ID && (pRx->buf[HD_F_DEV] == HD_DEV_LIGHT) || (pRx->buf[HD_F_DEV] == HD_DEV_USS && pRx->buf[HD_F_TYPE] == HD_TYPE_RES_SUCCESS))
    {
        if(G_Debug == DEBUG_HOST)
        {
            printf("\nRX(HYUNDAI) : ");
            for(i = 0; i < (uint16_t)pRx->buf[HD_F_LEN]; i++)
            {
                printf("%02X ", (uint16_t)pRx->buf[i]);
            }
            printf("\n");
        }	
	}
	if(pRx->buf[HD_F_DEV] == HD_DEV_LIGHT)
	{
		if(Loc_LSB > pG_Config->LightCount) return;	//LOC�� ���� ���� ���� ������ ����
	}

    if(Loc_MSB == Get_485_ID() || pRx->buf[HD_F_LOC] == HD_LOC_CIRCUIT_GROUP_ALL)
    {
		Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
        switch(pRx->buf[HD_F_DEV])
        {
            case HD_DEV_LIGHT:
                if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
                {
					if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
					{
						HYUNDAI_ACK(pRx);
					}
                }
                else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
                {
                    if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
                    {
						if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
						{
							for(i = 1; i < MAX_HYUNDAI_DATA_SEQUENCE; i++)
							{
								HYUNDAI_Control(pRx, i);
							}
						}
						else
						{
							item = HYUNDAI_LIGHT_ITEM_Sequence[Loc_LSB];
							HYUNDAI_Control(pRx, item);
						}
						HYUNDAI_ACK(pRx);
                    }
                }
                break;
        }
    }
	if(pRx->buf[HD_F_DEV] == HD_DEV_USS && pRx->buf[HD_F_TYPE] == HD_TYPE_RES_SUCCESS)	//�ϰ� �ҵ ���� ������ �ʿ��ϸ� ����.
	{
		Data_Cnt = (uint8_t)((pRx->buf[HD_F_LEN] - 10) / 2);
		j = HD_F_ARG_1;
		for(i = 0; i < Data_Cnt; i++)
		{
			if(pRx->buf[j] == HD_DEV_LIGHT)
			{
				j++;
				if(pRx->buf[j] == HYUNDAI_ON_FLAG)			//�ϰ� �ҵ� ����
				{
					if(Gu8_Batch_Light_Flag != 0x01)
					{
						for(k = 1; k < MAX_HYUNDAI_DATA_SEQUENCE; k++)
						{
							Store_Light_State[k] = HYUNDAI_Batch_Light_State(HYUNDAI_ALL_LIGHT_ITEM_Sequence[k]);	//���¸� ����. ����� SRV : 0x40 ���� ���� ��ü ������.
						}
						Gu8_Batch_Light_Flag = 0x01;
						printf("BATCH LIGHT OFF\r\n");
					}
				}
				else if(pRx->buf[j] == HYUNDAI_OFF_FLAG)	//�ϰ� �ҵ� ���� ����
				{
					if(Gu8_Batch_Light_Flag != 0x02)
					{
						for(k = 1; k < MAX_HYUNDAI_DATA_SEQUENCE; k++)
						{
							if(Store_Light_State[k] == HYUNDAI_ON_FLAG)
							{
								HYUNDAI_BatchLight_Control(HYUNDAI_ALL_LIGHT_ITEM_Sequence[k], HYUNDAI_ON_FLAG);
							}
						}
						Gu8_Batch_Light_Flag = 0x02;
						printf("BATCH LIGHT ON\r\n");
					}
				}
				j++;
			}
			else
			{
				printf("i : %d 0x%X\r\n", (uint16_t)i, (uint16_t)pRx->buf[j]);
				j++;
				printf("i : %d 0x%X\r\n", (uint16_t)i, (uint16_t)pRx->buf[j]);
				j++;
			}
		}
	}

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
#endif

#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined (_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
uint8_t HYUNDAI_ELEC_Data_Conversion(uint8_t item, uint8_t ack)
{
	uint8_t	ret = 0;

	switch(item)
	{
		default:
			ret = 0x00;
			break;
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			if(ack == HYUNDAI_AUTO_MODE)
			{
				if(item == mapping_ITEM_ELECTRICITY_1)
				{
					if(pG_State->ETC.Auto1)	ret = HYUNDAI_ON_FLAG;
					else					ret = HYUNDAI_OFF_FLAG;
				}
				else if(item == mapping_ITEM_ELECTRICITY_2)
				{
					if(pG_State->ETC.Auto2)	ret = HYUNDAI_ON_FLAG;
					else					ret = HYUNDAI_OFF_FLAG;					
				}
			}
			else if(ack == HYUNDAI_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn((uint8_t)item)))	ret	= HYUNDAI_ON_FLAG;
				else											ret	= HYUNDAI_OFF_FLAG;
			}
			break;
	}
	return	ret;
}

uint8_t Watt_Value(uint8_t item, uint8_t select)
{
	uint8_t ret = 0;
	uint8_t	Watt_1_MSB, Watt_1_LSB, Watt_2_MSB, Watt_2_LSB;

	Watt_1_MSB = (uint8_t)(Gu16_LCD_Watt_1 >> 8);
    Watt_1_LSB = (uint8_t)(Gu16_LCD_Watt_1 & 0xFF);
    Watt_2_MSB = (uint8_t)(Gu16_LCD_Watt_2 >> 8);
    Watt_2_LSB = (uint8_t)(Gu16_LCD_Watt_2 & 0xFF);

	switch(item)
	{
		case mapping_ITEM_ELECTRICITY_1:
		case mapping_ITEM_ELECTRICITY_2:
			if(item == mapping_ITEM_ELECTRICITY_1)
			{
				if(select == WATT_MSB)			ret = Watt_1_MSB;
				else if(select == WATT_LSB)		ret = Watt_1_LSB;
			}
			else if(item == mapping_ITEM_ELECTRICITY_2)
			{
				if(select == WATT_MSB)			ret = Watt_2_MSB;
				else if(select == WATT_LSB)		ret = Watt_2_LSB;
			}
			break;
	}
	return ret;
}

void HYUNDAI_ACK(HYUNDAI_BUF *pRx)
{
	uint8_t Light_Cnt, Dimming_Cnt, Data_Cnt, touch_switch, item, i;
    uint8_t Loc_MSB, Loc_LSB;

	HYUNDAI_BUF	*pTx;
	pTx = &TxBuf;

    pTx->count	= HD_F_STX;
    pTx->buf[pTx->count++] = HYUNDAI_STX;
    pTx->buf[pTx->count++] = pRx->buf[HD_F_LEN];
    pTx->buf[pTx->count++] = HYUNDAI_VEN;
    pTx->buf[pTx->count++] = pRx->buf[HD_F_DEV];
    pTx->buf[pTx->count++] = HD_TYPE_RES_SUCCESS;
    pTx->buf[pTx->count++] = pRx->buf[HD_F_SRV];
    pTx->buf[pTx->count++] = pRx->buf[HD_F_LOC];
    pTx->buf[pTx->count++] = pRx->buf[HD_F_CMD];

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC ���� �Ϻ�
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC ���� �Ϻ�

    Light_Cnt	= pG_Config->LightCount;
	Dimming_Cnt = pG_Config->DimmingCount;

    switch(pRx->buf[HD_F_DEV])
    {
        case HD_DEV_LIGHT:
			item = HYUNDAI_LIGHT_ITEM_Sequence[Loc_LSB];
            if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
            {
                if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
                {
                    for(i = mapping_ITEM_LIGHT_1; i < (mapping_ITEM_LIGHT_1 + Light_Cnt); i++)
                    {
                        if(item2tsn(i))	pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(i, HYUNDAI_ON_n_OFF);
                    }
                }
                else
                {
                    pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(item, HYUNDAI_ON_n_OFF);
                }
            }
            else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
            {
                if(Loc_LSB == HD_LOC_CIRCUIT_ALL)   pTx->buf[pTx->count++] = pRx->buf[HD_F_CMD];
                else                                pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(item, HYUNDAI_ON_n_OFF);
            }
            break;
		case HD_DEV_LIGHT_DIMMING:
			item = HYUNDAI_DIMMING_LIGHT_ITEM_Sequence[Loc_LSB];
			if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)	//SRV 0x40�� ON/OFF�� ����
			{
				if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
				{
                    for(i = mapping_ITEM_DIMMING_LIGHT_1; i < (mapping_ITEM_DIMMING_LIGHT_1 + Dimming_Cnt); i++)
                    {
                        if(item2tsn(i))	pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(i, HYUNDAI_ON_n_OFF);
                    }
				}
				else
				{
					pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(item, HYUNDAI_ON_n_OFF);
				}
			}
			else if(pRx->buf[HD_F_SRV] == HD_SRV_DIMMING)	//SRV 0x42�� ��� ������ ����.
			{
				if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
				{
                    for(i = mapping_ITEM_DIMMING_LIGHT_1; i < (mapping_ITEM_DIMMING_LIGHT_1 + Dimming_Cnt); i++)
                    {
                        if(item2tsn(i))	pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(i, HYUNDAI_DIMMING);
                    }
				}
				else
				{
					pTx->buf[pTx->count++] = HYUNDAI_LIGHT_Data_Conversion(item, HYUNDAI_DIMMING);
				}
			}
			break;
		case HD_DEV_ELETRICITY:
			item = HYUNDAI_ELEC_ITEM_Sequence[Loc_LSB];
			if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
			{
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
				{
					if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
					{
						pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[HD_F_LOC] + 1);												//ARG1
						pTx->buf[pTx->count++] = HYUNDAI_ELEC_Data_Conversion(mapping_ITEM_ELECTRICITY_1, HYUNDAI_ON_n_OFF);	//ARG2
						pTx->buf[pTx->count++] = Watt_Value(mapping_ITEM_ELECTRICITY_1, WATT_MSB);								//ARG3
						pTx->buf[pTx->count++] = Watt_Value(mapping_ITEM_ELECTRICITY_1, WATT_LSB);								//ARG4
						pTx->buf[pTx->count++] = 0;																				//ARG5
						pTx->buf[pTx->count++] = 0;																				//ARG6
						pTx->buf[pTx->count++] = 0;																				//ARG7
						pTx->buf[pTx->count++] = 0;																				//ARG8
						pTx->buf[pTx->count++] = HYUNDAI_ELEC_Data_Conversion(mapping_ITEM_ELECTRICITY_1, HYUNDAI_AUTO_MODE);	//ARG9
						
						pTx->buf[pTx->count++] = (uint8_t)(pRx->buf[HD_F_LOC] + 2);												//ARG10
						pTx->buf[pTx->count++] = HYUNDAI_ELEC_Data_Conversion(mapping_ITEM_ELECTRICITY_2, HYUNDAI_ON_n_OFF);	//ARG11
						pTx->buf[pTx->count++] = Watt_Value(mapping_ITEM_ELECTRICITY_2, WATT_MSB);								//ARG12
						pTx->buf[pTx->count++] = Watt_Value(mapping_ITEM_ELECTRICITY_2, WATT_LSB);								//ARG13
						pTx->buf[pTx->count++] = 0;																				//ARG14
						pTx->buf[pTx->count++] = 0;																				//ARG15
						pTx->buf[pTx->count++] = 0;																				//ARG16
						pTx->buf[pTx->count++] = 0;																				//ARG17
						pTx->buf[pTx->count++] = HYUNDAI_ELEC_Data_Conversion(mapping_ITEM_ELECTRICITY_2, HYUNDAI_AUTO_MODE);	//ARG18			
					}
					else
					{
						pTx->buf[pTx->count++] = HYUNDAI_ELEC_Data_Conversion(item, HYUNDAI_ON_n_OFF);
						pTx->buf[pTx->count++] = Watt_Value(item, WATT_MSB);
						pTx->buf[pTx->count++] = Watt_Value(item, WATT_LSB);
						pTx->buf[pTx->count++] = 0;
						pTx->buf[pTx->count++] = 0;
						pTx->buf[pTx->count++] = 0;
						pTx->buf[pTx->count++] = 0;
						pTx->buf[pTx->count++] = HYUNDAI_ELEC_Data_Conversion(item, HYUNDAI_AUTO_MODE);
					}
				}
				else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)	//��ü ��� ���� ������ Ÿ�� �����͸� �ٲ㼭 ����.
				{
					if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
					{
						for(i = HD_F_ARG_1; i < (pRx->buf[HD_F_LEN] - 2); i++)
						{
							pTx->buf[pTx->count++] = pRx->buf[i];
						}
					}
					else
					{
						pTx->buf[pTx->count++] = HYUNDAI_ELEC_Data_Conversion(item, HYUNDAI_ON_n_OFF);	//ARG1
					}
				}
			}
			else if(pRx->buf[HD_F_SRV] == HD_SRV_POWERSAVING_MODE)
			{
				pTx->buf[pTx->count++] = HYUNDAI_ELEC_Data_Conversion(item, HYUNDAI_AUTO_MODE);
			}
			break;
	}
	pTx->count++;		//CRC�� pTx->count üũ �� LEN ���� ������  ���̸� �缳���ϰ� ����ؾ��ϹǷ� ī��Ʈ��++��.
	pTx->buf[pTx->count++] = HYUNDAI_ETX;
	pTx->buf[HD_F_LEN] = (uint8_t)pTx->count;
	pTx->buf[pTx->count-2] = HYUNDAI_Crc(pTx, pTx->count-2);
	TxBuf.send_flag	= 1;
	HD_Group_Control(pRx, pTx);
}

void HYUNDAI_Data_Process(HYUNDAI_BUF	*pRx)
{
	uint8_t		Dimming_Cnt, Light_Cnt, Data_Cnt, Arg_Cnt, item, i, touch_switch;
	uint8_t 	Loc_MSB, Loc_LSB;
	uint8_t		j = 0, k = 0;

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC ���� �Ϻ�
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC ���� �Ϻ�

    Light_Cnt	= pG_Config->LightCount;
	Dimming_Cnt = pG_Config->DimmingCount;

    if(Loc_MSB == pG_Config->RS485_ID || pRx->buf[HD_F_LOC] == HD_LOC_CIRCUIT_GROUP_ALL)
    {
        if(pRx->buf[HD_F_DEV] == HD_DEV_LIGHT || pRx->buf[HD_F_DEV] == HD_DEV_LIGHT_DIMMING || pRx->buf[HD_F_DEV] == HD_DEV_ELETRICITY || pRx->buf[HD_F_DEV] == HD_DEV_STANDBY_POWER || (pRx->buf[HD_F_DEV] == HD_DEV_USS && pRx->buf[HD_F_TYPE] == HD_TYPE_RES_SUCCESS))
        {
            if(G_Debug == DEBUG_HOST)
            {
                printf("\nRX(HYUNDAI) : ");
                for(i = 0; i < (uint16_t)pRx->buf[HD_F_LEN]; i++)
                {
                    printf("%02X ", (uint16_t)pRx->buf[i]);
                }
                printf("\n");
            }
        }
    }

	if(pRx->buf[HD_F_DEV] == HD_DEV_LIGHT || pRx->buf[HD_F_DEV] == HD_DEV_LIGHT_DIMMING)
	{
		if(Loc_LSB > pG_Config->LightCount + pG_Config->DimmingCount) return;	//LOC�� ���� ���� ���� ������ ����
	}
	if(pRx->buf[HD_F_DEV] == HD_DEV_ELETRICITY || pRx->buf[HD_F_DEV] == HD_DEV_STANDBY_POWER)
	{
		if(Loc_LSB > pG_Config->ElectricityCount) return;	//LOC�� ���� ���� ���� ������ ����
	}

    if(Loc_MSB == Get_485_ID() || pRx->buf[HD_F_LOC] == HD_LOC_CIRCUIT_GROUP_ALL)
    {
		Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;

        switch(pRx->buf[HD_F_DEV])
        {
            case HD_DEV_LIGHT:
                if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
                {
					if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
					{
						HYUNDAI_ACK(pRx);
					}
                }
                else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
                {
                    if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
                    {
						if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
						{
							for(i = mapping_ITEM_LIGHT_1; i < (mapping_ITEM_LIGHT_1 + Light_Cnt); i++)
							{
								HYUNDAI_Control(pRx, i);
							}
						}
						else
						{
							item = HYUNDAI_LIGHT_ITEM_Sequence[Loc_LSB];
							HYUNDAI_Control(pRx, item);
						}
						HYUNDAI_ACK(pRx);
                    }
                }
                break;
			case HD_DEV_LIGHT_DIMMING:
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
				{
					if(pRx->buf[HD_F_SRV] == HD_SRV_POWER || pRx->buf[HD_F_SRV] == HD_SRV_DIMMING)
					{
						HYUNDAI_ACK(pRx);
					}
				}
				else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
				{
					if(pRx->buf[HD_F_SRV] == HD_SRV_POWER || pRx->buf[HD_F_SRV] == HD_SRV_DIMMING)
					{
						if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
						{
							for(i = mapping_ITEM_DIMMING_LIGHT_1; i < (mapping_ITEM_DIMMING_LIGHT_1 + Dimming_Cnt); i++)
							{
								HYUNDAI_Control(pRx, i);	//HYUNDAI_Control �Լ����� SRV �ľ��Ͽ� ON/OFF ���� �ϰų�, ��� ���� ������.
							}
						}
						else
						{
							item = HYUNDAI_DIMMING_LIGHT_ITEM_Sequence[Loc_LSB];
							HYUNDAI_Control(pRx, item);
						}
						HYUNDAI_ACK(pRx);
					}					
				}
				break;
			case HD_DEV_STANDBY_POWER:
				break;
			case HD_DEV_ELETRICITY:
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
				{
					if(pRx->buf[HD_F_SRV] == HD_SRV_POWER || pRx->buf[HD_F_SRV] == HD_SRV_POWERSAVING_MODE)
					{
						HYUNDAI_ACK(pRx);
					}
				}
				else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
				{
					if(pRx->buf[HD_F_SRV] == HD_SRV_POWER || pRx->buf[HD_F_SRV] == HD_SRV_POWERSAVING_MODE)
					{
						if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
						{
							Data_Cnt = (uint8_t)((pRx->buf[HD_F_LEN] - 10) / 2);
							Arg_Cnt = HD_F_ARG_1;
							for(i = 0; i < Data_Cnt; i++)
							{
								item = HYUNDAI_ELEC_ITEM_Sequence[(pRx->buf[Arg_Cnt++] & 0x0F)];
								if(pRx->buf[Arg_Cnt] == HYUNDAI_ON_FLAG)
								{
									if(GET_Switch_State(item2tsn(item)) == OFF)
									{
										Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
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
								else
								{
									if(GET_Switch_State(item2tsn(item)) == ON)
									{
										Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;	// 5sec
										touch_switch	= item2tsn(item);
										SET_Switch_State(touch_switch, OFF);
										SET_LED_State(touch_switch, OFF);
										Beep(OFF);
										PUT_RelayCtrl(item2ctrl(item), OFF);	// �׸���� ����
										SET_SWITCH_Delay_OFF_Flag(item, 0);
										SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// ������ �ϳ��� ������ ���� �����ҵ� �÷��״� Ŭ����
										ALL_Electricity_Switch_LED_Ctrl();
									}
								}
								Arg_Cnt++;
							}
						}
						else
						{
							item = HYUNDAI_ELEC_ITEM_Sequence[Loc_LSB];
							HYUNDAI_Control(pRx, item);
						}
						HYUNDAI_ACK(pRx);
					}
				}
				break;
        }
    }

	if(pRx->buf[HD_F_DEV] == HD_DEV_USS && pRx->buf[HD_F_TYPE] == HD_TYPE_RES_SUCCESS)	//�ϰ� �ҵ ���� ������ �ʿ��ϸ� ����.
	{
		Data_Cnt = (uint8_t)((pRx->buf[HD_F_LEN] - 10) / 2);
		j = HD_F_ARG_1;
		for(i = 0; i < Data_Cnt; i++)
		{
			if(pRx->buf[j] == HD_DEV_LIGHT)
			{
				j++;
				if(pRx->buf[j] == HYUNDAI_ON_FLAG)			//�ϰ� �ҵ� ����
				{
					if(Gu8_Batch_Light_Flag != 0x01)
					{
						for(k = 1; k < MAX_HYUNDAI_DATA_SEQUENCE; k++)
						{
							Store_Light_State[k] = HYUNDAI_Batch_Light_State(HYUNDAI_ALL_LIGHT_ITEM_Sequence[k]);	//���¸� ����. ����� SRV : 0x40 ���� ���� ��ü ������.
						}
						Gu8_Batch_Light_Flag = 0x01;
						printf("BATCH LIGHT OFF\r\n");
					}
				}
				else if(pRx->buf[j] == HYUNDAI_OFF_FLAG)	//�ϰ� �ҵ� ���� ����
				{
					if(Gu8_Batch_Light_Flag != 0x02)
					{
						for(k = 1; k < MAX_HYUNDAI_DATA_SEQUENCE; k++)
						{
							if(Store_Light_State[k] == HYUNDAI_ON_FLAG)
							{
								HYUNDAI_BatchLight_Control(HYUNDAI_ALL_LIGHT_ITEM_Sequence[k], HYUNDAI_ON_FLAG);
							}
						}
						Gu8_Batch_Light_Flag = 0x02;
						printf("BATCH LIGHT ON\r\n");
					}
				}
				j++;
			}
			else
			{
				// printf("i : %d 0x%X\r\n", (uint16_t)i, (uint16_t)pRx->buf[j]);
				j++;
				// printf("i : %d 0x%X\r\n", (uint16_t)i, (uint16_t)pRx->buf[j]);
				j++;
			}
		}
	}

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
#endif

#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
void HYUNDAI_ACK(HYUNDAI_BUF *pRx)
{
	uint8_t  Data_Cnt, touch_switch, i;
    uint8_t     Loc_MSB, Loc_LSB;

	HYUNDAI_BUF	*pTx;
	pTx = &TxBuf;

    pTx->count	= HD_F_STX;
    pTx->buf[pTx->count++] = HYUNDAI_STX;
    pTx->buf[pTx->count++] = pRx->buf[HD_F_LEN];
    pTx->buf[pTx->count++] = HYUNDAI_VEN;
    pTx->buf[pTx->count++] = pRx->buf[HD_F_DEV];
    pTx->buf[pTx->count++] = HD_TYPE_RES_SUCCESS;
    pTx->buf[pTx->count++] = pRx->buf[HD_F_SRV];
    pTx->buf[pTx->count++] = pRx->buf[HD_F_LOC];
    pTx->buf[pTx->count++] = pRx->buf[HD_F_CMD];

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC ���� �Ϻ�
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC ���� �Ϻ�

    switch(pRx->buf[HD_F_DEV])
    {
		case HD_DEV_USS:
			if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
			{
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
				{
					if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_USS)	//�ϰ����� ���¿�û
						{
							if(item2tsn(mapping_ITEM_GAS))		//���� ���� ����� ���� ��,
							{
								if((GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == 0) && (GET_Switch_State(item2tsn(mapping_ITEM_GAS))))		pTx->buf[HD_F_ARG_2] = HD_CMD_BLOCK;
								else              																											pTx->buf[HD_F_ARG_2] = HD_CMD_UNBLOCK;     //�ϰ�, ���� ���� �ϳ��� ���� ������ UNBLOCK(0x02)									
							}
							else								//���� ���� ����� ���� ��,
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == 0)		pTx->buf[HD_F_ARG_2] = HD_CMD_BLOCK;
								else																	pTx->buf[HD_F_ARG_2] = HD_CMD_UNBLOCK;
							}
						}
						else									//�ϰ� ���� ��ü ���¿�û
						{
							Data_Cnt = (uint8_t)((pRx->buf[HD_F_LEN] - 10) / 2);
							for(i = 0; i < Data_Cnt; i++)
							{
								if(pRx->buf[pTx->count] == HD_DEV_LIGHT)
								{
									pTx->buf[pTx->count++] = HD_DEV_LIGHT;
									if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))		pTx->buf[pTx->count++] = HD_CMD_UNBLOCK;
									else																pTx->buf[pTx->count++] = HD_CMD_BLOCK;
								}
								else if(pRx->buf[pTx->count] == HD_DEV_GAS)
								{
									if(item2tsn(mapping_ITEM_GAS))
									{
										pTx->buf[pTx->count++] = HD_DEV_GAS;
										if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))					pTx->buf[pTx->count++] = HD_CMD_CLOSE;
										else
										{
											if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)	pTx->buf[pTx->count++] = HD_CMD_CLOSE;		//���� ���� ��û�̸� �������� ����
											else															pTx->buf[pTx->count++] = HD_CMD_OPEN;		//���� �����������̸� �������� ����
										}
									}
									else
									{
										pTx->buf[pTx->count++] = pRx->buf[pTx->count];
										pTx->buf[pTx->count++] = pRx->buf[pTx->count];										
									}
								}
								else
								{
									pTx->buf[pTx->count++] = pRx->buf[pTx->count];
									pTx->buf[pTx->count++] = pRx->buf[pTx->count];
								}
							}
						}
					}
					else											//���� ȸ�� ����
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_LIGHT)	//�ϰ� �ҵ� ���� ��û
						{
							pTx->buf[pTx->count++] = HD_DEV_LIGHT;
							if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == OFF)			pTx->buf[pTx->count++] = HD_CMD_BLOCK;
							else																		pTx->buf[pTx->count++] = HD_CMD_UNBLOCK;							
						}
					}					
				}
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
				{
					if(Loc_LSB == HD_LOC_CIRCUIT_ALL)				//��ü ȸ�� ����
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_USS)		//�ϰ� ���� ����
						{
							pTx->buf[pTx->count++] = HD_DEV_USS;
							if(item2tsn(mapping_ITEM_GAS))			//���� ���� ��� ���� ��,
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == OFF && GET_Switch_State(item2tsn(mapping_ITEM_GAS)))
								{									//�ϰ�, ���� �� �� ���� ���� �� ��,
									pTx->buf[pTx->count++] = HD_CMD_BLOCK;
								}
								else								//�ϰ�, ���� �� �� �ϳ��� ������ �ƴϸ�
								{
									pTx->buf[pTx->count++] = HD_CMD_UNBLOCK;
								}
							}
							else	//���� ���� ��� ���� ��
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == OFF)		pTx->buf[pTx->count++] = HD_CMD_BLOCK;		//�ϰ��ҵ��̸� �������� ����
								else																	pTx->buf[pTx->count++] = HD_CMD_UNBLOCK;	//�ϰ��ҵ������� ���������� ����
							}
						}
					}
					else	//���� ȸ�� ����
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_LIGHT)	//�ϰ� �ҵ� ����
						{
							pTx->buf[pTx->count++] = HD_DEV_LIGHT;
							if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == OFF)		pTx->buf[pTx->count++] = HD_CMD_BLOCK;			//�ϰ��ҵ��̸� �������� ����
							else																	pTx->buf[pTx->count++] = HD_CMD_UNBLOCK;		//�ϰ��ҵ������� ���������� ����
						}
					}
				}
			}
			else if(pRx->buf[HD_F_SRV] == HD_SRV_OPEN_n_CLOSE)		//�ϰ����� ���¿�û
			{
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ || pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
				{
					if(pRx->buf[HD_F_ARG_1] == HD_DEV_GAS)
					{
						pTx->buf[pTx->count++] = HD_DEV_GAS;
						if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))	pTx->buf[pTx->count++] = HD_CMD_CLOSE;		//���� �������̸� �������� ����
						else
						{
							if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)	pTx->buf[pTx->count++] = HD_CMD_CLOSE;		//���� ���� ��û�̸� �������� ����
							else															pTx->buf[pTx->count++] = HD_CMD_OPEN;		//���� �����������̸� �������� ����
						}
					}
				}
			}
			break;
		case HD_DEV_ELEVATOR_CALL:
			if(item2tsn(mapping_ITEM_ELEVATOR))
			{
				touch_switch = item2tsn(mapping_ITEM_ELEVATOR);
				if(pRx->buf[HD_F_SRV] == HD_SRV_ELEVATOR_UP_n_DOWN)
				{
					if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
					{
						if(Loc_LSB == HD_LOC_CIRCUIT_ALL)		//���������� ���� ��û(Polling)
						{
							if(GET_Switch_State(touch_switch))	//ȣ�� ���¸� ���е忡�� �� ������ �״�� ����.
							{
								pTx->buf[pTx->count++] = pRx->buf[HD_F_ARG_1];
							}
							else													//ȣ�� ��û ���¸� ����ȣ��. ȣ�� ���°� �ƴϸ� ���� ������ ����.
							{
								if(GET_LED_State(touch_switch) == LED_FLASHING)					pTx->buf[pTx->count++] = HD_ARG_CALL_DOWN;
								else															pTx->buf[pTx->count++] = HD_ARG_NORMAL;
							}	
						}
						else									//���������� ���� ��û(Query)
						{
							if(GET_Switch_State(touch_switch))
							{
								pTx->buf[pTx->count++] = HD_ARG_CALL_DOWN;
							}
							else
							{
								if(GET_LED_State(touch_switch) == LED_FLASHING)					pTx->buf[pTx->count++] = HD_ARG_CALL_DOWN;
								else															pTx->buf[pTx->count++] = HD_ARG_NORMAL;
							}
						}
					}
					else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
					{
						if(GET_Switch_State(touch_switch))										pTx->buf[pTx->count++] = pRx->buf[HD_F_CMD];
						else																	pTx->buf[pTx->count++] = HD_ARG_NORMAL;
					}
				}
			}
			break;
    }
	pTx->count++;		//CRC�� pTx->count üũ �� LEN ���� ������  ���̸� �缳���ϰ� ����ؾ��ϹǷ� ī��Ʈ��++��.
	pTx->buf[pTx->count++] = HYUNDAI_ETX;
	pTx->buf[HD_F_LEN] = (uint8_t)pTx->count;
	pTx->buf[pTx->count-2] = HYUNDAI_Crc(pTx, pTx->count-2);
	TxBuf.send_flag	= 1;
	HD_Group_Control(pRx, pTx);
}

void HYUNDAI_Data_Process(HYUNDAI_BUF	*pRx)
{
    uint8_t     Loc_MSB, Loc_LSB, Data_Cnt, i, cnt = 0;

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC ���� �Ϻ�
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC ���� �Ϻ�

	if(Loc_MSB == (uint8_t)pG_Config->RS485_ID && (pRx->buf[HD_F_DEV] == HD_DEV_USS || pRx->buf[HD_F_DEV] == HD_DEV_ELEVATOR_CALL))
	{
		if(G_Debug == DEBUG_HOST)
		{
			printf("\nRX(HYUNDAI) : ");
			for(i = 0; i < (uint16_t)pRx->buf[HD_F_LEN]; i++)
			{
				printf("%02X ", (uint16_t)pRx->buf[i]);
			}
			printf("\n");
		}
	}
	else if(pRx->buf[HD_F_LOC] == HD_LOC_CIRCUIT_GROUP_ALL)
	{
		if(G_Debug == DEBUG_HOST)
		{
			printf("\nRX(HYUNDAI) : ");
			for(i = 0; i < (uint16_t)pRx->buf[HD_F_LEN]; i++)
			{
				printf("%02X ", (uint16_t)pRx->buf[i]);
			}
			printf("\n");
		}		
	}

    if(Loc_MSB == Get_485_ID() || pRx->buf[HD_F_LOC] == HD_LOC_CIRCUIT_GROUP_ALL)
    {
		Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
        switch(pRx->buf[HD_F_DEV])
        {
            case HD_DEV_USS:
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
				{
					if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
					{
						Data_Cnt = (uint8_t)((pRx->buf[HD_F_LEN] - 10) / 2);
						for(i = 0; i < Data_Cnt; i++)
						{
							if(pRx->buf[HD_F_ARG_1 + cnt] == HD_DEV_GAS)
							{
								if(pRx->buf[HD_F_ARG_1 + cnt + 1] == HD_CMD_CLOSE)		BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
								else if(pRx->buf[HD_F_ARG_1 + cnt + 1] == HD_CMD_OPEN)
								{
									if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) != LED_FLASHING)	//���� ��û���� �ƴ� ���� ��������
									{
										BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
									}
								}
								break;
							}
							cnt++;
							cnt++;
						}
						HYUNDAI_ACK(pRx);
					}
					else if(pRx->buf[HD_F_SRV] == HD_SRV_OPEN_n_CLOSE)
					{
						if(item2tsn(mapping_ITEM_GAS))	//���� ��� �ִ� �ϰ� ����ġ�� ����
						{
							if(pRx->buf[HD_F_ARG_1] == HD_DEV_GAS)
							{
								if(pRx->buf[HD_F_ARG_2] == HD_CMD_CLOSE)				BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
								else if(pRx->buf[HD_F_ARG_2] == HD_CMD_OPEN)			BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
							}
							HYUNDAI_ACK(pRx);
						}
					}
				}
				else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
				{
					if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
					{
						if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
						{
							if(pRx->buf[HD_F_ARG_1] == HD_DEV_USS)		//�ϰ� ���� ����
							{
								if(pRx->buf[HD_F_CMD] == HD_CMD_BLOCK)
								{
									BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
									if(item2tsn(mapping_ITEM_GAS))	BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
								}
								else if(pRx->buf[HD_F_CMD] == HD_CMD_UNBLOCK)
								{
									BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);
									if(item2tsn(mapping_ITEM_GAS))	BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
								}
								HYUNDAI_ACK(pRx);
							}
						}
						else
						{
							if(pRx->buf[HD_F_ARG_1] == HD_DEV_LIGHT)	//�ϰ� �ҵ� ����
							{
								if(pRx->buf[HD_F_CMD] == HD_CMD_BLOCK)			BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
								else if(pRx->buf[HD_F_CMD] == HD_CMD_UNBLOCK)	BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);
								HYUNDAI_ACK(pRx);
							}
						}
					}
					else if(pRx->buf[HD_F_SRV] == HD_SRV_OPEN_n_CLOSE)
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_GAS)	//�ϰ� ���� ����
						{
							if(item2tsn(mapping_ITEM_GAS))
							{
								if(pRx->buf[HD_F_CMD] == HD_CMD_CLOSE)			BATCH_BLOCK_Control(SET__GAS_CLOSE_STATE);
								else if(pRx->buf[HD_F_CMD] == HD_CMD_OPEN)		BATCH_BLOCK_Control(SET__GAS_OPEN_STATE);
								HYUNDAI_ACK(pRx);
							}
						}
					}
				}
				break;
			case HD_DEV_ELEVATOR_CALL:
				if(item2tsn(mapping_ITEM_ELEVATOR))
				{
					if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
					{
						if(pRx->buf[HD_F_SRV] == HD_SRV_ELEVATOR_UP_n_DOWN)
						{
							if(Loc_LSB == HD_LOC_CIRCUIT_ALL)	//���������� ���� ��û(Polling)
							{
								if(((pRx->buf[HD_F_ARG_1] & 0x0F) == HD_ARG_CALL_UP) || ((pRx->buf[HD_F_ARG_1] & 0x0F) == HD_ARG_CALL_DOWN))	//���������� ȣ��
								{
									BATCH_BLOCK_Control(SET__ELEVATOR_CALL);
								}
								else if(pRx->buf[HD_F_ARG_1] == HD_ARG_ARRIVE)		//���������� ����
								{
									BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);
								}
								else if(pRx->buf[HD_F_ARG_1] == HD_ARG_ERR)			//���������� ȣ�� ����
								{
									BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);
								}
								HYUNDAI_ACK(pRx);
							}
							else								//����������  ���� ��û(Query)
							{
								HYUNDAI_ACK(pRx);
							}
						}
					}
					else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
					{
						if(pRx->buf[HD_F_SRV] == HD_SRV_ELEVATOR_UP_n_DOWN)
						{
							if(((pRx->buf[HD_F_ARG_1] & 0x0F) == HD_ARG_CALL_UP) || ((pRx->buf[HD_F_ARG_1] & 0x0F) == HD_ARG_CALL_DOWN))
							{
								BATCH_BLOCK_Control(SET__ELEVATOR_CALL);
							}
							HYUNDAI_ACK(pRx);
						}
					}
				}
				break;
			case HD_DEV_EXTERNAL_CONTACT:
				if(pG_Config->Enable_Flag.ThreeWay)
				{
					if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
					{
						if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
						{
							if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
							{
								External_ACK(pRx);
							}
						}
					}
				}
				break;
       	}
    }
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))     //485��� �׽�Ʈ�� ���ؼ� �߰��� 
	{
		if(pRx->buf[2] != pG_Config->RS485_ID)			//���α׷����� ������ ����ġ ID�� ����ġ�� ID�� �ٸ���
		{
			pG_Config->RS485_ID = pRx->buf[2];			//����ġ ID ����.
			Store_CurrentConfig();
			printf("Switch ID Change\r\n");
		}
		RS_485_ID_RES();
	}		
}
#endif

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
			if(GET_LED_State(item2tsn(mapping_ITEM_ELEVATOR)) == LED_FLASHING)
			{
				BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);
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
			if(touch_switch)
			{
				if(GET_Switch_State(touch_switch))		//���� �������϶�
				{
					Beep(ON);
				}
				else													//���� �������� �ƴҶ�
				{
					Gu16_GAS_Off_Tmr	= 270;						// 60�� ��� �� LED �ҵ�(�� ���е忡�� close/open �����Ͱ� ���ŵǸ� �ش� ���·� ��ȯ), 
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

//�ϰ��ҵ� ����ġ�� ������ �� ���� -> ��Ʈ�p ����ġ�� ���� �״� 
//-> �̺�Ʈ ������ �ϰ� ���¸� �޴´� -> �ϰ��ҵ� ����ġ ���°� ���������� ���Ѵ� 
//-> ������ ���⶧����->  �����̿� ����� ���� ��� ������.

/*int SET_Address(int argc, char *argv[])
{
	uint8_t addr;
	
	if(argc == 2)
	{
		addr	= (uint8_t)atoh(argv[1]);
		if(addr <= MAX_ADDRESS)
		{
			pG_Config->RS485_ID = (uint8_t)((addr<<4)&0xF0);
			Store_CurrentConfig();
			printf("SET     ");
		}
		else
		{
			printf("ERR : addr(0x01~0x%02x)", MAX_ADDRESS);
			return 1;
		}
	}
	else
	{
		printf("Current ");
	}
	printf("RS485_ID %d(0x%04x)\n",	(uint16_t)Get_485_ID(), (uint16_t)pG_Config->RS485_ID);		// HYUNDAI
	
	return 1;
}*/

#endif
// ----------------------------------------------------------------------------------------
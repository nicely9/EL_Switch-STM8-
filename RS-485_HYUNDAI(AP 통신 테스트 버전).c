/************************************************************************************
	Project		: 전자식스위치
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
uint8_t HYUNDAI_ALL_LIGHT_ITEM_Sequence[MAX_HYUNDAI_DATA_SEQUENCE];			//현대 프로토콜은 일반 전등과 디밍 전등이 나뉘어져 있어서 일괄 소등 및 해제 시 전등 복구 위해 사용함.
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
void SET_HYUNDAI_DIMMING_LIGHT_ITEM_Sequence(uint8_t item, uint8_t count)		//디밍 전등만 따로 상태 값 사용할 부분이 있어서 추가함.
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
// HYUNDAI	어드레스 범위 1 ~ 15(어드레스 0도 포함시킴, 0으로 설정되면 응답X)

uint8_t Get_485_ID(void)
{
	return	(uint8_t)pG_Config->RS485_ID;		// 어드레스
}
void Protocol_Data_Init(void)
{
    uint8_t	count, i;

	memset((void*)&RxBuf,		0,	sizeof(HYUNDAI_BUF));
	memset((void*)&TxBuf,		0,	sizeof(HYUNDAI_BUF));
	
	Gu16_Elevator_Tmr			= 0;
	Gu16_GAS_Off_Tmr			= 0;
	Gu8_Batch_Light_Flag		= 0;
	memset(HYUNDAI_LIGHT_ITEM_Sequence, 0, MAX_HYUNDAI_DATA_SEQUENCE);	// 8개 항목 클리어
	memset(HYUNDAI_ELEC_ITEM_Sequence, 0, MAX_HYUNDAI_DATA_SEQUENCE);	// 8개 항목 클리어
	memset(HYUNDAI_DIMMING_LIGHT_ITEM_Sequence, 0, MAX_HYUNDAI_DATA_SEQUENCE);
	memset(HYUNDAI_ALL_LIGHT_ITEM_Sequence, 0, MAX_HYUNDAI_DATA_SEQUENCE);
	// 프로토콜 데이터 항목 순서
	// 전등모델	 최대항목	: 전등 6개, 전등 4개 + 디밍 2개
	// 전등+대기 최대항목	: 전등 4개 + 대기 2개, 전등2개 + 디밍2개 + 대기2개
	// ex) 조명 3개, 디밍 2개 = 조명1,조명2,조명3,디밍1,디밍2,0,0,0
	// ex) 조명 1개, 디밍 1개 = 조명1,디밍1,0,0,0,0,0,0
	
	count = 1;
	if(item2tsn(mapping_ITEM_LIGHT_1))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_1, count++);
	if(item2tsn(mapping_ITEM_LIGHT_2))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_2, count++);
	if(item2tsn(mapping_ITEM_LIGHT_3))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_3, count++);
	if(item2tsn(mapping_ITEM_LIGHT_4))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_4, count++);
	if(item2tsn(mapping_ITEM_LIGHT_5))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_5, count++);
	if(item2tsn(mapping_ITEM_LIGHT_6))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_LIGHT_6, count++);
	if(item2tsn(mapping_ITEM_3WAY_1))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_1, count++);
	if(item2tsn(mapping_ITEM_3WAY_2))			SET_HYUNDAI_LIGHT_ITEM_Sequence(mapping_ITEM_3WAY_2, count++);
#ifdef	_PROTOCOL_LIGHT_FIRST_and_DIMMING_		// 프로토콜 순서 조명, 디밍전등 순으로
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
#ifdef	_PROTOCOL_DIMMING_FIRST_and_LIGHT_		// 프로토콜 순서 디밍전등, 조명 순으로
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
    if(tmr == 0)		// 마지막 데이터 수신 후 X ms 초과하면 데이터 클리어
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

void HD_Group_Control(HYUNDAI_BUF *pRx, HYUNDAI_BUF *pTx)
{
	if(pRx->buf[HD_F_LOC] == HD_LOC_CIRCUIT_GROUP_ALL)      //전체 그룹 제어일때
	{
		if(pG_Config->RS485_ID == 0x01)     //1번 기기면
		{
			//제어 후 0x10 으로 응답
			pTx->buf[HD_F_LOC] = 0x10;
		}
		else
		{
			//제어만
			pTx->buf[0] = 0;
			pTx->count = 0;
            TxBuf.send_flag	= 0;
			// break;
		}
	}
	else if(pRx->buf[HD_F_LOC] != HD_LOC_CIRCUIT_GROUP_ALL)     //전체그룹 제어가 아니면, 제어 후 응답
	{
		pTx->buf[HD_F_LOC] = pRx->buf[HD_F_LOC];
		//제어 후 각 기기에 맞게 응답
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
uint8_t NIS_Crc(HYUNDAI_BUF *pTRx, uint8_t cal, uint8_t sel)		//생산시 485통신 테스트를 위해서 추가함.
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
	pTx->buf[pTx->count++]	= HD_ARG_EXTERNAL_CONTACT;	//ARG1(aDEVID), 외부접점제어
	pTx->buf[pTx->count++]	= pRx->buf[HD_F_ARG_2];		//ARG2(aLOC), 1번 그룹의 1번 외부 접점
	if(Gu8_External_Flag)								//ThreeWay() 실행 시 플래그 1로 설정됨. 접점 눌렸을 경우 0x01로 데이터 전송 하고 이후는 0x02로 전송함.
	{
		pTx->buf[pTx->count++]	= HD_CMD_ON;							//ARG3(aCMD), 0x01(ON), 0x02(OFF) OR oxo3 ~ 0xFF(DATA)
		Gu8_External_Flag = 0;
	}
	else
	{
		pTx->buf[pTx->count++]	= HD_CMD_OFF;
	}
	pTx->buf[pTx->count++]	= HYUNDAI_Crc(pTx, pTx->count);	//외부접점 프로토콜은 프레임 길이가 고정이므로 pTx->count-2 하지 않고 사용함.
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
	pTx->buf[pTx->count++]	= pG_Config->Protocol_Type;		//프로토콜 타입
	pTx->buf[pTx->count++]	= Get_485_ID();					//ID 번호
	pTx->buf[pTx->count++]	= 0;

	pTx->buf[pTx->count++]	= Elec_Watt_MSB;				//전열1, 2의 전력 합(정수)
	pTx->buf[pTx->count++]	= Elec_Watt_LSB;				//전열1, 2의 전력 합(소수)

	pTx->buf[pTx->count++]	= Gu8_ZeroCrossing_Err_Flag;	//제로크로싱 동작 상태 1이면 Err, 0이면 Pass

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

        if(pRx->count == cnt)       //cnt는 LED(데이터 길이) 즉, pRx->count가 데이터 길이와 값이 같아지면
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
			if(pG_State->Dimming_Level.Dimming1	!= (uint8_t)Dimming_Level)	Beep(ON);				// 월패드에서 제어했을때 부저음으로 제어 됐는지 구분이 쉽도록
			pG_State->Dimming_Level.Dimming1		= (uint8_t)Dimming_Level;
			if((uint8_t)pG_State->Dimming_Level.Dimming1 > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Dimming_Level.Dimming1	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
			}
			break;
		case mapping_ITEM_DIMMING_LIGHT_2:			
			if(pG_State->Dimming_Level.Dimming2	!= (uint8_t)Dimming_Level)	Beep(ON);				// 월패드에서 제어했을때 부저음으로 제어 됐는지 구분이 쉽도록
			pG_State->Dimming_Level.Dimming2		= (uint8_t)Dimming_Level;
			if((uint8_t)pG_State->Dimming_Level.Dimming2 > (uint8_t)pG_Config->Dimming_MAX_Level)	// 설정한 레벨이 최대 크기를 넘으면
			{
				pG_State->Dimming_Level.Dimming2	= (uint8_t)pG_Config->Dimming_MAX_Level;		// 최대 크기로 설정
			}			
			break;
	}
}

uint8_t HYUNDAI_Batch_Light_State(uint8_t item)		//일괄 소등 전 현재 전등 상태 저장.
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
			if(control_value == HYUNDAI_OFF_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)))		EventCtrl(item2tsn(item), OFF);						//전등 OFF
			}
			else if(control_value == HYUNDAI_ON_FLAG)
			{
				if(GET_Switch_State(item2tsn(item)) == 0)	EventCtrl(item2tsn(item), ON);						//전등 ON
			}
			break;
		default:
			Gu8_PowerSaving_Tmr			= tmr;	// 복구
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
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)	
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
						PUT_RelayCtrl(item2ctrl(item), ON);	// 항목기준 제어
						SET_SWITCH_Delay_OFF_Flag(item, 0);
						SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
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
						PUT_RelayCtrl(item2ctrl(item), OFF);	// 항목기준 제어
						SET_SWITCH_Delay_OFF_Flag(item, 0);
						SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
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
		case mapping_ITEM_LIGHT_1:				// 전등 1 (릴레이 제어)
		case mapping_ITEM_LIGHT_2:				// 전등 2 (릴레이 제어)
		case mapping_ITEM_LIGHT_3:				// 전등 3 (릴레이 제어)
		case mapping_ITEM_LIGHT_4:				// 전등 4 (릴레이 제어)
		case mapping_ITEM_LIGHT_5:				// 전등 5 (릴레이 제어)
		case mapping_ITEM_LIGHT_6:				// 전등 6 (릴레이 제어)
		case mapping_ITEM_3WAY_1:
		case mapping_ITEM_3WAY_2:
			if(ack == HYUNDAI_ON_n_OFF)
			{
				if(GET_Switch_State(item2tsn(item)))			ret = HYUNDAI_ON_FLAG;
				else											ret = HYUNDAI_OFF_FLAG;
			}
			else												ret = HYUNDAI_OFF_FLAG;
			break;
		case mapping_ITEM_DIMMING_LIGHT_1:		// 디밍1 스위치(PWM 제어)
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
		case mapping_ITEM_DIMMING_LIGHT_2:		// 디밍2 스위치(PWM 제어)
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

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC 상위 니블
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC 하위 니블

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
		pTx->count++;		//CRC는 pTx->count 체크 후 LEN 값에 데이터  길이를 재설정하고 계산해야하므로 카운트만++함.
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

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC 상위 니블
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC 하위 니블

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
		if(Loc_LSB > pG_Config->LightCount) return;	//LOC의 값이 전등 수를 넘으면 리턴
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
	if(pRx->buf[HD_F_DEV] == HD_DEV_USS && pRx->buf[HD_F_TYPE] == HD_TYPE_RES_SUCCESS)	//일괄 소등에 대한 복구가 필요하면 적용.
	{
		Data_Cnt = (uint8_t)((pRx->buf[HD_F_LEN] - 10) / 2);
		j = HD_F_ARG_1;
		for(i = 0; i < Data_Cnt; i++)
		{
			if(pRx->buf[j] == HD_DEV_LIGHT)
			{
				j++;
				if(pRx->buf[j] == HYUNDAI_ON_FLAG)			//일괄 소등 상태
				{
					if(Gu8_Batch_Light_Flag != 0x01)
					{
						for(k = 1; k < MAX_HYUNDAI_DATA_SEQUENCE; k++)
						{
							Store_Light_State[k] = HYUNDAI_Batch_Light_State(HYUNDAI_ALL_LIGHT_ITEM_Sequence[k]);	//상태만 저장. 제어는 SRV : 0x40 으로 전등 전체 제어함.
						}
						Gu8_Batch_Light_Flag = 0x01;
						printf("BATCH LIGHT OFF\r\n");
					}
				}
				else if(pRx->buf[j] == HYUNDAI_OFF_FLAG)	//일괄 소등 해제 상태
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

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC 상위 니블
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC 하위 니블

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
			if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)	//SRV 0x40은 ON/OFF로 응답
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
			else if(pRx->buf[HD_F_SRV] == HD_SRV_DIMMING)	//SRV 0x42는 디밍 레벨로 응답.
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
				else if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)	//전체 제어에 대한 응답은 타입 데이터만 바꿔서 응답.
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
	pTx->count++;		//CRC는 pTx->count 체크 후 LEN 값에 데이터  길이를 재설정하고 계산해야하므로 카운트만++함.
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

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC 상위 니블
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC 하위 니블

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
		if(Loc_LSB > pG_Config->LightCount + pG_Config->DimmingCount) return;	//LOC의 값이 전등 수를 넘으면 리턴
	}
	if(pRx->buf[HD_F_DEV] == HD_DEV_ELETRICITY || pRx->buf[HD_F_DEV] == HD_DEV_STANDBY_POWER)
	{
		if(Loc_LSB > pG_Config->ElectricityCount) return;	//LOC의 값이 전등 수를 넘으면 리턴
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
								HYUNDAI_Control(pRx, i);	//HYUNDAI_Control 함수에서 SRV 파악하여 ON/OFF 제어 하거나, 디밍 레벨 제어함.
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
										PUT_RelayCtrl(item2ctrl(item), ON);	// 항목기준 제어
										SET_SWITCH_Delay_OFF_Flag(item, 0);
										SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
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
										PUT_RelayCtrl(item2ctrl(item), OFF);	// 항목기준 제어
										SET_SWITCH_Delay_OFF_Flag(item, 0);
										SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);	// 전열을 하나라도 켰으면 전제 지연소등 플래그는 클리어
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

	if(pRx->buf[HD_F_DEV] == HD_DEV_USS && pRx->buf[HD_F_TYPE] == HD_TYPE_RES_SUCCESS)	//일괄 소등에 대한 복구가 필요하면 적용.
	{
		Data_Cnt = (uint8_t)((pRx->buf[HD_F_LEN] - 10) / 2);
		j = HD_F_ARG_1;
		for(i = 0; i < Data_Cnt; i++)
		{
			if(pRx->buf[j] == HD_DEV_LIGHT)
			{
				j++;
				if(pRx->buf[j] == HYUNDAI_ON_FLAG)			//일괄 소등 상태
				{
					if(Gu8_Batch_Light_Flag != 0x01)
					{
						for(k = 1; k < MAX_HYUNDAI_DATA_SEQUENCE; k++)
						{
							Store_Light_State[k] = HYUNDAI_Batch_Light_State(HYUNDAI_ALL_LIGHT_ITEM_Sequence[k]);	//상태만 저장. 제어는 SRV : 0x40 으로 전등 전체 제어함.
						}
						Gu8_Batch_Light_Flag = 0x01;
						printf("BATCH LIGHT OFF\r\n");
					}
				}
				else if(pRx->buf[j] == HYUNDAI_OFF_FLAG)	//일괄 소등 해제 상태
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

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC 상위 니블
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC 하위 니블

    switch(pRx->buf[HD_F_DEV])
    {
		case HD_DEV_USS:
			if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
			{
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ)
				{
					if(Loc_LSB == HD_LOC_CIRCUIT_ALL)
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_USS)	//일괄차단 상태요청
						{
							if(item2tsn(mapping_ITEM_GAS))		//가스 차단 기능이 있을 때,
							{
								if((GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == 0) && (GET_Switch_State(item2tsn(mapping_ITEM_GAS))))		pTx->buf[HD_F_ARG_2] = HD_CMD_BLOCK;
								else              																											pTx->buf[HD_F_ARG_2] = HD_CMD_UNBLOCK;     //일괄, 가스 둘중 하나라도 차단 해제면 UNBLOCK(0x02)									
							}
							else								//가스 차단 기능이 없을 때,
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == 0)		pTx->buf[HD_F_ARG_2] = HD_CMD_BLOCK;
								else																	pTx->buf[HD_F_ARG_2] = HD_CMD_UNBLOCK;
							}
						}
						else									//일괄 차단 전체 상태요청
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
											if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)	pTx->buf[pTx->count++] = HD_CMD_CLOSE;		//가스 차단 요청이면 닫힘으로 응답
											else															pTx->buf[pTx->count++] = HD_CMD_OPEN;		//가스 차단해제중이면 열림으로 응답
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
					else											//개별 회로 제어
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_LIGHT)	//일괄 소등 상태 요청
						{
							pTx->buf[pTx->count++] = HD_DEV_LIGHT;
							if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == OFF)			pTx->buf[pTx->count++] = HD_CMD_BLOCK;
							else																		pTx->buf[pTx->count++] = HD_CMD_UNBLOCK;							
						}
					}					
				}
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
				{
					if(Loc_LSB == HD_LOC_CIRCUIT_ALL)				//전체 회로 제어
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_USS)		//일괄 차단 제어
						{
							pTx->buf[pTx->count++] = HD_DEV_USS;
							if(item2tsn(mapping_ITEM_GAS))			//가스 차단 기능 있을 때,
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == OFF && GET_Switch_State(item2tsn(mapping_ITEM_GAS)))
								{									//일괄, 가스 둘 다 차단 상태 일 때,
									pTx->buf[pTx->count++] = HD_CMD_BLOCK;
								}
								else								//일괄, 가스 둘 중 하나라도 차단이 아니면
								{
									pTx->buf[pTx->count++] = HD_CMD_UNBLOCK;
								}
							}
							else	//가스 차단 기능 없을 때
							{
								if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == OFF)		pTx->buf[pTx->count++] = HD_CMD_BLOCK;		//일괄소등이면 차단으로 응답
								else																	pTx->buf[pTx->count++] = HD_CMD_UNBLOCK;	//일괄소등해제면 차단해제로 응답
							}
						}
					}
					else	//개별 회로 제어
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_LIGHT)	//일괄 소등 제어
						{
							pTx->buf[pTx->count++] = HD_DEV_LIGHT;
							if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == OFF)		pTx->buf[pTx->count++] = HD_CMD_BLOCK;			//일괄소등이면 차단으로 응답
							else																	pTx->buf[pTx->count++] = HD_CMD_UNBLOCK;		//일괄소등해제면 차단해제로 응답
						}
					}
				}
			}
			else if(pRx->buf[HD_F_SRV] == HD_SRV_OPEN_n_CLOSE)		//일괄가스 상태요청
			{
				if(pRx->buf[HD_F_TYPE] == HD_TYPE_STATE_REQ || pRx->buf[HD_F_TYPE] == HD_TYPE_CONTROL_REQ)
				{
					if(pRx->buf[HD_F_ARG_1] == HD_DEV_GAS)
					{
						pTx->buf[pTx->count++] = HD_DEV_GAS;
						if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))	pTx->buf[pTx->count++] = HD_CMD_CLOSE;		//가스 차단중이면 닫힘으로 응답
						else
						{
							if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)	pTx->buf[pTx->count++] = HD_CMD_CLOSE;		//가스 차단 요청이면 닫힘으로 응답
							else															pTx->buf[pTx->count++] = HD_CMD_OPEN;		//가스 차단해제중이면 열림으로 응답
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
						if(Loc_LSB == HD_LOC_CIRCUIT_ALL)		//엘리베이터 상태 요청(Polling)
						{
							if(GET_Switch_State(touch_switch))	//호출 상태면 월패드에서 온 데이터 그대로 전송.
							{
								pTx->buf[pTx->count++] = pRx->buf[HD_F_ARG_1];
							}
							else													//호출 요청 상태면 하향호출. 호출 상태가 아니면 평상시 데이터 전송.
							{
								if(GET_LED_State(touch_switch) == LED_FLASHING)					pTx->buf[pTx->count++] = HD_ARG_CALL_DOWN;
								else															pTx->buf[pTx->count++] = HD_ARG_NORMAL;
							}	
						}
						else									//엘리베이터 상태 요청(Query)
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
	pTx->count++;		//CRC는 pTx->count 체크 후 LEN 값에 데이터  길이를 재설정하고 계산해야하므로 카운트만++함.
	pTx->buf[pTx->count++] = HYUNDAI_ETX;
	pTx->buf[HD_F_LEN] = (uint8_t)pTx->count;
	pTx->buf[pTx->count-2] = HYUNDAI_Crc(pTx, pTx->count-2);
	TxBuf.send_flag	= 1;
	HD_Group_Control(pRx, pTx);
}

void HYUNDAI_Data_Process(HYUNDAI_BUF	*pRx)
{
    uint8_t     Loc_MSB, Loc_LSB, Data_Cnt, i, cnt = 0;

    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC 상위 니블
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC 하위 니블

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
									if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) != LED_FLASHING)	//가스 요청중이 아닐 때만 열림으로
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
						if(item2tsn(mapping_ITEM_GAS))	//가스 기능 있는 일괄 스위치만 응답
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
							if(pRx->buf[HD_F_ARG_1] == HD_DEV_USS)		//일괄 차단 제어
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
							if(pRx->buf[HD_F_ARG_1] == HD_DEV_LIGHT)	//일괄 소등 제어
							{
								if(pRx->buf[HD_F_CMD] == HD_CMD_BLOCK)			BATCH_BLOCK_Control(SET__BATCHLIGHT_OFF);
								else if(pRx->buf[HD_F_CMD] == HD_CMD_UNBLOCK)	BATCH_BLOCK_Control(SET__BATCHLIGHT_ON);
								HYUNDAI_ACK(pRx);
							}
						}
					}
					else if(pRx->buf[HD_F_SRV] == HD_SRV_OPEN_n_CLOSE)
					{
						if(pRx->buf[HD_F_ARG_1] == HD_DEV_GAS)	//일괄 가스 제어
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
							if(Loc_LSB == HD_LOC_CIRCUIT_ALL)	//엘리베이터 상태 요청(Polling)
							{
								if(((pRx->buf[HD_F_ARG_1] & 0x0F) == HD_ARG_CALL_UP) || ((pRx->buf[HD_F_ARG_1] & 0x0F) == HD_ARG_CALL_DOWN))	//엘리베이터 호출
								{
									BATCH_BLOCK_Control(SET__ELEVATOR_CALL);
								}
								else if(pRx->buf[HD_F_ARG_1] == HD_ARG_ARRIVE)		//엘리베이터 도착
								{
									BATCH_BLOCK_Control(SET__ELEVATOR_ARRIVE);
								}
								else if(pRx->buf[HD_F_ARG_1] == HD_ARG_ERR)			//엘리베이터 호출 에러
								{
									BATCH_BLOCK_Control(SET__ELEVATOR_CALL_FAIL);
								}
								HYUNDAI_ACK(pRx);
							}
							else								//엘리베이터  상태 요청(Query)
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
	if((pRx->buf[0] == NIS_LIGHT_ID_COMM_1) && (pRx->buf[1] == NIS_LIGHT_ID_COMM_2))     //485통신 테스트를 위해서 추가함 
	{
		if(pRx->buf[2] != pG_Config->RS485_ID)			//프로그램에서 설정한 스위치 ID와 스위치의 ID가 다르면
		{
			pG_Config->RS485_ID = pRx->buf[2];			//스위치 ID 설정.
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
			if(GET_LED_State(item2tsn(mapping_ITEM_GAS)) == LED_FLASHING)		// 프로토콜에 대해 설정이 없으면 OPEN 상태로 전환
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
				if(GET_Switch_State(touch_switch))		//가스 차단중일때
				{
					Beep(ON);
				}
				else													//가스 차단중이 아닐때
				{
					Gu16_GAS_Off_Tmr	= 270;						// 60초 경과 후 LED 소등(단 월패드에서 close/open 데이터가 수신되면 해당 상태로 전환), 
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
				if(GET_Switch_State(touch_switch) == 0)		//차단  상태 아닐 때
				{
					SET_Switch_State(touch_switch, ON);		// 가스밸브 차단(닫힘)
					SET_LED_State(touch_switch, OFF);		// 실제로는 LED 켜짐
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
					SET_Switch_State(touch_switch, OFF);	// 가스밸브 열림
					SET_LED_State(touch_switch, ON);		// 실제로는 LED 꺼짐
					Beep(ON);
				}
			}
			// printf("GAS OPEN\r\n");
			break;
		
		case SET__ELEVATOR_REQUEST:																			//스위치에서 엘리베이터 콜
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(touch_switch)
			{
				if(GET_Switch_State(touch_switch) == OFF || (GET_LED_State(touch_switch) == LED_FLASHING))		//스위치 OFF 거나 LED 상태 Flashing이면
				{
					Gu16_Elevator_Tmr = 60;																		//요청 타이머 60초 초기화. 0이되면 요청 취소되고 LED OFF됨.
					SET_Switch_State(touch_switch, OFF);															//스위치 OFF
					SET_LED_State(touch_switch, LED_FLASHING);													//LED Flashing
					Beep(ON);
					// printf("ELEVATOR REQEUST\r\n");
				}
			}
			break;
		case SET__ELEVATOR_CALL:																			//세대기에서 엘리베이터 콜
			touch_switch = item2tsn((uint8_t)mapping_ITEM_ELEVATOR);
			if(touch_switch)
			{
				if(GET_LED_State(touch_switch) == LED_FLASHING || GET_Switch_State(touch_switch) == 0)			//호출 요청 상태거나, 호출상태 아닐 때.
				{
					Gu16_Elevator_Tmr = 60;																			//콜 상태가 되면 타이머 60초 초기화. 타이머가 0되면 상태 원래대로 돌아감.
					SET_Switch_State(touch_switch, ON);																//스위치 ON
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
			if(touch_switch)
			{
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
			}
			break;
	}
}

//일괄소등 스위치로 전등을 다 껐다 -> 네트웤 스위치로 전등 켰다 
//-> 이벤트 샌더로 일괄 상태를 받는다 -> 일괄소등 스위치 상태가 차단해제로 변한다 
//-> 응답이 없기때문에->  릴레이에 연결된 전등 모두 켜진다.

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
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

HYUNDAI_BUF	HYUNDAI_RxBuf, HYUNDAI_TxBuf;
// __IO uint8_t	Gu8_RS_485_Rx_Tmr	= 0;
// __IO uint8_t	Gu8_RS_485_Tx_Tmr	= 0;/************************************************************************************
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

#ifdef HYUNDAI
HYUNDAI_BUF	HYUNDAI_RxBuf, HYUNDAI_TxBuf;
__IO uint8_t Gu8_RS_485_Rx_Tmr = 0;
__IO uint8_t Gu8_RS_485_Tx_Tmr = 0;
uint8_t			Gu8_RS485_TX_Enable	= 0;

// uint8_t test_485_tmr = 0;
void HYUNDAI_Process(uint8_t data);

//-------------------------------------------------------------------------------------------------------------------------
void irq_RS485_TX(void)
{
	uint8_t data;
	
	if(USART_GetITStatus(RS_485_USART, USART_IT_TC) != RESET)
	{
		if(Uart_GetTxQ(RS_485_PORT, &data) != 0)
		{
			USART_SendData8(RS_485_USART, data);
			USART_ClearITPendingBit(RS_485_USART, USART_IT_TC);
		}
		else
		{
			//Gu16_Check_Tmr	= 0;
			Gu8_RS485_TX_Enable	= 0;
			// Gu8_HD_RS485_TX_Enable = 0;
			GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);			// Receiver Active Low
			USART_ITConfig(RS_485_USART, USART_IT_TC, DISABLE);		// Disable the USART Transmit Complete interrupt
			//USART_ClearITPendingBit(RS_485_USART, USART_IT_TC);	// ???
		}
	}
}

void irq_RS485_RX(void)
{
	uint8_t data;
	
	if(USART_GetITStatus(RS_485_USART, USART_IT_RXNE) != RESET)
	{
		Gu8_RS_485_Rx_Tmr	= pG_Config->Protocol_IntervalTime;		// 6ms
		// Gu8_HD_RS_485_Rx_Tmr = pG_Config->Protocol_IntervalTime;
		data = USART_ReceiveData8(RS_485_USART);
		Uart_PutRxQ(RS_485_PORT, data);								// Read one byte from the receive data register
	}
}
//-------------------------------------------------------------------------------------------------------------------------
void RS485_Init(void)
{
	CLK_PeripheralClockConfig((CLK_Peripheral_TypeDef)RS_485_PORT_CLK, ENABLE);	// Enable RS_485 USART clock
	
	SYSCFG_REMAPPinConfig(REMAP_Pin_USART3TxRxPortF, DISABLE);		// USART3_TX mapped on PG1 and USART3_RX mapped on PG0
	
	GPIO_ExternalPullUpConfig(RS_485_TX_PORT, RS_485_TX_PIN, ENABLE);			// RS_485 Tx as alternate function push-pull
	GPIO_ExternalPullUpConfig(RS_485_RX_PORT, RS_485_RX_PIN, ENABLE);			// RS_485 Rx as alternate function push-pull
	USART_Init(RS_485_USART, 9600, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
	//-------------------------------------------------------------------------------------------------------------------------
	GPIO_Init(RS_485_DE_PORT, RS_485_DE_PIN, GPIO_Mode_Out_PP_Low_Fast);		// Output
	GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);		// Low
	//-------------------------------------------------------------------------------------------------------------------------
	USART_ITConfig(RS_485_USART, USART_IT_RXNE, ENABLE);
	USART_Cmd(RS_485_USART, ENABLE);
	//-------------------------------------------------------------------------------------------------------------------------
	printf("RS-485 Init\n");

	memset((void*)&HYUNDAI_RxBuf,		0,	sizeof(HYUNDAI_BUF));
	memset((void*)&HYUNDAI_TxBuf,		0,	sizeof(HYUNDAI_BUF));
}

uint8_t	Gu8_GAS_Off_Tmr				= 0;
uint8_t	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_NON;

void GAS_Off(void)
{
	if(pG_Config->Protocol_Type == CVNET_PROTOCOL)
	{
		//Gu8_RS_485_Tx_Tmr = 0;					// 즉시전송, 차 후 적용시 전송시컨스에 맞춰 전송될 수 있도록 해야 함
		//CVNET_TxBuf.count	= CVNET_MAX_BUF;	// 카운트 값이 있으면 전송
		printf("GAS_Off\n");
	}
}

void ELEVATOR_Call(void)
{
	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_CALL;	// 호출에 대한 응답이 없으면 x초 후 취소플래그 설정해야함
	printf("ELEVATOR_Call\n");
}

void ELEVATOR_Cancel(void)
{
	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_CANCEL;
	printf("ELEVATOR_Cancel\n");
}
// void test_485(void)
// {
//     int cnt = 0;
//     uint8_t sdata[11] = {0xF7, 0x0B, 0x01, 0x19, 0x01, 0x40, 0x10, 0x00, 0x00, 0xB5, 0xEE};
//     uint8_t data;
//     for(cnt = 0; cnt <= 11; cnt++)
//     {
//         data = sdata[cnt];
//         Uart_PutRxQ(RS_485_PORT, data);
//     }
//     if(cnt == 11)
//     {
//         cnt = 0;
//     }
// }

void RS485_Process(void)
{
	uint16_t	i;
	uint8_t		data;
	uint8_t		touch_switch;
    
	//---------------------------------------------------------------------------------------------------------------------------------------
	if(Gu8_RS485_TX_Enable == 0)		// 5000~10000번에 한번식 High로 유지되는 경우가 있어 적용함(많을 경우 1000번에 한번, Low 시키는 루틴은 정상적으로 실행됨)
	{
		if(GPIO_ReadInputDataBit(RS_485_DE_PORT, (GPIO_Pin_TypeDef)RS_485_DE_PIN))	// GPIO Read
		{
			GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);			// Receiver Active Low
		}
	}
    // if(test_485_tmr == 0){
	//     test_485();
    //     test_485_tmr = 5;
    // }
	while(Uart_GetRxQ(RS_485_PORT, &data) != 0)
	{
		// if(G_Debug == DEBUG_HOST_REALDATA)
        if(1)
		{
			if(pG_Config->Protocol_Type == HYUNDAI_PROTOCOL)
			{
				if(data == HYUNDAI_STX)	printf("\n");
			}
			printf("%02X ", (uint16_t)data);
		}
		//------------------------------------------------------------------------------------------
		HYUNDAI_Process(data);
		//------------------------------------------------------------------------------------------
	}
	//------------------------------------------------------------------------------------------
	if(Gu8_RS_485_Rx_Tmr == 0)		// CVNET 인터벌 타임은 5ms 초과 할 수 없다,		마지막 데이터 수신 후 6ms 가 초과하면 데이터 클리어
	{
		HYUNDAI_RxBuf.buf[0]	= 0;
		HYUNDAI_RxBuf.count	    = 0;
	}
	//------------------------------------------------------------------------------------------
	if(pG_Config->Protocol_Type == HYUNDAI_PROTOCOL)
	{
		if(Gu8_RS_485_Tx_Tmr == 0)		// 데이터 수신 후 30ms 이후 데이터 전송
		{
			if(HYUNDAI_TxBuf.count)
			{
				TX_Queue_Clear(RS_485_PORT);
				Uart_PutsTxQ(RS_485_PORT, HYUNDAI_TxBuf.buf, HYUNDAI_TxBuf.count);
				
				// if(G_Debug == DEBUG_HOST)
                if(1)
				{
					printf("\nTX(HYUNDAI) : ");
					for(i=0;i<HYUNDAI_TxBuf.count;i++)
					{
						printf("%02X ", (uint16_t)HYUNDAI_TxBuf.buf[i]);
					}
					printf("\n");
				}
				
				HYUNDAI_TxBuf.count	= 0;
			}
		}
	}
	//------------------------------------------------------------------------------------------
}


// HYUNDAI ----------------------------------------------------------------------------------
/*
	* 30ms 지나고 300ms 이내에 응답
	* Interval time은 5ms
	------------------------------------------------------------------------------------------
	STX | LEN | VEN | DEV | TYPE | SRV | LOC | CMD | ARG1 ~ ARGn | CRC | ETX
	------------------------------------------------------------------------------------------
    STX
    0xF7로 고정

    LEN
    STX ~ ETX까지의 Packet 길이
    
    VEN
    0x01

    DEV
    일반 조명       0x19
    디밍 조명       0x1A

    TYPE
    제어 요청에 대한 응답은 제어 기능 수행 이후 응답한다(단 ,응답시간이 300ms를 초과하여서는 안됨)
    상태요구             0x01       (명령사용 : 현대통신)
    제어명령             0x02       (명령사용 : 현대통신)
    Response Success    0x04       (명령사용 : 기기업체)
    
    SRV
    전원            0x40
    밝기 조절       0x42
    일괄제어        0x70

    LOC
    상위 니블 : 그룹 넘버, 하위 니블 : 그룹 내 기기ID
    상위 니블은 물리적인 조명 스위치를 의미
    하위 니블은 해당 조명 스위치의 회로를 의미
    각 니블은 1 ~ 6 까지 사용
    
    전체 그룹 조명 제어 시 : 상위 니블인 그룹 넘버를 0으로 제어
    해당 그룹 전체 제어 시 : 하위 니블인 기기 ID를 0으로 제어
    * 예시 0x11(1번  스위치의 1번 회로 : 11, 2번 회로 : 12)

	CMD
    ON              0x01
    OFF             0x02
    NUMBER          0 ~ 7

    ARG
    전체 상태 응답 시 해당 구수 별로 ARG 개수 가변
    ARG는 BYTE로 회로 상태 표시.
    ON : 0x01, OFF : 0x02
    1구일 경우 전체 상태 응답시 1BYTE, 6구일 경우 전체 상태 응답 시 6BYTE
    상태 응답 시 현재 상태 표시

    CRC
    STX ~ CRC전까지의 Exclusive OR

    ETX
    0xEE로 고정
*/

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

void HYUNDAI_Data_Process(HYUNDAI_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		item = 0;
    uint8_t     Loc_MSB;
    uint8_t     Loc_LSB;
    uint8_t     arg_cnt, light_cnt;
    uint8_t     crc_cnt = 8;
    uint8_t     etx_cnt = 9;
    uint8_t     max_cnt = 10;

	HYUNDAI_BUF	*pTx;

	pTx = &HYUNDAI_TxBuf;
	pTx->buf[HD_F_STX]	= HYUNDAI_STX;
	pTx->buf[HD_F_LEN]	= pRx->buf[HD_F_LEN];
	pTx->buf[HD_F_VEN]	= (uint8_t)pG_Config->RS485_ID;
    pTx->buf[HD_F_DEV]  = pRx->buf[HD_F_DEV];
    // pTx->buf[HD_F_TYPE]	= HD_RES_SUCCESS;
    pTx->buf[HD_F_SRV]  = pRx->buf[HD_F_SRV];
    pTx->buf[HD_F_LOC]  = pRx->buf[HD_F_LOC];
    pTx->buf[HD_F_CMD]  = pRx->buf[HD_F_CMD];
	pTx->buf[HD_F_ARG_1] = 0;
    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC 상위 니블
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC 하위 니블
	
    switch(pRx->buf[HD_F_TYPE])
    {
        case HD_TYPE_STATE_REQ:
            Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
            switch(pRx->buf[HD_F_DEV])
            {
                case HD_DEV_LIGHT:      //전등 상태 요청
                    if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)      //전원
                    {
                        if(Loc_MSB == SWITCH_NUM)        //1번 스위치
                        {
                            switch(Loc_LSB)
                            {
                                case HD_LOC_DEV_1:
                                case HD_LOC_DEV_2:
                                case HD_LOC_DEV_3:
                                case HD_LOC_DEV_4:
                                case HD_LOC_DEV_5:
                                case HD_LOC_DEV_6:
                                    arg_cnt = HD_F_ARG_1;
                                    item = Loc_LSB;
                                    if(GET_Switch_State(item2tsn(item)))                        pTx->buf[arg_cnt] = HD_CMD_ON;
                                    else                                                        pTx->buf[arg_cnt] = HD_CMD_OFF;
                                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                    pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                    pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                    pTx->count = HD_T_MAX_11;  
                                    break;                                       
                                case HD_LOC_DEV_ALL:
                                    switch(pG_Config->LightCount)
                                    {
                                        case 1:
                                            // arg_cnt = HD_F_ARG_1;
                                            // if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_1)))    pTx->buf[arg_cnt] = HD_CMD_ON;
                                            // else                                                    pTx->buf[arg_cnt] = HD_CMD_OFF;                              
                                            // break;
                                        case 2:
                                            // arg_cnt = HD_F_ARG_1;
                                            // for(item = mapping_ITEM_LIGHT_1; item <= mapping_ITEM_LIGHT_2; item++)
                                            // {
                                            //     if(GET_Switch_State(item2tsn(item)))                pTx->buf[arg_cnt] = HD_CMD_ON;
                                            //     else                                                pTx->buf[arg_cnt] = HD_CMD_OFF;
                                            //     arg_cnt++;
                                            // }                                    
                                            // pTx->buf[HD_F_ARG_2] = HD_CMD_OFF;
                                            // pTx->buf[HD_F_LEN] = HYUNDAI_LEN_0C;
                                            // pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                            // pTx->buf[HD_T_CRC_10]	= HYUNDAI_Crc(pTx, HD_T_CRC_10);
                                            // pTx->buf[HD_T_ETX_11]	= HYUNDAI_ETX;
                                            // pTx->count = HD_T_MAX_12;                                
                                            // break;
                                        case 3:
                                            // arg_cnt = HD_F_ARG_1;
                                            // for(item = mapping_ITEM_LIGHT_1; item <= mapping_ITEM_LIGHT_3; item++)
                                            // {
                                            //     if(GET_Switch_State(item2tsn(item)))                pTx->buf[arg_cnt] = HD_CMD_ON;
                                            //     else                                                pTx->buf[arg_cnt] = HD_CMD_OFF;
                                            //     arg_cnt++;
                                            // }                                                                     
                                            // pTx->buf[HD_F_LEN] = HYUNDAI_LEN_0D;
                                            // pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                            // pTx->buf[HD_T_CRC_11]	= HYUNDAI_Crc(pTx, HD_T_CRC_11);
                                            // pTx->buf[HD_T_ETX_12]	= HYUNDAI_ETX;
                                            // pTx->count = HD_T_MAX_13;                                
                                            // break;
                                        case 4:
                                            // arg_cnt = HD_F_ARG_1;
                                            // for(item = mapping_ITEM_LIGHT_1; item <= mapping_ITEM_LIGHT_4; item++)
                                            // {
                                            //     if(GET_Switch_State(item2tsn(item)))                pTx->buf[arg_cnt] = HD_CMD_ON;
                                            //     else                                                pTx->buf[arg_cnt] = HD_CMD_OFF;
                                            //     arg_cnt++;
                                            // }                                
                                            // pTx->buf[HD_F_LEN] = HYUNDAI_LEN_0E;
                                            // pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                            // pTx->buf[HD_T_CRC_12]	= HYUNDAI_Crc(pTx, HD_T_CRC_12);
                                            // pTx->buf[HD_T_ETX_13]	= HYUNDAI_ETX;
                                            // pTx->count = HD_T_MAX_14;                                
                                            // break;
                                        case 5:
                                            // arg_cnt = HD_F_ARG_1;
                                            // for(item = mapping_ITEM_LIGHT_1; item <= mapping_ITEM_LIGHT_5; item++)
                                            // {
                                            //     if(GET_Switch_State(item2tsn(item)))                pTx->buf[arg_cnt] = HD_CMD_ON;
                                            //     else                                                pTx->buf[arg_cnt] = HD_CMD_OFF;
                                            //     arg_cnt++;
                                            // }                                  
                                            // pTx->buf[HD_F_LEN] = HYUNDAI_LEN_0F;
                                            // pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                            // pTx->buf[HD_T_CRC_13]	= HYUNDAI_Crc(pTx, HD_T_CRC_13);
                                            // pTx->buf[HD_T_ETX_14]	= HYUNDAI_ETX;
                                            // pTx->count = HD_T_MAX_15;                                  
                                            // break;
                                        case 6:
                                            // arg_cnt = HD_F_ARG_1;
                                            // for(item = mapping_ITEM_LIGHT_1; item <= mapping_ITEM_LIGHT_6; item++)
                                            // {
                                            //     if(GET_Switch_State(item2tsn(item)))                pTx->buf[arg_cnt] = HD_CMD_ON;
                                            //     else                                                pTx->buf[arg_cnt] = HD_CMD_OFF;
                                            //     arg_cnt++;
                                            // }                                  
                                            // pTx->buf[HD_F_LEN] = HYUNDAI_LEN_10;
                                            // pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                            // pTx->buf[HD_T_CRC_14]	= HYUNDAI_Crc(pTx, HD_T_CRC_14);
                                            // pTx->buf[HD_T_ETX_15]	= HYUNDAI_ETX;
                                            // pTx->count = HD_T_MAX_16;                                
                                            // break;
                                            arg_cnt = HD_F_ARG_1;
                                            light_cnt = (uint8_t)pG_Config->LightCount;
                                            for(item = mapping_ITEM_LIGHT_1; item <= pG_Config->LightCount; item++)
                                            {
                                                if(GET_Switch_State(item2tsn(item)))                pTx->buf[arg_cnt] = HD_CMD_ON;
                                                else                                                pTx->buf[arg_cnt] = HD_CMD_OFF;
                                                arg_cnt++;
                                            }
                                            crc_cnt += light_cnt;
                                            etx_cnt += light_cnt;
                                            max_cnt += light_cnt;
                                            pTx->buf[HD_F_LEN]  += (uint8_t)(light_cnt - 1);
                                            pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                            pTx->buf[crc_cnt]	= HYUNDAI_Crc(pTx, crc_cnt);
                                            pTx->buf[etx_cnt]	= HYUNDAI_ETX;
                                            pTx->count = max_cnt;
                                            break;                                              
                                    }
                                    break;
                            }
                        }
                    }
                    break;
                case HD_DEV_LIGHT_DIM:
                    if(pRx->buf[HD_F_SRV] == HD_SRV_DIM)
                    {
                        if(Loc_MSB == SWITCH_NUM)
                        {
                            if(Loc_LSB == SWITCH_CIRCUIT_NUM)
                            {
                                if(pRx->buf[HD_F_DEV] == 0x1A)
                                {
                                    if(pG_State->Dimming_Level.Dimming1)    //dim level 1이상이면 ON으로 전송
                                    {
                                        pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                    }
                                    else
                                    {
                                        pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                    }
                                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                    pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                    pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                    pTx->count = HD_T_MAX_11;                            
                                }
                            }
                        }
                    }
                    break;
                case HD_DEV_USS:        //일괄 스위치 프로토콜
                    switch(pRx->buf[HD_F_ARG_1])
                    {
                        case HD_DEV_LIGHT:      //일괄 소등
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                if(Loc_LSB == SWITCH_CIRCUIT_NUM)
                                {
                                    if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))    pTx->buf[HD_F_ARG_2] = HD_CMD_OFF;
                                    else                                                        pTx->buf[HD_F_ARG_2] = HD_CMD_ON;
                                    pTx->buf[HD_F_ARG_1] = pRx->buf[HD_F_ARG_1];
                                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                    pTx->buf[HD_T_CRC_10]	= HYUNDAI_Crc(pTx, HD_T_CRC_10);
                                    pTx->buf[HD_T_ETX_11]	= HYUNDAI_ETX;
                                    pTx->count = HD_T_MAX_12;
                                }
                            }                            
                            break;
                        case HD_DEV_GAS:        //일괄 가스
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                if(Loc_LSB == SWITCH_CIRCUIT_NUM)
                                {
                                    if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))    pTx->buf[HD_F_ARG_2] = HD_CMD_CLOSE;
                                    else                                                pTx->buf[HD_F_ARG_2] = HD_CMD_OPEN;
                                    pTx->buf[HD_F_ARG_1] = pRx->buf[HD_F_ARG_1];
                                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                    pTx->buf[HD_T_CRC_10]	= HYUNDAI_Crc(pTx, HD_T_CRC_10);
                                    pTx->buf[HD_T_ETX_11]	= HYUNDAI_ETX;
                                    pTx->count = HD_T_MAX_12;
                                }
                            }
                            break;
                        case HD_DEV_STANDBY_POWER:
                            break;
                        case HD_DEV_ELETRICITY:
                            break;
                    }
                    break;
            }
            break;

        case HD_TYPE_CONTROL_REQ:
            // Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
            // if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
            // {
            switch(pRx->buf[HD_F_SRV])
            {
                case HD_SRV_POWER:
                    if(Loc_MSB == SWITCH_NUM)
                    {
                        switch(Loc_LSB)
                        {
                            case HD_LOC_DEV_ALL:
                                Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                                if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_ALL), ON);
                                    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                }
                                else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_ALL), OFF);
                                    pRx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                }
                                pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                pTx->count			= HD_T_MAX_11;
                                break;
                            case HD_LOC_DEV_1:
                                Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                                if(pRx->buf[HD_F_DEV == HD_DEV_LIGHT])  //0x19
                                {
                                    if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                    {
                                        EventCtrl(item2tsn(mapping_ITEM_LIGHT_1), ON);
                                        pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                    }
                                    else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                    {
                                        EventCtrl(item2tsn(mapping_ITEM_LIGHT_1), OFF);
                                        pRx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                    }
                                }
                                else if(pRx->buf[HD_F_DEV == HD_DEV_LIGHT_DIM]) //0x1A dimming light on/off
                                {
                                    if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                    {
                                        EventCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_1), ON);
                                        pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                    }
                                    else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                    {
                                        EventCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_1), OFF);
                                        pRx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                    }                                
                                }
                                pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                pTx->count			= HD_T_MAX_11;
                                break;
                            case HD_LOC_DEV_2:
                                Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                                if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_2), ON);
                                    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                }
                                else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_2), OFF);
                                    pRx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                }
                                pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                pTx->count			= HD_T_MAX_11;
                                break;
                            case HD_LOC_DEV_3:
                                Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                                if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_3), ON);
                                    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                }
                                else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_3), OFF);
                                    pRx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                }
                                pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                pTx->count			= HD_T_MAX_11;
                                break;                  
                            case HD_LOC_DEV_4:
                                Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                                if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_4), ON);
                                    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                }
                                else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_4), OFF);
                                    pRx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                }
                                pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                pTx->count			= HD_T_MAX_11;
                                break;                  
                            case HD_LOC_DEV_5:
                                Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                                if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_5), ON);
                                    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                }
                                else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_5), OFF);
                                    pRx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                }
                                pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                pTx->count			= HD_T_MAX_11;
                                break;                 
                            case HD_LOC_DEV_6:
                                Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                                if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_6), ON);
                                    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                }
                                else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_6), OFF);
                                    pRx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                }
                                pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                pTx->count			= HD_T_MAX_11;
                                break;                   
                
                        }
                    }
                    else
                    {
                        if(Loc_LSB == 0)
                        {
                            // all device control
                        }
                    }
                    break;
                case HD_SRV_DIM:
                    Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                    if(pRx->buf[HD_F_DEV] == HD_DEV_LIGHT_DIM)      //0x1A
                    {
                        if(Loc_MSB == SWITCH_NUM)
                        {
                            if(Loc_LSB == SWITCH_CIRCUIT_NUM)
                            {
                                if(pRx->buf[HD_F_CMD] > 0)
                                {
                                    pG_State->Dimming_Level.Dimming1 = pRx->buf[HD_F_CMD];
                                    PUT_PWMCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_1),ON);
                                    EventCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_1), ON);
                                    pTx->buf[HD_F_ARG_1]    = pRx->buf[HD_F_CMD];
                                }
                                else
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_1), OFF); //dimming level은 0이 없음. OFF 동작.
                                    pTx->buf[HD_F_ARG_1] = pRx->buf[HD_F_CMD];
                                }
                                pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                pTx->count			    = HD_T_MAX_11;
                            }
                        }
                    }
                    break;
                case HD_SRV_BATCHLIGHT:
                    Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                    if(Loc_MSB == SWITCH_NUM)
                    {
                        if(Loc_LSB == 0)
                        {
                            if(pRx->buf[HD_F_CMD] == 0x01)
                            {
                                EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), ON);
                                pTx->buf[HD_F_ARG_1] = 0x01;
                            }
                            else if(pRx->buf[HD_F_CMD] == 0x02)
                            {
                                EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), OFF);
                                pRx->buf[HD_F_ARG_1] = 0x02;
                            }
                            pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                            pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                            pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                            pTx->count			= HD_T_MAX_11;                        
                        }
                    }
                    break;
            }
            break;
    }
}
            

// 테스트	F7 20 21 01 01 00 00 00 00 00 00 00 00 43 AA
void HYUNDAI_Process(uint8_t data)
{
	HYUNDAI_BUF	*pRx;
	uint8_t		crc = 0;
    uint8_t     cnt = 0;
    int i;

	pRx = &HYUNDAI_RxBuf;
    switch(pRx->count)
    {
        default:
            if((pRx->buf[HD_F_STX] != HYUNDAI_STX) && (data == HYUNDAI_STX))
            {
                pRx->count = 0;
            }
            break;
        case 1:
            if(pRx->buf[HD_F_STX] != HYUNDAI_STX)
            {
                pRx->count = 0;
            }
            break;
        case 2:
            if(pRx->buf[HD_F_LEN] < 0x0B || pRx->buf[HD_F_LEN] > 0x10)  //LEN 0x0B ~ 0x10
            {
                pRx->count = 0;
            }
            break;
        case 3:
            if((pRx->buf[HD_F_VEN] != HYUNDAI_VEN))
            {
                pRx->count = 0;
            }
            break;
    }
    pRx->buf[pRx->count++] = data;

    if(pRx->count >= 17)
    {
        pRx->buf[0];
        pRx->count = 0;
        pRx->length.word = 0;
    }

    if(pRx->count == HD_F_SRV)
    {
        pRx->length.value = pRx->buf[HD_F_LEN];
    }

    if(pRx->length.value == 0x0B)        cnt = 11;
    else if(pRx->length.value == 0x0C)   cnt = 12;
    else if(pRx->length.value == 0x0D)   cnt = 13;
    else if(pRx->length.value == 0x0E)   cnt = 14;
    else if(pRx->length.value == 0x0F)   cnt = 15;
    else if(pRx->length.value == 0x10)   cnt = 16;

    if(pRx->count == pRx->length.word + cnt)
    {
        // printf("data = 0x%02X cnt = %d\r\n", (uint16_t)data, (uint16_t)cnt);
        if(data == HYUNDAI_ETX)
        {
            crc = HYUNDAI_Crc(pRx, pRx->count-2);
            
            if(crc == pRx->buf[pRx->count-2])
            {
                HYUNDAI_Data_Process(pRx);
                // printf("cal crc[0x%02X] == buf crc[0x%02X]\r\n", (uint16_t)crc, (uint16_t)pRx->buf[pRx->count-2]);
            }
            else
            {
                printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[pRx->count-2]);
            }
        }


        pRx->buf[0] = 0;
        pRx->count = 0;
        pRx->length.word = 0;
    }
}


#endif
// ----------------------------------------------------------------------------------------
// uint8_t			Gu8_RS485_TX_Enable	= 0;

void HYUNDAI_Process(uint8_t data);
void LEN_(HYUNDAI_BUF *pRx);
//-------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------
void HD_RS485_Init(void)
{
	CLK_PeripheralClockConfig((CLK_Peripheral_TypeDef)RS_485_PORT_CLK, ENABLE);	// Enable RS_485 USART clock
	
	SYSCFG_REMAPPinConfig(REMAP_Pin_USART3TxRxPortF, DISABLE);		// USART3_TX mapped on PG1 and USART3_RX mapped on PG0
	
	GPIO_ExternalPullUpConfig(RS_485_TX_PORT, RS_485_TX_PIN, ENABLE);			// RS_485 Tx as alternate function push-pull
	GPIO_ExternalPullUpConfig(RS_485_RX_PORT, RS_485_RX_PIN, ENABLE);			// RS_485 Rx as alternate function push-pull
	USART_Init(RS_485_USART, 9600, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
	//-------------------------------------------------------------------------------------------------------------------------
	GPIO_Init(RS_485_DE_PORT, RS_485_DE_PIN, GPIO_Mode_Out_PP_Low_Fast);		// Output
	GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);		// Low
	//-------------------------------------------------------------------------------------------------------------------------
	USART_ITConfig(RS_485_USART, USART_IT_RXNE, ENABLE);
	USART_Cmd(RS_485_USART, ENABLE);
	//-------------------------------------------------------------------------------------------------------------------------
	printf("RS-485 Init\n");

	memset((void*)&HYUNDAI_RxBuf,		0,	sizeof(HYUNDAI_BUF));
	memset((void*)&HYUNDAI_TxBuf,		0,	sizeof(HYUNDAI_BUF));
}

void HD_RS485_Process(void)
{
	uint16_t	i;
	uint8_t		data;
	uint8_t		touch_switch;
	//---------------------------------------------------------------------------------------------------------------------------------------
	if(Gu8_RS485_TX_Enable == 0)		// 5000~10000번에 한번식 High로 유지되는 경우가 있어 적용함(많을 경우 1000번에 한번, Low 시키는 루틴은 정상적으로 실행됨)
	{
		if(GPIO_ReadInputDataBit(RS_485_DE_PORT, (GPIO_Pin_TypeDef)RS_485_DE_PIN))	// GPIO Read
		{
			GPIO_ResetBits(RS_485_DE_PORT, RS_485_DE_PIN);			// Receiver Active Low
		}
	}
			
	while(Uart_GetRxQ(RS_485_PORT, &data) != 0)
	{
		// if(G_Debug == DEBUG_HOST_REALDATA)
        if(1)
		{
			if(pG_Config->Protocol_Type == HYUNDAI_PROTOCOL)
			{
				if(data == HYUNDAI_STX)	printf("\n");
			}
			printf("%02X ", (uint16_t)data);
		}
		//------------------------------------------------------------------------------------------
		HYUNDAI_Process(data);
		//------------------------------------------------------------------------------------------
	}
	//------------------------------------------------------------------------------------------
	if(Gu8_RS_485_Rx_Tmr == 0)		// CVNET 인터벌 타임은 5ms 초과 할 수 없다,		마지막 데이터 수신 후 6ms 가 초과하면 데이터 클리어
	{
		HYUNDAI_RxBuf.buf[0]	= 0;
		HYUNDAI_RxBuf.count	    = 0;
	}
	//------------------------------------------------------------------------------------------
	if(pG_Config->Protocol_Type == HYUNDAI_PROTOCOL)
	{
		if(Gu8_RS_485_Tx_Tmr == 0)		// 데이터 수신 후 10ms 이후 데이터 전송
		{
			if(HYUNDAI_TxBuf.count)
			{
				TX_Queue_Clear(RS_485_PORT);
				Uart_PutsTxQ(RS_485_PORT, HYUNDAI_TxBuf.buf, HYUNDAI_TxBuf.count);
				
				// if(G_Debug == DEBUG_HOST)
                if(1)
				{
					printf("\nTX(HYUNDAI) : ");
					for(i=0;i<HYUNDAI_TxBuf.count;i++)
					{
						printf("%02X ", (uint16_t)HYUNDAI_TxBuf.buf[i]);
					}
					printf("\n");
				}
				
				HYUNDAI_TxBuf.count	= 0;
			}
		}
	}
	else if(pG_Config->Protocol_Type == HYUNDAI_PROTOCOL)
	{
		if(Gu8_RS_485_Tx_Tmr == 0 && Event_Empt())		// 데이터 수신 후 30ms 지나고 제어를 완료하였으면 데이터 전송
		{
			;
		}
	}
	//------------------------------------------------------------------------------------------
}


// HYUNDAI ----------------------------------------------------------------------------------
/*
	* 30ms 지나고 300ms 이내에 응답
	* Interval time은 5ms
	------------------------------------------------------------------------------------------
	STX | LEN | VEN | DEV | TYPE | SRV | LOC | CMD | ARG1 ~ ARGn | CRC | ETX
	------------------------------------------------------------------------------------------
    STX
    0xF7로 고정

    LEN
    STX ~ ETX까지의 Packet 길이
    
    VEN
    0x01

    DEV
    일반 조명       0x19
    디밍 조명       0x1A

    TYPE
    제어 요청에 대한 응답은 제어 기능 수행 이후 응답한다(단 ,응답시간이 300ms를 초과하여서는 안됨)
    상태요구             0x01       (명령사용 : 현대통신)
    제어명령             0x02       (명령사용 : 현대통신)
    Response Success    0x04       (명령사용 : 기기업체)
    
    SRV
    전원            0x40
    밝기 조절       0x42
    일괄제어        0x70

    LOC
    상위 니블 : 그룹 넘버, 하위 니블 : 그룹 내 기기ID
    상위 니블은 물리적인 조명 스위치를 의미
    하위 니블은 해당 조명 스위치의 회로를 의미
    각 니블은 1 ~ 6 까지 사용
    
    전체 그룹 조명 제어 시 : 상위 니블인 그룹 넘버를 0으로 제어
    해당 그룹 전체 제어 시 : 하위 니블인 기기 ID를 0으로 제어
    * 예시 0x11(1번  스위치의 1번 회로 : 11, 2번 회로 : 12)

	CMD
    ON              0x01
    OFF             0x02
    NUMBER          0 ~ 7

    ARG
    전체 상태 응답 시 해당 구수 별로 ARG 개수 가변
    ARG는 BYTE로 회로 상태 표시.
    ON : 0x01, OFF : 0x02
    1구일 경우 전체 상태 응답시 1BYTE, 6구일 경우 전체 상태 응답 시 6BYTE
    상태 응답 시 현재 상태 표시

    CRC
    STX ~ CRC전까지의 Exclusive OR

    ETX
    0xEE로 고정
*/

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

void HYUNDAI_Data_Process(HYUNDAI_BUF	*pRx)
{
	uint16_t	i;
	uint8_t		arg_cnt, group_ctrl_flag	= 0;
	uint8_t		item = 0;
	uint8_t		touch_switch;		//20201118
	uint8_t		Flag;				//20201118
    uint8_t     Loc_MSB;
    uint8_t     Loc_LSB;

	HYUNDAI_BUF	*pTx;

	pTx = &HYUNDAI_TxBuf;
	pTx->buf[HD_F_STX]	= HYUNDAI_STX;
	pTx->buf[HD_F_LEN]	= pRx->buf[HD_F_LEN];
	pTx->buf[HD_F_VEN]	= (uint8_t)pG_Config->RS485_ID;
    pTx->buf[HD_F_DEV]  = pRx->buf[HD_F_DEV];
    // pTx->buf[HD_F_TYPE]	= HD_RES_SUCCESS;
    pTx->buf[HD_F_SRV]  = pRx->buf[HD_F_SRV];
    pTx->buf[HD_F_LOC]  = pRx->buf[HD_F_LOC];
    pTx->buf[HD_F_CMD]  = pRx->buf[HD_F_CMD];
	
    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC 상위 니블
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC 하위 니블
	
    // printf("data process start!\r\n");

    // for(arg_cnt = HD_F_ARG_1; arg_cnt <= HD_F_ARG_1; arg_cnt++)
	// {
	// 	pTx->buf[arg_cnt]	= 0;
	// }
	
    switch(pRx->buf[HD_F_TYPE])
    {
        case HD_TYPE_STATE_REQ:
            Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
            arg_cnt	= HD_F_ARG_1;

            if(Loc_MSB == 1)        //1번 스위치
            {
                if(Loc_LSB == SWITCH_NUM_1)     //switch1
                {
                    if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_1)))    pTx->buf[HD_F_ARG_1] = 0x01;
                    else                                                    pTx->buf[HD_F_ARG_1] = 0x02;
                    // printf("SWITCH_NUM1\r\n");
                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
			        pTx->buf[9]	= HYUNDAI_Crc(pTx, pRx->count-2);
                    pTx->buf[10]	= HYUNDAI_ETX;
                    pTx->count = 11;
                }
                else if(Loc_LSB == SWITCH_NUM_2)
                {
                    if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_2)))    pTx->buf[HD_F_ARG_1] = 0x01;
                    else                                                    pTx->buf[HD_F_ARG_1] = 0x02;
                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
			        pTx->buf[9]	= HYUNDAI_Crc(pTx, pRx->count-2);
                    pTx->buf[10]	= HYUNDAI_ETX;
                    pTx->count = 11;                    
                }
                else if(Loc_LSB == SWITCH_NUM_3)
                {
                    if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_3)))    pTx->buf[HD_F_ARG_1] = 0x01;
                    else                                                    pTx->buf[HD_F_ARG_1] = 0x02;
                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
			        pTx->buf[9]	= HYUNDAI_Crc(pTx, pRx->count-2);
                    pTx->buf[10]	= HYUNDAI_ETX;
                    pTx->count = 11;                    
                }
                else if(Loc_LSB == SWITCH_NUM_4)
                {
                    if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_4)))    pTx->buf[HD_F_ARG_1] = 0x01;
                    else                                                    pTx->buf[HD_F_ARG_1] = 0x02;
                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
			        pTx->buf[9]	= HYUNDAI_Crc(pTx, pRx->count-2);
                    pTx->buf[10]	= HYUNDAI_ETX;
                    pTx->count = 11;                    
                }
                else if(Loc_LSB == SWITCH_NUM_5)
                {
                    if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_5)))    pTx->buf[HD_F_ARG_1] = 0x01;
                    else                                                    pTx->buf[HD_F_ARG_1] = 0x02;
                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
			        pTx->buf[9]	= HYUNDAI_Crc(pTx, pRx->count-2);
                    pTx->buf[10]	= HYUNDAI_ETX;
                    pTx->count = 11;                    
                }
                else if(Loc_LSB == SWITCH_NUM_6)
                {
                    if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_6)))    pTx->buf[HD_F_ARG_1] = 0x01;
                    else                                                    pTx->buf[HD_F_ARG_1] = 0x02;
                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
			        pTx->buf[9]	= HYUNDAI_Crc(pTx, pRx->count-2);
                    pTx->buf[10]	= HYUNDAI_ETX;
                    pTx->count = 11;                    
                }
                else if(Loc_LSB == SWITCH_NUM_ALL)
                {
                    if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_1)))    pTx->buf[HD_F_ARG_1] = 0x01;
                    else                                                    pTx->buf[HD_F_ARG_1] = 0x02;
                    // printf("SWITCH_NUM_ALL\r\n");
                    // if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_2)))    pTx->buf[HD_F_ARG_2] = 0x01;
                    // else                                                    pTx->buf[HD_F_ARG_2] = 0x02;
                    // if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_3)))    pTx->buf[HD_F_ARG_3] = 0x01;
                    // else                                                    pTx->buf[HD_F_ARG_3] = 0x02;
                    // if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_4)))    pTx->buf[HD_F_ARG_4] = 0x01;
                    // else                                                    pTx->buf[HD_F_ARG_4] = 0x02;
                    // if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_5)))    pTx->buf[HD_F_ARG_5] = 0x01;
                    // else                                                    pTx->buf[HD_F_ARG_5] = 0x02;
                    // if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_6)))    pTx->buf[HD_F_ARG_6] = 0x01;
                    // else                                                    pTx->buf[HD_F_ARG_6] = 0x02;                                                                                                    
                }
            }
            break;

        case HD_TYPE_CONTROL_REQ:
            if(Loc_MSB == 1)
            {
                switch(Loc_LSB)
                {
                    case HD_LOC_DEV_ALL:
                        Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                        arg_cnt	= HD_F_ARG_1;

                        if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_ALL)))
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_ALL), OFF);
                            pTx->buf[arg_cnt] = 0x01;
                        }   
                        else
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_ALL), ON);
                            pTx->buf[arg_cnt] = 0x02;
                        }
                        pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                        pTx->buf[9]	= HYUNDAI_Crc(pTx, 9);
                        pTx->buf[10]	= HYUNDAI_ETX;
                        // pTx->count			= HD_MAX_BUF;
                        break;
                    case HD_LOC_DEV_1:
                        Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                        arg_cnt	= HD_F_ARG_1;
                        if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_1)))
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_1), OFF);
                            pTx->buf[arg_cnt] = 0x01;
                        }   
                        else
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_1), ON);
                            pTx->buf[arg_cnt] = 0x02;
                        }
                        pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                        pTx->buf[9]	= HYUNDAI_Crc(pTx, 9);
                        pTx->buf[10]	= HYUNDAI_ETX;
                        // pTx->count			= HD_MAX_BUF;
                        break;
                    case HD_LOC_DEV_2:
                        Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                        arg_cnt	= HD_F_ARG_1;
                        if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_2)))
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_2), OFF);
                            pTx->buf[arg_cnt] = 0x01;
                        }   
                        else
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_2), ON);
                            pTx->buf[arg_cnt] = 0x02;
                        }
                        pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                        pTx->buf[9]	= HYUNDAI_Crc(pTx, 9);
                        pTx->buf[10]	= HYUNDAI_ETX;
                        // pTx->count			= HD_MAX_BUF;
                        break;
                    case HD_LOC_DEV_3:
                        Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                        arg_cnt	= HD_F_ARG_1;
                        if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_3)))
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_3), OFF);
                            pTx->buf[arg_cnt] = 0x01;
                        }   
                        else
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_3), ON);
                            pTx->buf[arg_cnt] = 0x02;
                        }
                        pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                        pTx->buf[9]	= HYUNDAI_Crc(pTx, 9);
                        pTx->buf[10]	= HYUNDAI_ETX;
                        // pTx->count			= HD_MAX_BUF;
                        break;                  
                    case HD_LOC_DEV_4:
                        Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                        arg_cnt	= HD_F_ARG_1;
                        if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_4)))
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_4), OFF);
                            pTx->buf[arg_cnt] = 0x01;
                        }   
                        else
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_4), ON);
                            pTx->buf[arg_cnt] = 0x02;
                        }
                        pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                        pTx->buf[9]	= HYUNDAI_Crc(pTx, 9);
                        pTx->buf[10]	= HYUNDAI_ETX;
                        // pTx->count			= HD_MAX_BUF;
                        break;                  
                    case HD_LOC_DEV_5:
                        Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                        arg_cnt	= HD_F_ARG_1;
                        if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_5)))
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_5), OFF);
                            pTx->buf[arg_cnt] = 0x01;
                        }   
                        else
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_5), ON);
                            pTx->buf[arg_cnt] = 0x02;
                        }
                        pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                        pTx->buf[9]	= HYUNDAI_Crc(pTx, 9);
                        pTx->buf[10]	= HYUNDAI_ETX;
                        // pTx->count			= HD_MAX_BUF;
                        break;                 
                    case HD_LOC_DEV_6:
                        Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
                        arg_cnt	= HD_F_ARG_1;
                        if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_6)))
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_6), OFF);
                            pTx->buf[arg_cnt] = 0x01;
                        }   
                        else
                        {
                            EventCtrl(item2tsn(mapping_ITEM_LIGHT_6), ON);
                            pTx->buf[arg_cnt] = 0x02;
                        }
                        pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                        pTx->buf[9]	= HYUNDAI_Crc(pTx, 9);
                        pTx->buf[10]	= HYUNDAI_ETX;
                        // pTx->count			= HD_MAX_BUF;
                        break;                   
                    
                }
            }
            else
            {
                if(Loc_LSB == 0)
                {
                    // all device control
                }
            }
            break;
    }


/*
	switch(pRx->buf[CVNET_F_CMD])
	{
		case STATE_REQ:				// 0x01
			Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;		// 10ms 후 전송
			arg_cnt	= HD_F_ARG_1;
			
			for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
			{
				item	= light_circuit2item((uint8_t)i);
				
				if(item == mapping_ITEM_DIMMING_LIGHT_1)
				{
					if(GET_Switch_State(item2tsn(item)))	pTx->buf[arg_cnt]	= pG_State->Dimming_Level.Dimming1;
					else									pTx->buf[arg_cnt]	= 0;
				}
				else if(item == mapping_ITEM_DIMMING_LIGHT_2)
				{
					if(GET_Switch_State(item2tsn(item)))	pTx->buf[arg_cnt]	= pG_State->Dimming_Level.Dimming2;
					else									pTx->buf[arg_cnt]	= 0;
				}
				else if(item)
				{
					pTx->buf[arg_cnt]	= GET_Switch_State(item2tsn(item));
				}
				else
				{
					break;
				}
				arg_cnt++;
			}
			pTx->buf[CVNET_F_FEC]	= CVNET_FEC_Check(pTx);
			pTx->count				= CVNET_MAX_BUF;
			break;
		
		case TOTAL_CONTROL_REQ:		// 0x1F
			Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;		// 10ms 후 전송
			arg_cnt	= CVNET_F_D0;
			
			for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
			{
				item	= light_circuit2item((uint8_t)i);
				
				if(item == mapping_ITEM_DIMMING_LIGHT_1)
				{
					pG_State->Dimming_Level.Dimming1	= pRx->buf[arg_cnt];
					if(pRx->buf[arg_cnt])	PUT_PWMCtrl(item2tsn(item), ON);
					else					PUT_PWMCtrl(item2tsn(item), OFF);
				}
				else if(item == mapping_ITEM_DIMMING_LIGHT_2)
				{
					pG_State->Dimming_Level.Dimming2	= pRx->buf[arg_cnt];
					if(pRx->buf[arg_cnt])	PUT_PWMCtrl(item2tsn(item), ON);
					else					PUT_PWMCtrl(item2tsn(item), OFF);
					pTx->buf[arg_cnt]		= pRx->buf[arg_cnt];
				}
				else if(item)
				{
					if(pRx->buf[arg_cnt])	PUT_RelayCtrl(item2tsn(item), ON);
					else					PUT_RelayCtrl(item2tsn(item), OFF);
					pTx->buf[arg_cnt]		= pRx->buf[arg_cnt];
				}
				else
				{
					break;
				}
				arg_cnt++;
			}
			pTx->buf[CVNET_F_FEC]	= CVNET_FEC_Check(pTx);
			pTx->count				= CVNET_MAX_BUF;
			break;
		
		case SELECT_CONTROL_1_REQ:	// 0x11~0x18
			Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;		// 10ms 후 전송
			arg_cnt		= CVNET_F_D0;
			
			for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
			{
				item	= light_circuit2item((uint8_t)i);
				if(item == mapping_ITEM_DIMMING_LIGHT_1)
				{
					if(GET_Switch_State(item2tsn(item)))	pTx->buf[arg_cnt]	= pG_State->Dimming_Level.Dimming1;
					else									pTx->buf[arg_cnt]	= 0;
				}
				else if(item == mapping_ITEM_DIMMING_LIGHT_2)
				{
					if(GET_Switch_State(item2tsn(item)))	pTx->buf[arg_cnt]	= pG_State->Dimming_Level.Dimming2;
					else									pTx->buf[arg_cnt]	= 0;
				}
				// else if(item == mapping_ITEM_LIGHT_1)		//or mapping_ITEM_LIGHT_1 (F7 20 21 01 11 01 00 00 00 00 00 00 00 54 AA)
				// {
					// if(pRx->buf[arg_cnt])
					// {
					// 	if(GET_Switch_State(item2tsn(item)) == ON)
					// 	{
					// 		touch_switch = item2tsn(item);
					// 		Flag = SET_Switch_State(touch_switch, OFF);
					// 		SET_LED_State(touch_switch, Flag);
					// 		Beep(Flag);
					// 		PUT_RelayCtrl(item2ctrl(item), Flag);	//Flag
					// 	}
					// 	else if(GET_Switch_State(item2tsn(item)) == OFF)
					// 	{
					// 		touch_switch = item2tsn(item);
					// 		Flag = SET_Switch_State(touch_switch, ON);
					// 		SET_LED_State(touch_switch, Flag);
					// 		Beep(Flag);
					// 		PUT_RelayCtrl(item2ctrl(item), Flag); //Flag
					// 	}
						
					// }
					// else if(pRx->buf[arg_cnt] == 0)
					// {
					// 	touch_switch = item2tsn(item);
					// 	Flag = SET_Switch_State(touch_switch, OFF);
					// 	SET_LED_State(touch_switch, Flag);
					// 	Beep(Flag);
					// 	PUT_RelayCtrl(item2ctrl(item), Flag);
					// }
				// 	EventCtrl(item2tsn(item), INVERSE);
				// 	pTx->buf[arg_cnt]	= GET_Switch_State(item2tsn(item));
				// } //Rs485 인증테스트 20201118
				else if(item)
				{
					pTx->buf[arg_cnt]	= GET_Switch_State(item2tsn(item));
				}
				else
				{
					break;
				}
				arg_cnt++;
			}
			
			item	= light_circuit2item((uint8_t)(pRx->buf[CVNET_F_CMD]&0x0F));
			if(item == mapping_ITEM_DIMMING_LIGHT_1)
			{
				pG_State->Dimming_Level.Dimming1	= pRx->buf[CVNET_F_D0];
				if(pRx->buf[CVNET_F_D0])	PUT_PWMCtrl(item2tsn(item), ON);
				else						PUT_PWMCtrl(item2tsn(item), OFF);
			}
			else if(item == mapping_ITEM_DIMMING_LIGHT_2)
			{
				pG_State->Dimming_Level.Dimming2	= pRx->buf[CVNET_F_D0];
				if(pRx->buf[CVNET_F_D0])	PUT_PWMCtrl(item2tsn(item), ON);
				else						PUT_PWMCtrl(item2tsn(item), OFF);
			}
			// else if(item)
			// {
			// 	if(pRx->buf[CVNET_F_D0])	PUT_RelayCtrl(item2tsn(item), ON);
			// 	else						PUT_RelayCtrl(item2tsn(item), OFF);
			// 	;
			// }
			else if(item == mapping_ITEM_LIGHT_1)
			{
				EventCtrl(item2tsn(item), INVERSE);
			}
			else
			{
				break;
			}
			pTx->buf[(uint8_t)(pRx->buf[CVNET_F_CMD] & 0x0F)] = pRx->buf[CVNET_F_D0];
			pTx->buf[CVNET_F_CMD] = TOTAL_CONTROL_RES; // 선택제어에 대한 응답은 0x9F로 응답
			pTx->buf[CVNET_F_FEC] = CVNET_FEC_Check(pTx);
			pTx->count = CVNET_MAX_BUF;
			break;

		case SELECT_CONTROL_2_REQ:
			Gu8_RS_485_Tx_Tmr = pG_Config->Protocol_RES_DelayTime; // 10ms 후 전송
			arg_cnt = CVNET_F_D0;

			for (i = CIRCUIT_1; i < CIRCUIT_MAX; i++)
			{
				item = electricity_circuit2item((uint8_t)i);
				if (item == mapping_ITEM_DIMMING_LIGHT_1)
				{
					if (GET_Switch_State(item2tsn(item)))	pTx->buf[arg_cnt] = pG_State->Dimming_Level.Dimming1;
					else									pTx->buf[arg_cnt] = 0;
				}
				else if (item == mapping_ITEM_DIMMING_LIGHT_2)
				{
					if (GET_Switch_State(item2tsn(item)))	pTx->buf[arg_cnt] = pG_State->Dimming_Level.Dimming2;
					else									pTx->buf[arg_cnt] = 0;
				}
				else if (item)
				{
					pTx->buf[arg_cnt] = GET_Switch_State(item2tsn(item));
				}
				else
				{
					break;
				}
				arg_cnt++;
			}
				item = light_circuit2item((uint8_t)(pRx->buf[CVNET_F_CMD] & 0x0F));		//20201120 ?????
				//item = electricity_circuit2item((uint8_t)i);								//안되는듯..
				item = mapping_ITEM_ELECTRICITY_1;
				if (item == mapping_ITEM_DIMMING_LIGHT_1)
				{
					pG_State->Dimming_Level.Dimming1 = pRx->buf[CVNET_F_D0];
					if (pRx->buf[CVNET_F_D0])	PUT_PWMCtrl(item2tsn(item), ON);
					else						PUT_PWMCtrl(item2tsn(item), OFF);
				}
				else if (item == mapping_ITEM_DIMMING_LIGHT_2)
				{
					pG_State->Dimming_Level.Dimming2 = pRx->buf[CVNET_F_D0];
					if (pRx->buf[CVNET_F_D0])	PUT_PWMCtrl(item2tsn(item), ON);
					else						PUT_PWMCtrl(item2tsn(item), OFF);
				}
				else if (item == mapping_ITEM_ELECTRICITY_1)		//(F7 20 C1 01 12 01 01 00 00 00 00 00 00 F6 AA)
				{
					if (pRx->buf[CVNET_F_D0])
					{
						if (GET_Switch_State(item2tsn(item)) == ON)
						{
							touch_switch = item2tsn(item);
							Flag = SET_Switch_State(touch_switch, OFF);
							SET_LED_State(touch_switch, Flag);
							Beep(Flag);
							PUT_RelayCtrl(item2ctrl(item), Flag);
						}
						else if (GET_Switch_State(item2tsn(item)) == OFF)
						{
							touch_switch = item2tsn(item);
							Flag = SET_Switch_State(touch_switch, ON);
							SET_LED_State(touch_switch, Flag);
							Beep(Flag);
							PUT_RelayCtrl(item2ctrl(item), Flag);
						}
						PUT_RelayCtrl(item2tsn(item), ON);
					}
					else
						PUT_RelayCtrl(item2tsn(item), OFF);
				}
				else
				{
					break;
				}
			pTx->buf[(uint8_t)(pRx->buf[CVNET_F_CMD] & 0x0F)] = pRx->buf[CVNET_F_D0];
			pTx->buf[CVNET_F_CMD] = TOTAL_CONTROL_RES; // 선택제어에 대한 응답은 0x9F로 응답
			pTx->buf[CVNET_F_FEC] = CVNET_FEC_Check(pTx);
			pTx->count = CVNET_MAX_BUF;
			break;

		case SELECT_CONTROL_3_REQ:
		case SELECT_CONTROL_4_REQ:
		case SELECT_CONTROL_5_REQ:
		case SELECT_CONTROL_6_REQ:
		case SELECT_CONTROL_7_REQ:
		case SELECT_CONTROL_8_REQ:

		case CHARACTERISTIC_REQ:								   // 0x21 ~ 0xA1?
			Gu8_RS_485_Tx_Tmr = pG_Config->Protocol_RES_DelayTime; // 10ms 후 전송
			arg_cnt = CVNET_F_D0;

			for (i = CIRCUIT_1; i < CIRCUIT_MAX; i++)
			{
				item = light_circuit2item((uint8_t)i);

				if (item == mapping_ITEM_DIMMING_LIGHT_1)
				{
					pTx->buf[arg_cnt] = pG_Config->Dimming_MAX_Level;
				}
				else if (item == mapping_ITEM_DIMMING_LIGHT_2)
				{
					pTx->buf[arg_cnt] = pG_Config->Dimming_MAX_Level;
				}
				else if (item)
				{
					pTx->buf[arg_cnt] = 1;
				}
				else
				{
					break;
				}
				arg_cnt++;
			}
			pTx->buf[CVNET_F_FEC] = CVNET_FEC_Check(pTx);
			pTx->count = CVNET_MAX_BUF;
			break;

		case CHARACTERISTIC_FIRMWARE_VERSION_REQ:
			Gu8_RS_485_Tx_Tmr = pG_Config->Protocol_RES_DelayTime; // 10ms 후 전송

			sprintf((void *)&pTx->buf[CVNET_F_D0], "NEO %03d", VERSION); // 8바이트를 넘으면 안된다

			pTx->buf[CVNET_F_FEC] = CVNET_FEC_Check(pTx);
			pTx->count = CVNET_MAX_BUF;
			break;

		case TOTAL_GROUP_CONTROL_REQ:
			group_ctrl_flag = 1;
		case GROUP_CONTROL_1_REQ:																		  // 0x31	응답없음
			if ((uint8_t)(pG_Config->RS485_ID & 0x0F) >= 1 && (uint8_t)(pG_Config->RS485_ID & 0x0F) <= 3) // ID 1~3
			{
				group_ctrl_flag = 1;
			}
		case GROUP_CONTROL_2_REQ:
			if((uint8_t)(pG_Config->RS485_ID&0x0F) >= 4 && (uint8_t)(pG_Config->RS485_ID&0x0F) <= 6)	// ID 4~6
			{
				group_ctrl_flag	= 1;
			}
		case GROUP_CONTROL_3_REQ:
			if((uint8_t)(pG_Config->RS485_ID&0x0F) >= 7 && (uint8_t)(pG_Config->RS485_ID&0x0F) <= 9)	// ID 7~9
			{
				group_ctrl_flag	= 1;
			}
		case GROUP_CONTROL_4_REQ:
			if((uint8_t)(pG_Config->RS485_ID&0x0F) >= 10 && (uint8_t)(pG_Config->RS485_ID&0x0F) <= 12)	// ID 10~12
			{
				group_ctrl_flag	= 1;
			}
		case GROUP_CONTROL_5_REQ:
		case GROUP_CONTROL_6_REQ:
		case GROUP_CONTROL_7_REQ:
		case GROUP_CONTROL_8_REQ:
		case GROUP_CONTROL_9_REQ:
		case GROUP_CONTROL_10_REQ:
		case GROUP_CONTROL_11_REQ:
		case GROUP_CONTROL_12_REQ:
		case GROUP_CONTROL_13_REQ:
		case GROUP_CONTROL_14_REQ:
			arg_cnt	= CVNET_F_D0;
			
			if((pRx->buf[CVNET_F_DA]&0x0F) == 0x0F && group_ctrl_flag)		// 그룹주소 확인
			{
				for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
				{
					item	= light_circuit2item((uint8_t)i);
					
					if(item == mapping_ITEM_DIMMING_LIGHT_1)
					{
						pG_State->Dimming_Level.Dimming1	= pRx->buf[arg_cnt];
						if(pRx->buf[arg_cnt])	PUT_PWMCtrl(item2tsn(item), ON);
						else					PUT_PWMCtrl(item2tsn(item), OFF);
					}
					else if(item == mapping_ITEM_DIMMING_LIGHT_2)
					{
						pG_State->Dimming_Level.Dimming2	= pRx->buf[arg_cnt];
						if(pRx->buf[arg_cnt])	PUT_PWMCtrl(item2tsn(item), ON);
						else					PUT_PWMCtrl(item2tsn(item), OFF);
						pTx->buf[arg_cnt]		= pRx->buf[arg_cnt];
					}
					else if(item)
					{
						if(pRx->buf[arg_cnt])	PUT_RelayCtrl(item2tsn(item), ON);
						else					PUT_RelayCtrl(item2tsn(item), OFF);
						pTx->buf[arg_cnt]		= pRx->buf[arg_cnt];
					}
					else
					{
						break;
					}
					arg_cnt++;
				}
			}
			break;
		}

	if(G_Debug == DEBUG_HOST)
	{
		printf("\n\nRX(CVNET) : ");
		for(i=0;i<pRx->count;i++)
		{
			printf("%02x ", (uint16_t)pRx->buf[i]);
		}
		printf("\n");
	}
    */
}

// 테스트	F7 20 21 01 01 00 00 00 00 00 00 00 00 43 AA
void HYUNDAI_Process(uint8_t data)
{
	HYUNDAI_BUF	*pRx;
	uint8_t		crc = 0;
    uint8_t     cnt = 0;
    int i;

	pRx = &HYUNDAI_RxBuf;
    switch(pRx->count)
    {
        default:
            if((pRx->buf[HD_F_STX] != HYUNDAI_STX) && (data == HYUNDAI_STX))
            {
                pRx->count = 0;
                // printf("0\r\n");
            }
            break;
        case 1:
            if(pRx->buf[HD_F_STX] != HYUNDAI_STX)
            {
                pRx->count = 0;
                // printf("1\r\n");
            }
            break;
        case 2:
            if(pRx->buf[HD_F_LEN] < 0x0B || pRx->buf[HD_F_LEN] > 0x10)  //LEN 0x0B ~ 0x10
            {
                pRx->count = 0;
                // printf("2\r\n");
            }
            break;
        case 3:
            if((pRx->buf[HD_F_VEN] != HYUNDAI_VEN))
            {
                pRx->count = 0;
                // printf("3\r\n");
            }
            break;
    }
    pRx->buf[pRx->count++] = data;

    if(pRx->count >= 17)
    {
        pRx->buf[0];
        pRx->count = 0;
        pRx->length.word = 0;
    }

    if(pRx->count == HD_F_SRV)
    {
        pRx->length.value = pRx->buf[HD_F_LEN];
    }

    if(pRx->length.value == 0x0B)        cnt = 11;
    else if(pRx->length.value == 0x0C)   cnt = 12;
    else if(pRx->length.value == 0x0D)   cnt = 13;
    else if(pRx->length.value == 0x0E)   cnt = 14;
    else if(pRx->length.value == 0x0F)   cnt = 15;
    else if(pRx->length.value == 0x10)   cnt = 16;

    if(pRx->count == pRx->length.word + cnt)
    {
        // printf("data = 0x%02X cnt = %d\r\n", (uint16_t)data, (uint16_t)cnt);
        if(data == HYUNDAI_ETX)
        {
            crc = HYUNDAI_Crc(pRx, pRx->count-2);
            
            if(crc == pRx->buf[pRx->count-2])
            {
                HYUNDAI_Data_Process(pRx);
                // printf("cal crc[0x%02X] == buf crc[0x%02X]\r\n", (uint16_t)crc, (uint16_t)pRx->buf[pRx->count-2]);
            }
            else
            {
                printf("cal crc[0x%02X] != buf crc[0x%02X]", (uint16_t)crc, (uint16_t)pRx->buf[pRx->count-2]);
            }
        }


        pRx->buf[0] = 0;
        pRx->count = 0;
        pRx->length.word = 0;
    }
}

// ----------------------------------------------------------------------------------------
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

#ifdef HYUNDAI
HYUNDAI_BUF	HYUNDAI_RxBuf, HYUNDAI_TxBuf;
__IO uint8_t Gu8_RS_485_Rx_Tmr = 0;
__IO uint8_t Gu8_RS_485_Tx_Tmr = 0;
uint8_t			Gu8_RS485_TX_Enable	= 0;
uint8_t         Gu8_WallPad_Elevetor_Call = HD_ARG_NOMAL;
uint8_t         Gu8_Elevator_Tmr = 0;
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
uint8_t Gu8_Gas_Flag                = 0;

void GAS_Off(void)
{
	if(pG_Config->Protocol_Type == HYUNDAI_PROTOCOL)
	{
		//Gu8_RS_485_Tx_Tmr = 0;					// 즉시전송, 차 후 적용시 전송시컨스에 맞춰 전송될 수 있도록 해야 함
		//HYUNDAI_TxBuf.count	= HD_T_MAX_22;	// 카운트 값이 있으면 전송
        Gu8_Gas_Flag = 1;
		printf("GAS_Off\n");
	}
}

void ELEVATOR_Call(void)
{
    Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_CALL;	// 호출에 대한 응답이 없으면 x초 후 취소플래그 설정해야함
    Gu8_Elevator_Tmr = 60;      //60sec
	printf("ELEVATOR_Call\n");
}

void ELEVATOR_Cancel(void)
{
	Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_CANCEL;
	printf("ELEVATOR_Cancel\n");
}


void RS485_Process(void)
{
	uint16_t	i;
	uint8_t		data;
	uint8_t		touch_switch;
    
	//---------------------------------------------------------------------------------------------------------------------------------------
	if(Gu8_GAS_Off_Tmr == 0)
	{
		touch_switch	= item2tsn(mapping_ITEM_GAS);
		if(GET_Switch_State(touch_switch))
		{
			SET_Switch_State(touch_switch, OFF);
			SET_LED_State(touch_switch, LED_ON);		// 제어 후 3초 경과되면 LED OFF
		}
	}
	if(Gu8_ELEVATOR_Arrive_Flag == ELEVATOR_ARRIVE)		// ELEVATOR 도착
	{
		touch_switch	= item2tsn(mapping_ITEM_ELEVATOR);
		SET_Switch_State(touch_switch, OFF);
		SET_LED_State(touch_switch, LED_ON);
		Beep(BEEP_MEL);
		Gu8_ELEVATOR_Arrive_Flag	= ELEVATOR_NON;
	}
    if((Gu8_Elevator_Tmr == 0) && (Gu8_ELEVATOR_Arrive_Flag == ELEVATOR_CALL))      //20210527 엘리베이터 콜 한 뒤 일정 시간이 지나고 응답이 없을때 원래 상태로 복귀
    {
        printf("elevator nomal\r\n");
		touch_switch	= item2tsn(mapping_ITEM_ELEVATOR);
		SET_Switch_State(touch_switch, OFF);
		SET_LED_State(touch_switch, LED_ON);
        Beep(OFF);
        Gu8_ELEVATOR_Arrive_Flag = ELEVATOR_NON;
    }
    else if((Gu8_Elevator_Tmr) &&  (Gu8_ELEVATOR_Arrive_Flag == ELEVATOR_CALL))     //20210527 엘리베이터 콜 상태에서
    {
        if(Gu8_WallPad_Elevetor_Call == HD_ARG_ARRIVE || Gu8_WallPad_Elevetor_Call == HD_ARG_ERR)       //월패드에서 오는 신호가 도착이거나 에러면 평상시 상태로 복귀
        {
            Gu8_ELEVATOR_Arrive_Flag = ELEVATOR_NON;        //원래 상태로 복귀
            Gu8_Elevator_Tmr         = 0;                   //엘리베이터 콜 타이머 초기화
        }
    }

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
		if(G_Debug == DEBUG_HOST_REALDATA)
        // if(1)
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
				
				if(G_Debug == DEBUG_HOST)
                // if(1)
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
	uint16_t	i = 0;
	uint8_t		item = 0;
    uint8_t     touch_switch, Flag;
    uint8_t     Loc_MSB, Loc_LSB;
    uint8_t     arg_cnt = 0;
    uint8_t     light_cnt, dim_cnt, elec_cnt, res;
    uint8_t     crc_cnt = 8;
    uint8_t     etx_cnt = 9;
    uint8_t     max_cnt = 10;
    uint8_t     Watt_1_MSB, Watt_1_LSB, Watt_2_MSB, Watt_2_LSB;
    uint8_t     Flag_arr[2];
	HYUNDAI_BUF	*pTx;
	pTx = &HYUNDAI_TxBuf;

	pTx->buf[HD_F_STX]	= HYUNDAI_STX;
	pTx->buf[HD_F_LEN]	= pRx->buf[HD_F_LEN];
	pTx->buf[HD_F_VEN]	= (uint8_t)pG_Config->RS485_ID;
    pTx->buf[HD_F_DEV]  = pRx->buf[HD_F_DEV];
    pTx->buf[HD_F_SRV]  = pRx->buf[HD_F_SRV];
    pTx->buf[HD_F_LOC]  = pRx->buf[HD_F_LOC];
    pTx->buf[HD_F_CMD]  = pRx->buf[HD_F_CMD];
	pTx->buf[HD_F_ARG_1] = 0;
    Loc_MSB = (uint8_t)(pRx->buf[HD_F_LOC] >> 4);        //LOC 상위 니블
    Loc_LSB = (uint8_t)(pRx->buf[HD_F_LOC] & 0x0F);      //LOC 하위 니블
	Watt_1_MSB = (uint8_t)(Gu16_LCD_Watt_1 >> 4);
    Watt_1_LSB = (uint8_t)(Gu16_LCD_Watt_1 & 0x0F);
    Watt_2_MSB = (uint8_t)(Gu16_LCD_Watt_2 >> 4);
    Watt_2_LSB = (uint8_t)(Gu16_LCD_Watt_2 & 0x0F);

    if(Loc_MSB == SWITCH_NUM && pRx->buf[HD_F_DEV] == 0x34)
    {
        for(i = 0; i < (uint16_t)pRx->buf[HD_F_LEN]; i++)
        {
            printf("%02X ", (uint16_t)pRx->buf[i]);
        }
        printf("\n");
    }

    switch(pRx->buf[HD_F_TYPE])
    {
        case HD_TYPE_STATE_REQ:
            Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
            arg_cnt = HD_F_ARG_1;
            switch(pRx->buf[HD_F_DEV])
            {
#if defined(_TOW_SIZE_LIGHT_MODEL_) || defined(_TOW_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_ONE_SIZE_LIGHT_1_) || defined(_ONE_SIZE_LIGHT_2_) || defined(_ONE_SIZE_LIGHT_3_) || defined(_ONE_SIZE_LIGHT_2_WAKEUP_n_SLEEP_)
                case HD_DEV_LIGHT:      //전등 스위치
                    if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)      //전원
                    {
                        if(Loc_MSB == SWITCH_NUM)        //1번 스위치
                        {
                            switch(Loc_LSB)
                            {
                                case HD_LOC_CIRCUIT_1:
                                case HD_LOC_CIRCUIT_2:
                                case HD_LOC_CIRCUIT_3:
                                case HD_LOC_CIRCUIT_4:
                                case HD_LOC_CIRCUIT_5:
                                case HD_LOC_CIRCUIT_6:
                                    item = Loc_LSB;
                                    if(GET_Switch_State(item2tsn(item)))                        pTx->buf[arg_cnt] = HD_CMD_ON;
                                    else                                                        pTx->buf[arg_cnt] = HD_CMD_OFF;
                                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                    pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                    pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                    pTx->count = HD_T_MAX_11;  
                                    break;                                       
                                case HD_LOC_CIRCUIT_ALL:
                                    light_cnt = (uint8_t)(pG_Config->LightCount + pG_Config->DimmingCount);
                                    // for(item = mapping_ITEM_LIGHT_1; item <= pG_Config->LightCount; item++)
                                    for(item = mapping_ITEM_LIGHT_1; item <= mapping_ITEM_LIGHT_6; item++)
                                    {
                                        if(GET_Switch_State(item2tsn(item)))                pTx->buf[arg_cnt] = HD_CMD_ON;
                                        else                                                pTx->buf[arg_cnt] = HD_CMD_OFF;
                                        arg_cnt++;
                                    }
                                    crc_cnt += light_cnt;
                                    etx_cnt += light_cnt;
                                    max_cnt += light_cnt;
                                    pTx->buf[HD_F_LEN]      += (uint8_t)(light_cnt - 1);
                                    pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                    pTx->buf[crc_cnt]	    = HYUNDAI_Crc(pTx, crc_cnt);
                                    pTx->buf[etx_cnt]	    = HYUNDAI_ETX;
                                    pTx->count              = max_cnt;
                                    break;
                            }
                        }
                    }
                    break;
#endif
#if defined(_TOW_SIZE_LIGHT_n_ELEC_MODEL_)
                case HD_DEV_LIGHT_DIMMING:      //디밍스위치 0x1A
                    switch(pRx->buf[HD_F_SRV])
                    {
                        case HD_SRV_POWER:
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                switch(Loc_LSB)
                                {
                                    case HD_LOC_CIRCUIT_1:
                                        if(pG_State->Dimming_Level.Dimming1)    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                        else                                    pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]    = HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]   = HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_11;
                                        break;                                    
                                    case HD_LOC_CIRCUIT_2:
                                        if(pG_State->Dimming_Level.Dimming2)    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                        else                                    pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]    = HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]   = HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_11;
                                        break;
                                    case HD_LOC_CIRCUIT_ALL:
                                        dim_cnt = (uint8_t)pG_Config->DimmingCount;
                                        if(pG_Config->DimmingCount == 1)
                                        {
                                            if(pG_State->Dimming_Level.Dimming1)    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                            else                                    pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                        }
                                        else if(pG_Config->DimmingCount == 2)
                                        {
                                            if(pG_State->Dimming_Level.Dimming1)    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                            else                                    pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                            if(pG_State->Dimming_Level.Dimming2)    pTx->buf[HD_F_ARG_2] = HD_CMD_ON;
                                            else                                    pTx->buf[HD_F_ARG_2] = HD_CMD_OFF;
                                        }
                                        crc_cnt += dim_cnt;
                                        etx_cnt += dim_cnt;
                                        max_cnt += dim_cnt;
                                        pTx->buf[HD_F_LEN]      += (uint8_t)(dim_cnt - 1);
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[crc_cnt]	    = HYUNDAI_Crc(pTx, crc_cnt);
                                        pTx->buf[etx_cnt]	    = HYUNDAI_ETX;
                                        pTx->count              = max_cnt;
                                        break;
                                }
                            }
                            break;
                        case HD_SRV_DIMMING:        //0x42
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                switch(Loc_LSB)
                                {
                                    case HD_LOC_CIRCUIT_1:
                                        pTx->buf[HD_F_ARG_1]    = pG_State->Dimming_Level.Dimming1;
                                        // pTx->buf[HD_F_ARG_1]    = 1;        // 임시
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]    = HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]   = HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_11;                
                                        break;
                                    case HD_LOC_CIRCUIT_2:
                                        pTx->buf[HD_F_ARG_1]    = pG_State->Dimming_Level.Dimming2;
                                        // pTx->buf[HD_F_ARG_1]    = 2;        // 임시
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]    = HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]   = HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_11;                                        
                                        break;
                                    case HD_LOC_CIRCUIT_ALL:
                                        dim_cnt = (uint8_t)pG_Config->DimmingCount;
                                        if(pG_Config->DimmingCount == 1)
                                        {
                                            if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))
                                                if(pG_State->Dimming_Level.Dimming1 >= 8)                   pTx->buf[HD_F_ARG_1] = 7;
                                                else                                                        pTx->buf[HD_F_ARG_1] = pG_State->Dimming_Level.Dimming1;
                                            else                                                            pTx->buf[HD_F_ARG_1] = 0;
                                            crc_cnt += dim_cnt;
                                            etx_cnt += dim_cnt;
                                            max_cnt += dim_cnt;                                        
                                            pTx->buf[HD_F_LEN]      += (uint8_t)(dim_cnt - 1);
                                            pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                            pTx->buf[crc_cnt]	    = HYUNDAI_Crc(pTx, crc_cnt);
                                            pTx->buf[etx_cnt]	    = HYUNDAI_ETX;
                                            pTx->count              = max_cnt;                                            
                                        }
                                        else if(pG_Config->DimmingCount == 2)
                                        {
                                            if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_1)))    
                                                if(pG_State->Dimming_Level.Dimming1 >= 8)                   pTx->buf[HD_F_ARG_1] = 7;
                                                else                                                        pTx->buf[HD_F_ARG_1] = pG_State->Dimming_Level.Dimming1;
                                            else                                                            pTx->buf[HD_F_ARG_1] = 0;
                                            if(GET_Switch_State(item2tsn(mapping_ITEM_DIMMING_LIGHT_2)))
                                                if(pG_State->Dimming_Level.Dimming2 >= 8)                   pTx->buf[HD_F_ARG_2] = 7;
                                                else                                                        pTx->buf[HD_F_ARG_2] = pG_State->Dimming_Level.Dimming2;
                                            else                                                            pTx->buf[HD_F_ARG_2] = 0;
                                            crc_cnt += dim_cnt;
                                            etx_cnt += dim_cnt;
                                            max_cnt += dim_cnt;                                        
                                            pTx->buf[HD_F_LEN]      += (uint8_t)(dim_cnt - 1);
                                            pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                            pTx->buf[crc_cnt]	    = HYUNDAI_Crc(pTx, crc_cnt);
                                            pTx->buf[etx_cnt]	    = HYUNDAI_ETX;
                                            pTx->count              = max_cnt;                                            
                                        }
                                        else if(pG_Config->DimmingCount == 0)        //디밍 없는 모델로 테스트 중이므로 임의로 추가함.
                                        {
                                            if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_1)))            pTx->buf[HD_F_ARG_1] = pG_State->Dimming_Level.Dimming1;       //월패드 상 디밍 레벨은 7까지이므로 임의로 4
                                            else                                                            pTx->buf[HD_F_ARG_1] = 0;
                                            if(GET_Switch_State(item2tsn(mapping_ITEM_LIGHT_2)))            pTx->buf[HD_F_ARG_2] = pG_State->Dimming_Level.Dimming2;       //월패드 상 디밍 레벨은 7까지이므로 임의로 4
                                            else                                                            pTx->buf[HD_F_ARG_2] = 0;
                                            pTx->buf[HD_F_LEN]      += 1;
                                            pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                            pTx->buf[HD_T_CRC_10]	= HYUNDAI_Crc(pTx, HD_T_CRC_10);
                                            pTx->buf[HD_T_ETX_11]	= HYUNDAI_ETX;
                                            pTx->count              = HD_T_MAX_12;
                                        }                           
                                        break;
                                }
                            }
                            break;
                    }
                    break;

                case HD_DEV_ELETRICITY:
                    switch(pRx->buf[HD_F_SRV])
                    {
                        case HD_SRV_POWER:
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                switch(Loc_LSB)
                                {
                                    case HD_LOC_CIRCUIT_1:
                                        if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1)))  pTx->buf[HD_F_ARG_1]    = HD_CMD_ON;
                                        else                                                        pTx->buf[HD_F_ARG_1]    = HD_CMD_OFF;
                                        pTx->buf[HD_F_LEN]      = HD_T_MAX_18;
                                        pTx->buf[HD_F_ARG_2]    = Watt_1_MSB;
                                        pTx->buf[HD_F_ARG_3]    = Watt_1_LSB;
                                        pTx->buf[HD_F_ARG_4]    = 0xFF;
                                        pTx->buf[HD_F_ARG_5]    = 0xFF;
                                        pTx->buf[HD_F_ARG_6]    = 0xFF;
                                        pTx->buf[HD_F_ARG_7]    = 0xFF;
                                        pTx->buf[HD_F_ARG_8]    = pG_State->ETC.Auto1;
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_16]    = HYUNDAI_Crc(pTx, HD_T_CRC_16);
                                        pTx->buf[HD_T_ETX_17]   = HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_18;                                             
                                        break;
                                    case HD_LOC_CIRCUIT_2:
                                        if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2)))  pTx->buf[HD_F_ARG_1]    = HD_CMD_ON;
                                        else                                                        pTx->buf[HD_F_ARG_1]    = HD_CMD_OFF;
                                        pTx->buf[HD_F_LEN]      = HD_T_MAX_18;
                                        pTx->buf[HD_F_ARG_2]    = Watt_2_MSB;
                                        pTx->buf[HD_F_ARG_3]    = Watt_2_LSB;
                                        pTx->buf[HD_F_ARG_4]    = 0xFF;
                                        pTx->buf[HD_F_ARG_5]    = 0xFF;
                                        pTx->buf[HD_F_ARG_6]    = 0xFF;
                                        pTx->buf[HD_F_ARG_7]    = 0xFF;
                                        pTx->buf[HD_F_ARG_8]    = pG_State->ETC.Auto2;
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_16]    = HYUNDAI_Crc(pTx, HD_T_CRC_16);
                                        pTx->buf[HD_T_ETX_17]   = HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_18;
                                        break;                                
                                    case HD_LOC_CIRCUIT_ALL:
                                        // elec_cnt = (uint8_t)pG_Config->ElectricityCount;
                                        // if(pG_Config->ElectricityCount == 1)                                 //현재는 전열2 모델밖에 없음.
                                        // {
                                        //     if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_ALL)))    pTx->buf[HD_F_ARG_2] = HD_CMD_ON;
                                        //     else                                                            pTx->buf[HD_F_ARG_2] = HD_CMD_OFF;                                        
                                        // }
                                        if(pG_Config->ElectricityCount == 2)
                                        {
                                            if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1)))      pTx->buf[HD_F_ARG_2]  = HD_CMD_ON;
                                            else                                                            pTx->buf[HD_F_ARG_2]  = HD_CMD_OFF;
                                            if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2)))      pTx->buf[HD_F_ARG_11] = HD_CMD_ON;
                                            else                                                            pTx->buf[HD_F_ARG_11] = HD_CMD_OFF;                                        
                                        }
                                        // crc_cnt += elec_cnt;
                                        // etx_cnt += elec_cnt;
                                        // max_cnt += elec_cnt;
                                        pTx->buf[HD_F_ARG_1]  = (uint8_t)(pRx->buf[HD_F_LOC] + 1);
                                        pTx->buf[HD_F_ARG_3]  = Watt_1_MSB;
                                        pTx->buf[HD_F_ARG_4]  = Watt_1_LSB;
                                        pTx->buf[HD_F_ARG_5]  = 0xFF;
                                        pTx->buf[HD_F_ARG_6]  = 0xFF;
                                        pTx->buf[HD_F_ARG_7]  = 0xFF;
                                        pTx->buf[HD_F_ARG_8]  = 0xFF;
                                        pTx->buf[HD_F_ARG_9]  = pG_State->ETC.Auto1;
                                        
                                        pTx->buf[HD_F_ARG_10] = (uint8_t)(pRx->buf[HD_F_LOC] + 2);
                                        pTx->buf[HD_F_ARG_12] = Watt_2_MSB;
                                        pTx->buf[HD_F_ARG_13] = Watt_2_LSB;
                                        pTx->buf[HD_F_ARG_14] = 0xFF;
                                        pTx->buf[HD_F_ARG_15] = 0xFF;
                                        pTx->buf[HD_F_ARG_16] = 0xFF;
                                        pTx->buf[HD_F_ARG_17] = 0xFF;
                                        pTx->buf[HD_F_ARG_18] = pG_State->ETC.Auto2;
                                        // pTx->buf[HD_F_LEN]      = 12;
                                        pTx->buf[HD_F_LEN]    = HD_T_MAX_28;
                                        pTx->buf[HD_F_TYPE]   = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_26] = HYUNDAI_Crc(pTx, HD_T_CRC_26);
                                        pTx->buf[HD_T_ETX_27] = HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_28;
                                        break;
                                }
                            }                        
                            break;
                        /*
                        case HD_SRV_POWERSAVING_MODE:
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                switch(Loc_LSB)
                                {
                                    case HD_LOC_CIRCUIT_1:
                                        break;
                                    case HD_LOC_CIRCUIT_2:
                                        break;
                                    case HD_LOC_CIRCUIT_ALL:
                                        break;
                                }
                            }
                            break;
                        */
                    }
                    break;
#endif                    
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_)
                case HD_DEV_GAS:
                    if(pRx->buf[HD_F_SRV] == HD_SRV_OPEN_n_CLOSE)
                    {
                        if(Loc_MSB == SWITCH_NUM)
                        {
                            switch(Loc_LSB)
                            {
                                case HD_LOC_CIRCUIT_1:
                                case HD_LOC_CIRCUIT_2:
                                case HD_LOC_CIRCUIT_3:
                                case HD_LOC_CIRCUIT_4:
                                case HD_LOC_CIRCUIT_5:
                                case HD_LOC_CIRCUIT_6:
                                    if(Gu8_Gas_Flag)                                    pTx->buf[HD_F_ARG_1] = HD_CMD_CLOSE;
                                    else                                                pTx->buf[HD_F_ARG_1] = HD_CMD_OPEN;                                    
                                    break;
                            }
                            // if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))    pTx->buf[HD_F_ARG_1] = HD_CMD_CLOSE;
                            // else                                                pTx->buf[HD_F_ARG_1] = HD_CMD_OPEN;
                            
                        }
                    }
                    pTx->buf[HD_F_LEN]      = pRx->buf[HD_F_LEN];
                    pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                    pTx->buf[HD_T_CRC_9]    = HYUNDAI_Crc(pTx, HD_T_CRC_9);
                    pTx->buf[HD_T_ETX_10]   = HYUNDAI_ETX;
                    pTx->count              = HD_T_MAX_11;                          
                    break;
#endif
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_)
                case HD_DEV_USS:        //일괄 스위치 프로토콜
                    if(pRx->buf[HD_F_SRV] == HD_SRV_POWER)
                    {
                        if(pRx->buf[HD_F_ARG_1] == HD_DEV_LIGHT)                            //x019
                        {
                            if(pRx->buf[HD_F_ARG_3] == HD_DEV_GAS)                          //0x1B
                            {
                                if(pRx->buf[HD_F_ARG_5] == HD_DEV_HEATER)                   //0x18
                                {
                                    if(pRx->buf[HD_F_ARG_7] == HD_DEV_ELETRICITY)           //0x1F
                                    {
                                        if(pRx->buf[HD_F_ARG_9] == HD_DEV_OUTING)           //0x17
                                        {
                                            if(pRx->buf[HD_F_ARG_11] == HD_DEV_SECURITY)     //0x16
                                            {                                                // 전체 상태요청
                                                if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))    pTx->buf[HD_F_ARG_2] = HD_CMD_OFF;
                                                else                                                            pTx->buf[HD_F_ARG_2] = HD_CMD_ON;
                                                // if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))                pTx->buf[HD_F_ARG_4] = HD_CMD_CLOSE;
                                                // else                                                            pTx->buf[HD_F_ARG_4] = HD_CMD_OPEN;
                                                if(Gu8_Gas_Flag)                                                pTx->buf[HD_F_ARG_4] = HD_CMD_CLOSE;
                                                else                                                            pTx->buf[HD_F_ARG_4] = HD_CMD_OPEN;

                                                pTx->buf[HD_F_ARG_1]    = pRx->buf[HD_F_ARG_1];
                                                pTx->buf[HD_F_ARG_3]    = pRx->buf[HD_F_ARG_3];
                                                pTx->buf[HD_F_ARG_5]    = pRx->buf[HD_F_ARG_5];
                                                pTx->buf[HD_F_ARG_6]    = 0x00;
                                                pTx->buf[HD_F_ARG_7]    = pRx->buf[HD_F_ARG_7];
                                                pTx->buf[HD_F_ARG_8]    = 0x04;
                                                pTx->buf[HD_F_ARG_9]    = pRx->buf[HD_F_ARG_9];
                                                pTx->buf[HD_F_ARG_10]   = 0x00;
                                                pTx->buf[HD_F_ARG_11]   = pRx->buf[HD_F_ARG_11];
                                                pTx->buf[HD_F_ARG_12]   = 0x00;
                                                pTx->buf[HD_F_LEN]      = pRx->buf[HD_F_LEN];
                                                pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                                pTx->buf[HD_T_CRC_20]   = HYUNDAI_Crc(pTx, HD_T_CRC_20);
                                                pTx->buf[HD_T_ETX_21]   = HYUNDAI_ETX;
                                                pTx->count              = HD_T_MAX_22;                                             
                                            }
                                        }
                                    }
                                }
                            }
                            else        //일괄소등만 상태 요청 받았을 때
                            {
                                if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))    pTx->buf[HD_F_ARG_2] = HD_CMD_ON;
                                else                                                            pTx->buf[HD_F_ARG_2] = HD_CMD_OFF;
                                pTx->buf[HD_T_CRC_10] = HYUNDAI_Crc(pTx, HD_T_CRC_10);
                                pTx->buf[HD_T_ETX_11] = HYUNDAI_ETX;
                                pTx->count            = HD_T_MAX_12;
                            }
                        }
                        else if(pRx->buf[HD_F_ARG_1] == HD_DEV_GAS)     //일괄가스만 상태 요청 받았을 때
                        {
                            if(pRx->buf[HD_F_LOC] == 0x10)
                            {                            
                                // if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))                pTx->buf[HD_F_ARG_2] = HD_CMD_CLOSE;
                                // else                                                            pTx->buf[HD_F_ARG_2] = HD_CMD_OPEN;
                                if(Gu8_Gas_Flag)                                                pTx->buf[HD_F_ARG_2] = HD_CMD_CLOSE;
                                else                                                            pTx->buf[HD_F_ARG_2] = HD_CMD_OPEN;
                                pTx->buf[HD_T_CRC_10] = HYUNDAI_Crc(pTx, HD_T_CRC_10);
                                pTx->buf[HD_T_ETX_11] = HYUNDAI_ETX;
                                pTx->count            = HD_T_MAX_12;
                            }
                        }
                        else if(pRx->buf[HD_F_ARG_1] == HD_DEV_USS)     //일괄 차단 상태 요청
                        {
                            if(pRx->buf[HD_F_LOC] == 0x10 || 0x20)
                            {
                                pTx->buf[HD_F_ARG_1] = pRx->buf[HD_F_ARG_1];
                                if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) && (Gu8_Gas_Flag))  pTx->buf[HD_F_ARG_2] = HD_CMD_ON;       //모두 차단인 경우에만 차단(0x01), 그 외에는 차단해제(0x02)
                                else                                                                            pTx->buf[HD_F_ARG_2] = HD_CMD_OFF;

                                pTx->buf[HD_T_CRC_10] = HYUNDAI_Crc(pTx, HD_T_CRC_10);
                                pTx->buf[HD_T_ETX_11] = HYUNDAI_ETX;
                                pTx->count            = HD_T_MAX_12;                                
                            }
                        }
                    }
                    break;
                case HD_DEV_ELEVATOR_CALL:
                    if(pRx->buf[HD_F_SRV] == HD_SRV_ELEVATOR_UP_n_DOWN)
                    {
                        if(Loc_MSB == SWITCH_NUM)
                        {
                            switch(Loc_LSB)
                            {
                                case HD_LOC_CIRCUIT_1:
                                case HD_LOC_CIRCUIT_2:
                                case HD_LOC_CIRCUIT_ALL:
                                    if(GET_Switch_State(item2tsn(mapping_ITEM_ELEVATOR)))       pTx->buf[HD_F_ARG_1] = HD_ARG_CALL_DOWN;        //단방향 호출의 경우 하향호출이 기본
                                    else                                                        pTx->buf[HD_F_ARG_1] = HD_ARG_NOMAL;

                                    if(Gu8_ELEVATOR_Arrive_Flag == ELEVATOR_NON)                pTx->buf[HD_F_ARG_1] = HD_ARG_NOMAL;
                                    else if(Gu8_ELEVATOR_Arrive_Flag == ELEVATOR_CALL)          pTx->buf[HD_F_ARG_1] = HD_ARG_CALL;
                                    else if(Gu8_ELEVATOR_Arrive_Flag == ELEVATOR_ARRIVE)
                                    {
                                        pTx->buf[HD_F_ARG_1] = HD_ARG_ARRIVE;
                                        Gu8_ELEVATOR_Arrive_Flag = ELEVATOR_ARRIVE;
                                    }
                                    // else if(Gu8_ELEVATOR_Arrive_Flag == ELEVATOR_CANCEL)        pTx->buf[HD_F_ARG_1] = //호출취소 없음                                       
                                    else                                                        pTx->buf[HD_F_ARG_1] = HD_ARG_ERR;
                                    // pRx->buf[HD_F_ARG_1]        //엘리베이터 상태                                    
                                    // pRx->buf[HD_F_ARG_2]        //엘리베이터 현재 층
                                    // pRx->buf[HD_F_ARG_3]        //엘리베이터 호기                                    
                                    
                                    Gu8_WallPad_Elevetor_Call = pRx->buf[HD_F_ARG_1];
                                    pTx->buf[HD_F_LEN]  = HD_T_MAX_11;
                                    pTx->buf[HD_F_TYPE] = HD_TYPE_RES_SUCCESS;
                                    pTx->buf[HD_T_CRC_9] = HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                    pTx->buf[HD_T_ETX_10] = HYUNDAI_ETX;
                                    pTx->count            = HD_T_MAX_11;                                      
                                    break;
                            }
                        }
                    }
                    break;
#endif
                // break;
            }
            break;
        case HD_TYPE_CONTROL_REQ:
            Gu8_RS_485_Tx_Tmr		= pG_Config->Protocol_RES_DelayTime;
            arg_cnt = HD_F_ARG_1;
            switch(pRx->buf[HD_F_DEV])
            {
#if defined(_TOW_SIZE_LIGHT_MODEL_) || defined(_TOW_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_ONE_SIZE_LIGHT_1_) || defined(_ONE_SIZE_LIGHT_2_) || defined(_ONE_SIZE_LIGHT_3_) || defined(_ONE_SIZE_LIGHT_2_WAKEUP_n_SLEEP_)                
                case HD_DEV_LIGHT:
                    switch(pRx->buf[HD_F_SRV])
                    {
                        case HD_SRV_POWER:
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                switch(Loc_LSB)
                                {
                                    case HD_LOC_CIRCUIT_1:
                                    case HD_LOC_CIRCUIT_2:
                                    case HD_LOC_CIRCUIT_3:
                                    case HD_LOC_CIRCUIT_4:
                                    case HD_LOC_CIRCUIT_5:
                                    case HD_LOC_CIRCUIT_6:
                                        item = Loc_LSB;
                                        if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                        {
                                            touch_switch = item2tsn((uint8_t)item);
                                            Flag = SET_Switch_State(touch_switch, ON);
                                            SET_LED_State(touch_switch, Flag);
                                            Beep(Flag);
                                            PUT_RelayCtrl(item2ctrl((uint8_t)item), Flag);
                                            // EventCtrl(item2tsn(item), ON); 
                                            pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                        }
                                        else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                        {   
                                            touch_switch = item2tsn((uint8_t)item);
                                            Flag = SET_Switch_State(touch_switch, OFF);
                                            SET_LED_State(touch_switch, Flag);
                                            Beep(Flag);
                                            PUT_RelayCtrl(item2ctrl((uint8_t)item), Flag);                                            
                                            // EventCtrl(item2tsn(item), OFF);
                                            pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                        }
                                        ALL_n_Group_Light_Switch_LED_Ctrl();
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                        pTx->count			    = HD_T_MAX_11;                                        
                                        break;
                                    case HD_LOC_CIRCUIT_ALL:
                                        if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                        {
                                            // EventCtrl(item2tsn(mapping_ITEM_LIGHT_ALL), ON);     //지연소등
                                            // pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                            for(i = mapping_ITEM_LIGHT_1; i <= mapping_ITEM_LIGHT_6; i++)         //지연소등때문에 하나씩 제어
                                            {
                                                if(GET_Switch_State(item2tsn((uint8_t)i)) == 0)
                                                {
                                                    touch_switch = item2tsn((uint8_t)i);
                                                    Flag = SET_Switch_State(touch_switch, ON);
                                                    SET_LED_State(touch_switch, Flag);
                                                    Beep(Flag);
                                                    PUT_RelayCtrl(item2ctrl((uint8_t)i), Flag);
                                                }
                                            }
                                            ALL_n_Group_Light_Switch_LED_Ctrl();
                                        }
                                        else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                        {   
                                            // EventCtrl(item2tsn(mapping_ITEM_LIGHT_ALL), OFF);
                                            // pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                            for(i = mapping_ITEM_LIGHT_1; i <= mapping_ITEM_LIGHT_6; i++)         //지연소등때문에 하나씩 제어
                                            {
                                                if(GET_Switch_State(item2tsn((uint8_t)i)))
                                                {
                                                    touch_switch = item2tsn((uint8_t)i);
                                                    Flag = SET_Switch_State(touch_switch, OFF);
                                                    SET_LED_State(touch_switch, Flag);
                                                    Beep(Flag);
                                                    PUT_RelayCtrl(item2ctrl((uint8_t)i), Flag);
                                                }
                                            }
                                            ALL_n_Group_Light_Switch_LED_Ctrl();
                                        }
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                        pTx->count			    = HD_T_MAX_11;                                        
                                        break;                                    
                                }
                            }
                            break;
                        case HD_SRV_BATCHLIGHT:
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                if(Loc_LSB == HD_LOC_CIRCUIT_ALL)   //일괄제어
                                {
                                    if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                    {
                                        light_cnt = (uint8_t)pG_Config->LightCount;
                                        for(i = mapping_ITEM_LIGHT_1; i <= pG_Config->LightCount; i++)
                                        {
                                            res = GET_Switch_State(item2tsn(item));
                                            if(res == 0x01)         pTx->buf[arg_cnt] = HD_CMD_ON;
                                            else if(res == 0x00)    pTx->buf[arg_cnt] = HD_CMD_OFF;
                                            touch_switch =item2tsn((uint8_t)i);
                                            Flag = SET_Switch_State(touch_switch, ON);
                                            Beep(Flag);
                                            PUT_RelayCtrl(item2ctrl((uint8_t)i), Flag);
                                            arg_cnt++;
                                        }
                                        // EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), ON);
                                        crc_cnt += light_cnt;
                                        etx_cnt += light_cnt;
                                        max_cnt += light_cnt;
                                        pTx->buf[HD_F_LEN]      += (uint8_t)(light_cnt - 1);
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[crc_cnt]	    = HYUNDAI_Crc(pTx, crc_cnt);
                                        pTx->buf[etx_cnt]	    = HYUNDAI_ETX;
                                        pTx->count              = max_cnt;                                        
                                    }
                                    else if(pRx->buf[HD_F_CMD == HD_CMD_OFF])
                                    {
                                        light_cnt = (uint8_t)pG_Config->LightCount;
                                        for(item = mapping_ITEM_LIGHT_1; item <= pG_Config->LightCount; item++)
                                        {
                                            res = GET_Switch_State(item2tsn(item));
                                            if(res == 0x01)     pTx->buf[arg_cnt] = HD_CMD_ON;
                                            else if(res == 0x00)  pTx->buf[arg_cnt] = HD_CMD_OFF;
                                            touch_switch =item2tsn((uint8_t)i);
                                            Flag = SET_Switch_State(touch_switch, OFF);
                                            Beep(Flag);
                                            PUT_RelayCtrl(item2ctrl((uint8_t)i), Flag);                                            
                                            arg_cnt++;
                                        }
                                        // EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), OFF);
                                        crc_cnt += light_cnt;
                                        etx_cnt += light_cnt;
                                        max_cnt += light_cnt;
                                        pTx->buf[HD_F_LEN]      += (uint8_t)(light_cnt - 1);
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[crc_cnt]	    = HYUNDAI_Crc(pTx, crc_cnt);
                                        pTx->buf[etx_cnt]	    = HYUNDAI_ETX;
                                        pTx->count              = max_cnt;     
                                    }
                                }
                            }
                            break;
                    }
                    break;
#endif
#if defined(_TOW_SIZE_LIGHT_n_ELEC_MODEL_)
                case HD_DEV_LIGHT_DIMMING:
                    switch(pRx->buf[HD_F_SRV])
                    {
                        case HD_SRV_POWER:
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                switch(Loc_LSB)
                                {
                                    case HD_LOC_CIRCUIT_1:
                                    case HD_LOC_CIRCUIT_2:
                                        // item = (uint8_t)(Loc_LSB + 9);     //LSB + 9 = 10(DIMMING_LIGHT_1) or 11(DIMMING_LIGHT_2)
                                        item = (uint8_t)(Loc_LSB);      //디밍 없는 모델로 테스트 때문에 임시로 세팅
                                        if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                        {
                                            EventCtrl(item2tsn(item), ON);
                                            pTx->buf[HD_F_ARG_1] = HD_CMD_ON;  
                                        }
                                        else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                        {   
                                            EventCtrl(item2tsn(item), OFF);
                                            pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                        }
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                        pTx->count			    = HD_T_MAX_11;                                        
                                        break;
                                    case HD_LOC_CIRCUIT_ALL:
                                        if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                        {
                                            // for(item = mapping_ITEM_DIMMING_LIGHT_1; item <= mapping_ITEM_DIMMING_LIGHT_2; item++)
                                            for(item = mapping_ITEM_LIGHT_1; item <= mapping_ITEM_LIGHT_2; item++)
                                            {
                                                if(GET_Switch_State(item2tsn(item)) == 0)
                                                {
                                                    EventCtrl(item2tsn(item), ON);
                                                }
                                                pTx->buf[arg_cnt] = HD_CMD_ON;
                                                arg_cnt++;
                                            }
                                        }
                                        else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                                        {
                                            for(item = mapping_ITEM_LIGHT_1; item <= mapping_ITEM_LIGHT_2; item++)
                                            {
                                                if(GET_Switch_State(item2tsn(item)))
                                                {
                                                    EventCtrl(item2tsn(item), OFF);
                                                }
                                                pTx->buf[arg_cnt] = HD_CMD_OFF;
                                                arg_cnt++;
                                            }                                            
                                        }
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_10]	= HYUNDAI_Crc(pTx, HD_T_CRC_10);
                                        pTx->buf[HD_T_ETX_11]	= HYUNDAI_ETX;
                                        pTx->count			    = HD_T_MAX_12;                                        
                                        break;
                                }
                            }
                            break;
                        case HD_SRV_DIMMING:
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                switch(Loc_LSB)
                                {
                                    case HD_LOC_CIRCUIT_1:
                                        pG_State->Dimming_Level.Dimming1 = pRx->buf[HD_F_CMD];
                                        PUT_PWMCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_1),ON);
                                        pTx->buf[HD_F_ARG_1]    = pG_State->Dimming_Level.Dimming1;
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_11;                                             
                                        break;
                                    case HD_LOC_CIRCUIT_2:
                                        pG_State->Dimming_Level.Dimming2 = pRx->buf[HD_F_CMD];
                                        PUT_PWMCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_2),ON);
                                        pTx->buf[HD_F_ARG_1]    = pG_State->Dimming_Level.Dimming2;
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_11;                                         
                                        break;
                                    case HD_LOC_CIRCUIT_ALL:
                                        dim_cnt = (uint8_t)pG_Config->DimmingCount;
                                        if(dim_cnt == 1)
                                        {
                                            pG_State->Dimming_Level.Dimming1 = pRx->buf[HD_F_CMD];
                                            PUT_PWMCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_1),ON);
                                            pTx->buf[HD_F_ARG_1] = pG_State->Dimming_Level.Dimming1;
                                        }
                                        else if(dim_cnt == 2)
                                        {
                                            pG_State->Dimming_Level.Dimming1 = pRx->buf[HD_F_CMD];
                                            PUT_PWMCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_1),ON);
                                            pTx->buf[HD_F_ARG_1] = pG_State->Dimming_Level.Dimming1;
                                            
                                            pG_State->Dimming_Level.Dimming2 = pRx->buf[HD_F_CMD];
                                            PUT_PWMCtrl(item2tsn(mapping_ITEM_DIMMING_LIGHT_2),ON);
                                            pTx->buf[HD_F_ARG_2] = pG_State->Dimming_Level.Dimming2;
                                        }
                                        crc_cnt += dim_cnt;
                                        etx_cnt += dim_cnt;
                                        max_cnt += dim_cnt;
                                        pTx->buf[HD_F_LEN]      += (uint8_t)(dim_cnt - 1);
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[crc_cnt]	    = HYUNDAI_Crc(pTx, crc_cnt);
                                        pTx->buf[etx_cnt]	    = HYUNDAI_ETX;
                                        pTx->count              = max_cnt;                                        
                                        break;
                                }
                            }
                            break;
                        case HD_SRV_BATCHLIGHT:
                            break;
                    }
                    break;

                case HD_DEV_ELETRICITY:
                    switch(pRx->buf[HD_F_SRV])
                    {
                        case HD_SRV_POWER:
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                switch(Loc_LSB)
                                {
                                    case HD_LOC_CIRCUIT_1:
                                    case HD_LOC_CIRCUIT_2:
                                        item = (uint8_t)(Loc_LSB + 11);     //mapping_ITEM_ELECTRICITY_1 == 12
                                        if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                                        {
                                            if(GET_Switch_State(item2tsn(item)) == 0)
                                            {
                                                touch_switch = item2tsn(item);
                                                Flag = SET_Switch_State(touch_switch, ON);
                                                SET_LED_State(touch_switch, Flag);
                                                Beep(Flag);
                                                PUT_RelayCtrl(item2ctrl(item), Flag);
                                                pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                            }
                                        }
                                        else
                                        {   
                                            if(GET_Switch_State(item2tsn(item)))
                                            {
                                                touch_switch = item2tsn(item);
                                                Flag = SET_Switch_State(touch_switch, OFF);
                                                SET_LED_State(touch_switch, Flag);
                                                Beep(Flag);
                                                PUT_RelayCtrl(item2ctrl(item), Flag);
                                                pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                            }
                                        }
                                        ALL_Electricity_Switch_LED_Ctrl();
                                        pTx->buf[HD_F_LEN]      = pRx->buf[HD_F_LEN];
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_11;                                    
                                        break;
                                    case HD_LOC_CIRCUIT_ALL:
                                        if((pRx->buf[HD_F_ARG_1] == (uint8_t)(pRx->buf[HD_F_LOC] + 1)) && (pRx->buf[HD_F_ARG_3] == (uint8_t)(pRx->buf[HD_F_LOC] + 2)))
                                        {
                                            if(pRx->buf[HD_F_ARG_2] == HD_CMD_ON)           Flag_arr[0] = 1;
                                            else if(pRx->buf[HD_F_ARG_2] == HD_CMD_OFF)     Flag_arr[0] = 0;
                                            if(pRx->buf[HD_F_ARG_4] == HD_CMD_ON)           Flag_arr[1] = 1;
                                            else if(pRx->buf[HD_F_ARG_4] == HD_CMD_OFF)     Flag_arr[1] = 0;
                                            
                                            for(i = mapping_ITEM_ELECTRICITY_1; i <= mapping_ITEM_ELECTRICITY_2; i++)
                                            {
                                                if(GET_Switch_State(item2tsn((uint8_t)i)))
                                                {
                                                    touch_switch = item2tsn((uint8_t)i);
                                                    Flag = SET_Switch_State(touch_switch, Flag_arr[i-mapping_ITEM_ELECTRICITY_1]);
                                                    SET_LED_State(touch_switch, Flag);
                                                    Beep(Flag);
                                                    PUT_RelayCtrl(item2ctrl((uint8_t)i), Flag);
                                                }
                                            }
                                            ALL_Electricity_Switch_LED_Ctrl();
                                            if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1)))      pTx->buf[HD_F_ARG_2] = HD_CMD_ON;
                                            else                                                            pTx->buf[HD_F_ARG_2] = HD_CMD_OFF;                                            
                                            if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2)))      pTx->buf[HD_F_ARG_4] = HD_CMD_ON;
                                            else                                                            pTx->buf[HD_F_ARG_4] = HD_CMD_OFF;   
                                        }
                                        pTx->buf[HD_F_LEN]      = pRx->buf[HD_F_LEN];
                                        pTx->buf[HD_F_ARG_1]    = pRx->buf[HD_F_ARG_1];
                                        pTx->buf[HD_F_ARG_3]    = pRx->buf[HD_F_ARG_3];
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_12]	= HYUNDAI_Crc(pTx, HD_T_CRC_12);
                                        pTx->buf[HD_T_ETX_13]	= HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_14;                                      
                                        break;
                                }
                            }
                            break;
                        case HD_SRV_POWERSAVING_MODE:
                            if(Loc_MSB == SWITCH_NUM)
                            {
                                switch(Loc_LSB)
                                {
                                    case HD_LOC_CIRCUIT_1:
                                        if(pRx->buf[HD_F_CMD] == HD_CMD_ON)             pG_State->ETC.Auto1 = 1;
                                        else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)       pG_State->ETC.Auto1 = 0;
                                        
                                        if(pG_State->ETC.Auto1 == 1)                    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                        else if(pG_State->ETC.Auto1 == 0)               pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                        pTx->buf[HD_F_LEN]      = pRx->buf[HD_F_LEN];
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]    = HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]   = HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_11;
                                        break;                                    
                                    case HD_LOC_CIRCUIT_2:
                                        if(pRx->buf[HD_F_CMD] == HD_CMD_ON)             pG_State->ETC.Auto2 = 1;
                                        else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)       pG_State->ETC.Auto2 = 0;
                                        
                                        if(pG_State->ETC.Auto1 == 1)                    pTx->buf[HD_F_ARG_1] = HD_CMD_ON;
                                        else if(pG_State->ETC.Auto1 == 0)               pTx->buf[HD_F_ARG_1] = HD_CMD_OFF;
                                        pTx->buf[HD_F_LEN]      = pRx->buf[HD_F_LEN];
                                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                        pTx->buf[HD_T_CRC_9]    = HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                        pTx->buf[HD_T_ETX_10]   = HYUNDAI_ETX;
                                        pTx->count              = HD_T_MAX_11;                                        
                                        break;
                                    case HD_LOC_CIRCUIT_ALL:
                                        break;
                                }
                            }
                            break;
                    }
                    break;
#endif
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_)
                case HD_DEV_GAS:
                    if(pRx->buf[HD_F_SRV] == HD_SRV_OPEN_n_CLOSE)
                    {
                        if((Loc_MSB == SWITCH_NUM) && (Loc_LSB == SWITCH_CIRCUIT_NUM))
                        {
                            if(pRx->buf[HD_F_CMD] == HD_CMD_CLOSE)                          //가스 차단 명령
                            {
                                if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)) == 0)       //가스 차단 아니면
                                {
                                    EventCtrl(item2tsn(mapping_ITEM_GAS), ON);              //가스 차단 실행 후 
                                    if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))
                                    {    
                                        pTx->buf[HD_F_ARG_1] = HD_CMD_CLOSE;        //차단됐으면, CLOSE 보냄
                                        pTx->buf[HD_F_CMD]      = pRx->buf[HD_F_CMD];
                                    }
                                    else
                                    {
                                        pRx->buf[HD_F_ARG_1] = HD_CMD_OPEN;
                                        pTx->buf[HD_F_CMD]      = 0x00;
                                    }
                                }
                            }
                            // else if(pRx->buf[HD_F_CMD] == HD_CMD_OPEN)                      //가스 해제 명령
                            // {
                            //     if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))            //가스 차단이면
                            //     {
                            //         EventCtrl(item2tsn(mapping_ITEM_GAS), OFF);             //가스 해제 실행 후
                            //         if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)) == 0)   pTx->buf[HD_F_ARG_1] = HD_CMD_OPEN;     //해제됐으면, OPEN 보냄
                            //         else                                                    pTx->buf[HD_F_ARG_1] = HD_CMD_CLOSE;
                            //     }
                            // } //현재 전자식 스위치에 가스 해제 없음
                        }
                        
                        pTx->buf[HD_F_LEN]      = pRx->buf[HD_F_LEN];
                        pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                        pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, crc_cnt);
                        pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                        pTx->count              = HD_T_MAX_11;                        
                    }
                    break;
#endif
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_) || defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_) || defined(_ONE_SIZE_BATCH_LIGHT_)
                case HD_DEV_USS:
                    if(pRx->buf[HD_F_ARG_1] == HD_DEV_LIGHT)
                    {
                        if(pRx->buf[HD_F_CMD] == HD_CMD_ON)
                        {
                            touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
                            Flag = SET_Switch_State(touch_switch, OFF);
                            SET_LED_State(touch_switch, Flag);
                            Beep(Flag);
                            PUT_RelayCtrl(item2ctrl(mapping_ITEM_BATCH_LIGHT_OFF), Flag);
                        }
                        else if(pRx->buf[HD_F_CMD] == HD_CMD_OFF)
                        {
                            touch_switch = item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
                            Flag = SET_Switch_State(touch_switch, ON);
                            SET_LED_State(touch_switch, Flag);
                            Beep(Flag);
                            PUT_RelayCtrl(item2ctrl(mapping_ITEM_BATCH_LIGHT_OFF), Flag);                            
                        }
                        // EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), ON);
                        if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)))        pTx->buf[HD_F_ARG_2] = HD_CMD_OFF;
                        else                                                                pTx->buf[HD_F_ARG_2] = HD_CMD_ON;
                    }
                    else if(pRx->buf[HD_F_ARG_1] == HD_DEV_GAS)
                    {
                        EventCtrl(item2tsn(mapping_ITEM_GAS), ON);
                        // if(GET_Switch_State(item2tsn(mapping_ITEM_GAS)))                    pTx->buf[HD_F_ARG_2] = HD_CMD_CLOSE;
                        // else                                                                pTx->buf[HD_F_ARG_2] = HD_CMD_OPEN;
                        if(Gu8_Gas_Flag)                                                    pTx->buf[HD_F_ARG_2] = HD_CMD_CLOSE;
                        else                                                                pTx->buf[HD_F_ARG_2] = HD_CMD_OPEN;
                    }
                    else if(pRx->buf[HD_F_ARG_1] == HD_DEV_STANDBY_POWER)
                    {
                        ;
                    }
                    else if(pRx->buf[HD_F_ARG_1] == HD_DEV_ELETRICITY)
                    {
                        ;
                    }
                    else if(pRx->buf[HD_F_ARG_1] == HD_DEV_USS)
                    {
                        pTx->buf[HD_F_ARG_2] = HD_CMD_ON;
                    }
                    pTx->buf[HD_F_ARG_1]    = pRx->buf[HD_F_ARG_1];
                    // pTx->buf[HD_F_LEN]      = pRx->buf[HD_F_LEN];
                    pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                    pTx->buf[HD_T_CRC_10]	= HYUNDAI_Crc(pTx, HD_T_CRC_10);
                    pTx->buf[HD_T_ETX_11]	= HYUNDAI_ETX;
                    pTx->count              = HD_T_MAX_12;
                    break;

#endif
#if defined(_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_)
                case HD_DEV_ELEVATOR_CALL:
                    if(pRx->buf[HD_F_SRV] == HD_SRV_ELEVATOR_UP_n_DOWN)
                    {
                        if(Loc_MSB == SWITCH_NUM)
                        {
                            switch(Loc_LSB)
                            {
                                case HD_LOC_CIRCUIT_1:
                                case HD_LOC_CIRCUIT_2:
                                case HD_LOC_CIRCUIT_ALL:
                                    touch_switch = item2tsn(mapping_ITEM_ELEVATOR);
                                    Flag = SET_Switch_State(touch_switch, ON);
                                    SET_LED_State(touch_switch, Flag);
                                    Beep(Flag);
                                    
                                    Gu8_WallPad_Elevetor_Call = pRx->buf[HD_F_CMD];
                                    ELEVATOR_Call();
                                    pTx->buf[HD_F_CMD] = pRx->buf[HD_F_CMD];
                                    pTx->buf[HD_F_ARG_1] = pRx->buf[HD_F_CMD];

                                    pTx->buf[HD_F_TYPE]     = HD_TYPE_RES_SUCCESS;
                                    pTx->buf[HD_T_CRC_9]	= HYUNDAI_Crc(pTx, HD_T_CRC_9);
                                    pTx->buf[HD_T_ETX_10]	= HYUNDAI_ETX;
                                    pTx->count              = HD_T_MAX_11;                                    
                                    break;
                            }
                        }
                    }
                    break;
#endif
            }
            break;
    }
}
            
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
            if(pRx->buf[HD_F_LEN] < 0x0B || pRx->buf[HD_F_LEN] > 0x16)  //LEN 0x0B ~ 0x10
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
        case 5:
            if((pRx->buf[HD_F_TYPE] == HD_TYPE_RES_SUCCESS))
            {
                pRx->count = 0;
            }
            break;
    }
    pRx->buf[pRx->count++] = data;

    if(pRx->count >= 29)
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
    else if(pRx->length.value == 0x11)   cnt = 17;
    else if(pRx->length.value == 0x12)   cnt = 18;
    else if(pRx->length.value == 0x13)   cnt = 19;
    else if(pRx->length.value == 0x14)   cnt = 20;
    else if(pRx->length.value == 0x15)   cnt = 21;
    else if(pRx->length.value == 0x16)   cnt = 22;
    else if(pRx->length.value == 0x17)   cnt = 23;
    else if(pRx->length.value == 0x18)   cnt = 24;
    else if(pRx->length.value == 0x19)   cnt = 25;
    else if(pRx->length.value == 0x1A)   cnt = 26;
    else if(pRx->length.value == 0x1B)   cnt = 27;
    else if(pRx->length.value == 0x1C)   cnt = 28;

    if(pRx->count == pRx->length.word + cnt)
    {
        // printf("\nRX : ");
        // for(i = 0; i <= 22; i++)
        // {
        //     printf("%X ", pRx->buf[i]);
        // }
        // printf("\n");
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
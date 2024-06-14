/************************************************************************************
	Project		: 전자식스위치
	File Name	: IR.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/
/*----------------------------------------------------------------------------------
제품과 IR리모컨이 가리키는 방향(리모컨이 제품쪽을 향함)이 맞다면 6m까지 신호를 받는다.(6m까지 테스트 해본 것)
회의실 입구(리모컨)에서 끝(제품)까지 데이터 수신 함.
제품과 리모컨 사이에 장애물이 있다면 데이터 수신이 어려움.
제품과 리모컨의 거리가 멀어질수록 리모컨 방향이 제품과 일치해야 한다.
제품과 리모컨이 가까운 경우 리모컨 방향에 영향을 덜 받으나, 정확성이 떨어지며, 동작을 하지않을 가능성이 크다.
----------------------------------------------------------------------------------*/
/*()는 사용 버튼
| 09  (01)  0A |
|(02) (03) (04)|
| 0B   0C   0D |
| 0E  (05)  0F |
|(06)  10  (07)|
| 11  (08)  12 |
| 13   14   15 |

네트워크 스위치는 전등 구수 상관없이 전등1(K03), 전등2(K05), 전등 전체(K08) 3버튼 리모컨 사용한다. 
-> 2023. 11. 22에서 변경된 사안으로는 전등 1구 스위치는 전등1(K05)을 사용함(현장에 따라 달라질 수 있음). 전등 2구 스위치는 전등1(K03), 전등2(K05), 전등 전체(K08)는 언급이 없어서 그대로 유지하도록 함.
취침 + 네트워크 스위치는 전등 구수 상관없이 취침(K03), 전등1(K06), 전등2(K07), 전등 전체(K08) 4버튼 리모컨을 사용한다.
대기전력 스위치는 전등 구수 상관 없이 전등1(K02), 전등2(K04), 전등 전체(K01), 콘센트1(K06), 콘센트2(K07), 콘센트 전체(K08) 6버튼 리모컨을 사용한다.
* 단, 현재 대기전력 스위치는 네트워크 스위치랑 동일하게 사용중. 대기전력 기능 리모컨 제어 필요 할 시에는 수정 필요함
* 공통점 : 스위치의 최대 스펙이 전등1구 일 경우 전등1과 전등 전체 버튼으로 전체1번 제어 *
*/
#include "header.h"
#include "eeprom.h"
#include "touch.h"
#include "el_switch.h"
#include "IR.h"
#include "debug.h"
#include "led.h"
#include "WDGnBeep.h"

#define     MAX_IR_BUFFER   32

#define     K_01    0x45BA01FE  //CUSTOM CODE : 0x45BA | DATA CODE : 0x01 | !DATA CODE : 0xFE, 0xA25D 807F
#define     K_02    0x45BA02FD  //CUSTOM CODE : 0x45BA | DATA CODE : 0x02 | !DATA CODE : 0xFD, 0xA25D 40BF
#define     K_03    0x45BA03FC  //CUSTOM CODE : 0x45BA | DATA CODE : 0x03 | !DATA CODE : 0xFC, 0xA25D C03F
#define     K_04    0x45BA04FB  //CUSTOM CODE : 0x45BA | DATA CODE : 0x04 | !DATA CODE : 0xFB, 0xA25D 20DF 
#define     K_05    0x45BA05FA  //CUSTOM CODE : 0x45BA | DATA CODE : 0x05 | !DATA CODE : 0xFA, 0XA25D A05F
#define     K_06    0x45BA06F9  //CUSTOM CODE : 0x45BA | DATA CODE : 0x06 | !DATA CODE : 0xF9, 0XA25D 609F
#define     K_07    0x45BA07F8  //CUSTOM CODE : 0x45BA | DATA CODE : 0x07 | !DATA CODE : 0xF8, 0XA25D E01F
#define     K_08    0x45BA08F7  //CUSTOM CODE : 0x45BA | DATA CODE : 0x08 | !DATA CODE : 0xF7, 0xA25D 10EF
#define     K_09    0x45BA09F6  //CUSTOM CODE : 0x45BA | DATA CODE : 0x09 | !DATA CODE : 0xF6, 0xA25D 906F
#define     K_0A    0x45BA0AF5  //CUSTOM CODE : 0x45BA | DATA CODE : 0x0A | !DATA CODE : 0xF5, 0xA25D 50AF
#define     K_0B    0x45BA0BF4  //CUSTOM CODE : 0x45BA | DATA CODE : 0x0B | !DATA CODE : 0xF4, 0xA25D D02F
#define     K_0C    0x45BA0CF3  //CUSTOM CODE : 0x45BA | DATA CODE : 0x0C | !DATA CODE : 0xF3, 0xA25D 30CF
#define     K_0D    0x45BA0DF2  //CUSTOM CODE : 0x45BA | DATA CODE : 0x0D | !DATA CODE : 0xF2, 0xA25D B04F
#define     K_0E    0x45BA0EF1  //CUSTOM CODE : 0x45BA | DATA CODE : 0x0E | !DATA CODE : 0xF1, 0xA25D 708F
#define     K_0F    0x45BA0FF0  //CUSTOM CODE : 0x45BA | DATA CODE : 0x0F | !DATA CODE : 0xF0, 0xA25D F00F
#define     K_10    0x45BA10EF  //CUSTOM CODE : 0x45BA | DATA CODE : 0x10 | !DATA CODE : 0xEF, 0xA25D 08F7
#define     K_11    0x45BA11EE  //CUSTOM CODE : 0x45BA | DATA CODE : 0x11 | !DATA CODE : 0xEE, 0xA25D 8877
#define     K_12    0x45BA12ED  //CUSTOM CODE : 0x45BA | DATA CODE : 0x12 | !DATA CODE : 0xED, 0xA25D 48B7
#define     K_13    0x45BA13EC  //CUSTOM CODE : 0x45BA | DATA CODE : 0x13 | !DATA CODE : 0xEC, 0xA25D C837
#define     K_14    0x45BA14EB  //CUSTOM CODE : 0x45BA | DATA CODE : 0x14 | !DATA CODE : 0xEB, 0xA25D 28D7
#define     K_15    0x45BA15EA  //CUSTOM CODE : 0x45BA | DATA CODE : 0x15 | !DATA CODE : 0xEA, 0xA25D A857

const uint32_t BIT_Tbl[]=
{
    0x80000000, 0x40000000, 0x20000000, 0x10000000,
    0x08000000, 0x04000000, 0x02000000, 0x01000000,
    0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000,
    0x00008000, 0x00004000, 0x00002000, 0x00001000,
    0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00000080, 0x00000040, 0x00000020, 0x00000010,
    0x00000008, 0x00000004, 0x00000002, 0x00000001
};

const uint16_t FREQ_80HZ    = 12500;    //1000/80 = 12.5
const uint16_t FREQ_70HZ    = 14285;    //1000/70 = 14.2857....
const uint16_t FREQ_1000HZ  = 1000;		//1100 ~ 1200으로 범위 변경
const uint16_t FREQ_300HZ   = 3333;		//2200 ~ 2300으로 범위 변경

volatile uint16_t capVal[32];   //20210122
uint8_t IR_RX_Ok    = 0;        //20210122
void IR_Elec_Control(uint8_t touch_switch, uint8_t Flag);
// volatile uint16_t capture = 0;       //20210122

// Timer 5 input capture interrupt service routine

/*16Mhz / CCR1 = Signal Hz, 16000000 / 18000    = 888.889Hz
1000 / Hz = ms,             1000 / 888.889      = 1.125ms

Get the Input Capture value by reading CCR1 register
CCR1 regsiter contains signal frequency value
SignalFrequency = (uint32_t) (CLK_GetClockFreq() / CCR1)

Get the Input Capture value by reading CCR2 register
CCR2 regsiter contains how much time the signal remained at high level
SignalDutyCycle = ((uint32_t) CCR2 * 100) / CCR1*/

void irq_IR(void)
{
    volatile uint16_t   capture         = 0;
    static int8_t       IR_cnt          = 0;
    const uint32_t      FREQ_12_5HZ     = 80000;
    const uint32_t      FREQ_7HZ        = 150000;
    
    if (TIM5_GetITStatus(TIM5_IT_CC1) != RESET)
    {
        TIM5_ClearITPendingBit(TIM5_IT_CC1);
        capture = TIM5_GetCapture1();
        // printf("%d\r\n", (uint16_t)capture);
        if(((capture >= FREQ_80HZ) && (capture <= FREQ_70HZ)) && (IR_cnt == 0))     //period 12.5 ~ 14.285  13.5ms -> lead code, transmission start
        {
            IR_cnt++;
        } 
        if(((capture >= FREQ_1000HZ) && (capture <= FREQ_300HZ)) && (IR_cnt >= 1))      //period 1 ~ 3.333  1.12 -> Bit "0", 2.24 -> Bit "1"
        // if((((capture >= 1100) && (capture <= 1200)) || ((capture >= 2200) && (capture <= 2300))) && (IR_cnt >= 1))      //period 1 ~ 3.333  1.12 -> Bit "0", 2.24 -> Bit "1"
        {
            capVal[IR_cnt-1] = capture;
            IR_cnt++;
            if(IR_cnt > MAX_IR_BUFFER)
            {
                IR_cnt      = 0;
                IR_RX_Ok    = 1;
            }
        }
        if((capture >= FREQ_12_5HZ) && (capture <= FREQ_7HZ))
        {
            IR_RX_Ok    = 1;        //tranmission complete
        }
    }
}
//-------------------------------------------------------------------------------------------------------------------------

void IR_Init(void)
{
    uint8_t icfilter    = 0;        //0 ~ F 
    uint16_t period     = 15000;    //13700 ~ 60000
    if(pG_Config->Enable_Flag.IR)
    {
        CLK_PeripheralClockConfig(CLK_Peripheral_TIM5, ENABLE);
        TIM5_TimeBaseInit(TIM5_Prescaler_16, TIM5_CounterMode_Up, period);      //20210120?煞?
        // TIM5_PWMIConfig(TIM5_Channel_1, TIM5_ICPolarity_Rising, TIM5_ICSelection_DirectTI, TIM5_ICPSC_DIV1, icfilter);
        TIM5_PWMIConfig(TIM5_Channel_1, TIM5_ICPolarity_Falling, TIM5_ICSelection_DirectTI, TIM5_ICPSC_DIV1, icfilter);     //Rising->Falling 20210120
        TIM5_SelectInputTrigger(TIM5_TRGSelection_TI1FP1);  // Select the TIM1 Input Trigger: TI1FP1
        TIM5_SelectSlaveMode(TIM5_SlaveMode_Reset);
        TIM5_ITConfig(TIM5_IT_CC1, ENABLE);                 // Enable CC1 interrupt request
        TIM5_Cmd(ENABLE);

        printf("IR Init\n");
    }
}

uint32_t Bit_Sort(uint32_t* IR_Data)        //ex)0x06FD10EF >> 0x60BF08F7 ??트 ?ㅇ?
{
    uint32_t    temp        = 0;
    uint32_t    output      = 0;
    uint8_t     IR_Data1;
    uint8_t     IR_Data2;
    uint8_t     IR_Data3;
    uint8_t     IR_Data4;
    int         i;

    // printf("Input_Data = 0x%lX, ", (uint32_t)*IR_Data);

    for(i = 0; i <= 31; i++)
    {
        temp = (*IR_Data >> i) & 1;
        output |= temp << (31 - i);
    }

    *IR_Data = output;
    
    IR_Data1 = (uint8_t)((*IR_Data & 0xFF000000) >> 24);
    IR_Data2 = (uint8_t)((*IR_Data & 0xFF0000) >> 16);
    IR_Data3 = (uint8_t)((*IR_Data & 0xFF00) >> 8);
    IR_Data4 = (uint8_t)((*IR_Data & 0x0FF));
    
    *IR_Data = ((uint32_t)(IR_Data4) << 24) | ((uint32_t)(IR_Data3) << 16) | ((uint32_t)(IR_Data2) << 8) | ((uint32_t)(IR_Data1));
    // printf("Sort_Data = 0x%lX, ", (uint32_t)*IR_Data);
    
    return *IR_Data;
}

void IR_Process(void)
{
    const uint16_t  FREQ_500HZ  = 2000;
    static uint32_t IR_Data     = 0;
    int             i           = 0;
    uint32_t        Data        = 0;
    uint8_t         item        = 0;
    uint8_t touch_switch        = 0;

    if(pG_Config->Enable_Flag.IR == 0)  return;

    if(IR_RX_Ok == 1)
    {
        for(i = 0; i < MAX_IR_BUFFER; i++)
        {
            if(capVal[i] >= FREQ_500HZ) //Bit "0"?? 1.12ms, Bit "1"?? 2.24ms 2000(2.0) ????????. ??, Bit "1"?? ???????? ?????? ????.
            {
                IR_Data |= BIT_Tbl[i];
            }
        }
    }
/*#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
    if(IR_RX_Ok)
    {
        Data = Bit_Sort(&IR_Data);
        switch(Data)
        {           
            case K_01:  //(Data : 0x01)
                if(pG_Config->LightCount > 1)
                {
                    Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
                    if(GET_Light_ON_Count() == pG_Config->LightCount)   //?????? ?? ???????만?
                    {
                        for(item = mapping_ITEM_LIGHT_1; item < (mapping_ITEM_LIGHT_1 + pG_Config->LightCount); item++)
                        {
                            EventCtrl(item2tsn(item), OFF);
                        }
                    }
                    else    //?????? ??나???? ????????나, 모?? 꺼?????만?
                    {
                        for(item = mapping_ITEM_LIGHT_1; item < (mapping_ITEM_LIGHT_1 + pG_Config->LightCount); item++)
                        {
                            EventCtrl(item2tsn(item), ON);
                        }                       
                    }
                }
                printf("K_01\r\n");
                break;
            case K_02:
            case K_04:
                if(pG_Config->LightCount > 1)
                {
                    Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
                    if(Data == K_02)        touch_switch = item2tsn(mapping_ITEM_LIGHT_1);
                    else if(Data == K_04)   touch_switch = item2tsn(mapping_ITEM_LIGHT_2);
                    EventCtrl(touch_switch, INVERSE);
                }
                if(Data == K_02)        printf("K_02\r\n");
                else if(Data == K_04)   printf("K_04\r\n");             
                break;
            case K_03:
                if(pG_Config->LightCount == 1)
                {
                    Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_1), INVERSE);
                }
                printf("K_03\r\n");
                break;
            case K_06:
            case K_07:
                if(Data == K_06)
                {
                    printf("K_06\r\n");
                    touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_1);
                }
                else if(Data == K_07)
                {
                    printf("K_07\r\n");
                    touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_2);
                }
                if(GET_LED_State(touch_switch) == LED_FLASHING)     //?娩? ??열?? ????OFF????때 리모?? ???????? ON?막?
                {
                    Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
                    // EventCtrl(touch_switch, ON);
                    IR_Elec_Control(touch_switch, ON);
                }
                else
                {
                    Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
                    // EventCtrl(touch_switch, INVERSE);
                    IR_Elec_Control(touch_switch, INVERSE);
                }
                break;
            case K_08:
                touch_switch = item2tsn(mapping_ITEM_ELECTRICITY_ALL);
                if(GET_LED_State(touch_switch) == LED_FLASHING)     //???? ??열 ????OFF????때 리모?? ???????? ON?막?
                {
                    Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
                    for(item = mapping_ITEM_ELECTRICITY_1; item <= mapping_ITEM_ELECTRICITY_2; item++)
                    {
                        touch_switch = item2tsn(item);
                        IR_Elec_Control(touch_switch, ON);
                    }
                }
                else
                {
                    Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
                    if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1)) == OFF || GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2)) == OFF)       //??열1, 2 ?? ??나???? 꺼?????만? 
                    {
                        for(item = mapping_ITEM_ELECTRICITY_1; item <= mapping_ITEM_ELECTRICITY_2; item++)
                        {
                            touch_switch = item2tsn(item);
                            IR_Elec_Control(touch_switch, ON);
                        }
                    }
                    else
                    {
                        if((GET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_1)) == LED_FLASHING) || (GET_LED_State(item2tsn(mapping_ITEM_ELECTRICITY_2)) == LED_FLASHING))  //??열1, 2 ?? ??나???? ????OFF??????
                        {
                            EventCtrl(item2tsn(mapping_ITEM_ELECTRICITY_ALL), ON);  //???? OFF ????
                        }
                        else
                        {
                            for(item = mapping_ITEM_ELECTRICITY_1; item <= mapping_ITEM_ELECTRICITY_2; item++)
                            {
                                touch_switch = item2tsn(item);
                                IR_Elec_Control(touch_switch, OFF);
                            }
                        }                        
                    }
                }
                printf("K_08\r\n");
                break;
            default:
                if(Data == K_05)        printf("K_05\r\n");
                printf("Vaule is out of range\r\n");
                break;
        }
        IR_Data = 0;
    }
    IR_RX_Ok = 0;*/
#if defined(_ONE_SIZE_LIGHT_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)  
//221102
    if(IR_RX_Ok)
    {
        Data = Bit_Sort(&IR_Data);
        switch(Data)
        {
#if defined _ONE_SIZE_LIGHT_1_n_SLEEP_ || defined _ONE_SIZE_LIGHT_2_n_SLEEP_
            case K_02:
				printf("K_02\r\n");
                break;
            // case K_04:
			case K_03:	//221026
                Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
                touch_switch = item2tsn(mapping_ITEM_SLEEP);
                if(GET_LED_State(touch_switch) == LED_FLASHING)
                {
                    Sleep_Level(touch_switch);
                }
                else
                {
                    if(GET_Switch_State(touch_switch) == 0) EventCtrl(touch_switch, ON);
                    else                                    EventCtrl(touch_switch, OFF);
                }
				printf("K_03\r\n");
                break;
            case K_06:
                /*if(pG_Config->LightCount > 1)
                {
                    touch_switch = item2tsn(mapping_ITEM_LIGHT_1);
                    EventCtrl(touch_switch, INVERSE);
                }*/
                touch_switch = item2tsn(mapping_ITEM_LIGHT_1);  //스위치 전등 스펙 상관없이 K 으로전등1을 제어
                EventCtrl(touch_switch, INVERSE);
				printf("K_06\r\n");
                break;
            case K_07:
                if(pG_Config->LightCount > 1)
                {
                    touch_switch = item2tsn(mapping_ITEM_LIGHT_2);
                    EventCtrl(touch_switch, INVERSE);
                }
				printf("K_07\r\n");
                break;
            case K_08:
                if(pG_Config->LightCount > 1)
                {
                    if(GET_Light_ON_Count() == pG_Config->LightCount)
                    {
                        for(item = mapping_ITEM_LIGHT_1; item < (mapping_ITEM_LIGHT_1 + pG_Config->LightCount); item++)
                        {
                            EventCtrl(item2tsn(item), OFF);
                        }
                    }
                    else    
                    {
                        for(item = mapping_ITEM_LIGHT_1; item < (mapping_ITEM_LIGHT_1 + pG_Config->LightCount); item++)
                        {
                            EventCtrl(item2tsn(item), ON);
                        }                       
                    }
                }
                else        //스위치 전등1구 스펙일 때, 전체 제어 버튼으로 전등1 제어
                {
                    touch_switch = item2tsn(mapping_ITEM_LIGHT_1);
                    EventCtrl(touch_switch, INVERSE);                   
                }
				printf("K_08\r\n");
                break;                
#else
            case K_03:
                if(pG_Config->LightCount > 1)		//전등 수가 1개 보다 많을 때,
                {
                    touch_switch = item2tsn(mapping_ITEM_LIGHT_1);
                    EventCtrl(touch_switch, INVERSE);
                }
                /*touch_switch = item2tsn(mapping_ITEM_LIGHT_1);  //스위치 전등 갯수 상관없이 K03이 전등1 제어
                EventCtrl(touch_switch, INVERSE);*/
				printf("K_03\r\n");
                break;
            case K_05:
                if(pG_Config->LightCount > 1)		//전등 수가 1개 보다 많을 때,
                {
                    touch_switch = item2tsn(mapping_ITEM_LIGHT_2);
                    EventCtrl(touch_switch, INVERSE);
                }
                else								//전등 수가 1개일 때
                {
                    touch_switch = item2tsn(mapping_ITEM_LIGHT_1);
                    EventCtrl(touch_switch, INVERSE);
                }
				printf("K_05\r\n");
                break;
            case K_08:
                if(pG_Config->LightCount > 1)		//전등 수가 1개 보다 많을 때,
                {
                    if(GET_Light_ON_Count() == pG_Config->LightCount)
                    {
                        for(item = mapping_ITEM_LIGHT_1; item < (mapping_ITEM_LIGHT_1 + pG_Config->LightCount); item++)
                        {
                            EventCtrl(item2tsn(item), OFF);
                        }
                    }
                    else
                    {
                        for(item = mapping_ITEM_LIGHT_1; item < (mapping_ITEM_LIGHT_1 + pG_Config->LightCount); item++)
                        {
                            EventCtrl(item2tsn(item), ON);
                        }                       
                    }
                }
                else        //스위치 전등 스펙이 1구 일 때, 전체 버튼으로 전등1 제어
                {
                    touch_switch = item2tsn(mapping_ITEM_LIGHT_1);
                    EventCtrl(touch_switch, INVERSE);                   
                }
				printf("K_08\r\n");
                break;
#endif
            default:
                printf("Vaule is out of range, ");
                if(Data == K_01)        printf("K_01\r\n");
                else if(Data == K_02)   printf("K_02\r\n");
                else if(Data == K_04)   printf("K_04\r\n");
                else if(Data == K_06)   printf("K_06\r\n");
                else if(Data == K_07)   printf("K_07\r\n");
                else                    printf("\r\n");
                break;
        }
        IR_Data = 0;
    }
    IR_RX_Ok = 0;
#endif
}

void IR_Elec_Control(uint8_t touch_switch, uint8_t Flag)
{
    uint8_t item;
    item = tsn2item(touch_switch);
    if(Flag == INVERSE)
    {
        Flag	= (uint8_t)((~GET_Switch_State(touch_switch))&0x01);
    }
    SET_Switch_State(touch_switch, Flag);
    SET_LED_State(touch_switch, Flag);
    Beep(Flag);
    PUT_RelayCtrl(item2ctrl(item), Flag);
    SET_SWITCH_Delay_OFF_Flag(item, 0);
    SET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_ALL, 0);
    ALL_Electricity_Switch_LED_Ctrl();    
}

/*void irq_IR(void)
{   
    if (TIM5_GetITStatus(TIM5_IT_CC1) != RESET)
    {
        TIM5_ClearITPendingBit(TIM5_IT_CC1);
        capture = TIM5_GetCapture1();
        IR_FLAG = 1;    
    }
}*/

/*void IR_Process(void)
{
    static uint8_t  IR_cnt  = 0;
    uint8_t IR_RX_Ok    = 0;
    volatile uint16_t capVal[32];

    const uint32_t freq_12_5hz  = 80000;
    const uint32_t freq_7hz     = 150000;
    const uint16_t freq_500hz   = 2000;
    
    static uint32_t IR_Data = 0;
    int i = 0;

    if(pG_Config->Enable_Flag.IR == 0)  return;
    
    if(IR_FLAG == 1)
    {
        if(((capture >= freq_80hz) && (capture <= freq_70hz)) && (IR_cnt == 0))     //period 12.5 ~ 14.285  13.5ms -> lead code
        {
            IR_cnt++;
            // printf("Transmission START capture1 = %d\r\n", capture);
        } 
        if((((capture >= 1100) && (capture <= 1200)) || ((capture >= 2200) && (capture <= 2300))) && (IR_cnt >= 1))     //period 1 ~ 3.333  1.12 -> Bit "0", 2.24 -> Bit "1"
        {
            capVal[IR_cnt-1] = capture;
            // printf("capval[%d] = %d\r\n", IR_cnt-1, capVal[IR_cnt-1]);
            IR_cnt++;
            if(IR_cnt >= 33)
            {
                IR_cnt      = 0;
                IR_RX_Ok    = 1;
            }
        }
        if((capture >= freq_12_5hz) && (capture <= freq_7hz))
        {
            IR_RX_Ok    = 1;
            // printf("Transmission FINISH\r\n");
        }   

        if(IR_RX_Ok == 1)
        {
            for(i = 0; i < sizeof(capVal) / sizeof(uint16_t); i++)
            {
                if(capVal[i] >= freq_500hz)
                {
                    IR_Data |= BIT_Tbl[i];
                }
            }
        }

        if(IR_RX_Ok)
        {
            printf("\nIR_Data %08lx\n", IR_Data);
            switch(IR_Data)
            {                           //Custom Code = 60(0110 0000) BF(1011 1111) 2????트씩 나눠서 ???㎈?트???? ???????? 06(0000 0110) FD(1111 1101) -> 06FD
                case 0x06FD30CF:        //K1(Data : 0C)     0C(0000 1100) ???? ??트 ???? ???????? 30(0011 0000), 30?? ???????? CF(1100 1111) -> 30CF. 06FD(Custom Code) + 30(Data Code) + CF(!Data Code)
                    if(G_Trace) printf("IR 1\n");
#if defined(_ONE_SIZE_LIGHT_MODEL_)
                    EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), INVERSE);
#else
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_ALL), INVERSE);
#endif
                    break;
                case 0x06FD708F:        //K2(Data : 0E)
                    if(G_Trace) printf("IR 2\n");
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_1), INVERSE);
                    break;
                case 0x06FD10EF:        //K3(Data : 08)
                    if(G_Trace) printf("IR 3\n");
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_2), INVERSE);
                    break;
                case 0x06FD906F:        //K4(Data : 09)
                    if(G_Trace) printf("IR 4\n");
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_3), INVERSE);
                    break;
                case 0x06FDB04F:        //K5(Data : 0D)
                    if(G_Trace) printf("IR 5\n");
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_4), INVERSE);
                    break;
                case 0x06FDF00F:        //K6(Data : 0F)
                    if(G_Trace) printf("IR 6\n");
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_5), INVERSE);
                    break;
                case 0x06FDA05F:        //K7(Data : 05)
                    if(G_Trace) printf("IR 7\n");
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_6), INVERSE);
                    break;
                case 0x06FD28D7:        //K8(Data : 14)
                    if(G_Trace) printf("IR 8\n");
                    EventCtrl(item2tsn(mapping_ITEM_ELECTRICITY_ALL), INVERSE);
                    break;
            }
            IR_Data = 0;
        }
        IR_RX_Ok = 0;
    }
    IR_FLAG = 0;
}*/

/*switch(IR_Data)			Bit_Sort 사용안할 경우.
												Custom Code = 60(0110 0000) BF(1011 1111) 1바이트씩 나눠서 하위비트부터 들어오면 06(0000 0110) FD(1111 1101) -> 06FD
	case 0x06FD30CF:		K1(Data : 0C)		0C(0000 1100) 하위 비트 부터 들어오면 30(0011 0000), 30의 반전코드 CF(1100 1111) -> 30CF. 06FD(Custom Code) + 30(Data Code) + CF(!Data Code)
	case 0x06FD708F:		K2(Data : 0E)
	case 0x06FD10EF:		K3(Data : 08)
	case 0x06FD906F:		K4(Data : 09)
	case 0x06FDB04F:		K5(Data : 0D)
	case 0x06FDF00F:		K6(Data : 0F)
	case 0x06FDA05F:		K7(Data : 05)
	case 0x06FD28D7:		K8(Data : 14)
*/
					/*EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), INVERSE);		//지연소등 미적용
					if(GET_Switch_State(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF)) == OFF)
					{
						EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), INVERSE);
					}
					else
					{
						EventCtrl(item2tsn(mapping_ITEM_BATCH_LIGHT_OFF), OFF);		//batch light test.
					}*/ //지연소등 적용 20210126

/*#if defined(_ONE_SIZE_LIGHT_1_n_SLEEP_) || defined(_ONE_SIZE_LIGHT_2_n_SLEEP_)    //???? 스?????? ???? 1개?? 네트???? 스???×? ?????? 리모?? 모?? ?????????? ??. (????????꼐 ????봄)
    if(IR_RX_Ok)
    {
        Data = Bit_Sort(&IR_Data);
        switch(Data)
        {
            case K_02:  //Data : 0x02
                //???? ??튼. ?娩? ???? ???? 스???×〈? ???? ?????? ???????? ????.
                break;
            case K_04:  
                Gu8_PowerSaving_Tmr = POWER_SAVING_TMR;
                touch_switch = item2tsn(mapping_ITEM_SLEEP);
                if(GET_LED_State(touch_switch) == LED_FLASHING)
                {
                    Sleep_Level(touch_switch);
                }
                else
                {
                    if(GET_Switch_State(touch_switch) == 0) EventCtrl(touch_switch, ON);
                    else                                    EventCtrl(touch_switch, OFF);
                }
                break;
            case K_06:
                if(pG_Config->LightCount > 1)
                {
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_1), INVERSE);
                }
                break;
            case K_07:
                if(pG_Config->LightCount > 1)
                {
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_2), INVERSE);
                }
                break;
            case K_08:  //Data : 0x08
                if(pG_Config->LightCount > 1)
                {
                    if(GET_Light_ON_Count() == pG_Config->LightCount)   //?????? ?? ???????만?
                    {
                        for(item = mapping_ITEM_LIGHT_1; item < (mapping_ITEM_LIGHT_1 + pG_Config->LightCount); item++)
                        {
                            EventCtrl(item2tsn(item), OFF);
                        }
                    }
                    else    //?????? ??나???? ????????나, 모?? 꺼?????만?
                    {
                        for(item = mapping_ITEM_LIGHT_1; item < (mapping_ITEM_LIGHT_1 + pG_Config->LightCount); item++)
                        {
                            EventCtrl(item2tsn(item), ON);
                        }                       
                    }
                }
                else                                //???? ??가 1개?? 때
                {
                    EventCtrl(item2tsn(mapping_ITEM_LIGHT_1), INVERSE);         
                }
                break;          
            default:
                printf("Vaule is out of range\r\n");                
                break;
        }
        IR_Data = 0;
    }
    IR_RX_Ok = 0;*/



/************************************************************************************
	Project		: 전자식스위치
	File Name	: ThreeWay.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "el_switch.h"
#include "ThreeWay.h"
#include "timer.h"

__IO uint8_t	Gu8_ThreeWay_ON_Tmr[2]		= {100, 100};	// 1s
__IO uint8_t	Gu8_ThreeWay_OFF_Tmr[2]		= {100, 100};
__IO uint8_t	Gu8_ThreeWay_Toggle_Flag[2]	= {0, 0};

//-------------------------------------------------------------------------------------------------------------------------
void ThreeWay_Init(void)
{
	if(pG_Config->Enable_Flag.ThreeWay)
	{
	    if(pG_Config->Enable_Flag.ThreeWay & ENABLE_BIT_THREE_WAY_1)
	    {
			GPIO_Init(THREE_WAY_IN_1_PORT, THREE_WAY_IN_1_PIN, GPIO_Mode_In_FL_No_IT);				// Input floating, no external interrupt
			if(GPIO_ReadInputDataBit(THREE_WAY_IN_1_PORT, (GPIO_Pin_TypeDef)THREE_WAY_IN_1_PIN))	Gu8_ThreeWay_Toggle_Flag[0] = 1;
			else																					Gu8_ThreeWay_Toggle_Flag[0] = 0;
			printf("ThreeWay_1 Init\n");
		}
		if(pG_Config->Enable_Flag.ThreeWay & ENABLE_BIT_THREE_WAY_2)
	    {
			GPIO_Init(THREE_WAY_IN_2_PORT, THREE_WAY_IN_2_PIN, GPIO_Mode_In_FL_No_IT);				// Input floating, no external interrupt
			// printf("ThreeWay_2 Init\n");
		}
		
		// printf("ThreeWay Init\n");
	}
}

void ThreeWay_EXT_Switch_State_Process(void)		// 외부스위치
{
	uint8_t Flag;
	if(pG_Config->Enable_Flag.ThreeWay & ENABLE_BIT_THREE_WAY_1)
	{
		if(GPIO_ReadInputDataBit(THREE_WAY_IN_1_PORT, (GPIO_Pin_TypeDef)THREE_WAY_IN_1_PIN))	// GPIO Read
		{
			Gu8_ThreeWay_OFF_Tmr[0]	= 10;	// 100ms
			if(Gu8_ThreeWay_ON_Tmr[0] == 0)
			{
				if(Gu8_ThreeWay_Toggle_Flag[0] == 0)
				{
#ifndef _ONE_SIZE_BATCH_BLOCK_MODEL_
#if defined _KOCOM_PROTOCOL_
					Gu8_Light_n_ETC_Touch_Flag = 1;
#endif
					Flag	= SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), INVERSE);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
					//PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_1), Flag);						// 항목기준 제어
					EventCtrl(item2tsn(mapping_ITEM_3WAY_1), Flag);
					// printf("1\r\n");
#endif
				}
				Gu8_ThreeWay_Toggle_Flag[0]	= 1;
			}
		}
		else
		{
			Gu8_ThreeWay_ON_Tmr[0]		= 10;	// 100ms
			if(Gu8_ThreeWay_OFF_Tmr[0] == 0)
			{
				if(Gu8_ThreeWay_Toggle_Flag[0] == 1)
				{
#ifndef _ONE_SIZE_BATCH_BLOCK_MODEL_
#if defined _KOCOM_PROTOCOL_
					Gu8_Light_n_ETC_Touch_Flag = 1;
#endif
					Flag	= SET_Switch_State(item2tsn(mapping_ITEM_3WAY_1), INVERSE);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
					//PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_1), Flag);						// 항목기준 제어
					EventCtrl(item2tsn(mapping_ITEM_3WAY_1), Flag);
					// printf("2\r\n");
#endif
				}
				Gu8_ThreeWay_Toggle_Flag[0]	= 0;
			}
		}
	}
	
	if(pG_Config->Enable_Flag.ThreeWay & ENABLE_BIT_THREE_WAY_2)
	{
		if(GPIO_ReadInputDataBit(THREE_WAY_IN_2_PORT, (GPIO_Pin_TypeDef)THREE_WAY_IN_2_PIN))	// GPIO Read
		{
			Gu8_ThreeWay_OFF_Tmr[1]	= 10;	// 100ms
			if(Gu8_ThreeWay_ON_Tmr[1] == 0)
			{
				if(Gu8_ThreeWay_Toggle_Flag[1] == 0)
				{
					Flag	= SET_Switch_State(item2tsn(mapping_ITEM_3WAY_2), INVERSE);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
					//PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_2), Flag);						// 항목기준 제어
					EventCtrl(item2tsn(mapping_ITEM_3WAY_2), Flag);
				}
				Gu8_ThreeWay_Toggle_Flag[1]	= 1;
			}
		}
		else
		{
			Gu8_ThreeWay_ON_Tmr[1]		= 10;	// 100ms
			if(Gu8_ThreeWay_OFF_Tmr[1] == 0)
			{
				if(Gu8_ThreeWay_Toggle_Flag[1] == 1)
				{
					Flag	= SET_Switch_State(item2tsn(mapping_ITEM_3WAY_2), INVERSE);			// Switch 상태 설정하고 Switch 상태로 Flag 재설정
					//PUT_RelayCtrl(item2ctrl(mapping_ITEM_3WAY_2), Flag);						// 항목기준 제어
					EventCtrl(item2tsn(mapping_ITEM_3WAY_2), Flag);
				}
				Gu8_ThreeWay_Toggle_Flag[1]	= 0;
			}
		}
	}
}
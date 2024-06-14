/************************************************************************************
	Project		: 전자식스위치
	File Name	: RELAY.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "eeprom.h"
#include "touch.h"
#include "el_switch.h"
#include "Relay.h"

#define	FLOATING_TMR	20		// 190~200ms

uint8_t	Gu8_LatchRelay_1_Floating_Tmr;
uint8_t	Gu8_LatchRelay_2_Floating_Tmr;

uint8_t	Gu8_RelayOnOff[MAX_RELAY];
//-------------------------------------------------------------------------------------------------------------------------
void Relay_Init(void)
{
	int i;
		
	GPIO_Init(RELAY_1_PORT, RELAY_1_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
	GPIO_Init(RELAY_2_PORT, RELAY_2_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
	GPIO_Init(RELAY_3_PORT, RELAY_3_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
	GPIO_Init(RELAY_4_PORT, RELAY_4_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
#if defined _ONE_SIZE_LIGHT_n_ELEC_MODEL_ || defined _TWO_SIZE_LIGHT_n_ELEC_MODEL_ || defined _TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_ || defined _ONE_SIZE_BATCH_BLOCK_MODEL_
	GPIO_Init(LATCH_RELAY_1_1_PORT, LATCH_RELAY_1_1_PIN, GPIO_Mode_Out_PP_Low_Fast);		// Output push-pull, low level, 10MHz
	GPIO_Init(LATCH_RELAY_1_2_PORT, LATCH_RELAY_1_2_PIN, GPIO_Mode_Out_PP_Low_Fast);		// Output push-pull, low level, 10MHz
	GPIO_Init(LATCH_RELAY_2_1_PORT, LATCH_RELAY_2_1_PIN, GPIO_Mode_Out_PP_Low_Fast);		// Output push-pull, low level, 10MHz
	GPIO_Init(LATCH_RELAY_2_2_PORT, LATCH_RELAY_2_2_PIN, GPIO_Mode_Out_PP_Low_Fast);		// Output push-pull, low level, 10MHz
	
	Gu8_LatchRelay_1_Floating_Tmr	= 0;
	Gu8_LatchRelay_2_Floating_Tmr	= 0;
#else
	GPIO_Init(RELAY_5_PORT, RELAY_5_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
	GPIO_Init(RELAY_6_PORT, RELAY_6_PIN, GPIO_Mode_Out_PP_Low_Fast);	// Output push-pull, low level, 10MHz
#endif
	printf("Relay Init\n");
	
	for(i=0;i<MAX_RELAY;i++)
	{
		Gu8_RelayOnOff[i]	= 0;
	}
	//Relay_Ctrl(CONTROL_BIT_RELAY_LATCH_1 | CONTROL_BIT_RELAY_LATCH_2, OFF);		// 일단 최초전원 투입 후에는 꺼진것으로 설정
}

void Relay_Ctrl_Retry(void)
{
	if(Gu8_RelayOnOff[0])	GPIO_SetBits(RELAY_1_PORT, RELAY_1_PIN);
	else					GPIO_ResetBits(RELAY_1_PORT, RELAY_1_PIN);
	
	if(Gu8_RelayOnOff[1])	GPIO_SetBits(RELAY_2_PORT, RELAY_2_PIN);
	else					GPIO_ResetBits(RELAY_2_PORT, RELAY_2_PIN);
	
	if(Gu8_RelayOnOff[2])	GPIO_SetBits(RELAY_3_PORT, RELAY_3_PIN);
	else					GPIO_ResetBits(RELAY_3_PORT, RELAY_3_PIN);
	
	if(Gu8_RelayOnOff[3])	GPIO_SetBits(RELAY_4_PORT, RELAY_4_PIN);
	else					GPIO_ResetBits(RELAY_4_PORT, RELAY_4_PIN);
#if defined _ONE_SIZE_LIGHT_n_ELEC_MODEL_ || defined _TWO_SIZE_LIGHT_n_ELEC_MODEL_ || defined _TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_ || defined _ONE_SIZE_BATCH_BLOCK_MODEL_
	if(Gu8_LatchRelay_1_Floating_Tmr)
	{
		if(Gu8_RelayOnOff[4])
		{
			GPIO_ResetBits(LATCH_RELAY_1_2_PORT, LATCH_RELAY_1_2_PIN);
			GPIO_SetBits(LATCH_RELAY_1_1_PORT, LATCH_RELAY_1_1_PIN);
		}
		else
		{
			GPIO_ResetBits(LATCH_RELAY_1_1_PORT, LATCH_RELAY_1_1_PIN);
			GPIO_SetBits(LATCH_RELAY_1_2_PORT, LATCH_RELAY_1_2_PIN);
		}
	}
	else	//if(Gu8_LatchRelay_1_Floating_Tmr == 0)
	{
		GPIO_ResetBits(LATCH_RELAY_1_1_PORT, LATCH_RELAY_1_1_PIN);
		GPIO_ResetBits(LATCH_RELAY_1_2_PORT, LATCH_RELAY_1_2_PIN);
	}
	
	if(Gu8_LatchRelay_2_Floating_Tmr)
	{
		if(Gu8_RelayOnOff[5])
		{
			GPIO_ResetBits(LATCH_RELAY_2_2_PORT, LATCH_RELAY_2_2_PIN);
			GPIO_SetBits(LATCH_RELAY_2_1_PORT, LATCH_RELAY_2_1_PIN);
		}
		else
		{
			GPIO_ResetBits(LATCH_RELAY_2_1_PORT, LATCH_RELAY_2_1_PIN);
			GPIO_SetBits(LATCH_RELAY_2_2_PORT, LATCH_RELAY_2_2_PIN);
		}
	}
	else	//if(Gu8_LatchRelay_2_Floating_Tmr == 0)
	{
		GPIO_ResetBits(LATCH_RELAY_2_1_PORT, LATCH_RELAY_2_1_PIN);
		GPIO_ResetBits(LATCH_RELAY_2_2_PORT, LATCH_RELAY_2_2_PIN);
	}
#else
	if(Gu8_RelayOnOff[4])	GPIO_SetBits(RELAY_5_PORT, RELAY_5_PIN);
	else					GPIO_ResetBits(RELAY_5_PORT, RELAY_5_PIN);
	
	if(Gu8_RelayOnOff[5])	GPIO_SetBits(RELAY_6_PORT, RELAY_6_PIN);
	else					GPIO_ResetBits(RELAY_6_PORT, RELAY_6_PIN);
#endif
}

void Relay_Ctrl(uint8_t relay, uint8_t Flag)	// 전등
{
	uint8_t	item;
	
	if(relay&CONTROL_BIT_RELAY_1)
	{
		if(Flag == INVERSE)
		{
			item	= ctrl2item(CONTROL_BIT_RELAY_1);
			Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
		}
		Gu8_RelayOnOff[0]	= Flag;
		if(Flag)	GPIO_SetBits(RELAY_1_PORT, RELAY_1_PIN);
		else		GPIO_ResetBits(RELAY_1_PORT, RELAY_1_PIN);
	}
	if(relay&CONTROL_BIT_RELAY_2)
	{
		if(Flag == INVERSE)
		{
			item	= ctrl2item(CONTROL_BIT_RELAY_2);
			Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
		}
		Gu8_RelayOnOff[1]	= Flag;
		if(Flag)	GPIO_SetBits(RELAY_2_PORT, RELAY_2_PIN);
		else		GPIO_ResetBits(RELAY_2_PORT, RELAY_2_PIN);
	}
	if(relay&CONTROL_BIT_RELAY_3)
	{
		if(Flag == INVERSE)
		{
			item	= ctrl2item(CONTROL_BIT_RELAY_3);
			Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
		}
		Gu8_RelayOnOff[2]	= Flag;
		if(Flag)	GPIO_SetBits(RELAY_3_PORT, RELAY_3_PIN);
		else		GPIO_ResetBits(RELAY_3_PORT, RELAY_3_PIN);
	}
	if(relay&CONTROL_BIT_RELAY_4)
	{
		if(Flag == INVERSE)
		{
			item	= ctrl2item(CONTROL_BIT_RELAY_4);
			Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
		}
		Gu8_RelayOnOff[3]	= Flag;
		if(Flag)	GPIO_SetBits(RELAY_4_PORT, RELAY_4_PIN);
		else		GPIO_ResetBits(RELAY_4_PORT, RELAY_4_PIN);
	}
	if(relay&CONTROL_BIT_RELAY_LATCH_1)
	{
		if(Flag == INVERSE)
		{
			item	= ctrl2item(CONTROL_BIT_RELAY_LATCH_1);
			Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
		}
		Gu8_RelayOnOff[4]	= Flag;
		//Gu8_LatchRelay_1_Floating_Tmr	= 5;	// 40~50ms
#if defined _ONE_SIZE_LIGHT_n_ELEC_MODEL_ || defined _TWO_SIZE_LIGHT_n_ELEC_MODEL_ || defined _TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_ || defined _ONE_SIZE_BATCH_BLOCK_MODEL_
		Gu8_LatchRelay_1_Floating_Tmr	= FLOATING_TMR;	// 90~100ms	// jsyoon 20200925
		//printf("L\n");	// TEST
		if(Flag)
		{
			//printf("1\n");	// TEST
			GPIO_ResetBits(LATCH_RELAY_1_2_PORT, LATCH_RELAY_1_2_PIN);
			GPIO_SetBits(LATCH_RELAY_1_1_PORT, LATCH_RELAY_1_1_PIN);
		}
		else
		{
			//printf("2\n");	// TEST
			GPIO_ResetBits(LATCH_RELAY_1_1_PORT, LATCH_RELAY_1_1_PIN);
			GPIO_SetBits(LATCH_RELAY_1_2_PORT, LATCH_RELAY_1_2_PIN);
		}
#else
		if(Flag)	GPIO_SetBits(RELAY_5_PORT, RELAY_5_PIN);
		else		GPIO_ResetBits(RELAY_5_PORT, RELAY_5_PIN);
#endif
	}
	if(relay&CONTROL_BIT_RELAY_LATCH_2)
	{
		if(Flag == INVERSE)
		{
			item	= ctrl2item(CONTROL_BIT_RELAY_LATCH_2);
			Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
		}
		Gu8_RelayOnOff[5]	= Flag;
		//Gu8_LatchRelay_2_Floating_Tmr	= 5;	// 40~50ms
#if defined _ONE_SIZE_LIGHT_n_ELEC_MODEL_ || defined _TWO_SIZE_LIGHT_n_ELEC_MODEL_ || defined _TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_ || defined _ONE_SIZE_BATCH_BLOCK_MODEL_
		Gu8_LatchRelay_2_Floating_Tmr	= FLOATING_TMR;	// 90~100ms	// jsyoon 20200925
		//printf("K");	// TEST
		if(Flag)
		{
			//printf("3\n");	// TEST
			GPIO_ResetBits(LATCH_RELAY_2_2_PORT, LATCH_RELAY_2_2_PIN);
			GPIO_SetBits(LATCH_RELAY_2_1_PORT, LATCH_RELAY_2_1_PIN);
		}
		else
		{
			//printf("4\n");	// TEST
			GPIO_ResetBits(LATCH_RELAY_2_1_PORT, LATCH_RELAY_2_1_PIN);
			GPIO_SetBits(LATCH_RELAY_2_2_PORT, LATCH_RELAY_2_2_PIN);
		}
#else
		if(Flag)	GPIO_SetBits(RELAY_6_PORT, RELAY_6_PIN);
		else		GPIO_ResetBits(RELAY_6_PORT, RELAY_6_PIN);
#endif
	}
	if(relay&CONTROL_BIT_DIMMING_1)				//20210604 디밍 전등도 릴레이 제어 가능하도록 수정.
	{
		if(Flag == INVERSE)
		{
			item	= ctrl2item(CONTROL_BIT_DIMMING_1);
			Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
		}
		// Gu8_RelayOnOff[2]	= Flag;
		Gu8_RelayOnOff[0]	= Flag;
		if(Flag)
		{
			// if(pG_Config->LightCount == 0)
			// {
			// 	Gu8_RelayOnOff[0]	= Flag;	
			// 	GPIO_SetBits(RELAY_1_PORT, RELAY_1_PIN);	//wsm 20240207
			// }
			// else if(pG_Config->LightCount == 1)
			// {
			// 	Gu8_RelayOnOff[1]	= Flag;	
			// 	GPIO_SetBits(RELAY_2_PORT, RELAY_2_PIN);
			// }
			// else if(pG_Config->LightCount == 2)
			// {
			// 	Gu8_RelayOnOff[2]	= Flag;	
			// 	GPIO_SetBits(RELAY_3_PORT, RELAY_3_PIN);
			// }
			// else if(pG_Config->LightCount == 3)
			// {
			// 	Gu8_RelayOnOff[3]	= Flag;
			// 	GPIO_SetBits(RELAY_4_PORT, RELAY_4_PIN);
			// }
			//일반 전등 갯수가 0이고 디밍 전등이 있을 때는 릴레이1번 사용
			//일반 전등 갯수가 1이고 디밍 전등이 있을 때는 릴레이2번 사용
			//일반 전등 갯수가 2이고 디밍 전등이 있을 때는 릴레이3번 사용
			//일반 전등 갯수가 3이고 디밍 전등이 있을 때는 릴레이4번 사용
			// GPIO_SetBits(RELAY_3_PORT, RELAY_3_PIN);
			GPIO_SetBits(RELAY_1_PORT, RELAY_1_PIN);
		}
		else
		{
			// if(pG_Config->LightCount == 0)
			// {
			// 	Gu8_RelayOnOff[0]	= Flag;	
			// 	GPIO_ResetBits(RELAY_1_PORT, RELAY_1_PIN);
			// }
			// else if(pG_Config->LightCount == 1)
			// {
			// 	Gu8_RelayOnOff[1]	= Flag;	
			// 	GPIO_ResetBits(RELAY_2_PORT, RELAY_2_PIN);
			// }
			// else if(pG_Config->LightCount == 2)
			// {
			// 	Gu8_RelayOnOff[2]	= Flag;	
			// 	GPIO_ResetBits(RELAY_3_PORT, RELAY_3_PIN);
			// }
			// else if(pG_Config->LightCount == 3)
			// {
			// 	Gu8_RelayOnOff[3]	= Flag;	
			// 	GPIO_ResetBits(RELAY_4_PORT, RELAY_4_PIN);
			// }
			// GPIO_ResetBits(RELAY_3_PORT, RELAY_3_PIN);
			GPIO_ResetBits(RELAY_1_PORT, RELAY_1_PIN);
		}
	}
	if(relay&CONTROL_BIT_DIMMING_2)				//20210604 디밍 전등도 릴레이 제어 가능하도록 수정.
	{
		if(Flag == INVERSE)
		{
			item	= ctrl2item(CONTROL_BIT_DIMMING_2);
			Flag	= (uint8_t)((~GET_Switch_State(item2tsn(item)))&0x01);
		}
		Gu8_RelayOnOff[3]	= Flag;
		if(Flag)
		{
			if(pG_Config->LightCount == 0)		GPIO_SetBits(RELAY_1_PORT, RELAY_1_PIN);	//wsm 20240207
			else if(pG_Config->LightCount == 1)	GPIO_SetBits(RELAY_2_PORT, RELAY_2_PIN);
			else if(pG_Config->LightCount == 2)	GPIO_SetBits(RELAY_3_PORT, RELAY_3_PIN);
			else if(pG_Config->LightCount == 3)	GPIO_SetBits(RELAY_4_PORT, RELAY_4_PIN);
			// GPIO_SetBits(RELAY_4_PORT, RELAY_4_PIN);
		}
		else
		{
			if(pG_Config->LightCount == 0)		GPIO_ResetBits(RELAY_1_PORT, RELAY_1_PIN);
			else if(pG_Config->LightCount == 1)	GPIO_ResetBits(RELAY_2_PORT, RELAY_2_PIN);
			else if(pG_Config->LightCount == 2)	GPIO_ResetBits(RELAY_3_PORT, RELAY_3_PIN);
			else if(pG_Config->LightCount == 3)	GPIO_ResetBits(RELAY_4_PORT, RELAY_4_PIN);
			// GPIO_ResetBits(RELAY_4_PORT, RELAY_4_PIN);
		}
	}	
	//printf("E\n\n");	// TEST
}

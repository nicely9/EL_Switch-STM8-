/************************************************************************************
	Project		: ���ڽĽ���ġ
	File Name	: eeprom.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/07/10
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "el_switch.h"
#include "adc.h"
#include "Debug.h"
#include "rs-485.h"
#include "i2c.h"
#include "led.h"
#include "WDGnBeep.h"
#include "STPM3x_opt.h"

#define	MAX_EEPROM_ARRAY	(uint8_t)(MAX_EEPROM_DATA / sizeof(EL_STATE))

__IO uint8_t	Gu8_EEPROM_ArrayCnt = 0;

EL_CONFIG		*pG_Config,		GS_Config;
EL_STATE		*pG_State,		GS_State;
uint16_t		Gu16_ElecLimitCurrent_1 = 0;
uint16_t		Gu16_ElecLimitCurrent_2 = 0;
uint16_t		Gu16_ElecLimitCurrent_CRC;
uint8_t			Gu8_model, Gu8_protocol;
//-------------------------------------------------------------------------------------------------------------------------
#if	0
// Declare _fctcpy function prototype as it is packaged by default in the Cosmic machine library
int _fctcpy(char name);
#endif /*_COSMIC_*/
//-------------------------------------------------------------------------------------------------------------------------
#pragma section @near [dataeeprom] 
@near uint16_t			eepGu16_ElecLimitCurrent_1;
@near uint16_t			eepGu16_ElecLimitCurrent_2;
@near uint16_t			eepGu16_ElecLimitCurrent_CRC;
@near uint8_t			rev[50];
@near EL_STATE			eep_State[MAX_EEPROM_ARRAY];
/*
#ifndef	CONFIG_FLASH_STORE
@near EL_CONFIG			rev;		// �ӽð���
@near EL_CONFIG			eep_Config;
#endif
*/
#pragma section []
//-------------------------------------------------------------------------------------------------------------------------
/*#pragma section const {abs_const}
const u8 u8_abs_array[8] = { 0x11, };
#pragma section ()*/
//-------------------------------------------------------------------------------------------------------------------------

const uint8_t *str_TouchType[4]		= {"4ch*1  ", "8ch*1  ", "8ch*2  ", "16ch*1 " };	// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
const uint8_t *str_LCDType[4]		= {"DISABLE", "ENABLE ", "???    ", "???    " };
const uint8_t *str_BackLight[2]		= {"DISABLE", "ENABLE"};
const uint8_t *str_STPM3x[2]		= {"DISABLE", "ENABLE "};
const uint8_t *str_3WayPWMType[4]	= {"DISABLE", "ENABLE1", "ENABLE2", "ENA. 12" };
const uint8_t *str_EnableDisable[2]	= {"DISABLE", "ENABLE " };
const uint8_t *str_OnOff[2]			= {"OFF", "ON " };
const uint8_t *str_Protocol[PROTOCOL_MAX]	= {"NO", "HYUNDAI", "CVNET", "KOCOM", "COMMAX", "SAMSUNG", "KDW", "HW", "HDC"};
const uint8_t *str_mapping[mapping_ITEM_MAX] = 
{
	"DISABLE\t\t",
	
	"L_1\t\t\t",			// ���� 1
	"L_2\t\t\t",			// ���� 2
	"L_3\t\t\t",			// ���� 3
	"L_4\t\t\t",			// ���� 4
	"L_5\t\t\t",			// ���� 5
	"L_6\t\t\t",			// ���� 6
	
	"BATCH_LIGHT_OFF\t",	// �ϰ��ҵ�
	
	"3WAY_1\t\t",			// 3�� 1
	"3WAY_2\t\t",			// 3�� 2
	"DIM_L_1\t\t",			// ��� 1 (PWM)
	"DIM_L_2\t\t",			// ��� 2 (PWM)
	
	"ELEC_1\t\t\t",			// ���� 1
	"ELEC_2\t\t\t",			// ���� 2
	
	"L_GROUP\t\t",			// ���� GROUP ����ġ
	"L_ALL\t\t",			// ���� ALL ����ġ
	"ELEC_ALL\t\t",			// ���� ALL ����ġ
	
	"DIM_UP\t\t",			// ��ֽ���ġ UP
	"DIM_DN\t\t",			// ��ֽ���ġ DOWN
	
	"SETUP\t\t\t",			// ����
	
	"GAS\t\t\t",			// ��������
	"ELEVATOR\t\t",			// ���������� ȣ��

	"SLEEP\t\t",			//���/��ħ

	"COL_UP\t\t",			//����� 1
	"COL_DN\t\t",			//����� 2

	"COOK\t\t",					//��ž
	"BATCH_LIGHT_n_GAS\t\t",	//�ϰ��ҵ�, ����
	"BATCH_LIGHT_n_COOK\t\t",
	"GAS_n_COOK\t\t",
	
};
//--------------------------------------------------------------------------------------------------------------
void SET_State_Defaultdata(void)
{
	// uint8_t	touch_switch;
	// pG_State->SW_State.Word	= 0;

	/*touch_switch	= item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
	if(touch_switch)
	// {
		SET_Switch_State(touch_switch, ON);		// ����Ʈ ON ����
		// SET_LED_State(touch_switch, ON);
	// }
	touch_switch	= item2tsn(mapping_ITEM_GAS);
	if(touch_switch)	SET_Switch_State(touch_switch, ON);		//���� �������� ����
	touch_switch	= item2tsn(mapping_ITEM_ELECTRICITY_ALL);
	if(touch_switch)	SET_Switch_State(touch_switch, ON);		// ����Ʈ ON ����*/

	pG_State->KEY		= CONFIG_KEY;		// Key
	//SET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_ALL), ON);
	//SET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1), ON);
	//SET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2), ON);
#if defined _HYUNDAI_PROTOCOL_
	pG_State->Dimming_Level.Dimming1		= 7;	// ��� 80% 20210517 8->7
	pG_State->Dimming_Level.Dimming2		= 7;	// ��� 80% 20210517 8->7
	pG_State->Color_Temp_Level.Color_Temp1	= 9;
	pG_State->Color_Temp_Level.Color_Temp2	= 9;
#else
	pG_State->Dimming_Level.Dimming1		= 1;
	pG_State->Dimming_Level.Dimming2		= 1;
	pG_State->Color_Temp_Level.Color_Temp1	= 1;
	pG_State->Color_Temp_Level.Color_Temp2	= 1;
#endif
	//pG_State->ETC.Auto1					= 1;
	//pG_State->ETC.Auto2					= 1;
	pG_State->ETC.Auto1						= 0;
	pG_State->ETC.Auto2						= 0;
#if	defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)	//���� �𵨸� �������
// #if	defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) || defined(_ONE_SIZE_LIGHT_MODEL_)		//����  ����ġ ������� �ʿ��� ��
#if defined(_TWO_SIZE_LIGHT_1_n_ELEC_X_) || defined(_TWO_SIZE_LIGHT_2_n_ELEC_X_)	//�ش� ���� ��⽺��ġ���� ��Ʈ��ũ ����ġ �뵵�� ����ϹǷ� ���� ��� ������ ��.
	pG_State->ETC.LED_Mode					= LED_OFF__LIGHT_IS_ON_2;
#else
	pG_State->ETC.LED_Mode					= LED_OFF__LIGHT_IS_ON;		// 0 ~ 3
#endif
	//pG_State->ETC.LED_Mode					= LED_LOW_LEVEL__LIGHT_IS_OFF;		// 0 ~ 3
	pG_State->ETC.BeepMode					= 1;
	pG_State->ETC.DelayOFF					= 1;
	pG_State->User_Mapping_ALL_Light		= pG_Config->Factory_Mapping_ALL_Light;
#elif defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) ||  defined(_ONE_SIZE_LIGHT_MODEL_)	//�ϰ�, ��Ʈ��ũ ����ġ ���� ��� ����
// #elif defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) 		//���� ����ġ ������� �ʿ� �� ��, 
	pG_State->ETC.LED_Mode					= LED_OFF__LIGHT_IS_ON_2;
	pG_State->ETC.BeepMode					= 1;
	pG_State->ETC.DelayOFF					= 1;
	pG_State->User_Mapping_ALL_Light		= pG_Config->Factory_Mapping_ALL_Light;
#else
	pG_State->ETC.LED_Mode					= LED_OFF__LIGHT_IS_ON;		// 0 ~ 3
	pG_State->ETC.BeepMode					= 1;
	pG_State->ETC.DelayOFF					= 1;
	pG_State->User_Mapping_ALL_Light		= pG_Config->Factory_Mapping_ALL_Light;
#endif
	//pG_State->User_Mapping_ALL_Light		= 0;
	
	printf("\n\n--- defaultdata Init(STATE) ---\n\n");
}

#define	TP_Sensitivity		45	// Default High 12, Middle 15, Low 24(MAX 99)

void SET_Defaultdata(void)
{
	uint16_t	i, j;

#ifdef	_ADC_
	pG_Config->ACV_Offset		= 0;
	pG_Config->CT1_Offset		= 0;
	pG_Config->CT2_Offset		= 0;
#endif
	
	pG_Config->KEY		= CONFIG_KEY;		// Key
	// pG_Config->Mode		= 0;				// Mode
	pG_Config->Dimming_MAX_Level		= 0;	// �⺻ 0
	pG_Config->Color_Temp_MAX_Level		= 0;	// �⺻ 0

	// RS485_ID_Init();							//�� �ڸ��� ������ �Լ��� �ٸ� ID�� �����ߴ���� defaultdata�� �⺻ ������ ���ƿ��Ե�.

#if defined (_HYUNDAI_PROTOCOL_)
	pG_Config->Protocol_Type			= HYUNDAI_PROTOCOL;
	pG_Config->Protocol_RES_DelayTime	= HYUNDAI_RES_DELAY_TIME;
	pG_Config->Protocol_IntervalTime	= HYUNDAI_INTERVAL_TIME;
	pG_Config->Dimming_MAX_Level		= 7;						//��������� ��� ������ 0 ~ 7
	pG_Config->Color_Temp_MAX_Level		= 9;						//1, 3, 5, 7, 9�� ���� ����
#elif defined (_CVNET_PROTOCOL_)
	pG_Config->Protocol_Type			= CVNET_PROTOCOL;
	pG_Config->Protocol_RES_DelayTime	= CVNET_RES_DELAY_TIME;
	pG_Config->Protocol_IntervalTime	= CVNET_INTERVAL_TIME;
	pG_Config->Dimming_MAX_Level		= 5;
	pG_Config->Color_Temp_MAX_Level		= 9;						//1, 3, 5, 7, 9�� ���� ����
#elif defined (_KOCOM_PROTOCOL_)
	pG_Config->Protocol_Type			= KOCOM_PROTOCOL;
	pG_Config->Protocol_RES_DelayTime	= KOCOM_RES_DELAY_TIME;
	pG_Config->Protocol_IntervalTime	= KOCOM_INTERVAL_TIME;
	pG_Config->Dimming_MAX_Level		= 10;
	pG_Config->Color_Temp_MAX_Level		= 9;						//1, 3, 5, 7, 9�� ���� ����	
#elif defined (_COMMAX_PROTOCOL_)
	pG_Config->Protocol_Type			= COMMAX_PROTOCOL;
	pG_Config->Protocol_RES_DelayTime	= COMMAX_RES_DELAY_TIME;
	pG_Config->Protocol_IntervalTime	= COMMAX_INTERVAL_TIME;
	pG_Config->Dimming_MAX_Level		= 10;
	pG_Config->Color_Temp_MAX_Level		= 9;						//1, 3, 5, 7, 9�� ���� ����
#elif defined (_SAMSUNG_PROTOCOL_)
	pG_Config->Protocol_Type			= SAMSUNG_PROTOCOL;
	pG_Config->Protocol_RES_DelayTime	= SAMSUNG_RES_DELAY_TIME;
	pG_Config->Protocol_IntervalTime	= SAMSUNG_INTERVAL_TIME;
	pG_Config->Dimming_MAX_Level		= 10;
	pG_Config->Color_Temp_MAX_Level		= 9;						//1, 3, 5, 7, 9�� ���� ����
#elif defined (_KDW_PROTOCOL_)
	pG_Config->Protocol_Type			= KDW_PROTOCOL;
	pG_Config->Protocol_RES_DelayTime	= KDW_RES_DELAY_TIME;
	pG_Config->Protocol_IntervalTime	= KDW_INTERVAL_TIME;
	pG_Config->Dimming_MAX_Level		= 15;
	pG_Config->Color_Temp_MAX_Level		= 9;						//1, 3, 5, 7, 9�� ���� ����
#elif defined(_HW_PROTOCOL_)
	pG_Config->Protocol_Type			= HW_PROTOCOL;
	pG_Config->Protocol_RES_DelayTime	= 11;
	pG_Config->Protocol_IntervalTime	= 6;
	pG_Config->Dimming_MAX_Level		= 10;
	pG_Config->Color_Temp_MAX_Level		= 9;						//1, 3, 5, 7, 9�� ���� ����
#elif defined(_HDC_PROTOCOL_)
	pG_Config->Protocol_Type			= HDC_PROTOCOL;
	pG_Config->Protocol_RES_DelayTime	= HDC_RES_DELAY_TIME;
	pG_Config->Protocol_IntervalTime	= HDC_INTERVAL_TIME;
	pG_Config->Dimming_MAX_Level		= 10;
	pG_Config->Color_Temp_MAX_Level		= 9;						//1, 3, 5, 7, 9�� ���� ����
#else
	// pG_Config->RS485_ID = 0;
	pG_Config->Protocol_Type			= NO_PROTOCOL;
	pG_Config->Protocol_RES_DelayTime	= 10;
	pG_Config->Protocol_IntervalTime	= 6;
	pG_Config->Dimming_MAX_Level		= 10;
	pG_Config->Color_Temp_MAX_Level		= 9;						//1, 3, 5, 7, 9�� ���� ����
#endif
	
	pG_Config->Enable_Flag.ADC_Temperature		= 0;	// 0 = disable,	1 = enabel			// bit 0
	pG_Config->Enable_Flag.ADC_AC_Voltage		= 0;	// 0 = disable,	1 = enabel
	// pG_Config->Enable_Flag.ADC_5_0V_Ref			= 0;
	// pG_Config->Enable_Flag.ADC_3_3V_Ref			= 0;
	/*
	pG_Config->Enable_Flag.ADC_CT_Sensor1		= 0;	// 0 = disable,	1 = enabel
	pG_Config->Enable_Flag.ADC_CT_Sensor2		= 0;	// 0 = disable,	1 = enabel
	*/

	//pG_State->ETC.DelayOFF						= 1;
#ifdef IR_ENABLE
	pG_Config->Enable_Flag.IR					= 1;	// 0 = disable,	1 = enable
#endif
#ifdef IR_DISABLE	
	pG_Config->Enable_Flag.IR					= 0;	// 0 = disable,	1 = enable
#endif
	//pG_Config->BeepFreq							= 2000;
	//pG_Config->BeepDuty							= 95;
	//-------------------------------------------------------------------------
	for(i=0;i<2;i++)
	{
		for(j=0;j<8;j++)
		{
			pG_Config->GT308L[i].Sensitivity[j]	= TP_Sensitivity;
		}
	}
	//-------------------------------------------------------------------------
	for(i=0;i<MAX_SWITCH;i++)
	{
		pG_Config->Mapping_TSN_ITEM[i]		= 0;	// ����ġ�� �Ҵ�� �׸�
		pG_Config->Mapping_TSN_OnTmr[i]		= 250;	// 25��
		pG_Config->Mapping_TSN_OffTmr[i]	= 250;	// 25��
	}
	for(i=0;i<mapping_ITEM_MAX;i++)
	{
		pG_Config->Mapping_ITEM_TSN[i]		= 0;	// �׸� �Ҵ�� ����ġ ��ȣ
		pG_Config->Mapping_ITEM_Control[i]	= 0;	// �׸� �Ҵ�� ����
	}
	/*
	for(i=0;i<CIRCUIT_MAX;i++)
	{
		pG_Config->Mapping_LightCircuitI_ITEM[i]		= 0;
		pG_Config->Mapping_ElectricityCircuit_ITEM[i]	= 0;
	}
	*/
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#if defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_)
	pG_Config->GT308L[0].Sensitivity[0]	= 30;		// E1 ����ġ
	pG_Config->GT308L[0].Sensitivity[1]	= 32;		// E2 ����ġ
	pG_Config->GT308L[0].Sensitivity[2]	= 31;		// ���� ����ġ
	pG_Config->GT308L[0].Sensitivity[3]	= 48;		// ��� - ����ġ
	pG_Config->GT308L[0].Sensitivity[4]	= 50;		// ��� + ����ġ
	pG_Config->GT308L[0].Sensitivity[5]	= 41;		// ���� ��ü
	pG_Config->GT308L[0].Sensitivity[6]	= 38;		// ���� ��ü
	
	pG_Config->GT308L[1].Sensitivity[0]	= 44;		// 3���� ����ġ
	pG_Config->GT308L[1].Sensitivity[1]	= 38;		// 1���� ����ġ
	pG_Config->GT308L[1].Sensitivity[2]	= 45;		// 2���� ����ġ
	pG_Config->GT308L[1].Sensitivity[3]	= 52;		// 4���� ����ġ
	
	pG_Config->Enable_Flag.TOUCH_Chip_Type		= 2;	// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
	pG_Config->Enable_Flag.STPM3x				= 1;	// 0 = disable,	1 = enable
	pG_Config->Enable_Flag.Back_Light			= 0;
	
	SET_Mapping(mapping_ITEM_ELECTRICITY_ALL,	CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_100ms_1s,	mapping_SWITCH_6);	// ��� ������, 1�ʵ��� ������ ����
	SET_Mapping(mapping_ITEM_ELECTRICITY_1,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_100ms_1s,	mapping_SWITCH_1);
	SET_Mapping(mapping_ITEM_ELECTRICITY_2,		CONTROL_BIT_RELAY_LATCH_2,	TMR_0s,		TMR_100ms_1s,	mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_SETUP,				CONTROL_BIT_RELAY_NON ,		TMR_100ms_1s,	TMR_0s,		mapping_SWITCH_3);
	pG_Config->Factory_Mapping_ALL_Electricity	= (uint8_t)(CONTROL_BIT_RELAY_LATCH_1 | CONTROL_BIT_RELAY_LATCH_2);

#ifdef	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_1_
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	// SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	// pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Dimming			= ENABLE_BIT_DIMMING_1;	// 0 = disable,	0x1 = PWM1,	0x2 = PWM2
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xC1;
#endif
#ifdef	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_2 | CONTROL_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Dimming			= ENABLE_BIT_DIMMING_1;	// 0 = disable,	0x1 = PWM1,	0x2 = PWM2
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xC2;
#endif
#ifdef	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_n_DIM_1_		//��� �׽�Ʈ�� ����
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Dimming			= ENABLE_BIT_DIMMING_1;	// 0 = disable,	0x1 = PWM1,	0x2 = PWM2
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xC3;
#endif
#ifdef	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_n_DIM_1_		//��� �׽�Ʈ�� ����
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_LIGHT_4,			CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4 | CONTROL_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Dimming			= ENABLE_BIT_DIMMING_1;	// 0 = disable,	0x1 = PWM1,	0x2 = PWM2
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xCA;
#endif
#ifdef	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_2_
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_2,	CONTROL_BIT_DIMMING_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);	
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2);
	pG_Config->Enable_Flag.PWM_Dimming			= (uint8_t)(ENABLE_BIT_DIMMING_1 | ENABLE_BIT_DIMMING_2);	// 0 = disable,	0x1 = PWM1,	0x2 = PWM2
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xC4;
#endif
#ifdef	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_2_
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_2,	CONTROL_BIT_DIMMING_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);	
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_DIMMING_1 | CONTROL_BIT_DIMMING_2);
	pG_Config->Enable_Flag.PWM_Dimming			= (uint8_t)(ENABLE_BIT_DIMMING_1 | ENABLE_BIT_DIMMING_2);	// 0 = disable,	0x1 = PWM1,	0x2 = PWM2
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xC5;
#endif
#ifdef	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_
	pG_Config->Enable_Flag.LCD_Type				= 1;
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	// SET_Mapping(mapping_ITEM_COLOR_TEMP_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);		//�׽�Ʈ�� ����ġ �ѹ� ����
	// SET_Mapping(mapping_ITEM_COLOR_TEMP_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);		//�׽�Ʈ�� ����ġ �ѹ� ����
	// pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_2 | CONTROL_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Dimming			= (uint8_t)(ENABLE_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Color_Temp		= (uint8_t)(ENABLE_BIT_COLOR_TEMP_1);
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xCB;
#endif
#ifdef	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_
	pG_Config->Enable_Flag.LCD_Type				= 1;
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);	
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	// SET_Mapping(mapping_ITEM_COLOR_TEMP_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);		//�׽�Ʈ�� ����ġ �ѹ� ����
	// SET_Mapping(mapping_ITEM_COLOR_TEMP_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);		//�׽�Ʈ�� ����ġ �ѹ� ����
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_2 | CONTROL_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Dimming			= (uint8_t)(ENABLE_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Color_Temp		= (uint8_t)(ENABLE_BIT_COLOR_TEMP_1);
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xC6;
#endif
#ifdef	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_
	pG_Config->Enable_Flag.LCD_Type				= 1;
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);	
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	// SET_Mapping(mapping_ITEM_COLOR_TEMP_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);		//�׽�Ʈ�� ����ġ �ѹ� ����
	// SET_Mapping(mapping_ITEM_COLOR_TEMP_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);		//�׽�Ʈ�� ����ġ �ѹ� ����	
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Dimming			= (uint8_t)(ENABLE_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Color_Temp		= (uint8_t)(ENABLE_BIT_COLOR_TEMP_1);
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xC7;
#endif
#ifdef	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_
	pG_Config->Enable_Flag.LCD_Type				= 1;
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_LIGHT_4,			CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	// SET_Mapping(mapping_ITEM_COLOR_TEMP_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);		//�׽�Ʈ�� ����ġ �ѹ� ����
	// SET_Mapping(mapping_ITEM_COLOR_TEMP_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);		//�׽�Ʈ�� ����ġ �ѹ� ����	
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4 | CONTROL_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Dimming			= (uint8_t)(ENABLE_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Color_Temp		= (uint8_t)(ENABLE_BIT_COLOR_TEMP_1);	
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xC8;
#endif
#ifdef	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_n_3WAY_1_
	pG_Config->Enable_Flag.LCD_Type				= 1;
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_3WAY_2,			CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,	CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);	
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_4 | CONTROL_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Dimming			= (uint8_t)(ENABLE_BIT_DIMMING_1);
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_2);		//��ο� ����� dim_light_1, 3way_1 �Բ� ������ �����ؼ� ����ؾ���.
	pG_Config->model = 0xC9;
	pG_State->KEY	= 0;		// eeprom�� ����Ǵ� ������ �ʱ�ȭ�� ����
#endif
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#elif	defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_)
	pG_Config->GT308L[0].Sensitivity[0]	= 30;		// E1 ����ġ
	pG_Config->GT308L[0].Sensitivity[1]	= 32;		// E2 ����ġ
	pG_Config->GT308L[0].Sensitivity[2]	= 31;		// ���� ����ġ
	pG_Config->GT308L[0].Sensitivity[3]	= 48;		// ��� - ����ġ
	pG_Config->GT308L[0].Sensitivity[4]	= 50;		// ��� + ����ġ
	pG_Config->GT308L[0].Sensitivity[5]	= 41;		// ���� ��ü
	pG_Config->GT308L[0].Sensitivity[6]	= 38;		// ���� ��ü
	
	pG_Config->GT308L[1].Sensitivity[0]	= 44;		// 3���� ����ġ
	pG_Config->GT308L[1].Sensitivity[1]	= 38;		// 1���� ����ġ
	pG_Config->GT308L[1].Sensitivity[2]	= 45;		// 2���� ����ġ
	pG_Config->GT308L[1].Sensitivity[3]	= 52;		// 4���� ����ġ
#if defined(_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_) || defined(_TWO_SIZE_LIGHT_1_n_ELEC_2_) || defined(_TWO_SIZE_LIGHT_1_n_ELEC_X_) || defined(_TWO_SIZE_LIGHT_1_n_ELEC_2_n_3WAY_1)
	pG_Config->Enable_Flag.TOUCH_Chip_Type		= 1;
#else
	pG_Config->Enable_Flag.TOUCH_Chip_Type		= 2;	// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
#endif	// _TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_

	pG_Config->Enable_Flag.Back_Light			= 0;

#if defined(_TWO_SIZE_LIGHT_1_n_ELEC_X_) || defined(_TWO_SIZE_LIGHT_2_n_ELEC_X_)	//�ش� ���� ������� ����ġ�� ������� ���� ����� ������ ���̹Ƿ�, �ش� ���� ��� ������� ����ġ �������� ����.
	pG_Config->Enable_Flag.STPM3x				= 0;								//�ش� ���� ��� ����ϹǷ� ���� ���� �ʿ� ����. -> ���� ������ �ʿ������ ���� ������ ���� ����ؾ���. ���� ���� ����ÿ��� �Ŀ��� ��Ʈ?�� ����ϰ� STPM3x 0���� ����ϸ� �ɵ�
#ifdef _TWO_SIZE_LIGHT_1_n_ELEC_X_
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	pG_Config->Factory_Mapping_ALL_Light		= 0;
	pG_Config->Factory_Mapping_ALL_Electricity	= 0;
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xD3;
#endif	//_TWO_SIZE_LIGHT_1_n_ELEC_X_
#ifdef _TWO_SIZE_LIGHT_2_n_ELEC_X_
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2);
	pG_Config->Factory_Mapping_ALL_Electricity	= 0;
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xD4;
#endif	//_TWO_SIZE_LIGHT_2_n_ELEC_X_

#else	//�Ϲ� ������� ����ġ ��
	pG_Config->Enable_Flag.STPM3x				= 1;	// 0 = d..isable,	1 = enable
	SET_Mapping(mapping_ITEM_ELECTRICITY_ALL,	CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_100ms_1s,	mapping_SWITCH_6);	// ��� ������, 1�ʵ��� ������ ����
	SET_Mapping(mapping_ITEM_ELECTRICITY_1,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_100ms_1s,	mapping_SWITCH_1);
	SET_Mapping(mapping_ITEM_ELECTRICITY_2,		CONTROL_BIT_RELAY_LATCH_2,	TMR_0s,		TMR_100ms_1s,	mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_SETUP,				CONTROL_BIT_RELAY_NON ,		TMR_100ms_1s,	TMR_0s,		mapping_SWITCH_3);
	pG_Config->Factory_Mapping_ALL_Electricity	= (uint8_t)(CONTROL_BIT_RELAY_LATCH_1 | CONTROL_BIT_RELAY_LATCH_2);
#endif	//if defined(_TWO_SIZE_LIGHT_1_n_ELEC_X_) || defined(_TWO_SIZE_LIGHT_2_n_ELEC_X_)
#ifdef	_TWO_SIZE_LIGHT_1_n_ELEC_2_
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	pG_Config->Factory_Mapping_ALL_Light		= 0;
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xB1;
#endif
#ifdef	_TWO_SIZE_LIGHT_2_n_ELEC_2_
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xB2;
#endif
#ifdef	_TWO_SIZE_LIGHT_3_n_ELEC_2_
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xB3;
#endif
#ifdef	_TWO_SIZE_LIGHT_4_n_ELEC_2_
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_LIGHT_4,			CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xB4;
#endif
#ifdef	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_3WAY_1
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_3WAY_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0xD5;
#endif
#ifdef	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_3WAY_1
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_3WAY_1,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;	
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0xD6;
#endif
#ifdef	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_3WAY_1,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;	
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0xD7;
#endif
#ifdef	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_3WAY_1,			CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;	
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0xD8;
#endif

#ifdef	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	pG_Config->Factory_Mapping_ALL_Light		= 0;
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xB5;
#endif
#ifdef	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xB6;
#endif
#ifdef	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xB7;
#endif
#ifdef	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_LIGHT_4,			CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->model = 0xB8;
#endif
#ifdef	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1_n_LCD_
	pG_Config->Enable_Flag.LCD_Type				= 1;
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_3WAY_1,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_1);		//��ο� ����� dim_light_1, 3way_1 �Բ� ������ �����ؼ� ����ؾ���.
	pG_Config->model = 0xB9;	
#endif
#ifdef	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1_n_LCD_
	pG_Config->Enable_Flag.LCD_Type				= 1;
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	SET_Mapping(mapping_ITEM_3WAY_1,			CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4);
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_1);		//��ο� ����� dim_light_1, 3way_1 �Բ� ������ �����ؼ� ����ؾ���.
	pG_Config->model = 0xBA;
#endif

	pG_State->KEY	= 0;		// eeprom�� ����Ǵ� ������ �ʱ�ȭ�� ����
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#elif	defined(_TWO_SIZE_LIGHT_MODEL_)
	pG_Config->GT308L[0].Sensitivity[0]	= 36;		// ����5
	pG_Config->GT308L[0].Sensitivity[1]	= 35;		// ����4
	pG_Config->GT308L[0].Sensitivity[2]	= 36;		// ����1
	pG_Config->GT308L[0].Sensitivity[3]	= 41;		// ����2
	pG_Config->GT308L[0].Sensitivity[4]	= 45;		// �׷�
	pG_Config->GT308L[0].Sensitivity[5]	= 41;		// ������ü
	pG_Config->GT308L[0].Sensitivity[6]	= 38;		// ����3
	pG_Config->GT308L[0].Sensitivity[7] = 41;		// ����6

	pG_Config->GT308L[1].Sensitivity[0]	= 48;		// 3���� ����ġ
	pG_Config->GT308L[1].Sensitivity[1]	= 43;		// 1���� ����ġ
	pG_Config->GT308L[1].Sensitivity[2]	= 50;		// 2���� ����ġ
	pG_Config->GT308L[1].Sensitivity[3]	= 57;		// 4���� ����ġ
	//pG_Config->GT308L[1].Sensitivity[4]	= 45;
	//pG_Config->GT308L[1].Sensitivity[5]	= 45;
	//pG_Config->GT308L[1].Sensitivity[6]	= 45;
	//pG_Config->GT308L[1].Sensitivity[7]	= 45;
	
	pG_Config->Enable_Flag.TOUCH_Chip_Type		= 1;	// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	pG_Config->Enable_Flag.STPM3x				= 0;	// 0 = disable,	1 = enable
	pG_Config->Enable_Flag.Back_Light			= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
// #if defined(_TWO_SIZE_LIGHT_1_) || defined(_TWO_SIZE_LIGHT_2_)		//2���� ����1��, 2�� �� ��
#if defined(_TWO_SIZE_LIGHT_2_ETC_) || defined(_TWO_SIZE_LIGHT_3_ETC_)	//2���� ���� 2��, 3��(Ư��) �� ?
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
// #endif
#else		//2���� ���� 3��. 4��, 5��, 6�� �� ��
	SET_Mapping(mapping_ITEM_LIGHT_GROUP,		CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_6);
#endif

#ifdef	_TWO_SIZE_LIGHT_1_
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	pG_Config->Factory_Mapping_ALL_Light = 0;
	pG_Config->model = 0x25;
#endif
#ifdef	_TWO_SIZE_LIGHT_2_
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2);
	pG_Config->model = 0x2A;
#endif
#ifdef	_TWO_SIZE_LIGHT_3_
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3);
	pG_Config->model = 0x21;
#endif	
#ifdef	_TWO_SIZE_LIGHT_4_
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_LIGHT_4,			CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_8);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4);
	pG_Config->model = 0x22;
#endif	
#ifdef	_TWO_SIZE_LIGHT_5_
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,				CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	SET_Mapping(mapping_ITEM_LIGHT_3,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_4,				CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_LIGHT_5,				CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_8);
	pG_Config->Factory_Mapping_ALL_Light			= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4 | CONTROL_BIT_RELAY_LATCH_1);
	pG_Config->model = 0x23;
#endif	
#ifdef	_TWO_SIZE_LIGHT_6_
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,				CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	SET_Mapping(mapping_ITEM_LIGHT_3,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_4,				CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_LIGHT_5,				CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	SET_Mapping(mapping_ITEM_LIGHT_6,				CONTROL_BIT_RELAY_LATCH_2,	TMR_0s,		TMR_0s,			mapping_SWITCH_8);
	pG_Config->Factory_Mapping_ALL_Light			= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4 | CONTROL_BIT_RELAY_LATCH_1 | CONTROL_BIT_RELAY_LATCH_2);
	// pG_Config->Factory_Mapping_ALL_Light			= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4);	//�繫�� ������
	pG_Config->model = 0x24;
#endif
#ifdef	_TWO_SIZE_LIGHT_2_ETC_
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_8);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2);
	pG_Config->model = 0x26;
#endif
#ifdef	_TWO_SIZE_LIGHT_3_ETC_
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_8);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3);
	pG_Config->model = 0x27;
#endif
#ifdef _TWO_SIZE_LIGHT_3_n_3WAY_1_
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_3WAY_1,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3);
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0x28;
#endif
#ifdef _TWO_SIZE_LIGHT_4_n_3WAY_1_
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_3WAY_1,			CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_8);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4);
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0x29;
#endif
#ifdef _TWO_SIZE_LIGHT_5_n_3WAY_1_
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,				CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	SET_Mapping(mapping_ITEM_LIGHT_3,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_4,				CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_3WAY_1,				CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_8);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4 | CONTROL_BIT_RELAY_LATCH_1);
	pG_Config->Enable_Flag.ThreeWay				= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0x2B;
#endif
	// �Ʒ� ��ü�� ���´� ���� ������ ����� �����
	pG_Config->Factory_Mapping_ALL_Electricity	= 0;
	
	pG_State->KEY	= 0;		// eeprom�� ����Ǵ� ������ �ʱ�ȭ�� ����
	
	//pG_Config->Enable_Flag.PWM_Dimming			= ENABLE_BIT_DIMMING_1 | ENABLE_BIT_DIMMING_2;	// 0 = disable,	0x1 = PWM1,	0x2 = PWM2
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#elif	defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) || defined(_ONE_SIZE_LIGHT_MODEL_)
	pG_Config->GT308L[0].Sensitivity[0]	= 44;		// E1 ����ġ
	pG_Config->GT308L[0].Sensitivity[1]	= 49;		// E2 ����ġ
	pG_Config->GT308L[0].Sensitivity[2]	= 56;		// ���� ����ġ
	pG_Config->GT308L[0].Sensitivity[3]	= 52;		// ��� - ����ġ
	pG_Config->GT308L[0].Sensitivity[4]	= 52;		// ��� + ����ġ
	pG_Config->GT308L[0].Sensitivity[5]	= 40;		// ���� ��ü
	pG_Config->GT308L[0].Sensitivity[6]	= 42;		// ���� ��ü
	pG_Config->GT308L[0].Sensitivity[7]	= 45;

	// pG_Config->GT308L[1].Sensitivity[0]	= 48;		// 3���� ����ġ
	// pG_Config->GT308L[1].Sensitivity[1]	= 43;		// 1���� ����ġ
	// pG_Config->GT308L[1].Sensitivity[2]	= 50;		// 2���� ����ġ
	// pG_Config->GT308L[1].Sensitivity[3]	= 57;		// 4���� ����ġ
	// pG_Config->GT308L[1].Sensitivity[4]	= 45;
	// pG_Config->GT308L[1].Sensitivity[5]	= 45;
	// pG_Config->GT308L[1].Sensitivity[6]	= 45;
	// pG_Config->GT308L[1].Sensitivity[7]	= 45;
	
	pG_Config->Enable_Flag.TOUCH_Chip_Type		= 1;	// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
	pG_Config->Enable_Flag.LCD_Type				= 0;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	pG_Config->Enable_Flag.STPM3x				= 0;	// 0 = disable,	1 = enable
	pG_Config->Enable_Flag.Back_Light			= 0;
	
#ifdef	_ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_OFF,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_GAS,					CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_ELEVATOR,				CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model = 0xA1;
#endif
#ifdef	_ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_OFF,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_GAS,					CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_3WAY_1,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0xA2;
#endif
#ifdef	_ONE_SIZE_BATCH_LIGHT_n_GAS_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_OFF,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_GAS,					CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model = 0xA3;
#endif
#ifdef	_ONE_SIZE_BATCH_LIGHT_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_OFF,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model = 0xA4;
#endif
#ifdef _ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_n_GAS,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_ELEVATOR,				CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_3WAY_1,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0xA5;	
#endif
#ifdef _ONE_SIZE_BATCH_LIGHT_n_ELEVATOR_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_OFF,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_ELEVATOR,				CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model	= 0xA6;
#endif
#ifdef _ONE_SIZE_BATCH_LIGHT_n_ELEVATOR_n_3WAY_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_OFF,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_ELEVATOR,				CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	SET_Mapping(mapping_ITEM_3WAY_1,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	pG_Config->Enable_Flag.ThreeWay					= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0xA7;	
#endif
#ifdef _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_OFF,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_COOK,					CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_ELEVATOR,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model = 0xA8;	
#endif
#ifdef _ONE_SIZE_BATCH_LIGHT_n_3WAY_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_OFF,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_3WAY_1,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0xA9;	
#endif
#ifdef _ONE_SIZE_BATCH_LIGHT_n_GAS_n_COOK_n_ELEVATOR_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_OFF,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_GAS_n_COOK,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_ELEVATOR,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model = 0xAA;
#endif
#ifdef _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_n_3WAY_
	SET_Mapping(mapping_ITEM_BATCH_LIGHT_n_COOK,	CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_ELEVATOR,				CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_3WAY_1,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= (uint8_t)(ENABLE_BIT_THREE_WAY_1);
	pG_Config->model = 0xAB;
#endif
#ifdef	_ONE_SIZE_LIGHT_1_
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);	
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model = 0x11;
#endif
#ifdef	_ONE_SIZE_LIGHT_2_
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,				CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model	= 0x12;
#endif
#ifdef	_ONE_SIZE_LIGHT_3_
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,				CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_LIGHT_3,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model	= 0x13;
#endif
#ifdef	_ONE_SIZE_LIGHT_1_n_SLEEP_	//20210712
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,			TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_SLEEP,		CONTROL_BIT_RELAY_NON,		TMR_100ms_1s,	TMR_100ms_1s,	mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model	= 0x14;
#endif
#ifdef	_ONE_SIZE_LIGHT_2_n_SLEEP_	//20210331
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,			TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,				CONTROL_BIT_RELAY_2,		TMR_0s,			TMR_0s,			mapping_SWITCH_2);
	// SET_Mapping(mapping_ITEM_SLEEP,	CONTROL_BIT_RELAY_NON,		TMR_0s,			TMR_0s,			mapping_SWITCH_1);
	SET_Mapping(mapping_ITEM_SLEEP,		CONTROL_BIT_RELAY_NON,		TMR_100ms_1s,	TMR_100ms_1s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= 0;
	pG_Config->model	= 0x15;
#endif
#ifdef	_ONE_SIZE_LIGHT_1_n_3WAY_1_
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_3WAY_1,				CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= ENABLE_BIT_THREE_WAY_1;
	pG_Config->model	= 0x16;
#endif
#ifdef	_ONE_SIZE_LIGHT_2_n_3WAY_1_
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_3);
	SET_Mapping(mapping_ITEM_LIGHT_2,				CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_3WAY_1,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_1);
	pG_Config->Enable_Flag.ThreeWay					= ENABLE_BIT_THREE_WAY_1;
	pG_Config->model	= 0x17;
#endif
	// �Ʒ� ��ü�� ���´� ���� ������ ����� �����
	pG_Config->Factory_Mapping_ALL_Light		= 0;
	pG_Config->Factory_Mapping_ALL_Electricity	= 0;
	
	pG_State->KEY	= 0;		// eeprom�� ����Ǵ� ������ �ʱ�ȭ�� ����
	
	pG_Config->Enable_Flag.PWM_Dimming			= 0;	// 0 = disable,	0x1 = PWM1,	0x2 = PWM2
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->GT308L[0].Sensitivity[0]	= 44;		// ���� or �������� or ����2 or ����3 ����ġ
	pG_Config->GT308L[0].Sensitivity[1]	= 49;		// �ϰ� or �������� or ���� or ����2 ����ġ
	pG_Config->GT308L[0].Sensitivity[2]	= 56;		// �ϰ� or ����1 ����ġ
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#elif defined _ONE_SIZE_LIGHT_n_ELEC_MODEL_
	pG_Config->GT308L[0].Sensitivity[0]	= 51;		// ���� ��ü ����ġ
	pG_Config->GT308L[0].Sensitivity[1]	= 50;		// E2 ����ġ
	pG_Config->GT308L[0].Sensitivity[2]	= 58;		// 2���� ����ġ
	pG_Config->GT308L[0].Sensitivity[3]	= 63;		// 1���� ����ġ(���� 1�� ����ġ �� ��)
	pG_Config->GT308L[0].Sensitivity[4]	= 58;		// 1���� ����ġ(���� 2�� ����ġ �� ��)
	pG_Config->GT308L[0].Sensitivity[5]	= 53;		// E1 ����ġ
	pG_Config->GT308L[0].Sensitivity[6]	= 54;		// ���� ����ġ
	pG_Config->GT308L[0].Sensitivity[7]	= 59;		// ��� ����

	// pG_Config->GT308L[1].Sensitivity[0]	= 48;		// 3���� ����ġ
	// pG_Config->GT308L[1].Sensitivity[1]	= 43;		// 1���� ����ġ
	// pG_Config->GT308L[1].Sensitivity[2]	= 50;		// 2���� ����ġ
	// pG_Config->GT308L[1].Sensitivity[3]	= 57;		// 4���� ����ġ
	// pG_Config->GT308L[1].Sensitivity[4]	= 45;
	// pG_Config->GT308L[1].Sensitivity[5]	= 45;
	// pG_Config->GT308L[1].Sensitivity[6]	= 45;
	// pG_Config->GT308L[1].Sensitivity[7]	= 45;

	pG_Config->Enable_Flag.LCD_Type				= 1;
	pG_Config->Enable_Flag.Back_Light			= 1;
	pG_Config->Enable_Flag.TOUCH_Chip_Type		= 1;	// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
	pG_Config->Enable_Flag.STPM3x				= 1;	// 0 = d..isable,	1 = enable
	pG_Config->Enable_Flag.ThreeWay				= 0;
	pG_Config->Enable_Flag.PWM_Dimming			= 0;
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;	
	pG_Config->Factory_Mapping_ALL_Light		= 0;
	pG_Config->Factory_Mapping_ALL_Electricity	= (uint8_t)(CONTROL_BIT_RELAY_LATCH_1 | CONTROL_BIT_RELAY_LATCH_2);	
	
#ifdef _ONE_SIZE_LIGHT_1_n_ELEC_2_n_LCD_
#if 0
	SET_Mapping(mapping_ITEM_ELECTRICITY_ALL,	CONTROL_BIT_RELAY_NON,		TMR_0s,			TMR_100ms_1s,	mapping_SWITCH_1);	// ��� ������, 1�ʵ��� ������ ����
	SET_Mapping(mapping_ITEM_ELECTRICITY_1,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,			TMR_100ms_1s,	mapping_SWITCH_6);
	SET_Mapping(mapping_ITEM_ELECTRICITY_2,		CONTROL_BIT_RELAY_LATCH_2,	TMR_0s,			TMR_100ms_1s,	mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_SETUP,				CONTROL_BIT_RELAY_NON ,		TMR_100ms_1s,	TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,			TMR_0s,			mapping_SWITCH_4);
	pG_Config->model	= 0xD1;
#else		//Ȩ�� ��û���� ����ġ ��ġ�� ������ ����. ��û�� ���
	SET_Mapping(mapping_ITEM_ELECTRICITY_ALL,	CONTROL_BIT_RELAY_NON,		TMR_0s,			TMR_100ms_1s,	mapping_SWITCH_3);	// ��� ������, 1�ʵ��� ������ ����
	SET_Mapping(mapping_ITEM_ELECTRICITY_1,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,			TMR_100ms_1s,	mapping_SWITCH_6);
	SET_Mapping(mapping_ITEM_ELECTRICITY_2,		CONTROL_BIT_RELAY_LATCH_2,	TMR_0s,			TMR_100ms_1s,	mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_SETUP,				CONTROL_BIT_RELAY_NON ,		TMR_100ms_1s,	TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,			TMR_0s,			mapping_SWITCH_8);
	pG_Config->model	= 0xD1;	
#endif
#endif
#ifdef _ONE_SIZE_LIGHT_2_n_ELEC_2_n_LCD_
	SET_Mapping(mapping_ITEM_ELECTRICITY_ALL,	CONTROL_BIT_RELAY_NON,		TMR_0s,			TMR_100ms_1s,	mapping_SWITCH_1);	// ��� ������, 1�ʵ��� ������ ����
	SET_Mapping(mapping_ITEM_ELECTRICITY_1,		CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,			TMR_100ms_1s,	mapping_SWITCH_6);
	SET_Mapping(mapping_ITEM_ELECTRICITY_2,		CONTROL_BIT_RELAY_LATCH_2,	TMR_0s,			TMR_100ms_1s,	mapping_SWITCH_2);
	SET_Mapping(mapping_ITEM_SETUP,				CONTROL_BIT_RELAY_NON ,		TMR_100ms_1s,	TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,			CONTROL_BIT_RELAY_1,		TMR_0s,			TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_LIGHT_2,			CONTROL_BIT_RELAY_2,		TMR_0s,			TMR_0s,			mapping_SWITCH_3);
	// SET_Mapping(mapping_ITEM_LIGHT_3,			CONTROL_BIT_RELAY_3,		TMR_0s,			TMR_0s,			mapping_SWITCH_4);	//��ġ �׽�Ʈ ���� �߰�
	// SET_Mapping(mapping_ITEM_LIGHT_4,			CONTROL_BIT_RELAY_4,		TMR_0s,			TMR_0s,			mapping_SWITCH_8);	//��ġ �׽�Ʈ ���� �߰�
	pG_Config->model	= 0xD2;
#endif
	pG_State->KEY	= 0;		// eeprom�� ����Ǵ� ������ �ʱ�ȭ�� ����

#else
	pG_Config->Enable_Flag.TOUCH_Chip_Type		= 2;	// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
	pG_Config->Enable_Flag.LCD_Type				= 1;	// 0 = disable,	1 = enable,	2 = x,	3 = x
	pG_Config->Enable_Flag.STPM3x				= 1;	// 0 = disable,	1 = enable
	
	SET_Mapping(mapping_ITEM_LIGHT_ALL,			CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_0s,			mapping_SWITCH_7);
	SET_Mapping(mapping_ITEM_LIGHT_1,				CONTROL_BIT_RELAY_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_10);
	SET_Mapping(mapping_ITEM_LIGHT_2,				CONTROL_BIT_RELAY_2,		TMR_0s,		TMR_0s,			mapping_SWITCH_11);
	SET_Mapping(mapping_ITEM_LIGHT_3,				CONTROL_BIT_RELAY_3,		TMR_0s,		TMR_0s,			mapping_SWITCH_9);
	//SET_Mapping(mapping_ITEM_LIGHT_4,				CONTROL_BIT_RELAY_4,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	SET_Mapping(mapping_ITEM_DIMMING_LIGHT_1,		CONTROL_BIT_DIMMING_1,		TMR_0s,		TMR_0s,			mapping_SWITCH_12);
	
	SET_Mapping(mapping_ITEM_ELECTRICITY_ALL,	CONTROL_BIT_RELAY_NON,		TMR_0s,		TMR_100ms_1s,	mapping_SWITCH_6);	// ��� ������, 1�ʵ��� ������ ����
	SET_Mapping(mapping_ITEM_ELECTRICITY_1,			CONTROL_BIT_RELAY_LATCH_1,	TMR_0s,		TMR_100ms_1s,	mapping_SWITCH_1);
	SET_Mapping(mapping_ITEM_ELECTRICITY_2,			CONTROL_BIT_RELAY_LATCH_2,	TMR_0s,		TMR_100ms_1s,	mapping_SWITCH_2);
	
	SET_Mapping(mapping_ITEM_DIMMING_UP,		CONTROL_BIT_RELAY_NON ,		TMR_0s,		TMR_0s,			mapping_SWITCH_5);
	SET_Mapping(mapping_ITEM_DIMMING_DN,		CONTROL_BIT_RELAY_NON ,		TMR_0s,		TMR_0s,			mapping_SWITCH_4);
	
	// �Ʒ� ��ü�� ���´� ���� ������ ����� �����
	//pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_RELAY_4);
	pG_Config->Factory_Mapping_ALL_Light		= (uint8_t)(CONTROL_BIT_RELAY_1 | CONTROL_BIT_RELAY_2 | CONTROL_BIT_RELAY_3 | CONTROL_BIT_DIMMING_1);
	pG_Config->Factory_Mapping_ALL_Electricity	= (uint8_t)(CONTROL_BIT_RELAY_LATCH_1 | CONTROL_BIT_RELAY_LATCH_2);
	
	pG_State->KEY	= 0;		// eeprom�� ����Ǵ� ������ �ʱ�ȭ�� ����
	
	pG_Config->Enable_Flag.PWM_Dimming			= ENABLE_BIT_DIMMING_1 | ENABLE_BIT_DIMMING_2;	// 0 = disable,	0x1 = PWM1,	0x2 = PWM2
	pG_Config->Enable_Flag.PWM_Color_Temp		= 0;
	pG_Config->Enable_Flag.ThreeWay				= 0;
	
	pG_Config->GT308L[0].Sensitivity[0]	= 30;		// E1 ����ġ
	pG_Config->GT308L[0].Sensitivity[1]	= 30;		// E2 ����ġ
	pG_Config->GT308L[0].Sensitivity[2]	= 30;		// ���� ����ġ
	pG_Config->GT308L[0].Sensitivity[3]	= 45;		// ��� - ����ġ
	pG_Config->GT308L[0].Sensitivity[4]	= 45;		// ��� + ����ġ
	pG_Config->GT308L[0].Sensitivity[5]	= 40;		// ���� ��ü
	pG_Config->GT308L[0].Sensitivity[6]	= 35;		// ���� ��ü
	//pG_Config->GT308L[0].Sensitivity[7]	= 45;
	pG_Config->GT308L[1].Sensitivity[0]	= 45;		// 3���� ����ġ
	pG_Config->GT308L[1].Sensitivity[1]	= 45;		// 1���� ����ġ
	pG_Config->GT308L[1].Sensitivity[2]	= 50;		// 2���� ����ġ
	pG_Config->GT308L[1].Sensitivity[3]	= 52;		// 4���� ����ġ
	//pG_Config->GT308L[1].Sensitivity[4]	= 45;
	//pG_Config->GT308L[1].Sensitivity[5]	= 45;
	//pG_Config->GT308L[1].Sensitivity[6]	= 45;
	//pG_Config->GT308L[1].Sensitivity[7]	= 45;
//-----------------------------------------------------------------------------------------------------------------------------------------------------
#endif
	
	pG_Config->LightCount		= 0;	// ���� ��
	pG_Config->ThreeWayCount	= 0;	// 3�� ��
	pG_Config->DimmingCount		= 0;	// ��� ��
	pG_Config->ElectricityCount	= 0;	// ���� ��
	
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		switch(tsn2item((uint8_t)i))
		{
			case mapping_ITEM_BATCH_LIGHT_OFF:		// �ϰ��ҵ�
			case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
			case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
			case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
			case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
			case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
			case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
				pG_Config->LightCount++;
				break;
			case mapping_ITEM_ELECTRICITY_1:		// ���� 1 (������ ����)
			case mapping_ITEM_ELECTRICITY_2:		// ���� 2 (������ ����)
				pG_Config->ElectricityCount++;
				break;
			case mapping_ITEM_3WAY_1:				// 3�� 1 (������ ����)
			case mapping_ITEM_3WAY_2:				// 3�� 2 (������ ����)
				pG_Config->ThreeWayCount++;
				break;
			case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
				pG_Config->DimmingCount++;
				break;
			default:
				break;
		}
	}
	
	//--------------------------------------------------------------------------------------------------------------
	// 31:28 | 27:24,   23:20 | 19:16,   15:12 | 11:8,   7:4 | 3:0
	pG_Config->metroSetupDefault[0]		= 0x040000a0;		// DSP CTRL 1		default value,	ROC1 = 0(CT or Shunt)
	pG_Config->metroSetupDefault[1]		= 0x240000a0;		// DSP CTRL 2		default value,	ROC2 = 0(CT or Shunt)
#ifdef	_STPM3x_AUTO_LATCH_
	 pG_Config->metroSetupDefault[2]		= 0x0B8104e0;		// DSP CTRL 3		60Hz, LED1 disable, LED2 disable, AUTO Latch, ZCR Enable
	//pG_Config->metroSetupDefault[2]		= 0x0B8004e0;		//210107
#else
	pG_Config->metroSetupDefault[2]		= 0x0B0104e0;		// DSP CTRL 3		60Hz, LED1 disable, LED2 disable, ZCR Enable
#endif

#if 0
	pG_Config->metroSetupDefault[3]		= 0x00000000;		// DSP CTRL 4		����, ���� ���� ����
	pG_Config->metroSetupDefault[4]		= 0x003ff800;		// DSP CTRL 5		ä��1 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[5]		= 0x003ff800;		// DSP CTRL 6		ä��1 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[6]		= 0x003ff800;		// DSP CTRL 7		ä��2 ���� �̸��극�̼� - STPM34
	pG_Config->metroSetupDefault[7]		= 0x003ff800;		// DSP CTRL 8		ä��2 ���� �̸��극�̼�
#else
	/*
	pG_Config->metroSetupDefault[3]		= 0x00005005;		// DSP CTRL 4		����, ���� ���� ����
	//pG_Config->metroSetupDefault[4]		= 0x003ff89E;		// DSP CTRL 5		ä��1 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[4]		= 0x003ff857;		// DSP CTRL 5		ä��1 ���� �̸��극�̼�
	//pG_Config->metroSetupDefault[5]		= 0x003ff295;		// DSP CTRL 6		ä��1 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[5]		= 0x003ff2B6;		// DSP CTRL 6		ä��1 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[6]		= 0x003ff800;		// DSP CTRL 7		ä��2 ���� �̸��극�̼� - STPM34
	//pG_Config->metroSetupDefault[7]		= 0x003ffB6A;		// DSP CTRL 8		ä��2 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[7]		= 0x003ffB44;		// DSP CTRL 8		ä��2 ���� �̸��극�̼�
	*/
	
	// 1A���� �̸��극�̼�
#ifdef _ONE_SIZE_LIGHT_n_ELEC_MODEL_
	pG_Config->metroSetupDefault[3]		= 0x00000000;		// DSP CTRL 4		����, ���� ���� ����
	pG_Config->metroSetupDefault[4]		= 0x003ff83e;		// DSP CTRL 5		ä��1 ���� �̸��극�̼�	//SYS : 0x83C, IN2P : 0x83E, ���� 866
	pG_Config->metroSetupDefault[5]		= 0x003ff765;		// DSP CTRL 6		ä��1 ���� �̸��극�̼�	//SYS : 0x785, IN2P : 0x765, ���� 82B
	pG_Config->metroSetupDefault[6]		= 0x003ff835;		// DSP CTRL 7		ä��2 ���� �̸��극�̼� //SYS : 0x84C, IN2P : 0x835, ���� 800
	pG_Config->metroSetupDefault[7]		= 0x003ff7bb;		// DSP CTRL 8		ä��2 ���� �̸��극�̼� //SYS : 0x85F, IN2P : 0x7BB, ���� 848
#else
	pG_Config->metroSetupDefault[3]		= 0x00000000;		// DSP CTRL 4		����, ���� ���� ����
	pG_Config->metroSetupDefault[4]		= 0x003ff80d;		// DSP CTRL 5		ä��1 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[5]		= 0x003ff2bb;		// DSP CTRL 6		ä��1 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[6]		= 0x003ff800;		// DSP CTRL 7		ä��2 ���� �̸��극�̼� - STPM34
	pG_Config->metroSetupDefault[7]		= 0x003ff7d3;		// DSP CTRL 8		ä��2 ���� �̸��극�̼�
#endif
	/*
	// 5A���� �̸��극�̼�
	pG_Config->metroSetupDefault[3]		= 0x00000000;		// DSP CTRL 4		����, ���� ���� ����
	pG_Config->metroSetupDefault[4]		= 0x003ff80d;		// DSP CTRL 5		ä��1 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[5]		= 0x003ff296;		// DSP CTRL 6		ä��1 ���� �̸��극�̼�
	pG_Config->metroSetupDefault[6]		= 0x003ff800;		// DSP CTRL 7		ä��2 ���� �̸��극�̼� - STPM34
	pG_Config->metroSetupDefault[7]		= 0x003ff7cf;		// DSP CTRL 8		ä��2 ���� �̸��극�̼�
	*/
#endif
	
	pG_Config->metroSetupDefault[8]		= 0x00000fff;		// DSP CTRL 9		ä��1 �⺻ ��ȿ���� ������, ��ȿ���� ������, RMS ���� �Ӱ谪
	pG_Config->metroSetupDefault[9]		= 0x00000fff;		// DSP CTRL 10		ä��1 ��ȿ���� ������, �ǻ����� ������, RMS ���� �Ӱ谪
	pG_Config->metroSetupDefault[10]	= 0x00000fff;		// DSP CTRL 11		ä��2 �⺻ ��ȿ���� ������, ��ȿ���� ������, RMS ���� �Ӱ谪
	pG_Config->metroSetupDefault[11]	= 0x00000fff;		// DSP CTRL 12		ä��2 �⺻ ��ȿ���� ������, ��ȿ���� ������, RMS ���� �Ӱ谪
	
#if SHUNT == SHUNT_0_001
	pG_Config->metroSetupDefault[12]	= 0x0F270327;		// DFE CTRL 1		ä��1 ���� enable,  ���� enable, Gain 16
	pG_Config->metroSetupDefault[13]	= 0x0F270326;		// DFE CTRL 2		ä��2 ���� disable, ���� enable, Gain 16
#elif SHUNT == SHUNT_0_002
	pG_Config->metroSetupDefault[12]	= 0x0B270327;		// DFE CTRL 1		ä��1 ���� enable,  ���� enable, Gain 8
	pG_Config->metroSetupDefault[13]	= 0x0B270326;		// DFE CTRL 2		ä��2 ���� disable, ���� enable, Gain 8
#elif SHUNT == SHUNT_0_003
	pG_Config->metroSetupDefault[12]	= 0x0B270327;		// DFE CTRL 1		ä��1 ���� enable,  ���� enable, Gain 8
	pG_Config->metroSetupDefault[13]	= 0x0B270326;		// DFE CTRL 2		ä��2 ���� disable, ���� enable, Gain 8
#elif SHUNT == SHUNT_0_004
	pG_Config->metroSetupDefault[12]	= 0x07270327;		// DFE CTRL 1		ä��1 ���� enable,  ���� enable, Gain 4
	pG_Config->metroSetupDefault[13]	= 0x07270326;		// DFE CTRL 2		ä��2 ���� disable, ���� enable, Gain 4
#endif
	pG_Config->metroSetupDefault[14]	= 0x00000000;		// DSP IRQ  1		interrupt control mask register 1
	pG_Config->metroSetupDefault[15]	= 0x00000000;		// DSP IRQ  2		interrupt control mask register 2
	pG_Config->metroSetupDefault[16]	= 0x00000000;		// DSP SR   1 		status register 1
	pG_Config->metroSetupDefault[17]	= 0x00000000;		// DSP SR   2		status register 2
	pG_Config->metroSetupDefault[18]	= 0x00004007;		// UART & SPI CTRL 1	CRC Enable
	pG_Config->metroSetupDefault[19]	= 0x00000683;		// UART & SPI CTRL 2	9600(0x683)
	//pG_Config->metroSetupDefault[19]	= 0x00000341;		// UART & SPI CTRL 2	57600(0x341)
	//pG_Config->metroSetupDefault[19]	= 0x0000008B;		// UART & SPI CTRL 2	115200(0x8B)
	//--------------------------------------------------------------------------------------------------------------
// #if defined (_ONE_SIZE_BATCH_BLOCK_MODEL_) || defined (_TWO_SIZE_LIGHT_MODEL_) || defined (_ONE_SIZE_LIGHT_MODEL_)	//�ϰ� ����ġ LED MODE ���� �� �������� ������ �߰���.
// 	SET_State_Defaultdata();	//240104
// #endif
	Store_CurrentConfig();
	
	printf("\n\n--- defaultdata Init(CONFIG) ---\n\n");
}

//void SET_Mapping(uint8_t item, uint8_t circuit, uint8_t ctrl, uint8_t on_tmr, uint8_t off_tmr, uint8_t touch_switch)
void SET_Mapping(uint8_t item, uint8_t ctrl, uint8_t on_tmr, uint8_t off_tmr, uint8_t touch_switch)
{
	if(item < mapping_ITEM_MAX && touch_switch <= MAX_SWITCH)
	{
		/*
		if(item >= mapping_ITEM_LIGHT_1 && item <= mapping_ITEM_DIMMING_LIGHT_2)
		{
			pG_Config->Mapping_LightCircuitI_ITEM[circuit]	=	 item;			// �� ȸ�ο� ���� �׸�
		}
		else if(item >= mapping_ITEM_ELECTRICITY_1 && item <= mapping_ITEM_ELECTRICITY_2)
		{
			pG_Config->Mapping_ElectricityCircuit_ITEM[circuit]	= item;			// �� ȸ�ο� ���� �׸�
		}
		*/
		pG_Config->Mapping_ITEM_TSN[item]				= touch_switch;		// �׸���� ��ġ����ġ ��ȣ
		pG_Config->Mapping_ITEM_Control[item]			= ctrl;				// �׸���� ���� ������ġ
		pG_Config->Mapping_TSN_ITEM[touch_switch-1]		= item;				// ��ġ����ġ ���� �׸�
		pG_Config->Mapping_TSN_OnTmr[touch_switch-1]	= on_tmr;
		pG_Config->Mapping_TSN_OffTmr[touch_switch-1]	= off_tmr;
	}
	else
	{
		printf("ERR : Mapping(item[%d : 0~%d], relay[xx], touch_switch[%d : 0~%d]\n", item, mapping_ITEM_MAX, touch_switch, MAX_SWITCH);
	}
}

//--------------------------------------------------------------------------------------------------------------
//#pragma section(ram_code)
void Store_CurrentConfig(void)
{
	__IO uint8_t i;
	__IO uint8_t *ptr_source = (void*)pG_Config;
	FLASH_MemType_TypeDef	FLASH_MemType;
	FLASH_FLAG_TypeDef		FLASH_FLAG;
#ifdef	_ADC_
	uint8_t	tim2_enable	= 0;
#endif
	FLASH_MemType	= FLASH_MemType_Program;
	FLASH_FLAG		= FLASH_FLAG_PUL;			// Flash Program memory unlocked flag
	
	WDG_SetCounter();
	pG_Config->crc = eeprom_crc((void*)pG_Config, (void*)&pG_Config->crc, G_Trace);
	WDG_SetCounter();
	//--------------------------------------------------------------------------------------------------------------
#ifdef	_ADC_
	if(TIM2->CR1 & TIM_CR1_CEN)	// TIM2_Cmd(ENABLE) ����
	{
		tim2_enable	= 1;
		TIM2_Cmd(DISABLE);
	}
#endif
	WDG_SetCounter();
	FLASH_Unlock(FLASH_MemType);
	WDG_SetCounter();
	while (FLASH_GetFlagStatus(FLASH_FLAG) == RESET) WDG_SetCounter();	// Wait until Data Memory area unlocked flag is set
	WDG_SetCounter();
	
	if(G_Trace)	printf("\nwrite data : ");
	for(i=0;i<sizeof(EL_CONFIG);i++)
	{
		WDG_SetCounter();
		FLASH_ProgramByte(FLASH_BASE_ADDR+i, ptr_source[i]);
		
		if(G_Trace)	printf("%02x ", (uint16_t)ptr_source[i]);
	}
	if(G_Trace)	printf("\n");
	
	WDG_SetCounter();
	FLASH_WaitForLastOperation(FLASH_MemType);
	WDG_SetCounter();
	FLASH_Lock(FLASH_MemType);
#ifdef	_ADC_
	G_DMA_Flag	|= ADC_DATA_IGNORE;		// flash�� �����͸� �����Ͽ����� ���ͷ�Ʈ(TIM2_Cmd)�� ��� ������ ������ ADC AC RMS���� �ŷ��� �� �����Ƿ� �����͸� ���� �� �ֵ��� �Ѵ�)

	if(tim2_enable)			// �������� ����
	{
		TIM2_Cmd(ENABLE);
	}
#endif
	//--------------------------------------------------------------------------------------------------------------
	printf("\n\n--- Config Store ---\n\n");
	
}
//#pragma section()

void Store_ElecLimitCurrent(void)
{
	__IO uint16_t i;
	
#ifdef	_ADC_
	uint8_t	tim2_enable	= 0;
#endif
	
	if(Gu16_ElecLimitCurrent_CRC !=  (Gu16_ElecLimitCurrent_1 ^ Gu16_ElecLimitCurrent_2))
	{
		
		//--------------------------------------------------------------------------------------------------------------
#ifdef	_ADC_
		if(TIM2->CR1 & TIM_CR1_CEN)	// TIM2_Cmd(ENABLE) ����
		{
			tim2_enable	= 1;
			TIM2_Cmd(DISABLE);		// TIM2_TRGOSource_Update�� Ȱ��ȭ�Ǿ� ���� �� �ý��� �ٿ�Ǿ� ������
		}
#endif
		FLASH_Unlock(FLASH_MemType_Data);
		
		for(i=0;i<50;i++);	// ������Ű�� ������ ������ �ǳ�... 
		
		while (FLASH_GetFlagStatus(FLASH_FLAG_DUL) == RESET)
		{
			nop(); //WDG_SetCounter();	// Wait until Data EEPROM area unlocked flag is set
		}
		//--------------------------------------------------------------------------------------------------------------
		eepGu16_ElecLimitCurrent_1		= Gu16_ElecLimitCurrent_1;
		eepGu16_ElecLimitCurrent_2		= Gu16_ElecLimitCurrent_2;
		Gu16_ElecLimitCurrent_CRC		= Gu16_ElecLimitCurrent_1 ^ Gu16_ElecLimitCurrent_2;
		eepGu16_ElecLimitCurrent_CRC	= Gu16_ElecLimitCurrent_CRC;
		//--------------------------------------------------------------------------------------------------------------
		FLASH_WaitForLastOperation(FLASH_MemType_Data);
		FLASH_Lock(FLASH_MemType_Data);
#ifdef	_ADC_
		G_DMA_Flag	|= ADC_DATA_IGNORE;		// eeprom�� �����͸� �����Ͽ����� ���ͷ�Ʈ(TIM2_Cmd)�� ��� ������ ������ ADC AC RMS���� �ŷ��� �� �����Ƿ� �����͸� ���� �� �ֵ��� �Ѵ�)
	
		if(tim2_enable)			// �������� ����
		{
			TIM2_Cmd(ENABLE);
		}
#endif
		// if(G_Trace)	printf("Store_ElecLimitCurrent(%dW, %dW)\n", Gu16_ElecLimitCurrent_1, Gu16_ElecLimitCurrent_2);
		printf("Store_ElecLimitCurrent(%dW, %dW)\n", Gu16_ElecLimitCurrent_1, Gu16_ElecLimitCurrent_2);
	}
	//-------------------------------------------------------------------------------------------------------------
	//printf("### Store_ElecLimitCurrent(%dW, %dW)\n", Gu16_ElecLimitCurrent_1, Gu16_ElecLimitCurrent_2);
}


void Store_CurrentState(void)
{
	__IO uint16_t i;
	__IO uint8_t *ptr_source;
	uint8_t *ptr_dest;
#ifdef	_ADC_
	uint8_t	tim2_enable	= 0;
#endif
	ptr_source	= (void*)pG_State;
	
	//--------------------------------------------------------------------------------------------------------------
#ifdef	_ADC_
	if(TIM2->CR1 & TIM_CR1_CEN)	// TIM2_Cmd(ENABLE) ����
	{
		tim2_enable	= 1;
		TIM2_Cmd(DISABLE);		// TIM2_TRGOSource_Update�� Ȱ��ȭ�Ǿ� ���� �� �ý��� �ٿ�Ǿ� ������
	}
#endif
	FLASH_Unlock(FLASH_MemType_Data);
	
	//for(i=0;i<50;i++);	// ������Ű�� ������ ����???
	pG_State->crc = eeprom_crc((void*)pG_State, (void*)&pG_State->crc, G_Trace);
	
	while (FLASH_GetFlagStatus(FLASH_FLAG_DUL) == RESET)
	{
		nop(); //WDG_SetCounter();	// Wait until Data EEPROM area unlocked flag is set
	}

	ptr_dest	= (void*)&eep_State[Gu8_EEPROM_ArrayCnt];
#if 1
	ptr_dest[0]	= (uint8_t)0;		// eeprom erase
#else
	for(i=0;i<sizeof(EL_STATE);i++)
	{
		ptr_dest[i]	= (uint8_t)0;		// eeprom erase
		//FLASH_ProgramByte((uint32_t)ptr_dest+i, 0);
	}
#endif
	Gu8_EEPROM_ArrayCnt++;
	if(Gu8_EEPROM_ArrayCnt >= MAX_EEPROM_ARRAY)	Gu8_EEPROM_ArrayCnt	= 0;
	
	ptr_dest	= (void*)&eep_State[Gu8_EEPROM_ArrayCnt];
	//printf("Gu8_EEPROM_ArrayCnt %d, %d\n", (uint16_t)Gu8_EEPROM_ArrayCnt, (uint16_t)sizeof(EL_STATE));
	for(i=0;i<sizeof(EL_STATE);i++)
	{
		//WDG_SetCounter();
		ptr_dest[i]	= ptr_source[i];	// eepdata write
		//FLASH_ProgramByte((uint32_t)ptr_dest+i, ptr_source[i]);
	}
	
	//--------------------------------------------------------------------------------------------------------------
	if(Gu16_ElecLimitCurrent_CRC !=  (Gu16_ElecLimitCurrent_1 ^ Gu16_ElecLimitCurrent_2))
	{	
		eepGu16_ElecLimitCurrent_1		= Gu16_ElecLimitCurrent_1;
		eepGu16_ElecLimitCurrent_2		= Gu16_ElecLimitCurrent_2;
		Gu16_ElecLimitCurrent_CRC		= Gu16_ElecLimitCurrent_1 ^ Gu16_ElecLimitCurrent_2;
		eepGu16_ElecLimitCurrent_CRC	= Gu16_ElecLimitCurrent_CRC;
	}
	//--------------------------------------------------------------------------------------------------------------

	FLASH_WaitForLastOperation(FLASH_MemType_Data);
	FLASH_Lock(FLASH_MemType_Data);
#ifdef	_ADC_
	G_DMA_Flag	|= ADC_DATA_IGNORE;		// eeprom�� �����͸� �����Ͽ����� ���ͷ�Ʈ(TIM2_Cmd)�� ��� ������ ������ ADC AC RMS���� �ŷ��� �� �����Ƿ� �����͸� ���� �� �ֵ��� �Ѵ�)

	if(tim2_enable)			// �������� ����
	{
		TIM2_Cmd(ENABLE);
	}
#endif
	//--------------------------------------------------------------------------------------------------------------
	if(G_Trace)	printf("\n\n--- State Store(%d) ---\n\n", (uint16_t)Gu8_EEPROM_ArrayCnt);
}

#ifdef	_STATE_DIFF_SAVE__
void eeprom_diff_store_Process(void)		// 100ms Ÿ�̸ӿ��� �����	- �ϴ� ����
{
	static EL_STATE	old_State;

	if(memcmp((void*)&old_State,	(void*)pG_State,	sizeof(EL_STATE)) != 0)		// �����Ͱ� �ٸ���
	{
		Store_CurrentState();
		memcpy((void*)&old_State,	(void*)pG_State,	sizeof(EL_STATE));
	}
}
#endif
//--------------------------------------------------------------------------------------------------------------

void model_check(void)
{
#ifdef	_NO_PROTOCOL_
	Gu8_protocol = NO_PROTOCOL;
#elif defined _HYUNDAI_PROTOCOL_
	Gu8_protocol = HYUNDAI_PROTOCOL;	//0x01
#elif defined _CVNET_PROTOCOL_
	Gu8_protocol = CVNET_PROTOCOL;		//0x02
#elif defined _KOCOM_PROTOCOL_
	Gu8_protocol = KOCOM_PROTOCOL;		//0x03
#elif defined _COMMAX_PROTOCOL_
	Gu8_protocol = COMMAX_PROTOCOL;		//0x04
#elif defined _SAMSUNG_PROTOCOL_
	Gu8_protocol = SAMSUNG_PROTOCOL;	//0x05
#elif defined _KDW_PROTOCOL_
	Gu8_protocol = KDW_PROTOCOL;		//0x06
#elif defined _HW_PROTOCOL_
	Gu8_protocol = HW_PROTOCOL;			//0x07
#elif defined _HDC_PROTOCOL_
	Gu8_protocol = HDC_PROTOCOL;
#endif

#ifdef _ONE_SIZE_LIGHT_1_
	Gu8_model = 0x11;
#elif defined _ONE_SIZE_LIGHT_2_
	Gu8_model = 0x12;
#elif defined _ONE_SIZE_LIGHT_3_
	Gu8_model = 0x13;
#elif defined _ONE_SIZE_LIGHT_1_n_SLEEP_
	Gu8_model = 0x14;
#elif defined _ONE_SIZE_LIGHT_2_n_SLEEP_
	Gu8_model = 0x15;
#elif defined _ONE_SIZE_LIGHT_1_n_3WAY_1_
	Gu8_model = 0x16;
#elif defined _ONE_SIZE_LIGHT_2_n_3WAY_1_
	Gu8_model = 0x17;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_
	Gu8_model = 0xA1;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_GAS_n_3WAY_
	Gu8_model = 0xA2;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_GAS_
	Gu8_model = 0xA3;
#elif defined _ONE_SIZE_BATCH_LIGHT_
	Gu8_model = 0xA4;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_GAS_n_ELEVATOR_n_3WAY_
	Gu8_model = 0xA5;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_ELEVATOR_
	Gu8_model = 0xA6;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_ELEVATOR_n_3WAY_
	Gu8_model = 0xA7;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_
	Gu8_model = 0xA8;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_3WAY_
	Gu8_model = 0xA9;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_GAS_n_COOK_n_ELEVATOR_
	Gu8_model = 0xAA;
#elif defined _ONE_SIZE_BATCH_LIGHT_n_COOK_n_ELEVATOR_n_3WAY_
	Gu8_model = 0xAB;
#elif defined _TWO_SIZE_LIGHT_3_
	Gu8_model = 0x21;
#elif defined _TWO_SIZE_LIGHT_4_
	Gu8_model = 0x22;
#elif defined _TWO_SIZE_LIGHT_5_
	Gu8_model = 0x23;
#elif defined _TWO_SIZE_LIGHT_6_
	Gu8_model = 0x24;
#elif defined _TWO_SIZE_LIGHT_1_
	Gu8_model = 0x25;
#elif defined _TWO_SIZE_LIGHT_2_
	Gu8_model = 0x2A;
#elif defined _TWO_SIZE_LIGHT_2_ETC_
	Gu8_model = 0x26;
#elif defined _TWO_SIZE_LIGHT_3_ETC_
	Gu8_model = 0x27;
#elif defined _TWO_SIZE_LIGHT_3_n_3WAY_1_
	Gu8_model = 0x28;
#elif defined _TWO_SIZE_LIGHT_4_n_3WAY_1_
	Gu8_model = 0x29;
#elif defined _TWO_SIZE_LIGHT_5_n_3WAY_1_
	Gu8_model = 0x2B;
#elif defined	_TWO_SIZE_LIGHT_1_n_ELEC_2_
	Gu8_model = 0xB1;
#elif defined	_TWO_SIZE_LIGHT_2_n_ELEC_2_
	Gu8_model = 0xB2;
#elif defined	_TWO_SIZE_LIGHT_3_n_ELEC_2_
	Gu8_model = 0xB3;
#elif defined	_TWO_SIZE_LIGHT_4_n_ELEC_2_
	Gu8_model = 0xB4;
#elif defined	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_
	Gu8_model = 0xB5;
#elif defined	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_
	Gu8_model = 0xB6;
#elif defined	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_
	Gu8_model = 0xB7;
#elif defined	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_
	Gu8_model = 0xB8;
#elif defined	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1_n_LCD_
	Gu8_model = 0xB9;
#elif defined	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1_n_LCD_
	Gu8_model = 0xBA;	
#elif defined	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_1_
	Gu8_model = 0xC1;
#elif defined	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_
	Gu8_model = 0xC2;
#elif defined	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_n_DIM_1_
	Gu8_model = 0xC3;
#elif defined	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_2_
	Gu8_model = 0xC4;
#elif defined	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_2_
	Gu8_model = 0xC5;
#elif defined	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_
	Gu8_model = 0xC6;
#elif defined	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_
	Gu8_model = 0xC7;
#elif defined	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_
	Gu8_model = 0xC8;
#elif defined	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_LCD_n_DIM_1_n_3WAY_1_
	Gu8_model = 0xC9;
#elif defined	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_LCD_n_DIM_1_
	Gu8_model = 0xCA;
#elif defined	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_LCD_n_DIM_1_n_COLOR_1_
	Gu8_model = 0xCB;
#elif defined	_ONE_SIZE_LIGHT_1_n_ELEC_2_n_LCD_
	Gu8_model = 0xD1;
#elif defined	_ONE_SIZE_LIGHT_2_n_ELEC_2_n_LCD_
	Gu8_model = 0xD2;
#elif defined	_TWO_SIZE_LIGHT_1_n_ELEC_X_
	Gu8_model = 0xD3;
#elif defined	_TWO_SIZE_LIGHT_1_n_ELEC_X_
	Gu8_model = 0xD4;
#elif defined	_TWO_SIZE_LIGHT_1_n_ELEC_2_n_3WAY_1
	Gu8_model = 0xD5;
#elif defined 	_TWO_SIZE_LIGHT_2_n_ELEC_2_n_3WAY_1
	Gu8_model = 0xD6;
#elif defined	_TWO_SIZE_LIGHT_3_n_ELEC_2_n_3WAY_1
	Gu8_model = 0xD7;
#elif defined	_TWO_SIZE_LIGHT_4_n_ELEC_2_n_3WAY_1
	Gu8_model = 0xD8;
#endif
}

void eeprom_Init(void)
{
	__IO uint8_t  i, j;
	__IO uint8_t *ptr_source, *ptr_dest;
	__IO uint8_t	cfg_crc, sta_crc;
	
#if 0
	_fctcpy('r');
#endif
	
	printf("eeprom_Init(flash)\n");
	pG_Config	= &GS_Config;
	pG_State	= &GS_State;
	// SET_Defaultdata();	//240104
	FLASH_SetProgrammingTime(FLASH_ProgramTime_Standard);
	//----------------------------------------------------------------------------------
	ptr_dest	= (void*)pG_Config;
	//ptr_source	= (void*)FLASH_BASE_ADDR;
	
	for(i=0;i<sizeof(EL_CONFIG);i++)
	{
		ptr_dest[i]	= FLASH_ReadByte(FLASH_BASE_ADDR+i);
		WDG_SetCounter();
	}
	//----------------------------------------------------------------------------------
#if 0
	printf("\nread  data : ");
	for(i=0;i<sizeof(EL_CONFIG);i++)
	{
		printf("%02x ", (uint16_t)ptr_dest[i]);
	}
	printf("\n\n");
#endif
	
	Gu8_EEPROM_ArrayCnt	= 0;
	for(i=0;i<MAX_EEPROM_ARRAY;i++)
	{
		//printf("\n\nREAD 1 Gu8_EEPROM_ArrayCnt %d %d\n\n", (uint16_t)Gu8_EEPROM_ArrayCnt, (uint16_t)i);
		//----------------------------------------------------------------------------------
		//memcpy((void*)pG_State,	(void*)&eep_State[Gu8_EEPROM_ArrayCnt],	sizeof(EL_STATE));		// eeprom READ
		ptr_dest	= (void*)pG_State;
		ptr_source	= (void*)&eep_State[Gu8_EEPROM_ArrayCnt];
		for(j=0;j<sizeof(EL_STATE);j++)
		{
			WDG_SetCounter();
			ptr_dest[j]	= ptr_source[j];		// eeprom READ
		}
		//----------------------------------------------------------------------------------
		sta_crc = eeprom_crc((void*)pG_State, (void*)&pG_State->crc, G_Trace);
		
		if(sta_crc == pG_State->crc && pG_State->KEY == CONFIG_KEY)
		{
			WDG_SetCounter();
			STATE_Prn();
			break;
		}
		Gu8_EEPROM_ArrayCnt++;
		if(Gu8_EEPROM_ArrayCnt >= MAX_EEPROM_ARRAY)	Gu8_EEPROM_ArrayCnt	= 0;
		//printf("retry %d\n", (uint16_t)Gu8_EEPROM_ArrayCnt);
	}
	
	cfg_crc = eeprom_crc((void*)pG_Config, (void*)&pG_Config->crc, 0);
	printf("Config CRC[%02x], Cal CRC[%02x]\n", (uint16_t)pG_Config->crc, (uint16_t)cfg_crc);
	model_check();
	WDG_SetCounter();

	if(pG_Config->KEY != CONFIG_KEY)		// CRC�� �ٸ��� ȯ�漳�� �� ���µ����� �ʱ�ȭ �� ����
	{
		printf("pG_Config->KEY = 0x%x, CONFIG_KEY = 0x%x\r\n", (uint16_t)pG_Config->KEY, (uint16_t)CONFIG_KEY);
		printf("ERR : config Key\n");
		SET_Defaultdata();
	}
	else if(cfg_crc != pG_Config->crc)		// CRC�� �ٸ��� ȯ�漳�� �� ���µ����� �ʱ�ȭ �� ����
	{
		printf("ERR : Config CRC\n");
		cfg_crc = eeprom_crc((void*)pG_Config, (void*)&pG_Config->crc, 1);
		printf("\n");
		printf("RX CRC[%02x], Cal CRC[%02x]\n", (uint16_t)pG_Config->crc, (uint16_t)cfg_crc);
		
		SET_Defaultdata();
	}
	else if(Gu8_model != pG_Config->model || Gu8_protocol != pG_Config->Protocol_Type)
	{
		if(Gu8_model != pG_Config->model)				printf("pG_Config->model = 0x%x, model = 0x%x\r\n", (uint16_t)pG_Config->model, (uint16_t)Gu8_model);
		if(Gu8_protocol != pG_Config->Protocol_Type)	printf("pG_Config->Protocol_Type = 0x%x, protocol = 0x%x\r\n", (uint16_t)pG_Config->Protocol_Type, (uint16_t)Gu8_protocol);	
		
		SET_Defaultdata();		//210406 �ٿ�ε� �� �ٷ� ���� ���ؼ�
	}
	else if((pG_Config->Mode & 0xFF) != VERSION)
	{
		printf("Mode %d, VERSION %d\r\n", (uint16_t)pG_Config->Mode, (uint16_t)VERSION);
		pG_Config->Mode = (uint8_t)(VERSION);
		SET_Defaultdata();
	}
#ifdef IR_ENABLE
	else if(pG_Config->Enable_Flag.IR == 0)
	{
		SET_Defaultdata();
	}
#endif
	WDG_SetCounter();
	
	sta_crc = eeprom_crc((void*)pG_State, (void*)&pG_State->crc, 0);
	if(sta_crc != pG_State->crc || pG_State->KEY != CONFIG_KEY)	// CRC�� �ٸ��� ���µ����� �ʱ�ȭ �� ����
	{
		printf("ERR : State CRC or Key\n");
		sta_crc = eeprom_crc((void*)pG_State,(void*)&pG_State->crc, 1);
		printf("\n");
		printf("RX CRC[%02x], Cal CRC[%02x]\n", (uint16_t)pG_State->crc, (uint16_t)sta_crc);
		
		SET_State_Defaultdata();
		Store_CurrentState();		// �� ���� ����
	}
	else if(VERSION < 4)		//VERSION 4���� LED��� ������ �߰��� 20240304
	{
		printf("VERSION < 4\r\n");
		SET_State_Defaultdata();
		Store_CurrentState();		// �� ���� ����
	}

	WDG_SetCounter();
	if(Verification_Config())		// ȯ�漳�� ����
	{
		Debug_StateData();
		printf("\n\n\nERR : Config CHECK !!!!!\n\n\n");
		while(1);
	}
	
	//--------------------------------------------------------------------------------------------------------
	Gu16_ElecLimitCurrent_1		= (uint16_t)eepGu16_ElecLimitCurrent_1;
	Gu16_ElecLimitCurrent_2		= (uint16_t)eepGu16_ElecLimitCurrent_2;
	Gu16_ElecLimitCurrent_CRC	= eepGu16_ElecLimitCurrent_CRC;
	
	if(Gu16_ElecLimitCurrent_CRC != (Gu16_ElecLimitCurrent_1 ^ Gu16_ElecLimitCurrent_2))
	{
		printf("ElecLimitCurrent default SET\n");
		Gu16_ElecLimitCurrent_1	= 0;
		Gu16_ElecLimitCurrent_2	= 0;
		Store_ElecLimitCurrent();
	}
	if((Gu16_ElecLimitCurrent_1 <= 0 || Gu16_ElecLimitCurrent_1 >= 9999) || (Gu16_ElecLimitCurrent_2 <= 0 || Gu16_ElecLimitCurrent_2 >= 9999))
	{
		if(Gu16_ElecLimitCurrent_1 <= 0 || Gu16_ElecLimitCurrent_1 >= 9999)	//-�� �� �̻��� ���� �־ �߰���. 3300���� �������̹Ƿ� ��ǻ� 3300�̻� ������ ������.
		{
			Gu16_ElecLimitCurrent_1 = 0;
		}
		if(Gu16_ElecLimitCurrent_2 <= 0 || Gu16_ElecLimitCurrent_2 >= 9999)
		{
			Gu16_ElecLimitCurrent_2 = 0;
		}
		Store_ElecLimitCurrent();
	}
	printf("ElecLimitCurrent (Auto %d:%d, 0x%04x 0x%04x)\n", pG_State->ETC.Auto1, pG_State->ETC.Auto2, Gu16_ElecLimitCurrent_1, Gu16_ElecLimitCurrent_2);
	//--------------------------------------------------------------------------------------------------------
	printf("EEPROM Init\n");
}

uint8_t* str_Control(uint8_t control)
{
	if(control&CONTROL_BIT_RELAY_1)
	{
		return "RLY1 ";
	}
	if(control&CONTROL_BIT_RELAY_2)
	{
		return "RLY2 ";
	}
	if(control&CONTROL_BIT_RELAY_3)
	{
		return "RLY3 ";
	}
	if(control&CONTROL_BIT_RELAY_4)
	{
		return "RLY4 ";
	}
	if(control&CONTROL_BIT_RELAY_LATCH_1)
	{
		return "LRLY1";
	}
	if(control&CONTROL_BIT_RELAY_LATCH_2)
	{
		return "LRLY2";
	}
	if(control&CONTROL_BIT_DIMMING_1)
	{
		return "DIM1 ";
	}
	if(control&CONTROL_BIT_DIMMING_2)
	{
		return "DIM2 ";
	}
	return "     ";
}

void STATE_Prn(void)
{
	uint8_t		i;
	
	printf("\n\nREAD EEPROM Cnt  %d\n\n", (uint16_t)Gu8_EEPROM_ArrayCnt);
	printf("KEY           0x%04x\n", (uint16_t)pG_State->KEY);						//if(G_Trace)	
	printf("SW_State      0x%04x\n",	(uint16_t)pG_State->SW_State.Word);
	printf("Dim MAX Lv %d\n", (uint16_t)pG_Config->Dimming_MAX_Level);
	printf("Dim[1]  Lv %d\n", (uint16_t)(pG_State->Dimming_Level.Dimming1*10));		// ��� ��
	printf("Dim[2]  Lv %d\n", (uint16_t)(pG_State->Dimming_Level.Dimming2*10));		// ��� ��
	printf("Col MAX Lv %d\n", (uint16_t)pG_Config->Color_Temp_MAX_Level);					//210622 ����� �ִ� ��
	printf("Col[1]  Lv %d\n", (uint16_t)(pG_State->Color_Temp_Level.Color_Temp1*10));		//210622 ������µ� ��
	printf("Col[2]  Lv %d\n", (uint16_t)(pG_State->Color_Temp_Level.Color_Temp2*10));		//210622 ������µ� ��	
	printf("El.Auto 1[%d], 2[%d]\n", (uint16_t)pG_State->ETC.Auto1, (uint16_t)pG_State->ETC.Auto2);
	printf("Store_ElecLimitCurrent(1[%dW], 2[%dW])\n", Gu16_ElecLimitCurrent_1, Gu16_ElecLimitCurrent_2);
	printf("LED_Mode %d\n", (uint16_t)pG_State->ETC.LED_Mode);
	printf("BeepMode %d\n", (uint16_t)pG_State->ETC.BeepMode);
	printf("DelayOFF %d\n", (uint16_t)pG_State->ETC.DelayOFF);
	printf("BatchState %d\r\n", (uint16_t)pG_State->ETC.BatchState);
	printf("USR ALL_Light_SW( ");
	for(i=0;i<MAX_RELAY;i++)
	{
		WDG_SetCounter();
		if((uint8_t)((pG_State->User_Mapping_ALL_Light >> i) & 0x01))	printf("[Switch%d] ", (uint16_t)i+1);
	}
	printf(")\n");
	//printf("CRC                    0x%02x\n\n",	(uint16_t)pG_State->crc);
}

void Debug_StateData(void)
{
	uint8_t		i, j;
	uint8_t		sta;
	uint8_t		item;
	
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	if(G_Trace)
	{
		printf("KEY              0x%04x\n",	(uint16_t)pG_Config->KEY);
	}
	printf("CONFIG(%d) CRC[0x%02x]\n", sizeof(EL_CONFIG), (uint16_t)pG_Config->crc);
	printf("Mode		%04d\n",	(uint16_t)pG_Config->Mode);
#ifdef _CVNET_PROTOCOL_
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
	{
		printf("RS485_ID 0x%02x[0x%02x], ELE_ID 0x%02x\n", (uint16_t)pG_Config->RS485_ID, (uint16_t)Get_Batch_Block_485_ID(), (uint16_t)Get_Elevator_485_ID());
	}
#else
	{
		printf("RS485_ID   0x%02x[0x%02x]\n",	(uint16_t)pG_Config->RS485_ID, (uint16_t)Get_485_ID());
	}
#endif
#else
	printf("RS485_ID	0x%02x\n",	(uint16_t)pG_Config->RS485_ID);
	printf("RS485_ElecID	0x%02x\n",	(uint16_t)pG_Config->RS485_Elec_ID);
#endif
	printf("ProtocolType	%s\n", str_Protocol[pG_Config->Protocol_Type]);
	printf("RS-485 RES_D Time %d\n", (uint16_t)pG_Config->Protocol_RES_DelayTime);
	printf("RS-485 Inter Time %d\n", (uint16_t)pG_Config->Protocol_IntervalTime);
	
	printf("Light CNT %d\n", (uint16_t)pG_Config->LightCount);			// ���� ��
	printf("Dim   CNT %d\n", (uint16_t)pG_Config->DimmingCount);		// ��� ��
	printf("3Way  CNT %d\n", (uint16_t)pG_Config->ThreeWayCount);		// 3�� ��
	printf("El    CNT %d\n", (uint16_t)pG_Config->ElectricityCount);	// ���� ��
	//--------------------------------------------------------------------------------------------------------------
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------
	/*
	for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
	{
		printf("LIGHT Circuit %d  : %s\n", (uint16_t)i, str_mapping[light_circuit2item(i)]);
	}
	*/
	WDG_SetCounter();
	/*
	for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
	{
		printf("ELEC. Circuit %d  : %s\n", (uint16_t)i, str_mapping[electricity_circuit2item(i)]);
	}
	*/
	//--------------------------------------------------------------------------------------------------------------
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------
	for(i=mapping_ITEM_DISABLE+1;i<mapping_ITEM_MAX;i++)
	{
		if(item2tsn(i))
		{
			printf("%s Ctrl[0x%02x %s] Switch[%d]\n", str_mapping[i], (uint16_t)item2ctrl(i), str_Control(item2ctrl(i)), (uint16_t)(item2tsn(i)));
		}
		WDG_SetCounter();
	}
	//--------------------------------------------------------------------------------------------------------------
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------
	for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
	{
		printf("Switch[%2d] %s %s OnTmr[%3d], OffTmr[%3d]\n", (uint16_t)i, str_mapping[tsn2item(i)], str_OnOff[GET_Switch_State(i)], 
											(uint16_t)pG_Config->Mapping_TSN_OnTmr[i-1], (uint16_t)pG_Config->Mapping_TSN_OffTmr[i-1]);	// ��ġ����ġ ���� �׸�
		WDG_SetCounter();
		printf(", LED(%d)\n", (uint16_t)GET_LED_State(i));	// ��ġ����ġ ���� �׸�
	}
	//--------------------------------------------------------------------------------------------------------------
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------
#ifdef	_ADC_
	printf("ADC_Temperature %s\n", str_EnableDisable[pG_Config->Enable_Flag.ADC_Temperature]);	// 0 = disable,	1 = enabel
	printf("ADC_AC_Voltage  %s\n", str_EnableDisable[pG_Config->Enable_Flag.ADC_AC_Voltage]);	// 0 = disable,	1 = enabel
	printf("ADC_5.0V ref    %s\n", str_EnableDisable[pG_Config->Enable_Flag.ADC_5_0V_Ref]);		// 0 = disable,	1 = enabel
	printf("ADC_3.3V ref    %s\n", str_EnableDisable[pG_Config->Enable_Flag.ADC_3_3V_Ref]);		// 0 = disable,	1 = enabel
	printf("ADC_CT_Sensor1  %s\n", str_EnableDisable[pG_Config->Enable_Flag.ADC_CT_Sensor1]);	// 0 = disable,	1 = enabel
	printf("ADC_CT_Sensor2  %s\n", str_EnableDisable[pG_Config->Enable_Flag.ADC_CT_Sensor2]);	// 0 = disable,	1 = enabel
#endif
	printf("TOUCH_Chip_Type		%s\n", str_TouchType[pG_Config->Enable_Flag.TOUCH_Chip_Type]);		// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
	printf("LCD_Type		%s\n", str_LCDType[pG_Config->Enable_Flag.LCD_Type]);				// 0 = disable,	1 = enabel,	2 = x,	3 = x
	printf("Back_Light		%s\n", str_BackLight[pG_Config->Enable_Flag.Back_Light]);
	printf("PWM_Dimming		%s\n", str_3WayPWMType[pG_Config->Enable_Flag.PWM_Dimming]);		// 0 = disable,	1 = PWM1,	2 = PWM2,	3 = 1, 2
	printf("PWM_Color_Temp		%s\n", str_3WayPWMType[pG_Config->Enable_Flag.PWM_Color_Temp]);		// 0 = disable,	1 = PWM1,	2 = PWM2,	3 = 1, 2
	printf("3Way			%s\n", str_3WayPWMType[pG_Config->Enable_Flag.ThreeWay]);			// 0 = disable,	1 = 3��1,	2 = 3��2
	printf("IR			%s\n", str_EnableDisable[pG_Config->Enable_Flag.IR]);				// 0 = disable,	1 = enable
	printf("STPM3x			%s\n", str_STPM3x[pG_Config->Enable_Flag.STPM3x]);					// 0 = disable,	1 = enable
	//--------------------------------------------------------------------------------------------------------------
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------
	for(i=0;i<2;i++)
	{
		for(j=0;j<8;j++)
		{
			//WDG_SetCounter();
			printf("Touch%d Sensitivity%d  [%d]\n", (uint16_t)i, (uint16_t)j, (uint16_t)pG_Config->GT308L[i].Sensitivity[j]);
		}
	}
	//--------------------------------------------------------------------------------------------------------------
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------
	printf("RELAY State ( ");
	item	= ctrl2item(CONTROL_BIT_RELAY_1);		if(item)	printf("RLY1 %s, ", str_OnOff[GET_Switch_State(item2tsn(item))]);
	item	= ctrl2item(CONTROL_BIT_RELAY_2);		if(item)	printf("RLY2 %s, ", str_OnOff[GET_Switch_State(item2tsn(item))]);
	item	= ctrl2item(CONTROL_BIT_RELAY_3);		if(item)	printf("RLY3 %s, ", str_OnOff[GET_Switch_State(item2tsn(item))]);
	item	= ctrl2item(CONTROL_BIT_RELAY_4);		if(item)	printf("RLY4 %s, ", str_OnOff[GET_Switch_State(item2tsn(item))]);
	item	= ctrl2item(CONTROL_BIT_RELAY_LATCH_1);	if(item)	printf("RLY L1 %s, ", str_OnOff[GET_Switch_State(item2tsn(item))]);
	item	= ctrl2item(CONTROL_BIT_RELAY_LATCH_2);	if(item)	printf("RLY L2 %s, ", str_OnOff[GET_Switch_State(item2tsn(item))]);
	item	= ctrl2item(CONTROL_BIT_DIMMING_1);		if(item)	printf("DIM 1 %s, ", str_OnOff[GET_Switch_State(item2tsn(item))]);
	item	= ctrl2item(CONTROL_BIT_DIMMING_2);		if(item)	printf("DIM 2 %s", str_OnOff[GET_Switch_State(item2tsn(item))]);
	printf(")\n");
	WDG_SetCounter();
	
	STATE_Prn();
	
	printf("FACTORY ALL_Light_Switch   ( ");
	WDG_SetCounter();
	for(i=0;i<MAX_RELAY;i++)
	{
		sta	= (uint8_t)((pG_Config->Factory_Mapping_ALL_Light >> i) & 0x01);
		if(sta)	printf("LIGHT Switch%d ", (uint16_t)i+1);
		WDG_SetCounter();
	}
	printf(")\n");
	printf("FACTORY ALL_Elec_Switch   ( ");
	WDG_SetCounter();
	for(i=0;i<MAX_RELAY;i++)
	{
		sta	= (uint8_t)((pG_Config->Factory_Mapping_ALL_Electricity >> i) & 0x01);
		if(sta)	printf("ELEC. Switch%d ", (uint16_t)i+1);
		WDG_SetCounter();
	}
	printf(")\n");
	//--------------------------------------------------------------------------------------------------------------
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------
	printf("Event Relay Put[%d]Get%d]\n", (uint16_t)Relay_CtrlBuff.PUT_Event_Cnt, (uint16_t)Relay_CtrlBuff.GET_Event_Cnt);
	for(i=0;i<MAX_EVENT_BUF;i++)
	{
		if(i == Relay_CtrlBuff.PUT_Event_Cnt && i == Relay_CtrlBuff.GET_Event_Cnt) printf("{{");
		else if(i == Relay_CtrlBuff.PUT_Event_Cnt) printf("P[");
		else if(i == Relay_CtrlBuff.GET_Event_Cnt) printf("G[");
		
		printf("  0x%02x : %d, ", (uint16_t)Relay_CtrlBuff.Ctrl[i], (uint16_t)Relay_CtrlBuff.Flag[i]);
		
		if(i == Relay_CtrlBuff.PUT_Event_Cnt && i == Relay_CtrlBuff.GET_Event_Cnt) printf("}}");
		else if(i == Relay_CtrlBuff.PUT_Event_Cnt) printf("]");
		else if(i == Relay_CtrlBuff.GET_Event_Cnt) printf("]");
		
		if(!(i%5)) printf("\n");
		WDG_SetCounter();
	}
	printf("\n");
	//--------------------------------------------------------------------------------------------------------------
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------
	printf("Event PWM Put[%d]Get%d]\n", (uint16_t)PWM_CtrlBuff.PUT_Event_Cnt, (uint16_t)PWM_CtrlBuff.GET_Event_Cnt);
	for(i=0;i<MAX_EVENT_BUF;i++)
	{
		if(i == PWM_CtrlBuff.PUT_Event_Cnt && i == PWM_CtrlBuff.GET_Event_Cnt) printf("{{");
		else if(i == PWM_CtrlBuff.PUT_Event_Cnt) printf("P[");
		else if(i == PWM_CtrlBuff.GET_Event_Cnt) printf("G[");
		
		printf("  0x%02x : %d, ", (uint16_t)PWM_CtrlBuff.Ctrl[i], (uint16_t)PWM_CtrlBuff.Flag[i]);
		
		if(i == PWM_CtrlBuff.PUT_Event_Cnt && i == PWM_CtrlBuff.GET_Event_Cnt) printf("}}");
		else if(i == PWM_CtrlBuff.PUT_Event_Cnt) printf("]");
		else if(i == PWM_CtrlBuff.GET_Event_Cnt) printf("]");
		
		if(!(i%5)) printf("\n");
		WDG_SetCounter();
	}
	printf("\n");
	Continuity_Prn('-', 100);		//-------------------------------------------------------------------------------
	// printf("ElecLimitCurrent_1_Tmr %d ElecLimitCurrent_2_Tmr %d\n", (uint16_t)Gu8_ElecLimitCurrent_1_Tmr, (uint16_t)Gu8_ElecLimitCurrent_2_Tmr);
	printf("PowerSaving_Tmr %d\n", (uint16_t)Gu8_PowerSaving_Tmr);
#if defined(_ONE_SIZE_LIGHT_1_n_SLEEP_) || defined(_ONE_SIZE_LIGHT_2_n_SLEEP_)
	printf("Sleep_Tmr %dm %ds\n", (uint16_t)Gu16_Light_Sleep_tmr/54, (uint16_t)Gu16_Light_Sleep_tmr%54);		//210621
#endif
	// printf("SWITCH_NUM = %d\r\n", (uint16_t)SWITCH_NUM);		//210621
	
}
/*
uint8_t	light_circuit2item(uint8_t circuit)			// ���� ȸ�ο� ���� �׸� ����
{
	uint8_t	item	= 0;
	
	if(circuit > CIRCUIT_NON && circuit < CIRCUIT_MAX)
	{
		item	= pG_Config->Mapping_LightCircuitI_ITEM[circuit];
	}
	else
	{
		printf("ERR : light_circuit2item Over circuit\n");
	}
	
	return item;
}

uint8_t	light_item2circuit(uint8_t item)			// ���� �׸� ���� ȸ�ι�ȣ ����
{
	uint8_t	circuit	= 0;
	uint8_t	i;
	
	for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
	{
		if(light_circuit2item(i) == item)
		{
			circuit	= i;
			break;
		}
	}
	if(circuit == 0)
	{
		printf("ERR : light_item2circuit non circuit\n");
	}
	
	return circuit;
}

uint8_t	electricity_circuit2item(uint8_t circuit)	// ���� ȸ�ο� ���� �׸� ����
{
	uint8_t	item	= 0;
	
	if(circuit > CIRCUIT_NON && circuit < CIRCUIT_MAX)
	{
		item	= pG_Config->Mapping_ElectricityCircuit_ITEM[circuit];
	}
	else
	{
		printf("ERR : electricity_circuit2item Over circuit\n");
	}
	
	return item;
}

uint8_t	electricity_item2circuit(uint8_t item)		// ���� �׸� ���� ȸ�ι�ȣ ����
{
	uint8_t	circuit	= 0;
	uint8_t	i;
	
	for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
	{
		if(electricity_circuit2item(i) == item)
		{
			circuit	= i;
			break;
		}
	}
	if(circuit == 0)
	{
		printf("ERR : electricity_item2circuit non circuit\n");
	}
	
	return circuit;
}
*/
uint8_t	item2tsn(uint8_t item)						// �׸���� ��ġ����ġ ��ȣ ����
{
	uint8_t	touch_switch	= 0;
	
	if(item > mapping_ITEM_DISABLE && item < mapping_ITEM_MAX)
	{
		touch_switch	= pG_Config->Mapping_ITEM_TSN[item];
	}
	else
	{
		printf("ERR : item2tsn Over item[%d]\n", (uint16_t)item);
	}
	
	return touch_switch;
}

uint8_t	tsn2item(uint8_t touch_switch)				// ��ġ����ġ ���� �׸� ����
{
	uint8_t	item	= 0;
	
	if(touch_switch > mapping_SWITCH_DISABLE && touch_switch < mapping_SWITCH_MAX)	// touch_switch = 1~16
	{
		item	= pG_Config->Mapping_TSN_ITEM[touch_switch-1];
	}
	else
	{
		printf("ERR : tsn2item Over touch switch number[%d]\n", (uint16_t)touch_switch);
	}
	return	item;
}

uint8_t	item2ctrl(uint8_t item)						// �׸���� ����� ����
{
	uint8_t	ctrl	= 0;
	
	if(item > mapping_ITEM_DISABLE && item < mapping_ITEM_MAX)
	{
		ctrl	= pG_Config->Mapping_ITEM_Control[item];
	}
	else
	{
		printf("ERR : item2ctrl Over item[%d]\n", (uint16_t)item);
	}
	
	return ctrl;
}

uint8_t	ctrl2item(uint8_t ctrl)
{
	uint8_t	i;
	uint8_t	item	= 0;
	
	for(i=mapping_ITEM_DISABLE+1;i<mapping_ITEM_MAX;i++)
	{
		if(item2ctrl(i) == ctrl)
		{
			item	= i;
			break;
		}
	}
	
	return	item;
}

uint8_t	eeprom_crc(uint8_t *st_ptr, uint8_t *et_ptr, uint8_t debug_prn)
{
	uint8_t	crc = 0;
	
	crc = 0;
	
	while(st_ptr != et_ptr)
	{
		WDG_SetCounter();
		if(debug_prn)
		{
			printf("%02x, ", (uint16_t)*st_ptr);
		}
		crc	+= *st_ptr;
		st_ptr++;
	}
	if(debug_prn)
	{
		printf(" : CRC %02x\n", (uint16_t)crc);
	}
	return (uint8_t)crc;
}

void USART_Bootloader_Enable(void)
{
	FLASH_Unlock(FLASH_MemType_Data);
	FLASH_ProgramOptionByte(0x480B, 0x55);
	FLASH_ProgramOptionByte(0x480C, 0xAA);
	FLASH_Lock(FLASH_MemType_Data);
}

void USART_Bootloader_Disable(void)
{
	FLASH_Unlock(FLASH_MemType_Data);
	FLASH_ProgramOptionByte(0x480B, 0x00);
	FLASH_ProgramOptionByte(0x480C, 0x00);
	FLASH_Lock(FLASH_MemType_Data);
}

uint32_t Verification_Config(void)
{
	uint32_t	ret = 0;
	uint8_t i;
	uint8_t	item, tsn, cnt;
	
	printf("Config Verification\n");
	
	if(sizeof(EL_STATE) > MAX_EEPROM_DATA)
	{
		printf("data(eeprom) size OV\n");
		ret	= 0x00000001;
	}
	
	if(sizeof(EL_CONFIG) > MAX_FLASH_DATA)
	{
		printf("data(flash)  size OV %d\n", (uint16_t)sizeof(EL_CONFIG));
		ret	= 0x00000002;
	}
	//----------------------------------------------------------------------------------------------
	if((pG_Config->Enable_Flag.ThreeWay & ENABLE_BIT_THREE_WAY_1) && (pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_1))
	{
		printf("ERR : duplicated(Dimming1 & 3Way1)\n");
		ret	= 0x00000004;
	}
	if((pG_Config->Enable_Flag.ThreeWay & ENABLE_BIT_THREE_WAY_2) && (pG_Config->Enable_Flag.PWM_Dimming & ENABLE_BIT_DIMMING_2))
	{
		printf("ERR : duplicated(Dimming2 & 3Way2)\n");
		ret	= 0x00000008;
	}
	//----------------------------------------------------------------------------------------------
	/*
	for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
	{
		item	= light_circuit2item(i);
		switch(item)
		{
			case mapping_ITEM_DISABLE:
			case mapping_ITEM_BATCH_LIGHT_OFF:		// �ϰ��ҵ�
			case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
			case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
			case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
			case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
			case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
			case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
			case mapping_ITEM_3WAY_1:				// 3�� 1 (������ ����)
			case mapping_ITEM_3WAY_2:				// 3�� 2 (������ ����)
			case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
			case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
				break;
			default:
				printf("ERR : LIGHT CIRCUIT %d other item(%d %s)\n", (uint16_t)i, (uint16_t)item, str_mapping[item]);
				ret	= 0x00000010;
				break;
		}
	}

	for(i=CIRCUIT_1;i<CIRCUIT_MAX;i++)
	{
		item	= electricity_circuit2item(i);
		switch(item)
		{
			case mapping_ITEM_DISABLE:
			case mapping_ITEM_ELECTRICITY_1:		// ���� 1 (������ ����)
			case mapping_ITEM_ELECTRICITY_2:		// ���� 2 (������ ����)
				break;
			default:
				printf("ERR : ELEC. CIRCUIT %d other item(%d %s)\n", (uint16_t)i, (uint16_t)item, str_mapping[item]);
				ret	= 0x00000020;
				break;
		}
	}
	*/
	//----------------------------------------------------------------------------------------------
	if(pG_Config->LightCount <= MAX_LIGHT)	// ���� ��
	{
		cnt	= 0;
		for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
		{
			item	= tsn2item(i);
			switch(item)
			{
				case mapping_ITEM_BATCH_LIGHT_OFF:		// �ϰ��ҵ�
				case mapping_ITEM_LIGHT_1:				// ���� 1 (������ ����)
				case mapping_ITEM_LIGHT_2:				// ���� 2 (������ ����)
				case mapping_ITEM_LIGHT_3:				// ���� 3 (������ ����)
				case mapping_ITEM_LIGHT_4:				// ���� 4 (������ ����)
				case mapping_ITEM_LIGHT_5:				// ���� 5 (������ ����)
				case mapping_ITEM_LIGHT_6:				// ���� 6 (������ ����)
					tsn	= item2tsn(item);			// �׸� �Ҵ�� ��ġ����ġ ��ȣ
					if(tsn == mapping_SWITCH_DISABLE || tsn >= mapping_SWITCH_MAX)
					{
						printf("ERR : %s : missing switch number\n", str_mapping[item]);
						ret	= 0x00000040;
					}
					if(!(item2ctrl(item) & LIGHT_RELAY))
					{
						printf("ERR : %s missing control\n", str_mapping[item]);
						ret	= 0x00000080;
					}
					/*
					if(!light_item2circuit(item))
					{
						printf("ERR : %s missing circuit\n", str_mapping[item]);
						ret	= 0x00000100;
					}
					*/
					cnt++;
					break;
			}
		}
		if(pG_Config->LightCount != cnt)
		{
			printf("ERR : Light Cnt != Config Light Cnt\n");
			ret	= 0x00000200;
		}
	}
	else
	{
		printf("ERR : Light Over Cnt\n");
		ret	= 0x00000400;
	}
	//----------------------------------------------------------------------------------------------
	if(pG_Config->ThreeWayCount <= MAX_3WAY)	// 3�� ��
	{
		cnt	= 0;
		for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
		{
			item	= tsn2item(i);
			switch(item)
			{
				case mapping_ITEM_3WAY_1:				// 3�� 1 (������ ����)
				case mapping_ITEM_3WAY_2:				// 3�� 2 (������ ����)
					tsn	= item2tsn(item);			// �׸� �Ҵ�� ��ġ����ġ ��ȣ
					if(tsn == mapping_SWITCH_DISABLE || tsn >= mapping_SWITCH_MAX)
					{
						printf("ERR : %s : missing switch number\n", str_mapping[item]);
						ret	= 0x00000800;
					}
					if(!(item2ctrl(item) & LIGHT_RELAY))
					{
						printf("ERR : %s missing control\n", str_mapping[item]);
						ret	= 0x00001000;
					}
					/*
					if(!light_item2circuit(item))
					{
						printf("ERR : %s missing circuit\n", str_mapping[item]);
						ret	= 0x00002000;
					}
					*/
					cnt++;
					break;
			}
		}
		if(pG_Config->ThreeWayCount != cnt)
		{
			printf("ERR : 3Way Cnt != Config 3Way Cnt\n");
			ret	= 0x00004000;
		}
	}
	else
	{
		printf("ERR : 3Way Over Cnt\n");
		ret	= 0x00008000;
	}
	//----------------------------------------------------------------------------------------------
	if(pG_Config->DimmingCount <= MAX_DIMMING)	// ��� ��
	{
		cnt	= 0;
		for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
		{
			item	= tsn2item(i);
			switch(item)
			{
				case mapping_ITEM_DIMMING_LIGHT_1:		// ���1 ����ġ(PWM ����)
				case mapping_ITEM_DIMMING_LIGHT_2:		// ���2 ����ġ(PWM ����)
					tsn	= item2tsn(item);			// �׸� �Ҵ�� ��ġ����ġ ��ȣ
					if(tsn == mapping_SWITCH_DISABLE || tsn >= mapping_SWITCH_MAX)
					{
						printf("ERR : %s : missing switch number\n", str_mapping[item]);
						ret	= 0x00010000;
					}
					if(!(item2ctrl(item) & DIMMING_PWM))
					{
						printf("ERR : %s missing control\n", str_mapping[item]);
						ret	= 0x00020000;
					}
					/*
					if(!light_item2circuit(item))
					{
						printf("ERR : %s missing circuit\n", str_mapping[item]);
						ret	= 0x00040000;
					}
					*/
					cnt++;
					break;
			}
		}
		if(pG_Config->DimmingCount != cnt)
		{
			printf("ERR : Dimming Cnt != Config Dimming Cnt\n");
			ret	= 0x00080000;
		}
	}
	else
	{
		printf("ERR : Dimming Over Cnt\n");
		ret	= 0x00100000;
	}
	//----------------------------------------------------------------------------------------------
	if(pG_Config->ElectricityCount<= MAX_ELECTRICITY)	// ���� ��
	{
		cnt	= 0;
		for(i=mapping_SWITCH_1;i<mapping_SWITCH_MAX;i++)
		{
			item	= tsn2item(i);
			switch(item)
			{
				case mapping_ITEM_ELECTRICITY_1:		// ���� 1 (������ ����)
				case mapping_ITEM_ELECTRICITY_2:		// ���� 2 (������ ����)
					tsn	= item2tsn(item);			// �׸� �Ҵ�� ��ġ����ġ ��ȣ
					if(tsn == mapping_SWITCH_DISABLE || tsn >= mapping_SWITCH_MAX)
					{
						printf("ERR : %s : missing switch number\n", str_mapping[item]);
						ret	= 0x00200000;
					}
					if(!(item2ctrl(item) & ELECTRICITY_RELAY))
					{
						printf("ERR : %s missing control\n", str_mapping[item]);
						ret	= 0x00400000;
					}
					/*
					if(!electricity_item2circuit(item))
					{
						printf("ERR : %s missing circuit\n", str_mapping[item]);
						ret	= 0x00800000;
					}
					*/
					cnt++;
					break;
			}
		}
		if(pG_Config->ElectricityCount != cnt)
		{
			printf("ERR : Electricity Cnt != Config Electricity Cnt\n");
			ret	= 0x10000000;
		}
	}
	else
	{
		printf("ERR : Electricity Over Cnt\n");
		ret	= 0x20000000;
	}
	//----------------------------------------------------------------------------------------------
	return ret;
}

void RS485_ID_Init(void)
{
#if defined(_KOCOM_PROTOCOL_) || defined(_NO_PROTOCOL_)		//����ġ ID, 0���� ����
	pG_Config->RS485_ID = (uint8_t)(RS_ID - 1);
	#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_) 
	pG_Config->RS485_Elec_ID = (uint8_t)(RS_ID - 1);		//KOCOM���� �����
	#else	//���� ��� ���� �ƴϸ� 0.
	pG_Config->RS485_Elec_ID = 0;
	#endif
#elif defined(_HYUNDAI_PROTOCOL_) || defined(_SAMSUNG_PROTOCOL_) || defined(_KDW_PROTOCOL_)	|| defined(_HW_PROTOCOL_) || defined(_HDC_PROTOCOL_)	//COMMAX, SAMSUNG�� ȸ�� ID, CVNET�� ����ġ ID, 1���� ����
	pG_Config->RS485_ID = RS_ID;
#elif defined(_COMMAX_PROTOCOL_)
	pG_Config->RS485_ID = RS_ID;
	#if defined(_ONE_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_MODEL_) || defined(_TWO_SIZE_LIGHT_n_ELEC_n_DIM_n_COLOR_MODEL_) 
	pG_Config->RS485_Elec_ID = RS_ELEC_ID;
	#else
	pG_Config->RS485_Elec_ID = 0;
	#endif
#elif defined(_CVNET_PROTOCOL_)
#ifdef _ONE_SIZE_BATCH_BLOCK_MODEL_
#ifdef _BATCH_BLOCK_SWITCH_PROTOCOL_
	if(RS_ID == 1)		pG_Config->RS485_ID = 0x0B;		//1���� ����
	else if(RS_ID == 2)	pG_Config->RS485_ID = 0x0C;		//1���� ����
	else if(RS_ID == 3)	pG_Config->RS485_ID = 0x0D;		//1���� ����
	else if(RS_ID == 4)	pG_Config->RS485_ID = 0x0E;		//1���� ����
	else				pG_Config->RS485_ID = 0x0B;		//1���� ����
#else	//_TOTAL_SITWCH_PROTOCOL_
	pG_Config->RS485_ID = RS_ID;
#endif
#else
	pG_Config->RS485_ID = RS_ID;
#endif
#endif	//_CVNET_PROTOCOL_
/*#if defined(_ONE_SIZE_BATCH_BLOCK_MODEL_) || defined(_TWO_SIZE_LIGHT_MODEL_) ||  defined(_ONE_SIZE_LIGHT_MODEL_)	//������ �ش� �� LED MODE�� ���� �� ���� �־ ���� ��ǰ�� �߿��� �־��� ��� ��� ���� ���ؼ� ������
	SET_State_Defaultdata();
#endif*/
	Store_CurrentConfig();
	// SET_Defaultdata();
	printf("Rs485 ID init, ID : %d\r\n", (uint16_t)pG_Config->RS485_ID);
}

//�ڸƽ��� ��� ID�� ȸ�� ��ȣ��.
//���� 1�� ����ġ�� ������� ����ġ�� �������2, ����4��� ID ������ ���� ID : 1, ��� ID : 1
//���� 2�� ����ġ�� ������� ����ġ�� �������2, ����3��� ID ������ ���� ID : 5, ��� ID : 3
//���� 3�� ����ġ�� ������� ����ġ�� �������2, ����2��� ID ������ ���� ID : 8, ��� ID : 5

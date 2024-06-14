/************************************************************************************
	Project		: 전자식스위치
	File Name	: DEBUG.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "cmdparse.h"
#include "Debug.h"
#include "el_switch.h"
#include "adc.h"
#include "dimming.h"
#include "WDGnBeep.h"
#include "Relay.h"
#include "Timer.h"
#include "i2c.h"
#include "STPM3x_opt.h"
#include "Touch.h"		//20201112
#include "RS-485.h"
#include "LCD.h"

#define STX	0x02
#define ETX	0x03
#define ACK	0x06
#define NAK	0x15

#define CR	0x0d
#define LF	0x0a
#define BS	0x08
#define ESC 0x1b

uint8_t	G_Debug		= 0;
uint8_t	G_Trace		= 0;
//-------------------------------------------------------------------------------------------------------------------------
#ifdef	_ADC_
int SET_CT_Offset(int argc, char *argv[]);
#endif

#ifndef	_OPTIMIZE_
int Debug_i2c(int argc, char *argv[]);
int md(int argc, char *argv[]);
#endif

int FirmwareDownload(void);
u32 atoh(char *s);

int DebugCmd(int argc, char *argv[]);
int DebugRealCmd(int argc, char *argv[]);
int DebugOFFCmd(int argc, char *argv[]);
int DoHelp (int argc, char *argv[]);
void ClearScreen(void);
void BackSpace(void);

int SoftReset(void);

int SET_touch_sensitivity(int argc, char *argv[]);
void Touch_Init(void);
int RS485_ID_Change(int argc, char *argv[]);
int elec_control(int argc, char *argv[]);
int light_control(int argc, char *argv[]);
int Power_Set(void);
int Power_Reset(void);
int IR_Debug(void);
//-------------------------------------------------------------------------------------------------------------------------
void irq_Debug_TX(void)
{
	uint8_t data;
	
	if(USART_GetITStatus(DEBUG_USART, USART_IT_TC) != RESET)
	{
		if(Uart_GetTxQ(DEBUG_PORT, &data) != 0)
		{
			USART_SendData8(DEBUG_USART, data);
			USART_ClearITPendingBit(DEBUG_USART, USART_IT_TC);
		}
		else
		{
			USART_ITConfig(DEBUG_USART, USART_IT_TC, DISABLE);		// Disable the USART Transmit Complete interrupt
			//USART_ClearITPendingBit(DEBUG_USART, USART_IT_TC);	// ???
		}
	}
}

void irq_Debug_RX(void)
{
	uint8_t data;
	
	//GPIO_TOGGLE(LED_GREEN_PORT,LED_GREEN_PIN);
	if(USART_GetITStatus(DEBUG_USART, USART_IT_RXNE) != RESET)
	{
		data = USART_ReceiveData8(DEBUG_USART);
		Uart_PutRxQ(DEBUG_PORT, data);								// Read one byte from the receive data register
	}
}
//-------------------------------------------------------------------------------------------------------------------------
void Debug_Init(void)
{
	//G_Debug	= 0;
	//G_Trace	= 0;

	CLK_PeripheralClockConfig((CLK_Peripheral_TypeDef)DEBUG_PORT_CLK, ENABLE);	// Enable DEBUG USART clock
	GPIO_ExternalPullUpConfig(DEBUG_TX_PORT, DEBUG_TX_PIN, ENABLE);				// DEBUG Tx as alternate function push-pull
	GPIO_ExternalPullUpConfig(DEBUG_RX_PORT, DEBUG_RX_PIN, ENABLE);				// DEBUG Rx as alternate function push-pull
	USART_Init(DEBUG_USART, 115200, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
	//-------------------------------------------------------------------------------------------------------------------------
	USART_ITConfig(DEBUG_USART, USART_IT_RXNE, ENABLE);
	//USART_ITConfig(DEBUG_USART, USART_IT_TC, ENABLE);		// Enable the USART Transmit complete interrupt
	USART_Cmd(DEBUG_USART, ENABLE);
	//printf("Dubug Init");
	//-------------------------------------------------------------------------------------------------------------------------
}
//-------------------------------------------------------------------------------------------------------------------------
/*
int Beep_test(int argc, char *argv[])
{
	uint8_t	data;
	
	if(strcmp(argv[1],"t1") == 0)
	{
		BEEP->CSR2 &= (uint8_t)(~BEEP_CSR2_BEEPSEL);
		BEEP->CSR2 |= (uint8_t)(BEEP_Frequency_4KHz);
		BEEP->CSR2 &= (uint8_t)(~BEEP_CSR2_BEEPDIV);
		BEEP->CSR2 |= 23;		// 도
		Beep(ON);
		Gu16_1ms_delay_tmr	= 100;
		while(Gu16_1ms_delay_tmr)	WDG_SetCounter();
		//Beep(OFF);
		
		BEEP->CSR2 &= (uint8_t)(~BEEP_CSR2_BEEPSEL);
		BEEP->CSR2 |= (uint8_t)(BEEP_Frequency_4KHz);
		BEEP->CSR2 &= (uint8_t)(~BEEP_CSR2_BEEPDIV); 
		BEEP->CSR2 |= 16;		// 레
		Beep(ON);
		Gu16_1ms_delay_tmr	= 200;
		while(Gu16_1ms_delay_tmr)	WDG_SetCounter();
		//Beep(OFF);
		
	}
	else
	{
		if(strcmp(argv[1],"1") == 0)
		{
			printf("1K\t");
			BEEP->CSR2 &= (uint8_t)(~BEEP_CSR2_BEEPSEL);
			BEEP->CSR2 |= (uint8_t)(BEEP_Frequency_1KHz);
		}
		if(strcmp(argv[1],"2") == 0)
		{
			printf("2K\t");
			BEEP->CSR2 &= (uint8_t)(~BEEP_CSR2_BEEPSEL);
			BEEP->CSR2 |= (uint8_t)(BEEP_Frequency_2KHz);
		}
		if(strcmp(argv[1],"4") == 0)
		{
			printf("4K\t");
			BEEP->CSR2 &= (uint8_t)(~BEEP_CSR2_BEEPSEL);
			BEEP->CSR2 |= (uint8_t)(BEEP_Frequency_4KHz);
		}
		
		data = (uint8_t)atoi(argv[2]);
		
		if(data <= 31)
		{
			printf("beep %d\n", (uint16_t)data);
			
			BEEP->CSR2 &= (uint8_t)(~BEEP_CSR2_BEEPDIV);
		    BEEP->CSR2 |= data;		// max 31
		    
		    Beep(ON);
		}
	}
	
	return 1;
}
*/
int limit_test(int argc, char *argv[])
{
	if(atoi(argv[1]) == 1)	Gu16_ElecLimitCurrent_1 = atoi(argv[2]);
	if(atoi(argv[1]) == 2)	Gu16_ElecLimitCurrent_2 = atoi(argv[2]);
	
	Store_ElecLimitCurrent();
	return 1;
}

extern int Debug_Beep_PWM(int argc, char *argv[]);

const struct cmds cmds[] = 
{
	//"beep",			(PointerAttr void*)Debug_Beep_PWM,		1,	NULLCHAR,	NULLCHAR,
	
	"fwdn",			(PointerAttr void*)FirmwareDownload,	1,	NULLCHAR,	NULLCHAR,
	"reset",		(PointerAttr void*)SoftReset,			1,	NULLCHAR,	NULLCHAR,
	
#ifdef	STPM3x_OFFSET_SETTING
	"offset",		(PointerAttr void*)SET_STPM3x_Power_Offset,	1,	NULLCHAR,	NULLCHAR,
#endif
	
	"calibration",	(PointerAttr void*)SET_STPM3x_Calibration,	1,	NULLCHAR,	NULLCHAR,
	"cal",			(PointerAttr void*)SET_STPM3x_Calibration,	1,	NULLCHAR,	NULLCHAR,
	
#ifndef		_OPTIMIZE_
	"i2c",			(PointerAttr void*)Debug_i2c,			1,	NULLCHAR,	NULLCHAR,
	"md",			(PointerAttr void*)md,					2,	"hex",		NULLCHAR,
	"dfreq",		(PointerAttr void*)Debug_Dimming_Freq,	2,	"hex",		NULLCHAR,
#endif

#ifdef	_ADC_
	"adcoffset",	(PointerAttr void*)SET_CT_Offset,		1,	NULLCHAR,	NULLCHAR,
#endif
	
	"help",			(PointerAttr void*)DoHelp,				1,	NULLCHAR,	NULLCHAR,
  	"?",			(PointerAttr void*)DoHelp,				1,	NULLCHAR,	NULLCHAR,
  	"defaultdata",	(PointerAttr void*)SET_Defaultdata,		1,	NULLCHAR,	NULLCHAR,
	"defaultstate",	(PointerAttr void*)SET_State_Defaultdata,		1,	NULLCHAR,	NULLCHAR,
  	"state",		(PointerAttr void*)Debug_StateData,		1,	NULLCHAR,	NULLCHAR,
  	
  	//"f",			(PointerAttr void*)Flash_Write_Test,	1,	NULLCHAR,	NULLCHAR,
  	
	"debug",		(PointerAttr void*)DebugCmd,			2,	"debug <cmd>",	NULLCHAR,
	"real",			(PointerAttr void*)DebugRealCmd,		2,	"real <cmd>",	NULLCHAR,
  	"off",			(PointerAttr void*)DebugOFFCmd,			1,	NULLCHAR,	NULLCHAR,
  	
  	"touch",		(PointerAttr void*)SET_touch_sensitivity,	1,	NULLCHAR,	NULLCHAR,
	"tp",			(PointerAttr void*)SET_touch_sensitivity,	1,	NULLCHAR,	NULLCHAR,
	"rs",			(PointerAttr void*)RS485_ID_Change,			3,	"rs <cmd>",		"int",
	"cf",			(PointerAttr void*)Store_CurrentConfig,		1,	NULLCHAR,	NULLCHAR,
	"init",			(PointerAttr void*)RS485_ID_Init,			1,	NULLCHAR,	NULLCHAR,
	"lcd",			(PointerAttr void*)LCD_TEST,				2,	"int",		NULLCHAR,
	"elec",			(PointerAttr void*)elec_control,			2,	"int",		NULLCHAR,
	"light",		(PointerAttr void*)light_control,			2,	"int",		NULLCHAR,	
	// "treset",		(PointerAttr void*)Touch_Chip_Reset,		1,	NULLCHAR,	NULLCHAR,		//20201112
	// "tsend",			(PointerAttr void*)touchsend,				1,	NULLCHAR,	NULLCHAR,		//20201112
	// "tset",			(PointerAttr void*)touchchiptest,			1,	NULLCHAR,	NULLCHAR,		//20201112
	"strst",		(PointerAttr void*)Power_Reset,				1,	NULLCHAR,		NULLCHAR,
	"stset",		(PointerAttr void*)Power_Set,				1,	NULLCHAR,		NULLCHAR,
	"ir",			(PointerAttr void*)IR_Debug,				1,	NULLCHAR,		NULLCHAR,
	"lm",			(PointerAttr void*)limit_test,				1,	NULLCHAR,		NULLCHAR,
  	(0), (0), 0, NULLCHAR, NULLCHAR
};
//-------------------------------------------------------------------------------------------------------------------------
int SET_touch_sensitivity(int argc, char *argv[])
{
	uint16_t	i, j;
	uint16_t	chip, point = 0;
	uint8_t		sen;
	
	if(argc >= 2)
	{
		if(strcmp(argv[1],"la") == 0)		point	= item2tsn(mapping_ITEM_LIGHT_ALL);
		else if(strcmp(argv[1],"ea") == 0)	point	= item2tsn(mapping_ITEM_ELECTRICITY_ALL);
		else if(strcmp(argv[1],"lg") == 0)	point	= item2tsn(mapping_ITEM_LIGHT_GROUP);
		
		else if(strcmp(argv[1],"l1") == 0)	point	= item2tsn(mapping_ITEM_LIGHT_1);
		else if(strcmp(argv[1],"l2") == 0)	point	= item2tsn(mapping_ITEM_LIGHT_2);
		else if(strcmp(argv[1],"l3") == 0)	point	= item2tsn(mapping_ITEM_LIGHT_3);
		else if(strcmp(argv[1],"l4") == 0)	point	= item2tsn(mapping_ITEM_LIGHT_4);
		else if(strcmp(argv[1],"l5") == 0)	point	= item2tsn(mapping_ITEM_LIGHT_5);
		else if(strcmp(argv[1],"l6") == 0)	point	= item2tsn(mapping_ITEM_LIGHT_6);
		
		else if(strcmp(argv[1],"e1") == 0)	point	= item2tsn(mapping_ITEM_ELECTRICITY_1);
		else if(strcmp(argv[1],"e2") == 0)	point	= item2tsn(mapping_ITEM_ELECTRICITY_2);
		
		else if(strcmp(argv[1],"bl") == 0)	point	= item2tsn(mapping_ITEM_BATCH_LIGHT_OFF);
		else if(strcmp(argv[1],"gas") == 0)	point	= item2tsn(mapping_ITEM_GAS);
		else if(strcmp(argv[1],"el") == 0)	point	= item2tsn(mapping_ITEM_ELEVATOR);
		
		else if(strcmp(argv[1],"el") == 0)	point	= item2tsn(mapping_ITEM_ELEVATOR);
			
		else if(strcmp(argv[1],"3l1") == 0)	point	= item2tsn(mapping_ITEM_3WAY_1);
		else if(strcmp(argv[1],"3l2") == 0)	point	= item2tsn(mapping_ITEM_3WAY_2);
		else if(strcmp(argv[1],"d1") == 0)	point	= item2tsn(mapping_ITEM_DIMMING_LIGHT_1);
		else if(strcmp(argv[1],"d2") == 0)	point	= item2tsn(mapping_ITEM_DIMMING_LIGHT_2);
		else if(strcmp(argv[1],"up") == 0)	point	= item2tsn(mapping_ITEM_DIMMING_UP);
		else if(strcmp(argv[1],"dn") == 0)	point	= item2tsn(mapping_ITEM_DIMMING_DN);
		
		else if(strcmp(argv[1],"s") == 0)	point	= item2tsn(mapping_ITEM_SETUP);
		
		if(point >= 1 && point <= 16)
		{
			if(point >= 1 && point <= 8)
			{
				chip	= 0;
				point	= point - 1;
			}
			else
			{
				chip	= 1;
				point	= point - 9;
			}
			
			sen = (uint8_t)atoi(argv[2]);
			if(sen && sen <= 63)
			{
				printf("SET touch %d, point %d : sensitivity %d\n", chip+1, point+1, (uint16_t)(sen));
				TouchConfig.GT308L[chip].Sensitivity[point]	= sen;
				pG_Config->GT308L[chip].Sensitivity[point]	= sen;
				while(!I2C_OPT_WriteRegister(TouchConfig.Address[chip], 0x40, (void *)&TouchConfig.GT308L[chip].Sensitivity[0], 12));
				Store_CurrentConfig();
				SoftReset();
			}
			else	printf("ERR : touch sensitivity 1 ~ 63\n");
		}
		else
		{
			printf("ERR : Unknown item(config check)\n");
			printf("tp cmd xx\t cmd = la, lg, ea, l1, l2, l3, l4, l5, l6, e1, e2, bl, gas, el, 3l1, 3l2, d1, d2, up, dn, s\n");
		}
		
	}
	
	return 1;
}
//-------------------------------------------------------------------------------------------------------------------------
#ifndef	_OPTIMIZE_
int Debug_i2c(int argc, char *argv[])
{
	uint8_t i, reg, len, data;
	uint8_t buff[10];
	
	for(i=0;i<10;i++)
	{
		buff[i]	= 0;
	}
	
	if(strcmp(argv[1],"sen") == 0)		// register address set
	{
		if(argc == 3)
		{
			data = (uint8_t)atoi(argv[2]);
			if(data >= 64)
			{
				data	= 63;
			}
			
			for(i=0;i<8;i++)
			{
				TouchConfig.GT308L[0].Sensitivity[i]	= data;
				TouchConfig.GT308L[1].Sensitivity[i]	= data;
			}
			while (!I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[0], 0x40));
			while (!I2C_OPT_ReadRegister(TouchConfig.Address[0], (void *)&TouchConfig.GT308L[i].Sensitivity[0], 8));
			while (!I2C_OPT_SET_ReadRegisterAddr(TouchConfig.Address[1], 0x40));
			while (!I2C_OPT_ReadRegister(TouchConfig.Address[1], (void *)&TouchConfig.GT308L[i].Sensitivity[0], 8));
		}
		for(i=0;i<8;i++)
		{
			printf("TC[1] Sensitivity %d : %d\n", (uint16_t)i+1, (uint16_t)TouchConfig.GT308L[0].Sensitivity[i]);
		}
		printf("\n\n");
		for(i=0;i<8;i++)
		{
			printf("TC[2] Sensitivity %d : %d\n", (uint16_t)i+1, (uint16_t)TouchConfig.GT308L[1].Sensitivity[i]);
		}
		printf("\n\n");
	}
	else if(strcmp(argv[1],"init1") == 0)		// register address set
	{
		Gu8_I2C_1ms_Tmr	= 0;
		GPIO_SetBits(TOUCH_RESET_PORT, TOUCH_RESET_PIN);
		delay(2);		// 2ms
		GPIO_ResetBits(TOUCH_RESET_PORT, TOUCH_RESET_PIN);
		delay(10);		// 10ms
		
		memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
		Touch_Chip_Init();
	}
	else if(strcmp(argv[1],"init2") == 0)		// register address set
	{
		memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
	}
	else if(strcmp(argv[1],"init") == 0)		// register address set
	{
		memset((void*)&cmpTouchConfig, 0, sizeof(TOUCH_CFG));
		Touch_Chip_Init();
	}
	else if(strcmp(argv[1],"wr1") == 0)		// register address set
	{
		if(argc == 4)
		{
			reg = (uint8_t)atoh(argv[2]);
			data = (uint8_t)atoh(argv[3]);
			printf("I2c SET WRITE Register : %02x, data %02x\n", (uint16_t)reg, (uint16_t)data);
			while (!I2C_OPT_WriteRegister(TOUCH_1_ADDR, reg, &data, 1));
		}
	}
	else if(strcmp(argv[1],"wr2") == 0)		// register address set
	{
		if(argc == 4)
		{
			reg = (uint8_t)atoh(argv[2]);
			data = (uint8_t)atoh(argv[3]);
			printf("I2c SET WRITE Register : %02x, data %02x\n", (uint16_t)reg, (uint16_t)data);
			while (!I2C_OPT_WriteRegister(TOUCH_2_ADDR, reg, &data, 1));
		}
	}
	else if(strcmp(argv[1],"rd1") == 0)
	{
		if(argc >= 2)
		{
			reg = (uint8_t)atoh(argv[2]);
			len	= 1;
			if(argc == 4)
			{
				len = (uint8_t)atoh(argv[3]);
				if(len > 10)	len = 10;
			}
			printf("I2c READ Register : %02x len%d\n", (uint16_t)reg, (uint16_t)len);
			while (!I2C_OPT_SET_ReadRegisterAddr(TOUCH_1_ADDR, reg));
			while (!I2C_OPT_ReadRegister(TOUCH_1_ADDR, (void *)&buff, len) );
			while(I2C_OPT_Busy());		// 데이터 수신완료
			
			for(i=0;i<10;i++)
			{
				printf("%02x ", (uint16_t)buff[i]);
			}
			printf("\n");
			
		}
	}
	else if(strcmp(argv[1],"rd2") == 0)
	{
		if(argc >= 2)
		{
			reg = (uint8_t)atoh(argv[2]);
			len	= 1;
			if(argc == 4)
			{
				len = (uint8_t)atoh(argv[3]);
				if(len > 10)	len = 10;
			}
			printf("I2c READ Register : %02x len%d\n", (uint16_t)reg, (uint16_t)len);
			while (!I2C_OPT_SET_ReadRegisterAddr(TOUCH_2_ADDR, reg));
			while (!I2C_OPT_ReadRegister(TOUCH_2_ADDR, (void *)&buff, len) );
			while(I2C_OPT_Busy());		// 데이터 수신완료
			
			for(i=0;i<10;i++)
			{
				printf("%02x ", (uint16_t)buff[i]);
			}
			printf("\n");
			
		}
	}
	
	return 1;
}

int md(int argc, char *argv[])
{
	u32	i;
	u8	*ptr;
	
	i = atoh(argv[1]);
	printf("%x ~ %x\n", i, (u32)(i+255));
	
	ptr = (void*)i;
	
	//(*(PointerAttr uint8_t *) (MemoryAddressCast)Address);
	
	for(i=0;i<256;i++)
	{
		if( (i%16) == 0 ) printf("%x : ", (ptr+i));
		//printf("%02x ", (uint16_t)(*(ptr++)));
		printf("%02x ", (uint16_t)(*(PointerAttr uint8_t *)(MemoryAddressCast)(ptr++)));
		if( (i%16) == 15) printf("\n");
	}
	printf("\n");
	
	return 1;
}
#endif

int SoftReset(void)
{
	while(1);
	
	return 1;
}

#ifdef	_ADC_
uint8_t	Gu8_ADC_Offset_Cnt;
uint8_t	Gu8_ADC_Offset_1_Flag;
uint8_t	Gu8_ADC_Offset_2_Flag;

int SET_CT_Offset(int argc, char *argv[])
{
	double tmp = 0.0;
	
	if(argc >= 2)
	{
		if(strcmp(argv[1],"ct1") == 0)
		{
			pG_Config->CT1_Offset = (double)atof(argv[2]);
		}
		else if(strcmp(argv[1],"ct2") == 0)
		{
			pG_Config->CT2_Offset = (double)atof(argv[2]);
		}
		else if(strcmp(argv[1],"ct") == 0)
		{
			Gu8_ADC_Offset_1_Flag	= 0xFF;
			Gu8_ADC_Offset_2_Flag	= 0xFF;
			if(ctrl2item(CONTROL_BIT_RELAY_LATCH_1))
			{
				Gu8_ADC_Offset_1_Flag = GET_Switch_State(item2tsn(ctrl2item(CONTROL_BIT_RELAY_LATCH_1)));
			}
			if(ctrl2item(CONTROL_BIT_RELAY_LATCH_2))
			{
				Gu8_ADC_Offset_2_Flag = GET_Switch_State(item2tsn(ctrl2item(CONTROL_BIT_RELAY_LATCH_2)));
			}
			
			Relay_Ctrl(CONTROL_BIT_RELAY_LATCH_1 | CONTROL_BIT_RELAY_LATCH_2, OFF);
			Relay_Ctrl(CONTROL_BIT_RELAY_LATCH_1 | CONTROL_BIT_RELAY_LATCH_2, OFF);
			
			pG_Config->CT1_Offset	= 0;
			pG_Config->CT2_Offset	= 0;
			
			Gu8_ADC_Offset_Cnt		= 5;	// 4초간
		}
		else if(strcmp(argv[1],"acv") == 0)
		{
			Gdbl_ACV	= Gdbl_ACV - pG_Config->ACV_Offset;
			pG_Config->ACV_Offset = (double)atof(argv[2]);
			if(fabs(pG_Config->ACV_Offset) > 100.0)
			{
				pG_Config->ACV_Offset	= pG_Config->ACV_Offset - Gdbl_ACV;
			}
			printf("\n\nACV_Offset %f\n\n", pG_Config->ACV_Offset);
			
			Store_CurrentConfig();
		}
	}
	
	printf("ACV_Offset %f\n", pG_Config->ACV_Offset);
	printf("CT1_Offset %f\n", pG_Config->CT1_Offset);
	printf("CT2_Offset %f\n", pG_Config->CT2_Offset);
	
	return 1;
}
#endif

int FirmwareDownload(void)
{
	printf("Firmware Download\n");
	//CFG->GCR &= (uint8_t)~CFG_GCR_SWD;	// disable SWIM interface
	USART_Bootloader_Enable();
	while(1);
}

//-------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------

#define	MAX_DEBUG_LEN	30

static char debug_buf[MAX_DEBUG_LEN];
static char debug_cnt = 0;
static char	G_recmdBuf[MAX_DEBUG_LEN];
static char	G_recmdLen = 0;

void Debug_Process(void)
{
	uint8_t data;
	
	while(Uart_GetRxQ(DEBUG_PORT, (uint8_t*)&data))
	{
		if(data == CR || data == LF)
		{
			debug_buf[debug_cnt] = 0;
			if(debug_cnt >= 1)
			{
				G_recmdLen = debug_cnt;
				memcpy(G_recmdBuf, debug_buf, debug_cnt);
			}
			
			cmdparse(cmds,debug_buf);		//Debug command parsing
			printf("\nES Switch[Ver:%03d]>>", VERSION);
			
			debug_cnt = 0;
		}
		else if(data == BS)
		{
			if(debug_cnt > 0)
			{
				debug_cnt--;
				//UartPut(DEBUG_PORT, data);
				putchar(data);
				BackSpace();
			} 
		}
		else
		{
			if(data == STX)
			{
				debug_cnt = 0;
				debug_buf[0] = 0;
			}
			if( (debug_cnt == 0) && (data == 0x20) )	// 처음에 스페이스바를 입력하면 이전의 명령을 다시 사용할 수 있다
			{
				debug_cnt = G_recmdLen;
				memcpy(debug_buf, G_recmdBuf, G_recmdLen);
				printf("%s", debug_buf);
			}
			else
			{
				debug_buf[debug_cnt++] = data;
				if(debug_buf[0] != STX)
				{
					//UartPut(DEBUG_PORT, data);
					putchar(data);
					if(debug_cnt >= MAX_DEBUG_LEN)
					{
						debug_cnt = 0;
						debug_buf[0] = 0;
					}
				}
			}
		}
	}
	
}
//-------------------------------------------------------------------------------------------------------------------------
int DebugCmd(int argc, char *argv[])
{
	if(strcmp(argv[1],"trace") == 0)
	{
		printf("Debug trace\n");
		G_Trace = 1;
	}
	else if(strcmp(argv[1],"host") == 0)
	{
		printf("Debug host\n");
		G_Debug = DEBUG_HOST;
	}
	else if(strcmp(argv[1],"stpm") == 0)
	{
		printf("Debug stpm3x\n");
		G_Debug = DEBUG_STPM3x;
	}
	else if(strcmp(argv[1],"stpmdata") == 0)
	{
		printf("Debug stpm3x data\n");
		G_Debug = DEBUG_STPM3x_DATA;
	}
	else printf("Error: Unknown cmd!\n");
	
	return 1;
}

int DebugRealCmd(int argc, char *argv[])
{
	if(strcmp(argv[1],"host") == 0)
	{
		printf("Debug realdata\n");
		G_Debug = DEBUG_HOST_REALDATA;
	}
	else if(strcmp(argv[1],"stpm") == 0)
	{
		printf("Debug realstpm3x\n");
		G_Debug = DEBUG_STPM3x_REALDATA;
	}
	else printf("Error: Unknown cmd!\n");
	
	return 1;
}

int DebugOFFCmd(int argc, char *argv[])
{
	printf("Debug OFF\n");
	G_Trace	= 0;
	G_Debug	= 0;
	return 1;
}
//-------------------------------------------------------------------------------------------------------------------------
int DoHelp (int argc, char *argv[])
{
  	const struct cmds *cmdp;
  	int i, j;
	
	printf ("\nMain commands:\n");
	if(G_Trace)
	{
  		
  		for (i = 0, cmdp = cmds; cmdp->name != NULL; cmdp++, i++)
    	{
      		printf ("%s", cmdp->name);
      		if ((i % 4) == 3)
      		{
				printf("\n");
      		}
      		else
      		{
	  			for (j = strlen(cmdp->name); j < 16; j++)
	  			{
	    			printf(" ");
	    		}
			}
    	}
  		if ((i % 4) != 0)
  		{
  			printf("\n");
  		}
	}
  	
  	return 1;
}
//-------------------------------------------------------------------------------------------------------------------------
/* Terminal Emulation(ANSI Utility) */
void ClearScreen(void)
{
	printf("[2J");
}

void BackSpace(void)
{
	printf("[1P");
}

u32 atoh(char *s)
{                   
	u32	i,n;        
	                
	for(n=i=0; (s[i] != '\0') && (s[i] != ' '); i++){
		if(s[i] >= '0' && s[i] <= '9')
			n = 16*n+s[i]-'0';
		else if(s[i] >= 'a' && s[i] <= 'f')
			n = 16*n+s[i]-'a'+10;
		else if(s[i] >= 'A' && s[i] <= 'F')
			n = 16*n+s[i]-'A'+10;
		else {      
			//printf("\nError : Non hexadecimal char (0x%02x)!!",s[i]);
			return(n);
		}           
	}               
	return(n);      
}

void Continuity_Prn(uint8_t prn, uint8_t cnt)
{
	uint8_t i;
	
	WDG_SetCounter();
	for(i=0;i<cnt;i++)
	{
		printf("%c", prn);
	}
	printf("\n");
	WDG_SetCounter();
}


/*
void CursorPosition(int x,int y)
{
	int i;
	s8 buf[20];
	
	UartPut(DEBUG_PORT, 0x1b);	//esc
	sprintf((void*)buf, "[%d;%dH",x,y);
	for(i=0;i<strlen((void*)buf);i++)
	{
		UartPut(DEBUG_PORT, buf[i]);
	}
}
*/

int RS485_ID_Change(int argc, char *argv[])
{
	if(strcmp(argv[1],"id") == 0)
	{
		if(argv[2])
		{
			pG_Config->RS485_ID = (uint8_t)atoi(argv[2]);
			printf("RS485 ID = %d\r\n", (uint16_t)pG_Config->RS485_ID);
		}
	}
	else if(strcmp(argv[1], "eid") == 0)
	{
		if(argv[2])
		{
			pG_Config->RS485_Elec_ID = (uint8_t)atoi(argv[2]);
			printf("RS_ELEC_ID = %d\r\n", (uint16_t)pG_Config->RS485_Elec_ID);
		}
	}
	Store_CurrentConfig();
	// SET_Defaultdata();
	// SoftReset();

	return 1;
}

int light_control(int argc, char *argv[])
{
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;
	if(atoi(argv[1]) == 1)
	{
		EventCtrl(item2tsn(mapping_ITEM_LIGHT_1), INVERSE);
	}
	else if(atoi(argv[1]) == 2)
	{
		EventCtrl(item2tsn(mapping_ITEM_LIGHT_2), INVERSE);
	}
	else if(atoi(argv[1]) == 3)
	{
		EventCtrl(item2tsn(mapping_ITEM_LIGHT_3), INVERSE);
	}
	else if(atoi(argv[1]) == 4)
	{
		EventCtrl(item2tsn(mapping_ITEM_LIGHT_4), INVERSE);
	}
	return 1;	
}
int elec_control(int argc, char *argv[])
{
	Gu8_PowerSaving_Tmr			= POWER_SAVING_TMR;
	if(atoi(argv[1]) == 1)
	{
		EventCtrl(item2tsn(mapping_ITEM_ELECTRICITY_1), INVERSE);
	}
	else if(atoi(argv[1]) == 2)
	{
		EventCtrl(item2tsn(mapping_ITEM_ELECTRICITY_2), INVERSE);
	}
	return 1;
}

int Power_Reset(void)
{
	GPIO_ResetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);
	printf("STPM OFF\r\n");
	return 1;
}

int Power_Set(void)
{
	GPIO_SetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);
	printf("STPM ON\r\n");
	return 1;
}
//-------------------------------------------------------------------------------------------------------------------------
#if 1
void Debug_Puts(uint8_t *buf, int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		USART_SendData8(DEBUG_USART, buf[i]);
		while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TC) == RESET);
	}
}

void Debug_Put(uint8_t data)
{
	USART_SendData8(DEBUG_USART, data);
	while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TC) == RESET);
}
#endif

int IR_Debug(void)
{
    if(pG_Config->Enable_Flag.IR)   pG_Config->Enable_Flag.IR = 0;
    else                            pG_Config->Enable_Flag.IR = 1;
	Store_CurrentConfig();
    return 1;
}

#if 1
#define PUTCHAR_PROTOTYPE char putchar (char c)
#define GETCHAR_PROTOTYPE char getchar (void)

PUTCHAR_PROTOTYPE
{
	/* Write a character to the USART */
	if(c == '\n')
	{
		USART_SendData8(DEBUG_USART, '\r');
		while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TC) == RESET);
	}
	USART_SendData8(DEBUG_USART, c);
	/* Loop until the end of transmission */
	while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TC) == RESET);
	
	return (c);
}

GETCHAR_PROTOTYPE
{
	int c = 0;
	/* Loop until the Read data register flag is SET */
	while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_RXNE) == RESET);
	
	c = USART_ReceiveData8(DEBUG_USART);
	
	return (uint8_t)(c);
}
#endif

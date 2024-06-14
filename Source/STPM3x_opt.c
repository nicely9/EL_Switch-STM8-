/************************************************************************************
	Project		: 전자식스위치
	File Name	: STPM3x_opt.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/07/10
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "stm8l15x_usart.h"
#include "STPM3x_opt.h"
#include "Timer.h"
#include "Debug.h"
#include "BigInteger.h"
#include "WDGnBeep.h"
#include "el_switch.h"
#include "LCD.h"
#include "rs-485.h"
/*
유효 전력(active power) 또는 평균전력(average power)  [W]
     - 실제로 전동기 등의 동력을 돌리는 일(Work)을 행하는 전력
        . 에너지원을 필요로하는 전력 
        . 실제로 열 소비를 하는 전력
     - 시간평균하면, 0 이 아닌 일정 값을 갖는 전력
     - 단위 표시 : [W (Watt)]
       
무효 전력(reactive power)  [VAR, Volt Ampere Reactive] 
     - 실제로 어떠한 일도 행하지 않는 전력 
        . 에너지원을 필요로하지 않는 전력  
        . 에너지를 소비하지 않고, (열 또는 일 소비를 일으키지 않고)
           .. 전원과 부하 간에 축적 후 방출되며 교환 만 되는 전력 요소
     - 시간평균하면, 0 이 되는 전력
     단위표시 : [VAR (Volt Ampere Reactive)] 
     
피상 전력(apparent power) 또는 겉보기 전력  [W] 또는 [VA]
     - 교류회로에서 전압 및 전류의 실효값의 곱을 말함 (Vrms·Irms)
        . 전기설비(변압기,차단기등)의 부하나 전원의 용량을 표시할 때 사용
     - 단위표시 : [VA (볼트 암페어)] 
        . 단위가 와트[W]인 평균전력과 차원은 같으나, 혼동을 피하기위해 [VA]로 표시
*/

uint8_t	Gu8_ElecLimitCurrent_1_Tmr;
uint8_t	Gu8_ElecLimitCurrent_2_Tmr;

static uint8_t FRAME_for_UART_mode(u8 *pBuf);
static uint8_t CRC_u8Checksum;
uint8_t	Gu8_STPM3x_COMM_Error_Tmr = 255;

#define	ADuM5010_PDIS_DELAY_TMR		60		// 600ms	// jsyoon 20200925

#define CRC_8					(0x07)
#define STPM3x_FRAME_LEN		(5)

//#define	CUT_mA		(double)(0.010)		// 설계기준은 측정가능한 최소 전류인 10.02mA 미만은 0와트로 표현
#define	CUT_mA		(double)(0.006)			// 6.0mA 미만은 0와트로 표현
#define	LSBp		(double)(0.00508462)
#define	LSBpo		(double)(0.02033849)
//------------------------------------------------------------------------------------------------------------------
#define	Kint		1
#define	Vref		1.18
#define	R1			1800000.0
#define	R2			1200.0
#define	Av			2.0
#define	CALv		0.875
#define	CALi		0.875

#if SHUNT == SHUNT_0_001
#define	Ks			0.001
#define	Ai			16.0
#elif SHUNT == SHUNT_0_002
#define	Ks			0.002
#define	Ai			8.0
#elif SHUNT == SHUNT_0_003
#define	Ks			0.003
#define	Ai			8.0
#elif SHUNT == SHUNT_0_004
#define	Ks			0.004
#define	Ai			4.0
#else
#error	define error : SHUNT is SHUNT_0_001 ~ SHUNT_0_004
#endif

#define	POWER2_15	32768.0			// 2^15
#define	POWER2_17	131072.0		// 2^17
#define	POWER2_28	268435456.0		// 2^17
// Xv = (InputV * Av * CALv * POWER2_15) / (Vref * (1 + (R1 / R2)))
// Xi = (InputA * Ai * CALi * Ks * POWER2_17) / Vref
#define	Xv			((value * Av * CALv * POWER2_15) / (Vref * (1 + (R1 / R2))))
#define	Xi			((value * Ai * CALi * Ks * POWER2_17) / Vref)

#define	POWER_FACTOR	((Vref * Vref * (1 + R1 / R2))) / (Av * Ai * Ks * CALv * CALi)) * 100.0
#define	V_FACTOR		((Vref * (1 + (R1 / R2))) / (Av CALv)) * 100.0
#define	I_FACTOR		(Vref / (Ai CALi * Ks * Kint)) * 100.0
//------------------------------------------------------------------------------------------------------------------

static void LocalDelay(uint16_t n){ while (n--); }

#define DELAY(n)                LocalDelay(n)      // {TU16 d = (n); while (d--); }
#define DELAY_1()               DELAY(400)		// 250us

typedef enum 
{
	ADDR_STPM_DSPCTRL3 = 0,	// 1
	ADDR_STPM_DSPCTRL5,
	ADDR_STPM_DSPCTRL6,
	ADDR_STPM_DSPCTRL7,		// 2
	ADDR_STPM_DSPCTRL8,
	ADDR_STPM_DFECTRL1,
	ADDR_STPM_DFECTRL2,		// 3
	ADDR_STPM_DSP_REG14,
	ADDR_STPM_DSP_REG15,
	
#ifdef	STPM3x_OFFSET_SETTING
	ADDR_STPM_DSPCTRL9,		// 4
	ADDR_STPM_DSPCTRL10,
	ADDR_STPM_DSPCTRL11,
	ADDR_STPM_DSPCTRL12,
#endif

#ifdef STPM3x_POWER
	ADDR_STPM_CH1_REG5,		// 5
	ADDR_STPM_CH1_REG8,
	ADDR_STPM_CH2_REG5,
	ADDR_STPM_CH2_REG8,
#ifndef _STPM3x_MIN_READ_
	ADDR_STPM_CH1_REG6,		// 6
	ADDR_STPM_CH1_REG7,
	ADDR_STPM_CH1_REG9,
	ADDR_STPM_CH1_REG10,	// 7
	ADDR_STPM_CH1_REG11,
	ADDR_STPM_CH2_REG6,
	ADDR_STPM_CH2_REG7,		// 8
	ADDR_STPM_CH2_REG9,
	ADDR_STPM_CH2_REG10,
	ADDR_STPM_CH2_REG11,
#endif
#endif

#ifdef	STPM3x_ENERGY
	ADDR_STPM_CH1_REG1,		// 9
	ADDR_STPM_CH1_REG2,
	ADDR_STPM_CH1_REG3,
	ADDR_STPM_CH1_REG4,
	ADDR_STPM_CH2_REG1,		// 10
	ADDR_STPM_CH2_REG2,
	ADDR_STPM_CH2_REG3,
	ADDR_STPM_CH2_REG4,
#endif

#ifndef _STPM3x_MIN_READ_
	ADDR_STPM_DSPCTRL1,		// 11
	ADDR_STPM_DSPCTRL2,
	ADDR_STPM_DSPCTRL4,		// 12
	ADDR_STPM_DSPSR1,
	ADDR_STPM_DSPSR2,
#endif

#if 0
	ADDR_STPM_DSPIRQ1,
	ADDR_STPM_DSPIRQ2,
	ADDR_STPM_USREG1,
	ADDR_STPM_USREG2,
#endif
	
	MAX_BLOCK
}STPM_OPT_Address_Pos;


const uint8_t metro_stpm_Address[] =
{
	STPM_DSPCTRL3,
	STPM_DSPCTRL5,
	STPM_DSPCTRL6,
	STPM_DSPCTRL7,
	STPM_DSPCTRL8,
	STPM_DFECTRL1,
	STPM_DFECTRL2,
	STPM_DSP_REG14,
	STPM_DSP_REG15,

#ifdef	STPM3x_OFFSET_SETTING
	STPM_DSPCTRL9,
	STPM_DSPCTRL10,
	STPM_DSPCTRL11,
	STPM_DSPCTRL12,
#endif
	
#ifdef STPM3x_POWER
	STPM_CH1_REG5,
	STPM_CH1_REG8,
	STPM_CH2_REG5,
	STPM_CH2_REG8,
#ifndef _STPM3x_MIN_READ_
	STPM_CH1_REG6,
	STPM_CH1_REG7,
	STPM_CH1_REG9,
	STPM_CH1_REG10,
	STPM_CH1_REG11,
	STPM_CH2_REG6,
	STPM_CH2_REG7,
	STPM_CH2_REG9,
	STPM_CH2_REG10,
	STPM_CH2_REG11,
#endif
#endif

#ifdef	STPM3x_ENERGY
	STPM_CH1_REG1,
	STPM_CH1_REG2,
	STPM_CH1_REG3,
	STPM_CH1_REG4,
	STPM_CH2_REG1,
	STPM_CH2_REG2,
	STPM_CH2_REG3,
	STPM_CH2_REG4,
#endif

#ifndef _STPM3x_MIN_READ_
	STPM_DSPCTRL1,
	STPM_DSPCTRL2,
	STPM_DSPCTRL4,
	STPM_DSPSR1,
	STPM_DSPSR2,
#endif

#if 0
	STPM_DSPIRQ1,
	STPM_DSPIRQ2,
	STPM_USREG1,
	STPM_USREG2,
#endif
	0
};

// R1 = 1800K, R2 = 1.2K
const uint32_t metroVoltageFact[] =
{
	101210,			// ch 1
	101210			// ch 2
};

#if SHUNT == SHUNT_0_001
const uint32_t metroPowerFact[] =
{
	8530581,		// ch 1
	8530581			// ch 2
};
const uint32_t metroCurrentFact[] =
{
	8428,			// ch 1
	8428			// ch 2
};
#elif SHUNT == SHUNT_0_002
const uint32_t metroPowerFact[] =
{
	8530581,		// ch 1
	8530581			// ch 2
};
const uint32_t metroCurrentFact[] =
{
	8428,			// ch 1
	8428			// ch 2
};
#elif SHUNT == SHUNT_0_003
const uint32_t metroPowerFact[] =
{
	5687054,		// ch 1
	5687054			// ch 2
};
const uint32_t metroCurrentFact[] =
{
	5619,			// ch 1
	5619			// ch 2
};
#elif SHUNT == SHUNT_0_004
const uint32_t metroPowerFact[] =
{
	8530581,		// ch 1
	8530581			// ch 2
};
const uint32_t metroCurrentFact[] =
{
	8428,			// ch 1
	8428			// ch 2
};
#endif

metroData_t				metroData;
METRO_Device_Config_t	GS_Metro_Device_Config;

STPM3x_BUF	STPM3x_RxBuf;

uint8_t	Gu8_STPM3x_RW_Process_Flag = 0;
uint8_t	Gu8_STPM3x_Write_Address;
uint8_t	*Gu8_STPM3x_Read_Address;
uint8_t	Gu8_STPM3x_RW_Blocks = 0;
__IO uint8_t	Gu8_STPM3x_RW_Process_Tmr = 0;
uint8_t	Gu8_STPM3x_RW_Res =0 ;
uint32_t *G32ptr_STPM3x_RW_Data;

#define		_DOUBLE_AVERAGE_

//#define		MAX_WATT_AVERAGE		3
#define		MAX_WATT_AVERAGE		3
#ifdef	_DOUBLE_AVERAGE_
#define	AVR_DTYPE	double
#else
#define	AVR_DTYPE	uint16_t
#endif
static AVR_DTYPE	Average_Watt_1[MAX_WATT_AVERAGE];
static AVR_DTYPE	Average_Watt_2[MAX_WATT_AVERAGE];
static AVR_DTYPE	batch_Average_Watt_1 = 0;
static AVR_DTYPE	batch_Average_Watt_2 = 0;

uint8_t	Gu8_W_Tmr	= 0;

uint16_t	num_of_sample[MAX_AVG_FILTER];
AVR_DTYPE	prev_average[MAX_AVG_FILTER];

uint8_t		Gu16_Average_Watt_Cnt	= 0;
double	Gu16_Elec_1_Watt;
double	Gu16_Elec_2_Watt;
//-------------------------------------------------------------------------------------------------------------------------
void irq_STPM3x_TX(void)
{
	uint8_t data;
	
	if(USART_GetITStatus(STPM3x_USART, USART_IT_TC) != RESET)
	{
		if(Uart_GetTxQ(STPM3x_PORT, &data) != 0)
		{
			USART_SendData8(STPM3x_USART, data);
			USART_ClearITPendingBit(STPM3x_USART, USART_IT_TC);
		}
		else
		{
			USART_ITConfig(STPM3x_USART, USART_IT_TC, DISABLE);	// Disable the USART Transmit Complete interrupt
			//USART_ClearITPendingBit(STPM3x_USART, USART_IT_TC);	// ???
		}
	}
}

void irq_STPM3x_RX(void)
{
	uint8_t data;
	
	if(USART_GetITStatus(STPM3x_USART, USART_IT_RXNE) != RESET)
	{
		data = USART_ReceiveData8(STPM3x_USART);
		Uart_PutRxQ(STPM3x_PORT, data);							// Read one byte from the receive data register
	}
}
//-------------------------------------------------------------------------------------------------------------------------
void STM3x_Power_Reset(void)
{
	GPIO_ResetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	//210428 반대로
	delay( (ADuM5010_PDIS_DELAY_TMR * 10) );
	GPIO_SetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	//210428 반대로

	// GPIO_SetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Disable
	// delay( (ADuM5010_PDIS_DELAY_TMR * 10) );				// jsyoon 20200925
	// GPIO_ResetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Enable
	//delay(36);												// Wait 36ms
	delay(100);		// jsyoon 20200925
}
/*
void STM3x_Soft_Reset(void)
{
	uint32_t	tmp;
	
	tmp	= pG_Config->metroSetupDefault[2] | METRO_STPM_Reset_Bit;
	Gu8_STPM3x_RW_Process_Flag	= 0;
	STPM3x_Write(STPM_DSPCTRL3, 1, (void*)&tmp);
	STPM3x_While(100, (void*)"R");		// timeout 100ms
}
*/
//-------------------------------------------------------------------------------------------------------------------------
void STPM3x_GPIO_Init(void)
{
	if(pG_Config->Enable_Flag.STPM3x)
	{
		// GPIO_Init(STPM3x_SYN_PORT,		STPM3x_SYN_PIN,		GPIO_Mode_Out_PP_High_Fast);	// Output push-pull, high level, 10MHz
		// GPIO_Init(ADuM5010_PDIS_PORT,	ADuM5010_PDIS_PIN,	GPIO_Mode_Out_PP_Low_Fast);		// Output push-pull, low level, 10MHz
		GPIO_Init(ADuM5010_PDIS_PORT,   ADuM5010_PDIS_PIN,  GPIO_Mode_Out_PP_High_Fast);		//210111
		STM3x_Power_Reset();
	}
	else
	{
		//GPIO_Init(STPM3x_SYN_PORT,		STPM3x_SYN_PIN,		GPIO_Mode_In_FL_No_IT);			// Input floating, no external interrupt
		GPIO_Init(ADuM5010_PDIS_PORT,	ADuM5010_PDIS_PIN,	GPIO_Mode_In_FL_No_IT);			// Input floating, no external interrupt
		GPIO_Init(STPM3x_TX_PORT,		STPM3x_TX_PIN,		GPIO_Mode_In_FL_No_IT);			// Input floating, no external interrupt
		GPIO_Init(STPM3x_RX_PORT,		STPM3x_RX_PIN,		GPIO_Mode_In_FL_No_IT);			// Input floating, no external interrupt
	}
	printf("STPM3x GPIO Init\n");
}

void STPM3x_Init(void)
{
	uint8_t	i;
	
	for(i=0;i<MAX_AVG_FILTER;i++)
	{
		num_of_sample[i]	= 0;
		prev_average[i]		= (AVR_DTYPE)0;;
	}

	if(pG_Config->Enable_Flag.STPM3x)
	{
		for(i=0;i<MAX_WATT_AVERAGE;i++)
		{
			Average_Watt_1[i]	= (AVR_DTYPE)0;
			Average_Watt_2[i]	= (AVR_DTYPE)0;
		}
		
		Gu8_ElecLimitCurrent_1_Tmr	= ELEC_LIMIT_CURRENT_TIME;
		Gu8_ElecLimitCurrent_2_Tmr	= ELEC_LIMIT_CURRENT_TIME;
		
		memset((void*)&STPM3x_RxBuf, 0, sizeof(STPM3x_BUF));
		
		CLK_PeripheralClockConfig((CLK_Peripheral_TypeDef)STPM3x_PORT_CLK, ENABLE);		// Enable STPM3x USART clock
		
		SYSCFG_REMAPPinConfig(REMAP_Pin_USART1TxRxPortA, ENABLE);		// USART1_TX mapped on PA2 and USART3_RX mapped on PA3
		
		GPIO_ExternalPullUpConfig(STPM3x_TX_PORT, STPM3x_TX_PIN, ENABLE);				// STPM3x Tx as alternate function push-pull
		GPIO_ExternalPullUpConfig(STPM3x_RX_PORT, STPM3x_RX_PIN, ENABLE);				// STPM3x Rx as alternate function push-pull
		USART_Init(STPM3x_USART, 9600, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
		//-------------------------------------------------------------------------------------------------------------------------
		USART_ITConfig(STPM3x_USART, USART_IT_RXNE, ENABLE);
		//USART_ITConfig(STPM3x_USART, USART_IT_TC, ENABLE);		// Enable the USART Transmit complete interrupt
		USART_Cmd(STPM3x_USART, ENABLE);
		//-------------------------------------------------------------------------------------------------------------------------
		GS_Metro_Device_Config.factor_voltage_int_ch1	= metroVoltageFact[0];
		GS_Metro_Device_Config.factor_current_int_ch1	= metroCurrentFact[0];
		GS_Metro_Device_Config.factor_voltage_int_ch2	= metroVoltageFact[1];
		GS_Metro_Device_Config.factor_current_int_ch2	= metroCurrentFact[1];
		
		GS_Metro_Device_Config.factor_power_int_ch1		= metroPowerFact[0];
		GS_Metro_Device_Config.factor_energy_int_ch1	= metroPowerFact[0] / FACTOR_POWER_ON_ENERGY;
		GS_Metro_Device_Config.factor_power_int_ch2		= metroPowerFact[1];
		GS_Metro_Device_Config.factor_energy_int_ch2	= metroPowerFact[1] / FACTOR_POWER_ON_ENERGY;
		
		//G_Debug = DEBUG_STPM3x_REALDATA;
		
		Gu8_STPM3x_RW_Process_Flag	= 0;
		STPM3x_Write(STPM_DSPCTRL1, 20, pG_Config->metroSetupDefault);
		STPM3x_While(1000, (void*)"I1");		// timeout 1s
		
		Gu8_STPM3x_RW_Process_Flag	= 0;
#ifndef	_STPM3x_MIN_READ_
		STPM3x_Read((uint8_t *)&metro_stpm_Address[ADDR_STPM_DSPCTRL1], MAX_BLOCK, (uint32_t *)&GS_Metro_Device_Config.metro_stpm_reg);	// 확인
#else
		STPM3x_Read((uint8_t *)&metro_stpm_Address[ADDR_STPM_DSPCTRL3], MAX_BLOCK, (uint32_t *)&GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3);	// 확인
#endif
		STPM3x_While(1000, (void*)"I2");		// timeout 1s
		
		//printf("DSPCTRL3 %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3);
		printf("STPM3x Init %d\n", (uint16_t)MAX_BLOCK);
	}
}

//-------------------------------------------------------------------------------------------------------------------------
void STPM3x_UART_Process(void)
{
	uint16_t	i;
	uint8_t		data;
	u32_data	p_readdata;
	
	while(Uart_GetRxQ(STPM3x_PORT, &data) != 0)
	{
		if(G_Debug == DEBUG_STPM3x_REALDATA)
		{
			printf("%02x ", (uint16_t)data);
		}
		
		if(Gu8_STPM3x_RW_Process_Flag)
		{
			STPM3x_RxBuf.buf[STPM3x_RxBuf.count++]	= data;
			
			//printf("STPM3x_RxBuf.count %d\n", STPM3x_RxBuf.count);
			
			if(STPM3x_RxBuf.count == 5)
			{
				Gu8_STPM3x_COMM_Error_Tmr	= 100;		// 1s
				
				if(FRAME_for_UART_mode(STPM3x_RxBuf.buf) == STPM3x_RxBuf.buf[4])	// CRC
				{
					if(Gu8_STPM3x_RW_Process_Flag == STPM_READ_REQ_MODE)			// READ 모드
					{
						Gu8_STPM3x_RW_Res			+= 1;
						if(Gu8_STPM3x_RW_Res == 2)
						{
							Gu8_STPM3x_RW_Process_Tmr	= 0;
							
							p_readdata.byte.data[3]	= STPM3x_RxBuf.buf[0];
							p_readdata.byte.data[2]	= STPM3x_RxBuf.buf[1];
							p_readdata.byte.data[1]	= STPM3x_RxBuf.buf[2];
							p_readdata.byte.data[0]	= STPM3x_RxBuf.buf[3];
							
							if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
							{
								printf("STPM RD RX(ROW %d, addr 0x%02x) : ",  (uint16_t)((*Gu8_STPM3x_Read_Address) / 2), (uint16_t)*Gu8_STPM3x_Read_Address);
							}
							if(G_Debug == DEBUG_STPM3x_REALDATA)
							{
								for(i=0;i<5;i++)
								{
									printf("%02x, ", (uint16_t)STPM3x_RxBuf.buf[i]);
								}
								printf("\n");
							}
							
							if(G32ptr_STPM3x_RW_Data)
							{
								*G32ptr_STPM3x_RW_Data	= p_readdata.u32data;			// 데이터 저장
								 G32ptr_STPM3x_RW_Data++;
								 
								if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
								{
									printf("STPM3x RX Data %lx\n", p_readdata.u32data);
								}
							}
							
							if(Gu8_STPM3x_RW_Blocks)	Gu8_STPM3x_RW_Blocks--;
						}
						
					}
					else if(Gu8_STPM3x_RW_Process_Flag == STPM_WRITE_ING_MODE) 		// WRITE 모드
					{
						//if(G_Debug == DEBUG_STPM3x)
						if(G_Debug == DEBUG_STPM3x_REALDATA)
						{
							printf("STPM WR RX : ");
							for(i=0;i<5;i++)
							{
								printf("%02x, ", (uint16_t)STPM3x_RxBuf.buf[i]);
							}
							printf("\n");
						}
						
						Gu8_STPM3x_RW_Res			+= 1;
						if(Gu8_STPM3x_RW_Res == 2)
						{
							Gu8_STPM3x_RW_Process_Tmr	= 0;
						}
						
						if(Gu8_STPM3x_RW_Blocks)	Gu8_STPM3x_RW_Blocks--;
					}
				}
				else
				{
					if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
					{
						printf("\nERR : STPM3x CRC 0x%02x != 0x%02x\n\n", (uint16_t)FRAME_for_UART_mode(STPM3x_RxBuf.buf), (uint16_t)STPM3x_RxBuf.buf[4]);
					}
					Gu8_STPM3x_RW_Res			= 0;
					Gu8_STPM3x_RW_Process_Tmr	= 10;	// 100ms
					//Gu8_STPM3x_RW_Process_Tmr	= 1;	// 10ms
				}
				STPM3x_RxBuf.count	= 0;
				break;
			}
		}
	}
}
//------------------------------------------------------------------------------------------
//uint8_t		Gu8_STPM3x_Average_Tmr	= 0;
uint8_t		Gu8_STPM3x_Read_Tmr		= 0;
//uint8_t		Gu8_RW	= 0;

enum
{
	STPM3x_MODE_0	= 0,
	STPM3x_MODE_1,
	STPM3x_MODE_2,
	STPM3x_MODE_3,
	STPM3x_MODE_4,
#ifdef	STPM3x_ENERGY
	STPM3x_MODE_5,
	STPM3x_MODE_6,
	STPM3x_MODE_7,
#endif
#ifdef	STPM3x_POWER
	STPM3x_MODE_8,
	STPM3x_MODE_9,
	STPM3x_MODE_10,
	STPM3x_MODE_11,
#endif
	STPM3x_MODE_12,
	STPM3x_MODE_13
}_stpm3x_mode_;

void STPM3x_UartData_Init(void)
{
	Gu8_STPM3x_RW_Process_Tmr		= 0;
	Gu8_STPM3x_RW_Res				= 0;
	Gu8_STPM3x_RW_Blocks			= 0;
	Gu8_STPM3x_RW_Process_Flag		= 0;
	G32ptr_STPM3x_RW_Data			= 0;
	Gu8_STPM3x_Write_Address		= 0;
	Gu8_STPM3x_Read_Address			= 0;
	
	STPM3x_RxBuf.count				= 0;
}

#define	MAX_AVR_FILTER_SAMPLE		40
uint8_t	AvgFilter_MAX_Sample	= MAX_AVR_FILTER_SAMPLE;

double AvgFilter(uint8_t ch, double x)
{
    double average, alpha;
	
	//if(num_of_sample[ch] >= 20)	num_of_sample[ch] = 15;
	//if(num_of_sample[ch] >= 40)	num_of_sample[ch] = 35;
	//if(num_of_sample[ch] >= AvgFilter_MAX_Sample)	num_of_sample[ch] = AvgFilter_MAX_Sample-5;
	if(num_of_sample[ch] >= AvgFilter_MAX_Sample)	num_of_sample[ch] = AvgFilter_MAX_Sample;
	
    // 샘플 수 +1 (+1 the number of sample)
    num_of_sample[ch] += 1;
	
    // 평균 필터의 alpha 값 (alpha of average filter)
    alpha = (num_of_sample[ch] - 1) / (num_of_sample[ch] + 0.0);

    // 평균 필터의 재귀식 (recursive expression of average filter)
    average = alpha * prev_average[ch] + (1 - alpha) * x;

    // 평균 필터의 이전 상태값 업데이트 (update previous state value of average filter)
    prev_average[ch] = average;

    return average;
}

/*
W		diff
3335	
3220	
2990	50
2760	
2530	45
2300	40
2070	
1840	35
1610	30
1380	
1150	25
920		20
690		15
460		10
230		5
115	
*/
/*
uint8_t W_ChangeDetection(uint8_t ch, AVR_DTYPE w)
{
	static AVR_DTYPE old_powerActive[2]	= {(AVR_DTYPE)0, (AVR_DTYPE)0};
	AVR_DTYPE	diff, ratio, rate_change;
	uint8_t	ret	= 0;
	
	if(w >= (AVR_DTYPE)2990)			// 50	13A ~
	{
		ratio		= (AVR_DTYPE)0;
		rate_change = (AVR_DTYPE)50;
	}
	else if(w >= (AVR_DTYPE)2530)		// 45	11A ~ 13A
	{
		ratio		= (AVR_DTYPE)5/((AVR_DTYPE)2990-(AVR_DTYPE)2530);
		rate_change = (ratio * (w - (AVR_DTYPE)2530)) + (AVR_DTYPE)45;
	}
	else if(w >= (AVR_DTYPE)2300)		// 40	10A ~ 11A	6 v
	{
		ratio		= (AVR_DTYPE)5/((AVR_DTYPE)2530-(AVR_DTYPE)2300);
		rate_change = (ratio * (w - (AVR_DTYPE)2300)) + (AVR_DTYPE)40;
	}
	else if(w >= (AVR_DTYPE)1840)		// 35	8A ~ 10A	5
	{
		ratio		= (AVR_DTYPE)5/((AVR_DTYPE)2300-(AVR_DTYPE)1840);
		rate_change = (ratio * (w - (AVR_DTYPE)1840)) + (AVR_DTYPE)35;
	}
	else if(w >= (AVR_DTYPE)1610)		// 30	7A ~ 8A		3
	{
		ratio		= (AVR_DTYPE)5/((AVR_DTYPE)1840-(AVR_DTYPE)1610);
		rate_change = (ratio * (w - (AVR_DTYPE)1610)) + (AVR_DTYPE)30;
	}
	else if(w >= (AVR_DTYPE)1150)		// 25	5A ~ 7A		2
	{
		ratio		= (AVR_DTYPE)5/((AVR_DTYPE)1610-(AVR_DTYPE)1150);
		rate_change = (ratio * (w - (AVR_DTYPE)1150)) + (AVR_DTYPE)25;
	}
	else if(w >= (AVR_DTYPE)920)		// 20	4A ~ 5A
	{
		ratio		= (AVR_DTYPE)5/((AVR_DTYPE)1150-(AVR_DTYPE)920);
		rate_change = (ratio * (w - (AVR_DTYPE)920)) + (AVR_DTYPE)20;
	}
	else if(w >= (AVR_DTYPE)690)		// 15	3A ~ 4A
	{
		ratio		= (AVR_DTYPE)5/((AVR_DTYPE)920-(AVR_DTYPE)690);
		rate_change = (ratio * (w - (AVR_DTYPE)690)) + (AVR_DTYPE)15;
	}
	else if(w >= (AVR_DTYPE)460)		// 10	2A ~ 3A
	{
		ratio		= (AVR_DTYPE)5/((AVR_DTYPE)690-(AVR_DTYPE)460);
		rate_change = (ratio * (w - (AVR_DTYPE)460)) + (AVR_DTYPE)10;
	}
	else if(w >= (AVR_DTYPE)230)		// 5	1A ~ 2A
	{
		ratio		= (AVR_DTYPE)5/((AVR_DTYPE)460-(AVR_DTYPE)230);
		rate_change = (ratio * (w - (AVR_DTYPE)230)) + (AVR_DTYPE)5;
	}
	else 								// 5	0  ~ 1A
	{
		ratio		= (AVR_DTYPE)0;
		rate_change = (AVR_DTYPE)5;
	}
	
	diff	= w - old_powerActive[ch];
	if(diff > rate_change || diff < -rate_change)
	{
		//printf("%f : diff %f, ratio %f, rate %f\n", w, diff, ratio, rate_change);
		ret	= 1;
	}
	
	old_powerActive[ch]	= w;
	
	return ret;
}
*/

uint8_t A_ChangeDetection(uint8_t ch, double A)
{
	static double old_A[2]	= {(double)0, (double)0};
	double	diff, rate_change;
	uint8_t	ret	= 0;
	
	if(A <= 0.5)				// 0.5A 미만		110W미만
	{
		rate_change = 0.002;	// 2mA
	}
	else if(A <= 1.0)			// 0.5 ~ 1A		110W ~ 220W
	{
		rate_change = 0.002;	// 2mA
	}
	else if(A <= 2.0)			// 1A ~ 2A		220W ~ 440W
	{
		rate_change = 0.030;	// 30ma
	}
	else
	{
		rate_change = 0.050;	// 50mA
	}
	
	diff	= A - old_A[ch];
	if(diff > rate_change || diff < (rate_change * -1.0))
	{
		//printf("%f : diff %f, ratio %f, rate %f\n", A, diff, ratio, rate_change);
		if(G_Debug == DEBUG_STPM3x_DATA)
		{
			printf("\n\nA%d clear valule %f-------------------\n\n", ch + 1, diff);
		}
		ret	= 1;
	}
	
	old_A[ch]	= A;
	
	return ret;
}

void STPM3x_Process(void)
{
	static	uint8_t	mode = 0;
	static	uint8_t	diff	= 0;
	//static	uint8_t	tmp_flag_1	= 0, tmp_flag_2	= 0;
	static	uint8_t	stpm_read_addr	= ADDR_STPM_DSPCTRL3;
	static 	uint32_t	*read_data_addr	= (uint32_t*)&GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3;
	
	uint8_t	i;
	AVR_DTYPE	tmp1 = 0, tmp2 = 0, tmp;
	double	v, c1, c2, w1, w2;
	static	double	old_c1 = 0.0, old_c2 = 0.0;
	
	if(pG_Config->Enable_Flag.STPM3x)
	{
		STPM3x_UART_Process();		// jsyoon 20200925	통신에러 위로 위치 바꿈
		STPM3x_RW_Process();		// jsyoon 20200925	통신에러 위로 위치 바꿈
		
		if(Gu8_STPM3x_COMM_Error_Tmr == 0)
		{
			Gu8_STPM3x_COMM_Error_Tmr	= 255;
			
			printf("\n\nERR : STPM3x COMM\n\n");
			GPIO_ResetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Disable
			// GPIO_SetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Disable
			STPM3x_UartData_Init();
			Gu8_STPM3x_Read_Tmr	= ADuM5010_PDIS_DELAY_TMR;		// jsyoon 20200925
			mode	= STPM3x_MODE_0;
			diff	= 1;
			return;		// jsyoon 20200925
		}
		
		switch(mode)
		{
			case STPM3x_MODE_0:
				if(Gu8_STPM3x_RW_Process_Flag == 0 && Gu8_STPM3x_Read_Tmr == 0)
				{
					if(diff == 1)
					{
						//STM3x_Power_Reset();
						//diff	= 0;
						// GPIO_ResetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Enable
						GPIO_SetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Disable
						Gu8_STPM3x_Read_Tmr			= 10;		// 100ms
						Gu8_STPM3x_COMM_Error_Tmr	= 255;		// jsyoon 20200925
						mode++;
						break;
					}
#ifdef	_STPM3x_AUTO_LATCH_
					else
					{
						METRO_Set_Latch_device_type(LATCH_SW);
						mode++;
						break;
					}
#else
					else
					{
						mode++;
					}
#endif
					
				}
				//break;
			case STPM3x_MODE_1:
				if(Gu8_STPM3x_RW_Process_Flag == 0 && Gu8_STPM3x_Read_Tmr == 0)
				{
					if(diff == 1)
					{
						diff	= 0;
						STPM3x_UartData_Init();
						STPM3x_Write(STPM_DSPCTRL1, 20, pG_Config->metroSetupDefault);
						Gu8_STPM3x_Read_Tmr		= 10;		// 100ms
						break;
					}
					//STPM3x_Read((uint8_t *)&metro_stpm_Address[ADDR_STPM_DSPCTRL3], 3, (uint32_t*)&GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3);
					STPM3x_Read((uint8_t *)&metro_stpm_Address[stpm_read_addr++], 1, read_data_addr++);
					if(stpm_read_addr == MAX_BLOCK)
					{
						stpm_read_addr	= ADDR_STPM_DSPCTRL3;
						read_data_addr	= (uint32_t*)&GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3;
						mode++;
					}
				}
				break;
				
			case STPM3x_MODE_2:
				if(Gu8_STPM3x_RW_Process_Flag == 0)
				{
					diff	= 0;
#ifndef _STPM3x_MIN_READ_
					if(pG_Config->metroSetupDefault[0]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL1)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL1   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL1);		}
					if(pG_Config->metroSetupDefault[1]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL2)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL2   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL2);		}
#endif
					if(pG_Config->metroSetupDefault[2]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL3   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3);		}
#ifndef _STPM3x_MIN_READ_
					if(pG_Config->metroSetupDefault[3]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL4)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL4   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL4);		}
#endif
					if(pG_Config->metroSetupDefault[4]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL5)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL5   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL5);		}
					if(pG_Config->metroSetupDefault[5]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL6)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL6   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL6);		}
					if(pG_Config->metroSetupDefault[6]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL7)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL7   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL7);		}
					if(pG_Config->metroSetupDefault[7]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL8)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL8   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL8);		}                   
#ifdef	STPM3x_OFFSET_SETTING
					if(pG_Config->metroSetupDefault[8]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL9)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL9   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL9);		}
					if(pG_Config->metroSetupDefault[9]  != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL10)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL10  %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL10);	}
					if(pG_Config->metroSetupDefault[10] != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL11)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL11  %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL11);	}
					if(pG_Config->metroSetupDefault[11] != GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL12)		{	diff	= 1;	if(G_Trace)	printf("DSPCTRL12  %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL12);	}
#endif
					if(pG_Config->metroSetupDefault[12] != GS_Metro_Device_Config.metro_stpm_reg.DFECTRL1)		{	diff	= 1;	if(G_Trace)	printf("DFECTRL1   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DFECTRL1);		}
					if(pG_Config->metroSetupDefault[13] != GS_Metro_Device_Config.metro_stpm_reg.DFECTRL2)		{	diff	= 1;	if(G_Trace)	printf("DFECTRL2   %lx\n", GS_Metro_Device_Config.metro_stpm_reg.DFECTRL2);		}
#if 0
					if(pG_Config->metroSetupDefault[18] != GS_Metro_Device_Config.metro_stpm_reg.UARTSPICR1)	{	diff	= 1;	if(G_Trace)	printf("UARTSPICR1 %lx\n", GS_Metro_Device_Config.metro_stpm_reg.UARTSPICR1);	}
					if(pG_Config->metroSetupDefault[19] != GS_Metro_Device_Config.metro_stpm_reg.UARTSPICR2)	{	diff	= 1;	if(G_Trace)	printf("UARTSPICR2 %lx\n", GS_Metro_Device_Config.metro_stpm_reg.UARTSPICR2);	}
#endif
					if(diff == 1)
					{
						//STM3x_Power_Reset();
						// GPIO_SetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Disable
						GPIO_ResetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Disable
						Gu8_STPM3x_Read_Tmr	= ADuM5010_PDIS_DELAY_TMR;		// jsyoon 20200925
						if(G_Debug == DEBUG_STPM3x_DATA)	printf("\n\nSTPM Config diff(rewrite)\n\n");
						mode	= STPM3x_MODE_0;
						//Beep(OFF);	// TEST only
					}
					else
					{
						mode++;
					}
				}
				break;
			case STPM3x_MODE_3:
				//mode	= STPM3x_MODE_8;				break;
				Metro_Read_RMS(CHANNEL_1, &metroData.rmsvoltage1, &metroData.rmscurrent1, 1);	mode++;		break;		// 약18ms
				break;
			case STPM3x_MODE_4:
				Metro_Read_RMS(CHANNEL_2, &metroData.rmsvoltage2, &metroData.rmscurrent2, 1);	mode++;		break;		// 약18ms
#ifdef	STPM3x_ENERGY
			case STPM3x_MODE_5:
				//if(G_Trace)	printf("%ld\t", Metro_Read_energy(CHANNEL_1, E_W_ACTIVE));
				if(G_Trace)	printf("%ld\t", Metro_Read_energy(CHANNEL_2, E_W_ACTIVE));
				mode++;
				break;
			case STPM3x_MODE_6:
				//if(G_Trace)		printf("%ld\t", Metro_Read_energy(CHANNEL_1, E_REACTIVE));
				if(G_Trace)		printf("%ld\t", Metro_Read_energy(CHANNEL_2, E_REACTIVE));
				mode++;
				break;
			case STPM3x_MODE_7:
				//if(G_Trace)		printf("%ld\n", Metro_Read_energy(CHANNEL_1, E_APPARENT));
				if(G_Trace)		printf("%ld\n", Metro_Read_energy(CHANNEL_2, E_APPARENT));
				mode++;
				break;
#endif
#ifdef	STPM3x_POWER
			
			case STPM3x_MODE_8:
				metroData.powerActive1		= Metro_Read_Power(CHANNEL_1, W_ACTIVE);			mode++;		break;		// 약8ms
			case STPM3x_MODE_9:
				metroData.powerApparent1	= Metro_Read_Power(CHANNEL_1, APPARENT_RMS);		mode++;		break;		// 약8ms
			case STPM3x_MODE_10:
				metroData.powerActive2		= Metro_Read_Power(CHANNEL_2, W_ACTIVE);			mode++;		break;		// 약8ms
			case STPM3x_MODE_11:
				metroData.powerApparent2	= Metro_Read_Power(CHANNEL_2, APPARENT_RMS);		mode++;		break;		// 약8ms
			
			/*
			case STPM3x_MODE_8:
				metroData.powerActive1		= Metro_Read_Power(CHANNEL_1, W_ACTIVE);			mode	= STPM3x_MODE_10;		break;		// 약8ms
			case STPM3x_MODE_10:
				metroData.powerActive2		= Metro_Read_Power(CHANNEL_2, W_ACTIVE);			mode	= STPM3x_MODE_12;		break;		// 약8ms
			*/
#endif
			case STPM3x_MODE_12:		// MAX 100ms 이내 갱신
				
				v	= (double)metroData.rmsvoltage1 / 1000.0;
				c1	= (double)metroData.rmscurrent1 / 1000.0;
				c2	= (double)metroData.rmscurrent2 / 1000.0;
				
				if(v > 300.0 || v == 0.0 || v < 100.0)
				{
					//if(++diff_cnt > 3)
					{
						//diff_cnt	= 0;
						printf("\n\n\ndata init\n\n\n");
						diff	= 1;
						// GPIO_SetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Disable
						GPIO_ResetBits(ADuM5010_PDIS_PORT, ADuM5010_PDIS_PIN);	// DC-DC Converter Disable
						Gu8_STPM3x_Read_Tmr	= ADuM5010_PDIS_DELAY_TMR;		// jsyoon 20200925
						mode	= STPM3x_MODE_0;
						break;
					}
				}
				
				w1	= v * c1;
				w2	= v * c2;
				
				tmp1	= (AVR_DTYPE)(metroData.powerActive1) / (AVR_DTYPE)1000;
				tmp2	= (AVR_DTYPE)(metroData.powerActive2) / (AVR_DTYPE)1000;
				
				/*
				if((old_c1 - c1) > STPM3x_DIFF_A || (old_c1 - c1) < -STPM3x_DIFF_A)		// 약 15mA
				{
					if(G_Debug == DEBUG_STPM3x_DATA)	printf("\n\nA1 clear valule %f -------------------\n\n", (old_c1 - c1));
					num_of_sample[AVG_FILTER_CH1]	= 0;
					prev_average[AVG_FILTER_CH1]	= (AVR_DTYPE)0;
				}
				*/
				if(A_ChangeDetection(AVG_FILTER_CH1, c1))
				{
					num_of_sample[AVG_FILTER_CH1]	= 0;
					prev_average[AVG_FILTER_CH1]	= (AVR_DTYPE)0;;
				}
				else
				{
					tmp	= tmp1 - (AVR_DTYPE)Gu16_LCD_Watt_1;
					if(tmp > (AVR_DTYPE)90.0 || tmp < (AVR_DTYPE)-90.0)
					{
						if(G_Debug == DEBUG_STPM3x_DATA)	printf("\n\nW1 clear valule %f -------------------\n\n", (old_c2 - c2));
						//num_of_sample[AVG_FILTER_CH1]	= MAX_AVR_FILTER_SAMPLE - 5;
						//prev_average[AVG_FILTER_CH1]	= tmp1;
						num_of_sample[AVG_FILTER_CH1]	= 0;
						prev_average[AVG_FILTER_CH1]	= (AVR_DTYPE)0;
					}
				}
				/*
				else if(W_ChangeDetection(AVG_FILTER_CH1, tmp1))
				{
					if(G_Debug == DEBUG_STPM3x_DATA)	printf("\n\nW1 clear valule %f-------------------\n\n", tmp1);
					num_of_sample[AVG_FILTER_CH1]	= 0;
					prev_average[AVG_FILTER_CH1]	= batch_Average_Watt_1;
					//Gu8_W_Tmr	= 50;	// 0.5s
				}
				*/
				
				/*
				if((old_c2 - c2) > STPM3x_DIFF_A || (old_c2 - c2) < -STPM3x_DIFF_A)		// 약 15mA
				{
					if(G_Debug == DEBUG_STPM3x_DATA)	printf("\n\nA2 clear valule %f -------------------\n\n", (old_c2 - c2));
					num_of_sample[AVG_FILTER_CH2]	= 0;
					prev_average[AVG_FILTER_CH2]	= (AVR_DTYPE)0;
				}
				*/
				if(A_ChangeDetection(AVG_FILTER_CH2, c2))
				{
					num_of_sample[AVG_FILTER_CH2]	= 0;
					prev_average[AVG_FILTER_CH2]	= (AVR_DTYPE)0;;
				}
				else
				{
					tmp	= tmp2 - (AVR_DTYPE)Gu16_LCD_Watt_2;
					if(tmp > (AVR_DTYPE)90.0 || tmp < (AVR_DTYPE)-90.0)
					{
						if(G_Debug == DEBUG_STPM3x_DATA)	printf("\n\nW2 clear valule %f -------------------\n\n", (old_c2 - c2));
						num_of_sample[AVG_FILTER_CH2]	= 0;
						prev_average[AVG_FILTER_CH2]	= (AVR_DTYPE)0;
					}
				}
				/*
				else if(W_ChangeDetection(AVG_FILTER_CH2, tmp2))
				{
					if(G_Debug == DEBUG_STPM3x_DATA)	printf("\n\nW2 clear valule %f-------------------\n\n", tmp2);
					num_of_sample[AVG_FILTER_CH2]	= 0;
					prev_average[AVG_FILTER_CH2]	= batch_Average_Watt_2;
					//Gu8_W_Tmr	= 50;	// 0.5s
				}
				*/
				
				tmp1	= AvgFilter(AVG_FILTER_CH1, tmp1);	// 		유효전력(실제 소비되는 전력)
				tmp2	= AvgFilter(AVG_FILTER_CH2, tmp2);
				Gu16_Elec_1_Watt = tmp1;
				Gu16_Elec_2_Watt = tmp2;
				if(G_Trace)
				{
					printf("tmp1 = %f tmp2 = %f\r\n", tmp1, tmp2);
				}
				if(Gu8_W_Tmr == 0)
				{
					if(tmp1 > (AVR_DTYPE)0)	Gu16_LCD_Watt_1	= (uint16_t)tmp1;
					else					Gu16_LCD_Watt_1	= (uint16_t)0;
					if(tmp2 > (AVR_DTYPE)0)	Gu16_LCD_Watt_2	= (uint16_t)tmp2;
					else					Gu16_LCD_Watt_2	= (uint16_t)0;
					
					Gu8_W_Tmr	= 100;	// 1s
				}
				
				if(G_Debug == DEBUG_STPM3x_DATA)
				{
					WDG_SetCounter();
					printf("\n\n");
					//printf("Cal %5.3f[V]\t %5.3f[A] %5.3f[W],\t %5.3f[A] %5.3f[W]\n", v, c1, w1, c2, w2);
					printf("Cal %5.3f[V]\t %5.3f[A] diff %5.3f[A],\t %5.3f[A] diff %5.3f[A]\n", v, c1, (old_c1 - c1), c2, (old_c2 - c2));
					
					//printf("Cal %5.3f[V]\t %5.3f[A],\t %5.3f[A]\n", v, c1, c2);
					WDG_SetCounter();
					printf("LCD        %d[W]\t\t %d[W]\n", Gu16_LCD_Watt_1, Gu16_LCD_Watt_2);
					WDG_SetCounter();
					printf("filter avr %5.3f[W]\t %5.3f[W]\n", tmp1, tmp2);
					//WDG_SetCounter();
					//printf("batch  avr %5.3f[W]\t %5.3f[W]\n", batch_Average_Watt_1, batch_Average_Watt_2);
					//printf("\t\t\t\t\t\t\tdiff %5.3f[W]\t %5.3f[W]\n", batch_Average_Watt_1-tmp1, batch_Average_Watt_2-tmp2);
					WDG_SetCounter();
#ifndef _STPM3x_MIN_READ_
					printf("1 Active %5.3f[W]\t ReActice %5.3f[W]\t Apparent %5.3f[W]\n", metroData.powerActive1/1000.0, metroData.powerReactive1/1000.0, metroData.powerApparent1/1000.0);
					WDG_SetCounter();
					printf("2 Active %5.3f[W]\t ReActice %5.3f[W]\t Apparent %5.3f[W]\n", metroData.powerActive2/1000.0, metroData.powerReactive2/1000.0, metroData.powerApparent2/1000.0);
					WDG_SetCounter();
#else
					/*
					printf("1 Active %5.3f[W]\t Apparent %5.3f[W]\n", metroData.powerActive1/1000.0, metroData.powerApparent1/1000.0);
					WDG_SetCounter();
					printf("2 Active %5.3f[W]\t Apparent %5.3f[W]\n", metroData.powerActive2/1000.0, metroData.powerApparent2/1000.0);
					*/
					{
						static double old_powerActive1	= 0.0;
						static double old_powerActive2	= 0.0;
						
						printf("1 Active %5.3f[W]\t diff %5.3f[W]\tApparent %5.3f[W]\n", metroData.powerActive1/1000.0, (metroData.powerActive1/1000.0) - old_powerActive1, metroData.powerApparent1/1000.0);
						WDG_SetCounter();
						printf("2 Active %5.3f[W]\t diff %5.3f[W]\tApparent %5.3f[W]\n", metroData.powerActive2/1000.0, (metroData.powerActive2/1000.0) - old_powerActive2, metroData.powerApparent2/1000.0);
						
						old_powerActive1	= metroData.powerActive1/1000.0;
						old_powerActive2	= metroData.powerActive2/1000.0;
					}
#endif
				}
				
				old_c1 = c1;
				old_c2 = c2;
				
				if((Gu16_LCD_Watt_1 <= Gu16_ElecLimitCurrent_1) && (GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1))) && pG_State->ETC.Auto1)
				{
					// printf("1, lcd : %d, limit %d\r\n", (uint16_t)Gu16_LCD_Watt_1, (uint16_t)Gu16_ElecLimitCurrent_1);
					if(Gu8_ElecLimitCurrent_1_Tmr == 0)
					{
						if(GET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_1) == 0)
						{
							EventCtrl(item2tsn(mapping_ITEM_ELECTRICITY_1), OFF);
#ifdef _HDC_PROTOCOL_
							Gu8_Elec_Auto_OFF[0] = 1;
#endif
						}
					}
				}
				else
				{
					Gu8_ElecLimitCurrent_1_Tmr	= ELEC_LIMIT_CURRENT_TIME;			// 120sec
				}
				
				if((Gu16_LCD_Watt_2 <= Gu16_ElecLimitCurrent_2) && (GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2))) && pG_State->ETC.Auto2)
				{
					// printf("2, lcd : %d, limit %d\r\n", (uint16_t)Gu16_LCD_Watt_2, (uint16_t)Gu16_ElecLimitCurrent_2);
					if(Gu8_ElecLimitCurrent_2_Tmr == 0)
					{
						
						if(GET_SWITCH_Delay_OFF_Flag(mapping_ITEM_ELECTRICITY_2) == 0)
						{
							EventCtrl(item2tsn(mapping_ITEM_ELECTRICITY_2), OFF);
#ifdef _HDC_PROTOCOL_
							Gu8_Elec_Auto_OFF[1] = 1;
#endif
						}
					}
				}
				else
				{
					Gu8_ElecLimitCurrent_2_Tmr	= ELEC_LIMIT_CURRENT_TIME;			// 120sec
				}
				
				if(Gu16_LCD_Watt_1 >= 3300)		// 3300W 보다 크면 즉시차단,		20200709	사업팀 요청으로 기능 수정함(추이사, 심차장), 220V 기준 15A
				{
					if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_1)))		// 켜져 있을 때
					{
						EventCtrl(item2tsn(mapping_ITEM_ELECTRICITY_1), OFF);
						SET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_ELECTRICITY_1, 0);
						Beep(BEEP_AC_WARRING);
#ifdef _HDC_PROTOCOL_
							Gu8_Elec_Overload_OFF[0] = 1;
#endif
					}
				}
				if(Gu16_LCD_Watt_2 >= 3300)		// 3300W 보다 크면 즉시차단,		20200709	사업팀 요청으로 기능 수정함(추이사, 심차장), 220V 기준 15A
				{
					if(GET_Switch_State(item2tsn(mapping_ITEM_ELECTRICITY_2)))		// 켜져 있을 때
					{
						EventCtrl(item2tsn(mapping_ITEM_ELECTRICITY_2), OFF);
						SET_SWITCH_Delay_OFF_Tmr(mapping_ITEM_ELECTRICITY_2, 0);
						Beep(BEEP_AC_WARRING);
#ifdef _HDC_PROTOCOL_
							Gu8_Elec_Overload_OFF[1] = 1;
#endif
					}
				} //20201204 안전인증때문에 차단함.
#ifdef	_STPM3x_AUTO_LATCH_
				//Gu8_STPM3x_Read_Tmr	= 10;	// 100ms
				//Gu8_STPM3x_Read_Tmr	= 10;	// 100ms
#endif
				mode	= STPM3x_MODE_0;
			
				break;
/*
			case STPM3x_MODE_13:
				if(Gu8_RW == 1)			// WRITE
				{
					if(Gu8_STPM3x_RW_Process_Flag == 0)
					{
						G_Debug = DEBUG_STPM3x_REALDATA;
						STPM3x_Write(STPM_DSPCTRL1, 20, pG_Config->metroSetupDefault);
						STPM3x_While(1000, (void*)"WR3");		// timeout 1s
						G_Debug = 0;
						Gu8_RW	= 0;
						mode	= 0;
					}
				}
				else if(Gu8_RW == 2)	// READ
				{
					if(Gu8_STPM3x_RW_Process_Flag == 0)
					{
						G_Debug = DEBUG_STPM3x_REALDATA;
						STPM3x_Read((uint8_t *)&metro_stpm_Address[ADDR_STPM_DSPCTRL1], 2, (uint32_t *)&GS_Metro_Device_Config.metro_stpm_reg);
						STPM3x_While(1000, (void*)"RD3");		// timeout 1s
						G_Debug = 0;
						Gu8_RW	= 0;
						mode	= 0;
					}
				}
				else	mode = 0;
				break;
*/
		}
	}
}

/*
	This function set latch inside Metrology devices
	Latch the device registers according to the latch type selection driving SYN pin
	or writing S/W Latchx bits in the DSP_CR3 register
	or setting auto-latch by S/W Auto Latch bit in the DSP_CR3 register
*/
void METRO_Set_Latch_device_type(METRO_Latch_Device_Type_t in_Metro_Latch_Device_Type)
{
	switch (in_Metro_Latch_Device_Type)
	{
		case LATCH_AUTO:
			// reset latch 1 and 2 bits in the internal DSP Control Register 3
			GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3 &= ~ BIT_MASK_STPM_LATCH1;
			GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3 &= ~ BIT_MASK_STPM_LATCH2;
			
			// Set  latch auto in the internal DSP Control Register 3
			GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3 |= BIT_MASK_STPM_AUTO_LATCH;
			
			// Now send data to the external chip
			// the address should be provided in 2 bytes format (16 bits by 16 bits) for STPM
			// Write register inside external chip
			STPM3x_Write(STPM_DSPCTRL3, 1 ,&GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3);
			break;
			
		case LATCH_SW:
			// Set  latch SW 1 et 2 for he Two channels  the internal DSP Control Register 3
			GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3 |= BIT_MASK_STPM_LATCH1|BIT_MASK_STPM_LATCH2;
			STPM3x_Write(STPM_DSPCTRL3, 1, &GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3);
			break;
			
		case LATCH_SYN_SCS:
			// Latch external chip with syn PIN : 1 pulses is needed to latch
			// Metro_HAL_STPM_SYN_single_pulse();
			break;
	}
}

uint32_t Metro_HAL_read_RMS_Voltage(METRO_internal_Channel_t in_Metro_int_Channel)
{
	uint32_t RMS_voltage = 0;
	
	if (in_Metro_int_Channel == INT_CHANNEL_1)
	{
		// get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg
		RMS_voltage = (GS_Metro_Device_Config.metro_stpm_reg.DSP_REG14) & BIT_MASK_STPM_DATA_VRMS;
	}
	else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
	{
		// get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg
		RMS_voltage = (GS_Metro_Device_Config.metro_stpm_reg.DSP_REG15) & BIT_MASK_STPM_DATA_VRMS;
	}
	return (RMS_voltage);
}

uint32_t Metro_HAL_read_RMS_Current(METRO_internal_Channel_t in_Metro_int_Channel)
{
	uint32_t RMS_current = 0;
	
	if (in_Metro_int_Channel == INT_CHANNEL_1)
	{
		// get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg
		RMS_current = ((GS_Metro_Device_Config.metro_stpm_reg.DSP_REG14)&BIT_MASK_STPM_DATA_C1_RMS)>>BIT_MASK_STPM_DATA_C_RMS_SHIFT;
	}
	else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
	{
		// get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg
		RMS_current = ((GS_Metro_Device_Config.metro_stpm_reg.DSP_REG15)&BIT_MASK_STPM_DATA_C2_RMS)>>BIT_MASK_STPM_DATA_C_RMS_SHIFT;
	}
	return (RMS_current);
}

void Metro_Read_RMS(METRO_Channel_t in_Metro_Channel, uint32_t * out_Metro_RMS_voltage, uint32_t * out_Metro_RMS_current, uint8_t in_RAW_vs_RMS)
{
	struct bn num1, num2, num3;
	struct bn result;
	
	METRO_internal_Channel_t int_Channel;
	
	uint32_t raw_RMS_Voltage = 0;
	uint32_t raw_RMS_Current = 0;
	uint32_t Factor_Voltage = 0;
	uint32_t Factor_Current = 0;
	
	uint32_t calc_RMS_Voltage = 0;
	uint32_t calc_RMS_Current = 0;
	
	int_Channel =(METRO_internal_Channel_t) in_Metro_Channel;	// Get if the channel requested is the one or the two of the device
	raw_RMS_Voltage = Metro_HAL_read_RMS_Voltage(int_Channel);	// Get raw RMS voltage according to device and channel
	raw_RMS_Current = Metro_HAL_read_RMS_Current(int_Channel);	// Get RAW RMS current according to device and channel
	
	if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
	{
		if(int_Channel == CHANNEL_1)
		{
			printf("CH1 rawV  %ld\n", raw_RMS_Voltage);
			printf("CH2 rawC1 %ld\n", raw_RMS_Current);
		}
		else
		{
			printf("CH2 rawC2 %ld\n", raw_RMS_Current);
		}
	}
	if (in_RAW_vs_RMS == 0)								// Return RAW values from registers to ouput
	{
		//*out_Metro_RMS_voltage = (uint32_t)((double)raw_RMS_Voltage * 0.30886928);	// LSB vrms = 0.30886928
		//*out_Metro_RMS_current = (uint32_t)((double)raw_RMS_Current * 0.000643049);	// LSB vrms = 0.000643049
		*out_Metro_RMS_voltage = (uint32_t)raw_RMS_Voltage;
		*out_Metro_RMS_current = (uint32_t)raw_RMS_Current;
	}
	else if (in_RAW_vs_RMS == 1)						// Return values to ouput params in mV and mA
	{
		if (int_Channel == INT_CHANNEL_1)							// gat Voltage and current factors to calculate the RMS values
		{
			Factor_Voltage = GS_Metro_Device_Config.factor_voltage_int_ch1;
			Factor_Current = GS_Metro_Device_Config.factor_current_int_ch1;
		}
		else 
		{
			//Factor_Voltage = GS_Metro_Device_Config.factor_voltage_int_ch2;	// STPM34
			Factor_Voltage = GS_Metro_Device_Config.factor_voltage_int_ch1;		// STPM33
			Factor_Current = GS_Metro_Device_Config.factor_current_int_ch2;       
		}
		
		bignum_from_int(&num1, raw_RMS_Voltage);
		bignum_from_int(&num2, Factor_Voltage);
		bignum_mul(&num1, &num2, &result);
		
		bignum_assign(&num1, &result);		// Copy result -> num1
		bignum_from_int(&num2, 10);
		bignum_mul(&num1, &num2, &result);
		
		bignum_rshift(&result, &result, 15);
		calc_RMS_Voltage	= bignum_to_int(&result);
		//--------------------------------------------------------
		bignum_from_int(&num1, raw_RMS_Current);
		bignum_from_int(&num2, Factor_Current);
		bignum_mul(&num1, &num2, &result);
		
		bignum_assign(&num1, &result);		// Copy result -> num1
		bignum_from_int(&num2, 10);
		bignum_mul(&num1, &num2, &result);
		
		bignum_rshift(&result, &result, 17);
		calc_RMS_Current	= bignum_to_int(&result);
		/*
		// Calculate real values with factors
		calc_RMS_Voltage = (uint64_t)raw_RMS_Voltage * Factor_Voltage;
		calc_RMS_Voltage *= 10;
		calc_RMS_Voltage >>= 15;
		
		calc_RMS_Current = (uint64_t)raw_RMS_Current * Factor_Current;
		calc_RMS_Current *= 10;
		calc_RMS_Current >>= 17; // 17 bits resolution revBC 
		*/
		
		*out_Metro_RMS_voltage = (uint32_t)calc_RMS_Voltage;
		*out_Metro_RMS_current = (uint32_t)calc_RMS_Current;
	}
}

static void Metro_HAL_Crc8Calc (uint8_t in_Data)
{
    uint8_t loc_u8Idx;
    uint8_t loc_u8Temp;
    loc_u8Idx=0;
    while(loc_u8Idx<8)
    {
        loc_u8Temp = (uint8_t)(in_Data ^ CRC_u8Checksum);
        CRC_u8Checksum<<=1;
        if(loc_u8Temp&0x80)
        {
            CRC_u8Checksum^=CRC_8;
        }
        in_Data<<=1;
        loc_u8Idx++;
    }
}

static u8 byteReverse(u8 n)
{
	n = (u8)(((n >> 1) & 0x55) | ((n << 1) & 0xaa));
	n = (u8)(((n >> 2) & 0x33) | ((n << 2) & 0xcc));
	n = (u8)(((n >> 4) & 0x0F) | ((n << 4) & 0xF0));
	return n;
}

static uint8_t Metro_HAL_CalcCRC8(uint8_t *pBuf)
{
    uint8_t     i;
    CRC_u8Checksum = 0x00;

    for (i=0; i<STPM3x_FRAME_LEN-1; i++)
    {
        Metro_HAL_Crc8Calc(pBuf[i]);
    }

    return CRC_u8Checksum;
}
// rx : a0 00 00 04 75
static uint8_t FRAME_for_UART_mode(u8 *pBuf)
{
	u8 temp[4],x,CRC_on_reversed_buf;
	
	for (x=0;x<(STPM3x_FRAME_LEN-1);x++)
	{
		temp[x] = byteReverse(pBuf[x]);
		//printf("%02x, ", (uint16_t)temp[x]);
	}
	CRC_on_reversed_buf = Metro_HAL_CalcCRC8(temp);
	//printf("%02x\n", (uint16_t)CRC_on_reversed_buf);
	return byteReverse(CRC_on_reversed_buf);
}

void STPM_Data_Write(void)
{
	uint8_t	i = 0;
	uint8_t	k = 0;
	uint8_t frame_with_CRC[STPM3x_FRAME_LEN];
	u32_data	p_writedata;
	
	Gu8_STPM3x_RW_Process_Flag	= STPM_WRITE_ING_MODE;
	
	RX_Queue_Clear(STPM3x_PORT);
	STPM3x_RxBuf.count	= 0;
	Gu8_STPM3x_RW_Res				= 0;
	//printf("\nQQQ\n");
	
	//for (k=0;k<Gu8_STPM3x_RW_Blocks;k+=2)
	{
		p_writedata.u32data = *G32ptr_STPM3x_RW_Data;
    	
    	//printf("\nkk : %d", (uint16_t)k);
    	
		frame_with_CRC[0] = 0xff;									// No read requested, put dummy frame
		frame_with_CRC[1] = (uint8_t)(Gu8_STPM3x_Write_Address);					// write Address requested
		frame_with_CRC[2] = p_writedata.byte.data[3];				// DATA for 16-bit register to be written, LSB
		frame_with_CRC[3] = p_writedata.byte.data[2];				// DATA for 16-bit register to be written, MSB
		frame_with_CRC[4] = FRAME_for_UART_mode(frame_with_CRC);	// Calculate CRC and put it at the end of the frame
		Uart_PutsTxQ(STPM3x_PORT, frame_with_CRC, STPM3x_FRAME_LEN);
		
		//if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
		if(G_Debug == DEBUG_STPM3x_REALDATA)
		{
			printf("\nSTPM WR TX1 : ");
			for (i=0;i<STPM3x_FRAME_LEN;i++)	printf("%02x, ", (uint16_t)frame_with_CRC[i]);
			printf("\n");
		}
		//if(in_wait_stpm == STPM_DELAY)	delay(1);	// 1ms
		
		frame_with_CRC[0] = 0xff;									// No read requested, put dummy frame
		frame_with_CRC[1] = (uint8_t)(Gu8_STPM3x_Write_Address + 1);					// write Address requested
		frame_with_CRC[2] = p_writedata.byte.data[1];				// DATA for 16-bit register to be written, LSB
		frame_with_CRC[3] = p_writedata.byte.data[0];				// DATA for 16-bit register to be written, MSB
		frame_with_CRC[4] = FRAME_for_UART_mode(frame_with_CRC);	// Calculate CRC and put it at the end of the frame
		Uart_PutsTxQ(STPM3x_PORT, frame_with_CRC, STPM3x_FRAME_LEN);
		
		//if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
		if(G_Debug == DEBUG_STPM3x_REALDATA)
		{
			printf("STPM WR TX2 : ");
			for (i=0;i<STPM3x_FRAME_LEN;i++)	printf("%02x, ", (uint16_t)frame_with_CRC[i]);
			printf("\n");
		}
		//if(in_wait_stpm == STPM_DELAY)	delay(1);	// 1ms
		
		//G32ptr_STPM3x_RW_Data++;		// Increment Pointer to next U8 data for the next loop
	}
}

uint8_t STPM3x_Write(uint8_t address, uint8_t nb_blocks, uint32_t *in_p_Buffer)
{
	uint8_t	ret	= 0;
	
	if(Gu8_STPM3x_RW_Process_Flag == 0)
	{
		if (nb_blocks==0)	Gu8_STPM3x_RW_Blocks = 2;
		else				Gu8_STPM3x_RW_Blocks = (uint8_t)(nb_blocks * 2);
		
		Gu8_STPM3x_RW_Process_Flag		= STPM_WRITE_MODE;
		Gu8_STPM3x_Write_Address		= address;
		
		G32ptr_STPM3x_RW_Data			= in_p_Buffer;
		Gu8_STPM3x_RW_Process_Tmr		= 0;
		Gu8_STPM3x_RW_Res				= 0;
		
		if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
		{
			printf("\nSTPM3x_Write address[0x%02x] block[%d]\n", (uint16_t)Gu8_STPM3x_Write_Address, (uint16_t)Gu8_STPM3x_RW_Blocks);
		}
		
		ret	= 1;
	}
	
	return ret;
}

/*
void SET_STPM_Read_Address(uint8_t address)
{
	uint8_t frame_with_CRC[STPM3x_FRAME_LEN];
	
	RX_Queue_Clear(STPM3x_PORT);
	STPM3x_RxBuf.count	= 0;
	
	frame_with_CRC[0] = address;		// put the read adress
	frame_with_CRC[1] = 0xFF;			// no write requested
	frame_with_CRC[2] = 0xFF;			// no Data
	frame_with_CRC[3] = 0xFF;			// no Data
	frame_with_CRC[4] = FRAME_for_UART_mode(frame_with_CRC);
	Uart_PutsTxQ(STPM3x_PORT, frame_with_CRC, STPM3x_FRAME_LEN);
	
	if(G_Debug == DEBUG_STPM3x_REALDATA)	printf("SET STPM3x Read Address (%d)\n", (uint16_t)address);
}
*/

void STPM_Data_Read_Req(void)
{
	uint8_t	i;
	uint8_t frame_with_CRC[STPM3x_FRAME_LEN];
	
	Gu8_STPM3x_RW_Process_Flag	= STPM_READ_REQ_MODE;
	
	RX_Queue_Clear(STPM3x_PORT);
	STPM3x_RxBuf.count	= 0;
	Gu8_STPM3x_RW_Res				= 0;
	
	
	frame_with_CRC[0] = *Gu8_STPM3x_Read_Address;		// put the read adress
	frame_with_CRC[1] = 0xFF;			// no write requested
	frame_with_CRC[2] = 0xFF;			// no Data
	frame_with_CRC[3] = 0xFF;			// no Data
	frame_with_CRC[4] = FRAME_for_UART_mode(frame_with_CRC);
	Uart_PutsTxQ(STPM3x_PORT, frame_with_CRC, STPM3x_FRAME_LEN);
	
	//if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
	if(G_Debug == DEBUG_STPM3x_REALDATA)
	{
		printf("STPM RD TX2 : ");
		for (i=0;i<STPM3x_FRAME_LEN;i++)	printf("%02x, ", (uint16_t)frame_with_CRC[i]);
		printf("\n");
	}
	
	
	// send FF to read next frame
	// Format the frame with Read base address
	frame_with_CRC[0] = 0xFF;	// put FF to read
	frame_with_CRC[1] = 0xFF;	// no write requested
	frame_with_CRC[2] = 0xFF;	// no Data
	frame_with_CRC[3] = 0xFF;	// no Data
	frame_with_CRC[4] = FRAME_for_UART_mode(frame_with_CRC);
	Uart_PutsTxQ(STPM3x_PORT, frame_with_CRC, STPM3x_FRAME_LEN);
	
	//if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
	if(G_Debug == DEBUG_STPM3x_REALDATA)
	{
		printf("STPM RD TX2 : ");
		for (i=0;i<STPM3x_FRAME_LEN;i++)	printf("%02x, ", (uint16_t)frame_with_CRC[i]);
		printf("\n");
	}
	//if(G_Debug == DEBUG_STPM3x_REALDATA)	printf("STPM3x Read REQ\n");
}

uint8_t STPM3x_Read(uint8_t *address, uint8_t nb_blocks, uint32_t *out_p_read_data)
{
	uint8_t	ret	= 0;
	
	if(Gu8_STPM3x_RW_Process_Flag == 0)
	{
		if(nb_blocks	== 0 )	nb_blocks	= 1;
		
		Gu8_STPM3x_RW_Process_Flag		= STPM_READ_MODE;
		Gu8_STPM3x_Read_Address			= address;
		Gu8_STPM3x_RW_Blocks			= nb_blocks;
		G32ptr_STPM3x_RW_Data			= out_p_read_data;
		Gu8_STPM3x_RW_Process_Tmr		= 0;
		Gu8_STPM3x_RW_Res				= 0;
		
		if(G_Debug == DEBUG_STPM3x || G_Debug == DEBUG_STPM3x_REALDATA)
		{
			printf("\nSTPM3x_Read address[0x%02x] block[%d]\n", (uint16_t)*Gu8_STPM3x_Read_Address, (uint16_t)Gu8_STPM3x_RW_Blocks);
		}
		
		ret	= 1;
	}
	
	return	ret;
}

void STPM3x_RW_Process(void)
{
	if(Gu8_STPM3x_RW_Process_Tmr == 0 && Gu8_STPM3x_RW_Process_Flag)
	{
		if(Gu8_STPM3x_RW_Process_Flag <= STPM_READ_REQ_MODE)
		{
			if(Gu8_STPM3x_RW_Res == 2)			// 응답이 있으면
			{
				Gu8_STPM3x_RW_Res	= 0;
				if(Gu8_STPM3x_RW_Blocks)
				{
					//Gu8_STPM3x_Read_Address	+= 2;
					Gu8_STPM3x_Read_Address++;
					STPM_Data_Read_Req();	// 다음데이터 요청
					Gu8_STPM3x_RW_Process_Tmr	= 20;		// 200ms
				}
				else						// 수신할 데이터가 없으면
				{
					Gu8_STPM3x_RW_Process_Flag	= 0;
				}
			}
			else
			{
				STPM_Data_Read_Req();			// 전송 or 재전송
				Gu8_STPM3x_RW_Process_Tmr	= 10;		// 100ms
			}
			
		}
		else if(Gu8_STPM3x_RW_Process_Flag <= STPM_WRITE_ING_MODE)
		{
			if(Gu8_STPM3x_RW_Res == 2)			// 응답이 있으면
			{
				Gu8_STPM3x_RW_Res	= 0;
				if(Gu8_STPM3x_RW_Blocks)
				{
					G32ptr_STPM3x_RW_Data++;	// Increment Pointer to next U8 data for the next loop
					Gu8_STPM3x_Write_Address	+= 2;
					STPM_Data_Write();			// 다음데이터 전송
					Gu8_STPM3x_RW_Process_Tmr	= 20;		// 200ms
				}
				else						// 수신할 데이터가 없으면
				{
					Gu8_STPM3x_RW_Process_Flag	= 0;
				}
			}
			else
			{
				STPM_Data_Write();				// 전송 or 재전송
				Gu8_STPM3x_RW_Process_Tmr	= 20;		// 200ms
			}
		}
	}
}
//------------------------------------------------------------------------------------------
void STPM3x_While(uint16_t timeout, uint8_t *debugprn)
{
	uint8_t	timeout_break = 0;
	
	if(timeout)
	{
		timeout_break	= 1;
		Gu16_1ms_delay_tmr	= timeout;	// ms
	}
	
	while(Gu8_STPM3x_RW_Process_Flag)
	{
		WDG_SetCounter();
		Timer10ms();
		STPM3x_UART_Process();
		STPM3x_RW_Process();
		
		if(timeout_break)
		{
			if(Gu16_1ms_delay_tmr == 0)
			{
				//GS_Metro_Device_Config.metro_stpm_reg.DSPCTRL3	= pG_Config->metroSetupDefault[2];
				printf("ERR: STPM3x_While %s\n", debugprn);
				Gu8_STPM3x_RW_Process_Flag	= 0;
				break;
			}
		}
	}
}

#ifdef	STPM3x_OFFSET_SETTING
int SET_STPM3x_Power_Offset(int argc, char *argv[])
{
	//double	In_Offset;
	int32_t	Offset;
	
	if(argc == 3)
	{
		Offset = atoh(argv[2]);	// step 0.00487, 511 = 2.487, 1023 = -0.005
		
		if(strcmp(argv[1],"1a") == 0)
		{
			// W_ACTIVE CH1
			pG_Config->metroSetupDefault[8]	&= ~BIT_MASK_STPM_OFFSET_A1;		// DSP CTRL 9
			pG_Config->metroSetupDefault[8]	|= (((uint32_t)(Offset)<<BIT_MASK_STPM_OFFSET_A_SHIFT)& BIT_MASK_STPM_OFFSET_A1); 
			printf("\n\noffset W_ACTIVE1 %ld\n\n", Offset);
		}
		else if(strcmp(argv[1],"1fa") == 0)
		{
			// F_ACTIVE CH1
			pG_Config->metroSetupDefault[8]	&= ~BIT_MASK_STPM_OFFSET_AF1;		// DSP CTRL 9
			pG_Config->metroSetupDefault[8]	|= (((uint32_t)(Offset)<<BIT_MASK_STPM_OFFSET_AF_SHIFT)& BIT_MASK_STPM_OFFSET_AF1); 
			printf("\n\noffset F_ACTIVE1 %ld\n\n", Offset);
		}
		else if(strcmp(argv[1],"1ra") == 0)
		{
			// REACTIVE CH1
			pG_Config->metroSetupDefault[9]	&= ~BIT_MASK_STPM_OFFSET_R1;		// DSP CTRL 10
			pG_Config->metroSetupDefault[9]	|= (((uint32_t)(Offset)<<BIT_MASK_STPM_OFFSET_R_SHIFT)& BIT_MASK_STPM_OFFSET_R1); 
			printf("\n\noffset REACTIVE1 %ld\n\n", Offset);
		}
		else if(strcmp(argv[1],"1rms") == 0)		// 1% 0x3fe, 2% 3fc	-1% 0x2, -2% 0x4
		{
			// APPARENT_RMS CH1
			pG_Config->metroSetupDefault[9]	&= ~BIT_MASK_STPM_OFFSET_S1;		// DSP CTRL 10
			pG_Config->metroSetupDefault[9]	|= (((uint32_t)(Offset)<<BIT_MASK_STPM_OFFSET_S_SHIFT)& BIT_MASK_STPM_OFFSET_S1); 
			printf("\n\noffset APPARENT1_RMS %ld\n\n", Offset);
		}
		//----------------------------------------------------------------------------------------------
		else if(strcmp(argv[1],"2a") == 0)
		{
			// W_ACTIVE CH2
			pG_Config->metroSetupDefault[10]	&= ~BIT_MASK_STPM_OFFSET_A2;		// DSP CTRL 11
			pG_Config->metroSetupDefault[10]	|= (((uint32_t)(Offset)<<BIT_MASK_STPM_OFFSET_A_SHIFT)& BIT_MASK_STPM_OFFSET_A2); 
			printf("\n\noffset W_ACTIVE2 %ld\n\n", Offset);
		}
		else if(strcmp(argv[1],"2fa") == 0)
		{
			// F_ACTIVE CH2
			pG_Config->metroSetupDefault[10]	&= ~BIT_MASK_STPM_OFFSET_AF2;		// DSP CTRL 11
			pG_Config->metroSetupDefault[10]	|= (((uint32_t)(Offset)<<BIT_MASK_STPM_OFFSET_AF_SHIFT)& BIT_MASK_STPM_OFFSET_AF2); 
			printf("\n\noffset F_ACTIVE2 %ld\n\n", Offset);
		}
		else if(strcmp(argv[1],"2ra") == 0)
		{
			// REACTIVE CH2
			pG_Config->metroSetupDefault[11]	&= ~BIT_MASK_STPM_OFFSET_R2;		// DSP CTRL 12
			pG_Config->metroSetupDefault[11]	|= (((uint32_t)(Offset)<<BIT_MASK_STPM_OFFSET_R_SHIFT)& BIT_MASK_STPM_OFFSET_R2);
			printf("\n\noffset REACTIVE2 %ld\n\n", Offset);
		}
		else if(strcmp(argv[1],"2rms") == 0)		// 1% 0x3fe, 2% 3fc	-1% 0x2, -2% 0x4
		{
			// APPARENT_RMS CH2
			pG_Config->metroSetupDefault[11]	&= ~BIT_MASK_STPM_OFFSET_S2;		// DSP CTRL 12
			pG_Config->metroSetupDefault[11]	|= (((uint32_t)(Offset)<<BIT_MASK_STPM_OFFSET_S_SHIFT)& BIT_MASK_STPM_OFFSET_S2); 
			printf("\n\noffset APPARENT2_RMS %ld\n\n", Offset);
		}
		Gu8_STPM3x_RW_Process_Flag	= 0;
		STPM3x_Write(STPM_DSPCTRL9, 4, (uint32_t*)&pG_Config->metroSetupDefault[8]);
		STPM3x_While(0, 0);
	}
	
	return 1;
}

#endif


// 테스트 : 전압은 228V, 전류는 8.2A(8A로 설정했지만 시간에 따라 조금식 변함)
// 전압 228.x로 켈리브레이션
// 전류1은 8.1A로 켈리브레이션(0.1A 감소한 값으로 설정)
// 전류2는 8.2A로 켈리브레이션
int SET_STPM3x_Calibration(int argc, char *argv[])
{
	double	value;
	double	cal, avrage = 0;
	uint32_t	tmp;
	uint8_t	i;
	
	if(argc == 3)
	{
		value = (double)atof(argv[2]);
		
		if(strcmp(argv[1],"v") == 0)
		{
			tmp	= pG_Config->metroSetupDefault[4] & 0xfffff000;		// DSP CTRL 5		채널1 전압 켈리브레이션
			tmp	|= 0x00000800;
			
			Gu8_STPM3x_RW_Process_Flag	= 0;
			STPM3x_Write(STPM_DSPCTRL5, 1, (uint32_t*)&tmp);
			STPM3x_While(0, (void*)"v1");
			
			tmp	= 0x0B8104e0;
			STPM3x_Write(STPM_DSPCTRL3, 1, (uint32_t*)&tmp);		// auto latch
			STPM3x_While(0, (void*)"v2");
			
			Gu16_1ms_delay_tmr	= 500;		// 500msec
			while(Gu16_1ms_delay_tmr)	WDG_SetCounter();
			
			for(i=0;i<15;i++)
			{
				Gu16_1ms_delay_tmr	= 100;		// 10msec
				while(Gu16_1ms_delay_tmr);
				
				STPM3x_Read((uint8_t *)&metro_stpm_Address[ADDR_STPM_DSP_REG14], 2, (uint32_t*)&GS_Metro_Device_Config.metro_stpm_reg.DSP_REG14);	// READ DSP_REG14 and DSP_REG15
				STPM3x_While(0, (void*)"V");
				tmp		= Metro_HAL_read_RMS_Voltage(INT_CHANNEL_1);
				printf("rawV %ld\n", tmp);
				avrage	+= (double)tmp;
			}
			avrage	/= (double)i;
			
			cal	= ((14336.0 * (Xv / avrage)) - 12288.0) + 0.5;
			printf("inV %f, Xv %f, Avr Vrms %f, CHV %f\n", value, Xv, avrage, cal);
			
			if(cal >= 0.0 && cal <= 4095.0)
			{
				printf("V  calibration[calculation %f, 0x%lx]\n", cal, (uint32_t)cal);
				
				pG_Config->metroSetupDefault[4]	&= 0xfffff000;		// DSP CTRL 5		채널1 전압 켈리브레이션
				pG_Config->metroSetupDefault[4]	|= (uint32_t)cal;
				
				Store_CurrentConfig();
			}
			else printf("ERR : V calibration[calculation %f, 0x%lx]\n", cal, (uint32_t)cal);
		}
		else if(strcmp(argv[1],"c1") == 0)
		{
			tmp	= pG_Config->metroSetupDefault[5] & 0xfffff000;		// DSP CTRL 6		채널1 전류 켈리브레이션
			tmp	|= 0x00000800;
			
			Gu8_STPM3x_RW_Process_Flag	= 0;
			STPM3x_Write(STPM_DSPCTRL6, 1, (uint32_t*)&tmp);
			STPM3x_While(0, (void*)"c10");
			
			tmp	= 0x0B8104e0;
			STPM3x_Write(STPM_DSPCTRL3, 1, (uint32_t*)&tmp);		// auto latch
			STPM3x_While(0, (void*)"c12");
			
			Gu16_1ms_delay_tmr	= 500;		// 500msec
			while(Gu16_1ms_delay_tmr)	WDG_SetCounter();
			
			for(i=0;i<15;i++)
			{
				Gu16_1ms_delay_tmr	= 100;		// 10msec
				while(Gu16_1ms_delay_tmr);
				
				STPM3x_Read((uint8_t *)&metro_stpm_Address[ADDR_STPM_DSP_REG14], 2, (uint32_t*)&GS_Metro_Device_Config.metro_stpm_reg.DSP_REG14);	// READ DSP_REG14 and DSP_REG15
				STPM3x_While(0, (void*)"c13");
				tmp		= Metro_HAL_read_RMS_Current(INT_CHANNEL_1);
				printf("rawA %ld\n", tmp);
				avrage	+= (double)tmp;
			}
			avrage	/= (double)i;
			
			
			cal	= ((14336.0 * (Xi / avrage)) - 12288.0) + 0.5;
			printf("inA %f, Xi %f, Avr Crms %f, CHC %f\n", value, Xi, avrage, cal);
			if(cal >= 0.0 && cal <= 4095.0)
			{
				//cal = (uint32_t)cal & 0x0fff;
				printf("C1 calibration[calculation %f, 0x%lx]\n", cal, (uint32_t)cal);
				
				pG_Config->metroSetupDefault[5]	&= 0xfffff000;		// DSP CTRL 6		채널1 전류 켈리브레이션
				pG_Config->metroSetupDefault[5]	|= (uint32_t)cal;
				
				Store_CurrentConfig();
			}
			else printf("ERR : C1 calibration[calculation %f, 0x%lx]\n", cal, (uint32_t)cal);
		}
		else if(strcmp(argv[1],"c2") == 0)
		{
			tmp	= pG_Config->metroSetupDefault[7] & 0xfffff000;		// DSP CTRL 8		채널2 전류 켈리브레이션
			tmp	|= 0x00000800;
			
			Gu8_STPM3x_RW_Process_Flag	= 0;
			STPM3x_Write(STPM_DSPCTRL8, 1, (uint32_t*)&tmp);
			STPM3x_While(0, (void*)"c20");
			
			tmp	= 0x0B8104e0;
			STPM3x_Write(STPM_DSPCTRL3, 1, (uint32_t*)&tmp);		// auto latch
			STPM3x_While(0, (void*)"c21");
			
			Gu16_1ms_delay_tmr	= 500;		// 500msec
			while(Gu16_1ms_delay_tmr)	WDG_SetCounter();
				
			for(i=0;i<15;i++)
			{
				Gu16_1ms_delay_tmr	= 100;		// 10msec
				while(Gu16_1ms_delay_tmr);
				
				STPM3x_Read((uint8_t *)&metro_stpm_Address[ADDR_STPM_DSP_REG14], 2, (uint32_t*)&GS_Metro_Device_Config.metro_stpm_reg.DSP_REG14);	// READ DSP_REG14 and DSP_REG15
				STPM3x_While(0, (void*)"c23");
				tmp		= Metro_HAL_read_RMS_Current(INT_CHANNEL_2);
				printf("rawA %ld\n", tmp);
				avrage	+= (double)tmp;
			}
			avrage	/= (double)i;
			
			
			cal	= ((14336.0 * (Xi / avrage)) - 12288.0) + 0.5;
			printf("inA %f, Xi %f, Avr Crms %f, CHC %f\n", value, Xi, avrage, cal);
			if(cal >= 0.0 && cal <= 4095.0)
			{
				//cal = (uint32_t)cal & 0x0fff;
				printf("C2 calibration[calculation %f, 0x%lx]\n", cal, (uint32_t)cal);
				
				pG_Config->metroSetupDefault[7]	&= 0xfffff000;		// DSP CTRL 8		채널2 전류 켈리브레이션
				pG_Config->metroSetupDefault[7]	|= (uint32_t)cal;
				
				Store_CurrentConfig();
			}
			else printf("ERR : C2 calibration[calculation %f, 0x%lx]\n", cal, (uint32_t)cal);
		}
	}
	return 1;
}
//------------------------------------------------------------------------------------------
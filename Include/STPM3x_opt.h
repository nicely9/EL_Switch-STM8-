#ifndef _STPM3x_H
#define _STPM3x_H

#include "stm8l15x.h"
#include "STPM3x_metrology.h"

//----------------------------------------------------------------------------------------------------------------
typedef enum
{
	SHUNT_0_001	= 1,	// 1mOhm,	Vmax 313V,	Current Gain 16, Imax 26A
	SHUNT_0_002,		// 2mOhm,	Vmax 313V,	Current Gain 8,  Imax 26A
	SHUNT_0_003,		// 3mOhm,	Vmax 313V,	Current Gain 8,  Imax 17A	or Current Gain 4,  Imax 35A
	SHUNT_0_004			// 4mOhm,	Vmax 313V,	Current Gain 4,  Imax 26A
}_shunt_;

#define		SHUNT	SHUNT_0_003		// 0.001 ~ 0.004, 0.001 = 1mOhm
//----------------------------------------------------------------------------------------------------------------
typedef enum
{
	AVG_FILTER_CH1	= 0,
	AVG_FILTER_CH2,
	//AVG_FILTER_CH1_A,
	//AVG_FILTER_CH2_A,
	MAX_AVG_FILTER
}_avr_filter_;

//#define		STPM3x_DIFF_A		0.015		// 15mA
#define		STPM3x_DIFF_A		0.050		// 50mA

#define		STPM_DELAY		1
#define		STPM_NO_DELAY	2
#define		FACTOR_POWER_ON_ENERGY		(858)	// (3600 * 16000000 / 0x4000000) = 858.3...

#define		ELEC_LIMIT_CURRENT_TIME		120		// 제한값 아래에 있으면 2분 후 차단

#define		STPM_READ_MODE				0x1
#define		STPM_READ_REQ_MODE			0x2
#define		STPM_WRITE_MODE				0x3
#define		STPM_WRITE_ING_MODE			0x4

#define	STPM3x_MAX_BUF	5
typedef struct
{
	uint8_t		buf[STPM3x_MAX_BUF];
	uint16_t	count;
}STPM3x_BUF;

typedef enum 
{
	LATCH_SYN_SCS = 1,  
	LATCH_SW,
	LATCH_AUTO
}METRO_Latch_Device_Type_t;

typedef enum 
{
	CHANNEL_NONE=0,
	CHANNEL_1,  
	CHANNEL_2,
	NB_MAX_CHANNEL  
}METRO_Channel_t; 

typedef enum 
{
  INT_NONE_CHANNEL=0,
  INT_CHANNEL_1,  
  INT_CHANNEL_2,
  CHANNEL_TAMPER
}METRO_internal_Channel_t; 


typedef struct
{
  uint8_t       metroTimerActive;
  
  //uint8_t       nbPhase;
//#ifdef	STPM3x_POWER
  int32_t       powerActive1;
  int32_t       powerReactive1;
  int32_t       powerApparent1;
  int32_t       powerActive2;
  int32_t       powerReactive2;
  int32_t       powerApparent2;
//#endif  
#ifdef	STPM3x_ENERGY
  int32_t       energyActive;
  int32_t       energyReactive;
  int32_t       energyApparent;
#endif
	
  uint32_t      rmsvoltage1;
  uint32_t      rmsvoltage2;
  uint32_t      rmscurrent1;
  uint32_t      rmscurrent2;
} metroData_t;

typedef struct
{
  uint32_t                    factor_power_int_ch1;
  uint32_t                    factor_energy_int_ch1;
  
  uint32_t                    factor_power_int_ch2;
  uint32_t                    factor_energy_int_ch2;
  
  uint32_t                    factor_voltage_int_ch1;
  uint32_t                    factor_current_int_ch1;
  
  uint32_t                    factor_voltage_int_ch2;
  uint32_t                    factor_current_int_ch2;
  
  METRO_STPM_TypeDef          metro_stpm_reg;
}METRO_Device_Config_t;

/*
typedef union{
	uint32_t	Data;
	struct{    
		uint32_t	CLRSS_TOx	:4;		
		uint32_t	ClassSS		:1;		
		uint32_t	ENVREFx		:1;	//  Enable internal voltage reference for primary channel:
									// 0: reference disabled ?external Vref required
									// 1: reference enabled
		uint32_t	TCx			:3;	// Temperature compensation coefficient selection for primary 
									// channel voltage reference VREF1
		uint32_t	rev_1		:8;
		uint32_t	AEMx		:1;	// Apparent energy mode for primary channel:
									// 0: use apparent RMS power
									// 1: use apparent vectorial power
		uint32_t	APMx		:1;	// Apparent vectorial power mode for primary channel:
									// 0: use fundamental power
									// 1: use active power					    
		uint32_t	BHPFVx		:1;	// Bypass hi-pass filter for primary voltage channel:
									// 0: HPF enabled
									// 1: HPF bypassed
		uint32_t	BHPFCx		:1;	// Bypass hi-pass filter for primary current channel:
									// 0: HPF enabled
									// 1: HPF bypassed
		uint32_t	ROCx		:1;	// Add Rogowski integrator t
		
		uint32_t	rev_2		:2;

		uint32_t	LPWx		:4;	// LED1 speed dividing factor: 0x0 = 2^(-4), 0xF = 2^11
									// Default 0x4 = 1
		uint32_t	LPCx		:2;	// LED1 pulse-out power selection:LPS1 [1:0]: 00,01,10,11
									// LED1 output: active, fundamental, reactive, apparent
		uint32_t	LCSx		:2;	// LCS1 [1:0]: 00,01,10,11LED1: primary channels, secondary channels, cumulative, 
									// sigma-delta bitstream
	}Bit;
}DSP_CR1_CR2;


typedef union{
	uint32_t	Data;
	struct{
		uint32_t	SAG_TIME_THR	:14;
		uint32_t	ZCR_SEL			:2;
		uint32_t	ZCR_EN			:1;	//ZCR/CLK pin output: 0: CLK 1: ZCR
		uint32_t	TMP_TOL			:2;
		uint32_t	TMP_EN			:1;
		uint32_t	SW_Reset		:1;	//SW reset brings the configuration registers to default
										//This bit is set to zero after this action automatically
		uint32_t	SW_Latch1		:1;	//Primary channel measurement register latch
										//This bit is set to zero after this action automatically
		uint32_t	SW_Latch2		:1;	//Secondary channel measurement register latch
										//his bit is set to zero after this action automatically
		uint32_t	SWAuto_Latch	:1;	//Automatic measurement register latch at 7.8125 kHz 
		uint32_t	LED_OFF1		:1;	//LED1 pin output disable 0: LED1 output on 1: LED1 output disabled
		uint32_t	LED_OFF2		:1;	//LED2 pin output disable 0: LED2 output on1: LED2 output disabled
		uint32_t	EN_CUM			:1;	//Cumulative energy calculation
										//0: cumulative is the sum of channel energies
										//1: total is the difference of energies
		uint32_t	REFFREQ			:1;	//Reference line frequency: 0: 50 Hz 1: 60 Hz
		uint32_t	rev				:4;
	};
}DSP_CR3;
*/
typedef union{
	uint32_t	u32data;
	struct{
		uint8_t	data[5];
	}byte;
}u32_data;

#ifdef	STPM3x_POWER

typedef enum 
{
  W_ACTIVE = 1,  
  F_ACTIVE,
  REACTIVE, 
  APPARENT_RMS,
  APPARENT_VEC,
  MOM_WIDE_ACT,
  MOM_FUND_ACT
}METRO_Power_selection_t;

extern int32_t Metro_Read_Power(METRO_Channel_t in_Metro_Channel, METRO_Power_selection_t in_Metro_Power_Selection);

#endif

#ifdef	STPM3x_ENERGY

typedef enum 
{
  LED_W_ACTIVE = 0,  
  LED_F_ACTIVE,
  LED_REACTIVE, 
  LED_APPARENT_RMS

}METRO_LED_Power_selection_t;

typedef enum 
{
  E_W_ACTIVE = 1,  
  E_F_ACTIVE,
  E_REACTIVE, 
  E_APPARENT,
  NB_MAX_TYPE_NRJ
}METRO_Energy_selection_t;

typedef struct
{
  int32_t       energy[NB_MAX_CHANNEL][NB_MAX_TYPE_NRJ];
  int32_t       energy_extension[NB_MAX_CHANNEL][NB_MAX_TYPE_NRJ];
}METRO_Data_Energy_t;

extern int32_t Metro_HAL_read_energy(METRO_internal_Channel_t in_Metro_int_Channel, METRO_Energy_selection_t in_Metro_Energy_Selection);
extern int32_t Metro_Read_energy(METRO_Channel_t in_Metro_Channel, METRO_Energy_selection_t in_Metro_Energy_Selection);

#endif


extern uint8_t	Gu8_STPM3x_Read_Tmr;
extern __IO uint8_t	Gu8_STPM3x_RW_Process_Tmr;
extern uint8_t	Gu8_STPM3x_RW_Process_Flag;
extern METRO_Device_Config_t	GS_Metro_Device_Config;

extern uint8_t	Gu8_ElecLimitCurrent_1_Tmr;
extern uint8_t	Gu8_ElecLimitCurrent_2_Tmr;

extern uint32_t	Gu32_RAW_RMS_Voltage;
extern uint32_t	Gu32_RAW_RMS_Current;
extern uint32_t	Gu32_RAW_RMS_Current_1;
extern uint32_t	Gu32_RAW_RMS_Current_2;
extern uint8_t	Gu8_STPM3x_COMM_Error_Tmr;

extern void STPM3x_UartData_Init(void);
extern void STPM3x_GPIO_Init(void);
extern void STPM3x_Init(void);
extern void irq_STPM3x_TX(void);
extern void irq_STPM3x_RX(void);
extern void STPM3x_Process(void);
//extern void STM3x_Reset(void);
//extern void STM3x_Soft_Reset(void);

extern void METRO_Set_Latch_device_type(METRO_Latch_Device_Type_t in_Metro_Latch_Device_Type);
extern uint32_t Metro_HAL_read_RMS_Voltage(METRO_internal_Channel_t in_Metro_int_Channel);
extern uint32_t Metro_HAL_read_RMS_Current(METRO_internal_Channel_t in_Metro_int_Channel);
extern void Metro_Read_RMS(METRO_Channel_t in_Metro_Channel, uint32_t * out_Metro_RMS_voltage, uint32_t * out_Metro_RMS_current, uint8_t in_RAW_vs_RMS);
extern uint8_t STPM3x_Read(uint8_t *address, uint8_t nb_blocks, uint32_t *out_p_read_data);
extern uint8_t STPM3x_Write(uint8_t address, uint8_t nb_blocks, uint32_t *in_p_Buffer);
extern void STPM3x_RW_Process(void);
extern void STPM3x_UART_Process(void);
extern void STPM_Data_Write(void);

extern int SET_STPM3x_Calibration(int argc, char *argv[]);
extern int SET_STPM3x_Power_Offset(int argc, char *argv[]);
extern void STPM3x_While(uint16_t timeout, uint8_t *debugprn);
extern double Gu16_Elec_1_Watt;
extern double Gu16_Elec_2_Watt;

#endif

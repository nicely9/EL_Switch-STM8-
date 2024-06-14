#ifndef _STPM3x_H
#define _STPM3x_H

#include "stm8l15x.h"
#include "STPM3x_metrology.h"

#define		STPM3x_BAUD		57600
#define		STPM_WAIT		1
#define		STPM_NO_WAIT	2

#define		FACTOR_POWER_ON_ENERGY		(858)	// (3600 * 16000000 / 0x4000000) = 858.3...

typedef struct
{
  uint8_t       metroTimerActive;
  
  //uint8_t       nbPhase;
#ifdef	STPM3x_POWER_ENERGY
  int32_t       powerActive;
  int32_t       powerReactive;
  int32_t       powerApparent;
  
  int32_t       energyActive;
  int32_t       energyReactive;
  int32_t       energyApparent;
#endif
	
  uint32_t      rmsvoltage;
  uint32_t      rmscurrent;
} metroData_t;

 /**
  * @brief METROLOGY Reset type
  *
  */  
   
typedef enum 
{
  RESET_SYN_SCS = 1,
  RESET_SW
}METRO_ResetType_t;  

/**
  * @brief METROLOGY External device Enable/Disable
  *
  */  
   
typedef enum 
{
  EXT_DISABLE = 0,  
  EXT_ENABLE    
}METRO_CMD_EXT_Device_t;  


 /**
  * @brief METROLOGY generic cmd Enable/Disable
  *
  */  
   
typedef enum 
{
  DEVICE_DISABLE = 0,  
  DEVICE_ENABLE = 1,
  NO_CHANGE
}METRO_CMD_Device_t;  


 /**
  * @brief METROLOGY  Voltage Channel definition
  *
  */  
   
typedef enum 
{
  V_1 = 1,  
  V_2,
  V_3, 
  V_4  
}METRO_Voltage_Channel_t; 

 /**
  * @brief METROLOGY  Current CHANNEL definition
  *
  */  
   
typedef enum 
{
  C_1 = 1,  
  C_2,
  C_3, 
  C_4  
}METRO_Current_Channel_t; 

 /**
  * @brief METROLOGY  Current Gain definition
  *
  */  
   
typedef enum 
{
  X2 = 0,  
  X4,
  X8, 
  X16  
}METRO_Gain_t; 


/**
  * @brief METROLOGY  Vref device definition
  *
  */  
   
typedef enum 
{
  EXT_VREF =0,
  INT_VREF  
}METRO_Vref_t;


 /**
  * @brief METROLOGY  Current CHANNEL definition
  *
  */  
   
typedef enum 
{
  PRIMARY = 0,  
  SECONDARY,
  ALGEBRIC, 
  SIGMA_DELTA  
}METRO_LED_Channel_t; 


 /**
  * @brief METROLOGY  LED Slection type
  *
  */  
   
typedef enum 
{
  LED1 = 1,  
  LED2 
}METRO_LED_Selection_t; 


/**
  * @brief METROLOGY  Power selection type
  *
  */  
   
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

/**
  * @brief METROLOGY  Calculation Power selection type
  *
  */  
   
typedef enum 
{
  FROM_RMS = 1,  
  FROM_PWIDE,
  FROM_PFUND
}METRO_Calculation_Power_selection_t;

/**
  * @brief METROLOGY  Latch device type
  *
  */  
typedef enum 
{
  LATCH_SYN_SCS = 1,  
  LATCH_SW,
  LATCH_AUTO
 }METRO_Latch_Device_Type_t;

/**
  * @brief METROLOGY  Voltage read type
  *
  */  
typedef enum 
{
  V_WIDE = 1,  
  V_FUND
 }METRO_Voltage_type_t;

/**
  * @brief METROLOGY  Current read type
  *
  */  
typedef enum 
{
  C_WIDE = 1,  
  C_FUND
 }METRO_Current_type_t;



/**
  * @brief METROLOGY  Tamper Tolerance type
  *
  */  
typedef enum 
{
  TOL_12_5 = 0,  
  TOL_8_33,
  TOL_6_25,
  TOL_3_125,
  NO_CHANGE_TOL
 }METRO_Tamper_Tolerance_t;


/**
  * @brief METROLOGY  ZCR Signal Selection
  *
  */  
typedef enum 
{
  ZCR_SEL_V1 = 0,  
  ZCR_SEL_C1,
  ZCR_SEL_V2,
  ZCR_SEL_C2,
  NO_CHANGE_ZCR
 }METRO_ZCR_Sel_t;

 
 /**
  * @brief METROLOGY  CLK  Selection
  *
  */  
typedef enum 
{
  CLK_SEL_7KHz = 0,  
  CLK_SEL_4MHz,
  CLK_SEL_4MHz_50,
  CLK_SEL_16MHz,
  NO_CHANGE_CLK
 }METRO_CLK_Sel_t;
 
   
  /**
  * @brief METROLOGY  Live Event type
  *
  */  
typedef enum 
{
  ALL_LIVE_EVENTS =0,
  LIVE_EVENT_REFRESHED,
  LIVE_EVENT_WRONG_INSERTION,
  LIVE_EVENT_VOLTAGE_SAG,  
  LIVE_EVENT_VOLTAGE_SWELL,
  LIVE_EVENT_CURRENT_SWELL,
  LIVE_EVENT_VOLTAGE_ZCR,
  LIVE_EVENT_CURRENT_ZCR,  
  LIVE_EVENT_VOLTAGE_PERIOD_STATUS,
  LIVE_EVENT_VOLTAGE_SIGNAL_STUCK,
  LIVE_EVENT_CURRENT_SIGNAL_STUCK,
  LIVE_EVENT_CURRENT_TAMPER,
  LIVE_EVENT_CURRENT_SIGN_CHANGE_APPARENT_POWER,
  LIVE_EVENT_CURRENT_SIGN_CHANGE_REACTIVE_POWER,
  LIVE_EVENT_CURRENT_SIGN_CHANGE_FUNDAMENTAL_POWER,
  LIVE_EVENT_CURRENT_SIGN_CHANGE_ACTIVE_POWER,
  LIVE_EVENT_CURRENT_OVERFLOW_APPARENT_NRJ,
  LIVE_EVENT_CURRENT_OVERFLOW_REACTIVE_NRJ,
  LIVE_EVENT_CURRENT_OVERFLOW_FUNDAMENTAL_NRJ,
  LIVE_EVENT_CURRENT_OVERFLOW_ACTIVE_NRJ,
  LIVE_EVENT_CURRENT_NAH
 }METRO_Live_Event_Type_t;

  /**
  * @brief METROLOGY Status type
  *
  */  
typedef enum 
{
  ALL_STATUS = 0,
  STATUS_REFRESHED,
  STATUS_TAMPER_DETECTED,
  STATUS_TAMPER_OR_WRONG,
  STATUS_VOLTAGE_SWELL_DOWN,
  STATUS_VOLTAGE_SWELL_UP,
  STATUS_VOLTAGE_SAG_DOWN,
  STATUS_VOLTAGE_SAG_UP,    
  STATUS_VOLTAGE_PERIOD_STATUS,
  STATUS_VOLTAGE_SIGNAL_STUCK,
  STATUS_CURRENT_OVERFLOW_APPARENT_NRJ,
  STATUS_CURRENT_OVERFLOW_REACTIVE_NRJ,
  STATUS_CURRENT_OVERFLOW_FUNDAMENTAL_NRJ,
  STATUS_CURRENT_OVERFLOW_ACTIVE_NRJ,
  STATUS_CURRENT_SIGN_APPARENT_POWER,
  STATUS_CURRENT_SIGN_CHANGE_REACTIVE_POWER,
  STATUS_CURRENT_SIGN_CHANGE_FUNDAMENTAL_POWER,
  STATUS_CURRENT_SIGN_CHANGE_ACTIVE_POWER,
  STATUS_CURRENT_SWELL_DOWN,
  STATUS_CURRENT_SWELL_UP,
  STATUS_CURRENT_NAH_TMP,
  STATUS_CURRENT_SIGNAL_STUCK
 }METRO_Status_Type_t;
 
  /**
  * @brief METROLOGY Status type
  *
  */  
typedef enum 
{
  ALL_STPM_LINK_STATUS = 0,
  STATUS_STPM_UART_LINK_BREAK,
  STATUS_STPM_UART_LINK_CRC_ERROR,
  STATUS_STPM_UART_LINK_TIME_OUT_ERROR,
  STATUS_STPM_UART_LINK_FRAME_ERROR,
  STATUS_STPM_UART_LINK_NOISE_ERROR,
  STATUS_STPM_UART_LINK_RX_OVERRUN,
  STATUS_STPM_UART_LINK_TX_OVERRUN,    
  STATUS_STPM_SPI_LINK_RX_FULL,
  STATUS_STPM_SPI_LINK_TX_EMPTY,
  STATUS_STPM_LINK_READ_ERROR,
  STATUS_STPM_LINK_WRITE_ERROR,
  STATUS_STPM_SPI_LINK_CRC_ERROR,
  STATUS_STPM_SPI_LINK_UNDERRUN,
  STATUS_STPM_SPI_LINK_OVERRRUN
 }METRO_STPM_LINK_IRQ_Status_Type_t;
  
 /**
  * @brief METROLOGY  Boolean  type
  *
  */  
typedef enum 
{
  BOOL_FALSE = 0,
  BOOL_TRUE  
}METRO_Bool_Type_t;
  
 /**
  * @brief METROLOGY External device Number
  *
  */  
   
typedef enum 
{
  HOST = 0,  
  EXT1,  
  NB_MAX_DEVICE
}METRO_NB_Device_t;  

 /**
  * @brief METROLOGY  CHANNEL definition
  *
  */  
   
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



 /**
  * @brief METROLOGY hardware Device type
  *
  */
     
typedef enum 
{
  Device_NONE=0,
  STM32 = 5,
  STPM32 = 6,                           
  STPM33,                            
  STPM34,
  NB_MAX_STPM
}METRO_Device_t;

typedef struct
{
  int32_t       energy[NB_MAX_CHANNEL][NB_MAX_TYPE_NRJ];
  int32_t       energy_extension[NB_MAX_CHANNEL][NB_MAX_TYPE_NRJ];
}METRO_Data_Energy_t;

#define    CHANNEL_MASK_CONF_CHANNEL_1     0x01
#define    CHANNEL_MASK_CONF_CHANNEL_2     0x02
#define    CHANNEL_MASK_CONF_CHANNEL_3     0x04
#define    CHANNEL_MASK_CONF_CHANNEL_4     0x08
  
#define    NB_NAX_CHANNEL                   3
 
#define    DEVICE_MASK_CONF                0x0F
#define    CHANNEL_MASK_CONF               0xF0

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

extern void STPM3x_GPIO_Init(void);
extern void STPM3x_Init(void);
extern void irq_STPM3x_TX(void);
extern void irq_STPM3x_RX(void);
extern void STPM3x_Process(void);

extern void METRO_Set_Latch_device_type(METRO_Latch_Device_Type_t in_Metro_Latch_Device_Type);
extern void METRO_Get_Data_device(void);
extern void METRO_Update_Measures(void);
extern int32_t Metro_HAL_read_energy(METRO_internal_Channel_t in_Metro_int_Channel, METRO_Energy_selection_t in_Metro_Energy_Selection);
extern int32_t Metro_Read_energy(METRO_Channel_t in_Metro_Channel, METRO_Energy_selection_t in_Metro_Energy_Selection);
extern int32_t Metro_Read_Power(METRO_Channel_t in_Metro_Channel,METRO_Power_selection_t in_Metro_Power_Selection);
extern void Metro_Read_RMS(METRO_Channel_t in_Metro_Channel,uint32_t * out_Metro_RMS_voltage,uint32_t * out_Metro_RMS_current, uint8_t in_RAW_vs_RMS);
extern uint32_t Metro_HAL_Stpm_write(uint8_t *in_p_data, uint8_t nb_blocks, uint32_t *in_p_Buffer, uint8_t in_wait_stpm);
extern uint32_t Metro_HAL_Stpm_Read(uint8_t *in_p_data, uint8_t nb_blocks, uint32_t * out_p_read_data);
extern int32_t  UTIL_FixPointMul(int32_t a, uint16_t b, uint8_t q);
//extern void  UTIL_ReverseTU32(uint8_t *p);

#endif

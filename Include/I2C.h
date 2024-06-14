#ifndef __I2C_MST_INT_H
#define __I2C_MST_INT_H

#define I2C_MAX_STANDARD_FREQ ((uint32_t)100000)
#define I2C_MAX_FAST_FREQ     ((uint32_t)400000)

// flag clearing sequence - uncoment next for peripheral clock under 2MHz
//#define I2C_TOUT  40		// 40ms
#define I2C_TOUT  100		// 100ms
#define set_tout_ms(a)    { Gu8_I2C_1ms_Tmr= a; }
#define dead_time() { /* _asm("nop"); _asm("nop"); */ }

#define WRITE			0
#define READ			1
#define SEV_BIT_ADDRESS	0
#define TEN_BIT_ADDRESS	1
#define STOP			0
#define NOSTOP			1

// Define I2C STATE MACHINE :
#define INI_00			00

// Write states 0x
#define SB_01			01
#define ADD10_02		02
#define ADDR_03			03
#define BTF_04			04

// Read states 1x
#define SB_11			11
#define ADD10_12		12
#define ADDR_13			13
#define BTF_14			14
#define BTF_15			15
#define RXNE_16			16
#define BTF_17			17
#define RXNE_18			18


#define	TMR_0s			0

#define	TMR_10ms_100ms	10
#define	TMR_10ms_200ms	20
#define	TMR_10ms_300ms	30
#define	TMR_10ms_400ms	40
#define	TMR_10ms_500ms	50
#define	TMR_10ms_600ms	60
#define	TMR_10ms_700ms	70
#define	TMR_10ms_800ms	80
#define	TMR_10ms_900ms	90
#define	TMR_10ms_1000ms	100

#define	TMR_100ms_1s	10
#define	TMR_100ms_2s	20
#define	TMR_100ms_3s	30
#define	TMR_100ms_4s	40
#define	TMR_100ms_5s	50
#define	TMR_100ms_6s	60
#define	TMR_100ms_7s	70
#define	TMR_100ms_8s	80
#define	TMR_100ms_9s	90
#define	TMR_100ms_10s	100


extern u8  u8_Direction;
extern u8  u8_NumByte_cpy ; 
extern u8* pu8_DataBuffer_cpy ;
extern u16 u16_SlaveAdd_cpy;
extern u8  u8_AddType_cpy;
extern u8  u8_NoStop_cpy;

extern __IO uint8_t	Gu8_I2C_1ms_Tmr;

extern void I2C_OPT_Init(uint32_t OutputClockFrequency);
extern void irq_i2c(void);
extern u8 I2C_OPT_WriteRegister(u16 u16_SlaveAdd, u8 RegisterAddress, u8 *pu8_DataBuffer, u8 u8_NumByteToWrite);
extern u8 I2C_OPT_ReadRegister(u16 u16_SlaveAdd, u8 *u8_DataBuffer, u8 u8_NumByteToRead);
extern u8 I2C_OPT_SET_ReadRegisterAddr(u16 u16_SlaveAdd, u8 RegisterAddress);
extern u8 I2C_OPT_Busy(void);
extern void ErrProc(uint8_t err);

#endif


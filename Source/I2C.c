/************************************************************************************
	Project		: 전자식스위치
	File Name	: I2C.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "i2c.h"

//-------------------------------------------------------------------------------------------------------------------------
u8  STATE;								// curent I2C states machine state
volatile u8 err_state;  	// error state 
volatile u8 err_save;   	// I2C1->SR2 copy in case of error

__IO uint8_t	Gu8_I2C_1ms_Tmr			= 0;

uint8_t		I2C_Tx_Buff[80];

u8  u8_Direction;

u8  u8_NumByte_cpy ; 
u8* pu8_DataBuffer_cpy ;
u16 u16_SlaveAdd_cpy;
u8  u8_AddType_cpy;
u8  u8_NoStop_cpy;
//-------------------------------------------------------------------------------------------------------------------------
/******************************************************************************
* Function name : ErrProc
* Description 	: Managed Error durring I2C communication to be modified depending of final application
* Input param 	: None
* Return 		    : None
* See also 		  : None.
*******************************************************************************/
void ErrProc(uint8_t err)
{
	err_save = I2C1->SR2;
	err_state = STATE;
	I2C1->SR2= 0;
	STATE = INI_00;
	set_tout_ms(0);
	
	Gu16_ERR_Flag	|=	ERR_FLAG_TOUCH_TIMEOUT;
	
	//if(G_Trace)	printf("ErrProc[%d], SR2[0x%02x] STATE[%d]\n", (uint16_t)err, (uint16_t)err_save, (uint16_t)err_state);
	printf("ErrProc[%d], SR2[0x%02x] STATE[%d]\n", (uint16_t)err, (uint16_t)err_save, (uint16_t)err_state);
	//while(1);		// for WDG reset
}

u8 I2C_OPT_Busy(void)
{
	if ((I2C1->SR3 & I2C_SR3_BUSY) == I2C_SR3_BUSY)	// check if communication on going
	{
		return 1;
	}
	if (STATE != INI_00)							// check if STATE MACHINE is in state INI_00
	{
		return 2;
	}
	return 0;
}
/******************************************************************************
* Function name : I2C_WriteRegister
* Description 	: write defined number bytes to slave memory starting with defined offset
* Input param 	: Slave Address ; Address type (TEN_BIT_ADDRESS or SEV_BIT_ADDRESS) ; STOP/NOSTOP ;
*									Number byte to Write ; address of the application send buffer
* Return 		    : 0 : START Writing not performed -> Communication onging on the bus
*                 1 : START Writing performed 
* See also 		  : None.
*******************************************************************************/
u8 I2C_OPT_WriteRegister(u16 u16_SlaveAdd, u8 RegisterAddress, u8 *pu8_DataBuffer, u8 u8_NumByteToWrite)
{
	if(u8_NumByte_cpy)	return 0;
	if(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))	return 0;
	if((I2C1->CR2 & I2C_CR2_STOP) == I2C_CR2_STOP)	return 0;
	
	I2C_AcknowledgeConfig(I2C1, ENABLE);					//I2C1->CR2 |= (uint8_t)(~I2C_CR2_ACK);
	I2C_AckPositionConfig(I2C1, I2C_AckPosition_Current);	//I2C1->CR2			&= (uint8_t)(~I2C_CR2_POS);		// reset POS
	
	u8_Direction		= WRITE;					// setup I2C comm. in write
	u16_SlaveAdd_cpy	= u16_SlaveAdd;
	
	I2C_Tx_Buff[0]		= RegisterAddress;
	memcpy((void*)&I2C_Tx_Buff[1], (void*)pu8_DataBuffer, u8_NumByteToWrite);
	pu8_DataBuffer_cpy	= (void*)&I2C_Tx_Buff;
	u8_NumByte_cpy		= (uint8_t)(u8_NumByteToWrite+1); 
	
	I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_ERR) , ENABLE);
	I2C_GenerateSTART(I2C1, ENABLE);
	set_tout_ms(I2C_TOUT);							// set comunication Timeout
	
	return 1;
}

u8 I2C_OPT_SET_ReadRegisterAddr(u16 u16_SlaveAdd, u8 RegisterAddress)
{
	if(u8_NumByte_cpy)	return 0;
	if(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))	return 0;
	if((I2C1->CR2 & I2C_CR2_STOP) == I2C_CR2_STOP)	return 0;
	
	I2C_AcknowledgeConfig(I2C1, ENABLE);					//I2C1->CR2			|= I2C_CR2_ACK;				// set ACK
	I2C_AckPositionConfig(I2C1, I2C_AckPosition_Current);	//I2C1->CR2			&= (uint8_t)(~I2C_CR2_POS);		// reset POS
	
	u8_Direction		= WRITE;					// setup I2C comm. in write
	u16_SlaveAdd_cpy	= u16_SlaveAdd;
	
	I2C_Tx_Buff[0]		= RegisterAddress;
	pu8_DataBuffer_cpy	= (void*)&I2C_Tx_Buff;
	u8_NumByte_cpy		= 1; 
	
	I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_ERR) , ENABLE);
	I2C_GenerateSTART(I2C1, ENABLE);
	set_tout_ms(I2C_TOUT);							// set comunication Timeout
	
	return 1;
}
/******************************************************************************
* Function name : I2C_ReadRegister
* Description 	: Read defined number bytes from slave memory starting with defined offset
* Input param 	: Slave Address ; Address type (TEN_BIT_ADDRESS or SEV_BIT_ADDRESS) ; STOP/NOSTOP ;
*									Number byte to Read ; address of the application receive buffer
* Return 		    : 0 : START Reading not performed -> Communication onging on the bus
*                 1 : START Reading performed 
* See also 		  : None
*******************************************************************************/
u8 I2C_OPT_ReadRegister(u16 u16_SlaveAdd, u8 *u8_DataBuffer, u8 u8_NumByteToRead)
{
	if(u8_NumByte_cpy)	return 0;
	if(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))	return 0;
	if((I2C1->CR2 & I2C_CR2_STOP) == I2C_CR2_STOP)	return 0;
	
	I2C_AcknowledgeConfig(I2C1, ENABLE);					//I2C1->CR2			|= I2C_CR2_ACK;				// set ACK
	I2C_AckPositionConfig(I2C1, I2C_AckPosition_Current);	//I2C1->CR2			&= (uint8_t)(~I2C_CR2_POS);		// reset POS
	
	u8_Direction		= READ;						// setup I2C comm. in write
	u16_SlaveAdd_cpy	= u16_SlaveAdd;
	pu8_DataBuffer_cpy	= u8_DataBuffer;
	u8_NumByte_cpy		= u8_NumByteToRead; 
	
	//I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT) , ENABLE);
	//I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_BUF) , ENABLE);
	//I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR) , ENABLE);
	I2C_ITConfig(I2C1, (I2C_IT_TypeDef)(I2C_IT_EVT | I2C_IT_ERR) , ENABLE);
	I2C_GenerateSTART(I2C1, ENABLE);
	set_tout_ms(I2C_TOUT);							// set comunication Timeout
	
	return 1;
}

/*
void irq_i2c(void)
{
	u8 sr1,sr2,cr2 ;
	
	sr1 = I2C1->SR1;	// Get Value of Status registers and Control register 2
	sr2 = I2C1->SR2;
	cr2 = I2C1->CR2;

	if (sr2 != 0)		// Check for error in communication
	{
		ErrProc(1);					
	}
	
	if ((sr1 & I2C_SR1_SB) == 1)	// Start bit detected
	{
		switch(STATE)
		{
			case SB_01:				// Write
				if (u8_AddType_cpy == TEN_BIT_ADDRESS)
				{
					I2C1->DR = (u8)(((u16_SlaveAdd_cpy >> 7) & 6) | 0xF0);  // send header of 10-bit device address (R/W = 0)
					STATE = ADD10_02; 
					break;
				} else {
					I2C1->DR = (u8)(u16_SlaveAdd_cpy << 1);   // send 7-bit device address & Write (R/W = 0)
					STATE = ADDR_03; 
					break;
				}
								
			case SB_11:				// Read
				if (u8_AddType_cpy == TEN_BIT_ADDRESS)
				{
					I2C1->DR = (u8)(((u16_SlaveAdd_cpy >> 7) & 6) | 0xF1);// send header of 10-bit device address (R/W = 1)
				} else {
					I2C1->DR = (u8)((u16_SlaveAdd_cpy << 1)|1) ; // send 7-bit device address & Write (R/W = 1)
				}
				STATE = ADDR_13; 
				break;
			
			default:
				//ErrProc(2);
				I2Cx->CR2 |= I2C_CR2_STOP;
				break;
		}
	}
	if (I2C1->SR1 & I2C_SR1_ADD10)				// ADD10(첫번째 주소 전송완료)	TEN_BIT_ADDRESS
	{
		switch(STATE)
		{
			case ADD10_02:							// 10bit address 나머지 전송
				I2C1->DR = (u8)(u16_SlaveAdd_cpy);	// send lower 8-bit device address & Write  
				STATE = ADDR_03;
				break;
	
			default:
				ErrProc(3);
				break;
		}
	}
	
	if ((sr1 & I2C_SR1_ADDR) == I2C_SR1_ADDR)	// ADDR(주소전송 완료)		Address sent (master mode) / matched (slave mode)
	{
		switch(STATE)
		{					
			case ADDR_13:
				if (u8_NumByte_cpy == 3)		// 데이터 READ 첫번째 루틴(3바이트만 읽을 때, 총 3개 루틴)
				{
					I2C1->SR3;
					STATE = BTF_15;
					break;
				}
				if (u8_NumByte_cpy == 2)		// 데이터 READ 첫번째 루틴(2바이트만 읽을 때, 총 2개 루틴)
				{
					I2C1->CR2 |= I2C_CR2_POS;	// set POS bit
					I2C1->SR3;					// Clear Add Ack Flag
					I2C1->CR2 &= (uint8_t)(~I2C_CR2_ACK);	// set No ACK
					STATE = BTF_17;
					break;
				}
				if (u8_NumByte_cpy == 1)		// 데이터 READ 첫번째 루틴(1바이트만 읽을 때, 총 2개 루틴)
				{
					I2C1->CR2 &= (uint8_t)(~I2C_CR2_ACK);
					I2C1->SR3;					// Clear Add Ack Flag
					I2C1->CR2 |= I2C_CR2_STOP;
					I2C1->ITR |= I2C_ITR_ITBUFEN;
					STATE = RXNE_18;
					break;
				}
				if (u8_NumByte_cpy > 3)			// 데이터 READ 첫번째 루틴(4바이트 이상 읽을 때, 총 x개 루틴)
				{
					I2C1->SR3;
					STATE = BTF_14;
					break;
				}
				ErrProc(4);
				break;
			case ADDR_03:						// 데이터 WRITE 첫번째 루틴(터치 register address)
				I2C1->SR3;						// Clear Add Ack Flag
				I2C1->DR = *pu8_DataBuffer_cpy++;
				u8_NumByte_cpy--;
				STATE = BTF_04;
				break;
			default:
				ErrProc(5);
				break;
		}
	}
	if ((sr1  & I2C_SR1_RXNE)==I2C_SR1_RXNE)			// Data Register not Empty (receivers)
	{
		switch(STATE)
		{
			case RXNE_18:								// 데이터 READ 두번째 루틴(1바이트만 읽을 때, 총 2개 루틴)
				*(pu8_DataBuffer_cpy++) = I2C1->DR;		// Read next data byte
				STATE = INI_00;
				set_tout_ms(0);
				break;
			case RXNE_16:								// 데이터 READ 세번째 루틴(3바이트만 읽을 때 1바이트 남았을 때 또는 4바이트 이상 읽을 때 1바이트 남았을 때, 총 3개 루틴)
				*(pu8_DataBuffer_cpy++) = I2C1->DR;     // Read next data byte
				STATE = INI_00;
				set_tout_ms(0);
				break;
		}
		I2C1->ITR &= (uint8_t)(~I2C_ITR_ITBUFEN);		// Disable Buffer interrupts (errata)
	}
	if ((sr1 & I2C_SR1_BTF) == I2C_SR1_BTF)				// BTF		Byte Transfer Finished
	{
		switch(STATE)
		{
			case BTF_17:								// 데이터 READ 두번째 루틴(2바이트만 읽을 때, 총 2개 루틴)
				I2C1->CR2 |= I2C_CR2_STOP;				// generate stop request here (STOP=1)
				*(pu8_DataBuffer_cpy++) = I2C1->DR;		// Read next data byte
				*(pu8_DataBuffer_cpy++) = I2C1->DR;		// Read next data byte
				STATE = INI_00;
				set_tout_ms(0);
				break;
								
			case BTF_14:								// 데이터 READ 두번째 루틴(4바이트 이상 읽을 때, 총 x개 루틴)
				*(pu8_DataBuffer_cpy++) = I2C1->DR;
				u8_NumByte_cpy--;
				if (u8_NumByte_cpy <= 3)				// 3개 이하로 남았을 때
				{
					STATE = BTF_15;
				}
				break;
			
			case BTF_15:								// 데이터 READ 두번째 루틴(3바이트만 읽을 때, 또는 3바이트 남았을 때, 총 3개 루틴)
				I2C1->CR2 &= (uint8_t)(~I2C_CR2_ACK);	// Set NACK (ACK=0)
				*(pu8_DataBuffer_cpy++) = I2C1->DR;		// Read next data byte
				I2C1->CR2 |= I2C_CR2_STOP;				// Generate stop here (STOP=1)
				*(pu8_DataBuffer_cpy++) = I2C1->DR;		// Read next data byte
				I2C1->ITR |= I2C_ITR_ITBUFEN;			// Enable Buffer interrupts (errata)
				STATE = RXNE_16;
				break;
												
			case BTF_04:								// 데이터 WRITE 두번째 루틴(데이터)
				if ((u8_NumByte_cpy) && ((I2C1->SR1 & I2C_SR1_TXE) == I2C_SR1_TXE))		// 데이터 레지스터(DR) empty
				{
					I2C1->DR = *pu8_DataBuffer_cpy++;	// Write next data byte
					u8_NumByte_cpy--;
					break;
				} 
				else 
				{
					if (u8_NoStop_cpy == 0)
					{										
						I2C1->CR2 |= I2C_CR2_STOP;					// Generate stop here (STOP=1)
					}
					else
					{
						I2C1->ITR = 0;								// disable interrupt 
					}
					STATE = INI_00;
					set_tout_ms(0);
					break;
				}
		}
	}
}
*/
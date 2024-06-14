#ifndef _LCD_H
#define _LCD_H

#ifdef _ONE_SIZE_LIGHT_n_ELEC_MODEL_
typedef union
{
	uint8_t			TN_LCD_Buff[7];
	struct
	{
		// uint8_t		COM3_SEG_11_8	:4;		//COM3 4 ~ 7
		uint8_t		D1				:1;		//COM 8
		uint8_t		Icon_ELEC_1		:1;		//COM 9
		uint8_t		D2				:1;		//COM 10
		uint8_t		Icon_Auto1		:1;		//COM 11
		uint8_t		D3				:1;		//COM 12
		uint8_t		Icon_ELEC_2		:1;		//COM 13
		uint8_t		D4				:1;		//COM 14
		uint8_t		Icon_Auto2		:1;		//COM 15

		uint8_t		Icon_W			:1;		//COM 16
		uint8_t		COM3_SEG_23_17	:7;		//COM 17 ~ 19

		uint8_t		COM2_SEG_7_4	:4;
		uint8_t		E1				:1;		//COM2 8
		uint8_t		C1				:1;		//COM2 9
		uint8_t		E2				:1;		//COM2 10
		uint8_t		C2				:1;		//COM2 11

		uint8_t		E3				:1;		//COM2 12
		uint8_t		C3				:1;		//COM2 13
		uint8_t		E4				:1;		//COM2 14
		uint8_t		C4				:1;		//COM2 15
		uint8_t		COM2_SEG_19_16	:4;

		// uint8_t		COM1_SEG_11_8	:4;		//COM1 4 ~ 7
		uint8_t		G1				:1;		//COM1 8
		uint8_t		B1				:1;		//COM1 9
		uint8_t		G2				:1;		//COM1 10
		uint8_t		B2				:1;		//COM1 11
		uint8_t		G3				:1;		//COM1 12
		uint8_t		B3				:1;		//COM1 13
		uint8_t		G4				:1;		//COM1 14
		uint8_t		B4				:1;		//COM1 15

		uint8_t		COM0_SEG_7_4	:4;
		uint8_t		F1				:1;		//COM0 8
		uint8_t		A1				:1;		//COM0 9
		uint8_t		F2				:1;		//COM0 10
		uint8_t		A2				:1;		//COM0 11

		uint8_t		F3				:1;		//COM0 12
		uint8_t		A3				:1;		//COM0 13
		uint8_t		F4				:1;		//COM0 14
		uint8_t		A4				:1;		//COM0 15
		uint8_t		COM0_SEG_19_16	:4;
	}Bit;
}_TN_LCD_SEG_;

#else

typedef union
{
	uint8_t			TN_LCD_Buff[8];
	struct
	{
		uint8_t		Icon_ELEC_2		:1;		// COM0 SEG8	bit 0
		uint8_t		D1				:1;		// COM0	SEG9
		uint8_t		C1				:1;
		uint8_t		D2				:1;
		uint8_t		C2				:1;
		uint8_t		D3				:1;
		uint8_t		C3				:1;
		uint8_t		D4				:1;
		uint8_t		C4				:1;
		uint8_t		COM0_SEG_23_17	:7;
			
		uint8_t		COM1_SEG_7_4	:4;
		uint8_t		Icon_Auto2		:1;		// COM1 SEG8	bit 1
		uint8_t		E1				:1;		// COM1 SEG9
		uint8_t		G1				:1;
		uint8_t		E2				:1;
		uint8_t		G2				:1;
		uint8_t		E3				:1;
		uint8_t		G3				:1;
		uint8_t		E4				:1;
		uint8_t		G4				:1;
		uint8_t		COM1_SEG19_17	:3;
		
		uint8_t		Icon_Auto1		:1;		// COM2 SEG8	bit 0
		uint8_t		F1				:1;		// COM2	SEG9
		uint8_t		B1				:1;
		uint8_t		F2				:1;
		uint8_t		B2				:1;
		uint8_t		F3				:1;
		uint8_t		B3				:1;
		uint8_t		F4				:1;
		uint8_t		B4				:1;
		uint8_t		COM2_SEG_23_17	:7;
		
		uint8_t		COM3_SEG_7_4	:4;
		uint8_t		Icon_ELEC_1		:1;		// COM3 SEG8	bit 1
		uint8_t		A1				:1;		// COM3 SEG9
		uint8_t		COM3_SEG_10		:1;
		uint8_t		A2				:1;
		uint8_t		COM3_SEG_12		:1;
		uint8_t		A3				:1;
		uint8_t		Icon_DIM		:1;
		uint8_t		A4				:1;
		uint8_t		Icon_W			:1;
		uint8_t		COM3_SEG19_17	:3;
	}Bit;
}_TN_LCD_SEG_;
#endif

extern uint16_t		Gu16_LCD_Watt_1;
extern uint16_t		Gu16_LCD_Watt_2;

extern uint8_t	Gu8_LCD_ElecLimitCurrent_Flashing_Flag;
extern uint8_t	Gu8_LCD_ElecLimitCurrent_Tmr;
extern uint8_t	Gu8_LCD_DIM_Tmr;
extern uint8_t	Gu8_INFO_Disp_Tmr;
extern uint8_t	Gu8_Back_Light_Flag;

extern void TN_LCD_Test(void);
extern void TN_LCD_Init(void);
extern void TN_LCD_Process(void);
extern void BACK_LIGHT_Init(void);
extern void BACK_LIGHT_Process(void); 
extern void LCD_TEST(int argc, char *argv[]);
extern void LCD_GLASS_DisplayString(uint8_t* ptr);

#endif
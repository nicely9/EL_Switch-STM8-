
#ifndef __TOUCH_H__
#define __TOUCH_H__


#define	TOUCH_1_ADDR	(uint8_t)(0xB8>>1)	// GT308L CtrlPin OPEN
#define	TOUCH_2_ADDR	(uint8_t)(0xB4>>1)	// GT308L CtrlPin GND

#define	ALL_LED				0xFF
#define	POWER_SAVING_TMR	50		// 5sec
#define	SETTING_half_1S_TMR	5
#define	SETTING_1S_TMR		10		// 1sec	그룹설정 또는 대기전력차단 설정을 위해 그룹스위치를 1초이상 눌러야 한다
#define	SETTING_2S_TMR		20
#define	SETTING_3S_TMR		30
#define	SETTING_4S_TMR		40
#define	SETTING_5S_TMR		50
//#define	SETTING_10S_TMR		100
#define	SETTING_10S_TMR		99

#define SETTING_GAS_4S_TMR 4		//Gu16_GAS_Off_Tmr를 100ms 타이머에서 1000ms 타이머로 변경하면서 추가.
#define SETTING_GAS_5S_TMR 5

typedef enum
{
	IGNORE_CLEAR		= 0,
	
	SET_IGNORE_SWITCH,
	SET_SPECIAL_SWITCH,
	
	IGNORE_MAX
}_touch_ignore_;

typedef enum
{
	LED_Level_0	= 0,
	LED_Level_1,		// 06.66%
	LED_Level_2,		// 13.33%
	LED_Level_3,		// 20.00%
	LED_Level_4,		// 26.66%
	LED_Level_5,		// 33.33%
	LED_Level_6,		// 40.00%
	LED_Level_7,		// 46.66%
	LED_Level_8,		// 53.33%
	LED_Level_9,		// 60.00%
	LED_Level_10,		// 66.66%
	LED_Level_11,		// 73.33%
	LED_Level_12,		// 8.000%
	LED_Level_13,		// 86.66%
	LED_Level_14,		// 93.33%
	LED_Level_15		// 100%
}_led_brightness_;

typedef enum
{
	LED_OFF__LIGHT_IS_ON		= 0,	// 전등이 켜지면 LED OFF, 꺼지면 LED ON
	LED_OFF__LIGHT_IS_ON_2,				// 전등이 켜지면 LED OFF, 꺼지면 LED ON(절전모드에서 미등)
	LED_LOW_LEVEL__LIGHT_IS_OFF			// 전등이 켜지면 LED ON, 전등이 꺼지면 LED 미등(Low level on)
}_led_mode_;

typedef enum
{
	_MODE_	= 0,
	//SENSITIVITY_MODE	= _MODE_,
	TOUCH_MODE			= _MODE_,
	LED_PWM_MODE		= _MODE_+10,
	SENSITIVITY_MODE	= _MODE_+20
}_touch_n_led_mode_;


#pragma pack(push, 1)
typedef union
{
	uint8_t		Byte[2];
	struct
	{
		uint8_t			TS1		:1;	// Bit0
		uint8_t			TS2		:1;
		uint8_t			TS3		:1;
		uint8_t			TS4		:1;
		uint8_t			TS5		:1;
		uint8_t			TS6		:1;
		uint8_t			TS7		:1;
		uint8_t			TS8		:1;
			
		uint8_t			TS9		:1;	// Bit0
		uint8_t			TS10	:1;
		uint8_t			TS11	:1;
		uint8_t			TS12	:1;
		uint8_t			TS13	:1;
		uint8_t			TS14	:1;
		uint8_t			TS15	:1;
		uint8_t			TS16	:1;
	}Bit;
}_TOUCH_16_CHANNEL_;

typedef union
{
	uint8_t		Byte;
	struct
	{
		uint8_t			TS1		:1;	// Bit0
		uint8_t			TS2		:1;
		uint8_t			TS3		:1;
		uint8_t			TS4		:1;
		uint8_t			TS5		:1;
		uint8_t			TS6		:1;
		uint8_t			TS7		:1;
		uint8_t			TS8		:1;
	}Bit;
}_TOUCH_CHANNEL_;

typedef union
{
	uint8_t		Byte;
	struct
	{
		uint8_t			rev1			:2;	// Bit0
		uint8_t			Flag			:1;	// 0 = Single touch Mode, 1 = Multi touch Mode
		uint8_t			rev2			:5;
	}Bit;
}_H04_;

typedef union
{
	uint8_t		Byte;
	struct
	{
		uint8_t			CalibrationTime	:4;		// The calibration time to protect from environmental change, (CalibrationTime * 100ms) + 1Period, Deviation : ±30% (@5.0V)
		uint8_t			TouchPeriod		:3;		// If the TOUCH_PEIROD is increased, it will be stronger to electrical noise. But, the response time is slower
		uint8_t			rev				:1;
	}Bit;
}_H07_;

typedef union
{
	uint8_t		Byte;
	struct
	{
		uint8_t			rev1			:1;
		uint8_t			OUT_Polarity	:1;		// 0 = Active Low, 1 = Active High
		uint8_t			rev2			:2;
		uint8_t			INT_Mode		:1;		// 0 = Pulse Mode, 1 = Level Mode
		uint8_t			rev3			:3;
	}Bit;
}_H3A_;

typedef union
{
	uint8_t		Byte;
	struct
	{
		uint8_t			SleepMode		:1;		// 0 = Operation mode, 1 = SleepMode
		uint8_t			rev				:3;
		uint8_t			PWM_Enable		:1;		// PWM Generation
		uint8_t			Sen_IDLE_Time	:2;		// 0 = 4ms, 1 = 30ms, 2 = 80ms, 3 = 420ms
		uint8_t			rev3			:1;
	}Bit;
}_H3B_;

typedef union
{
	uint8_t		Byte[4];
	struct
	{
		uint8_t			N1				:4;		// 0 ~ 15
		uint8_t			N2				:4;
			
		uint8_t			N3				:4;
		uint8_t			N4				:4;
			
		uint8_t			N5				:4;
		uint8_t			N6				:4;
			
		uint8_t			N7				:4;
		uint8_t			N8				:4;
	}Bit;
}_H48_H4B_;

typedef union
{
	uint8_t		Byte;
	struct
	{
		uint8_t			rev1			:5;
		uint8_t			MON_Rst			:1;		// 0 = not active and clear bit by user, 1 = active and set bit by GT308L
		uint8_t			rev2			:2;
	}Bit;
}_H4F_;

typedef struct
{
	// GT308L
	_TOUCH_CHANNEL_	TS_Enable;		// 03H	0xFF	0 = Disable, 1 = Enable
	_H04_			TouchMode;		// 04H	0x51	Single touch Mode
	_TOUCH_CHANNEL_	PWM_Enable;		// 05H	0xFF	0 = Output Mode, 1 = PWM Mode
	
	_H07_			H07;			// 07H	0x27	TouchPeriod 2, CalibrationTime 7
	
	_H3A_			H3A;			// 3AH	0x05	Pulse Mode, Active Low
	_H3B_			H3B;			// 3BH	0x30	IDLE_Time 30ms, PWM Generation Enable
	
	uint8_t			Sensitivity[8];	// 40H ~ 47H	Default High 0x0C, Middle 0xF0, Low 0x18
	_H48_H4B_		PWM_Duty;		// 48H ~ 4BH	0 ~ 15
	
	_H4F_			H4F;			// 4FH	0x20
}_GT308L_;

typedef struct
{
	uint8_t			MAX_TouchChip;
	uint16_t		Address[2];
	//-------------------------------------------------------------------------------------------
	_GT308L_		GT308L[2];
	//-------------------------------------------------------------------------------------------
}TOUCH_CFG;

#define	MAX_EVENT_BUF	50
typedef struct
{
	uint8_t		GET_Event_Cnt;
	uint8_t		PUT_Event_Cnt;
	uint8_t		Ctrl[MAX_EVENT_BUF];
	uint8_t		Flag[MAX_EVENT_BUF];
}EVNET_BUF;

#pragma pack(pop)

extern TOUCH_CFG			cmpTouchConfig;
extern TOUCH_CFG			TouchConfig;
extern _TOUCH_16_CHANNEL_	TouchState;					// 2AH	ReadOnly
extern uint8_t				Touch_Ignore_Flag[MAX_SWITCH];
extern uint8_t				Gu8_TouchChip_INT[2];
extern uint8_t				Gu8_TouchChip_INT_Tmr[2];
extern uint8_t				Gu8_Sensitivity[8];
extern uint8_t				Gu8_LightGroup_SET_Flag;
extern uint8_t				Gu8_Touch_Check_Tmr;

extern void Touch_GPIO_Init(void);
extern void Touch_Init(void);
extern void Touch_Chip_Init(void);
extern uint8_t SET_Touch_Config_Send(TOUCH_CFG *cmpTouchConfig, uint16_t chip);

extern void TouchSwitch_Event_Ctrl(_TOUCH_16_CHANNEL_ *touch_state, uint8_t tmr);
extern void Touch_and_LED_Process(void);
extern void SET_Touch_Ignore_Flag(uint8_t switch_led, uint8_t state);
extern uint8_t GET_Touch_Ignore_Flag(uint8_t switch_led);
extern void irq_Touch_1(void);
extern void irq_Touch_2(void);
extern void SET_Touch_Config(TOUCH_CFG *cmpTouchConfig, uint16_t i);
extern uint16_t TouchConfigCmp(_GT308L_ *cmp1, _GT308L_ *cmp2);
extern void Touch_Check(void);
// extern void Touch_Chip_Reset(void);		//20201112
// extern void touchchiptest(void);		//20201112
// extern void touchsend(void);			//20201112

#endif

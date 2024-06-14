#ifndef __EEPROM_H__
#define __EEPROM_H__

#define FLASH_MEMORY	1
#define EEPROM_MEMORY	2
//#define	CONFIG_KEY			(uint16_t)0xAA55
#define	CONFIG_KEY			(uint8_t)0x55

//#define MAX_EEPROM_DATA		256		// 256
#define MAX_EEPROM_DATA		200		// 200 바이트만 사용하여 상태저장
#define EEPROM_BASE_ADDR	0x1000		// 256Byte(STM8L052R8)
#define EEPROM_END			0x10FF

#define MAX_FLASH_DATA		255		// 255
#define FLASH_END_ADDR		0x017FFF	// STM8L052R8	64K
#define FLASH_BASE_ADDR		(uint32_t)(FLASH_END_ADDR	- MAX_FLASH_DATA)
// 플래시 주소를 하나 밀고 ID를 박아넣는 방법 고려. 0x017FFF - 255 = 0x17DAA. 
// 0x017DAA에 ID를 넣고 플래시 주소는 0x017DAB부터 시작? 
// 0x017FFF - 254?
#define	MAX_SWITCH			16	// 스위치			16
#define	MAX_RELAY			6	// 릴레이			6
#define	MAX_LIGHT			6	// 전등			6
#define	MAX_3WAY			2	// 3로			2
#define	MAX_DIMMING			2	// 디밍			2
#define	MAX_ELECTRICITY		2	// 전열			2
#define	MAX_REV				10

// 프로토콜에 따른 회로기준
/*
typedef enum
{
	CIRCUIT_NON	= 0,
	CIRCUIT_1,
	CIRCUIT_2,
	CIRCUIT_3,
	CIRCUIT_4,
	CIRCUIT_5,
	CIRCUIT_6,
	CIRCUIT_7,
	CIRCUIT_8,
	
	CIRCUIT_MAX
}_circuit_no_;
*/

// 제어비트(릴레이 및 PWM 출력)
#define	CONTROL_BIT_RELAY_NON		0x00
#define	CONTROL_BIT_RELAY_1			0x01
#define	CONTROL_BIT_RELAY_2			0x02
#define	CONTROL_BIT_RELAY_3			0x04
#define	CONTROL_BIT_RELAY_4			0x08
#define	CONTROL_BIT_RELAY_LATCH_1	0x10
#define	CONTROL_BIT_RELAY_LATCH_2	0x20
#define	CONTROL_BIT_DIMMING_1		0x40
#define	CONTROL_BIT_DIMMING_2		0x80

#define	LIGHT_RELAY			(CONTROL_BIT_RELAY_1|CONTROL_BIT_RELAY_2|CONTROL_BIT_RELAY_3|CONTROL_BIT_RELAY_4|CONTROL_BIT_RELAY_LATCH_1|CONTROL_BIT_RELAY_LATCH_2|CONTROL_BIT_DIMMING_1|CONTROL_BIT_DIMMING_2)	// 전등에 사용할 수 있는 릴레이
#define	ELECTRICITY_RELAY	(CONTROL_BIT_RELAY_LATCH_1|CONTROL_BIT_RELAY_LATCH_2)													// 전열에 사용할 수 있는 릴레이
#define	DIMMING_PWM			(CONTROL_BIT_DIMMING_1|CONTROL_BIT_DIMMING_2)	// 디밍에 사용할 수 있는 PWM 포트

typedef enum
{
	OFF	= 0,
	ON,
	INVERSE,
	//ON_OFF,
	
	LIGHT_STATE,
	ELECTRICITY_STATE,
	
	FORCE_BEEP		= 200,
	BEEP_AC_WARRING,
	BEEP_ONE,
	BEEP_TWO,
	BEEP_MEL,
	BEEP_30MINUTES,
	BEEP_60MINUTES,
	BEEP_90MINUTES
	
}_state_;

typedef enum
{
	mapping_ITEM_DISABLE	= 0,
	
	mapping_ITEM_LIGHT_1	= 1,		// 전등 1 (릴레이 제어)
	mapping_ITEM_LIGHT_2,				// 전등 2 (릴레이 제어)
	mapping_ITEM_LIGHT_3,				// 전등 3 (릴레이 제어)
	mapping_ITEM_LIGHT_4,				// 전등 4 (릴레이 제어)
	mapping_ITEM_LIGHT_5,				// 전등 5 (릴레이 제어)
	mapping_ITEM_LIGHT_6,				// 전등 6 (릴레이 제어)
	
	mapping_ITEM_BATCH_LIGHT_OFF,		// 일괄소등
	
	mapping_ITEM_3WAY_1,				// 3로 1 (릴레이 제어)
	mapping_ITEM_3WAY_2,				// 3로 2 (릴레이 제어)
	mapping_ITEM_DIMMING_LIGHT_1,		// 디밍1 스위치(PWM 제어)	저장된 밝기로 즉시제어
	mapping_ITEM_DIMMING_LIGHT_2,		// 디밍2 스위치(PWM 제어)	저장된 밝기로 즉시제어
	
	mapping_ITEM_ELECTRICITY_1,			// 전열 1 (릴레이 제어)
	mapping_ITEM_ELECTRICITY_2,			// 전열 2 (릴레이 제어)
	
	mapping_ITEM_LIGHT_GROUP,			// 전등 GROUP 스위치
	mapping_ITEM_LIGHT_ALL,				// 전등 ALL 스위치
	mapping_ITEM_ELECTRICITY_ALL,		// 전열 ALL 스위치
	
	mapping_ITEM_DIMMING_UP,			// 디밍  스위치 UP
	mapping_ITEM_DIMMING_DN,			// 디밍  스위치 DOWN
	
	mapping_ITEM_SETUP,					// 설정
	
	mapping_ITEM_GAS,					// 가스차단
	mapping_ITEM_ELEVATOR,				// 엘리베이터 호츨
	
	mapping_ITEM_SLEEP,		//기상/취침
	
	mapping_ITEM_COLOR_TEMP_UP,
	mapping_ITEM_COLOR_TEMP_DN,
	
	mapping_ITEM_COOK,
	mapping_ITEM_BATCH_LIGHT_n_GAS,
	mapping_ITEM_BATCH_LIGHT_n_COOK,
	mapping_ITEM_GAS_n_COOK,
	mapping_ITEM_MAX
}_mapping_flag_;

typedef enum
{
	mapping_SWITCH_DISABLE	= 0,
	mapping_SWITCH_1,
	mapping_SWITCH_2,
	mapping_SWITCH_3,
	mapping_SWITCH_4,
	mapping_SWITCH_5,
	mapping_SWITCH_6,
	mapping_SWITCH_7,
	mapping_SWITCH_8,
	mapping_SWITCH_9,
	mapping_SWITCH_10,
	mapping_SWITCH_11,
	mapping_SWITCH_12,
	mapping_SWITCH_13,
	mapping_SWITCH_14,
	mapping_SWITCH_15,
	mapping_SWITCH_16,
	mapping_SWITCH_MAX
}_mapping_switch_flag;

#pragma pack(push, 1)
typedef struct
{
	uint16_t		N1		:1;	// Bit0
	uint16_t		N2		:1;
	uint16_t		N3		:1;
	uint16_t		N4		:1;
	uint16_t		N5		:1;
	uint16_t		N6		:1;
	uint16_t		N7		:1;
	uint16_t		N8		:1;
		
	uint16_t		N9		:1;	// Bit0
	uint16_t		N10		:1;
	uint16_t		N11		:1;
	uint16_t		N12		:1;
	uint16_t		N13		:1;
	uint16_t		N14		:1;
	uint16_t		N15		:1;
	uint16_t		N16		:1;
}_16BIT_;

typedef union
{
	uint16_t	Word;
	_16BIT_		Bit;
}_SW_STATE_;

typedef struct
{
	uint8_t			N1		:1;	// Bit0
	uint8_t			N2		:1;
	uint8_t			N3		:1;
	uint8_t			N4		:1;
	uint8_t			N5		:1;
	uint8_t			N6		:1;
	uint8_t			N7		:1;
	uint8_t			N8		:1;
}_8BIT_;

typedef struct
{
	uint8_t			Dimming1	:4;	// Bit0
	uint8_t			Dimming2	:4;
}_DIMMING_BIT_;

typedef struct 
{
	uint8_t			Color_Temp1 :4;
	uint8_t			Color_Temp2 :4;
}_Color_Temp_BIT_;		//210622


typedef struct
{
	uint8_t			Batch		:1;
	uint8_t			Gas			:1;
	uint8_t			Elevator	:1;
	uint8_t			Cook		:1;
	uint8_t			Threeway	:1;
	uint8_t			Rev			:3;
}_BLOCK_FLAG_;

typedef struct
{
	uint8_t			Auto1					:1;		// Bit0
	uint8_t			Auto2					:1;
	uint8_t			LED_Mode				:2;		// 0 , 1, 2, 3

	uint8_t			BeepMode				:1;	//20230811
	uint8_t			DelayOFF				:1;
	uint8_t			BatchState				:2;	//20230811
	//uint8_t			rev						:4;
	// uint8_t			BeepMode				:3;
}_STATE_ETC_BIT_;

typedef struct
{
	uint16_t		ADC_Temperature	:1;		// 0 = disable,	1 = enable			// bit 0
	uint16_t		ADC_AC_Voltage	:1;		// 0 = disable,	1 = enable
	// uint16_t		ADC_CT_Sensor1	:1;		// 0 = disable,	1 = enable
	// uint16_t		ADC_CT_Sensor2	:1;		// 0 = disable,	1 = enable
	uint16_t		TOUCH_Chip_Type	:2;		// 0 = 4ch*1,	1 = 8ch*1,	2 = 8ch*2,	3 = 16ch*1
	uint16_t		LCD_Type		:2;		// 0 = disable,	1 = enable,	2 = x,	3 = x
	uint16_t		PWM_Dimming		:2;		// 0 = disable,	1 = PWM1,	2 = PWM2
	
	uint16_t		PWM_Color_Temp	:2;		// 0 = disable,	1 = PWM1,	2 = PWM2 			//210622 추가
	uint16_t		ThreeWay		:2;		// 0 = disable,	1 = 3로1,	2 = 3로2
	uint16_t		IR				:1;		// 0 = disable,	1 = enable
	// uint16_t		ADC_5_0V_Ref	:1;
	// uint16_t		ADC_3_3V_Ref	:1;
	uint16_t		STPM3x			:1;		// 0 = disable,	1 = enable
	uint16_t		Back_Light		:1;		//220211
	uint16_t		RELOAD_VALUE	:1;		//220211
}_ENABLE_FLAG;

typedef struct
{
	uint8_t			Sensitivity[8];	// 40H ~ 47H	Default High 0x0C, Middle 0xF0, Low 0x18
}_CONFIG_GT308L_;

// Flash Memory struct
typedef struct
{
	uint8_t			KEY;											// Key
	uint8_t			Mode;											// Mode		차 후 특정모드 동작을 위해 rev		//210621 uint16-> uint8
	uint8_t			RS485_ID;										// RS-485 ID									//210621 uint16-> uint8
	uint8_t			RS485_Elec_ID;									// KOCOM에서 전등, 전열 아이디 따로 사용해야함		//210923
	uint8_t			Protocol_Type;									// CVNET, HYUNDAI 등
	uint8_t			Protocol_IntervalTime;							// RS-485 IntervalTime
	uint8_t			Protocol_RES_DelayTime;							// RS-485 응답 지연시간
	uint8_t			LightCount;										// 조명 수
	uint8_t			ThreeWayCount;									// 3로 수
	uint8_t			DimmingCount;									// 디밍 수
	uint8_t			ElectricityCount;								// 전열 수
	uint8_t			Dimming_MAX_Level;								// 디밍단계														13
	uint8_t			Color_Temp_MAX_Level;							// 전등색 단계									//210622 추가
	uint8_t			Mapping_ITEM_TSN[mapping_ITEM_MAX];				// 항목기준 터치스위치 번호										19
	uint8_t			Mapping_ITEM_Control[mapping_ITEM_MAX];			// 항목기준 동작 제어장치										19
	uint8_t			Mapping_TSN_ITEM[MAX_SWITCH];					// 터치스위치 기준 항목											16
	uint8_t			Mapping_TSN_OnTmr[MAX_SWITCH];					// 터치스위치 ON 시간(설정된 시간동안 누르고 있어야 ON)			16
	uint8_t			Mapping_TSN_OffTmr[MAX_SWITCH];					// 터치스위치 OFF 시간(설정된 시간동안 누르고 있어야 OFF)		16
	uint8_t			Factory_Mapping_ALL_Light;						// 전체등 스위치 릴레이											1
	uint8_t			Factory_Mapping_ALL_Electricity;				// 전체전열 스위치 릴레이										1
	_ENABLE_FLAG	Enable_Flag;									// Touch 및 Relay는 제어장치 목록에 없으면 Disable 임			2
	_CONFIG_GT308L_	GT308L[2];										//													16
	uint32_t		metroSetupDefault[20];							//													20
	uint8_t			model;
	uint8_t			crc;											//													1
	//uint8_t			Mapping_LightCircuitI_ITEM[CIRCUIT_MAX];		// 회로에 할당된 전등 항목									9
	//uint8_t			Mapping_ElectricityCircuit_ITEM[CIRCUIT_MAX];	// 회로에 할당된 전열 항목									9	
	//double			CT1_Offset;										//	4
	//double			CT2_Offset;										//	4
	//double			ACV_Offset;										//	4	
	//uint16_t			BeepFreq;										//													1
	//uint8_t			BeepDuty;										//													1	
}EL_CONFIG;																// pG_Config										total 

// EEPROM Memory struct
typedef struct
{
	uint8_t				KEY;								// 								1
	_SW_STATE_			SW_State;							// 스위치 OnOff 상태			2
	_DIMMING_BIT_		Dimming_Level;						// 디밍 값						1
	_Color_Temp_BIT_	Color_Temp_Level;					// 전등색 값					1
	_STATE_ETC_BIT_		ETC;								//								1
	uint8_t				User_Mapping_ALL_Light;				// 전체스위치 릴레이			1
	// uint8_t				Batch_State;
	uint8_t				crc;								// CRC							1
}EL_STATE;												// pG_State					7Byte 저장시간 약 7ms
#pragma pack(pop)


extern EL_CONFIG	*pG_Config,		GS_Config;
extern EL_STATE		*pG_State,		GS_State;
extern uint16_t		Gu16_ElecLimitCurrent_1;
extern uint16_t		Gu16_ElecLimitCurrent_2;

extern void eeprom_Init(void);
extern void SET_Defaultdata(void);
extern void SET_State_Defaultdata(void);
extern uint32_t Verification_Config(void);

extern void Debug_StateData(void);
extern void STATE_Prn(void);

// extern uint8_t	light_circuit2item(uint8_t circuit);			// 전등 회로에 따른 항목 리턴
// extern uint8_t	light_item2circuit(uint8_t item);				// 전등 항목에 따른 회로번호 리턴
// extern uint8_t	electricity_circuit2item(uint8_t circuit);		// 전열 회로에 따른 항목 리턴
// extern uint8_t	electricity_item2circuit(uint8_t item);			// 전열 항목에 따른 회로번호 리턴
extern uint8_t	item2tsn(uint8_t item);	
extern uint8_t	tsn2item(uint8_t touch_switch);
extern uint8_t	item2ctrl(uint8_t item);
extern uint8_t	ctrl2item(uint8_t ctrl);
extern void Store_CurrentState(void);
extern void eeprom_diff_store_Process(void);
extern void Store_CurrentConfig(void);
extern void USART_Bootloader_Enable(void);
extern void USART_Bootloader_Disable(void);
// extern void SET_Mapping(uint8_t item, uint8_t circuit, uint8_t ctrl, uint8_t on_tmr, uint8_t off_tmr, uint8_t touch_switch);
extern void SET_Mapping(uint8_t item, uint8_t ctrl, uint8_t on_tmr, uint8_t off_tmr, uint8_t touch_switch);
extern uint8_t	eeprom_crc(uint8_t *st_ptr, uint8_t *et_ptr, uint8_t debug_prn);
extern uint8_t* str_Control(uint8_t control);
extern uint8_t Gu8_model;
extern void model_check(void);
extern void Store_ElecLimitCurrent(void);
extern void RS485_ID_Init(void);
#endif

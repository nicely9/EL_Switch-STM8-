#ifndef __LED_H
#define __LED_H

#define	LED_ON_LEVEL				LED_Level_6
#define	LED_OFF_LEVEL				LED_Level_0

#define	LED_ON_NORMAL_LEVEL			LED_Level_10
#define	LED_ON_POWERSAVE_LEVEL		LED_Level_6
#define	LED_OFF_POWERSAVE_LEVEL		LED_Level_1

#define	LED_ON_POWERSAVE_LOW_LEVEL	LED_Level_1
	
typedef enum
{
	LED_OFF		= 10,
	LED_ON,
	LED_FLASHING,
	//LED_DELAYED_OFF
	LED_DETECT_ON,
	
	LED_UNDEFINED	= 0xFF
}_led_state_;

extern uint8_t				LED_State_Flag[MAX_SWITCH];
extern uint8_t				LED_Flashing_Flag[MAX_SWITCH];
extern uint8_t				Gu8_LED_Flashing_Tmr;
extern uint8_t				Gu8_LED_TEST_Tmr;		//210106

extern void LED_Init(void);
extern void LED_Output_Process(void);
extern void ALL_Electricity_Switch_LED_Ctrl(void);
extern void ALL_n_Group_Light_Switch_LED_Ctrl(void);



extern void SET_Touch_Ignore_Flag(uint8_t switch_led, uint8_t state);
extern uint8_t GET_Touch_Ignore_Flag(uint8_t switch_led);
extern void SET_LED_State(uint8_t switch_led, uint8_t state);
extern uint8_t GET_LED_State(uint8_t switch_led);

#endif
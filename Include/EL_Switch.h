#ifndef __EL_SWITCH_H
#define __EL_SWITCH_H

typedef union
{
	uint8_t		Byte;
	_8BIT_		Bit;
}_CONTROL_STATE_;

#define	DELAY_OFF_PASS	0
#define	DELAY_OFF_CHECK	1

extern uint8_t				Gu8_PowerSaving_Tmr;
extern EVNET_BUF			Relay_CtrlBuff, PWM_CtrlBuff;
extern uint8_t				Gu8_SWITCH_Delay_OFF_Tmr[mapping_ITEM_MAX];
extern uint8_t				Gu8_SWITCH_Delay_Flag[mapping_ITEM_MAX];
extern uint8_t				Gu8_Special_Function_Key;
extern uint8_t				Gu8_Special_Function_Key_Tmr;
extern uint16_t				Gu16_Light_Sleep_tmr;
extern uint8_t				Gu8_Sleep_Set_Tmr;
extern void EL_Switch_Init(void);
extern void EventCtrl(uint8_t touch_switch, uint8_t Flag);
extern void SWITCH_Delay_OFF_Process(void);
extern void SET_SWITCH_Delay_OFF_Tmr(uint8_t item, uint8_t tmr);
extern uint8_t GET_SWITCH_Delay_OFF_Tmr(uint8_t item);
extern void SET_SWITCH_Delay_OFF_Flag(uint8_t item, uint8_t Flag);
extern uint8_t GET_SWITCH_Delay_OFF_Flag(uint8_t item);
extern void PUT_RelayCtrl(uint8_t Ctrl, uint8_t Flag);
extern void PUT_PWMCtrl(uint8_t Ctrl, uint8_t Flag);
extern uint8_t Event_Empt(void);
extern void GET_Event(void);
extern uint8_t SET_Switch_State(uint8_t touch_switch, uint8_t Flag);
extern uint8_t GET_Switch_State(uint8_t touch_switch);
extern uint8_t	GET_Light_State(void);
extern uint8_t	GET_Electricity_State(void);
extern void ALL_Light_Switch_Ctrl(uint8_t mapping, uint8_t Flag, uint8_t DelayOFF_Flag);
extern void ALL_Electricity_Switch_Ctrl(uint8_t mapping, uint8_t Flag);
extern void Control_Recovery_Init(void);
extern void Control_Recovery(void);

extern void Special_Function_Key_Process(uint8_t touch_switch, uint8_t tmr);
extern uint8_t	GET_Light_ON_Count(void);
extern uint8_t GET_Electricity_Count(void);
extern void TouchSwitch_Action_Check(uint8_t touch_switch, uint8_t tmr);
extern void Light_Sleep_Process(void);
extern uint8_t Gu8_Sleep_Flag;
extern uint16_t Gu16_Touch_Err_Tmr;
extern uint8_t Gu8_Touch_Err_Flag;
extern uint8_t Gu8_Touch_Err_Cnt;
extern uint8_t Gu8_Color_Temp_Flag;
extern uint8_t Gu8_Dim_Flag;
extern uint8_t Gu8_Light_n_ETC_Touch_Flag;
extern uint8_t Gu8_Elec_Touch_Flag;
extern uint8_t Gu8_Direct_Control;
#if defined(_ONE_SIZE_LIGHT_1_n_SLEEP_) || defined(_ONE_SIZE_LIGHT_2_n_SLEEP_)
extern void Sleep_Level(uint8_t touch_switch);
#endif
#endif

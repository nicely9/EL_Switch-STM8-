#ifndef _DIMMING_H
#define _DIMMING_H


typedef enum
{
	LEVEL_EQUALL	= 0,
	LEVEL_UP,
	LEVEL_DN
}_pwm_level_;

extern void Dimming_Init(void);
extern void PWM_Ctrl(uint8_t pwm, uint8_t Flag);
extern void PWM_Level_Set(uint8_t pwm, uint8_t ctrl);
extern uint8_t	DimmingLevelExchange(uint8_t level);
extern uint8_t ColorTempLevelExchange(uint8_t level);
extern uint16_t PWM_Freq2Period(uint32_t freq, uint32_t prescaler);
extern uint16_t Duty2CCR(uint16_t Duty, uint16_t period);
extern int Debug_Dimming_Freq(int argc, char *argv[]);
extern void irq_Dimming(void);
extern void Dimming_Smooth_Process(void);

#endif
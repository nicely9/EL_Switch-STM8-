#ifndef _DEBUG_H
#define _DEBUG_H


typedef enum 
{
	DEBUG_HOST	= 1,
	DEBUG_HOST_REALDATA,
	
	DEBUG_STPM3x,
	DEBUG_STPM3x_REALDATA,
	DEBUG_STPM3x_DATA,
	
	DEBUG_TOUCH_SENSITIVITY,
	
	MAX_DEBUG
}_debug_;


extern uint8_t	G_Debug;
extern uint8_t	G_Trace;

#ifdef	_ADC_
extern uint8_t	Gu8_ADC_Offset_Cnt;
extern uint8_t	Gu8_ADC_Offset_1_Flag;
extern uint8_t	Gu8_ADC_Offset_2_Flag;
#endif

extern void Debug_Init(void);
extern void irq_Debug_TX(void);
extern void irq_Debug_RX(void);
extern void Debug_Process(void);

extern void Debug_Puts(uint8_t *buf, int len);
extern void Debug_Put(uint8_t data);
extern void Continuity_Prn(uint8_t prn, uint8_t cnt);

extern u32 atoh(char *s);

#endif

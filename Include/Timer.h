#ifndef _TIMER_H
#define _TIMER_H

extern __IO uint16_t	Gu16_Check_Tmr;
extern __IO uint16_t	Gu16_1ms_delay_tmr;
extern __IO uint8_t		Gu8_10ms_Tmr;
extern __IO uint8_t		Gu8_100ms_Tmr;
extern __IO uint8_t		Gu8_1000ms_Tmr;
extern __IO uint8_t		Gu8_10ms_Flag;
#ifndef _KOCOM_PROTOCOL_
extern __IO uint8_t     Gu8_RS_485_Enable_Tmr;
#endif

extern void TIM_Init(void);
extern void irq_TIM4_Timer(void);
extern void Timer10ms(void);
extern void Timer100ms(void);
extern void Timer1000ms(void);
extern void irq_TIM2_Timer(void);
#endif

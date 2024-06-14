#ifndef _ZEROCROSSING_H
#define _ZEROCROSSING_H

extern __IO uint8_t	Gu8_ZeroCrossing_Tmr;
extern __IO uint8_t	Gu8_ZeroCrossing_Clr_Tmr;
extern uint8_t Gu8_ZeroCrossing_Err_Flag;
extern void ZeroCrossing_Init(void);
extern void irq_ZeroCrossing(void);
extern void BlackOut_Process(void);

#endif

#ifndef _RELAY_H
#define _RELAY_H

extern uint8_t	Gu8_LatchRelay_1_Floating_Tmr;
extern uint8_t	Gu8_LatchRelay_2_Floating_Tmr;


extern void Relay_Init(void);
extern void Relay_Ctrl(uint8_t relay, uint8_t Flag);
extern void Relay_Ctrl_Retry(void);

#endif

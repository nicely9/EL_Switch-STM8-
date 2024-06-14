#ifndef _IR_H
#define _IR_H

// extern uint8_t		Gu8_IR_PutCnt;
// extern uint8_t		Gu8_IR_GetCnt;


extern void IR_Init(void);
extern void IR_Process(void);
extern void irq_IR(void);

#endif

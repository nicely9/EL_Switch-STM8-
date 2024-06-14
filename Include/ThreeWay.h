#ifndef _THREEWAY_H
#define _THREEWAY_H

extern __IO uint8_t	Gu8_ThreeWay_ON_Tmr[2];
extern __IO uint8_t	Gu8_ThreeWay_OFF_Tmr[2];
extern __IO uint8_t	Gu8_ThreeWay_Toggle_Flag[2];

extern void ThreeWay_Init(void);
extern void ThreeWay_EXT_Switch_State_Process(void);
extern void ThreeWay_Tmr(void);

#endif

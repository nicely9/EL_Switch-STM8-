#ifndef _WDG_BEEP_H
#define _WDG_BEEP_H

#define WWDG_WINDOW_VALUE		127		// specifies the WWDG Window Register, range is 0 to 127(0x7F).
#define WWDG_COUNTER_INIT		127

#define RELOAD_VALUE			254

#define	BEEP_MAXQ	3
typedef struct {
    uint8_t  buff[BEEP_MAXQ];	/* data buffer */
    uint8_t           wptr;		/* write pointer */
    uint8_t           rptr;		/* read pointer */
}BEEP_QUEUE;

extern uint8_t	Gu8_Beep_Step_Down_Tmr;
extern uint8_t	Gu8_Beep_Tmr;
extern uint8_t	Gu8_Beep_Cnt;
extern uint8_t	Gu8_WDG_Boot;
extern uint8_t	Gu8_Beep_Put_Tmr;

extern void Beep_WDG_Init(void);
extern void WDG_BootModeCheck(void);
extern void WDG_Config_Set(void);
extern void Beep_Process(void);
extern void Beep(uint8_t Flag);
extern void WDG_SetCounter(void);
extern void irq_TIM2_LSIMeasurment(void);
extern void WWDG_Disable(void);

extern void Beep_Test(uint8_t data);

#endif
#ifndef __BEEP_PWM_H_
#define __BEEP_PWM_H_

// 온음표 2000ms, 2분음표 1000ms, 4분음표 500ms, 8분음표 250ms, 16분음표 125ms
// 음표에 따른 지연시간(ms) = 2000 / x분음표,	즉 2분음표 지연시간 1000(ms) = 2000 / 2

#define	NOTE_REST	0
#define	NOTE_C5		523	//523.2511	5옥타브	C(도)
#define	NOTE_Cs5	554	//554.3653			C#
#define	NOTE_D5		587	//587.3295			D(레)
#define	NOTE_Ds5	622	//622.2540			D#
#define	NOTE_E5		659	//659.2551			E(미)
#define	NOTE_F5		698	//698.4565			F(파)
#define	NOTE_Fs5	740	//739.9888			F#
#define	NOTE_G5		784	//783.9909			G(솔)
#define	NOTE_Gs5	831	//830.6094			G#
#define	NOTE_A5		880	//880.0000			A(라)
#define	NOTE_As5	932	//932.3275			A#
#define	NOTE_B5		988	//987.7666			B(시)

extern void BEEP_PWM_Init(void);

#endif
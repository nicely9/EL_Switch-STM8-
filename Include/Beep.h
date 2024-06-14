#ifndef __BEEP_PWM_H_
#define __BEEP_PWM_H_

// ����ǥ 2000ms, 2����ǥ 1000ms, 4����ǥ 500ms, 8����ǥ 250ms, 16����ǥ 125ms
// ��ǥ�� ���� �����ð�(ms) = 2000 / x����ǥ,	�� 2����ǥ �����ð� 1000(ms) = 2000 / 2

#define	NOTE_REST	0
#define	NOTE_C5		523	//523.2511	5��Ÿ��	C(��)
#define	NOTE_Cs5	554	//554.3653			C#
#define	NOTE_D5		587	//587.3295			D(��)
#define	NOTE_Ds5	622	//622.2540			D#
#define	NOTE_E5		659	//659.2551			E(��)
#define	NOTE_F5		698	//698.4565			F(��)
#define	NOTE_Fs5	740	//739.9888			F#
#define	NOTE_G5		784	//783.9909			G(��)
#define	NOTE_Gs5	831	//830.6094			G#
#define	NOTE_A5		880	//880.0000			A(��)
#define	NOTE_As5	932	//932.3275			A#
#define	NOTE_B5		988	//987.7666			B(��)

extern void BEEP_PWM_Init(void);

#endif
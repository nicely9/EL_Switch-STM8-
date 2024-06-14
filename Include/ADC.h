#ifndef _ADC_H
#define _ADC_H

typedef enum 
{
	ADC_BUFF_TEMPERATURE	= 0,	// ADC ch4
	ADC_BUFF_AC_VOLTAGE,			// ADC ch11
	ADC_BUFF_5V_VREF,				// ADC ch22		5V
	ADC_BUFF_CT_SENSOR_1,			// ADC ch24
	ADC_BUFF_CT_SENSOR_2,			// ADC ch25
	ADC_BUFF_VREFINT,				// ADC vrefint	3.3V
	
	MAX_ADC_BUFF
}_ADC_BUFF_;

//#define	ADC_AVR_CNT		360			// 60Hz 기준 360번
//#define	ADC_AVR_CNT		720			// 60Hz 기준 720
#define	ADC_AVR_CNT		2048			// 60Hz 기준 1020
#define	ADC_PRESCALER	TIM2_Prescaler_128
#define TIM2_PERIOD		(uint16_t)((double)((CLK_GetClockFreq() / (double)((double)(ADC_AVR_CNT * ADC1_BUFFER_SIZE) * (double)(0x1<<ADC_PRESCALER)) ) - 1) + 0.5)
//#define TIM2_PERIOD		(uint16_t)((double)((CLK_GetClockFreq() / (double)((double)(ADC_AVR_CNT) * (double)(0x1<<ADC_PRESCALER)) ) - 1) + 0.5)
//#define	ADC_MAX_MIN_CNT	(uint16_t)(ADC_AVR_CNT / 10)	// 36
#define	ADC_MAX_MIN_CNT	(5)	// 3

#define	MAX_ADC_CNT		(uint16_t)((uint16_t)ADC_AVR_CNT * (uint16_t)ADC1_BUFFER_SIZE)
//#define	MAX_ADC_CNT		(uint16_t)((uint16_t)ADC_AVR_CNT)


#define	ADC_COMPLETE	(uint8_t)0x01
#define	ADC_DATA_IGNORE	(uint8_t)0x02

#define ADC_BIT					4095.0		// 12bit(0xFFF)
#define ADC_VREF				3.3

#define	TEMP_ADC				4095.0		// 4096-1
#define	TEMP_B					3936.0		// 3936K 1%,	B-Constant(25-50)(K)
#define	TEMP_T0					298.15		// 절대온도,		25'C + 273.15
#define	TEMP_0C					273.15		// 섭씨0도,		273.15
#define	TEMP_PullUP_R			10.0		// 10K
#define	TEMP_Thermistor_25C_R	10.0		// 10K
#define	TEMP_BT0				1173518.4	// B * T0

#define ACV_R1					990000.0	// 990K
#define ACV_R2					3750.0		// 3.8K

#define ACV_VREF				5.0			// 5V
#define ACV_VREF_R1				20000.0		// 20K
#define ACV_VREF_R2				10000.0		// 10K
#define ACV_VREF_OUTPUT_V		((ACV_VREF_R2 / (ACV_VREF_R1 + ACV_VREF_R2)) * ACV_VREF)	// 정확한 Vref가 아니어도 피크-피크 값으로 계산하기 때문에 큰 의미는 없음

#define CT_R1					5000.0		// 5K
#define CT_R2					10000.0		// 10K
#define CT_OUTPUT_V_DIV			(CT_R2 / (CT_R1 + CT_R2))
#define CT_0A_OUTPUT_V			2.5			// 0A[2.5V], 10A[3.5], 20A[4.5]

extern double		Gdbl_VREF_3_3V;
extern double		Gdbl_VREF_2_5V;
extern double		Gdbl_VREF_5_0V;
extern double		Gdbl_ACV;
extern double		Gdbl_ACCurrent_1;
extern double		Gdbl_ACCurrent_2;
extern double		Gdbl_AC_1_Watt;
extern double		Gdbl_AC_2_Watt;
extern double		Gdbl_Temperature;

extern uint8_t	Gu8_AC_Warring_Tmr;
extern uint8_t	ADC1_BUFFER_SIZE;
extern __IO uint8_t	G_DMA_Flag;
extern uint16_t	G_ADC_DMA_Buffer[MAX_ADC_BUFF];
extern uint16_t	Gu16_ADC_DMA_Cnt;
extern uint32_t	G_ADC_Sum_Buffer[MAX_ADC_BUFF];
extern double Gdbl_ADC_Avr_Buffer[MAX_ADC_BUFF];

extern uint16_t Gu16_MAX_ACV_Adc[ADC_MAX_MIN_CNT];
extern uint16_t Gu16_MAX_ACV_Adc_tmp[ADC_MAX_MIN_CNT];
extern uint16_t Gu16_MIN_ACV_Adc[ADC_MAX_MIN_CNT];
extern uint16_t Gu16_MIN_ACV_Adc_tmp[ADC_MAX_MIN_CNT];

extern uint16_t Gu16_MAX_CT1_Adc[ADC_MAX_MIN_CNT];
extern uint16_t Gu16_MAX_CT1_Adc_tmp[ADC_MAX_MIN_CNT];
extern uint16_t Gu16_MIN_CT1_Adc[ADC_MAX_MIN_CNT];
extern uint16_t Gu16_MIN_CT1_Adc_tmp[ADC_MAX_MIN_CNT];

extern uint16_t Gu16_MAX_CT2_Adc[ADC_MAX_MIN_CNT];
extern uint16_t Gu16_MAX_CT2_Adc_tmp[ADC_MAX_MIN_CNT];
extern uint16_t Gu16_MIN_CT2_Adc[ADC_MAX_MIN_CNT];
extern uint16_t Gu16_MIN_CT2_Adc_tmp[ADC_MAX_MIN_CNT];

extern void ADC_Sensor_Init(void);
extern void irq_ADC_DMA(void);
extern void ADC_Process(void);


#endif //_ADC_H

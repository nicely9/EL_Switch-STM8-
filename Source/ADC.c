/************************************************************************************
	Project		: 전자식스위치
	File Name	: ADC.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "adc.h"
#include "Debug.h"
#include "WDGnBeep.h"
#include "relay.h"

#ifdef	_ADC_
//-------------------------------------------------------------------------------------------------------------------------

#define ADC1_DR_ADDRESS			((uint16_t)(ADC1_BASE + 0x04))			// ADC1 data register high
//#define ADC1_BUFFER_SIZE		((uint8_t) MAX_ADC_BUFF)			// Data count
#define ADC1_BUFFER_ADDRESS		((uint16_t)(&G_ADC_DMA_Buffer))

__IO uint8_t	G_DMA_Flag;
uint16_t G_ADC_DMA_Buffer[MAX_ADC_BUFF];
uint16_t Gu16_ADC_DMA_Cnt;
uint32_t G_ADC_Sum_Buffer[MAX_ADC_BUFF];
double Gdbl_ADC_Avr_Buffer[MAX_ADC_BUFF];

uint8_t	ADC1_BUFFER_SIZE = 0;

uint8_t	Gu8_AC_Warring_Tmr	= 0;

uint16_t Gu16_MAX_ACV_Adc[ADC_MAX_MIN_CNT];
uint16_t Gu16_MAX_ACV_Adc_tmp[ADC_MAX_MIN_CNT];
uint16_t Gu16_MIN_ACV_Adc[ADC_MAX_MIN_CNT];
uint16_t Gu16_MIN_ACV_Adc_tmp[ADC_MAX_MIN_CNT];

uint16_t Gu16_MAX_CT1_Adc[ADC_MAX_MIN_CNT];
uint16_t Gu16_MAX_CT1_Adc_tmp[ADC_MAX_MIN_CNT];
uint16_t Gu16_MIN_CT1_Adc[ADC_MAX_MIN_CNT];
uint16_t Gu16_MIN_CT1_Adc_tmp[ADC_MAX_MIN_CNT];

uint16_t Gu16_MAX_CT2_Adc[ADC_MAX_MIN_CNT];
uint16_t Gu16_MAX_CT2_Adc_tmp[ADC_MAX_MIN_CNT];
uint16_t Gu16_MIN_CT2_Adc[ADC_MAX_MIN_CNT];
uint16_t Gu16_MIN_CT2_Adc_tmp[ADC_MAX_MIN_CNT];

//double		Gdbl_AC_Ratio		= 0;
double		Gdbl_CT_Ratio		= 0;

double		Gdbl_VREF_3_3V		= 3.3;
double		Gdbl_VREF_2_5V		= 2.5;
double		Gdbl_VREF_5_0V		= 5.0;
double		Gdbl_ACV			= 0;	// AC Voltage		AC220V 피크전압 = 220 * 1.414 = 311.08Vpp
double		Gdbl_ACCurrent_1	= 0;	// AC Current 1
double		Gdbl_ACCurrent_2	= 0;	// AC Current 2
double		Gdbl_AC_1_Watt		= 0;
double		Gdbl_AC_2_Watt		= 0;
double		Gdbl_Temperature	= 0;	// Temperature

double		Gdbl_ACV_old	= 0;
double		Gdbl_ACV_tmp	= 0;
double		Gdbl_CT1_tmp[ADC_MAX_MIN_CNT];
double		Gdbl_CT2_tmp[ADC_MAX_MIN_CNT];

double	Gdbl_ADC_Ratio;
double	Gdbl_Vpp;
double	Gdbl_Vp;
double	Gdbl_ACV_INPUT_DIV;
double	Gdbl_ACV_Vref_DIV;
double	Gdbl_ACV_Vref;
double	Gdbl_CT_OUTPUT_DIV;
double	Gdbl_CT_Vref;
//-------------------------------------------------------------------------------------------------------------------------
void ADC_Sensor_Init(void)		// AC(ch4), Temp(ch23), CT1(ch24), CT2(ch25)
{
	int i;
	
	G_DMA_Flag			= 0;
	Gu16_ADC_DMA_Cnt	= 0;
	
	Gdbl_VREF_3_3V		= 3.3;
	Gdbl_VREF_2_5V		= 2.5;
	Gdbl_VREF_5_0V		= 5.0;
	Gdbl_ADC_Ratio		= ADC_VREF / ADC_BIT;							// 0.0008058608 = 3.3 / 4095
	Gdbl_Vpp			= 0.3535533905932738;							// 1 / 2√2
	Gdbl_Vp				= 0.7071067811865475;							// 1 / √2
	Gdbl_ACV_INPUT_DIV	= ACV_R2 / (ACV_R1 + ACV_R2);					// 0.0.00502512562814 = 5KΩ / (990KΩ + 5KΩ)
	Gdbl_ACV_Vref_DIV	= ACV_VREF_R2 / (ACV_VREF_R1 + ACV_VREF_R2);	// 0.3333333333333333 = (10KΩ / (20KΩ + 10KΩ)
	Gdbl_ACV_Vref		= Gdbl_ACV_Vref_DIV * Gdbl_VREF_5_0V;			// 1.66667 = (10KΩ / (20KΩ + 10KΩ) * 5.0)	정확한 Vref가 아니어도 피크-피크 값으로 계산하기 때문에 큰 의미는 없음
	Gdbl_CT_OUTPUT_DIV	= CT_R2 / (CT_R1 + CT_R2);						// 0.6666666666666667 = 10KΩ / (5KΩ + 10K)
	Gdbl_CT_Vref		= Gdbl_CT_OUTPUT_DIV * Gdbl_VREF_2_5V;
	
	/*
	printf("Gdbl_ADC_Ratio      %f\n", Gdbl_ADC_Ratio);
	printf("Gdbl_Vpp            %f\n", Gdbl_Vpp);
	printf("Gdbl_Vp             %f\n", Gdbl_Vp);
	printf("Gdbl_ACV_INPUT_DIV  %f\n", Gdbl_ACV_INPUT_DIV);
	printf("Gdbl_CT_OUTPUT_DIV  %f\n", Gdbl_CT_OUTPUT_DIV);
	printf("Gdbl_ACV_Vref       %f\n", Gdbl_ACV_Vref);
	*/
	
	for(i=0;i<MAX_ADC_BUFF;i++)
	{
		G_ADC_DMA_Buffer[i]		= 0;
		G_ADC_Sum_Buffer[i]		= 0;
		Gdbl_ADC_Avr_Buffer[i]	= 0;
	}
	for(i=0;i<ADC_MAX_MIN_CNT;i++)
	{
		Gdbl_CT1_tmp[i]			= 0;
		Gdbl_CT2_tmp[i]			= 0;
		
		Gu16_MAX_ACV_Adc[i]		= 0;
		Gu16_MAX_ACV_Adc_tmp[i]	= 0;
		Gu16_MIN_ACV_Adc[i]		= 0;
		Gu16_MIN_ACV_Adc_tmp[i]	= 0xFFFF;
		
		Gu16_MAX_CT1_Adc[i]		= 0;
		Gu16_MAX_CT1_Adc_tmp[i]	= 0;
		Gu16_MIN_CT1_Adc[i]		= 0;
		Gu16_MIN_CT1_Adc_tmp[i]	= 0xFFFF;
		
		Gu16_MAX_CT2_Adc[i]		= 0;
		Gu16_MAX_CT2_Adc_tmp[i]	= 0;
		Gu16_MIN_CT2_Adc[i]		= 0;
		Gu16_MIN_CT2_Adc_tmp[i]	= 0xFFFF;
	}
	
	ADC1_BUFFER_SIZE = 0;
	ADC1_BUFFER_SIZE	+= pG_Config->Enable_Flag.ADC_Temperature;
	ADC1_BUFFER_SIZE	+= pG_Config->Enable_Flag.ADC_AC_Voltage;
	ADC1_BUFFER_SIZE	+= pG_Config->Enable_Flag.ADC_5_0V_Ref;
	ADC1_BUFFER_SIZE	+= pG_Config->Enable_Flag.ADC_CT_Sensor1;
	ADC1_BUFFER_SIZE	+= pG_Config->Enable_Flag.ADC_CT_Sensor2;
	ADC1_BUFFER_SIZE	+= pG_Config->Enable_Flag.ADC_3_3V_Ref;
	
	if(ADC1_BUFFER_SIZE)
	{
		CLK_PeripheralClockConfig(CLK_Peripheral_ADC1, ENABLE);
		ADC_Init(ADC1, ADC_ConversionMode_Single, ADC_Resolution_12Bit, ADC_Prescaler_1);		// 1개의 신호가 발생할 때마다 DMA에 등록된 버퍼에 전부 등록되면 인터럽트 발생(등록된 모든 채널을 읽고나서 인터럽트 발생)
		//ADC_Init(ADC1, ADC_ConversionMode_Continuous, ADC_Resolution_12Bit, ADC_Prescaler_1);
		
		if(pG_Config->Enable_Flag.ADC_Temperature || pG_Config->Enable_Flag.ADC_AC_Voltage || pG_Config->Enable_Flag.ADC_5_0V_Ref || pG_Config->Enable_Flag.ADC_3_3V_Ref)
		{
			ADC_SamplingTimeConfig(ADC1, ADC_Group_SlowChannels, ADC_SamplingTime_384Cycles);	// ch4, ch23
		}
		if(pG_Config->Enable_Flag.ADC_CT_Sensor1 || pG_Config->Enable_Flag.ADC_CT_Sensor2)
		{
			// 일단 천천히 측정, 고속측정은 다소 오차가...
			//ADC_SamplingTimeConfig(ADC1, ADC_Group_FastChannels, ADC_SamplingTime_384Cycles);	// ch24, ch25
		}
		
		ADC_ITConfig(ADC1, ADC_IT_OVER | ADC_IT_EOC, ENABLE);	// Over Run / End of Conversion Interrupt ENABLE
		ADC_Cmd(ADC1, ENABLE);
		
		if(pG_Config->Enable_Flag.ADC_Temperature)
		{
			ADC_ChannelCmd(ADC1, TEMPERATURE_ADC_CH, ENABLE);	// connected to TEMPERATURE_SENSOR
		}
		if(pG_Config->Enable_Flag.ADC_AC_Voltage)
		{
			ADC_ChannelCmd(ADC1, AC_VOLTAGE_ADC_CH, ENABLE);	// connected to AC_VOLTAGE
		}
		if(pG_Config->Enable_Flag.ADC_5_0V_Ref)
		{
			ADC_ChannelCmd(ADC1, VREFINT_5_0V_CH, ENABLE);		// connected to VREFINT_5_0V
		}
		if(pG_Config->Enable_Flag.ADC_CT_Sensor1)
		{
			ADC_ChannelCmd(ADC1, CT_SENSOR_1_ADC_CH, ENABLE);	// connected to CT_SENSOR_1
		}
		if(pG_Config->Enable_Flag.ADC_CT_Sensor2)
		{
			ADC_ChannelCmd(ADC1, CT_SENSOR_2_ADC_CH, ENABLE);	// connected to CT_SENSOR_2
		}
		//-------------------------------------------------------------------------------------------------------------------------
		if(pG_Config->Enable_Flag.ADC_3_3V_Ref)
		{
			// VREFINT
			ADC_ChannelCmd(ADC1, VREFINT_ADC_CH, ENABLE);
			ADC_VrefintCmd(ENABLE);
		}
		//-------------------------------------------------------------------------------------------------------------------------
		ADC_DMACmd(ADC1, ENABLE);		// Enable ADC1 DMA requests
		//ADC_SoftwareStartConv(ADC1);	// Start ADC1 Conversion using Software trigger
		ADC_ExternalTrigConfig(ADC1, ADC_ExtEventSelection_Trigger3, ADC_ExtTRGSensitivity_Rising);	// Start ADC1 Conversion using TIM TRGO
		//-------------------------------------------------------------------------------------------------------------------------
		CLK_PeripheralClockConfig(CLK_Peripheral_DMA1, ENABLE);
		//DMA_DeInit(DMA1_Channel0);	// ADC1			TEMP, ACV, CT1, CT2
		SYSCFG_REMAPDMAChannelConfig(REMAP_DMA1Channel_ADC1ToChannel0);		// Connect ADC to DMA channel 0
		DMA_Init(DMA1_Channel0, ADC1_BUFFER_ADDRESS, ADC1_DR_ADDRESS, ADC1_BUFFER_SIZE,
				DMA_DIR_PeripheralToMemory,
				DMA_Mode_Circular,					// DMA circular buffer mode
				DMA_MemoryIncMode_Inc,				// DMA memory incremented mode is incremental
				DMA_Priority_High,
				DMA_MemoryDataSize_HalfWord );
		DMA_Cmd(DMA1_Channel0, ENABLE);						// DMA Channel0 enable
		DMA_ITConfig(DMA1_Channel0, DMA_ITx_TC, ENABLE);	// Enable DMA1 channel0 Transfer complete interrupt
		DMA_GlobalCmd(ENABLE);						// DMA enable
		//-------------------------------------------------------------------------------------------------------------------------
		/* TIM2 configuration:
		- TIM2CLK is set to 16 MHz, the TIM4 Prescaler is equal to 128 so the TIM1 counter clock used is 16 MHz / 128 = 125,000 Hz
		- With 125,000 Hz we can generate time base:
		  TIM2 Channel1 output frequency = TIM2CLK / (TIM2_PERIOD + 1) = 125,000 / (346+1)) = 360.2305 Hz, 2.77ms
		  TIM2 Channel1 output frequency = TIM2CLK / (TIM2_PERIOD + 1) = 125,000 / (693+1)) = 180.1153 Hz, 5.55ms */
		CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);
		TIM2_TimeBaseInit(ADC_PRESCALER, TIM2_CounterMode_Up, TIM2_PERIOD);
		TIM2_ClearFlag(TIM2_FLAG_Update);
		TIM2_SelectOutputTrigger(TIM2_TRGOSource_Update);	// ADC1 TRGO
		//TIM2_ITConfig(TIM2_IT_Update, ENABLE);	// Enable update interrupt
		TIM2_Cmd(ENABLE);
		
		//ITC_SetSoftwarePriority(DMA1_CHANNEL0_1_IRQn, ITC_PriorityLevel_2);
		//ITC_SetSoftwarePriority(TIM4_UPD_OVF_TRG_IRQn, ITC_PriorityLevel_2);
		
		//printf("ADC Init\n");
		printf("ADC Init(%fhz %fms %dperiod)\n", (double)(ADC_AVR_CNT * ADC1_BUFFER_SIZE), 1000/(double)(ADC_AVR_CNT * ADC1_BUFFER_SIZE), TIM2_PERIOD);
		
		printf("ADC BUFFER %d\n", (uint16_t)ADC1_BUFFER_SIZE);
	}
	
}

void ADC_Process(void)
{
	uint8_t	i;
	double Temperature_Resistor;
	double max = 0, min = 0, tmp;
	
	if(G_DMA_Flag & ADC_COMPLETE)				// 데이터 습득 후 평균값이 완료되었으면
	{
		G_DMA_Flag	&= (uint8_t)~ADC_COMPLETE;
		if(G_DMA_Flag & ADC_DATA_IGNORE)
		{
			G_DMA_Flag	&= (uint8_t)~ADC_DATA_IGNORE;
			return;
		}
		Gdbl_VREF_3_3V	= ((ADC_BIT) / Gdbl_ADC_Avr_Buffer[ADC_BUFF_VREFINT]) * 1.224;		// Vrefint Voltage = 1.224
		Gdbl_ADC_Ratio	= Gdbl_VREF_3_3V / ADC_BIT;							// 0.0008058608 = 3.3 / 4095
		
		Gdbl_VREF_2_5V	= Gdbl_ADC_Avr_Buffer[ADC_BUFF_5V_VREF] * Gdbl_ADC_Ratio;
		Gdbl_VREF_5_0V	= Gdbl_VREF_2_5V * 2.0;
		
		Gdbl_ACV_Vref	= Gdbl_ACV_Vref_DIV * Gdbl_VREF_5_0V;
		
		Gdbl_CT_Vref	= Gdbl_CT_OUTPUT_DIV * Gdbl_VREF_2_5V;
		//-----------------------------------------------------------------------------------------------------------------------
#if 0
		max	= (double)((Gu16_MAX_ACV_Adc[1] + Gu16_MAX_ACV_Adc[0]) / 2.0);
		min	= (double)((Gu16_MIN_ACV_Adc[1] + Gu16_MIN_ACV_Adc[0]) / 2.0);
#else
		max	= (double)(Gu16_MAX_ACV_Adc[0]);
		min	= (double)(Gu16_MIN_ACV_Adc[0]);
#endif
		
		if(G_Trace)
		{
			/*
			for(i=0;i<ADC_MAX_MIN_CNT;i++)
			{
				printf("%d, %d\n", Gu16_MAX_ACV_Adc[i], Gu16_MIN_ACV_Adc[i]);
			}
			*/
			WDG_SetCounter();
			printf("Gdbl_ADC_Ratio      %f\n", Gdbl_ADC_Ratio);
			printf("Gdbl_ACV_Vref       %f\n", Gdbl_ACV_Vref);
			printf("Gdbl_ACV_INPUT_DIV  %f\n", Gdbl_ACV_INPUT_DIV);
			
			printf("ACV  ADC MAX[%f], inV[%f],   ADC MIN[%f], inV[%f]\n", max, max*Gdbl_ADC_Ratio, min, min*Gdbl_ADC_Ratio);	// ACV RMS
		}
		max	= ((max * Gdbl_ADC_Ratio) - Gdbl_ACV_Vref)  / Gdbl_ACV_INPUT_DIV;
		min	= ((min * Gdbl_ADC_Ratio) - Gdbl_ACV_Vref)  / Gdbl_ACV_INPUT_DIV;
		
		Gdbl_ACV_tmp	= ((fabs(max) + fabs(min)) * Gdbl_Vpp) + pG_Config->ACV_Offset;		// ACV rms
		if(fabs(Gdbl_ACV_tmp - Gdbl_ACV_old) >= 2.0)		// 2V 이상 차이가 발생하면
		{
			Gdbl_ACV	= Gdbl_ACV_tmp;
			//printf("-------------------\n");
		}
		else
		{
			Gdbl_ACV	= (Gdbl_ACV + Gdbl_ACV_tmp) / 2.0;	// 2V 미만으로 차이가 발행하면 평균 누적
		}
		Gdbl_ACV_old	= Gdbl_ACV_tmp;
		
		if(G_Trace)
		{
			printf("ACV  MAX[%f]v, MIN[%f]v\t", max, min);	// ACV RMS
			printf("ACV  [%f]Vrms\n\n", Gdbl_ACV);	// ACV RMS
		}
		//-----------------------------------------------------------------------------------------------------------------------
#if 0
		max	= (double)((Gu16_MAX_CT1_Adc[1] + Gu16_MAX_CT1_Adc[0]) / 2.0);
		min	= (double)((Gu16_MIN_CT1_Adc[1] + Gu16_MIN_CT1_Adc[0]) / 2.0);
		/*
		max	= 0;
		for(i=0;i<3;i++)
		{
			if(Gu16_MAX_CT1_Adc[i] != 0)
			{
				max	+= (double)Gu16_MAX_CT1_Adc[i];
			}
			else
			{
				break;
			}
		}
		max	= max / i;
		min	= 0;
		for(i=0;i<3;i++)
		{
			if(Gu16_MIN_CT1_Adc[i] != 0xffff)
			{
				min	+= (double)Gu16_MIN_CT1_Adc[i];
			}
			else
			{
				break;
			}
		}
		min	= min / i;
		*/
#else
		max	= (double)(Gu16_MAX_CT1_Adc[0]);
		min	= (double)(Gu16_MIN_CT1_Adc[0]);
#endif
		
		if(G_Trace)
		{
			
			for(i=0;i<ADC_MAX_MIN_CNT;i++)
			{
				printf("%d, %d\n", Gu16_MAX_CT1_Adc[i], Gu16_MIN_CT1_Adc[i]);
			}
			
			WDG_SetCounter();
			
			printf("CT1  ADC MAX[%f], inV[%f],   ADC MIN[%f], inV[%f]\n", max, max*Gdbl_ADC_Ratio, min, min*Gdbl_ADC_Ratio);	// CT1 RMS
		}
		max	= (max * Gdbl_ADC_Ratio) / Gdbl_CT_OUTPUT_DIV;
		min	= (min * Gdbl_ADC_Ratio) / Gdbl_CT_OUTPUT_DIV;
		
		for(i=1;i<ADC_MAX_MIN_CNT;i++)
		{
			Gdbl_CT1_tmp[i]	= Gdbl_CT1_tmp[i-1];
		}
		Gdbl_CT1_tmp[0]	= ((fabs(fabs(max) - fabs(min)) * Gdbl_Vpp) / 0.1) + pG_Config->CT1_Offset;		// CT1 rms
		tmp	= 0;
		for(i=0;i<ADC_MAX_MIN_CNT;i++)
		{
			tmp	+= Gdbl_CT1_tmp[i];
		}
		Gdbl_ACCurrent_1	= tmp / (double)i;
		Gdbl_AC_1_Watt		= Gdbl_ACV * Gdbl_ACCurrent_1;
		
		if(G_Trace)
		{
			printf("CT1  MAX[%f]v, MIN[%f]v\t", max, min);
			printf("[%f]Arms\t", Gdbl_ACCurrent_1);
			printf("[%f]W\n\n", Gdbl_AC_1_Watt);
		}
		//-----------------------------------------------------------------------------------------------------------------------
#if 0
		max	= (double)((Gu16_MAX_CT2_Adc[1] + Gu16_MAX_CT2_Adc[0]) / 2.0);
		min	= (double)((Gu16_MIN_CT2_Adc[1] + Gu16_MIN_CT2_Adc[0]) / 2.0);
#else
		max	= (double)(Gu16_MAX_CT2_Adc[0]);
		min	= (double)(Gu16_MIN_CT2_Adc[0]);
#endif
		
		if(G_Trace)
		{
			/*
			for(i=0;i<ADC_MAX_MIN_CNT;i++)
			{
				printf("%d, %d\n", Gu16_MAX_CT2_Adc[i], Gu16_MIN_CT2_Adc[i]);
			}
			*/
			WDG_SetCounter();
			
			printf("CT2  ADC MAX[%f], inV[%f],   ADC MIN[%f], inV[%f]\n", max, max*Gdbl_ADC_Ratio, min, min*Gdbl_ADC_Ratio);	// CT2 RMS
		}
		max	= (max * Gdbl_ADC_Ratio) / Gdbl_CT_OUTPUT_DIV;
		min	= (min * Gdbl_ADC_Ratio) / Gdbl_CT_OUTPUT_DIV;
		
		for(i=1;i<ADC_MAX_MIN_CNT;i++)
		{
			Gdbl_CT2_tmp[i]	= Gdbl_CT2_tmp[i-1];
		}
		Gdbl_CT2_tmp[0]	= ((fabs(fabs(max) - fabs(min)) * Gdbl_Vpp) / 0.1) + pG_Config->CT2_Offset;		// CT2 rms
		tmp	= 0;
		for(i=0;i<ADC_MAX_MIN_CNT;i++)
		{
			tmp	+= Gdbl_CT2_tmp[i];
		}
		Gdbl_ACCurrent_2	= tmp / (double)i;
		Gdbl_AC_2_Watt		= Gdbl_ACV * Gdbl_ACCurrent_2;
		
		if(G_Trace)
		{
			printf("CT2  MAX[%f]v, MIN[%f]v\t", max, min);
			printf("[%f]Arms\t", Gdbl_ACCurrent_2);
			printf("[%f]W\n\n", Gdbl_AC_2_Watt);
		}
		//-----------------------------------------------------------------------------------------------------------------------
		
		//Gdbl_ACCurrent_1	= (((Gdbl_ADC_Avr_Buffer[ADC_BUFF_CT_SENSOR_1] * Gdbl_ADC_Ratio) / Gdbl_CT_OUTPUT_DIV) - Gdbl_VREF_2_5V) + pG_Config->CT1_Offset;
		//Gdbl_ACCurrent_2	= (((Gdbl_ADC_Avr_Buffer[ADC_BUFF_CT_SENSOR_2] * Gdbl_ADC_Ratio) / Gdbl_CT_OUTPUT_DIV) - Gdbl_VREF_2_5V) + pG_Config->CT2_Offset;
		
		if(Gu8_ADC_Offset_Cnt)	Gu8_ADC_Offset_Cnt--;
		if(Gu8_ADC_Offset_Cnt == 1)
		{
			printf("\n\nGdbl_ACCurrent_1 %f\n", Gdbl_ACCurrent_1);
			printf("Gdbl_ACCurrent_2 %f\n\n", Gdbl_ACCurrent_2);
			
			pG_Config->CT1_Offset	= 0.0 - Gdbl_ACCurrent_1;
			pG_Config->CT2_Offset	= 0.0 - Gdbl_ACCurrent_2;
			
			printf("\n\nCT1_Offset %f\n", pG_Config->CT1_Offset);
			printf("CT1_Offset %f\n\n", pG_Config->CT2_Offset);
			
			if(Gu8_ADC_Offset_1_Flag != 0xff && Gu8_ADC_Offset_1_Flag)	Relay_Ctrl(CONTROL_BIT_RELAY_LATCH_1, ON);
			if(Gu8_ADC_Offset_2_Flag != 0xff && Gu8_ADC_Offset_2_Flag)	Relay_Ctrl(CONTROL_BIT_RELAY_LATCH_2, ON);
			
			Store_CurrentConfig();
		}
		
		//Temperature_Resistor	= (TEMP_PullUP_R * (TEMP_ADC - Gdbl_ADC_Avr_Buffer[ADC_BUFF_TEMPERATURE])) / Gdbl_ADC_Avr_Buffer[ADC_BUFF_TEMPERATURE];	// pull DN
		Temperature_Resistor	= (TEMP_PullUP_R / (TEMP_ADC - Gdbl_ADC_Avr_Buffer[ADC_BUFF_TEMPERATURE])) * Gdbl_ADC_Avr_Buffer[ADC_BUFF_TEMPERATURE];	// pull UP
		Gdbl_Temperature	= (TEMP_BT0 / ((log(Temperature_Resistor / TEMP_Thermistor_25C_R) * TEMP_T0) + TEMP_B)) - TEMP_0C;
		
		if(G_Trace)
		{
			WDG_SetCounter();
			printf("5V   ADC[%f], inV[%f]\n", Gdbl_ADC_Avr_Buffer[ADC_BUFF_5V_VREF], Gdbl_ADC_Avr_Buffer[ADC_BUFF_5V_VREF]*0.0008058608);
			printf("TEMP ADC[%f], inV[%f], %f'C\n", Gdbl_ADC_Avr_Buffer[ADC_BUFF_TEMPERATURE], Gdbl_ADC_Avr_Buffer[ADC_BUFF_TEMPERATURE]*0.0008058608, Gdbl_Temperature);
			//printf("CT_1 ADC[%f], inV[%f], %fA, %fW\n", Gdbl_ADC_Avr_Buffer[ADC_BUFF_CT_SENSOR_1], Gdbl_ADC_Avr_Buffer[ADC_BUFF_CT_SENSOR_1]*0.0008058608, Gdbl_ACCurrent_1, Gdbl_AC_1_Watt);
			//printf("CT_2 ADC[%f], inV[%f], %fA, %fW\n", Gdbl_ADC_Avr_Buffer[ADC_BUFF_CT_SENSOR_2], Gdbl_ADC_Avr_Buffer[ADC_BUFF_CT_SENSOR_2]*0.0008058608, Gdbl_ACCurrent_2, Gdbl_AC_2_Watt);
			
			printf("VREFINT [%f], [%f], [%f]\n\n", Gdbl_VREF_2_5V, Gdbl_VREF_3_3V, Gdbl_VREF_5_0V);
		}
		
		if(Gdbl_ACV < 50.0)		// 전원코드를 반대로 꼽았을 때 전원측정이 안되므로 경고음 발생
		{
			if(Gu8_AC_Warring_Tmr == 0)
			{
				Gu8_AC_Warring_Tmr	= 20;		// 2s
				Beep(BEEP_AC_WARRING);
			}
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------
#endif

/************************************************************************************
	Project		: 전자식스위치
	File Name	: ZeroCrossing.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "el_switch.h"
#include "ZeroCrossing.h"

// 제로크로스 기능이라는 것은 교류부하전원의 제로볼트 부근에서 ON함에 따라, 급격한 부하전류의 시동 시에 발생하는 노이즈를 억제하는 기능
// 노이즈에는 전원라인에 직접 영향을 주는 것과 공간에 방사되는 것이 있는데, 이 양쪽에 모두 제로크로스 기능이 효과가 있다
// 또한, 램프의 ON시 등 매우 큰 돌입전류가 흐르지만, 제로크로스 기능에 의해 부하전류가 반드시 제로부근에서 흘러나오므로, 돌입전류를 억제할 수 있다
// 그러나, 제로볼트 부근에서 ON하는 것이 이상적이지만, 회로 구성상의 제약에 따라 0V±20V의 범위 내에서 동작하는 것을 제로크로스 전압이라고 한다

__IO uint8_t	Gu8_ZeroCrossing_Tmr		= 0;
__IO uint8_t	Gu8_ZeroCrossing_Clr_Tmr	= 100;	// 1s
uint8_t Gu8_ZeroCrossing_Err_Flag = 0;
//-------------------------------------------------------------------------------------------------------------------------
void ZeroCrossing_Init(void)
{
	//GPIO_Init(ZERO_CROSSING_PORT, ZERO_CROSSING_PIN, GPIO_Mode_In_PU_IT);					// Input pull-up, external interrupt
	GPIO_Init(ZERO_CROSSING_PORT, ZERO_CROSSING_PIN, GPIO_Mode_In_FL_IT);					// Input floating, external interrupt
    // EXTI_SetPinSensitivity((EXTI_Pin_TypeDef)ZERO_CROSSING_EXTI, EXTI_Trigger_Falling);		// EXTI Interrupt
    EXTI_SetPinSensitivity((EXTI_Pin_TypeDef)ZERO_CROSSING_EXTI, EXTI_Trigger_Rising);		// EXTI Interrupt		Rising 파형이 좀 더 정확함(ADC 회로...)
    
    printf("ZC Init\n");
}

void irq_ZeroCrossing(void)		// EXTI_Pin_7
{
	// if(GPIO_ReadInputDataBit(ZERO_CROSSING_PORT, (GPIO_Pin_TypeDef)ZERO_CROSSING_PIN))	// GPIO Read
	{
		EXTI_ClearITPendingBit(EXTI_IT_Pin7);
		//Gu8_ZeroCrossing_Tmr	= 0;	// ZeroCrossing은 초당 60회 발생(약 16.667ms 간격)
		//Gu8_ZeroCrossing_Tmr	= 35;	// ZeroCrossing은 초당 60회 발생(약 16.667ms 간격)
		//Gu8_ZeroCrossing_Tmr	= 20;	// ZeroCrossing은 초당 60회 발생(약 16.667ms 간격)
		Gu8_ZeroCrossing_Tmr	= 18;	// ZeroCrossing은 초당 60회 발생(약 16.667ms 간격)
		GET_Event();
	}
}

void BlackOut_Process(void)
{
	static __IO uint8_t	zerocrossing_err	= 0;
	
	//printf("%d\n", (uint16_t)Gu8_ZeroCrossing_Tmr);
	
	if(Gu8_ZeroCrossing_Tmr	== 0)		// 18ms 경과하면
	{
		if(zerocrossing_err == 0)
		{
#ifndef	_STATE_DIFF_SAVE__
			Store_CurrentState();		// 약 3회이상 시그널이 없으면 현재 상태값 저장
			
			//printf("\n\n\nBlackOut : Store Currnet State\n\n\n");
			// printf("\n\nStore Currnet State\n\n\n");
			printf("\n\n\nBlackOut\r\n");
#endif
			Gu8_ZeroCrossing_Clr_Tmr	= 50;		// 500ms
			zerocrossing_err	= 1;
			Gu8_ZeroCrossing_Err_Flag = 0;
		}
	}
	else if(Gu8_ZeroCrossing_Clr_Tmr == 0)
	{
		zerocrossing_err	= 0;
		Gu8_ZeroCrossing_Err_Flag = 0;	//wsm 231208
	}
	
	if(zerocrossing_err)
	{
		GET_Event();		// ZC가 발생하지 않을 때에는 ZC와 관계없이 제어
		Gu8_ZeroCrossing_Err_Flag = 1;
	}
}
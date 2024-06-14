/************************************************************************************
	Project		: ���ڽĽ���ġ
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

// ����ũ�ν� ����̶�� ���� �������������� ���κ�Ʈ �αٿ��� ON�Կ� ����, �ް��� ���������� �õ� �ÿ� �߻��ϴ� ����� �����ϴ� ���
// ������� �������ο� ���� ������ �ִ� �Ͱ� ������ ���Ǵ� ���� �ִµ�, �� ���ʿ� ��� ����ũ�ν� ����� ȿ���� �ִ�
// ����, ������ ON�� �� �ſ� ū ���������� �帣����, ����ũ�ν� ��ɿ� ���� ���������� �ݵ�� ���καٿ��� �귯�����Ƿ�, ���������� ������ �� �ִ�
// �׷���, ���κ�Ʈ �αٿ��� ON�ϴ� ���� �̻���������, ȸ�� �������� ���࿡ ���� 0V��20V�� ���� ������ �����ϴ� ���� ����ũ�ν� �����̶�� �Ѵ�

__IO uint8_t	Gu8_ZeroCrossing_Tmr		= 0;
__IO uint8_t	Gu8_ZeroCrossing_Clr_Tmr	= 100;	// 1s
uint8_t Gu8_ZeroCrossing_Err_Flag = 0;
//-------------------------------------------------------------------------------------------------------------------------
void ZeroCrossing_Init(void)
{
	//GPIO_Init(ZERO_CROSSING_PORT, ZERO_CROSSING_PIN, GPIO_Mode_In_PU_IT);					// Input pull-up, external interrupt
	GPIO_Init(ZERO_CROSSING_PORT, ZERO_CROSSING_PIN, GPIO_Mode_In_FL_IT);					// Input floating, external interrupt
    // EXTI_SetPinSensitivity((EXTI_Pin_TypeDef)ZERO_CROSSING_EXTI, EXTI_Trigger_Falling);		// EXTI Interrupt
    EXTI_SetPinSensitivity((EXTI_Pin_TypeDef)ZERO_CROSSING_EXTI, EXTI_Trigger_Rising);		// EXTI Interrupt		Rising ������ �� �� ��Ȯ��(ADC ȸ��...)
    
    printf("ZC Init\n");
}

void irq_ZeroCrossing(void)		// EXTI_Pin_7
{
	// if(GPIO_ReadInputDataBit(ZERO_CROSSING_PORT, (GPIO_Pin_TypeDef)ZERO_CROSSING_PIN))	// GPIO Read
	{
		EXTI_ClearITPendingBit(EXTI_IT_Pin7);
		//Gu8_ZeroCrossing_Tmr	= 0;	// ZeroCrossing�� �ʴ� 60ȸ �߻�(�� 16.667ms ����)
		//Gu8_ZeroCrossing_Tmr	= 35;	// ZeroCrossing�� �ʴ� 60ȸ �߻�(�� 16.667ms ����)
		//Gu8_ZeroCrossing_Tmr	= 20;	// ZeroCrossing�� �ʴ� 60ȸ �߻�(�� 16.667ms ����)
		Gu8_ZeroCrossing_Tmr	= 18;	// ZeroCrossing�� �ʴ� 60ȸ �߻�(�� 16.667ms ����)
		GET_Event();
	}
}

void BlackOut_Process(void)
{
	static __IO uint8_t	zerocrossing_err	= 0;
	
	//printf("%d\n", (uint16_t)Gu8_ZeroCrossing_Tmr);
	
	if(Gu8_ZeroCrossing_Tmr	== 0)		// 18ms ����ϸ�
	{
		if(zerocrossing_err == 0)
		{
#ifndef	_STATE_DIFF_SAVE__
			Store_CurrentState();		// �� 3ȸ�̻� �ñ׳��� ������ ���� ���°� ����
			
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
		GET_Event();		// ZC�� �߻����� ���� ������ ZC�� ������� ����
		Gu8_ZeroCrossing_Err_Flag = 1;
	}
}
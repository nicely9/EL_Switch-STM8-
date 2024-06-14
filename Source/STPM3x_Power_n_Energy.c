/************************************************************************************
	Project		: 전자식스위치
	File Name	: STPM3x_Power_n_Energy.C
	Target		: EL_Switch
	Revision	: Rev 1.0
	Compiler	: ST Visual Develop(COSMIC)
	Date		: 2020/04/20
	Designed by	: J.S Yoon
	Modify		:
	Section		: NEO INFORMATION SYSTEMS CO.,LTD. R&D
************************************************************************************/

#include "header.h"
#include "STPM3x_opt.h"
#include "Timer.h"
#include "Debug.h"
#include "BigInteger.h"

#ifdef	STPM3x_ENERGY

METRO_Data_Energy_t		METRO_Data;


/**
  * @brief      This function Read Energy according to the selected type for the given channel
  * @param[in]   in_Metro_Channel (Channel ID), CHANNEL_1 to CHANNEL_2
  * @param[in]   in_Metro_energy_Selection : W_ACTIVE , F_ACTIVE, REACTIVE, APPARENT
  * @param[out]  None
  * @retval     Return NRJ value in mWh , mVARh  or mVAh ...  
  */

int32_t Metro_Read_energy(METRO_Channel_t in_Metro_Channel, METRO_Energy_selection_t in_Metro_Energy_Selection)
{
	struct bn num1, num2, num3;
	struct bn result;
	
	METRO_internal_Channel_t int_Channel;
	uint32_t	calc_nrj = 0;
	uint32_t	raw_nrj = 0;
	
	/* Get if the channel requested is the one or the two of the device */
	int_Channel =(METRO_internal_Channel_t) in_Metro_Channel;
	
	/* Get raw nrj according to device, channel and NRJ type */
	raw_nrj = Metro_HAL_read_energy(int_Channel,in_Metro_Energy_Selection);
	
	/* manage the 2 U32 to have enougth place to save energy cumulated */
	/* Make sure between two reads inside hardware registers if we have to add carry inside ext U32 */
	if (((uint32_t)(METRO_Data.energy[in_Metro_Channel][in_Metro_Energy_Selection])) > 0xA0000000  && (((uint32_t)raw_nrj) < 0x60000000))
	{
		METRO_Data.energy_extension[in_Metro_Channel][in_Metro_Energy_Selection] ++;
	}
	if (((uint32_t)(METRO_Data.energy[in_Metro_Channel][in_Metro_Energy_Selection])) < 0x60000000 && (((uint32_t)raw_nrj) > 0xA0000000))
	{
		METRO_Data.energy_extension[in_Metro_Channel][in_Metro_Energy_Selection] --;
	}
	
	/* save the new result cumulated come from register inside internal structure */
	METRO_Data.energy[in_Metro_Channel][in_Metro_Energy_Selection] = raw_nrj;
	
	
	// calculate the nrj value and add the 32 bits extension
	// calc_nrj = (uint64_t)raw_nrj + ((int64_t)METRO_Data.energy_extension[in_Metro_Channel][in_Metro_Energy_Selection] << 32);
	bignum_from_int(&num1, raw_nrj);
	bignum_from_int(&num2, METRO_Data.energy_extension[in_Metro_Channel][in_Metro_Energy_Selection]);
	bignum_lshift(&num2, &num3, 32);	// num3	= num2 << 32
	bignum_add(&num1, &num3, &result);
	
	// Apply Energy factors
	if (int_Channel == INT_CHANNEL_1)
	{
		bignum_assign(&num1, &result);		// Copy result -> num1
		//bignum_init(&result);				// clear
		bignum_from_int(&num2, GS_Metro_Device_Config.factor_energy_int_ch1);
		bignum_mul(&num1, &num2, &result);
	}
	else if (int_Channel == INT_CHANNEL_2)
	{
		bignum_assign(&num1, &result);		// Copy result -> num1
		//bignum_init(&result);				// clear
		bignum_from_int(&num2, GS_Metro_Device_Config.factor_energy_int_ch2);
		bignum_mul(&num1, &num2, &result);
	}
	//calc_nrj *= 10;		// multiply by 10 to have in milli-
	bignum_assign(&num1, &result);		// Copy result -> num1
	bignum_from_int(&num2, 10);
	bignum_mul(&num1, &num2, &result);
	
	//calc_nrj >>= 32;	// Put the result in 32 bits format
	bignum_rshift(&result, &result, 32);
	calc_nrj	= bignum_to_int(&result);
	
	return(calc_nrj);	// return the nrj value
	/*
	// calculate the nrj value and add the 32 bits extension
	calc_nrj = (uint64_t)raw_nrj + ((int64_t)METRO_Data.energy_extension[in_Metro_Channel][in_Metro_Energy_Selection] << 32);
	
	// Apply Energy factors
	if (int_Channel == INT_CHANNEL_1)
	{
		calc_nrj *= (int64_t)GS_Metro_Device_Config.factor_energy_int_ch1;
	}
	else if (int_Channel == INT_CHANNEL_2)
	{
		calc_nrj *= (int64_t)GS_Metro_Device_Config.factor_energy_int_ch2;
	}
	calc_nrj *= 10;		// multiply by 10 to have in milli-
	calc_nrj >>= 32;	// Put the result in 32 bits format
	
	return((int32_t)calc_nrj);	// return the nrj value
	*/
}

/**
  * @brief      This function read the Energy of signal come from one Channel mapped in one device
  * @param[in]   in_Metro_Device_Id (device ID), HOST or EXT1 to EXT4
  * @param[in]   in_Metro_int_Channel (Channel ID), Channel 1 or 2 ( according to device )
  * @param[in]   in_Metro_Energy_Selection : E_W_ACTIVE , E_F_ACTIVE, E_REACTIVE, E_APPARENT
  * @param[out]  None
  * @retval     int32_t raw_nrj according to Energy type  read inside device register
  */

int32_t Metro_HAL_read_energy(METRO_internal_Channel_t in_Metro_int_Channel, METRO_Energy_selection_t in_Metro_Energy_Selection)
{
	int32_t raw_nrj = 0;
	
	switch (in_Metro_Energy_Selection)
	{
		case (E_W_ACTIVE):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				// get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg
				raw_nrj = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG1));
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)
			{
				// get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg
				raw_nrj = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG1));
			}
			break;
		case (E_F_ACTIVE):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				// get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg
				raw_nrj = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG2));
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				// get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg
				raw_nrj = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG2));
			}
			break;
		case (E_REACTIVE):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				// get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg
				raw_nrj = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG3));
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				// get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg
				raw_nrj = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG3));
			}
			break;
		case (E_APPARENT):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				// get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg
				raw_nrj = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG4));
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				// get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg
				raw_nrj = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG4));
			}
			break;
	}
	
	return raw_nrj;
}

#endif


#ifdef STPM3x_POWER


/**
  * @brief      This function read the Power of signal come from one Channel mapped in one device
* @param[in]   in_Metro_Device_Id : EXT1 to EXT4
  * @param[in]   in_Metro_int_Channel (Channel ID), Channel 1 or 2 ( according to device )
  * @param[in]   in_Metro_Power_Selection : W_ACTIVE , F_ACTIVE, REACTIVE, APPARENT_RMS, APPARENT_VEC, MOM_WIDE_ACT, MOM_FUND_ACT
  * @param[out]  None
  * @retval     int32_t raw_power according to power type  read inside device register
  */
int32_t Metro_HAL_read_power(METRO_internal_Channel_t in_Metro_int_Channel,METRO_Power_selection_t in_Metro_Power_Selection)
{
	int32_t raw_power = 0;
	
	switch (in_Metro_Power_Selection)
	{
		case (W_ACTIVE):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				/* get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG5)&BIT_MASK_STPM_PRIM_CURR_ACTIVE_POW);
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				/* get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG5)&BIT_MASK_STPM_SEC_CURR_ACTIVE_POW);
			}
			break;
#ifndef _STPM3x_MIN_READ_
		case (F_ACTIVE):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				/* get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG6)&BIT_MASK_STPM_PRIM_CURR_FUND_POW);
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				/* get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG6)&BIT_MASK_STPM_SEC_CURR_FUND_POW);
			}
			break;
		case (REACTIVE):
			/* is Channel is the first or the second  channel of the chip */
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				/* get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG7)&BIT_MASK_STPM_PRIM_CURR_REACT_POW);
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				/* get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG7)&BIT_MASK_STPM_SEC_CURR_REACT_POW);
			}
			break;
#endif
		case (APPARENT_RMS):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				/* get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG8)&BIT_MASK_STPM_PRIM_CURR_RMS_POW);
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				/* get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG8)&BIT_MASK_STPM_SEC_CURR_RMS_POW);
			}
			break;
#ifndef _STPM3x_MIN_READ_
		case (APPARENT_VEC):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				/* get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG9)&BIT_MASK_STPM_PRIM_CURR_VEC_POW);
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				/* get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG9)&BIT_MASK_STPM_SEC_CURR_VEC_POW);
			}
			break;
		case (MOM_WIDE_ACT):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				/* get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG10)&BIT_MASK_STPM_PRIM_CURR_MOM_ACTIVE_POW);
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				/* get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG10)&BIT_MASK_STPM_SEC_CURR_MOM_ACTIVE_POW);
			}
			break;
		case (MOM_FUND_ACT):
			if (in_Metro_int_Channel == INT_CHANNEL_1)
			{
				/* get directly from RAM, be carrefull : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH1_REG11)&BIT_MASK_STPM_PRIM_CURR_MOM_FUND_POW);
			}
			else if (in_Metro_int_Channel == INT_CHANNEL_2)/* is channel 2 */
			{
				/* get directly from RAM, be carrefull !!!!! : latch should be made before to have coherents values inside DSP data reg */
				raw_power = ((GS_Metro_Device_Config.metro_stpm_reg.CH2_REG11)&BIT_MASK_STPM_SEC_CURR_MOM_FUND_POW);
			}
			break;
#endif
	} /* end switch */
	
	raw_power <<= 4;  // handle sign extension as power is on 28 bits
	raw_power >>= 4;

	return raw_power;
}

/**
  * @brief      This function Read Power according to the selected type for the given channel
  * @param[in]   in_Metro_Channel (Channel ID ), CHANNEL_1 to CHANNEL_4
  * @param[in]   in_Metro_Power_Selection : W_ACTIVE , F_ACTIVE, REACTIVE, APPARENT_RMS, APPARENT_VEC, MOM_WIDE_ACT, MOM_FUND_ACT
  * @param[out]  None
  * @retval     Return power value in  in mW  , mVAR  or mVA ... 
  */
int32_t Metro_Read_Power(METRO_Channel_t in_Metro_Channel, METRO_Power_selection_t in_Metro_Power_Selection)
{
	int i, sign = 0;
	struct bn num1, num2, num3;
	struct bn result;
	
	METRO_internal_Channel_t int_Channel;
	
	int32_t calc_power = 0;
	int32_t raw_power = 0;
	
	/* Get if the channel requested is the one or the two of the device */
	int_Channel =(METRO_internal_Channel_t) in_Metro_Channel;
	
	/* Get raw power according to device and channel */
	raw_power = Metro_HAL_read_power(int_Channel, in_Metro_Power_Selection);

	if(in_Metro_Power_Selection == W_ACTIVE || in_Metro_Power_Selection == REACTIVE)
	{
		if(raw_power < 0)
		{
			raw_power = raw_power * -1;
			sign	= 1;
		}
	}
	
	/* Calc real power  */
	if (int_Channel == INT_CHANNEL_1)
	{
		//calc_power =  (int64_t)raw_power * GS_Metro_Device_Config.factor_power_int_ch1;
		bignum_from_int(&num1, raw_power);
		bignum_from_int(&num2, GS_Metro_Device_Config.factor_power_int_ch1);
		bignum_mul(&num1, &num2, &result);
		// printf("CH1 num1 = %d raw_power = %d\r\n", num1, raw_power);
		// printf("CH1 num2 = %d factor_power_int_ch1 = %d\r\n", num2, GS_Metro_Device_Config.factor_power_int_ch1);
		// printf("CH1 result = %d\r\n", result);
	}
	else if (int_Channel == INT_CHANNEL_2)
	{
		//calc_power = (int64_t)raw_power * GS_Metro_Device_Config.factor_power_int_ch2;
		bignum_from_int(&num1, raw_power);
		bignum_from_int(&num2, GS_Metro_Device_Config.factor_power_int_ch2);
		bignum_mul(&num1, &num2, &result);
		// printf("CH2 num1 = %d raw_power = %d\r\n", num1, raw_power);
		// printf("CH2 num2 = %d factor_power_int_ch2 = %d\r\n", num2, GS_Metro_Device_Config.factor_power_int_ch1);
		// printf("CH2 result = %d\r\n", result);		
	}
	
	//calc_power *= 10;
	bignum_assign(&num1, &result);		// Copy result -> num1
	bignum_from_int(&num2, 10);
	bignum_mul(&num1, &num2, &result);
	
	//calc_power >>= 28;
	bignum_rshift(&result, &result, 28);
	calc_power	= bignum_to_int(&result);
	// printf("calc_power = %d\r\n", calc_power);
	if(in_Metro_Power_Selection == W_ACTIVE || in_Metro_Power_Selection == REACTIVE)
	{
		//if(sign)
		if(sign == 0)		// 흠... 결과를 반대로 해야하내...
		{
			calc_power = calc_power * -1;
		}
	}
	
	return (calc_power);
	/*
	// multiply by 10 to have in milli-
	calc_power *= 10; 
	// Shift calcul result to 28 bits ( resolution of Reg inside metrology block)
	calc_power >>= 28;
	// return power selection calculated with Factor Power
	return ((int32_t)calc_power);
	*/
}

#endif

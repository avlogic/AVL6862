
/* **************************************************** */
/*!
   @file	MN_DMD_Tuner_template.c
   @brief	Tuner API wrapper (sample)
   @author	R.Mori
   @date	2011/6/30
   @param
		(c)	Panasonic
   */
/* **************************************************** */
//---------- this is sample Tuner API  ----------------
#include "MN_DMD_driver.h"
#include "MN_DMD_common.h"
#if (TUNER_TYPE == MXL603)   
#include "MxL603_TunerApi.h"
#include "MaxLinearDataTypes.h"
#endif
/* **************************************************** */
/* **************************************************** */

/*! Tuner Initialize Function*/
DMD_u32_t DMD_Tuner_init(DMD_PARAMETER* param)
{
	//TODO:	Please call tuner initialize API here

	DMD_ERROR_t ret;
	#if (TUNER_TYPE == MXL603)   
	ret = MXL603_INIT(param);	
	 #endif
     if (ret != DMD_E_OK)
    {
      DMD_DBG_TRACE("Error! DMD_Tuner_init\n");    
    }
 
	//this function is called by DMD_init
	
	//DMD_DBG_TRACE("Tuner is not implemeted\n");
	return DMD_E_OK;
}

/*! Tuner Tune Function */
DMD_u32_t DMD_Tuner_tune(DMD_PARAMETER *param)
{
	//TODO:	Please call tune  API here
	DMD_ERROR_t ret;
	#if (TUNER_TYPE == MXL603)   
	ret = Mxl603SetFreqBw(param);
	 #endif
     if (ret != DMD_E_OK)
    {
      DMD_DBG_TRACE("Error! DMD_Tuner_init()\n");    
     }
	 
	//this function is called by DMD_tune
	//DMD_DBG_TRACE("Tuner is not implemeted\n");
	return DMD_E_OK;
}

/*! Tuner Termination Function */
DMD_u32_t DMD_Tuner_term(void)
{
	//TODO:	Please call tune  API here
	//this function is called by DMD_term
	DMD_DBG_TRACE("Tuner is not implemeted\n");
	return DMD_E_OK;
}

/*! Tuner Tune Function */
DMD_u32_t DMD_Tuner_set_system(DMD_PARAMETER *param)
{
	//TODO:	Please call tune  API here
	DMD_ERROR_t ret;
	#if (TUNER_TYPE == MXL603)   
	ret = Mxl603SetSystemMode();
	 #endif
     if (ret != DMD_E_OK)
    {
      DMD_DBG_TRACE("Error! DMD_Tuner_set_system \n");    
     }	
	
	//this function is called by DMD_tune
	//DMD_DBG_TRACE("Tuner is not implemeted\n");
	return DMD_E_OK;
}

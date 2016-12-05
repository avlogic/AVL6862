/* **************************************************** */
/*!
   @file	MN_DMD_Device.h
   @brief	Panasonic Demodulator Driver
   @author	R.Mori
   @date	2011/6/30
   @param
		(c)	Panasonic
   */
/* **************************************************** */
/*!
   this file defines common interface for each demodulator device
   */

#include "MN_DMD_driver.h"

#ifndef MN_DMD_DEVICE_H
#define MN_DMD_DEVICE_H

//#ifdef __cplusplus
//extern "C" {
//#endif

//return DMD_E_OK , if device support the system & bandwidth

extern DMD_u32_t	DMD_system_support( DMD_SYSTEM_t sys );

/* **************************************************** */
/*  Demodulator dependence functions (not exported)*/
/* **************************************************** */
//these functions is defined by each device (device_name.c)
extern DMD_u32_t	DMD_device_open( DMD_PARAMETER *param );
extern DMD_u32_t	DMD_device_term( DMD_PARAMETER *param );
extern DMD_u32_t	DMD_device_close( DMD_PARAMETER *param );
extern DMD_u32_t	DMD_device_init( DMD_PARAMETER *param );
extern DMD_u32_t	DMD_device_load_pseq ( DMD_PARAMETER *param );
extern DMD_u32_t	DMD_device_pre_tune ( DMD_PARAMETER *param );
extern DMD_u32_t	DMD_device_post_tune ( DMD_PARAMETER *param );
extern DMD_u32_t	DMD_device_set_system( DMD_PARAMETER *param ); 
extern DMD_u32_t	DMD_device_reset( DMD_PARAMETER *param );
extern DMD_u32_t	DMD_device_scan( DMD_PARAMETER *param );
extern DMD_u32_t	DMD_device_get_info( DMD_PARAMETER *param , DMD_u32_t id);
extern DMD_u32_t	DMD_device_set_info( DMD_PARAMETER *param , DMD_u32_t id ,DMD_u32_t val);
extern DMD_u32_t	DMD_device_set_TCB_mode( DMD_PARAMETER* param); //troy.wangyx, 20120801 
extern DMD_u32_t   DMD_device_set_echo_enhance(DMD_PARAMETER* param, DMD_ECHO_PERFORMANCE_SET echo_out );//troy.wangyx, 20120801 
extern DMD_u32_t   DMD_device_cochan_interface_detect( DMD_PARAMETER *param );//troy.wangyx, 20120801 

/* '11/08/29 : OKAMOTO	Select TS output. */
extern DMD_u32_t DMD_set_ts_output(DMD_PARAMETER *param, DMD_TSOUT_MODE ts_out_mode, DMD_TSCLK_POLARITY ts_clk_polarity);
extern DMD_u32_t	DMD_set_error_flag_output(DMD_u8_t bErrorFlagOutputEnable );
extern DMD_u32_t	DMD_RegSet_Rev;

//#ifdef __cplusplus
//}
//#endif

#endif

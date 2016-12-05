/* **************************************************** */
/*!
   @file	MN_DMD_common.h
   @brief	Panasonic Demodulator Driver
   @author	R.Mori
   @date	2011/6/30
   @param
		(c)	Panasonic
   */
/* **************************************************** */


#ifndef MN_DMD_CONSOLE_H
#define MN_DMD_CONSOLE_H

#include "MN_DMD_driver.h"
#include "mnfrontend.h"

#ifdef __cplusplus
extern "C" {
#endif

DMD_u32_t	DMD_con_get_int(void);
DMD_u32_t	DMD_con_select_system( DMD_PARAMETER *param );
DMD_u32_t	DMD_con_select_bw( DMD_PARAMETER *param );
DMD_u32_t	DMD_con_input_freq( DMD_PARAMETER *param );
DMD_u32_t	DMD_con_show_status( DMD_PARAMETER *param );
DMD_u32_t	DMD_con_show_monitor( DMD_PARAMETER *param );
DMD_u32_t	DMD_con_i2c_test( DMD_PARAMETER *param );
DMD_u32_t	DMD_con_channel_search( DMD_PARAMETER *param );
DMD_u32_t DMD_con_tcb_test( DMD_PARAMETER* param);

#ifdef __cplusplus
}
#endif

#endif

/* **************************************************** */
/*!
   @file	MN_I2C.h
   @brief	I2C Infterface for MN_DMD_Driver
   @author	R.Mori
   @date	2011/6/30
   @param
		(c)	Panasonic
   */
/* **************************************************** */


#ifndef MN_I2C_H
#define MN_I2C_H
//#include <linux/i2c.h>
#include "mnfrontend.h"

#ifdef __cplusplus
extern "C" {
#endif

/* **************************************************** */
/*  System dependence functions */
/* **************************************************** */
/* I2C Bus Functions */
/* these function is defined by MN_DMD_I2C_(system).c */
extern DMD_u32_t DMD_I2C_open(void);
extern DMD_u32_t DMD_I2C_Write(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t  data );
extern DMD_u32_t DMD_I2C_Read (DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t* data );
extern DMD_u32_t DMD_I2C_Write_Anylenth(DMD_u8_t slvadr , DMD_u8_t adr , DMD_u8_t* wdata , DMD_u32_t wlen);
extern DMD_u32_t DMD_TCBI2C_Read(DMD_u8_t slvadr , DMD_u8_t adr , DMD_u8_t* wdata , DMD_u32_t wlen , DMD_u8_t* rdata , DMD_u32_t rlen);
extern DMD_u32_t DMD_I2C_WriteRead(DMD_u8_t slvadr , DMD_u8_t adr , DMD_u8_t* wdata , DMD_u32_t wlen , DMD_u8_t* rdata , DMD_u32_t rlen);
extern DMD_u32_t DMD_I2C_term(void);
extern DMD_u32_t DMD_wait( DMD_u32_t	msecond);
extern DMD_u32_t DMD_timer(DMD_u32_t* tim);
extern DMD_u32_t I2CRead(DMD_u8_t I2CSlaveAddr, DMD_u8_t *data, DMD_u8_t length);
extern DMD_u32_t I2CWrite(DMD_u8_t I2CSlaveAddr, DMD_u8_t *data, DMD_u8_t length);



#ifdef DMD_I2C_DEBUG
extern DMD_u32_t	dmd_i2c_debug_flag;
#endif


#ifdef __cplusplus
}
#endif

#endif

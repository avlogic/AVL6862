/* **************************************************** */
/*!
   @file	MN_TCB_diag.c
   @brief	Device dependence functions 
   @author	R.Mori
   @date	2012/4/9
   @param
		(c)	Panasonic
   */
/* **************************************************** */

#include "MN_DMD_driver.h"
#include "MN_DMD_common.h"
#include "MN_DMD_device.h"
#include "MN_I2C.h"
#include "MN_TCB.h"
#include "MN88436_reg.h"

/* **************************************************** */
/* Tuner BUS Controll */
/* **************************************************** */
/* **************************************************** */
/*! Write 1byte to Tuner via Demodulator */
/* **************************************************** */

DMD_u32_t DMD_TCB_Write(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t data )
{
	return DMD_TCB_WriteRead( slvadr , adr , &data , 1 , 0, 0 );
}

/* **************************************************** */
/*! Read 1byte from Tuner via Demodulator */
/* **************************************************** */
DMD_u32_t DMD_TCB_Read(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t *data )
{
	return DMD_TCB_WriteRead( slvadr , adr ,  0, 0 , data , 1 );
}

/* '11/08/05 : OKAMOTO Implement "Through Mode". */
DMD_u32_t DMD_TCB_WriteAnyLength(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t* data , DMD_u32_t wlen)
{
	return DMD_TCB_WriteRead( slvadr , adr , data , wlen , 0, 0 );
}

/* '11/08/05 : OKAMOTO Implement "Through Mode". */
DMD_u32_t DMD_TCB_ReadAnyLength(DMD_u8_t	slvadr , DMD_u8_t *data  , DMD_u8_t rlen)
{
	return DMD_TCB_WriteRead( slvadr , 0 ,  0, 0 , data , rlen );
}

/* **************************************************** */
/*! Write&Read any length from/to Tuner via Demodulator */
/* **************************************************** */
DMD_u32_t DMD_TCB_WriteRead(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t* wdata , DMD_u32_t wlen , DMD_u8_t* rdata , DMD_u32_t rlen)
{
	DMD_u8_t	d[DMD_TCB_DATA_MAX];
	DMD_u32_t i;
	DMD_ERROR_t	ret= DMD_E_ERROR;
	DMD_u8_t	bank;
	DMD_u8_t	tcbcom;//,tcbadr,tcbset;


	bank = DMD_BANK_MAIN(0);//param->id
	tcbcom=DMD_MAIN_TCBCOM;
//	tcbadr=DMD_MAIN_TCBADR;
//	tcbset=DMD_MAIN_TCBSET;

	/* Set TCB Through Mode */
//	ret  = DMD_I2C_MaskWrite( bank , tcbset , 0x7f , 0x53 );//Troy.wangyx masked, 20120801, once is enough; already done after initialization.(call function)
//	ret |= DMD_I2C_Write( bank , tcbadr , 0x00 );

	if( (wlen == 0 && rlen == 0) ||  (wlen != 0) )
	{
		d[0] = slvadr;
		d[1] = adr;
		for(i=0;i<wlen;i++)	d[i+2] = wdata[i];
		/* Read/Write */
		ret = DMD_I2C_Write_Anylenth(bank , tcbcom , d , wlen + 2 );
	}
	else
	{
		d[0] = slvadr | 1;
		ret = DMD_TCBI2C_Read(bank , tcbcom , d , 1 , rdata , rlen );

	}

	return ret;
}	

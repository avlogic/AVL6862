/* **************************************************** */
/*!
   @file	MN_DMD_device.c
   @brief	Device dependence functions for MN88436 multiple DMD
   @author	Simon
   @date	2012/7/24
   @param
		(c)	Panasonic
   */
/* **************************************************** */

#include "MN_DMD_driver.h"
#include "MN_DMD_common.h"
#include "MN_DMD_device.h"
#include "MN_I2C.h"
#include "MN88436_reg.h"
#include "MN_DMD_types.h"

/* **************************************************** */
/*	Constant */
/* **************************************************** */

DMD_u8_t	DMD_I2C_SLAVE[DMD_MAX_DEVICE][DMD_REG_BANK] =	{ {0x18,0x10} , {0x19,0x11} , {0x1A,0x12} , {0x1B,0x13} };

/* **************************************************** */
/*	Local Functions */
/* **************************************************** */

/* Information */
DMD_u32_t DMD_all       (DMD_PARAMETER* param);
DMD_u32_t DMD_system    (DMD_PARAMETER* param);
DMD_u32_t DMD_lock      (DMD_PARAMETER* param);
DMD_u32_t DMD_agc       (DMD_PARAMETER* param);
DMD_u32_t DMD_ber       (DMD_PARAMETER* param);
DMD_u32_t DMD_per       (DMD_PARAMETER* param);
DMD_u32_t DMD_cnr       (DMD_PARAMETER* param);
DMD_u32_t DMD_errorfree (DMD_PARAMETER* param);

DMD_u32_t DMD_info_atsc  ( DMD_PARAMETER *param , DMD_u32_t id , DMD_u32_t allflag);
DMD_u32_t DMD_info_qam   ( DMD_PARAMETER *param , DMD_u32_t id , DMD_u32_t allflag);
DMD_u32_t DMD_info_analog( DMD_PARAMETER *param,DMD_u32_t id,DMD_u32_t allflag );

/* Scan ( for Channel Search )*/
DMD_u32_t DMD_scan_vq( DMD_PARAMETER *param );

DMD_u32_t DMD_send_regset( DMD_u32_t	devid , DMD_u8_t* regset );

/* **************************************************** */
/*	Functions */
/* **************************************************** */
DMD_u8_t DMD_BANK_MAIN(DMD_u8_t id)
{
	DMD_u8_t	ret;

	switch(id)
	{
		case 0: ret = 0x18; break;
		case 1: ret = 0x19; break;
		case 2: ret = 0x1A; break;
		case 3: ret = 0x1B; break;
		default: ret = 0x18;break;
	}
	return ret;
}

DMD_u8_t DMD_BANK_USR(DMD_u8_t id)
{
	DMD_u8_t	ret;

	switch(id)
	{
		case 0: ret = 0x10; break;
		case 1: ret = 0x11; break;
		case 2: ret = 0x12; break;
		case 3: ret = 0x13; break;
		default: ret = 0x10;break;
	}
	return ret;
}


/* **************************************************** */
/*! System Support information */
/* **************************************************** */

DMD_u32_t	DMD_system_support( DMD_SYSTEM_t sys )
{
	DMD_ERROR_t	ret;

	switch( sys )
	{
	case DMD_E_ATSC:
	case DMD_E_QAMB_64QAM:
	case DMD_E_QAMB_256QAM:
		ret = DMD_E_OK;
		break;
	case DMD_E_ISDBT:
	case DMD_E_ISDBT_BRAZIL:
	case DMD_E_ISDBS:
	case DMD_E_DVBT:
	case DMD_E_DVBC:
	case DMD_E_QAMC_64QAM:
	case DMD_E_QAMC_256QAM:
	case DMD_E_NTSC_M_BTSC:
	case DMD_E_PAL_M_BTSC:
	case DMD_E_PAL_N_BTSC:
	case DMD_E_PAL_B_G_NICAM:
	case DMD_E_PAL_I_NiCAM:
	case DMD_E_PAL_D_NiCAM:
	case DMD_E_PAL_B_G_A2:
	case DMD_E_SECAM_L_NiCAM:
	case DMD_E_SECAM_L1_NiCAM:
	case DMD_E_SECAM_D_K_A2:
	case DMD_E_SECAM_BG_NICAM:
	case DMD_E_DVBT2:
	case DMD_E_DVBC2:
	default:
		ret = DMD_E_ERROR;
		break;
	}

	return ret;
}
/* **************************************************** */
/*! Load Auto Control Sequence */
/* **************************************************** */
DMD_u32_t DMD_device_load_pseq( DMD_PARAMETER* param )
{
	DMD_ERROR_t ret = DMD_E_OK;
	DMD_u8_t	data;
	DMD_u32_t	i;
	/* Load PSEQ Program */
	ret  = DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_PSEQSET , 0x03 );
	for(i=0;i<MN88436_REG_AUTOCTRL_SIZE;i++)
		ret |= DMD_I2C_Write( DMD_BANK_MAIN(param->devid), DMD_MAIN_PSEQPRG  , MN88436_REG_AUTOCTRL[i] );

	/* Check Parity bit */
	ret |= DMD_I2C_Read( DMD_BANK_MAIN(param->devid) , DMD_MAIN_PSEQSET , &data);
	printk("[0xf0]data is %x\n",data);
	if( data & 0x20 ){
		DMD_DBG_TRACE( "ERROR: PSEQ Parity" );
		ret |= DMD_E_ERROR;
	}
	else
	{
		ret |= DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_PSEQSET  , 0x00 );
	}
	return ret;
}

/* **************************************************** */
/*! Device Open */
/* **************************************************** */
DMD_u32_t DMD_device_open( DMD_PARAMETER* param )
{
	 return DMD_E_OK;
}
/* **************************************************** */
/*! Device term */
/* **************************************************** */
DMD_u32_t DMD_device_term( DMD_PARAMETER* param )
{
	 return DMD_E_OK;
}

/* **************************************************** */
/*! Device close */
/* **************************************************** */
DMD_u32_t DMD_device_close( DMD_PARAMETER* param )
{
	 return DMD_E_OK;
}

/* **************************************************** */
/*!	Get Register Settings for Broadcast system */
/* **************************************************** */
DMD_u32_t DMD_device_init( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret;
	ret = DMD_E_OK;

	ret = DMD_send_registers( DMD_I2C_SLAVE[param->devid] , DMD_REG_ATSC );
		
	return ret;
}


/* **************************************************** */
/*!	Set Register setting for each broadcast system & bandwidth */
/* **************************************************** */
DMD_u32_t	DMD_device_set_system( DMD_PARAMETER* param){
	DMD_ERROR_t	ret;
	ret = DMD_E_OK;
	//Register transfer
	switch( param->system ){
		case DMD_E_ATSC:
			DMD_send_registers( DMD_I2C_SLAVE[param->devid] , DMD_REG_ATSC );
			//DMD_device_load_pseq( param ); //troy.wangyx masked, 20120801, only need do once, in initialization. 
			break;
		case DMD_E_QAMB_64QAM:
			DMD_send_registers( DMD_I2C_SLAVE[param->devid] , DMD_REG_QAM_B_64QAM );
			//DMD_device_load_pseq( param );
			break;
		case DMD_E_QAMB_256QAM:
			DMD_send_registers( DMD_I2C_SLAVE[param->devid] , DMD_REG_QAM_B_256QAM );
			//DMD_device_load_pseq( param );
			break;
		default:
			//Not Support
			DMD_DBG_TRACE( "ERROR : Not Supported System");
			ret = DMD_E_ERROR;
	}

	return ret;

}
/* **************************************************** */
/*! Pretune Process */
/* **************************************************** */
DMD_u32_t DMD_device_pre_tune( DMD_PARAMETER* param )
{
	return DMD_E_OK;
}
/* **************************************************** */
/*! Pretune Process */
/* **************************************************** */
DMD_u32_t DMD_device_post_tune( DMD_PARAMETER* param )
{
	DMD_device_reset(param);
	return DMD_E_OK;
}
/* **************************************************** */
/*! Soft Reset */
/* **************************************************** */
DMD_u32_t DMD_device_reset( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret;
	switch( param->system ){
		case DMD_E_ATSC:
		case DMD_E_QAMB_64QAM:
		case DMD_E_QAMB_256QAM:
			DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_RSTSET1 , 0x77);
			ret = DMD_E_OK;
			break;
		default:
			ret = DMD_E_ERROR;
			break;
	}

	return ret;
}

/* **************************************************** */
/*! scan */
/* **************************************************** */
DMD_u32_t DMD_device_scan( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret = DMD_E_ERROR;
	switch( param->system )
	{
		case DMD_E_ATSC:
		case DMD_E_QAMB_64QAM:
		case DMD_E_QAMB_256QAM:
			ret = DMD_scan_vq(param);
			break;
		default:
			break;
	}
	return ret;
}

/* **************************************************** */
/*! Set Information  */
/* **************************************************** */
DMD_u32_t DMD_device_set_info( DMD_PARAMETER* param , DMD_u32_t id  , DMD_u32_t val)
{
	DMD_ERROR_t	ret =DMD_E_OK;

	return ret;
}


/* **************************************************** */
/*! Get Information  */
/* **************************************************** */
DMD_u32_t DMD_device_get_info( DMD_PARAMETER* param , DMD_u32_t id )
{
	DMD_ERROR_t	ret;
	DMD_u32_t	all=0;
	DMD_u8_t	rd;

	ret = DMD_E_OK;
	switch(id)
	{
		case DMD_E_INFO_ALL:
			ret |= DMD_all(param);
			all = 1;
		case DMD_E_INFO_REGREV:
			param->info[DMD_E_INFO_REGREV] = DMD_RegSet_Rev;
			if( all == 0 ) break;
		case DMD_E_INFO_PSEQREV:
			DMD_I2C_Read( DMD_BANK_MAIN(param->devid) , DMD_MAIN_PSEQOP1 , &rd );
			param->info[DMD_E_INFO_PSEQREV] = rd;
			if( all == 0 ) break;
		case DMD_E_INFO_SYSTEM:
			ret |= DMD_system(param);
			if( all == 0 ) break;
		case DMD_E_INFO_STATUS:
		case DMD_E_INFO_LOCK:
			ret |= DMD_lock(param);
			if( all == 0 ) break;
		case DMD_E_INFO_AGC:
			ret |= DMD_agc(param);
			if( all == 0 ) break;
		case DMD_E_INFO_BERRNUM	:	
		case DMD_E_INFO_BITNUM	:
			ret |= DMD_ber(param);
			if( all == 0 ) break;
		case DMD_E_INFO_CNR_INT:
		case DMD_E_INFO_CNR_DEC:
			ret |= DMD_cnr(param);
			if( all == 0 ) break;
		case DMD_E_INFO_PERRNUM:	
		case DMD_E_INFO_PACKETNUM:
			ret |= DMD_per(param);
			if( all == 0 ) break;
		case DMD_E_INFO_ERRORFREE:
			ret |= DMD_errorfree(param);
			if( all == 0 ) break;
		default:
			switch( param->system )
			{
				case DMD_E_ATSC:
					ret |= DMD_info_atsc(param,id,all);
					break;
				case DMD_E_QAMB_64QAM:
				case DMD_E_QAMB_256QAM:
					ret |= DMD_info_qam(param,id,all);
					break;
				default:
					break;
			}
	}
	
	return ret;
}

/* **************************************************** */
/* Informations */
/* **************************************************** */
/*Start********************************************************************/
/**
 * DMD_all
 * 
 * DMD_PARAMETER* param
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_all( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	DMD_u32_t	info = 0;
	switch( param->system ){
		case DMD_E_ATSC:
			info = DMD_E_INFO_ATSC_END_OF_INFORMATION;
			break;
		case DMD_E_QAMB_64QAM:
		case DMD_E_QAMB_256QAM:
			info = DMD_E_INFO_QAM_END_OF_INFORMATION;
			break;
		default:
			//Not Support
				DMD_DBG_TRACE( "ERROR : Not Supported System");
				ret = DMD_E_ERROR;
	}

	param->info[DMD_E_INFO_ALL] = info;
	return ret;
}
/*Start********************************************************************/
/**
 * DMD_system
 * 
 * DMD_PARAMETER* param
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_system( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	
	param->info[DMD_E_INFO_SYSTEM] = param->system;

	return ret;
}
/*Start********************************************************************/
/**
 * DMD_lock
 * 
 * DMD_PARAMETER* param
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_lock( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	DMD_u8_t	rd = 0;
	DMD_u32_t	lockstatus = 0;
	switch( param->system ){
		case DMD_E_ATSC:
		case DMD_E_QAMB_64QAM:
		case DMD_E_QAMB_256QAM:
			DMD_I2C_Read( DMD_BANK_MAIN(param->devid) , DMD_MAIN_STSMON1	 , &rd );
			rd &= 1;
			if( rd ) lockstatus = 1;
			break;
		default:
			//Not Support
				DMD_DBG_TRACE( "ERROR : Not Supported System");
				ret = DMD_E_ERROR;
	}

	param->info[DMD_E_INFO_STATUS] = rd;
	//printk("lockstatus is %d\n",lockstatus);
	if( lockstatus )
	{
		param->info[DMD_E_INFO_LOCK] = DMD_E_LOCKED;
	} 
	else if( param->info[DMD_E_INFO_LOCK] == DMD_E_LOCKED )
	{
		param->info[DMD_E_INFO_LOCK] = DMD_E_LOCK_ERROR;
	}
	return ret;
}
/*Start********************************************************************/
/**
 * DMD_agc
 * 
 * DMD_PARAMETER* param
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_agc( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	DMD_u32_t	ifagc = 0, rfagc = 0;
	DMD_u8_t	RF1, RF2, IF1, IF2;
	
	switch( param->system ){
		case DMD_E_ATSC:
		case DMD_E_QAMB_64QAM:
		case DMD_E_QAMB_256QAM:
			DMD_I2C_Read( DMD_BANK_USR(param->devid) , DMD_USR_RFAGCMON1 , &RF1 );
			DMD_I2C_Read( DMD_BANK_USR(param->devid) , DMD_USR_RFAGCMON2 , &RF2 );
			DMD_I2C_Read( DMD_BANK_USR(param->devid) , DMD_USR_IFAGCMON1 , &IF1 );
			DMD_I2C_Read( DMD_BANK_USR(param->devid) , DMD_USR_IFAGCMON2 , &IF2 );
			rfagc = RF1 * 256 + RF2;
			ifagc = IF1 * 256 + IF2;
			break;
		default:
			//Not Support
				DMD_DBG_TRACE( "ERROR : Not Supported System");
				ret = DMD_E_ERROR;
	}

	param->info[DMD_E_INFO_AGC] = ifagc;
	
	return ret;
}
/*Start********************************************************************/
/**
 * DMD_ber
 * 
 * DMD_PARAMETER* param
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_ber( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	DMD_u32_t	berr = 0 ,bit = 1;
	DMD_u8_t	rd;
	switch( param->system ){
		case DMD_E_ATSC:
		case DMD_E_QAMB_64QAM:
		case DMD_E_QAMB_256QAM:
			DMD_I2C_Write( DMD_BANK_USR(param->devid) , DMD_USR_BERTSET1 , 0x40 );
			DMD_I2C_Write( DMD_BANK_USR(param->devid) , DMD_USR_BERTSET2 , 0x0C );
			DMD_I2C_Read( DMD_BANK_USR(param->devid) , DMD_USR_BERTSET1 , &rd );
			rd &= 0xfe;		//bit error mode
			DMD_I2C_Write( DMD_BANK_USR(param->devid) , DMD_USR_BERTSET1 , rd );
			DMD_I2C_Read ( DMD_BANK_USR(param->devid) , DMD_USR_BERTSET2 , &rd );
			bit = (rd >> 2) & 0x7;
			if( param->system == DMD_E_ATSC )
				bit = (1 << (5+bit*2))*208*8;
			else
				bit = (1 << (5+bit*2))*127*7;
			DMD_I2C_Read ( DMD_BANK_USR(param->devid) , DMD_USR_BERMON1 , &rd );
			berr = rd * 65536;
			DMD_I2C_Read ( DMD_BANK_USR(param->devid) , DMD_USR_BERMON2 , &rd );
			berr += rd * 256;
			DMD_I2C_Read ( DMD_BANK_USR(param->devid) , DMD_USR_BERMON3 , &rd );
			berr += rd ;
			break;
		default:
			//Not Support
				DMD_DBG_TRACE( "ERROR : Not Supported System");
				ret = DMD_E_ERROR;
	}

	param->info[DMD_E_INFO_BERRNUM] = berr;
	param->info[DMD_E_INFO_BITNUM]  = bit;
	return ret;
}

/*Start********************************************************************/
/**
 * DMD_per
 * 
 * DMD_PARAMETER* param
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_per( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	DMD_u32_t	perr = 0 ,pkt = 1;
	DMD_u8_t	rd;
	switch( param->system ){
		case DMD_E_ATSC:
		case DMD_E_QAMB_64QAM:
		case DMD_E_QAMB_256QAM:
			DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_BERTSET1P , 0x41 );
			DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_BERTSET2P , 0x0C );
			DMD_I2C_Read( DMD_BANK_MAIN(param->devid) , DMD_MAIN_BERTSET1P , &rd );
			rd &= 0xff;	
			DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_BERTSET1P , rd );
			DMD_I2C_Read ( DMD_BANK_MAIN(param->devid) , DMD_MAIN_BERTSET2P , &rd );
			pkt = (rd >> 2) & 0x7;
			pkt = (1 << (5+pkt*2));
			DMD_I2C_Read ( DMD_BANK_MAIN(param->devid) , DMD_MAIN_BERMON1P , &rd );
			perr = rd * 65536;
			DMD_I2C_Read ( DMD_BANK_MAIN(param->devid) , DMD_MAIN_BERMON2P , &rd );
			perr += rd * 256;
			DMD_I2C_Read ( DMD_BANK_MAIN(param->devid) , DMD_MAIN_BERMON3P , &rd );
			perr += rd ;

			break;
		default:
			//Not Support
				DMD_DBG_TRACE( "ERROR : Not Supported System");
				ret = DMD_E_ERROR;
	}

	param->info[DMD_E_INFO_PERRNUM] = perr;
	param->info[DMD_E_INFO_PACKETNUM]  = pkt;
	printf("per is %d\n",perr);
	return ret;
}
/*Start********************************************************************/
/**
 * DMD_cnr
 * 
 * DMD_PARAMETER* param
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_cnr( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	DMD_u8_t	rd;
	DMD_u32_t	cni,cnd;
	DMD_u32_t	cnr;
	DMD_u32_t	x,y;

	switch( param->system ){
		case DMD_E_ATSC:
		case DMD_E_QAMB_64QAM:
		case DMD_E_QAMB_256QAM:
			DMD_I2C_Read( DMD_BANK_USR(param->devid) , DMD_USR_CNMON1 , &rd );
			x = 0x100 * rd;
			DMD_I2C_Read( DMD_BANK_USR(param->devid) , DMD_USR_CNMON2 , &rd );
			x += rd;
			DMD_I2C_Read( DMD_BANK_USR(param->devid) , DMD_USR_CNMON3 , &rd );
			y = 0x100 * rd;
			DMD_I2C_Read( DMD_BANK_USR(param->devid) , DMD_USR_CNMON4 , &rd );
			y += rd;
			if( param->system == DMD_E_ATSC )
			{
				//after EQ
				cnr = 4634 - log10_easy( y );
				
			}
			else
			{
				if( y != 0  )
					cnr = log10_easy( (8*x) / y );
				else
					cnr = 0;

			}
			break;
		default:
			cnr = 0;
			//Not Support
				DMD_DBG_TRACE( "ERROR : Not Supported System");
				ret = DMD_E_ERROR;
	}
	if( cnr < 0 ) cnr = 0;
	cni = (DMD_u32_t ) cnr / 100;
	cnd = (DMD_u32_t ) cnr % 100;
	param->info[DMD_E_INFO_CNR_INT] = cni;
	param->info[DMD_E_INFO_CNR_DEC] = cnd;
	printf("snr is %d\n",cni);
	return ret;
}
/*Start********************************************************************/
/**
 * DMD_errorfree
 * 
 * DMD_PARAMETER* param
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_errorfree( DMD_PARAMETER* param )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	DMD_u32_t	erfree = 0;
	DMD_u8_t	rd;

	switch( param->system ){
		case DMD_E_ATSC:
		case DMD_E_QAMB_64QAM:
		case DMD_E_QAMB_256QAM:
			DMD_I2C_Read( DMD_BANK_MAIN(param->devid) , DMD_MAIN_BERFLGP , &rd );
			if( rd & 0x10 )
				erfree = 0;
			else
				erfree = 1;
		  break;
		default:
			//Not Support
				DMD_DBG_TRACE( "ERROR : Not Supported System");
				ret = DMD_E_ERROR;
	}

	param->info[DMD_E_INFO_ERRORFREE] = erfree;
	
	return ret;
}

/*Start********************************************************************/
/**
 * DMD_info_atsc
 * 
 * DMD_PARAMETER *param
 * DMD_u32_t id
 * DMD_u32_t allflag
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_info_atsc( DMD_PARAMETER *param,DMD_u32_t id,DMD_u32_t allflag )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	

	return ret;
}
/*Start********************************************************************/
/**
 * DMD_info_qam
 * 
 * DMD_PARAMETER *param
 * DMD_u32_t id
 * DMD_u32_t allflag
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_info_qam( DMD_PARAMETER *param,DMD_u32_t id,DMD_u32_t allflag )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	

	return ret;
}
/*Start********************************************************************/
/**
 * DMD_info_analog
 * 
 * DMD_PARAMETER *param
 * DMD_u32_t id
 * DMD_u32_t allflag
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_info_analog( DMD_PARAMETER *param,DMD_u32_t id,DMD_u32_t allflag )
{
	DMD_ERROR_t	ret = DMD_E_OK;
	

	return ret;
}

/*Start********************************************************************/
/**
 * DMD_scan_vq
 * 
 * DMD_PARAMETER *param
 * 
 * @return
 *      success : True
 *      fail    : False
 *************************************************************************/
DMD_u32_t DMD_scan_vq( DMD_PARAMETER *param )
{
	DMD_u32_t	st;
	DMD_u32_t	now;
	DMD_u32_t	timeout = 1500 ;
	DMD_u8_t	rd;
	DMD_ERROR_t	ret = DMD_E_ERROR;
	int count=20;
	DMD_timer(&st);
	param->info[DMD_E_INFO_LOCK] = DMD_E_LOCK_ERROR;	


	//timeout setting & etc.
	switch( param->system )
	{
	case DMD_E_ATSC:
		timeout = 650;
		break;
	default:
		timeout = 2000;
		break;
	}

	do
	{
		DMD_I2C_Read( DMD_BANK_MAIN(param->devid)  ,DMD_MAIN_STSMON1	 , &rd );
		//VQ LOCK
		if( rd & 0x1 )
		{
			param->info[DMD_E_INFO_LOCK] = DMD_E_LOCKED;	
			ret = DMD_E_OK;
			break;
		}
		DMD_wait(100);			//wait 1ms
		DMD_timer( &now );
		count --;
	}
	while( count);//now - st < timeout );	//timeout
	return ret;
}

/* **************************************************** */
/* Register  */
/* **************************************************** */
DMD_u32_t DMD_send_regset( DMD_u32_t	devid , DMD_u8_t* regset )
{
	DMD_u32_t	i;
	for(i=0;;)
	{
		if( regset[i] == 0xff ) break;
		DMD_I2C_Write( DMD_I2C_SLAVE[devid][regset[i]] , regset[i+1] , regset[i+2] );
		i=i+3;
	}
	return DMD_E_OK;
}

/* Set TS parallel or serial mode. If you want to set to parallel, make sure your hardware connection no problem */
DMD_u32_t DMD_set_ts_output(DMD_PARAMETER *param, DMD_TSOUT_MODE ts_out_mode, DMD_TSCLK_POLARITY ts_clk_polarity)
{  
	switch(ts_out_mode)
       {
			case DMD_E_TSOUT_PARALLEL_BRTG_MODE:
				//TS parallel  	
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET2 , 0xc1 );
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_GPSET1 , 0xff );//troy.wang, 121010, for parallel data setting
				break;

			case DMD_E_TSOUT_PARALLEL_SMOOTH_MODE:
				//TS parallel conventional mode : smooth 
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET2 , 0x01 );
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_GPSET1 , 0xff );//troy.wang, 121010, for parallel data setting
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET1 , 0x00 ); //address : 0x01
				break;
				
			case DMD_E_TSOUT_PARALLEL_BURST_GATED_MODE:
				//TS parallel conventional mode : burst(gated mode)
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET2 , 0x01 );
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_GPSET1 , 0xff );//troy.wang, 121010, for parallel data setting
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET1 , 0x02 ); 
				break;

			case DMD_E_TSOUT_PARALLEL_BURST_CONTINUOUS_MODE:
				//TS parallel conventional mode : burst(continous mode)
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET2 , 0x01 );
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_GPSET1 , 0xff );//troy.wang, 121010, for parallel data setting
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET1 , 0x0a ); 
				break;

			case DMD_E_TSOUT_SERIAL_BRTG_MODE:					
				//TS serial BRTG mode 
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET2 , 0xc0 );
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_GPSET1 , 0xC0 ); //set error output
				break;

			case DMD_E_TSOUT_SERIAL_SMOOTH_MODE:
				//TS serial conventional mode : smooth 
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET2 , 0x00 );
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_GPSET1 , 0xC0 ); //set error output
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET1 , 0x00 ); //address : 0x01
				break;

			case DMD_E_TSOUT_SERIAL_BURST_GATED_MODE:
				//TS serial conventional mode : burst(gated mode)
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET2 , 0x00 );
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_GPSET1 , 0xC0 ); //set error output
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET1 , 0x02 ); 
				break;

			case DMD_E_TSOUT_SERIAL_BURST_CONTINUOUS_MODE:
				//TS serial conventional mode : burst(continous mode)
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET2 , 0x00 );
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_GPSET1 , 0xC0 ); //set error output
				DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_CPOSET1 , 0x0a ); 
				break;
				
			default:
				return DMD_E_ERROR;
	    }

	if (ts_clk_polarity == DMD_E_TSCLK_POLARITY_INVERSE)
	{		
		if (ts_out_mode & 0x10) //serial clk
		{
		DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_INVSET, 0x01 );
		}
		else //parallel clk
		{
		DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_INVSET, 0x02 );
		}
	}
	return DMD_E_OK;
}

/* **************************************************** */
/*!	Set Register setting for I2C communication mode between DMD  and Tuner*/
/* **************************************************** */
//Troy.wangyx 20120801, set once is enough 
DMD_u32_t	DMD_device_set_TCB_mode( DMD_PARAMETER* param)
{
	DMD_ERROR_t	ret;
	DMD_u8_t	bank;
	DMD_u8_t	tcbcom,tcbadr,tcbset;
	
	ret = DMD_E_ERROR;

	bank = DMD_BANK_MAIN(param->devid);
	tcbcom=DMD_MAIN_TCBCOM;
	tcbadr=DMD_MAIN_TCBADR;
	tcbset=DMD_MAIN_TCBSET;

	/* Set TCB Through Mode */
	ret  = DMD_I2C_MaskWrite( bank , tcbset , 0x7f , 0x53 );
	ret |= DMD_I2C_Write( bank , tcbadr , 0x00 );

	return ret;

}

/* **************************************************** */
/*!	Set Register  to enhance "Single static echo  MINUS delay "'s performance */
/* **************************************************** */
DMD_u32_t DMD_device_set_echo_enhance(DMD_PARAMETER* param, DMD_ECHO_PERFORMANCE_SET echo_out )
{    
	switch(echo_out)
       {
	       case DMD_E_ENHANCE_MINUS_ECHO_DELAY_DEFAULT:
			// DMD_MAIN_VEQSET2	0x80   |  field test passed result	
			DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_VEQSET2 , 0x80 );
			break;
		case DMD_E_ENHANCE_MINUS_ECHO_DELAY_NO_SIDEEFFECT:
			//DMD_MAIN_VEQSET2		0xE0    | Lab test. enhance MINUS echo dalay performance, no side effect
			DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_VEQSET2 , 0xE0 );
			break;
		case DMD_E_ENHANCE_MINUS_ECHO_DELAY_HAS_SIDEEFFECT:
			//DMD_MAIN_VEQSET2		0xF0    |	FIFOSET:0xE3 Lab test. enhance MINUS echo dalay performance, has side effect on CCI(NTSC interface)
			DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_VEQSET2 , 0xF0 );
			break;
	default:
		return DMD_E_ERROR;
	}
	return DMD_E_OK;
}

/* **************************************************** */
/* CCI(NTSC interface) ipmrovement  */
/* To detect analog inference signal and do relative EQ setting  */
/* 
[effect]
To improve CCI performance about 2dB. 

[Notice]
1. Pls. add this function after signal locked and decoding begins , once a second, once a second
2. Always do read register MAIN_NRFRDAT
*/
/* **************************************************** */
DMD_u32_t DMD_device_cochan_interface_detect( DMD_PARAMETER *param )
{
    DMD_u8_t rd_co, rd_eq;

	// only affect 8VSB 
	if (( param->system == DMD_E_ATSC )&&(param->info[DMD_E_INFO_LOCK] == DMD_E_LOCKED ))
	{		
		// detect co-channel status
		DMD_I2C_Read ( DMD_BANK_MAIN(param->devid) , DMD_MAIN_NRFRDAT, &rd_co );

		// EQ setting check 
		DMD_I2C_Read( DMD_BANK_MAIN(param->devid) , DMD_MAIN_VEQSET21, &rd_eq );
       printf(" --- Pana ATSC --- Co-ch status[%x], EQset[%x] \n ", rd_co,rd_eq);
			    
		//strong analog Interface signal detected and correponding EQ mode not set yet 
		if ( (( rd_co & 0x20) == 0x20) && ( rd_eq != 0xe5) )
		{
		DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_VEQSET21, 0xE5 );
		}
		else if ((( rd_co & 0x20) == 0)&& ( rd_eq != 0xe9))// normal status 
		{
		DMD_I2C_Write( DMD_BANK_MAIN(param->devid) , DMD_MAIN_VEQSET21, 0xE9 );//for other test items' performance 
		}  

	} 

   return DMD_E_OK;
}



/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmitnr.c
**	
**		This module implements the necessary functions for driving the NMI 120,13x  Chipset.
**		Porting driver for NTK 310/216. It can be used as an example for other boards
**
** 
*******************************************************************************/

#include "nmicfg.h"
#include "nmicmn.h"
#include "nmiver.h"
#include "nmiioctl.h"


/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmiport.c
**
**		This module implements the necessary functions OS and bus function for
**		NM131 tuner.
**
**
*******************************************************************************/


//#include <string.h>
#include "nmicmn.h"
#include "nmi_tuner_os.h"
#include "nmiver.h"
#include "nmiioctl.h"

/******************************************************************************
**
**	ASIC Helper Functions
**
*******************************************************************************/
void print_32hex(uint32_t value)
{
	//nmi_tuner_os_log("%x:",(uint16_t)(value>>16));
	//nmi_tuner_os_log("%4x\n",(uint16_t)((value<<16)>>16));
}

void nmi_memset(void *pu8Des, uint8_t value, uint32_t len)
{
	nmi_tuner_os_memset(pu8Des,value,len);
	/*
	uint32_t i=0;
	uint8_t* tmpDes = pu8Des;
	for(i=0;i<len;i++)
	{
		tmpDes[i] = value;
	}
	*/
}


void nmi_memcpy(void *pu8Des, void *pu8Src, uint32_t len)
{
	nmi_tuner_os_memcpy(pu8Des,pu8Src,len);
	/*
	uint32_t i=0;
	uint8_t* tmpDes = pu8Des;
	uint8_t* tmpSrc = pu8Src;
	for(i=0;i<len;i++)
	{
		tmpDes[i] = tmpSrc[i];
	}
	*/
}

int nmi_abs(int x)
{
    return x < 0 ? -x : x;
}

static void nmi_log(char *str)
{
	//nmi_tuner_os_log(str);
}

static void nmi_delay(uint32_t msec)
{
	nmi_tuner_os_delay(msec);
}

static uint32_t nmi_get_tick(void)
{
	uint32_t time;
	//time = MsOS_GetSystemTime();
	time = nmi_tuner_os_get_tick();
	return time;
}

static int nmi_bus_read(uint8_t DeviceAddr, uint8_t * pArray, uint32_t count)
{
	return nmi_tuner_os_bus_read(DeviceAddr, pArray, count);
}


static int nmi_bus_write(uint8_t DeviceAddr, uint8_t * pArray, uint32_t count)
{
	return nmi_tuner_os_bus_write(DeviceAddr, pArray, count);
}

static unsigned long nmi_bus_get_read_blksz(void)
{
	//return UINT_MAX;
	return 0;
}

static unsigned long nmi_bus_get_write_blksz(void)
{
	return 14;
}



/******************************************************************************
**
**	Includes Driver Tnr Functions
**
*******************************************************************************/

/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmiioctl_cmn.c
**
**	Driver Tnr Functions
**
*******************************************************************************/

//#include <stdio.h>
//#include <stdarg.h>

/*******************************************************************************
**
**	Driver Global Variables
**
*******************************************************************************/
#include "nmitypes.h"

 
static int already_init = 0;

typedef struct {
	uint32_t frequency;
	int rssi;
	int overth;
} tNmiFmSeek;


typedef struct {
	uint32_t						chipid;
	tTnrVtbl						tnr;
	tNmiTvStd					standard;
	tNmiFmSeek 				fmSeek[7];
} tNmiDrv;

static tNmiDrv  drv;

/******************************************************************************
**
**	Driver Debug Defines
**
*******************************************************************************/

uint32_t dflag = N_ERR | N_INFO;

#if 0
static void dPrint(unsigned long flag, char *fmt, ...)
{
	return;
}
#endif
/******************************************************************************
**
**	Driver Tnr Functions
**
*******************************************************************************/

static void nmi_tnr_deinit(void)
{

	already_init = 0;
	return;
}

static int nmi_tnr_init(void *pv)
{
	tNmiDrv *pd = &drv;
	int result	= NMI_S_OK;
	tNmiIn inp;
	tTnrInit *p = (tTnrInit *)pv;

	if (!already_init) {
		nmi_memset((void *)pd, 0, sizeof(tNmiDrv));


		/**
			Asic Init
		**/
		nmi_memset((void *)&inp, 0, sizeof(tNmiIn));		
		inp.xo							= p->xo;
		inp.ldobypass				= p->ldobypass;
		inp.ai2c                    = p->i2c_adr;
		inp.zone                    = N_ERR|N_INFO;
		inp.hlp.c.write				= nmi_bus_write;
		inp.hlp.c.read				= nmi_bus_read;
		inp.hlp.c.getreadblksz      = nmi_bus_get_read_blksz;
		inp.hlp.c.getwriteblksz     = nmi_bus_get_write_blksz;
		inp.hlp.delay				= nmi_delay;
		inp.hlp.gettick				= nmi_get_tick;
		inp.hlp.log					= nmi_log;
		inp.init_bus_only           = p->init_bus_only;
		inp.xo_out_en               = p->xo_out_en;
		inp.ltGain                  = p->ltGain;
		inp.disableOneKhzShift      = p->disableOneKhzShift;

		nmi_common_init(&inp, &pd->tnr);
		//nmi_tuner_os_log("nmi_tnr_init 2\n");
		//print_32hex(pd->tnr.init);
 
		/**
			initialize chip
		**/
		if (pd->tnr.init() < 0) {
			//dPrint(N_ERR, "[NMI] Failed to initialize chip...\n");
			nmi_tuner_os_log("[NMI] Failed to initialize chip...\n");
			result = NMI_E_CHIP_INIT;
			goto _fail_;
		}

		/**
		    Get chip id
		**/
		pd->chipid = pd->tnr.getchipid();
		nmi_tuner_os_log("chip id:");
		print_32hex(pd->chipid);
		already_init = 1;
	}

_fail_:

	return result;
}

static uint32_t nmi_tnr_get_chipid(void)
{
	tNmiDrv *pd = &drv;
	uint32_t chipid;

	chipid = pd->tnr.getchipid();

	return chipid;
}


static int nmi_tnr_tune(tTnrTune *p)
{
	int retVal;
	tNmiDrv *pd = &drv;

	retVal = pd->tnr.tune(p);

	return retVal;
}

static void nmi_tnr_reset_demod(void)
{
	tNmiDrv *pd = &drv;

	pd->tnr.reset();
}

static void nmi_tnr_no_lock_reset(void)
{
	tNmiDrv *pd = &drv;

	pd->tnr.nolockreset();
}

static void nmi_tnr_get_status(tTnrStatus *p)
{
	tNmiDrv *pd = &drv;

	pd->tnr.getstatus(&p->ds);
	pd->tnr.getlna(&p->gain);
}

static int nmi_tnr_scan(tTnrScan * p)
{
	tNmiDrv *pd = &drv;

	pd->tnr.scan(p);

	return 0;
}
#if !defined NTK_ENV
static void nmi_tnr_set_volume(tTnrVolume *p)
{
	tNmiDrv *pd = &drv;
	pd->tnr.setvolume(p->vol);
}

static void nmi_tnr_get_volume(tTnrVolume *p)
{
	tNmiDrv *pd = &drv;

	p->vol = pd->tnr.getvolume();
}

static void nmi_tnr_set_mute(tTnrMute *p)
{
	tNmiDrv *pd = &drv;
	pd->tnr.setmute(p->mute);
}

static void nmi_tnr_get_mute(tTnrMute *p)
{
	tNmiDrv *pd = &drv;

	p->mute = pd->tnr.getmute();
}
#endif

static void nmi_tnr_lt_ctrl(tTnrLtCtrl *p)
{
	tNmiDrv *pd = &drv;

	pd->tnr.ltctrl(p->enable);
}

#if !defined NTK_ENV
static void nmi_tnr_set_video_amp(tTnrVideoAmp *p)
{
	tNmiDrv *pd = &drv;
	pd->tnr.setvideoamp(p->amp);
}

static void nmi_tnr_get_video_amp(tTnrVideoAmp *p)
{
	tNmiDrv *pd = &drv;

	p->amp = pd->tnr.getvideoamp();
}

static void nmi_tnr_set_brightness(tTnrBrt *p)
{
	tNmiDrv *pd = &drv;
	pd->tnr.setbrightness(p->brt);
}

static void nmi_tnr_get_brightness(tTnrBrt *p)
{
	tNmiDrv *pd = &drv;

	p->brt = pd->tnr.getbrightness();
}
#endif

static unsigned int nmi_svn_rev_to_int(const char *r)
{
	const unsigned int count = 
		r[11] == ' ' ? 1
		: r[12] == ' ' ? 10
		: r[13] == ' ' ? 100
		: r[14] == ' ' ? 1000
		: r[15] == ' ' ? 10000
		: r[16] == ' ' ? 100000
		: r[17] == ' ' ? 1000000
		: r[18] == ' ' ? 10000000
		: r[19] == ' ' ? 100000000
		: 0;

	if ( r == NULL || r[0] == '\0' || r[10] == '\0' || r[11] == '\0' || r[12] == '\0' || r[13] == '\0' )
		return 0;

	return
		(r[11] == ' ' ? 0 : (r[11]-'0') * (count/10) +
		(r[12] == ' ' ? 0 : (r[12]-'0') * (count/100) + 
		(r[13] == ' ' ? 0 : (r[13]-'0') * (count/1000) + 
		(r[14] == ' ' ? 0 : (r[14]-'0') * (count/10000) + 
		(r[15] == ' ' ? 0 : (r[15]-'0') * (count/100000) +
		(r[16] == ' ' ? 0 : (r[16]-'0') * (count/1000000) +
		(r[17] == ' ' ? 0 : (r[17]-'0') * (count/10000000) +
		(r[18] == ' ' ? 0 : (r[18]-'0') * (count/100000000) +
		(r[19] == ' ' ? 0 : (r[19]-'0') * (count/1000000000) +
		0)))))))));
}


static void nmi_tnr_get_version(tNmiDriverVer * vp)
{	
	vp->major = ASIC_MAJOR_VER;
	vp->minor = ASIC_MINOR_VER;
	vp->rev1 = ASIC_REV1;
	vp->rev2 = ASIC_REV2;
	vp->buildrev = nmi_svn_rev_to_int(BUILD_REV);
}

 int32_t nmi_drv_ctl(uint32_t codeswan,void* inp)
{
	tNmiDrv *pd = &drv;
	int result = 0;

	switch (codeswan) {
	case NMI_DRV_INIT:
		result = nmi_tnr_init(inp);
		break;
	case NMI_DRV_DEINIT:
		nmi_tnr_deinit();
		break;
	case NMI_DRV_VERSION:		
		nmi_tnr_get_version((tNmiDriverVer *)inp);		
		break;
	case NMI_DRV_GET_CHIPID:
		*((uint32_t *)inp) = nmi_tnr_get_chipid();	
		break;
	case NMI_DRV_TUNE:
		result = nmi_tnr_tune((tTnrTune *)inp);
			break;
	case NMI_DRV_SCAN:
		result = nmi_tnr_scan((tTnrScan*)inp);
		break;
	case NMI_DRV_SET_SCAN_TH:
		pd->tnr.setscanlevel(*((int*)inp));
		break;
	case NMI_DRV_RESET_DEMOD:
		nmi_tnr_reset_demod();
		break;
	case NMI_DRV_RESET_DEMOD_MOSAIC:
		nmi_tnr_no_lock_reset();
		break;			
	case NMI_DRV_GET_STATUS:
		nmi_tnr_get_status((tTnrStatus *)inp);
		break;
	case NMI_DRV_SET_LNA_GAIN:
		pd->tnr.setgain((tNmiLnaGain)inp);
		break;
	case NMI_DRV_INVERT_SPECTRUM:
		pd->tnr.invertspectum(*((int*)inp));
		break;
	case NMI_DRV_SET_IF_OUTPUT_VOLTAGE:
		pd->tnr.setifoutvoltage(*((int*)inp));
		break;	
	case NMI_DRV_SLEEP_LT:
	case NMI_DRV_SLEEP:
		pd->tnr.sleep_lt();
		break;	
	case NMI_DRV_WAKE_UP_LT:
		pd->tnr.wake_up_lt();
		break;
	case NMI_DRV_LT_CTRL:
		nmi_tnr_lt_ctrl((tTnrLtCtrl *)inp);
		break;
	default:
		result = -1;
		break;
	}
	
	return result;
}

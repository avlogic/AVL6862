//#include <string.h>

#include "nmi_tuner_os.h"

#include "nmiioctl.h"
//#include "MsCommon.h"
//#include "HbCommon.h"
//#include "Board.h"
//#include"drvTuner.h"
//#include"MsOS.h"
//#include "apiDigiTuner.h"
//#include "Board.h"
//#if(FRONTEND_TUNER_TYPE == TUNER_NMI120 ||FRONTEND_TUNER_TYPE == TUNER_MIXER)

#include "nmituner_api.h"

#define NMI_USE_TUNER_INTERFACE  1

#ifdef NMI_USE_TUNER_INTERFACE
#include "nmi_tuner_interface.h"
#endif
//extern void wReg8(uint8_t adr, uint8_t val);
//extern void wReg32(uint16_t adr, uint32_t val);

//extern void DumpRegs();

typedef enum
{
	NMI_DEMOD_TYPE_NTSC,
	NMI_DEMOD_TYPE_256QAM,
	NMI_DEMOD_TYPE_64QAM,
	NMI_DEMOD_TYPE_8VSB,
	NMI_DEMOD_TYPE_NUM
} NMI_MODU_TYPE;

#define XO_OUT_EN 1
#define XO_OUT_DIS 0

#if 1
#define NMI_LOG		nmi_tuner_os_log
#else
#define NMI_LOG		nmi_tuner_os_log
#endif
static uint32_t chipid;

//#ifdef DVBT2_STYLE
#if 0
FEMode TunerMode;
void MDrv_Tuner_SetTunerMode(FEMode mode)
{
     TunerMode.Fetype = mode.Fetype;
}

EN_TUNE_TYPE MDrv_Tuner_GetTuneType(void)
{
     return TunerMode.Fetype;
}
#endif
#if 0
static void NMI120_ShowAsicPara(void)
{
	tTnrStatus status;

	if(nmi_drv_ctl(NMI_DRV_GET_STATUS,(void*)(&status))<0)
	{
		//nmi_tuner_os_log("[NMI ASIC] Failed to get status!!!\n");
		return	;
	}
	
	//nmi_tuner_os_log("[NMI ASIC] AGC Lock: %d\n", status.ds.agclock);
	//nmi_tuner_os_log("[NMI ASIC] Dagc: %2.3f\n", ((double)status.ds.dagc)/(1ul << 15));

	//nmi_tuner_os_log("[NMI ASIC] Rssi: %3.2f dBm\n", status.gain.rssix100/100.0);
	
	//nmi_tuner_os_log("[NMI ASIC] Lna Code: %02x\n", (uint8_t)status.gain.lnacode);
	//nmi_tuner_os_log("[NMI ASIC] Lna Gain: %2.1f\n", status.gain.lnadbx100/100.0);

	//nmi_tuner_os_log("[NMI ASIC] BBLI Code: %02x\n", (uint8_t)status.gain.bblicode);
	//nmi_tuner_os_log("[NMI ASIC] BBLI Cain: %2.1f\n", status.gain.bblidbx100/100.0);

	return ;
}
#endif

uint8_t MDrv_Tuner_Init(void)
{
	tTnrInit cfg;
	uint32_t ret;
	cfg.ldobypass = 0;
	cfg.xo = 24;
	cfg.i2c_adr = 0x67;//0xce>>1;
	cfg.init_bus_only = 0;
	cfg.xo_out_en = 0;
	cfg.disableOneKhzShift = FALSE;
	cfg.ltGain = nLtGainMax;


	ret = Nmi_Tuner_Interface_init_chip(&cfg);
	//ret = Nmi_Tuner_Interface_init_chip(&cfg);
	if(ret == FALSE)
	{
		//nmi_tuner_os_log(">> NMI_DRV_INIT failed. result = %d\n", result);
		//nmi_tuner_os_log("Nmi_Tuner_Interface_init_chip error\n");
		return FALSE;
	}
	else
	{
		chipid = Nmi_Tuner_Interface_Get_ChipID();		
		
		return TRUE;
	}
}

static tNmiSwrfliArg SwrfliArg=
{
	.bbli_upper = 0xf0,
	.bbli_lower = 0xea,
	.bbli_upper2 = 0xf0,
	.bbli_lower2 = 0xef,
	.bbli_avg_enable = 0,
	.bbli_avg_weight = 1*32768,
	.dagc_avg_enable = 0,
	.dagc_avg_weight = 1*32768,
	.use_dagc = 0,
	.dagc_upper = 4*32768,
	.dagc_lower = 2*32768,
	.lnaTarget = 9, //wood, --
	//.lnaTarget = 0xa,//wood, ++
};
uint8_t MDrv_NMI120_Tuner_SetTuner(uint32_t dwFreq, uint8_t ucBw,uint8_t type)//MHZ
{
	tNMI_TUNE_PARAM param;
	uint8_t ret = FALSE;

	nmi_tuner_os_memset((void *)&param, 0x00, sizeof(tNMI_TUNE_PARAM));

	param.freq = dwFreq;
	param.if_freq = 5000000;
	param.dacsel =  nDac2;
	param.if_freq_invert = FALSE;
	param.freq_invert = FALSE;
	param.ucBw = ucBw;
	param.output = nIf;
	param.freq_invert = FALSE;
	param.tvstd       = type;	//NMI_DVBT;

	if(param.tvstd == NMI_DVBT || param.tvstd == NMI_DVBT2)
		nmi_tuner_os_memcpy((void*)&param.poll_param,&SwrfliArg,sizeof(tNmiSwrfliArg));
	if(param.tvstd == NMI_DVBT2)
	{
		param.scan_aci[0] = 0x07;//6mhz
		param.scan_aci[1] = 0x0a;//7mhz
		param.scan_aci[2] = 0x04;//8mhz
		
		param.play_aci[0] = 0x07;//6mhz
		param.play_aci[1] = 0x0a;//7mhz
		param.play_aci[2] = 0x04;//8mhz
	}	
	else if(param.tvstd == NMI_DVBT || param.tvstd == NMI_DVBC)
	{
		param.scan_aci[0] = 0x07;//6mhz
		param.scan_aci[1] = 0x0a;//7mhz
		param.scan_aci[2] = 0x04;//8mhz
		
		param.play_aci[0] = 0x07;//6mhz
		param.play_aci[1] = 0x0a;//7mhz
		param.play_aci[2] = 0x04;//8mhz
	}	
	else
	{
		param.scan_aci[0] = 0x01;//6mhz
		param.scan_aci[1] = 0x03;//7mhz
		param.scan_aci[2] = 0x05;//8mhz
		
		param.play_aci[0] = 0x01;//6mhz
		param.play_aci[1] = 0x03;//7mhz
		param.play_aci[2] = 0x05;//8mhz
	}
	nmi_tuner_os_log("tune: rf=%d, if=%d, bw=%d, tvstd=%d\n", param.freq, param.if_freq, param.ucBw, param.tvstd);
	ret = Nmi_Tuner_Interface_Tuning(&param);

	Nmi_Tuner_Interface_Wreg(0x35,0x18);//0x18 ~ 0x10, 0x5f~ 0x50
	Nmi_Tuner_Interface_Wreg(0x164,0x400);

	if((param.freq >666000000-1500000) &&(param.freq <666000000+1500000))
	{
		Nmi_Tuner_Interface_Wreg(0x05,0x5);
	}

	return ret;

}

MS_U32 MDrv_Tuner_SetTuner(MS_U32 dwFreq, MS_U8 ucBw)
{
	uint8_t type=NMI_ATSC_8VSB;
/*	if(E_TUNE_TYPE_T2 == MDrv_Tuner_GetTuneType())
		type = NMI_DVBT2;
	else
		type = NMI_DVBT;*/
	nmi_tuner_os_log("120 set tuner dwFreq(%d),ucBw(%d)\n",dwFreq,ucBw);
	return MDrv_NMI120_Tuner_SetTuner(dwFreq,ucBw,type);
}


static int ABS(int x)
{
    return x < 0 ? -x : x;
}

int NMI120_GetRSSI(uint8_t outputChoice)
{
	int rssi;

	rssi = (int)Nmi_Tuner_Interface_GetRSSI();
	if (1 == outputChoice) // for percent value
	{
		if ((rssi<=0) && (rssi>=-100))
		{
			rssi = ABS(rssi);
            if(rssi>85)
                rssi = 0;
            else if(rssi > 45)
                rssi = (85-rssi)*100/40;
            else
                rssi = 100;
		}
		else
		{
			if (rssi > 0)
			{
				rssi = 100;
			}

			if (rssi < -100)
			{
				rssi = 0;
			}
		}
	}
    nmi_tuner_os_log("[NMI]SSI=%d\n",rssi);
	return rssi;
}


uint8_t MDrv_Tuner_PowerOnOff(uint8_t bPowerOn)
{
    if(bPowerOn==FALSE)
    {
        //if need loop through, passing LOOP_THROUGH,else passing SIGLE_IN
		//tTnrStatus status;

		//nmi_drv_ctl(NMI_DRV_SLEEP,NULL);
		Nmi_Tuner_Interface_Sleep_Lt();
    }
    return TRUE;
}




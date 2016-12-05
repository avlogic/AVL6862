#include "nmi_tuner_interface.h"
#include "nmi_tuner_os.h"

typedef struct {
	uint32_t chip_id;
	/*bSleep:if TRUE,it shows the tuner has sleep.You must stop the mosaic_reset operation.
	   		 if FALSE,it shows the tuner is working.The mosaic_reset operation can work */
	bool_t    bSleep;
	/*bInit:This param indicates the tuner is inited,when retune the tuner,you don't need to wait for 4s*/
	bool_t	 bInit;
	bool_t   btuned;//FALSE no tune
	bool_t   bdemodlock;
	bool_t   btaskopen;
	uint32_t xo_hz;//晶体频率
	tNMI_TUNE_PARAM currentparam;
	tNMI_TUNE_PARAM orgparam;
	uint32_t  jiffies;
	uint32_t  mutex;
} tNmiInterfaceGlobal;

#define NML120_MAX_RESET_WAITTIME 4000
#define NML120_MIN_OFFSET 			250//660 //KHz
#define NML120_MAX_OFFSET 			450//660 //KHz
#define TUNER_NML120_MAX_TIME 3000 //ms

static tNmiInterfaceGlobal gChipNowInfo;

extern uint8_t rReg8(uint8_t adr);
extern uint32_t rReg32(uint16_t adr);
extern void wReg8(uint8_t adr, uint8_t val);
extern void wReg32(uint16_t adr, uint32_t val);
extern void nmi_demod_aci_config(uint8_t aci);
extern void nm131_demod_T_config(tNmiTvStd std,bool_t enable);
extern void nm120_rfli_track(tNmiSwrfliArg *p);

static bool_t  Nmi_Tuner_Interface_poll(void);
static uint8_t Nmi_Tuner_reset_Agc_Dvbc(uint8_t init);//this is for dvbc for test
static uint8_t Nmi_Tuner_reset_Agc_Dvbt(uint8_t init);//this is for dvbt for test
static void    Nmi_Tuner_Interface_SelfWreg(uint32_t addr,uint32_t value);
static uint32_t  Nmi_Tuner_Interface_SelfRreg(uint32_t addr);
static int16_t Nmi_Tuner_Interface_SelfGetRSSI(void);
static void Nmi_Tuner_Interface_SelfWake_Up_Lt(void);
static void Nmi_Play_Aci(void);
static void Nmi_Scan_Aci(void);
	
uint8_t Nmi_Tuner_Interface_CallBack(void)
{		
	if(!nmi_tuner_os_get_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("Obtain NMI Mutex failed\n");
		return 0;
	}
	nmi_tuner_os_log("Nmi_Tuner_Interface_CallBack\n");
	if(gChipNowInfo.currentparam.tvstd == NMI_DVBT || gChipNowInfo.currentparam.tvstd == NMI_DVBT2 || gChipNowInfo.currentparam.tvstd == NMI_DTMB) 
	{
		Nmi_Tuner_Interface_poll();
		
	} else if(gChipNowInfo.currentparam.tvstd == NMI_DVBC) 
	{
		Nmi_Tuner_reset_Agc_Dvbc(FALSE);
	}

	if(nmi_tuner_os_get_demod_lock()==TRUE)
		Nmi_Play_Aci();
		
	nmi_tuner_os_release_mutex(&gChipNowInfo.mutex);
	return 1;
	
}

static void Nmi_Tuner_Task(void)
{
	nmi_tuner_os_log("Nmi_Tuner_Task\n");
	while(gChipNowInfo.bInit)
	{
		if(TRUE == gChipNowInfo.bSleep)
		{
			nmi_tuner_os_log("NMI task break for sleep!!!!\n");
			break;
		}
		
		nmi_tuner_os_delay(260);//ms,
		
		if(gChipNowInfo.bInit != TRUE)
			break;

		Nmi_Tuner_Interface_CallBack() ;


	}
}


static int Nmi_Tuner_Interface_ABS(int x)
{
    return x < 0 ? -x : x;
}


static void Nmi_Play_Aci(void)
{
	// add for diff demod
	if(gChipNowInfo.currentparam.tvstd >=NMI_ATSC_8VSB)//all DTV
	{
		nmi_tuner_os_change_iic_mode(TRUE);
		if(gChipNowInfo.currentparam.ucBw == 6)
			nmi_demod_aci_config(gChipNowInfo.currentparam.play_aci[0]);
		if(gChipNowInfo.currentparam.ucBw == 7)
			nmi_demod_aci_config(gChipNowInfo.currentparam.play_aci[1]);
		if(gChipNowInfo.currentparam.ucBw == 8)
			nmi_demod_aci_config(gChipNowInfo.currentparam.play_aci[2]);

		nmi_tuner_os_change_iic_mode(FALSE);
	}	
}

static void Nmi_Scan_Aci(void)
{
	// add for diff demod
	if(gChipNowInfo.currentparam.tvstd >=NMI_ATSC_8VSB)
	{
		if(gChipNowInfo.currentparam.ucBw == 6)
			nmi_demod_aci_config(gChipNowInfo.currentparam.scan_aci[0]);
		if(gChipNowInfo.currentparam.ucBw == 7)
			nmi_demod_aci_config(gChipNowInfo.currentparam.scan_aci[1]);
		if(gChipNowInfo.currentparam.ucBw == 8)
			nmi_demod_aci_config(gChipNowInfo.currentparam.scan_aci[2]);
	}	
}

#if 0
static uint8_t Nmi_Tuner_Interface_Reset_Moaic(void)
{
	if(gChipNowInfo.bSleep == TRUE)
		return TRUE;
	
	nmi_drv_ctl(NMI_DRV_RESET_DEMOD_MOSAIC,NULL);
	return TRUE;
}
#endif

static uint8_t Nmi_Tuner_Interface_LoopThrough(void)
{
	//int result;
	tTnrInit cfg;
	tTnrLtCtrl ltCtl;
	//tTnrStatus status;

	nmi_tuner_os_memset((void *)&cfg, 0, sizeof(tTnrInit));

	ltCtl.enable = TRUE;
	if(nmi_drv_ctl(NMI_DRV_LT_CTRL, (void *)&ltCtl)<0)
	{
		return FALSE;
	}

	nmi_drv_ctl(NMI_DRV_WAKE_UP_LT, NULL);
	return TRUE;
}


uint32_t Nmi_Tuner_Interface_init_chip(tTnrInit* pcfg)
{
   	 int result;
	nmi_tuner_os_log("Nmi_Tuner_Interface_init_chip\n");
	nmi_tuner_os_memset((void *)&gChipNowInfo, 0, sizeof(tNmiInterfaceGlobal));
	
	nmi_tuner_os_chip_power_on();
	nmi_tuner_os_chip_enable();

	nmi_tuner_os_log("before nmi_tuner_os_create_task\n");
	if(1 != nmi_tuner_os_create_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("nmi_tuner_os_create_mutex false" );

		goto _NMI_MUTEXT_FALSE;
	}

	
	result = nmi_drv_ctl(NMI_DRV_INIT, (void *)pcfg);
	
	if(result < 0)
	{
		nmi_tuner_os_log(">> NMI_DRV_INIT failed. result = %d\n", result);
		gChipNowInfo.bInit = FALSE;
		goto _NMI_INIT_FALSE;
	}
	//Leo else
	{
		uint32_t chipid;
		tNmiDriverVer ver;

		nmi_drv_ctl(NMI_DRV_VERSION, &ver);
		nmi_tuner_os_log("[NMI] driver version (%d.%d.%d.%d) build %d\n", ver.major, ver.minor, ver.rev1, ver.rev2, ver.buildrev);

		nmi_drv_ctl(NMI_DRV_GET_CHIPID, &chipid);
		nmi_tuner_os_log("[NMI] Chip ID:");
		nmi_tuner_os_log("%4x:",(uint16_t)(chipid>>16));
		nmi_tuner_os_log("%4x\n",(uint16_t)((chipid<<16)>>16));

		gChipNowInfo.chip_id =  chipid;
		gChipNowInfo.xo_hz = pcfg->xo * 1000000;
	}

	gChipNowInfo.bSleep = FALSE;
	gChipNowInfo.bInit = 	TRUE;
	gChipNowInfo.btuned = FALSE;
	gChipNowInfo.jiffies = nmi_tuner_os_get_tick();
       
	Nmi_Tuner_Interface_LoopThrough();
	
	nmi_tuner_os_create_task((NmiTaskEntry)Nmi_Tuner_Task);
	gChipNowInfo.btaskopen = TRUE;
	return TRUE;
_NMI_INIT_FALSE:
	nmi_tuner_os_delete_task();
	gChipNowInfo.btaskopen = FALSE;
_NMI_MUTEXT_FALSE:
	gChipNowInfo.bSleep = FALSE;
	gChipNowInfo.bInit = 	FALSE;
	gChipNowInfo.btuned = FALSE;

	nmi_tuner_os_chip_power_off();
	return FALSE;
}
uint32_t Nmi_Tuner_Interface_deinit_chip(void)
{
	nmi_tuner_os_log("Nmi_Tuner_Interface_deinit_chip\n");
	if(!nmi_tuner_os_get_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("Obtain NMI Mutex failed\n");
		return FALSE;
	}
	nmi_tuner_os_delete_task();
	nmi_tuner_os_delete_mutex(&gChipNowInfo.mutex);
	
	nmi_drv_ctl(NMI_DRV_DEINIT, NULL);
	nmi_tuner_os_chip_power_off();
	gChipNowInfo.bSleep = FALSE;
	gChipNowInfo.bInit =   FALSE;
	gChipNowInfo.btuned = FALSE;
	gChipNowInfo.btaskopen = FALSE;
	
	nmi_tuner_os_release_mutex(&gChipNowInfo.mutex);
	return TRUE;
}

static uint8_t Nmi_Tuner_Interface_SelfTune(tNMI_TUNE_PARAM* param)
{
	uint8_t ret = TRUE;
	tTnrTune tune;

	nmi_tuner_os_memset((void *)&tune, 0x00, sizeof(tTnrTune));
	tune.aif = param->aif;
	tune.dacSel = param->dacsel;
	tune.is_stereo = param->is_stereo;
	tune.output = param->output;
	tune.rf = param->freq;
	tune.vif = param->if_freq;
	tune.rfinvert = param->freq_invert;
     
	nmi_tuner_os_log("Nmi_Tuner_Interface_SelfTune:\n");	
	nmi_tuner_os_log("tune.aif:%d\n",tune.aif);	
	nmi_tuner_os_log("tune.dacSel:%d\n",tune.dacSel);	
	nmi_tuner_os_log("tune.is_stereo:%d\n",tune.is_stereo);	
	nmi_tuner_os_log("tune.output:%d\n",tune.output);	
	nmi_tuner_os_log("tune.rf:%d\n",tune.rf);	
	nmi_tuner_os_log("tune.vif:%d\n",tune.vif);	
	nmi_tuner_os_log("tune.rfinvert:%d\n",tune.rfinvert);	
	nmi_tuner_os_log("param->ucBw:%d\n",param->ucBw);	
	nmi_tuner_os_log("param->tvstd:%d\n",param->tvstd);	

	switch(param->tvstd)
	{
		case NMI_NTSC: 
			{
			    //add codes	
				tune.std = nNTSC;
			}
			break;
		case NMI_PAL_G: 
			{
			        tune.std = nPAL_G;	
			}
			break;
		case NMI_PAL_M: 
			{
			    tune.std = nPAL_M;		
			}
			break;
		case NMI_PAL_N: 
			{
			    tune.std = nPAL_N;	
			}
			break;
		case NMI_PAL_K: 
			{
			    tune.std = nPAL_K;	
			}
			break;
		case NMI_PAL_L: 
			{
			    tune.std = nPAL_L;	
			}
			break;
		case NMI_PAL_D: 
			{
			     tune.std = nPAL_L;//-----------
			}
			break;
		case NMI_SECAM_L: 
			{
			    tune.std = nSECAM_L;	
			}
			break;
		case NMI_SECAM_B: 
			{
			   tune.std = nSECAM_B;		
			}
			break;
		case NMI_SECAM_D: 
			{
			    tune.std = nSECAM_D;		
			}
			break;
		case NMI_ATSC_8VSB: 
		case NMI_ATSC_64QAM: 
		case NMI_ATSC_256QAM: 
			{
			    //add codes
				nmi_tuner_os_log("mode NMI_ATSC %d\n",param->ucBw);
			    	switch(param->ucBw)
				{
					case 0:
					case 6: tune.std = nDTV_6; break;
					case 1:
					case 7: tune.std = nDTV_7; break;
					case 2:
					case 8:
					default:
					tune.std = nDTV_8; break;
				}
			}
			break;
		case NMI_DVBT: 
		case NMI_DVBT2: 
			{
			    //add codes	
				switch(param->ucBw)
				{
					case 0:
					case 6: tune.std = nDTV_6; break;
					case 1:
					case 7: tune.std = nDTV_7; break;
					case 2:
					case 8:
					default:
					tune.std = nDTV_8; break;
				}
			}
			break;
		case NMI_DVBC:
			{
				//add codes	
				//swan add for dvbc
				uint32_t tmp_freq;
				tmp_freq = (tune.rf+250000)/1000000;
				if(tmp_freq%3 != 0)
				{
					tune.rf = tmp_freq*1000000 + 1000;
				}

				switch(param->ucBw)
				{
					case 0:
					case 6: tune.std = nDTV_6; break;
					case 1:
					case 7: tune.std = nDTV_7; break;
					case 2:
					case 8:
					default:
					tune.std = nDTV_8; break;
				}
			}
			break;
		case NMI_DTMB: 
			{
			    //add codes	
				switch(param->ucBw)
				{
					case 0:
					case 6: tune.std = nDTV_6; break;
					case 1:
					case 7: tune.std = nDTV_7; break;
					case 2:
					case 8:
					default:
					tune.std = nDTV_8; break;
				}
			}
			break;
		case NMI_ISDBT: 
			{
			    //add codes	
			    switch(param->ucBw)
				{
					case 0:
					case 6: tune.std = nDTV_6; break;
					case 1:
					case 7: tune.std = nDTV_7; break;
					case 2:
					case 8:
					default:
					tune.std = nDTV_8; break;
				}
				//tune.std = nDTV_6;
			}
			break;
		default: 
			{
			    //add codes	
			}
			break;
	}

	tune.nmistd = param->tvstd;
	if(param->tvstd <= NMI_SECAM_D &&( param->output == nCvbsSif || param->output == nCvbsBBAud))
	{
		if(nmi_drv_ctl(NMI_DRV_TUNE, &tune) == 0)
		{
			nmi_tuner_os_log("\n tuneFail......\n");
			ret =  FALSE;
		}
	}
      else
      	{
      		if(nmi_drv_ctl(NMI_DRV_TUNE, &tune) < 0)
		{
			nmi_tuner_os_log("\n tuneFail......\n");
			ret = FALSE;
		}
		nmi_tuner_os_log("\n NMI_DRV_TUNE vif %d , rf %d,std %d\n",tune.vif,tune.rf,tune.std);	
		if(param->if_freq_invert == TRUE)
		{
			int spectrum = 1;
			nmi_drv_ctl(NMI_DRV_INVERT_SPECTRUM, (void *)(&spectrum));
			nmi_tuner_os_log("\n swan if invert \n");
		}
      	}
	if(param->tvstd == NMI_DVBC)//swan 2012_7_4  
	{
		uint32_t tmp_freq;
		tmp_freq = (tune.rf+180000)/1000000;//250000 maybe too big, swan 2012-11-16
		if(tmp_freq>650)
		{
			Nmi_Tuner_Interface_SelfWreg(0x5, 0x85);
			Nmi_Tuner_Interface_SelfWreg(0x1c, 0x1e);
			Nmi_Tuner_Interface_SelfWreg(0x3b, 0x40);
			nmi_tuner_os_log("\n **fre:%d\n", tmp_freq);
		}
		//have test ok
		switch(tmp_freq)
		{
			case 722:
			case 770:
			case 842:
			case 794:
			case 746:
			case 698:
				{
					Nmi_Tuner_Interface_SelfWreg(0x5, 0x5);
				}
				break;
			default:
				{
				}
		}
		
		/*
		if(tune.rf>600000000)//not test code
		{
			uint32_t xiebo;
			
			xiebo = (tune.rf/gChipNowInfo.xo_hz)*gChipNowInfo.xo_hz;
			if((tune.std == nDTV_6 && (xiebo < tune.rf+3000000) && (xiebo > tune.rf-3000000)) ||
			   (tune.std == nDTV_7 && (xiebo < tune.rf+3500000) && (xiebo > tune.rf-3500000)) ||
			   (tune.std == nDTV_8 && (xiebo < tune.rf+4000000) && (xiebo > tune.rf-4000000)))
			{
				Nmi_Tuner_Interface_SelfWreg(0x5, 0x5);
			}
		}
		*/
		Nmi_Tuner_Interface_SelfWreg(0x0e,0x25);
		Nmi_Tuner_reset_Agc_Dvbc(TRUE);
	}
	
	if(param->tvstd == NMI_DTMB)//swan 2012_6_1  honestar for C/N
	{
		Nmi_Tuner_Interface_SelfWreg(0x0e,0x25);
	}
	
	if(param->tvstd == NMI_DVBT ||param->tvstd == NMI_DVBT2)
	{
		if((tune.rf >191500000-1500000) &&(tune.rf <191500000+1500000))
		{
			Nmi_Tuner_Interface_SelfWreg(0x05,0x87);
		}
		if(((tune.rf> 810000000-1500000) && (tune.rf< 810000000 + 1500000))||
		   ((tune.rf> 594000000-1500000) && (tune.rf< 594000000 + 1500000)))
	   	 {
			if(Nmi_Tuner_Interface_SelfRreg(0x05) != 0x85)
				Nmi_Tuner_Interface_SelfWreg(0x05,0x85);
			nmi_tuner_os_log("poll 0x05~~%x\n",Nmi_Tuner_Interface_SelfRreg(0x05) );
	   	 }
	         if(tune.std == nDTV_7)
	         {
	       		Nmi_Tuner_Interface_SelfWreg(0x2e,0x56);
	         }
		//Nmi_Tuner_Interface_SelfWreg(0x0e,0x25);//for CN
		
		if(tune.rf==-1)//just save the code swan
			Nmi_Tuner_reset_Agc_Dvbt(TRUE);
	}

	// add for diff demod  swan 2012-9-25
	
//-----------------------------------toshiba_isdbt
	if(param->tvstd == NMI_ISDBT)
	{
		//nmi_demod_aci_config(0x01);

		if(param->freq >=93000000 && param->freq <510000000)
		{
			Nmi_Tuner_Interface_SelfWreg(0x37,0x9c);
		}
		else
		{
			Nmi_Tuner_Interface_SelfWreg(0x37,0x84);
		}

		if(param->freq >=261000000 && param->freq <412000000)
		{
			Nmi_Tuner_Interface_SelfWreg(0x36,0x54);
		}
		else
		{
			Nmi_Tuner_Interface_SelfWreg(0x36,0x7c);
		}

		switch(param->freq-143*1000)
		{
			//case channel 7,18,2226,30,34,38    //need change these channel to freqs
			//	Nmi_Tuner_Interface_SelfWreg(0x5,0x5);
			//	break;
			default:
				Nmi_Tuner_Interface_SelfWreg(0x5,0x85);
		}


	}
//----------------------------------------
// add for diff demod  wang 2012-11-2
	nmi_tuner_os_memcpy((void*)&gChipNowInfo.currentparam,param,sizeof(tNMI_TUNE_PARAM));
	Nmi_Scan_Aci();	
	
	nmi_tuner_os_log("gChipNowInfo.currentparam.freq(%d) \n",gChipNowInfo.currentparam.freq);
	nmi_tuner_os_log("Nmi_Tuner_Interface_SelfTune end\n");
	return ret;
}


uint8_t Nmi_Tuner_Interface_Tuning(tNMI_TUNE_PARAM* param)
{
	uint8_t ret;
	nmi_tuner_os_log("Nmi_Tuner_Interface_Tuning\n");
	if(gChipNowInfo.bInit != TRUE)
	{
		return FALSE;
	}
	
	if(!nmi_tuner_os_get_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("Obtain NMI Mutex failed\n");
		return FALSE;
	}

	if(gChipNowInfo.orgparam.freq == param->freq || param->freq >900000000)
	{
		ret = FALSE;
		goto _TUNE_EXIT_;
	}
	gChipNowInfo.bdemodlock = FALSE;
	ret = Nmi_Tuner_Interface_SelfTune(param);
	gChipNowInfo.btuned = TRUE;
	nmi_tuner_os_memcpy((void*)&gChipNowInfo.orgparam,param,sizeof(tNMI_TUNE_PARAM));

_TUNE_EXIT_:
	nmi_tuner_os_release_mutex(&gChipNowInfo.mutex);
	nmi_tuner_os_log("Nmi_Tuner_Interface_Tuning end\n");
	return ret;
}

int Nmi_Tuner_Interface_GetLockStatus(void)
{
	tTnrStatus status;
	nmi_tuner_os_log("Nmi_Tuner_Interface_GetLockStatus\n");
	if(!nmi_tuner_os_get_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("Obtain NMI Mutex failed\n");
		return 0;
	}
	if(nmi_drv_ctl(NMI_DRV_GET_STATUS,&status)<0)
	{
		return 0;
	}
	nmi_tuner_os_release_mutex(&gChipNowInfo.mutex);
	return status.ds.agclock;
}

static int16_t Nmi_Tuner_Interface_SelfGetRSSI(void)
{
	int16_t rssi;
	tTnrStatus status;
	nmi_tuner_os_log("Nmi_Tuner_Interface_SelfGetRSSI\n");

	if(nmi_drv_ctl(NMI_DRV_GET_STATUS,&status)<0)
	{
		return 0;
	}

	rssi = (int16_t)status.gain.rssix100/100;
    nmi_tuner_os_log("LNAGAIN:%02x\n",(uint8_t)status.gain.lnacode);
	return rssi;
}

int16_t Nmi_Tuner_Interface_GetRSSI(void)
{
	int16_t rssi;
	nmi_tuner_os_log("Nmi_Tuner_Interface_GetRSSI\n");
	if(!nmi_tuner_os_get_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("Obtain NMI Mutex failed\n");
		return FALSE;
	}
	
	rssi = Nmi_Tuner_Interface_SelfGetRSSI();
	
	nmi_tuner_os_release_mutex(&gChipNowInfo.mutex);
	return rssi;
}

void Nmi_Tuner_Interface_Sleep_Lt(void)
{
	if(!nmi_tuner_os_get_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("Obtain NMI Mutex failed\n");
		return ;
	}
	nmi_tuner_os_log("Nmi_Tuner_Interface_Sleep_Lt ~~~~~~~~~~~~\n");
	gChipNowInfo.bSleep = TRUE;
	nmi_tuner_os_delete_task();
	gChipNowInfo.btaskopen = FALSE;
	nmi_tuner_os_log("nmi_tuner_os_delete_task~~~~~~~~111111111111~~~~~~~\n");

	nmi_drv_ctl(NMI_DRV_SLEEP_LT, NULL);
	
	nmi_tuner_os_release_mutex(&gChipNowInfo.mutex);
}

static void Nmi_Tuner_Interface_SelfWake_Up_Lt(void)
{
	gChipNowInfo.bSleep = FALSE;
	nmi_drv_ctl(NMI_DRV_WAKE_UP_LT, NULL);
	if(gChipNowInfo.btaskopen != TRUE)
		nmi_tuner_os_create_task((NmiTaskEntry)Nmi_Tuner_Task);

}

void Nmi_Tuner_Interface_Wake_Up_Lt(void)
{
	if(!nmi_tuner_os_get_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("Obtain NMI Mutex failed\n");
		return ;
	}
	if(TRUE == gChipNowInfo.bSleep)
	{
		Nmi_Tuner_Interface_SelfWake_Up_Lt();
	}
	nmi_tuner_os_release_mutex(&gChipNowInfo.mutex);
}

static uint8_t Nmi_Tuner_reset_Agc_Dvbc(uint8_t init)//this is for dvbc for test
{
	static int16_t nmi_rssi_last=-100;
	int16_t rssi_now;
	static uint8_t run_count=0;
	nmi_tuner_os_change_iic_mode(TRUE);
	rssi_now = Nmi_Tuner_Interface_SelfGetRSSI();//

	nmi_tuner_os_log("Nmi_Tuner_reset_Agc_Dvbc\n");
	nmi_tuner_os_log("reset, rssi_now:%d \n", rssi_now);
	
	if(init==TRUE)//初始化
	{
		nmi_rssi_last=-100;
		Nmi_Tuner_Interface_SelfWreg(0x30,0xbf);
		Nmi_Tuner_Interface_SelfWreg(0x2e,0xef);
		nmi_tuner_os_log("Nmi_Tuner_reset_Agc init\n");
	}
	
	run_count++;
	if(run_count<3)//记录次数，不用每次都调用。
	{
		nmi_tuner_os_log("Nmi_Tuner_reset_Agc_Dvbc %d return\n", run_count);
		goto _dvbc_exit_;
	}
	run_count=0;
	nmi_tuner_os_log("Nmi_Tuner_reset_Agc_Dvbc run\n");
	
	if(rssi_now>=-50)
	{
		goto _dvbc_exit_;
	}
	else if(rssi_now<-50 && rssi_now>=-60)
	{
		if(Nmi_Tuner_Interface_ABS(rssi_now - nmi_rssi_last)>4)
		{
			nmi_rssi_last = rssi_now;
			Nmi_Tuner_Interface_SelfWreg(0x2b,0x27);
			//nmi_tuner_os_delay(5);
			Nmi_Tuner_Interface_SelfWreg(0x2b,0x91);
		}
	}
	else if(rssi_now<-60)
	{
		if(Nmi_Tuner_Interface_ABS(rssi_now - nmi_rssi_last)>2)
		{
			nmi_rssi_last = rssi_now;
			Nmi_Tuner_Interface_SelfWreg(0x2b,0x27);
			//nmi_tuner_os_delay(5);
			Nmi_Tuner_Interface_SelfWreg(0x2b,0x91);
		}
	}
	
_dvbc_exit_:
	nmi_tuner_os_change_iic_mode(FALSE);
	return TRUE;
}

static uint8_t Nmi_Tuner_reset_Agc_Dvbt(uint8_t init)//this is for dvbt for test
{
	static int16_t rssi_last = -100;
	int16_t rssi_now;
	uint8_t value;
	nmi_tuner_os_log("Nmi_Tuner_reset_Agc_Dvbt\n");	
	if(init==TRUE)//初始化
	{
		rssi_last=-100;
		nmi_tuner_os_log("Nmi_Tuner_reset_Agc_Dvbt init\n");
	}
    if((gChipNowInfo.currentparam.freq<666000000+250000) &&
		(gChipNowInfo.currentparam.freq>666000000-250000))
    {
		rssi_now = Nmi_Tuner_Interface_SelfGetRSSI();
		if(rssi_now<-64 &&rssi_now>-77)
		{
			if(Nmi_Tuner_Interface_ABS(rssi_now-rssi_last)>3)
			{
				value = Nmi_Tuner_Interface_SelfRreg(0x2b);
				rssi_last = rssi_now;
				if(value!=0x2a)
					Nmi_Tuner_Interface_SelfWreg(0x2b,0x2a);
			}
		}
		else
		{
			if(Nmi_Tuner_Interface_ABS(rssi_now-rssi_last)>3)
			{
				value = Nmi_Tuner_Interface_SelfRreg(0x2b);
				rssi_last = rssi_now;
				if(value!=0x91)
					Nmi_Tuner_Interface_SelfWreg(0x2b,0x91);
			}
		}
    }
	else
	{
		nm120_rfli_track(&gChipNowInfo.currentparam.poll_param);
	}
	return FALSE;
}




static bool_t  Nmi_Tuner_Interface_poll(void)
{
	nmi_tuner_os_log("Nmi_Tuner_Interface_poll ~~~\n");

	if(!gChipNowInfo.btuned)
		return FALSE;

	nmi_tuner_os_change_iic_mode(TRUE);
	nm120_rfli_track(&gChipNowInfo.currentparam.poll_param);
	nmi_tuner_os_log("poll reg0x2b = %x\n",Nmi_Tuner_Interface_SelfRreg(0x2b));
	nmi_tuner_os_change_iic_mode(FALSE);

	return TRUE;
}

void  Nmi_Tuner_Interface_Wreg(uint32_t addr,uint32_t value)
{
	if(!nmi_tuner_os_get_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("Obtain NMI Mutex failed\n");
		return;
	}
	//nmi_tuner_os_change_iic_mode(TRUE);
	Nmi_Tuner_Interface_SelfWreg(addr,value);
	//nmi_tuner_os_change_iic_mode(FALSE);
	nmi_tuner_os_release_mutex(&gChipNowInfo.mutex);
}


uint32_t  Nmi_Tuner_Interface_Rreg(uint32_t addr)
{
	uint32_t ret = 0;
	if(!nmi_tuner_os_get_mutex(&gChipNowInfo.mutex))
	{
		nmi_tuner_os_log("Obtain NMI Mutex failed\n");
		return 0;
	}
	//nmi_tuner_os_change_iic_mode(TRUE);
	ret = Nmi_Tuner_Interface_SelfRreg(addr);
	//nmi_tuner_os_change_iic_mode(FALSE);
	nmi_tuner_os_release_mutex(&gChipNowInfo.mutex);
	return ret;
}

uint32_t Nmi_Tuner_Interface_Get_ChipID(void)
{
	if(gChipNowInfo.bInit != TRUE)
	{
		return 0;
	}
	return gChipNowInfo.chip_id;
}

void Nmi_Tuner_Interface_Demod_Lock(void)
{
	if(gChipNowInfo.bInit != TRUE)
	{
		return;
	}
	if(gChipNowInfo.currentparam.tvstd == NMI_ATSC_64QAM || gChipNowInfo.currentparam.tvstd == NMI_ATSC_256QAM)
	{
		if(gChipNowInfo.bdemodlock == FALSE)
		{
			nmi_demod_aci_config(0x01);
			gChipNowInfo.bdemodlock = TRUE;
		}
	}
}

static void  Nmi_Tuner_Interface_SelfWreg(uint32_t addr,uint32_t value)
{
	if(addr<0x100)
	{
		wReg8((uint8_t)addr,(uint8_t)value);
	}
	else
	{
		wReg32(addr,value);
	}
}


static uint32_t  Nmi_Tuner_Interface_SelfRreg(uint32_t addr)
{
	if(addr<0x100)
	{
		return rReg8((uint8_t)addr);
	}
	else
	{
		return rReg32(addr);
	}
}



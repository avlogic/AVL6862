/*****************************************************************
**
**  Copyright (C) 2009 Amlogic,Inc.
**  All rights reserved
**        Filename : avlfrontend.c
**
**  comment:
**        Driver for AVL6211 demodulator
**  author :
**	    Shijie.Rong@amlogic
**  version :
**	    v1.0	 12/3/30
*****************************************************************/

/*
    Driver for AVL6211 demodulator
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/kthread.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#ifdef ARC_700
#include <asm/arch/am_regs.h>
#else
//#include <mach/am_regs.h>
#endif
#include <linux/i2c.h>
//#include <asm/gpio.h>
#include "avlfrontend.h"
#include "LockSignal_Api.h"


#include <linux/version.h>
#if LINUX_VERSION_CODE>= KERNEL_VERSION(3,10,0)
#include <linux/amlogic/aml_gpio_consumer.h>
#endif

#include <aml_fe.h>

#if 1
#define pr_dbg	printk
//#define pr_dbg(fmt, args...) printk( KERN_DEBUG"DVB: " fmt, ## args)
#else
#define pr_dbg(fmt, args...)
#endif

#define pr_error(fmt, args...) printk( KERN_ERR"DVB: " fmt, ## args)

#define M_TUNERMAXLPF_100KHZ	440
#define bs_start_freq			950				//The start RF frequency, 950MHz
#define bs_stop_freq			2150			//The stop RF frequency, 2150MHz
#define Blindscan_Mode   AVL_DVBSx_BS_Slow_Mode	//The Blind scan mode.	AVL_DVBSx_BS_Fast_Mode = 0,AVL_DVBSx_BS_Slow_Mode = 1

extern struct AVL_Tuner *avl6211pTuner;
extern struct AVL_DVBSx_Chip * pAVLChip_all;
AVL_semaphore blindscanSem;
static int blindstart=0;
struct dvb_frontend *fe_use = NULL;
struct aml_fe_dev *cur_dvbdev = NULL;

static char *device_name = "avl6211";

struct aml_fe_dev * avl6211_get_cur_dev(void)
{
	return cur_dvbdev;
}

static int AVL6211_Reset(int reset_gpio)
{
#if LINUX_VERSION_CODE>= KERNEL_VERSION(3,10,0)

	amlogic_gpio_request(reset_gpio,device_name);
	amlogic_gpio_direction_output(reset_gpio, 0, device_name);
	msleep(300);
	amlogic_gpio_direction_output(reset_gpio, 1, device_name);

#else

	gpio_out(reset_gpio, 0);
	msleep(300);
	gpio_out(reset_gpio, 1);

#endif
	return 0;
}

static int AVL6211_Lnb_Power_Ctrl(int lnb)
{
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;

	if(1 == lnb)
		r=AVL_DVBSx_IDiseqc_SetLNB1Out(1, pAVLChip_all);	//set LNB1_PIN60 1: Hight 
	else
		r=AVL_DVBSx_IDiseqc_SetLNB1Out(0, pAVLChip_all);	//set LNB1_PIN60 1: Low 	
	
	return r;
}

static int AVL6211_Tuner_Power_Ctrl(int tunerpwr)
{
	if(1==tunerpwr)	AVL_DVBSx_IBase_SetGPIOVal(0, pAVLChip_all);
	else				AVL_DVBSx_IBase_SetGPIOVal(0, pAVLChip_all);
	return 0;
}

#if 0
static int AVL6211_Ant_Overload_Ctrl(void)
{
	return 0;//gpio_get_value(frontend_ANT);
}

#endif

static int	AVL6211_Diseqc_Reset_Overload(struct dvb_frontend* fe)
{
		return 0;
}


static int	AVL6211_Diseqc_Send_Master_Cmd(struct dvb_frontend* fe, struct dvb_diseqc_master_cmd* cmd)
{
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	AVL_uchar ucData[8];
	int j=100;
	struct AVL_DVBSx_Diseqc_TxStatus TxStatus;
	int i;
	pr_dbg("msg_len is %d,\n data is",cmd->msg_len);
	for(i=0;i<cmd->msg_len;i++){
		ucData[i]=cmd->msg[i];
		printk("%x ",cmd->msg[i]);
	}
	
	r=AVL_DVBSx_IDiseqc_SendModulationData(ucData, cmd->msg_len, pAVLChip_all);
	if(r != AVL_DVBSx_EC_OK)
	{
		pr_dbg("AVL_DVBSx_IDiseqc_SendModulationData failed !\n");
	}
	else
	{
		do
		{
			j--;
			AVL_DVBSx_IBSP_Delay(1);
			r= AVL_DVBSx_IDiseqc_GetTxStatus(&TxStatus, pAVLChip_all);
		}while((TxStatus.m_TxDone != 1)&&j);
		if(r ==AVL_DVBSx_EC_OK )
		{

		}
		else
		{
			pr_dbg("AVL_DVBSx_IDiseqc_SendModulationData Err. !\n");
		}		
	}
	return r;
}

static int	AVL6211_Diseqc_Recv_Slave_Reply(struct dvb_frontend* fe, struct dvb_diseqc_slave_reply* reply)
{
		return 0;
}

static int	AVL6211_Diseqc_Send_Burst(struct dvb_frontend* fe, fe_sec_mini_cmd_t minicmd)
{
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
 	struct AVL_DVBSx_Diseqc_TxStatus sTxStatus;
	AVL_uchar ucTone = 0;
	int i=100;
	#define TONE_COUNT				8
	if(minicmd == SEC_MINI_A)
		ucTone = 1;
	else if(minicmd == SEC_MINI_B)
		ucTone = 0;
	else ;

  	r = AVL_DVBSx_IDiseqc_SendTone(ucTone, TONE_COUNT, pAVLChip_all);
	if(AVL_DVBSx_EC_OK != r)
	{
		pr_dbg("\rSend tone %d --- Fail!\n",ucTone);
	}
	else
	{
	    do
	    {
	    	i--;
			AVL_DVBSx_IBSP_Delay(1);
		    r =AVL_DVBSx_IDiseqc_GetTxStatus(&sTxStatus, pAVLChip_all);   //Get current status of the Diseqc transmitter data FIFO.
	    }
	    while((1 != sTxStatus.m_TxDone)&&i);			//Wait until operation finished.
	    if(AVL_DVBSx_EC_OK != r)
	    {
		    pr_dbg("\rOutput tone %d --- Fail!\n",ucTone);
	    }
	}
	return (r);

}

static int	AVL6211_Set_Tone(struct dvb_frontend* fe, fe_sec_tone_mode_t tone)
{
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	AVL_uchar  uc22kOn = 0;
	if(SEC_TONE_ON == tone)
		uc22kOn = 1;
	else if(SEC_TONE_OFF == tone)
		uc22kOn = 0;
	else ;
	if(uc22kOn)
	{
		r=AVL_DVBSx_IDiseqc_StartContinuous(pAVLChip_all);
	}else{
		r=AVL_DVBSx_IDiseqc_StopContinuous(pAVLChip_all);
	}
	if(r!=AVL_DVBSx_EC_OK)
	{
		pr_dbg("[AVL6211_22K_Control] Err:0x%x\n",r);
	}	
	
	return r;
	
}

static int	AVL6211_Set_Voltage(struct dvb_frontend* fe, fe_sec_voltage_t voltage)
{
	AVL_DVBSx_ErrorCode r=AVL_DVBSx_EC_OK;	
	AVL_uchar nValue = 1;
	if(voltage == SEC_VOLTAGE_OFF){
		AVL6211_Lnb_Power_Ctrl(0);//lnb power off
		return 0;
	}

	if(voltage ==  SEC_VOLTAGE_13)
		nValue = 0;
	else if(voltage ==SEC_VOLTAGE_18)
		nValue = 1;
	else;	
	
	AVL6211_Lnb_Power_Ctrl(1);//lnb power on

	if(1==nValue)
		r=AVL_DVBSx_IDiseqc_SetLNBOut(1, pAVLChip_all);	//set LNB0_PIN59 1: Hight 
	else
		r=AVL_DVBSx_IDiseqc_SetLNBOut(0, pAVLChip_all);	//set LNB0_PIN59 1: Low 

	return r;
}

static int	AVL6211_Enable_High_Lnb_Voltage(struct dvb_frontend* fe, long arg)
{
	return 0;
}
#if 1
#if 1
static void AVL6211_DumpSetting(struct AVL_DVBSx_BlindScanAPI_Setting *pBSsetting)
{
	printk(KERN_INFO "AVL6211_DumpSetting+++\n");

	printk(KERN_INFO "m_uiScan_Min_Symbolrate_MHz %d\n", pBSsetting->m_uiScan_Min_Symbolrate_MHz);
	printk(KERN_INFO "m_uiScan_Max_Symbolrate_MHz %d\n", pBSsetting->m_uiScan_Max_Symbolrate_MHz);
	printk(KERN_INFO "m_uiScan_Start_Freq_MHz %d\n", pBSsetting->m_uiScan_Start_Freq_MHz);
	printk(KERN_INFO "m_uiScan_Stop_Freq_MHz %d\n", pBSsetting->m_uiScan_Stop_Freq_MHz);
	printk(KERN_INFO "m_uiScan_Next_Freq_100KHz %d\n", pBSsetting->m_uiScan_Next_Freq_100KHz);
	printk(KERN_INFO "m_uiScan_Progress_Per %d\n", pBSsetting->m_uiScan_Progress_Per);
	printk(KERN_INFO "m_uiScan_Bind_No %d\n", pBSsetting->m_uiScan_Bind_No);
	printk(KERN_INFO "m_uiTuner_MaxLPF_100kHz %d\n", pBSsetting->m_uiTuner_MaxLPF_100kHz);
	printk(KERN_INFO "m_uiScan_Center_Freq_Step_100KHz %d\n", pBSsetting->m_uiScan_Center_Freq_Step_100KHz);
	printk(KERN_INFO "BS_Mode %d\n", pBSsetting->BS_Mode);
	printk(KERN_INFO "m_uiScaning %d\n", pBSsetting->m_uiScaning);
	printk(KERN_INFO "m_uiChannelCount %d\n", pBSsetting->m_uiChannelCount);
	printk(KERN_INFO "m_eSpectrumMode %d\n", pBSsetting->m_eSpectrumMode);

	printk(KERN_INFO "AVL6211_DumpSetting---\n");
	
	return;
}
#endif

struct AVL_DVBSx_BlindScanAPI_Setting BSsetting;
static int dvbs2_blindscan_task(void *p)
{
	struct dvbsx_blindscanpara *pbspara =  (struct dvbsx_blindscanpara *)p ;

	struct dvbsx_blindscanevent bsevent;
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	AVL_uint16	index = 0;
	
	struct AVL_DVBSx_Channel * pChannel;
	

	enum AVL_DVBSx_BlindScanAPI_Status BS_Status;
	struct AVL_DVBSx_BlindScanAPI_Setting * pBSsetting = &BSsetting;
	BS_Status = AVL_DVBSx_BS_Status_Init;

	memset(pBSsetting, 0, sizeof(struct AVL_DVBSx_BlindScanAPI_Setting));

	
	pBSsetting->m_uiScan_Start_Freq_MHz=pbspara->minfrequency/1000;
	pBSsetting->m_uiScan_Stop_Freq_MHz=pbspara->maxfrequency/1000;
	pBSsetting->m_uiScan_Max_Symbolrate_MHz=pbspara->maxSymbolRate/(1000 * 1000);
	pBSsetting->m_uiScan_Min_Symbolrate_MHz=pbspara->minSymbolRate/(1000 * 1000);

	

	while(BS_Status != AVL_DVBSx_BS_Status_Exit)
	{
		if(!blindstart)
		{
			BS_Status = AVL_DVBSx_BS_Status_Cancel;
			printf("AVL_DVBSx_BS_Status_Cancel\n");
		}
		
		printk(KERN_INFO "BS_Status %d blindstart %d\n", BS_Status, blindstart);
		switch(BS_Status)
		{
		
			case AVL_DVBSx_BS_Status_Init:
			{
				printk(KERN_INFO "AVL_DVBSx_BS_Status_Init\n");
				AVL_DVBSx_IBlindScanAPI_Initialize(pBSsetting);//this function set the parameters blind scan process needed.	

				AVL_DVBSx_IBlindScanAPI_SetFreqRange(pBSsetting, bs_start_freq, bs_stop_freq); //Default scan rang is from 950 to 2150. User may call this function to change scan frequency rang.
				AVL_DVBSx_IBlindScanAPI_SetScanMode(pBSsetting, Blindscan_Mode);

				AVL_DVBSx_IBlindScanAPI_SetSpectrumMode(pBSsetting, AVL_DVBSx_Spectrum_Invert); //Default set is AVL_DVBSx_Spectrum_Normal, it must be set correctly according Board HW configuration
				AVL_DVBSx_IBlindScanAPI_SetMaxLPF(pBSsetting, M_TUNERMAXLPF_100KHZ); //Set Tuner max LPF value, this value will difference according tuner type

				BS_Status = AVL_DVBSx_BS_Status_Start;

				AVL6211_DumpSetting(pBSsetting);
				break;
			}
			case AVL_DVBSx_BS_Status_Start: 
			{
				r = AVL_DVBSx_IBlindScanAPI_Start(pAVLChip_all, avl6211pTuner, pBSsetting);
				printk(KERN_INFO "AVL_DVBSx_BS_Status_Start %d\n", r);
				if(AVL_DVBSx_EC_OK != r)
				{
					BS_Status = AVL_DVBSx_BS_Status_Exit;
				}
				else
				{	
					bsevent.status = BLINDSCAN_UPDATESTARTFREQ;
					bsevent.u.m_uistartfreq_khz = avl6211pTuner->m_uiFrequency_100kHz * 100;
					fe_use->ops.blindscan_ops.info.blindscan_callback(fe_use, &bsevent);
					BS_Status = AVL_DVBSx_BS_Status_Wait;
				}
				break;
			}

			case AVL_DVBSx_BS_Status_Wait:		
			{
				r = AVL_DVBSx_IBlindScanAPI_GetCurrentScanStatus(pAVLChip_all, pBSsetting);
				printk(KERN_INFO "AVL_DVBSx_BS_Status_Wait %d %d\n", r, pBSsetting->bsInfo.m_uiChannelCount);
				if(AVL_DVBSx_EC_GeneralFail == r)
				{
					BS_Status = AVL_DVBSx_BS_Status_Exit;
				}
				if(AVL_DVBSx_EC_OK == r)
				{
					BS_Status = AVL_DVBSx_BS_Status_Adjust;
				}
				if(AVL_DVBSx_EC_Running == r)
				{
					AVL_DVBSx_IBSP_Delay(100);
				}
				break;
			}

			case AVL_DVBSx_BS_Status_Adjust:
			{
				r = AVL_DVBSx_IBlindScanAPI_Adjust(pAVLChip_all, pBSsetting);
				printk(KERN_INFO "AVL_DVBSx_BS_Status_Adjust %d\n", r);
				if(AVL_DVBSx_EC_OK != r)
				{
					BS_Status = AVL_DVBSx_BS_Status_Exit;
				}
				BS_Status = AVL_DVBSx_BS_Status_User_Process;
				break;
			}

			case AVL_DVBSx_BS_Status_User_Process:
			{
				printk(KERN_INFO "AVL_DVBSx_BS_Status_User_Process\n");
				//------------Custom code start-------------------
				//customer can add the callback function here such as adding TP information to TP list or lock the TP for parsing PSI
				//Add custom code here; Following code is an example

				/*----- example 1: print Blindscan progress ----*/
				printf(" %2d%% \n", AVL_DVBSx_IBlindscanAPI_GetProgress(pBSsetting)); //display progress Percent of blindscan process

				/*----- example 2: print TP information if found valid TP ----*/
				while(index < pBSsetting->m_uiChannelCount) //display new TP info found in current stage
				{
					pChannel = &pBSsetting->channels[index++];
					printf("	  Ch%2d: RF: %4d SR: %5d ",index, (pChannel->m_uiFrequency_kHz/1000),(pChannel->m_uiSymbolRate_Hz/1000));

					bsevent.status = BLINDSCAN_UPDATERESULTFREQ;
					bsevent.u.parameters.frequency = pChannel->m_uiFrequency_kHz;
					bsevent.u.parameters.u.qpsk.symbol_rate = pChannel->m_uiSymbolRate_Hz;

					fe_use->ops.blindscan_ops.info.blindscan_callback(fe_use, &bsevent);
				}	

				bsevent.status = BLINDSCAN_UPDATEPROCESS;
				bsevent.u.m_uiprogress = AVL_DVBSx_IBlindscanAPI_GetProgress(pBSsetting);
				fe_use->ops.blindscan_ops.info.blindscan_callback(fe_use, &bsevent);
				//------------Custom code end -------------------

				if ((AVL_DVBSx_IBlindscanAPI_GetProgress(pBSsetting) < 100))
					BS_Status = AVL_DVBSx_BS_Status_Start;
				else											
					BS_Status = AVL_DVBSx_BS_Status_WaitExit;
				break;
			}
			case AVL_DVBSx_BS_Status_WaitExit:
			{
				msleep(50);
				break;
			}
			case AVL_DVBSx_BS_Status_Cancel:
			{ 
				r = AVL_DVBSx_IBlindScanAPI_Exit(pAVLChip_all,pBSsetting);
				printk(KERN_INFO "AVL_DVBSx_BS_Status_Cancel %d\n", r);
				BS_Status = AVL_DVBSx_BS_Status_Exit;
				blindstart=2;
				break;
			}
		
			default:
			{
				BS_Status = AVL_DVBSx_BS_Status_Cancel;
				break;
			}
			
		}
	}

	return 0;
}
#endif

static struct task_struct *dvbs2_task;
static int AVL6211_Blindscan_Scan(struct dvb_frontend* fe, struct dvbsx_blindscanpara *pbspara)
{
	printk(KERN_INFO "AVL6211_Blindscan_Scan printk\n");

	AVL_DVBSx_IBSP_WaitSemaphore(&blindscanSem);
	fe_use = fe;
	blindstart=1;
	AVL_DVBSx_IBSP_ReleaseSemaphore(&blindscanSem);

	dvbs2_task = kthread_create(dvbs2_blindscan_task, pbspara, "dvbs2_task");
      if(!dvbs2_task){
     	printk("Unable to start dvbs2 thread.\n");
     	dvbs2_task = NULL;
      return -1;
      }
	  wake_up_process(dvbs2_task);
	  return 0;
}

static int AVL6211_Blindscan_Cancel(struct dvb_frontend* fe)
{
		blindstart=0;
		printk(KERN_INFO "AVL6211_Blindscan_Cancel\n");
		while(2!=blindstart){
				pr_dbg("wait for scan exit\n");
				msleep(100);
		}
		/*call do_exit() directly*/
		dvbs2_task = NULL;
		fe_use = NULL;
		return 0;
}

static int initflag=-1;

static int AVL6211_Read_Status(struct dvb_frontend *fe, fe_status_t * status)
{
	unsigned char s=0;
	unsigned int i;
//printk("Get status blindstart:%d.\n",blindstart);
for(i=0;i<10;i++)
{

	if(1==blindstart)
	     s=1;
         else
	   s=AVL6211_GETLockStatus();

	if(s==1)
	{
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
		break;
	}
	else
	{
		*status = FE_TIMEDOUT;
	}
	msleep(10);
}
	return  0;
}

static int AVL6211_Read_Ber(struct dvb_frontend *fe, u32 * ber)
{
	if(1==blindstart)
		return 0;
	*ber=AVL6211_GETBer();
	return 0;
}

static int AVL6211_Read_Signal_Strength(struct dvb_frontend *fe, u16 *strength)
{
	if(1==blindstart)
		return 0;
	*strength=AVL_Get_Level_Percent(pAVLChip_all);
//	*strength=AVL6211_GETSignalLevel();
	return 0;
}

static int AVL6211_Read_Snr(struct dvb_frontend *fe, u16 * snr)
{
	if(1==blindstart)
		return 0;
	*snr=AVL_Get_Quality_Percent(pAVLChip_all);
	//*snr=AVL6211_GETSnr();
	return 0;
}

static int AVL6211_Read_Ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	ucblocks=NULL;
	return 0;
}

static int AVL6211_Set_Frontend(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	int async_ret = 0;
	int lockstatus = 0;
	int waittime=150;//3 
	struct avl6211_state *state = fe->demodulator_priv;
	struct AVL_DVBSx_Channel Channel;
	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	
	pr_dbg("avl6211 set frontend=>frequency=%d,symbol_rate=%d\r\n",c->frequency,c->symbol_rate);

	if(initflag!=0)
	{
		pr_dbg("[%s] avl6211 init fail\n",__FUNCTION__);
		return 0;	
	}
//	printk("[AVL6211_Set_Frontend],blindstart is %d\n",blindstart);

	if(1==blindstart)
		return 0;
	AVL_DVBSx_IBSP_WaitSemaphore(&blindscanSem);
	if((850000>c->frequency)||(c->frequency>2300000))
	{
			c->frequency =945000;
			pr_dbg("freq is out of range,force to set 945000khz\n");
	}
	
	avl6211pTuner->m_uiFrequency_100kHz=c->frequency/100;
//	avl6211pTuner->m_uiFrequency_100kHz=15000;
//	printk("avl6211pTuner m_uiFrequency_100kHz is %d",avl6211pTuner->m_uiFrequency_100kHz);
	
	/* r = CPU_Halt(pAVLChip_all);
	if(AVL_DVBSx_EC_OK != r)
	{
		printf("CPU halt failed !\n");
		return (r);
	}*/

	//Change the value defined by macro and go back here when we want to lock a new channel.
//	avl6211pTuner->m_uiFrequency_100kHz = tuner_freq*10;      
	avl6211pTuner->m_uiSymbolRate_Hz = c->symbol_rate;//c->symbol_rate;//30000000; //symbol rate of the channel to be locked.
	//This function should be called before locking the tuner to adjust the tuner LPF based on channel symbol rate.
	AVL_Set_LPF(avl6211pTuner, avl6211pTuner->m_uiSymbolRate_Hz);

	r=avl6211pTuner->m_pLockFunc(avl6211pTuner);
	if (AVL_DVBSx_EC_OK != r)
	{
		state->freq=c->frequency;
		state->mode=c->modulation ;
		state->symbol_rate=c->symbol_rate;
		AVL_DVBSx_IBSP_ReleaseSemaphore(&blindscanSem);
 		pr_dbg("Tuner test failed !\n");
		return (r);
	}
	pr_dbg("Tuner test ok !\n");
	//msleep(50);
	fe->ops.asyncinfo.set_frontend_asyncpreproc(fe);
	async_ret = fe->ops.asyncinfo.set_frontend_asyncwait(fe, 50);
	if(async_ret > 0){
		fe->ops.asyncinfo.set_frontend_asyncpostproc(fe, async_ret);
		AVL_DVBSx_IBSP_ReleaseSemaphore(&blindscanSem);
		return 0;
	}	
	fe->ops.asyncinfo.set_frontend_asyncpostproc(fe, async_ret);	
	#if 0
	Channel.m_uiSymbolRate_Hz = c->symbol_rate;      //Change the value defined by macro when we want to lock a new channel.
	Channel.m_Flags = (CI_FLAG_MANUAL_LOCK_MODE) << CI_FLAG_MANUAL_LOCK_MODE_BIT;		//Manual lock Flag
									
	Channel.m_Flags |= (CI_FLAG_IQ_NO_SWAPPED) << CI_FLAG_IQ_BIT;   		//Auto IQ swap
	Channel.m_Flags |= (CI_FLAG_IQ_AUTO_BIT_AUTO) << CI_FLAG_IQ_AUTO_BIT;			//Auto IQ swap Flag
													//Support QPSK and 8PSK  dvbs2
	{
	#define Coderate				RX_DVBS2_2_3
	#define Modulation				AVL_DVBSx_MM_QPSK
	
		if (Coderate > 16 || Coderate < 6 || Modulation > 3)
		{			
			printf("Configure error !\n");
			return AVL_DVBSx_EC_GeneralFail;
		}
		Channel.m_Flags |= (CI_FLAG_DVBS2) << CI_FLAG_DVBS2_BIT;											//Disable automatic standard detection
		Channel.m_Flags |= (enum AVL_DVBSx_FecRate)(Coderate) << CI_FLAG_CODERATE_BIT;						//Manual config FEC code rate
		Channel.m_Flags |= ((enum AVL_DVBSx_ModulationMode)(Modulation)) << CI_FLAG_MODULATION_BIT;			//Manual config Modulation
	}
	//This function should be called after tuner locked to lock the channel.
	#else
	Channel.m_uiSymbolRate_Hz = c->symbol_rate;
	Channel.m_Flags = (CI_FLAG_IQ_NO_SWAPPED) << CI_FLAG_IQ_BIT;	//Normal IQ
	Channel.m_Flags |= (CI_FLAG_IQ_AUTO_BIT_AUTO) << CI_FLAG_IQ_AUTO_BIT;	//Enable automatic IQ swap detection
	Channel.m_Flags |= (CI_FLAG_DVBS2_UNDEF) << CI_FLAG_DVBS2_BIT;			//Enable automatic standard detection
	#endif
	r = AVL_DVBSx_IRx_LockChannel(&Channel, pAVLChip_all);  
	if (AVL_DVBSx_EC_OK != r)
	{
		state->freq=c->frequency;
		state->mode=c->modulation ;
		state->symbol_rate=c->symbol_rate;	
		AVL_DVBSx_IBSP_ReleaseSemaphore(&blindscanSem);
		pr_dbg("Lock channel failed !\n");
		return (r);
	}
	AVL_DVBSx_IBSP_ReleaseSemaphore(&blindscanSem);
	if (AVL_DVBSx_EC_OK != r)
	{
		printf("Lock channel failed !\n");
		return (r);
	}
	
	//Channel lock time increase while symbol rate decrease.Give the max waiting time for different symbolrates.
	if(c->symbol_rate<5000000)
	{
		waittime = 150;//250;       //The max waiting time is 5000ms,considering the IQ swapped status the time should be doubled.
	}
	else if(c->symbol_rate<10000000)
	{
        waittime = 30;        //The max waiting time is 600ms,considering the IQ swapped status the time should be doubled.
	}
	else
	{
        waittime = 15;        //The max waiting time is 300ms,considering the IQ swapped status the time should be doubled.
	} 
	
	fe->ops.asyncinfo.set_frontend_asyncpreproc(fe);
	while(waittime)
	{
		//msleep(20);
		async_ret = fe->ops.asyncinfo.set_frontend_asyncwait(fe, 20);
		if(async_ret > 0){
			break;
		}
		
		lockstatus=AVL6211_GETLockStatus();
		if(1==lockstatus){
			pr_dbg("lock success !\n");
			break;
		}
		
		waittime--;
	}
	fe->ops.asyncinfo.set_frontend_asyncpostproc(fe, async_ret);
	
	if(!AVL6211_GETLockStatus())
		pr_dbg("lock timeout !\n");
	
	r=AVL_DVBSx_IRx_ResetErrorStat(pAVLChip_all);
	if (AVL_DVBSx_EC_OK != r)
	{
		state->freq=c->frequency;
		state->mode=c->modulation ;
		state->symbol_rate=c->symbol_rate;
		
		printf("Reset error status failed !\n");
		return (r);
	}
	
//	demod_connect(state, p->frequency,p->u.qam.modulation,p->u.qam.symbol_rate);
	state->freq=c->frequency;
	state->mode=c->modulation ;
	state->symbol_rate=c->symbol_rate; //these data will be writed to eeprom

	pr_dbg("avl6211=>frequency=%d,symbol_rate=%d\r\n",c->frequency,c->symbol_rate);
	return  0;
}

static int AVL6211_Get_Frontend(struct dvb_frontend *fe)
{//these content will be writed into eeprom .
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct avl6211_state *state = fe->demodulator_priv;

	printk("delivery sys: %d\n", c->delivery_system);
	c->frequency=state->freq;
	c->symbol_rate=state->symbol_rate;
	
	return 0;
}

#if 0
static ssize_t avl_frontend_show_short_circuit(struct class* class, struct class_attribute* attr, char* buf)
{
	int ant_overload_status = AVL6211_Ant_Overload_Ctrl();
	
	return sprintf(buf, "%d\n", ant_overload_status);

}

static struct class_attribute avl_frontend_class_attrs[] = {
	__ATTR(short_circuit,  S_IRUGO | S_IWUSR, avl_frontend_show_short_circuit, NULL),
	__ATTR_NULL
};
#endif

static int avl6211_fe_get_ops(struct aml_fe_dev *dev, int mode, void *ops)
{
	struct dvb_frontend_ops *fe_ops = (struct dvb_frontend_ops*)ops;

	char *fe_name = "AMLOGIC DVB-S2";
	memcpy(fe_ops->info.name, fe_name, strlen(fe_name));
	fe_ops->info.type = FE_QPSK;
	fe_ops->info.frequency_min = 850000;
	fe_ops->info.frequency_max = 2300000;
	fe_ops->info.frequency_stepsize = 0;
	fe_ops->info.frequency_tolerance = 0;
	fe_ops->info.caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_QPSK | FE_CAN_QAM_16 |
			FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_HIERARCHY_AUTO |
			FE_CAN_RECOVER |
			FE_CAN_MUTE_TS;

	fe_ops->set_frontend = AVL6211_Set_Frontend;
	fe_ops->get_frontend = AVL6211_Get_Frontend;	
	fe_ops->read_status = AVL6211_Read_Status;
	fe_ops->read_ber = AVL6211_Read_Ber;
	fe_ops->read_signal_strength = AVL6211_Read_Signal_Strength;
	fe_ops->read_snr = AVL6211_Read_Snr;
	fe_ops->read_ucblocks = AVL6211_Read_Ucblocks;

	fe_ops->diseqc_reset_overload = AVL6211_Diseqc_Reset_Overload;
	fe_ops->diseqc_send_master_cmd = AVL6211_Diseqc_Send_Master_Cmd;
	fe_ops->diseqc_recv_slave_reply = AVL6211_Diseqc_Recv_Slave_Reply;
	fe_ops->diseqc_send_burst = AVL6211_Diseqc_Send_Burst;
	fe_ops->set_tone = AVL6211_Set_Tone;
	fe_ops->set_voltage = AVL6211_Set_Voltage;
	fe_ops->enable_high_lnb_voltage = AVL6211_Enable_High_Lnb_Voltage;

	fe_ops->blindscan_ops.blindscan_scan = AVL6211_Blindscan_Scan;
	fe_ops->blindscan_ops.blindscan_cancel = AVL6211_Blindscan_Cancel;

	fe_ops->asyncinfo.set_frontend_asyncenable = 1;

	return 0;
}

static int avl6211_fe_enter_mode(struct aml_fe *fe, int mode)
{
	struct aml_fe_dev *dev = fe->dtv_demod;

	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	//init sema
	cur_dvbdev = dev;
	pr_dbg("=========================demod init\r\n");

	AVL_DVBSx_IBSP_InitSemaphore( &blindscanSem );
	//reset
	AVL6211_Reset(dev->reset_gpio);
	msleep(100);
	//init
	r=AVL6211_LockSignal_Init();
	//LBNON
//	AVL6211_Lnb_Power_Ctrl(1);
	//tunerpower
	AVL6211_Tuner_Power_Ctrl(0);	
//	r=AVL_DVBSx_IDiseqc_StopContinuous(pAVLChip_all);
	if(AVL_DVBSx_EC_OK != r)
	{
		return r;
	}
	initflag =0;
	pr_dbg("0x%x(ptuner),0x%x(pavchip)=========================demod init\r\n",avl6211pTuner->m_uiSlaveAddress,pAVLChip_all->m_SlaveAddr);
	msleep(200);
	
	return 0;
}


static int avl6211_fe_resume(struct aml_fe_dev *dev)
{

	AVL_DVBSx_ErrorCode r = AVL_DVBSx_EC_OK;
	cur_dvbdev = dev;
	
	pr_dbg("avl6211_fe_resume \n");
	//init sema
	AVL_DVBSx_IBSP_InitSemaphore( &blindscanSem );
	//reset
	AVL6211_Reset(dev->reset_gpio);
	msleep(100);
	//init
	r=AVL6211_LockSignal_Init();
	//LBNON
//	AVL6211_Lnb_Power_Ctrl(1);
	//tunerpower
	AVL6211_Tuner_Power_Ctrl(0);
//	r=AVL_DVBSx_IDiseqc_StopContinuous(pAVLChip_all);

	r=AVL_DVBSx_IBase_SetGPIODir(0, pAVLChip_all);
	if(AVL_DVBSx_EC_OK != r)
	{
		return r;
	}
	initflag =0;
	pr_dbg("0x%x(ptuner),0x%x(pavchip)=========================demod init\r\n",avl6211pTuner->m_uiSlaveAddress,pAVLChip_all->m_SlaveAddr);
	msleep(200);
	return 0;

}

static int avl6211_fe_suspend(struct aml_fe_dev *dev)
{
	return 0;
}

static struct aml_fe_drv avl6211_dtv_demod_drv = {
.owner      = THIS_MODULE,
.id         = AM_DTV_DEMOD_AVL6211,
.name       = "Avl6211",
.capability = AM_FE_QPSK,
.get_ops    = avl6211_fe_get_ops,
.enter_mode = avl6211_fe_enter_mode,
.suspend    = avl6211_fe_suspend,
.resume     = avl6211_fe_resume
};

static int __init avlfrontend_init(void)
{
	pr_dbg("register avl6211 demod driver\n");
	return aml_register_fe_drv(AM_DEV_DTV_DEMOD, &avl6211_dtv_demod_drv);
}


static void __exit avlfrontend_exit(void)
{
	pr_dbg("unregister avl6211 demod driver\n");
	aml_unregister_fe_drv(AM_DEV_DTV_DEMOD, &avl6211_dtv_demod_drv);	
}

fs_initcall(avlfrontend_init);
module_exit(avlfrontend_exit);


MODULE_DESCRIPTION("avl6211 DVB-S2 Demodulator driver");
MODULE_AUTHOR("RSJ");
MODULE_LICENSE("Proprietary");



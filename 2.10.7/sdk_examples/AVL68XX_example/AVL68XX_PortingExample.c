/*
 *           Copyright 2007-2014 Availink, Inc.
 *
 *  This software contains Availink proprietary information and
 *  its use and disclosure are restricted solely to the terms in
 *  the corresponding written license agreement. It shall not be 
 *  disclosed to anyone other than valid licensees without
 *  written permission of Availink, Inc.
 *
 */


using namespace System;

#include "AVL_Demod.h"
#include "AVL_Tuner.h"
#include "R848_API.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"




#define I2C_SERVER_IP               "localhost"
#define I2C_SERVER_PORT             88


AVL_uint32 g_ChipNo = 0;//support two demod chips
AVL_DemodMode eStartupMode = AVL_DVBC;

static AVL_Tuner global_tuner_t_c = 
{
    0x7A,
    AVL_STATUS_UNLOCK,//tuner lock status

    DTVMode_DVBTX,
    666*1000*1000,
    5*1000*1000,
    8000*1000,
    340*1000*1000,               //LPF setting, not used for T/C tuner

    NULL,
      
    R848_Initialize,                              //tuner initialization function
    R848_Lock,                                   //tuner lock function
    R848_GetLockStatus,                     //tuner check lock status function
    NULL,
   
};

static AVL_Tuner global_tuner_s = 
{
    //tuner configuration
    0x7A,
    AVL_STATUS_UNLOCK,

    DTVMode_DVBSX,
    666*1000*1000,
    5000*1000,//IF frequency setting, not used for S tuner
    8000*1000,//BW setting, not used for S tuner
    340*1000*1000,

    NULL,
      
    R848_Initialize,                              //tuner initialization function
    R848_Lock,                                   //tuner lock function
    R848_GetLockStatus,                     //tuner check lock status function
    NULL,
};

static AVL_ChannelInfo g_BlindScan_channels[256];
static AVL_uchar g_u16Total_tp = 0;
#define Sub_abs(a,b) ((a>=b)?(a-b):(b-a))
#define AVL_min(x,y) (((x) < (y)) ? (x) : (y))
#define EAST 0
#define WEST 1

static AVL_ErrorCode A8293_Control( AVL_uint32 LNB_LEVEL)
{
    AVL_ErrorCode r = AVL_EC_OK;
    AVL_uint16 uiSize = 1;
    AVL_uchar uiData = 0x10;

    r |= AVL_IBSP_I2C_Read(0x08, &uiData, &uiSize);
    if(LNB_LEVEL == 2)//Horizontal
    {
          printf("18V\n");
        uiData = 0x3E;
    }
    else if(LNB_LEVEL == 1)  //Vertical
    {
        printf("13V\n");
        uiData =0x34;// 0x37;
    }
    else if(LNB_LEVEL == 0)  //LNB supply off
    {
        printf("LNB Power off.\n");
        uiData = 0x10;
    }
    r |= AVL_IBSP_I2C_Write(0x08, &uiData, &uiSize);

    uiData = 0x82;
    r |= AVL_IBSP_I2C_Write(0x08, &uiData, &uiSize); //Open 22K to LNB supply

    return r;
}

static void AVL_Check_LockStatus(AVL_uchar *pLockFlag)
{
    AVL_uint16 times_out_cnt = 0;
    AVL_ErrorCode r = AVL_EC_OK;
    AVL_uchar ucLockStatus = 0;
    AVL_uchar ucNosignal = 0;
    AVL_uint32 uiChipNo = g_ChipNo;
    AVL_DemodMode eCurrentDemodMode;
	*pLockFlag = 0;

    GetMode_Demod(&eCurrentDemodMode,uiChipNo);
    // check the lock status of the demod
    while(times_out_cnt<200)//this time-out window can be customized
    {
        AVL_IBSP_Delay(15);
        // No signal Detection
        if(eCurrentDemodMode ==  AVL_DVBTX)
        { 
          
          AVL_Demod_DVBTxSignalDetection(&ucNosignal, uiChipNo);
          if(ucNosignal==0)//ucNosignal=1:signal exist 0:nosignal
          {
             printf("[AVL_Check_LockStatus] ---- NoSignal Channel! ---- \n");
             break;
          }
        }       
        //get the lock status
        r = AVL_Demod_GetLockStatus(&ucLockStatus, uiChipNo);
        if(r != AVL_EC_OK)
        {
            printf("[AVL_Check_LockStatus] AVL_GetLockStatus Failed!\n");
            return;
        }
        if(ucLockStatus == 1)
        {
            printf("[AVL_Check_LockStatus] --- Channel locked! ---\n");
            *pLockFlag = 1;
            return;
        }
        times_out_cnt++;
    }
    printf("[AVL_Check_LockStatus] channel unlocked!\n");
}

static AVL_ErrorCode DVB_Sx_tuner_Lock(AVL_uint32 Freq_Khz,AVL_uint32 Symbol_Khz)
{ 
    AVL_Tuner *pTuner = &global_tuner_s;
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_uchar tuner_lock_retry = 0;
    AVL_uint32 uiChipNo = g_ChipNo;

    pTuner->uiRFFrequencyHz = Freq_Khz*1000;
    pTuner->uiLPFHz = Symbol_Khz * 1000/200*135+2000000;///should be set  different
    pTuner->eDTVMode = DTVMode_DVBSX;

    //Open the I2C bus for Sx tuner
    AVL_Demod_I2CBypassOn(uiChipNo);
    
    if(pTuner->fpLockFunc != NULL)
    {
        return_code = pTuner->fpLockFunc(pTuner);
        if(return_code!=AVL_EC_OK)
        {
            printf("[DVB_Sx_tuner_Lock] Tuner lock function Failed!\n");
            return (return_code);
        }
    }
    else
    {
        printf("[DVB_Sx_tuner_Lock] Tuner lock function is NULL,............WARNING.!\n");
    }
    
    //check the tuner is locked
    if(pTuner->fpGetLockStatusFunc != NULL)
    {
        while(tuner_lock_retry <= 5)
        {
            AVL_IBSP_Delay(20);
            return_code = pTuner->fpGetLockStatusFunc(pTuner);
            if(return_code!=AVL_EC_OK)
            {
                printf("[DVB_Sx_tuner_Lock] Tuner error or I2C error!\n");
                AVL_Demod_I2CBypassOff(uiChipNo);
                return return_code;
            }

            if(pTuner->ucTunerLocked == 1)
            {
                printf("[DVB_Sx_tuner_Lock] Tuner locked!\n");      
                break;
            }
            else
            {
                printf("[DVB_Sx_tuner_Lock] Tuner unlock!\n");
                AVL_Demod_I2CBypassOff(uiChipNo);
                return return_code;
            }
        }
    }
    else
    {
        printf("[DVB_Sx_tuner_Lock] Tuner check status function is NULL,............WARNING.!\n");
    }
    
    //Close the I2C bus to avoid interference
    AVL_Demod_I2CBypassOff(uiChipNo);
    
    return return_code;
}

static AVL_ErrorCode DVB_C_tuner_Lock(AVL_uint32 Freq_Khz,AVL_uint32 BandWidth_Khz)
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_Tuner *pTuner = &global_tuner_t_c;
    AVL_uchar tuner_lock_retry = 0;
    AVL_uint32 uiChipNo = g_ChipNo;

    pTuner->uiRFFrequencyHz = Freq_Khz*1000;
    pTuner->uiBandwidthHz = BandWidth_Khz*1150;///should be set different, the rolloff  is 0.15.
    pTuner->eDTVMode = DTVMode_DVBC;
    
     // Open the I2C bus for Tx/C tuner
     AVL_Demod_I2CBypassOn(uiChipNo);
     
     if(pTuner->fpLockFunc != NULL)
     {
         return_code = pTuner->fpLockFunc(pTuner);
         if(return_code!=AVL_EC_OK)
         {
             printf("[DVB_C_tuner_Lock] Tuner lock function Failed!\n");
             return (return_code);
         }
     }
     else
     {
         printf("[DVB_C_tuner_Lock] Tuner lock function is NULL,............WARRING.!\n");
     }
     
     //check the tuner is locked
     if(pTuner->fpGetLockStatusFunc != NULL)
     {
         while(tuner_lock_retry <= 5)
         {
             AVL_IBSP_Delay(20);
             return_code = pTuner->fpGetLockStatusFunc(pTuner);
             if(return_code != AVL_EC_OK)
             {
                 printf("[DVB_C_tuner_Lock] Tuner error or I2C error!\n");
                 AVL_Demod_I2CBypassOff(uiChipNo);
                 return return_code;
             }
    
             if(pTuner->ucTunerLocked == 1)
             {
                 printf("[DVB_C_tuner_Lock] Tuner locked!\n");
                 break;
             }
             else
             {
                 printf("[DVB_C_tuner_Lock] Tuner unlock!\n");
                 AVL_Demod_I2CBypassOff(uiChipNo);
                 return return_code;
             }
         }
     }
     else
     {
         printf("[DVB_C_tuner_Lock] Tuner check status function is NULL,............WARRING.!\n");
     }
     
     // Close the I2C bus to avoid interference
     AVL_Demod_I2CBypassOff(uiChipNo);
     return return_code;
}

static AVL_ErrorCode DVB_Tx_tuner_Lock(AVL_uint32 Freq_Khz,AVL_uint16 BandWidth_Khz)
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_Tuner *pTuner = &global_tuner_t_c;
    AVL_uchar tuner_lock_retry = 0;
    AVL_uint32 uiChipNo = g_ChipNo;
    
    pTuner->uiRFFrequencyHz = Freq_Khz*1000;
    pTuner->uiBandwidthHz = BandWidth_Khz*1000;
    pTuner->eDTVMode = DTVMode_DVBTX;
    
    // Open the I2C bus for Tx/C tuner
    AVL_Demod_I2CBypassOn(uiChipNo);
    
    if(pTuner->fpLockFunc != NULL)
    {
        return_code = pTuner->fpLockFunc(pTuner);
        if(return_code != AVL_EC_OK)
        {
            printf("[DVB_Tx_tuner_Lock] Tuner lock function Failed!\n");
            return (return_code);
        }
    }
    else
    {
        printf("[DVB_Tx_tuner_Lock] Tuner lock function is NULL,............WARNING.!\n");
    }
    
    //check the tuner is locked
    if(pTuner->fpGetLockStatusFunc != NULL)
    {
        while(tuner_lock_retry <= 5)
        {
             AVL_IBSP_Delay(10);
            return_code = pTuner->fpGetLockStatusFunc(pTuner);
            if(return_code!=AVL_EC_OK)
            {
                printf("[DVB_Tx_tuner_Lock] Tuner error or I2C error!\n");
                AVL_Demod_I2CBypassOff(uiChipNo);
                return return_code;
            }
    
            if(pTuner->ucTunerLocked == 1)
            {
                 printf("[DVB_Tx_tuner_Lock] Tuner locked!\n");
                break;
            }
            else
            {
                tuner_lock_retry++;
                printf("[DVB_Tx_tuner_Lock] Tuner unlock!\n");
            }
             //AVL_Demod_I2CBypassOff(uiChipNo);
        }
    }
    else
    {
        printf("[DVB_Tx_tuner_Lock] Tuner check status function is NULL,............WARNING.!\n");
    }
    
    // Close the I2C bus to avoid interference
    AVL_Demod_I2CBypassOff(uiChipNo);
    
    return(return_code);
}

static AVL_ErrorCode ISDBT_tuner_Lock(AVL_uint32 Freq_Khz,AVL_uint16 BandWidth_Khz)
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_Tuner *pTuner = &global_tuner_t_c;
    AVL_uchar tuner_lock_retry = 0;
    AVL_uint32 uiChipNo = g_ChipNo;
    
    pTuner->uiRFFrequencyHz = Freq_Khz*1000;
    pTuner->uiBandwidthHz = BandWidth_Khz*1000;
    pTuner->eDTVMode = DTVMode_ISDBT;
    
    // Open the I2C bus for Tx/C tuner
    AVL_Demod_I2CBypassOn(uiChipNo);
    
    if(pTuner->fpLockFunc != NULL)
    {
        return_code = pTuner->fpLockFunc(pTuner);
        if(return_code != AVL_EC_OK)
        {
            printf("[ISDBT_tuner_Lock] Tuner lock function Failed!\n");
            return (return_code);
        }
    }
    else
    {
        printf("[ISDBT_tuner_Lock] Tuner lock function is NULL,............WARNING.!\n");
    }
    
    //check the tuner is locked
    if(pTuner->fpGetLockStatusFunc != NULL)
    {
        while(tuner_lock_retry <= 5)
        {
             AVL_IBSP_Delay(10);
            return_code = pTuner->fpGetLockStatusFunc(pTuner);
            if(return_code!=AVL_EC_OK)
            {
                printf("[ISDBT_tuner_Lock] Tuner error or I2C error!\n");
                AVL_Demod_I2CBypassOff(uiChipNo);
                return return_code;
            }
    
            if(pTuner->ucTunerLocked == 1)
            {
                 printf("[ISDBT_tuner_Lock] Tuner locked!\n");
                break;
            }
            else
            {
                tuner_lock_retry++;
                printf("[ISDBT_tuner_Lock] Tuner unlock!\n");
            }
             AVL_Demod_I2CBypassOff(uiChipNo);
        }
    }
    else
    {
        printf("[ISDBT_tuner_Lock] Tuner check status function is NULL,............WARNING.!\n");
    }
    
    // Close the I2C bus to avoid interference
    AVL_Demod_I2CBypassOff(uiChipNo);
    
    return(return_code);
}

static AVL_ErrorCode AVL_Init(void)
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    static AVL_uchar tuner_t2_isdbt_c_initflag = 0;
    static AVL_uchar tuner_s_initflag = 0;
    AVL_uint32 uiChipNo = g_ChipNo;
    AVL_uint32 uiChipID = 0;
    AVL_uint32 uiFamilyID ;

    return_code = AVL_Demod_GetChipID(&uiChipID,uiChipNo);
    if(return_code == AVL_EC_OK)
        printf("[GetChipId] chip id:0x%x\n",uiChipID);
    else
        printf("[GetChipId] get chip id Fail.\n");  
    
    return_code = GetFamilyID_Demod(&uiFamilyID, uiChipNo);
    if(return_code == AVL_EC_OK)
        printf("[GetChipId] Family ID:0x%x\n",uiFamilyID);
    else
        printf("[GetChipId] get Family ID Fail.\n");

    return_code = AVL_Demod_Initialize(eStartupMode, uiChipNo);
    if(return_code != AVL_EC_OK)
    {
        printf("[AVL_Init] AVL_Initialize Failed!\n");
        return return_code;
    }
    else
    {
        printf("[AVL_Init] AVL_Initialize Booted!\n");
    }

    //if T/C tuner isn't initialized
    if(tuner_t2_isdbt_c_initflag == 0)
    {
        tuner_t2_isdbt_c_initflag = 1;

        // Open the I2C bus for tuner
        AVL_Demod_I2CBypassOn(uiChipNo);
        
        if(global_tuner_t_c.fpInitializeFunc != NULL)
        {
            return_code = global_tuner_t_c.fpInitializeFunc(&global_tuner_t_c);
            if(return_code!=AVL_EC_OK)
            {
                printf("[AVL_Init] T/C Tuner Init Failed!\n");
            }           
        }

        // Close the I2C bus to avoid interference
        AVL_Demod_I2CBypassOff(uiChipNo);
    }

    //if Sx tuner isn't initialized
    if(tuner_s_initflag == 0)
    {
        tuner_s_initflag = 1;

        // Open the I2C bus for tuner
        AVL_Demod_I2CBypassOn(uiChipNo);
        
        if(global_tuner_s.fpInitializeFunc != NULL)
        {
            return_code = global_tuner_s.fpInitializeFunc(&global_tuner_s);
            if(return_code!=AVL_EC_OK)
            {
                printf("[AVL_Init] S Tuner Init Failed!\n");
            }           
        }
        // Close the I2C bus to avoid interference
        AVL_Demod_I2CBypassOff(uiChipNo);
    }   
 
    printf("[AVL_Init] ok\n");

    return return_code;
}

static AVL_ErrorCode AVL_LockChannel_DVBSx(AVL_uint32 Freq_Khz,AVL_uint32 Symbol_Khz) 
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_DemodMode eCurrentDemodMode;
    AVL_uint32 uiChipNo = g_ChipNo;

    GetMode_Demod(&eCurrentDemodMode,uiChipNo);
    if(eCurrentDemodMode != AVL_DVBSX)
    {
        printf("[AVL_LockChannel_DVBSx] demod mode is not DVB-Sx,Err.\n");
        return AVL_EC_GENERAL_FAIL;
    }

    printf("[AVL_LockChannel_DVBSx] Freq:%d Mhz,sym:%d Khz\n",Freq_Khz/1000,Symbol_Khz);

    DVB_Sx_tuner_Lock(Freq_Khz, Symbol_Khz);
    
    return_code = AVL_Demod_DVBSxAutoLock(Symbol_Khz*1000, uiChipNo);
    if(return_code != AVL_EC_OK)
    {
        printf("[AVL_LockChannel_DVBSx] Failed to lock the channel!\n");
        return return_code;
    }

    return return_code;
}

static AVL_ErrorCode AVL_LockChannel_ISDBT(AVL_uint32 Freq_Khz, AVL_uint16 BandWidth_Khz)
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_DemodMode eCurrentDemodMode;
    AVL_uint32 uiChipNo = g_ChipNo;

    GetMode_Demod(&eCurrentDemodMode,uiChipNo);
    if(eCurrentDemodMode != AVL_ISDBT)
    {
        printf("[AVL_LockChannel_ISDBT] demod mode is not ISDBT,Err.\n");
        return AVL_EC_GENERAL_FAIL;
    }

    printf("[AVL_LockChannel_ISDBT] Freq:%d Mhz,sym:%d Khz\n",Freq_Khz/1000,BandWidth_Khz);
    
    ISDBT_tuner_Lock(Freq_Khz, BandWidth_Khz);
    
    return_code = AVL_Demod_ISDBTAutoLock(uiChipNo);
    if(return_code != AVL_EC_OK)
    {
        printf("[AVL_LockChannel_ISDBT] Failed to lock the channel!\n");
        return return_code;
    }

    return return_code; 
}

static AVL_DVBTxBandWidth Convert2DemodBand(AVL_uint16 BandWidth_Khz)
{
    AVL_DVBTxBandWidth nBand = AVL_DVBTx_BW_8M;

    if(BandWidth_Khz == 1700)
     {
         nBand = AVL_DVBTx_BW_1M7;
     }
     else if(BandWidth_Khz == 5000)
     {
         nBand = AVL_DVBTx_BW_5M;
     }
     else if(BandWidth_Khz == 6000)
     {
         nBand = AVL_DVBTx_BW_6M;
     }
     else if(BandWidth_Khz == 7000)
     {
         nBand = AVL_DVBTx_BW_7M;
     }
     else if(BandWidth_Khz == 8000)
     {
         nBand = AVL_DVBTx_BW_8M;
     }
     else
     {
         printf("[Convert2DemodBand] BandWidth_Khz is Err\n");
         nBand = AVL_DVBTx_BW_8M;
     }
     return nBand;
}

static AVL_ErrorCode AVL_LockChannel_T(AVL_uint32 Freq_Khz,AVL_uint16 BandWidth_Khz, AVL_int32 DVBT_layer_info)
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_DVBTxBandWidth nBand = AVL_DVBTx_BW_8M;
    AVL_uint32 uiChipNo = g_ChipNo;
    AVL_DemodMode eCurrentDemodMode;

    GetMode_Demod(&eCurrentDemodMode,uiChipNo);
    if(eCurrentDemodMode != AVL_DVBTX)
    {
        printf("[AVL_LockChannel_T] demod mode is not DVB-Tx,Err.\n");
        return AVL_EC_GENERAL_FAIL;
    }
    
    return_code = DVB_Tx_tuner_Lock(Freq_Khz,BandWidth_Khz);
    
    printf("[AVL_LockChannel_T] Freq is %d MHz, Bandwide is %d MHz, Layer Info is %d (0 : LP; 1 : HP)\n",
                       Freq_Khz/1000, BandWidth_Khz/1000, DVBT_layer_info);

    nBand = Convert2DemodBand(BandWidth_Khz);

    return_code = AVL_Demod_DVBTAutoLock(nBand, DVBT_layer_info, uiChipNo);
    
    if(return_code != AVL_EC_OK)
    {
        printf("[AVL_LockChannel_T] Failed to lock the channel!\n");
    }

    return return_code;
}

static AVL_ErrorCode AVL_LockChannel_T2(AVL_uint32 Freq_Khz,AVL_uint16 BandWidth_Khz, AVL_uchar T2_Profile, AVL_int32 PLP_ID)
{
    
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_DVBTxBandWidth nBand = AVL_DVBTx_BW_8M;
    AVL_uint32 uiChipNo = g_ChipNo;
    AVL_DemodMode eCurrentDemodMode;

    GetMode_Demod(&eCurrentDemodMode,uiChipNo);
    if(eCurrentDemodMode != AVL_DVBTX)
    {
        printf("[AVL_LockChannel_T2] demod mode is not DVB-Tx,Err.\n");
        return AVL_EC_GENERAL_FAIL;
    }

    return_code = DVB_Tx_tuner_Lock(Freq_Khz,BandWidth_Khz);

    printf("[AVL_LockChannel_T2] Freq is %d MHz, Bandwide is %d MHz, DATA PLP ID is %d \n",
                       Freq_Khz/1000, BandWidth_Khz/1000, PLP_ID);

    nBand = Convert2DemodBand(BandWidth_Khz);

    return_code = AVL_Demod_DVBT2AutoLock(nBand, (AVL_DVBT2_PROFILE)T2_Profile, PLP_ID, uiChipNo);

    if(return_code != AVL_EC_OK)
    {
        printf("[AVL_LockChannel_T2] Failed to lock the channel!\n");
    }
    
    return return_code;
}

static AVL_ErrorCode AVL_LockChannel_DVBC(AVL_uint32 Freq_Khz,AVL_uint16 BandWidth_Khz)
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_uint32 uiChipNo = g_ChipNo;
    AVL_DemodMode eCurrentDemodMode;

    GetMode_Demod(&eCurrentDemodMode,uiChipNo);
    if(eCurrentDemodMode !=  AVL_DVBC)
    {
        printf("[AVL_LockChannel_C] demod mode is not DVB-C,Err.\n");
        return AVL_EC_GENERAL_FAIL;
    }

    DVB_C_tuner_Lock(Freq_Khz, BandWidth_Khz);
    
    return_code = AVL_Demod_DVBCAutoLock(uiChipNo);
//	AVL_Demod_DVBCManualLock (5217*1000,AVL_DVBC_256QAM, uiChipNo);
    if(return_code!=AVL_EC_OK)
    {
        printf("[AVL_LockChannel_C] Failed to lock the channel!\n");
        return return_code;
    }

    return return_code; 
}

typedef struct s_DVBTx_Channel_TS
{
    // number, example 474*1000 is RF frequency 474MHz.
    int channel_freq_khz;
    // number, example 8000 is 8MHz bandwith channel.
    int channel_bandwith_khz;

    AVL_DVBTx_Standard channel_type;
    // 0 - Low priority layer, 1 - High priority layer
    unsigned char dvbt_hierarchy_layer;
    // data PLP id, 0 to 255; for single PLP DVBT2 channel, this ID is 0; for DVBT channel, this ID isn't used.
    unsigned char data_plp_id;
    AVL_DVBT2_PROFILE channel_profile;
}s_DVBTx_Channel_TS;



int current_table_index = 0;
#define MAX_CHANNEL_INFO 256
static s_DVBTx_Channel_TS global_channel_ts_table[MAX_CHANNEL_INFO];
static AVL_uint16 g_nChannel_ts_total = 0;


static void Channels_Filter_and_Adjust(struct AVL_ChannelInfo *Ch_list_valid, AVL_uchar *TP_No_valid, struct AVL_ChannelInfo *Ch_list_Temp, AVL_uchar TP_No_Temp)
{
    AVL_uchar i,j,flag;
    AVL_uchar Num = *TP_No_valid;
    struct AVL_ChannelInfo *pTemp;
    struct AVL_ChannelInfo *pValid;
    AVL_uint32 uiSymbolRate_Hz;
    AVL_uint32 ui_SR_offset;

    
    for(i=0;i< TP_No_Temp;i++)
    {
        pTemp = &Ch_list_Temp[i];
        flag = 0;
        for(j=0;j<*TP_No_valid;j++)
        {
            pValid = &Ch_list_valid[j];
            if( Sub_abs(pValid->m_uiFrequency_kHz,pTemp->m_uiFrequency_kHz) < AVL_min(pValid->m_uiSymbolRate_Hz,pTemp->m_uiSymbolRate_Hz)/2000)
            {
                flag = 1;
                break;
            }               
        }

        if(0 == flag)
        {
            Ch_list_valid[Num].m_Flags = pTemp->m_Flags;
            Ch_list_valid[Num].m_uiSymbolRate_Hz = pTemp->m_uiSymbolRate_Hz;
            Ch_list_valid[Num].m_uiFrequency_kHz = pTemp->m_uiFrequency_kHz;

            uiSymbolRate_Hz = Ch_list_valid[Num].m_uiSymbolRate_Hz;
            //----------------------------adjust symbolrate offset------------------------------------------------------------
            ui_SR_offset = ((uiSymbolRate_Hz%10000)>5000)?(10000-(uiSymbolRate_Hz%10000)):(uiSymbolRate_Hz%10000);
            if( ((uiSymbolRate_Hz>10000000) && (ui_SR_offset<3500)) || ((uiSymbolRate_Hz>5000000) && (ui_SR_offset<2000))  )
                uiSymbolRate_Hz =  (uiSymbolRate_Hz%10000<5000)?(uiSymbolRate_Hz-ui_SR_offset):(uiSymbolRate_Hz+ui_SR_offset);

            ui_SR_offset = ((uiSymbolRate_Hz%1000)>500)?(1000-(uiSymbolRate_Hz%1000)):(uiSymbolRate_Hz%1000);
            if( (uiSymbolRate_Hz<5000000) && (ui_SR_offset< 500))
                uiSymbolRate_Hz =  (uiSymbolRate_Hz%1000<500)?(uiSymbolRate_Hz-ui_SR_offset):(uiSymbolRate_Hz+ui_SR_offset);
    
            Ch_list_valid[Num].m_uiSymbolRate_Hz =1000*(uiSymbolRate_Hz/1000);
            //----------------------------------------------------------------------------------------------------------------
            Num++;
        }
    }
    
    *TP_No_valid = Num;
}

static void AVL_Blindscan_init(void)
{
    g_u16Total_tp = 0;
    memset(g_BlindScan_channels,0,sizeof(AVL_ChannelInfo)*256);
    return ;
}

static void AVL_BlindScanProcess(AVL_uint16 centerFreq_Mhz,AVL_uint16 *pnextCenterFreq_Mhz,AVL_ChannelInfo *pChannelList,AVL_uchar *Find_TP_num, AVL_int32 uiChipNo)
{

    AVL_ErrorCode r = AVL_EC_OK;
    AVL_Tuner *pTuner = &global_tuner_s;
    AVL_BlindScanPara BSPara;
    AVL_BSInfo BSInfo;
    AVL_uint16 ChannelCount = 0;
    AVL_ChannelInfo tempList[64];
    AVL_uchar ChannelNum_temp = 0;
    AVL_uchar Pre_total_num = 0;
    AVL_uchar j = 0,i = 0;

    pTuner->uiLPFHz = 400*100*1000;
    pTuner->uiRFFrequencyHz = centerFreq_Mhz*1000*1000;
    printf("Scan Freq:%d Mhz\n",centerFreq_Mhz);

    AVL_Demod_I2CBypassOn(uiChipNo);
    r = pTuner->fpLockFunc(pTuner);
    AVL_Demod_I2CBypassOff(uiChipNo);
    
    if(r != AVL_EC_OK)
    {
        printf("[AVL_BlindScanProcess] Tuner Lock Fail.\n");
        return ;
    }
    AVL_IBSP_Delay(50);
    BSPara.m_enumBSSpectrumPolarity = AVL_Spectrum_Normal;
    BSPara.m_uiMaxSymRate_kHz = 45*1000;
    BSPara.m_uiMinSymRate_kHz = 2*1000;
    BSPara.m_uiStartFreq_100kHz = centerFreq_Mhz*10 - 340;
    BSPara.m_uiStopFreq_100kHz = centerFreq_Mhz*10 + 340;

    //AVL_DVBSx_BlindScan_Reset(pAVL_Chip);
    AVL_Demod_DVBSx_SetFunctionalMode(AVL_FuncMode_BlindScan,uiChipNo);
    AVL_Demod_DVBSx_BlindScan_Reset(uiChipNo);
    AVL_Demod_DVBSx_BlindScan_Start(&BSPara,340,uiChipNo);
    do
    {
        printf("$.");
        r = AVL_Demod_DVBSx_BlindScan_GetStatus(&BSInfo,uiChipNo);
        if(r != AVL_EC_OK)
        {
            printf("Blindscan get status Err\n");
            break;
        }
        if(BSInfo.m_uiProgress == 100)
        {
            printf("Next Scan freq:%d Mhz\n",BSInfo.m_uiNextStartFreq_100kHz/10 + 34);
            *pnextCenterFreq_Mhz = BSInfo.m_uiNextStartFreq_100kHz/10 + 34;
            break;
        }
        AVL_IBSP_Delay(100);
    }while(1);
    if(BSInfo.m_uiChannelCount > 0)
    {
        printf("Scan Tp:%d.\n",BSInfo.m_uiChannelCount);
        ChannelCount = BSInfo.m_uiChannelCount;

        AVL_Demod_DVBSx_BlindScan_ReadChannelInfo(0,&ChannelCount,tempList,uiChipNo);
        for(i = 0 ; i < ChannelCount; i++)
        {
            printf("Tp:%d,Freq:%d Mhz,sym:%d Khz\n",i,tempList[i].m_uiFrequency_kHz/1000,tempList[i].m_uiSymbolRate_Hz/1000);
        }
    }
    else 
    {
        printf("Scan Nothing..\n");
    }

    ChannelNum_temp = (AVL_uchar)BSInfo.m_uiChannelCount;
    Pre_total_num = g_u16Total_tp;
    Channels_Filter_and_Adjust(g_BlindScan_channels, &g_u16Total_tp, tempList, ChannelNum_temp);
    i = 0;
    if(g_u16Total_tp > Pre_total_num)
    {   
        for(j = Pre_total_num;j < g_u16Total_tp;j++)
        {
            pChannelList[i].m_uiFrequency_kHz = g_BlindScan_channels[j].m_uiFrequency_kHz;
            pChannelList[i].m_uiSymbolRate_Hz = g_BlindScan_channels[j].m_uiSymbolRate_Hz;
            i++;
            printf("[BlindscanExamples] Ch%d: RF: %4d Mhz,SR: %5d Khz\n",j, (g_BlindScan_channels[j].m_uiFrequency_kHz/1000),(g_BlindScan_channels[j].m_uiSymbolRate_Hz/1000)); 
        }
    }
    *Find_TP_num = i;

    AVL_Demod_DVBSx_SetFunctionalMode(AVL_FuncMode_Demod,uiChipNo);
    return ;
}

static void BlindScanExamples()
{
    AVL_ChannelInfo ChannelList[64];
    AVL_ErrorCode r = AVL_EC_OK;
    AVL_uchar Find_TP_num = 0;
    AVL_uint32 uiChipNo = g_ChipNo;
    AVL_uint16 i = 0;
    AVL_uint16 freq_Mhz = 950;
    AVL_uint16 nextCenterFreq_Mhz = 0;
    AVL_uchar lasttime = 0;

    //A8293_Control(2);

    AVL_Blindscan_init();
    for(;freq_Mhz <= 2150+34;)
    {
        if (lasttime == 1)
        {
            break;
        }
        Find_TP_num = 0;
        if (freq_Mhz > 2150)
        {
            freq_Mhz = 2150;
            lasttime = 1;
        }
        AVL_BlindScanProcess(freq_Mhz, &nextCenterFreq_Mhz, ChannelList, &Find_TP_num, uiChipNo);
        freq_Mhz = nextCenterFreq_Mhz;
        i = 0;
        for(;i < Find_TP_num;i++)
        {
            r = AVL_LockChannel_DVBSx(ChannelList[i].m_uiFrequency_kHz, ChannelList[i].m_uiSymbolRate_Hz/1000);
            if(r != AVL_EC_OK)
            {
                printf("lock channel fail!\n");
            }   
            else
            {
                printf("lock channel ok!\n");
            }
        }
    }
    
    for(i = 0;i < g_u16Total_tp;i ++)
    {
        printf("Tp:%d,freq:%d Mhz---%d Mhz,sym:%d Khz\n",i,(5150-g_BlindScan_channels[i].m_uiFrequency_kHz/1000),g_BlindScan_channels[i].m_uiFrequency_kHz/1000,g_BlindScan_channels[i].m_uiSymbolRate_Hz/1000);
    }
}

static AVL_ErrorCode AVL_SX_22K_Control(AVL_uchar OnOff)
{
    AVL_ErrorCode r = AVL_EC_OK;
    AVL_uint32 uiChipNo = g_ChipNo;
    
    if(OnOff)
    {
        r = AVL_Demod_DVBSx_Diseqc_StartContinuous(uiChipNo);
    }else{
        r = AVL_Demod_DVBSx_Diseqc_StopContinuous(uiChipNo);
    }
    if(r != AVL_EC_OK)
    {
        printf("[AVL_SX_22K_Control] Err:0x%x\n",r);
    }   
    return r;
}

static AVL_ErrorCode AVL_SX_SetToneOut(AVL_uchar ucTone)
{
    AVL_ErrorCode r = AVL_EC_OK;
    struct AVL_Diseqc_TxStatus TxStatus;
    AVL_uint32 uiChipNo = g_ChipNo;
    
    r = AVL_Demod_DVBSx_Diseqc_SendTone( ucTone, 1, uiChipNo);
    if(r != AVL_EC_OK)
    {
        printf("AVL_SX_SetToneOut failed !\n");
    }
    else
    {
        do
        {
            AVL_IBSP_Delay(5);
            r = AVL_Demod_DVBSx_Diseqc_GetTxStatus(&TxStatus, uiChipNo);
        }while(TxStatus.m_TxDone != 1);
        if(r == AVL_EC_OK )
        {

        }
        else
        {
            printf("AVL_SX_SetToneOut Err. !\n");
        }
    }
    return r;    
}

static void AVL_SX_DiseqcSendCmd(AVL_puchar pCmd, AVL_uchar CmdSize)
{
    AVL_ErrorCode r = AVL_EC_OK;
    struct AVL_Diseqc_TxStatus TxStatus;
    AVL_uint32 uiChipNo = g_ChipNo;

    r = AVL_Demod_DVBSx_Diseqc_SendModulationData(pCmd, CmdSize, uiChipNo);
    if(r != AVL_EC_OK)
    {
        printf("AVL_SX_DiseqcSendCmd failed !\n");
    }
    else
    {
        do
        {
            AVL_IBSP_Delay(5);
            r |= AVL_Demod_DVBSx_Diseqc_GetTxStatus(&TxStatus, uiChipNo);
        }while(TxStatus.m_TxDone != 1);
        if(r == AVL_EC_OK )
        {

        }
        else
        {
            printf("AVL_SX_DiseqcSendCmd Err. !\n");
        }       
    }
}

static void DiseqcExamples(void)
{
    AVL_ErrorCode r = AVL_EC_OK;    
    AVL_uchar ucData[8];
    AVL_uchar uPortBit = 0; 
    AVL_uchar uLNBPort = 1;
    AVL_uchar uDirection = EAST;
    AVL_uchar uCommandByte = 0;

    //22K Control examples
    r = AVL_SX_22K_Control(1);
    if(r == AVL_EC_OK)
    {
        printf("Set 22K On,OK\n");
    }
    AVL_IBSP_Delay(1000);
    r = AVL_SX_22K_Control(0);
    if(r == AVL_EC_OK)
    {
        printf("Set 22K Off,OK\n");
    }
    AVL_IBSP_Delay(1000);
    
    //Send the tone burst command   
    r = AVL_SX_SetToneOut(1);
    if(r == AVL_EC_OK)
    {
        printf("Send ToneBurst 1,OK\n");
    }
    AVL_IBSP_Delay(1000);       
    r = AVL_SX_SetToneOut(0);
    if(r == AVL_EC_OK)
    {
        printf("Send ToneBurst 0,OK\n");
    }
    AVL_IBSP_Delay(1000);   

    //LNB switch control
    ucData[0] = 0xE0;
    ucData[1] = 0x10;
    ucData[2] = 0x38;
    ucData[3] = 0xF0;

    switch(uLNBPort) 
    { 
        case 1: 
            uPortBit = 0;
            break; 

        case 2: 
            uPortBit = 0x04;
            break; 

        case 3:
            uPortBit = 0x08;
            break; 

        case 4:
            uPortBit = 0x0C;
            break; 

        default:
            uPortBit = 0; 
            break; 

    } 
    ucData[3] += uPortBit; 

    //This function can be called after initialization to send out 4 modulation bytes to select the LNB port if used the 1/4 LNB switch.
    AVL_SX_DiseqcSendCmd(ucData, 4);


    //Positioner control one degree. 
    ucData[0] = 0xE0;
    ucData[1] = 0x31;
    ucData[2] = 0x68;
    ucData[3] = 0xFF;

    switch(uDirection) 
    { 
        case EAST: 
            uCommandByte = 0x68;       //Turn east
            break; 

        case WEST: 
            uCommandByte = 0x69;       //Turn west
            break; 

        default:
            uCommandByte = 0x68;
            break; 
    }

    ucData[2] = uCommandByte; 
    AVL_SX_DiseqcSendCmd(ucData, 4);
}

static AVL_ErrorCode AVL_SetWorkMode(AVL_DemodMode eDemodWorkMode)
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_uint32 uiChipNo = g_ChipNo;

    return_code = AVL_Demod_SetMode(eDemodWorkMode, uiChipNo);

    if(return_code != AVL_EC_OK)
    {
        printf("Failed to set work mode!\n");
        return (return_code);
    }
       
    return (return_code);
}

static AVL_ErrorCode AVL_ScanChannel_Tx(AVL_uint32 Freq_Khz,AVL_uint16 BandWidth_Khz)
{
    AVL_ErrorCode return_code = AVL_EC_OK;
    AVL_uint32 uiChipNo = g_ChipNo;
    AVL_DVBTxScanInfo stDVBTxScanInfo;
    AVL_DVBTxBandWidth nBand = AVL_DVBTx_BW_8M;
    AVL_DemodMode eCurrentDemodMode;
    AVL_uint16 cur_index = 0;
    AVL_uchar ucLockFlag = 0;
    AVL_DVBT2_PROFILE ucT2Profile = AVL_DVBT2_PROFILE_UNKNOWN;
    AVL_uchar ucTemp = 0;
    AVL_uchar ucDataPLPArray[255] = {0};
    AVL_uchar ucDataPLPNumber = 0;

    GetMode_Demod(&eCurrentDemodMode,uiChipNo);
    if(eCurrentDemodMode != AVL_DVBTX)
    {
        printf("[AVL_LockChannel_T] demod mode is not DVB-Tx,Err.\n");
        return AVL_EC_GENERAL_FAIL;     
    }
    //=====Tuner Lock=====//
    printf("[AVL_ChannelScan_Tx] Lock Tuner : \n===  Freq is %d MHz \n===  Bandwide is %d MHz \n",
                       Freq_Khz/1000, BandWidth_Khz/1000);
    return_code = DVB_Tx_tuner_Lock(Freq_Khz, BandWidth_Khz);

    nBand = Convert2DemodBand(BandWidth_Khz);
    
    //=====Demod Lock=====//
    return_code = AVL_Demod_DVBTxChannelScan(nBand, AVL_DVBTx_LockMode_ALL, uiChipNo);
    printf("[AVL_ChannelScan_Tx] Freq is %d MHz, Bandwide is %d MHz \n",
                       Freq_Khz/1000, BandWidth_Khz/1000);
    //=====Check Lock Status =====//   
    AVL_Check_LockStatus(&ucLockFlag);

    if(ucLockFlag == 1)//DVBTx is locked
    {
        printf("[AVL_ChannelScan_Tx] Freq is %d MHz, Bandwide is %d MHz \n",
                       Freq_Khz/1000, BandWidth_Khz/1000);
        return_code |= AVL_Demod_DVBTxGetScanInfo(&stDVBTxScanInfo, uiChipNo);
    
        if(stDVBTxScanInfo.eTxStandard == AVL_DVBTx_Standard_T2)//get PLP ID list only for DVBT2 signal, not for DVBT
        {
            cur_index = g_nChannel_ts_total;
            return_code = AVL_Demod_DVBT2GetPLPList(ucDataPLPArray, &ucDataPLPNumber, uiChipNo);

            for (ucTemp = 0; ucTemp < ucDataPLPNumber; ucTemp++)
            {
                printf("[DVB-T2_Scan_Info] DATA PLP ID is %d, profile = %d\n",ucDataPLPArray[ucTemp], stDVBTxScanInfo.ucTxInfo); 

                //save channel RF frequency
                global_channel_ts_table[cur_index].channel_freq_khz = Freq_Khz;
                // save channel bandwidth
                global_channel_ts_table[cur_index].channel_bandwith_khz = BandWidth_Khz;
                // save data plp id
                global_channel_ts_table[cur_index].data_plp_id = ucDataPLPArray[ucTemp];
                // 0 - DVBT; 1 - DVBT2.
                global_channel_ts_table[cur_index].channel_type = AVL_DVBTx_Standard_T2;
                // 0 - Base profile; 1 - Lite profile.
                global_channel_ts_table[cur_index].channel_profile = (AVL_DVBT2_PROFILE)stDVBTxScanInfo.ucTxInfo;

                cur_index++;
            }
            g_nChannel_ts_total = cur_index%MAX_CHANNEL_INFO;
            
            
            if (stDVBTxScanInfo.ucFEFInfo == 1)
            {  
                ucT2Profile = (AVL_DVBT2_PROFILE) stDVBTxScanInfo.ucTxInfo;
                
                if (ucT2Profile == AVL_DVBT2_PROFILE_BASE)//profile is base
                {         
                    //If T2 base is locked, try to lock T2 lite 
                    AVL_Demod_DVBTxChannelScan(nBand, AVL_DVBTx_LockMode_T2LITE, uiChipNo);
                    ucT2Profile = AVL_DVBT2_PROFILE_LITE;
                }
                else
                {         
                    //If T2 lite is locked, try to lock T2 base 
                    AVL_Demod_DVBTxChannelScan(nBand, AVL_DVBTx_LockMode_T2BASE, uiChipNo);                    
                    ucT2Profile = AVL_DVBT2_PROFILE_BASE;
                }
                AVL_Check_LockStatus(&ucLockFlag);
                if(ucLockFlag == 1)//DVBTx is locked
                { 
                    cur_index = g_nChannel_ts_total;
                    ucDataPLPNumber = 0;
                    return_code = AVL_Demod_DVBT2GetPLPList(ucDataPLPArray, &ucDataPLPNumber, uiChipNo);

                    // data PLP ID and common PLP ID pairing
                    for (ucTemp = 0; ucTemp < ucDataPLPNumber; ucTemp++)
                    {
                        printf("[DVB-T2_Scan_Info] DATA PLP ID is %d, profile = %d\n",ucDataPLPArray[ucTemp], ucT2Profile); 

                        //save channel RF frequency
                        global_channel_ts_table[cur_index].channel_freq_khz = Freq_Khz;
                        // save channel bandwidth
                        global_channel_ts_table[cur_index].channel_bandwith_khz = BandWidth_Khz;
                        // save data plp id
                        global_channel_ts_table[cur_index].data_plp_id = ucDataPLPArray[ucTemp];
                        // 0 - DVBT; 1 - DVBT2.
                        global_channel_ts_table[cur_index].channel_type = AVL_DVBTx_Standard_T2;
                        // 0 - Base profile; 1 - Lite profile.
                        global_channel_ts_table[cur_index].channel_profile = ucT2Profile;

                        cur_index++;
                    }
                    g_nChannel_ts_total = cur_index%MAX_CHANNEL_INFO;
                }
            }
            else
            {
                printf("No FEF \n");
            }
        }
        else // DVBT
        {
            cur_index = g_nChannel_ts_total;
            // save channel RF frequency
            global_channel_ts_table[cur_index].channel_freq_khz = Freq_Khz;
            // save channel bandwidth
            global_channel_ts_table[cur_index].channel_bandwith_khz = BandWidth_Khz;
            // save data plp id(not used for DVBT, set to 0xff)
            global_channel_ts_table[cur_index].data_plp_id = 0;
            // 0 - DVBT; 1 - DVBT2.
            global_channel_ts_table[cur_index].channel_type = AVL_DVBTx_Standard_T;
            // 0 - Low priority layer, 1 - High priority layer
            global_channel_ts_table[cur_index].dvbt_hierarchy_layer = 1; 
            cur_index++;

            if(stDVBTxScanInfo.ucTxInfo == 1)// for hierarchy
            {
                // save channel RF frequency
                global_channel_ts_table[cur_index].channel_freq_khz = Freq_Khz;
                // save channel bandwidth
                global_channel_ts_table[cur_index].channel_bandwith_khz = BandWidth_Khz;
                // save data plp id(not used for DVBT, set to 0xff)
                global_channel_ts_table[cur_index].data_plp_id = 0;
                // 0 - DVBT; 1 - DVBT2.
                global_channel_ts_table[cur_index].channel_type = AVL_DVBTx_Standard_T;
                // 0 - Low priority layer, 1 - High priority layer
                global_channel_ts_table[cur_index].dvbt_hierarchy_layer = 0; 
                
                cur_index++;
            }
            g_nChannel_ts_total = cur_index%MAX_CHANNEL_INFO;
        }
        
    }
    else // return for unlock
    {
        printf("[DVBTx_Channel_ScanLock_Example] DVBTx channel scan is fail,Err.\n");
    }
    return return_code; 
}

static AVL_ErrorCode get_SSI_info(void)
{
    AVL_ErrorCode r = AVL_EC_OK;
    AVL_Tuner *pTuner;
    AVL_uint16 signal_strength = 0;// range 0 - 65535
    AVL_uchar ucSSI = 0;// range 0 - 100    
    AVL_int32  iRFPower = -100;//dBm
    AVL_DemodMode current_mode;
    AVL_uint32 uiChipNo = g_ChipNo;

    r = GetMode_Demod(&current_mode,uiChipNo);
    switch(current_mode)
    {
        case AVL_DVBTX:
            pTuner = &global_tuner_t_c;
            if(pTuner->fpGetRFStrength != NULL)
            {
                AVL_Demod_I2CBypassOn(uiChipNo);//before tuner control, the tuner I2C control path should be enabled
                pTuner->fpGetRFStrength(pTuner,&iRFPower);
                AVL_Demod_I2CBypassOff(uiChipNo);//before the control is done, the tuner I2C control path should be disabled to avoid interference towards tuner
                AVL_Demod_DVBTxGetNorDigSSI(&ucSSI,iRFPower,uiChipNo);
            }
            else
            {
                printf("Tuner doesn't support RF signal estimation with unit dBm. Please contact with Availink for precise signal strength indicator.");
            }

            //the range for returned signal_strength is 0 to 100
            printf("SSI is %d\n", ucSSI);
            break;
        case AVL_DVBSX:
        case AVL_ISDBT:
        case AVL_DVBC: 
            /* the range for returned signal_strength is 0 to 100 */
            r |= AVL_Demod_GetSSI(&signal_strength, uiChipNo);
            ucSSI = signal_strength*100/65535;
            printf("SSI is %d\n",ucSSI);
            break;
        default:
            r = AVL_EC_GENERAL_FAIL;
            break;
    }
    
    return (r);
}

static AVL_ErrorCode get_SQI_info(void)
{
    AVL_ErrorCode r = AVL_EC_OK;
    AVL_uint16 signal_quality = 0;
    AVL_uint32 uiChipNo = g_ChipNo;

    
    /******** get SQI *******/
    /* the range for returned signal_quality is 0 to 100 */
    r = AVL_Demod_GetSQI(&signal_quality, uiChipNo);
    printf("SQI is %d\n",signal_quality);
    
    return (r);
}

static AVL_ErrorCode get_SNR_info(void)
{
    AVL_ErrorCode r = AVL_EC_OK;
    AVL_uint32 snr_dbx100 = 0;//value: 100 times as actual SNR
    AVL_uint32 uiChipNo = g_ChipNo;
    
    /******** get SNR *******/
    /* the returned value of snr_dbx100 is 100 times as actual SNR */
    r = AVL_Demod_GetSNR(&snr_dbx100, uiChipNo);
    printf("SNR is %f\n",(float)snr_dbx100/100);
    
    return (r);
}

static AVL_ErrorCode get_PER_info(void)
{
    AVL_ErrorCode r = AVL_EC_OK;
    AVL_uint32 per_1e9 = 0;
    AVL_uint32 uiChipNo = g_ChipNo;
    
    /******** get PER *******/
    /*the returned value of per_1e9 is 1e9 times as actual PER*/
    r = AVL_Demod_GetPER(&per_1e9, uiChipNo);
    printf("PER is %f\n",(float)per_1e9/1e9);
    
    return (r);
}

static AVL_ErrorCode AVL_R848_get_RFPower_info(void)
{
    AVL_ErrorCode r = AVL_EC_OK;
    AVL_Tuner    *pTuner;
    AVL_uint32 uiChipNo = g_ChipNo;
    AVL_uint32 uiRFGain;
	AVL_uint16 usSSI;
	AVL_int16  uiRFPower=0;
	AVL_int16  iRFPower=0;
    AVL_DemodMode current_mode;
    AVL_uchar ucSSI = 0;// range 0 - 100  

    r = GetMode_Demod(&current_mode,uiChipNo);
    switch(current_mode)
    {
        case AVL_DVBTX:
            pTuner = &global_tuner_t_c;
			break;
		case AVL_DVBSX:
            pTuner = &global_tuner_s;
			break;
        case AVL_ISDBT:
            pTuner = &global_tuner_t_c;
			break;
        case AVL_DVBC: 
            printf("can not calculate the RF Power\n");  
			return r;
			break;
		default:
            pTuner = &global_tuner_t_c;
            break;
		
	}
    /******** get RFPower *******/
    
	AVL_Demod_I2CBypassOn(uiChipNo);
	R848_GetGain(pTuner, &uiRFGain);//get gain
	AVL_Demod_I2CBypassOff(uiChipNo);
	AVL_Demod_GetSSI(&usSSI,uiChipNo);// get SSI					
	uiRFPower=R848_RFPowerCalc(uiRFGain,usSSI);//Calc Power
    printf("The RF Power is %d dBm\n",uiRFPower); 
	iRFPower=uiRFPower;
    AVL_Demod_DVBTxGetNorDigSSI(&ucSSI,iRFPower,uiChipNo);
	printf("SSI is %d\n",ucSSI);
    return (r);
}

static void DVB_Tx_locksignal_example(AVL_uint32 Freq_Khz,AVL_uint16 BandWidth_Khz)
{
    AVL_uint16 index = 0;
    AVL_uchar nLockFlag = 0;
    for(;index < g_nChannel_ts_total; index++)
    {
        nLockFlag = 0;
        if(global_channel_ts_table[index].channel_type == AVL_DVBTx_Standard_T)//DVB-T signal..
        {
            AVL_LockChannel_T(Freq_Khz,BandWidth_Khz,global_channel_ts_table[index].dvbt_hierarchy_layer);
        }
        else if(global_channel_ts_table[index].channel_type == AVL_DVBTx_Standard_T2)//DVB-T2 signal, do not process FEF...
        {
            AVL_LockChannel_T2( Freq_Khz, BandWidth_Khz,global_channel_ts_table[index].channel_profile, global_channel_ts_table[index].data_plp_id);
        }

        AVL_Check_LockStatus(&nLockFlag);
    }
    
    return;
}

///just for debug
static void AVL_PrintVersion(void)
{
    AVL_DemodVersion sVerInfo;
    AVL_int32 uiChipNo = g_ChipNo ;

    AVL_Demod_GetVersion(&sVerInfo,uiChipNo);
    printf("SDK Version,Major-Minor-Bulid:%d-%d-%d\n",\
    sVerInfo.stAPI.ucMajor,sVerInfo.stAPI.ucMinor,sVerInfo.stAPI.usBuild);

    printf("Patch Version,Major-Minor-Bulid:%d-%d-%d\n",\
    sVerInfo.stPatch.ucMajor,sVerInfo.stPatch.ucMinor,sVerInfo.stPatch.usBuild);
}

//Scan channel Example
static void AVL_DVBTxChannelScan_example(void)
{
        AVL_uint32 uiChannelStart = 618000;//KHz
        AVL_uint32 uiChannelStop = 691000;//KHz
        AVL_uint32 uiChannelUse = 0;
        for(uiChannelUse = uiChannelStart;uiChannelUse < uiChannelStop;uiChannelUse = uiChannelUse+8000)
        {
            AVL_ScanChannel_Tx(uiChannelUse, 8*1000); 
        }
}

/*
    ||=====                                                    ====            ====
    ||       \\                                     ||         ||\\            //||
    ||        ||                                    ||         || \\          // ||
    ||       //                                     ||         ||  \\        //  ||
    ||=====         //===\\     //===\\       //====||         ||   \\      //   ||   //===\\ 
    ||       \\    ||=====//   ||     ||     ||     ||         ||    \\    //    ||  ||=====//
    ||        \\   ||          ||     ||     ||     ||         ||     \\  //     ||  ||
    ||         \\   \\====//    \\===//\\     \\===//\\        ||      \\//      ||   \\====//
*/
//****************************************************************************//
//****************************************************************************//
//       User can ignore the detailed implementation in the above functions.  //
//       Just need to invoke above functions as main() function does.         //
//****************************************************************************//
//****************************************************************************//
int main(void)
{
    int return_code = AVL_EC_OK;
    AVL_uchar nLockFlag = 0;
    AVL_int32 uiChipNo = g_ChipNo ;

    //Open  I2C device. IBSP functions should be customized according specific application platform
    return_code = AVL_IBSP_Initialize(I2C_SERVER_IP,I2C_SERVER_PORT,nullptr,0);
    if(return_code != AVL_EC_OK)
    {
        printf("Failed to initialize the BSP!\n");
        return return_code;
    }



//Reset Demod
//======================================================//
    return_code = AVL_IBSP_Reset();
    if(return_code != AVL_EC_OK)
    {
        printf("Failed to Resed demod via BSP!\n");
        return return_code;
    }
//======================================================//

// initialize demod and tuner
//======================================================//
    return_code = AVL_Init();
    if(return_code != AVL_EC_OK)
    {
        printf("Failed to Init demod!\n");
        return return_code;
    }
    AVL_PrintVersion();
//======================================================//
/*
// DVBTx example
//======================================================//
    //Set demod work mode to DVB-Tx
    AVL_SetWorkMode(AVL_DVBTX);
    //DVBTx Channel Scan Functions
    AVL_ScanChannel_Tx(666*1000, 8*1000);
    DVB_Tx_locksignal_example(666*1000, 8*1000);
//======================================================//


// DVBSx example
//======================================================//
    //Set demod work mode to DVB-Sx
    AVL_SetWorkMode(AVL_DVBSX);
   // DiseqcExamples();
   // BlindScanExamples();
    AVL_LockChannel_DVBSx(1000*1000,30*1000);
    AVL_Check_LockStatus(&nLockFlag);
//======================================================//
*/

// DVBC example
//======================================================//
    //Set demod work mode to DVB-C
    AVL_SetWorkMode(AVL_DVBC);
    AVL_LockChannel_DVBC(138*1000,5217);
    AVL_Check_LockStatus(&nLockFlag);
//======================================================//


#if 0
// ISDBT example
//======================================================//
    //Set demod work mode to ISDBT
    AVL_SetWorkMode(AVL_ISDBT);
    AVL_LockChannel_ISDBT(666*1000,6000);
    AVL_Check_LockStatus(&nLockFlag);
//======================================================//
#endif

    //get lock status to determine whether it's needed to get signal status
    AVL_Check_LockStatus(&nLockFlag);

    if(nLockFlag == 1)
    {
        for(;;)
        {
                  AVL_IBSP_Delay(1000);
        //========get SSI===========================================//
                  //get_SSI_info();
                  AVL_R848_get_RFPower_info();   
        //========get SQI===========================================//
                  get_SQI_info();
                
        //========get SNR===========================================//
                  get_SNR_info();
                
        //========get PER===========================================//
                  get_PER_info();

       
        }        
    }

    return (return_code);
}

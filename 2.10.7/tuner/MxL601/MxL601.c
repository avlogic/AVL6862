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



#include "MxL601.h"
#include "MxL601_TunerApi.h"

#define IF_OUT_FREQUENCY            IF_36MHz
#define IF_OUT_PATH                 IF_PATH1            //pin 9 & 10
#define IF_INVERSION_TYPE           MXL_ENABLE//not inverted
#define AGC_CONTROL_PATH            AGC1                //AGC control path used
#define AGC_CONTROL_TYPE            AGC_EXTERNAL//AGC_SELF//        //external
#define CHIP_ID_SharpVA4M1DE2169    0xff

AVL_uint32 MxL601_Initialize(AVL_Tuner *pTuner)
{
    MXL_STATUS status; 
    MXL_COMMAND_T apiCmd;
    UINT8 tuner_addr;
    AVL_uint32 if_output_khz;

    tuner_addr=(UINT8)pTuner->usTunerI2CAddr;
    if_output_khz = pTuner->uiIFHz/1000;

    // Soft Reset MxL601
    apiCmd.commandId = MXL_DEV_SOFT_RESET_CFG;
    apiCmd.MxLIf.cmdResetCfg.I2cSlaveAddr = tuner_addr; 
    status = MxLWare601_API_ConfigDevice(&apiCmd);

    // Overwrite Default
    apiCmd.commandId = MXL_DEV_OVERWRITE_DEFAULT_CFG;
    apiCmd.MxLIf.cmdOverwriteDefault.I2cSlaveAddr = tuner_addr;
    apiCmd.MxLIf.cmdOverwriteDefault.SingleSupply_3_3V = MXL_ENABLE;
    status = MxLWare601_API_ConfigDevice(&apiCmd);

      // Xtal Setting
    apiCmd.commandId = MXL_DEV_XTAL_SET_CFG;
    apiCmd.MxLIf.cmdXtalCfg.I2cSlaveAddr = tuner_addr;
    apiCmd.MxLIf.cmdXtalCfg.XtalFreqSel = XTAL_16MHz;
    apiCmd.MxLIf.cmdXtalCfg.XtalCap = 12;
    apiCmd.MxLIf.cmdXtalCfg.ClkOutEnable = MXL_ENABLE;
    apiCmd.MxLIf.cmdXtalCfg.ClkOutDiv = MXL_DISABLE;
    apiCmd.MxLIf.cmdXtalCfg.SingleSupply_3_3V = MXL_ENABLE;

    apiCmd.MxLIf.cmdXtalCfg.XtalSharingMode = MXL_DISABLE;
    status = MxLWare601_API_ConfigDevice(&apiCmd);

    // IF Out setting
    apiCmd.commandId = MXL_DEV_IF_OUT_CFG;
    apiCmd.MxLIf.cmdIfOutCfg.I2cSlaveAddr = tuner_addr;
    //apiCmd.MxLIf.cmdIfOutCfg.IFOutFreqinKHz = IF_OUT_FREQUENCY;//availink customized
    apiCmd.MxLIf.cmdIfOutCfg.IFInversion = IF_INVERSION_TYPE;
    apiCmd.MxLIf.cmdIfOutCfg.GainLevel = 10;//11
    apiCmd.MxLIf.cmdIfOutCfg.PathSel = IF_OUT_PATH;//availink customized
    apiCmd.MxLIf.cmdIfOutCfg.ManualFreqSet=MXL_ENABLE;
    //apiCmd.MxLIf.cmdIfOutCfg.ManualIFOutFreqinKHzInKHz=if_output_khz;
    apiCmd.MxLIf.cmdIfOutCfg.ManualIFOutFreqInKHz= if_output_khz;
    //apiCmd.MxLIf.cmdIfOutCfg.ManualIFOutFreqInKHz=36000;

    status = MxLWare601_API_ConfigDevice(&apiCmd);

    // AGC Setting
    apiCmd.commandId = MXL_TUNER_AGC_CFG;
    apiCmd.MxLIf.cmdAgcSetCfg.I2cSlaveAddr = tuner_addr;
    apiCmd.MxLIf.cmdAgcSetCfg.AgcSel = AGC_CONTROL_PATH;//availink customized
    apiCmd.MxLIf.cmdAgcSetCfg.AgcType = AGC_CONTROL_TYPE;//availink customized
    apiCmd.MxLIf.cmdAgcSetCfg.SetPoint = 66;
    apiCmd.MxLIf.cmdAgcSetCfg.AgcPolarityInverstion = MXL_DISABLE;
    status = MxLWare601_API_ConfigTuner(&apiCmd);

    // Application Mode setting
    apiCmd.commandId = MXL_TUNER_MODE_CFG;
    apiCmd.MxLIf.cmdModeCfg.I2cSlaveAddr = tuner_addr;
    if (pTuner->eDTVMode == DTVMode_DVBTX)
    {
        apiCmd.MxLIf.cmdModeCfg.SignalMode = DIG_DVB_T_DTMB;
    }
    else if (pTuner->eDTVMode == DTVMode_DVBC)
    {
        apiCmd.MxLIf.cmdModeCfg.SignalMode = DIG_DVB_C;
    }
    else if(pTuner->eDTVMode == DTVMode_ISDBT)
    {
        apiCmd.MxLIf.cmdModeCfg.SignalMode = DIG_ISDBT_ATSC;
    }
    else
    {
        apiCmd.MxLIf.cmdModeCfg.SignalMode = DIG_DVB_T_DTMB;
    }
    apiCmd.MxLIf.cmdModeCfg.XtalFreqSel = XTAL_16MHz;
    apiCmd.MxLIf.cmdModeCfg.IFOutGainLevel = 11;

    apiCmd.MxLIf.cmdModeCfg.IFOutFreqinKHz=if_output_khz;//IF_OUT_FREQUENCY;
    status = MxLWare601_API_ConfigTuner(&apiCmd);

    return status;
}

AVL_uint32 MxL601_Lock(AVL_Tuner *pTuner)
{
    MXL_STATUS status; 
    MXL_COMMAND_T apiCmd;
    UINT8 tuner_addr;
    AVL_uint32  band_width_khz;

    tuner_addr=(UINT8)pTuner->usTunerI2CAddr;

    // Channel frequency & bandwidth setting
    apiCmd.commandId = MXL_TUNER_CHAN_TUNE_CFG;
    apiCmd.MxLIf.cmdChanTuneCfg.I2cSlaveAddr = tuner_addr;
    apiCmd.MxLIf.cmdChanTuneCfg.TuneType = VIEW_MODE;

    band_width_khz=pTuner->uiBandwidthHz/1000;
    if( band_width_khz <= 6000)//6MHz
    {
        if(pTuner->eDTVMode == DTVMode_DVBC)
        {
            apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = ANA_TV_DIG_CABLE_BW_6MHz;
        }
        else if(pTuner->eDTVMode == DTVMode_DVBTX || pTuner->eDTVMode == DTVMode_DTMB  || pTuner->eDTVMode == DTVMode_ISDBT)
        {
            apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = DIG_TERR_BW_6MHz;
        }
        else
        {
            apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = DIG_TERR_BW_6MHz;
        }
    }
    else if ( band_width_khz <= 7000)
    {
        if(pTuner->eDTVMode == DTVMode_DVBC)
        {
            apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = ANA_TV_DIG_CABLE_BW_7MHz;
        }
        else if(pTuner->eDTVMode == DTVMode_DVBTX || pTuner->eDTVMode == DTVMode_DTMB)
        {
            apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = DIG_TERR_BW_7MHz;
        }
        else
        {
            apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = DIG_TERR_BW_7MHz;
        }
    }
    else
    {
        if(pTuner->eDTVMode == DTVMode_DVBC)
        {
            apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = ANA_TV_DIG_CABLE_BW_8MHz;
        }
        else if(pTuner->eDTVMode == DTVMode_DVBTX || pTuner->eDTVMode == DTVMode_DTMB  || pTuner->eDTVMode == DTVMode_ISDBT)
        {
            apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = DIG_TERR_BW_8MHz;
        }
        else
        {
            apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = DIG_TERR_BW_8MHz;
        }
    }

    apiCmd.MxLIf.cmdChanTuneCfg.FreqInHz = pTuner->uiRFFrequencyHz;

    if(pTuner->eDTVMode == DTVMode_DVBTX || pTuner->eDTVMode == DTVMode_DTMB)
    {
        apiCmd.MxLIf.cmdChanTuneCfg.SignalMode = DIG_DVB_T_DTMB;
    }
    else if (pTuner->eDTVMode == DTVMode_DVBC)
    {
        apiCmd.MxLIf.cmdChanTuneCfg.SignalMode = DIG_DVB_C;
    }
    else if(pTuner->eDTVMode == DTVMode_ISDBT)
    {
        apiCmd.MxLIf.cmdModeCfg.SignalMode = DIG_ISDBT_ATSC;
    }
    else
    {
        apiCmd.MxLIf.cmdChanTuneCfg.SignalMode = DIG_DVB_T_DTMB;
    }
    apiCmd.MxLIf.cmdChanTuneCfg.XtalFreqSel = XTAL_16MHz;
    status = MxLWare601_API_ConfigTuner(&apiCmd);

    // Power up setting
    apiCmd.commandId = MXL_TUNER_POWER_UP_CFG;
    apiCmd.MxLIf.cmdTunerPoweUpCfg.I2cSlaveAddr = tuner_addr;
    apiCmd.MxLIf.cmdTunerPoweUpCfg.Enable = MXL_ENABLE;
    status = MxLWare601_API_ConfigTuner(&apiCmd);

    // Sequencer setting
    apiCmd.commandId = MXL_TUNER_START_TUNE_CFG;
    apiCmd.MxLIf.cmdStartTuneCfg.I2cSlaveAddr = tuner_addr;
    apiCmd.MxLIf.cmdStartTuneCfg.StartTune = MXL_ENABLE;
    status = MxLWare601_API_ConfigTuner(&apiCmd);

    return status;
}

AVL_uint32 MxL601_GetLockStatus(struct AVL_Tuner * pTuner )
{
    MXL_STATUS status; 
    MXL_COMMAND_T apiCmd;

    apiCmd.commandId = MXL_TUNER_LOCK_STATUS_REQ;
    apiCmd.MxLIf.cmdTunerLockReq.I2cSlaveAddr = (UINT8)pTuner->usTunerI2CAddr;
    status=MxLWare601_API_GetTunerStatus(&apiCmd);
    if (MXL_TRUE == status)
    {
        if (MXL_LOCKED == apiCmd.MxLIf.cmdTunerLockReq.RfSynLock &&
            MXL_LOCKED == apiCmd.MxLIf.cmdTunerLockReq.RefSynLock)
        {
            pTuner->ucTunerLocked = 1;
        }
        else
        {
            pTuner->ucTunerLocked = 0;
        }
    }

    return status;
}

AVL_uint32 MxL601_GetRFStrength(AVL_Tuner * pTuner,AVL_int32 *power)
{   
    MXL_STATUS status; 
    MXL_COMMAND_T apiCmd;

    apiCmd.commandId = MXL_TUNER_RX_PWR_REQ;
    apiCmd.MxLIf.cmdResetCfg.I2cSlaveAddr = (UINT8)pTuner->usTunerI2CAddr; 
    status = MxLWare601_API_GetTunerStatus(&apiCmd);

    AVL_int32  rf_offset = 0;
    if(pTuner->uiRFFrequencyHz <= 100000000)
        rf_offset = 50;
    else if(pTuner->uiRFFrequencyHz <= 250000000)
        rf_offset = -50;
    else if(pTuner->uiRFFrequencyHz <= 350000000)
        rf_offset = -100;
    else
        rf_offset = -150;

    *power=((AVL_int32)apiCmd.MxLIf.cmdTunerPwrReq.RxPwr - rf_offset)/100;//14db gain

    return status;
}



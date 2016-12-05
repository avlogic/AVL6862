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


#include <stdio.h>
#include "MxL601_TunerApi.h"
#include "MxL601_OEM_Drv.h"

#define MXL601_I2C_ADDR     0x60

#define MXL601_DEFAULT_SINGLE_POWER_SUPPLY  MXL_ENABLE   // Dual power supply
#define MXL601_DEFAULT_XTAL_FREQ_SEL        XTAL_16MHz    // 16MHz
#define MXL601_DEFAULT_XTAL_CAP             25            // 25 PF
#define MXL601_DEFAULT_XTAL_CLOCK_OUT       MXL_ENABLE    // Enable clock out
#define MXL601_DEFAULT_XTAL_CLOCK_DIV       MXL_DISABLE   // Disable clock div
#define MXL601_DEFAULT_IF_PATH              IF_PATH2      // IF path 2
#define MXL601_DEFAULT_IF_FREQ_SEL          IF_4MHz       // 4MHz
#define MXL601_DEFAULT_IF_INVERSION         MXL_ENABLE // IF spectrum inversion
#define MXL601_DEFAULT_IF_GAIN              11  // For digital range is 5 to 11
#define MXL601_DEFAULT_AGC_TYPE             AGC_EXTERNAL  // External AGC
#define MXL601_DEFAULT_AGC_SEL              AGC2          // Select AGC2
#define MXL601_DEFAULT_AGC_SET_POINT        66  // Default value for Self AGC

// Define MxL601 device power supply, Xtal, IF out and AGC setting
typedef struct
{

  MXL_BOOL Single_3_3_v_Supply; // Define Tuner is single 3.3v power supply or not.
  MXL_XTAL_FREQ_E XtalFreqSel;  // XTAL frequency selection, either 16MHz or 24MHz
  UINT8 XtalCap;                // XTAL capacity
  MXL_BOOL XtalClkOut;          // XTAL clock out enable or disable
  MXL_BOOL XtalClkDiv;          // Define if clock out freq is divided by 4 or not
  UINT8 SignalMode;             // Tuner work mode, refers MXL_SIGNAL_MODE_E definition
  UINT8 BandWidth;              // signal band width in MHz unit
  MXL_IF_PATH_E IFPath;                 // define which IF path is selected
  MXL_IF_FREQ_E IFFreqSel;      // IF out signel frequency selection. Refers MXL_IF_FREQ_E define.
  MXL_BOOL IFInversion;         // IF spectrum is inverted or not
  UINT8 IFGain;                 // IF out gain level
  MXL_AGC_TYPE_E AgcType;       // AGC mode selection, self or closed loop
  MXL_AGC_ID_E AgcSel;          // AGC selection, AGC1 or AGC2
  UINT8 AgcSetPoint;            // AGC attack point set value
} MXL601_CHARACTER_SET_T;

static MXL601_CHARACTER_SET_T MxL601_Default_Set =
{
  MXL601_DEFAULT_SINGLE_POWER_SUPPLY, // power supply type
  MXL601_DEFAULT_XTAL_FREQ_SEL,  // Xtal freq selection
  MXL601_DEFAULT_XTAL_CAP,       // Xtal cap
  MXL601_DEFAULT_XTAL_CLOCK_OUT, // clock out
  MXL601_DEFAULT_XTAL_CLOCK_DIV, // clock div
  ANA_PAL_BG,  // PAL_BG
  8,  // 8MHz
  MXL601_DEFAULT_IF_PATH, // IF path
  MXL601_DEFAULT_IF_FREQ_SEL, // IF freq selection
  MXL601_DEFAULT_IF_INVERSION, // IF spectrum inversion
  MXL601_DEFAULT_IF_GAIN,  // IF gain
  MXL601_DEFAULT_AGC_TYPE, // AGC type, self or external
  MXL601_DEFAULT_AGC_SEL,  // AGC selection
  MXL601_DEFAULT_AGC_SET_POINT // AGC set point, effective for self AGC
};

void MxL601_API_Init(void)
{
  MXL_STATUS status;
  MXL_COMMAND_T apiCmd;

  // Soft Reset MxL601
  apiCmd.commandId = MXL_DEV_SOFT_RESET_CFG;
  apiCmd.MxLIf.cmdResetCfg.I2cSlaveAddr = MXL601_I2C_ADDR;
  status = MxLWare601_API_ConfigDevice(&apiCmd);

  // Overwrite Default
  apiCmd.commandId = MXL_DEV_OVERWRITE_DEFAULT_CFG;
  apiCmd.MxLIf.cmdOverwriteDefault.I2cSlaveAddr = MXL601_I2C_ADDR;
  apiCmd.MxLIf.cmdOverwriteDefault.SingleSupply_3_3V = MxL601_Default_Set.Single_3_3_v_Supply;
  status = MxLWare601_API_ConfigDevice(&apiCmd);

  // Xtal Setting
  apiCmd.commandId = MXL_DEV_XTAL_SET_CFG;
  apiCmd.MxLIf.cmdXtalCfg.I2cSlaveAddr = MXL601_I2C_ADDR;
  apiCmd.MxLIf.cmdXtalCfg.XtalFreqSel = MxL601_Default_Set.XtalFreqSel;
  apiCmd.MxLIf.cmdXtalCfg.XtalCap = MxL601_Default_Set.XtalCap;
  apiCmd.MxLIf.cmdXtalCfg.ClkOutEnable = MxL601_Default_Set.XtalClkOut;
  apiCmd.MxLIf.cmdXtalCfg.ClkOutDiv = MxL601_Default_Set.XtalClkDiv;
  apiCmd.MxLIf.cmdXtalCfg.SingleSupply_3_3V = MxL601_Default_Set.Single_3_3_v_Supply;
  status = MxLWare601_API_ConfigDevice(&apiCmd);

  // Power up setting
  apiCmd.commandId = MXL_TUNER_POWER_UP_CFG;
  apiCmd.MxLIf.cmdTunerPoweUpCfg.I2cSlaveAddr = MXL601_I2C_ADDR;
  apiCmd.MxLIf.cmdTunerPoweUpCfg.Enable = MXL_ENABLE;
  status = MxLWare601_API_ConfigTuner(&apiCmd);
}

void MxL601_API_SetAppMode(UINT8 AppMode)
{
  MXL_STATUS status;
  MXL_COMMAND_T apiCmd;
  UINT32 IFAbsFreqInKHz[] = {3650, 4000, 4100, 4150, 4500, 4570, 5000, 5380, 6000, 6280, 7200, 8250, 35250, 36000, 36150, 36650, 44000};

  // IF Out setting
  apiCmd.commandId = MXL_DEV_IF_OUT_CFG;
  apiCmd.MxLIf.cmdIfOutCfg.I2cSlaveAddr = MXL601_I2C_ADDR;
  apiCmd.MxLIf.cmdIfOutCfg.IFOutFreq = MxL601_Default_Set.IFFreqSel;
  apiCmd.MxLIf.cmdIfOutCfg.IFInversion = MxL601_Default_Set.IFInversion;
  apiCmd.MxLIf.cmdIfOutCfg.GainLevel = MxL601_Default_Set.IFGain;
  apiCmd.MxLIf.cmdIfOutCfg.PathSel = MxL601_Default_Set.IFPath;
  apiCmd.MxLIf.cmdIfOutCfg.ManualFreqSet = MXL_DISABLE;
  apiCmd.MxLIf.cmdIfOutCfg.ManualIFOutFreqInKHz = 0;
  status = MxLWare601_API_ConfigDevice(&apiCmd);

  // AGC Setting
  apiCmd.commandId = MXL_TUNER_AGC_CFG;
  apiCmd.MxLIf.cmdAgcSetCfg.I2cSlaveAddr = MXL601_I2C_ADDR;
  apiCmd.MxLIf.cmdAgcSetCfg.AgcSel = MxL601_Default_Set.AgcSel;
  apiCmd.MxLIf.cmdAgcSetCfg.AgcType = MxL601_Default_Set.AgcType;
  apiCmd.MxLIf.cmdAgcSetCfg.SetPoint = MxL601_Default_Set.AgcSetPoint;
  apiCmd.MxLIf.cmdAgcSetCfg.AgcPolarityInverstion = MXL_DISABLE;
  status = MxLWare601_API_ConfigTuner(&apiCmd);

  // Application Mode setting
  apiCmd.commandId = MXL_TUNER_MODE_CFG;
  apiCmd.MxLIf.cmdModeCfg.I2cSlaveAddr = MXL601_I2C_ADDR;
  apiCmd.MxLIf.cmdModeCfg.SignalMode = (MXL_SIGNAL_MODE_E)AppMode;
  apiCmd.MxLIf.cmdModeCfg.IFOutFreqinKHz = IFAbsFreqInKHz[MxL601_Default_Set.IFFreqSel];
  apiCmd.MxLIf.cmdModeCfg.XtalFreqSel = MxL601_Default_Set.XtalFreqSel;
  apiCmd.MxLIf.cmdModeCfg.IFOutGainLevel = MxL601_Default_Set.IFGain;
  status = MxLWare601_API_ConfigTuner(&apiCmd);

  MxL601_Default_Set.SignalMode = AppMode;
}

void MxL601_API_ChannelTune(UINT32 RfCenterFreqInHz, UINT8 BandWidthInMHz)
{
  MXL_STATUS status;
  MXL_COMMAND_T apiCmd;
  MXL_SIGNAL_MODE_E TunerAppMode;
  MXL_BW_E bandWidth = ANA_TV_DIG_CABLE_BW_8MHz;

  TunerAppMode = (MXL_SIGNAL_MODE_E)MxL601_Default_Set.SignalMode;

  if (TunerAppMode <= DIG_DVB_C)
  {
    switch(BandWidthInMHz)
    {
      case 6: bandWidth = ANA_TV_DIG_CABLE_BW_6MHz; break;
      case 7: bandWidth = ANA_TV_DIG_CABLE_BW_7MHz; break;
      case 8: bandWidth = ANA_TV_DIG_CABLE_BW_8MHz; break;
      default: break;
    }
  }
  else
  {
    switch(BandWidthInMHz)
    {
      case 6: bandWidth = DIG_TERR_BW_6MHz; break;
      case 7: bandWidth = DIG_TERR_BW_7MHz; break;
      case 8: bandWidth = DIG_TERR_BW_8MHz; break;
      default: break;
    }
  }
  // Channel frequency & bandwidth setting
  apiCmd.commandId = MXL_TUNER_CHAN_TUNE_CFG;
  apiCmd.MxLIf.cmdChanTuneCfg.I2cSlaveAddr = MXL601_I2C_ADDR;
  apiCmd.MxLIf.cmdChanTuneCfg.TuneType = VIEW_MODE;
  apiCmd.MxLIf.cmdChanTuneCfg.BandWidth = bandWidth;
  apiCmd.MxLIf.cmdChanTuneCfg.FreqInHz = RfCenterFreqInHz;
  apiCmd.MxLIf.cmdChanTuneCfg.SignalMode = TunerAppMode;
  apiCmd.MxLIf.cmdChanTuneCfg.XtalFreqSel = MxL601_Default_Set.XtalFreqSel;
  status = MxLWare601_API_ConfigTuner(&apiCmd);

  // Sequencer setting, disable tune
  apiCmd.commandId = MXL_TUNER_START_TUNE_CFG;
  apiCmd.MxLIf.cmdStartTuneCfg.I2cSlaveAddr = MXL601_I2C_ADDR;
  apiCmd.MxLIf.cmdStartTuneCfg.StartTune = MXL_DISABLE;
  status = MxLWare601_API_ConfigTuner(&apiCmd);

  // Sequencer setting, enable tune
  apiCmd.commandId = MXL_TUNER_START_TUNE_CFG;
  apiCmd.MxLIf.cmdStartTuneCfg.I2cSlaveAddr = MXL601_I2C_ADDR;
  apiCmd.MxLIf.cmdStartTuneCfg.StartTune = MXL_ENABLE;
  status = MxLWare601_API_ConfigTuner(&apiCmd);

  MxL601_Default_Set.BandWidth = BandWidthInMHz;
}

void MxL601_API_GetLockStatus(UINT8* RfLock, UINT8* RefLock)
{
  MXL_COMMAND_T apiCmd;

  // Read back Tuner lock status
  apiCmd.commandId = MXL_TUNER_LOCK_STATUS_REQ;
  apiCmd.MxLIf.cmdTunerLockReq.I2cSlaveAddr = MXL601_I2C_ADDR;

  if (MXL_TRUE == MxLWare601_API_GetTunerStatus(&apiCmd))
  {
       *RfLock = apiCmd.MxLIf.cmdTunerLockReq.RfSynLock;
       *RefLock = apiCmd.MxLIf.cmdTunerLockReq.RefSynLock;
  }
  return;
}

int main(void)
{
  UINT8 RfLock, RefLock;

  // Here user can change MxL601 character setting in MxL601_Device_Default_Set
  // MxL601_Default_Set.Single_3_3_v_Supply =
  // MxL601_Default_Set.XtalFreqSel =
  // MxL601_Default_Set.XtalCap =
  // MxL601_Default_Set.XtalClkOut =
  // MxL601_Default_Set.XtalClkDiv =
  // MxL601_Default_Set.SignalMode =
  // MxL601_Default_Set.BandWidth =
  // MxL601_Default_Set.IFPath =
  // MxL601_Default_Set.IFFreqSel =
  // MxL601_Default_Set.IFInversion =
  // MxL601_Default_Set.IFGain =
  // MxL601_Default_Set.AgcType =
  // MxL601_Default_Set.AgcSel =
  // MxL601_Default_Set.AgcSetPoint =

  // Tuner initialization only need be called once.
  MxL601_API_Init();

  // Set appllication mode and related IF out, AGC setting parameters
  MxL601_API_SetAppMode(ANA_PAL_BG);

  // Channel tune at certain frequency point
  MxL601_API_ChannelTune(530000000, 8);

  // Inquire Tuner lock status
  MxL601_API_GetLockStatus(&RfLock, &RefLock);
  if ((MXL_LOCKED == RfLock) && (MXL_LOCKED == RefLock))
    printf(" Tuner locked\n");
  else
    printf(" Tuner unlocked\n");
}



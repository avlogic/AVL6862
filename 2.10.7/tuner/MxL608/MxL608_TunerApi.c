/*******************************************************************************
 *
 * FILE NAME          : MxL608_TunerApi.cpp
 * 
 * AUTHOR             : Dong Liu 
 *                    : Joy Zhang
 *                      
 *
 * DATE CREATED       : 3/14/2013
 *                    : 7/30/2013
 *
 * DESCRIPTION        : This file contains MxL608 driver APIs
 *
 *
 *******************************************************************************
 *                Copyright (c) 2011, MaxLinear, Inc.
 ******************************************************************************/

#include <math.h>
#include "MxL608_TunerApi.h"
#include "MxL608_TunerCfg.h"
#include "MxL608_Debug.h"

/* MxLWare Driver version for MxL608 */
const UINT8_608 MxLWare608DrvVersion[] = {1, 1, 1, 7, 0}; 

/* OEM Data pointer array */
void * MxL608_OEM_DataPtr[MXL608_MAX_NUM_DEVICES];

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgDrvInit
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API must be called prior to any other API function.  
--|                 Cannot be called more than once.  
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgDrvInit(UINT8_608 devId, void* oemDataPtr)
{
  MXL608_STATUS status = MXL608_SUCCESS;

  if (oemDataPtr)
  {
    if (devId <= MXL608_MAX_NUM_DEVICES) MxL608_OEM_DataPtr[devId] = oemDataPtr;
    else status = MXL608_INVALID_PARAMETER;
  }

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgDevSoftReset
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API is used to reset MxL608 tuner device. After reset,
--|                 all the device regiaters and modules will be set to power-on  
--|                 default state. 
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_FAILED 
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgDevSoftReset(UINT8_608 devId)
{
  UINT8_608 status = MXL608_SUCCESS;
  
  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  // Write 0xFF with 0 to reset tuner 
  status = MxLWare608_OEM_WriteRegister(devId, AIC_RESET_REG, 0x00); 
  
  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgDevOverwriteDefaults
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : Register(s) that requires default values to be overwritten 
--|                 during initialization
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgDevOverwriteDefaults(UINT8_608 devId, 
                                                  MXL608_BOOL singleSupply_3_3V)
{
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 readData = 0;

  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  status |= MxL608_Ctrl_ProgramRegisters(devId, MxL608_OverwriteDefaults);

  status |= MxLWare608_OEM_WriteRegister(devId, 0x00, 0x01);
  status |= MxLWare608_OEM_ReadRegister(devId, 0x31, &readData);
  readData &= 0x2F;
  readData |= 0xD0;
  status |= MxLWare608_OEM_WriteRegister(devId, 0x31, readData);
  status |= MxLWare608_OEM_WriteRegister(devId, 0x00, 0x00);


  /* If Single supply 3.3v is used */
  if (MXL608_ENABLE == singleSupply_3_3V)
    status |= MxLWare608_OEM_WriteRegister(devId, MAIN_REG_AMP, 0x04);

  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgDevXtal
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API is used to configure XTAL settings of MxL608 tuner
--|                 device. XTAL settings include frequency, capacitance & 
--|                 clock out
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgDevXtal(UINT8_608 devId, MXL608_XTAL_SET_CFG_T xtalCfg)
{
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 control = 0;

  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  // XTAL freq and cap setting, Freq set is located at bit<5>, cap bit<4:0> 
  // and  XTAL clock out enable <0>
  if ((xtalCfg.xtalFreqSel == MXL608_XTAL_16MHz) || (xtalCfg.xtalFreqSel == MXL608_XTAL_24MHz))
  {
    control = (UINT8_608)((xtalCfg.xtalFreqSel << 5) | (xtalCfg.xtalCap & 0x1F));  
    control |= (xtalCfg.clkOutEnable << 7);
    status = MxLWare608_OEM_WriteRegister(devId, XTAL_CAP_CTRL_REG, control);

    // XTAL frequency div 4 setting <1> 
    control = (0x01 & (UINT8_608)xtalCfg.clkOutDiv);
    
    // XTAL sharing mode
    if (xtalCfg.XtalSharingMode == MXL608_ENABLE) 
    {
      control |= 0x40;
      // program Clock out div & Xtal sharing
      status |= MxLWare608_OEM_WriteRegister(devId, XTAL_ENABLE_DIV_REG, control);
      status |= MxLWare608_OEM_WriteRegister(devId, XTAL_EXT_BIAS_REG, 0x80);
    }
    else 
    {
      control &= 0x01;
      // program Clock out div & Xtal sharing
      status |= MxLWare608_OEM_WriteRegister(devId, XTAL_ENABLE_DIV_REG, control); 
      status |= MxLWare608_OEM_WriteRegister(devId, XTAL_EXT_BIAS_REG, 0x0A);
    }

    // Main regulator re-program
    if (MXL608_ENABLE == xtalCfg.singleSupply_3_3V)
      status |= MxLWare608_OEM_WriteRegister(devId, MAIN_REG_AMP, 0x14);
  }
  else 
    status |= MXL608_INVALID_PARAMETER;
  
  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgDevPowerMode
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function configures MxL608 power mode 
--|                 If Power mode is set to standby and application environment 
--|                 is south africa area, then the standbyLt shall be set as 0x06
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgDevPowerMode(UINT8_608 devId, 
                                          MXL608_PWR_MODE_E powerMode, 
                                          MXL608_BOOL enableLoopthrough, 
                                          UINT8_608 standbyLt)
{
  UINT8_608 status = MXL608_SUCCESS;

  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  switch(powerMode)
  {
    case MXL608_PWR_MODE_SLEEP:
      break;

    case MXL608_PWR_MODE_ACTIVE:
      status |= MxLWare608_OEM_WriteRegister(devId, TUNER_ENABLE_REG, MXL608_ENABLE);
      status |= MxLWare608_OEM_WriteRegister(devId, START_TUNE_REG, MXL608_ENABLE);
      status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x01);
      if (enableLoopthrough == MXL608_ENABLE)     
        status |= MxLWare608_OEM_WriteRegister(devId,DFE_SEQ_TUNE_RF1_BO_REG,0x0E);
      else 
        status |= MxLWare608_OEM_WriteRegister(devId,DFE_SEQ_TUNE_RF1_BO_REG,0x37);
      status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x00); 
      break;

    case MXL608_PWR_MODE_STANDBY:
      status |= MxLWare608_OEM_WriteRegister(devId, START_TUNE_REG, MXL608_DISABLE);
      status |= MxLWare608_OEM_WriteRegister(devId, TUNER_ENABLE_REG, MXL608_DISABLE);
      if ((standbyLt != 0) && (enableLoopthrough == MXL608_ENABLE))
      {
        status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x01);
        status |= MxLWare608_OEM_WriteRegister(devId,DFE_SEQ_TUNE_RF1_BO_REG,(standbyLt & 0x3F));
        status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x00);
      }	
      break;

    default:
      status |= MXL608_INVALID_PARAMETER;
  }

  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgDevGPO
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API configures GPO pin of MxL608 tuner device.
--|                 There is only 1 GPO pin available in MxL608 device.  
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgDevGPO(UINT8_608 devId, MXL608_GPO_STATE_E gpoState)
{
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 regData = 0;
  UINT8_608 gpoStateData = 0;

  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  switch(gpoState)
  {
    case MXL608_GPO_AUTO_CTRL:
    case MXL608_GPO_HIGH:
    case MXL608_GPO_LOW:
      status = MxLWare608_OEM_ReadRegister(devId, GPO_SETTING_REG, &regData);
      if (MXL608_GPO_AUTO_CTRL == gpoState)
        regData &= 0xEF; // 0x0A[4]
      else
      {
        regData &= 0xCF; // 0x0A[5:4] is clear to 0.
        if(gpoState == MXL608_GPO_HIGH) 
          gpoStateData = 0;
        else if(gpoState == MXL608_GPO_LOW) 
		  gpoStateData = 1;
        regData |= (0x10 | (gpoStateData << 5)); 
      }
     
      status |= MxLWare608_OEM_WriteRegister(devId, GPO_SETTING_REG, regData);
      break;

    default:
      status = MXL608_INVALID_PARAMETER;
  }

  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_ReqDevVersionInfo
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function is used to get MxL608 version information.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_ReqDevVersionInfo(UINT8_608 devId, 
                                            MXL608_VER_INFO_T* mxlDevVerInfoPtr)
{
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 readBack = 0;
  UINT8_608 k = 0;

  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  if (mxlDevVerInfoPtr)
  {
    status |= MxLWare608_OEM_ReadRegister(devId, CHIP_ID_REQ_REG, &readBack);
    mxlDevVerInfoPtr->chipId = (readBack & 0xFF); 

    status |= MxLWare608_OEM_ReadRegister(devId, CHIP_VERSION_REQ_REG, &readBack);
    mxlDevVerInfoPtr->chipVersion = (readBack & 0xFF); 

    MxL608_DLL_DEBUG0("Chip ID = 0x%d, Version = 0x%d \n", mxlDevVerInfoPtr->chipId, 
                                                        mxlDevVerInfoPtr->chipVersion);
    
    // Get MxLWare version infromation
    for (k = 0; k < MXL608_VERSION_SIZE; k++)
      mxlDevVerInfoPtr->mxlwareVer[k] = MxLWare608DrvVersion[k];
  }
  else 
    status = MXL608_INVALID_PARAMETER;

  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_ReqDevGPOStatus
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This API is used to get GPO pin's status information from
--|                 MxL608 tuner device.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_ReqDevGPOStatus(UINT8_608 devId, 
                                          MXL608_GPO_STATE_E* gpoStatusPtr)
{
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 regData = 0;
  UINT8_608 gpoStateData = 0;

  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  if (gpoStatusPtr)
  {
    status = MxLWare608_OEM_ReadRegister(devId, GPO_SETTING_REG, &regData);

    // GPO bit<5:4>
    if ((regData & 0x10) == 0) 
		*gpoStatusPtr = MXL608_GPO_AUTO_CTRL;
    else
    {
      gpoStateData = ((regData & 0x20) >> 5);
      if (gpoStateData == 0)
        *gpoStatusPtr = MXL608_GPO_HIGH;
      else 
        *gpoStatusPtr = MXL608_GPO_LOW;
	}
  }
  else
    status = MXL608_INVALID_PARAMETER;

  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_ReqDevPllState
--| 
--| AUTHOR        : Dong Liu
--|
--| DATE CREATED  : 11/26/2012  
--|
--| DESCRIPTION   : This API is used to check PLL state is normal or wrong. 
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_ReqDevPllState(UINT8_608 devId, MXL608_PLL_STATE_E* PllStatePtr)
{
  UINT8_608 status = MXL608_TRUE;
  UINT8_608 condition[3] = {0};
  UINT8_608 regAddr[7] = {0x2B, 0x30, 0x32, 0x34, 0x2F, 0x31, 0x33};
  UINT8_608 k, regData[7] = {0}; 

  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  *PllStatePtr = MXL608_PLL_STATE_NA; 

  for (k = 0; k < 7; k++)
  {
    // inquire PLL circuitry status and RSSI read back registers  
    status |= MxLWare608_OEM_ReadRegister(devId, regAddr[k], &regData[k]);
  }

  if ((MXL608_STATUS)status == MXL608_SUCCESS)
  {
    // Check 0x2B register 
    condition[0] = (regData[0] != 0x07)? 1 : 0;

    // Check if register 0x30, 0x32, 0x34 values are all 0 
    condition[1] = ((regData[1] == 0) && (regData[2] == 0) && (regData[3] == 0)) ? 1: 0; 

    // Check if register 0x30, 0x32, 0x34 values are all 0 
    condition[2] = ((regData[1] == regData[2]) && (regData[2] == regData[3]) 
                 && (regData[4] == regData[5]) && (regData[5] == regData[6])) ? 1: 0;

    if ((condition[0] == 1) || (condition[1] == 1) || (condition[2] == 1))
      *PllStatePtr = MXL608_PLL_STATE_WRONG; 
    else 
      *PllStatePtr = MXL608_PLL_STATE_NORMAL; 
  }

  MxL608_DLL_DEBUG0("Tuner PLL state = %d (0:Normal, 1:Wrong) \n", *PllStatePtr);
  return (MXL608_STATUS)status;  
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgTunerMode
--| 
--| AUTHOR        : Mahendra Kondur
--|               : Joy Zhang
--|
--| DATE CREATED  : 12/10/2011  
--|               : 08/01/2013  
--|
--| DESCRIPTION   : This fucntion is used to configure MxL608 tuner's 
--|                 application modes like DVB-T, DVB-C, ISDB-T etc.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgTunerMode(UINT8_608 devId, 
                                       MXL608_TUNER_MODE_CFG_T tunerModeCfg)
{
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 dfeRegData = 0;
  MXL608_REG_CTRL_INFO_T* tmpRegTable;

  MxL608_DLL_DEBUG0("%s: Signal Mode = %d, IF Freq = %d, xtal = %d, IF Gain = %d", 
                                                __FUNCTION__,
                                                tunerModeCfg.signalMode,
                                                tunerModeCfg.ifOutFreqinKHz,
                                                tunerModeCfg.xtalFreqSel,
                                                tunerModeCfg.ifOutGainLevel); 

  switch(tunerModeCfg.signalMode)
  {
    case MXL608_DIG_DVB_C:
    case MXL608_DIG_J83B:
      tmpRegTable = MxL608_DigitalDvbc;
      status = MxL608_Ctrl_ProgramRegisters(devId, tmpRegTable);

      if (tunerModeCfg.ifOutFreqinKHz < HIGH_IF_35250_KHZ)
      {
        // Low power
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_0, 0xFE);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_1, 0x10);
      }
      else
      {
        // High power
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_0, 0xD9);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_1, 0x16);
      }

      if (tunerModeCfg.xtalFreqSel == MXL608_XTAL_16MHz) dfeRegData = 0x0D;
      else if (tunerModeCfg.xtalFreqSel == MXL608_XTAL_24MHz) dfeRegData = 0x0E;
      else status |= MXL608_INVALID_PARAMETER;
        
      status |= MxLWare608_OEM_WriteRegister(devId, DFE_CSF_SS_SEL, dfeRegData);

      break;

    case MXL608_DIG_ISDBT_ATSC:
      tmpRegTable = MxL608_DigitalIsdbtAtsc;
      status = MxL608_Ctrl_ProgramRegisters(devId, tmpRegTable);

      if (tunerModeCfg.ifOutFreqinKHz < HIGH_IF_35250_KHZ)
      {
        // Low power
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_0, 0xF9);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_1, 0x18);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_PWR, 0xF1);
      }
      else
      {
        // High power
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_0, 0xD9);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_1, 0x16);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_PWR, 0xB1);
      }

      if (MXL608_XTAL_16MHz == tunerModeCfg.xtalFreqSel) dfeRegData = 0x0D;
      else if (MXL608_XTAL_24MHz == tunerModeCfg.xtalFreqSel) dfeRegData = 0x0E;
      else status |= MXL608_INVALID_PARAMETER;
      
      status |= MxLWare608_OEM_WriteRegister(devId, DFE_CSF_SS_SEL, dfeRegData);

      dfeRegData = 0x1C;
      switch(tunerModeCfg.ifOutGainLevel)
      {
        case 0x09: dfeRegData = 0x44; break;
        case 0x08: dfeRegData = 0x43; break;
        case 0x07: dfeRegData = 0x42; break;
        case 0x06: dfeRegData = 0x41; break;
        case 0x05: dfeRegData = 0x40; break;
        default: break;
      }
      status |= MxLWare608_OEM_WriteRegister(devId, DFE_DACIF_GAIN, dfeRegData);
     
      break;
    case MXL608_DIG_DVB_T_DTMB:
      tmpRegTable = MxL608_DigitalDvbt;
      status = MxL608_Ctrl_ProgramRegisters(devId, tmpRegTable);

      if (tunerModeCfg.ifOutFreqinKHz < HIGH_IF_35250_KHZ)
      {
        // Low power
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_0, 0xFE);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_1, 0x18);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_PWR, 0xF1);
      }
      else
      {
        // High power
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_0, 0xD9);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_CFG_1, 0x16);
        status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_IF_PWR, 0xB1);
      }
      
      if (MXL608_XTAL_16MHz == tunerModeCfg.xtalFreqSel) dfeRegData = 0x0D;
      else if (MXL608_XTAL_24MHz == tunerModeCfg.xtalFreqSel) dfeRegData = 0x0E;
      else status |= MXL608_INVALID_PARAMETER;
      
      status |= MxLWare608_OEM_WriteRegister(devId, DFE_CSF_SS_SEL, dfeRegData);

      dfeRegData = 0;
      switch(tunerModeCfg.ifOutGainLevel)
      {
        case 0x09: dfeRegData = 0x44; break;
        case 0x08: dfeRegData = 0x43; break;
        case 0x07: dfeRegData = 0x42; break;
        case 0x06: dfeRegData = 0x41; break;
        case 0x05: dfeRegData = 0x40; break;
        default: break;
      }
      status |= MxLWare608_OEM_WriteRegister(devId, DFE_DACIF_GAIN, dfeRegData);
      break;

    default:
      status = MXL608_INVALID_PARAMETER;
      break;
  }

  if (status == MXL608_SUCCESS)  
  {
    // XTAL calibration
    status |= MxLWare608_OEM_WriteRegister(devId, XTAL_CALI_SET_REG, 0x00);   
    status |= MxLWare608_OEM_WriteRegister(devId, XTAL_CALI_SET_REG, 0x01);   

    // 50 ms sleep after XTAL calibration
    MxLWare608_OEM_Sleep(50);
  }

  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgTunerAGC
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function is used to configure AGC settings of MxL608
--|                 tuner device.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgTunerAGC(UINT8_608 devId, MXL608_AGC_CFG_T agcCfg)
{
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 regData = 0; 

  MxL608_DLL_DEBUG0("%s, AGC sel = %d, attack point set = %d, Flip = %d \n", 
                                                __FUNCTION__, 
                                                agcCfg.agcType,
                                                agcCfg.setPoint, 
                                                agcCfg.agcPolarityInverstion);

  if ((agcCfg.agcPolarityInverstion <= MXL608_ENABLE) && 
      (agcCfg.agcType <= MXL608_AGC_EXTERNAL))
  {
    // AGC selecton <3:2> and mode setting <0>
    status |= MxLWare608_OEM_ReadRegister(devId, AGC_CONFIG_REG, &regData); 
    regData &= 0xF2; // Clear bits <3:2> & <0>
    regData = (UINT8_608) (regData | (agcCfg.agcType << 2) | 0x01);
    status |= MxLWare608_OEM_WriteRegister(devId, AGC_CONFIG_REG, regData);

    // AGC set point <6:0>
    status |= MxLWare608_OEM_ReadRegister(devId, AGC_SET_POINT_REG, &regData);
    regData &= 0x80; // Clear bit <6:0>
    regData |= agcCfg.setPoint;
    status |= MxLWare608_OEM_WriteRegister(devId, AGC_SET_POINT_REG, regData);

    // AGC Polarity <4>
    status |= MxLWare608_OEM_ReadRegister(devId, AGC_FLIP_REG, &regData);
    regData &= 0xEF; // Clear bit <4>
    regData |= (agcCfg.agcPolarityInverstion << 4);
    status |= MxLWare608_OEM_WriteRegister(devId, AGC_FLIP_REG, regData);
  }
  else
    status = MXL608_INVALID_PARAMETER;

  return(MXL608_STATUS) status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgTunerLoopThrough
--| 
--| AUTHOR        : Mahendra Kondur, Dong Liu
--|
--| DATE CREATED  : 12/10/2011, 06/18/2012   
--|
--| DESCRIPTION   : This function is used to enable or disable Loop-Through
--|                 settings of MxL608 tuner device.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgTunerLoopThrough(UINT8_608 devId, MXL608_BOOL loopThroughCtrl, UINT8_608* gainArray, UINT8_608* attArray)
{
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 regData;
  
  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  if (loopThroughCtrl <= MXL608_ENABLE)
  {
    status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x01);

    status |= MxLWare608_OEM_ReadRegister(devId, DIG_ANA_GINJO_LT_REG, &regData);

    if (loopThroughCtrl == MXL608_ENABLE)
      regData |= 0x10;  // Bit<4> = 1       
    else
      regData &= 0xEF;  // Bit<4> = 0  
    status |= MxLWare608_OEM_WriteRegister(devId, DIG_ANA_GINJO_LT_REG, regData);

    if ((loopThroughCtrl == MXL608_ENABLE) && (gainArray != NULL) && (attArray != NULL))
    {
      regData = (gainArray[3]<<6 ) | (gainArray[2] <<4 ) | (gainArray[1] <<2 ) | gainArray[0] ;
      status |= MxLWare608_OEM_WriteRegister(devId, DFE_SEQ_DIGANA_LT_GAIN_REG, regData);
      regData = (attArray[3]<<6 ) | (attArray[2] <<4 ) | (attArray[1] <<2 ) | attArray[0] ;
      status |= MxLWare608_OEM_WriteRegister(devId, DFE_SEQ_DIGANA_LT_ATTN_REG, regData);
    }
    status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x00);
  }
  else
    status = MXL608_INVALID_PARAMETER;

  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgTunerSouthAfricaLT
--| 
--| AUTHOR        : Dong Liu
--|
--| DATE CREATED  : 03/21/2013   
--|
--| DESCRIPTION   : This function is used to set Loop-Through settings of 
--|                 MxL608 tuner device for South Africa.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgTunerSouthAfricaLT(UINT8_608 devId)
{
  UINT8_608 status = MXL608_SUCCESS;
  
  MxL608_DLL_DEBUG0("%s", __FUNCTION__); 

  status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x01);
  status |= MxLWare608_OEM_WriteRegister(devId, DFE_SEQ_DIGANA_LT_GAIN_REG, 0x95);
  status |= MxLWare608_OEM_WriteRegister(devId, DFE_SEQ_DIGANA_LT_ATTN_REG, 0x00);
  status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x00);

  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgTunerChanTune
--| 
--| AUTHOR        : Mahendra Kondur
--|               : Joy Zhang
--|
--| DATE CREATED  : 12/10/2011  
--|               : 08/01/2013  
--|
--| DESCRIPTION   : This API configures RF channel frequency and bandwidth. 
--|                 Radio Frequency unit is Hz, and Bandwidth is in MHz units.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgTunerChanTune(UINT8_608 devId, 
                                           MXL608_CHAN_TUNE_CFG_T chanTuneCfg)
{
  UINT32_608 frequency;
  UINT32_608 freq = 0;
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 regData = 0;
  UINT8_608 agcData = 0;
  UINT8_608 dfeTuneData = 0;
  UINT8_608 dfeCdcData = 0;
  UINT8_608 dfeAgcRssispData = 0;
  MXL608_CHAN_DEPENDENT_FREQ_TABLE_T *freqLutPtr = NULL;

  MxL608_DLL_DEBUG0("%s, signal type = %d, Freq = %d, BW = %d, Xtal = %d \n",  
                                              __FUNCTION__,
                                              chanTuneCfg.signalMode, 
                                              chanTuneCfg.freqInHz, 
                                              chanTuneCfg.bandWidth, 
                                              chanTuneCfg.xtalFreqSel);

  // Abort Tune
  status |= MxLWare608_OEM_WriteRegister(devId, START_TUNE_REG, 0x00); 

  if (chanTuneCfg.startTune == MXL608_ENABLE)
  {
    if (chanTuneCfg.signalMode <= MXL608_DIG_J83B) 
    {
      // RF Frequency VCO Band Settings 
      if (chanTuneCfg.freqInHz < APP_MODE_FREQ_HZ_THRESHOLD_3) 
      {
        status |= MxLWare608_OEM_WriteRegister(devId, 0x7C, 0x1F);
        if ((chanTuneCfg.signalMode == MXL608_DIG_DVB_C) || (chanTuneCfg.signalMode == MXL608_DIG_J83B)) 
          regData = 0xC1;
        else
        regData = 0x81;        
        
      }
      else 
      {
        status |= MxLWare608_OEM_WriteRegister(devId, 0x7C, 0x9F);
        if ((chanTuneCfg.signalMode == MXL608_DIG_DVB_C) || (chanTuneCfg.signalMode == MXL608_DIG_J83B)) 
          regData = 0xD1;
        else
        regData = 0x91;       
        
      }

      status |= MxLWare608_OEM_WriteRegister(devId, 0x00, 0x01);
      status |= MxLWare608_OEM_WriteRegister(devId, 0x31, regData);
      status |= MxLWare608_OEM_WriteRegister(devId, 0x00, 0x00);

#ifndef ENABLE_BALUN_SETTING
      // RF Frequency balunless Settings 
      // dfe_agc_rfrssi_range has been set to 0 in the DVB-C application mode setting.
      if ((chanTuneCfg.freqInHz >= APP_MODE_FREQ_HZ_THRESHOLD_1) 
          && ((chanTuneCfg.signalMode == MXL608_DIG_DVB_C) || (chanTuneCfg.signalMode == MXL608_DIG_J83B))
          )
      {
          dfeAgcRssispData = ((0x3 <<4) | 0x7);   // dfe_agc_rssisphi1_w = 7  dfe_agc_rssisplo1_w = 3 
      }
      else 
      {
          dfeAgcRssispData = ((0x5 <<4) | 0x9);  // dfe_agc_rssisphi1_w = 9 (default) dfe_agc_rssisplo1_w = 5 (default)
      }
      status |= MxLWare608_OEM_WriteRegister(devId, FINE_TUNE_INIT1_REG, dfeAgcRssispData); 
#endif
      
      // Process spur table programming 
      switch (chanTuneCfg.signalMode) 
      {
        case MXL608_DIG_DVB_C:
        case MXL608_DIG_J83B:
          freqLutPtr = MXL608_DIG_CABLE_FREQ_LUT;
          break; 
        case MXL608_DIG_ISDBT_ATSC:
        case MXL608_DIG_DVB_T_DTMB:
          freqLutPtr = MXL608_DIG_TERR_FREQ_LUT;
          break; 
        default: break; 
      }

      if (freqLutPtr)
        status |= Ctrl_SetRfFreqLutTblReg(devId, chanTuneCfg.freqInHz, freqLutPtr);
      // Bandwidth <7:0>
      switch(chanTuneCfg.bandWidth)
      {
        case MXL608_CABLE_BW_6MHz:
        case MXL608_CABLE_BW_7MHz:
        case MXL608_CABLE_BW_8MHz:
        case MXL608_TERR_BW_6MHz:
        case MXL608_TERR_BW_7MHz:
        case MXL608_TERR_BW_8MHz:
            status |= MxLWare608_OEM_WriteRegister(devId, CHAN_TUNE_BW_REG, (UINT8_608)chanTuneCfg.bandWidth);
            
            // Frequency
            frequency = chanTuneCfg.freqInHz / 1000;

            /* Calculate RF Channel = DIV(64*RF(Hz), 1E6) */
            frequency *= 64;
            freq = (UINT32_608)((frequency + 500) / 1000); // Round operation

            // Set RF  
            status |= MxLWare608_OEM_WriteRegister(devId, CHAN_TUNE_LOW_REG, (UINT8_608)(freq & 0xFF));
            status |= MxLWare608_OEM_WriteRegister(devId, CHAN_TUNE_HI_REG, (UINT8_608)((freq >> 8 ) & 0xFF));
            break;

        default:
          status |= MXL608_INVALID_PARAMETER;
          break;
      }
    
      // Power up tuner module
      status |= MxLWare608_OEM_WriteRegister(devId, TUNER_ENABLE_REG, 0x01);
      

      // Start Sequencer settings
      status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x01); 
      status |= MxLWare608_OEM_ReadRegister(devId, DIG_ANA_GINJO_LT_REG, &regData);
      status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x00); 
      
      status |= MxLWare608_OEM_ReadRegister(devId, 0xB6, &agcData);
      status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x01); 
      status |= MxLWare608_OEM_ReadRegister(devId, 0x60, &dfeTuneData);
      status |= MxLWare608_OEM_ReadRegister(devId, 0x5F, &dfeCdcData);

      // Check if LT is enabled
      if ((regData & 0x10) == 0x10)
      {
        // dfe_agc_auto = 0 & dfe_agc_rf_bo_w = 14
        agcData &= 0xBF;
        agcData |= 0x0E;
        
        // dfe_seq_tune_rf1_bo = 14
        dfeTuneData &= 0xC0;
        dfeTuneData |= 0x0E;
        
        // dfe_seq_cdc_rf1_bo = 14
        dfeCdcData &= 0xC0;
        dfeCdcData |= 0x0E;
      }
      else
      {
        // dfe_agc_auto = 1 & dfe_agc_rf_bo_w = 0
        agcData |= 0x40;
        agcData &= 0xC0;
        
        // dfe_seq_tune_rf1_bo = 55
        dfeTuneData &= 0xC0;
        dfeTuneData |= 0x37;

        // dfe_seq_cdc_rf1_bo = 55
        dfeCdcData &= 0xC0;
        dfeCdcData |= 0x37;
      }
      
      status |= MxLWare608_OEM_WriteRegister(devId, 0x60, dfeTuneData); 
      status |= MxLWare608_OEM_WriteRegister(devId, 0x5F, dfeCdcData); 
      status |= MxLWare608_OEM_WriteRegister(devId, PAGE_CHANGE_REG, 0x00); 
      status |= MxLWare608_OEM_WriteRegister(devId, 0xB6, agcData); 
      
      // Bit <0> 1 : start , 0 : abort calibrations
      status |= MxLWare608_OEM_WriteRegister(devId, START_TUNE_REG, 0x01); 

      // Sleep 15 ms
      MxLWare608_OEM_Sleep(15);

      // dfe_agc_auto = 1 
      agcData = (agcData | 0x40);
      status |= MxLWare608_OEM_WriteRegister(devId, 0xB6, agcData); 

    }
    else
      status = MXL608_INVALID_PARAMETER;
  }

  return (MXL608_STATUS)status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_CfgTunerIFOutParam
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function is used to configure IF out settings of MxL608 
--|                 tuner device.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_CfgTunerIFOutParam(UINT8_608 devId, MXL608_IF_OUT_CFG_T ifOutCfg)
{
  UINT16_608 ifFcw;
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 readData = 0;
  UINT8_608 control = 0;
 
  MxL608_DLL_DEBUG0("%s, Manual set = %d \n", __FUNCTION__, ifOutCfg.manualFreqSet); 

  // Read back register for manual IF Out 
  status = MxLWare608_OEM_ReadRegister(devId, IF_FREQ_SEL_REG, &readData);

  if (ifOutCfg.manualFreqSet == MXL608_ENABLE)
  {
    MxL608_DLL_DEBUG0("%s, IF Freq = %d \n", __FUNCTION__, ifOutCfg.manualIFOutFreqInKHz); 

    // IF out manual setting : bit<5>
    readData |= 0x20;
    status = MxLWare608_OEM_WriteRegister(devId, IF_FREQ_SEL_REG, readData);

    // Manual IF freq set
    ifFcw = (UINT16_608)(ifOutCfg.manualIFOutFreqInKHz * 8192 / 216000);
    control = (ifFcw & 0xFF); // Get low 8 bit 
    status |= MxLWare608_OEM_WriteRegister(devId, IF_FCW_LOW_REG, control); 

    control = ((ifFcw >> 8) & 0x0F); // Get high 4 bit 
    status |= MxLWare608_OEM_WriteRegister(devId, IF_FCW_HIGH_REG, control);
  }
  else if (ifOutCfg.manualFreqSet == MXL608_DISABLE)
  {
    // bit<5> = 0, use IF frequency from IF frequency table  
    readData &= 0xC0;

    // IF Freq <4:0>
    readData |= ifOutCfg.ifOutFreq;
    status |= MxLWare608_OEM_WriteRegister(devId, IF_FREQ_SEL_REG, readData);
  }
  else
    status |= MXL608_INVALID_PARAMETER;

  if (status == MXL608_SUCCESS)
  {
    // Set spectrum invert, gain level and IF path 
    // Spectrum invert indication is bit<7:6>
    if (ifOutCfg.ifInversion <= MXL608_ENABLE)
    {
      control = 0;
      if (MXL608_ENABLE == ifOutCfg.ifInversion) control = 0x3 << 6;
    
      // Gain level is bit<3:0> 
      control += (ifOutCfg.gainLevel & 0x0F);
      control |= (0x20); // Enable IF out
      status |= MxLWare608_OEM_WriteRegister(devId, IF_PATH_GAIN_REG, control);
    } 
    else
      status |= MXL608_INVALID_PARAMETER;
  }

  return(MXL608_STATUS) status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_ReqTunerAGCLock
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function returns AGC Lock status of MxL608 tuner.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_ReqTunerAGCLock(UINT8_608 devId, MXL608_BOOL* agcLockStatusPtr)
{
  MXL608_STATUS status = MXL608_SUCCESS;
  UINT8_608 regData = 0;
  MXL608_BOOL lockStatus = MXL608_UNLOCKED;

  MxL608_DLL_DEBUG0("%s", __FUNCTION__);

  if (agcLockStatusPtr)
  {
    status = MxLWare608_OEM_ReadRegister(devId, AGC_SAGCLOCK_STATUS_REG, &regData);  
    if ((regData & 0x08) == 0x08) lockStatus = MXL608_LOCKED;

    *agcLockStatusPtr =  lockStatus;
    
    MxL608_DLL_DEBUG0(" Agc lock = %d", (UINT8_608)*agcLockStatusPtr); 
  }
  else
    status = MXL608_INVALID_PARAMETER;

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_ReqTunerLockStatus
--| 
--| AUTHOR        : Mahendra Kondur
--|
--| DATE CREATED  : 12/10/2011  
--|
--| DESCRIPTION   : This function returns Tuner Lock status of MxL608 tuner.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_ReqTunerLockStatus(UINT8_608 devId, MXL608_BOOL* rfLockPtr, 
                                                          MXL608_BOOL* refLockPtr)
{
  MXL608_STATUS status = MXL608_SUCCESS;
  UINT8_608 regData = 0;
  MXL608_BOOL rfLockStatus = MXL608_UNLOCKED;
  MXL608_BOOL refLockStatus = MXL608_UNLOCKED;

  MxL608_DLL_DEBUG0("%s", __FUNCTION__);

  if ((rfLockPtr) && (refLockPtr))
  {
    status = MxLWare608_OEM_ReadRegister(devId, RF_REF_STATUS_REG, &regData);  

    if ((regData & 0x02) == 0x02) rfLockStatus = MXL608_LOCKED;
    if ((regData & 0x01) == 0x01) refLockStatus = MXL608_LOCKED;
    
    MxL608_DLL_DEBUG0(" RfSynthStatus = %d, RefSynthStatus = %d", (UINT8_608)rfLockStatus,
                                                               (UINT8_608)refLockStatus); 

    *rfLockPtr =  rfLockStatus;
    *refLockPtr = refLockStatus;
  }
  else
    status = MXL608_INVALID_PARAMETER;
  
  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_API_ReqTunerRxPower
--| 
--| AUTHOR        : Mahendra Kondur
--|                 Dong Liu 
--|
--| DATE CREATED  : 12/10/2011
--|                 06/18/2012
--|
--| DESCRIPTION   : This function returns RF input power in 0.01dBm.
--|
--| RETURN VALUE  : MXL608_SUCCESS, MXL608_INVALID_PARAMETER, MXL608_FAILED
--|
--|---------------------------------------------------------------------------*/

MXL608_STATUS MxLWare608_API_ReqTunerRxPower(UINT8_608 devId, SINT16* rxPwrPtr)
{
  UINT8_608 status = MXL608_SUCCESS;
  UINT8_608 regData = 0;
  UINT16_608 tmpData = 0;

  MxL608_DLL_DEBUG0("%s", __FUNCTION__);

  if (rxPwrPtr)
  {
    // RF input power low <7:0>
    status = MxLWare608_OEM_ReadRegister(devId, RFPIN_RB_LOW_REG, &regData);
    tmpData = regData;

    // RF input power high <1:0>
    status |= MxLWare608_OEM_ReadRegister(devId, RFPIN_RB_HIGH_REG, &regData);
    tmpData |= (regData & 0x03) << 8;

    // Fractional last 2 bits
    *rxPwrPtr = (tmpData & 0x01FF) * 25;  //100 times dBm

    if (tmpData & 0x02) *rxPwrPtr += 50;;
    if (tmpData & 0x01) *rxPwrPtr += 25;
    if (tmpData & 0x0200) *rxPwrPtr -= 128*100;
      
    MxL608_DLL_DEBUG0(" Rx power = %d times of 0.01dBm \n", *rxPwrPtr);
  }
  else
    status = MXL608_INVALID_PARAMETER;

  return (MXL608_STATUS)status;
}


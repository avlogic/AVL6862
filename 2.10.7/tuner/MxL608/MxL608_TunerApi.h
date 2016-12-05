/*******************************************************************************
 *
 * FILE NAME          : MxL608_TunerApi.h
 * 
 * AUTHOR             : Dong Liu
 *
 * DATE CREATED       : 12/21/2012
 *
 * DESCRIPTION        : This file is header file for MxL608_TunerApi.cpp 
 *
 *******************************************************************************
 *                Copyright (c) 2011, MaxLinear, Inc.
 ******************************************************************************/

#ifndef __MXL608_TUNER_API_H__
#define __MXL608_TUNER_API_H__

/******************************************************************************
    Include Header Files
    (No absolute paths - paths handled by make file)
******************************************************************************/

#include "MxL608_MaxLinearDataTypes.h"

/******************************************************************************
    Macros
******************************************************************************/

#define MXL608_VERSION_SIZE     5

#define MXL608_MAX_NUM_DEVICES  4

/******************************************************************************
    User-Defined Types (Typedefs)
******************************************************************************/
 
typedef struct
{
  UINT8_608 chipId;                           /* OUT, Device chip ID information  */
  UINT8_608 chipVersion;                      /* OUT, Device chip revision  */
  UINT8_608 mxlwareVer[MXL608_VERSION_SIZE];  /* OUT, MxLWare version information */    
} MXL608_VER_INFO_T, *PMXL608_VER_INFO_T;

typedef enum
{
  MXL608_XTAL_16MHz = 0,
  MXL608_XTAL_24MHz
} MXL608_XTAL_FREQ_E;

typedef struct
{
  MXL608_XTAL_FREQ_E xtalFreqSel;  /* IN, XTAL Frequency, refers above */
  UINT8_608 xtalCap;                   /* IN, XTAL capacity, 1 LSB = 1pF, maximum is 31pF */
  MXL608_BOOL clkOutEnable;           /* IN, enable or disable clock out */
  MXL608_BOOL clkOutDiv;              /* IN, indicate if XTAL frequency is dived by 4 or not */
  MXL608_BOOL clkOutExt;              /* IN, enable or disable external clock out*/
  MXL608_BOOL singleSupply_3_3V;      /* IN, Single Supply 3.3v */
  MXL608_BOOL XtalSharingMode;        /* IN, XTAL sharing mode. default Master, MXL608_ENABLE to config Slave mode */
} MXL608_XTAL_SET_CFG_T, *PMXL608_XTAL_SET_CFG_T;

typedef enum
{
  MXL608_PWR_MODE_SLEEP = 0,      /* power saving mode - Sleep */
  MXL608_PWR_MODE_ACTIVE,         /* power saving mode - Active */
  MXL608_PWR_MODE_STANDBY         /* power saving mode - Standby */
} MXL608_PWR_MODE_E;    

typedef enum
{
  MXL608_GPO_LOW = 0,
  MXL608_GPO_HIGH = 1,
  MXL608_GPO_AUTO_CTRL = 2
} MXL608_GPO_STATE_E;         /* GPO control*/

typedef enum
{
  MXL608_PLL_STATE_NORMAL = 0,
  MXL608_PLL_STATE_WRONG  = 1,
  MXL608_PLL_STATE_NA = 2
} MXL608_PLL_STATE_E;

typedef enum
{
  MXL608_DIG_DVB_C = 0,     /* DVB-C mode */
  MXL608_DIG_ISDBT_ATSC,    /* ATSC/ISDB-T mode */
  MXL608_DIG_DVB_T_DTMB = 2,    /* DVB-T/DVB-T2 and DTMB mode */
  MXL608_DIG_J83B           /* J.83B mode */
} MXL608_SIGNAL_MODE_E;     /* MxL608 Application mode */

typedef enum
{
  MXL608_IF_3_65MHz  = 0,
  MXL608_IF_4MHz     = 1,
  MXL608_IF_4_1MHz   = 2,
  MXL608_IF_4_15MHz  = 3,
  MXL608_IF_4_5MHz   = 4, 
  MXL608_IF_4_57MHz  = 5,
  MXL608_IF_5MHz     = 6,
  MXL608_IF_5_38MHz  = 7, 
  MXL608_IF_6MHz     = 8,
  MXL608_IF_6_28MHz  = 9,
  MXL608_IF_7_2MHz   = 10, 
  MXL608_IF_8_25MHz  = 11,
  MXL608_IF_35_25MHz = 12,
  MXL608_IF_36MHz    = 13, 
  MXL608_IF_36_15MHz = 14, 
  MXL608_IF_36_65MHz = 15,
  MXL608_IF_44MHz    = 16
} MXL608_IF_FREQ_E;       /* Pre-Define IF out values */

typedef struct
{
  MXL608_SIGNAL_MODE_E signalMode;       /* IN , Tuner application mode */
  MXL608_XTAL_FREQ_E xtalFreqSel;        /* XTAL Frequency, refers above */
  UINT32_608 ifOutFreqinKHz;                 /* IN, IF Frequency in KHz */
  UINT8_608 ifOutGainLevel;                  /* IF out gain level */ 
} MXL608_TUNER_MODE_CFG_T, *PMXL608_TUNER_MODE_CFG_T;

typedef enum
{
  MXL608_AGC_SELF = 0,   
  MXL608_AGC_EXTERNAL = 1 
} MXL608_AGC_TYPE_E;

typedef struct
{
  UINT8_608 setPoint;                   /* IN, AGC attack point set value */
  MXL608_BOOL agcPolarityInverstion;   /* IN, Config AGC Polarity inversion */  
  MXL608_AGC_TYPE_E agcType;        /* IN, AGC mode selection, self or closed loop */
} MXL608_AGC_CFG_T, *PMXL608_AGC_CFG_T;

typedef enum
{
  MXL608_CABLE_BW_6MHz = 0x00,    /* Digital Cable Mode 6 MHz */      
  MXL608_CABLE_BW_7MHz = 0x01,    /* Digital Cable Mode 7 MHz */            
  MXL608_CABLE_BW_8MHz = 0x02,    /* Digital Cable Mode 8 MHz */              
  MXL608_TERR_BW_6MHz = 0x20,     /* Digital Terrestrial Mode 6 MHz */     
  MXL608_TERR_BW_7MHz = 0x21,     /* Digital Terrestrial Mode 7 MHz */    
  MXL608_TERR_BW_8MHz = 0x22      /* Digital Terrestrial Mode 8 MHz */    
} MXL608_BW_E;

typedef struct
{
  UINT32_608 freqInHz;                     /* IN, Radio Frequency in Hz */ 
  MXL608_BOOL startTune;                  /* IN, MXL608_ENABLE to start tune, MXL608_DISABLE to Abort tune */
  MXL608_BW_E bandWidth;               /* IN, band width in MHz. Refer "MXL608_BW_E" */
  MXL608_SIGNAL_MODE_E signalMode;     /* IN, Tuner application mode */
  MXL608_XTAL_FREQ_E xtalFreqSel;      /* IN, Xtal frequency */
} MXL608_CHAN_TUNE_CFG_T, *PMXL608_CHAN_TUNE_CFG_T;

typedef struct
{
  MXL608_IF_FREQ_E ifOutFreq;       /* IN, band width of IF out signal */
  MXL608_BOOL manualFreqSet;           /* IN, IF out frequency is set by manual or not */ 
  MXL608_BOOL ifInversion;             /* IN, IF spectrum is inverted or not */
  UINT8_608 gainLevel;                  /* IN, IF out gain level */
  UINT32_608 manualIFOutFreqInKHz;      /* IN, IF out frequency selection when manualFreqSet = 0*/
}MXL608_IF_OUT_CFG_T, *PMXL608_IF_OUT_CFG_T;

/******************************************************************************
    Global Variable Declarations
******************************************************************************/
extern const UINT8_608 MxLWare608DrvVersion[];

/******************************************************************************
    Prototypes
******************************************************************************/
MXL608_STATUS MxLWare608_API_CfgDrvInit(UINT8_608 devId, void* oemDataPtr);

MXL608_STATUS MxLWare608_API_CfgDevSoftReset(UINT8_608 devId);
MXL608_STATUS MxLWare608_API_CfgDevOverwriteDefaults(UINT8_608 devId, MXL608_BOOL singleSupply_3_3V);
MXL608_STATUS MxLWare608_API_CfgDevXtal(UINT8_608 devId, MXL608_XTAL_SET_CFG_T xtalCfg);
MXL608_STATUS MxLWare608_API_CfgDevPowerMode(UINT8_608 devId,
                                          MXL608_PWR_MODE_E powerMode, 
                                          MXL608_BOOL enableLoopthrough, 
                                          UINT8_608 standbyLt);
MXL608_STATUS MxLWare608_API_CfgDevGPO(UINT8_608 devId, MXL608_GPO_STATE_E gpoState);

MXL608_STATUS MxLWare608_API_ReqDevVersionInfo(UINT8_608 devId, MXL608_VER_INFO_T* mxlDevVerInfoPtr);
MXL608_STATUS MxLWare608_API_ReqDevGPOStatus(UINT8_608 devId, MXL608_GPO_STATE_E* gpoStatusPtr);
MXL608_STATUS MxLWare608_API_ReqDevPllState(UINT8_608 devId, MXL608_PLL_STATE_E* PllStatePtr);

MXL608_STATUS MxLWare608_API_CfgTunerMode(UINT8_608 devId, MXL608_TUNER_MODE_CFG_T tunerModeCfg);
MXL608_STATUS MxLWare608_API_CfgTunerAGC(UINT8_608 devId, MXL608_AGC_CFG_T agcCfg);
MXL608_STATUS MxLWare608_API_CfgTunerLoopThrough(UINT8_608 devId, MXL608_BOOL loopThroughCtrl, UINT8_608* gainArray, UINT8_608* attArray);
MXL608_STATUS MxLWare608_API_CfgTunerIFOutParam(UINT8_608 devId, MXL608_IF_OUT_CFG_T ifOutCfg);
MXL608_STATUS MxLWare608_API_CfgTunerSouthAfricaLT(UINT8_608 devId);
MXL608_STATUS MxLWare608_API_CfgTunerChanTune(UINT8_608 devId, MXL608_CHAN_TUNE_CFG_T chanTuneCfg);

MXL608_STATUS MxLWare608_API_ReqTunerAGCLock(UINT8_608 devId, MXL608_BOOL* agcLockStatusPtr);
MXL608_STATUS MxLWare608_API_ReqTunerLockStatus(UINT8_608 devId, MXL608_BOOL* rfLockPtr, MXL608_BOOL* refLockPtr);
MXL608_STATUS MxLWare608_API_ReqTunerRxPower(UINT8_608 devId, SINT16* rxPwrPtr);

#endif /* __MXL608_TUNER_API_H__*/





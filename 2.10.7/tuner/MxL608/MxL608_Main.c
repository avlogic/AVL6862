#include <stdio.h>
#include "MxL608_TunerApi.h"
#include "MxL608_OEM_Drv.h"

#define EXAMPLE_DEV_MAX 2
#define MXL608_I2C_ADDR 0x60

/* Example of OEM Data, customers should have
below data structure declared at their appropriate 
places as per their software design 

typedef struct
{
  UINT8_608   i2c_address;
  UINT8_608   i2c_bus;
  sem_type_t sem;
  UINT16_608  i2c_cnt;
} user_data_t;  

user_data_t device_context[EXAMPLE_DEV_MAX];

*/

int t_main(void)
{
  MXL608_STATUS status; 
  UINT8_608 devId;
  MXL608_BOOL singleSupply_3_3V;
  MXL608_XTAL_SET_CFG_T xtalCfg;
  MXL608_IF_OUT_CFG_T ifOutCfg;
  MXL608_AGC_CFG_T agcCfg;
  MXL608_TUNER_MODE_CFG_T tunerModeCfg;
  MXL608_CHAN_TUNE_CFG_T chanTuneCfg;
  MXL608_BOOL refLockPtr = MXL608_UNLOCKED;
  MXL608_BOOL rfLockPtr = MXL608_UNLOCKED;		

/* If OEM data is implemented, customer needs to use OEM data structure  
   related operation. Following code should be used as a reference. 
   For more information refer to sections 2.5 & 2.6 of 
   MxL608_mxLWare_API_UserGuide document.

  for (devId = 0; devId < EXAMPLE_DEV_MAX; devId++)
  {
    // assigning i2c address for  the device
    device_context[devId].i2c_address = GET_FROM_FILE_I2C_ADDRESS(devId);     

    // assigning i2c bus for  the device
    device_context[devId].i2c_bus = GET_FROM_FILE_I2C_BUS(devId);                      

    // create semaphore if necessary
    device_context[devId].sem = CREATE_SEM();                                                           

    // sample stat counter
    device_context[devId].i2c_cnt = 0;                                                                               

    status = MxLWare608_API_CfgDrvInit(devId, (void *) &device_context[devId]);  

    // if you donn't want to pass any oem data, just use NULL as a parameter:
    // status = MxLWare608_API_CfgDrvInit(devId, NULL);  
  }

*/

  /* If OEM data is not required, customer should treat devId as 
     I2C slave Address */
  devId = MXL608_I2C_ADDR;
    
  //Step 1 : Soft Reset MxL608
  status = MxLWare608_API_CfgDevSoftReset(devId);
  if (status != MXL608_SUCCESS)
  {
    printf("Error! MxLWare608_API_CfgDevSoftReset\n");    
  }
    
  //Step 2 : Overwrite Default
  singleSupply_3_3V = MXL608_DISABLE;
  status = MxLWare608_API_CfgDevOverwriteDefaults(devId, singleSupply_3_3V);
  if (status != MXL608_SUCCESS)
  {
    printf("Error! MxLWare608_API_CfgDevOverwriteDefaults\n");    
  }

  //Step 3 : XTAL Setting
  xtalCfg.xtalFreqSel = MXL608_XTAL_16MHz;
  xtalCfg.xtalCap = 12;
  xtalCfg.clkOutEnable = MXL608_ENABLE;
  xtalCfg.clkOutDiv = MXL608_DISABLE;
  xtalCfg.clkOutExt = MXL608_DISABLE;
  xtalCfg.singleSupply_3_3V = MXL608_DISABLE;
  xtalCfg.XtalSharingMode = MXL608_DISABLE;
  status = MxLWare608_API_CfgDevXtal(devId, xtalCfg);
  if (status != MXL608_SUCCESS)
  {
    printf("Error! MxLWare608_API_CfgDevXtal\n");    
  }

  //Step 4 : IF Out setting
  ifOutCfg.ifOutFreq = MXL608_IF_4_1MHz;
  ifOutCfg.ifInversion = MXL608_DISABLE;
  ifOutCfg.gainLevel = 11;
  ifOutCfg.manualFreqSet = MXL608_DISABLE;
  ifOutCfg.manualIFOutFreqInKHz = 0;
  status = MxLWare608_API_CfgTunerIFOutParam(devId, ifOutCfg);
  if (status != MXL608_SUCCESS)
  {
    printf("Error! MxLWare608_API_CfgTunerIFOutParam\n");    
  }

  //Step 5 : AGC Setting
  agcCfg.agcType = MXL608_AGC_EXTERNAL;
  agcCfg.setPoint = 66;
  agcCfg.agcPolarityInverstion = MXL608_DISABLE;
  status = MxLWare608_API_CfgTunerAGC(devId, agcCfg);
  if (status != MXL608_SUCCESS)
  {
    printf("Error! MxLWare608_API_CfgTunerAGC\n");    
  }

  //Step 6 : Application Mode setting
  tunerModeCfg.signalMode = MXL608_DIG_DVB_T_DTMB;
  tunerModeCfg.ifOutFreqinKHz = 4100;
  tunerModeCfg.xtalFreqSel = MXL608_XTAL_16MHz;
  tunerModeCfg.ifOutGainLevel = 11;
  status = MxLWare608_API_CfgTunerMode(devId, tunerModeCfg);
  if (status != MXL608_SUCCESS)
  {
    printf("Error! MxLWare608_API_CfgTunerMode\n");    
  }

  //Step 7 : Channel frequency & bandwidth setting
  chanTuneCfg.bandWidth = MXL608_TERR_BW_6MHz;
  chanTuneCfg.freqInHz = 666000000;
  chanTuneCfg.signalMode = MXL608_DIG_DVB_T_DTMB;
  chanTuneCfg.xtalFreqSel = MXL608_XTAL_16MHz;
  chanTuneCfg.startTune = MXL608_START_TUNE;
  status = MxLWare608_API_CfgTunerChanTune(devId, chanTuneCfg);
  if (status != MXL608_SUCCESS)
  {
    printf("Error! MxLWare608_API_CfgTunerChanTune\n");    
  }

  // Wait 15 ms 
  MxLWare608_OEM_Sleep(15);

  // Read back Tuner lock status
  status = MxLWare608_API_ReqTunerLockStatus(devId, &rfLockPtr, &refLockPtr);
  if (status == MXL608_TRUE)
  {
    if (MXL608_LOCKED == rfLockPtr && MXL608_LOCKED == refLockPtr)
    {
      printf("Tuner locked\n");
    }
    else
      printf("Tuner unlocked\n");
  }

  // To Change Channel, GOTO Step #7

  // To change Application mode settings, GOTO Step #6

  return 0;
}
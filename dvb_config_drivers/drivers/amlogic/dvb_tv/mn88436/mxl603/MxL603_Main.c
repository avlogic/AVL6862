#include "MaxLinearDataTypes.h"
#include "MxL603_TunerApi.h"
#include "MxL603_OEM_Drv.h"

#define EXAMPLE_DEV_MAX 2
MXL603_TUNER_MODE_CFG_T tunerModeCfg; //global parameter 
MXL603_CHAN_TUNE_CFG_T chanTuneCfg;//set to be global value
int gMxl603Mode = 1; // 1-ATSC 8vsb; 0-J.83B

/* Example of OEM Data, customers should have
below data structure declared at their appropriate 
places as per their software design 

typedef struct
{
  UINT8   i2c_address;
  UINT8   i2c_bus;
  sem_type_t sem;
  UINT16  i2c_cnt;
} user_data_t;  

user_data_t device_context[EXAMPLE_DEV_MAX];

*/

MXL_STATUS MXL603_INIT(DMD_PARAMETER* param)
{
  MXL_STATUS status; 
  UINT8 devId;
  MXL_BOOL singleSupply_3_3V;
  MXL603_XTAL_SET_CFG_T xtalCfg;
  MXL603_IF_OUT_CFG_T ifOutCfg;
  MXL603_AGC_CFG_T agcCfg;
  //MXL603_TUNER_MODE_CFG_T tunerModeCfg; //set to be global value
  //MXL603_CHAN_TUNE_CFG_T chanTuneCfg;//set to be global value
  MXL_BOOL refLockPtr;
  MXL_BOOL rfLockPtr;
/* If OEM data is implemented, customer needs to use OEM data structure  
   related operation. Following code should be used as a reference. 
   For more information refer to sections 2.5 & 2.6 of 
   MxL603_mxLWare_API_UserGuide document.

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

    status = MxLWare603_API_CfgDrvInit(devId, (void *) &device_context[devId]);  

    // if you don’t want to pass any oem data, just use NULL as a parameter:
    // status = MxLWare603_API_CfgDrvInit(devId, NULL);  
  }

*/

  /* If OEM data is not required, customer should treat devId as 
     I2C slave Address */
  devId = MXL603_I2C_ADDR;
  printk("MXL603_INIT\n");
    
  //Step 1 : Soft Reset MxL603
  status = MxLWare603_API_CfgDevSoftReset(devId);
  if (status != MXL_SUCCESS)
  {
    printk("Error! MxLWare603_API_CfgDevSoftReset\n");    
  }
    
  //Step 2 : Overwrite Default
  singleSupply_3_3V = MXL_ENABLE;//MXL_DISABLE;
  status = MxLWare603_API_CfgDevOverwriteDefaults(devId, singleSupply_3_3V);
  if (status != MXL_SUCCESS)
  {
    printk("Error! MxLWare603_API_CfgDevOverwriteDefaults\n");    
  }

  //Step 3 : XTAL Setting
  xtalCfg.xtalFreqSel = MXL603_XTAL_24MHz;//MXL603_XTAL_16MHz;
  xtalCfg.xtalCap = 31;//20;//12;
  xtalCfg.clkOutEnable = MXL_ENABLE;
  xtalCfg.clkOutDiv = MXL_DISABLE;
  xtalCfg.clkOutExt = MXL_DISABLE;
  xtalCfg.singleSupply_3_3V = MXL_ENABLE;//MXL_DISABLE;
  xtalCfg.XtalSharingMode = MXL_DISABLE;
  status = MxLWare603_API_CfgDevXtal(devId, xtalCfg);
  if (status != MXL_SUCCESS)
  {
    printk("Error! MxLWare603_API_CfgDevXtal\n");    
  }

  //Step 4 : IF Out setting
  ifOutCfg.ifOutFreq = MXL603_IF_5MHz;
  ifOutCfg.ifInversion = MXL_DISABLE;
  ifOutCfg.gainLevel = 11;
  ifOutCfg.manualFreqSet = MXL_DISABLE;
  ifOutCfg.manualIFOutFreqInKHz = 0;
  status = MxLWare603_API_CfgTunerIFOutParam(devId, ifOutCfg);
  if (status != MXL_SUCCESS)
  {
    printk("Error! MxLWare603_API_CfgTunerIFOutParam\n");    
  }

  //Step 5 : AGC Setting
  //agcCfg.agcType = MXL603_AGC_SELF; //if you doubt DMD IF-AGC part, pls. use Tuner self AGC instead.
  agcCfg.agcType = MXL603_AGC_EXTERNAL;
  agcCfg.setPoint = 66;
  agcCfg.agcPolarityInverstion = MXL_DISABLE;
  status = MxLWare603_API_CfgTunerAGC(devId, agcCfg);
  if (status != MXL_SUCCESS)
  {
    printk("Error! MxLWare603_API_CfgTunerAGC\n");    
  }

  //Step 6 : Application Mode setting
  if(gMxl603Mode == 0)
  {
	tunerModeCfg.signalMode = MXL603_DIG_J83B;//MXL603_DIG_ISDBT_ATSC;//MXL603_DIG_J83B;
  }
  else
  {
	tunerModeCfg.signalMode = MXL603_DIG_ISDBT_ATSC;
  }

  tunerModeCfg.ifOutFreqinKHz = 5000;
  tunerModeCfg.xtalFreqSel = MXL603_XTAL_24MHz;
  tunerModeCfg.ifOutGainLevel = 11;
  status = MxLWare603_API_CfgTunerMode(devId, tunerModeCfg);
  if (status != MXL_SUCCESS)
  {
    printk("##### Error! pls. make sure return value no problem, otherwise, it will cause Tuner unable to unlock signal #####\n");   
    printk("Error! MxLWare603_API_CfgTunerMode\n");    
  }

  //Step 7 : Channel frequency & bandwidth setting
   if(gMxl603Mode == 0)
   {
  	chanTuneCfg.bandWidth = MXL603_CABLE_BW_6MHz;
  	chanTuneCfg.signalMode = MXL603_DIG_J83B; //MXL603_DIG_ISDBT_ATSC;
   }
  else
  {
  	chanTuneCfg.bandWidth = MXL603_TERR_BW_6MHz;
 	chanTuneCfg.signalMode = MXL603_DIG_ISDBT_ATSC; //MXL603_DIG_ISDBT_ATSC;
  }
  
  chanTuneCfg.freqInHz =  param->freq;//474000000;//666000000; //474000000;
  chanTuneCfg.xtalFreqSel = MXL603_XTAL_24MHz;
  chanTuneCfg.startTune = MXL_START_TUNE;
  status = MxLWare603_API_CfgTunerChanTune(devId, chanTuneCfg);
  if (status != MXL_SUCCESS)
  {
    printk("Error! MxLWare603_API_CfgTunerChanTune\n");    
  }

  // Wait 15 ms 
  MxLWare603_OEM_Sleep(15);

  // Read back Tuner lock status
  status = MxLWare603_API_ReqTunerLockStatus(devId, &rfLockPtr, &refLockPtr);
  if (status == MXL_TRUE)
  {
    if (MXL_LOCKED == rfLockPtr && MXL_LOCKED == refLockPtr)
    {
      printk("Tuner locked\n");
    }
    else
      printk("Tuner unlocked\n");
  }

  // To Change Channel, GOTO Step #7

  // To change Application mode settings, GOTO Step #6

  return status;
}


MXL_STATUS Mxl603SetSystemMode(void)
{
  MXL_STATUS status; 
  UINT8 devId;
  devId = MXL603_I2C_ADDR;  								 

  //Step 6 : Application Mode setting
  //tunerModeCfg.signalMode = MXL603_DIG_DVB_T_DTMB;//MXL603_DIG_ISDBT_ATSC;
  //tunerModeCfg.ifOutFreqinKHz = 5000;
  //tunerModeCfg.ifOutGainLevel = 11;

  //  tunerModeCfg is global struct. 
  status = MxLWare603_API_CfgTunerMode(devId, tunerModeCfg);
  if (status != MXL_SUCCESS)
  {
    printk("Error! MxLWare603_API_CfgTunerMode\n");    
  }

  return status;

}  

MXL_STATUS Mxl603SetFreqBw(DMD_PARAMETER* param)
{
  MXL_STATUS status; 
  UINT8 devId;
  MXL_BOOL refLockPtr;
  MXL_BOOL rfLockPtr;
 
     devId = MXL603_I2C_ADDR;
 
  //Step 7 : Channel frequency & bandwidth setting
 /*
  chanTuneCfg.bandWidth = MXL603_TERR_BW_8MHz;
  chanTuneCfg.freqInHz = 666000000;
  chanTuneCfg.signalMode = MXL603_DIG_DVB_T_DTMB;
  chanTuneCfg.startTune = MXL_START_TUNE;
 */
  chanTuneCfg.freqInHz =  param->freq;
  //printk("\n-------------------> Mxl603SetFreqBw  @@@@@@@@@@@@@@@@@@@\n");
  //chanTuneCfgis global struct. 
  status = MxLWare603_API_CfgTunerChanTune(devId, chanTuneCfg);
  if (status != MXL_SUCCESS)
  {
    printk("Error! MxLWare603_API_CfgTunerChanTune\n");    
  }
  
  // Wait 15 ms 
  MxLWare603_OEM_Sleep(15);

  // Read back Tuner lock status
  status = MxLWare603_API_ReqTunerLockStatus(devId, &rfLockPtr, &refLockPtr);
  if (status == MXL_TRUE)
  {
    if (MXL_LOCKED == rfLockPtr && MXL_LOCKED == refLockPtr)
    {
      printk("Tuner locked\n"); //If system runs into here, it mean that Tuner locked and output IF OK!!
    }
    else
      printk("Tuner unlocked\n");
  }
  return status; 
}

MXL_STATUS Mxl603_set_system(int is8VSB)
{
	UINT8 devId;
	//MXL_STATUS status; 
  	devId = MXL603_I2C_ADDR; 
	
	if(is8VSB)
	{
		chanTuneCfg.bandWidth = MXL603_TERR_BW_6MHz;
		chanTuneCfg.signalMode = MXL603_DIG_ISDBT_ATSC; //MXL603_DIG_ISDBT_ATSC;		
		gMxl603Mode = 1 ;
	}
	else
	{
		chanTuneCfg.bandWidth = MXL603_CABLE_BW_6MHz;
		chanTuneCfg.signalMode = MXL603_DIG_J83B; //MXL603_DIG_ISDBT_ATSC;
		
		tunerModeCfg.signalMode = MXL603_DIG_J83B;//MXL603_DIG_ISDBT_ATSC;//MXL603_DIG_J83B;
		gMxl603Mode = 0;
	}

	return 0;
}

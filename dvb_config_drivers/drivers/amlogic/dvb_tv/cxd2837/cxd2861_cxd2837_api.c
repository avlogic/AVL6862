

#include <linux/delay.h>
#include <linux/mutex.h>

#include <linux/gpio.h>
#include <linux/amlogic/aml_gpio_consumer.h>
#include <linux/dvb/frontend.h>
#include <linux/i2c.h>

#include "../aml_dvb.h"
#include "../aml_fe.h"


#include "sony_example_terr_cable_configuration.h"
#include "sony_integ.h"
#include "sony_integ_dvbt_t2.h"
#include "sony_integ_dvbc.h"
#include "sony_dvbt.h"
#include "sony_dvbt2.h"
#include "sony_demod_dvbt.h"
#include "sony_demod_dvbt2.h"
#include "sony_demod_dvbc.h"
#include "sony_demod_dvbt_monitor.h"
#include "sony_demod_dvbt2_monitor.h"
#include "sony_demod_dvbc_monitor.h"

#ifdef SONY_EXAMPLE_TUNER_ASCOT2E
#include "sony_ascot2e.h"
#include "sony_tuner_ascot2e.h"
#endif
#ifdef	SONY_EXAMPLE_TUNER_ASCOT3
#include "sony_ascot3.h"
#include "sony_tuner_ascot3.h"
#endif
#include "sony_demod.h"
#include "sony_dtv.h"
#include "cxd2861_cxd2837_api.h"
#include "sony_demod_dvbt2.h"
#include "mxl603/MxL603_TunerApi.h"


#define TUNER_IFAGCPOS            
//**< Define for IFAGC sense positive. */
//* #define TUNER_SPECTRUM_INV       1 */
//**< Define for spectrum inversion. */
#define TUNER_RFLVLMON_DISABLE      1       /**< Define to disable RF level monitoring. */
#define TUNER_SONY_ASCOT            1       /**< Define for Sony CXD2815 / Ascot2S tuner. */


#define DEMOD_TUNE_POLL_INTERVAL    10


#if 0
#define pr_dbg(fmt, args...) printk(KERN_ERR "cxd2837: " fmt, ## args)
#else
#define pr_dbg(fmt, args...)
#endif

#define pr_error(fmt, args...) printk(KERN_ERR "cxd2837: " fmt, ## args)


#define MAX_I2C_WRITE 256

#if 0
static const char *FormatResult (sony_result_t result);
static const char *FormatSystem (sony_dtv_system_t sys);
#endif
sony_result_t CXD2837_Scan(struct cxd2837_state *state,uint32_t startFreqkHz,uint8_t bwMHz);

static struct cxd2837_state* cxd2837;


int cxd2837_set_addr(void *pstate,unsigned char ChipAddress, unsigned char regaddr)
{
	struct cxd2837_state *state = (struct cxd2837_state *)pstate;
	int  nRetCode = 0;
	char buffer[2];
	struct i2c_msg msg;
	

	buffer[0] = regaddr;
	memset(&msg, 0, sizeof(msg));
	msg.addr = ChipAddress;
	msg.flags &=  ~I2C_M_RD;  //write  I2C_M_RD=0x01
	msg.len = 1;
	msg.buf = buffer;
	nRetCode = i2c_transfer(state->i2c, &msg, 1);
	
	if(nRetCode != 1)
	{
		pr_error("cxd2837_writeregister reg 0x%x failure,ret %d!\n",regaddr,nRetCode);
		return 0;
	}
	return 1;   //success
}

////////////////////I2c & Delay////////////////////
int I2cReadWrite(void *pstate,int mode, unsigned char ChipAddress, unsigned char *Data, int NbData)
{
	int ret = 0;
	char regbuf[1];
	struct cxd2837_state *state = (struct cxd2837_state *)pstate;
    //ChipAddress = 0xA5;
    //pr_dbg("cxd2837 read register mode 0x%x,ChipAddress 0x%x\n",mode,ChipAddress);
	if(Data == 0 || NbData == 0)
	{
		pr_err("cxd2837 read register parameter error !!\n");
		return 0;
	}
    
	if(mode == 0) // read mode
	{ 
		struct i2c_msg msg;
 		memset(&msg, 0, sizeof(msg));
		msg.addr = ChipAddress;
		msg.flags |=  I2C_M_RD;  //write  I2C_M_RD=0x01
		msg.len = NbData;
		msg.buf = Data;
 		ret = i2c_transfer(state->i2c,  &msg, 1);
  	}
    
	else if(mode == 3)
	{
		struct i2c_msg msg; 
		cxd2837_set_addr(pstate,ChipAddress, Data[0]) ; //set reg address first
 		//read real data 
		memset(&msg, 0, sizeof(msg));
		msg.addr = ChipAddress;
		msg.flags |=  I2C_M_RD;  //write  I2C_M_RD=0x01
		msg.len = NbData;
		msg.buf = Data;
		ret = i2c_transfer(state->i2c,  &msg, 1);
	
	}
	else
	{
      

		struct i2c_msg msg[2];

		regbuf[0] = Data[0] & 0xff;
		
		memset(msg, 0, sizeof(msg));
	

        #if 1
        /*write reg address*/
        
        /*write value*/
        msg[0].addr = ChipAddress;
        msg[0].flags &= ~I2C_M_RD;   /*i2c_transfer will emit a stop flag, so we should send 2 msg together,
                                                                     * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/
                                                                         
        msg[0].buf = Data;
        msg[0].len = NbData;
        
        if((*msg[0].buf)== 0x09)
        {
           // dump_stack();
        }

        ret = i2c_transfer(state->i2c, msg, 1);

        #else
            /*write reg address*/
            msg[0].addr = ChipAddress;                  
            msg[0].flags = 0;
            msg[0].buf = regbuf;
            msg[0].len = 1;
            
            /*write value*/
            msg[1].addr = ChipAddress;
            msg[1].flags = I2C_M_NOSTART;   /*i2c_transfer will emit a stop flag, so we should send 2 msg together,
                                                                         * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/
                                                                             
            msg[1].buf = Data;
            msg[1].len = NbData;
            
            if((*msg[0].buf)== 0x09)
            {
               // dump_stack();
            }
            //pr_dbg("msg[0].buf 0x%x,msg[0].addr[0] 0x%x,msg[1].buf 0x%x,msg[1].addr 0x%x,",*msg[0].buf,msg[0].addr,*msg[1].buf,msg[1].addr);
            pr_dbg("cxd2837 i2c_transfer\n");
            ret = i2c_transfer(state->i2c, msg, 2);
		#endif
		
	}
	
	if(ret<0){
		pr_err (" %s: writereg error, errno is %d \n", __FUNCTION__, ret);
		return -1;
	}
	else
		return 0;
}
////////////////////I2c & Delay////////////////////
int TunerI2cReadWrite(void *pstate,int mode, unsigned char ChipAddress, unsigned char *Data, int NbData)
{
    int ret = 0;
    char regbuf[1];
    struct cxd2837_state *state = (struct cxd2837_state *)pstate;
    //ChipAddress = 0xA5;
    pr_dbg("cxd2837 read register mode 0x%x,ChipAddress 0x%x\n",mode,ChipAddress);
    if(Data == 0 || NbData == 0)
    {
        pr_err("cxd2837 read register parameter error !!\n");
        return 0;
    }
    
    if(mode == 0) // read mode
    { 
        struct i2c_msg msg;
        memset(&msg, 0, sizeof(msg));
        msg.addr = ChipAddress;
        msg.flags |=  I2C_M_RD;  //write  I2C_M_RD=0x01
        msg.len = NbData;
        msg.buf = Data;
        ret = i2c_transfer(state->i2c,  &msg, 1);
    }
    
    else if(mode == 3)
    {
        struct i2c_msg msg; 
        cxd2837_set_addr(pstate,ChipAddress, Data[0]) ; //set reg address first
        //read real data 
        memset(&msg, 0, sizeof(msg));
        msg.addr = ChipAddress;
        msg.flags |=  I2C_M_RD;  //write  I2C_M_RD=0x01
        msg.len = NbData;
        msg.buf = Data;
        ret = i2c_transfer(state->i2c,  &msg, 1);
    
    }
    else
    {
      
//      pr_dbg("cxd2837 i2c_transfe11111r\n");
        struct i2c_msg msg[2];
        pr_dbg("cxd2837 i2c_transfe2222 Data 0x%xr\n",Data);
        /*construct 2 msgs, 1 for reg addr, 1 for reg value, send together*/
        regbuf[0] = Data[0] & 0xff;
        
        pr_dbg("cxd2837 i2c_transfe33333r\n");
        memset(msg, 0, sizeof(msg));
    
    pr_dbg("cxd2837 i2c_transfe44444r\n");
        /*write value*/
        msg[0].addr = ChipAddress;
        msg[0].flags &= ~I2C_M_RD;   /*i2c_transfer will emit a stop flag, so we should send 2 msg together,
                                                                     * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/
                                                                         
        msg[0].buf = Data;
        msg[0].len = NbData;
        
        if((*msg[0].buf)== 0x09)
        {
           // dump_stack();
        }
        //pr_dbg("msg[0].buf 0x%x,msg[0].addr[0] 0x%x,msg[1].buf 0x%x,msg[1].addr 0x%x,",*msg[0].buf,msg[0].addr,*msg[1].buf,msg[1].addr);
       // pr_dbg("cxd2837 i2c_transfer\n");
        ret = i2c_transfer(state->i2c, msg, 1);

        
        
    }
    
    if(ret<0){
        pr_err (" %s: writereg error, errno is %d \n", __FUNCTION__, ret);
        return -1;
    }
    else
        return 0;
}

sony_result_t dvbtest2_i2c_Write(void *pstate,struct sony_i2c_t * pI2c, 
								uint8_t deviceAddress, 
								const uint8_t * pData, 
								uint32_t size,
								uint8_t mode)
{
    pstate = cxd2837;
 	return (sony_result_t) I2cReadWrite(pstate,1, deviceAddress, (uint8_t *)pData, size);
} 

sony_result_t dvbtest2_i2c_Read(void *pstate,struct sony_i2c_t * pI2c, 
							   uint8_t deviceAddress, 
							   uint8_t * pData, 
							   uint32_t size,
							   uint8_t mode)
{
    pstate = cxd2837;
    //printk("dvbtest2_i2c_Read\n");
	return (sony_result_t) I2cReadWrite(pstate,0,deviceAddress, (uint8_t *)pData, size);
}
sony_result_t dvbtest2_i2c_WriteGw(void *pstate,struct sony_i2c_t * pI2c, 
							   uint8_t deviceAddress, 
							   const uint8_t * pData, 
							   uint32_t size,
							   uint8_t mode)
{
 	uint8_t data[MAX_I2C_WRITE];
    struct i2c_msg msg[3];
        uint8_t rData[MAX_I2C_WRITE];
	//pstate = cxd2837;
	    int ret = 0;
	   // printk("dvbtest2_i2c_WriteGw\n");
	if ((!pI2c) || (!pData)) {
		   return SONY_RESULT_ERROR_ARG;
	   }
	
	if (size > (MAX_I2C_WRITE - (1 + 1))) {
			return SONY_RESULT_ERROR_ARG;
		}
//		printk("pdata 0x%x\n",*pData);	
 		memset(data,0,255);
        memset(rData,0,255);
        memset(msg,0,sizeof(msg));
        memcpy (rData, pData, size);
		data[0] = pI2c->gwSub;
		//data[1]= deviceAddress;

         msg[0].addr = pI2c->gwAddress;
         msg[0].flags &= ~I2C_M_RD;   /*i2c_transfer will emit a stop flag, so we should send 2 msg together,
                                                                          * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/                                                                     
         msg[0].buf = data;
         msg[0].len = 1;

         //rData[0]=deviceAddress;
         //memcpy (&rData[1], pData, size);
         
         msg[1].addr = 0x60;//pI2c->gwAddress;
         msg[1].flags &= ~I2C_M_RD;  //write  I2C_M_RD=0x01
         msg[1].len = size;
         msg[1].buf = rData;

         ret = i2c_transfer(cxd2837->i2c,msg, 2);
        
           //memcpy(pData,rData,size); 
          // printk("ret %d\n",ret);
         if(ret != 2)
         	{
         		return (sony_result_t)ret ;
         	}
         return SONY_RESULT_OK ;
		//memcpy (&data[2], pData, size);
		
	//return (sony_result_t) TunerI2cReadWrite(pstate,1,pI2c->gwAddress, data, (size+2 ));
 }	//L

sony_result_t dvbtest2_i2c_ReadGw(void *pstate,struct sony_i2c_t * pI2c, 
							   uint8_t deviceAddress, 
							   uint8_t * pData, 
							   uint32_t size,
							   uint8_t mode)
{	
    uint8_t data[50];
    uint8_t rData[50];
    
    int ret = 0;
    struct i2c_msg msg[4];
    int i;
    pstate = cxd2837;
    //printk("dvbtest2_i2c_ReadGw\n");
	if ((!pI2c) || (!pData)) {
		return SONY_RESULT_ERROR_ARG;
	}
	memset(msg,0,sizeof(msg));

    data[0] = pI2c->gwSub;
    msg[0].addr = pI2c->gwAddress;
    msg[0].flags &= ~I2C_M_RD ;   /*i2c_transfer will emit a stop flag, so we should send 2 msg together,
                                                                     * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/                                                                     
    msg[0].buf = data;
    msg[0].len = 1;

    
    msg[1].addr = 0x60;//pI2c->gwAddress;
    msg[1].flags |= I2C_M_RD;  //write  I2C_M_RD=0x01
    msg[1].len = 1;
    msg[1].buf = rData;

    msg[2].addr = 0x6C;//pI2c->gwAddress;
    //msg[2].flags =  (I2C_M_RD);  //write  I2C_M_RD=0x01
    msg[2].flags |= (I2C_M_NO_RD_ACK);  //write  I2C_M_RD=0x01
    msg[2].len = size;
    msg[2].buf = (uint8_t *)rData;
    ret = i2c_transfer(cxd2837->i2c,msg, 3);
	//memset(data,0,sizeof(data));
    //data[0] = pData[0];
	memset(pData,0,size);
  	memcpy(pData,rData,size); 
	for(i = 0;i<size;i++)
//    printk("ReadGW reg rData 0x%x ,ret = %d\n",rData[i],ret);
		if(ret != 3)
		{
			return (sony_result_t)ret ;
		}
		 return SONY_RESULT_OK ;

}	//LGI



unsigned char user_I2CReadArray(unsigned char SubAddress, short int nBytes, unsigned char *ReadData)
{
	ReadData[0]=SubAddress;
	//return dvbtest2_i2c_ReadGw(cxd2837,&cxd2837->tunerI2C,TUNER_IIC_DEV_ADR,ReadData,nBytes,0);
	return 0;
}
unsigned char user_I2CWriteArray(unsigned char SubAddress,short int nBytes, unsigned char *WriteData)
{
	unsigned char i=0;
	unsigned char u8Buff[20];

	if (nBytes > 20) return 1;
	u8Buff[0] = SubAddress;
	for(i=0; i<nBytes; i++)
	{
		u8Buff[1+i] = *(WriteData+i);
	}

	//return dvbtest2_i2c_WriteGw(cxd2837,&cxd2837->tunerI2C,TUNER_IIC_DEV_ADR,u8Buff,nBytes+1,0);
	return 0;
}

/*------------------------------------------------------------------------------
 Function: Example Scanning Callback
 ------------------------------------------------------------------------------*/

/**
 @brief Example scanning call-back function that implements 
        carrier offset compensation for DVB-T/T2/C.
        It also provides an example scan progress indicator.

        Carrier offset compensation allows the demodulator 
        to lock to signals that have large enough carrier offsets
        as to prevent TS lock.
        Carrier offset compensation is important to real world tuning
        and scanning and is highly recommended.

 @param pDriver The driver instance.
 @param pResult The scan result.

*/
/////////////////for tuner ///////////////
sony_result_t CXD2837tuner_Init(struct sony_tuner_terr_cable_t * pTuner)
{
	//initialize all static variables
	//g_prev_BWKHz=-1;
	//tuner_Init();
	//tuner_SetHLMode( TUNER_HP_DECT_ENABLE);
	//tuner_SetModes( TUNER_STBY_MODE, TUNER_GAIN_MODE, TUNER_INV_MODE, TUNER_IF_DC_MODE);
#ifdef SONY_EXAMPLE_TUNER_ASCOT2E
	printk("init ascot2e tuner!!!\n");
#endif
#ifdef	SONY_EXAMPLE_TUNER_ASCOT3
	printk("init ASCOT3 tuner!!!\n");
#endif
	//sony_dvb_ascot2d_Initialize (pTuner);
	return SONY_RESULT_OK;
}

void CXD2837_init(struct cxd2837_state *state)
{
	sony_result_t result = SONY_RESULT_OK;
//	sony_result_t tuneResult = SONY_RESULT_OK;

//	sony_integ_t integ;
//	sony_demod_t demod;
//	sony_tuner_terr_cable_t tunerTerrCable;
	#ifdef SONY_EXAMPLE_TUNER_ASCOT2D    
	sony_ascot2d_t ascot2d;
	#elif defined SONY_EXAMPLE_TUNER_ASCOT2E
//	sony_ascot2e_t ascot2e;
	#elif defined SONY_EXAMPLE_TUNER_ASCOT3
//	sony_ascot3_t ascot3;
	#endif
//	sony_i2c_t i2c;
//	sony_dvbt2_tune_param_t tuneParam;
//	int i ;
	cxd2837 = state;
	state->tunerI2C.gwAddress = SONY_DVB_DEMOD_ADDRESS;
	state->tunerI2C.gwSub = 0x00;	   /* Connected via demod I2C gateway function. */
	state->tunerI2C.Read = dvbtest2_i2c_Read;
	state->tunerI2C.Write = dvbtest2_i2c_Write;
	state->tunerI2C.ReadRegister = sony_i2c_CommonReadRegister;
	state->tunerI2C.WriteRegister = sony_i2c_CommonWriteRegister;
	state->tunerI2C.WriteOneRegister = sony_i2c_CommonWriteOneRegister;
		
	state->demodI2C.gwAddress = 0x00; 
	state->demodI2C.gwSub = 0x00;	   /* N/A */
	state->demodI2C.Read = dvbtest2_i2c_Read;
	state->demodI2C.Write = dvbtest2_i2c_Write;
	state->demodI2C.ReadRegister = sony_i2c_CommonReadRegister;
	state->demodI2C.WriteRegister = sony_i2c_CommonWriteRegister;
	state->demodI2C.WriteOneRegister = sony_i2c_CommonWriteOneRegister;
	#ifdef CONFIG_TH_CXD2837_DVBC_ENABLE
	state->demod.system=SONY_DTV_SYSTEM_DVBC;
	#else
	state->demod.system=SONY_DTV_SYSTEM_DVBT;
	#endif
	/* Create ASCOT2E tuner using the parameters defined in sony_example_terr_cable_configuration.h */
	{
#ifdef SONY_EXAMPLE_TUNER_ASCOT2E
	uint8_t xtalFreqMHz = SONY_EXAMPLE_TUNER_XTAL;
	uint8_t i2cAddress = SONY_EXAMPLE_TUNER_I2C_ADDRESS;
	uint32_t configFlags = SONY_EXAMPLE_TUNER_FLAGS;
	result = sony_tuner_ascot2e_Create (&(state->tuner), xtalFreqMHz, i2cAddress, &(state->tunerI2C), configFlags, &(state->ascot2e));
#endif
#ifdef	SONY_EXAMPLE_TUNER_ASCOT3
	  uint8_t xtalFreq = SONY_EXAMPLE_TUNER_XTAL;
	  uint8_t i2cAddress = 0x60;
	  uint32_t configFlags = SONY_EXAMPLE_TUNER_FLAGS;
	  result = sony_tuner_ascot3_Create (&(state->tuner), xtalFreq, i2cAddress, &(state->tunerI2C), configFlags, &(state->ascot3));
#endif
#ifdef CONFIG_TH_CXD2837_TUNER_MXL603
	 uint8_t xtalFreq = 16;
	  uint8_t i2cAddress = 0x60;
	  uint32_t configFlags = 0;
#endif

	if (result == SONY_RESULT_OK) {
		printk (" Tuner Created with the following parameters:\n");
#ifdef	SONY_EXAMPLE_TUNER_ASCOT2E
		printk ("  - Tuner Type     : CXD2861 (ASCOT2E) \n");
		printk ("  - XTal Frequency : %uMHz\n", xtalFreqMHz);
#endif
#ifdef	SONY_EXAMPLE_TUNER_ASCOT3
        printk("  - Tuner Type     : CXD2871/72 (ASCOT3) \n");
        printk("  - XTal Frequency : %sMHz\n", ASCOT3_Xtal[xtalFreq]);
#endif
		printk ("  - I2C Address    : %u\n", i2cAddress);
		printk ("  - Config Flags   : %u\n\n", configFlags);
	}
	else {
		printk (" Error: Unable to create Sony ASCOT2E tuner driver. (result = %s)\n", Common_Result[result]);
		return;
	 	}
	}
	/* Create the integration structure which contains the demodulaor and tuner part instances.  This 
	* function also internally Creates the demodulator part.  Once created the driver is in 
	* SONY_DEMOD_STATE_INVALID and must be initialized before calling a Tune / Scan or Monitor API. 
	*/

	/* The following settings are taken from sony_example_terr_cable_configuration.h */
	{
	sony_demod_xtal_t xtalFreq = SONY_EXAMPLE_DEMOD_XTAL;
	uint8_t i2cAddress =0x6c;// SONY_EXAMPLE_DEMOD_I2C_ADDRESS;
	/* Create parameters for integration structure:
	*  sony_integ_t * pInteg                       Integration object
	*  sony_demod_xtal_t xtalFreq                  Demodulator xTal frequency
	*  uint8_t i2cAddress                          Demodulator I2C address
	*  sony_i2c_t i2c                              Demodulator I2C driver
	*  sony_demod_t *pDemod                        Demodulator object
	*
	*  Note: Set the following to NULL to disable control
	*  sony_tuner_terr_cable_t * pTunerTerrCable   Terrestrial / Cable tuner object 
	*  sony_tuner_sat_t * pTunerSat                Satellite tuner object
	*  sony_lnbc_t * pLnbc                         LNB Controller object
	*/
	#ifdef CONFIG_TH_CXD2837_TUNER_MXL603
		MxLWare603_SetPoint((void *)(cxd2837->i2c));
	#endif
	result = sony_integ_Create (&(state->device), xtalFreq, i2cAddress, &(state->demodI2C), &(state->demod)
	#ifdef SONY_DEMOD_SUPPORT_TERR_OR_CABLE
	/* Terrestrial and Cable supported so include the tuner object into the Create API */
	,&(state->tuner)
	#endif
	#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
	/* Satellite supported so include the tuner and LNB objects into the Create API */
	, NULL, NULL
	#endif
	);
	if (result == SONY_RESULT_OK) {
		printk (" Demod Created with the following parameters:\n");
		printk ("  - XTal Frequency : %s\n", Common_DemodXtal[xtalFreq]);
		printk ("  - I2C Address    : %u\n\n", i2cAddress);
	}else {
		printk (" Error: Unable to create demodulator driver. (result = %s)\n", Common_Result[result]);
		return ;
	}
	}
	result = sony_integ_InitializeT_C (&(state->device));
	if (result == SONY_RESULT_OK) {
		printk (" Driver initialized, current state = SONY_DEMOD_STATE_SLEEP_T_C\n\n");
	}else{
		printk (" Error: Unable to initialise the integration driver to terrestiral / cable mode. (result = %s)\n", Common_Result[result]);
		return ;
	}

	/* ---------------------------------------------------------------------------------
	* Configure the Demodulator
	* ------------------------------------------------------------------------------ */
	/* DVB-T demodulator IF configuration for terrestrial / cable tuner */
	state->demod.iffreqConfig.configDVBT_5 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT_5MHz_IF);
	state->demod.iffreqConfig.configDVBT_6 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT_6MHz_IF);
	state->demod.iffreqConfig.configDVBT_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT_7MHz_IF);
	state->demod.iffreqConfig.configDVBT_8 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT_8MHz_IF);
	
	/* DVB-T2 demodulator IF configuration for terrestrial / cable tuner */
	state->demod.iffreqConfig.configDVBT2_1_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_1_7MHz_IF);
	state->demod.iffreqConfig.configDVBT2_5 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_5MHz_IF);
	state->demod.iffreqConfig.configDVBT2_6 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_6MHz_IF);
	state->demod.iffreqConfig.configDVBT2_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_7MHz_IF);
	state->demod.iffreqConfig.configDVBT2_8 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_8MHz_IF);

	#ifdef CONFIG_TH_CXD2837_DVBC_ENABLE
		state->demod.iffreqConfig.configDVBC = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBC_IF);
	#endif

//	printk ("------------------------------------------\n");
//	printk (" Demodulator configuration \n");
//	printk ("------------------------------------------\n");
	/* Run through the defined Set Config options */
	{
	uint8_t configIndex = 0;
	uint8_t configCount = sizeof(demodulatorConfiguration) / sizeof(sony_example_demod_configuration_t);
	for (configIndex = 0 ; configIndex < configCount ; configIndex++) {
	result = sony_demod_SetConfig (&(state->demod),demodulatorConfiguration[configIndex].configId,
									demodulatorConfiguration[configIndex].configValue);
	if (result == SONY_RESULT_OK) {
		printk (" %u. %s set to %u\n", configIndex,Common_ConfigId[demodulatorConfiguration[configIndex].configId],
		demodulatorConfiguration[configIndex].configValue);
		}else {
		printk (" Error setting %s to %u (result = %s)\n", Common_ConfigId[demodulatorConfiguration[configIndex].configId],
		demodulatorConfiguration[configIndex].configValue,Common_Result[result]);
		return ;
		}
	}
	}
}
uint32_t opfreq;
uint8_t opbwMHz;

/*------------------------------------------------------------------------------
 Const char definitions
 ------------------------------------------------------------------------------*/
#ifndef CONFIG_TH_CXD2837_DVBC_ENABLE
static const char *DVBT_Profile[] = { "HP", "LP" };
static const char *DVBT2_TuneInfo[] = { "OK", "Invalid PLP ID", "Invalid T2 Mode"};
static const char *DVBT2_Profile[] = { "T2-Base", "T2-Lite", "Any" };
#endif
static uint8_t plpidold = 0;
static uint8_t plpidnew = 0;

sony_result_t CXD2837_Scan(struct cxd2837_state *state,uint32_t startFreqkHz,uint8_t bwMHz)
{
#ifndef CONFIG_TH_CXD2837_DVBC_ENABLE
  	unsigned int modulation_mode=state->mode;
#endif
//	sony_result_t result = SONY_RESULT_OK;
	sony_result_t tuneResult = SONY_RESULT_OK;
//	sony_integ_t integ;
//	sony_demod_t demod;
//	sony_tuner_terr_cable_t tunerTerrCable;
#ifdef SONY_EXAMPLE_TUNER_ASCOT2E
//	sony_ascot2e_t ascot2e;
#endif
#ifdef	SONY_EXAMPLE_TUNER_ASCOT3
//	sony_ascot3_t ascot3;
#endif
//	sony_i2c_t i2c;
#ifndef CONFIG_TH_CXD2837_DVBC_ENABLE
	sony_dvbt2_tune_param_t tuneParam;

        sony_dvbt_tune_param_t dvbttuneParam;
#endif
//	int i;

	#ifdef CONFIG_TH_CXD2837_DVBC_ENABLE
		sony_dvbc_tune_param_t dvtcTunerParam;
		dvtcTunerParam.centerFreqKHz = startFreqkHz/1000;

		printk (" Tune to DVB-C signal with the following parameters:\n");
		printk ("  - Center Freq    : %uKHz\n", dvtcTunerParam.centerFreqKHz);

		/* Perform DVBC Tune */
		tuneResult = sony_integ_dvbc_Tune (&(state->device), &dvtcTunerParam);
		printk ("  - DVBC - Result         : %s\n\n", Common_Result[tuneResult]);

		if (tuneResult == SONY_RESULT_OK) {
			 SONY_TRACE_RETURN (tuneResult);
		}
		return tuneResult;
	#else

		/*------------------------------------------------------------------------------
		   Scan - DVB-T/T2 multiple standard scan.
		  ------------------------------------------------------------------------------*/

		//	printk ("------------------------------------------\n");
		//	printk (" Demodulator configuration \n");
		//	printk ("------------------------------------------\n");
		if((modulation_mode==0))
		{
			/* Configure the DVBT tune parameters based on the channel requirements */
			dvbttuneParam.bandwidth = bwMHz;//SONY_DEMOD_BW_8_MHZ;          /* Channel bandwidth */
			dvbttuneParam.centerFreqKHz = startFreqkHz/1000;                   /* Channel centre frequency in KHz */
			dvbttuneParam.profile = SONY_DVBT_PROFILE_HP;           /* Channel profile for hierachical modes.  For non-hierachical use HP */

			printk (" Tune to DVB-T signal with the following parameters:\n");
			printk ("  - Center Freq    : %uKHz\n", dvbttuneParam.centerFreqKHz);
			printk ("  - Bandwidth      : %s\n", Common_Bandwidth[dvbttuneParam.bandwidth]);
			printk ("  - Profile        : %s\n", DVBT_Profile[dvbttuneParam.profile]);

			/* Perform DVBT Tune */
			tuneResult = sony_integ_dvbt_Tune (&(state->device), &dvbttuneParam);
			printk ("  - Result         : %s\n\n", Common_Result[tuneResult]);

			if (tuneResult == SONY_RESULT_OK) {
				 SONY_TRACE_RETURN (tuneResult);
			}

		}
		else
		{
			/* Configure the DVBT2 tune parameters based on the channel requirements */
			tuneParam.bandwidth = bwMHz;// SONY_DEMOD_BW_8_MHZ;          /* Channel bandwidth */
			tuneParam.centerFreqKHz = startFreqkHz/1000;                   /* Channel center frequency in KHz */
			tuneParam.dataPlpId = 0;                            /* PLP ID where multiple PLP's are available */
			tuneParam.profile = SONY_DVBT2_PROFILE_BASE;        /* Channel profile is T2-Base */
			/* Additional tune information fed back from the driver.  This parameter should be checked
			if the result from the tune call is SONY_RESULT_OK_CONFIRM. */
			tuneParam.tuneInfo = SONY_DEMOD_DVBT2_TUNE_INFO_OK;

		        tuneParam.dataPlpId = plpidnew;
			plpidold = plpidnew;

			printk (" Tune to DVB-T2 signal with the following parameters:\n");
			printk ("  - Center Freq    : %uKHz\n", tuneParam.centerFreqKHz);
			printk ("  - Bandwidth      : %s\n", Common_Bandwidth[tuneParam.bandwidth]);
			printk ("  - PLP ID         : %u\n", tuneParam.dataPlpId);
			printk ("  - Profile        : %s\n", DVBT2_Profile[tuneParam.profile]);
			/* Perform DVBT2 Tune */
			tuneResult = sony_integ_dvbt2_Tune (&(state->device), &tuneParam);
			printk ("  - Result         : %s\n", Common_Result[tuneResult]);
			printk ("  - Tune Info      : %s\n\n", DVBT2_TuneInfo[tuneParam.tuneInfo]);
		}
	#endif
	return tuneResult;
}
#if 0
static const char *FormatResult (sony_result_t result)
{
    char *pErrorName = "UNKNOWN";
    switch (result) {
    case SONY_RESULT_OK:
        pErrorName = "OK";
        break;
    case SONY_RESULT_ERROR_TIMEOUT:
        pErrorName = "ERROR_TIMEOUT";
        break;
    case SONY_RESULT_ERROR_UNLOCK:
        pErrorName = "ERROR_UNLOCK";
        break;
    case SONY_RESULT_ERROR_CANCEL:
        pErrorName = "ERROR_CANCEL";
        break;
    case SONY_RESULT_ERROR_ARG:
        pErrorName = "ERROR_ARG";
        break;
    case SONY_RESULT_ERROR_I2C:
        pErrorName = "ERROR_I2C";
        break;
    case SONY_RESULT_ERROR_SW_STATE:
        pErrorName = "ERROR_SW_STATE";
        break;
    case SONY_RESULT_ERROR_HW_STATE:
        pErrorName = "ERROR_HW_STATE";
        break;
    case SONY_RESULT_ERROR_RANGE:
        pErrorName = "ERROR_RANGE";
        break;
    case SONY_RESULT_ERROR_NOSUPPORT:
        pErrorName = "ERROR_NOSUPPORT";
        break;
    case SONY_RESULT_ERROR_OTHER:
        pErrorName = "ERROR_OTHER";
        break;
    default:
        pErrorName = "ERROR_UNKNOWN";
        break;
    }
    return pErrorName;
}

static const char *FormatSystem (sony_dtv_system_t sys)
{
    char *pName = "Unknown";
    switch (sys) {
    case SONY_DTV_SYSTEM_DVBC:
        pName = "DVB-C";
        break;
    case SONY_DTV_SYSTEM_DVBT:
        pName = "DVB-T";
        break;
    case SONY_DTV_SYSTEM_DVBT2:
        pName = "DVB-T2";
        break;
    case SONY_DTV_SYSTEM_DVBC2:
        pName = "DVB-C2";
        break;
    case SONY_DTV_SYSTEM_UNKNOWN:
    default:
        break;
    }
    return pName;
}
#endif
sony_result_t CXD2837_GetTPLock(struct cxd2837_state *state)
{	
	uint8_t syncState = 0;
	uint8_t tsLock = 0;
	uint8_t earlyUnlock = 0;
#ifndef CONFIG_TH_CXD2837_DVBC_ENABLE
	unsigned int modulation_mode = state->mode;
#endif	
	sony_result_t result = SONY_RESULT_ERROR_UNLOCK;

	#ifdef CONFIG_TH_CXD2837_DVBC_ENABLE
		result = sony_demod_dvbc_monitor_SyncStat (state->device.pDemod, &syncState, &tsLock, &earlyUnlock);
		if (result == SONY_RESULT_OK) {
			//	printk (" SyncStat                | SyncStat        | %lu\n", syncState);
			pr_dbg("debug27 DVB_C                       | TS Lock         | %s\n", Common_YesNo[tsLock]);
			//	printk ("T                        | Early Unlock    | %s\n", Common_YesNo[earlyUnlock]);
		}else{
			pr_dbg(" debug27 DVB_C            | Error           | %s\n", Common_Result[result]);
		}

		 if (result != SONY_RESULT_OK) {
		            return SONY_RESULT_ERROR_UNLOCK;
		}

		if(tsLock) return SONY_RESULT_OK;
			else return SONY_DEMOD_LOCK_RESULT_UNLOCKED;

	#else
		if(modulation_mode == 0){
			result = sony_demod_dvbt_monitor_SyncStat (state->device.pDemod, &syncState, &tsLock, &earlyUnlock);
			if (result == SONY_RESULT_OK) {
				//	printk (" SyncStat                | SyncStat        | %lu\n", syncState);
				pr_dbg ("T                        | TS Lock         | %s\n", Common_YesNo[tsLock]);
				//	printk ("T                        | Early Unlock    | %s\n", Common_YesNo[earlyUnlock]);
			}else {
				pr_dbg(" SyncStat   T            | Error           | %s\n", Common_Result[result]);
			}
		//printk ("-------------------------|-----------------|----------------- \n");
	        if (result != SONY_RESULT_OK) {
			            return SONY_RESULT_ERROR_UNLOCK;
			}
		if(tsLock) return SONY_RESULT_OK;
			else return SONY_DEMOD_LOCK_RESULT_UNLOCKED;
		}
		else
		{
			syncState = 0;
			tsLock = 0;
			earlyUnlock = 0;

			result = sony_demod_dvbt2_monitor_SyncStat (state->device.pDemod, &syncState, &tsLock, &earlyUnlock);
			if (result == SONY_RESULT_OK) {
				//	printk (" SyncStat                | SyncStat        | %lu\n", syncState);
				pr_dbg ("T2                       | TS Lock         | %s\n", Common_YesNo[tsLock]);
				//	printk ("T2                       | Early Unlock    | %s\n", Common_YesNo[earlyUnlock]);
			}else {
				pr_dbg (" SyncStat     T2         | Error           | %s\n", Common_Result[result]);
			}
			//printk ("-------------------------|-----------------|----------------- \n");

			if (result != SONY_RESULT_OK) {
				return SONY_RESULT_ERROR_UNLOCK;
			}
			if(tsLock) return SONY_RESULT_OK;
			else return SONY_DEMOD_LOCK_RESULT_UNLOCKED;
		}
	#endif
}

void demod_reset(struct cxd2837_state *state)
{
	//gpio_request(state->config.reset_pin, "cxd2837:RESET");
	pr_info("the reset pin is %x\n",state->config.reset_pin);
	//state->config.reset_pin = 6;
    
//	gpio_out(state->config.reset_pin, 0);
	amlogic_gpio_request(state->config.reset_pin, "cxd2837_reset");
	amlogic_gpio_direction_output(state->config.reset_pin, 0, "cxd2837_reset");
    
	pr_dbg("Jay reset pin \n");
	msleep(600);
//	gpio_out(state->config.reset_pin, 1);
	amlogic_gpio_direction_output(state->config.reset_pin, 1, "cxd2837_reset");
	msleep(200);
}


int  demod_init(struct cxd2837_state *state)
{
	demod_reset(state);						// need reset demod on our board		
	
	state->i2c = i2c_get_adapter(state->config.i2c_id);
	if (!state->i2c) {
		pr_error("cannot get i2c adaptor id(%x)\n", state->config.i2c_id);
	}
	CXD2837_init(state);
				
	return 0;
}
int demod_check_locked(unsigned char* lock)
{
	if(CXD2837_GetTPLock(cxd2837)==SONY_RESULT_OK)
	{
		pr_dbg("demod_check_locked !!\n");
		*lock =1;
		return 1;
	}
	*lock =0;
	return 0;

}
int demod_connect(struct cxd2837_state *state,unsigned int freq_khz, unsigned char bandwidth)
{
	unsigned char wbMHz;

	switch(bandwidth)
		{
			case 0:
				wbMHz = 8;
				break;
			case 1: 
				wbMHz = 7;
				break;
			case 2: 
				wbMHz = 6;
				break;				
				
			default:
				wbMHz = 8;
				break;	
		}
	CXD2837_Scan(state,freq_khz,wbMHz);
	return 0;
}
int demod_disconnect(void)
{
	return 0;
}
int demod_get_signal_strength(struct cxd2837_state* state,unsigned int* strength)
{
	int32_t pSNR;
#ifdef CONFIG_TH_CXD2837_DVBC_ENABLE
	sony_demod_dvbc_monitor_SNR (&(state->demod), &pSNR);
	*strength =pSNR/400;
	if(*strength >= 100) *strength = 99;
#else
	uint32_t uiPostBer,uiPreBer;
	unsigned int modulation_mode=state->mode;

	if(modulation_mode==0){
			sony_demod_dvbt_monitor_PreViterbiBER(&(state->demod), &uiPreBer);
		
			sony_demod_dvbt_monitor_PreRSBER(&(state->demod), &uiPostBer);
			
			sony_demod_dvbt_monitor_SNR(&(state->demod), &pSNR);
		}
	else
	{
		sony_demod_dvbt2_monitor_SNR(&(state->demod), &pSNR);
	}
	if(pSNR<0)
		*strength =5;
	else
		*strength =pSNR/1000;
#endif	
	return 0;
}
int demod_get_signal_quality(struct cxd2837_state *state,unsigned int* quality)
{
#ifdef CONFIG_TH_CXD2837_DVBC_ENABLE
	uint32_t pQuality;
	sony_demod_dvbc_monitor_Quality(&(state->demod), &pQuality);
//	sony_demod_dvbc_monitor_PreRSBER(&(state->demod), &pQuality);
	*quality =pQuality;
	if(*quality >= 100) *quality = 99;
#else
	uint8_t pQuality;
	unsigned int modulation_mode=state->mode;
	if(modulation_mode==0){
	sony_demod_dvbt_monitor_Quality(&(state->demod), &pQuality);
		}
	else
	{
		sony_demod_dvbt2_monitor_Quality(&(state->demod), &pQuality);
	}
		*quality =pQuality;
#endif
	return 0;
}
int demod_get_signal_errorate(struct cxd2837_state *state,unsigned int* errorrate)
{
	uint32_t uiPostBer;//,uiPreBer;
#ifdef CONFIG_TH_CXD2837_DVBC_ENABLE
	sony_demod_dvbc_monitor_PER (&(state->demod), &uiPostBer);
	*errorrate = uiPostBer;
#else
	sony_demod_dvbt_monitor_PreRSBER(&(state->demod), &uiPostBer);
	*errorrate = uiPostBer;
#endif
	return 0;
}

int demod_set_data_plp(struct cxd2837_state *state, uint8_t plp_id)
{
	sony_result_t ret;
	
	printk("Sony DVBT2 cxd2837 set plp, id=%d\n", plp_id);
	if(plpidold != plp_id)
		plpidnew = plp_id;
	ret = sony_demod_dvbt2_SetPLPConfig(&(state->demod), 0x0, plp_id);
	if (ret != SONY_RESULT_OK) {
		printk("Sony DVBT2 cxd2837 set plp config failed, ret=0x%x\n", ret);
		return 1;
	}
	printk("Sony DVBT2 cxd2837 set plp config ok\n");
	return 0;
}

int demod_get_active_data_plp(struct cxd2837_state *state, sony_dvbt2_plp_t *plp_info)
{
	sony_result_t ret;
	
	ret = sony_demod_dvbt2_monitor_ActivePLP(&(state->demod), SONY_DVBT2_PLP_DATA, plp_info);
	if (ret != SONY_RESULT_OK) {
		printk("Sony DVBT2 get active data plp failed, ret=0x%x\n", ret);
		return 1;
	}
	return 0;
}

int demod_get_data_plps(struct cxd2837_state *state, uint8_t *plp_ids, uint8_t *plp_num)
{
	sony_result_t ret;
	int wait_time = 0;

	*plp_num = 0;
	for(;;) {
    	ret = sony_demod_dvbt2_monitor_DataPLPs(&(state->demod), plp_ids, plp_num);
        if (ret == SONY_RESULT_OK) {
            break;
        } else if (ret == SONY_RESULT_ERROR_HW_STATE) {
            if (wait_time >= SONY_DVBT2_L1POST_TIMEOUT) {
				printk("Sony DVBT2 get data plps timeout\n");
				return 0;
            } else {
                msleep (DEMOD_TUNE_POLL_INTERVAL);
                wait_time += DEMOD_TUNE_POLL_INTERVAL;
            }
        } else {
        	printk("Sony DVBT2 get data plps failed, ret=0x%x\n", ret);
           return 1;
        }
    }

	printk("Sony DVBT2 cxd2837 get data plps: num=%d\n", *plp_num);
	return 0;
}

int demod_deinit(struct cxd2837_state *state)
{
	if(state->i2c)
	{
		i2c_put_adapter(state->i2c);
	}

	return 0;
}


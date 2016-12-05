/*****************************************************************************************
 *
 * FILE NAME          : MxL603_OEM_Drv.c
 * 
 * AUTHOR             : Mahendra Kondur
 *
 * DATE CREATED       : 12/23/2011  
 *
 * DESCRIPTION        : This file contains I2C driver and Sleep functins that 
 *                      OEM should implement for MxL603 APIs
 *                             
 *****************************************************************************************
 *                Copyright (c) 2011, MaxLinear, Inc.
 ****************************************************************************************/
#include <linux/delay.h>
#include <linux/mutex.h>

#include <linux/dvb/frontend.h>
#include <linux/i2c.h>
#include "MxL603_OEM_Drv.h"


struct i2c_adapter * pMxl603Point = NULL ;
//#include "MN_TCB.h" 
unsigned char DemMode=39;

/*----------------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_OEM_WriteRegister
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 7/30/2009
--|
--| DESCRIPTION   : This function does I2C write operation.
--|
--| RETURN VALUE  : True or False
--|
--|-------------------------------------------------------------------------------------*/
int I2CWrite(UINT8 I2CSlaveAddr, UINT8 *data, UINT8 length)
 {
	//printk("\n[I2CWrite] enter I2CSlaveAddr is %x,length is %d,data is %x, %x,%x\n",I2CSlaveAddr,length,data[0],data[1],data[2]);
//	printk("I2CSlaveAddr is %d\n",I2CSlaveAddr);
   /* I2C write, please port this function*/
	int ret = 0;
//	unsigned char regbuf[1];			/*8 bytes reg addr, regbuf 1 byte*/
	struct i2c_msg msg;			/*construct 2 msgs, 1 for reg addr, 1 for reg value, send together*/
	struct i2c_adapter *p = pMxl603Point;//Cxd2837_getPoint();

//	regbuf[0] = I2CSlaveAddr & 0xff;

	memset(&msg, 0, sizeof(msg));

	/*write reg address*/
/*	msg[0].addr = (state->config.demod_addr);					
	msg[0].flags = 0;
	msg[0].buf = regbuf;
	msg[0].len = 1;*/


	/*write value*/
	msg.addr = I2CSlaveAddr;
	msg.flags = 0;  //I2C_M_NOSTART;	/*i2c_transfer will emit a stop flag, so we should send 2 msg together,
																// * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/
	msg.buf = data;
	msg.len = length;

//	struct i2c_adapter *p = pMxl603Point;//Cxd2837_getPoint();
	ret = i2c_transfer(p, &msg, 1);
	if(ret<0){
		printk(" %s: writereg error, errno is %d \n", __FUNCTION__, ret);
		return 0;
	}
	else{
		//printk(" %s:write success, errno is %d \n", __FUNCTION__, ret);
		return 1;
	}
      return 1;
 }

int  I2CRead(UINT8 I2CSlaveAddr, UINT8 *data, UINT8 length)
{
	/* I2C read, please port this function*/
	//	printk("I2CSlaveAddr is %d,length is %d\n",I2CSlaveAddr,length);
	//		printk("I2CSlaveAddr is %d\n",I2CSlaveAddr);
	int nRetCode = 0;
	struct i2c_msg msg[1];
	struct i2c_adapter *p = pMxl603Point ;//Cxd2837_getPoint();

	if(data == 0 || length == 0)
	{
		printk("mn88436 read register parameter error !!\n");
		return 0;
	}

	//read real data 
	memset(msg, 0, sizeof(msg));
	msg[0].addr = I2CSlaveAddr;
	msg[0].flags |=  I2C_M_RD;  //write  I2C_M_RD=0x01
	msg[0].len = length;
	msg[0].buf = data;


//	struct i2c_adapter *p = pMxl603Point ;//Cxd2837_getPoint();
	nRetCode = i2c_transfer(p, msg, 1);

	if(nRetCode != 1)
	{
		printk("mn88436_readregister reg failure!\n");
		return 0;
	}
	return 1;
}

int DMD_I2C_Read(UINT8	slvadr , UINT8 adr , UINT8 *data )
{
	//TODO:	call I2C 1byte Write API
	//printk("slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
	UINT8 i2cSlaveAddr = slvadr;
	UINT8 u8Reg[1];
	u8Reg[0] = adr;
	I2CWrite(i2cSlaveAddr, u8Reg, 1);
	if(I2CRead(i2cSlaveAddr, data, 1)==0)
	{
		return 1;
	}
	return 0;
}

int DMD_I2C_Write(UINT8	slvadr , UINT8 adr , UINT8 data )
{
	//TODO:	Please call I2C 1byte Write API
    UINT8 i2cSlaveAddr = slvadr;//>>1;
    UINT8 u8Reg[2];
    u8Reg[0] = adr;
    u8Reg[1] = data;
	if(I2CWrite(i2cSlaveAddr, u8Reg, 2)==0)
    {
  		return 1;
    }
	return 0;
}
 
int Mxl603_Write_With_CXD2837(UINT8 devId, UINT8 RegAddr, UINT8 RegData)
{
	uint8_t data[100];
	struct i2c_msg msg[3];
	uint8_t wData[100];
	int ret = 0;
	struct i2c_adapter *p = pMxl603Point;//Cxd2837_getPoint();

	memset(data,0,sizeof(data));
	memset(wData,0,sizeof(wData));
	memset(msg,0,sizeof(msg));
	wData[0] = RegAddr;
	wData[1] = RegData;
	
	data[0] = 0x00;
	msg[0].addr = 0x36;//SONY_DVB_DEMOD_ADDRESS;
	msg[0].flags &= ~I2C_M_RD;   /*i2c_transfer will emit a stop flag, so we should send 2 msg together,
	                                                  * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/                                                                     
	msg[0].buf = data;
	msg[0].len = 1;

	msg[1].addr = devId;//pI2c->gwAddress;
	msg[1].flags &= ~I2C_M_RD;  //write  I2C_M_RD=0x01
	msg[1].len = 2;
	msg[1].buf = wData;

//	struct i2c_adapter *p = pMxl603Point;//Cxd2837_getPoint();
	if(p == NULL)
	{
		printk("debug25, null point i2c_adapter \n");
		return 1;
	}

	ret = i2c_transfer(p,msg, 2);
	if(ret != 2)
	{
		printk("debug25,Mxl603_Write_With_CXD2837 I2C error \n");
		return ret ;
	}
	return 1 ;
}	//L

#if 0
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
    int error,i;
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
    ret = i2c_transfer(cxd2837->i2c,&msg, 3);
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
#endif


MXL_STATUS MxLWare603_OEM_WriteRegister(UINT8 devId, UINT8 RegAddr, UINT8 RegData)
{
  // OEM should implement I2C write protocol that complies with MxL603 I2C
  // format.

  // 8 bit Register Write Protocol:
  // +------+-+-----+-+-+----------+-+----------+-+-+
  // |MASTER|S|SADDR|W| |RegAddr   | |RegData(L)| |P|
  // +------+-+-----+-+-+----------+-+----------+-+-+
  // |SLAVE |         |A|          |A|          |A| |
  // +------+---------+-+----------+-+----------+-+-+
  // Legends: SADDR (I2c slave address), S (Start condition), A (Ack), N(NACK), 
  // P(Stop condition)

  MXL_STATUS status = MXL_FALSE;
  
/* If OEM data is implemented, customer needs to use OEM data structure related operation 
   Following code should be used as a reference. 
   For more information refer to sections 2.5 & 2.6 of MxL603_mxLWare_API_UserGuide document.

  UINT8 i2cSlaveAddr;
  UINT8 i2c_bus;
  user_data_t * user_data = (user_data_t *) MxL603_OEM_DataPtr[devId];
 
  if (user_data)
  {
    i2cSlaveAddr = user_data->i2c_address;           // get device i2c address
    i2c_bus = user_data->i2c_bus;                   // get device i2c bus  
  
    sem_up(user_data->sem);                         // up semaphore if needed

    // I2C Write operation 
    status = USER_I2C_WRITE_FUNCTION(i2cSlaveAddr, i2c_bus, RegAddr, RegData);
    
    sem_down(user_data->sem);                       // down semaphore
    user_data->i2c_cnt++;                           // user statistics
  }

*/
  /* If OEM data is not required, customer should treat devId as I2C slave Address */
	//sony_i2c_CommonWriteOneRegister
	//sony_i2c_CommonWriteRegister
	//result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x44, 0x07);
       // result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x01, data, sizeof(data));
#if 1
	//status = Mxl603_Write_With_CXD2837(devId, RegAddr, RegData);
	status = DMD_I2C_Write(devId,RegAddr,RegData);
#else
#ifndef CONFIG_EDA31924_31804_MN88436_MXL603
	devId = devId;
	RegAddr = RegAddr;
	RegData = RegData;

	printf(" Tuner write in , devid[%x], regaddr[%x], data[%x] \n", devId, RegAddr,RegData);

	//status = DMD_TCB_WriteRead(devId, RegAddr, &RegData, 1, 0, 0);

	status = DMD_I2C_Write(devId,RegAddr,RegData);
#else
	unsigned char buf[20];
	if(DemMode==39)
	{//MN88436

		//	printf(" Tuner write in , devid[%x], regaddr[%x], data[%x] \n", devId, RegAddr,RegData);

		//	status = DMD_MN88436_TCB_Write(0xc0,RegAddr, RegData);
		buf[0]=0x15;
		buf[1]=0x53;
		I2CWrite(0x18,buf,2);

		buf[0]=0x17;
		buf[1]=0x00;
		I2CWrite(0x18,buf,2);
		//
		buf[0] = 0x18;
		buf[1] = 0x20;
		buf[2] = 0xc0;
		buf[3] = RegAddr;
		buf[4] = RegData;

		status = I2CWrite(buf[0],&buf[1],4);
		if(status)
		status = MXL_TRUE;
		else
		status = MXL_FALSE;
		if(status)
		printf("MN88436 write Tuner err   %x\n",devId);
		// DMD_MN88436_TCB_Read(UINT8	slvadr , UINT8 adr , UINT8 *data);

	}
#endif
#endif
  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_OEM_ReadRegister
--| 
--| AUTHOR        : Brenndon Lee
--|
--| DATE CREATED  : 7/30/2009
--|
--| DESCRIPTION   : This function does I2C read operation.
--|
--| RETURN VALUE  : True or False
--|
--|---------------------------------------------------------------------------*/

MXL_STATUS MxLWare603_OEM_ReadRegister(UINT8 devId, UINT8 RegAddr, UINT8 *DataPtr)
{
  // OEM should implement I2C read protocol that complies with MxL603 I2C
  // format.

  // 8 bit Register Read Protocol:
  // +------+-+-----+-+-+----+-+----------+-+-+
  // |MASTER|S|SADDR|W| |0xFB| |RegAddr   | |P|
  // +------+-+-----+-+-+----+-+----------+-+-+
  // |SLAVE |         |A|    |A|          |A| |
  // +------+-+-----+-+-+----+-+----------+-+-+
  // +------+-+-----+-+-+-----+--+-+
  // |MASTER|S|SADDR|R| |     |MN|P|
  // +------+-+-----+-+-+-----+--+-+
  // |SLAVE |         |A|Data |  | |
  // +------+---------+-+-----+--+-+
  // Legends: SADDR(I2c slave address), S(Start condition), MA(Master Ack), MN(Master NACK), 
  // P(Stop condition)

  MXL_STATUS status = MXL_TRUE;

/* If OEM data is implemented, customer needs to use OEM data structure related operation 
   Following code should be used as a reference. 
   For more information refer to sections 2.5 & 2.6 of MxL603_mxLWare_API_UserGuide document.

  UINT8 i2cSlaveAddr;
  UINT8 i2c_bus;
  user_data_t * user_data = (user_data_t *) MxL603_OEM_DataPtr[devId];
 
  if (user_data)
  {
    i2cSlaveAddr = user_data->i2c_address;           // get device i2c address
    i2c_bus = user_data->i2c_bus;                   // get device i2c bus  
  
    sem_up(user_data->sem);                         // up semaphore if needed

    // I2C Write operation 
    status = USER_I2C_READ_FUNCTION(i2cSlaveAddr, i2c_bus, RegAddr, DataPtr);
    
    sem_down(user_data->sem);                       // down semaphore
    user_data->i2c_cnt++;                           // user statistics
  }

*/

  /* If OEM data is not required, customer should treat devId as I2C slave Address */
//	int sub;
//  	unsigned char buf[20];
#if 1
#if 1
  devId = devId;
  RegAddr = RegAddr;
  *DataPtr = *DataPtr;

 
//  printk(" Tuner read  in , devid[%x], regaddr[%x] ,data[%x]\n", devId, RegAddr,*DataPtr);

   //status = DMD_TCB_WriteRead(devId, 0xFB, &RegAddr, 1, 0, 0);
   //status =  DMD_TCB_WriteRead(devId, 0, 0, 0, DataPtr, 1);

  status = DMD_I2C_Read(devId,RegAddr,DataPtr);
#else
buf[0]=0x15;
buf[1]=0x53;
I2CWrite(0x18,buf,2);

buf[0]=0x17;
buf[1]=0x00;
I2CWrite(0x18,buf,2);

		buf[0]=0x18;
		buf[1]=0x20;
		buf[2]=0xc0;
		buf[3]=0xfB;
		buf[4]=RegAddr;
		status = I2CWrite(buf[0],&buf[1],4);
		if(status)
		    status = MXL_TRUE;
		else
		      status = MXL_FALSE;
		if(status)
			printf("MN88472 I2C Mxl603 read Error.\n");

		buf[1]=0x20;
		buf[2]=0xc1;
		status = I2C_Write_Read(buf[0],&buf[1],2,DataPtr,1);
		if(status)
		    status = MXL_TRUE;
		else
		      status = MXL_FALSE;
		printf("Read %x  :%x   \n",RegAddr,*DataPtr);
		if(status)
				printf("MN88436 Read Tuner err   %x\n",devId);

#endif
#endif

  return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare603_OEM_Sleep
--| 
--| AUTHOR        : Dong Liu
--|
--| DATE CREATED  : 01/10/2010
--|
--| DESCRIPTION   : This function complete sleep operation. WaitTime is in ms unit
--|
--| RETURN VALUE  : None
--|
--|-------------------------------------------------------------------------------------*/

void MxLWare603_OEM_Sleep(UINT16 DelayTimeInMs)
{
  // OEM should implement sleep operation 
  #ifdef already_add_BE_API
  DHwTm_Timer_DelayMS( DelayTimeInMs); //sample code, pls. use your API .
  #endif
  msleep(DelayTimeInMs);
  
}

void  MxLWare603_SetPoint(void *pI2c)
{
	printk("debug27, call MxLWare603_SetPoint \n");
	pMxl603Point = (struct i2c_adapter*)pI2c;
	return ;
}
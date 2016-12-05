/* **************************************************** */
/*!
   @file	MN_DMD_I2C_WIN.c
   @brief	I2C communication wrapper
   @author	R.Mori
   @date	2011/6/30
   @param
		(c)	Panasonic
   */
/* **************************************************** */

#include "MN_DMD_driver.h"
#include "MN_I2C.h"
#include "MN_DMD_common.h"
/* **************************************************** */
/* **************************************************** */
#define DMD_I2C_MAXSIZE	127
/*! I2C Initialize Function*/
DMD_u32_t DMD_API DMD_I2C_open(void)
{
	//TODO:	Please call I2C initialize API here
	//this function is called by DMD_API_open

	return DMD_E_OK;
}
/*! Write 1byte */
DMD_u32_t DMD_I2C_Write(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t data )
{
	//TODO:	Please call I2C 1byte Write API
	#ifdef already_add_BE_API
	twsbSetData(slvadr, adr, data); //sample, pls. use your API 
	#endif
	
//	printk("slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
	DMD_u8_t i2cSlaveAddr = slvadr;//>>1;
    DMD_u8_t u8Reg[2];
    u8Reg[0] = adr;
    u8Reg[1] = data;
	if(I2CWrite(i2cSlaveAddr, u8Reg, 2)==0)
    {
  		return DMD_E_ERROR;
    }
	return DMD_E_OK;

	
	/*		;
			slvadr>>=1;
		msleep(500);
		printk("2slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
		i2cSlaveAddr = slvadr;//>>1;
		if(I2CWrite(i2cSlaveAddr, u8Reg, 2)==0)
				msleep(500);
		printk("3slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
		i2cSlaveAddr = 0x19;//>>1;
		if(I2CWrite(i2cSlaveAddr, u8Reg, 2)==0)
						msleep(500);
		printk("3slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
		i2cSlaveAddr = 0x19>>1;//>>1;
		if(I2CWrite(i2cSlaveAddr, u8Reg, 2)==0)
							msleep(500);
		printk("3slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
		i2cSlaveAddr = 0x1a;//>>1;
		if(I2CWrite(i2cSlaveAddr, u8Reg, 2)==0)
							msleep(500);
		printk("3slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
		i2cSlaveAddr = 0x1a>>1;//>>1;
		if(I2CWrite(i2cSlaveAddr, u8Reg, 2)==0)
								msleep(500);
		printk("3slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
		i2cSlaveAddr = 0x1b;//>>1;
		if(I2CWrite(i2cSlaveAddr, u8Reg, 2)==0)
					msleep(500);
		printk("3slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
		i2cSlaveAddr = 0x1b>>1;//>>1;
		if(I2CWrite(i2cSlaveAddr, u8Reg, 2)==0)*/
}

/*! Read 1byte */
DMD_u32_t DMD_I2C_Read(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t *data )
{
	//TODO:	call I2C 1byte Write API
	#ifdef already_add_BE_API
	*data = twsbGetData(slvadr, adr); //sample, pls. use your API 
	#endif 
	//printk("slvadr is %x,adr is %x,data is %x\n",slvadr,adr,data);
	DMD_u8_t i2cSlaveAddr = slvadr;
    DMD_u8_t u8Reg[1];
    u8Reg[0] = adr;
    I2CWrite(i2cSlaveAddr, u8Reg, 1);
    if(I2CRead(i2cSlaveAddr, data, 1)==0)
    {
  		return DMD_E_ERROR;
    }
	return DMD_E_OK;
}

/* 
Write/Read any Length to DMD and Tuner;
DMD_u8_t slvadr : DMD's slave address.   
DMD_u8_t adr    : DMD's REG address; 

For writing CMD to tuner, 
DMD_u8_t slvadr is DMD's slave address;
DMD_u8_t adr is DMD's register TCBCOM(0xF7)
!!!Tuner's slave addr. and REG addr. are as continous data< upper layer : data[x]> 
*/
DMD_u32_t DMD_I2C_Write_Anylenth(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t* wdata , DMD_u32_t wlen)
{

	//TODO:	Please call I2C Read/Write API here
#ifdef already_add_BE_API
	DMD_u8_t max_length_once = 127; //the maximum bytes BE be able to write to device, one time. 
	DMD_u8_t* curwdata;
	if(wlen == 0)
	{
	}
	else if(wlen < max_length_once)
	{
	 	twsbSetDatas(slvadr, adr, wdata, wlen);
	}
	else
	{
	 	curwdata = wdata;
	 	while(wlen>=max_length_once)
		{
			twsbSetDatas(slvadr, adr, curwdata, length);
			wlen -= max_length_once;
			curwdata += length;
		}
		if(wlen > 0)
		{
		 	twsbSetDatas(slvadr, adr, curwdata, wlen);
		}
	}			
#endif
	//printk("tuner slvadr is %x,adr is %x,data is %x\n",slvadr,adr,wdata);
	DMD_u8_t i2cSlaveAddr = slvadr;
    DMD_u8_t u8Reg[4];
    u8Reg[0] = adr;
    u8Reg[1] = wdata[0];
	u8Reg[2] = wdata[1];
	u8Reg[3] = wdata[2];
	if(I2CWrite(i2cSlaveAddr, u8Reg, 4)==0)
    {
  		return DMD_E_ERROR;
    }

	return DMD_E_OK;
}

/*
twsbGetData_MN88472_TCB(slvadr, adr, wdata)
This is specific function for MN88472 to read Tuner reg's data.
бя If you use MN88472 to control Tuner, you may need to create new I2C bottom code.

There are two steps to read Tuner, take Mxl603 for example. (See Tuner I2C read protocal as below)
Step 1 : Tell Tuner which Reg to read 
      -> Pls. call function DMD_I2C_Write_Anylenth()
Step 2 : Read Tuner Reg's data
      -> Pls. call function DMD_TCBI2C_Read()
------ Tuner Mxl603 I2C read protocal -------
// 8 bit Register Read Protocol:
  Step1
  // +------+-+-----+-+-+----+-+----------+-+-+
  // |MASTER|S|SADDR|W| |0xFB| |RegAddr   | |P|
  // +------+-+-----+-+-+----+-+----------+-+-+
  // |SLAVE |         |A|    |A|          |A| |
  // +------+-+-----+-+-+----+-+----------+-+-+
  Step2
  // +------+-+-----+-+-+-----+--+-+
  // |MASTER|S|SADDR|R| |     |MN|P|
  // +------+-+-----+-+-+-----+--+-+
  // |SLAVE |         |A|Data |  | |
  // +------+---------+-+-----+--+-+
  // SADDR(I2c slave address), S(Start condition), 
  // A(Slave Ack), MN(Master NACK),  P(Stop condition)
  
------Step2 I2C commnication between BE and DMD -------
  // +------+-+-----+-+-+----+-+----------+-+-++------+-+-----+-+-+----
  // |BE |S|SADDR|W| |R-addr| |SADDR-M|R| |S|SADDR|R| |      |MN|P|
  // +------+-+-----+-+-+----+-+----------+-+-++------+-+-----+-+-+----
  // |DMD|         |A|      |A|         |A|         |A| Data |  | |
  // +------+-+-----+-+-+----+-+----------+-+-++------+-+-----+-+-+----
  // SADDR(DMD's slave address), R-addr(DMD's reg addr. -> TCBCOM ),
  // SADDR-M(Tuner's slave address)
  You can also refer to file "PRODUCT_SPECIFICATIONS_MN88472_ver041_120120.pdf"
  page 34 to see the TCB control flow.
*/
DMD_u32_t DMD_TCBI2C_Read(DMD_u8_t slvadr , DMD_u8_t adr , DMD_u8_t* wdata , DMD_u32_t wlen , DMD_u8_t* rdata , DMD_u32_t rlen)
{
#ifdef already_add_BE_API
    DMD_u8_t* tuner_slave_addr = wdata;
	if(rlen)
	{
		*rdata=twsbGetData_MN88472_TCB(slvadr, adr, tuner_slave_addr); //sample, pls. use your API 
	}
	else
	{
		printf(" ########### There is no Rlen indicated while reading Tuner !!! ##################### ");
	}
#endif
//	printk("tuner slvadr is %x,adr is %x,data is %x\n",slvadr,adr,wdata);
	DMD_u8_t i2cSlaveAddr = slvadr;
    DMD_u8_t u8Reg[2];
//	u8Reg[0] = adr;
 //   I2CWrite(i2cSlaveAddr, u8Reg, 1);
	u8Reg[0]=adr;
	u8Reg[1]=*wdata;
  	I2CWrite(i2cSlaveAddr, u8Reg, 1);
    if(I2CRead(i2cSlaveAddr, rdata, 1)==0)
    {
  		return DMD_E_ERROR;
    }
	return DMD_E_OK;
}

/*! Write/Read any Length*/
// This is general API for you to communicate with external device which DIRECTLY connect to BE,
// However, to communicate with Tuner through DMD MN88472, you may need to modify Bottom code of I2C sequence. 
DMD_u32_t DMD_I2C_WriteRead(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t* wdata , DMD_u32_t wlen , DMD_u8_t* rdata , DMD_u32_t rlen)
{
	//TODO:	Please call I2C Read/Write API here
	return DMD_E_OK;
}

/*! Timer wait */
DMD_u32_t DMD_wait( DMD_u32_t msecond ){

	//TODO: call timer wait function 
	#ifdef already_add_BE_API
	DHwTm_Timer_DelayMS(msecond); //sample, pls. use your API 
	#endif
	msleep(msecond);
	return DMD_E_OK;
}

/*! Get System Time (ms) */
DMD_u32_t DMD_timer( DMD_u32_t* tim ){


	return DMD_E_OK;
}

extern int mn88436_get_fe_config(struct mn88436_fe_config *cfg);


extern DMD_u32_t I2CWrite(DMD_u8_t I2CSlaveAddr, DMD_u8_t *data, DMD_u8_t length)
 {
	//printk("\n[I2CWrite] enter I2CSlaveAddr is %x,length is %d,data is %x, %x,%x\n",I2CSlaveAddr,length,data[0],data[1],data[2]);
//	printk("I2CSlaveAddr is %d\n",I2CSlaveAddr);
   /* I2C write, please port this function*/
	int ret = 0;
//	unsigned char regbuf[1];			/*8 bytes reg addr, regbuf 1 byte*/
	struct i2c_msg msg;			/*construct 2 msgs, 1 for reg addr, 1 for reg value, send together*/
	struct	mn88436_fe_config cfg;
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
#if 0

	/*write reg address*/
	msg[0].addr = 0x80;					
	msg[0].flags = 0;
	msg[0].buf = 0x7;
	msg[0].len = 1;

	/*write value*/
	msg[1].addr = 0x80;
	msg[1].flags = I2C_M_NOSTART;	/*i2c_transfer will emit a stop flag, so we should send 2 msg together,
																 * and the second msg's flag=I2C_M_NOSTART, to get the right timing*/
	msg[1].buf = 0x8;
	msg[1].len = 1;

#endif

	mn88436_get_fe_config(&cfg);
	ret = i2c_transfer((struct i2c_adapter *)cfg.i2c_adapter, &msg, 1);
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
 
extern DMD_u32_t  I2CRead(DMD_u8_t I2CSlaveAddr, DMD_u8_t *data, DMD_u8_t length)
 {
     /* I2C read, please port this function*/
	//	printk("I2CSlaveAddr is %d,length is %d\n",I2CSlaveAddr,length);
//		printk("I2CSlaveAddr is %d\n",I2CSlaveAddr);
		 int nRetCode = 0;
		 struct i2c_msg msg[1];
		 struct	mn88436_fe_config cfg;
		 
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

		 mn88436_get_fe_config(&cfg);
		 
		 nRetCode = i2c_transfer((struct i2c_adapter *)cfg.i2c_adapter, msg, 1);
	 
		 if(nRetCode != 1)
		 {
			 printk("mn88436_readregister reg failure!\n");
			 return 0;
		 }
        return 1;
 }


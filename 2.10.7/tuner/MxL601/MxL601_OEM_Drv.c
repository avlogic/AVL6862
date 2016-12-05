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


/*****************************************************************************************
 *
 * FILE NAME          : MxL601_OEM_Drv.c
 * 
 * AUTHOR             : Dong Liu 
 *
 * DATE CREATED       : 01/23/2011  
 *
 * DESCRIPTION        : This file contains I2C driver and Sleep functins that 
 *                      OEM should implement for MxL601 APIs
 *                             
 *****************************************************************************************
 *                Copyright (c) 2010, MaxLinear, Inc.
 ****************************************************************************************/

#include "MxL601_OEM_Drv.h"
#include "AVL_Tuner.h"
/*----------------------------------------------------------------------------------------
--| FUNCTION NAME : Ctrl_WriteRegister
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

MXL_STATUS Ctrl_WriteRegister(UINT8 I2cSlaveAddr, UINT8 RegAddr, UINT8 RegData)
{
    MXL_STATUS status = MXL_TRUE;
    AVL_uint32 r = 0;

  // OEM should implement I2C write protocol that complies with MxL601 I2C
  // format.

  // 8 bit Register Write Protocol:
  // +------+-+-----+-+-+----------+-+----------+-+-+
  // |MASTER|S|SADDR|W| |RegAddr   | |RegData(L)| |P|
  // +------+-+-----+-+-+----------+-+----------+-+-+
  // |SLAVE |         |A|          |A|          |A| |
  // +------+---------+-+----------+-+----------+-+-+
  // Legends: SADDR (I2c slave address), S (Start condition), A (Ack), N(NACK), 
  // P(Stop condition)
  
	AVL_uchar buffer[2]={0};
	AVL_uint16 size=0;

	buffer[0]=RegAddr;
	buffer[1]=RegData;
	size=2;

	//r = AVL6381_TunerWrite(I2cSlaveAddr, buffer, size);
	r = AVL_IBSP_I2C_Write(I2cSlaveAddr,buffer,&size);

	if( r != 0)
	{
		status = MXL_FALSE;
	}

    return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : Ctrl_ReadRegister
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

MXL_STATUS Ctrl_ReadRegister(UINT8 I2cSlaveAddr, UINT8 RegAddr, UINT8 *DataPtr)
{
    MXL_STATUS status = MXL_TRUE;
  	AVL_uint32 r = 0;

  // OEM should implement I2C read protocol that complies with MxL601 I2C
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
  AVL_uchar buffer[2]={0};
	buffer[0]=0xFB;
	buffer[1]=RegAddr;
	AVL_uint16 size=0;

	//r = AVL6381_TunerRead(I2cSlaveAddr, buffer, 2, DataPtr, 1);
	size = 2;
	r = AVL_IBSP_I2C_Write(I2cSlaveAddr,buffer,&size);
	size = 1;
	r |= AVL_IBSP_I2C_Read(I2cSlaveAddr,DataPtr,&size);
	if( r != 0)
	{
		status = MXL_FALSE;
	}

    return status;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MXL_Sleep
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

void MxL_Sleep(UINT16 DelayTimeInMs)
{
  // OEM should implement sleep operation 
    AVL_IBSP_Delay(DelayTimeInMs);
}

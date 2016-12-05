/*****************************************************************************************
 *
 * FILE NAME          : MxL608_OEM_Drv.c
 * 
 * AUTHOR             : Mahendra Kondur
 *
 * DATE CREATED       : 12/23/2011  
 *
 * DESCRIPTION        : This file contains I2C driver and Sleep functins that 
 *                      OEM should implement for MxL608 APIs
 *                             
 *****************************************************************************************
 *                Copyright (c) 2011, MaxLinear, Inc.
 ****************************************************************************************/

#include "MxL608_OEM_Drv.h"
#include "AVL_Tuner.h"

//#include "gvariable.h"
//#include "AVL63X1_include.h"

extern struct AVL63X1_Chip g_Demod;
/*----------------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_OEM_WriteRegister
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

MXL608_STATUS MxLWare608_OEM_WriteRegister(UINT8_608 devId, UINT8_608 RegAddr, UINT8_608 RegData)
{
  // OEM should implement I2C write protocol that complies with MxL608 I2C
  // format.

  // 8 bit Register Write Protocol:
  // +------+-+-----+-+-+----------+-+----------+-+-+
  // |MASTER|S|SADDR|W| |RegAddr   | |RegData(L)| |P|
  // +------+-+-----+-+-+----------+-+----------+-+-+
  // |SLAVE |         |A|          |A|          |A| |
  // +------+---------+-+----------+-+----------+-+-+
  // Legends: SADDR (I2c slave address), S (Start condition), A (Ack), N(NACK), 
  // P(Stop condition)

  MXL608_STATUS status = MXL608_FALSE;
  AVL_uint32 r = 0;

/* If OEM data is implemented, customer needs to use OEM data structure related operation 
   Following code should be used as a reference. 
   For more information refer to sections 2.5 & 2.6 of MxL608_mxLWare_API_UserGuide document.

  UINT8_608 i2cSlaveAddr;
  UINT8_608 i2c_bus;
  user_data_t * user_data = (user_data_t *) MxL608_OEM_DataPtr[devId];
 
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
/*
  devId = devId;
  RegAddr = RegAddr;
  RegData = RegData;
  
  return status;
  */
  UINT8_608 i2c_addr;
  AVL_uchar buffer[2] = {0};
  AVL_uint16 size = 0;

  i2c_addr = devId;//write
  buffer[0] = RegAddr;
  buffer[1] = RegData;
  size = 2;
  //if (pAVLChipForMxL603->m_RepeaterInfo[0].m_uiRepeaterMode == MESSAGE_TYPE)
  //{
	 // r = AVL6381_II2C_Repeater_WriteData(i2c_addr, buffer, size, pAVLChipForMxL603);
  //}
  //else
  //{
  //  if (global::bHeliodorRepeater)
  //{
	 // r = AVL63X1_II2C_Repeater_WriteData(i2c_addr, buffer, size,&g_Demod);
  //}
  //else
  //{
	  r = AVL_IBSP_I2C_Write(i2c_addr, buffer, &size);
 // }
  //r = AVL6381_TunerWrite(i2c_addr, buffer, size);
  //}
  //if (r != 0)
  //{
	 // return MXL603_FALSE;
  //}

  return MXL608_TRUE;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_OEM_ReadRegister
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

MXL608_STATUS MxLWare608_OEM_ReadRegister(UINT8_608 devId, UINT8_608 RegAddr, UINT8_608 *DataPtr)
{
  // OEM should implement I2C read protocol that complies with MxL608 I2C
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

  MXL608_STATUS status = MXL608_TRUE;
  AVL_uint32 r = 0;

/* If OEM data is implemented, customer needs to use OEM data structure related operation 
   Following code should be used as a reference. 
   For more information refer to sections 2.5 & 2.6 of MxL608_mxLWare_API_UserGuide document.

  UINT8_608 i2cSlaveAddr;
  UINT8_608 i2c_bus;
  user_data_t * user_data = (user_data_t *) MxL608_OEM_DataPtr[devId];
 
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
/*
  devId = devId;
  RegAddr = RegAddr;
  *DataPtr = *DataPtr;
  
  return status;
  */
  AVL_uchar buffer[3] = {0};
  UINT8_608 i2c_addr;
  AVL_uint16 size = 0;

  i2c_addr = devId + 0x00;//write
  buffer[0] = 0xFB;
  buffer[1] = RegAddr;
  //if (pAVLChipForMxL603->m_RepeaterInfo[0].m_uiRepeaterMode == MESSAGE_TYPE)
  //{
	 // size = 2;
	 // r = AVL6381_II2C_Repeater_WriteData(i2c_addr, buffer, size, pAVLChipForMxL603);
	 // if(r != 0)
	 // {
		//  return MXL603_FALSE;
	 // }

	 // i2c_addr = devId;
	 // size = 1;
	 // r = AVL6381_II2C_Repeater_ReadData(i2c_addr, DataPtr, size,pAVLChipForMxL603);
  //}
  //else
  //{
  //r = AVL6381_TunerRead(i2c_addr, buffer, 2, DataPtr, 1);
  //  if (global::bHeliodorRepeater)
  //{
	 // size = 2;
	 // r = AVL63X1_II2C_Repeater_WriteData(i2c_addr,buffer,size,&g_Demod);
	 // size = 1;
	 // r |= AVL63X1_II2C_Repeater_ReadData(i2c_addr,DataPtr,size,&g_Demod);
  //}
  //else
  //{
	  size = 2;
	  r = AVL_IBSP_I2C_Write(i2c_addr,buffer,&size);
	  size = 1;
	  r |= AVL_IBSP_I2C_Read(i2c_addr,DataPtr,&size);
 // }
  //}

  //if (r != 0)
  //{
	 // return MXL603_FALSE;
  //}

  return MXL608_TRUE;
}

/*------------------------------------------------------------------------------
--| FUNCTION NAME : MxLWare608_OEM_Sleep
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

void MxLWare608_OEM_Sleep(UINT16_608 DelayTimeInMs)
{
  // OEM should implement sleep operation 
  //DelayTimeInMs = DelayTimeInMs;
  AVL_IBSP_Delay(DelayTimeInMs);
}
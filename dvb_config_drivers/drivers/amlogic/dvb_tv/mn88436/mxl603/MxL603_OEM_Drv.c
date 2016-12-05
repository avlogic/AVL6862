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

#include "MxL603_OEM_Drv.h"
#include "MN_TCB.h" 
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
DMD_u32_t DMD_I2C_Read(DMD_u8_t slvadr , DMD_u8_t adr , DMD_u8_t *data );
DMD_u32_t DMD_I2C_Write(DMD_u8_t	slvadr , DMD_u8_t adr , DMD_u8_t data );

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

  devId = devId;
  RegAddr = RegAddr;
  RegData = RegData;

  //printf(" Tuner write in , devid[%x], regaddr[%x], data[%x] \n", devId, RegAddr,RegData);

  //status = DMD_TCB_WriteRead(devId, RegAddr, &RegData, 1, 0, 0);
  
  status = DMD_I2C_Write(devId,RegAddr,RegData);

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

  devId = devId;
  RegAddr = RegAddr;
  *DataPtr = *DataPtr;

 
//  printk(" Tuner read  in , devid[%x], regaddr[%x] ,data[%x]\n", devId, RegAddr,*DataPtr);

   //status = DMD_TCB_WriteRead(devId, 0xFB, &RegAddr, 1, 0, 0);
   //status =  DMD_TCB_WriteRead(devId, 0, 0, 0, DataPtr, 1);

  status = DMD_I2C_Read(devId,RegAddr,DataPtr);


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

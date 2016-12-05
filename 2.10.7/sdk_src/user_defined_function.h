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



#ifndef USER_DEFINED_FUNCTION_H
#define USER_DEFINED_FUNCTION_H

#include "user_defined_data_type.h"

#ifdef AVL_CPLUSPLUS
extern "C" {
#endif

// For this user defined function, Availink has a different implementation and the parameter list isn't 'avoid'.
#if defined(AVL_I2C_DEFINE)
AVL_uint32 AVL_IBSP_Initialize(System::String^ strServerName0, int iSocketPort0,System::String^ strServerName1, int iSocketPort1);
#else
AVL_uint32 AVL6381_IBSP_Initialize(void);
#endif

#if defined(AVL_INTERNAL)
AVL_uint32 AVL_IBSP_Reset(unsigned char SocketTag);
AVL_uint32 AVL_IBSP_SetPort( unsigned char SocketIndex);
#else
AVL_uint32 AVL_IBSP_Reset();
#endif

AVL_uint32 AVL_IBSP_Delay(AVL_uint32 uiDelay_ms);
AVL_uint32 AVL_IBSP_I2C_Read(AVL_uint16 uiSlaveAddr,  AVL_puchar pucBuff, AVL_puint16 puiSize);
AVL_uint32 AVL_IBSP_I2C_Write(AVL_uint16 uiSlaveAddr,  AVL_puchar pucBuff, AVL_puint16 puiSize);
AVL_uint32 AVL_IBSP_Dispose(void);
AVL_uint32 AVL_IBSP_InitSemaphore(AVL_psemaphore pSemaphore);
AVL_uint32 AVL_IBSP_ReleaseSemaphore(AVL_psemaphore pSemaphore);
AVL_uint32 AVL_IBSP_WaitSemaphore(AVL_psemaphore pSemaphore);
AVL_uint32 AVL_SERIAL_Initialize(bool valid);
void AVL_SERIAL_ByPassOn();
void AVL_SERIAL_ByPassOff();
#ifdef AVL_CPLUSPLUS
}
#endif

#endif


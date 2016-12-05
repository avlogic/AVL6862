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



#include "user_defined_function.h"

AVL_uint32 AVL_IBSP_Reset(void)
{
    return(0);
}

AVL_uint32 AVL_IBSP_Delay(AVL_uint32 uiDelay_ms)
{
    return(0);
}

AVL_uint32 AVL_IBSP_I2C_Read(AVL_uint16 usSlaveAddr,  AVL_puchar pucBuff, AVL_puint16 pusSize)
{
    return(0);
}

AVL_uint32 AVL_IBSP_I2C_Write(AVL_uint16 usSlaveAddr,  AVL_puchar pucBuff,  AVL_puint16  pusSize)
{
    return(0);
}

AVL_uint32 AVL_IBSP_Initialize(void)
{
    return(0);
}

AVL_uint32 AVL_IBSP_InitSemaphore(AVL_psemaphore pSemaphore)
{
    return(0);
}

AVL_uint32 AVL_IBSP_ReleaseSemaphore(AVL_psemaphore pSemaphore)
{
    return(0);
}

AVL_uint32 AVL_IBSP_WaitSemaphore(AVL_psemaphore pSemaphore)
{
    return(0);
}

AVL_uint32 AVL_IBSP_Dispose(void)
{
    return(0);
}



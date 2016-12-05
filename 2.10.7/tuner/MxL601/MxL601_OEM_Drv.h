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


/*******************************************************************************
 *
 * FILE NAME          : MxL601_OEM_Drv.h
 * 
 * AUTHOR             : Dong Liu 
 *
 * DATE CREATED       : 11/23/2011
 *
 * DESCRIPTION        : Header file for MxL601_OEM_Drv.c
 *
 *******************************************************************************
 *                Copyright (c) 2010, MaxLinear, Inc.
 ******************************************************************************/

#ifndef __MxL601_OEM_DRV_H__
#define __MxL601_OEM_DRV_H__

/******************************************************************************
    Include Header Files
    (No absolute paths - paths handled by make file)
******************************************************************************/

#include "MaxLinearDataTypes.h"
#include "MxL_Debug.h"

/******************************************************************************
    Macros
******************************************************************************/

/******************************************************************************
    User-Defined Types (Typedefs)
******************************************************************************/

/******************************************************************************
    Global Variable Declarations
******************************************************************************/

/******************************************************************************
    Prototypes
******************************************************************************/

MXL_STATUS Ctrl_WriteRegister(UINT8 I2cSlaveAddr, UINT8 RegAddr, UINT8 RegData);
MXL_STATUS Ctrl_ReadRegister(UINT8 I2cSlaveAddr, UINT8 RegAddr, UINT8 *DataPtr);
void MxL_Sleep(UINT16 DelayTimeInMs);  

#endif /* __MxL601_OEM_DRV_H__*/





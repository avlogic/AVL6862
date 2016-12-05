/*******************************************************************************
 *
 * FILE NAME          : MaxLinearDataTypes.h
 * 
 * AUTHOR             : Brenndon Lee
 *                      Dong Liu 
 *
 * DATE CREATED       : Jul/31, 2006
 *                      Jan/23, 2010
 *
 * DESCRIPTION        : This file contains MaxLinear-defined data types.
 *                      Instead of using ANSI C data type directly in source code
 *                      All module should include this header file.
 *                      And conditional compilation switch is also declared
 *                      here.
 *
 *******************************************************************************
 *                Copyright (c) 2010, MaxLinear, Inc.
 ******************************************************************************/

#ifndef __MxL608_MAXLINEAR_DATA_TYPES_H__
#define __MxL608_MAXLINEAR_DATA_TYPES_H__

/******************************************************************************
    Include Header Files
    (No absolute paths - paths handled by make file)
******************************************************************************/

/******************************************************************************
    Macros
******************************************************************************/

#ifndef _POSIX_SOURCE 
#define _POSIX_SOURCE 
#endif

#ifdef _ANSI_C_SOURCE 
#define false                1
#define true                 0

#define TRUE                 1
#define FALSE                0
#endif 
/******************************************************************************
    User-Defined Types (Typedefs)
******************************************************************************/
typedef unsigned char        UINT8_608;
typedef unsigned short       UINT16_608;
typedef unsigned int         UINT32_608;
typedef unsigned long long   UINT64_608;
typedef char                 SINT8;
typedef short                SINT16;
typedef int                  SINT32;
typedef float                REAL32;
typedef double               REAL64;
typedef unsigned long        ULONG_32;

#ifdef _ANSI_C_SOURCE
typedef unsigned char        bool;
#endif 

typedef enum 
{
  MXL608_TRUE = 0,
  MXL608_FALSE = 1,
  MXL608_SUCCESS = 0,
  MXL608_FAILED,
  MXL608_BUSY,
  MXL608_NULL_PTR,
  MXL608_INVALID_PARAMETER,
  MXL608_NOT_INITIALIZED,
  MXL608_ALREADY_INITIALIZED,
  MXL608_BUFFER_TOO_SMALL,
  MXL608_NOT_SUPPORTED,
  MXL608_TIMEOUT
} MXL608_STATUS;

typedef enum{
	MxL608_OK					        =  0x0,
	MxL608_ERR_INIT			      =  0x1,
	MxL608_ERR_RFTUNE			    =  0x2,
	MxL608_ERR_SET_REG			    =  0x3,
	MxL608_ERR_GET_REG			    =  0x4,
	MxL608_ERR_MODE			      =  0x10,
	MxL608_ERR_IF_FREQ			    =  0x11,
	MxL608_ERR_XTAL_FREQ		    =  0x12,
	MxL608_ERR_BANDWIDTH		    =  0x13,
	MxL608_GET_ID_FAIL			    =  0x14,
	MxL608_ERR_DEMOD_LOCK		  =  0x20,
	MxL608_NOREADY_DEMOD_LOCK	=  0x21,
	MxL608_ERR_OTHERS			    =  0x0A
}MxL608_ERR_MSG;

typedef enum
{
  MXL608_DISABLE = 0,
  MXL608_ENABLE,

  MXL608_UNLOCKED = 0,
  MXL608_LOCKED,

  MXL608_INVALID = 0, 
  MXL608_VALID,      

  MXL608_PORT_LOW = 0,
  MXL608_PORT_HIGH,

  MXL608_START = 0,
  MXL608_FINISH,

  MXL608_ABORT_TUNE = 0,
  MXL608_START_TUNE,

  MXL608_FINE_TUNE_STEP_DOWN = 0,
  MXL608_FINE_TUNE_STEP_UP

} MXL608_BOOL;

typedef enum 
{
  IFX608_SUCCESS = 0,
  IFX608_FAILED,
  IFX608_BUSY,
  IFX608_NULL_PTR,
  IFX608_INVALID_PARAMETER,
  IFX608_BUFFER_TOO_SMALL,
  IFX608_TIMEOUT  
} IFX608_STATUS;

/******************************************************************************
    Global Variable Declarations
******************************************************************************/

/******************************************************************************
    Prototypes
******************************************************************************/

#endif /* __MAXLINEAR_DATA_TYPES_H__ */


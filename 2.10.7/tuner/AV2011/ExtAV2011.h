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


#ifndef AV2011_H
#define AV2011_H

#include "AVL_Tuner.h"

#ifdef AVL_CPLUSPLUS
extern "C" {
#endif

typedef unsigned char        UINT8;
typedef unsigned short       UINT16;
typedef unsigned int         UINT32;
typedef unsigned long long   UINT64;
typedef char                 SINT8;
typedef short                SINT16;
typedef int                  SINT32;
typedef float                REAL32;
typedef double               REAL64;
typedef unsigned long        ULONG_32;

#define AV2011_ENABLE_TIMEOUT
#define AV2011_TIMEOUT 100

    /// AV2011_Setting: Structure for AV2011 special setting
typedef struct AV2011_Setting
{
    AVL_uchar AutoGain;
    AVL_uchar Single_End;
    AVL_uchar Auto_Scan;
    AVL_uchar RF_Loop;
}AV2011_Setting;

typedef enum AutoGain
{
    AV2011_AutoGain_OFF = 0, //FT_EN 0, fix LNA gain 
    AV2011_AutoGain_ON = 1 //FT_EN 0, LNA gain Auto control
}AutoGain;

typedef enum Single_End
{
    AV2011_Differential = 0,  //IQ Differential mode
    AV2011_SingleEnd = 1    //IQ Single end mode
}Single_End;

typedef enum Auto_Scan
{
    Auto_Scan_OFF = 0,  //normal manual lock mode
    Auto_Scan_ON = 1    //blind scan search mode
}Auto_Scan;

typedef enum RF_Loop
{
    RF_Loop_OFF = 0,  //0 = RF loop through off
    RF_Loop_ON = 1 //RF loop through on
}RF_Loop;

AVL_uint32 ExtAV2011_Initialize(struct AVL_Tuner * pTuner);
AVL_uint32 ExtAV2011_GetLockStatus(struct AVL_Tuner * pTuner);
AVL_uint32 ExtAV2011_Lock(struct AVL_Tuner *pTuner);

#ifdef AVL_CPLUSPLUS
}
#endif

#endif


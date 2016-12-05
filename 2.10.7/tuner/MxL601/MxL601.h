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



#ifndef MXL601_H
#define MXL601_H

#include "AVL_Tuner.h"
    #ifdef AVL_CPLUSPLUS
extern "C" {
    #endif

AVL_uint32 MxL601_Initialize(AVL_Tuner * pTuner);
AVL_uint32 MxL601_Lock(AVL_Tuner *pTuner);
AVL_uint32 MxL601_GetLockStatus(AVL_Tuner * pTuner);
AVL_uint32 MxL601_GetRFStrength(AVL_Tuner * pTuner, AVL_int32 *rf_strength);
    
#ifdef AVL_CPLUSPLUS
}
#endif

#endif


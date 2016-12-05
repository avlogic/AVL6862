#ifndef _NMI120_DEFINE_H
#define _NMI120_DEFINE_H
#include "nmitypes.h"

typedef unsigned int MS_U32; 
typedef signed int MS_S32;
typedef unsigned char MS_U8;


uint8_t MDrv_Tuner_Init(void);
MS_U32 MDrv_Tuner_SetTuner(MS_U32 dwFreq, MS_U8 ucBw);
int NMI120_GetLockStatus(void);
int NMI120_GetRSSI(uint8_t outputChoice);
uint8_t MDrv_Tuner_PowerOnOff(uint8_t bPowerOn);
uint8_t NMI120_LoopThrough(void);

//uint8_t MDrv_NMI120_Tuner_SetTuner(uint32_t dwFreq, uint8_t ucBw,uint8_t type);//MHZ//MHZ
#endif



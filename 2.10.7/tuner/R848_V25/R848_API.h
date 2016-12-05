
#ifndef R848_API_H
#define R848_API_H

#include "AVL_Tuner.h"
#include "R848.h"

#ifdef AVL_CPLUSPLUS
extern "C" {
#endif

	AVL_uint32  R848_Initialize(AVL_Tuner * pTuner );
	AVL_uint32  R848_Lock(AVL_Tuner * pTuner );
	AVL_uint32  R848_GetLockStatus(AVL_Tuner * pTuner);
	AVL_uint32  R848_GetGain(AVL_Tuner *pTuner, AVL_uint32 *puiRFGain);
	AVL_int32   R848_RFPowerCalc(AVL_uint32 uiRFGain,AVL_uint16 usSSI);
	AVL_uint32  R848_ReadReg();

#ifdef AVL_CPLUSPLUS
}
#endif

#endif

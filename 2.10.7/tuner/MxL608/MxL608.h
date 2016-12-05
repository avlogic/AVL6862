#ifndef MxL608_H
#define MxL608_H

	#include "MxL608_TunerApi.h"
	#include "MxL608_OEM_Drv.h"
	#include "AVL_Tuner.h"

	#ifdef AVL_CPLUSPLUS
extern "C" {
	#endif

		AVL_uint32 MxL608_Initialize(AVL_Tuner *pTuner);
		AVL_uint32 MxL608_GetLockStatus(AVL_Tuner *pTuner);
		AVL_uint32 MxL608_Lock(AVL_Tuner *pTuner);
		AVL_uint32 MxL608_GetRFStrength(AVL_Tuner *pTuner, AVL_int32 *power);

	#ifdef AVL_CPLUSPLUS
}
	#endif

#endif

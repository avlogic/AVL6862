/******************************************************************************
**
**	Copyright (c) Newport Media Inc.  All rights reserved.
**
** 	Module Name:  nmiiioctl.h
**	
** 
*******************************************************************************/

#ifndef _NMIIOCTL_PLTFRM_H_
#define _NMIIOCTL_PLTFRM_H_

#include "nmitypes.h"
#include "nmicmndefs.h"
#include "nm131.h"

/******************************************************************************
**
**	Includes common IOCTLs.
**
*******************************************************************************/
#include "nmiioctl_cmn.h"
 

extern int32_t nmi_drv_ctl(uint32_t ,void*);

#endif

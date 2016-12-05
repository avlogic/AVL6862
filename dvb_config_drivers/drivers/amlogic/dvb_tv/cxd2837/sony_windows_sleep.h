/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2012-11-01 10:44:56 #$
  File Revision : $Revision:: 6345 $
------------------------------------------------------------------------------*/
/**
 @file    sony_windows_sleep.h

          This file provides the Windows specific sleep function.
*/
/*----------------------------------------------------------------------------*/

#ifndef SONY_WINDOWS_SLEEP_H
#define SONY_WINDOWS_SLEEP_H

/*------------------------------------------------------------------------------
 Function Prototypes
------------------------------------------------------------------------------*/
/**
 @brief High precision sleep function for Windows PC.
        This function ensures that sleeping time is not shorter than specified time.

 @param sleepTimeMs Sleep time in ms.

 @return 1 if successful. 0 if failed.
*/
int sony_windows_Sleep (unsigned long sleepTimeMs);

/**
 @brief This function improves timer accuracy by setting the timer resolution to the 
        minimum supported by the Windows PC.  This function should be called at the 
        start of the application.  If this function is used, 
        ::sony_windows_RestoreTimeResolution should be called at the end of the 
        application.

 @param void.

 @return 1 if successful. 0 if failed.
*/
int sony_windows_ImproveTimeResolution (void);

/**
 @brief This function restores the time resolution setting configured
        by ::sony_windows_ImproveTimeResolution.

 @param void.

 @return 1 if successful. 0 if failed.
*/
int sony_windows_RestoreTimeResolution (void);

#endif /* SONY_WINDOWS_SLEEP_H */

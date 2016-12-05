/*------------------------------------------------------------------------------

 <dev:header>
    Copyright(c) 2011 Sony Corporation.

    $Revision: 2675 $
    $Author: mrushton $

</dev:header>

------------------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#include "sony_common.h"

#ifndef _WINDOWS
sony_result_t sony_stopwatch_start (sony_stopwatch_t * pStopwatch)
{
//#error sony_stopwatch_start is not implemented
//SONY_TRACE_ENTER("sony_stopwatch_start");

   if (!pStopwatch) {
	   SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
   }
   pStopwatch->startTime = jiffies;
   //unsigned long delay = jiffies + 5000*100; //5*HZ = 5Ãë
    ///while(delay > jiffies);
   SONY_TRACE_RETURN(SONY_RESULT_OK);

}

sony_result_t sony_stopwatch_sleep (sony_stopwatch_t * pStopwatch, uint32_t ms)
{
//#error sony_stopwatch_sleep is not implemented
if (!pStopwatch) {
	   SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
   }
   SONY_ARG_UNUSED(*pStopwatch);
   msleep (ms);

   SONY_TRACE_RETURN(SONY_RESULT_OK);

}

sony_result_t sony_stopwatch_elapsed (sony_stopwatch_t * pStopwatch, uint32_t* pElapsed)
{
//#error sony_stopwatch_elapsed is not implemented
	if (!pStopwatch || !pElapsed) {
		   SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
	   }
	   *pElapsed = jiffies - pStopwatch->startTime;
	
	   SONY_TRACE_RETURN(SONY_RESULT_OK);

    return 0;
}
#else

#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

sony_result_t sony_stopwatch_start (sony_stopwatch_t * pStopwatch)
{
    SONY_TRACE_ENTER("sony_stopwatch_start");
    if (!pStopwatch) {
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }
    pStopwatch->startTime = timeGetTime ();

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_stopwatch_sleep (sony_stopwatch_t * pStopwatch, uint32_t ms)
{
    SONY_TRACE_ENTER("sony_stopwatch_sleep");
    if (!pStopwatch) {
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }
    SONY_ARG_UNUSED(*pStopwatch);
    SONY_SLEEP (ms);

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_stopwatch_elapsed (sony_stopwatch_t * pStopwatch, uint32_t* pElapsed)
{
    SONY_TRACE_ENTER("sony_stopwatch_elapsed");

    if (!pStopwatch || !pElapsed) {
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }
    *pElapsed = timeGetTime () - pStopwatch->startTime;

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}
#endif

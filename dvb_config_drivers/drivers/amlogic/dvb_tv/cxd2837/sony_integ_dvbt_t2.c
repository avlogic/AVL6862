/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2013-03-22 13:51:01 #$
  File Revision : $Revision:: 6790 $
------------------------------------------------------------------------------*/

#include "sony_integ.h"
#include "sony_integ_dvbt_t2.h"
#include "sony_demod.h"
#include "sony_demod_dvbt_monitor.h"
#include "sony_demod_dvbt2_monitor.h"

#ifndef SONY_INTEG_DISABLE_ASCOT_TUNER
#include "sony_tuner_ascot2e.h"
#include "sony_tuner_ascot3.h"
#endif
#include "mxl603/MaxLinearDataTypes.h"
#include "mxl603/MxL603_TunerApi.h"
/*------------------------------------------------------------------------------
 Static Function Prototypes
------------------------------------------------------------------------------*/
/**
 @brief Waits for demodulator lock, polling ::sony_demod_dvbt_monitor_SyncStat 
        at 10ms intervals.  Called as part of the Tune process.
*/
static sony_result_t dvbt_WaitDemodLock (sony_integ_t * pInteg);

/**
 @brief Waits for demodulator lock, polling ::sony_demod_dvbt2_monitor_SyncStat 
        at 10ms intervals.  Called as part of the Tune process.
*/
static sony_result_t dvbt2_WaitDemodLock (sony_integ_t * pInteg, sony_dvbt2_profile_t profile);

/**
 @brief Waits for L1 Post to be valid to ensure that subsequent calls to 
        L1 based monitors do not return HW State error.  Polls
        ::sony_demod_dvbt2_CheckL1PostValid at 10ms intervals.  Called as 
        part of the Tune, BlindTune and Scan processes.
*/
static sony_result_t dvbt2_WaitL1PostLock (sony_integ_t * pInteg);
/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_integ_dvbt_Tune(sony_integ_t * pInteg,
                                   sony_dvbt_tune_param_t * pTuneParam)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_dvbt_Tune");

    if ((!pInteg) || (!pTuneParam) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if ((pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C)) {
        /* This api is accepted in SLEEP_T_C and ACTIVE_T_C states only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Clear cancellation flag. */
    sony_atomic_set (&(pInteg->cancel), 0);

    /* Check bandwidth validity */
    if ((pTuneParam->bandwidth != SONY_DEMOD_BW_5_MHZ) && (pTuneParam->bandwidth != SONY_DEMOD_BW_6_MHZ) && 
        (pTuneParam->bandwidth != SONY_DEMOD_BW_7_MHZ) && (pTuneParam->bandwidth != SONY_DEMOD_BW_8_MHZ)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    /* Set DVB-T profile for acquisition */
    result = sony_demod_dvbt_SetProfile(pInteg->pDemod, pTuneParam->profile);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Tune the demodulator */
    result = sony_demod_dvbt_Tune (pInteg->pDemod, pTuneParam);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

#ifndef CONFIG_TH_CXD2837_TUNER_MXL603
    if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Tune))
#endif
   {
#ifndef SONY_DISABLE_I2C_REPEATER
        /* Enable the I2C repeater */
        result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
#endif

	#ifdef CONFIG_TH_CXD2837_TUNER_MXL603
		result = Mxl603SetFreqBw(pTuneParam->bandwidth,pTuneParam->centerFreqKHz);
	#else
        /* Tune the RF part */
        result = pInteg->pTunerTerrCable->Tune (pInteg->pTunerTerrCable, pTuneParam->centerFreqKHz, SONY_DTV_SYSTEM_DVBT, pTuneParam->bandwidth);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
	#endif

#ifndef SONY_DISABLE_I2C_REPEATER
        /* Disable the I2C repeater */
        result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
#endif
    }

    /* Reset the demod to enable acquisition */
    result = sony_demod_TuneEnd (pInteg->pDemod);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }    

    /* Wait for TS lock */
    result = sony_integ_dvbt_WaitTSLock (pInteg);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }    

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_dvbt2_Tune(sony_integ_t * pInteg,
                                    sony_dvbt2_tune_param_t * pTuneParam)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_dvbt2_Tune");

    if ((!pInteg) || (!pTuneParam) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }
    
    if ((pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C)) {
        /* This api is accepted in SLEEP_T_C and ACTIVE_T_C states only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Clear cancellation flag. */
    sony_atomic_set (&(pInteg->cancel), 0);

    /* Check bandwidth validity */
    if ((pTuneParam->bandwidth != SONY_DEMOD_BW_1_7_MHZ) && (pTuneParam->bandwidth != SONY_DEMOD_BW_5_MHZ) && 
        (pTuneParam->bandwidth != SONY_DEMOD_BW_6_MHZ) && (pTuneParam->bandwidth != SONY_DEMOD_BW_7_MHZ) && 
        (pTuneParam->bandwidth != SONY_DEMOD_BW_8_MHZ)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }
    
    /* Check for valid profile selection */
    if ((pTuneParam->profile != SONY_DVBT2_PROFILE_BASE) && (pTuneParam->profile != SONY_DVBT2_PROFILE_LITE)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Configure for manual PLP selection. */
    result = sony_demod_dvbt2_SetPLPConfig (pInteg->pDemod, 0x00, pTuneParam->dataPlpId);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Configure the DVB-T2 profile without recovery */
    result = sony_demod_dvbt2_SetProfile(pInteg->pDemod, pTuneParam->profile);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Tune the demodulator */
    result = sony_demod_dvbt2_Tune (pInteg->pDemod, pTuneParam);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    #ifndef CONFIG_TH_CXD2837_TUNER_MXL603
    if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Tune))
    #endif
    {
#ifndef SONY_DISABLE_I2C_REPEATER
        /* Enable the I2C repeater */
        result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
#endif

	#ifdef CONFIG_TH_CXD2837_TUNER_MXL603
		result = Mxl603SetFreqBw(pTuneParam->bandwidth,pTuneParam->centerFreqKHz);
	#else
        /* Tune the RF part */
        result = pInteg->pTunerTerrCable->Tune (pInteg->pTunerTerrCable, pTuneParam->centerFreqKHz, SONY_DTV_SYSTEM_DVBT2, pTuneParam->bandwidth);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
	#endif

#ifndef SONY_DISABLE_I2C_REPEATER
        /* Disable the I2C repeater */
        result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
#endif
    }

    /* Reset the demod to enable acquisition */
    result = sony_demod_TuneEnd (pInteg->pDemod);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }    

    /* Wait for demodulator lock */
    result = dvbt2_WaitDemodLock (pInteg, pTuneParam->profile);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }    

    /* Wait for TS lock */
    result = sony_integ_dvbt2_WaitTSLock (pInteg, pTuneParam->profile);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }    
    
    /* In DVB-T2, L1 Post information may not immediately be valid after acquisition
     * (L1POST_OK bit != 1).  This wait loop handles such cases.  This issue occurs 
     * only under clean signal lab conditions, and will therefore not extend acquistion 
     * time under normal conditions.
     */ 
    result = dvbt2_WaitL1PostLock (pInteg);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }    

    /* Confirm correct PLP selection in acquisition */
    {
        uint8_t plpNotFound;
        
        result = sony_demod_dvbt2_monitor_DataPLPError (pInteg->pDemod, &plpNotFound);
        if (result == SONY_RESULT_ERROR_HW_STATE) {
            /* Demod lock is lost causing monitor to fail, return UNLOCK instead of HW STATE */
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_UNLOCK);
        }
        else if (result != SONY_RESULT_OK) {
            /* Serious error, so return result */
            SONY_TRACE_RETURN (result);
        }   

        if (plpNotFound) {
            result = SONY_RESULT_OK_CONFIRM;
            pTuneParam->tuneInfo = SONY_DEMOD_DVBT2_TUNE_INFO_INVALID_PLP_ID;
        }  
        else {
            pTuneParam->tuneInfo = SONY_DEMOD_DVBT2_TUNE_INFO_OK;
        }
    }

    SONY_TRACE_RETURN (result);
}


sony_result_t sony_integ_dvbt_t2_Scan(sony_integ_t * pInteg,
                                      sony_integ_dvbt_t2_scan_param_t * pScanParam,
                                      sony_integ_dvbt_t2_scan_callback_t callBack)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_integ_dvbt_t2_scan_result_t scanResult;

    SONY_TRACE_ENTER ("sony_integ_dvbt_t2_Scan");

    if ((!pInteg) || (!pScanParam) || (!callBack) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if ((pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C)) {
        /* This api is accepted in SLEEP_T_C and ACTIVE_T_C states only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Clear cancellation flag. */
    sony_atomic_set (&(pInteg->cancel), 0);

    /* Ensure the scan parameters are valid. */
    if (pScanParam->endFrequencyKHz < pScanParam->startFrequencyKHz) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pScanParam->stepFrequencyKHz == 0) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Check bandwidth validity */
    if ((pScanParam->bandwidth != SONY_DEMOD_BW_1_7_MHZ) && (pScanParam->bandwidth != SONY_DEMOD_BW_5_MHZ) && 
        (pScanParam->bandwidth != SONY_DEMOD_BW_6_MHZ) && (pScanParam->bandwidth != SONY_DEMOD_BW_7_MHZ) && 
        (pScanParam->bandwidth != SONY_DEMOD_BW_8_MHZ)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    /* Check for invalid combination of 1.7MHz scanning with DVB-T */
    if ((pScanParam->system == SONY_DTV_SYSTEM_DVBT) && (pScanParam->bandwidth == SONY_DEMOD_BW_1_7_MHZ)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    /* Set the start frequency */
    scanResult.centerFreqKHz = pScanParam->startFrequencyKHz;

    /* Set scan mode enabled */
    sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x01);

    /* Scan routine */
    while (scanResult.centerFreqKHz <= pScanParam->endFrequencyKHz) {
        sony_dtv_system_t systemFound = SONY_DTV_SYSTEM_UNKNOWN;
        sony_dvbt2_profile_t profileFound = SONY_DVBT2_PROFILE_ANY;
        uint8_t channelComplete = 0;
        sony_dtv_system_t blindTuneSystem = pScanParam->system;
        sony_dvbt2_profile_t blindTuneProfile = pScanParam->t2Profile;
        
        while (!channelComplete) {
            scanResult.tuneResult = sony_integ_dvbt_t2_BlindTune(pInteg, scanResult.centerFreqKHz, pScanParam->bandwidth, blindTuneSystem, blindTuneProfile, &systemFound, &profileFound);
            switch (scanResult.tuneResult) {
            case SONY_RESULT_OK:
                scanResult.system = systemFound;
            
                /* Channel found, callback to application */
                if (systemFound == SONY_DTV_SYSTEM_DVBT){
                    scanResult.dvbtTuneParam.centerFreqKHz = scanResult.centerFreqKHz;
                    scanResult.dvbtTuneParam.bandwidth = pScanParam->bandwidth;
                    scanResult.dvbtTuneParam.profile = SONY_DVBT_PROFILE_HP;
                    callBack (pInteg, &scanResult, pScanParam);

                    /* DVB-T detected, set channel complete to move to next frequency */
                    channelComplete = 1;
                }
                else if (systemFound == SONY_DTV_SYSTEM_DVBT2) {
                    uint8_t numPLPs = 0;
                    uint8_t PLPIds[255];
                    uint8_t i;

                    scanResult.dvbt2TuneParam.centerFreqKHz = scanResult.centerFreqKHz;
                    scanResult.dvbt2TuneParam.bandwidth = pScanParam->bandwidth;
                    scanResult.dvbt2TuneParam.tuneInfo = SONY_DEMOD_DVBT2_TUNE_INFO_OK;
                    scanResult.dvbt2TuneParam.profile = profileFound;

                    /* Obtain the T2 PLP list from Data PLP monitor */
                    result = sony_demod_dvbt2_monitor_DataPLPs (pInteg->pDemod, PLPIds, &numPLPs);
                    if (result == SONY_RESULT_ERROR_HW_STATE) {
                        /* Callback to application for progress updates */
                        scanResult.system = SONY_DTV_SYSTEM_UNKNOWN;
                        scanResult.tuneResult = SONY_RESULT_ERROR_UNLOCK;
                        callBack (pInteg, &scanResult, pScanParam);

                        /* Error in monitored data, ignore current channel */
                        channelComplete = 1;
                    }
                    else if (result != SONY_RESULT_OK) {
                        /* Serious error occurred -> cancel operation. */
                        sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
                        SONY_TRACE_RETURN (result);
                    }
                    else {
                        /* Check for mixed profile channels */
                        sony_dvbt2_ofdm_t ofdm;
                        result = sony_demod_dvbt2_monitor_Ofdm (pInteg->pDemod, &ofdm);
                        if (result == SONY_RESULT_ERROR_HW_STATE) {
                            /* Callback to application for progress updates */
                            scanResult.system = SONY_DTV_SYSTEM_UNKNOWN;
                            scanResult.tuneResult = SONY_RESULT_ERROR_UNLOCK;
                            callBack (pInteg, &scanResult, pScanParam);

                            /* Error in monitor, move onto next channel */
                            channelComplete = 1;
                        }
                        else if (result != SONY_RESULT_OK) {
                            /* Serious error occurred -> cancel operation. */
                            sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
                            SONY_TRACE_RETURN (result);
                        }
                        else {
                            /* Set PLP ID in tune parameter structure and provide callback for first PLP */
                            scanResult.dvbt2TuneParam.dataPlpId = PLPIds[0];
                            callBack (pInteg, &scanResult, pScanParam);

                            /* Callback for each subsequent PLP in current profile */
                            for (i = 1; i < numPLPs; i++) {
                                uint16_t plpAcquisitionTime = 0;
                                sony_stopwatch_t timer;
                                
                                result = sony_demod_dvbt2_SetPLPConfig (pInteg->pDemod, 0x00, PLPIds[i]);
                                if (result != SONY_RESULT_OK) {
                                    /* Serious error occurred -> cancel operation. */
                                    sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
                                    SONY_TRACE_RETURN (result);
                                }

                                if ((profileFound == SONY_DVBT2_PROFILE_BASE) && (ofdm.mixed)) {
                                    plpAcquisitionTime = 510;
                                }
                                else if ((profileFound == SONY_DVBT2_PROFILE_LITE) && (ofdm.mixed)) {
                                    plpAcquisitionTime = 1260;
                                }
                                else {
                                    plpAcquisitionTime = 260;
                                }

                                /* Start stopwatch to measure PLP acquisition time */
                                result = sony_stopwatch_start (&timer);
                                if (result != SONY_RESULT_OK) {
                                    sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
                                    SONY_TRACE_RETURN (result);
                                }

                                for (;;) {
                                    uint32_t elapsed;

                                    /* Check cancellation. */
                                    if (sony_atomic_read (&(pInteg->cancel)) != 0) {
                                        sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
                                        SONY_TRACE_RETURN (SONY_RESULT_ERROR_CANCEL);
                                    }

                                    result = sony_stopwatch_elapsed(&timer, &elapsed);
                                    if (result != SONY_RESULT_OK) {
                                        sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
                                        SONY_TRACE_RETURN (result);
                                    }

                                    if (elapsed >= plpAcquisitionTime) {
                                        break; /* finish waiting */
                                    } else {
                                        result = sony_stopwatch_sleep (&timer, SONY_DVBT_T2_WAIT_LOCK_INTERVAL);
                                        if (result != SONY_RESULT_OK) {
                                            sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
                                            SONY_TRACE_RETURN (result);
                                        }
                                    }
                                }

                                /* Set PLP ID in tune parameter structure and provide callback */
                                scanResult.dvbt2TuneParam.dataPlpId = PLPIds[i];
                                callBack (pInteg, &scanResult, pScanParam);
                            }                                

                            /* If profile is ANY, check for mixed profile channels */
                            if (blindTuneProfile == SONY_DVBT2_PROFILE_ANY) {
                                /* Check for mixed profiles available */
                                if (ofdm.mixed) {
                                    /* Set Blind Tune parameters to DVB-T2 only and the other profile
                                        * compared to that already located */
                                    blindTuneSystem = SONY_DTV_SYSTEM_DVBT2;

                                    if (profileFound == SONY_DVBT2_PROFILE_BASE) {
                                        blindTuneProfile = SONY_DVBT2_PROFILE_LITE;
                                    }
                                    else if (profileFound == SONY_DVBT2_PROFILE_LITE) {
                                        blindTuneProfile = SONY_DVBT2_PROFILE_BASE;
                                    }
                                    else {
                                        /* Serious error occurred -> cancel operation. */
                                        sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
                                        SONY_TRACE_RETURN (result);
                                    }
                                }
                                else {
                                    /* Channel is not mixed profile so continue to next frequency */
                                    channelComplete = 1;
                                }
                            }
                            else {
                                /* Profile is fixed therefore this frequency is complete */
                                channelComplete = 1;
                            }
                        }
                    }
                }
                break;

            /* Intentional fall-through. */
            case SONY_RESULT_ERROR_UNLOCK:
            case SONY_RESULT_ERROR_TIMEOUT:
                /* Channel not found, callback to application for progress updates */
                scanResult.system = SONY_DTV_SYSTEM_UNKNOWN;
                callBack (pInteg, &scanResult, pScanParam);
                
                /* Go to next frequency */
                channelComplete = 1;
                break;

            default:
                {
                    /* Serious error occurred -> cancel operation. */
                    sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
                    SONY_TRACE_RETURN (scanResult.tuneResult);
                }
            }
        }

        scanResult.centerFreqKHz += pScanParam->stepFrequencyKHz;

        /* Check cancellation. */
        if (sony_atomic_read (&(pInteg->cancel)) != 0) {
            sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_CANCEL);
        }
    }

    /* Clear scan mode */
    result = sony_demod_terr_cable_SetScanMode(pInteg->pDemod, pScanParam->system, 0x00);

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_dvbt_t2_BlindTune(sony_integ_t * pInteg,
                                           uint32_t centerFreqKHz,
                                           sony_demod_bandwidth_t bandwidth,
                                           sony_dtv_system_t system,
                                           sony_dvbt2_profile_t profile,
                                           sony_dtv_system_t * pSystemTuned,
                                           sony_dvbt2_profile_t * pProfileTuned)
{
    sony_result_t result = SONY_RESULT_OK;
    uint8_t channelFound = 0;
    sony_dtv_system_t tuneSystems[2] = {SONY_DTV_SYSTEM_UNKNOWN , SONY_DTV_SYSTEM_UNKNOWN};
    uint8_t tuneIteration;

    SONY_TRACE_ENTER ("sony_integ_dvbt_t2_BlindTune");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (!pInteg->pDemod->scanMode) {
        /* Clear cancellation flag. */
        sony_atomic_set (&(pInteg->cancel), 0);
    }

    /* Confirm the demod is in a valid state to accept this API */
    if ((pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Check bandwidth validity */
    if ((bandwidth != SONY_DEMOD_BW_1_7_MHZ) && (bandwidth != SONY_DEMOD_BW_5_MHZ) && 
        (bandwidth != SONY_DEMOD_BW_6_MHZ) && (bandwidth != SONY_DEMOD_BW_7_MHZ) && 
        (bandwidth != SONY_DEMOD_BW_8_MHZ)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    /* Check for invalid system parameter */
    if ((system != SONY_DTV_SYSTEM_DVBT) && (system != SONY_DTV_SYSTEM_DVBT2) && (system != SONY_DTV_SYSTEM_ANY)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    /* Check for invalid combination of 1.7MHz tune */
    if ((system != SONY_DTV_SYSTEM_DVBT2) && (bandwidth == SONY_DEMOD_BW_1_7_MHZ)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    if (system == SONY_DTV_SYSTEM_ANY) {
        tuneSystems[0] = pInteg->pDemod->blindTuneDvbt2First? SONY_DTV_SYSTEM_DVBT2 : SONY_DTV_SYSTEM_DVBT;
        tuneSystems[1] = pInteg->pDemod->blindTuneDvbt2First? SONY_DTV_SYSTEM_DVBT : SONY_DTV_SYSTEM_DVBT2;
    }
    else {
        tuneSystems[0] = system;
    }

    for (tuneIteration = 0; tuneIteration <=1; tuneIteration++) {
        /* Attempt DVB-T acquisition */
        if ((tuneSystems[tuneIteration] == SONY_DTV_SYSTEM_DVBT) && (!channelFound)) {
            sony_dvbt_tune_param_t tuneParam;

            tuneParam.bandwidth = bandwidth;
            tuneParam.centerFreqKHz = centerFreqKHz;
            tuneParam.profile = SONY_DVBT_PROFILE_HP;

            /* Set DVB-T profile to HP to allow detection of hierachical and non-hierachical modes */
            result = sony_demod_dvbt_SetProfile(pInteg->pDemod, SONY_DVBT_PROFILE_HP);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }

            /* Tune the demodulator */
            result = sony_demod_dvbt_Tune (pInteg->pDemod, &tuneParam);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }

#ifdef SAME_TUNER_CONFIG_DVBT_T2
            /* If SAME_TUNER_CONFIG_DVBT_T2 is defined the driver assumes
               that the tuner configuration used for DVBT tuning is identical to 
               DVBT2, therefore the tuner does not need to be reconfigured.  This will
               help to reduce blindtune and therefore scan duration. This is only 
               valid if SONY_DTV_SYSTEM_ANY is used. */
            if (tuneIteration == 0)
#endif
            {
                #ifndef CONFIG_TH_CXD2837_TUNER_MXL603
                if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Tune))
		#endif
		{
#ifndef SONY_DISABLE_I2C_REPEATER
                    /* Enable the I2C repeater */
                    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
                    if (result != SONY_RESULT_OK) {
                        SONY_TRACE_RETURN (result);
                    }
#endif

		    #ifdef CONFIG_TH_CXD2837_TUNER_MXL603
			result = Mxl603SetFreqBw(tuneParam.bandwidth,tuneParam.centerFreqKHz);
		    #else
                    /* Tune the RF part */
                    result = pInteg->pTunerTerrCable->Tune (pInteg->pTunerTerrCable, tuneParam.centerFreqKHz, SONY_DTV_SYSTEM_DVBT, tuneParam.bandwidth);
                    if (result != SONY_RESULT_OK) {
                        SONY_TRACE_RETURN (result);
                    }
		    #endif


#ifndef SONY_DISABLE_I2C_REPEATER
                    /* Disable the I2C repeater */
                    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
                    if (result != SONY_RESULT_OK) {
                        SONY_TRACE_RETURN (result);
                    }
#endif
                }
            }
            /* Reset the demod to enable acquisition */
            result = sony_demod_TuneEnd (pInteg->pDemod);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }    

            /* Wait for demodulator lock */
            result = dvbt_WaitDemodLock (pInteg);
            switch (result) {
            case SONY_RESULT_OK:
                channelFound = 1;
                *pSystemTuned = SONY_DTV_SYSTEM_DVBT;
                break;

            /* Intentional fall-through */
            case SONY_RESULT_ERROR_TIMEOUT:
            case SONY_RESULT_ERROR_UNLOCK:
                /* Attempt DVB-T2 acquisition */
                break;

            default:
                SONY_TRACE_RETURN(result);
            }
        }
        /* Attempt DVB-T2 acquisition */
        if ((tuneSystems[tuneIteration] == SONY_DTV_SYSTEM_DVBT2) && (!channelFound)) {
            sony_dvbt2_tune_param_t tuneParam;

            tuneParam.bandwidth = bandwidth;
            tuneParam.centerFreqKHz = centerFreqKHz;
            tuneParam.dataPlpId = 0; /* Not used in blind acquisition */

            /* Configure for automatic PLP selection. */
            result = sony_demod_dvbt2_SetPLPConfig (pInteg->pDemod, 0x01, 0x00);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }

            /* Configure the DVB-T2 profile without recovery */
            result = sony_demod_dvbt2_SetProfile(pInteg->pDemod, profile);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }

            /* Tune the demodulator */
            result = sony_demod_dvbt2_Tune (pInteg->pDemod, &tuneParam);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }

#ifdef SAME_TUNER_CONFIG_DVBT_T2
            /* If SAME_TUNER_CONFIG_DVBT_T2 is defined the driver assumes
               that the tuner configuration used for DVBT tuning is identical to 
               DVBT2, therefore the tuner does not need to be reconfigured.  This will
               help to reduce blindtune and therefore scan duration. This is only 
               valid if SONY_DTV_SYSTEM_ANY is used. */
            if (tuneIteration == 0)
#endif
            {
            #ifndef CONFIG_TH_CXD2837_TUNER_MXL603
                if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Tune))
	    #endif
	        {
#ifndef SONY_DISABLE_I2C_REPEATER
                    /* Enable the I2C repeater */
                    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
                    if (result != SONY_RESULT_OK) {
                        SONY_TRACE_RETURN (result);
                    }
#endif

                    /* Tune the RF part */
		   #ifdef CONFIG_TH_CXD2837_TUNER_MXL603
			result = Mxl603SetFreqBw(tuneParam.bandwidth,tuneParam.centerFreqKHz);
		   #else
                    result = pInteg->pTunerTerrCable->Tune (pInteg->pTunerTerrCable, tuneParam.centerFreqKHz, SONY_DTV_SYSTEM_DVBT2, tuneParam.bandwidth);
                    if (result != SONY_RESULT_OK) {
                        SONY_TRACE_RETURN (result);
                    }
		    #endif

#ifndef SONY_DISABLE_I2C_REPEATER
                    /* Disable the I2C repeater */
                    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
                    if (result != SONY_RESULT_OK) {
                        SONY_TRACE_RETURN (result);
                    }
#endif
                }
            }

            /* Reset the demod to enable acquisition */
            result = sony_demod_TuneEnd (pInteg->pDemod);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }    

            /* Wait for demodulator lock */
            result = dvbt2_WaitDemodLock (pInteg, profile);
            switch (result) {
            case SONY_RESULT_OK:
                /* In DVB-T2, L1 Post information may not immediately be valid after acquisition
                 * (L1POST_OK bit != 1).  This wait loop handles such cases.  This issue occurs 
                 * only under clean signal lab conditions, and will therefore not extend acquistion 
                 * time under normal conditions.
                 */    
                result = dvbt2_WaitL1PostLock (pInteg);
                if (result != SONY_RESULT_OK) {
                    SONY_TRACE_RETURN (result);
                }  

                if (profile == SONY_DVBT2_PROFILE_ANY) {
                    /* Obtain the current profile if detection was automatic. */
                    result = sony_demod_dvbt2_monitor_Profile (pInteg->pDemod, pProfileTuned);
                    if (result == SONY_RESULT_ERROR_HW_STATE) {
                        /* Demod lock is lost causing monitor to fail, return UNLOCK instead of HW STATE */
                        SONY_TRACE_RETURN (SONY_RESULT_ERROR_UNLOCK);
                    }
                    else if (result != SONY_RESULT_OK) {
                        /* Serious error, so return result */
                        SONY_TRACE_RETURN (result);
                    }   
                }
                else {
                    /* Else, set the tuned profile to the input parameter */
                    *pProfileTuned = profile;
                }
        
                *pSystemTuned = SONY_DTV_SYSTEM_DVBT2;
                channelFound = 1;
                break;

            /* Intentional fall-through */
            case SONY_RESULT_ERROR_TIMEOUT:
            case SONY_RESULT_ERROR_UNLOCK:
                break;

            default:
                SONY_TRACE_RETURN(result);
            }
        }
    }
    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_dvbt_WaitTSLock (sony_integ_t * pInteg)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
    sony_stopwatch_t timer;
    uint8_t continueWait = 1;
    uint32_t elapsed = 0;

    SONY_TRACE_ENTER ("sony_integ_dvbt_WaitTSLock");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE_T_C state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Wait for TS lock */
    result = sony_stopwatch_start (&timer);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    for (;;) {
        result = sony_stopwatch_elapsed(&timer, &elapsed);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
  
        if (elapsed >= SONY_DVBT_WAIT_TS_LOCK) {
            continueWait = 0;
        }

        result = sony_demod_dvbt_CheckTSLock (pInteg->pDemod, &lock);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        switch (lock) {
        case SONY_DEMOD_LOCK_RESULT_LOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_OK);

        case SONY_DEMOD_LOCK_RESULT_UNLOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_UNLOCK);

        default:
            /* continue waiting... */
            break;              
        }

        /* Check cancellation. */
        if (sony_atomic_read (&(pInteg->cancel)) != 0) {
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_CANCEL);
        }

        if (continueWait) {
            result = sony_stopwatch_sleep (&timer, SONY_DVBT_T2_WAIT_LOCK_INTERVAL);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }
        } else {
            result = SONY_RESULT_ERROR_TIMEOUT;
            break;
        }
    }
    
    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_dvbt2_WaitTSLock (sony_integ_t * pInteg, sony_dvbt2_profile_t profile)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
    uint16_t timeout = 0;
    sony_stopwatch_t timer;
    uint8_t continueWait = 1;
    uint32_t elapsed = 0;

    SONY_TRACE_ENTER ("sony_integ_dvbt2_WaitTSLock");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE_T_C state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Wait for TS lock */
    result = sony_stopwatch_start (&timer);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    if (profile == SONY_DVBT2_PROFILE_BASE) {
        timeout = SONY_DVBT2_BASE_WAIT_TS_LOCK;
    }
    else if (profile == SONY_DVBT2_PROFILE_LITE) {
        timeout = SONY_DVBT2_LITE_WAIT_TS_LOCK;
    }
    else {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    for (;;) {
        result = sony_stopwatch_elapsed(&timer, &elapsed);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
  
        if (elapsed >= timeout) {
            continueWait = 0;
        }

        result = sony_demod_dvbt2_CheckTSLock (pInteg->pDemod, &lock);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        switch (lock) {
        case SONY_DEMOD_LOCK_RESULT_LOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_OK);

        case SONY_DEMOD_LOCK_RESULT_UNLOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_UNLOCK);

        default:
            /* continue waiting... */
            break;              
        }

        /* Check cancellation. */
        if (sony_atomic_read (&(pInteg->cancel)) != 0) {
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_CANCEL);
        }

        if (continueWait) {
            result = sony_stopwatch_sleep (&timer, SONY_DVBT_T2_WAIT_LOCK_INTERVAL);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }
        } else {
            result = SONY_RESULT_ERROR_TIMEOUT;
            break;
        }
    }
    
    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_dvbt_t2_monitor_RFLevel (sony_integ_t * pInteg, int32_t * pRFLeveldB)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_dvbt_t2_monitor_RFLevel");

    if ((!pInteg) || (!pInteg->pDemod) || (!pRFLeveldB)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if ((pInteg->pDemod->system != SONY_DTV_SYSTEM_DVBT) && (pInteg->pDemod->system != SONY_DTV_SYSTEM_DVBT2)) {
        /* Not DVB-T or DVB-T2*/
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    switch (pInteg->pDemod->tunerOptimize) {
#ifndef SONY_INTEG_DISABLE_ASCOT_TUNER
    case SONY_DEMOD_TUNER_OPTIMIZE_ASCOT2D:
        {
            uint32_t ifAgc;

            if (pInteg->pDemod->system == SONY_DTV_SYSTEM_DVBT) {
                result = sony_demod_dvbt_monitor_IFAGCOut(pInteg->pDemod, &ifAgc);
            }
            else {
                result = sony_demod_dvbt2_monitor_IFAGCOut(pInteg->pDemod, &ifAgc);
            }

            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }

            /* Protect against overflow. IFAGC is unsigned 12-bit. */
            if (ifAgc > 0xFFF) {
                SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
            }

            /* 2nd order polynomial relationship :
             * RF (dB) = -4E-6 * IFAGC^2 + 0.0594 * IFAGC - 131.24
             * RF (dB*1000) = ((-4 * IFAGC^2) + (59400* IFAGC) - 131240000) / 1000
             */
            *pRFLeveldB = (int32_t) ((-4 * (int32_t) (ifAgc * ifAgc)) + (59400 * (int32_t) ifAgc) - 131240000);
            *pRFLeveldB = (*pRFLeveldB < 0) ? *pRFLeveldB - 500 : *pRFLeveldB + 500;
            *pRFLeveldB /= 1000;
        }
        break;

    case SONY_DEMOD_TUNER_OPTIMIZE_ASCOT2E:
        {
            uint32_t ifAgc;
            int32_t ifGain;
            int32_t rfGain;

            if (pInteg->pTunerTerrCable) {
                if (pInteg->pDemod->system == SONY_DTV_SYSTEM_DVBT) {
                    result = sony_demod_dvbt_monitor_IFAGCOut(pInteg->pDemod, &ifAgc);
                }
                else {
                    result = sony_demod_dvbt2_monitor_IFAGCOut(pInteg->pDemod, &ifAgc);
                }

                if (result != SONY_RESULT_OK) {
                    SONY_TRACE_RETURN (result);
                }

                /* Protect against overflow. IFAGC is unsigned 12-bit. */
                if (ifAgc > 0xFFF) {
                    SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
                }

                result = sony_tuner_ascot2e_CalcGainFromAGC(pInteg->pTunerTerrCable, ifAgc, &ifGain, &rfGain);
                if (result != SONY_RESULT_OK) {
                    SONY_TRACE_RETURN (result);
                }

                /* RF Level dBm = IFOUT - IFGAIN - RFGAIN
                 * IFOUT is the target IF level for tuner, -4.0dBm
                 */
                *pRFLeveldB = 10 * (-400 - ifGain - rfGain);

                /* Note : An implementation specific offset may be required
                 * to compensate for component gains / attenuations */
            }
            else {
                SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
            }
        }
        break;

    case SONY_DEMOD_TUNER_OPTIMIZE_ASCOT3:
        {
            int32_t rssi = 0;

            if (pInteg->pTunerTerrCable) {
                #ifndef SONY_DISABLE_I2C_REPEATER
                    /* Enable the I2C repeater */
                    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
                    if (result != SONY_RESULT_OK) {
                        SONY_TRACE_RETURN (result);
                    }
                #endif

                result = sony_tuner_ascot3_ReadRssi (pInteg->pTunerTerrCable, &rssi);
                if (result != SONY_RESULT_OK) {
                    SONY_TRACE_RETURN (result);
                }

                #ifndef SONY_DISABLE_I2C_REPEATER
                    /* Disable the I2C repeater */
                    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
                    if (result != SONY_RESULT_OK) {
                        SONY_TRACE_RETURN (result);
                    }
                #endif

                /* RF Level dBm = RSSI + IFOUT
                 * IFOUT is the target IF level for tuner, -4.0dBm
                 */
                *pRFLeveldB = 10 * (rssi - 400);
                
                /* Note : An implementation specific offset may be required
                 * to compensate for component gains / attenuations */
            }
            else {
                SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
            }
        }
        break;
#endif

    default:
        /* Please add RF level calculation for non ASCOT tuners. */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_NOSUPPORT);
    }

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_dvbt_monitor_SSI (sony_integ_t * pInteg, uint8_t * pSSI)
{
    int32_t rfLevel;
    sony_dvbt_tpsinfo_t tps;
    int32_t prel;
    int32_t tempSSI = 0;
    sony_result_t result = SONY_RESULT_OK;

    static const int32_t pRefdBm1000[3][5] = {
    /*    1/2,    2/3,    3/4,    5/6,    7/8,               */
        {-93000, -91000, -90000, -89000, -88000}, /* QPSK    */
        {-87000, -85000, -84000, -83000, -82000}, /* 16-QAM  */
        {-82000, -80000, -78000, -77000, -76000}, /* 64-QAM  */
    };

    SONY_TRACE_ENTER ("sony_integ_dvbt_monitor_SSI");

    if ((!pInteg) || (!pInteg->pDemod) || (!pSSI)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if (pInteg->pDemod->system != SONY_DTV_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Get estimated RF Level */
    result = sony_integ_dvbt_t2_monitor_RFLevel (pInteg, &rfLevel);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Monitor TPS for Modulation / Code Rate */
    result = sony_demod_dvbt_monitor_TPSInfo (pInteg->pDemod, &tps);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Ensure correct TPS values. */
    if ((tps.constellation >= SONY_DVBT_CONSTELLATION_RESERVED_3) || (tps.rateHP >= SONY_DVBT_CODERATE_RESERVED_5)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    /* prel = prec - pref */
    prel = rfLevel - pRefdBm1000[tps.constellation][tps.rateHP];

    /* SSI (Signal Strength Indicator) is calculated from:
     *
     * if (prel < -15dB)             SSI = 0
     * if (-15dB <= prel < 0dB)       SSI = (2/3) * (prel + 15)
     * if (0dB <= prel < 20dB)        SSI = (4 * prel) + 10
     * if (20dB <= prel < 35dB)       SSI = (2/3) * (prel - 20) + 90
     * if (prel >= 35dB)              SSI = 100
     */
    if (prel < -15000) {
        tempSSI = 0;
    }
    else if (prel < 0) {
        /* Note : prel and 2/3 scaled by 10^3 so divide by 10^6 added */
        tempSSI = ((2 * (prel + 15000)) + 1500) / 3000;
    }
    else if (prel < 20000) {
        /* Note : prel scaled by 10^3 so divide by 10^3 added */
        tempSSI = (((4 * prel) + 500) / 1000) + 10;
    }
    else if (prel < 35000) {
        /* Note : prel and 2/3 scaled by 10^3 so divide by 10^6 added */
        tempSSI = (((2 * (prel - 20000)) + 1500) / 3000) + 90;
    }
    else {
        tempSSI = 100;
    }

    /* Clip value to 100% */
    *pSSI = (tempSSI > 100)? 100 : (uint8_t)tempSSI;

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_dvbt2_monitor_SSI (sony_integ_t * pInteg, uint8_t * pSSI)
{
    int32_t rfLevel;
    sony_dvbt2_plp_constell_t qam;
    sony_dvbt2_plp_code_rate_t codeRate;
    int32_t prel;
    int32_t tempSSI = 0;
    sony_result_t result = SONY_RESULT_OK;

    static const int32_t pRefdBm1000[4][8] = {
    /*    1/2,    3/5,    2/3,    3/4,    4/5,    5/6,    1/3,    2/5                */
        {-96000, -95000, -94000, -93000, -92000, -92000, -98000, -97000}, /* QPSK    */
        {-91000, -89000, -88000, -87000, -86000, -86000, -93000, -92000}, /* 16-QAM  */
        {-86000, -85000, -83000, -82000, -81000, -80000, -89000, -88000}, /* 64-QAM  */
        {-82000, -80000, -78000, -76000, -75000, -74000, -86000, -84000}, /* 256-QAM */
    };

    SONY_TRACE_ENTER ("sony_integ_dvbt2_monitor_SSI");

    if ((!pInteg) || (!pInteg->pDemod) || (!pSSI)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if (pInteg->pDemod->system != SONY_DTV_SYSTEM_DVBT2) {
        /* Not DVB-T2*/
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Get estimated RF Level */
    result = sony_integ_dvbt_t2_monitor_RFLevel (pInteg, &rfLevel);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Get PLP constellation */
    result = sony_demod_dvbt2_monitor_QAM (pInteg->pDemod, SONY_DVBT2_PLP_DATA, &qam);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Get PLP code rate */
    result = sony_demod_dvbt2_monitor_CodeRate (pInteg->pDemod, SONY_DVBT2_PLP_DATA, &codeRate);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Ensure correct plp info. */
    if ((codeRate > SONY_DVBT2_R2_5) || (qam > SONY_DVBT2_QAM256)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    /* prel = prec - pref */
    prel = rfLevel - pRefdBm1000[qam][codeRate];

    /* SSI (Signal Strength Indicator) is calculated from:
     *
     * if (prel < -15dB)              SSI = 0
     * if (-15dB <= prel < 0dB)       SSI = (2/3) * (prel + 15)
     * if (0dB <= prel < 20dB)        SSI = 4 * prel + 10
     * if (20dB <= prel < 35dB)       SSI = (2/3) * (prel - 20) + 90
     * if (prel >= 35dB)              SSI = 100
     */
    if (prel < -15000) {
        tempSSI = 0;
    }
    else if (prel < 0) {
        /* Note : prel and 2/3 scaled by 10^3 so divide by 10^6 added */
        tempSSI = ((2 * (prel + 15000)) + 1500) / 3000;
    }
    else if (prel < 20000) {
        /* Note : prel scaled by 10^3 so divide by 10^3 added */
        tempSSI = (((4 * prel) + 500) / 1000) + 10;
    }
    else if (prel < 35000) {
        /* Note : prel and 2/3 scaled by 10^3 so divide by 10^6 added */
        tempSSI = (((2 * (prel - 20000)) + 1500) / 3000) + 90;
    }
    else {
        tempSSI = 100;
    }

    /* Clip value to 100% */
    *pSSI = (tempSSI > 100)? 100 : (uint8_t)tempSSI;

    SONY_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/
static sony_result_t dvbt_WaitDemodLock (sony_integ_t * pInteg)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
    sony_stopwatch_t timer;
    uint8_t continueWait = 1;
    uint32_t elapsed = 0;

    SONY_TRACE_ENTER ("dvbt_WaitDemodLock");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE_T_C state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Wait for demod lock */
    result = sony_stopwatch_start (&timer);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    for (;;) {
        result = sony_stopwatch_elapsed(&timer, &elapsed);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
  
        if (elapsed >= SONY_DVBT_WAIT_DEMOD_LOCK) {
            continueWait = 0;
        }

        result = sony_demod_dvbt_CheckDemodLock (pInteg->pDemod, &lock);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        switch (lock) {
        case SONY_DEMOD_LOCK_RESULT_LOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_OK);

        case SONY_DEMOD_LOCK_RESULT_UNLOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_UNLOCK);

        default:
            /* continue waiting... */
            break;              
        }

        /* Check cancellation. */
        if (sony_atomic_read (&(pInteg->cancel)) != 0) {
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_CANCEL);
        }

        if (continueWait) {
            result = sony_stopwatch_sleep (&timer, SONY_DVBT_T2_WAIT_LOCK_INTERVAL);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }
        } else {
            result = SONY_RESULT_ERROR_TIMEOUT;
            break;
        }
    }
    
    SONY_TRACE_RETURN (result);
}

static sony_result_t dvbt2_WaitDemodLock (sony_integ_t * pInteg, sony_dvbt2_profile_t profile)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_demod_lock_result_t lock = SONY_DEMOD_LOCK_RESULT_NOTDETECT;
    uint16_t timeout = 0;
    sony_stopwatch_t timer;
    uint8_t continueWait = 1;
    uint32_t elapsed = 0;

    SONY_TRACE_ENTER ("dvbt2_WaitDemodLock");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE_T_C state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Wait for demod lock */
    result = sony_stopwatch_start (&timer);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    if (profile == SONY_DVBT2_PROFILE_BASE) {
        timeout = SONY_DVBT2_BASE_WAIT_DEMOD_LOCK;
    }
    else if ((profile == SONY_DVBT2_PROFILE_LITE) || (profile == SONY_DVBT2_PROFILE_ANY)) {
        timeout = SONY_DVBT2_LITE_WAIT_DEMOD_LOCK;
    }
    else {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    for (;;) {
        result = sony_stopwatch_elapsed(&timer, &elapsed);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
  
        if (elapsed >= timeout) {
            continueWait = 0;
        }

        result = sony_demod_dvbt2_CheckDemodLock (pInteg->pDemod, &lock);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        switch (lock) {
        case SONY_DEMOD_LOCK_RESULT_LOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_OK);

        case SONY_DEMOD_LOCK_RESULT_UNLOCKED:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_UNLOCK);

        default:
            /* continue waiting... */
            break;              
        }

        /* Check cancellation. */
        if (sony_atomic_read (&(pInteg->cancel)) != 0) {
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_CANCEL);
        }

        if (continueWait) {
            result = sony_stopwatch_sleep (&timer, SONY_DVBT_T2_WAIT_LOCK_INTERVAL);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }
        } else {
            result = SONY_RESULT_ERROR_TIMEOUT;
            break;
        }
    }
    
    SONY_TRACE_RETURN (result);
}

static sony_result_t dvbt2_WaitL1PostLock (sony_integ_t * pInteg)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_stopwatch_t timer;
    uint8_t continueWait = 1;
    uint32_t elapsed = 0;
    uint8_t l1PostValid;

    SONY_TRACE_ENTER ("dvbt2_WaitL1PostLock");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = sony_stopwatch_start (&timer);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    for (;;) {
        result = sony_stopwatch_elapsed(&timer, &elapsed);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
  
        /* Check for timeout condition */
        if (elapsed >= SONY_DVBT2_L1POST_TIMEOUT) {
            continueWait = 0;
        }

        result = sony_demod_dvbt2_CheckL1PostValid (pInteg->pDemod, &l1PostValid);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }

        /* If L1 Post is valid, return from loop, else continue waiting */
        if (l1PostValid) {
            SONY_TRACE_RETURN (SONY_RESULT_OK);
        }
        
        /* Check cancellation. */
        if (sony_atomic_read (&(pInteg->cancel)) != 0) {
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_CANCEL);
        }

        if (continueWait) {
            result = sony_stopwatch_sleep (&timer, SONY_DVBT_T2_WAIT_LOCK_INTERVAL);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }
        } 
        else {
            result = SONY_RESULT_ERROR_TIMEOUT;
            break;
        }
    }
    
    SONY_TRACE_RETURN (result);
}

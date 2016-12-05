/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2013-05-03 12:09:14 #$
  File Revision : $Revision:: 7035 $
------------------------------------------------------------------------------*/
/**
 @file    sony_integ_dvbt_t2.h

          This file provides the integration layer interface for DVB-T and DVB-T2
          specific demodulator functions.
*/
/*----------------------------------------------------------------------------*/

#ifndef SONY_INTEG_DVBT_T2_H
#define SONY_INTEG_DVBT_T2_H

#include "sony_demod.h"
#include "sony_integ.h"
#include "sony_demod_dvbt.h"
#include "sony_demod_dvbt2.h"

/*------------------------------------------------------------------------------
 Defines
------------------------------------------------------------------------------*/
#define SONY_DVBT_WAIT_DEMOD_LOCK           1000    /**< 1s timeout for wait demodulator lock process for DVB-T channels */
#define SONY_DVBT_WAIT_TS_LOCK              1000    /**< 1s timeout for wait TS lock process for DVB-T channels */
#define SONY_DVBT2_BASE_WAIT_DEMOD_LOCK     3500    /**< 3.5s timeout for wait demodulator lock process for DVB-T2-Base channels */
#define SONY_DVBT2_BASE_WAIT_TS_LOCK        1500    /**< 1.5s timeout for wait TS lock process for DVB-T2-Base channels */
#define SONY_DVBT2_LITE_WAIT_DEMOD_LOCK     5000    /**< 5.0s timeout for wait demodulator lock process for DVB-T2-Lite channels */
#define SONY_DVBT2_LITE_WAIT_TS_LOCK        2300    /**< 2.3s timeout for wait TS lock process for DVB-T2-Lite channels */
#define SONY_DVBT_T2_WAIT_LOCK_INTERVAL     10      /**< 10ms polling interval for demodulator and TS lock functions */
#define SONY_DVBT2_L1POST_TIMEOUT           300     /**< 300ms timeout for L1Post Valid loop */

/*------------------------------------------------------------------------------
 Structs
------------------------------------------------------------------------------*/
/**
 @brief The parameters used for DVB-T and DVB-T2 scanning.
*/
typedef struct sony_integ_dvbt_t2_scan_param_t{
    /**
     @brief The start frequency in kHz for scanning.  Ensure that this is
            aligned with the channel raster.
    */
    uint32_t startFrequencyKHz;

    /**
     @brief The end frequency in kHz for scanning.
    */
    uint32_t endFrequencyKHz;

    /**
     @brief The step frequency in kHz for scanning.
    */
    uint32_t stepFrequencyKHz;

    /**
     @brief The bandwidth to use for tuning during the scan
    */
    sony_demod_bandwidth_t bandwidth;

    /**
     @brief The system to attempt to blind tune to at each step.  Use
            ::SONY_DTV_SYSTEM_ANY to run a multiple system scan (DVB-T and
            DVB-T2).
    */
    sony_dtv_system_t system;

    /** 
     @brief The DVB-T2 profile to use for the blind tune.  Use 
            ::SONY_DVBT2_PROFILE_ANY for mixed or Base and Lite spectrums.
    */
    sony_dvbt2_profile_t t2Profile;
}sony_integ_dvbt_t2_scan_param_t;

/**
 @brief The structure used to return a channel located or progress update 
        as part of a DVB-T / DVB-T2 or combined scan.
*/
typedef struct sony_integ_dvbt_t2_scan_result_t{
    /**
     @brief Indicates the current frequency just attempted for the scan.  This would
            primarily be used to calculate scan progress from the scan parameters.
    */
    uint32_t centerFreqKHz;

    /**
     @brief Indicates if the tune result at the current frequency.  SONY_RESULT_OK
            means that a channel has been locked and one of the tuneParam structures
            contain the channel infomration.
    */
    sony_result_t tuneResult;

    /**
     @brief The system of the channel detected by the scan.  This should be used to determine
            which of the following tune param structs are valid.
    */
    sony_dtv_system_t system;

    /**
     @brief The tune params for a located DVB-T channel.
    */
    sony_dvbt_tune_param_t dvbtTuneParam;

    /**
     @brief The tune params for a located DVB-T2 channel.
    */
    sony_dvbt2_tune_param_t dvbt2TuneParam;
}sony_integ_dvbt_t2_scan_result_t;

/*------------------------------------------------------------------------------
 Function Pointers
------------------------------------------------------------------------------*/
/**
 @brief Callback function that is called for every attempted frequency during a 
        scan.  For successful channel results the function is called after 
        demodulator lock but before TS lock is achieved (DVB-T : TPS Lock, 
        DVB-T2 : Demod Lock).
        
        NOTE: for DVB-T2 this function is invoked for each PLP within the signal.
 
 @param pInteg The driver instance.
 @param pResult The current scan result.
 @param pScanParam The current scan parameters.
*/
typedef void (*sony_integ_dvbt_t2_scan_callback_t) (sony_integ_t * pInteg, 
                                                    sony_integ_dvbt_t2_scan_result_t * pResult,
                                                    sony_integ_dvbt_t2_scan_param_t * pScanParam);

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
/**
 @brief Performs acquisition to a DVB-T channel. 
        Blocks the calling thread until the TS has locked or has timed out.
        Use ::sony_integ_Cancel to cancel the operation at any time.
 
 @note  For non-hierachical modes, the profile should be set to 
        SONY_DVBT_PROFILE_HP. If SONY_DVBT_PROFILE_LP is requested, but the 
        detected signal mode is non-hierachical, the demodulator will default to 
        HP, incurring a slight time penalty in acquisition.

 @param pInteg The driver instance.
 @param pTuneParam The parameters required for the tune.

 @return SONY_RESULT_OK if tuned successfully to the channel.
*/
sony_result_t sony_integ_dvbt_Tune(sony_integ_t * pInteg,
                                   sony_dvbt_tune_param_t * pTuneParam);

/**
 @brief Performs acquisition to a DVB-T2 channel. 
        Blocks the calling thread until the TS has locked or has timed out.
        Use ::sony_integ_Cancel to cancel the operation at any time.
 
        During a tune the device will wait for a T2 P1 symbol in order to decode the 
        L1 pre signalling and then begin demodulation. If the data PLP ID 
        (::sony_dvbt2_tune_param_t::dataPlpId) or the associated common PLP is not 
        found in the channel, the device will always select the first found PLP and 
        output the associated TS. In this case, an indicator in the 
        ::sony_dvbt2_tune_param_t::tuneInfo will be set.
          
 @param pInteg The driver instance.
 @param pTuneParam The parameters required for the tune.

 @return SONY_RESULT_OK if tuned successfully to the channel.
*/
sony_result_t sony_integ_dvbt2_Tune(sony_integ_t * pInteg,
                                    sony_dvbt2_tune_param_t * pTuneParam);

/**
 @brief Attempts to acquire to the channel at the center frequency provided. The
        system can be specified directly or set to ::SONY_DTV_SYSTEM_ANY to allow
        tuning to DVB-T or DVB-T2 for unknown cases.

        This function blocks the calling thread until the demod has locked or has 
        timed out. Use ::sony_integ_Cancel to cancel the operation at any time.

        For TS lock please call the wait TS lock function
        ::sony_integ_dvbt_WaitTSLock or ::sony_integ_dvbt2_WaitTSLock.
 
        NOTE: For T2 the PLP selected will be the first found in the L1 pre signalling.
        Use ::sony_demod_dvbt2_monitor_DataPLPs to obtain a full list of PLPs contained
        in the T2 signal.

        Note: For DVB-T2 the output selected if profile is set to ::SONY_DVBT2_PROFILE_ANY
        will be determined by the first frame received.
          
 @param pInteg The driver instance.
 @param centerFreqKHz The center frequency of the channel to attempt acquisition on
 @param bandwidth The bandwidth of the channel
 @param system The system to attempt to tune to, use ::SONY_DTV_SYSTEM_ANY to attempt
               both DVB-T and DVB-T2
 @param profile The DVB-T2 profile to detect, use ::SONY_DVBT2_PROFILE_ANY to detect Base 
                or Lite.
 @param pSystemTuned The system of the channel located by the blind tune.
 @param pProfileTuned The DVB-T2 profile tuned by the blind tune.

 @return SONY_RESULT_OK if tuned successfully to the channel.
*/
sony_result_t sony_integ_dvbt_t2_BlindTune(sony_integ_t * pInteg,
                                           uint32_t centerFreqKHz,
                                           sony_demod_bandwidth_t bandwidth,
                                           sony_dtv_system_t system,
                                           sony_dvbt2_profile_t profile,
                                           sony_dtv_system_t * pSystemTuned,
                                           sony_dvbt2_profile_t * pProfileTuned);

/**
 @brief Performs a scan over the spectrum specified. 

        The scan can perform a multiple system scan for DVB-T and DVB-T2 channels by 
        setting the ::sony_integ_dvbt_t2_scan_param_t::system to ::SONY_DTV_SYSTEM_ANY
        and setting the.

        Blocks the calling thread while scanning. Use ::sony_integ_Cancel to cancel 
        the operation at any time.
 
 @param pInteg The driver instance.
 @param pScanParam The scan parameters.
 @param callBack User registered call-back to receive scan progress information and 
        notification of found channels. The call back is called for every attempted 
        frequency during a scan.

 @return SONY_RESULT_OK if scan completed successfully.
        
*/
sony_result_t sony_integ_dvbt_t2_Scan(sony_integ_t * pInteg,
                                      sony_integ_dvbt_t2_scan_param_t * pScanParam,
                                      sony_integ_dvbt_t2_scan_callback_t callBack);

/**
 @brief Polls the demodulator waiting for TS lock at 10ms intervals up to a timeout of 1s.

 @param pInteg The driver instance

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_integ_dvbt_WaitTSLock (sony_integ_t * pInteg);

/**
 @brief Polls the demodulator waiting for TS lock at 10ms intervals up to a profile dependant 
        timeout duration (DVB-T2_Base : 1.5s, DVB-T2-Lite : 2.3s).  SONY_DVBT2_PROFILE_ANY is
        invalid for this API.

 @param pInteg The driver instance
 @param profile The DVB-T2 profile (base or lite)

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_integ_dvbt2_WaitTSLock (sony_integ_t * pInteg, sony_dvbt2_profile_t profile);

/**
 @brief This function returns the estimated RF level based on demodulator gain measurements
        and a tuner dependant conversion calculation.  The calculation provided in this monitor
        has been optimised for either Sony Evaluation Board PCB-0085-MV3.0 or PCB-0093-MV1.0, and 
        may require modifications for your own HW integration.  Please contact Sony support 
        for advice on characterising your own implementation.

 @param pInteg The driver instance
 @param pRFLeveldB The RF Level estimation in dB * 1000

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_integ_dvbt_t2_monitor_RFLevel (sony_integ_t * pInteg, int32_t * pRFLeveldB);

/**
 @brief DVB-T monitor for SSI (Signal Strength Indicator), based on the RF Level monitor value
        ::sony_integ_dvbt_t2_monitor_RFLevel.

 @note The RF Level monitor function should be optimised for your HW configuration before using
       this monitor.

 @param pInteg The driver instance
 @param pSSI The Signal Strength Indicator value in %

 @return SONY_RESULT_OK if successful
*/
sony_result_t sony_integ_dvbt_monitor_SSI (sony_integ_t * pInteg, uint8_t * pSSI);

/**
 @brief DVB-T2 monitor for SSI (Signal Strength Indicator), based on the RF Level monitor value
        ::sony_integ_dvbt_t2_monitor_RFLevel.

 @note The RF Level monitor function should be optimised for your HW configuration before using
       this monitor.

 @param pInteg The driver instance
 @param pSSI The Signal Strength Indicator value in %

 @return SONY_RESULT_OK if successful
*/
sony_result_t sony_integ_dvbt2_monitor_SSI (sony_integ_t * pInteg, uint8_t * pSSI);
#endif /* SONY_INTEG_DVBT_T2_H */

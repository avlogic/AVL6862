/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2013-03-22 14:36:23 #$
  File Revision : $Revision:: 6794 $
------------------------------------------------------------------------------*/
/**
 @file    sony_example_dvbt2_tune_monitor.c

         This file provides an example of DVB-T2 tuning and looped monitoring
*/
/*------------------------------------------------------------------------------
 Includes
------------------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "sony_example_terr_cable_configuration.h" /* HW configuration definition */ 
#include "sony_integ.h"                 /* Integration layer API */
#include "sony_demod_driver_version.h"  /* Driver version file */
#include "sony_demod.h"                 /* Demodulator layer API and definitions */
#include "sony_demod_dvbt2.h"           /* DVBT2 type definitions and demodulator functions */
#include "sony_integ_dvbt_t2.h"         /* DVBT2 integration layer functions (Tune, Scan, RF Level monitor etc)*/
#include "sony_demod_dvbt2_monitor.h"   /* DVBT2 monitor functions */
#include "drvi2c_feusb.h"               /* USB->I2C driver implementation. */
#ifdef SONY_EXAMPLE_TUNER_ASCOT2D    
#include "sony_ascot2d.h"               /* Sony ASCOT2D tuner driver core. */
#include "sony_tuner_ascot2d.h"         /* Sony ASCOT2D tuner driver wrapper. */
#elif defined SONY_EXAMPLE_TUNER_ASCOT2E
#include "sony_ascot2e.h"               /* Sony ASCOT2D tuner driver core. */
#include "sony_tuner_ascot2e.h"         /* Sony ASCOT2D tuner driver wrapper. */
#elif defined SONY_EXAMPLE_TUNER_ASCOT3
#include "sony_ascot3.h"                /* Sony ASCOT3 tuner driver core. */
#include "sony_tuner_ascot3.h"          /* Sony ASCOT3 tuner driver wrapper. */
#endif

#ifdef __cplusplus
}
#endif

/*------------------------------------------------------------------------------
 Defines
------------------------------------------------------------------------------*/
/* Comment out definitions below to disable monitor output */
#define MONITOR_SYNCSTAT            
#define MONITOR_CARRIEROFFSET
#define MONITOR_IFAGCOUT
#define MONITOR_L1PRE
#define MONITOR_VERSION
#define MONITOR_OFDM
#define MONITOR_DATAPLPS
#define MONITOR_ACTIVEPLP
#define MONITOR_DATAPLPERROR
#define MONITOR_L1CHANGE
#define MONITOR_L1POST
#define MONITOR_SPECTRUMSENSE
#define MONITOR_SNR
#define MONITOR_PRELDPCBER
#define MONITOR_POSTBCHFER
#define MONITOR_PREBCHBER
#define MONITOR_PACKETERRORNUMBER
#define MONITOR_PPMOFFSET
#define MONITOR_TSRATE
#define MONITOR_QUALITY
#define MONITOR_PER
#define MONITOR_PROFILE
#define MONITOR_SSI
#define MONITOR_RFLEVEL

/*------------------------------------------------------------------------------
 Const char definitions
------------------------------------------------------------------------------*/
static const char *DVBT2_TuneInfo[] = { "OK", "Invalid PLP ID", "Invalid T2 Mode"};
static const char *DVBT2_Version[] = { "1.1.1", "1.2.1", "1.3.1" };
static const char *DVBT2_S1Signalling[] = { "SISO", "MISO", "Non-DVBT2", "T2-Lite SISO", "T2-Lite MISO" };
static const char *DVBT2_GaurdInterval[] = { "1/32", "1/16", "1/8", "1/4", "1/128", "19/128", "19/256" };
static const char *DVBT2_FFTMode[] = { "2k", "8k", "4k", "1k", "16k", "32k" };
static const char *DVBT2_L1PreType[] = { "TS", "GS", "TS & GS" };
static const char *DVBT2_PAPR[] = { "None", "ACE", "TR", "TR & ACE" };
static const char *DVBT2_L1PostModulation[] = { "BPSK", "QPSK", "16QAM", "64QAM" };
static const char *DVBT2_L1PostCodeRate[] = { "1/2" };
static const char *DVBT2_L1PostFEC[] = { "16k" };
static const char *DVBT2_PPMode[] = { "PP1", "PP2", "PP3", "PP4", "PP5", "PP6", "PP7", "PP8" };
static const char *DVBT2_CodeRate[] = { "1/2", "3/5", "2/3", "3/4", "4/5", "5/6", "1/3", "2/5" };
static const char *DVBT2_PLPModulation[] = { "QPSK", "16QAM", "64QAM", "256QAM" };
static const char *DVBT2_PLPType[] = { "Common", "Data Type 1", "Data Type 2" };
static const char *DVBT2_PLPPayload[] = { "GFPS", "GCS", "GSE", "TS" };
static const char *DVBT2_FEC[] = { "16k", "64k" };
static const char *DVBT2_Profile[] = { "T2-Base", "T2-Lite", "Any" };

/*------------------------------------------------------------------------------
 Static Function Prototypes
 ------------------------------------------------------------------------------*/
static sony_result_t sony_integ_dvbt2_monitor_AveragedSNR (sony_integ_t * pInteg, int32_t * pSNR);
static sony_result_t sony_integ_dvbt2_monitor_AveragedQuality (sony_integ_t * pInteg, uint8_t * pQuality);

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
    sony_result_t result = SONY_RESULT_OK;
    sony_result_t tuneResult = SONY_RESULT_OK;
            
    sony_integ_t integ;
    sony_demod_t demod;
    sony_tuner_terr_cable_t tunerTerrCable;
#ifdef SONY_EXAMPLE_TUNER_ASCOT2D    
    sony_ascot2d_t ascot2d;
#elif defined SONY_EXAMPLE_TUNER_ASCOT2E
    sony_ascot2e_t ascot2e;
#elif defined SONY_EXAMPLE_TUNER_ASCOT3
    sony_ascot3_t ascot3;
#endif

    sony_i2c_t i2c;
    sony_dvbt2_tune_param_t tuneParam;
    drvi2c_feusb_t feusb;
    uint8_t i = 0;

    argc = argc;
    argv = argv;

    /*------------------------------------------------------------------------------
     Setup / Initialisation
    ------------------------------------------------------------------------------*/
    /* Create I2C interface for tuner and demosulator parts.  GW (GateWay) members 
       provided but not required for tuner communication.  I2C switching handled 
       internally in the driver using a repeater. */
    i2c.gwAddress = 0x00;                                   /* N/A */
    i2c.gwSub = 0x00;                                       /* N/A */
    i2c.Read = drvi2c_feusb_Read;                           /* Base level HW interfacing I2C read function */
    i2c.Write = drvi2c_feusb_Write;                         /* Base level HW interfacing I2C write function */
    i2c.ReadRegister = sony_i2c_CommonReadRegister;         /* Common wrapper function for multi byte Read operation */
    i2c.WriteRegister = sony_i2c_CommonWriteRegister;       /* Common wrapper function for multi byte Write operation */
    i2c.WriteOneRegister = sony_i2c_CommonWriteOneRegister; /* Common wrapper function for single byte Write operation */
    i2c.user = (void *) &feusb;                             /* I2C driver instance */

    /* Display driver credentials */
    printf ("Driver Version : %s\n", SONY_DEMOD_DRIVER_VERSION);
    printf ("Built          : %s %s\n\n", SONY_DEMOD_DRIVER_RELEASE_DATE, SONY_DEMOD_DRIVER_RELEASE_TIME);

    printf ("------------------------------------------\n");
    printf (" Create / Inititialize   \n");
    printf ("------------------------------------------\n");

#ifdef SONY_EXAMPLE_TUNER_ASCOT2D
    /* Create ASCOT2D tuner using the parameters defined in sony_example_terr_cable_configuration.h */
    {
        uint8_t xtalFreqMHz = SONY_EXAMPLE_TUNER_XTAL;
        uint8_t i2cAddress = SONY_EXAMPLE_TUNER_I2C_ADDRESS;
        uint32_t configFlags = SONY_EXAMPLE_TUNER_FLAGS;

        result = sony_tuner_ascot2d_Create (&tunerTerrCable, xtalFreqMHz, i2cAddress, &i2c, configFlags, &ascot2d);
        if (result == SONY_RESULT_OK) {
            printf (" Tuner Created with the following parameters:\n");
            printf ("  - Tuner Type     : CXD2831 (ASCOT2D) \n");
            printf ("  - XTal Frequency : %uMHz\n", xtalFreqMHz);
            printf ("  - I2C Address    : %u\n", i2cAddress);
            printf ("  - Config Flags   : %u\n\n", configFlags);
        }
        else {
            printf (" Error: Unable to create Sony ASCOT2D tuner driver. (result = %s)\n", Common_Result[result]);
            return -1;
        }
    }
#elif defined SONY_EXAMPLE_TUNER_ASCOT2E
    /* Create ASCOT2E tuner using the parameters defined in sony_example_terr_cable_configuration.h */
    {
        uint8_t xtalFreqMHz = SONY_EXAMPLE_TUNER_XTAL;
        uint8_t i2cAddress = SONY_EXAMPLE_TUNER_I2C_ADDRESS;
        uint32_t configFlags = SONY_EXAMPLE_TUNER_FLAGS;

        result = sony_tuner_ascot2e_Create (&tunerTerrCable, xtalFreqMHz, i2cAddress, &i2c, configFlags, &ascot2e);
        if (result == SONY_RESULT_OK) {
            printf (" Tuner Created with the following parameters:\n");
            printf ("  - Tuner Type     : CXD2861 (ASCOT2E) \n");
            printf ("  - XTal Frequency : %uMHz\n", xtalFreqMHz);
            printf ("  - I2C Address    : %u\n", i2cAddress);
            printf ("  - Config Flags   : %u\n\n", configFlags);
        }
        else {
            printf (" Error: Unable to create Sony ASCOT2E tuner driver. (result = %s)\n", Common_Result[result]);
            return -1;
        }
    }
#elif defined SONY_EXAMPLE_TUNER_ASCOT3
    /* Create ASCOT3 tuner using the parameters defined in sony_example_terr_cable_configuration.h */
    {
        sony_ascot3_xtal_t xtalFreq = SONY_EXAMPLE_TUNER_XTAL;
        uint8_t i2cAddress = SONY_EXAMPLE_TUNER_I2C_ADDRESS;
        uint32_t configFlags = SONY_EXAMPLE_TUNER_FLAGS;

        result = sony_tuner_ascot3_Create (&tunerTerrCable, xtalFreq, i2cAddress, &i2c, configFlags, &ascot3);
        if (result == SONY_RESULT_OK) {
            printf (" Tuner Created with the following parameters:\n");
            printf ("  - Tuner Type     : CXD2871/72 (ASCOT3) \n");
            printf ("  - XTal Frequency : %sMHz\n", ASCOT3_Xtal[xtalFreq]);
            printf ("  - I2C Address    : %u\n", i2cAddress);
            printf ("  - Config Flags   : %u\n\n", configFlags);
        }
        else {
            printf (" Error: Unable to create Sony ASCOT3 tuner driver. (result = %s)\n", Common_Result[result]);
            return -1;
        }
    }
#elif defined SONY_EXAMPLE_TUNER_OTHER
    /* Other third party tuner.  Add in tuner object creation call here. */
    printf (" Error: Tuner not defined.");
    return -1;
#endif

    /* Create the integration structure which contains the demodulaor and tuner part instances.  This 
     * function also internally Creates the demodulator part.  Once created the driver is in 
     * SONY_DEMOD_STATE_INVALID and must be initialized before calling a Tune / Scan or Monitor API. 
     */
    {
        /* The following settings are taken from sony_example_terr_cable_configuration.h */
        sony_demod_xtal_t xtalFreq = SONY_EXAMPLE_DEMOD_XTAL;
        uint8_t i2cAddress = SONY_EXAMPLE_DEMOD_I2C_ADDRESS;

        /* Create parameters for integration structure:
         *  sony_integ_t * pInteg                       Integration object
         *  sony_demod_xtal_t xtalFreq                  Demodulator xTal frequency
         *  uint8_t i2cAddress                          Demodulator I2C address
         *  sony_i2c_t i2c                              Demodulator I2C driver
         *  sony_demod_t *pDemod                        Demodulator object
         *
         *  Note: Set the following to NULL to disable control
         *  sony_tuner_terr_cable_t * pTunerTerrCable   Terrestrial / Cable tuner object 
         *  sony_tuner_sat_t * pTunerSat                Satellite tuner object
         *  sony_lnbc_t * pLnbc                         LNB Controller object
         */
        result = sony_integ_Create (&integ, xtalFreq, i2cAddress, &i2c, &demod
#ifdef SONY_DEMOD_SUPPORT_TERR_OR_CABLE
            /* Terrestrial and Cable supported so include the tuner object into the Create API */
            ,&tunerTerrCable
#endif
#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
            /* Satellite supported so include the tuner and LNB objects into the Create API */
            , NULL, NULL
#endif
            );

        if (result == SONY_RESULT_OK) {
            printf (" Demod Created with the following parameters:\n");
            printf ("  - XTal Frequency : %s\n", Common_DemodXtal[xtalFreq]);
            printf ("  - I2C Address    : %u\n\n", i2cAddress);
        }
        else {
            printf (" Error: Unable to create demodulator driver. (result = %s)\n", Common_Result[result]);
            return -1;
        }
    }

    /* Initialize the I2C interface */
    result = drvi2c_feusb_Initialize (&feusb);
    if (result == SONY_RESULT_OK) {
        printf (" I2C interface initialized\n\n");
    }
    else {
        printf (" Error: Unable to initialise FEUSB I2C driver. (result = %s)\n", Common_Result[result]);
        return -1;
    }

    /* Initialize the tuner and demodulator parts to Terrestrial / Cable mode.  Following this call the
     * driver will be in SONY_DEMOD_STATE_SLEEP_T_C state. From here you can call any Shutdown, Sleep or Tune
     * API for Terrestrial / Cable systems or call sony_integ_SleepS to transfer to Satellite mode. 
     * 
     * Note : Initialize API should only be called once at the start of the driver creation.  Subsequent calls
     *        to any of the Tune API's do not require re-initialize. 
     */
    result = sony_integ_InitializeT_C (&integ);
    if (result == SONY_RESULT_OK) {
        printf (" Driver initialized, current state = SONY_DEMOD_STATE_SLEEP_T_C\n\n");
    }
    else {
        printf (" Error: Unable to initialise the integration driver to terrestiral / cable mode. (result = %s)\n", Common_Result[result]);
        return -1;
    }

    /* ---------------------------------------------------------------------------------
     * Configure the Demodulator
     * ------------------------------------------------------------------------------ */
    /* DVB-T demodulator IF configuration for terrestrial / cable tuner */
    demod.iffreqConfig.configDVBT_5 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT_5MHz_IF);
    demod.iffreqConfig.configDVBT_6 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT_6MHz_IF);
    demod.iffreqConfig.configDVBT_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT_7MHz_IF);
    demod.iffreqConfig.configDVBT_8 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT_8MHz_IF);

    /* DVB-T2 demodulator IF configuration for terrestrial / cable tuner */
    demod.iffreqConfig.configDVBT2_1_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_1_7MHz_IF);
    demod.iffreqConfig.configDVBT2_5 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_5MHz_IF);
    demod.iffreqConfig.configDVBT2_6 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_6MHz_IF);
    demod.iffreqConfig.configDVBT2_7 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_7MHz_IF);
    demod.iffreqConfig.configDVBT2_8 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBT2_8MHz_IF);

    /* DVB-C demodulator IF configuration for terrestrial / cable tuner */
    demod.iffreqConfig.configDVBC = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBC_IF);

    /* DVB-C2 demodulator IF configuration for terrestrial / cable tuner */
    demod.iffreqConfig.configDVBC2_6 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBC2_6MHz_IF);
    demod.iffreqConfig.configDVBC2_8 = SONY_DEMOD_MAKE_IFFREQ_CONFIG (SONY_EXAMPLE_DVBC2_8MHz_IF);

    /* Demod tunerOptimize member allows the demod to be optimized internally when
       connected to Sony RF parts. */
    demod.tunerOptimize = SONY_EXAMPLE_TUNER_OPTIMIZE;

    printf ("------------------------------------------\n");
    printf (" Demodulator configuration \n");
    printf ("------------------------------------------\n");
    /* Run through the defined Set Config options */
    {
        uint8_t configIndex = 0;
        uint8_t configCount = sizeof(demodulatorConfiguration) / sizeof(sony_example_demod_configuration_t);

        for (configIndex = 0 ; configIndex < configCount ; configIndex++) {
            result = sony_demod_SetConfig (&demod, 
                                           demodulatorConfiguration[configIndex].configId, 
                                           demodulatorConfiguration[configIndex].configValue);
            if (result == SONY_RESULT_OK) {
                printf (" %u. %s set to %u\n", configIndex, 
                                               Common_ConfigId[demodulatorConfiguration[configIndex].configId], 
                                               demodulatorConfiguration[configIndex].configValue);
            }
            else {
                printf (" Error setting %s to %u (result = %s)\n", Common_ConfigId[demodulatorConfiguration[configIndex].configId], 
                                                                   demodulatorConfiguration[configIndex].configValue, 
                                                                   Common_Result[result]);
                return -1;
            }
        }
    }

    printf ("\n Demodulator optimised for %s tuner.\n\n", Common_TunerOptimize[demod.tunerOptimize]);

    /* ---------------------------------------------------------------------------------
     * Tune
     * ------------------------------------------------------------------------------ */
    printf ("------------------------------------------\n");
    printf (" Tune   \n");
    printf ("------------------------------------------\n");

    /* Configure the DVBT2 tune parameters based on the channel requirements */
    tuneParam.bandwidth = SONY_DEMOD_BW_8_MHZ;          /* Channel bandwidth */
    tuneParam.centerFreqKHz = 666000;                   /* Channel center frequency in KHz */
    tuneParam.dataPlpId = 0;                            /* PLP ID where multiple PLP's are available */
    tuneParam.profile = SONY_DVBT2_PROFILE_BASE;        /* Channel profile is T2-Base */
    /* Additional tune information fed back from the driver.  This parameter should be checked
       if the result from the tune call is SONY_RESULT_OK_CONFIRM. */
    tuneParam.tuneInfo = SONY_DEMOD_DVBT2_TUNE_INFO_OK; 

    printf (" Tune to DVB-T2 signal with the following parameters:\n");
    printf ("  - Center Freq    : %uKHz\n", tuneParam.centerFreqKHz);
    printf ("  - Bandwidth      : %s\n", Common_Bandwidth[tuneParam.bandwidth]);
    printf ("  - PLP ID         : %u\n", tuneParam.dataPlpId);
    printf ("  - Profile        : %s\n", DVBT2_Profile[tuneParam.profile]);

    /* Perform DVBT2 Tune */
    tuneResult = sony_integ_dvbt2_Tune (&integ, &tuneParam);
    printf ("  - Result         : %s\n", Common_Result[tuneResult]);
    printf ("  - Tune Info      : %s\n\n", DVBT2_TuneInfo[tuneParam.tuneInfo]);

    /* ---------------------------------------------------------------------------------
     * Confirm PLP ID
     * ------------------------------------------------------------------------------ */
    /* If the tune result = SONY_RESULT_OK_CONFIRM it means that the demodulator has acquired
       a channel at the provided frequency and bandwidth but there was an issue with the PLP
       selection.  If no matching PLP is found the demodulator will provide TS lock to the
       first entry in the L1 table */
    if ((tuneResult == SONY_RESULT_OK_CONFIRM) && (tuneParam.tuneInfo == SONY_DEMOD_DVBT2_TUNE_INFO_INVALID_PLP_ID)) {
        sony_dvbt2_plp_t activePlpInfo;

        printf (" PLP ID error in acquisition:\n");

        result = sony_demod_dvbt2_monitor_ActivePLP (integ.pDemod, SONY_DVBT2_PLP_DATA, & activePlpInfo);
        if (result != SONY_RESULT_OK) {
            printf (" Error: Unable to monitor T2 Active PLP. (result = %s)\n", Common_Result[result]);
            return -1;
        }

        printf ("  - PLP Requested : %u\n", tuneParam.dataPlpId);
        printf ("  - PLP Acquired  : %u\n\n", activePlpInfo.id);
    }

    /* ---------------------------------------------------------------------------------
     * Carrier Offset Compensation
     * ------------------------------------------------------------------------------ */
    /* Measure the current carrier offset and retune to compensate for cases outside the demodulator
     * acquisition range. */
    if ((tuneResult == SONY_RESULT_ERROR_TIMEOUT) || (tuneResult == SONY_RESULT_OK) || (tuneResult == SONY_RESULT_OK_CONFIRM)) {
        int32_t offsetHz = 0;
        uint32_t stepHz = SONY_EXAMPLE_TUNER_OFFSET_CUTOFF;

        /* Monitor carrier offset. */
        result = sony_demod_dvbt2_monitor_CarrierOffset (integ.pDemod, &offsetHz);
        if (result != SONY_RESULT_OK) {
            printf ("Error: Unable to monitor T2 carrier offset. (result = %s)\n", Common_Result[result]);
            return -1;
        }

        printf (" DVB-T2 carrier offset of %ldHz detected.\n", offsetHz);

        /* Carrier recovery loop locked (demod locked), compensate for the offset and retry tuning. */
        stepHz = (stepHz + 1) / 2;
        if ((uint32_t) abs (offsetHz) > stepHz) {
            /* Tuners have only a fixed frequency step size (stepkHz), therefore we must query the tuner driver to get the actual
             * center frequency set by the tuner. */
            tuneParam.centerFreqKHz = (uint32_t) ((int32_t) integ.pTunerTerrCable->frequencyKHz + ((offsetHz + 500) / 1000));

            printf (" Re-tuning to compensate offset. New parameters:\n");
            printf ("  - Center Freq    : %uKHz\n", tuneParam.centerFreqKHz);
            printf ("  - Bandwidth      : %s\n", Common_Bandwidth[tuneParam.bandwidth]);
            printf ("  - PLP ID         : %u\n", tuneParam.dataPlpId);
            /* Perform DVBT2 Tune */
            tuneResult = sony_integ_dvbt2_Tune (&integ, &tuneParam);
            printf ("  - Result         : %s\n", Common_Result[tuneResult]);
            printf ("  - Tune Info      : %s\n\n", DVBT2_TuneInfo[tuneParam.tuneInfo]);
        }
        else {
            printf (" Carrier offset compensation not required.\n");
        }
    }
    if ((tuneResult != SONY_RESULT_OK) && (tuneResult != SONY_RESULT_OK_CONFIRM)) {
        printf (" Error: Unable to get TS lock DVB-T2 signal at %lukHz. (status=%d, %s)\n", tuneParam.centerFreqKHz, tuneResult, Common_Result[result]);
        return -1;
    }

    printf (" TS locked to DVB-T2 signal at %lukHz.\n\n", tuneParam.centerFreqKHz);

    /* ---------------------------------------------------------------------------------
     * Monitor the Channel
     * ------------------------------------------------------------------------------ */
    printf ("------------------------------------------\n");
    printf (" Monitor   \n");
    printf ("------------------------------------------\n");

    /* Allow the monitors time to settle */
    SONY_SLEEP (1000);

    for (i = 1; i <= MONITOR_LOOP_COUNT; i++) {

    printf ("\n Monitor Iteration : %u \n\n",i);
    printf ("-------------------------|-----------------|----------------- \n");
    printf ("      MONITOR NAME       |    PARAMETER    |      VALUE       \n");
    printf ("-------------------------|-----------------|----------------- \n");

#ifdef MONITOR_SYNCSTAT        
        {
            uint8_t syncState = 0;
            uint8_t tsLock = 0;
            uint8_t earlyUnlock = 0;
            result = sony_demod_dvbt2_monitor_SyncStat (integ.pDemod, &syncState, &tsLock, &earlyUnlock);
            if (result == SONY_RESULT_OK) {
                printf (" SyncStat                | SyncStat        | %lu\n", syncState);
                printf ("                         | TS Lock         | %s\n", Common_YesNo[tsLock]);
                printf ("                         | Early Unlock    | %s\n", Common_YesNo[earlyUnlock]);
            }
            else {
                printf (" SyncStat                | Error           | %s\n", Common_Result[result]);
            }    
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_CARRIEROFFSET
        {
            int32_t offset;
            result = sony_demod_dvbt2_monitor_CarrierOffset (integ.pDemod, &offset);
            if (result == SONY_RESULT_OK) {
                printf (" CarrierOffset           | Carrier Offset  | %dHz\n", offset);
            }
            else {
                printf (" CarrierOffset           | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_IFAGCOUT
        {
            uint32_t ifAGCOut = 0;
            result = sony_demod_dvbt2_monitor_IFAGCOut (integ.pDemod, &ifAGCOut);
            if (result == SONY_RESULT_OK) {
                printf (" IFAGCOut                | IF AGC          | %lu\n", ifAGCOut);
            }
            else {
                printf (" IFAGCOut                | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_L1PRE
        {
            sony_dvbt2_l1pre_t l1Pre;
            result = sony_demod_dvbt2_monitor_L1Pre (integ.pDemod, &l1Pre);
            if (result == SONY_RESULT_OK) {
                printf (" L1Pre                   | Type            | %s\n", DVBT2_L1PreType[l1Pre.type]);
                printf ("                         | Extended BW     | %s\n", Common_YesNo[l1Pre.bwExt]);
                printf ("                         | S1 Signalling   | %s\n", DVBT2_S1Signalling[l1Pre.s1]);
                printf ("                         | S2 Signalling   | %u\n", l1Pre.s2);
                printf ("                         | Mixed Indicator | %s\n", Common_YesNo[l1Pre.mixed]);
                printf ("                         | FFT Mode        | %s\n", DVBT2_FFTMode[l1Pre.fftMode]);
                printf ("                         | L1 Repeat       | %s\n", Common_YesNo[l1Pre.l1Rep]);
                printf ("                         | Guard Interval  | %s\n", DVBT2_GaurdInterval[l1Pre.gi]);
                printf ("                         | PAPR            | %s\n", DVBT2_PAPR[l1Pre.papr]);
                printf ("                         | L1 Modulation   | %s\n", DVBT2_L1PostModulation[l1Pre.mod]);
                printf ("                         | L1 Code Rate    | %s\n", DVBT2_L1PostCodeRate[l1Pre.cr]);
                printf ("                         | L1 FEC Type     | %s\n", DVBT2_L1PostFEC[l1Pre.fec]);
                printf ("                         | L1 Size         | %u\n", l1Pre.l1PostSize);
                printf ("                         | L1 Info Size    | %u\n", l1Pre.l1PostInfoSize);
                printf ("                         | Pilot Pattern   | %s\n", DVBT2_PPMode[l1Pre.pp]);
                printf ("                         | Tx ID           | %u\n", l1Pre.txIdAvailability);
                printf ("                         | T2 Cell ID      | %u\n", l1Pre.cellId);
                printf ("                         | T2 Network ID   | %u\n", l1Pre.networkId);
                printf ("                         | T2 System ID    | %u\n", l1Pre.systemId);
                printf ("                         | Frames/S.Frames | %u\n", l1Pre.numFrames);
                printf ("                         | OFDM Sym/Frame  | %u\n", l1Pre.numSymbols);
                printf ("                         | Regeneration    | %s\n", Common_YesNo[l1Pre.regen]);
                printf ("                         | L1 Extensions   | %s\n", Common_YesNo[l1Pre.postExt]);
                printf ("                         | RF Freq No.     | %u\n", l1Pre.numRfFreqs);
                printf ("                         | RF Index        | %u\n", l1Pre.rfIdx);
                printf ("                         | T2 Version      | %s\n", DVBT2_Version[l1Pre.t2Version]);
                printf ("                         | L1 Scrambled    | %s\n", Common_YesNo[l1Pre.l1PostScrambled]);
                printf ("                         | Base-Lite       | %s\n", Common_YesNo[l1Pre.t2BaseLite]);
                printf ("                         | CRC-32 of L1Pre | %u\n", l1Pre.crc32);
            }
            else {
                printf (" L1Pre                   | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_VERSION
        {
            sony_dvbt2_version_t version;
            result = sony_demod_dvbt2_monitor_Version (integ.pDemod, &version);
            if (result == SONY_RESULT_OK) {
                printf (" Version                 | Version         | %s\n", DVBT2_Version[version]);
            }
            else {
                printf (" Version                 | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_OFDM
        {
            sony_dvbt2_ofdm_t ofdm;
            result = sony_demod_dvbt2_monitor_Ofdm (integ.pDemod, &ofdm);
            if (result == SONY_RESULT_OK) {
                printf (" Ofdm                    | Mixed           | %s\n", DVBT2_L1PreType[ofdm.mixed]);
                printf ("                         | Is MISO         | %s\n", Common_YesNo[ofdm.isMiso]);
                printf ("                         | FFT Mode        | %s\n", DVBT2_FFTMode[ofdm.mode]);
                printf ("                         | Guard Interval  | %s\n", DVBT2_GaurdInterval[ofdm.gi]);
                printf ("                         | Pilot Pattern   | %s\n", DVBT2_PPMode[ofdm.pp]);
                printf ("                         | Extended BW     | %s\n", Common_YesNo[ofdm.bwExt]);
                printf ("                         | PAPR            | %u\n", ofdm.papr);
                printf ("                         | OFDM Sym/Frame  | %u\n", ofdm.numSymbols);
            }
            else {
                printf (" Ofdm                    | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_DATAPLPS
        {
            uint8_t plpCount;
            uint8_t plpList[256];
            uint8_t i;

            result = sony_demod_dvbt2_monitor_DataPLPs (integ.pDemod, &plpList[0], &plpCount);
            if (result == SONY_RESULT_OK) {
                printf (" DataPLPs                | Count           | %u\n", plpCount);
                for (i = 0; i<plpCount; i++) {
                    printf ("                         | PLP %u ID        | %u\n", i, *(plpList + i));
                }
            }
            else {
                printf (" DataPLPs                | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_ACTIVEPLP
        {
            sony_dvbt2_plp_t plp;
            result = sony_demod_dvbt2_monitor_ActivePLP (integ.pDemod, SONY_DVBT2_PLP_DATA, &plp);
            if (result == SONY_RESULT_OK) {
                printf (" ActivePLP               | ID              | %u\n", plp.id);
                printf ("                         | Type            | %s\n", DVBT2_PLPType[plp.type]);
                printf ("                         | Payload         | %s\n", DVBT2_PLPPayload[plp.payload]);
                printf ("                         | FF              | %u\n", plp.ff);
                printf ("                         | First RF IDx    | %u\n", plp.firstRfIdx);
                printf ("                         | First Frame IDx | %u\n", plp.firstFrmIdx);
                printf ("                         | Group ID        | %u\n", plp.groupId);
                printf ("                         | Modulation      | %s\n", DVBT2_PLPModulation[plp.constell]);
                printf ("                         | Code Rate       | %s\n", DVBT2_CodeRate[plp.plpCr]);
                printf ("                         | Rotation        | %u\n", plp.rot);
                printf ("                         | FEC Type        | %s\n", DVBT2_FEC[plp.fec]);
                printf ("                         | Max PLP Blocks  | %u\n", plp.numBlocksMax);
                printf ("                         | Frame Interval  | %u\n", plp.frmInt);
                printf ("                         | TI Length       | %u\n", plp.tilLen);
                printf ("                         | TI Type         | %u\n", plp.tilType);
                printf ("                         | IBS Flag        | %u\n", plp.inBandBFlag);
                printf ("                         | Issy Indicator  | %u\n", plp.issy);
                printf ("                         | Null Packets    | %u\n", plp.npd);
                printf ("                         | PLP Mode        | %u\n", plp.plpMode);
                printf ("                         | RSVD            | %u\n", plp.rsvd);
                printf ("                         | InBand B Flag   | %u\n", plp.inBandBFlag);
                printf ("                         | InBand B TSRate | %u\n", plp.inBandBTSRate);
            }
            else {
                printf (" ActivePLP               | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_DATAPLPERROR
        {
            uint8_t plpError = 0;
            result = sony_demod_dvbt2_monitor_DataPLPError (integ.pDemod, &plpError);
            if (result == SONY_RESULT_OK) {
                printf (" DataPLPError            | Data PLP Error  | %s\n", Common_YesNo[plpError]);
            }
            else {
                printf (" DataPLPError            | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_L1CHANGE
        {
            uint8_t l1Change = 0;
            result = sony_demod_dvbt2_monitor_L1Change (integ.pDemod, &l1Change);
            if (result == SONY_RESULT_OK) {
                printf (" L1Change                | L1 Change       | %s\n", Common_YesNo[l1Change]);
            }
            else {
                printf (" L1Change                | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_L1POST
        {
            sony_dvbt2_l1post_t l1post;
            result = sony_demod_dvbt2_monitor_L1Post (integ.pDemod, &l1post);
            if (result == SONY_RESULT_OK) {
                printf (" L1Post                  | SubSlices/Frame | %u\n", l1post.subSlicesPerFrame);
                printf ("                         | No. PLP's       | %u\n", l1post.numPLPs);
                printf ("                         | No. Aux's       | %u\n", l1post.numAux);
                printf ("                         | Aux Config      | %u\n", l1post.auxConfigRFU);
                printf ("                         | RF Index        | %u\n", l1post.rfIdx);
                printf ("                         | Freq RF Index   | %u\n", l1post.freq);
                printf ("                         | FEF Type        | %u\n", l1post.fefType);
                printf ("                         | FEF Length      | %u\n", l1post.fefLength);
                printf ("                         | FEF Interval    | %u\n", l1post.fefInterval);
            }
            else {
                printf (" L1Post                  | Error           | %s\n", Common_Result[result]);
            }   
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_SPECTRUMSENSE
        {
            sony_demod_terr_cable_spectrum_sense_t sense;
            result = sony_demod_dvbt2_monitor_SpectrumSense (integ.pDemod, &sense);
            if (result == SONY_RESULT_OK) {
                printf (" SpectrumSense           | Spectrum Sense  | %s\n", Common_SpectrumSense[sense]);
            }
            else {
                printf (" SpectrumSense           | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_SNR
        {
            int32_t snr;
            result = sony_demod_dvbt2_monitor_SNR (integ.pDemod, &snr);
            if (result == SONY_RESULT_OK) {
                printf (" SNR                     | SNR             | %ddB x 10^3\n", snr);
            }
            else {
                printf (" SNR                     | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PRELDPCBER
        {
            uint32_t ber;
            result = sony_demod_dvbt2_monitor_PreLDPCBER (integ.pDemod, &ber);
            if (result == SONY_RESULT_OK) {
                printf (" PreLDPCBER              | Pre-LDPC BER    | %u x 10^-7\n", ber);
            }
            else {
                printf (" PreLDPCBER              | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_POSTBCHFER
        {
            uint32_t fer;
            result = sony_demod_dvbt2_monitor_PostBCHFER (integ.pDemod, &fer);
            if (result == SONY_RESULT_OK) {
                printf (" PostBCHBER              | Post-BCH BER    | %u x 10^-6\n", fer);
            }
            else {
                printf (" PostBCHBER              | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PREBCHBER
        {
            uint32_t ber;
            result = sony_demod_dvbt2_monitor_PreBCHBER (integ.pDemod, &ber);
            if (result == SONY_RESULT_OK) {
                printf (" PreBCHBER               | Pre-BCH BER     | %u x 10^-9\n", ber);
            }
            else {
                printf (" PreBCHBER               | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PACKETERRORNUMBER
        {
            uint32_t pen;
            result = sony_demod_dvbt2_monitor_PacketErrorNumber (integ.pDemod, &pen);
            if (result == SONY_RESULT_OK) {
                printf (" PacketErrorNumber       | PEN             | %u\n", pen);
            }
            else {
                printf (" PacketErrorNumber       | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PPMOFFSET
        {
            int32_t ppmOffset;
            result = sony_demod_dvbt2_monitor_SamplingOffset (integ.pDemod, &ppmOffset);
            if (result == SONY_RESULT_OK) {
                printf (" SamplingOffset          | PPM Offset      | %d\n", ppmOffset);
            }
            else {
                printf (" SamplingOffset          | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_TSRATE
        {
            uint32_t tsRate;
            result = sony_demod_dvbt2_monitor_TsRate (integ.pDemod, &tsRate);
            if (result == SONY_RESULT_OK) {
                printf (" TsRate                  | TS Data Rate    | %u\n", tsRate);
            }
            else {
                printf (" TsRate                  | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_QUALITY
        {
            uint8_t quality;
            result = sony_demod_dvbt2_monitor_Quality (integ.pDemod, &quality);
            if (result == SONY_RESULT_OK) {
                printf (" Quality                 | SQI             | %u\n", quality);
            }
            else {
                printf (" Quality                 | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PER
        {
            uint32_t per;
            result = sony_demod_dvbt2_monitor_PER (integ.pDemod, &per);
            if (result == SONY_RESULT_OK) {
                printf (" PER                     | PER             | %u x 10^-6\n", per);
            }
            else {
                printf (" PER                     | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PROFILE
        {
            sony_dvbt2_profile_t profile;
            result = sony_demod_dvbt2_monitor_Profile (&demod, &profile);
            if (result == SONY_RESULT_OK) {
                printf (" Profile                 | Profile         | %s\n", DVBT2_Profile[profile]);
            }
            else {
                printf (" Profile                 | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_SSI
        {
            uint8_t strength;
            result = sony_integ_dvbt2_monitor_SSI (&integ, &strength);
            if (result == SONY_RESULT_OK) {
                printf (" SSI                     | Signal Strength | %u\n", strength);
            }
            else {
                printf (" SSI                     | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_RFLEVEL
        {
            int32_t rfLevel;
            result = sony_integ_dvbt_t2_monitor_RFLevel (&integ, &rfLevel);
            if (result == SONY_RESULT_OK) {
                printf (" RF Level                | RF Level        | %ddB x 10^3\n", rfLevel);
            }
            else {
                printf (" RF Level                | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

    printf ("\nAveraged SNR and SQI\n");
    printf ("-------------------------|-----------------|----------------- \n");
    printf ("      MONITOR NAME       |    PARAMETER    |      VALUE       \n");
    printf ("-------------------------|-----------------|----------------- \n");

#ifdef MONITOR_SNR
        {
            int32_t snr;
            result = sony_integ_dvbt2_monitor_AveragedSNR (&integ, &snr);
            if (result == SONY_RESULT_OK) {
                printf (" Averaged SNR            | SNR             | %ddB x 10^3\n", snr);
            }
            else {
                printf (" Averaged SNR            | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_QUALITY
        {
            uint8_t quality;
            result = sony_integ_dvbt2_monitor_AveragedQuality (&integ, &quality);
            if (result == SONY_RESULT_OK) {
                printf (" Averaged Quality        | SQI             | %u\n", quality);
            }
            else {
                printf (" Averaged Quality        | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

        SONY_SLEEP (1000);
    }

    /*------------------------------------------------------------------------------
     Closing / Shutdown
    ------------------------------------------------------------------------------*/
    /* Shutdown the demodulator and tuner parts, placing them into a low power mode.  After this
     * operation the driver will be in SONY_DEMOD_STATE_SHUTDOWN state.  From this state you can 
     * call sony_integ_SleepT_C to return to Terrestrial / Cable mode, or sony_integ_SleepS to
     * transition to Satellite operation.  Both Sleep API's will load the demodulator configuration
     * from memory to retain functionality from before Shutdown.  
     * 
     * Note : If sony_integ_InitializeT_C or sony_integ_InitializeS are called the configuration 
     * memory will be cleared and the demod will perform a SW reset of all device registers.
     * This method of returning from Shutdown state is not recommended. */
    printf ("\n------------------------------------------\n");
    printf (" Shutdown / Finalize                      \n");
    printf ("------------------------------------------\n");

    result = sony_integ_Shutdown (&integ);
    if (result == SONY_RESULT_OK) {
        printf (" Demodulator and tuner put into Shutdown state.\n");
    }
    else {
        printf (" Error: Unable to shutdown the integration driver. (result = %s)\n", Common_Result[result]);
        return -1;
    }

    /* Finalise the I2C */
    result = drvi2c_feusb_Finalize (&feusb);
    if (result == SONY_RESULT_OK) {
        printf (" I2C driver instance closed.\n");
    }
    else {
        printf (" Error: Unable to finalize FEUSB I2C driver. (result = %s)\n", Common_Result[result]);
        return -1;
    }

    return 0;
}

/* 
 @brief Monitors 100 SNR samples at 20ms intervals to obtain an averaged SNR
        with improved stability over the instantaneous monitor.

 @note This monitor is thread blocking for 2s.

 @param pInteg The driver instance.
 @param pSNR 100 sample avearged SNR in dB*1000.

 @return SONY_DVB_OK if successful.
 */
static sony_result_t sony_integ_dvbt2_monitor_AveragedSNR(sony_integ_t * pInteg, int32_t * pSNR)
{
    sony_result_t result = SONY_RESULT_OK;

    /* The sample count can be modified to suit the application requirements.
     * 100 is suggested for a stable response with a 2s measurement period.
     *
     * Measurement period(ms) = sampleCount * 20
     *
     * Valid range : 2 <= sampleCount <= 400
     */
    uint32_t sampleCount = 100;

    SONY_TRACE_ENTER ("sony_integ_dvbt2_monitor_AveragedSNR");

    if ((!pInteg) || (!pSNR)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Software state check */
    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE T/C state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if (pInteg->pDemod->system != SONY_DTV_SYSTEM_DVBT2) {
        /* Not DVB-T */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Check sampleCount range for validity */
    if ((sampleCount < 2) || (sampleCount > 400)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_RANGE);
    }

    {
        int32_t totalSNR = 0;
        int32_t currentSNR = 0;
        uint8_t i = 0;

        for (i = 0 ; i < sampleCount ; i++) {
            /* Monitor instantaneous SNR */
            result = sony_demod_dvbt2_monitor_SNR(pInteg->pDemod, &currentSNR);
            if (result != SONY_RESULT_OK) {
                SONY_TRACE_RETURN (result);
            }

            totalSNR += currentSNR;

            /* Sleep for 20ms between measurements */
            SONY_SLEEP(20);
        }

        *pSNR = (totalSNR + (sampleCount / 2)) / sampleCount;
    }
    return (result);
}

/**
 @brief Monitor the DVB-T2 quality metric using the 100 point avergaed 
        SNR monitor. Based on Nordig SQI equation.  THis monitor offers
        improved stability over the instantaneous value.
 
 @note This monitor is thread blocking for 2s.

 @param pDemod The demodulator instance.
 @param pQuality The quality as a percentage (0-100).

 @return SONY_RESULT_OK if successful and pQuality valid.
*/
static sony_result_t sony_integ_dvbt2_monitor_AveragedQuality (sony_integ_t * pInteg, 
                                                               uint8_t * pQuality)
{
    sony_result_t result = SONY_RESULT_OK;
    int32_t snr = 0;
    int32_t snrRel = 0;
    uint32_t ber = 0;
    uint32_t berSQI = 0;
    sony_dvbt2_plp_constell_t qam;
    sony_dvbt2_plp_code_rate_t codeRate;

    static const int32_t snrNordigP1dB1000[4][8] = {
    /*   1/2,   3/5,    2/3,    3/4,    4/5,    5/6,   1/3,   2/5                */
        {3500,  4700,   5600,   6600,   7200,   7700,  1300,  2200 }, /* QPSK    */
        {8700,  10100,  11400,  12500,  13300,  13800, 6000,  7200 }, /* 16-QAM  */
        {13000, 14800,  16200,  17700,  18700,  19400, 9800,  11100}, /* 64-QAM  */
        {17000, 19400,  20800,  22900,  24300,  25100, 13200, 14800}, /* 256-QAM */
    };

    SONY_TRACE_ENTER ("sony_integ_dvbt2_monitor_AveragedQuality");

    if ((!pInteg) || (!pQuality)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Software state check */
    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if (pInteg->pDemod->system != SONY_DTV_SYSTEM_DVBT2) {
        /* Not DVB-T2 */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Get Pre BCH BER. */
    result = sony_demod_dvbt2_monitor_PreBCHBER (pInteg->pDemod, &ber);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

     /* Get SNR */
    result = sony_integ_dvbt2_monitor_AveragedSNR (pInteg, &snr);
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

    /* BER_SQI Calculated from:
     * if (Pre-BCH BER > 10^-4)            BER_SQI = 0
     * if (10^-4 >= Pre-BCH BER >= 10^-7)  BER_SQI = 100/15 = 6.667
     * if (Pre-BCH BER < 10^-7)            BER_SQI = 100/6  = 16.667
     *
     * Note : Pre-BCH BER is scaled by 10^9
     */
    if (ber > 100000) {
        berSQI = 0;
    } 
    else if (ber >= 100) {
        berSQI = 6667;
    } 
    else {
        berSQI = 16667;
    }

    /* C/Nrel = C/Nrec - C/Nnordigp1 */
    snrRel = snr - snrNordigP1dB1000[qam][codeRate];

    /* SQI (Signal Quality Indicator) given by:
     * if (C/Nrel < -3dB)         SQI = 0
     * if (-3dB <= CNrel <= 3dB)  SQI = (C/Nrel + 3) * BER_SQI 
     * if (CNrel > 3dB)           SQI = 100
     */
    if (snrRel < -3000) {
        *pQuality = 0;
    } 
    else if (snrRel <= 3000) {
        /* snrRel and berSQI scaled by 10^3 so divide by 10^6 */
        uint32_t tempSQI = (((snrRel + 3000) * berSQI) + 500000) / 1000000;
        /* Clip value to 100% */
        *pQuality = (tempSQI > 100)? 100 : (uint8_t) tempSQI;
    } 
    else {
        *pQuality = 100;
    }

    SONY_TRACE_RETURN (result);
}

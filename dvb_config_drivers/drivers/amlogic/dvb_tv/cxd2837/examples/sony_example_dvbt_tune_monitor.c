/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2013-03-22 14:36:23 #$
  File Revision : $Revision:: 6794 $
------------------------------------------------------------------------------*/
/**
 @file    sony_example_dvbt_tune_monitor.c

         This file provides an example of DVB-T tuning and looped monitoring
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
#include "sony_demod_dvbt.h"            /* DVBT type definitions and demodulator functions */
#include "sony_integ_dvbt_t2.h"         /* DVBT integration layer functions (Tune, Scan, RF Level monitor etc)*/
#include "sony_demod_dvbt_monitor.h"    /* DVBT monitor functions */
#include "drvi2c_feusb.h"               /* USB->I2C driver implementation. */
#include "sony_math.h"                  /* Common math library functions. */
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
#define MONITOR_IFAGCOUT
#define MONITOR_MODEGUARD
#define MONITOR_CARRIEROFFSET
#define MONITOR_PREVITERBIBER
#define MONITOR_PRERSBER
#define MONITOR_TPSINFO
#define MONITOR_PACKETERRORNUMBER
#define MONITOR_SPECTRUMSENSE
#define MONITOR_SNR
#define MONITOR_PPMOFFSET
#define MONITOR_QUALITY
#define MONITOR_PER
#define MONITOR_SSI
#define MONITOR_RFLEVEL

/*------------------------------------------------------------------------------
 Const char definitions
------------------------------------------------------------------------------*/
static const char *DVBT_Constellation[] = { "QPSK", "16-QAM", "64-QAM" };
static const char *DVBT_Hier[] = { "Non", "A1", "A2", "A4" };
static const char *DVBT_CodeRate[] = { "1/2", "2/3", "3/4", "5/6", "7/8" };
static const char *DVBT_GI[] = { "1/32", "1/16", "1/8", "1/4" };
static const char *DVBT_Mode[] = { "2K", "8K" };
static const char *DVBT_Profile[] = { "HP", "LP" };

/*------------------------------------------------------------------------------
 Static Function Prototypes
 ------------------------------------------------------------------------------*/
static sony_result_t sony_integ_dvbt_monitor_AveragedSNR (sony_integ_t * pInteg, int32_t * pSNR);
static sony_result_t sony_integ_dvbt_monitor_AveragedQuality (sony_integ_t * pInteg, uint8_t * pQuality);

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
    sony_dvbt_tune_param_t tuneParam;
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

    /* Configure the DVBT tune parameters based on the channel requirements */
    tuneParam.bandwidth = SONY_DEMOD_BW_8_MHZ;          /* Channel bandwidth */
    tuneParam.centerFreqKHz = 666000;                   /* Channel centre frequency in KHz */
    tuneParam.profile = SONY_DVBT_PROFILE_HP;           /* Channel profile for hierachical modes.  For non-hierachical use HP */

    printf (" Tune to DVB-T signal with the following parameters:\n");
    printf ("  - Center Freq    : %uKHz\n", tuneParam.centerFreqKHz);
    printf ("  - Bandwidth      : %s\n", Common_Bandwidth[tuneParam.bandwidth]);
    printf ("  - Profile        : %s\n", DVBT_Profile[tuneParam.profile]);

    /* Perform DVBT Tune */
    tuneResult = sony_integ_dvbt_Tune (&integ, &tuneParam);
    printf ("  - Result         : %s\n\n", Common_Result[tuneResult]);
    /* ---------------------------------------------------------------------------------
     * Carrier Offset Compensation
     * ------------------------------------------------------------------------------ */
    /* Measure the current carrier offset and retune to compensate for cases outside the demodulator
     * acquisition range. */
    if ((tuneResult == SONY_RESULT_ERROR_TIMEOUT) || (tuneResult == SONY_RESULT_OK)) {
        int32_t offsetHz = 0;
        uint32_t stepHz = SONY_EXAMPLE_TUNER_OFFSET_CUTOFF;

        /* Monitor carrier offset. */
        result = sony_demod_dvbt_monitor_CarrierOffset (integ.pDemod, &offsetHz);
        if (result != SONY_RESULT_OK) {
            printf ("Error: Unable to monitor T carrier offset. (result = %s)\n", Common_Result[result]);
            return -1;
        }

        printf (" DVB-T carrier offset of %ldHz detected.\n", offsetHz);

        /* Carrier recovery loop locked (demod locked), compensate for the offset and retry tuning. */
        stepHz = (stepHz + 1) / 2;
        if ((uint32_t) abs (offsetHz) > stepHz) {
            /* Tuners have only a fixed frequency step size (stepkHz), therefore we must query the tuner driver to get the actual
             * center frequency set by the tuner. */
            tuneParam.centerFreqKHz = (uint32_t) ((int32_t) integ.pTunerTerrCable->frequencyKHz + ((offsetHz + 500) / 1000));

            printf (" Re-tuning to compensate offset. New parameters:\n");
            printf ("  - Center Freq    : %uKHz\n", tuneParam.centerFreqKHz);
            printf ("  - Bandwidth      : %s\n", Common_Bandwidth[tuneParam.bandwidth]);
            printf ("  - Profile        : %s\n", DVBT_Profile[tuneParam.profile]);
            tuneResult = sony_integ_dvbt_Tune (&integ, &tuneParam);
            printf ("  - Result         : %s\n\n", Common_Result[tuneResult]);
        }
        else {
            printf (" Carrier offset compensation not required.\n");
        }
    }
    if (tuneResult != SONY_RESULT_OK) {
        printf (" Error: Unable to get TS lock DVB-T signal at %lukHz. (status=%d, %s)\n", tuneParam.centerFreqKHz, tuneResult, Common_Result[result]);
        return -1;
    }

    printf (" TS locked to DVB-T signal at %lukHz.\n\n", tuneParam.centerFreqKHz);

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
            result = sony_demod_dvbt_monitor_SyncStat (integ.pDemod, &syncState, &tsLock, &earlyUnlock);
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

#ifdef MONITOR_IFAGCOUT
        {
            uint32_t ifAGCOut = 0;
            result = sony_demod_dvbt_monitor_IFAGCOut (integ.pDemod, &ifAGCOut);
            if (result == SONY_RESULT_OK) {
                printf (" IFAGCOut                | IF AGC          | %lu\n", ifAGCOut);
            }
            else {
                printf (" IFAGCOut                | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_MODEGUARD
        {
            sony_dvbt_mode_t mode;
            sony_dvbt_guard_t guard;
            result = sony_demod_dvbt_monitor_ModeGuard (integ.pDemod, &mode, &guard);
            if (result == SONY_RESULT_OK) {
                printf (" ModeGuard               | Mode            | %s\n", DVBT_Mode[mode]);
                printf ("                         | Guard Interval  | %s\n", DVBT_GI[guard]);
            }
            else {
                printf (" ModeGuard               | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_CARRIEROFFSET
        {
            int32_t offset;
            result = sony_demod_dvbt_monitor_CarrierOffset (integ.pDemod, &offset);
            if (result == SONY_RESULT_OK) {
                printf (" CarrierOffset           | Carrier Offset  | %dHz\n", offset);
            }
            else {
                printf (" CarrierOffset           | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PREVITERBIBER
        {
            uint32_t ber;
            result = sony_demod_dvbt_monitor_PreViterbiBER (integ.pDemod, &ber);
            if (result == SONY_RESULT_OK) {
                printf (" PreViterbiBER           | Pre-Viterbi BER | %u x 10^-7\n", ber);
            }
            else {
                printf (" PreViterbiBER           | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PRERSBER
        {
            uint32_t ber;
            result = sony_demod_dvbt_monitor_PreRSBER (integ.pDemod, &ber);
            if (result == SONY_RESULT_OK) {
                printf (" PreRSBER                | Pre-RS BER      | %u x 10^-7\n", ber);
            }
            else {
                printf (" PreRSBER                | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_TPSINFO
        {
            sony_dvbt_tpsinfo_t tpsInfo;
            result = sony_demod_dvbt_monitor_TPSInfo (integ.pDemod, &tpsInfo);
            if (result == SONY_RESULT_OK) {
                printf (" TPSInfo                 | Constellation   | %s\n", DVBT_Constellation[tpsInfo.constellation]);
                printf ("                         | Hierachy        | %s\n", DVBT_Hier[tpsInfo.hierarchy]);
                printf ("                         | Code Rate HP    | %s\n", DVBT_CodeRate[tpsInfo.rateHP]);
                printf ("                         | Code Rate LP    | %s\n", DVBT_CodeRate[tpsInfo.rateLP]);
                printf ("                         | Guard Interval  | %s\n", DVBT_GI[tpsInfo.guard]);
                printf ("                         | Mode            | %s\n", DVBT_Mode[tpsInfo.mode]);
            }
            else {
                printf (" TPSInfo                 | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PACKETERRORNUMBER
        {
            uint32_t pen;
            result = sony_demod_dvbt_monitor_PacketErrorNumber (integ.pDemod, &pen);
            if (result == SONY_RESULT_OK) {
                printf (" PacketErrorNumber       | PEN             | %u\n", pen);
            }
            else {
                printf (" PacketErrorNumber       | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_SPECTRUMSENSE
        {
            sony_demod_terr_cable_spectrum_sense_t sense;
            result = sony_demod_dvbt_monitor_SpectrumSense (integ.pDemod, &sense);
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
            result = sony_demod_dvbt_monitor_SNR (integ.pDemod, &snr);
            if (result == SONY_RESULT_OK) {
                printf (" SNR                     | SNR             | %ddB x 10^3\n", snr);
            }
            else {
                printf (" SNR                     | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_PPMOFFSET
        {
            int32_t ppmOffset;
            result = sony_demod_dvbt_monitor_SamplingOffset (integ.pDemod, &ppmOffset);
            if (result == SONY_RESULT_OK) {
                printf (" SamplingOffset          | PPM Offset      | %d\n", ppmOffset);
            }
            else {
                printf (" SamplingOffset          | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_QUALITY
        {
            uint8_t quality;
            result = sony_demod_dvbt_monitor_Quality (integ.pDemod, &quality);
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
            result = sony_demod_dvbt_monitor_PER (integ.pDemod, &per);
            if (result == SONY_RESULT_OK) {
                printf (" PER                     | PER             | %u x 10^-6\n", per);
            }
            else {
                printf (" PER                     | Error           | %s\n", Common_Result[result]);
            }  
            printf ("-------------------------|-----------------|----------------- \n");
        }
#endif

#ifdef MONITOR_SSI
        {
            uint8_t strength;
            result = sony_integ_dvbt_monitor_SSI (&integ, &strength);
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
            result = sony_integ_dvbt_monitor_AveragedSNR (&integ, &snr);
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
            result = sony_integ_dvbt_monitor_AveragedQuality (&integ, &quality);
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
static sony_result_t sony_integ_dvbt_monitor_AveragedSNR(sony_integ_t * pInteg, int32_t * pSNR)
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

    SONY_TRACE_ENTER ("sony_integ_dvbt_monitor_AveragedSNR");

    if ((!pInteg) || (!pSNR)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Software state check */
    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE T/C state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if (pInteg->pDemod->system != SONY_DTV_SYSTEM_DVBT) {
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
            result = sony_demod_dvbt_monitor_SNR(pInteg->pDemod, &currentSNR);
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
 @brief Monitor the DVB-T quality metric using the 100 point avergaed 
        SNR monitor. Based on Nordig SQI equation.  THis monitor offers
        improved stability over the instantaneous value.
 
 @note This monitor is thread blocking for 2s.

 @param pDemod The demodulator instance.
 @param pQuality The quality as a percentage (0-100).

 @return SONY_RESULT_OK if successful and pQuality valid.
*/
static sony_result_t sony_integ_dvbt_monitor_AveragedQuality (sony_integ_t * pInteg, 
                                                       uint8_t * pQuality)
{
    sony_dvbt_tpsinfo_t tps;
    sony_dvbt_profile_t profile = SONY_DVBT_PROFILE_HP;
    uint32_t ber = 0;
    int32_t sn = 0;
    int32_t snRel = 0;
    int32_t berSQI = 0;

    /**
     @brief The list of DVB-T Nordig Profile 1 for Non Hierachical signal
            C/N values in dBx1000.
    */
    static const int32_t nordigNonHDVBTdB1000[3][5] = {
    /*   1/2,   2/3,   3/4,   5/6,   7/8                  */    
        {5100,  6900,  7900,  8900,  9700},     /* QPSK   */
        {10800, 13100, 14600, 15600, 16000},    /* 16-QAM */
        {16500, 18700, 20200, 21600, 22500}     /* 64-QAM */
    };

    /**
     @brief The list of DVB-T Nordig Profile 1 for Hierachical signal, HP
            C/N values in dBx1000.
    */
    static const int32_t nordigHierHpDVBTdB1000[3][2][5] = {
          /* alpha = 1                                                */ 
        { /* 1/2,    2/3,    3/4,    5/6,    7/8                      */
            {9100,   12000,  13600,  15000,  16600},   /* LP = 16-QAM */
            {10900,  14100,  15700,  19400,  20600}    /* LP = 64-QAM */
        },/* alpha = 2                                                */ 
        { /* 1/2,    2/3,    3/4,    5/6,    7/8                      */
            {6800,   9100,   10400,  11900,  12700},   /* LP = 16-QAM */
            {8500,   11000,  12800,  15000,  16000}    /* LP = 64-QAM */
        },/* alpha = 4                                                */ 
        { /* 1/2,    2/3,    3/4,    5/6,    7/8                      */
            {5800,   7900,   9100,   10300,  12100},   /* LP = 16-QAM */
            {8000,   9300,   11600,  13000,  12900}    /* LP = 64-QAM */
        }
    };
    /**
     @brief The list of DVB-T Nordig Profile 1 for Hierachical signal, LP
            C/N values in dBx1000.
    */
    static const int32_t nordigHierLpDVBTdB1000[3][2][5] = {
          /* alpha = 1                                           */ 
        { /* 1/2,    2/3,    3/4,    5/6,    7/8                 */
            {12500,  14300,  15300,  16300,  16900},   /* 16-QAM */
            {16700,  19100,  20900,  22500,  23700}    /* 64-QAM */
        },/* alpha = 2                                           */ 
        { /* 1/2,    2/3,    3/4,    5/6,    7/8                 */
            {15000,  17200,  18400,  19100,  20100},   /* 16-QAM */
            {18500,  21200,  23600,  24700,  25900}    /* 64-QAM */
        },/* alpha = 4                                           */ 
        { /* 1/2,    2/3,    3/4,    5/6,    7/8                 */
            {19500,  21400,  22500,  23700,  24700},   /* 16-QAM */
            {21900,  24200,  25600,  26900,  27800}    /* 64-QAM */
        }
    };

    sony_result_t result = SONY_RESULT_OK;
    SONY_TRACE_ENTER ("sony_integ_dvbt_monitor_AveragedQuality");

    if ((!pInteg) || (!pQuality)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Software state check */
    if (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) {
        /* This api is accepted in ACTIVE state only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    if (pInteg->pDemod->system != SONY_DTV_SYSTEM_DVBT) {
        /* Not DVB-T */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

    /* Monitor TPS for Modulation / Code Rate */
    result = sony_demod_dvbt_monitor_TPSInfo (pInteg->pDemod, &tps);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Monitor profile for hierarchical signal*/
    if (tps.hierarchy != SONY_DVBT_HIERARCHY_NON) {
        uint8_t data = 0;
        /* Set SLV-T Bank : 0x10 */
        if (pInteg->pDemod->pI2c->WriteOneRegister (pInteg->pDemod->pI2c, pInteg->pDemod->i2cAddressSLVT, 0x00, 0x10) != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_I2C);
        }

        /* slave    Bank    Addr    Bit    default    Value          Name
         * ----------------------------------------------------------------------------------
         * <SLV-T>   10h     67h     [0]      8'h00      8'h01       OREG_LPSELECT
         */
        if (pInteg->pDemod->pI2c->ReadRegister (pInteg->pDemod->pI2c, pInteg->pDemod->i2cAddressSLVT, 0x67, &data, 1) != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_I2C);
        }

        profile = ((data & 0x01) == 0x01) ? SONY_DVBT_PROFILE_LP : SONY_DVBT_PROFILE_HP;
    }

    /* Get Pre-RS (Post-Viterbi) BER. */
    result = sony_demod_dvbt_monitor_PreRSBER (pInteg->pDemod, &ber);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Get SNR value. */
    result = sony_integ_dvbt_monitor_AveragedSNR (pInteg, &sn);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Ensure correct TPS values. */
    if ((tps.constellation >= SONY_DVBT_CONSTELLATION_RESERVED_3) || 
        (tps.rateHP >= SONY_DVBT_CODERATE_RESERVED_5) || 
        (tps.rateLP >= SONY_DVBT_CODERATE_RESERVED_5) || 
        (tps.hierarchy > SONY_DVBT_HIERARCHY_4)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    /* Hierachical transmission with QPSK modulation is an invalid combination */
    if ((tps.hierarchy != SONY_DVBT_HIERARCHY_NON) && (tps.constellation == SONY_DVBT_CONSTELLATION_QPSK)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_OTHER);
    }

    /* Obtain snRel based on code rate and modulation */
    if (tps.hierarchy == SONY_DVBT_HIERARCHY_NON) {
        snRel = sn - nordigNonHDVBTdB1000[tps.constellation][tps.rateHP];        
    }
    else if ( profile == SONY_DVBT_PROFILE_LP ) {
        snRel = sn - nordigHierLpDVBTdB1000[(int32_t)tps.hierarchy-1][(int32_t)tps.constellation-1][tps.rateLP];
    }
    else {
        snRel = sn - nordigHierHpDVBTdB1000[(int32_t)tps.hierarchy-1][(int32_t)tps.constellation-1][tps.rateHP];
    }
      
    /* BER_SQI is calculated from:
     * if (BER > 10^-3)          : 0
     * if (10^-7 < BER <= 10^-3) : BER_SQI = 20*log10(1/BER) - 40
     * if (BER <= 10^-7)         : BER_SQI = 100
      */
    if (ber > 10000) {
        berSQI = 0;
    }
    else if (ber > 1) {
        /* BER_SQI = 20 * log10(1/BER) - 40
         * BER_SQI = 20 * (log10(1) - log10(BER)) - 40
         * 
         * If BER in units of 1e-7
         * BER_SQI = 20 * (log10(1) - (log10(BER) - log10(1e7)) - 40
         * 
         * BER_SQI = 20 * (log10(1e7) - log10(BER)) - 40
         * BER_SQI = 20 * (7 - log10 (BER)) - 40
         */
        berSQI = (int32_t) (10 * sony_math_log10 (ber));
        berSQI = 20 * (7 * 1000 - (berSQI)) - 40 * 1000;
    }
    else {
        berSQI = 100 * 1000;
    }

    /* SQI (Signal Quality Indicator) given by:
     * if (C/Nrel < -7dB)         : SQI = 0
     * if (-7dB <= C/Nrel < +3dB) : SQI = (((C/Nrel - 3) / 10) + 1) * BER_SQI
     * if (C/Nrel >= +3dB)        : SQI = BER_SQI
     */
    if (snRel < -7 * 1000) {
        *pQuality = 0;
    }
    else if (snRel < 3 * 1000) {
        int32_t tmpSQI = (((snRel - (3 * 1000)) / 10) + 1000);
        *pQuality = (uint8_t) (((tmpSQI * berSQI) + (1000000/2)) / (1000000)) & 0xFF;
    }
    else {
        *pQuality = (uint8_t) ((berSQI + 500) / 1000);
    }

    /* Clip to 100% */
    if (*pQuality > 100) {
        *pQuality = 100;
    }

    SONY_TRACE_RETURN (result);
}

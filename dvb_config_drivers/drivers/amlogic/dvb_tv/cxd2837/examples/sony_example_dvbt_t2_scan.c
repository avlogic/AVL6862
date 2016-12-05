/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2013-05-03 12:26:06 #$
  File Revision : $Revision:: 7037 $
------------------------------------------------------------------------------*/
/**
 @file    sony_example_dvbt_t2_scan.c

         This file provides an example of DVB-T / T2 channel scanning
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
#include "sony_demod_dvbt2.h"           /* DVBT2 type definitions and demodulator functions */
#include "sony_integ_dvbt_t2.h"         /* DVBT integration layer functions (Tune, Scan, RF Level monitor etc)*/
#include "sony_demod_dvbt_monitor.h"    /* DVBT monitor functions */
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
 Const char definitions
------------------------------------------------------------------------------*/
static const char *DVBT_Profile[] = { "HP", "LP" };
static const char *DVBT2_TuneInfo[] = { "OK", "Invalid PLP ID", "Invalid T2 Mode"};
static const char *DVBT2_Profile[] = { "T2-Base", "T2-Lite", "Any" };

/*------------------------------------------------------------------------------
 Types
------------------------------------------------------------------------------*/
typedef struct scanResults {
    struct scanResults * prev;
    sony_dvbt_tune_param_t dvbtTuneParam;
    sony_dvbt2_tune_param_t dvbt2TuneParam;
    sony_dtv_system_t system;
    struct scanResults * next;
} scanResults;

/*------------------------------------------------------------------------------
 Global Variables
------------------------------------------------------------------------------*/
static scanResults * pScanResultsHead = NULL;
static scanResults * pScanResultsCurrent = NULL;

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
static void scanCallbackHandler (sony_integ_t * pInteg, sony_integ_dvbt_t2_scan_result_t * pResult, sony_integ_dvbt_t2_scan_param_t * pScanParam)
{
    sony_result_t result;
    char * tsLock;
    /* This function is provided as an example callback handler.  It simply adds the 
       valid channels to a linked list and displays the channel information to the
       console.  It also waits for TS lock as a scan callback is made after only
       demodulator lock is acheived.  Depending on the system requirements you could 
       use this function to monitor for detailed channel parameters (constellation, 
       coderate, carrier offset, TS rate etc.).
       
       Note: A callback is made for each inidvidual PLP in a tuned DVB-T2 channel. */

    if (pResult->tuneResult == SONY_RESULT_OK) {
        /* Callback contains channel information, so add to scanResults linked list. */
        scanResults * newResult = (scanResults*) malloc (sizeof (scanResults));
        
        if (newResult != NULL) {
            newResult->next = NULL;
            newResult->prev = NULL;
            newResult->system = pResult->system;

            if (newResult->system == SONY_DTV_SYSTEM_DVBT) {
                newResult->dvbtTuneParam = pResult->dvbtTuneParam;
            }
            else if  (newResult->system == SONY_DTV_SYSTEM_DVBT2) {
                newResult->dvbt2TuneParam = pResult->dvbt2TuneParam;
            }

            if (pScanResultsHead == NULL) {
                pScanResultsHead = newResult;
                pScanResultsCurrent = newResult;
            }
            else {
                newResult->prev = pScanResultsCurrent;
                pScanResultsCurrent->next = newResult;
                pScanResultsCurrent = pScanResultsCurrent->next;
            }
        }

        /* Output channel information to table */
        if (pResult->system == SONY_DTV_SYSTEM_DVBT) {
            result = sony_integ_dvbt_WaitTSLock(pInteg);
            tsLock = (result == SONY_RESULT_OK)? "Yes" : "No";

            printf (" %-3u | DVB-T  | %-6s    | %-6uKHz        | ------ | %s      | %s \n",
                    ((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) *100) / (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz),
                    Common_Bandwidth[pResult->dvbtTuneParam.bandwidth], 
                    pResult->dvbtTuneParam.centerFreqKHz, 
                    DVBT_Profile[pResult->dvbtTuneParam.profile], 
                    tsLock);
        }
        else if (pResult->system == SONY_DTV_SYSTEM_DVBT2) {
            result = sony_integ_dvbt2_WaitTSLock(pInteg, pResult->dvbt2TuneParam.profile);
            tsLock = (result == SONY_RESULT_OK)? "Yes" : "No";

            printf (" %-3u | DVB-T2 | %-6s    | %-6uKHz        | %-3u    | %s | %s \n",
                    ((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) *100) / (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz),
                    Common_Bandwidth[pResult->dvbt2TuneParam.bandwidth],
                    pResult->dvbt2TuneParam.centerFreqKHz, 
                    pResult->dvbt2TuneParam.dataPlpId, 
                    DVBT2_Profile[pResult->dvbt2TuneParam.profile], 
                    tsLock);
        }
    }
    else {
        /* Callback is for progress only */
        printf (" %-3u |   -    |     -     |        -         |   -    |    -    |    -     \n",
            ((pResult->centerFreqKHz - pScanParam->startFrequencyKHz) *100) / (pScanParam->endFrequencyKHz - pScanParam->startFrequencyKHz));
    }
}

/* Count the number of entries in the channel linked list */
static uint16_t getChannelCount()
{
    uint16_t channelCount;
    scanResults * scanResult = pScanResultsHead;

    if (pScanResultsHead == NULL) {
        return 0;
    }
    else {
        channelCount = 1;
        while (scanResult->next != NULL) {
            channelCount++;
            scanResult = scanResult->next;
        }
    }

    return channelCount;
}

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
    sony_integ_dvbt_t2_scan_param_t scanParam;
    drvi2c_feusb_t feusb;

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
     * Scan
     * ------------------------------------------------------------------------------ */
    printf ("------------------------------------------\n");
    printf (" Scan   \n");
    printf ("------------------------------------------\n");

    /* Configure the DVBT / T2 scan parameters based on a full spectrum.  Modify the following
       parameters specific to the scan requirements. */
    scanParam.bandwidth = SONY_DEMOD_BW_8_MHZ;  /* Bandwidth in of channels to locate */
    scanParam.startFrequencyKHz = 474000;       /* Start frequency of scan process */
    scanParam.endFrequencyKHz = 858000;         /* End frequency of scan process */
    scanParam.stepFrequencyKHz = 8000;          /* Step frequency between attempted tunes in scan */

    /* The system parameter indicates which DVB systems should be included in the scan
       process.  For individual systems use either SONY_DTV_SYSTEM_DVBT or 
       SONY_DTV_SYSTEM_DVBT2, or to include both use SONY_DTV_SYSTEM_ANY */
    scanParam.system = SONY_DTV_SYSTEM_ANY;

    /* The profile parameter is used to denote the DVB-T2 system profiles
     * to  */
    scanParam.t2Profile = SONY_DVBT2_PROFILE_ANY;

    printf (" Scan with the following parameters:\n");
    printf ("  - Bandwidth       : %s\n", Common_Bandwidth[scanParam.bandwidth]);
    printf ("  - Start Frequency : %uKHz\n", scanParam.startFrequencyKHz);
    printf ("  - End Frequency   : %uKHz\n", scanParam.endFrequencyKHz);
    printf ("  - Step Frequency  : %uKHz\n", scanParam.stepFrequencyKHz);
    printf ("  - System          : %s\n", Common_System[scanParam.system]);
    printf ("  - T2 Profile      : %s\n", DVBT2_Profile[scanParam.t2Profile]);

    /* Create table header for the scan results */
    printf ("\nScan Results:\n");
    printf ("-----|--------|-----------|------------------|--------|---------|--------- \n");
    printf ("  %%  | SYSTEM | BANDWIDTH | CENTRE FREQUENCY | PLP ID | PROFILE | TS LOCK  \n");
    printf ("-----|--------|-----------|------------------|--------|---------|--------- \n");
    /* Call Scan.  This is thread blocking and can take severeal seconds depending 
       on scan range and step size. Channel information will be passed back to
       scanCallbackHandler at each step. */
    tuneResult = sony_integ_dvbt_t2_Scan (&integ, &scanParam, &scanCallbackHandler);
    
    printf ("\n  - Result         : %s\n", Common_Result[tuneResult]);
    printf ("  - Channels Found : %u\n\n", getChannelCount());

    if (getChannelCount() > 0) {
        /* ---------------------------------------------------------------------------------
        * Tune to first channel in list
        * ------------------------------------------------------------------------------ */
        printf ("------------------------------------------\n");
        printf (" Tune to first channel in list  \n");
        printf ("------------------------------------------\n");

        if (pScanResultsHead->system == SONY_DTV_SYSTEM_DVBT) {
            printf (" Tune to DVB-T signal with the following parameters:\n");
            printf ("  - Center Freq    : %uKHz\n", pScanResultsHead->dvbtTuneParam.centerFreqKHz);
            printf ("  - Bandwidth      : %s\n", Common_Bandwidth[pScanResultsHead->dvbtTuneParam.bandwidth]);
            printf ("  - Profile        : %s\n", DVBT_Profile[pScanResultsHead->dvbtTuneParam.profile]);

            /* Perform DVBT Tune using the scan results tune param structure directly */
            tuneResult = sony_integ_dvbt_Tune (&integ, &pScanResultsHead->dvbtTuneParam);
            
            printf ("  - Result         : %s\n\n", Common_Result[tuneResult]);
        }
        else if (pScanResultsHead->system == SONY_DTV_SYSTEM_DVBT2) {
            printf (" Tune to DVB-T2 signal with the following parameters:\n");
            printf ("  - Center Freq    : %uKHz\n", pScanResultsHead->dvbt2TuneParam.centerFreqKHz);
            printf ("  - Bandwidth      : %s\n", Common_Bandwidth[pScanResultsHead->dvbt2TuneParam.bandwidth]);
            printf ("  - PLP ID         : %u\n", pScanResultsHead->dvbt2TuneParam.dataPlpId);
            printf ("  - Profile        : %s\n", DVBT2_Profile[pScanResultsHead->dvbt2TuneParam.profile]);

            /* Perform DVBT2 Tune using the scan results tune param structure directly */
            tuneResult = sony_integ_dvbt2_Tune (&integ, &pScanResultsHead->dvbt2TuneParam);

            printf ("  - Result         : %s\n", Common_Result[tuneResult]);
            printf ("  - Tune Info      : %s\n\n", DVBT2_TuneInfo[pScanResultsHead->dvbt2TuneParam.tuneInfo]);
        }
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

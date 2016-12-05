/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2013-01-29 12:48:35 #$
  File Revision : $Revision:: 6581 $
------------------------------------------------------------------------------*/
#include "sony_tuner_ascot3.h"

/*------------------------------------------------------------------------------
 Driver Version
------------------------------------------------------------------------------*/
const char* sony_tuner_ascot3_version =  SONY_ASCOT3_VERSION;

/*------------------------------------------------------------------------------
 Static Function Prototypes
------------------------------------------------------------------------------*/
static sony_result_t sony_tuner_ascot3_Initialize (sony_tuner_terr_cable_t * pTuner);

static sony_result_t sony_tuner_ascot3_Tune (sony_tuner_terr_cable_t * pTuner,
                                             uint32_t frequency, 
                                             sony_dtv_system_t system, 
                                             sony_demod_bandwidth_t bandwidth);

static sony_result_t sony_tuner_ascot3_Shutdown (sony_tuner_terr_cable_t * pTuner);

static sony_result_t sony_tuner_ascot3_Sleep (sony_tuner_terr_cable_t * pTuner);

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_tuner_ascot3_Create (sony_tuner_terr_cable_t * pTuner,
                                        sony_ascot3_xtal_t xtalFreq,  
                                        uint8_t i2cAddress,
                                        sony_i2c_t * pI2c,
                                        uint32_t configFlags, 
                                        sony_ascot3_t * pAscot3Tuner)
{
    sony_result_t result = SONY_RESULT_OK;
	printk("[sony_tuner_ascot3.c]: sony_tuner_ascot3_Create():flags(%x)\n", configFlags);
    SONY_TRACE_ENTER ("sony_tuner_ascot3_Create");

    if ((!pI2c) || (!pAscot3Tuner) || (!pTuner)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }
    
    /* Create the underlying Ascot3 reference driver. */
    result = sony_ascot3_Create (pAscot3Tuner, xtalFreq, i2cAddress, pI2c, configFlags);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Create local copy of instance data. */
    pTuner->Initialize = sony_tuner_ascot3_Initialize;
    pTuner->Sleep = sony_tuner_ascot3_Sleep;
    pTuner->Shutdown = sony_tuner_ascot3_Shutdown;
    pTuner->Tune = sony_tuner_ascot3_Tune;
    pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
    pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;  
    pTuner->frequencyKHz = 0;
    pTuner->i2cAddress = i2cAddress;
    pTuner->pI2c = pI2c;
    pTuner->flags = configFlags;
    pTuner->user = pAscot3Tuner;

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_tuner_ascot3_RFFilterConfig (sony_tuner_terr_cable_t * pTuner, uint8_t coeff, uint8_t offset)
{
    sony_result_t result = SONY_RESULT_OK;
	printk("[sony_tuner_ascot3.c]: sony_tuner_ascot3_RFFilterConfig(): coeff(%d) offset(%d)\n", coeff, offset);
    SONY_TRACE_ENTER ("sony_tuner_ascot3_RFFilterConfig");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = sony_ascot3_RFFilterConfig (((sony_ascot3_t *) pTuner->user), coeff, offset);

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_tuner_ascot3_SetGPO (sony_tuner_terr_cable_t * pTuner, uint8_t id, uint8_t value)
{
    sony_result_t result = SONY_RESULT_OK;
	printk("[sony_tuner_ascot3.c]: sony_tuner_ascot3_SetGPO(); id(%d) valeu(%d)\n", id, value);
    SONY_TRACE_ENTER ("sony_tuner_ascot3_Write_GPIO");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = sony_ascot3_SetGPO (((sony_ascot3_t *) pTuner->user), id, value);

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_tuner_ascot3_ReadRssi (sony_tuner_terr_cable_t * pTuner, int32_t * pRssi)
{
    sony_result_t result = SONY_RESULT_OK;
	printk("[sony_tuner_ascot3.c]: sony_tuner_ascot3_ReadRssi():\n");
    SONY_TRACE_ENTER ("sony_tuner_ascot3_ReadRssi");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = sony_ascot3_ReadRssi (((sony_ascot3_t *) pTuner->user), pRssi);

    SONY_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/
static sony_result_t sony_tuner_ascot3_Initialize (sony_tuner_terr_cable_t * pTuner)
{
    sony_result_t result = SONY_RESULT_OK;
	printk("[sony_tuner_ascot3.c]: sony_tuner_ascot3_Initialize():\n");
    SONY_TRACE_ENTER ("sony_tuner_ascot3_Initialize");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Call into underlying driver. */
    result = sony_ascot3_Initialize (((sony_ascot3_t *) pTuner->user));

    /* Device is in "Power Save" state. */
    pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
    pTuner->frequencyKHz = 0;
    pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;

    SONY_TRACE_RETURN (result);
}

static sony_result_t sony_tuner_ascot3_Tune (sony_tuner_terr_cable_t * pTuner,
                                             uint32_t frequencyKHz, 
                                             sony_dtv_system_t system, 
                                             sony_demod_bandwidth_t bandwidth)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_ascot3_tv_system_t dtvSystem;

	printk("[sony_tuner_ascot3.c]: sony_tuner_ascot3_Tune(): system(%d) Band(%d)\n", system, bandwidth);
    SONY_TRACE_ENTER ("sony_tuner_ascot3_Tune");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Convert system, bandwidth into dtv system. */
    switch (system) {
    case SONY_DTV_SYSTEM_DVBC:
		switch (bandwidth){
			case SONY_DEMOD_BW_6_MHZ:
        dtvSystem = SONY_ASCOT3_DTV_DVBC_6;
		break;
		case SONY_DEMOD_BW_7_MHZ:

		case SONY_DEMOD_BW_8_MHZ:
			dtvSystem = SONY_ASCOT3_DTV_DVBC_8;
			break;
		default:
			SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
		}
        break;

    case SONY_DTV_SYSTEM_DVBT:
        switch (bandwidth) {
        case SONY_DEMOD_BW_5_MHZ:
             dtvSystem = SONY_ASCOT3_DTV_DVBT_5;
            break;
        case SONY_DEMOD_BW_6_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBT_6;
            break;
        case SONY_DEMOD_BW_7_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBT_7;
            break;
        case SONY_DEMOD_BW_8_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBT_8;
            break;
        default:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
        }
        break;

    case SONY_DTV_SYSTEM_DVBT2:
        switch (bandwidth) {
        case SONY_DEMOD_BW_1_7_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBT2_1_7;
            break;
        case SONY_DEMOD_BW_5_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBT2_5;
            break;
        case SONY_DEMOD_BW_6_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBT2_6;
            break;
        case SONY_DEMOD_BW_7_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBT2_7;
            break;
        case SONY_DEMOD_BW_8_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBT2_8;
            break;
        default:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);

        }
        break;

    case SONY_DTV_SYSTEM_DVBC2:
        switch (bandwidth) {
        case SONY_DEMOD_BW_6_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBC2_6;
            break;
        case SONY_DEMOD_BW_8_MHZ:
            dtvSystem = SONY_ASCOT3_DTV_DVBC2_8;
            break;
        default:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
        }
            break;

    /* Intentional fall-through */
    case SONY_DTV_SYSTEM_UNKNOWN:
    default:
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Call into underlying driver. */
    result = sony_ascot3_Tune(((sony_ascot3_t *) pTuner->user), frequencyKHz, dtvSystem);
    if (result != SONY_RESULT_OK) {
        pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
        pTuner->frequencyKHz = 0;
        pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;
        SONY_TRACE_RETURN (result);
    }

    /* Allow the tuner time to settle */
    SONY_SLEEP(50);

    result = sony_ascot3_TuneEnd((sony_ascot3_t *) pTuner->user);
    if (result != SONY_RESULT_OK) {
        pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
        pTuner->frequencyKHz = 0;
        pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;
        SONY_TRACE_RETURN (result);
    }

    /* Assign current values. */
    pTuner->system = system;
    pTuner->frequencyKHz = ((sony_ascot3_t *) pTuner->user)->frequencykHz;
    pTuner->bandwidth = bandwidth;

	printk("[sony_tuner_ascot3.c]: sony_tuner_ascot3_Tune(): system(%d)[C(%d)] bandwidth(%d)[%d/%d]\n", pTuner->system, SONY_DTV_SYSTEM_DVBC, pTuner->bandwidth, SONY_DEMOD_BW_6_MHZ, SONY_DEMOD_BW_8_MHZ);
    SONY_TRACE_RETURN (result);
}

static sony_result_t sony_tuner_ascot3_Shutdown (sony_tuner_terr_cable_t * pTuner)
{
    sony_result_t result = SONY_RESULT_OK;
	printk("[sony_tuner_ascot3.c]: sony_tuner_ascot3_Shutdown():\n");
    SONY_TRACE_ENTER ("sony_tuner_ascot3_Shutdown");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Call into underlying driver. */
    result = sony_ascot3_Sleep (((sony_ascot3_t *) pTuner->user));

    /* Device is in "Power Save" state. */
    pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
    pTuner->frequencyKHz = 0;
    pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;

    SONY_TRACE_RETURN (result);
}

static sony_result_t sony_tuner_ascot3_Sleep (sony_tuner_terr_cable_t * pTuner)
{
    sony_result_t result = SONY_RESULT_OK;
	printk("[sony_tuner_ascot3.c]: sony_tuner_ascot3_Sleep(): \n");
    SONY_TRACE_ENTER ("sony_tuner_ascot3_Sleep");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Call into underlying driver. */
    result = sony_ascot3_Sleep (((sony_ascot3_t *) pTuner->user));

    /* Device is in "Power Save" state. */
    pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
    pTuner->frequencyKHz = 0;
    pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;

    SONY_TRACE_RETURN (result);
}

/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2013-02-21 16:53:18 #$
  File Revision : $Revision:: 6676 $
------------------------------------------------------------------------------*/
#include "sony_tuner_ascot2e.h"

/*------------------------------------------------------------------------------
 Driver Version
------------------------------------------------------------------------------*/
const char* sony_tuner_ascot2e_version =  SONY_ASCOT2E_VERSION;

/*------------------------------------------------------------------------------
 Static Function Prototypes
------------------------------------------------------------------------------*/
static sony_result_t sony_tuner_ascot2e_Initialize (sony_tuner_terr_cable_t * pTuner);

static sony_result_t sony_tuner_ascot2e_Tune (sony_tuner_terr_cable_t * pTuner,
                                              uint32_t frequency, 
                                              sony_dtv_system_t system, 
                                              sony_demod_bandwidth_t bandwidth);

static sony_result_t sony_tuner_ascot2e_Shutdown (sony_tuner_terr_cable_t * pTuner);

static sony_result_t sony_tuner_ascot2e_Sleep (sony_tuner_terr_cable_t * pTuner);

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_tuner_ascot2e_Create (sony_tuner_terr_cable_t * pTuner,
                                         uint32_t xtalFreqMHz,                                     
                                         uint8_t i2cAddress,
                                         sony_i2c_t * pI2c,
                                         uint32_t configFlags, 
                                         sony_ascot2e_t * pAscot2ETuner)
{
    sony_result_t result = SONY_RESULT_OK;
    SONY_TRACE_ENTER ("sony_tuner_ascot2e_Create");

    if ((!pI2c) || (!pAscot2ETuner) || (!pTuner)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Create the underlying Ascot2E reference driver. */
    result = sony_ascot2e_Create (pAscot2ETuner, xtalFreqMHz, i2cAddress, pI2c, configFlags);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Create local copy of instance data. */
    pTuner->Initialize = sony_tuner_ascot2e_Initialize;
    pTuner->Sleep = sony_tuner_ascot2e_Sleep;
    pTuner->Shutdown = sony_tuner_ascot2e_Shutdown;
    pTuner->Tune = sony_tuner_ascot2e_Tune;
    pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
    pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;  
    pTuner->frequencyKHz = 0;
    pTuner->i2cAddress = i2cAddress;
    pTuner->pI2c = pI2c;
    pTuner->flags = configFlags;
    pTuner->user = pAscot2ETuner;

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_tuner_ascot2e_RFFilterConfig (sony_tuner_terr_cable_t * pTuner, uint8_t coeff, uint8_t offset)
{
    sony_result_t result = SONY_RESULT_OK;
    SONY_TRACE_ENTER ("sony_tuner_ascot2e_RFFilterConfig");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = sony_ascot2e_RFFilterConfig (((sony_ascot2e_t *) pTuner->user), coeff, offset);

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_tuner_ascot2e_SetGPO (sony_tuner_terr_cable_t * pTuner, uint8_t id, uint8_t value)
{
    sony_result_t result = SONY_RESULT_OK;
    SONY_TRACE_ENTER ("sony_tuner_ascot2e_Write_GPIO");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = sony_ascot2e_SetGPO (((sony_ascot2e_t *) pTuner->user), id, value);

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_tuner_ascot2e_ReadGain (sony_tuner_terr_cable_t * pTuner, int32_t * pIFGain, int32_t * pRFGain, uint8_t forceRFAGCRead)
{
    sony_result_t result = SONY_RESULT_OK;
    SONY_TRACE_ENTER ("sony_tuner_ascot2e_ReadGain");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = sony_ascot2e_ReadGain (((sony_ascot2e_t *) pTuner->user), pIFGain, pRFGain, forceRFAGCRead);

    SONY_TRACE_RETURN (result);
}

sony_result_t sony_tuner_ascot2e_CalcGainFromAGC (sony_tuner_terr_cable_t * pTuner, uint32_t agcReg, int32_t * pIFGain, int32_t * pRFGain)
{
    SONY_TRACE_ENTER ("sony_tuner_ascot2e_CalcGainFromAGC");

    if (!pTuner || !pIFGain || !pRFGain) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /*
        IFGAIN = if(AGC > 0.6){
                     6 + IF_BPF_GC 
                 }else{
                     if(AGC > 0.19){
                         6 + IF_BPF_GC + 69 * (0.6 - AGC)
                     }else{
                         6 + IF_BPF_GC + 69 * 0.41
                     }
                 }

        Note that AGC(V) = DemodAGCReg * 2.27 / 4096
        So...
        IFGAIN(100xdB) = if(DemodAGCReg * 227 > 245760){
                             (6 + IF_BPF_GC) * 100 
                         }else if(DemodAGCReg * 227 > 77824){
                             (6 + IF_BPF_GC) * 100 + (69 * (245760 - DemodAGCReg * 227))/4096
                         }else{
                             (6 + IF_BPF_GC) * 100 + 69 * 41
                         }
    */
    {
        int8_t if_bpf_gc = 0;
        int32_t agcRegX = (int32_t)agcReg * 227;

        switch(pTuner->system){
        case SONY_DTV_SYSTEM_DVBT:
        case SONY_DTV_SYSTEM_DVBT2:
            if_bpf_gc = 12;
            break;
        case SONY_DTV_SYSTEM_DVBC:
            if_bpf_gc = 4;
            break;
        case SONY_DTV_SYSTEM_DVBC2:
            if_bpf_gc = 0;
            break;
        default:
            SONY_TRACE_RETURN(SONY_RESULT_ERROR_SW_STATE);
        }

        if(agcRegX > 245760){
            *pIFGain = (6 + if_bpf_gc) * 100;
        }else if(agcRegX > 77824){
            *pIFGain = (6 + if_bpf_gc) * 100 + (69 * (245760 - agcRegX) + 2048) / 4096; /* Round */
        }else{
            *pIFGain = (6 + if_bpf_gc) * 100 + 69 * 41;
        }
    }

    /*
        RFGAIN = if(AGC < 0.42){
                     RFGAIN_MAX
                 }else if(AGC < 0.49){
                     RF_GAIN_MAX - 63 * (AGC - 0.42)
                 }else if(AGC < 0.6){
                     RF_GAIN_MAX - 63 * 0.07
                 }else if(AGC < 1.13){
                     RF_GAIN_MAX - 63 * 0.07 - 63 * (AGC - 0.6)
                 }else if(AGC < 1.38){
                     RF_GAIN_MAX - 63 * 0.6 - 45 * (AGC - 1.13)
                 }else{
                     RF_GAIN_MAX - 63 * 0.6 - 45 * 0.25 + RFGRAD * (AGC - 1.38)
                 }

        Note that AGC(V) = DemodAGCReg * 2.27 / 4096

        So...
        RFGAIN(100xdB) = if(DemodAGCReg * 227 < 172032){
                             RFGAIN_MAX * 100
                         }else if(DemodAGCReg * 227 < 200704){
                             RFGAIN_MAX * 100 - (63 * (DemodAGCReg * 227 - 172032))/4096
                         }else if(DemodAGCReg * 227 < 245760){
                             RFGAIN_MAX * 100 - 63 * 7
                         }else if(DemodAGCReg * 227 < 462848){
                             RFGAIN_MAX * 100 - 63 * 7 - (63 * (DemodAGCReg * 227 - 245760))/4096
                         }else if(DemodAGCReg * 227 < 565248){
                             RFGAIN_MAX * 100 - 63 * 60 - (45 * (DemodAGCReg * 227 - 462848))/4096
                         }else{
                             RFGAIN_MAX * 100 - 63 * 60 - 45 * 25 + RFGRAD * (DemodAGCReg * 227 - 565248))/4096
                         }
    */
    {
        int32_t agcRegX = (int32_t)agcReg * 227;
        int32_t rfgainmax_x100 = 0;

        if(pTuner->frequencyKHz > 900000){
            rfgainmax_x100 = 4320;
        }else if(pTuner->frequencyKHz > 700000){
            rfgainmax_x100 = 4420;
        }else if(pTuner->frequencyKHz > 600000){
            rfgainmax_x100 = 4330;
        }else if(pTuner->frequencyKHz > 504000){
            rfgainmax_x100 = 4160;
        }else if(pTuner->frequencyKHz > 400000){
            rfgainmax_x100 = 4550;
        }else if(pTuner->frequencyKHz > 320000){
            rfgainmax_x100 = 4400;
        }else if(pTuner->frequencyKHz > 270000){
            rfgainmax_x100 = 4520;
        }else if(pTuner->frequencyKHz > 235000){
            rfgainmax_x100 = 4370;
        }else if(pTuner->frequencyKHz > 192000){
            rfgainmax_x100 = 4190;
        }else if(pTuner->frequencyKHz > 130000){
            rfgainmax_x100 = 4550;
        }else if(pTuner->frequencyKHz > 86000){
            rfgainmax_x100 = 4630;
        }else if(pTuner->frequencyKHz > 50000){
            rfgainmax_x100 = 4350;
        }else{
            rfgainmax_x100 = 4450;
        }

        if(agcRegX < 172032){
            *pRFGain = rfgainmax_x100;
        }else if(agcRegX < 200704){
            *pRFGain = rfgainmax_x100 - (63 * (agcRegX - 172032) + 2048) / 4096; /* Round */
        }else if(agcRegX < 245760){
            *pRFGain = rfgainmax_x100 - 63 * 7;
        }else if(agcRegX < 462848){
            *pRFGain = rfgainmax_x100 - 63 * 7 - (63 * (agcRegX - 245760) + 2048) / 4096; /* Round */
        }else if(agcRegX < 565248){
            *pRFGain = rfgainmax_x100 - 63 * 60 - (45 * (agcRegX - 462848) + 2048) / 4096; /* Round */
        }else{
            int32_t rfgrad = -95;

            *pRFGain = rfgainmax_x100 - 63 * 60 - 45 * 25 + (rfgrad * (agcRegX - 565248) - 2048) / 4096; /* Round */
        }
    }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

/*------------------------------------------------------------------------------
 Static Functions
------------------------------------------------------------------------------*/
static sony_result_t sony_tuner_ascot2e_Initialize (sony_tuner_terr_cable_t * pTuner)
{
    sony_result_t result = SONY_RESULT_OK;
    SONY_TRACE_ENTER ("sony_tuner_ascot2e_Initialize");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Call into underlying driver. */
    result = sony_ascot2e_Initialize (((sony_ascot2e_t *) pTuner->user));

    /* Device is in "Power Save" state. */
    pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
    pTuner->frequencyKHz = 0;
    pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;

    SONY_TRACE_RETURN (result);
}

static sony_result_t sony_tuner_ascot2e_Tune (sony_tuner_terr_cable_t * pTuner,
                                              uint32_t frequencyKHz, 
                                              sony_dtv_system_t system, 
                                              sony_demod_bandwidth_t bandwidth)
{
    sony_result_t result = SONY_RESULT_OK;
    sony_ascot2e_tv_system_t dtvSystem;

    SONY_TRACE_ENTER ("sony_tuner_ascot2e_Tune");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

 /* Call into underlying driver. Convert system, bandwidth into dtv system. */
    switch (system) {
    case SONY_DTV_SYSTEM_DVBC:
        dtvSystem = SONY_ASCOT2E_DTV_DVBC;
        break;

    case SONY_DTV_SYSTEM_DVBT:
        switch (bandwidth) {
        case SONY_DEMOD_BW_5_MHZ:
             dtvSystem = SONY_ASCOT2E_DTV_DVBT_5;
            break;
        case SONY_DEMOD_BW_6_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBT_6;
            break;
        case SONY_DEMOD_BW_7_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBT_7;
            break;
        case SONY_DEMOD_BW_8_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBT_8;
            break;
        default:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
        }
        break;

    case SONY_DTV_SYSTEM_DVBT2:
        switch (bandwidth) {
        case SONY_DEMOD_BW_1_7_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBT2_1_7;
            break;
        case SONY_DEMOD_BW_5_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBT2_5;
            break;
        case SONY_DEMOD_BW_6_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBT2_6;
            break;
        case SONY_DEMOD_BW_7_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBT2_7;
            break;
        case SONY_DEMOD_BW_8_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBT2_8;
            break;
        default:
            SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);

        }
        break;

    case SONY_DTV_SYSTEM_DVBC2:
        switch (bandwidth) {
        case SONY_DEMOD_BW_6_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBC2_6;
            break;
        case SONY_DEMOD_BW_8_MHZ:
            dtvSystem = SONY_ASCOT2E_DTV_DVBC2_8;
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

    result = sony_ascot2e_Tune(((sony_ascot2e_t *) pTuner->user), frequencyKHz, dtvSystem);
    if (result != SONY_RESULT_OK) {
        pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
        pTuner->frequencyKHz = 0;
        pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;
        SONY_TRACE_RETURN (result);
    }

    /* Allow the tuner time to settle */
    SONY_SLEEP(50);

    result = sony_ascot2e_TuneEnd((sony_ascot2e_t *) pTuner->user);
    if (result != SONY_RESULT_OK) {
        pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
        pTuner->frequencyKHz = 0;
        pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;
        SONY_TRACE_RETURN (result);
    }

    /* Assign current values. */
    pTuner->system = system;
    pTuner->frequencyKHz = ((sony_ascot2e_t *) pTuner->user)->frequencykHz;
    pTuner->bandwidth = bandwidth;

    SONY_TRACE_RETURN (result);
}

static sony_result_t sony_tuner_ascot2e_Shutdown (sony_tuner_terr_cable_t * pTuner)
{
    sony_result_t result = SONY_RESULT_OK;
    SONY_TRACE_ENTER ("sony_tuner_ascot2e_Shutdown");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Call into underlying driver. */
    result = sony_ascot2e_Sleep (((sony_ascot2e_t *) pTuner->user));

    /* Device is in "Power Save" state. */
    pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
    pTuner->frequencyKHz = 0;
    pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;

    SONY_TRACE_RETURN (result);
}

static sony_result_t sony_tuner_ascot2e_Sleep (sony_tuner_terr_cable_t * pTuner)
{
    sony_result_t result = SONY_RESULT_OK;
    SONY_TRACE_ENTER ("sony_tuner_ascot2e_Sleep");

    if (!pTuner || !pTuner->user) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Call into underlying driver. */
    result = sony_ascot2e_Sleep (((sony_ascot2e_t *) pTuner->user));

    /* Device is in "Power Save" state. */
    pTuner->system = SONY_DTV_SYSTEM_UNKNOWN;
    pTuner->frequencyKHz = 0;
    pTuner->bandwidth = SONY_DEMOD_BW_UNKNOWN;

    SONY_TRACE_RETURN (result);
}

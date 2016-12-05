/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2012-06-12 13:06:10 #$
  File Revision : $Revision:: 5540 $
------------------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "sony_common.h"
#include "sony_demod.h"
#include "sony_integ.h"
#include "mxl603/MxL603_TunerApi.h"

/*------------------------------------------------------------------------------
 Functions
------------------------------------------------------------------------------*/
sony_result_t sony_integ_Create (sony_integ_t * pInteg,
                                 sony_demod_xtal_t xtalFreq,
                                 uint8_t i2cAddress,
                                 sony_i2c_t * pDemodI2c,
                                 sony_demod_t * pDemod 
#ifdef SONY_DEMOD_SUPPORT_TERR_OR_CABLE
                                 ,sony_tuner_terr_cable_t * pTunerTerrCable
#endif
#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
                                 ,sony_tuner_sat_t * pTunerSat,
                                 sony_lnbc_t * pLnbc
#endif
                                 )
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_Create");

    if ((!pInteg) || (!pDemodI2c) || (!pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Create demodulator instance */
    result = sony_demod_Create (pDemod, xtalFreq, i2cAddress, pDemodI2c);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    /* Populate the integration structure */
    pInteg->pDemod = pDemod;

#ifdef SONY_DEMOD_SUPPORT_TERR_OR_CABLE
    pInteg->pTunerTerrCable = pTunerTerrCable;
#endif

#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
    pInteg->pTunerSat = pTunerSat;
    pInteg->pLnbc = pLnbc;
#endif

    sony_atomic_set (&(pInteg->cancel), 0);

    SONY_TRACE_RETURN (result);
}

#ifdef SONY_DEMOD_SUPPORT_TERR_OR_CABLE
sony_result_t sony_integ_InitializeT_C (sony_integ_t * pInteg)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_InitializeT_C");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }
    
    result = sony_demod_InitializeT_C (pInteg->pDemod);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Enable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

#ifdef CONFIG_TH_CXD2837_TUNER_MXL603
	MXL603_INIT();
#else
    if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Initialize)) {
        /* Initialize the terrestrial / cable tuner. */
        result = pInteg->pTunerTerrCable->Initialize (pInteg->pTunerTerrCable);
        if (result != SONY_RESULT_OK) {
			printk("turner iinitialize error\n");
            SONY_TRACE_RETURN (result);
        }
    }
#endif

#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
    if ((pInteg->pTunerSat) && (pInteg->pTunerSat->Initialize)) {
        /* Initialize the satellite tuner. */
        result = pInteg->pTunerSat->Initialize (pInteg->pTunerSat);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Disable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
    if ((pInteg->pLnbc) && (pInteg->pLnbc->Initialize)) {
        /* Initialize the lnb controller. */
        result = pInteg->pLnbc->Initialize (pInteg->pLnbc);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
    if ((pInteg->pLnbc) && (pInteg->pLnbc->Sleep)) {
        /* Sleep the lnb controller */
        result = pInteg->pLnbc->Sleep (pInteg->pLnbc);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

    SONY_TRACE_RETURN (result);
}
#endif

#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
sony_result_t sony_integ_InitializeS (sony_integ_t * pInteg)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_InitializeS");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    result = sony_demod_InitializeS (pInteg->pDemod);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Enable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

#ifdef SONY_DEMOD_SUPPORT_TERR_OR_CABLE
    if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Initialize)) {
        /* Initialize the terrestrial / cable tuner. */
        result = pInteg->pTunerTerrCable->Initialize (pInteg->pTunerTerrCable);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

    if ((pInteg->pTunerSat) && (pInteg->pTunerSat->Initialize)) {
        /* Initialize the satellite tuner. */
        result = pInteg->pTunerSat->Initialize (pInteg->pTunerSat);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Disable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

    if ((pInteg->pLnbc) && (pInteg->pLnbc->Initialize)) {
        /* Initialize the lnb controller. */
        result = pInteg->pLnbc->Initialize (pInteg->pLnbc);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }

    SONY_TRACE_RETURN (result);
}
#endif

#ifdef SONY_DEMOD_SUPPORT_TERR_OR_CABLE
sony_result_t sony_integ_SleepT_C (sony_integ_t * pInteg)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_SleepT_C");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if ((pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_S) &&
        (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_S) &&
        (pInteg->pDemod->state != SONY_DEMOD_STATE_SHUTDOWN)) {
        /* This api is accepted in SLEEP, ACTIVE and SHUTDOWN states only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Enable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

#ifdef CONFIG_TH_CXD2837_TUNER_MXL603
#else
    if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Sleep)) {
        /* Call the terrestrial / cable tuner Sleep implementation */
        result = pInteg->pTunerTerrCable->Sleep (pInteg->pTunerTerrCable);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
    if ((pInteg->pTunerSat) && (pInteg->pTunerSat->Sleep)) {
        /* Call the satellite tuner Sleep implementation */
        result = pInteg->pTunerSat->Sleep (pInteg->pTunerSat);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Disable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
    if ((pInteg->pLnbc) && (pInteg->pLnbc->Sleep)) {
        /* Sleep the lnb controller. */
        result = pInteg->pLnbc->Sleep (pInteg->pLnbc);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

    /* Call the demodulator Sleep function for DVB-C / C2 / T / T2 */
    result = sony_demod_SleepT_C (pInteg->pDemod);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    SONY_TRACE_RETURN (result);
}
#endif

#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
sony_result_t sony_integ_SleepS (sony_integ_t * pInteg)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_SleepS");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if ((pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_S) &&
        (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_S) &&
        (pInteg->pDemod->state != SONY_DEMOD_STATE_SHUTDOWN)) {
        /* This api is accepted in SLEEP, ACTIVE and SHUTDOWN states only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Enable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

#ifdef SONY_DEMOD_SUPPORT_TERR_OR_CABLE
    if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Sleep)) {
        /* Call the terrestrial / cable tuner Sleep implementation */
        result = pInteg->pTunerTerrCable->Sleep (pInteg->pTunerTerrCable);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

    if ((pInteg->pTunerSat) && (pInteg->pTunerSat->Sleep)) {
        /* Call the satellite tuner Sleep implementation */
        result = pInteg->pTunerSat->Sleep (pInteg->pTunerSat);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Disable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

    if ((pInteg->pLnbc) && (pInteg->pLnbc->WakeUp)) {
        /* Call the lnb controller WakeUp implementation. */
        result = pInteg->pLnbc->WakeUp (pInteg->pLnbc);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }

    /* Call the demodulator Sleep function for DVB-S / S2 */
    result = sony_demod_SleepS (pInteg->pDemod);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }

    SONY_TRACE_RETURN (result);
}
#endif

sony_result_t sony_integ_Shutdown (sony_integ_t * pInteg)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER ("sony_integ_Shutdown");

    if ((!pInteg) || (!pInteg->pDemod)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    if ((pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_SLEEP_S) &&
        (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_T_C) && (pInteg->pDemod->state != SONY_DEMOD_STATE_ACTIVE_S) &&
        (pInteg->pDemod->state != SONY_DEMOD_STATE_SHUTDOWN)) {
        /* This api is accepted in SLEEP, ACTIVE and SHUTDOWN states only */
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_SW_STATE);
    }

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Enable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x01);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

#ifdef SONY_DEMOD_SUPPORT_TERR_OR_CABLE
    if ((pInteg->pTunerTerrCable) && (pInteg->pTunerTerrCable->Shutdown)) {
        /* Call the terrestrial / cable tuner Shutdown implementation */
        result = pInteg->pTunerTerrCable->Shutdown (pInteg->pTunerTerrCable);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
    if ((pInteg->pTunerSat) && (pInteg->pTunerSat->Shutdown)) {
        /* Call the satellite tuner Shutdown implementation */
        result = pInteg->pTunerSat->Shutdown (pInteg->pTunerSat);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

#ifndef SONY_DISABLE_I2C_REPEATER
    /* Disable the I2C repeater */
    result = sony_demod_I2cRepeaterEnable (pInteg->pDemod, 0x00);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
#endif

#ifdef SONY_DEMOD_SUPPORT_DVBS_S2
    if ((pInteg->pLnbc) && (pInteg->pLnbc->Sleep)) {
        /* Sleep the lnb controller. */
        result = pInteg->pLnbc->Sleep (pInteg->pLnbc);
        if (result != SONY_RESULT_OK) {
            SONY_TRACE_RETURN (result);
        }
    }
#endif

    /* Call the demodulator Shutdown function */
    result = sony_demod_Shutdown (pInteg->pDemod);
    if (result != SONY_RESULT_OK) {
        SONY_TRACE_RETURN (result);
    }
    
    SONY_TRACE_RETURN (result);
}

sony_result_t sony_integ_Cancel (sony_integ_t * pInteg)
{
    SONY_TRACE_ENTER ("sony_integ_Cancel");

    /* Argument verification. */
    if ((!pInteg)) {
        SONY_TRACE_RETURN (SONY_RESULT_ERROR_ARG);
    }

    /* Set the cancellation flag. */
    sony_atomic_set (&(pInteg->cancel), 1);

    SONY_TRACE_RETURN (SONY_RESULT_OK);
}

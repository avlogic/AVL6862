/*------------------------------------------------------------------------------
  Copyright 2013 Sony Corporation

  Last Updated  : 2013/04/01
  File Revision : 1.0.3.0
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
 Based on ASCOT3 application note 1.2.1
------------------------------------------------------------------------------*/
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include "sony_ascot3.h"

/*------------------------------------------------------------------------------
 Definitions of static const table
------------------------------------------------------------------------------*/
#define AUTO         (0xFF) /* For IF_OUT_SEL and AGC_SEL, it means that the value is desided by config flags. */
                            /* For RF_GAIN, it means that RF_GAIN_SEL(SubAddr:0x4E) = 1 */
#define OFFSET(ofs)  ((uint8_t)(ofs) & 0x1F)
#define BW_6         (0x00)
#define BW_7         (0x01)
#define BW_8         (0x02)
#define BW_1_7       (0x03)

/**
  @brief Sony silicon tuner setting for each broadcasting system.
         These values are optimized for Sony demodulators.
         The user have to change these values if other demodulators are used.
         Please check Sony silicon tuner application note for detail.
*/
static const sony_ascot3_adjust_param_t g_param_table[SONY_ASCOT3_TV_SYSTEM_NUM] = {
    /*
    OUTLMT    IF_BPF_GC  IFOVLD_DET_LV      BW             BW_OFFSET        IF_OUT_SEL
      |  RF_GAIN |RFOVLD_DET_LV1| IF_BPF_F0 |   FIF_OFFSET    |       AGC_SEL  |  IS_LOWERLOCAL
      |     |    |     |        |   |       |      |          |          |     |     |          */
    {0x00, AUTO, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, BW_6,  OFFSET(0),  OFFSET(0),  AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_TV_SYSTEM_UNKNOWN */
    /* Analog */
    {0x00, AUTO, 0x05, 0x02, 0x05, 0x02, 0x01, 0x01, 0x01, 0x00, BW_6,  OFFSET(0),  OFFSET(1),  AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_ATV_MN_EIAJ   (System-M (Japan)) */
    {0x00, AUTO, 0x05, 0x02, 0x05, 0x02, 0x01, 0x01, 0x01, 0x00, BW_6,  OFFSET(0),  OFFSET(1),  AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_ATV_MN_SAP    (System-M (US)) */
    {0x00, AUTO, 0x05, 0x02, 0x05, 0x02, 0x01, 0x01, 0x01, 0x00, BW_6,  OFFSET(3),  OFFSET(1),  AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_ATV_MN_A2     (System-M (Korea)) */
    {0x00, AUTO, 0x05, 0x02, 0x05, 0x02, 0x01, 0x01, 0x01, 0x00, BW_7,  OFFSET(11), OFFSET(5),  AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_ATV_BG        (System-B/G) */
    {0x00, AUTO, 0x05, 0x02, 0x05, 0x02, 0x01, 0x01, 0x01, 0x00, BW_8,  OFFSET(2),  OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_ATV_I         (System-I) */
    {0x00, AUTO, 0x05, 0x02, 0x05, 0x02, 0x01, 0x01, 0x01, 0x00, BW_8,  OFFSET(2),  OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_ATV_DK        (System-D/K) */
    {0x00, AUTO, 0x03, 0x03, 0x06, 0x03, 0x04, 0x04, 0x04, 0x00, BW_8,  OFFSET(2),  OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_ATV_L         (System-L) */
    {0x00, AUTO, 0x03, 0x03, 0x06, 0x03, 0x04, 0x04, 0x04, 0x00, BW_8,  OFFSET(-1), OFFSET(4),  AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_ATV_L_DASH    (System-L DASH) */
    /* Digital */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x03, 0x03, 0x03, 0x00, BW_6,  OFFSET(-6), OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_8VSB      (ATSC 8VSB) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_6,  OFFSET(-6), OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_QAM       (US QAM) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_6,  OFFSET(-9), OFFSET(-5), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_ISDBT_6   (ISDB-T 6MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_7,  OFFSET(-7), OFFSET(-6), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_ISDBT_7   (ISDB-T 7MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_8,  OFFSET(-5), OFFSET(-7), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_ISDBT_8   (ISDB-T 8MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_6,  OFFSET(-8), OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBT_5    (DVB-T 5MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_6,  OFFSET(-8), OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBT_6    (DVB-T 6MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_7,  OFFSET(-6), OFFSET(-5), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBT_7    (DVB-T 7MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_8,  OFFSET(-4), OFFSET(-6), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBT_8    (DVB-T 8MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_1_7,OFFSET(-10),OFFSET(-10),AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBT2_1_7 (DVB-T2 1.7MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_6,  OFFSET(-8), OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBT2_5   (DVB-T2 5MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_6,  OFFSET(-8), OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBT2_6   (DVB-T2 6MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_7,  OFFSET(-6), OFFSET(-5), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBT2_7   (DVB-T2 7MHzBW) */
    {0x00, AUTO, 0x09, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_8,  OFFSET(-4), OFFSET(-6), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBT2_8   (DVB-T2 8MHzBW) */
    {0x00, AUTO, 0x05, 0x09, 0x09, 0x09, 0x02, 0x02, 0x02, 0x00, BW_6,  OFFSET(-6), OFFSET(-4), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBC_6    (DVB-C 6MHzBW) */
    {0x00, AUTO, 0x05, 0x09, 0x09, 0x09, 0x02, 0x02, 0x02, 0x00, BW_8,  OFFSET(-2), OFFSET(-3), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBC_8    (DVB-C 8MHzBW) */
    {0x00, AUTO, 0x03, 0x0A, 0x0A, 0x0A, 0x02, 0x02, 0x02, 0x00, BW_6,  OFFSET(-6), OFFSET(-2), AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBC2_6   (DVB-C2 6MHzBW) */
    {0x00, AUTO, 0x03, 0x0A, 0x0A, 0x0A, 0x02, 0x02, 0x02, 0x00, BW_8,  OFFSET(-2), OFFSET(0),  AUTO, AUTO, 0x00}, /**< SONY_ASCOT3_DTV_DVBC2_8   (DVB-C2 8MHzBW) */
    {0x00, AUTO, 0x04, 0x0C, 0x0C, 0x0C, 0x02, 0x02, 0x02, 0x00, BW_8,  OFFSET(2),  OFFSET(1),  AUTO, AUTO, 0x00}  /**< SONY_ASCOT3_DTV_DTMB      (DTMB) */
};

/*------------------------------------------------------------------------------
 Definitions of static functions
------------------------------------------------------------------------------*/
/**
 @brief Configure the ASCOT3 tuner from Power On to Sleep state.
*/
static sony_result_t X_pon(sony_ascot3_t *pTuner);
/**
 @brief Configure the ASCOT3 tuner for specified broadcasting system.
*/
static sony_result_t X_tune(sony_ascot3_t *pTuner, uint32_t frequencykHz, sony_ascot3_tv_system_t tvSystem, uint8_t vcoCal);
/**
 @brief The last part of tuning sequence.
*/
static sony_result_t X_tune_end(sony_ascot3_t *pTuner, sony_ascot3_tv_system_t tvSystem);
/**
 @brief Configure the ASCOT3 tuner to Power Save state.
*/
static sony_result_t X_fin(sony_ascot3_t *pTuner);
/**
 @brief Configure the ASCOT3 tuner to Oscillation Stop state.
*/
static sony_result_t X_oscdis(sony_ascot3_t *pTuner);
/**
 @brief Configure the ASCOT3 tuner back from Oscillation Stop state.
*/
static sony_result_t X_oscen(sony_ascot3_t *pTuner);
/**
 @brief Reading gain information to calculate IF and RF gain levels.
*/
static sony_result_t X_rssi(sony_ascot3_t *pTuner, int32_t *pRssiOut);

/*------------------------------------------------------------------------------
 Definitions of internal used macros
------------------------------------------------------------------------------*/
/**
 @brief Checking that the internal loop filter can be used for the broadcasting system.
*/
#define INTERNAL_LOOPFILTER_AVAILABLE(tvSystem) (SONY_ASCOT3_IS_DTV(tvSystem)\
    && ((tvSystem) != SONY_ASCOT3_DTV_DVBC_6) && ((tvSystem) != SONY_ASCOT3_DTV_DVBC_8)\
    && ((tvSystem) != SONY_ASCOT3_DTV_DVBC2_6) && ((tvSystem) != SONY_ASCOT3_DTV_DVBC2_8))

/*------------------------------------------------------------------------------
 Implementation of public functions.
------------------------------------------------------------------------------*/

sony_result_t sony_ascot3_Create(sony_ascot3_t *pTuner, sony_ascot3_xtal_t xtalFreq,
    uint8_t i2cAddress, sony_i2c_t *pI2c, uint32_t flags)
{
	printk("[sony_ascot3.c]: sony_ascot3_Create(): flags(%x)\n", flags);
    SONY_TRACE_ENTER("sony_ascot3_Create");

    if((!pTuner) || (!pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* 41MHz is only for external reference input */
    if((xtalFreq == SONY_ASCOT3_XTAL_41000KHz) && !(flags & SONY_ASCOT3_CONFIG_EXT_REF)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
    }

    /* SONY_ASCOT3_CONFIG_EXT_REF and SONY_ASCOT3_CONFIG_SLEEP_DISABLEXTAL cannot be used at the same time. */
    if((flags & SONY_ASCOT3_CONFIG_EXT_REF) && (flags & SONY_ASCOT3_CONFIG_SLEEP_DISABLEXTAL)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
    }

    pTuner->state = SONY_ASCOT3_STATE_UNKNOWN; /* Chip is not accessed for now. */
    pTuner->xtalFreq = xtalFreq;
    pTuner->pI2c = pI2c;
    pTuner->i2cAddress = i2cAddress;
    pTuner->flags = flags;
    pTuner->frequencykHz = 0;
    pTuner->tvSystem = SONY_ASCOT3_TV_SYSTEM_UNKNOWN;
    pTuner->chipId = SONY_ASCOT3_CHIP_ID_UNKNOWN;
    pTuner->pParamTable = g_param_table;
    pTuner->xosc_sel = 0x04; /* 4 x 25 = 100uA is default. */
    pTuner->xosc_cap_set = 0x30; /* 48 x 0.25 = 12pF is default. */
    pTuner->user = NULL;

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_ascot3_Initialize(sony_ascot3_t *pTuner)
{
    sony_result_t result = SONY_RESULT_OK;

	printk("[sony_ascot3.c]: sony_ascot3_Initialize(): flag(%d)\n", pTuner->flags);
    SONY_TRACE_ENTER("sony_ascot3_Initialize");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    {
        uint8_t data = 0x00;

        /* Confirm connected device is ASCOT3 */
        result = pTuner->pI2c->ReadRegister(pTuner->pI2c, pTuner->i2cAddress, 0x7F, &data, 1);
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

        if(((data & 0xF0) != 0xC0) && ((data & 0xF0) != 0xD0)){
            SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
        }
    }

    /* X_pon sequence */
    result = X_pon(pTuner);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }

    if((pTuner->flags & SONY_ASCOT3_CONFIG_SLEEP_DISABLEXTAL) && !(pTuner->flags & SONY_ASCOT3_CONFIG_EXT_REF)){
        /* Disable Xtal */
        result = X_oscdis(pTuner);
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }
    }

    pTuner->state = SONY_ASCOT3_STATE_SLEEP;
    pTuner->frequencykHz = 0;
    pTuner->tvSystem = SONY_ASCOT3_TV_SYSTEM_UNKNOWN;

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_ascot3_Tune(sony_ascot3_t *pTuner, uint32_t frequencykHz, sony_ascot3_tv_system_t tvSystem)
{
    sony_result_t result = SONY_RESULT_OK;

	printk("[sony_ascot3.c]: sony_ascot3_Tune(): system(%d) freq(%d)\n", tvSystem, frequencykHz);
    SONY_TRACE_ENTER("sony_ascot3_Tune");

    /* Argument check */
    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    if(!SONY_ASCOT3_IS_ATV(tvSystem) && !SONY_ASCOT3_IS_DTV(tvSystem)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* State check */
    if((pTuner->state != SONY_ASCOT3_STATE_SLEEP) && (pTuner->state != SONY_ASCOT3_STATE_ACTIVE)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_SW_STATE);
    }

    /* Rough frequency range check */
    if((frequencykHz < 1000) || (frequencykHz > 1200000)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_RANGE);
    }
    if(pTuner->state == SONY_ASCOT3_STATE_SLEEP){
        /* Set system to "Unknown". (for safe) */
        pTuner->tvSystem = SONY_ASCOT3_TV_SYSTEM_UNKNOWN;

        if((pTuner->flags & SONY_ASCOT3_CONFIG_SLEEP_DISABLEXTAL) && !(pTuner->flags & SONY_ASCOT3_CONFIG_EXT_REF)){
            /* Enable Xtal */
            result = X_oscen(pTuner);
            if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }
        }
    }

    /* Broadcasting system dependent setting and tuning. */
    result = X_tune(pTuner, frequencykHz, tvSystem, 1);
    if(result != SONY_RESULT_OK){
        SONY_TRACE_RETURN(result);
    }

    pTuner->state = SONY_ASCOT3_STATE_ACTIVE;
    pTuner->frequencykHz = frequencykHz;
    pTuner->tvSystem = tvSystem;

	printk("[sony_ascot3.c]: sony_ascot3_Tune(): tvSystem(%d)[%d/%d] frequencykHz(%d)\n", pTuner->tvSystem, SONY_ASCOT3_DTV_DVBC_6, SONY_ASCOT3_DTV_DVBC_8, pTuner->frequencykHz);
    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_ascot3_TuneEnd(sony_ascot3_t *pTuner)
{
    sony_result_t result = SONY_RESULT_OK;

	printk("[sony_ascot3.c]: sony_ascot3_TuneEnd(): state(%d)\n", pTuner->state);
    SONY_TRACE_ENTER("sony_ascot3_TuneEnd");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    if(pTuner->state != SONY_ASCOT3_STATE_ACTIVE){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_SW_STATE);
    }

    result = X_tune_end(pTuner, pTuner->tvSystem);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_ascot3_ShiftFRF(sony_ascot3_t *pTuner, uint32_t frequencykHz)
{
    sony_result_t result = SONY_RESULT_OK;

	printk("[sony_ascot3.c]: sony_ascot3_ShiftFRF(): freq(%d)\n", frequencykHz);
    SONY_TRACE_ENTER("sony_ascot3_ShiftFRF");

    /* Argument check */
    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* State check */
    if(pTuner->state != SONY_ASCOT3_STATE_ACTIVE){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_SW_STATE);
    }

    /* In digital tuning, step size is 25kHz. (+ rounding) */
    if((frequencykHz < 1000) || (frequencykHz > 1200000)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_RANGE);
    }

    /* Tune using current system, no VCO calibration */
    result = X_tune(pTuner, frequencykHz, pTuner->tvSystem, 0);
    if(result != SONY_RESULT_OK){
        SONY_TRACE_RETURN(result);
    }

    pTuner->frequencykHz = frequencykHz;

    SONY_SLEEP(10);

    result = X_tune_end(pTuner, pTuner->tvSystem);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_ascot3_Sleep(sony_ascot3_t *pTuner)
{
    sony_result_t result = SONY_RESULT_OK;

	printk("[sony_ascot3.c]: sony_ascot3_Sleep():\n");
    SONY_TRACE_ENTER("sony_ascot3_Sleep");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* State check */
    if((pTuner->state != SONY_ASCOT3_STATE_SLEEP) && (pTuner->state != SONY_ASCOT3_STATE_ACTIVE)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_SW_STATE);
    }

    if(pTuner->state == SONY_ASCOT3_STATE_SLEEP){
        /* Nothing to do */
        SONY_TRACE_RETURN(SONY_RESULT_OK);
    }

    /* To Power Save state */
    result = X_fin(pTuner);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }

    if((pTuner->flags & SONY_ASCOT3_CONFIG_SLEEP_DISABLEXTAL) && !(pTuner->flags & SONY_ASCOT3_CONFIG_EXT_REF)){
        /* Disable Xtal */
        result = X_oscdis(pTuner);
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }
    }

    pTuner->state = SONY_ASCOT3_STATE_SLEEP;
    pTuner->frequencykHz = 0;
    pTuner->tvSystem = SONY_ASCOT3_TV_SYSTEM_UNKNOWN;

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_ascot3_SetGPO(sony_ascot3_t *pTuner, uint8_t id, uint8_t val)
{
    sony_result_t result = SONY_RESULT_OK;

	printk("[sony_ascot3.c]: sony_ascot3_SetGPO(): id(%d) value(%d)\n", id, val);
    SONY_TRACE_ENTER("sony_ascot3_SetGPO");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    switch(id){
    case 0:
        result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x85, (uint8_t)(val ? 0x01 : 0x00));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
        break;
    case 1:
        result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x86, (uint8_t)(val ? 0x01 : 0x00));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
        break;
    default:
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_ascot3_SetRfExtCtrl(sony_ascot3_t *pTuner, uint8_t enable)
{
    sony_result_t result = SONY_RESULT_OK;

	printk("[sony_ascot3.c]: sony_ascot3_SetRfExtCtrl(): enable(%d)\n", enable);
    SONY_TRACE_ENTER("sony_ascot3_SetRfExtCtrl");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* State check */
    if(pTuner->state != SONY_ASCOT3_STATE_ACTIVE){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_SW_STATE);
    }

    /* Only for CXD2871 */
    if(pTuner->chipId != SONY_ASCOT3_CHIP_ID_2871){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
    }

    /* RF_EXT bit setting */
    result = sony_i2c_SetRegisterBits(pTuner->pI2c, pTuner->i2cAddress, 0x67, (uint8_t)(enable ? 0x01 : 0x00), 0x01);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_ascot3_ReadRssi(sony_ascot3_t *pTuner, int32_t *pRssi)
{
    sony_result_t result = SONY_RESULT_OK;
    int32_t rssiOut;

	printk("[sony_ascot3.c]: sony_ascot3_ReadRssi():\n");
    SONY_TRACE_ENTER("sony_ascot3_ReadRssi");

    if((!pTuner) || (!pTuner->pI2c) || (!pRssi)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    if(pTuner->state != SONY_ASCOT3_STATE_ACTIVE){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_SW_STATE);
    }

    result = X_rssi(pTuner, &rssiOut);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }

    /*
      RSSI_OUT[10:0] is (dBm x 8) integer value. */
    if(rssiOut >= 0){
        *pRssi = (rssiOut * 100 + 4) / 8;
    }else{
        *pRssi = (rssiOut * 100 - 4) / 8;
    }

    /* RSSI adjustment (RSSI_OFS) */
    if(pTuner->frequencykHz > 532000){
        *pRssi += 100;
    }else if(pTuner->frequencykHz > 464000){
        *pRssi += 300;
    }else if(pTuner->frequencykHz > 350000){
        *pRssi += 100;
    }else if(pTuner->frequencykHz > 320000){
        *pRssi += 300;
    }else if(pTuner->frequencykHz > 215000){
        *pRssi += 100;
    }else if(pTuner->frequencykHz > 184000){
        *pRssi += 300;
    }else if(pTuner->frequencykHz > 172000){
        *pRssi += 400;
    }else if(pTuner->frequencykHz > 150000){
        *pRssi += 300;
    }else{
        *pRssi += 200;
    }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

sony_result_t sony_ascot3_RFFilterConfig(sony_ascot3_t *pTuner, uint8_t coeff, uint8_t offset)
{
    sony_result_t result = SONY_RESULT_OK;

	printk("[sony_ascot3.c]: sony_ascot3_RFFilterConfig(): coeff(%d) offset(%d)\n", coeff, offset);
    SONY_TRACE_ENTER("sony_ascot3_RFFilterConfig");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    if((pTuner->state != SONY_ASCOT3_STATE_SLEEP) && (pTuner->state != SONY_ASCOT3_STATE_ACTIVE)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_SW_STATE);
    }

    if(pTuner->state == SONY_ASCOT3_STATE_SLEEP){
        if((pTuner->flags & SONY_ASCOT3_CONFIG_SLEEP_DISABLEXTAL) && !(pTuner->flags & SONY_ASCOT3_CONFIG_EXT_REF)){
            /* Enable Xtal */
            result = X_oscen(pTuner);
            if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }
        }
    }

    /* Clock enable for internal logic block, CPU wake-up */
    {
        const uint8_t cdata[2] = {0xC4, 0x40};
        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x87, cdata, sizeof(cdata));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    {
        uint8_t data[3];

        /* Write multiplier */
        data[0] = coeff;
        data[1] = 0x49;
        data[2] = 0x03;
        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x16, data, sizeof(data));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

        SONY_SLEEP(1);

        /* Write offset */
        data[0] = offset;
        data[1] = 0x4B;
        data[2] = 0x03;
        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x16, data, sizeof(data));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

        SONY_SLEEP(1);
    }

    /* Standby setting for CPU */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x88, 0x00);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    /* Standby setting for internal logic block */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x87, 0xC0);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    if(pTuner->state == SONY_ASCOT3_STATE_SLEEP){
        if((pTuner->flags & SONY_ASCOT3_CONFIG_SLEEP_DISABLEXTAL) && !(pTuner->flags & SONY_ASCOT3_CONFIG_EXT_REF)){
            /* Disable Xtal */
            result = X_oscdis(pTuner);
            if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }
        }
    }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

/*------------------------------------------------------------------------------
 Implementation of static functions
------------------------------------------------------------------------------*/

static sony_result_t X_pon(sony_ascot3_t *pTuner)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER("X_pon");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* 0x99, 0x9A */
    {
        /* Initial setting for internal logic block */
        const uint8_t cdata[] = {0x7A, 0x01};
        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x99, cdata, sizeof(cdata));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* 0x81 - 0x94 */
    {
        uint8_t data[20];

        /* Frequency setting for crystal oscillator */
        switch(pTuner->xtalFreq){
        case SONY_ASCOT3_XTAL_16000KHz:
            data[0] = 0x10;
            break;
        case SONY_ASCOT3_XTAL_20500KHz:
            data[0] = 0xD4;
            break;
        case SONY_ASCOT3_XTAL_24000KHz:
            data[0] = 0x18;
            break;
        case SONY_ASCOT3_XTAL_41000KHz:
            data[0] = 0x69;
            break;
        default:
            SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
        }

        /* Driver current setting for crystal oscillator */
        /* Load capacitance setting for crystal oscillator */
        if(pTuner->flags & SONY_ASCOT3_CONFIG_EXT_REF){
            /* XOSC_APC_EN = 0, XOSC_SEL= 0uA */
            data[1] = 0x00;
            /* XOSC_CALC_EN = 0, XOSC_CAP_SET = 0pF */
            data[2] = 0x00;
        }else{
            /* XOSC_APC_EN = 1, XOSC_SEL = xosc_sel (sony_ascot3_t member) */
            data[1] = (uint8_t)(0x80 | (pTuner->xosc_sel & 0x1F));
            /* XOSC_CALC_EN = 1, XOSC_CAP_SET = xosc_cap_set (sony_ascot3_t member) */
            data[2] = (uint8_t)(0x80 | (pTuner->xosc_cap_set & 0x7F));
        }

        /* Setting for REFOUT signal output */
        switch(pTuner->flags & SONY_ASCOT3_CONFIG_REFOUT_MASK){
        case 0:
            data[3] = 0x00; /* REFOUT_EN = 0, REFOUT_CNT = 0 */
            break;
        case SONY_ASCOT3_CONFIG_REFOUT_500mVpp:
            data[3] = 0x80; /* REFOUT_EN = 1, REFOUT_CNT = 0 */
            break;
        case SONY_ASCOT3_CONFIG_REFOUT_400mVpp:
            data[3] = 0x81; /* REFOUT_EN = 1, REFOUT_CNT = 1 */
            break;
        case SONY_ASCOT3_CONFIG_REFOUT_600mVpp:
            data[3] = 0x82; /* REFOUT_EN = 1, REFOUT_CNT = 2 */
            break;
        case SONY_ASCOT3_CONFIG_REFOUT_800mVpp:
            data[3] = 0x83; /* REFOUT_EN = 1, REFOUT_CNT = 3 */
            break;
        default:
            SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
        }

        /* GPIO0, GPIO1 port setting */
        /* GPIO setting should be done by sony_ascot3_SetGPO after initialization */
        data[4] = 0x00;
        data[5] = 0x00;

        /* Clock enable for internal logic block */
        data[6] = 0xC4;

        /* Start CPU boot-up */
        data[7] = 0x40;

        /* For burst-write */
        data[8] = 0x10;

        /* Setting for internal RFAGC */
        if(pTuner->flags & SONY_ASCOT3_CONFIG_OVERLOAD_STANDARD){
            data[9] = 0x00;
            data[10] = 0x01;
            data[11] = 0x75;
        }else{
            data[9] = 0x00;
            data[10] = 0x45;
            data[11] = 0x56;
        }

        /* Setting for analog block */
        data[12] = 0x07;

        /* Initial setting for internal analog block */
        data[13] = 0x1C;
        data[14] = 0x3F;
        data[15] = 0x02;
        data[16] = 0x10;
        data[17] = 0x20;
        data[18] = 0x0A;
        data[19] = 0x00;

        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x81, data, sizeof(data));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* Setting for internal RFAGC */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x9B, 0x00);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    /* Wait 10ms */
    SONY_SLEEP(10);

    /* Check CPU_STT/CPU_ERR */
    {
        uint8_t rdata[2];

        result = pTuner->pI2c->ReadRegister(pTuner->pI2c, pTuner->i2cAddress, 0x1A, rdata, sizeof(rdata));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

        if(rdata[0] != 0x00){
            SONY_TRACE_RETURN(SONY_RESULT_ERROR_HW_STATE); /* CPU_STT != 0x00 */
        }

#ifndef SONY_ASCOT3_IGNORE_NVM_ERROR /* For no NVM tuner evaluation */
        if((rdata[1] & 0x3F) != 0x00){
            SONY_TRACE_RETURN(SONY_RESULT_ERROR_HW_STATE); /* CPU_ERR[5:0] != 0x00 (NVM Error) */
        }
#endif /* SONY_ASCOT3_IGNORE_NVM_ERROR */
    }

    /* Chip ID auto detection */
    {
        const uint8_t cdata[] = {0x96, 0x06}; /* 0x17, 0x18 */
        uint8_t data = 0;

        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x17, cdata, sizeof(cdata));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

        SONY_SLEEP(1);

        result = pTuner->pI2c->ReadRegister(pTuner->pI2c, pTuner->i2cAddress, 0x19, &data, 1);
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

        if(data & 0x40){
            pTuner->chipId = SONY_ASCOT3_CHIP_ID_2871;
        }else{
            pTuner->chipId = SONY_ASCOT3_CHIP_ID_2872;
        }
    }

    /* VCO current setting */
    {
        const uint8_t cdata[] = {0x8D, 0x06}; /* 0x17, 0x18 */
        uint8_t data = 0;

        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x17, cdata, sizeof(cdata));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

        SONY_SLEEP(1);

        result = pTuner->pI2c->ReadRegister(pTuner->pI2c, pTuner->i2cAddress, 0x19, &data, 1);
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

        result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x95, (uint8_t)((data >> 4) & 0x0F));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    result = X_fin(pTuner);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(result); }

    /* Load capacitance control setting for crystal oscillator (0x80) */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x80, 0x01);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

static sony_result_t X_tune(sony_ascot3_t *pTuner, uint32_t frequencykHz, sony_ascot3_tv_system_t tvSystem, uint8_t vcoCal)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER("X_tune");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    if(vcoCal){
        /* Disable IF signal output (IF_OUT_SEL setting) */
        result = sony_i2c_SetRegisterBits(pTuner->pI2c, pTuner->i2cAddress, 0x74, 0x02, 0x03);
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* 0x85 - 0x86 */
    /* GPIO0, GPIO1 is changed by sony_ascot3_SetGPO */

    /* Clock enable for internal logic block, CPU wake-up */
    {
        const uint8_t cdata[2] = {0xC4, 0x40};
        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x87, cdata, sizeof(cdata));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* 0x91 - 0x92 */
    {
        uint8_t data[2];

        /* Initial setting for internal analog block (0x91, 0x92) */
        if((tvSystem == SONY_ASCOT3_DTV_DVBC_6) || (tvSystem == SONY_ASCOT3_DTV_DVBC_8)){
            data[0] = 0x16;
            data[1] = 0x26;
        }else{
            data[0] = 0x10;
            data[1] = 0x20;
        }

        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x91, data, sizeof(data));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* 0x9C - 0x9D */
    {
        uint8_t data[2];
        /* Setting for analog block */
        if((pTuner->flags & SONY_ASCOT3_CONFIG_LOOPFILTER_INTERNAL) && INTERNAL_LOOPFILTER_AVAILABLE(tvSystem)){
            data[0] = 0x90;
        }else{
            data[0] = 0x00;
        }

        data[1] = (uint8_t)(pTuner->pParamTable[tvSystem].IS_LOWERLOCAL & 0x01);

        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x9C, data, sizeof(data));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* 0x5E - 0x66 */
    {
        uint8_t data[9];

        /* Enable for analog block */
        data[0] = 0xEE;
        data[1] = 0x02;
        data[2] = 0x1E;

        /* Tuning setting for CPU */
        if(vcoCal){
            data[3] = 0x67;
        }else{
            data[3] = 0x47;
        }

        /* Setting for PLL reference divider (REF_R) (0x62) */
        if(SONY_ASCOT3_IS_ATV(tvSystem)){
            /* Analog */
            switch(pTuner->xtalFreq){
            case SONY_ASCOT3_XTAL_16000KHz:
                data[4] = 0x10;
                break;
            case SONY_ASCOT3_XTAL_20500KHz:
                data[4] = 0x14;
                break;
            case SONY_ASCOT3_XTAL_24000KHz:
                data[4] = 0x18;
                break;
            case SONY_ASCOT3_XTAL_41000KHz:
                data[4] = 0x28;
                break;
            default:
                SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
            }
        }else{
            /* Digital (except for DVB-C) */
            switch(pTuner->xtalFreq){
            case SONY_ASCOT3_XTAL_16000KHz:
                data[4] = 0x02;
                break;
            case SONY_ASCOT3_XTAL_20500KHz:
                data[4] = 0x02;
                break;
            case SONY_ASCOT3_XTAL_24000KHz:
                data[4] = 0x03;
                break;
            case SONY_ASCOT3_XTAL_41000KHz:
                data[4] = 0x05;
                break;
            default:
                SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
            }
        }

        /* Tuning setting for analog block */
        if((pTuner->flags & SONY_ASCOT3_CONFIG_LOOPFILTER_INTERNAL) && INTERNAL_LOOPFILTER_AVAILABLE(tvSystem)){
            if(pTuner->xtalFreq == SONY_ASCOT3_XTAL_20500KHz){
                data[5] = 0x2C;
            }else{
                data[5] = 0x38;
            }
            data[6] = 0x1E;
            data[7] = 0x02;
            data[8] = 0x24;
        }else if(SONY_ASCOT3_IS_ATV(tvSystem)){
            data[5] = 0x20;
            data[6] = 0x78;
            data[7] = 0x08;
            data[8] = 0x08;
        }else if((tvSystem == SONY_ASCOT3_DTV_DVBC_6) || (tvSystem == SONY_ASCOT3_DTV_DVBC_8)){
            if(pTuner->xtalFreq == SONY_ASCOT3_XTAL_20500KHz){
                data[5] = 0x40;
            }else{
                data[5] = 0x50;
            }
            data[6] = 0x78;
            data[7] = 0x08;
            data[8] = 0x30;
        }else{
            if(pTuner->xtalFreq == SONY_ASCOT3_XTAL_20500KHz){
                data[5] = 0x8C;
            }else{
                data[5] = 0xAF;
            }
            data[6] = 0x78;
            data[7] = 0x08;
            data[8] = 0x30;
        }

        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x5E, data, sizeof(data));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* 0x67 */
    /* LT_AMP_EN should be 0 for CXD2871 */
    if(pTuner->chipId == SONY_ASCOT3_CHIP_ID_2871){
        result = sony_i2c_SetRegisterBits(pTuner->pI2c, pTuner->i2cAddress, 0x67, 0x00, 0x02);
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* 0x68 - 0x78 */
    {
        uint8_t data[17];

        /* Setting for IFOUT_LIMIT (0x68) */
        data[0] = (uint8_t)(pTuner->pParamTable[tvSystem].OUTLMT & 0x03);

        /* Setting for IF BPF buffer gain (0x69) */
        /* RF_GAIN setting */
        if(pTuner->pParamTable[tvSystem].RF_GAIN == AUTO){
            data[1] = 0x80; /* RF_GAIN_SEL = 1 */
        }else{
            data[1] = (uint8_t)((pTuner->pParamTable[tvSystem].RF_GAIN << 4) & 0x70);
        }

        /* IF_BPF_GC setting */
        data[1] |= (uint8_t)(pTuner->pParamTable[tvSystem].IF_BPF_GC & 0x0F);

        /* Setting for internal RFAGC (0x6A - 0x6C) */
        data[2] = 0x00;
        if(frequencykHz <= 172000){
            data[3] = (uint8_t)(pTuner->pParamTable[tvSystem].RFOVLD_DET_LV1_VL & 0x0F);
            data[4] = (uint8_t)(pTuner->pParamTable[tvSystem].IFOVLD_DET_LV_VL & 0x07);
        }else if(frequencykHz <= 464000){
            data[3] = (uint8_t)(pTuner->pParamTable[tvSystem].RFOVLD_DET_LV1_VH & 0x0F);
            data[4] = (uint8_t)(pTuner->pParamTable[tvSystem].IFOVLD_DET_LV_VH & 0x07);
        }else{
            data[3] = (uint8_t)(pTuner->pParamTable[tvSystem].RFOVLD_DET_LV1_U & 0x0F);
            data[4] = (uint8_t)(pTuner->pParamTable[tvSystem].IFOVLD_DET_LV_U & 0x07);
        }
        data[4] |= 0x20;

        /* Setting for IF frequency and bandwidth */

        /* IF filter center frequency offset (IF_BPF_F0) (0x6D) */
        data[5] = (uint8_t)((pTuner->pParamTable[tvSystem].IF_BPF_F0 << 4) & 0x30);

        /* IF filter band width (BW) (0x6D) */
        data[5] |= (uint8_t)(pTuner->pParamTable[tvSystem].BW & 0x03);

        /* IF frequency offset value (FIF_OFFSET) (0x6E) */
        data[6] = (uint8_t)(pTuner->pParamTable[tvSystem].FIF_OFFSET & 0x1F);

        /* IF band width offset value (BW_OFFSET) (0x6F) */
        data[7] = (uint8_t)(pTuner->pParamTable[tvSystem].BW_OFFSET & 0x1F);

        /* RF tuning frequency setting (0x70 - 0x72) */
        data[8]  = (uint8_t)(frequencykHz & 0xFF);         /* FRF_L */
        data[9]  = (uint8_t)((frequencykHz >> 8) & 0xFF);  /* FRF_M */
        data[10] = (uint8_t)((frequencykHz >> 16) & 0x0F); /* FRF_H (bit[3:0]) */

        if(tvSystem == SONY_ASCOT3_ATV_L_DASH){
            data[10] |= 0x40; /* IS_L_DASH (bit[6]) */
        }

        if(SONY_ASCOT3_IS_ATV(tvSystem)){
            data[10] |= 0x80; /* IS_FP (bit[7]) */
        }

        /* Tuning command (0x73) */
        if(vcoCal){
            data[11] = 0xFF;
        }else{
            data[11] = 0x8F;
        }

        /* Enable IF output, AGC and IFOUT pin selection */
        if(pTuner->chipId == SONY_ASCOT3_CHIP_ID_2871){
            data[12] = 0;

            if(pTuner->pParamTable[tvSystem].AGC_SEL == AUTO){
                /* AGC pin setting by config flags */
                if(SONY_ASCOT3_IS_ATV(tvSystem)){
                    /* Analog */
                    if(pTuner->flags & SONY_ASCOT3_CONFIG_AGC2_ATV){
                        data[12] |= 0x10;
                    }
                }else{
                    /* Digital */
                    if(pTuner->flags & SONY_ASCOT3_CONFIG_AGC2_DTV){
                        data[12] |= 0x10;
                    }
                }
            }else{
                /* AGC pin setting from parameter table */
                data[12] |= (uint8_t)((pTuner->pParamTable[tvSystem].AGC_SEL << 4) & 0x30);
            }

            if(pTuner->pParamTable[tvSystem].IF_OUT_SEL == AUTO){
                /* IFOUT pin setting by config flags */
                if(SONY_ASCOT3_IS_ATV(tvSystem)){
                    /* Analog */
                    if(pTuner->flags & SONY_ASCOT3_CONFIG_IF2_ATV){
                        data[12] |= 0x01;
                    }
                }else{
                    /* Digital */
                    if(pTuner->flags & SONY_ASCOT3_CONFIG_IF2_DTV){
                        data[12] |= 0x01;
                    }
                }
            }else{
                /* IFOUT pin setting from parameter table */
                data[12] |= (uint8_t)(pTuner->pParamTable[tvSystem].IF_OUT_SEL & 0x03);
            }
        }else if(pTuner->chipId == SONY_ASCOT3_CHIP_ID_2872){
            data[12] = 0x11;
        }else{
            SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
        }

        /* Tuning setting for analog block (0x75 - 0x78) */
        if(SONY_ASCOT3_IS_ATV(tvSystem) || (tvSystem == SONY_ASCOT3_DTV_DVBC_6) || (tvSystem == SONY_ASCOT3_DTV_DVBC_8)){
            data[13] = 0xD9;
            data[14] = 0x0F;
            data[15] = 0x25;
            data[16] = 0x87;
        }else{
            data[13] = 0x99;
            data[14] = 0x00;
            data[15] = 0x24;
            data[16] = 0x87;
        }

        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x68, data, sizeof(data));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

static sony_result_t X_tune_end(sony_ascot3_t *pTuner, sony_ascot3_tv_system_t tvSystem)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER("X_tune_end");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* Standby setting for CPU (0x88) */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x88, 0x00);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    /* Standby setting for internal logic block */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x87, 0xC0);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

static sony_result_t X_fin(sony_ascot3_t *pTuner)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER("X_fin");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* Loop Through setting And RFIN matching in Power Save */
    {
        uint8_t data = 0;

        if(pTuner->chipId == SONY_ASCOT3_CHIP_ID_2871){
            if(pTuner->flags & SONY_ASCOT3_CONFIG_RFIN_MATCHING_ENABLE){
                data = 0x02;
            }else{
                data = 0x00;
            }
        }else if(pTuner->chipId == SONY_ASCOT3_CHIP_ID_2872){
            if(pTuner->flags & SONY_ASCOT3_CONFIG_LOOPTHRU_ENABLE){
                data = 0x06;
            }else if(pTuner->flags & SONY_ASCOT3_CONFIG_RFIN_MATCHING_ENABLE){
                data = 0x02;
            }else{
                data = 0x00;
            }
        }else{
            SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
        }

        result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x67, data);
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* Disable IF signal output (IF_OUT_SEL setting) */
    result = sony_i2c_SetRegisterBits(pTuner->pI2c, pTuner->i2cAddress, 0x74, 0x02, 0x03);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    /* Power save setting for analog block */
    {
        const uint8_t cdata[] = {0x15, 0x00, 0x00};
        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x5E, cdata, sizeof(cdata));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* Standby setting for CPU */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x88, 0x00);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    /* Standby setting for internal logic block  */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x87, 0xC0);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

static sony_result_t X_oscdis(sony_ascot3_t *pTuner)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER("X_oscdis");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* Setting for REFOUT signal output (0x84) */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x84, 0x00);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    /* Driver current setting for crystal oscillator (0x82) */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x82, 0x00);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

static sony_result_t X_oscen(sony_ascot3_t *pTuner)
{
    sony_result_t result = SONY_RESULT_OK;
    uint8_t data = 0;

    SONY_TRACE_ENTER("X_oscen");

    if((!pTuner) || (!pTuner->pI2c)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* Driver current setting for crystal oscillator (0x82) */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x82, 0x9F);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    /* Setting for REFOUT signal output (0x84) */
    switch(pTuner->flags & SONY_ASCOT3_CONFIG_REFOUT_MASK){
    case 0:
        data = 0x00; /* REFOUT_EN = 0, REFOUT_CNT = 0 */
        break;
    case SONY_ASCOT3_CONFIG_REFOUT_500mVpp:
        data = 0x80; /* REFOUT_EN = 1, REFOUT_CNT = 0 */
        break;
    case SONY_ASCOT3_CONFIG_REFOUT_400mVpp:
        data = 0x81; /* REFOUT_EN = 1, REFOUT_CNT = 1 */
        break;
    case SONY_ASCOT3_CONFIG_REFOUT_600mVpp:
        data = 0x82; /* REFOUT_EN = 1, REFOUT_CNT = 2 */
        break;
    case SONY_ASCOT3_CONFIG_REFOUT_800mVpp:
        data = 0x83; /* REFOUT_EN = 1, REFOUT_CNT = 3 */
        break;
    default:
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_NOSUPPORT);
    }

    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x84, data);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    SONY_SLEEP(10);

    /* Driver current setting for crystal oscillator (0x82) */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x82,
        (uint8_t)(0x80 | (pTuner->xosc_sel & 0x1F)));
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

static sony_result_t X_rssi(sony_ascot3_t *pTuner, int32_t *pRssiOut)
{
    sony_result_t result = SONY_RESULT_OK;

    SONY_TRACE_ENTER("X_rssi");

    if((!pTuner) || (!pTuner->pI2c) || (!pRssiOut)){
        SONY_TRACE_RETURN(SONY_RESULT_ERROR_ARG);
    }

    /* Clock enable for internal logic block, CPU wake-up */
    {
        const uint8_t cdata[2] = {0xC4, 0x40};
        result = pTuner->pI2c->WriteRegister(pTuner->pI2c, pTuner->i2cAddress, 0x87, cdata, sizeof(cdata));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }
    }

    /* Enable RSSI calculation */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x73, 0x30);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    SONY_SLEEP(5);

    {
        uint8_t data[2];

        /* The result of RSSI calculation */
        result = pTuner->pI2c->ReadRegister(pTuner->pI2c, pTuner->i2cAddress, 0x7A, data, sizeof(data));
        if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

        *pRssiOut = sony_Convert2SComplement((data[1] << 3) | ((data[0] >> 5) & 0x07), 11);
    }

    /* Standby setting for CPU */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x88, 0x00);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    /* Standby setting for internal logic block */
    result = pTuner->pI2c->WriteOneRegister(pTuner->pI2c, pTuner->i2cAddress, 0x87, 0xC0);
    if(result != SONY_RESULT_OK){ SONY_TRACE_RETURN(SONY_RESULT_ERROR_I2C); }

    SONY_TRACE_RETURN(SONY_RESULT_OK);
}

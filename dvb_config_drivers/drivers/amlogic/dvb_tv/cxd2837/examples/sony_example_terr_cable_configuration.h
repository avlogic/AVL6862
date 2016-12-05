/*------------------------------------------------------------------------------
  Copyright 2012 Sony Corporation

  Last Updated  : $Date:: 2012-06-01 13:26:58 #$
  File Revision : $Revision:: 5431 $
------------------------------------------------------------------------------*/
/**
 @file    sony_example_terr_cable_configuration.h

          This file provides the HW configuration settings used by each terrestrial
          and cable example.  This file should be modified to suit your implementation.
*/
/*----------------------------------------------------------------------------*/
#ifndef SONY_EXAMPLE_TERR_CABLE_CONFIGURATION_H
#define SONY_EXAMPLE_TERR_CABLE_CONFIGURATION_H

#include "sony_common.h"
#include "sony_demod.h"
#include "sony_ascot2d.h"
#include "sony_ascot2e.h"
#include "sony_ascot3.h"

/*------------------------------------------------------------------------------
 High level example setting
------------------------------------------------------------------------------*/
/* Select one of the following terrestrial / cable tuners */
//#define SONY_EXAMPLE_TUNER_ASCOT2D
#define SONY_EXAMPLE_TUNER_ASCOT2E
//#define SONY_EXAMPLE_TUNER_ASCOT3
//#define SONY_EXAMPLE_TUNER_OTHER

#define MONITOR_LOOP_COUNT  1   /* Number of times to run the channel monitors */


#ifdef SONY_EXAMPLE_TUNER_ASCOT2D
/*------------------------------------------------------------------------------
CXD2831 - ASCOT2D
------------------------------------------------------------------------------*/
#define SONY_EXAMPLE_TUNER_XTAL             16
#define SONY_EXAMPLE_TUNER_I2C_ADDRESS      SONY_ASCOT2D_ADDRESS
#define SONY_EXAMPLE_TUNER_OPTIMIZE         SONY_DEMOD_TUNER_OPTIMIZE_ASCOT2D
#define SONY_EXAMPLE_TUNER_OFFSET_CUTOFF    SONY_TUNER_ASCOT2D_OFFSET_CUTOFF_HZ

/* Select the tuner configuration flags from the list below based on your
 * implementation.  For further details on each setting refer to the 
 * sony_ascot2d.h header file:
 *
 * SONY_ASCOT2D_CONFIG_EXT_REF             Use external Xtal. Should be used for slave ASCOT2D in Xtal shared system.                                          
 * SONY_ASCOT2D_CONFIG_SLEEP_DISABLEXTAL   Disable Xtal in Sleep state. Should NOT be used for Xtal shared system.
 * SONY_ASCOT2D_CONFIG_NON_LPN             Non Low PN component setting.
 * SONY_ASCOT2D_CONFIG_IFAGCSEL_ALL1       IF/AGC 1 is used for both Analog and Digital
 * SONY_ASCOT2D_CONFIG_IFAGCSEL_ALL2       IF/AGC 2 is used for both Analog and Digital
 * SONY_ASCOT2D_CONFIG_IFAGCSEL_A1D2       IF/AGC 1 is used for Analog, 2 is used for Digital
 * SONY_ASCOT2D_CONFIG_IFAGCSEL_D1A2       IF/AGC 1 is used for Digital, 2 is used for Analog
 * SONY_ASCOT2D_CONFIG_NVMSEL_0_1          NVM Bank 0/1 (default)
 * SONY_ASCOT2D_CONFIG_NVMSEL_0_3          NVM Bank 0/3
 * SONY_ASCOT2D_CONFIG_NVMSEL_AUTO         NVM Bank Auto Select
 *
 * Multiple flags can be defined using the | operator, e.g:
 *   #define SONY_EXAMPLE_TUNER_FLAGS   (SONY_ASCOT2D_CONFIG_EXT_REF | SONY_ASCOT2D_CONFIG_IFAGCSEL_ALL2)
 */
#define SONY_EXAMPLE_TUNER_FLAGS            0

#elif defined SONY_EXAMPLE_TUNER_ASCOT2E
/*------------------------------------------------------------------------------
CXD2861 - ASCOT2E
------------------------------------------------------------------------------*/
#define SONY_EXAMPLE_TUNER_XTAL             16
#define SONY_EXAMPLE_TUNER_I2C_ADDRESS      SONY_ASCOT2E_ADDRESS
#define SONY_EXAMPLE_TUNER_OPTIMIZE         SONY_DEMOD_TUNER_OPTIMIZE_ASCOT2E
#define SONY_EXAMPLE_TUNER_OFFSET_CUTOFF    SONY_TUNER_ASCOT2E_OFFSET_CUTOFF_HZ

/* Select the tuner configuration flags from the list below based on your
 * implementation.  For further details on each setting refer to the 
 * sony_ascot2e.h header file:
 *
 * SONY_ASCOT2E_CONFIG_EXT_REF             Use external Xtal. Should be used for slave ASCOT2E in Xtal shared system.                                          
 * SONY_ASCOT2E_CONFIG_SLEEP_DISABLEXTAL   Disable Xtal in Sleep state. Should NOT be used for Xtal shared system.
 * SONY_ASCOT2E_CONFIG_NON_LPN             Non Low PN component setting.
 * SONY_ASCOT2E_CONFIG_IFAGCSEL_ALL1       IF/AGC 1 is used for both Analog and Digital
 * SONY_ASCOT2E_CONFIG_IFAGCSEL_ALL2       IF/AGC 2 is used for both Analog and Digital
 * SONY_ASCOT2E_CONFIG_IFAGCSEL_A1D2       IF/AGC 1 is used for Analog, 2 is used for Digital
 * SONY_ASCOT2E_CONFIG_IFAGCSEL_D1A2       IF/AGC 1 is used for Digital, 2 is used for Analog
 *
 * Multiple flags can be defined using the | operator, e.g:
 *   #define SONY_EXAMPLE_TUNER_FLAGS   (SONY_ASCOT2E_CONFIG_EXT_REF | SONY_ASCOT2E_CONFIG_IFAGCSEL_ALL2)
 */
#define SONY_EXAMPLE_TUNER_FLAGS            0

#elif defined SONY_EXAMPLE_TUNER_ASCOT3
/*------------------------------------------------------------------------------
CXD2871/72 - ASCOT3
------------------------------------------------------------------------------*/
#define SONY_EXAMPLE_TUNER_XTAL             SONY_ASCOT3_XTAL_16000KHz
#define SONY_EXAMPLE_TUNER_I2C_ADDRESS      SONY_ASCOT3_ADDRESS
#define SONY_EXAMPLE_TUNER_OPTIMIZE         SONY_DEMOD_TUNER_OPTIMIZE_ASCOT3
#define SONY_EXAMPLE_TUNER_OFFSET_CUTOFF    SONY_TUNER_ASCOT3_OFFSET_CUTOFF_HZ

/* Select the tuner configuration flags from the list below based on your
 * implementation.  For further details on each setting refer to the 
 * sony_ascot2e.h header file:
 *
 * SONY_ASCOT3_CONFIG_EXT_REF               Use external Xtal. Should be used for slave ASCOT3 in Xtal shared system.
 * SONY_ASCOT3_CONFIG_SLEEP_DISABLEXTAL     Disable Xtal in Sleep state. Should NOT be used for Xtal shared system.
 * SONY_ASCOT3_CONFIG_OVERLOAD_STANDARD     Internal RFAGC (overload) standard setting.
 * SONY_ASCOT3_CONFIG_OVERLOAD_EXTENDED_TC  Internal RFAGC (overload) setting for long term burst interferer .
 * SONY_ASCOT3_CONFIG_LOOPFILTER_INTERNAL   Internal loop filter setting.
                                            If this is used, internal loop filter is used for
                                            digital broadcasting system except for DVB-C/C2.
                                            For analog and DVB-C/C2, external loop filter will be used 
 * SONY_ASCOT3_CONFIG_LOOPTHRU_ENABLE       Loop through enable. Available CXD2872 only. 
 * SONY_ASCOT3_CONFIG_RFIN_MATCHING_ENABLE  RFIN matching for Power Save state enable. For CXD2872, 
                                            this is automatically enabled if 
                                            SONY_ASCOT3_CONFIG_LOOPTHRU_ENABLE is used.
 * SONY_ASCOT3_CONFIG_IFAGCSEL_ALL1         IF/AGC 1 is used for both Analog and Digital
 * SONY_ASCOT3_CONFIG_IFAGCSEL_ALL2         IF/AGC 2 is used for both Analog and Digital
 * SONY_ASCOT3_CONFIG_IFAGCSEL_A1D2         IF/AGC 1 is used for Analog, 2 is used for Digital
 * SONY_ASCOT3_CONFIG_IFAGCSEL_D1A2         IF/AGC 1 is used for Digital, 2 is used for Analog
 * - If following values are not used, REFOUT output is disabled.
 * SONY_ASCOT3_CONFIG_REFOUT_500mVpp        REFOUT enable, output level is 500mVp-p. 
 * SONY_ASCOT3_CONFIG_REFOUT_400mVpp        REFOUT enable, output level is 400mVp-p. 
 * SONY_ASCOT3_CONFIG_REFOUT_600mVpp        REFOUT enable, output level is 600mVp-p. 
 * SONY_ASCOT3_CONFIG_REFOUT_800mVpp        REFOUT enable, output level is 800mVp-p. 
 *
 * Multiple flags can be defined using the | operator, e.g:
 *   #define SONY_EXAMPLE_TUNER_FLAGS   (SONY_ASCOT3_CONFIG_EXT_REF | SONY_ASCOT3_CONFIG_IFAGCSEL_ALL2)
 */
#define SONY_EXAMPLE_TUNER_FLAGS            0

/* ASCOT3 specific const char definitions */
static const char *ASCOT3_Xtal[] = {"16MHz", "20.5MHz", "24MHz", "41MHz"};

#elif defined SONY_EXAMPLE_OTHER
/*------------------------------------------------------------------------------
 Other third party tuner
------------------------------------------------------------------------------*/
#define SONY_EXAMPLE_TUNER_XTAL             16
#define SONY_EXAMPLE_TUNER_I2C_ADDRESS      0xC0
#define SONY_EXAMPLE_TUNER_OPTIMIZE         SONY_DEMOD_TUNER_OPTIMIZE_UNKNOWN
#define SONY_EXAMPLE_TUNER_FLAGS            0
#define SONY_EXAMPLE_TUNER_OFFSET_CUTOFF    0
#endif


/*------------------------------------------------------------------------------
  Demodulator
------------------------------------------------------------------------------*/
#define SONY_EXAMPLE_DEMOD_XTAL             SONY_DEMOD_XTAL_41000KHz
#define SONY_EXAMPLE_DEMOD_I2C_ADDRESS      0x6C

/* IF Settings, update to match your tuner output */
#define SONY_EXAMPLE_DVBT_5MHz_IF           3.60
#define SONY_EXAMPLE_DVBT_6MHz_IF           3.60
#define SONY_EXAMPLE_DVBT_7MHz_IF           4.20
#define SONY_EXAMPLE_DVBT_8MHz_IF           4.80
#define SONY_EXAMPLE_DVBT2_1_7MHz_IF        3.50
#define SONY_EXAMPLE_DVBT2_5MHz_IF          3.60
#define SONY_EXAMPLE_DVBT2_6MHz_IF          3.60
#define SONY_EXAMPLE_DVBT2_7MHz_IF          4.20
#define SONY_EXAMPLE_DVBT2_8MHz_IF          4.80
#define SONY_EXAMPLE_DVBC_IF                4.90
#define SONY_EXAMPLE_DVBC2_6MHz_IF          3.70
#define SONY_EXAMPLE_DVBC2_8MHz_IF          4.90

/** 
 @brief Storage structure for demodulator configuration settings.
 */
typedef struct {
    sony_demod_config_id_t configId;    /**< Demodulator config setting ID */
    uint32_t configValue;               /**< Config setting value */
}sony_example_demod_configuration_t ;

/* The following array defines the demodulator config options that will be loaded
 * after initialization.  These should be modified with respect to the platform
 * design, and external component IO requirements.  
 *
 * Ensure the "Tuner Specific Settings" are modified to an appropriate value during
 * early debug, based on the tuner specification.  The values provided are correct 
 * for ASCOT2D / ASCOT2E implementations.
 * 
 * Where the setting description is more complex "See definition in sony_demod.h" 
 * is indicated, as this file contains more detailed information.  Alternatively, 
 * each setting is described in detail in the DUG document. */
sony_example_demod_configuration_t demodulatorConfiguration[] = {
    /* ----------------------------------------------------------------------------------------------------------------------------------------
     *  Tuner Specific Settings                                        Description              Range     Meaning 
     * ---------------------------------------------------------------------------------------------------------------------------------------- */
    { SONY_DEMOD_CONFIG_IFAGCNEG,                           1},     /* IFAGC polarity           [0-1]     0 : Positive, 1 : Negative            */
    { SONY_DEMOD_CONFIG_IFAGC_ADC_FS,                       0},     /* IFAGC ADC range          [0-2]     0 : 1.4Vpp, 1 : 1.0Vpp, 2 : 0.7Vpp    */
    { SONY_DEMOD_CONFIG_SPECTRUM_INV,                       0},     /* Spectrum                 [0-1]     0 : Normal, 1 : Inverted              */
    { SONY_DEMOD_CONFIG_RFAIN_ENABLE,                       0},     /* RFAIN monitoring         [0-1]     0 : Disabled, 1 : Enabled             */

    /* ----------------------------------------------------------------------------------------------------------------------------------------
     *  TS Configuration                                               Description              Range     Meaning 
     * ---------------------------------------------------------------------------------------------------------------------------------------- */
    //{ SONY_DEMOD_CONFIG_PARALLEL_SEL,                       1},     /* TS format                [0-1]     0 : Serial, 1 : Parallel              */
    //{ SONY_DEMOD_CONFIG_SER_DATA_ON_MSB,                    1},     /* Serial output pin        [0-1]     0 : TSDATA0, 1 : TSDATA7              */
    //{ SONY_DEMOD_CONFIG_OUTPUT_SEL_MSB,                     1},     /* Bit order                [0-1]     See definition in sony_demod.h        */
    //{ SONY_DEMOD_CONFIG_TSVALID_ACTIVE_HI,                  1},     /* TS valid polarity        [0-1]     0 : Valid low, 1 : Valid hi           */
    //{ SONY_DEMOD_CONFIG_TSSYNC_ACTIVE_HI,                   1},     /* TS sync polarity         [0-1]     0 : Valid low, 1 : Valid hi           */
    //{ SONY_DEMOD_CONFIG_TSERR_ACTIVE_HI,                    1},     /* TS error polarity        [0-1]     0 : Valid low, 1 : Valid hi           */
    //{ SONY_DEMOD_CONFIG_LATCH_ON_POSEDGE,                   1},     /* Clock polarity           [0-1]     0 : Falling edge, 1 : Rising edge     */
    //{ SONY_DEMOD_CONFIG_TSCLK_CONT,                         1},     /* Clock mode               [0-1]     0 : Gated (data only), 1 : Continuous */
    //{ SONY_DEMOD_CONFIG_TSCLK_MASK,                         0},     /* Clock masking                      See definition in sony_demod.h        */
    //{ SONY_DEMOD_CONFIG_TSVALID_MASK,                       1},     /* TS valid masking                   See definition in sony_demod.h        */
    //{ SONY_DEMOD_CONFIG_TSERR_MASK,                         0},     /* TS error masking                   See definition in sony_demod.h        */
    //{ SONY_DEMOD_CONFIG_TSCLK_CURRENT_10mA,                 0},     /* TS clock current         [0-1]     0 : 8mA, 1 : 10mA                     */
    //{ SONY_DEMOD_CONFIG_TS_CURRENT_10mA,                    0},     /* Other TS pin current     [0-1]     0 : 8mA, 1 : 10mA                     */
    //{ SONY_DEMOD_CONFIG_TS_BACKWARDS_COMPATIBLE,            0},     /* TS backwards compatible  [0-1]     See definition in sony_demod.h        */
    //{ SONY_DEMOD_CONFIG_TSIF_SDPR,                          10250}, /* TS clock rate            [0-10250] See definition in sony_demod.h        */
    //{ SONY_DEMOD_CONFIG_TS_AUTO_RATE_ENABLE,                1},     /* TS clock rate mode       [0-1]     0 : Manual, 1 : Automatic             */
    //{ SONY_DEMOD_CONFIG_TERR_CABLE_TS_SERIAL_CLK_FREQ,      1},     /* Serial clock frequency   [0-5]     See definition in sony_demod.h        */
    
    /* ----------------------------------------------------------------------------------------------------------------------------------------
     *  Generic Demoduolator Settings                    
     * ---------------------------------------------------------------------------------------------------------------------------------------- */
    //{ SONY_DEMOD_CONFIG_TERR_BLINDTUNE_DVBT2_FIRST,         0},     /* Blind tune priority      [0-1]     0 : DVB-T first, 1 : DVB-T2 first     */
    //{ SONY_DEMOD_CONFIG_PWM_VALUE,                          0},     /* PWM output value         [0-4095]  0 : GND, 4095 : DVDD                  */

    /* ----------------------------------------------------------------------------------------------------------------------------------------
     *  Error Rate Measurement Periods                    
     * ---------------------------------------------------------------------------------------------------------------------------------------- */
    //{ SONY_DEMOD_CONFIG_DVBT_BERN_PERIOD,                  11},     /* DVB-T Pre-RS BER       [0-31]                                          */
    //{ SONY_DEMOD_CONFIG_DVBC_BERN_PERIOD,                  11},     /* DVB-C Pre-RS BER       [0-31]                                          */
    //{ SONY_DEMOD_CONFIG_DVBT_VBER_PERIOD,                   3},     /* DVB-T Pre-Viterbi BER  [0-7]                                           */
    //{ SONY_DEMOD_CONFIG_DVBT2C2_BBER_MES,                   8},     /* DVB-T2/C2 Pre-BCH BER &                                                  */
                                                                      /* DVB-T2/C2 Post-BCH FER [0-15]                                          */
    //{ SONY_DEMOD_CONFIG_DVBT2C2_LBER_MES,                   8},     /* DVB-T2/C2 Pre-LDPC BER [0-15]                                          */
    //{ SONY_DEMOD_CONFIG_DVBT_PER_MES,                      10},     /* DVB-T PER              [0-15]                                          */
    //{ SONY_DEMOD_CONFIG_DVBC_PER_MES,                      10},     /* DVB-C PER              [0-15]                                          */
    //{ SONY_DEMOD_CONFIG_DVBT2C2_PER_MES,                    10},    /* DVB-T2/C2 PER          [0-15]                                          */
    };

/*------------------------------------------------------------------------------
 Common const char definitions
------------------------------------------------------------------------------*/
static const char *Common_DemodXtal[] = {"20.5MHz", "41MHz"};
static const char *Common_Bandwidth[] = { "Unknown", "1.7MHz", "Invalid", "Invalid", "Invalid", "5MHz", "6MHz", "7MHz", "8MHz"};
static const char *Common_TunerOptimize[] = { "Unknown", "ASCOT2D", "ASCOT2E", "ASCOT3"};
static const char *Common_SpectrumSense[] = { "Normal", "Inverted"};
static const char *Common_Result[] = { "OK", "Argument Error", "I2C Error", "SW State Error", "HW State Error", "Timeout", "Unlock", "Out of Range", "No Support", "Cancelled", "Other Error", "Overflow", "OK - Confirm"};
static const char *Common_YesNo[] = { "No", "Yes" };
static const char *Common_System[] = { "Unknown", "DVB-T", "DVB-T2", "DVB-C", "DVB-C2", "DVB-S", "DVB-S2", "Any" };
static const char *Common_ConfigId[] = { "SONY_DEMOD_CONFIG_PARALLEL_SEL",
                                         "SONY_DEMOD_CONFIG_SER_DATA_ON_MSB",
                                         "SONY_DEMOD_CONFIG_OUTPUT_SEL_MSB",
                                         "SONY_DEMOD_CONFIG_TSVALID_ACTIVE_HI",
                                         "SONY_DEMOD_CONFIG_TSSYNC_ACTIVE_HI",
                                         "SONY_DEMOD_CONFIG_TSERR_ACTIVE_HI",
                                         "SONY_DEMOD_CONFIG_LATCH_ON_POSEDGE",
                                         "SONY_DEMOD_CONFIG_TSCLK_CONT",
                                         "SONY_DEMOD_CONFIG_TSCLK_MASK",
                                         "SONY_DEMOD_CONFIG_TSVALID_MASK",
                                         "SONY_DEMOD_CONFIG_TSERR_MASK",
                                         "SONY_DEMOD_CONFIG_TSCLK_CURRENT_10mA",
                                         "SONY_DEMOD_CONFIG_TS_CURRENT_10mA",
                                         "SONY_DEMOD_CONFIG_TS_BACKWARDS_COMPATIBLE",
                                         "SONY_DEMOD_CONFIG_PWM_VALUE",
                                         "SONY_DEMOD_CONFIG_TSIF_SDPR",
                                         "SONY_DEMOD_CONFIG_TS_AUTO_RATE_ENABLE",
                                         "SONY_DEMOD_CONFIG_TERR_CABLE_TS_SERIAL_CLK_FREQ",
                                         "SONY_DEMOD_CONFIG_IFAGCNEG",
                                         "SONY_DEMOD_CONFIG_IFAGC_ADC_FS",
                                         "SONY_DEMOD_CONFIG_SPECTRUM_INV",
                                         "SONY_DEMOD_CONFIG_RFAIN_ENABLE",
                                         "SONY_DEMOD_CONFIG_TERR_BLINDTUNE_DVBT2_FIRST",
                                         "SONY_DEMOD_CONFIG_DVBT_BERN_PERIOD",
                                         "SONY_DEMOD_CONFIG_DVBC_BERN_PERIOD",
                                         "SONY_DEMOD_CONFIG_DVBT_VBER_PERIOD",
                                         "SONY_DEMOD_CONFIG_DVBT2C2_BBER_MES",
                                         "SONY_DEMOD_CONFIG_DVBT2C2_LBER_MES",
                                         "SONY_DEMOD_CONFIG_DVBT_PER_MES",
                                         "SONY_DEMOD_CONFIG_DVBC_PER_MES",
                                         "SONY_DEMOD_CONFIG_DVBT2C2_PER_MES",
                                         "SONY_DEMOD_CONFIG_SAT_TS_SERIAL_CLK_FREQ",
                                         "SONY_DEMOD_CONFIG_SAT_TUNER_IQ_SENSE_INV",
                                         "SONY_DEMOD_CONFIG_SAT_IFAGCNEG",
                                         "SONY_DEMOD_CONFIG_SAT_TSDATA6_DISEQC",
                                         "SONY_DEMOD_CONFIG_SAT_MEAS_PERIOD1",
                                         "SONY_DEMOD_CONFIG_SAT_MEAS_PERIOD2",
                                         "SONY_DEMOD_CONFIG_SAT_MEAS_PERIOD3",
                                         "SONY_DEMOD_CONFIG_SAT_MEAS_PERIOD4"};

#endif /* SONY_EXAMPLE_TERR_CABLE_CONFIGURATION_H */

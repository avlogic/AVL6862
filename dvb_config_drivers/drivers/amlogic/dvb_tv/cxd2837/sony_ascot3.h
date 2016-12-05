/*------------------------------------------------------------------------------
  Copyright 2013 Sony Corporation

  Last Updated  : 2013/04/01
  File Revision : 1.0.3.0
------------------------------------------------------------------------------*/
/**
 @file    sony_ascot3.h

          This file provides the ASCOT3 tuner control interface.
*/
/*----------------------------------------------------------------------------*/

#ifndef SONY_ASCOT3_H
#define SONY_ASCOT3_H

#include "sony_common.h"
#include "sony_i2c.h"

/*------------------------------------------------------------------------------
  Defines
------------------------------------------------------------------------------*/
/**
 @brief Version of this driver.
*/
#define SONY_ASCOT3_VERSION       "1.0.3.0"

/**
 @brief Default I2C slave address of the ASCOT3 tuner.
*/
#define SONY_ASCOT3_ADDRESS       0xC0

/*------------------------------------------------------------------------------
  Enums
------------------------------------------------------------------------------*/
/**
 @brief ASCOT3 tuner state.
*/
typedef enum {
    SONY_ASCOT3_STATE_UNKNOWN,    /**< ASCOT3 state is Unknown */
    SONY_ASCOT3_STATE_SLEEP,      /**< ASCOT3 state is Sleep */
    SONY_ASCOT3_STATE_ACTIVE      /**< ASCOT3 state is Active */
} sony_ascot3_state_t;

/**
 @brief ASCOT3 chip type.
*/
typedef enum {
    SONY_ASCOT3_CHIP_ID_UNKNOWN,  /**< Unknown */
    SONY_ASCOT3_CHIP_ID_2871,     /**< CXD2871 (for TV) */
    SONY_ASCOT3_CHIP_ID_2872      /**< CXD2872 (for STB) */
} sony_ascot3_chip_id_t;

/**
 @brief Xtal frequencies supported by ASCOT3.
*/
typedef enum {
    SONY_ASCOT3_XTAL_16000KHz,    /**< 16 MHz */
    SONY_ASCOT3_XTAL_20500KHz,    /**< 20.5 MHz */
    SONY_ASCOT3_XTAL_24000KHz,    /**< 24 MHz */
    SONY_ASCOT3_XTAL_41000KHz     /**< 41 MHz */
} sony_ascot3_xtal_t;

/**
 @brief Analog/Digital terrestrial broadcasting system definitions supported by ASCOT3.
*/
typedef enum {
    SONY_ASCOT3_TV_SYSTEM_UNKNOWN,
    /* Analog */
    SONY_ASCOT3_ATV_MN_EIAJ,  /**< System-M (Japan) */
    SONY_ASCOT3_ATV_MN_SAP,   /**< System-M (US) */
    SONY_ASCOT3_ATV_MN_A2,    /**< System-M (Korea) */
    SONY_ASCOT3_ATV_BG,       /**< System-B/G */
    SONY_ASCOT3_ATV_I,        /**< System-I */
    SONY_ASCOT3_ATV_DK,       /**< System-D/K */
    SONY_ASCOT3_ATV_L,        /**< System-L */
    SONY_ASCOT3_ATV_L_DASH,   /**< System-L DASH */
    /* Digital */
    SONY_ASCOT3_DTV_8VSB,     /**< ATSC 8VSB */
    SONY_ASCOT3_DTV_QAM,      /**< US QAM */
    SONY_ASCOT3_DTV_ISDBT_6,  /**< ISDB-T 6MHzBW */
    SONY_ASCOT3_DTV_ISDBT_7,  /**< ISDB-T 7MHzBW */
    SONY_ASCOT3_DTV_ISDBT_8,  /**< ISDB-T 8MHzBW */
    SONY_ASCOT3_DTV_DVBT_5,   /**< DVB-T 5MHzBW */
    SONY_ASCOT3_DTV_DVBT_6,   /**< DVB-T 6MHzBW */
    SONY_ASCOT3_DTV_DVBT_7,   /**< DVB-T 7MHzBW */
    SONY_ASCOT3_DTV_DVBT_8,   /**< DVB-T 8MHzBW */
    SONY_ASCOT3_DTV_DVBT2_1_7,/**< DVB-T2 1.7MHzBW */
    SONY_ASCOT3_DTV_DVBT2_5,  /**< DVB-T2 5MHzBW */
    SONY_ASCOT3_DTV_DVBT2_6,  /**< DVB-T2 6MHzBW */
    SONY_ASCOT3_DTV_DVBT2_7,  /**< DVB-T2 7MHzBW */
    SONY_ASCOT3_DTV_DVBT2_8,  /**< DVB-T2 8MHzBW */
    SONY_ASCOT3_DTV_DVBC_6,     /**< DVB-C */
	SONY_ASCOT3_DTV_DVBC_8,
    SONY_ASCOT3_DTV_DVBC2_6,  /**< DVB-C2 6MHzBW */
    SONY_ASCOT3_DTV_DVBC2_8,  /**< DVB-C2 8MHzBW */
    SONY_ASCOT3_DTV_DTMB,     /**< DTMB */

    SONY_ASCOT3_ATV_MIN = SONY_ASCOT3_ATV_MN_EIAJ, /**< Minimum analog terrestrial system */
    SONY_ASCOT3_ATV_MAX = SONY_ASCOT3_ATV_L_DASH,  /**< Maximum analog terrestrial system */
    SONY_ASCOT3_DTV_MIN = SONY_ASCOT3_DTV_8VSB,    /**< Minimum digital terrestrial system */
    SONY_ASCOT3_DTV_MAX = SONY_ASCOT3_DTV_DTMB,    /**< Maximum digital terrestrial system */
    SONY_ASCOT3_TV_SYSTEM_NUM                       /**< Number of supported broadcasting system */
} sony_ascot3_tv_system_t;

/**
 @brief Macro to check that the system is analog terrestrial or not.
*/
#define SONY_ASCOT3_IS_ATV(tvSystem) (((tvSystem) >= SONY_ASCOT3_ATV_MIN) && ((tvSystem)<= SONY_ASCOT3_ATV_MAX))

/**
 @brief Macro to check that the system is digital terrestrial or not.
*/
#define SONY_ASCOT3_IS_DTV(tvSystem) (((tvSystem) >= SONY_ASCOT3_DTV_MIN) && ((tvSystem) <= SONY_ASCOT3_DTV_MAX))

/*------------------------------------------------------------------------------
  Structs
------------------------------------------------------------------------------*/

/**
 @brief ASCOT3 settings that may need to change depend on customer's system.
*/
typedef struct sony_ascot3_adjust_param_t {
    uint8_t OUTLMT;         /**< Addr:0x68 Bit[1:0] : Maximum IF output. (0: 1.5Vp-p, 1: 1.2Vp-p) */
    uint8_t RF_GAIN;        /**< Addr:0x69 Bit[6:4] : RFVGA gain. 0xFF means Auto. (RF_GAIN_SEL = 1) */
    uint8_t IF_BPF_GC;      /**< Addr:0x69 Bit[3:0] : IF_BPF gain. */
	uint8_t RFOVLD_DET_LV1_VL;
	uint8_t RFOVLD_DET_LV1_VH;
    uint8_t RFOVLD_DET_LV1_U; /**< Addr:0x6B Bit[3:0] : RF overload RF input detect level. */
	uint8_t IFOVLD_DET_LV_VL;
	uint8_t IFOVLD_DET_LV_VH;
    uint8_t IFOVLD_DET_LV_U;  /**< Addr:0x6C Bit[2:0] : Internal RFAGC detect level. */
    uint8_t IF_BPF_F0;      /**< Addr:0x6D Bit[5:4] : IF filter center offset. */
    uint8_t BW;             /**< Addr:0x6D Bit[1:0] : 6MHzBW(0x00) or 7MHzBW(0x01) or 8MHzBW(0x02) or 1.7MHzBW(0x03) */
    uint8_t FIF_OFFSET;     /**< Addr:0x6E Bit[4:0] : 5bit signed. IF offset (kHz) = FIF_OFFSET x 50 */
    uint8_t BW_OFFSET;      /**< Addr:0x6F Bit[4:0] : 5bit signed. BW offset (kHz) = BW_OFFSET x 50 (BW_OFFSET x 10 in 1.7MHzBW) */
    uint8_t AGC_SEL;        /**< Addr:0x74 Bit[5:4] : AGC pin select. (0: AGC1, 1: AGC2) 0xFF means Auto (by config flags) */
    uint8_t IF_OUT_SEL;     /**< Addr:0x74 Bit[1:0] : IFOUT pin select. (0: IFOUT1, 1: IFOUT2) 0xFF means Auto. (by config flags) */
    uint8_t IS_LOWERLOCAL;  /**< Addr:0x9C Bit[0]   : Local polarity. (0: Upper Local, 1: Lower Local) */
} sony_ascot3_adjust_param_t;

/**
 @brief The ASCOT3 tuner definition which allows control of the ASCOT3 tuner device 
        through the defined set of functions.
*/
typedef struct sony_ascot3_t {
    sony_ascot3_xtal_t        xtalFreq;     /**< Xtal frequency for ASCOT3. */
    uint8_t                   i2cAddress;   /**< I2C slave address of the ASCOT3 tuner (8-bit form - 8'bxxxxxxx0) */
    sony_i2c_t*               pI2c;         /**< I2C API instance. */
    uint32_t                  flags;        /**< ORed value of SONY_ASCOT3_CONFIG_XXXX */

    /* For saving current setting */
    sony_ascot3_state_t       state;        /**< The driver operating state. */
    uint32_t                  frequencykHz; /**< Currently RF frequency(kHz) tuned. */
    sony_ascot3_tv_system_t   tvSystem;     /**< Current broadcasting system tuned. */

    sony_ascot3_chip_id_t     chipId;       /**< Auto detected chip ID at initialization */
    /**
     @brief Adjustment parameter table (SONY_ASCOT3_TV_SYSTEM_NUM size)
    */
    const sony_ascot3_adjust_param_t *pParamTable;

    /* Following Xtal related parameters can be changed if optimization is necessary. */
    uint8_t                   xosc_sel;     /**< Driver current setting for crystal oscillator. (Addr:0x82 Bit[4:0]) */
    uint8_t                   xosc_cap_set; /**< Driver current setting for crystal oscillator. (Addr:0x83 Bit[6:0]) */

    void*                     user;         /**< User defined data. */
} sony_ascot3_t;

/*
 Config flag definitions. (ORed value should be set to flags argument of Create API.)
*/
/**
 @brief Use external Xtal
        Should be used for slave ASCOT3 in Xtal shared system.
*/
#define SONY_ASCOT3_CONFIG_EXT_REF                0x80000000

/**
 @brief Disable Xtal in Sleep state.
        Should NOT be used for Xtal shared system.
        Not only for the master ASCOT3 which has Xtal, but also slave ASCOT3 which receive the clock signal.
        Cannot be used with SONY_ASCOT3_CONFIG_EXT_REF.
*/
#define SONY_ASCOT3_CONFIG_SLEEP_DISABLEXTAL      0x40000000

/**
 @name Internal RFAGC (overload) setting.
       Should be set correctly depend on external component.
*/
/**@{*/
#define SONY_ASCOT3_CONFIG_OVERLOAD_STANDARD      0x20000000  /**< Standard setting. */
#define SONY_ASCOT3_CONFIG_OVERLOAD_EXTENDED_TC   0x00000000  /**< For long term burst interferer. (default) */
/**@}*/

/**
 @brief Internal loop filter setting.
        If this is used, internal loop filter is used for
        digital broadcasting system except for DVB-C/C2.
        For analog and DVB-C/C2, external loop filter will be used.
*/
#define SONY_ASCOT3_CONFIG_LOOPFILTER_INTERNAL    0x10000000

/**
 @brief Loop through enable.
        Available CXD2872 only.
*/
#define SONY_ASCOT3_CONFIG_LOOPTHRU_ENABLE        0x08000000

/**
 @brief RFIN matching for Power Save state enable
        For CXD2872, this is automatically enabled if SONY_ASCOT3_CONFIG_LOOPTHRU_ENABLE is used.
*/
#define SONY_ASCOT3_CONFIG_RFIN_MATCHING_ENABLE   0x04000000

/**
 @name IF/AGC pin individually setting.
       Used if sony_ascot3_adjust_param_t::IFAGCSEL == 0xFF
       Available CXD2871 only.
*/
/**@{*/
#define SONY_ASCOT3_CONFIG_IF2_ATV                0x00100000  /**< IF2 is used for Analog. (Otherwise IF1 is used.) */
#define SONY_ASCOT3_CONFIG_AGC2_ATV               0x00200000  /**< AGC2 is used for Analog. (Otherwise AGC1 is used.) */
#define SONY_ASCOT3_CONFIG_IF2_DTV                0x00400000  /**< IF2 is used for Digital. (Otherwise IF1 is used.) */
#define SONY_ASCOT3_CONFIG_AGC2_DTV               0x00800000  /**< AGC2 is used for Digital. (Otherwise AGC1 is used.) */
/**@}*/

/**
 @name IF/AGC pin setting for normal cases.
       Used if sony_ascot3_adjust_param_t::IFAGCSEL == 0xFF
       Available CXD2871 only.
*/
/**@{*/
/**
 @brief IF/AGC 1 is used for both Analog and Digital
*/
#define SONY_ASCOT3_CONFIG_IFAGCSEL_ALL1          0x00000000
/**
 @brief IF/AGC 2 is used for both Analog and Digital
*/
#define SONY_ASCOT3_CONFIG_IFAGCSEL_ALL2          (SONY_ASCOT3_CONFIG_IF2_ATV | SONY_ASCOT3_CONFIG_AGC2_ATV | \
                                                   SONY_ASCOT3_CONFIG_IF2_DTV | SONY_ASCOT3_CONFIG_AGC2_DTV)
/**
 @brief IF/AGC 1 is used for Analog, 2 is used for Digital
*/
#define SONY_ASCOT3_CONFIG_IFAGCSEL_A1D2          (SONY_ASCOT3_CONFIG_IF2_DTV | SONY_ASCOT3_CONFIG_AGC2_DTV)
/**
 @brief IF/AGC 1 is used for Digital, 2 is used for Analog
*/
#define SONY_ASCOT3_CONFIG_IFAGCSEL_D1A2          (SONY_ASCOT3_CONFIG_IF2_ATV | SONY_ASCOT3_CONFIG_AGC2_ATV)
/**@}*/

/**
 @name REFOUT setting related macros.
       If following values are not used, REFOUT output is disabled.
*/
/**@{*/
/**
 @brief REFOUT enable, output level is 500mVp-p.
*/
#define SONY_ASCOT3_CONFIG_REFOUT_500mVpp         0x00001000
/**
 @brief REFOUT enable, output level is 400mVp-p.
*/
#define SONY_ASCOT3_CONFIG_REFOUT_400mVpp         0x00002000
/**
 @brief REFOUT enable, output level is 600mVp-p.
*/
#define SONY_ASCOT3_CONFIG_REFOUT_600mVpp         0x00003000
/**
 @brief REFOUT enable, output level is 800mVp-p.
*/
#define SONY_ASCOT3_CONFIG_REFOUT_800mVpp         0x00004000
/**
 @brief Internal use only. Do not use this value.
*/
#define SONY_ASCOT3_CONFIG_REFOUT_MASK            0x00007000
/**@}*/

/*------------------------------------------------------------------------------
  APIs
------------------------------------------------------------------------------*/
/**
 @brief Set up the ASCOT3 tuner driver.

        This MUST be called before calling sony_ascot3_Initialize.

 @param pTuner      Reference to memory allocated for the ASCOT3 instance.
                    The create function will setup this ASCOT3 instance.
 @param xtalFreq    The frequency of the ASCOT3 crystal.
 @param i2cAddress  The ASCOT3 tuner I2C slave address in 8-bit form.
 @param pI2c        The I2C APIs that the ASCOT3 driver will use as the I2C interface.
 @param flags       Configuration flags. It should be ORed value of SONY_ASCOT3_CONFIG_XXXX.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_ascot3_Create(sony_ascot3_t *pTuner, sony_ascot3_xtal_t xtalFreq,
    uint8_t i2cAddress, sony_i2c_t *pI2c, uint32_t flags);

/**
 @brief Initialize the ASCOT3 tuner.
        
        This MUST be called before calling sony_ascot3_Tune.

 @param pTuner       The ASCOT3 tuner instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_ascot3_Initialize(sony_ascot3_t *pTuner);

/**
 @brief Tune to a given RF frequency with broadcasting system.

        To complete tuning, sony_ascot3_TuneEnd should be called after waiting 50ms.

 @param pTuner       The ASCOT3 tuner instance.
 @param frequencykHz RF frequency to tune. (kHz)
 @param tvSystem     The type of broadcasting system to tune.

 @return SONY_RESULT_OK if tuner successfully tuned.
*/
sony_result_t sony_ascot3_Tune(sony_ascot3_t *pTuner, uint32_t frequencykHz, sony_ascot3_tv_system_t tvSystem);

/**
 @brief Completes the ASCOT3 tuning sequence.

        This MUST be called after calling sony_ascot3_Tune and 50ms wait.

 @param pTuner       The ASCOT3 tuner instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_ascot3_TuneEnd(sony_ascot3_t *pTuner);

/**
 @brief Shift RF frequency.

        This API shift RF frequency without VCO calibration.
        This API is normally useful for analog TV "fine tuning" that
        shift RF frequency without picture distortion.
        NOTE: Please check the frequency range that VCO calibration is unnecessary.
              (See hardware specification sheet.)

 @param pTuner       The ASCOT3 tuner instance.
 @param frequencykHz RF frequency to tune. (kHz)

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_ascot3_ShiftFRF(sony_ascot3_t *pTuner, uint32_t frequencykHz);

/**
 @brief Put the ASCOT3 tuner into Sleep state.
 
        From this state the tuner can be directly tuned by calling sony_ascot3_Tune.

 @param pTuner       The ASCOT3 tuner instance.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_ascot3_Sleep(sony_ascot3_t *pTuner);

/**
 @brief Write to GPO0 or GPO1.

 @param pTuner       The ASCOT3 tuner instance.
 @param id           Pin ID 0 = GPO0, 1 = GPO1
 @param val          Output logic level, 0 = Low, 1 = High

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_ascot3_SetGPO(sony_ascot3_t *pTuner, uint8_t id, uint8_t val);

/**
 @brief Set the RF external circuit control pin.
        (Set RF_EXT_CTRL register.)

 @param pTuner       The ASCOT3 tuner instance.
 @param enable       The value to set. (Enable(1) or Disable(0))

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_ascot3_SetRfExtCtrl(sony_ascot3_t *pTuner, uint8_t enable);

/**
 @brief Read the RSSI in dBm from the tuner.

 @param pTuner          The ASCOT3 tuner instance.
 @param pRssi           RSSI in dBm * 100
                        Target level of IF signal output (depend on demodulator)
                        should be added to this value.

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_ascot3_ReadRssi(sony_ascot3_t *pTuner, int32_t *pRssi);

/**
 @brief RF filter compensation setting for VHF-L band.
        (Please see RFVGA Description of datasheet.)
        New setting will become effective after next tuning.

        mult = coeff / 128
        (compensated value) = (original value) * mult + offset

 @param pTuner       The ASCOT3 tuner instance.
 @param coeff        Multiplier value. (8bit unsigned)
 @param offset       Additional term. (8bit 2s complement)

 @return SONY_RESULT_OK if successful.
*/
sony_result_t sony_ascot3_RFFilterConfig(sony_ascot3_t *pTuner, uint8_t coeff, uint8_t offset);

#endif /* SONY_ASCOT3_H */

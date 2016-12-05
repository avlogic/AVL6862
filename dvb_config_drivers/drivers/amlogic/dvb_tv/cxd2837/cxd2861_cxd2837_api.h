

#ifndef _CXD2861_CDX2837_API_H
#define _CXD2861_CDX2837_API_H
#ifdef SONY_EXAMPLE_TUNER_ASCOT3
#include "sony_ascot3.h"
#endif
#ifdef SONY_EXAMPLE_TUNER_ASCOT2E
#include "sony_ascot2e.h"
#endif
struct cxd2837_fe_config {
	int                   i2c_id;
	int                 reset_pin;
	int                 demod_addr;
	int                 tuner_addr;
};


struct cxd2837_state {
	struct cxd2837_fe_config config;
	struct i2c_adapter *i2c;
	u32                 freq;
	u32 	bandwidth;
	u32 	mode;
	struct dvb_frontend fe;
	sony_integ_t device;
	struct sony_tuner_terr_cable_t tuner;
	sony_i2c_t demodI2C;
	sony_i2c_t tunerI2C;
	sony_demod_t demod;
#ifdef SONY_EXAMPLE_TUNER_ASCOT3
	sony_ascot3_t ascot3;
#endif
#ifdef SONY_EXAMPLE_TUNER_ASCOT2E
	sony_ascot2e_t ascot2e;
#endif
};

/*****************************************************************************
 * Error code definitions
 *****************************************************************************/

#define DEMOD_ERROR_BASE                          (0x80000000+0x1000)
#define DEMOD_SUCCESS                             (AM_SUCCESS)
#define DEMOD_ERR_NO_MEMORY                       (DEMOD_ERROR_BASE+1)
#define DEMOD_ERR_DEVICE_NOT_EXIST                (DEMOD_ERROR_BASE+2)
#define DEMOD_ERR_HARDWARE_ERROR                  (DEMOD_ERROR_BASE+3)
#define DEMOD_ERR_BAD_PARAMETER                   (DEMOD_ERROR_BASE+4)
#define DEMOD_ERR_NOT_INITLIZED                   (DEMOD_ERROR_BASE+5)
#define DEMOD_ERR_DESTROY_FAILED                  (DEMOD_ERROR_BASE+6)
#define DEMOD_ERR_FEATURE_NOT_SUPPORT             (DEMOD_ERROR_BASE+7)
#define DEMOD_ERR_OTHER                           (DEMOD_ERROR_BASE+8)


/*****************************************************************************
 * Function prototypes	
 *****************************************************************************/

int  demod_init(struct cxd2837_state *state);
int demod_check_locked(unsigned char* lock);
int demod_connect(struct cxd2837_state *state,unsigned int freq_khz, unsigned char bandwidth);

int demod_disconnect(void);
int demod_get_signal_strength(struct cxd2837_state *state,unsigned int* strength);
int demod_get_signal_quality(struct cxd2837_state *state,unsigned int* quality);
int demod_get_signal_errorate(struct cxd2837_state *state,unsigned int* errorrate);
int demod_control(unsigned int cmd, void* param);
int demod_destroy(void* para);

int demod_deinit(struct cxd2837_state *state);
int demod_set_data_plp(struct cxd2837_state *state, uint8_t plp_id);
int demod_get_active_data_plp(struct cxd2837_state *state, sony_dvbt2_plp_t *plp_info);
int demod_get_data_plps(struct cxd2837_state *state, uint8_t *plp_ids, uint8_t *plp_num);

#endif

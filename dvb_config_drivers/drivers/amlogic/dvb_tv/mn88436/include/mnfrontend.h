


#ifndef _MN88436SF_H_
#define _MN88436SF_H_



#include <linux/dvb/frontend.h>
//#include "../../../../media/dvb-core/dvb_frontend.h"
#include "../../aml_dvb.h"
#include "../../aml_fe.h"

#include "MaxLinearDataTypes.h"
#include "MN_DMD_common.h"
#include "MN_DMD_console.h"
#include "MN_DMD_device.h"
#include "MN_DMD_driver.h"
#include "MN_DMD_types.h"
#include "MN_TCB.h"
#include "MN_Tuner.h"
#include "MN88436_reg.h"

//#include "nmituner_api.h"


#define printf printk




struct mn88436_fe_config {
	int                   i2c_id;
	int                 reset_pin;
	int                 demod_addr;
	int                 tuner_addr;
	void 			  *i2c_adapter;
};


struct mn88436_state {
	struct mn88436_fe_config config;
	struct i2c_adapter *i2c;
	u32                 freq;
    fe_modulation_t     mode;
    u32                 symbol_rate;
    struct dvb_frontend fe;
};


#endif

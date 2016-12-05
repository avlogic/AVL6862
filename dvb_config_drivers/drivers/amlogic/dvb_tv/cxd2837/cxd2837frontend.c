/*****************************************************************
**
**  Copyright (C) 2009 Amlogic,Inc.
**  All rights reserved
**        Filename : gxfrontend.c
**
**  comment:
**        Driver for CDX2837 demodulator
**  author :
**	    jianfeng_wang@amlogic
**  version :
**	    v1.0	 09/03/04
*****************************************************************/

/*
    Driver for CDX2837 demodulator
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#ifdef ARC_700
#include <asm/arch/am_regs.h>
#else
//#include <mach/am_regs.h>
#endif
#include <linux/i2c.h>
#include <linux/gpio.h>

//#include <mach/gpio_data.h>

#include <linux/dvb/frontend.h>
#include <linux/i2c.h>
#include "../../../../../common/drivers/media/dvb-core/dvb_frontend.h"
#include "../../../../../common/drivers/amlogic/dvb_tv/aml_dvb.h"
#include "../../../../../common/drivers/amlogic/dvb_tv/aml_fe.h"

//#define REAL_CABLE_CONFIG
#include "sony_integ.h"
#define SONY_EXAMPLE_TUNER_ASCOT3
#include "cxd2861_cxd2837_api.h"
#include "sony_dvbt2.h"

#define ANT_POWER_ON_LEVEL	1
#define pr_dbg(args...) printk("cxd2837: " args)
#define pr_error(args...) printk("cxd2837: " args)

static int tuner_ant_power_gpio = 0;
static struct cxd2837_state *pCxd2837_state;

int cxd2837_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	//struct cxd2837_state *state = fe->demodulator_priv;

	return 0;
}

#ifdef REAL_CABLE_CONFIG
static int cxd2837_set_mode(struct dvb_frontend* fe, fe_type_t type)
{
	pr_dbg("cxd2837_set_mode mode(%d)\n", type);
	return 1;
}
#endif

static int cxd2837_ant_power(struct dvb_frontend* fe, fe_sec_voltage_t voltage)
{
#if 0 //Do not control Ant Power for Cable */
	int nValue = 0;
//	struct aml_fe_dev *dev = fe->dtv_demod;

    pr_dbg("cxd2837_ant_power: %d\n",voltage);
	if(voltage ==  SEC_VOLTAGE_ON)
		nValue = ANT_POWER_ON_LEVEL;
	else if(voltage ==SEC_VOLTAGE_OFF)
		nValue = (ANT_POWER_ON_LEVEL ^ 1);
	else
		return -1;

	if(tuner_ant_power_gpio > 0)
	{
//		pio_out(PAD_GPIOD_7, nValue);
		amlogic_gpio_request(tuner_ant_power_gpio, "cxd2837_antPower");
		amlogic_gpio_direction_output(tuner_ant_power_gpio, nValue, "cxd2837_antPower");

		pr_dbg("\n cxd2837_ant_power (%s) (%d)\n", (voltage ==  SEC_VOLTAGE_ON)?"On":"OFF", nValue);
	}
#endif
	return 0;
}


int cxd2837_get_fe_config(struct cxd2837_fe_config *cfg)
{
	//pr_dbg("cfg->demod_addr is %x,cfg->i2c_adapter is %x\n",cfg->demod_addr,cfg->i2c_adapter);
	return 0;
}

static int cxd2837_read_status(struct dvb_frontend *fe, fe_status_t * status)
{
//	struct cxd2837_state *state = fe->demodulator_priv;
	unsigned char s=0;
	
	demod_check_locked(&s);
	if(s==1)
	{
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		*status = FE_TIMEDOUT;
	}

//    pr_dbg("cxd2837_read_status: 0x%02x, FE_TIMEDOUT = 0x%02x\n",*status, FE_TIMEDOUT);
	return  0;
}

static int cxd2837_read_ber(struct dvb_frontend *fe, u32 * ber)
{
//	struct cxd2837_state *state = fe->demodulator_priv;
	uint   rec_ber;
	
	demod_get_signal_errorate(pCxd2837_state, &rec_ber);
	*ber=rec_ber;

//  pr_dbg("cxd2837_read_ber: %d\n", rec_ber);
	return 0;
}

static int cxd2837_read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
//	struct cxd2837_state *state = fe->demodulator_priv;
	uint   rec_strength;
	
	demod_get_signal_strength(pCxd2837_state, &rec_strength);
	*strength=rec_strength;

//  pr_dbg("cxd2837_read_signal_strength: %d\n",rec_strength);
	return 0;
}

static int cxd2837_read_snr(struct dvb_frontend *fe, u16 * snr)
{
//	struct cxd2837_state *state = fe->demodulator_priv;
	uint   rec_snr ;
	
	demod_get_signal_quality(pCxd2837_state, &rec_snr) ;
	*snr = rec_snr;//>>16;
	
//    pr_dbg("cxd2837_read_snr: %d\n",rec_snr);
	return 0;
}

static int cxd2837_read_ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	ucblocks=NULL;
	return 0;
}

static int cxd2837_set_frontend(struct dvb_frontend *fe)
{
//	struct cxd2837_state *state = fe->demodulator_priv;
	struct aml_fe *afe = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	unsigned char s=0;
	int                  freq_hz;
	int                  bandwidth;

	freq_hz = c->frequency;
	bandwidth = (c->bandwidth_hz/1000000);
	if(bandwidth == 8) bandwidth = 0;
	else if(bandwidth == 7) bandwidth = 1;
	else if(bandwidth == 6) bandwidth = 2;

	pCxd2837_state->mode = 1; /* DVB_T(0), DVB_T2(1), DVB-C(Dont care) */
	pr_dbg("cxd2837_set_frontend : system(%d) freq(%d) Band(%d)\n", pCxd2837_state->mode, c->frequency, c->bandwidth_hz);
	demod_connect(pCxd2837_state, freq_hz, bandwidth);
 	afe->params = *c;
	demod_check_locked(&s);
	/*if(s!=1)
	{
		pCxd2837_state->mode = 1;
		p->u.ofdm.ofdm_mode = pCxd2837_state->mode;
		pr_dbg(" Not T ,Lock T2\n");
		demod_connect(pCxd2837_state, freqKhz, bandwidth);

		demod_check_locked(&s);
		if(s!=1)
		{	
			pCxd2837_state->mode = 0;
			p->u.ofdm.ofdm_mode = pCxd2837_state->mode;
		}
	}*/
	
	return  0;
}

static int cxd2837_get_frontend(struct dvb_frontend *fe)
{
	struct aml_fe *afe = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	
	*c = afe->params;
	pr_dbg("cxd2837_get_frontend : freq(%d) Band(%d)\n", c->frequency, c->bandwidth_hz);
	
	return 0;
}

static int cxd2837_set_property(struct dvb_frontend *fe, struct dtv_property *p)
{
//	struct cxd2837_state *state = fe->demodulator_priv;
	int r = 0;
	
	pr_dbg("cxd2837_set_property : cmd(%d)\n", p->cmd);
	switch (p->cmd) {
		case DTV_DVBT2_PLP_ID:
			if (demod_set_data_plp(pCxd2837_state, (uint8_t)p->u.data) != 0) {
				r = -EINVAL;
			}
			break;
		default:
			r = EOPNOTSUPP;
			break;
	}

	return r;
}

static int cxd2837_get_property(struct dvb_frontend *fe, struct dtv_property *p)
{
//	struct cxd2837_state *state = fe->demodulator_priv;
	int r = 0;
	
	pr_dbg("cxd2837_get_property : cmd(%d)\n", p->cmd);
	switch (p->cmd) {
		case DTV_DVBT2_PLP_ID:
			{
				sony_dvbt2_plp_t plp_info;
				if (demod_get_active_data_plp(pCxd2837_state, &plp_info) != 0) {
					p->u.buffer.len = 0;
					r = -EINVAL;
				} else {
					p->u.buffer.len = 2;
					p->u.buffer.data[0] = plp_info.id;
					p->u.buffer.data[1] = plp_info.type;
				}
			}
			break;
		#if 1
		case DTV_DVBT2_DATA_PLPS:
			{
				uint8_t plpids[256];
				uint8_t plpnum = 0;

				p->u.buffer.len = 0;
				p->u.buffer.reserved1[0] = 0;
				if (p->u.buffer.reserved2 != NULL) {
					demod_get_data_plps(pCxd2837_state, plpids, &plpnum);
					/* As linux dvb has property_dump, buffer.len cannot be used in this case, 
					 * it must < 32 , we use u.buffer.resvered1[0] to save plp num instead */
					p->u.buffer.reserved1[0] = plpnum;
					if (plpnum > 0 && 
						copy_to_user(p->u.buffer.reserved2, plpids, plpnum * sizeof(uint8_t))) {
						p->u.buffer.reserved1[0] = 0;
					}
				}
			}
			break;
		#endif
		default:
			r = EOPNOTSUPP;
			break;
	}

	return r;
}

static int cxd2837_fe_get_ops(struct aml_fe_dev *dev, int mode, void *ops)
{
	struct dvb_frontend_ops *fe_ops = (struct dvb_frontend_ops*)ops;

	fe_ops->info.frequency_min = 51000000;
	fe_ops->info.frequency_max = 858000000;
	fe_ops->info.frequency_stepsize = 8000;
	fe_ops->info.frequency_tolerance = 0;
	fe_ops->info.caps = FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_QPSK | FE_CAN_QAM_16 |
			FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_HIERARCHY_AUTO |
			FE_CAN_RECOVER |
			FE_CAN_MUTE_TS;

	fe_ops->set_frontend = cxd2837_set_frontend;
	fe_ops->get_frontend = cxd2837_get_frontend; 
	fe_ops->read_status = cxd2837_read_status;
	fe_ops->read_ber = cxd2837_read_ber;
	fe_ops->read_signal_strength = cxd2837_read_signal_strength;
	fe_ops->read_snr = cxd2837_read_snr;
	fe_ops->read_ucblocks = cxd2837_read_ucblocks;
	fe_ops->set_property = cxd2837_set_property,
	fe_ops->get_property = cxd2837_get_property,
	fe_ops->set_voltage = cxd2837_ant_power,
#ifdef REAL_CABLE_CONFIG
	fe_ops->set_mode = cxd2837_set_mode,
#endif
	pr_dbg("cxd2837_fe_get_ops : i2c_adap_id(%d), i2c_addr(%x) \n\n",dev->i2c_adap_id, dev->i2c_addr);

	pCxd2837_state = kmalloc(sizeof(struct cxd2837_state), GFP_KERNEL);
	pCxd2837_state->config.i2c_id = dev->i2c_adap_id;
	pCxd2837_state->config.demod_addr = dev->i2c_addr;
	pCxd2837_state->config.reset_pin = dev->reset_gpio;

	return 0;
}

static int cxd2837_fe_enter_mode(struct aml_fe *fe, int mode)
{
	struct aml_fe_dev *dev = fe->dtv_demod;
//	struct cxd2837_state *state = fe->fe->demodulator_priv;
	int ret;
	
	pr_dbg("cxd2837_fe_enter_mode: ResetPio(%d) AntPowerPio(%d) \n\n", dev->reset_gpio, dev->tuner_power_gpio);
	ret=amlogic_gpio_request(dev->reset_gpio, "cxd2837_reset");
	ret = amlogic_gpio_direction_output(dev->reset_gpio, dev->reset_value, "cxd2837_reset");
	msleep(300);
	ret=amlogic_gpio_direction_output(dev->reset_gpio, !dev->reset_value, "cxd2837_reset"); //enable tuner power
	msleep(200);

//	gpio_set_status(PAD_GPIOD_7,gpio_status_out);
	tuner_ant_power_gpio = dev->tuner_power_gpio;
	ret=amlogic_gpio_request(dev->tuner_power_gpio, "cxd2837_antPower");
	amlogic_gpio_direction_output(dev->tuner_power_gpio, (ANT_POWER_ON_LEVEL ^ 1), "cxd2837_antPower");

	demod_init(pCxd2837_state);
	msleep(200);

	return 0;

}

static int cxd2837_fe_resume(struct aml_fe_dev *dev)
{
	pr_dbg("cxd2837_fe_resume \n\n");
	amlogic_gpio_request(dev->reset_gpio,"cxd2837_reset");
	amlogic_gpio_direction_output(dev->reset_gpio, dev->reset_value, "cxd2837_reset");
	msleep(300);
	amlogic_gpio_direction_output(dev->reset_gpio, !dev->reset_value, "cxd2837_reset"); //enable tuner power
	msleep(200);

	return 0;
}

static int cxd2837_fe_suspend(struct aml_fe_dev *dev)
{
	pr_dbg("cxd2837_fe_suspend \n\n");
	return 0;
}

static struct aml_fe_drv cxd2837_fe_driver = {
.id 		= AM_DTV_DEMOD_CXD2837,
.name		= "Cxd2837",
#ifdef REAL_CABLE_CONFIG
.capability = AM_FE_QAM, /* (cable)AM_FE_QAM, (Terr)AM_FE_OFDM */
#else
.capability = AM_FE_OFDM,
#endif
.get_ops	= cxd2837_fe_get_ops,
.enter_mode = cxd2837_fe_enter_mode,
.suspend	= cxd2837_fe_suspend,
.resume 	= cxd2837_fe_resume
};

static int __init cxd2837frontend_init(void)
{
	pr_dbg("register cxd2837 demod driver\n");
	return aml_register_fe_drv(AM_DEV_DTV_DEMOD, &cxd2837_fe_driver);
}


static void __exit cxd2837frontend_exit(void)
{
	pr_dbg("unregister cxd2837 demod driver\n");
	aml_unregister_fe_drv(AM_DEV_DTV_DEMOD, &cxd2837_fe_driver);
}

fs_initcall(cxd2837frontend_init);
module_exit(cxd2837frontend_exit);


MODULE_DESCRIPTION("cxd2837 DVB-T2 Demodulator driver");
MODULE_AUTHOR("Dennis Noermann and Andrew de Quincey");
MODULE_LICENSE("GPL");



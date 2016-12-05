/*****************************************************************
**
**  Copyright (C) 2009 Amlogic,Inc.
**  All rights reserved
**        Filename : avlfrontend.c
**
**  comment:
**        Driver for MN88436 demodulator
**  author :
**	    Shijie.Rong@amlogic
**  version :
**	    v1.0	 12/3/30
*****************************************************************/

/*
    Driver for MN88436 demodulator
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
#include "mnfrontend.h"
//#include <linux/kthread.h>
#include "../aml_fe.h"

static int debug_Mn88436 = 0;

#define pr_dbg(fmt, args...)\
	do{\
		if(debug_Mn88436)\
			printk("Mn88436 debug: " fmt, ## args);\
	}while(0)
#define pr_error(fmt, args...) printk("Mn88436 error: " fmt, ## args)

MODULE_PARM_DESC(frontend0_reset, "\n\t\t Reset GPIO of frontend0");
static int frontend0_reset = -1;
module_param(frontend0_reset, int, S_IRUGO);

MODULE_PARM_DESC(frontend0_i2c, "\n\t\t IIc adapter id of frontend0");
static int frontend0_i2c = -1;
module_param(frontend0_i2c, int, S_IRUGO);

MODULE_PARM_DESC(frontend0_tuner_addr, "\n\t\t Tuner IIC address of frontend0");
static int frontend0_tuner_addr = -1;
module_param(frontend0_tuner_addr, int, S_IRUGO);

MODULE_PARM_DESC(frontend0_demod_addr, "\n\t\t Demod IIC address of frontend0");
static int frontend0_demod_addr = -1;
module_param(frontend0_demod_addr, int, S_IRUGO);

//static struct aml_fe mn88436_fe[FE_DEV_COUNT];

MODULE_PARM_DESC(frontend_reset, "\n\t\t Reset GPIO of frontend");
static int frontend_reset = -1;
module_param(frontend_reset, int, S_IRUGO);

MODULE_PARM_DESC(frontend_i2c, "\n\t\t IIc adapter id of frontend");
static int frontend_i2c = -1;
module_param(frontend_i2c, int, S_IRUGO);

MODULE_PARM_DESC(frontend_tuner_addr, "\n\t\t Tuner IIC address of frontend");
static int frontend_tuner_addr = -1;
module_param(frontend_tuner_addr, int, S_IRUGO);

MODULE_PARM_DESC(frontend_demod_addr, "\n\t\t Demod IIC address of frontend");
static int frontend_demod_addr = -1;
module_param(frontend_demod_addr, int, S_IRUGO);

MODULE_PARM_DESC(frontend_power, "\n\t\t ANT_PWR_CTRL of frontend");
static int frontend_power = -1;
module_param(frontend_power, int, S_IRUGO);

//static int gSystemMode = 1; //  0=ATSC jB38 ; 1=ATSC 8vsb
//static int gInitFlag = 0 ;

static DMD_PARAMETER param;
static struct mn88436_fe_config mn88436_cfg;


#if 0
static int MN88436_Reset(void)
{
	//reset demod
	gpio_direction_output(frontend_reset, 0);
	msleep(200);
	gpio_direction_output(frontend_reset, 1);
	//enable tuner
	gpio_direction_output(frontend_power, 1);
	return 0;
}
static int MN88436_I2c_Gate_Ctrl(struct dvb_frontend *fe, int enable)
{
	return 0;
}
#endif

#define TYPE_ISDB 1
#define TYPE_DVB 2
#define TYPE_ATSC 3

//extern int dvb_demod_mach_id;
#if 0
static int MN88436_Init(struct dvb_frontend *fe)
{
	pr_dbg("frontend_reset is %d\n",frontend_reset);
	//reset
	MN88436_Reset();
	msleep(100);
	//Select device ID
	param.devid = 0;
	if(gSystemMode == 0)
	{
		param.system=DMD_E_QAMB_256QAM;
	}
	else
	{
		param.system=DMD_E_ATSC;
	}
	param.ts_out_mode=DMD_E_TSOUT_PARALLEL_BRTG_MODE;
//	DMD_PARAMETER param;
	
	//Initialize LSI
	printf("Initializing LSI .. \n");
	if( DMD_init(&param) == DMD_E_OK )
	{
		pr_dbg("DEMOD OK\n");
		//dvb_demod_mach_id = TYPE_ATSC;//TYPE_DVB;//TYPE_ATSC;
	}
	else
		pr_dbg("DEMOD NG\n");
	DMD_set_system(&param);
	/*if(MDrv_Tuner_Init()==1)
		pr_dbg("TUNER OK\n");
	else
		pr_dbg("TUNER NG\n");
	*/

//	pr_dbg("0x%x(ptuner),0x%x(pavchip)=========================demod init\r\n",mn88436pTuner->m_uiSlaveAddress,pAVLChip_all->m_SlaveAddr);
	gInitFlag = 1;
	msleep(200);
	return 0;
}
static int MN88436_Sleep(struct dvb_frontend *fe)
{
	//struct mn88436_state *state = fe->demodulator_priv;
	return 0;
}
#endif

static int MN88436_Read_Status(struct dvb_frontend *fe, fe_status_t * status)
{
	unsigned char s=0;
	DMD_get_info(&param,DMD_E_INFO_LOCK);
	if(param.info[DMD_E_INFO_LOCK]==0)
		s=1;

	if(s==1)
	{
		*status = FE_HAS_LOCK|FE_HAS_SIGNAL|FE_HAS_CARRIER|FE_HAS_VITERBI|FE_HAS_SYNC;
	}
	else
	{
		*status = FE_TIMEDOUT;
	}
	
	return  0;
}

static int MN88436_Read_Ber(struct dvb_frontend *fe, u32 * ber)
{
	DMD_get_info(&param,DMD_E_INFO_PERRNUM);
	*ber=param.info[DMD_E_INFO_PERRNUM] ;
	return 0;
}

static int MN88436_Read_Signal_Strength(struct dvb_frontend *fe, u16 *strength)
{
	*strength=DMD_get_info(&param,DMD_E_INFO_ALL);
	return 0;
}

static int MN88436_Read_Snr(struct dvb_frontend *fe, u16 * snr)
{
	DMD_get_info(&param,DMD_E_INFO_CNR_DEC);
	*snr=param.info[DMD_E_INFO_CNR_INT] ;
	return 0;
}

static int MN88436_Read_Ucblocks(struct dvb_frontend *fe, u32 * ucblocks)
{
	ucblocks=NULL;
	return 0;
}

static int MN88436_Set_Frontend(struct dvb_frontend *fe)
{
	int count;
	struct mn88436_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	pr_dbg("MN88436_set_frontend\n");
	//DMD_PARAMETER param;
	param.freq=c->frequency;
	param.funit=DMD_E_KHZ;
	//MDrv_Tuner_SetTuner(param.freq,6);
	//MDrv_Tuner_SetTuner(474000,6);
	DMD_scan(&param);
	DMD_tune(&param);
	state->freq=c->frequency;
	state->mode=c->modulation ; //these data will be writed to eeprom
	DMD_get_info(&param,DMD_E_INFO_CNR_DEC);
	DMD_get_info(&param,DMD_E_INFO_PERRNUM);

	pr_dbg("mn88436=>frequency=%d ,state->mode = %d \r\n",c->frequency,state->mode);
	for(count=0;count<20;count++){
		DMD_get_info(&param,DMD_E_INFO_LOCK);
		if(param.info[DMD_E_INFO_LOCK]==0)
			break;
		msleep(50);

	}
	pr_dbg("mn88436=>frequency=%d\r\n",c->frequency);
	return  0;
}

static int MN88436_Get_Frontend(struct dvb_frontend *fe)
{//these content will be writed into eeprom .

	struct mn88436_state *state = fe->demodulator_priv;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	c->frequency=state->freq;
	c->modulation=state->mode;
	
	return 0;
}

#if 0
static void MN88436_Release(struct dvb_frontend *fe)
{
	struct mn88436_state *state = fe->demodulator_priv;
	kfree(state);
}

static ssize_t mn_frontend_show_short_circuit(struct class* class, struct class_attribute* attr, char* buf)
{
	return 0;
}

static struct class_attribute mn_frontend_class_attrs[] = {
	__ATTR(short_circuit,  S_IRUGO | S_IWUSR, mn_frontend_show_short_circuit, NULL),
	__ATTR_NULL
};

static struct class mn_frontend_class = {
	.name = "mn_frontend",
	.class_attrs = mn_frontend_class_attrs,
};

static struct dvb_frontend_ops mn88436_ops;

struct dvb_frontend *mn88436_attach(const struct mn88436_fe_config *config)
{
	struct mn88436_state *state = NULL;

	/* allocate memory for the internal state */
	
	state = kmalloc(sizeof(struct mn88436_state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	/* setup the state */
	state->config = *config;
	
	/* create dvb_frontend */
	memcpy(&state->fe.ops, &mn88436_ops, sizeof(struct dvb_frontend_ops));
	state->fe.demodulator_priv = state;
	
	return &state->fe;
}
#endif
static int mn88436_fe_get_ops(struct aml_fe_dev *dev, int mode, void *ops)
{
	struct dvb_frontend_ops *fe_ops = (struct dvb_frontend_ops*)ops;

	fe_ops->info.frequency_min = 51000000;
	fe_ops->info.frequency_max = 858000000;
	fe_ops->info.frequency_stepsize = 0;
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

	fe_ops->set_frontend = MN88436_Set_Frontend;
	fe_ops->get_frontend = MN88436_Get_Frontend; 
	fe_ops->read_status = MN88436_Read_Status;
	fe_ops->read_ber = MN88436_Read_Ber;
	fe_ops->read_signal_strength = MN88436_Read_Signal_Strength;
	fe_ops->read_snr = MN88436_Read_Snr;
	fe_ops->read_ucblocks = MN88436_Read_Ucblocks;
	mn88436_cfg.demod_addr=dev->i2c_addr;
	mn88436_cfg.i2c_adapter=dev->i2c_adap;
	printk("i2c_adap_id is %d,i2c_addr is %x\n",dev->i2c_adap_id,dev->i2c_addr);
	
	return 0;
}

/*static struct dvb_frontend_ops mn88436_ops = {


		.info = {
		 .name = "AMLOGIC ATSC",
		.type = FE_ATSC,
		.frequency_min = 51000000,
		.frequency_max = 858000000,
		.frequency_stepsize = 0,
		.frequency_tolerance = 0,
		.caps =
			FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_QPSK | FE_CAN_QAM_16 |
			FE_CAN_QAM_64 | FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_HIERARCHY_AUTO |
			FE_CAN_RECOVER |
			FE_CAN_MUTE_TS
	},

	.release = MN88436_Release,

	.init = MN88436_Init,
	.sleep = MN88436_Sleep,
	.i2c_gate_ctrl = MN88436_I2c_Gate_Ctrl,

	.set_frontend = MN88436_Set_Frontend,
	.get_frontend = MN88436_Get_Frontend,	
	.read_status = MN88436_Read_Status,
	.read_ber = MN88436_Read_Ber,
	.read_signal_strength =MN88436_Read_Signal_Strength,
	.read_snr = MN88436_Read_Snr,
	.read_ucblocks = MN88436_Read_Ucblocks,

};

static void mn88436_fe_release(struct aml_dvb *advb, struct aml_fe *fe)
{
	if(fe && fe->fe) {
		pr_dbg("release mn88436 frontend %d\n", fe->id);
		dvb_unregister_frontend(fe->fe);
		dvb_frontend_detach(fe->fe);
		if(fe->cfg){
			kfree(fe->cfg);
			fe->cfg = NULL;
		}
		fe->id = -1;
	}
}
*/


static int mn88436_fe_enter_mode(struct aml_fe *fe, int mode)
{
	int ret;
	struct aml_fe_dev *dev = fe->dtv_demod;
	pr_dbg("=========================mn88436 demod init\r\n");
	//reset
	ret=amlogic_gpio_request(dev->reset_gpio, "mn88436_reset");
	ret = amlogic_gpio_direction_output(dev->reset_gpio, dev->reset_value, "mn88436_reset");
	msleep(300);
	ret=amlogic_gpio_direction_output(dev->reset_gpio, !dev->reset_value, "mn88436_reset"); //enable tuner power
	msleep(200);
	//Select device ID
	param.devid = 0;
	param.system=DMD_E_ATSC;//DMD_E_QAMB_256QAM;//DMD_E_ATSC;
	param.ts_out_mode=DMD_E_TSOUT_PARALLEL_BRTG_MODE;
	//DMD_PARAMETER param;
	
	//Initialize LSI
	printf("Initializing LSI .. \n");
	if( DMD_init(&param) == DMD_E_OK )
	{
		pr_dbg("DEMOD OK\n");
	}
	else
		pr_dbg("DEMOD NG\n");
	DMD_set_system(&param);
	msleep(200);
	return 0;
}

#if 0
static int mn88436_fe_init(struct aml_dvb *advb, struct platform_device *pdev, struct aml_fe *fe, int id)
{
	struct dvb_frontend_ops *ops;
	int ret;
	struct resource *res;
	struct mn88436_fe_config *cfg;
	char buf[32];
	
	pr_dbg("init mn88436 frontend %d\n", id);


	cfg = kzalloc(sizeof(struct mn88436_fe_config), GFP_KERNEL);
	if (!cfg)
		return -ENOMEM;
	
	cfg->reset_pin = frontend_reset;
	if(cfg->reset_pin==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_reset_pin", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->reset_pin = res->start;		
	}
	
	cfg->i2c_id = frontend_i2c;
	if(cfg->i2c_id==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_i2c", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->i2c_id = res->start;
	}
	
	cfg->tuner_addr = frontend_tuner_addr;
	
	if(cfg->tuner_addr==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_tuner_addr", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->tuner_addr = res->start>>1;
	}
	
	cfg->demod_addr = frontend_demod_addr;
	if(cfg->demod_addr==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_demod_addr", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		cfg->demod_addr = res->start>>1;
	}
	if(frontend_power==-1) {
		snprintf(buf, sizeof(buf), "frontend%d_power", id);
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, buf);
		if (!res) {
			pr_error("cannot get resource \"%s\"\n", buf);
			ret = -EINVAL;
			goto err_resource;
		}
		frontend_power = res->start;
	}
	
	frontend_reset = cfg->reset_pin;
	frontend_i2c = cfg->i2c_id;
	frontend_tuner_addr = cfg->tuner_addr;
	frontend_demod_addr = cfg->demod_addr;	
	fe->fe = mn88436_attach(cfg);
	if (!fe->fe) {
		ret = -ENOMEM;
		goto err_resource;
	}

	if ((ret=dvb_register_frontend(&advb->dvb_adapter, fe->fe))) {
		pr_error("frontend registration failed!");
		ops = &fe->fe->ops;
		if (ops->release != NULL)
			ops->release(fe->fe);
		fe->fe = NULL;
		goto err_resource;
	}
	
	fe->id = id;
	fe->cfg = cfg;
	
	return 0;

err_resource:
	kfree(cfg);
	return ret;
}
#endif

int mn88436_get_fe_config(struct mn88436_fe_config *cfg)
{
	cfg->demod_addr=mn88436_cfg.demod_addr;
	cfg->i2c_adapter=mn88436_cfg.i2c_adapter;
	return 0;
	/*
	struct i2c_adapter *i2c_handle;
	cfg->i2c_id = frontend_i2c;
	cfg->demod_addr = frontend_demod_addr;
	cfg->tuner_addr = frontend_tuner_addr;
	cfg->reset_pin = frontend_reset;
	//printk("\n frontend_i2c is %d,,frontend_demod_addr is %x,frontend_tuner_addr is %x,frontend_reset is %d",frontend_i2c,frontend_demod_addr,frontend_tuner_addr,frontend_reset);
	i2c_handle = i2c_get_adapter(cfg->i2c_id);
	if (!i2c_handle) {
		printk("cannot get i2c adaptor\n");
		return 0;
	}
	cfg->i2c_adapter =i2c_handle;
	//printk("\n frontend0_i2c is %d, frontend_i2c is %d,",frontend0_i2c,frontend_i2c);
	return 1;
	*/
}
/*
static int mn88436_fe_probe(struct platform_device *pdev)
{
	printk("Johnny-atsc, mn88436_fe_probe start\n");
	int ret = 0;
	struct aml_dvb *dvb = aml_get_dvb_device();
	
	if(mn88436_fe_init(dvb, pdev, &mn88436_fe[0], 0)<0)
		return -ENXIO;

	platform_set_drvdata(pdev, &mn88436_fe[0]);

	if((ret = class_register(&mn_frontend_class))<0) {
		pr_error("register class error\n");

		struct aml_fe *drv_data = platform_get_drvdata(pdev);
		
		platform_set_drvdata(pdev, NULL);
	
		mn88436_fe_release(dvb, drv_data);

		return ret;
	}

	printk("Johnny-atsc, mn88436_fe_probe end\n");
	return ret;
}

static int mn88436_fe_remove(struct platform_device *pdev)
{
	struct aml_fe *drv_data = platform_get_drvdata(pdev);
	struct aml_dvb *dvb = aml_get_dvb_device();

	class_unregister(&mn_frontend_class);

	platform_set_drvdata(pdev, NULL);
	
	mn88436_fe_release(dvb, drv_data);
	
	return 0;
}
*/
static int mn88436_fe_resume(struct aml_fe_dev *dev)
{
	pr_dbg("mn88436_fe_resume \n");
	//reset
	amlogic_gpio_request(dev->reset_gpio,"mn88436_reset");
	amlogic_gpio_direction_output(dev->reset_gpio, dev->reset_value, "mn88436_reset");
	msleep(300);
	amlogic_gpio_direction_output(dev->reset_gpio, !dev->reset_value, "mn88436_reset"); //enable tuner power
	msleep(200);
	//Select device ID
	param.devid = 0;
	param.system=DMD_E_ATSC;
		param.ts_out_mode=DMD_E_TSOUT_PARALLEL_BRTG_MODE;
	//	DMD_PARAMETER param;
		
	//Initialize LSI
	printf("Initializing LSI .. \n");
	if( DMD_init(&param) == DMD_E_OK )
		pr_dbg("DEMOD OK\n");
	else
		pr_dbg("DEMOD NG\n");
	DMD_set_system(&param);
	/*
	if(MDrv_Tuner_Init()==1)
		pr_dbg("TUNER OK\n");
	else
		pr_dbg("TUNER NG\n");
	*/

	return 0;

}

static int mn88436_fe_suspend(struct aml_fe_dev *dev)
{
	return 0;
}


static struct aml_fe_drv mn88436_dtv_demod_drv = {
	.id 		= AM_DTV_DEMOD_MN88436,
	.name		= "Mn88436",
	.capability = AM_FE_QAM|AM_FE_ATSC,
	.get_ops	= mn88436_fe_get_ops,
	.enter_mode = mn88436_fe_enter_mode,
	.suspend	= mn88436_fe_suspend,
	.resume 	= mn88436_fe_resume
};

static int __init mnfrontend_init(void)
{
	printk("Johnny-atsc, mnfrontend_init\n");
	return aml_register_fe_drv(AM_DEV_DTV_DEMOD, &mn88436_dtv_demod_drv);
	//return platform_driver_register(&aml_fe_driver);
}


static void __exit mnfrontend_exit(void)
{
	printk("unregister mnfrontend demod driver\n");
	aml_unregister_fe_drv(AM_DEV_DTV_DEMOD, &mn88436_dtv_demod_drv);
	//platform_driver_unregister(&aml_fe_driver);
}

fs_initcall(mnfrontend_init);
module_exit(mnfrontend_exit);


MODULE_DESCRIPTION("mn88436 atsc Demodulator driver");
MODULE_AUTHOR("RSJ");
MODULE_LICENSE("GPL");



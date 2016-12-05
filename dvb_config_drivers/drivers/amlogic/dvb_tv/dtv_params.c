#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/i2c-gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>

#define pr_dbg(args...)\
	do {\
		printk(args);\
	} while (0)
#define pr_error(fmt, args...) printk("dtv_params: " fmt, ## args)
#define pr_inf(fmt, args...)   printk("dtv_params: " fmt, ## args)


//static struct pinctrl *s_pinctrl = NULL;

static unsigned int s_demodResetPin, s_antPowerPin;
static unsigned char demodPinDetected = 0, antPowerPinDetected = 0;

static void set_demod_reset_pin(unsigned char val)
{
    if(demodPinDetected){
    	gpio_set_value(s_demodResetPin, val);
        pr_err("set_demod_reset_pin : %d\r\n", val);
    }
    else{
        pr_err("error : demod reset pin not config..\r\n");
    }
}

static void set_ant_power_pin(unsigned char val)
{
    if(antPowerPinDetected){
    	gpio_set_value(s_antPowerPin, val);
        pr_err("set_ant_power_pin : %d\r\n", val);
    }
    else{
        pr_err("error : ant power pin not config..\r\n");
    }
}

static int get_demod_reset_pin(void)
{
    int val = 0;
    
    if(demodPinDetected){
        val = gpio_get_value(s_demodResetPin);
    }

    return val;
}

static int get_ant_power_pin(void)
{
    int val = 0;
    
    if(antPowerPinDetected){
        val = gpio_get_value(s_antPowerPin);
    }

    return val;
}

static ssize_t stb_show_demod_reset_pin(struct class *class,
			       struct class_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	if(demodPinDetected)
    	ret = sprintf(buf, "%d\n", get_demod_reset_pin());
    else
    	ret = sprintf(buf, "error : demod reset pin not config..\n");
    
	return ret;
}

static ssize_t stb_store_demod_reset_pin(struct class *class,
				struct class_attribute *attr, const char *buf,
				size_t size)
{
	if (!strncmp("0", buf, 1))
        set_demod_reset_pin(0);
    else    
        set_demod_reset_pin(1);

	return size;
}

static ssize_t stb_show_ant_power_pin(struct class *class,
			       struct class_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	
	if(antPowerPinDetected)
    	ret = sprintf(buf, "%d\n", get_ant_power_pin());
    else
    	ret = sprintf(buf, "error : ant power pin not config..\n");
    	
	return ret;
}

static ssize_t stb_store_ant_power_pin(struct class *class,
				struct class_attribute *attr, const char *buf,
				size_t size)
{
	if (!strncmp("0", buf, 1))
        set_ant_power_pin(0);
    else    
        set_ant_power_pin(1);

	return size;
}

static struct class_attribute dtv_params_class_attrs[] = {
	__ATTR(demod_reset_pin, S_IRUGO | S_IWUSR | S_IWGRP, stb_show_demod_reset_pin,
	       stb_store_demod_reset_pin),
	__ATTR(ant_power_pin, S_IRUGO | S_IWUSR | S_IWGRP, stb_show_ant_power_pin,
	       stb_store_ant_power_pin),

	__ATTR_NULL
};


static struct class dtv_params_class = {
	.name = "dtv-params",
	.class_attrs = dtv_params_class_attrs,
};

static int dtv_params_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_inf("probe dtv params driver : start\n");

	if (pdev->dev.of_node) {
        s_demodResetPin = of_get_named_gpio_flags(pdev->dev.of_node, "demod_reset-gpio", 0, NULL);
        s_antPowerPin = of_get_named_gpio_flags(pdev->dev.of_node, "ant_power-gpio", 0, NULL);

    	if (!gpio_is_valid(s_demodResetPin)) {
    		pr_err("%s: invalid GPIO pin, s_demodResetPin=%d\n",
    		       pdev->dev.of_node->full_name, s_demodResetPin);
    		return -ENODEV;
    	}
    	else{
			demodPinDetected = 1;
			gpio_request(s_demodResetPin,"dtv params");
			gpio_direction_output(s_demodResetPin, 1);//output mode
    	}
    	
    	if (!gpio_is_valid(s_antPowerPin)) {
    		pr_err("%s: invalid GPIO pin, s_antPowerPin=%d, skip this pin\n",
    		       pdev->dev.of_node->full_name, s_antPowerPin);
    	}
    	else{
			antPowerPinDetected = 1;
			gpio_request(s_antPowerPin,"dtv params");
			gpio_direction_output(s_antPowerPin, 0); //output mode
    	}

//        s_pinctrl = devm_pinctrl_get_select(&pdev->dev, "default");
//   		pr_err("set pinctrl : %p\n", s_pinctrl);
    	
    	if (class_register(&dtv_params_class) < 0) {
    		pr_error("register class error\n");
    		goto error;
    	}
	}
   
	pr_inf("probe dtv params driver : end\n");
	
	return 0;
error:

	return ret;
}

static int dtv_params_remove(struct platform_device *pdev)
{
//	if (s_pinctrl) {
//		devm_pinctrl_put(s_pinctrl);
//		s_pinctrl = NULL;
//	}

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id dtv_params_dt_match[] = {
	{
	 .compatible = "dtv-params",
	 },
	{},
};
#endif /*CONFIG_OF */

static struct platform_driver dtv_params_driver = {
	.probe = dtv_params_probe,
	.remove = dtv_params_remove,
	.driver = {
		   .name = "dtv-params",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = dtv_params_dt_match,
#endif
		   }
};

static int __init dtv_params_init(void)
{
	pr_dbg("dtv params init\n");
	return platform_driver_register(&dtv_params_driver);
}

static void __exit dtv_params_exit(void)
{
	pr_dbg("dtv params exit\n");
	platform_driver_unregister(&dtv_params_driver);
}

module_init(dtv_params_init);
module_exit(dtv_params_exit);

MODULE_DESCRIPTION("driver for the AMLogic dtv params");
MODULE_AUTHOR("AMLOGIC");
MODULE_LICENSE("GPL");

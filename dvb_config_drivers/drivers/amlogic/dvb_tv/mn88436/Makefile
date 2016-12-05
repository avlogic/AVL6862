obj-$(CONFIG_AM_MN88436) += mn88436_fe.o

mn88436_src = src
nmi120_src = nmi120
nmi120_src_driver = nmi120/nmi_driver
mn88436_fe-objs +=  mnfrontend.o	
mn88436_fe-objs +=  $(mn88436_src)/MN_DMD_common.o  $(mn88436_src)/MN_DMD_device.o	 $(mn88436_src)/MN_DMD_driver.o	$(mn88436_src)/MN_I2C.o  $(mn88436_src)/MN_TCB.o	\
										$(mn88436_src)/MN_Tuner.o  $(mn88436_src)/MN88436_autoctrl.o  $(mn88436_src)/MN88436_regset.o	##$(mn88436_src)/MN_DMD_console.o	
										

#mn88436_fe-objs +=  $(nmi120_src)/nmi_tuner_os.o	$(nmi120_src)/nmituner_api.o	$(nmi120_src_driver)/nmi_tuner_interface.o	$(nmi120_src_driver)/nmicmn.o \
										$(nmi120_src_driver)/nmitnr.o

mn88436_fe-objs +=  mxl603/MxL603_Main.o	mxl603/MxL603_OEM_Drv.o	mxl603/MxL603_TunerApi.o	mxl603/MxL603_TunerCfg.o	

mn88436_fe-objs +=  #Register_setting/MN88436_regset_24Mhz_IF5Mhz.o	Register_setting/MN88436_regset_24MHz_IF44MHz.o	Register_setting/MN88436_regset_24MHz_IF44MHz_change_for_LG_CAN_tuner_121017.o	Register_setting/MN88436_regset_25Mhz_IF5Mhz.o	Register_setting/MN88436_regset_25Mhz_IF44Mhz.o	

EXTRA_CFLAGS += -Idrivers/media/dvb-core
EXTRA_CFLAGS += -Idrivers/amlogic/dvb_tv/mn88436/include
EXTRA_CFLAGS += -Idrivers/amlogic/dvb_tv/mn88436
#EXTRA_CFLAGS += -Idrivers/amlogic/dvb/mn88436/nmi120
#EXTRA_CFLAGS += -Idrivers/amlogic/dvb/mn88436/nmi120/nmi_driver


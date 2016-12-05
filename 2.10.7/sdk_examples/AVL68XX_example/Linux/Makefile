
CXXFLAGS = -Wall -Wextra -Winit-self -Woverloaded-virtual -Wuninitialized -Wmissing-declarations -std=c++11 -O3
CXXFLAGS += -I../../../sdk_src
CXXFLAGS += -I../../../tuner
CXXFLAGS += -I../../../tuner/MxL601
CXXFLAGS += -I../../../tuner/AV2011

SRCFILES = ../AVL68XX_PortingExample.c
SRCFILES += ../../../sdk_src/AVL_Demod_CommonAPI.c
SRCFILES += ../../../sdk_src/AVL_Demod_CommonInternal.c
SRCFILES += ../../../sdk_src/AVL_Demod_DVBC.c
SRCFILES += ../../../sdk_src/AVL_Demod_DVBSx.c
SRCFILES += ../../../sdk_src/AVL_Demod_DVBTx.c
SRCFILES += ../../../sdk_src/AVL_Demod_ISDBT.c
SRCFILES += ../../../sdk_src/user_defined_function.c
SRCFILES += ../../../sdk_src/AVL_Demod_Config.c

SRCFILES += ../../../tuner/MxL601/MxL601.c
SRCFILES += ../../../tuner/MxL601/MxL601_TunerApi.c
SRCFILES += ../../../tuner/MxL601/MxL601_TunerCfg.c
SRCFILES += ../../../tuner/MxL601/MxL601_OEM_Drv.c
SRCFILES += ../../../tuner/MxL601/MxL601_TunerSpurTable.c

SRCFILES += ../../../tuner/AV2011/ExtAV2011.c

BINFILE = AVL68XX_PortingExample

all:
	g++ $(CXXFLAGS) $(SRCFILES) -o $(BINFILE)
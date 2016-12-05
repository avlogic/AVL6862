//-----------------------------------------------------//
//
// Filename: R848.c
//
// This file is R848 tuner driver
// Copyright 2014 by Rafaelmicro., Inc.
// Author: Ryan Chang
//-----------------------------------------------------//

//#include "stdafx.h"
#include "R848.h"
#include "R848_API.h"
#include "stdio.h"
//#include "..\I2C_Sys.h"    // "I2C_Sys" is only for SW porting reference.


UINT32 ADC_READ_DELAY = 3;
UINT8  ADC_READ_COUNT = 1;
                                                                      

//PLL LO=161MHz (R20:0x70 ; R24:0x70 ; R28:0x10 ; R29:0x00 ; R30:0x80)

UINT8 R848_iniArray_hybrid[R848_REG_NUM] = {
						0x00, 0x00, 0x40, 0x44, 0x17, 0x00, 0x06, 0xF0, 0x00, 0x41,
					//  0x08  0x09  0x0A  0x0B  0x0C  0x0D  0x0E  0x0F  0x10  0x11
						0x7B, 0x0B, 0x70, 0x06, 0x6E, 0x20, 0x70, 0x87, 0x96, 0x00,
					//  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1A  0x1B  
						0x10, 0x00, 0x80, 0xA5, 0xB7, 0x00, 0x40, 0xCB, 0x95, 0xF0,
					//  0x1C  0x1D  0x1E  0x1F  0x20  0x21  0x22  0x23  0x24  0x25
						0x24, 0x00, 0xFD, 0x8B, 0x17, 0x13, 0x01, 0x07, 0x01, 0x3F};
					//  0x26  0x27  0x28  0x29  0x2A  0x2B  0x2C  0x2D  0x2E  0x2F



UINT8 R848_iniArray_dvbs[R848_REG_NUM] = {
						0x80, 0x05, 0x40, 0x40, 0x1F, 0x1F, 0x07, 0xFF, 0x00, 0x40,
					//  0x08  0x09  0x0A  0x0B  0x0C  0x0D  0x0E  0x0F  0x10  0x11
						0xF0, 0x0F, 0x4D, 0x06, 0x6F, 0x20, 0x28, 0x83, 0x96, 0x00,  //0x16[1] pulse_flag HPF : Bypass ;  0x19[1:0] Deglich SW Cur : highest
					//  0x12  0x13  0x14  0x15  0x16  0x17  0x18  0x19  0x1A  0x1B   
						0x1C, 0x99, 0xC1, 0x83, 0xB7, 0x00, 0x4F, 0xCB, 0x95, 0xFD,
					//  0x1C  0x1D  0x1E  0x1F  0x20  0x21  0x22  0x23  0x24  0x25
						0xA4, 0x01, 0x24, 0x0B, 0x4F, 0x05, 0x01, 0x47, 0x3F, 0x3F};
					//  0x26  0x27  0x28  0x29  0x2A  0x2B  0x2C  0x2D  0x2E  0x2F




UINT8 R848_ADDRESS = 0xF4;			
UINT8 R848_XtalDiv = XTAL_DIV2;
UINT8 R848_SetTfType = R848_TF_BEAD;   //Depend on HW
//UINT8 R848_SetTfType = R848_TF_82N_270N;   //Depend on HW
UINT8 R848_DetectTfType = R848_UL_USING_BEAD;
//UINT8 R848_DetectTfType = R848_UL_USING_270NH;
UINT8 R848_Fil_Cal_Gap = 8;
UINT32 R848_IF_HIGH = 8500;  
UINT8 R848_Xtal_Pwr = XTAL_SMALL_HIGHEST;
UINT8 R848_Xtal_Pwr_tmp = XTAL_LARGE_HIGHEST;

//----------------------------------------------------------//
//                   Internal Parameters                    //
//----------------------------------------------------------//

UINT8 R848_Array[40];
R848_SectType R848_IMR_Data[5] = {
                                                  {0, 0, 0, 0},
                                                  {0, 0, 0, 0},
                                                  {0, 0, 0, 0},
                                                  {0, 0, 0, 0},
                                                  {0, 0, 0, 0}
                                                };//Please keep this array data for standby mode.
I2C_TYPE  R848_I2C;
I2C_LEN_TYPE R848_I2C_Len;
I2C_LEN_TYPE Test_Len;

UINT8  R848_IMR_point_num;
UINT8  R848_Initial_done_flag = FALSE;
UINT8  R848_Initial_xtal_check_flag = FALSE;
UINT8  R848_IMR_done_flag = FALSE;
UINT8  R848_Bandwidth = 0x00;
UINT8  R848_SATELLITE_FLAG = 0;
UINT8  R848_Fil_Cal_flag[R848_STD_SIZE];
static UINT8 R848_Fil_Cal_BW[R848_STD_SIZE];
static UINT8 R848_Fil_Cal_code[R848_STD_SIZE];
static R848_Standard_Type R848_pre_standard = R848_STD_SIZE;
static UINT8 R848_IMR_Cal_Type = R848_IMR_CAL;


//0: L270n/68n(ISDB-T, DVB-T/T2)
//1: Bead/68n(DTMB)
//2: L270n/68n(N/A)
//3: L270n/68n(ATV)
//4: Bead/68n(DTMB+ATV)
//5: L270n/68n(ATSC, DVB-C, J83B)
//6: Bead/68n(ATSC, DVB-C, J83B)
//7: Bead/82n(R848_DTMB)
//8: L270n/82n(R848_DVB-C, J83B, ATSC, DVB-T/T2, ISDB-T)
UINT32 R848_LNA_HIGH_MID[R848_TF_SIZE] = { 644000, 644000, 644000, 644000, 644000, 500000, 500000, 500000, 500000}; 
UINT32 R848_LNA_MID_LOW[R848_TF_SIZE] = { 388000, 388000, 348000, 348000, 348000, 300000, 300000, 300000, 300000};
UINT32 R848_LNA_LOW_LOWEST[R848_TF_SIZE] = {164000, 164000, 148000, 124000, 124000, 156000, 156000, 108000, 108000};

UINT32 R848_TF_Freq_High[R848_TF_SIZE][R848_TF_HIGH_NUM] = 
{  	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
	 { 784000, 784000, 776000, 744000, 712000, 680000, 648000, 647000},
     { 784000, 784000, 776000, 680000, 608000, 584000, 536000, 504000},
	 { 784000, 784000, 776000, 680000, 608000, 584000, 536000, 504000},
	 { 784000, 776000, 712000, 616000, 584000, 560000, 520000, 504000},
	 { 784000, 776000, 712000, 616000, 584000, 560000, 520000, 504000}
};


UINT32 R848_TF_Freq_Mid[R848_TF_SIZE][R848_TF_MID_NUM] = 
{	  {608000, 584000, 560000, 536000, 488000, 440000, 416000, 392000},
	  {608000, 584000, 560000, 536000, 488000, 440000, 416000, 392000},
	  {608000, 560000, 536000, 488000, 440000, 392000, 376000, 352000},
	  {608000, 560000, 536000, 488000, 440000, 392000, 376000, 352000},
	  {608000, 560000, 536000, 488000, 440000, 392000, 376000, 352000},
      {488000, 464000, 440000, 416000, 392000, 352000, 320000, 304000},
	  {488000, 464000, 440000, 416000, 392000, 352000, 320000, 304000},
	  {480000, 464000, 440000, 416000, 392000, 352000, 320000, 304000},
	  {480000, 464000, 440000, 416000, 392000, 352000, 320000, 304000},
};
UINT32 R848_TF_Freq_Low[R848_TF_SIZE][R848_TF_LOW_NUM] = 
{    {320000, 304000, 272000, 240000, 232000, 216000, 192000, 168000},  //164~388
      {320000, 304000, 272000, 240000, 232000, 216000, 192000, 168000},  //164~388
	  {256000, 240000, 232000, 224000, 216000, 192000, 168000, 160000},  //148~348
	  {256000, 240000, 232000, 192000, 160000, 152000, 144000, 128000},  //124~348
	  {264000, 240000, 192000, 184000, 176000, 152000, 144000, 128000},  //124~348
      {280000, 248000, 232000, 216000, 192000, 176000, 168000, 160000},  //156~300
      {280000, 248000, 232000, 216000, 192000, 176000, 168000, 160000},   //156~300
	  {296000, 280000, 256000, 216000, 184000, 168000, 136000, 112000},   //
	  {296000, 280000, 256000, 216000, 184000, 168000, 136000, 112000}   //
};
UINT32 R848_TF_Freq_Lowest[R848_TF_SIZE][R848_TF_LOWEST_NUM] = 
{    {160000, 120000, 104000, 88000, 80000, 72000, 56000, 48000},
      {160000, 120000, 104000, 88000, 80000, 72000, 56000, 48000},
	  {144000, 120000, 104000, 88000, 80000, 72000, 56000, 48000},
	  {120000, 96000,   88000,   80000, 72000, 64000, 56000, 48000},
	  {104000, 96000,   88000,   80000, 72000, 64000, 56000, 48000},
	  {136000, 120000, 104000, 88000, 72000, 64000, 56000, 48000},
	  {128000, 120000, 104000, 96000, 80000, 72000, 64000, 56000},
	  {104000, 96000, 88000, 80000, 72000, 64000, 56000, 48000},
	  {104000, 96000, 88000, 80000, 72000, 64000, 56000, 48000}
};

UINT8 R848_TF_Result_High[R848_TF_SIZE][R848_TF_HIGH_NUM] = 
{    {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
      {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
	  {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
	  {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
	  {0x00, 0x00, 0x01, 0x03, 0x04, 0x05, 0x07, 0x07},
      {0x00, 0x00, 0x01, 0x05, 0x0A, 0x0C, 0x13, 0x19},
	  {0x00, 0x00, 0x01, 0x05, 0x0A, 0x0C, 0x13, 0x19},
	  {0x00, 0x03, 0x07, 0x0C, 0x0E, 0x0F, 0x16, 0x1A},
	  {0x00, 0x03, 0x07, 0x0C, 0x0E, 0x0F, 0x16, 0x1A}
};

UINT8 R848_TF_Result_Mid[R848_TF_SIZE][R848_TF_MID_NUM] = 
{    {0x00, 0x01, 0x03, 0x03, 0x06, 0x0B, 0x0E, 0x11},
      {0x00, 0x01, 0x03, 0x03, 0x06, 0x0B, 0x0E, 0x11},
	  {0x00, 0x03, 0x03, 0x06, 0x0B, 0x11, 0x12, 0x19},  
	  {0x00, 0x03, 0x03, 0x06, 0x0B, 0x11, 0x12, 0x19},  
	  {0x00, 0x03, 0x03, 0x06, 0x0B, 0x11, 0x12, 0x19},
	  {0x06, 0x08, 0x0B, 0x0E, 0x13, 0x17, 0x1E, 0x1F},
      {0x06, 0x08, 0x0B, 0x0E, 0x13, 0x17, 0x1E, 0x1F},
	  {0x09, 0x0D, 0x10, 0x12, 0x16, 0x1B, 0x1E, 0x1F},
	  {0x09, 0x0D, 0x10, 0x12, 0x16, 0x1B, 0x1E, 0x1F}
};
UINT8 R848_TF_Result_Low[R848_TF_SIZE][R848_TF_LOW_NUM] = 
{    {0x00, 0x02, 0x04, 0x07, 0x0A, 0x0B, 0x0F, 0x16},
      {0x00, 0x02, 0x04, 0x07, 0x0A, 0x0B, 0x0F, 0x16},
	  {0x05, 0x07, 0x0A, 0x0B, 0x0B, 0x0F, 0x16, 0x1A},
	  {0x05, 0x07, 0x0A, 0x0F, 0x1A, 0x1A, 0x23, 0x2A},
	  {0x05, 0x08, 0x10, 0x13, 0x1A, 0x1A, 0x23, 0x2A},
	  {0x05, 0x08, 0x0C, 0x0E, 0x10, 0x14, 0x18, 0x1A},
	  {0x05, 0x08, 0x0C, 0x0E, 0x10, 0x14, 0x18, 0x1A},
	  {0x00, 0x01, 0x03, 0x07, 0x0D, 0x11, 0x1E, 0x2F},
	  {0x00, 0x01, 0x03, 0x07, 0x0D, 0x11, 0x1E, 0x2F}
};
UINT8 R848_TF_Result_Lowest[R848_TF_SIZE][R848_TF_LOWEST_NUM] = 
{    {0x00, 0x06, 0x0C, 0x15, 0x1C, 0x1F, 0x3C, 0x3F},
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08},
	  {0x02, 0x06, 0x0C, 0x15, 0x1C, 0x1F, 0x3C, 0x3F},
	  {0x06, 0x11, 0x15, 0x1C, 0x1F, 0x2F, 0x3C, 0x3F},
      {0x04, 0x08, 0x08, 0x08, 0x10, 0x12, 0x13, 0x13},
	  {0x06, 0x09, 0x0E, 0x18, 0x25, 0x2F, 0x3C, 0x3F},
	  {0x00, 0x04, 0x04, 0x08, 0x08, 0x10, 0x12, 0x13},
	  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x08},
	  {0x0E, 0x14, 0x18, 0x1E, 0x25, 0x2F, 0x3C, 0x3F}
};


UINT8  R848_TF = 0;
//----------------------------------------------------------//
//                   Internal static struct                         //
//----------------------------------------------------------//
static R848_SysFreq_Info_Type  SysFreq_Info1;
static R848_Sys_Info_Type         Sys_Info1;
static R848_Freq_Info_Type       Freq_Info1;
//----------------------------------------------------------//
//                   Internal Functions                            //
//----------------------------------------------------------//
R848_ErrCode R848_TF_Check(void);
R848_ErrCode R848_Xtal_Check(void);
R848_ErrCode R848_InitReg(R848_Standard_Type R848_Standard);
R848_ErrCode R848_Cal_Prepare(UINT8 u1CalFlag);
R848_ErrCode R848_IMR(UINT8 IMR_MEM, BOOL IM_Flag);
R848_ErrCode R848_PLL(UINT32 LO_Freq, R848_Standard_Type R848_Standard);
R848_ErrCode R848_MUX(UINT32 LO_KHz, UINT32 RF_KHz, R848_Standard_Type R848_Standard);
R848_ErrCode R848_IQ(R848_SectType* IQ_Pont);
R848_ErrCode R848_IQ_Tree(UINT8 FixPot, UINT8 FlucPot, UINT8 PotReg, R848_SectType* CompareTree);
R848_ErrCode R848_CompreCor(R848_SectType* CorArry);
R848_ErrCode R848_CompreStep(R848_SectType* StepArry, UINT8 Pace);
R848_ErrCode R848_Muti_Read(UINT8* IMR_Result_Data);
R848_ErrCode R848_Section(R848_SectType* SectionArry);
R848_ErrCode R848_F_IMR(R848_SectType* IQ_Pont);
R848_ErrCode R848_IMR_Cross(R848_SectType* IQ_Pont, UINT8* X_Direct);
R848_ErrCode R848_IMR_Iqcap(R848_SectType* IQ_Point);   
R848_ErrCode R848_SetTF(UINT32 u4FreqKHz, UINT8 u1TfType);
R848_ErrCode R848_SetStandard(R848_Standard_Type RT_Standard);
R848_ErrCode R848_SetFrequency(R848_Set_Info R848_INFO);
R848_ErrCode R848_DVBS_Setting(R848_Set_Info R848_INFO);

R848_Sys_Info_Type R848_Sys_Sel(R848_Standard_Type R848_Standard);
R848_Freq_Info_Type R848_Freq_Sel(UINT32 LO_freq, UINT32 RF_freq, R848_Standard_Type R848_Standard);
R848_SysFreq_Info_Type R848_SysFreq_Sel(R848_Standard_Type R848_Standard,UINT32 RF_freq);

UINT8 R848_Filt_Cal_ADC(UINT32 IF_Freq, UINT8 R848_BW, UINT8 FilCal_Gap);


R848_Sys_Info_Type R848_Sys_Sel(R848_Standard_Type R848_Standard)
{
	R848_Sys_Info_Type R848_Sys_Info;

	switch (R848_Standard)
	{
	case R848_MN_5100:   
		R848_Sys_Info.IF_KHz=5100;                    //IF
		R848_Sys_Info.BW=0x60;                          //BW=6M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=6050;          //CAL IF
		R848_Sys_Info.HPF_COR=0x0F;	             //R11[3:0]=15			 R848:R19[3:0]
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable	 R848:R19[4]
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;	 //R30[2]=0, ext normal		 R848:R38[2]
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0, lna=max-1	 R848:R38[1:0]
		break;

    case R848_MN_5800:   
		R848_Sys_Info.IF_KHz=5800;                    //IF
		R848_Sys_Info.BW=0x40;                          //BW=7M   R11[6:5]  R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=6800;          //CAL IF  (%)
		R848_Sys_Info.HPF_COR=0x0A;	             //R11[3:0]=10  (%)		 R848:R19[3:0]
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable	 R848:R19[4]
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal		 R848:R38[2]
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0, lna=max-1	 R848:R38[1:0]
		break;

	case R848_MN_CIF_5M:     
		R848_Sys_Info.IF_KHz=6750;                    //IF
		R848_Sys_Info.BW=0x40;                          //BW=7M
		R848_Sys_Info.FILT_CAL_IF=7750;          //CAL IF  
		R848_Sys_Info.HPF_COR=0x05;	             //R11[3:0]=5  (2M)
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0,  lna=max-1 & Buf 4, hpf+1
		break;

	case R848_PAL_I:         
		R848_Sys_Info.IF_KHz=7300;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M   R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8650;          //CAL IF  
		R848_Sys_Info.HPF_COR=0x0D;	             //R11[3:0]=13
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0, lna=max-1
		break;

	case R848_PAL_I_CIF_5M:       
		R848_Sys_Info.IF_KHz=7750;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M
		R848_Sys_Info.FILT_CAL_IF=9100;          //CAL IF  
		R848_Sys_Info.HPF_COR=0x09;	             //R11[3:0]=9 (1.15M)
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0,  lna=max-1 & Buf 4, hpf+1
		break;

	case R848_PAL_DK:
		R848_Sys_Info.IF_KHz=7300;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M   R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8500;          //CAL IF     
		R848_Sys_Info.HPF_COR=0x0D;	             //R11[3:0]=13
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0, lna=max-1
		break;

	case R848_PAL_DK_CIF_5M: 
		R848_Sys_Info.IF_KHz=7750;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M
		R848_Sys_Info.FILT_CAL_IF=8950;          //CAL IF     
		R848_Sys_Info.HPF_COR=0x09;	             //R11[3:0]=9 (1.15M)
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0,  lna=max-1 & Buf 4, hpf+1
		break;

	case R848_PAL_B_7M:  
		R848_Sys_Info.IF_KHz=6600;                     //IF
		R848_Sys_Info.BW=0x40;                           //BW=7M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7800;           //CAL IF
		//R848_Sys_Info.HPF_COR=0x0D;	             //R11[3:0]=13
		R848_Sys_Info.HPF_COR=0x0A;	             //R11[3:0]=10 (0.9M)
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0, lna=max-1
		break;

	case R848_PAL_B_7M_CIF_5M:  
		R848_Sys_Info.IF_KHz=7250;                     //IF
		R848_Sys_Info.BW=0x00;                           //BW=8M
		R848_Sys_Info.FILT_CAL_IF=8450;           //CAL IF
		R848_Sys_Info.HPF_COR=0x08;	             //R11[3:0]=8 (1.45M)
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0,  lna=max-1 & Buf 4, hpf+1
		break;

    case R848_PAL_BGH_8M:  
		R848_Sys_Info.IF_KHz=6600;                    //IF
		R848_Sys_Info.BW=0x40;                          //BW=7M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7800;          //CAL IF
		R848_Sys_Info.HPF_COR=0x0D;	             //R11[3:0]=13
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0, lna=max-1
		break;

	case R848_PAL_BGH_8M_CIF_5M:  
		R848_Sys_Info.IF_KHz=7750;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M
		R848_Sys_Info.FILT_CAL_IF=8950;          //CAL IF
		R848_Sys_Info.HPF_COR=0x09;	             //R11[3:0]=9 (1.15M)
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0,  lna=max-1 & Buf 4, hpf+1
		break;

	case R848_SECAM_L: 
		R848_Sys_Info.IF_KHz=7300;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8550;          //CAL IF
		R848_Sys_Info.HPF_COR=0x0D;	             //R11[3:0]=13
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0, lna=max-1
		break;

	case R848_SECAM_L_CIF_5M: 
		R848_Sys_Info.IF_KHz=7750;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M
		R848_Sys_Info.FILT_CAL_IF=8950;          //CAL IF
		R848_Sys_Info.HPF_COR=0x09;	             //R11[3:0]=9 (1.15M)
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0,  lna=max-1 & Buf 4, hpf+1
		break;

    case R848_SECAM_L1:   
        R848_Sys_Info.IF_KHz=1320;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M  R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8050;          //CAL IF
		R848_Sys_Info.HPF_COR=0x0F;	             //R11[3:0]=13
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0, lna=max-1
		break;

	case R848_SECAM_L1_CIF_5M:   
        R848_Sys_Info.IF_KHz=2250;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M
		R848_Sys_Info.FILT_CAL_IF=8950;          //CAL IF
		R848_Sys_Info.HPF_COR=0x09;	             //R11[3:0]=9 (1.15M)
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0,  lna=max-1 & Buf 4, hpf+1
		break;

    case R848_SECAM_L1_INV: 
		R848_Sys_Info.IF_KHz=7300;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M   R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8550;          //CAL IF
		R848_Sys_Info.HPF_COR=0x0D;	             //R11[3:0]=13
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0, lna=max-1
		break;

	case R848_SECAM_L1_INV_CIF_5M: 
		R848_Sys_Info.IF_KHz=7750;                    //IF
		R848_Sys_Info.BW=0x00;                          //BW=8M
		R848_Sys_Info.FILT_CAL_IF=8950;          //CAL IF
		R848_Sys_Info.HPF_COR=0x09;	             //R11[3:0]=9 (1.15M)
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R11[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R30[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R30[1:0]=0,  lna=max-1 & Buf 4, hpf+1
		break;

	case R848_DVB_T_6M: 
	case R848_DVB_T2_6M: 
		R848_Sys_Info.IF_KHz=4570;                    //IF
		R848_Sys_Info.BW=0x40;                          //BW=7M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7450;          //CAL IF
		R848_Sys_Info.HPF_COR=0x05;	             //R19[3:0]=5
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x03;   //R38[1:0]=11, buf 8
		break;



	case R848_DVB_T_7M:  
	case R848_DVB_T2_7M:  
		R848_Sys_Info.IF_KHz=4570;                     //IF
		R848_Sys_Info.BW=0x40;                           //BW=7M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7750;           //CAL IF
		R848_Sys_Info.HPF_COR=0x08;	             //R19[3:0]=8  (1.45M)
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x03;   //R38[1:0]=11, buf 8
		break;

	case R848_DVB_T_8M: 
	case R848_DVB_T2_8M: 
		R848_Sys_Info.IF_KHz=4570;                     //IF
		R848_Sys_Info.BW=0x00;                           //BW=8M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8130;           //CAL IF
		R848_Sys_Info.HPF_COR=0x09;	             //R19[3:0]=9
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x03;   //R38[1:0]=11, buf 8
		break;

	case R848_DVB_T2_1_7M: 
		R848_Sys_Info.IF_KHz=1900;
		R848_Sys_Info.BW=0x40;                           //BW=7M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7800;           //CAL IF
		R848_Sys_Info.HPF_COR=0x08;	             //R19[3:0]=8
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	case R848_DVB_T2_10M: 
		R848_Sys_Info.IF_KHz=5600;
		R848_Sys_Info.BW=0x00;                           //BW=8M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=10800;         //CAL IF
		R848_Sys_Info.HPF_COR=0x0C;	             //R19[3:0]=12
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	case R848_DVB_C_8M:  
#if (FOR_TDA10024==1)
		R848_Sys_Info.IF_KHz=5070;
		R848_Sys_Info.BW=0x00;                           //BW=8M   R11[6:5]
		R848_Sys_Info.FILT_CAL_IF=9200;           //CAL IF //8750
		R848_Sys_Info.HPF_COR=0x0A;	             //R19[3:0]=10
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=1, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x02;   //R38[1:0]=10, lna=max-1 & Buf 4
#else
		R848_Sys_Info.IF_KHz=5070;
		R848_Sys_Info.BW=0x00;                           //BW=8M   R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=9000;           //CAL IF //9150
		R848_Sys_Info.HPF_COR=0x0A;	             //R19[3:0]=10
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x02;   //R38[1:0]=10, lna=max-1 & Buf 4
#endif
		break;

	case R848_DVB_C_6M:  
		R848_Sys_Info.IF_KHz=5070;
		R848_Sys_Info.BW=0x40;                          //BW=7M   R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8025;          //CAL IF   
		R848_Sys_Info.HPF_COR=0x03;	             //R19[3:0]=3 //3
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	case R848_J83B:  
		R848_Sys_Info.IF_KHz=5070;
		R848_Sys_Info.BW=0x40;                          //BW=7M   R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8025;          //CAL IF  
		R848_Sys_Info.HPF_COR=0x03;	             //R19[3:0]=3 //3
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	case R848_ISDB_T: 
		R848_Sys_Info.IF_KHz=4063;
		R848_Sys_Info.BW=0x40;                          //BW=7M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7000;          //CAL IF  //7200
		R848_Sys_Info.HPF_COR=0x08;	             //R19[3:0]=8
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x03;   //R38[1:0]=11, buf 8
		break;
	case R848_ISDB_T_4570:
		R848_Sys_Info.IF_KHz=4570;                    //IF
		R848_Sys_Info.BW=0x40;                          //BW=7M
		R848_Sys_Info.FILT_CAL_IF=7250;          //CAL IF
		R848_Sys_Info.HPF_COR=0x05;	             //R19[3:0]=5 (2.0M)
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x03;   //R38[1:0]=11, buf 8, hpf+3
		break;

	case R848_DTMB_4570: 
		R848_Sys_Info.IF_KHz=4570;
		R848_Sys_Info.BW=0x00;                           //BW=8M   R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8330;           //CAL IF  //8130
		R848_Sys_Info.HPF_COR=0x0B;	             //R19[3:0]=11
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

    case R848_DTMB_6000: 
		R848_Sys_Info.IF_KHz=6000;
		R848_Sys_Info.BW=0x00;                           //BW=8M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=9550;           //CAL IF
		R848_Sys_Info.HPF_COR=0x03;	             //R19[3:0]=3
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x02;   //R38[1:0]=10, lna=max-1 & Buf 4
		break;

	case R848_DTMB_6M_BW_IF_5M:
		R848_Sys_Info.IF_KHz=5000;
		R848_Sys_Info.BW=0x40;                           //BW=7M
		R848_Sys_Info.FILT_CAL_IF=7700;           //CAL IF  
		R848_Sys_Info.HPF_COR=0x04;	             //R19[3:0]=4
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1 & Buf 4, hpf+1
		break;

	case R848_DTMB_6M_BW_IF_4500:
		R848_Sys_Info.IF_KHz=4500;
		R848_Sys_Info.BW=0x40;                           //BW=7M
		R848_Sys_Info.FILT_CAL_IF=7000;           //CAL IF  
		R848_Sys_Info.HPF_COR=0x05;	             //R19[3:0]=5
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x02;   //R38[1:0]=10, lna=max-1 & Buf 4, hpf+3
		break;
	
	case R848_ATSC:  
		R848_Sys_Info.IF_KHz=5070;
		R848_Sys_Info.BW=0x40;                          //BW=7M      R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7900;          //CAL IF     20130621 Ryan Modify
		R848_Sys_Info.HPF_COR=0x04;	             //R19[3:0]=4 
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

    case R848_DVB_T_6M_IF_5M: 
	case R848_DVB_T2_6M_IF_5M: 
		R848_Sys_Info.IF_KHz=5000;                    //IF
		R848_Sys_Info.BW=0x40;                          //BW=7M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7800;          //CAL IF
		R848_Sys_Info.HPF_COR=0x04;	             //R19[3:0]=4
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x03;   //R38[1:0]=11, buf 8
		break;

	case R848_DVB_T_7M_IF_5M:  
	case R848_DVB_T2_7M_IF_5M:  
		R848_Sys_Info.IF_KHz=5000;                     //IF
		R848_Sys_Info.BW=0x00;                           //BW=8M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8300;           //CAL IF
		R848_Sys_Info.HPF_COR=0x07;	             //R19[3:0]=7
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x03;   //R38[1:0]=11, buf 8
		break;

	case R848_DVB_T_8M_IF_5M: 
	case R848_DVB_T2_8M_IF_5M: 
		R848_Sys_Info.IF_KHz=5000;                     //IF
		R848_Sys_Info.BW=0x00;                           //BW=8M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8500;           //CAL IF
		R848_Sys_Info.HPF_COR=0x08;	             //R19[3:0]=8
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x03;   //R38[1:0]=11, buf 8
		break;

	case R848_DVB_T2_1_7M_IF_5M: 
		R848_Sys_Info.IF_KHz=5000;
		R848_Sys_Info.BW=0x60;                           //BW=6M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=5900;           //CAL IF
		R848_Sys_Info.HPF_COR=0x01;	             //R19[3:0]=1
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	case R848_DVB_C_8M_IF_5M:  
//	case R848_DVB_C_CHINA_IF_5M:   //RF>115MHz
		R848_Sys_Info.IF_KHz=5000;
		R848_Sys_Info.BW=0x00;                           //BW=8M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=9000;           //CAL IF 
		R848_Sys_Info.HPF_COR=0x0A;	             //R19[3:0]=10
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x02;   //R38[1:0]=10, lna=max-1 & Buf 4
		break;

	case R848_DVB_C_6M_IF_5M:  
		R848_Sys_Info.IF_KHz=5000;
		R848_Sys_Info.BW=0x40;                          //BW=7M     R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8100;          //CAL IF   
		R848_Sys_Info.HPF_COR=0x04;	             //R19[3:0]=4 
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	case R848_J83B_IF_5M:  
		R848_Sys_Info.IF_KHz=5000;
		R848_Sys_Info.BW=0x40;                          //BW=7M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8025;          //CAL IF  
		R848_Sys_Info.HPF_COR=0x03;	             //R19[3:0]=3 
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	case R848_ISDB_T_IF_5M:
		R848_Sys_Info.IF_KHz=5000;  
		R848_Sys_Info.BW=0x40;                          //BW=7M      R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7500;          //CAL IF  
		R848_Sys_Info.HPF_COR=0x04;	             //R19[3:0]=4
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x03;   //R38[1:0]=11, buf 8
		break;

	case R848_DTMB_IF_5M: 
		R848_Sys_Info.IF_KHz=5000;
		R848_Sys_Info.BW=0x00;                           //BW=8M      R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8650;           //CAL IF  
		R848_Sys_Info.HPF_COR=0x09;	             //R19[3:0]=9
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x02;   //R38[1:0]=0, lna=max-1
		break;
	
	case R848_ATSC_IF_5M:  
		R848_Sys_Info.IF_KHz=5000;
		R848_Sys_Info.BW=0x40;                          //BW=7M      R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7900;          //CAL IF   
		R848_Sys_Info.HPF_COR=0x04;	             //R19[3:0]=4 
		R848_Sys_Info.FILT_EXT_ENA=0x00;      //R19[4]=0, ext disable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	case R848_FM:  
		R848_Sys_Info.IF_KHz=2400;
		R848_Sys_Info.BW=0x40;                           //BW=7M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=7200;           //CAL IF
		R848_Sys_Info.HPF_COR=0x02;	             //R19[3:0]=2
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	default:  //R848_DVB_T_8M
		R848_Sys_Info.IF_KHz=4570;                     //IF
		R848_Sys_Info.BW=0x00;                           //BW=8M    R848:R19[6:5]
		R848_Sys_Info.FILT_CAL_IF=8330;           //CAL IF
		R848_Sys_Info.HPF_COR=0x0B;	             //R19[3:0]=11
		R848_Sys_Info.FILT_EXT_ENA=0x10;      //R19[4]=1, ext enable
		R848_Sys_Info.FILT_EXT_WIDEST=0x00;//R38[2]=0, ext normal
		R848_Sys_Info.FILT_EXT_POINT=0x00;   //R38[1:0]=0, lna=max-1
		break;

	}


	//Set By DTV/ATV
	if (R848_Standard < R848_ATV_SIZE)  //ATV
	{
		//R848_Sys_Info.INDUC_BIAS = 0x01;      //normal   (R4[0]=1)
		R848_Sys_Info.SWCAP_CLK = 0x02;      //8k      	// R848:R26[1:0]
		R848_Sys_Info.NA_PWR_DET = 0x80;    //off         //R848:R38[7]
	}
	else
	{
		//R848_Sys_Info.INDUC_BIAS = 0x01;     //normal      
		//R848_Sys_Info.SWCAP_CLK = 0x01;     //32k       
		R848_Sys_Info.SWCAP_CLK = 0x02;      //8k        //AGC 500Hz map to 8k	R26[1:0]
		R848_Sys_Info.NA_PWR_DET = 0x00;   //on         						R38[7]
	}


	#if(FOR_KOREA_CTMR==TRUE)
		if (R848_Standard < R848_ATV_SIZE)  //ATV
		{
			R848_Sys_Info.SWBUF_CUR = 0x00;     //high  ( R12[2]=0)
			R848_Sys_Info.FILT_CUR = 0x20;         //high  (R18[6:5]=2'b01)
		}
		else
		{
			R848_Sys_Info.SWBUF_CUR = 0x04;     //low  ( R12[2]=1)
			R848_Sys_Info.FILT_CUR = 0x00;         //highest  (R18[6:5]=2'b00)
		}
	#else
		R848_Sys_Info.SWBUF_CUR = 0x04;          //low  ( R12[2]=1)
		R848_Sys_Info.FILT_CUR = 0x00;         //highest  (R18[6:5]=2'b00)
	#endif




	R848_Sys_Info.TF_CUR = 0x40;                  //low  R11[6]=1
	R848_Sys_Info.FILT_COMP = 0x20;      //1       R38[6:5]        


	//Filter 3dB
	switch(R848_Standard)
	{
		case R848_DVB_C_8M_IF_5M:
			R848_Sys_Info.FILT_3DB = 0x08;         // ON      R38[3]
		break;
		default: 
		    R848_Sys_Info.FILT_3DB = 0x00;         // OFF     R38[3]
		break;
	}



	//BW 1.7M
	if((R848_Standard==R848_DVB_T2_1_7M) || (R848_Standard==R848_FM))
		R848_Sys_Info.V17M = 0x80;       //enable, 	R19[7]
	else
	    R848_Sys_Info.V17M = 0x00;       //disable, (include DVBT2 1.7M IF=5MHz) R19[7]

	//TF Type select
	switch(R848_Standard)
	{
		case R848_MN_5100:	
		case R848_MN_5800:	
		case R848_PAL_I:
		case R848_PAL_DK:
		case R848_PAL_B_7M:
		case R848_PAL_BGH_8M:
		case R848_SECAM_L:
		case R848_SECAM_L1:
		case R848_SECAM_L1_INV:
		case R848_MN_CIF_5M:	
		case R848_PAL_I_CIF_5M:
		case R848_PAL_DK_CIF_5M:
		case R848_PAL_B_7M_CIF_5M:
		case R848_PAL_BGH_8M_CIF_5M:
		case R848_SECAM_L_CIF_5M:
		case R848_SECAM_L1_CIF_5M:
		case R848_SECAM_L1_INV_CIF_5M:
                if(R848_DetectTfType == R848_UL_USING_BEAD )
				{
					R848_SetTfType = R848_TF_82N_BEAD;   
				}
				else
				{
					R848_SetTfType = R848_TF_82N_270N;   
				}
			 break;

		case R848_DTMB_4570:
		case R848_DTMB_6000:
		case R848_DTMB_6M_BW_IF_5M:
		case R848_DTMB_6M_BW_IF_4500:
		case R848_DTMB_IF_5M:
				if(R848_DetectTfType == R848_UL_USING_BEAD )
				{
					R848_SetTfType = R848_TF_82N_BEAD;   
				}
				else
				{
					R848_SetTfType = R848_TF_82N_270N;   
				}
			 break;	

		default:		
				if(R848_DetectTfType == R848_UL_USING_BEAD)
				{
					R848_SetTfType = R848_TF_82N_BEAD;   
				}
				else
				{
					R848_SetTfType = R848_TF_82N_270N;   
				}	
			break;
	}

	return R848_Sys_Info;
}


R848_Freq_Info_Type R848_Freq_Sel(UINT32 LO_freq, UINT32 RF_freq, R848_Standard_Type R848_Standard)
{
	R848_Freq_Info_Type R848_Freq_Info;

	//----- RF dependent parameter --------????
	//LNA band  R13[6:5]
	if((RF_freq>=0) && (RF_freq<R848_LNA_LOW_LOWEST[R848_SetTfType]))  //<164
		 R848_Freq_Info.LNA_BAND = 0x60;   // ultra low			// R848:R13[6:5]
	else if((RF_freq>=R848_LNA_LOW_LOWEST[R848_SetTfType]) && (RF_freq<R848_LNA_MID_LOW[R848_SetTfType]))  //164~388
		 R848_Freq_Info.LNA_BAND = 0x40;   //low					// R848:R13[6:5]
	else if((RF_freq>=R848_LNA_MID_LOW[R848_SetTfType]) && (RF_freq<R848_LNA_HIGH_MID[R848_SetTfType]))  //388~612
		 R848_Freq_Info.LNA_BAND = 0x20;   // mid					// R848:R13[6:5]
	else     // >612
		 R848_Freq_Info.LNA_BAND = 0x00;   // high				// R848:R13[6:5]
	
	//----- LO dependent parameter --------
	//IMR point 
	if((LO_freq>=0) && (LO_freq<133000))  
         R848_Freq_Info.IMR_MEM = 0;   
	else if((LO_freq>=133000) && (LO_freq<221000))  
         R848_Freq_Info.IMR_MEM = 1;   
	else if((LO_freq>=221000) && (LO_freq<450000))  
		 R848_Freq_Info.IMR_MEM = 2;  
	else if((LO_freq>=450000) && (LO_freq<775000))  
		 R848_Freq_Info.IMR_MEM = 3; 
	else 
		 R848_Freq_Info.IMR_MEM = 4; 

	//RF polyfilter band   R33[7:6]
	if((LO_freq>=0) && (LO_freq<133000))  
         R848_Freq_Info.RF_POLY = 0x80;   //low	
	else if((LO_freq>=133000) && (LO_freq<221000))  
         R848_Freq_Info.RF_POLY = 0x40;   // mid
	else if((LO_freq>=221000) && (LO_freq<775000))  
		 R848_Freq_Info.RF_POLY = 0x00;   // highest
	else
		 R848_Freq_Info.RF_POLY = 0xC0;   // ultra high

	
	//LPF Cap, Notch
	switch(R848_Standard)
	{
		case R848_MN_5100:	
		case R848_MN_5800:	
		case R848_PAL_I:
		case R848_PAL_DK:
		case R848_PAL_B_7M:
		case R848_PAL_BGH_8M:
		case R848_SECAM_L:
		case R848_SECAM_L1:
		case R848_SECAM_L1_INV:
		case R848_MN_CIF_5M:	
		case R848_PAL_I_CIF_5M:
		case R848_PAL_DK_CIF_5M:
		case R848_PAL_B_7M_CIF_5M:
		case R848_PAL_BGH_8M_CIF_5M:
		case R848_SECAM_L_CIF_5M:
		case R848_SECAM_L1_CIF_5M:
		case R848_SECAM_L1_INV_CIF_5M:
		case R848_DVB_C_8M:
		case R848_DVB_C_6M:
		case R848_J83B:
        case R848_DVB_C_8M_IF_5M:
		case R848_DVB_C_6M_IF_5M:
		case R848_J83B_IF_5M:
			if((LO_freq>=0) && (LO_freq<77000))  
			{
				R848_Freq_Info.LPF_CAP = 16;
				R848_Freq_Info.LPF_NOTCH = 10;
			}
			else if((LO_freq>=77000) && (LO_freq<85000))
			{
				R848_Freq_Info.LPF_CAP = 16;
				R848_Freq_Info.LPF_NOTCH = 4;
			}
			else if((LO_freq>=85000) && (LO_freq<115000))
			{
				R848_Freq_Info.LPF_CAP = 13;
				R848_Freq_Info.LPF_NOTCH = 3;
			}
			else if((LO_freq>=115000) && (LO_freq<125000))
			{
				R848_Freq_Info.LPF_CAP = 11;
				R848_Freq_Info.LPF_NOTCH = 1;
			}
			else if((LO_freq>=125000) && (LO_freq<141000))
			{
				R848_Freq_Info.LPF_CAP = 9;
				R848_Freq_Info.LPF_NOTCH = 0;
			}
			else if((LO_freq>=141000) && (LO_freq<157000))
			{
				R848_Freq_Info.LPF_CAP = 8;
				R848_Freq_Info.LPF_NOTCH = 0;
			}
			else if((LO_freq>=157000) && (LO_freq<181000))
			{
				R848_Freq_Info.LPF_CAP = 6;
				R848_Freq_Info.LPF_NOTCH = 0;
			}
			else if((LO_freq>=181000) && (LO_freq<205000))
			{
				R848_Freq_Info.LPF_CAP = 3;
				R848_Freq_Info.LPF_NOTCH = 0;
			}
			else //LO>=201M
			{
				R848_Freq_Info.LPF_CAP = 0;
				R848_Freq_Info.LPF_NOTCH = 0;
			}

			break;

		default:
			if((LO_freq>=0) && (LO_freq<73000))  
			{
				R848_Freq_Info.LPF_CAP = 8;
				R848_Freq_Info.LPF_NOTCH = 10;
			}
			else if((LO_freq>=73000) && (LO_freq<81000))
			{
				R848_Freq_Info.LPF_CAP = 8;
				R848_Freq_Info.LPF_NOTCH = 4;
			}
			else if((LO_freq>=81000) && (LO_freq<89000))
			{
				R848_Freq_Info.LPF_CAP = 8;
				R848_Freq_Info.LPF_NOTCH = 3;
			}
			else if((LO_freq>=89000) && (LO_freq<121000))
			{
				R848_Freq_Info.LPF_CAP = 6;
				R848_Freq_Info.LPF_NOTCH = 1;
			}
			else if((LO_freq>=121000) && (LO_freq<145000))
			{
				R848_Freq_Info.LPF_CAP = 4;
				R848_Freq_Info.LPF_NOTCH = 0;
			}
			else if((LO_freq>=145000) && (LO_freq<153000))
			{
				R848_Freq_Info.LPF_CAP = 3;
				R848_Freq_Info.LPF_NOTCH = 0;
			}
			else if((LO_freq>=153000) && (LO_freq<177000))
			{
				R848_Freq_Info.LPF_CAP = 2;
				R848_Freq_Info.LPF_NOTCH = 0;
			}
			else if((LO_freq>=177000) && (LO_freq<201000))
			{
				R848_Freq_Info.LPF_CAP = 1;
				R848_Freq_Info.LPF_NOTCH = 0;
			}
			else //LO>=201M
			{
				R848_Freq_Info.LPF_CAP = 0;
				R848_Freq_Info.LPF_NOTCH = 0;
			}

			break;

	}//end switch(standard)

	return R848_Freq_Info;

}



R848_SysFreq_Info_Type R848_SysFreq_Sel(R848_Standard_Type R848_Standard,UINT32 RF_freq)
{
	R848_SysFreq_Info_Type R848_SysFreq_Info;

	switch(R848_Standard)
	{
	case R848_MN_5100:	
	case R848_MN_5800:	
	case R848_PAL_I:
	case R848_PAL_DK:
	case R848_PAL_B_7M:
	case R848_PAL_BGH_8M:
	case R848_SECAM_L:
    case R848_SECAM_L1:
    case R848_SECAM_L1_INV:
	case R848_MN_CIF_5M:	                      //ATV CIF=5M
	case R848_PAL_I_CIF_5M:
	case R848_PAL_DK_CIF_5M:
	case R848_PAL_B_7M_CIF_5M:
	case R848_PAL_BGH_8M_CIF_5M:
	case R848_SECAM_L_CIF_5M:
    case R848_SECAM_L1_CIF_5M:
    case R848_SECAM_L1_INV_CIF_5M:

			if(RF_freq<165000)
			{
				R848_SysFreq_Info.LNA_TOP=0x04;		       // LNA TOP=5(0x02) =>3(0x04)                 //  R848:R35[2:0]
			}
			else if((RF_freq>=165000) && (RF_freq<345000))
			{
				R848_SysFreq_Info.LNA_TOP=0x05;		       // LNA TOP=4(0x03) =>2 (0x05)                  // R848:R35[2:0]
			}
			else
			{
				R848_SysFreq_Info.LNA_TOP=0x05;		       // LNA TOP=2                     R848:R35[2:0]
			}
				R848_SysFreq_Info.LNA_VTH_L=0xA3;		   // LNA VTH/L=1.34/0.64          (R13=0xA3)		 //  R848:R31[7:0]
				R848_SysFreq_Info.MIXER_TOP=0x05;	       // MIXER TOP=3(0x0C)=>10(0x05)                  (R28[3:0]=4'b1100)// R848:R36[3:0]
				R848_SysFreq_Info.MIXER_VTH_L=0xA5;        // MIXER VTH/L=1.34/0.84        (R14=0xA5)		 // R848:R32[7:0]
				R848_SysFreq_Info.RF_TOP=0xA0;              // RF TOP=2                    (R26[7:5]=3'b101) // R848:R34[7:5]
				R848_SysFreq_Info.NRB_TOP=0x30;            // Nrb TOP=12                   (R28[7:4]=4'b0011)// R848:R36[7:4]
				R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                (R27[7:6]=2'b11)  // R848:R35[7:6]
#if (FOR_KOREA_CTMR==1)	
			if(RF_freq>700000)
		    {
	            R848_SysFreq_Info.LNA_TOP=0x06;		       // LNA TOP=1                    (R27[2:0]=3'b110)
				R848_SysFreq_Info.LNA_VTH_L=0x71;	       // LNA VTH/L=1.04/0.44     (R13=0x71)
				R848_SysFreq_Info.RF_TOP=0x80;              // RF TOP=3                        (R26[7:5]=3'b100)
			}
#endif			
		break;

	case R848_DVB_T_6M:
	case R848_DVB_T_7M:
	case R848_DVB_T_8M:
	case R848_DVB_T_6M_IF_5M:
	case R848_DVB_T_7M_IF_5M:
	case R848_DVB_T_8M_IF_5M:
	case R848_DVB_T2_6M:
	case R848_DVB_T2_7M: 
	case R848_DVB_T2_8M:
	case R848_DVB_T2_1_7M:
	case R848_DVB_T2_10M:
    case R848_DVB_T2_6M_IF_5M:
	case R848_DVB_T2_7M_IF_5M:
	case R848_DVB_T2_8M_IF_5M:
	case R848_DVB_T2_1_7M_IF_5M:
		if((RF_freq>=300000)&&(RF_freq<=472000))
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA4;	   // LNA VTH/L=1.34/0.74     (R31=0xA4)
			R848_SysFreq_Info.MIXER_TOP=0x05;	       // MIXER TOP=10        (R36[3:0]=4'b0101)
			R848_SysFreq_Info.MIXER_VTH_L=0x95;   // MIXER VTH/L=1.24/0.84  (R32=0x95)
			R848_SysFreq_Info.NRB_TOP=0xC0;            // Nrb TOP=3                       (R36[7:4]=4'b1100)

		}
		else if((RF_freq>=184000) && (RF_freq<=299000))
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA5;	   // LNA VTH/L=1.34/0.84     (R31=0xA5)
			R848_SysFreq_Info.MIXER_TOP=0x04;	       // MIXER TOP=11        (R36[3:0]=4'b0101)
			R848_SysFreq_Info.MIXER_VTH_L=0xA6;   // MIXER VTH/L=1.34/0.94  (R32=0xA6)
			R848_SysFreq_Info.NRB_TOP=0x70;            // Nrb TOP=8                       (R36[7:4]=4'b1100)
		}
		else
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA5;	   // LNA VTH/L=1.34/0.84     (R31=0xA5)
			R848_SysFreq_Info.MIXER_TOP=0x05;	       // MIXER TOP=10        (R36[3:0]=4'b0101)
			R848_SysFreq_Info.MIXER_VTH_L=0x95;   // MIXER VTH/L=1.24/0.84  (R32=0x95)
			R848_SysFreq_Info.NRB_TOP=0xC0;            // Nrb TOP=3                       (R36[7:4]=4'b1100)
		}
		    R848_SysFreq_Info.LNA_TOP=0x03;		       // LNA TOP=4           (R35[2:0]=3'b011)
			R848_SysFreq_Info.RF_TOP=0x40;               // RF TOP=5                        (R34[7:5]=3'b010)                 
			R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)	
		break;

    case R848_DVB_C_8M:
	case R848_DVB_C_6M:	
	case R848_J83B:
	case R848_DVB_C_8M_IF_5M:
	case R848_DVB_C_6M_IF_5M:	
	case R848_J83B_IF_5M:
		if(RF_freq<165000)
		{
			R848_SysFreq_Info.LNA_TOP=0x02;		       // LNA TOP=5                    (R35[2:0]=3'b010)
			R848_SysFreq_Info.LNA_VTH_L=0x94;	   // LNA VTH/L=1.24/0.74     (R31=0x94)
			R848_SysFreq_Info.RF_TOP=0x80;               // RF TOP=3                        (R34[7:5]=3'b100)
			R848_SysFreq_Info.NRB_TOP=0x90;            // Nrb TOP=6                       (R36[7:4]=4'b1001)   //R848:R36[7:4]
		}
		else if((RF_freq>=165000) && (RF_freq<=299000))
		{
			R848_SysFreq_Info.LNA_TOP=0x03;		       // LNA TOP=4                    (R35[2:0]=3'b011)
			R848_SysFreq_Info.LNA_VTH_L=0x94;	   // LNA VTH/L=1.24/0.74     (R31=0x94)
			R848_SysFreq_Info.RF_TOP=0x80;               // RF TOP=3                        (R34[7:5]=3'b100)
			R848_SysFreq_Info.NRB_TOP=0x90;            // Nrb TOP=6                       (R36[7:4]=4'b1001)
		}
		else if((RF_freq>299000) && (RF_freq<=345000))
		{
			R848_SysFreq_Info.LNA_TOP=0x03;		        // LNA TOP=4                    (R35[2:0]=3'b011)
			R848_SysFreq_Info.LNA_VTH_L=0xA4;			// LNA VTH/L=1.34/0.74     (R31=0xA4)
			R848_SysFreq_Info.RF_TOP=0x80;              // RF TOP=3                        (R34[7:5]=3'b100)
			R848_SysFreq_Info.NRB_TOP=0x90;             // Nrb TOP=6                       (R36[7:4]=4'b1001)
		}
		else if((RF_freq>345000) && (RF_freq<=472000))
		{
			R848_SysFreq_Info.LNA_TOP=0x04;		        // LNA TOP=3                    (R35[2:0]=3'b100)
			R848_SysFreq_Info.LNA_VTH_L=0x93;		    // LNA VTH/L=1.24/0.64     (R31=0x93)
			R848_SysFreq_Info.RF_TOP=0xA0;				// RF TOP=2                        (R34[7:5]=3'b101)
			R848_SysFreq_Info.NRB_TOP=0x20;             // Nrb TOP=13                       (R36[7:4]=4'b0010)
		}
		else
		{
			R848_SysFreq_Info.LNA_TOP=0x04;		        // LNA TOP=3                    (R35[2:0]=3'b100)
			R848_SysFreq_Info.LNA_VTH_L=0x83;		    // LNA VTH/L=1.14/0.64     (R31=0x83)
			R848_SysFreq_Info.RF_TOP=0xA0;              // RF TOP=2                        (R34[7:5]=3'b101)
			R848_SysFreq_Info.NRB_TOP=0x20;             // Nrb TOP=13                       (R36[7:4]=4'b0010)
		}			
			R848_SysFreq_Info.MIXER_TOP=0x05;	        // MIXER TOP=10               (R36[3:0]=4'b0100)
			R848_SysFreq_Info.MIXER_VTH_L=0x95;			// MIXER VTH/L=1.24/0.84  (R32=0x95)
			R848_SysFreq_Info.NRB_BW=0xC0;              // Nrb BW=lowest                  (R35[7:6]=2'b11)                                   
		break;

	case R848_ISDB_T:	
	case R848_ISDB_T_4570:
	case R848_ISDB_T_IF_5M:	
		if((RF_freq>=300000)&&(RF_freq<=472000))
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA4;	   // LNA VTH/L=1.34/0.74     (R31=0xA4)
		}
		else
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA5;	   // LNA VTH/L=1.34/0.84     (R31=0xA5)
		}
			R848_SysFreq_Info.LNA_TOP=0x03;		       // LNA TOP=4                    (R35[2:0]=3'b011)
			R848_SysFreq_Info.MIXER_TOP=0x05;	       // MIXER TOP=10               (R36[3:0]=4'b0101)
			R848_SysFreq_Info.MIXER_VTH_L=0x95;   // MIXER VTH/L=1.24/0.84  (R32=0x95)
			R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			//R848_SysFreq_Info.NRB_TOP=0x20;            // Nrb TOP=13                       (R36[7:4]=4'b0010)
			R848_SysFreq_Info.NRB_TOP=0xB0;            // Nrb TOP=4                       (R36[7:4]=4'b1011)
			R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11) 
                                
		break;

	case R848_DTMB_4570:
	case R848_DTMB_6000:
	case R848_DTMB_6M_BW_IF_5M:
	case R848_DTMB_6M_BW_IF_4500:
	case R848_DTMB_IF_5M:
		if((RF_freq>=300000)&&(RF_freq<=472000))
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA4;	   // LNA VTH/L=1.34/0.74     (R31=0xA4)
		}
		else
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA5;	   // LNA VTH/L=1.34/0.84     (R31=0xA5)
		}
			R848_SysFreq_Info.LNA_TOP=0x03;		       // LNA TOP=4                    (R35[2:0]=3'b011)
			R848_SysFreq_Info.MIXER_TOP=0x05;	       // MIXER TOP=10               (R36[3:0]=4'b0100)
			R848_SysFreq_Info.MIXER_VTH_L=0x95;   // MIXER VTH/L=1.24/0.84  (R32=0x95)
			R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			R848_SysFreq_Info.NRB_TOP=0xA0;            // Nrb TOP=5                       (R36[7:4]=4'b1010)
			R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)         
		break;

	case R848_ATSC:
	case R848_ATSC_IF_5M: 
       if(R848_DetectTfType==R848_UL_USING_BEAD)
	   {
		   if(RF_freq<88000)
		   {
		       R848_SysFreq_Info.LNA_TOP=0x03;	 	       // LNA TOP=4                    (R35[2:0]=3'b011)
			   R848_SysFreq_Info.LNA_VTH_L=0xA5;	       // LNA VTH/L=1.34/0.84     (R31=0xA5)
			   R848_SysFreq_Info.RF_TOP=0xC0;               // RF TOP=1                        (R34[7:5]=3'b110)
			   R848_SysFreq_Info.NRB_TOP=0x30;             // Nrb TOP=12                    (R36[7:4]=4'b0011)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
		   else if((RF_freq>=88000) && (RF_freq<104000))
		   {
			   R848_SysFreq_Info.LNA_TOP=0x02;		       // LNA TOP=5                    (R35[2:0]=3'b010)
			   R848_SysFreq_Info.LNA_VTH_L=0xA5;	       // LNA VTH/L=1.34/0.84     (R31=0xA5)		
			   R848_SysFreq_Info.RF_TOP=0xA0;               // RF TOP=2                        (R34[7:5]=3'b101)
			   R848_SysFreq_Info.NRB_TOP=0x30;             // Nrb TOP=12                    (R36[7:4]=4'b0011)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
		   else if((RF_freq>=104000) && (RF_freq<156000))
		   {
			   R848_SysFreq_Info.LNA_TOP=0x01;		       // LNA TOP=6                    (R35[2:0]=3'b001)
			   R848_SysFreq_Info.LNA_VTH_L=0xA5;	       // LNA VTH/L=1.34/0.84     (R31=0xA5)		
			   R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			   R848_SysFreq_Info.NRB_TOP=0x30;             // Nrb TOP=12                    (R36[7:4]=4'b0011)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
		   else if((RF_freq>=156000) && (RF_freq<464000))
		   {
			   R848_SysFreq_Info.LNA_TOP=0x01;		       // LNA TOP=6                    (R35[2:0]=3'b001)
			   R848_SysFreq_Info.LNA_VTH_L=0xA5;	       // LNA VTH/L=1.34/0.84     (R31=0xA5)		
			   R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			   R848_SysFreq_Info.NRB_TOP=0x90;             // Nrb TOP=6                      (R36[7:4]=4'b1001)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
		   else if((RF_freq>=464000) && (RF_freq<500000))
		   {
			   R848_SysFreq_Info.LNA_TOP=0x01;		       // LNA TOP=6                    (R35[2:0]=3'b001)
			   R848_SysFreq_Info.LNA_VTH_L=0xB6;	       // LNA VTH/L=1.44/0.94     (R31=0xB6)		
			   R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			   R848_SysFreq_Info.NRB_TOP=0x90;             // Nrb TOP=6                      (R36[7:4]=4'b1001)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
		   else
		   {
				   R848_SysFreq_Info.LNA_TOP=0x01;		       // LNA TOP=6                    (R35[2:0]=3'b001)
				   R848_SysFreq_Info.LNA_VTH_L=0x94;	       // LNA VTH/L=1.24/0.74     (R31=0x94)		
				   R848_SysFreq_Info.RF_TOP=0x40;               // RF TOP=5                        (R34[7:5]=3'b010)
				   R848_SysFreq_Info.NRB_TOP=0x90;             // Nrb TOP=6                      (R36[7:4]=4'b1001)
				   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
	   }
	   else  //270n
	   {
		    if(RF_freq<88000)
		   {
		       R848_SysFreq_Info.LNA_TOP=0x02;	 	       // LNA TOP=5                    (R35[2:0]=3'b010)
			   R848_SysFreq_Info.LNA_VTH_L=0x94;	       // LNA VTH/L=1.24/0.74     (R31=0x94)
			   R848_SysFreq_Info.RF_TOP=0x80;               // RF TOP=3                        (R34[7:5]=3'b100)
			   R848_SysFreq_Info.NRB_TOP=0xC0;             // Nrb TOP=3                    (R36[7:4]=4'b1100)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
		   else if((RF_freq>=88000) && (RF_freq<104000))
		   {
			   R848_SysFreq_Info.LNA_TOP=0x02;		       // LNA TOP=5                    (R35[2:0]=3'b010)
			   R848_SysFreq_Info.LNA_VTH_L=0x94;	       // LNA VTH/L=1.24/0.74     (R31=0x94)
			   R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			   R848_SysFreq_Info.NRB_TOP=0x90;             // Nrb TOP=6                      (R36[7:4]=4'b1001)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
		   else if((RF_freq>=104000) && (RF_freq<248000))
		   {
			   R848_SysFreq_Info.LNA_TOP=0x01;		       // LNA TOP=6                    (R35[2:0]=3'b001)
			   R848_SysFreq_Info.LNA_VTH_L=0x94;	       // LNA VTH/L=1.24/0.74     (R31=0x94)	
			   R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			   R848_SysFreq_Info.NRB_TOP=0x90;             // Nrb TOP=6                      (R36[7:4]=4'b1001)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
		   else if((RF_freq>=248000) && (RF_freq<464000))
		   {
			   R848_SysFreq_Info.LNA_TOP=0x01;		       // LNA TOP=6                    (R35[2:0]=3'b001)
			   R848_SysFreq_Info.LNA_VTH_L=0xA5;	       // LNA VTH/L=1.34/0.84     (R31=0xA5)		
			   R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			   R848_SysFreq_Info.NRB_TOP=0x90;             // Nrb TOP=6                      (R36[7:4]=4'b1001)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
		   else if((RF_freq>=464000) && (RF_freq<500000))
		   {
			   R848_SysFreq_Info.LNA_TOP=0x01;		       // LNA TOP=6                    (R35[2:0]=3'b001)
			   R848_SysFreq_Info.LNA_VTH_L=0xB6;	       // LNA VTH/L=1.44/0.94     (R31=0xB6)		
			   R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			   R848_SysFreq_Info.NRB_TOP=0x90;             // Nrb TOP=6                      (R36[7:4]=4'b1001)
			   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }	
		   else
		   {
		       
				   R848_SysFreq_Info.LNA_TOP=0x01;		       // LNA TOP=6                    (R35[2:0]=3'b001)
				   R848_SysFreq_Info.LNA_VTH_L=0x94;	       // LNA VTH/L=1.24/0.74     (R31=0x94)		
				   R848_SysFreq_Info.RF_TOP=0x40;               // RF TOP=5                        (R34[7:5]=3'b010)
				   R848_SysFreq_Info.NRB_TOP=0x90;             // Nrb TOP=6                      (R36[7:4]=4'b1001)
				   R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)
		   }
	   
	   }
	        R848_SysFreq_Info.MIXER_TOP=0x04;	       // MIXER TOP=11               (R36[3:0]=4'b0100)
			R848_SysFreq_Info.MIXER_VTH_L=0xB7;   // MIXER VTH/L=1.44/1.04  (R32=0xB7)
			//R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)

		break;

	case R848_FM:
		if((RF_freq>=300000)&&(RF_freq<=472000))
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA4;	   // LNA VTH/L=1.34/0.74     (R31=0xA4)
		}
		else
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA5;	   // LNA VTH/L=1.34/0.84     (R31=0xA5)
		}
			R848_SysFreq_Info.LNA_TOP=0x03;		       // LNA TOP=4                    (R35[2:0]=3'b011)
			R848_SysFreq_Info.MIXER_TOP=0x05;	       // MIXER TOP=10               (R36[3:0]=4'b0100)
			R848_SysFreq_Info.MIXER_VTH_L=0x95;   // MIXER VTH/L=1.24/0.84  (R32=0x95)
			R848_SysFreq_Info.RF_TOP=0x60;               // RF TOP=4                        (R34[7:5]=3'b011)
			R848_SysFreq_Info.NRB_TOP=0x20;            // Nrb TOP=13                       (R36[7:4]=4'b0010)
			R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)                                
		break;

	default: //DVB-T 8M
		if((RF_freq>=300000)&&(RF_freq<=472000))
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA4;	   // LNA VTH/L=1.34/0.74     (R31=0xA4)
		}
		else
		{
			R848_SysFreq_Info.LNA_VTH_L=0xA5;	   // LNA VTH/L=1.34/0.84     (R31=0xA5)
		}
			R848_SysFreq_Info.LNA_TOP=0x03;		       // LNA TOP=4                    (R35[2:0]=3'b011)
			R848_SysFreq_Info.MIXER_TOP=0x05;	       // MIXER TOP=10               (R36[3:0]=4'b0100)
			R848_SysFreq_Info.MIXER_VTH_L=0x95;   // MIXER VTH/L=1.24/0.84  (R32=0x95)
			R848_SysFreq_Info.RF_TOP=0x40;               // RF TOP=5                        (R34[7:5]=3'b010)                 
			R848_SysFreq_Info.NRB_TOP=0x20;            // Nrb TOP=13                       (R36[7:4]=4'b0010)
			R848_SysFreq_Info.NRB_BW=0xC0;             // Nrb BW=lowest                  (R35[7:6]=2'b11)	                              
		break;
	
	} //end switch

	R848_SysFreq_Info.RF_FAST_DISCHARGE = 0x00;    //0 	 R848:R46[3:1]=3'b000
	R848_SysFreq_Info.RF_SLOW_DISCHARGE = 0x80;    //4   R848:R22[7:5]=2'b100
	R848_SysFreq_Info.RFPD_PLUSE_ENA = 0x10;	   //1   R848:R38[4]=1

	R848_SysFreq_Info.LNA_FAST_DISCHARGE = 0x0A;    //10  R848:R43[4:0]=5'b01010
	R848_SysFreq_Info.LNA_SLOW_DISCHARGE = 0x00;    //0  R848:R22[4:2]=3'b000
	R848_SysFreq_Info.LNAPD_PLUSE_ENA = 0x00 ;	    //0  R848:R17[7]=0

    //AGC Clk Rate
	R848_SysFreq_Info.AGC_CLK = 0x00;              //1k   R26[4:2]  //default

	//TF LPF setting
	switch(R848_Standard)
	{
		case R848_DTMB_4570:
	    case R848_DTMB_6000:
		case R848_DTMB_6M_BW_IF_5M:
		case R848_DTMB_6M_BW_IF_4500:
		case R848_DTMB_IF_5M:
			 //if(RF_freq<=196000)
			//	 R848_SysFreq_Info.BYP_LPF = 0x40;		  //low pass R12[6]
			 //else
				 R848_SysFreq_Info.BYP_LPF = 0x00;      //bypass  R12[6]

	break;

		case R848_DVB_T_6M:
		case R848_DVB_T_7M:
		case R848_DVB_T_8M:
		case R848_DVB_T_6M_IF_5M:
		case R848_DVB_T_7M_IF_5M:
		case R848_DVB_T_8M_IF_5M:
		case R848_DVB_T2_6M: 
		case R848_DVB_T2_7M:
		case R848_DVB_T2_8M:
		case R848_DVB_T2_1_7M: 
		case R848_DVB_T2_10M: 
		case R848_DVB_T2_6M_IF_5M:
		case R848_DVB_T2_7M_IF_5M:
		case R848_DVB_T2_8M_IF_5M:
		case R848_DVB_T2_1_7M_IF_5M: 
			if(RF_freq>=662000 && RF_freq<=670000)
			{
				R848_SysFreq_Info.NRB_TOP=0xB0;            // Nrb TOP=4                       (R36[7:4]=4'b1011)
				R848_SysFreq_Info.RF_SLOW_DISCHARGE = 0x20;    //1   R848:R22[7:5]=2'b001
				R848_SysFreq_Info.AGC_CLK = 0x18;		 //60Hz   R26[4:2] 
			}
			else
			{
				R848_SysFreq_Info.RF_SLOW_DISCHARGE = 0x80;    //4   R848:R22[7:5]=2'b100
				R848_SysFreq_Info.AGC_CLK = 0x00;		 //1KHz   R26[4:2] 
				switch(R848_Standard)
				{
					case R848_DVB_T2_6M: 
					case R848_DVB_T2_7M:
					case R848_DVB_T2_8M:
					case R848_DVB_T2_1_7M: 
					case R848_DVB_T2_10M: 
					case R848_DVB_T2_6M_IF_5M:
					case R848_DVB_T2_7M_IF_5M:
					case R848_DVB_T2_8M_IF_5M:
					case R848_DVB_T2_1_7M_IF_5M:
						R848_SysFreq_Info.AGC_CLK = 0x1C;		 //250Hz   R26[4:2] 
					break;
				}
			}
			if(RF_freq<=236000)
				R848_SysFreq_Info.BYP_LPF = 0x40;      //low pass  R12[6]
			else
				R848_SysFreq_Info.BYP_LPF = 0x00;      //bypass  R12[6]
		break;

		default:  //other standard
			 if(RF_freq<=236000)
				 R848_SysFreq_Info.BYP_LPF = 0x40;      //low pass  R12[6]
			 else
				 R848_SysFreq_Info.BYP_LPF = 0x00;      //bypass  R12[6]

        break;
	}//end switch

	return R848_SysFreq_Info;
	
	}




R848_ErrCode R848_Init()
{
		UINT8 i;

		if(R848_Initial_done_flag==FALSE)
		{

			  //reset filter cal.
			  for (i=0; i<R848_STD_SIZE; i++)
			  {	  
				  R848_Fil_Cal_flag[i] = FALSE;
				  R848_Fil_Cal_code[i] = 0;
				  R848_Fil_Cal_BW[i] = 0x00;
			  }

			  if(R848_IMR_done_flag==FALSE)
			  {

				if(R848_InitReg(R848_STD_SIZE) != RT_Success)        
				 return RT_Fail;

				if(R848_TF_Check() != RT_Success)        
				 return RT_Fail;

				  //start IMR calibration
				  if(R848_InitReg(R848_STD_SIZE) != RT_Success)        //write initial reg before doing IMR Cal
					 return RT_Fail;
	    
				  if(R848_Cal_Prepare(R848_IMR_CAL) != RT_Success)     
					  return RT_Fail;

				  if(R848_IMR(3, TRUE) != RT_Success)       //Full K node 3
					return RT_Fail;

				  if(R848_IMR(0, FALSE) != RT_Success)
					return RT_Fail;

				  if(R848_IMR(1, FALSE) != RT_Success)
					return RT_Fail;

				  if(R848_IMR(2, FALSE) != RT_Success)
					return RT_Fail;

				  if(R848_IMR(4, TRUE) != RT_Success)   //Full K node 4
					return RT_Fail;

				  R848_IMR_done_flag = TRUE;
			  }

#if(R848_SHARE_XTAL_OUT==TRUE)
		R848_Xtal_Pwr = XTAL_LARGE_HIGHEST;
#else
		//do Xtal check
		if(R848_InitReg(R848_STD_SIZE) != RT_Success)        
		 return RT_Fail;
		R848_Xtal_Pwr = XTAL_SMALL_LOWEST;
		R848_Xtal_Pwr_tmp = XTAL_SMALL_LOWEST;

			for (i=0; i<3; i++)
			{
				 if(R848_Xtal_Check() != RT_Success)        
					 return RT_Fail;

			 if(R848_Xtal_Pwr_tmp > R848_Xtal_Pwr)
				   R848_Xtal_Pwr = R848_Xtal_Pwr_tmp;
		}
#endif

			  R848_Initial_done_flag = TRUE;

		} //end if(check init flag)

		//write initial reg	
             if(R848_InitReg(R848_STD_SIZE) != RT_Success)        
			 {  return RT_Fail;}

		R848_pre_standard = R848_STD_SIZE;

	return RT_Success;
}



R848_ErrCode R848_InitReg(R848_Standard_Type R848_Standard)
{
	UINT8 InitArrayCunt = 0;
	
	//Write Full Table, Set Xtal Power = highest at initial
	R848_I2C_Len.RegAddr = 0x08;   //  R848:0x08
	R848_I2C_Len.Len = R848_REG_NUM;
	if(R848_Standard!=R848_DVB_S)
	{
		for(InitArrayCunt = 0; InitArrayCunt<R848_REG_NUM; InitArrayCunt ++)
		{
			R848_I2C_Len.Data[InitArrayCunt] = R848_iniArray_hybrid[InitArrayCunt];
			R848_Array[InitArrayCunt] = R848_iniArray_hybrid[InitArrayCunt];
		}
	}
	else
	{
		for(InitArrayCunt = 0; InitArrayCunt<R848_REG_NUM; InitArrayCunt ++)
		{
			R848_I2C_Len.Data[InitArrayCunt] = R848_iniArray_dvbs[InitArrayCunt];
			R848_Array[InitArrayCunt] = R848_iniArray_dvbs[InitArrayCunt];
		}
	}
	if(I2C_Write_Len(&R848_I2C_Len) != RT_Success)
		return RT_Fail;

	return RT_Success;
}





R848_ErrCode R848_TF_Check(void)
{
	UINT32   RingVCO = 0;
	UINT32   RingFreq = 72000;
	UINT32   RingRef = R848_Xtal;
	UINT8     divnum_ring = 0;

	if(R848_Xtal==16000)  //16M
	{
         divnum_ring = 11;
	}
	else  //24M
	{
		 divnum_ring = 2;
	}
	 RingVCO = (16+divnum_ring)* 8 * RingRef;
	 RingFreq = RingVCO/48;

	 if(R848_Cal_Prepare(R848_TF_LNA_CAL) != RT_Success)      
	      return RT_Fail;


     R848_I2C.RegAddr = 0x27;	
     R848_Array[31] = (R848_Array[31] & 0x03) | 0x80 | (divnum_ring<<2);  //pre div=6 & div_num
     R848_I2C.Data = R848_Array[31];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	 //Ring PLL PW on
	 R848_I2C.RegAddr = 0x12;	
     R848_Array[18] = (R848_Array[18] & 0xEF); 
     R848_I2C.Data = R848_Array[18];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	 //NAT Det Sour : Mixer buf out
	 R848_I2C.RegAddr = 0x25;	
     R848_Array[37] = (R848_Array[37] & 0x7F); 
     R848_I2C.Data = R848_Array[37];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;



	 //R848 R33[7:0]   

	 R848_Array[25] = (R848_Array[25] & 0x00) | 0x8B;  //out div=8, RF poly=low band, power=min_lp
	 if(RingVCO>=3200000)
	    R848_Array[25] = (R848_Array[25] & 0xDF);      //vco_band=high, R25[5]=0
	 else
        R848_Array[25] = (R848_Array[25] | 0x20);      //vco_band=low, R25[5]=1
	 R848_I2C.RegAddr = 0x21;
	 R848_I2C.Data = R848_Array[25];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;	

     //Must do before PLL()
	 if(R848_MUX(RingFreq + 5000, RingFreq, R848_STD_SIZE) != RT_Success)     //FilCal MUX (LO_Freq, RF_Freq)
	     return RT_Fail;

	 //Set PLL
	 if(R848_PLL((RingFreq + 5000), R848_STD_SIZE) != RT_Success)   //FilCal PLL
	       return RT_Fail;


	//Set LNA TF=(1,15),for VGA training   // R848 R8[6:0]
	 R848_I2C.RegAddr = 0x08;
     R848_Array[0] = (R848_Array[0] & 0x80) | 0x1F;  	
     R848_I2C.Data = R848_Array[0];
     if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;
	//Qctrl=off  // R848 R14[5] 
	 R848_I2C.RegAddr = 0x0E;
     R848_Array[6] = (R848_Array[6] & 0xDF);  	
     R848_I2C.Data = R848_Array[6];
     if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;

	// FilterComp OFF  // R848 R14[2]  
	 R848_I2C.RegAddr = 0x0E;
     R848_Array[6] = (R848_Array[6] & 0xFB);  	
     R848_I2C.Data = R848_Array[6];
     if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;

	// Byp-LPF: Bypass  R848 R12[6]  12-8=4  12(0x0C) is addr ; [4] is data
	 R848_I2C.RegAddr = 0x0C;
     R848_Array[4] = R848_Array[4] & 0xBF;  	
     R848_I2C.Data = R848_Array[4];
     if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;


	 //Adc=on; Vga code mode, Gain = -12dB 
	 //R848 R20[3:0]         set 0
	 //R848 R9[1]            set 1
	 //R848 R11[3]           set 0
	 //R848 R18[7]           set 0
	 //R848 R15[7]           set 0
	 
	 // VGA Gain = -12dB 
     R848_I2C.RegAddr = 0x14;
     R848_Array[12] = (R848_Array[12] & 0xF0);
     R848_I2C.Data = R848_Array[12];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	// Vga code mode
     R848_I2C.RegAddr = 0x09;
     R848_Array[1] = (R848_Array[1] | 0x02);
     R848_I2C.Data = R848_Array[1];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	// VGA 6dB
     R848_I2C.RegAddr = 0x0B;
     R848_Array[3] = (R848_Array[3] & 0xF7);
     R848_I2C.Data = R848_Array[3];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	// VGA PW ON
     R848_I2C.RegAddr = 0x12;
     R848_Array[10] = (R848_Array[10] & 0x7F);
     R848_I2C.Data = R848_Array[10];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	 // Adc on
     R848_I2C.RegAddr = 0x0F;
     R848_Array[7] = (R848_Array[7] & 0x7F);
     R848_I2C.Data = R848_Array[7];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;



	 //------- increase VGA power to let ADC read value significant ---------//
	 UINT8   VGA_Count = 0;
	 UINT8   VGA_Read = 0;

	 for(VGA_Count=0; VGA_Count < 16; VGA_Count ++)
	 {
		R848_I2C.RegAddr = 0x14;
		R848_I2C.Data = (R848_Array[12] & 0xF0) + VGA_Count;  
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

		R848_Delay_MS(10); //
		
		if(R848_Muti_Read(&VGA_Read) != RT_Success)
			return RT_Fail;

		if(VGA_Read > 40*ADC_READ_COUNT)
			break;
	 }

	 //Set LNA TF=(0,0)
	 R848_I2C.RegAddr = 0x08;
     R848_Array[0] =(R848_Array[0] & 0x80);  	
     R848_I2C.Data = R848_Array[0];
     if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;

	 R848_Delay_MS(10); //

	 if(R848_Muti_Read(&VGA_Read) != RT_Success)
		  return RT_Fail;

	 if(VGA_Read > (36*ADC_READ_COUNT))
        R848_DetectTfType = R848_UL_USING_BEAD;
	 else
	    R848_DetectTfType = R848_UL_USING_270NH;

	return RT_Success;
}

R848_ErrCode R848_Xtal_Check(void)
{
	UINT8 i = 0;

	// TF force sharp mode (for stable read)
	R848_I2C.RegAddr = 0x16;
	R848_Array[14] = R848_Array[14] | 0x01;    
	R848_I2C.Data = R848_Array[14];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	// NA det off (for stable read)
	R848_I2C.RegAddr = 0x26;
	R848_Array[30] = R848_Array[30] | 0x80;  
	R848_I2C.Data = R848_Array[30];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	//Set NA det 710 = OFF
	R848_I2C.RegAddr  = 0x28;								
	R848_Array[32] = (R848_Array[32] | 0x08);
    R848_I2C.Data = R848_Array[32];
    if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;
	
	// Xtal_pow=lowest(11)  R848:R23[6:5]
	R848_I2C.RegAddr = 0x17;
	R848_Array[15] = (R848_Array[15] & 0x9F) | 0x60;
	R848_I2C.Data = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	//Xtal_Gm=SMALL(0) R848:R27[0]
	R848_I2C.RegAddr = 0x1B;
	R848_Array[19] = (R848_Array[19] & 0xFE) | 0x00;
	R848_I2C.Data = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;


	//set pll autotune = 128kHz  R848:R23[4:3]
	R848_I2C.RegAddr = 0x17;
	R848_Array[15] = R848_Array[15] & 0xE7;
	R848_I2C.Data = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	//set manual initial reg = 1 000000; b5=0 => cap 30p;		 R848:R27[7:0]
	R848_I2C.RegAddr = 0x1B;
	R848_Array[19] = (R848_Array[19] & 0x80) | 0x40;
	R848_I2C.Data = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	//set auto
	R848_I2C.RegAddr = 0x1B;
	R848_Array[19] = (R848_Array[19] & 0xBF);
	R848_I2C.Data = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	//Xtal cap
	R848_I2C.RegAddr = 0x1B;
	if(R848_Xtal==16000)   // Xtal cap =16pF
		R848_Array[19] &=  0xDF;
	else				// Xtal cap =10pF
		R848_Array[19] |= 0x20;
	R848_I2C.Data = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	R848_Delay_MS(10);


	// Xtal_pow=lowest(11) R848:R23[6:5]
	R848_I2C.RegAddr = 0x17;
	R848_Array[15] = (R848_Array[15] & 0x9F) | 0x60;
	R848_I2C.Data = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	//Xtal_Gm=SMALL(0) R848:R27[0]
	R848_I2C.RegAddr = 0x1B;
	R848_Array[19] = (R848_Array[19] & 0xFE) | 0x00;
	R848_I2C.Data = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;




	for(i=0; i<XTAL_CHECK_SIZE; i++)
	{
	    //set power
		if(i<=XTAL_SMALL_HIGHEST)
		{
			R848_Array[15] = (R848_Array[15] & 0x9F) | ((UINT8)(XTAL_SMALL_HIGHEST-i)<<5) ;   
			R848_Array[19] = (R848_Array[19] & 0xFE) | 0x00 ;		//| 0x00;  //SMALL Gm(0)
		}
		else if(i==XTAL_LARGE_HIGHEST)
		{
			//R848_Array[15] = (R848_Array[15] & 0x87) | 0x00 | 0x18;  //3v Gm, highest
			R848_Array[15] = (R848_Array[15] & 0x9F) | 0X00 ;   
			R848_Array[19] = (R848_Array[19] & 0xFE) | 0x01 ;		//| 0x00;  //LARGE Gm(1)
		}
		else
		{
            //R848_Array[15] = (R848_Array[15] & 0x87) | 0x00 | 0x18;  //3v Gm, highest
			R848_Array[15] = (R848_Array[15] & 0x9F) | 0X00 ;   
			R848_Array[19] = (R848_Array[19] & 0xFE) | 0x01 ;		//| 0x00;  //LARGE Gm(1)
		}


		R848_I2C.RegAddr = 0x17;
		R848_I2C.Data = R848_Array[15];
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

		R848_I2C.RegAddr = 0x1B;
		R848_I2C.Data = R848_Array[19];
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;


		R848_Delay_MS(50);

		R848_I2C_Len.RegAddr = 0x00;
		R848_I2C_Len.Len = 48;
		if(I2C_Read_Len(&R848_I2C_Len) != RT_Success)
		{
			if(I2C_Read_Len(&R848_I2C_Len) != RT_Success)
				return RT_Fail;
		}
			

		//depend on init Nint & Div value (N=59.6667, Div=16)
		//lock to VCO band 8 if VCO=2768M for 16M Xtal
		//lock to VCO band 46 if VCO=2768M for 24M Xtal
#if(R848_Xtal==16000)
		//if(((R848_I2C_Len.Data[2] & 0x40) == 0x40) && ((R848_I2C_Len.Data[2] & 0x3F) <= 11) && ((R848_I2C_Len.Data[2] & 0x3F) >= 5))  
		if(((R848_I2C_Len.Data[2] & 0x40) == 0x40) && ((R848_I2C_Len.Data[2] & 0x3F) <= 14) && ((R848_I2C_Len.Data[2] & 0x3F) >= 2))  
#else
		if(((R848_I2C_Len.Data[2] & 0x40) == 0x40) && ((R848_I2C_Len.Data[2] & 0x3F) <= 51) && ((R848_I2C_Len.Data[2] & 0x3F) >= 39)) 
#endif
		{
		    R848_Xtal_Pwr_tmp = i;
			break;
		}

	    if(i==(XTAL_CHECK_SIZE-1))
		{
	        R848_Xtal_Pwr_tmp = i;
		}
	}


    return RT_Success;
}	
R848_ErrCode R848_Cal_Prepare(UINT8 u1CalFlag)
{
	 R848_Cal_Info_Type  Cal_Info;
 
	 switch(u1CalFlag)
	 {
	    case R848_IMR_CAL:
			    Cal_Info.FILTER_6DB = 0x08;              //+6dB		 R848:R38[3]
				//Cal_Info.RFBUF_OUT = 0x60;            //from RF_Buf ON, RF_Buf pwr off		 // R848:R12[5]
				//from RF_Buf ON, RF_Buf pwr off
				Cal_Info.RFBUF_OUT = 0x20;				//from RF_Buf ON
				Cal_Info.RFBUF_POWER=0x04;				//RF_BUF_pwr OFF
				Cal_Info.LNA_POWER = 0x80;              //LNA power OFF,RF_Buf pwr off  //  R848:R8[7]
				//Cal_Info.LNA_POWER = 0x00;				//LNA need on
				Cal_Info.TF_CAL = 0x00;					// TF cal OFF, -6dB	OFF   // R848:R14[6:5]
				Cal_Info.MIXER_AMP_GAIN = 0x08;			//manual +8				  // R848:R15[4:0]
				Cal_Info.MIXER_BUFFER_GAIN = 0x10;		//manual min(0)			  // R848:R34[4:0]
				Cal_Info.LNA_GAIN = 0x9F;                 //manual: max		//  R848:R13[7:0]
				//Cal_Info.LNA_GAIN = 0x80;
				R848_IMR_Cal_Type = R848_IMR_CAL;
			break;
		case R848_IMR_LNA_CAL:						    
				Cal_Info.FILTER_6DB = 0x08;              //+6dB
				//Cal_Info.RFBUF_OUT = 0x00;              //from RF_Buf ON, RF_Buf pwr on

				Cal_Info.RFBUF_OUT = 0x00;				//from RF_Buf ON
				Cal_Info.RFBUF_POWER=0x00;				//RF_BUF_pwr OFF

				Cal_Info.LNA_POWER = 0x80;             // LNA power OFF
				//Cal_Info.LNA_POWER = 0x00;				//LNA need on
				Cal_Info.TF_CAL = 0x60;				   // TF cal ON, -6dB ON
				Cal_Info.MIXER_AMP_GAIN = 0x00;    //manual min(0)
				Cal_Info.MIXER_BUFFER_GAIN = 0x10; //manual min(0)
				Cal_Info.LNA_GAIN = 0x9F;                 //manual: max
				//Cal_Info.LNA_GAIN = 0x80;
				R848_IMR_Cal_Type = R848_IMR_LNA_CAL;
			break;
        case R848_TF_CAL: //TBD
			    Cal_Info.FILTER_6DB = 0x08;              //+6dB
				//Cal_Info.RFBUF_OUT = 0x60;               //from RF_Buf ON, RF_Buf pwr off

				Cal_Info.RFBUF_OUT = 0x20;				//from RF_Buf ON
				Cal_Info.RFBUF_POWER=0x04;				//RF_BUF_pwr OFF

				Cal_Info.RFBUF_OUT = 0x20;
				Cal_Info.LNA_POWER = 0x80;              //LNA power OFF
				//Cal_Info.LNA_POWER = 0x00;				//LNA need on
				Cal_Info.TF_CAL = 0x00;					//TF cal OFF, -6dB OFF	
				Cal_Info.MIXER_AMP_GAIN = 0x00;    //manual min(0)
				Cal_Info.MIXER_BUFFER_GAIN = 0x10; //manual min(0)
				Cal_Info.LNA_GAIN = 0x9F;                  //manual: max
			break;
        case R848_TF_LNA_CAL:
			    Cal_Info.FILTER_6DB = 0x08;              //+6dB				
				//Cal_Info.RFBUF_OUT = 0x00;              //from RF_Buf ON, RF_Buf pwr on	

				Cal_Info.RFBUF_OUT = 0x00;				//from RF_Buf ON
				Cal_Info.RFBUF_POWER=0x00;				//RF_BUF_pwr OFF

				Cal_Info.LNA_POWER = 0x80;              //LNA power OFF
				//Cal_Info.LNA_POWER = 0x00;				//LNA need on
				Cal_Info.TF_CAL = 0x60;					// TF cal ON, -6dB ON	
				Cal_Info.MIXER_AMP_GAIN = 0x00;    //manual min(0)
				Cal_Info.MIXER_BUFFER_GAIN = 0x10; //manual min(0)
				Cal_Info.LNA_GAIN = 0x80;                  //manual: min
			break;
		case R848_LPF_CAL: 
			    Cal_Info.FILTER_6DB = 0x08;              //+6dB						//  R848:R38[3]
				//Cal_Info.RFBUF_OUT = 0x60;               //from RF_Buf ON, RF_Buf pwr off
				//Cal_Info.RFBUF_OUT = 0x20;				//RF_Buf pwr off			//  R848:R12[5]
				Cal_Info.RFBUF_OUT = 0x20;				//from RF_Buf ON
				Cal_Info.RFBUF_POWER=0x04;				//RF_BUF_pwr OFF

				Cal_Info.LNA_POWER = 0x80;              //LNA power OFF, TF cal OFF, -6dB OFF	
				//Cal_Info.LNA_POWER = 0x00;              //LNA need on			   //  R848:R8[7]
				Cal_Info.TF_CAL = 0x00;					// TF cal OFF, -6dB OFF		// R848:R14[6:5]
				Cal_Info.MIXER_AMP_GAIN = 0x08;    //manual +8						// R848:R15[4:0]
				Cal_Info.MIXER_BUFFER_GAIN = 0x10; //manual min(0)					// R848:R34[4:0]	
				Cal_Info.LNA_GAIN = 0x9F;                 //manual: max				// R848:R13[7:0]
				R848_IMR_Cal_Type = R848_LPF_CAL;
			break;
		case R848_LPF_LNA_CAL:
			    Cal_Info.FILTER_6DB = 0x08;              //+6dB
				//Cal_Info.RFBUF_OUT = 0x00;               //from RF_Buf ON, RF_Buf pwr on
				Cal_Info.RFBUF_OUT = 0x00;				//from RF_Buf ON
				Cal_Info.RFBUF_POWER=0x00;				//RF_BUF_pwr OFF
				Cal_Info.LNA_POWER = 0x80;              //LNA power OFF
				//Cal_Info.LNA_POWER = 0x00;              //LNA need on
				Cal_Info.TF_CAL = 0x20;					// TF cal ON, -6dB OFF	
				Cal_Info.MIXER_AMP_GAIN = 0x00;    //manual min(0)
				Cal_Info.MIXER_BUFFER_GAIN = 0x10; //manual min(0)
				Cal_Info.LNA_GAIN = 0x80;                  //manual: min
			break;
		default:
			    Cal_Info.FILTER_6DB = 0x08;              //+6dB
				//Cal_Info.RFBUF_OUT = 0x60;               //from RF_Buf ON, RF_Buf pwr off
				Cal_Info.RFBUF_OUT = 0x20;				//from RF_Buf ON
				Cal_Info.RFBUF_POWER=0x04;				//RF_BUF_pwr OFF
				Cal_Info.LNA_POWER = 0x80;              //LNA power OFF
				//Cal_Info.LNA_POWER = 0x00;              //LNA need on
				Cal_Info.TF_CAL = 0x00;					//TF cal OFF, -6dB OFF
				Cal_Info.MIXER_AMP_GAIN = 0x08;    //manual +8
				Cal_Info.MIXER_BUFFER_GAIN = 0x10; //manual min(0)
				Cal_Info.LNA_GAIN = 0x9F;                 //manual: max
	 }

	  //Ring From RF_Buf Output & RF_Buf Power
	  R848_I2C.RegAddr = 0x0C;
      R848_Array[4] = (R848_Array[4] & 0xDF) | Cal_Info.RFBUF_OUT;   //  R848:R12[5]  12-8=4  12(0x0C) is addr ; [4] is data
      R848_I2C.Data = R848_Array[4];
      if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail; 


	  //RF_Buf Power
	  R848_I2C.RegAddr = 0x09;
      R848_Array[1] = (R848_Array[1] & 0xFB) | Cal_Info.RFBUF_POWER;   //  R848:R12[5]  12-8=4  12(0x0C) is addr ; [4] is data
      R848_I2C.Data = R848_Array[1];
      if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail; 
  
	  /*//TF cal (LNA power ON/OFF , TF cal ON/OFF, TF_-6dB ON/OFF)
	  R848_I2C.RegAddr = 0x06;
      R848_Array[6] = (R848_Array[6] & 0x1F) | Cal_Info.LNA_POWER;
      R848_I2C.Data = R848_Array[6];
      if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail; */


	  //(LNA power ON/OFF )
	  R848_I2C.RegAddr = 0x08;
      R848_Array[0] = (R848_Array[0] & 0x7F) | Cal_Info.LNA_POWER;	 //  R848:R8[7]  8-8=0  8(0x08) is addr ; [0] is data
	  //R848_Array[0] = (R848_Array[0] & 0x80) 	 // R848:R8[7]  8-8=0  14(0x08) is addr ; [0] is data
      R848_I2C.Data = R848_Array[0];
      if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;


	  //TF cal (TF cal ON/OFF, TF_-6dB ON/OFF)
	  R848_I2C.RegAddr = 0x0E;
      R848_Array[6] = (R848_Array[6] & 0x9F) | Cal_Info.TF_CAL;	 // R848:R14[6:5]  14-8=6  14(0x0E) is addr ; [6] is data
      R848_I2C.Data = R848_Array[6];
      if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;


	  //LNA gain
	  R848_I2C.RegAddr = 0x0D;
	  R848_Array[5] = (R848_Array[5] & 0x60) | Cal_Info.LNA_GAIN; // R848:R13[7:0]  13-8=5  13(0x0D) is addr ; [5] is data
      R848_I2C.Data = R848_Array[5];
      if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;

	  //Mixer Amp Gain
	  R848_I2C.RegAddr = 0x0F;
	  R848_Array[7] = (R848_Array[7] & 0xE0) | Cal_Info.MIXER_AMP_GAIN; // R848:R15[4:0]  15-8=7  15(0x0F) is addr ; [7] is data
      R848_I2C.Data = R848_Array[7];
      if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;

	  //Mixer Buffer Gain
	  R848_I2C.RegAddr = 0x22;								// R848:R34[4:0]  34-8=26  34(0x22) is addr ; [26] is data
	  R848_Array[26] = (R848_Array[26] & 0xE0) | Cal_Info.MIXER_BUFFER_GAIN;  
      R848_I2C.Data = R848_Array[26];
      if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;	 

	  // Set filter +0/6dB; NA det=OFF 
      R848_I2C.RegAddr  = 0x26;								// R848:R38[3]  38-8=30  38(0x26) is addr ; [30] is data
	  R848_Array[30] = (R848_Array[30] & 0xF7) | Cal_Info.FILTER_6DB | 0x80;
      R848_I2C.Data = R848_Array[30];
      if(I2C_Write(&R848_I2C) != RT_Success)
	     return RT_Fail;

	  //Set NA det 710 = OFF
	  R848_I2C.RegAddr  = 0x28;								// R848:R40[3]  40-8=32  40(0x28) is addr ; [32] is data
	  R848_Array[32] = (R848_Array[32] | 0x08);
      R848_I2C.Data = R848_Array[32];
      if(I2C_Write(&R848_I2C) != RT_Success)
	     return RT_Fail;


	 //---- General calibration setting ----//	 
	 //IMR IQ cap=0
	 R848_I2C.RegAddr = 0x0B;		//R848:R11[1:0]  11-8=3  11(0x0B) is addr ; [3] is data
     R848_Array[3] = (R848_Array[3] & 0xFC);
     R848_I2C.Data = R848_Array[3];
     if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;

	 // Set RF_Flag ON(%)
	 R848_I2C.RegAddr = 0x16;		//R848:R22[0]  22-8=14  22(0x16) is addr ; [14] is data
     R848_Array[14] = R848_Array[14] | 0x01;  //force far-end mode
     R848_I2C.Data = R848_Array[14];
     if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;

	 //RingPLL power ON
     R848_I2C.RegAddr = 0x12;	  //R848:R18[4]  18-8=10  18(0x12) is addr ; [10] is data
     R848_Array[10] = (R848_Array[10] & 0xEF);
     R848_I2C.Data = R848_Array[10];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	 //LPF filter code = 15
     R848_I2C.RegAddr = 0x12;	//R848:R18[3:0]  18-8=10  18(0x12) is addr ; [10] is data
     R848_Array[10] = (R848_Array[10] & 0xF0) | 0x0F;
     R848_I2C.Data = R848_Array[10];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;
	 
     //HPF corner=narrowest; LPF coarse=6M; 1.7M disable
     R848_I2C.RegAddr = 0x13;	//R848:R19[7:0]  19-8=11  19(0x13) is addr ; [11] is data
     R848_Array[11] = (R848_Array[11] & 0x00) | 0x60;
     R848_I2C.Data = R848_Array[11];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

     //ADC/VGA PWR on; Vga code mode(b4=1), Gain = 26.5dB; Large Code mode Gain(b5=1)
	 //ADC PWR on (b7=0)
	 R848_I2C.RegAddr = 0x0F;	//R848:R15[7]  15-8=7  15(0x0F) is addr ; [7] is data
     R848_Array[7] = (R848_Array[7] & 0x7F);
     R848_I2C.Data = R848_Array[7];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	 //VGA PWR on (b0=0)
	 //R848_I2C.RegAddr = 0x09;	// R848:R9[0]  9-8=1  9(0x09) is addr ; [1] is data
     //R848_Array[1] = (R848_Array[1] & 0xFE);  
     //R848_I2C.Data = R848_Array[1];
     //if(I2C_Write(&R848_I2C) != RT_Success)
     //      return RT_Fail;

	 //VGA PWR on (b0=0)  MT2
	 R848_I2C.RegAddr = 0x12;	//R848:R18[7]  9-8=1  9(0x09) is addr ; [1] is data
     R848_Array[10] = (R848_Array[10] & 0x7F);  
     R848_I2C.Data = R848_Array[10];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	 //Large Code mode Gain(b5=1)
	 R848_I2C.RegAddr = 0x0B;	//R848:R11[3]  11-8=3  11(0x0B) is addr ; [3] is data
     R848_Array[3] = (R848_Array[3] & 0xF7) | 0x08;  
     R848_I2C.Data = R848_Array[3];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	 //Vga code mode(b4=1)
	 R848_I2C.RegAddr = 0x09;	//R848:R9[1]  9-8=1  9(0x09) is addr ; [1] is data
     R848_Array[1] = (R848_Array[1] & 0xFD) | 0x02;  
     R848_I2C.Data = R848_Array[1];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	 //Gain = 26.5dB
     R848_I2C.RegAddr = 0x14;	//R848:R20[3:0]  20-8=12  20(0x14) is addr ; [12] is data
     R848_Array[12] = (R848_Array[12] & 0xF0) | 0x0B;  
     R848_I2C.Data = R848_Array[12];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;



	 //LNA, RF, Nrb dector pw on; det2 cap=IF_det 
     R848_I2C.RegAddr = 0x25;	//R848:R37[3:0]  37-8=29  37(0x25) is addr ; [29] is data
     R848_Array[29] = (R848_Array[29] & 0xF0) | 0x02;
     R848_I2C.Data = R848_Array[29];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

      return RT_Success;
}




R848_ErrCode R848_IMR(UINT8 IMR_MEM, BOOL IM_Flag)
{
	UINT32 RingVCO = 0;
	UINT32 RingFreq = 0;
	UINT8  u1MixerGain = 8;
	UINT32 RingRef = R848_Xtal;
	UINT8   divnum_ring = 0;

	R848_SectType IMR_POINT;

	R848_Array[31] &= 0x3F;   //clear ring_div1, R24[7:6]	//R848:R39[7:6]  39-8=31  39(0x27) is addr ; [31] is data 
	R848_Array[25] &= 0xFC;   //clear ring_div2, R25[1:0]	//R848:R33[1:0]  33-8=25  33(0x21) is addr ; [25] is data 

	if(R848_Xtal==16000)  //16M
	{
         divnum_ring = 9;    //R39[5:2]=9  0x24
	}
	else  //24M
	{
		 divnum_ring = 1;		//R39[5:2]=1   0x04
	}

	RingVCO = (16+divnum_ring)* 8 * RingRef;
	
	if(RingVCO>=3200000)
	{
		R848_Array[25] &= 0xDF;   //clear vco_band, R25[5]		//R848:R33[5]    33-8=25  33(0x21) is addr ; [25] is data 
	}
	else
	{
		R848_Array[25] |= 0x20;   //clear vco_band, R25[5]		//R848:R33[5]    33-8=25  33(0x21) is addr ; [25] is data 
	}

	switch(IMR_MEM)
	{
	case 0: // RingFreq = 66.66M
		RingFreq = RingVCO/48;
		R848_Array[31] |= 0x80;  // ring_div1 /6 (2)
		R848_Array[25] |= 0x03;  // ring_div2 /8 (3)		
		//R848_Array[25] |= 0x00;  // vco_band = 0 (high)
        //R848_Array[31] |= 0x24;  // ring_div_num = 9
		u1MixerGain = 8;
		break;
	case 1: // RingFreq = 200M
		RingFreq = RingVCO/16;
		R848_Array[31] |= 0x00;  // ring_div1 /4 (0)
		R848_Array[25] |= 0x02;  // ring_div2 /4 (2)		
		//R848_Array[25] |= 0x00;  // vco_band = 0 (high)
        //R848_Array[31] |= 0x24;  // ring_div_num = 9
		u1MixerGain = 6;
		break;
	case 2: // RingFreq = 400M
		RingFreq = RingVCO/8;
		R848_Array[31] |= 0x00;  // ring_div1 /4 (0)
		R848_Array[25] |= 0x01;  // ring_div2 /2 (1)		
		//R848_Array[25] |= 0x00;  // vco_band = 0 (high)
        //R848_Array[31] |= 0x24;  // ring_div_num = 9
		u1MixerGain = 6;
		break;
	case 3: // RingFreq = 533.33M
		RingFreq = RingVCO/6;
		R848_Array[31] |= 0x80;  // ring_div1 /6 (2)
		R848_Array[25] |= 0x00;  // ring_div2 /1 (0)		
		//R848_Array[25] |= 0x00;  // vco_band = 0 (high)
        //R848_Array[31] |= 0x24;  // ring_div_num = 9
		u1MixerGain = 8;
		break;
	case 4: // RingFreq = 800M
		RingFreq = RingVCO/4;
		R848_Array[31] |= 0x00;  // ring_div1 /4 (0)
		R848_Array[25] |= 0x00;  // ring_div2 /1 (0)		
		//R848_Array[25] |= 0x00;  // vco_band = 0 (high)
        //R848_Array[31] |= 0x24;  // ring_div_num = 9
		u1MixerGain = 8;
		break;
	default:
		RingFreq = RingVCO/4;
		R848_Array[31] |= 0x00;  // ring_div1 /4 (0)
		R848_Array[25] |= 0x00;  // ring_div2 /1 (0)		
		//R848_Array[25] |= 0x00;  // vco_band = 0 (high)
        //R848_Array[31] |= 0x24;  // ring_div_num = 9
		u1MixerGain = 8;
		break;
	}

	//Mixer Amp Gain
	R848_I2C.RegAddr = 0x0F;	//R848:R15[4:0] 
	R848_Array[7] = (R848_Array[7] & 0xE0) | u1MixerGain; 
    R848_I2C.Data = R848_Array[7];
    if(I2C_Write(&R848_I2C) != RT_Success)
        return RT_Fail;

	//write I2C to set RingPLL
	R848_I2C.RegAddr = 0x27;
	R848_Array[31]=(R848_Array[31] & 0xC3) | (divnum_ring << 2);
	R848_I2C.Data    = R848_Array[31] & 0xC3;
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	
	R848_I2C.RegAddr = 0x21;
	R848_I2C.Data    = R848_Array[25];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	//Ring PLL power
	//if((RingFreq>=0) && (RingFreq<R848_RING_POWER_FREQ_LOW))
	if((RingFreq>=0) && ((RingFreq<R848_RING_POWER_FREQ_LOW)||(RingFreq>R848_RING_POWER_FREQ_HIGH)))  //R848:R33[3:2] 
         R848_Array[25] = (R848_Array[25] & 0xF3) | 0x08;   //R25[3:2]=2'b10; min_lp
	else
        R848_Array[25] = (R848_Array[25] & 0xF3) | 0x00;   //R25[3:2]=2'b00; min

	R848_I2C.RegAddr = 0x21;
	R848_I2C.Data = R848_Array[25];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	
	//Must do MUX before PLL() 
	if(R848_MUX(RingFreq - R848_IMR_IF, RingFreq, R848_STD_SIZE) != RT_Success)      //IMR MUX (LO, RF)
		return RT_Fail;

	if(R848_PLL((RingFreq - R848_IMR_IF), R848_STD_SIZE) != RT_Success)  //IMR PLL
	    return RT_Fail;

	//Set TF, place after R848_MUX( )
	//TF is dependent to LNA/Mixer Gain setting
	if(R848_SetTF(RingFreq, (UINT8)R848_SetTfType) != RT_Success)
		return RT_Fail;

	//clear IQ_cap
	IMR_POINT.Iqcap = R848_Array[3] & 0xFC; // R848:R11[1:0] 

	if(IM_Flag == TRUE)
	{
	     if(R848_IQ(&IMR_POINT) != RT_Success)
		    return RT_Fail;
	}
	else
	{
		IMR_POINT.Gain_X = R848_IMR_Data[3].Gain_X;
		IMR_POINT.Phase_Y = R848_IMR_Data[3].Phase_Y;
		IMR_POINT.Value = R848_IMR_Data[3].Value;
		//IMR_POINT.Iqcap = R848_IMR_Data[3].Iqcap;
		if(R848_F_IMR(&IMR_POINT) != RT_Success)
			return RT_Fail;
	}

	//Save IMR Value
	switch(IMR_MEM)
	{
	case 0:
		R848_IMR_Data[0].Gain_X  = IMR_POINT.Gain_X;
		R848_IMR_Data[0].Phase_Y = IMR_POINT.Phase_Y;
		R848_IMR_Data[0].Value = IMR_POINT.Value;
		R848_IMR_Data[0].Iqcap = IMR_POINT.Iqcap;		
		break;
	case 1:
		R848_IMR_Data[1].Gain_X  = IMR_POINT.Gain_X;
		R848_IMR_Data[1].Phase_Y = IMR_POINT.Phase_Y;
		R848_IMR_Data[1].Value = IMR_POINT.Value;
		R848_IMR_Data[1].Iqcap = IMR_POINT.Iqcap;
		break;
	case 2:
		R848_IMR_Data[2].Gain_X  = IMR_POINT.Gain_X;
		R848_IMR_Data[2].Phase_Y = IMR_POINT.Phase_Y;
		R848_IMR_Data[2].Value = IMR_POINT.Value;
		R848_IMR_Data[2].Iqcap = IMR_POINT.Iqcap;
		break;
	case 3:
		R848_IMR_Data[3].Gain_X  = IMR_POINT.Gain_X;
		R848_IMR_Data[3].Phase_Y = IMR_POINT.Phase_Y;
		R848_IMR_Data[3].Value = IMR_POINT.Value;
		R848_IMR_Data[3].Iqcap = IMR_POINT.Iqcap;
		break;
	case 4:
		R848_IMR_Data[4].Gain_X  = IMR_POINT.Gain_X;
		R848_IMR_Data[4].Phase_Y = IMR_POINT.Phase_Y;
		R848_IMR_Data[4].Value = IMR_POINT.Value;
		R848_IMR_Data[4].Iqcap = IMR_POINT.Iqcap;
		break;
    default:
		R848_IMR_Data[4].Gain_X  = IMR_POINT.Gain_X;
		R848_IMR_Data[4].Phase_Y = IMR_POINT.Phase_Y;
		R848_IMR_Data[4].Value = IMR_POINT.Value;
		R848_IMR_Data[4].Iqcap = IMR_POINT.Iqcap;
		break;
	}
	return RT_Success;
}


R848_ErrCode R848_PLL(UINT32 LO_Freq, R848_Standard_Type R848_Standard)
{
	UINT8  MixDiv = 2;
	UINT8  DivBuf = 0;
	UINT8  Ni = 0;
	UINT8  Si = 0;
	UINT8  DivNum = 0;
	UINT16  Nint = 0;
	UINT32 VCO_Min = 2410000; 
	UINT32 VCO_Max = VCO_Min*2;
	UINT32 VCO_Freq = 0;
	UINT32 PLL_Ref	= R848_Xtal;		
	UINT32 VCO_Fra	= 0;		
	UINT16 Nsdm = 2;
	UINT16 SDM = 0;
	UINT16 SDM16to9 = 0;
	UINT16 SDM8to1 = 0;
	UINT8   CP_CUR = 0x00;
	UINT8   CP_OFFSET = 0x00;
	UINT8   SDM_RES = 0x00;
	UINT8   XTAL_POW1 = 0x00;
	UINT8   XTAL_POW0 = 0x00;
	UINT8   XTAL_GM = 0x00;
	UINT16  u2XalDivJudge;
	UINT8   u1XtalDivRemain;
	UINT8   VCO_current_trial = 0;

	UINT8   u1RfFlag = 0;
	UINT8   u1PulseFlag = 0;
	UINT8   u1SPulseFlag=0;
	UINT8   cp_cur_24[50] = {
			5, 5, 5, 5, 5, 4, 0, 4, 5, 4, 
			5, 3, 5, 3, 3, 0, 3, 0, 0, 0, 
			0, 0, 0, 0, 0, 0, 5, 0, 3, 0,
			4, 0, 2, 0, 4, 0, 3, 5, 1, 5,
			0, 5, 2, 5, 3, 5, 3, 5, 1, 5};

	//112MHz to 176MHz CP Set Value
	UINT8   cp_cur_16_low[5] = {
			5, 2, 3, 1, 3};
	//304MHz to 496MHz CP Set Value
	UINT8   cp_cur_16_mid[25] = {
			2, 0, 4, 0, 3, 0, 1, 0, 4, 0, 
			2, 0, 3, 0, 4, 0, 1, 5, 1, 5, 
			1, 5, 3, 5, 4};

	//TF, NA fix
	u1RfFlag = (R848_Array[14] & 0x01);         //R22[0]
	u1PulseFlag = (R848_Array[30] & 0x80);   //R38[7]
	u1SPulseFlag= (R848_Array[32] & 0x08);   //R40[3]


	R848_I2C.RegAddr = 0x16;
	R848_Array[14] = R848_Array[14] | 0x01;		// TF force sharp mode
	R848_I2C.Data = R848_Array[14];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;


	R848_I2C.RegAddr = 0x26;	
	R848_Array[30] = R848_Array[30] | 0x80;		// NA det off
	R848_I2C.Data = R848_Array[30];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	//Set NA det 710 = OFF
	R848_I2C.RegAddr  = 0x28;								
	R848_Array[32] = (R848_Array[32] | 0x08);
    R848_I2C.Data = R848_Array[32];
    if(I2C_Write(&R848_I2C) != RT_Success)
	   return RT_Fail;


	//Xtal cap
	R848_I2C.RegAddr = 0x1B;
	if(R848_Xtal==16000)   // Xtal cap =16pF
		R848_Array[19] = (R848_Array[19] & 0xDF);
	else				// Xtal cap =10pF
		R848_Array[19] = (R848_Array[19] | 0x20) ;
	R848_I2C.Data = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	SDM_RES = 0x00;    //short, R27[4:3]=00
	if(R848_Xtal==16000)
	{
		//cp cur & offset setting
		if(R848_Standard < R848_ATV_SIZE) //ATV
		{
			u2XalDivJudge = (UINT16) (LO_Freq/1000/8);

			if((u2XalDivJudge==6)||(u2XalDivJudge==14)||(u2XalDivJudge==16)||(u2XalDivJudge==18)||(u2XalDivJudge==20))
				SDM_RES = 0x18;    //400R, R27[4:3]=11
			else
				SDM_RES = 0x00;    //short, R27[4:3]=00

			//offset
			if(LO_Freq < (160000+R848_IF_HIGH))  //  R848:R21[7]  21-8=13  21(0x15) is addr ; [13] is data	// R848:R21[7]
			{
				CP_OFFSET = 0x80;  //30u,   [2]=1
			}
			else if ((u2XalDivJudge>=38)&&(u2XalDivJudge<63))
			{
				if((u2XalDivJudge % 2)==1) //odd
					CP_OFFSET = 0x00;  //0u,   [2]=0
				else
					CP_OFFSET = 0x80;  //30u,     [2]=1
			}
			else 
			{
				CP_OFFSET = 0x00;  //0u,     [2]=0
			}

			//current
			if(LO_Freq < (48000+R848_IF_HIGH))
				CP_CUR = 0x00;        //0.7m, [6:4]=000
			else if(LO_Freq < (64000+R848_IF_HIGH))
				CP_CUR = 0x40;        //0.3m, [6:4]=100
			else if(LO_Freq < (80000+R848_IF_HIGH))
				CP_CUR = 0x00;        //0.7m, [6:4]=000
			else if(LO_Freq < (104000+R848_IF_HIGH))
				CP_CUR = 0x40;        //0.3m, [6:4]=100
			else if(LO_Freq < (176000+R848_IF_HIGH))
				if(u2XalDivJudge % 2 == 1) //odd
					CP_CUR = 0x30;        //0.4m, [6:4]=011
				else
					CP_CUR = ( 0x00 | ( cp_cur_16_low[(u2XalDivJudge-14)/2] << 4));  //14, 16, 18, 20, 22
			else if(LO_Freq < (296000+R848_IF_HIGH))
				CP_CUR = 0x00;        //0.7m, [6:4]=000		
			else if(LO_Freq < (496000+R848_IF_HIGH))
				CP_CUR = ( 0x00 | ( cp_cur_16_mid[(u2XalDivJudge-38)] << 4));
			else if(LO_Freq < (592000+R848_IF_HIGH))
				CP_CUR = 0x50;        //0.2m, [6:4]=101
			else if(LO_Freq < (744000+R848_IF_HIGH))
				CP_CUR = 0x40;        //0.3m, [6:4]=100
			else if(LO_Freq < (752000+R848_IF_HIGH))
				CP_CUR = 0x00;        //0.7m, [6:4]=000
			else
				CP_CUR = 0x40;        //0.3m, [6:4]=100
		}
		else
		{
			//DTV
			CP_CUR = 0x00;     //0.7m, R25[6:4]=000
			CP_OFFSET = 0x00;  //0u,     [2]=0
		}
	}
	else   //24MHz 
	{	
		//cp cur & offset setting
		if(R848_Standard < R848_ATV_SIZE) //ATV
		{
			u2XalDivJudge = (UINT16) (LO_Freq/1000/12);

			if((u2XalDivJudge==6)||(u2XalDivJudge==10)||(u2XalDivJudge==12))
				SDM_RES = 0x18;    //400R, R27[4:3]=11
			else
				SDM_RES = 0x00;    //short, R27[4:3]=00

			//offset
			if((LO_Freq < (160000+R848_IF_HIGH))||(u2XalDivJudge==16))  //R848:R21[7]  21-8=13  21(0x15) is addr ; [13] is data
			{
				CP_OFFSET = 0x80;  //30u,   [2]=1
			}
			else if ((u2XalDivJudge>=25)&&(u2XalDivJudge<50))
			{
				if((u2XalDivJudge % 2)==1) //odd
					CP_OFFSET = 0x00;  //0u,   [2]=0
				else
					CP_OFFSET = 0x80;  //30u,     [2]=1
			}
			else 
			{
				CP_OFFSET = 0x00;  //0u,     [2]=0
			}

			//CP Current
			if(LO_Freq < (600000+R848_IF_HIGH))
				CP_CUR = ( 0x00 | ( cp_cur_24[u2XalDivJudge] << 4));
			else if(LO_Freq < (744000+R848_IF_HIGH))
				CP_CUR = 0x40;        //0.3m, [6:4]=100
			else if(LO_Freq < (752000+R848_IF_HIGH))
				CP_CUR = 0x00;        //0.7m, [6:4]=000
			else
				CP_CUR = 0x40;        //0.3m, [6:4]=100
		}
		else
		{
			//DTV
			CP_CUR = 0x00;     //0.7m, R25[6:4]=000
			CP_OFFSET = 0x00;  //0u,     [2]=0
		}
	}
	
	//SDM_res
	//SDM_RES = 0x00;    //short, R27[4:3]=00
	R848_I2C.RegAddr = 0x1B;
	R848_Array[19]    = (R848_Array[19] & 0xE7) | SDM_RES; 
	R848_I2C.Data    = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;


	//CP current  R25[6:4]=000
	R848_I2C.RegAddr = 0x19; 
	R848_Array[17]    = (R848_Array[17] & 0x8F)  | CP_CUR ;
	R848_I2C.Data    = R848_Array[17];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;


	//Div Cuurent   R20[7:6]=2'b01(150uA)	
	R848_I2C.RegAddr = 0x14;    
	R848_Array[12]    = (R848_Array[12] & 0x3F)  | 0x40;  
	R848_I2C.Data    = R848_Array[12];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;


	//CPI*2 R28[7]=1
	if((R848_Standard!=R848_DVB_S) && (LO_Freq >= 865000))
	{
		R848_I2C.RegAddr = 0x1C;
		R848_Array[20] = (R848_Array[20] & 0x7F) | 0x80;
		R848_I2C.Data = R848_Array[20];
		if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	}
	else
	{
		R848_I2C.RegAddr = 0x1C;
		R848_Array[20] = (R848_Array[20] & 0x7F);
		R848_I2C.Data = R848_Array[20];
		if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	}

	//  R848:R26[7:5]  VCO_current= 2
	R848_I2C.RegAddr = 0x1A;  
	R848_Array[18]    = (R848_Array[18] & 0x1F) | 0x40; 
	R848_I2C.Data    = R848_Array[18];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;


	//CP Offset R21[7] 
	R848_I2C.RegAddr = 0x15;  
	R848_Array[13]    = (R848_Array[13] & 0x7F) | CP_OFFSET; 
	R848_I2C.Data    = R848_Array[13];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	//set XTAL Power
#if(R848_SHARE_XTAL_OUT==TRUE)
	XTAL_POW1 = 0x00;        //high,     // R848:R23[7]    
	XTAL_POW0 = 0x00;        //highest,  // R848:R23[6:5]
	XTAL_GM = 0x01;          //LARGE(1),         R27[0]=1

	R848_SetXtalIntCap(XTAL_CAP_SMALL);

#else
	if(R848_Initial_done_flag==TRUE)
	{
		//set XTAL Power
		if((R848_Xtal_Pwr<XTAL_SMALL_HIGH) && (LO_Freq > (64000+R848_IF_HIGH)))
		{			
				XTAL_POW1 = 0x00;        //high,       R16[4]=0  //R848:R23[7] 
				XTAL_POW0 = 0x20;        //high,       R15[6:5]=01  //R848:R23[6:5] 
				XTAL_GM = 0x00;            //SMALL(0),   R15[4:3]=00		
		}
		else
		{
			if(R848_Xtal_Pwr <= XTAL_SMALL_HIGHEST)
			{
				XTAL_POW1 = 0x00;        //high,       R16[4]=0	// R848:R23[7]    
				XTAL_POW0 = ((UINT8)(XTAL_SMALL_HIGHEST-R848_Xtal_Pwr)<<5);	//R848:R23[6:5]      
				XTAL_GM = 0x00;            //SMALL(0),   R27[0]=0
			}
			else if(R848_Xtal_Pwr == XTAL_LARGE_HIGHEST)
			{
				XTAL_POW1 = 0x00;        //high,      	// R848:R23[7]  
				XTAL_POW0 = 0x00;        //highest,  	// R848:R23[6:5] 
				XTAL_GM = 0x01;            //LARGE(1),         R27[0]=1
			}
			else
			{
				XTAL_POW1 = 0x00;        //high,     // R848:R23[7]    
				XTAL_POW0 = 0x00;        //highest,  // R848:R23[6:5]
				XTAL_GM = 0x01;            //LARGE(1),         R27[0]=1
			}
		}
	}
	else
	{
		XTAL_POW1 = 0x00;        //high,      	// R848:R23[7]  
		XTAL_POW0 = 0x00;        //highest,  	// R848:R23[6:5] 
		XTAL_GM = 0x01;          //LARGE(1),         R27[0]=1
	}
#endif

	//Xtal_Gm=SMALL(0) R27[0]
	R848_I2C.RegAddr = 0x1B;
	R848_Array[19] = (R848_Array[19] & 0xFE) | XTAL_GM;
	R848_I2C.Data = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	R848_I2C.RegAddr = 0x17;		// R23[6:5]  
	R848_Array[15]    = (R848_Array[15] & 0x9F) | XTAL_POW0; 
	R848_I2C.Data    = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	R848_I2C.RegAddr = 0x17;		// R23[7]   
	R848_Array[15]    = (R848_Array[15] & 0x7F) | XTAL_POW1; 
	R848_I2C.Data    = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	//IQ gen ON 
	R848_I2C.RegAddr = 0x27;		// R39[1]
	R848_Array[31]    = (R848_Array[31] & 0xFD) | 0x00; //[0]=0'b0
	R848_I2C.Data    = R848_Array[31];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	// current:Dmin, Bmin
	R848_I2C.RegAddr = 0x23;		// R848:R35[5:4]=2'b00
	R848_Array[27]    = (R848_Array[27] & 0xCF) | 0x00;
	R848_I2C.Data    = R848_Array[27];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	//set pll autotune = 128kHz (fast)  R23[4:3]=2'b00   
	R848_I2C.RegAddr = 0x17;		
	R848_Array[15]    = R848_Array[15] & 0xE7;
	R848_I2C.Data    = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	//Divider
	while(MixDiv <= 64)
	{
		if(((LO_Freq * MixDiv) >= VCO_Min) && ((LO_Freq * MixDiv) < VCO_Max))
		{
			DivBuf = MixDiv;
			while(DivBuf > 2)
			{
				DivBuf = DivBuf >> 1;
				DivNum ++;
			}			
			break;
		}
		MixDiv = MixDiv << 1;
	}

	//SDM_Res
	/*if(MixDiv <= 4)  //Div=2,4
	{
		SDM_RES = 0x00;    //short, R27[4:3]=00
	}
	else
	{
		SDM_RES = 0x18;   //400R, R27[4:3]=11	  
	}
	SDM_RES = 0x00;    //short, R27[4:3]=00
	R848_I2C.RegAddr = 0x1B;
	R848_Array[19]    = (R848_Array[19] & 0xE7) | SDM_RES; 
	R848_I2C.Data    = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;
	*/

	//Xtal Div
	if(R848_Standard == R848_STD_SIZE) //for cal
	{
		    R848_XtalDiv = XTAL_DIV1;
	        R848_Array[16] = R848_Array[16] & 0xFB; //b2=0  // R848:R24[2]   
	        PLL_Ref = R848_Xtal;
	}
	else	//DTV_Standard
	{
		    u2XalDivJudge = (UINT16) (LO_Freq/1000/8);
			u1XtalDivRemain = (UINT8) (u2XalDivJudge % 2);
		   if(u1XtalDivRemain==1) //odd
		   {
				R848_XtalDiv = XTAL_DIV1;
				R848_Array[16] = R848_Array[16] & 0xFB; //R24[2]=0  
				PLL_Ref = R848_Xtal;
			}
			else  // div2, note that agc clk also div2
			{

			   R848_XtalDiv = XTAL_DIV2;
				R848_Array[16] |= 0x04;   	//R24[2]=1
				PLL_Ref = R848_Xtal / 2;
				
			}	
	}

	R848_I2C.RegAddr = 0x18;	//R848:R24[2] 
	R848_I2C.Data = R848_Array[16];
    if(I2C_Write(&R848_I2C) != RT_Success)
	   return RT_Fail;

/*
	R848_I2C_Len.RegAddr = 0x00;
	R848_I2C_Len.Len = 5;
	if(I2C_Read_Len(&R848_I2C_Len) != RT_Success)
		return RT_Fail;	

   if((R848_I2C_Len.Data[3] & 0xE0) < 0xE0)         //0x60
		DivNum = DivNum + 1;
   else if((R848_I2C_Len.Data[3] & 0xE0) > 0xE0)   //0x60
	    DivNum = DivNum - 1; 
*/	

	//Divider num
	R848_I2C.RegAddr = 0x18; //R24[7:5] 
	R848_Array[16] &= 0x1F;
	R848_Array[16] |= (DivNum << 5);
	R848_I2C.Data = R848_Array[16];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	VCO_Freq = LO_Freq * MixDiv;
	Nint     = (UINT16) (VCO_Freq / 2 / PLL_Ref);
	VCO_Fra  = (UINT16) (VCO_Freq - 2 * PLL_Ref * Nint);

	//Boundary spur prevention
	if (VCO_Fra < PLL_Ref/64)           //2*PLL_Ref/128
		VCO_Fra = 0;
	else if (VCO_Fra > PLL_Ref*127/64)  //2*PLL_Ref*127/128
	{
		VCO_Fra = 0;
		Nint ++;
	}
	else if((VCO_Fra > PLL_Ref*127/128) && (VCO_Fra < PLL_Ref)) //> 2*PLL_Ref*127/256,  < 2*PLL_Ref*128/256
		VCO_Fra = PLL_Ref*127/128;      // VCO_Fra = 2*PLL_Ref*127/256
	else if((VCO_Fra > PLL_Ref) && (VCO_Fra < PLL_Ref*129/128)) //> 2*PLL_Ref*128/256,  < 2*PLL_Ref*129/256
		VCO_Fra = PLL_Ref*129/128;      // VCO_Fra = 2*PLL_Ref*129/256
	else
		VCO_Fra = VCO_Fra;

	//Ni:R848:R28[6:0]   Si:R848:R20[5:4]
	Ni = (Nint - 13) / 4;
	Si = Nint - 4 *Ni - 13;
	//Si
	R848_I2C.RegAddr = 0x14;
	R848_Array[12] = (R848_Array[12] & 0xCF) | ((Si << 4));
	R848_I2C.Data = R848_Array[12];
	if(I2C_Write(&R848_I2C) != RT_Success)
	  return RT_Fail;
	//Ni
	R848_I2C.RegAddr = 0x1C;
	R848_Array[20] = (R848_Array[20] & 0x80) | (Ni);
	R848_I2C.Data = R848_Array[20];
	if(I2C_Write(&R848_I2C) != RT_Success)
	  return RT_Fail;

         	
	//pw_sdm		// R848:R27[7]  
	R848_I2C.RegAddr = 0x1B;
	R848_Array[19] &= 0x7F;
	if(VCO_Fra == 0)
		R848_Array[19] |= 0x80;
	R848_I2C.Data = R848_Array[19];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	//SDM calculator
	while(VCO_Fra > 1)
	{			
		if (VCO_Fra > (2*PLL_Ref / Nsdm))
		{		
			SDM = SDM + 32768 / (Nsdm/2);
			VCO_Fra = VCO_Fra - 2*PLL_Ref / Nsdm;
			if (Nsdm >= 0x8000)
				break;
		}
		Nsdm = Nsdm << 1;
	}

	SDM16to9 = SDM >> 8;
	SDM8to1 =  SDM - (SDM16to9 << 8);

	// R848:R30[7:0]  
	R848_I2C.RegAddr = 0x1E;
	R848_Array[22]    = (UINT8) SDM16to9;
	R848_I2C.Data    = R848_Array[22];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	//R848:R29[7:0] 
	R848_I2C.RegAddr = 0x1D;
	R848_Array[21]    = (UINT8) SDM8to1;
	R848_I2C.Data    = R848_Array[21];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;


	//if(R848_Standard <= R848_SECAM_L1_INV)
	if(R848_XtalDiv == XTAL_DIV2)
	    R848_Delay_MS(20);
	else
	    R848_Delay_MS(10);

	for(VCO_current_trial=0; VCO_current_trial<3; VCO_current_trial++)
	{
		//check PLL lock status 
		R848_I2C_Len.RegAddr = 0x00;
		R848_I2C_Len.Len = 3;
		if(I2C_Read_Len(&R848_I2C_Len) != RT_Success)
		{	
			I2C_Read_Len(&R848_I2C_Len);
		}

		// R848:R26[7:5] 
		if( (R848_I2C_Len.Data[2] & 0x40) == 0x00 ) 
		{
			//Set VCO current = 011 (3)
			R848_I2C.RegAddr = 0x1A;
			//R848_Array[18]    = (R848_Array[18] & 0x1F) | 0x60;  //increase VCO current
			R848_Array[18]    = (R848_Array[18] & 0x1F) | ((2-VCO_current_trial) << 5);  //increase VCO current
			R848_I2C.Data    = R848_Array[18];
			if(I2C_Write(&R848_I2C) != RT_Success)
				return RT_Fail;
		}
	}

	if(VCO_current_trial==2)
	{
		//check PLL lock status 
		R848_I2C_Len.RegAddr = 0x00;
		R848_I2C_Len.Len = 3;
		if(I2C_Read_Len(&R848_I2C_Len) != RT_Success)
			return RT_Fail;

		if( (R848_I2C_Len.Data[2] & 0x40) == 0x00) 
		{
			if(R848_Xtal_Pwr <= XTAL_SMALL_HIGHEST)
				XTAL_GM = 0x01;            //LARGE(1),         R15[4:3]=11

			R848_I2C.RegAddr = 0x1B;
			R848_Array[19] = (R848_Array[19] & 0xFE) | XTAL_GM;
			R848_I2C.Data = R848_Array[19];
			if(I2C_Write(&R848_I2C) != RT_Success)
				return RT_Fail;
		}
	}

	//set pll autotune = 8kHz (slow)
	R848_I2C.RegAddr = 0x17;
	R848_Array[15] = (R848_Array[15] & 0xE7) | 0x10;
	R848_I2C.Data = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
		

	//restore TF, NA det setting
	R848_I2C.RegAddr = 0x16;
	R848_Array[14] = (R848_Array[14] & 0xFE) | u1RfFlag;     
	R848_I2C.Data = R848_Array[14];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	R848_I2C.RegAddr = 0x26;
	R848_Array[30] = (R848_Array[30] & 0x7F) | u1PulseFlag;  
	R848_I2C.Data = R848_Array[30];
	if(I2C_Write(&R848_I2C) != RT_Success)
	    return RT_Fail;

	//Set NA det 710 = OFF
	R848_I2C.RegAddr  = 0x28;								
	R848_Array[32] = (R848_Array[32] & 0xF7) | u1SPulseFlag;
    R848_I2C.Data = R848_Array[32];
    if(I2C_Write(&R848_I2C) != RT_Success)
	   return RT_Fail;

	return RT_Success;

}


R848_ErrCode R848_MUX(UINT32 LO_KHz, UINT32 RF_KHz, R848_Standard_Type R848_Standard)
{	

	//Freq_Info_Type Freq_Info1;
	Freq_Info1 = R848_Freq_Sel(LO_KHz, RF_KHz, R848_Standard);
	UINT8 Reg08_IMR_Gain   = 0;
	UINT8 Reg09_IMR_Phase  = 0;
	UINT8 Reg03_IMR_Iqcap  = 0;

	// LNA band (depend on RF_KHz)
	R848_I2C.RegAddr = 0x0D;	// R13[6:5] 
	R848_Array[5] = (R848_Array[5] & 0x9F) | Freq_Info1.LNA_BAND;
	R848_I2C.Data = R848_Array[5];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// RF Polyfilter
	R848_I2C.RegAddr = 0x21;	// R33[7:6]  
	R848_Array[25] = (R848_Array[25] & 0x3F) | Freq_Info1.RF_POLY;
	R848_I2C.Data = R848_Array[25];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// LNA Cap
	R848_I2C.RegAddr = 0x09;	// R9[7:3]  
	R848_Array[1] = (R848_Array[1] & 0x07) | (Freq_Info1.LPF_CAP<<3);	
	R848_I2C.Data = R848_Array[1];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// LNA Notch
	R848_I2C.RegAddr = 0x0A;	// R10[4:0] 
	R848_Array[2] = (R848_Array[2] & 0xE0) | (Freq_Info1.LPF_NOTCH);	
	R848_I2C.Data = R848_Array[2];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	//Set_IMR
	if(R848_IMR_done_flag == TRUE)
	{
		Reg08_IMR_Gain = R848_IMR_Data[Freq_Info1.IMR_MEM].Gain_X & 0x3F;
		Reg09_IMR_Phase = R848_IMR_Data[Freq_Info1.IMR_MEM].Phase_Y & 0x3F;
		Reg03_IMR_Iqcap = R848_IMR_Data[Freq_Info1.IMR_MEM].Iqcap & 0x03;
	}
	else
	{
		Reg08_IMR_Gain = 0;
	    Reg09_IMR_Phase = 0;
		Reg03_IMR_Iqcap = 0;
	}

	R848_I2C.RegAddr = 0x10; // R16[5:0]            
	R848_Array[8] = (R848_Array[8] & 0xC0) | Reg08_IMR_Gain;
	R848_I2C.Data = R848_Array[8];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	R848_I2C.RegAddr = 0x11; // R17[5:0]  
	R848_Array[9] = (R848_Array[9] & 0xC0) | Reg09_IMR_Phase;
	R848_I2C.Data =R848_Array[9]  ;
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	R848_I2C.RegAddr = 0x0B; // R11[1:0]  
	R848_Array[3] = (R848_Array[3] & 0xFC) | Reg03_IMR_Iqcap;
	R848_I2C.Data =R848_Array[3]  ;
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	return RT_Success;
}


R848_ErrCode R848_SetTF(UINT32 u4FreqKHz, UINT8 u1TfType)
{
    UINT8    u1FreqCount = 0;
	UINT32   u4Freq1 = 0;
	UINT32   u4Freq2 = 0;
	UINT32   u4Ratio;
	UINT8    u1TF_Set_Result1 = 0;
	UINT8    u1TF_Set_Result2 = 0;
	UINT8    u1TF_tmp1, u1TF_tmp2;
	UINT8    u1TFCalNum = R848_TF_HIGH_NUM;

	if((u4FreqKHz>=0) && (u4FreqKHz<R848_LNA_LOW_LOWEST[R848_SetTfType]))  //Ultra Low
	{
		 u1TFCalNum = R848_TF_LOWEST_NUM;
         while((u4FreqKHz < R848_TF_Freq_Lowest[u1TfType][u1FreqCount]) && (u1FreqCount<R848_TF_LOWEST_NUM))
		 {
            u1FreqCount++;
		 }

		 if(u1FreqCount==0)
		 {
			 R848_TF = R848_TF_Result_Lowest[u1TfType][0];
		 }
		 else if(u1FreqCount==R848_TF_LOWEST_NUM)
         {
			 R848_TF = R848_TF_Result_Lowest[u1TfType][R848_TF_LOWEST_NUM-1];
		 }
		 else
		 {
			 u1TF_Set_Result1 = R848_TF_Result_Lowest[u1TfType][u1FreqCount-1]; 
		     u1TF_Set_Result2 = R848_TF_Result_Lowest[u1TfType][u1FreqCount]; 
		     u4Freq1 = R848_TF_Freq_Lowest[u1TfType][u1FreqCount-1];
		     u4Freq2 = R848_TF_Freq_Lowest[u1TfType][u1FreqCount]; 

			 //u4Ratio = (u4Freq1- u4FreqKHz)*100/(u4Freq1 - u4Freq2);
             //R848_TF = u1TF_Set_Result1 + (UINT8)((u1TF_Set_Result2 - u1TF_Set_Result1)*u4Ratio/100);

			 u1TF_tmp1 = ((u1TF_Set_Result1 & 0x40)>>2)*3 + (u1TF_Set_Result1 & 0x3F);  //b6 is 3xb4
			 u1TF_tmp2 = ((u1TF_Set_Result2 & 0x40)>>2)*3 + (u1TF_Set_Result2 & 0x3F);			 
			 u4Ratio = (u4Freq1- u4FreqKHz)*100/(u4Freq1 - u4Freq2);
			 R848_TF = u1TF_tmp1 + (UINT8)((u1TF_tmp2 - u1TF_tmp1)*u4Ratio/100);
			 if(R848_TF>=0x40)
				 R848_TF = (R848_TF + 0x10);

		 }
	}
	else if((u4FreqKHz>=R848_LNA_LOW_LOWEST[R848_SetTfType]) && (u4FreqKHz<R848_LNA_MID_LOW[R848_SetTfType]))  //Low
	{
		 u1TFCalNum = R848_TF_LOW_NUM;
         while((u4FreqKHz < R848_TF_Freq_Low[u1TfType][u1FreqCount]) && (u1FreqCount<R848_TF_LOW_NUM))
		 {
            u1FreqCount++;
		 }

		 if(u1FreqCount==0)
		 {
			 R848_TF = R848_TF_Result_Low[u1TfType][0];
		 }
		 else if(u1FreqCount==R848_TF_LOW_NUM)
        {
			 R848_TF = R848_TF_Result_Low[u1TfType][R848_TF_LOW_NUM-1];
		 }
		 else
		 {
			 u1TF_Set_Result1 = R848_TF_Result_Low[u1TfType][u1FreqCount-1]; 
		     u1TF_Set_Result2 = R848_TF_Result_Low[u1TfType][u1FreqCount]; 
		     u4Freq1 = R848_TF_Freq_Low[u1TfType][u1FreqCount-1];
		     u4Freq2 = R848_TF_Freq_Low[u1TfType][u1FreqCount]; 

			 //u4Ratio = (u4Freq1- u4FreqKHz)*100/(u4Freq1 - u4Freq2);
             //R848_TF = u1TF_Set_Result1 + (UINT8)((u1TF_Set_Result2 - u1TF_Set_Result1)*u4Ratio/100);

			 u1TF_tmp1 = ((u1TF_Set_Result1 & 0x40)>>2) + (u1TF_Set_Result1 & 0x3F);  //b6 is 1xb4
			 u1TF_tmp2 = ((u1TF_Set_Result2 & 0x40)>>2) + (u1TF_Set_Result2 & 0x3F);			 
			 u4Ratio = (u4Freq1- u4FreqKHz)*100/(u4Freq1 - u4Freq2);
			 R848_TF = u1TF_tmp1 + (UINT8)((u1TF_tmp2 - u1TF_tmp1)*u4Ratio/100);
			 if(R848_TF>=0x40)
				 R848_TF = (R848_TF + 0x30);
		 }
	}
	else if((u4FreqKHz>=R848_LNA_MID_LOW[R848_SetTfType]) && (u4FreqKHz<R848_LNA_HIGH_MID[R848_SetTfType]))  //Mid
    {
		 u1TFCalNum = R848_TF_MID_NUM;
         while((u4FreqKHz < R848_TF_Freq_Mid[u1TfType][u1FreqCount]) && (u1FreqCount<R848_TF_MID_NUM))
		 {
            u1FreqCount++;
		 }

		 if(u1FreqCount==0)
		 {
			 R848_TF = R848_TF_Result_Mid[u1TfType][0];
		 }
		 else if(u1FreqCount==R848_TF_MID_NUM)
        {
			 R848_TF = R848_TF_Result_Mid[u1TfType][R848_TF_MID_NUM-1];
		 }
		 else
		 {
			 u1TF_Set_Result1 = R848_TF_Result_Mid[u1TfType][u1FreqCount-1]; 
		     u1TF_Set_Result2 = R848_TF_Result_Mid[u1TfType][u1FreqCount]; 
		     u4Freq1 = R848_TF_Freq_Mid[u1TfType][u1FreqCount-1];
		     u4Freq2 = R848_TF_Freq_Mid[u1TfType][u1FreqCount]; 
			 u4Ratio = (u4Freq1- u4FreqKHz)*100/(u4Freq1 - u4Freq2);
             R848_TF = u1TF_Set_Result1 + (UINT8)((u1TF_Set_Result2 - u1TF_Set_Result1)*u4Ratio/100);
		 }
	}
	else  //HIGH
	{
		 u1TFCalNum = R848_TF_HIGH_NUM;
         while((u4FreqKHz < R848_TF_Freq_High[u1TfType][u1FreqCount]) && (u1FreqCount<R848_TF_HIGH_NUM))
		 {
            u1FreqCount++;
		 }

		 if(u1FreqCount==0)
		 {
			 R848_TF = R848_TF_Result_High[u1TfType][0];
		 }
		 else if(u1FreqCount==R848_TF_HIGH_NUM)
        {
			 R848_TF = R848_TF_Result_High[u1TfType][R848_TF_HIGH_NUM-1];
		 }
		 else
		 {
			 u1TF_Set_Result1 = R848_TF_Result_High[u1TfType][u1FreqCount-1]; 
		     u1TF_Set_Result2 = R848_TF_Result_High[u1TfType][u1FreqCount]; 
		     u4Freq1 = R848_TF_Freq_High[u1TfType][u1FreqCount-1];
		     u4Freq2 = R848_TF_Freq_High[u1TfType][u1FreqCount]; 
			 u4Ratio = (u4Freq1- u4FreqKHz)*100/(u4Freq1 - u4Freq2);
             R848_TF = u1TF_Set_Result1 + (UINT8)((u1TF_Set_Result2 - u1TF_Set_Result1)*u4Ratio/100);
		 }
	}
  
	// R8[6:0] 
	R848_I2C.RegAddr = 0x08;
	R848_Array[0] = (R848_Array[0] & 0x80) | R848_TF;
	R848_I2C.Data = R848_Array[0]  ;
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

    return RT_Success;
}


R848_ErrCode R848_IQ(R848_SectType* IQ_Pont)
{
	R848_SectType Compare_IQ[3];
	UINT8   VGA_Count = 0;
	UINT8   VGA_Read = 0;
	UINT8   X_Direction;  // 1:X, 0:Y
		
	// increase VGA power to let image significant
	for(VGA_Count=11; VGA_Count < 16; VGA_Count ++)
	{
		R848_I2C.RegAddr = 0x14; // R848:R20[3:0]  
		R848_I2C.Data    = (R848_Array[12] & 0xF0) + VGA_Count;  
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

		R848_Delay_MS(10); //
		
		if(R848_Muti_Read(&VGA_Read) != RT_Success)
			return RT_Fail;

		if(VGA_Read > 40*ADC_READ_COUNT)
			break;
	}

	Compare_IQ[0].Gain_X  = R848_Array[8] & 0xC0; // R16[5:0]  
	Compare_IQ[0].Phase_Y = R848_Array[9] & 0xC0; // R17[5:0] 
	//Compare_IQ[0].Iqcap = R848_iniArray[3] & 0xFC;

	    // Determine X or Y
	    if(R848_IMR_Cross(&Compare_IQ[0], &X_Direction) != RT_Success)
			return RT_Fail;

		if(X_Direction==1)
		{
			//compare and find min of 3 points. determine I/Q direction
		    if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
			  return RT_Fail;

		    //increase step to find min value of this direction
		    if(R848_CompreStep(&Compare_IQ[0], 0x10) != RT_Success)  //X
			  return RT_Fail;	
		}
		else
		{
		   //compare and find min of 3 points. determine I/Q direction
		   if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
		   	 return RT_Fail;

		   //increase step to find min value of this direction
		   if(R848_CompreStep(&Compare_IQ[0], 0x11) != RT_Success)  //Y
			 return RT_Fail;	
		}

		//Another direction
		if(X_Direction==1)
		{	    
           if(R848_IQ_Tree(Compare_IQ[0].Gain_X, Compare_IQ[0].Phase_Y, 0x10, &Compare_IQ[0]) != RT_Success) //Y	
		     return RT_Fail;	

		   //compare and find min of 3 points. determine I/Q direction
		   if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
		   	 return RT_Fail;

		   //increase step to find min value of this direction
		   if(R848_CompreStep(&Compare_IQ[0], 0x11) != RT_Success)  //Y
			 return RT_Fail;	
		}
		else
		{
		   if(R848_IQ_Tree(Compare_IQ[0].Phase_Y, Compare_IQ[0].Gain_X, 0x11, &Compare_IQ[0]) != RT_Success) //X
		     return RT_Fail;	
        
		   //compare and find min of 3 points. determine I/Q direction
		   if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
		     return RT_Fail;

	       //increase step to find min value of this direction
		   if(R848_CompreStep(&Compare_IQ[0], 0x10) != RT_Success) //X
		     return RT_Fail;	
		}
		

		//--- Check 3 points again---//
		if(X_Direction==1)
		{
		    if(R848_IQ_Tree(Compare_IQ[0].Phase_Y, Compare_IQ[0].Gain_X, 0x11, &Compare_IQ[0]) != RT_Success) //X
			  return RT_Fail;	
		}
		else
		{
		   if(R848_IQ_Tree(Compare_IQ[0].Gain_X, Compare_IQ[0].Phase_Y, 0x10, &Compare_IQ[0]) != RT_Success) //Y
			return RT_Fail;		
		}

		if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
			return RT_Fail;

    //Section-9 check
    //if(R848_F_IMR(&Compare_IQ[0]) != RT_Success)
	if(R848_Section(&Compare_IQ[0]) != RT_Success)
			return RT_Fail;

	//clear IQ_Cap = 0   //  R11[1:0]  
	Compare_IQ[0].Iqcap = R848_Array[3] & 0xFC;

	if(R848_IMR_Iqcap(&Compare_IQ[0]) != RT_Success)
			return RT_Fail;

	*IQ_Pont = Compare_IQ[0];

	//reset gain/phase/iqcap control setting
	R848_I2C.RegAddr = 0x10;	// R16[5:0]  
	R848_Array[8] = R848_Array[8] & 0xC0;
	R848_I2C.Data = R848_Array[8];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	R848_I2C.RegAddr = 0x11;	// R17[5:0]  
	R848_Array[9] = R848_Array[9] & 0xC0;
	R848_I2C.Data = R848_Array[9];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	R848_I2C.RegAddr = 0x0B;	//  R11[1:0] 
	R848_Array[3] = R848_Array[3] & 0xFC;
	R848_I2C.Data = R848_Array[3];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	return RT_Success;
}



//--------------------------------------------------------------------------------------------
// Purpose: record IMC results by input gain/phase location
//          then adjust gain or phase positive 1 step and negtive 1 step, both record results
// input: FixPot: phase or gain
//        FlucPot phase or gain
//        PotReg: 0x10 or 0x11 for R848
//        CompareTree: 3 IMR trace and results
// output: TREU or FALSE
//--------------------------------------------------------------------------------------------

//20131217-Ryan


R848_ErrCode R848_IQ_Tree(UINT8 FixPot, UINT8 FlucPot, UINT8 PotReg, R848_SectType* CompareTree)
{
	UINT8 TreeCunt  = 0;
	UINT8 TreeTimes = 3;
	UINT8 PntReg    = 0;

	if(PotReg == 0x10)
		PntReg = 0x11; //phase control
	else
		PntReg = 0x10; //gain control

	for(TreeCunt = 0;TreeCunt < TreeTimes;TreeCunt ++)
	{
		R848_I2C.RegAddr = PotReg;
		R848_I2C.Data    = FixPot;
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

		R848_I2C.RegAddr = PntReg;
		R848_I2C.Data    = FlucPot;
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

		if(R848_Muti_Read(&CompareTree[TreeCunt].Value) != RT_Success)
			return RT_Fail;
	

		if(PotReg == 0x10)
		{
			CompareTree[TreeCunt].Gain_X  = FixPot;
			CompareTree[TreeCunt].Phase_Y = FlucPot;
		}
		else
		{
			CompareTree[TreeCunt].Phase_Y  = FixPot;
			CompareTree[TreeCunt].Gain_X = FlucPot;
		}
		
		if(TreeCunt == 0)   //try right-side point
			FlucPot ++; 
		else if(TreeCunt == 1) //try left-side point
		{
			if((FlucPot & 0x1F) == 1) //if absolute location is 1, change I/Q direction
			{
				if(FlucPot & 0x20) //b[5]:I/Q selection. 0:Q-path, 1:I-path
				{
					FlucPot = (FlucPot & 0xC0) | 0x01;			
				}
				else
				{
					FlucPot = (FlucPot & 0xC0) | 0x21;
				}
			}
			else
				FlucPot = FlucPot - 2;  
				
		}
	}

	return RT_Success;
}




//-----------------------------------------------------------------------------------/ 
// Purpose: compare IMC result aray [0][1][2], find min value and store to CorArry[0]
// input: CorArry: three IMR data array
// output: TRUE or FALSE
//-----------------------------------------------------------------------------------/
R848_ErrCode R848_CompreCor(R848_SectType* CorArry)
{
	UINT8 CompCunt = 0;
	R848_SectType CorTemp;

	for(CompCunt = 3;CompCunt > 0;CompCunt --)
	{
		if(CorArry[0].Value > CorArry[CompCunt - 1].Value) //compare IMC result [0][1][2], find min value
		{
			CorTemp = CorArry[0];
			CorArry[0] = CorArry[CompCunt - 1];
			CorArry[CompCunt - 1] = CorTemp;
		}
	}

	return RT_Success;
}


//-------------------------------------------------------------------------------------//
// Purpose: if (Gain<9 or Phase<9), Gain+1 or Phase+1 and compare with min value
//          new < min => update to min and continue
//          new > min => Exit
// input: StepArry: three IMR data array
//        Pace: gain or phase register
// output: TRUE or FALSE 
//-------------------------------------------------------------------------------------//
R848_ErrCode R848_CompreStep(R848_SectType* StepArry, UINT8 Pace)
{
	R848_SectType StepTemp;
	
	//min value already saved in StepArry[0]
	StepTemp.Phase_Y = StepArry[0].Phase_Y;
	StepTemp.Gain_X  = StepArry[0].Gain_X;
	//StepTemp.Iqcap  = StepArry[0].Iqcap;

	while(((StepTemp.Gain_X & 0x1F) < R848_IMR_TRIAL) && ((StepTemp.Phase_Y & 0x1F) < R848_IMR_TRIAL))  
	{
		if(Pace == 0x10)	
			StepTemp.Gain_X ++;
		else
			StepTemp.Phase_Y ++;
	
		R848_I2C.RegAddr = 0x10;	
		R848_I2C.Data    = StepTemp.Gain_X ;
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

		R848_I2C.RegAddr = 0x11;	
		R848_I2C.Data    = StepTemp.Phase_Y;
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

		if(R848_Muti_Read(&StepTemp.Value) != RT_Success)
			return RT_Fail;

		if(StepTemp.Value <= StepArry[0].Value)
		{
			StepArry[0].Gain_X  = StepTemp.Gain_X;
			StepArry[0].Phase_Y = StepTemp.Phase_Y;
			//StepArry[0].Iqcap = StepTemp.Iqcap;
			StepArry[0].Value   = StepTemp.Value;
		}
		else if((StepTemp.Value - 2*ADC_READ_COUNT) > StepArry[0].Value)
		{
			break;		
		}
		
	} //end of while()
	
	return RT_Success;
}


//-----------------------------------------------------------------------------------/ 
// Purpose: read multiple IMC results for stability
// input: IMR_Reg: IMC result address
//        IMR_Result_Data: result 
// output: TRUE or FALSE
//-----------------------------------------------------------------------------------/
R848_ErrCode R848_Muti_Read(UINT8* IMR_Result_Data)
{
	UINT8 ReadCunt     = 0;
	UINT16 ReadAmount  = 0;
	UINT8 ReadMax = 0;
	UINT8 ReadMin = 255;
	UINT8 ReadData = 0;
	
    R848_Delay_MS(ADC_READ_DELAY);//3

	for(ReadCunt = 0; ReadCunt < (ADC_READ_COUNT+2); ReadCunt ++)
	{
		R848_I2C_Len.RegAddr = 0x00;
		R848_I2C_Len.Len = 2;              // read 0x01
		if(I2C_Read_Len(&R848_I2C_Len) != RT_Success)
		{
			I2C_Read_Len(&R848_I2C_Len);
		}

		ReadData = (R848_I2C_Len.Data[1] & 0x3F);
		
		ReadAmount = ReadAmount + (UINT16)ReadData;
		
		if(ReadData < ReadMin)
			ReadMin = ReadData;
		
        if(ReadData > ReadMax)
			ReadMax = ReadData;
	}
	*IMR_Result_Data = (UINT8) (ReadAmount - (UINT16)ReadMax - (UINT16)ReadMin);


	return RT_Success;
}


R848_ErrCode R848_Section(R848_SectType* IQ_Pont)
{
	R848_SectType Compare_IQ[3];
	R848_SectType Compare_Bet[3];

	//Try X-1 column and save min result to Compare_Bet[0]
	if((IQ_Pont->Gain_X & 0x1F) == 0x00)
	{
		Compare_IQ[0].Gain_X = ((IQ_Pont->Gain_X) & 0xDF) + 1;  //Q-path, Gain=1
	}
	else
	{
		Compare_IQ[0].Gain_X  = IQ_Pont->Gain_X - 1;  //left point
	}
	Compare_IQ[0].Phase_Y = IQ_Pont->Phase_Y;

	if(R848_IQ_Tree(Compare_IQ[0].Gain_X, Compare_IQ[0].Phase_Y, 0x10, &Compare_IQ[0]) != RT_Success)  // y-direction
		return RT_Fail;		

	if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
		return RT_Fail;

	Compare_Bet[0].Gain_X = Compare_IQ[0].Gain_X;
	Compare_Bet[0].Phase_Y = Compare_IQ[0].Phase_Y;
	Compare_Bet[0].Value = Compare_IQ[0].Value;

	//Try X column and save min result to Compare_Bet[1]
	Compare_IQ[0].Gain_X = IQ_Pont->Gain_X;
	Compare_IQ[0].Phase_Y = IQ_Pont->Phase_Y;

	if(R848_IQ_Tree(Compare_IQ[0].Gain_X, Compare_IQ[0].Phase_Y, 0x10, &Compare_IQ[0]) != RT_Success)
		return RT_Fail;	

	if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
		return RT_Fail;

	Compare_Bet[1].Gain_X = Compare_IQ[0].Gain_X;
	Compare_Bet[1].Phase_Y = Compare_IQ[0].Phase_Y;
	Compare_Bet[1].Value = Compare_IQ[0].Value;

	//Try X+1 column and save min result to Compare_Bet[2]
	if((IQ_Pont->Gain_X & 0x1F) == 0x00)		
		Compare_IQ[0].Gain_X = ((IQ_Pont->Gain_X) | 0x20) + 1;  //I-path, Gain=1
	else
	    Compare_IQ[0].Gain_X = IQ_Pont->Gain_X + 1;
	Compare_IQ[0].Phase_Y = IQ_Pont->Phase_Y;

	if(R848_IQ_Tree(Compare_IQ[0].Gain_X, Compare_IQ[0].Phase_Y, 0x10, &Compare_IQ[0]) != RT_Success)
		return RT_Fail;		

	if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
		return RT_Fail;

	Compare_Bet[2].Gain_X = Compare_IQ[0].Gain_X;
	Compare_Bet[2].Phase_Y = Compare_IQ[0].Phase_Y;
	Compare_Bet[2].Value = Compare_IQ[0].Value;

	if(R848_CompreCor(&Compare_Bet[0]) != RT_Success)
		return RT_Fail;

	*IQ_Pont = Compare_Bet[0];
	
	return RT_Success;
}


R848_ErrCode R848_IMR_Cross(R848_SectType* IQ_Pont, UINT8* X_Direct)
{

	R848_SectType Compare_Cross[9]; //(0,0)(0,Q-1)(0,I-1)(Q-1,0)(I-1,0)+(0,Q-2)(0,I-2)(Q-2,0)(I-2,0)
	R848_SectType Compare_Temp;
	UINT8 CrossCount = 0;
	UINT8 Reg16 = R848_Array[8] & 0xC0;	
	UINT8 Reg17 = R848_Array[9] & 0xC0;	

	//memset(&Compare_Temp,0, sizeof(R848_SectType));
	Compare_Temp.Value = 255;

	for(CrossCount=0; CrossCount<9; CrossCount++)
	{

		if(CrossCount==0)
		{
		  Compare_Cross[CrossCount].Gain_X = Reg16;
		  Compare_Cross[CrossCount].Phase_Y = Reg17;
		}
		else if(CrossCount==1)
		{
		  Compare_Cross[CrossCount].Gain_X = Reg16;       //0
		  Compare_Cross[CrossCount].Phase_Y = Reg17 + 1;  //Q-1
		}
		else if(CrossCount==2)
		{
		  Compare_Cross[CrossCount].Gain_X = Reg16;               //0
		  Compare_Cross[CrossCount].Phase_Y = (Reg17 | 0x20) + 1; //I-1
		}
		else if(CrossCount==3)
		{
		  Compare_Cross[CrossCount].Gain_X = Reg16 + 1; //Q-1
		  Compare_Cross[CrossCount].Phase_Y = Reg17;
		}
		else if(CrossCount==4)
		{
		  Compare_Cross[CrossCount].Gain_X = (Reg16 | 0x20) + 1; //I-1
		  Compare_Cross[CrossCount].Phase_Y = Reg17;
		}
		else if(CrossCount==5)
		{
		  Compare_Cross[CrossCount].Gain_X = Reg16;       //0
		  Compare_Cross[CrossCount].Phase_Y = Reg17 + 2;  //Q-2
		}
		else if(CrossCount==6)
		{
		  Compare_Cross[CrossCount].Gain_X = Reg16;               //0
		  Compare_Cross[CrossCount].Phase_Y = (Reg17 | 0x20) + 2; //I-2
		}
		else if(CrossCount==7)
		{
		  Compare_Cross[CrossCount].Gain_X = Reg16 + 2; //Q-2
		  Compare_Cross[CrossCount].Phase_Y = Reg17;
		}
		else if(CrossCount==8)
		{
		  Compare_Cross[CrossCount].Gain_X = (Reg16 | 0x20) + 2; //I-2
		  Compare_Cross[CrossCount].Phase_Y = Reg17;
		}

    	R848_I2C.RegAddr = 0x10;	
	    R848_I2C.Data    = Compare_Cross[CrossCount].Gain_X;
	    if(I2C_Write(&R848_I2C) != RT_Success)
		   return RT_Fail;

	    R848_I2C.RegAddr = 0x11;	
	    R848_I2C.Data    = Compare_Cross[CrossCount].Phase_Y;
	    if(I2C_Write(&R848_I2C) != RT_Success)
		  return RT_Fail;
	
        if(R848_Muti_Read(&Compare_Cross[CrossCount].Value) != RT_Success)
		  return RT_Fail;

		if( Compare_Cross[CrossCount].Value < Compare_Temp.Value)
		{
		  Compare_Temp.Value = Compare_Cross[CrossCount].Value;
		  Compare_Temp.Gain_X = Compare_Cross[CrossCount].Gain_X;
		  Compare_Temp.Phase_Y = Compare_Cross[CrossCount].Phase_Y;		
		}
	} //end for loop


    if(((Compare_Temp.Phase_Y & 0x3F)==0x01) || (Compare_Temp.Phase_Y & 0x3F)==0x02)  //phase Q1 or Q2
	{
      *X_Direct = (UINT8) 0;
	  IQ_Pont[0].Gain_X = Compare_Cross[0].Gain_X;    //0
	  IQ_Pont[0].Phase_Y = Compare_Cross[0].Phase_Y; //0
	  IQ_Pont[0].Value = Compare_Cross[0].Value;

	  IQ_Pont[1].Gain_X = Compare_Cross[1].Gain_X;    //0
	  IQ_Pont[1].Phase_Y = Compare_Cross[1].Phase_Y; //Q1
	  IQ_Pont[1].Value = Compare_Cross[1].Value;

	  IQ_Pont[2].Gain_X = Compare_Cross[5].Gain_X;   //0
	  IQ_Pont[2].Phase_Y = Compare_Cross[5].Phase_Y;//Q2
	  IQ_Pont[2].Value = Compare_Cross[5].Value;
	}
	else if(((Compare_Temp.Phase_Y & 0x3F)==0x21) || (Compare_Temp.Phase_Y & 0x3F)==0x22)  //phase I1 or I2
	{
      *X_Direct = (UINT8) 0;
	  IQ_Pont[0].Gain_X = Compare_Cross[0].Gain_X;    //0
	  IQ_Pont[0].Phase_Y = Compare_Cross[0].Phase_Y; //0
	  IQ_Pont[0].Value = Compare_Cross[0].Value;

	  IQ_Pont[1].Gain_X = Compare_Cross[2].Gain_X;    //0
	  IQ_Pont[1].Phase_Y = Compare_Cross[2].Phase_Y; //Q1
	  IQ_Pont[1].Value = Compare_Cross[2].Value;

	  IQ_Pont[2].Gain_X = Compare_Cross[6].Gain_X;   //0
	  IQ_Pont[2].Phase_Y = Compare_Cross[6].Phase_Y;//Q2
	  IQ_Pont[2].Value = Compare_Cross[6].Value;
	}
	else if(((Compare_Temp.Gain_X & 0x3F)==0x01) || (Compare_Temp.Gain_X & 0x3F)==0x02)  //gain Q1 or Q2
	{
      *X_Direct = (UINT8) 1;
	  IQ_Pont[0].Gain_X = Compare_Cross[0].Gain_X;    //0
	  IQ_Pont[0].Phase_Y = Compare_Cross[0].Phase_Y; //0
	  IQ_Pont[0].Value = Compare_Cross[0].Value;

	  IQ_Pont[1].Gain_X = Compare_Cross[3].Gain_X;    //Q1
	  IQ_Pont[1].Phase_Y = Compare_Cross[3].Phase_Y; //0
	  IQ_Pont[1].Value = Compare_Cross[3].Value;

	  IQ_Pont[2].Gain_X = Compare_Cross[7].Gain_X;   //Q2
	  IQ_Pont[2].Phase_Y = Compare_Cross[7].Phase_Y;//0
	  IQ_Pont[2].Value = Compare_Cross[7].Value;
	}
	else if(((Compare_Temp.Gain_X & 0x3F)==0x21) || (Compare_Temp.Gain_X & 0x3F)==0x22)  //gain I1 or I2
	{
      *X_Direct = (UINT8) 1;
	  IQ_Pont[0].Gain_X = Compare_Cross[0].Gain_X;    //0
	  IQ_Pont[0].Phase_Y = Compare_Cross[0].Phase_Y; //0
	  IQ_Pont[0].Value = Compare_Cross[0].Value;

	  IQ_Pont[1].Gain_X = Compare_Cross[4].Gain_X;    //I1
	  IQ_Pont[1].Phase_Y = Compare_Cross[4].Phase_Y; //0
	  IQ_Pont[1].Value = Compare_Cross[4].Value;

	  IQ_Pont[2].Gain_X = Compare_Cross[8].Gain_X;   //I2
	  IQ_Pont[2].Phase_Y = Compare_Cross[8].Phase_Y;//0
	  IQ_Pont[2].Value = Compare_Cross[8].Value;
	}
	else //(0,0) 
	{	
	  *X_Direct = (UINT8) 1;
	  IQ_Pont[0].Gain_X = Compare_Cross[0].Gain_X;
	  IQ_Pont[0].Phase_Y = Compare_Cross[0].Phase_Y;
	  IQ_Pont[0].Value = Compare_Cross[0].Value;

	  IQ_Pont[1].Gain_X = Compare_Cross[3].Gain_X;    //Q1
	  IQ_Pont[1].Phase_Y = Compare_Cross[3].Phase_Y; //0
	  IQ_Pont[1].Value = Compare_Cross[3].Value;

	  IQ_Pont[2].Gain_X = Compare_Cross[4].Gain_X;   //I1
	  IQ_Pont[2].Phase_Y = Compare_Cross[4].Phase_Y; //0
	  IQ_Pont[2].Value = Compare_Cross[4].Value;
	}
	return RT_Success;
}


//----------------------------------------------------------------------------------------//
// purpose: search surrounding points from previous point 
//          try (x-1), (x), (x+1) columns, and find min IMR result point
// input: IQ_Pont: previous point data(IMR Gain, Phase, ADC Result, RefRreq)
//                 will be updated to final best point                 
// output: TRUE or FALSE
//----------------------------------------------------------------------------------------//
R848_ErrCode R848_F_IMR(R848_SectType* IQ_Pont)
{
	R848_SectType Compare_IQ[3];
	R848_SectType Compare_Bet[3];
	UINT8 VGA_Count = 0;
	UINT8 VGA_Read = 0;

	//VGA
	for(VGA_Count=11; VGA_Count < 16; VGA_Count ++)
	{
		R848_I2C.RegAddr = 0x14;	//  R20[3:0]  
        R848_I2C.Data    = (R848_Array[12] & 0xF0) + VGA_Count;
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

		R848_Delay_MS(10);
		
		if(R848_Muti_Read(&VGA_Read) != RT_Success)
			return RT_Fail;

		if(VGA_Read > 40*ADC_READ_COUNT)
		break;
	}

	//Try X-1 column and save min result to Compare_Bet[0]
	if((IQ_Pont->Gain_X & 0x1F) == 0x00)
	{
		Compare_IQ[0].Gain_X = ((IQ_Pont->Gain_X) & 0xDF) + 1;  //Q-path, Gain=1
	}
	else
	{
		Compare_IQ[0].Gain_X  = IQ_Pont->Gain_X - 1;  //left point
	}
	Compare_IQ[0].Phase_Y = IQ_Pont->Phase_Y;

	if(R848_IQ_Tree(Compare_IQ[0].Gain_X, Compare_IQ[0].Phase_Y, 0x10, &Compare_IQ[0]) != RT_Success)  // y-direction
		return RT_Fail;	

	if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
		return RT_Fail;

	Compare_Bet[0].Gain_X = Compare_IQ[0].Gain_X;
	Compare_Bet[0].Phase_Y = Compare_IQ[0].Phase_Y;
	Compare_Bet[0].Value = Compare_IQ[0].Value;

	//Try X column and save min result to Compare_Bet[1]
	Compare_IQ[0].Gain_X = IQ_Pont->Gain_X;
	Compare_IQ[0].Phase_Y = IQ_Pont->Phase_Y;

	if(R848_IQ_Tree(Compare_IQ[0].Gain_X, Compare_IQ[0].Phase_Y, 0x10, &Compare_IQ[0]) != RT_Success)
		return RT_Fail;	

	if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
		return RT_Fail;

	Compare_Bet[1].Gain_X = Compare_IQ[0].Gain_X;
	Compare_Bet[1].Phase_Y = Compare_IQ[0].Phase_Y;
	Compare_Bet[1].Value = Compare_IQ[0].Value;

	//Try X+1 column and save min result to Compare_Bet[2]
	if((IQ_Pont->Gain_X & 0x1F) == 0x00)		
		Compare_IQ[0].Gain_X = ((IQ_Pont->Gain_X) | 0x20) + 1;  //I-path, Gain=1
	else
	    Compare_IQ[0].Gain_X = IQ_Pont->Gain_X + 1;
	Compare_IQ[0].Phase_Y = IQ_Pont->Phase_Y;

	if(R848_IQ_Tree(Compare_IQ[0].Gain_X, Compare_IQ[0].Phase_Y, 0x10, &Compare_IQ[0]) != RT_Success)
		return RT_Fail;		

	if(R848_CompreCor(&Compare_IQ[0]) != RT_Success)
		return RT_Fail;

	Compare_Bet[2].Gain_X = Compare_IQ[0].Gain_X;
	Compare_Bet[2].Phase_Y = Compare_IQ[0].Phase_Y;
	Compare_Bet[2].Value = Compare_IQ[0].Value;

	if(R848_CompreCor(&Compare_Bet[0]) != RT_Success)
		return RT_Fail;

	//clear IQ_Cap = 0
	Compare_Bet[0].Iqcap = R848_Array[3] & 0xFC;	//  R11[1:0] 

	if(R848_IMR_Iqcap(&Compare_Bet[0]) != RT_Success)
		return RT_Fail;

	*IQ_Pont = Compare_Bet[0];
	
	return RT_Success;
}


R848_ErrCode R848_IMR_Iqcap(R848_SectType* IQ_Point)   
{
    R848_SectType Compare_Temp;
	int i = 0;

	//Set Gain/Phase to right setting
	R848_I2C.RegAddr = 0x10;	// R16[5:0]  
	R848_I2C.Data = IQ_Point->Gain_X; 
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	R848_I2C.RegAddr = 0x11;	// R17[5:0]  
	R848_I2C.Data = IQ_Point->Phase_Y;
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	//try iqcap
	for(i=0; i<3; i++)	
	{
		Compare_Temp.Iqcap = (UINT8) i;  
		R848_I2C.RegAddr = 0x0B;		// R11[1:0] 
		R848_Array[3] = (R848_Array[3] & 0xFC) | Compare_Temp.Iqcap;  
		R848_I2C.Data = R848_Array[3];  
		if(I2C_Write(&R848_I2C) != RT_Success)
			   return RT_Fail;

		if(R848_Muti_Read(&(Compare_Temp.Value)) != RT_Success)
			   return RT_Fail;

		if(Compare_Temp.Value < IQ_Point->Value)
		{
			IQ_Point->Value = Compare_Temp.Value; 
			IQ_Point->Iqcap = Compare_Temp.Iqcap;
		}
	}

    return RT_Success;
}


R848_ErrCode R848_GPO(R848_GPO_Type R848_GPO_Conrl)
{
	if(R848_GPO_Conrl == HI_SIG)	//  R23[0]  
		R848_Array[15] |= 0x01;   //high
	else
		R848_Array[15] &= 0xFE;  //low
	R848_I2C.RegAddr = 0x17;
	R848_I2C.Data = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	return RT_Success;
}


R848_ErrCode R848_SetStandard(R848_Standard_Type RT_Standard)
{	 
	//printf("RT_Standard = %d\n",RT_Standard);
	if(RT_Standard != R848_pre_standard)
	{
		 if(R848_InitReg(RT_Standard) != RT_Success)      
		     return RT_Fail;
	}
    R848_pre_standard = RT_Standard;


	Sys_Info1 = R848_Sys_Sel(RT_Standard);

	// Filter Calibration
	UINT8 u1FilCalGap = 8;

	if(RT_Standard<R848_ATV_SIZE)    //ATV
	    u1FilCalGap = R848_Fil_Cal_Gap;
	else
	    u1FilCalGap = 8;

    if(R848_Fil_Cal_flag[RT_Standard] == FALSE)
	{
		R848_Fil_Cal_code[RT_Standard] = R848_Filt_Cal_ADC(Sys_Info1.FILT_CAL_IF, Sys_Info1.BW, u1FilCalGap);
		R848_Fil_Cal_BW[RT_Standard] = R848_Bandwidth;
        R848_Fil_Cal_flag[RT_Standard] = TRUE;

	    //Reset register and Array 
	    if(R848_InitReg(RT_Standard) != RT_Success)        
		   return RT_Fail;
	}

	// Set Filter Auto Ext
	R848_I2C.RegAddr = 0x13;	// R19[4]  
	R848_Array[11] = (R848_Array[11] & 0xEF) | Sys_Info1.FILT_EXT_ENA;  
	R848_I2C.Data = R848_Array[11];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;


	if(Sys_Info1.FILT_EXT_ENA==0x10)  //(%)
	{
		
			 if(R848_Fil_Cal_code[RT_Standard]< 2)  
			   R848_Fil_Cal_code[RT_Standard] = 2;
		
			 if((Sys_Info1.FILT_EXT_POINT & 0x02)==0x02)  //HPF+3
			 {
				  if(Sys_Info1.HPF_COR>12)  
				  {    Sys_Info1.HPF_COR = 12; }
			 }
			 else  //HPF+1
			 {
				  if(Sys_Info1.HPF_COR>14)  
				  {    Sys_Info1.HPF_COR = 15; }		 
			 }		  			
	}


	// Set LPF fine code
	R848_I2C.RegAddr = 0x12;	//  R848:R18[3:0]  
	R848_Array[10] = (R848_Array[10] & 0xF0) | R848_Fil_Cal_code[RT_Standard];  
	R848_I2C.Data = R848_Array[10];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// Set LPF coarse BW
	R848_I2C.RegAddr = 0x13;	// R848:R19[6:5]  
	R848_Array[11] = (R848_Array[11] & 0x9F) | R848_Fil_Cal_BW[RT_Standard];
	R848_I2C.Data = R848_Array[11];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	
	// Set HPF corner & 1.7M mode
	R848_I2C.RegAddr = 0x13;	//R848:R19[7 & 3:0]  
	R848_Array[11] = (R848_Array[11] & 0x70) | Sys_Info1.HPF_COR | Sys_Info1.V17M;
	R848_I2C.Data = R848_Array[11];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// Set TF current 
	R848_I2C.RegAddr = 0x0B;	//  R848:R11[6]  
	R848_Array[3] = (R848_Array[3] & 0xBF) | Sys_Info1.TF_CUR;  
	R848_I2C.Data = R848_Array[3];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// Set Filter current 
	R848_I2C.RegAddr = 0x12;	//  R848:R18[6:5]  
	R848_Array[10] = (R848_Array[10] & 0x9F) | Sys_Info1.FILT_CUR;  
	//R848_Array[10] = (R848_Array[10] & 0x9F) | 0x60;   //lowest
	R848_I2C.Data = R848_Array[10];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// Set Switch Buffer current 
	R848_I2C.RegAddr = 0x0C;	//  R848:R12[2] 
	R848_Array[4] = (R848_Array[4] & 0xFB) | Sys_Info1.SWBUF_CUR;   
	R848_I2C.Data = R848_Array[4];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// Set Filter Comp 
	R848_I2C.RegAddr = 0x26;	//  R848:R38[6:5]  
	R848_Array[30] = (R848_Array[30] & 0x9F) | Sys_Info1.FILT_COMP;  
	R848_I2C.Data = R848_Array[30];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

   // Set Filter 3dB
	R848_I2C.RegAddr = 0x26;	// R848:R38[7]  
	R848_Array[30] = (R848_Array[30] & 0xF7) | Sys_Info1.FILT_3DB;  
	R848_I2C.Data = R848_Array[30];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// Set Filter Ext Condition (%)
	R848_I2C.RegAddr = 0x26;	//  R848:R38[2:0] 
    R848_Array[30] = (R848_Array[30] & 0xF8) | 0x04 | Sys_Info1.FILT_EXT_POINT;   //ext both HPF/LPF
	R848_I2C.Data = R848_Array[30];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
/*
	// Set Inductor Bias
	R848_I2C.RegAddr = 0x04;
	R848_Array[4] = (R848_Array[4] & 0xFE) | Sys_Info1.INDUC_BIAS; 
	R848_I2C.Data = R848_Array[4];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
*/
	// Set sw cap clk
	R848_I2C.RegAddr = 0x1A;	//  R848:R26[1:0]  
	R848_Array[18] = (R848_Array[18] & 0xFC) | Sys_Info1.SWCAP_CLK; 
	R848_I2C.Data = R848_Array[18];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	// Set NA det power
	R848_I2C.RegAddr = 0x26;	// R848:R38[7] 
	R848_Array[30] = (R848_Array[30] & 0x7F) | Sys_Info1.NA_PWR_DET; 
	R848_I2C.Data = R848_Array[30];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

/*
	// Set AGC clk 
	R848_I2C.RegAddr = 0x1A;	//  R848:R26[4:2] 
	R848_Array[18] = (R848_Array[18] & 0xE3) | Sys_Info1.AGC_CLK;  
	R848_I2C.Data = R848_Array[18];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
*/

	//Set GPO High
	R848_I2C.RegAddr = 0x17;	// R848:R23[4:2]  
	R848_Array[15] = (R848_Array[15] & 0xFE) | 0x01;
	R848_I2C.Data = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
	return RT_Fail;


	return RT_Success;
}


UINT8  R848_Filt_Cal_ADC(UINT32 IF_Freq, UINT8 R848_BW, UINT8 FilCal_Gap)
{
	 UINT8     u1FilterCodeResult = 0;
	 UINT8     u1FilterCode = 0;
	 UINT32   u4RingFreq = 72000;
	 UINT8     u1FilterCalValue = 0;
	 UINT8     u1FilterCalValuePre = 0;
	 UINT8     initial_cnt = 0;
	 UINT8     i = 0;
	 UINT32   RingVCO = 0;
	 UINT32   RingRef = R848_Xtal;
	 UINT8     divnum_ring = 0;

	
	if(R848_Xtal==16000)  //16M
	{
         divnum_ring = 11;
	}
	else  //24M
	{
		 divnum_ring = 2;
	}
	 RingVCO = (16+divnum_ring)* 8 * RingRef;
	 u4RingFreq = RingVCO/48;



	 R848_Standard_Type	R848_Standard;
	 R848_Standard=R848_ATSC; //no set R848_DVB_S

	 //Write initial reg before doing calibration 
	 if(R848_InitReg(R848_Standard) != RT_Success)        
		return RT_Fail;

	 if(R848_Cal_Prepare(R848_LPF_CAL) != RT_Success)      
	      return RT_Fail;


	 //Set Ring PLL(72MHz)	 	
	 /*R848_I2C.RegAddr = 0x18;	//  R848:R22[7:2]  
	 R848_Array[24] = (R848_Array[24] & 0x00) | 0x8B;  //div=11, pre div=6
	 R848_I2C.Data = R848_Array[24];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;*/

	 R848_I2C.RegAddr = 0x27;	//  R848:R39[5:2]  //div
	 R848_Array[31] = (R848_Array[31] & 0xC3) | (divnum_ring<<2);  
	 R848_I2C.Data = R848_Array[31];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	 R848_I2C.RegAddr = 0x12;	//R848:R18[4]  
	 R848_Array[10] = (R848_Array[10] & 0xEF) | 0x00; 
	 R848_I2C.Data = R848_Array[10];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	 R848_I2C.RegAddr = 0x25;	//  R848:R37[7]  
	 R848_Array[29] = (R848_Array[29] & 0x7F) | 0x00; 
	 R848_I2C.Data = R848_Array[29];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	
	 R848_I2C.RegAddr = 0x27;	//  R848:R39[7:6]  
	 R848_Array[31] = (R848_Array[31] & 0x3F) | 0x80;  
	 R848_I2C.Data = R848_Array[31];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	 R848_Array[25] = (R848_Array[25] & 0x00) | 0x8B;   //out div=8, RF poly=low band, power=min_lp
	 if(RingVCO>=3200000)
	{
		R848_Array[25] &= 0xDF;   //clear vco_band, R25[5]		//R848:R33[5]    33-8=25  33(0x21) is addr ; [25] is data 
	}
	else
	{
		R848_Array[25] |= 0x20;   //clear vco_band, R25[5]		//R848:R33[5]    33-8=25  33(0x21) is addr ; [25] is data 
	}
	 R848_I2C.RegAddr = 0x21;	//  R848:R33[7:0]  
	 R848_I2C.Data = R848_Array[25];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;	

     //Must do before PLL() 
	 if(R848_MUX(u4RingFreq + IF_Freq, u4RingFreq, R848_STD_SIZE) != RT_Success)     //FilCal MUX (LO_Freq, RF_Freq)
	     return RT_Fail;

	 //Set PLL
	 if(R848_PLL((u4RingFreq + IF_Freq), R848_STD_SIZE) != RT_Success)   //FilCal PLL
	       return RT_Fail;

	 //-----below must set after R848_MUX()-------//
	 //Set LNA TF for RF=72MHz. no use
	 if((R848_SetTfType==R848_TF_NARROW) || (R848_SetTfType==R848_TF_NARROW_LIN))   //UL use 270n setting
	 {
	    R848_I2C.RegAddr = 0x08;	//  R848:R8[6:0]  
        R848_Array[0] = (R848_Array[0] & 0x80) | 0x1F;  
	 }
	 else
	 {
	    R848_I2C.RegAddr = 0x08;	//  R848:R8[6:0]  
        R848_Array[0] = (R848_Array[0] & 0x80) | 0x00;  
	 }
     R848_I2C.Data = R848_Array[0];
     if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;

	 //Adc=on set 0;
	 R848_I2C.RegAddr = 0x0F;		//  R848:R15[7]  
     R848_Array[7] = (R848_Array[7] & 0x7F);
     R848_I2C.Data = R848_Array[7];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;



	//pwd_vga  vga power on set 0;
	 R848_I2C.RegAddr = 0x12;	//  R848:R18[7] 
     R848_Array[10] = (R848_Array[10] & 0x7F);  
     R848_I2C.Data = R848_Array[10];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;




	 //vga6db normal set 0;
	 R848_I2C.RegAddr = 0x0B;		// R848:R11[3]  
     R848_Array[3] = (R848_Array[3] & 0xF7);
     R848_I2C.Data = R848_Array[3];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

 	 //Vga Gain = -12dB 
	 R848_I2C.RegAddr = 0x14;		//  R848:R20[3:0]  
     R848_Array[12] = (R848_Array[12] & 0xF0);
     R848_I2C.Data = R848_Array[12];
     if(I2C_Write(&R848_I2C) != RT_Success)
           return RT_Fail;

	
	 // vcomp = 0
	 R848_I2C.RegAddr = 0x26;	//  R848:R38[6:5]  
	 R848_Array[30] = (R848_Array[30] & 0x9F);	
	 R848_I2C.Data = R848_Array[30];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	
	 //Set BW=8M, HPF corner narrowest; 1.7M disable
     R848_I2C.RegAddr = 0x13;	//  R848:R19[7:0]  
	 R848_Array[11] = (R848_Array[11] & 0x00);	  
     R848_I2C.Data = R848_Array[11];		
     if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;	

	 //------- increase VGA power to let ADC read value significant ---------//
	 UINT8   VGA_Count = 0;
	 UINT8   VGA_Read = 0;

	 R848_I2C.RegAddr = 0x12;	//  R848:R18[3:0]  
     R848_Array[10] = (R848_Array[10] & 0xF0) | 0;  //filter code=0
     R848_I2C.Data = R848_Array[10];
     if(I2C_Write(&R848_I2C) != RT_Success)
          return RT_Fail;

	 for(VGA_Count=0; VGA_Count < 16; VGA_Count ++)
	 {
		R848_I2C.RegAddr = 0x14;	//  R848:R20[3:0]  
		R848_I2C.Data    = (R848_Array[12] & 0xF0) + VGA_Count;  
		if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

		R848_Delay_MS(10); //
		
		if(R848_Muti_Read(&VGA_Read) != RT_Success)
			return RT_Fail;

		if(VGA_Read > 40*ADC_READ_COUNT)
			break;
	 }

	 //------- Try suitable BW --------//

	 if(R848_BW==0x60) //6M
         initial_cnt = 1;  //try 7M first
	 else
		 initial_cnt = 0;  //try 8M first

	 for(i=initial_cnt; i<3; i++)
	 {
         if(i==0)
             R848_Bandwidth = 0x00; //8M
		 else if(i==1)
			 R848_Bandwidth = 0x40; //7M
		 else
			 R848_Bandwidth = 0x60; //6M

		 R848_I2C.RegAddr = 0x13;	//  R848:R19[7:0]  
	     R848_Array[11] = (R848_Array[11] & 0x00) | R848_Bandwidth;	  
         R848_I2C.Data = R848_Array[11];		
         if(I2C_Write(&R848_I2C) != RT_Success)
		      return RT_Fail;	

		 // read code 0
		 R848_I2C.RegAddr = 0x12;	//  R848:R18[3:0]  
		 R848_Array[10] = (R848_Array[10] & 0xF0) | 0;  //code 0
		 R848_I2C.Data = R848_Array[10];
		 if(I2C_Write(&R848_I2C) != RT_Success)
			  return RT_Fail;

		 R848_Delay_MS(10); //delay ms
	     
		 if(R848_Muti_Read(&u1FilterCalValuePre) != RT_Success)
			  return RT_Fail;

		 //read code 13
		 R848_I2C.RegAddr = 0x12;	// R848:R18[3:0]  
		 R848_Array[10] = (R848_Array[10] & 0xF0) | 13;  //code 13
		 R848_I2C.Data = R848_Array[10];
		 if(I2C_Write(&R848_I2C) != RT_Success)
			  return RT_Fail;

		 R848_Delay_MS(10); //delay ms
	     
		 if(R848_Muti_Read(&u1FilterCalValue) != RT_Success)
			  return RT_Fail;

		 if(u1FilterCalValuePre > (u1FilterCalValue+8))  //suitable BW found
			 break;
	 }

     //-------- Try LPF filter code ---------//
	 u1FilterCalValuePre = 0;
	 for(u1FilterCode=0; u1FilterCode<16; u1FilterCode++)
	 {
         R848_I2C.RegAddr = 0x12;	//  R848:R18[3:0]  
         R848_Array[10] = (R848_Array[10] & 0xF0) | u1FilterCode;
         R848_I2C.Data = R848_Array[10];
         if(I2C_Write(&R848_I2C) != RT_Success)
              return RT_Fail;

		 R848_Delay_MS(10); //delay ms

		 if(R848_Muti_Read(&u1FilterCalValue) != RT_Success)
		      return RT_Fail;

		 if(u1FilterCode==0)
              u1FilterCalValuePre = u1FilterCalValue;

		 if((u1FilterCalValue+FilCal_Gap*ADC_READ_COUNT) < u1FilterCalValuePre)
		 {
			 u1FilterCodeResult = u1FilterCode;
			  break;
		 }

	 }

	 if(u1FilterCode==16)
          u1FilterCodeResult = 15;

	  return u1FilterCodeResult;

}
R848_ErrCode R848_SetFrequency(R848_Set_Info R848_INFO)
{

	 UINT32	LO_KHz;

	 if(R848_INFO.R848_Standard!=R848_DVB_S)
	 {
		 // Check Input Frequency Range
		 if((R848_INFO.RF_KHz<40000) || (R848_INFO.RF_KHz>1002000))
		 {
				  return RT_Fail;
		 }
	 }
	 else
	 {
		// Check Input Frequency Range
		 if((R848_INFO.RF_KHz<40000) || (R848_INFO.RF_KHz>2400000))
		 {
				  return RT_Fail;
		 }
	 }

	      LO_KHz = R848_INFO.RF_KHz + Sys_Info1.IF_KHz;

	 //Set MUX dependent var. Must do before PLL( ) 
     if(R848_MUX(LO_KHz, R848_INFO.RF_KHz, R848_INFO.R848_Standard) != RT_Success)   //normal MUX
        return RT_Fail;


     //Set PLL
     if(R848_PLL(LO_KHz, R848_INFO.R848_Standard) != RT_Success) //noraml PLL
        return RT_Fail;

	 //Set TF
	 if(R848_SetTF(R848_INFO.RF_KHz, R848_SetTfType) != RT_Success)
		return RT_Fail;



	 #if(FOR_KOREA_CTMR==TRUE)  //Q_CTRL OFF
		 if((R848_INFO.RF_KHz>700000) && (R848_INFO.R848_Standard<R848_ATV_SIZE))
		 {
			R848_I2C.RegAddr = 0x08;
			R848_Array[0] = (R848_Array[0] & 0x80) | 0x00;
			R848_I2C.Data = R848_Array[0];
			if(I2C_Write(&R848_I2C) != RT_Success)
				return RT_Fail;
		 }
	 #endif




     R848_IMR_point_num = Freq_Info1.IMR_MEM;

	 //Q1.5K   Q_ctrl  R848:R14[4]
	 //if(R848_INFO.RF_KHz<R848_LNA_MID_LOW[R848_TF_NARROW]) //<300MHz
     //     R848_Array[6] = R848_Array[6] | 0x10;	
	 //else
	 //	  R848_Array[6] = R848_Array[6] & 0xEF;		 

	if(R848_INFO.RF_KHz<=472000) //<472MHz
		R848_Array[6] = R848_Array[6] | 0x10;	
	else
	 	R848_Array[6] = R848_Array[6] & 0xEF;

	//medQctrl 1.5K
	if((R848_INFO.RF_KHz>=300000)&&(R848_INFO.RF_KHz<=472000)) //<473MHz and >299MHz
		R848_Array[6] = R848_Array[6] | 0x01;	
	else
	 	R848_Array[6] = R848_Array[6] & 0xFE;

	 R848_I2C.RegAddr = 0x0E;	//  R848:R14[1] 
	 R848_I2C.Data = R848_Array[6];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	
	 //3~6 shrink
	if((R848_INFO.RF_KHz>=300000)&&(R848_INFO.RF_KHz<=550000)) //<551MHz and >299MHz
		R848_Array[3] = R848_Array[3] & 0xFB;	
	else
	 	R848_Array[3] = R848_Array[3] | 0x04;

	 R848_I2C.RegAddr = 0x0B;	//  R848:R11[2] 
	 R848_I2C.Data = R848_Array[3];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;



	 //set Xtal AAC on=1 ;off=0
	 R848_I2C.RegAddr = 0x18;	//  R848:R24[1]  
	 R848_Array[16] = R848_Array[16] & 0xFD;
	 R848_I2C.Data = R848_Array[16];
	 if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;


     //Get Sys-Freq parameter	
     SysFreq_Info1 = R848_SysFreq_Sel(R848_INFO.R848_Standard, R848_INFO.RF_KHz);

	
     // Set LNA_TOP
	 R848_I2C.RegAddr = 0x23;	//  R848:R35[2:0]  
     R848_Array[27] = (R848_Array[27] & 0xF8) | (SysFreq_Info1.LNA_TOP);
     R848_I2C.Data = R848_Array[27];
     if(I2C_Write(&R848_I2C) != RT_Success)
         return RT_Fail;

	  // Set LNA VTHL
	 R848_I2C.RegAddr = 0x1F;	// R848:R31[7:0]  
     R848_Array[23] = (R848_Array[23] & 0x00) | SysFreq_Info1.LNA_VTH_L;
     R848_I2C.Data = R848_Array[23];
     if(I2C_Write(&R848_I2C) != RT_Success)
        return RT_Fail;

     // Set MIXER TOP
	 R848_I2C.RegAddr = 0x24;	// R848:R36[7:0]   
     R848_Array[28] = (R848_Array[28] & 0xF0) | (SysFreq_Info1.MIXER_TOP); 
     R848_I2C.Data = R848_Array[28];
     if(I2C_Write(&R848_I2C) != RT_Success)
        return RT_Fail;

     // Set MIXER VTHL
	 R848_I2C.RegAddr = 0x20;	//  R848:R32[7:0]  
     R848_Array[24] = (R848_Array[24] & 0x00) | SysFreq_Info1.MIXER_VTH_L;
     R848_I2C.Data = R848_Array[24];
     if(I2C_Write(&R848_I2C) != RT_Success)
        return RT_Fail;


	 // Set RF TOP
	 R848_I2C.RegAddr = 0x22;	// R848:R34[7:0]  
	 R848_Array[26] = (R848_Array[26] & 0x1F) | SysFreq_Info1.RF_TOP;
     R848_I2C.Data = R848_Array[26];
     if(I2C_Write(&R848_I2C) != RT_Success)
        return RT_Fail;

	 // Set Nrb TOP
	 R848_I2C.RegAddr = 0x24;	//R848:R36[7:4]   
	 R848_Array[28] = (R848_Array[28] & 0x0F) | SysFreq_Info1.NRB_TOP;
     R848_I2C.Data = R848_Array[28];
     if(I2C_Write(&R848_I2C) != RT_Success)
        return RT_Fail;


	 // Set Nrb BW
	 R848_I2C.RegAddr = 0x23;	//  R848:R35[7:6]  
	 R848_Array[27] = (R848_Array[27] & 0x3F) | SysFreq_Info1.NRB_BW;
     R848_I2C.Data = R848_Array[27];
     if(I2C_Write(&R848_I2C) != RT_Success)
        return RT_Fail;

     //printf("in set fre wwr\n");
     //R848_ReadReg();

	 // Set TF LPF
	 R848_I2C.RegAddr = 0x0C;	// R848:R12[6]  
	 R848_Array[4] = (R848_Array[4] & 0xBF) | SysFreq_Info1.BYP_LPF;
     R848_I2C.Data = R848_Array[4];
     if(I2C_Write(&R848_I2C) != RT_Success)
        return RT_Fail;

     //printf("in set fre wwrr\n");
     //R848_ReadReg();


	R848_I2C.RegAddr = 0x2E;	//0 	 R848:R46[3:1]=0'b000	
	R848_Array[38] = (R848_Array[38] & 0xF1) | SysFreq_Info1.RF_FAST_DISCHARGE;  
	R848_I2C.Data = R848_Array[38];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;



	R848_I2C.RegAddr = 0x16;	//4   R848:R22[7:5]=2'b010	
	R848_Array[14] = (R848_Array[14] & 0x1F) | SysFreq_Info1.RF_SLOW_DISCHARGE;  
	R848_I2C.Data = R848_Array[14];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	R848_I2C.RegAddr = 0x26;	//1   R848:R38[4]=1	
	R848_Array[30] = (R848_Array[30] & 0xEF) | SysFreq_Info1.RFPD_PLUSE_ENA;  
	R848_I2C.Data = R848_Array[30];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;



	R848_I2C.RegAddr = 0x2B;	//10  R848:R43[4:0]=5'b01010	
	R848_Array[35] = (R848_Array[35] & 0xE0) | SysFreq_Info1.LNA_FAST_DISCHARGE;  
	R848_I2C.Data = R848_Array[35];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;


	R848_I2C.RegAddr = 0x16;	//0  R848:R22[4:2]=3'b000
	R848_Array[14] = (R848_Array[14] & 0xE3) | SysFreq_Info1.LNA_SLOW_DISCHARGE;
	R848_I2C.Data = R848_Array[14];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	R848_I2C.RegAddr = 0x11;	//1  R848:R17[7]=0
	R848_Array[9] = (R848_Array[9] & 0x7F) | SysFreq_Info1.LNAPD_PLUSE_ENA;
	R848_I2C.Data = R848_Array[9];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;


// Set AGC clk 
	R848_I2C.RegAddr = 0x1A;	//  R848:R26[4:2] 
	R848_Array[18] = (R848_Array[18] & 0xE3) | SysFreq_Info1.AGC_CLK;  
	R848_I2C.Data = R848_Array[18];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	 //no clk out
	 R848_I2C.RegAddr = 0x19;
	 R848_Array[17] = (R848_Array[17] | 0x80);   //no clk out // R848:R25[7]  
	 R848_I2C.Data = R848_Array[17];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;


	 if(R848_INFO.R848_Standard<R848_ATV_SIZE)   //ATV
	 {
		 if(R848_XtalDiv == XTAL_DIV1)
		 {
			 //AGC CLK to 500hz		// R848:R26[4:2]   26-8=18   26(0x1A) is addr ; [18] is data
			 R848_Array[18] = (R848_Array[18] & 0xE3) | 0x14;  //[4:2]=101
		 }
		 else
		 {
			 //AGC CLK to 1khz
			 R848_Array[18] = (R848_Array[18] & 0xE3) | 0x00;  //[4:2]=000
		 }
		 R848_I2C.RegAddr = 0x1A;						
		 R848_I2C.Data = R848_Array[18];
		 if(I2C_Write(&R848_I2C) != RT_Success)
			 return RT_Fail;
	 }
	 else
	 {
		 //for DVB-T2
         switch(R848_INFO.R848_Standard)
		 {
		    case R848_DVB_T2_6M:
			case R848_DVB_T2_7M:
			case R848_DVB_T2_8M:
			case R848_DVB_T2_1_7M:
			case R848_DVB_T2_10M:
			case R848_DVB_T2_6M_IF_5M:
			case R848_DVB_T2_7M_IF_5M:
			case R848_DVB_T2_8M_IF_5M:
			case R848_DVB_T2_1_7M_IF_5M:

                 R848_Delay_MS(100);
                /*
				 //AGC clk 250Hz
				 R848_I2C.RegAddr = 0x1A;	//  R26[4:2]  	
				 R848_Array[18] = (R848_Array[18] & 0xE3) | 0x1C;  //[4:2]=111
				 R848_I2C.Data = R848_Array[18];
				 if(I2C_Write(&R848_I2C) != RT_Success)
					 return RT_Fail;
                   */
				 //force plain mode
				 R848_I2C.RegAddr = 0x0B;	// R848:R11[7]   		
				 R848_Array[3] = (R848_Array[3] | 0x80);  //[7]=1
				 R848_I2C.Data = R848_Array[3];
				 if(I2C_Write(&R848_I2C) != RT_Success)
					 return RT_Fail;

				 R848_I2C.RegAddr = 0x0A;	// R848:R10[5]   		
				 R848_Array[2] = (R848_Array[2] & 0xDF);  //[5]=0
				 R848_I2C.Data = R848_Array[2];
				 if(I2C_Write(&R848_I2C) != RT_Success)
					 return RT_Fail;

				 break;

		     default:		 
                 break;
		 }//end switch
	 //}
	 return RT_Success;
}

}


R848_ErrCode R848_DVBS_Setting(R848_Set_Info R848_INFO)
{
	 UINT32	LO_KHz;
	 UINT8 fine_tune,Coarse_tune;
	 UINT8 test_coar=0x0D;
	 UINT32 Coarse;

	if(R848_INFO.R848_Standard != R848_pre_standard)
	{
		 if(R848_InitReg(R848_INFO.R848_Standard) != RT_Success)      
		     return RT_Fail;
	}
    R848_pre_standard = R848_INFO.R848_Standard;


	 LO_KHz = R848_INFO.RF_KHz;

	if(R848_PLL(LO_KHz, R848_INFO.R848_Standard)!= RT_Success)
	{
		return RT_Fail;
	}
	
	//VTH/VTL
	if((R848_INFO.RF_KHz >= 1200000)&&(R848_INFO.RF_KHz <= 1750000))
	{
		R848_Array[23]=(R848_Array[23] & 0x00) | 0x93;			//R848:R31[7:0]    1.24/0.64
	}
	else
	{	
		R848_Array[23]=(R848_Array[23] & 0x00) | 0x83;			//R848:R31[7:0]   1.14/0.64
	}
	R848_I2C.RegAddr = 0x1F;
	R848_I2C.Data = R848_Array[23];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;



	if(R848_INFO.RF_KHz >= 2000000) 
	{
		R848_I2C.RegAddr = 0x2E;
		R848_Array[38]=(R848_Array[38] & 0xCF) | 0x20;			//R848:R46[4:5]
		R848_I2C.Data = R848_Array[38];
		if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;
	}


	if((R848_INFO.RF_KHz >= 1600000) && (R848_INFO.RF_KHz <= 1950000))
	{
		R848_Array[35] |= 0x20; //LNA Mode with att   //R710 R2[6]    R848:R43[5]   43-8=35   43(0x2B) is addr ; [35] is data
		//R848_Array[36] |= 0x04; //Mixer Buf -3dB		  //R710 R8[7]    R848:R44[2]   44-8=36   44(0x2C) is addr ; [36] is data
	}
	else
	{
		R848_Array[35] &= 0xDF; //LNA Mode no att
		//R848_Array[36] &= 0xFB; //Mixer Buf off
	}

	R848_I2C.RegAddr = 0x2B;
	R848_I2C.Data = R848_Array[35];
	if(I2C_Write(&R848_I2C) != RT_Success)
	return RT_Fail;


	R848_I2C.RegAddr = 0x2C;
	R848_I2C.Data = R848_Array[36];
	if(I2C_Write(&R848_I2C) != RT_Success)
	return RT_Fail;


	//Output Signal Mode    (  O is diff  ; 1 is single  )
	if(R848_INFO.R848_DVBS_OutputSignal_Mode != SINGLEOUT)
	{
		R848_Array[35] &=0x7F;
	}
	else
	{
		R848_Array[35] |=0x80;  //R710 R11[4]    R848:R43[7]   43-8=35   43(0x2B) is addr ; [35] is data
	}

	R848_I2C.RegAddr = 0x2B;
	R848_I2C.Data = R848_Array[35];
	if(I2C_Write(&R848_I2C) != RT_Success)
	return RT_Fail;



	//AGC Type  //R13[4] Negative=0 ; Positive=1;
	if(R848_INFO.R848_DVBS_AGC_Mode != AGC_POSITIVE)
	{
		R848_Array[37] &= 0xF7;
	}
	else
	{
		R848_Array[37] |= 0x08;  //R710 R13[4]    R848:R45[3]   45-8=37   45(0x2D) is addr ; [37] is data
	}
	R848_I2C.RegAddr = 0x2D;
	R848_I2C.Data = R848_Array[37];
	if(I2C_Write(&R848_I2C) != RT_Success)
	return RT_Fail;


	//RT710_Vga_Sttenuator_Type
	if(R848_INFO.R848_DVBS_AttenVga_Mode != ATTENVGAON)
	{
		R848_Array[34] &= 0x7F;
	}
	else
	{
		R848_Array[34] |= 0x80;
	}
	R848_I2C.RegAddr = 0x2A;
	R848_I2C.Data = R848_Array[34];
	if(I2C_Write(&R848_I2C) != RT_Success)
	return RT_Fail;


	//R710_Fine_Gain_Type
	switch(R848_INFO.R848_DVBS_FineGain)
	{
		case FINEGAIN_3DB:  
			 R848_Array[38] = (R848_Array[38] & 0x3F) | 0x00;
		break;
		case FINEGAIN_2DB:  
			 R848_Array[38] = (R848_Array[38] & 0x3F) | 0x40;
		break;
		case FINEGAIN_1DB:  
			 R848_Array[38] = (R848_Array[38] & 0x3F) | 0x80;
		break;
		case FINEGAIN_0DB:  
			 R848_Array[38] = (R848_Array[38] & 0x3F) | 0xC0;
		break;
		default:		
			R848_Array[38] = (R848_Array[38] & 0x3F) | 0x00;
		break;
	}
	R848_I2C.RegAddr = 0x2E;
	R848_I2C.Data = R848_Array[38];
	if(I2C_Write(&R848_I2C) != RT_Success)
	return RT_Fail;


	if (R848_INFO.DVBS_BW >67400)
	{
		fine_tune=1;
		Coarse =(( R848_INFO.DVBS_BW -67400) /1600)+31;
		if((( R848_INFO.DVBS_BW -67400) % 1600) > 0)
		Coarse+=1;
	}

	else if((R848_INFO.DVBS_BW >62360)&&(R848_INFO.DVBS_BW<=67400))
	{
		Coarse=31;
		fine_tune=1;		
	}
	else if((R848_INFO.DVBS_BW >38000)&&(R848_INFO.DVBS_BW<=62360))
	{
		fine_tune=1;	
		Coarse =(( R848_INFO.DVBS_BW -38000) /1740)+16;
		if((( R848_INFO.DVBS_BW -38000) % 1740) > 0)
		Coarse+=1;
				
	}
	else if(R848_INFO.DVBS_BW<=5000)
	{	
		Coarse=0;
		fine_tune=0;
	}
	else if((R848_INFO.DVBS_BW>5000) && (R848_INFO.DVBS_BW<=8000))
	{
		Coarse=0;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>8000) && (R848_INFO.DVBS_BW<=10000))
	{
		Coarse=1;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>10000) && (R848_INFO.DVBS_BW<=12000))
	{
		Coarse=2;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>12000) && (R848_INFO.DVBS_BW<=14200))
	{
		Coarse=3;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>14200) && (R848_INFO.DVBS_BW<=16000))
	{
		Coarse=4;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>16000) && (R848_INFO.DVBS_BW<=17800))
	{
		Coarse=5;
		fine_tune=0;
	}
	else if((R848_INFO.DVBS_BW>17800) && (R848_INFO.DVBS_BW<=18600))
	{
		Coarse=5;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>18600) && (R848_INFO.DVBS_BW<=20200))
	{
		Coarse=6;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>20200) && (R848_INFO.DVBS_BW<=22400))
	{
		Coarse=7;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>22400) && (R848_INFO.DVBS_BW<=24600))
	{
		Coarse=9;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>24600) && (R848_INFO.DVBS_BW<=25400))
	{
		Coarse=10;
		fine_tune=0;
	}
	else if((R848_INFO.DVBS_BW>25400) && (R848_INFO.DVBS_BW<=26000))
	{
		Coarse=10;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>26000) && (R848_INFO.DVBS_BW<=27200))
	{
		Coarse=11;
		fine_tune=0;
	}
	else if((R848_INFO.DVBS_BW>27200) && (R848_INFO.DVBS_BW<=27800))
	{
		Coarse=11;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>27800) && (R848_INFO.DVBS_BW<=30200))
	{
		Coarse=12;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>30200) && (R848_INFO.DVBS_BW<=32600))
	{
		Coarse=13;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>32600) && (R848_INFO.DVBS_BW<=33800))
	{
		Coarse=14;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>33800) && (R848_INFO.DVBS_BW<=36800))
	{
		Coarse=15;
		fine_tune=1;
	}
	else if((R848_INFO.DVBS_BW>36800) && (R848_INFO.DVBS_BW<=38000))
	{
		Coarse=16;
		fine_tune=1;
	}

	Coarse_tune = (unsigned char) Coarse;//coras filter value

	//fine tune and coras filter write		
	R848_I2C.RegAddr = 0x2F;
	R848_Array[39] &= 0x00;		//47-8=39
	R848_Array[39] = ((R848_Array[39] | ( fine_tune<< 6 ) ) | ( Coarse_tune));
	R848_I2C.Data = R848_Array[39];
	if(I2C_Write(&R848_I2C) != RT_Success)
		return RT_Fail;

	//Set GPO Low
	R848_I2C.RegAddr = 0x17;	// R848:R23[4:2]  23-8=15  23(0x17) is addr ; [15] is data
	R848_Array[15] = (R848_Array[15] & 0xFE);
	R848_I2C.Data = R848_Array[15];
	if(I2C_Write(&R848_I2C) != RT_Success)
	return RT_Fail;


	return RT_Success;
}
R848_ErrCode R848_SetPllData(R848_Set_Info R848_INFO)
{	  
	if(R848_Initial_done_flag==FALSE)
	{
		R848_Init();
	}

	if(R848_INFO.R848_Standard!=R848_DVB_S)
	{
      if(R848_SetStandard(R848_INFO.R848_Standard) != RT_Success)
	  {
		  return RT_Fail;
	  }

      if(R848_SetFrequency(R848_INFO) != RT_Success)
	  {
          return RT_Fail;
	  }
	  R848_SATELLITE_FLAG = 0;
	}
	else
	{
		 if(R848_DVBS_Setting(R848_INFO) != RT_Success)
		  return RT_Fail;
		 R848_SATELLITE_FLAG = 1;
	}

      return RT_Success;
}


R848_ErrCode R848_Standby()
{
	 // R848:R8[7]   
	 R848_Array[0] = (R848_Array[0] | 0x80);         //LNA off   All / LNA / Buffer PW off
	//  R848:R37[3]  	
	 R848_Array[31] = R848_Array[31] | 0x08;        //LNA det off


	 R848_I2C.RegAddr = 0x08;	//  R848:R8[7]   
	 R848_I2C.Data = R848_Array[0];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	 //LT PW off
	 R848_I2C.RegAddr = 0x09;	// R848:R9[0]   
	 R848_Array[1] = R848_Array[1] | 0x01;
	 R848_I2C.Data = R848_Array[1];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;


	 //RF Buffer PW off
	 R848_I2C.RegAddr = 0x09;	// R848:R9[0] 	
	 R848_Array[1] = R848_Array[1] | 0x04;
	 R848_I2C.Data = R848_Array[1];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	 //RF, Nrb Det PW off
	 R848_I2C.RegAddr = 0x25;	// R848:R37[3] 
	 R848_Array[31] = R848_Array[31] | 0x05;
	 R848_I2C.Data = R848_Array[31];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	 //LNA current = lowest, R6[1:0]=11   
	 R848_I2C.RegAddr = 0x0E;	// R848:R14[1:0] 
	 R848_Array[6] = R848_Array[6] | 0x03;
	 R848_I2C.Data = R848_Array[6];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;


	 //NAT PW off
	 R848_I2C.RegAddr = 0x26;	// R848:R38[7]   
	 R848_Array[30] = R848_Array[30] | 0x80;
	 R848_I2C.Data = R848_Array[30];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	 //Mixer PW off
	 R848_I2C.RegAddr = 0x0C;	// R848:R12[3]  
	 R848_Array[4] = R848_Array[4] | 0x08;
	 R848_I2C.Data = R848_Array[4];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;


	 //Filter PW off    //pwd_filt / all / vga / poly / amp
	 R848_I2C.RegAddr = 0x12;	//  R848:R18[7]   
	 R848_Array[10] = R848_Array[10] | 0x80;
	 R848_I2C.Data = R848_Array[10];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	 //ADC PW off
	 R848_I2C.RegAddr = 0x0F;	// R848:R15[7]   
	 R848_Array[7] = R848_Array[7] | 0x80;
	 R848_I2C.Data = R848_Array[7];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;


     //PLL LDO A off
	 R848_I2C.RegAddr = 0x15;	//  R848:R21[5:4]   
	 R848_Array[13] = R848_Array[13] | 0x30;
	 R848_I2C.Data = R848_Array[13];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	 //PLL DLDO 1& 2 off
	 R848_I2C.RegAddr = 0x15;	//  R848:R21[3:0]   
	 R848_Array[13] = R848_Array[13] | 0x0F;
	 R848_I2C.Data = R848_Array[13];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;
	
	 //PLL SELS & SELT off
	 R848_I2C.RegAddr = 0x18;	//  R848:R24[3]  
	 R848_Array[16] = R848_Array[16] & 0xE7;
	 R848_I2C.Data = R848_Array[16];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	 //AGC off
	 R848_I2C.RegAddr = 0x15;	//R848:R21[6]  
	 R848_Array[13] = R848_Array[13] | 0x40;
	 R848_I2C.Data = R848_Array[13];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	//RF_DET_POWER
	 R848_I2C.RegAddr = 0x28;	//R848:R40[0]   
	 R848_Array[32] = R848_Array[32] | 0x01;
	 R848_I2C.Data = R848_Array[32];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	 //NET_DET PW
	 R848_I2C.RegAddr = 0x28;	//R848:R40[3]   40-8=32   
	 R848_Array[32] = R848_Array[32] | 0x08;
	 R848_I2C.Data = R848_Array[32];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;
	
	 //ALL_LNA_BUF PW
	 R848_I2C.RegAddr = 0x28;	//R848:R40[3]   40-8=32  
	 R848_Array[32] = R848_Array[32] | 0x10;
	 R848_I2C.Data = R848_Array[32];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;
	
	 //AMP_FILT PW
	 R848_I2C.RegAddr = 0x28;	//R848:R40[6]   40-8=32  
	 R848_Array[32] = R848_Array[32] | 0x40;
	 R848_I2C.Data = R848_Array[32];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;

	//LNA PW
	 R848_I2C.RegAddr = 0x28;	//R848:R40[6]   40-8=32   
	 R848_Array[32] = R848_Array[32] | 0x80;
	 R848_I2C.Data = R848_Array[32];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;
	
	//BYP_LPF
	 R848_I2C.RegAddr = 0x0C;	//R848:R12[6]   12-8=4   
	 R848_Array[4] = R848_Array[4] & 0xBF;
	 R848_I2C.Data = R848_Array[4];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;
	
	 //XTAL LDO PW
	 R848_I2C.RegAddr = 0x17;	//R848:R23[2]   23-8=15  
	 R848_Array[15] = R848_Array[15] | 0x04;
	 R848_I2C.Data = R848_Array[15];
	 if(I2C_Write(&R848_I2C) != RT_Success)
			return RT_Fail;
	
	return RT_Success;
}



R848_ErrCode R848_GetRfGain(R848_RF_Gain_Info *pR848_rf_gain)
{

	R848_I2C_Len.RegAddr = 0x00;
	R848_I2C_Len.Len =0x05;
	if(I2C_Read_Len(&R848_I2C_Len) != RT_Success)
	{
		I2C_Read_Len(&R848_I2C_Len);
	}
	pR848_rf_gain->RF_gain1 = (R848_I2C_Len.Data[4] & 0x1F);            //lna    //848:4[4:0]  
	pR848_rf_gain->RF_gain2 = ((R848_I2C_Len.Data[3] & 0xF0) >> 4);     //rf	 //848:3[4:0]  
	pR848_rf_gain->RF_gain3 = (R848_I2C_Len.Data[3] & 0x0F);             //mixer //848:3[4:0]  	
/*
	Test_Len.Data[0]=0;
	Test_Len.Data[1]=0;
	Test_Len.RegAddr = 0x03;
	Test_Len.Len =2;
	if(I2C_Read_Len(&Test_Len) != RT_Success)
		return RT_Fail;

	pR848_rf_gain->RF_gain1 = (Test_Len.Data[1] & 0x1F);            //lna    //848:4[4:0]  
	pR848_rf_gain->RF_gain2 = ((Test_Len.Data[0] & 0xF0) >> 4);  //rf		 //848:3[4:0]  
	pR848_rf_gain->RF_gain3 = (Test_Len.Data[0] & 0x0F);             //mixer //848:3[4:0]  */
	


	if(R848_SATELLITE_FLAG==0)
	{
		if(pR848_rf_gain->RF_gain1 > 22) 
			pR848_rf_gain->RF_gain1 = 22;  //LNA gain max is 22

		if(pR848_rf_gain->RF_gain3 > 10)
			pR848_rf_gain->RF_gain3 = 10;  //MixerAmp gain max is 10

		pR848_rf_gain->RF_gain_comb = (pR848_rf_gain->RF_gain1*14 + pR848_rf_gain->RF_gain2*12 + pR848_rf_gain->RF_gain3*12);
	}
	else
	{
		if (pR848_rf_gain->RF_gain1 <= 2)
		{
			pR848_rf_gain->RF_gain1=0;
		}
		else if(pR848_rf_gain->RF_gain1 > 2 && pR848_rf_gain->RF_gain1 <= 9) 
		{
			pR848_rf_gain->RF_gain1 -=2;
		}
		else if(pR848_rf_gain->RF_gain1 >9 && pR848_rf_gain->RF_gain1 <=12)
		{
			pR848_rf_gain->RF_gain1 = 8;
		}
		else if(pR848_rf_gain->RF_gain1 >12 && pR848_rf_gain->RF_gain1 <= 20)
		{
			pR848_rf_gain->RF_gain1 -= 4;
		}
		else if(pR848_rf_gain->RF_gain1 > 20 && pR848_rf_gain->RF_gain1 <= 22 )
		{
			pR848_rf_gain->RF_gain1 = 17;
		}
		else if(pR848_rf_gain->RF_gain1 > 22 )
		{
			pR848_rf_gain->RF_gain1 = 18;
		}
	}

    return RT_Success;
}


R848_ErrCode R848_RfGainMode(R848_RF_Gain_TYPE R848_RfGainType)
{
    UINT8 MixerGain = 0;
	UINT8 RfGain = 0;
	UINT8 LnaGain = 0;

	if(R848_RfGainType==RF_MANUAL)
	{
		//LNA auto off
	     R848_I2C.RegAddr = 0x0D;
	     R848_Array[5] = R848_Array[5] | 0x80;   // 848:13[7:0]   
		 R848_I2C.Data = R848_Array[5];
	     if(I2C_Write(&R848_I2C) != RT_Success)
		       return RT_Fail;

		 //Mixer buffer off
	     R848_I2C.RegAddr = 0x22;
	     R848_Array[26] = R848_Array[26] | 0x10;  // 848:34[7:0]   
		 R848_I2C.Data = R848_Array[26];
	     if(I2C_Write(&R848_I2C) != RT_Success)
		       return RT_Fail;

		 //Mixer auto off
	     R848_I2C.RegAddr = 0x0F;
	     R848_Array[7] = R848_Array[7] & 0xEF;  //848:15[6:0]
		 R848_I2C.Data = R848_Array[7];
	     if(I2C_Write(&R848_I2C) != RT_Success)
		       return RT_Fail;

		R848_I2C_Len.RegAddr = 0x00;
		R848_I2C_Len.Len     = 5; 
		if(I2C_Read_Len(&R848_I2C_Len) != RT_Success)
		{
			I2C_Read_Len(&R848_I2C_Len);
		}

		//MixerGain = (((R848_I2C_Len.Data[1] & 0x40) >> 6)<<3)+((R848_I2C_Len.Data[3] & 0xE0)>>5);   //?
		MixerGain = (R848_I2C_Len.Data[3] & 0x0F); //mixer // 848:3[4:0]
		RfGain = ((R848_I2C_Len.Data[3] & 0xF0) >> 4);   //rf		 // 848:3[4:0] 
		LnaGain = R848_I2C_Len.Data[4] & 0x1F;  //lna    // 848:4[4:0]  

		//set LNA gain
	     R848_I2C.RegAddr = 0x0D;
	     R848_Array[5] = (R848_Array[5] & 0xE0) | LnaGain;  // 848:13[7:0]  
		 R848_I2C.Data = R848_Array[5];
	     if(I2C_Write(&R848_I2C) != RT_Success)
		       return RT_Fail;

		 //set Mixer Buffer gain
	     R848_I2C.RegAddr = 0x22;
	     R848_Array[26] = (R848_Array[26] & 0xF0) | RfGain;  //848:34[7:0] 
		 R848_I2C.Data = R848_Array[26];
	     if(I2C_Write(&R848_I2C) != RT_Success)
		       return RT_Fail;

		 //set Mixer gain
	     R848_I2C.RegAddr = 0x0F;
	     R848_Array[7] = (R848_Array[7] & 0xF0) | MixerGain; // 848:15[6:0]  
		 R848_I2C.Data = R848_Array[7];
	     if(I2C_Write(&R848_I2C) != RT_Success)
		       return RT_Fail;
	}
	else
	{
	    //LNA auto on
	     R848_I2C.RegAddr = 0x0D;
	     R848_Array[5] = R848_Array[5] & 0x7F;  // 848:13[7:0]  
		 R848_I2C.Data = R848_Array[5];
	     if(I2C_Write(&R848_I2C) != RT_Success)
		       return RT_Fail;

		 //Mixer buffer on
	     R848_I2C.RegAddr = 0x22;
	     R848_Array[26] = R848_Array[26] & 0xEF;	// 848:34[7:0]  
		 R848_I2C.Data = R848_Array[26];
	     if(I2C_Write(&R848_I2C) != RT_Success)
		       return RT_Fail;

		 //Mixer auto on
	     R848_I2C.RegAddr = 0x0F;
	     R848_Array[7] = R848_Array[7] | 0x10;	// 848:15[6:0]  
		 R848_I2C.Data = R848_Array[7];
	     if(I2C_Write(&R848_I2C) != RT_Success)
		       return RT_Fail;
	}

    return RT_Success;
}




//--------- Set IC_Internal_cap------------------------------------//
//   Xtal loading match formula: 
//   Xtal_cap_load*2 = IC_Internal_cap + External_cap
//   (default is for Xtal 16pF load)
//--------------------------------------------------------------------//
R848_ErrCode R848_SetXtalIntCap(R848_Xtal_Cap_TYPE R848_XtalCapType)
{
	 //Set internal Xtal cap 
	if(R848_XtalCapType==XTAL_CAP_LARGE)  //for Xtal CL >16pF
		R848_Array[19] = R848_Array[19] & 0xDF;
	else       //for Xtal CL < 16pF
		R848_Array[19] = R848_Array[19] | 0x20;

     R848_I2C.RegAddr = 0x1B;
	 R848_I2C.Data = R848_Array[19];
     if(I2C_Write(&R848_I2C) != RT_Success)
	       return RT_Fail;

	return RT_Success;
}


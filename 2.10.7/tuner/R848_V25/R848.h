//#include "..\stdafx.h"
#ifndef  _R848_H_ 
#define _R848_H_

#include "R848_I2C.h"

//----------------------------------------------------------//
//                   Define                                 //
//----------------------------------------------------------//
#define FOR_KOREA_CTMR  FALSE
#define FOR_TDA10024    0

#define VERSION   "R848_GUI_v2.5"


#define R848_USE_16M_XTAL			TRUE       //16M 
//#define R848_USE_16M_XTAL			FALSE       //24M

#define R848_SHARE_XTAL_OUT    FALSE     //self-oscillation
//#define R848_SHARE_XTAL_OUT    TRUE     //share Xtal, ext clk connect to XTAL_OUT


#define R848_REG_NUM         40
#define R848_TF_HIGH_NUM  8  
#define R848_TF_MID_NUM    8
#define R848_TF_LOW_NUM   8
#define R848_TF_LOWEST_NUM   8
#define R848_RING_POWER_FREQ_LOW  115000
#define R848_RING_POWER_FREQ_HIGH 450000
#define R848_IMR_IF              5300         
#define R848_IMR_TRIAL       9
#if (R848_USE_16M_XTAL==TRUE)
    #define R848_Xtal	  16000      //16M
#else
    #define R848_Xtal	  24000      //24M
#endif

//----------------------------------------------------------//
//                   Internal Structure                            //
//----------------------------------------------------------//
typedef struct _R848_Sys_Info_Type
{
	UINT16		   IF_KHz;
	UINT16		   FILT_CAL_IF;
	UINT8          BW;
	UINT8		   V17M; 
	UINT8		   HPF_COR;
	UINT8          FILT_EXT_ENA;
	UINT8          FILT_EXT_WIDEST;
	UINT8          FILT_EXT_POINT;
//	UINT8          AGC_CLK;
	UINT8		   FILT_COMP;
	UINT8		   FILT_CUR;  
	UINT8		   FILT_3DB; 
	UINT8		   SWBUF_CUR;  
	UINT8          TF_CUR;              
	UINT8		   INDUC_BIAS;  
	UINT8          SWCAP_CLK;
	UINT8		   NA_PWR_DET;  
}R848_Sys_Info_Type;

typedef struct _R848_Freq_Info_Type
{
	UINT8		RF_POLY;
	UINT8		LNA_BAND;
	UINT8		LPF_CAP;
	UINT8		LPF_NOTCH;
    UINT8		XTAL_POW0;
	UINT8		CP_CUR;
	UINT8		IMR_MEM;
	UINT8		Q_CTRL;   
}R848_Freq_Info_Type;

typedef struct _R848_SysFreq_Info_Type
{
	UINT8		LNA_TOP;
	UINT8		LNA_VTH_L;
	UINT8		MIXER_TOP;
	UINT8		MIXER_VTH_L;
	UINT8       RF_TOP;
	UINT8       NRB_TOP;
	UINT8       NRB_BW;
	UINT8       BYP_LPF;
	UINT8       RF_FAST_DISCHARGE;
	UINT8       RF_SLOW_DISCHARGE;
	UINT8       RFPD_PLUSE_ENA;
	UINT8       LNA_FAST_DISCHARGE;
	UINT8       LNA_SLOW_DISCHARGE;
	UINT8       LNAPD_PLUSE_ENA;
	UINT8       AGC_CLK;

}R848_SysFreq_Info_Type;

typedef struct _R848_Cal_Info_Type
{
	UINT8		FILTER_6DB;
	UINT8		MIXER_AMP_GAIN;
	UINT8		MIXER_BUFFER_GAIN;
	UINT8		LNA_GAIN;
	UINT8		LNA_POWER;
	UINT8		RFBUF_OUT;
	UINT8		RFBUF_POWER;
	UINT8		TF_CAL;
}R848_Cal_Info_Type;

typedef struct _R848_SectType
{
	UINT8   Phase_Y;
	UINT8   Gain_X;
	UINT8   Iqcap;
	UINT8   Value;
}R848_SectType;

typedef struct _R848_TF_Result
{
	UINT8   TF_Set;
	UINT8   TF_Value;
}R848_TF_Result;

typedef enum _R848_TF_Band_Type
{
    TF_HIGH = 0,
	TF_MID,
	TF_LOW
}R848_TF_Band_Type;

typedef enum _R848_TF_Type
{
	R848_TF_NARROW = 0,             //270n/68n   (ISDB-T, DVB-T/T2)
    R848_TF_BEAD,                   //Bead/68n   (DTMB)
	R848_TF_NARROW_LIN,             //270n/68n   (N/A)
	R848_TF_NARROW_ATV_LIN,			//270n/68n   (ATV)
	R848_TF_BEAD_LIN,               //Bead/68n   (PAL_DK for China Hybrid TV)
	R848_TF_NARROW_ATSC,			//270n/68n   (ATSC, DVB-C, J83B)
	R848_TF_BEAD_LIN_ATSC,			//Bead/68n   (ATSC, DVB-C, J83B)
	R848_TF_82N_BEAD,				//Bead/82n (DTMB)
	R848_TF_82N_270N,				//270n/82n (OTHER Standard)
	R848_TF_SIZE
}R848_TF_Type;

typedef enum _R848_UL_TF_Type
{
	R848_UL_USING_BEAD = 0,            
    R848_UL_USING_270NH,                      
}R848_UL_TF_Type;



typedef enum _R848_Cal_Type
{
	R848_IMR_CAL = 0,
	R848_IMR_LNA_CAL,
	R848_TF_CAL,
	R848_TF_LNA_CAL,
	R848_LPF_CAL,
	R848_LPF_LNA_CAL
}R848_Cal_Type;

typedef enum _R848_BW_Type
{
	BW_6M = 0,
	BW_7M,
	BW_8M,
	BW_1_7M,
	BW_10M,
	BW_200K
}R848_BW_Type;


enum R848_XTAL_PWR_VALUE
{
	XTAL_SMALL_LOWEST = 0,
    XTAL_SMALL_LOW,
    XTAL_SMALL_HIGH,
    XTAL_SMALL_HIGHEST,
    XTAL_LARGE_HIGHEST,
	XTAL_CHECK_SIZE
};


typedef enum _R848_Xtal_Div_TYPE
{
	XTAL_DIV1 = 0,
	XTAL_DIV2
}R848_Xtal_Div_TYPE;

//----------------------------------------------------------//
//                   Type Define                                    //
//----------------------------------------------------------//
#define UINT8  unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned long

//----------------------------------------------------------//
//                   R848 Public Parameter                     //
//----------------------------------------------------------//
typedef enum _R848_ErrCode
{
	RT_Success = true,
	RT_Fail    = false
}R848_ErrCode;

typedef enum _R848_Standard_Type  //Don't remove standand list!!
{
	R848_MN_5100 = 0,          //for NTSC_MN, PAL_M (IF=5.1M)
	R848_MN_5800,              //for NTSC_MN, PLA_M (IF=5.8M)
	R848_PAL_I,                //for PAL-I
	R848_PAL_DK,               //for PAL DK in non-"DTMB+PAL DK" case
	R848_PAL_B_7M,             //no use
	R848_PAL_BGH_8M,           //for PAL B/G, PAL G/H
	R848_SECAM_L,              //for SECAM L
	R848_SECAM_L1,             //for SECAM L'
	R848_SECAM_L1_INV,       
	R848_MN_CIF_5M,             //for NTSC_MN, PLA_M (CIF=5.0M)
	R848_PAL_I_CIF_5M,          //for PAL-I (CIF=5.0M)
	R848_PAL_DK_CIF_5M,         //for PAL DK (CIF=5M)
	R848_PAL_B_7M_CIF_5M,       //for PAL-B 7M (CIF=5M)
	R848_PAL_BGH_8M_CIF_5M,     //for PAL G/H 8M (CIF=5M)
	R848_SECAM_L_CIF_5M,        //for SECAM L (CIF=5M)
	R848_SECAM_L1_CIF_5M,       //for SECAM L' (CIF=5M)
	R848_SECAM_L1_INV_CIF_5M,   //(CIF=5M)
	R848_ATV_SIZE,
	R848_DVB_T_6M = R848_ATV_SIZE,
	R848_DVB_T_7M,
	R848_DVB_T_8M, 
    R848_DVB_T2_6M,			//IF=4.57M
	R848_DVB_T2_7M,			//IF=4.57M
	R848_DVB_T2_8M,			//IF=4.57M
	R848_DVB_T2_1_7M,
	R848_DVB_T2_10M,
	R848_DVB_C_8M,
	R848_DVB_C_6M, 
	R848_J83B,
	R848_ISDB_T,             //IF=4.063M
	R848_ISDB_T_4570,		 //IF=4.57M
	R848_DTMB_4570,			 //IF=4.57M
	R848_DTMB_6000,			 //IF=6.00M
	R848_DTMB_6M_BW_IF_5M,   //IF=5.0M, BW=6M
	R848_DTMB_6M_BW_IF_4500, //IF=4.5M, BW=6M
	R848_ATSC,
	R848_DVB_S,
	R848_DVB_T_6M_IF_5M,
	R848_DVB_T_7M_IF_5M,
	R848_DVB_T_8M_IF_5M,
	R848_DVB_T2_6M_IF_5M,
	R848_DVB_T2_7M_IF_5M,
	R848_DVB_T2_8M_IF_5M,
	R848_DVB_T2_1_7M_IF_5M,
	R848_DVB_C_8M_IF_5M,
	R848_DVB_C_6M_IF_5M, 
	R848_J83B_IF_5M,
	R848_ISDB_T_IF_5M,            
	R848_DTMB_IF_5M,     
	R848_ATSC_IF_5M,
	R848_FM,
	R848_STD_SIZE,
}R848_Standard_Type;

typedef enum _R848_GPO_Type
{
	HI_SIG = TRUE,
	LO_SIG = FALSE
}R848_GPO_Type;

typedef enum _R848_RF_Gain_TYPE
{
	RF_AUTO = 0,
	RF_MANUAL
}R848_RF_Gain_TYPE;

typedef enum _R848_Xtal_Cap_TYPE
{
	XTAL_CAP_LARGE = 0,
	XTAL_CAP_SMALL
}R848_Xtal_Cap_TYPE;

typedef enum _R848_DVBS_OutputSignal_Type
{
	DIFFERENTIALOUT = TRUE,
	SINGLEOUT     = FALSE
}R848_DVBS_OutputSignal_Type;

typedef enum _R848_DVBS_AGC_Type
{
	AGC_NEGATIVE = TRUE,
	AGC_POSITIVE = FALSE
}R848_DVBS_AGC_Type;

typedef enum _R848_DVBS_AttenVga_Type
{
	ATTENVGAON = TRUE,
	ATTENVGAOFF= FALSE
}R848_DVBS_AttenVga_Type;

typedef enum _R848_DVBS_FineGain_Type
{
	FINEGAIN_3DB = 0,
	FINEGAIN_2DB,
	FINEGAIN_1DB,
	FINEGAIN_0DB
}R848_DVBS_FineGain_Type;

typedef struct _R848_Set_Info
{
	UINT32                       RF_KHz;
	UINT32						 DVBS_BW;
	R848_Standard_Type           R848_Standard;
	R848_DVBS_OutputSignal_Type  R848_DVBS_OutputSignal_Mode;
	R848_DVBS_AGC_Type           R848_DVBS_AGC_Mode; 
    R848_DVBS_AttenVga_Type	  R848_DVBS_AttenVga_Mode;
	R848_DVBS_FineGain_Type	  R848_DVBS_FineGain;
}R848_Set_Info;

typedef struct _R848_RF_Gain_Info
{
	UINT16   RF_gain_comb;
	UINT8   RF_gain1;
	UINT8   RF_gain2;
	UINT8   RF_gain3;
}R848_RF_Gain_Info;


//----------------------------------------------------------//
//                   R848 Public Function                       //
//----------------------------------------------------------//
#define R848_Delay_MS	AVL_IBSP_Delay


R848_ErrCode R848_Init();
R848_ErrCode R848_SetPllData(R848_Set_Info R848_INFO);
R848_ErrCode R848_Standby();
R848_ErrCode R848_GPO(R848_GPO_Type R848_GPO_Conrl);
R848_ErrCode R848_GetRfGain(R848_RF_Gain_Info *pR848_rf_gain);
R848_ErrCode R848_RfGainMode(R848_RF_Gain_TYPE R848_RfGainType);

#endif
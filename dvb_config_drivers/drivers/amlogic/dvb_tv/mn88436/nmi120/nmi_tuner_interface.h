
#ifndef NMI_TUNER_INTERFACE_H
#define NMI_TUNER_INTERFACE_H
#include "nmiioctl.h"
#include "nmicmn.h"

 
typedef struct {
	tNMI_ALLTVSTD tvstd;   //??ê?
	tNmiOutput output;       //ê?3?ààDí￡?CVBS?ò???D?μê?3?
	tNmiDacSel dacsel;       //DAC?￡ê?
	uint32_t freq;             //RF?μ?ê
	uint32_t if_freq;          //?D?μ?μ?ê
	uint8_t freq_invert;       //RF ê?·?·′?à
	uint8_t if_freq_invert;    //?D?μê?·?·′?à
	uint32_t aif;               //ADUIO?D?μ?μ?ê
	uint8_t is_stereo;         //ê?·?á￠ì?éù
	uint8_t ucBw;             //′??í
	bool_t  bretune;           //ê?·?Dèòa??D????¨
	tNmiSwrfliArg poll_param;
	uint8_t scan_aci[3];  //6mhz  7mhz  8mhz 
	uint8_t play_aci[3];  //6mhz  7mhz  8mhz 
}tNMI_TUNE_PARAM;

//调用此函数，实现NMI TUNER的初始化操作，只在开始时执行
extern uint32_t Nmi_Tuner_Interface_init_chip(tTnrInit* pcfg);

//设置频点，以及运行的制式
extern uint8_t Nmi_Tuner_Interface_Tuning(tNMI_TUNE_PARAM* param);

//获取TUNER的AGC 是否LOCK
extern int Nmi_Tuner_Interface_GetLockStatus(void);

//SLEEP模式
extern void Nmi_Tuner_Interface_Sleep_Lt(void);

//wake up 芯片
extern void Nmi_Tuner_Interface_Wake_Up_Lt(void);

//获取CHIPID
extern uint32_t Nmi_Tuner_Interface_Get_ChipID(void);

//设置寄存器
extern void  Nmi_Tuner_Interface_Wreg(uint32_t addr,uint32_t value);

extern uint32_t  Nmi_Tuner_Interface_Rreg(uint32_t addr);

//获取RSSI
extern  int16_t Nmi_Tuner_Interface_GetRSSI(void);

//DEMOD通知 TUNER  频点是否锁定，DEMOD锁定频点后，调用此函数
extern void Nmi_Tuner_Interface_Demod_Lock(void);

extern uint8_t Nmi_Tuner_Interface_CallBack(void);

#endif



   


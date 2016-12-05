#ifndef NMI_TUNER_OS_H
#define NMI_TUNER_OS_H

#include "nmitypes.h"
#include "nmiioctl_cmn.h"

#if !defined(TRUE) && !defined(FALSE) 
/// definition for TRUE
#define TRUE                        1
/// definition for FALSE
#define FALSE                       0
#endif

//#define NMI_USE_VHF_FILTER

#define nmi_tuner_os_log printk
void nmi_tuner_os_delay(uint32_t msec);
uint32_t nmi_tuner_os_get_tick(void);
void nmi_tuner_os_bus_deinit(void);
int nmi_tuner_os_bus_init(void);
int nmi_tuner_os_bus_write(unsigned char DeviceAddr, unsigned char* pArray, unsigned long count);
int nmi_tuner_os_bus_read(unsigned char DeviceAddr, unsigned char* pArray, unsigned long count);
int nmi_tuner_os_chip_enable(void);
uint8_t nmi_tuner_os_change_iic_mode(uint8_t enable);
uint8_t nmi_tuner_os_get_demod_lock(void);
uint8_t nmi_tuner_os_reset_demod(uint32_t freq);
void nmi_tuner_os_Get_Packet_Error(uint16_t* value);
void nmi_tuner_os_Get_FreqOffset(int32_t *pFreqOff, uint8_t u8BW);
uint8_t nmi_tuner_os_chip_power_on(void);
uint8_t nmi_tuner_os_chip_power_off(void);
uint32_t* nmi_tuner_os_malloc(uint32_t size);
void nmi_tuner_os_free(uint32_t* p);
void nmi_tuner_os_memset( void* mem, int value, uint32_t len);
void nmi_tuner_os_memcpy(void *pu8Des, void *pu8Src, uint32_t len);
typedef void ( *NmiTaskEntry ) (void);
void nmi_tuner_os_create_task(NmiTaskEntry func);
void nmi_tuner_os_delete_task(void);
uint32_t nmi_tuner_os_create_mutex(uint32_t * mutex); 
uint32_t nmi_tuner_os_get_mutex(uint32_t * mutex);
uint32_t nmi_tuner_os_release_mutex(uint32_t * mutex);
uint32_t nmi_tuner_os_delete_mutex(uint32_t * mutex);
#endif



#include "nmi_tuner_os.h"
#include "nmicmn.h"
#include "nmiioctl.h"
//#include "datatype.h"


//#include "drvIIC.h"
//#include "MsOS.h"
//#include "MsCommon.h"
//#include "HbCommon.h"
//#include "Board.h"
//#include"drvTuner.h"
//#include "drvGPIO.h"

//#include "drvDemod.h"
//#include "apiDigiTuner.h"

#include "MN_I2C.h"
#include "nmi_tuner_os.h"


#define TUNER_ENABLE_GPIO  7

//#define NMI_PRINTF_CHAR_MAX 100
/*
void nmi_tuner_os_log(const char *format, ... )
{
	int i, ir;
	MS_U8 szPrint[NMI_PRINTF_CHAR_MAX];
	va_list ap;

	va_start(ap, format);
	ir = _vsnprintf(szPrint, NMI_PRINTF_CHAR_MAX, format, ap);
	va_end(ap);
	nmi_tuner_os_log(szPrint);
}
*/

void nmi_tuner_os_delay(uint32_t msec)
{
	msleep(msec);
}

extern unsigned int jiffies_to_msecs(const unsigned long j);

uint32_t nmi_tuner_os_get_tick(void)
{
	uint32_t time;
	time = jiffies_to_msecs(jiffies);
	return time;	
}

void nmi_tuner_os_bus_deinit(void)
{
   //return 0;
}

int nmi_tuner_os_bus_init(void)
{
   return 0;
}


int nmi_tuner_os_bus_write(unsigned char DeviceAddr, unsigned char* pArray, unsigned long count)
{
	int status = 0;
//	if(MDrv_IIC_Write(DeviceAddr,0,0,pArray,count))
//	printk("DeviceAddr is  %x\n",DeviceAddr);
	if(I2CWrite(DeviceAddr,pArray,count)!=0)
	{
		status = TRUE;
	}
	else
	{
		status =FALSE;
	}
	return (status);
}

int nmi_tuner_os_bus_read(unsigned char DeviceAddr, unsigned char* pArray, unsigned long count)
{
	int status = 0;
//	if(MDrv_IIC_Read(DeviceAddr,0,0,pArray,count))
//	printk("DeviceAddr is  %x\n",DeviceAddr);
	if(I2CRead(DeviceAddr,pArray,count)!=0)
	{
		status = TRUE;
	}
	else
	{
		status =FALSE;
	}

	return (status);    //success
}

int nmi_tuner_os_chip_enable(void)
{
//	mdrv_gpio_set_high(TUNER_ENABLE_GPIO);//LGH ADD FOR ENABLE	2012_3_3
//	MsOS_DelayTask(500);
	return TRUE;
}

uint8_t nmi_tuner_os_change_iic_mode(uint8_t enable)
{

//extern MS_BOOL MSB1233C_IIC_Bypass_Mode(MS_BOOL enable);
//return MSB1233C_IIC_Bypass_Mode(enable);	
//extern MS_BOOL MDrv_Demod_I2C_ByPass(MS_BOOL bOn);
//return MDrv_Demod_I2C_ByPass(enable);

}

uint8_t nmi_tuner_os_get_demod_lock(void)
{	

//return MApi_DigiTuner_TPSGetLock();	 
}

uint8_t nmi_tuner_os_reset_demod(uint32_t freq)
{
#if 0
	extern DEMOD_MS_FE_CARRIER_PARAM _demod_param_120;
	extern MS_BOOL MDrv_Demod_Restart(DEMOD_MS_FE_CARRIER_PARAM *pParam);
	_demod_param_120.u32Frequency = (uint32_t)(freq/1000);
	MDrv_Demod_Restart(&_demod_param_120);
	nmi_tuner_os_log("nmi_tuner_os_reset_demod,freq\n");
#endif
	return TRUE;
}

void nmi_tuner_os_Get_Packet_Error(uint16_t* value)
{
#if 0
	uint16_t u16data;
	extern MS_U8 MSB1231_Get_Packet_Error(MS_U16 *u16_data);
	MSB1231_Get_Packet_Error(&u16data);
	*value = (uint32_t)u16data;
#endif
}

void nmi_tuner_os_Get_FreqOffset(int32_t *pFreqOff, uint8_t u8BW)
{
#if 0
	float FreqOff;
	extern MS_U16 MSB1231_Get_FreqOffset(float *FreqOff, MS_U8 u8BW);
	MSB1231_Get_FreqOffset(&FreqOff, u8BW);
	*pFreqOff = FreqOff;
#endif
}

uint8_t nmi_tuner_os_chip_power_on(void)
{
	return 1;
}

uint8_t nmi_tuner_os_chip_power_off(void)
{
	return 1;
}

uint32_t* nmi_tuner_os_malloc(uint32_t size)
{
	return 0;
}

void nmi_tuner_os_free(uint32_t* p)
{

}
void nmi_tuner_os_memset( void* mem, int value, uint32_t len)
{
	uint32_t i;
	uint8_t* temp = (uint8_t*)mem;
	for(i=0;i<len;i++)
	{
		temp[i]=value;
	}
}

void nmi_tuner_os_memcpy(void *pu8Des, void *pu8Src, uint32_t len)
{
	uint32_t i=0;
	uint8_t* tmpDes = pu8Des;
	uint8_t* tmpSrc = pu8Src;
	for(i=0;i<len;i++)
	{
		tmpDes[i] = tmpSrc[i];
	}
}


extern MS_S32 gs32CachedPoolID ;
static void *_pTaskStack = NULL;
static  MS_S32 _s32TaskId = -1;
#define  TUNER_TASK_STACK  4096
void nmi_tuner_os_create_task(NmiTaskEntry func)
{
/*	if(_s32TaskId != -1)
	{
		nmi_tuner_os_log("nmi_tuner_os_create_task 111111111~~~~~~~~~~~\n");
		return;
	}
	nmi_tuner_os_log("nmi_tuner_os_create_task 222222222222222~~~~~~~~~~~\n");
	_pTaskStack=HB_AllocateMemory(TUNER_TASK_STACK, gs32CachedPoolID);
	_s32TaskId=MsOS_CreateTask((TaskEntry)func, 0, E_TASK_PRI_HIGH, TRUE, _pTaskStack, TUNER_TASK_STACK, "APPTUNER_Task");
*/
}

void nmi_tuner_os_delete_task(void)
{
/*
	if(_s32TaskId!=-1)
	{
		MsOS_DeleteTask(_s32TaskId);
		_s32TaskId = -1;
	}
	if(_pTaskStack!=NULL)
	{
		HB_FreeMemory(_pTaskStack,gs32CachedPoolID);
		_pTaskStack = NULL;
	}
	*/
}
#define NMI_MUTEX_MAX 10
static MS_S32 _s32MutexId[NMI_MUTEX_MAX] = {0}; 
#define TUNER_MUTEX_TIMEOUT         600
uint32_t nmi_tuner_os_create_mutex(uint32_t * mutex)
{
/*	 uint32_t  i;
	 for(i=0;i<NMI_MUTEX_MAX ;i++)
	{
		if(_s32MutexId[i] ==0)
		break;
	}
	if(i<NMI_MUTEX_MAX )
	{
		_s32MutexId[i] = MsOS_CreateMutex(E_MSOS_FIFO, "Digi_Mutex",MSOS_PROCESS_SHARED);
		*mutex = i;
		return 1;
	}
	else
	{
		return 0;
	}*/
	return 1;
}

uint32_t nmi_tuner_os_get_mutex(uint32_t * mutex)
{	
	uint32_t ret=FALSE;
	nmi_tuner_os_log("nmi_tuner_os_get_mutex in %d\n",*mutex);
	ret =  TRUE;//MsOS_ObtainMutex((_s32MutexId[*mutex]),0xFFFFFFFF);
	nmi_tuner_os_log("nmi_tuner_os_get_mutex out %d %d\n",ret,*mutex);
	//nmi_tuner_os_delay(2000);
	return ret;
}

uint32_t nmi_tuner_os_release_mutex(uint32_t * mutex)
{
	uint32_t ret=FALSE;
	nmi_tuner_os_log("nmi_tuner_os_release_mutex mutex in %d\n",*mutex);
	ret =  TRUE;//MsOS_ReleaseMutex((_s32MutexId[*mutex]));
	nmi_tuner_os_log("nmi_tuner_os_release_mutex out%d\n",ret);
	return ret;
}

uint32_t nmi_tuner_os_delete_mutex(uint32_t * mutex)
{
/*
	MsOS_DeleteMutex((_s32MutexId[*mutex]));
	_s32MutexId[*mutex] = 0;
	nmi_tuner_os_log("nmi_tuner_os_delete_mutex\n");*/
	return TRUE;
}

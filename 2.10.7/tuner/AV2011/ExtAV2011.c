/*
 *           Copyright 2007-2014 Availink, Inc.
 *
 *  This software contains Availink proprietary information and
 *  its use and disclosure are restricted solely to the terms in
 *  the corresponding written license agreement. It shall not be 
 *  disclosed to anyone other than valid licensees without
 *  written permission of Availink, Inc.
 *
 */


//#include "gvariable.h"
//#include "II2CRepeater.h"
#include "ExtAV2011.h"

#define AV2011_R0_PLL_LOCK 0x01
#define AV2011_Tuner

static AVL_uint16 gusI2CAddr;
static UINT16 tuner_crystal = 27; //unit is MHz
static AVL_uchar auto_scan=0; //0 for normal mode, 1 for Blindscan mode
static void Time_DELAY_MS(UINT32 ms);
 //Main function at tuner control
static AVL_uint32 Tuner_control(UINT32 channel_freq, UINT32 bb_sym, struct AV2011_Setting * AV2011_Configure);
// I2C write function ( start register, register array pointer, register length)
static AVL_uint32 AV2011_I2C_write(UINT8 reg_start,UINT8* buff,UINT8 len);
AVL_uint32 ExtAV2011_RegInit(AVL_Tuner * pTuner); 

extern struct AVL_DVBSx_Chip gDemodRuby;
extern AVL_semaphore gsemI2C;

AVL_uint32 ExtAV2011_GetLockStatus(struct AVL_Tuner * pTuner)
{
    AVL_uint32 r = 0;
    AVL_uchar uilock = 0x0b;
	AVL_uint16 size;

	size = 1;
	
	r = AVL_IBSP_WaitSemaphore(&(gsemI2C));
	r |= AVL_IBSP_I2C_Write((AVL_uchar)(gusI2CAddr), &uilock, &size);
	r |= AVL_IBSP_I2C_Read((AVL_uchar)(gusI2CAddr), &uilock, &size);
	r |= AVL_IBSP_ReleaseSemaphore(&(gsemI2C));
	
    if( 0 == r ) 
    {
        if( 0 == (uilock & AV2011_R0_PLL_LOCK) ) 
        {
            pTuner->ucTunerLocked = 0;
            r = 1;
        }
        else
        {
            pTuner->ucTunerLocked = 1;
        }
    }
    return(r);
}

AVL_uint32 ExtAV2011_Initialize(struct AVL_Tuner * pTuner)
{
    AVL_uint32 r = 0;
    
    gusI2CAddr = pTuner->usTunerI2CAddr;

    r = ExtAV2011_RegInit(pTuner); //init all tuner register
    
    return(r);  
}


AVL_uint32 ExtAV2011_RegInit(struct AVL_Tuner *pTuner)
{
    UINT8 reg[50];
    AVL_uint32 r;
    
    //Tuner initial registers R0~R41
    reg[0]=(char) (0x38);
    reg[1]=(char) (0x00);
    reg[2]=(char) (0x00);
    reg[3]=(char) (0x50);
    reg[4]=(char) (0x1f);
    reg[5]=(char) (0xa3);
    reg[6]=(char) (0xfd);
    reg[7]=(char) (0x58);
    reg[8]=(char) (0x0e);
    reg[9]=(char) (0x82);
    reg[10]=(char) (0x88);
    reg[11]=(char) (0xb4);
    reg[12]=(char) (0xd6); //RFLP=ON at Power on initial
    reg[13]=(char) (0x40);
#ifdef AV2011_Tuner
    reg[14]=(char) (0x94);
    reg[15]=(char) (0x4a);
#else
    reg[14]=(char) (0x5b);
    reg[15]=(char) (0x6a);
#endif
    reg[16]=(char) (0x66);
    reg[17]=(char) (0x40);
    reg[18]=(char) (0x80);
    reg[19]=(char) (0x2b);
    reg[20]=(char) (0x6a);
    reg[21]=(char) (0x50);
    reg[22]=(char) (0x91);
    reg[23]=(char) (0x27);
    reg[24]=(char) (0x8f);
    reg[25]=(char) (0xcc);
    reg[26]=(char) (0x21);
    reg[27]=(char) (0x10);
    reg[28]=(char) (0x80);
    reg[29]=(char) (0x02);
    reg[30]=(char) (0xf5);
    reg[31]=(char) (0x7f);
    reg[32]=(char) (0x4a);
    reg[33]=(char) (0x9b);
    reg[34]=(char) (0xe0);
    reg[35]=(char) (0xe0);
    reg[36]=(char) (0x36);
    reg[37]=(char) (0x00); //Disable FT-function at Power on initial
    reg[38]=(char) (0xab);
    reg[39]=(char) (0x97);
    reg[40]=(char) (0xc5);
    reg[41]=(char) (0xa8);
    
        // Sequence 1
        // Send Reg0 ->Reg11
        r =  AV2011_I2C_write(0,reg,12);
        if(r!=0)
        {
            return(r);
        }
        // Time delay 1ms
        Time_DELAY_MS(1);
        // Sequence 2
        // Send Reg13 ->Reg24
        r = AV2011_I2C_write(13,reg+13,12);
        if(r!=0)
        {
            return(r);
        }
        // Send Reg25 ->Reg35
        r = AV2011_I2C_write(25,reg+25,11);
        if(r!=0)
        {
            return(r);
        }
        // Send Reg36 ->Reg41
        r = AV2011_I2C_write(36,reg+36,6);
        if(r!=0)
        {
            return(r);
        }
        // Time delay 1ms
        Time_DELAY_MS(1);
        // Sequence 3
        // send reg12
        r = AV2011_I2C_write(12,reg+12,1);
        if(r!=0)
        {
            return(r);
        }
        //monsen 20081125, Wait 100 ms
        Time_DELAY_MS(10);
        //monsen 20081030, re initial again
        {
            // Sequence 1
            // Send Reg0 ->Reg11
            r = AV2011_I2C_write(0,reg,12);
            if(r!=0)
            {
                return(r);
            }
            // Time delay 1ms
            Time_DELAY_MS(1);
            // Sequence 2
            // Send Reg13 ->Reg24       
            r = AV2011_I2C_write(13,reg+13,12);
            if(r!=0)
            {
                return(r);
            }
            // Send Reg25 ->Reg35
            r = AV2011_I2C_write(25,reg+25,11);
            if(r!=0)
            {
                return(r);
            }
            // Send Reg36 ->Reg41
            r = AV2011_I2C_write(36,reg+36,6);
            if(r!=0)
            {
                return(r);
            }
            // Time delay 1ms
            Time_DELAY_MS(1);
            // Sequence 3
            // send reg12
            r = AV2011_I2C_write(12,reg+12,1);
            if(r!=0)
            {
                return(r);
            }
            Time_DELAY_MS(5);
            return(r);
        }
}

AVL_uint32 Tuner_control(UINT32 channel_freq, UINT32 tuner_lpf, struct AV2011_Setting * AV2011_Configure)
{
    UINT8 reg[50];
    UINT32 fracN;
    UINT32 BW;
    UINT32 BF;
    AVL_uint32 r = 0;
	AVL_uchar uia_state = 29;
	AVL_uchar uib_state = 29;
	AVL_uint16 size = 1;

    Time_DELAY_MS(50);

#if 1
	//add for delay 100ms and send R0~R3	-start-0
	
	r = AVL_IBSP_WaitSemaphore(&(gsemI2C));
	r |= AVL_IBSP_I2C_Write((AVL_uchar)(gusI2CAddr),&uia_state,&size);
	r |= AVL_IBSP_I2C_Read((AVL_uchar)(gusI2CAddr), &uia_state, &size);
	r |= AVL_IBSP_ReleaseSemaphore(&(gsemI2C));

	if (0 != r)
	{
		return (r);
	}
	uia_state &= 0x3;

	//add for delay 100ms and send R0~R3   -end-0
#endif

    fracN = (channel_freq + tuner_crystal/2)/tuner_crystal;
    if(fracN > 0xff)
    fracN = 0xff;
    reg[0]=(char) (fracN & 0xff);
    fracN = (channel_freq<<17)/tuner_crystal;
    fracN = fracN & 0x1ffff;
    reg[1]=(char) ((fracN>>9)&0xff);
    reg[2]=(char) ((fracN>>1)&0xff);
    // reg[3]_D7 is frac<0>, D6~D0 is 0x50
#ifdef AV2011_Tuner
    //AV2011 IQ Single_end/Differential mode at bit2
    reg[3]=(char) (((fracN<<7)&0x80) | 0x50 |(AV2011_Configure->Single_End)<<2);
#else
    //AV2020 no IQ mode
    reg[3]=(char) (((fracN<<7)&0x80) | 0x50 );
#endif
    // Channel Filter Bandwidth Setting.
    //"sym" unit is Hz;
    if(AV2011_Configure->Auto_Scan)//auto scan requested by BB
    {
        reg[5] = 0xA3; //BW=27MHz
        // Sequence 4
        // Send Reg0 ->Reg4
        r = AV2011_I2C_write(0,reg,4 );
        if(r!=0)
        {
            return(r);
        }
        Time_DELAY_MS(5);

        // Sequence 5
        // Send Reg5
        r = AV2011_I2C_write(5, reg+5, 1);
        if(r!=0)
        {
            return(r);
        }
        Time_DELAY_MS(5);

        // Fine-tune Function Control
        // Auto-scan mode 
        // FT_block=1, FT_EN=0, FT_hold=1
        reg[37] = 0x05;
        r = AV2011_I2C_write(37, reg+37, 1);
        if(r!=0)
        {
            return(r);
        }
        // Time delay 4ms
        Time_DELAY_MS(4);
    }
    else
    {
        BW = tuner_lpf;
        // Bandwidth can be tuned from 4M to 40M
        if( BW< 4000)
        {
            BW = 4000;
        }
        if( BW> 40000)
        {
            BW = 40000;
        }
        BF = (BW*127 + 21100/2) / (21100); 
        reg[5] = (UINT8)BF;

        // Sequence 4
        // Send Reg0 ->Reg4
        Time_DELAY_MS(5);
        r = AV2011_I2C_write(0,reg,4);
        if(r!=0)
        {
            return(r);
        }
        Time_DELAY_MS(5);
        //r = AV2011_I2C_write(0,reg,4);
        //if(r!=0)
        //{
        //    return(r);
        //}
        //Time_DELAY_MS(5);

#if 1
		// add for delay 100ms and send R0~R3   -start-1
		
		r = AVL_IBSP_WaitSemaphore(&(gsemI2C));
		r |= AVL_IBSP_I2C_Write((AVL_uchar)(gusI2CAddr),&uib_state,&size);
		r |= AVL_IBSP_I2C_Read((AVL_uchar)(gusI2CAddr), &uib_state, &size);				
		r |= AVL_IBSP_ReleaseSemaphore(&(gsemI2C));

		if (0 != r)
		{
			return (r);
		}
		uib_state &= 0x3;// get bit 0~1
		if((uia_state!=3) && (uib_state==3))
		{
			Time_DELAY_MS(100);
			r = AV2011_I2C_write(0,reg,4);
			if( 0 != r ) 
			{
				return(r);
			}
			Time_DELAY_MS(5);
		}
		// add for delay 100ms and send R0~R3 -End-1	
#endif

        // Sequence 5
        // Send Reg5
        r = AV2011_I2C_write(5, reg+5, 1);
        if(r!=0)
        {
            return(r);
        }
        Time_DELAY_MS(5);
        
        // Fine-tune Function Control
        // not auto-scan case. enable block function.
        // FT_block=1, FT_EN=1, FT_hold=0
        reg[37] = 0x04 | ((AV2011_Configure->AutoGain)<<1);
        r = AV2011_I2C_write(37, reg+37, 1);
        if(r!=0)
        {
        return(r);
        }
        Time_DELAY_MS(5);
        //Disable RFLP at Lock Channel sequence after reg[37]
        //RFLP=OFF at Lock Channel sequence
        // RFLP can be Turned OFF, only at Receiving mode.
        reg[12] = 0x96 + ((AV2011_Configure->RF_Loop)<<6);
        r = AV2011_I2C_write(12, reg+12, 1);
        if(r!=0)
        {
            return(r);
            Time_DELAY_MS(5);
        }
    }
    return r;
}

static void Time_DELAY_MS(UINT32 ms)
{
    AVL_IBSP_Delay(ms);
}

static AVL_uint32 AV2011_I2C_write(UINT8 reg_start,UINT8* buff,UINT8 len)
{
    AVL_uint32 r=0;
    AVL_uint16 uiTimeOut = 0;
    AVL_uchar ucTemp[50];
    int i = 0;
	AVL_uint16 size;
    
    Time_DELAY_MS(5);
    ucTemp[0] = reg_start;

    for(i=1;i<len+1;i++)
    {
        ucTemp[i]=*(buff+i-1);
    }

	size = len + 1;
	
		r = AVL_IBSP_WaitSemaphore(&(gsemI2C));
		r = AVL_IBSP_I2C_Write((AVL_uchar) gusI2CAddr,ucTemp,&size);
		r = AVL_IBSP_ReleaseSemaphore(&(gsemI2C));
	
    
    if(r != 0)
    {
        return(r);
    }
    Time_DELAY_MS(1);
    
    return(r);
}

AVL_uint32 ExtAV2011_Lock(struct AVL_Tuner *pTuner)
{
    AVL_uint32 r = 0;
    AV2011_Setting AV2011_Set;

    AV2011_Set.Auto_Scan = 0;  //Default Setting: Normal lock mode
    AV2011_Set.AutoGain = 1;   //Default Setting: Auto Gain control on
    AV2011_Set.Single_End = 1; //Default setting: IQ Differential mode
    AV2011_Set.RF_Loop = 1;    //Default setting: open RF loop through

    r = Tuner_control(pTuner->uiRFFrequencyHz/1000000, pTuner->uiLPFHz/1000,&AV2011_Set);

    return(r);
}

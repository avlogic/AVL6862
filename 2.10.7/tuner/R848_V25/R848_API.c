
#include "R848_API.h"

AVL_Tuner *pTuner_ForR848;

#include "stdio.h"

//only for debug
AVL_uint32 R848_ReadReg()
{
    I2C_LEN_TYPE i2c_para;

    int index = 47;

    pTuner_ForR848->usTunerI2CAddr = 0xF4;//0x7A;

    i2c_para.RegAddr = 0x00;
    i2c_para.Len     = 48;
    if(I2C_Read_Len(&i2c_para) != RT_Success)
    {
        return 1;
    }

    printf("print Reg value begin---tuner lock:\n");
	for(int i = 0;i<=index;i++)
	{		  
      printf("0x%X\n",i2c_para.Data[i]);		 
	}
    printf("print Reg value end:\n");
    return 0;
}


AVL_uint32 R848_Initialize(AVL_Tuner * pTuner )
{
    R848_ErrCode r = RT_Success;

    pTuner_ForR848 = pTuner;  
    r = R848_Init();
    if (r != RT_Success)
    {
        return 1;
    } 

    return 0;
}

AVL_uint32 R848_Lock(AVL_Tuner * pTuner )
{
    R848_ErrCode r = RT_Success;
    R848_Set_Info R848_INFO;
    
    pTuner_ForR848 = pTuner;

    R848_INFO.RF_KHz = pTuner->uiRFFrequencyHz/1000;

    if(pTuner->eDTVMode == DTVMode_DVBTX)
    {
        if(pTuner->uiBandwidthHz == 8*1000*1000)
        {
            R848_INFO.R848_Standard = R848_DVB_T2_8M_IF_5M;
        }
        else if(pTuner->uiBandwidthHz == 7*1000*1000)
        {
            R848_INFO.R848_Standard = R848_DVB_T2_7M_IF_5M;
        }
        else if(pTuner->uiBandwidthHz == 6*1000*1000)
        {
            R848_INFO.R848_Standard = R848_DVB_T2_6M_IF_5M;
        }
        else if(pTuner->uiBandwidthHz == 1700*1000)
        {
            R848_INFO.R848_Standard = R848_DVB_T2_1_7M_IF_5M;
        }
		else
		{
			r = RT_Fail;
		}
    }
    else if(pTuner->eDTVMode == DTVMode_ISDBT)
    {
        if(pTuner->uiBandwidthHz == 6*1000*1000)
        {
            R848_INFO.R848_Standard = R848_ISDB_T_IF_5M;
        }
		else
		{
			r = RT_Fail;
		}
    }
    else if(pTuner->eDTVMode == DTVMode_DVBC)
    {
        if(pTuner->uiBandwidthHz <= 6*1000*1000)
        {
            R848_INFO.R848_Standard = R848_DVB_C_6M_IF_5M;
        }
        else if(pTuner->uiBandwidthHz <= 8*1000*1000)
        {
            R848_INFO.R848_Standard = R848_DVB_C_8M_IF_5M;
        }
		else
		{
			r = RT_Fail;
		}
    }
    else if(pTuner->eDTVMode == DTVMode_DTMB)
    {
        R848_INFO.R848_Standard = R848_DTMB_IF_5M;
    }
    else if(pTuner->eDTVMode == DTVMode_DVBSX)
    {
        R848_INFO.R848_Standard = R848_DVB_S;
        R848_INFO.DVBS_BW = pTuner->uiLPFHz/1000 * 2;//unit KHz
        R848_INFO.R848_DVBS_OutputSignal_Mode = DIFFERENTIALOUT;
        R848_INFO.R848_DVBS_AGC_Mode = AGC_NEGATIVE;
    }
	else
	{
		r = RT_Fail;
	}
    
	if(r != RT_Fail)
	{
      r = R848_SetPllData(R848_INFO);	
	}
	else
	{
      return 1;
	}

    if (r != RT_Success)
    {
        return 1;
    }
    return 0;
}



AVL_uint32 R848_GetLockStatus(AVL_Tuner * pTuner)
{
    I2C_LEN_TYPE i2c_para;

    pTuner_ForR848 = pTuner;
    i2c_para.RegAddr = 0x00;
    i2c_para.Len     = 3;
    if(I2C_Read_Len(&i2c_para) != RT_Success)
    {
        return 1;
    }

    if( (i2c_para.Data[2] & 0x40) == 0x00 ) 
    {
        pTuner->ucTunerLocked = 0; //tuner unlock
    }
    else
    {
        pTuner->ucTunerLocked = 1; //tuner lock
    }

    return 0;
}

AVL_uint32  R848_GetGain(AVL_Tuner *pTuner, AVL_uint32 *puiRFGain)
{
    R848_RF_Gain_Info stRFGain;

    R848_GetRfGain(&stRFGain);
    
    if(pTuner->eDTVMode == DTVMode_DVBSX)
    {
        *puiRFGain = stRFGain.RF_gain1;
    }
    else
    {
        *puiRFGain = stRFGain.RF_gain_comb;
    }
    
    return 0;
}


AVL_int32  R848_RFPowerCalc(AVL_uint32 uiRFGain,AVL_uint16 usSSI)
{
    AVL_int32 usPower =0;
 
    if(uiRFGain<19)
    { //DBVSX
        
        //calc the power
        switch(uiRFGain)
            {
                case 18:
                    if(usSSI>29000)
                    {
                        usPower=79-	(usSSI-29000)/380;
                    }
                    else
                    { 
                        if(usSSI>23000)
                           {
                               usPower=109-(usSSI-20000)/210;
                           }
                        else
                           {
                             if(usSSI==0)
                               {
                                   usPower=95;
                               }
                             else
                               {
                                   usPower=109-(usSSI-20000)/110;
                               }
                           }
                    }
                    break;
                case 17:
                    if(usSSI>38000)
                    {
                        usPower=50;
                    }
                    else
                    {
                        usPower=63-(usSSI-33000)/360;
                    }
                    break;
                case 16:
                    if(usSSI>37500)
                    {
                        usPower=49;
                    }
                    else
                    {
                        usPower=62-(usSSI-33000)/360;
                    }
                    break;
                case 15:
                    if(usSSI>37500)
                    {
                        usPower=48;
                    }
                    else
                    {
                        usPower=61-(usSSI-33000)/360;
                    }
                    break;
                case 14:
                    if(usSSI>37500)
                    {
                        usPower=47;
                    }
                    else
                    {
                        usPower=58-(usSSI-33000)/360;
                    }
                    break;
                case 13:
                    if(usSSI>37500)
                    {
                        usPower=42;
                    }
                    else
                    {
                        usPower=55-(usSSI-33000)/360;
                    }
                    break;
                case 12:
                    if(usSSI>37500)
                    {
                        usPower=38;
                    }
                    else
                    {
                        usPower=51-(usSSI-33000)/360;
                    }
                    break;
                case 11:
                    if(usSSI>37500)
                    {
                        usPower=37;
                    }
                    else
                    {
                        usPower=49-(usSSI-33000)/360;
                    }
                    break;
                case 10:
                    if(usSSI>37500)
                    {
                        usPower=33;
                    }
                    else
                    {
                        usPower=47-(usSSI-33000)/360;
                    }
                    break;
                case 9:
                    if(usSSI>37500)
                    {
                        usPower=33;
                    }
                    else
                    {
                        usPower=47-(usSSI-33000)/360;
                    }
                    break;
                case 8:
                    if(usSSI>37500)
                    {
                        usPower=35;
                    }
                    else
                    {
                        usPower=45-(usSSI-33000)/360;
                    }
                    break;
                case 7:
                    if(usSSI>37500)
                    {
                        usPower=27;
                    }
                    else
                    {
                        usPower=45-(usSSI-33000)/360;
                    }
                    break;
                case 6:
                    if(usSSI>37500)
                    {
                        usPower=31;
                    }
                    else
                    {
                        usPower=42-(usSSI-33000)/360;
                    }
                    break;
                case 5:
                    if(usSSI>38000)
                    {
                        usPower=25;
                    }
                    else
                    {
                        usPower=39-(usSSI-33000)/360;
                    }
                    break;
                case 4:
                    if(usSSI>38000)
                    {
                        usPower=24;
                    }
                    else
                    {
                        usPower=37-(usSSI-33000)/360;
                    }
                    break;
                case 3:
                    if(usSSI>38000)
                    {
                        usPower=21;
                    }
                    else
                    {
                        usPower=33-(usSSI-33000)/360;
                    }
                    break;
                case 2:
                    if(usSSI>37500)
                    {
                        usPower=17;
                    }
                    else
                    {
                        usPower=31-(usSSI-33000)/360;
                    }
                    break;
                case 1:
                    if(usSSI>37500)
                    {
                        usPower=15;
                    }
                    else
                    {
                        usPower=29-(usSSI-33000)/360;
                    }
                    break;
                case 0:
                    if(usSSI>37500)
                    {
                        usPower=11;
                    }
                    else
                    {
                        usPower=25-(usSSI-33000)/360;
                    }
                    break;
                default:
                    break;
            }
    }
    else
    {
        
        //calc the power
        
        if(608==uiRFGain)
        {        
            if( 0 == usSSI)
            {
                usPower=95;
            }
            else
            {
                if(usSSI<37000)
                {
                   usPower=95-(usSSI-28000)/500;
                }
                else
                {
                   usPower=85-(usSSI-35000)/230;
                }
             }
        }
        else
        {
           if((usSSI+(608-uiRFGain)*15)<46000)
           {
               usPower=73-(usSSI+(608-uiRFGain)*15-40000)/153;
           }
           else
           {
               usPower=74-(usSSI+(608-uiRFGain)*15-40000)/153;
           }
           }

    }
    
    if(usPower>100)
    {
        usPower=100;
    }
    else
    {
        if(usPower<0)
          usPower=0;
    }

    return (-usPower);
}

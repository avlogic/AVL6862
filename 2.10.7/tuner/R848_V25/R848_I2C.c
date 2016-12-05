#include "R848_I2C.h"

extern AVL_Tuner *pTuner_ForR848;
extern AVL_semaphore gsemI2C;

bool I2C_Write_Len(I2C_LEN_TYPE *I2C_Info)
{
    AVL_uint32 r = 0;
    AVL_uchar buffer[64] = {0};
    AVL_uint16 i = 0;
    AVL_uint16 size = 0;

    buffer[0] = I2C_Info->RegAddr;
    for(i = 0;i < I2C_Info->Len; i++)
    {
        buffer[1+i] = I2C_Info->Data[i];
    }

    size = I2C_Info->Len+1;
	r = AVL_IBSP_WaitSemaphore(&(gsemI2C));
    r = AVL_IBSP_I2C_Write(pTuner_ForR848->usTunerI2CAddr,buffer,&size);
	r |= AVL_IBSP_ReleaseSemaphore(&(gsemI2C));

    if( r != 0)
    {
        return false;
    }

    return true;
}


bool I2C_Read_Len(I2C_LEN_TYPE *I2C_Info)
{
    AVL_uint32 r = 0;
    AVL_uchar buffer[64] = {0};
    AVL_uint16 i = 0;
    AVL_uint16 size = 0;

    size = I2C_Info->Len;
	r = AVL_IBSP_WaitSemaphore(&(gsemI2C));
    r = AVL_IBSP_I2C_Read(pTuner_ForR848->usTunerI2CAddr,buffer,&size);
	r |= AVL_IBSP_ReleaseSemaphore(&(gsemI2C));

    if(r != 0)
    {
        return false;
    }

    for(i = 0;i < size;i++)
    {
        I2C_Info->Data[i] = R848_Convert(buffer[i]);
    }

    return true;
}

bool I2C_Write(I2C_TYPE *I2C_Info)
{
    AVL_uint32 r = 0;
    I2C_LEN_TYPE i2c_para;

    i2c_para.RegAddr = I2C_Info->RegAddr;
    i2c_para.Data[0] = I2C_Info->Data;
    i2c_para.Len = 1;

    if (I2C_Write_Len(&i2c_para) != true)
    {
        return false;
    }

    return true;
}

int R848_Convert(int InvertNum)
{
    int ReturnNum = 0;
    int AddNum    = 0x80;
    int BitNum    = 0x01;
    int CuntNum   = 0;

    for(CuntNum = 0;CuntNum < 8;CuntNum ++)
    {
        if(BitNum & InvertNum)
            ReturnNum += AddNum;

        AddNum /= 2;
        BitNum *= 2;
    }

    return ReturnNum;
}

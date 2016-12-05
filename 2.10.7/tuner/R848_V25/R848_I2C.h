#include "AVL_Tuner.h"

#define TRUE	0
#define FALSE	1
#define BOOL	bool

//----------------------------------------------------------//
//                   Type Define                                    //
//----------------------------------------------------------//
#define UINT8  unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned long


typedef struct _R848_I2C_LEN_TYPE
{
	UINT8 RegAddr;
	UINT8 Data[50];
	UINT8 Len;
}I2C_LEN_TYPE;

typedef struct _R848_I2C_TYPE
{
	UINT8 RegAddr;
	UINT8 Data;
}I2C_TYPE;

bool I2C_Write_Len(I2C_LEN_TYPE *I2C_Info);
bool I2C_Read_Len(I2C_LEN_TYPE *I2C_Info);
bool I2C_Write(I2C_TYPE *I2C_Info);
int R848_Convert(int InvertNum);
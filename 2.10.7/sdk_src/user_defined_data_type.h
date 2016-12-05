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



#ifndef USER_DEFINED_DATA_TYPE_H
#define USER_DEFINED_DATA_TYPE_H

#ifdef AVL_CPLUSPLUS
extern "C" {
#endif
    
typedef  char AVL_char;     ///< 8 bits signed char data type.
typedef  unsigned char AVL_uchar;   ///< 8 bits unsigned char data type.

typedef  short AVL_int16;   ///< 16 bits signed char data type.
typedef  unsigned short AVL_uint16; ///< 16 bits unsigned char data type.

typedef  int AVL_int32;     ///< 32 bits signed char data type.
typedef  unsigned int AVL_uint32;   ///< 32 bits unsigned char data type.

typedef  char * AVL_pchar;  ///< pointer to a 8 bits signed char data type.
typedef  unsigned char * AVL_puchar; ///< pointer to a 8 bits unsigned char data type.

typedef  short * AVL_pint16;    ///< pointer to a 16 bits signed char data type.
typedef  unsigned short * AVL_puint16;  ///< pointer to a 16 bits unsigned char data type.

typedef  int * AVL_pint32;  ///< pointer to a 32 bits signed char data type.
typedef  unsigned int * AVL_puint32; ///< pointer to a 32 bits unsigned char data type.

typedef unsigned char AVL_semaphore;    ///< the semaphore data type.
typedef unsigned char * AVL_psemaphore;     ///< the pointer to a semaphore data type.

#define MAX_II2C_READ_SIZE  64
#define MAX_II2C_WRITE_SIZE 64


typedef AVL_uint16 AVL_ErrorCode;       // Defines the error code 

#ifdef AVL_CPLUSPLUS
}
#endif

#endif


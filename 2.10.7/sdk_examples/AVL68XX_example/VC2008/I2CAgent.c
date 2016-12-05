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



#include "user_defined_function.h"

#using <System.dll>
using namespace System;
using namespace System::Threading;
using namespace System::Net;
using namespace System::Net::Sockets;

#define SHARP_TUNER_SLAVE_ADDR 0x61 

//AA_GPIO_BITS
#define AA_GPIO_SCL         0x01        ///< Pin 1
#define  AA_GPIO_SDA        0x02        ///< Pin 3
#define AA_GPIO_MISO        0x04        ///< Pin 5
#define AA_GPIO_SCK         0x08        ///< Pin 7
#define AA_GPIO_MOSI        0x10        ///< Pin 8
#define  AA_GPIO_SS         0x20        ///< Pin 9

AVL_uint32 AVL_IBSP_AAGPIO_SetDirection( AVL_uchar ucDirectionMask );
AVL_uint32 AVL_IBSP_AAGPIO_SetValue( AVL_uchar ucValueMask );
AVL_uint32 AVL_IBSP_AAGPIO_GetValue( AVL_puchar pucValueMask );

const static AVL_uchar ucGPIODirMask = (AA_GPIO_MISO | AA_GPIO_MOSI | AA_GPIO_SCK) & (~AA_GPIO_SS);  //Pin9 is set up to input
const static AVL_uchar ucGPIOValueMask = AA_GPIO_MISO;  //only the reset pin output high, all others output low.
const static AVL_uchar ucGPIOResetPin = AA_GPIO_MISO;


namespace AVL_GLOBAL
{
	public ref class AVL_global
	{
	public: static System::Net::Sockets::Socket^ ClientSocket; 	
	public: static array<AVL_uchar>^ I2CBuff;
	public: static array<AVL_uchar>^ GPIOBuff;	
	public: static array<System::Threading::Semaphore^>^ g_semaphores;
	public: static unsigned char sem_ID;
	public: static System::Threading::Semaphore^ GPIO_sem;
	public: static System::Net::Sockets::Socket^ ClientSocket0; 
	public: static array<AVL_uchar>^ I2CBuff0;
	public: static array<AVL_uchar>^ GPIOBuff0;
	public: static System::Threading::Semaphore^ GPIO_sem0;
	public: static System::Net::Sockets::Socket^ ClientSocket1; 
	public: static array<AVL_uchar>^ I2CBuff1;
	public: static array<AVL_uchar>^ GPIOBuff1;
	public: static System::Threading::Semaphore^ GPIO_sem1;
	};
}

AVL_uint32 AVL_IBSP_Delay( AVL_uint32 uiDelay_ms )
{
	System::Threading::Thread::Sleep(uiDelay_ms);
	return(0);
}

AVL_uint32 AVL_IBSP_Dispose(void)
{
	return(0);   
}

AVL_uint32 AVL_IBSP_I2C_Read(  AVL_uint16 uiSlaveAddr,  AVL_puchar pucBuff, AVL_puint16 puiSize )
{
	int i ,j; 
	
	if((0x14 != uiSlaveAddr) && (0x15 != uiSlaveAddr) && (SHARP_TUNER_SLAVE_ADDR != uiSlaveAddr)&& (nullptr != AVL_GLOBAL::AVL_global::ClientSocket1))
	{
		AVL_GLOBAL::AVL_global::ClientSocket = AVL_GLOBAL::AVL_global::ClientSocket1;
		AVL_GLOBAL::AVL_global::I2CBuff = AVL_GLOBAL::AVL_global::I2CBuff1;
		AVL_GLOBAL::AVL_global::GPIOBuff = AVL_GLOBAL::AVL_global::GPIOBuff1;
		AVL_GLOBAL::AVL_global::GPIO_sem = AVL_GLOBAL::AVL_global::GPIO_sem1;
	}
	else
	{
		AVL_GLOBAL::AVL_global::ClientSocket = AVL_GLOBAL::AVL_global::ClientSocket0;
		AVL_GLOBAL::AVL_global::I2CBuff = AVL_GLOBAL::AVL_global::I2CBuff0;
		AVL_GLOBAL::AVL_global::GPIOBuff = AVL_GLOBAL::AVL_global::GPIOBuff0;
		AVL_GLOBAL::AVL_global::GPIO_sem = AVL_GLOBAL::AVL_global::GPIO_sem0;
	}

	AVL_GLOBAL::AVL_global::I2CBuff[0] = 5;
	AVL_GLOBAL::AVL_global::I2CBuff[1] = 1;
	AVL_GLOBAL::AVL_global::I2CBuff[2] = (AVL_uchar) (*puiSize);
	AVL_GLOBAL::AVL_global::I2CBuff[3] = (AVL_uchar)(uiSlaveAddr>>8);
	AVL_GLOBAL::AVL_global::I2CBuff[4] = (AVL_uchar)(uiSlaveAddr);
	try
	{
		AVL_GLOBAL::AVL_global::ClientSocket->Send(AVL_GLOBAL::AVL_global::I2CBuff, AVL_GLOBAL::AVL_global::I2CBuff[0], Sockets::SocketFlags::None);
		i = AVL_GLOBAL::AVL_global::ClientSocket->Receive(AVL_GLOBAL::AVL_global::I2CBuff);
		j = AVL_GLOBAL::AVL_global::I2CBuff[0];
		while( i<j )	  //the following loop code may not be used for ever.
		{
			array<AVL_uchar> ^ tempBuff = gcnew array<AVL_uchar>(256);
			int k1, k2;
			k1 = AVL_GLOBAL::AVL_global::ClientSocket->Receive(tempBuff);
			for( k2=0; k2<k1; k2++ )
			{
				AVL_GLOBAL::AVL_global::I2CBuff[k2+i] = tempBuff[k2]; 
			}
			i += k1;
		}
	}
	catch( System::Net::Sockets::SocketException^ )
	{
		return(4);
	}

	if( *puiSize != AVL_GLOBAL::AVL_global::I2CBuff[1] )
	{
		*puiSize = AVL_GLOBAL::AVL_global::I2CBuff[1];
		return(4);
	}
	for( i=0; i<AVL_GLOBAL::AVL_global::I2CBuff[1]; i++ )
	{
		pucBuff[i] = AVL_GLOBAL::AVL_global::I2CBuff[i+2];
	}
	return(0);
}

AVL_uint32 AVL_IBSP_I2C_Write(  AVL_uint16 uiSlaveAddr,  AVL_puchar pucBuff,  AVL_puint16  puiSize )
{
	int i ,j; 

	if((0x14 != uiSlaveAddr) && (0x15 != uiSlaveAddr) && (SHARP_TUNER_SLAVE_ADDR != uiSlaveAddr)&& (nullptr != AVL_GLOBAL::AVL_global::ClientSocket1))
	{
		AVL_GLOBAL::AVL_global::ClientSocket = AVL_GLOBAL::AVL_global::ClientSocket1;
		AVL_GLOBAL::AVL_global::I2CBuff = AVL_GLOBAL::AVL_global::I2CBuff1;
		AVL_GLOBAL::AVL_global::GPIOBuff = AVL_GLOBAL::AVL_global::GPIOBuff1;
		AVL_GLOBAL::AVL_global::GPIO_sem = AVL_GLOBAL::AVL_global::GPIO_sem1;
	}
	else
	{
		AVL_GLOBAL::AVL_global::ClientSocket = AVL_GLOBAL::AVL_global::ClientSocket0;
		AVL_GLOBAL::AVL_global::I2CBuff = AVL_GLOBAL::AVL_global::I2CBuff0;
		AVL_GLOBAL::AVL_global::GPIOBuff = AVL_GLOBAL::AVL_global::GPIOBuff0;
		AVL_GLOBAL::AVL_global::GPIO_sem = AVL_GLOBAL::AVL_global::GPIO_sem0;
	}

	AVL_GLOBAL::AVL_global::I2CBuff[0] = 5+(*puiSize);
	AVL_GLOBAL::AVL_global::I2CBuff[1] = 0;
	AVL_GLOBAL::AVL_global::I2CBuff[2] = (AVL_uchar) (*puiSize);
	AVL_GLOBAL::AVL_global::I2CBuff[3] = (AVL_uchar)(uiSlaveAddr>>8);
	AVL_GLOBAL::AVL_global::I2CBuff[4] = (AVL_uchar)(uiSlaveAddr);
	for( i=0; i<(*puiSize); i++ )
	{
		AVL_GLOBAL::AVL_global::I2CBuff[5+i] = pucBuff[i];
	}
	try
	{
		AVL_GLOBAL::AVL_global::ClientSocket->Send(AVL_GLOBAL::AVL_global::I2CBuff, AVL_GLOBAL::AVL_global::I2CBuff[0], Sockets::SocketFlags::None);
		i = AVL_GLOBAL::AVL_global::ClientSocket->Receive(AVL_GLOBAL::AVL_global::I2CBuff);
		j = AVL_GLOBAL::AVL_global::I2CBuff[0];
		while( i<j )	  //the following loop code may not be used for ever.
		{
			array<AVL_uchar> ^ tempBuff = gcnew array<AVL_uchar>(256);
			int k1, k2;
			k1 = AVL_GLOBAL::AVL_global::ClientSocket->Receive(tempBuff);
			for( k2=0; k2<k1; k2++ )
			{
				AVL_GLOBAL::AVL_global::I2CBuff[k2+i] = tempBuff[k2]; 
			}
			i += k1;
		}
	}
	catch( System::Net::Sockets::SocketException^ )
	{
		return(4 );
	}

	if( *puiSize != AVL_GLOBAL::AVL_global::I2CBuff[1] )
	{
		return(4);
	}
	return(0);
}

AVL_uint32 AVL_IBSP_Initialize(System::String^ strServerName0, int iSocketPort0,System::String^ strServerName1, int iSocketPort1)
{
#if 1
	if( 0 == strServerName0->CompareTo("localhost") )
	{
		strServerName0 = Dns::GetHostName();
	}
	if((nullptr != strServerName1) &&( 0 == strServerName1->CompareTo("localhost")))
	{
		strServerName1 = Dns::GetHostName();
	}
	AVL_GLOBAL::AVL_global::ClientSocket0 = gcnew System::Net::Sockets::Socket(System::Net::Sockets::AddressFamily::InterNetwork, System::Net::Sockets::SocketType::Stream, System::Net::Sockets::ProtocolType::Tcp);
	AVL_GLOBAL::AVL_global::I2CBuff0 = gcnew array<AVL_uchar>(512);
	AVL_GLOBAL::AVL_global::GPIOBuff0 = gcnew array<AVL_uchar>(64);
	AVL_GLOBAL::AVL_global::g_semaphores = gcnew array<System::Threading::Semaphore^>(16);
	AVL_GLOBAL::AVL_global::GPIO_sem0 = gcnew System::Threading::Semaphore(1, 1);
	AVL_GLOBAL::AVL_global::sem_ID = 0;
	try
	{
		AVL_GLOBAL::AVL_global::ClientSocket0->Connect(strServerName0, iSocketPort0);
	}
	catch( System::Net::Sockets::SocketException^ )
	{
		return(1);
	}

	AVL_GLOBAL::AVL_global::ClientSocket0->ReceiveTimeout = 15000;		// 15 seconds
	AVL_GLOBAL::AVL_global::ClientSocket0->SendTimeout = 15000;			// 15 seconds
#endif
	
	if(nullptr != strServerName1)
	{
		AVL_GLOBAL::AVL_global::ClientSocket1 = gcnew System::Net::Sockets::Socket(System::Net::Sockets::AddressFamily::InterNetwork, System::Net::Sockets::SocketType::Stream, System::Net::Sockets::ProtocolType::Tcp);
		AVL_GLOBAL::AVL_global::I2CBuff1 = gcnew array<AVL_uchar>(512);
		AVL_GLOBAL::AVL_global::GPIOBuff1 = gcnew array<AVL_uchar>(64);
		AVL_GLOBAL::AVL_global::GPIO_sem1 = gcnew System::Threading::Semaphore(1, 1);

		try
		{
			AVL_GLOBAL::AVL_global::ClientSocket1->Connect(strServerName1, iSocketPort1);
		}
		catch( System::Net::Sockets::SocketException^ )
		{
			return(1);
		}
		AVL_GLOBAL::AVL_global::ClientSocket1->ReceiveTimeout = 15000;		// 15 seconds
		AVL_GLOBAL::AVL_global::ClientSocket1->SendTimeout = 15000;			// 15 seconds
	}
	return(0);   
}

AVL_uint32 AVL_IBSP_InitSemaphore( AVL_psemaphore pSemaphore )
{
	if( AVL_GLOBAL::AVL_global::sem_ID < AVL_GLOBAL::AVL_global::g_semaphores->Length )
	{
		*pSemaphore = AVL_GLOBAL::AVL_global::sem_ID;
		AVL_GLOBAL::AVL_global::g_semaphores[AVL_GLOBAL::AVL_global::sem_ID] = gcnew System::Threading::Semaphore(1, 1);
		AVL_GLOBAL::AVL_global::sem_ID ++;
		return(0);
	}
	else
	{
		return(4);
	}
}

AVL_uint32 AVL_IBSP_ReleaseSemaphore( AVL_psemaphore pSemaphore )
{
	if( *pSemaphore < AVL_GLOBAL::AVL_global::g_semaphores->Length )
	{
		if( nullptr != AVL_GLOBAL::AVL_global::g_semaphores[*pSemaphore] )
		{
			AVL_GLOBAL::AVL_global::g_semaphores[*pSemaphore]->Release();
			return(0);
		}
	}

	return(0);
}

AVL_uint32 AVL_IBSP_WaitSemaphore( AVL_psemaphore pSemaphore )
{
	if( *pSemaphore < AVL_GLOBAL::AVL_global::g_semaphores->Length )
	{
		if( nullptr != AVL_GLOBAL::AVL_global::g_semaphores[*pSemaphore] )
		{
			AVL_GLOBAL::AVL_global::g_semaphores[*pSemaphore]->WaitOne();
			return(0);
		}
	}

	return(4);
}

AVL_uint32 AVL_IBSP_AAGPIO_SetDirection( AVL_uchar ucDirectionMask )
{
	int i ,j;
	AVL_uint32 r = 0;

	AVL_GLOBAL::AVL_global::ClientSocket = AVL_GLOBAL::AVL_global::ClientSocket0;
	AVL_GLOBAL::AVL_global::I2CBuff = AVL_GLOBAL::AVL_global::I2CBuff0;
	AVL_GLOBAL::AVL_global::GPIOBuff = AVL_GLOBAL::AVL_global::GPIOBuff0;
	AVL_GLOBAL::AVL_global::GPIO_sem = AVL_GLOBAL::AVL_global::GPIO_sem0;

	AVL_GLOBAL::AVL_global::GPIO_sem->WaitOne();
	AVL_GLOBAL::AVL_global::GPIOBuff[0] = 4;		//package size
	AVL_GLOBAL::AVL_global::GPIOBuff[1] = 2;		//cmd
	AVL_GLOBAL::AVL_global::GPIOBuff[2] = 2;		//size
	AVL_GLOBAL::AVL_global::GPIOBuff[3] = ucDirectionMask;
	try
	{
		AVL_GLOBAL::AVL_global::ClientSocket->Send(AVL_GLOBAL::AVL_global::GPIOBuff, AVL_GLOBAL::AVL_global::GPIOBuff[0], Sockets::SocketFlags::None);
		i = AVL_GLOBAL::AVL_global::ClientSocket->Receive(AVL_GLOBAL::AVL_global::GPIOBuff);
		j = AVL_GLOBAL::AVL_global::GPIOBuff[0];
		while( i<j )	  //the following loop code may not be used forever.
		{
			array<AVL_uchar> ^ tempBuff = gcnew array<AVL_uchar>(256);
			int k1, k2;
			k1 = AVL_GLOBAL::AVL_global::ClientSocket->Receive(tempBuff);
			for( k2=0; k2<k1; k2++ )
			{
				AVL_GLOBAL::AVL_global::GPIOBuff[k2+i] = tempBuff[k2]; 
			}
			i += k1;
		}
	}
	catch( System::Net::Sockets::SocketException^ )
	{
		r = 2;
	}

	if( 0 != AVL_GLOBAL::AVL_global::GPIOBuff[1] )
	{
		r = 4;
	}
	AVL_GLOBAL::AVL_global::GPIO_sem->Release();
	return (r);
}

AVL_uint32 AVL_IBSP_AAGPIO_SetValue( AVL_uchar ucValueMask )
{
		int i ,j;
	AVL_uint32 r = 0;

	AVL_GLOBAL::AVL_global::ClientSocket = AVL_GLOBAL::AVL_global::ClientSocket0;
	AVL_GLOBAL::AVL_global::I2CBuff = AVL_GLOBAL::AVL_global::I2CBuff0;
	AVL_GLOBAL::AVL_global::GPIOBuff = AVL_GLOBAL::AVL_global::GPIOBuff0;
	AVL_GLOBAL::AVL_global::GPIO_sem = AVL_GLOBAL::AVL_global::GPIO_sem0;

	AVL_GLOBAL::AVL_global::GPIO_sem->WaitOne();
	AVL_GLOBAL::AVL_global::GPIOBuff[0] = 4;		//package size
	AVL_GLOBAL::AVL_global::GPIOBuff[1] = 3;		//cmd
	AVL_GLOBAL::AVL_global::GPIOBuff[2] = 2;		//size
	AVL_GLOBAL::AVL_global::GPIOBuff[3] = ucValueMask;
	try
	{
		AVL_GLOBAL::AVL_global::ClientSocket->Send(AVL_GLOBAL::AVL_global::GPIOBuff, AVL_GLOBAL::AVL_global::GPIOBuff[0], Sockets::SocketFlags::None);
		i = AVL_GLOBAL::AVL_global::ClientSocket->Receive(AVL_GLOBAL::AVL_global::GPIOBuff);
		j = AVL_GLOBAL::AVL_global::GPIOBuff[0];
		while( i<j )	  //the following loop code may not be used forever.
		{
			array<AVL_uchar> ^ tempBuff = gcnew array<AVL_uchar>(256);
			int k1, k2;
			k1 = AVL_GLOBAL::AVL_global::ClientSocket->Receive(tempBuff);
			for( k2=0; k2<k1; k2++ )
			{
				AVL_GLOBAL::AVL_global::GPIOBuff[k2+i] = tempBuff[k2]; 
			}
			i += k1;
		}
	}
	catch( System::Net::Sockets::SocketException^ )
	{
		r = 2;
	}

	if( 0 != AVL_GLOBAL::AVL_global::GPIOBuff[1] )
	{
		r = 4;
	}
	AVL_GLOBAL::AVL_global::GPIO_sem->Release();
	return (r);
}

AVL_uint32 AVL_IBSP_AAGPIO_GetValue( AVL_puchar pucValueMask )
{
	int i ,j;
	AVL_uint32 r = 0;

	AVL_GLOBAL::AVL_global::ClientSocket = AVL_GLOBAL::AVL_global::ClientSocket0;
	AVL_GLOBAL::AVL_global::I2CBuff = AVL_GLOBAL::AVL_global::I2CBuff0;
	AVL_GLOBAL::AVL_global::GPIOBuff = AVL_GLOBAL::AVL_global::GPIOBuff0;
	AVL_GLOBAL::AVL_global::GPIO_sem = AVL_GLOBAL::AVL_global::GPIO_sem0;

	AVL_GLOBAL::AVL_global::GPIO_sem->WaitOne();
	AVL_GLOBAL::AVL_global::GPIOBuff[0] = 3;		//package size
	AVL_GLOBAL::AVL_global::GPIOBuff[1] = 4;		//cmd
	AVL_GLOBAL::AVL_global::GPIOBuff[2] = 1;		//size
	try
	{
		AVL_GLOBAL::AVL_global::ClientSocket->Send(AVL_GLOBAL::AVL_global::GPIOBuff, AVL_GLOBAL::AVL_global::GPIOBuff[0], Sockets::SocketFlags::None);
		i = AVL_GLOBAL::AVL_global::ClientSocket->Receive(AVL_GLOBAL::AVL_global::GPIOBuff);
		j = AVL_GLOBAL::AVL_global::GPIOBuff[0];
		while( i<j )	  //the following loop code may not be used forever.
		{
			array<AVL_uchar> ^ tempBuff = gcnew array<AVL_uchar>(256);
			int k1, k2;
			k1 = AVL_GLOBAL::AVL_global::ClientSocket->Receive(tempBuff);
			for( k2=0; k2<k1; k2++ )
			{
				AVL_GLOBAL::AVL_global::GPIOBuff[k2+i] = tempBuff[k2]; 
			}
			i += k1;
		}
	}
	catch( System::Net::Sockets::SocketException^ )
	{
		r = 2;
	}

	if( 0 != AVL_GLOBAL::AVL_global::GPIOBuff[1] )
	{
		r = 4;
	}
	else
	{
		*pucValueMask = AVL_GLOBAL::AVL_global::GPIOBuff[2];
	}
	AVL_GLOBAL::AVL_global::GPIO_sem->Release();
	return (r);
}

AVL_uint32 AVL_IBSP_Reset()
{
    AVL_uint32 r = 0;

    // Set up the Aardvark GPIO
    r = AVL_IBSP_AAGPIO_SetDirection(ucGPIODirMask);
    r |= AVL_IBSP_AAGPIO_SetValue(ucGPIOValueMask);

    ///perform hardware reset
    r |= AVL_IBSP_AAGPIO_SetValue(ucGPIOValueMask & (~ucGPIOResetPin));
    AVL_IBSP_Delay(2);
    r |= AVL_IBSP_AAGPIO_SetValue(ucGPIOValueMask);
    AVL_IBSP_Delay(10);

    return (r);
}

/* **************************************************** */
/*!
   @file	MN_DMD_console.c
   @brief	common function for CUI
   @author	R.Mori
   @date	2011/8/26
   @param
		(c)	Panasonic
   */
/* **************************************************** */

#include "MN_DMD_common.h"
#include "MN_DMD_console.h"
#include "MN_I2C.h"
#include "MN_TCB.h"
#include "MN_DMD_device.h"
#include "MN_DMD_driver.h"


#ifdef PC_CONTROL_FE
/* **************************************************** */
//	Get integer value from stdin
/* **************************************************** */
DMD_u32_t	DMD_con_get_int(void)
{
	DMD_s8_t	buf[10];
	DMD_u32_t	ret;
	fgets( buf , 10 , stdin );

	ret = atoi(buf);

	return ret;
}

/* **************************************************** */
//	TCB Test
/* **************************************************** */
DMD_u32_t DMD_con_tcb_test( DMD_PARAMETER* param)
{
	DMD_u8_t	wd[256];
	DMD_u8_t	rd[256];
	DMD_s8_t	buf[20];
	DMD_u32_t	slv,adr;
	DMD_u32_t	wlen,rlen;
	DMD_u32_t	i,x;
	DMD_ERROR_t	ret;

	printf("TCB Test:input slave   >");
	fgets( buf , 20 , stdin );
	sscanf( buf , "%x" ,&slv );
	printf("TCB Test:input address >");
	fgets( buf , 20 , stdin );
	sscanf( buf , "%x" ,&adr);
	
	i=0;
	do
	{
		printf("TCB Test:write data(q for Exit) >");
		fgets( buf , 20 , stdin );
		if( buf[0] == 'q' ) break;
		sscanf( buf , "%x" , &x );
		wd[i] = (DMD_u8_t) x;
		i++;
	}
	while( i<256 );
	printf("TCB Test:read num (0:no read) >");
	fgets( buf , 20 , stdin );
	rlen = atoi(buf);
	wlen = i;
	
	ret = DMD_TCB_WriteRead( (DMD_u8_t)slv , (DMD_u8_t)adr , wd , wlen , rd , rlen  );
	if( ret == DMD_E_OK )
	{
		printf("slave       :%02x\n" , slv );
		printf("address     :%02x\n" , adr );
		printf("write       :");
		for(i=0;i<wlen;i++)
			printf("%02x ",wd[i]);
		printf("\n");
		printf("read        :");
		for(i=0;i<rlen;i++)
			printf("%02x ",rd[i]);
		printf("\n");
		
		
	}

	return ret;
}

/* **************************************************** */
//	Get sytem from user input
/* **************************************************** */
DMD_u32_t	DMD_con_i2c_test( DMD_PARAMETER *param )
{

	DMD_s8_t	buf[20];
	DMD_s8_t	com[10];
	DMD_s32_t	slv,adr,data;
	DMD_s32_t	exitFlag=0;
	DMD_u8_t	rd,rdd[257];
	int i;
	while(!exitFlag)
	{
		printf("I2C Test : (r/w/d) (slave) (address) [data]>");
		fgets( buf , 20 , stdin );
		sscanf(buf , "%s %x %x %x" , com , &slv , &adr , &data);
		switch( com[0] )
		{
		case 'w':
			DMD_I2C_Write( (DMD_u8_t)slv ,(DMD_u8_t)adr ,(DMD_u8_t)data );
			printf("Write: %02x %02x %02x\n",slv,adr,data);
			break;
		case 'r':
			DMD_I2C_Read( (DMD_u8_t) slv, (DMD_u8_t) adr,&rd);
			printf("Read: %02x %02x %02x\n",slv,adr,rd);
			break;
		case 'd':
			printf("       _0 _1 _2 _3 _4 _5 _6 _7 _8 _9 _a _b _c _d _e _f");
			for( i=0;i<256;i++ )
			{
				if( (i%16) == 0 )
				{
					DMD_I2C_WriteRead( ( DMD_u8_t) slv , (DMD_u8_t) i , 0 , 0, rdd , 16 );
					printf("\n0x%x_ : " , i/16 );

				}
				printf("%02x ",rdd[i%16] );
			}
			printf("\n");
			break;
		case 'q':
			exitFlag = 1;
			break;
		}

	}

	return DMD_E_OK;
}

/* **************************************************** */
//	Get sytem from user input
/* **************************************************** */
DMD_u32_t	DMD_con_select_system( DMD_PARAMETER *param )
{
	DMD_u32_t	i;
	printf( "**** Select Broadcast System ****\n" );
	for(i=0;i<DMD_E_END_OF_SYSTEM;i++)
	{
		if( DMD_system_support(i) == DMD_E_OK )
			printf("%2d : %-20s\n" ,i, DMD_TEXTLIST_SYSTEM.list[i] );
	}
	printf("Select No>");
	param->system = DMD_con_get_int();

	switch( param->system )
	{
	case DMD_E_DVBT:
	case DMD_E_DVBT2:
		DMD_con_select_bw( param );
	default:
		break;
	}

	return DMD_E_OK;
}

/* **************************************************** */
//	Get bandwidth from user input
/* **************************************************** */
DMD_u32_t	DMD_con_select_bw( DMD_PARAMETER *param )
{
	DMD_u32_t	i;
	printf( "**** Select Bandwidth ****\n" );
	for(i=0;i<DMD_E_BW_END_OF_BW;i++)
	{
		printf("%2d : %-20s\n" ,i, DMD_TEXTLIST_BW.list[i] );
	}
	printf("Select No>");
	param->bw = DMD_con_get_int();


	return DMD_E_OK;
}
/* **************************************************** */
//	Get frequency from user input
/* **************************************************** */
DMD_u32_t	DMD_con_input_freq( DMD_PARAMETER *param )
{
	printf( "**** Input RF Frequency ****\n" );
	printf("Input [kHz] (eg.666000)>");
	param->freq = DMD_con_get_int();
	param->funit =DMD_E_KHZ;
	return DMD_E_OK;
}
/* **************************************************** */
//	Channel search
/* **************************************************** */
DMD_u32_t	DMD_con_channel_search( DMD_PARAMETER *param )
{
	DMD_u32_t	stf,edf,step;
	DMD_u32_t	f;
	DMD_ERROR_t	result;
	DMD_u32_t	stim,laptim,now;
	DMD_u32_t ok=0;
	DMD_u32_t ng =0;

	if( param->system == DMD_E_NOT_DEFINED )
	{
		DMD_con_select_system(param);
	}

	printf("Start Freq [kHz]>");
	stf = DMD_con_get_int();
	printf("End Freq [kHz]  >");
	edf = DMD_con_get_int();
	printf("Step [KHz]      >");
	step = DMD_con_get_int();

	param->funit = DMD_E_KHZ;
	DMD_timer(&stim);
	for(f=stf;f<=edf;f+=step)
	{
		DMD_timer(&laptim);
		printf("%4d [kHz]: ",f);
		param->freq = f;
		result = DMD_scan(param);
		DMD_timer(&now);
		printf("%4d[ms] ",now-laptim);
		if( result == DMD_E_OK )
		{
			printf("OK");
			ok++;
		}
		else{
			printf("NG : ");
			printf("%s",DMD_value_text(param,DMD_E_INFO_LOCK) );
			ng ++;
		}
		printf("\n");
	}

	DMD_timer(&now);
	printf("\n----\n" );
	printf("Result OK .. %d NG .. %d\n" , ok,ng );
	printf("Total : %d.%03d [s]\n" , (now-stim)/1000 , (now-stim)%1000);
	return DMD_E_OK;
}

/* **************************************************** */
//	Show status
/* **************************************************** */
DMD_u32_t	DMD_con_show_status( DMD_PARAMETER *param )
{
	DMD_u32_t	i;
	DMD_s8_t	buf[10];
	for(i=0;i<param->info[DMD_E_INFO_ALL];i++)
	{
		printf("%25s : " , DMD_info_title( param->system , i ) );
		printf("%15s  "  , DMD_value_text( param, i ) );
		printf(":%d  "   , param->info[i]  );
		printf("\n");
		if( (i%20) == 19 ){
		printf("--More--");
		fgets( buf , 10 , stdin );
		}
	}

	return DMD_E_OK;
}

/* **************************************************** */
//	Show monitor
/* **************************************************** */
DMD_u32_t	DMD_con_show_monitor( DMD_PARAMETER *param )
{
	printf("AGC : %4d ",param->info[DMD_E_INFO_AGC] );
	printf("Status: ");
	printf("%-20s  "  , DMD_info_value( param->system , DMD_E_INFO_STATUS ,param->info[DMD_E_INFO_STATUS] ) );
#ifdef DMD_FLOATING_FUNCTION
	printf("BER=%e  "    , DMD_get_ber(param));
	printf("CNR=%2.2f  " , DMD_get_cnr(param));
#else
	printf("BER=%8d " ,param->info[DMD_E_INFO_BERRNUM]);
	printf("/ %8d "   ,param->info[DMD_E_INFO_BITNUM]);
	printf("CNR=%2d " ,param->info[DMD_E_INFO_CNR_INT]);
	printf(".%02d "   ,param->info[DMD_E_INFO_CNR_DEC]);
	printf("%15s"   ,DMD_value_text(param,DMD_E_INFO_ERRORFREE));
#endif
	printf("\r");

	return DMD_E_OK;

}
#endif //PC_CONTROL_FE


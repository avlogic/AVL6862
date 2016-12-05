#include <windows.h>
#ifdef FX2I2C_EXPORTS
#define FX2I2C_API __declspec(dllexport)
#else
#define FX2I2C_API __declspec(dllimport)
#endif

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short int u16;
typedef signed short int s16;
typedef unsigned int u32;
typedef signed int s32;

#define BXIN_OK		0x03
#define ON_400K		0x01
#define OFF_400K	0x00
#define ACK			1
#define NOACK		0
#define ERRSEND		-1	//please i2c_ini();
#define bmpSTART	0x80
#define bmpSTOP		0x40
#define bmpLASTRD	0x20
#define bmpBERR		0x04
#define bmpACK		0x02
#define bmpDONE		0x01
#define WAIT_SEND 0x80		
#define LAST_WAIT 0x53
#define LAST_400 0x13


s32 FX2I2C_API	usbi2c_open(void);
s32 FX2I2C_API  usbi2c_close(void);
s32 FX2I2C_API  bixin(void);	
void FX2I2C_API  general_write(u8 command , u8 data);
s32 FX2I2C_API  general_read(u8 command);		
void FX2I2C_API  i2c_ini(void);				
u8  FX2I2C_API AckCheck(void);							
s32 FX2I2C_API usbi2c_beforecheck(void);				
void FX2I2C_API usbi2c_send(u8 slavead , u8 regad , u8 regsend , s32 *wdata , u8 wlen , s32 *rdata , u8 rlen , u8 waitsend , u8 f400khz_on , u8 lastwait);
void FX2I2C_API usbi2c_wr(s32 *slavead , s32 *regad , s32 *wdata , s32 length , s32 *rdata , u8 rlen  , u8 waitsend , u8 f400khz_on , u8 lastwait );
void FX2I2C_API usbi2c_restart_write( s32 *wdata , s32 *restart , s32 length , u8 waitsend , u8 f400khz_on, u8 last_wait );
void FX2I2C_API usbi2c_read(s32 *slavead , s32 *regad , s32 *rdata , s32 length ,  u8 waitsend , u8 f400khz_on );

s32 wregsend  ( u8 slavead ,u8 regad ,u8 waitsend , u8 *cmd, s32 i );
s32 write_send( u8 wlen , s32 *wdata , u8 waitsend , u8 *cmd , s32 i );
s32 read_send ( u8 slavead , u8 rlen , s32 *rdata , u8 waitsend , u8 *cmd , s32 i , u8 f400khz_on , u8 lastwait);
s32 r_no_send ( u8 *cmd , s32 i );
s32 f400onoff ( u8 f400khz_on , u8 *cmd , s32 i );
void  cmd_ini(void);
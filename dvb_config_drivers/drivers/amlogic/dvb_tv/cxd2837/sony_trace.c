#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>
#include "sony_trace.h"

void sony_trace_log_enter(const char* funcname, const char* filename, unsigned int linenum)
{
    printk("%s,file%s,line %d\n",funcname,filename,linenum);
}
void sony_trace_log_return(int result, const char* filename, unsigned int linenum)
{
	printk("result %d,file%s,line %d\n",result,filename,linenum);
}

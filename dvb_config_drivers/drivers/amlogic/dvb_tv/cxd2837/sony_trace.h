#ifndef _SONY_TRACE_H_
#define _SONY_TRACE_H_



#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/delay.h>

void sony_trace_log_enter(const char* funcname, const char* filename, unsigned int linenum);
void sony_trace_log_return(int result, const char* filename, unsigned int linenum);

#endif

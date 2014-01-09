#ifndef _KERNELLIB_H
#define _KERNELLIB_H

#ifdef DEBUG_TRACE
#define Ktrace(fmt, ...)	DbgPrint(fmt, ##__VA_ARGS__)
#else
#define	Ktrace(fmt, ...)
#endif

#endif

#ifndef _KERNELLIB_H
#define _KERNELLIB_H

#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include <intrin.h>

#define DEBUG_TRACE

#ifdef DEBUG_TRACE
#define Ktrace(fmt, ...)	DbgPrint(fmt, ##__VA_ARGS__)
#else
#define	Ktrace(fmt, ...)
#endif

#endif

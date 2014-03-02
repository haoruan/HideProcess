#ifndef _KERNELLIB_H
#define _KERNELLIB_H

#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include <intrin.h>
#include <ntddkbd.h>

#define DEBUG_TRACE

#ifdef DEBUG_TRACE
#define Ktrace(fmt, ...)	DbgPrint(fmt, ##__VA_ARGS__)
#else
#define	Ktrace(fmt, ...)
#endif

KIRQL WPOFFx64();
void WPONx64(KIRQL irql);

#endif

#ifndef _DKOMEPROCESS_H
#define _DKOMEPROCESS_H

#include "kernellib.h"

#define SYSPROCESS (PUCHAR)PsInitialSystemProcess
NTSTATUS DE_Onload(IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath);
VOID DE_OnUnload(IN PDRIVER_OBJECT DriverObject);

#endif
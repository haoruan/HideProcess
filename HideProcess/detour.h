#ifndef _DETOUR_H
#define _DETOUR_H

#include "kernellib.h"

#define FUNCTION_NtDeviceIoControlFile_OFFSET 16
#define FUNCTION_SeAccessCheck_OFFSET 17
#define FUNCTION_OFFSET FUNCTION_SeAccessCheck_OFFSET
#define FUNCTION SeAccessCheck
#define FUNCTION_NAME "SeAccessCheck"
#define CHECK_FUNCTION Check_Function_SeAccessCheck
#define DETOUR_FUNCTION MySeAccessCheck
#define POOL_SIZE 128

ULONG64 d_reentry_address;
ULONG64 call_count;
BOOL check_success;
BOOL alloc_pool;
PUCHAR non_paged_memory;

VOID NTAPI MyNtDeviceIoControlFile();
VOID NTAPI MySeAccessCheck();

NTSTATUS DT_Onload(IN PDRIVER_OBJECT theDriverObject,
	IN PUNICODE_STRING theRegistryPath);
VOID DT_OnUnload(IN PDRIVER_OBJECT DriverObject);



#endif
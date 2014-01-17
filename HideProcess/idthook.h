#ifndef _IDTHOOK_H
#define _IDTHOOK_H

#include "kernellib.h"

//#define MAKELONG(a, b) ((LONG) (((WORD) (a)) | ((DWORD) ((WORD) (b))) << 16)) 

ULONG gProcessNameOffset;
ULONG gProcessID;
ULONG gSyscallLimit;

/**********************************************************************************
* Interrupt Descriptor Table
**********************************************************************************/
#pragma pack(1)
typedef struct
{
	WORD LowlOffset;
	WORD selector;
	BYTE IST : 3;
	BYTE zero_field : 5;
	BYTE TYPE_FIELD : 4; /* stored TYPE ? */
	BYTE zero_field2 : 1;
	BYTE DPL : 2;
	BYTE P : 1; /* present */
	WORD LowhOffset;
	DWORD HiOffset;
	DWORD unused_lo;
} IDTENTRY;

/* sidt returns idt in this format */
typedef struct
{
	WORD IDTLimit;
	DWORD LowIDTbase;
	DWORD HiIDTbase;
} IDTINFO;

/* from undoc nt */
typedef struct
{
	unsigned short  limit_0_15;
	unsigned short  base_0_15;
	unsigned char   base_16_23;

	unsigned char    accessed : 1;
	unsigned char    readable : 1;
	unsigned char    conforming : 1;
	unsigned char    code_data : 1;
	unsigned char    app_system : 1;
	unsigned char    dpl : 2;
	unsigned char    present : 1;

	unsigned char    limit_16_19 : 4;
	unsigned char    unused : 1;
	unsigned char    always_0 : 1;
	unsigned char    seg_16_32 : 1;
	unsigned char    granularity : 1;

	unsigned char   base_24_31;
} CODE_SEG_DESCRIPTOR;

/* from undoc nt */
typedef struct
{
	unsigned short  offset_0_15;
	unsigned short  selector;

	unsigned char    param_count : 4;
	unsigned char    some_bits : 4;

	unsigned char    type : 4;
	unsigned char    app_system : 1;
	unsigned char    dpl : 2;
	unsigned char    present : 1;

	unsigned short  offset_16_31;
} CALLGATE_DESCRIPTOR;

#pragma pack()


#define MAX_IDT_ENTRIES 0xFF

#define NT_SYSTEM_SERVICE_INT 0x2e

VOID NTAPI JmpOrigEntry(ULONGLONG origFastCallEntry);

NTSTATUS IH_Onload(IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath);
VOID IH_OnUnload(IN PDRIVER_OBJECT DriverObject);

ULONG getPID();
int	HookInterrupts();
int	UnhookInterrupts();
void PrintDbg(ULONG);


#endif
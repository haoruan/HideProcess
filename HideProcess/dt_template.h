#ifndef _DT_TEMPLATE_H
#define _DT_TEMPLATE_H

#include "kernellib.h"

#define MAX_IDT_ENTRIES 0x100
#define START_IDT_OFFSET 0x00
#define JUMP_TEMPLATE_SIZE 128

ULONG64 call_count[MAX_IDT_ENTRIES];
ULONG64 old_ISR_pointers[MAX_IDT_ENTRIES];
PUCHAR idt_detour_tablebase;
BOOL alloc_pool;

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
#pragma pack()

VOID NTAPI JumpTemplate();

NTSTATUS DTT_Onload(IN PDRIVER_OBJECT theDriverObject,
	IN PUNICODE_STRING theRegistryPath);
VOID DTT_OnUnload(IN PDRIVER_OBJECT DriverObject);

#endif
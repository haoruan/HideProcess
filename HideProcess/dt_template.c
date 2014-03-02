#include "dt_template.h"

VOID count_interrupts(ULONG index)
{
	call_count[index]++;
}

NTSTATUS DTT_Onload(IN PDRIVER_OBJECT theDriverObject,
	IN PUNICODE_STRING theRegistryPath)
{
	IDTINFO idt_info;
	IDTENTRY* idt_entries;
	__sidt(&idt_info);

	idt_entries = (IDTENTRY*)*(PULONG64)&idt_info.LowIDTbase;

	for (int i = START_IDT_OFFSET; i < MAX_IDT_ENTRIES; i++) {
		IDTENTRY idt_i = idt_entries[i];
		ULONG64 addr = (ULONG64)((ULONG64)idt_i.HiOffset) << 32 | ((DWORD)idt_i.LowhOffset << 16) | (idt_i.LowlOffset);
		old_ISR_pointers[i] = addr;
		Ktrace("(MYRKT)Interrupt %x: ISR %llx.\n", i, addr);
	}

	idt_detour_tablebase = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, JUMP_TEMPLATE_SIZE * MAX_IDT_ENTRIES, "tag1");
	if (!idt_detour_tablebase) {
		Ktrace("(MYRKT)ExAllocatePoolWithTag :There is insufficient memory in the free pool.\n");
		return STATUS_UNSUCCESSFUL;
	}
	alloc_pool = TRUE;

	for (int i = START_IDT_OFFSET; i < MAX_IDT_ENTRIES; i++) {
		call_count[i] = 0;
		int offset = JUMP_TEMPLATE_SIZE * i;
		PUCHAR entry_ptr = idt_detour_tablebase + offset;
		memcpy(entry_ptr, JumpTemplate, JUMP_TEMPLATE_SIZE);
		*(PULONG)(entry_ptr + 15) = i;
		*(PULONG64)(entry_ptr + 21) = (ULONG64)count_interrupts;

		ULONG low_reentry_address = (ULONG)old_ISR_pointers[i];
		ULONG high_reentry_address = (ULONG)(old_ISR_pointers[i] >> 32);
		*(PULONG)(entry_ptr + 44) = low_reentry_address;
		*(PULONG)(entry_ptr + 52) = high_reentry_address;

		_disable();
		idt_entries[i].HiOffset = (ULONG)((ULONG64)entry_ptr >> 32);
		idt_entries[i].LowhOffset = (WORD)(((DWORD)entry_ptr) >> 16);
		idt_entries[i].LowlOffset = (WORD)entry_ptr;
		_enable();
	}

	return STATUS_SUCCESS;
}
VOID DTT_OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	IDTINFO idt_info;
	IDTENTRY* idt_entries;
	__sidt(&idt_info);

	idt_entries = (IDTENTRY*)*(PULONG64)&idt_info.LowIDTbase;

	for (int i = START_IDT_OFFSET; i < MAX_IDT_ENTRIES; i++) {
		_disable();
		idt_entries[i].HiOffset = (ULONG)(old_ISR_pointers[i] >> 32);
		idt_entries[i].LowhOffset = (WORD)(((DWORD)old_ISR_pointers[i]) >> 16);
		idt_entries[i].LowlOffset = (WORD)old_ISR_pointers[i];
		_enable();
		ULONG64 addr = (ULONG64)((ULONG64)idt_entries[i].HiOffset) << 32 | ((DWORD)idt_entries[i].LowhOffset << 16) | (idt_entries[i].LowlOffset);
		Ktrace("(MYRKT)Interrupt %x: ISR %llx called %d times.\n", i, addr, call_count[i]);
	}

	if (alloc_pool){
		ExFreePoolWithTag((PVOID)idt_detour_tablebase, "tag1");
	}
}
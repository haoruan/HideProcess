#include "detour.h"

UCHAR orig_code[] = "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90";

VOID TestFun()
{
	call_count++;
	Ktrace("(MYRKT)Fuck You %s !\n", FUNCTION_NAME);
}

NTSTATUS Check_Function_NtDeviceIoControlFile()
{
	int i = 0;
	PUCHAR check_function = (PUCHAR)NtDeviceIoControlFile;
	UCHAR code[] = { 0x48, 0x83, 0xec, 0x68, 0x8b, 0x84, 0x24, 0xb8, 0x00, 0x00, 0x00, 0xc6, 0x44, 0x24, 0x50, 0x01 };
	while (i < FUNCTION_NtDeviceIoControlFile_OFFSET) {
		if (check_function[i] != code[i]) {
			return STATUS_UNSUCCESSFUL;
		}
		i++;
	}

	return STATUS_SUCCESS;
}

NTSTATUS Check_Function_SeAccessCheck()
{
	int i = 0;
	PUCHAR check_function = (PUCHAR)SeAccessCheck;
	UCHAR code[] = { 0x48, 0x83, 0xec, 0x68, 0x48, 0x8b, 0x84, 0x24, 0xb8, 0x00, 0x00, 0x00, 0x48, 0x89, 0x44, 0x24, 0x50 };
	while (i < FUNCTION_SeAccessCheck_OFFSET) {
		if (check_function[i] != code[i]) {
			return STATUS_UNSUCCESSFUL;
		}
		i++;
	}

	return STATUS_SUCCESS;
}

NTSTATUS DT_Onload(IN PDRIVER_OBJECT theDriverObject,
	IN PUNICODE_STRING theRegistryPath)
{
	call_count = 0;
	check_success = FALSE;
	alloc_pool = FALSE;
	if (CHECK_FUNCTION() != STATUS_SUCCESS) {
		Ktrace("(MYRKT)Match Failure on  %s!\n", FUNCTION_NAME);
		return STATUS_UNSUCCESSFUL;
	}
	//PUCHAR assem = (PUCHAR)MyNtDeviceIoControlFile;
	/*PUCHAR testfun = (PUCHAR)TestFun;
	StarAssem();
	return STATUS_SUCCESS;*/
	d_reentry_address = (ULONG64)FUNCTION + FUNCTION_OFFSET;
	ULONG low_reentry_address = (ULONG)d_reentry_address;
	ULONG high_reentry_address = (ULONG)(d_reentry_address >> 32);

	non_paged_memory = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, POOL_SIZE, "tag1");
	if (!non_paged_memory) {
		Ktrace("(MYRKT)ExAllocatePoolWithTag :There is insufficient memory in the free pool.\n");
		return STATUS_UNSUCCESSFUL;
	}
	alloc_pool = TRUE;
	for (int i = 0; i < POOL_SIZE; i++) {
		non_paged_memory[i] = ((PUCHAR)DETOUR_FUNCTION)[i];
	}

	int replace = 0;
	for (int i = 0; i < POOL_SIZE - 4; i++) {
		if (*(PULONG64)(non_paged_memory + i) == 0x4321432143214321) {
			*(PULONG64)(non_paged_memory + i) = (ULONG64)TestFun;
			Ktrace("(MYRKT)Detour :Call address has benn replaced.\n");
			replace++;
		}
		if (*(PULONG)(non_paged_memory + i) == 0x12341234) {
			*(PULONG)(non_paged_memory + i) = low_reentry_address;
			*(PULONG)(non_paged_memory + i + 8) = high_reentry_address;
			Ktrace("(MYRKT)Detour :Jmp address has benn replaced.\n");
			replace++;
			break;
		}
	}

	//return STATUS_SUCCESS;

	if (replace < 2) {
		return STATUS_UNSUCCESSFUL;
	}
	
	KIRQL irql = WPOFFx64();
	ULONG64 myfun;
	UCHAR jmp_code[] = "\x48\xB8\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\xFF\xE0\x90\x90\x90\x90\x90";
	myfun = (ULONG64)non_paged_memory;
	memcpy(jmp_code + 2, &myfun, 8);
	memcpy(orig_code, FUNCTION, FUNCTION_OFFSET);
	memcpy(FUNCTION, jmp_code, FUNCTION_OFFSET);
	WPONx64(irql);

	check_success = TRUE;

	return STATUS_SUCCESS;
}

VOID DT_OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	Ktrace("(MYRKT)%s Call Count : %d\n", FUNCTION_NAME, call_count);
	if (check_success){
		KIRQL irql = WPOFFx64();
		memcpy(FUNCTION, orig_code, FUNCTION_OFFSET);
		WPONx64(irql);
	}

	if (alloc_pool){
		ExFreePoolWithTag((PVOID)non_paged_memory, "tag1");
	}
}
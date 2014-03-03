#include "basic_hardware.h"

UCHAR g_key_bits = 0;
PUCHAR KEYBOARD_PORT_60 = (PUCHAR)0x60;
PUCHAR KEYBOARD_PORT_64 = (PUCHAR)0x64;

ULONG WaitForKeyboard()
{
	char _t[255];
	int i = 100;	// number of times to loop
	UCHAR mychar;

	//DbgPrint("waiting for keyboard to become accecssable\n");
	do
	{
		mychar = READ_PORT_UCHAR(KEYBOARD_PORT_64);

		KeStallExecutionProcessor(50);

		//_snprintf(_t, 253, "WaitForKeyboard::read byte %02X from port 0x64\n", mychar);
		//DbgPrint(_t);

		if (!(mychar & IBUFFER_FULL)) break;	// if the flag is clear, we go ahead
	} while (i--);

	if (i) return TRUE;
	return FALSE;
}

// call WaitForKeyboard before calling this function
VOID DrainOutputBuffer()
{
	char _t[255];
	int i = 100;	// number of times to loop
	UCHAR c;

	//DbgPrint("draining keyboard buffer\n");
	do
	{
		c = READ_PORT_UCHAR(KEYBOARD_PORT_64);

		KeStallExecutionProcessor(50);

		//_snprintf(_t, 253, "DrainOutputBuffer::read byte %02X from port 0x64\n", c);
		//DbgPrint(_t);

		if (!(c & OBUFFER_FULL)) break;	// if the flag is clear, we go ahead

		// gobble up the byte in the output buffer
		c = READ_PORT_UCHAR(KEYBOARD_PORT_60);

		//_snprintf(_t, 253, "DrainOutputBuffer::read byte %02X from port 0x60\n", c);
		//DbgPrint(_t);
	} while (i--);
}

// write a byte to the data port at 0x60
ULONG SendKeyboardCommand(IN UCHAR theCommand)
{
	char _t[255];

	if (TRUE == WaitForKeyboard())
	{
		DrainOutputBuffer();

		//_snprintf(_t, 253, "SendKeyboardCommand::sending byte %02X to port 0x60\n", theCommand);
		//DbgPrint(_t);

		WRITE_PORT_UCHAR(KEYBOARD_PORT_60, theCommand);

		//DbgPrint("SendKeyboardCommand::sent\n");
	}
	else
	{
		//DbgPrint("SendKeyboardCommand::timeout waiting for keyboard\n");
		return FALSE;
	}

	// TODO: wait for ACK or RESEND from keyboard	

	return TRUE;
}

void SetLEDS(UCHAR theLEDS)
{
	// setup for setting LEDS
	if (FALSE == SendKeyboardCommand(0xED))
	{
		//DbgPrint("SetLEDS::error sending keyboard command\n");
	}

	// send the flags for the LEDS
	if (FALSE == SendKeyboardCommand(theLEDS))
	{
		//DbgPrint("SetLEDS::error sending keyboard command\n");
	}
}

// called periodically
VOID timerDPC(IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID sys1,
	IN PVOID sys2)
{
	//WRITE_PORT_UCHAR(KEYBOARD_PORT_64, 0xFE);
	SetLEDS(g_key_bits++);
	if (g_key_bits > 0x07) g_key_bits = 0;
}

NTSTATUS BH_Onload(IN PDRIVER_OBJECT theDriverObject, IN PUNICODE_STRING theRegistryPath)
{
	LARGE_INTEGER timeout;

	// these objects must be non paged
	gTimer = ExAllocatePoolWithTag(NonPagedPool, sizeof(KTIMER), "time");
	gDPCP = ExAllocatePoolWithTag(NonPagedPool, sizeof(KDPC), "dpc");

	timeout.QuadPart = -10;

	KeInitializeTimer(gTimer);
	KeInitializeDpc(gDPCP, timerDPC, NULL);

	if (TRUE == KeSetTimerEx(gTimer, timeout, 300, gDPCP))	// 300 ms timer	
	{
		DbgPrint("Timer was already queued..");
	}

	return STATUS_SUCCESS;
}

VOID BH_OnUnload(IN PDRIVER_OBJECT DriverObject)
{
	KeCancelTimer(gTimer);
	ExFreePoolWithTag(gTimer, "time");
	ExFreePoolWithTag(gDPCP, "dpc");
}
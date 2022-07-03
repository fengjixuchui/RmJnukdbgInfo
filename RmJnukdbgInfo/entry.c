#include <ntifs.h>

extern POBJECT_TYPE* IoDriverObjectType;

NTKERNELAPI
NTSTATUS
ObReferenceObjectByName(
	IN PUNICODE_STRING ObjectName,
	IN ULONG Attributes,
	IN PACCESS_STATE PassedAccessState OPTIONAL,
	IN ACCESS_MASK DesiredAccess OPTIONAL,
	IN POBJECT_TYPE ObjectType,
	IN KPROCESSOR_MODE AccessMode,
	IN OUT PVOID ParseContext OPTIONAL,
	OUT PVOID* Object
);


KIRQL WriteProtectOff()
{
	KIRQL OldIrql = 0;
	ULONG_PTR cr0 = 0;

	OldIrql = KeRaiseIrqlToDpcLevel();
	cr0 = __readcr0();
#ifdef _X86_
	cr0 &= 0xfffeffff;
#else 
	cr0 &= 0xfffffffffffeffff;
#endif 
	_disable();
	__writecr0(cr0);

	return(OldIrql);
}


VOID WriteProtectOn(KIRQL irql)
{
	ULONG_PTR cr0 = 0;

	cr0 = __readcr0();
#ifdef _X86_
	cr0 |= 0x00010000;
#else 
	cr0 |= 0x0000000000010000;
#endif 
	__writecr0(cr0);
	_enable();
	KeLowerIrql(irql);
}


NTSTATUS RemoveStorNVMeJunkInfo()
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING DriverName = { 0 };
	PDRIVER_OBJECT DriverObject = NULL;

	RtlInitUnicodeString(&DriverName, L"\\Driver\\stornvme");

	status = ObReferenceObjectByName(&DriverName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		&DriverObject);


	if (!NT_SUCCESS(status))
	{
		KdPrint(("%s status:%x\n", __FUNCTION__, status));
		return status;
	}

	//KdPrint(("DriverObject = %p\n", DriverObject));

	if (MmIsAddressValid(DriverObject) && MmIsAddressValid(DriverObject->DriverStart) && MmIsAddressValid((PUCHAR)DriverObject->DriverStart + 0x1C6AE))
	{
/*
fffff800`83b45bd2 488d15578d0100  lea     rdx,[fffff800`83b5e930]
fffff800`83b45bd9 b903000000      mov     ecx,3
fffff800`83b45bde 4c8b154b140200  mov     r10,qword ptr [fffff800`83b67030]
fffff800`83b45be5 e83631f7ff      call    cs:__imp_StorPortDebugPrint			;垃圾信息输出
fffff800`83b45bea 0fb64314        movzx   eax,byte ptr [rbx+14h]
fffff800`83b45bee c1e002          shl     eax,2
fffff800`83b45bf1 338778060000    xor     eax,dword ptr [rdi+678h]
fffff800`83b45bf7 83e004          and     eax,4
*/
		//ImageBase+5BD2
		for (PUCHAR start = DriverObject->DriverStart; start < (PUCHAR)DriverObject->DriverStart + 0x1C6AE; ++start)
		{
			//B9 03 ?? ?? ?? 4C 8B 15 ?? ?? ?? ?? E8 XX XX XX XX
			if (*(PUINT16)start == 0x03B9 && *(PUINT16)(start + 0x5) == 0x8B4C && start[0xC] == 0xE8)
			{
				KdPrint(("Hit!!! Address = %p\n", start));
				
				start += 0xC;
				__try 
				{
					KIRQL irql = WriteProtectOff();
					for (size_t i = 0; i < 5; i++)
					{
						start[i] = 0x90;
					}
					WriteProtectOn(irql);
				}
				__except(1)
				{
					KdPrint(("Write error\n"));
				}
			}
		}
	}

	ObDereferenceObject(DriverObject);

	return status;
}


//此处采用直接调用API写KdComponentTable的方式，也可直接硬编码获取表项硬写
NTSTATUS RemoveDbgViewJunkInfo()
{
	NTSTATUS status = STATUS_SUCCESS;

	//Kd_SXS_Mask = KdComponentTable[0x33]
	DbgSetDebugFilterState(0x33, 0x3fffffff, FALSE);
	//Kd_FUSION_Mask = KdComponentTable[0x34]
	DbgSetDebugFilterState(0x34, 0x3fffffff, FALSE);

	return status;
}


NTSTATUS DriverUnload(PDRIVER_OBJECT pDrvObj)
{
	UNREFERENCED_PARAMETER(pDrvObj);

	NTSTATUS status = STATUS_SUCCESS;
	return status;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath)
{
	UNREFERENCED_PARAMETER(pRegPath);

	NTSTATUS status = STATUS_SUCCESS;
	pDrvObj->DriverUnload = DriverUnload;

	//清除Nvme垃圾信息
	RemoveStorNVMeJunkInfo();
	//清除DbgView垃圾信息
	RemoveDbgViewJunkInfo();

	return status;
}
#include <ntifs.h>
#include "utils.hpp"


NTSTATUS RemoveStorNVMeJunkInfo()
{
	PVOID StorportBase = GetModuleBaseEx(L"storport.sys");
	if (!StorportBase) {
		DbgPrint("[-] Fail to find storport.sys\n");
		return STATUS_NOT_FOUND;
	}
	DbgPrint("[+] Found storport.sys at %p\n", StorportBase);
	PVOID StorPortDebugPrint = RtlFindExportedRoutineByName(StorportBase, "StorPortDebugPrint");
	if (!StorPortDebugPrint) {
		DbgPrint("[-] Fail to find export StorPortDebugPrint\n");
		return STATUS_NOT_FOUND;
	}
	DbgPrint("[+] Found export StorPortDebugPrint at %p\n", StorPortDebugPrint);
	if (!WriteToReadOnly(StorPortDebugPrint, "\xC3", 1)) {
		DbgPrint("[-] Fail to patch StorPortDebugPrint\n");
		return STATUS_UNSUCCESSFUL;
	}
	DbgPrint("[+] Patch StorPortDebugPrint success\n");
	return STATUS_SUCCESS;
}

//此处采用直接调用API写KdComponentTable的方式，也可直接硬编码获取表项硬写
NTSTATUS RemoveDbgViewJunkInfo()
{
	NTSTATUS status = STATUS_SUCCESS;

	//Kd_SXS_Mask = KdComponentTable[0x33]
	status = DbgSetDebugFilterState(0x33, 0x3fffffff, FALSE);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[-] Fail to set Kd_SXS_Mask\n");
		return status;
	}
	DbgPrint("[+] Set Kd_SXS_Mask success\n");
	//Kd_FUSION_Mask = KdComponentTable[0x34]
	status = DbgSetDebugFilterState(0x34, 0x3fffffff, FALSE);
	if (!NT_SUCCESS(status)) {
		DbgPrint("[-] Fail to set Kd_FUSION_Mask\n");
		return status;
	}
	DbgPrint("[+] Set Kd_FUSION_Mask success\n");
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
	DbgPrint("[+] Successfully removed StorNVMe Junk Info\n");

	//清除DbgView垃圾信息
	RemoveDbgViewJunkInfo();
	DbgPrint("[+] Successfully removed DbgView Junk Info\n");

	return status;
}
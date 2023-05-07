#include <ntifs.h>
#include <windef.h>

EXTERN_C
NTKERNELAPI
PVOID
NTAPI
RtlFindExportedRoutineByName(
    PVOID ImageBase,
    PCCH RoutineNam);

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

PVOID GetModuleBaseEx(PWCHAR szModuleName)
{
    UNICODE_STRING uName = RTL_CONSTANT_STRING(L"PsLoadedModuleList");
    PLIST_ENTRY PsLoadedModuleList, NextEntry;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    UNICODE_STRING uModuleName = { 0 };

    PsLoadedModuleList = (PLIST_ENTRY)MmGetSystemRoutineAddress(&uName);
    if (!MmIsAddressValid(PsLoadedModuleList)) {
        return NULL;
    }
    RtlInitUnicodeString(&uModuleName, szModuleName);

    for (NextEntry = PsLoadedModuleList->Flink;
        NextEntry != PsLoadedModuleList;
        NextEntry = NextEntry->Flink)
    {
        LdrEntry = (PLDR_DATA_TABLE_ENTRY)NextEntry;
        if (RtlEqualUnicodeString(&uModuleName, &LdrEntry->BaseDllName, TRUE))
        {
            return LdrEntry->DllBase;
        }
    }
    return NULL;
}

BOOLEAN WriteToReadOnly(PVOID destination, PVOID buffer, ULONG size)
{
    PMDL mdl = IoAllocateMdl(destination, size, FALSE, FALSE, 0);
    if (!mdl) {
        return FALSE;
    }
    MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
    MmProtectMdlSystemAddress(mdl, PAGE_EXECUTE_READWRITE);

    PVOID mmMap = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
    memcpy(mmMap, buffer, size);

    MmUnmapLockedPages(mmMap, mdl);
    MmUnlockPages(mdl);
    IoFreeMdl(mdl);

    return TRUE;
}



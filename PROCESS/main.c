#include <ntifs.h>
#include <ntstrsafe.h>
#include <string.h>


/*
	Windows 10 Kernel Version 15063 MP (2 procs) Free x64
	Edition build lab: 15063.0.amd64fre.rs2_release.170317-1834
	Kernel base = 0xfffff800`b1c1a000 PsLoadedModuleList = 0xfffff800`b1f69700
	Debug session time: Tue Oct 13 15:49:14.864 2026 (UTC + 8:00)
	System Uptime: 0 days 0:00:42.045
*/


// 仅对应本次 WinDbg 显示的目标系统布局
#define OFFSET_EPROCESS_PID                  0x2E0
#define OFFSET_EPROCESS_ACTIVE_PROCESS_LINKS 0x2E8
#define OFFSET_EPROCESS_IMAGE_FILE_NAME      0x450
#define OFFSET_EPROCESS_IMAGE_FILE_POINTER   0x450-8
#define OFFSET_FILE_FileName				 0x58
#define OFFSET_EPROCESS_ACTIVE_THREADS		 0x498

/*
 * 当前调试会话中观察到的函数入口地址。
 * 临时硬编码：重启或更换系统版本后需要重新核对。
 * TODO：后续改用特征码搜索工具定位，并验证搜索结果。
 */
#define PSP_TERMINATE_THREAD_BY_POINTER_ADDR ((ULONG_PTR)0xFFFFF800B203E1D0ULL)


// ImageFileName 是内嵌数组，长度不是偏移
#define EPROCESS_IMAGE_FILE_NAME_LENGTH      15


typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBasicInformation = 0,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,               // 5
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation,
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,                // 16
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemMirrorMemoryInformation,
	SystemPerformanceTraceInformation,
	SystemObsolete0,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeparation,
	SystemVerifierAddDriverInformation,
	SystemVerifierRemoveDriverInformation,
	SystemProcessorIdleInformation,
	SystemLegacyDriverInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation,
	SystemTimeSlipNotification,
	SystemSessionCreate,
	SystemSessionDetach,
	SystemSessionInformation,
	SystemRangeStartInformation,
	SystemVerifierInformation,
	SystemVerifierThunkExtend,
	SystemSessionProcessInformation,
	SystemLoadGdiDriverInSystemSpace,
	SystemNumaProcessorMap,
	SystemPrefetcherInformation,
	SystemExtendedProcessInformation,
	SystemRecommendedSharedDataAlignment,
	SystemComPlusPackage,
	SystemNumaAvailableMemory,
	SystemProcessorPowerInformation,
	SystemEmulationBasicInformation,
	SystemEmulationProcessorInformation,
	SystemExtendedHandleInformation,        // 64
	SystemLostDelayedWriteInformation,
	SystemBigPoolInformation,
	SystemSessionPoolTagInformation,
	SystemSessionMappedViewInformation,
	SystemHotpatchInformation,
	SystemObjectSecurityMode,
	SystemWatchdogTimerHandler,
	SystemWatchdogTimerInformation,
	SystemLogicalProcessorInformation,
	SystemWow64SharedInformation,
	SystemRegisterFirmwareTableInformationHandler,
	SystemFirmwareTableInformation,
	SystemModuleInformationEx,
	SystemVerifierTriageInformation,
	SystemSuperfetchInformation,
	SystemMemoryListInformation,
	SystemFileCacheInformationEx,
	MaxSystemInfoClass
} SYSTEM_INFORMATION_CLASS;


typedef struct _SYSTEM_PROCESS_INFORMATION
{
	ULONG NextEntryOffset;
	ULONG NumberOfThreads;

	LARGE_INTEGER SpareLi1;
	LARGE_INTEGER SpareLi2;
	LARGE_INTEGER SpareLi3;

	LARGE_INTEGER CreateTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER KernelTime;

	UNICODE_STRING ImageName;
	KPRIORITY BasePriority;

	HANDLE UniqueProcessId;
	HANDLE InheritedFromUniqueProcessId;

	ULONG HandleCount;
	ULONG SessionId;
	ULONG_PTR PageDirectoryBase;

	SIZE_T PeakVirtualSize;
	SIZE_T VirtualSize;
	ULONG PageFaultCount;

	SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize;

	SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage;
	SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage;

	SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;

	LARGE_INTEGER ReadOperationCount;
	LARGE_INTEGER WriteOperationCount;
	LARGE_INTEGER OtherOperationCount;

	LARGE_INTEGER ReadTransferCount;
	LARGE_INTEGER WriteTransferCount;
	LARGE_INTEGER OtherTransferCount;

} SYSTEM_PROCESS_INFORMATION, * PSYSTEM_PROCESS_INFORMATION;


typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
	USHORT UniqueProcessId;        // 句柄持有者的PID
	USHORT CreatorBackTraceIndex;  // 创建回溯索引
	UCHAR ObjectTypeIndex;         // 目标对象的类型索引
	UCHAR HandleAttributes;        // 句柄属性
	USHORT HandleValue;            // 句柄值
	PVOID Object;                 // 句柄指向的对象地址
	ULONG GrantedAccess;          // 授予的访问权限
} SYSTEM_HANDLE_TABLE_ENTRY_INFO,* PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG NumberOfHandles;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;


NTSTATUS
NTAPI
ZwQuerySystemInformation(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength
);


typedef NTSTATUS(*FN_PspTerminateThreadByPointer)(
	PETHREAD Thread,
	NTSTATUS ExitStatus,
	BOOLEAN DirectTerminate
	);


FN_PspTerminateThreadByPointer g_PspTerminateThreadByPointer = NULL;

VOID EnumProcess();
VOID EnumActiveProcess();
VOID EnumProcessbySystemInfo();
VOID EnumProcessByHandleInfo();
NTSTATUS KillProcess(HANDLE processId);



VOID EnumProcess()
{
	NTSTATUS status;
	
	ULONG process_count = 0;

	for (ULONG pid = 4; pid < 65535; pid+=4)
	{
		PEPROCESS process=NULL;
		status = PsLookupProcessByProcessId((HANDLE)pid, &process);
		if (NT_SUCCESS(status))
		{
			PUCHAR name = (PUCHAR)process + OFFSET_EPROCESS_IMAGE_FILE_NAME;
			if (process != NULL)
			{
				DbgPrint("[JAKE] PID=%p Name=%.*s\n",
					(HANDLE)pid,
					EPROCESS_IMAGE_FILE_NAME_LENGTH,
					(PCSTR)name);
				process_count++;
			}

			ObDereferenceObject(process);
			
		}
	}
	DbgPrint("[JAKE] EnumProcess结束 counts=%lu\n", process_count);
}

VOID EnumActiveProcess()
{
	NTSTATUS status;
	PEPROCESS current=NULL;
	PEPROCESS head = NULL;
	current = PsGetCurrentProcess();
	ULONG active_link_counts=1;
	head = current;

	
	do{
		//PFILE_OBJECT OBJECT_POINTER = *(PFILE_OBJECT*)((PUCHAR)current + OFFSET_EPROCESS_IMAGE_FILE_POINTER);
		//UNICODE_STRING filename = { 0 };
		//if (OBJECT_POINTER != NULL) { filename = OBJECT_POINTER->FileName; }
		HANDLE pid = *(HANDLE*)((PUCHAR)current + OFFSET_EPROCESS_PID);
		PCSTR name = (PCSTR)((PUCHAR)current + OFFSET_EPROCESS_IMAGE_FILE_NAME);



		DbgPrint(
			"[JAKE] EPROCESS=%p PID=%p Name=%.*s \n",
			(PVOID)current,
			pid,
			EPROCESS_IMAGE_FILE_NAME_LENGTH,
			name
			/*(PVOID)OBJECT_POINTER,
			&filename*/
		);
		PLIST_ENTRY ActiveProcessLinks = (PLIST_ENTRY)((PUCHAR)current + OFFSET_EPROCESS_ACTIVE_PROCESS_LINKS);
		PLIST_ENTRY nextLink = ActiveProcessLinks->Flink;
		current = (PEPROCESS)((PUCHAR)nextLink - OFFSET_EPROCESS_ACTIVE_PROCESS_LINKS);
		active_link_counts++;
	} while (current != head);
}


VOID EnumProcessbySystemInfo()
{
	NTSTATUS status;
	PVOID buffer = NULL;
	ULONG bufferSize = 0x10000;
	ULONG return_length = 0;

	PSYSTEM_PROCESS_INFORMATION processInfo = NULL;
	ULONG count = 0;

	while (1)
	{
		
		buffer = ExAllocatePool(NonPagedPool, bufferSize);
		if (buffer == NULL)
		{
			return;
		}

		status = ZwQuerySystemInformation(
			SystemProcessInformation,
			
			buffer,
			bufferSize,
			&return_length
		);

		if (status == STATUS_INFO_LENGTH_MISMATCH)
		{
			ExFreePool(buffer);
			buffer = NULL;
			bufferSize = return_length + 0x1000;
			continue;
		}

		break;
	}

	if (!NT_SUCCESS(status))
	{
		ExFreePool(buffer);
		return;
	}

	processInfo = (PSYSTEM_PROCESS_INFORMATION)buffer;
	while (1)
	{
		// 1. 打印当前记录
		if (processInfo->ImageName.Buffer != NULL)
		{
			DbgPrint("[JAKE] PID=%p Name=%wZ\n",
				(PVOID)processInfo->UniqueProcessId,
				&processInfo->ImageName);
		}
		else
		{
			DbgPrint("[JAKE] PID=%p Name=<empty>\n",
				(PVOID)processInfo->UniqueProcessId);
		}

		// 2. 当前记录计入总数
		count++;

		// 3. 当前是最后一条，结束遍历
		if (processInfo->NextEntryOffset == 0)
		{
			break;
		}

		// 4. 按字节偏移移动到下一条记录
		processInfo = (PSYSTEM_PROCESS_INFORMATION)(
			(PUCHAR)processInfo + processInfo->NextEntryOffset);
	}

	// 遍历结束，打印数量并释放整块缓冲区
	DbgPrint("[JAKE] EnumProcessBySystemInfo结束 Count=%lu\n", count);

	ExFreePool(buffer);
	buffer = NULL;
	processInfo = NULL;
	
}


VOID EnumProcessByHandleInfo()
{
	NTSTATUS status;
	PVOID buffer = NULL;
	ULONG bufferSize = 0x10000;
	ULONG return_length = 0;

	PSYSTEM_HANDLE_INFORMATION handle_info = NULL;

	PULONG foundPids = NULL;
	ULONG foundCount = 0;
	ULONG successCount = 0;
	ULONG failedCount = 0;
	BOOLEAN truncated = FALSE;

	// 1. 分配去重数组，最多保存4096个不同PID
	foundPids = (PULONG)ExAllocatePool(
		NonPagedPool,
		4096 * sizeof(ULONG)
	);

	if (foundPids == NULL)
	{
		return;
	}

	RtlZeroMemory(foundPids, 4096 * sizeof(ULONG));

	// 2. 查询系统句柄信息
	while (1)
	{
		buffer = ExAllocatePool(NonPagedPool, bufferSize);
		if (buffer == NULL)
		{
			ExFreePool(foundPids);
			return;
		}

		return_length = 0;

		status = ZwQuerySystemInformation(
			SystemHandleInformation,
			buffer,
			bufferSize,
			&return_length
		);

		if (status == STATUS_INFO_LENGTH_MISMATCH)
		{
			ExFreePool(buffer);
			buffer = NULL;

			// 防止扩容加法溢出
			if (return_length > MAXULONG - 0x1000)
			{
				ExFreePool(foundPids);
				return;
			}

			bufferSize = return_length + 0x1000;
			continue;
		}

		break;
	}

	if (!NT_SUCCESS(status))
	{
		DbgPrint("[JAKE] HandleInfo查询失败 Status=0x%08lX\n",
			(ULONG)status);

		ExFreePool(buffer);
		ExFreePool(foundPids);
		return;
	}

	handle_info = (PSYSTEM_HANDLE_INFORMATION)buffer;

	// 3. 遍历句柄记录
	for (ULONG i = 0; i < handle_info->NumberOfHandles; i++)
	{
		ULONG pid = handle_info->Handles[i].UniqueProcessId;
		BOOLEAN alreadyFound = FALSE;

		// 4. 检查这个持有者PID是否已经登记
		for (ULONG j = 0; j < foundCount; j++)
		{
			if (foundPids[j] == pid)
			{
				alreadyFound = TRUE;
				break;
			}
		}

		if (alreadyFound)
		{
			continue;
		}

		if (foundCount >= 4096)
		{
			truncated = TRUE;
			break;
		}

		// 5. 新PID先登记，然后查询进程对象
		foundPids[foundCount++] = pid;

		PEPROCESS process = NULL;

		status = PsLookupProcessByProcessId(
			(HANDLE)(ULONG_PTR)pid,
			&process
		);

		if (NT_SUCCESS(status))
		{
			PCSTR name = (PCSTR)(
				(PUCHAR)process + OFFSET_EPROCESS_IMAGE_FILE_NAME);

			DbgPrint("[JAKE] OwnerPID=%lu Name=%.*s\n",
				pid,
				(int)EPROCESS_IMAGE_FILE_NAME_LENGTH,
				name);

			ObDereferenceObject(process);
			successCount++;
		}
		else
		{
			DbgPrint("[JAKE] OwnerPID=%lu LookupStatus=0x%08lX\n",
				pid,
				(ULONG)status);

			failedCount++;
		}
	}

	DbgPrint(
		"[JAKE] TotalHandles=%lu FoundPIDs=%lu "
		"Success=%lu Failed=%lu Truncated=%u\n",
		handle_info->NumberOfHandles,
		foundCount,
		successCount,
		failedCount,
		(unsigned int)truncated
	);

	// 6. 释放两块内存
	ExFreePool(buffer);
	ExFreePool(foundPids);
}


NTSTATUS KillProcess(HANDLE processId)
{
	NTSTATUS status;
	PEPROCESS targetProcess = NULL;
	
	// 检查函数指针是否已赋值
	if (g_PspTerminateThreadByPointer == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

	// 根据 PID 获取目标进程对象
	status = PsLookupProcessByProcessId(
		processId,
		&targetProcess
	);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	DbgPrint("[JAKE] Target EPROCESS: %p\n",
		(PVOID)targetProcess);

	for (ULONG_PTR tid = 4; tid < 0xFFFFFF; tid += 4)
	{
		PETHREAD thread = NULL;

		NTSTATUS lookupStatus = PsLookupThreadByThreadId(
			(HANDLE)tid,
			&thread
		);

		if (!NT_SUCCESS(lookupStatus))
		{
			continue;
		}

		PEPROCESS ownerProcess = IoThreadToProcess(thread);

		if (ownerProcess == targetProcess)
		{
			DbgPrint("[JAKE] Matched TID=%p, ETHREAD=%p\n",
				(PVOID)tid,
				(PVOID)thread);
			g_PspTerminateThreadByPointer(thread, 0, 1);
		}
		ObDereferenceObject(thread);
	}

	ObDereferenceObject(targetProcess);
	return STATUS_SUCCESS;

}


VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	DbgPrint("[JAKE] unloaded\n");
}



NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING path)
{
	NTSTATUS status;
	DriverObject->DriverUnload = DriverUnload;

	g_PspTerminateThreadByPointer =
		(FN_PspTerminateThreadByPointer)
		PSP_TERMINATE_THREAD_BY_POINTER_ADDR;

	//EnumProcess();
	//EnumActiveProcess();
	//EnumProcessbySystemInfo();
	//EnumProcessByHandleInfo();

	status=KillProcess(5148);
	return status;
} 
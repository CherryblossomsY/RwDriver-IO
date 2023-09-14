#include<ntifs.h>
//定义设备名
#define DEVICE_NAME L"\\Device\\MyDevice"
#define SYMBOL_NAME_LINK L"\\??\\DeviceCx"

#define CODE_READ CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CODE_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

//定义结构
typedef struct _MyStruct
{
	ULONG	ProcessID;
	PVOID	Addre;
	ULONG	Length;
	PVOID	Buff;
} MyStruct, * PMyStruct;


//写
NTSTATUS WriteMemory(ULONG ProcessID, PVOID Addre, ULONG Length, PVOID Buff)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PEPROCESS pEProcess = NULL;
	KAPC_STATE ApcState = { 0 };

	PVOID tempbuff = ExAllocatePool(NonPagedPool, Length);  //申请内存地址
	if (tempbuff == NULL)
	{
		DbgPrint("[+]申请地址失败\n");
		return STATUS_UNSUCCESSFUL;
	}
	RtlCopyMemory(tempbuff, Buff, Length);//写入的数据存在内存地址里

	Status = PsLookupProcessByProcessId((HANDLE)ProcessID, &pEProcess);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("[+]进程查找错误\n");
		ExFreePool(tempbuff); //释放内存地址
		return STATUS_UNSUCCESSFUL;
	}
	KeStackAttachProcess(pEProcess, &ApcState);
	__try
	{
		ProbeForWrite(Addre, Length, 1);//是否可写
		RtlCopyMemory(Addre, tempbuff, Length); //写入数据
		Status = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Status = STATUS_UNSUCCESSFUL;
	}
	KeUnstackDetachProcess(&ApcState);
	ObDereferenceObject(pEProcess);
	ExFreePool(tempbuff); //释放内存地址
	return Status;
}

//读
NTSTATUS ReadMemory(ULONG ProcessID, PVOID Addr, ULONG Length, PVOID Buff)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PEPROCESS pEProcess = NULL;
	KAPC_STATE ApcState = { 0 };
	Status = PsLookupProcessByProcessId((HANDLE)ProcessID, &pEProcess);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("[+]进程查找错误\n");
		return STATUS_UNSUCCESSFUL;
	}
	KeStackAttachProcess(pEProcess, &ApcState);
	__try
	{
		ProbeForRead(Addr, Length, 1);//校验是否可读
		RtlCopyMemory(Buff, Addr, Length); //读取进程数据
		Status = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Status = STATUS_UNSUCCESSFUL;

		RtlZeroMemory(Buff, Length); //清零返回值
	}
	KeUnstackDetachProcess(&ApcState);
	ObDereferenceObject(pEProcess);
	return Status;
}


VOID DriverUnload(PDRIVER_OBJECT driver)
{
	UNICODE_STRING symbolLink;
	RtlInitUnicodeString(&symbolLink, SYMBOL_NAME_LINK);
	IoDeleteSymbolicLink(&symbolLink);
	IoDeleteDevice(driver->DeviceObject);
	DbgPrint("[+]卸载\n");
	return;
}

//连接设备
NTSTATUS CreateCallBack(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
	DbgPrint("[+]链接了\n");
	Irp->IoStatus.Information = STATUS_SUCCESS;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//关闭设备
NTSTATUS CloseCallBack(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
	DbgPrint("[+]关闭了\n");
	Irp->IoStatus.Information = STATUS_SUCCESS;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//派发函数
NTSTATUS DispathCallBack(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION Psl = IoGetCurrentIrpStackLocation(Irp);
	ULONG Code = Psl->Parameters.DeviceIoControl.IoControlCode;
	PVOID systemBuf = Irp->AssociatedIrp.SystemBuffer;
	ULONG inLen = Psl->Parameters.DeviceIoControl.InputBufferLength;
	ULONG InputDataLength = 0, OutputDataLength = 0, IoControlCode = 0;
	InputDataLength = Psl->Parameters.DeviceIoControl.InputBufferLength;
	OutputDataLength = Psl->Parameters.DeviceIoControl.OutputBufferLength;
	DbgPrint("[+]进派发函数啦\n");
	switch (Code)
	{
	case CODE_READ:
		ReadMemory(((PMyStruct)systemBuf)->ProcessID, ((PMyStruct)systemBuf)->Addre, ((PMyStruct)systemBuf)->Length, systemBuf);
		Status = STATUS_SUCCESS;
		break;
	case CODE_WRITE:
		WriteMemory(((PMyStruct)systemBuf)->ProcessID, ((PMyStruct)systemBuf)->Addre, ((PMyStruct)systemBuf)->Length, ((PMyStruct)systemBuf)->Buff);
		Status = STATUS_SUCCESS;
		break;
	default:
		Status = STATUS_UNSUCCESSFUL;
		break;
	}

	if (Status == STATUS_SUCCESS)
	{
		Irp->IoStatus.Information = OutputDataLength;
	}
	else
	{
		Irp->IoStatus.Information = 0;
	}

	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

// DriverEntry，入口函数。相当于main。
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
	driver->DriverUnload = DriverUnload;

	//驱动通信初学
	//Unicode参数
	UNICODE_STRING DeivceName;
	UNICODE_STRING SymBolNameLink;
	//初始化设备名
	RtlInitUnicodeString(&DeivceName, DEVICE_NAME);
	//初始化设备链接名
	RtlInitUnicodeString(&SymBolNameLink, SYMBOL_NAME_LINK);
	//给IO的最后一个参数 是个二级指针的PDEVICE_OBJECT
	PDEVICE_OBJECT pDEVICEObj;
	//创建一个状态
	NTSTATUS Status;
	//创建IO通信
	Status = IoCreateDevice(driver, 0, &DeivceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &pDEVICEObj);

	//判断设备时候是否创建失败
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("[+]设备创建失败\n");
		return Status;
	}
	//创建一个符号链接
	Status = IoCreateSymbolicLink(&SymBolNameLink, &DeivceName);
	//判断设备链接是否创建失败
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("[+]设备链接创建失败\n");
		//设备链接创建失败，卸载设备对象
		IoDeleteDevice(pDEVICEObj);
		return Status;
	}
	//
	pDEVICEObj->Flags |= DO_BUFFERED_IO;
	//创建
	driver->MajorFunction[IRP_MJ_CREATE] = CreateCallBack;
	//关闭
	driver->MajorFunction[IRP_MJ_CLOSE] = CloseCallBack;
	//控制
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispathCallBack;
	//输出初学
	DbgPrint("[+]加载\n");

	// 设置一个卸载函数，便于这个函数退出

	return STATUS_SUCCESS;
}
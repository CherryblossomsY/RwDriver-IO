#include<ntifs.h>
//�����豸��
#define DEVICE_NAME L"\\Device\\MyDevice"
#define SYMBOL_NAME_LINK L"\\??\\DeviceCx"

#define CODE_READ CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CODE_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

//����ṹ
typedef struct _MyStruct
{
	ULONG	ProcessID;
	PVOID	Addre;
	ULONG	Length;
	PVOID	Buff;
} MyStruct, * PMyStruct;


//д
NTSTATUS WriteMemory(ULONG ProcessID, PVOID Addre, ULONG Length, PVOID Buff)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PEPROCESS pEProcess = NULL;
	KAPC_STATE ApcState = { 0 };

	PVOID tempbuff = ExAllocatePool(NonPagedPool, Length);  //�����ڴ��ַ
	if (tempbuff == NULL)
	{
		DbgPrint("[+]�����ַʧ��\n");
		return STATUS_UNSUCCESSFUL;
	}
	RtlCopyMemory(tempbuff, Buff, Length);//д������ݴ����ڴ��ַ��

	Status = PsLookupProcessByProcessId((HANDLE)ProcessID, &pEProcess);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("[+]���̲��Ҵ���\n");
		ExFreePool(tempbuff); //�ͷ��ڴ��ַ
		return STATUS_UNSUCCESSFUL;
	}
	KeStackAttachProcess(pEProcess, &ApcState);
	__try
	{
		ProbeForWrite(Addre, Length, 1);//�Ƿ��д
		RtlCopyMemory(Addre, tempbuff, Length); //д������
		Status = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Status = STATUS_UNSUCCESSFUL;
	}
	KeUnstackDetachProcess(&ApcState);
	ObDereferenceObject(pEProcess);
	ExFreePool(tempbuff); //�ͷ��ڴ��ַ
	return Status;
}

//��
NTSTATUS ReadMemory(ULONG ProcessID, PVOID Addr, ULONG Length, PVOID Buff)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PEPROCESS pEProcess = NULL;
	KAPC_STATE ApcState = { 0 };
	Status = PsLookupProcessByProcessId((HANDLE)ProcessID, &pEProcess);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("[+]���̲��Ҵ���\n");
		return STATUS_UNSUCCESSFUL;
	}
	KeStackAttachProcess(pEProcess, &ApcState);
	__try
	{
		ProbeForRead(Addr, Length, 1);//У���Ƿ�ɶ�
		RtlCopyMemory(Buff, Addr, Length); //��ȡ��������
		Status = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Status = STATUS_UNSUCCESSFUL;

		RtlZeroMemory(Buff, Length); //���㷵��ֵ
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
	DbgPrint("[+]ж��\n");
	return;
}

//�����豸
NTSTATUS CreateCallBack(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
	DbgPrint("[+]������\n");
	Irp->IoStatus.Information = STATUS_SUCCESS;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//�ر��豸
NTSTATUS CloseCallBack(_In_ struct _DEVICE_OBJECT* DeviceObject, _Inout_ struct _IRP* Irp)
{
	DbgPrint("[+]�ر���\n");
	Irp->IoStatus.Information = STATUS_SUCCESS;
	IofCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//�ɷ�����
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
	DbgPrint("[+]���ɷ�������\n");
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

// DriverEntry����ں������൱��main��
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
	driver->DriverUnload = DriverUnload;

	//����ͨ�ų�ѧ
	//Unicode����
	UNICODE_STRING DeivceName;
	UNICODE_STRING SymBolNameLink;
	//��ʼ���豸��
	RtlInitUnicodeString(&DeivceName, DEVICE_NAME);
	//��ʼ���豸������
	RtlInitUnicodeString(&SymBolNameLink, SYMBOL_NAME_LINK);
	//��IO�����һ������ �Ǹ�����ָ���PDEVICE_OBJECT
	PDEVICE_OBJECT pDEVICEObj;
	//����һ��״̬
	NTSTATUS Status;
	//����IOͨ��
	Status = IoCreateDevice(driver, 0, &DeivceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, TRUE, &pDEVICEObj);

	//�ж��豸ʱ���Ƿ񴴽�ʧ��
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("[+]�豸����ʧ��\n");
		return Status;
	}
	//����һ����������
	Status = IoCreateSymbolicLink(&SymBolNameLink, &DeivceName);
	//�ж��豸�����Ƿ񴴽�ʧ��
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("[+]�豸���Ӵ���ʧ��\n");
		//�豸���Ӵ���ʧ�ܣ�ж���豸����
		IoDeleteDevice(pDEVICEObj);
		return Status;
	}
	//
	pDEVICEObj->Flags |= DO_BUFFERED_IO;
	//����
	driver->MajorFunction[IRP_MJ_CREATE] = CreateCallBack;
	//�ر�
	driver->MajorFunction[IRP_MJ_CLOSE] = CloseCallBack;
	//����
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispathCallBack;
	//�����ѧ
	DbgPrint("[+]����\n");

	// ����һ��ж�غ�����������������˳�

	return STATUS_SUCCESS;
}
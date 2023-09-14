#include <Windows.h>
//Ioͨ��ͷ�ļ�
#include <winioctl.h>
#include <stdlib.h>
#include <stdio.h>

HANDLE handle;

typedef struct _MyDataStruct
{
	ULONG64	ProcessID;
	ULONG64	Address;
	ULONG64	Length;
	ULONG64* Buff;
} MyDataStruct, * PMyDataStruct;

#define CODE_READ CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define CODE_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define SYMBOL_NAME_LINK L"\\\\.\\DeviceCx"



BOOLEAN OpenDevice(HANDLE* hanlde)
{
	HANDLE _hanlde = CreateFile(SYMBOL_NAME_LINK, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	*hanlde = _hanlde;
	return ((int)_hanlde) > 0;
}

void CloseDevice(HANDLE hanlde)
{
	//�رվ��
	CloseHandle(hanlde);
}

BOOLEAN SendCode(HANDLE Handle, DWORD code, PVOID inData, ULONG64 InLen, PVOID OutData, ULONG64 OutLen, LPDWORD ResuLtLen)
{
	return DeviceIoControl(Handle, code, inData, InLen, OutData, OutLen, ResuLtLen, NULL);
}

//��
template <typename DataType> DataType ReadMem(ULONG64 PID, ULONG64 address)
{
	MyDataStruct data;
	data.ProcessID = PID;
	data.Address = address;
	data.Length = sizeof(DataType);
	DataType pBuffer;
	DeviceIoControl(handle, CODE_READ, &data, 32, &pBuffer, data.Length, NULL, NULL);
	//CloseDevice(handle);
	return pBuffer;
}

//д
template <typename DataType> VOID WriteMem(ULONG64 PID, ULONG64 address, DataType value)
{
	MyDataStruct data;
	data.ProcessID = PID;
	data.Address = address;
	data.Length = sizeof(DataType);
	data.Buff = &value;
	DeviceIoControl(handle, CODE_WRITE, &data, 32, NULL, NULL, NULL, NULL);
}

int main()
{
	if (!OpenDevice(&handle))
	{
		printf("�򿪾��ʧ�ܣ�\n");
		return 0;
	}
	printf("ULONG64->Buf=%llx\n", ReadMem<ULONG64>(0x1E14, 0x7FFA638BF000));
	printf("DWORD->Buf=%d\n", ReadMem<DWORD>(0x1E14, 0x7FFA638BF000));

	WriteMem<ULONG64>(0x1E14, 0x7FFA638BF000, 0x7FFA6387FCA8);
	//WriteMem<DWORD>(0x1E14, 0x7FFA638BF000)

	system("pause");
	return 0;
}



//By ChuXue 
//2023/9/14
//���������ж�д��������
//������װ��Ҫ�ֶ���װ
//�����ʱ��Ļ��������Լ�дһ����main�������Զ���װ
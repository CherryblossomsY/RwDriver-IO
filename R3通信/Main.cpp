#include <Windows.h>
//Io通信头文件
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
	//关闭句柄
	CloseHandle(hanlde);
}

BOOLEAN SendCode(HANDLE Handle, DWORD code, PVOID inData, ULONG64 InLen, PVOID OutData, ULONG64 OutLen, LPDWORD ResuLtLen)
{
	return DeviceIoControl(Handle, code, inData, InLen, OutData, OutLen, ResuLtLen, NULL);
}

//读
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

//写
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
		printf("打开句柄失败！\n");
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
//此驱动仅有读写两个功能
//驱动安装需要手动安装
//如果有时间的话，可以自己写一个在main函数中自动安装
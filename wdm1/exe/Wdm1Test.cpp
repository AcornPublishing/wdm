//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm1Test example
/////////////////////////////////////////////////////////////////////////////
//	Wdm1Test.cpp:	Win32 console application to exercise Wdm1 devices
/////////////////////////////////////////////////////////////////////////////
//	main					Program main line
//	GetDeviceViaInterface	Open a handle via a device interface
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "windows.h"
#include "c:\98ddk\inc\win98\setupapi.h"	// VC++ 5 one is out of date
#include "stdio.h"
#include "initguid.h"
#include "..\sys\GUIDs.h"
#include "winioctl.h"
#include "..\sys\Ioctl.h"

HANDLE GetDeviceViaInterface( GUID* pGuid, DWORD instance);

int main(int argc, char* argv[])
{
	int TestNo = 1;

	printf("\nWdm1Test\n");

	/////////////////////////////////////////////////////////////////////////
	// Open device
	printf("\nTest %d\n",TestNo++);
	HANDLE hWdm1 = GetDeviceViaInterface((LPGUID)&WDM1_GUID,0);
	if( hWdm1==NULL)
	{
		printf("XXX  Could not find open Wdm1 device\n");
		return 1;
	}
	printf("     Opened OK\n");

	/////////////////////////////////////////////////////////////////////////
	// Read first ULONG that's left in buffer
	printf("\nTest %d\n",TestNo++);
	DWORD TxdBytes;
	ULONG Rvalue = 0;
	if( !ReadFile( hWdm1, &Rvalue, 4, &TxdBytes, NULL))
		printf("XXX  Could not read value %d\n", GetLastError());
	else if( TxdBytes==4)
		printf("     Read successfully read stored value of 0x%X\n",Rvalue);
	else
		printf("XXX  Wrong number of bytes read: %d\n",TxdBytes);

	/////////////////////////////////////////////////////////////////////////
	// Write 0x12345678
	printf("\nTest %d\n",TestNo++);
	ULONG Wvalue = 0x12345678;
	if( !WriteFile( hWdm1, &Wvalue, 4, &TxdBytes, NULL))
		printf("XXX  Could not write %X\n",Wvalue);
	else if( TxdBytes==4)
		printf("     Write 0x12345678 succeeded\n");
	else
		printf("XXX  Wrong number of bytes written: %d\n",TxdBytes);

	/////////////////////////////////////////////////////////////////////////
	// Set file pointer
	printf("\nTest %d\n",TestNo++);
	DWORD dwNewPtr = SetFilePointer( hWdm1, 3, NULL, FILE_BEGIN);
	if( dwNewPtr==0xFFFFFFFF)
		printf("XXX  SetFilePointer failed %d\n",GetLastError());
	else
		printf("     SetFilePointer worked\n");

	/////////////////////////////////////////////////////////////////////////
	// Read
	printf("\nTest %d\n",TestNo++);
	Rvalue = 0;
	if( !ReadFile( hWdm1, &Rvalue, 1, &TxdBytes, NULL))
		printf("XXX  Could not read value\n");
	else if( TxdBytes==1)
		printf("     Read successfully read stored value of 0x%X\n",Rvalue);
	else
		printf("XXX  Wrong number of bytes read: %d\n",TxdBytes);

	/////////////////////////////////////////////////////////////////////////
	// Write
	printf("\nTest %d\n",TestNo++);
	if( !WriteFile( hWdm1, &Wvalue, 4, &TxdBytes, NULL))
		printf("XXX  Could not write %X\n",Wvalue);
	else if( TxdBytes==4)
		printf("     Write at new file pointer succeeded\n");
	else
		printf("XXX  Wrong number of bytes written: %d\n",TxdBytes);

	/////////////////////////////////////////////////////////////////////////
	// Get buffer size
	printf("\nTest %d\n",TestNo++);
	ULONG BufferSize;
	DWORD BytesReturned;
	if( !DeviceIoControl( hWdm1, IOCTL_WDM1_GET_BUFFER_SIZE,
							NULL, 0,					// Input
							&BufferSize, sizeof(ULONG),	// Output
							&BytesReturned, NULL))
		printf("XXX  Could not get buffer size\n");
	else
		printf("     Buffer size is %i (%d bytes returned)\n",BufferSize,BytesReturned);

	/////////////////////////////////////////////////////////////////////////
	// Get buffer size
	printf("\nTest %d\n",TestNo++);
	char* Buffer = new char[BufferSize+1];
	if( !DeviceIoControl( hWdm1, IOCTL_WDM1_GET_BUFFER,
							NULL, 0,			// Input
							Buffer, BufferSize,	// Output
							&BytesReturned, NULL))
		printf("XXX  Could not get buffer\n");
	else
		printf("     First DWORD of buffer is %08X (%d bytes returned)\n",*((DWORD*)Buffer),BytesReturned);

	/////////////////////////////////////////////////////////////////////////
	// Get too big a buffer size
	printf("\nTest %d\n",TestNo++);
	if( !DeviceIoControl( hWdm1, IOCTL_WDM1_GET_BUFFER,
							NULL, 0,				// Input
							Buffer, BufferSize+1,	// Output
							&BytesReturned, NULL))
		printf("     Too big get buffer failed correctly %d\n",GetLastError());
	else
		printf("XXX  Too big get buffer unexpectedly succeeded\n");

	/////////////////////////////////////////////////////////////////////////
	// Zero all buffer bytes
	printf("\nTest %d\n",TestNo++);
	if( !DeviceIoControl( hWdm1, IOCTL_WDM1_ZERO_BUFFER,
							NULL, 0,	// Input
							NULL, 0,	// Output
							&BytesReturned, NULL))
		printf("XXX  Zero buffer failed %d\n",GetLastError());
	else
		printf("     Zero buffer succeeded\n");
	if( !DeviceIoControl( hWdm1, IOCTL_WDM1_GET_BUFFER,
							NULL, 0,			// Input
							Buffer, BufferSize,	// Output
							&BytesReturned, NULL))
		printf("XXX  Could not get buffer\n");
	else
		printf("     First DWORD of buffer is %08X (%d bytes returned)\n",*((DWORD*)Buffer),BytesReturned);

	/////////////////////////////////////////////////////////////////////////
	// Remove buffer
	printf("\nTest %d\n",TestNo++);
	if( !DeviceIoControl( hWdm1, IOCTL_WDM1_REMOVE_BUFFER,
							NULL, 0,	// Input
							NULL, 0,	// Output
							&BytesReturned, NULL))
		printf("XXX  Remove buffer failed %d\n",GetLastError());
	else
		printf("     Remove buffer succeeded\n");
	if( !DeviceIoControl( hWdm1, IOCTL_WDM1_GET_BUFFER_SIZE,
							NULL, 0,					// Input
							&BufferSize, sizeof(ULONG),	// Output
							&BytesReturned, NULL))
		printf("XXX  Could not get buffer size\n");
	else
		printf("     Buffer size is %i (%d bytes returned)\n",BufferSize,BytesReturned);

	/////////////////////////////////////////////////////////////////////////
	// Unrecognised IOCTL
	printf("\nTest %d\n",TestNo++);
	if( !DeviceIoControl( hWdm1, IOCTL_WDM1_UNRECOGNISED,
							NULL, 0,	// Input
							NULL, 0,	// Output
							&BytesReturned, NULL))
		printf("     Unrecognised IOCTL correctly failed %d\n",GetLastError());
	else
		printf("XXX  Unrecognised IOCTL unexpectedly succeeded\n");

	/////////////////////////////////////////////////////////////////////////
	// Write 0xabcdef01 to start of buffer
	printf("\nTest %d\n",TestNo++);
	dwNewPtr = SetFilePointer( hWdm1, 0, NULL, FILE_BEGIN);
	if( dwNewPtr==0xFFFFFFFF)
		printf("XXX  SetFilePointer failed %d\n",GetLastError());
	else
		printf("     SetFilePointer worked\n");
	Wvalue = 0xabcdef01;
	if( !WriteFile( hWdm1, &Wvalue, 4, &TxdBytes, NULL))
		printf("XXX  Could not write %X\n",Wvalue);
	else if( TxdBytes==4)
		printf("     Write 0xabcdef01 succeeded\n");
	else
		printf("XXX  Wrong number of bytes written: %d\n",TxdBytes);

	/////////////////////////////////////////////////////////////////////////
	// Close device
	printf("\nTest %d\n",TestNo++);
	if( !CloseHandle(hWdm1))
		printf("XXX  CloseHandle failed %d\n",GetLastError());
	else
		printf("     CloseHandle worked\n");

	/////////////////////////////////////////////////////////////////////////
	delete Buffer;
	printf("\nPress enter please");
	char line[80];
	gets(line);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
//	GetDeviceViaInterface:	Open a handle via a device interface

HANDLE GetDeviceViaInterface( GUID* pGuid, DWORD instance)
{
	// Get handle to relevant device information set
	HDEVINFO info = SetupDiGetClassDevs(pGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
	if(info==INVALID_HANDLE_VALUE)
	{
		printf("No HDEVINFO available for this GUID\n");
		return NULL;
	}

	// Get interface data for the requested instance
	SP_INTERFACE_DEVICE_DATA ifdata;
	ifdata.cbSize = sizeof(ifdata);
	if(!SetupDiEnumDeviceInterfaces(info, NULL, pGuid, instance, &ifdata))
	{
		printf("No SP_INTERFACE_DEVICE_DATA available for this GUID instance\n");
		SetupDiDestroyDeviceInfoList(info);
		return NULL;
	}

	// Get size of symbolic link name
	DWORD ReqLen;
	SetupDiGetDeviceInterfaceDetail(info, &ifdata, NULL, 0, &ReqLen, NULL);
	PSP_INTERFACE_DEVICE_DETAIL_DATA ifDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)(new char[ReqLen]);
	if( ifDetail==NULL)
	{
		SetupDiDestroyDeviceInfoList(info);
		return NULL;
	}

	// Get symbolic link name
	ifDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
	if( !SetupDiGetDeviceInterfaceDetail(info, &ifdata, ifDetail, ReqLen, NULL, NULL))
	{
		SetupDiDestroyDeviceInfoList(info);
		delete ifDetail;
		return NULL;
	}

	printf("Symbolic link is %s\n",ifDetail->DevicePath);
	// Open file
	HANDLE rv = CreateFile( ifDetail->DevicePath, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if( rv==INVALID_HANDLE_VALUE) rv = NULL;

	delete ifDetail;
	SetupDiDestroyDeviceInfoList(info);
	return rv;
}

/////////////////////////////////////////////////////////////////////////////

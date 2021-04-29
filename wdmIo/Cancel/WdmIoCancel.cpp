//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	WdmIoCancel example
/////////////////////////////////////////////////////////////////////////////
//	WdmIoCancel.cpp:	Win32 console application to check WdmIo cancel and cleanup
/////////////////////////////////////////////////////////////////////////////
//	main					Program main line
//	GetDeviceViaInterfaceOl	Open a handle via a device interface overlapped
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "windows.h"
#include "c:\98ddk\inc\win98\setupapi.h"	// VC++ 5 one is out of date
#include "stdio.h"
#include "initguid.h"
#include "..\..\GUIDs.h"
#include "winioctl.h"
#include "..\sys\Ioctl.h"

/////////////////////////////////////////////////////////////////////////////

HANDLE GetDeviceViaInterface( GUID* pGuid, DWORD instance);
HANDLE GetDeviceViaInterfaceOl( GUID* pGuid, DWORD instance);

/////////////////////////////////////////////////////////////////////////////

BYTE	PARPORT_DATA	= 0;
BYTE	PARPORT_STATUS	= 1;
BYTE	PARPORT_CONTROL	= 2;

/////////////////////////////////////////////////////////////////////////////

#define length(x) (sizeof(x)/sizeof(x[0]))

BYTE ConnectToInterrupts[] =
{
	PHDIO_WRITE, PARPORT_CONTROL, 0xCC,				// Disable interrupts
	PHDIO_TIMEOUT, 10,								// Write timeout of 1s
	PHDIO_IRQ_CONNECT, PARPORT_STATUS, 0x00, 0x00,	// Connect to interrupt
};

BYTE InitPrinter[] =
{
	PHDIO_WRITE, PARPORT_CONTROL, 0xC8,			// Take INIT low
	PHDIO_DELAY, 60,							// Delay 60us
	PHDIO_WRITE, PARPORT_CONTROL, 0xDC,			// INIT high, select printer, enable interrupts
	PHDIO_DELAY, 60,							// Delay 60us
};

BYTE ReadStatus[] =
{
	PHDIO_READ, PARPORT_STATUS,					// Read status
};

BYTE WriteByte[] =
{
	PHDIO_WRITE, PARPORT_CONTROL, 0xDC,			// Ensure STROBE off
	PHDIO_WRITE_NEXT, PARPORT_DATA,				// Write next byte
	PHDIO_DELAY, 1,								// Delay 1us
	PHDIO_WRITE, PARPORT_CONTROL, 0xDD,			// STROBE on
	PHDIO_DELAY, 1,								// Delay 1us
	PHDIO_WRITE, PARPORT_CONTROL, 0xDC,			// STROBE off
	PHDIO_DELAY, 1,								// Delay 1us

	PHDIO_READ, PARPORT_STATUS,					// Read status
};

BYTE DisableInterrupts[] =
{
	PHDIO_WRITE, PARPORT_CONTROL, 0xCC,			// Disable interrupts
};

/////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	int TestNo = 1;

	printf("\nWdmIoCancel\n");

	/////////////////////////////////////////////////////////////////////////
	// Open device
	printf("\nTest %d\n",TestNo++);
	HANDLE hWdmIo = GetDeviceViaInterfaceOl((LPGUID)&WDMIO_GUID,0);
	if( hWdmIo==NULL)
	{
		printf("XXX  Could not find open WdmIo device\n");
		return 1;
	}
	printf("     Opened OK\n");

	/////////////////////////////////////////////////////////////////////////
	// Run commands
	/////////////////////////////////////////////////////////////////////////
	// Connect to interrupts (but disable interrupts)
	printf("\nTest %d\n",TestNo++);
	DWORD BytesReturned;
	WORD rv[3];
	if( DeviceIoControl( hWdmIo, IOCTL_PHDIO_RUN_CMDS,
							ConnectToInterrupts, length(ConnectToInterrupts),	// Input
							rv, sizeof(rv),										// Output
							&BytesReturned, NULL))
		printf("     ConnectToInterrupts OK.  rv=%d at %d\n", rv[0], rv[1]);
	else
	{
		printf("XXX  ConnectToInterrupts failed %d\n",GetLastError());
		goto fail;
	}

	/////////////////////////////////////////////////////////////////////////
	// Init Printer
	printf("\nTest %d\n",TestNo++);
	if( DeviceIoControl( hWdmIo, IOCTL_PHDIO_RUN_CMDS,
							InitPrinter, length(InitPrinter),	// Input
							rv, sizeof(rv),						// Output
							&BytesReturned, NULL))
	{
		printf("     InitPrinter OK.  rv=%d at %d\n", rv[0], rv[1]);
	}
	else
	{
		printf("XXX  InitPrinter failed %d\n",GetLastError());
		goto fail;
	}

	/////////////////////////////////////////////////////////////////////////
	// Store transmit data commands
	printf("\nTest %d\n",TestNo++);
	if( DeviceIoControl( hWdmIo, IOCTL_PHDIO_CMDS_FOR_WRITE,
							WriteByte, length(WriteByte),	// Input
							NULL, 0,						// Output
							&BytesReturned, NULL))
	{
		printf("     Stored WriteByte cmds OK.");
	}
	else
	{
		printf("XXX  Storing WriteByte cmds failed %d\n",GetLastError());
		goto fail;
	}

	/////////////////////////////////////////////////////////////////////////
	// Write message
	printf("\nTest %d\n",TestNo++);

	{
	// Create Overlapped read structure and event
	HANDLE FileIOWaiter = CreateEvent( NULL, TRUE, FALSE, NULL);
	if( FileIOWaiter==NULL)
		goto fail;
	OVERLAPPED ol;
	ol.Offset = 0;
	ol.OffsetHigh = 0;
	ol.hEvent = FileIOWaiter;
	
	char* Msg = "Hello from WdmIo example\r\nChris Cant, PHD Computer Consultants Ltd\r\n";
	DWORD len = strlen(Msg);
	ResetEvent(FileIOWaiter);
	if( !WriteFile( hWdmIo, Msg, len, &BytesReturned, &ol))
	{
		if( GetLastError()!=ERROR_IO_PENDING)
		{
			printf("XXX  Could not write message %d\n",GetLastError());
			goto fail;
		}
	}
	// Queue second write
	if( !WriteFile( hWdmIo, Msg, len, &BytesReturned, &ol))
	{
		if( GetLastError()!=ERROR_IO_PENDING)
		{
			printf("XXX  Could not write message %d\n",GetLastError());
			goto fail;
		}
/*		printf("     Trying to cancel\n");
		if( !CancelIo(hWdmIo))
			printf("     Trying to cancel failed %d\n",GetLastError());
		printf("     Trying to cancel over\n");
*/
	}
	}
	/////////////////////////////////////////////////////////////////////////
	// Close device
fail:
/*	printf("\nTest %d\n",TestNo++);
	if( !CloseHandle(hWdmIo))
		printf("XXX  CloseHandle failed %d\n",GetLastError());
	else
		printf("     CloseHandle worked\n");

	/////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////
	char line[80];
	gets(line);
	gets(line);
*/
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
//	GetDeviceViaInterfaceOl:	Open DebugPrint device in overlapped mode

HANDLE GetDeviceViaInterfaceOl( GUID* pGuid, DWORD instance)
{
	// Get handle to relevant device information set
	HDEVINFO info = SetupDiGetClassDevs(pGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
	if(info==INVALID_HANDLE_VALUE)
		return NULL;

	// Get interface data for the requested instance
	SP_INTERFACE_DEVICE_DATA ifdata;
	ifdata.cbSize = sizeof(ifdata);
	if(!SetupDiEnumDeviceInterfaces(info, NULL, pGuid, instance, &ifdata))
	{
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

	// Open file
	HANDLE rv = CreateFile( ifDetail->DevicePath, 
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL);
	if( rv==INVALID_HANDLE_VALUE) rv = NULL;

	// Tidy up
	delete ifDetail;
	SetupDiDestroyDeviceInfoList(info);
	return rv;
}

/////////////////////////////////////////////////////////////////////////////

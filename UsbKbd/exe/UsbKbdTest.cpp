//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	UsbKbdTest example
/////////////////////////////////////////////////////////////////////////////
//	UsbKbdTest.cpp:	Win32 console application to exercise UsbKbd devices
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
#include "..\..\GUIDs.h"
#include "winioctl.h"
#include "..\sys\Ioctl.h"

HANDLE GetDeviceViaInterface( GUID* pGuid, DWORD instance);

int main(int argc, char* argv[])
{
	int TestNo = 1;

	printf("\nUsbKbdTest\n");

	/////////////////////////////////////////////////////////////////////////
	// Open device
	printf("\nTest %d\n",TestNo++);
	HANDLE hUsbKbd = GetDeviceViaInterface((LPGUID)&USBKBD_GUID,0);
	if( hUsbKbd==NULL)
	{
		printf("XXX  Could not find open UsbKbd device\n");
		return 1;
	}
	printf("     Opened OK\n");

	/////////////////////////////////////////////////////////////////////////
	// Get device descriptor
	printf("\nTest %d\n",TestNo++);
	DWORD BytesReturned;
	BYTE deviceDescriptor[50];
	if( !DeviceIoControl( hUsbKbd, IOCTL_USBKBD_GET_DEVICE_DESCRIPTOR,
							NULL, 0,					// Input
							deviceDescriptor, sizeof(deviceDescriptor),	// Output
							&BytesReturned, NULL))
		printf("XXX  Could not get device descriptor\n");
	else
	{
		printf("     Device descriptor is");
		for( DWORD i=0; i<BytesReturned; i++)
			printf(" %02X",deviceDescriptor[i]);
		printf("\n");
	}

	/////////////////////////////////////////////////////////////////////////
	// Get configuration descriptors
	printf("\nTest %d\n",TestNo++);
	BYTE descriptors[500];
	if( !DeviceIoControl( hUsbKbd, IOCTL_USBKBD_GET_CONFIGURATION_DESCRIPTORS,
							NULL, 0,					// Input
							descriptors, sizeof(descriptors),	// Output
							&BytesReturned, NULL))
		printf("XXX  Could not get configuration descriptors\n");
	else
	{
		printf("     Configuration descriptors are");
		for( DWORD i=0; i<BytesReturned; i++)
			printf(" %02X",descriptors[i]);
		printf("\n");
	}

	/////////////////////////////////////////////////////////////////////////
	// Get HID Report descriptor
	// Size of HID report is usually in second last byte of all above descriptors
	ULONG Size = descriptors[BytesReturned-2];
	if( Size<=sizeof(descriptors))
	{
		printf("\nTest %d\n",TestNo++);
		ULONG Info[2];
		Info[0] = 0x22;	// HID Report Descriptor type
		Info[1] = Size;
		if( !DeviceIoControl( hUsbKbd, IOCTL_USBKBD_GET_SPECIFIED_DESCRIPTOR,
								Info, sizeof(Info),					// Input
								descriptors, sizeof(descriptors),	// Output
								&BytesReturned, NULL))
			printf("XXX  Could not get HID Report descriptor\n");
		else
		{
			printf("     HID Report descriptor is");
			for( DWORD i=0; i<BytesReturned; i++)
				printf(" %02X",descriptors[i]);
			printf("\n");
		}
	}
	/////////////////////////////////////////////////////////////////////////
	// Set read timeout
	printf("\nTest %d\n",TestNo++);
	ULONG Timeout = 15;
	if( !DeviceIoControl( hUsbKbd, IOCTL_USBKBD_SET_READ_TIMEOUT,
							&Timeout, sizeof(Timeout),	// Input
							NULL, 0,					// Output
							&BytesReturned, NULL))
		printf("XXX  Could not set read timeout\n");
	else
		printf("     Read timeout set\n");

	/////////////////////////////////////////////////////////////////////////
	// Read
	printf("\nTest %d\n",TestNo++);
	DWORD TxdBytes;
	BYTE KbdReport[8];
	// Loop until Esc pressed on USB keyboard
	do
	{
		if( !ReadFile( hUsbKbd, KbdReport, sizeof(KbdReport), &TxdBytes, NULL))
		{
			printf("XXX  Could not read value %d\n", GetLastError());
			break;
		}
		else if( TxdBytes==sizeof(KbdReport))
		{
			printf("     Kbd report %2x %2x %2x %2x %2x %2x %2x %2x\n",
				KbdReport[0], KbdReport[1], KbdReport[2], KbdReport[3],
				KbdReport[4], KbdReport[5], KbdReport[6], KbdReport[7]);
		}
		else
		{
			printf("XXX  Wrong number of bytes read: %d\n",TxdBytes);
			break;
		}
	}
	while( KbdReport[2]!=0x29);

	/////////////////////////////////////////////////////////////////////////
	// Write data: cycle all keyboard LEDs

	printf("\nTest %d\n",TestNo++);
	BYTE bits[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xFF, 0x00, };

	for( int i=0; i<sizeof(bits)/sizeof(BYTE); i++)
	{
		if( !WriteFile( hUsbKbd, &bits[i], 1, &TxdBytes, NULL))
		{
			printf("XXX  Could not write value %d\n", GetLastError());
			break;
		}
		else if( TxdBytes==1)
		{
			printf("     Wrote %2x OK\n", bits[i]);
			Sleep(333);
		}
		else
		{
			printf("XXX  Wrong number of bytes written: %d\n",TxdBytes);
			break;
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// Get statuses
	printf("\nTest %d\n",TestNo++);
	WORD Statuses[3];
	if( !DeviceIoControl( hUsbKbd, IOCTL_USBKBD_GET_STATUSES,
							NULL, 0,					// Input
							Statuses, sizeof(Statuses),	// Output
							&BytesReturned, NULL))
		printf("XXX  Could not get statuses\n");
	else
		printf("     Statuses are %x %x %x\n", Statuses[0], Statuses[1], Statuses[2]);


	/////////////////////////////////////////////////////////////////////////
	// Get frame info
	printf("\nTest %d\n",TestNo++);
	ULONG Info[3];
	if( !DeviceIoControl( hUsbKbd, IOCTL_USBKBD_GET_FRAME_INFO,
							NULL, 0,			// Input
							Info, sizeof(Info),	// Output
							&BytesReturned, NULL))
		printf("XXX  Could not get frame info\n");
	else
		printf("     FrameLength %d bits.  FrameNumber %d.  FrameAlterNumber %d\n",
				Info[0], Info[1], Info[2]);

	
	/////////////////////////////////////////////////////////////////////////
	// Close device
	printf("\nTest %d\n",TestNo++);
	if( !CloseHandle(hUsbKbd))
		printf("XXX  CloseHandle failed %d\n",GetLastError());
	else
		printf("     CloseHandle worked\n");

	/////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////
	char line[80];
	gets(line);
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

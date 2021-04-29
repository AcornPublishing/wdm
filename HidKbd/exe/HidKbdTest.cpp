//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	HidKbdTest example
//////////////////////////////////////////////////////////////////////////////
//	HidKbdTest.cpp:	Win32 console application to exercise HidKbd devices
/////////////////////////////////////////////////////////////////////////////
//	main					Program main line
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "windows.h"
#include "stdio.h"

int main(int argc, char* argv[])
{
	int TestNo = 1;

	printf("\nHidKbdTest\n");

	/////////////////////////////////////////////////////////////////////////
	// Open device
	printf("\nTest %d\n",TestNo++);
	HANDLE hHidKbd = CreateFile("\\\\.\\HidKbd", GENERIC_READ|GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if( hHidKbd==INVALID_HANDLE_VALUE)
	{
		printf("XXX  Could not find open HidKbd device\n");
		return 1;
	}
	printf("     Opened OK\n");

	/////////////////////////////////////////////////////////////////////////
	// Read
	printf("\nTest %d\n",TestNo++);
	DWORD TxdBytes;
	BYTE KbdReport[9];
	if( !ReadFile( hHidKbd, KbdReport, sizeof(KbdReport), &TxdBytes, NULL))
	{
		printf("XXX  Could not read value %d\n", GetLastError());
	}
	else
	{
		printf("     Kbd report: %d bytes txd", TxdBytes);
		for( DWORD i=0; i<TxdBytes; i++)
			printf(" %02X", KbdReport[i]);
		printf("\n");
	}

	/////////////////////////////////////////////////////////////////////////
	// Close device
	printf("\nTest %d\n",TestNo++);
	if( !CloseHandle(hHidKbd))
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

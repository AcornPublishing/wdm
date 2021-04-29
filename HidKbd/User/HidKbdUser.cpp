//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	HidKbdUser example
/////////////////////////////////////////////////////////////////////////////
//	HidKbdUser.cpp:			Win32 console application to control HID keyboard
/////////////////////////////////////////////////////////////////////////////
//	main					Program main line
//	GetDeviceViaInterface	Open a handle via a device interface
//	ShowButtonCaps			Show button capabilities, ie possible usage values
//	GetCapabilities			Get HID device capabilities.  Return true if keyboard
//	DecodeInputUsages		Show usages and usages broken or made
//	SetLEDs					Write an output report to set LED state
//	WaitAndReturn			Wait for user input and return passed value
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
///////////////////////////////////////////////////////////////////////////////////

#include "windows.h"
#include "c:\98ddk\inc\win98\setupapi.h"	// VC++ 5 one is out of date
#include "stdio.h"
#include "winioctl.h"
#include "assert.h"

extern "C"
{
#include "c:\98ddk\src\hid\inc\hidsdi.h"
}

///////////////////////////////////////////////////////////////////////////////////

HANDLE GetDeviceViaInterface( GUID* pGuid, DWORD instance);
bool GetCapabilities( HANDLE hHidKbd, PHIDP_PREPARSED_DATA& HidPreparsedData,
					  USHORT& InputReportLen, USHORT& OutputReportLen);
void DecodeInputUsages( char* KbdReport, USHORT KbdReportLen, PHIDP_PREPARSED_DATA HidPreparsedData);
void SetLEDs(	HANDLE hHidKbd, USHORT OutputReportLen,
				PHIDP_PREPARSED_DATA HidPreparsedData,
				USAGE Usage1=0, USAGE Usage2=0, USAGE Usage3=0);
int WaitAndReturn(int rv);

///////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	int TestNo = 1;

	printf("\nHidKbdUser\n");

	/////////////////////////////////////////////////////////////////////////
	// Open device

	printf("\nTest %d\n",TestNo++);
	GUID HidGuid;
	HidD_GetHidGuid( &HidGuid);

	bool found = false;
	DWORD Instance = 0;
	HANDLE hHidKbd = NULL;
	PHIDP_PREPARSED_DATA HidPreparsedData;
	USHORT InputReportLen = 0, OutputReportLen = 0;
	while( !found)
	{
		hHidKbd = GetDeviceViaInterface( &HidGuid, Instance++);
		if( hHidKbd==NULL)
			break;
		printf("     Found HID device\n");

		if( GetCapabilities( hHidKbd, HidPreparsedData, InputReportLen, OutputReportLen))
		{
			found = true;
			break;
		}
		CloseHandle(hHidKbd);
	}
	if( !found || InputReportLen==0 || OutputReportLen==0)
	{
		printf("XXX  Could not find a HID keyboard\n");
		return WaitAndReturn(1);
	}
	printf("     Opened OK\n");

	/////////////////////////////////////////////////////////////////////////
	// Read input reports

	printf("\nTest %d\n",TestNo++);
	DWORD TxdBytes;
	char* InputReport = new char[InputReportLen];
	assert(InputReport!=NULL);
	memset( InputReport, 0, InputReportLen);
	// Loop until Esc pressed on keyboard
	do
	{
		if( !ReadFile( hHidKbd, InputReport, InputReportLen, &TxdBytes, NULL))
		{
			printf("XXX  Could not read value %d\n", GetLastError());
			break;
		}
		else if( TxdBytes==InputReportLen)
		{
			printf("     Input report %d:", InputReport[0]);
			for( USHORT i=1; i<InputReportLen; i++)
				printf(" %02X", InputReport[i]);
			printf("\n");

			DecodeInputUsages( InputReport, InputReportLen, HidPreparsedData);
		}
		else
		{
			printf("XXX  Wrong number of bytes read: %d\n",TxdBytes);
			break;
		}
	}
	while( InputReport[3]!=0x29);
	delete InputReport;

	
	/////////////////////////////////////////////////////////////////////////
	// Write data: cycle all keyboard LEDs

	printf("\nTest %d\n",TestNo++);

	SetLEDs( hHidKbd, OutputReportLen, HidPreparsedData);
	Sleep(333);
	SetLEDs( hHidKbd, OutputReportLen, HidPreparsedData,
		HID_USAGE_LED_SCROLL_LOCK);
	Sleep(333);
	SetLEDs( hHidKbd, OutputReportLen, HidPreparsedData,
		HID_USAGE_LED_CAPS_LOCK);
	Sleep(333);
	SetLEDs( hHidKbd, OutputReportLen, HidPreparsedData,
		HID_USAGE_LED_NUM_LOCK);
	Sleep(333);
	SetLEDs( hHidKbd, OutputReportLen, HidPreparsedData,
		HID_USAGE_LED_CAPS_LOCK, HID_USAGE_LED_SCROLL_LOCK);
	Sleep(333);
	SetLEDs( hHidKbd, OutputReportLen, HidPreparsedData,
		HID_USAGE_LED_NUM_LOCK, HID_USAGE_LED_SCROLL_LOCK);
	Sleep(333);
	SetLEDs( hHidKbd, OutputReportLen, HidPreparsedData,
		HID_USAGE_LED_NUM_LOCK, HID_USAGE_LED_CAPS_LOCK);
	Sleep(333);
	SetLEDs( hHidKbd, OutputReportLen, HidPreparsedData,
		HID_USAGE_LED_NUM_LOCK, HID_USAGE_LED_CAPS_LOCK, HID_USAGE_LED_SCROLL_LOCK);
	Sleep(333);
	SetLEDs( hHidKbd, OutputReportLen, HidPreparsedData);
		
	/////////////////////////////////////////////////////////////////////////

	HidD_FreePreparsedData( HidPreparsedData);

	/////////////////////////////////////////////////////////////////////////
	// Close device
	printf("\nTest %d\n",TestNo++);
	if( !CloseHandle(hHidKbd))
		printf("XXX  CloseHandle failed %d\n",GetLastError());
	else
		printf("     CloseHandle worked\n");

	/////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////
	
	return WaitAndReturn(0);
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
///////////////////////////////////////////////////////////////////////////////////
//	ShowButtonCaps:	Show button capabilities, ie possible usage values

void ShowButtonCaps( char* Msg, HIDP_REPORT_TYPE ReportType, USHORT NumCaps, PHIDP_PREPARSED_DATA HidPreparsedData)
{
	if( NumCaps==0) return;

	printf("     %s\n", Msg);

	HIDP_BUTTON_CAPS* ButtonCaps = new HIDP_BUTTON_CAPS[NumCaps];
	if( ButtonCaps==NULL) return;

	NTSTATUS status = HidP_GetButtonCaps( ReportType, ButtonCaps, &NumCaps, HidPreparsedData);
	if( status==HIDP_STATUS_SUCCESS)
	{
		for( USHORT i=0; i<NumCaps; i++)
		{
			printf("     ButtonCaps[%d].UsagePage %d\n", i, ButtonCaps[i].UsagePage);
			if( ButtonCaps[i].IsRange)
				printf("                  .Usages    %d..%d\n\n", ButtonCaps[i].Range.UsageMin, ButtonCaps[i].Range.UsageMax);
			else
				printf("                  .Usage     %d\n\n", ButtonCaps[i].NotRange.Usage);
		}
	}
	delete ButtonCaps;
}

///////////////////////////////////////////////////////////////////////////////////
//	GetCapabilities:	Get HID device capabilities.  Return true if keyboard
//						HidPreparsedData, InputReportLen and OutputReportLen returned
//						Don't forget to HidD_FreePreparsedData(HidPreparsedData)

bool GetCapabilities( HANDLE hHidKbd, PHIDP_PREPARSED_DATA& HidPreparsedData,
					  USHORT& InputReportLen, USHORT& OutputReportLen)
{
	// Get attributes. ie find vendor and product ids
	HIDD_ATTRIBUTES HidAttributes;
	if( !HidD_GetAttributes( hHidKbd, &HidAttributes))
	{
		printf("XXX  Could not get HID attributes\n");
		return false;
	}
	printf("     HID attributes: VendorID=%04X, ProductID=%04X, VersionNumber=%04X\n",
				HidAttributes.VendorID, HidAttributes.ProductID, HidAttributes.VersionNumber);

	// Get preparsed data
	if( !HidD_GetPreparsedData( hHidKbd, &HidPreparsedData))
	{
		printf("XXX  Could not get HID preparsed data\n");
		return false;
	}

	// Work out capabilities
	HIDP_CAPS HidCaps;
	bool found = false;
	NTSTATUS status = HidP_GetCaps( HidPreparsedData, &HidCaps);
	if( status==HIDP_STATUS_SUCCESS)
	{
		printf("     Top level Usage page %d usage %d\n", HidCaps.UsagePage, HidCaps.Usage);
		if( HidCaps.UsagePage==HID_USAGE_PAGE_GENERIC &&
			HidCaps.Usage==HID_USAGE_GENERIC_KEYBOARD)
		{
			printf("     Found HID keyboard\n\n");
			found = true;
		}
		// Remember max lengths of input and output reports
		InputReportLen = HidCaps.InputReportByteLength;
		OutputReportLen = HidCaps.OutputReportByteLength;
		printf("     InputReportByteLength %d\n", HidCaps.InputReportByteLength);
		printf("     OutputReportByteLength %d\n", HidCaps.OutputReportByteLength);
		printf("     FeatureReportByteLength %d\n\n", HidCaps.FeatureReportByteLength);

		printf("     NumberLinkCollectionNodes %d\n\n", HidCaps.NumberLinkCollectionNodes);

		printf("     NumberInputButtonCaps %d\n", HidCaps.NumberInputButtonCaps);
		printf("     NumberInputValueCaps %d\n", HidCaps.NumberInputValueCaps);
		printf("     NumberOutputButtonCaps %d\n", HidCaps.NumberOutputButtonCaps);
		printf("     NumberOutputValueCaps %d\n", HidCaps.NumberOutputValueCaps);
		printf("     NumberFeatureButtonCaps %d\n", HidCaps.NumberFeatureButtonCaps);
		printf("     NumberFeatureValueCaps %d\n\n", HidCaps.NumberFeatureValueCaps);
		
		ShowButtonCaps( "Input button capabilities", HidP_Input, HidCaps.NumberInputButtonCaps, HidPreparsedData);
		ShowButtonCaps( "Output button capabilities", HidP_Output, HidCaps.NumberOutputButtonCaps, HidPreparsedData);
	}
	return found;
}

///////////////////////////////////////////////////////////////////////////////////
//	DecodeInputUsages:	Show usages (and usages broken or made) in input report

const ULONG MaxPreviousUsages = 14;
USAGE_AND_PAGE Usages[MaxPreviousUsages];
USAGE PreviousUsages[MaxPreviousUsages];

void DecodeInputUsages( char* KbdReport, USHORT KbdReportLen,
					   PHIDP_PREPARSED_DATA HidPreparsedData)
{
	// Get max number of USAGE_AND_PAGEs required for all input reports in top-level collection
	ULONG MaxUsages = HidP_MaxUsageListLength( HidP_Input, 0, HidPreparsedData);
	if( MaxUsages==0 || MaxUsages>MaxPreviousUsages)
	{
		printf("XXX  Invalid HidP_MaxUsageListLength returned %d\n", MaxUsages);
		return;
	}
	
	// Get usages set in given keyboard report
	ULONG ValidUsages = MaxUsages;
	NTSTATUS status = HidP_GetButtonsEx( HidP_Input, 0, Usages, &ValidUsages, HidPreparsedData, KbdReport, KbdReportLen);
	if( status==HIDP_STATUS_SUCCESS)
	{
		USAGE CurrentUsages[MaxPreviousUsages];
		USAGE BreakUsages[MaxPreviousUsages];
		USAGE MakeUsages[MaxPreviousUsages];

		// Show current usages
		memset( CurrentUsages, 0, sizeof(CurrentUsages));
		printf("     Usages set: ");
		for( ULONG i=0; i<ValidUsages; i++)
		{
			printf( " %02X:%02X", Usages[i].UsagePage, Usages[i].Usage);
			CurrentUsages[i] = Usages[i].Usage;
		}

		// Work out differences compared to previous usages
		HidP_UsageListDifference( PreviousUsages, CurrentUsages, BreakUsages, MakeUsages, MaxUsages);

		// Print out usages broken and made
		printf(" (Break: ");
		for( i=0; i<MaxUsages; i++)
		{
			if( BreakUsages[i]==0) break;
			printf( " %02X", BreakUsages[i]);
		}
		printf(") (Make: ");
		for( i=0; i<MaxUsages; i++)
		{
			if( MakeUsages[i]==0) break;
			printf( " %02X", MakeUsages[i]);
		}
		printf(")\n\n");

		// Save previous usages
		memcpy( PreviousUsages, CurrentUsages, MaxUsages*sizeof(USAGE));
	}
}

///////////////////////////////////////////////////////////////////////////////////
//	SetLEDs:	Write an output report to set LED state

void SetLEDs(	HANDLE hHidKbd, USHORT OutputReportLen,
				PHIDP_PREPARSED_DATA HidPreparsedData,
				USAGE Usage1/*=0*/, USAGE Usage2/*=0*/, USAGE Usage3/*=0*/)
{
	// Build Output report from given usage(s)
	char* OutputReport = new char[OutputReportLen];
	assert(OutputReport!=NULL);
	memset( OutputReport, 0, OutputReportLen);

	USAGE UsageList[3];
	UsageList[0] = Usage1;
	UsageList[1] = Usage2;
	UsageList[2] = Usage3;
	ULONG UsageLength = 0;

	if( Usage1!=0)
	{
		UsageLength++;
		if( Usage2!=0)
		{
			UsageLength++;
			if( Usage3!=0)
			{
				UsageLength++;
			}
		}
	}

	// Convert usages into an output report
	NTSTATUS status = HidP_SetButtons( HidP_Output, HID_USAGE_PAGE_LED, 0,
			UsageList, &UsageLength, HidPreparsedData, OutputReport, OutputReportLen);
	if( status!=HIDP_STATUS_SUCCESS)
	{
		delete OutputReport;
		return;
	}

	printf("     Output report: ");
	for( ULONG i=1; i<OutputReportLen; i++)
		printf( " %02X", OutputReport[i]);
	printf("\n");

	// Send off output report
	DWORD TxdBytes;
	if( !WriteFile( hHidKbd, OutputReport, OutputReportLen, &TxdBytes, NULL))
		printf("XXX  Could not write value %d\n", GetLastError());
	else if( TxdBytes==OutputReportLen)
		printf("     Wrote output report OK\n");
	else
		printf("XXX  Wrong number of bytes written: %d\n",TxdBytes);
	delete OutputReport;
}

///////////////////////////////////////////////////////////////////////////////////
//	WaitAndReturn:	Wait for user input and return passed value

int WaitAndReturn(int rv)
{
	char line[80];
	gets(line);
	gets(line);
	return rv;
}

///////////////////////////////////////////////////////////////////////////////////

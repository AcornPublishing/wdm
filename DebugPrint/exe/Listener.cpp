//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//
//	DebugPrint Monitor
/////////////////////////////////////////////////////////////////////////////
//	Listener.cpp:	Worker thread which lists for DebugPrint events
//					Posts event message to view window
/////////////////////////////////////////////////////////////////////////////
//	GMTtoLocalTime			Convert from GMT to local time
//	ListenThreadFunction	Function that implements worker thread
//	StartListener			Called to start listening thread
//	StopListener			Called to ask listening thread to stop
//	GetDeviceViaInterfaceOl	Open overlapped device via its interface
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	21-Oct-98	1.0.0	CC	creation
//	20-Nov-98	1.01	CC	Starting to listen has O/S version appended
//	25-Nov-98	1.02	CC	Version added to starting msg
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "afxmt.h"
#include "c:\98ddk\inc\win98\setupapi.h"	// VC++ 5 one is out of date

#include "DebugPrint Monitor.h"

#include "initguid.h"
#include "..\DbgPrtGUID.h"

#include "Listener.h"

/////////////////////////////////////////////////////////////////////////////

HANDLE GetDeviceViaInterfaceOl( GUID* pGuid, DWORD instance);

/////////////////////////////////////////////////////////////////////////////
// Copied from WDM.H

typedef short CSHORT;

typedef struct _TIME_FIELDS {
    CSHORT Year;        // range [1601...]
    CSHORT Month;       // range [1..12]
    CSHORT Day;         // range [1..31]
    CSHORT Hour;        // range [0..23]
    CSHORT Minute;      // range [0..59]
    CSHORT Second;      // range [0..59]
    CSHORT Milliseconds;// range [0..999]
    CSHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS *PTIME_FIELDS;

/////////////////////////////////////////////////////////////////////////////
//	Declare static member of DebugPrint_Event class

HWND DebugPrint_Event::ViewHwnd = NULL;

/////////////////////////////////////////////////////////////////////////////
//	Information passed to ListenThreadFunction

typedef struct _LISTENER_INFO
{
	HANDLE DebugPrintDriver;
	bool KeepGoing;
} LISTENER_INFO, *PLISTENER_INFO;

LISTENER_INFO ListenerInfo;

/////////////////////////////////////////////////////////////////////////////
//	GMTtoLocalTime:	Convert from GMT to local time
//					Can't find an easy function to do this

CTime GMTtoLocalTime( CTime CTgmt)
{
	static boolean NotGotDiff = true;
	static time_t diff = 0;
	time_t gmt = CTgmt.GetTime();
	if( NotGotDiff)
	{
		// Get GMT to Local time difference by looking at current time
		time_t Now = time(NULL);
		struct tm* TM = gmtime(&Now);
		CTime gmtNow(TM->tm_year+1900,TM->tm_mon+1,TM->tm_mday,TM->tm_hour,TM->tm_min,TM->tm_sec);
		diff = gmtNow.GetTime()-Now;
		NotGotDiff = false;
	}
	return CTime(gmt-diff);
}

/////////////////////////////////////////////////////////////////////////////
//	ListenThreadFunction:	Function that listens for events

UINT ListenThreadFunction( LPVOID pParam)
{
	PLISTENER_INFO pListenerInfo = (PLISTENER_INFO)pParam;
	if (pListenerInfo==NULL) return -1;

	CString StartMsg = "Starting to listen";
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if( GetVersionEx(&osvi))
	{
		switch( osvi.dwPlatformId)
		{
		case VER_PLATFORM_WIN32_WINDOWS:
			StartMsg.Format("Version %s starting to listen under Windows 9%c (%d.%d build %d) %s",
				Version,
				osvi.dwMinorVersion==0?'5':'8',
				osvi.dwMajorVersion, osvi.dwMinorVersion, LOWORD(osvi.dwBuildNumber), osvi.szCSDVersion);
			break;
		case VER_PLATFORM_WIN32_NT:
			StartMsg.Format("Version %s starting to listen under Windows 2000 (%d.%d build %d) %s",
				Version,
				osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, osvi.szCSDVersion);
			break;
		}
	}
	DebugPrint_Event::SendEvent( "Monitor", StartMsg, CTime::GetCurrentTime(), 0, false);

	// Buffer for events
	const int MAX_EVENT_LEN = 1024;
	char Event[MAX_EVENT_LEN+1];

	// Create Overlapped read structure and event
	HANDLE FileIOWaiter = CreateEvent( NULL, TRUE, FALSE, NULL);
	if( FileIOWaiter==NULL)
		goto Exit2;
	OVERLAPPED ol;
	ol.Offset = 0;
	ol.OffsetHigh = 0;
	ol.hEvent = FileIOWaiter;

	// Keep looping, waiting for events, until KeepGoing goes false
	for(;;)
	{
		// Initiate overlapped read
		DWORD TxdBytes;
		ResetEvent(FileIOWaiter);
		memset(Event,0,MAX_EVENT_LEN+1);
		if( !ReadFile( ListenerInfo.DebugPrintDriver, Event, MAX_EVENT_LEN, &TxdBytes, &ol))
		{
			// Check for read errors
			if( GetLastError()!=ERROR_IO_PENDING)
			{
				CString Msg;
				Msg.Format("Read didn't return pending %d", GetLastError());
				DebugPrint_Event::SendEvent( "Monitor", Msg);
				goto Exit;
			}

//			DebugPrint_Event::SendEvent( "Monitor", "Waiting for read completion");

			// Wait for read to complete (check for KeepGoing going false every 100ms)
			while( WaitForSingleObject( FileIOWaiter, 100)==WAIT_TIMEOUT)
			{
				if( !ListenerInfo.KeepGoing)
				{
					// Cancel the pending read
					CancelIo(ListenerInfo.DebugPrintDriver);
					goto Exit;
				}
			}

			// Get read result, ie bytes transferred
			if( !GetOverlappedResult( ListenerInfo.DebugPrintDriver, &ol, &TxdBytes, FALSE))
			{
				CString Msg;
				Msg.Format("GetOverlappedResult failed %d (ol.Internal=%X)",
								GetLastError(),ol.Internal);
				DebugPrint_Event::SendEvent( "Monitor", Msg);
				continue;
			}
		}

		// Check there's something there
		if( TxdBytes < sizeof(TIME_FIELDS)+2)
		{
			DebugPrint_Event::SendEvent( "Monitor", "Short read msg" );
			continue;
		}
		// Extract Timestamp, Driver and Msg, and post to View
		Event[MAX_EVENT_LEN] = '\0';
		PTIME_FIELDS pTF = (PTIME_FIELDS)Event;
		CTime gmtEventTime(pTF->Year,pTF->Month,pTF->Day,pTF->Hour,pTF->Minute,pTF->Second);
		CTime EventTime = GMTtoLocalTime(gmtEventTime);
		char* DriverName = Event+sizeof(TIME_FIELDS);
		CString CSDriverName = DriverName;
		CString CSDriverMsg = Event+sizeof(TIME_FIELDS)+strlen(DriverName)+1;
		DebugPrint_Event::SendEvent( CSDriverName, CSDriverMsg, EventTime, pTF->Milliseconds);
	}

Exit:
	CloseHandle(FileIOWaiter);
Exit2:
	CloseHandle(ListenerInfo.DebugPrintDriver);
	ListenerInfo.DebugPrintDriver = NULL;
	DebugPrint_Event::SendEvent( "Monitor", "Stopped listening");
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
//	StartListener:	Called by App to start listener thread

void StartListener(CListView* pView)
{
	DebugPrint_Event::ViewHwnd = pView->GetSafeHwnd();

	// Try to open first DebugPrint driver device
	ListenerInfo.DebugPrintDriver = GetDeviceViaInterfaceOl((LPGUID)&DEBUGPRINT_GUID,0);
	if( ListenerInfo.DebugPrintDriver==NULL)
	{
		DebugPrint_Event::SendEvent( "Monitor", "Could not open DebugPrint driver" );
		return;
	}
	// Start worker thread
	ListenerInfo.KeepGoing = true;
	AfxBeginThread(ListenThreadFunction, &ListenerInfo);
}

/////////////////////////////////////////////////////////////////////////////
//	StopListener:	Called by App to tell listener thread to stop

void StopListener()
{
	if( ListenerInfo.DebugPrintDriver==NULL)
		return;

	DebugPrint_Event::SendEvent( "Monitor", "About to end listen" );
	ListenerInfo.KeepGoing = false;
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


//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//
//	DebugPrint Monitor
/////////////////////////////////////////////////////////////////////////////
//	Listener.h		Listening thread and Event header
/////////////////////////////////////////////////////////////////////////////
//	WM_DEBUGPRINTEVENT	Event message id
//	DebugPrint_Event
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//	Message id for sending event messages to view

const UINT WM_DEBUGPRINTEVENT = (WM_USER+1);

/////////////////////////////////////////////////////////////////////////////
//	DebugPrint_Event class encapsulates events sent from Listener to View

class DebugPrint_Event
{
public:
	CString Driver;
	CTime Timestamp;
	int Milliseconds;
	CString Message;
	bool SetModified;	// false to reset document SetModifiedFlag.

	static HWND ViewHwnd;	// View Hwnd

	// Generate and send an event
	static void SendEvent( CString d, CString m, CTime t = 0, int milliseconds = 0, bool sm=true)
	{
		if( ViewHwnd==NULL) return;
		DebugPrint_Event* pEvent = new DebugPrint_Event;
		pEvent->Driver = d;
		if( t==0) t = CTime::GetCurrentTime();
		pEvent->Timestamp = t;
		pEvent->Milliseconds = milliseconds;
		pEvent->Message = m;
		pEvent->SetModified = sm;
		::PostMessage( ViewHwnd, WM_DEBUGPRINTEVENT, 0, (LPARAM)pEvent);
	}
};

/////////////////////////////////////////////////////////////////////////////

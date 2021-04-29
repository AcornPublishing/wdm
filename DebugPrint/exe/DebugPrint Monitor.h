//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//
//	DebugPrint Monitor
/////////////////////////////////////////////////////////////////////////////
//	DebugPrint Monitor.h:		MFC app header
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	20-Oct-98	1.0.0	CC	creation
//	25-Nov-98	1.02	CC	Version added
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEBUGPRINTMONITOR_H__074AE6A6_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_)
#define AFX_DEBUGPRINTMONITOR_H__074AE6A6_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////

#define Version "1.03"

/////////////////////////////////////////////////////////////////////////////
//	Listener thread globals

void StartListener(CListView* pView);
void StopListener();

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorApp:

class CDebugPrintMonitorApp : public CWinApp
{
public:
	CDebugPrintMonitorApp();

	// Registry names
	static CString RegKey;
	static CString KeyBase;
	static CString KeySettings;
	static CString SettingsShowWindow;
	static CString SettingsWindowPosition;

	// Registry settings
	int m_ShowWindow;
	RECT m_rcNormalPosition;
	CString m_PrintFace;
	int m_PrintPointSize;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDebugPrintMonitorApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDebugPrintMonitorApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void LoadSettings();
	void SaveSettings();
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBUGPRINTMONITOR_H__074AE6A6_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_)

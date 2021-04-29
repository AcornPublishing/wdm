// Wdm2Notify.h : main header file for the WDM2NOTIFY application
//

#if !defined(AFX_WDM2NOTIFY_H__5A5B3EF7_8454_11D2_AE71_00C0DFE4C1F3__INCLUDED_)
#define AFX_WDM2NOTIFY_H__5A5B3EF7_8454_11D2_AE71_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWdm2NotifyApp:
// See Wdm2Notify.cpp for the implementation of this class
//

class CWdm2NotifyApp : public CWinApp
{
public:
	CWdm2NotifyApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWdm2NotifyApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWdm2NotifyApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WDM2NOTIFY_H__5A5B3EF7_8454_11D2_AE71_00C0DFE4C1F3__INCLUDED_)

//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm2Power
//////////////////////////////////////////////////////////////////////////////
//	Wdm2Power.h
//////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WDM2POWER_H__8B67F777_8BB1_11D2_AE94_00C0DFE4C1F3__INCLUDED_)
#define AFX_WDM2POWER_H__8B67F777_8BB1_11D2_AE94_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWdm2PowerApp:
// See Wdm2Power.cpp for the implementation of this class
//

class CWdm2PowerApp : public CWinApp
{
public:
	CWdm2PowerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWdm2PowerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWdm2PowerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WDM2POWER_H__8B67F777_8BB1_11D2_AE94_00C0DFE4C1F3__INCLUDED_)

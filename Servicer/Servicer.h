//////////////////////////////////////////////////////////////////////////////
//	Copyright ? 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Servicer
/////////////////////////////////////////////////////////////////////////////
//	Servicer.h
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVICER_H__B2AC6D86_5928_11D2_AE08_00C0DFE4C1F3__INCLUDED_)
#define AFX_SERVICER_H__B2AC6D86_5928_11D2_AE08_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CServicerApp:
// See Servicer.cpp for the implementation of this class
//

class CServicerApp : public CWinApp
{
public:
	CServicerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServicerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CServicerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVICER_H__B2AC6D86_5928_11D2_AE08_00C0DFE4C1F3__INCLUDED_)

//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//
//	DebugPrint Monitor
/////////////////////////////////////////////////////////////////////////////
//	MainFrm.h:		MFC Main Frame Window header
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__074AE6AA_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_)
#define AFX_MAINFRM_H__074AE6AA_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMainFrame : public CFrameWnd
{
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL DestroyWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__074AE6AA_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_)

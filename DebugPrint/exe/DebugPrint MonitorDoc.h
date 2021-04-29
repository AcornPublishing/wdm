//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//
//	DebugPrint Monitor
/////////////////////////////////////////////////////////////////////////////
//	DebugPrint MonitorDoc.h:	Document header
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEBUGPRINTMONITORDOC_H__074AE6AC_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_)
#define AFX_DEBUGPRINTMONITORDOC_H__074AE6AC_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CDebugPrintMonitorDoc : public CDocument
{
protected: // create from serialization only
	CDebugPrintMonitorDoc();
	DECLARE_DYNCREATE(CDebugPrintMonitorDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDebugPrintMonitorDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDebugPrintMonitorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CDebugPrintMonitorDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBUGPRINTMONITORDOC_H__074AE6AC_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_)

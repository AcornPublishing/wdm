//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//
//	DebugPrint Monitor
/////////////////////////////////////////////////////////////////////////////
//	DebugPrint MonitorView.h:	View header
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEBUGPRINTMONITORVIEW_H__074AE6AE_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_)
#define AFX_DEBUGPRINTMONITORVIEW_H__074AE6AE_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CDebugPrintMonitorView : public CListView
{
protected: // create from serialization only
	CDebugPrintMonitorView();
	DECLARE_DYNCREATE(CDebugPrintMonitorView)

// Attributes
public:
	CDebugPrintMonitorDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDebugPrintMonitorView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDebugPrintMonitorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CDebugPrintMonitorView)
	afx_msg void OnEditClearEvents();
	afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnTrack(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndtrack(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnViewShowMilliseconds();
	afx_msg void OnUpdateViewShowMilliseconds(CCmdUI* pCmdUI);
	afx_msg void OnViewShowDate();
	afx_msg void OnUpdateViewShowDate(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg LONG OnDebugPrintEvent( UINT, LONG);
	DECLARE_MESSAGE_MAP()

private:
	bool m_ShowMilliseconds;
	bool m_ShowDate;
	bool m_EventListInited;
	int DriverColWidth;
	int TimestampColWidth;
	void ResizeColumnWidths();
	void StoreColWidths();
};

#ifndef _DEBUG  // debug version in DebugPrint MonitorView.cpp
inline CDebugPrintMonitorDoc* CDebugPrintMonitorView::GetDocument()
   { return (CDebugPrintMonitorDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBUGPRINTMONITORVIEW_H__074AE6AE_636D_11D2_B677_00C0DFE4C1F3__INCLUDED_)

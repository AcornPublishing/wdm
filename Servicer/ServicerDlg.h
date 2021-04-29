//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Servicer
/////////////////////////////////////////////////////////////////////////////
//	ServicerDlg.h
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVICERDLG_H__B2AC6D88_5928_11D2_AE08_00C0DFE4C1F3__INCLUDED_)
#define AFX_SERVICERDLG_H__B2AC6D88_5928_11D2_AE08_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CServicerDlg dialog

class CServicerDlg : public CDialog
{
// Construction
public:
	CServicerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CServicerDlg)
	enum { IDD = IDD_SERVICER_DIALOG };
	CString	m_Driver;
	CString	m_Status;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServicerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CServicerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLookup();
	afx_msg void OnStart();
	afx_msg void OnStop();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	SC_HANDLE m_hSCManager;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVICERDLG_H__B2AC6D88_5928_11D2_AE08_00C0DFE4C1F3__INCLUDED_)

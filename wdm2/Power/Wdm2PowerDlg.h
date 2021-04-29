//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm2Power
//////////////////////////////////////////////////////////////////////////////
//	Wdm2PowerDlg.h
//////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WDM2POWERDLG_H__8B67F779_8BB1_11D2_AE94_00C0DFE4C1F3__INCLUDED_)
#define AFX_WDM2POWERDLG_H__8B67F779_8BB1_11D2_AE94_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CWdm2PowerDlg dialog

class CWdm2PowerDlg : public CDialog
{
// Construction
public:
	CWdm2PowerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CWdm2PowerDlg)
	enum { IDD = IDD_WDM2POWER_DIALOG };
	CListBox	m_EventList;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWdm2PowerDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CWdm2PowerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSuspend();
	afx_msg void OnHibernate();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	afx_msg LRESULT OnPowerBroadcast(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
private:
	bool m_ImNT;
	HANDLE m_hFile;
	void SuspendOrHibernate( BOOLEAN Suspend, CString Opname);
	void PowerEvent(CString Msg);
	void GetPowerStatus();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WDM2POWERDLG_H__8B67F779_8BB1_11D2_AE94_00C0DFE4C1F3__INCLUDED_)

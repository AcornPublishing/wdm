// Wdm2NotifyDlg.h : header file
//

#if !defined(AFX_WDM2NOTIFYDLG_H__5A5B3EF9_8454_11D2_AE71_00C0DFE4C1F3__INCLUDED_)
#define AFX_WDM2NOTIFYDLG_H__5A5B3EF9_8454_11D2_AE71_00C0DFE4C1F3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CWdm2NotifyDlg dialog

class CWdm2NotifyDlg : public CDialog
{
// Construction
public:
	CWdm2NotifyDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CWdm2NotifyDlg)
	enum { IDD = IDD_WDM2NOTIFY_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWdm2NotifyDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CWdm2NotifyDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	afx_msg BOOL OnDeviceChange( UINT nEventType, DWORD dwData );
	DECLARE_MESSAGE_MAP()

private:
	HDEVNOTIFY WdmNotificationHandle;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WDM2NOTIFYDLG_H__5A5B3EF9_8454_11D2_AE71_00C0DFE4C1F3__INCLUDED_)

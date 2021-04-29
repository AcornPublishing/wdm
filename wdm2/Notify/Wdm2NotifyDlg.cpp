// Wdm2NotifyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Wdm2Notify.h"
#include "Wdm2NotifyDlg.h"

#include "objbase.h"
#include "initguid.h"
#include "..\..\GUIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWdm2NotifyDlg dialog

CWdm2NotifyDlg::CWdm2NotifyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWdm2NotifyDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWdm2NotifyDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	WdmNotificationHandle = NULL;
}

void CWdm2NotifyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWdm2NotifyDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWdm2NotifyDlg, CDialog)
	//{{AFX_MSG_MAP(CWdm2NotifyDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DEVICECHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWdm2NotifyDlg message handlers

BOOL CWdm2NotifyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	// Register for Wdm2 device interface changes
	DEV_BROADCAST_DEVICEINTERFACE dbch;
	dbch.dbcc_size = sizeof(dbch);
	dbch.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	dbch.dbcc_classguid = WDM2_GUID;
	dbch.dbcc_name[0] = '\0';
	WdmNotificationHandle = RegisterDeviceNotification( GetSafeHwnd(), &dbch, DEVICE_NOTIFY_WINDOW_HANDLE);
	if( WdmNotificationHandle==NULL)
		GetDlgItem(IDC_STATUS)->SetWindowText("Cannot register for Wdm2 class device notification");

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWdm2NotifyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWdm2NotifyDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWdm2NotifyDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CWdm2NotifyDlg::DestroyWindow() 
{
	if( WdmNotificationHandle!=NULL)
	{
		UnregisterDeviceNotification(WdmNotificationHandle);
		WdmNotificationHandle = NULL;
	}
	return CDialog::DestroyWindow();
}

BOOL CWdm2NotifyDlg::OnDeviceChange( UINT nEventType, DWORD dwData )
{
	CString Msg = "duh";
	switch( nEventType)
	{
	case DBT_CONFIGCHANGECANCELED:
		Msg.Format("DBT_CONFIGCHANGECANCELED");
		break;
	case DBT_CONFIGCHANGED:
		Msg.Format("DBT_CONFIGCHANGED");
		break;
	case DBT_CUSTOMEVENT:
		Msg.Format("DBT_CUSTOMEVENT");
		break;
	case DBT_DEVICEARRIVAL:
		Msg.Format("DBT_DEVICEARRIVAL");
		break;
	case DBT_DEVICEQUERYREMOVE:
		Msg.Format("DBT_DEVICEQUERYREMOVE");
		break;
	case DBT_DEVICEQUERYREMOVEFAILED:
		Msg.Format("DBT_DEVICEQUERYREMOVEFAILED");
		break;
	case DBT_DEVICEREMOVEPENDING:
		Msg.Format("DBT_DEVICEREMOVEPENDING");
		break;
	case DBT_DEVICEREMOVECOMPLETE:
		Msg.Format("DBT_DEVICEREMOVECOMPLETE");
		break;
	case DBT_DEVICETYPESPECIFIC:
		Msg.Format("DBT_DEVICETYPESPECIFIC");
		break;
	case DBT_QUERYCHANGECONFIG:
		Msg.Format("DBT_QUERYCHANGECONFIG");
		break;
	case DBT_DEVNODES_CHANGED:
		Msg.Format("DBT_DEVNODES_CHANGED");
		break;
	case DBT_USERDEFINED:
		Msg.Format("DBT_USERDEFINED");
		break;
	default:
		Msg.Format("Event type %d",nEventType);
	}

	PDEV_BROADCAST_DEVICEINTERFACE pdbch = (PDEV_BROADCAST_DEVICEINTERFACE)dwData;
	if( pdbch!=NULL && pdbch->dbcc_devicetype==DBT_DEVTYP_DEVICEINTERFACE)
	{
		CString Msg2;
		Msg2.Format("%s: %s",Msg,pdbch->dbcc_name);
		Msg = Msg2;
	}

	CListBox* EventList = (CListBox*)GetDlgItem(IDC_EVENT_LIST);
	EventList->AddString(Msg);
	return TRUE;	// or BROADCAST_QUERY_DENY to deny a query remove
}

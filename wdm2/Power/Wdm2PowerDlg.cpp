//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm2Power
/////////////////////////////////////////////////////////////////////////////
//	Wdm2PowerDlg.cpp
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Wdm2Power.h"
#include "Wdm2PowerDlg.h"

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
// CWdm2PowerDlg dialog

CWdm2PowerDlg::CWdm2PowerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWdm2PowerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWdm2PowerDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWdm2PowerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWdm2PowerDlg)
	DDX_Control(pDX, IDC_EVENT_LIST, m_EventList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWdm2PowerDlg, CDialog)
	//{{AFX_MSG_MAP(CWdm2PowerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SUSPEND, OnSuspend)
	ON_BN_CLICKED(IDC_HIBERNATE, OnHibernate)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_MESSAGE( WM_POWERBROADCAST, OnPowerBroadcast)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWdm2PowerDlg message handlers

BOOL CWdm2PowerDlg::OnInitDialog()
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
	// Get OS type
	m_ImNT = false;
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if( GetVersionEx(&osvi))
		m_ImNT = osvi.dwPlatformId==VER_PLATFORM_WIN32_NT;
	if( m_ImNT)
		GetDlgItem(IDC_HIBERNATE)->EnableWindow();

	// Start 1 second timer
	SetTimer(1,1000,NULL);

	// Open file handle
	m_hFile = NULL;
	char ModuleFileName[_MAX_PATH];
	if( GetModuleFileName( NULL, ModuleFileName, _MAX_PATH))
		m_hFile = CreateFile( ModuleFileName, GENERIC_READ, FILE_SHARE_READ, NULL, 0, FILE_ATTRIBUTE_NORMAL, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWdm2PowerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CWdm2PowerDlg::OnPaint() 
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
HCURSOR CWdm2PowerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////////////////////////

void CWdm2PowerDlg::OnSuspend() 
{
	SuspendOrHibernate(TRUE, "Suspended");	
}

void CWdm2PowerDlg::OnHibernate() 
{
	SuspendOrHibernate(FALSE, "Hibernated");	
}

void CWdm2PowerDlg::SuspendOrHibernate( BOOLEAN Suspend, CString Opname) 
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tp;
	if( m_ImNT)
	{
		if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken))
		{
			CString Msg;
			Msg.Format("Could not open process token %d",GetLastError());
			PowerEvent(Msg);
			return;
		}
		tp.PrivilegeCount = 1;
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid); 
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if( !AdjustTokenPrivileges( hToken, FALSE, &tp, 0, (PTOKEN_PRIVILEGES)NULL, 0)) 
		{
			CString Msg;
			Msg.Format("Could not AdjustTokenPrivileges %d",GetLastError());
			PowerEvent(Msg);
			CloseHandle(hToken);
			return;
		}
	}
	// Ask to suspend the system
	if( SetSystemPowerState( TRUE, FALSE))
		PowerEvent(Opname+" and resumed OK");
	else
	{
		CString Msg;
		Msg.Format("Could not suspend %d",GetLastError());
		PowerEvent(Msg);
	}
	if( m_ImNT)
	{
		// Disable shutdown priv
		tp.Privileges[0].Attributes = 0;
		AdjustTokenPrivileges( hToken, FALSE, &tp, 0,  (PTOKEN_PRIVILEGES)NULL, 0);
		CloseHandle(hToken);
	}
}

/////////////////////////////////////////////////////////////////////////////

LRESULT CWdm2PowerDlg::OnPowerBroadcast( WPARAM wParam, LPARAM lParam)
{
	DWORD dwPowerEvent = (DWORD)wParam;
	DWORD dwData = (DWORD)lParam;
	CString Msg;
	switch( dwPowerEvent)
	{
	case PBT_APMBATTERYLOW:
		PowerEvent("Battery power is low"); break;
	case PBT_APMOEMEVENT:
		PowerEvent("OEM-defined event occurred"); break;
	case PBT_APMPOWERSTATUSCHANGE:
		PowerEvent("Power status has changed");
		GetPowerStatus();
		break;
	case PBT_APMQUERYSUSPEND:
		PowerEvent("Request for permission to suspend"); break;
	case PBT_APMQUERYSUSPENDFAILED:
		PowerEvent("Suspension request denied"); break;
	case PBT_APMRESUMEAUTOMATIC:
		PowerEvent("Operation resuming automatically after event"); break;
	case PBT_APMRESUMECRITICAL:
		PowerEvent("Operation resuming after critical suspension"); break;
	case PBT_APMRESUMESUSPEND:
		PowerEvent("Operation resuming after suspension"); break;
	case PBT_APMSUSPEND:
		PowerEvent("System is suspending operation"); break;
	default:
		Msg.Format("dwPowerEvent %d",dwPowerEvent);
		PowerEvent(Msg);
	}
	return TRUE;	// Grant request	// or BROADCAST_QUERY_DENY to refuse
}

void CWdm2PowerDlg::PowerEvent(CString Msg)
{
	m_EventList.SetCurSel(m_EventList.AddString(Msg));
}

void CWdm2PowerDlg::OnTimer(UINT nIDEvent) 
{
	GetPowerStatus();
	CDialog::OnTimer(nIDEvent);
}

void CWdm2PowerDlg::GetPowerStatus()
{
	SYSTEM_POWER_STATUS sps;
	if( !GetSystemPowerStatus(&sps))
		return;
	GetDlgItem(IDC_ACPOWER)->SetWindowText(	sps.ACLineStatus==0?"Offline":
											sps.ACLineStatus==1?"Online":"Unknown");
	CString BattFlags;
	if( sps.BatteryFlag==255)
		BattFlags = "Unknown";
	else
	{
		if( sps.BatteryFlag&1) BattFlags += "High ";
		if( sps.BatteryFlag&2) BattFlags += "Low ";
		if( sps.BatteryFlag&4) BattFlags += "Critical ";
		if( sps.BatteryFlag&8) BattFlags += "Charging ";
		if( sps.BatteryFlag&128) BattFlags += "No system battery";
	}
	GetDlgItem(IDC_BATT_CHARGE)->SetWindowText(BattFlags);
	CString Msg;
	if( sps.BatteryLifeTime==DWORD(-1))
		Msg = "Unknown";
	else
		Msg.Format("%d seconds",sps.BatteryLifeTime);
	GetDlgItem(IDC_BATT_LIFE)->SetWindowText(Msg);
	if( sps.BatteryFullLifeTime==DWORD(-1))
		Msg = "Unknown";
	else
		Msg.Format("%d seconds",sps.BatteryFullLifeTime);
	GetDlgItem(IDC_BATT_FULLLIFE)->SetWindowText(Msg);

	if( m_hFile!=NULL)
	{
		Msg.Empty();
		BOOL fon;
		if( GetDevicePowerState( m_hFile, &fon))
			Msg = (fon?"Spun up":"Spun down");
		GetDlgItem(IDC_THIS_DISK)->SetWindowText(Msg);
	}
}

/////////////////////////////////////////////////////////////////////////////

BOOL CWdm2PowerDlg::DestroyWindow() 
{
	if( m_hFile!=NULL)
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
	
	return CDialog::DestroyWindow();
}

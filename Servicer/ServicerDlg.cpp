//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Servicer
/////////////////////////////////////////////////////////////////////////////
//	ServicerDlg.cpp
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "winsvc.h"
#include "Servicer.h"
#include "ServicerDlg.h"

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
// CServicerDlg dialog

CServicerDlg::CServicerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServicerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServicerDlg)
	m_Driver = _T("");
	m_Status = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServicerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServicerDlg)
	DDX_Text(pDX, IDC_DRIVER, m_Driver);
	DDX_Text(pDX, IDC_STATUS, m_Status);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CServicerDlg, CDialog)
	//{{AFX_MSG_MAP(CServicerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOOKUP, OnLookup)
	ON_BN_CLICKED(IDC_START, OnStart)
	ON_BN_CLICKED(IDC_STOP, OnStop)
	ON_BN_CLICKED(IDOK, OnClose)
	ON_BN_CLICKED(IDCANCEL, OnClose)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServicerDlg message handlers

BOOL CServicerDlg::OnInitDialog()
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

	
	m_hSCManager = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if( m_hSCManager==NULL)
	{
		m_Status = "Could not open Service Control Manager";
		UpdateData(FALSE);
	}





	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CServicerDlg::OnClose() 
{
	CDialog::OnOK();
	if( m_hSCManager!=NULL)
		CloseServiceHandle(m_hSCManager);
	m_hSCManager = NULL;
}

void CServicerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CServicerDlg::OnPaint() 
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
HCURSOR CServicerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CServicerDlg::OnLookup() 
{
	if( m_hSCManager==NULL) return;

	UpdateData();

	/////////////////////////////////////////////////////////////////////////

	SC_HANDLE hDriver = OpenService(m_hSCManager,m_Driver,SERVICE_ALL_ACCESS);
	if( hDriver==NULL)
	{
		m_Status = "Could not open Service "+m_Driver;
		UpdateData(FALSE);
		return;
	}

	SERVICE_STATUS ss;
	if( ControlService(hDriver,SERVICE_CONTROL_INTERROGATE,&ss))
	{
		switch( ss.dwCurrentState)
		{
		case SERVICE_STOPPED:
			m_Status = "Stopped";	break;
		case SERVICE_START_PENDING:
			m_Status = "Start pending";	break;
		case SERVICE_STOP_PENDING:
			m_Status = "Stop pending";	break;
		case SERVICE_RUNNING:
			m_Status = "Running";	break;
		case SERVICE_CONTINUE_PENDING:
			m_Status = "Continue pending";	break;
		case SERVICE_PAUSE_PENDING:
			m_Status = "Pause pending";	break;
		case SERVICE_PAUSED:
			m_Status = "Paused";	break;
		default:
			m_Status = "Unknown";	break;
		}
	}
	UpdateData(FALSE);
	CloseServiceHandle(hDriver);
}

void CServicerDlg::OnStart() 
{
	if( m_hSCManager==NULL) return;

	UpdateData();

	/////////////////////////////////////////////////////////////////////////

	SC_HANDLE hDriver = OpenService(m_hSCManager,m_Driver,SERVICE_ALL_ACCESS);
	if( hDriver==NULL)
	{
		_TCHAR System32Directory[_MAX_PATH];
		if( 0==GetSystemDirectory(System32Directory,_MAX_PATH))
		{
			m_Status = "Could not find Windows system directory";
			return;
		}
		CString Drivers = System32Directory;
		Drivers += "\\Drivers\\"+m_Driver+".sys";

		hDriver = CreateService(m_hSCManager,m_Driver,m_Driver,SERVICE_ALL_ACCESS,
								SERVICE_KERNEL_DRIVER,SERVICE_AUTO_START,SERVICE_ERROR_NORMAL,
								Drivers,NULL,NULL,NULL,NULL,NULL);
		if( hDriver==NULL)
		{
			m_Status = "Could not create Service "+m_Driver;
			goto StartExit;
		}
	}

	SERVICE_STATUS ss;
	ControlService(hDriver,SERVICE_CONTROL_INTERROGATE,&ss);
	if( ss.dwCurrentState!=SERVICE_RUNNING)
	{
		if( !StartService(hDriver,0,NULL))
		{
			m_Status.Format("Could not start %s %d", m_Driver, GetLastError());
			goto StartExit;
		}
		m_Status = "Starting";
		UpdateData(FALSE);
		GetDlgItem(IDC_STATUS)->UpdateWindow();
		// Give it 10 seconds to start
		BOOL Done = FALSE;
		for(int seconds=0;seconds<10;seconds++)
		{
			Sleep(1000);
			if( ControlService(hDriver,SERVICE_CONTROL_INTERROGATE,&ss) &&
				ss.dwCurrentState==SERVICE_RUNNING)
			{
				m_Status = "Running";
				Done = TRUE;
				break;
			}
		}
		if( !Done)
		{
			m_Status = "Could not start "+m_Driver;
			goto StartExit;
		}
	}
StartExit:
	UpdateData(FALSE);
	CloseServiceHandle(hDriver);
}

void CServicerDlg::OnStop() 
{
	if( m_hSCManager==NULL) return;

	UpdateData();

	/////////////////////////////////////////////////////////////////////////

	SC_HANDLE hDriver = OpenService(m_hSCManager,m_Driver,SERVICE_ALL_ACCESS);
	if( hDriver==NULL)
	{
		m_Status = "Could not open Service "+m_Driver;
		UpdateData(FALSE);
		return;
	}

	SERVICE_STATUS ss;
	ControlService(hDriver,SERVICE_CONTROL_INTERROGATE,&ss);
	if( ss.dwCurrentState!=SERVICE_STOPPED)
	{
		if( !ControlService(hDriver,SERVICE_CONTROL_STOP,&ss))
		{
			m_Status = "Could not stop "+m_Driver;
			goto StartExit;
		}
		m_Status = "Stopping";
		UpdateData(FALSE);
		GetDlgItem(IDC_STATUS)->UpdateWindow();
		// Give it 10 seconds to stop
		BOOL Done = FALSE;
		for(int seconds=0;seconds<10;seconds++)
		{
			Sleep(1000);
			if( ControlService(hDriver,SERVICE_CONTROL_INTERROGATE,&ss)==0 ||
				ss.dwCurrentState==SERVICE_STOPPED)
			{
				m_Status = "Stopped";
				Done = TRUE;
				break;
			}
		}
		if( !Done)
		{
			m_Status = "Could not stop "+m_Driver;
			goto StartExit;
		}
	}
StartExit:
	UpdateData(FALSE);
	CloseServiceHandle(hDriver);
}


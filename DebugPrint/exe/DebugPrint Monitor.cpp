//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//
//	DebugPrint Monitor
/////////////////////////////////////////////////////////////////////////////
//	DebugPrint Monitor.cpp:		MFC app startup/stop
/////////////////////////////////////////////////////////////////////////////
//	InitInstance	Initialise MFC app
//	ExitInstance	Exit MFC app
//					About box stuff
//	makeInt			Make int from string
//	LoadSettings	Load settings from registry
//	SaveSettings	Save most settings in registry
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MainFrm.h"
#include "DebugPrint Monitor.h"
#include "DebugPrint MonitorDoc.h"
#include "DebugPrint MonitorView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//	Registry key and value names

CString CDebugPrintMonitorApp::RegKey = _T("PHD");
CString CDebugPrintMonitorApp::KeyBase = _T("Software\\")+CDebugPrintMonitorApp::RegKey+_T("\\DebugPrint Monitor");
CString CDebugPrintMonitorApp::KeySettings = CDebugPrintMonitorApp::KeyBase + _T("\\Settings");

CString CDebugPrintMonitorApp::SettingsShowWindow = _T("ShowWindow");
CString CDebugPrintMonitorApp::SettingsWindowPosition = _T("WindowPosition");

const CString PrintFace = _T("PrintFace");
const CString PrintPointSize = _T("PrintPointSize");

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorApp

BEGIN_MESSAGE_MAP(CDebugPrintMonitorApp, CWinApp)
	//{{AFX_MSG_MAP(CDebugPrintMonitorApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorApp construction

CDebugPrintMonitorApp::CDebugPrintMonitorApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDebugPrintMonitorApp object

CDebugPrintMonitorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorApp initialization

BOOL CDebugPrintMonitorApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	SetRegistryKey(RegKey);
	LoadSettings();
	LoadStdProfileSettings(10);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CDebugPrintMonitorDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CDebugPrintMonitorView));
	AddDocTemplate(pDocTemplate);

	// Open hidden frame now so that we can position it before showing
	pDocTemplate->OpenDocumentFile( NULL,FALSE);
	if( m_ShowWindow!=-1)
		m_pMainWnd->MoveWindow(&m_rcNormalPosition);

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Maximise if wanted
	if( m_ShowWindow==SW_SHOWMAXIMIZED)
		m_pMainWnd->ShowWindow(m_ShowWindow);

	// Open connection to DebugPrint driver
	CDebugPrintMonitorView* pView = (CDebugPrintMonitorView*)((CFrameWnd*)AfxGetMainWnd())->GetActiveView();
	if( pView!=NULL)
		StartListener(pView);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

int CDebugPrintMonitorApp::ExitInstance() 
{
	StopListener();
	SaveSettings();	
	
	return CWinApp::ExitInstance();
}

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
		// No message handlers
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

// App command to run the dialog
void CDebugPrintMonitorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorApp commands



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//	Throws LPTSTR if bad

int makeInt( LPCTSTR pStr)
{
	int rval = _ttoi( pStr);

	// atoi(duff) gives 0!
	if( rval == 0 && *pStr != _T('0'))
		throw _T("Not an integer\n");

	return rval;
}

/////////////////////////////////////////////////////////////////////////////
//	LoadSettings:	Load settings from registry

void CDebugPrintMonitorApp::LoadSettings()
{
	// Set defaults
	m_ShowWindow = -1;
	m_PrintFace = _T("Arial");
	m_PrintPointSize = 10;

	// Try reading the real values
	HKEY mru;
	if( RegOpenKeyEx( HKEY_CURRENT_USER, CDebugPrintMonitorApp::KeySettings, 0, KEY_READ, &mru) == ERROR_SUCCESS)
	{
		DWORD wType, DWORDSize=sizeof(DWORD);
		DWORD MaxLength;
		
		// PrintFace
		MaxLength = _MAX_PATH;
		LPTSTR lptstrPrintFace = m_PrintFace.GetBuffer(MaxLength);
		if( RegQueryValueEx( mru, PrintFace, NULL, &wType, (LPBYTE)lptstrPrintFace, &MaxLength)
			!= ERROR_SUCCESS)
			m_PrintFace = _T("Arial");
		m_PrintFace.ReleaseBuffer();
		if( m_PrintFace.IsEmpty())
			m_PrintFace = _T("Arial");

		// PrintPointSize
		DWORD dwPrintPointSize = 12;
		RegQueryValueEx( mru, PrintPointSize, NULL, &wType, (LPBYTE)&dwPrintPointSize, &DWORDSize);
		m_PrintPointSize = dwPrintPointSize;
		if( m_PrintPointSize<5)
			m_PrintPointSize = 10;

		// ShowWindow
		DWORD dwShowWindow = SW_SHOWNORMAL;
		if( RegQueryValueEx( mru, SettingsShowWindow, NULL, &wType, (LPBYTE)&dwShowWindow, &DWORDSize)
			== ERROR_SUCCESS)
		{
			m_ShowWindow = dwShowWindow;

			// WindowPosition
			CString strWindowPosition;
			DWORD MaxLength = 80;
			LPTSTR lptstrWindowPosition = strWindowPosition.GetBuffer(MaxLength);
			if( RegQueryValueEx( mru, SettingsWindowPosition, NULL, &wType, (LPBYTE)lptstrWindowPosition, &MaxLength)
				!= ERROR_SUCCESS)
				m_ShowWindow = -1;
			else
			{
				const LPTSTR WPTokens = _T(" ,;");
				try
				{
					LPTSTR pToken = _tcstok(lptstrWindowPosition,WPTokens);
					m_rcNormalPosition.left = makeInt(pToken);
					pToken = _tcstok(NULL,WPTokens);
					m_rcNormalPosition.right = makeInt(pToken);
					pToken = _tcstok(NULL,WPTokens);
					m_rcNormalPosition.top = makeInt(pToken);
					pToken = _tcstok(NULL,WPTokens);
					m_rcNormalPosition.bottom = makeInt(pToken);
				}
				catch(...)
				{
					m_ShowWindow = -1;
					TRACE(_T("Error parsing window position (\"%s\")"),lptstrWindowPosition);
				}
			}
			strWindowPosition.ReleaseBuffer();
		}

		RegCloseKey(mru);
	}
}

/////////////////////////////////////////////////////////////////////////////
//	SaveSettings:	Save settings to registry
//	NB CMainFrame stores window placement

void CDebugPrintMonitorApp::SaveSettings()
{
	HKEY mru;
	if( RegOpenKeyEx( HKEY_CURRENT_USER, CDebugPrintMonitorApp::KeySettings, 0, KEY_WRITE, &mru) == ERROR_SUCCESS)
	{
		// PrintFace
		LPTSTR lptstrPrintFace = m_PrintFace.GetBuffer(1);
		RegSetValueEx( mru, PrintFace, NULL, REG_SZ, (LPBYTE)lptstrPrintFace,
						(m_PrintFace.GetLength()+1)*sizeof(_TCHAR));
		m_PrintFace.ReleaseBuffer();
		// PrintPointSize
		DWORD dwPrintPointSize = m_PrintPointSize;
		RegSetValueEx( mru, PrintPointSize, NULL, REG_DWORD, (LPBYTE)&dwPrintPointSize, sizeof(DWORD));

		RegCloseKey(mru);
	}
}

/////////////////////////////////////////////////////////////////////////////

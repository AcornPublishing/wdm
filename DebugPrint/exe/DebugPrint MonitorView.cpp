//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//
//	DebugPrint Monitor
/////////////////////////////////////////////////////////////////////////////
//	DebugPrint MonitorView.cpp:	MFC View
//					Holds all event info in the CListCtrl
/////////////////////////////////////////////////////////////////////////////
//					Standard stuff
//	OnInitialUpdate		Gets column widths from registry and sets ListCtrl style
//	OnDebugPrintEvent	Adds an event to the view
//	OnEditClearEvents	Clears all the events
//	OnSize				Called when view resized
//	ResizeColumnWidths	Resizes third column
//	OnDestroy			Calls...
//	StoreColWidths		Saves column widths in registry
//?	OnTrack				Track change in column header width
//?	OnEndtrack			End track change in column header width
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	20-Oct-98	1.0.0	CC	creation
//	25-Nov-98	1.0.2	CC	Clear events
//	3-Dec-98	1.0.2a	CC	Clear events clears modified flag
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DebugPrint Monitor.h"

#include "DebugPrint MonitorDoc.h"
#include "DebugPrint MonitorView.h"

#include "Listener.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

const CString DriverColWidthName = "DriverColWidth";
const CString TimestampColWidthName = "TimestampColWidth";

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorView

IMPLEMENT_DYNCREATE(CDebugPrintMonitorView, CListView)

BEGIN_MESSAGE_MAP(CDebugPrintMonitorView, CListView)
	//{{AFX_MSG_MAP(CDebugPrintMonitorView)
	ON_COMMAND(ID_EDIT_CLEAREVENTS, OnEditClearEvents)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(HDN_TRACK, OnTrack)
	ON_NOTIFY_REFLECT(HDN_ENDTRACK, OnEndtrack)
	ON_COMMAND(ID_VIEW_SHOWMILLISECONDS, OnViewShowMilliseconds)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWMILLISECONDS, OnUpdateViewShowMilliseconds)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_PREVIEW, OnUpdateFilePrint)
	ON_COMMAND(ID_VIEW_SHOWDATE, OnViewShowDate)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWDATE, OnUpdateViewShowDate)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_DEBUGPRINTEVENT,OnDebugPrintEvent)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CListView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CListView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorView construction/destruction

CDebugPrintMonitorView::CDebugPrintMonitorView()
{
	m_ShowMilliseconds = false;
	m_ShowDate = false;
	m_EventListInited = false;
	DriverColWidth = 100;
	TimestampColWidth = 150;
}

CDebugPrintMonitorView::~CDebugPrintMonitorView()
{
}

BOOL CDebugPrintMonitorView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorView drawing

void CDebugPrintMonitorView::OnDraw(CDC* pDC)
{
	CDebugPrintMonitorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorView printing

void CDebugPrintMonitorView::OnUpdateFilePrint(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(false);
}

BOOL CDebugPrintMonitorView::OnPreparePrinting(CPrintInfo* pInfo)
{
	return DoPreparePrinting(pInfo);
}

void CDebugPrintMonitorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CDebugPrintMonitorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorView diagnostics

#ifdef _DEBUG
void CDebugPrintMonitorView::AssertValid() const
{
	CListView::AssertValid();
}

void CDebugPrintMonitorView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CDebugPrintMonitorDoc* CDebugPrintMonitorView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDebugPrintMonitorDoc)));
	return (CDebugPrintMonitorDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorView message handlers

void CDebugPrintMonitorView::OnInitialUpdate() 
{
	CListView::OnInitialUpdate();

	if( !m_EventListInited)
	{
		m_EventListInited = true;

		// Load widths from registry settings
		DriverColWidth = 100;
		TimestampColWidth = 150;
		HKEY mru;
		if( RegOpenKeyEx( HKEY_CURRENT_USER, CDebugPrintMonitorApp::KeySettings, 0, KEY_READ, &mru) == ERROR_SUCCESS)
		{
			DWORD wType, widthSize=sizeof(DWORD), width;
			if( (RegQueryValueEx( mru, DriverColWidthName, NULL, &wType, (LPBYTE)&width, &widthSize) == ERROR_SUCCESS) &&
				(wType==REG_DWORD) && (width<500) && (width>10))
				DriverColWidth = width;
			if( (RegQueryValueEx( mru, TimestampColWidthName, NULL, &wType, (LPBYTE)&width, &widthSize) == ERROR_SUCCESS) &&
				(wType==REG_DWORD) && (width<500) && (width>10))
				TimestampColWidth = width;

			RegCloseKey(mru);
		}

		CListCtrl& EventList = GetListCtrl();

		// Set to report style, single selection
		long listStyle = GetWindowLong( EventList.m_hWnd, GWL_STYLE);
		SetWindowLong( EventList.m_hWnd, GWL_STYLE, listStyle | LVS_REPORT | LVS_SINGLESEL);

		// Make columns of right size
		RECT cr;
		EventList.GetClientRect(&cr);

		LV_COLUMN lvc;
		lvc.mask = LVCF_TEXT|LVCF_SUBITEM|LVCF_WIDTH;
		lvc.cx = DriverColWidth;
		lvc.pszText = _T("Driver");
		lvc.iSubItem = 0;
		EventList.InsertColumn(0,&lvc);
		lvc.cx = TimestampColWidth;
		lvc.pszText = _T("Time");
		lvc.iSubItem = 1;
		EventList.InsertColumn(1,&lvc);
		lvc.cx = cr.right-DriverColWidth-TimestampColWidth;
		lvc.pszText = _T("Event");
		lvc.iSubItem = 2;
		EventList.InsertColumn(2,&lvc);
	}
}

void CDebugPrintMonitorView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
}

/////////////////////////////////////////////////////////////////////////////
//	OnDebugPrintEvent:	Called when posted a message by listening thread

LONG CDebugPrintMonitorView::OnDebugPrintEvent( UINT, LONG lParam)
{
	DebugPrint_Event* pEvent = (DebugPrint_Event*)lParam;

	CListCtrl& EventList = GetListCtrl();

	LV_ITEM lvitem;
	lvitem.mask = LVIF_TEXT;
	lvitem.iItem = EventList.GetItemCount();
	lvitem.iSubItem = 0;
	lvitem.pszText = (LPTSTR)(LPCTSTR)pEvent->Driver;
	int ix = EventList.InsertItem(&lvitem);
	CString EventTimeMajor = pEvent->Timestamp.Format("%H:%M:%S");
	CString EventTimeMinor;
	if( m_ShowMilliseconds)
		EventTimeMinor.Format(".%03d",pEvent->Milliseconds);
	CString EventDate;
	if( m_ShowDate || !pEvent->SetModified)
		EventDate = pEvent->Timestamp.Format(", %d %b %Y");
	EventList.SetItemText( ix, 1, EventTimeMajor+EventTimeMinor+EventDate);
	EventList.SetItemText( ix, 2, pEvent->Message);
	EventList.EnsureVisible(ix,FALSE);
	GetDocument()->SetModifiedFlag(pEvent->SetModified);

	delete pEvent;
	return 0;
}

/////////////////////////////////////////////////////////////////////////////

void CDebugPrintMonitorView::OnEditClearEvents() 
{
	CListCtrl& EventList = GetListCtrl();
	EventList.DeleteAllItems();
	GetDocument()->SetModifiedFlag(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// Column widths stuff
/////////////////////////////////////////////////////////////////////////////

void CDebugPrintMonitorView::OnSize(UINT nType, int cx, int cy) 
{
	ResizeColumnWidths();
	CListView::OnSize(nType, cx, cy);
}

/////////////////////////////////////////////////////////////////////////////

void CDebugPrintMonitorView::ResizeColumnWidths()
{
	CListCtrl& EventList = GetListCtrl();
	DriverColWidth = EventList.GetColumnWidth(0);
	TimestampColWidth = EventList.GetColumnWidth(1);
	RECT crWords;
	EventList.GetClientRect(&crWords);
	if( DriverColWidth+TimestampColWidth<crWords.right)
		EventList.SetColumnWidth(2,crWords.right-DriverColWidth-TimestampColWidth);
}

/////////////////////////////////////////////////////////////////////////////

void CDebugPrintMonitorView::OnDestroy() 
{
	StoreColWidths();
	CListView::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////

void CDebugPrintMonitorView::StoreColWidths()
{
	CListCtrl& EventList = GetListCtrl();
	DriverColWidth = EventList.GetColumnWidth(0);
	TimestampColWidth = EventList.GetColumnWidth(1);

	HKEY mru;
	if( RegOpenKeyEx( HKEY_CURRENT_USER, CDebugPrintMonitorApp::KeySettings, 0, KEY_WRITE, &mru) == ERROR_SUCCESS)
	{
		RegSetValueEx( mru, DriverColWidthName, NULL, REG_DWORD, (LPBYTE)&DriverColWidth, sizeof(DWORD));
		RegSetValueEx( mru, TimestampColWidthName, NULL, REG_DWORD, (LPBYTE)&TimestampColWidth, sizeof(DWORD));
		RegCloseKey(mru);
	}
}

/////////////////////////////////////////////////////////////////////////////
//	Can't seem to catch resize headers
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//	No COMCTL32.DLL dependencies
/*
void CDebugPrintMonitorView::OnHdrItemChangedWordsColumnWidths(NMHDR* pNMHDR, LRESULT* pResult) 
{
	HD_NOTIFY *phdn = (HD_NOTIFY *) pNMHDR;
	
	if( phdn->iItem<2)
		ResizeColumnWidths();
	*pResult = 0;
}
*/

void CDebugPrintMonitorView::OnTrack(NMHDR* pNMHDR, LRESULT* pResult) 
{
	HD_NOTIFY *phdn = (HD_NOTIFY *) pNMHDR;
	
	*pResult = 0;
}

void CDebugPrintMonitorView::OnEndtrack(NMHDR* pNMHDR, LRESULT* pResult) 
{
	HD_NOTIFY *phdn = (HD_NOTIFY *) pNMHDR;
	
	*pResult = 0;
}

void CDebugPrintMonitorView::OnViewShowMilliseconds() 
{
	m_ShowMilliseconds = !m_ShowMilliseconds;
}

void CDebugPrintMonitorView::OnUpdateViewShowMilliseconds(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_ShowMilliseconds);
}

void CDebugPrintMonitorView::OnViewShowDate() 
{
	m_ShowDate = !m_ShowDate;
}

void CDebugPrintMonitorView::OnUpdateViewShowDate(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( m_ShowDate);
}

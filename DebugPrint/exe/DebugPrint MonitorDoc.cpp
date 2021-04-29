//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//
//	DebugPrint Monitor
/////////////////////////////////////////////////////////////////////////////
//	DebugPrint MonitorDoc.cpp:	MFC Document
//					No info held in document.  All in view CListCtrl
/////////////////////////////////////////////////////////////////////////////
//					Standard stuff
//	OnNewDocument	Clears view events
//	OnSaveDocument	Save events in file
//	OnOpenDocument	Load events from file
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DebugPrint Monitor.h"

#include "DebugPrint MonitorDoc.h"
#include "DebugPrint MonitorView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorDoc

IMPLEMENT_DYNCREATE(CDebugPrintMonitorDoc, CDocument)

BEGIN_MESSAGE_MAP(CDebugPrintMonitorDoc, CDocument)
	//{{AFX_MSG_MAP(CDebugPrintMonitorDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorDoc construction/destruction

CDebugPrintMonitorDoc::CDebugPrintMonitorDoc()
{
}

CDebugPrintMonitorDoc::~CDebugPrintMonitorDoc()
{
}

BOOL CDebugPrintMonitorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	POSITION pos = GetFirstViewPosition();
	CDebugPrintMonitorView* pView = (CDebugPrintMonitorView*)GetNextView(pos);
	if( pView!=NULL)
	{
		CListCtrl& EventList = pView->GetListCtrl();
		EventList.DeleteAllItems();
	}

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorDoc serialization

void CDebugPrintMonitorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorDoc diagnostics

#ifdef _DEBUG
void CDebugPrintMonitorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CDebugPrintMonitorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CDebugPrintMonitorDoc commands

BOOL CDebugPrintMonitorDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	CStdioFile dpmFile;	
	if( !dpmFile.Open(lpszPathName,CFile::modeCreate|CFile::modeWrite))
		return FALSE;
	POSITION pos = GetFirstViewPosition();
	CDebugPrintMonitorView* pView = (CDebugPrintMonitorView*)GetNextView(pos);
	CListCtrl& EventList = pView->GetListCtrl();

	LV_ITEM lv;
	char entry[1024];
	lv.mask = LVIF_TEXT;
	lv.pszText = entry;
	lv.cchTextMax = 1023;
	int numEvents = EventList.GetItemCount();
	for(int EventNo=0;EventNo<numEvents;EventNo++)
	{
		lv.iItem = EventNo;
		lv.iSubItem = 0;
		if( !EventList.GetItem(&lv))
			continue;
		dpmFile.WriteString(entry);
		dpmFile.WriteString("\t");
		lv.iSubItem = 1;
		if( !EventList.GetItem(&lv))
			continue;
		dpmFile.WriteString(entry);
		dpmFile.WriteString("\t");
		lv.iSubItem = 2;
		if( !EventList.GetItem(&lv))
			continue;
		dpmFile.WriteString(entry);
		dpmFile.WriteString("\n");
	}
	SetModifiedFlag(FALSE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CDebugPrintMonitorDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	CStdioFile dpmFile;	
	if( !dpmFile.Open(lpszPathName,CFile::modeRead))
		return FALSE;

	POSITION pos = GetFirstViewPosition();
	CDebugPrintMonitorView* pView = (CDebugPrintMonitorView*)GetNextView(pos);
	CListCtrl& EventList = pView->GetListCtrl();
	EventList.DeleteAllItems();

	LV_ITEM lvitem;
	lvitem.mask = LVIF_TEXT;

	CString line;
	for(;dpmFile.ReadString(line);)
	{
		int FirstTabPos = line.Find('\t');
		if( FirstTabPos==-1) continue;
		CString Driver = line.Left(FirstTabPos);
		line = line.Mid(FirstTabPos+1);
		int NextTabPos = line.Find('\t');
		if( NextTabPos==-1) continue;
		CString Timestamp = line.Left(NextTabPos);
		CString Msg = line.Mid(NextTabPos+1);
		lvitem.iItem = EventList.GetItemCount();
		lvitem.iSubItem = 0;
		lvitem.pszText = (LPTSTR)(LPCTSTR)Driver;
		int ix = EventList.InsertItem(&lvitem);
		EventList.SetItemText( ix, 1, Timestamp);
		EventList.SetItemText( ix, 2, Msg);
	}
	if( EventList.GetItemCount()>0)
		EventList.SetItemState(0,LVIS_SELECTED,-1);
	EventList.UpdateWindow();
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

// mainfrm.cpp : implementation of the CMainFrame class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"

#include "wordpad.h"
#include "mainfrm.h"
#include "wordpdoc.h"
#include "wordpvw.h"
#include "strings.h"
#include "colorlis.h"

extern BOOL AFXAPI AfxFullPath(LPTSTR lpszPathOut, LPCTSTR lpszFileIn);

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_SIZE()
	ON_WM_MOVE()
	ON_COMMAND(ID_HELP, OnHelpFinder)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_WM_DROPFILES()
	ON_COMMAND(ID_CHAR_COLOR, OnCharColor)
	ON_COMMAND(ID_PEN_TOGGLE, OnPenToggle)
	ON_WM_FONTCHANGE()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_PALETTECHANGED()
	ON_WM_DEVMODECHANGE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_DOC_TABS, OnSelChangeDocTabs)
	ON_COMMAND(ID_HELP_INDEX, OnHelpFinder)
	//}}AFX_MSG_MAP
	// Global help commands
//  ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, OnHelpFinder)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FORMATBAR, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_RULER, OnUpdateControlBarMenu)
	ON_MESSAGE(WPM_BARSTATE, OnBarState)
	ON_REGISTERED_MESSAGE(CWordPadApp::m_nOpenMsg, OnOpenMsg)
	ON_COMMAND_EX(ID_VIEW_STATUS_BAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_TOOLBAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_FORMATBAR, OnBarCheck)
	ON_COMMAND_EX(ID_VIEW_RULER, OnBarCheck)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars

// toolbar buttons - IDs are command buttons
static UINT BASED_CODE toolbar[] =
{
	// same order as in the bitmap 'toolbar.bmp'
	// (int nBitmap, int nCommand, BYTE byteState, BYTE byteStyle, DWORD dw, int nString)
	ID_FILE_NEW,
	ID_FILE_OPEN,
	ID_FILE_SAVE,
ID_SEPARATOR,
	ID_FILE_PRINT_DIRECT,
	ID_FILE_PRINT_PREVIEW,
ID_SEPARATOR,
	ID_EDIT_FIND,
ID_SEPARATOR,
	ID_EDIT_CUT,
	ID_EDIT_COPY,
	ID_EDIT_PASTE,
	ID_EDIT_UNDO,
ID_SEPARATOR,
	ID_INSERT_DATE_TIME,
ID_SEPARATOR,
	ID_PEN_TOGGLE,
	ID_PEN_PERIOD,
	ID_PEN_SPACE,
	ID_PEN_BACKSPACE,
	ID_PEN_NEWLINE,
	ID_PEN_LENS
};

#define NUM_PEN_ITEMS 7
#define NUM_PEN_TOGGLE 5

static UINT BASED_CODE format[] =
{
	// same order as in the bitmap 'format.bmp'
		ID_SEPARATOR, // font name combo box
		ID_SEPARATOR,
		ID_SEPARATOR, // font size combo box
		ID_SEPARATOR,
	ID_CHAR_BOLD,
	ID_CHAR_ITALIC,
	ID_CHAR_UNDERLINE,
	ID_CHAR_COLOR,
		ID_SEPARATOR,
	ID_PARA_LEFT,
	ID_PARA_CENTER,
	ID_PARA_RIGHT,
		ID_SEPARATOR,
	ID_INSERT_BULLET,
};

static UINT BASED_CODE indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
};

static const int kDocTabHeight = 24;
static const UINT kDragQueryAllFiles = 0xFFFFFFFFU;

static BOOL HasPath(LPCTSTR lpszPathName)
{
	return lpszPathName != NULL && lpszPathName[0] != _T('\0');
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
	: m_nActiveTab(-1), m_bChangingTabs(FALSE)
{
	m_hIconDoc = theApp.LoadIcon(IDI_ICON_DOC);
	m_hIconText = theApp.LoadIcon(IDI_ICON_TEXT);
	m_hIconWrite = theApp.LoadIcon(IDI_ICON_WRITE);
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	WNDCLASS wndcls;

	BOOL bRes = CFrameWnd::PreCreateWindow(cs);
	HINSTANCE hInst = AfxGetInstanceHandle();

	// see if the class already exists
	if (!::GetClassInfo(hInst, szWordPadClass, &wndcls))
	{
		// get default stuff
		::GetClassInfo(hInst, cs.lpszClass, &wndcls);
		wndcls.style &= ~(CS_HREDRAW|CS_VREDRAW);
		// register a new class
		wndcls.lpszClassName = szWordPadClass;
		wndcls.hIcon = ::LoadIcon(hInst, MAKEINTRESOURCE(IDR_MAINFRAME));
		ASSERT(wndcls.hIcon != NULL);
		if (!AfxRegisterClass(&wndcls))
			AfxThrowResourceException();
	}
	cs.lpszClass = szWordPadClass;
	CRect rect = theApp.m_rectInitialFrame;
	if (rect.Width() > 0 && rect.Height() > 0)
	{
		// make sure window will be visible
		CDisplayIC dc;
		CRect rectDisplay(0, 0, dc.GetDeviceCaps(HORZRES),
			dc.GetDeviceCaps(VERTRES));
		if (rectDisplay.PtInRect(rect.TopLeft()) &&
			rectDisplay.PtInRect(rect.BottomRight()))
		{
			cs.x = rect.left;
			cs.y = rect.top;
			cs.cx = rect.Width();
			cs.cy = rect.Height();
		}
	}
	return bRes;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!CreateToolBar())
		return -1;

	if (!CreateFormatBar())
		return -1;

	if (!CreateStatusBar())
		return -1;

	if (!m_wndDocTabs.Create(WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|TCS_TABS,
		CRect(0, 0, 0, 0), this, IDC_DOC_TABS))
	{
		TRACE0("Failed to create document tabs\n");
		return -1;
	}

	EnableDocking(CBRS_ALIGN_ANY);

	if (!CreateRulerBar())
		return -1;

	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndFormatBar.EnableDocking(CBRS_ALIGN_TOP|CBRS_ALIGN_BOTTOM);
	DockControlBar(&m_wndToolBar);
	DockControlBar(&m_wndFormatBar);

	CWnd* pView = GetDlgItem(AFX_IDW_PANE_FIRST);
	if (pView != NULL)
	{
		pView->SetWindowPos(&wndBottom, 0, 0, 0, 0,
			SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
	}

	return 0;
}

BOOL CMainFrame::CreateToolBar()
{
	int nPen = GetSystemMetrics(SM_PENWINDOWS) ? NUM_PEN_TOGGLE :
		NUM_PEN_ITEMS;
	UINT nID = theApp.m_bLargeIcons ? IDR_MAINFRAME1_BIG :
		IDR_MAINFRAME1;
	if (!m_wndToolBar.Create(this,
		WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_SIZE_DYNAMIC)||
		!m_wndToolBar.LoadBitmap(nID) ||
		!m_wndToolBar.SetButtons(toolbar, sizeof(toolbar)/sizeof(UINT) - nPen))
	{
		TRACE0("Failed to create toolbar\n");
		return FALSE;      // fail to create
	}
	if (theApp.m_bLargeIcons)
		m_wndToolBar.SetSizes(CSize(31,30), CSize(24,24));
	else
		m_wndToolBar.SetSizes(CSize(23,22), CSize(16,16));
	CString str;
	str.LoadString(IDS_TITLE_TOOLBAR);
	m_wndToolBar.SetWindowText(str);
	return TRUE;
}

BOOL CMainFrame::CreateFormatBar()
{
	UINT nID = theApp.m_bLargeIcons ? IDB_FORMATBAR_BIG : IDB_FORMATBAR;
	if (!m_wndFormatBar.Create(this,
		WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_HIDE_INPLACE|CBRS_SIZE_DYNAMIC,
		ID_VIEW_FORMATBAR) ||
		!m_wndFormatBar.LoadBitmap(nID) ||
		!m_wndFormatBar.SetButtons(format, sizeof(format)/sizeof(UINT)))
	{
		TRACE0("Failed to create FormatBar\n");
		return FALSE;      // fail to create
	}

	if (theApp.m_bLargeIcons)
		m_wndFormatBar.SetSizes(CSize(31,30), CSize(24,24));
	else
		m_wndFormatBar.SetSizes(CSize(23,22), CSize(16,16));
	CString str;
	str.LoadString(IDS_TITLE_FORMATBAR);
	m_wndFormatBar.SetWindowText(str);
	m_wndFormatBar.PositionCombos();
	return TRUE;
}

BOOL CMainFrame::CreateRulerBar()
{
	if (!m_wndRulerBar.Create(this,
		WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_HIDE_INPLACE, ID_VIEW_RULER))
	{
		TRACE0("Failed to create ruler\n");
		return FALSE;      // fail to create
	}
	return TRUE;
}

BOOL CMainFrame::CreateStatusBar()
{
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return FALSE;      // fail to create
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame Operations

HICON CMainFrame::GetIcon(int nDocType)
{
	switch (nDocType)
	{
		case RD_WINWORD6:
		case RD_WORDPAD:
		case RD_EMBEDDED:
		case RD_RICHTEXT:
			return m_hIconDoc;
		case RD_TEXT:
		case RD_OEMTEXT:
			return m_hIconText;
		case RD_WRITE:
			return m_hIconWrite;
	}
	return m_hIconDoc;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnFontChange()
{
	m_wndFormatBar.SendMessage(CWordPadApp::m_nPrinterChangedMsg);
}

void CMainFrame::OnDevModeChange(LPTSTR lpDeviceName)
{
	theApp.NotifyPrinterChanged();
	CFrameWnd::OnDevModeChange(lpDeviceName); //sends message to descendants
}

void CMainFrame::OnSysColorChange()
{
	CFrameWnd::OnSysColorChange();
	m_wndRulerBar.SendMessage(WM_SYSCOLORCHANGE);
}

void CMainFrame::ActivateFrame(int nCmdShow)
{
	CFrameWnd::ActivateFrame(nCmdShow);
	// make sure and display the toolbar, ruler, etc while loading a document.
	OnIdleUpdateCmdUI();
	SyncActiveTabWithDocument();
	UpdateWindow();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);
	theApp.m_bMaximized = (nType == SIZE_MAXIMIZED);
	if (nType == SIZE_RESTORED)
		GetWindowRect(theApp.m_rectInitialFrame);
	if (::IsWindow(m_wndDocTabs.GetSafeHwnd()))
	{
		CRect rect;
		GetClientRect(rect);
		RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, reposQuery, &rect);
		m_wndDocTabs.MoveWindow(rect.left, rect.top, rect.Width(), kDocTabHeight);
		CWnd* pView = GetDlgItem(AFX_IDW_PANE_FIRST);
		if (pView != NULL)
		{
			pView->MoveWindow(rect.left, rect.top + kDocTabHeight, rect.Width(),
				max(0, rect.Height() - kDocTabHeight));
		}
	}
}

LONG_PTR CMainFrame::OnBarState(UINT_PTR wParam, LONG_PTR lParam)
{
	if (lParam == -1)
		return 0L;
	ASSERT(lParam != RD_EMBEDDED);
	if (wParam == 0)
	{
		CDockState& ds = theApp.GetDockState(lParam);
		ds.Clear(); // empty out the dock state
		GetDockState(ds);
	}
	else
	{
		if (IsTextType(lParam))
		{
			// in text mode hide the ruler and format bar so that it is the default
			CControlBar* pBar = GetControlBar(ID_VIEW_FORMATBAR);
			if (pBar != NULL)
				pBar->ShowWindow(SW_HIDE);
			pBar = GetControlBar(ID_VIEW_RULER);
			if (pBar != NULL)
				pBar->ShowWindow(SW_HIDE);
		}
		HICON hIcon = GetIcon((int)lParam);
		SendMessage(WM_SETICON, TRUE, (LPARAM)hIcon);
		SetDockState(theApp.GetDockState(lParam));
	}
	return 0L;
}

void CMainFrame::OnMove(int x, int y)
{
	CFrameWnd::OnMove(x, y);
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);
	theApp.m_rectInitialFrame = wp.rcNormalPosition;
	CView* pView = GetActiveView();
	if (pView != NULL)
		pView->SendMessage(WM_MOVE);
}

LONG_PTR CMainFrame::OnOpenMsg(UINT_PTR, LONG_PTR lParam)
{
	TCHAR szAtomName[256];
	szAtomName[0] = _T('\0');
	GlobalGetAtomName((ATOM)lParam, szAtomName, 256);
	if (szAtomName[0] == _T('\0'))
		return FALSE;
	int nTab = FindTabByPath(szAtomName);
	if (nTab >= 0 && ActivateTab(nTab))
		return TRUE;
	return OpenDocumentAsTab(szAtomName);
}

void CMainFrame::OnHelpFinder()
{
	theApp.WinHelp(0, HELP_FINDER);
}

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	TCHAR szFileName[_MAX_PATH];
	UINT nFiles = ::DragQueryFile(hDropInfo, kDragQueryAllFiles, NULL, 0);
	SyncActiveTabWithDocument();
	for (UINT nFile = 0; nFile < nFiles; ++nFile)
	{
		::DragQueryFile(hDropInfo, nFile, szFileName, _MAX_PATH);
		OpenDocumentAsTab(szFileName);
	}
	::DragFinish(hDropInfo);
}

void CMainFrame::OnFileOpen()
{
	CString newName;
	int nType = RD_DEFAULT;
	if (!theApp.PromptForFileName(newName, AFX_IDS_OPENFILE,
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, TRUE, &nType))
	{
		return;
	}

	SyncActiveTabWithDocument();
	if (nType == RD_OEMTEXT)
		theApp.m_bForceOEM = TRUE;
	OpenDocumentAsTab(newName);
	theApp.m_bForceOEM = FALSE;
}

void CMainFrame::OnFileNew()
{
	SyncActiveTabWithDocument();
	int nTab = m_wndDocTabs.InsertItem(m_wndDocTabs.GetItemCount(), _T("Untitled"));
	m_tabPaths.Add(_T(""));
	m_nActiveTab = nTab;
	m_bChangingTabs = TRUE;
	m_wndDocTabs.SetCurSel(nTab);
	m_bChangingTabs = FALSE;
	theApp.OnFileNew();
	SyncActiveTabWithDocument();
}

void CMainFrame::OnSelChangeDocTabs(NMHDR*, LRESULT* pResult)
{
	if (pResult != NULL)
		*pResult = 0;
	if (m_bChangingTabs)
		return;
	int nSel = m_wndDocTabs.GetCurSel();
	if (nSel >= 0 && nSel != m_nActiveTab)
		ActivateTab(nSel);
}

int CMainFrame::FindTabByPath(LPCTSTR lpszPathName) const
{
	if (!HasPath(lpszPathName))
		return -1;
	TCHAR szPath[_MAX_PATH];
	if (!AfxFullPath(szPath, lpszPathName))
		return -1;
	for (int i = 0; i < m_tabPaths.GetSize(); ++i)
	{
		if (!m_tabPaths[i].IsEmpty() && lstrcmpi(m_tabPaths[i], szPath) == 0)
			return i;
	}
	return -1;
}

void CMainFrame::UpdateTabText(int nTab)
{
	if (nTab < 0 || nTab >= m_wndDocTabs.GetItemCount())
		return;
	TC_ITEM item;
	item.mask = TCIF_TEXT;
	CString strText;
	if (nTab < m_tabPaths.GetSize() && !m_tabPaths[nTab].IsEmpty())
	{
		strText = m_tabPaths[nTab];
		int nPos = strText.ReverseFind(_T('\\'));
		if (nPos >= 0)
			strText = strText.Mid(nPos + 1);
	}
	if (strText.IsEmpty())
		strText = _T("Untitled");
	item.pszText = strText.GetBuffer(strText.GetLength());
	m_wndDocTabs.SetItem(nTab, &item);
	strText.ReleaseBuffer();
}

BOOL CMainFrame::ActivateTab(int nTab)
{
	if (nTab < 0 || nTab >= m_tabPaths.GetSize())
		return FALSE;

	SyncActiveTabWithDocument();
	m_bChangingTabs = TRUE;
	BOOL bOpened = FALSE;
	if (m_tabPaths[nTab].IsEmpty())
		bOpened = theApp.OpenDocumentFile(NULL) != NULL;
	else
		bOpened = theApp.OpenDocumentFile(m_tabPaths[nTab]) != NULL;
	if (bOpened)
	{
		m_nActiveTab = nTab;
		m_wndDocTabs.SetCurSel(nTab);
		SyncActiveTabWithDocument();
	}
	else if (m_nActiveTab >= 0 && m_nActiveTab < m_wndDocTabs.GetItemCount())
	{
		m_wndDocTabs.SetCurSel(m_nActiveTab);
	}
	m_bChangingTabs = FALSE;
	return bOpened;
}

BOOL CMainFrame::OpenDocumentAsTab(LPCTSTR lpszPathName)
{
	if (!HasPath(lpszPathName))
		return FALSE;
	TCHAR szPath[_MAX_PATH];
	if (!AfxFullPath(szPath, lpszPathName))
	{
		TRACE0("Failed to resolve document path for tab open\n");
		return FALSE;
	}
	int nTab = FindTabByPath(szPath);
	if (nTab < 0)
	{
		nTab = m_wndDocTabs.InsertItem(m_wndDocTabs.GetItemCount(), _T(""));
		m_tabPaths.Add(szPath);
		UpdateTabText(nTab);
	}
	return ActivateTab(nTab);
}

void CMainFrame::SyncActiveTabWithDocument()
{
	if (!::IsWindow(m_wndDocTabs.GetSafeHwnd()))
		return;
	CWordPadDoc* pDoc = (CWordPadDoc*)GetActiveDocument();
	if (pDoc == NULL)
		return;
	CString strPath = pDoc->GetPathName();
	if (m_nActiveTab < 0 || m_nActiveTab >= m_tabPaths.GetSize())
	{
		m_nActiveTab = m_wndDocTabs.InsertItem(m_wndDocTabs.GetItemCount(), _T(""));
		m_tabPaths.Add(strPath);
	}
	else
	{
		m_tabPaths[m_nActiveTab] = strPath;
	}
	UpdateTabText(m_nActiveTab);
	m_bChangingTabs = TRUE;
	m_wndDocTabs.SetCurSel(m_nActiveTab);
	m_bChangingTabs = FALSE;
}

void CMainFrame::OnCharColor()
{
	CColorMenu colorMenu;
	CRect rc;
	int index = m_wndFormatBar.CommandToIndex(ID_CHAR_COLOR);
	m_wndFormatBar.GetItemRect(index, &rc);
	m_wndFormatBar.ClientToScreen(rc);
	colorMenu.TrackPopupMenu(TPM_LEFTALIGN|TPM_LEFTBUTTON,rc.left,rc.bottom, this);
}

void CMainFrame::OnPenToggle()
{
	static int nPen = 0;
	m_wndToolBar.SetButtons(toolbar, sizeof(toolbar)/sizeof(UINT) - nPen);
	nPen = (nPen == 0) ? NUM_PEN_TOGGLE : 0;
	m_wndToolBar.Invalidate();
	m_wndToolBar.GetParentFrame()->RecalcLayout();
}

BOOL CMainFrame::OnQueryNewPalette()
{
	CView* pView = GetActiveView();
	if (pView != NULL)
		return (BOOL)pView->SendMessage(WM_QUERYNEWPALETTE);
	return FALSE;
}

void CMainFrame::OnPaletteChanged(CWnd* pFocusWnd)
{
	CView* pView = GetActiveView();
	if (pView != NULL)
		pView->SendMessage(WM_PALETTECHANGED, (WPARAM)pFocusWnd->GetSafeHwnd());
}

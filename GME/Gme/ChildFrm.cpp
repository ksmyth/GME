// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "GMEApp.h"

#include "ChildFrm.h"
#include "MainFrm.h"

#include "GMEView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
	ON_WM_CREATE()
	ON_COMMAND_EX(CG_ID_VIEW_MODELPROPERTIESBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(CG_ID_VIEW_MODELPROPERTIESBAR, OnUpdateControlBarMenu)
	//{{AFX_MSG_MAP(CChildFrame)
	ON_WM_CLOSE()
	ON_WM_MDIACTIVATE()
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
	sendEvent = true;
	view = 0;
}

CChildFrame::~CChildFrame()
{
}

CString CChildFrame::GetTitle(void) const
{
	return m_title;
}

void CChildFrame::SetTitle(const CString& title)
{
	m_title = title;
}

CString CChildFrame::GetAppTitle(void) const
{
	return m_appTitle;
}

void CChildFrame::SetAppTitle(const CString& appTitle)
{
	m_appTitle = appTitle;
}

void CChildFrame::SetSendEvent(bool sendEventParam)
{
	sendEvent = sendEventParam;
}

void CChildFrame::SetView(CView* viewParam)
{
	view = viewParam;
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	if( !CMDIChildWndEx::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWndEx::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWndEx::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

int CChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIChildWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

#if !defined(ACTIVEXGMEVIEW)
	{
		// Initialize dialog bar propBar
		if (!propBar.Create(_T("Model Properties"), this, FALSE, CG_IDD_MODELPROPERTIESBAR,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP,
			CG_ID_VIEW_MODELPROPERTIESBAR))
		{
			TRACE0("Failed to create dialog bar propBar\n");
			return -1;		// fail to create
		}

		// terge 
		int zoomvals[] = {ZOOM_MIN, 10, 25, 50, 75, ZOOM_NO, 150, 200, 300, ZOOM_MAX, ZOOM_WIDTH, ZOOM_HEIGHT, ZOOM_ALL, 0};
//		int zoomvals[] = {ZOOM_NO, 150, 200, 250, 300, 350, 400, 0}; // for test
		propBar.SetZoomList(zoomvals);
		propBar.DockToFrameWindow(CBRS_ALIGN_TOP);
		propBar.SetMinSize(CSize(10, 26 * CClientDC(this).GetDeviceCaps(LOGPIXELSY) / 96));

		ShowPane(&propBar, true, false, true);
	}
#endif

	return 0;
}

void CChildFrame::OnClose()
{
	bool doClose = true;
	if (sendEvent && view)
		doClose = ((CGMEView *)view)->SendCloseModelEvent();
	sendEvent = true;

	if(doClose)
	{
		CGMEMFCTabCtrl& tabs = *(CGMEMFCTabCtrl*)m_pRelatedTabGroup;
		
		tabs.EnableActivateLastActive(TRUE);
		tabs.EnableActivateLastVisible(TRUE);
		tabs._SetLastActiveTab();
		if (tabs.GetTabsNum() > 2)
		{
			int active = tabs.GetActiveTab();
			int visible;
			CWnd* activeWnd = tabs.GetFirstVisibleTab(active, visible);
			bool thisWindowActive = activeWnd != NULL && activeWnd->GetSafeHwnd() == GetSafeHwnd();
			if (thisWindowActive && active != tabs.GetTabsNum())
			{
				tabs.EnableActivateLastActive(FALSE);
			}
		}
		// CFramewWnd::OnClose calls CMDIChildWnd::DestroyWindow
		CMDIChildWndEx::OnClose();
	}


	// CMDIChildWndEx::OnClose: when the last ChildWnd is closed
	// the document is considered closed and the title changes to Paradigm
	// that's why we call this:
	theApp.UpdateMainTitle();

// 	DestroyWindow();
}

void CChildFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIChildWndEx::OnSize(nType, cx, cy);

	// it is necessary to change the title manually especially
	// when the first childwnd is created
	// when the childwnd's are maximized
	theApp.UpdateMainTitle();
}

void CChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle) {
	TCHAR szOld[256];
	if (GetWindowText(szOld, _countof(szOld)) != m_title.GetLength() ||
		lstrcmp(szOld, m_title) != 0) {
		SetWindowText(m_title); 
	}
}

BOOL CChildFrame::PreTranslateMessage(MSG* pMsg)
{
	long xPos = GET_X_LPARAM(pMsg->lParam);
	long yPos = GET_Y_LPARAM(pMsg->lParam);

	auto& GetChildFrame = [&]() {
		CMFCTabCtrl* tabCtrl = GetRelatedTabGroup();
		CPoint point(xPos, yPos);
		if (tabCtrl->IsPtInTabArea(point)) {
			int i = tabCtrl->GetTabFromPoint(point);
			if (i >= 0) {
				CWnd* tabCtrlWnd = tabCtrl->GetTabWnd(i);
				if (tabCtrlWnd != NULL && tabCtrlWnd->IsKindOf(RUNTIME_CLASS(CChildFrame))) {
					CChildFrame* cf = STATIC_DOWNCAST(CChildFrame, tabCtrlWnd);
					return cf;
				}
			}
		}
		return (CChildFrame*)nullptr;
	};

	CChildFrame* cf;
	if (pMsg->message == WM_MBUTTONUP && (cf = GetChildFrame())) {
		if (cf) {
			cf->PostMessage(WM_CLOSE);
			return TRUE;
		}
	}
	else if (pMsg->message == WM_RBUTTONUP && (cf = GetChildFrame())) {
		CMenu menu;
		menu.LoadMenu(IDR_TAB_MENU);
		CMenu *submenu = menu.GetSubMenu(0);
		CPoint screen = { xPos, yPos };
		::ClientToScreen(pMsg->hwnd, &screen);

		int cmd = (int)submenu->TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
			screen.x, screen.y, GetParent());
		switch (cmd) {
		case ID_CLOSEOTHERS_CLOSE:
			cf->PostMessage(WM_CLOSE);
			return TRUE;
		case ID_CLOSEOTHERS_CLOSETOTHERIGHT: // fallthrough
		case ID_CLOSEOTHERS_CLOSEOTHERS:
		{
			CMFCTabCtrl* tabCtrl = GetRelatedTabGroup();
			int currentTab = 0;
			for (int i = 0; i < tabCtrl->GetTabsNum(); i++) {
				CWnd* tab = tabCtrl->GetTabWnd(i);
				if (tab == cf) {
					currentTab = i;
				}
			}
			for (int i = 0; i < tabCtrl->GetTabsNum(); i++) {
				CWnd* tab = tabCtrl->GetTabWnd(i);
				CRect rect;
				tabCtrl->GetTabRect(i, rect);

				if (tab == cf) {
				}
				else if (cmd != ID_CLOSEOTHERS_CLOSETOTHERIGHT || i > currentTab) {
					tab->PostMessage(WM_CLOSE);
				}
			}
			return TRUE;
		}
		}
	}

	return CMDIChildWndEx::PreTranslateMessage(pMsg);
}

void CChildFrame::OnMDIActivate(BOOL bActivate,
		CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
	CMDIChildWndEx::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);
	if (pActivateWnd == this)
	{
		CGMEView* view = ((CGMEView*)GetActiveView());
		if (!view->alive)
			return;
		
		view->DoPannWinRefresh();
		if (view->guiMeta) {
			theApp.UpdateCompList4CurrentKind( view->guiMeta->name);
			CMainFrame::theInstance->SetPartBrowserMetaModel(view->guiMeta);
			CMainFrame::theInstance->SetPartBrowserBg(view->bgColor);
			CMainFrame::theInstance->ChangePartBrowserAspect(view->currentAspect->index);

			view->ChangeAttrPrefObjs(view->selected);

		}
		view->SetScroll();
	}
}

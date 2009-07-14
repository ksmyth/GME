// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "GMEApp.h"

#include "ChildFrm.h"

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
		propBar.SetMinSize(CSize(0, 25));

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
		CMDIChildWndEx::OnClose();

	// CMDIChildWndEx::OnClose: when the last ChildWnd is closed
	// the document is considered closed and the title changes to Paradigm
	// that's why we call this:
	theApp.UpdateMainTitle();
}

void CChildFrame::OnSize(UINT nType, int cx, int cy)
{
	CMDIChildWndEx::OnSize(nType, cx, cy);

	// it is necessary to change the title manually especially
	// when the first childwnd is created
	// when the childwnd's are maximized
	theApp.UpdateMainTitle();
}

BOOL CChildFrame::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_MBUTTONUP) {
		CMFCTabCtrl* tabCtrl = GetRelatedTabGroup();
		long xPos = GET_X_LPARAM(pMsg->lParam);
		long yPos = GET_Y_LPARAM(pMsg->lParam);
		CPoint point(xPos, yPos);
		if (tabCtrl->IsPtInTabArea(point)) {
			int i = tabCtrl->GetTabFromPoint(point);
			if (i >= 0) {
				CWnd* tabCtrlWnd = tabCtrl->GetTabWnd(i);
				if (tabCtrlWnd != NULL && tabCtrlWnd->IsKindOf(RUNTIME_CLASS(CChildFrame))) {
					CChildFrame* cf = STATIC_DOWNCAST(CChildFrame, tabCtrlWnd);
					cf->PostMessage(WM_CLOSE);
				}
			}
		}
	}

	return CMDIChildWndEx::PreTranslateMessage(pMsg);
}

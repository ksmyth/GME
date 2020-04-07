// PartBrowserPaneFrame.cpp : implementation file
//

#include "stdafx.h"
#include "PartBrowserPane.h"
#include "PartBrowserPaneFrame.h"
#include "PartBrowserDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPartBrowserPaneFrame dialog


CPartBrowserPaneFrame::CPartBrowserPaneFrame(CWnd* pParent/* = NULL*/)
	: CDialog(CPartBrowserPaneFrame::IDD, pParent),
	vScrollWidth (::GetSystemMetrics (SM_CXVSCROLL)),	// WinXP default style: 17
	lineSize (20),
	pageSize (60),
    pageHeight(100), // maybe fix UNINITIALIZED READ: reading register edx PartBrowser.OCX!CPartBrowserPaneFrame::Resize [c:\users\kevin\documents\gme\gme\partbrowser\partbrowserpaneframe.cpp:57]
    logicalHeight(100),
    scrollPos(0)
{
	//{{AFX_DATA_INIT(CPartBrowserPaneFrame)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPartBrowserPaneFrame::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPartBrowserPaneFrame)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPartBrowserPaneFrame, CDialog)
	//{{AFX_MSG_MAP(CPartBrowserPaneFrame)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CPartBrowserPaneFrame::Resize(RECT r)
{
	MoveWindow(&r);
	GetClientRect(&r);

	// ok, if there is the scroll bar there then we want to
	// increase the size, the number 16 is empirical 

	if (logicalHeight > pageHeight)
		r.right += vScrollWidth;
	
	pane.MoveWindow(&r);
	BringWindowToTop();
	pane.BringWindowToTop();
	pane.Resize(r);
}

void CPartBrowserPaneFrame::SetScrollBar()
{
	if (scrollPos > (logicalHeight - pageHeight + 1))
		scrollPos = (logicalHeight - pageHeight + 1);
	if (scrollPos < 0)
		scrollPos = 0;

	if (logicalHeight > pageHeight) {
		SCROLLINFO si;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		si.nPage = pageHeight;
		si.nMin = 0;
		si.nMax = logicalHeight;
		si.nPos = scrollPos;
		SetScrollInfo(SB_VERT, &si);
	}

	ShowScrollBar(SB_VERT, logicalHeight > pageHeight);
}

/////////////////////////////////////////////////////////////////////////////
// Get/Set methods

CPartBrowserPane& CPartBrowserPaneFrame::GetPane (void)
{
	return pane;
}


void CPartBrowserPaneFrame::SetLogicalHeight (int logHeight)
{
	logicalHeight = logHeight;
}


void CPartBrowserPaneFrame::SetPageHeight (int pgHeight)
{
	pageHeight = pgHeight;
}


int CPartBrowserPaneFrame::GetScrollPosition (void)
{
	return scrollPos;
}


void CPartBrowserPaneFrame::SetScrollPosition (int scrollPosition)
{
	scrollPos = scrollPosition;
}


/////////////////////////////////////////////////////////////////////////////
// CPartBrowserPaneFrame message handlers

BOOL CPartBrowserPaneFrame::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	RECT r;
	GetClientRect(&r);
	BOOL success = pane.Create(NULL, _T("PartsPane"), WS_CHILD | WS_VISIBLE | WS_BORDER, r, this, IDD_PARTBROWSER_PANE);
	if (success) {
		pane.ModifyStyleEx(0, WS_EX_CLIENTEDGE);
	}

	return success;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE
}

void CPartBrowserPaneFrame::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* /*pScrollBar*/)
{
	int oldPos = scrollPos;

	switch(nSBCode)
	{
	case SB_TOP:
		scrollPos = 0;
		break;
	case SB_BOTTOM:
		scrollPos = logicalHeight - pageHeight;
		break;
	case SB_ENDSCROLL:
		break;
	case SB_LINEDOWN:
		scrollPos += lineSize;
		break;
	case SB_LINEUP:
		scrollPos -= lineSize; 
		break;
	case SB_PAGEDOWN:
		scrollPos += pageSize;
		break;
	case SB_PAGEUP:
		scrollPos -= pageSize;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		scrollPos = nPos;
		break;
	}

	if (scrollPos > (logicalHeight - pageHeight + 1))
		scrollPos = (logicalHeight - pageHeight + 1);
	if (scrollPos < 0)
		scrollPos = 0;

	pane.ScrollWindow(0, oldPos - scrollPos, NULL, NULL);
	SetScrollPos(SB_VERT, scrollPos);

//	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CPartBrowserPaneFrame::OnMouseWheel(UINT fFlags, short zDelta, CPoint point)
{
	UNUSED_ALWAYS(point);

	// we don't handle anything but scrolling just now
	if (fFlags & (MK_SHIFT | MK_CONTROL))
		return FALSE;

	// if we have a vertical scroll bar, the wheel scrolls that
	// if we have _only_ a horizontal scroll bar, the wheel scrolls that
	// otherwise, don't do any work at all

	DWORD dwStyle = GetStyle();
	CScrollBar* pBar = GetScrollBarCtrl(SB_VERT);
	BOOL bHasVertBar = ((pBar != NULL) && pBar->IsWindowEnabled()) ||
						(dwStyle & WS_VSCROLL);

	if (bHasVertBar) {
		OnVScroll(zDelta < 0 ? SB_LINEDOWN : SB_LINEUP, 0, NULL);

		UpdateWindow();
		return TRUE;
	}

	return FALSE;
}

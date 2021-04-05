// SrollZoomView.cpp : implementation file
//


#include "stdafx.h"
#include "ScrollZoomView.h"

#include "GMEApp.h"
#include "Mainfrm.h"
//

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CX_BORDER   1
#define CY_BORDER   1

/////////////////////////////////////////////////////////////////////////////
// CScrollZoomView
IMPLEMENT_DYNAMIC(CScrollZoomView, CView)
BEGIN_MESSAGE_MAP(CScrollZoomView, CView)
	//{{AFX_MSG_MAP(CScrollZoomView)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Special mapping modes just for CScrollZoomView implementation
#define MM_NONE             0
#define MM_SCALETOFIT       (-1)
	// standard GDI mapping modes are > 0


UINT CScrollZoomView::uCachedScrollLines	= 3;
bool CScrollZoomView::bScrollLinesRequested	= false;

/////////////////////////////////////////////////////////////////////////////
// CScrollZoomView construction/destruction

CScrollZoomView::CScrollZoomView() : 
	m_nMapMode(MM_NONE), 
	m_totalLog(0,0), 
	m_totalDev(0,0), 
	m_pageDev(0,0), 
	m_lineDev(0,0), 
	m_scalePercent(0), 
	m_bCenter(FALSE), 
	m_bInsideUpdate(FALSE), 
	m_zoomP(0,0),
	m_zoomScroll(false),
	m_noHscroll(true),
	m_noVscroll(true)
{
}

CScrollZoomView::~CScrollZoomView()
{
}

UINT CScrollZoomView::GetMouseScrollLines(void)
{

	// if we've already got it and we're not refreshing,
	// return what we've already got
	if (bScrollLinesRequested)
		return uCachedScrollLines;

	// try system settings
	::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &uCachedScrollLines, 0);
	bScrollLinesRequested = true;

	return uCachedScrollLines;
}

/////////////////////////////////////////////////////////////////////////////
// CScrollZoomView painting

void CScrollZoomView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	ASSERT_VALID(pDC);

#ifdef _DEBUG
	if (m_nMapMode == MM_NONE)
	{
		TRACE0("Error: must call SetScrollSizes() or SetScaleToFitSize()");
		TRACE0("\tbefore painting scroll view.\n");
		ASSERT(FALSE);
		return;
	}
#endif //_DEBUG
	ASSERT(m_totalDev.cx >= 0 && m_totalDev.cy >= 0);
	switch (m_nMapMode)
	{
	case MM_SCALETOFIT:
		pDC->SetMapMode(MM_ANISOTROPIC);
		pDC->SetWindowExt(m_totalLog);  // window is in logical coordinates
		pDC->SetViewportExt(m_totalDev);
#ifdef DEBUG
		if (m_totalDev.cx == 0 || m_totalDev.cy == 0)
			TRACE0("Warning: CScrollZoomView scaled to nothing.\n");
#endif
		break;

	case MM_ISOTROPIC:
		pDC->SetMapMode(m_nMapMode);
		pDC->SetWindowExt(100,100);

		{
			int size = MulDiv(m_scalePercent, CClientDC(this).GetDeviceCaps(LOGPIXELSY), 96.0);


			pDC->SetViewportExt(size, size);
		}
		break;
	default:
		ASSERT(m_nMapMode > 0);
		pDC->SetMapMode(m_nMapMode);
		break;
	}

	CPoint ptVpOrg(0, 0);       // assume no shift for printing
	if (!pDC->IsPrinting())
	{
		ASSERT(pDC->GetWindowOrg() == CPoint(0,0));

		// by default shift viewport origin in negative direction of scroll
		ptVpOrg = -GetDeviceScrollPosition();

		if (m_bCenter)
		{
			CRect rect;
			GetClientRect(&rect);

			// if client area is larger than total device size,
			// override scroll positions to place origin such that
			// output is centered in the window
			if (m_totalDev.cx < rect.Width())
				ptVpOrg.x = (rect.Width() - m_totalDev.cx) / 2;
			if (m_totalDev.cy < rect.Height())
				ptVpOrg.y = (rect.Height() - m_totalDev.cy) / 2;
		}
	}
	pDC->SetViewportOrg(ptVpOrg);

	CView::OnPrepareDC(pDC, pInfo);     // For default Printing behavior
}

/////////////////////////////////////////////////////////////////////////////
// Set mode and scaling/scrolling sizes

void CScrollZoomView::SetScaleToFitSize(SIZE sizeTotal)
{
	// Note: It is possible to set sizeTotal members to negative values to
	//  effectively invert either the X or Y axis.

	ASSERT(m_hWnd != NULL && FromHandlePermanent(m_hWnd) != NULL);	// See comment about this at a later FromHandlePermanent
	m_nMapMode = MM_SCALETOFIT;     // special internal value
	m_totalLog = sizeTotal;

	// reset and turn any scroll bars off
	if (m_hWnd != NULL && (GetStyle() & (WS_HSCROLL|WS_VSCROLL)))
	{
		SetScrollPos(SB_HORZ, 0);
		SetScrollPos(SB_VERT, 0);
		EnableScrollBarCtrl(SB_BOTH, FALSE);
		m_noHscroll = true; // hack terge
		m_noVscroll = true; // hack terge
		SetScrollRange(SB_HORZ, 0, 0, TRUE);
		SetScrollRange(SB_VERT, 0, 0, TRUE);
		ASSERT((GetStyle() & (WS_HSCROLL|WS_VSCROLL)) == 0);
	}

	CRect rectT;
	GetClientRect(rectT);
	m_totalDev = rectT.Size();

	if (m_hWnd != NULL && FromHandlePermanent(m_hWnd) != NULL)	// See comment about this at a later FromHandlePermanent
	{
		// window has been created, invalidate
		UpdateBars();
		Invalidate(TRUE);
	}
}

const AFX_DATADEF SIZE CScrollZoomView::sizeDefault = {0,0};

void CScrollZoomView::SetScrollSizes(int nMapMode, SIZE sizeTotal, int scalePercent, 
	const SIZE& sizePage, const SIZE& sizeLine)
{
	ASSERT(sizeTotal.cx >= 0 && sizeTotal.cy >= 0);
	ASSERT(nMapMode > 0);
	ASSERT(nMapMode != MM_ANISOTROPIC);

	int nOldMapMode = m_nMapMode;
	m_nMapMode = nMapMode;
	m_totalLog = sizeTotal;
	m_scalePercent = scalePercent;

	//BLOCK: convert logical coordinate space to device coordinates
	{
		CWindowDC dc(NULL);
		ASSERT(m_nMapMode > 0);
		dc.SetMapMode(m_nMapMode);
		if (m_nMapMode == MM_ISOTROPIC) {
			dc.SetWindowExt(100,100);
			dc.SetViewportExt(scalePercent, scalePercent);

			int size = MulDiv(m_scalePercent, CClientDC(const_cast<CScrollZoomView*>(this)).GetDeviceCaps(LOGPIXELSY), 96.0);

			dc.SetViewportExt(size, size);

		}

		// total size
		m_totalDev = m_totalLog;
		dc.LPtoDP((LPPOINT)&m_totalDev);
		m_pageDev = sizePage;
		dc.LPtoDP((LPPOINT)&m_pageDev);
		m_lineDev = sizeLine;
		dc.LPtoDP((LPPOINT)&m_lineDev);
		if (m_totalDev.cy < 0)
			m_totalDev.cy = -m_totalDev.cy;
		if (m_pageDev.cy < 0)
			m_pageDev.cy = -m_pageDev.cy;
		if (m_lineDev.cy < 0)
			m_lineDev.cy = -m_lineDev.cy;
	} // release DC here

	// now adjust device specific sizes
	ASSERT(m_totalDev.cx >= 0 && m_totalDev.cy >= 0);
	if (m_pageDev.cx == 0)
		m_pageDev.cx = m_totalDev.cx / 10;
	if (m_pageDev.cy == 0)
		m_pageDev.cy = m_totalDev.cy / 10;
	if (m_lineDev.cx == 0)
		m_lineDev.cx = m_pageDev.cx / 10;
	if (m_lineDev.cy == 0)
		m_lineDev.cy = m_pageDev.cy / 10;

	if (m_hWnd != NULL)
	{
		// trick to overcome MFC9.0 AssertValid ASSERT (MFC9.0 became more strict, pickyer, probably this was present before also):
		// sometimes m_hWnd != NULL, but not valid, because HWND is missing from the permanent and the temporary map also,
		// so we basically query pMap->LookupPermanent(m_hWnd) here with the FromHandlePermanent call
		if (FromHandlePermanent(m_hWnd) != NULL) {
			// set scroll position after zoom
			// window has been created, invalidate now
			UpdateBars();
			if (nOldMapMode != m_nMapMode)
				Invalidate(TRUE);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Getting information

CPoint CScrollZoomView::GetScrollPosition() const   // logical coordinates
{
	if (m_nMapMode == MM_SCALETOFIT)
	{
		return CPoint(0, 0);    // must be 0,0
	}

	CPoint pt = GetDeviceScrollPosition();
	// pt may be negative if m_bCenter is set

	if (m_nMapMode != MM_TEXT)
	{
		ASSERT(m_nMapMode > 0); // must be set
		CWindowDC dc(NULL);
		dc.SetMapMode(m_nMapMode);
		if (m_nMapMode == MM_ISOTROPIC) {
			dc.SetWindowExt(100,100);
			dc.SetViewportExt(m_scalePercent, m_scalePercent);

			int size = MulDiv(m_scalePercent, CClientDC(const_cast<CScrollZoomView*>(this)).GetDeviceCaps(LOGPIXELSY), 96.0);

			dc.SetViewportExt(size, size);
		}
		dc.DPtoLP((LPPOINT)&pt);
	}
	return pt;
}

void CScrollZoomView::ScrollToPosition(POINT pt)    // logical coordinates
{
	ASSERT(m_nMapMode > 0);     // not allowed for shrink to fit
	if (m_nMapMode != MM_TEXT)
	{
		CWindowDC dc(NULL);
		dc.SetMapMode(m_nMapMode);
		if (m_nMapMode == MM_ISOTROPIC) {
			dc.SetWindowExt(100,100);
			dc.SetViewportExt(m_scalePercent, m_scalePercent);

			int size = MulDiv(m_scalePercent, CClientDC(const_cast<CScrollZoomView*>(this)).GetDeviceCaps(LOGPIXELSY), 96.0);

			dc.SetViewportExt(size, size);
		}
		dc.LPtoDP((LPPOINT)&pt);
	}

	// now in device coordinates - limit if out of range
	int xMax = GetScrollLimit(SB_HORZ);
	int yMax = GetScrollLimit(SB_VERT);
//	int xMin, xMax, yMin, yMax;
//	GetScrollRange(SB_HORZ, &xMin, &xMax); // it does not work correctly ?? !!
//	GetScrollRange(SB_VERT, &yMin, &yMax);
	if (m_noHscroll)   // hack terge
	{
		xMax = 0;
	}
	if (m_noVscroll)   // hack terge
	{
		yMax = 0;
	}

	// terge ?? szar van

	if (pt.x < 0)
		pt.x = 0;
	else if (pt.x > xMax)
		pt.x = xMax;
	if (pt.y < 0)
		pt.y = 0;
	else if (pt.y > yMax)
		pt.y = yMax;

	ScrollToDevicePosition(pt);
}

CPoint CScrollZoomView::GetDeviceScrollPosition() const
{
	CPoint pt(GetScrollPos(SB_HORZ), GetScrollPos(SB_VERT));
	ASSERT(pt.x >= 0 && pt.y >= 0);

	if (m_bCenter)
	{
		CRect rect;
		GetClientRect(&rect);

		// if client area is larger than total device size,
		// the scroll positions are overridden to place origin such that
		// output is centered in the window
		// GetDeviceScrollPosition() must reflect this

		if (m_totalDev.cx < rect.Width())
			pt.x = -((rect.Width() - m_totalDev.cx) / 2);
		if (m_totalDev.cy < rect.Height())
			pt.y = -((rect.Height() - m_totalDev.cy) / 2);
	}

	return pt;
}

void CScrollZoomView::GetDeviceScrollSizes(int& nMapMode, SIZE& sizeTotal,
			SIZE& sizePage, SIZE& sizeLine) const
{
#ifdef DEBUG
	if (m_nMapMode <= 0)
		TRACE0("Warning: CScrollZoomView::GetDeviceScrollSizes returning invalid mapping mode.\n");
#endif
	nMapMode = m_nMapMode;
	sizeTotal = m_totalDev;
	sizePage = m_pageDev;
	sizeLine = m_lineDev;
}

void CScrollZoomView::notifyPanning(CPoint pt)
{
//	CMainFrame* main = (CMainFrame*)AfxGetMainWnd();
	CMainFrame* main = (CMainFrame*)theApp.m_pMainWnd;

	if (!main || !main->m_panningWindow.IsVisible())
		return;
	CRect client;
	GetClientRect(&client);
	CWindowDC dc(NULL);
	dc.SetMapMode(MM_ISOTROPIC);
	dc.SetWindowExt(100,100);
	dc.SetViewportExt(m_scalePercent, m_scalePercent);

	int size = MulDiv(m_scalePercent, CClientDC(const_cast<CScrollZoomView*>(this)).GetDeviceCaps(LOGPIXELSY), 96.0);

	dc.SetViewportExt(size, size);


	dc.DPtoLP((LPPOINT)&pt);
	dc.DPtoLP(&client);
	CRect pannw(pt.x, pt.y, pt.x + client.Width(), pt.y + client.Height());
	pannw.NormalizeRect();

	main->m_panningWindow.SetViewRect(pannw);
}

void CScrollZoomView::ScrollToDevicePosition(POINT ptDev)
{
	ASSERT(ptDev.x >= 0);
	ASSERT(ptDev.y >= 0);
	int maxh = GetScrollLimit(SB_HORZ);
	int maxv = GetScrollLimit(SB_VERT);
//	int xMin, yMin;
//	GetScrollRange(SB_HORZ, &xMin, &maxh);
//	GetScrollRange(SB_VERT, &yMin, &maxv);
	if (maxh == 0  &&  maxv == 0)
	{
		TRACE("notifyPanning from ScrollToDevicePosition - maxh == 0  &&  maxv == 0\n");
		notifyPanning(ptDev);
		return;
	}

	// Note: ScrollToDevicePosition can and is used to scroll out-of-range
	//  areas as far as CScrollZoomView is concerned -- specifically in
	//  the print-preview code.  Since OnScrollBy makes sure the range is
	//  valid, ScrollToDevicePosition does not vector through OnScrollBy.

	int xOrig = GetScrollPos(SB_HORZ);
	SetScrollPos(SB_HORZ, ptDev.x);
	int yOrig = GetScrollPos(SB_VERT);
	SetScrollPos(SB_VERT, ptDev.y);
	ptDev.x = (maxh<ptDev.x)? maxh: ptDev.x; 
	ptDev.y = (maxv<ptDev.y)? maxv: ptDev.y;
	TRACE("notifyPanning from ScrollToDevicePosition\n");
	notifyPanning(ptDev);
//	ScrollWindow(xOrig - ptDev.x, yOrig - ptDev.y);
	ScrollWindow((ptDev.x - xOrig), (ptDev.y - yOrig) );
}

/////////////////////////////////////////////////////////////////////////////
// Other helpers

void CScrollZoomView::FillOutsideRect(CDC* pDC, CBrush* pBrush)
{
	ASSERT_VALID(pDC);
	ASSERT_VALID(pBrush);

	// fill rect outside the image
	CRect rect;
	GetClientRect(rect);
	ASSERT(rect.left == 0 && rect.top == 0);
	rect.left = m_totalDev.cx;
	if (!rect.IsRectEmpty())
		pDC->FillRect(rect, pBrush);    // vertical strip along the side
	rect.left = 0;
	rect.right = m_totalDev.cx;
	rect.top = m_totalDev.cy;
	if (!rect.IsRectEmpty())
		pDC->FillRect(rect, pBrush);    // horizontal strip along the bottom
}

void CScrollZoomView::ResizeParentToFit(BOOL bShrinkOnly)
{
	// adjust parent rect so client rect is appropriate size
	ASSERT(m_nMapMode != MM_NONE);  // mapping mode must be known

	// determine current size of the client area as if no scrollbars present
	CRect rectClient;
	GetWindowRect(rectClient);
	CRect rect = rectClient;
	CalcWindowRect(rect);
	rectClient.left += rectClient.left - rect.left;
	rectClient.top += rectClient.top - rect.top;
	rectClient.right -= rect.right - rectClient.right;
	rectClient.bottom -= rect.bottom - rectClient.bottom;
	rectClient.OffsetRect(-rectClient.left, -rectClient.top);
	ASSERT(rectClient.left == 0 && rectClient.top == 0);

	// determine desired size of the view
	CRect rectView(0, 0, m_totalDev.cx, m_totalDev.cy);
	if (bShrinkOnly)
	{
		if (rectClient.right <= m_totalDev.cx)
			rectView.right = rectClient.right;
		if (rectClient.bottom <= m_totalDev.cy)
			rectView.bottom = rectClient.bottom;
	}
	CalcWindowRect(rectView, CWnd::adjustOutside);
	rectView.OffsetRect(-rectView.left, -rectView.top);
	ASSERT(rectView.left == 0 && rectView.top == 0);
	if (bShrinkOnly)
	{
		if (rectClient.right <= m_totalDev.cx)
			rectView.right = rectClient.right;
		if (rectClient.bottom <= m_totalDev.cy)
			rectView.bottom = rectClient.bottom;
	}

	// dermine and set size of frame based on desired size of view
	CRect rectFrame;
	CFrameWnd* pFrame = GetParentFrame();
	ASSERT_VALID(pFrame);
	pFrame->GetWindowRect(rectFrame);
	CSize size = rectFrame.Size();
	size.cx += rectView.right - rectClient.right;
	size.cy += rectView.bottom - rectClient.bottom;
	pFrame->SetWindowPos(NULL, 0, 0, size.cx, size.cy,
		SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
}

/////////////////////////////////////////////////////////////////////////////

void CScrollZoomView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	if (m_nMapMode == MM_SCALETOFIT)
	{
		// force recalculation of scale to fit parameters
		SetScaleToFitSize(m_totalLog);
	}
	else
	{
		// UpdateBars() handles locking out recursion
		UpdateBars();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Scrolling Helpers

void CScrollZoomView::CenterOnPoint(CPoint ptCenter) // center in device coords
{
	CRect rect;
	GetClientRect(&rect);           // find size of client window

	int xDesired = ptCenter.x - rect.Width() / 2;
	int yDesired = ptCenter.y - rect.Height() / 2;

	DWORD dwStyle = GetStyle();

	if ((dwStyle & WS_HSCROLL) == 0 || xDesired < 0)
	{
		xDesired = 0;
	}
	else
	{
//		int xMin, xMax;
//		GetScrollRange(SB_HORZ, &xMin, &xMax);
		int xMax = GetScrollLimit(SB_HORZ);
		if (xDesired > xMax)
			xDesired = xMax;
	}

	if ((dwStyle & WS_VSCROLL) == 0 || yDesired < 0)
	{
		yDesired = 0;
	}
	else
	{
//		int yMin, yMax;
//		GetScrollRange(SB_VERT, &yMin, &yMax);
		int yMax = GetScrollLimit(SB_VERT);
		if (yDesired > yMax)
			yDesired = yMax;
	}

	ASSERT(xDesired >= 0);
	ASSERT(yDesired >= 0);

	SetScrollPos(SB_HORZ, xDesired);
	SetScrollPos(SB_VERT, yDesired);
}

/////////////////////////////////////////////////////////////////////////////
// Tie to scrollbars and CWnd behaviour

void CScrollZoomView::GetScrollBarSizes(CSize& sizeSb)
{
	sizeSb.cx = sizeSb.cy = 0;
	DWORD dwStyle = GetStyle();

	if (GetScrollBarCtrl(SB_VERT) == NULL)
	{
		// vert scrollbars will impact client area of this window
		sizeSb.cx = GetSystemMetrics(SM_CXVSCROLL) + CX_BORDER;
		if (dwStyle & WS_BORDER)
			sizeSb.cx -= CX_BORDER;
	}
	if (GetScrollBarCtrl(SB_HORZ) == NULL)
	{
		// horz scrollbars will impact client area of this window
		sizeSb.cy = GetSystemMetrics(SM_CYHSCROLL) + CY_BORDER;
		if (dwStyle & WS_BORDER)
			sizeSb.cy -= CY_BORDER;
	}
}

BOOL CScrollZoomView::GetTrueClientSize(CSize& size, CSize& sizeSb)
	// return TRUE if enough room to add scrollbars if needed
{
	CRect rect;
	GetClientRect(&rect);
	ASSERT(rect.top == 0 && rect.left == 0);
	size.cx = rect.right;
	size.cy = rect.bottom;
	DWORD dwStyle = GetStyle();

	// first get the size of the scrollbars for this window
	GetScrollBarSizes(sizeSb);

	// first calculate the size of a potential scrollbar
	// (scroll bar controls do not get turned on/off)
	if (sizeSb.cx != 0 && (dwStyle & WS_VSCROLL))
	{
 	 	// vert scrollbars will impact client area of this window
		size.cx += sizeSb.cx;   // currently on - adjust now
	}
	if (sizeSb.cy != 0 && (dwStyle & WS_HSCROLL))
	{
		// horz scrollbars will impact client area of this window
		size.cy += sizeSb.cy;   // currently on - adjust now
	}

	// return TRUE if enough room
	return (size.cx > sizeSb.cx && size.cy > sizeSb.cy);
}

// helper to return the state of the scrollbars without actually changing
//  the state of the scrollbars
void CScrollZoomView::GetScrollBarState(CSize sizeClient, CSize& needSb,
	CSize& sizeRange, CPoint& ptMove, BOOL bInsideClient)
{
	// get scroll bar sizes (the part that is in the client area)
	CSize sizeSb;
	GetScrollBarSizes(sizeSb);

	// enough room to add scrollbars
	sizeRange = m_totalDev - sizeClient;
		// > 0 => need to scroll
	ptMove = GetDeviceScrollPosition();
		// point to move to (start at current scroll pos)

	BOOL bNeedH = sizeRange.cx > 0;
	if (!bNeedH)
		ptMove.x = 0;                       // jump back to origin
	else if (bInsideClient)
		sizeRange.cy += sizeSb.cy;          // need room for a scroll bar

	BOOL bNeedV = sizeRange.cy > 0;
	if (!bNeedV)
		ptMove.y = 0;                       // jump back to origin
	else if (bInsideClient)
		sizeRange.cx += sizeSb.cx;          // need room for a scroll bar

	if (bNeedV && !bNeedH && sizeRange.cx > 0)
	{
		ASSERT(bInsideClient);
		// need a horizontal scrollbar after all
		bNeedH = TRUE;
		sizeRange.cy += sizeSb.cy;
	}

	// if current scroll position will be past the limit, scroll to limit
	if (sizeRange.cx > 0 && ptMove.x >= sizeRange.cx)
		ptMove.x = sizeRange.cx;
	if (sizeRange.cy > 0 && ptMove.y >= sizeRange.cy)
		ptMove.y = sizeRange.cy;

	// now update the bars as appropriate
	needSb.cx = bNeedH;
	needSb.cy = bNeedV;

	// needSb, sizeRange, and ptMove area now all updated
}

void CScrollZoomView::UpdateBars()
{
	// UpdateBars may cause window to be resized - ignore those resizings
	if (m_bInsideUpdate)
		return;         // Do not allow recursive calls

	// Lock out recursion
	m_bInsideUpdate = TRUE;

	// update the horizontal to reflect reality
	// NOTE: turning on/off the scrollbars will cause 'OnSize' callbacks
	ASSERT(m_totalDev.cx >= 0 && m_totalDev.cy >= 0);

	CRect rectClient;
	BOOL bCalcClient = TRUE;

	// allow parent to do inside-out layout first
	CWnd* pParentWnd = GetParent();
	if (pParentWnd != NULL)
	{
		// if parent window responds to this message, use just
		//  client area for scroll bar calc -- not "true" client area
		if ((BOOL)pParentWnd->SendMessage(WM_RECALCPARENT, 0,
			(LPARAM)(LPCRECT)&rectClient) != 0)
		{
			// use rectClient instead of GetTrueClientSize for
			//  client size calculation.
			bCalcClient = FALSE;
		}
	}

	CSize sizeClient;
	CSize sizeSb;

	if (bCalcClient)
	{
		// get client rect
		if (!GetTrueClientSize(sizeClient, sizeSb))
		{
			// no room for scroll bars (common for zero sized elements)
			CRect rect;
			GetClientRect(&rect);
			if (rect.right > 0 && rect.bottom > 0)
			{
				// if entire client area is not invisible, assume we have
				//  control over our scrollbars
				SetScrollPos(SB_HORZ, 0);
				SetScrollPos(SB_VERT, 0);
				EnableScrollBarCtrl(SB_BOTH, FALSE);
				m_noHscroll = true; // hack terge
				m_noVscroll = true; // hack terge
				SetScrollRange(SB_HORZ, 0, 0, TRUE);
				SetScrollRange(SB_VERT, 0, 0, TRUE);
			}
			m_bInsideUpdate = FALSE;
//			notifyPanning(CPoint(0,0)); // ??
			return;
		}
	}
	else
	{
		// let parent window determine the "client" rect
		GetScrollBarSizes(sizeSb);
		sizeClient.cx = rectClient.right - rectClient.left;
		sizeClient.cy = rectClient.bottom - rectClient.top;
	}

	// enough room to add scrollbars
	CSize sizeRange;
	CPoint ptMove;
	CSize needSb;

	// get the current scroll bar state given the true client area
	GetScrollBarState(sizeClient, needSb, sizeRange, ptMove, bCalcClient);
	if (needSb.cx)
		sizeClient.cy -= sizeSb.cy;
	if (needSb.cy)
		sizeClient.cx -= sizeSb.cx;

	// first scroll the window as needed
	if (!m_zoomScroll)
	{
		ScrollToDevicePosition(ptMove); // will set the scroll bar positions too
	}

	// this structure needed to update the scrollbar page range
	SCROLLINFO info;
	info.fMask = SIF_PAGE|SIF_RANGE;
	info.nMin = 0;

	// now update the bars as appropriate
	EnableScrollBarCtrl(SB_HORZ, needSb.cx);
	if (needSb.cx)
	{
		m_noHscroll = false; // hack terge
		info.nPage = sizeClient.cx;
		info.nMax = m_totalDev.cx-1;
		SetScrollInfo(SB_HORZ, &info, TRUE);
	}
	else
	{
		m_noHscroll = true; // hack terge
//		DWORD err = 0;
//		BOOL ret = TRUE;
		SetScrollPos(SB_HORZ, 0);
		SetScrollRange(SB_HORZ, 0, 0, TRUE);
/*		CScrollBar* pScrollBar;
		if ((pScrollBar = GetScrollBarCtrl(SB_HORZ)) != NULL)
			pScrollBar->SetScrollRange(0, 0, TRUE);
		else
		{
			ret = ::SetScrollRange(m_hWnd, SB_HORZ, 0, 0, TRUE);
			if (!ret)
				err = GetLastError();
		}*/
//		int lxMin, lxMax;
//		GetScrollRange(SB_HORZ, &lxMin, &lxMax);
	}
	EnableScrollBarCtrl(SB_VERT, needSb.cy);
//	int ixMin, ixMax;
//	GetScrollRange(SB_HORZ, &ixMin, &ixMax);
	if (needSb.cy)
	{
		m_noVscroll = false; // hack terge
		info.nPage = sizeClient.cy;
		info.nMax = m_totalDev.cy-1;
		SetScrollInfo(SB_VERT, &info, TRUE);
			//SetScrollRange(SB_VERT, 0, sizeRange.cy, TRUE);
	}
	else
	{
		m_noVscroll = true; // hack terge
		SetScrollPos(SB_VERT, 0);
//		int jxMin, jxMax;
//		GetScrollRange(SB_HORZ, &jxMin, &jxMax);
		SetScrollRange(SB_VERT, 0, 0, TRUE);
	}
//	int pxMin, pxMax, pyMin, pyMax;
///	GetScrollRange(SB_HORZ, &pxMin, &pxMax); // pxMax == 100 after setting it 0 - framework error
//	GetScrollRange(SB_VERT, &pyMin, &pyMax);
	if (m_zoomScroll)
	{
		ScrollToPosition(m_zoomP);    // logical coordinates
		m_zoomP.x = m_zoomP.y = 0;
		m_zoomScroll = false;
	}

	// remove recursion lockout
	m_bInsideUpdate = FALSE;
}

void CScrollZoomView::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType)
{
	if (nAdjustType == adjustOutside)
	{
		// allow for special client-edge style
		::AdjustWindowRectEx(lpClientRect, 0, FALSE, GetExStyle());

		if (m_nMapMode != MM_SCALETOFIT)
		{
			// if the view is being used in-place, add scrollbar sizes
			//  (scollbars should appear on the outside when in-place editing)
			CSize sizeClient(
				lpClientRect->right - lpClientRect->left,
				lpClientRect->bottom - lpClientRect->top);

			CSize sizeRange = m_totalDev - sizeClient;
				// > 0 => need to scroll

			// get scroll bar sizes (used to adjust the window)
			CSize sizeSb;
			GetScrollBarSizes(sizeSb);

			// adjust the window size based on the state
			if (sizeRange.cy > 0)
			{   // vertical scroll bars take up horizontal space
				lpClientRect->right += sizeSb.cx;
			}
			if (sizeRange.cx > 0)
			{   // horizontal scroll bars take up vertical space
				lpClientRect->bottom += sizeSb.cy;
			}
		}
	}
	else
	{
		// call default to handle other non-client areas
		::AdjustWindowRectEx(lpClientRect, GetStyle(), FALSE,
			GetExStyle() & ~(WS_EX_CLIENTEDGE));
	}
}

/////////////////////////////////////////////////////////////////////////////
// CScrollZoomView scrolling

void CScrollZoomView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar != NULL && pScrollBar->SendChildNotifyLastMsg())
		return;     // eat it

	// ignore scroll bar msgs from other controls
	if (pScrollBar != NULL  &&  pScrollBar != GetScrollBarCtrl(SB_HORZ))
		return;
	//  GetScrollInfo http://msdn.microsoft.com/en-us/library/bb787583%28VS.85%29.aspx
	// "The messages that indicate scroll bar position, WM_HSCROLL and WM_VSCROLL, provide only 16 bits of position data"
	if (nSBCode == SB_THUMBTRACK || nSBCode == SB_THUMBPOSITION)
	{
		SCROLLINFO si = {0};
		if (!GetScrollInfo(SB_HORZ, &si, SIF_TRACKPOS))
		{
			ASSERT(false);
			return;
		}
		nPos = si.nTrackPos;
	}

	OnScroll(MAKEWORD(nSBCode, -1), nPos);
}

void CScrollZoomView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CView::OnVScroll(nSBCode, nPos, pScrollBar);
	if (pScrollBar != NULL && pScrollBar->SendChildNotifyLastMsg())
		return;     // eat it

	// ignore scroll bar msgs from other controls
	if (pScrollBar != NULL  &&  pScrollBar != GetScrollBarCtrl(SB_VERT))
		return;
	//  GetScrollInfo http://msdn.microsoft.com/en-us/library/bb787583%28VS.85%29.aspx
	// "The messages that indicate scroll bar position, WM_HSCROLL and WM_VSCROLL, provide only 16 bits of position data"
	if (nSBCode == SB_THUMBTRACK || nSBCode == SB_THUMBPOSITION)
	{
		SCROLLINFO si = {0};
		if (!GetScrollInfo(SB_VERT, &si, SIF_TRACKPOS))
		{
			ASSERT(false);
			return;
		}
		nPos = si.nTrackPos;
	}

	OnScroll(MAKEWORD(-1, nSBCode), nPos);
}

BOOL CScrollZoomView::OnMouseWheel(UINT fFlags, short zDelta, CPoint point)
{
	// we don't handle anything but scrolling just now
	if (fFlags & MK_CONTROL)
		return FALSE;

	// if the parent is a splitter, it will handle the message
	if (GetParentSplitter(this, TRUE))
		return FALSE;

	// we can't get out of it--perform the scroll ourselves
	return DoMouseWheel(fFlags, zDelta, point);
}

// This function isn't virtual. If you need to override it,
// you really need to override OnMouseWheel() here or in
// CSplitterWnd.

BOOL CScrollZoomView::DoMouseWheel(UINT fFlags, short zDelta, CPoint point)
{
	UNUSED_ALWAYS(point);
	UNUSED_ALWAYS(fFlags);

	// if we have a vertical scroll bar, the wheel scrolls that
	// if we have _only_ a horizontal scroll bar, the wheel scrolls that
	// otherwise, don't do any work at all

	DWORD dwStyle = GetStyle();
	CScrollBar* pBar = GetScrollBarCtrl(SB_VERT);
	BOOL bHasVertBar = ((pBar != NULL) && pBar->IsWindowEnabled()) ||
					(dwStyle & WS_VSCROLL);

	pBar = GetScrollBarCtrl(SB_HORZ);
	BOOL bHasHorzBar = ((pBar != NULL) && pBar->IsWindowEnabled()) ||
					(dwStyle & WS_HSCROLL);

	if (!bHasVertBar && !bHasHorzBar)
		return FALSE;

	bool preferHorizZoom = (fFlags & MK_SHIFT) != 0;

	BOOL bResult = FALSE;
	UINT uWheelScrollLines = GetMouseScrollLines();
	int nToScroll;
	int nDisplacement;

	if (bHasVertBar && !(bHasHorzBar && preferHorizZoom))
	{
		nToScroll = ::MulDiv(-zDelta, uWheelScrollLines, WHEEL_DELTA);
		if (uWheelScrollLines == WHEEL_PAGESCROLL)
		{
			nDisplacement = m_pageDev.cy;
			if (zDelta > 0)
				nDisplacement = -nDisplacement;
		}
		else
		{
			nDisplacement = nToScroll * m_lineDev.cy;
			nDisplacement = (nDisplacement<m_pageDev.cy)? nDisplacement: m_pageDev.cy;
		}
		bResult = OnScrollBy(CSize(0, nDisplacement), TRUE);
	}
	else if (bHasHorzBar)
	{
		nToScroll = ::MulDiv(-zDelta, uWheelScrollLines, WHEEL_DELTA);
		if (uWheelScrollLines == WHEEL_PAGESCROLL)
		{
			nDisplacement = m_pageDev.cx;
			if (zDelta > 0)
				nDisplacement = -nDisplacement;
		}
		else
		{
			nDisplacement = nToScroll * m_lineDev.cx;
			nDisplacement = (nDisplacement<m_pageDev.cx)? nDisplacement: m_pageDev.cx;
		}
		bResult = OnScrollBy(CSize(nDisplacement, 0), TRUE);
	}

	if (bResult)
		UpdateWindow();

	return bResult;
}


BOOL CScrollZoomView::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll)
{
	// calc new x position
	int x = GetScrollPos(SB_HORZ);
	int xOrig = x;

	switch (LOBYTE(nScrollCode))
	{
	case SB_TOP:
		x = 0;
		break;
	case SB_BOTTOM:
		x = INT_MAX;
		break;
	case SB_LINEUP:
		x -= m_lineDev.cx;
		break;
	case SB_LINEDOWN:
		x += m_lineDev.cx;
		break;
	case SB_PAGEUP:
		x -= m_pageDev.cx;
		break;
	case SB_PAGEDOWN:
		x += m_pageDev.cx;
		break;
	case SB_THUMBTRACK:
		x = nPos;
		break;
	}

	// calc new y position
	int y = GetScrollPos(SB_VERT);
	int yOrig = y;

	switch (HIBYTE(nScrollCode))
	{
	case SB_TOP:
		y = 0;
		break;
	case SB_BOTTOM:
		y = INT_MAX;
		break;
	case SB_LINEUP:
		y -= m_lineDev.cy;
		break;
	case SB_LINEDOWN:
		y += m_lineDev.cy;
		break;
	case SB_PAGEUP:
		y -= m_pageDev.cy;
		break;
	case SB_PAGEDOWN:
		y += m_pageDev.cy;
		break;
	case SB_THUMBTRACK:
		y = nPos;
		break;
	}

	BOOL bResult = OnScrollBy(CSize(x - xOrig, y - yOrig), bDoScroll);
	if (bResult && bDoScroll)
		UpdateWindow();

	return bResult;
}

BOOL CScrollZoomView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	int xOrig, x;
	int yOrig, y;

	// don't scroll if there is no valid scroll range (ie. no scroll bar)
	CScrollBar* pBar;
	DWORD dwStyle = GetStyle();
	pBar = GetScrollBarCtrl(SB_VERT);
	if ((pBar != NULL && !pBar->IsWindowEnabled()) ||
		(pBar == NULL && !(dwStyle & WS_VSCROLL)))
	{
		// vertical scroll bar not enabled
		sizeScroll.cy = 0;
	}
	pBar = GetScrollBarCtrl(SB_HORZ);
	if ((pBar != NULL && !pBar->IsWindowEnabled()) ||
		(pBar == NULL && !(dwStyle & WS_HSCROLL)))
	{
		// horizontal scroll bar not enabled
		sizeScroll.cx = 0;
	}

	// adjust current x position
	xOrig = x = GetScrollPos(SB_HORZ);
//	int xMax, xMin;
//	GetScrollRange(SB_HORZ, &xMin, &xMax);
	int xMax = GetScrollLimit(SB_HORZ);
	x += sizeScroll.cx;
	if (x < 0)
		x = 0;
	else if (x > xMax)
		x = xMax;

	// adjust current y position
	yOrig = y = GetScrollPos(SB_VERT);
//	int yMin, yMax;
//	GetScrollRange(SB_VERT, &yMin, &yMax);
	int yMax = GetScrollLimit(SB_VERT);
	y += sizeScroll.cy;
	if (y < 0)
		y = 0;
	else if (y > yMax)
		y = yMax;

	// did anything change?
	if (x == xOrig && y == yOrig)
		return FALSE;

	if (bDoScroll)
	{
		// do scroll and update scroll positions
		ScrollWindow(-(x-xOrig), -(y-yOrig));
		if (x != xOrig)
			SetScrollPos(SB_HORZ, x);
		if (y != yOrig)
			SetScrollPos(SB_VERT, y);
	}

	// terge
	TRACE("notifyPanning from OnScrollBy\n");
	notifyPanning(GetDeviceScrollPosition());

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CScrollZoomView diagnostics

#ifdef _DEBUG
void CScrollZoomView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);

	dc << "m_totalLog = " << m_totalLog;
	dc << "\nm_totalDev = " << m_totalDev;
	dc << "\nm_pageDev = " << m_pageDev;
	dc << "\nm_lineDev = " << m_lineDev;
	dc << "\nm_bCenter = " << m_bCenter;
	dc << "\nm_bInsideUpdate = " << m_bInsideUpdate;
	dc << "\nm_nMapMode = ";
	switch (m_nMapMode)
	{
	case MM_NONE:
		dc << "MM_NONE";
		break;
	case MM_SCALETOFIT:
		dc << "MM_SCALETOFIT";
		break;
	case MM_TEXT:
		dc << "MM_TEXT";
		break;
	case MM_LOMETRIC:
		dc << "MM_LOMETRIC";
		break;
	case MM_HIMETRIC:
		dc << "MM_HIMETRIC";
		break;
	case MM_LOENGLISH:
		dc << "MM_LOENGLISH";
		break;
	case MM_HIENGLISH:
		dc << "MM_HIENGLISH";
		break;
	case MM_TWIPS:
		dc << "MM_TWIPS";
		break;
	case MM_ISOTROPIC:
		dc << "MM_ISOTROPIC";
		break;
	default:
		dc << "*unknown*";
		break;
	}

	dc << "\n";
}

void CScrollZoomView::AssertValid() const
{
	CView::AssertValid();

	switch (m_nMapMode)
	{
	case MM_NONE:
	case MM_SCALETOFIT:
	case MM_TEXT:
	case MM_LOMETRIC:
	case MM_HIMETRIC:
	case MM_LOENGLISH:
	case MM_HIENGLISH:
	case MM_TWIPS:
	case MM_ISOTROPIC:
		break;
	case MM_ANISOTROPIC:
		ASSERT(FALSE); // illegal mapping mode
	default:
		ASSERT(FALSE); // unknown mapping mode
	}
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////

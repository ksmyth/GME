// GMEView.cpp : implementation of the CGMEView class
//

#include "stdafx.h"
#include "GMEApp.h"
#include <math.h>
#include <algorithm>
#include <deque>
#include <new>
#include <sstream>
#include "GMEstd.h"

#include "GuiMeta.h"
#include "GMEDoc.h"
#include "PendingObjectPosRequest.h"
#include "GMEView.h"
#include "ChildFrm.h"
#include "ModelGrid.h"
#include "Mainfrm.h"
#include "GMEBrowser.h"
#include "GMEEventLogger.h"
#include "GmePrintDialog.h"
#include "ConnityDlg.h"
#if defined(ADDCRASHTESTMENU)
#include "CrashTest.h"
#endif

#include "Autoroute/AutoRouter.h"

#include "GraphicsUtil.h"

#include "GMEOLEModel.h"

#include "Autoroute/AutoRouterGraph.h"	// just for testing auto router performance
#include <sys/timeb.h>

// The following is for Xerces, for DumpModelGeometryXML
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/util/IOException.hpp>
#if defined(XERCES_NEW_IOSTREAMS)
#include <iostream>
#else
#include <iostream.h>
#endif
#include <xercesc/util/OutOfMemoryException.hpp>

#undef min
#undef max

CGraphics graphics;
static CViewList viewsToKill;

class CRectTrackerDpi : public CRectTracker {
public:
	void Draw(CDC* pDC) const
{
	// set initial DC state
	VERIFY(pDC->SaveDC() != 0);

	// get normalized rectangle
	CRect rect = m_rect;
	rect.NormalizeRect();
	int yRes = ::GetDeviceCaps(*pDC, LOGPIXELSY);

	CPen* pOldPen = NULL;
	CBrush* pOldBrush = NULL;
	int nOldROP;

	// draw lines
	CPen pen;
	if ((m_nStyle & (dottedLine | solidLine)) != 0)
	{
		LOGBRUSH brush;
		brush.lbStyle = BS_SOLID;
		brush.lbColor = RGB(0, 0, 0);
		brush.lbHatch = 0;
		int width = MulDiv(1, yRes, 96);

		pen.Attach(ExtCreatePen(PS_GEOMETRIC | ((m_nStyle & dottedLine) ? PS_DOT : PS_SOLID), width,
			&brush, 0, nullptr));
		pOldPen = (CPen*)pDC->SelectObject(pen);
		pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);
		nOldROP = pDC->SetROP2(R2_COPYPEN);
		rect.InflateRect(+1, +1);   // borders are one pixel outside
		pDC->Rectangle(rect.left, rect.top, rect.right, rect.bottom);
		pDC->SetROP2(nOldROP);
	}

	// draw resize handles
	if ((m_nStyle & (resizeInside | resizeOutside)) != 0)
	{
		UINT mask = GetHandleMask();
		for (int i = 0; i < 8; ++i)
		{
			if (mask & (1 << i))
			{
				GetHandleRect((TrackerHit)i, &rect);
				pDC->FillSolidRect(rect, RGB(0, 0, 0));
			}
		}
	}

	// cleanup pDC state
	if (pOldPen != NULL)
		pDC->SelectObject(pOldPen);
	if (pOldBrush != NULL)
		pDC->SelectObject(pOldBrush);
	VERIFY(pDC->RestoreDC(-1));
}
};

// CSGUIInterop.cpp
void MoveReferenceWithRefportConnectionsAndWriteToConsole(IMgaFCO* target, IMgaReference* ref);

#define MIN_ZOOM_RECT 12 // the minimal size of zoomable rectangle
/*
int setZoomPercents[GME_ZOOM_LEVEL_NUM] = {
	GME_ZOOM_LEVEL_0,
	GME_ZOOM_LEVEL_1,
	GME_ZOOM_LEVEL_2,
	GME_ZOOM_LEVEL_3,
	GME_ZOOM_LEVEL_4,
	GME_ZOOM_LEVEL_5,
	GME_ZOOM_LEVEL_6,
	GME_ZOOM_LEVEL_7
};*/

// #define END_SCROLL_OFFSET 50 // not used - instead EXTENT_ERROR_CORR

/////////////////////////////////////////////////////////////////////////////
// CViewDriver
bool CViewDriver::attrNeedsRefresh = false;

static int CountDeriveds(IMgaFCOPtr archetype)
{
	int nDerived = 0;
	std::deque<IMgaFCOsPtr> derivedQueue;
	derivedQueue.push_back(archetype->DerivedObjects);
	while (derivedQueue.size())
	{
		IMgaFCOsPtr deriveds = derivedQueue.front();
		derivedQueue.pop_front();
		nDerived += deriveds->Count;
		MGACOLL_ITERATE(IMgaFCO, deriveds)
		{
			if (MGACOLL_ITER->Status == OBJECT_EXISTS)
			{
				derivedQueue.push_back(MGACOLL_ITER->DerivedObjects);
			}
			else
			{
				nDerived--;
			}
		}
		MGACOLL_ITERATE_END
	}
	return nDerived;
}

static wchar_t* ObjTypeName[] = {
	L"Null",
	L"Model",
	L"Atom",
	L"Reference",
	L"Connection",
	L"Set",
	L"Folder",
	L"Aspect",
	L"Role",
	L"Attribute",
	L"Part",
};

STDMETHODIMP CViewDriver::GlobalEvent(globalevent_enum event)
{
	POSITION ppos;

	if(view == 0)
		return S_OK;
	if (view->executingPendingRequests)
		return S_OK;

	CGMEView::inEventHandler = true;
	switch(event) {
	case GLOBALEVENT_UNDO:
	case GLOBALEVENT_REDO:
		ppos = view->pendingRequests.GetHeadPosition();
		while (ppos)
			delete view->pendingRequests.GetNext(ppos);
		view->pendingRequests.RemoveAll();

		// PETER: Let's reset only the view....
		VERIFY(view);
		long status;
		COMTHROW(view->currentModel->get_Status(&status));
		if (status != OBJECT_EXISTS) {
			view->alive = false;
			view->frame->SetSendEvent(false);
			view->frame->PostMessage(WM_CLOSE);
		}
		if (view->alive) {
			view->Reset(true);
			view->needsReset = false;
		}
		// view->GetDocument()->ResetAllViews();
		// PETER END
		break;
    case GLOBALEVENT_ABORT_TRANSACTION:
        view->Reset(true);
        break;    
	case GLOBALEVENT_NOTIFICATION_READY: 
		VERIFY(view);
		POSITION pos = viewsToKill.GetHeadPosition();
		while(pos) {
			CGMEView *v = viewsToKill.GetNext(pos);
			v->alive = false;
			v->frame->SetSendEvent(false);
			v->frame->PostMessage(WM_CLOSE);
		}
		viewsToKill.RemoveAll();
		if(view->alive && view->needsReset) {
			view->Reset(true); // FIXME KMS: maybe post a message instead?
			view->needsReset = false;
		}
		if(attrNeedsRefresh) {
			attrNeedsRefresh = false;
		}
		break;        
	}
	CGMEView::inEventHandler = false;
	return S_OK;
}

STDMETHODIMP CViewDriver::ObjectEvent(IMgaObject *obj, unsigned long eventmask,VARIANT /*v*/)
{
	if(view == 0)
		return S_OK;
	CGMEView::inEventHandler = true;

	// Clear all invalidated PendingRequests
	POSITION ppos = view->pendingRequests.GetHeadPosition();
	while (ppos) {
		POSITION tmp = ppos;
		CPendingObjectPosRequest *req = dynamic_cast<CPendingObjectPosRequest *> (view->pendingRequests.GetNext(ppos));
		if (req) {
			if ( req->object->mgaFco == obj ) {
				view->pendingRequests.RemoveAt(tmp);
				delete req;
			}
		}
	}

	if(IsEqualObject(obj,view->currentModel)) {
		// PARENT EVENT!!!
		if(eventmask & OBJEVENT_DESTROYED) {
			if( theApp.isHistoryEnabled() && view && view->GetDocument())
			{
				CComBSTR id; 
				COMTHROW( obj->get_ID( &id)); // get the id of the deleted object
				view->GetDocument()->eraseFromHistory( PutInCString( id)); // clear from history
			}
			TRACE(_T("   OBJEVENT_DESTROYED\n"));
			viewsToKill.AddTail(view);
			attrNeedsRefresh = true;
		}
		if(eventmask & OBJEVENT_NEWCHILD) {
			TRACE(_T("   OBJEVENT_NEWCHILD\n"));
			view->needsReset = true;
		}
		if(eventmask & OBJEVENT_LOSTCHILD) {
			TRACE(_T("   OBJEVENT_LOSTCHILD\n"));
			view->needsReset = true;
		}
		if(eventmask & OBJEVENT_REGISTRY) {
			view->needsReset = true;
		}
		if(eventmask & OBJEVENT_PROPERTIES) {
			TRACE(_T("   OBJEVENT_PROPERTIES\n"));
			view->SetName();
			attrNeedsRefresh = true;
		}
	}
	else if(IsEqualObject(obj,view->baseType)) {
		if(eventmask & OBJEVENT_PROPERTIES) {
			TRACE(_T("   OBJEVENT_PROPERTIES\n"));
			view->SetTypeNameProperty();
		}
	}
	else {
		// CHILD EVENT!!!
		if(eventmask & OBJEVENT_CREATED) {
			view->needsReset = true;
			TRACE(_T("   OBJEVENT_CREATED\n"));
		}
		else if(eventmask & OBJEVENT_DESTROYED) {
			view->needsReset = true;
			attrNeedsRefresh = true;
			TRACE(_T("   OBJEVENT_DESTROYED\n"));
		}
		else if(eventmask & OBJEVENT_SETINCLUDED || eventmask & OBJEVENT_SETEXCLUDED) {
			view->needsReset = true;
		}
		else if(eventmask & OBJEVENT_RELATION) {
			view->needsReset = true;
		}
		else if(eventmask & OBJEVENT_REGISTRY) {
			view->needsReset = true;
		}
		else if(eventmask & OBJEVENT_ATTR) {
			view->needsReset = true;
			attrNeedsRefresh = true;
		}
		else if(eventmask & OBJEVENT_PROPERTIES) {	   	// Because of the connectionlabels
			view->needsReset = true;					// and attr browser
			attrNeedsRefresh = true;
		}
		else {
			CComPtr<IMgaObject> object = obj;
			CComPtr<IMgaFCO> fco;
			if(SUCCEEDED(object.QueryInterface(&fco))) {
				CGuiObject *guiObj = CGuiFco::FindObject(fco,view->children);
				CGuiConnection *conn = CGuiFco::FindConnection(fco,view->connections);
				if(guiObj) {
					if(eventmask & OBJEVENT_PROPERTIES) {
						COMTHROW(object->get_Name(PutOut(guiObj->GetName())));
						view->Invalidate();
					}
				}
				else if(conn) {
					if(eventmask & OBJEVENT_PROPERTIES) {
						COMTHROW(object->get_Name(PutOut(conn->name)));
						// ?? 
						view->Invalidate();
					}
				}
				else
					view->needsReset = true; // e.g. port name change etc.
			}
		}
	}
	CGMEView::inEventHandler = false;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CGMEView

int CGMEView::offScreenCreated = 0;
CDC * CGMEView::offScreen;
CBitmap *CGMEView::ofsbmp;
int CGMEView::instanceCount = 0;
int CGMEView::inTransaction = 0;
bool CGMEView::inRWTransaction = false;
bool CGMEView::inEventHandler = false;


HCURSOR CGMEView::autoconnectCursor;
HCURSOR CGMEView::autoconnect2Cursor;
HCURSOR CGMEView::disconnectCursor;
HCURSOR CGMEView::disconnect2Cursor;
HCURSOR CGMEView::setCursor;
HCURSOR CGMEView::set2Cursor;
HCURSOR CGMEView::zoomCursor;
HCURSOR CGMEView::visualCursor;
HCURSOR CGMEView::editCursor;

bool						CGMEView::showConnectedPortsOnly = false;
bool						CGMEView::derivedDrop = false;
bool						CGMEView::instanceDrop = false;
bool						CGMEView::m_bUseStretchBlt = false;
Gdiplus::SmoothingMode		CGMEView::m_eEdgeAntiAlias = Gdiplus::SmoothingModeHighQuality;		// Edge smoothing mode
Gdiplus::TextRenderingHint	CGMEView::m_eFontAntiAlias = Gdiplus::TextRenderingHintAntiAlias;	// Text renndering hint mode

IMPLEMENT_DYNCREATE(CGMEView, CScrollZoomView)

BEGIN_MESSAGE_MAP(CGMEView, CScrollZoomView)
	//{{AFX_MSG_MAP(CGMEView)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()
	ON_WM_DROPFILES()
	ON_EN_KILLFOCUS(IDC_NAME, OnKillfocusNameProp)
	ON_CBN_SELCHANGE(IDC_ASPECT, OnSelChangeAspectProp)
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEWHEEL()
	ON_WM_APPCOMMAND()
	ON_COMMAND(ID_VIEW_PARENT, OnViewParent)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PARENT, OnUpdateViewParent)
	ON_COMMAND(ID_VIEW_GRID, OnViewGrid)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GRID, OnUpdateViewGrid)
	ON_COMMAND(ID_EDIT_NUDGEDOWN, OnEditNudgedown)
	ON_COMMAND(ID_EDIT_NUDGELEFT, OnEditNudgeleft)
	ON_COMMAND(ID_EDIT_NUDGERIGHT, OnEditNudgeright)
	ON_COMMAND(ID_EDIT_NUDGEUP, OnEditNudgeup)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_COMMAND(ID_CNTX_PROPERTIES, OnContextProperties)
	ON_COMMAND(ID_CNTX_ATTRIBUTES, OnCntxAttributes)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_COPYCLOSURE, OnEditCopyClosure)
	ON_COMMAND(ID_EDIT_COPYSMART, OnEditCopySmart)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPYCLOSURE, OnUpdateEditCopyClosure)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPYSMART, OnUpdateEditCopySmart)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_CNTX_COPY, OnCntxCopy)
	ON_COMMAND(ID_CNTX_COPYCLOSURE, OnCntxCopyClosure)
	ON_COMMAND(ID_CNTX_COPYSMART, OnCntxCopySmart)
	ON_COMMAND(ID_CNTX_CUT, OnCntxCut)
	ON_COMMAND(ID_CNTX_DELETE, OnCntxDelete)
	ON_COMMAND(ID_SELFCNTX_COPY, OnSelfcntxCopy)
	ON_UPDATE_COMMAND_UI(ID_SELFCNTX_COPY, OnUpdateSelfcntxCopy)
	ON_COMMAND(ID_SELFCNTX_COPYCLOSURE, OnSelfcntxCopyClosure)
	ON_COMMAND(ID_SELFCNTX_COPYSMART, OnSelfcntxCopySmart)
	ON_UPDATE_COMMAND_UI(ID_SELFCNTX_COPYCLOSURE, OnUpdateSelfcntxCopyClosure)
	ON_UPDATE_COMMAND_UI(ID_SELFCNTX_COPYSMART, OnUpdateSelfcntxCopySmart)
	ON_COMMAND(ID_SELFCNTX_CUT, OnSelfcntxCut)
	ON_UPDATE_COMMAND_UI(ID_SELFCNTX_CUT, OnUpdateSelfcntxCut)
	ON_COMMAND(ID_SELFCNTX_DELETE, OnSelfcntxDelete)
	ON_UPDATE_COMMAND_UI(ID_SELFCNTX_DELETE, OnUpdateSelfcntxDelete)
	ON_COMMAND(ID_SELFCNTX_PASTE, OnSelfcntxPaste)
	ON_UPDATE_COMMAND_UI(ID_SELFCNTX_PASTE, OnUpdateSelfcntxPaste)
	ON_UPDATE_COMMAND_UI(ID_CNTX_ATTRIBUTES, OnUpdateCntxAttributes)
	ON_COMMAND(ID_EDIT_CANCEL, OnEditCancel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CANCEL, OnUpdateEditCancel)
	ON_COMMAND(ID_CNTX_DISCONNECTALL, OnCntxDisconnectall)
	ON_UPDATE_COMMAND_UI(ID_CNTX_DISCONNECTALL, OnUpdateCntxDisconnectall)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	ON_COMMAND(ID_CONNCNTX_PROPERTIES, OnConncntxProperties)
	ON_COMMAND(ID_CONNCNTX_DELETE, OnConncntxDelete)
	ON_COMMAND(ID_CONNCNTX_REVERSE, OnConncntxReverse)
	ON_UPDATE_COMMAND_UI(ID_CONNCNTX_REVERSE, OnUpdateConncntxReverse)
	ON_COMMAND(ID_CONNCNTX_FOLLOW, OnConnCntxFollow)
	ON_COMMAND(ID_CONNCNTX_JUMP_SRC, OnConnCntxRevfollow)
	ON_COMMAND(ID_PORTCNTX_FOLLOWCONNECTION, OnPortCntxFollowConnection)
	ON_COMMAND(ID_PORTCNTX_REVERSECONNECTION, OnPortCntxRevfollowConnection)
	ON_COMMAND(ID_PORTCNTX_DELETE, OnCntxPortDelete)
	ON_COMMAND(ID_CNTX_FOLLOWCONNECTION, OnCntxFollowConnection)
	ON_COMMAND(ID_CNTX_REVERSECONNECTION, OnCntxRevfollowConnection)
	ON_COMMAND(ID_CNTX_SHOWPORTINPARENT, OnCntxPortShowInParent)
	ON_COMMAND(ID_CNTX_LOCATEPORTINBROWSER, OnCntxPortLocateInBrw)
	ON_COMMAND(ID_JUMPALONGCONN, OnJumpAlongConnection)
	ON_COMMAND(ID_BACKALONGCONN, OnBackAlongConnection)
	ON_COMMAND(ID_TRYTOSNAPHORZVERTPATH, OnTryToSnapHorzVertPath)
	ON_COMMAND(ID_DELETECONNEDGECUSTOMDATA, OnDeleteConnEdgeCustomData)
	ON_COMMAND(ID_DELETECONNPOINTCUSTOMDATA, OnDeleteConnPointCustomData)
	ON_COMMAND(ID_DELETECONNCUSTOMDATA_THISASPECT, OnDeleteConnRouteCustomDataThisAspect)
	ON_COMMAND(ID_DELETECONNCUSTOMDATA_ALLASPECTS, OnDeleteConnRouteCustomDataAllAspects)
	ON_COMMAND(ID_JUMPTOFIRSTOBJ, OnJumpToFirstObject)
	ON_COMMAND(ID_JUMPTONEXTOBJ, OnJumpToNextObject)
	ON_COMMAND(ID_SHOWCONTEXTMENU, OnShowContextMenu)
	ON_COMMAND(ID_CNTX_CLEAR, OnCntxClear)
	ON_UPDATE_COMMAND_UI(ID_CNTX_CLEAR, OnUpdateCntxClear)
	ON_COMMAND(ID_CNTX_RESET, OnCntxReset)
	ON_UPDATE_COMMAND_UI(ID_CNTX_RESET, OnUpdateCntxReset)
	ON_UPDATE_COMMAND_UI(ID_CNTX_DELETE, OnUpdateCntxDelete)
	ON_UPDATE_COMMAND_UI(ID_CONNCNTX_DELETE, OnUpdateConncntxDelete)
	ON_UPDATE_COMMAND_UI(ID_CNTX_CUT, OnUpdateCntxCut)
	ON_UPDATE_COMMAND_UI(ID_PORTCNTX_FOLLOWCONNECTION, OnUpdatePortCntxFollowConnection)
	ON_UPDATE_COMMAND_UI(ID_PORTCNTX_REVERSECONNECTION, OnUpdatePortCntxRevfollowConnection)
	ON_UPDATE_COMMAND_UI(ID_CNTX_FOLLOWCONNECTION, OnUpdateCntxFollowConnection)
	ON_UPDATE_COMMAND_UI(ID_CNTX_REVERSECONNECTION, OnUpdateCntxRevfollowConnection)
	ON_UPDATE_COMMAND_UI(ID_JUMPALONGCONN, OnUpdateJumpAlongConnection)
	ON_UPDATE_COMMAND_UI(ID_BACKALONGCONN, OnUpdateBackAlongConnection)
	ON_UPDATE_COMMAND_UI(ID_TRYTOSNAPHORZVERTPATH, OnUpdateTryToSnapHorzVertPath)
	ON_UPDATE_COMMAND_UI(ID_DELETECONNEDGECUSTOMDATA, OnUpdateDeleteConnEdgeCustomData)
	ON_UPDATE_COMMAND_UI(ID_DELETECONNPOINTCUSTOMDATA, OnUpdateDeleteConnPointCustomData)
	ON_UPDATE_COMMAND_UI(ID_DELETECONNCUSTOMDATA_THISASPECT, OnUpdateDeleteConnRouteCustomDataThisAspect)
	ON_UPDATE_COMMAND_UI(ID_DELETECONNCUSTOMDATA_ALLASPECTS, OnUpdateDeleteConnRouteCustomDataAllAspects)
#if defined(ADDCRASHTESTMENU)
	ON_COMMAND(ID_CRASHTEST_ILLEGALWRITE, OnCrashTestIllegalWrite)
	ON_COMMAND(ID_CRASHTEST_ILLEGALREAD, OnCrashTestIllegalRead)
	ON_COMMAND(ID_CRASHTEST_ILLEGALREADINCRUNTIME, OnCrashTestIllegalReadInCRuntime)
	ON_COMMAND(ID_CRASHTEST_ILLEGALCODEREAD, OnCrashTestIllegalCodeRead)
	ON_COMMAND(ID_CRASHTEST_DIVIDEBYZERO, OnCrashTestDivideByZero)
	ON_COMMAND(ID_CRASHTEST_ABORT, OnCrashTestAbort)
	ON_COMMAND(ID_CRASHTEST_TERMINATE, OnCrashTestTerminate)
	ON_UPDATE_COMMAND_UI(IDR_CRASH_TEST_MENU, OnUpdateCrashTestMenu)
	ON_UPDATE_COMMAND_UI(ID_CRASHTEST_ILLEGALWRITE, OnUpdateCrashTestIllegalWrite)
	ON_UPDATE_COMMAND_UI(ID_CRASHTEST_ILLEGALREAD, OnUpdateCrashTestIllegalRead)
	ON_UPDATE_COMMAND_UI(ID_CRASHTEST_ILLEGALREADINCRUNTIME, OnUpdateCrashTestIllegalReadInCRuntime)
	ON_UPDATE_COMMAND_UI(ID_CRASHTEST_ILLEGALCODEREAD, OnUpdateCrashTestIllegalCodeRead)
	ON_UPDATE_COMMAND_UI(ID_CRASHTEST_DIVIDEBYZERO, OnUpdateCrashTestDivideByZero)
	ON_UPDATE_COMMAND_UI(ID_CRASHTEST_ABORT, OnUpdateCrashTestAbort)
	ON_UPDATE_COMMAND_UI(ID_CRASHTEST_TERMINATE, OnUpdateCrashTestTerminate)
#endif
	ON_COMMAND(ID_CNTX_PREFERENCES, OnCntxPreferences)
	ON_COMMAND(ID_EDIT_PREFERENCES, OnEditPreferences)
	ON_COMMAND(ID_HELP_HELP, OnHelpHelp)
	ON_COMMAND(ID_CNTX_HELP, OnCntxHelp)
	ON_COMMAND(ID_EDIT_SHOWTYPE, OnEditShowtype)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHOWTYPE, OnUpdateEditShowtype)
	ON_COMMAND(ID_EDIT_SHOWBASETYPE, OnEditShowbasetype)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHOWBASETYPE, OnUpdateEditShowbasetype)
	ON_COMMAND(ID_CNTX_SHOWTYPE, OnCntxShowtype)
	ON_UPDATE_COMMAND_UI(ID_CNTX_SHOWTYPE, OnUpdateCntxShowtype)
	ON_COMMAND(ID_CNTX_SHOWBASETYPE, OnCntxShowbasetype)
	ON_UPDATE_COMMAND_UI(ID_CNTX_SHOWBASETYPE, OnUpdateCntxShowbasetype)
	ON_COMMAND(ID_FILE_INTERPRET, OnFileInterpret)
	ON_COMMAND(ID_FILE_CHECK, OnFileCheck)
	ON_COMMAND(ID_FILE_CHECKSEL, OnFileCheckSelected)
	ON_COMMAND(ID_CNTX_INTERPRET, OnCntxInterpret)
	ON_COMMAND(ID_CNTX_CHECK, OnCntxCheck)
	ON_COMMAND(ID_CNTX_LOCATE, OnCntxLocate)
	ON_UPDATE_COMMAND_UI(ID_CNTX_INTERPRET, OnUpdateCntxInterpret)
	ON_UPDATE_COMMAND_UI(ID_CNTX_CHECK, OnUpdateCntxCheck)
	ON_UPDATE_COMMAND_UI(ID_CNTX_LOCATE, OnUpdateCntxLocate)
	ON_COMMAND(ID_CNTX_REGISTRY, OnCntxRegistry)
	ON_COMMAND(ID_EDIT_REGISTRY, OnEditRegistry)
	ON_COMMAND(ID_EDIT_SYNC, OnEditSync)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SYNC, OnUpdateEditSync)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
	ON_UPDATE_COMMAND_UI(ID_FILE_CHECK, OnUpdateFileCheck)
	ON_UPDATE_COMMAND_UI(ID_FILE_CHECKSEL, OnUpdateFileCheckSelected)
	ON_UPDATE_COMMAND_UI(ID_FILE_SETTINGS, OnUpdateFileSettings)
	ON_COMMAND(ID_EDIT_PASTESPECIAL_ASINSTANCE, OnEditPastespecialAsinstance)
	ON_COMMAND(ID_EDIT_PASTESPECIAL_ASREFERENCE, OnEditPastespecialAsreference)
	ON_COMMAND(ID_EDIT_PASTESPECIAL_ASSUBTYPE, OnEditPastespecialAssubtype)
	ON_COMMAND(ID_EDIT_PASTESPECIAL_ASCLOSURE, OnEditPastespecialAsclosure)
	ON_COMMAND(ID_EDIT_PASTESPECIAL_SMART_ADDITIVE, OnEditPastespecialAdditive)
	ON_COMMAND(ID_EDIT_PASTESPECIAL_SMART_MERGE, OnEditPastespecialMerge)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTESPECIAL_ASINSTANCE, OnUpdateEditPastespecialAsinstance)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTESPECIAL_ASREFERENCE, OnUpdateEditPastespecialAsreference)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTESPECIAL_ASSUBTYPE, OnUpdateEditPastespecialAssubtype)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTESPECIAL_ASCLOSURE, OnUpdateEditPastespecialAsclosure)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTESPECIAL_SMART_ADDITIVE, OnUpdateEditPastespecialAdditive)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTESPECIAL_SMART_MERGE, OnUpdateEditPastespecialMerge)
	ON_COMMAND(ID_CNTX_PASTESPECIAL_ASINSTANCE, OnCntxPastespecialAsinstance)
	ON_UPDATE_COMMAND_UI(ID_CNTX_PASTESPECIAL_ASINSTANCE, OnUpdateCntxPastespecialAsinstance)
	ON_COMMAND(ID_CNTX_PASTESPECIAL_ASREFERENCE, OnCntxPastespecialAsreference)
	ON_UPDATE_COMMAND_UI(ID_CNTX_PASTESPECIAL_ASREFERENCE, OnUpdateCntxPastespecialAsreference)
	ON_COMMAND(ID_CNTX_PASTESPECIAL_ASSUBTYPE, OnCntxPastespecialAssubtype)
	ON_UPDATE_COMMAND_UI(ID_CNTX_PASTESPECIAL_ASSUBTYPE, OnUpdateCntxPastespecialAssubtype)
	ON_COMMAND(ID_CNTX_PASTESPECIAL_ASCLOSURE, OnCntxPastespecialAsclosure)
	ON_UPDATE_COMMAND_UI(ID_CNTX_PASTESPECIAL_ASCLOSURE, OnUpdateCntxPastespecialAsclosure)
	ON_COMMAND(ID_CNTX_PASTESPECIAL_SMART_ADDITIVE, OnCntxPastespecialAdditive)
	ON_UPDATE_COMMAND_UI(ID_CNTX_PASTESPECIAL_SMART_ADDITIVE, OnUpdateCntxPastespecialAdditive)
	ON_COMMAND(ID_CNTX_PASTESPECIAL_SMART_MERGE, OnCntxPastespecialMerge)
	ON_UPDATE_COMMAND_UI(ID_CNTX_PASTESPECIAL_SMART_MERGE, OnUpdateCntxPastespecialMerge)
	ON_COMMAND(ID_CNTX_REDIRECTIONPASTE, OnCntxRedirectionpaste)
	ON_UPDATE_COMMAND_UI(ID_CNTX_REDIRECTIONPASTE, OnUpdateCntxRedirectionpaste)
	ON_COMMAND(ID_KEY_CONNECT, OnKeyConnect)
	ON_COMMAND(ID_CNTX_CONNECT, OnCntxConnect)
	ON_UPDATE_COMMAND_UI(ID_CNTX_CONNECT, OnUpdateCntxConnect)
	ON_COMMAND(ID_RESET_STICKY, OnResetSticky)
	ON_WM_NCMOUSEMOVE()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_UPDATE_COMMAND_UI(ID_CNTX_INSERTANNOTATION, OnUpdateCntxInsertannotation)
	ON_COMMAND(ID_CNTX_INSERTANNOTATION, OnCntxInsertannotation)
	ON_COMMAND(ID_CNTX_ANNOTATIONS, OnCntxAnnotations)
	ON_COMMAND(ID_EDIT_ANNOTATIONS, OnEditAnnotations)
	ON_UPDATE_COMMAND_UI( ID_CNTX_SRCAR_NORTH, OnUpdateCntxAutoRouters )
	ON_COMMAND(ID_CNTX_SRCAR_SOUTH, OnCntxSrcarSouth)
	ON_COMMAND(ID_CNTX_SRCAR_NORTH, OnCntxSrcarNorth)
	ON_COMMAND(ID_CNTX_SRCAR_EAST, OnCntxSrcarEast)
	ON_COMMAND(ID_CNTX_SRCAR_WEST, OnCntxSrcarWest)
	ON_COMMAND(ID_CNTX_DSTAR_EAST, OnCntxDstarEast)
	ON_COMMAND(ID_CNTX_DSTAR_NORTH, OnCntxDstarNorth)
	ON_COMMAND(ID_CNTX_DSTAR_SOUTH, OnCntxDstarSouth)
	ON_COMMAND(ID_CNTX_DSTAR_WEST, OnCntxDstarWest)
	ON_COMMAND(ID_CNTX_SRCAR_CLEAR, OnCntxSrcarClear)
	ON_COMMAND(ID_CNTX_DSTAR_CLEAR, OnCntxDstarClear)
	ON_COMMAND(ID_CNTX_SRCAR_SET, OnCntxSrcarSet)
	ON_COMMAND(ID_CNTX_DSTAR_SET, OnCntxDstarSet)
	ON_UPDATE_COMMAND_UI( ID_CNTX_DSTAR_NORTH, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_SRCAR_SOUTH, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_DSTAR_SOUTH, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_SRCAR_WEST, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_DSTAR_WEST, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_SRCAR_EAST, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_DSTAR_EAST, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_SRCAR_CLEAR, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_DSTAR_CLEAR, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_SRCAR_SET, OnUpdateCntxAutoRouters )
	ON_UPDATE_COMMAND_UI( ID_CNTX_DSTAR_SET, OnUpdateCntxAutoRouters )
	ON_COMMAND(ID_PRINT_METAFILE, OnPrintMetafile)
	ON_COMMAND(ID_CNTX_NMPOS_SOUTH, OnCntxNamePositionSouth)
	ON_COMMAND(ID_CNTX_NMPOS_NORTH, OnCntxNamePositionNorth)
	ON_COMMAND(ID_CNTX_NMPOS_EAST, OnCntxNamePositionEast)
	ON_COMMAND(ID_CNTX_NMPOS_WEST, OnCntxNamePositionWest)
	ON_UPDATE_COMMAND_UI( ID_CNTX_NMPOS_SOUTH, OnUpdateCntxNamePositionSouth )
	ON_UPDATE_COMMAND_UI( ID_CNTX_NMPOS_NORTH, OnUpdateCntxNamePositionNorth )
	ON_UPDATE_COMMAND_UI( ID_CNTX_NMPOS_EAST, OnUpdateCntxNamePositionEast )
	ON_UPDATE_COMMAND_UI( ID_CNTX_NMPOS_WEST, OnUpdateCntxNamePositionWest )
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CScrollZoomView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CScrollZoomView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScrollZoomView::OnFilePrintPreview)
	ON_COMMAND(ID_KEY_ZOOMIN, OnZoomIn)
	ON_COMMAND(ID_KEY_ZOOMOUT, OnZoomOut)
	ON_COMMAND(ID_KEY_CYCLEOBJINSPFRWD, OnKeyCycleObjInspectorFrwd)
	ON_COMMAND(ID_KEY_CYCLEOBJINSPBKWD, OnKeyCycleObjInspectorBkwd)
	ON_MESSAGE(WM_USER_ZOOM, OnZoom)
	ON_MESSAGE(WM_USER_PANNREFRESH, OnPannRefresh)
	ON_MESSAGE(WM_PANN_SCROLL, OnPannScroll)
	ON_MESSAGE(WM_USER_DECOR_VIEWREFRESH_REQ, OnDecoratorViewRefreshRequest)
	ON_MESSAGE(WM_USER_EXECUTEPENDINGREQUESTS, OnExecutePendingRequests)
	ON_COMMAND(ID_VIEW_SHOWSELMODEL, OnShowSelectedModel)
	ON_COMMAND(ID_VIEW_FOCUSBROWSER, OnFocusBrowser)
	ON_COMMAND(ID_VIEW_RENAME, OnRename)
	ON_COMMAND(ID_VIEW_FOCUSINSPECTOR, OnFocusInspector)
	ON_COMMAND(ID_VIEW_CYCLEASPECTKEY, OnCycleAspect)
	ON_COMMAND(ID_VIEW_CYCLEASPECTBACKWARDSKEY, OnCycleAspectBackwards)
	ON_COMMAND(ID_VIEW_CYCLEALLASPECTS, OnCycleAllAspects)
	ON_COMMAND(ID_VIEW_HISTORYBACKKEY, OnHistoryBack)
	ON_COMMAND(ID_VIEW_HISTORYFORWKEY, OnHistoryForw)
	ON_COMMAND(ID_MULTIUSER_SHOWOWNER, OnViewMultiUserShowObjectOwner)
	ON_UPDATE_COMMAND_UI( ID_MULTIUSER_SHOWOWNER, OnUpdateViewMultiUserShowObjectOwner)
	ON_WM_KILLFOCUS()
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)

	ON_COMMAND(ID_VIEW_SHOWCONNECTEDPORTSONLY, &CGMEView::OnViewShowconnectedportsonly)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWCONNECTEDPORTSONLY, &CGMEView::OnUpdateViewShowconnectedportsonly)
	END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGMEView construction/destruction

CGMEView::CGMEView()
{
	m_isActive						= false;
	m_preview						= false;
	m_scalePrnPages					= 1;
	m_currPrnNumAsp					= 0;
	m_fullPrnAspNum					= 1;
	m_prevcurrasp					= NULL;
	m_prnpos						= NULL;
	m_lastPrnPage					= 0;
	m_zoomVal						= ZOOM_NO;
	m_bEnablePannWindowRefresh = false;

	initDone						= false;
	isModelAutoRouted				= theApp.useAutoRouting;
	inTransaction					= 0;

	autoconnectCursor				= AfxGetApp()->LoadCursor(IDC_AUTOCONNECT_CURSOR);
	autoconnect2Cursor				= AfxGetApp()->LoadCursor(IDC_AUTOCONNECT2_CURSOR);
	disconnectCursor				= AfxGetApp()->LoadCursor(IDC_DISCONNECT_CURSOR);
	disconnect2Cursor				= AfxGetApp()->LoadCursor(IDC_DISCONNECT2_CURSOR);
	setCursor						= AfxGetApp()->LoadCursor(IDC_SET_CURSOR);
	set2Cursor						= AfxGetApp()->LoadCursor(IDC_SET2_CURSOR);
	zoomCursor						= AfxGetApp()->LoadCursor(IDC_ZOOM_CURSOR);
	visualCursor					= AfxGetApp()->LoadCursor(IDC_VISUAL_CURSOR);
	editCursor						= LoadCursor(0,IDC_ARROW);

	drawGrid						= false;

	SetIsCursorChangedByDecorator	(false);
	SetOriginalRectEmpty			();
	SetInElementDecoratorOperation	(false);
	SetInOpenedDecoratorTransaction	(false);
	SetIsContextInitiatedOperation	(false);
	SetShouldCommitOperation		(false);
	SetDecoratorOrAnnotator			(true);
	SetObjectInDecoratorOperation	(NULL);
	SetAnnotatorInDecoratorOperation(NULL);
	selectedObjectOfContext			= NULL;
	selectedAnnotationOfContext		= NULL;
	selectedConnection				= NULL;
	isLeftMouseButtonDown			= false;
	isConnectionJustSelected		= false;
	isInConnectionCustomizeOperation= false;
	doNotDeselectAfterInPlaceEdit	= false;
	isCursorChangedByEdgeCustomize	= false;
	selectedContextConnection		= NULL;

	prevDropEffect					= DROPEFFECT_NONE;
	inDrag							= false;
	contextMenuLocation				= CPoint(0,0);

	guiMeta							= 0;
	currentAspect					= 0;
	currentSet						= 0;
	last_Connection					= 0;
	lastObject						= 0;
	lastPort						= 0;
	dragSource						= 0;

	needsReset						= false;
	needsConnConversion				= false;
	alive							= true;
	bgColor							= 0;

	instanceCount++;

	animRefCnt						= 0;
	timerID							= 0;

	driver							= new CComObject<CViewDriver>;
	driver->view					= this;

	executingPendingRequests		= false;

	contextSelection				= 0;
	contextAnnotation				= 0;
	contextPort						= 0;

	tmpConnectMode					= false;
	ClearSupressConnectionCheckAlert();
	ClearConnSpecs();

	CComPtr<IMgaRegistrar> registrar;
	COMTHROW(registrar.CoCreateInstance(CComBSTR(L"Mga.MgaRegistrar")));
	ASSERT(registrar != NULL);
	edgesmoothmode_enum edgeSmoothMode;
	COMTHROW(registrar->get_EdgeSmoothMode(REGACCESS_USER, &edgeSmoothMode));
	m_eEdgeAntiAlias = (Gdiplus::SmoothingMode)edgeSmoothMode;
	fontsmoothmode_enum fontSmoothMode;
	COMTHROW(registrar->get_FontSmoothMode(REGACCESS_USER, &fontSmoothMode));
	m_eFontAntiAlias = (Gdiplus::TextRenderingHint)fontSmoothMode;

	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\GME\\GUI\\"),
					 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		TCHAR szData[128];
		DWORD dwKeyDataType;
		DWORD dwDataBufSize = sizeof(szData)/sizeof(TCHAR);

		// Supposed workaround for Vista black view problem: use StretchBlt instead of BitBlt
		// Problem can arise in multi-monitor systems with NVidia cards (but who knows what other configs)
		// If user wants to fall back to the faster BitBlt, he can force GME to use it by creating a
		// string registry key with "0" value under the "HKCU\Software\GME\GUI\UseStretchBlt"
		if (RegQueryValueEx(hKey, _T("UseStretchBlt"), NULL, &dwKeyDataType,
							(LPBYTE) &szData, &dwDataBufSize) == ERROR_SUCCESS)
		{
			UINT uUseStretchBlt = _tcstoul(szData, NULL, 10);
			m_bUseStretchBlt = (uUseStretchBlt != 0);
		}
		RegCloseKey(hKey);
	}
}

CGMEView::~CGMEView()
{
	// a good idea to release ptrs
	baseType.Release();
	parent.Release();
	currentModel.Release();

	if(--instanceCount <= 0) {
		// update & disable some components
		theApp.UpdateCompList4CurrentKind( CGMEApp::m_no_model_open_string);

		DeleteOffScreen();
		if( CMainFrame::theInstance != NULL ) {
			CMainFrame::theInstance->SetPartBrowserMetaModel(NULL);
			CMainFrame::theInstance->SetPartBrowserBg(::GetSysColor(COLOR_APPWORKSPACE));
		}
	}
	POSITION pos = children.GetHeadPosition();
	while(pos)
		delete children.GetNext(pos);

	pos = annotators.GetHeadPosition();
	while(pos)
		delete annotators.GetNext(pos);

	pos = pendingRequests.GetHeadPosition();
	while(pos)
		delete pendingRequests.GetNext(pos);
	pendingRequests.RemoveAll();

	driver->view = 0;
}

BOOL CGMEView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollZoomView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CGMEView drawing

#define PANNING_RATIO_MIN	4 // ??
void CGMEView::DoPannWinRefresh()
{
	if (!m_bEnablePannWindowRefresh)
		return;
//	CMainFrame* main = (CMainFrame*)AfxGetMainWnd();
	CMainFrame* main = (CMainFrame*)theApp.m_pMainWnd;

	if (!main || !main->m_panningWindow.IsVisible() || CGMEView::GetActiveView() != this)
		return;

	// the original size of the image
	CRect extent, objext, annext;
	CGuiObject::GetExtent(children,objext);
	CGuiAnnotator::GetExtent(annotators,annext);
	extent.UnionRect(&objext, &annext);
	extent.right = (int)(extent.right * EXTENT_ERROR_CORR); // ??
	extent.bottom = (int)(extent.bottom * EXTENT_ERROR_CORR); // ??

	CRect target = CRect(0, 0, extent.Width() / PANNING_RATIO_MIN, extent.Height() / PANNING_RATIO_MIN);
	// FIXME: if we detected Panning Window size change, we could do this and waste less:
	// target = main->m_panningWindow.GetWindowRect(&rectt);

	// make a bitmap DC
	HDC tmpDC = ::GetWindowDC(this->m_hWnd);
	HDC pannDC = CreateCompatibleDC(tmpDC);
	ASSERT(pannDC != NULL);

	HBITMAP pannBmp = ::CreateCompatibleBitmap(tmpDC, target.Width(), target.Height());
	if (pannBmp == NULL) {	// introd' by zolmol
		if (pannDC != NULL)
			::DeleteDC(pannDC);
		return;
	}
	HBITMAP oldBmp = (HBITMAP)::SelectObject(pannDC, pannBmp);

	// set background color
	::SetMapMode(pannDC, MM_TEXT);
	// DWORD dw1 = GetSysColor(COLOR_WINDOW);
	// BYTE r1 = GetRValue(dw1);
	// BYTE g1 = GetGValue(dw1);
	// BYTE b1 = GetBValue(dw1);
	HBRUSH bgBrush = ::CreateSolidBrush(bgColor);
	::FillRect(pannDC, target, bgBrush);
	::DeleteObject(bgBrush);

	::SetMapMode(pannDC, MM_ISOTROPIC);
	::SetWindowExtEx(pannDC, extent.Width(), extent.Height(), NULL);
	::SetViewportExtEx(pannDC, target.Width(), target.Height(), NULL);

	{
		Gdiplus::Graphics gdip(pannDC);
		gdip.SetPageUnit(Gdiplus::UnitPixel);
		gdip.SetSmoothingMode(m_eEdgeAntiAlias);
		gdip.SetTextRenderingHint(m_eFontAntiAlias);

		// draw the image
		POSITION pos = annotators.GetHeadPosition();
		while (pos) {
			CGuiAnnotator *annotator = annotators.GetNext(pos);
			if (annotator->IsVisible()) {
				annotator->Draw(pannDC, &gdip);
			}
		}
	}
	{
		POSITION pos = children.GetHeadPosition();
		while (pos) {
			CGuiFco* fco = children.GetNext(pos);
			ASSERT(fco != NULL);
			if (fco->IsVisible()) {
				CGuiConnection* conn = fco->dynamic_cast_CGuiConnection();
				if (!conn)
				{
					// GME-339: Gdiplus::Graphics may modify pDC (e.g. SetViewportOrgEx) when a new-style decorator runs
					// a modified pDC makes old-style (i.e. no DrawEx) decorators render incorrectly (e.g. when the scrollbar is scrolled)
					Gdiplus::Graphics gdip(pannDC);
					gdip.SetPageUnit(Gdiplus::UnitPixel);
					gdip.SetSmoothingMode(m_eEdgeAntiAlias);
					gdip.SetTextRenderingHint(m_eFontAntiAlias);
					fco->Draw(pannDC, &gdip);
				}
			}
		}
	}

	// force CPanningWindow to reset the DC 
	main->m_panningWindow.SetBitmapDC(this->m_hWnd, pannDC, oldBmp, extent, target, bgColor);
	notifyPanning(GetDeviceScrollPosition());
	::ReleaseDC(this->m_hWnd, tmpDC);
}

void CGMEView::OnDraw(CDC* pDC)
{
	CGMEDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	onScreen = pDC;
	if(!onScreen->IsPrinting()  &&  !IsPreview()) { 
		CRect onScreenClipBox;
		onScreen->GetClipBox(&onScreenClipBox);
		if (onScreenClipBox.IsRectEmpty()) // empty check is enough: windows may never overlap, since they don't float
			return;
		CreateOffScreen(pDC);
		pDC = offScreen;
		OnPrepareDC(pDC);
	}

	{
		Gdiplus::Graphics gdip(pDC->m_hDC);
		gdip.SetPageUnit(Gdiplus::UnitPixel);
		gdip.SetSmoothingMode(m_eEdgeAntiAlias);
		gdip.SetTextRenderingHint(m_eFontAntiAlias);

		if(drawGrid && !pDC->IsPrinting() && !IsPreview() && /// zoomIdx >= GME_ZOOM_LEVEL_MED) 
				m_zoomVal >= ZOOM_NO)
		{
			CRect objext, annext, extent;
			CGuiObject::GetExtent(children, objext);
			CGuiAnnotator::GetExtent(annotators, annext);
			extent.UnionRect(&objext, &annext);
			extent.right = (int)(extent.right*EXTENT_ERROR_CORR); // ??
			extent.bottom = (int)(extent.bottom*EXTENT_ERROR_CORR); // ??
			CSize s(extent.right, extent.bottom);
//			s.cx = s.cx + END_SCROLL_OFFSET;
//			s.cy = s.cy + END_SCROLL_OFFSET;
			CRect rect;
			GetClientRect(&rect);
			graphics.DrawGrid(&gdip, GME_GRID_SIZE, GME_GRID_SIZE,
							  max(s.cx,rect.right), max(s.cy,rect.bottom));
		}

		/*
		// DEBUG

		CPoint spt = GetScrollPosition();
		pDC->LPtoDP(&spt);
		CRect clientRect;
		GetClientRect(&clientRect);
		clientRect += spt;
		COLORREF col = RGB(10,10,10);
		for (int x = clientRect.left; x < clientRect.right; x++) {
			for (int y = clientRect.top; y < clientRect.bottom; y++) {
				CPoint ppp(x, y);
				CoordinateTransfer(ppp);
				if (!modelGrid.IsAvailable(ppp.x, ppp.y)) {
					CPoint np((x - spt.x)*zoom, (y-spt.y)*zoom);
					pDC->DPtoLP(&np);
					pDC->SetPixel(np, col);
				}
			}
		}
		//END-DEBUG
		*/


		POSITION pos = annotators.GetHeadPosition();
		while (pos) {
			CGuiAnnotator *annotator = annotators.GetNext(pos);
			if (annotator->IsVisible()) {
				annotator->Draw(pDC->m_hDC, &gdip);
			}
		}

		CRect visible;
		if (pDC->IsPrinting())
		{
			visible = CRect(0, 0, INT_MAX, INT_MAX);
		}
		else
		{
			CRect clientRect;
			GetClientRect(clientRect);
			CPoint clientSize = clientRect.Size();
			CPoint scrollpos = GetDeviceScrollPosition();
			scrollpos.x = MulDiv(scrollpos.x, 96, CClientDC(this).GetDeviceCaps(LOGPIXELSY));
			scrollpos.y = MulDiv(scrollpos.y, 96, CClientDC(this).GetDeviceCaps(LOGPIXELSY));
			visible = CRect(scrollpos, scrollpos + clientSize);
			visible.InflateRect(10 * 100 / m_scalePercent, 10 * 100 / m_scalePercent);
			visible = CRect(std::max(0l, visible.left), std::max(0l, visible.top),
				visible.right, visible.bottom);
		}

		pos = children.GetHeadPosition();
		int miss = 0;
		while (pos) {
			CGuiFco* fco = children.GetNext(pos);
                  ASSERT(fco != NULL);
			if (fco->IsVisible()) {
				CGuiConnection* conn = fco->dynamic_cast_CGuiConnection();

				CRect loc(0, 0, INT_MAX, INT_MAX);
				if (fco->dynamic_cast_CGuiObject())
				{
					CRect fcoLoc = fco->dynamic_cast_CGuiObject()->GetCurrentAspect()->GetLocation();
					fcoLoc = fcoLoc.MulDiv(m_scalePercent, 100);
					CRect labelLoc = fco->dynamic_cast_CGuiObject()->GetCurrentAspect()->GetNameLocation();
					labelLoc = labelLoc.MulDiv(m_scalePercent, 100);
					loc.UnionRect(fcoLoc, labelLoc);
				}
				if (!conn && CRect().IntersectRect(visible, loc)) {
					// GME-339: Gdiplus::Graphics may modify pDC (e.g. SetViewportOrgEx) when a new-style decorator runs
					// a modified pDC makes old-style (i.e. no DrawEx) decorators render incorrectly (e.g. when the scrollbar is scrolled)
					gdip.~Graphics();
					::new ((void*)&gdip) Gdiplus::Graphics(pDC->m_hDC);
					gdip.SetPageUnit(Gdiplus::UnitPixel);
					gdip.SetSmoothingMode(m_eEdgeAntiAlias);
					gdip.SetTextRenderingHint(m_eFontAntiAlias);
					fco->Draw(pDC->m_hDC, &gdip);
				}
				else
					miss++;
			}
		}
		DrawConnections(pDC->m_hDC, &gdip);

		if(pDoc->GetEditMode() == GME_EDIT_MODE || pDoc->GetEditMode() == GME_SET_MODE) {
			// msdn "Interoperability between GDI and GDI+" "you shouldn't access the HDC directly from GDI until the Graphics object is destroyed"
			gdip.~Graphics();

			if( ( (selected.GetCount() > 0) || (selectedAnnotations.GetCount() > 0) ) 
						&& !pDC->IsPrinting() && !IsPreview()) {
				POSITION pos = selected.GetHeadPosition();
				CRectTrackerDpi tracker;
				CGuiObject *obj;
				while(pos) {
					obj = selected.GetNext(pos);
					tracker.m_rect = obj->GetLocation();
					tracker.m_nStyle = CRectTracker::solidLine;
					if (obj->IsResizable())
						tracker.m_nStyle |= CRectTracker::resizeInside;
					tracker.Draw(pDC);
				}

				pos = selectedAnnotations.GetHeadPosition();
				CGuiAnnotator *ann;
				while(pos) {
					ann = selectedAnnotations.GetNext(pos);
					if (ann->IsVisible())	// the selectedAnnotation might become hidden in this aspect
					{
						tracker.m_rect = ann->GetLocation();
						tracker.m_nStyle = CRectTracker::solidLine;
						if (ann->IsResizable())
							tracker.m_nStyle |= CRectTracker::resizeInside;
						tracker.Draw(pDC);
					}
				}
			}
			::new ((void*)&gdip) Gdiplus::Graphics(pDC->m_hDC);
			gdip.SetPageUnit(Gdiplus::UnitPixel);
			gdip.SetSmoothingMode(m_eEdgeAntiAlias);
			gdip.SetTextRenderingHint(m_eFontAntiAlias);
		}

		if (GetFocus() == this && ((pDoc->GetEditMode() == GME_AUTOCONNECT_MODE || pDoc->GetEditMode() == GME_SHORTAUTOCONNECT_MODE) || (tmpConnectMode))) {
			if (connSrc) {
				CRect rect = connSrc->GetLocation();
				if (connSrcPort) {
					rect = connSrcPort->GetLocation() + rect.TopLeft();
				}
				Gdiplus::Pen* xorPen = graphics.GetGdipPen2(&gdip, GME_DARKRED_COLOR, GME_LINE_SOLID, GME_CONNSELECT_WIDTH);
				gdip.DrawRectangle(xorPen, rect.left - .5f, rect.top - .5f, (float)rect.Width(), (float)rect.Height());

				if ((connSrcHotSide != GME_CENTER) && (!connSrcPort)) {
					CPoint hotSpot;
					switch (connSrcHotSide) {
					case GME_SOUTH:
						hotSpot.x = rect.CenterPoint().x;
						hotSpot.y = rect.bottom;
						break;
					case GME_NORTH:
						hotSpot.x = rect.CenterPoint().x;
						hotSpot.y = rect.top;
						break;
					case GME_WEST:
						hotSpot.x = rect.left;
						hotSpot.y = rect.CenterPoint().y;
						break;
					case GME_EAST:
						hotSpot.x = rect.right;
						hotSpot.y = rect.CenterPoint().y;
						break;
					}
					Gdiplus::Brush* xorBrush = graphics.GetGdipBrush(GME_DARKRED_COLOR);
					gdip.FillRectangle(xorBrush, hotSpot.x - GME_HOTSPOT_VISUAL_RADIUS, hotSpot.y - GME_HOTSPOT_VISUAL_RADIUS, 2 * GME_HOTSPOT_VISUAL_RADIUS, 2 * GME_HOTSPOT_VISUAL_RADIUS);
					//gdip.FillEllipse(xorBrush, hotSpot.x - GME_HOTSPOT_VISUAL_RADIUS, hotSpot.y - GME_HOTSPOT_VISUAL_RADIUS, 2 * GME_HOTSPOT_VISUAL_RADIUS, 2 * GME_HOTSPOT_VISUAL_RADIUS);
				}
			}
			if (connTmp) {
				CRect rect = connTmp->GetLocation();
				if (connTmpPort) {
					rect = connTmpPort->GetLocation() + rect.TopLeft();
				}
				Gdiplus::Pen* xorPen = graphics.GetGdipPen2(&gdip, GME_RED_COLOR, GME_LINE_SOLID, GME_CONNSELECT_WIDTH);
				gdip.DrawRectangle(xorPen, rect.left - .5f, rect.top - .5f, (float) rect.Width(), (float) rect.Height());

				if ((connTmpHotSide != GME_CENTER) && (!connTmpPort)) {
					CPoint hotSpot;
					switch (connTmpHotSide) {
					case GME_SOUTH:
						hotSpot.x = rect.CenterPoint().x;
						hotSpot.y = rect.bottom;
						break;
					case GME_NORTH:
						hotSpot.x = rect.CenterPoint().x;
						hotSpot.y = rect.top;
						break;
					case GME_WEST:
						hotSpot.x = rect.left;
						hotSpot.y = rect.CenterPoint().y;
						break;
					case GME_EAST:
						hotSpot.x = rect.right;
						hotSpot.y = rect.CenterPoint().y;
						break;
					}
					Gdiplus::Brush* xorBrush = graphics.GetGdipBrush(GME_RED_COLOR);
					gdip.FillRectangle(xorBrush, hotSpot.x - GME_HOTSPOT_VISUAL_RADIUS, hotSpot.y - GME_HOTSPOT_VISUAL_RADIUS, 2 * GME_HOTSPOT_VISUAL_RADIUS, 2 * GME_HOTSPOT_VISUAL_RADIUS);
					//gdip.FillEllipse(xorBrush, hotSpot.x - GME_HOTSPOT_VISUAL_RADIUS, hotSpot.y - GME_HOTSPOT_VISUAL_RADIUS, 2 * GME_HOTSPOT_VISUAL_RADIUS, 2 * GME_HOTSPOT_VISUAL_RADIUS);
				}
			}
		}

		if (isInConnectionCustomizeOperation) {
			DrawConnectionCustomizationTracker(pDC, &gdip);
		}
	}

	if(!onScreen->IsPrinting()  &&  !IsPreview()) {
		CRect r;
		GetClientRect(&r);
		onScreen->DPtoLP(&r);
		CPoint pt = GetScrollPosition();
		// Supposed workaround for Vista black view problem: use StretchBlt instead of BitBlt
		// Problem can arise in multi-monitor systems with NVidia cards (but who knows what other configs)
		if (m_bUseStretchBlt)
			onScreen->StretchBlt(pt.x - 5, pt.y - 5, r.Width() + 10, r.Height() + 10, offScreen, pt.x - 5, pt.y - 5, r.Width() + 10, r.Height() + 10, SRCCOPY);
		else
			onScreen->BitBlt(pt.x - 5, pt.y - 5, r.Width() + 10, r.Height() + 10, offScreen, pt.x - 5, pt.y - 5, SRCCOPY);
	}
}


void CGMEView::OnInitialUpdate()
{
	BeginWaitCursor();
	CScrollZoomView::OnInitialUpdate();
	frame = (CChildFrame *)(GetParentFrame());
	frame->SetView(this);

	dropTarget.Register(this);
	EnableToolTips(TRUE);

//	zoomIdx = GME_ZOOM_LEVEL_MED;
	m_zoomVal = ZOOM_NO;
	int zv = 0;
	int l = _stscanf( (LPCTSTR) theApp.getDefZoomLev(), _T("%d"), &zv);
	if( l == 1 && zv && zv >= ZOOM_MIN && zv <= ZOOM_MAX) // do not accept ZOOM_WIDTH,ZOOM_HEIGHT,ZOOM_ALL for now as ZoomValue
		m_zoomVal = zv;
	// sets the combo too 
	frame->propBar.SetZoomVal(m_zoomVal);
	CMainFrame::theInstance->WriteStatusZoom(m_zoomVal); // setZoomPercents[zoomIdx]);

	CDC* pDC = GetDC();
	CreateOffScreen(pDC);
	ReleaseDC(pDC);
	CComPtr<IMgaFCO> centerObj;

	try {
		COMTHROW(theApp.mgaProject->CreateTerritory(driver,&terry, NULL));
		BeginTransaction(TRANSACTION_READ_ONLY);

		{
		   CComPtr<IMgaObject> ob;
		   COMTHROW(terry->OpenObj(GetDocument()->nextToView, &ob));
		   GetDocument()->ResetNextToView();
		   COMTHROW(ob->QueryInterface(&currentModel));
		}

		COMTHROW(currentModel->Open(OPEN_READ));
		COMTHROW(currentModel->get_Name(PutOut(name)));
		VARIANT_BOOL isLibObject = VARIANT_FALSE;
		COMTHROW(currentModel->get_IsLibObject(&isLibObject));
		this->isLibObject = isLibObject != VARIANT_FALSE;

		CComPtr<IMgaMetaFCO> meta;
		COMTHROW(currentModel->get_Meta(&meta));
		COMTHROW(meta->get_Name(PutOut(kindName)));
		COMTHROW(meta->get_DisplayedName(PutOut(kindDisplayedName)));

		CComBSTR modid;
		COMTHROW( currentModel->get_ID( &modid));
		currentModId = modid;

		metaref_type mt;
		COMTHROW(meta->get_MetaRef(&mt));
		guiMeta = CGuiMetaProject::theInstance->GetGuiMetaModel(mt);
		currentAspect = guiMeta->FindAspect(GetDocument()->nextAspect);
		if(!currentAspect)
			currentAspect = guiMeta->GetFirstAspect();

		{
			VARIANT_BOOL b;
			COMTHROW(currentModel->get_IsInstance(&b));
			isType = (b == VARIANT_FALSE);
			if(isType) {
				COMTHROW(currentModel->get_BaseType(&baseType));
			}
			else{
				COMTHROW(currentModel->get_Type(&baseType));
			}

			if(isType) {
				CComPtr<IMgaModel> type;
				FindDerivedFrom(currentModel,type);
				isSubType = (type != 0);
			}
		}

		SetBgColor();
		needsConnConversion = CreateGuiObjects();
		SetProperties();

		if (GetDocument()->initialCenterObj) {
			COMTHROW(terry->OpenFCO(GetDocument()->initialCenterObj, &centerObj));
			GetDocument()->initialCenterObj = NULL;
		}

		CommitTransaction();

		if (needsConnConversion) {
			ConvertNeededConnections();
			needsConnConversion = false;
		}
	}
	catch(hresult_exception &e) {
		// CrashRpt c51312b4-866b-4a9f-a18d-9928ef9905ef: Close before MessageBox
		frame->SendMessage(WM_CLOSE);
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to open model"),MB_OK | MB_ICONSTOP);
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnInitialUpdate - Unable to open model.\r\n"));
		EndWaitCursor();
		return;
	}

	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnInitialUpdate() - opened model: ")+path+name+_T("\r\n"));

	modelGrid.Clear();
	FillModelGrid();
	AutoRoute();
	ClearConnSpecs();
	if(guiMeta) {
		theApp.UpdateCompList4CurrentKind( guiMeta->name);
		CMainFrame::theInstance->SetPartBrowserMetaModel(guiMeta);
		CMainFrame::theInstance->SetPartBrowserBg(bgColor);
		CMainFrame::theInstance->ChangePartBrowserAspect(currentAspect->index);
	}
	SetScroll();
	SetCenterObject(centerObj);
	initDone = true;
	EndWaitCursor();

	SendOpenModelEvent();
	DragAcceptFiles(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CGMEView printing

#define LOGOSIZE		64 // 115

void CGMEView::PrintMultiLineText(Gdiplus::Graphics* gdip, CDC *pDC, CString txt, int x, int &y, int ry, int xwidth)
{
	Gdiplus::Font* font = graphics.GetGdipFont(GME_PORTNAME_FONT);
	CPoint pt(x,y);
	CSize size = graphics.MeasureText(gdip, txt, pt, font);
	if(size.cx < xwidth) {
		graphics.DrawGdipText(gdip, txt, pt, font, GME_BLACK_COLOR, TA_LEFT | TA_BOTTOM);
	}
	else {
		int incr = 2;
		int lng = txt.GetLength();
		int width = incr;
		for(int start = 0; start < lng; start += width ) {
			for(width = incr; ; width += incr) {
				bool printIt = false;
				CString cur = txt.Mid(start,width);
				if(start + width >= lng)
					printIt = true;
				else {
					CSize size = graphics.MeasureText(gdip, cur, pt, font);
					if(size.cx > xwidth) {
						width -= incr;
						cur = txt.Mid(start,width);
						printIt = true;
					}
				}
				if(printIt) {
					graphics.DrawGdipText(gdip, cur, pt, font, GME_BLACK_COLOR, TA_LEFT | TA_BOTTOM);
					pt.y += (int)(ry / 12);
					break;
				}
			}
		}
	}

	y = pt.y + (int)(ry / 25);
}

void CGMEView::PrintHeader(CDC* pDC, CPrintInfo* pInfo)
{
	PrintHeaderRect(pDC, pInfo->m_rectDraw);
}

void CGMEView::PrintHeaderRect(CDC* pDC, CRect &rectDraw)
{
	/*const static*/ int logdpi = 140;
	int savedID = pDC->SaveDC();

	CString line1;
	CString line2;
	line1 = name;
	line2 = _T("Paradigm: ") + theApp.guiMetaProject->displayedName;
	line2 += _T("     Project: ") + theApp.projectName;
	line2 += _T("     Model: ") + kindDisplayedName;
	line2 += _T("     Aspect: ") + currentAspect->displayedName;
	CString tim;
	{
		struct tm *newtime;
		TCHAR am_pm[] = _T("AM");
		time_t long_time;
		time( &long_time );                /* Get time as long integer. */
		newtime = localtime( &long_time ); /* Convert to local time. */
		if( newtime->tm_hour > 12 )        /* Set up extension. */
			_tcscpy( am_pm, _T("PM") );
		if( newtime->tm_hour > 12 )        /* Convert from 24-hour */
			newtime->tm_hour -= 12;		   /*   to 12-hour clock.  */
		if( newtime->tm_hour == 0 )        /* Set hour to 12 if midnight. */
			newtime->tm_hour = 12;
		tim.Format(_T("%.19s  %s"), asctime( newtime ), am_pm );
	}
	line2 += _T("     Time: ") + tim;

	// Setup mapping mode to have 50 pixel in 1 inch
	pDC->SetMapMode(MM_ISOTROPIC);
	double inchX = pDC->GetDeviceCaps(LOGPIXELSX);
	double inchY = pDC->GetDeviceCaps(LOGPIXELSY);
	pDC->SetWindowExt(logdpi,logdpi);
	pDC->SetViewportExt((int)inchX,(int)inchY);

	int gap = (int)(logdpi / 40);                             // 1/40 inch
	int txty = gap * 6 ;
	int xx = LOGOSIZE/4 + 20;

	Gdiplus::Graphics gdip(pDC->m_hDC);
	gdip.SetPageUnit(Gdiplus::UnitPixel);
	gdip.SetSmoothingMode(m_eEdgeAntiAlias);
	gdip.SetTextRenderingHint(m_eFontAntiAlias);

	PrintMultiLineText(&gdip,pDC,line1,xx,txty,logdpi,rectDraw.Width() - xx);

	txty += 4 * gap;

	CPoint pt = CPoint(LOGOSIZE /4  + 20,txty);
// ??	PrintMultiLineText(&gdip, pDC, line2, LOGOSIZE /4  + 20, txty, logdpi, rectDraw.Width() - xx);
	graphics.DrawGdipText(&gdip,line2,pt,graphics.GetGdipFont(GME_PORTNAME_FONT),GME_BLACK_COLOR,TA_LEFT | TA_BOTTOM);
//	pt = CPoint(rectDraw.right-gap * 2,LOGOSIZE + 20);
//	graphics.DrawGdipText(&gdip,tim,pt,graphics.GetGdipFont(GME_PORTNAME_FONT),GME_BLACK_COLOR,TA_RIGHT | TA_BOTTOM);


	CBitmap logo;
	// TODO: Error checking
	logo.LoadBitmap(IDB_BITMAP_LOGO);
	CDC logoDC;
	// TODO: Error checking
	logoDC.CreateCompatibleDC(pDC);
	logoDC.SetMapMode(MM_TEXT);
	CBitmap * bmp = logoDC.SelectObject(&logo);
	int bits = pDC->GetDeviceCaps(BITSPIXEL);
	if (bits == 1) 
		pDC->SetStretchBltMode(HALFTONE);
//	BOOL ret2 = pDC->StretchBlt(0, txty-LOGOSIZE/4,LOGOSIZE/4,LOGOSIZE/4,&logoDC,0,0,LOGOSIZE, LOGOSIZE, SRCCOPY);
	// TODO: Error checking
	pDC->StretchBlt(0, txty-LOGOSIZE/2,LOGOSIZE/2,LOGOSIZE/2,&logoDC,0,0,LOGOSIZE, LOGOSIZE, SRCCOPY);
	if (bmp)
		bmp = logoDC.SelectObject((CBitmap*)bmp);
	logo.DeleteObject();


	int y = txty;
	CPen pen;
	CPen *oldpen;
	int pw = (int)(logdpi / 48);                              // 1/48 inch
	pen.CreatePen(PS_SOLID,pw,RGB(0,0,0));
	oldpen = pDC->SelectObject(&pen);
	pDC->MoveTo(0,y);
	pDC->LineTo(rectDraw.right,y);

	y += 3 * gap;
	CPoint offset(0,y);
	pDC->LPtoDP(&offset);

	pDC->SelectObject(oldpen);
	pen.DeleteObject();
	pDC->RestoreDC(savedID);
	pDC->DPtoLP(&offset);
	rectDraw.top += (offset.y < logdpi/3)? logdpi/3: offset.y; // one third of an inch
}

BOOL CGMEView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// change the PrintDialog to a customized one
	CGmePrintDialog *gpd = new CGmePrintDialog(this, guiMeta, FALSE);
	if (pInfo->m_pPD)
		delete pInfo->m_pPD;
	pInfo->m_pPD = gpd;
	pInfo->m_pPD->m_pd.Flags =	PD_ALLPAGES |  // actually the current aspect
								PD_ENABLEPRINTTEMPLATE | PD_NONETWORKBUTTON  | 
								PD_USEDEVMODECOPIES | PD_USEDEVMODECOPIESANDCOLLATE |
								PD_ENABLEPRINTHOOK | PD_ENABLESETUPHOOK |
								PD_RETURNDC ;
	pInfo->m_pPD->m_pd.nMinPage = 1;      // one based page numbers
    pInfo->m_pPD->m_pd.nMaxPage = 0xffff; // how many pages is unknown
	pInfo->m_pPD->m_pd.hInstance = AfxGetInstanceHandle();
	pInfo->m_pPD->m_pd.lpPrintTemplateName = MAKEINTRESOURCE(IDD_PRINT_DIALOG);

	if (!CScrollZoomView::DoPreparePrinting(pInfo))
		return FALSE;

	HDC hdc = pInfo->m_pPD->GetPrinterDC();
	CDC cdc;
	cdc.Attach(hdc);
    LPDEVMODE devmode = pInfo->m_pPD->GetDevMode();
	devmode->dmOrientation = (gpd->IsPortrait())? (short)DMORIENT_PORTRAIT: (short)DMORIENT_LANDSCAPE;
	m_scalePrnPages = (short)gpd->NumOfPages();
	pInfo->m_pPD->m_pd.nMinPage = 1;      
	m_fullPrnAspNum = gpd->NumOfSelAspects();
    pInfo->m_pPD->m_pd.nMaxPage = (short)(m_scalePrnPages * m_scalePrnPages * m_fullPrnAspNum); 
	pInfo->m_pPD->m_pd.nToPage = (unsigned short)(0xffff);
	cdc.ResetDC(devmode);
	cdc.Detach();
	m_currPrnNumAsp = 0;
	m_prnpos = NULL;
	m_lastPrnPage = 0; // hack
	return TRUE;
}

void CGMEView::PrepareAspectPrn(CPrintInfo* pInfo)
{
	CGuiMetaAspect *asp = NULL;
	CGmePrintDialog* pdlg = (CGmePrintDialog*)(pInfo->m_pPD);
	bool changeAsp = false;

	if (m_lastPrnPage == pInfo->m_nCurPage)
		return;

	if (pdlg->IsAllAspect()  ||  pdlg->IsSelAspect())
	{ // all or some Aspect selected
		if (!m_currPrnNumAsp)
		{  // find the first Aspect
			SaveCurrAsp();
			m_prnpos = guiMeta->aspects.GetHeadPosition();
		}
		if (m_scalePrnPages == 1  ||  
				pInfo->m_nCurPage % (m_scalePrnPages*m_scalePrnPages) == 1)
		{  // change aspect
			asp = guiMeta->aspects.GetNext(m_prnpos);
			if (pdlg->IsSelAspect()) 
			{
				while (!pdlg->IsSelectedAspect(asp->name))
				{
					if (m_prnpos)
						asp = guiMeta->aspects.GetNext(m_prnpos);
					else
					{
						asp = NULL;
						ASSERT(asp);
						break;
					}
				}
			}
			m_currPrnNumAsp++;
			changeAsp = true;
		}
		if (changeAsp)
		{
			if (ChangePrnAspect(asp->name))
			{
				if (pdlg->IsAutorotate()) // it doen't work in OnPrint only in OnPrepareDC
				{ // it works only before StartPage
					HDC hdc = pInfo->m_pPD->GetPrinterDC();
					CDC cdc;
					cdc.Attach(hdc);
					CRect extent, objext, annext;
					CGuiObject::GetExtent(children,objext);
					CGuiAnnotator::GetExtent(annotators,annext);
					extent.UnionRect(&objext, &annext);
					LPDEVMODE devmode = pInfo->m_pPD->GetDevMode();
					devmode->dmFields = DM_ORIENTATION;
					if (extent.Width() > extent.Height()) // rotate it
						devmode->dmOrientation = (short)DMORIENT_LANDSCAPE;
					else
						devmode->dmOrientation = (short)DMORIENT_PORTRAIT;
					// TODO: Error checking
					cdc.ResetDC(devmode); // done for the AttribDC
					cdc.Detach();
				}
			}
		}
	}
	m_lastPrnPage = pInfo->m_nCurPage;
}

void CGMEView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnPrint\r\n"));
	CGmePrintDialog* pdlg = (CGmePrintDialog*)(pInfo->m_pPD);
	int headerY = 0;

	if (pdlg->HasHeader())
	{
		PrintHeader(pDC, pInfo);
		headerY = pInfo->m_rectDraw.top;
	}
	// setup the DC according to the current page number - multipage
	int line = (int)ceil((double)(pInfo->m_nCurPage % (m_scalePrnPages * m_scalePrnPages)) / m_scalePrnPages);
	int col = pInfo->m_nCurPage % (m_scalePrnPages * m_scalePrnPages) % m_scalePrnPages;
	col = (col)? col: m_scalePrnPages;
	line = (line)? line: m_scalePrnPages;
	line--; // starts with 0
	col--;

	{
		pDC->SetMapMode(MM_ISOTROPIC);
		CRect extent, objext, annext;
		CGuiObject::GetExtent(children,objext);
		CGuiAnnotator::GetExtent(annotators,annext);
		extent.UnionRect(&objext, &annext);
		extent.right = (int)(extent.right*EXTENT_ERROR_CORR); // ??
		extent.bottom = (int)(extent.bottom*EXTENT_ERROR_CORR); // ??
		double wpage, hpage;
		double wmargin = GetDeviceCaps(pDC->m_hDC,PHYSICALOFFSETX);
		double hmargin = GetDeviceCaps(pDC->m_hDC,PHYSICALOFFSETY);
		wpage = GetDeviceCaps(pDC->m_hDC,PHYSICALWIDTH) - 2*wmargin;
		hpage = GetDeviceCaps(pDC->m_hDC,PHYSICALHEIGHT)- 2*hmargin;

		// headerY was calculated with another scaling
		pDC->SetWindowExt(extent.right, extent.bottom);
		int devheader = (headerY)? (int)(pDC->GetDeviceCaps(LOGPIXELSY)/3): 0; // I know it is 1/3 inch - pfujj

		pDC->SetViewportExt((int)(m_scalePrnPages*wpage), (int)(m_scalePrnPages*(hpage-devheader)));
		pDC->SetViewportOrg((int)(-wpage*col), (int)(-(hpage-devheader)*line) +devheader);
	}

	OnDraw(pDC);

	// restore Aspect
	if (pInfo->m_pPD->m_pd.nMaxPage == pInfo->m_nCurPage  && m_currPrnNumAsp == m_fullPrnAspNum)
	{ 
		CGuiMetaAspect* lastcurr = GetSavedAsp();
		ChangePrnAspect(lastcurr->name);
		Invalidate();
	}
}

void CGMEView::OnDestroy()
{
	// Deactivate the item on destruction; this is important
	// when a splitter view is being used.
	if (timerID) {
		KillTimer(timerID);
		timerID = 0;
	}
	CMainFrame* main = (CMainFrame*)AfxGetMainWnd();

	CDocument* doc = GetDocument();
	POSITION pos = doc->GetFirstViewPosition();
	if (pos != NULL  &&  doc->GetNextView(pos)  &&  pos == NULL)
	{
		CRect extent(0,0,0,0), target(0,0,0,0); // terge
		main->m_panningWindow.SetBitmapDC(this->m_hWnd, NULL, NULL, extent, target, bgColor);
	}

	CScrollZoomView::OnDestroy();
}

void CGMEView::OnSize(UINT nType, int cx, int cy)
{
	m_overlay = nullptr;
	CScrollZoomView::OnSize(nType, cx, cy);
}

/////////////////////////////////////////////////////////////////////////////
// CGMEView diagnostics

#ifdef _DEBUG
void CGMEView::AssertValid() const
{
	CScrollZoomView::AssertValid();
}

void CGMEView::Dump(CDumpContext& dc) const
{
	CScrollZoomView::Dump(dc);
}

CGMEDoc* CGMEView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGMEDoc)));
	return (CGMEDoc*)m_pDocument;
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGMEView custom operations

void CGMEView::BeginTransaction(transactiontype_enum mode)
{
	VERIFY(inTransaction >= 0);
	if(!inEventHandler && ++inTransaction == 1) {
		inRWTransaction = (mode == TRANSACTION_GENERAL || mode == TRANSACTION_NON_NESTED);
		COMTHROW(theApp.mgaProject->BeginTransaction(terry,mode));
	}
}

void CGMEView::CommitTransaction()
{
	if(inEventHandler)
		return;
	VERIFY(inTransaction > 0);
	if(inTransaction == 1)
		COMTHROW(theApp.mgaProject->CommitTransaction());
	inTransaction--;
	inRWTransaction = false;
}

void CGMEView::__CommitTransaction()
{
	if(inEventHandler)
		return;
	VERIFY(inTransaction > 0);
	if(inTransaction == 1)
		theApp.mgaProject->__CommitTransaction();
	inTransaction--;
	inRWTransaction = false;
}

void CGMEView::AbortTransaction(HRESULT hr)
{
	if(inEventHandler)
		return;
	VERIFY(inTransaction > 0);
	if(--inTransaction > 0)
		throw hresult_exception(hr);
	else
		theApp.mgaProject->AbortTransaction();
}


bool CGMEView::SendOpenModelEvent()
{
	bool ok = true;
	try {
		BeginTransaction();

		COMTHROW(currentModel->SendEvent(OBJEVENT_OPENMODEL));

		__CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		ok = false;
	}
	catch(_com_error& e) {
		AbortTransaction(e.Error());
		ok = false;
		CString error = _T("Notifying Addons of OpenModel failed");
		if (e.Description().length() != 0)
		{
			error += _T(": ");
			error += static_cast<const TCHAR*>(e.Description());
		}
		CGMEEventLogger::LogGMEEvent(error + _T("\r\n"));
		AfxMessageBox(error,MB_ICONSTOP | MB_OK);
	}
	return ok;
}

bool CGMEView::SendCloseModelEvent()
{
	bool ok = true;
	try {
		BeginTransaction();

		COMTHROW(currentModel->SendEvent(OBJEVENT_CLOSEMODEL));

		CommitTransaction();
	}
	catch(hresult_exception &e) {
		try {
			AbortTransaction(e.hr);
		} catch(hresult_exception &e) {
		} // Our transaction count was wrong. What else can we do?
		ok = false;
	}
	catch(_com_error& e) {
		AbortTransaction(e.Error());
		ok = false;
		CString error = _T("Notifying Addons of CloseModel failed");
		if (e.Description().length() != 0)
		{
			error += _T(": ");
			error += static_cast<const TCHAR*>(e.Description());
		}
		CGMEEventLogger::LogGMEEvent(error + _T("\r\n"));
		AfxMessageBox(error,MB_ICONSTOP | MB_OK);
	}
	return ok;
}

bool CGMEView::SendMouseOver4Object( CGuiObject * pObject)
{
	bool ok = true;
	try {
		BeginTransaction();

		if( pObject && pObject->mgaFco)
		{
			long oStatus;
			COMTHROW(pObject->mgaFco->get_Status(&oStatus));
			if(oStatus == OBJECT_EXISTS) // make sure it has not been deleted since then
				COMTHROW( pObject->mgaFco->SendEvent(OBJEVENT_MOUSEOVER));
		}

		CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		ok = false;
	}
	return ok;
}

bool CGMEView::SendSelecEvent4Object( CGuiObject* pSelection)
{
	bool ok = true;

	std::list<CGuiObject*>::iterator found = std::find( m_lstUnselect.begin(), m_lstUnselect.end(), pSelection);
	if( found != m_lstUnselect.end()) // it can't be unsel and sel at the same time
	{
		m_lstUnselect.erase( found);
	}

	found = std::find( m_lstSelect.begin(), m_lstSelect.end(), pSelection);
	if( found != m_lstSelect.end()) // it should not be twice in sel
	{
		m_lstSelect.erase( found);
	}

	m_lstSelect.push_back( pSelection);
	return ok;
}

bool CGMEView::SendUnselEvent4Object( CGuiObject* pUnselection)
{
	bool ok = true;

	std::list<CGuiObject*>::iterator found = std::find( m_lstSelect.begin(), m_lstSelect.end(), pUnselection);
	if( found != m_lstSelect.end()) // it can't be unsel and sel at the same time
	{
		m_lstSelect.erase( found);
	}

	found = std::find( m_lstUnselect.begin(), m_lstUnselect.end(), pUnselection);
	if( found != m_lstUnselect.end()) // it should not be twice in unsel
	{
		m_lstUnselect.erase( found);
	}

	m_lstUnselect.push_back( pUnselection);
	return ok;
}

bool CGMEView::SendSelecEvent4List( CGuiObjectList* pSelection)
{
	bool ok = true;
	// send select event for each object in list
	POSITION pos = pSelection->GetHeadPosition();
	CGuiObject *obj;
	while(pos) {
		obj = pSelection->GetNext(pos);
		bool ok2 = SendSelecEvent4Object( obj);
		ok = ok && ok2;
	}
	return ok;
}


bool CGMEView::SendUnselEvent4List( CGuiObjectList* pUnselection)
{
	bool ok = true;
	// send deselect event for each object in list
	POSITION pos = pUnselection->GetHeadPosition();
	CGuiObject *obj;
	while(pos) {
		obj = pUnselection->GetNext(pos);
		bool ok2 = SendUnselEvent4Object( obj);
		ok = ok && ok2;
	}
	return ok;
}

bool CGMEView::SendNow(bool onlyDecoratorNotification)
{
	if (m_lstSelect.empty() && m_lstUnselect.empty()) {
		return false;
	} else {
		if (selectedConnection != NULL) {
			selectedConnection->SetSelect(false);
			selectedConnection = NULL;
		}
	}

	bool ok = true;
	try {
		if (!onlyDecoratorNotification)
			BeginTransaction();

		std::list<CGuiObject*>::iterator it;
		for( it = m_lstUnselect.begin(); it != m_lstUnselect.end(); ++it)
		{
			CGuiObject * pUnselection = *it;
			if( pUnselection && pUnselection->mgaFco)
			{
				long oStatus;
				COMTHROW(pUnselection->mgaFco->get_Status(&oStatus));
				if(oStatus == OBJECT_EXISTS) {	// make sure it has not been deleted since then
					if (!onlyDecoratorNotification)
						COMTHROW( pUnselection->mgaFco->SendEvent(OBJEVENT_DESELECT));
					// Sending decorator events (for efficiency)
					CGuiAspect* pAspect = pUnselection->GetCurrentAspect();
					if (pAspect != NULL) {
						CComQIPtr<IMgaElementDecorator> newDecorator(pAspect->GetDecorator());
						if (newDecorator)
							HRESULT retVal = newDecorator->SetSelected(VARIANT_FALSE);
					}
				}
			}
		}

		for( it = m_lstSelect.begin(); it != m_lstSelect.end(); ++it)
		{
			CGuiObject * pSelection = *it;
			if( pSelection && pSelection->mgaFco)
			{
				long oStatus;
				COMTHROW(pSelection->mgaFco->get_Status(&oStatus));
				if (oStatus == OBJECT_EXISTS) {
					if (!onlyDecoratorNotification)
						COMTHROW( pSelection->mgaFco->SendEvent(OBJEVENT_SELECT));
					// Sending decorator events (for efficiency)
					CGuiAspect* pAspect = pSelection->GetCurrentAspect();
					if (pAspect != NULL) {
						CComQIPtr<IMgaElementDecorator> newDecorator(pAspect->GetDecorator());
						if (newDecorator)
							HRESULT retVal = newDecorator->SetSelected(VARIANT_TRUE);
					}
				}
			}
		}

		if (!onlyDecoratorNotification) {
			m_lstSelect.clear();
			m_lstUnselect.clear();

			CommitTransaction();
		}
	}
	catch(hresult_exception &e) {
		if (!onlyDecoratorNotification)
			AbortTransaction(e.hr);
		ok = false;
	}
	return ok;
}

void CGMEView::AddAnnotationToSelectionHead(CGuiAnnotator* ann)
{
	AddAnnotationToSelection(ann, true);
}

void CGMEView::AddAnnotationToSelectionTail(CGuiAnnotator* ann)
{
	AddAnnotationToSelection(ann, false);
}

void CGMEView::AddAnnotationToSelection(CGuiAnnotator* ann, bool headOrTail)
{
	ClearConnectionSelection();
	CComPtr<IMgaElementDecorator> decorator = ann->GetDecorator(currentAspect->index);
	if (decorator)
		HRESULT retVal = decorator->SetSelected(VARIANT_TRUE);
	if (headOrTail)
		selectedAnnotations.AddHead(ann);
	else
		selectedAnnotations.AddTail(ann);
}

void CGMEView::RemoveAllAnnotationFromSelection(void)
{
	POSITION pos = selectedAnnotations.GetHeadPosition();
	CGuiAnnotator *ann;
	while (pos) {
		ann = selectedAnnotations.GetNext(pos);
		CComPtr<IMgaElementDecorator> decorator = ann->GetDecorator(currentAspect->index);
		if (decorator)
			HRESULT retVal = decorator->SetSelected(VARIANT_FALSE);
	}
	selectedAnnotations.RemoveAll();
}

void CGMEView::RemoveAnnotationFromSelectionHead(void)
{
	CGuiAnnotator* head = selectedAnnotations.GetHead();
	CComPtr<IMgaElementDecorator> decorator = head->GetDecorator(currentAspect->index);
	if (decorator)
		HRESULT retVal = decorator->SetSelected(VARIANT_FALSE);
	selectedAnnotations.RemoveHead();
}

void CGMEView::RemoveAnnotationFromSelection(POSITION annPos)
{
	CGuiAnnotator* ann = selectedAnnotations.GetAt(annPos);
	CComPtr<IMgaElementDecorator> decorator = ann->GetDecorator(currentAspect->index);
	if (decorator)
		HRESULT retVal = decorator->SetSelected(VARIANT_FALSE);
	selectedAnnotations.RemoveAt(annPos);
}

void CGMEView::ClearConnectionSelection(void)
{
	if (selectedConnection != NULL) {
		selectedConnection->SetSelect(false);
		selectedConnection = NULL;
	}
}

void CGMEView::UpdateConnectionSelection(int aspect)
{
	if (selectedConnection != NULL) {
		if(!selectedConnection->IsVisible(aspect)) {
			selectedConnection->SetSelect(false);
			selectedConnection = NULL;
		}
	}
}

void CGMEView::ResetParent(bool doInvalidate)
{
#if !defined (ACTIVEXGMEVIEW)
	CGMEView *parentView = GetDocument()->FindView(parent);
	if(parentView) {
		parentView->Reset(doInvalidate);
		parentView->Invalidate();
	}
#endif
}

CGuiFco* CGMEView::CreateGuiObject(CComPtr<IMgaFCO>& fco, CGuiFcoList* objList, CGuiConnectionList* connList)
{
	CComPtr<IMgaMetaRole> role;
	COMTHROW(fco->get_MetaRole(&role));
	objtype_enum tp;
	COMTHROW(fco->get_ObjType(&tp));
	CGuiFco* guiFco;
	bool isCGuiConnection = false;
	if (tp == OBJTYPE_MODEL) {
		guiFco = new CGuiModel(fco, role, this, guiMeta->NumberOfAspects());
		((CGuiModel *)guiFco)->InitObject(this);
		((CGuiModel *)guiFco)->SetAspect(currentAspect->index);
	} else if(tp == OBJTYPE_REFERENCE) {
		// Immediate referred object
		CComPtr<IMgaFCO> refd;
		CComPtr<IMgaReference> ref;
		COMTHROW(fco.QueryInterface(&ref));
		COMTHROW(ref->get_Referred(&refd));

		// Final referred (non-reference) object
		CComPtr<IMgaFCO> termRefd;
		CComObjPtr<IMgaFCOs> refChain;
		COMTHROW(refChain.CoCreateInstance(L"Mga.MgaFCOs"));
// This seems to fix the ref-ref problem
// the GetRefereeChain() puts in this fco
//		COMTHROW(refChain->Append(fco));
		GetRefereeChain(refChain,fco);

		CComPtr<IMgaFCO> lastRef;
		long refChainCnt;
		COMTHROW(refChain->get_Count(&refChainCnt));
		COMTHROW(refChain->get_Item(refChainCnt, &lastRef));

		ref = NULL;
		COMTHROW(lastRef.QueryInterface(&ref));
		COMTHROW(ref->get_Referred(&termRefd));

		objtype_enum rtp = OBJTYPE_NULL;
		if (termRefd) {
			COMTHROW(termRefd->get_ObjType(&rtp));
		}

		if (rtp == OBJTYPE_MODEL) {
			guiFco = new CGuiCompoundReference(fco, role, this, guiMeta->NumberOfAspects(), refd, termRefd);
			((CGuiCompoundReference *)guiFco)->InitObject(this);
			((CGuiCompoundReference *)guiFco)->SetAspect(currentAspect->index);
		} else {
			guiFco = new CGuiReference(fco, role, this, guiMeta->NumberOfAspects(), refd, termRefd);
			((CGuiReference *)guiFco)->InitObject(this);
			((CGuiReference *)guiFco)->SetAspect(currentAspect->index);
		}	
	} else if(tp == OBJTYPE_SET) {
		guiFco = new CGuiSet(fco, role, this, guiMeta->NumberOfAspects());
		((CGuiSet *)guiFco)->InitObject(this);
		((CGuiSet *)guiFco)->SetAspect(currentAspect->index);
		if (objList != NULL && !currentSetID.IsEmpty()) {
			CString setID;
			fco->get_ID(PutOut(setID));
			if(setID == currentSetID) {
				if (guiFco != NULL)
					currentSet = guiFco->dynamic_cast_CGuiSet();
			}
		}
	} else if (tp == OBJTYPE_CONNECTION) {
		guiFco = new CGuiConnection(fco, role, this, guiMeta->NumberOfAspects(), false);
		isCGuiConnection = true;
	} else {
		guiFco = new CGuiObject(fco, role, this, guiMeta->NumberOfAspects());
		((CGuiObject *)guiFco)->InitObject(this);
		((CGuiObject *)guiFco)->SetAspect(currentAspect->index);
	}
	guiFco->SetAspect(currentAspect->index);
	CComBSTR bstr;
	COMTHROW(fco->get_Name(&bstr));
	if (!isCGuiConnection) {
		CGuiObject* guiObj = static_cast<CGuiObject*> (guiFco);
		guiObj->ReadAllLocations();
		guiObj->name = bstr;
		if (objList != NULL)
			objList->AddTail(guiObj);
	} else {
		CGuiConnection* guiConn = static_cast<CGuiConnection*> (guiFco);
		VERIFY(guiConn);
		guiConn->name = bstr;
		if (objList != NULL)
			objList->AddTail(guiConn);
		if (connList != NULL)
			connList->AddTail(guiConn);
	}
	return guiFco;
}

// ??
void CGMEView::CreateGuiObjects(CComPtr<IMgaFCOs>& fcos, CGuiFcoList& objList, CGuiConnectionList& connList)
{
	CComPtr<IMgaFCO> fco;
	MGACOLL_ITERATE(IMgaFCO,fcos) {
		fco = MGACOLL_ITER;
		CGuiFco* guiFco = CreateGuiObject(fco, &objList, &connList);
	}
	MGACOLL_ITERATE_END;
}

// ??
bool CGMEView::CreateGuiObjects()
{
	COMTHROW(currentModel->get_Name(PutOut(name)));

	CreateAnnotators();

	CComPtr<IMgaFCOs> fcos;
	COMTHROW(currentModel->get_ChildFCOs(&fcos));

	{
		isModelAutoRouted = theApp.useAutoRouting;	// update view's value to the app settings

		CComBSTR pathBstr = MODELAUTOROUTING;
		CComBSTR bstrVal;
		CString val;
		COMTHROW(currentModel->get_RegistryValue(pathBstr, PutOut(val)));
		if (!val.IsEmpty()) {
			if (val == _T("false"))
				isModelAutoRouted = false;
			else
				isModelAutoRouted = true;
		}
	}

	CreateGuiObjects(fcos,children,connections);

	InitSets();
	if(GetDocument()->GetEditMode() == GME_SET_MODE && currentSet) {
		CGuiFco::GrayOutFcos(children,true);
		CGuiFco::GrayOutFcos(connections,true);
		CGuiAnnotator::GrayOutAnnotations(annotators, true);
		currentSet->GrayOutMembers(false);
		currentSet->GrayOut(false);
	}

	CGuiFco::SetAspect(children,currentAspect->index);
	CGuiAnnotator::SetAspect(annotators, currentAspect->index);
	ResolveConnections();

	CComPtr<IMgaObject> cont;
	objtype_enum tp;
	COMTHROW(currentModel->GetParent(&cont,&tp));
	if(tp == OBJTYPE_MODEL) {
		parent = 0;
		COMTHROW(cont.QueryInterface(&parent));
	}

	return IsConnectionConversionNeeded();
}

void CGMEView::CreateAnnotators(CComPtr<IMgaRegNodes> &regNodes, CGuiAnnotatorList &annList)
{
	MGACOLL_ITERATE(IMgaRegNode, regNodes) {
		CComPtr<IMgaRegNode> anNode;
		anNode = MGACOLL_ITER;
		bool arch = !isSubType && isType; // archetype if not subtype and not instance (type)
		if( arch || (!arch && CGuiAnnotator::Showable(anNode, baseType)))
		{
			CGuiAnnotator *annotator = new CGuiAnnotator(currentModel, anNode, this, guiMeta->NumberOfAspects());
			annList.AddTail(annotator);
		}
	}
	MGACOLL_ITERATE_END;
}

void CGMEView::CreateAnnotators()
{
	try {
		CComPtr<IMgaRegNode> anRoot;
		CComBSTR path(AN_ROOT);
		COMTHROW(currentModel->get_RegistryNode(path, &anRoot));
		if (anRoot != NULL) {
			CComPtr<IMgaRegNodes> anNodes;
			COMTHROW(anRoot->get_SubNodes(VARIANT_TRUE, &anNodes));
			CreateAnnotators(anNodes, annotators);
		}
	}
	catch (hresult_exception &) {
	}
}

void CGMEView::ResetPartBrowser()
{
	guiMeta->ResetParts();
	CMainFrame::theInstance->SetPartBrowserMetaModel(guiMeta);
	CMainFrame::theInstance->SetPartBrowserBg(bgColor);
	CMainFrame::theInstance->RePaintPartBrowser();
}


void CGMEView::Reset(bool doInvalidate)
{
//	CGMEView* gmeviewA = (CGMEView*)GetActiveView();
//	if (gmeviewA)
	if (m_isActive)
	{
		TRACE(_T("CGMEView::Reset GetActiveView\n"));
	}
	CComPtr<IMgaFCO> selConn;
	if (selectedConnection != NULL)
		selConn = selectedConnection->mgaFco;
	try {
		BeginTransaction(TRANSACTION_READ_ONLY);
		validGuiObjects = false;

		BeginWaitCursor();

		// store the ids of the objects placed into the notifiable objects
		CStringList lstSelBuffIDs;
		CStringList lstUnsBuffIDs;
		CString     cntxSelID;
		CString     cntxAnnID;
		if( contextSelection)
			cntxSelID = contextSelection->id;
		if( contextAnnotation)
			cntxAnnID = contextAnnotation->id;

		for( std::list<CGuiObject*>::iterator iti = m_lstSelect.begin(); iti != m_lstSelect.end(); ++iti)
			lstSelBuffIDs.AddTail( (*iti)->id);

		for( std::list<CGuiObject*>::iterator itj = m_lstUnselect.begin(); itj != m_lstUnselect.end(); ++itj)
			lstUnsBuffIDs.AddTail( (*itj)->id);

		CStringList selIDs;
		CStringList selAnIDs;
		if(newObjectIDs.IsEmpty()) {
			POSITION pos = selected.GetHeadPosition();
			while(pos) {
				CGuiObject *obj = selected.GetNext(pos);
				selIDs.AddTail(obj->id);
			}
			pos = selectedAnnotations.GetHeadPosition();
			while(pos) {
				selAnIDs.AddTail(selectedAnnotations.GetNext(pos)->id);
			}
		}
		else {
			selIDs.AddHead(&newObjectIDs);
			newObjectIDs.RemoveAll();
		}

		currentSetID.Empty();
		if(currentSet) {
			currentSet->mgaFco->get_ID(PutOut(currentSetID));
			currentSet = 0;
		}

		ClearConnSpecs();
		tmpConnectMode = false;

		POSITION pos = children.GetHeadPosition();
		while(pos) {
			CGuiFco* fco = children.GetNext(pos);
			ASSERT(fco != NULL);
			CGuiObject* obj = fco->dynamic_cast_CGuiObject();
			if(obj && obj->IsVisible()) {
				if(obj->GetRouterBox())					// it may be a new object not yet routed
					// FIXME: so slow. Can't we just create a new router?
					obj->RemoveFromRouter(router);
			}
			delete obj;
		}

		RemoveAllAnnotationFromSelection();
		pos = annotators.GetHeadPosition();
		while(pos) {
			delete annotators.GetNext(pos);
		}

		ClearConnectionSelection();
		pos = connections.GetHeadPosition();
		while(pos) {
			CGuiConnection *conn = connections.GetNext(pos);
			//vt THIS MIGHT BE UNNECESSARY
			conn->RemoveFromRouter(router);
			delete conn;
		}


		m_lstSelect.clear(); // clear the contents of these buffers [will be refilled soon] 
		m_lstUnselect.clear();
		// these will be recalculated also
		contextAnnotation = nullptr;
		contextSelection = nullptr;
		contextPort = nullptr;
		selectedContextConnection = nullptr;
		connTmp = nullptr;
		children.RemoveAll();
		annotators.RemoveAll();
		connections.RemoveAll();
		selected.RemoveAll(); // we don't call here the this->SendUnselEvent4List( &selected); because it might contain freshly deleted objects, which can't be notified


		// Now build up new objectset

		SetBgColor();
		CreateGuiObjects();
		SetProperties();

		// Note: Refresh type/subtype/instance property - if it crashes, contact Peter
		{
			baseType = NULL;
			VARIANT_BOOL b;
			COMTHROW(currentModel->get_IsInstance(&b));
			isType = (b == VARIANT_FALSE);
			if(isType) {
				COMTHROW(currentModel->get_BaseType(&baseType));
			}
			else{
				COMTHROW(currentModel->get_Type(&baseType));
			}

			if(isType) {
				CComPtr<IMgaModel> type;
				FindDerivedFrom(currentModel,type);
				isSubType = (type != 0);
			}
		}
		// EndNote

		if( !(selIDs.IsEmpty() && lstSelBuffIDs.IsEmpty() && lstUnsBuffIDs.IsEmpty() && cntxSelID.IsEmpty()))
		{
			// filling up selected, m_lstSelect and m_lstUnselect lists
			POSITION oPos = children.GetHeadPosition();
			while(oPos) {
				CGuiFco* fco = children.GetNext(oPos);
				ASSERT(fco != NULL);
				CGuiObject* obj = fco->dynamic_cast_CGuiObject();
				if( obj)
				{
					if( !cntxSelID.IsEmpty() && obj->id == cntxSelID)
						contextSelection = obj;
					
					POSITION sPos = selIDs.GetHeadPosition();
					while( sPos) {
						CString id = selIDs.GetNext( sPos);
						if( id == obj->id)
						{
							selected.AddTail( obj);
							sPos = 0;
						}
					}

					bool found = false; // will save time because if obj is in lstSelBuffIDs (m_lstSelect), it can't be in lstUnsBuffIDs (m_lstUnselect)
					sPos = lstSelBuffIDs.GetHeadPosition();
					while( !found && sPos) {
						CString id = lstSelBuffIDs.GetNext( sPos);
						if( id == obj->id)
						{
							m_lstSelect.push_back( obj);
							found = true;
						}
					}

					sPos = lstUnsBuffIDs.GetHeadPosition();
					while( !found && sPos) {
						CString id = lstUnsBuffIDs.GetNext( sPos);
						if( id == obj->id)
						{
							m_lstUnselect.push_back( obj);
							found = true;
						}
					}
				}
			}
		}

		// selected was filled up previously like this:
		// now it is done along with m_lstSelect, m_lstUnselect
		//pos = selIDs.GetHeadPosition();
		//while(pos) {
		//	CString id = selIDs.GetNext(pos);
		//	POSITION oPos = children.GetHeadPosition();
		//	while(oPos) {
		//		CGuiFco* fco = children.GetNext(oPos);
		//		ASSERT(fco != NULL);
		//		CGuiObject* obj = fco->dynamic_cast_CGuiObject();
		//		if(obj) {
		//			if(id == obj->id)
		//				selected.AddTail(obj); // this->SendSelecEvent4Object( obj); omitted because of a READONLY transaction
		//		}
		//	}
		//}

		// selectedAnnotations was filled up previously like this:
		//pos = selAnIDs.GetHeadPosition();
		//while(pos) {
		//	CString id = selAnIDs.GetNext(pos);
		//	POSITION oPos = annotators.GetHeadPosition();
		//	while(oPos) {
		//		CGuiAnnotator *ann = annotators.GetNext(oPos);
		//		if (id == ann->id) {
		//			AddAnnotationToSelectionTail(ann);
		//		}
		//	}
		//}		

		pos = annotators.GetHeadPosition();
		while(pos) {
			CGuiAnnotator *ann = annotators.GetNext(pos);
			POSITION oPos = selAnIDs.GetHeadPosition();
			while(oPos) {
				CString id = selAnIDs.GetNext(oPos);
				if (id == ann->id) {
					AddAnnotationToSelectionTail(ann);
				}
			}
			if( !cntxAnnID.IsEmpty() && cntxAnnID == ann->id)
				contextAnnotation = ann;
		}

		SendNow(true);

		try {
			POSITION pos = selected.GetHeadPosition();
			while (pos) {
				CGuiObject* go = selected.GetNext(pos);
				if (go && go->mgaFco) {
					// Sending decorator events (for efficiency)
					CGuiAspect* pAspect = go->GetCurrentAspect();
					if (pAspect != NULL) {
						CComQIPtr<IMgaElementDecorator> newDecorator(pAspect->GetDecorator());
						if (newDecorator)
							HRESULT retVal = newDecorator->SetSelected(VARIANT_TRUE); // FIXME: decorators expect a transaction to be open
					}
				}
			}
		}
		catch (hresult_exception&) {
			AfxMessageBox(_T("Unable to refresh selected status to decorators"));
			CGMEEventLogger::LogGMEEvent(_T("CGMEView::Reset - Unable to refresh selected status to decorators.\r\n"));
		}

		if (selConn != NULL) {
			POSITION pos = connections.GetHeadPosition();
			while(pos) {
				CGuiConnection *conn = connections.GetNext(pos);
				if (conn->mgaFco == selConn) {
					selectedConnection = conn;
					conn->SetSelect(true);
					break;
				}
			}
		}
		CommitTransaction();
	}
	catch (hresult_exception& e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to refresh model"));
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::Reset - Unable to refresh model.\r\n"));
		frame->PostMessage(WM_CLOSE);
		EndWaitCursor();
		return;
	}


	Invalidate(doInvalidate);
	AutoRoute();
	SetScroll(); // TODO: will this work?
	DoPannWinRefresh();

	EndWaitCursor();
}

void CGMEView::InitSets()
{
	POSITION pos = children.GetHeadPosition();
	while(pos) {
		CGuiFco* fco = children.GetNext(pos);
		ASSERT(fco != NULL);
		CGuiSet* set = fco->dynamic_cast_CGuiSet();
		if(set)
			set->Init(children,connections);
	}
}

void CGMEView::ChangeAttrPrefObjs(CGuiObjectList &objlist)
{
	CComPtr<IMgaObjects> mgaobjs;
	COMTHROW(mgaobjs.CoCreateInstance(L"Mga.MgaObjects"));
	if (objlist.GetCount() == 0)
	{
		COMTHROW(mgaobjs->Append(currentModel));
	}
	else
	{
		POSITION pos = objlist.GetHeadPosition();
		while (pos) {
			CGuiObject *guiObj = objlist.GetNext(pos);
			CComPtr<IMgaObject> mgaobj;
			COMTHROW(guiObj->mgaFco.QueryInterface(&mgaobj));
			COMTHROW(mgaobjs->Append(mgaobj));
		}
	}

	CGMEObjectInspector::theInstance->SetObjects(mgaobjs);
}

void CGMEView::ChangeAttrPrefFco(CGuiFco* guiFco)
{
	ASSERT(guiFco != NULL);
	CGuiConnection* conn = guiFco->dynamic_cast_CGuiConnection();
	if (conn != NULL) {
		if (selectedConnection != conn) {
			if (selectedConnection != NULL)
				selectedConnection->SetSelect(false);
			conn->SetSelect(true);
			selectedConnection = conn;
			if (isLeftMouseButtonDown)
				isConnectionJustSelected = true;
		}
	} else {
		if (selectedConnection != NULL)
			selectedConnection->SetSelect(false);
		selectedConnection = NULL;
	}

	CComPtr<IMgaObjects> mgaobjs;
	COMTHROW(mgaobjs.CoCreateInstance(L"Mga.MgaObjects"));
	CComPtr<IMgaObject> mgaobj;
	COMTHROW(guiFco->mgaFco.QueryInterface(&mgaobj));
	COMTHROW(mgaobjs->Append(mgaobj));

	CGMEObjectInspector::theInstance->SetObjects(mgaobjs);
}

void CGMEView::ChangeAttrPrefFco() // currentModel
{
	if (selectedConnection != NULL)
		selectedConnection->SetSelect(false);
	selectedConnection = NULL;

	CComPtr<IMgaObjects> mgaobjs;
	COMTHROW(mgaobjs.CoCreateInstance(L"Mga.MgaObjects"));
	CComPtr<IMgaObject> mgaobj;
	COMTHROW(currentModel.QueryInterface(&mgaobj));
	COMTHROW(mgaobjs->Append(mgaobj));

	CGMEObjectInspector::theInstance->SetObjects(mgaobjs);
}

void CGMEView::ShowProperties(CGuiFco* guiFco)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ShowProperties(")+guiFco->GetName()+_T(" ")+guiFco->GetID()+_T(")\r\n"));
    ChangeAttrPrefFco(guiFco);

	CGMEObjectInspector::theInstance->ShowPanel(2);
}

void CGMEView::ShowProperties()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ShowProperties()\r\n"));
    ChangeAttrPrefFco();

	CGMEObjectInspector::theInstance->ShowPanel(2);
}

void CGMEView::ShowAttributes(CGuiFco* guiFco)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ShowAttributes(")+guiFco->GetName()+_T(" ")+guiFco->GetID()+_T(")\r\n"));
	ChangeAttrPrefFco(guiFco);

	CGMEObjectInspector::theInstance->ShowPanel(0);
}

// TODO
void CGMEView::ShowAttributes()	// currentModel
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ShowAttributes() on ")+path+name+_T("\r\n"));
	ChangeAttrPrefFco();

	CGMEObjectInspector::theInstance->ShowPanel(0);
}

void CGMEView::ShowPreferences(CGuiFco *guiFco)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ShowPreferences(")+guiFco->GetName()+_T(" ")+guiFco->GetID()+_T(")\r\n"));
	ChangeAttrPrefFco(guiFco);

	CGMEObjectInspector::theInstance->ShowPanel(1);
}


// TODO
void CGMEView::ShowPreferences()	// currentModel
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ShowPreferences()\r\n"));
	ChangeAttrPrefFco();

	CGMEObjectInspector::theInstance->ShowPanel(1);
}

void CGMEView::RetrievePath()
{
	if ( currentModel ) {
		BeginTransaction(TRANSACTION_READ_ONLY);
		path = _T("");
		CComPtr<IMgaObject> spObject = currentModel.p;
		while ( true ) {
			CComPtr<IMgaObject> spParent;
			if ( SUCCEEDED( spObject->GetParent( &spParent, NULL ) ) && spParent ) {;
				CComBSTR bstrName;
				COMTHROW( spParent->get_Name( &bstrName ) );
				path = CString( bstrName ) + _T("/") + path;
				spObject = spParent;
			}
			else
				break;
		}
		path = _T("/") + path;
		CommitTransaction();
	}
}

void CGMEView::SetName()
{
	if(currentModel != 0) {
		try {
			BeginTransaction(TRANSACTION_READ_ONLY);
			COMTHROW(currentModel->get_Name(PutOut(name)));
			CommitTransaction();

			RetrievePath();

			SetTitles();

			SetNameProperty();
		}
		catch(hresult_exception &e) {
			AbortTransaction(e.hr);
		}
	}
}

void CGMEView::SetBgColor()
{
	if(currentModel != 0) {
		try {
			BeginTransaction(TRANSACTION_READ_ONLY);
			CComBSTR bstr;
			CComBSTR path(MODEL_BACKGROUND_COLOR_PREF);
			COMTHROW(currentModel->get_RegistryValue(path, &bstr));
			CString strVal(bstr);
			unsigned int hexval;
			if (_stscanf(strVal,_T("%x"),&hexval) == 1) {
				unsigned int r = (hexval & 0xff0000) >> 16;
				unsigned int g = (hexval & 0xff00) >> 8;
				unsigned int b = hexval & 0xff;
				bgColor = RGB(r,g,b);
			}
			else {
				bgColor = ::GetSysColor(COLOR_WINDOW);
			}
			CMainFrame::theInstance->SetPartBrowserBg(bgColor);
			CMainFrame::theInstance->RePaintPartBrowser();
			CommitTransaction();
		}
		catch(hresult_exception &e) {
			AbortTransaction(e.hr);
		}
	}
}

// prevous zoom value : curzoom
// new zoom value stored in m_zoomVal
// point : win client coordinates - this image point has to be centered
void CGMEView::SetZoomPoint(int curzoom, CPoint point)
{
	CRect clientW(0,0,0,0);
	GetClientRect(&clientW);
	int w = clientW.Width(); 
	int h = clientW.Height(); 
	CPoint offset = CPoint(w,h);
	{
		CWindowDC dc(NULL);
		dc.SetMapMode(MM_ISOTROPIC);
		dc.SetWindowExt(100,100);
		dc.SetViewportExt(curzoom, curzoom);
		dc.DPtoLP((LPPOINT)&point);
	}
	{
		CWindowDC dc(NULL);
		dc.SetMapMode(MM_ISOTROPIC);
		dc.SetWindowExt(100,100);
		dc.SetViewportExt(m_zoomVal, m_zoomVal);
		dc.DPtoLP((LPPOINT)&offset);
	}
	offset.x /= 2;
	offset.y /= 2;

	CPoint scp = GetScrollPosition();       // upper corner of scrolling
	m_zoomP = scp + point - offset;

	m_zoomScroll = true;
}

void CGMEView::ZoomIn(CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ZoomIn() in ")+path+name+_T("\r\n"));
//	zoomIdx = min(GME_ZOOM_LEVEL_NUM-1, zoomIdx+1);
	int curzoom = m_zoomVal;
	int newScale;
	frame->propBar.NextZoomVal(newScale);
	m_zoomP.x = m_zoomP.y = 0;
	ZoomToPoint(curzoom, newScale, point);
}

void CGMEView::ZoomOut(CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ZoomOut() in ")+path+name+_T("\r\n"));
	int curzoom = m_zoomVal;
	int newScale;
	frame->propBar.PrevZoomVal(newScale);
	m_zoomP.x = m_zoomP.y = 0;
	ZoomToPoint(curzoom, newScale, point);
}

void CGMEView::ShowHelp(CComPtr<IMgaFCO> fco)
{
	try {
		BeginTransaction(TRANSACTION_READ_ONLY);

		CGMEEventLogger::GMEEventPrintf(_T("CGMEView::ShowHelp(%Z) in ")+path+name+_T("\r\n"),fco,NULL);
		CComObjPtr<IMgaLauncher> launcher;
		COMTHROW( launcher.CoCreateInstance(L"Mga.MgaLauncher") );
		COMTHROW( launcher->ShowHelp(fco) );
		CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to access context-specific help information!"),MB_OK | MB_ICONSTOP);
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::ShowHelp - Unable to access context-specific help information.\r\n"));
	}
}

// ??
void CGMEView::ShowModel(CComPtr<IMgaModel> model, const CString& aspect)
{
	// FIX for JIRA bug: GME-135
	if (!model) return;
#if !defined (ACTIVEXGMEVIEW)
	// endFIX
	CString newAspect = aspect != _T("") ? aspect : currentAspect->name;
	CGMEDoc *doc = GetDocument();
	CGMEView *view = doc->FindView(model);
	CComPtr<IMgaFCO> fakeObj;
	if(!view)
		doc->SetNextToView(model, newAspect, fakeObj);
	else
		view->ChangeAspect(newAspect);
	CMainFrame::theInstance->CreateNewView(view, model);
	if( theApp.isHistoryEnabled())
	{
		doc->tellHistorian(model, newAspect);
		doc->clearForwHistory();
	}
#endif
}

void CGMEView::GetModelInContext(CComPtr<IMgaModel> &model)
{
	if (contextSelection)
		VERIFY(SUCCEEDED(contextSelection->mgaFco.QueryInterface(&model)));
	else
		model = currentModel;
	VERIFY(model != 0);
}

void CGMEView::FindDerivedFrom(CComPtr<IMgaModel> model,CComPtr<IMgaModel> &type)
{
	CComPtr<IMgaFCO> der;
	try {
		BeginTransaction(TRANSACTION_READ_ONLY);
		CGMEEventLogger::GMEEventPrintf(_T("CGMEView::FindDerivedFrom(model=%Z,type=%z)\r\n"),model,type);
		COMTHROW(model->get_DerivedFrom(&der));
		if(der != 0)
			COMTHROW(der.QueryInterface(&type));
		CommitTransaction();
	}
	catch(hresult_exception e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to find type model"),MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to find type model\r\n"));
	}
}

void CGMEView::DrawConnections(HDC pDC, Gdiplus::Graphics* gdip)
{
	POSITION pos = connections.GetHeadPosition();
	while (pos) {
		CGuiConnection* conn = connections.GetNext(pos);
		if (conn->IsVisible()) {
			conn->Draw(pDC, gdip);
		}
	}
}

void CGMEView::DrawTracker(CDC* pDC, const CRect& trackerRect, CRectTracker::StyleFlags styleFlags)
{
	CRectTrackerDpi tracker;
	tracker.m_rect = trackerRect;
	tracker.m_nStyle = styleFlags;
	tracker.Draw(pDC);
}

void CGMEView::DrawConnectionCustomizationTracker(CDC* pDC, Gdiplus::Graphics* gdip)
{
	if (customizeConnectionType == SimpleEdgeDisplacement)
	{
		if (customizeConnectionPartMoveMethod == HorizontalEdgeMove ||
			customizeConnectionPartMoveMethod == AdjacentEdgeMove)
		{
			CPoint startPoint	= customizeConnectionEdgeStartPoint;
			CPoint endPoint		= customizeConnectionEdgeEndPoint;
			if (customizeConnectionPartMoveMethod == AdjacentEdgeMove && customizeHorizontalOrVerticalEdge) {
				startPoint	= customizeConnectionEdgeEndPoint;
				endPoint	= customizeConnectionEdgeThirdPoint;
			}
			CRect trackerRect;
			trackerRect.left	= min(startPoint.x, max(customizeConnectionCurrCursor.x, customizeConnectionEdgeXMinLimit));
			trackerRect.top		= min(startPoint.y, endPoint.y);
			trackerRect.right	= max(startPoint.x, min(customizeConnectionCurrCursor.x, customizeConnectionEdgeXMaxLimit));
			trackerRect.bottom	= max(startPoint.y, endPoint.y);
			//TRACE("Conn Customization Tracker: (%ld,%ld)-(%ld,%ld)\n", trackerRect.left, trackerRect.top,
			//														   trackerRect.right, trackerRect.bottom);
			DrawTracker(pDC, trackerRect, CRectTracker::dottedLine);
		}
		if (customizeConnectionPartMoveMethod == VerticalEdgeMove ||
			customizeConnectionPartMoveMethod == AdjacentEdgeMove)
		{
			CPoint startPoint	= customizeConnectionEdgeStartPoint;
			CPoint endPoint		= customizeConnectionEdgeEndPoint;
			if (customizeConnectionPartMoveMethod == AdjacentEdgeMove && !customizeHorizontalOrVerticalEdge) {
				startPoint	= customizeConnectionEdgeEndPoint;
				endPoint	= customizeConnectionEdgeThirdPoint;
			}
			CRect trackerRect;
			trackerRect.left	= min(startPoint.x, endPoint.x);
			trackerRect.top		= min(startPoint.y, max(customizeConnectionCurrCursor.y, customizeConnectionEdgeYMinLimit));
			trackerRect.right	= max(startPoint.x, endPoint.x);
			trackerRect.bottom	= max(startPoint.y, min(customizeConnectionCurrCursor.y, customizeConnectionEdgeYMaxLimit));
			//TRACE("Conn Customization Tracker: (%ld,%ld)-(%ld,%ld)\n", trackerRect.left, trackerRect.top,
			//														   trackerRect.right, trackerRect.bottom);
			DrawTracker(pDC, trackerRect, CRectTracker::dottedLine);
		}
	} else if (customizeConnectionType == CustomPointCustomization) {
		int yRes = ::GetDeviceCaps(*pDC, LOGPIXELSY);
		Gdiplus::Pen* dashPen = graphics.GetGdipPen2(gdip, GME_BLACK_COLOR, GME_LINE_DASH, MulDiv(1, yRes, 96));
		ASSERT(customizeConnectionEdgeStartPoint != emptyPoint);
		if (customizeConnectionEdgeStartPoint != emptyPoint)
			gdip->DrawLine(dashPen, customizeConnectionEdgeStartPoint.x, customizeConnectionEdgeStartPoint.y,
						   customizeConnectionCurrCursor.x, customizeConnectionCurrCursor.y);
		ASSERT(customizeConnectionEdgeEndPoint != emptyPoint);
		if (customizeConnectionEdgeEndPoint != emptyPoint)
			gdip->DrawLine(dashPen, customizeConnectionCurrCursor.x, customizeConnectionCurrCursor.y,
						   customizeConnectionEdgeEndPoint.x, customizeConnectionEdgeEndPoint.y);
	}
}

void CGMEView::InsertCustomEdge(CGuiConnection* selectedConn, PathCustomizationType custType,
								int newPosX, int newPosY, int edgeIndex, bool horizontalOrVerticalEdge)
{
	CustomPathData pathData;
	selectedConn->FillOutCustomPathData(pathData, custType, currentAspect->index, newPosX, newPosY, edgeIndex, -1, horizontalOrVerticalEdge);
	selectedConn->InsertCustomPathData(pathData);
}

void CGMEView::UpdateCustomEdges(CGuiConnection* selectedConn, PathCustomizationType custType,
								 int newPosX, int newPosY, int edgeIndex, bool horizontalOrVerticalEdge)
{
	CustomPathData pathData;
	selectedConn->FillOutCustomPathData(pathData, custType, currentAspect->index, newPosX, newPosY, edgeIndex, -1, horizontalOrVerticalEdge);
	selectedConn->UpdateCustomPathData(pathData);
}

void CGMEView::DeleteCustomEdges(CGuiConnection* selectedConn, PathCustomizationType custType,
								 int edgeIndex, bool horizontalOrVerticalEdge)
{
	CustomPathData pathData;
	selectedConn->FillOutCustomPathData(pathData, custType, currentAspect->index, 0, 0, edgeIndex, -1, horizontalOrVerticalEdge);
	selectedConn->DeletePathCustomization(pathData);
}

void CGMEView::ConvertPathToCustom(CComPtr<IUnknown>& pMgaObject)
{
	CComQIPtr<IMgaModel> pMgaModel(pMgaObject);
	if (pMgaModel) {
		VARIANT_BOOL isEq = VARIANT_FALSE;
		COMTHROW(currentModel->get_IsEqual(pMgaModel, &isEq));
		if (isEq == VARIANT_FALSE)
			return;
	}
	CComQIPtr<IMgaConnection> pMgaConn(pMgaObject);

	bool isThereAnyConversion = false;
	POSITION pos = connections.GetHeadPosition();
	while (pos) {
		CGuiConnection* conn = connections.GetNext(pos);
		bool isThisConnection = false;
		if (pMgaConn != NULL) {
			VARIANT_BOOL isEq = VARIANT_FALSE;
			COMTHROW(conn->mgaFco->get_IsEqual(pMgaConn, &isEq));
			if (isEq == VARIANT_TRUE)
				isThisConnection = true;
		} else {
			isThisConnection = true;
		}
		if (isThisConnection) {
			conn->ConvertAutoRoutedPathToCustom(currentAspect->index, false);
			if (isThisConnection && pMgaConn != NULL)
				break;
		}
	}
}

void CGMEView::AutoRoute()
{
	BeginWaitCursor();
	router.AutoRoute(children, currentAspect->index);  // Must reroute the whole thing, other code depends on it
	EndWaitCursor();
}

void CGMEView::IncrementalAutoRoute()
{
	BeginWaitCursor();
	router.AutoRoute(currentAspect->index);
	EndWaitCursor();
}

void CGMEView::ModeChange()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ModeChange in ")+path+name+_T("\r\n"));
	if (last_Connection)
	{
		last_Connection->ToggleHover();
		last_Connection = 0;
	}
	this->SendUnselEvent4List( &selected);
	selected.RemoveAll();
	ChangeAttrPrefObjs(selected);
	RemoveAllAnnotationFromSelection();
	ClearConnectionSelection();
	ClearConnSpecs();
	CGMEDoc *pDoc = GetDocument();
	CGuiAnnotator::GrayOutAnnotations(annotators, pDoc->GetEditMode() == GME_VISUAL_MODE);
	CGuiFco::GrayOutFcos(children,pDoc->GetEditMode() == GME_VISUAL_MODE);
	CGuiFco::GrayOutFcos(connections,pDoc->GetEditMode() == GME_VISUAL_MODE);
	tmpConnectMode = false;
	OnSetCursor(NULL, HTCLIENT, 0);
	Invalidate();
	DoPannWinRefresh();
}

INT_PTR CGMEView::OnToolHitTest( CPoint point, TOOLINFO* pTI ) const
{
	static CGuiObject *oldObject = 0;
	static CGuiPort *oldPort = 0;
	CGMEView *self = const_cast<CGMEView *>(this);
	CoordinateTransfer(point);

#ifdef _ARDEBUG
	SArEdge* edge = router.router->GetListEdgeAt(point, 2);
	if( edge )
	{
		pTI->hwnd = m_hWnd;
		pTI->rect = CRect(point.x - 3, point.y - 3, point.x + 3, point.y + 3);
		pTI->uId = 1;
		CString str;
		str.Format(_T("%p y=%f (%d,%d,%d,%d) prev=%p next=%p"),
			edge, edge->position_y,
			edge->startpoint->x, edge->startpoint->y,
			edge->endpoint->x, edge->endpoint->y,
			edge->block_prev, edge->block_next);
		pTI->lpszText = _strdup(str);
		return 1;
	}
	return -1;
#endif
	CGuiConnection *conn= router.FindConnection(point);
	if(conn)
	{
		pTI->hwnd = m_hWnd;
		pTI->rect = CRect(point.x - 8, point.y - 8, point.x + 8, point.y + 8);
		pTI->uId = 1;
		CString str = conn->GetInfoText();
		// FIXME: does this leak?
		pTI->lpszText = _tcsdup(str);
		return 1;
	}

	CGuiObject *object = self->FindObject(point);
	if(object) {
		CString portinfo;
		CRect rect = object->GetLocation();
		CGuiPort *port = object->FindPort(point);
		if(port && port->IsRealPort()) {
			portinfo = _T(" : ") + port->GetInfoText();
			rect = port->GetLocation() + rect.TopLeft();
		}
		if(object != oldObject || port != oldPort) {
			oldPort = port;
			oldObject = object;
			return -1;
		}

		CClientDC dc(self);
		self->OnPrepareDC(&dc);
		dc.LPtoDP(rect);
		pTI->hwnd = m_hWnd;
		pTI->rect = rect;
		pTI->uId = 1;
		pTI->lpszText = _tcsdup(object->GetInfoText() + portinfo);
		return 1;
	}
	oldObject = 0;
	return -1;
}

void CGMEView::CreateOffScreen(CDC *dc)
{
	if(offScreenCreated)
		return;
	offScreen = new CDC;
	BOOL success = offScreen->CreateCompatibleDC(dc);
	ASSERT(success);
	// In multi-monitor systems a window can bigger than just one screen, monitor resolutions can be different, etc.
	// TODO: Maybe we should calculate with SM_CXMAXTRACK,SM_CYMAXTRACK? A window can be larger than the displays!!!
	int offScreenWidth = GetSystemMetrics(SM_CXMAXTRACK);
	int offScreenHeight = GetSystemMetrics(SM_CYMAXTRACK);
//	int offScreenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
//	int offScreenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	ofsbmp = new CBitmap;
	success = ofsbmp->CreateCompatibleBitmap(dc,offScreenWidth,offScreenHeight);
	ASSERT(success);
	// HACK: what about palettes?
    offScreenCreated = SaveDC(*offScreen);
    ASSERT(offScreenCreated);
    offScreen->SelectObject(ofsbmp);
}

void CGMEView::DeleteOffScreen() {
	if (offScreenCreated) {
		::RestoreDC(*offScreen, offScreenCreated);
		delete offScreen;
		delete ofsbmp;
		offScreenCreated = 0;
	}
}

void CGMEView::SetScroll()
{
	CRect objext, annext, extent;
	CGuiObject::GetExtent(children, objext);
	CGuiAnnotator::GetExtent(annotators, annext);
	extent.UnionRect(&objext, &annext);
	extent.right = (int)(extent.right*EXTENT_ERROR_CORR); // ??
	extent.bottom = (int)(extent.bottom*EXTENT_ERROR_CORR); // ??
	CSize s(extent.right, extent.bottom);
//	s.cx = s.cx + END_SCROLL_OFFSET;
//	s.cy = s.cy + END_SCROLL_OFFSET;

//	if (setZoomPercents[zoomIdx] == 100) {
	if (false && m_zoomVal == ZOOM_NO) {
		SetScrollSizes(MM_TEXT,s, m_zoomVal); // setZoomPercents[zoomIdx]);
	}
	else {
		SetScrollSizes(MM_ISOTROPIC,s, m_zoomVal); // setZoomPercents[zoomIdx]);
	}
}

void CGMEView::SetCenterObject(CComPtr<IMgaFCO> centerObj)
{
	if(centerObj != NULL) {
		CGuiObject* guiObj = NULL;
		try {
			BeginTransaction(TRANSACTION_READ_ONLY);
			guiObj = CGuiFco::FindObject(centerObj, children);
			CommitTransaction();
		}
		catch(hresult_exception e) {
			AbortTransaction(e.hr);
			guiObj = NULL;
		}

		CGuiConnection* connection = NULL;
		if (guiObj == NULL) {
			try {
				BeginTransaction(TRANSACTION_READ_ONLY);
				connection = CGuiFco::FindConnection(centerObj, connections);
				CommitTransaction();
			}
			catch(hresult_exception e) {
				AbortTransaction(e.hr);
				connection = NULL;
			}
		}

		if (guiObj || connection) {
			if (guiObj)
				CGMEEventLogger::LogGMEEvent(_T("CGMEView::SetCenterFCO(")+guiObj->GetName()+_T(" ")+guiObj->GetID()+_T(") in ")+path+name+_T("\r\n"));
			else
				CGMEEventLogger::LogGMEEvent(_T("CGMEView::SetCenterFCO(")+connection->GetName()+_T(" ")+connection->GetID()+_T(") in ")+path+name+_T("\r\n"));
			if (guiObj && !guiObj->IsVisible() ||
				connection && !connection->IsVisible())
			{
				int aspNum = guiMeta->aspects.GetCount();
				for (int aindex = 0; aindex < aspNum; aindex++) {
					if (guiObj && guiObj->IsVisible(aindex) ||
						connection && connection->IsVisible(aindex))
					{
						ChangeAspect(aindex);
						CMainFrame::theInstance->ChangePartBrowserAspect(aindex);
						break;
					}
				}
			}
			if (guiObj && guiObj->IsVisible() ||
				connection && connection->IsVisible())
			{
				CDC* pDC = GetDC();
				OnPrepareDC(pDC);
				CPoint centerPt;
				if (guiObj)
					centerPt = guiObj->GetCenter();
				else
					centerPt = connection->GetCenter();
				CRect wndRect;
				GetClientRect(&wndRect);
				pDC->DPtoLP(&wndRect);
				CSize totalSize = GetTotalSize();
				CPoint spos;
				spos.x = centerPt.x - (wndRect.Width()/2);
				spos.y = centerPt.y - (wndRect.Height()/2);

				spos.x = max(spos.x, 0L);
				spos.x = min(spos.x, totalSize.cx);
				spos.y = max(spos.y, 0L);
				spos.y = min(spos.y, totalSize.cy);

				this->SendUnselEvent4List( &selected);
				selected.RemoveAll();
				RemoveAllAnnotationFromSelection();
				ClearConnectionSelection();
				if (guiObj) {
					this->SendSelecEvent4Object( guiObj);
					selected.AddTail(guiObj);
				}
				ScrollToPosition(spos);
				if (guiObj)
					ChangeAttrPrefFco(guiObj);
				else
					ChangeAttrPrefFco(connection);	// sets selectedConnection
				Invalidate();
				this->SendNow();
				ReleaseDC(pDC);
			}
		}
	}
}

void CGMEView::Invalidate(bool thorough)
{
	if(thorough) {
		modelGrid.Clear();
		FillModelGrid();
	}
	m_overlay = nullptr;
	SetScroll();
	CScrollZoomView::Invalidate();
}

int CGMEView::GetAspectProperty()
{
	CComboBox *box;
	box = (CComboBox *)(frame->propBar.GetDlgItem(IDC_ASPECT));
	ASSERT(box);
	return box->GetCurSel();
}

void CGMEView::SetAspectProperty(int ind)
{
	CComboBox *box;
	box = (CComboBox *)(frame->propBar.GetDlgItem(IDC_ASPECT));
	ASSERT(box);
	box->SetCurSel(ind);
}

void CGMEView::SetNameProperty()
{
	CWnd *ctrl;
	ctrl = frame->propBar.GetDlgItem(IDC_NAME);
	ASSERT(ctrl);
	ctrl->SetWindowText( name );
	SetTitles();
}

void CGMEView::SetTitles(void)
{
	frame->SetTitle(name);
	frame->SetAppTitle(path);
	frame->OnUpdateFrameTitle(true);
}

void CGMEView::GetNameProperty(CString &txt)
{
	CWnd *ctrl;
	ctrl = frame->propBar.GetDlgItem(IDC_NAME);
	ASSERT(ctrl);
	ctrl->GetWindowText(txt);
}

void CGMEView::SetKindNameProperty()
{
	CWnd *ctrl;
	ctrl = frame->propBar.GetDlgItem(IDC_KINDNAME);
	ASSERT(ctrl);
	ctrl->SetWindowText(kindDisplayedName);
}

void CGMEView::SetTypeProperty(bool type)
{
	if(type)
		frame->propBar.ShowType();
	else
		frame->propBar.ShowInstance();
}

void CGMEView::SetTypeNameProperty()
{
	CWnd *ctrl;
	ctrl = frame->propBar.GetDlgItem(IDC_TYPENAME);
	ASSERT(ctrl);
	CComPtr<IMgaFCO> fco;
	CString txt = _T("N/A");
	if(baseType != 0) {
		COMTHROW(baseType->get_Name(PutOut(txt)));
	}
	RetrievePath();
	ctrl->SetWindowText(txt);
	SetTitles();
}

void CGMEView::SetProperties()
{
	SetTypeProperty(isType);
	SetTypeNameProperty();
	SetNameProperty();
	SetKindNameProperty();

	CComboBox *box = (CComboBox *)(frame->propBar.GetDlgItem(IDC_ASPECT));
	ASSERT(box);
	if(box->GetCount() <= 0) {
		guiMeta->InitAspectBox(box);
		box->SetCurSel(currentAspect->index);
	}
}

void CGMEView::CoordinateTransfer(CPoint &point) const
{
	CClientDC dc(const_cast<CGMEView *>(this));
	(const_cast<CGMEView *>(this))->OnPrepareDC(&dc);
	dc.DPtoLP(&point);
}

CGuiObject *CGMEView::FindFirstObject()
{
	m_findNextAlreadyAchieved = 0; // reset the currently achieved minimum level to its default

	return HelpMeFindNextObject( false);
}

CGuiObject *CGMEView::FindNextObject()
{
	return HelpMeFindNextObject( true);
}

CGuiObject *CGMEView::HelpMeFindNextObject( bool p_secondFind)
{
	CGuiObject*      first     = 0;
	unsigned long    first_abs = 0; // distance square from (0,0)
	CPoint           first_pos( 0, 0);

	POSITION pos = children.GetHeadPosition();
	while(pos) {
		CGuiFco* fco = children.GetNext(pos);
		ASSERT(fco != NULL);
		CGuiObject* cur = fco->dynamic_cast_CGuiObject();
		if( cur && cur->IsVisible())
		{
			CPoint        cur_pos = cur->GetCenter();
			unsigned long cur_abs = cur_pos.x * cur_pos.x + cur_pos.y * cur_pos.y;

			// m_findNextAlreadyAchieved is the previously achieved minimum distance
			// and we need to get further now if not invoked from FindFirst
			// we disregard objects closer than m_curAbs
			if( p_secondFind && cur_abs <= m_findNextAlreadyAchieved)
				continue; // skip if its closer than the allowed/required level

			if( !first) 
			{
				first      = cur;
				first_pos  = cur_pos;
				first_abs  = cur_abs;
			}
			else
			{
				if( first_abs > cur_abs) // 'cur' closer to (0, 0) than 'first'?
				{
					first     = cur;
					first_pos = cur_pos;
					first_abs = cur_abs;
				}
			}
		}
	}

	if( first) // something found
		m_findNextAlreadyAchieved = first_abs;
	return first;
}

CGuiObject* CGMEView::FindObject(CPoint &pt, bool lookNearToo, bool lookForLabel)
{
	POSITION pos = children.GetHeadPosition();
	while(pos) {
		CGuiFco* fco = children.GetNext(pos);
		ASSERT(fco != NULL);
		CGuiObject* obj = fco->dynamic_cast_CGuiObject();
		if(obj && obj->IsVisible()) {
			if (!lookForLabel) {
				if (obj->IsInside(pt, lookNearToo))
					return obj;
			} else {
				if (obj->IsLabelInside(pt, lookNearToo))
					return obj;
			}
		}
	}
	return NULL;
}

CGuiAnnotator *CGMEView::FindAnnotation(CPoint &pt)
{
	POSITION pos = annotators.GetHeadPosition();
	while(pos) {
		CGuiAnnotator *ann = annotators.GetNext(pos);
		if (ann->IsVisible() && ann->GetLocation().PtInRect(pt)) {
			return ann;
		}
	}
	return 0;
}

bool CGMEView::FindObjects(CRect &rect,CGuiObjectList &objectList)
{
	bool ret = false;
	CGuiObject* obj;
	CRect r, dummy;
	POSITION pos = children.GetHeadPosition();
	while(pos) {
		CGuiFco* fco = children.GetNext(pos);
		ASSERT(fco != NULL);
		obj = fco->dynamic_cast_CGuiObject();
		if(obj) {
			if(!obj->IsVisible())
				continue;
			r = obj->GetLocation();
			if(dummy.IntersectRect(&r,&rect)) {
				objectList.AddTail(obj);
				ret = true;
			}
		}
	}
	return ret;
}

bool CGMEView::FindAnnotations(CRect &rect,CGuiAnnotatorList &annotatorList)
{
	bool ret = false;
	CRect r,dummy;
	POSITION pos = annotators.GetHeadPosition();
	while(pos) {
		CGuiAnnotator *ann = annotators.GetNext(pos);
		if(!ann->IsVisible())
			continue;
		r = ann->GetLocation();
		if(dummy.IntersectRect(&r,&rect)) {
			annotatorList.AddTail(ann);
			ret = true;
		}
	}
	return ret;
}

bool CGMEView::DisconnectAll(CGuiObject *end,CGuiPort *endPort,bool onlyVisible)
{
	if(endPort)
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::DisconnectAll(end=")+end->GetName()+_T(" ")+end->GetID()+_T(" endPort=")+endPort->GetName()+_T(" ")+endPort->GetID()+_T(")\r\n"));
	else
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::DisconnectAll(end=")+end->GetName()+_T(" ")+end->GetID()+_T(")\r\n"));
	CGuiConnectionList conns;
	FindConnections(end,endPort,conns);
	POSITION pos = conns.GetHeadPosition();
	while(pos) {
		CGuiConnection *guiConn = conns.GetNext(pos);
		if(!onlyVisible || guiConn->IsVisible()) {
			try {
				BeginTransaction();
				long status;
				COMTHROW(guiConn->mgaFco->get_Status(&status));
				if(status == OBJECT_EXISTS)
				{
					bool ok = DeleteConnection(guiConn,false);
					if (!ok)
					{
						try {
						AbortTransaction(E_MGA_MUST_ABORT);
						} catch(hresult_exception e) { }
						return false;
					}	
				}
				__CommitTransaction();
			}
			catch(hresult_exception e) {
				AbortTransaction(e.hr);
				AfxMessageBox(_T("Unable to delete connection"), MB_ICONSTOP | MB_OK);
				CGMEEventLogger::LogGMEEvent(_T("    Unable to delete connection.\r\n"));
			}
			catch(_com_error& e) {
				AbortTransaction(e.Error());
				CString error = _T("Unable to delete connection");
				if (e.Description().length() != 0)
				{
					error += _T(": ");
					error += static_cast<const TCHAR*>(e.Description());
				}
				CGMEEventLogger::LogGMEEvent(error + _T("\r\n"));
				AfxMessageBox(error,MB_ICONSTOP | MB_OK);
			}
		}
	}
	return true;
}

void CGMEView::FindConnections(CGuiObject *end,CGuiPort *endPort,CGuiConnectionList &res)
{
	POSITION pos = connections.GetHeadPosition();
	while(pos) {
		CGuiConnection *conn = connections.GetNext(pos);
		if(!conn->IsVisible())
			continue;
		if((conn->src == end && conn->srcPort == endPort) ||
				(conn->dst == end && conn->dstPort == endPort))
			res.AddTail(conn);
	}
}

void CGMEView::FindConnections(CGuiObject *end1,CGuiPort *end1Port,CGuiObject *end2,CGuiPort *end2Port,CGuiConnectionList &res)
{
	POSITION pos = connections.GetHeadPosition();
	while(pos) {
		CGuiConnection *conn = connections.GetNext(pos);
		if(!conn->IsVisible())
			continue;
		if((conn->src == end1 && end1Port == conn->srcPort && conn->dst == end2 && end2Port == conn->dstPort) ||
				(conn->src == end2 && end2Port == conn->srcPort &&	conn->dst == end1 && end1Port == conn->dstPort))
			res.AddTail(conn);
	}
}

// returns: false if user answers no
static bool AskDeleteArcheType(IMgaFCOPtr obj)
{
	IMgaFCOPtr archetype = obj;
	do
	{
		archetype = archetype->ArcheType;
	} while (archetype->ArcheType);
	if (archetype->Status == OBJECT_EXISTS)
	{
		int nDerived = CountDeriveds(archetype);
		_bstr_t message = ObjTypeName[obj->Meta->ObjType];
		message += L" '" + (obj->Name.length() ? obj->Name : obj->Meta->Name);
		message += L"' is derived. Children of derived objects cannot be deleted.\n\nDo you want to delete its archetype?";
		if (nDerived > 1)
		{
			wchar_t wDerived[20];
			_itow(nDerived - 1, wDerived, 10);
			message += L" Deleting the archetype will also delete " + _bstr_t(wDerived) + L" other instances/subtypes.";
		}
		if (AfxMessageBox(message, MB_YESNO) == IDYES)
		{
			archetype->__DestroyObject();
			return true;
		}
		else
			return false;
	}
	return true;
}

bool CGMEView::DeleteConnection(CGuiConnection *guiConn,bool checkAspect)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::DeleteConnection(")+guiConn->GetName()+_T(" ")+guiConn->GetID()+_T(") in ")+path+name+_T("\r\n"));
	bool ok = false;
	BeginWaitCursor();
	try {
		BeginTransaction();
		if (guiConn->mgaFco->ArcheType && guiConn->mgaFco->IsPrimaryDerived == false)
		{
			ok = AskDeleteArcheType(guiConn->mgaFco.p);
		} else if(!checkAspect || guiConn->IsPrimary(guiMeta,currentAspect)) {
			guiConn->mgaFco->__DestroyObject();
			ok = true;
		}
		CommitTransaction();
	}
	catch(_com_error &e) {
		CString errorstring = L"Unable to delete connection";
		if (e.Description().length() != 0)
		{
			errorstring += _T(": ");
			errorstring += static_cast<const TCHAR*>(e.Description());
		}
		AfxMessageBox(errorstring, MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(CString(L"    ") + errorstring + L"\r\n");
		AbortTransaction(e.Error());
	}
	catch(hresult_exception &e) {
		CComBSTR error;
		CString errorstring = L"Unable to delete connection";
		if (GetErrorInfo(&error))
		{
			errorstring += L": ";
			errorstring += error;
		}
		AfxMessageBox(errorstring, MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(CString(L"    ") + errorstring + L"\r\n");
		AbortTransaction(e.hr);
	}
	this->SetFocus();
	return ok;
}

bool CGMEView::CheckBeforeDeleteObjects(CGuiObjectList &objectList,CString &txt)
{
	bool ok = true;
	try {
		BeginTransaction(TRANSACTION_READ_ONLY);
		POSITION pos = objectList.GetHeadPosition();
		while(pos) {
			CGuiObject *guiObj = objectList.GetNext(pos);
			if(!guiObj->IsPrimary(guiMeta,currentAspect)) {
				ok = false;
				txt += _T("\n   ") + guiObj->name;
			}
		}
		CommitTransaction();
	}
	catch(hresult_exception e) {
		AbortTransaction(e.hr);
	}
	return ok;
}

bool CGMEView::DeleteObjects(CGuiObjectList &objectList)
{
	bool brw_refresh_needed = false;
	try {
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::DeleteObjects in ")+path+name+_T("\r\n"));
		CString msg;
		if(!CheckBeforeDeleteObjects(objectList,msg)) {
			AfxMessageBox(_T("The following object(s) cannot be deleted: ") + msg);
			CGMEEventLogger::LogGMEEvent(_T("    The following object(s) cannot be deleted: ")+msg+_T("\r\n"));
			return true;
		}
		BeginWaitCursor();
		BeginTransaction();
		GMEEVENTLOG_GUIOBJS(objectList);
		POSITION pos = objectList.GetHeadPosition();
		while(pos) {
			CGuiObject *obj = objectList.GetNext(pos);

			POSITION pos2 = pendingRequests.GetHeadPosition();
			while (pos2) {
				POSITION tmp = pos2;
				CPendingObjectPosRequest *req = dynamic_cast<CPendingObjectPosRequest *> (pendingRequests.GetNext(pos2));
				if (req) {
					if ( req->object->mgaFco == obj->mgaFco ) {
						pendingRequests.RemoveAt(tmp);
						delete req;
					}
				}
			}

			if (obj->IsVisible()) {
				POSITION pos3 = obj->GetPorts().GetHeadPosition();
				while(pos3) {
						bool ok = DisconnectAll(obj,obj->GetPorts().GetNext(pos3),false);
						if (!ok)
							throw hresult_exception(E_MGA_MUST_ABORT);
				}
			}
		}
		pos = objectList.GetHeadPosition();
		while(pos) {
			CGuiObject *obj = objectList.GetNext(pos);
			long oStatus;
			COMTHROW(obj->mgaFco->get_Status(&oStatus));
			// making sure that the fco was not deleted previously in the loop due to a dependency 
			if(oStatus == OBJECT_EXISTS) {
				// throws E_MGA_MUST_ABORT if user selects CANCEL
				brw_refresh_needed = AskUserAndDetachIfNeeded(obj->mgaFco); // detach the dependents if needed
				if (obj->mgaFco->ArcheType && obj->mgaFco->IsPrimaryDerived == false)
				{
					if (AskDeleteArcheType(obj->mgaFco.p) == false)
					{
						throw hresult_exception(E_MGA_MUST_ABORT);
					}
				}
				else
				{
					obj->mgaFco->__DestroyObject();
				}
				COMTHROW(obj->mgaFco->Close());
			}
		}
		CommitTransaction();
	}
	catch(_com_error &e) {
		AbortTransaction(e.Error());
		CString error = _T("Unable to delete models");
		if (e.Description().length() != 0)
		{
			error += _T(": ");
			error += static_cast<const TCHAR*>(e.Description());
		}
		AfxMessageBox(error,MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(error);
		EndWaitCursor();
		return false;
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		if( e.hr == E_MGA_MUST_ABORT)
			CGMEEventLogger::LogGMEEvent(_T("    Archetype delete cancelled by user.\r\n"));
		else
		{
			AfxMessageBox(_T("Unable to delete models"),MB_ICONSTOP | MB_OK);
			CGMEEventLogger::LogGMEEvent(_T("    Unable to delete models.\r\n"));
		}
		EndWaitCursor();
		return false;
	}
	EndWaitCursor();
	ResetParent();
	if( brw_refresh_needed) CGMEBrowser::theInstance->RefreshAll();
	this->SetFocus();
	return true;
}

void CGMEView::DeleteAnnotations(CGuiAnnotatorList &annotatorList)
{
	try {
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::DeleteAnnotations() in ")+path+name+_T("\r\n"));
		GMEEVENTLOG_GUIANNOTATORS(annotatorList);
		BeginWaitCursor();
		BeginTransaction();
		POSITION pos = annotatorList.GetHeadPosition();
		while(pos) {
			CGuiAnnotator *ann = annotatorList.GetNext(pos);
			if( ann->IsSpecial())
				COMTHROW( CGuiAnnotator::Hide( ann->rootNode));
			else
				COMTHROW(ann->rootNode->RemoveTree());
		}
		CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to delete annotations"),MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to delete annotations.\r\n"));
		EndWaitCursor();
		return;
	}
	EndWaitCursor();
	ResetParent();
}


bool CGMEView::DoPasteItem(COleDataObject* pDataObject,bool drag,bool move,bool reference,bool derive,bool instance,bool closure,bool merge, CGuiObject *ref,CPoint pt)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::DoPasteItem"));
	if(drag)
		CGMEEventLogger::LogGMEEvent(_T(" DRAG"));
	if(move)
		CGMEEventLogger::LogGMEEvent(_T(" MOVE"));
	if(reference)
		CGMEEventLogger::LogGMEEvent(_T(" REFERENCE"));
	if(derive)
		CGMEEventLogger::LogGMEEvent(_T(" DERIVE"));
	if(instance)
		CGMEEventLogger::LogGMEEvent(_T(" INSTANCE"));
	if(closure)
		CGMEEventLogger::LogGMEEvent(_T(" CLOSURE"));
	if(merge)
		CGMEEventLogger::LogGMEEvent(_T(" SMART"));
	CGMEEventLogger::LogGMEEvent(_T(" in ")+path+name+_T("\r\n"));
	if(ref)
		VERIFY(reference);
	ASSERT(pDataObject != NULL);
	ASSERT_VALID(this);
	bool ok = false;

	try
	{
		CWaitCursor wait;

		if (CGMEDataSource::IsGmeNativeDataAvailable(pDataObject,theApp.mgaProject) && !closure)
		{
			ok = DoPasteNative(pDataObject,drag,move,reference,derive,instance,ref,pt);
		}
		else if( CGMEDataSource::IsXMLDataAvailable(pDataObject) )
		{
			if( closure && theApp.mgaConstMgr) theApp.mgaConstMgr->Enable(VARIANT_FALSE); // if closure is inserted disable the constraint manager ...
			ok = CGMEDataSource::ParseXMLData(pDataObject, currentModel, merge);
			if( closure && theApp.mgaConstMgr) theApp.mgaConstMgr->Enable(VARIANT_TRUE); // ... and enable it after done
		}
		else
			AfxThrowNotSupportedException();
	}
	catch(hresult_exception &)
	{
		// general cleanup
		TRACE(_T("failed to embed/link an OLE object\n"));
		return false;
	}
	this->SetFocus();
	return ok;
}

void CGMEView::MakeSureGUIDIsUniqueForSmartCopy( CComPtr<IMgaFCO>& fco)
{
	// this method prevents cloned objects having the same guid
	// as their original ones
	CComBSTR bstr;
	COMTHROW( fco->get_RegistryValue( CComBSTR( L"guid"), &bstr));
	if( bstr == 0 || bstr == L"") return; // no guid present, no need to replace it

	GUID t_guid = GUID_NULL;
	::CoCreateGuid(&t_guid);

	if (t_guid != GUID_NULL)
	{
		CString str_guid;
		str_guid.Format(_T("{%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
			t_guid.Data1, t_guid.Data2, t_guid.Data3,
			t_guid.Data4[0], t_guid.Data4[1], t_guid.Data4[2], t_guid.Data4[3],
			t_guid.Data4[4], t_guid.Data4[5], t_guid.Data4[6], t_guid.Data4[7]);
		
		// thus replace the old guid with a new one
		COMTHROW( fco->put_RegistryValue( CComBSTR( L"guid"), CComBSTR(str_guid)));
	}

	// store the previous guid in prev subnode
	COMTHROW( fco->put_RegistryValue( CComBSTR( L"guid/prev"), bstr));
}

bool CGMEView::DoPasteNative(COleDataObject *pDataObject,bool drag,bool move,bool reference,bool derive,bool instance,CGuiObject *ref,CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::DoPasteNative"));
	if(drag)
		CGMEEventLogger::LogGMEEvent(_T(" DRAG"));
	if(move)
		CGMEEventLogger::LogGMEEvent(_T(" MOVE"));
	if(reference)
		CGMEEventLogger::LogGMEEvent(_T(" REFERENCE"));
	if(derive)
		CGMEEventLogger::LogGMEEvent(_T(" DERIVE"));
	if(instance)
		CGMEEventLogger::LogGMEEvent(_T(" INSTANCE"));
	CGMEEventLogger::LogGMEEvent(_T(" in ")+path+name+_T("\r\n"));
	CComPtr<IDataObject> p = pDataObject->GetIDataObject(FALSE);
	CComPtr<IMgaDataSource> pt;
	COMTHROW(p.QueryInterface(&pt));

	CGMEDoc *doc = GetDocument();

	CComPtr<IUnknown> unk;

	CComPtr<IMgaRegNodes> regNodes;
	COMTHROW(pt->get_RegistryData(&unk));
	if (unk != NULL) {
		if (FAILED(unk.QueryInterface(&regNodes))) {
			regNodes = NULL;
		}
	}

	unk = NULL;
	COMTHROW(pt->get_Data(&unk));
	CComPtr<IMgaFCOs> fcos;
	CComPtr<IMgaMetaRole> metaRole;

	if(unk && SUCCEEDED(unk.QueryInterface(&fcos))) {			// not dragging from PartBrowser
		CComPtr<IMgaMetaAspect> aspect;
		currentAspect->GetMetaAspect(aspect);
		try {
			BeginTransaction();
//			if(currentAspect->CheckFcosBeforeInsertion(fcos) || reference) {
			{
				// Handle annotations
				CComPtr<IMgaRegNodes> newRegNodes;
				COMTHROW(newRegNodes.CoCreateInstance(OLESTR("Mga.MgaRegNodes")));
				PasteAnnotations(currentModel, regNodes, newRegNodes, (drag && move));

				long l;
				CComPtr<IMgaFCOs> interrfcos;
				COMTHROW(terry->OpenFCOs( fcos, &interrfcos));
				COMTHROW( interrfcos->get_Count( &l));

				CComPtr<IMgaFCOs> newFcos;
				if( l > 0)
				{
				if ((drag && move) || (!reference && !derive)) {
					CComPtr<IMgaMetaRoles> newRoles;
					COMTHROW(newRoles.CoCreateInstance(OLESTR("Mga.MgaMetaRoles")));
					MGACOLL_ITERATE(IMgaFCO, fcos) {
						CComPtr<IMgaFCO> fco;
						COMTHROW(terry->OpenFCO(MGACOLL_ITER, &fco));
						CComPtr<IMgaMetaRole> role;
						COMTHROW(fco->get_MetaRole(&role));
						CComPtr<IMgaMetaFCO> kind;
						COMTHROW(fco->get_Meta(&kind));
						CComPtr<IMgaMetaRole> newRole;

						COMTHROW(doc->resolver->get_RoleByMeta(currentModel,kind,OBJTYPE_NULL,role,aspect,&newRole));
						COMTHROW(newRoles->Append(newRole));
					}
					MGACOLL_ITERATE_END;

					if (drag && move) {
						COMTHROW(currentModel->MoveFCOs(fcos,newRoles,&newFcos));
					}
					else if(!reference && !derive) {
						COMTHROW(currentModel->CopyFCOs(fcos,newRoles,&newFcos));
					}

					if(!reference && !derive) // smart copy subtask:
					{
						MGACOLL_ITERATE(IMgaFCO, newFcos) {
							MakeSureGUIDIsUniqueForSmartCopy( CComPtr<IMgaFCO>(MGACOLL_ITER));
						}
						MGACOLL_ITERATE_END;
					}

					MGACOLL_ITERATE(IMgaFCO, newFcos) {
						CComPtr<IMgaFCO> newFco;
						newFco = MGACOLL_ITER;
						CString newID;
						COMTHROW(newFco->get_ID(PutOut(newID)));
						newObjectIDs.AddHead(newID);
					}
					MGACOLL_ITERATE_END;
				}
				else if(derive) {

					COMTHROW(newFcos.CoCreateInstance(L"Mga.MgaFCOs"));

					/*CComPtr<IMgaMetaFCO> metaFco;
					COMTHROW(currentModel->get_Meta(&metaFco));
					CComPtr<IMgaMetaModel> metaModel;
					COMTHROW(metaFco.QueryInterface(&metaModel));*/ // commented by zolmol, these vars are NOT used
					bool normalExit = true;
					HRESULT hr = S_OK;
					MGACOLL_ITERATE(IMgaFCO,fcos) {
						CComPtr<IMgaFCO> fco;
						COMTHROW(terry->OpenFCO(MGACOLL_ITER, &fco));
						CString fcoName;
						COMTHROW(fco->get_Name(PutOut(fcoName)));
						// Akos: do not want to derive a connection all by itself. Just skip it. 6/14/2013
						CComPtr<IMgaConnection> conn;
						HRESULT hr;
						if((hr = fco.QueryInterface(&conn)) == S_OK) {
							continue;
						}
						// Akos
/*
						CComPtr<IMgaModel> model;
						HRESULT hr;
						if((hr = fco.QueryInterface(&model)) != S_OK) {
							AfxMessageBox(fcoName + " is not a model. Only models can be derived!");
							AbortTransaction(hr);
							newObjectIDs.RemoveAll();
							return false;
						}
						{
							CComPtr<IMgaFCO> base;
							COMTHROW(fco->get_DerivedFrom(&base));
							if(base != 0) {
								AfxMessageBox(fcoName + " is not a root model type. Only root model types can be derived!");
								AbortTransaction(hr);
								newObjectIDs.RemoveAll();
								return false;
							}
						}
*/
						CComPtr<IMgaMetaFCO> kind;
						COMTHROW(fco->get_Meta(&kind));
						CComPtr<IMgaMetaRole> role;
						COMTHROW(fco->get_MetaRole(&role));
						CComPtr<IMgaMetaRole> newRole;
						COMTHROW(doc->resolver->get_RoleByMeta(currentModel,kind,OBJTYPE_NULL,role,aspect,&newRole));
						if(newRole == 0)
						{
							AfxMessageBox(_T("Cannot insert object derived from ") + fcoName);
							CGMEEventLogger::LogGMEEvent(_T("    Cannot insert object derived from ")+fcoName+_T("\r\n"));
						}
						else {
							CComPtr<IMgaFCO> obj;
							VARIANT_BOOL inst = instance ? VARIANT_TRUE : VARIANT_FALSE;
							if((hr = currentModel->DeriveChildObject(fco,newRole,inst,&obj)) != S_OK) {
								CString msg( (LPCTSTR) fcoName); msg += _T(" cannot be derived! Some of its ancestors or descendants may be already derived!");
								if( hr == E_MGA_NOT_DERIVABLE)
								{
									if( !CGMEConsole::theInstance) AfxMessageBox( msg + _T(" [Error code E_MGA_NOT_DERIVABLE]"));
									else CGMEConsole::theInstance->Message( msg + _T(" [Error code E_MGA_NOT_DERIVABLE]"), MSG_ERROR);
								}
								else
									AfxMessageBox( msg);
								CGMEEventLogger::LogGMEEvent(_T("    ") + msg + _T(" \r\n"));
								normalExit = false;
								break;
							}

							newFcos->Append(obj);

							CString newID;
							COMTHROW(obj->get_ID(PutOut(newID)));
							newObjectIDs.AddHead(newID);
						}
					}
					MGACOLL_ITERATE_END;
					if(!normalExit) {
						AbortTransaction(hr);
						newObjectIDs.RemoveAll();
						return false;
					}
				}
				else if(reference) {
					if(ref) {
						CGMEEventLogger::LogGMEEvent(_T("    ref=")+ref->GetName()+_T(" ")+ref->GetID()+_T("\r\n"));
						CComPtr<IMgaReference> mgaRef;
						COMTHROW(ref->mgaFco.QueryInterface(&mgaRef));
						long count;
						COMTHROW(fcos->get_Count(&count));
						/* if(count != 1) {
							AfxMessageBox(_T("Only a single object can be dropped on a reference for redirection!"));
							throw hresult_exception(E_FAIL);
						} */
						if(count < 1) {
							AfxMessageBox(_T("Cannot redirect reference to specified object!"));
							CGMEEventLogger::LogGMEEvent(_T("    Cannot redirect reference to specified object.\r\n"));
							throw hresult_exception(E_FAIL);
						}
						CComPtr<IMgaFCO> fco;
						COMTHROW(fcos->get_Item(1,&fco));
						if(!IsEqualObject(fco,mgaRef)) {
							try {
								if (mgaRef->UsedByConns->Count > 0)
									MoveReferenceWithRefportConnectionsAndWriteToConsole(fco, mgaRef); // only works if fco.ObjType == model
								else
									COMTHROW(mgaRef->put_Referred(fco));

								CComBSTR bstr;
								CString newID;
								COMTHROW(mgaRef->get_ID(PutOut(newID)));
								newObjectIDs.AddHead(newID);
							}
							catch(hresult_exception e) {
								AbortTransaction(e.hr);
								CGMEEventLogger::LogGMEEvent( _T("    Cannot redirect reference to specified object.\r\n"));
								const TCHAR* t1 = _T("Cannot redirect reference to specified object because of active connections!");
								const TCHAR* t2 = _T("Cannot redirect reference to specified object.");
								if( e.hr == E_MGA_REFPORTS_USED)
								{
									if( !CGMEConsole::theInstance) AfxMessageBox( t1);
									else CGMEConsole::theInstance->Message( t1, MSG_ERROR);
								}
								else
									if( CGMEConsole::theInstance) AfxMessageBox( t2);
									else CGMEConsole::theInstance->Message( t2, MSG_ERROR);

								return false;
							}
							catch(_com_error &e) {
								AbortTransaction(e.Error());
								CString error = _T("Cannot redirect reference to specified object");
								if (e.Description().length() != 0)
								{
									error += _T(": ");
									error += static_cast<const TCHAR*>(e.Description());
								}
								CGMEEventLogger::LogGMEEvent(error + "\r\n");
								if (CGMEConsole::theInstance)
									CGMEConsole::theInstance->Message(error, MSG_ERROR);

								return false;
							}
						}
					}
					else {
						COMTHROW(newFcos.CoCreateInstance(L"Mga.MgaFCOs"));
						/*CComPtr<IMgaMetaFCO> metaFco;
						COMTHROW(currentModel->get_Meta(&metaFco));
						CComPtr<IMgaMetaModel> metaModel;
						COMTHROW(metaFco.QueryInterface(&metaModel));*/ // commented by zolmol, these vars are NOT used
						MGACOLL_ITERATE(IMgaFCO,fcos) {
							CComPtr<IMgaFCO> fco;
							COMTHROW(terry->OpenFCO(MGACOLL_ITER, &fco));
							CComPtr<IMgaConnection> conn;
							if(fco.QueryInterface(&conn) != S_OK) { // skip connections, they cannot be referenced
								CComPtr<IMgaMetaRole> role;
								COMTHROW(doc->resolver->put_IsStickyEnabled(::GetKeyState(VK_SHIFT) < 0 ? VARIANT_FALSE :VARIANT_TRUE));
								HRESULT hr = doc->resolver->get_RefRoleByMeta(currentModel,aspect,fco,&role);
								if (hr == E_ABORT) {
									AbortTransaction(hr);
									return false;
								} else {
									COMTHROW(hr);
								}
								if(role == 0)
								{
									AfxMessageBox(_T("Cannot create reference"));
									CGMEEventLogger::LogGMEEvent(_T("    Cannot create reference.\r\n"));
								}
								else {
									CComPtr<IMgaFCO> ref;
									COMTHROW(currentModel->CreateReference(role,fco,&ref));
									newFcos->Append(ref);

									CString newID;
									COMTHROW(ref->get_ID(PutOut(newID)));
									newObjectIDs.AddHead(newID);

									CComBSTR nmb;
									COMTHROW(fco->get_Name(&nmb));
									// After Larry's wishes
									/* CString nm;
									CopyTo(nmb,nm);
									nm += _T("Ref");
									CopyTo(nm,nmb); */
									COMTHROW(ref->put_Name(nmb));
								}
							}
						}
						MGACOLL_ITERATE_END;
					}
				}
				else
					VERIFY(false);	// shouldn't be here!
				} // endif l > 0

				if(drag && !ref) {
					CGuiObjectList newObjs;
					CGuiFcoList newGuiFcos;
					CGuiConnectionList newConnections;
					CGuiAnnotatorList newAnns;
					if( newFcos) CreateGuiObjects(newFcos,newGuiFcos,newConnections);
					POSITION fpos = newGuiFcos.GetHeadPosition();
					while (fpos) {
						CGuiFco* fgfco = newGuiFcos.GetNext(fpos);
						ASSERT(fgfco != NULL);
						CGuiObject* fgobj = fgfco->dynamic_cast_CGuiObject();
						if (fgobj) {
							newObjs.AddTail(fgobj);
						}
					}
					CreateAnnotators(newRegNodes, newAnns);

					int left = 0, top = 0, leftA = 0, topA = 0;
					bool valid = false, validA = false;
					if (newObjs.GetCount() > 0) {
						CGuiObject::FindUpperLeft(newObjs, left, top);
						valid = true;
					}
					if (newAnns.GetCount() > 0) {
						CGuiAnnotator::FindUpperLeft(newAnns, leftA, topA);
						validA = true;
					}
					if (valid && validA) {
						left = min(left, leftA);
						top = min(top, topA);
					}
					else if (validA) {
						left = leftA;
						top = topA;
					}
					else if (!valid) {
						ASSERT(("There is no object to move", false));
					}
					CPoint diff = point - CPoint(left,top);
					CGuiObject::ShiftModels(newObjs, diff);
					CGuiAnnotator::ShiftAnnotations(newAnns,diff);


					// CGuiAnnotatorList newAnns;
					// CreateGuiAnnotations(newAnns, );
					CGuiObject::MoveObjects(newObjs,point);

					// We don't need this, since reset
					// children.AddTail(&newGuiFcos);
					// annotators.AddTail(&newAnns);
					// Instead:

					POSITION dpos = newGuiFcos.GetHeadPosition();
					while(dpos) {
						delete newGuiFcos.GetNext(dpos);
					}

					dpos = newAnns.GetHeadPosition();
					while(dpos) {
						delete newAnns.GetNext(dpos);
					}



				}
				Invalidate(true);
				AutoRoute();
				CommitTransaction();
				ResetParent();
				ChangeAttrPrefObjs(selected);
				return true;
			}
	/*
			else {
				CommitTransaction();
				AfxMessageBox("Objects cannot be inserted!");
			}
	*/
		}
		catch (hresult_exception& e) {
			AbortTransaction(e.hr);
			if (e.hr == E_MGA_CONSTRAINT_VIOLATION)
				return false;
			_bstr_t err;
			GetErrorInfo(e.hr, err.GetAddress());
			AfxMessageBox((std::wstring(L"Unable to insert objects: ") + static_cast<const wchar_t*>(err)).c_str(), MB_ICONSTOP | MB_OK); // in most cases there was an error msg already...
			newObjectIDs.RemoveAll();
		}
		return false;
	}
	else if(unk && SUCCEEDED(unk.QueryInterface(&metaRole))) {			// dragging from PartBrowser
		try {
			BeginTransaction();
			bool ok = false;
			if(!CGuiFco::IsPrimary(guiMeta,currentAspect,metaRole)) {
				if(currentAspect->IsPrimaryByRoleName(metaRole)) {
					CString roleName;
					COMTHROW(metaRole->get_Name(PutOut(roleName)));
					metaRole = 0;
					if(currentAspect->GetRoleByName(roleName,metaRole))
						ok = true;
				}
			}
			else
				ok = true;
			if(ok) {
				CComPtr<IMgaFCO> child;
				COMTHROW(currentModel->CreateChildObject(metaRole,&child));

				if (metaRole->Name == metaRole->Kind->Name)
				{
					child->Name = metaRole->Kind->DisplayedName;
				}
				else
				{
					child->Name = metaRole->DisplayedName;
				}
				CString newID;
				COMTHROW(child->get_ID(PutOut(newID)));
				newObjectIDs.AddHead(newID);

				SetObjectLocation(child,metaRole,point);

//				Invalidate(true);
//				AutoRoute();
				CommitTransaction();
				ChangeAttrPrefObjs(selected);
//				ResetParent();
			}
			else {
				CommitTransaction();
				AfxMessageBox(_T("Paradigm violation: cannot insert new part!"));
				CGMEEventLogger::LogGMEEvent(_T("    Paradigm violation: cannot insert new part.\r\n"));
				return false;
			}
		}
		catch(hresult_exception& e) {
			_bstr_t err;
			GetErrorInfo(e.hr, err.GetAddress());
			AbortTransaction(e.hr);
			if (e.hr == E_MGA_CONSTRAINT_VIOLATION)
				return false;
			AfxMessageBox((std::wstring(L"Unable to insert objects: ") + static_cast<const wchar_t*>(err)).c_str(), MB_ICONSTOP | MB_OK);
			CGMEEventLogger::LogGMEEvent(_T("    Unable to insert objects.\r\n"));
			newObjectIDs.RemoveAll();
            Reset(true); //BGY
			return false;
		}
	}
	return true;
}

void CGMEView::PasteAnnotations(CComPtr<IMgaModel> &targetModel, CComPtr<IMgaRegNodes> &regNodes, CComPtr<IMgaRegNodes> &newRegNodes, bool isMove)
{
	if (regNodes == NULL) {
		return;
	}
	MGACOLL_ITERATE(IMgaRegNode,regNodes) {
		CComPtr<IMgaRegNode> regNode;
		regNode = MGACOLL_ITER;

		CComBSTR nodePath;
		COMTHROW(regNode->get_Path(&nodePath));

		bool collision = true;
		while (collision) {
			CComPtr<IMgaRegNode> exRegNode;
			COMTHROW(targetModel->get_RegistryNode(nodePath, &exRegNode));
			long status;
			COMTHROW(exRegNode->get_Status(&status));
			if (status == ATTSTATUS_UNDEFINED) {
				collision = false;
			}
			else {
				nodePath.Append(_T("Copy"));
			}
		}

		CComPtr<IMgaRegNode> newNode;
		COMTHROW(targetModel->get_RegistryNode(nodePath, &newNode));
		COMTHROW(newRegNodes->Append(newNode));

		CopyRegTree(regNode, newNode);

		if (isMove) {
			COMTHROW(regNode->RemoveTree());
		}

		// Let's make it visible in all aspects
		CComPtr<IMgaRegNode> newAspRoot;
		CComBSTR aspRootName(AN_ASPECTS);
		COMTHROW(newNode->get_SubNodeByName(aspRootName, &newAspRoot));
		CComPtr<IMgaRegNodes> aspNodes;
		COMTHROW(newAspRoot->get_SubNodes(VARIANT_FALSE, &aspNodes));
		MGACOLL_ITERATE(IMgaRegNode,aspNodes) {
			CComPtr<IMgaRegNode> aspNode;
			aspNode = MGACOLL_ITER;
			COMTHROW(aspNode->RemoveTree());
		}
		MGACOLL_ITERATE_END;

		CComBSTR defAspName(AN_DEFASPECT);
		CComPtr<IMgaRegNode> defAspNode;
		COMTHROW(newAspRoot->get_SubNodeByName(defAspName, &defAspNode));
		CComBSTR defAspVal(AN_VISIBLE_DEFAULT);
		COMTHROW(defAspNode->put_Value(defAspVal));
	}
	MGACOLL_ITERATE_END;
}

void CGMEView::CopyRegTree(CComPtr<IMgaRegNode> &regNode, CComPtr<IMgaRegNode> &newNode)
{
	CComBSTR nodeValue;
	COMTHROW(regNode->get_Value(&nodeValue));
	COMTHROW(newNode->put_Value(nodeValue));

	CComPtr<IMgaRegNodes> subRegNodes;
	COMTHROW(regNode->get_SubNodes(VARIANT_FALSE, &subRegNodes));
	MGACOLL_ITERATE(IMgaRegNode,subRegNodes) {
		CComPtr<IMgaRegNode> subRegNode;
		subRegNode = MGACOLL_ITER;
		CComBSTR subName;
		COMTHROW(subRegNode->get_Name(&subName));
		CComPtr<IMgaRegNode> newSubNode;
		COMTHROW(newNode->get_SubNodeByName(subName, &newSubNode));
		CopyRegTree(subRegNode, newSubNode);
	}
	MGACOLL_ITERATE_END;
}

void CGMEView::FillModelGrid()
{
	BeginWaitCursor();
	modelGrid.SetSource(this);
	CGuiObject* obj;
	POSITION pos = children.GetHeadPosition();
	bool postPendingRequestEvent = false;
	while(pos) {
		CGuiFco* fco = children.GetNext(pos);
		ASSERT(fco != NULL);
		obj = fco->dynamic_cast_CGuiObject();
		if(!obj || !obj->IsVisible())
			continue;
		if(!modelGrid.IsAvailable(obj)) {
			CRect loc = obj->GetLocation();
			if (theApp.labelAvoidance) {
				CRect name = obj->GetNameLocation();
				loc.UnionRect(loc, name);
			}
			if(!modelGrid.GetClosestAvailable(obj, loc)) {
				//AfxMessageBox(_T("Too Many Models! Internal Program Error!"),MB_OK | MB_ICONSTOP);
				//EndWaitCursor();
				break;
			}
			// ASSERT(modelGrid.IsAvailable(loc));
			if (!executingPendingRequests && !IsInstance()) {
				CPendingObjectPosRequest *req = new CPendingObjectPosRequest(obj, loc, obj->GetParentAspect());
				pendingRequests.AddHead(req);
				postPendingRequestEvent = true;
			}

			obj->SetObjectLocation(loc, -1, false);
		}
		modelGrid.Set(obj);
	}
	if (postPendingRequestEvent)
		PostMessage(WM_USER_EXECUTEPENDINGREQUESTS);
	EndWaitCursor();
}

void CGMEView::SetObjectLocation(CComPtr<IMgaFCO> &child,CComPtr<IMgaMetaRole> &mmRole,CPoint pt)
{
	// We temporarily create a GuiObject in order to have a decorator and be able to access real size data
	CGuiFco* guiFco = CreateGuiObject(child, NULL, NULL);
	ASSERT(guiFco != NULL);
	CGuiObject* guiObject = guiFco->dynamic_cast_CGuiObject();
	ASSERT(guiObject != NULL);
	CRect loc = guiObject->GetLocation();
	::SetLocation(loc,pt);

	// CPoint npt = loc.CenterPoint();
	// if(!modelGrid.IsAvailable(loc)) {
	// 	if(!modelGrid.GetClosestAvailable(loc.Size(),npt)) {
	//		AfxMessageBox(_T("Too Many Models! Internal Program Error!"),MB_OK | MB_ICONSTOP);
	//		EndWaitCursor();
	//		return;
	//	}
	// }

	try {
		CComPtr<IMgaMetaParts> mmParts;
		CComPtr<IMgaMetaPart> mmPart;
		COMTHROW(mmRole->get_Parts(&mmParts));
		MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
			mmPart = MGACOLL_ITER;
			CComPtr<IMgaMetaAspect> mmAspect;
			COMTHROW(mmPart->get_ParentAspect(&mmAspect));
			CComPtr<IMgaPart> part;
			COMTHROW(child->get_Part(mmAspect,&part));
			COMTHROW(part->SetGmeAttrs(0,loc.TopLeft().x,loc.TopLeft().y));
			// COMTHROW(part->SetGmeAttrs(0,npt.x,npt.y));
		}
		MGACOLL_ITERATE_END;
	}
	catch(hresult_exception &e) {
		throw hresult_exception(e.hr);
	}

	delete guiFco;
}

void CGMEView::GetRefereeChain(IMgaFCOs* visitedRefs, IMgaFCO* fco)
{
	CComQIPtr<IMgaReference> spReference(fco);
	CComPtr<IMgaFCO> spReferenced;
	if (spReference) {
		while (spReference) {
			spReferenced = NULL;

			// Avoid circular references
			long res;
			if ((visitedRefs->Find(spReference, 1L, &res)) == E_NOTFOUND) {
				COMTHROW(visitedRefs->Append(spReference));
				COMTHROW(spReference->get_Referred(&spReferenced));
			}

			spReference = spReferenced;
		}
	}
}

bool CGMEView::Connect(CGuiObject *src,CGuiPort *srcPort, int srcHotSide, CGuiObject *dst,CGuiPort *dstPort, int dstHotSide, bool nosticky)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::Connect src=")+src->GetName()+_T(" ")+src->GetID()+_T(",dst=")+dst->GetName()+_T(" ")+dst->GetID()+_T(" in ")+path+name+_T("\r\n"));
	bool ret = false;
	CGMEDoc *doc = GetDocument();
	try {
		BeginTransaction();

		CComPtr<IMgaMetaRole> role;
		CComPtr<IMgaMetaAspect> aspect;
		currentAspect->GetMetaAspect(aspect);

		if (srcPort) {
			srcPort = srcPort->IsRealPort() ? srcPort : NULL;
			if (srcPort) CGMEEventLogger::LogGMEEvent(_T("    srcPort=")+srcPort->GetName()+_T(" ")+srcPort->GetID()+_T("\r\n"));
		}
		if (dstPort) {
			dstPort = dstPort->IsRealPort() ? dstPort : NULL;
			if (dstPort) CGMEEventLogger::LogGMEEvent(_T("    dstPort=")+dstPort->GetName()+_T(" ")+dstPort->GetID()+_T("\r\n"));
		}
		COMTHROW(doc->resolver->put_IsStickyEnabled(nosticky ? VARIANT_FALSE :VARIANT_TRUE));
		role = doc->resolver->ConnRoleByMeta[currentModel,
													aspect,
													src->mgaFco,
													srcPort ? srcPort->mgaFco : NULL,
													dst->mgaFco,
													dstPort ? dstPort->mgaFco : NULL];
		CComPtr<IMgaFCO> conn;
		if(role != 0) {
			CComObjPtr<IMgaFCOs> srcRefs, dstRefs;
			COMTHROW(srcRefs.CoCreateInstance(L"Mga.MgaFCOs"));
			COMTHROW(dstRefs.CoCreateInstance(L"Mga.MgaFCOs"));

// Bakay hack: avoid setting up refchains for atomrefs.
			if(srcPort) GetRefereeChain(srcRefs,src->mgaFco);
//			GetRefereeChain(srcRefs,src->mgaFco);
			if(dstPort) GetRefereeChain(dstRefs,dst->mgaFco);
//			GetRefereeChain(dstRefs,dst->mgaFco);

/*
// Bakay hack
// Atom references must pass the referenced atom as src and dst
			CComQIPtr<IMgaReference> refp;
			while((refp = src->mgaFco) != NULL) {
				src->mgaFco = NULL;
				COMTHROW(refp->get_Referred(&src->mgaFco));
			}
			while((refp = dst->mgaFco) != NULL) {
				dst->mgaFco = NULL;
				COMTHROW(refp->get_Referred(&dst->mgaFco));
			}
// Bakay hack ends
*/
			conn = currentModel->__CreateSimpleConn(
				role,
				srcPort ? srcPort->mgaFco : src->mgaFco,
				dstPort ? dstPort->mgaFco : dst->mgaFco,
				srcRefs,
				dstRefs);
			CComBSTR nm;
			COMTHROW(role->get_DisplayedName(&nm));
			COMTHROW(conn->put_Name(nm));

			// Setup autorouter prefs
			CString routerPrefStr;
			switch (srcHotSide) {
			case GME_NORTH:
				routerPrefStr = _T("N");
				break;
			case GME_SOUTH:
				routerPrefStr = _T("S");
				break;
			case GME_WEST:
				routerPrefStr = _T("W");
				break;
			case GME_EAST:
				routerPrefStr = _T("E");
				break;
			}
			switch (dstHotSide) {
			case GME_NORTH:
				routerPrefStr += _T("n");
				break;
			case GME_SOUTH:
				routerPrefStr += _T("s");
				break;
			case GME_WEST:
				routerPrefStr += _T("w");
				break;
			case GME_EAST:
				routerPrefStr += _T("e");
				break;
			}
			if (!routerPrefStr.IsEmpty()) {
				CComBSTR	bstrPath(AUTOROUTER_PREF);
				CComBSTR	bstrVal(routerPrefStr);
				COMTHROW(conn->put_RegistryValue(bstrPath,bstrVal));
			}

			ret = true;
			CommitTransaction();
		}
		else {
			CommitTransaction();
			CGMEEventLogger::LogGMEEvent(_T("    Paradigm violation: cannot connect selected objects!\r\n"));
			AfxMessageBox(_T("Paradigm violation: cannot connect selected objects!"),MB_ICONSTOP | MB_OK);
		}
	}
	catch(hresult_exception &e) {
		CGMEEventLogger::LogGMEEvent(_T("    Cannot Connect, hresult_exception in CGMEView::Connect\r\n"));
		AbortTransaction(e.hr);
//		AfxMessageBox(_T("Unable to connect specified parts!"),MB_ICONSTOP | MB_OK);
        Reset(true); // BGY: something similar needed, otherwise the created conenction not 
        // deleted form the gui if the committransaction failed
		ret = false;
	}
	catch(_com_error &e) {
		AbortTransaction(e.Error());
		CString error = _T("Cannot create connection");
		if (e.Description().length() != 0)
		{
			error += _T(": ");
			error += static_cast<const TCHAR*>(e.Description());
		}
		CGMEEventLogger::LogGMEEvent(error + _T("\r\n"));
		if (e.Error() != E_ABORT) // "Operation canceled by user"
			AfxMessageBox(error,MB_ICONSTOP | MB_OK);

        Reset(true); // BGY: something similar needed, otherwise the created conenction not 
        // deleted form the gui if the committransaction failed
		ret = false;
	}
	// error messages output to the console window
	// or simple refreshing of ActiveBrowser might leave
	// the focus set to another window, thus making the
	// connectmode cursor switch back to the normal one
	this->SetFocus();
	return ret;
}

void CGMEView::ResolveConnections()
{
	POSITION pos = connections.GetHeadPosition();
	while(pos)
		connections.GetNext(pos)->Resolve();
}

bool CGMEView::IsConnectionConversionNeeded(void)
{
	if (isModelAutoRouted)
		return false;

	POSITION pos = connections.GetHeadPosition();
	while(pos) {
		if (connections.GetNext(pos)->NeedsRouterPathConversion(false))
			return true;
	}

	return false;
}

void CGMEView::ConvertNeededConnections(void)
{
	BeginTransaction();
	POSITION pos = connections.GetHeadPosition();
	while (pos) {
		CGuiConnection* conn = connections.GetNext(pos);
		conn->ConvertAutoRoutedPathToCustom(currentAspect->index, false, false);
	}
	CommitTransaction();
}

void CGMEView::InsertNewPart(const CString& roleName, const CPoint& pt)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::InsertNewPart(")+roleName+_T(") in ")+path+name+_T("\r\n"));
	CComPtr<IMgaFCO> child;
	CComPtr<IMgaMetaRole> role;
	try {
		BeginTransaction();
		if(!currentAspect->GetRoleByName(roleName, role, true)) {
			AfxMessageBox(_T("Internal Program Error in CGMEView::InsertNewPart"));
			CGMEEventLogger::LogGMEEvent(_T("    Internal Program Error in CGMEView::InsertNewPart.\r\n"));
			AbortTransaction(E_FAIL);
			return;
		}
		COMTHROW(currentModel->CreateChildObject(role,&child));
		CComBSTR realRoleName;
		CComPtr<IMgaMetaFCO> childMeta;
		COMTHROW(child->get_Meta(&childMeta));
		CComBSTR kindName;
		COMTHROW(childMeta->get_Name(&kindName));
		CComBSTR bstrRoleName;
		COMTHROW(role->get_DisplayedName(&bstrRoleName));
		if (kindName == bstrRoleName) {
			CComBSTR kindDisplayName;
			COMTHROW(childMeta->get_DisplayedName(&kindDisplayName));
			COMTHROW(child->put_Name(kindDisplayName));
		} else {
			COMTHROW(child->put_Name(bstrRoleName));
		}

		CString newID;
		COMTHROW(child->get_ID(PutOut(newID)));
		newObjectIDs.AddHead(newID);

		SetObjectLocation(child, role, pt);
		__CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		newObjectIDs.RemoveAll();
		AfxMessageBox(_T("Unable to insert new part"),MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to insert new part.\r\n"));
		return;
	}
	catch(_com_error& e) {                
		AbortTransaction(e.Error());
		newObjectIDs.RemoveAll();
		CString error = _T("Unable to insert new part");
		if (e.Description().length() != 0)
		{
			error += _T(": ");
			error += static_cast<const TCHAR*>(e.Description());
		}
		CGMEEventLogger::LogGMEEvent(error + _T("\r\n"));
		AfxMessageBox(error,MB_ICONSTOP | MB_OK);
		return;
	}
	ChangeAttrPrefObjs(selected);
	try {
		BeginTransaction(TRANSACTION_READ_ONLY);
		if(currentAspect->IsLinkedRole(role) && parent != 0)
			ResetParent();
		CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to update parent"),MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to update parent.\r\n"));
	}
	Invalidate(true);
}

void CGMEView::ChangeAspect(CString aspName, bool p_eraseStack /*=true*/)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ChangeAspect(")+aspName+_T(") in ")+path+name+_T("\r\n"));
	if(currentAspect->name != aspName) {
		CGuiMetaAspect *newAsp = guiMeta->FindAspect(aspName);
		if(newAsp) {
			currentAspect = newAsp;
			SetAspectProperty(currentAspect->index);
			CGuiAnnotator::SetAspect(annotators, currentAspect->index);

			if( theApp.isHistoryEnabled())
			{
				if( m_isActive) // only the active view's changeaspect event is recorded
				{
					GetDocument()->tellHistorian( currentModel, currentAspect?currentAspect->name:_T(""));
				}

				if( p_eraseStack)
				{
					GetDocument()->clearForwHistory();
				}
			}
			CGuiFco::SetAspect(children,currentAspect->index);
			ResolveConnections();

			currentSet = 0;
			if(GetDocument()->GetEditMode() == GME_SET_MODE) {
				CGuiAnnotator::GrayOutAnnotations(annotators,false);
				CGuiFco::GrayOutFcos(children,false);
				CGuiFco::GrayOutFcos(connections,false);
			}

			modelGrid.Clear();
			FillModelGrid();
			AutoRoute(); // HACK we may have size change here, reroute the whole thing for now
			this->SendUnselEvent4List( &selected);

			// Keep selection active if it is still visible in the new aspect
			POSITION pos = selected.GetHeadPosition();
			while( pos != NULL ) {
				POSITION oldPos = pos;
				CGuiFco *gfco = selected.GetNext(pos);
				if(!gfco->IsVisible(currentAspect->index))
					selected.RemoveAt(oldPos);
			}
			UpdateConnectionSelection(currentAspect->index);

			RemoveAllAnnotationFromSelection();

			ChangeAttrPrefObjs(selected);


//			CGMEView* gmeviewA = (CGMEView*)GetActiveView();
//			if (gmeviewA)
			if (m_isActive)
			{
				TRACE(_T("CGMEView::ChangeAspect activeView\n"));
				DoPannWinRefresh();
			}
			Invalidate();
		}
	}
}

void CGMEView::ChangeAspect(int ind)
{
	CGuiMetaAspect *am = guiMeta->FindAspect(ind);
	VERIFY(am);
	ChangeAspect(am->name);
}

CString& CGMEView::GetAspectName(int ind)
{
	CGuiMetaAspect *am = guiMeta->FindAspect(ind);
	VERIFY(am);
	return am->name;
}

CString& CGMEView::GetCurrentAspectName(void)
{
	return GetAspectName(currentAspect->index);
}

CGMEView *CGMEView::GetActiveView()
{
	CMDIFrameWnd *pFrame = (CMDIFrameWnd*)AfxGetApp()->m_pMainWnd;
//	CMDIFrameWnd *pFrame = (CMDIFrameWnd*)AfxGetMainWnd( );
	if (!pFrame)
	{
		return NULL;
	}
	CMDIChildWnd *pChild = (CMDIChildWnd *) pFrame->GetActiveFrame();
//	BOOL ismax = FALSE;
//	CMDIChildWnd *pChild = pFrame->MDIGetActive(&ismax);
	if (!pChild)
	{
		return NULL;
	}
	return ((CGMEView *)(pChild->GetActiveView()));
}

/////////////////////////////////////////////////////////////////////////////
// CGMEView message handlers

BOOL CGMEView::OnEraseBkgnd(CDC* pDC)
{
	if (!pDC->IsPrinting() && !IsPreview()) {
		CreateOffScreen(pDC);
		OnPrepareDC(offScreen);
		CRect r;
		GetClientRect(&r);
		offScreen->DPtoLP(&r);
		r.InflateRect(5, 5);
		offScreen->FillSolidRect(&r,bgColor);
	}

	return TRUE;

//	return CScrollZoomView::OnEraseBkgnd(pDC);
}

BOOL CGMEView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (IsCursorChangedByDecorator() || isCursorChangedByEdgeCustomize)
		return TRUE;

	if (nHitTest != HTCLIENT) {
		SetCursor(editCursor);	// JIRA GME-222
		return FALSE;
	}

	if( GetFocus() != this )
		SetCursor(editCursor);
	else {
		CGMEDoc *pDoc = GetDocument();
		switch(pDoc->GetEditMode()) {
		case GME_AUTOCONNECT_MODE:
		case GME_SHORTAUTOCONNECT_MODE:
			SetCursor(connSrc ? autoconnect2Cursor : autoconnectCursor);
			break;
		case GME_DISCONNECT_MODE:
		case GME_SHORTDISCONNECT_MODE:
			SetCursor(connSrc ? disconnect2Cursor : disconnectCursor);
			break;
		case GME_SET_MODE:
			SetCursor(currentSet ? set2Cursor : setCursor);
			break;
		case GME_ZOOM_MODE:
			SetCursor(zoomCursor);
			break;
		case GME_VISUAL_MODE:
			SetCursor(visualCursor);
			break;
		default:
			SetCursor(tmpConnectMode ? autoconnect2Cursor : editCursor);
			break;
		}
	}

	ShowCursor(TRUE);
	return TRUE;
}

void CGMEView::OnDropFiles(HDROP p_hDropInfo)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnDropFiles in ") + path + name + _T("\r\n"));
	if (GetDocument()->GetEditMode() == GME_EDIT_MODE) {
		CPoint point;
		DragQueryPoint(p_hDropInfo, &point);
		CoordinateTransfer(point);

		CGMEView* self = const_cast<CGMEView*> (this);
		CGuiObject*	object	= self ? self->FindObject(point, true, true) : 0;
		if (object == NULL)	// not label of the object but can be some port label inside the object
			object = self ? self->FindObject(point, true, false) : 0;

		HRESULT retVal = S_OK;
		if (object != NULL) {
			CGuiAspect* pAspect = object->GetCurrentAspect();
			if (pAspect != NULL) {
				CComQIPtr<IMgaElementDecorator> newDecorator(pAspect->GetDecorator());
				if (newDecorator) {
					CClientDC transformDC(this);
					OnPrepareDC(&transformDC);
					retVal = newDecorator->DropFile((ULONGLONG)p_hDropInfo, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
					if (retVal == S_DECORATOR_EVENT_HANDLED) {
						if (IsInOpenedDecoratorTransaction()) {
							if (ShouldCommitOperation()) {
								try {
									CommitTransaction();
								} catch (hresult_exception& e) {
									// GME-292: the commit may fail
									AbortTransaction(e.hr);
									if (e.hr != E_MGA_CONSTRAINT_VIOLATION) {
										CGMEEventLogger::LogGMEEvent(_T("    Couldn't commit transaction.\r\n"));
										AfxMessageBox(_T("Couldn't commit transaction."),MB_ICONSTOP | MB_OK);
									}
								}
								SetShouldCommitOperation(false);
								SetObjectInDecoratorOperation(NULL);
								SetAnnotatorInDecoratorOperation(NULL);
								SetInOpenedDecoratorTransaction(false);
								SetIsContextInitiatedOperation(false);
								SetInElementDecoratorOperation(false);
								return;
							} else if (!IsInElementDecoratorOperation()) {
								AbortTransaction(S_OK);
								SetInOpenedDecoratorTransaction(false);
								SetIsContextInitiatedOperation(false);
								SetInElementDecoratorOperation(false);
								return;
							}
							// Note: DecoratorLib in-place edit currently not use SetCapture,
							// but ReleaseCapture can be handy if some decorator issues SetCapture
							if (::GetCapture() != NULL)
								::ReleaseCapture();
							SetInOpenedDecoratorTransaction(false);
							SetIsContextInitiatedOperation(false);
						} else {
							if (IsInElementDecoratorOperation()) {
								if (IsDecoratorOrAnnotator())
									SetObjectInDecoratorOperation(object);
								else
									ASSERT(false);
							}
							return;
						}
					} else if (retVal != S_OK &&
							   retVal != S_DECORATOR_EVENT_NOT_HANDLED &&
							   retVal != E_DECORATOR_NOT_IMPLEMENTED)
					{
						CancelDecoratorOperation();
						// FIXME: how to handle this error?
						// COMTHROW(retVal);
						return;
					}
					if (IsInElementDecoratorOperation()) {
						if (IsDecoratorOrAnnotator())
							SetObjectInDecoratorOperation(object);
						else
							ASSERT(false);
					}
				}
			}
		}
		if (IsInElementDecoratorOperation())
			return;
		if (retVal == S_DECORATOR_EVENT_HANDLED)
			return;
	}

	CMainFrame::theInstance->OnDropFiles(p_hDropInfo);
}

void CGMEView::OnKillfocusNameProp()
{
	if(!initDone)
		return;
	CString txt;
	GetNameProperty(txt);
	if(txt != name) {
		if(currentModel != 0) {
			try {
				BeginTransaction();
				COMTHROW(currentModel->put_Name(_bstr_t(txt)));
				CommitTransaction();
				RetrievePath();
				name = txt;
				SetTitles();
			}
			catch(hresult_exception &e) {
				AbortTransaction(e.hr);
				AfxMessageBox(_T("Unable to set model name"));
				SetNameProperty();
			}
		}
	}
}

void CGMEView::OnSelChangeAspectProp()
{
	int ind = GetAspectProperty();
	CGuiMetaAspect *am = guiMeta->FindAspect(ind);
	VERIFY(am);
	CString aspName = am->name;
	if(currentAspect->name != aspName) 
	{
		currentAspect = guiMeta->FindAspect(aspName);
		CMainFrame::theInstance->ChangePartBrowserAspect(currentAspect->index);
		CGuiAnnotator::SetAspect(annotators, currentAspect->index);
		CGuiFco::SetAspect(children,currentAspect->index);
		ResolveConnections();
		if( theApp.isHistoryEnabled())
		{
			GetDocument()->tellHistorian( currentModel, currentAspect?currentAspect->name:_T(""));
			GetDocument()->clearForwHistory();
		}

		currentSet = 0;
		if(GetDocument()->GetEditMode() == GME_SET_MODE) {
			CGuiAnnotator::GrayOutAnnotations(annotators,false);
			CGuiFco::GrayOutFcos(children,false);
			CGuiFco::GrayOutFcos(connections,false);
		}

		modelGrid.Clear();
		FillModelGrid();
		AutoRoute(); // HACK we may have size change here, reroute the whole thing for now
		this->SendUnselEvent4List( &selected);
		selected.RemoveAll();
		RemoveAllAnnotationFromSelection();
		ClearConnectionSelection();
		if (m_isActive)
		{
			DoPannWinRefresh();
		}

		TRACE(_T("CGMEView::OnSelChangeAspectProp\n"));
		Invalidate();
	}
}

bool CGMEView::ChangePrnAspect(CString aspName)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ChangePrnAspect(")+aspName+_T(") in ")+path+name+_T("\r\n"));
	if(currentAspect->name == aspName)
		return true;
	CGuiMetaAspect *newAsp = guiMeta->FindAspect(aspName);
	if(!newAsp) 
		return false;
	currentAspect = newAsp;
	SetAspectProperty(currentAspect->index);
	CGuiAnnotator::SetAspect(annotators, currentAspect->index);
	CGuiFco::SetAspect(children,currentAspect->index);
	ResolveConnections();

	modelGrid.Clear();
	FillModelGrid();
	AutoRoute(); // HACK we may have size change here, reroute the whole thing for now
	this->SendUnselEvent4List( &selected);
	selected.RemoveAll();
	RemoveAllAnnotationFromSelection();
	ClearConnectionSelection();
	return true;
}

void CGMEView::OnLButtonUp(UINT nFlags, CPoint point)
{
	isLeftMouseButtonDown = false;
	isConnectionJustSelected = false;
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnLButtonUp in ") + path + name + _T("\r\n"));

	CPoint ppoint = point;
	if (!tmpConnectMode) {
		CPoint trackPoint = point;
		CoordinateTransfer(point);

		CGMEDoc* doc = GetDocument();
		switch(doc->GetEditMode()) {
		case GME_EDIT_MODE:
			{
				CGMEEventLogger::LogGMEEvent(_T("    mode=GME_EDIT_MODE\r\n"));

				CGMEView* self = const_cast<CGMEView*> (this);
				CGuiObject*	object	= self		? self->FindObject(point, true) : 0;
//				CGuiPort*	port	= object	? object->FindPort(point, true) : 0;
				if (object == NULL)
					object = self ? self->FindObject(point, true, true) : 0;

				if (IsInElementDecoratorOperation()) {
					CComPtr<IMgaElementDecorator> newDecorator;
					if (IsDecoratorOrAnnotator()) {
						CGuiObject* objInOp = GetObjectInDecoratorOperation();
						if (objInOp != NULL) {
							CGuiAspect* pAspect = objInOp->GetCurrentAspect();
							if (pAspect != NULL) {
								CComQIPtr<IMgaElementDecorator> newDecorator2(pAspect->GetDecorator());
								newDecorator = newDecorator2;
							}
						}
					} else {
						CGuiAnnotator* annInOp = GetAnnotatorInDecoratorOperation();
						if (annInOp != NULL)
							newDecorator = annInOp->GetDecorator(currentAspect->index);
					}
					if (newDecorator) {
						CClientDC transformDC(this);
						OnPrepareDC(&transformDC);
						HRESULT retVal = newDecorator->MouseLeftButtonUp(nFlags, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
						if (retVal == S_DECORATOR_EVENT_HANDLED) {
							if (IsInOpenedDecoratorTransaction()) {
								if (ShouldCommitOperation()) {
									try {
										CommitTransaction();
									} catch (hresult_exception& e) {
										// GME-292: the commit may fail
										AbortTransaction(e.hr);
										if (e.hr != E_MGA_CONSTRAINT_VIOLATION) {
											CGMEEventLogger::LogGMEEvent(_T("    Couldn't commit transaction.\r\n"));
											AfxMessageBox(_T("Couldn't commit transaction."),MB_ICONSTOP | MB_OK);
										}
									}
									SetShouldCommitOperation(false);
									SetObjectInDecoratorOperation(NULL);
									SetAnnotatorInDecoratorOperation(NULL);
								} else {
									AbortTransaction(S_OK);
								}
								SetInOpenedDecoratorTransaction(false);
								SetIsContextInitiatedOperation(false);
							}
							break;
						} else if (retVal != S_OK &&
								   retVal != S_DECORATOR_EVENT_NOT_HANDLED &&
								   retVal != E_DECORATOR_NOT_IMPLEMENTED)
						{
							CancelDecoratorOperation();
							// FIXME: how to handle this error?
							// COMTHROW(retVal);
							return;
						}
					}
				} else if (isInConnectionCustomizeOperation) {
					isInConnectionCustomizeOperation = false;
					::SetCursor(customizeConnectionCursorBackup);
					isCursorChangedByEdgeCustomize = false;
					bool trySnap = false;
					if (customizeConnectionType == SimpleEdgeDisplacement) {
						int newPos = 0;
						if (customizeConnectionPartMoveMethod == HorizontalEdgeMove ||
							customizeConnectionPartMoveMethod == AdjacentEdgeMove)
						{
							newPos = min(max(point.x, customizeConnectionEdgeXMinLimit),
													  customizeConnectionEdgeXMaxLimit);
							int edgeIndex = customizeConnectionEdgeIndex;
							if (customizeConnectionPartMoveMethod == AdjacentEdgeMove && customizeHorizontalOrVerticalEdge)
								edgeIndex++;
							UpdateCustomEdges(selectedConnection, SimpleEdgeDisplacement, newPos, 0, edgeIndex, false);
						}
						if (customizeConnectionPartMoveMethod == VerticalEdgeMove ||
							customizeConnectionPartMoveMethod == AdjacentEdgeMove)
						{
							newPos = min(max(point.y, customizeConnectionEdgeYMinLimit),
													  customizeConnectionEdgeYMaxLimit);
							int edgeIndex = customizeConnectionEdgeIndex;
							if (customizeConnectionPartMoveMethod == AdjacentEdgeMove && !customizeHorizontalOrVerticalEdge)
								edgeIndex++;
							UpdateCustomEdges(selectedConnection, SimpleEdgeDisplacement, 0, newPos, edgeIndex, true);
						}
					} else if (customizeConnectionType == CustomPointCustomization) {
						if (customizeConnectionPartMoveMethod == InsertNewCustomPoint) {
							InsertCustomEdge(selectedConnection, CustomPointCustomization, point.x, point.y, customizeConnectionEdgeIndex, true);
							trySnap = true;
						} else if (customizeConnectionPartMoveMethod == ModifyExistingCustomPoint) {
							long edgeIndex = selectedConnection->IsPointOnSectionAndDeletable(customizeConnectionEdgeIndex, point);
							if (edgeIndex >= 0) {
								DeleteCustomEdges(selectedConnection, CustomPointCustomization, edgeIndex);
							} else {
								UpdateCustomEdges(selectedConnection, CustomPointCustomization, point.x, point.y, customizeConnectionEdgeIndex, true);
								trySnap = true;
							}
						}
					}
					if (trySnap && !(nFlags & MK_CONTROL))	// Control button disables snapping
						selectedConnection->VerticalAndHorizontalSnappingOfConnectionLineSegments(currentAspect->index, customizeConnectionEdgeIndex);
					selectedConnection->WriteCustomPathData();
				} else {
					CGuiAnnotator* annotation = NULL;
					CComPtr<IMgaElementDecorator> newDecorator;
					if (object != NULL) {
						CGuiAspect* pAspect = object->GetCurrentAspect();
						if (pAspect != NULL) {
							CComQIPtr<IMgaElementDecorator> newDecorator2(pAspect->GetDecorator());
							newDecorator = newDecorator2;
						}
					} else {
						annotation = FindAnnotation(point);
						if (annotation)
							newDecorator = annotation->GetDecorator(currentAspect->index);
					}
					if (newDecorator) {
						CClientDC transformDC(this);
						OnPrepareDC(&transformDC);
						HRESULT retVal = newDecorator->MouseLeftButtonUp(nFlags, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
						if (retVal == S_DECORATOR_EVENT_HANDLED) {
							doNotDeselectAfterInPlaceEdit = true;

							if (IsInOpenedDecoratorTransaction()) {
								if (ShouldCommitOperation()) {
									try {
										CommitTransaction();
									} catch (hresult_exception& e) {
										// GME-292: the commit may fail
										AbortTransaction(e.hr);
										if (e.hr != E_MGA_CONSTRAINT_VIOLATION) {
											CGMEEventLogger::LogGMEEvent(_T("    Couldn't commit transaction.\r\n"));
											AfxMessageBox(_T("Couldn't commit transaction."),MB_ICONSTOP | MB_OK);
										}
									}
									SetShouldCommitOperation(false);
									SetObjectInDecoratorOperation(NULL);
									SetAnnotatorInDecoratorOperation(NULL);
									SetInOpenedDecoratorTransaction(false);
									SetIsContextInitiatedOperation(false);
								} else if (!IsInElementDecoratorOperation()) {
									AbortTransaction(S_OK);
									SetInOpenedDecoratorTransaction(false);
									SetIsContextInitiatedOperation(false);
								} else {
									// Note: DecoratorLib in-place edit currently not use SetCapture,
									// but ReleaseCapture can be handy if some decorator issues SetCapture
									if (::GetCapture() != NULL)
										::ReleaseCapture();
								}
								SetInElementDecoratorOperation(false);
								SetInOpenedDecoratorTransaction(false);
								SetIsContextInitiatedOperation(false);
							}
						}
						if (retVal != S_OK &&
							retVal != S_DECORATOR_EVENT_HANDLED &&
							retVal != S_DECORATOR_EVENT_NOT_HANDLED &&
							retVal != E_DECORATOR_NOT_IMPLEMENTED)
						{
							CancelDecoratorOperation();
							// FIXME: how to handle this error?
							// COMTHROW(retVal);
							return;
						}
						if (IsInElementDecoratorOperation()) {
							if (IsDecoratorOrAnnotator())
								SetObjectInDecoratorOperation(object);
							else
								SetAnnotatorInDecoratorOperation(annotation);
						}
					} else {
						SetConnectionCustomizeCursor(point);
					}
				}
			}	//case
		}	// switch
	}	// if (!tmpConnectMode)
	CScrollZoomView::OnLButtonUp(nFlags, ppoint);
}

void CGMEView::OnLButtonDown(UINT nFlags, CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnLButtonDown in ")+path+name+_T("\r\n"));
	isLeftMouseButtonDown = true;
	if (!(nFlags & MK_LBUTTON)) {
		// PETER: this was needed to discard "got focus" situations: eg.: add-on dialog appears during attribute editing
		return;
	}

	CPoint trackPoint = point;
	CPoint ppoint = point;
	CoordinateTransfer(point);

	if(tmpConnectMode) {
		if(connTmp && (connSrc != 0)) {
			Connect(connSrc, connSrcPort, connSrcHotSide, connTmp, connTmpPort, connTmpHotSide, 0 != (nFlags & MK_SHIFT));
		}
		tmpConnectMode = false;
		ClearConnSpecs();
		SetCursor(editCursor);
		ShowCursor(TRUE);
		Invalidate();
	}
	else {
		CGMEDoc *doc = GetDocument();
		switch(doc->GetEditMode()) {
		case GME_EDIT_MODE:
			{
				CGMEEventLogger::LogGMEEvent(_T("    mode=GME_EDIT_MODE\r\n"));

				CGMEView* self = const_cast<CGMEView*> (this);
				CGuiObject*	object	= self		? self->FindObject(point, true) : 0;
//				CGuiPort*	port	= object	? object->FindPort(point, true) : 0;
				if (object == NULL)
					object = self ? self->FindObject(point, true, true) : 0;

				if (IsInElementDecoratorOperation()) {
					CComPtr<IMgaElementDecorator> newDecorator;
					if (IsDecoratorOrAnnotator()) {
						CGuiObject* objInOp = GetObjectInDecoratorOperation();
						if (objInOp != NULL) {
							CGuiAspect* pAspect = objInOp->GetCurrentAspect();
							if (pAspect != NULL) {
								CComQIPtr<IMgaElementDecorator> newDecorator2(pAspect->GetDecorator());
								newDecorator = newDecorator2;
							}
						}
					} else {
						CGuiAnnotator* annInOp = GetAnnotatorInDecoratorOperation();
						if (annInOp != NULL)
							newDecorator = annInOp->GetDecorator(currentAspect->index);
					}
					if (newDecorator) {
						CClientDC transformDC(this);
						OnPrepareDC(&transformDC);
						HRESULT retVal = newDecorator->MouseLeftButtonDown(nFlags, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
						if (retVal == S_DECORATOR_EVENT_HANDLED) {
							if (IsInOpenedDecoratorTransaction()) {
								if (ShouldCommitOperation()) {
									try {
										CommitTransaction();
									} catch (hresult_exception& e) {
										// GME-292: the commit may fail
										AbortTransaction(e.hr);
										if (e.hr != E_MGA_CONSTRAINT_VIOLATION) {
											CGMEEventLogger::LogGMEEvent(_T("    Couldn't commit transaction.\r\n"));
											AfxMessageBox(_T("Couldn't commit transaction."),MB_ICONSTOP | MB_OK);
										}
									}
									SetShouldCommitOperation(false);
									SetObjectInDecoratorOperation(NULL);
									SetAnnotatorInDecoratorOperation(NULL);
								} else {
									AbortTransaction(S_OK);
								}
								SetInElementDecoratorOperation(false);
								SetInOpenedDecoratorTransaction(false);
								SetIsContextInitiatedOperation(false);
							}
						} else if (retVal != S_OK &&
								   retVal != S_DECORATOR_EVENT_NOT_HANDLED &&
								   retVal != E_DECORATOR_NOT_IMPLEMENTED)
						{
							CancelDecoratorOperation();
							// FIXME: how to handle this error?
							// COMTHROW(retVal);
							return;
						}
						CScrollZoomView::OnLButtonDown(nFlags, ppoint);
						return;
					}
				} else {
					CGuiAnnotator* annotation = NULL;
					CComPtr<IMgaElementDecorator> newDecorator;
					if (object != NULL) {
						CGuiAspect* pAspect = object->GetCurrentAspect();
						if (pAspect != NULL) {
							CComQIPtr<IMgaElementDecorator> newDecorator2(pAspect->GetDecorator());
							newDecorator = newDecorator2;
						}
					} else {
						annotation = FindAnnotation(point);
						if (annotation)
							newDecorator = annotation->GetDecorator(currentAspect->index);
					}
					if (selectedConnection != NULL && !IsInstance()) {	// customization not allowed in instances
						// Start edge moving operation if needed
						bool isPartFixed = false;
						customizeConnectionEdgeIndex = selectedConnection->GetEdgeIndex(
																		point,
																		customizeConnectionEdgeStartPoint,
																		customizeConnectionEdgeEndPoint,
																		customizeConnectionEdgeThirdPoint,
																		customizeConnectionPartMoveMethod,
																		customizeHorizontalOrVerticalEdge,
																		isPartFixed,
																		customizeConnectionEdgeXMinLimit,
																		customizeConnectionEdgeXMaxLimit,
																		customizeConnectionEdgeYMinLimit,
																		customizeConnectionEdgeYMaxLimit);
						if (customizeConnectionEdgeIndex >= 0 && !isPartFixed) {
							TRACE(_T("Starting edge customize operation of %ld.: (%ld, %ld)-(%ld, %ld)-(%ld, %ld) min/max: X(%ld, %ld) Y(%ld, %ld) h/v: %d\n"),
									customizeConnectionEdgeIndex,
									customizeConnectionEdgeStartPoint.x, customizeConnectionEdgeStartPoint.y,
									customizeConnectionEdgeEndPoint.x, customizeConnectionEdgeEndPoint.y,
									customizeConnectionEdgeThirdPoint.x, customizeConnectionEdgeThirdPoint.y,
									customizeConnectionEdgeXMinLimit, customizeConnectionEdgeXMaxLimit,
									customizeConnectionEdgeYMinLimit, customizeConnectionEdgeYMaxLimit,
									customizeConnectionPartMoveMethod);
							if (selectedConnection->IsAutoRouted()) {
								customizeConnectionType = SimpleEdgeDisplacement;
							} else {
								customizeConnectionType = CustomPointCustomization;
							}
							isInConnectionCustomizeOperation = true;
							customizeConnectionOrigCursor = point;
							customizeConnectionCurrCursor = point;
							Invalidate();
						}
					} else if (newDecorator && !isInConnectionCustomizeOperation) {
						CClientDC transformDC(this);
						OnPrepareDC(&transformDC);
						HRESULT retVal = newDecorator->MouseLeftButtonDown(nFlags, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
						if (retVal == S_DECORATOR_EVENT_HANDLED) {
							if (IsInOpenedDecoratorTransaction()) {
								if (ShouldCommitOperation()) {
									try {
										CommitTransaction();
									} catch (hresult_exception& e) {
										// GME-292: the commit may fail
										AbortTransaction(e.hr);
										if (e.hr != E_MGA_CONSTRAINT_VIOLATION) {
											CGMEEventLogger::LogGMEEvent(_T("    Couldn't commit transaction.\r\n"));
											AfxMessageBox(_T("Couldn't commit transaction."),MB_ICONSTOP | MB_OK);
										}
									}
									SetShouldCommitOperation(false);
									SetObjectInDecoratorOperation(NULL);
									SetAnnotatorInDecoratorOperation(NULL);
									SetInOpenedDecoratorTransaction(false);
									SetIsContextInitiatedOperation(false);
									SetInElementDecoratorOperation(false);
									CScrollZoomView::OnLButtonDown(nFlags, ppoint);
									return;
								} else if (!IsInElementDecoratorOperation()) {
									AbortTransaction(S_OK);
									SetInOpenedDecoratorTransaction(false);
									SetIsContextInitiatedOperation(false);
									SetInElementDecoratorOperation(false);
									CScrollZoomView::OnLButtonDown(nFlags, ppoint);
									return;
								}
								// Note: DecoratorLib in-place edit currently not use SetCapture,
								// but ReleaseCapture can be handy if some decorator issues SetCapture
								if (::GetCapture() != NULL)
									::ReleaseCapture();
								SetInElementDecoratorOperation(false);
							} else {
								if (IsInElementDecoratorOperation()) {
									if (IsDecoratorOrAnnotator())
										SetObjectInDecoratorOperation(object);
									else
										SetAnnotatorInDecoratorOperation(annotation);
								}
								CScrollZoomView::OnLButtonDown(nFlags, ppoint);
								return;
							}
						} else if (retVal != S_OK &&
								   retVal != S_DECORATOR_EVENT_NOT_HANDLED &&
								   retVal != E_DECORATOR_NOT_IMPLEMENTED)
						{
							CancelDecoratorOperation();
							// FIXME: how to handle this error?
							// COMTHROW(retVal);
							return;
						}
						if (IsInElementDecoratorOperation()) {
							if (IsDecoratorOrAnnotator())
								SetObjectInDecoratorOperation(object);
							else
								SetAnnotatorInDecoratorOperation(annotation);
						}
					}
				}
				if (IsInElementDecoratorOperation() || isInConnectionCustomizeOperation) {
					CScrollZoomView::OnLButtonDown(nFlags, ppoint);
					return;
				}

				CGuiObject* selection = FindObject(point);
				if (selection == NULL)	// select with label
					selection = FindObject(point, false, true);

				CGuiAnnotator* annotation = selection ? NULL : FindAnnotation(point);
				CGuiConnection* connection = router.FindConnection(point);

				POSITION alreadySelected = 0;
				if((selection != 0) || (annotation != 0)) {
					if (selection) {
						CGMEEventLogger::LogGMEEvent(_T("    LButton over ")+selection->GetName()+_T(" ")+selection->GetID()+_T("\r\n")); 
						alreadySelected = selected.Find(selection);
						if(alreadySelected)
						{
							this->SendUnselEvent4Object( selected.GetAt( alreadySelected));
							selected.RemoveAt(alreadySelected);
						}
						else if(!(nFlags & MK_CONTROL)) {
							RemoveAllAnnotationFromSelection();
							this->SendUnselEvent4List( &selected);
							selected.RemoveAll();
							ClearConnectionSelection();
						}

						this->SendSelecEvent4Object( selection);
						selected.AddHead(selection);
					}
					if (annotation) {
						CGMEEventLogger::LogGMEEvent(_T("    LButton over ")+annotation->GetName()+_T("\r\n")); 
						alreadySelected = selectedAnnotations.Find(annotation);
						if (alreadySelected) {
							RemoveAnnotationFromSelection(alreadySelected);
						} else if (!(nFlags & MK_CONTROL)) {
							this->SendUnselEvent4List( &selected);
							selected.RemoveAll();
							RemoveAllAnnotationFromSelection();
						}
						AddAnnotationToSelectionHead(annotation);
					}

					inDrag = true;
					CRect selRect(0,0,0,0);
					CRect selAnnRect(0,0,0,0);
					CGuiObject::GetExtent(selected,selRect);
					CGuiAnnotator::GetExtent(selectedAnnotations, selAnnRect);
					dragRect.UnionRect(selRect, selAnnRect);

					CPoint ptClickOffset(point.x - dragRect.left,
											point.y - dragRect.top);
					CRect rectAwake = CRect(trackPoint.x,trackPoint.y,trackPoint.x + 1,trackPoint.y + 1);
					rectAwake.InflateRect(GetSystemMetrics(SM_CXDRAG), GetSystemMetrics(SM_CYDRAG));
					ClientToScreen(&dragRect);
					ClientToScreen(&rectAwake);

					CRectList rects;
					CRectList annRects;
					CGuiObject::GetRectList(selected,rects);
					CGuiAnnotator::GetRectList(selectedAnnotations,annRects);
					CGMEDataDescriptor desc(rects,annRects,point,ptClickOffset);
					CGMEDataDescriptor::destructList( rects);
					CGMEDataDescriptor::destructList( annRects);

					validGuiObjects = true;
					dragSource = (selected.GetCount() > 0) ? selected.GetHead() : NULL;
					DROPEFFECT dropEffect = DROPEFFECT_NONE;
					if (!isLibObject) {
						dropEffect = CGMEDoc::DoDragDrop(&selected, &selectedAnnotations, &desc,
							DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK, &rectAwake, this);
					}
					if (validGuiObjects && dropEffect == DROPEFFECT_NONE) {
						if (!isLibObject && inDrag && alreadySelected != NULL && (selection || annotation)) {
							OnLButtonUp(nFlags, ppoint);
						}
						if (nFlags & MK_CONTROL) {
							if (selection) {
								if (alreadySelected) {
									this->SendUnselEvent4Object( selected.GetHead());
									selected.RemoveHead();
									selection = selected.GetCount() ? selected.GetHead() : 0;
								}
								ChangeAttrPrefObjs(selected);
							}
							if (annotation) {
								if (alreadySelected) {
									RemoveAnnotationFromSelectionHead();
									annotation = selectedAnnotations.GetCount() ? selectedAnnotations.GetHead() : 0;
									/*
									if(annotation)
										ChangeAttrPrefFco(annotation);
									*/
								}
								else
									/*
									ChangeAttrPrefFco(annotation);
									*/ ;
							}
						}
						else if (!doNotDeselectAfterInPlaceEdit) {
							this->SendUnselEvent4List( &selected);
							selected.RemoveAll();
							RemoveAllAnnotationFromSelection();
							ClearConnectionSelection();
							if (selection) {
								if(!alreadySelected) {
									this->SendSelecEvent4Object( selection);
									selected.AddHead(selection);
									ChangeAttrPrefObjs(selected);
								}
							}
							if (annotation) {
								if(!alreadySelected) {
									AddAnnotationToSelectionHead(annotation);
									 /* ChangeAttrPrefFco(selectedAnnotations.GetTail()); */
								}
							}
						}
						doNotDeselectAfterInPlaceEdit = false;
					}
					else if (!isLibObject) {
						ChangeAttrPrefObjs(selected);
					}
					Invalidate();
					if(inDrag && dropEffect == DROPEFFECT_MOVE) {
/* HACK
						int num;
						if((num = currentModel->CheckForReferences(selected)) > 0) {
							char txt[128];
							sprintf(txt,_T("Selected model(s) cannot be deleted due to %ld reference(s)"),num);
							AfxMessageBox(txt,MB_OK | MB_ICONSTOP);
						}
						else if((num = currentModel->CheckForInherited(selected)) > 0) {
							char txt[128];
							sprintf(txt,_T("Selected model(s) cannot be deleted due to %ld inherited part(s)"),num);
							AfxMessageBox(txt,MB_OK | MB_ICONSTOP);
						}
						else {
							MoveToTrash(selected);
						 	CGMEDoc *doc = GetDocument();
							doc->SetModifiedFlag(TRUE);
							selected.RemoveAll();
							selection = 0;
						}
*/
					}
					inDrag = false;
				}
				else {
					this->SendUnselEvent4List( &selected);
					selected.RemoveAll();
					RemoveAllAnnotationFromSelection();
					ClearConnectionSelection();
					selection = 0;
					annotation = 0;

					if (connection) {
						ChangeAttrPrefFco(connection);
					}
					else {
						CRectTracker tracker;
						CClientDC dc(this);
						OnPrepareDC(&dc);
						if(tracker.TrackRubberBand(this, trackPoint,TRUE)) {
							tracker.m_rect.NormalizeRect();
							dc.DPtoLP(tracker.m_rect);
							FindObjects(tracker.m_rect,selected);
							this->SendSelecEvent4List( &selected);
							FindAnnotations(tracker.m_rect,selectedAnnotations);
							if(selected.GetCount() > 0) {
								selection = selected.GetHead();
							}
							else if (selectedAnnotations.GetCount() > 0 ){
								annotation = selectedAnnotations.GetHead();
								// ANNTODO: ChangeAttrPrefFco...
							}
						}
						ChangeAttrPrefObjs(selected);
					}
				}
				bool succ = this->SendNow();
				Invalidate( succ);
			}
			break;
		case GME_AUTOCONNECT_MODE:
		case GME_SHORTAUTOCONNECT_MODE:
			{
				CGMEEventLogger::LogGMEEvent(_T("    mode=GME_AUTOCONNECT_MODE\r\n"));
				if(connTmp) {
					if(connSrc == 0) {
						connSrc = connTmp;
						connSrcPort = connTmpPort;
						connSrcHotSide = connTmpHotSide;
					}
					else {
						Connect(connSrc, connSrcPort, connSrcHotSide, connTmp, connTmpPort, connTmpHotSide, 0 != (nFlags & MK_SHIFT));
						ClearConnSpecs();
						if( doc->GetEditMode() == GME_SHORTAUTOCONNECT_MODE)
							GetDocument()->SetMode(0); // switch back to GME_EDIT_MODE
					}

				}
			}
			Invalidate();
			break;
		case GME_DISCONNECT_MODE:
		case GME_SHORTDISCONNECT_MODE:
			{
				CGMEEventLogger::LogGMEEvent(_T("    mode=GME_DISCONNECT_MODE\r\n"));
				CGuiObject *selection = FindObject(point);
				if(selection) {
					CGuiPort *port = 0;
					port = selection->FindPort(point);
					if(connSrc == 0) {
						if (port != 0) {
							connSrc = port->parent->GetParent();
							connSrcPort = port;
							CGuiConnectionList conns;
							FindConnections(connSrc,connSrcPort,conns);
							if(conns.GetCount() < 2) {
								if(conns.GetCount() == 1) {
									if(!DeleteConnection(conns.GetHead())) {
										AfxMessageBox(_T("Connection cannot be deleted!"));
									}
								}
								else if(conns.GetCount() == 0) {
									AfxMessageBox(_T("Selected object is not connected!"));
								}
								ClearConnSpecs();
								if( doc->GetEditMode() == GME_SHORTDISCONNECT_MODE)
									GetDocument()->SetMode(0); // switch back to GME_EDIT_MODE
							}
						} else {
							AfxMessageBox(_T("Cannot find port to connection!"));
						}
					}
					else {
						CGuiConnectionList conns;
						FindConnections(connSrc,connSrcPort,selection,port,conns);
						if(conns.GetCount()) {
							if(!DeleteConnection(conns.GetHead()))
								AfxMessageBox(_T("Connection cannot be deleted!"));
						}
						else
							AfxMessageBox(_T("Selected objects are not connected!"));
						ClearConnSpecs();
						if( doc->GetEditMode() == GME_SHORTDISCONNECT_MODE)
							GetDocument()->SetMode(0); // switch back to GME_EDIT_MODE
					}
				}
				if(!selection) {
					CGuiConnection *conn = router.FindConnection(point);
					if(conn) {
						if(!DeleteConnection(conn))
							AfxMessageBox(_T("Connection cannot be deleted!"));
						ClearConnSpecs();
						if( doc->GetEditMode() == GME_SHORTDISCONNECT_MODE)
							GetDocument()->SetMode(0); // switch back to GME_EDIT_MODE
					}
				}
			}
			Invalidate();
			break;
		case GME_SET_MODE:
			{
				CGMEEventLogger::LogGMEEvent(_T("    mode=GME_SET_MODE\r\n"));
				if(!currentSet)
					break;
				CGuiFco *fco = 0;
				try {
					BeginTransaction(TRANSACTION_READ_ONLY);
					CGuiObject *object = FindObject(point);
					if(object) {
						CGMEEventLogger::LogGMEEvent(_T("    LButton over ")+object->GetName()+_T(" ")+object->GetID()+_T("\r\n"));
						if(currentSet->CheckMember(object))
							fco = object;
					}
					else {
						CGuiConnection *conn = router.FindConnection(point);
						if(conn && currentSet->CheckMember(conn))
							fco = conn;
					}
					CommitTransaction();
				}
				catch(hresult_exception &e) {
					AbortTransaction(e.hr);
					break;
				}
				if(fco) {
					try {
						BeginTransaction();
						currentSet->ToggleMember(fco);	// nothing else needs to be done: events will be generated!
						CommitTransaction();
					}
					catch(hresult_exception &e) {
						AbortTransaction(e.hr);
						break;
					}
				}
			}
			break;
		case GME_ZOOM_MODE:
			{
				CGMEEventLogger::LogGMEEvent(_T("    mode=GME_ZOOM_MODE\r\n"));
                CRectTracker tracker;
				tracker.m_rect = CRect(0,0,0,0);
                if(tracker.TrackRubberBand(this, trackPoint,TRUE)) 
				{
					CRect truerect(0,0,0,0);
					tracker.GetTrueRect(&truerect);
					truerect.NormalizeRect();
					if (truerect.Height() <= MIN_ZOOM_RECT && truerect.Width() <= MIN_ZOOM_RECT)
						ZoomIn(ppoint); 
					else {
						ZoomRect(truerect);
					}
                }
				else
					ZoomIn(ppoint); 
				Invalidate();
			}
			break;
		case GME_VISUAL_MODE:
			{
				CGMEEventLogger::LogGMEEvent(_T("    mode=GME_VISUAL_MODE\r\n"));
				CGuiObject *obj = FindObject(point);
				if(obj) {
					CGMEEventLogger::LogGMEEvent(_T("    LButton over ")+obj->GetName()+_T(" ")+obj->GetID()+_T("\r\n"));
					obj->ToggleGrayOut();
					CGuiFco::GrayOutNonInternalConnections(connections);
					Invalidate();
				}
				else {
					CGuiConnection *conn = router.FindConnection(point);
					if(conn) {
						CGMEEventLogger::LogGMEEvent(_T("    LButton over ")+conn->GetName()+_T(" ")+conn->GetID()+_T("\r\n"));
						conn->ToggleGrayOut();
						conn->GrayOutEndPoints();
						Invalidate();
					}
					else {
						CGuiAnnotator *ann = FindAnnotation(point);
						if (ann) {
							CGMEEventLogger::LogGMEEvent(_T("    LButton over ")+ann->GetName()+_T("\r\n"));
							ann->ToggleGrayOut();
							Invalidate();
						}
					}
				}
			}
			break;
		default:
			break;
		}
	}
	dragSource = 0;
	CScrollZoomView::OnLButtonDown(nFlags, ppoint);
}

void CGMEView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnLButtonDblClk in ") + path + name + _T("\r\n"));
	CPoint ppoint = point;
	if (GetDocument()->GetEditMode() == GME_EDIT_MODE) {
		CoordinateTransfer(point);	// DPtoLP

		CGMEView* self = const_cast<CGMEView*> (this);
		CGuiObject*	object	= self ? self->FindObject(point, true, true) : 0;
		if (object == NULL)	// not label of the object but can be some port label inside the object
			object = self ? self->FindObject(point, true, false) : 0;
		CGuiAnnotator *annotation = FindAnnotation(point);

		if (object || annotation) {
			CComPtr<IMgaElementDecorator> newDecorator;
			if (object != NULL) {
				CGuiAspect* pAspect = object->GetCurrentAspect();
				if (pAspect != NULL) {
					CComQIPtr<IMgaElementDecorator> newDecorator2(pAspect->GetDecorator());
					newDecorator = newDecorator2;
				}
			} else {
				annotation = FindAnnotation(point);
				if (annotation)
					newDecorator = annotation->GetDecorator(currentAspect->index);
			}

			if (newDecorator) {
				CClientDC transformDC(this);
				OnPrepareDC(&transformDC);
				HRESULT retVal = newDecorator->MouseLeftButtonDoubleClick(nFlags, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
				if (retVal == S_DECORATOR_EVENT_HANDLED) {
					if (IsInOpenedDecoratorTransaction()) {
						if (ShouldCommitOperation()) {
							try {
								CommitTransaction();
							} catch (hresult_exception& e) {
								// GME-292: the commit may fail
								AbortTransaction(e.hr);
								if (e.hr != E_MGA_CONSTRAINT_VIOLATION) {
									CGMEEventLogger::LogGMEEvent(_T("    Couldn't commit transaction.\r\n"));
									AfxMessageBox(_T("Couldn't commit transaction."),MB_ICONSTOP | MB_OK);
								}
							}
							SetShouldCommitOperation(false);
							SetObjectInDecoratorOperation(NULL);
							SetAnnotatorInDecoratorOperation(NULL);
							SetInOpenedDecoratorTransaction(false);
							SetIsContextInitiatedOperation(false);
							SetInElementDecoratorOperation(false);
							CScrollZoomView::OnLButtonDblClk(nFlags, ppoint);
							return;
						} else if (!IsInElementDecoratorOperation()) {
							AbortTransaction(S_OK);
							SetInOpenedDecoratorTransaction(false);
							SetIsContextInitiatedOperation(false);
							SetInElementDecoratorOperation(false);
							CScrollZoomView::OnLButtonDblClk(nFlags, ppoint);
							return;
						}
						// Note: DecoratorLib in-place edit currently not use SetCapture,
						// but ReleaseCapture can be handy if some decorator issues SetCapture
						if (::GetCapture() != NULL)
							::ReleaseCapture();
						SetInElementDecoratorOperation(false);
						SetInOpenedDecoratorTransaction(false);
						SetIsContextInitiatedOperation(false);
					} else {
						if (IsInElementDecoratorOperation()) {
							if (IsDecoratorOrAnnotator())
								SetObjectInDecoratorOperation(object);
							else
								SetAnnotatorInDecoratorOperation(annotation);
						}
						CScrollZoomView::OnLButtonDown(nFlags, ppoint);
						return;
					}
				} else if (retVal != S_OK &&
						   retVal != S_DECORATOR_EVENT_NOT_HANDLED &&
						   retVal != E_DECORATOR_NOT_IMPLEMENTED)
				{
					CancelDecoratorOperation();
					// FIXME: how to handle this error?
					// COMTHROW(retVal);
					return;
				}
				if (IsInElementDecoratorOperation()) {
					if (IsDecoratorOrAnnotator())
						SetObjectInDecoratorOperation(object);
					else
						SetAnnotatorInDecoratorOperation(annotation);
				}
			}
		}
		if (IsInElementDecoratorOperation()) {
			CScrollZoomView::OnLButtonDown(nFlags, ppoint);
			return;
		}

		CGuiObject *selection = FindObject(point);

		if(selection) {
			CGMEEventLogger::LogGMEEvent(    _T("LButton double clicked on ")+selection->GetName()+_T(" ")+selection->GetID()+_T("\r\n"));
			CString aspectName = currentAspect->name;
			CComPtr<IMgaFCO> mgaFco = selection->mgaFco;
			CComPtr<IMgaModel> model;
			CComPtr<IMgaReference> ref;
			CComPtr<IMgaFCO> referred;
			CComBSTR         referred_id;
			if(FAILED(mgaFco.QueryInterface(&model))) {		// it is not a model
				if(SUCCEEDED(mgaFco.QueryInterface(&ref))) {		// it is a reference
					CComPtr<IMgaObject> obj;
					try {
						BeginTransaction(TRANSACTION_READ_ONLY);
						COMTHROW(ref->get_Referred(&referred));
						if(referred)
						{
							COMTHROW(referred->GetParent(&obj, NULL));
							COMTHROW(referred->get_ID( &referred_id));
						}
						CommitTransaction();
					}
					catch(hresult_exception e) {
						AbortTransaction(e.hr);
						CScrollZoomView::OnLButtonDblClk(nFlags, ppoint);
					}
					if(referred) {
						if(FAILED(obj.QueryInterface(&model))) {		// it is a root
							if (SUCCEEDED(referred.QueryInterface(&model))) {
								int aspi = selection->MapAspect(selection->GetParentAspect());
								if(aspi >= 0) {
									try {
										BeginTransaction(TRANSACTION_READ_ONLY);
										ASSERT(selection != NULL);
										CGuiCompoundReference* compref = selection->dynamic_cast_CGuiCompoundReference();
										VERIFY(compref);
										CComPtr<IMgaMetaFCO> metaFco;
										COMTHROW(compref->GetTerminalReferee()->get_Meta(&metaFco));
										metaref_type metaRef;
										VERIFY(metaFco);
										COMTHROW(metaFco->get_MetaRef(&metaRef));
										CGuiMetaModel *guiMetaModel = CGuiMetaProject::theInstance->GetGuiMetaModel(metaRef);
										VERIFY(guiMetaModel);
										CGuiMetaAspect *asp = guiMetaModel->FindAspect(aspi);
										if(asp) {
											aspectName = asp->name;
										}
										CommitTransaction();
									}
									catch (hresult_exception &e) {
										AbortTransaction(e.hr);
									}
								}
							}
//							AfxMessageBox(_T("Referenced model is a root model. Opening model.")); // instead show target selected
							else // obj is not a model, referred is also not a model
							{
								CGMEConsole::theInstance->Message( _T("Reference target is child of a folder, thus it is shown in the TreeBrowser only."), MSG_INFO);
								CGMEBrowser::theInstance->FocusItem( referred_id);//CComBSTR( mgaObjectId));//FireLocateMgaObject( (LPCTSTR) (CString) referred_id);
								//CGMEBrowser::theInstance->FocusItem( CComBSTR( (LPCTSTR) CString( referred_id)));
								CScrollZoomView::OnLButtonDblClk(nFlags, ppoint);
								return;
							}
						} else {
							try {
								BeginTransaction(TRANSACTION_READ_ONLY);

								// Get the first aspect of the referenced element's parent model
								CComPtr<IMgaMetaFCO> spMetaFCO;
								COMTHROW(model->get_Meta(&spMetaFCO));
								CComQIPtr<IMgaMetaModel> spMetaModel = spMetaFCO;

								CComPtr<IMgaMetaAspects> spAspects;
								COMTHROW(spMetaModel->get_Aspects(&spAspects));
								ASSERT(spAspects);
								long nAspects = 0;
								COMTHROW(spAspects->get_Count(&nAspects));
								CComPtr<IMgaMetaAspect> spAspect;
								if (nAspects > 0) {
									COMTHROW(spAspects->get_Item(1, &spAspect));
								}
								CComBSTR nm;
								COMTHROW(spAspect->get_Name(&nm));
								aspectName = nm;

								CommitTransaction();
							}
							catch (hresult_exception &e) {
								AbortTransaction(e.hr);
							}
						}
					}
					else
					{
						AfxMessageBox(_T("Unable to show referred object of null reference."));
						CGMEEventLogger::LogGMEEvent(_T("    Unable to show referred object of null reference.\r\n"));
					}
				}
			}
			else {
				int aspi = selection->MapAspect(selection->GetParentAspect());
				if(aspi >= 0) {
					ASSERT(selection != NULL);
					CGuiCompound* comp = selection->dynamic_cast_CGuiCompound();
					VERIFY(comp);
					CGuiMetaModel* guiMetaModel = dynamic_cast<CGuiMetaModel*>(comp->guiMeta);
					VERIFY(guiMetaModel);
					CGuiMetaAspect* asp = guiMetaModel->FindAspect(aspi);
					if(asp)
						aspectName = asp->name;
				}
			}
			auto guiport = selection->FindPort(point);
			CComPtr<IMgaFCO> port;
			if (guiport)
				port = guiport->mgaFco; // n.b. get the MgaFCO here, or double-clicking on refport fails (guiport is deleted?)
			if(model != 0) {
				ShowModel(model, aspectName);
#if !defined (ACTIVEXGMEVIEW)
				CGMEView *view = CGMEDoc::theInstance->FindView(model);
				if (view)
				{
					if (port)
						view->SetCenterObject(port); // this may change the aspect. (does it matter?)
					else
						view->SetCenterObject(referred);
				}
#endif
			}
		}
		else if (annotation) {
			CGMEEventLogger::LogGMEEvent(    _T("LButton double clicked on ")+annotation->GetName()+_T("\r\n"));
			CComPtr<IMgaFCO> fcoToShow;
			currentModel.QueryInterface(&fcoToShow);
			ShowAnnotationBrowser(fcoToShow, annotation->rootNode);
		}
		else {
//#ifdef _DEBUG
/*			// Auto Router stress test
			struct _timeb measuerementStartTime;
			_ftime(&measuerementStartTime);
			CString structSizeStr;
			structSizeStr.Format(_T("sizeof(CAutoRouterEdge) = %ld\n"), sizeof(CAutoRouterEdge));
			OutputDebugString(structSizeStr);
			for (long i = 0; i < 10; i++)
				AutoRoute();
			struct _timeb measuerementEndTime;
			_ftime(&measuerementEndTime);
			unsigned long elapsedSeconds = (unsigned long)(measuerementEndTime.time - measuerementStartTime.time);
			long elapsedMilliSeconds = ((long)measuerementEndTime.millitm - measuerementStartTime.millitm);
			CString elapsedTimeStr;
			elapsedTimeStr.Format(_T("Ellapsed: %lu s + %d ms\n"), elapsedSeconds, elapsedMilliSeconds);
			OutputDebugString(elapsedTimeStr);
			Reset();*/
			// XML dump test
//			HRESULT hr = DumpModelGeometryXML(_T("C:\\XMLDump.xml"));
//#else
			OnViewParent();	// double click on model background brings up the parent model
							// user requested standard behavior

//#endif
		}
	}
	CScrollZoomView::OnLButtonDblClk(nFlags, ppoint);
}

void CGMEView::OnRButtonDown(UINT nFlags, CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnRButtonDown in ")+path+name+_T("\r\n"));

	CPoint trackPoint = point;
	CPoint ppoint = point;
	CoordinateTransfer(point);

	if(!tmpConnectMode && !isLibObject) {
		CGMEDoc *doc = GetDocument();
		if(doc->GetEditMode() == GME_EDIT_MODE)
		{
			CGuiObject* selection = FindObject(point);
			POSITION alreadySelected = 0;
			if (selection) {
				CGMEEventLogger::LogGMEEvent(_T("    RButton over ")+selection->GetName()+_T(" ")+selection->GetID()+_T("\r\n")); 
				ClearConnectionSelection();
				RemoveAllAnnotationFromSelection();
				alreadySelected = selected.Find(selection);
				if(!alreadySelected)
				{
					if(!(nFlags & MK_CONTROL)) {
						this->SendUnselEvent4List( &selected);
						selected.RemoveAll();
					}
					this->SendSelecEvent4Object( selection);
					selected.AddHead(selection);
				}
				inDrag = true;
				CGuiObject::GetExtent(selected,dragRect);
				CPoint ptClickOffset(point.x - dragRect.left,
										point.y - dragRect.top);
				CRect rectAwake = CRect(trackPoint.x,trackPoint.y,trackPoint.x + 1,trackPoint.y + 1);
				rectAwake.InflateRect(GetSystemMetrics(SM_CXDRAG), GetSystemMetrics(SM_CYDRAG));

				ClientToScreen(&dragRect);
				ClientToScreen(&rectAwake);

				CRectList rects,annRects;
				CGuiObject::GetRectList(selected,rects);
				CGuiAnnotator::GetRectList(selectedAnnotations,annRects);
				CGMEDataDescriptor desc(rects,annRects,point,ptClickOffset);
				CGMEDataDescriptor::destructList( rects);
				CGMEDataDescriptor::destructList( annRects);

				dragSource = (selected.GetCount() > 0) ? selected.GetHead() : NULL;
				validGuiObjects = true;
				DROPEFFECT dropEffect = CGMEDoc::DoDragDrop(&selected, &selectedAnnotations, &desc,
						DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK, &rectAwake, this);
				if (validGuiObjects && dropEffect == DROPEFFECT_NONE) {
					OnRButtonUp(nFlags, trackPoint);
				}
				inDrag = false;
			}
		}
	}

	CScrollZoomView::OnRButtonDown(nFlags, ppoint);
}

void CGMEView::OnRButtonUp(UINT nFlags, CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnRButtonUp in ")+path+name+_T("\r\n"));
	CPoint local = point;
	CPoint ppoint = point;
	CoordinateTransfer(local);	// DPtoLP
	contextMenuLocation = local;

	CGMEDoc* doc = GetDocument();
	contextPort = 0;
	contextSelection = FindObject(local);
	if (contextSelection == NULL && doc->GetEditMode() == GME_EDIT_MODE)
		contextSelection = FindObject(local, false, true);
	CGuiObject* cgobj = NULL;
	if (contextSelection != NULL)
		cgobj = contextSelection->dynamic_cast_CGuiObject();
	if (cgobj) {
		CGuiPort *port = cgobj->FindPort( local);
		if (port && port->IsRealPort()) {
			contextPort = port;
		}
	}

	if (contextSelection == 0)
		contextSelection = router.FindConnection(local);
	if (!contextSelection) {
		contextAnnotation = FindAnnotation(local);
	}
	else {
		contextAnnotation = NULL;
	}
	if (contextSelection)
		CGMEEventLogger::LogGMEEvent(_T("    RButton over ")+contextSelection->GetName()+_T(" ")+contextSelection->GetID()+_T("\r\n"));
	else if (contextAnnotation)
		CGMEEventLogger::LogGMEEvent(_T("    RButton over ")+contextAnnotation->GetName()+_T("\r\n"));

	switch(doc->GetEditMode()) {
	case GME_SET_MODE:
		{
			CGMEEventLogger::LogGMEEvent(_T("    mode=GME_SET_MODE\r\n"));
			selected.RemoveAll();
			RemoveAllAnnotationFromSelection();
			ClearConnectionSelection();
			CGuiSet* set = NULL;
			if (contextSelection)
				set = contextSelection->dynamic_cast_CGuiSet();
			if(set) {
				if(set == currentSet)
					currentSet = 0;
				else {
					currentSet = set;
					selected.AddHead(set);
					ChangeAttrPrefFco(set);
				}
			}
			else
				currentSet = 0;
			if(currentSet) {
				CGuiAnnotator::GrayOutAnnotations(annotators,true);
				CGuiFco::GrayOutFcos(children,true);
				CGuiFco::GrayOutFcos(connections,true);
				currentSet->GrayOutMembers(false);
				currentSet->GrayOut(false);
			}
			else {
				CGuiAnnotator::GrayOutAnnotations(annotators,false);
				CGuiFco::GrayOutFcos(children,false);
				CGuiFco::GrayOutFcos(connections,false);
			}
			Invalidate();
			DoPannWinRefresh();
		}
		break;
	case GME_ZOOM_MODE:
		{
			CGMEEventLogger::LogGMEEvent(_T("    mode=GME_ZOOM_MODE\r\n"));
			ZoomOut(ppoint);
			Invalidate();
		}
		break;
	case GME_VISUAL_MODE:
		{
			CGMEEventLogger::LogGMEEvent(_T("    mode=GME_VISUAL_MODE\r\n"));
			CGuiObject* obj = NULL;
			if (contextSelection)
				obj = contextSelection->dynamic_cast_CGuiObject();
			if(obj) {
				obj->ToggleGrayOut();
				obj->GrayOutNeighbors();
				CGuiFco::GrayOutNonInternalConnections(connections);
				Invalidate();
			}
			else {
				CGuiConnection* conn = NULL;
				if (contextSelection)
					conn = contextSelection->dynamic_cast_CGuiConnection();
				if (conn) {
					conn->ToggleGrayOut();
					conn->GrayOutEndPoints();
					CGuiFco::GrayOutNonInternalConnections(connections);
					Invalidate();
				}
			}
			if (contextAnnotation) {
				contextAnnotation->ToggleGrayOut();
				Invalidate();
			}
		}
		break;
	case GME_EDIT_MODE:
		{
			CGMEEventLogger::LogGMEEvent(_T("    mode=GME_EDIT_MODE\r\n"));
			CPoint global = point;
			ClientToScreen(&global);

			// new selection logic 8/2/00
			CGuiObject* selection = FindObject(local);
			if (selection == NULL)	// select with label
				selection = FindObject(local, false, true);

			{
				CGuiAnnotator *annotation = selection ? NULL : FindAnnotation(local);

				POSITION alreadySelected = 0;
				if(selection != 0) {
					alreadySelected = selected.Find(selection);
					if(!(nFlags & MK_CONTROL)) {
						this->SendUnselEvent4List( &selected);
						selected.RemoveAll();
						RemoveAllAnnotationFromSelection();
						ClearConnectionSelection();
					}
					else if(alreadySelected) {
						this->SendUnselEvent4Object( selected.GetAt( alreadySelected));
						selected.RemoveAt(alreadySelected);
					}
					this->SendSelecEvent4Object( selection);
					selected.AddHead(selection);
					ChangeAttrPrefObjs(selected);
				} else {
					if (annotation != 0) {
						alreadySelected = selectedAnnotations.Find(annotation);
						if (!(nFlags & MK_CONTROL)) {
							this->SendUnselEvent4List( &selected);
							selected.RemoveAll();
							RemoveAllAnnotationFromSelection();
						} else if (alreadySelected) {
							RemoveAnnotationFromSelection(alreadySelected);
						}
						AddAnnotationToSelectionHead(annotation);
						// ANNTODO: ChangeAttrPref....
					}
				}

				Invalidate();
			}

			if (contextPort != NULL) {
				CString itemname = CString( _T("[")) + (contextSelection?(contextSelection->GetInfoText() + CString(_T(" : "))): CString(_T(""))) + contextPort->GetInfoText() + CString( _T("]"));

				CMenu menu;
				menu.LoadMenu(IDR_PORTCONTEXT_MENU);

				CMenu *sm = menu.GetSubMenu(0); ASSERT( sm);
				if( sm) {
					sm->InsertMenu( 0, MF_BYPOSITION|MF_SEPARATOR); 
					sm->InsertMenu( 0, MF_BYPOSITION|MFS_DEFAULT, ID_CNTX_SHOWPORTINPARENT, itemname);
					sm->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, global.x,global.y,GetParent());
				}
			} else if (contextSelection != NULL) {
				selectedContextConnection = contextSelection->dynamic_cast_CGuiConnection();
				if (selectedContextConnection) {
					bool isPartFixed = false;
					contextConnectionEdgeIndex = selectedContextConnection->IsPathAt(local,
						contextConnectionPartMoveMethod, customizeHorizontalOrVerticalEdge, isPartFixed);
					contextConnectionCustomizationType = selectedContextConnection->IsAutoRouted() ? SimpleEdgeDisplacement : CustomPointCustomization;
				}

				HMENU decoratorAdditionalMenu = ::CreatePopupMenu();
				if (selection != NULL) {
					CGuiAspect* pAspect = selection->GetCurrentAspect();
					if (pAspect != NULL) {
						CComQIPtr<IMgaElementDecorator> newDecorator(pAspect->GetDecorator());
						HRESULT retVal = S_OK;
						if (newDecorator) {
							CClientDC transformDC(this);
							OnPrepareDC(&transformDC);
							retVal = newDecorator->MouseRightButtonDown((ULONGLONG)decoratorAdditionalMenu, nFlags, local.x, local.y, (ULONGLONG)transformDC.m_hDC);
						}
					}
				}
				CMenu menu;
				menu.LoadMenu(selectedContextConnection != NULL ? IDR_CONNCONTEXT_MENU : IDR_CONTEXT_MENU);
				CMenu* subMenu = menu.GetSubMenu(0);
				if (::GetMenuItemCount(decoratorAdditionalMenu) > 0) {
					subMenu->InsertMenu(0, MF_BYPOSITION | MF_SEPARATOR, 0, _T(""));
					subMenu->InsertMenu(0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)(decoratorAdditionalMenu), _T("Decorator Edit"));
				}
				UINT cmdId = (UINT)subMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
															global.x,global.y,GetParent());
				// Save state for possible later use, see OnCmdMsg
				ctxClkSt.nFlags = nFlags;
				ctxClkSt.lpoint = local;
				ctxClkSt.dpoint = ppoint;
				selectedObjectOfContext = selection;
				selectedAnnotationOfContext = NULL;
				::DestroyMenu(decoratorAdditionalMenu);
			} else if (contextAnnotation != NULL) {
				HMENU decoratorAdditionalMenu = ::CreatePopupMenu();
				CComPtr<IMgaElementDecorator> decorator = contextAnnotation->GetDecorator(currentAspect->index);
				HRESULT retVal = S_OK;
				if (decorator) {
					CClientDC transformDC(this);
					OnPrepareDC(&transformDC);
					retVal = decorator->MouseRightButtonDown((ULONGLONG)decoratorAdditionalMenu, nFlags, local.x, local.y, (ULONGLONG)transformDC.m_hDC);
				}
				CMenu menu;
				menu.LoadMenu(IDR_ANNCONTEXT_MENU);
				CMenu* subMenu = menu.GetSubMenu(0);
				if (::GetMenuItemCount(decoratorAdditionalMenu) > 0) {
					subMenu->InsertMenu(0, MF_BYPOSITION | MF_SEPARATOR, 0, _T(""));
					subMenu->InsertMenu(0, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)(decoratorAdditionalMenu), _T("Decorator Edit"));
				}
				UINT cmdId = (UINT)subMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
															global.x,global.y,GetParent());
				// Save state for possible later use, see OnCmdMsg
				ctxClkSt.nFlags = nFlags;
				ctxClkSt.lpoint = local;
				ctxClkSt.dpoint = ppoint;
				selectedAnnotationOfContext = contextAnnotation;
				selectedObjectOfContext = NULL;
				::DestroyMenu(decoratorAdditionalMenu);
			} else {
				CMenu menu;
				menu.LoadMenu(IDR_SELFCONTEXT_MENU);
				CMenu *submenu = menu.GetSubMenu(0);
				currentAspect->InitContextMenu(submenu);
#if defined(ADDCRASHTESTMENU)
				CMenu crashTestMenu;
				crashTestMenu.LoadMenu(IDR_CRASH_TEST_MENU);
				submenu->AppendMenu(MF_POPUP, (UINT_PTR)((HMENU)crashTestMenu), _T("Debug"));
#endif
				submenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
												global.x,global.y,GetParent());
				currentAspect->ResetContextMenu(submenu);
			}
		}
		break;

	case GME_AUTOCONNECT_MODE:
	case GME_SHORTAUTOCONNECT_MODE:
		CGMEEventLogger::LogGMEEvent(_T("    mode=GME_AUTOCONNECT_MODE\r\n"));
	case GME_DISCONNECT_MODE:
	case GME_SHORTDISCONNECT_MODE:
		{
			CGMEEventLogger::LogGMEEvent(_T("    mode=GME_DISCONNECT_MODE\r\n"));
			CPoint global = point;
			ClientToScreen(&global);
			if (contextSelection) {
				CMenu menu;
				menu.LoadMenu(contextSelection->dynamic_cast_CGuiConnection() ? IDR_CONNCONTEXT_MENU : IDR_CONTEXT_MENU);
				menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
													global.x,global.y,GetParent());
			}
			else if (contextAnnotation) {
				CMenu menu;
				menu.LoadMenu(IDR_ANNCONTEXT_MENU);
				menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
													global.x,global.y,GetParent());
			}
			else {
				CMenu menu;
				menu.LoadMenu(IDR_SELFCONTEXT_MENU);
				CMenu *submenu = menu.GetSubMenu(0);
				currentAspect->InitContextMenu(submenu);
				submenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
												global.x,global.y,GetParent());
				currentAspect->ResetContextMenu(submenu);
			}
		}
		break;
	}
	CScrollZoomView::OnRButtonUp(nFlags, ppoint);
	this->SendNow();
}

void CGMEView::ZoomToPoint(int scale, int newScale, CPoint point)
{
	if (scale != newScale)
	{
		int yRes = CClientDC(this).GetDeviceCaps(LOGPIXELSY);
		// goal: zoom in and move scrollbars such that the mouse is over the same spot in the grid (logical coords)
		//  (sometimes this cannot be acheived due to scrollbar limits, especially at low zooms)
		CPoint scp = GetScrollPosition(); // upper corner of scrolling (logical coords)

		CPoint scaledPoint = point;
		//  scaledPoint /= scale / 100.0f;
		//  scaledPoint /= yRes / 96
		scaledPoint.x = ::MulDiv(scaledPoint.x, 100 * 96, scale * yRes);
		scaledPoint.y = ::MulDiv(scaledPoint.y, 100 * 96, scale * yRes);

		// now scp + scaledPoint == mouse coords (logical). Translate using new zoom back to mouse coords
		//  scaledPoint -= point / (zoom * (scale / 100.0f));
		//  scaledPoint -= point / (yRes / 96);
		scaledPoint.x -= ::MulDiv(point.x, 100 * 96, newScale * yRes);
		scaledPoint.y -= ::MulDiv(point.y, 100 * 96, newScale * yRes);

		SetRedraw(FALSE); // Reduce flicker: OnZoom sets scrollbar position, and Windows redraws it before we can ScrollToPosition
		OnZoom(0, newScale);
		ScrollToPosition(scp + scaledPoint);

		frame->propBar.SetZoomVal(m_zoomVal);
		CMainFrame::theInstance->WriteStatusZoom(m_zoomVal);

		SetRedraw(TRUE);
		Invalidate();
	}

}


BOOL CGMEView::OnMouseWheel(UINT fFlags, short zDelta, CPoint point)
{
	// handle zoom in/out
	if (fFlags & MK_CONTROL) {
		UINT uWheelScrollLines = GetMouseScrollLines();
		int nToScroll = ::MulDiv(zDelta, uWheelScrollLines, WHEEL_DELTA);

		int scale = m_zoomVal;
		float zoom = pow((float)1 + (float)abs(nToScroll)/8, nToScroll > 0 ? 1 : -1);
		int newScale = scale * zoom + 0.49999997f;
		newScale = max(ZOOM_MIN, min(ZOOM_MAX, newScale));
		if (abs((float)newScale - 100) < 8) // TODO: implement fixup for more values?
		{
			newScale = 100;
			zoom = (float) newScale / scale;
		}
		this->ScreenToClient(&point);
		ZoomToPoint(scale, newScale, point);
		return TRUE;
	}

	return CScrollZoomView::OnMouseWheel(fFlags, zDelta, point);
}

void CGMEView::OnAppCommand(CWnd* pWnd, UINT nCmd, UINT nDevice, UINT nKey)
{
	bool handled = true;
	switch (nCmd) {
		case APPCOMMAND_BROWSER_BACKWARD:
			{
				CGMEDoc* pDoc = GetDocument();
				ASSERT_VALID(pDoc);
				pDoc->back();
			}
			break;
		case APPCOMMAND_BROWSER_FORWARD:
			{
				CGMEDoc* pDoc = GetDocument();
				ASSERT_VALID(pDoc);
				pDoc->forw();
			}
			break;
		case APPCOMMAND_BROWSER_HOME:
			{
				CGMEDoc* pDoc = GetDocument();
				ASSERT_VALID(pDoc);
				pDoc->home();
			}
			break;
		case APPCOMMAND_BROWSER_REFRESH:
			{
				CGMEDoc* pDoc = GetDocument();
				ASSERT_VALID(pDoc);
				pDoc->DoOnViewRefresh();
			}
			break;
		case APPCOMMAND_BROWSER_SEARCH:
			{
				if (CMainFrame::theInstance != NULL)
					CMainFrame::theInstance->ShowFindDlg();
			}
			break;
		default:
			handled = false;
			break;
	}
	if (!handled)
		CScrollZoomView::OnAppCommand(pWnd, nCmd, nDevice, nKey);
}

DROPEFFECT CGMEView::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnDragEnter in ")+path+name+_T("\r\n"));
	ASSERT(prevDropEffect == DROPEFFECT_NONE);

	m_dropRightClick = GetKeyState(VK_RBUTTON) & 0x8000;

	if(isType && CGMEDataSource::IsGmeNativeDataAvailable(pDataObject,theApp.mgaProject)) {
//	if(pDataObject->IsDataAvailable(CGMEDataSource::cfGMEDesc)) {

		if(dragDesc.Load(pDataObject)) {
			dragOffset = dragDesc.offset;
			dragPoint = point - CSize(1,1);
			return OnDragOver(pDataObject,dwKeyState,point);
		}
	}
	else if( isType && CGMEDataSource::IsXMLDataAvailable(pDataObject) )
		return DROPEFFECT_COPY;

	CoordinateTransfer(point);
	DROPEFFECT dropEffect = DROPEFFECT_NONE;
	HRESULT retVal = S_OK;
	CGuiObject* selection = FindObject(point);
	if (selection != NULL) {
		CGuiAspect* pAspect = selection->GetCurrentAspect();
		if (pAspect != NULL) {
			CComQIPtr<IMgaElementDecorator> newDecorator(pAspect->GetDecorator());
			if (newDecorator) {
				CClientDC transformDC(this);
				OnPrepareDC(&transformDC);
				retVal = newDecorator->DragEnter(&dropEffect, (ULONGLONG)pDataObject, dwKeyState, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
			}
		}
	}
	if (dropEffect != DROPEFFECT_NONE) {
		dragPoint = point;
		return dropEffect;
	}

	CGuiAnnotator* annotation = FindAnnotation(point);
	if (annotation != NULL) {
		CComPtr<IMgaElementDecorator> decorator = annotation->GetDecorator(currentAspect->index);
		if (decorator) {
			CClientDC transformDC(this);
			OnPrepareDC(&transformDC);
			retVal = decorator->DragEnter(&dropEffect, (ULONGLONG)pDataObject, dwKeyState, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
		}
	}
	if (dropEffect != DROPEFFECT_NONE) {
		dragPoint = point;
		return dropEffect;
	}

	return DROPEFFECT_NONE;
}

void CGMEView::OnDragLeave()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnDragLeave from ")+path+name+_T("\r\n"));
 	CClientDC dc(this);
	OnPrepareDC(&dc);
	if(prevDropEffect != DROPEFFECT_NONE) {
		dragDesc.Draw(&dc,dragPoint);
		prevDropEffect = DROPEFFECT_NONE;
	}
	dragDesc.Clean();
}

DROPEFFECT CGMEView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnDragOver in ")+path+name+_T("\r\n"));

	//this event happens too much, logfile size could explode...
	if(!CGMEDataSource::IsGmeNativeDataAvailable(pDataObject,theApp.mgaProject))
//	if(!pDataObject->IsDataAvailable(CGMEDataSource::cfGMEDesc))
	{
		if(isType && CGMEDataSource::IsXMLDataAvailable(pDataObject))
			return DROPEFFECT_COPY;

		return DROPEFFECT_NONE;
	}

	CoordinateTransfer(point);
	CGuiObject *obj = FindObject(point);
	HRESULT retVal = S_OK;
	DROPEFFECT dropEffect = DROPEFFECT_NONE;
	if(obj) {
		CGMEEventLogger::LogGMEEvent(_T("    Dragging over: ")+obj->GetName()+_T(" ")+obj->GetID()+_T(" in ")+path+name+_T("\r\n"));//better this way, not logging dragging over empty space

		CGuiAspect* pAspect = obj->GetCurrentAspect();
		if (pAspect != NULL) {
			CComQIPtr<IMgaElementDecorator> newDecorator(pAspect->GetDecorator());
			if (newDecorator) {
				CClientDC transformDC(this);
				OnPrepareDC(&transformDC);
				retVal = newDecorator->DragOver(&dropEffect, (ULONGLONG)pDataObject, dwKeyState, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
			}
		}
		if (dropEffect != DROPEFFECT_NONE)
			return dropEffect;
	}

	CGuiAnnotator* annotation = FindAnnotation(point);
	if (annotation != NULL) {
		CComPtr<IMgaElementDecorator> decorator = annotation->GetDecorator(currentAspect->index);
		if (decorator) {
			CClientDC transformDC(this);
			OnPrepareDC(&transformDC);
			retVal = decorator->DragOver(&dropEffect, (ULONGLONG)pDataObject, dwKeyState, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
		}
	}
	if (dropEffect != DROPEFFECT_NONE)
		return dropEffect;

	CGuiObject* ref = NULL;
	if (obj)
		ref = obj->dynamic_cast_CGuiReference();
	if (obj && !ref)
		ref = obj->dynamic_cast_CGuiCompoundReference();

	if (ref && obj != dragSource && dragDesc.GetCount() <= 1)
		return DROPEFFECT_LINK;
	else if(!isType)
		return DROPEFFECT_NONE;

	DROPEFFECT de = DROPEFFECT_MOVE;
	if((dwKeyState & MK_CONTROL) != 0)
		de = (((dwKeyState & MK_SHIFT) == 0) ? DROPEFFECT_COPY : DROPEFFECT_LINK);

	if((dwKeyState & MK_ALT) != 0) {
		de = DROPEFFECT_COPY | DROPEFFECT_LINK;
		derivedDrop = true;
		instanceDrop = ((dwKeyState & MK_SHIFT) == 0);
	}
	else {
		derivedDrop = false;
		instanceDrop = false;
	}

	if(point == dragPoint)
		return de;

	CClientDC dc(this);
	OnPrepareDC(&dc);
	if(prevDropEffect != DROPEFFECT_NONE) {
		dragDesc.Draw(&dc,dragPoint);
	}
	prevDropEffect = de;
	if(prevDropEffect != DROPEFFECT_NONE) {
		dragPoint = point;
		dragDesc.Draw(&dc,point);
	}
	return de;
}

BOOL CGMEView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnDrop in ")+path+name+_T("\r\n"));
	if(dropEffect & DROPEFFECT_MOVE)
		CGMEEventLogger::LogGMEEvent(_T("    DROPEFFECT_MOVE\r\n"));
	if(dropEffect & DROPEFFECT_LINK)
		CGMEEventLogger::LogGMEEvent(_T("    DROPEFFECT_LINK\r\n"));
	if(dropEffect & DROPEFFECT_COPY)
		CGMEEventLogger::LogGMEEvent(_T("    DROPEFFECT_COPY\r\n"));
	if(dropEffect == DROPEFFECT_NONE) //DROPEFFECT_NONE==0
		CGMEEventLogger::LogGMEEvent(_T("    DROPEFFECT_NONE\r\n"));
	ASSERT_VALID(this);
	CGMEDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	CSize size;
	CPoint orig = point;
	CPoint testPoint = point;

	CPoint screen = point;
	ClientToScreen(&screen);

	CoordinateTransfer(point);
	CoordinateTransfer(testPoint);

	point.x = (long)(point.x - dragOffset.x);
	point.y = (long)(point.y - dragOffset.y);

	OnDragLeave();
	if (m_dropRightClick)
	{
		bool canMove = true;
		bool canRef = true;
		if (CGMEDataSource::IsGmeNativeDataAvailable(pDataObject, theApp.mgaProject))
		{
			canMove = false;
			canRef = false;
			try {
				BeginTransaction(TRANSACTION_READ_ONLY);

				CComPtr<IDataObject> p = pDataObject->GetIDataObject(FALSE);
				CComPtr<IMgaDataSource> pt;
				COMTHROW(p.QueryInterface(&pt));

				CGMEDoc *doc = GetDocument();

				CComPtr<IUnknown> unk;
				COMTHROW(pt->get_Data(&unk));
				IMgaFCOsPtr fcos = unk.p;

				if (fcos)
				{
					IMgaMetaModelPtr meta = currentModel->Meta;
					auto roles = meta->Roles;

					for (int i = 1; i <= fcos->Count; i++)
					{
						for (int j = 1; j <= roles->Count; j++)
						{
							if (fcos->Item[i]->Meta == roles->Item[j]->Kind)
							{
								canMove = true;
							}
							IMgaMetaReferencePtr ref = roles->Item[j]->Kind;
							if (ref)
							{
								_bstr_t descs;
								IMgaMetaPointerItemsPtr specs = ref->RefSpec->Items;
								for (int k = 1; k <= specs->Count; k++)
								{
									if (wcscmp(fcos->Item[i]->Meta->Name, specs->Item[k]->Desc) == 0)
									{
										canRef = true;
									}
								}
							}
						}
					}
				}
			}
			catch (...) {
				CommitTransaction();
				throw;
			}
			CommitTransaction();
		}

		CMenu menu;
		if (menu.CreatePopupMenu())
		{
			enum actions { MOVE= 1000, COPY, REF, INSTANCE, SUBTYPE, CANCEL };
			UINT moveFlag = MF_STRING | (canMove ? 0u : MF_GRAYED);
			menu.AppendMenu(moveFlag, MOVE, L"Move");
			menu.AppendMenu(moveFlag, COPY, L"Copy");
			menu.AppendMenu(MF_STRING | (canRef ? 0u : MF_GRAYED), REF, L"Create reference");
			menu.AppendMenu(moveFlag, SUBTYPE, L"Create subtype");
			menu.AppendMenu(moveFlag, INSTANCE, L"Create instance");
			menu.AppendMenu(MF_STRING, CANCEL, L"Cancel");

			UINT nItemID = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, screen.x, screen.y, this);

			switch (nItemID)
			{
			case MOVE:
				DoPasteItem(pDataObject, true, true, false, false, false, false, false, 0, point);
				break;
			case COPY:
				DoPasteItem(pDataObject, true, false, false, false, false, false, false, 0, point);
				break;
			case REF:
				DoPasteItem(pDataObject, true, false, true, false, false, false, false, 0, point);
				break;
			case SUBTYPE:
				DoPasteItem(pDataObject, true, false, false, true, false, false, false, 0, point);
				break;
			case INSTANCE:
				DoPasteItem(pDataObject, true, false, false, true, true, false, false, 0, point);
				break;
			}
		}
		return TRUE;
	}

	if(isType) {
		if ((dropEffect & DROPEFFECT_MOVE) && inDrag)
		{
			ASSERT((selected.GetCount() + selectedAnnotations.GetCount()) > 0);
			CGMEEventLogger::LogGMEEvent(_T("    Dropping:\r\n"));
			GMEEVENTLOG_GUIOBJS(selected);
			GMEEVENTLOG_GUIANNOTATORS(selectedAnnotations);
			Invalidate();

			try {
				BeginTransaction();
				int left = 0, top = 0, leftA = 0, topA = 0;
				bool valid = false, validA = false;
				if (selected.GetCount() > 0) {
					CGuiObject::FindUpperLeft(selected, left, top);
					valid = true;
				}
				if (selectedAnnotations.GetCount() > 0) {
					CGuiAnnotator::FindUpperLeft(selectedAnnotations, leftA, topA);
					validA = true;
				}
				if (valid && validA) {
					left = min(left, leftA);
					top = min(top, topA);
				}
				else if (validA) {
					left = leftA;
					top = topA;
				}
				else if (!valid) {
					ASSERT(("There is no object to move",false));
				}
				CPoint diff = point - CPoint(left,top);
				CGuiObject::ShiftModels(selected, diff);
				CGuiAnnotator::ShiftAnnotations(selectedAnnotations,diff);
				// moving a port inside this may make the decorator for this in the parent bigger
				// so we reset the model grid for the parent
				ResetParent(true);
				__CommitTransaction();
			}
			catch(hresult_exception e) {                
				AbortTransaction(e.hr);
				CGMEEventLogger::LogGMEEvent(_T("    Unable to complete drop operation.\r\n"));
				AfxMessageBox(_T("Unable to complete drop operation"),MB_ICONSTOP | MB_OK);
				Reset(true);
				return FALSE;
			}
			catch(_com_error& e) {                
				AbortTransaction(e.Error());
				CString error = _T("Unable to complete drop operation");
				if (e.Description().length() != 0)
				{
					error += _T(": ");
					error += static_cast<const TCHAR*>(e.Description());
				}
				CGMEEventLogger::LogGMEEvent(error + _T("\r\n"));
				AfxMessageBox(error,MB_ICONSTOP | MB_OK);
				Reset(true);
				return FALSE;
			}
			AutoRoute(); // HACK reroute the whole thing for now!
			Invalidate(true);
			inDrag = false;
			return TRUE;
		}
	}
	CGuiObject *target = FindObject(testPoint);
	HRESULT retVal = S_OK;
	if (target != NULL) {
		CGuiAspect* pAspect = target->GetCurrentAspect();
		if (pAspect != NULL) {
			CComQIPtr<IMgaElementDecorator> newDecorator(pAspect->GetDecorator());
			if (newDecorator) {
				CClientDC transformDC(this);
				OnPrepareDC(&transformDC);
				retVal = newDecorator->Drop((ULONGLONG)pDataObject, dropEffect, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
			}
		}
	}
	if (retVal == S_DECORATOR_EVENT_HANDLED)
		return TRUE;

	CGuiAnnotator* annotation = FindAnnotation(point);
	if (annotation != NULL) {
		CComPtr<IMgaElementDecorator> decorator = annotation->GetDecorator(currentAspect->index);
		if (decorator) {
			CClientDC transformDC(this);
			OnPrepareDC(&transformDC);
			retVal = decorator->Drop((ULONGLONG)pDataObject, dropEffect, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
		}
	}
	if (retVal == S_DECORATOR_EVENT_HANDLED)
		return TRUE;

	CGuiReference* guiRef = NULL;
	CGuiCompoundReference* compRef = NULL;
	if (target != NULL) {
		guiRef = target->dynamic_cast_CGuiReference();
		compRef = target->dynamic_cast_CGuiCompoundReference();
	}
	bool sameTarget = (target == dragSource);
	if(isType || guiRef || compRef) {
		DoPasteItem(pDataObject,
					true,
					((dropEffect & DROPEFFECT_MOVE) != 0) && !(guiRef || compRef),
					(dropEffect & DROPEFFECT_LINK) != 0 || guiRef || compRef,
					derivedDrop,
					instanceDrop,
					false,
					false,
					((guiRef || compRef) && !sameTarget) ? target : 0,
					point);
		derivedDrop = instanceDrop = false;
//		GetDocument()->InvalidateAllViews(true);
		return TRUE;
	}
	CGMEEventLogger::LogGMEEvent(_T("    Nothing Dropped\r\n"));
	return FALSE;
}

void CGMEView::OnViewParent()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnViewParent from ")+path+name+_T("\r\n"));
	ShowModel(parent);
}

void CGMEView::OnUpdateViewParent(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(parent != 0);
}

void CGMEView::ShowGrid(bool show)
{
	drawGrid = show;
	Invalidate();
}

void CGMEView::OnViewGrid()
{
	drawGrid = !drawGrid;
	Invalidate();
}

void CGMEView::OnUpdateViewGrid(CCmdUI* pCmdUI)
{
//	pCmdUI->Enable(zoomIdx >= GME_ZOOM_LEVEL_MED);
	pCmdUI->Enable(m_zoomVal >= ZOOM_NO);
	pCmdUI->SetCheck(drawGrid);
}

void CGMEView::OnEditNudgedown()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditNudgedown in ")+path+name+_T("\r\n"));

	try {
		BeginTransaction();
		CGuiAnnotator::NudgeAnnotations(selectedAnnotations, 0, 1);
		if(isType && CGuiObject::NudgeObjects(selected, 0, 1))
			router.NudgeObjects(selected, 0, 1, currentAspect->index);
		Invalidate();

		CommitTransaction();
	}
	catch(hresult_exception e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to nudge objects"),MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to nudge objects.\r\n"));
		return;
	}
	ResetParent();
//  Workaround fixes sticky objects
	Invalidate(true);
}

void CGMEView::OnEditNudgeleft()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditNudgeleft in ")+path+name+_T("\r\n"));

	try {
		BeginTransaction();
		CGuiAnnotator::NudgeAnnotations(selectedAnnotations, -1, 0);
		if(isType && CGuiObject::NudgeObjects(selected, -1, 0))
			router.NudgeObjects(selected, -1, 0, currentAspect->index);
		Invalidate();
		CommitTransaction();
	}
	catch(hresult_exception e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to nudge objects"),MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to nudge objects.\r\n"));
		return;
	}
	ResetParent();
//  Workaround fixes sticky objects
	Invalidate(true);
}

void CGMEView::OnEditNudgeright()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditNudgeright in ")+path+name+_T("\r\n"));

	try {
		BeginTransaction();
		CGuiAnnotator::NudgeAnnotations(selectedAnnotations, 1, 0);
		if(isType && CGuiObject::NudgeObjects(selected, 1, 0))
			router.NudgeObjects(selected, 1, 0, currentAspect->index);
		Invalidate();
		CommitTransaction();
	}
	catch(hresult_exception e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to nudge objects"),MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to nudge objects.\r\n"));
		return;
	}
	ResetParent();
//  Workaround fixes sticky objects
	Invalidate(true);
}

void CGMEView::OnEditNudgeup()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditNudgeup in ")+path+name+_T("\r\n"));

	try {
		BeginTransaction();
		CGuiAnnotator::NudgeAnnotations(selectedAnnotations, 0, -1);
		if(isType && CGuiObject::NudgeObjects(selected, 0, -1))
			router.NudgeObjects(selected, 0, -1, currentAspect->index);
		Invalidate();
		CommitTransaction();
	}
	catch(hresult_exception e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to nudge objects"),MB_ICONSTOP | MB_OK);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to nudge objects.\r\n"));
		return;
	}
	ResetParent();
//  Workaround fixes sticky objects
	Invalidate(true);
}

BOOL CGMEView::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if (nID == ID_EDIT_CANCEL) {	// capture ESC for decorator or connection customization operation cancel
		if (IsInElementDecoratorOperation())
			CancelDecoratorOperation();
		if (isInConnectionCustomizeOperation) {
			isInConnectionCustomizeOperation = false;
			::SetCursor(customizeConnectionCursorBackup);
			isCursorChangedByEdgeCustomize = false;
		}
		if (tmpConnectMode) {
			tmpConnectMode = false;
			ClearConnSpecs();
			SetCursor(editCursor);
			ShowCursor(TRUE);
			Invalidate();
		}
	}

	if(CGuiMetaProject::theInstance->CmdIDInRange(nID))	{
		if(nCode == CN_COMMAND && pHandlerInfo == NULL) {
			CString label;
			switch (CGuiMetaProject::theInstance->CmdType(nID, label)) {
			case GME_CMD_CONTEXT:
				if (isType && currentAspect->FindCommand(nID, label))
					InsertNewPart(label, contextMenuLocation);
				break;
			default:
				ASSERT(FALSE);
				break;
			}
			return TRUE;
		}
		else if(nCode == CN_UPDATE_COMMAND_UI && pExtra != NULL) {
			CCmdUI *pUI = (CCmdUI *)pExtra;
			pUI->Enable(isType);
			return TRUE;
		}
	} else if (nID >= DECORATOR_CTX_MENU_MINID && nID < DECORATOR_CTX_MENU_MAXID && CGuiMetaProject::theInstance->maxMenuCmdID < DECORATOR_CTX_MENU_MINID) {
		if (nCode == CN_UPDATE_COMMAND_UI && pExtra != NULL) {
			CCmdUI* pUI = (CCmdUI*) pExtra;
			pUI->Enable(isType);
			return TRUE;
		} else if (nCode == CN_COMMAND) {
			if (selectedObjectOfContext != NULL || selectedAnnotationOfContext != NULL) {
				// Send command using saved state
				CComPtr<IMgaElementDecorator> newDecorator;
				if (selectedObjectOfContext != NULL) {
					CGuiAspect* pAspect = selectedObjectOfContext->GetCurrentAspect();
					if (pAspect != NULL) {
						CComQIPtr<IMgaElementDecorator> newDecorator2(pAspect->GetDecorator());
						newDecorator = newDecorator2;
					}
				} else {
					newDecorator = selectedAnnotationOfContext->GetDecorator(currentAspect->index);
				}
				if (newDecorator) {
					SetIsContextInitiatedOperation(true);
					if (selectedObjectOfContext != NULL)
						SetObjectInDecoratorOperation(selectedObjectOfContext);
					else
						SetAnnotatorInDecoratorOperation(selectedAnnotationOfContext);
					CClientDC transformDC(this);
					OnPrepareDC(&transformDC);
					HRESULT retVal = newDecorator->MenuItemSelected(nID, ctxClkSt.nFlags, ctxClkSt.lpoint.x, ctxClkSt.lpoint.y,
																	(ULONGLONG)transformDC.m_hDC);
					if (retVal == S_DECORATOR_EVENT_HANDLED) {
						if (IsInOpenedDecoratorTransaction()) {
							if (ShouldCommitOperation()) {
								try {
									CommitTransaction();
								} catch (hresult_exception& e) {
									// GME-292: the commit may fail
									AbortTransaction(e.hr);
									if (e.hr != E_MGA_CONSTRAINT_VIOLATION) {
										CGMEEventLogger::LogGMEEvent(_T("    Couldn't commit transaction.\r\n"));
										AfxMessageBox(_T("Couldn't commit transaction."),MB_ICONSTOP | MB_OK);
									}
								}
								SetShouldCommitOperation(false);
								SetObjectInDecoratorOperation(NULL);
								SetAnnotatorInDecoratorOperation(NULL);
							} else {
								AbortTransaction(S_OK);
							}
							SetInOpenedDecoratorTransaction(false);
							SetIsContextInitiatedOperation(false);
							SetInElementDecoratorOperation(false);
						}
					} else if (retVal != S_OK &&
								retVal != S_DECORATOR_EVENT_NOT_HANDLED &&
								retVal != E_DECORATOR_NOT_IMPLEMENTED)
					{
						CancelDecoratorOperation();
						// FIXME: how to handle this error?
						// COMTHROW(retVal);
						return TRUE;
					}
				}
			}
		}
	}
	return CScrollZoomView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CGMEView::OnEditDelete()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditDelete in ")+path+name+_T("\r\n"));

	if (selectedConnection && selected.IsEmpty() && selectedAnnotations.IsEmpty()) {
		bool ok = DeleteConnection(selectedConnection);
		// user already got a MessageBox, no need for another
	} else {
		GMEEVENTLOG_GUIANNOTATORS(selectedAnnotations);
		DeleteAnnotations(selectedAnnotations);
		RemoveAllAnnotationFromSelection();
		ClearConnectionSelection();

		GMEEVENTLOG_GUIOBJS(selected);
		this->SendUnselEvent4List( &selected);
		if(DeleteObjects( selected))
			selected.RemoveAll();
	}
}

void CGMEView::OnUpdateEditDelete(CCmdUI* pCmdUI)
{
	if( !selected.IsEmpty())
		pCmdUI->Enable(true);
	else
		pCmdUI->Enable(!selectedAnnotations.IsEmpty() || selectedConnection);
}

void CGMEView::OnContextProperties()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnContextProperties in ")+path+name+_T("\r\n"));
    if (contextSelection) {
		CGuiObject* guiObj = contextSelection->dynamic_cast_CGuiObject();
		if (guiObj)
			ShowProperties(guiObj);
		else {
			CGuiConnection* guiConn = contextSelection->dynamic_cast_CGuiConnection();
			if (guiConn)
				ShowProperties(guiConn);
		}
	}
	else
		ShowProperties();
	contextSelection = 0;
	contextPort = 0;
}

void CGMEView::AttributepanelPage(long page)
{
	switch (page)
	{
	case 0:
		ShowAttributes();
		break;
	case 1:
		ShowPreferences();
		break;
	case 2:
		ShowProperties();
		break;
	}
}

void CGMEView::OnCntxPreferences()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxPreferences in ")+path+name+_T("\r\n"));
	if (contextSelection) {
		CGuiObject* guiObj = contextSelection->dynamic_cast_CGuiObject();
		if(guiObj)
			ShowPreferences(guiObj);
		else {
			CGuiConnection* guiConn = contextSelection->dynamic_cast_CGuiConnection();
			if (guiConn)
				ShowPreferences(guiConn);
		}
	}
	else {
		ShowPreferences();
	}

	contextSelection = 0;
	contextAnnotation = 0;
	contextPort = 0;
}

void CGMEView::OnCntxDisconnectall()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxDisconnectall in ")+path+name+_T("\r\n"));
	if (!isType)
		return;
	if (contextSelection) {
		CGuiObject* guiObj = contextSelection->dynamic_cast_CGuiObject();
		if (guiObj && guiObj->IsVisible()) {
			try {
				BeginTransaction();
				POSITION pos = guiObj->GetPorts().GetHeadPosition();
				while(pos) {
						DisconnectAll(guiObj,guiObj->GetPorts().GetNext(pos));
				}
				contextSelection = 0;
				contextPort = 0;
				__CommitTransaction();
			}
			catch(hresult_exception e) {
				AbortTransaction(e.hr);
				AfxMessageBox(_T("Could not complete disconnect operation"),MB_OK | MB_ICONSTOP);
				CGMEEventLogger::LogGMEEvent(_T("    Could not complete disconnect operation.\r\n"));
			}
			catch(_com_error& e) {                
				AbortTransaction(e.Error());
				CString error = _T("Could not complete disconnect operation");
				if (e.Description().length() != 0)
				{
					error += _T(": ");
					error += static_cast<const TCHAR*>(e.Description());
				}
				CGMEEventLogger::LogGMEEvent(error + _T("\r\n"));
				AfxMessageBox(error,MB_ICONSTOP | MB_OK);
			}
		}
	}
}

void CGMEView::OnUpdateCntxDisconnectall(CCmdUI* pCmdUI)
{
	// HACK
	pCmdUI->Enable(isType);
}

void CGMEView::OnCntxAttributes()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxAttributes in ")+path+name+_T("\r\n"));
	if (contextSelection) {
		CGuiObject* guiObj = contextSelection->dynamic_cast_CGuiObject();
		if (guiObj)
			ShowAttributes(guiObj);
		else {
			CGuiConnection* guiConn = contextSelection->dynamic_cast_CGuiConnection();
			if (guiConn)
				ShowAttributes(guiConn);
		}
	}
	else
		ShowAttributes();
	contextSelection = 0;
	contextPort = 0;
}

void CGMEView::OnUpdateCntxAttributes(CCmdUI* pCmdUI)
{
	bool enable = false;
	CGuiMetaAttributeList *metaAttrs = 0;
	try {
		BeginTransaction(TRANSACTION_READ_ONLY);
		metaAttrs = contextSelection ? contextSelection->GetMetaAttributes() : &currentAspect->attrs;
		if(metaAttrs->GetCount() > 0)
			enable = true;
		CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to get model attributes"),MB_OK | MB_ICONSTOP);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to get model attributes.\r\n"));
	}
	pCmdUI->Enable(enable);
}

void CGMEView::OnEditUndo() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditUndo\r\n"));
	if (IsInElementDecoratorOperation())
		return;
	theApp.mgaProject->Undo();
	// Don't lose TreeBrowser focus
	//this->SetFocus();
}

void CGMEView::OnEditRedo() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditRedo\r\n"));
	if (IsInElementDecoratorOperation())
		return;
	theApp.mgaProject->Redo();
	// Don't lose TreeBrowser focus
	//this->SetFocus();
}

void CGMEView::OnEditCopy()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditCopy in ")+path+name+_T("\r\n"));
	if(selected.GetCount() + selectedAnnotations.GetCount() > 0) {
		GMEEVENTLOG_GUIOBJS(selected);
		GMEEVENTLOG_GUIANNOTATORS(selectedAnnotations);
		CPoint pt = CPoint(0,0);
		CRectList rects, annRects;
		CGuiObject::GetRectList(selected,rects);
		CGuiAnnotator::GetRectList(selectedAnnotations,annRects);
		CGMEDataDescriptor desc(rects,annRects,pt,pt);
		CGMEDataDescriptor::destructList( rects);
		CGMEDataDescriptor::destructList( annRects);
		GetDocument()->CopyToClipboard(&selected,&selectedAnnotations,&desc,this);
	}
}

void CGMEView::OnEditCopyClosure()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditCopyClosure in ")+path+name+_T("\r\n"));
	if(selected.GetCount() > 0) {
		GMEEVENTLOG_GUIOBJS(selected);
		GMEEVENTLOG_GUIANNOTATORS(selectedAnnotations);
		CPoint pt = CPoint(0,0);
		CRectList rects, annRects;
		CGuiObject::GetRectList(selected,rects);
		CGuiAnnotator::GetRectList(selectedAnnotations,annRects);
		CGMEDataDescriptor desc(rects,annRects,pt,pt);
		CGMEDataDescriptor::destructList( rects);
		CGMEDataDescriptor::destructList( annRects);
		GetDocument()->CopyClosureToClipboard( &selected, &selectedAnnotations, &desc, this);
	}
}

void CGMEView::OnEditCopySmart()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditCopySmart in ")+path+name+_T("\r\n"));
	if(selected.GetCount() > 0) {
		GMEEVENTLOG_GUIOBJS(selected);
		GMEEVENTLOG_GUIANNOTATORS(selectedAnnotations);
		CPoint pt = CPoint(0,0);
		CRectList rects, annRects;
		CGuiObject::GetRectList(selected,rects);
		CGuiAnnotator::GetRectList(selectedAnnotations,annRects);
		CGMEDataDescriptor desc(rects,annRects,pt,pt);
		CGMEDataDescriptor::destructList( rects);
		CGMEDataDescriptor::destructList( annRects);
		CGuiFcoList fcoList;// copy selected->selectedFco
		POSITION pos = selected.GetHeadPosition();
		while( pos != NULL ) fcoList.AddTail( selected.GetNext( pos ));

		GetDocument()->CopySmartToClipboard( &fcoList, &selectedAnnotations, &desc, this);
	}
}

void CGMEView::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(selected.GetCount() + selectedAnnotations.GetCount() > 0);
}

void CGMEView::OnUpdateEditCopyClosure(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(selected.GetCount() > 0);
}

void CGMEView::OnUpdateEditCopySmart(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(selected.GetCount() > 0);
}

void CGMEView::OnEditCut()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditCut in ")+path+name+_T("\r\n"));
	if(selected.GetCount() + selectedAnnotations.GetCount() > 0) {
		if(isType) GMEEVENTLOG_GUIOBJS(selected);
		GMEEVENTLOG_GUIANNOTATORS(selectedAnnotations);
		CPoint pt = CPoint(0,0);
		CRectList rects,annRects;
		if(isType) CGuiObject::GetRectList(selected,rects);
		CGuiAnnotator::GetRectList(selectedAnnotations,annRects);
		CGMEDataDescriptor desc(rects,annRects,pt,pt);
		CGMEDataDescriptor::destructList( rects);
		CGMEDataDescriptor::destructList( annRects);
		GetDocument()->CopyToClipboard(&selected,&selectedAnnotations,&desc,this);
		if(isType) DeleteObjects(selected);
		DeleteAnnotations(selectedAnnotations);
		if(isType) selected.RemoveAll();
		RemoveAllAnnotationFromSelection();
		ClearConnectionSelection();
	}
}

void CGMEView::OnUpdateEditCut(CCmdUI* pCmdUI)
{
	if( !selectedAnnotations.IsEmpty() && selected.IsEmpty())
		pCmdUI->Enable( TRUE); // allow if annotations are selected only
	else
		pCmdUI->Enable(isType && selected.GetCount() + selectedAnnotations.GetCount() > 0);
}

void CGMEView::OnEditPaste()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditPaste in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = false;
		DoPasteItem(&clipboardData);
	}
}

void CGMEView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
	COleDataObject dataObj;
	BOOL bEnable = isType && dataObj.AttachClipboard() &&
		((CGMEDataSource::IsGmeNativeDataAvailable(&dataObj,theApp.mgaProject)) ||
		(CGMEDataSource::IsXMLDataAvailable(&dataObj))
		);

	pCmdUI->Enable(bEnable);
}

void CGMEView::OnCntxCopy()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxCopy in ")+path+name+_T("\r\n"));
	if (contextSelection) {
		CGuiObject* guiObj = contextSelection->dynamic_cast_CGuiObject();
		if(guiObj) {
			CGMEEventLogger::LogGMEEvent(_T("    ")+guiObj->GetName()+_T(" ")+guiObj->GetID()+_T("\r\n"));
			CGuiObjectList list;
			CGuiAnnotatorList dummyList;
			list.AddTail(guiObj);
			CPoint pt = CPoint(0,0);
			CRectList rects;
			CGuiObject::GetRectList(list,rects);
			CRectList dummyAnnList;
			CGMEDataDescriptor desc(rects,dummyAnnList,pt,pt);
			CGMEDataDescriptor::destructList( rects);
			GetDocument()->CopyToClipboard(&list,&dummyList,&desc,this);
		}
		contextSelection = 0;
		contextPort = 0;
	}
	else if (contextAnnotation) {
		CGMEEventLogger::LogGMEEvent(_T("    ")+contextAnnotation->GetName()+_T("/r/n"));
		CGuiObjectList dummyList;
		CGuiAnnotatorList list;
		list.AddTail(contextAnnotation);
		CPoint pt = CPoint(0,0);
		CRectList rects;
		CGuiAnnotator::GetRectList(list,rects);
		CRectList dummyObjList;
		CGMEDataDescriptor desc(dummyObjList,rects,pt,pt);
		CGMEDataDescriptor::destructList( rects);
		GetDocument()->CopyToClipboard(&dummyList,&list,&desc,this);
		contextAnnotation = 0;
	}
}

void CGMEView::OnCntxCopyClosure()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxCopyClosure in ")+path+name+_T("\r\n"));
	if (contextSelection) {
		CGuiObject* guiObj = contextSelection->dynamic_cast_CGuiObject();
		if (guiObj) {
			CGMEEventLogger::LogGMEEvent(_T("    ")+guiObj->GetName()+_T(" ")+guiObj->GetID()+_T("\r\n"));
			CGuiObjectList list;
			CGuiAnnotatorList dummyList;
			list.AddTail(guiObj);
			CPoint pt = CPoint(0,0);
			CRectList rects;
			CGuiObject::GetRectList(list,rects);
			CRectList dummyAnnList;
			CGMEDataDescriptor desc(rects,dummyAnnList,pt,pt);
			CGMEDataDescriptor::destructList( rects);
			GetDocument()->CopyClosureToClipboard( &list, &dummyList, &desc, this);
		}
		contextSelection = 0;
		contextPort = 0;
	}
	else if (contextAnnotation) {
		CGMEEventLogger::LogGMEEvent(_T("    ")+contextAnnotation->GetName()+_T("/r/n"));
		CGuiObjectList dummyList;
		CGuiAnnotatorList list;
		list.AddTail(contextAnnotation);
		CPoint pt = CPoint(0,0);
		CRectList rects;
		CGuiAnnotator::GetRectList(list,rects);
		CRectList dummyObjList;
		CGMEDataDescriptor desc(dummyObjList,rects,pt,pt);
		CGMEDataDescriptor::destructList( rects);
		GetDocument()->CopyClosureToClipboard(&dummyList,&list,&desc,this);
		contextAnnotation = 0;
	}
}

void CGMEView::OnCntxCopySmart()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxCopySmart in ")+path+name+_T("\r\n"));
	if (contextSelection) {
		CGuiObject* guiObj = contextSelection->dynamic_cast_CGuiObject();
		if (guiObj) {
			CGMEEventLogger::LogGMEEvent(_T("    ")+guiObj->GetName()+_T(" ")+guiObj->GetID()+_T("\r\n"));
			CGuiObjectList list;
			CGuiAnnotatorList dummyList;
			list.AddTail(guiObj);
			CPoint pt = CPoint(0,0);
			CRectList rects;
			CGuiObject::GetRectList(list,rects);
			CRectList dummyAnnList;
			CGMEDataDescriptor desc(rects,dummyAnnList,pt,pt);
			CGMEDataDescriptor::destructList( rects);

			CGuiFcoList fcoList;
			POSITION pos = list.GetHeadPosition();// copy list->fcoList
			while( pos != NULL ) fcoList.AddTail( list.GetNext( pos ));

			GetDocument()->CopySmartToClipboard( &fcoList, &dummyList, &desc, this);
		}
		else 
		{
			CGuiConnection* guiConn = contextSelection->dynamic_cast_CGuiConnection();
			if( guiConn) // a valid connection
			{
				CGMEEventLogger::LogGMEEvent(_T("    ")+guiConn->GetName()+_T(" ")+guiConn->GetID()+_T("\r\n"));
				CGuiFcoList list;
				CGuiAnnotatorList dummyList;
				list.AddTail(guiConn);
				CPoint pt = CPoint(0,0);
				CRectList dummyRects;
				CRectList dummyAnnList;
				CGMEDataDescriptor desc(dummyRects,dummyAnnList,pt,pt);
				GetDocument()->CopySmartToClipboard( &list, &dummyList, &desc, this);
			}
		}
		contextSelection = 0;
		contextPort = 0;
	}
	else if (contextAnnotation) {
		CGMEEventLogger::LogGMEEvent(_T("    ")+contextAnnotation->GetName()+_T("/r/n"));
		CGuiFcoList dummyList;
		CGuiAnnotatorList list;
		list.AddTail(contextAnnotation);
		CPoint pt = CPoint(0,0);
		CRectList rects;
		CGuiAnnotator::GetRectList(list,rects);
		CRectList dummyObjList;
		CGMEDataDescriptor desc(dummyObjList,rects,pt,pt);
		CGMEDataDescriptor::destructList( rects);
		GetDocument()->CopySmartToClipboard(&dummyList,&list,&desc,this);
		contextAnnotation = 0;
	}
}


void CGMEView::OnCntxCut()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxCut in ")+path+name+_T("\r\n"));
	if(isType && contextSelection) {
		CGuiObject* guiObj = contextSelection->dynamic_cast_CGuiObject();
		if(guiObj) {
			CGMEEventLogger::LogGMEEvent(_T("    ")+guiObj->GetName()+_T(" ")+guiObj->GetID()+_T("\r\n"));
			CGuiObjectList list;
			CGuiAnnotatorList dummyList;
			list.AddTail(guiObj);
			CPoint pt = CPoint(0,0);
			CRectList rects;
			CGuiObject::GetRectList(list,rects);
			CRectList dummyAnnList;
			CGMEDataDescriptor desc(rects,dummyAnnList,pt,pt);
			CGMEDataDescriptor::destructList( rects);
			GetDocument()->CopyToClipboard(&list,&dummyList,&desc,this);
			DeleteObjects(list);
		}
		contextSelection = 0;
		contextPort = 0;
	}
	else if (contextAnnotation) {
		CGMEEventLogger::LogGMEEvent(_T("    ")+contextAnnotation->GetName()+_T("/r/n"));
		CGuiObjectList dummyList;
		CGuiAnnotatorList list;
		list.AddTail(contextAnnotation);
		CPoint pt = CPoint(0,0);
		CRectList rects;
		CGuiAnnotator::GetRectList(list,rects);
		CRectList dummyObjList;
		CGMEDataDescriptor desc(dummyObjList,rects,pt,pt);
		CGMEDataDescriptor::destructList( rects);
		GetDocument()->CopyToClipboard(&dummyList,&list,&desc,this);
		DeleteAnnotations(list);
		contextAnnotation = 0;
	}
}

void CGMEView::OnUpdateCntxCut(CCmdUI* pCmdUI)
{
	if( contextAnnotation)
		pCmdUI->Enable( TRUE);
	else
		pCmdUI->Enable(isType);
}

void CGMEView::OnCntxDelete()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxDelete in ")+path+name+_T("\r\n"));
	if(isType && contextSelection) {
		CGuiObject* guiObj = contextSelection->dynamic_cast_CGuiObject();
		if(guiObj) {
			CGMEEventLogger::LogGMEEvent(_T("    ")+guiObj->GetName()+_T(" ")+guiObj->GetID()+_T("\r\n"));
			CGuiObjectList list;
			list.AddTail(guiObj);
			DeleteObjects(list);
		}
		contextSelection = 0;
		contextPort = 0;
	}
	if( contextAnnotation) {
		CGMEEventLogger::LogGMEEvent(_T("    ")+contextAnnotation->GetName()+_T("/r/n"));
		CGuiAnnotatorList list;
		list.AddTail(contextAnnotation);
		DeleteAnnotations(list);
		contextAnnotation = 0;
	}
}

void CGMEView::OnUpdateCntxDelete(CCmdUI* pCmdUI)
{
	if( contextAnnotation && !contextSelection)
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(isType);
}

void CGMEView::OnSelfcntxCopy()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnSelfcntxCopy in ")+path+name+_T("\r\n"));
	OnEditCopy();
}

void CGMEView::OnUpdateSelfcntxCopy(CCmdUI* pCmdUI)
{
	OnUpdateEditCopy(pCmdUI);
}

void CGMEView::OnSelfcntxCopyClosure()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnSelfcntxCopyClosure in ")+path+name+_T("\r\n"));
	OnEditCopyClosure();
}

void CGMEView::OnSelfcntxCopySmart()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnSelfcntxCopySmart in ")+path+name+_T("\r\n"));
	OnEditCopySmart();
}

void CGMEView::OnUpdateSelfcntxCopyClosure(CCmdUI* pCmdUI)
{
	OnUpdateEditCopyClosure(pCmdUI);
}

void CGMEView::OnUpdateSelfcntxCopySmart(CCmdUI* pCmdUI)
{
	OnUpdateEditCopySmart(pCmdUI);
}

void CGMEView::OnSelfcntxCut()
{
	OnEditCut();
}

void CGMEView::OnUpdateSelfcntxCut(CCmdUI* pCmdUI)
{
	OnUpdateEditCut(pCmdUI);
}

void CGMEView::OnSelfcntxDelete()
{
	OnEditDelete();
}

void CGMEView::OnUpdateSelfcntxDelete(CCmdUI* pCmdUI)
{
	OnUpdateEditDelete(pCmdUI);
}

void CGMEView::OnSelfcntxPaste()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnSelfcntxPaste in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = false;
		DoPasteItem(&clipboardData,true,false,false,false,false,false,false,0,contextMenuLocation);
	}
}

void CGMEView::OnUpdateSelfcntxPaste(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnActivateFrame( UINT nState, CFrameWnd* pFrameWnd )
{

//	CGMEView* gmeviewA = (CGMEView*)GetActiveView();
//	if (gmeviewA)
	if (m_isActive)
	{
		TRACE(_T("CGMEView::OnActivateFrame\n"));
	}
	CScrollZoomView::OnActivateFrame(nState, pFrameWnd);
}

void CGMEView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	CString s = bActivate ? _T("ACTIVATE "):_T("DEACTIVATE ");
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnActivateView ")+s+path+name+_T("\r\n"));
	//I tried logging pActivateView and pDeactiveView, but they always seemed to be "this"
	//anyways, OnActivateView is called on both views, so you would know if going from
	//one to another by the ACTIVATE/DEACTIVATE - Brian
	m_overlay = nullptr;

	if (bActivate && (!initDone || needsReset)) {
		if( theApp.isHistoryEnabled())
		{
			GetDocument()->tellHistorian( currentModId, currentAspect?currentAspect->name:_T(""));
		}

		modelGrid.Clear();
		FillModelGrid();
		AutoRoute();
		ClearConnSpecs();
		if(guiMeta) {
			theApp.UpdateCompList4CurrentKind( guiMeta->name);
			CMainFrame::theInstance->SetPartBrowserMetaModel(guiMeta);
			CMainFrame::theInstance->SetPartBrowserBg(bgColor);
			CMainFrame::theInstance->ChangePartBrowserAspect(currentAspect->index);
		}
		needsReset = false;
	}
	else if(tmpConnectMode) {
		tmpConnectMode = false;
		ClearConnSpecs();
	}
	//comm'd by zolmol
	//CMainFrame::theInstance->UpdateTitle(theApp.projectName);

//	CGMEView * gmeviewA = (CGMEView *)pActivateView;
//	CGMEView * gmeviewI = (CGMEView *)pDeactiveView;
	m_isActive = bActivate;
	TRACE(_T("CGMEView::OnActivateView final false\n"));
	if (bActivate)
		theApp.UpdateMainTitle();
	CScrollZoomView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


void CGMEView::OnEditCancel()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditCancel in ")+path+name+_T("\r\n"));
	if(tmpConnectMode) {
		tmpConnectMode = false;
		ClearConnSpecs();
		SetCursor(editCursor);
		ShowCursor(TRUE);
		Invalidate();
	}
	else {
	 	CGMEDoc *doc = GetDocument();
		if(doc->GetEditMode() == GME_AUTOCONNECT_MODE
		|| doc->GetEditMode() == GME_SHORTAUTOCONNECT_MODE) {
			ClearConnSpecs();
			SetCursor(autoconnectCursor);
			Invalidate();
		}
		else if(doc->GetEditMode() == GME_DISCONNECT_MODE
			|| doc->GetEditMode() == GME_SHORTDISCONNECT_MODE) {
			ClearConnSpecs();
			SetCursor(disconnectCursor);
			Invalidate();
		}
	}
}

void CGMEView::OnUpdateEditCancel(CCmdUI* pCmdUI)
{
 	CGMEDoc *doc = GetDocument();
	pCmdUI->Enable(((doc->GetEditMode() == GME_AUTOCONNECT_MODE || doc->GetEditMode() == GME_SHORTAUTOCONNECT_MODE) && connSrc) ||
					((doc->GetEditMode() == GME_DISCONNECT_MODE || doc->GetEditMode() == GME_SHORTDISCONNECT_MODE) && connSrc));

}

BOOL CGMEView::PreTranslateMessage(MSG* pMsg)
{
	ASSERT( m_hWnd != NULL && theApp.m_GMEView_hAccel != NULL && pMsg != NULL );
	if( TranslateAccelerator(m_hWnd, theApp.m_GMEView_hAccel, pMsg) )
		return TRUE;

	return CScrollZoomView::PreTranslateMessage(pMsg);
}

void CGMEView::OnFileClose()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnFileClose() in ")+path+name+_T("\r\n"));
	frame->SetSendEvent(true);
	frame->PostMessage(WM_CLOSE);
}

void CGMEView::OnFileInterpret()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnFileInterpret in ")+path+name+_T("\r\n"));
	RunComponent(_T(""));
}

void CGMEView::RunComponent(CString compname)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnRunComponent ")+compname+_T(" in ")+path+name+_T("\r\n"));
	CGMEEventLogger::LogGMEEvent(_T("    Selected FCOs:"));
	GMEEVENTLOG_GUIFCOS(selected);
	MSGTRY
	{
		IMgaLauncherPtr launcher;
		COMTHROW( launcher.CreateInstance(L"Mga.MgaLauncher") );
		if(!launcher) {
			AfxMessageBox(_T("Cannot start up component launcher"));
			CGMEEventLogger::LogGMEEvent(_T("    Cannot start up component launcher.\r\n"));
		}
		else {
			CComPtr<IMgaFCO> focus;
			CComPtr<IMgaFCOs> selfcos;
			COMTHROW(selfcos.CoCreateInstance(OLESTR("Mga.MgaFCOs")));
			COMTHROW(currentModel.QueryInterface(&focus));
			POSITION pos = selected.GetHeadPosition();
			while (pos) {
				CGuiFco *gfco = selected.GetNext(pos);
				COMTHROW(selfcos->Append(gfco->mgaFco));
			}

			if(theApp.bNoProtect) COMTHROW( launcher->put_Parameter(CComVariant(true)));
			// Disable the DCOM wait dialogs: if interpreters want them, they can do it themselves; but if they don't want them, they need to link to GME's mfc1xxu.dll
			COleMessageFilter* messageFilter = AfxOleGetMessageFilter();
			messageFilter->EnableBusyDialog(FALSE);
			messageFilter->EnableNotRespondingDialog(FALSE);
			std::shared_ptr<COleMessageFilter> busyRestore(messageFilter, [](COleMessageFilter* filter){ filter->EnableBusyDialog(TRUE); } );
			std::shared_ptr<COleMessageFilter> notRespondingRestore(messageFilter, [](COleMessageFilter* filter){ filter->EnableNotRespondingDialog(TRUE); } );
			launcher->__RunComponent(_bstr_t(compname), theApp.mgaProject, focus, selfcos, GME_MAIN_START);
		}
	}
	MSGCATCH(_T("Error while trying to run the interpreter"),;)
}

void CGMEView::SetEditCursor(void)
{
	SetCursor(editCursor);
	SetIsCursorChangedByDecorator(false);
}

void CGMEView::StartDecoratorOperation(void)
{
}

void CGMEView::EndDecoratorOperation(void)
{
}						

void CGMEView::CancelDecoratorOperation(bool notify)
{
	if (IsInElementDecoratorOperation()) {
		// Note: DecoratorLib in-place edit currently not use SetCapture,
		// but ReleaseCapture can be handy if some decorator issues SetCapture
		if (::GetCapture() != NULL)
			::ReleaseCapture();
		EndDecoratorOperation();
		SetShouldCommitOperation(false);
		if (GetOriginalRect().IsRectEmpty() == FALSE) {
			if (IsDecoratorOrAnnotator())
				GetObjectInDecoratorOperation()->ResizeObject(GetOriginalRect());
//			else
//				GetAnnotatorInDecoratorOperation()->ResizeObject(GetOriginalRect());
			Invalidate();
			SetOriginalRectEmpty();
		}
		SetInElementDecoratorOperation(false);
		if (IsCursorChangedByDecorator())
			SetEditCursor();
		if (notify) {
			CComPtr<IMgaElementDecorator> newDecorator;
			if (GetObjectInDecoratorOperation() != NULL) {
				CGuiAspect* pAspect = GetObjectInDecoratorOperation()->GetCurrentAspect();
				if (pAspect != NULL)
					newDecorator = pAspect->GetNewDecorator();
			} else if (GetAnnotatorInDecoratorOperation() != NULL) {
				newDecorator = GetAnnotatorInDecoratorOperation()->GetDecorator(currentAspect->index);
			}
			if (newDecorator)
				HRESULT retVal = newDecorator->OperationCanceled();
		}
		SetObjectInDecoratorOperation(NULL);
		SetAnnotatorInDecoratorOperation(NULL);
		SetIsContextInitiatedOperation(false);
	}
	if (IsInOpenedDecoratorTransaction()) {
		AbortTransaction(S_OK);
		SetInOpenedDecoratorTransaction(false);
	}
}


bool CGMEView::IsCursorChangedByDecorator(void) const
{
	return isCursorChangedByDecorator;
}

void CGMEView::SetIsCursorChangedByDecorator(bool isCurChanged)
{
	isCursorChangedByDecorator = isCurChanged;
}

CRect CGMEView::GetOriginalRect(void) const
{
	return originalRect;
}

void CGMEView::SetOriginalRect(const CRect& rect)
{
	originalRect = rect;
}

void CGMEView::SetOriginalRectEmpty(void)
{
	originalRect.SetRectEmpty();
}

bool CGMEView::IsInElementDecoratorOperation(void) const
{
	return inElementDecoratorOperation;
}

void CGMEView::SetInElementDecoratorOperation(bool isIn)
{
	inElementDecoratorOperation = isIn;
}

bool CGMEView::IsInOpenedDecoratorTransaction(void) const
{
	return inOpenedDecoratorTransaction;
}

void CGMEView::SetInOpenedDecoratorTransaction(bool inOpenedTr)
{
	inOpenedDecoratorTransaction = inOpenedTr;
}

bool CGMEView::IsContextInitiatedOperation(void) const
{
	return isContextInitiatedOperation;
}

void CGMEView::SetIsContextInitiatedOperation(bool isCtxInit)
{
	isContextInitiatedOperation = isCtxInit;
}

bool CGMEView::ShouldCommitOperation(void) const
{
	return shouldCommitOperation;
}

void CGMEView::SetShouldCommitOperation(bool shouldCommitOp)
{
	shouldCommitOperation = shouldCommitOp;
}

bool CGMEView::IsDecoratorOrAnnotator(void) const
{
	return decoratorOrAnnotator;
}

void CGMEView::SetDecoratorOrAnnotator(bool decorOrAnnot)
{
	decoratorOrAnnotator = decorOrAnnot;
}

CGuiObject* CGMEView::GetObjectInDecoratorOperation(void) const
{
	return objectInDecoratorOperation;
}

void CGMEView::SetObjectInDecoratorOperation(CGuiObject* obj)
{
	objectInDecoratorOperation = obj;
}

CGuiAnnotator*CGMEView::GetAnnotatorInDecoratorOperation(void) const
{
	return annotatorInDecoratorOperation;
}

void CGMEView::SetAnnotatorInDecoratorOperation(CGuiAnnotator* ann)
{
	annotatorInDecoratorOperation = ann;
}

XERCES_CPP_NAMESPACE_USE

// ---------------------------------------------------------------------------
//  This is a simple class that lets us do easy (though not terribly efficient)
//  trancoding of char* data to XMLCh data.
// ---------------------------------------------------------------------------
class XStr
{
public :
	// -----------------------------------------------------------------------
	//  Constructors and Destructor
	// -----------------------------------------------------------------------
	XStr(const char* const toTranscode)
	{
		// Call the private transcoding method
		fUnicodeForm = XMLString::transcode(toTranscode);
	}

	XStr(const wchar_t* const toTranscode)
	{
		fUnicodeForm = XMLString::replicate(toTranscode);
	}
	~XStr()
	{
		XMLString::release(&fUnicodeForm);
	}


	// -----------------------------------------------------------------------
	//  Getter methods
	// -----------------------------------------------------------------------
	const XMLCh* unicodeForm() const
	{
		return fUnicodeForm;
	}

private :
	// -----------------------------------------------------------------------
	//  Private data members
	//
	//  fUnicodeForm
	//      This is the Unicode XMLCh format of the string.
	// -----------------------------------------------------------------------
	XMLCh*   fUnicodeForm;
};

#define X(str) XStr(str).unicodeForm()


HRESULT CGMEView::DumpModelGeometryXML(LPCTSTR filePath)
{
	HRESULT hr = S_OK;
	try {
		XMLPlatformUtils::Initialize();

		DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(X("Core"));
		if (impl != NULL)
		{
			try
			{
				xercesc::DOMDocument* doc = impl->createDocument(
							0,					// root element namespace URI.
							X("model"),			// root element name
							0);					// document type object (DTD).

				DOMElement* rootElem = doc->getDocumentElement();
				rootElem->setAttribute(X("name"), X(name));
				rootElem->setAttribute(X("id"), X(currentModId));

				DOMElement* aspectsElem = doc->createElement(X("aspects"));
				rootElem->appendChild(aspectsElem);
				// save current aspect
				SaveCurrAsp();
				POSITION aspPos = guiMeta->aspects.GetHeadPosition();
				// rotate through all aspects
				while (aspPos) {
					CGuiMetaAspect* asp = guiMeta->aspects.GetNext(aspPos);
					ChangePrnAspect(asp->name);
					DOMElement* aspectElem = doc->createElement(X("aspect"));
					aspectsElem->appendChild(aspectElem);
					CString intValStr;
					intValStr.Format(_T("%ld"), asp->index);
					aspectElem->setAttribute(X("index"), X(intValStr));					
					aspectElem->setAttribute(X("name"), X(asp->name));

					// List the contained elements (objects, connections)
					DOMElement* objsElem = doc->createElement(X("objects"));
					aspectElem->appendChild(objsElem);
					DOMElement* connsElem = doc->createElement(X("connections"));
					aspectElem->appendChild(connsElem);

					POSITION pos = children.GetHeadPosition();
					while (pos) {
						CGuiFco* fco = children.GetNext(pos);
						ASSERT(fco != NULL);
						if (fco->IsVisible()) {
							CGuiObject* obj = fco->dynamic_cast_CGuiObject();
							if (obj) {
								DOMElement* objElem = doc->createElement(X("object"));
								objsElem->appendChild(objElem);
								objElem->setAttribute(X("name"), X(obj->GetName()));
								objElem->setAttribute(X("id"), X(obj->GetID()));

								CString intValStr;
								CRect loc = obj->GetLocation();
								DOMElement* locElem = doc->createElement(X("location"));
								objElem->appendChild(locElem);
								intValStr.Format(_T("%ld"), loc.left);
								locElem->setAttribute(X("left"), X(intValStr));
								intValStr.Format(_T("%ld"), loc.top);
								locElem->setAttribute(X("top"), X(intValStr));
								intValStr.Format(_T("%ld"), loc.right);
								locElem->setAttribute(X("right"), X(intValStr));
								intValStr.Format(_T("%ld"), loc.bottom);
								locElem->setAttribute(X("bottom"), X(intValStr));

								CRect nameLoc = obj->GetNameLocation();
								DOMElement* nameLocElem = doc->createElement(X("namelocation"));
								objElem->appendChild(nameLocElem);
								intValStr.Format(_T("%ld"), nameLoc.left);
								nameLocElem->setAttribute(X("left"), X(intValStr));
								intValStr.Format(_T("%ld"), nameLoc.top);
								nameLocElem->setAttribute(X("top"), X(intValStr));
								intValStr.Format(_T("%ld"), nameLoc.right);
								nameLocElem->setAttribute(X("right"), X(intValStr));
								intValStr.Format(_T("%ld"), nameLoc.bottom);
								nameLocElem->setAttribute(X("bottom"), X(intValStr));
							}
							CGuiConnection* conn = fco->dynamic_cast_CGuiConnection();
							if (conn) {
								DOMElement* connElem = doc->createElement(X("connection"));
								connsElem->appendChild(connElem);
								connElem->setAttribute(X("name"), X(conn->GetName()));
								connElem->setAttribute(X("id"), X(conn->GetID()));

								CString intValStr;
								CPointList points;
								conn->GetPointList(points);
								DOMElement* pointsElem = doc->createElement(X("points"));
								connElem->appendChild(pointsElem);

								POSITION pos = points.GetHeadPosition();
								while (pos) {
									CPoint pt = points.GetNext(pos);
									DOMElement* ptElem = doc->createElement(X("pt"));
									pointsElem->appendChild(ptElem);
									intValStr.Format(_T("%ld"), pt.x);
									ptElem->setAttribute(X("x"), X(intValStr));
									intValStr.Format(_T("%ld"), pt.y);
									ptElem->setAttribute(X("y"), X(intValStr));
								}

								DOMElement* labelsElem = doc->createElement(X("labels"));
								connElem->appendChild(labelsElem);
								for(int i = 0; i < GME_CONN_LABEL_NUM; i++)
								{
									CGuiConnectionLabelSet& labelset = conn->GetLabelSet();
									CString label = labelset.GetLabel(i);
									if (label.GetLength() > 0) {
										DOMElement* labelElem = NULL;
										switch(i) {
											case GME_CONN_SRC_LABEL1: labelElem = doc->createElement(X("srcLabel1")); break;
											case GME_CONN_SRC_LABEL2: labelElem = doc->createElement(X("srcLabel2")); break;
											case GME_CONN_DST_LABEL1: labelElem = doc->createElement(X("dstLabel1")); break;
											case GME_CONN_DST_LABEL2: labelElem = doc->createElement(X("dstLabel2")); break;
											case GME_CONN_MAIN_LABEL: labelElem = doc->createElement(X("mainLabel")); break;
										}
										labelsElem->appendChild(labelElem);
										CPoint pt = labelset.GetLocation(i);
										intValStr.Format(_T("%ld"), pt.x);
										labelElem->setAttribute(X("x"), X(intValStr));
										intValStr.Format(_T("%ld"), pt.y);
										labelElem->setAttribute(X("y"), X(intValStr));
										intValStr.Format(_T("%ld"), labelset.GetAlignment(i));
										labelElem->setAttribute(X("alignment"), X(intValStr));
										labelElem->setAttribute(X("label"), X(label));
									}
								}
							}
						}
					}
				}
				// restore Aspect
				CGuiMetaAspect* lastcurr = GetSavedAsp();
				ChangePrnAspect(lastcurr->name);
				Invalidate();

				//
				// Now serialize the DOM tree with pretty print format.
				//

				DOMLSSerializer* theSerializer = impl->createLSSerializer();
				theSerializer->setNewLine(X("\n\r"));
				theSerializer->getDomConfig()->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);

				XMLFormatTarget* myFormatTarget = new LocalFileFormatTarget(filePath);
				DOMLSOutput* theOutput = impl->createLSOutput();
				theOutput->setByteStream(myFormatTarget);
				theSerializer->write( doc, theOutput );
				
				// Free up stuff

				delete myFormatTarget;
				theSerializer->release();
				theOutput->release();

				doc->release();
			}
			catch (const OutOfMemoryException&)
			{
				XERCES_STD_QUALIFIER cerr << "OutOfMemoryException" << XERCES_STD_QUALIFIER endl;
				SetStandardOrGMEErrorInfo(E_OUTOFMEMORY);
				hr = E_OUTOFMEMORY;
			}
			catch (const DOMException& e)
			{
				XERCES_STD_QUALIFIER cerr << "DOMException code is:  " << e.code << XERCES_STD_QUALIFIER endl;
				SetErrorInfo(E_FAIL, L"DOMException");
				hr = E_FAIL;
			}
			catch (const IOException& e)
			{
				SetErrorInfo(E_FAIL, (wchar_t*)e.getMessage());
				hr = E_FAIL;
			}
			catch (...)
			{
				XERCES_STD_QUALIFIER cerr << "An error occurred creating the document" << XERCES_STD_QUALIFIER endl;
				SetStandardOrGMEErrorInfo(E_FAIL);
				hr = E_FAIL;
			}
		} // (inpl != NULL)
		else
		{
			XERCES_STD_QUALIFIER cerr << "Requested implementation is not supported" << XERCES_STD_QUALIFIER endl;
			SetStandardOrGMEErrorInfo(E_FAIL);
			hr = E_FAIL;
		}

		XMLPlatformUtils::Terminate();
	}
	catch(const XMLException& toCatch)
	{
		// FIXME does this leak XMLPlatformUtils::Terminate();
		std::basic_stringstream<wchar_t> err;
		err << L"Error during Xerces-c Initialization.\n"
			 << L"  Exception message:"
			 << (wchar_t*)toCatch.getMessage();
		SetErrorInfo(err.str().c_str());
	}

	return hr;
}

void CGMEView::SetConnectionCustomizeCursor(const CPoint& point)
{
	if (selectedConnection != NULL) {
		ConnectionPartMoveType connectionPartMoveType;
		bool horizontalOrVerticalEdge = false;
		bool isPartFixed;
		int edgeIndex = selectedConnection->IsPathAt(point, connectionPartMoveType, horizontalOrVerticalEdge, isPartFixed);
		if (edgeIndex >= 0 && !isPartFixed && !IsInstance()) {	// customization not allowed in instances
			HCURSOR wantedCursor;
			if (connectionPartMoveType == HorizontalEdgeMove) {
				wantedCursor = LoadCursor(NULL, IDC_SIZEWE);
			} else if (connectionPartMoveType == VerticalEdgeMove) {
				wantedCursor = LoadCursor(NULL, IDC_SIZENS);
			} else if (connectionPartMoveType == AdjacentEdgeMove) {
				wantedCursor = LoadCursor(NULL, IDC_SIZEALL);
			} else if (connectionPartMoveType == InsertNewCustomPoint) {
				wantedCursor = LoadCursor(NULL, IDC_HAND);
			} else if (connectionPartMoveType == ModifyExistingCustomPoint) {
				wantedCursor = LoadCursor(NULL, IDC_CROSS);
			} else {
				ASSERT(false);
			}
			HCURSOR cursorBackup = ::SetCursor(wantedCursor);
			if (!isCursorChangedByEdgeCustomize)
				customizeConnectionCursorBackup = cursorBackup;
			isCursorChangedByEdgeCustomize = true;
		} else {
			if (isCursorChangedByEdgeCustomize) {
				::SetCursor(customizeConnectionCursorBackup);
				isCursorChangedByEdgeCustomize = false;
			}
		}
	}
}

bool CGMEView::IsInstance(void) const
{
	return !isType && isSubType;
}

void CGMEView::TryToExecutePendingRequests(void)
{
	if (!inEventHandler && inTransaction > 0 && inRWTransaction) {
		while (!pendingRequests.IsEmpty()) {
			CPendingRequest* req = pendingRequests.RemoveHead();
			if( CGMEDoc::theInstance && !CGMEDoc::theInstance->m_isClosing)
				req->Execute(this);
			delete req;
		}
	}
}

void CGMEView::OnConncntxProperties()
{
	OnContextProperties(); // We now use the Launcher COM interface.
}

static bool CanReverseConnection(CString& srcKind, CString& dstKind, IMgaMetaConnectionPtr& meta)
{
	for (int i = 1; i <= meta->Joints->Count; i++)
	{
		IMgaMetaConnJointPtr joint = meta->Joints->GetItem(i);
		bool srcFound = false;
		bool dstFound = false;
		for (int j = 1; j <= joint->PointerSpecs->Count; j++)
		{
			IMgaMetaPointerSpecPtr spec = joint->PointerSpecs->GetItem(j);
			_bstr_t specName = spec->Name;
			for (int k = 1; k <= spec->Items->Count; k++)
			{
				IMgaMetaPointerItemPtr item = spec->Items->GetItem(k);
				if (wcscmp(specName.GetBSTR(), L"src") == 0)
				{
					if (wcscmp(dstKind, item->Desc) == 0)
					{
						srcFound = true;
					}
				}
				else if (wcscmp(specName.GetBSTR(), L"dst") == 0)
				{
					if (wcscmp(srcKind, item->Desc) == 0)
					{
						dstFound = true;
					}
				}
			}
		}
		if (srcFound && dstFound)
			return true;
	}
	return false;
}

void CGMEView::OnConncntxReverse()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnConncntxReverse in ")+path+name+_T("\r\n"));
	if (isType) {
		CGuiConnection* conn = NULL;
		if (contextSelection)
			conn = contextSelection->dynamic_cast_CGuiConnection();
		else
		{
			ASSERT(false); // should only get here thru connection context menu
			return;
		}
		MSGTRY
		{
			//CString srcKind = conn->srcPort ? static_cast<const TCHAR*>(conn->srcPort->metaFco->Name) : conn->src->kindName;
			//CString dstKind = conn->dstPort ? static_cast<const TCHAR*>(conn->dstPort->metaFco->Name) : conn->dst->kindName;
			//IMgaMetaConnectionPtr meta = conn->metaFco.p;
			//CanReverseConnection(srcKind, dstKind, meta) ? TRUE : FALSE;
			BeginTransaction();
			IMgaSimpleConnectionPtr connection = conn->mgaFco.p;
			IMgaFCOPtr src = connection->Src;
			IMgaFCOPtr dst = connection->Dst;
			IMgaFCOsPtr srcRefs = connection->SrcReferences;
			IMgaFCOsPtr dstRefs = connection->DstReferences;

			long oldprefs = connection->Project->Preferences;
			try {
				connection->Project->Preferences = connection->Project->Preferences | MGAPREF_IGNORECONNCHECKS;
				// n.b. need to disconnect first, as self-connection is often illegal
				connection->__SetSrc(NULL, NULL);
				connection->__SetDst(NULL, NULL);
			} catch (...) {
				connection->Project->Preferences = oldprefs;
				throw;
			}
			connection->__SetSrc(dstRefs, dst);
			connection->__SetDst(srcRefs, src);
			_bstr_t autoroutePrefKey = _bstr_t(L"autorouterPref");
			_bstr_t autoroutePref = connection->RegistryValue[autoroutePrefKey];
			if (autoroutePref.length())
			{
				wchar_t* dir = autoroutePref.GetBSTR();
				while (*dir)
				{
					if (isupper(*dir))
					{
						*dir = tolower(*dir);
					}
					else if (islower(*dir))
					{
						*dir = toupper(*dir);
					}
					dir++;
				}
				connection->RegistryValue[autoroutePrefKey] = autoroutePref;
			}
			__CommitTransaction();
		} MSGCATCH(L"Could not reverse connection direction", --inTransaction; theApp.mgaProject->AbortTransaction();)
		contextSelection = 0;
		contextPort = 0;
	}
}

void CGMEView::OnUpdateConncntxReverse(CCmdUI* pCmdUI)
{
	BOOL enable = FALSE;
	if (isType && contextSelection) {
		CGuiConnection* conn = contextSelection->dynamic_cast_CGuiConnection();
		CString srcKind = conn->srcPort ? static_cast<const TCHAR*>(conn->srcPort->metaFco->Name) : conn->src->kindName;
		CString dstKind = conn->dstPort ? static_cast<const TCHAR*>(conn->dstPort->metaFco->Name) : conn->dst->kindName;
		if (conn) {
			IMgaMetaConnectionPtr meta = conn->metaFco.p;
			enable = CanReverseConnection(srcKind, dstKind, meta) ? TRUE : FALSE;
			enable = TRUE;
		}
	}
	pCmdUI->Enable(enable);
}

void CGMEView::OnConncntxDelete()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnConncntxDelete in ")+path+name+_T("\r\n"));
	if(isType) {
		CGuiConnection* conn = NULL;
		if (contextSelection)
			conn = contextSelection->dynamic_cast_CGuiConnection();
		if(!conn || !DeleteConnection(conn))
			AfxMessageBox(_T("Connection cannot be deleted!"));
		contextSelection = 0;
		contextPort = 0;
	}
}

void CGMEView::OnUpdateConncntxDelete(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(isType);
}

void CGMEView::OnShowContextMenu() // called from Accelerators like SHIFT+F10 or Property (VK_APPS)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnShowContextMenu in ")+path+name+_T("\r\n"));

	CGMEDoc *doc = GetDocument();
	if( doc && doc->GetEditMode() == GME_EDIT_MODE)
	{
		CRect cr;// GetWindowRect( &cr);
		GetClientRect( &cr);
		ClientToScreen( &cr);
		CPoint global( cr.TopLeft() + CPoint( 10, 10));
		if( selected.GetCount() > 0)
		{
			CGuiObject* head = selected.GetHead();
			CPoint local = !head? CPoint( 0, 0): head->GetLocation().TopLeft();//GetCenter()); // overwrite the menu's topleft placement

			// reverse CoordinateTransfer
			CClientDC fernst(this);
			(const_cast<CGMEView *>(this))->OnPrepareDC(&fernst);
			fernst.LPtoDP( &local);

			global.Offset( local);
			CMenu menu;
			menu.LoadMenu( dynamic_cast< CGuiConnection *>( head)? IDR_CONNCONTEXT_MENU: IDR_CONTEXT_MENU);
			menu.GetSubMenu( 0)->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, global.x, global.y, GetParent());
		}
		else if( selectedAnnotations.GetCount() > 0)
		{
			CGuiAnnotator* head = selectedAnnotations.GetHead();
			CPoint local = !head? CPoint( 0, 0): head->GetLocation().TopLeft();

			// reverse CoordinateTransfer
			CClientDC fernst(this);
			(const_cast<CGMEView *>(this))->OnPrepareDC(&fernst);
			fernst.LPtoDP( &local);

			global.Offset( local); // overwrite the menu's topleft placement
			CMenu menu;
			menu.LoadMenu( IDR_ANNCONTEXT_MENU);
			menu.GetSubMenu( 0)->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, global.x, global.y, GetParent());
		}
		else
		{
			CMenu menu;
			menu.LoadMenu( IDR_SELFCONTEXT_MENU);
			CMenu *submenu = menu.GetSubMenu( 0);
			currentAspect->InitContextMenu( submenu);
			submenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, global.x, global.y, GetParent());
			currentAspect->ResetContextMenu( submenu);
		}
	}
}

void CGMEView::OnJumpToFirstObject()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnJumpToFirstObject in ")+path+name+_T("\r\n"));

	CGuiObject* first = FindFirstObject();

	if( first && first->mgaFco && CGMEDoc::theInstance)
	{
		CGMEDoc::theInstance->ShowObject( CComPtr<IUnknown>( first->mgaFco), TRUE);
	}
}

void CGMEView::OnJumpToNextObject()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnJumpToNextObject in ")+path+name+_T("\r\n"));

	CGuiObject* next = FindNextObject();

	if( next && next->mgaFco && CGMEDoc::theInstance)
	{
		CGMEDoc::theInstance->ShowObject( CComPtr<IUnknown>( next->mgaFco), TRUE);
	}
}

void CGMEView::OnConnCntxFollow() // 'Go to Dst' context command of a connection 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnConnCntxFollow in ")+path+name+_T("\r\n"));
	if (contextSelection) {
		CGuiConnection* conn = contextSelection->dynamic_cast_CGuiConnection();
		FollowLine( conn, false, ::GetKeyState( VK_CONTROL) < 0);
		contextSelection = 0;
		contextPort = 0;
	}
}

void CGMEView::OnConnCntxRevfollow() // 'Go to Src' context command of a connection
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnConnCntxRevfollow in ")+path+name+_T("\r\n"));
	if (contextSelection) {
		CGuiConnection* conn = contextSelection->dynamic_cast_CGuiConnection();
		FollowLine( conn, true, ::GetKeyState( VK_CONTROL) < 0);
		contextSelection = 0;
		contextPort = 0;
	}
}

void CGMEView::OnPortCntxFollowConnection() // 'Follow Connection' context command of a port
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnPortCntxFollowConnection in ")+path+name+_T("\r\n"));
	if( contextPort)
	{
		FollowLine( contextPort, false, ::GetKeyState( VK_CONTROL) < 0);
		contextSelection = 0;
		contextPort = 0;
	}
}

void CGMEView::OnCntxPortDelete()
{
	try {
		if (contextPort) {
			CGMEEventLogger::LogGMEEvent(_T("OnCntxPortDelete  ") + contextPort->GetName() + _T(" ") + contextPort->GetID() + _T("\r\n"));
			BeginTransaction();
			COMTHROW(contextPort->mgaFco->__DestroyObject());
			CommitTransaction();
		}
	}
	catch (const _com_error& e) {
		_bstr_t errorMessage = _bstr_t(L"Cannot delete port: ") + e.Description();
		AbortTransaction(e.Error());
		if (!CGMEConsole::theInstance)
			AfxMessageBox(errorMessage);
		else
			CGMEConsole::theInstance->Message(static_cast<const TCHAR *>(errorMessage), MSG_ERROR);
	}

}

void CGMEView::OnPortCntxRevfollowConnection() // 'Follow Reverse Connection' context command of a port
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxRevfollowConnection in ")+path+name+_T("\r\n"));
	if( contextPort)
	{
		FollowLine( contextPort, true, ::GetKeyState( VK_CONTROL) < 0);
		contextSelection = 0;
		contextPort = 0;
	}
}

void CGMEView::OnCntxFollowConnection() // 'Follow Connection' context command of an fco
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxFollowConnection in ")+path+name+_T("\r\n"));
	if( selected.GetCount() > 0)
		FollowLine( selected.GetHead(), false, ::GetKeyState( VK_CONTROL) < 0);
}

void CGMEView::OnCntxRevfollowConnection() // 'Follow Reverse Connection' context command of an fco
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxRevfollowConnection in ")+path+name+_T("\r\n"));
	if( selected.GetCount() > 0)
		FollowLine( selected.GetHead(), true, ::GetKeyState( VK_CONTROL) < 0);
}

void CGMEView::OnCntxPortShowInParent() // 'Show Port in Parent' context command of a PORT
{
	if( !contextPort) return;
	
	CGMEDoc::theInstance->ShowObject( CComPtr<IUnknown>( contextPort->mgaFco), TRUE);
	contextPort = 0;
}

void CGMEView::OnCntxPortLocateInBrw() // 'Locate Port in Browser' context command of a PORT
{
	if( !contextPort) return;

	CGMEBrowser::theInstance->FocusItem( CComBSTR( contextPort->id));
	contextPort = 0;
}

void CGMEView::OnJumpAlongConnection() // 'Jump Along Conn' command on the Navigation toolbar
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnJumpAlongConnection in ")+path+name+_T("\r\n"));
	if( selected.GetCount() > 0)
		FollowLine( selected.GetHead(), false, ::GetKeyState( VK_CONTROL) < 0);
}

void CGMEView::OnBackAlongConnection() // 'Jump back Along Conn' on Navigation toolbar
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnBackAlongConnection in ")+path+name+_T("\r\n"));
	if( selected.GetCount() > 0)
		FollowLine( selected.GetHead(), true, ::GetKeyState( VK_CONTROL) < 0);
}

void CGMEView::OnTryToSnapHorzVertPath()
{
	selectedContextConnection->VerticalAndHorizontalSnappingOfConnectionLineSegments(currentAspect->index, -1);
	selectedConnection->WriteCustomPathData();
}

void CGMEView::OnDeleteConnEdgeCustomData()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnDeleteConnEdgeCustomData in ")+path+name+_T("\r\n"));
	if (selectedContextConnection != NULL && contextConnectionEdgeIndex >= 0) {
		if (contextConnectionCustomizationType == SimpleEdgeDisplacement) {
			if (contextConnectionPartMoveMethod == HorizontalEdgeMove ||
				contextConnectionPartMoveMethod == AdjacentEdgeMove)
			{
				int edgeIndex = contextConnectionEdgeIndex;
				if (contextConnectionPartMoveMethod == AdjacentEdgeMove && customizeHorizontalOrVerticalEdge)
					edgeIndex++;
				DeleteCustomEdges(selectedContextConnection, SimpleEdgeDisplacement, edgeIndex, false);
			}
			if (contextConnectionPartMoveMethod == VerticalEdgeMove ||
				contextConnectionPartMoveMethod == AdjacentEdgeMove)
			{
				int edgeIndex = contextConnectionEdgeIndex;
				if (contextConnectionPartMoveMethod == AdjacentEdgeMove && !customizeHorizontalOrVerticalEdge)
					edgeIndex++;
				DeleteCustomEdges(selectedContextConnection, SimpleEdgeDisplacement, edgeIndex, true);
			}
		} else if (contextConnectionCustomizationType == CustomPointCustomization) {
			DeleteCustomEdges(selectedContextConnection, CustomPointCustomization, contextConnectionEdgeIndex);
		}
		selectedContextConnection->WriteCustomPathData();
	}
	selectedContextConnection = NULL;
}

void CGMEView::OnDeleteConnPointCustomData()
{
	OnDeleteConnEdgeCustomData();
}

void CGMEView::OnDeleteConnRouteCustomDataThisAspect()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnDeleteConnRouteCustomDataThisAspect in ")+path+name+_T("\r\n"));
	if (selectedContextConnection != NULL) {
		selectedContextConnection->DeleteAllPathCustomizationsForCurrentAspect();
		selectedContextConnection->WriteCustomPathData();
	}
	selectedContextConnection = NULL;
}

void CGMEView::OnDeleteConnRouteCustomDataAllAspects()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnDeleteConnRouteCustomDataAllAspects in ")+path+name+_T("\r\n"));
	if (selectedContextConnection != NULL) {
		selectedContextConnection->DeleteAllPathCustomizationsForAllAspects();
		selectedContextConnection->WriteCustomPathData();
	}
	selectedContextConnection = NULL;
}

void CGMEView::OnKillFocus(CWnd* pNewWnd)
{
	bool destroyOverlay = true;
	CString name;
	if (pNewWnd)
	{
		CWnd* parent = pNewWnd->GetParent();
		CWnd* wnd = pNewWnd;
		while (wnd)
		{
			if (wnd == parent)
			{
				destroyOverlay = false;
				break;
			}
			wnd = wnd->GetParent();
		}
	}
	if (destroyOverlay)
	{
		//overlay = nullptr;
	}
}

void CGMEView::HighlightConnection(CGuiConnection* connection)
{
	m_overlay = std::unique_ptr<GMEViewOverlay>(new GMEViewOverlay());
	CRect rect;
	GetWindowRect(&rect);
	if (this->GetStyle() & WS_VSCROLL)
	{
		rect.right -= GetSystemMetrics(SM_CXVSCROLL);
	}
	if (this->GetStyle() & WS_HSCROLL)
	{
		rect.bottom -= GetSystemMetrics(SM_CYHSCROLL);
	}
	rect.top++;
	rect.left++;
	rect.right--;
	rect.bottom--;
	LPCTSTR pszClassName = _T("GMEViewOverlay");
	WNDCLASS wndcls = {0};
    if (!::GetClassInfo(AfxGetInstanceHandle(), pszClassName, &wndcls))
    {
		wndcls.style = CS_VREDRAW | CS_HREDRAW;
        wndcls.lpszClassName = pszClassName;
		wndcls.lpfnWndProc = AfxWndProc;
		wndcls.hInstance = AfxGetInstanceHandle();
        VERIFY(::RegisterClass(&wndcls));
    }

	if (m_overlay->CreateEx(0 /* GMEViewOverlay::style*/, L"GMEViewOverlay", NULL, GMEViewOverlay::exstyle, rect, 0, 0, 0) == FALSE)
	{
		ASSERT(false);
		return;
	}
	
	bool selected = connection->IsSelected();
	connection->SetSelect(true);
	
	m_overlay->Init1();
	if (1) {
		OnPrepareDC(m_overlay->m_memcdc, NULL);
		CPoint point = m_overlay->m_memcdc->GetViewportOrg();
		{
			Gdiplus::Graphics gdip(*m_overlay->m_memcdc);
			//gdip.Clear(Gdiplus::Color(0));
			//gdip.SetTransform(
			gdip.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
			gdip.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
			connection->Draw(*m_overlay->m_memcdc, &gdip);
		}
		m_overlay->m_memcdc->SetViewportOrg(0, 0);
	}
	m_overlay->Init2();

	connection->SetSelect(selected);

	AfxGetApp()->OnIdle(0); // update button status immediately (ON_UPDATE_COMMAND_UI)
}

bool CGMEView::jumpToSelectedEnd( CGuiConnectionList& p_collOfConns, bool p_reverse, bool p_tryPort)
{
	int hmany = p_collOfConns.GetCount();

	if( hmany > 0)
	{
		CGuiConnection*     a_conn       = p_collOfConns.GetHead();
		CGuiFco*            a_neighbor   = a_conn? p_reverse? a_conn->src: a_conn->dst: 0;
		CGuiPort*           a_port       = a_conn? p_reverse? a_conn->srcPort: a_conn->dstPort: 0;

		if( hmany > 1)
		{
			CConnityDlg dlg( p_reverse);
			dlg.setList( p_collOfConns);
			if( IDOK != dlg.DoModal())
				return false;

			a_conn     = dlg.getSelectedC(); // owerwrite with the selected one (if > 1)
			a_neighbor = a_conn? p_reverse? a_conn->src: a_conn->dst: 0;
			a_port     = a_conn? p_reverse? a_conn->srcPort: a_conn->dstPort: 0;
		}
			
		if( a_neighbor)
		{
			if( p_tryPort && a_port && a_port->IsRealPort())
				CGMEDoc::theInstance->ShowObject( CComPtr<IUnknown>( a_port->mgaFco), TRUE);
			else
				CGMEDoc::theInstance->ShowObject( CComPtr<IUnknown>( a_neighbor->mgaFco), TRUE);
			HighlightConnection(a_conn);
			return true;
		}
	}
	return false;
}

bool CGMEView::FollowLine( CGuiConnection *p_guiConn, bool p_reverse, bool p_tryPort)
{
	if( !p_guiConn) 
		return false;

	CGuiConnectionList curr_conns;
	curr_conns.AddTail( p_guiConn);

	return jumpToSelectedEnd( curr_conns, p_reverse, p_tryPort);
}

bool CGMEView::FollowLine( CGuiPort* p_guiPort, bool p_reverse, bool p_tryPort)
{
	if( !p_guiPort)
		return false;

	CGuiConnectionList& curr_conns = p_reverse? p_guiPort->inConns: p_guiPort->outConns;
	return jumpToSelectedEnd( curr_conns, p_reverse, p_tryPort);
}

bool CGMEView::FollowLine( CGuiObject* p_guiObj, bool p_reverse, bool p_tryPort)
{
	if( !p_guiObj)
		return false;

	CGuiConnectionList curr_conns;
	p_guiObj->GetRelationsInOut( curr_conns, p_reverse);
	return jumpToSelectedEnd( curr_conns, p_reverse, p_tryPort);
}

bool areConnsForSels( CGuiObjectList& p_sels, bool p_inOrOut)
{
	CGuiConnectionList conn_list;
	if( p_sels.GetCount() == 1)
	{
		CGuiObject* head = p_sels.GetHead();
		if( head) head->GetRelationsInOut( conn_list, p_inOrOut);
	}
	return conn_list.GetCount() > 0;
}

bool areConnsForPort( CGuiPort* p_contextPort, bool p_inOrOut)
{
	return p_contextPort && (p_inOrOut? p_contextPort->inConns: p_contextPort->outConns).GetCount() > 0;
}

void CGMEView::OnUpdatePortCntxFollowConnection( CCmdUI* pCmdUI)
{
	pCmdUI->Enable( areConnsForPort( contextPort, false));
}

void CGMEView::OnUpdatePortCntxRevfollowConnection( CCmdUI* pCmdUI)
{
	pCmdUI->Enable( areConnsForPort( contextPort, true));
}

void CGMEView::OnUpdateCntxFollowConnection( CCmdUI* pCmdUI)
{
	pCmdUI->Enable( areConnsForSels( selected, false));
}

void CGMEView::OnUpdateCntxRevfollowConnection( CCmdUI* pCmdUI)
{
	pCmdUI->Enable( areConnsForSels( selected, true));
}

void CGMEView::OnUpdateJumpAlongConnection(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( areConnsForSels( selected, false));
}

void CGMEView::OnUpdateBackAlongConnection(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( areConnsForSels( selected, true));
}

void CGMEView::OnUpdateTryToSnapHorzVertPath(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(selectedContextConnection != NULL &&
				   !selectedContextConnection->IsAutoRouted());
}

void CGMEView::OnUpdateDeleteConnEdgeCustomData(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(selectedContextConnection != NULL && selectedContextConnection->IsAutoRouted() &&
				   selectedContextConnection->HasPathCustomizationForTypeAndCurrentAspect(SimpleEdgeDisplacement, contextConnectionEdgeIndex));
}

void CGMEView::OnUpdateDeleteConnPointCustomData(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(selectedContextConnection != NULL && !selectedContextConnection->IsAutoRouted() &&
				   contextConnectionPartMoveMethod == ModifyExistingCustomPoint &&
				   selectedContextConnection->HasPathCustomizationForTypeAndCurrentAspect(CustomPointCustomization, contextConnectionEdgeIndex));
}

void CGMEView::OnUpdateDeleteConnRouteCustomDataThisAspect(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(selectedContextConnection != NULL &&
				   selectedContextConnection->HasPathCustomizationForCurrentAspect());
}

void CGMEView::OnUpdateDeleteConnRouteCustomDataAllAspects(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(selectedContextConnection != NULL &&
				   selectedContextConnection->HasPathCustomization());
}

#if defined(ADDCRASHTESTMENU)
void CGMEView::OnCrashTestIllegalWrite(void)
{
	CrashTest::IllegalWrite();
}

void CGMEView::OnCrashTestIllegalRead(void)
{
	CrashTest::IllegalRead();
}

void CGMEView::OnCrashTestIllegalReadInCRuntime(void)
{
	CrashTest::IllegalReadInCRuntime();
}

void CGMEView::OnCrashTestIllegalCodeRead(void)
{
	CrashTest::IllegalCodeRead();
}

void CGMEView::OnCrashTestDivideByZero(void)
{
	CrashTest::DivideByZero();
}

void CGMEView::OnCrashTestAbort()
{
	CrashTest::Abort();
}

void CGMEView::OnCrashTestTerminate()
{
	CrashTest::Terminate();
}

void CGMEView::OnUpdateCrashTestMenu(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}

void CGMEView::OnUpdateCrashTestIllegalWrite(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}

void CGMEView::OnUpdateCrashTestIllegalRead(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}

void CGMEView::OnUpdateCrashTestIllegalReadInCRuntime(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}

void CGMEView::OnUpdateCrashTestIllegalCodeRead(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}

void CGMEView::OnUpdateCrashTestDivideByZero(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}

void CGMEView::OnUpdateCrashTestAbort(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}

void CGMEView::OnUpdateCrashTestTerminate(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}
#endif

void CGMEView::OnCntxClear()	// set refs to null, delete all members from set
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxClear in ")+path+name+_T("\r\n"));
	if (!isType)
		return;
	CGuiObject* obj = NULL;
	if (contextSelection)
		obj = contextSelection->dynamic_cast_CGuiReference();
	if (contextSelection && !obj)
		obj = contextSelection->dynamic_cast_CGuiCompoundReference();
	if (obj) {
		try {
			CGMEEventLogger::LogGMEEvent(_T("    ")+obj->GetName()+_T(" ")+obj->GetID()+_T("\r\n"));
			BeginTransaction();
			CComPtr<IMgaReference> mgaRef;
			COMTHROW(obj->mgaFco.QueryInterface(&mgaRef));
			CComPtr<IMgaFCO> nullFco;
			COMTHROW(mgaRef->put_Referred(nullFco));
			CommitTransaction();
		}
		catch(hresult_exception e) {
			AbortTransaction(e.hr);
			const TCHAR* t1 = _T("Cannot clear reference because of active connections!");
			const TCHAR* t2 = _T("Cannot clear reference.");
			if( e.hr == E_MGA_REFPORTS_USED)
			{
				if( !CGMEConsole::theInstance) AfxMessageBox( t1);
				else CGMEConsole::theInstance->Message( t1, MSG_ERROR);
			}
			else
				if( !CGMEConsole::theInstance) AfxMessageBox( t2);
				else CGMEConsole::theInstance->Message( t2, MSG_ERROR);
		}
	}
	else {
		CGuiSet* set = NULL;
		if (contextSelection)
			set = contextSelection->dynamic_cast_CGuiSet();
		if (set) {
			try {
				CGMEEventLogger::LogGMEEvent(_T("    ")+set->GetName()+_T(" ")+set->GetID()+_T("\r\n"));
				BeginTransaction();
				CComPtr<IMgaSet> mgaSet;
				COMTHROW(set->mgaFco.QueryInterface(&mgaSet));
				COMTHROW(mgaSet->RemoveAll());
				CommitTransaction();
			}
			catch(hresult_exception e) {
				AbortTransaction(e.hr);
			}
		}
	}
}

void CGMEView::OnUpdateCntxClear(CCmdUI* pCmdUI)
{
	CGuiObject* obj = NULL;
	if (contextSelection)
		obj = contextSelection->dynamic_cast_CGuiReference();
	if (contextSelection && !obj)
		obj = contextSelection->dynamic_cast_CGuiCompoundReference();
	if (contextSelection && !obj)
		obj = contextSelection->dynamic_cast_CGuiSet();
	pCmdUI->Enable(isType && obj != 0);
}

void CGMEView::OnCntxReset()	// revert to base i.e. reestablish dependency chain for refs and sets
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxReset in ")+path+name+_T("\r\n"));
	CGuiObject* obj = NULL;
	if (contextSelection)
		obj = contextSelection->dynamic_cast_CGuiReference();
	if (contextSelection && !obj)
		obj = contextSelection->dynamic_cast_CGuiCompoundReference();
	if (obj) {
		try {
			CGMEEventLogger::LogGMEEvent(_T("    ")+obj->GetName()+_T(" ")+obj->GetID()+_T("\r\n"));
			BeginTransaction();
			CComPtr<IMgaReference> mgaRef;
			COMTHROW(obj->mgaFco.QueryInterface(&mgaRef));
			COMTHROW(mgaRef->RevertToBase());
			CommitTransaction();
		}
		catch(hresult_exception e) {
			AbortTransaction(e.hr);
		}
	}
	else {
		CGuiSet* set = NULL;
		if (contextSelection)
			set = contextSelection->dynamic_cast_CGuiSet();
		if (set) {
			try {
				CGMEEventLogger::LogGMEEvent(_T("    ")+set->GetName()+_T(" ")+set->GetID()+_T("\r\n"));
				BeginTransaction();
				CComPtr<IMgaSet> mgaSet;
				COMTHROW(set->mgaFco.QueryInterface(&mgaSet));
				COMTHROW(mgaSet->RevertToBase());
				CommitTransaction();
			}
			catch(hresult_exception e) {
				AbortTransaction(e.hr);
			}
		}
	}
}

void CGMEView::OnUpdateCntxReset(CCmdUI* pCmdUI)
{
	CGuiObject* obj = NULL;
	if (contextSelection)
		obj = contextSelection->dynamic_cast_CGuiReference();
	if (contextSelection && !obj)
		obj = contextSelection->dynamic_cast_CGuiCompoundReference();
	if (contextSelection && !obj)
		obj = contextSelection->dynamic_cast_CGuiSet();
	pCmdUI->Enable(baseType != 0 && obj != 0);
}

void CGMEView::OnEditPreferences()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditPreferences in ")+path+name+_T("\r\n"));
	ShowPreferences();
}

void CGMEView::OnHelpHelp()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnHelpHelp in ")+path+name+_T("\r\n"));
	CComPtr<IMgaFCO> fco;
	POSITION pos = selected.GetHeadPosition();
	if( pos) // if any object selected
	{
		CGuiObject *obj = selected.GetAt( pos);
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnHelpHelp for selected: ")+obj->GetID()+_T("\r\n"));
		fco = obj->mgaFco;
	}
	else
		COMTHROW(currentModel.QueryInterface(&fco));

	ShowHelp(fco);
}

void CGMEView::OnCntxHelp()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxHelp in ")+path+name+_T("\r\n"));
	CComPtr<IMgaFCO> fco;
	if (contextSelection)
	{
		CGMEEventLogger::LogGMEEvent(_T("    ")+contextSelection->GetName()+_T(" ")+contextSelection->GetID()+_T("\r\n"));
		fco = contextSelection->mgaFco;
	}
	else
		currentModel.QueryInterface(&fco);

	ShowHelp(fco);

	contextSelection = 0;
	contextPort = 0;
}



void CGMEView::OnEditShowtype()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditShowtype in ")+path+name+_T("\r\n"));
	contextSelection = 0;	// just to be on the safe side
	contextPort = 0;
	CComPtr<IMgaModel> type;
	FindDerivedFrom(currentModel,type);
	ShowModel(type);
	contextSelection = 0;
}

void CGMEView::OnUpdateEditShowtype(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!isType);
}

void CGMEView::OnEditShowbasetype()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnShowbasetype in ")+path+name+_T("\r\n"));
	CComPtr<IMgaModel> type;
	FindDerivedFrom(currentModel,type);
	ShowModel(type);
	contextSelection = 0;
	contextPort = 0;
}

void CGMEView::OnUpdateEditShowbasetype(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(isSubType);
}

void CGMEView::OnCntxShowtype()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxShowtype in ")+path+name+_T("\r\n"));
	CComPtr<IMgaModel> model;
	GetModelInContext(model);
	CComPtr<IMgaModel> type;
	FindDerivedFrom(model,type);
	ShowModel(type);
	contextSelection = 0;
	contextPort = 0;
}

void CGMEView::OnUpdateCntxShowtype(CCmdUI* pCmdUI)
{
	bool type = (contextSelection ? contextSelection->IsType() : isType);
	bool model = (contextSelection ? (contextSelection->dynamic_cast_CGuiModel() != NULL) : true);
	pCmdUI->Enable(model && !type);
}

void CGMEView::OnCntxShowbasetype()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxShowbasetype in ")+path+name+_T("\r\n"));
	CComPtr<IMgaModel> model;
	GetModelInContext(model);
	CComPtr<IMgaModel> type;
	FindDerivedFrom(model,type);
	ShowModel(type);
	contextSelection = 0;
	contextPort = 0;
}

void CGMEView::OnUpdateCntxShowbasetype(CCmdUI* pCmdUI)
{
	bool ok = false;
	bool type = contextSelection ? contextSelection->IsType() : isType;
	bool model = (contextSelection ? (contextSelection->dynamic_cast_CGuiModel() != NULL) : true);
	if(type && model) {
		CComPtr<IMgaModel> model;
		GetModelInContext(model);
		CComPtr<IMgaModel> type;
		FindDerivedFrom(model,type);
		ok = (type != 0);
	}
	pCmdUI->Enable(ok);
}


void CGMEView::OnUpdateFileCheck(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(theApp.mgaConstMgr != NULL);
}

void CGMEView::OnUpdateFileCheckSelected(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(theApp.mgaConstMgr != NULL && selected.GetCount());
}

void CGMEView::OnFileCheck()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnFileCheck in ")+path+name+_T("\r\n"));
	ASSERT(theApp.mgaConstMgr);
	if (!theApp.mgaConstMgr)
		return;
	if(currentModel)
	{
		// message boxes displayed from constraint manager if in interactive mode
		theApp.mgaConstMgr->ObjectsInvokeEx(theApp.mgaProject, currentModel, NULL, NULL);
	}
	else
		AfxMessageBox(_T("No context selection for CM."));
}


void CGMEView::OnFileCheckSelected()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnFileCheckSelected in ")+path+name+_T("\r\n"));
	ASSERT(theApp.mgaConstMgr);
	if (!theApp.mgaConstMgr)
		return;
	GMEEVENTLOG_GUIOBJS(selected);
	POSITION pos = selected.GetHeadPosition();
	if(pos)
	{

#if(0)   // while we cannot add connections
		while(pos) {
			theApp.mgaConstMgr->ObjectsInvokeEx(
					theApp.mgaProject,
					selected.GetNext(pos)->mgaFco,
					NULL, NULL);
		}
#else
		CComPtr<IMgaFCOs> fcos;
		if(CGMEDoc::CreateFcoList(&selected,fcos,this)) {
			MGACOLL_ITERATE(IMgaFCO, fcos) {
				theApp.mgaConstMgr->ObjectsInvokeEx(
						theApp.mgaProject,
						MGACOLL_ITER,
						NULL, NULL);
			} MGACOLL_ITERATE_END;
		}
#endif
	}
	else
		AfxMessageBox(_T("No context selection for CM."));
}

void CGMEView::OnCntxCheck()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxCheck\r\n"));
	CGMEEventLogger::LogGMEEvent(_T("    Selected FCOs:"));
	GMEEVENTLOG_GUIFCOS(selected);

	ASSERT(theApp.mgaConstMgr);
	if (!theApp.mgaConstMgr)
		return;

	MSGTRY
	{
		CComPtr<IMgaFCO> selfco;

		if (selected.IsEmpty())
			selfco = currentModel;
		else
		{
			POSITION pos = selected.GetHeadPosition();
			selfco = selected.GetNext(pos)->mgaFco;
		}
		theApp.mgaConstMgr->ObjectsInvokeEx(theApp.mgaProject, selfco, NULL, NULL);
	}
	MSGCATCH(_T("Error while trying to check the selected or current model"),;)
}

void CGMEView::OnCntxInterpret()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxInterpret in ")+path+name+_T("\r\n"));
	CGMEEventLogger::LogGMEEvent(_T("    Selected FCOs:"));
	GMEEVENTLOG_GUIFCOS(selected);
	MSGTRY
	{
		IMgaLauncherPtr launcher;
		COMTHROW( launcher.CreateInstance(L"Mga.MgaLauncher") );
		if(!launcher) {
			AfxMessageBox(_T("Cannot start up component launcher"));
		}
		else {
			CComPtr<IMgaFCO> focus;
			CComPtr<IMgaFCOs> selfcos;
			COMTHROW(selfcos.CoCreateInstance(OLESTR("Mga.MgaFCOs")));
			COMTHROW(currentModel.QueryInterface(&focus));
			if (!selected.IsEmpty()) {
				POSITION pos = selected.GetHeadPosition();
				while (pos) {
					CGuiFco *gfco = selected.GetNext(pos);
					COMTHROW(selfcos->Append(gfco->mgaFco));
				}
			} else {
				COMTHROW(selfcos->Append(currentModel));
			}

			if(theApp.bNoProtect) COMTHROW( launcher->put_Parameter(CComVariant(true)));
			// Disable the DCOM wait dialogs: if interpreters want them, they can do it themselves; but if they don't want them, they need to link to GME's mfc1xxu.dll
			COleMessageFilter* messageFilter = AfxOleGetMessageFilter();
			messageFilter->EnableBusyDialog(FALSE);
			messageFilter->EnableNotRespondingDialog(FALSE);
			std::shared_ptr<COleMessageFilter> busyRestore(messageFilter, [](COleMessageFilter* filter){ filter->EnableBusyDialog(TRUE); } );
			std::shared_ptr<COleMessageFilter> notRespondingRestore(messageFilter, [](COleMessageFilter* filter){ filter->EnableNotRespondingDialog(TRUE); } );

			MSGTRY {
			launcher->__RunComponent(_bstr_t(), theApp.mgaProject, focus, selfcos, contextSelection ? GME_CONTEXT_START :  GME_BGCONTEXT_START);
			} MSGCATCH(L"Component execution failed",;)
		}
	}
	MSGCATCH(_T("Error while trying to run the interpreter"),;)
}

void CGMEView::OnCntxLocate()
{
	// ?? 
	// position the Object Browser to the selected or current object
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxLocate\r\n"));
	CGMEEventLogger::LogGMEEvent(_T("    Selected FCO:"));
	GMEEVENTLOG_GUIFCOS(selected);

	MSGTRY
	{
		CComPtr<IMgaFCO> selfco;

		if (selected.IsEmpty())
			selfco = currentModel;
		else
		{
			POSITION pos = selected.GetHeadPosition();
			selfco = selected.GetNext(pos)->mgaFco;
		}
		BeginTransaction(TRANSACTION_READ_ONLY);
		_bstr_t IDObj;
		selfco->get_ID(IDObj.GetAddress());
		CommitTransaction();
		CGMEBrowser::theInstance->FocusItem(IDObj);
	}
	MSGCATCH(_T("Error while trying to check the selected or current model"),;)
}

void CGMEView::OnUpdateCntxCheck(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!selected.IsEmpty()  ||  currentModel != NULL); 
}

void CGMEView::OnUpdateCntxInterpret(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);	// selected.IsEmpty() means we'll activete the interpeter for the current model (currentModel)
}

void CGMEView::OnUpdateCntxLocate(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!selected.IsEmpty()  ||  currentModel != NULL); 
}

void CGMEView::OnCntxRegistry()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxRegistry in ")+path+name+_T("\r\n"));
	CComPtr<IMgaFCO> fco;
	if (contextSelection)
	{
		CGMEEventLogger::LogGMEEvent(_T("    ")+contextSelection->GetName()+_T(" ")+contextSelection->GetID()+_T("\r\n"));
		fco = contextSelection->mgaFco;
	}
	else
		currentModel.QueryInterface(&fco);

	ShowRegistryBrowser(fco);

	contextSelection = 0;
	contextPort = 0;

}

void CGMEView::OnEditRegistry()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditRegistry in ")+path+name+_T("\r\n"));
	CComPtr<IMgaFCO> fco;
	COMTHROW(currentModel.QueryInterface(&fco));
	ShowRegistryBrowser(fco);
}

void CGMEView::OnEditAnnotations()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditAnnotations in ")+path+name+_T("\r\n"));
	CComPtr<IMgaFCO> fco;
	COMTHROW(currentModel.QueryInterface(&fco));
	ShowAnnotationBrowser(fco, NULL);
}


void CGMEView::ShowRegistryBrowser(CComPtr<IMgaFCO> fco)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ShowRegistryBrowser in ")+path+name+_T("\r\n"));
	try {
		BeginTransaction();

		CComObjPtr<IMgaLauncher> launcher;
		COMTHROW( launcher.CoCreateInstance(L"Mga.MgaLauncher") );
		COMTHROW( launcher->RegistryBrowser(fco) );
		CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to access object registry"),MB_OK | MB_ICONSTOP);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to access object registry.\r\n"));
	}
}

bool CGMEView::ShowAnnotationBrowser(CComPtr<IMgaFCO> fco, CComPtr<IMgaRegNode> focus)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ShowAnnotationBrowser in ")+path+name+_T("\r\n"));
	bool success = true;
	try {
		BeginTransaction();
		CComObjPtr<IMgaLauncher> launcher;
		COMTHROW( launcher.CoCreateInstance(L"Mga.MgaLauncher") );
		HRESULT hr = launcher->AnnotationBrowser(fco, focus);
		if (hr == E_MGA_MUST_ABORT) {	// JIRA GME-236 special ret code, indicating that the dialog was cancelled
			throw hresult_exception(S_OK);
		} else if (FAILED(hr)) {
			ASSERT((_T("COMTHROW: Throwing HRESULT exception. Press IGNORE"), false));
			throw hresult_exception(hr);
		} else {
			CommitTransaction();
		}
	}
	catch(hresult_exception &e) {
		success = false;
		AbortTransaction(e.hr);
		if (e.hr != S_OK) {
			AfxMessageBox(_T("Unable to access annotations"),MB_OK | MB_ICONSTOP);
			CGMEEventLogger::LogGMEEvent(_T("    Unable to access annotations.\r\n"));
		}
	}
	return success;
}

void CGMEView::OnEditSync()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditSync in ")+path+name+_T("\r\n"));
	CAspectSyncDlg dlg;
	if (IsInstance()) {
		AfxMessageBox(L"Cannot synchronize aspects for instances", MB_ICONEXCLAMATION);
		return;
	}

	dlg.m_srcAspect = currentAspect;
	POSITION apos = guiMeta->aspects.GetHeadPosition();
	while (apos) {
		CGuiMetaAspect *metaAspect = guiMeta->aspects.GetNext(apos);
		dlg.m_allAspects.AddTail(metaAspect);

		if (currentAspect != metaAspect) {
			dlg.m_dstAspects.AddTail(metaAspect);
		}
	}

	POSITION opos = children.GetHeadPosition();
	while (opos) {
		CGuiFco* ofco = children.GetNext(opos);
		ASSERT(ofco != NULL);
		CGuiObject* obj = ofco->dynamic_cast_CGuiObject();

		if (!obj) {
			// It is a connection
			continue;
		}

		bool isMoving;
		if (selected.GetCount() > 0)
			isMoving = (selected.Find(obj) != NULL);
		else
			isMoving = obj->IsVisible();

		dlg.m_allObjects.AddTail(obj);
		if (isMoving)
			dlg.m_movingObjects.AddTail(obj);
	}


	if ((GetKeyState(VK_CONTROL) & 0x8000) || dlg.DoModal() == IDOK) {
		CGuiMetaAspectList dstAspects;
		CGuiMetaAspect*    srcAspect;
		CGuiObjectList	   movingObjects;
		CGuiObjectList	   sedentaryObjects;

		dstAspects.AddTail(&dlg.m_dstAspects);
		srcAspect = dlg.m_srcAspect;
		movingObjects.AddTail(&dlg.m_movingObjects);


		POSITION opos = dlg.m_allObjects.GetHeadPosition();
		while (opos) {
			CGuiObject* obj = (CGuiObject*)dlg.m_allObjects.GetNext(opos);
			if (!dlg.m_movingObjects.Find(obj))
				sedentaryObjects.AddTail(obj);
		}
		SyncAspects(srcAspect, dstAspects, movingObjects, sedentaryObjects, dlg.m_priorityForSrcVisible == TRUE, dlg.m_priorityForSelected == TRUE);
	}
}

void CGMEView::OnUpdateEditSync(CCmdUI* pCmdUI)
{
	pCmdUI->Enable((guiMeta->aspects.GetCount()>1));
}


void CGMEView::SyncAspects(CGuiMetaAspect *srcAspect, CGuiMetaAspectList &dstAspects, CGuiObjectList &movingObjects, CGuiObjectList &sedentaryObjects,  bool priorityForSrcVisible, bool priorityForSelected)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::SyncAspects in ")+path+name+_T("\r\n"));
	if(srcAspect)
		CGMEEventLogger::LogGMEEvent(_T("    srcAspect=")+srcAspect->name+_T("\r\n"));
	CGMEEventLogger::LogGMEEvent(_T("    moving objects:"));
	GMEEVENTLOG_GUIOBJS(movingObjects);
	CGMEEventLogger::LogGMEEvent(_T("    sedentary objects:"));
	GMEEVENTLOG_GUIOBJS(sedentaryObjects);
	CGMEEventLogger::LogGMEEvent(_T("    dstAspects:\r\n"));
	try {
		BeginTransaction(TRANSACTION_GENERAL);
		BeginWaitCursor();

		modelGrid.SetSource(this);

		POSITION apos = dstAspects.GetHeadPosition();
		while (apos) {
			CGuiMetaAspect *dstAspect = dstAspects.GetNext(apos);
			if (dstAspect == srcAspect)
				continue;
			CGMEEventLogger::LogGMEEvent(_T("    ")+dstAspect->name+_T("\r\n"));

			modelGrid.Clear();

			POSITION pos;

			if (!priorityForSelected) {
				pos = sedentaryObjects.GetHeadPosition();
				while (pos) {
					CGuiObject *obj = sedentaryObjects.GetNext(pos);
					if (obj->IsVisible(dstAspect->index)) {
						SyncOnGrid(obj, dstAspect->index, dstAspect->index);
					}
				}
			}

			CGuiObjectList	lowPriorityObjects;
			pos = movingObjects.GetHeadPosition();
			while (pos) {
				CGuiObject *obj = movingObjects.GetNext(pos);
				if (obj->IsVisible(dstAspect->index)) {
					if (priorityForSrcVisible) {
						if (obj->IsVisible(srcAspect->index)) {
							SyncOnGrid(obj, srcAspect->index, dstAspect->index);
						}
						else {
							lowPriorityObjects.AddTail(obj);
						}

					}
					else {
						if (obj->IsVisible(srcAspect->index)) {
							lowPriorityObjects.AddTail(obj);
						}
						else {
							SyncOnGrid(obj, dstAspect->index, dstAspect->index);
						}
					}
				}
			}

			pos = lowPriorityObjects.GetHeadPosition();
			while (pos) {
				CGuiObject *obj = lowPriorityObjects.GetNext(pos);
				if (priorityForSrcVisible) {
					SyncOnGrid(obj, dstAspect->index, dstAspect->index);
				}
				else {
					SyncOnGrid(obj, srcAspect->index, dstAspect->index);
				}
			}

			if (priorityForSelected) {
				pos = sedentaryObjects.GetHeadPosition();
				while (pos) {
					CGuiObject *obj = sedentaryObjects.GetNext(pos);
					if (obj->IsVisible(dstAspect->index)) {
						SyncOnGrid(obj, dstAspect->index, dstAspect->index);
					}
				}
			}
		}

		needsReset = true;
		EndWaitCursor();
		__CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		AfxMessageBox(_T("Unable to synchronize aspects"),MB_OK | MB_ICONSTOP);
		CGMEEventLogger::LogGMEEvent(_T("    Unable to synchronize aspects.\r\n"));
		EndWaitCursor();
		return;
	}
	catch(_com_error& e) {                
		AbortTransaction(e.Error());
		CString error = _T("Unable to synchronize aspects");
		if (e.Description().length() != 0)
		{
			error += _T(": ");
			error += static_cast<const TCHAR*>(e.Description());
		}
		CGMEEventLogger::LogGMEEvent(error + _T("\r\n"));
		AfxMessageBox(error, MB_ICONSTOP | MB_OK);
		EndWaitCursor();
	}

}

void CGMEView::SyncOnGrid(CGuiObject *obj, int aspectIndexFrom, int aspectIndexTo)
{
	// aspectIndexTo might be equal with aspectIndexFrom
	CRect loc = obj->GetLocation(aspectIndexFrom);//take the pos from the aspFrom (source) aspect

	if (!modelGrid.IsAvailable(obj, aspectIndexFrom)) {//is enough space to occupy the pos taken from the aspFrom aspect?
		if (!modelGrid.GetClosestAvailable(obj, loc, aspectIndexTo)) { // if cannot get any position close to the position got above
			//AfxMessageBox(_T("Too Many Models! Internal Program Error!"),MB_OK | MB_ICONSTOP);
			//throw hresult_exception();
		}
	}
	obj->SetLocation(loc, aspectIndexTo);
	modelGrid.Set(obj, FALSE, aspectIndexTo);
}


void CGMEView::OnEditSelectall()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditSelectall in ")+path+name+_T("\r\n"));
	if (IsInElementDecoratorOperation())
		return;
	this->SendUnselEvent4List( &selected);
	selected.RemoveAll();
	RemoveAllAnnotationFromSelection();
	ClearConnectionSelection();
	POSITION pos = children.GetHeadPosition();
	while(pos) {
		CGuiFco* ofco = children.GetNext(pos);
		ASSERT(ofco != NULL);
		CGuiObject* obj = ofco->dynamic_cast_CGuiObject();
		if(obj && obj->IsVisible()) {
			this->SendSelecEvent4Object( obj);
			selected.AddTail(obj);
		}
	}

	GMEEVENTLOG_GUIOBJS(selected);

	pos = annotators.GetHeadPosition();
	while(pos) {
		CGuiAnnotator *ann = annotators.GetNext(pos);
		if (ann->IsVisible()) {
			AddAnnotationToSelectionTail(ann);
		}
	}

	GMEEVENTLOG_GUIANNOTATORS(selectedAnnotations);
	Invalidate();
	ChangeAttrPrefObjs(selected);
	this->SendNow();
}


void CGMEView::OnUpdateFileSettings(CCmdUI* pCmdUI)
{
	// This is an update message for Tools/Options menu
	// This triggers the refresh of plugins/interpreters menus
	theApp.UpdateDynMenus(pCmdUI->m_pMenu);
}

void CGMEView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	CScrollZoomView::OnPrepareDC(pDC, pInfo);

	if (pDC->IsPrinting()) 
	{
		PrepareAspectPrn(pInfo);
/*
		pDC->SetMapMode(MM_ISOTROPIC);
		CRect extent, objext, annext;
		CGuiObject::GetExtent(children,objext);
		CGuiAnnotator::GetExtent(annotators,annext);
		extent.UnionRect(&objext, &annext);
		pDC->SetWindowExt(extent.right, extent.bottom);
		double w, h;
		w = GetDeviceCaps(pDC->m_hDC,PHYSICALWIDTH) - 4*GetDeviceCaps(pDC->m_hDC,PHYSICALOFFSETX);
		h = GetDeviceCaps(pDC->m_hDC,PHYSICALHEIGHT)- 4*GetDeviceCaps(pDC->m_hDC,PHYSICALOFFSETY);
		pDC->SetViewportExt((int)w, (int)h);
*/
	}
}

void CGMEView::OnEditPastespecialAssubtype()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditPastespecialAssubtype in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = false;
		DoPasteItem(&clipboardData,false,false,false,true,false);
	}
}

void CGMEView::OnEditPastespecialAsinstance()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditPastespecialAsinstance in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = false;
		DoPasteItem(&clipboardData,false,false,false,true,true);
	}
}

void CGMEView::OnEditPastespecialAsreference()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditPastespecialAsreference in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = false;
		DoPasteItem(&clipboardData,false,false,true);
	}
}

void CGMEView::OnEditPastespecialAsclosure()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditPastespecialAsclosure in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = false;
		DoPasteItem(&clipboardData,false,false,false,false,false,true, false);
	}
}

void CGMEView::OnEditPastespecialAdditive()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditPastespecialAdditive in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = false;
		DoPasteItem(&clipboardData,false,false,false,false,false,true, false);
	}
}

void CGMEView::OnEditPastespecialMerge()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnEditPastespecialMerge in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = false;
		DoPasteItem(&clipboardData,false,false,false,false,false,true, true);
	}
}

void CGMEView::OnUpdateEditPastespecialAsinstance(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnUpdateEditPastespecialAsreference(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnUpdateEditPastespecialAssubtype(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnUpdateEditPastespecialAsclosure(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnUpdateEditPastespecialAdditive(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnUpdateEditPastespecialMerge(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnCntxPastespecialAsinstance()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxPastespecialAsinstance in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = true;
		DoPasteItem(&clipboardData,true,false,false,true,true,false,false,0,contextMenuLocation);
	}
}

void CGMEView::OnUpdateCntxPastespecialAsinstance(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnCntxPastespecialAsreference()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxPastespecialAsreference in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = instanceDrop = false;
		DoPasteItem(&clipboardData,true,false,true,false,false,false,false,0,contextMenuLocation);
	}
}

void CGMEView::OnUpdateCntxPastespecialAsreference(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnCntxPastespecialAssubtype()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxPastespecialAssubtype in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = true;
		instanceDrop = false;
		DoPasteItem(&clipboardData,true,false,false,true,false,false,false,0,contextMenuLocation);
	}
}

void CGMEView::OnUpdateCntxPastespecialAssubtype(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnCntxPastespecialAsclosure()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxPastespecialAsclosure in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = true;
		instanceDrop = false;
		DoPasteItem(&clipboardData,false,false,false,false,false,true,false,0,contextMenuLocation);
	}
}

void CGMEView::OnUpdateCntxPastespecialAsclosure(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnCntxPastespecialAdditive()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxPastespecialAdditive in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = true;
		instanceDrop = false;
		DoPasteItem(&clipboardData,false,false,false,false,false,true,false,0,contextMenuLocation);
	}
}

void CGMEView::OnUpdateCntxPastespecialAdditive(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnCntxPastespecialMerge()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxPastespecialMerge in ")+path+name+_T("\r\n"));
	if(isType) {
		COleDataObject clipboardData;
		clipboardData.AttachClipboard();
		derivedDrop = true;
		instanceDrop = false;
		DoPasteItem(&clipboardData,false,false,false,false,false,true,true,0,contextMenuLocation);
	}
}

void CGMEView::OnUpdateCntxPastespecialMerge(CCmdUI* pCmdUI)
{
	OnUpdateEditPaste(pCmdUI);
}

void CGMEView::OnCntxRedirectionpaste()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxRedirectionpaste in ")+path+name+_T("\r\n"));
	if(isType && contextSelection) {
		CGuiObject* ref = contextSelection->dynamic_cast_CGuiReference();
		if (!ref)
			ref = contextSelection->dynamic_cast_CGuiCompoundReference();
		if (ref) {
			COleDataObject clipboardData;
			clipboardData.AttachClipboard();
			derivedDrop = instanceDrop = false;
			DoPasteItem(&clipboardData,true,false,true,false,false,false,false,ref,contextMenuLocation);
		}
	}
}

void CGMEView::OnUpdateCntxRedirectionpaste(CCmdUI* pCmdUI)
{
	BOOL bEnable = (contextSelection != 0);
	CGuiObject* ref = NULL;
	if (contextSelection != NULL)
		ref = contextSelection->dynamic_cast_CGuiReference();
	if (contextSelection && !ref)
		ref = contextSelection->dynamic_cast_CGuiCompoundReference();
	bEnable = bEnable && (ref != 0);
	COleDataObject dataObj;
	bEnable = bEnable && dataObj.AttachClipboard() &&
		(CGMEDataSource::IsGmeNativeDataAvailable(&dataObj,theApp.mgaProject));

	pCmdUI->Enable(bEnable);
}

void CGMEView::OnCntxConnect()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxConnect in ")+path+name+_T("\r\n"));
	if (contextSelection) {
		CGuiObject* obj = contextSelection->dynamic_cast_CGuiObject();
		if (obj) {
			CGuiPort* port = obj->FindPort(contextMenuLocation);
			if (connSrc == 0) {
				connSrc = obj;
				connSrcPort = port;
				connSrcHotSide = GME_CENTER;
				tmpConnectMode = true;
				SetCursor(autoconnect2Cursor);
			}
			else {
				Connect(connSrc,connSrcPort,connSrcHotSide,obj,port, GME_CENTER, ::GetKeyState(VK_SHIFT) < 0);
				ClearConnSpecs();
				tmpConnectMode = false;
				SetCursor(editCursor);
			}
			ShowCursor(TRUE);
			Invalidate();
		}
	}
}

void CGMEView::OnUpdateCntxConnect(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocument()->GetEditMode() == GME_EDIT_MODE && isType);
}

void CGMEView::OnResetSticky()
{
	CGMEDoc *doc = GetDocument();
	if(doc && doc->resolver) {
		COMTHROW(doc->resolver->Clear());
	}
}

void CGMEView::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	if (nHitTest != HTCLIENT && !IsInElementDecoratorOperation() && IsCursorChangedByDecorator())
		SetEditCursor();
	else
		CScrollZoomView::OnNcMouseMove(nHitTest, point);
}

bool CGMEView::IsLegalConnectionEnd(CGuiObject *connEnd, CGuiPort *port)
{
	IMgaMetaModelPtr metaModel = guiMeta->mgaMeta.p;
	IMgaMetaRolePtr srcMeta = connSrc->dynamic_cast_CGuiObject()->metaRole.p;
	IMgaMetaRolePtr srcPortMeta = connSrcPort ? connSrcPort->metaRole.p : NULL;
	IMgaMetaRolePtr dstMeta = connEnd->dynamic_cast_CGuiObject()->metaRole.p;
	IMgaMetaRolePtr dstPortMeta = port ? port->metaRole.p : NULL;

	wchar_t srcPath[100];
	if (srcPortMeta)
		swprintf_s(srcPath, L"src %ld %ld, ", srcMeta->MetaRef, srcPortMeta->MetaRef);
	else
		swprintf_s(srcPath, L"src %ld, ", srcMeta->MetaRef);

	wchar_t dstPath[100];
	if (dstPortMeta)
		swprintf_s(dstPath, L"dst %ld %ld", dstMeta->MetaRef, dstPortMeta->MetaRef);
	else
		swprintf_s(dstPath, L"dst %ld", dstMeta->MetaRef);

	_bstr_t path = srcPath;
	path += dstPath;

	IMgaMetaRolesPtr roles = metaModel->__LegalConnectionRoles(path);

	// TODO: check for primary aspect

	return roles->Count > 0;
}

void CGMEView::OnMouseMove(UINT nFlags, CPoint screenpoint)
{
	if (!isLeftMouseButtonDown || (GetKeyState(VK_LBUTTON) & 0x8000) == 0) {
		isConnectionJustSelected = false;
		isLeftMouseButtonDown = false;
	}

	CGMEView *self = const_cast<CGMEView *>(this);
	CPoint point(screenpoint);
	CoordinateTransfer(point);

	{
		CGuiObject *object = self ? self->FindObject(point) : 0;
		CGuiPort   *port   = object? object->FindPort(point) : 0;
		if(object && object != lastObject && theApp.isMouseOverNotifyEnabled()/* && GetDocument()->GetEditMode() == GME_EDIT_MODE*/) {
			this->SendMouseOver4Object(object);
		}
		lastObject = object;
		lastPort = port;
	}

	if (GetDocument()->GetEditMode() == GME_EDIT_MODE) { // new decorator notification logic
		//static CGuiObject* lastObject = 0;
		CGuiObject*	object	= self		? self->FindObject(point, true) : 0;
		CGuiPort*	port	= object	? object->FindPort(point, true) : 0;
		if (object == NULL)
			object = self ? self->FindObject(point, true, true) : 0;
		CGuiAnnotator* annotation = FindAnnotation(point);

		if (IsInElementDecoratorOperation()) {
			CComPtr<IMgaElementDecorator> newDecorator;
			if (IsDecoratorOrAnnotator()) {
				CGuiObject* objInOp = GetObjectInDecoratorOperation();
				// It can be NULL: if we start a label edit operation and we double click on another label
				// then in-spite of the modal behavior of the in-place dialog GMEView gets MouseMoves
				if (objInOp != NULL) {
					CGuiAspect* pAspect = objInOp->GetCurrentAspect();
					if (pAspect != NULL) {
						CComQIPtr<IMgaElementDecorator> newDecorator2(pAspect->GetDecorator());
						newDecorator = newDecorator2;
					}
				}
			} else {
				CGuiAnnotator* annInOp = GetAnnotatorInDecoratorOperation();
				if (annInOp != NULL)
					newDecorator = annInOp->GetDecorator(currentAspect->index);
			}
			if (newDecorator) {
				CClientDC transformDC(this);
				OnPrepareDC(&transformDC);
				HRESULT retVal = newDecorator->MouseMoved(nFlags, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
				if (retVal == S_DECORATOR_EVENT_HANDLED) {
					CScrollZoomView::OnMouseMove(nFlags, screenpoint);
					return;
				} else if (retVal != S_OK &&
						   retVal != S_DECORATOR_EVENT_NOT_HANDLED &&
						   retVal != E_DECORATOR_NOT_IMPLEMENTED)
				{
					CancelDecoratorOperation();
					// FIXME: how to handle this error?
					// COMTHROW(retVal);
					return;
				}
			}
		} else if (isInConnectionCustomizeOperation) {
			// Update Connection Customization Tracker
			customizeConnectionCurrCursor = point;
			Invalidate();
		} else if (object != NULL || annotation != NULL) {
			CComPtr<IMgaElementDecorator> newDecorator;
			if (object != NULL) {
				CGuiAspect* pAspect = object->GetCurrentAspect();
				if (pAspect != NULL) {
					CComQIPtr<IMgaElementDecorator> newDecorator2(pAspect->GetDecorator());
					newDecorator = newDecorator2;
				}
			} else {
				ASSERT(annotation != NULL);
				newDecorator = annotation->GetDecorator(currentAspect->index);
			}
			if (newDecorator) {
				CClientDC transformDC(this);
				OnPrepareDC(&transformDC);
				HRESULT retVal = newDecorator->MouseMoved(nFlags, point.x, point.y, (ULONGLONG)transformDC.m_hDC);
				if (retVal == S_DECORATOR_EVENT_HANDLED) {
					CScrollZoomView::OnMouseMove(nFlags, screenpoint);
					return;
				} else if (retVal != S_OK &&
						   retVal != S_DECORATOR_EVENT_NOT_HANDLED &&
						   retVal != E_DECORATOR_NOT_IMPLEMENTED)
				{
					CancelDecoratorOperation();
					// FIXME: how to handle this error?
					// COMTHROW(retVal);
					return;
				}
			}
		} else {
			if (selectedConnection != NULL && !isConnectionJustSelected) {
				SetConnectionCustomizeCursor(point);
			} else {
				if (isCursorChangedByEdgeCustomize) {
					::SetCursor(customizeConnectionCursorBackup);
					isCursorChangedByEdgeCustomize = false;
				}
			}
			if (IsCursorChangedByDecorator())
				SetEditCursor();
		}
	}

	if (GetDocument()->GetEditMode() == GME_VISUAL_MODE)
	{
		CGuiObject *object = self? self->FindObject(point): 0;
		// if object found, curr_Connection will be 0
		CGuiConnection        *curr_Connection = object? 0: router.FindConnection( point);
		if( last_Connection != curr_Connection) // state change for at most two connections
		{
			if( last_Connection) last_Connection->ToggleHover(); // if a previous was selected, now it will become unselected
			if( curr_Connection) curr_Connection->ToggleHover(); // toggle the new one
			last_Connection = curr_Connection;
			Invalidate();
		} 
	}
	if ((GetDocument()->GetEditMode() == GME_AUTOCONNECT_MODE || GetDocument()->GetEditMode() == GME_SHORTAUTOCONNECT_MODE || (tmpConnectMode))) {
		CGuiObject *object = self->FindObject(point);
		CGuiPort *port = object ? object->FindPort(point) : NULL;
		if (object && (connSrc == NULL || IsLegalConnectionEnd(object, port))) {
			CRect rect = object->GetLocation();
			int hotSide = GME_CENTER;
			if(port && port->IsRealPort()) {
				rect = port->GetLocation() + rect.TopLeft();
			}
			else {
				port = NULL;

				if (object->IsHotspotEnabled()) {
					int mx = rect.CenterPoint().x;
					int my = rect.CenterPoint().y;
					int hsRadx = GME_HOTSPOT_RADIUS;
					int hsRady = GME_HOTSPOT_RADIUS;
					if (rect.Width() < (GME_HOTSPOT_RADIUS * 3)) {
						hsRadx = rect.Width()/3;
					}
					if (rect.Height() < (GME_HOTSPOT_RADIUS * 3)) {
						hsRady = rect.Height()/3;
					}
					if ( abs(point.x - mx) < hsRadx) {
						if ( abs(point.y - rect.top) < hsRady ) {
							hotSide = GME_NORTH;
						}
						if ( abs(point.y - rect.bottom) < hsRady ) {
							hotSide = GME_SOUTH;
						}
					}
					else if ( abs(point.y - my) < hsRady) {
						if ( abs(point.x - rect.left) < hsRadx ) {
							hotSide = GME_WEST;
						}
						if ( abs(point.x - rect.right) < hsRadx ) {
							hotSide = GME_EAST;
						}
					}
				}
			}

			if((object != connTmp) || (port != connTmpPort) || (hotSide != connTmpHotSide)) {
					connTmp = object;
					connTmpPort = port;
					connTmpHotSide = hotSide;
					Invalidate();
			}
		}
		else {
			connTmp = NULL;
			connTmpPort = NULL;
			connTmpHotSide = GME_CENTER;
			Invalidate();
		}
	}

	CScrollZoomView::OnMouseMove(nFlags, screenpoint);
}

void CGMEView::ClearConnSpecs()
{
	connSrc = NULL;
	connSrcPort = NULL;
	connSrcHotSide = GME_CENTER;
	connTmp = NULL;
	connTmpPort = NULL;
	connTmpHotSide = GME_CENTER;
}

void CGMEView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == GME_ANIM_EVENT_ID) {
		Invalidate();
	}
	CScrollZoomView::OnTimer(nIDEvent);
}

void CGMEView::OnUpdateCntxInsertannotation(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CGMEView::OnCntxInsertannotation()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxInsertannotation in ")+path+name+_T("\r\n"));
	CComPtr<IMgaRegNode> rootReg;
	try {
		BeginTransaction();
		int annID = 0;
		bool found = false;

		while (!found) {
			rootReg = NULL;
			CString path;
			path.Format(_T("%s/%s%d"), AN_ROOT, AN_DEFANNOTATION_NAME, annID++);
			CComBSTR bstr(path);
			COMTHROW(currentModel->get_RegistryNode(bstr, &rootReg));
			long status;
			COMTHROW(rootReg->get_Status(&status));
			if (status == ATTSTATUS_UNDEFINED) {
				found = true;
			}
		}

		CComBSTR value(AN_DEFAULTANNOTATION_TXT);
		COMTHROW(rootReg->put_Value(value));

		CComPtr<IMgaRegNode> aspRoot;
		CComBSTR aspName(AN_ASPECTS);
		COMTHROW(rootReg->get_SubNodeByName(aspName, &aspRoot));
		CString pos;
		pos.Format(_T("%d,%d"), contextMenuLocation.x, contextMenuLocation.y);
		CComBSTR posval(pos);
		COMTHROW(aspRoot->put_Value(posval));

		CComPtr<IMgaRegNode> defAspNode;
		CComBSTR defAspName(AN_DEFASPECT);
		COMTHROW(aspRoot->get_SubNodeByName(defAspName, &defAspNode));
		CComBSTR bstrVis(AN_VISIBLE_DEFAULT);
		COMTHROW(defAspNode->put_Value(bstrVis));

		CComPtr<IMgaFCO> fcoToShow;
		currentModel.QueryInterface(&fcoToShow);

		if (ShowAnnotationBrowser(fcoToShow, rootReg)) {
			// ANNTODO: new object id list
			CommitTransaction();
		} else {
			throw hresult_exception(S_OK);	// the dialog was cancelled
		}
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		if (e.hr != S_OK) {
			AfxMessageBox(_T("Unable to insert annotation"),MB_ICONSTOP | MB_OK);
			CGMEEventLogger::LogGMEEvent(_T("    Unable to insert annotation.\r\n"));
			return;
		}
	}
	Invalidate(true);
}

void CGMEView::OnCntxAnnotations()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxAnnotations in ")+path+name+_T("\r\n"));
	CComPtr<IMgaFCO> fco;
	currentModel.QueryInterface(&fco);
	ShowAnnotationBrowser(fco, contextAnnotation ? contextAnnotation->rootNode : NULL);
	contextAnnotation = NULL;
	contextSelection = NULL;
}

void CGMEView::OnUpdateCntxAutoRouters( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();
	if ( contextSelection ) {
		CComPtr<IMgaFCO> spFCO = contextSelection->mgaFco;
		if ( spFCO ) {
			BeginTransaction(TRANSACTION_READ_ONLY);
			CComBSTR bstrPref;
			COMTHROW( spFCO->get_RegistryValue( CComBSTR( AUTOROUTER_PREF ), &bstrPref ) );
			CommitTransaction();
			CString strPref( bstrPref );
			switch ( pCmdUI->m_nID ) {
				case ID_CNTX_SRCAR_NORTH : 	pCmdUI->SetCheck( ( strPref.Find( _T("N") ) != -1 ) ? 1 : 0 ); return;
				case ID_CNTX_SRCAR_SOUTH : 	pCmdUI->SetCheck( ( strPref.Find( _T("S") ) != -1 ) ? 1 : 0 ); return;
				case ID_CNTX_SRCAR_WEST :	pCmdUI->SetCheck( ( strPref.Find( _T("W") ) != -1 ) ? 1 : 0 ); return;
				case ID_CNTX_SRCAR_EAST :	pCmdUI->SetCheck( ( strPref.Find( _T("E") ) != -1 ) ? 1 : 0 ); return;
				case ID_CNTX_DSTAR_NORTH :	pCmdUI->SetCheck( ( strPref.Find( _T("n") ) != -1 ) ? 1 : 0 ); return;
				case ID_CNTX_DSTAR_SOUTH :	pCmdUI->SetCheck( ( strPref.Find( _T("s") ) != -1 ) ? 1 : 0 ); return;
				case ID_CNTX_DSTAR_WEST :	pCmdUI->SetCheck( ( strPref.Find( _T("w") ) != -1 ) ? 1 : 0 ); return;
				case ID_CNTX_DSTAR_EAST :	pCmdUI->SetCheck( ( strPref.Find( _T("e") ) != -1 ) ? 1 : 0 ); return;
			}
			int iCnt = 0;
			bool bSet = pCmdUI->m_nID == ID_CNTX_SRCAR_SET || pCmdUI->m_nID == ID_CNTX_DSTAR_SET;
			CString str = ( pCmdUI->m_nID == ID_CNTX_SRCAR_SET || pCmdUI->m_nID == ID_CNTX_SRCAR_CLEAR ) ? _T("NEWS") : _T("news");
			for ( int i = 0 ; i < 4 ; i++ )
				if ( strPref.Find( str.Mid( i, 1 ) ) != -1 ) iCnt++;
			if ( iCnt == 4 && bSet || iCnt == 0 && ! bSet )
				pCmdUI->Enable( FALSE );
		}
	}
}

void CGMEView::OnCntxSrcarSouth()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxSrcarSouth in ")+path+name+_T("\r\n"));
	SwapAutoRouterPref( _T("S") );
}

void CGMEView::OnCntxSrcarNorth()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxSrcarNorth in ")+path+name+_T("\r\n"));
	SwapAutoRouterPref( _T("N") );
}

void CGMEView::OnCntxSrcarEast()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxSrcarEast in ")+path+name+_T("\r\n"));
	SwapAutoRouterPref( _T("E") );
}

void CGMEView::OnCntxSrcarWest()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxSrcarWest in ")+path+name+_T("\r\n"));
	SwapAutoRouterPref( _T("W") );
}

void CGMEView::OnCntxDstarEast()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxDstarEast in ")+path+name+_T("\r\n"));
	SwapAutoRouterPref( _T("e") );
}

void CGMEView::OnCntxDstarNorth()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxDstarNorth in ")+path+name+_T("\r\n"));
	SwapAutoRouterPref( _T("n") );
}

void CGMEView::OnCntxDstarSouth()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxDstarSouth in ")+path+name+_T("\r\n"));
	SwapAutoRouterPref( _T("s") );
}

void CGMEView::OnCntxDstarWest()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxDstarWest in ")+path+name+_T("\r\n"));
	SwapAutoRouterPref( _T("w") );
}

void CGMEView::OnCntxDstarClear()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxDstarClear in ")+path+name+_T("\r\n"));
	SetAllAutoRouterPref( false, true );
}

void CGMEView::OnCntxSrcarClear()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxSrcarClear in ")+path+name+_T("\r\n"));
	SetAllAutoRouterPref( true, true );
}

void CGMEView::OnCntxDstarSet()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxDstarSet in ")+path+name+_T("\r\n"));
	SetAllAutoRouterPref( false, false );
}

void CGMEView::OnCntxSrcarSet()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnCntxSrcarSet in ")+path+name+_T("\r\n"));
	SetAllAutoRouterPref( true, false );
}

void CGMEView::SwapAutoRouterPref( const CString& strP )
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::SwapAutoRouterPref in ")+path+name+_T("\r\n"));
	if( contextSelection ) {
		CGMEEventLogger::LogGMEEvent(_T("    ")+contextSelection->GetName()+_T(" ")+contextSelection->GetID()+_T("\r\n"));
		CComPtr<IMgaFCO> spFCO = contextSelection->mgaFco;
		if ( spFCO ) {
			BeginTransaction(TRANSACTION_GENERAL);
			CComBSTR bstrPref;
			COMTHROW( spFCO->get_RegistryValue( CComBSTR( AUTOROUTER_PREF ), &bstrPref ) );
			CString strPref( bstrPref );
			int iPos = strPref.Find( strP );
			if ( iPos == -1 )
				if ( strP == _T("n") || strP == _T("s") || strP == _T("w") || strP == _T("e") )
					strPref = strP + strPref;
				else
					strPref += strP;
			else
				strPref.Replace( strP, _T("") );
			COMTHROW( spFCO->put_RegistryValue( CComBSTR( AUTOROUTER_PREF ), CComBSTR( strPref ) ) );
			CommitTransaction();
		}
	}
}

void CGMEView::SetAllAutoRouterPref( bool bSrc, bool bClear )
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::SetAllAutoRouterPref in ")+path+name+_T("\r\n"));
	if( contextSelection ) {
		CGMEEventLogger::LogGMEEvent(_T("    ")+contextSelection->GetName()+_T(" ")+contextSelection->GetID()+_T("\r\n"));
		CComPtr<IMgaFCO> spFCO = contextSelection->mgaFco;
		if ( spFCO ) {
			BeginTransaction(TRANSACTION_GENERAL);
			CComBSTR bstrPref;
			COMTHROW( spFCO->get_RegistryValue( CComBSTR( AUTOROUTER_PREF ), &bstrPref ) );
			CString strPref( bstrPref );
			CString src = _T("NEWS");
			CString dst = _T("news");
			for ( int i = 0 ; i < 4 ; i++ )
				strPref.Replace( ( bSrc ) ? src.Mid( i, 1 ) : dst.Mid( i, 1 ), _T("") );
			if ( ! bClear )
				if ( bSrc )
					strPref = src + strPref;
				else
					strPref += dst;
			COMTHROW( spFCO->put_RegistryValue( CComBSTR( AUTOROUTER_PREF ), CComBSTR( strPref ) ) );
			CommitTransaction();
		}
	}
}

void CGMEView::OnPrintMetafile()
{
	CString filePath = _T(""); // "c:\\tmp\\meta.emf";
	// call FileOpenDialog
	CFileDialog filedlg(FALSE, _T("emf"), name, OFN_OVERWRITEPROMPT|OFN_HIDEREADONLY,
		_T("Enhanced Metafile Files (*.emf)|*.emf|All Files (*.*)|*.*||"));
	if (filedlg.DoModal() != IDOK)
		return;
	filePath = filedlg.GetPathName();
	CMetaFileDC cDC;
	CDC* pDC = GetDC();
	BOOL ret = cDC.CreateEnhanced(pDC,filePath,NULL,_T("GME Model"));
	ReleaseDC(pDC);
	if (ret == FALSE) {
		AfxMessageBox(_T("Unable to create metafile."), MB_OK | MB_ICONSTOP);
		return;
	}

	cDC.m_bPrinting = TRUE;		// HACK by Peter (c)

	OnDraw(&cDC);

	HENHMETAFILE hEmf = cDC.CloseEnhanced();
	if ( hEmf ) {
		DeleteEnhMetaFile(hEmf);
	}
}

void CGMEView::ZoomRect(CRect srect)
{
	CRect crect;
	GetClientRect(&crect);
	int curzoom = m_zoomVal;
	CPoint home = CPoint(srect.left, srect.top);
	CSize orisize_srect(srect.Width(), srect.Height());
	srect.OffsetRect(-home);

	// convert srect to client coordinates like extent
	{
		CWindowDC dc(NULL);
		dc.SetMapMode(MM_ISOTROPIC);
		dc.SetWindowExt(100,100);
		dc.SetViewportExt(curzoom, curzoom);
		dc.DPtoLP(&srect);
	}
	int zoom = (int)min(100.*crect.Height()/srect.Height(), 100.*crect.Width()/srect.Width());
	m_zoomVal = (zoom<ZOOM_MIN)? ZOOM_MIN: ((zoom>ZOOM_MAX)? ZOOM_MAX: zoom);

	if (m_zoomVal != curzoom)
		frame->propBar.SetZoomVal(m_zoomVal);
	CMainFrame::theInstance->WriteStatusZoom(m_zoomVal);
	m_zoomP.x = m_zoomP.y = 0;
	if (curzoom == m_zoomVal)
		return;

	CPoint point = home;
	int yRes = CClientDC(this).GetDeviceCaps(LOGPIXELSY);

	point.x = MulDiv(point.x, 96, yRes);
	point.y = MulDiv(point.y, 96, yRes);

	point.x += orisize_srect.cx / 2;
	point.y += orisize_srect.cy / 2;

// prevous zoom value : curzoom
// new zoom value stored in m_zoomVal
// point : win client coordinates - this image point has to be centered
	SetZoomPoint(curzoom, point);
	Invalidate();
}

void CGMEView::ZoomToFCOs(CRect srect)
{
	// perform a zoom to that rectangle
	CRect crect;
	GetClientRect(&crect);
	CSize sizeSb;
	GetScrollBarSizes(sizeSb);
	if (m_noHscroll)	// let's suppose it will be after zoom
		crect.bottom -= sizeSb.cy; 
	if (m_noVscroll)	// let's suppose it will be
		crect.right -= sizeSb.cx;

	int curzoom = m_zoomVal;
	CRect ori_proj = srect;
	srect.OffsetRect(-CPoint(srect.left, srect.top));

	int zoom = (int)min(100.*crect.Height()/srect.Height(), 100.*crect.Width()/srect.Width());
	m_zoomVal = (zoom<ZOOM_MIN)? ZOOM_MIN: ((zoom>ZOOM_MAX)? ZOOM_MAX: zoom);

	if (m_zoomVal != curzoom)
		frame->propBar.SetZoomVal(m_zoomVal);
	CMainFrame::theInstance->WriteStatusZoom(m_zoomVal);
	m_zoomP.x = m_zoomP.y = 0;

	CPoint scp = GetScrollPosition();       // upper corner of scrolling

	{
		CWindowDC dc(NULL);
		dc.SetMapMode(MM_ISOTROPIC);
		dc.SetWindowExt(100,100);
		dc.SetViewportExt(curzoom, curzoom);
		dc.LPtoDP(&ori_proj);
		dc.LPtoDP(&scp);
	}
	CPoint home = CPoint(ori_proj.left, ori_proj.top);
	CSize orisize_srect(ori_proj.Width(), ori_proj.Height());

	CPoint point = home - scp;
	point.x += orisize_srect.cx/2;
	point.y += orisize_srect.cy/2;

// prevous zoom value : curzoom
// new zoom value stored in m_zoomVal
// point : win client coordinates - this image point has to be centered
	SetZoomPoint(curzoom, point);
	Invalidate();
}

void CGMEView::OnZoomIn()
{
	int zoom = m_zoomVal+5;
	zoom = (zoom<ZOOM_MIN)? ZOOM_MIN: ((zoom>ZOOM_MAX)? ZOOM_MAX: zoom);
	ZoomPercent( zoom);
}

void CGMEView::OnZoomOut()
{
	int zoom = m_zoomVal-5;
	zoom = (zoom<ZOOM_MIN)? ZOOM_MIN: ((zoom>ZOOM_MAX)? ZOOM_MAX: zoom);
	ZoomPercent( zoom);
}

void CGMEView::ZoomPercent(long percent)
{
	OnZoom(0, (LPARAM)percent);
}

LRESULT CGMEView::OnZoom(WPARAM, LPARAM lParam)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnZoom() in ")+path+name+_T("\r\n"));
	// BOOL userdef = (BOOL)wParam;
	int zoom = (int)lParam;

	// zoom it 
	if (zoom > 0)
	{
		int curzoom = m_zoomVal;
		m_zoomVal = (zoom<ZOOM_MIN)? ZOOM_MIN: ((zoom>ZOOM_MAX)? ZOOM_MAX: zoom);
		if (m_zoomVal != zoom)
			frame->propBar.SetZoomVal(m_zoomVal);
		CMainFrame::theInstance->WriteStatusZoom(m_zoomVal);
		m_zoomP.x = m_zoomP.y = 0;
		if (curzoom == m_zoomVal)
			return 0;

		CPoint point;
		CRect client(0,0,0,0);
		GetClientRect(&client);

		CRect rect;
		GetClientRect(&rect);

		CRect objext, annext, extent;
		CGuiObject::GetExtent(children, objext);
		CGuiAnnotator::GetExtent(annotators, annext);
		extent.UnionRect(&objext, &annext);
		extent.right = (int)(extent.right*EXTENT_ERROR_CORR); // ??
		extent.bottom = (int)(extent.bottom*EXTENT_ERROR_CORR); // ??
		CSize s(extent.right, extent.bottom);

		CWindowDC dc(NULL);
		dc.SetMapMode(MM_ISOTROPIC);
		dc.SetWindowExt(100,100);
		dc.SetViewportExt(curzoom, curzoom);
		CPoint expoint(extent.right, extent.bottom);
		dc.LPtoDP((LPPOINT)&expoint);

		point.x = (expoint.x < client.Width())? point.x = expoint.x/2 :point.x = client.Width()/2;
		point.y = (expoint.y < client.Height())? point.y = expoint.y/2 :point.y = client.Height()/2;
		
		SetZoomPoint(curzoom, point);
		frame->propBar.SetZoomVal(m_zoomVal);
		CMainFrame::theInstance->WriteStatusZoom(m_zoomVal);
		Invalidate();
	}
	else
	{
		CRect rect;
		GetClientRect(&rect);

		CRect objext, annext, extent;
		CGuiObject::GetExtent(children, objext);
		CGuiAnnotator::GetExtent(annotators, annext);
		extent.UnionRect(&objext, &annext);
		extent.right = (int)(extent.right*EXTENT_ERROR_CORR); // ??
		extent.bottom = (int)(extent.bottom*EXTENT_ERROR_CORR); // ??
		CSize s(extent.right, extent.bottom);
//		s.cx = s.cx + END_SCROLL_OFFSET;
//		s.cy = s.cy + END_SCROLL_OFFSET;
		switch (zoom)
		{
		case ZOOM_WIDTH:
			m_zoomVal = (int)(100.*rect.Width()/s.cx);
			break;
		case ZOOM_HEIGHT:
			m_zoomVal = (int)(100.*rect.Height()/s.cy);
			break;
		case ZOOM_ALL:
			m_zoomVal = (int)min(100.*rect.Height()/s.cy, 100.*rect.Width()/s.cx);
			break;
		}
		frame->propBar.SetZoomVal(m_zoomVal);
		CMainFrame::theInstance->WriteStatusZoom(m_zoomVal);
		Invalidate();
	}
	return 0;
}

LRESULT CGMEView::OnPannRefresh(WPARAM, LPARAM)
{
	DoPannWinRefresh();
	return 0;
}

LRESULT CGMEView::OnPannScroll(WPARAM wParam, LPARAM lParam)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnPannScroll() in ")+path+name+_T("\r\n"));
	int relx = (DWORD)wParam;
	int rely = (DWORD)lParam;
	CPoint current = GetScrollPosition();       // upper corner of scrolling
	current.x += relx;
	current.y += rely;
	ScrollToPosition(current);				    // set upper left position
	Invalidate();
	return 0;
}

LRESULT CGMEView::OnDecoratorViewRefreshRequest(WPARAM wParam, LPARAM lParam)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnDecoratorViewRefreshRequest() in ") + path + name + _T("\r\n"));
	refresh_mode_enum refreshMode = (refresh_mode_enum) lParam;
	switch(refreshMode) {
		case RM_REGENERATE_PARENT_ALSO:
			{
				ResetParent(true);
			}
			// Intentionally no break!
		case RM_REGENERATE_SELF:
			{
				Reset(true);
			}
			break;
		case RM_REGENERATE_ALL_VIEWS:
			{
				GetDocument()->ResetAllViews();
			}
			break;
		default: break;	// RM_NOREFRESH, RM_REDRAW_SELF
	}
	return 0;
}

LRESULT CGMEView::OnExecutePendingRequests(WPARAM wParam, LPARAM lParam)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnExecutePendingRequests() in ") + path + name + _T("\r\n"));
	
	executingPendingRequests = true;
	try {
		CComPtr<IMgaTerritory> terr;
		COMTHROW(theApp.mgaProject->get_ActiveTerritory(&terr));
		if (!terr)
			BeginTransaction();
		TryToExecutePendingRequests();
		if (!terr)
			CommitTransaction();
	}
	catch(hresult_exception &e) {
		// silent failure (not critical)
		AbortTransaction(e.hr);
	}

	executingPendingRequests = false;
	return 0;
}

void CGMEView::OnCntxNamePositionNorth()
{
	ChangeNamePosition(0);
}

void CGMEView::OnCntxNamePositionEast()
{
	ChangeNamePosition(2);
}

void CGMEView::OnCntxNamePositionSouth()
{
	ChangeNamePosition(4);
}

void CGMEView::OnCntxNamePositionWest()
{
	ChangeNamePosition(6);
}

void CGMEView::OnUpdateCntxNamePositionNorth( CCmdUI* pCmdUI )
{
	UpdateNamePositionMenuItem( pCmdUI, 0);
}

void CGMEView::OnUpdateCntxNamePositionEast( CCmdUI* pCmdUI )
{
	UpdateNamePositionMenuItem( pCmdUI, 2);
}

void CGMEView::OnUpdateCntxNamePositionSouth( CCmdUI* pCmdUI )
{
	UpdateNamePositionMenuItem( pCmdUI, 4);
}

void CGMEView::OnUpdateCntxNamePositionWest( CCmdUI* pCmdUI )
{
	UpdateNamePositionMenuItem( pCmdUI, 6);
}

void CGMEView::UpdateNamePositionMenuItem( CCmdUI* pCmdUI, int p_this_value)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::UpdateNamePositionMenuItem\r\n"));

	ASSERT(p_this_value == 0 || p_this_value == 2 || p_this_value == 4 || p_this_value == 6);

	bool any_sel = !selected.IsEmpty();
	pCmdUI->Enable( any_sel);
	if( !any_sel) return;

	// calculate the common selected namePos attribute value of the selected objects
	int common_value( -1);
	bool all_equal( true);

	try {
		BeginTransaction(TRANSACTION_READ_ONLY);

		bool first_value( true), res(true);
		POSITION pos = selected.GetHeadPosition();
		while(pos && all_equal) {
			CGuiObject *obj = selected.GetNext(pos);
			int v( -1);

			if( obj && obj->mgaFco)
				res = GetNamePositionVal( obj->mgaFco, &v);

			if( first_value)
			{
				common_value = v;
				first_value = false;
			}
			all_equal = all_equal && res && common_value == v && common_value == p_this_value;
		}

		CommitTransaction();
	}
	catch(hresult_exception &e) {
		all_equal = false;
		AbortTransaction(e.hr);
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::UpdateNamePositionMenuItem - Unable to get NamePosition preference value.\r\n"));
	}

	// set the radiobutton like icon on/off based on the all_equal
	pCmdUI->SetRadio( all_equal);
}

void CGMEView::ChangeNamePosition( int p_val)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::ChangeNamePosition\r\n"));

	try {
		BeginTransaction();
		POSITION pos = selected.GetHeadPosition();
		while(pos) {
			CGuiObject *obj = selected.GetNext(pos);
			if( obj && obj->mgaFco)
				SetNamePositionVal( obj->mgaFco, p_val);
		}

		CommitTransaction();
	}
	catch(hresult_exception &e) {
		AbortTransaction(e.hr);
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::ChangeNamePosition - Unable to change NamePosition preference value.\r\n"));
	}
}

// 
// code below has to be in sync with the objectinspector's preference.cpp
//
void CGMEView::SetNamePositionVal(CComPtr<IMgaFCO>& p_ccpMgaFCO, int val)
{	
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::SetNamePositionVal\r\n"));

	ASSERT( p_ccpMgaFCO);
	if( !p_ccpMgaFCO) return;

	const CComBSTR bstrRegPath(L"namePosition");
	CString valString;
	valString.Format(_T("%d"), val);
	CComBSTR bstrValue( valString);

	// For a specific object we modify the registry value
	COMTHROW(p_ccpMgaFCO->put_RegistryValue(bstrRegPath,bstrValue));
}

bool CGMEView::GetNamePositionVal(CComPtr<IMgaFCO>& p_ccpMgaFCO, int* p_valRet)
{	
	ASSERT( p_ccpMgaFCO);
	ASSERT( p_valRet);
	if( !p_ccpMgaFCO) return false;

	const CComBSTR bstrRegPath(L"namePosition");
	CString strRegValue;

	// Getting regnode
	CComPtr<IMgaRegNode> ccpMgaRegNode;
	COMTHROW(p_ccpMgaFCO->get_RegistryNode(bstrRegPath,&ccpMgaRegNode));

	// Getting regnode status
	long lRegNodeStatus;
	COMTHROW(ccpMgaRegNode->get_Status(&lRegNodeStatus));


	//Status of definition: 0: this node, -1: in meta, >=1: inherited
	if(lRegNodeStatus==0)
	{
		/* Getting the value of the registry node */

		CComBSTR bstrRegValue;
		COMTHROW(ccpMgaRegNode->get_Value(&bstrRegValue));

		strRegValue=bstrRegValue;

	}
	else if(lRegNodeStatus==-1)
	{
		/* Getting value from meta */

		// Getting Meta
		CComPtr<IMgaMetaFCO> ccpMetaFCO;
		COMTHROW(p_ccpMgaFCO->get_Meta(&ccpMetaFCO));

		// Getting registry value from meta
		CComBSTR bstrRegValue;
		COMTHROW(ccpMetaFCO->get_RegistryValue(bstrRegPath,&bstrRegValue));

		strRegValue=bstrRegValue;
	}
	else if(lRegNodeStatus>=1)
	{
		/* Getting inherited value */

		// Getting ancestor FCO
		CComPtr<IMgaFCO> ccpAncestorFCO;
		COMTHROW(p_ccpMgaFCO->get_DerivedFrom(&ccpAncestorFCO));

		// Getting registry value from the ancestor
		CComBSTR bstrRegValue;
		COMTHROW(ccpAncestorFCO->get_RegistryValue(bstrRegPath,&bstrRegValue));

		strRegValue=bstrRegValue;
	}
	else if(lRegNodeStatus==-2)  // ATTRSTATUS_INVALID - It does happen.
	{
		strRegValue=_T("");
	}
	else
	{
		ASSERT(("Undocumented(and undesired) MGA feature",false));
		CGMEEventLogger::LogGMEEvent(_T("CGMEView::GetNamePositionVal: Undocumented(and undesired) MGA feature\r\n"));
		strRegValue=_T("");
	}

	int val = -1;
	if( _stscanf((LPCTSTR)strRegValue, _T("%d"), &val) != 1 || val < 0 || val > 8)
	{
		*p_valRet = -1;
		return false;
	}

	*p_valRet = val;
	return true;
}

bool CGMEView::AskUserAndDetachIfNeeded( CComPtr<IMgaFCO>& mgaFco)
{
	// check whether dependends of mgaFco exist
	CComPtr<IMgaFCOs> der_objs;
	COMTHROW(mgaFco->get_DerivedObjects( &der_objs));
	long cnt = 0;
	if( der_objs) COMTHROW( der_objs->get_Count( &cnt));
	if( cnt > 0) // if dependents exist should they be deleted?
	{
		bool question_asked = false;
		bool detach_answered = false;
		MGACOLL_ITERATE(IMgaFCO, der_objs) {
			CComPtr<IMgaFCO> one_derived(MGACOLL_ITER);

			VARIANT_BOOL prim_deriv;
			COMTHROW( one_derived->get_IsPrimaryDerived( &prim_deriv));
			if( prim_deriv == VARIANT_TRUE)
			{
				if( !question_asked) // pop up dialog only for the first time
				{
					CComBSTR nm;
					COMTHROW( mgaFco->get_Name( &nm));
					CString msg = _T("There are objects primary derived from: \"");
					msg += nm;
					msg += _T("\". Would you like to delete them as well?\n");
					msg += _T("If you answer 'No' the derived objects will be detached, thus preserved.");

					// this answer will be applied to all deriveds of this fco
					int resp = AfxMessageBox( msg, MB_YESNOCANCEL);
					if( resp == IDCANCEL) COMTHROW(E_MGA_MUST_ABORT);
					else if( resp == IDNO) detach_answered = true;
					
					question_asked = true;
				}

				// if detach and preserve selected by the user:
				if( detach_answered)
					COMTHROW( one_derived->DetachFromArcheType());
			}
		}MGACOLL_ITERATE_END;

		return detach_answered; // refresh needed for GMEActiveBrowser?
	}
	return false;
}

// called when VK_RETURN pressed on a model in a view
void CGMEView::OnShowSelectedModel()
{
	if(selected.GetCount()) {
		GMEEVENTLOG_GUIOBJS(selected);
		CComPtr<IMgaFCOs> coll;
		GetDocument()->CreateFcoList( &selected, coll, this);

		long cnt = 0;
		try {
			BeginTransaction(TRANSACTION_READ_ONLY);
			if( coll) COMTHROW( coll->get_Count( &cnt));
			CommitTransaction();
		}
		catch(hresult_exception e) {
			AbortTransaction(e.hr);
		}

		for( long i = 1; i <= cnt; ++i) {
			try {
				BeginTransaction(TRANSACTION_READ_ONLY);
				CComPtr<IMgaFCO> fco;
				COMTHROW( coll->get_Item( i, &fco));
				objtype_enum ot = OBJTYPE_ATOM;
				if( fco) COMTHROW( fco->get_ObjType( &ot));
				CommitTransaction();

				if( fco && ot == OBJTYPE_MODEL)
				{
					CComPtr<IMgaModel> cm;
					COMTHROW( fco.QueryInterface( &cm));
					if( cm) ShowModel( cm);
				}
				else if( fco && ot == OBJTYPE_REFERENCE) 
				{
					CComPtr<IMgaModel> next_mod;
					CComPtr<IMgaFCO> next_fco;
					CComBSTR         special_case_id_of_next_fco; // next_fco might sit in a folder, we will
					                                              // focus on it in the treebrowser in this case
					CComQIPtr<IMgaReference> rf( fco);
					try {
						BeginTransaction(TRANSACTION_READ_ONLY);
						COMTHROW( rf->get_Referred( &next_fco));
						objtype_enum ot = OBJTYPE_ATOM;
						if( next_fco) 
						{
							COMTHROW( next_fco->get_ObjType( &ot));
							if( ot == OBJTYPE_MODEL)
							{
								COMTHROW( next_fco.QueryInterface( &next_mod));
							}
							else
							{
								CComPtr<IMgaObject> parent;
								COMTHROW( next_fco->GetParent( &parent, NULL));
								
								if( parent)
								{
									HRESULT hr = parent.QueryInterface( &next_mod);
									if( FAILED( hr)) // next_fco is sitting in a folder
										COMTHROW( next_fco->get_ID( &special_case_id_of_next_fco));
								}
							}
						}
						
						CommitTransaction();
					}
					catch(hresult_exception e) {
						AbortTransaction(e.hr);
						next_mod = 0;
					}

					if( special_case_id_of_next_fco.Length() > 0)
					{
						CGMEConsole::theInstance->Message( _T("Reference target is child of a folder, thus it is shown in the TreeBrowser only."), MSG_INFO);
						CGMEBrowser::theInstance->FocusItem( special_case_id_of_next_fco);
					}
					else if( next_fco && next_mod) ShowModel( next_mod);
				}
			}
			catch(hresult_exception e) {
				AbortTransaction(e.hr);
			}
		}
	}
}

void CGMEView::OnRename()
{
	CGMEObjectInspector::theInstance->m_ObjectInspector.EditName();
}

// called when VK_B + CTRL is pressed
void CGMEView::OnFocusBrowser()
{
	OnCntxLocate();
}

// called when VK_I + CTRL is pressed
void CGMEView::OnFocusInspector()
{
	HWND hwnd = CGMEObjectInspector::theInstance->GetSafeHwnd();
	if( hwnd) ::SetFocus( hwnd);
}

// called when TAB is pressed
void CGMEView::OnCycleAspect()
{
	ASSERT( currentAspect);
	ASSERT( guiMeta);
	if( !currentAspect || !guiMeta) return;
	int aspNum = guiMeta->aspects.GetCount();
	ASSERT( aspNum >= 1);
	if( aspNum <= 1) return; // if 1 aspect no reason for continuing

	ChangeAspect( (currentAspect->index + 1) % aspNum);
	// statement above changes the currentAspect->index to the new value
	if( CMainFrame::theInstance)
		CMainFrame::theInstance->ChangePartBrowserAspect( currentAspect->index);
}

// called when TAB is pressed
void CGMEView::OnCycleAspectBackwards()
{
	ASSERT( currentAspect);
	ASSERT( guiMeta);
	if( !currentAspect || !guiMeta) return;
	int aspNum = guiMeta->aspects.GetCount();
	ASSERT( aspNum >= 1);
	if( aspNum <= 1) return; // if 1 aspect no reason for continuing

	ChangeAspect( (currentAspect->index - 1 + aspNum) % aspNum);
	// statement above changes the currentAspect->index to the new value
	if( CMainFrame::theInstance)
		CMainFrame::theInstance->ChangePartBrowserAspect( currentAspect->index);
}

// called when '`' is pressed (above the TAB key)
void CGMEView::OnCycleAllAspects()
{
	if( CMainFrame::theInstance)
		CMainFrame::theInstance->CyclePartBrowserAspect();
}

void CGMEView::OnHistoryBack()
{
	GetDocument()->back();
}

void CGMEView::OnHistoryForw()
{
	GetDocument()->forw();
}

void CGMEView::OnKeyConnect()
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEView::OnKeyConnect in ")+path+name+_T("\r\n"));
	if (lastObject) {
		CGuiPort *port = lastPort;
		if(connSrc == 0) {
			connSrc = lastObject;
			connSrcPort = port;
			connSrcHotSide = GME_CENTER;
			tmpConnectMode = true;
			SetCursor(autoconnect2Cursor);
			Invalidate();
		}
		else {
			Connect(connSrc, connSrcPort, connSrcHotSide, lastObject, port, GME_CENTER, ::GetKeyState(VK_SHIFT) < 0);
			ClearConnSpecs();
			tmpConnectMode = false;
			SetCursor(editCursor);
		}
		ShowCursor(TRUE);
	}
}

void CGMEView::OnKeyCycleObjInspectorFrwd()
{
	CGMEObjectInspector::theInstance->CyclePanel( VARIANT_TRUE);
}

void CGMEView::OnKeyCycleObjInspectorBkwd()
{
	CGMEObjectInspector::theInstance->CyclePanel( VARIANT_FALSE);
}

void CGMEView::OnViewMultiUserShowObjectOwner()
{
	theApp.mgaProject->SourceControlObjectOwner( currentModId);
}

void CGMEView::OnUpdateViewMultiUserShowObjectOwner( CCmdUI* pCmdUI)
{
	pCmdUI->Enable( theApp.isMultiUserProj());
}


void CGMEView::OnViewShowconnectedportsonly()
{
	showConnectedPortsOnly = !showConnectedPortsOnly;
	GetDocument()->ResetAllViews();
}

void CGMEView::OnUpdateViewShowconnectedportsonly(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(showConnectedPortsOnly);
}

LRESULT CGMEView::OnDisplayChange(WPARAM wParam, LPARAM lParam)
{
	DeleteOffScreen();
	return TRUE;
}

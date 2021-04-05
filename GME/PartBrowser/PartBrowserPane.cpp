// PartBrowserPane.cpp : implementation file
//

#include "stdafx.h"
#include "PartBrowserPane.h"
#include "PartBrowserPaneFrame.h"
#include "..\Annotator\AnnotationDefs.h"
#include "..\GME\GMEOLEData.h"
#include <algorithm>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// from GME
typedef enum { GME_NAME_FONT = 0, GME_PORTNAME_FONT, GME_CONNLABEL_FONT, GME_FONT_KIND_NUM } GMEFontKind;
static int  fontSizes[GME_FONT_KIND_NUM]	= { 18, 15, 12 };
#define GME_DEFAULT_DECORATOR		"MGA.BoxDecorator"
#define DECORATOR_PREF				"decorator"
typedef CTypedPtrList<CPtrList, CRect *>	CRectList;

/////////////////////////////////////////////////////////////////////////////
// CPartBrowserPane

Gdiplus::SmoothingMode		CPartBrowserPane::m_eEdgeAntiAlias = Gdiplus::SmoothingModeHighQuality;		// Edge smoothing mode
Gdiplus::TextRenderingHint	CPartBrowserPane::m_eFontAntiAlias = Gdiplus::TextRenderingHintAntiAlias;	// Text renndering hint mode

CPartBrowserPane::CPartBrowserPane():
	mgaProject(NULL),
	mgaMetaModel(NULL),
	currentAspectIndex(-1),
	omitPaintMessages(true)
{
	backgroundColor = ::GetSysColor(COLOR_APPWORKSPACE);
	txtMetricFont.CreateFont(fontSizes[GME_NAME_FONT], 0, 0, 0, FW_NORMAL, false, 0, 0, ANSI_CHARSET,
							 OUT_DEVICE_PRECIS, CLIP_DEFAULT_PRECIS,
							 PROOF_QUALITY, FF_SWISS, _T("Arial"));
	HDC hdc = ::GetDC(NULL);
	logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
	::ReleaseDC(NULL, hdc);
}

CPartBrowserPane::~CPartBrowserPane()
{
	txtMetricFont.DeleteObject();
}

CComBSTR CPartBrowserPane::GetDecoratorProgId(IMgaMetaFCO* metaFCO)
{
	CComBSTR pathBstr(DECORATOR_PREF);
	CComBSTR bstrVal;
	COMTHROW(metaFCO->get_RegistryValue(pathBstr,&bstrVal));
	if (bstrVal.Length() <= 0)
		bstrVal = GME_DEFAULT_DECORATOR;
	return bstrVal;
}

bool CPartBrowserPane::IsPartDisplayable(CComPtr<IMgaMetaPart> metaPart)
{
	ASSERT(metaPart != NULL);
	VARIANT_BOOL vb_primary;
	COMTHROW(metaPart->get_IsPrimary(&vb_primary));
	CComPtr<IMgaMetaRole> mmRole;
	COMTHROW(metaPart->get_Role(&mmRole));
	CComPtr<IMgaMetaFCO> mFco;
	COMTHROW(mmRole->get_Kind(&mFco));
	objtype_enum oType;
	COMTHROW(mFco->get_ObjType(&oType));

	if (vb_primary != VARIANT_FALSE &&
		(oType == OBJTYPE_MODEL ||
		 oType == OBJTYPE_REFERENCE ||
		 oType != OBJTYPE_CONNECTION))
	{
		return true;
	}
	return false;
}

bool CPartBrowserPane::FindObject(CPoint &pt, const PartWithDecorator*& pdt)
{
	if (currentAspectIndex < 0 || pdts.size() <= 0)
		return NULL;

	try {
		const std::vector<PartWithDecorator>& pdtv = pdts[currentAspectIndex];
		// calculate the maximum size
		for (std::vector<PartWithDecorator>::const_iterator ii = pdtv.begin(); ii != pdtv.end(); ++ii) {
			ASSERT((*ii).decorator != NULL);
			long x1 = 0; long y1 = 0;
			long x2 = 0; long y2 = 0;
			COMTHROW((*ii).decorator->GetLocation(&x1, &y1, &x2, &y2));
			CRect rc (x1, y1, x2, y2);
			if (rc.PtInRect(pt)) {
				pdt = &(*ii);
				return true;
			}
		}
	}
	catch (hresult_exception&) {
	}

	return false;
}

void CPartBrowserPane::CreateDecorators(CComPtr<IMgaMetaParts> metaParts)
{
	std::vector<PartWithDecorator> pdt;
	try {
		CComPtr<IMgaMetaPart> metaPart;
		MGACOLL_ITERATE(IMgaMetaPart, metaParts) {
			metaPart = MGACOLL_ITER;
			if (IsPartDisplayable(metaPart)) {
				PartWithDecorator tuple;
				tuple.part = metaPart;

				IMgaMetaRolePtr mmRole = metaPart->Role;
				IMgaMetaFCOPtr mFco = mmRole->Kind;
				IMgaDecoratorPtr decorator;
				IMgaElementDecoratorPtr newDecorator;
				CComBSTR decoratorProgId = GetDecoratorProgId(mFco);

				CString nameString;
				_bstr_t bstrKindName = mFco->Name;
				_bstr_t bstrRoleName = mmRole->Name;
				if (bstrKindName == bstrRoleName) {
					_bstr_t bstrDisplayedName = mFco->DisplayedName;
					nameString = (const wchar_t*)bstrDisplayedName;
				}
				else {
					nameString = (const wchar_t*)bstrRoleName;
				}
				tuple.name = nameString;

				HRESULT hres = newDecorator.CreateInstance((LPCWSTR)decoratorProgId);
				if (FAILED(hres) && hres != CO_E_CLASSSTRING) {	// might be an old decorator
					hres = decorator.CreateInstance(PutInBstr(decoratorProgId));
				}
				if (FAILED(hres)) {	// fall back to default decorator
					COMTHROW(newDecorator.CreateInstance(GME_DEFAULT_DECORATOR));
				}

				if (newDecorator) {
					tuple.newDecorator = newDecorator;
					// KMS: this is a workaround for a bug in many decorators: QI(IID_IMgaDecorator) returns E_NOINTERFACE
					// (Fixed in MetaGME and UML in r1367)
					decorator = (IMgaDecorator*)(IMgaElementDecorator*)newDecorator;
					newDecorator->__InitializeEx(mgaProject, metaPart, NULL, NULL /*decorEventSinkIface*/, (ULONGLONG)m_hWnd);
				} else {
					decorator->__Initialize(mgaProject, metaPart, NULL);
				}

				long sx, sy;
				decorator->__GetPreferredSize(&sx, &sy);
				decorator->__SetLocation(0, 0, sx, sy);
				tuple.decorator = decorator;

				pdt.emplace_back(std::move(tuple));
			}
		}
		MGACOLL_ITERATE_END;
	}
	catch (hresult_exception& e) {
		_bstr_t error;
		GetErrorInfo(e.hr, error.GetAddress());
		error = L"Part Browser decorator error: " + error;
		CGMEDataSource::get_GME(CComObjPtr<IMgaProject>(mgaProject))->ConsoleMessage(error, MSG_ERROR);
		return;
	}
	catch (_com_error& e) {
		CGMEDataSource::get_GME(CComObjPtr<IMgaProject>(mgaProject))->ConsoleMessage(_bstr_t(L"Couldn't initialize decorator: " + e.Description()), MSG_ERROR);
		return;
	}
	std::sort(pdt.begin(), pdt.end(), [](const PartWithDecorator& a, const PartWithDecorator& b){ return a.name < b.name; });
	pdts.push_back(std::move(pdt));
}

void CPartBrowserPane::DestroyDecorators(void)
{
	for (std::vector<std::vector<PartWithDecorator> >::iterator ii = pdts.begin(); ii != pdts.end(); ++ii) {
		for (std::vector<PartWithDecorator>::iterator jj = (*ii).begin(); jj != (*ii).end(); ++jj) {
			(*jj).decorator->Destroy();
			if ((*jj).newDecorator) {
				(*jj).newDecorator.Release();
				(*jj).newDecorator = NULL;
			}
			(*jj).decorator.Release();
			(*jj).decorator = NULL;
			(*jj).part.Release();
		}
		(*ii).clear();
	}
	pdts.clear();
}

void CPartBrowserPane::Resize(CRect r)
{
	if (!mgaMetaModel || currentAspectIndex < 0 || pdts.size() <= 0 || currentAspectIndex >= (int)pdts.size())
		return;

	r.bottom = MulDiv(r.bottom, 96, logPixelsY);
	r.top = MulDiv(r.top, 96, logPixelsY);
	r.left = MulDiv(r.left, 96, logPixelsY);
	r.right = MulDiv(r.right, 96, logPixelsY);

	bool oldOmitPaintMessages = omitPaintMessages;
	omitPaintMessages = true;
	ChangeAspect(currentAspectIndex);
	omitPaintMessages = oldOmitPaintMessages;

	const int leftMargin = 10;
	const int rightMargin = 10;
	const int topMargin = 10;
	const int bottomMargin = 30;

	const int xPadding = 10;
	const int yPadding = 30;

	// set the location
	int objColumn = 0;
	int leftStartPos = leftMargin + maxSize.cx / 2;
	int rowHeight = 0;

	// calculate the actual offset
	int objNumInRow = (r.Width() - leftMargin - rightMargin + xPadding) / (maxSize.cx + xPadding);
	if (objNumInRow < 1) {
		objNumInRow = 1;
		leftStartPos = r.Width() / 2 - 3;
	}

	// the pt contains the x center point, and y top point
	CPoint pt = CPoint(leftStartPos, topMargin);

	try {
		const std::vector<PartWithDecorator>& pdtv = pdts[currentAspectIndex];
		// calculate the maximum size
		for (std::vector<PartWithDecorator>::const_iterator ii = pdtv.begin(); ii != pdtv.end(); ++ii) {
			ASSERT((*ii).decorator != NULL);
			long sizeX = 0;
			long sizeY = 0;
			COMTHROW((*ii).decorator->GetPreferredSize(&sizeX, &sizeY));
			CSize size(sizeX, sizeY);

			// calculate the row height
			if (objColumn == 0) {
				rowHeight = size.cy;

				std::vector<PartWithDecorator>::const_iterator iibak = ii;
				for (std::vector<PartWithDecorator>::const_iterator jj = ++ii; jj != pdtv.end() && (objColumn + 1) < objNumInRow; ++jj) {
					ASSERT((*jj).decorator != NULL);
					long sizeX2 = 0;
					long sizeY2 = 0;
					COMTHROW((*jj).decorator->GetPreferredSize(&sizeX2, &sizeY2));
					CSize size2 (sizeX2, sizeY2);

					if (rowHeight < size2.cy)
						rowHeight = size2.cy;

					++objColumn;
				}
				ii = iibak;

				objColumn = 0;
				pt.y += rowHeight / 2;
			}

			// set the location (based on center: pt is the center)
			COMTHROW((*ii).decorator->SetLocation(pt.x - (size.cx - size.cx / 2), pt.y - (size.cy - size.cy / 2), pt.x + size.cx / 2, pt.y + size.cy / 2));

			if (++objColumn >= objNumInRow) {
				objColumn = 0;
				pt.x = leftStartPos;
				pt.y += rowHeight / 2 + yPadding;
			} else {
				pt.x += maxSize.cx + xPadding;
			}
		}
	}
	catch (hresult_exception&) {
	}

	omitPaintMessages = false;

	// if no object in this row
	if (objColumn == 0)
		pt.y -= yPadding;
	else
		pt.y += rowHeight / 2;

	pt.y += bottomMargin;

	CPartBrowserPaneFrame* parent = (CPartBrowserPaneFrame*)GetParent();
	ASSERT(parent != NULL);

	parent->SetLogicalHeight(MulDiv(pt.y, logPixelsY, 96));
	parent->SetPageHeight(MulDiv(r.Height(), logPixelsY, 96));
	parent->SetScrollBar();

	Invalidate();
}

void CPartBrowserPane::SetCurrentProject(CComPtr<IMgaProject> project)
{
	if (mgaProject != NULL)
		mgaProject.Release();
	mgaProject = project;
}

void CPartBrowserPane::SetMetaModel(CComPtr<IMgaMetaModel> meta)
{
	CComPtr<IMgaRegistrar> registrar;
	COMTHROW(registrar.CoCreateInstance(CComBSTR(L"Mga.MgaRegistrar")));
	ASSERT(registrar != NULL);
	edgesmoothmode_enum edgeSmoothMode;
	COMTHROW(registrar->get_EdgeSmoothMode(REGACCESS_USER, &edgeSmoothMode));
	m_eEdgeAntiAlias = (Gdiplus::SmoothingMode)edgeSmoothMode;
	fontsmoothmode_enum fontSmoothMode;
	COMTHROW(registrar->get_FontSmoothMode(REGACCESS_USER, &fontSmoothMode));
	m_eFontAntiAlias = (Gdiplus::TextRenderingHint)fontSmoothMode;

	if (mgaMetaModel != NULL)
		mgaMetaModel.Release();
	mgaMetaModel = meta;

	DestroyDecorators();
	// Store parts and create & initialize decorators
	// FIXME: don't do this if PartBrowser is not displayed, it is expensive
	// FIXME: lazily instantiate when aspect is changed
	if (meta) {
		CComPtr<IMgaMetaAspects> mmAspects;
		COMTHROW(mgaMetaModel->get_Aspects(&mmAspects));
		long aspect_count = 0;
		COMTHROW(mmAspects->get_Count(&aspect_count));
		CComPtr<IMgaMetaAspect> mmAspect;
		MGACOLL_ITERATE(IMgaMetaAspect, mmAspects) {
			mmAspect = MGACOLL_ITER;

			CComPtr<IMgaMetaParts> metaParts;
			COMTHROW(mmAspect->get_Parts(&metaParts));
			CreateDecorators(metaParts);
		}
		MGACOLL_ITERATE_END;
	} else
		Invalidate(TRUE);
}

void CPartBrowserPane::SetBgColor(COLORREF bgColor)
{
	backgroundColor = bgColor;
}

void CPartBrowserPane::ChangeAspect(int index)
{
	currentAspectIndex = index;
	if (currentAspectIndex < 0 || pdts.size() <= 0)
		return;

	// create a DC for text metric
	CDC textMetric;
	textMetric.CreateCompatibleDC(NULL);
	CFont* oldCFont = textMetric.SelectObject(&txtMetricFont);
	HFONT oldFont = (HFONT)oldCFont->GetSafeHandle();

	// just to be safe
	maxSize.cx = 30;
	maxSize.cy = 10;

	try {
		const std::vector<PartWithDecorator>& pdtv = pdts[currentAspectIndex];
		// calculate the maximum size
		for (std::vector<PartWithDecorator>::const_iterator ii = pdtv.begin(); ii != pdtv.end(); ++ii) {
			ASSERT((*ii).part != NULL);
			ASSERT((*ii).decorator != NULL);

			long sizeX = 0;
			long sizeY = 0;
			COMTHROW((*ii).decorator->GetPreferredSize(&sizeX, &sizeY));
			CSize size(sizeX, sizeY);

			if (maxSize.cx < size.cx)
				maxSize.cx = size.cx;

			if (maxSize.cy < size.cy)
				maxSize.cy = size.cy;

			int text_cx = textMetric.GetTextExtent((*ii).name).cx;
			if (maxSize.cx < text_cx)
				maxSize.cx = text_cx;
		}
	}
	catch (hresult_exception&) {
	}

	CPartBrowserPaneFrame* parent = (CPartBrowserPaneFrame*)GetParent();
	ASSERT(parent != NULL);
	ASSERT(dynamic_cast<CPartBrowserPaneFrame*>(GetParent()));

	Invalidate();

	CFont* oldCFont2 = CFont::FromHandle(oldFont);
	textMetric.SelectObject(oldCFont2);
}


BEGIN_MESSAGE_MAP(CPartBrowserPane, CWnd)
	//{{AFX_MSG_MAP(CPartBrowserPane)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPartBrowserPane message handlers

void CPartBrowserPane::OnPaint()
{
	if (omitPaintMessages) {
		CWnd::OnPaint();
		return;
	}

	CPartBrowserPaneFrame* parent = (CPartBrowserPaneFrame*)GetParent();
	ASSERT(parent != NULL);

	CPaintDC dc(this); // device context for painting

	dc.SetWindowOrg(0, MulDiv(parent->GetScrollPosition(), 96, logPixelsY));

	dc.SetMapMode(MM_ISOTROPIC);
	dc.SetWindowExt(100, 100);
	int curzoom = MulDiv(100, logPixelsY, 96);
	dc.SetViewportExt(curzoom, curzoom);


	if (pdts.size() > 0 && currentAspectIndex >= 0) {
		try {
			const std::vector<PartWithDecorator>& pdtv = pdts[currentAspectIndex];
			for (std::vector<PartWithDecorator>::const_iterator ii = pdtv.begin(); ii != pdtv.end(); ++ii) {
				if ((*ii).newDecorator) {
					// GME-339
					Gdiplus::Graphics gdipGraphics(dc.m_hDC);
					gdipGraphics.SetPageUnit(Gdiplus::UnitPixel);
					gdipGraphics.SetSmoothingMode(m_eEdgeAntiAlias);
					gdipGraphics.SetTextRenderingHint(m_eFontAntiAlias);
					COMTHROW((*ii).newDecorator->DrawEx((ULONG)dc.m_hDC, (ULONGLONG)(&gdipGraphics)));
				} else {
					COMTHROW((*ii).decorator->Draw((ULONG)dc.m_hDC));
				}
			}
		}
		catch (hresult_exception&) {
		}
	}

	// Do not call CWnd::OnPaint() for painting messages
}

BOOL CPartBrowserPane::OnEraseBkgnd(CDC* pDC)
{
	RECT r;
	GetClientRect(&r);
	pDC->FillSolidRect(&r, backgroundColor);
	return TRUE;
	
//	return CWnd::OnEraseBkgnd(pDC);
}

void CPartBrowserPane::OnLButtonDown(UINT nFlags, CPoint point) 
{
//	CGMEEventLogger::LogGMEEvent("CPartBrowserPane::OnLButtonDown\r\n");
	
	// Needed to flush the Object Inspector and the Browser (JIRA #GME-52)
	SetFocus();

	CPartBrowserPaneFrame* parent = (CPartBrowserPaneFrame*)GetParent();
	ASSERT(parent != NULL);

	point.y += parent->GetScrollPosition ();

	const PartWithDecorator* pdt;

	point.x = MulDiv(point.x, 96, logPixelsY);
	point.y = MulDiv(point.y, 96, logPixelsY);
	bool found = FindObject(point, pdt);

	if (found) {
//		CGMEEventLogger::LogGMEEvent("    LButton over " + guiObj->GetName() + "\r\n");

		CRect rectAwake = CRect(point.x, point.y, point.x + 1, point.y + 1);
		rectAwake.InflateRect(3, 3);

		long x1 = 0; long y1 = 0;
		long x2 = 0; long y2 = 0;
		COMTHROW(pdt->decorator->GetLocation(&x1, &y1, &x2, &y2));
		CRect partRect (x1, y1, x2, y2);
		CPoint ptClickOffset(point.x - partRect.left, point.y - partRect.top);

		CRectList rects;
		rects.AddTail(&partRect);

		CRectList dummyAnnList;
		CGMEDataDescriptor desc(rects, dummyAnnList, point, ptClickOffset);
//		CGMEDataDescriptor::destructList(rects);

		CGMEDataSource dataSource(mgaProject);
		CComPtr<IMgaMetaRole> mmRole;
		COMTHROW(pdt->part->get_Role(&mmRole));
		dataSource.SetMetaRole(mmRole);
		dataSource.CacheDescriptor(&desc);
		DROPEFFECT de = dataSource.DoDragDrop(DROPEFFECT_COPY, &rectAwake);
	}

	CWnd::OnLButtonDown(nFlags, point);
}

#include "stdafx.h"

#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "GMEApp.h"
#include "GMEstd.h"
#include "GMEOLEData.h"
#include "PendingObjectPosRequest.h"
#include "GMEView.h"
#include "GraphicsUtil.h"
#include "GuiMeta.h"
#include "GuiObject.h"
#include "ModelGrid.h"
#include "DecoratorEventSink.h"
#include "AnnotatorEventSink.h"

#include "AutoRoute/AutoRouterGraph.h"
#include "AutoRoute/AutoRouter.h"
#include "AutoRoute/AutoRouterPath.h"

#include "GMEEventLogger.h"
#include "MainFrm.h"


CModelGrid modelGrid;

/////////////////////////////// Helper functions /////////////////////////////////

static void SetSize(CRect& location, CSize s)
{
	location.left = max(GME_GRID_SIZE, location.left / GME_GRID_SIZE * GME_GRID_SIZE);
	location.top = max(GME_GRID_SIZE, location.top / GME_GRID_SIZE * GME_GRID_SIZE);
	location.right = location.left + s.cx;
	location.bottom = location.top + s.cy;
}

void SetLocation(CRect& location, CPoint pt)
{
	CSize s = location.Size();
	location.top = pt.y;
	location.left = pt.x;
	SetSize(location, s);
}

//////////////////////////////////// CGuiAspect /////////////////////////////

////////////////////////////////////
// Non-virtual methods of CGuiAspect
////////////////////////////////////
CGuiAspect::CGuiAspect(CGuiMetaAspect* meta, CGuiObject* p, int ind, int pind, CComPtr<IMgaDecorator>& decor,
					   CComPtr<IMgaElementDecorator>& newDecor, CComObjPtr<CDecoratorEventSink>& decorEventSink):
	guiMeta(meta),
	parent(p),
	index(ind),
	parentIndex(pind),
	decorator(decor),
	newDecorator(newDecor),
	decoratorEventSink(decorEventSink),
	routerBox(0),
	routerNameBox(0)
{
	try {
		COMTHROW(decor->GetFeatures(&features));
	}
	catch (hresult_exception &) {
		features = 0UL;
	}
	name = meta->name;
	if (parent->IsReal()) {
		InitPorts();
	}

	if (features & F_ANIMATION) {
		parent->IncrementAnimRefCnt();
	}
}

CGuiAspect::~CGuiAspect()
{
	if (newDecorator) {
		newDecorator->Destroy();
		newDecorator.Release();
		newDecorator = NULL;
	} else {
		decorator->Destroy();
	}
	decorator.Release();
	decorator = NULL;
	if (decoratorEventSink != NULL) {
		decoratorEventSink.Release();
		decoratorEventSink = NULL;
	}
	POSITION pos = ports.GetHeadPosition();
	while(pos) {
		delete ports.GetNext(pos);
	}
	if (features & F_ANIMATION) {
		parent->DecrementAnimRefCnt();
	}
}

void CGuiAspect::InitPorts()
{
	CComPtr<IMgaFCOs> portFcos;
	COMTHROW(decorator->GetPorts(&portFcos));
	if (portFcos) {
		CComPtr<IMgaFCO> fco;
		MGACOLL_ITERATE(IMgaFCO,portFcos) {
			fco = MGACOLL_ITER;
			CGuiPort *p = new CGuiPort(this,fco);
			ports.AddTail(p);
		}
		MGACOLL_ITERATE_END;
	}
	CGuiPort *p = new CGuiPort(this,parent->mgaFco);
	ports.AddTail(p);

}

void CGuiAspect::SetLocation(const CRect& location)
{
	loc = location; 
	
	try {
		COMTHROW(decorator->SetLocation(location.left, location.top, location.right, location.bottom));
		long sx, sy, ex, ey;
		COMTHROW(decorator->GetLabelLocation(&sx, &sy, &ex, &ey));
		
		/*
		sx = min(sx, location.right-1);
		ex = max(ex, location.left+1);
		sy = min(sy, location.bottom-1);
		ey = max(ey, location.top+1);
		*/
		if (sx != 0 && ex != 0 && sy != 0 && ey != 0) {
			sx = min(sx, location.right+1);
			ex = max(ex, location.left-1);
			sy = min(sy, location.bottom+1);
			ey = max(ey, location.top-1);
		}
		nameLoc.SetRect(sx, sy, ex, ey);
	} catch (hresult_exception &) {
			ASSERT(false);
	}
}
////////////////////////////////////
// Static methods of CGuiAspect
////////////////////////////////////

////////////////////////////////////
// Virtual methods of CGuiAspect
////////////////////////////////////


////////////////////////////////// CGuiPort /////////////////////////////

////////////////////////////////////
// Non-virtual methods of CGuiPort
////////////////////////////////////
CGuiPort::CGuiPort(CGuiAspect* asp, CComPtr<IMgaFCO> fco) : parent(asp), mgaFco(fco)
{
	CComBSTR bstr;
	COMTHROW(fco->get_Name(&bstr));
	CopyTo(bstr,name);

	bstr.Empty();
	COMTHROW(fco->get_ID(&bstr));
	CopyTo(bstr,id);

	COMTHROW(fco->get_Meta(&metaFco));

	COMTHROW(fco->get_MetaRole(&metaRole));

	if (!IsRealPort()) {
		ReadARPreferences();
	}

}

bool CGuiPort::GetPreference(CString &val, CString path)
{
	VERIFY(mgaFco);
	CComBSTR pathBstr;
	CopyTo(path, pathBstr);
	CComBSTR bstrVal;
	COMTHROW(mgaFco->get_RegistryValue(pathBstr,&bstrVal));
	CopyTo(bstrVal, val);
	return !val.IsEmpty();
}



void CGuiPort::ReadARPreferences()
{
	CString val;
	if (GetPreference(val, AUTOROUTER_PREF)) {
		autorouterPrefs[GME_START_NORTH] = (val.Find(_T("N")) != -1);
		autorouterPrefs[GME_START_EAST] = (val.Find(_T("E")) != -1);
		autorouterPrefs[GME_START_SOUTH] = (val.Find(_T("S")) != -1);
		autorouterPrefs[GME_START_WEST] = (val.Find(_T("W")) != -1);
		autorouterPrefs[GME_END_NORTH] = (val.Find(_T("n")) != -1);
		autorouterPrefs[GME_END_EAST] = (val.Find(_T("e")) != -1);
		autorouterPrefs[GME_END_SOUTH] = (val.Find(_T("s")) != -1);
		autorouterPrefs[GME_END_WEST] = (val.Find(_T("w")) != -1);
	}
	else {
		if (parent->GetParent()->dynamic_cast_CGuiCompound() != NULL) {
			autorouterPrefs[GME_START_NORTH] = true;
			autorouterPrefs[GME_START_EAST] = false;
			autorouterPrefs[GME_START_SOUTH] = true;
			autorouterPrefs[GME_START_WEST] = false;
			autorouterPrefs[GME_END_NORTH] = true;
			autorouterPrefs[GME_END_EAST] = false;
			autorouterPrefs[GME_END_SOUTH] = true;
			autorouterPrefs[GME_END_WEST] = false;
		} else {
			autorouterPrefs[GME_START_NORTH] = true;
			autorouterPrefs[GME_START_EAST] = true;
			autorouterPrefs[GME_START_SOUTH] = true;
			autorouterPrefs[GME_START_WEST] = true;
			autorouterPrefs[GME_END_NORTH] = true;
			autorouterPrefs[GME_END_EAST] = true;
			autorouterPrefs[GME_END_SOUTH] = true;
			autorouterPrefs[GME_END_WEST] = true;
		}
	}
}

bool CGuiPort::GetARPref(int dir)
{
	if (IsRealPort()) {
		CRect parentLoc = parent->GetLocation();
		if (GetLocation().CenterPoint().x < (parentLoc.Size().cx / 2)) {
			autorouterPrefs[GME_START_NORTH] = false;
			autorouterPrefs[GME_START_EAST] = false;
			autorouterPrefs[GME_START_SOUTH] = false;
			autorouterPrefs[GME_START_WEST] = true;
			autorouterPrefs[GME_END_NORTH] = false;
			autorouterPrefs[GME_END_EAST] = false;
			autorouterPrefs[GME_END_SOUTH] = false;
			autorouterPrefs[GME_END_WEST] = true;
		} else {
			autorouterPrefs[GME_START_NORTH] = false;
			autorouterPrefs[GME_START_EAST] = true;
			autorouterPrefs[GME_START_SOUTH] = false;
			autorouterPrefs[GME_START_WEST] = false;
			autorouterPrefs[GME_END_NORTH] = false;
			autorouterPrefs[GME_END_EAST] = true;
			autorouterPrefs[GME_END_SOUTH] = false;
			autorouterPrefs[GME_END_WEST] = false;

		}
	}
	return autorouterPrefs[dir];
}


bool CGuiPort::IsVisible()			// called on a tmp obj that only exists to get this info
{
	bool visible = true;
	VERIFY(parent);
	VERIFY(parent->parent);
	CComPtr<IMgaFCO> fco = parent->parent->mgaFco;
	CComBSTR bstr = L"GME/ports";
	CComPtr<IMgaRegNode> root;
	COMTHROW(fco->get_RegistryNode(bstr,&root));
	CComPtr<IMgaRegNodes> nodes;

	VARIANT_BOOL vb = VARIANT_TRUE;
	COMTHROW(root->get_SubNodes(vb,&nodes));
	MGACOLL_ITERATE(IMgaRegNode,nodes) {
		CComPtr<IMgaRegNode> reg;
		reg = MGACOLL_ITER;
		CComPtr<IMgaFCO> portFco;
		COMTHROW(reg->get_FCOValue(&portFco));
		if(IsEqualObject(portFco,mgaFco)) {
			CComPtr<IMgaRegNode> subreg;
			CComBSTR path = L"visible";
			COMTHROW(reg->get_SubNodeByName(path,&subreg));
			if(subreg != 0) {
				CComBSTR val;
				COMTHROW(subreg->get_Value(&val));
				CString value;
				CopyTo(val,value);
				int v;
				if(_stscanf((LPCTSTR)value,_T("%d"),&v) > 0)
					visible = v ? true : false;
			}
			break;
		}
	}
	MGACOLL_ITERATE_END;
	return visible;
}

void CGuiPort::SetVisible(bool v)	// called on a tmp obj that only exists to set this info
{
	VERIFY(parent);
	VERIFY(parent->parent);
	CComPtr<IMgaFCO> fco = parent->parent->mgaFco;
	CComBSTR id;
	CComBSTR bstr = L"GME/ports/";
	COMTHROW(mgaFco->get_ID(&id));
	bstr += id;
	CComPtr<IMgaRegNode> reg;
	COMTHROW(fco->get_RegistryNode(bstr,&reg));
	COMTHROW(reg->put_FCOValue(mgaFco));
	CComBSTR path = L"visible";
	CComPtr<IMgaRegNode> subreg;
	COMTHROW(reg->get_SubNodeByName(path,&subreg));
	CComBSTR bstrval = v ? L"1" : L"0";
	COMTHROW(subreg->put_Value(bstrval));
}

CRect CGuiPort::GetLocation()
{
	long sx = 0;
	long sy = 0;
	long ex = 0;
	long ey = 0;
	if (!IsRealPort()) {
		CSize size = parent->GetLocation().Size();
		sx = sy = 0;
		ex = size.cx;
		ey = size.cy;
	}
	else {
		try {
			parent->GetDecorator()->GetPortLocation(mgaFco, &sx, &sy, &ex, &ey);
		}
		catch(hresult_exception& e) {
			if (e.hr != E_DECORATOR_PORTNOTFOUND) {
				throw e;
			}
		}
	}
	return CRect(sx, sy, ex, ey);
}

bool CGuiPort::IsRealPort() {
	return !mgaFco.IsEqualObject(parent->GetParent()->mgaFco);
}

////////////////////////////////////
// Static methods of CGuiPort
////////////////////////////////////

////////////////////////////////////
// Virtual methods of CGuiPort
////////////////////////////////////

////////////////////////////////// CGuiAnnotator /////////////////////////////

AnnotatorDecoratorData::AnnotatorDecoratorData()
{
	decorator = NULL;
	annotatorEventSink = NULL;
}

AnnotatorDecoratorData::AnnotatorDecoratorData(CComPtr<IMgaElementDecorator>& nD, CComObjPtr<CAnnotatorEventSink>& aES,
											   const CRect& loc):
	decorator(nD),
	annotatorEventSink(aES),
	location(loc)
{
}

AnnotatorDecoratorData::~AnnotatorDecoratorData()
{
}


////////////////////////////////// CGuiAnnotator /////////////////////////////

////////////////////////////////////
// Non-virtual methods of CGuiAnnotator
////////////////////////////////////
CGuiAnnotator::CGuiAnnotator(CComPtr<IMgaModel>& pModel, CComPtr<IMgaRegNode>& mRootNode, CGMEView* vw, int numAsp)
{
	model = pModel;
	rootNode = mRootNode;
	numParentAspects = numAsp;
	parentAspect = 0;
	grayedOut = false;
	view = vw;
	special = false;

	for (int i = 0; i < numAsp; i++) {
		decoratorData.push_back(new AnnotatorDecoratorData());
	}

	try {
		CComBSTR bName;
		COMTHROW(rootNode->get_Name(&bName));
		name = bName;

		long st; // if main node status is inherited it means it is a special node
		COMTHROW(rootNode->get_Status( &st));
		if( st > ATTSTATUS_HERE)
			special = true;

		id = view->name + _T(":") + name;		// Fake id

		CComPtr<IMgaRegNode> aspRootNode;
		CComBSTR aspRootName(AN_ASPECTS);
		COMTHROW(rootNode->get_SubNodeByName(aspRootName, &aspRootNode));

		if (aspRootNode) {
			CComPtr<IMgaRegNodes>	aspNodes;
			COMTHROW(aspRootNode->get_SubNodes(VARIANT_TRUE, &aspNodes));
			MGACOLL_ITERATE(IMgaRegNode, aspNodes) {
				CComPtr<IMgaRegNode>	aspNode;
				aspNode = MGACOLL_ITER;
				CComBSTR aspName;
				COMTHROW(aspNode->get_Name(&aspName));
				CString aspNameStr(aspName);
				if (aspNameStr != AN_DEFASPECT) {
					CGuiMetaAspect *guiMetaAsp = view->guiMeta->FindAspect(aspNameStr);
					if (guiMetaAsp) {
						ReadLocation(guiMetaAsp->index, aspNode);
						InitDecorator(guiMetaAsp->index);
					}
				}
			}
			MGACOLL_ITERATE_END;

			CComPtr<IMgaRegNode> defAspNode;
			CComBSTR defAspName(AN_DEFASPECT);
			COMTHROW(aspRootNode->get_SubNodeByName(defAspName, &defAspNode));
			long status;
			COMTHROW(defAspNode->get_Status(&status));
			if (status != ATTSTATUS_UNDEFINED) {
				for (int i = 0; i < numParentAspects; i++) {
					if (decoratorData[i]->decorator == NULL) {
						ReadLocation(i, defAspNode);
						InitDecorator(i);
					}
				}
			}
		}
	}
	catch(hresult_exception &e) {
		for (std::vector<AnnotatorDecoratorData*>::iterator ii = decoratorData.begin(); ii != decoratorData.end(); ++ii) {
			if ((*ii)->decorator != NULL) {
				COMTHROW((*ii)->decorator->Destroy());
				(*ii)->decorator.Release();
				(*ii)->decorator = NULL;
			}
			if ((*ii)->annotatorEventSink != NULL) {
				(*ii)->annotatorEventSink.Release();
				(*ii)->annotatorEventSink = NULL;
			}
			delete (*ii);
		}
		decoratorData.clear();

		numParentAspects = 0;
		parentAspect = 0;
		throw hresult_exception(e.hr);
		return;
	}
}

CGuiAnnotator::~CGuiAnnotator()
{
	for (std::vector<AnnotatorDecoratorData*>::iterator ii = decoratorData.begin(); ii != decoratorData.end(); ++ii) {
		if ((*ii)->decorator != NULL) {
			COMTHROW((*ii)->decorator->Destroy());
			(*ii)->decorator.Release();
			(*ii)->decorator = NULL;
		}
		if ((*ii)->annotatorEventSink != NULL) {
			(*ii)->annotatorEventSink.Release();
			(*ii)->annotatorEventSink = NULL;
		}
		delete (*ii);
	}
	decoratorData.clear();
}

void CGuiAnnotator::InitDecorator(int asp)
{
	try {
		CString progId = AN_DECORATOR_PROGID;
		COMTHROW(decoratorData[asp]->decorator.CoCreateInstance(PutInBstr(progId)));

		CComPtr<IMgaElementDecoratorEvents> annotatorEventSinkIface;
		::CreateComObject(decoratorData[asp]->annotatorEventSink);
		HRESULT hr = ::QueryInterface((IMgaElementDecoratorEvents*)(decoratorData[asp]->annotatorEventSink.p),
									  &annotatorEventSinkIface);
		if (SUCCEEDED(hr)) {
			decoratorData[asp]->annotatorEventSink->SetView(view);
			decoratorData[asp]->annotatorEventSink->SetGuiAnnotator(this);
		}
		CComBSTR param(AN_PARAM_ROOTNODE);
		CComVariant value(rootNode);
		COMTHROW(decoratorData[asp]->decorator->SetParam(param, value));

		COMTHROW(decoratorData[asp]->decorator->InitializeEx(theApp.mgaProject, NULL, NULL, annotatorEventSinkIface, (ULONGLONG)view->m_hWnd));

		long sx, sy;
		COMTHROW(decoratorData[asp]->decorator->GetPreferredSize(&sx, &sy));
		decoratorData[asp]->location.right = decoratorData[asp]->location.left + sx;
		decoratorData[asp]->location.bottom = decoratorData[asp]->location.top + sy;

		COMTHROW(decoratorData[asp]->decorator->SetLocation(decoratorData[asp]->location.left, decoratorData[asp]->location.top,
															decoratorData[asp]->location.right, decoratorData[asp]->location.bottom));
	}
	catch (hresult_exception &) {
		AfxMessageBox(_T("Cannot initialize annotator for annotation: ") + name, MB_OK | MB_ICONSTOP);
		decoratorData[asp]->decorator = NULL;
	}
}

bool CGuiAnnotator::IsVisible(int aspect)
{
	if (aspect < 0) {
		aspect = parentAspect;
	}
	return (decoratorData[aspect]->decorator != NULL);
}

bool CGuiAnnotator::IsResizable(void) const
{
	if (decoratorData[parentAspect]->decorator) {
		try {
			feature_code fc = 0;
			COMTHROW(decoratorData[parentAspect]->decorator->GetFeatures(&fc));
			return ((fc & F_RESIZABLE) != 0);
		}
		catch (hresult_exception &) {
			AfxMessageBox(_T("Error in annotator [method IsResizable()]"));
		}
	}

	return false;
}

void CGuiAnnotator::Draw(HDC pDC, Gdiplus::Graphics* gdip)
{
	ASSERT(sizeof(ULONG) == sizeof(HDC));
	if (decoratorData[parentAspect]->decorator) {
		try {
			COMTHROW(decoratorData[parentAspect]->decorator->DrawEx((ULONG)pDC, (ULONGLONG)gdip));
		}
		catch (hresult_exception &) {
			AfxMessageBox(_T("Error in annotator [method Draw()]"));
		}
	}
}

void CGuiAnnotator::GrayOut(bool set)
{
	if (decoratorData[parentAspect]->decorator) {
		grayedOut = set;
		try {
			COMTHROW(decoratorData[parentAspect]->decorator->SetActive(set ? VARIANT_FALSE : VARIANT_TRUE));
		}
		catch (hresult_exception &) {
		}
	}
}

const CRect& CGuiAnnotator::GetLocation(int aspect)
{
	if (aspect < 0) {
		aspect = parentAspect;
	}
	return decoratorData[aspect]->location;
}

void  CGuiAnnotator::SetLocation(const CRect& toLoc, int aspect, bool doMga)
{
	if (aspect < 0) {
		aspect = parentAspect;
	}

	CRect loc(toLoc);
	if (loc.left < 0) {
		loc.right += -loc.left;
		loc.left = 0;
	}
	if (loc.top < 0) {
		loc.bottom += -loc.top;
		loc.top = 0;
	}

	decoratorData[aspect]->location = loc;
	try {
		COMTHROW(decoratorData[aspect]->decorator->SetLocation(loc.left, loc.top, loc.right, loc.bottom));
	}
	catch (hresult_exception &) {
		AfxMessageBox(_T("Cannot set location of annotation ") + name);
	}

	if (doMga) {
		WriteLocation(aspect);
	}
}

void  CGuiAnnotator::ReadLocation(int aspect, CComPtr<IMgaRegNode>& aspNode)
{
	decoratorData[aspect]->location.left = 0;
	decoratorData[aspect]->location.top = 0;

	try {
		CComBSTR bstr;
		COMTHROW(aspNode->get_Value(&bstr));
		CString str(bstr);
		long lx, ly;
		if(_stscanf(str,_T("%d,%d"), &lx, &ly) == 2) {
			decoratorData[aspect]->location.left = lx;
			decoratorData[aspect]->location.top = ly;
			return;
		}
	}
	catch (hresult_exception &) {
	}
	try {
		CComPtr<IMgaRegNode> rootNode;
		COMTHROW(aspNode->get_ParentNode(&rootNode));
		CComBSTR bstrRoot;
		COMTHROW(rootNode->get_Value(&bstrRoot));
		CString strRoot(bstrRoot);
		long lxr, lyr;
		if(_stscanf(strRoot,_T("%d,%d"), &lxr, &lyr) == 2) {
			decoratorData[aspect]->location.left = lxr;
			decoratorData[aspect]->location.top = lyr;
			return;
		}
	}
	catch (hresult_exception &) {
	}
}

void  CGuiAnnotator::WriteLocation(int aspect)
{
	if (aspect < 0) {
		aspect = parentAspect;
	}

	OLECHAR bbc[40];
	_snwprintf(bbc, 40, OLESTR("%ld,%ld"), decoratorData[aspect]->location.left, decoratorData[aspect]->location.top);
	CComBSTR bb(bbc);


	CComPtr<IMgaRegNode> aspRoot;
	CComBSTR aspRootName(AN_ASPECTS);
	COMTHROW(rootNode->get_SubNodeByName(aspRootName, &aspRoot));

	CComPtr<IMgaRegNode> aspNode;
	CComBSTR aspName(view->guiMeta->FindAspect(aspect)->name);
	COMTHROW(aspRoot->get_SubNodeByName(aspName, &aspNode));

	COMTHROW(aspNode->put_Value(bb));
}

////////////////////////////////////
// Static methods of CGuiAnnotator
////////////////////////////////////
void CGuiAnnotator::SetAspect(CGuiAnnotatorList& list, int asp)
{
	POSITION pos = list.GetHeadPosition();
	while (pos) {
		list.GetNext(pos)->SetAspect(asp);
	}
}

void CGuiAnnotator::GrayOutAnnotations(CGuiAnnotatorList& list, bool set)
{
	POSITION pos = list.GetHeadPosition();
	while (pos) {
		list.GetNext(pos)->GrayOut(set);
	}
}


void CGuiAnnotator::NudgeAnnotations(CGuiAnnotatorList& annotatorList, int right, int down)
{
	CGMEEventLogger::LogGMEEvent(_T("CGuiAnnotator::NudgeAnnotations "));
	GMEEVENTLOG_GUIANNOTATORS(annotatorList);
	ASSERT(right == 0 || down == 0); // cannot nudge diagonally for now
	POSITION pos = annotatorList.GetHeadPosition();
	while(pos) {
		CGuiAnnotator* ann = annotatorList.GetNext(pos);
		VERIFY(ann->IsVisible());
		CRect rect = ann->GetLocation();
		rect.top += down * GME_GRID_SIZE;
		rect.bottom += down * GME_GRID_SIZE;
		rect.left += right * GME_GRID_SIZE;
		rect.right += right * GME_GRID_SIZE;
		ann->SetLocation(rect);
	}
}

void CGuiAnnotator::GetExtent(CGuiAnnotatorList& annotatorList, CRect& rect)
{
	CGuiAnnotator* ann;
	CRect cur1, cur2;
	POSITION pos = annotatorList.GetHeadPosition();
	rect = CRect(0,0,0,0);
	while(pos) {
		ann = annotatorList.GetNext(pos);
		if(ann && ann->IsVisible()) {
			cur2 = ann->GetLocation();
			cur1 = rect;
			rect.UnionRect(&cur1, &cur2);
		}
	}
}

void CGuiAnnotator::GetRectList(CGuiAnnotatorList &annotatorList, CRectList &annRects)
{
	POSITION pos = annotatorList.GetHeadPosition();
	while(pos) {
		CRect* rect = new CRect(annotatorList.GetNext(pos)->GetLocation());
		annRects.AddTail(rect);
	}
}


void CGuiAnnotator::FindUpperLeft(CGuiAnnotatorList& anns, int& left, int& top)
{
	bool start = true;
	POSITION pos = anns.GetHeadPosition();
	CGuiAnnotator* ann;
	CRect r;
	while(pos) {
		ann = anns.GetNext(pos);
		if(ann->IsVisible()) {
			r = ann->GetLocation();
			if(start) {
				left = r.left;
				top = r.top;
				start = false;
			}
			else {
				if(r.left < left)
					left = r.left;
				if(r.top < top)
					top = r.top;
			}
		}
	}
}

void CGuiAnnotator::ShiftAnnotations(CGuiAnnotatorList& annList, CPoint& shiftBy)
{
	CGMEEventLogger::LogGMEEvent(_T("CGuiAnnotator::ShiftAnnotations "));
	GMEEVENTLOG_GUIANNOTATORS(annList);
	POSITION pos = annList.GetHeadPosition();
	while(pos) {
		CGuiAnnotator* ann = annList.GetNext(pos);
		VERIFY(ann->IsVisible());
		CRect newLoc = ann->GetLocation();
		newLoc.OffsetRect(shiftBy);
		ann->SetLocation(newLoc);
	}
}

// static
int CGuiAnnotator::Hide(CComPtr<IMgaRegNode>& mRootNode )
{
	CComBSTR val;
	COMTHROW(mRootNode->get_Value( &val));
	COMTHROW(mRootNode->put_Value( val));
	// thus we made the value of the annotator defined 'HERE'
	// which will decide whether an annotation regnode is virtual (inherited) or not

	CComBSTR bstr(L"1");

	// hidden node set to true
	CComPtr<IMgaRegNode> hideNode;
	CComBSTR hideNm(AN_HIDDEN);
	COMTHROW(mRootNode->get_SubNodeByName( hideNm, &hideNode));
	COMTHROW( hideNode->put_Value( bstr));

	return S_OK;
}

// static
bool CGuiAnnotator::Showable( CComPtr<IMgaRegNode>& mRootNode, CComPtr<IMgaFCO>& baseType )
{   // decides whether an annotation should be displayed in case of a subtype/instance
	// Subtype/instance may have: owned annotations (not present in the ancestor) -> main status = ATTSTATUS_HERE
	//                            inherited annotations                           -> main status = ATTSTATUS_INHERITED1,2,...
	//                            inherited, but modified annotations             -> main status = ATTSTATUS_HERE
	try {
		CComBSTR bName;
		COMTHROW(mRootNode->get_Name(&bName));

		bool hidden_set = false;
		CComPtr<IMgaRegNode> hideNode;
		CComBSTR hideNm(AN_HIDDEN);
		COMTHROW(mRootNode->get_SubNodeByName( hideNm, &hideNode));
		if (hideNode)
		{
			long hideStatus;
			COMTHROW( hideNode->get_Status( &hideStatus));
			if( hideStatus != ATTSTATUS_UNDEFINED)  // meta, here or inherited
			{
				CComBSTR bstr;
				COMTHROW( hideNode->get_Value( &bstr));
				if( bstr == L"1")
					return false;
			}
		}

		if (baseType->GetRegistryValue(mRootNode->Path).length())
		{
			CComPtr<IMgaRegNode> inheritNode;
			CComBSTR inhName(AN_INHERITABLE);
			COMTHROW(mRootNode->get_SubNodeByName( inhName, &inheritNode));

			if (inheritNode) {
				CComBSTR bstr;
				COMTHROW(inheritNode->get_Value( &bstr));
				if (bstr == L"1") // if "inheritable" is 1 show it
					return true;
			}
		}
		else
		{
			return true;
		}
	}
	catch(hresult_exception &e) {
		throw hresult_exception(e.hr);
		return false;
	}

	return false;
}

////////////////////////////////////
// Virtual methods of CGuiAnnotator
////////////////////////////////////



//////////////////////////////////// CGuiFco /////////////////////////////

//////////////////////////////////
// Non-virtual methods of CGuiFco
//////////////////////////////////

CGuiFco::CGuiFco(CComPtr<IMgaFCO>& pt, CComPtr<IMgaMetaRole>& role, CGMEView* vw, int numAsp) :
	CGuiBase(),
	mgaFco(pt),
	metaRole(role),
	view(vw),
	numParentAspects(numAsp),
	parentAspect(0),
	grayedOut(false),
	isType(false)
{
	{
		CComBSTR bstr;
		COMTHROW(role->get_Name(&bstr));
		CopyTo(bstr, roleName);
	}
	{
		CComBSTR bstr;
		COMTHROW(role->get_DisplayedName(&bstr));
		CopyTo(bstr, roleDisplayedName);
	}
	{
		COMTHROW(role->get_Kind(&metaFco));
	}
	{
		CComBSTR bstr;
		COMTHROW(metaFco->get_Name(&bstr));
		CopyTo(bstr, kindName);
	}
	{
		CComBSTR bstr;
		COMTHROW(metaFco->get_DisplayedName(&bstr));
		CopyTo(bstr, kindDisplayedName);
	}
	if (pt) {
		CComBSTR bstr;
		COMTHROW(mgaFco->get_ID(&bstr));
		CopyTo(bstr, id);
	}
	if (pt) {
		CComBSTR bstr;
		COMTHROW(mgaFco->get_Name(&bstr));
		CopyTo(bstr, name);
	}

	metaref_type metaRef;
	COMTHROW(metaFco->get_MetaRef(&metaRef));
	guiMeta = CGuiMetaProject::theInstance->GetGuiMetaFco(metaRef);
}

metaref_type CGuiFco::GetRoleMetaRef()
{
	metaref_type tp;
	VERIFY(metaRole);
	COMTHROW(metaRole->get_MetaRef(&tp));
	return tp;
}

bool CGuiFco::IsPrimary(CGuiMetaModel* guiMetaModel, int aspectInd)
{
	bool prim = false;
	CGuiMetaAspect *aspect = guiMetaModel->FindAspect(aspectInd);
	if(aspect)
		prim = CGuiFco::IsPrimary(guiMetaModel, aspect, metaRole);
	return prim;
}

bool CGuiFco::IsPrimary(CGuiMetaModel* guiMetaModel, CGuiMetaAspect* aspect)
{
	return CGuiFco::IsPrimary(guiMetaModel, aspect, metaRole);
}

bool CGuiFco::GetPreference(CString& val, CString path)
{
	CComBSTR pathBstr;
	CopyTo(path, pathBstr);
	CComBSTR bstrVal;
	if (mgaFco) {
		COMTHROW(mgaFco->get_RegistryValue(pathBstr, &bstrVal));
	}
	else {
		COMTHROW(metaFco->get_RegistryValue(pathBstr, &bstrVal));
	}
	CopyTo(bstrVal, val);
	return !val.IsEmpty();
}


bool CGuiFco::GetPreference(int& val, CString path, bool hex)
{
	CComBSTR pathBstr;
	CopyTo(path, pathBstr);
	CComBSTR bstrVal;
	if(mgaFco == 0) {
		COMTHROW(metaFco->get_RegistryValue(pathBstr, &bstrVal));
	}
	else {
		COMTHROW(mgaFco->get_RegistryValue(pathBstr, &bstrVal));
	}
	CString strVal;
	CopyTo(bstrVal,strVal);
	return (_stscanf(strVal,hex ? _T("%x") : _T("%d"),&val) == 1);
}

bool CGuiFco::GetColorPreference(unsigned long& color, CString path)
{
	int i;
	if(GetPreference(i,path,true)) {
		unsigned int r = (i & 0xff0000) >> 16;
		unsigned int g = (i & 0xff00) >> 8;
		unsigned int b = i & 0xff;
		color = RGB(r,g,b);
		return true;
	}
	return false;
}

CGuiMetaModel *CGuiFco::GetGuiMetaParent()
{
	CComPtr<IMgaMetaModel>	metaParent;
	COMTHROW(metaRole->get_ParentModel(&metaParent));
	VERIFY(metaParent);
	metaref_type metaRef;
	COMTHROW(metaParent->get_MetaRef(&metaRef));
	return CGuiMetaProject::theInstance->GetGuiMetaModel(metaRef);

}

////////////////////////////
// Static methods of CGuiFco
/////////////////////////////
bool CGuiFco::IsPrimary(CGuiMetaModel* guiMetaModel, CGuiMetaAspect* guiAspect, CComPtr<IMgaMetaRole>& metaRole)
{
	VARIANT_BOOL prim = VARIANT_FALSE;
	CComPtr<IMgaMetaAspect> mAspect;
	guiAspect->GetMetaAspect(mAspect);
	CComPtr<IMgaMetaPart> part;
	guiMetaModel->GetPartByRole(metaRole, mAspect, part);
	if(part != 0)
		COMTHROW(part->get_IsPrimary(&prim));
	return prim != VARIANT_FALSE;
}

void CGuiFco::SetAspect(CGuiFcoList& modelList, int asp)
{
	POSITION pos = modelList.GetHeadPosition();
	while(pos)
		modelList.GetNext(pos)->SetAspect(asp);
}

CGuiObject* CGuiFco::FindObject(CComPtr<IMgaFCO>& fco, CGuiFcoList& fcoList)
{
	POSITION pos = fcoList.GetHeadPosition();
	while(pos) {
		CGuiFco* ofco = fcoList.GetNext(pos);
		ASSERT(ofco != NULL);
		CGuiObject* obj = ofco->dynamic_cast_CGuiObject();
		if (obj) {
			VARIANT_BOOL b;
			COMTHROW(obj->mgaFco->get_IsEqual(fco, &b));
			if (b)
				return obj;
		}
	}
	return 0;
}

CGuiConnection* CGuiFco::FindConnection(CComPtr<IMgaFCO>& fco, CGuiConnectionList& conns)
{
	POSITION pos = conns.GetHeadPosition();
	while(pos) {
		CGuiConnection* conn = conns.GetNext(pos);
		VARIANT_BOOL b;
		COMTHROW(conn->mgaFco->get_IsEqual(fco, &b));
		if(b != VARIANT_FALSE)
			return conn;
	}
	return 0;
}


void CGuiFco::GrayOutFcos(CGuiFcoList& list, bool set)
{
	POSITION pos = list.GetHeadPosition();
	while(pos)
		list.GetNext(pos)->GrayOut(set);
}

void CGuiFco::GrayOutFcos(CGuiConnectionList& list, bool set)
{
	POSITION pos = list.GetHeadPosition();
	while(pos)
		list.GetNext(pos)->GrayOut(set);
}

void CGuiFco::GrayOutNonInternalConnections(CGuiConnectionList& list)
{
	POSITION pos = list.GetHeadPosition();
	while(pos) {
		CGuiConnection *conn = list.GetNext(pos);
		if(conn->src && conn->dst)
			conn->GrayOut(conn->src->grayedOut || conn->dst->grayedOut);
	}
}

void CGuiFco::ResetFlags(CGuiFcoList& list)
{
	POSITION pos = list.GetHeadPosition();
	while(pos)
		list.GetNext(pos)->flag = 0;
}

////////////////////////////
// Virtual methods of CGuiFco
/////////////////////////////


CGuiMetaAttributeList *CGuiFco::GetMetaAttributes()
{
	return &guiMeta->attrs;
}


////////////////////////////////// CGuiObject /////////////////////////////

//////////////////////////////////
// Non-virtual methods of CGuiObject
//////////////////////////////////
CGuiObject::CGuiObject(CComPtr<IMgaFCO>& pt, CComPtr<IMgaMetaRole>& role, CGMEView* vw, int numAsp) : CGuiFco(pt, role, vw, numAsp)
{
	guiAspects.SetSize(numAsp);
	for (int i = 0; i<numAsp; i++) {
		guiAspects[i] = NULL;
	}
}

void CGuiObject::InitObject(CWnd* viewWnd)
{
	try {
		CComPtr<IMgaMetaParts> mmParts;
		COMTHROW(metaRole->get_Parts(&mmParts));
		MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
			CComPtr<IMgaMetaPart> mmPart;
			mmPart = MGACOLL_ITER;
			CComPtr<IMgaMetaAspect> mmAspect;
			COMTHROW(mmPart->get_ParentAspect(&mmAspect));
			CComBSTR bstr;
			COMTHROW(mmAspect->get_Name(&bstr));
			CString aspName;
			CopyTo(bstr,aspName);
			CGuiMetaAspect *guiMetaAsp = GetGuiMetaParent()->FindAspect(aspName);
			VERIFY(guiMetaAsp);
			VERIFY(guiMetaAsp->index < numParentAspects);
			CString decoratorStr;
			GetDecoratorStr(decoratorStr);
			InitAspect(guiMetaAsp->index, mmPart, decoratorStr, viewWnd);
		}
		MGACOLL_ITERATE_END;

		CString hotspotPref;
		GetPreference(hotspotPref, HOTSPOT_PREF);
		if (hotspotPref == HOTSPOT_FALSE) {
			isHotspotEnabled = false;
		}
		else {
			isHotspotEnabled = true;
		}
	}
	catch(hresult_exception &e) {
		numParentAspects = 0;
		parentAspect = 0;
		throw hresult_exception(e.hr);
		return;
	}
	if (IsReal()) {
		ReadAllLocations();
	}
	else {
		SetAllSizesToNative();
	}
}

CGuiObject::~CGuiObject()
{
	if (view) {
		POSITION ppos = view->pendingRequests.GetHeadPosition();
		while (ppos) {
			POSITION tmp = ppos;
			CPendingObjectPosRequest *req = dynamic_cast<CPendingObjectPosRequest*> (view->pendingRequests.GetNext(ppos));
			if (req) {
				if ( req->object == this ) {
					view->pendingRequests.RemoveAt(tmp);
					delete req;
				}
			}
		}
	}

	for (int i = 0; i < guiAspects.GetSize(); i++) {
		delete guiAspects[i];
	}
	guiAspects.RemoveAll();
}

void CGuiObject::InitAspect(int asp, CComPtr<IMgaMetaPart>& metaPart, CString& decorStr, CWnd* viewWnd)
{
	VERIFY(asp < numParentAspects);
	CGuiMetaAspect* metaAspect = GetKindAspect(metaPart);
	VERIFY(metaAspect);

	CString	progId;
	CStringList params;
	CStringList values;

	if (!decorStr.IsEmpty()) {	// no decorator progId and no paarmeters => use default box decorator later
		if (decorStr.FindOneOf(_T("\n\t ,=")) == -1) {	// just a progId, no parameters
			progId = decorStr;
		} else {	// there is some parameter
			LPTSTR lpsz = new TCHAR[decorStr.GetLength()+1];
			LPTSTR tok = new TCHAR[decorStr.GetLength()+1];
			_tcscpy(lpsz, decorStr);
			int curpos = 0, tokpos = 0;
			int state = 0;  // 0:progid, 1:var, 2:val
			bool	gotOne = false;
			TCHAR	ch;

			while ( (ch = lpsz[curpos++]) != 0) {
				if (ch == _T('\n') || ch == _T('\t') || ch == _T(' ')) {
					if (gotOne) {
						state = 1;
						gotOne = false;
						tok[tokpos] = 0;
						progId = tok;
						tokpos = 0;
					}
					continue;
				}
				switch (state) {
					case 0:
						gotOne = true;
						tok[tokpos++] = ch;
						break;
					case 1:
						if (ch == _T('=')) {
							state = 2;
							tok[tokpos] = 0;
							params.AddTail(tok);
							tokpos = 0;
						}
						else {
							tok[tokpos++] = ch;
						}
						break;
					case 2:
						if (ch == _T(',')) {
							state = 1;
							tok[tokpos] = 0;
							values.AddTail(tok);
							tokpos = 0;
						}
						else {
							tok[tokpos++] = ch;
						}
						break;
				}
			}
			if (state == 0) {
				tok[tokpos] = 0;
				progId = tok;
			}
			if (state == 2) {
				tok[tokpos] = 0;
				values.AddTail(tok);
			}

			delete [] lpsz; // WAS: delete lpsz;
			delete [] tok;; // WAS: delete tok;;
		}
	}

	if (progId.IsEmpty()) {
		progId = GME_DEFAULT_DECORATOR;
	}
	CComPtr<IMgaDecorator> decor;
	CComPtr<IMgaElementDecorator> newDecor;

	CComObjPtr<CDecoratorEventSink> decoratorEventSink;
	try {
		CComPtr<IMgaElementDecoratorEvents> decoratorEventSinkIface;
		HRESULT hres = newDecor.CoCreateInstance(PutInBstr(progId));
		if (FAILED(hres) && hres != CO_E_CLASSSTRING) {	// might be an old decorator
			hres = decor.CoCreateInstance(PutInBstr(progId));
		}
		if (FAILED(hres)) {	// fall back to default decorator
			if (this->view->uncreatableDecorators.find(progId) == this->view->uncreatableDecorators.end())
			{
				CMainFrame::theInstance->m_console.Message(_T("Cannot create ") + progId + _T(" decorator! Trying default (") + GME_DEFAULT_DECORATOR + _T(") decorator."), 3);
				this->view->uncreatableDecorators.insert(progId);
			}
			progId = GME_DEFAULT_DECORATOR;
			COMTHROW(newDecor.CoCreateInstance(PutInBstr(progId)));
		}

		if (newDecor) {
			::CreateComObject(decoratorEventSink);
			HRESULT hr = ::QueryInterface((IMgaElementDecoratorEvents*)decoratorEventSink.p, &decoratorEventSinkIface);
			if (SUCCEEDED(hr)) {
				decoratorEventSink->SetView(view);
				decoratorEventSink->SetGuiObject(this);
			} else {
				ASSERT(false);
			}
			decor = CComQIPtr<IMgaDecorator>(newDecor);
		}

		// It seems that SetParam fails before InitializeEx, so this was probably never tested...
		POSITION ppos = params.GetHeadPosition();
		POSITION vpos = values.GetHeadPosition();
		while (ppos && vpos) {
			CComBSTR param(params.GetNext(ppos));
			CComVariant value(values.GetNext(vpos));
			COMTHROW(decor->SetParam(param, value));
		}
		// *************************************************************************************

		if (newDecor)
			COMTHROW(newDecor->InitializeEx(theApp.mgaProject, metaPart, mgaFco, decoratorEventSinkIface, (ULONGLONG)viewWnd->m_hWnd));
		else
			COMTHROW(decor->Initialize(theApp.mgaProject, metaPart, mgaFco));

		if (CGMEView::showConnectedPortsOnly )
		{
			CComVariant value(VARIANT_TRUE);
			CComBSTR param(DEC_CONNECTED_PORTS_ONLY_PARAM);
			COMTHROW(decor->SetParam(param,value));
		}
	}
	catch (hresult_exception&) {
		CMainFrame::theInstance->m_console.Message(_T("Cannot create ") + progId + _T(" decorator."), 3);
	}
	guiAspects[asp] = new CGuiAspect(metaAspect, this, metaAspect->index, asp, decor, newDecor, decoratorEventSink);
	parentAspect = 0;
}

void CGuiObject::GetDecoratorStr(CString& decorStr)
{
	if (!GetPreference(decorStr, DECORATOR_PREF))
		decorStr = GME_DEFAULT_DECORATOR;
}

void CGuiObject::SetObjectLocation(CRect& rect, int aspect, bool doMga)
{
	if(aspect < 0)
		aspect = parentAspect;
	VERIFY(aspect >= 0);
	VERIFY(guiAspects[aspect] != NULL);
	CRect loc = guiAspects[aspect]->GetLocation();
	// if (IsReal()) {
	loc.MoveToXY(rect.left, rect.top);
	guiAspects[aspect]->SetLocation(loc);
	if(IsReal() && doMga)
		WriteLocation(aspect);
}

void CGuiObject::SetSize(CSize& s, int aspect, bool doMga)
{
	if(aspect < 0)
		aspect = parentAspect;
	VERIFY(aspect >= 0);
	VERIFY(guiAspects[aspect] != NULL);
	CRect loc = guiAspects[aspect]->GetLocation();
	::SetSize(loc,s);
	guiAspects[aspect]->SetLocation(loc);
	if(IsReal() && doMga)
		WriteLocation(aspect);
}

void CGuiObject::SetAllSizes(CSize& s, bool doMga)
{
	for(int i = 0; i < numParentAspects; i++)
		if(guiAspects[i] != NULL)
			SetSize(s,i,doMga);
}

void CGuiObject::SetLocation(CRect& r, int aspect, bool doMga/*, bool savePreferredSize*/)
{
	if(aspect < 0)
		aspect = parentAspect;
	VERIFY(aspect >= 0);
	VERIFY(guiAspects[aspect] != NULL);
	guiAspects[aspect]->SetLocation(r);
	if(IsReal() && doMga)
		WriteLocation(aspect/*, savePreferredSize*/);
}

CSize CGuiObject::GetNativeSize(int aspect)
{
	if(aspect < 0)
		aspect = parentAspect;
	VERIFY(aspect >= 0);
	if(guiAspects[aspect] == NULL)
		return CSize(0,0);
	try {
		long sizex, sizey;
		COMTHROW(guiAspects[aspect]->GetDecorator()->GetPreferredSize(&sizex, &sizey));
		return CSize(sizex, sizey);
	}
	catch (hresult_exception &) {
		return CSize(0, 0);
	}
	//return true;// unreachable code
}


void CGuiObject::SetAllSizesToNative()
{
	CSize s;
	for(int i = 0; i < numParentAspects; i++) {
		if(guiAspects[i]) {
			s=GetNativeSize(i);
			CRect loc(0, 0, 0, 0);
			guiAspects[i]->SetLocation(loc);
			SetSize(s, i, false);
		}
	}
}

void CGuiObject::GetNeighbors(CGuiFcoList& list)
{
	VERIFY(GetCurrentAspect());
	CGuiPortList& portList = GetCurrentAspect()->GetPortList();
	POSITION ppos = portList.GetHeadPosition();
	while (ppos) {
		CGuiPort *port = portList.GetNext(ppos);
		POSITION cpos = port->GetInConns().GetHeadPosition();
		while(cpos) {
			CGuiConnection *conn = port->GetInConns().GetNext(cpos);
			if(conn->src->flag == 0) {
				list.AddTail(conn->src);
				conn->src->flag = 1;
			}
		}
		cpos = port->GetOutConns().GetHeadPosition();
		while(cpos) {
			CGuiConnection *conn = port->GetOutConns().GetNext(cpos);
			if(conn->dst->flag == 0) {
				list.AddTail(conn->dst);
				conn->dst->flag = 1;
			}
		}
	}
}

void CGuiObject::GetRelationsInOut(CGuiConnectionList& p_list, bool p_inOrOut)
{
	VERIFY(GetCurrentAspect());
	
	// acquire ports
	CGuiPortList& portList = GetCurrentAspect()->GetPortList();
	POSITION ppos = portList.GetHeadPosition();
	while (ppos) 
	{
		// one port
		CGuiPort *port = portList.GetNext(ppos);

		ASSERT( port); 
		if( !port) continue;

		// acquire the in/out connections through the current port
		POSITION cpos = p_inOrOut? port->GetInConns().GetHeadPosition(): port->GetOutConns().GetHeadPosition();
		while(cpos) 
		{
			// one connection
			CGuiConnection* conn = p_inOrOut? port->GetInConns().GetNext(cpos): port->GetOutConns().GetNext(cpos);

			ASSERT( conn);
			if( !conn) continue;

			// store it
			p_list.AddTail( conn);
		}
	}
}

void CGuiObject::ReadAllLocations()
{
	try {
		CComPtr<IMgaMetaParts> mmParts;
		CComPtr<IMgaMetaPart> mmPart;
		COMTHROW(metaRole->get_Parts(&mmParts));
		MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
			mmPart = MGACOLL_ITER;
			CComPtr<IMgaMetaAspect> mmAspect;
			COMTHROW(mmPart->get_ParentAspect(&mmAspect));
			CComBSTR bstr;
			COMTHROW(mmAspect->get_Name(&bstr));
			CString aspName;
			CopyTo(bstr,aspName);
			CGuiMetaAspect *guiMetaAsp = GetGuiMetaParent()->FindAspect(aspName);
			VERIFY(guiMetaAsp);
			VERIFY(guiMetaAsp->index < numParentAspects);
			int aspIndex = guiMetaAsp->index;
			VERIFY(aspIndex >= 0);
			CComPtr<IMgaPart> part;
			COMTHROW(mgaFco->get_Part(mmAspect,&part));
			CComBSTR icon;
			long x;
			long y;

			CRect loc(0, 0, 0, 0);
			CSize s = GetNativeSize(aspIndex);
			::SetSize(loc,s);

			if(part->GetGmeAttrs(0, &x, &y) == S_OK) {
				::SetLocation(loc, CPoint(x, y));
				guiAspects[aspIndex]->SetLocation(loc);
			}
			else { // unparsable integer pair found, use (-1, -1) like in CMgaPart::GetGmeAttrs
				::SetLocation(loc,CPoint(-1, -1));
				guiAspects[aspIndex]->SetLocation(loc);
			}
		}
		MGACOLL_ITERATE_END;
	}
	catch(hresult_exception &e) {
		throw hresult_exception(e.hr);
	}
}

void CGuiObject::WriteLocation(int aspect)
{
	VERIFY(IsReal());
	if(aspect < 0)
		aspect = parentAspect;
	VERIFY(aspect >= 0);
	VERIFY(guiAspects[aspect]);
	try {
		view->BeginTransaction();
		CComPtr<IMgaPart> part;
		CGuiMetaAspect *guiAsp = GetGuiMetaParent()->FindAspect(aspect);
		CComPtr<IMgaMetaBase> mBase = guiAsp->mgaMeta;
		CComPtr<IMgaMetaAspect> mAspect;
		COMTHROW(mBase.QueryInterface(&mAspect));
		COMTHROW(mgaFco->get_Part(mAspect,&part));
		CRect r = guiAspects[aspect]->GetLocation();
		// Save position part
		CPoint pt = r.TopLeft();
		COMTHROW(part->SetGmeAttrs(0, pt.x, pt.y));
		// This would be too agressive: to delete any connecting connections customizations if the box is moved
		// DeleteCustomizationOfInOutConnections(aspect);
		view->CommitTransaction();
	}
	catch(hresult_exception &e) {
		view->AbortTransaction(e.hr);
	}
}

void CGuiObject::DeleteCustomizationOfConnections(CGuiConnectionList& conns, long aspect)
{
	POSITION pos = conns.GetHeadPosition();
	while(pos) {
		CGuiConnection* conn = conns.GetNext(pos);
		if (conn->HasPathCustomization())
			if (conn->DeleteAllPathCustomizationsForAnAspect(aspect))
				conn->WriteCustomPathData(false);
	}
}

void CGuiObject::DeleteCustomizationOfInOutConnections(long aspect)
{
	CGuiConnectionList inConns;
	GetRelationsInOut(inConns, true);
	DeleteCustomizationOfConnections(inConns, aspect);
	CGuiConnectionList outConns;
	GetRelationsInOut(outConns, false);
	DeleteCustomizationOfConnections(outConns, aspect);
}

void CGuiObject::GrayOutNeighbors()
{
	CGuiFcoList neighbors;
	GetNeighbors(neighbors);
	GrayOutFcos(neighbors, grayedOut);
	ResetFlags(neighbors);
}

bool CGuiObject::IsInside(CPoint& pt, bool lookNearToo)
{
	CRect loc = GetLocation();
	if (lookNearToo)
		loc.InflateRect(3, 3);
	return (loc.PtInRect(pt) == TRUE);
}

bool CGuiObject::IsLabelInside(CPoint& pt, bool lookNearToo)
{
	CRect loc = GetNameLocation();
	if (lookNearToo)
		loc.InflateRect(3, 3);
	return (loc.PtInRect(pt) == TRUE);
}

CGuiPort* CGuiObject::FindPort(CPoint& pt, bool lookNearToo)
{
	CGuiPort* found = NULL;
	CSize foundSize(0, 0);
	VERIFY(GetCurrentAspect());
	CGuiPortList& portList = GetCurrentAspect()->GetPortList();
	POSITION pos = portList.GetHeadPosition();
	while(pos) {
		CGuiPort* port = portList.GetNext(pos);
		// The last one in the list is weird one: fco is the object's fco, skip that.
		// See CGuiAspect::InitPorts
		if (!mgaFco.IsEqualObject(port->mgaFco))
		{
			CRect r = port->GetLocation() + GetLocation().TopLeft();
			r.InflateRect(1, 1); // consider mouseover on red outline too
			CRect rInflated = r;
			rInflated.InflateRect(3, 3);
			if (r.PtInRect(pt) == TRUE || (lookNearToo && rInflated.PtInRect(pt) == TRUE))
			{
				CSize psize = port->GetLocation().Size();
				if (found)
				{
					if (psize.cx < foundSize.cx && psize.cy < foundSize.cy ) {
						foundSize = psize;
						found = port;
					}
				}
				else
				{
					foundSize = psize;
					found = port;
				}
			}
		}
	}
	return found;
}

CGuiPort* CGuiObject::FindPort(CComPtr<IMgaFCO> mgaFco)
{
	ASSERT(GetCurrentAspect());
	CGuiPortList& portList = GetCurrentAspect()->GetPortList();
	POSITION pos = portList.GetHeadPosition();
	while(pos) {
		CGuiPort* port = portList.GetNext(pos);
		VARIANT_BOOL b;
		COMTHROW(port->mgaFco->get_IsEqual(mgaFco, &b));
		if (b)
			return port;
	}
	return 0;
}

void CGuiObject::IncrementAnimRefCnt()
{
	if (view) {
		if (!(view->animRefCnt++)) {
			view->timerID = view->SetTimer(GME_ANIM_EVENT_ID, GME_ANIM_INTERVAL, NULL);
		}
	}
}

void CGuiObject::DecrementAnimRefCnt()
{
	if (view) {
		ASSERT(view->animRefCnt);
		if (!(--view->animRefCnt)) {
			if (view->timerID) {
				view->KillTimer(view->timerID);
				view->timerID = 0;
			}
		}
	}
}


//////////////////////////////////
// Static methods of CGuiObject
//////////////////////////////////

void CGuiObject::GetExtent(CGuiFcoList& objectList, CRect& rect)
{
	CGuiObject* obj;
	CRect cur1, cur2;
	POSITION pos = objectList.GetHeadPosition();
	rect = CRect(0, 0, 10, 10);
	while (pos) {
		CGuiFco* fco = objectList.GetNext(pos);
		ASSERT(fco != NULL);
		obj = fco->dynamic_cast_CGuiObject();
		if(obj && obj->IsVisible()) {
			cur2 = obj->GetLocation();
			cur1 = rect;
			rect.UnionRect(&cur1, &cur2);
		}
	}
}

void CGuiObject::GetExtent(CGuiObjectList& objectList, CRect& rect)
{
	CGuiObject* obj;
	CRect cur1, cur2;
	POSITION pos = objectList.GetHeadPosition();
	rect = CRect(0, 0, 0, 0);
	while(pos) {
		obj = objectList.GetNext(pos);
		if(obj && obj->IsVisible()) {
			cur2 = obj->GetLocation();
			cur1 = rect;
			rect.UnionRect(&cur1, &cur2);
		}
	}
}

void CGuiObject::MoveObjects(CGuiFcoList& fcoList, CPoint& pt)
{
	CGuiObjectList objs;
	POSITION pos = fcoList.GetHeadPosition();
	while(pos) {
		CGuiFco* fco = fcoList.GetNext(pos);
		ASSERT(fco != NULL);
		CGuiObject* obj = fco->dynamic_cast_CGuiObject();
		if (obj)
			objs.AddTail(obj);
	}
	MoveObjects(objs, pt);
}

void CGuiObject::MoveObjects(CGuiObjectList& objList, CPoint& pt)
{
	int left, top;
	FindUpperLeft(objList, left, top);
	CPoint diff = pt - CPoint(left, top);
	ShiftModels(objList, diff);
}


void CGuiObject::FindUpperLeft(CGuiObjectList& objs, int& left, int& top)
{
	bool start = true;
	POSITION pos = objs.GetHeadPosition();
	CGuiObject* obj;
	CRect r;
	while(pos) {
		obj = objs.GetNext(pos);
		if(obj->IsVisible()) {
			r = obj->GetLocation();
			if(start) {
				left = r.left;
				top = r.top;
				start = false;
			}
			else {
				if(r.left < left)
					left = r.left;
				if(r.top < top)
					top = r.top;
			}
		}
	}
}

void CGuiObject::ShiftModels(CGuiObjectList& objList, CPoint& shiftBy)
{
	CGMEEventLogger::LogGMEEvent(_T("CGuiObject::ShiftModels "));
	GMEEVENTLOG_GUIOBJS(objList);

	CGuiObject* first_obj = objList.IsEmpty() ? 0 : objList.GetHead();
	if( first_obj && first_obj->GetView() && first_obj->GetView() != modelGrid.GetSource())
	{
		// if the view where the object was moved and the view of the grid 
		// do not correspond, we must clear & update the grid.
		// possible because if a port is moved inside a model, then the model
		// is redrawn as well, and modelGrid is a global variable.
		// clearing only the grid would be too dangerous, would allow any movement
		// because other objects will disappear from the radar
		// but a Clear with a FillModelGrid will do the correct update
		modelGrid.Clear();
		first_obj->GetView()->FillModelGrid();
	}

	POSITION pos = objList.GetHeadPosition();
	while(pos) {
		CGuiObject* obj = objList.GetNext(pos);
		modelGrid.Reset(obj);
	}
	pos = objList.GetHeadPosition();
	while(pos) {
		CGuiObject* obj = objList.GetNext(pos);
		VERIFY(obj->IsVisible());
		CRect rect = obj->GetLocation();
		rect.MoveToXY(rect.left + shiftBy.x, rect.top + shiftBy.y);
		if(!modelGrid.GetClosestAvailable(obj, rect)) {
			//AfxMessageBox(_T("Too Many Models! Internal Program Error!"),MB_OK | MB_ICONSTOP);
			//return;
		}
		obj->SetLocation(rect);
		modelGrid.Set(obj);
	}
}

void CGuiObject::ResizeObject(const CRect& newLocation/*, bool doMga*/)
{
	CGMEEventLogger::LogGMEEvent(_T("CGuiObject::ResizeObject\n"));

	VERIFY(IsVisible());
	SetLocation((CRect)newLocation, -1, false/*doMga, true*/);
	// Note: there's no need to update modelgrid now, because after finishing the resize operation
	//		 there will be a full GMEView::Reset induced by the transaction commit
}

bool CGuiObject::NudgeObjects(CGuiObjectList& modelList, int right, int down)
{
	CGMEEventLogger::LogGMEEvent(_T("CGuiObject::NudgeObjects "));
	GMEEVENTLOG_GUIOBJS(modelList);
	ASSERT(right == 0 || down == 0); // cannot nudge diagonally for now

	CGuiObject* first_obj = modelList.IsEmpty() ? 0 : modelList.GetHead();
	if( first_obj && first_obj->GetView() && first_obj->GetView() != modelGrid.GetSource())
	{
		// if the view where the object was nudged and the view of the grid 
		// do not correspond, we must clear & update the grid
		// possible because if a port is moved inside a model, then the model
		// is redrawn as well, and modelGrid is a global variable.
		// clearing only the grid would be too dangerous, would allow nudging always
		// because other objects will not seem to sit in their places
		// but a Clear with a FillModelGrid will do the correct update
		modelGrid.Clear();
		first_obj->GetView()->FillModelGrid();
	}

	POSITION pos = modelList.GetHeadPosition();
	while(pos) {
		CGuiObject* model = modelList.GetNext(pos);
		VERIFY(model->IsVisible());
		modelGrid.Reset(model);
	}
	bool canDo = true;
	pos = modelList.GetHeadPosition();
	while(pos) {
		CGuiObject* model = modelList.GetNext(pos);
		if(!modelGrid.CanNudge(model, right, down)) {
			canDo = false;
			break;
		}
	}
	if(canDo) {
		POSITION pos = modelList.GetHeadPosition();
		while(pos) {
			CGuiObject *model = modelList.GetNext(pos);
			CRect loc = model->GetLocation();
			loc.OffsetRect(CPoint(right * GME_GRID_SIZE, down * GME_GRID_SIZE));
			model->SetLocation(loc);
			ASSERT(modelGrid.IsAvailable(model));
			modelGrid.Set(model);
		}
	}
	else {
		POSITION pos = modelList.GetHeadPosition();
		while(pos) {
			CGuiObject* model = modelList.GetNext(pos);
			modelGrid.Set(model);
		}
	}
	return canDo;
}

void CGuiObject::GetRectList(CGuiObjectList& objList, CRectList& rects)
{
	POSITION pos = objList.GetHeadPosition();
	while(pos) {
		CRect* rect = new CRect(objList.GetNext(pos)->GetLocation());
		rects.AddTail(rect);
	}
}

bool CGuiObject::IsResizable(void)
{
	VERIFY(parentAspect >= 0);
	VERIFY(GetCurrentAspect());

	try {
		feature_code fc = 0;
		CGuiAspect* aspect = GetCurrentAspect();
		if (aspect->GetNewDecorator())
			COMTHROW(GetCurrentAspect()->GetNewDecorator()->GetFeatures(&fc));
		else
			COMTHROW(GetCurrentAspect()->GetDecorator()->GetFeatures(&fc));
		return ((fc & F_RESIZABLE) != 0);
	}
	catch (hresult_exception &) {
		AfxMessageBox(_T("Error in CGuiObject [method IsResizable()]"));
	}

	return false;
}


//////////////////////////////////
// Virtual methods of CGuiObject
//////////////////////////////////
void CGuiObject::Draw(HDC pDC, Gdiplus::Graphics* gdip)
{
	VERIFY(parentAspect >= 0);
	VERIFY(GetCurrentAspect());

	try {
		CGuiAspect* aspect = GetCurrentAspect();
		if (aspect->GetNewDecorator())
			COMTHROW(GetCurrentAspect()->GetNewDecorator()->DrawEx((ULONG)pDC, (ULONGLONG)gdip));
		else
			COMTHROW(GetCurrentAspect()->GetDecorator()->Draw((ULONG)pDC));
	}
	catch (hresult_exception &e) {
		CComQIPtr<ISupportErrorInfo> errorInfo = GetCurrentAspect()->GetDecorator();
		_bstr_t error;
		if (errorInfo) {
			GetErrorInfo(error.GetAddress());
		} else {
			GetErrorInfo(e.hr, error.GetAddress());
		}
		// FIXME: KMS: won't Draw() be called after the MessageBox is dismissed?
		AfxMessageBox(CString(_T("Error in decorator [method Draw()]: ")) + static_cast<const TCHAR*>(error));
	}

// #define _ARDEBUG
#ifdef _ARDEBUG
	CPen arPen(PS_DOT, 1, 0x000000ff);
	CPen *oldPen = pDC->SelectObject(&arPen);
	pDC->SelectStockObject(HOLLOW_BRUSH);
	//pDC->Rectangle(GetCurrentAspect()->GetLocation());
	//pDC->Rectangle(GetCurrentAspect()->GetNameLocation());
	if (GetRouterBox()) {
		pDC->Rectangle(GetRouterBox()->GetRect());
	}
	if (GetRouterNameBox()) {
		pDC->Rectangle(GetRouterNameBox()->GetRect());
	}
	pDC->SelectObject(oldPen);
#endif

}

void CGuiObject::GrayOut(bool set)
{
	CGuiFco::GrayOut(set);

	for (int i = 0; i< guiAspects.GetSize(); i++) {
		if (IsVisible(i)) {
			COMTHROW(guiAspects[i]->GetDecorator()->SetActive(set ? VARIANT_FALSE : VARIANT_TRUE));
		}
	}
}



CGuiMetaAspect *CGuiObject::GetKindAspect(CComPtr<IMgaMetaPart> metaPart)
{
	// Kindaspect is the same as parentaspect in regular objects
	CComBSTR bstr;
	CComPtr<IMgaMetaAspect>	pAspect;
	COMTHROW(metaPart->get_ParentAspect(&pAspect));
	COMTHROW(pAspect->get_Name(&bstr));
	CString aspName;
	CopyTo(bstr, aspName);
	return GetGuiMetaParent()->FindAspect(aspName);
}

////////////////////////////////// CGuiCompound /////////////////////////////

////////////////////////////////////////
// Non-virtual methods of CGuiCompound
////////////////////////////////////////

///////////////////////////////////////
// Static methods of CGuiCompound
///////////////////////////////////////

///////////////////////////////////////
// Virtual methods of CGuiCompound
///////////////////////////////////////


////////////////////////////////// CGuiModel /////////////////////////////

///////////////////////////////////////
// Non-virtual methods of CGuiModel
///////////////////////////////////////
CGuiModel::CGuiModel(CComPtr<IMgaFCO>& pt, CComPtr<IMgaMetaRole>& role,CGMEView* vw, int numAsp) : CGuiCompound(pt, role, vw, numAsp)
{
	if (IsReal()) {
		VARIANT_BOOL inst;
		COMTHROW(pt->get_IsInstance(&inst));
		isType = (inst == VARIANT_FALSE);
	}
}

///////////////////////////////////////
// Static methods of CGuiModel
///////////////////////////////////////

///////////////////////////////////////
// Virtual methods of CGuiModel
///////////////////////////////////////
CGuiMetaAttributeList* CGuiModel::GetMetaAttributes()
{
	return &(GetCurrentAspect()->guiMeta->attrs);
}

CGuiMetaAspect* CGuiModel::GetKindAspect(CComPtr<IMgaMetaPart> metaPart)
{
	CGuiMetaModel* guiMetaModel = dynamic_cast<CGuiMetaModel*>(guiMeta);
	VERIFY(guiMetaModel);
	CGuiMetaAspect* metaAspect = NULL;
	CComBSTR bstr;
	COMTHROW(metaPart->get_KindAspect(&bstr));
	CString kindAspect;
	CopyTo(bstr, kindAspect);
	if (kindAspect.IsEmpty()) {
		CComBSTR bstr;
		CComPtr<IMgaMetaAspect>	pAspect;
		COMTHROW(metaPart->get_ParentAspect(&pAspect));
		COMTHROW(pAspect->get_Name(&bstr));
		CopyTo(bstr, kindAspect);
		metaAspect = guiMetaModel->FindAspect(kindAspect);
		if(!metaAspect)	{
			metaAspect = guiMetaModel->GetFirstAspect();
			/*
			CComBSTR bstr;
			COMTHROW(metaRole->get_Name(&bstr));
			CString roleName;
			CopyTo(bstr,roleName);
			AfxMessageBox(_T("Missing aspect mapping specification for model ") + name + _T(" aspect ") + kindAspect +
					_T(" role ") + roleName + _T("!\nFirst aspect ") + metaAspect->name + _T(" is used!"));
			*/
		}
	}
	else {
		metaAspect = guiMetaModel->FindAspect(kindAspect);
	}
	return metaAspect;
}
////////////////////////////////// CReference /////////////////////////////

///////////////////////////////////////
// Non-virtual methods of CReference
///////////////////////////////////////

CReference::CReference(CComPtr<IMgaFCO> mgaRefd, CComPtr<IMgaFCO> mgaTermRefd) : mgaReferee(mgaRefd), mgaTerminalReferee(mgaTermRefd)
{
	mgaReferee = mgaRefd;
	mgaTerminalReferee = mgaTermRefd;
	if(!IsNull()) {
		{
			CComBSTR bstr;
			COMTHROW(mgaRefd->get_Name(&bstr));
			CopyTo(bstr,targetName);
		}
		{
			CComPtr<IMgaMetaFCO> meta;
			COMTHROW(mgaRefd->get_Meta(&meta));
			CComBSTR bstr;
			COMTHROW(meta->get_DisplayedName(&bstr));
			CopyTo(bstr, targetKindDisplayedName);
		}
	}
}

CString CReference::GetInfoText(CString &name)
{
	CString txt;
	if(IsNull())
		txt = name + L" \u2192 null ";
	else
		txt.Format(L"%s \u2192 %s (%s) ", name, targetName, targetKindDisplayedName);
	return CString(txt);
}
///////////////////////////////////////
// Static methods of CReference
///////////////////////////////////////

///////////////////////////////////////
// Virtual methods of CReference
///////////////////////////////////////

////////////////////////////////// CGuiReference /////////////////////////////

///////////////////////////////////////
// Non-virtual methods of CGuiReference
///////////////////////////////////////
///////////////////////////////////////
// Static methods of CGuiReference
///////////////////////////////////////
///////////////////////////////////////
// Virtual methods of CGuiReference
///////////////////////////////////////


/////////////////////////////// CGuiCompoundReference //////////////////////////
////////////////////////////////////////////////
// Non-virtual methods of CGuiCompoundReference
////////////////////////////////////////////////

////////////////////////////////////////////////
// Static methods of CGuiCompoundReference
////////////////////////////////////////////////

////////////////////////////////////////////////
// Virtual methods of CGuiCompoundReference
////////////////////////////////////////////////

CGuiMetaAttributeList* CGuiCompoundReference::GetMetaAttributes()
{
	return &guiMeta->attrs;
}

CGuiMetaAspect* CGuiCompoundReference::GetKindAspect(CComPtr<IMgaMetaPart> metaPart)
{
	CComPtr<IMgaMetaFCO> metaFcoForAM;
	COMTHROW(GetTerminalReferee()->get_Meta(&metaFcoForAM));
	metaref_type metaRef;
	COMTHROW(metaFcoForAM->get_MetaRef(&metaRef));
	CGuiMetaModel* guiMetaModel = CGuiMetaProject::theInstance->GetGuiMetaModel(metaRef);
	VERIFY(guiMetaModel);

	CGuiMetaAspect* metaAspect = NULL;
	CComBSTR bstr;
	COMTHROW(metaPart->get_KindAspect(&bstr));
	CString kindAspect;
	CopyTo(bstr, kindAspect);
	if (kindAspect.IsEmpty()) {
		CComBSTR bstr;
		CComPtr<IMgaMetaAspect>	pAspect;
		COMTHROW(metaPart->get_ParentAspect(&pAspect));
		COMTHROW(pAspect->get_Name(&bstr));
		CopyTo(bstr, kindAspect);
		metaAspect = guiMetaModel->FindAspect(kindAspect);
		if(!metaAspect)	{
			metaAspect = guiMetaModel->GetFirstAspect();
			/*
			CComBSTR bstr;
			COMTHROW(metaRole->get_Name(&bstr));
			CString roleName;
			CopyTo(bstr,roleName);
			AfxMessageBox(_T("Missing aspect mapping specification for model ") + name + _T(" aspect ") + kindAspect +
					_T(" role ") + roleName + _T("!\nFirst aspect ") + metaAspect->name + _T(" is used!"));
			*/
		}
	}
	else {
		metaAspect = guiMetaModel->FindAspect(kindAspect);
	}
	return metaAspect;
}


//////////////////////////////////// CGuiSet ////////////////////////////////

////////////////////////////////////////////////
// Non-virtual methods of CGuiSet
////////////////////////////////////////////////

void CGuiSet::Init(CGuiFcoList& objs, CGuiConnectionList& conns)
{
	if (!IsReal()) {
		return;
	}
	CComPtr<IMgaSet> mgaSet;
	COMTHROW(mgaFco.QueryInterface(&mgaSet));

	CComPtr<IMgaFCOs> fcos;
	COMTHROW(mgaSet->get_Members(&fcos));
	MGACOLL_ITERATE(IMgaFCO,fcos) {
		CComPtr<IMgaFCO> member;
		member = MGACOLL_ITER;
		CGuiFco* obj =  CGuiFco::FindObject(member, objs);
		if(!obj)
			obj =  CGuiFco::FindConnection(member, conns);
		VERIFY(obj);
		members.AddTail(obj);
	}
	MGACOLL_ITERATE_END;
}

bool CGuiSet::CheckMember(CGuiFco* fco)
{
	CComPtr<IMgaMetaSet> metaSet;
	COMTHROW(metaFco.QueryInterface(&metaSet));

	metaref_type mr = fco->GetRoleMetaRef();
	CString path;
	path.Format(_T("%d"), mr);
	CComBSTR bstr;
	CopyTo(path, bstr);

	VARIANT_BOOL ok = VARIANT_FALSE;
	COMTHROW(metaSet->CheckPath(bstr, &ok));

	return (ok != VARIANT_FALSE);
}

bool CGuiSet::ToggleMember(CGuiFco* member)
{
	VERIFY(mgaFco);
	CComPtr<IMgaSet> mgaSet;
	COMTHROW(mgaFco.QueryInterface(&mgaSet));
	POSITION pos = members.Find(member);
	if(pos) {
		COMTHROW(mgaSet->RemoveMember(member->mgaFco));
	}
	else {
		COMTHROW(mgaSet->AddMember(member->mgaFco));
	}
	return pos == 0;
}


////////////////////////////////////////////////
// Static methods of CGuiSet
////////////////////////////////////////////////

////////////////////////////////////////////////
// Virtual methods of CGuiSet
////////////////////////////////////////////////

////////////////////////////////// CGuiConnectionLabel /////////////////////////////

CGuiConnectionLabel::CGuiConnectionLabel():
	alignment(TA_BASELINE | TA_CENTER),
	primary(false)
{
}

CGuiConnectionLabel::~CGuiConnectionLabel()
{
}

void CGuiConnectionLabel::SetLabel(const CString& l)
{
	label = l;
}

void CGuiConnectionLabel::SetPrimary(bool prim)
{
	primary = prim;
}

void CGuiConnectionLabel::SetLocation(const CPoint& endPoint, const CPoint& nextPoint, const CRect& box)
{
	if (label.IsEmpty())
		return;

	int diffx = 3;
	int diffy = 3;
	BOOL f_center = box.IsRectNull();

	loc = endPoint;
	CPoint center = box.CenterPoint();

	bool skew = (endPoint.x != nextPoint.x && endPoint.y != nextPoint.y);
	double alpha = 0.0;
	bool moreVerticalThanHorizontal = true;
	if (skew) {
		alpha = atan2(-((double)nextPoint.y - endPoint.y), (double)nextPoint.x - endPoint.x);
		moreVerticalThanHorizontal = (alpha >= -3 * M_PI_4 && alpha < -M_PI_4) || (alpha < 3 * M_PI_4 && alpha >= M_PI_4);
	}
	RoutingDirection lineDir = Dir_None;
	if (!f_center)
		lineDir = PointOnSide(endPoint, box);

	if (f_center)
	{
		if (endPoint.x == nextPoint.x || skew && moreVerticalThanHorizontal)	// vertical line
		{
			alignment = TA_BASELINE | (primary ? TA_RIGHT : TA_LEFT);
			loc.x += primary ? -diffx : diffx;
		}
		else if (endPoint.y == nextPoint.y || skew && !moreVerticalThanHorizontal)	// horizontal line
		{
			alignment = TA_CENTER | (primary ? TA_BOTTOM : TA_TOP);
		}
		else
		{
			ASSERT(false);
		}
	}
	else
	{
		switch(lineDir) {
			case Dir_Top:	// up
				{
					loc.y -= diffy;
					loc.x += primary ? -diffx : diffx;
					alignment = TA_BOTTOM | (primary ? TA_RIGHT : TA_LEFT);
				}
				break;
			case Dir_Bottom:	// down
				{
					loc.y += diffy;
					loc.x += primary ? -diffx : diffx;
					alignment = TA_TOP | (primary ? TA_RIGHT : TA_LEFT);
				}
				break;
			case Dir_Left:		// left
				{
					loc.x -= diffx;
					alignment = TA_RIGHT | (primary ? TA_BOTTOM : TA_TOP);
				}
				break;
			default:
				ASSERT(false);
			case Dir_Skew:	// TODO?
			case Dir_Right:		// right
				{
					loc.x += diffx;
					alignment = TA_LEFT | (primary ? TA_BOTTOM : TA_TOP);
				}
				break;
		}
	}
}

void CGuiConnectionLabel::Draw(Gdiplus::Graphics* gdip, COLORREF color, CGuiConnection* conn)
{
	if (label.IsEmpty())
		return;

	label.Replace(_T("%name%"), conn->name);
	label.Replace(_T("%kind%"), conn->kindDisplayedName);
	label.Replace(_T("%role%"), conn->roleDisplayedName);

	CGuiMetaAttributeList *metaAttrs = conn->GetMetaAttributes();
	POSITION pos = metaAttrs->GetHeadPosition();
	while (pos) {
		CGuiMetaAttribute *metaAttr = metaAttrs->GetNext(pos);
		CString attrName;
		attrName += _T("%");
		attrName += metaAttr->name;
		attrName += _T("%");
		label.Replace(attrName, conn->attributeCache[metaAttr->name]);
	}

	if (label.IsEmpty())
		return;
	graphics.DrawGdipText(gdip, label, loc, graphics.GetGdipFont(GME_CONNLABEL_FONT), color, alignment);
}

CPoint CGuiConnectionLabel::GetLocation(void) const
{
	return loc;
}

int CGuiConnectionLabel::GetAlignment(void) const
{
	return alignment;
}

CString CGuiConnectionLabel::GetLabel(void) const
{
	return label;
}

////////////////////////////////// CGuiConnectionLabel /////////////////////////////

CGuiConnectionLabelSet::CGuiConnectionLabelSet()
{
	labels[GME_CONN_SRC_LABEL1].SetPrimary(false);
	labels[GME_CONN_SRC_LABEL2].SetPrimary(true);
	labels[GME_CONN_DST_LABEL1].SetPrimary(false);
	labels[GME_CONN_DST_LABEL2].SetPrimary(true);
	labels[GME_CONN_MAIN_LABEL].SetPrimary(true);
}

CGuiConnectionLabelSet::~CGuiConnectionLabelSet()
{
}

void CGuiConnectionLabelSet::SetLabel(int index, const CString& label)
{
	if (index < 0 || index >= GME_CONN_LABEL_NUM)
		return;
	labels[index].SetLabel(label);
}

void CGuiConnectionLabelSet::SetLocation(int index, const CPoint& endPoint, const CPoint& nextPoint, const CRect& box)
{
	if (index < 0 || index >= GME_CONN_LABEL_NUM)
		return;
	labels[index].SetLocation(endPoint, nextPoint, box);
}

void CGuiConnectionLabelSet::Draw(Gdiplus::Graphics* gdip, COLORREF color, CGuiConnection* conn)
{
	for(int i = 0; i < GME_CONN_LABEL_NUM; i++)
		labels[i].Draw(gdip, color, conn);
}

CPoint CGuiConnectionLabelSet::GetLocation(int index) const
{
	CPoint pt(-1, -1);
	if (index >= 0 && index < GME_CONN_LABEL_NUM)
		pt = labels[index].GetLocation();
	return pt;
}

int CGuiConnectionLabelSet::GetAlignment(int index) const
{
	int alignment = 0;
	if (index >= 0 && index < GME_CONN_LABEL_NUM)
		alignment = labels[index].GetAlignment();
	return alignment;
}

CString CGuiConnectionLabelSet::GetLabel(int index) const
{
	CString label;
	if (index >= 0 && index < GME_CONN_LABEL_NUM)
		label = labels[index].GetLabel();
	return label;
}


////////////////////////////////// CGuiConnection /////////////////////////////

////////////////////////////////////////////////
// Non-virtual methods of CGuiConnection
////////////////////////////////////////////////

CGuiConnection::CGuiConnection(CComPtr<IMgaFCO>& pt, CComPtr<IMgaMetaRole>& role, CGMEView* vw, int numAsp, bool resolve):
	CGuiFco(pt, role, vw, numAsp),
	src						(NULL),
	srcPort					(NULL),
	dst						(NULL),
	dstPort					(NULL),
	hovered					(false),
	selected				(false),
	connRegAutoRouteNotSet	(true),
	isAutoRouted			(theApp.useAutoRouting)
{
	routerPath = NULL;

	if (resolve)
		Resolve();

	{
		CString pref;
		GetPreference(pref, CONN_LINE_TYPE_PREF);
		lineType = (pref == _T("dash")) ? GME_LINE_DASH : GME_LINE_SOLID;
	}
	{
		CString pref;
		GetPreference(pref, CONN_SRC_END_STYLE_PREF);
		if ( pref == _T("arrow") )
			srcStyle = GME_ARROW_END;
		else if ( pref == _T("diamond") )
			srcStyle = GME_DIAMOND_END;
		else if ( pref == _T("empty diamond") )
			srcStyle = GME_EMPTYDIAMOND_END;
		else if ( pref == _T("apex") )
			srcStyle = GME_APEX_END;
		else if ( pref == _T("empty apex") )
			srcStyle = GME_EMPTYAPEX_END;
		else if ( pref == _T("bullet") )
			srcStyle = GME_BULLET_END;
		else if ( pref == _T("empty bullet") )
			srcStyle = GME_EMPTYBULLET_END;
		else if ( pref == _T("left half arrow") )
			srcStyle = GME_HALFARROWLEFT_END;
		else if ( pref == _T("right half arrow") )
			srcStyle = GME_HALFARROWRIGHT_END;
		else
			srcStyle = GME_BUTT_END;
	}
	{
		CString pref;
		GetPreference(pref, CONN_DST_END_STYLE_PREF);
		if ( pref == _T("arrow") )
			dstStyle = GME_ARROW_END;
		else if ( pref == _T("diamond") )
			dstStyle = GME_DIAMOND_END;
		else if ( pref == _T("empty diamond") )
			dstStyle = GME_EMPTYDIAMOND_END;
		else if ( pref == _T("apex") )
			dstStyle = GME_APEX_END;
		else if ( pref == _T("empty apex") )
			dstStyle = GME_EMPTYAPEX_END;
		else if ( pref == _T("bullet") )
			dstStyle = GME_BULLET_END;
		else if ( pref == _T("empty bullet") )
			dstStyle = GME_EMPTYBULLET_END;
		else if ( pref == _T("left half arrow") )
			dstStyle = GME_HALFARROWLEFT_END;
		else if ( pref == _T("right half arrow") )
			dstStyle = GME_HALFARROWRIGHT_END;
		else
			dstStyle = GME_BUTT_END;
	}

	{
		ReadARPreferences();
	}
	{
		CString pref;

		GetPreference(pref,CONN_LABEL_FORMATSTR_PREF);
		labelset.SetLabel(GME_CONN_MAIN_LABEL, pref);
		GetPreference(pref,CONN_SRC_LABEL1_PREF);
		labelset.SetLabel(GME_CONN_SRC_LABEL1, pref);
		GetPreference(pref,CONN_SRC_LABEL2_PREF);
		labelset.SetLabel(GME_CONN_SRC_LABEL2, pref);
		GetPreference(pref,CONN_DST_LABEL1_PREF);
		labelset.SetLabel(GME_CONN_DST_LABEL1, pref);
		GetPreference(pref,CONN_DST_LABEL2_PREF);
		labelset.SetLabel(GME_CONN_DST_LABEL2, pref);
	}
	if (!GetColorPreference(color, COLOR_PREF)) {
		color = GME_BLACK_COLOR;
	}
	if (!GetColorPreference(nameColor, NAME_COLOR_PREF)) {
		nameColor = GME_BLACK_COLOR;
	}
	ReadCustomPathData();
	SetAutoRouted(ReadAutoRouteState());
	RefreshAttributeCache();
}

void CGuiConnection::GiveConnectionEndErroMessage(const TCHAR* mainMsg, const TCHAR* srcOrDst, const CGuiPort* otherPort) const
{
	if (!view->ShouldSupressConnectionCheckAlert()) {
		CString msgEx;
		msgEx.Append(_T("Connection properties:\n"));
		msgEx.Append(_T("\nName: "));
		msgEx.Append(name);
		msgEx.Append(_T("\nKind Name: "));
		msgEx.Append(kindDisplayedName);
		msgEx.Append(_T("\nRole Name: "));
		msgEx.Append(roleDisplayedName);
		msgEx.Append(_T("\nID: "));
		msgEx.Append(id);
		msgEx.Append(_T("\nDirection: "));
		msgEx.Append(srcOrDst);
		msgEx.Append(_T("\nOther endpoint properties:"));
		if (otherPort) {
			msgEx.Append(_T("\n\tName: "));
			msgEx.Append(otherPort->name);
			msgEx.Append(_T("\n\tID: "));
			msgEx.Append(otherPort->id);
		} else {
			msgEx.Append(_T("\n\tOther Endpoint is also null."));
		}
		msgEx.Append(_T("\nMissing endpoints sometimes can be generated by UDM based interpreters."));
		msgEx.Append(_T("\nCancel button supress further connection error messages like this."));
		int retVal = view->MessageBox(msgEx, mainMsg, MB_OKCANCEL | MB_ICONERROR);
		if (retVal == IDCANCEL)
			view->SupressConnectionCheckAlert();
	}
}

void CGuiConnection::Resolve()
{
	try {
		view->BeginTransaction(TRANSACTION_READ_ONLY);
		CComPtr<IMgaSimpleConnection> conn;
		mgaFco.QueryInterface(&conn);
		VERIFY(conn);

		// Compute visibility
		visible = std::unique_ptr<bool[]>(new bool[numParentAspects]);
		memset(visible.get(), 0, numParentAspects * sizeof(bool));
		CComPtr<IMgaMetaParts> mmParts;
		COMTHROW(metaRole->get_Parts(&mmParts));
		MGACOLL_ITERATE(IMgaMetaPart,mmParts) {
			CComPtr<IMgaMetaPart> mmPart;
			mmPart = MGACOLL_ITER;
			CComPtr<IMgaMetaAspect> mmAspect;
			COMTHROW(mmPart->get_ParentAspect(&mmAspect));
			CComBSTR bstr;
			COMTHROW(mmAspect->get_Name(&bstr));
			CString aspName;
			CopyTo(bstr,aspName);
			CGuiMetaAspect *guiMetaAsp = view->guiMeta->FindAspect(aspName);
			VERIFY(guiMetaAsp);
			VERIFY(guiMetaAsp->index < numParentAspects);
			visible[guiMetaAsp->index] = true;
		}
		MGACOLL_ITERATE_END;
		
		srcPort = NULL;
		dstPort = NULL;
		if(visible[parentAspect]) {
			CComPtr<IMgaFCO> mgaSrc;

			// Lookup first conn. reference first
			CComPtr<IMgaFCOs> mgaSrcRefs;
			bool src_err = false;
			HRESULT src_hr = S_OK;
			{
				try {
					src_hr = conn->get_SrcReferences(&mgaSrcRefs);
				}
				catch(hresult_exception &e)
				{
					ASSERT(FAILED(e.hr));
					SetErrorInfo(e.hr);
					src_hr = e.hr;
				}
			}
			if (SUCCEEDED(src_hr) || mgaSrcRefs) {
				long nSrcRef = 0;
				COMTHROW(mgaSrcRefs->get_Count(&nSrcRef));
				if (nSrcRef > 0) {
					COMTHROW(mgaSrcRefs->get_Item(1, &mgaSrc));
				}
				else {
					COMTHROW(conn->get_Src(&mgaSrc));
				}

				POSITION pos = view->children.GetHeadPosition();
				while (pos) {
					CGuiFco* fco = view->children.GetNext(pos);
					ASSERT(fco != NULL);
					CGuiObject* srcObj = fco->dynamic_cast_CGuiObject();
					if (srcObj && srcObj->IsVisible()) {
						srcPort = srcObj->FindPort(mgaSrc);
						if (srcPort) {
							CGuiCompoundReference* modelRefObj = srcObj->dynamic_cast_CGuiCompoundReference();
							if (modelRefObj) {
								CComPtr<IMgaFCO> mgaSrcPort;
								COMTHROW(conn->get_Src(&mgaSrcPort));
								CGuiPort *srcFPort = srcObj->FindPort(mgaSrcPort);
								srcPort = srcFPort ? srcFPort : srcPort;
								if ((srcPort->IsRealPort()) && (!nSrcRef) ) {
									continue;
								}
							}
							src = srcPort->parent->GetParent();
							VERIFY(src);
							break;
						}
					}
				}
				if (!srcPort) {
					visible[parentAspect] = false;
				}
			} else {
				src_err = true;
			}

			CComPtr<IMgaFCO> mgaDst;

			// Lookup first conn. reference first
			HRESULT dst_hr = S_OK;
			CComPtr<IMgaFCOs> mgaDstRefs;
			bool dst_err = false;
			{
				try {
					dst_hr = conn->get_DstReferences(&mgaDstRefs);
				}
				catch(hresult_exception &e)
				{
					ASSERT(FAILED(e.hr));
					SetErrorInfo(e.hr);
					dst_hr = e.hr;
				}
			}
			if (SUCCEEDED(dst_hr) && mgaDstRefs) {
				long nDstRef = 0;
				COMTHROW(mgaDstRefs->get_Count(&nDstRef));
				if (nDstRef > 0) {
					COMTHROW(mgaDstRefs->get_Item(1, &mgaDst));
				}
				else {
					COMTHROW(conn->get_Dst(&mgaDst));
				}

				POSITION pos = view->children.GetHeadPosition();
				while (pos) {
					CGuiFco* fco = view->children.GetNext(pos);
					ASSERT(fco != NULL);
					CGuiObject* dstObj = fco->dynamic_cast_CGuiObject();
					if (dstObj && dstObj->IsVisible()) {
						dstPort = dstObj->FindPort(mgaDst);
						if (dstPort) {
							CGuiCompoundReference* modelRefObj = dstObj->dynamic_cast_CGuiCompoundReference();
							if (modelRefObj) {
								CComPtr<IMgaFCO> mgaDstPort;
								COMTHROW(conn->get_Dst(&mgaDstPort));
								CGuiPort *dstFPort = dstObj->FindPort(mgaDstPort);
								dstPort = dstFPort ? dstFPort : dstPort;
								if ((dstPort->IsRealPort()) && (!nDstRef) ) {
									continue;
								}
							}
							dst = dstPort->parent->GetParent();
							VERIFY(dst);
							break;
						}
					}
				}
				if (!dstPort) {
					visible[parentAspect] = false;
				}
			} else {
				dst_err = true;
			}
			if (src_err || dst_err) {
				if (src_err) {
					GiveConnectionEndErroMessage(_T("Source endpoint error"), _T("src"), dstPort);
				} else if (dst_err) {
					GiveConnectionEndErroMessage(_T("Destination endpoint error"), _T("dst"), srcPort);
				}
			} else // FIXME?

			if (visible[parentAspect]) {
				if( !srcPort->outConns.Find( this)) // WAS: outConns.AddTail(this); and as a result as aspects have cycled the collection got bigger and bigger
					srcPort->outConns.AddTail(this);// NOW: added only if ptr was not already stored in outConns
				
				if( !dstPort->inConns.Find( this)) // WAS: inConns.AddTail(this);
					dstPort->inConns.AddTail(this);// NOW: added only if ptr was not already stored in inConns
			}

		}
		view->CommitTransaction();
	}
	catch (hresult_exception e) {
		view->AbortTransaction(e.hr);
	}
}

void CGuiConnection::Draw(HDC pDC, Gdiplus::Graphics* gdip)
{
	if(!IsVisible())
		return;
	if (!src || !dst)
		return;
	VERIFY(src->IsVisible());
	VERIFY(dst->IsVisible());

	CPointList points;
	GetPointList(points);

	if (points.GetSize() <= 0) {
		ASSERT(false);
		return;
	}

	std::vector<long> customizedEdgeIndexes;
	if (selected && IsAutoRouted())
		customizedEdgeIndexes = GetRelevantCustomizedEdgeIndexes();
	int width = selected ? 3 : hovered ? 5 : 1;
	if (selected && view->m_zoomVal < ZOOM_NO)
	{
		// selection was hard to see at <100% zoom. Magic numbers determined by experimentation
		width = (int)((float)((ZOOM_NO - 25) / max(10, view->m_zoomVal) + 0.2) * width + 0.5f);
	}
	graphics.DrawConnection(gdip, points, customizedEdgeIndexes, grayedOut ? GME_GRAYED_OUT_COLOR : color,
							lineType, srcStyle, dstStyle, !IsAutoRouted() && selected,
							width);

	if (points.GetSize() < 2) {
		ASSERT(false);
		return;
	}

	POSITION pos = points.GetHeadPosition();
	CPoint start = points.GetNext(pos);
	CPoint start2 = points.GetNext(pos);
	pos = points.GetTailPosition();
	CPoint end = points.GetPrev(pos);
	CPoint end2 = points.GetPrev(pos);

	// Get the center position of the whole connection

	int segments = points.GetCount();
	pos = points.FindIndex((int)(segments/2)-1);
	CPoint middle = points.GetNext(pos);
	CPoint middle2 = points.GetNext(pos);
	CRect tmpRect(middle, middle2);
	middle = tmpRect.CenterPoint();

	const CRect srcRect = src->GetLocation();
	const CRect dstRect = dst->GetLocation();

	CRect box;
	box = src->GetLocation();
	if (srcPort != NULL && srcPort->IsRealPort())
		box = srcPort->GetLocation() + box.TopLeft();
	labelset.SetLocation(0, start, start2, box);
	labelset.SetLocation(1, start, start2, box);
	box = dst->GetLocation();
	if (dstPort != NULL && dstPort->IsRealPort())
		box = dstPort->GetLocation() + box.TopLeft();
	labelset.SetLocation(2, end, end2, box);
	labelset.SetLocation(3, end, end2, box);
	box = CRect(0, 0, 0, 0);
	labelset.SetLocation(GME_CONN_MAIN_LABEL, middle, middle2, box);


	labelset.Draw(gdip, (grayedOut ? GME_GRAYED_OUT_COLOR : nameColor), this);
}

void CGuiConnection::RemoveFromRouter(CAutoRouter &router)
{
	router.DeleteConnection(this);
}

void CGuiConnection::GrayOutEndPoints()
{
	src->GrayOut(grayedOut);
	dst->GrayOut(grayedOut);
}

void CGuiConnection::RefreshAttributeCache()
{
	attributeCache.RemoveAll();

	CGuiMetaAttributeList* guiMetaAttrs = GetMetaAttributes();
	POSITION pos = guiMetaAttrs->GetHeadPosition();

	while (pos) {
		CGuiMetaAttribute* guiMetaAttr = guiMetaAttrs->GetNext(pos);
		CComPtr<IMgaAttribute> attr;
		CComPtr<IMgaMetaAttribute> metaAttr;
		attval_enum	tp;
		COMTHROW(guiMetaAttr->mgaMeta.QueryInterface(&metaAttr));
		COMTHROW(metaAttr->get_ValueType(&tp));
		COMTHROW(mgaFco->get_Attribute(metaAttr, &attr));
		CComVariant	attrValue;
		COMTHROW(attr->get_Value(&attrValue));

		CString aval;
		CString aname = guiMetaAttr->name;

		if (tp == ATTVAL_BOOLEAN) {
			VERIFY(attrValue.vt == VT_BOOL);
			aval = attrValue.boolVal ? _T("True") : _T("False");
		}
		else if (tp == ATTVAL_REFERENCE) {
			aval = _T("Reference attr. label not supported");	// Sorry, maybe later
		}
		else if (tp == _attval_enum) {
			VERIFY(attrValue.vt == VT_BSTR);
			CString attrValueStr(attrValue.bstrVal);
			/* CGuiMenuCtrl *guiMenuCtrl = dynamic_cast<CGuiMenuCtrl *>(guiMetaAttr->depCtrl);
			if (guiMenuCtrl) {
				aval = attrValue.bstrVal;
				int idx = guiMenuCtrl->FindItemByValue(aval);
				if (idx != -1)
					aval = (guiMenuCtrl->items[idx]).label;
				else
					aval = _T("N/A");
			}
			else {
				aval = _T("N/A");
			} */
			aval = _T("N/A");
			CComPtr<IMgaMetaEnumItems> metaEnumItems;
			COMTHROW(metaAttr->get_EnumItems(&metaEnumItems));

			MGACOLL_ITERATE(IMgaMetaEnumItem,metaEnumItems)
			{
				CComBSTR bstrItemLabel;
				CComBSTR bstrItemValue;
				COMTHROW(MGACOLL_ITER->get_DisplayedName(&bstrItemLabel));
				COMTHROW(MGACOLL_ITER->get_Value(&bstrItemValue));

				CString strItemValue(bstrItemValue);
				if (strItemValue == attrValueStr) {
					aval = bstrItemLabel;
				}
			}
			MGACOLL_ITERATE_END;

		}
		else {
			if (attrValue.vt != VT_BSTR)
				attrValue.ChangeType(VT_BSTR);
			aval = attrValue.bstrVal;
		}
		attributeCache[aname] = aval;
	}
}

void CGuiConnection::ReadARPreferences()
{
	CString val;
	if (GetPreference(val, AUTOROUTER_PREF)) {
		autorouterPrefs[GME_START_NORTH]	= (val.Find(_T("N")) != -1);
		autorouterPrefs[GME_START_EAST]		= (val.Find(_T("E")) != -1);
		autorouterPrefs[GME_START_SOUTH]	= (val.Find(_T("S")) != -1);
		autorouterPrefs[GME_START_WEST]		= (val.Find(_T("W")) != -1);
		autorouterPrefs[GME_END_NORTH]		= (val.Find(_T("n")) != -1);
		autorouterPrefs[GME_END_EAST]		= (val.Find(_T("e")) != -1);
		autorouterPrefs[GME_END_SOUTH]		= (val.Find(_T("s")) != -1);
		autorouterPrefs[GME_END_WEST]		= (val.Find(_T("w")) != -1);
	} else {
		autorouterPrefs[GME_START_NORTH]	= false;
		autorouterPrefs[GME_START_EAST]		= false;
		autorouterPrefs[GME_START_SOUTH]	= false;
		autorouterPrefs[GME_START_WEST]		= false;
		autorouterPrefs[GME_END_NORTH]		= false;
		autorouterPrefs[GME_END_EAST]		= false;
		autorouterPrefs[GME_END_SOUTH]		= false;
		autorouterPrefs[GME_END_WEST]		= false;
	}
}

void CGuiConnection::GetPointList(CPointList& points) const
{
	if (routerPath != NULL)
	{
		CPointListPath& pointList = routerPath->GetPointList();
		POSITION pos = pointList.GetHeadPosition();
		while( pos != NULL )
		{
			points.AddTail(pointList.GetNext(pos));
		}
	}

	if (!routerPath || points.GetSize() <= 0)
	{
		if (srcPort != NULL) {
			CPoint start = srcPort->GetLocation().CenterPoint() + src->GetLocation().TopLeft();
			points.AddHead(start);
		}
		if (dstPort != NULL) {
			CPoint end = dstPort->GetLocation().CenterPoint() + dst->GetLocation().TopLeft();
			points.AddTail(end);
		}
	}
}

int CGuiConnection::GetEdgeCount(void) const
{
	CPointList points;
	GetPointList(points);
	return points.GetSize() - 1;
}

CRect CGuiConnection::GetBounds(void) const
{
	CPointList points;
	GetPointList(points);

	POSITION pos = points.GetHeadPosition();
	long xmin = LONG_MAX;
	long xmax = LONG_MIN;
	long ymin = LONG_MAX;
	long ymax = LONG_MIN;
	CRect bounds;
	bounds.SetRectEmpty();
	if (pos) {
		while (pos) {
			CPoint& pt = points.GetNext(pos);
			if (pt.x > xmax)
				xmax = pt.x;
			if (pt.x < xmin)
				xmin = pt.x;
			if (pt.y > ymax)
				ymax = pt.y;
			if (pt.y < ymin)
				ymin = pt.y;
		}
		bounds = CRect(xmin, ymin, xmax, ymax);
	}

	return bounds;
}

CPoint CGuiConnection::GetCenter(void) const
{
	CRect bounds = GetBounds();
	return bounds.CenterPoint();
}

bool CGuiConnection::AdjustCoordLimits(CPointList& points, int edgeIndex, bool isPathEnd, bool xOrY, POSITION pos,
									   int ptCoord, int lastlastCoord, long& coordMinLimit, long& coordMaxLimit) const
{
	if (!isPathEnd) {
		// Internal edge
		CPoint nextPt = points.GetNext(pos);
		int nextPtCoord = xOrY ? nextPt.x : nextPt.y;
		if (ptCoord < nextPtCoord && ptCoord < lastlastCoord)
			coordMinLimit = LONG_MIN;
		else
			coordMinLimit = min(nextPtCoord, lastlastCoord);
		if (ptCoord > nextPtCoord && ptCoord > lastlastCoord)
			coordMaxLimit = LONG_MAX;
		else
			coordMaxLimit = max(nextPtCoord, lastlastCoord);
	} else {
		// Inspect entities at the end to calculate limits
		CRect portRect;
		portRect.SetRectEmpty();
		if (edgeIndex == 0) {
			if (srcPort != NULL && srcPort->IsRealPort()) {
				return true;
			} else if (src != NULL) {
				portRect = src->GetLocation();
			}
		} else {
			if (dstPort != NULL && dstPort->IsRealPort()) {
				return true;
			} else if (dst != NULL) {
				portRect = dst->GetLocation();
			}
		}

		if (!portRect.IsRectEmpty()) {
			if (xOrY) {
				coordMinLimit = portRect.left;
				coordMaxLimit = portRect.right;
			} else {
				coordMinLimit = portRect.top;
				coordMaxLimit = portRect.bottom;
			}
		} else {
			ASSERT(false);
			return true;
		}
	}

	return false;
}

int CGuiConnection::GetEdgeIndex(const CPoint& point, CPoint& startPoint, CPoint& endPoint, CPoint& thirdPoint,
								 ConnectionPartMoveType& connectionMoveMethod, bool& horizontalOrVerticalEdge,
								 bool& isPartFixed, long& xMinLimit, long& xMaxLimit, long& yMinLimit, long& yMaxLimit,
								 bool checkPointOnEdge) const
{
	CPointList points;
	GetPointList(points);

	int numEdges = points.GetSize() - 1;
	if (numEdges == -1 || routerPath == NULL)
		return -1;
	CPoint last = emptyPoint;
	CPoint lastlast = emptyPoint;
	POSITION pos = points.GetHeadPosition();
	int i = 0;
	CAutoRouterGraph* ag = routerPath->GetOwner();
	ASSERT(ag != NULL);
	if (pos) {
		CPoint pt = points.GetNext(pos);
		last = pt;
		while (pos) {
			pt = points.GetNext(pos);
			if (IsAutoRouted()) {
				bool moveAction = false;
				if (last.x == pt.x) {	// vertical edge, horizontal move
					if (abs(pt.x - point.x) <= 3 &&
						(point.y >= min(last.y, pt.y) - 3 ||
						 point.y <= max(last.y, pt.y) + 3))
					{
						connectionMoveMethod = HorizontalEdgeMove;
						horizontalOrVerticalEdge = false;
						moveAction = true;
					}
				} else if (last.y == pt.y) {	// horizontal line, vertical move
					if (abs(pt.y - point.y) <= 3 &&
						(point.x >= min(last.x, pt.x) - 3 ||
						 point.x <= max(last.x, pt.x) + 3))
					{
						connectionMoveMethod = VerticalEdgeMove;
						horizontalOrVerticalEdge = true;
						moveAction = true;
					}
				} else {
					// There mustn't be any skew edge in case of auto routed connection
					ASSERT(false);
				}
				if (i < numEdges - 1 &&		// We can't drag two connected edges at the last point!
					abs(pt.x - point.x) <= 3 &&
					abs(pt.y - point.y) <= 3)
				{
					connectionMoveMethod = AdjacentEdgeMove;
				}
				if (moveAction) {
					isPartFixed = ag->IsEdgeFixed(routerPath, last, pt);

					startPoint = last;
					endPoint = pt;
					bool isPathEnd = (i == 0 || i == numEdges - 1);

					bool isPartFixed2 = false;
					bool pointOnEdge = false;
					if (connectionMoveMethod == HorizontalEdgeMove) {
						isPartFixed2 = AdjustCoordLimits(points, i, isPathEnd, true, pos,
														 pt.x, lastlast.x, xMinLimit, xMaxLimit);
						if (checkPointOnEdge)
							pointOnEdge = point.y >= min(startPoint.y, endPoint.y) - 3 && point.y <= max(startPoint.y, endPoint.y) + 3;
					} else if (connectionMoveMethod == VerticalEdgeMove) {
						isPartFixed2 = AdjustCoordLimits(points, i, isPathEnd, false, pos,
														 pt.y, lastlast.y, yMinLimit, yMaxLimit);
						if (checkPointOnEdge)
							pointOnEdge = point.x >= min(startPoint.x, endPoint.x) - 3 && point.x <= max(startPoint.x, endPoint.x) + 3;
					} else  if (connectionMoveMethod == AdjacentEdgeMove) {
						POSITION nextPos = pos;
						CPoint nextPt = points.GetNext(nextPos);
						thirdPoint = nextPt;
						if (horizontalOrVerticalEdge) {
							isPartFixed2 = AdjustCoordLimits(points, i, isPathEnd, false, pos,
															 pt.y, lastlast.y, yMinLimit, yMaxLimit);
							if (checkPointOnEdge)
								pointOnEdge = point.x >= min(startPoint.x, endPoint.x) - 3 && point.x <= max(startPoint.x, endPoint.x) + 3;
							if (isPartFixed2) {
								connectionMoveMethod = HorizontalEdgeMove;
								isPartFixed2 = false;
							} // FIXME else ?
							isPartFixed2 = AdjustCoordLimits(points, i + 1, i >= numEdges - 2, true, nextPos,
															 nextPt.x, last.x, xMinLimit, xMaxLimit);
							if (checkPointOnEdge)
								pointOnEdge = pointOnEdge && (point.y >= min(endPoint.y, nextPt.y) - 3 && point.y <= max(endPoint.y, nextPt.y) + 3);
							if (isPartFixed2) {
								if (connectionMoveMethod != HorizontalEdgeMove) {
									connectionMoveMethod = VerticalEdgeMove;
									isPartFixed2 = false;
								}
							}
						} else {
							isPartFixed2 = AdjustCoordLimits(points, i, isPathEnd, true, pos,
															 pt.x, lastlast.x, xMinLimit, xMaxLimit);
							if (checkPointOnEdge)
								pointOnEdge = point.y >= min(startPoint.y, endPoint.y) - 3 && point.y <= max(startPoint.y, endPoint.y) + 3;
							if (isPartFixed2) {
								connectionMoveMethod = VerticalEdgeMove;
								isPartFixed2 = false;
							} // FIXME else?
							isPartFixed2 = AdjustCoordLimits(points, i + 1, i >= numEdges - 2, false, nextPos,
															 nextPt.y, last.y, yMinLimit, yMaxLimit);
							if (checkPointOnEdge)
								pointOnEdge = pointOnEdge && (point.x >= min(endPoint.x, nextPt.x) - 3 && point.x <= max(endPoint.x, nextPt.x) + 3);
							if (isPartFixed2) {
								if (connectionMoveMethod != VerticalEdgeMove) {
									connectionMoveMethod = HorizontalEdgeMove;
									isPartFixed2 = false;
								}
							}
						}
					}
					isPartFixed |= isPartFixed2;

					if (!checkPointOnEdge || pointOnEdge)
						return i;
				}
			} else {
				if (abs(pt.x - point.x) <= 3 && abs(pt.y - point.y) <= 3) {
					startPoint = last;
					CPoint next = points.GetNext(pos);
					endPoint = next;

					connectionMoveMethod = ModifyExistingCustomPoint;
					isPartFixed = false;
					return i;
				} else {
					if (IsOnEdge(last, pt, point))
					{
						startPoint = last;
						endPoint = pt;

						connectionMoveMethod = InsertNewCustomPoint;
						isPartFixed = false;
						return i;
					}
				}
			}

			i++;
			lastlast = last;
			last = pt;
		}
	}

	return -1;
}

int CGuiConnection::IsPathAt(const CPoint& point, ConnectionPartMoveType& connectionMoveMethod,
							 bool& horizontalOrVerticalEdge, bool& isPartFixed) const
{
	CPoint customizeConnectionEdgeStartPoint;
	CPoint customizeConnectionEdgeEndPoint;
	CPoint customizeConnectionEdgeThirdPoint;
	long customizeConnectionEdgeXMinLimit;
	long customizeConnectionEdgeXMaxLimit;
	long customizeConnectionEdgeYMinLimit;
	long customizeConnectionEdgeYMaxLimit;

	return GetEdgeIndex(point, customizeConnectionEdgeStartPoint, customizeConnectionEdgeEndPoint, customizeConnectionEdgeThirdPoint,
						connectionMoveMethod, horizontalOrVerticalEdge,
						isPartFixed, customizeConnectionEdgeXMinLimit, customizeConnectionEdgeXMaxLimit,
						customizeConnectionEdgeYMinLimit, customizeConnectionEdgeYMaxLimit);
}

long CGuiConnection::IsPointOnSectionAndDeletable(long edgeIndex, const CPoint& point)
{
	CPointList points;
	GetPointList(points);

	CPoint last;
	CPoint lastlast;
	POSITION pos = points.GetHeadPosition();
	int i = 0;
	if (pos) {
		CPoint pt = points.GetNext(pos);
		last = pt;
		while (pos) {
			pt = points.GetNext(pos);

			if (i == edgeIndex) {
				if (pos) {
					CPoint next = points.GetNext(pos);

					if (IsOnEdge(last, next, point, 5)) {
						return i;
					}
				}
				return -1;
			}

			i++;
			lastlast = last;
			last = pt;
		}
	}

	return -1;
}

std::vector<long> CGuiConnection::GetRelevantCustomizedEdgeIndexes(void)
{
	int edgeCount = GetEdgeCount();
	long asp = view->currentAspect->index;
	std::vector<long> customizedEdgeIndexes;
	std::vector<CustomPathData>::iterator ii = customPathData.begin();
	while (ii != customPathData.end() && edgeCount > 0) {
		if ((*ii).GetAspect() == asp || asp == -1) {
			if (IsAutoRouted() && (*ii).GetType() == SimpleEdgeDisplacement && (*ii).GetEdgeCount() == edgeCount ||
			   !IsAutoRouted() && (*ii).GetType() != SimpleEdgeDisplacement)
			{
				customizedEdgeIndexes.push_back((*ii).GetEdgeIndex());
			}
		}
		++ii;
	}
	return customizedEdgeIndexes;
}

void CGuiConnection::FillOutCustomPathData(CustomPathData& pathData, PathCustomizationType custType, long asp,
										   int newPosX, int newPosY, int edgeIndex, int edgeCount, bool horizontalOrVerticalEdge)
{
	pathData.SetVersion					(CONNECTIONCUSTOMIZATIONDATAVERSION);
	pathData.SetAspect					(asp);
	pathData.SetEdgeIndex				(edgeIndex);
	pathData.SetEdgeCount				(edgeCount < 0 ? GetEdgeCount() : edgeCount);
	pathData.SetType					(custType);
	pathData.SetHorizontalOrVertical	(horizontalOrVerticalEdge);
	if (custType == SimpleEdgeDisplacement) {
		pathData.SetX					(!horizontalOrVerticalEdge ? newPosX : 0);
		pathData.SetY					(horizontalOrVerticalEdge ? newPosY : 0);
	} else {
		pathData.SetX					(newPosX);
		pathData.SetY					(newPosY);
	}
}

std::vector<CustomPathData> CGuiConnection::GetCurrentPathCustomizations(void)
{
	std::vector<CustomPathData> cd;
	for (std::vector<CustomPathData>::iterator ii = customPathData.begin(); ii != customPathData.end(); ++ii) {
		if ((*ii).GetAspect() == parentAspect)
			cd.push_back(*ii);
	}
	return cd;
}

bool CGuiConnection::HasPathCustomization(void) const
{
	return customPathData.size() > 0;
}

bool CGuiConnection::HasPathCustomizationForCurrentAspect(int edgeIndex) const
{
	return HasPathCustomizationForAspect(view->currentAspect->index, edgeIndex);
}

bool CGuiConnection::HasPathCustomizationForAspect(long asp, int edgeIndex) const
{
	return HasPathCustomizationForTypeAndAspect(asp, Invalid, edgeIndex);
}

bool CGuiConnection::HasPathCustomizationForTypeAndCurrentAspect(PathCustomizationType custType, int edgeIndex) const
{
	return HasPathCustomizationForTypeAndAspect(view->currentAspect->index, custType, edgeIndex);
}

bool CGuiConnection::HasPathCustomizationForTypeAndAspect(long asp, PathCustomizationType custType, int edgeIndex) const
{
	for (std::vector<CustomPathData>::const_iterator ii = customPathData.begin(); ii != customPathData.end(); ++ii) {
		if ((*ii).GetAspect() == asp &&
			((*ii).GetEdgeIndex() == edgeIndex || edgeIndex == -1) &&
			((*ii).GetType() == custType || custType == Invalid))
		{
			return true;
		}
	}
	return false;
}

void CGuiConnection::ReadCustomPathData(void)
{
	customPathData.clear();
	CString pref;
	if (GetPreference(pref, CUSTOMCONNECTIONDATA)) {
		if (pref != EMPTYCONNECTIONCUSTOMIZATIONDATAMAGIC) {	// -1 is a magic number for deleted data
			CString subStr;
			int curPos = 0;
			subStr = pref.Tokenize(_T(";"), curPos);
			while (subStr != _T("")) {
				CustomPathData pathData;
				if (pathData.Deserialize(subStr))
					customPathData.push_back(pathData);
				subStr = pref.Tokenize(_T(";"), curPos);
			}
		}
	}
}

void CGuiConnection::WriteCustomPathData(bool handleTransaction)
{
	CString valStr;
	for (std::vector<CustomPathData>::iterator ii = customPathData.begin(); ii != customPathData.end(); ++ii) {
		CString edgeStr;
		(*ii).Serialize(edgeStr);
		if (valStr != _T(""))
			valStr.Append(_T(";"));
		valStr.Append(edgeStr);
	}
	VERIFY(mgaFco);
	CComBSTR pathBstr = CUSTOMCONNECTIONDATA;
	CComBSTR bstrVal;
	if (valStr == _T(""))
		bstrVal = EMPTYCONNECTIONCUSTOMIZATIONDATAMAGIC;
	else
		CopyTo(valStr, bstrVal);

	if (handleTransaction)
		view->BeginTransaction();

	if (valStr == _T("")) {
		CComPtr<IMgaRegNode> ccpMgaRegNode;
		COMTHROW(mgaFco->get_RegistryNode(pathBstr, &ccpMgaRegNode));
		COMTHROW(ccpMgaRegNode->RemoveTree());
	} else {
		COMTHROW(mgaFco->put_RegistryValue(pathBstr, bstrVal));
	}

	if (handleTransaction)
		view->CommitTransaction();
}

void CGuiConnection::InsertCustomPathData(const CustomPathData& pathData)
{
	// Note:
	//	We assume that element are in ascending order by edgeIndexes
	//	Types and aspects can be mixed up, but edgeIndexes should be in order within an aspect and type

	// Update indexes
	std::vector<CustomPathData>::iterator jj;	// insertion point
	bool found = false;
	for (std::vector<CustomPathData>::iterator ii = customPathData.begin(); ii != customPathData.end(); ++ii) {
		ASSERT((*ii).GetVersion() == pathData.GetVersion());
		if ((*ii).GetAspect() == pathData.GetAspect() &&
			(*ii).GetType() == pathData.GetType() &&
			(!IsAutoRouted() || (*ii).GetEdgeCount() == pathData.GetEdgeCount()) &&
			(*ii).GetEdgeIndex() >= pathData.GetEdgeIndex())
		{
			if (!found)
				jj = ii;
			found = true;
			if (pathData.GetType() == CustomPointCustomization)
				(*ii).SetEdgeIndex((*ii).GetEdgeIndex() + 1);
			else if (pathData.GetType() == SimpleEdgeDisplacement)
				break;
			// TODO: other cases
		}
	}
	if (found)
		// Insert
		customPathData.insert(jj, pathData);
	else
		// If we didn't find elements with bigger or equal edgeIndexes, then we should insert it as last one (ascending order)
		customPathData.push_back(pathData);
}

void CGuiConnection::UpdateCustomPathData(const CustomPathData& pathData)
{
	for (std::vector<CustomPathData>::iterator ii = customPathData.begin(); ii != customPathData.end(); ++ii) {
		ASSERT((*ii).GetVersion() == pathData.GetVersion());
		if ((*ii).GetAspect() == pathData.GetAspect() &&
			(*ii).GetType() == pathData.GetType() &&
			(!IsAutoRouted() || (*ii).GetEdgeCount() == pathData.GetEdgeCount()) &&
			(*ii).GetEdgeIndex() == pathData.GetEdgeIndex())
		{
			(*ii) = pathData;
			return;
		}
	}

	// not found => insert it, preserving order
	InsertCustomPathData(pathData);
}

void CGuiConnection::DeletePathCustomization(const CustomPathData& pathData)
{
	long fullCustomEdgeCounter = 0;
	bool found = false;
	for (std::vector<CustomPathData>::iterator ii = customPathData.begin(); ii != customPathData.end(); ++ii) {
		ASSERT((*ii).GetVersion() == pathData.GetVersion());
		if ((*ii).GetAspect() == pathData.GetAspect() &&
			(*ii).GetType() == pathData.GetType())
		{
			if (pathData.GetType() == SimpleEdgeDisplacement) {
				if ((*ii).GetEdgeIndex() == pathData.GetEdgeIndex() &&
					(*ii).GetEdgeCount() == pathData.GetEdgeCount())
				{
					ASSERT((*ii).IsHorizontalOrVertical() == pathData.IsHorizontalOrVertical());
					ii = customPathData.erase(ii);
					found = true;
					return;
				}
			} else if (pathData.GetType() == CustomPointCustomization) {
				// in case of CustomPointCustomization delete by array index and not edgeIndex
				if (fullCustomEdgeCounter == pathData.GetEdgeIndex()) {
					ii = customPathData.erase(ii);
					found = true;
					break;
				}
				fullCustomEdgeCounter++;
			} else {
				// TODO: other checks for other types
			}
		}
	}
	// update indexes in case of full customization
	if (pathData.GetType() == CustomPointCustomization) {
		for (std::vector<CustomPathData>::iterator ii = customPathData.begin(); ii != customPathData.end(); ++ii) {
			ASSERT((*ii).GetVersion() == pathData.GetVersion());
			if ((*ii).GetAspect() == pathData.GetAspect() &&
				(*ii).GetType() == pathData.GetType())
			{
				ASSERT((*ii).GetEdgeIndex() != pathData.GetEdgeIndex());	// because we just deleted it
				if ((*ii).GetEdgeIndex() > pathData.GetEdgeIndex()) {
					(*ii).SetEdgeIndex((*ii).GetEdgeIndex() - 1);
				}
			}
		}
	}
	// not found
	ASSERT(found);
}

bool CGuiConnection::DeleteAllPathCustomizationsForAnAspect(long asp)
{
	bool wereThereAnyDeletion = false;
	std::vector<CustomPathData>::iterator ii = customPathData.begin();
	while (ii != customPathData.end()) {
		if ((*ii).GetAspect() == asp &&
			(IsAutoRouted() && (*ii).GetType() == SimpleEdgeDisplacement ||
			!IsAutoRouted() && (*ii).GetType() != SimpleEdgeDisplacement))
		{
			wereThereAnyDeletion = true;
			ii = customPathData.erase(ii);
		} else {
			++ii;
		}
	}
	return wereThereAnyDeletion;
}

bool CGuiConnection::DeleteAllPathCustomizationsForCurrentAspect(void)
{
	return DeleteAllPathCustomizationsForAnAspect(view->currentAspect->index);
}

void CGuiConnection::DeleteAllPathCustomizationsForAllAspects(void)
{
	customPathData.clear();
}

void CGuiConnection::RemoveDeletedPathCustomizations(const std::vector<CustomPathData>& customPathDat)
{
	std::vector<CustomPathData>::const_iterator ii = customPathDat.begin();
	while (ii != customPathDat.end()) {
		std::vector<CustomPathData>::iterator jj = customPathData.begin();
		while (jj != customPathData.end()) {
			if ((*ii).GetAspect() == (*jj).GetAspect() &&
				(*ii).GetEdgeIndex() == (*jj).GetEdgeIndex() &&
				(*ii).GetType() == (*jj).GetType() &&
				((*ii).GetType() == CustomPointCustomization || (*ii).GetEdgeCount() == (*jj).GetEdgeCount()))
			{
				jj = customPathData.erase(jj);
			} else {
				++jj;
			}
		}
		++ii;
	}
}

void CGuiConnection::SnapCoordIfApplicable(CustomPathData* coordToSet, const CPoint& last, const CPoint& pt)
{
	double radEps = 4.0 / 360.0 * 2 * M_PI;
	RoutingDirection dir = GetDir(last - pt);
	if (dir == Dir_Skew) {
		bool modify = false;
		RoutingDirection dirToGo = GetSkewDir(last - pt);
		if ((dirToGo == Dir_Left || dirToGo == Dir_Right) && last.y != pt.y) {
			if (abs(last.y - pt.y) <= 3) {
				modify = true;
			} else if (last.x != pt.x) {
				double alpha = atan2(-((double)pt.y - last.y), (double)pt.x - last.x);
				TRACE(_T("Horizontal alpha %lf %lf\n"), alpha / M_PI * 180.0, alpha);
				if (abs(alpha + M_PI) < radEps || abs(alpha) < radEps || abs(alpha - M_PI) < radEps)
					modify = true;
			}
			if (modify)
				coordToSet->SetY(last.y);
		} else if (last.x != pt.x) {	// dirToGo == Dir_Top || dirToGo == Dir_Bottom (also can be Dir_Skew but this would be error and no_dir in case of singularity)
			if (abs(last.x - pt.x) <= 3) {
				modify = true;
			} else if (last.y != pt.y) {
				double alpha = atan2((double)pt.x - last.x, -((double)pt.y - last.y));
				TRACE(_T("Vertical alpha %lf %lf\n"), alpha / M_PI * 180.0, alpha);
				if (abs(alpha + M_PI) < radEps || abs(alpha) < radEps || abs(alpha - M_PI) < radEps)
					modify = true;
			}
			if (modify)
				coordToSet->SetX(last.x);
		}
	}
}

bool CGuiConnection::VerticalAndHorizontalSnappingOfConnectionLineSegments(long asp, int edgeIndex)
{
	if (IsAutoRouted() && customPathData.size() < 1)
		return false;

	CPointList points;
	GetPointList(points);

	int i = 0;
	CustomPathData* lastData = NULL;
	for (std::vector<CustomPathData>::iterator ii = customPathData.begin(); ii != customPathData.end(); ++ii) {
		ASSERT((*ii).GetVersion() == CONNECTIONCUSTOMIZATIONDATAVERSION);
		if ((*ii).GetAspect() == asp) {
			if ((*ii).GetType() == CustomPointCustomization) {
				CPoint pt((*ii).GetX(), (*ii).GetY());
				CPoint last;
				if (lastData != NULL) {
					last = CPoint(lastData->GetX(), lastData->GetY());
				} else {
					last = points.GetHead();
				}
				if (edgeIndex == -1 || i == edgeIndex || i == edgeIndex + 1)	// Apply snapping only to the point neighbour edges
					SnapCoordIfApplicable(&(*ii), last, pt);
				lastData = &(*ii);
				i++;
			}
		}
	}
	// see the very last connection line segment
	if (lastData != NULL && (edgeIndex == -1 || i == edgeIndex || i == edgeIndex + 1)) {
		CPoint pt(lastData->GetX(), lastData->GetY());
		CPoint last = points.GetTail();
		SnapCoordIfApplicable(lastData, last, pt);
	}

	return false;
}

bool CGuiConnection::IsAutoRouted(void) const
{
	if (view->needsConnConversion && connRegAutoRouteNotSet)
		return true;

	return isAutoRouted;
}

void CGuiConnection::SetAutoRouted(bool autoRouteState)
{
	isAutoRouted = autoRouteState;
}

bool CGuiConnection::NeedsRouterPathConversion(bool expectedAutoRouterState)
{
	TRACE(_T("NeedsRouterPathConversion %d %d %d\n"), connRegAutoRouteNotSet, isAutoRouted,
			!HasPathCustomizationForTypeAndCurrentAspect(CustomPointCustomization));
	return (connRegAutoRouteNotSet && isAutoRouted == expectedAutoRouterState &&
			!HasPathCustomizationForTypeAndCurrentAspect(CustomPointCustomization));
}

void CGuiConnection::ConvertAutoRoutedPathToCustom(long asp, bool handleTransaction, bool expectedAutoRouterState)
{
	if (NeedsRouterPathConversion(expectedAutoRouterState)) {
		CPointList points;
		GetPointList(points);
		int numPoints = points.GetSize();

		bool wasThereDataToWrite = false;
		if (numPoints > 2) {
			POSITION pos = points.GetHeadPosition();
			int i = 0;
			while (pos) {
				CPoint pt = points.GetNext(pos);
				if (i > 0 && i < numPoints - 1) {	// do not include the start and end point
					CustomPathData pathData;
					FillOutCustomPathData(pathData, CustomPointCustomization, asp, pt.x, pt.y, i - 1, numPoints - 1, true);
					wasThereDataToWrite = true;
					UpdateCustomPathData(pathData);
				}
				i++;
			}
		}

		if (wasThereDataToWrite)
			WriteCustomPathData(handleTransaction);
	}
}

bool CGuiConnection::ReadAutoRouteState(void)
{
	CString autoRoutingStateStr;

	// The parent model's setting overrides the global (theApp.useAutoRouting) settings
	bool connRegAutoRoute = view->isModelAutoRouted;
	// The connection setting overrides the global or model settings
	if (GetPreference(autoRoutingStateStr, CONNECTIONAUTOROUTING)) {
		if (autoRoutingStateStr == _T("false"))
			connRegAutoRoute = false;
		else
			connRegAutoRoute = true;
		connRegAutoRouteNotSet = false;
	} else {
		connRegAutoRouteNotSet = true;
	}
	return connRegAutoRoute;
}

void CGuiConnection::WriteAutoRouteState(bool handleTransaction)
{
	VERIFY(mgaFco);
	CComBSTR pathBstr = CONNECTIONAUTOROUTING;
	CString valStr = isAutoRouted ? _T("true") : _T("false");
	CComBSTR bstrVal;
	CopyTo(valStr, bstrVal);
	if (handleTransaction)
		view->BeginTransaction();
	COMTHROW(mgaFco->put_RegistryValue(pathBstr, bstrVal));
	if (handleTransaction)
		view->CommitTransaction();
}

CGuiConnectionLabelSet& CGuiConnection::GetLabelSet(void)
{
	return labelset;
}

////////////////////////////////////////////////
// Static methods of CGuiConnection
////////////////////////////////////////////////

////////////////////////////////////////////////
// Virtual methods of CGuiConnection
////////////////////////////////////////////////




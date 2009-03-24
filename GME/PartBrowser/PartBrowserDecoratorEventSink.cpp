//################################################################################################
//
// PartBrowser Decorator Event Sink implementation for PartBrowser
//	PartBrowserDecoratorEventSink.cpp
//
//################################################################################################

#include "StdAfx.h"
#include "PartBrowserDecoratorEventSink.h"
#include "MgaDecorator_i.c"


//################################################################################################
//
// CLASS : CPartBrowserDecoratorEventSink
//
//################################################################################################

IMPLEMENT_DYNCREATE(CPartBrowserDecoratorEventSink, CCmdTarget)


HRESULT CPartBrowserDecoratorEventSink::QuerySinkInterface(void** ppv)
{
	return m_xEventSink.QueryInterface(IID_IMgaElementDecoratorEvents, ppv);
}

CPartBrowserDecoratorEventSink::CPartBrowserDecoratorEventSink()
{
	EnableAutomation();	// only needed for Dispatch maps
}

CPartBrowserDecoratorEventSink::~CPartBrowserDecoratorEventSink()
{
}

void CPartBrowserDecoratorEventSink::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CPartBrowserDecoratorEventSink, CCmdTarget)
	//{{AFX_MSG_MAP(CPartBrowserDecoratorEventSink)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CPartBrowserDecoratorEventSink, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CPartBrowserDecoratorEventSink)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

BEGIN_INTERFACE_MAP(CPartBrowserDecoratorEventSink, CCmdTarget)
	INTERFACE_PART(CPartBrowserDecoratorEventSink, IID_IMgaElementDecoratorEvents, EventSink)
END_INTERFACE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPartBrowserDecoratorEventSink::XEventSink - native COM interface - peter
STDMETHODIMP_(ULONG) CPartBrowserDecoratorEventSink::XEventSink::AddRef()
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);
	return pThis->ExternalAddRef();
}

STDMETHODIMP_(ULONG) CPartBrowserDecoratorEventSink::XEventSink::Release()
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);
	return pThis->ExternalRelease();
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::QueryInterface(REFIID riid, void** ppv)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);
	return pThis->ExternalQueryInterface(&riid, ppv);
}


STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::Refresh()
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::OperationCanceled()
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::CursorChanged(LONG newCursorID)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::CursorRestored()
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelEditingStarted(LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelEditingFinished(LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelChanged(BSTR newLabel)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelMovingStarted(LONG nType, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelMoving(LONG nSide, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelMovingFinished(LONG nType, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelMoved(LONG nType, LONG x, LONG y)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelResizingStarted(LONG nType, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelResizing(LONG nSide, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelResizingFinished(LONG nType, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::LabelResized(LONG nType, LONG cx, LONG cy)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::GeneralOperationStarted(ULONGLONG operationData)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::GeneralOperationFinished(ULONGLONG operationData)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::WindowMovingStarted(LONG nType, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::WindowMoving(LONG nSide, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::WindowMovingFinished(LONG nType, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::WindowMoved(LONG nType, LONG x, LONG y)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::WindowResizingStarted(LONG nType, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::WindowResizing(LONG nSide, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::WindowResizingFinished(LONG nType, LONG left, LONG top, LONG right, LONG bottom)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

STDMETHODIMP CPartBrowserDecoratorEventSink::XEventSink::WindowResized(LONG nType, LONG cx, LONG cy)
{
	METHOD_PROLOGUE(CPartBrowserDecoratorEventSink,EventSink);

	return S_OK;
}

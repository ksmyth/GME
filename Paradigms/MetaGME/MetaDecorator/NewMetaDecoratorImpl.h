//################################################################################################
//
// New Meta Decorator COM side Implementation
//	NewMetaDecoratorImpl.h
//
//################################################################################################

#ifndef __NEWMETADECORATORIMPL_H_
#define __NEWMETADECORATORIMPL_H_



#include "StdAfx.h"
#define __Meta_h__
#define __Mga_h__
#define __MgaDecorator_h__
#include "DecoratorLib.h"
#include "resource.h"       // main symbols
#include "DecoratorInterface.h"


class DecoratorInterface;

//################################################################################################
//
// CLASS : CNewMetaDecoratorImpl
//
//################################################################################################

class ATL_NO_VTABLE CNewMetaDecoratorImpl :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IMgaNewDecorator,
	public CComCoClass<CNewMetaDecoratorImpl, &CLSID_NewMetaDecorator>
{
protected:
	DecoratorSDK::DecoratorInterface*	m_pNewDecorator;
	bool								m_bLocationSet;
	bool								m_bInitCallFromEx;

public:
	CNewMetaDecoratorImpl();
	~CNewMetaDecoratorImpl();

	DECLARE_REGISTRY_RESOURCEID( IDR_NEWDECORATOR )
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP( CNewMetaDecoratorImpl )
		COM_INTERFACE_ENTRY( IMgaNewDecorator )
	END_COM_MAP()

// =============== INewMetaDecorator
public:
	// =============== inherited from IMgaDecorator
	STDMETHOD( Initialize )						( /*[in]*/ IMgaProject* pProject, /*[in]*/ IMgaMetaPart* pPart, /*[in]*/ IMgaFCO* pFCO );
	STDMETHOD( Destroy )						( void );
	STDMETHOD( GetMnemonic )					( /*[out]*/ BSTR* bstrMnemonic );
	STDMETHOD( GetFeatures )					( /*[out]*/ feature_code* pFeatureCodes );
	STDMETHOD( SetParam )						( /*[in]*/ BSTR bstrName, /*[in]*/ VARIANT vValue );
	STDMETHOD( GetParam )						( /*[in]*/ BSTR bstrName, /*[out]*/ VARIANT* pvValue );
	STDMETHOD( SetActive )						( /*[in]*/ VARIANT_BOOL bIsActive );
	STDMETHOD( GetPreferredSize )				( /*[out]*/ LONG* plWidth, /*[out]*/ LONG* plHeight );
	STDMETHOD( SetLocation )					( /*[in]*/ LONG sx, /*[in]*/ LONG sy, /*[in]*/ LONG ex, /*[in]*/ LONG ey );
	STDMETHOD( GetLocation )					( /*[out]*/ LONG* sx, /*[out]*/ LONG* sy, /*[out]*/ LONG* ex, /*[out]*/ LONG* ey );
	STDMETHOD( GetLabelLocation )				( /*[out]*/ LONG* sx, /*[out]*/ LONG* sy, /*[out]*/ LONG* ex, /*[out]*/ LONG* ey );
	STDMETHOD( GetPortLocation )				( /*[in]*/ IMgaFCO* fco, /*[out]*/ LONG* sx, /*[out]*/ LONG* sy, /*[out]*/ LONG* ex, /*[out]*/ LONG* ey );
	STDMETHOD( GetPorts )						( /*[out, retval]*/ IMgaFCOs** portFCOs );
	STDMETHOD( Draw )							( /*[in]*/ HDC hdc );
	STDMETHOD( SaveState )						( void );

	// =============== INewMetaDecorator
	STDMETHOD( InitializeEx )					( /*[in]*/ IMgaProject* pProject, /*[in]*/ IMgaMetaPart* pPart, /*[in]*/ IMgaFCO* pFCO, /*[in]*/ IMgaNewDecoratorEvents* eventSink, /*[in]*/ ULONGLONG parentWnd );
	STDMETHOD( SetSelected )					( /*[in]*/ VARIANT_BOOL bIsSelected );
	STDMETHOD( MouseMoved )						( /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseLeftButtonDown )			( /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseLeftButtonUp )				( /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseLeftButtonDoubleClick )		( /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseRightButtonDown )			( /*[in]*/ ULONGLONG hCtxMenu, /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseRightButtonUp )				( /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseRightButtonDoubleClick )	( /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseMiddleButtonDown )			( /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseMiddleButtonUp )			( /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseMiddleButtonDoubleClick )	( /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MouseWheelTurned )				( /*[in]*/ ULONG nFlags, /*[in]*/ LONG distance, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( MenuItemSelected )				( /*[in]*/ ULONG menuItemId, /*[in]*/ ULONG nFlags, /*[in]*/ LONG pointx, /*[in]*/ LONG pointy, /*[in]*/ ULONGLONG transformHDC );
	STDMETHOD( OperationCanceled )				( void );
};

#endif //__NEWMETADECORATORIMPL_H_

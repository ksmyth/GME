// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__461F30A9_3BF0_11D4_B3F0_005004D38590__INCLUDED_)
#define AFX_STDAFX_H__461F30A9_3BF0_11D4_B3F0_005004D38590__INCLUDED_

#pragma once

#define NOMINMAX

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#define ISOLATION_AWARE_ENABLED 1

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS

//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT

 
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

# undef  _ATL_APARTMENT_THREADED
# undef  _ATL_SINGLE_THREADED
# define _ATL_FREE_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <CommCtrl.h>

#define wireHWND HWND

// Imports
#import "CoreLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#import "MetaLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#import "MgaLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#import "MgaUtilLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#import "ParserLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#import "GMELib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry

#include "CommonImport.h"

#define cpp_quote(x) 
#include "Interfaceversion.h"
#undef cpp_quote
typedef GMEInterfaceVersion_enum GMEInterfaceVersion;

// Don't load the MIDL-generated headers
#define __Core_h__
#define __Meta_h__
#define __Mga_h__
#define __MgaUtil_h__
#define __MgaUtilLib_h__
#define __Parser_h__
#define __Gme_h__
// End Imports

///////////////////////////////////////////////////////////////////

#define chSTR(x) #x
#define chSTR2(x) chSTR(x)
#define bookmark(desc) message(__FILE__"("\
 chSTR2(__LINE__) ") : PRAGMA-- " #desc )

///////////////////////////////////////////////////////////////////

#include <vector>
#include <list>

#include "CommonSmart.h"
#include "CommonStl.h"
#include "CommonError.h"
#include "CommonMfc.h"
#include "CommonMgaTrukk.h"
///////////////////////////////////////////////////////////////////

#ifndef __AFXTEMPL_H__
#include <afxtempl.h>
#endif

#include "Resource.h"

regaccessmode_enum regacc_translate(int x);

template <const IID* piid1, const IID* piid2>
class ATL_NO_VTABLE ISupportErrorInfoImpl2 :
	public ISupportErrorInfo
{
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(_In_ REFIID riid)
	{
		return (InlineIsEqualGUID(riid, *piid1) || InlineIsEqualGUID(riid, *piid2)) ? S_OK : S_FALSE;
	}
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__461F30A9_3BF0_11D4_B3F0_005004D38590__INCLUDED_)

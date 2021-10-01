// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__BD235B4D_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_)
#define AFX_STDAFX_H__BD235B4D_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_

#pragma once

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

#if defined(_DEBUG)
#define ADDCRASHTESTMENU
// also change it in PartBrowser/StdAfx.h
//#define ACTIVEXGMEVIEW
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#include <afxctl.h>			// Pick up the definition of AfxOleRegisterTypeLib
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxcontrolbars.h>
#include "mfcdual.h"

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>


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
#define __GmeLib_h__
#define IID_IMgaDataSource __uuidof(IMgaDataSource)
// End Imports

#include "CommonError.h"
#include "CommonSmart.h"
#include "CommonMfc.h"
#include "CommonMgaTrukk.h"

#if defined(_M_IX86)
#define PROCESSOR_ARCHITECTURE "x86"
#elif defined(_M_X64)
#define PROCESSOR_ARCHITECTURE "amd64"
#endif
#pragma comment(linker,"/manifestdependency:\"type='win32'  name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='" PROCESSOR_ARCHITECTURE "' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "AutoRoute\AutoRouterGraph.h"
#include "DynMenu.h"
#include "RecentConnStrList.h"

#define WM_USER_ZOOM					(WM_USER + 111)
#define WM_PANN_SCROLL					(WM_USER + 112)
#define WM_USER_PANNREFRESH				(WM_USER + 113)
#define WM_USER_DECOR_VIEWREFRESH_REQ	(WM_USER + 114)	// used for deferred view refreshes in decorator events
#define WM_USER_EXECUTEPENDINGREQUESTS	(WM_USER + 115)	// try to execute pending requests
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#pragma warning( push )
#pragma warning( disable : 4091 )
#include "CrashRpt.h"
#pragma warning( pop )


#endif // !defined(AFX_STDAFX_H__BD235B4D_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_)

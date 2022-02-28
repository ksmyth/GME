// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


#if !defined(AFX_STDAFX_H__270B4F89_B17C_11D3_9AD1_00AA00B6FE26__INCLUDED_)
#define AFX_STDAFX_H__270B4F89_B17C_11D3_9AD1_00AA00B6FE26__INCLUDED_

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS 1
#ifdef _DEBUG
// #define _ATL_DEBUG_INTERFACES 1
#endif

#pragma warning(3:4701) // Potentially uninitialized local variable 'name' used

#pragma once

//#define _SECURE_SCL 0
//#define _HAS_ITERATOR_DEBUGGING 0

//#define _ATL_DEBUG_INTERFACES
// If you find a leak, put this in DllMain to DebugBreak on every AddRef/Release on that interface pointer
// _AtlDebugInterfacesModule.m_nIndexBreakAt = 42;

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS

#define _ATL_APARTMENT_THREADED

#ifdef _ATL_DEBUG_INTERFACES
#pragma warning(push)
#pragma warning(disable : 4777)	// 'swprintf_s' : format string '%d' requires an argument of type 'int', but variadic argument 3 has type 'ULONG'	Core	C : \Program Files(x86)\Microsoft Visual Studio 14.0\VC\atlmfc\include\atlbase.h	2760
#include <atlbase.h>
#pragma warning(pop)
#else
#include <atlbase.h>
#endif

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#pragma warning(push,3)


#include <list>//slist
#include <string>
#include <set>
#include <queue>
#include <vector>
#include <hash_set>
#include <hash_map>
#pragma warning(pop)

#ifdef _DEBUG
#define MGA_TRACE AtlTrace
#else
inline void NOOP_TRACE2(LPCSTR, ...) { }
#define MGA_TRACE 1 ? void(0) : NOOP_TRACE2
#endif

// Imports
#import "CoreLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#import "MetaLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#import "MgaLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry

#include "CommonImport.h"

#define cpp_quote(x) 
#include "Interfaceversion.h"
#undef cpp_quote

// Don't load the MIDL-generated headers
#define __Core_h__
#define __Meta_h__
#define __Mga_h__
#define INTERFACECOLL_INCLUDED
// End Imports

#if 1
#define ASSERT ATLASSERT
#else
static int _MgaDbgReportW(const wchar_t* file, __int64 line, const wchar_t* msg)
{
	wchar_t message[1024 * 2];
	swprintf_s(message, L"Assert failed at %s:%I64d\n\nExpression: %s\n\nPress Ok to debug.", file, line, msg);
	return ::MessageBox(0, message, L"Assert failed", MB_OKCANCEL);
}

#define _MGAASSERTE(expr, msg) \
        (void) ((!!(expr)) || \
                (IDOK == _MgaDbgReportW(_CRT_WIDE(__FILE__), __LINE__, msg)) && (DebugBreak(), 0))
#define ASSERT(expr)  _MGAASSERTE((expr), _CRT_WIDE(#expr))

#endif
#include "CommonSmart.h"
#include "CommonStl.h"
#include "CommonError.h"
#include "CommonCollection.h"
#include "MgaGeneric.h"
#include "MgaCoreObj.h"
#include "MgaTrukk.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__270B4F89_B17C_11D3_9AD1_00AA00B6FE26__INCLUDED)

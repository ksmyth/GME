// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__9E9AAAD2_28B8_11D3_B36C_0060082DF884__INCLUDED_)
#define AFX_STDAFX_H__9E9AAAD2_28B8_11D3_B36C_0060082DF884__INCLUDED_

#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS 1
#ifdef _DEBUG
// #define _ATL_DEBUG_INTERFACES 1
#endif

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

// #define CORE_USE_EASTL

#ifdef CORE_USE_EASTL
#define EA_CHAR16_NATIVE 1
#include "EASTL/list.h"
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#else
#include <hash_set>
#include <hash_map>
#include <forward_list>
#endif

#include <hash_map>

namespace core { 
#ifdef CORE_USE_EASTL
	using eastl::hash_map;
	using eastl::hash_set;
	using eastl::pair;
	using eastl::list;
#else
	using stdext::hash_set;
	using stdext::hash_map;
	using std::list;
	using std::forward_list;
	using std::pair;
#endif
}

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

#ifdef _ATL_DEBUG_INTERFACES
bool IsQIThunk(IUnknown *p);
#endif

#define _NOTHREADS


#define ASSERT ATLASSERT

#include "resource.h"
#import "CoreLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#include "CommonImport.h"
#define IID_ICoreDictionaryAttributeValue __uuidof(ICoreDictionaryAttributeValue)
#define LIBID_MGACoreLib __uuidof(__MGACoreLib)

#include "CommonSmart.h"
#include "CommonStl.h"
#include "CoreUtilities.h"

#include "CoreProject.h"


extern HRESULT check_location_compatibility(ICoreMetaObject *newobj, ICoreMetaObject *oldobj);
extern HRESULT check_location_compatibility(ICoreMetaAttribute *newobj, ICoreMetaAttribute *oldobj);

extern HRESULT check_location_compatibility(ICoreObject *newobj, ICoreObject *oldobj);
extern HRESULT check_location_compatibility(ICoreAttribute *newobj, ICoreAttribute *oldobj);



#ifdef _DEBUG
//#define DEBUG_REPOSITORY
//#define DEBUG_OBJECTLOOKUP
//#define DEBUG_CONTAINERS		// this is really slow
//#define TRACE_REPOSITORY
//#define TRACE_CORE
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9E9AAAD2_28B8_11D3_B36C_0060082DF884__INCLUDED)

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__0ADEEC74_D83A_11D3_B36B_005004D38590__INCLUDED_)
#define AFX_STDAFX_H__0ADEEC74_D83A_11D3_B36B_005004D38590__INCLUDED_

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

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS

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
#include <hash_map>


#define ASSERT ATLASSERT

#import "CoreLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#import "MetaLib.tlb" no_implementation no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#include "CommonImport.h"

#include "resource.h"
//#include "Core.h"
//#include "Gme.h"
//#include "MetaLib.h"
#include "CommonSmart.h"
#include "CommonStl.h"
#include "MetaUtilities.h"
#include "CoreMetaDefines.h"

typedef IMgaConstraint IMgaMetaConstraint;
typedef IMgaConstraints IMgaMetaConstraints;

typedef CCoreCollectionEx<IMgaMetaRoles, std::vector<IMgaMetaRole*>,
			IMgaMetaRole, IMgaMetaRole, &__uuidof(MgaMetaRoles), IDR_MGAMETAROLES> RolesExCOMType;
typedef CCoreCollectionEx<IMgaMetaFCOs, std::vector<IMgaMetaFCO*>,
			IMgaMetaFCO, IMgaMetaFCO, &__uuidof(MgaMetaFCOs), IDR_MGAMETAFCOS> FCOsExCOMType;

extern HRESULT check_location_compatibility(IMgaMetaBase *newobj, IMgaMetaBase *oldobj);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0ADEEC74_D83A_11D3_B36B_005004D38590__INCLUDED)

// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#endif

#import "MgaLib.tlb" implementation_only no_namespace raw_method_prefix("") high_method_prefix("__") no_registry
#import "MgaUtilLib.tlb" implementation_only no_namespace raw_method_prefix("") high_method_prefix("__") no_registry

_locale_t c_locale = _create_locale(LC_ALL, "C");

struct _XMLPlatformUtilsCriticalSection {
	CRITICAL_SECTION section;
	_XMLPlatformUtilsCriticalSection() {
		InitializeCriticalSection(&section);
	};
	~_XMLPlatformUtilsCriticalSection() {
		DeleteCriticalSection(&section);
	}
} XMLPlatformUtilsCriticalSection;

#include <xercesc/util/PlatformUtils.hpp>
XMLPlatformUtilsTerminate_RAII::XMLPlatformUtilsTerminate_RAII() {
	EnterCriticalSection(&XMLPlatformUtilsCriticalSection.section);
	XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::Initialize();
	LeaveCriticalSection(&XMLPlatformUtilsCriticalSection.section);
}

XMLPlatformUtilsTerminate_RAII::~XMLPlatformUtilsTerminate_RAII() {
	EnterCriticalSection(&XMLPlatformUtilsCriticalSection.section);
	XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::Terminate();
	LeaveCriticalSection(&XMLPlatformUtilsCriticalSection.section);
}

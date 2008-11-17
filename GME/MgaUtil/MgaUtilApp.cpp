// MgaUtil.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <initguid.h>
#include "MgaUtilApp.h"

#include "MgaUtil_i.c"
#include "MgaUtilLib_i.c"
#include "Gme_i.c"

#include "MgaResolver.h"
#include "MgaRegistrar.h"
#include "MgaLauncher.h"
#include "MgaProgressDlg.h"
#include "MgaClosure.h"
#include "MgaEventLogger.h"
#include "ComponentProxy.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CMgaUtilApp

BEGIN_MESSAGE_MAP(CMgaUtilApp, CWinApp)
	//{{AFX_MSG_MAP(CMgaUtilApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMgaUtilApp construction

CMgaUtilApp::CMgaUtilApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMgaUtilApp object

CMgaUtilApp theApp;

	
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_MgaResolver, CMgaResolver)
OBJECT_ENTRY(CLSID_MgaRegistrar, CMgaRegistrar)
OBJECT_ENTRY(CLSID_MgaLauncher, CMgaLauncher)
OBJECT_ENTRY(CLSID_MgaProgressDlg, CMgaProgressDlg)
OBJECT_ENTRY(CLSID_ComponentProxy, CComponentProxy)
OBJECT_ENTRY(CLSID_MgaClosure, CMgaClosure)
OBJECT_ENTRY(CLSID_MgaEventLogger, CMgaEventLogger)
END_OBJECT_MAP()

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}
/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	_Module.UpdateRegistryFromResource(IDR_MGAUTIL, TRUE);
	return _Module.RegisterServer(TRUE);
}
/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	_Module.UpdateRegistryFromResource(IDR_MGAUTIL, FALSE);
	_Module.UnregisterServer(TRUE); //TRUE indicates that typelib is unreg'd
	return S_OK;
}

BOOL CMgaUtilApp::InitInstance()
{
	if (!InitATL())
		return FALSE;

	return CWinApp::InitInstance();

}

int CMgaUtilApp::ExitInstance()
{
	_Module.Term();

	return CWinApp::ExitInstance();

}

BOOL CMgaUtilApp::InitATL()
{
	_Module.Init(ObjectMap, AfxGetInstanceHandle());
	return TRUE;

}

regaccessmode_enum regacc_translate(int x) {
	if(x == 0) return REGACCESS_SYSTEM;
	if(x == 1) return REGACCESS_USER;
	if(x == 2) return REGACCESS_BOTH;
	ASSERT(false);
	return REGACCESS_NONE;
}

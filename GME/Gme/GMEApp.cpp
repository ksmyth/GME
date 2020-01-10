// GMEApp.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <afxwin.h> // for CWaitCursor
#include "GMEApp.h"
#include "GMEVersion.h"


#include "Parser.h"

#include "GMEstd.h"
#include "GuiMeta.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "GMEDoc.h"
#include "GMEView.h"
#include "GMEChildFrame.h"
#include "GMEBrowser.h"
#include "GMEObjectInspector.h"
#include <locale.h>
#include "MgaOpenDlg.h"
#include "GmeDocTemplate.h"
#include "ProjectPropertiesDlg.h"
#include "GmeLib.h"
#include "GMEOLEApp.h"
#include "GMEEventLogger.h"
#include "GMEPrintDialog.h"
#include <sstream>

#include "CrashRpt.h"
#ifdef _DEBUG
#pragma comment(lib, "CrashRpt1403.lib")
// https://msdn.microsoft.com/en-us/library/bb531344.aspx#BK_CRT
#pragma comment(lib, "legacy_stdio_definitions.lib")
#else
#pragma comment(lib, "CrashRpt1403.lib")
#endif

#import "C:\WINDOWS\Microsoft.NET\Framework\v2.0.50727\mscorlib.tlb" no_implementation
namespace CSGUI {
using namespace mscorlib;
}
#import "CSGUI.tlb" no_implementation

#ifndef _DEBUG
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Core' version='1.0.0.0' language='*'\"")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Meta' version='1.0.0.0' language='*'\"")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Mga' version='1.0.0.0' language='*'\"")
#endif

#include "CrashTest.h"
#include <Gdiplus.h>
#include "GraphicsUtil.h"
#include <signal.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// ATL ///////////////////////////////////////////////////////////////////////////
#include <atlsafe.h>
#include "mga_i.c"
#include "meta_i.c"
#include "Splash.h"

CComModule _Module;


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CGMEApp

BEGIN_MESSAGE_MAP(CGMEApp, CWinAppEx)
	//{{AFX_MSG_MAP(CGMEApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEPROJECT, OnUpdateFileCloseproject)
	ON_COMMAND(ID_FILE_CLOSEPROJECT, OnFileCloseproject)
	ON_UPDATE_COMMAND_UI(ID_FILE_NEW, OnUpdateFileNew)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, OnUpdateFileOpen)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_UPDATE_COMMAND_UI(ID_FILE_EXPORTXML, OnUpdateFileExportxml)
	ON_COMMAND(ID_FILE_EXPORTXML, OnFileExportxml)
	ON_COMMAND(ID_FILE_IMPORTXML, OnFileImportxml)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PROJECTPROPERTIES, OnUpdateEditProjectproperties)
	ON_COMMAND(ID_EDIT_PROJECTPROPERTIES, OnEditProjectproperties)
	ON_COMMAND(ID_FILE_SETTINGS, OnFileSettings)
	ON_COMMAND(ID_FILE_CLEARLOCKS, OnFileClearLocks)
	ON_COMMAND(ID_EDIT_CLEARUNDO, OnEditClearUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEARUNDO, OnUpdateEditClearUndo)
	ON_COMMAND(ID_HELP_CONTENTS, OnHelpContents)
	ON_COMMAND(ID_FILE_CHECKALL, OnFileCheckall)
	ON_UPDATE_COMMAND_UI(ID_FILE_CHECKALL, OnUpdateFileCheckall)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEPROJECT, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEPROJECTAS, OnUpdateFileSaveAs)
	ON_COMMAND(ID_FILE_SAVEPROJECTAS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVEPROJECT, OnFileSave)
	ON_COMMAND(ID_FILE_ABORTPROJECT, OnFileAbortProject)
	ON_UPDATE_COMMAND_UI(ID_FILE_ABORTPROJECT, OnUpdateFileAbortProject)
	ON_COMMAND(ID_FILE_REGCOMPONENTS, OnFileRegcomponents)
	ON_COMMAND(ID_FILE_REGPARADIGMS, OnFileRegparadigms)
	ON_UPDATE_COMMAND_UI(ID_FILE_REGCOMPONENTS, OnUpdateFileRegcomponents)
	ON_UPDATE_COMMAND_UI(ID_FILE_UPDATETHROUGHXML, OnUpdateFileXMLUpdate)
	ON_COMMAND(ID_FILE_UPDATETHROUGHXML, OnFileXMLUpdate)
	ON_COMMAND(ID_FILE_DISPLAY_CONSTRAINTS, OnFileDisplayConstraints)
	ON_UPDATE_COMMAND_UI(ID_FILE_DISPLAY_CONSTRAINTS, OnUpdateFileDisplayConstraints)
	ON_COMMAND(ID_APP_EXIT, OnAppExit)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	//}}AFX_MSG_MAP
	// Standard file based document commands
//	ON_COMMAND(ID_FILE_NEW, CWinAppEx::OnFileNew)
//	ON_COMMAND(ID_FILE_OPEN, CWinAppEx::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, OnUniquePrintSetup) // CWinAppEx::OnFilePrintSetup)
	// MRU - most recently used project menu
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_PRJ1, OnUpdateRecentProjectMenu)
	ON_COMMAND_EX_RANGE(ID_FILE_MRU_PRJ1, ID_FILE_MRU_PRJ16, OnOpenRecentProject)
	ON_COMMAND_RANGE(ID_FILE_RUNPLUGIN1, ID_FILE_RUNPLUGIN_LAST, OnRunPlugin)
	ON_COMMAND_RANGE(ID_FILE_INTERPRET1, ID_FILE_INTERPRET_LAST, OnRunInterpreter)
	ON_COMMAND_RANGE(ID_HELP_PARADIGMHELP_FIRST, ID_HELP_PARADIGMHELP_LAST, OnRunParadigmHelp)
	ON_COMMAND_RANGE(ID_HELP_COMPONENTHELP_FIRST, ID_HELP_COMPONENTHELP_LAST, OnRunComponentHelp)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_RUNPLUGIN1, ID_FILE_RUNPLUGIN_LAST, OnUpdateFilePluginX)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_INTERPRET1, ID_FILE_INTERPRET_LAST, OnUpdateFileInterpretX)
	ON_COMMAND(ID_FOCUS_BROWSER, OnFocusBrowser)
	ON_COMMAND(ID_FOCUS_INSPECTOR, OnFocusInspector)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// The one and only CGMEApp object

CGMEApp theApp;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.


/*static*/ const TCHAR * CGMEApp::m_no_model_open_string = _T("_NO_MODEL_IS_OPEN_");

CComBSTR StringFromGUID2(const CComVariant& guid)
{
	ASSERT(guid.vt == (VT_UI1 | VT_ARRAY));
	GUID guid2;
	CopyTo(guid, guid2);
	CComBSTR strGuid;
	CopyTo(guid2, &strGuid.m_str);
	return strGuid;
}

/////////////////////////////////////////////////////////////////////////////
// CGMEApp construction

CGMEApp::CGMEApp() :
    m_RecentProjectList(0, _T("Recent Project List"), _T("Project%d"), 8)
  , m_compFilterOn( false)
{
	multipleView = false;
	useAutoRouting = true;
	labelAvoidance = false;
	defZoomLev = _T("100");
	mouseOverNotify = false;
	maintainHistory = false;
	realFmtStr = _T("%.12g");
	// TODO: add construction code here,

	// Place all significant initialization in InitInstance
	SIZE size;
	size.cx = 16;
	size.cy = 16;
	m_userImages.SetImageSize(size);
}

CGMEApp::~CGMEApp()
{
}

class CGMECommandLineInfo : public CCommandLineInfo {
public:
 	bool bNoProtect, bOpenLast;
	CString run;
 	CGMECommandLineInfo() : bNoProtect(false), bOpenLast(false) { ; }

	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) {
		
		if(bFlag && !_wcsicmp(pszParam, L"d")) bNoProtect = true;
 		else if(bFlag && !_wcsicmp(pszParam, L"l")) bOpenLast = true;
		else if(bFlag && _wcsnicmp(pszParam, L"run:", 4) == 0)
		{
			run = pszParam + 4;
		}
		else CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
	}

};


bool CheckInterfaceVersions()	{
		bool err = false;
		LPCOLESTR components[] = {
			L"Mga.MgaProject",
			L"Mga.MgaMetaProject",
			L"Mga.CoreProject",
			L"Mga.MgaRegistrar",
			L"Mga.MgaParser",
			L"Mga.AddOn.ConstraintManager",
			NULL };
		CString errstring;
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		TCHAR hrbuf[20];
		if(hr != S_OK) AfxMessageBox(CString(_T("Coinitialize failure. Err: #")) + _ltot(hr,hrbuf,16));
		for(LPCOLESTR *p = components; *p; p++) {
			GMEInterfaceVersion verid = GMEInterfaceVersion_None;
			CComPtr<IUnknown> unk;
			CString ee;
			if(S_OK != (hr = unk.CoCreateInstance(*p))) {
				{
					_com_error error(hr);
					errstring.Format(_T("Cannot create object %%s (Err: #%X, %s)"), hr, error.ErrorMessage());
				}
gerr:
				CString a;
				a.Format(errstring, *p, verid/0x1000, verid%0x1000, GMEInterfaceVersion_Current/0x1000,GMEInterfaceVersion_Current%0x1000);
				AfxMessageBox(a);
				err = true;
				continue;
			}
			CComQIPtr<IGMEVersionInfo> vinf = unk;
			if(!vinf) {
				errstring = _T("Incompatible version of object %s (does not support version information)");
				goto gerr;
			}
			if(S_OK != vinf->get_version(&verid)) {
				errstring = _T("Get_Version failed for object %s");
				goto gerr;
			}
			if(verid != GMEInterfaceVersion_Current) {
				errstring = _T("Interface version for class %s (%d.%d) differs from GME interface version (%d.%d)");
				goto gerr;
			}
		}
		return err;
}


CString getUserRegistryStringValue(const TCHAR* name) {
	CString ret;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\GME\\"),
		0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		TCHAR szData[256];
		DWORD dwKeyDataType;
		DWORD dwDataBufSize = sizeof(szData) / sizeof(TCHAR);

		if (RegQueryValueEx(hKey, name, NULL, &dwKeyDataType,
			(LPBYTE)&szData, &dwDataBufSize) == ERROR_SUCCESS)
		{
			ret = CString(szData, dwDataBufSize);
		}

		RegCloseKey(hKey);
	}
	return ret;
}

/////////////////////////////////////////////////////////////////////////////
// CGMEApp initialization

BOOL CGMEApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

#if _MSC_VER < 1700
	// See MSDN example code for CWinApp::InitInstance: http://msdn.microsoft.com/en-us/library/ae6yx0z0.aspx
	// MFC module state handling code is changed with VC80.
	// We follow the Microsoft's suggested way, but in case of any trouble the set the
	// HKCU\Software\GME\AfxSetAmbientActCtxMod key to 0
	UINT uAfxSetAmbientActCtxMod = 1;
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\GME\\"),
					 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
	{
		TCHAR szData[128];
		DWORD dwKeyDataType;
		DWORD dwDataBufSize = sizeof(szData)/sizeof(TCHAR);

		if (RegQueryValueEx(hKey, _T("AfxSetAmbientActCtxMod"), NULL, &dwKeyDataType,
							(LPBYTE) &szData, &dwDataBufSize) == ERROR_SUCCESS)
		{
			uAfxSetAmbientActCtxMod = _tcstoul(szData, NULL, 10);
		}

		RegCloseKey(hKey);
	}
	if (uAfxSetAmbientActCtxMod != 0)
	{
		AfxSetAmbientActCtx(FALSE);
	}
#endif

	CWinAppEx::InitInstance();

	// CG: The following block was added by the Splash Screen component.
	CGMECommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	
	if( cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen || cmdInfo.m_nShellCommand == CCommandLineInfo::AppRegister)
		;
	cmdInfo.m_bShowSplash = false;

	CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);
	// CG: The following block was inserted by 'Status Bar' component.
	{
		//Set up date and time defaults so they're the same as system defaults
		setlocale(LC_ALL, "");
	}

	//if(CheckInterfaceVersions()) return FALSE;


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	//Enable3dControls(); // deprecated
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif


	// Enable assigning shortcut keys to the menus
	InitKeyboardManager();

	// Enabling user tools
	EnableUserTools(ID_TOOLS_EXTERNAL_TOOLS, ID_USER_TOOL1, ID_USER_TOOL10, RUNTIME_CLASS(CUserTool)/*, IDR_MENU_ARGS, IDR_MENU_DIRS*/);

	InitContextMenuManager();

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("GME\\GUI"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)
	m_RecentProjectList.ReadList();


	// Load Accelerator Table for CGMEView

	m_GMEView_hAccel = LoadAccelerators(m_hInstance, MAKEINTRESOURCE(IDR_GMEVIEW));
	ASSERT( m_GMEView_hAccel );

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	pDocTemplate = new CGmeDocTemplate(
		IDR_GMETYPE,
		RUNTIME_CLASS(CGMEDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
#if !defined(ACTIVEXGMEVIEW)
		RUNTIME_CLASS(CGMEView));
#else
		RUNTIME_CLASS(CGMEChildFrame));
#endif
	pDocTemplate->SetContainerInfo(IDR_GMETYPE_CNTR_IP);
	AddDocTemplate(pDocTemplate);



	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		// n.b. if LoadFrame fails, CFrameWnd::PostNcDestroy is called which does `delete this`
		// delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;
	pMainFrame->CheckForOffscreenPanes();

#if !defined(ADDCRASHTESTMENU) && defined(_DEBUG)
	bNoProtect = true;
#else
	bNoProtect = cmdInfo.bNoProtect;
#endif
	
	//initialize the logfile - first check registry to see if the user wants logging
	MSGTRY
	{
		CComPtr<IMgaRegistrar> registrar;
		COMTHROW(registrar.CoCreateInstance(CComBSTR(L"Mga.MgaRegistrar")));
		VARIANT_BOOL enablelogging;
		COMTHROW( registrar->get_EventLoggingEnabled(REGACCESS_USER, &enablelogging) );
		if(enablelogging != VARIANT_FALSE)
		{
			CGMEEventLogger::initialize();
			CGMEEventLogger::LogGMEEvent("GME started\r\n");
		}
	}
	MSGCATCH(_T("Error while trying to get logfile settings"),;);

	ASSERT((CMainFrame*)m_pMainWnd);
	CGMEOLEApp *t_pGmeOleApp = new CGMEOLEApp();
	((CMainFrame*)m_pMainWnd)->setGmeOleApp( t_pGmeOleApp );

	// Register all OLE server factories as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleObjectFactory::RegisterAll();
		// Note: MDI applications register all server objects without regard
		//  to the /Embedding or /Automation on the command line.
	// Note: we switched the default REGCLS_MULTIPLEUSE behavior to REGCLS_SINGLEUSE,
	// see GMEOLEApp.cpp's MY_IMPLEMENT_OLECREATE macro, and
	// "How to use single or multiple instances of an OLE object in MFC by using Visual C++" KB article:
	// http://support.microsoft.com/kb/141154

	// Check to see if launched as OLE server
	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		// Application was run with /Embedding or /Automation.  Don't show the
		//  main window in this case.

		//
		// load settings in this case as well
		GetSettings();

		return TRUE;
	}

	// Updates system registry with all creatable classes (_OLECREATE)
	// PETER: It works only with administrator priviledges
	COleObjectFactory::UpdateRegistryAll();
	
	// Make sure the type library is registered or dual interface won't work.
	AfxOleRegisterTypeLib(AfxGetInstanceHandle(), __uuidof(__GmeLib));

	// Dispatch commands specified on the command line
	if ((cmdInfo.m_nShellCommand == CCommandLineInfo::AppRegister || cmdInfo.m_nShellCommand == CCommandLineInfo::AppUnregister) && !ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();


	dynmenus_need_refresh = true;

	GetSettings();
	m_pMainWnd->DragAcceptFiles(TRUE);

	return TRUE;
}

BOOL CGMEApp::PreTranslateMessage(MSG* pMsg)
{
	// CG: The following lines were added by the Splash Screen component.
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;
	// KMS: m_pMainWnd->PreTranslateMessage here for Alt-F,C. Otherwise, CGMEView::PreTranslateMessage TranslateAccelerator will return TRUE
	if (m_pMainWnd && m_pMainWnd->PreTranslateMessage(pMsg))
		return TRUE;

	return CWinAppEx::PreTranslateMessage(pMsg);
}



/////////////////////////////////////////////////////////////////////////////
// CGMEApp emergency

void CGMEApp::EmergencySave(void)
{
	CGMEEventLogger::LogGMEEvent("EMERGENCY EVENT\r\n");
	if (mgaProject && (proj_type_is_mga || proj_type_is_xmlbackend)) {
		CString embackupname = currentConnection;
		int p = embackupname.ReverseFind('.');
		if ((p == -1) || embackupname.Find('\\', p) != -1)
			p = embackupname.GetLength();
		CString emcode;
		static int emnum;
		emcode.Format(_T("-emergency%ld"), ++emnum);
		embackupname.Insert(p, emcode);
		long status = 0;
		mgaProject->get_ProjectStatus(&status); // ignore failure
		if (status == 3 || status == 4) // i.e. in tx
			mgaProject->AbortTransaction(); // ignore failure
		HRESULT hr = mgaProject->Save(PutInBstr(embackupname), VARIANT_TRUE);
		if (proj_type_is_xmlbackend) {
			AfxMessageBox(_T("Your current work can be found in the local checkout directory."));
		} else {
			CString emergencySaveMsg;
			emergencySaveMsg.FormatMessage(_T("Your current work %1 been saved to %2.\n")
										   _T("The original project file has not been modified. ")
										   _T("We apologize for the inconvenience."),
										   (hr == S_OK)? _T("has") : _T("might have"),
										   embackupname);
			AfxMessageBox(emergencySaveMsg);
			m_RecentProjectList.AddAndWriteList(embackupname);
		}
	}
}

BOOL CGMEApp::OpenCommandLineProject()
{
	CGMECommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	// We don't want a new document at startup
	if(cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	if (cmdInfo.m_nShellCommand == CCommandLineInfo::AppRegister)
		cmdInfo.bOpenLast = true;
	if(cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen)
	{
		CString conn = cmdInfo.m_strFileName;
		if(conn.Find(_T("=")) < 0) {
			if (conn.Right(4).CompareNoCase(_T(".xme")) == 0)
				conn.Insert(0, _T("XML="));
			else
				conn.Insert(0,_T("MGA="));
		}
		OpenProject(conn);

		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
		return TRUE;
	}
 	else if(cmdInfo.bOpenLast && !m_RecentProjectList[0].IsEmpty())
	{
 		OpenProject(m_RecentProjectList[0]);
 		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
		return TRUE;
 	} else if(cmdInfo.run != "")
	{
		if (CMainFrame::theInstance)
			CMainFrame::theInstance->m_console.m_Console.RunCode(CComBSTR(cmdInfo.run));
		return TRUE;
	}

	// Dispatch commands specified on the command line
	//if (!ProcessShellCommand(cmdInfo))
	//	return FALSE;
	if (!(cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated))
	{
		ShowWelcomeWindow();
	}
	return FALSE;
}

BOOL CGMEApp::ShowWelcomeWindow()
{
	CSGUI::_WelcomeScreenExpPtr ws;
	ws.CreateInstance(L"CSGUI.WelcomeScreenExp");
	if (ws)
	try {
		ATL::CComSafeArray<BSTR> recents;
		for (int i = 0; i < m_RecentProjectList.GetSize(); i++)
		{
			recents.Add(CComBSTR(m_RecentProjectList[i]));
		}
		VARIANT vrecents;
		vrecents.parray = (SAFEARRAY*)recents;
		vrecents.vt = VT_ARRAY | VT_BSTR;
		ws->SetRecentProjects(_variant_t(vrecents));
		_bstr_t SelectedProject = ws->ShowWelcomeWindow((unsigned long)m_pMainWnd->GetSafeHwnd());

		if (SelectedProject != _bstr_t()) {
			if (SelectedProject == ws->CREATE_SENTINEL) {
				OnFileNew();
			} else {
				CString conn = static_cast<const wchar_t*>(SelectedProject);
				if(conn.Find(_T("=")) < 0) {
					if (conn.Right(4).CompareNoCase(_T(".xme")) == 0)
						conn.Insert(0, _T("XML="));
					else if (conn.Right(4).CompareNoCase(_T(".mgx")) == 0)
					{
						CString filename, dirname;
						GetFullPathName(conn, filename, dirname);
						conn = CString(L"MGX=\"") + dirname + L"\"";
					}
					else
						conn.Insert(0,_T("MGA="));
				}
				OpenProject(conn);
			}
		}
	} catch (_com_error& e) {
		//AfxMessageBox(e.ErrorMessage());
	}
	return TRUE;
}

// CInvalidArgException _simpleInvalidArgException(FALSE);
static void (__stdcall *InvalidArg)() = nullptr;
void __stdcall InvalidArgReplacement() {
	CR_EXCEPTION_INFO ei;
	memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
	ei.cb = sizeof(CR_EXCEPTION_INFO);
	ei.exctype = CR_SEH_EXCEPTION;
	ei.code = 1234;
	ei.pexcptrs = NULL;

	int result = crGenerateErrorReport(&ei);

	if (result != 0)
	{
		TCHAR szErrorMsg[256];
		crGetLastErrorMsg(szErrorMsg, 256);
		AfxMessageBox(szErrorMsg);
	}
	// FIXME: this shows "Internal application error" instead of invalid argument
	throw new CInvalidArgException();
}

static void HookInvalidArg() {
#if defined(_M_IX86)
	// reg add HKCU\Software\GME /v HookInvalidArg /d 1 /t REG_SZ /f
	if (getUserRegistryStringValue(L"HookInvalidArg") == L"1") {
		InvalidArg = &AfxThrowInvalidArgException;
		int** invalidArg = (int**)(2 + ((char*)(void*)InvalidArg));
		DWORD oldProtect;
		if (VirtualProtect((LPVOID)**invalidArg, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
			int invalidArgHook = (int)(void*)&InvalidArgReplacement;
			ptrdiff_t diff = invalidArgHook - **invalidArg;
			diff -= 5;
			*((char*)**invalidArg) = 0xe9;
			*((int*)(1 + (char*)**invalidArg)) = diff;
			// AfxThrowInvalidArgException();
		}
	}
#endif
}

int CGMEApp::Run()
{
	CoFreeUnusedLibraries();	// JIRA 221: GME 9.12.15 Crashing
								// Unload ConstraintManager.dll, GME crashes if the system unloads it by itself

	Gdiplus::GdiplusStartupInput  gdiplusStartupInput;
	Gdiplus::GdiplusStartupOutput  gdiplusStartupOutput;
	ULONG_PTR gdiplusToken;
	ULONG_PTR gdiplusHookToken;

	// Tihamer: Initializing GDI+
	// See "Special CWinApp Services" MSDN topic http://msdn.microsoft.com/en-us/library/001tckck.aspx
	gdiplusStartupInput.SuppressBackgroundThread = TRUE;
	VERIFY(Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, &gdiplusStartupOutput) == Gdiplus::Ok);
	gdiplusStartupOutput.NotificationHook(&gdiplusHookToken);
	graphics.Initialize();


	// KMS: set up CrashRpt info
	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);
	info.pszAppName = _T("GME");
	info.pszAppVersion = _T(GME_VERSION_ID);
	info.pszEmailSubject = _T("GME CrashRpt");
	info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;
	info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY;
	info.pszUrl = _T("https://gme-crashrpt.metamorphsoftware.com/crashrpt");
	info.dwFlags = CR_INST_SEH_EXCEPTION_HANDLER | CR_INST_PURE_CALL_HANDLER | CR_INST_SECURITY_ERROR_HANDLER
		| CR_INST_INVALID_PARAMETER_HANDLER | CR_INST_SIGABRT_HANDLER | CR_INST_SIGINT_HANDLER | CR_INST_SIGTERM_HANDLER;
		// missing: CR_INST_NEW_OPERATOR_ERROR_HANDLER: the default std::bad_alloc is fine (sometimes we handle it)
#ifdef _DEBUG
	bNoProtect = true;
#endif
	auto disableCrashRpt = []() -> bool {
		// reg add HKCU\Software\GME /v InstallCrashRpt /d 0 /t REG_SZ /f
		if (getUserRegistryStringValue(L"InstallCrashRpt") == L"0") {
			return true;
		}
		return false;
	};
	bNoProtect = bNoProtect || disableCrashRpt();
	bNoProtect = bNoProtect || static_cast<bool>(IsDebuggerPresent());
	if (!bNoProtect) {
		if (crInstall(&info) != 0)
		{
			TCHAR buff[1024];
			crGetLastErrorMsg(buff, 1024);
			AfxMessageBox(buff);
			return FALSE;
		}
		HookInvalidArg();
	}

	OpenCommandLineProject();

	int retVal = 0;
	if (bNoProtect) {
		retVal = CWinAppEx::Run();
	} else {
		__try {
			retVal = CWinAppEx::Run();
		}
		__except(crExceptionFilter(GetExceptionCode(), GetExceptionInformation())) {
			EmergencySave();

			// KMS: As our state may be corrupted, TerminateProcess so we don't run any atexits and crash again
			TerminateProcess(GetCurrentProcess(), GetExceptionCode());
		}
	}
	// KMS: need to unload decorators before closing GDI+: e.g. ~Facilities() does GDI+ ops
	CoFreeUnusedLibraries();
	// Closing GDI+
	graphics.Uninitialize();
	gdiplusStartupOutput.NotificationUnhook(gdiplusHookToken);
	Gdiplus::GdiplusShutdown(gdiplusToken);

	return retVal;
}

/////////////////////////////////////////////////////////////////////////////
// CGMEApp Autosave
void CGMEApp::Autosave()
{

	if(mgaProject == NULL || (0&&!proj_type_is_mga) || (!autosaveEnabled)) {
		// The KillTimer function does not remove WM_TIMER messages 
		// already posted to the message queue ...
		CGMEEventLogger::LogGMEEvent(_T("WARNING: CGMEApp::Autosave was called with no active project or autosave disabled.\r\n"));
		return;
	}

	// Figure out the filename
	CString conn;
	if (autosaveUseDir) {
		int p = currentConnection.ReverseFind('\\');
		CString fname = currentConnection.Mid(p + 1);

		conn = CString(_T("MGA=")) + autosaveDir + CString(_T("\\")) + fname + GME_AUTOSAVE_EXTENSION;
	}
	else {
		conn = currentConnection + GME_AUTOSAVE_EXTENSION;
	}

	// autosave in case of xmlbackend means saving into the sourcecontrol
	if( proj_type_is_xmlbackend)
		conn = currentConnection;

	// Check if in transaction
	bool inTrans;
	try {
		long status;
		COMTHROW(mgaProject->get_ProjectStatus(&status));
		inTrans = (status & 0x08L) != 0;
	}
	catch(hresult_exception &) {
		// Safe presumption
		inTrans = true;
	}

	if (inTrans) {
		CGMEEventLogger::LogGMEEvent(_T("WARNING: CGMEApp::Autosave failed ") + 
			conn + _T(". We are in transaction.\r\n"));
		return;
	}

	// Save the project
	try {
		COMTHROW(mgaProject->Save(CComBSTR(conn), VARIANT_TRUE));
		CGMEEventLogger::LogGMEEvent(_T("CGMEApp::Autosave succeeded ") + 
			conn + _T("\r\n"));
	}
	catch(hresult_exception &e) {
		CGMEEventLogger::LogGMEEvent(_T("WARNING: CGMEApp::Autosave failed ") + 
			conn + _T(" ") + e.what() + _T("\r\n"));
	}
}


/////////////////////////////////////////////////////////////////////////////
// CGMEApp MGA project management


void CGMEApp::CloseProject(bool updateStatusBar, bool force_abort)
{

	CWaitCursor wait;
	
	if(CGMEDataSource::myData) // it is true when object copied to the clipboard from the view
		OleFlushClipboard();
	
	if( CMainFrame::theInstance != NULL ) {
		CMainFrame::theInstance->StopAutosaveTimer();
		CMainFrame::theInstance->SetPartBrowserMetaModel(NULL);
		CMainFrame::theInstance->SetPartBrowserBg(::GetSysColor(COLOR_APPWORKSPACE));
	}

	if( CGMEBrowser::theInstance != NULL )
		CGMEBrowser::theInstance->CloseProject();
	
	if(CGMEObjectInspector::theInstance!=NULL)
		CGMEObjectInspector::theInstance->CloseProject();

	if(CGMESearch::theInstance!=NULL)
		CGMESearch::theInstance->CloseProject();

	if (CGMEPartBrowser::theInstance)
		CGMEPartBrowser::theInstance->SetProject(CComPtr<IMgaProject>(0));



	if( guiMetaProject != NULL )
	{
		delete guiMetaProject;
		guiMetaProject = NULL;
	}

	mgaMetaProject = NULL;
	UpdateComponentLists();

	if( mgaProject != NULL ) {
		mgaClient = NULL;
		if(mgaProject->Close((abort_on_close || force_abort) ? VARIANT_TRUE : VARIANT_FALSE) != S_OK) {
			AfxMessageBox(CString(_T("Error occurred ")) + ((abort_on_close || force_abort) ? _T("aborting") : _T("closing")) + _T(" the project"));
		}
		mgaProject.Release();
	}
	

	if(updateStatusBar) {
		CMainFrame::theInstance->WriteStatusParadigm(_T("-"));
		CMainFrame::theInstance->WriteStatusMode(_T("EDIT"));
		CMainFrame::theInstance->WriteStatusZoom(100);
	}
	projectName.Empty();
	ChangedProjectConnStrings();

	if(CGMEDoc::theInstance)
		CGMEDoc::theInstance->SetTitle(_T(""));
	if(CMainFrame::theInstance) {
		CMainFrame::theInstance->SetTitle(m_pszAppName);
		CMainFrame::theInstance->UpdateTitle(0);//WAS: "" .By passing 0 instead of "" we won't get title such as "GME-" after a project was closed
	}
}


void CGMEApp::ChangedProjectConnStrings() {
	if(!mgaProject) {  // project closed
		paradigmDir.Empty();
		projectDir.Empty();
		proj_type_is_mga = false;
        proj_type_is_xmlbackend = false;
		return;
	}
	CString conn, metaconn;
	CComBstrObj cc;
	conn = cc;
	COMTHROW(mgaProject->get_ProjectConnStr(PutOut(cc)));
	conn = cc;
	cc.Empty();
	COMTHROW(mgaProject->get_ParadigmConnStr(PutOut(cc)));
	metaconn = cc;
	if(!conn.IsEmpty()) {
		TCHAR cd[200];
		GetCurrentDirectory(200, cd);
		projectDir = cd;
		if (conn.Find(_T("MGA=")) == 0) {
			proj_type_is_mga = true;
			int epos = conn.ReverseFind('\\');
			if(epos >= 4) {
				projectDir = conn.Mid(4, epos-4);
				if(projectDir.IsEmpty()) projectDir= '\\';
				SetCurrentDirectory(projectDir);
			}
		}
		else proj_type_is_mga = false;

        if( conn.Find(_T("MGX=")) == 0 ) 
            proj_type_is_xmlbackend = true;
        else
            proj_type_is_xmlbackend = false;
	}       
	if(!metaconn.IsEmpty()) {
		if (metaconn.Find(_T("MGA=")) == 0) {
			int epos = metaconn.ReverseFind('\\');
			if(epos >= 4) {
				paradigmDir = metaconn.Mid(4, epos-4);
				if(paradigmDir.IsEmpty()) paradigmDir= '\\';
			}
		}
	}
}


// throws exceptions!!
void CGMEApp::UpdateProjectName(bool retrievePath) {
	if( mgaProject == NULL ) {
		projectName.Empty();
	}
	else {


		CComObjPtr<IMgaTerritory> terry;
		COMTHROW(mgaProject->CreateTerritory(NULL, PutOut(terry), NULL) );
		CComBSTR nm;
	
		MSGTRY
		{
			COMTHROW(mgaProject->BeginTransaction(terry,TRANSACTION_READ_ONLY));
			COMTHROW(mgaProject->get_Name(&nm));
			COMTHROW(mgaProject->CommitTransaction());
		}
		MSGCATCH(_T("Error getting project name"), mgaProject->AbortTransaction())

		CopyTo(nm,projectName);
	}

	if(CGMEDoc::theInstance)
		CGMEDoc::theInstance->SetTitle(projectName);
	UpdateMainFrameTitle(projectName, retrievePath);
}

void CGMEApp::UpdateMainFrameTitle(const CString& projName, bool retrievePath)
{
	CString projectName = projName;

	if (retrievePath) {
		CDocument* pDocument = CGMEDoc::theInstance;
		if (pDocument) {
			POSITION pos = pDocument->GetFirstViewPosition();
			while (pos != NULL) {
#if !defined(ACTIVEXGMEVIEW)
				CGMEView* pView = (CGMEView*) pDocument->GetNextView(pos);
				ASSERT(pView);
				pView->RetrievePath();
				pView->SetTitles();
			}
#endif
		}
	}
	CMDIChildWnd* pChild  = CMainFrame::theInstance->MDIGetActive();
	if (pChild && pChild->IsKindOf(RUNTIME_CLASS(CChildFrame))) {
#if !defined (ACTIVEXGMEVIEW)
		if (retrievePath) {
			CGMEView* view = (CGMEView*)pChild->GetActiveView();
			if (view) {
				view->RetrievePath();
				view->SetTitles();
			}
		}
#endif
		CChildFrame* childFrame = STATIC_DOWNCAST(CChildFrame, pChild);
		projectName = childFrame->GetTitle() + _T(" - ") + childFrame->GetAppTitle();
	}
	if (mgaProject && isMgaProj()) {
		CString filename, dirname;
		getMgaPaths(filename, dirname);
		projectName = projectName + L" (" + filename + L")";
	}
	CMainFrame::theInstance->UpdateTitle(projectName);
}

void CGMEApp::FindConstraintManager()
{
	CComPtr<IMgaComponents> comps;
	COMTHROW( mgaProject->get_AddOnComponents(&comps));
	MGACOLL_ITERATE(IMgaComponent, comps) {
		CComBSTR name;
		COMTHROW(MGACOLL_ITER->get_ComponentName(&name));
		if(name == _T("ConstraintManager")) {
			mgaConstMgr = CComQIPtr<IMgaComponentEx>(MGACOLL_ITER); 
			COMTHROW(mgaConstMgr->put_InteractiveMode(VARIANT_TRUE));
			break;
		}
	} MGACOLL_ITERATE_END;
}


void CGMEApp::UpdateComponentToolbar()
{
		if (!CMainFrame::theInstance)
			return;
		CComponentBar &componentBar = CMainFrame::theInstance->m_wndComponentBar;
		componentBar.ShowWindow(SW_HIDE);
		CMainFrame::theInstance->ShowPane(&componentBar, FALSE, FALSE, FALSE);

		// Removing the add-in and plug-in buttons
		const CObList &componentButtons = componentBar.GetAllButtons();
		for(POSITION pos = componentButtons.GetHeadPosition(); pos!= NULL; )
		{
			const CMFCToolBarButton* pCurrent = (const CMFCToolBarButton*) componentButtons.GetNext(pos); 
			if(pCurrent->m_bUserButton == TRUE)
			{
				// FIXME: this doesn't work, so we leak all the icons. Maybe we could m_userImages.UpdateImage instead?
				// VERIFY(m_userImages.DeleteImage(pCurrent->GetImage()));
				int buttonIndex = componentBar.ButtonToIndex(pCurrent);
				componentBar.RemoveButton(buttonIndex);
			}
		}

		componentBar.AdjustLayout();	// CMFCToolBar::AdjustLayout
		componentBar.AdjustSizeImmediate(TRUE);
		componentBar.RecalcLayout();	// CPane::RecalcLayout

		if (!mgaMetaProject)
		{
			return;
		}

		// Updating the Component toolbar
		CComPtr<IMgaRegistrar> registrar;
		if(registrar.CoCreateInstance(L"Mga.MgaRegistrar") != S_OK) return;


		// Traversing  the plugins and interpreters
		int plugins_size = min((int)plugins.GetSize(), ID_FILE_RUNPLUGIN_LAST - ID_FILE_RUNPLUGIN1);
		int interpreters_size = min((int)interpreters.GetSize(), ID_FILE_INTERPRET_LAST - ID_FILE_INTERPRET1);
		for(int i = 0; i < plugins_size + interpreters_size; ++i)
		{
			// Querying component name
			CComBSTR componentName;
			if(i < plugins_size) // if it is a plugin
			{
				componentName = plugins[i];
			}
			else					// if it is an interpreter
			{
				componentName = interpreters[i-plugins_size];
			}

			// Obtaining ToolTip
			HRESULT errCode;
			CString toolTip;
			errCode = registrar->get_ComponentExtraInfo(REGACCESS_PRIORITY, componentName, CComBSTR(L"Tooltip"), PutOut(toolTip));
			if(errCode != S_OK || toolTip.IsEmpty())
			{
				toolTip = componentName;
			}
			if(i < plugins_size)
			{
				pluginTooltips.Add(toolTip);
			}
			else
			{
				interpreterTooltips.Add(toolTip);
			}

			// Querying icon information
			CString iconInfo;
			errCode = registrar->get_ComponentExtraInfo(REGACCESS_PRIORITY, componentName, CComBSTR(L"Icon"), PutOut(iconInfo));
			if(errCode != S_OK || iconInfo.IsEmpty()) 
			{
				continue;
			}
			
			// Loading icon
			HICON hIcon = NULL; //, hictofree = NULL;
			int commaPos;
			HMODULE hModule = NULL;
			if((commaPos = iconInfo.Find(',')) >= 0)  //Format:   <modulename>,<resourceID>
			{
				if(commaPos)  // module name present;
				{
					// XP doesn't support LOAD_LIBRARY_AS_IMAGE_RESOURCE
					hModule = LoadLibraryEx(iconInfo.Left(commaPos), NULL, LOAD_LIBRARY_AS_DATAFILE);
				}
				else // No module name provided,
				{
					_bstr_t modulePath;
					registrar->get_LocalDllPath(componentName, modulePath.GetAddress());
					if(modulePath.length() != 0)
					{
						// XP doesn't support LOAD_LIBRARY_AS_IMAGE_RESOURCE
						hModule = LoadLibraryEx(modulePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
					}
				}
			}

			int cx = m_userImages.GetImageSize().cx;
			int cy = m_userImages.GetImageSize().cy;

			if( hModule != NULL )
			{
				hIcon = (HICON)::LoadImage(hModule, iconInfo.Mid(commaPos+1), IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
			}
			else
			{
				// simple .ico file with path
				hIcon =(HICON)LoadImage(NULL, iconInfo, IMAGE_ICON, cx, cy, LR_LOADFROMFILE);
			}
			
			// If icon is not found either in the DLL or a standalone file
			if(!hIcon)
			{
				// Displaying component not found icon: red X
				hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_COMPNOTFOUND), IMAGE_ICON, 16,16, LR_DEFAULTCOLOR); 
			}

			//Adding button icon
			int nIndex = m_userImages.AddIcon(hIcon);
			ASSERT(nIndex >= 0);
			BOOL succ = CMFCToolBar::SetUserImages(&m_userImages);
			ASSERT(succ == TRUE);

			// Adding button
			UINT commandID = (i < plugins_size) ? ID_FILE_RUNPLUGIN1 + i : ID_FILE_INTERPRET1 + i - plugins_size;
			CMFCToolBarButton toolBarButton(commandID, nIndex, componentName + '\n' + toolTip, TRUE);

			VERIFY(componentBar.InsertButton(toolBarButton) != -1);
			if (hModule)
				FreeLibrary(hModule);
		}
		if (plugins_size + interpreters_size != 0) {
			componentBar.AdjustLayout();	// CMFCToolBar::AdjustLayout
			componentBar.AdjustSizeImmediate(TRUE);
			componentBar.RecalcLayout();	// CPane::RecalcLayout
			CMainFrame::theInstance->ShowPane(&componentBar, TRUE, FALSE, FALSE);
			componentBar.ShowWindow(SW_SHOW);
		}
}


void CGMEApp::UpdateComponentLists(bool restart_addons)
{
	ClearDisabledComps();
	plugins.RemoveAll();
	pluginTooltips.RemoveAll();
	interpreters.RemoveAll();
	interpreterTooltips.RemoveAll();
	CStringArray tempaddons; tempaddons.Copy(addons);
	addons.RemoveAll();
	mgaConstMgr = NULL;
	if(mgaMetaProject) {
		CComBSTR b;
		COMTHROW(mgaMetaProject->get_Name(&b));
		CComPtr<IMgaRegistrar> reg;
		COMTHROW(reg.CoCreateInstance(CComBSTR(L"Mga.MgaRegistrar")));
		{
			CComVariant v;
			COMTHROW(reg->get_AssociatedComponents(b, COMPONENTTYPE_PLUGIN, REGACCESS_BOTH, &v));
			CopyTo(v, plugins);
		}
		{
			CComVariant v;
			COMTHROW(reg->get_AssociatedComponents(b, COMPONENTTYPE_INTERPRETER, REGACCESS_BOTH, &v));
			CopyTo(v, interpreters);
		}
		{
			CComVariant v;
			COMTHROW(reg->get_AssociatedComponents(b, COMPONENTTYPE_ADDON, REGACCESS_BOTH, &v));

			CopyTo(v, addons);
			if(restart_addons) {
				bool redo = false;
				if(tempaddons.GetSize() != addons.GetSize()) redo = true;
				else {
					for(int j = 0 ; j < tempaddons.GetSize(); j++) {
						if(addons[j].Compare(tempaddons[j])) {
							redo = true;
							break;
						}
					}
				}
				if(redo && AfxMessageBox(_T("AddOn configuration has changed.\nRestart addons?"), MB_YESNO) == IDYES) {
					COMTHROW(mgaProject->EnableAutoAddOns(VARIANT_FALSE));
					COMTHROW(mgaProject->EnableAutoAddOns(VARIANT_TRUE));
				}
			}
		}

		this->addonOnlineHelp.RemoveAll(); this->addonOfflineHelp.RemoveAll();
		this->pluginOnlineHelp.RemoveAll(); this->pluginOfflineHelp.RemoveAll();
		this->interpreterOnlineHelp.RemoveAll(); this->interpreterOfflineHelp.RemoveAll();
		for (INT i = 0; i < this->plugins.GetSize() + this->interpreters.GetSize() + this->addons.GetSize(); ++i) {
			HRESULT errCode;
			CString onlineHelp;
			CString offlineHelp;
			CComBSTR componentName =
				(i < plugins.GetSize()
					? plugins[i]
					: (i < plugins.GetSize() + interpreters.GetSize()
						? interpreters[i - plugins.GetSize()]
						: addons[i - plugins.GetSize() - interpreters.GetSize()]));

			errCode = reg->get_ComponentExtraInfo(REGACCESS_PRIORITY, componentName, CComBSTR(L"OnlineHelp"), PutOut(onlineHelp));
			if (errCode != S_OK || onlineHelp.IsEmpty())
			{
				onlineHelp = L"";
			}
			errCode = reg->get_ComponentExtraInfo(REGACCESS_PRIORITY, componentName, CComBSTR(L"OfflineHelp"), PutOut(offlineHelp));
			if (errCode != S_OK || offlineHelp.IsEmpty())
			{
				offlineHelp = L"";
			}
			if (i < this->plugins.GetSize())
			{
				this->pluginOnlineHelp.Add(onlineHelp);
				this->pluginOfflineHelp.Add(offlineHelp);
			}
			else if (i < this->plugins.GetSize() + this->interpreters.GetSize())
			{
				this->interpreterOnlineHelp.Add(onlineHelp);
				this->interpreterOfflineHelp.Add(offlineHelp);
			}
			else
			{
				this->addonOnlineHelp.Add(onlineHelp);
				this->addonOfflineHelp.Add(offlineHelp);
			}
		}

		// access constraint mgr
		FindConstraintManager();

	}
	dynmenus_need_refresh = true;
	UpdateComponentToolbar();
}

void CGMEApp::UpdateDynMenus(CMenu *toolmenu)
{
	CString runPluginLabel = _T("R&un Plug-In");
	CString runInterpreterLabel = _T("Run In&terpreter");
	CString helpParadigmLabel = _T("&Paradigm Help");
	CString helpComponentLabel = _T("C&omponent Help");
	CString label;
	// [ Begin workaround
	// If you just go left to the Window menu next to the Tools menu, and back to the Tools menu (so not even abandoming the menubar)
	// the original menu (without added intepreters and plugins) is switched back.
	// To workaround this we check always if the original plugin and interpreter placeholder menus are present,
	// because if they are present we have to populate the interpreters plugins again.
	// Step 1: Get tools menu if we got NULL menu
	if (toolmenu == NULL) {
		CMenu* mainMenu = CMainFrame::theInstance->GetMenu();
		if (mainMenu == NULL || mainMenu->m_hMenu == NULL) {
			HMENU mainHMenu = CMainFrame::theInstance->GetMenuBar()->GetHMenu();
			mainMenu = CMenu::FromHandle(mainHMenu);
		}
		ASSERT(mainMenu);
		for(UINT idxa = 0; idxa < mainMenu->GetMenuItemCount(); idxa++) {
			CString labela;
			mainMenu->GetMenuString(idxa, labela, MF_BYPOSITION);
			if (!labela.CompareNoCase(_T("&Tools"))) {
				toolmenu = mainMenu->GetSubMenu(idxa);
				break;
			}
		}
	}
	// Step 2: Check if placeholder menus are present -> we switch trigger in that case
	if (toolmenu != NULL) {
		for(UINT idx = 0; idx < toolmenu->GetMenuItemCount(); idx++) {
			toolmenu->GetMenuString(idx, label, MF_BYPOSITION);
			UINT menuID = toolmenu->GetMenuItemID(idx);
			if (menuID == ID_TOOLS_RUNPLUG || menuID == ID_FILE_RUNINTERPRETER || menuID == ID_HELP_COMPONENTHELP || menuID == ID_HELP_PARADIGMHELP) {
				dynmenus_need_refresh = true;
				break;
			}
		}
	}
	// End workaround ]
	if (!dynmenus_need_refresh)
		return;
	ASSERT(toolmenu);
	bool found = false;
	for(UINT idx = 0; idx < toolmenu->GetMenuItemCount(); idx++) {
		toolmenu->GetMenuString(idx, label, MF_BYPOSITION);
		UINT menuID = toolmenu->GetMenuItemID(idx);
		if (!label.CompareNoCase(runPluginLabel) ||
			menuID == ID_TOOLS_RUNPLUG || menuID == ID_FILE_RUNPLUGIN1)
		{
			if (dynmenus_need_refresh) {
				toolmenu->DeleteMenu(idx, MF_BYPOSITION);
				if (plugins.GetSize() == 1) {
					toolmenu->InsertMenu(idx, MF_BYPOSITION | MF_ENABLED,
						ID_FILE_RUNPLUGIN1, pluginTooltips[0]);
				}
				else {
					CMenu pluginmenu;
					pluginmenu.CreatePopupMenu();
					for(int i = 0; i < min(plugins.GetSize(), ID_FILE_RUNPLUGIN_LAST - ID_FILE_RUNPLUGIN1); ++i) {
						pluginmenu.AppendMenu(MF_ENABLED, ID_FILE_RUNPLUGIN1 + i, pluginTooltips[i]);
					}
					toolmenu->InsertMenu(idx,
						plugins.GetSize() ? MF_BYPOSITION | MF_POPUP | MF_ENABLED : MF_BYPOSITION | MF_POPUP | MF_GRAYED,
						(UINT_PTR)pluginmenu.Detach(), runPluginLabel);
				}
				found = true;
			}
		}
		else if (!label.CompareNoCase(runInterpreterLabel) ||
			menuID == ID_FILE_RUNINTERPRETER || menuID == ID_FILE_INTERPRET1)
		{
			if (dynmenus_need_refresh) {
				toolmenu->DeleteMenu(idx, MF_BYPOSITION);
				if (interpreters.GetSize() == 1) {
					toolmenu->InsertMenu(idx, MF_BYPOSITION | MF_ENABLED,
						ID_FILE_INTERPRET1, interpreterTooltips[0]);
				}
				else {
					CMenu pluginmenu;
					pluginmenu.CreatePopupMenu();

					struct TooltipSorter {
						CString tooltip;
						int id;
						bool operator<(const TooltipSorter& that) {
							return this->tooltip < that.tooltip;
						}
					};
					std::vector<TooltipSorter> interpreters_sorted;

					for (int i = 0; i < min(interpreters.GetSize(), ID_FILE_INTERPRET_LAST - ID_FILE_INTERPRET1); ++i) {
						TooltipSorter tt = { interpreterTooltips[i], ID_FILE_INTERPRET1 + i };
						interpreters_sorted.push_back(std::move(tt));
					}
					std::sort(begin(interpreters_sorted), end(interpreters_sorted));
					std::for_each(begin(interpreters_sorted), end(interpreters_sorted), [&](const TooltipSorter& tt)
					{
						pluginmenu.AppendMenu(MF_ENABLED, tt.id, tt.tooltip);
					});
					toolmenu->InsertMenu(idx,
						interpreters.GetSize() ? MF_BYPOSITION | MF_POPUP | MF_ENABLED : MF_BYPOSITION | MF_POPUP | MF_GRAYED,
						(UINT_PTR)pluginmenu.Detach(), runInterpreterLabel);
				}
				found = true;
			}
		}
		else if (!label.CompareNoCase(helpComponentLabel) ||
			menuID == ID_HELP_COMPONENTHELP || menuID == ID_HELP_COMPONENTHELP_FIRST)
		{
			if (dynmenus_need_refresh) {
				toolmenu->DeleteMenu(idx, MF_BYPOSITION);
				CMenu helpComponent;
				helpComponent.CreatePopupMenu();
				for (int i = 0; i < min(plugins.GetSize() + interpreters.GetSize() + addons.GetSize(), ID_HELP_COMPONENTHELP_LAST - ID_HELP_COMPONENTHELP_FIRST);) {
					int index = i / 2;
					CString componentName = (index < plugins.GetSize() ? 
						pluginTooltips[index] : (index < plugins.GetSize() + interpreters.GetSize() ?
						interpreterTooltips[index - plugins.GetSize()] : addons[index - plugins.GetSize() - interpreters.GetSize()]));
					CString onlineHelpLocation = (index < plugins.GetSize() ? 
							pluginOnlineHelp[index] : (index < plugins.GetSize() + interpreters.GetSize() ? 
								interpreterOnlineHelp[index - plugins.GetSize()] : addonOnlineHelp[index - plugins.GetSize() - interpreters.GetSize()]));
					CString offlineHelpLocation = (index < plugins.GetSize() ? 
							pluginOfflineHelp[index] : (index < plugins.GetSize() + interpreters.GetSize() ? 
								interpreterOfflineHelp[index - plugins.GetSize()] : addonOfflineHelp[index - plugins.GetSize() - interpreters.GetSize()]));

					CMenu help;
					help.CreatePopupMenu();
					if (!onlineHelpLocation.IsEmpty())
					{
						help.AppendMenu(MF_ENABLED, ID_HELP_COMPONENTHELP_FIRST + i++, L"Online Help");
					}
					else 
					{ 
						++i; 
					}

					if (!offlineHelpLocation.IsEmpty())
					{
						help.AppendMenu(MF_ENABLED, ID_HELP_COMPONENTHELP_FIRST + i++, L"Offline Help");
					}
					else 
					{ 
						++i; 
					}

					if (help.GetMenuItemCount() != 0)
					{
						helpComponent.InsertMenu(index, MF_BYPOSITION | MF_POPUP | MF_ENABLED, (UINT_PTR)help.Detach(), componentName);
					}
				}
				toolmenu->InsertMenu(idx, 
					helpComponent.GetMenuItemCount() ? MF_BYPOSITION | MF_POPUP | MF_ENABLED : MF_BYPOSITION | MF_POPUP | MF_GRAYED, 
					(UINT_PTR)helpComponent.Detach(), helpComponentLabel);
				found = true;
			}
		}
		else if (!label.CompareNoCase(helpParadigmLabel) ||
			menuID == ID_HELP_PARADIGMHELP || menuID == ID_HELP_PARADIGMHELP_FIRST)
		{
			if (dynmenus_need_refresh && mgaProject) {
				toolmenu->DeleteMenu(idx, MF_BYPOSITION);
				CComBSTR pname = guiMetaProject->name;
				CComBSTR pguidstr = guiMetaProject->guid;

				CMenu help;
				help.CreatePopupMenu();
				CComPtr<IMgaRegistrar2> reg;
				HRESULT hr = reg.CoCreateInstance(CComBSTR(L"Mga.MgaRegistrar"));
				if (SUCCEEDED(hr))
				{
					reg->GetParadigmExtraInfoDisp(pname, pguidstr, CComBSTR(L"OnlineHelp"), PutOut(this->paradigmOnlineHelp));
					reg->GetParadigmExtraInfoDisp(pname, pguidstr, CComBSTR(L"OfflineHelp"), PutOut(this->paradigmOfflineHelp));
				}

				if (!this->paradigmOnlineHelp.IsEmpty())
				{
					help.AppendMenu(MF_ENABLED, ID_HELP_PARADIGMHELP_FIRST, L"Online Help");
				}
				if (!this->paradigmOfflineHelp.IsEmpty())
				{
					help.AppendMenu(MF_ENABLED, ID_HELP_PARADIGMHELP_LAST, L"Offline Help");
				}
				toolmenu->InsertMenu(idx, 
					help.GetMenuItemCount() != 0 ? MF_BYPOSITION | MF_POPUP | MF_ENABLED : MF_BYPOSITION | MF_POPUP | MF_GRAYED, 
					(UINT_PTR)help.Detach(), pname);
				found = true;
			}
		}
	}
	if (found)
		dynmenus_need_refresh = false;
}

// throws exceptions!!
void CGMEApp::AfterOpenOrCreateProject(const CString &conn)
{
	UpdateProjectName();
	if( mgaProject != NULL ) {
		abort_on_close = false;
// get meta
		COMTHROW( mgaProject->get_RootMeta(&mgaMetaProject) );
		UpdateComponentLists();

// create guimetaproject
		ASSERT( guiMetaProject == NULL );
		guiMetaProject = new CGuiMetaProject(mgaMetaProject);
		CMainFrame::theInstance->WriteStatusParadigm(guiMetaProject->displayedName);
		CMainFrame::theInstance->SetTitle(guiMetaProject->displayedName);
		UpdateProjectName();

// Register OLE Server / MGA Client
		CGMEOLEApp *oleApp = ((CMainFrame*)m_pMainWnd)->mGmeOleApp;
		CComBSTR clientName(OLESTR("GME.Application"));
		COMTHROW(mgaProject->RegisterClient(clientName, oleApp->GetIDispatch(FALSE), &mgaClient)); // mgaClient increments the refcount of oleApp->idispatch by 1

		((CMainFrame*)m_pMainWnd)->setMgaProj();

// notify object inspector
		ASSERT(CGMEObjectInspector::theInstance!=NULL);
		CGMEObjectInspector::theInstance->SetProject(mgaProject);

// notify Part Browser
		ASSERT(CGMEPartBrowser::theInstance!=NULL);
		CGMEPartBrowser::theInstance->SetProject(mgaProject);

// by swapping the order of SetProject (first ObjectInspector, then Browser), 
// the user will see the properties of the initally selected project at once
// notify browser
		ASSERT( CGMEBrowser::theInstance != NULL );
		CGMEBrowser::theInstance->SetProject(mgaProject);

// notify search control
		ASSERT(CGMESearch::theInstance!=NULL);
		CGMESearch::theInstance->SetProject(mgaProject);


// change dir
		ChangedProjectConnStrings();
// record connection name
		currentConnection = conn;
		m_RecentProjectList.AddAndWriteList(conn);
		if (!CGMEDoc::theInstance) {
			CWinAppEx::OnFileNew();
		}
// start autosave
		if (autosaveEnabled) {
			CMainFrame::theInstance->StartAutosaveTimer(autosaveFreq);
		}
	}
}


static int guidcmp(VARIANT &qGUID, VARIANT &pGUID)	{
	GUID g1, g2;
	CopyTo(qGUID, g1);
	CopyTo(pGUID, g2);
	return memcmp(&g1, &g2, sizeof(g1));
}

static int versioncmp(CComBSTR &qVer, CComBSTR &pVer) {
	CString q(qVer);
	CString p(pVer);

	if ((!p.IsEmpty()) && (!q.IsEmpty())) {
		return p.Compare(q);
	}
	return 1;
}


bool DiagnoseParadigm(CString metaname, bool syscheck = false) {
	CComPtr<IMgaRegistrar> reg;
	try {
		HRESULT hr = reg.CoCreateInstance(CComBSTR(L"Mga.MgaRegistrar"));
		if(hr != S_OK) {
			throw CString(_T("Cannot create the registrar component\n")
						  _T("We recommend you to reinstall GME"));
		}
		CComBSTR conn;
		CComVariant guid;
		hr = reg->QueryParadigm(CComBSTR(metaname), &conn, &guid, 
			syscheck ? REGACCESS_SYSTEM : REGACCESS_PRIORITY);
		if(hr != S_OK) {
			throw CString(_T("Cannot access registry info for paradigm ") + metaname +
						  _T("\nWe recommend you remove and re-register the paradigm"));
		}
		CComObjPtr<IMgaMetaProject> paradigm;
		hr =  paradigm.CoCreateInstance(OLESTR("MGA.MgaMetaProject"));
		if(hr != S_OK) {
			throw CString(_T("Cannot create the meta component\n")
						  _T("We recommend you reinstall GME"));
		}

		hr = paradigm->Open(conn);

		if(hr != S_OK) {
			throw CString(_T("Cannot open the paradigm ") + metaname + _T("\n")
						  _T("Probable cause is file non-existence,\n")
						  _T("insufficient access, or format error\n")
						  _T("Connection string: ") + CString(conn));
		}


		CComBSTR parname;
		CComVariant gguid;
		hr = paradigm->get_Name(&parname);
		if(hr == S_OK) paradigm->get_GUID(&gguid);
		hr |= paradigm->Close();

		if(hr != S_OK) {
			throw CString(_T("Cannot read the paradigm ") + metaname + _T("\n")
						  _T("Probable cause is file format error\n")
						  _T("Connection string: ") + CString(conn));
		}
		if(!( parname == CComBSTR( (LPCTSTR) metaname))) {
			throw CString(_T("The paradigm opened '") + CString(parname) + _T("'\n")
		 				  _T("differs from the requested paradigm '")+ metaname + _T("'\n")
						  _T("We recommend you unregister '") + metaname + _T("'\n")
						  _T("Connection string: ") + CString(conn));

		}

		CComBstrObj parg1, parg2;
		GUID g;
		CopyTo(gguid, g);
		CopyTo(g, parg1);
		CopyTo(guid, g);
		CopyTo(g, parg2);
		if(parg1 != parg2) {
			throw CString(_T("The GUID in paradigm '") + CString(parname) + _T("'\n")
						  _T("{") + CString(parg1) + _T("}\n")
		 				  _T("differs from the requested GUID for '")+ metaname + _T("'\n")
						  _T("{") + CString(parg2) + _T("}\n")
						  _T("We recommend you unregister '") + metaname + _T("'\n")
						  _T("Connection string: ") + CString(conn));
		}
	} catch( CString &c) {
		if(!syscheck) {
			AfxMessageBox(c); 
			CComBSTR cc; CComVariant gg;
			if(reg && E_NOTFOUND != reg->QueryParadigm(CComBSTR(metaname), &cc, &gg, REGACCESS_SYSTEM)) {
				if(DiagnoseParadigm(metaname, true)) {
					AfxMessageBox(_T("SYSTEM registry for '") + metaname + _T("' is correct\n")
								_T("We recommend you remove the USER registration for ") + metaname);
				}
				else {
					AfxMessageBox(_T("SYSTEM registry for '") + metaname + _T("' is also incorrect\n")
								_T("We recommend you reinstall the paradigm."));
				}
			}
		}
		return false;
	}
	catch(...) {
		return false;
	}
	return true;
}



void CGMEApp::OpenProject(const CString &conn) {

	MSGTRY
	{
		if (conn.Left(4) == _T("XML=")) {
			CString fullPath = conn.Right(conn.GetLength() - 4);
			TCHAR buffer[MAX_PATH];
			TCHAR* filepart = NULL;
			GetFullPathName(fullPath, MAX_PATH, buffer, &filepart);
			if (filepart == NULL) {
				COMTHROW(E_FILEOPEN);
			}
			CString filename = filepart;
			CString title = filename.Left(filename.ReverseFind('.'));
			Importxml(fullPath, filepart, title);
			return;
		}

		CWaitCursor wait;

		ASSERT( mgaProject == 0 );
		COMTHROW( mgaProject.CoCreateInstance(OLESTR("Mga.MgaProject")) );

		VARIANT_BOOL enableAutoAddOns = VARIANT_TRUE;
		VARIANT_BOOL readable_only;

		COMTHROW( mgaProject->EnableAutoAddOns(VARIANT_TRUE));
		HRESULT hr = mgaProject->Open(PutInBstr(conn), &readable_only);
		if(hr != S_OK) {
			_bstr_t mgaProjectOpenError;
			GetErrorInfo(mgaProjectOpenError.GetAddress());
			CComBSTR parn;
			CComBSTR parv;
			long version;
			CComVariant parg;
			VARIANT_BOOL ro_mode;
			if( conn.Left(5) == _T("MGX=\""))
			{
				if( E_FILEOPEN == hr) {
					consoleMessage( _T("Could not open project!"), MSG_ERROR);
				}
				else if( E_MGA_PARADIGM_INVALID == hr) {
					consoleMessage( _T("Project could not access its original version of paradigm!"), MSG_ERROR);
				}
				else if( E_MGA_PARADIGM_NOTREG == hr) {
					consoleMessage( _T("Project could not access its paradigm!"), MSG_ERROR);
				}
				else if( E_MGA_META_INCOMPATIBILITY == hr) {
					consoleMessage( _T("Versioned project is not compatible with the registered paradigm!"), MSG_ERROR);
				}
				else if( E_UNKNOWN_STORAGE == hr) { 
					// no additional comment in this case
				}
				else {
					consoleMessage( _T("Could not open project (unknown error)!"), MSG_ERROR);
				}
				CloseProject();
				return; // ensures no more exception handlers or explanatory messages (or QueryProjectInfo calls)
			}

			COMTHROW(mgaProject->QueryProjectInfo(PutInBstr(conn), &version, &parn, &parv, &parg, &ro_mode));
			while (true) {
				CString msg;
				CComVariant guidpar;
				CString newparname;
				bool tryit = false;

				if(hr == E_MGA_MODULE_INCOMPATIBILITY) {
					msg = _T("WARNING: The project data is not in the current MGA format\n")
						_T("Do you want to upgrade it?");
					if (AfxMessageBox(msg ,MB_OKCANCEL) == IDOK) {
						tryit = true;
					}
				}
				if(hr == E_MGA_PARADIGM_INVALID) {
					msg = _T("WARNING: Project could not access its original version of\n")
						_T("paradigm '") + CString(parn) + _T("'\n");
					if (mgaProjectOpenError.length())
						msg += static_cast<const wchar_t*>(mgaProjectOpenError);
					msg += _T("Do you want to try with the current version of the paradigm?");
					if (AfxMessageBox(msg ,MB_OKCANCEL) == IDOK) {
						guidpar = true;
						tryit = true;
						newparname = parn;
					}
				}
				if(hr == E_MGA_PARADIGM_NOTREG) {
					CString msg = _T("Could not find paradigm '") + CString(parn) + "'";
					if (parv != "") {
						msg += CString(" version ") + static_cast<const wchar_t*>(parv);
					} else if (parg.vt == (VT_UI1 | VT_ARRAY)) {
						msg += CString(" with GUID ") + StringFromGUID2(parg);
					}
					IMgaRegistrarPtr registrar;
					COMTHROW(registrar.CreateInstance(L"Mga.MgaRegistrar"));
					_variant_t current_guid;
					_bstr_t current_version;
					registrar->QueryParadigm(static_cast<BSTR>(parn), _bstr_t().GetAddress(), current_guid.GetAddress(), REGACCESS_BOTH);
					registrar->VersionFromGUID(static_cast<BSTR>(parn), current_guid, current_version.GetAddress(), REGACCESS_BOTH);
					if (current_guid.vt != VT_EMPTY || current_version.length())
					{
						CString msg_current = msg;
						msg_current += L"\nDo you want to open with the current version (";
						if (current_version.length())
						{
							msg_current += static_cast<const wchar_t*>(current_version);
						}
						else if (current_guid.vt == (VT_UI1 | VT_ARRAY))
						{
							msg_current += StringFromGUID2(current_guid);
						}
						msg_current += L")?";
						int mbRes = AfxMessageBox(msg_current, MB_YESNOCANCEL);
						if (mbRes == IDCANCEL)
						{
							break;
						}
						if (mbRes == IDYES)
						{
							guidpar = true;
							tryit = true;
							newparname = parn;
						}
					}

					if (!tryit)
					{
						if (CString(parn) == _T("MetaGME2000"))
							msg += _T("\n (In GME3 the MetaGME2000 paradigm was renamed to MetaGME)");
						msg += _T("\nDo you want to import with another registered paradigm ?");
						if (AfxMessageBox(msg ,MB_OKCANCEL) == IDOK) {
					
							CComObjPtr<IMgaLauncher> launcher;
							COMTHROW( launcher.CoCreateInstance(CComBSTR(L"Mga.MgaLauncher")) );
							if (SUCCEEDED(launcher->MetaDlg(METADLG_NONE))) {
								guidpar = true;
								newparname.Empty();
								COMTHROW( launcher->get_ParadigmName(PutOut(newparname)) );
								tryit = !newparname.IsEmpty();
							}
						}
					}
				}
				if(hr == E_MGA_META_INCOMPATIBILITY && (parv.Length() > 0)) {
					msg = _T("WARNING: Versioned project is not compatible with the paradigm '") + CString(parn) + _T("'\n") 
						_T(" (Eg.: Same version string was assigned to incompatible paradigms)\n")
						_T("Do you want to open it based on the paradigm GUID?");
					if (AfxMessageBox(msg ,MB_OKCANCEL) == IDOK) {
						guidpar = parg;
						tryit = true;
						newparname = parn;
					}
				}
				if(hr == E_MGA_COMPONENT_ERROR) {
					_bstr_t err(_T("ERROR: automatic addon components could not start up:\n"));
                    err += mgaProjectOpenError;
					err += "\nDo you want to open the project without addons?";
					if (AfxMessageBox(err, MB_YESNO) == IDYES) {
						enableAutoAddOns = VARIANT_FALSE;
						mgaProject->EnableAutoAddOns(enableAutoAddOns);
						tryit = true;
					}
				}
				if(tryit) {
					hr = mgaProject->OpenEx(PutInBstr(conn), CComBSTR(newparname), guidpar);
					if(hr == E_MGA_PARADIGM_NOTREG || hr == E_MGA_PARADIGM_INVALID) {
						DiagnoseParadigm(CString(parn));
					}
				}
				else break;
			}
			COMTHROW(hr);
		}

		if(readable_only != VARIANT_FALSE) {
			AfxMessageBox(_T("WARNING: Project file is read-only\nChange file access or use Save As to save your work"));
		}
		else {
			CComVariant g, g2;
			CComBSTR pname; 
			CComBSTR pver, pver2;
			{
				CComPtr<IMgaTerritory> t;
				COMTHROW(mgaProject->CreateTerritory(NULL, &t, NULL));
				COMTHROW(mgaProject->BeginTransaction(t, TRANSACTION_READ_ONLY));
				COMTHROW(mgaProject->get_MetaName(&pname));
				COMTHROW(mgaProject->get_MetaGUID(&g));
				COMTHROW(mgaProject->get_MetaVersion(&pver));
				// COMTHROW(mgaProject->AbortTransaction()); PETER: Why abort ?
				COMTHROW(mgaProject->CommitTransaction());
				CComPtr<IMgaRegistrar> mgareg;
				COMTHROW(mgareg.CoCreateInstance(OLESTR("MGA.MgaRegistrar")));
				CComBSTR connstr;
				COMTHROW(mgareg->QueryParadigm(pname, &connstr, &g2, REGACCESS_PRIORITY));
				mgareg->VersionFromGUID(pname, g2, &pver2, REGACCESS_PRIORITY);
			}
			if(guidcmp(g, g2) && versioncmp(pver, pver2)) {
				CString project_version = pver;
				if (project_version == _T(""))
					project_version = StringFromGUID2(g);
				CString registered_paradigm_version = pver2;
				if (pver2 == _T(""))
					registered_paradigm_version = StringFromGUID2(g2);
				CString prompt;
				prompt.Format(_T("The '%s' paradigm version used to save this file, '%s', is not the currently-registered version, '%s'.\n\n")
								_T("Do you want to upgrade to the current paradigm?"),
								static_cast<const TCHAR*>(pname), project_version, registered_paradigm_version);
				int answer = AfxMessageBox(prompt, MB_YESNO);
				if(answer == IDYES) {
					COMTHROW(mgaProject->Close(VARIANT_FALSE));

					// PETER: Create new MgaProject COM object (workaround MGA addon bug)
					mgaProject.Release();
					COMTHROW( mgaProject.CoCreateInstance(OLESTR("Mga.MgaProject")) );
					COMTHROW( mgaProject->EnableAutoAddOns(enableAutoAddOns));
					
					HRESULT hr = mgaProject->OpenEx(PutInBstr(conn), pname, g2);
					if(hr == E_MGA_PARADIGM_NOTREG || hr == E_MGA_PARADIGM_INVALID) {
						AfxMessageBox(_T("Paradigm error"));
						DiagnoseParadigm(CString(pname));
					}
					else if(hr != S_OK) {
						AfxMessageBox(_T("Upgrade failed, probably due to incompatibility.\n")
									   _T("You can probably reopen the file without upgrade,\n")
									   _T("and use the 'Upgrade through XML' function later."));
						throw_last_com_error(hr);
					}
					else readable_only = false;
					COMTHROW(hr);
				}
			}	
		}
		AfterOpenOrCreateProject(conn);
		hr = mgaProject->Notify(GLOBALEVENT_OPEN_PROJECT_FINISHED);
		ASSERT(SUCCEEDED(hr));
	}
	MSGCATCH(_T("Could not open project"), CloseProject(true, true))
	
	UpdateProjectName();

}



void CGMEApp::CreateProject(const CString &metaname, const CString &conn)
{
	CString msg;
	try
	{
		CWaitCursor wait;

		// create the project
		msg = _T("Fatal error while initializing project");
		ASSERT( mgaProject == 0 );
		COMTHROW( mgaProject.CoCreateInstance(L"Mga.MgaProject") );
		ASSERT( mgaProject != NULL );

		COMTHROW( mgaProject->EnableAutoAddOns(VARIANT_TRUE));
		msg = _T("Could not create project");
		HRESULT hr = mgaProject->Create(PutInBstr(conn), PutInBstr(metaname)) ;
	    if(hr == E_MGA_PARADIGM_NOTREG || hr == E_MGA_PARADIGM_INVALID) {
			TCHAR buf[200];
		    _stprintf_s(buf, _T("Could not open current version of paradigm %s"), 
				static_cast<const TCHAR*>(metaname));

			AfxMessageBox(buf);
			DiagnoseParadigm(metaname);
		}
		if(hr == E_MGA_COMPONENT_ERROR) {
			_bstr_t errorInfo;
			GetErrorInfo(errorInfo.GetAddress());
			_bstr_t err(_T("ERROR: automatic addon components could not start up:\n"));
			err += errorInfo;
			err += "\nDo you want to create the project without addons?";
			if (AfxMessageBox(err, MB_YESNO) == IDYES) {
				mgaProject->EnableAutoAddOns(VARIANT_FALSE);
				hr = mgaProject->Create(PutInBstr(conn), PutInBstr(metaname)) ;
			}
		}
		if( hr == E_UNKNOWN_STORAGE && conn.Left(5) == _T("MGX=\"")) {
			CloseProject();
			return; // no more exception handler explanatory messages
		}
	    COMTHROW(hr);

		AfterOpenOrCreateProject(conn);
		// TODO: set RootFolder name to filename?
		hr = mgaProject->Notify(GLOBALEVENT_OPEN_PROJECT_FINISHED);
		ASSERT(SUCCEEDED(hr));
	}
	MSGCATCH(_T("Could not create project"), CloseProject())
}


void CGMEApp::SaveProject(const CString &conn) {
	if( mgaProject != NULL ) {
		HRESULT hr = mgaProject->Save(CComBSTR(conn), VARIANT_FALSE);
		if(hr != S_OK) {
			CComBSTR error;
			if (GetErrorInfo(&error)) {
				CString errmsg = _T("Could not save project: ");
				errmsg += error;
				if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
					// FIXME: KMS: not too sure why this is ACCESS_DENIED from MoveFile instead of SHARING_VIOLATION
					errmsg += _T("\nCheck that no other GME has this file open");
				}
				AfxMessageBox(errmsg);
			} else {
				AfxMessageBox(_T("ERROR: Could not save project\nCheck access permissions"));
			}
		}
	}

	if ((!conn.IsEmpty()) && (currentConnection != conn)) {
		ChangedProjectConnStrings();
		currentConnection = conn;
		m_RecentProjectList.AddAndWriteList(conn);
	}
}	



// ********************************************************************************************


void CGMEApp::GetSettings()
{
	bool oldUseAutoRouting = useAutoRouting;
	MSGTRY
	{
		CComObjPtr<IMgaRegistrar> registrar;
		COMTHROW( registrar.CoCreateInstance(L"Mga.MgaRegistrar") );

		// Icons
		CComBSTR bstr;
		COMTHROW( registrar->get_IconPath(REGACCESS_BOTH,&bstr));
		CopyTo(bstr,bitmapPath);

		// Multiview
		VARIANT_BOOL enabledmv;
		COMTHROW( registrar->get_ShowMultipleView(REGACCESS_USER, &enabledmv) );
		multipleView = (enabledmv != VARIANT_FALSE);

		// Autosave
		VARIANT_BOOL autosaveenable;
		COMTHROW( registrar->get_AutosaveEnabled(REGACCESS_USER, &autosaveenable) );
		autosaveEnabled = (autosaveenable == VARIANT_FALSE) ? FALSE : TRUE;

		long autosavefreq;
		COMTHROW( registrar->get_AutosaveFreq(REGACCESS_USER, &autosavefreq) );
		autosaveFreq = (int)autosavefreq;

		VARIANT_BOOL autosaveusedir;
		COMTHROW( registrar->get_AutosaveUseDir(REGACCESS_USER, &autosaveusedir) );
		autosaveUseDir = (autosaveusedir == VARIANT_FALSE) ? 0 : 1;

		COMTHROW( registrar->get_AutosaveDir(REGACCESS_USER, PutOut(autosaveDir)) );

		//Event Logging
		VARIANT_BOOL enablelogging;
		COMTHROW( registrar->get_EventLoggingEnabled(REGACCESS_USER, &enablelogging) );
		if(enablelogging != VARIANT_FALSE)
		{
			CGMEEventLogger::initialize();
			CGMEEventLogger::LogGMEEvent(_T("CGMEApp::GetSettings() Event Logging Enabled\r\n"));
		}
		else
		{
			CGMEEventLogger::LogGMEEvent(_T("CGMEApp::GetSettings() Event Logging Disabled\r\n"));
			CGMEEventLogger::StopLogging();
		}

		// Autorouter
		VARIANT_BOOL useautorouting = VARIANT_TRUE;
		COMTHROW( registrar->get_UseAutoRouting(REGACCESS_USER, &useautorouting) );
		useAutoRouting = (useautorouting != VARIANT_FALSE);

		VARIANT_BOOL labelavoidance;
		COMTHROW( registrar->get_LabelAvoidance(REGACCESS_USER, &labelavoidance) );
		labelAvoidance = (labelavoidance != VARIANT_FALSE);

		// Default Zoom Level
		CComBSTR bstr_zl;
		COMTHROW( registrar->GetDefZoomLevel( REGACCESS_USER, &bstr_zl));
		if( bstr_zl)
			CopyTo( bstr_zl, defZoomLev);

		// SendMouseOver notification
		VARIANT_BOOL send_mouse_over;
		COMTHROW( registrar->GetMouseOverNotify(REGACCESS_USER, &send_mouse_over));
		mouseOverNotify = ( send_mouse_over != VARIANT_FALSE);

		// Real number format string
		CComBSTR bstr_fmt;
		COMTHROW( registrar->GetRealNmbFmtStr( REGACCESS_USER, &bstr_fmt));
		if( bstr_fmt)
			CopyTo( bstr_fmt, realFmtStr);

		// History Maintained?
		VARIANT_BOOL history_maintained;
		COMTHROW( registrar->GetNavigation( REGACCESS_USER, &history_maintained));
		maintainHistory = ( history_maintained != VARIANT_FALSE);
	}
	MSGCATCH(_T("Error while trying to get program settings"),;);
	if(CGMEDoc::theInstance) {
		// Global AutoRouting policy changed, convert opened views if necessary
		if (!useAutoRouting && oldUseAutoRouting) {
			CComPtr<IUnknown> nullPtr;
			CGMEDoc::theInstance->ConvertPathToCustom(nullPtr);
		}
		if (autosaveEnabled) {
			CMainFrame::theInstance->StartAutosaveTimer(autosaveFreq);
		}
		else {
			CMainFrame::theInstance->StopAutosaveTimer();
		}
		CGMEDoc::theInstance->ResetAllViews();
	}
}

LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata)
{
	HKEY hKey;
    LONG lResult = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hKey); 
  
    if( lResult == ERROR_SUCCESS)
	{
         long lDataSize = MAX_PATH;
		TCHAR data[MAX_PATH];
  
		RegQueryValue( hKey, NULL, data, &lDataSize);
		lstrcpy( retdata,data);
		RegCloseKey( hKey);
    }
  
    return lResult;
}
  
HINSTANCE GotoURL(LPCTSTR url, int showcmd)
{
	TCHAR key[MAX_PATH + MAX_PATH];
  
    // First try ShellExecute()
    HINSTANCE hResult = ShellExecute( NULL, _T("open"), url, NULL,NULL, showcmd);
    //HINSTANCE hResult;
    // If it failed, get the .htm regkey and lookup the program
    if((UINT)hResult <= HINSTANCE_ERROR)
	{
		if( GetRegKey( HKEY_CLASSES_ROOT, _T(".htm"), key) == ERROR_SUCCESS)
		{
             lstrcat( key, _T("\\shell\\open\\command")); 
  
             if( GetRegKey( HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS)
			 {
				TCHAR *pos;
                 pos = _tcsstr( key, _T("\"%1\"")); 
  
                 if( pos == NULL)
				 {
					// No quotes found
                    pos = _tcsstr( key, _T("%1")); // Check for % 1, without quotes
                    if( pos == NULL)    // No  parameter at all...
                         pos = key+lstrlen( key)-1;
                     else
                         *pos = '\0';    //  Remove the parameter
                 }
                 else
                     *pos = '\0';
					// Remove the parameter
  
                 lstrcat(pos, _T(" "));
                 lstrcat(pos, url);
  
				 // FIXME: should use CreateProcess
                 hResult = (HINSTANCE)WinExec( CStringA(key),showcmd);
             }
         }
	}
  
     return hResult;
}

/////////////////////////////////////////////////////////////////////////////
// CGMEApp message handlers

// *******************************************************************************
// *******************************************************************************
//								FILE
// *******************************************************************************
// *******************************************************************************


void CGMEApp::OnFileOpen() 
{
	CMgaOpenDlg dlg(CMgaOpenDlg::OpenDialog);
	CString conn = dlg.AskConnectionString(true, true);

	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileOpen ")+conn+_T("\r\n"));

	if( conn.IsEmpty() )
		return;

	CWaitCursor wait;

	if( mgaProject != NULL )
		CloseProject();

	if (conn.Left(4) == _T("MGX=")) {
		CString fullPath = conn.Right(conn.GetLength() - 4);
		TCHAR buffer[MAX_PATH];
		TCHAR* filepart = NULL;
		GetFullPathName(fullPath, MAX_PATH, buffer, &filepart);
		if (filepart == NULL) {
			DisplayError(_T("Error opening MGX file"), E_FILEOPEN);
			return;
		}
		// FIXME: KMS: yes, the quotes are necessary...
		conn = _T("MGX=\"");
		// FIXME: KMS: yes, a trailing slash makes it not work
		conn += fullPath.Left(fullPath.GetLength() - _tcslen(filepart) - 1);
		conn += _T("\"");
	}
	OpenProject(conn);
}


#define PROJECT_STATUS_CHANGED 4

void CGMEApp::OnAppExit()
{
	if (SaveAllModified())
	{
		CWinAppEx::OnAppExit();
		return;
	}
}

BOOL CGMEApp::SaveAllModified() 
{
	// Focus must be killed to flush ObjectInspector and Browser
	::SetFocus(NULL);
	if (mgaProject != NULL && (proj_type_is_mga || proj_type_is_xmlbackend)) {
		int ret = IDNO;
		long l;
		COMTHROW(mgaProject->get_ProjectStatus(&l));
		if (l & PROJECT_STATUS_CHANGED)
		{
			CString filename;
			CString connstr = static_cast<const wchar_t*>(mgaProject->ProjectConnStr);
			if (connstr.Mid(0, 4) == "MGA=")
			{
				CString dirname;
				GetFullPathName(connstr.Mid(4), filename, dirname);
				filename = L" (" + filename + ")";
			}
			CString message = _T("Do you want to save changes to '") + projectName + L"'" + filename + _T("?");
			ret = AfxMessageBox(message,  MB_YESNOCANCEL);
		}
		if (ret == IDCANCEL) {
			return FALSE;
		} else if (ret == IDNO) {
			abort_on_close = true;
			OnFileAbortProject();
		} else {
			((CMainFrame*)m_pMainWnd)->clearMgaProj();
			return SafeCloseProject();
		}
	}
	return TRUE;
}

int CGMEApp::ExitInstance() 
{
	CloseProject(false);

#ifdef _DEBUG
	// Do this under Debug to silence memory leaks
	// Don't do it under Release, since we may yet crash, and it's wasteful since we're exiting anyway
	crUninstall();
#endif

	return CWinAppEx::ExitInstance();
}

void CGMEApp::OnFileNew() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileNew\r\n"));

	MSGTRY	{
		CString metaname;
		CString dataconn;
		CMgaOpenDlg dlg(CMgaOpenDlg::NewDialog);

		CComObjPtr<IMgaLauncher> launcher;
		COMTHROW( launcher.CoCreateInstance(L"Mga.MgaLauncher", 0, CLSCTX_INPROC_SERVER) );

	meta_label:
		HRESULT hr = launcher->MetaDlg(METADLG_NEWFILE);
		if( hr == S_FALSE )
			return;
		COMTHROW( hr );

		metaname.Empty();
		COMTHROW( launcher->get_ParadigmName(PutOut(metaname)) );

		dataconn = dlg.AskConnectionString(false, false);

		if( dlg.pressed_back )
			goto meta_label;

		if( dataconn.IsEmpty() )
			return;

		CWaitCursor wait;

		if( mgaProject != NULL )
			CloseProject();

		CreateProject(metaname, dataconn);

		if (mgaProject != NULL && (proj_type_is_mga||proj_type_is_xmlbackend)) {
			OnFileSave();
		}
	}
	MSGCATCH(_T("Error creating new project"),;)

}

BOOL CGMEApp::OnOpenRecentProject(UINT nID)
{
	ASSERT_VALID(this);

	ASSERT(nID >= ID_FILE_MRU_PRJ1);
	ASSERT(nID < ID_FILE_MRU_PRJ1 + (UINT)m_RecentProjectList.GetSize());

	int nIndex = nID - ID_FILE_MRU_PRJ1;
	CString conn = m_RecentProjectList[nIndex];

	ASSERT(conn.GetLength() != 0);

	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnOpenRecentProject ")+conn+_T("\r\n"));

	CWaitCursor wait;

	if( mgaProject != NULL )
		CloseProject();

	OpenProject(conn);

	return TRUE;
}


bool CGMEApp::SafeCloseProject() {
// In case of an MGA file, try to save it first to find out 
	abort_on_close = false;
	if(mgaProject != NULL && (proj_type_is_mga||proj_type_is_xmlbackend)) {
		HRESULT hr = mgaProject->Save(NULL, VARIANT_FALSE);
		if(hr != S_OK) {
			CComBSTR error;
			if (GetErrorInfo(&error)) {
                CString errmsg = _T("Could not save project: ");

                errmsg += error;
                if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
				    // FIXME: KMS: not too sure why this is ACCESS_DENIED from MoveFile instead of SHARING_VIOLATION
                    errmsg += _T("\nCheck that no other GME has this file open");
                }
                AfxMessageBox(errmsg);
                return false;
            }
            else {
                AfxMessageBox(_T("ERROR: Could not save project\nCheck access permissions"));
                return false;
            }
        }
		abort_on_close = true;
	}

	BeginWaitCursor();
	if(CGMEDoc::theInstance)
		CGMEDoc::theInstance->OnCloseDocument();
	EndWaitCursor();
	return true;
}

void CGMEApp::OnFileCloseproject() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileCloseproject\r\n"));
	SaveAllModified();
#ifdef _DEBUG
	//CoFreeUnusedLibraries();
	HMODULE mga = GetModuleHandle(L"Mga.dll");
	if (mga)
	{
		typedef HRESULT (__stdcall *DllCanUnloadNow)(void);
		DllCanUnloadNow proc = (DllCanUnloadNow)GetProcAddress(mga, "DllCanUnloadNow");
		if (proc && (*proc)() != S_OK)
		{
			DebugBreak();
			// If Mga.dll is compiled with /D_ATL_DEBUG_INTERFACES, this will dump the leaks via OutputDebugString (then crash later)
			FreeLibrary(mga);
		}
	}
#endif
}


void CGMEApp::OnFileSaveAs() {
	CMgaOpenDlg dlg(CMgaOpenDlg::SaveAsDialog);
	CString conn = dlg.AskMGAConnectionString();

	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileSaveAs ")+conn+_T("\r\n"));

	if( conn.IsEmpty() )
		return;
	BeginWaitCursor();
	SaveProject(conn);
	EndWaitCursor();
}

void CGMEApp::OnFileSave() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileSave\r\n"));
	// GME-307 Focus must be killed to flush ObjectInspector and Browser
	::SetFocus(NULL);

	BeginWaitCursor();
	SaveProject(_T(""));
	EndWaitCursor();
}

void CGMEApp::OnFileAbortProject() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileAbortProject\r\n"));
	((CMainFrame*)m_pMainWnd)->clearMgaProj();

	long l;
	COMTHROW(mgaProject->get_ProjectStatus(&l));
	if(!abort_on_close && IsUndoPossible() && (l & PROJECT_STATUS_CHANGED) &&
		AfxMessageBox(_T("Discard edits to project ") + projectName + _T("?"), 
		MB_OKCANCEL) == IDCANCEL) {
		return;
	}
	abort_on_close = true;

	BeginWaitCursor();
	if(CGMEDoc::theInstance)
		CGMEDoc::theInstance->OnCloseDocument(true);
	EndWaitCursor();
}


void CGMEApp::OnFileExportxml() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileExportxml "));
	MSGTRY
	{
		CComPtr<IMgaDumper> dumper;
		COMTHROW( dumper.CoCreateInstance(L"Mga.MgaDumper") );
		ASSERT( dumper != NULL );

		CString initialFile;
		CString initialDir;
		if (theApp.isMgaProj()) {
			getMgaPaths(initialFile, initialDir);
			if (initialFile.Right(3) == _T("mga")) {
				initialFile.Truncate(initialFile.GetLength() - 3);
				initialFile += _T("xme");
			}
		}

		CFileDialog dlg(FALSE, _T("xme"), initialFile,
			OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR,
			_T("Exported Files (*.xme)|*.xme|All Files (*.*)|*.*||"));
		if (initialDir)
		{
			dlg.GetOFN().lpstrInitialDir = initialDir;
		}
		if( dlg.DoModal() != IDOK )
		{
			CGMEEventLogger::LogGMEEvent(_T("Canceled\r\n"));
			return;
		}
		CGMEEventLogger::LogGMEEvent(dlg.GetPathName()+_T("\r\n"));

		CWaitCursor wait;
		IMgaDumper2Ptr dumper2 = (IMgaDumper*)dumper;
		if (dumper2 && m_pMainWnd)
			dumper2->__DumpProject2(theApp.mgaProject, _bstr_t(dlg.GetPathName()), (ULONGLONG)(m_pMainWnd->GetSafeHwnd()));
		else
			dumper->__DumpProject(theApp.mgaProject, _bstr_t(dlg.GetPathName()));

		if( CMainFrame::theInstance) CMainFrame::theInstance->m_console.Message( CString( _T("Project successfully exported into ")) + dlg.GetPathName() + _T("."), 1);
	}
	MSGCATCH(_T("Error while generating XML file"),;)
}


void CGMEApp::OnFileImportxml() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileImportxml "));

	CString new_file_name = _T("");

	MSGTRY
	{
		CFileDialog dlg(TRUE, _T("xme"), (LPCTSTR) new_file_name,
			OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
			OFN_FILEMUSTEXIST,
			_T("GME Exported Files (*.xme;*.xml)|*.xme; *.xml|All Files (*.*)|*.*||"));
		if( dlg.DoModal() != IDOK )
		{
			CGMEEventLogger::LogGMEEvent(_T("Cancelled\r\n"));
			return;
		}
		CGMEEventLogger::LogGMEEvent(dlg.GetPathName()+_T("\r\n"));
		CString fullPath = dlg.GetPathName();
		CString fname = dlg.GetFileName();
		CString ftitle = dlg.GetFileTitle();
		Importxml(fullPath, fname, ftitle);
	}
	MSGCATCH(_T("Error importing XML file"),;)
}		
		
void CGMEApp::Importxml(CString fullPath, CString fname, CString ftitle)
{
		CComPtr<IMgaParser> parser;
		COMTHROW( parser.CoCreateInstance(L"Mga.MgaParser") );
		ASSERT( parser != NULL );

		CString folderPath = fullPath.Left(fullPath.GetLength() - fname.GetLength());

		if (fullPath.Right(3).CompareNoCase(_T("xml")) == 0 ) {
			AfxMessageBox(
					_T("Newer versions of GME use the \".xme.\" filename extension\n")
					_T("for exported XML data files.\n")
					_T("Please, rename your existing files to avoid further problems!\n"), 
					MB_OK | MB_ICONINFORMATION);
		}

		bool newproject = !mgaProject;
		if (newproject) {
			CString dataconn;
			COMTRY {
				CComBstrObj xmeparadigm, xmeparversion, xmebasename, xmeversion;
				CComVariant xmeparguid, parguid;

				COMTHROW( parser->GetXMLInfo(PutInBstr(fullPath), PutOut(xmeparadigm), PutOut(xmeparversion), &xmeparguid, PutOut(xmebasename), PutOut(xmeversion)) );
				parguid = xmeparguid;

				CMgaOpenDlg opdlg(CMgaOpenDlg::ImportDialog);
				if (ftitle.IsEmpty())
					opdlg.SetFileNameHint(PutInCString(xmebasename));
				else
					opdlg.SetFileNameHint(ftitle);
				opdlg.SetFolderPathHint(folderPath);
				dataconn = opdlg.AskConnectionString(false, false);
				if (dataconn.IsEmpty()) {
				   CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileImportxml exited because empty connection string has been given"));
				   return;
				}

				{
					CComPtr<IMgaRegistrar> reg;
					COMTHROW( reg.CoCreateInstance(L"Mga.MgaRegistrar") );
					CComBstrObj conn;
					HRESULT h1 = reg->QueryParadigm(xmeparadigm, PutOut(conn), &xmeparguid, REGACCESS_PRIORITY);
					CComVariant regparguid;
					conn.Empty();
					HRESULT h2 = reg->QueryParadigm(xmeparadigm, PutOut(conn), &regparguid, REGACCESS_PRIORITY);
					CComBstrObj regCurrentVersion;
					if (SUCCEEDED(h2)) {
						reg->VersionFromGUID(xmeparadigm, regparguid, &regCurrentVersion.p, REGACCESS_PRIORITY);
					}

					CComVariant regGuidFromVersion;
					HRESULT h3GuidFromVersion = E_FAIL;
					if (xmeparversion.Length() != 0) {
						h3GuidFromVersion = reg->GUIDFromVersion(xmeparadigm, xmeparversion, &regGuidFromVersion, REGACCESS_PRIORITY);
						if (SUCCEEDED(h3GuidFromVersion))
						{
							xmeparguid = regGuidFromVersion;
						} else
						{
							h1 = E_MGA_PARADIGM_NOTREG;
						}
					}

					TCHAR buf[300];
					if(h2 != S_OK) {
						ASSERT(h1 != S_OK);
						CString msg = _T("Could not find paradigm paradigm '") + CString(xmeparadigm) + "'";
						if (CString(xmeparadigm) == _T("MetaGME2000"))
							msg += _T("\n (In GME3 the MetaGME2000 paradigm was renamed to MetaGME)");
						msg += _T("\nDo you want to import with an other registered paradigm ?");
						if (AfxMessageBox(msg ,MB_OKCANCEL) == IDOK) {	
							CComObjPtr<IMgaLauncher> launcher;
							COMTHROW( launcher.CoCreateInstance(CComBSTR(L"Mga.MgaLauncher")) );
							if (SUCCEEDED(launcher->MetaDlg(METADLG_NONE))) {
								// parguid = true;
								parguid.Clear();
								xmeparadigm.Empty();
								COMTHROW( launcher->get_ParadigmName(PutOut(xmeparadigm)) );
							}
							else {
								return;   // safe before create
							}
						}
						else {
							return;   // safe before create
						}
					}
					else {
						CComBstrObj bstrxmeparguid, bstrregparguid;
						GUID gg;

						CopyTo(xmeparguid,gg);
						CopyTo(gg, bstrxmeparguid);

						CopyTo(regparguid,gg);
						CopyTo(gg, bstrregparguid);

						if(h1 != S_OK) {
							_stprintf_s(buf, _T("Could not locate paradigm %s, version '%s'.\n\n")
										 _T("Do you want to upgrade to the current version ('%s') instead?"),
										 static_cast<const TCHAR*>(xmeparadigm),
										 static_cast<const TCHAR*>(xmeparversion.Length() != 0 ? xmeparversion : bstrxmeparguid),
										 static_cast<const TCHAR*>(regCurrentVersion.Length() != 0 ? regCurrentVersion : bstrregparguid));
							if(AfxMessageBox(buf, MB_OKCANCEL | MB_ICONQUESTION) == IDOK) {
								parguid = regparguid;
							}
							else {
								// AfxMessageBox(_T("Import canceled"));
								return; // safe before create
							}	

						}
						else if(bstrxmeparguid.Compare(bstrregparguid)) {
							_stprintf_s(buf, _T("This model was exported using paradigm %s, version '%s'.\n\n")
										 _T("Do you want to upgrade to the current version ('%s')?"),
										 static_cast<const TCHAR*>(xmeparadigm),
										 static_cast<const TCHAR*>(xmeparversion.Length() != 0 ? xmeparversion : bstrxmeparguid),
										 static_cast<const TCHAR*>(regCurrentVersion.Length() != 0 ? regCurrentVersion : bstrregparguid));
							int answer = AfxMessageBox(buf,MB_YESNOCANCEL | MB_ICONQUESTION);
							if(answer == IDYES) {
								parguid = regparguid;
							}
							else if(answer == IDCANCEL) {
								// AfxMessageBox(_T("Import canceled"));
								return;  // safe before create
							}
						}
					}
				}
				COMTHROW( mgaProject.CoCreateInstance(L"Mga.MgaProject") );
				COMTHROW( mgaProject->EnableAutoAddOns(VARIANT_TRUE));
				HRESULT hr = mgaProject->CreateEx(PutInBstr(dataconn), PutInBstr(xmeparadigm), parguid);
				if(hr == E_MGA_PARADIGM_NOTREG || hr == E_MGA_PARADIGM_INVALID) {
					TCHAR buf[300];
					CComBstrObj parguid1;
					GUID gg;
					CopyTo(parguid,gg);
					CopyTo(gg, parguid1);
					_stprintf_s(buf, _T("Could not open paradigm %s\nVersion ID: %s"), 
						static_cast<const TCHAR*>(xmeparadigm), static_cast<const TCHAR*>(parguid1));

					AfxMessageBox(buf);
				}
				if(hr == E_MGA_COMPONENT_ERROR) {
					_bstr_t errorInfo;
					GetErrorInfo(errorInfo.GetAddress());
					_bstr_t err(_T("ERROR: automatic addon components could not start up:\n"));
					err += errorInfo;
					err += "\nDo you want to create the project without addons?";
					if (AfxMessageBox(err, MB_YESNO) == IDYES) {
						mgaProject->EnableAutoAddOns(VARIANT_FALSE);
						hr = mgaProject->CreateEx(PutInBstr(dataconn), PutInBstr(xmeparadigm), parguid);
					}
				}
				COMTHROW(hr);
				AfterOpenOrCreateProject(dataconn); 
			} catch(hresult_exception &e) {
				CloseProject();
				DisplayError(_T("Could not create the project"), e.hr); 
				throw;
			}
		}

		//UpdateProjectName(); // moved below

		CWaitCursor wait;
		if(mgaConstMgr) COMTHROW(mgaConstMgr->Enable(VARIANT_FALSE));

		CString file_name = fullPath;
		if( CMainFrame::theInstance) CMainFrame::theInstance->m_console.Message( CString( _T("Importing ")) + file_name + _T("..."), 1);
		IMgaParser2Ptr parser2 = (IMgaParser*)parser;
		if (parser2 && m_pMainWnd)
			parser2->__ParseProject2(theApp.mgaProject, _bstr_t(fullPath), (ULONGLONG)(m_pMainWnd->GetSafeHwnd()));
		else
			COMTHROW(parser->ParseProject(theApp.mgaProject, PutInBstr(fullPath)));
		
		// mgaproject has been filled with data, let's update title:
		UpdateProjectName();

		if(newproject && (proj_type_is_mga||proj_type_is_xmlbackend)) {
			OnFileSave();
		}

		if (CMainFrame::theInstance)
			CMainFrame::theInstance->m_console.Message(fullPath + _T(" was successfully imported."), 1);
		else
			AfxMessageBox(fullPath + _T(" was successfully imported."));

	if (mgaConstMgr)
		COMTHROW(mgaConstMgr->Enable(VARIANT_TRUE));
	HRESULT hr = mgaProject->Notify(GLOBALEVENT_OPEN_PROJECT_FINISHED);
	ASSERT(SUCCEEDED(hr));
}



void CGMEApp::OnFileXMLUpdate() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileXMLUpdate\r\n"));
    ASSERT(mgaProject);
    ASSERT(mgaMetaProject);

	TCHAR xmlname[MAX_PATH];
	GetTempFileName(_T("."), _T("XEX"),0, xmlname);
	
	if(currentConnection.Find(_T("MGA=")) != 0) {
		AfxMessageBox(_T("Function is available only for .mga models"));
	}

	CString fname = currentConnection.Mid(4);
	CString backupname;

	MSGTRY
	{
		CWaitCursor wait;
		CComBstrObj parname;
		COMTHROW( mgaMetaProject->get_Name(PutOut(parname)) );
		CComBstrObj metaconn;

        CComPtr<IMgaRegistrar> reg;
        COMTHROW( reg.CoCreateInstance(L"Mga.MgaRegistrar") );
		CComBstrObj currentguid;
        GUID currentguid_guid;
		{
			CComVariant parguid;
			COMTHROW( mgaMetaProject->get_GUID(&parguid) );

			CComVariant pg2;

			COMTHROW(reg->QueryParadigm(parname, PutOut(metaconn), &pg2, REGACCESS_PRIORITY));

			CComBstrObj parguid1;
			GUID gg;
			CopyTo(parguid,gg);
			CopyTo(gg, parguid1);

			CopyTo(pg2,gg);
            currentguid_guid = gg;
			CopyTo(gg, currentguid);

			if(!parguid1.Compare(currentguid)) {
				TCHAR buf[200];
				_stprintf_s(buf, _T("There is no need to upgrade this model\nIts Meta Version ID is the current ID\nCurrent ID: %s"), 
					static_cast<const TCHAR*>(currentguid));
				AfxMessageBox(buf);
				return;
			}
		}

		CComPtr<IMgaDumper> dumper;
		COMTHROW( dumper.CoCreateInstance(L"Mga.MgaDumper") );
		ASSERT( dumper != NULL );

		COMTHROW( dumper->DumpProject(theApp.mgaProject, PutInBstr(CString(xmlname))) );


		SafeCloseProject();
		ASSERT(!mgaProject);

		backupname = fname;
		int p = backupname.ReverseFind('.');
		if(!p || backupname.Find('\\',p) != -1) p = backupname.GetLength();
		backupname.Insert(p,_T("-backup"));
		DeleteFile(backupname);
		if(!MoveFile(fname, backupname)) {
			backupname = fname;
			CString buf;
			buf.Format(L"Could not save original file '%s' to '%s'",
				static_cast<const TCHAR*>(fname), static_cast<const TCHAR*>(backupname)); 
			AfxMessageBox(buf);
			COMTHROW(E_NOTFOUND);
		}



		CreateProject(PutInCString(parname), currentConnection);

		if(!mgaProject || !mgaMetaProject) {
			AfxMessageBox(_T("Error creating project"));
			return;
		}

		UpdateComponentLists();
		ChangedProjectConnStrings();

		CComPtr<IMgaParser> parser;
		COMTHROW( parser.CoCreateInstance(L"Mga.MgaParser") );
		ASSERT( parser != NULL );
	    if(mgaConstMgr) COMTHROW(mgaConstMgr->Enable(VARIANT_FALSE));
		IMgaParser2Ptr parser2 = (IMgaParser*)parser;
		if (parser2 && m_pMainWnd)
			COMTHROW(parser2->ParseProject2(mgaProject,PutInBstr(CString(xmlname)), (ULONGLONG)(m_pMainWnd->GetSafeHwnd())));
		else
			COMTHROW(parser->ParseProject(mgaProject,PutInBstr(CString(xmlname))) );
		{
            _bstr_t version;
			_variant_t v_guid;
			CopyTo(currentguid_guid, &v_guid);
            reg->VersionFromGUID(parname, v_guid, version.GetAddress(), REGACCESS_PRIORITY);
			CString buf = CString(_T("The model has been updated.\nCurrent version: "))
				+ (version.length() ? static_cast<const wchar_t*>(version) : currentguid) + L"\nThe original model has been saved to " + backupname;
			AfxMessageBox(buf);
		}
	}
	catch(hresult_exception &e)	{
		if(backupname.IsEmpty()) {
			CString buf;
			buf.Format(L"The upgrade failed: 0x%x\nThe model has not been closed", e.hr);
			AfxMessageBox(buf);
		}
		else {
			if(backupname.Compare(fname)) {
				if(MoveFile(backupname, fname)) backupname = fname;
			}
			CString buf;
			buf.Format(L"The upgrade failed: 0x%x\nThe original model is in file %s", e.hr, 
				static_cast<const TCHAR*>(backupname));
			AfxMessageBox(buf);
		}
	}

    if (mgaConstMgr) COMTHROW(mgaConstMgr->Enable(VARIANT_TRUE));
}



void CGMEApp::OnFileRegcomponents() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileRegcomponents "));
	MSGTRY
	{
		CComObjPtr<IMgaLauncher> launcher;
		COMTHROW( launcher.CoCreateInstance(L"Mga.MgaLauncher") );
		COMTHROW( launcher->put_ParadigmName(PutInBstr(guiMetaProject->name)));
		COMTHROW( launcher->put_ComponentType(COMPONENTTYPE_ALL) );

		ATLASSERT(mgaProject);
		CComBSTR runningcomps;
		{
			CComPtr<IMgaComponents> comps;
			COMTHROW(mgaProject->get_AddOnComponents(&comps));
			MGACOLL_ITERATE(IMgaComponent, comps) {
				CComQIPtr<IMgaComponentEx> compx = MGACOLL_ITER;
				CComBSTR p;
				if(compx) COMTHROW(compx->get_ComponentProgID(&p));
				else COMTHROW(MGACOLL_ITER->get_ComponentName(&p));
				if(runningcomps) runningcomps += _T(" ");
				runningcomps += p;
			}
			MGACOLL_ITERATE_END;
		}
		// FIXME: huge kludge here: enables CCompDlg to show which addons are running
		COMTHROW(launcher->put_Parameter(CComVariant(runningcomps)));

		CGMEEventLogger::LogGMEEvent(CString(runningcomps)+_T("\r\n"));

		COMTHROW( launcher->ComponentDlg(COMPONENTDLG_INTERP));
		UpdateComponentLists(true);
	}
	MSGCATCH(_T("Error while trying to register the interpreter"),;)
}

void CGMEApp::OnFileSettings() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileSettings\r\n"));
	MSGTRY
	{
		CComObjPtr<IMgaLauncher> launcher;
		COMTHROW( launcher.CoCreateInstance(L"Mga.MgaLauncher") );
		COMTHROW( launcher->GmeDlg());
	}
	MSGCATCH(_T("Error while trying to get GME settings"),;)
	GetSettings();
}

void CGMEApp::OnFileClearLocks() 
{
	if( mgaProject != NULL || mgaMetaProject != NULL )
		return;

	MSGTRY
	{
		CMgaOpenDlg dlg(CMgaOpenDlg::ClearLocksDialog);
		CString conn = dlg.AskConnectionString(false, true);

		if( conn.IsEmpty() )
			return;

		CWaitCursor wait;

		CComObjPtr<IMgaProject> project;
		COMTHROW( project.CoCreateInstance(OLESTR("MGA.MgaProject")) );
		ASSERT( project != NULL );
		COMTHROW( project->CheckLocks(PutInBstr(conn), VARIANT_TRUE) );

		AfxMessageBox(_T("Database locks are cleared"));
	}
	MSGCATCH(_T("Error while clearing locks in database"),;)
}

void CGMEApp::OnHelpContents() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnHelpContents\r\n"));

	MSGTRY {
		CComObjPtr<IMgaLauncher> launcher;
		COMTHROW( launcher.CoCreateInstance(L"Mga.MgaLauncher") );
		COMTHROW( launcher->ShowHelp(NULL) );
	}
	MSGCATCH(_T("Error while showing help contents."),;)
}

void CGMEApp::OnFileCheckall() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileCheckall\r\n"));
	ASSERT(mgaConstMgr);
	if (!mgaConstMgr)
		return;
	// message boxes displayed from constraint manager if in interactive mode
	mgaConstMgr->ObjectsInvokeEx(mgaProject, NULL, NULL, NULL);
}

void CGMEApp::OnFileDisplayConstraints() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileDisplayConstraints\r\n"));
	ASSERT(mgaConstMgr);
	if (!mgaConstMgr)
		return;
	mgaConstMgr->ObjectsInvokeEx(mgaProject, NULL, NULL, CONSTMGR_SHOW_CONSTRAINTS);
	
}



void CGMEApp::OnFileRegparadigms() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnFileRegparadigms\r\n"));
	MSGTRY
	{
		CComPtr<IMgaLauncher> launcher;
		COMTHROW( launcher.CoCreateInstance(L"Mga.MgaLauncher", 0, CLSCTX_INPROC_SERVER) );

		while(1) {
			HRESULT hr = launcher->MetaDlg(METADLG_PARREG);
			if( hr == S_FALSE )	return;

			COMTHROW( launcher->put_ComponentType(COMPONENTTYPE_ALL) );

			bool workonrunningparadigm = false;
			if(guiMetaProject) {
				CComBstrObj metaname;
				COMTHROW( launcher->get_ParadigmName(PutOut(metaname)) );
				if(metaname == PutInBstr(guiMetaProject->name)) workonrunningparadigm = true;
			}

			if(workonrunningparadigm) {
				ATLASSERT(mgaProject);
				CComBSTR runningcomps;
				CComPtr<IMgaComponents> comps;
				COMTHROW(mgaProject->get_AddOnComponents(&comps));
				MGACOLL_ITERATE(IMgaComponent, comps) {
					CComQIPtr<IMgaComponentEx> compx = MGACOLL_ITER;
					CComBSTR p;
					if(compx) COMTHROW(compx->get_ComponentProgID(&p));
					else COMTHROW(MGACOLL_ITER->get_ComponentName(&p));
					if(runningcomps) runningcomps += _T(" ");
					runningcomps += p;
				}
				MGACOLL_ITERATE_END;
				COMTHROW(launcher->put_Parameter(CComVariant(runningcomps)));
			}

			COMTHROW( launcher->ComponentDlg(COMPONENTDLG_INTERP));

			if(workonrunningparadigm) UpdateComponentLists(true);
		}
	} MSGCATCH(_T("Error registering paradigms"), ;);
}


// *******************************************************************************

void CGMEApp::OnUpdateFileNew(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(guiMetaProject == NULL);
}

void CGMEApp::OnUpdateFileOpen(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(guiMetaProject == NULL);
}

void CGMEApp::OnUpdateFileCloseproject(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(guiMetaProject != NULL);
}

void CGMEApp::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(mgaProject != NULL && (proj_type_is_mga||proj_type_is_xmlbackend));
}

void CGMEApp::OnUpdateFileSaveAs(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(mgaProject != NULL && proj_type_is_mga);
}

void CGMEApp::OnUpdateFileAbortProject(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(mgaProject != NULL && proj_type_is_mga);
}

void CGMEApp::OnUpdateFileExportxml(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(guiMetaProject != NULL);
}

void CGMEApp::OnUpdateFileXMLUpdate(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(mgaProject != NULL && proj_type_is_mga);
}

void CGMEApp::OnUpdateFileCheckall(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(mgaConstMgr != NULL);
}

void CGMEApp::OnUpdateFileDisplayConstraints(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(mgaConstMgr != NULL);	
	
}

void CGMEApp::OnUpdateFileRegcomponents(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(guiMetaProject != NULL);
}

void CGMEApp::OnUpdateRecentProjectMenu(CCmdUI* pCmdUI)
{
	ASSERT_VALID(this);
	m_RecentProjectList.UpdateMenu(pCmdUI,guiMetaProject == NULL);
}


// *******************************************************************************
// *******************************************************************************
//								EDIT
// *******************************************************************************
// *******************************************************************************


void CGMEApp::OnEditProjectproperties() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnEditProjectproperties ") + projectName +_T("\r\n"));
	CProjectPropertiesDlg dlg;
	if(dlg.DoModal() == IDOK) {
		if(CGMEDoc::theInstance)
			CGMEDoc::theInstance->SetTitle(projectName);
		UpdateMainFrameTitle(projectName);
	}
}


void CGMEApp::OnEditUndo() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnEditUndo\r\n"));
	mgaProject->Undo();
}

void CGMEApp::OnEditRedo() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnEditRedo\r\n"));
	mgaProject->Redo();
}

void CGMEApp::OnEditClearUndo() 
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnEditClearUndo\r\n"));
	if( mgaProject == NULL )
		return;

	if(AfxMessageBox(_T("You are about to lose all Undo/Redo information. Proceed?"),MB_YESNO | MB_ICONQUESTION) == IDYES) {
		MSGTRY
		{
			COMTHROW( mgaProject->FlushUndoQueue() );
		}
		MSGCATCH(_T("Error while clearing the undo queue"),;)
	}
}


// *******************************************************************************

void CGMEApp::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
	if(guiMetaProject != NULL) {
		short undoSize;
		short redoSize;
		mgaProject->UndoRedoSize(&undoSize,&redoSize);
		pCmdUI->Enable(undoSize > 0);
	}
	else
		pCmdUI->Enable(FALSE);
}

void CGMEApp::OnUpdateEditRedo(CCmdUI* pCmdUI) 
{
	if(guiMetaProject != NULL) {
		short undoSize;
		short redoSize;
		mgaProject->UndoRedoSize(&undoSize,&redoSize);
		pCmdUI->Enable(redoSize > 0);
	}
	else
		pCmdUI->Enable(FALSE);
}

void CGMEApp::OnUpdateEditClearUndo(CCmdUI* pCmdUI) 
{
	if(mgaProject != NULL) {
		short undoSize;
		short redoSize;
		mgaProject->UndoRedoSize(&undoSize,&redoSize);
		pCmdUI->Enable(undoSize > 0);
	}
	else
		pCmdUI->Enable(FALSE);
}

void CGMEApp::OnUpdateEditProjectproperties(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(CGMEDoc::theInstance != 0);
}



/*
void CGMEApp::OnUpdateFileRunplugin1(CCmdUI* pCmdUI) 
{
	
	CMenu *filemenu = pCmdUI->m_pMenu;
	UINT idx = pCmdUI->m_nIndex;
if(idx == 0) return;
	CString label;
	ASSERT(filemenu);
	filemenu->GetMenuString(idx,label,MF_BYPOSITION);
	filemenu->DeleteMenu(idx,MF_BYPOSITION);
	CMenu pluginmenu;
	pluginmenu.CreatePopupMenu();
	pluginmenu.GetMenuItemID
	for(int i = 0; i < comps.GetSize(); ++i)	{
			pluginmenu.AppendMenu(MF_ENABLED, pCmdUI->m_nID + i,comps[i]);
	}
	filemenu->InsertMenu(idx,
						comps.GetSize() ? MF_BYPOSITION|MF_POPUP|MF_ENABLED: MF_BYPOSITION|MF_POPUP|MF_GRAYED,
						(UINT)pluginmenu.Detach(),label);
	
}
*/

void CGMEApp::OnRunPlugin(UINT nID) {
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnRunPlugin ")+plugins[nID - ID_FILE_RUNPLUGIN1]+_T("\r\n"));

	// Focus must be killed to flush ObjectInspector and Browser
	::SetFocus(NULL);

	RunComponent(plugins[nID - ID_FILE_RUNPLUGIN1]);
}

void CGMEApp::OnRunInterpreter(UINT nID) {
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnRunInterpreter ")+interpreters[nID - ID_FILE_INTERPRET1]+_T("\r\n"));

	// Focus must be killed to flush ObjectInspector and Browser
	::SetFocus(NULL);

	RunComponent(interpreters[nID - ID_FILE_INTERPRET1]);
}

void CGMEApp::OnRunComponentHelp(UINT nID) {
	UINT id = (nID - ID_HELP_COMPONENTHELP_FIRST) / 2;
	bool isEven = (nID - ID_HELP_COMPONENTHELP_FIRST) % 2 == 0;
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::OnRunComponentHelp ") +
		(id < plugins.GetSize() ?
			plugins[id] : (id < plugins.GetSize() + interpreters.GetSize() ?
				interpreters[id - plugins.GetSize()] : addons[id - plugins.GetSize() - interpreters.GetSize()])) + 
		_T("\r\n"));

	CString onlineHelpLocation = (id < plugins.GetSize() ? 
			pluginOnlineHelp[id] : (id < plugins.GetSize() + interpreters.GetSize() ? 
				interpreterOnlineHelp[id - plugins.GetSize()] : addonOnlineHelp[id - plugins.GetSize() - interpreters.GetSize()]));
	CString offlineHelpLocation = (id < plugins.GetSize() ? 
			pluginOfflineHelp[id] : (id < plugins.GetSize() + interpreters.GetSize() ? 
				interpreterOfflineHelp[id - plugins.GetSize()] : addonOfflineHelp[id - plugins.GetSize() - interpreters.GetSize()]));

	ShellExecute(NULL, L"open", (isEven ? onlineHelpLocation : offlineHelpLocation), NULL, NULL, SW_SHOWNORMAL);
}

void CGMEApp::OnRunParadigmHelp(UINT nID)
{
	CComBSTR pname = guiMetaProject->name;

	CGMEEventLogger::LogGMEEvent((_T("CGMEApp::OnRunParadigmHelp ") +
		std::wstring(static_cast<const wchar_t*>(pname)) + _T("\r\n")).c_str());
  
	CString help = (nID == ID_HELP_PARADIGMHELP_FIRST ? this->paradigmOnlineHelp : this->paradigmOfflineHelp);
	if (!help.IsEmpty())
	{
		ShellExecute(NULL, L"open", help, NULL, NULL, SW_SHOWNORMAL);
	}
}

void CGMEApp::RunComponent(const CString &compname)
{

	IMgaLauncherPtr launcher;
	launcher.CreateInstance(L"Mga.MgaLauncher");
	if(!launcher) {
		AfxMessageBox(_T("Cannot start up component launcher"));
	}
	else {
		CComPtr<IMgaFCO> focus;
		CComPtr<IMgaFCOs> selfcos;
		COMTHROW(selfcos.CoCreateInstance(OLESTR("Mga.MgaFCOs")));
		CMDIChildWnd *pChild  = CMainFrame::theInstance->MDIGetActive();
		if (pChild) {
#if !defined (ACTIVEXGMEVIEW)
			CGMEView *view = (CGMEView*)pChild->GetActiveView();
			if (view) {
				COMTHROW(view->currentModel.QueryInterface(&focus));
				POSITION pos = view->selected.GetHeadPosition();
				while (pos) {
					CGuiFco *gfco = view->selected.GetNext(pos);
					COMTHROW(selfcos->Append(gfco->mgaFco));
				}
			}
#endif
		}

		if(theApp.bNoProtect) COMTHROW( launcher->put_Parameter(CComVariant(true)));
		// Disable the DCOM wait dialogs: if interpreters want them, they can do it themselves; but if they don't want them, they need to link to GME's mfc1xxu.dll
		COleMessageFilter* messageFilter = AfxOleGetMessageFilter();
		messageFilter->EnableBusyDialog(FALSE);
		messageFilter->EnableNotRespondingDialog(FALSE);
		std::shared_ptr<COleMessageFilter> busyRestore(messageFilter, [](COleMessageFilter* filter){ filter->EnableBusyDialog(TRUE); } );
		std::shared_ptr<COleMessageFilter> notRespondingRestore(messageFilter, [](COleMessageFilter* filter){ filter->EnableNotRespondingDialog(TRUE); } );
		if(launcher->RunComponent(PutInBstr(compname), mgaProject, focus, selfcos, GME_MENU_START) != S_OK) {
			CComObjPtr<IErrorInfo> errinfo;
			GetErrorInfo(0, PutOut(errinfo));
			if (errinfo) {
				_bstr_t desc;
				errinfo->GetDescription(desc.GetAddress());
				std::wstring error;
				error += L"Component execution failed: ";
				error += desc;
				AfxMessageBox(error.c_str());
			} else {
				AfxMessageBox(_T("Component execution failed"));
			}
		}
	}
}

class CGmePrintSetup : public CPrintDialog
{
public:
	enum { IDD = IDD_PRINT_DIALOG  };
	CGmePrintSetup() : CPrintDialog(true, 0, NULL) {}
};

void CGMEApp::OnUniquePrintSetup()
{
	CGmePrintSetup setup;
	setup.m_pd.Flags =	PD_PRINTSETUP |  // actually the current aspect
								PD_ENABLESETUPTEMPLATE |
								PD_ENABLEPRINTHOOK | PD_ENABLESETUPHOOK ;
	setup.m_pd.hInstance = AfxGetInstanceHandle();
	setup.m_pd.lpSetupTemplateName = MAKEINTRESOURCE(IDD_PRINTSETUP_DIALOG);
	DoPrintDialog(&setup);
}

void CGMEApp::ImportDroppedFile(const CString& fname)
{
	CGMEEventLogger::LogGMEEvent(_T("CGMEApp::ImportDroppedFile "));

	CString file_name = fname;

	MSGTRY
	{
		int fnameStart = fname.ReverseFind('\\');
		if (fnameStart == -1)
			fnameStart = fname.ReverseFind('/');

		CString ftitle;
		CString fpath;
		if (fnameStart != -1) {
			fpath = fname.Left(fnameStart + 1);
			ftitle = fname.Right(fname.GetLength() - fnameStart - 1);
			int extStart = ftitle.ReverseFind('.');
			if (extStart != -1)
				ftitle = ftitle.Left(extStart);
		}
		Importxml(file_name, file_name.Right(file_name.GetLength() - fnameStart), ftitle);
	}
	MSGCATCH(_T("Error importing XML file"),;)
}

void CGMEApp::RegisterDroppedFile( const CString& fname, bool userReg/* = true*/)
{
	try
	{
		CWaitCursor wait;
		IMgaRegistrarPtr registrar;
		COMTHROW(registrar.CreateInstance(L"Mga.MgaRegistrar"));

		regaccessmode_enum reg_access = REGACCESS_USER;

		_bstr_t newname;
		registrar->__RegisterParadigmFromData(_bstr_t(fname), newname.GetAddress(), reg_access);

		CMainFrame::theInstance->m_console.Message( _T("Done."), 1);
	}
	catch( hresult_exception &e)
	{
		CMainFrame::theInstance->m_console.Message( _T("Error while registering paradigm."), 3);
	}
	catch(_com_error &e)
	{
		_bstr_t error(L"");
		if (e.Description() != _bstr_t())
			error = e.Description();
		else
			GetErrorInfo(e.Error(), error.GetAddress());
		CMainFrame::theInstance->m_console.Message(CString("Error while registering paradigm: ") + static_cast<const wchar_t*>(error), 3);
	}
}

void CGMEApp::OnUpdateFilePluginX(CCmdUI* pCmdUI)
{
	bool enabled = m_vecDisabledPlugIns.find( pCmdUI->m_nID) == m_vecDisabledPlugIns.end();
	if( pCmdUI->m_nID >= ID_FILE_RUNPLUGIN1 && pCmdUI->m_nID <= ID_FILE_RUNPLUGIN_LAST)
		pCmdUI->Enable( enabled );
}

void CGMEApp::OnUpdateFileInterpretX(CCmdUI* pCmdUI)
{
	bool enabled = m_vecDisabledComps.find( pCmdUI->m_nID) == m_vecDisabledComps.end();
	if( pCmdUI->m_nID >= ID_FILE_INTERPRET1 && pCmdUI->m_nID <= ID_FILE_INTERPRET_LAST)
		pCmdUI->Enable( enabled );
}

// this method is used internally
void CGMEApp::ClearDisabledComps()
{
	m_vecDisabledPlugIns.clear();
	m_vecDisabledComps.clear();
}

// this method is called from CGMEOLEApp::DisableComp
//                        and CGMEApp::UpdateCompList4CurrentKind
void CGMEApp::DisableComp( const CString& pCompToFind, bool pbHide)
{
	UINT id_of_comp = 0;
	bool is_plugin = false;
	for(int i = 0; !id_of_comp && i < plugins.GetSize(); ++i)	
	{
		if( plugins[i] == pCompToFind)
		{
			id_of_comp = ID_FILE_RUNPLUGIN1 + i;
			is_plugin = true;
		}
	}

	for(int i = 0; !id_of_comp && i < interpreters.GetSize(); ++i)
	{
		if( interpreters[i] == pCompToFind)
		{
			id_of_comp = ID_FILE_INTERPRET1 + i;
			is_plugin = false;
		}
	}

	if( id_of_comp) // a valid command ID
	{
		//ActivateComp( id_of_comp, is_plugin, pbShow);
		if( is_plugin)
		{
			ONE_ID_LIST::const_iterator pos = m_vecDisabledPlugIns.find( id_of_comp);
			if( pbHide) // disable, so id_of_comp must be inserted into the vector
			{
				if( m_vecDisabledPlugIns.end() == pos) // not found
					m_vecDisabledPlugIns.insert( m_vecDisabledPlugIns.end(), id_of_comp);
			}
			else // enable, so remove id_of_comp from the vector
			{
				if( m_vecDisabledPlugIns.end() != pos) // if really found
					m_vecDisabledPlugIns.erase( pos);
			}
		}
		else
		{
			ONE_ID_LIST::const_iterator pos = m_vecDisabledComps.find( id_of_comp);
			if( pbHide) // disable, so id_of_comp must be inserted into the vector
			{
				if( m_vecDisabledComps.end() == pos) // not found
					m_vecDisabledComps.insert( m_vecDisabledComps.end(), id_of_comp);
			}
			else // enable, so remove id_of_comp from the vector
			{
				if( m_vecDisabledComps.end() != pos) // if really found
					m_vecDisabledComps.erase( pos);
			}
		}
	}
}

// this method is called from CGMEOLEApp::DisableCompForKinds
void CGMEApp::DisableCompForKinds( const CString& pComp, const CString& pKindSeq)
{
	int nm_of_tokens = 0; // will count the parsed kind names
	int pos = 0;
	CString t_kind;
	t_kind = pKindSeq.Tokenize( _T(";"), pos); // tokenize by ';'
	while( t_kind != _T(""))
	{
		ONE_COMP_LIST &my_comps = m_compsOfKind[ t_kind ];
		ONE_COMP_LIST::const_iterator it = my_comps.find( pComp);
		if( it == my_comps.end()) // not found, so insert it
			m_compsOfKind[ t_kind ].insert( pComp);

		++nm_of_tokens;
		t_kind = pKindSeq.Tokenize( _T(";"), pos); // move to next token
	}
}

// this method is called from CGMEView::OnActivateView    (with the kindname shown)
//                            CGMEView::~CGMEView         (with the special string: _NO_MODEL_IS_OPEN_ == m_no_model_open_string)
//                            CGMEApp::SetCompFiltering   (with one from the items above)
void CGMEApp::UpdateCompList4CurrentKind( const CString& pKind)
{
	// if filter is OFF return
	if( !m_compFilterOn) 
	{
		return;
	}

	// kind name should not be empty
	if( pKind.IsEmpty()) { ASSERT(0); return; }

	ClearDisabledComps(); // reset all to original state (enabled)

	if( m_compsOfKind.find( pKind) != m_compsOfKind.end()) // if it has an entry
	{
		ONE_COMP_LIST &my_comps = m_compsOfKind[ pKind]; // this key already existed
		for( ONE_COMP_LIST::const_iterator it = my_comps.begin(); it != my_comps.end(); ++it)
		{
			DisableComp( *it, true); // disable comps which are registered for this kind
		}
	} // if this kind has no assigned [excluded] component set: NOP [every component is enabled]
}

void CGMEApp::SetCompFiltering( bool pOn)
{ 
	m_compFilterOn = pOn;
	if( !m_compFilterOn) // if turned OFF
	{
		ClearDisabledComps(); // reset all to original state (enabled)
	}
	else // turned ON
	{
		// find the current view
		bool found = false;
		CMDIChildWnd *pChild  = CMainFrame::theInstance->MDIGetActive();
		if( pChild) 
		{
#if !defined (ACTIVEXGMEVIEW)
			CGMEView *view = (CGMEView*)pChild->GetActiveView();
			if( view && view->guiMeta)
			{
				found = true;
				this->UpdateCompList4CurrentKind( view->guiMeta->name);
			}
#endif
		}

		if( !found)
			this->UpdateCompList4CurrentKind( CGMEApp::m_no_model_open_string);
	}
}

bool CGMEApp::GetCompFiltering()
{
	return m_compFilterOn;
}

bool CGMEApp::SetWorkingDirectory( LPCTSTR pPath)
{
	return SetCurrentDirectory( pPath) == TRUE;
	//int sc = _chdir( pPath);
	//return sc == 0;
}

void CGMEApp::UpdateMainTitle(bool retrievePath)
{
	if (CMainFrame::theInstance)
		UpdateMainFrameTitle(projectName, retrievePath);
}

bool CGMEApp::IsUndoPossible(void) const
{
	if(!mgaProject)
		return false;

	if (guiMetaProject != NULL) {
		short undoSize;
		short redoSize;
		mgaProject->UndoRedoSize(&undoSize,&redoSize);
		return undoSize > 0;
	}

	return false;
}

void CGMEApp::OnFocusBrowser()
{
	if( !mgaProject) return;

	CComObjPtr<IMgaTerritory> terry;
	COMTHROW(mgaProject->CreateTerritory(NULL, PutOut(terry), NULL) );

	LPUNKNOWN objs = CGMEObjectInspector::theInstance->GetObjects();
	CComQIPtr<IMgaObjects> one_obj_coll( objs);
	long len = 0;
	if( one_obj_coll) COMTHROW( one_obj_coll->get_Count( &len));
	if( len >= 1)
	{
		MSGTRY
		{
			CComPtr<IMgaObject> one_obj, one_obj2;
			long status = OBJECT_ZOMBIE;
			CComBSTR id;
			COMTHROW( one_obj_coll->get_Item( 1, &one_obj));
			COMTHROW( mgaProject->BeginTransaction(terry,TRANSACTION_READ_ONLY));

			if( one_obj)  COMTHROW( terry->OpenObj( one_obj, &one_obj2));
			if( one_obj2) {
				COMTHROW( one_obj2->get_Status( &status));
				COMTHROW( one_obj2->get_ID( &id));
			}

			COMTHROW( mgaProject->CommitTransaction());

			if( id.Length() > 0 && status == OBJECT_EXISTS) 
			{
				CGMEBrowser::theInstance->FocusItem( id);
				return;
			}
		}
		MSGCATCH(_T("Error getting project rootfolder"), mgaProject->AbortTransaction())
	}

	MSGTRY
	{
		CComBSTR id;
		CComPtr<IMgaFolder> rf;
		COMTHROW(mgaProject->BeginTransaction(terry,TRANSACTION_READ_ONLY));
		COMTHROW(mgaProject->get_RootFolder( &rf));
		if( rf) COMTHROW(rf->get_ID(&id));
		COMTHROW(mgaProject->CommitTransaction());

		if( id.Length() > 0) 
			CGMEBrowser::theInstance->FocusItem( id);
	}
	MSGCATCH(_T("Error getting project rootfolder"), mgaProject->AbortTransaction())
}

void CGMEApp::OnFocusInspector()
{
	HWND hwnd = CGMEObjectInspector::theInstance->GetSafeHwnd();
	if( hwnd) ::SetFocus( hwnd);
}

void CGMEApp::consoleMessage( const CString& p_msg, short p_type)
{
	if( CMainFrame::theInstance) // 99.99% of cases
		CMainFrame::theInstance->m_console.Message( p_msg, p_type);
	else
		AfxMessageBox( p_msg);
}

#ifdef _DEBUG
extern "C" FILE * __cdecl __iob_func(void)
{
	terminate();
	return NULL;
}

// https://msdn.microsoft.com/en-us/library/bb531344.aspx#BK_CRT "Visual C++ 2015 Conformance Changes   C Runtime Library(CRT)"
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

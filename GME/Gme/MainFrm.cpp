// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "GMEApp.h"

#include "GMEstd.h"
#include "MainFrm.h"
#include "GMEDoc.h"
#include "GMEChildFrame.h"
#include "Splash.h"
#include "GMEEventLogger.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "ChildFrm.h"
#include "GMEView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Tooltip helper for components

afx_msg BOOL CComponentBar::OnTT(UINT, NMHDR * pNMHDR, LRESULT * ) {
	TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
	UINT_PTR nIndex =pNMHDR->idFrom;

	if(nIndex == 1)
	{
		_tcsncpy(pTTT->szText, _T("Check constraints"), 79);
		return FALSE;
	}

	if(nIndex == 2)
	{
		_tcsncpy(pTTT->szText, _T("Interpret the current model"), 79);
		return FALSE;
	}


	CMFCToolBarButton* pButton = GetButton(nIndex - 1);
	ASSERT(pButton);
	if (pButton) {
		UINT nID = pButton->m_nID;

		if(nID >= ID_FILE_RUNPLUGIN1 && nID <= ID_FILE_RUNPLUGIN_LAST) {
			_tcsncpy(pTTT->szText, theApp.pluginTooltips[nID-ID_FILE_RUNPLUGIN1], 79);
			return FALSE;
		}
		if(nID >= ID_FILE_INTERPRET1 && nID <= ID_FILE_INTERPRET_LAST) {
			_tcsncpy(pTTT->szText, theApp.interpreterTooltips[nID-ID_FILE_INTERPRET1], 79);
				return FALSE;
		}
	}
	return TRUE;
}

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;



IMPLEMENT_DYNCREATE(CComponentBar, CMFCToolBar)

BEGIN_MESSAGE_MAP(CComponentBar, CMFCToolBar)
	ON_NOTIFY_EX( TTN_NEEDTEXT, 0, OnTT )
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

CMainFrame *CMainFrame::theInstance = 0;

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TIME, OnUpdateTime)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_EDIT_SEARCH, OnEditSearch)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SEARCH, OnUpdateEditSearch)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_NEW, OnUpdateWindowNew)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	
	ON_COMMAND(ID_VIEW_REFRESH_SOURCECONTROL, OnViewMultiUserRefreshSourceControl)
	ON_UPDATE_COMMAND_UI(ID_VIEW_REFRESH_SOURCECONTROL, OnUpdateViewMultiUserRefreshSourceControl)
	ON_COMMAND(ID_MULTIUSER_ACTIVEUSERS, OnViewMultiUserActiveUsers)
	ON_UPDATE_COMMAND_UI(ID_MULTIUSER_ACTIVEUSERS, OnUpdateViewMultiUserActiveUsers)
	ON_COMMAND(ID_VIEW_CLEARCONSOLE, OnViewClearConsole)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CLEARCONSOLE, OnUpdateViewClearConsole)
	ON_UPDATE_COMMAND_UI(ID_HELP_CONTENTS, OnUpdateFileSettings)

#define MSG_MAP_VIEW_PANE(ID, class_, member) \
	ON_COMMAND(ID, (&CMainFrame::OnViewPane<class_, &CMainFrame::member>)) \
	ON_UPDATE_COMMAND_UI(ID, (&CMainFrame::OnUpdateViewPane<class_, &CMainFrame::member>))
	MSG_MAP_VIEW_PANE(ID_VIEW_GMEBROWSER,		CGMEBrowser,		m_browser)
	MSG_MAP_VIEW_PANE(ID_VIEW_OBJECTINSPECTOR,	CGMEObjectInspector,m_objectInspector)
	MSG_MAP_VIEW_PANE(ID_VIEW_PARTBROWSER,		CGMEPartBrowser,	m_partBrowser)
	MSG_MAP_VIEW_PANE(ID_VIEW_PANNWIN,			CGMEPanningWindow,	m_panningWindow)
	MSG_MAP_VIEW_PANE(ID_VIEW_CONSOLE,			CGMEConsole,		m_console)
	MSG_MAP_VIEW_PANE(ID_VIEW_GMEBROWSER,		CGMEBrowser,		m_browser)
	MSG_MAP_VIEW_PANE(ID_VIEW_SEARCH,			CGMESearch,			m_search)
	ON_WM_CLOSE()
	ON_WM_DROPFILES()
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_AQUA, &CMainFrame::OnUpdateApplicationLook)
	ON_COMMAND(ID_WINDOW_MANAGER, &CMainFrame::OnWindowManager)
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullScreen)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, OnToolbarCreateNew)
//}}AFX_MSG_MAP
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)

	// By making the Menu IDs that same as the ToolBar IDs
	// we can leverage off of code that is already provided
	// in MFCs implementation of CFrameWnd to check, uncheck
	// show and hide toolbars.
	ON_COMMAND_EX(IDW_TOOLBAR_MAIN, OnBarCheck)
	ON_COMMAND_EX(IDW_TOOLBAR_WINS, OnBarCheck)
	ON_COMMAND_EX(IDW_TOOLBAR_COMPONENT, OnBarCheck)
	ON_COMMAND_EX(IDW_TOOLBAR_MODE, OnBarCheck)
	ON_COMMAND_EX(IDW_TOOLBAR_NAVIG, OnBarCheck)
	ON_COMMAND_EX(IDW_TOOLBAR_MODELING, OnBarCheck)
	ON_UPDATE_COMMAND_UI(IDW_TOOLBAR_MAIN, OnUpdateControlBarMenus)
	ON_UPDATE_COMMAND_UI(IDW_TOOLBAR_WINS, OnUpdateControlBarMenus)
	ON_UPDATE_COMMAND_UI(IDW_TOOLBAR_COMPONENT, OnUpdateControlBarMenus)
	ON_UPDATE_COMMAND_UI(IDW_TOOLBAR_MODE, OnUpdateControlBarMenus)
	ON_UPDATE_COMMAND_UI(IDW_TOOLBAR_NAVIG, OnUpdateControlBarMenus)
	ON_UPDATE_COMMAND_UI(IDW_TOOLBAR_MODELING, OnUpdateControlBarMenus)
	ON_COMMAND(ID_BUTTON33020, OnBtnBack)
	ON_COMMAND(ID_BUTTON33022, OnBtnHome)
	ON_UPDATE_COMMAND_UI(ID_BUTTON33020, OnUpdateBtnBack)
	ON_UPDATE_COMMAND_UI(ID_BUTTON33022, OnUpdateBtnHome)
	ON_COMMAND(ID_WINDOW_NEWHORIZONTALTABGROUP, &CMainFrame::OnWindowNewhorizontaltabgroup)
	ON_COMMAND(ID_WINDOW_NEWVERTICALTABGROUP, &CMainFrame::OnWindowNewverticaltabgroup)
	ON_COMMAND(ID_WINDOW_MOVETOPREVIOUSTABGROUP, &CMainFrame::OnWindowMovetoprevioustabgroup)
	ON_COMMAND(ID_WINDOW_MOVETONEXTTABGROUP, &CMainFrame::OnWindowMovetonexttabgroup)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_NEWHORIZONTALTABGROUP, &CMainFrame::OnUpdateWindowNewhorizontaltabgroup)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_NEWVERTICALTABGROUP, &CMainFrame::OnUpdateWindowNewverticaltabgroup)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_MOVETOPREVIOUSTABGROUP, &CMainFrame::OnUpdateWindowMovetoprevioustabgroup)
	ON_UPDATE_COMMAND_UI(ID_WINDOW_MOVETONEXTTABGROUP, &CMainFrame::OnUpdateWindowMovetonexttabgroup)
	ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, &CMainFrame::OnGetTabTooltip)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

class CGMEMDIClientAreaWnd : public CMDIClientAreaWnd
{
	virtual CMFCTabCtrl* CreateTabGroup(CMFCTabCtrl* pWndTab)
	{
		pWndTab = new CGMEMFCTabCtrl;
		return __super::CreateTabGroup(pWndTab);
	}
};

CMainFrame::CMainFrame()
{
	theInstance = this;	
	m_autosaveTimerID = NULL;
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2005);
#ifdef new
#pragma push_macro("new")
#undef new
	new ((void*)&m_wndClientArea) CGMEMDIClientAreaWnd();
#pragma pop_macro("new")
#else
	new ((void*)&m_wndClientArea) CGMEMDIClientAreaWnd();
#endif
}

CMainFrame::~CMainFrame()
{
	theInstance = 0;	
}

void CMainFrame::setGmeOleApp( CGMEOLEApp * the_ole_app)
{
	mGmeOleApp = the_ole_app;

	IDispatch *disp;							// REFCOUNT is initially 1 ( ONE )

	disp = mGmeOleApp->GetIDispatch(FALSE);	// do not increment the refcount

	m_console.SetGMEApp(disp); // increments the refcount to 2 (since it stores the value in a smart pointer)

	// decrement the refcount, relying on the other components maintaining the correct refcounts
	//this line is needed either in setGmeOleApp or here
	// ULONG ul = mGmeOleApp->GetIDispatch(FALSE)->Release();
	//ul = mGmeOleApp->GetIDispatch(FALSE)->Release(); //decrement the refcount to 1

	// later when a project is opened the "mGmeOleApp" will be registered as a client of the "MgaProject", thus
	// it will be incremented once again the refcount
	
	// when the project is closed, then the CGMEApp::CloseProject will erase this reference to the "mGmeOleApp"  with a  "mgaClient = NULL;" call

	// so the Mga component is referencing the GmeOleApp by its MgaProject's client member
	//        Console compt is referencing the GmeOleApp by its member
	//
	// beware that when the main GME would like to destruct the Console upon exiting
	// then the Console has to be the last referring to the GmeOleApp, so that it 
	// can be released
}

void CMainFrame::setMgaProj()
{
	IDispatch *disp;

	disp = mGmeOleApp->GetIDispatch(FALSE);	// do not increment the refcount

	m_console.SetGMEProj(disp);
}

void CMainFrame::clearGmeOleApp()
{
	m_console.SetGMEProj( 0 );
	m_console.SetGMEApp( 0 );

	//this line is needed either in setGmeOleApp or here
	mGmeOleApp->GetIDispatch(FALSE)->Release();

	// make sure
	mGmeOleApp = 0;
}

void CMainFrame::clearMgaProj()
{
	m_console.SetGMEProj( 0 );
}


void CMainFrame::OnClose()
{
	if (theApp.SaveAllModified())
	{
#ifdef _DEBUG
		clearGmeOleApp();
		CMDIFrameWndEx::OnClose();
#else
		// n.b. this saves pane positions
		CMDIFrameWndEx::OnClose();
		// n.b. C# interpreters may not Release() IGMEOLEApp, which keeps us ::Run()ing forever
		// TerminateProcess will be unpleasant for DCOM (but the user asked for it)
		TerminateProcess(GetCurrentProcess(), 0);
#endif
	}
}

int CMainFrame::CreateToolBars()
{
	
	// 3 separate TOOLBARs are created
	//#define AFX_IDW_CONTROLBAR_FIRST        0xE800 = 59392
	//#define AFX_IDW_CONTROLBAR_LAST         0xE8FF
	//
	//#define AFX_IDW_TOOLBAR                 0xE800  // main Toolbar for window
	//#define AFX_IDW_STATUS_BAR              0xE801  // Status bar window
	//#define AFX_IDW_PREVIEW_BAR             0xE802  // PrintPreview Dialog Bar
	//#define AFX_IDW_RESIZE_BAR              0xE803  // OLE in-place resize bar
	//#define AFX_IDW_REBAR                   0xE804  // COMCTL32 "rebar" Bar
	//#define AFX_IDW_DIALOGBAR               0xE805  // CDialogBar
	//#define AFX_IDW_DOCKBAR_TOP             0xE81B
	//#define AFX_IDW_DOCKBAR_LEFT            0xE81C
	//#define AFX_IDW_DOCKBAR_RIGHT           0xE81D
	//#define AFX_IDW_DOCKBAR_BOTTOM          0xE81E
	//#define AFX_IDW_DOCKBAR_FLOAT           0xE81F = 59423

	// thus the IDW_TOOLBAR_* ids conform to these limits
	// being assigned numbers from 0xE820


	// Checking resolution
	CWindowDC dc(NULL);
	bool bHiColorIcons = dc.GetDeviceCaps(BITSPIXEL) >= 16;


	// -- MAIN ToolBar
	if( !m_wndToolBarMain.CreateEx( this
		, TBSTYLE_FLAT
		, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP
		, CRect(0,0,0,0)
		, IDW_TOOLBAR_MAIN) // provide unqiue ID for each toolbar [important !!!]
		||
		!m_wndToolBarMain.LoadToolBar(IDR_TOOLBAR_MAIN, 0, 0, FALSE, 0, 0, bHiColorIcons ? IDB_MAIN_TOOLBAR24 : 0)
		)
	{
		TRACE(_T("Failed to create main toolbar\n"));
		return -1;      // fail to create
	}

	m_wndToolBarMain.SetPaneStyle(m_wndToolBarMain.GetPaneStyle()
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndToolBarMain.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, _T("Customize..."));

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, _T("Customize..."), ID_VIEW_TOOLBAR, FALSE, TRUE);


	// -- Modeling ToolBar
	if( !m_wndToolBarModeling.CreateEx( this
		, TBSTYLE_FLAT
		, WS_CHILD |  WS_VISIBLE | CBRS_ALIGN_TOP
		, CRect(0,0,0,0)
		, IDW_TOOLBAR_MODELING) // provide unqiue ID for each toolbar [important !!!]
		||
		!m_wndToolBarModeling.LoadToolBar(IDR_TOOLBAR_MODELING, 0, 0, FALSE, 0, 0, bHiColorIcons ? IDB_MODELING_TOOLBAR24 : 0)
		)
	{
		TRACE(_T("Failed to create modeling toolbar\n"));
		return -1;      // fail to create
	}

	m_wndToolBarModeling.SetPaneStyle( m_wndToolBarModeling.GetPaneStyle()
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndToolBarModeling.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, _T("Customize..."));

	// -- Window Arrangement ToolBar
	if( !m_wndToolBarWins.CreateEx( this
		, TBSTYLE_FLAT
		, WS_CHILD |  WS_VISIBLE | CBRS_ALIGN_TOP
		, CRect(0,0,0,0)
		, IDW_TOOLBAR_WINS) // provide unqiue ID for each toolbar
		||
		!m_wndToolBarWins.LoadToolBar(IDR_TOOLBAR_WINS, 0, 0, FALSE, 0, 0, bHiColorIcons ? IDB_WINS_TOOLBAR24 : 0)
		)
	{
		TRACE0("Failed to create windows toolbar\n");
		return -1;
	}

	m_wndToolBarWins.SetPaneStyle( m_wndToolBarWins.GetPaneStyle()
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndToolBarWins.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, _T("Customize..."));

	// -- User-defined Component ToolBar
	if( !m_wndComponentBar.CreateEx( this
		, TBSTYLE_FLAT
		, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP
		, CRect(0, 0, 0, 0)
		, IDW_TOOLBAR_COMPONENT) // provide unqiue ID for each toolbar
		||
		!m_wndComponentBar.LoadToolBar(IDR_TOOLBAR_COMPONENTS, 0, 0, TRUE, 0, 0, bHiColorIcons ? IDB_COMPONENTS_TOOLBAR24 : 0)
		)
	{
		TRACE0("Failed to create component toolbar\n");
		return -1;      // fail to create
	}
	m_wndComponentBar.SetPaneStyle(m_wndComponentBar.GetPaneStyle()
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);


	// Mode toolbar

	if( !m_wndModeBar.CreateEx( this
		, TBSTYLE_FLAT
		, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP
		, CRect(0, 0, 0, 0)
		, IDW_TOOLBAR_MODE)	// provide unqiue ID for each toolbar [important !!!] 
							// see MainFrm.cpp OnCreate for other details
		||
		!m_wndModeBar.LoadToolBar(IDR_TOOLBAR_MODE, 0, 0, FALSE, 0, 0, bHiColorIcons ? IDB_MODE_TOOLBAR24 : 0)
		)
	{
		TRACE0("Failed to create mode toolbar\n");
		return -1;      // fail to create
	}
	m_wndModeBar.SetPaneStyle(m_wndModeBar.GetPaneStyle() 
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);


	// Navigation toolbar
	if( !m_wndNaviBar.CreateEx( this
		, TBSTYLE_FLAT
		, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP
		, CRect(0, 0, 0, 0)
		, IDW_TOOLBAR_NAVIG) // unique!
		||
		!m_wndNaviBar.LoadToolBar(IDR_TOOLBAR_NAVIG, 0, 0, FALSE, 0, 0, bHiColorIcons ? IDB_NAVIG_TOOLBAR24 : 0)
		)
	{
		TRACE0("Failed to create m_wndNaviBar toolbar\n");
		return -1;      // fail to create
	}
	m_wndNaviBar.SetPaneStyle(m_wndNaviBar.GetPaneStyle()
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);



	// --- Docking ---
	// Toolbars are dockable to any side of the frame
	m_wndToolBarMain.SetWindowText(_T("Standard"));
	m_wndToolBarMain.EnableDocking(CBRS_ALIGN_ANY);

	m_wndToolBarModeling.SetWindowText(_T("Modeling"));
	m_wndToolBarModeling.EnableDocking(CBRS_ALIGN_ANY);

	m_wndToolBarWins.SetWindowText(_T("Windows"));
	m_wndToolBarWins.EnableDocking(CBRS_ALIGN_ANY);

	m_wndComponentBar.SetWindowText(_T("Components"));
	m_wndComponentBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndComponentBar.AdjustSizeImmediate(TRUE);

	m_wndModeBar.SetWindowText(_T("Mode")); // will show this title when floating
	m_wndModeBar.EnableDocking(CBRS_ALIGN_ANY);

	m_wndNaviBar.SetWindowText(_T("Navigator")); // will show this title when floating
	m_wndNaviBar.EnableDocking(CBRS_ALIGN_ANY);

	// Because of "DockPaneLeftOf" we dock them in "reverse" order: the rightmost first
	DockPane(&m_wndComponentBar, AFX_IDW_DOCKBAR_TOP);
	DockPaneLeftOf(&m_wndToolBarWins, &m_wndComponentBar);
	DockPaneLeftOf(&m_wndToolBarModeling, &m_wndToolBarWins);
	DockPaneLeftOf(&m_wndToolBarMain, &m_wndToolBarModeling);

	DockPane(&m_wndNaviBar, AFX_IDW_DOCKBAR_LEFT);
	DockPaneLeftOf(&m_wndModeBar, &m_wndNaviBar);	// This means actually "TopOf" instead of "LeftOf", because they are vetical

	// Hide navigation and mode panels first, they are visible when a model is open
	ShowNavigationAndModeToolbars(false);

	// Allow user-defined toolbars operations:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

	return 0;
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// Creating tabs for the MDI Children (documents)
	CMDITabInfo mdiTabParams;
	mdiTabParams.m_style = CMFCTabCtrl::STYLE_3D_VS2005;	// other styles available...
	mdiTabParams.m_bActiveTabCloseButton = TRUE;			// set to FALSE to place close button at right of tab area
	mdiTabParams.m_bTabIcons = FALSE;						// set to TRUE to enable document icons on MDI tabs
	mdiTabParams.m_bAutoColor = TRUE;						// set to FALSE to disable auto-coloring of MDI tabs
	mdiTabParams.m_bDocumentMenu = TRUE;					// enable the document menu at the right edge of the tab area
	mdiTabParams.m_bEnableTabSwap = TRUE;					// enable the user to change the tabs positions by dragging the tabs
	mdiTabParams.m_bFlatFrame = TRUE;						// give each tab window has a flat frame
	mdiTabParams.m_bTabCloseButton = FALSE;					// enable each tab window to display the Close button on the right edge of the tab.
	mdiTabParams.m_bTabCustomTooltips = TRUE;				// enable the tabs to display tooltips.
	mdiTabParams.m_tabLocation = CMFCTabCtrl::LOCATION_TOP;	// Specifies that the tabs labels are located at the top of the page
	EnableMDITabbedGroups(TRUE, mdiTabParams);

 	CMFCToolBar::EnableQuickCustomization();

	// Set the visual manager and style based on persisted value
	OnApplicationLook(theApp.m_nAppLook);

	EnableDocking(CBRS_ALIGN_ANY);



	// STATUS BAR
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;		// fail to create
	}

	CMFCToolBar::EnableQuickCustomization();

	// MENU BAR
	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;		// fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_TOP);
	DockPane(&m_wndMenuBar, AFX_IDW_DOCKBAR_TOP);
	
	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	// TOOLBARS
	if(CreateToolBars())
	{
		return -1;
	}


	// Enable enhanced windows management dialog
	EnableWindowsDialog(ID_WINDOW_MANAGER, _T("Model windows..."), TRUE);




	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	CMDIFrameWndEx::EnableMDITabsLastActiveActivation(TRUE);

	// enable Visual Studio 2005 style docking window auto-hide behavior
//	EnableAutoHidePanes(CBRS_ALIGN_ANY);



	// PART BROWSER
	if (!m_partBrowser.Create(_T("Part Browser"), this, CSize(240,480),
		TRUE, ID_PARTBROWSER, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI))
	{
			TRACE0("Failed to create part browser\n");
			return -1;	// fail to create
	}
	m_partBrowser.EnableDocking(CBRS_ALIGN_ANY);
	int captionHeight = m_partBrowser.GetCaptionHeight();
	m_partBrowser.SetMinSize(CSize(captionHeight, captionHeight));

	// PANNING WINDOW
	if (!m_panningWindow.Create(_T("Panning Window"), this, CSize(240, 160),
		TRUE, ID_PANNWIN, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create panning Window\n");
		return -1;		// fail to create
	}	
	m_panningWindow.EnableDocking(CBRS_ALIGN_ANY);
	m_panningWindow.SetMinSize(CSize(captionHeight, captionHeight));

	// GME ACTIVE BROWSER TREE
	if (!m_browser.Create(_T("GME Browser"), this, CSize(240, 80),
		TRUE, ID_GMEBROWSER, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create browser\n");
		return -1;		// fail to create
	}
	m_browser.EnableDocking(CBRS_ALIGN_ANY);
	m_browser.SetMinSize(CSize(captionHeight, captionHeight));

	// OBJECT INSPECTOR
	if(!m_objectInspector.Create(_T("Object Inspector"), this, CSize(240, 160),
		TRUE, IDD_OBJECT_INSPECTOR_DIALOG, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Object Inspector\n");
		return -1;		// fail to create
	}
	m_objectInspector.EnableDocking(CBRS_ALIGN_ANY);
	m_objectInspector.SetMinSize(CSize(captionHeight, captionHeight));

	// CONSOLE
	if(!m_console.Create(_T("Console"), this, CSize(80, 160),
		TRUE, IDD_CONSOLE_DIALOG, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Console\n");
		return -1;		// fail to create
	}
	m_console.EnableDocking(CBRS_ALIGN_ANY);
	m_console.SetMinSize(CSize(captionHeight, captionHeight));

	// SEARCH - Modal Dialog
	if(!m_search.Create(_T("Search"), this, CSize(200, 200),
		TRUE, IDD_SEARCH_DIALOG, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		TRACE0("Failed to create Search Control\n");
		return -1;		// fail to create
	}
	m_search.EnableDocking(CBRS_ALIGN_ANY);
	m_search.SetMinSize(CSize(captionHeight, captionHeight));


	// --- Docking ---

	DockPane(&m_browser, AFX_IDW_DOCKBAR_RIGHT);

	DockPane(&m_partBrowser, AFX_IDW_DOCKBAR_LEFT);

	m_panningWindow.DockToWindow(&m_partBrowser, CBRS_ALIGN_BOTTOM);

	m_objectInspector.DockToWindow(&m_browser,CBRS_ALIGN_BOTTOM);

	DockPane(&m_console,AFX_IDW_DOCKBAR_BOTTOM);

	CDockablePane* pTabbedBar = NULL;
	m_search.AttachToTabWnd(&m_console, DM_SHOW, TRUE, &pTabbedBar);

	m_search.ShowPane(FALSE, FALSE, FALSE);	


	// CG: The following block was inserted by 'Status Bar' component.
	{
		// Find out the size of the static variable 'indicators' defined
		// by AppWizard and copy it
		int nOrigSize = sizeof(indicators) / sizeof(UINT);

		UINT pIndicators[16];
		memcpy(pIndicators, indicators, sizeof(indicators));

		// Call the Status Bar Component's status bar creation function
		if (!InitStatusBar(pIndicators, nOrigSize, 60))
		{
			TRACE0("Failed to initialize Status Bar\n");
			return -1;
		}
	}

	// hide initially, openProject will show it if components available
	// thus we ignore the registry settings
	m_wndComponentBar.ShowWindow(SW_HIDE);
	ShowPane(&m_wndComponentBar, FALSE, FALSE, FALSE);


	// CG: The following line was added by the Splash Screen component.
	CSplashWnd::ShowSplashScreen(this);

	// Adding full screen functionality
	EnableFullScreenMode(ID_VIEW_FULLSCREEN);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWndEx::Dump(dc);
}

#endif //_DEBUG

void CMainFrame::UpdateTitle(LPCTSTR title)
{
	UpdateFrameTitleForDocument(title);
}

void CMainFrame::DestroyView(CView *view)
{
	ASSERT(view);
	CFrameWnd *frame = view->GetParentFrame();
	ASSERT(frame);
	frame->DestroyWindow();
}

void CMainFrame::ActivateView(CView *view)
{
	ActivateFrame();
	ASSERT(view);
	CFrameWnd *frame = view->GetParentFrame();
	ASSERT(frame);
	CFrameWnd *activeFrame = GetActiveFrame();
	ASSERT(activeFrame);
	if(frame != activeFrame)
		frame->ActivateFrame(frame->IsIconic() ? SW_SHOWNORMAL : -1);
}

struct AccessProtectedKludge : public CMDIClientAreaWnd 
{
	CObList& get()
	{
		return m_lstRemovedTabbedGroups;
	}
	static CObList& get(CMDIClientAreaWnd& clientarea)
	{
		return ((AccessProtectedKludge*)(&clientarea))->get();
	}
};

void CMainFrame::CreateNewView(CView *view, CComPtr<IMgaModel>& model)
{
	CMultiDocTemplate *docTemplate = theApp.pDocTemplate;
	if(view != 0) {
		ActivateView(view);
	}
	else {
		CDocument *pDocument = CGMEDoc::theInstance;
		ASSERT( pDocument );

		// KMS: WM_MDIACTIVATE is sent 3 times for tabs after the first: once for the new CChildFrame, once for the old,
		// and again for the new. Disable panning window refresh, since it can be expensive.
		CGMEView* oldGmeview = CGMEView::GetActiveView();
		if (oldGmeview)
			oldGmeview->m_bEnablePannWindowRefresh = false;
		CFrameWnd *pFrame = docTemplate->CreateNewFrame(pDocument, NULL);
		if(pFrame == NULL) {
			AfxMessageBox(_T("Failed to create window"),MB_OK | MB_ICONSTOP);
			return;
		}
		docTemplate->InitialUpdateFrame(pFrame,pDocument);
		HWND hwndActive = (HWND) m_wndClientArea.SendMessage(WM_MDIGETACTIVE);

		m_wndClientArea.UpdateMDITabbedGroups(FALSE);  // The framework by default calls this via OnUpdateFrameTitle 
		                                              // (overloaded in our implementation without calling the base class intentionally)

		// n.b. m_wndClientArea is long-lived, but puts closed tab groups in .m_lstRemovedTabbedGroups
		// delete them here to prevent a resource leak
		// FIXME: this should run when the last tab is closed
		CObList& m_lstRemovedTabbedGroups = AccessProtectedKludge::get(m_wndClientArea);
		while (!m_lstRemovedTabbedGroups.IsEmpty())
		{
			CMFCTabCtrl* pWnd= DYNAMIC_DOWNCAST(CMFCTabCtrl, m_lstRemovedTabbedGroups.RemoveTail());
			if (pWnd != NULL)
			{
				pWnd->DestroyWindow();
				delete pWnd;
			}
		}

		CGMEView* newGmeview = CGMEView::GetActiveView();

		if (oldGmeview)
		{
			oldGmeview->m_bEnablePannWindowRefresh = true;
		}
		newGmeview->m_bEnablePannWindowRefresh = true;
		newGmeview->DoPannWinRefresh();

#if defined(ACTIVEXGMEVIEW)
		CMainFrame* pMainFrame = (CMainFrame*)theApp.m_pMainWnd;
		// Get the active MDI child window.
		CMDIChildWnd* pChild = (CMDIChildWnd*)pMainFrame->GetActiveFrame();
		// or CMDIChildWnd *pChild = pFrame->MDIGetActive();
		// Get the active view attached to the active MDI child window.
		CGMEChildFrame* pView = (CGMEChildFrame*)pChild->GetActiveView();
		pView->SetProject(theApp.mgaProject);

		m_partBrowser.SetProject(theApp.mgaProject);
		try {
			long status;
			COMTHROW(theApp.mgaProject->get_ProjectStatus(&status));
			bool inTrans = (status & 0x08L) != 0;
			CComPtr<IMgaTerritory> terr;
			if (!inTrans) {
				COMTHROW(theApp.mgaProject->CreateTerritory(NULL, &terr));
				COMTHROW(theApp.mgaProject->BeginTransaction(terr, TRANSACTION_READ_ONLY));
			} else {
				COMTHROW(theApp.mgaProject->get_ActiveTerritory(&terr));
			}

			CComPtr<IMgaFCO> modelFco;
			COMTHROW(terr->OpenFCO(model, &modelFco));
			CComQIPtr<IMgaModel> model2 = modelFco;

			status = OBJECT_ZOMBIE;
			COMTHROW(model2->get_Status(&status));
			if (status == OBJECT_EXISTS) {
				CComPtr<IMgaMetaFCO> ccpMetaFCO;
				COMTHROW(model2->get_Meta(&ccpMetaFCO));
				CComQIPtr<IMgaMetaModel> metaModel = ccpMetaFCO;
				metaref_type metaid;
				COMTHROW(metaModel->get_MetaRef(&metaid));
				CGuiMetaModel* guiMeta = CGuiMetaProject::theInstance->GetGuiMetaModel(metaid);
				m_partBrowser.SetMetaModel(guiMeta);
				pView->SetMetaModel(guiMeta);
				pView->ChangeAspect(0);
				m_partBrowser.SetBgColor(RGB(0xFF, 0xFF, 0xFF));
				m_partBrowser.ChangeAspect(0);
				m_partBrowser.RePaint();
			}

			if (!inTrans) {
				theApp.mgaProject->CommitTransaction();
			}
		} catch(...) {
			ASSERT(0);
		}
		pView->SetModel(model);
#endif
	}
}

void CMainFrame::WriteStatusMode(const CString& txt)
{
	WriteStatusText(modePaneNo,txt);
}

void CMainFrame::WriteStatusParadigm(const CString& txt)
{
	WriteStatusText(paradigmPaneNo,txt);
}

void CMainFrame::WriteStatusZoom(int zoomPct)
{
	CString txt;
	txt.Format(_T("%d%%"), zoomPct);
	WriteStatusText(zoomPaneNo,txt);
}

void CMainFrame::WriteStatusText(int pane, const CString& txt)
{
	CSize size;
	{
		HGDIOBJ hOldFont = NULL;
		HFONT hFont = (HFONT)m_wndStatusBar.SendMessage(WM_GETFONT);
		CClientDC dc(NULL);
		if (hFont != NULL) 
			hOldFont = dc.SelectObject(hFont);
		size = dc.GetTextExtent(txt);
		if (hOldFont != NULL) 
			dc.SelectObject(hOldFont);
	}
	
	int lng;
	UINT nID,nStyle;
	m_wndStatusBar.GetPaneInfo(pane, nID, nStyle, lng);
	m_wndStatusBar.SetPaneInfo(pane, nID, nStyle, size.cx);
    m_wndStatusBar.SetPaneText(pane,txt);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


void CMainFrame::OnUpdateTime(CCmdUI* /*pCmdUI*/)
{
	// CG: This function was inserted by 'Status Bar' component.

	// Get current date and format it
	CTime time = CTime::GetCurrentTime();
	CString strTime = time.Format(_T("%I:%M  "));
	strTime = (time.GetHour() < 12 ? _T("") : _T(""))+ strTime +(time.GetHour() < 12 ? _T("AM "):_T("PM "));

	WriteStatusText(timePaneNo,strTime);
}

BOOL CMainFrame::InitStatusBar(UINT *pIndicators, int nSize, int nSeconds)
{
	modePaneNo = nSize++;
	pIndicators[modePaneNo] = ID_INDICATOR_MODE;
	zoomPaneNo = nSize++;
	pIndicators[zoomPaneNo] = ID_INDICATOR_ZOOM;
	paradigmPaneNo = nSize++;
	pIndicators[paradigmPaneNo] = ID_INDICATOR_PARADIGM;
	timePaneNo = nSize++;
	pIndicators[timePaneNo] = ID_INDICATOR_TIME;

	nSeconds = 10;
	m_wndStatusBar.SetTimer(0x1000, nSeconds * 1000, NULL);

	return m_wndStatusBar.SetIndicators(pIndicators, nSize);
}


void CMainFrame::OnEditSearch()
{
	m_search.ShowPane(TRUE, FALSE, TRUE);
/*	CRect searchWindowRect;
	m_search.GetWindowRect(&searchWindowRect);
	if (m_search.CanBeResized() && searchWindowRect.Height() <= searchWindowRect.GetCaptionHeight()) {
		ScreenToClient(&searchWindowRect);
		m_search.SetWindowPos(NULL, searchWindowRect.left, searchWindowRect.top, searchWindowRect.Width(), searchWindowRect.Height() + 100, SWP_NOZORDER | SWP_NOMOVE);
	}*/

	CComPtr<IMgaObjects> objs;
	if( CGMEBrowser::theInstance->GetSelectedItems( objs))
		m_search.SetSelMgaObjects( objs);
	m_search.SetFocus(); // allows for easy 'Find Next' feature
	// the user might press ^F whenever she'd like to jump
	// to a focused searchbox
}

void CMainFrame::OnUpdateEditSearch(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(
		(CGMEDoc::theInstance != 0) &&
		m_search &&
		1 //(!m_search.IsWindowVisible()) 
		// from now on will allow multiple ^F-s, and as a result it 
		// will set focus back to search window (like in VStudio).
		// allows for easy 'Find Next'-like functionality
		);
}


void CMainFrame::SetGMEViewMetaModel(CGuiMetaModel* meta)
{
	CDocument* pDocument = CGMEDoc::theInstance;
	POSITION pos = pDocument->GetFirstViewPosition();
	if (pos) {
		while (pos != NULL) {
#if defined(ACTIVEXGMEVIEW)
			CGMEChildFrame* pView = (CGMEChildFrame*) pDocument->GetNextView(pos);
			pView->SetMetaModel(meta);
#else
			break;
#endif
		}
	}
}

void CMainFrame::ChangeGMEViewAspect(int ind)
{
	CDocument* pDocument = CGMEDoc::theInstance;
	POSITION pos = pDocument->GetFirstViewPosition();
	if (pos) {
		while (pos != NULL) {
#if defined(ACTIVEXGMEVIEW)
			CGMEChildFrame* pView = (CGMEChildFrame*) pDocument->GetNextView(pos);
			pView->ChangeAspect(ind);
#else
			break;
#endif
		}
	}
}

void CMainFrame::CycleGMEViewAspect()
{
	CDocument* pDocument = CGMEDoc::theInstance;
	POSITION pos = pDocument->GetFirstViewPosition();
	if (pos) {
		while (pos != NULL) {
#if defined(ACTIVEXGMEVIEW)
			CGMEChildFrame* pView = (CGMEChildFrame*) pDocument->GetNextView(pos);
			pView->CycleAspect();
#else
			break;
#endif
		}
	}
}

void CMainFrame::GMEViewInvalidate()
{
	CDocument* pDocument = CGMEDoc::theInstance;
	POSITION pos = pDocument->GetFirstViewPosition();
	if (pos) {
		while (pos != NULL) {
#if defined(ACTIVEXGMEVIEW)
			CGMEChildFrame* pView = (CGMEChildFrame*) pDocument->GetNextView(pos);
			pView->Invalidate();
#endif
		}
	}
}


// This function is Copyright (c) 2000, Cristi Posea.
// See www.datamekanix.com for more control bars tips&tricks.
BOOL CMainFrame::VerifyBarState(LPCTSTR lpszProfileName)
{
    CDockState state;
    state.LoadState(lpszProfileName);

    for (int i = 0; i < state.m_arrBarInfo.GetSize(); i++)
    {
        CControlBarInfo* pInfo = (CControlBarInfo*)state.m_arrBarInfo[i];
        ASSERT(pInfo != NULL);
        int nDockedCount = pInfo->m_arrBarID.GetSize();
        if (nDockedCount > 0)
        {
            // dockbar
            for (int j = 0; j < nDockedCount; j++)
            {
                UINT nID = (UINT) pInfo->m_arrBarID[j];
                if (nID == 0) continue; // row separator
                if (nID > 0xFFFF)
                    nID &= 0xFFFF; // placeholder - get the ID
                if (GetControlBar(nID) == NULL)
                    return FALSE;
            }
        }
        
        if (!pInfo->m_bFloating) // floating dockbars can be created later
            if (GetControlBar(pInfo->m_nBarID) == NULL)
                return FALSE; // invalid bar ID
    }

    return TRUE;
}

BOOL CMainFrame::DestroyWindow()
{
	return CMDIFrameWndEx::DestroyWindow();
}

void CMainFrame::ShowObjectInspector()
{
	ShowPane(&m_objectInspector, TRUE, FALSE, TRUE);
}

void CMainFrame::ShowFindDlg()
{
	OnEditSearch();
}

void CMainFrame::HideFindDlg()
{
	m_search.SetSelMgaObjects( CComPtr<IMgaObjects>( 0));
	m_search.ShowWindow( SW_HIDE);
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == GME_AUTOSAVE_EVENT_ID) {
		theApp.Autosave();
	}
	
	CMDIFrameWndEx::OnTimer(nIDEvent);
}


void CMainFrame::StartAutosaveTimer(int secs)
{
	StopAutosaveTimer();
	ASSERT(m_autosaveTimerID == NULL);

	m_autosaveTimerID = SetTimer(GME_AUTOSAVE_EVENT_ID, 1000 * secs, NULL);
}

void CMainFrame::StopAutosaveTimer()
{
	if (m_autosaveTimerID) {
		KillTimer(m_autosaveTimerID);
		m_autosaveTimerID = NULL;
	}	
}



void CMainFrame::OnDestroy() 
{
	CMDIFrameWndEx::OnDestroy();
	
	StopAutosaveTimer();
}


/*
IMgaObject*	CMainFrame::folderFindByPath(IMgaFolder* folder, const wstring& strPath)
{
	wstring strPath2;
	wstring strName;
	wstring::size_type iPos = strPath.find( L"/" );
	if ( iPos == wstring::npos ) 
	{
		strName = strPath;
		strPath2 = L"";
	}
	else {
		if ( iPos == 0 )
			strPath2 = strPath.substr( iPos + 1);
		iPos = strPath2.find( L"/" );
		if ( iPos == wstring::npos ) 
		{
			strName = strPath2;
			strPath2 = L"";
*		}
		else 
		{
			strName = strPath2.substr( 0, iPos );
			strPath2 = strPath2.substr( iPos );
		}
	}
	COMTRY {
		CComPtr<IMgaFolder> root;
		if (!folder)
		{
			COMTHROW(theApp.mgaProject->get_RootFolder(&root));
			if (!root)
				return NULL;
			CComBSTR strname;
			COMTHROW(root->get_Name(&strname));
			if ( strName == (wstring)(WCHAR*)(BSTR)strname) 
			{
				if ( strPath2.empty() )
				{
					CComPtr<IMgaObject> obj;
					root.QueryInterface(&obj);
					return obj.Detach();
				}
				else 
				{
					IMgaObject* o = folderFindByPath(root,  strPath2);
					if (o)
						return o;
				}
			}
			return NULL;
		}

		CComPtr<IMgaObjects> iobjects;
		COMTHROW(folder->get_ChildObjects(&iobjects));
		CComPtr<IMgaObject> item;
		long count = 0;
		COMTHROW(iobjects->get_Count(&count));
		for (int i=1; i<=count; i++)
		{
			COMTHROW(iobjects->get_Item(i, &item));
			CComBSTR strname;
			COMTHROW(item->get_Name(&strname));
			if ( strName == (wstring)(WCHAR*)(BSTR)strname) 
			{
				if ( strPath2.empty() )
					return item.Detach();
				else 
				{
					CComPtr<IMgaModel> model;
					CComPtr<IMgaFolder> folder;
					item.QueryInterface(&model);
					item.QueryInterface(&folder);
					if ( model) 
					{
						IMgaObject *o = modelFindByPath(model,  strPath2);
						if (o)
							return o;
					}
					if ( folder) 
					{
						IMgaObject* o = folderFindByPath(folder,  strPath2);
						if (o)
							return o;
					}
				}
			}
			item.Release();
		}
	} 
	catch(hresult_exception &e) 
	{ 
		// SetErrorInfo(e.hr); 
		return NULL; 
	} 

	return NULL;
}


IMgaObject*	CMainFrame::modelFindByPath(IMgaModel *model,  const wstring& strPath)
{
	wstring strPath2;
	wstring strName;
	wstring::size_type iPos = strPath.find( L"/" );
	if ( iPos == wstring::npos ) 
	{
		strName = strPath;
		strPath2 = L"";
	}
	else 
	{
		if ( iPos == 0 )
			strPath2 = strPath.substr( iPos + 1);
		iPos = strPath2.find( L"/" );
		if ( iPos == wstring::npos ) 
		{
			strName = strPath2;
			strPath2 = L"";
		}
		else 
		{
			strName = strPath2.substr( 0, iPos );
			strPath2 = strPath2.substr( iPos );
		}
	}

	COMTRY {
		CComPtr<IMgaFCOs> fcos;
		COMTHROW(model->get_ChildFCOs(&fcos));
		CComPtr<IMgaFCO> item;
		long count = 0;
		COMTHROW(fcos->get_Count(&count));
		for (int i=1; i<=count; i++)
		{
			COMTHROW(fcos->get_Item(i, &item));
			CComBSTR strname;
			COMTHROW(item->get_Name(&strname));
			if ( strName == (wstring)(WCHAR*)(BSTR)strname) 
			{
				if ( strPath2.empty() )
					return item.Detach();
				else 
				{
					CComPtr<IMgaModel> model;
					item.QueryInterface(&model);
					if (model )
					{
						IMgaObject* o = modelFindByPath(model,  strPath2);
						if (o)
							return o;
					}

				}
			}
			item.Release();
		}
	} 
	catch(hresult_exception &e) 
	{ 
		// SetErrorInfo(e.hr); 
		return NULL; 
	} 

	return NULL;
}
*/



void CMainFrame::OnViewMultiUserRefreshSourceControl() 
{
	BeginWaitCursor();
	try
	{
		theApp.mgaProject->UpdateSourceControlInfo( 0);
		m_browser.RefreshAll();
	}
	catch(...)
	{
	}  
	EndWaitCursor();
}

void CMainFrame::OnUpdateViewMultiUserRefreshSourceControl(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( theApp.isMultiUserProj() && CGMEDoc::theInstance != 0);
}

void CMainFrame::OnViewMultiUserActiveUsers() 
{
	BeginWaitCursor();
	try
	{
		theApp.mgaProject->SourceControlActiveUsers();
	}
	catch(...) { ASSERT(0); }
	EndWaitCursor();
}

void CMainFrame::OnUpdateViewMultiUserActiveUsers(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(theApp.mgaProject && (!CGMEDoc::theInstance || !theApp.isMgaProj())); // always TRUE (also when no project is open) except when mga_proj is open
}

void CMainFrame::OnUpdateViewMultiUserSubversion(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(theApp.mgaProject && (!CGMEDoc::theInstance || !theApp.isMgaProj())); // always TRUE (also when no project is open) except when mga_proj is open
}

void CMainFrame::OnViewClearConsole() 
{
	m_console.Clear();
}

void CMainFrame::OnUpdateViewClearConsole(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
}

void CMainFrame::OnUpdateFileSettings(CCmdUI* pCmdUI)
{
	// This is an update message for Tools/Options menu
	// This triggers the refresh of plugins/interpreters menus
	theApp.UpdateDynMenus(pCmdUI->m_pMenu);
}

void CMainFrame::OnDropFiles(HDROP p_hDropInfo)
{
	CGMEEventLogger::LogGMEEvent(_T("MainFrame:OnDropFiles\r\n"));
	
	// get the number of files dropped
	UINT nFiles = DragQueryFile( p_hDropInfo, 0xFFFFFFFF, NULL, 0);
	if( nFiles < 1)
	{
		CGMEEventLogger::LogGMEEvent(_T("Can't inquire file information!\r\n"));
		m_console.Message( _T("No file dropped or can't inquire file information!"), 3);
	}

	bool one_just_opened = false; // we opened/imported one project just now -> disables opening of more .mga files
	for( UINT iF = 0; iF < nFiles; ++iF)
	{
		TCHAR szFileName[_MAX_PATH];
		UINT res = DragQueryFile( p_hDropInfo, iF, szFileName, _MAX_PATH);
		if (res > 0)
		{
			bool is_dir = false; 
			struct _stat fstatus;
			if( 0 == _tstat( szFileName, &fstatus))
				is_dir = (fstatus.st_mode & _S_IFDIR) == _S_IFDIR;
			
			CString conn( szFileName);
			if( is_dir || conn.Right(4).CompareNoCase(_T(".mga")) == 0 || conn.Right(4).CompareNoCase(_T(".mgx")) == 0)
			{
				if( one_just_opened)
					m_console.Message( _T("Project already open. No other MGA file can be dropped!"), 3);
				else if( theApp.guiMetaProject == NULL && theApp.mgaProject == 0)
				{
					if( conn.Right(4).CompareNoCase(_T(".mga")) == 0) {
						m_console.Message( _T("Opening ") + conn + _T("."), 1);
						conn = _T("MGA=") + conn;
					} else {
						int pos = conn.ReverseFind( '\\'); // we don't need the file name, only the path
						if( is_dir)
							conn = _T("MGX=\"") + conn + _T(")\""); // directory dropped
						else if( pos != -1)
							conn = _T("MGX=\"") + conn.Left( pos) + _T("\""); // the .mgx file dropped, cut off the file part
						m_console.Message( _T("Opening multiuser project ") + conn + _T("."), 1);
					}
					theApp.OpenProject(conn);
					one_just_opened = true;
				}
				else
					m_console.Message( _T("Another MGA file can't be opened while a project is open."), 3);
			}
			else if( conn.Right(4).CompareNoCase(_T(".xme"))==0)
			{
				theApp.ImportDroppedFile(conn);
				one_just_opened = true;
			}
			else if( conn.Right(4).CompareNoCase(_T(".xmp"))==0 || conn.Right(4).CompareNoCase(_T(".mta"))==0)
			{
				if( theApp.guiMetaProject == NULL && theApp.mgaProject == 0) // no project opened
				{
					m_console.Message( _T("Registering ") + conn + _T(" as a paradigm."), 1);

					theApp.RegisterDroppedFile( conn.Right(4).CompareNoCase(_T(".xmp"))==0?_T("XML=") + conn:_T("MGA=") + conn);
					one_just_opened = false; // we did not open a file, just registered
				}
				else
					m_console.Message( _T("Can't register paradigm file while project is open!"), 3);
			}
			else if (conn.Right(4).CompareNoCase(_T(".htm")) == 0 || conn.Right(5).CompareNoCase(_T(".html")) == 0)
			{
				m_console.NavigateTo(conn);
			}

			else
				m_console.Message( _T("Only .MGX, .MGA, .XME, .MTA, .XMP, and .html files may be dropped. Can't open file: ") + conn + _T("."), 3);
		}
		else
			m_console.Message( _T("Can't inquire file information!"), 3);
	}
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	return CMDIFrameWndEx::PreTranslateMessage(pMsg);
}


void CMainFrame::OnBtnBack()
{
	if( !theApp.isHistoryEnabled() || !CGMEDoc::theInstance) return;
	CGMEDoc::theInstance->back();
}

void CMainFrame::OnUpdateBtnBack(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( theApp.isHistoryEnabled() && CGMEDoc::theInstance && CGMEDoc::theInstance->m_historian.isEnabledBack());
}

void CMainFrame::OnBtnHome()
{
	if( !theApp.isHistoryEnabled() || !CGMEDoc::theInstance) return;
	CGMEDoc::theInstance->home();
}

void CMainFrame::OnUpdateBtnHome(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( theApp.isHistoryEnabled() && CGMEDoc::theInstance && CGMEDoc::theInstance->m_historian.isEnabledHome());
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}


void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);

}


void CMainFrame::OnWindowManager()
{
	ShowWindowsDialog();
}

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

void CMainFrame::OnViewFullScreen()
{
	ShowFullScreen();
}


/*

void CMainFrame::OnHelpKeyboardmap()
{
	CMFCKeyMapDialog dlg(this, TRUE ); // Enable Print
	dlg.DoModal();
}

*/

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CMDIFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*) lres;
	ASSERT_VALID(pUserToolbar);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, _T("Customize..."));
	return lres;
}

void CMainFrame::OnUpdateControlBarMenus(CCmdUI* pCmdUI)
{
	switch (pCmdUI->m_nID) {
		case IDW_TOOLBAR_MAIN:		pCmdUI->SetCheck(m_wndToolBarMain.IsVisible());		break;
		case IDW_TOOLBAR_WINS:		pCmdUI->SetCheck(m_wndToolBarWins.IsVisible());		break;
		case IDW_TOOLBAR_COMPONENT:	pCmdUI->SetCheck(m_wndComponentBar.IsVisible());	break;
		case IDW_TOOLBAR_MODE:		pCmdUI->SetCheck(m_wndModeBar.IsVisible());			break;
		case IDW_TOOLBAR_NAVIG:		pCmdUI->SetCheck(m_wndNaviBar.IsVisible());			break;
		case IDW_TOOLBAR_MODELING:	pCmdUI->SetCheck(m_wndToolBarModeling.IsVisible());	break;
		default: ASSERT(false);
	}
}

BOOL CMainFrame::OnBarCheck(UINT nID)
{
	switch (nID) {
		case IDW_TOOLBAR_MAIN:		ShowPane(&m_wndToolBarMain,		!m_wndToolBarMain.IsVisible(),		FALSE, FALSE); break;
		case IDW_TOOLBAR_WINS:		ShowPane(&m_wndToolBarWins,		!m_wndToolBarWins.IsVisible(),		FALSE, FALSE); break;
		case IDW_TOOLBAR_COMPONENT:	ShowPane(&m_wndComponentBar,	!m_wndComponentBar.IsVisible(),		FALSE, FALSE); break;
		case IDW_TOOLBAR_MODE:		ShowPane(&m_wndModeBar,			!m_wndModeBar.IsVisible(),			FALSE, FALSE); break;
		case IDW_TOOLBAR_NAVIG:		ShowPane(&m_wndNaviBar,			!m_wndNaviBar.IsVisible(),			FALSE, FALSE); break;
		case IDW_TOOLBAR_MODELING:	ShowPane(&m_wndToolBarModeling,	!m_wndToolBarModeling.IsVisible(),	FALSE, FALSE); break;
		default: return FALSE;
	}
	return TRUE;
}

void CMainFrame::OnWindowNewhorizontaltabgroup()
{
	MDITabNewGroup(FALSE);
}

void CMainFrame::OnWindowNewverticaltabgroup()
{
	MDITabNewGroup();
}

void CMainFrame::OnWindowMovetoprevioustabgroup()
{
	MDITabMoveToNextGroup(FALSE);
}

void CMainFrame::OnWindowMovetonexttabgroup()
{
	MDITabMoveToNextGroup();
}

void CMainFrame::OnUpdateWindowNewhorizontaltabgroup(CCmdUI* pCmdUI)
{
	DWORD dwAllowedItems = GetMDITabsContextMenuAllowedItems();
	if ((dwAllowedItems & AFX_MDI_CREATE_HORZ_GROUP) == 0)
	{
		pCmdUI->Enable(FALSE);
	}
	else
	{
		pCmdUI->Enable(TRUE);
	}
}

void CMainFrame::OnUpdateWindowNewverticaltabgroup(CCmdUI* pCmdUI)
{
	DWORD dwAllowedItems = GetMDITabsContextMenuAllowedItems();
	if ((dwAllowedItems & AFX_MDI_CREATE_VERT_GROUP) == 0)
	{
		pCmdUI->Enable(FALSE);
	}
	else
	{
		pCmdUI->Enable(TRUE);
	}
}

void CMainFrame::OnUpdateWindowMovetoprevioustabgroup(CCmdUI* pCmdUI)
{
	DWORD dwAllowedItems = GetMDITabsContextMenuAllowedItems();
	if ((dwAllowedItems & AFX_MDI_CAN_MOVE_PREV) == 0)
	{
		pCmdUI->Enable(FALSE);
	}
	else
	{
		pCmdUI->Enable(TRUE);
	}
}

void CMainFrame::OnUpdateWindowMovetonexttabgroup(CCmdUI* pCmdUI)
{
	DWORD dwAllowedItems = GetMDITabsContextMenuAllowedItems();
	if ((dwAllowedItems & AFX_MDI_CAN_MOVE_NEXT) == 0)
	{
		pCmdUI->Enable(FALSE);
	}
	else
	{
		pCmdUI->Enable(TRUE);
	}
}

LRESULT CMainFrame::OnGetTabTooltip(WPARAM /*wParam*/, LPARAM lParam)
{
	CMFCTabToolTipInfo* pInfo = (CMFCTabToolTipInfo*)lParam;
	ASSERT(pInfo != NULL);

	if (pInfo)
	{
		CMFCBaseTabCtrl* tabControl = pInfo->m_pTabWnd;
		ASSERT_VALID(tabControl);
		if (tabControl->IsMDITab())
		{
			CWnd* tabPaneWnd = tabControl->GetTabWndNoWrapper(pInfo->m_nTabIndex);
			if (tabPaneWnd->IsKindOf(RUNTIME_CLASS(CChildFrame))) {
				CChildFrame* childFrame = STATIC_DOWNCAST(CChildFrame, tabPaneWnd);
				pInfo->m_strText = childFrame->GetTitle() + _T(" - ") + childFrame->GetAppTitle();
			}
		}
	}

	return 0;
}

void CMainFrame::ShowNavigationAndModeToolbars(bool isVisible)
{
	ShowPane(&m_wndModeBar, isVisible, FALSE, FALSE);
	ShowPane(&m_wndNaviBar, isVisible, FALSE, FALSE);
}

void CMainFrame::OnUpdateWindowNew(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(FALSE);
}

void CMainFrame::CheckForOffscreenPanes()
{
	// it seems Windows does this for us when resolution is changed, so no need to handle WM_DISPLAYCHANGE
	// but it is possible to float a pane, then close GME, then change the screen resolution

	// TODO: these can float offscreen too...
	//m_wndComponentBar; m_wndStatusBar; m_wndToolBarMain; m_wndToolBarModeling; m_wndToolBarWins; m_wndMenuBar; m_wndModeBar; m_wndNaviBar;
	CDockablePane* panes[] = { &m_panningWindow, &m_console, &m_partBrowser, &m_browser, &m_objectInspector, &m_search, nullptr };
	CDockablePane** pane = panes;
	while (*pane)
	{
		HMONITOR mon = MonitorFromWindow((*pane)->GetSafeHwnd(), MONITOR_DEFAULTTONULL);
		bool flt = (*pane)->IsFloating();
		if (mon == nullptr && flt)
		{
			DockPane(*pane, AFX_IDW_DOCKBAR_BOTTOM);
		}
		pane++;
	}
	return;
}

LRESULT CMainFrame::OnDisplayChange(WPARAM wParam, LPARAM lParam) {
	CDocument* pDocument = CGMEDoc::theInstance;
	POSITION pos = pDocument->GetFirstViewPosition();
	if (pos) {
		while (pos != NULL) {
			CGMEChildFrame* pView = (CGMEChildFrame*)pDocument->GetNextView(pos);
			pView->SendMessage(WM_DISPLAYCHANGE, wParam, lParam);
		}
	}

	return TRUE;
}

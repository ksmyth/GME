// ConsoleCtl.cpp : Implementation of the CConsoleCtrl ActiveX Control class.

#include "stdafx.h"
#include "Console.h"
#include "Console_i.h"
#include "ConsoleCtl.h"
#include "ConsolePpg.h"
#include "GME.h"
#include "MgaUtil.h"
#include <mshtml.h>         //IWeb objects
#include <limits.h>
#include <afx.h>

#define BUTTON_ICON_SIZE		16


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CConsoleCtrl, COleControl)

/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CConsoleCtrl, COleControl)
	//{{AFX_MSG_MAP(CConsoleCtrl)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_COMMAND( IDC_MENU_COMMAND, OnShowMenu)
	ON_COMMAND( IDC_LOADSCRIPT_COMMAND, LoadScriptDlg)
	ON_COMMAND( IDC_RUNSCRIPT_COMMAND, runScript)
	ON_COMMAND( IDC_RELOAD_COMMAND, relScript)
	ON_COMMAND( IDC_RELOADRUN_COMMAND, rlrScript)
	ON_COMMAND( IDC_CLEARCONSOLE_COMMAND, Clear)
	ON_COMMAND( IDC_SELECTENGINE_COMMAND, selectEngine)
	ON_COMMAND( IDC_PREV_COMMAND, prevCommand)
	ON_COMMAND( IDC_NEXT_COMMAND, nextCommand)
	ON_COMMAND( IDC_RETURN_COMMAND, retCommand)
	ON_COMMAND_RANGE( IDC_RECENT_SCRIPT1, IDC_RECENT_SCRIPT5, loadRecent)
	ON_NOTIFY_EX( TTN_NEEDTEXT, 0, OnToolTipNotify)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CConsoleCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CConsoleCtrl)
	DISP_PROPERTY_EX(CConsoleCtrl, "Contents", GetContents, SetContents, VT_BSTR)
	DISP_FUNCTION(CConsoleCtrl, "Message", Message, VT_EMPTY, VTS_BSTR VTS_I2)
	DISP_FUNCTION(CConsoleCtrl, "Clear", Clear, VT_EMPTY, VTS_NONE)
	DISP_DEFVALUE(CConsoleCtrl, "Contents")
	DISP_FUNCTION(CConsoleCtrl, "SetGMEApp", SetGMEApp, VT_EMPTY, VTS_DISPATCH)
	DISP_FUNCTION(CConsoleCtrl, "SetGMEProj", SetGMEProj, VT_EMPTY, VTS_DISPATCH)
	DISP_FUNCTION_ID(CConsoleCtrl, "NavigateTo", dispidNavigateTo, NavigateTo, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION_ID(CConsoleCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CConsoleCtrl, COleControl)
	//{{AFX_EVENT_MAP(CConsoleCtrl)
	EVENT_CUSTOM("ClickMGAID", FireClickMGAID, VTS_BSTR)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CConsoleCtrl, 1)
	PROPPAGEID(CConsolePropPage::guid)
END_PROPPAGEIDS(CConsoleCtrl)

/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CConsoleCtrl, "GME.Console.1",
	0xe4eb8324, 0x13f0, 0x46cb, 0x96, 0xa3, 0xf5, 0x39, 0xdf, 0x7f, 0x79, 0xd4)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CConsoleCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DConsole =
		{ 0x71655c2b, 0xd6dd, 0x4345, { 0xb2, 0x73, 0x5b, 0xd1, 0x42, 0x23, 0x70, 0xcb } };
const IID BASED_CODE IID_DConsoleEvents =
		{ 0x39f56b4c, 0xe7db, 0x4a54, { 0x86, 0x8c, 0xca, 0xa7, 0xeb, 0xe2, 0x32, 0x33 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwConsoleOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CConsoleCtrl, IDS_CONSOLE, _dwConsoleOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CConsoleCtrl::CConsoleCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CConsoleCtrl

BOOL CConsoleCtrl::CConsoleCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_CONSOLE,
			IDB_CONSOLE,
			afxRegApartmentThreading,
			_dwConsoleOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CConsoleCtrl::CConsoleCtrl - Constructor

CConsoleCtrl::CConsoleCtrl()
: m_recent1("")
, m_recent2("")
, m_recent3("")
, m_recent4("")
, m_recent5("")
, m_hIco1(NULL)
, m_hIco2(NULL)
, m_hIco3(NULL)
, m_hIcou(NULL)
, m_hIcod(NULL)
, m_hIcor(NULL)
{
	InitializeIIDs(&IID_DConsole, &IID_DConsoleEvents);

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CConsoleCtrl::~CConsoleCtrl - Destructor

CConsoleCtrl::~CConsoleCtrl()
{
	// TODO: Cleanup your control's instance data here.
	if (m_hIco1)
		::DestroyIcon(m_hIco1);
	if (m_hIco2)
		::DestroyIcon(m_hIco2);
	if (m_hIco3)
		::DestroyIcon(m_hIco3);
	if (m_hIcou)
		::DestroyIcon(m_hIcou);
	if (m_hIcod)
		::DestroyIcon(m_hIcod);
	if (m_hIcor)
		::DestroyIcon(m_hIcor);
}

// creates a 3d look and feel around a textbox, 
// so that it resembles an edit field much likely
CRect has3dLook(CDC* pdc, CRect rc)
{
	rc.bottom += 1;
	rc.right  += 1;
	pdc->SelectStockObject( NULL_PEN);

	DWORD dw = ::GetSysColor(COLOR_3DFACE);
	BYTE r = GetRValue(dw);
	BYTE g = GetGValue(dw);
	BYTE b = GetBValue(dw);
	COLORREF col = RGB(r,g,b);
	CBrush brush;
	brush.CreateSolidBrush(col);
	// background painted for the area
	pdc->FillRect(&rc, &brush);

	int decr_x = 3;                           // will deflate with this constants
	int decr_y = (rc.Height()-20)/2;
	rc.bottom -= decr_y;   rc.right -= decr_x;
	rc.top    += decr_y;   rc.left  += decr_x;

	pdc->SelectStockObject( WHITE_BRUSH);
	pdc->Rectangle( rc); // the biggest rect
	
	pdc->SelectStockObject( GRAY_BRUSH);
	rc.bottom -= 1; rc.right -= 1;
	pdc->Rectangle( rc); // the 2nd biggest rect (inverse cascaded compared to the white rc)

	rc.top += 1; rc.left += 1;
	pdc->FillRect(&rc, &brush);	// cascaded compared to the white rc

	pdc->SelectStockObject( BLACK_BRUSH);
	rc.bottom -= 1; rc.right -= 1;
	pdc->Rectangle( rc);

	rc.DeflateRect( 1, 1, 1, 1);
	return rc;
}

// centeres a rectangle inside another one, so that a 
// square with p_bt_size side will sit in the center
CRect centered( const CRect& p_rcIn, int p_vertic_size, int p_horiz_size, int p_bt_size)
{
	CRect rc( p_rcIn);
	rc.top += (p_vertic_size - p_bt_size)/2;
	rc.bottom -= (p_vertic_size - p_bt_size)/2;
	rc.left += (p_horiz_size - p_bt_size)/2;
	rc.right -= (p_horiz_size - p_bt_size)/2;
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
// CConsoleCtrl::OnDraw - Drawing function

void CConsoleCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	const int vertic_size   = 26;
	const int horiz_size    = 26;
	const int bt_size       = 20;
	const int prnx_size     = 19; // width of the prev/next buttons


	if (m_browser.GetSafeHwnd()) {
		bool  anything_loaded = !m_edit.GetLoadedScript().IsEmpty();
		CRect rcBrwsr, rcExtra, rcExtra2;  // browser rect, extra rect
		rcExtra2 = rcBrwsr = rcExtra = rcBounds;

		rcExtra2.left -= 1; rcExtra2.bottom += 1;
		// light gray background for the whole region
		DWORD dw = ::GetSysColor(COLOR_3DFACE);
		BYTE r = GetRValue(dw);
		BYTE g = GetGValue(dw);
		BYTE b = GetBValue(dw);
		COLORREF col = RGB(r,g,b);
		CBrush brush;
		brush.CreateSolidBrush(col);
		pdc->FillRect(&rcExtra2, &brush);

		rcBrwsr.bottom -= vertic_size;     // take from the browser's space 30 lines
		rcExtra.top = rcBrwsr.bottom;      // extra is the remaining space
		rcBrwsr.bottom -= 1;               // use less space for the browser to have a divide


		pdc->SelectStockObject( BLACK_PEN);
		pdc->SelectStockObject( GRAY_BRUSH);
		rcExtra2.top = rcBrwsr.bottom;
		rcExtra2.bottom = rcExtra2.top+1;
		pdc->Rectangle( rcExtra2);         // a divider line between the browser and scripting 

		// place browser
		m_browser.MoveWindow( rcBrwsr, TRUE);

		// place the generic command button
		CRect rcLoad( rcExtra);                     // load btn rect
		rcLoad.right = rcLoad.left + horiz_size;
		
		m_cmdButton.MoveWindow( centered( rcLoad, vertic_size, horiz_size, bt_size), TRUE);

		// place and show/hide 'Execute' button based on bool (anything_loaded)
		if( anything_loaded) {
			rcLoad.OffsetRect( horiz_size, 0);
			
			m_exeButton.MoveWindow( centered( rcLoad, vertic_size, horiz_size, bt_size), TRUE);
			m_exeButton.ShowWindow(SW_SHOW);
		}
		else {
			m_exeButton.ShowWindow(SW_HIDE);
		}

		// place 'Clear'
		CRect rcClean = rcExtra;                    // clear btn rect
		rcClean.left = rcClean.right - horiz_size;
		
		CRect rcClrBtn(rcClean);
		m_clrButton.MoveWindow( centered( rcClrBtn, vertic_size, horiz_size, bt_size), TRUE);

		// place 'Return'
		rcClean.right = rcClean.left; rcClean.left -= horiz_size;

		m_retButton.MoveWindow( centered( rcClean, vertic_size, horiz_size, bt_size), TRUE);

		// place history prev/next button pair to the left of Clean
		rcClean.right = rcClean.left; rcClean.left -= prnx_size;
		rcClean.bottom = rcClean.top + static_cast<int>(vertic_size/2);
		CRect rcUDBtn( rcClean);
		rcUDBtn.DeflateRect( 0, 2, 0, 0);
		m_upButton.MoveWindow( rcUDBtn, TRUE);

		rcClean.top = rcClean.bottom;
		rcClean.bottom = rcExtra.bottom;//rcClean.OffsetRect( 0, vertic_size - static_cast<int>(vertic_size/2));
		rcUDBtn = rcClean;
		rcUDBtn.DeflateRect( 0, 1, 0, 2);
		m_dnButton.MoveWindow( rcUDBtn, TRUE);
		
		CRect rcRemain = rcExtra; // remaining rect
		rcRemain.left = rcLoad.right;         // left side
		rcRemain.right= rcClean.left;         // right side

		rcRemain = has3dLook( pdc, rcRemain);

		// move input control too 
		m_edit.MoveWindow(rcRemain, TRUE);
	}
	else {
		CBrush brush (RGB(255,255,255));
		pdc->FillRect (rcBounds, &brush);

		CRect edge(rcBounds);
		pdc->DrawEdge(&edge, EDGE_BUMP, BF_RECT);
		
		CString label("GME Console OCX");

		BITMAP bm;
		CBitmap bitmap;
		bitmap.LoadBitmap(IDB_CONSOLE);
		bitmap.GetBitmap(&bm);
		CSize size(bm.bmWidth, bm.bmHeight);
		pdc->DPtoLP(&size);
		CPoint org(0,0);
		pdc->DPtoLP(&org);
		CPoint pos(rcBounds.Width()/2, rcBounds.Height()/2 - (pdc->GetTextExtent(label).cy) );
		CDC dcMem;
		dcMem.CreateCompatibleDC(pdc);
		CBitmap *oldbitmap = dcMem.SelectObject(&bitmap);
		dcMem.SetMapMode (pdc->GetMapMode());
		pdc->BitBlt(pos.x, pos.y, size.cx, size.cy, &dcMem, org.x, org.y, SRCCOPY);
		dcMem.SelectObject(oldbitmap);
		
		pdc->SetTextAlign(TA_CENTER);
 		pdc->TextOut(rcBounds.Width()/2, rcBounds.Height()/2, label);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CConsoleCtrl::DoPropExchange - Persistence support

void CConsoleCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CConsoleCtrl::OnResetState - Reset control to default state

void CConsoleCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CConsoleCtrl::AboutBox - Display an "About" box to the user

void CConsoleCtrl::AboutBox()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	CDialog dlgAbout(IDD_ABOUTBOX_CONSOLE);
	dlgAbout.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
void CConsoleCtrl::SetGMEApp(IDispatch *disp)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	m_edit.SetGMEApp(disp);
}

void CConsoleCtrl::SetGMEProj(IDispatch *disp)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	m_edit.SetGMEProj(disp);
}

void CConsoleCtrl::NavigateTo(LPCTSTR url)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	m_browser.Navigate2(url);
}
/////////////////////////////////////////////////////////////////////////////
// CConsoleCtrl message handlers

int CConsoleCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CRect rect(0,0,0,0);
	// create an input controll too
	m_browser.Create(NULL, 
		_T("GME Console"), 
		(WS_VISIBLE | WS_CHILD), 
		rect, 
		this, 
		IDD_BROWSER);
	m_browser.LoadFromResource("BLANK.HTML");

	m_edit.Create((ES_AUTOHSCROLL | WS_VISIBLE | WS_CHILD), rect, this, IDD_EDIT);
	m_edit.LimitText(300);
	bool ret = m_edit.Init(this);

	m_hIco1 = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_LOADSCR),	IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	m_hIco2 = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_EXECSCR),	IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	m_hIco3 = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CLEANCON),	IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	m_hIcou = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_BTNUP),	IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	m_hIcod = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_BTNDN),	IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	m_hIcor = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_BTNRET),	IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);

	rect.bottom = rect.right = 100;
	rect.left = rect.right - BUTTON_ICON_SIZE;
	rect.bottom = rect.top - BUTTON_ICON_SIZE;

	m_cmdButton.Create( _T("MenuButton"), WS_CHILD|WS_VISIBLE|BS_ICON, rect, this, IDC_MENU_COMMAND);
	m_cmdButton.SetIcon(m_hIco1);

	rect.right += BUTTON_ICON_SIZE;
	rect.left  += BUTTON_ICON_SIZE;
	m_exeButton.Create( _T("ExecButton"), WS_CHILD/*|WS_VISIBLE*/|BS_ICON, rect, this, IDC_RUNSCRIPT_COMMAND);
	m_exeButton.SetIcon(m_hIco2);

	rect.left  += BUTTON_ICON_SIZE;
	rect.right += BUTTON_ICON_SIZE;
	m_clrButton.Create( _T("ClearConsButton"), WS_CHILD|WS_VISIBLE|BS_ICON, rect, this, IDC_CLEARCONSOLE_COMMAND);
	m_clrButton.SetIcon(m_hIco3);

	CRect rec2 = rect;
	//rec2.right = rec2.left + 40; rec2.bottom = rec2.top + BUTTON_ICON_SIZE;
	rec2.right = rec2.left + BUTTON_ICON_SIZE + 3;	//bmuSize.cx + 3; 
	rec2.bottom = rec2.top + BUTTON_ICON_SIZE;		//bmuSize.cy + 3;
	m_upButton.Create(_T("Prev"), WS_CHILD|WS_VISIBLE|BS_ICON, rec2, this, IDC_PREV_COMMAND);
	m_upButton.SetIcon(m_hIcou);

	m_dnButton.Create(_T("Next"), WS_CHILD|WS_VISIBLE|BS_ICON, rec2, this, IDC_NEXT_COMMAND);
	m_dnButton.SetIcon(m_hIcod);

	m_retButton.Create(_T("Return"), WS_CHILD|WS_VISIBLE|BS_ICON, rec2, this, IDC_RETURN_COMMAND);
	m_retButton.SetIcon(m_hIcor);

	EnableToolTips();

	return 0;
}


static const TCHAR* icons[] = {
	_T("NORMAL.GIF"),
	_T("INFO.GIF"),
	_T("WARNING.GIF"),
	_T("ERROR.GIF")
};

void CConsoleCtrl::Message(LPCTSTR str, short type) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	if (type < MSG_NORMAL || type > MSG_ERROR) {
		type = MSG_ERROR;
	}
    CComPtr<IDispatch> pDispatch = m_browser.GetHtmlDocument();
    if( pDispatch != NULL )
    {
		CString line;
		
		line.Format("<IMG SRC=\"%s\" ALIGN=MIDDLE > %s<BR>", icons[type], str);

		VARIANT_BOOL time_stamping = VARIANT_FALSE; // default
		try {
			CComPtr<IMgaRegistrar> registrar;
			COMTHROW( registrar.CoCreateInstance(L"Mga.MgaRegistrar") );

			COMTHROW( registrar->GetTimeStamping(REGACCESS_USER, &time_stamping));
			if(time_stamping != VARIANT_FALSE)
			{
				CTime time = CTime::GetCurrentTime();
				line = time.Format(_T("[%H:%M:%S] ")) + line;
			}
		} catch(hresult_exception &)
		{
		}

		try {
			CComPtr<IHTMLDocument2> pHtmlDoc;
			COMTHROW(pDispatch.QueryInterface(&pHtmlDoc));
			ASSERT(pHtmlDoc != NULL);
			CComPtr<IHTMLElement> pElement;
			COMTHROW(pHtmlDoc->get_body( &pElement ));
			ASSERT(pElement != NULL);
			CComBSTR where("BeforeEnd");
			CComBSTR text(line);
			COMTHROW(pElement->insertAdjacentHTML(where, text));

			// Scroll
			CComPtr<IHTMLWindow2> parentWindow;
			COMTHROW(pHtmlDoc->get_parentWindow( &parentWindow ));
			ASSERT(parentWindow != NULL);
			HRESULT hhh = (parentWindow->scrollTo( 0,  LONG_MAX/16)); // Starting from IE8 we need this /16 hack.  
																	  // If you know the proper way of scrolling to the bottom, fix this.
			//COMTHROW(parentWindow->scrollTo( 0,  LONG_MAX));
		}
		catch (hresult_exception &)
		{
		}
    }

}

void CConsoleCtrl::Clear() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	m_browser.LoadFromResource("BLANK.HTML");
}

BSTR CConsoleCtrl::GetContents() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	CComBSTR contents;
	
    CComPtr<IDispatch> pDispatch = m_browser.GetHtmlDocument();
    if( pDispatch != NULL )
    {
		try {
			CComPtr<IHTMLDocument2> pHtmlDoc;
			COMTHROW(pDispatch.QueryInterface(&pHtmlDoc));
			ASSERT(pHtmlDoc != NULL);
			CComPtr<IHTMLElement> pElement;
			COMTHROW(pHtmlDoc->get_body( &pElement ));
			ASSERT(pElement != NULL);
			COMTHROW(pElement->get_innerHTML(&contents));
		}
		catch (hresult_exception &)
		{
		}
    }

	return contents;
}

void CConsoleCtrl::SetContents(LPCTSTR lpszNewValue) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	CComBSTR contents(lpszNewValue);
	
    CComPtr<IDispatch> pDispatch = m_browser.GetHtmlDocument();
    if( pDispatch != NULL )
    {
		try {
			CComPtr<IHTMLDocument2> pHtmlDoc;
			COMTHROW(pDispatch.QueryInterface(&pHtmlDoc));
			ASSERT(pHtmlDoc != NULL);
			CComPtr<IHTMLElement> pElement;
			COMTHROW(pHtmlDoc->get_body( &pElement ));
			ASSERT(pElement != NULL);
			COMTHROW(pElement->put_innerHTML(contents));
		}
		catch (hresult_exception &)
		{
		}
    }

	SetModifiedFlag();
}

void CConsoleCtrl::OnShowMenu()
{
	if( 0 > ::GetKeyState( VK_CONTROL))
	{
		runScript();
		return;
	}
	else if( 0 > ::GetKeyState( VK_SHIFT))
	{
		LoadScriptDlg();
		return;
	}

	CMenu r; // recentMenu;
	if( r.CreatePopupMenu())
	{
		if( !m_recent1.IsEmpty()) r.AppendMenu( MF_STRING, IDC_RECENT_SCRIPT1,  m_recent1);
		if( !m_recent2.IsEmpty()) r.AppendMenu( MF_STRING, IDC_RECENT_SCRIPT2,  m_recent2);
		if( !m_recent3.IsEmpty()) r.AppendMenu( MF_STRING, IDC_RECENT_SCRIPT3,  m_recent3);
		if( !m_recent4.IsEmpty()) r.AppendMenu( MF_STRING, IDC_RECENT_SCRIPT4,  m_recent4);
		if( !m_recent5.IsEmpty()) r.AppendMenu( MF_STRING, IDC_RECENT_SCRIPT5,  m_recent5);
	}

	CMenu p;
	if( p.CreatePopupMenu())
	{
		p.AppendMenu( MF_STRING, IDC_LOADSCRIPT_COMMAND,   "Load Script");
		if( !m_edit.GetLoadedScriptFileName().IsEmpty())
			p.AppendMenu( MF_STRING,IDC_RELOAD_COMMAND, "Reload Current"); // or IDC_RELOADRUN_COMMAND
		if( !m_recent1.IsEmpty()) // m_recent1 can tell us if there is anything in the recent list
			p.AppendMenu( MF_POPUP, (UINT_PTR) r.GetSafeHmenu(), "Recent Scripts");
		p.AppendMenu( MF_STRING, IDC_SELECTENGINE_COMMAND,   "Settings");
		
		CRect rc;
		m_cmdButton.GetWindowRect( &rc);
		p.TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, rc.left + rc.Width()/2, rc.top + rc.Height()/2, this);
		Invalidate();
	}
}

void CConsoleCtrl::RunScript()
{
	if( !m_edit.GetLoadedScript().IsEmpty())
	{
		Message( CString( "Executing script: ") + m_edit.GetLoadedScriptFileName(), MSG_INFO);
		m_edit.ExecuteScript( m_edit.GetLoadedScript());
	}
	else
		Message( "Script not found!", MSG_INFO);
}

void CConsoleCtrl::LoadScript( const CString& p_fileName)
{
	m_edit.SetLoadedScript(""); // erase old loaded script contents

	CStdioFile _file;

	// open file
	if( _file.Open( p_fileName, CFile::modeRead | CFile::typeText) == 0) {
		Message( "Unable to open file.", MSG_ERROR);
		return;
	}

	CString script_buf;
	unsigned int sizeof_file = (unsigned int) _file.GetLength();
	sizeof_file = _file.Read( script_buf.GetBufferSetLength( sizeof_file), sizeof_file);
	// sizeof_file was modified above because of newline handling
	if( sizeof_file > 0) script_buf.ReleaseBufferSetLength( sizeof_file); 
	else script_buf.ReleaseBuffer(0);
	_file.Close();

	m_edit.SetLoadedScript( script_buf);
	m_edit.SetScriptFileName( p_fileName);

	Message( CString( "Loaded script: ") + p_fileName, MSG_INFO);
	Invalidate();
}

void CConsoleCtrl::LoadScriptDlg()
{
	CFileDialog dlg(TRUE, "py", 0,
		OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"Script Files (*.js;*.py;*.vbs)|*.js;*.vbs;*.py|"
		"Python Script Files (*.py)|*.py|"
		"JScript Files (*.js)|*.js|"
		"VBScript Files (*.vbs)|*.vbs|"
		"Text Files (*.txt)|*.txt|"
		"HTML Files (*.html;*.htm)|*.html;*.htm|"
		"All Files (*.*)|*.*||");

	if( dlg.DoModal() == IDOK) 
	{
		CString fpath = dlg.GetPathName();
		CString ext = dlg.GetFileExt().MakeLower();
		if (ext == "html" || ext == "htm") {
			m_browser.Navigate2(fpath);
			return;
		}

		LoadScript( fpath);

		if( m_recent1.IsEmpty())
			m_recent1 = fpath;
		else if( m_recent2.IsEmpty())
		{
			m_recent2 = m_recent1;
			m_recent1 = fpath;
		}
		else if( m_recent3.IsEmpty())
		{
			m_recent3 = m_recent2;
			m_recent2 = m_recent1;
			m_recent1 = fpath;
		}
		else if( m_recent4.IsEmpty())
		{
			m_recent4 = m_recent3;
			m_recent3 = m_recent2;
			m_recent2 = m_recent1;
			m_recent1 = fpath;
		}
		else if( m_recent5.IsEmpty())
		{
			m_recent5 = m_recent4;
			m_recent4 = m_recent3;
			m_recent3 = m_recent2;
			m_recent2 = m_recent1;
			m_recent1 = fpath;
		}
	}
}

void CConsoleCtrl::runScript()
{
	RunScript();
}

void CConsoleCtrl::relScript() // reload
{
	LoadScript( m_edit.GetLoadedScriptFileName());
}

void CConsoleCtrl::rlrScript() // reload & run
{
	LoadScript( m_edit.GetLoadedScriptFileName());
	RunScript();
}

void CConsoleCtrl::selectEngine()
{
	try {
		CComPtr<IMgaLauncher> launcher;
		COMTHROW( launcher.CoCreateInstance(L"Mga.MgaLauncher") );
		COMTHROW( launcher->GmeDlg());
	}
	catch(...) {
		Message( "Error while trying to show GME settings dialog", MSG_ERROR);
	}
}

void CConsoleCtrl::loadRecent( UINT which)
{
	switch( which) {
	case IDC_RECENT_SCRIPT1: LoadScript( m_recent1);break;
	case IDC_RECENT_SCRIPT2: LoadScript( m_recent2);break;
	case IDC_RECENT_SCRIPT3: LoadScript( m_recent3);break;
	case IDC_RECENT_SCRIPT4: LoadScript( m_recent4);break;
	case IDC_RECENT_SCRIPT5: LoadScript( m_recent5);break;
	}
}

void CConsoleCtrl::prevCommand()
{
	m_edit.showPrev();
}

void CConsoleCtrl::nextCommand()
{
	m_edit.showNext();
}

void CConsoleCtrl::retCommand()
{
	m_edit.returnHit();
}

BOOL CConsoleCtrl::OnToolTipNotify(UINT id, NMHDR *pNMHDR, LRESULT *pResult)
{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	UINT            nID = pNMHDR->idFrom;
	
	CString tip;

	if( pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
	    pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	if( nID != 0) // will be zero on a separator
	{
		//tip.Format("Control ID = %d", nID);
		switch( nID)
		{
			      case IDC_RETURN_COMMAND:       tip = "Executes the command typed into the edit field";
			break;case IDC_RUNSCRIPT_COMMAND:    tip = "Executes " + m_edit.GetLoadedScriptFileName();
			break;case IDC_MENU_COMMAND:         tip = "Load scripts and access recent scripts";
			break;case IDC_CLEARCONSOLE_COMMAND: tip = "Clear the console";
			break;case IDC_PREV_COMMAND:         tip = "Previous command in history";
			break;case IDC_NEXT_COMMAND:         tip = "Next command in history";
			break;case IDD_EDIT:                 tip = "Enter a command here";
		}
	}

	if( pNMHDR->code == TTN_NEEDTEXTA)
		lstrcpyn(pTTTA->szText, tip, sizeof(pTTTA->szText));
	else
		::MultiByteToWideChar( CP_ACP , 0, tip, -1, pTTTW->szText, sizeof(pTTTW->szText));

	*pResult = 0;
	return TRUE;    // message was handled
}




// GmeDlg.cpp : implementation file
//
//#undef _NO_SCRIPT_GUIDS

#include "stdafx.h"
#include "MgaUtil.h"
#include "GmeDlg.h"
#include "DirDialog.h"
#include "comcat.h"
#include "UACUtils.h"

//#include "basetyps.h"
const GUID CATID_ActiveScript = { 0xf0b7a1a1, 0x9847, 0x11cf, { 0x8f, 0x20, 0x0, 0x80, 0x5f, 0x2c, 0xd0, 0x64} };

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*static*/ const TCHAR* CGmeDlg::m_strZWidth			= _T("Fit Width");
/*static*/ const TCHAR* CGmeDlg::m_strZHeight		= _T("Fit Height");
/*static*/ const TCHAR* CGmeDlg::m_strZAll			= _T("Fit All");

/*static*/ const TCHAR* CGmeDlg::m_strFmtStrg		= _T("%g");
/*static*/ const TCHAR* CGmeDlg::m_strFmtStrf		= _T("%f");
/*static*/ const TCHAR* CGmeDlg::m_strFmtStrE		= _T("%E");
/*static*/ const TCHAR* CGmeDlg::m_strFmtStre		= _T("%e");
/*static*/ const TCHAR* CGmeDlg::m_strFmtStrg2		= _T("%.12g"); // the default one
/*static*/ const TCHAR* CGmeDlg::m_strFmtStrf2		= _T("%lf");

/*static*/ const TCHAR* CGmeDlg::m_strESStrD			= _T("No Smooth");
/*static*/ const TCHAR* CGmeDlg::m_strESStrHS		= _T("High Speed Mode");
/*static*/ const TCHAR* CGmeDlg::m_strESStrHQ		= _T("High Quality Mode");

/*static*/ const TCHAR* CGmeDlg::m_strFSStrSD		= _T("System Default");
/*static*/ const TCHAR* CGmeDlg::m_strFSStrSBPPGF	= _T("Single Bit Per Pixel Grid Fit");
/*static*/ const TCHAR* CGmeDlg::m_strFSStrSBPP		= _T("Single Bit Per Pixel");
/*static*/ const TCHAR* CGmeDlg::m_strFSStrAAGF		= _T("Anti Alias Grid Fit");
/*static*/ const TCHAR* CGmeDlg::m_strFSStrAA		= _T("Anti Alias");
/*static*/ const TCHAR* CGmeDlg::m_strFSStrCTGF		= _T("Clear Type Grid Fit");


/////////////////////////////////////////////////////////////////////////////
// CGmeDlg dialog


CGmeDlg::CGmeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGmeDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGmeDlg)
	m_iconpath = _T("");
	m_sysiconpath = _T("");
	m_multipleview = FALSE;
	m_enablelogging = TRUE;
	m_autosave_dir = _T("");
	m_autosave_enabled = FALSE;
	m_autosave_freq = 0;
	m_autosave_dest = -1;
	m_ext_enable = FALSE;
	m_ext_editor = _T("");
	m_useAutoRouting = TRUE;
	m_labelavoidance = FALSE;
	m_sendOverObj = FALSE;
	m_timeStamps = FALSE;
	m_navigationHistory = FALSE;
	//}}AFX_DATA_INIT
	m_scriptEngine = _T("JScript");
}


void CGmeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGmeDlg)
	DDX_Control(pDX, IDC_LIST1, m_ScriptEgines);
	DDX_Text(pDX, IDC_ICONPATH, m_iconpath);
	DDX_Text(pDX, IDC_SYSICONPATH, m_sysiconpath);
	DDX_Check(pDX, IDC_MULTIPLE_OPEN, m_multipleview);
	DDX_Check(pDX, IDC_EVENT_LOGGING, m_enablelogging);
	DDX_Text(pDX, IDC_AUTOSAVE_DIR, m_autosave_dir);
	DDX_Check(pDX, IDC_AUTOSAVE_ENABLED, m_autosave_enabled);
	DDX_Text(pDX, IDC_AUTOSAVE_FREQ, m_autosave_freq);
	DDX_Radio(pDX, IDC_AUTOSAVE_SAME_DIR, m_autosave_dest);
	DDX_Check(pDX, IDC_EXT_ENABLE, m_ext_enable);
	DDX_Text(pDX, IDC_EXT_EDITOR, m_ext_editor);
	DDX_Check(pDX, IDC_AUTOROUTEDEFAULT, m_useAutoRouting);
	DDX_Check(pDX, IDC_LABELAVOIDANCE, m_labelavoidance);
	DDX_Check(pDX, IDC_SENDOVEROBJECT, m_sendOverObj);
	DDX_Check(pDX, IDC_TIMESTAMPING, m_timeStamps);
	DDX_Check(pDX, IDC_NAVIGATIONHISTORY, m_navigationHistory);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGmeDlg, CDialog)
	//{{AFX_MSG_MAP(CGmeDlg)
	ON_BN_CLICKED(IDC_ADDICONPATH, OnAddIconPath)
	ON_BN_CLICKED(IDC_ADDSYSICONPATH, OnAddsysiconpath)
	ON_BN_CLICKED(IDC_AUTOSAVE_DIR_BUTTON, OnAutosaveDirButton)
	ON_BN_CLICKED(IDC_AUTOSAVE_ENABLED, OnAutosaveEnabled)
	ON_BN_CLICKED(IDC_AUTOSAVE_SAME_DIR, OnAutosaveSameDir)
	ON_BN_CLICKED(IDC_AUTOSAVE_DEDICATED_DIR, OnAutosaveDedicatedDir)
	ON_BN_CLICKED(IDC_EXT_BUTTON, OnExtButton)
	ON_BN_CLICKED(IDC_EXT_ENABLE, OnExtEnable)
	//}}AFX_MSG_MAP
	ON_CBN_SELCHANGE(IDC_ZOOMS, OnCbnSelchangeZooms)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGmeDlg message handlers

static TCHAR icofilter[] = _T("Icon files (*.ico;*.bmp)|*.ico;*.bmp|All Files (*.*)|*.*||");
static TCHAR bakfilter[] = _T("Backup files (*.bak)|*.bak|All Files (*.*)|*.*||");
static TCHAR exefilter[] = _T("Executable files (*.exe)|*.exe|All Files (*.*)|*.*||");

//AFX_MANAGE_STATE( AfxGetStaticModuleState()); 

// GMEVistaUtil.cpp:
HRESULT VistaBrowseDirectory(CString& directory);

CString CGmeDlg::getDirectory( const CString& text = _T("Specify the icons directory"))
{
	if (CUACUtils::isVistaOrLater())
	{
		CString ret;
		HRESULT hr = VistaBrowseDirectory(ret);
		return ret;
	}

	BROWSEINFO bi;
	TCHAR szDir[MAX_PATH];
	CString Dir = _T("");
	LPITEMIDLIST pidl;
	LPMALLOC pMalloc;	
	OleInitialize(NULL);
	if (SUCCEEDED(SHGetMalloc(&pMalloc)))
	{
		ZeroMemory(&bi,sizeof(bi));
		bi.hwndOwner = NULL;
		bi.pszDisplayName = 0;
		bi.lpszTitle = text;
		bi.pidlRoot = 0;
		bi.ulFlags = /*BIF_BROWSEINCLUDEFILES |*/ BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_EDITBOX | BIF_VALIDATE ;
		bi.lpfn = NULL;
		//Displays a dialog box that enables the user to select a shell folder
		pidl = SHBrowseForFolder(&bi) ;
		if (pidl)
		{
			// Converts an item identifier list to a file system path
			if (!SHGetPathFromIDList(pidl,szDir))
			{
				pMalloc->Free(pidl) ;
				pMalloc->Release() ;
			}
			pMalloc->Free(pidl);
			pMalloc->Release();
		}
		else
		{	// Selected 'Not to save'
			return _T("");
		}
		Dir = szDir;
		//Dir += _T("\\");
	}
	// end of getting directory info 
	return Dir;
}

void CGmeDlg::OnAddIconPath() 
{
	//CDirDialog dlg(NULL, icofilter, NULL);
	//dlg.DoModal();
	//CString path = dlg.GetPath();
	CString path = getDirectory();

	GetDlgItem(IDC_ICONPATH)->GetWindowText(m_iconpath);

	if( !m_iconpath.IsEmpty() && !path.IsEmpty() )
		m_iconpath += ';';

	m_iconpath += path;

	GetDlgItem(IDC_ICONPATH)->SetWindowText(m_iconpath);
}


void CGmeDlg::OnAddsysiconpath() 
{
	//CDirDialog dlg(NULL, icofilter, NULL);
	//dlg.DoModal();
	//CString path = dlg.GetPath();
	CString path = getDirectory();

	GetDlgItem(IDC_SYSICONPATH)->GetWindowText(m_sysiconpath);

	if( !m_sysiconpath.IsEmpty() && !path.IsEmpty() )
		m_sysiconpath += ';';

	m_sysiconpath += path;

	GetDlgItem(IDC_SYSICONPATH)->SetWindowText(m_sysiconpath);
}

void CGmeDlg::fillScripEngineList()
{
	// enumerate ScriptEngines -> m_scriptEngines ??
	// set the current to m_scriptEngine
    CATID               ourCatID;
    CComPtr<ICatInformation>    pCatInformation;
    CComPtr<IEnumCLSID>          pEnumGUID;
    HRESULT             hResult = S_OK;


	m_ScriptEgines.InsertColumn( 0, _T(""), LVCFMT_LEFT, 205, -1 );
    memcpy(&ourCatID, &CATID_ActiveScript, sizeof(ourCatID));
//    memcpy(&ourCatID, &CATID_ActiveScriptParse, sizeof(ourCatID));

	hResult = pCatInformation.CoCreateInstance(CLSID_StdComponentCategoriesMgr);
	if (FAILED(hResult)) {
		return;
	}

    hResult = pCatInformation->EnumClassesOfCategories(
        1, &ourCatID, ((ULONG) -1), NULL, &pEnumGUID);

	int index = 0;
	bool sel = false;
    while (1)
    {
        CLSID       ourCLSID;
        ULONG       lCount = 0;
		BSTR		str;

        hResult = pEnumGUID->Next(1, &ourCLSID, &lCount);

        if (lCount == 0)
            break;

        hResult = ProgIDFromCLSID(ourCLSID, &str);

        {
			CString pszString(str);
			
			LVITEM lvItem;
			lvItem.mask = LVIF_PARAM | LVIF_STATE | LVIF_TEXT;  
			if	(!m_scriptEngine.Compare(pszString))
			{
				lvItem.state = LVIS_SELECTED;
				sel = true;
			}
			else
				lvItem.state = 0;
			lvItem.stateMask = LVIS_SELECTED;

			lvItem.iItem = index++;
			lvItem.iSubItem = 0;
			lvItem.lParam = lvItem.iItem;

			lvItem.pszText = pszString.GetBuffer(pszString.GetLength());
			int ret = m_ScriptEgines.InsertItem( &lvItem );
			pszString.ReleaseBuffer();
        }
    }
	if (!sel)
	{
		LVITEM lvItem;
		lvItem.mask = LVIF_STATE; 
		lvItem.state = LVIS_SELECTED;
		lvItem.stateMask = LVIS_SELECTED;
		lvItem.iItem = 0;
		lvItem.iSubItem = 0;
		int ret1 = m_ScriptEgines.SetItem( &lvItem );
	}
	m_ScriptEgines.SetColumnWidth(0, LVSCW_AUTOSIZE);
}

BOOL CGmeDlg::OnInitDialog() 
{
	MSGTRY
	{
		ASSERT( registrar == NULL );
		COMTHROW( registrar.CoCreateInstance(OLESTR("MGA.MgaRegistrar")) );
		ASSERT( registrar != NULL );

		// Icons
		COMTHROW( registrar->get_IconPath(REGACCESS_USER, PutOut(m_iconpath)) );
		COMTHROW( registrar->get_IconPath(REGACCESS_SYSTEM, PutOut(m_sysiconpath)) );

		m_isave = m_iconpath;
		m_sysisave = m_sysiconpath;
		CRegKey accessTest;
		if (accessTest.Open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\GME"), KEY_READ | KEY_WRITE) == ERROR_ACCESS_DENIED) {
			GetDlgItem(IDC_SYSICONPATH)->EnableWindow(false);
			GetDlgItem(IDC_ADDSYSICONPATH)->EnableWindow(false);
		}

		// Multiview
		VARIANT_BOOL enabledmv;
		COMTHROW( registrar->get_ShowMultipleView(REGACCESS_USER, &enabledmv) );
		m_multipleview = (enabledmv == VARIANT_FALSE) ? FALSE : TRUE;

		// Logging
		VARIANT_BOOL enablelogging;
		COMTHROW( registrar->get_EventLoggingEnabled(REGACCESS_USER, &enablelogging) );
		m_enablelogging = (enablelogging == VARIANT_FALSE) ? FALSE : TRUE;

		// Autosave
		VARIANT_BOOL autosaveenable;
		COMTHROW( registrar->get_AutosaveEnabled(REGACCESS_USER, &autosaveenable) );
		m_autosave_enabled = (autosaveenable == VARIANT_FALSE) ? FALSE : TRUE;

		long autosavefreq;
		COMTHROW( registrar->get_AutosaveFreq(REGACCESS_USER, &autosavefreq) );
		m_autosave_freq = (int)autosavefreq;

		VARIANT_BOOL autosaveusedir;
		COMTHROW( registrar->get_AutosaveUseDir(REGACCESS_USER, &autosaveusedir) );
		m_autosave_dest = (autosaveusedir == VARIANT_FALSE) ? 0 : 1;

		COMTHROW( registrar->get_AutosaveDir(REGACCESS_USER, PutOut(m_autosave_dir)) );
		
		// External Editor
		VARIANT_BOOL extenable;
		COMTHROW( registrar->get_ExternalEditorEnabled(REGACCESS_USER, &extenable) );
		m_ext_enable = (extenable == VARIANT_FALSE) ? FALSE : TRUE;

		COMTHROW( registrar->get_ExternalEditor(REGACCESS_USER, PutOut(m_ext_editor)) );

		// Autorouter
		VARIANT_BOOL useAutoRouting;
		COMTHROW( registrar->get_UseAutoRouting(REGACCESS_USER, &useAutoRouting) );
		m_useAutoRouting = (useAutoRouting == VARIANT_FALSE) ? FALSE : TRUE;

		VARIANT_BOOL labelavoidance;
		COMTHROW( registrar->get_LabelAvoidance(REGACCESS_USER, &labelavoidance) );
		m_labelavoidance = (labelavoidance == VARIANT_FALSE) ? FALSE : TRUE;

		// Script
		COMTHROW( registrar->get_ScriptEngine(REGACCESS_USER, PutOut(m_scriptEngine)) );
		if (m_scriptEngine == _T(""))
			m_scriptEngine = _T("JScript");

		// Zoom
		// COMMENT:
		// commented out until GMEView and other classes are capable of drawing at once in ZOOM_WIDTH,Z_HEIGHT,Z_ALL mode
		//int zoomvals[] = {ZOOM_MIN, 10, 25, 50, 75, ZOOM_NO, 150, 200, 300, ZOOM_MAX,ZOOM_WIDTH,ZOOM_HEIGHT,ZOOM_ALL,0};
		int zoomvals[] = {ZOOM_MIN, 10, 25, 50, 75, ZOOM_NO, 150, 200, 300, ZOOM_MAX,0};
		fillZoomComboBox( zoomvals);

		CString set_val = getZoomValueFromReg();
		setZoomValue( set_val);

		// Send OverObject Notification?
		VARIANT_BOOL send_over_obj;
		COMTHROW( registrar->GetMouseOverNotify(REGACCESS_USER, &send_over_obj) );
		m_sendOverObj = (send_over_obj == VARIANT_FALSE) ? FALSE : TRUE;

		// format string pref
		fillFmtStrComboBox();

		// Console Timestamping
		VARIANT_BOOL console_timestamps;
		COMTHROW( registrar->GetTimeStamping( REGACCESS_USER, &console_timestamps));
		m_timeStamps = ( console_timestamps == VARIANT_FALSE) ? FALSE : TRUE;

		// History maintained
		VARIANT_BOOL history_maintained;
		COMTHROW( registrar->GetNavigation( REGACCESS_USER, &history_maintained));
		m_navigationHistory = ( history_maintained == VARIANT_FALSE) ? FALSE : TRUE;

		// Undo queue size
		fillUndoComboBox();

		// Edge smoothing mode
		fillEdgeSmoothModeComboBox();

		// Font smoothing mode
		fillFontSmoothModeComboBox();
	}
	MSGCATCH(_T("Error while initializing GmeDlg"),;)

	AutosaveControlManager();
	ExtControlManager();
	CDialog::OnInitDialog();
	fillScripEngineList();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGmeDlg::OnOK() 
{
	CDialog::OnOK();

	ASSERT( registrar != NULL );
	// Icons
	if(m_iconpath.Compare(m_isave)) COMTHROW( registrar->put_IconPath(REGACCESS_USER, PutInBstr(m_iconpath)) );
	if(m_sysiconpath.Compare(m_sysisave)) COMTHROW( registrar->put_IconPath(REGACCESS_SYSTEM, PutInBstr(m_sysiconpath)) );
	
	// Multiview
	VARIANT_BOOL enabledmv = (m_multipleview == FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
	COMTHROW( registrar->put_ShowMultipleView(REGACCESS_USER, enabledmv) );
	
	// Logging
	VARIANT_BOOL enablelogging = (m_enablelogging == FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
	COMTHROW( registrar->put_EventLoggingEnabled(REGACCESS_USER, enablelogging) );

	// Autosave
	VARIANT_BOOL autosaveenable = (m_autosave_enabled == FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
	COMTHROW( registrar->put_AutosaveEnabled(REGACCESS_USER, autosaveenable) );

	long autosavefreq = m_autosave_freq;
	COMTHROW( registrar->put_AutosaveFreq(REGACCESS_USER, autosavefreq) );

	VARIANT_BOOL autosaveusedir = (m_autosave_dest == 1) ? VARIANT_TRUE : VARIANT_FALSE;
	COMTHROW( registrar->put_AutosaveUseDir(REGACCESS_USER, autosaveusedir) );

	COMTHROW( registrar->put_AutosaveDir(REGACCESS_USER, PutInBstr(m_autosave_dir)) );

	// External Editor
	VARIANT_BOOL extenable = (m_ext_enable == FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
	COMTHROW( registrar->put_ExternalEditorEnabled(REGACCESS_USER, extenable) );

	COMTHROW( registrar->put_ExternalEditor(REGACCESS_USER, PutInBstr(m_ext_editor)) );

	// Autorouter
	VARIANT_BOOL useAutoRouting = (m_useAutoRouting == FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
	COMTHROW( registrar->put_UseAutoRouting(REGACCESS_USER, useAutoRouting) );

	VARIANT_BOOL labelavoidance = (m_labelavoidance == FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
	COMTHROW( registrar->put_LabelAvoidance(REGACCESS_USER, labelavoidance) );

	// ScriptEngine
	// m_scriptEngine <- current item from m_scriptEngines ??
	POSITION pos = m_ScriptEgines.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		int nItem = m_ScriptEgines.GetNextSelectedItem(pos);
		m_scriptEngine = m_ScriptEgines.GetItemText(nItem, 0);
	}
	COMTHROW( registrar->put_ScriptEngine(REGACCESS_USER, PutInBstr(m_scriptEngine)) );

	// Zoom
	CString val_sel = getZoomValue();
	COMTHROW( registrar->SetDefZoomLevel( REGACCESS_USER, PutInBstr(val_sel)));

	// Send Mouse Over Object Notification
	VARIANT_BOOL send_over_obj = (m_sendOverObj == FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
	COMTHROW( registrar->SetMouseOverNotify(REGACCESS_USER, send_over_obj) );

	// format string
	CString fmt_sel = getFmtStrValue();
	COMTHROW( registrar->SetRealNmbFmtStr(REGACCESS_USER, PutInBstr(fmt_sel)));

	// console timestamping
	VARIANT_BOOL console_timestamping = ( m_timeStamps == FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
	COMTHROW( registrar->SetTimeStamping( REGACCESS_USER, console_timestamping) );

	// History maintained
	VARIANT_BOOL enable_navigation = ( m_navigationHistory == FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
	COMTHROW( registrar->SetNavigation( REGACCESS_USER, enable_navigation) );

	// Undo queue size
	CString undo_queue_size = getUndoQueueSizeValue();
	COMTHROW( registrar->SetUndoQueueSize( REGACCESS_USER, PutInBstr( undo_queue_size)) );

	// Edge smoothing mode
	edgesmoothmode_enum edgeSmoothMode = getEdgeSmoothModeValue();
	COMTHROW( registrar->put_EdgeSmoothMode( REGACCESS_USER, edgeSmoothMode) );

	// Font smoothing mode
	fontsmoothmode_enum fontSmoothMode = getFontSmoothModeValue();
	COMTHROW( registrar->put_FontSmoothMode( REGACCESS_USER, fontSmoothMode) );
}



void CGmeDlg::OnAutosaveDirButton() 
{
	//CDirDialog dlg(NULL, bakfilter, NULL);

	//dlg.DoModal();
	UpdateData(TRUE);
	//m_autosave_dir = dlg.GetPath();
	m_autosave_dir = getDirectory(_T("Select output directory for autosave"));
	UpdateData(FALSE);
}

void CGmeDlg::OnAutosaveEnabled() 
{
	UpdateData(TRUE);
	AutosaveControlManager();
}

void CGmeDlg::OnAutosaveSameDir() 
{
	UpdateData(TRUE);
	AutosaveControlManager();
}

void CGmeDlg::OnAutosaveDedicatedDir() 
{
	UpdateData(TRUE);
	AutosaveControlManager();
}

void CGmeDlg::AutosaveControlManager()
{
	// NOTE: we assume, that the exchange data members are
	// holding valid data. So, please call UpdateData if needed 
	// prior calling this

	BOOL freqEnabled = FALSE;
	BOOL sameDirEnabled = FALSE;
	BOOL dedicatedDirEnabled = FALSE;
	BOOL dirEnabled = FALSE;
	BOOL dirButtonEnabled = FALSE;

	if (m_autosave_enabled) {
		freqEnabled = TRUE;
		sameDirEnabled = TRUE;
		dedicatedDirEnabled = TRUE;
		if (m_autosave_dest == 1) {
			dirEnabled = TRUE;
			dirButtonEnabled = TRUE;
		}
	}

	GetDlgItem(IDC_AUTOSAVE_FREQ)->EnableWindow(freqEnabled);
	GetDlgItem(IDC_AUTOSAVE_SAME_DIR)->EnableWindow(sameDirEnabled);
	GetDlgItem(IDC_AUTOSAVE_DEDICATED_DIR)->EnableWindow(dedicatedDirEnabled);
	GetDlgItem(IDC_AUTOSAVE_DIR)->EnableWindow(dirEnabled);
	GetDlgItem(IDC_AUTOSAVE_DIR_BUTTON)->EnableWindow(dirButtonEnabled);
}

void CGmeDlg::ExtControlManager()
{
	// NOTE: we assume, that the exchange data members are
	// holding valid data. So, please call UpdateData if needed 
	// prior calling this
	GetDlgItem(IDC_EXT_EDITOR)->EnableWindow(m_ext_enable);
	GetDlgItem(IDC_EXT_BUTTON)->EnableWindow(m_ext_enable);
}

void CGmeDlg::OnExtButton() 
{
	UpdateData(TRUE);
	CFileDialog dlg(TRUE, NULL, m_ext_editor, OFN_HIDEREADONLY, exefilter, NULL);
	if (dlg.DoModal() == IDOK) {
		m_ext_editor = dlg.GetPathName();
		UpdateData(FALSE);
	}
}

void CGmeDlg::OnExtEnable() 
{
	UpdateData(TRUE);
	ExtControlManager();
}

void CGmeDlg::fillZoomComboBox(int *list)
{
	if (!list) return;

	// supposed it is sorted, copy the values into the member
	int i;
	for( i = 0; list[i] != 0 && i < MAX_ZOOM-1; ++i)
	{
		m_zoomList[i] = list[i];
	}
	if( i <= MAX_ZOOM-1) m_zoomList[i] = 0;

	// transform the integer values to strings, and add to the comboBox
	CComboBox* zoom = 0;
	zoom = (CComboBox*)GetDlgItem(IDC_ZOOMS);
	for( int k=0; m_zoomList[k] != 0; ++k)
	{
		CString str;
		if (m_zoomList[k] > 0)
		{
			TCHAR buff[100];
			_itot(m_zoomList[k], buff, 10);
			str = buff;
			str += _T("%");
		}
		/*else	// commented out for a while (see COMMENT above)
		{
			switch (m_zoomList[k])
			{
			case ZOOM_WIDTH:
				str = m_strZWidth;
				break;
			case ZOOM_HEIGHT:
				str = m_strZHeight;
				break;
			case ZOOM_ALL:
				str = m_strZAll;
				break;
			}
		}*/
		zoom->AddString(str);
	}
}

CString CGmeDlg::getZoomValueFromReg()
{
	CString def_zoom;
	CComBSTR bs_def_zoom;
	COMTHROW( registrar->GetDefZoomLevel( REGACCESS_USER, &bs_def_zoom));
	if( bs_def_zoom) 
		CopyTo( bs_def_zoom, def_zoom);

	int level = 100;
	int zv = 0;
	int l = _stscanf( (LPCTSTR) def_zoom, _T("%d"), &zv);
	if( l == 1 && zv )
		level = zv;

	CString str;
	if (level > 0)
	{
		TCHAR buff[100];
		_itot(level, buff, 10);
		str = buff;
		str += _T("%");
	}
	/*else // for now do not accept: ZOOM_WIDTH, ZOOM_HEIGHT, ZOOM_ALL values from the registry, see COMMENT above
	{
		switch (level)
		{
		case ZOOM_WIDTH:
			str = m_strZWidth;
			break;
		case ZOOM_HEIGHT:
			str = m_strZHeight;
			break;
		case ZOOM_ALL:
			str = m_strZAll;
			break;
		}
	}*/
	
	if( str.IsEmpty())
		str = _T("100%");
	return str;
}

CString CGmeDlg::getZoomValue()
{
	CString res(_T("100"));
	CWnd* zoom = 0;
	zoom = GetDlgItem(IDC_ZOOMS);
	if (zoom)
	{
		CEdit* edit = 0;
		edit = (CEdit*)(zoom->GetDlgItem(1001));
		if (edit)
		{
			CString buff = CEditGetLine(*edit, 0);

			int zv = 0;
			/* for now accept only positive numbers, commented out, see COMMENT above
			if( strcmp( m_strZWidth, buff) == 0)
				zv = ZOOM_WIDTH;
			else if( strcmp( m_strZHeight, buff) == 0)
				zv = ZOOM_HEIGHT;
			else if( strcmp( m_strZAll, buff) == 0)
				zv = ZOOM_ALL;
			else
			{
				int l = _stscanf( buff, _T("%d"), &zv);
				if( l != 1 || zv < ZOOM_MIN || zv > ZOOM_MAX)
					zv = 100;
			}
			
			if( !zv || zv > 0 && zv < ZOOM_MIN || zv > 0 && zv > ZOOM_MAX 
				|| zv < 0 && zv != ZOOM_WIDTH && zv != ZOOM_HEIGHT && zv != ZOOM_ALL) 
				zv = 100;
			*/

			int l = _stscanf( buff, _T("%d"), &zv);
			if( l != 1 || zv < ZOOM_MIN || zv > ZOOM_MAX)
				zv = 100;

			_itot(zv, buff.GetBuffer(35), 10);
			buff.ReleaseBuffer();
			res = buff;
		}
	}
	return res;
}

void CGmeDlg::setZoomValue(CString& val )
{
	CWnd* zoom = 0;
	zoom = GetDlgItem(IDC_ZOOMS);
	if (zoom) 
	{
		CEdit* edit = 0;
		edit = (CEdit*)(zoom->GetDlgItem(1001));
		if (edit) 
		{
			edit->ReplaceSel( val);
		}
	}
}

void CGmeDlg::fillFmtStrComboBox()
{
	CComboBox*  fmtBox = 0;
	fmtBox = (CComboBox*)GetDlgItem(IDC_CMBFMTSTRS);
	fmtBox->AddString( m_strFmtStrg);
	fmtBox->AddString( m_strFmtStrf);
	fmtBox->AddString( m_strFmtStre);
	fmtBox->AddString( m_strFmtStrE);
	fmtBox->AddString( m_strFmtStrg2);
	fmtBox->AddString( m_strFmtStrf2);

	CString regv = getFmtStrFromReg();
	int pos = fmtBox->FindStringExact( -1, regv);
	if( pos == CB_ERR)
		fmtBox->InsertString( -1, regv);
	fmtBox->SelectString( -1, regv);
}

CString CGmeDlg::getFmtStrFromReg()
{
	CString fmt_str;
	CComBSTR bs_fmt_str;
	COMTHROW( registrar->GetRealNmbFmtStr( REGACCESS_USER, &bs_fmt_str));
	if( bs_fmt_str) 
		CopyTo( bs_fmt_str, fmt_str);

	if( fmt_str.IsEmpty())
		fmt_str = m_strFmtStrg2;

	return fmt_str;
}

CString CGmeDlg::getFmtStrValue()
{
	CString res( m_strFmtStrg2);
	CWnd* combo = 0;
	combo = GetDlgItem(IDC_CMBFMTSTRS);
	if( combo)
	{
		CEdit* edit = 0;
		edit = (CEdit*)( combo->GetDlgItem(1001));
		if( edit)
		{
			res = CEditGetLine(*edit, 0);
		}
	}
	return res;
}

void CGmeDlg::fillUndoComboBox()
{
	CComboBox* sz_list = 0;
	sz_list = (CComboBox*)GetDlgItem(IDC_UNDOSIZE);
	sz_list->AddString(_T("1"));
	sz_list->AddString(_T("10"));

	CString regv = getUndoQueueSizeFromReg();
	int pos = sz_list->FindStringExact( -1, regv);
	if( pos == CB_ERR)
		sz_list->InsertString( -1, regv);
	sz_list->SelectString( -1, regv);

}

CString CGmeDlg::getUndoQueueSizeFromReg()
{
	CString queuesize;
	COMTHROW( registrar->GetUndoQueueSize( REGACCESS_USER, PutOut( queuesize)) );
	if( queuesize.IsEmpty())
		queuesize = _T("10");

	return queuesize;
}

CString CGmeDlg::getUndoQueueSizeValue()
{
	CString res( _T("10"));
	CWnd* combo = 0;
	combo = GetDlgItem(IDC_UNDOSIZE);
	if( combo)
	{
		CEdit* edit = 0;
		edit = (CEdit*)( combo->GetDlgItem(1001));
		if (edit)
		{
			CString buff = CEditGetLine(*edit, 0);
			int value = _ttoi(buff);
			if (value > 0 && value < 100) // above 0 and below 100
				res = buff;
		}
	}
	return res;
}

void CGmeDlg::fillEdgeSmoothModeComboBox()
{
	CComboBox* modeBox = 0;
	modeBox = (CComboBox*)GetDlgItem(IDC_EDGESMOOTHMODE);
	modeBox->AddString(m_strESStrD);
	modeBox->AddString(m_strESStrHS);
	modeBox->AddString(m_strESStrHQ);

	CString regv = getEdgeSmoothModeStrFromReg();
	int pos = modeBox->FindStringExact(-1, regv);
	if (pos == CB_ERR)
		modeBox->InsertString(-1, regv);
	modeBox->SelectString(-1, regv);
}

CString CGmeDlg::getEdgeSmoothModeStrFromReg()
{
	edgesmoothmode_enum edgeSmoothMode;
	COMTHROW(registrar->get_EdgeSmoothMode(REGACCESS_USER, &edgeSmoothMode));
	CString reg_str;
	switch(edgeSmoothMode) {
		case EdgeSmooth_NoSmooth:			reg_str = m_strESStrD;	break;
		case EdgeSmooth_HighSpeedMode:		reg_str = m_strESStrHS;	break;
		default:
		case EdgeSmooth_HighQualityMode:	reg_str = m_strESStrHQ;	break;
	}
	return reg_str;
}

edgesmoothmode_enum CGmeDlg::getEdgeSmoothModeValue()
{
	CString res(m_strESStrHQ);
	CWnd* combo = 0;
	combo = GetDlgItem(IDC_EDGESMOOTHMODE);
	if (combo)
	{
		CEdit* edit = 0;
		edit = (CEdit*)(combo->GetDlgItem(1001));
		if (edit)
		{
			res = CEditGetLine(*edit, 0);
		}
	}
	edgesmoothmode_enum edgeSmoothMode = EdgeSmooth_HighQualityMode;
	if (res == m_strESStrD)
		edgeSmoothMode = EdgeSmooth_NoSmooth;
	else if (res == m_strESStrHS)
		edgeSmoothMode = EdgeSmooth_HighSpeedMode;
	else if (res == m_strESStrHQ)
		edgeSmoothMode = EdgeSmooth_HighQualityMode;
	return edgeSmoothMode;
}

void CGmeDlg::fillFontSmoothModeComboBox()
{
	CComboBox* modeBox = 0;
	modeBox = (CComboBox*)GetDlgItem(IDC_FONTSMOOTHMODE);
	modeBox->AddString(m_strFSStrSD);
	modeBox->AddString(m_strFSStrSBPPGF);
	modeBox->AddString(m_strFSStrSBPP);
	modeBox->AddString(m_strFSStrAAGF);
	modeBox->AddString(m_strFSStrAA);
	modeBox->AddString(m_strFSStrCTGF);

	CString regv = getFontSmoothModeStrFromReg();
	int pos = modeBox->FindStringExact(-1, regv);
	if (pos == CB_ERR)
		modeBox->InsertString(-1, regv);
	modeBox->SelectString(-1, regv);
}

CString CGmeDlg::getFontSmoothModeStrFromReg()
{
	fontsmoothmode_enum fontSmoothMode;
	COMTHROW(registrar->get_FontSmoothMode(REGACCESS_USER, &fontSmoothMode));
	CString reg_str;
	switch(fontSmoothMode) {
		case FontSmooth_SystemDefault:				reg_str = m_strFSStrSD;		break;
		case FontSmooth_SingleBitPerPixelGridFit:	reg_str = m_strFSStrSBPPGF;	break;
		case FontSmooth_SingleBitPerPixel:			reg_str = m_strFSStrSBPP;	break;
		case FontSmooth_AntiAliasGridFit:			reg_str = m_strFSStrAAGF;	break;
		default:
		case FontSmooth_AntiAlias:					reg_str = m_strFSStrAA;		break;
		case FontSmooth_ClearTypeGridFit:			reg_str = m_strFSStrCTGF;	break;
	}
	return reg_str;
}

fontsmoothmode_enum CGmeDlg::getFontSmoothModeValue()
{
	CString res(m_strFSStrAA);
	CWnd* combo = 0;
	combo = GetDlgItem(IDC_FONTSMOOTHMODE);
	if (combo)
	{
		CEdit* edit = 0;
		edit = (CEdit*)(combo->GetDlgItem(1001));
		if (edit)
		{
			res = CEditGetLine(*edit, 0);
		}
	}
	fontsmoothmode_enum fontSmoothMode = FontSmooth_AntiAlias;
	if (res == m_strFSStrSD)
		fontSmoothMode = FontSmooth_SystemDefault;
	else if (res == m_strFSStrSBPPGF)
		fontSmoothMode = FontSmooth_SingleBitPerPixelGridFit;
	else if (res == m_strFSStrSBPP)
		fontSmoothMode = FontSmooth_SingleBitPerPixel;
	else if (res == m_strFSStrAAGF)
		fontSmoothMode = FontSmooth_AntiAliasGridFit;
	else if (res == m_strFSStrAA)
		fontSmoothMode = FontSmooth_AntiAlias;
	else if (res == m_strFSStrCTGF)
		fontSmoothMode = FontSmooth_ClearTypeGridFit;
	return fontSmoothMode;
}

void CGmeDlg::OnCbnSelchangeZooms()
{
	// TODO: Add your control notification handler code here
}

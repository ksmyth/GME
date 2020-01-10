// GMEApp.h : main header file for the GME application
//

#if !defined(AFX_GME_H__BD235B4A_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_)
#define AFX_GME_H__BD235B4A_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include <set>
#include <map>
#include "GMELib.h"

#define ID_FILE_PLUGINMENU 40000


#include "RecentConnStrList.h"

/////////////////////////////////////////////////////////////////////////////
// CGMEApp:
// See GMEApp.cpp for the implementation of this class
//

extern LONG GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata);
extern HINSTANCE GotoURL(LPCTSTR url, int showcmd);


struct IMgaProject;
struct IMgaMetaProject;
class CGuiMetaProject;
class CGuiPreferenceManager;

class CGMEApp : public CWinAppEx
{
	friend class CGMEOLEApp;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	CGMEApp();
	virtual ~CGMEApp();

	CMultiDocTemplate* pDocTemplate;
	CComPtr<IMgaProject> mgaProject;
	CComPtr<IMgaClient> mgaClient;
	CComPtr<IMgaMetaProject> mgaMetaProject;
//  dynamic menus
	CStringArray plugins, interpreters, addons;
	CStringArray pluginTooltips, interpreterTooltips, addonToolTips;
	CStringArray pluginOnlineHelp, interpreterOnlineHelp, addonOnlineHelp;
	CStringArray pluginOfflineHelp, interpreterOfflineHelp, addonOfflineHelp;
	CString paradigmOnlineHelp, paradigmOfflineHelp;
	bool dynmenus_need_refresh;

	// Application look
	UINT  m_nAppLook;
	CMFCToolBarImages m_userImages;
	CComPtr<IMgaComponentEx> mgaConstMgr;
	CGuiMetaProject *guiMetaProject;
	CString projectName;
	CString bitmapPath, projectDir, paradigmDir;
	bool	multipleView;
	bool	useAutoRouting;
	bool	labelAvoidance;

	HACCEL m_GMEView_hAccel;
	CRecentConnStrList m_RecentProjectList;
	bool bNoProtect;   // do not catch application errors
	inline bool isMultiUserProj() const { return proj_type_is_xmlbackend; }
	inline bool isMgaProj()       const { return proj_type_is_mga; }
	inline CString connString() const { return currentConnection; }

	void getMgaPaths(CString& filename, CString& dirname) {
		ASSERT(isMgaProj());
		ASSERT(mgaProject);
		CString conn = connString();
		const TCHAR* zsConn = conn;
		zsConn += 4; // skip MGA=
		GetFullPathName(zsConn, filename, dirname);
	}

	BOOL OpenCommandLineProject();
	BOOL ShowWelcomeWindow();

private:
	virtual int Run();
	void EmergencySave();
	bool proj_type_is_mga;
	bool proj_type_is_xmlbackend;
	bool abort_on_close;
	CString currentConnection;

	bool autosaveEnabled;
	bool autosaveUseDir;
	CString autosaveDir;
	int  autosaveFreq;
	CString defZoomLev;
	bool mouseOverNotify;
	CString realFmtStr;
	bool maintainHistory;

	//
	// component filter related fields
	//

	// the state of component filtering (not preserved through GME sessions)
	bool m_compFilterOn;

	// will store those command ids, which are disabled by the user 
	// through the ActivateComponent
	typedef std::set<unsigned int> ONE_ID_LIST;
	ONE_ID_LIST m_vecDisabledPlugIns;
	ONE_ID_LIST m_vecDisabledComps;

	typedef std::set<CString> ONE_COMP_LIST;
	typedef std::map<CString, ONE_COMP_LIST > ONE_COMP_MAP;
	// map of kind as key, and components registered for it
	ONE_COMP_MAP m_compsOfKind;

	//
	// component filter related private methods
	//
	void ClearDisabledComps();

	// Updates the plugins and addons on the component toolbar
	void CGMEApp::UpdateComponentToolbar();

	//
	// component filter related public methods
	//
public:
	// called from CGMEOLEApp:
	void SetCompFiltering( bool pOn);
	bool GetCompFiltering();
	void DisableComp(const CString& pCompToFind, bool pbHide);
	void DisableCompForKinds( const CString& pComp, const CString& pKindSeq);
	// called from CGMEView:
	void UpdateCompList4CurrentKind( const CString& kind);

	static const TCHAR * m_no_model_open_string;

	// called from CGMEOLEAPP:
	bool SetWorkingDirectory( LPCTSTR pPath);

public:
	void ImportDroppedFile(const CString& fname);
	void RegisterDroppedFile( const CString& fname, bool userReg = true);
	bool SafeCloseProject();
	void CloseProject(bool updateStatusBar = true, bool force_abort=false);
	void OpenProject(const CString &conn);
	void Importxml(CString fullPath, CString fname, CString ftitle);
	void SaveProject(const CString &conn);
	void CreateProject(const CString &metaname, const CString &dataconn);
	void UpdateProjectName(bool retrievePath = false);
	void UpdateMainFrameTitle(const CString& projName, bool retrievePath = false);
	void AfterOpenOrCreateProject(const CString &conn);
	void UpdateComponentLists(bool restart_addons =  false);
	void UpdateDynMenus(CMenu *filemenu);
	void FindConstraintManager();
	void RunComponent(const CString &progid);
	void exit() { OnAppExit(); }
	void consoleMessage( const CString& p_msg, short type);

	void Autosave();
	CString getDefZoomLev() { return defZoomLev; }
	inline bool isMouseOverNotifyEnabled() { return mouseOverNotify; }
	inline bool isHistoryEnabled() { return maintainHistory; }
	inline CString getRealFmtStr() { return realFmtStr; }

	// Both empty: reset on close, only one empty: no change to that name.
	void ChangedProjectConnStrings();
	void GetSettings();

	void UpdateMainTitle(bool retrievePath = false);
	bool IsUndoPossible(void) const;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMEApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual void OnAppExit();
	virtual BOOL SaveAllModified();
	//}}AFX_VIRTUAL

	virtual LRESULT ProcessWndProcException(CException* e, const MSG* pMsg)
	{
		// TODO send with CrashRpt (if we didn't send it with CGMEView::OnWndMsg
		// but the stack is gone, so a report here is much less useful
		return __super::ProcessWndProcException(e, pMsg);
	}

	void OnUniquePrintSetup();
// Implementation
		// Server object for document creation
	//{{AFX_MSG(CGMEApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	afx_msg void OnFileNew();
	afx_msg void OnUpdateRecentProjectMenu(CCmdUI* pCmdUI);
	afx_msg BOOL OnOpenRecentProject(UINT nID);
	afx_msg void OnUpdateFileCloseproject(CCmdUI* pCmdUI);
	afx_msg void OnFileCloseproject();
	afx_msg void OnUpdateFileNew(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileOpen(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileExportxml(CCmdUI* pCmdUI);
	afx_msg void OnFileExportxml();
	afx_msg void OnFileImportxml();
	afx_msg void OnUpdateEditProjectproperties(CCmdUI* pCmdUI);
	afx_msg void OnEditProjectproperties();
	afx_msg void OnFileSettings();
	afx_msg void OnFileClearLocks();
	afx_msg void OnEditClearUndo();
	afx_msg void OnUpdateEditClearUndo(CCmdUI* pCmdUI);
	afx_msg void OnHelpContents();
	afx_msg void OnFileCheckall();
	afx_msg void OnUpdateFileCheckall(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
	afx_msg void OnFileAbortProject();
	afx_msg void OnUpdateFileAbortProject(CCmdUI* pCmdUI);
	afx_msg void OnFileRegcomponents();
	afx_msg void OnFileRegparadigms();
	afx_msg void OnUpdateFileRegcomponents(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileXMLUpdate(CCmdUI* pCmdUI);
	afx_msg void OnFileXMLUpdate();
	afx_msg void OnRunPlugin(UINT nID);
	afx_msg void OnRunInterpreter(UINT nID);
	afx_msg void OnRunComponentHelp(UINT nID);
	afx_msg void OnRunParadigmHelp(UINT nID);
	afx_msg void OnUpdateFilePluginX(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileInterpretX(CCmdUI* pCmdUI);
	afx_msg void OnFileDisplayConstraints();
	afx_msg void OnUpdateFileDisplayConstraints(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnFocusBrowser();
	afx_msg void OnFocusInspector();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CGMEApp theApp;

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CDLLInfoDlg dialog

class CDLLInfoDlg : public CDialog
{
// Construction
public:
	CDLLInfoDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDLLInfoDlg)
	enum { IDD = IDD_DLL_DIALOG };
	CListCtrl	m_DLLList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDLLInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDLLInfoDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GME_H__BD235B4A_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_)

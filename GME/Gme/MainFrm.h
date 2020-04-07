// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__BD235B4F_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_)
#define AFX_MAINFRM_H__BD235B4F_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GMEPartBrowser.h"
#include "GMEBrowser.h"
#include "GMEObjectInspector.h"
#include "GMEConsole.h"
#include "GMESearch.h"
#include "GMEPanningWindow.h"
#include "GMEOLEApp.h"

struct CGMEMFCTabCtrl : public CMFCTabCtrl
{
	virtual CWnd* GetLastVisibleTab(int& iTabNum)
	{
		if (m_bActivateLastActiveTab)
		{
			iTabNum = m_iActiveTab;
			return GetTabWnd(m_iActiveTab);
		}
		// this is a hack to fix the tab behavior when you close the current tab
		// Activate the next tab (same tab index), except if the last tab was closed. In that case, activate the new last tab (index - 1)
		EnableActivateLastVisible(TRUE);
		if (GetTabsNum() == m_iActiveTab)
		{
			m_iActiveTab--;
		}
		iTabNum = m_iActiveTab;
		return GetTabWnd(m_iActiveTab);
	}

	void _SetLastActiveTab()
	{
		m_iLastActiveTab = m_iActiveTab;
	}

	void EnableActivateLastVisible(BOOL enable)
	{
		m_bActivateLastVisibleTab = enable;
	}
};

// Tooltip helper for components
class CComponentBar : public CMFCToolBar {
	DECLARE_DYNCREATE(CComponentBar)
	afx_msg BOOL OnTT(UINT, NMHDR * pNMHDR, LRESULT * );
	DECLARE_MESSAGE_MAP()
};

class CMainFrame : public CMDIFrameWndEx
{
	friend void CGMEApp::UpdateComponentLists(bool restart_addons);
	friend class CGMEOLEApp;
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:
	static CMainFrame *theInstance;

	int timePaneNo;
	int paradigmPaneNo;
	int modePaneNo;
	int zoomPaneNo;

// Operations
public:
	void ActivateView(CView *view);
	void CreateNewView(CView *view, CComPtr<IMgaModel>& model);
	void DestroyView(CView *view);
	void WriteStatusZoom(int zoomPct);
	void WriteStatusParadigm(const CString& txt);
	void WriteStatusMode(const CString& txt);
	void WriteStatusText(int pane, const CString& txt);
	void UpdateTitle(LPCTSTR title);
	void SetPartBrowserMetaModel(CGuiMetaModel* meta)	{ m_partBrowser.SetMetaModel(meta); }
	void SetPartBrowserBg(COLORREF bgColor)				{ m_partBrowser.SetBgColor(bgColor); m_partBrowser.RePaint(); }
	void RePaintPartBrowser()							{ m_partBrowser.RePaint(); }
	void ChangePartBrowserAspect(int ind)				{ m_partBrowser.ChangeAspect(ind); }
	void CyclePartBrowserAspect()						{ m_partBrowser.CycleAspect(); }
	void SetGMEViewMetaModel(CGuiMetaModel* meta);
	void ChangeGMEViewAspect(int ind);
	void CycleGMEViewAspect();
	void GMEViewInvalidate();
	BOOL VerifyBarState(LPCTSTR lpszProfileName);
	void ShowObjectInspector();
	void ShowFindDlg();
	void HideFindDlg();

	void StartAutosaveTimer(int secs);
	void StopAutosaveTimer();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();

	void setGmeOleApp( CGMEOLEApp * the_ole_app);
	void setMgaProj();
	void clearGmeOleApp();
	void clearMgaProj();
	CGMEOLEApp *       mGmeOleApp;

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CGMEPanningWindow	m_panningWindow;
	CGMEConsole			m_console;

	CComponentBar		m_wndComponentBar;
protected:  // control bar embedded members
	CMFCStatusBar		m_wndStatusBar;
	CMFCToolBar			m_wndToolBarMain;
	CMFCToolBar			m_wndToolBarModeling;
	CMFCToolBar			m_wndToolBarWins;
	CMFCMenuBar			m_wndMenuBar;

	CMFCToolBar			m_wndModeBar;
	CMFCToolBar			m_wndNaviBar;




//	CDialogBar			m_wndDlgBar;
	CGMEPartBrowser		m_partBrowser;
	CGMEBrowser			m_browser;
	CGMEObjectInspector	m_objectInspector;
	CGMESearch			m_search;

	UINT_PTR			m_autosaveTimerID;

// Generated message map functions
protected:
	afx_msg void OnUpdateTime(CCmdUI* pCmdUI);
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void OnViewFullScreen();
	afx_msg void OnEditSearch();
	afx_msg void OnUpdateEditSearch(CCmdUI* pCmdUI);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnViewMultiUserRefreshSourceControl();
	afx_msg void OnUpdateViewMultiUserRefreshSourceControl(CCmdUI* pCmdUI);
	afx_msg void OnViewMultiUserActiveUsers();
	afx_msg void OnUpdateViewMultiUserActiveUsers(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewMultiUserSubversion(CCmdUI* pCmdUI);
	afx_msg void OnViewClearConsole();
	afx_msg void OnUpdateViewClearConsole( CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSettings(CCmdUI* pCmdUI);

	template<class paneclass, typename paneclass CMainFrame::* const m_pane> afx_msg void OnViewPane() 
	{
		(this->*m_pane).ShowPane(!(this->*m_pane).IsVisible(), FALSE, FALSE);
	}
	template<class paneclass, typename paneclass CMainFrame::* const m_pane> afx_msg void OnUpdateViewPane(CCmdUI* pCmdUI) 
	{
		pCmdUI->Enable();
		pCmdUI->SetCheck((this->*m_pane).IsVisible());
	}

	afx_msg void OnUpdateWindowNew(CCmdUI* pCmdUI);
	afx_msg void OnClose();
public:
	afx_msg void OnDropFiles(HDROP p_hDropInfo);
protected:
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);

	afx_msg void OnBtnBack();
	afx_msg void OnBtnHome();
	afx_msg void OnUpdateBtnBack(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBtnHome(CCmdUI* pCmdUI);

	afx_msg void OnWindowManager();
	afx_msg void OnViewCustomize();

	afx_msg void OnMdiMoveToNextGroup();
	afx_msg void OnMdiMoveToPrevGroup();
	afx_msg void OnMdiNewHorzTabGroup();
	afx_msg void OnMdiNewVertGroup();
	LRESULT OnToolbarCreateNew(WPARAM wp,LPARAM lp);
	afx_msg BOOL OnBarCheck(UINT nID);
	afx_msg void OnUpdateControlBarMenus(CCmdUI* pCmdUI);
	afx_msg void OnWindowNewhorizontaltabgroup();
	afx_msg void OnWindowNewverticaltabgroup();
	afx_msg void OnWindowMovetoprevioustabgroup();
	afx_msg void OnWindowMovetonexttabgroup();
	afx_msg void OnUpdateWindowNewhorizontaltabgroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindowNewverticaltabgroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindowMovetoprevioustabgroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateWindowMovetonexttabgroup(CCmdUI* pCmdUI);
	afx_msg LRESULT OnGetTabTooltip(WPARAM wp, LPARAM lp);

	//}}AFX_MSG
	LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
private:
	BOOL InitStatusBar(UINT *pIndicators, int nSize, int nSeconds);
	int CreateToolBars();
	void OnApplicationLook(UINT id);
//	IMgaObject*	modelFindByPath(IMgaModel *model,  const wstring& strPath);
//	IMgaObject*	folderFindByPath(IMgaFolder* folder, const wstring& strPath);

public:
	void ShowNavigationAndModeToolbars(bool isVisible);

	void EnableActivateLastActive(BOOL enable)
	{
		m_wndClientArea.GetMDITabs().EnableActivateLastActive(enable);
	}

	void CheckForOffscreenPanes();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__BD235B4F_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_)

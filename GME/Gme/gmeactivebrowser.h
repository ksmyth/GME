#if !defined(AFX_GMEACTIVEBROWSER_H__E2FAA477_405B_4403_9AF4_4D21ECB0EECC__INCLUDED_)
#define AFX_GMEACTIVEBROWSER_H__E2FAA477_405B_4403_9AF4_4D21ECB0EECC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Machine generated IDispatch wrapper class(es) created by Microsoft Visual C++

// NOTE: Do not modify the contents of this file.  If this class is regenerated by
//  Microsoft Visual C++, your modifications will be overwritten.

/////////////////////////////////////////////////////////////////////////////
// CGMEActiveBrowser wrapper class

#pragma warning(disable: 4100) // unreferenced formal parameter warning

class CGMEActiveBrowser : public CWnd
{
protected:
	DECLARE_DYNCREATE(CGMEActiveBrowser)
public:
	CLSID const& GetClsid()
	{
		static CLSID const clsid
			= { 0xdd4f2deb, 0xd064, 0x4844, { 0x9e, 0xe1, 0x4, 0x67, 0x3c, 0x87, 0x2e, 0x7b } };
		return clsid;
	}
	virtual BOOL Create(LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect,
		CWnd* pParentWnd, UINT nID,
		CCreateContext* pContext = NULL)
	{ return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID); }


    BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect, CWnd* pParentWnd, UINT nID,
		CFile* pPersist = NULL, BOOL bStorage = FALSE,
		BSTR bstrLicKey = NULL)
	{ return CreateControl(GetClsid(), lpszWindowName, dwStyle, rect, pParentWnd, nID,
		pPersist, bStorage, bstrLicKey); }

// Attributes
public:

// Operations
public:
	short SetCurrentProject(LPUNKNOWN pMgaProject);
	short CloseCurrentProject();
	void RefreshAll();
	LPUNKNOWN GetSelectedMgaObjects();
	void FocusItem(BSTR Id);
};

#pragma warning(default: 4100) // unreferenced formal parameter warning

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMEACTIVEBROWSER_H__E2FAA477_405B_4403_9AF4_4D21ECB0EECC__INCLUDED_)

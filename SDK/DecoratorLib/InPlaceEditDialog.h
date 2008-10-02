#if !defined(AFX_INPLACEEDITDIALOG_H__0D3F6303_5878_489e_ADB0_FFBF2A4CA609__INCLUDED_)
#define AFX_INPLACEEDITDIALOG_H__0D3F6303_5878_489e_ADB0_FFBF2A4CA609__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InPlaceEditDialog.h : header file
//

#include "stdafx.h"
#include "Resource.h"

// CInPlaceEditDialog dialog

class CInPlaceEditDialog : public CDialog
{
	DECLARE_DYNAMIC(CInPlaceEditDialog)

public:
	CInPlaceEditDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInPlaceEditDialog();

// Dialog Data
	//{{AFX_DATA(CInPlaceEditDialog)
	enum { IDD = IDD_INPLACEEDITDIALOG };
	CString	m_Text;
	//}}AFX_DATA

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	// Generated message map functions
	//{{AFX_MSG(CInPlaceEditDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnEnChangeTextedit();
	//}}AFX_MSG

	CWnd*	m_parentCWnd;
	HWND	m_parentHWnd;
	CRect	m_initialRect;
	CFont*	m_font;
	long	m_zoomVal;

	void	SetParentWnd(HWND parentWnd, CWnd* parentCWnd) { m_parentHWnd = parentWnd; m_parentCWnd = parentCWnd; };
	void	SetInitialRect(const CRect& rect) { m_initialRect = rect; };
	void	SetFont(CFont* font) { m_font = font; };
	CString GetText() const { return m_Text; };
	void	SetText(const CString& text) { m_Text = text; };

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CInPlaceEditDialog)
    public:
    virtual void EndDialog(int nResult);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

	CRect	GetWindowSizeFromResource(void) const;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPLACEEDITDIALOG_H__0D3F6303_5878_489e_ADB0_FFBF2A4CA609__INCLUDED_)

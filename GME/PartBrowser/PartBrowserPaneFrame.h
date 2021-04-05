#if !defined(AFX_PARTBROWSERPANEFRAME_H__B9443F66_C8F4_11D3_91EB_00104B98EAD9__INCLUDED_)
#define AFX_PARTBROWSERPANEFRAME_H__B9443F66_C8F4_11D3_91EB_00104B98EAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PartBrowserPaneFrame.h : header file
//

#include "StdAfx.h"
#include "PartBrowserPane.h"
#include "Resource.h"

/////////////////////////////////////////////////////////////////////////////
// CPartBrowserPaneFrame dialog

class CPartBrowserPaneFrame : public CDialog
{
// Construction
public:
	CPartBrowserPaneFrame(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPartBrowserPaneFrame)
	enum { IDD = IDD_PARTBROWSERFRAME_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPartBrowserPaneFrame)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CPartBrowserPane pane;

	int logicalHeight;
	int pageHeight;
	int scrollPos;
	const int vScrollWidth;
	int lineSize;
	const int pageSize;

public:
	void Resize(RECT r);
	void SetScrollBar();

	// Get/Set methods
	CPartBrowserPane&	GetPane(void);
	void				SetLogicalHeight(int logHeight);
	void				SetPageHeight(int pgHeight);
	int					GetScrollPosition(void);
	void				SetScrollPosition(int scrollPosition);

protected:

	// Generated message map functions
	//{{AFX_MSG(CPartBrowserPaneFrame)
	virtual BOOL OnInitDialog();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT fFlags, short zDelta, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARTBROWSERPANEFRAME_H__B9443F66_C8F4_11D3_91EB_00104B98EAD9__INCLUDED_)

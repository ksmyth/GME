#if !defined(AFX_PARTBROWSERPANE_H__B9443F67_C8F4_11D3_91EB_00104B98EAD9__INCLUDED_)
#define AFX_PARTBROWSERPANE_H__B9443F67_C8F4_11D3_91EB_00104B98EAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PartBrowserPane.h : header file
//

#include "StdAfx.h"
#include <vector>

class CDecoratorEventSink;


/////////////////////////////////////////////////////////////////////////////
// CPartBrowserPane window

class CPartBrowserPane : public CWnd
{
// Construction
public:
	struct PartWithDecorator {
		CComPtr<IMgaMetaPart>	part;
		CComPtr<IMgaDecorator>	decorator;
		CDecoratorEventSink*	decorEventSink;
	};

	CPartBrowserPane();
	virtual ~CPartBrowserPane();

private:
// Operations
	CComBSTR	GetDecoratorProgId(CComPtr<IMgaMetaFCO> metaFCO);
	bool		IsPartDisplayable(CComPtr<IMgaMetaPart> metaPart, bool debugPrint = false);
	bool		FindObject(CPoint &pt, PartWithDecorator& pdt);
	void		CreateDecorators(CComPtr<IMgaMetaParts> metaParts);
	void		DestroyDecorators(void);

// Attributes
protected:
	CComPtr<IMgaProject>	mgaProject;
	CComPtr<IMgaMetaModel>	mgaMetaModel;
	COLORREF				backgroundColor;
	std::vector<std::vector<PartWithDecorator> >	pdts;
	int						currentAspectIndex;
	CSize					maxSize;
	bool					omitPaintMessages;
	CFont					txtMetricFont;

public:
// Operations
	void Resize(CRect r);

// Get/Set methods
	void SetCurrentProject(CComPtr<IMgaProject> project);
	void SetMetaModel(CComPtr<IMgaMetaModel> meta);
	void SetBgColor(COLORREF bgColor);
	void ChangeAspect(int index);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPartBrowserPane)
	//}}AFX_VIRTUAL

// Implementation

	// Generated message map functions
protected:
	//{{AFX_MSG(CPartBrowserPane)
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARTBROWSERPANE_H__B9443F67_C8F4_11D3_91EB_00104B98EAD9__INCLUDED_)

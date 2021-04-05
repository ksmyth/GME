#if !defined(AFX_PARTBROWSERPANE_H__B9443F67_C8F4_11D3_91EB_00104B98EAD9__INCLUDED_)
#define AFX_PARTBROWSERPANE_H__B9443F67_C8F4_11D3_91EB_00104B98EAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PartBrowserPane.h : header file
//

#include "StdAfx.h"
#include <vector>
#include <Gdiplus.h>


/////////////////////////////////////////////////////////////////////////////
// PartWithDecorator helper class

class PartWithDecorator
{
public:
	CComPtr<IMgaMetaPart>						part;
	CString										name;	// Calculated, cached for speedup
	CComPtr<IMgaDecorator>						decorator;
	CComPtr<IMgaElementDecorator>				newDecorator;
#if _MSC_VER >= 1900
	PartWithDecorator& operator=(const PartWithDecorator& that) = delete;
#endif
	PartWithDecorator& operator=(PartWithDecorator&& that)
	{
		if (this != &that)
		{
			this->part = std::move(that.part);
			this->name = std::move(that.name);
			this->decorator = std::move(that.decorator);
			this->newDecorator = std::move(that.newDecorator);
		}
		return *this;
	}
	PartWithDecorator(PartWithDecorator&& that)
		: part(std::move(that.part)),
		name(std::move(that.name)),
		decorator(std::move(that.decorator)),
		newDecorator(std::move(that.newDecorator))
	{
	}
	PartWithDecorator() {}
};


/////////////////////////////////////////////////////////////////////////////
// CPartBrowserPane window

class CPartBrowserPane : public CWnd
{
// Construction
public:
	CPartBrowserPane();
	virtual ~CPartBrowserPane();

private:
// Operations
	CComBSTR	GetDecoratorProgId(IMgaMetaFCO* metaFCO);
	bool		IsPartDisplayable(CComPtr<IMgaMetaPart> metaPart);
	bool		FindObject(CPoint &pt, const PartWithDecorator*& pdt);
	void		CreateDecorators(CComPtr<IMgaMetaParts> metaParts);
	void		DestroyDecorators(void);
	int logPixelsY;
	HRESULT GetDecoratorSize(IMgaDecorator* decorator, long &sizeX, long &sizeY) {
		HRESULT hr = decorator->GetPreferredSize(&sizeX, &sizeY);
		sizeX = MulDiv(sizeX, logPixelsY, 96);
		sizeY = MulDiv(sizeX, logPixelsY, 96);
		return hr;
	}
	HRESULT GetDecoratorSize(const PartWithDecorator& decorator, long &sizeX, long &sizeY) {
		return GetDecoratorSize(decorator.decorator, sizeX, sizeY);
	}

// Attributes
protected:
	CComPtr<IMgaProject>							mgaProject;
	CComPtr<IMgaMetaModel>							mgaMetaModel;
	COLORREF										backgroundColor;
	std::vector<std::vector<PartWithDecorator> >	pdts;
	int												currentAspectIndex;
	CSize											maxSize;
	bool											omitPaintMessages;
	CFont											txtMetricFont;
	static Gdiplus::SmoothingMode					m_eEdgeAntiAlias;		// Edge smoothing mode
	static Gdiplus::TextRenderingHint				m_eFontAntiAlias;		// Text renndering hint mode

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

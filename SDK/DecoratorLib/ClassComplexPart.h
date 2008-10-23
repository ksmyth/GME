//################################################################################################
//
// Class complex part class (decorator part)
//	ClassComplexPart.h
//
//################################################################################################

#ifndef __CLASSCOMPLEXPART_H_
#define __CLASSCOMPLEXPART_H_


#include "StdAfx.h"
#include "VectorPart.h"

#include "ClassLabelPart.h"
#include "StereoLabelPart.h"
#include "AttributePart.h"
#include "MaskedBitmapPart.h"


namespace DecoratorSDK {

//################################################################################################
//
// CLASS : ClassComplexPart
//
//################################################################################################

class ClassComplexPart: public VectorPart
{
protected:
	ClassLabelPart*				m_LabelPart;
	StereoLabelPart*			m_StereotypePart;
	std::vector<AttributePart*>	m_AttributeParts;
	MaskedBitmapPart*			m_copySignPart;

	long						m_SeparatorLoc;
	CSize						m_calcSize;
	COLORREF					m_crAttributeText;
	long						m_lMaxTextWidth;
	long						m_lMaxTextHeight;
	long						m_lMinTextWidth;
	long						m_lMinTextHeight;

	long						m_DecoratorMarginX;
	long						m_DecoratorMarginY;
	long						m_DecoratorGapY;
	long						m_DecoratorMinAttrSize;

	std::vector<DecoratorSDK::CoordCommand*>	m_coordCommands;
public:
	ClassComplexPart(PartBase* pPart, CComPtr<IMgaNewDecoratorEvents> eventSink);
	virtual ~ClassComplexPart();

// =============== resembles IMgaNewDecorator
public:
	virtual void			Initialize			(CComPtr<IMgaProject>& pProject, CComPtr<IMgaMetaPart>& pPart,
												 CComPtr<IMgaFCO>& pFCO);
	virtual void			Destroy				(void);
	virtual CString			GetMnemonic			(void) const;
	virtual feature_code	GetFeatures			(void) const;
	virtual void			SetParam			(const CString& strName, VARIANT vValue);
	virtual bool			GetParam			(const CString& strName, VARIANT* pvValue);
	virtual void			SetActive			(bool bIsActive);
	virtual CSize			GetPreferredSize	(void) const;
	virtual void			SetLocation			(const CRect& location);
	virtual CRect			GetLocation			(void) const;
	virtual CRect			GetLabelLocation	(void) const;
	virtual void			Draw				(CDC* pDC);
	virtual void			SaveState			(void);

	virtual void	InitializeEx				(CComPtr<IMgaProject>& pProject, CComPtr<IMgaMetaPart>& pPart,
												 CComPtr<IMgaFCO>& pFCO, HWND parentWnd, PreferenceMap& preferences);
	virtual void	SetSelected					(bool bIsSelected);
	virtual bool	MouseMoved					(UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseLeftButtonDown			(UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseLeftButtonUp			(UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseLeftButtonDoubleClick	(UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseRightButtonDown		(HMENU hCtxMenu, UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseRightButtonUp			(UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseRightButtonDoubleClick	(UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseMiddleButtonDown		(UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseMiddleButtonUp			(UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseMiddleButtonDoubleClick(UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	MouseWheelTurned			(UINT nFlags, short distance, const CPoint& point, HDC transformHDC);
	virtual bool	MenuItemSelected			(UINT menuItemId, UINT nFlags, const CPoint& point, HDC transformHDC);
	virtual bool	OperationCanceledByGME		(void);

	virtual void	SetBrush					(CDC* pDC);

	virtual void	CollectAttributes			(CComPtr<IMgaFCO> mgaFco = NULL) = 0;
	virtual void	CalcRelPositions			(CDC* pDC = NULL);
	virtual void	SetBoxLocation				(const CRect& cRect);
	virtual void	SetReferenced				(bool referenced);
	virtual void	SetParentPart				(PartBase* pPart);
	void			SortAttributes				(void);
};

}; // namespace DecoratorSDK

#endif //__CLASSCOMPLEXPART_H_

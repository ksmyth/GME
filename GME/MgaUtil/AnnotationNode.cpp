// AnnotationNode.cpp: implementation of the CAnnotationNode class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mgautil.h"
#include "AnnotationNode.h"
#include "..\Annotator\AnnotationDefs.h"
#include "AnnotationBrowserDlg.h"
#include "AnnotationUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
LOGFONT	CAnnotationNode::defFont;
COLORREF CAnnotationNode::defColor;
COLORREF CAnnotationNode::defBgcolor;
COLORREF CAnnotationNode::defShadowcolor;
COLORREF CAnnotationNode::defGradientcolor;
bool CAnnotationNode::classIsInitialized	= false;
bool CAnnotationNode::defInheritable		= false;
bool CAnnotationNode::defHidden				= false;
bool CAnnotationNode::defCanBeRederived		= false;
bool CAnnotationNode::defGradientFill		= false;
int  CAnnotationNode::defGradientDirection	= 0;
bool CAnnotationNode::defCastShadow			= false;
int  CAnnotationNode::defShadowDepth		= 9;
int  CAnnotationNode::defShadowDirection	= 45;
bool CAnnotationNode::defRoundCornerRect	= false;
int  CAnnotationNode::defRoundCornerRadius	= 9;

CAnnotationNode::CAnnotationNode(const CComPtr<IMgaRegNode> &regNode, IMgaFCOPtr& archetype)
{
	m_regNode = regNode;
	m_archetype = archetype;
	if (!classIsInitialized) {
		InitializeClass();
	}
}

void CAnnotationNode::Read(CAnnotationBrowserDlg *dlg)
{
	try {
		m_canBeRederived = false;
		if (m_archetype)
		{
			for_each_subnode([&](IMgaRegNodePtr& regnode)
			{
				long status = ATTSTATUS_UNDEFINED;
				regnode->GetStatus(&status);
				if (status == ATTSTATUS_HERE)
				{
					m_canBeRederived = true;
				}
			});
		}
	}
	catch (hresult_exception &) {
		ASSERT(("Error while reading annotation from registry.", false));
	}

	// Annotation name
	try {
		CComBSTR bstr;
		COMTHROW(m_regNode->get_Name(&bstr));
		m_name = bstr;
	}
	catch (hresult_exception &) {
		ASSERT(("Error while reading annotation from registry.", false));
		m_name = _T("ERROR!!!");
	}
	
	// Annotation text
	try {
		CComBSTR bstr;
		COMTHROW(m_regNode->get_Value(&bstr));
		m_text = bstr;
		m_text = CAnnotationUtil::ResolveNewLinesToCRLF(m_text);
	}
	catch (hresult_exception &) {
		ASSERT(("Error while reading annotation from registry.", false));
		m_text = _T("Unable to read annotation text.");
	}

	// 'Inheritable'
	try {
		m_inheritable = defInheritable;
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_INHERITABLE);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
			if (bstr == L"1")
				m_inheritable = true;
		}
	}
	catch (hresult_exception &) {
		m_inheritable = defInheritable;
	}

	// 'Hidden'
	try {
		m_hidden = defHidden;
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_HIDDEN);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
			if (bstr == L"1")
				m_hidden = true;
		}
	}
	catch (hresult_exception &) {
		m_hidden = defHidden;
	}

	// Font
	try {
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_FONT_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
		}
		CString str(bstr);
		if (!CAnnotationUtil::LogfontDecode(str, &m_logfont)) {
			memcpy(&m_logfont, &defFont, sizeof(LOGFONT));
		}
	}
	catch (hresult_exception &) {
		memcpy(&m_logfont, &defFont, sizeof(LOGFONT));
	}

	// Text Color
	try {
		CComBSTR bstr;
		CComPtr<IMgaRegNode> colNode;
		CComBSTR colName(AN_COLOR_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(colName, &colNode));
		if (colNode != NULL) {
			COMTHROW(colNode->get_Value(&bstr));
		}
		CString strVal(bstr);
		unsigned int val;
		if (_stscanf(strVal,_T("%x"),&val) == 1) {
			unsigned int r = (val & 0xff0000) >> 16;
			unsigned int g = (val & 0xff00) >> 8;
			unsigned int b = val & 0xff;
			m_color = RGB(r,g,b);
		}
		else {
			m_color = defColor;
		}
	}
	catch (hresult_exception &) {
		m_color = defColor;
	}

	// Background Color
	try {
		CComBSTR bstr;
		CComPtr<IMgaRegNode> bgcolNode;
		CComBSTR bgcolName(AN_BGCOLOR_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(bgcolName, &bgcolNode));
		if (bgcolNode != NULL) {
			COMTHROW(bgcolNode->get_Value(&bstr));
		}
		CString strVal(bstr);
		unsigned int val;
		if (_stscanf(strVal,_T("%x"),&val) == 1) {
			unsigned int r = (val & 0xff0000) >> 16;
			unsigned int g = (val & 0xff00) >> 8;
			unsigned int b = val & 0xff;
			m_bgcolor = RGB(r,g,b);
		}
		else {
			m_bgcolor = defBgcolor;
		}
	}
	catch (hresult_exception &) {
		m_bgcolor = AN_DEFAULT_BGCOLOR;
	}

	// Gradient Color
	try {
		CComBSTR bstr;
		CComPtr<IMgaRegNode> gradientcolNode;
		CComBSTR gradientcolName(AN_GRADIENTCOLOR_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(gradientcolName, &gradientcolNode));
		if (gradientcolNode != NULL) {
			COMTHROW(gradientcolNode->get_Value(&bstr));
		}
		CString strVal(bstr);
		unsigned int val;
		if (_stscanf(strVal,_T("%x"),&val) == 1) {
			unsigned int r = (val & 0xff0000) >> 16;
			unsigned int g = (val & 0xff00) >> 8;
			unsigned int b = val & 0xff;
			m_crGradient = RGB(r,g,b);
		}
		else {
			m_crGradient = defGradientcolor;
		}
	}
	catch (hresult_exception &) {
		m_crGradient = AN_DEFAULT_GRADIENTCOLOR;
	}

	// Shadow Color
	try {
		CComBSTR bstr;
		CComPtr<IMgaRegNode> bordercolNode;
		CComBSTR bordercolName(AN_SHADOWCOLOR_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(bordercolName, &bordercolNode));
		if (bordercolNode != NULL) {
			COMTHROW(bordercolNode->get_Value(&bstr));
		}
		CString strVal(bstr);
		unsigned int val;
		if (_stscanf(strVal,_T("%x"),&val) == 1) {
			unsigned int r = (val & 0xff0000) >> 16;
			unsigned int g = (val & 0xff00) >> 8;
			unsigned int b = val & 0xff;
			m_crShadow = RGB(r,g,b);
		}
		else {
			m_crShadow = defShadowcolor;
		}
	}
	catch (hresult_exception &) {
		m_crShadow = AN_DEFAULT_SHADOWCOLOR;
	}

	// 'GradienFill'
	try {
		m_bGradientFill = defGradientFill;
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_GRADIENTFILL_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
			if (bstr == L"1")
				m_bGradientFill = true;
			else
				m_bGradientFill = false;
		}
	}
	catch (hresult_exception &) {
		m_bGradientFill = defGradientFill;
	}

	// 'GradientDirection'
	try {
		m_iGradientDirection = defGradientDirection;
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_GRADIENTDIRECTION_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
			CString strVal(bstr);
			if (_stscanf(strVal,_T("%d"),&m_iGradientDirection) != 1) {
				m_iGradientDirection = defGradientDirection;
			}
		}
	}
	catch (hresult_exception &) {
		m_iGradientDirection = defGradientDirection;
	}

	// 'CastShadow'
	try {
		m_bCastShadow = defCastShadow;
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_CASTSHADOW_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
			if (bstr == L"1")
				m_bCastShadow = true;
			else
				m_bCastShadow = false;
		}
	}
	catch (hresult_exception &) {
		m_bCastShadow = defCastShadow;
	}

	// 'ShadowDepth'
	try {
		m_iShadowDepth = defShadowDepth;
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_SHADOWDEPTH_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
			CString strVal(bstr);
			if (_stscanf(strVal,_T("%d"),&m_iShadowDepth) != 1) {
				m_iShadowDepth = defShadowDepth;
			}
		}
	}
	catch (hresult_exception &) {
		m_iShadowDepth = defShadowDepth;
	}

	// 'ShadowDirection'
	try {
		m_iShadowDirection = defShadowDirection;
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_SHADOWDIRECTION_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
			CString strVal(bstr);
			if (_stscanf(strVal,_T("%d"),&m_iShadowDirection) != 1) {
				m_iShadowDirection = defShadowDirection;
			}
		}
	}
	catch (hresult_exception &) {
		m_iShadowDirection = defShadowDirection;
	}

	// 'RoundCornerRect'
	try {
		m_bRoundCornerRect = defRoundCornerRect;
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_ROUNDCORNERRECT_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
			if (bstr == L"1")
				m_bRoundCornerRect = true;
			else
				m_bRoundCornerRect = false;
		}
	}
	catch (hresult_exception &) {
		m_bRoundCornerRect = defRoundCornerRect;
	}

	// 'RoundCornerRadius'
	try {
		m_iRoundCornerRadius = defRoundCornerRadius;
		CComBSTR bstr;
		CComPtr<IMgaRegNode> lfNode;
		CComBSTR lfName(AN_ROUNDCORNERRADIUS_PREF);
		COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
		if (lfNode != NULL) {
			COMTHROW(lfNode->get_Value(&bstr));
			CString strVal(bstr);
			if (_stscanf(strVal,_T("%d"),&m_iRoundCornerRadius) != 1) {
				m_iRoundCornerRadius = defRoundCornerRadius;
			}
		}
	}
	catch (hresult_exception &) {
		m_iRoundCornerRadius = defRoundCornerRadius;
	}

	// Aspects
	m_aspects.SetSize(dlg->m_aspectNames.GetSize());
	CComPtr<IMgaRegNode> aspRoot;
	CComBSTR aspRootName(AN_ASPECTS);
	COMTHROW(m_regNode->get_SubNodeByName(aspRootName, &aspRoot));
	for (int aspIdx = 0; aspIdx < dlg->m_aspectNames.GetSize(); aspIdx++) {
		if (aspIdx == 0) {
			m_aspects[aspIdx].m_isLocDef = false;

			CComBSTR defName(AN_DEFASPECT);
			CComPtr<IMgaRegNode> defNode;
			COMTHROW(aspRoot->get_SubNodeByName(defName, &defNode));
			long status;
			COMTHROW(defNode->get_Status(&status));
			m_aspects[aspIdx].m_isVisible = (status != ATTSTATUS_UNDEFINED);

			CComBSTR  rootVal;
			COMTHROW(aspRoot->get_Value(&rootVal));
			CString rootValStr(rootVal);
			int lx, ly;
			if(_stscanf(rootValStr,_T("%d,%d"), &lx, &ly) == 2) {
				m_aspects[aspIdx].m_loc = CPoint(lx,ly);	
			}
			else {
				m_aspects[aspIdx].m_loc = CPoint(0,0);
			}
		}
		else {
			CComBSTR aspName(dlg->m_aspectNames[aspIdx]);
			CComPtr<IMgaRegNode> aspNode;
			COMTHROW(aspRoot->get_SubNodeByName(aspName, &aspNode));
			long status;
			COMTHROW(aspNode->get_Status(&status));
			if (status == ATTSTATUS_UNDEFINED) {
				m_aspects[aspIdx].m_isVisible = false;
				m_aspects[aspIdx].m_loc = m_aspects[0].m_loc;
				m_aspects[aspIdx].m_isLocDef = true;
			}
			else {
				m_aspects[aspIdx].m_isVisible = true;
				CComBSTR  aspVal;
				COMTHROW(aspNode->get_Value(&aspVal));
				CString aspValStr(aspVal);
				int lx, ly;
				if(_stscanf(aspValStr,_T("%d,%d"), &lx, &ly) == 2) {
					m_aspects[aspIdx].m_loc = CPoint(lx,ly);
					m_aspects[aspIdx].m_isLocDef = false;
				}
				else {
					m_aspects[aspIdx].m_loc = m_aspects[0].m_loc;
					m_aspects[aspIdx].m_isLocDef = true;
				}
			}
		}
	}
}

void CAnnotationNode::Write(CAnnotationBrowserDlg *dlg)
{
	// it will notify if we just broke an annotation inheritance relationship (if text in the derived becomes different than the base's text)
	bool broken_inheritance = false;

	// Drop previous node, start new one
	try {
		CComPtr<IMgaRegNode> parentNode;
		COMTHROW(m_regNode->get_ParentNode(&parentNode));

		// get the old text, used later to check if subtypes/instances break the derivation chain of the annotation
		CComBSTR bstr1;
		COMTHROW(m_regNode->get_Value(&bstr1));
		CString old_m_text = bstr1;
		old_m_text = CAnnotationUtil::ResolveNewLinesToCRLF(old_m_text);

		COMTHROW(m_regNode->RemoveTree()); // we will write a new one
	}
	catch (hresult_exception &) {
		ASSERT(("Error while creating annotation to registry.", false));
		return;
	}

	// Store text
	try {
		CString tmpstr = CAnnotationUtil::ResolveNewLinesToLF(m_text);
		CComBSTR bstr(tmpstr);
		COMTHROW(m_regNode->put_Value(bstr));
	}
	catch (hresult_exception &) {
		ASSERT(("Error while writing annotation to registry.", false));
	}

	CComPtr<IMgaRegNode>& regNode = m_regNode;
	auto storeInt = [&regNode](int default_, int value, const TCHAR* regName)
	{
		try {
			CComPtr<IMgaRegNode> lfNode;
			CComBSTR lfName(regName);
			COMTHROW(regNode->get_SubNodeByName(lfName, &lfNode));
			if (default_ != value)
			{
				CString str;
				str.Format(_T("%ld"), value);
				CComBSTR bstr(str);
				COMTHROW(lfNode->put_Value(bstr));
			}
		}
		catch (hresult_exception &) {
			ASSERT(("Error while writing annotation to registry.", false));
		}
	};

	auto storeBool = [&regNode](bool default_, bool value, const TCHAR* regName)
	{
		try {
			CComPtr<IMgaRegNode> lfNode;
			CComBSTR lfName(regName);
			COMTHROW(regNode->get_SubNodeByName(lfName, &lfNode));
			if (default_ != value)
			{
				CString str(value ? _T("1") : _T("0"));
				CComBSTR bstr(str);
				COMTHROW(lfNode->put_Value(bstr));
			}
		}
		catch (hresult_exception &) {
			ASSERT(("Error while writing annotation to registry.", false));
		}
	};

	auto storeColorRef = [&regNode](COLORREF default_, COLORREF value, const TCHAR* regName)
	{
		try {
			CComPtr<IMgaRegNode> colNode;
			CComBSTR colName(regName);
			COMTHROW(regNode->get_SubNodeByName(colName, &colNode));
			if (default_ != value) {
				unsigned long ival = value;
				unsigned long r = (ival & 0xff0000) >> 16;
				unsigned long g = (ival & 0xff00) >> 8;
				unsigned long b = ival & 0xff;
				ival = (unsigned long)(RGB(r,g,b));
				CString str;
				str.Format(_T("0x%06x"), (unsigned long)ival);
				CComBSTR bstr(str);
				COMTHROW(colNode->put_Value(bstr));
			}
		}
		catch (hresult_exception &) {
			ASSERT(("Error while writing annotation to registry.", false));
		}
	};

	storeInt(defInheritable, m_inheritable, AN_INHERITABLE);

	storeBool(defHidden, m_hidden, AN_HIDDEN);

	// Store color,bgcolor,font preferences
	if (memcmp(&m_logfont, &defFont, sizeof(LOGFONT)) != 0) {
		try {
			CString str;
			CAnnotationUtil::LogfontEncode(str, &m_logfont);
			CComBSTR bstr(str);
			CComPtr<IMgaRegNode> lfNode;
			CComBSTR lfName(AN_FONT_PREF);
			COMTHROW(m_regNode->get_SubNodeByName(lfName, &lfNode));
			COMTHROW(lfNode->put_Value(bstr));
		}
		catch (hresult_exception &) {
			ASSERT(("Error while writing annotation to registry.", false));
		}
	}

	storeColorRef(defColor, m_color, AN_COLOR_PREF);

	storeColorRef(defBgcolor, m_bgcolor, AN_BGCOLOR_PREF);

	storeColorRef(defShadowcolor, m_crShadow, AN_SHADOWCOLOR_PREF);

	storeColorRef(defGradientcolor, m_crGradient, AN_GRADIENTCOLOR_PREF);

	storeBool(defGradientFill, m_bGradientFill, AN_GRADIENTFILL_PREF);

	storeInt(defGradientDirection, m_iGradientDirection, AN_GRADIENTDIRECTION_PREF);

	storeBool(defCastShadow, m_bCastShadow, AN_CASTSHADOW_PREF);

	storeInt(defShadowDepth, m_iShadowDepth, AN_SHADOWDEPTH_PREF);

	storeInt(defShadowDirection, m_iShadowDirection, AN_SHADOWDIRECTION_PREF);

	storeBool(defRoundCornerRect, m_bRoundCornerRect, AN_ROUNDCORNERRECT_PREF);

	storeInt(defRoundCornerRadius, m_iRoundCornerRadius, AN_ROUNDCORNERRADIUS_PREF);

	// Store default location & aspect visibility
	CComPtr<IMgaRegNode> aspRoot;
	CComBSTR aspRootName(AN_ASPECTS);
	COMTHROW(m_regNode->get_SubNodeByName(aspRootName, &aspRoot));
	for (int aspIdx = 0; aspIdx < dlg->m_aspectNames.GetSize(); aspIdx++) {
		if (aspIdx == 0) {
			CString rootValStr;
			rootValStr.Format(_T("%d,%d"), m_aspects[aspIdx].m_loc.x, m_aspects[aspIdx].m_loc.y);
			CComBSTR  rootVal(rootValStr);
			COMTHROW(aspRoot->put_Value(rootVal));

			CComBSTR defName(AN_DEFASPECT);
			CComPtr<IMgaRegNode> defNode;
			COMTHROW(aspRoot->get_SubNodeByName(defName, &defNode));
			if (m_aspects[aspIdx].m_isVisible) {
				CComBSTR defVal(AN_VISIBLE_DEFAULT);
				COMTHROW(defNode->put_Value(defVal));
			}
			else // by default a value is inserted into the AN_DEFASPECT key in the registry
			{    // in CGMEView::OnCntxInsertannotation() so we remove in case the user 
				 // did UNCHECK that option
				long st;
				COMTHROW( defNode->get_Status( &st));
				if( st == ATTSTATUS_HERE) COMTHROW( defNode->Clear());
			}
		}
		else {
			if (m_aspects[aspIdx].m_isVisible) {
				CComBSTR aspName(dlg->m_aspectNames[aspIdx]);
				CComPtr<IMgaRegNode> aspNode;
				COMTHROW(aspRoot->get_SubNodeByName(aspName, &aspNode));
				CString aspValStr;
				if (m_aspects[aspIdx].m_isLocDef) {
					aspValStr = AN_VISIBLE_DEFAULT;
				}
				else {
					aspValStr.Format(_T("%d,%d"), m_aspects[aspIdx].m_loc.x, m_aspects[aspIdx].m_loc.y);
				}
				CComBSTR  aspVal(aspValStr);
				COMTHROW(aspNode->put_Value(aspVal));
			}
		}
	}

	if (m_archetype)
	{
		for_each_subnode([&](IMgaRegNodePtr& regnode)
		{
			/*
			IMgaRegNodePtr archetypeReg = m_archetype->RegistryNode[regnode->Path];
			long status = ATTSTATUS_UNDEFINED;
			archetypeReg->GetStatus(&status);
			if (status != ATTSTATUS_UNDEFINED && archetypeReg->Value == regnode->Value)
			{
				regnode->
			}*/
			if (m_archetype->RegistryValue[regnode->Path] == regnode->Value)
			{
				regnode->Clear();
			}
		});
	}

}

void CAnnotationNode::InitializeClass()
{
	CAnnotationUtil::FillLogFontWithDefault(&defFont);

	defColor = AN_DEFAULT_COLOR;

	defBgcolor = AN_DEFAULT_BGCOLOR;

	defShadowcolor = AN_DEFAULT_SHADOWCOLOR;

	defGradientcolor = AN_DEFAULT_GRADIENTCOLOR;

	classIsInitialized = true;
}

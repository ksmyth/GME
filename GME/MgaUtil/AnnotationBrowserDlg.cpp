// AnnotationBrowserDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mgautil.h"
#include "AnnotationBrowserDlg.h"
#include "..\Annotator\AnnotationDefs.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const TCHAR * show_str = _T("Show");
const TCHAR * hide_str = _T("Hide");
/////////////////////////////////////////////////////////////////////////////
// CAnnotationBrowserDlg dialog


CAnnotationBrowserDlg::CAnnotationBrowserDlg(const CComPtr<IMgaModel> &model, const CComPtr<IMgaRegNode> &focus, CWnd* pParent /*=NULL*/)
	: CDialog(CAnnotationBrowserDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAnnotationBrowserDlg)
	m_modelName				= _T("");
	m_modelRole				= _T("");
	m_modelKind				= _T("");
	m_anName				= _T("");
	m_anText				= _T("");
	m_anFont				= _T("");
	m_aaDefPos				= FALSE;
	m_aaXCoord				= 0;
	m_aaYCoord				= 0;
	m_bGradientFill			= FALSE;
	m_iGradientDirection	= 0;
	m_bCastShadow			= FALSE;
	m_iShadowDepth			= 9;
	m_iShadowDirection		= 45;
	m_bRoundCornerRect		= FALSE;
	m_iRoundCornerRadius	= 9;
	//}}AFX_DATA_INIT

	m_model = model;
	m_focus = focus;
}

CAnnotationBrowserDlg::~CAnnotationBrowserDlg()
{
	POSITION pos = m_annotations.GetHeadPosition();
	while (pos) {
		delete m_annotations.GetNext(pos);
	}
}

void CAnnotationBrowserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAnnotationBrowserDlg)
	DDX_Control(pDX, IDC_LIST_ASPECT, m_wndAnnotationAspectList);
	DDX_Control(pDX, IDC_ANNOTATION_LIST, m_wndAnnotationList);
	DDX_Control(pDX, IDC_ANNOTATION, m_wndAnnotation);
	DDX_Text(pDX, IDC_MODELNAME, m_modelName);
	DDX_Text(pDX, IDC_MODELROLE, m_modelRole);
	DDX_Text(pDX, IDC_MODELKIND, m_modelKind);
	DDX_Text(pDX, IDC_EDIT_NAME, m_anName);
	DDX_Text(pDX, IDC_EDIT_TEXT, m_anText);
	DDX_Text(pDX, IDC_EDIT_FONT, m_anFont);
	DDX_Check(pDX, IDC_CHECK_DEFPOS, m_aaDefPos);
	DDX_Text(pDX, IDC_EDIT_XCOORD, m_aaXCoord);
	DDX_Text(pDX, IDC_EDIT_YCOORD, m_aaYCoord);
	DDX_Check(pDX, IDC_CHECK_GRADIENTFILL, m_bGradientFill);
	DDX_Text(pDX, IDC_EDIT_GRADIENTDIR, m_iGradientDirection);
	DDX_Check(pDX, IDC_CHECK_CASTSHADOW, m_bCastShadow);
	DDX_Text(pDX, IDC_EDIT_SHADOWDEPTH, m_iShadowDepth);
	DDX_Text(pDX, IDC_EDIT_SHADOWDIRECTION, m_iShadowDirection);
	DDX_Check(pDX, IDC_CHECK_ROUNDCORNER, m_bRoundCornerRect);
	DDX_Text(pDX, IDC_EDIT_CORNERRADIUS, m_iRoundCornerRadius);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CHECK1, m_inheritable);
	DDX_Control(pDX, IDC_SHOWHIDEBUTTON, m_showHideBtn);
	DDX_Control(pDX, IDC_REDERIVEBUTTON, m_rederiveBtn);
}


BEGIN_MESSAGE_MAP(CAnnotationBrowserDlg, CDialog)
	//{{AFX_MSG_MAP(CAnnotationBrowserDlg)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ANNOTATION_LIST, OnItemchangedAnnotationList)
	ON_BN_CLICKED(IDC_BUTTON_FONT, OnButtonFont)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_ASPECT, OnItemchangedListAspect)
	ON_BN_CLICKED(IDC_CHECK_DEFPOS, OnCheckDefpos)
	ON_BN_CLICKED(IDC_SHOWHIDEBUTTON, OnBnClickedShowhidebutton)
	ON_BN_CLICKED(IDC_REDERIVEBUTTON, OnBnClickedRederivebutton)
	ON_BN_CLICKED(IDC_CHECK_GRADIENTFILL, OnBnClickedCheckGradientfill)
	ON_BN_CLICKED(IDC_CHECK_CASTSHADOW, OnBnClickedCheckCastshadow)
	ON_BN_CLICKED(IDC_CHECK_ROUNDCORNER, OnBnClickedCheckRoundCorner)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnnotationBrowserDlg message handlers

BOOL CAnnotationBrowserDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_colorbtn.SubclassDlgItem(IDC_COMBO_COLOR,this); 
	m_bgcolorbtn.SubclassDlgItem(IDC_COMBO_BGCOLOR,this); 
	m_shadowcolorbtn.SubclassDlgItem(IDC_COMBO_SHADOWCOLOR,this); 
	m_gradientcolorbtn.SubclassDlgItem(IDC_COMBO_GRADIENTCOLOR,this); 

	CRect rect(0,0,0,0);

	m_wndSplitterBar.Create(WS_CHILD|WS_BORDER|WS_DLGFRAME|WS_VISIBLE, rect,this,999);
	m_wndSplitterBar.SetPanes(&m_wndAnnotationList,&m_wndAnnotation);

	CRect clientRect;
	m_wndAnnotationList.GetClientRect(&clientRect);
	int ctrlSize = clientRect.Width();
	m_wndAnnotationList.InsertColumn(0, _T("Name"), LVCFMT_LEFT,  ctrlSize / 3, -1);
	m_wndAnnotationList.InsertColumn(1, _T("Text"), LVCFMT_LEFT,  (2*ctrlSize) / 3, -1);

	m_wndAnnotationAspectList.GetClientRect(&clientRect);
	ctrlSize = clientRect.Width();
	m_wndAnnotationAspectList.InsertColumn(0, _T("Aspect"), LVCFMT_LEFT,  ctrlSize, -1);

	LPARAM dwStyle = ::SendMessage(m_wndAnnotationAspectList.GetSafeHwnd(),LVM_GETEXTENDEDLISTVIEWSTYLE,0,0);
	dwStyle |= LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES ;
	::SendMessage(m_wndAnnotationAspectList.GetSafeHwnd(),LVM_SETEXTENDEDLISTVIEWSTYLE,0,dwStyle);

	dwStyle = ::SendMessage(m_wndAnnotationList.GetSafeHwnd(),LVM_GETEXTENDEDLISTVIEWSTYLE,0,0);
	dwStyle |= LVS_EX_FULLROWSELECT;
	::SendMessage(m_wndAnnotationList.GetSafeHwnd(),LVM_SETEXTENDEDLISTVIEWSTYLE,0,dwStyle);

	try {
		CComBSTR bstr_tmp;
		COMTHROW(m_model->get_Name(&bstr_tmp));
		CopyTo(bstr_tmp, m_modelName);
		SetWindowTextW(L"Annotations for '" + m_modelName + "'");

		CComPtr<IMgaMetaFCO> metaFco;
		COMTHROW(m_model->get_Meta(&metaFco));
		if (metaFco) {
			bstr_tmp.Empty();
			COMTHROW(metaFco->get_DisplayedName(&bstr_tmp));
			CopyTo(bstr_tmp,m_modelKind);
		}
		else {
			m_modelKind = _T("N/A");
		}

		CComPtr<IMgaMetaRole> metaRole;
		COMTHROW(m_model->get_MetaRole(&metaRole) );
		if (metaRole) {
			bstr_tmp.Empty();
			COMTHROW(metaRole->get_Name(&bstr_tmp));
			CopyTo(bstr_tmp, m_modelRole);
		}
		else {
			m_modelRole = _T("N/A");
		}
	}
	catch (hresult_exception &){
		ASSERT(("Error while initializing Annotation Browser.", false));
	}
	UpdateData(FALSE);

	if (m_model) {
		FillAspects();
		FillAnnotations();
	}

	int listIdx = 0, focusIdx = 0;
	POSITION pos = m_annotations.GetHeadPosition();
	if (pos) {
		while (pos) {
			CAnnotationNode *node = m_annotations.GetNext(pos);
			LVITEM lvItem;
			lvItem.mask = LVIF_PARAM | LVIF_TEXT;
			lvItem.iItem = listIdx++;
			lvItem.iSubItem = 0;
			lvItem.pszText = node->m_name.GetBuffer(node->m_name.GetLength());
			lvItem.lParam = (LPARAM)node;

			int retIdx = m_wndAnnotationList.InsertItem(&lvItem);
			if (m_focus && node->m_regNode->Path == m_focus->Path) {
				focusIdx = retIdx;
			}

			lvItem.mask = LVIF_TEXT;
			lvItem.iItem = retIdx;
			lvItem.iSubItem = 1;
			lvItem.pszText = node->m_text.GetBuffer(node->m_text.GetLength());
			m_wndAnnotationList.SetItem(&lvItem);
		}
		m_wndAnnotationList.SetItemState(focusIdx, LVIS_SELECTED, LVIS_SELECTED | LVIS_FOCUSED);
		m_wndAnnotationList.EnsureVisible(focusIdx, FALSE);
	}
	else
	{
		// FIXME: should disable more things, but this gets the point across
		GetDlgItem(IDC_EDIT_NAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_TEXT)->EnableWindow(FALSE);
	}

	::PostMessage(GetSafeHwnd(), WM_NEXTDLGCTL, (WPARAM)(GetDlgItem(IDC_EDIT_TEXT)->GetSafeHwnd()), TRUE);
	/*
	//Test
	for (int i = 0; i< m_aspectNames.GetSize(); i++) {
		m_wndAnnotationAspectList.InsertItem(i, m_aspectNames[i]);
		m_wndAnnotationAspectList.SetCheck(i, i % 2);
	}
*/
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAnnotationBrowserDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	CRect origTopRect(0, 0, 0+406, 0+20), origBottomRect(0, 334, 0+406, 334+20 ); 
	MapDialogRect(&origTopRect);
	MapDialogRect(&origBottomRect);

	CRect rectList, rectPanel;
	if (m_wndAnnotationList.GetSafeHwnd()) {
		m_wndAnnotationList.GetWindowRect(&rectList);
		ScreenToClient(&rectList);
	}
	if (m_wndAnnotation.GetSafeHwnd()) {
		m_wndAnnotation.GetWindowRect(&rectPanel);
		ScreenToClient(&rectPanel);
	}

	// Not perfect....
	double ratio = (double)rectList.Width()/((double)rectPanel.Width()+(double)rectList.Width());

	if (m_wndAnnotation.GetSafeHwnd() && m_wndAnnotationList.GetSafeHwnd()) {
		m_wndAnnotationList.SetWindowPos(NULL, 0, origTopRect.bottom, static_cast<int>(cx*ratio), cy - (origBottomRect.bottom-origBottomRect.top) - origTopRect.bottom, SWP_NOZORDER);
		m_wndAnnotation.SetWindowPos(NULL, static_cast<int>(cx*ratio)+GetSystemMetrics(SM_CYFRAME), origTopRect.bottom, cx - (static_cast<int>(cx*ratio)+GetSystemMetrics(SM_CYFRAME)), cy - (origBottomRect.bottom-origBottomRect.top) - origTopRect.bottom, SWP_NOZORDER);
		m_wndSplitterBar.SetPanes(&m_wndAnnotationList,&m_wndAnnotation);
	}

	CRect rect3(0, 338, 0+406, 338+17);
	MapDialogRect(&rect3);
	CRect rectDlg;
	GetClientRect(&rectDlg);
	ratio = ((double)(rectDlg.Width())/ 406.0 );

	CWnd* wndOK = this->GetDlgItem(IDOK);
	if (wndOK) {
		CRect origWnd;
		wndOK->GetClientRect(&origWnd);
		wndOK->MoveWindow(static_cast<int>(112.0 * ratio), rectDlg.bottom - (rect3.bottom - rect3.top), origWnd.Width(), origWnd.Height());
	}
	CWnd* wndCANCEL = this->GetDlgItem(IDCANCEL);
	if (wndCANCEL) {
		CRect origWnd;
		wndCANCEL->GetClientRect(&origWnd);
		wndCANCEL->MoveWindow(static_cast<int>(242.0 * ratio), rectDlg.bottom - (rect3.bottom - rect3.top), origWnd.Width(), origWnd.Height());
	}
}

void CAnnotationBrowserDlg::OnOK() 
{
	POSITION pos = m_wndAnnotationList.GetFirstSelectedItemPosition();
	if (pos) {
		int nItem = m_wndAnnotationList.GetNextSelectedItem(pos);
		CAnnotationNode *node = (CAnnotationNode *)m_wndAnnotationList.GetItemData(nItem);
		SavePanelToNode(node);
	}
	
	CDialog::OnOK();
}

void CAnnotationBrowserDlg::FillAspects()
{
	m_aspectNames.RemoveAll();
	m_aspectNames.Add(_T("DEFAULT"));

	CComPtr<IMgaMetaFCO>	metaFCO;
	COMTHROW(m_model->get_Meta(&metaFCO));
	CComPtr<IMgaMetaModel>	metaModel;
	COMTHROW(metaFCO.QueryInterface(&metaModel));
	CComPtr<IMgaMetaAspects>	aspects;
	COMTHROW(metaModel->get_Aspects(&aspects));
	MGACOLL_ITERATE(IMgaMetaAspect, aspects) {
		CComPtr<IMgaMetaAspect> aspect;
		aspect = MGACOLL_ITER;

		CComBSTR bstr;
		COMTHROW(aspect->get_Name(&bstr));
		COLE2T aspectName(bstr);
		m_aspectNames.Add(aspectName);
	}
	MGACOLL_ITERATE_END;
}

void CAnnotationBrowserDlg::FillAnnotations()
{
	try {
		CComBSTR annRootName(AN_ROOT);
		CComPtr<IMgaRegNode> annRootNode;
		COMTHROW(m_model->get_RegistryNode(annRootName, &annRootNode));
		CComPtr<IMgaRegNodes> subNodes;
		COMTHROW(annRootNode->get_SubNodes(VARIANT_TRUE, &subNodes));//get all nodes (inheriteds also)
		MGACOLL_ITERATE(IMgaRegNode, subNodes) {
			CComPtr<IMgaRegNode> subNode;
			subNode = MGACOLL_ITER;
			CAnnotationNode *node = new CAnnotationNode(subNode, m_model->DerivedFrom);
			node->Read(this);
			m_annotations.AddTail(node);
		}
		MGACOLL_ITERATE_END;
	}
	catch (hresult_exception &) {
		ASSERT(("Error while reading annotations.", false));
	}
}


void CAnnotationBrowserDlg::UpdateAnnotations(void)
{
	POSITION pos = m_annotations.GetHeadPosition();
	while (pos) {
		m_annotations.GetNext(pos)->Write(this);
	}
}

void CAnnotationBrowserDlg::OnItemchangedAnnotationList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;

	if ((pNMListView->uChanged & LVIF_STATE) == LVIF_STATE) {
		CAnnotationNode *node = (CAnnotationNode*)pNMListView->lParam;

		if ( (pNMListView->uNewState & LVIS_SELECTED) && (!(pNMListView->uOldState & LVIS_SELECTED)) ) {
			LoadNodeToPanel(node);
		}
		if ( (!(pNMListView->uNewState & LVIS_SELECTED)) && (pNMListView->uOldState & LVIS_SELECTED) ) {
			SavePanelToNode(node);		
		}
		if (!(pNMListView->uNewState & LVIS_SELECTED) && !this->m_wndAnnotationList.GetFirstSelectedItemPosition()) {
			GetDlgItem(IDC_EDIT_NAME)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_TEXT)->EnableWindow(FALSE);
		}
		else
		{
			GetDlgItem(IDC_EDIT_NAME)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_TEXT)->EnableWindow(TRUE);
		}
	}
}

void CAnnotationBrowserDlg::UpdateFontStr(void) {
	int height = m_anLogFont.lfHeight;
	if (height < 0) {
		height = -MulDiv(height, 72, CClientDC(this).GetDeviceCaps(LOGPIXELSY));
	}
	m_anFont.Format(_T("%s, %d, %s%s"), m_anLogFont.lfFaceName, height, m_anLogFont.lfWeight <= 500 ? _T("Regular") : _T("Bold"), m_anLogFont.lfItalic ? _T(", Italic") : _T(""));
}

void CAnnotationBrowserDlg::SavePanelToNode(CAnnotationNode *node) {
	UpdateData();
	node->m_name = m_anName;
	node->m_text = m_anText;

	// set inheritable if checkbutton is checked
	node->m_inheritable  = (m_inheritable.GetCheck() == BST_CHECKED);

	// set hidden attr iff window text is 'show'
	CString tx;
	m_showHideBtn.GetWindowText( tx);
	node->m_hidden = tx == show_str;

	memcpy(&(node->m_logfont), &m_anLogFont, sizeof(LOGFONT));
	node->m_color				= m_colorbtn.currentcolor;
	node->m_bgcolor				= m_bgcolorbtn.currentcolor;
	node->m_crShadow			= m_shadowcolorbtn.currentcolor;
	node->m_crGradient			= m_gradientcolorbtn.currentcolor;
	node->m_bGradientFill		= (m_bGradientFill == TRUE);
	node->m_iGradientDirection	= m_iGradientDirection;
	node->m_bCastShadow			= (m_bCastShadow == TRUE);
	node->m_iShadowDepth		= m_iShadowDepth;
	node->m_iShadowDirection	= m_iShadowDirection;
	node->m_bRoundCornerRect	= (m_bRoundCornerRect == TRUE);
	node->m_iRoundCornerRadius	= m_iRoundCornerRadius;

	POSITION pos = m_wndAnnotationAspectList.GetFirstSelectedItemPosition();
	if (pos) {
		int nItem = m_wndAnnotationAspectList.GetNextSelectedItem(pos);
		CAnnotationAspect *aa = (CAnnotationAspect *)m_wndAnnotationAspectList.GetItemData(nItem);
		SaveVisibilityPanelToAA(aa);
	}

	for (int i = 0; i < m_wndAnnotationAspectList.GetItemCount(); i++) {
		node->m_aspects[i].m_isVisible = (m_wndAnnotationAspectList.GetCheck(i) == TRUE);
	}

	LVFINDINFO lvInfo;
	lvInfo.flags = LVFI_PARAM;
	lvInfo.lParam = (LPARAM)node;
	int nItem = m_wndAnnotationList.FindItem(&lvInfo);
	if (nItem >= 0) {
		LVITEM lvItem;
		
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = nItem;
		lvItem.iSubItem = 0;
		lvItem.pszText = node->m_name.GetBuffer(node->m_name.GetLength());
		m_wndAnnotationList.SetItem(&lvItem);

		lvItem.iSubItem = 1;
		lvItem.pszText = node->m_text.GetBuffer(node->m_text.GetLength());
		m_wndAnnotationList.SetItem(&lvItem);
	}
}

void CAnnotationBrowserDlg::LoadNodeToPanel(CAnnotationNode *node) {
	m_anName = node->m_name;
	m_anText = node->m_text;

	// inheritable checkbox
	m_inheritable.SetCheck( node->m_inheritable? BST_CHECKED:BST_UNCHECKED );
	
	// rederive btn
	m_rederiveBtn.EnableWindow(node->m_canBeRederived);
	
	// show/hide btn
	m_showHideBtn.SetWindowText(node->m_hidden?show_str:hide_str);
	m_showHideBtn.EnableWindow(node->m_hidden);
	if (node->m_hidden == false && node->m_archetype)
	{
		IMgaRegNodePtr archetypeRegNode = node->m_archetype->RegistryNode[node->m_regNode->Path];
		long status = ATTSTATUS_UNDEFINED;
		archetypeRegNode->GetStatus(&status);
		if (status == ATTSTATUS_UNDEFINED)
		{
			m_showHideBtn.EnableWindow(FALSE);
		}
		else
		{
			m_showHideBtn.EnableWindow(TRUE);
		}
	}

	memcpy(&m_anLogFont, &(node->m_logfont), sizeof(LOGFONT));
	m_colorbtn.currentcolor = node->m_color;
	m_bgcolorbtn.currentcolor = node->m_bgcolor;
	m_shadowcolorbtn.currentcolor = node->m_crShadow;
	m_gradientcolorbtn.currentcolor = node->m_crGradient;
	m_bGradientFill = node->m_bGradientFill ? TRUE : FALSE;
	m_iGradientDirection = node->m_iGradientDirection;
	GetDlgItem(IDC_STATIC_GRADIENTCOLOR)->EnableWindow(node->m_bGradientFill);
	GetDlgItem(IDC_COMBO_GRADIENTCOLOR)->EnableWindow(node->m_bGradientFill);
	GetDlgItem(IDC_STATIC_GRADIENTDIR)->EnableWindow(node->m_bGradientFill);
	GetDlgItem(IDC_EDIT_GRADIENTDIR)->EnableWindow(node->m_bGradientFill);
	m_bCastShadow = node->m_bCastShadow ? TRUE : FALSE;
	m_iShadowDepth = node->m_iShadowDepth;
	GetDlgItem(IDC_STATIC_SHADOWCOLOR)->EnableWindow(node->m_bCastShadow);
	GetDlgItem(IDC_COMBO_SHADOWCOLOR)->EnableWindow(node->m_bCastShadow);
	GetDlgItem(IDC_STATIC_SHADOWDEPTH)->EnableWindow(node->m_bCastShadow);
	GetDlgItem(IDC_EDIT_SHADOWDEPTH)->EnableWindow(node->m_bCastShadow);
	GetDlgItem(IDC_STATIC_SHADOWDIRECTION)->EnableWindow(node->m_bCastShadow);
	GetDlgItem(IDC_EDIT_SHADOWDIRECTION)->EnableWindow(node->m_bCastShadow);
	m_iShadowDirection = node->m_iShadowDirection;
	m_bRoundCornerRect = node->m_bRoundCornerRect ? TRUE : FALSE;
	GetDlgItem(IDC_STATIC_CORNERRADIUS)->EnableWindow(node->m_bRoundCornerRect);
	GetDlgItem(IDC_EDIT_CORNERRADIUS)->EnableWindow(node->m_bRoundCornerRect);
	m_iRoundCornerRadius = node->m_iRoundCornerRadius;

	m_wndAnnotationAspectList.DeleteAllItems();
	for (int i = 0; i < m_aspectNames.GetSize(); i++) {
		LVITEM lvItem;
		lvItem.mask = LVIF_PARAM | LVIF_TEXT;
		lvItem.iItem = i;
		lvItem.iSubItem = 0;
		lvItem.pszText = m_aspectNames[i].GetBuffer(m_aspectNames[i].GetLength());
		lvItem.lParam = (LPARAM)&(node->m_aspects[i]);

		int retIdx = m_wndAnnotationAspectList.InsertItem(&lvItem);
		m_wndAnnotationAspectList.SetCheck(retIdx, node->m_aspects[i].m_isVisible ? TRUE : FALSE);
	}
	m_wndAnnotationAspectList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED | LVIS_FOCUSED);
	m_wndAnnotationAspectList.EnsureVisible(0, FALSE);

	UpdateFontStr();
	UpdateData(FALSE);
	m_colorbtn.RedrawWindow();
	m_bgcolorbtn.RedrawWindow();
	m_shadowcolorbtn.RedrawWindow();
	m_gradientcolorbtn.RedrawWindow();
}

void CAnnotationBrowserDlg::OnButtonFont() 
{
	UpdateData(TRUE);//fix for JIRA bug GME-133
	CFontDialog dlg(&m_anLogFont);
	if (dlg.DoModal() == IDOK) {
		dlg.GetCurrentFont(&m_anLogFont);
		UpdateFontStr();
		UpdateData(FALSE);
	}
}

void CAnnotationBrowserDlg::OnItemchangedListAspect(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;

	if ((pNMListView->uChanged & LVIF_STATE) == LVIF_STATE) {
		CAnnotationAspect *aa = (CAnnotationAspect*)pNMListView->lParam;

		if ( (pNMListView->uNewState & LVIS_SELECTED) && (!(pNMListView->uOldState & LVIS_SELECTED)) ) {
			LoadAAToVisibilityPanel(aa);
		}
		if ( (!(pNMListView->uNewState & LVIS_SELECTED)) && (pNMListView->uOldState & LVIS_SELECTED) ) {
			SaveVisibilityPanelToAA(aa);		
		}
	}
}


void CAnnotationBrowserDlg::LoadAAToVisibilityPanel(CAnnotationAspect *aa) {
	m_aaXCoord = aa->m_loc.x;
	m_aaYCoord = aa->m_loc.y;
	m_aaDefPos = aa->m_isLocDef ? TRUE : FALSE;
	GetDlgItem(IDC_EDIT_XCOORD)->EnableWindow(aa->m_isLocDef ? FALSE : TRUE);
	GetDlgItem(IDC_EDIT_YCOORD)->EnableWindow(aa->m_isLocDef ? FALSE : TRUE);
	UpdateData(FALSE);
}

void CAnnotationBrowserDlg::SaveVisibilityPanelToAA(CAnnotationAspect *aa) {
	UpdateData();
	aa->m_loc.x = m_aaXCoord;
	aa->m_loc.y = m_aaYCoord;
	aa->m_isLocDef = (m_aaDefPos == TRUE);
}

void CAnnotationBrowserDlg::OnCheckDefpos() 
{
	CButton *check = (CButton*)GetDlgItem(IDC_CHECK_DEFPOS);
	GetDlgItem(IDC_EDIT_XCOORD)->EnableWindow(check->GetCheck() > 0 ? FALSE : TRUE);
	GetDlgItem(IDC_EDIT_YCOORD)->EnableWindow(check->GetCheck() > 0 ? FALSE : TRUE);		
}

void CAnnotationBrowserDlg::OnBnClickedShowhidebutton()
{
	// swap 'show' and 'hide'
	CString tx;
	m_showHideBtn.GetWindowText( tx);
	m_showHideBtn.SetWindowText( tx == show_str?hide_str:show_str);
}

void CAnnotationBrowserDlg::OnBnClickedRederivebutton()
{
	POSITION pos = m_wndAnnotationList.GetFirstSelectedItemPosition();
	if (pos) {
		// the actually selected item
		int nItem = m_wndAnnotationList.GetNextSelectedItem(pos);
		CAnnotationNode *node = (CAnnotationNode *)m_wndAnnotationList.GetItemData(nItem);
		
		// rederive by clearing the node contents
		if (node)
			COMTHROW(node->m_regNode->RemoveTree());
		
		// read back the values from node->regNode, this time (i)nherited values are read
		node->Read( this);
		
		LoadNodeToPanel(node);
	}
}

void CAnnotationBrowserDlg::OnBnClickedCheckGradientfill()
{
	CButton* pCheck = (CButton*)GetDlgItem(IDC_CHECK_GRADIENTFILL);
	BOOL bGradientFill = pCheck->GetCheck();

	GetDlgItem(IDC_STATIC_GRADIENTCOLOR)->EnableWindow(bGradientFill);
	GetDlgItem(IDC_COMBO_GRADIENTCOLOR)->EnableWindow(bGradientFill);
	GetDlgItem(IDC_STATIC_GRADIENTDIR)->EnableWindow(bGradientFill);
	GetDlgItem(IDC_EDIT_GRADIENTDIR)->EnableWindow(bGradientFill);
}

void CAnnotationBrowserDlg::OnBnClickedCheckCastshadow()
{
	CButton* pCheck = (CButton*)GetDlgItem(IDC_CHECK_CASTSHADOW);
	BOOL bCastShadow = pCheck->GetCheck();

	GetDlgItem(IDC_STATIC_SHADOWCOLOR)->EnableWindow(bCastShadow);
	GetDlgItem(IDC_COMBO_SHADOWCOLOR)->EnableWindow(bCastShadow);
	GetDlgItem(IDC_STATIC_SHADOWDEPTH)->EnableWindow(bCastShadow);
	GetDlgItem(IDC_EDIT_SHADOWDEPTH)->EnableWindow(bCastShadow);
	GetDlgItem(IDC_STATIC_SHADOWDIRECTION)->EnableWindow(bCastShadow);
	GetDlgItem(IDC_EDIT_SHADOWDIRECTION)->EnableWindow(bCastShadow);
}

void CAnnotationBrowserDlg::OnBnClickedCheckRoundCorner()
{
	CButton* pCheck = (CButton*)GetDlgItem(IDC_CHECK_ROUNDCORNER);
	BOOL bRoundCorner = pCheck->GetCheck();

	GetDlgItem(IDC_STATIC_CORNERRADIUS)->EnableWindow(bRoundCorner);
	GetDlgItem(IDC_EDIT_CORNERRADIUS)->EnableWindow(bRoundCorner);
}

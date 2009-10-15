// ComboBoxSelectDlg.cpp : implementation file
//

#include "stdafx.h"
#include "objectinspector.h"
#include "ComboBoxSelectDlg.h"
#include "InspectorDefs.h"
#include "ItemData.h"
#include "InPlaceCommon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComboBoxSelectDlg dialog

class CListItem;

CComboBoxSelectDlg::CComboBoxSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CComboBoxSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CComboBoxSelectDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInited = true;
	m_bClosed = false;
}


void CComboBoxSelectDlg::SetParameters(const CRect& rectBound, CListItem* pListItem, CFont* pFontWnd)
{
	m_RectBound			= rectBound;
	m_pListItem			= pListItem;
	m_pFontWnd			= pFontWnd;
}


void CComboBoxSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CComboBoxSelectDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CComboBoxSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CComboBoxSelectDlg)
	ON_WM_NCACTIVATE()
	ON_MESSAGE(HLB_SELENDOK, OnComboSelEnd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CComboBoxSelectDlg::OnNcActivate(BOOL bActive)
{
	if(m_bInited)
	{
		m_bInited=false;
	}
	else
	{
		if (!bActive && !m_bClosed) {
			m_bClosed = true;
			EndDialog(IDCANCEL);
			DWORD pos = GetMessagePos();
			CPoint msgPoint(LOWORD(pos), HIWORD(pos));
			RelayMouseClickToInspectorList(m_pParentWnd, msgPoint);
		}
		// OnOK();
	}
	return FALSE;//CDialog::OnNcActivate(bActive);
}

BOOL CComboBoxSelectDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowPos( NULL, m_RectBound.left, m_RectBound.top, m_RectBound.Width(), m_RectBound.Height(), SWP_NOZORDER );
	CRect rect( 0, 0, m_RectBound.Width(), m_RectBound.Height() + INSP_COMBOBOX_LINE_HEIGHT );
	m_lstBox.Create( LBS_NOTIFY | WS_VSCROLL, rect, this, IDC_COMBO_LISTBOX );
	m_lstBox.SetFont( m_pFontWnd );

	if ( m_pListItem->Value.dataType == ITEMDATA_BOOLEAN ) {
		m_lstBox.AddString("True");
		m_lstBox.AddString("False");
		m_lstBox.SetCurSel( m_pListItem->Value.boolVal );
	}
	else {
		for( int i=0 ; i  <= m_pListItem->Value.stringVal.GetUpperBound()  ; i++ )
			m_lstBox.AddString( m_pListItem->Value.stringVal[ i ] );
		m_lstBox.SetCurSel( m_pListItem->Value.listVal );
	}

	m_lstBox.ShowWindow(SW_SHOW);
	//m_lstBox.SetFocus();
	// m_lstBox.Invalidate();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CComboBoxSelectDlg::OnOK()
{
	if( m_pListItem->Value.dataType == ITEMDATA_BOOLEAN )
		m_pListItem->Value.SetBoolValue( ! m_lstBox.GetCurSel() );
	else
		m_pListItem->Value.listVal = m_lstBox.GetCurSel();
	m_bClosed = true;
	CDialog::OnOK();
}

LONG CComboBoxSelectDlg::OnComboSelEnd( UINT lParam, LONG /*wParam*/)
{
	// m_uiSelItem = lParam;
	OnOK();
	return TRUE;
}
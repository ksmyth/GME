// InspectorList.cpp : implementation file
//

#include "stdafx.h"
#include "objectinspector.h"
#include "InspectorList.h"
#include "InspectorDlg.h"
#include "ColourPopup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInspectorList

CInspectorList::CInspectorList(bool bCategories):m_bCategories(bCategories),m_InPlaceManager(this)
{
	m_bIsDividerDrag=FALSE;
	m_ItemHeight = 16;
	m_ComboboxLineHeight = 13;
	HDC hdc = ::GetDC(NULL);
	if (hdc)
	{
		m_ItemHeight = m_ItemHeight * GetDeviceCaps(hdc, LOGPIXELSY) / 96;
		m_ComboboxLineHeight = m_ComboboxLineHeight * GetDeviceCaps(hdc, LOGPIXELSY) / 96;
		::ReleaseDC(NULL, hdc);
	}
}

CInspectorList::~CInspectorList()
{

}


BEGIN_MESSAGE_MAP(CInspectorList, CListBox)
	//{{AFX_MSG_MAP(CInspectorList)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSelChange)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_WM_RBUTTONDOWN()
	ON_BN_CLICKED(IDC_ARROW_BUTTON, OnArrowClicked)
	ON_BN_CLICKED(IDC_EDITOR_BUTTON, OnEditorClicked)
	ON_MESSAGE(MSG_EDIT_END_OK, OnEditEndOK)
	ON_COMMAND(ID_LISTCONTEXT_RESETTODEFAULT, OnListContextResetToDefault)
	ON_COMMAND(ID_LISTCONTEXT_COPY, OnListContextCopy)
	ON_COMMAND(ID_OPENREFERED, OnOpenRefered)
	//}}AFX_MSG_MAP
	ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInspectorList message handlers

BOOL CInspectorList::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Add your specialized code here and/or call the base class
	cs.style |= WS_VSCROLL |LBS_OWNERDRAWVARIABLE | LBS_NOTIFY |
				LBS_NOINTEGRALHEIGHT | LBS_EXTENDEDSEL | LBS_HASSTRINGS;
	cs.style&=~LBS_SORT;
	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	return CListBox::PreCreateWindow(cs);
}

void CInspectorList::PreSubclassWindow()
{
	m_entryFont.CreatePointFont(INSP_ENTRY_FONT_PTSIZE, INSP_ENTRY_FONT_FACE);


	LOGFONT logFont;
	memset(&logFont, 0, sizeof(LOGFONT));
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfHeight = INSP_ENTRY_FONT_PTSIZE;
	logFont.lfWeight = FW_BOLD;
	lstrcpyn(logFont.lfFaceName, INSP_ENTRY_FONT_FACE, sizeof(logFont.lfFaceName) / sizeof(TCHAR));

	VERIFY(m_entryBoldFont.CreatePointFontIndirect(&logFont, NULL));


	m_hCurArrow=AfxGetApp()->LoadStandardCursor(IDC_ARROW);
	m_hCurSize=AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);



	CListBox::PreSubclassWindow();

}

void CInspectorList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{

	UINT nIndex = lpDrawItemStruct->itemID;
	BOOL bIsSelected=lpDrawItemStruct->itemState & ODS_SELECTED;
	BOOL bIsDisabled=lpDrawItemStruct->itemState & ODS_DISABLED;

	// Obtaining the DeviceContext
	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);
	int savedDC = dc.SaveDC();


	// If the item exists
	if (nIndex != (UINT) -1)
	{
		COLORREF crSelBackGround=::GetSysColor(COLOR_HIGHLIGHT);
		COLORREF crNormBackGround=::GetSysColor(COLOR_WINDOW);
		COLORREF crDisabledBackGround=::GetSysColor(COLOR_INACTIVEBORDER);

		COLORREF crSelText=::GetSysColor(COLOR_HIGHLIGHTTEXT);
		COLORREF crNormText=::GetSysColor(COLOR_WINDOWTEXT);
		COLORREF crDisabledText=::GetSysColor(COLOR_GRAYTEXT);

		COLORREF crText=bIsSelected?crSelText:crNormText;
		COLORREF crBckGr=bIsSelected?crSelBackGround:crNormBackGround;



		CListItem& ListItem=m_ListItemArray[nIndex];



		// Getting the full rectangle for the item
		CRect rectFull = lpDrawItemStruct->rcItem;

		// Setting up one rectangle for each columns
		CRect rectLeftMargin(rectFull),rectLeft(rectFull),rectRight(rectFull);
		if(m_bCategories)
		{
			rectLeftMargin.right=INSP_LEFT_MARGIN;
		}
		else
		{
			rectLeftMargin.right=rectLeftMargin.left;
		}

		rectLeft.left=rectLeftMargin.right;
		rectLeft.right=m_Settings.m_nDivider;
		rectRight.left=m_Settings.m_nDivider;



		// Repainting the background

		// Left margin
		if(m_bCategories)
		{
			dc.FillSolidRect(rectLeftMargin,crNormBackGround);
		}

		// Left side (name)
		dc.FillSolidRect(rectLeft.left-1,rectLeft.top-1,rectLeft.Width()+1,rectLeft.Height()+1,crBckGr);

		// Not selecting right panel side - only if it's default
		if(ListItem.bIsDefault)
		{
			// crText=crDisabledText;     -- Modification by Volgyesi
			crText=bIsSelected ? crText : crDisabledText;
			//crBckGr=crDisabledBackGround;
		}

		if(ListItem.bIsDifferentValue)
		{
			dc.FillSolidRect(rectRight.left,rectRight.top-1,rectRight.Width(),rectRight.Height()+1,crDisabledBackGround);
		}
		else
		{
			dc.FillSolidRect(rectRight.left,rectRight.top-1,rectRight.Width(),rectRight.Height()+1,crNormBackGround);
		}

		static CBrush Brush(crNormText);
		static CPen Pen(PS_SOLID,0,crNormText);
		static CPen PenInactiveBorder(PS_SOLID,0,crDisabledBackGround);


		// PlusMinus Button
		if(m_bCategories)
		{

			if(ListItem.bIsContainer)
			{
				int nXPlusMinusOffset=(rectLeftMargin.Width()-INSP_PLUS_MINUS_BUTTON_SIZE)/2;
				int nYPlusMinusOffset=(rectLeftMargin.Height()-INSP_PLUS_MINUS_BUTTON_SIZE)/2;
				CRect rectPlusMinus(rectLeftMargin.left+nXPlusMinusOffset,
									rectLeftMargin.top+nYPlusMinusOffset,
									rectLeftMargin.left+nXPlusMinusOffset+INSP_PLUS_MINUS_BUTTON_SIZE,
									rectLeftMargin.top+nYPlusMinusOffset+INSP_PLUS_MINUS_BUTTON_SIZE);

				CPoint ptCenter(rectPlusMinus.CenterPoint());


				dc.FrameRect(rectPlusMinus,&Brush);

				if(!ListItem.bIsContainerOpen)
				{
					// perpendicular component of + sign
					dc.MoveTo(ptCenter.x, ptCenter.y - 2);
					dc.LineTo(ptCenter.x, ptCenter.y + 3);
				}
				// - sign
				dc.MoveTo(ptCenter.x - 2, ptCenter.y);
				dc.LineTo(ptCenter.x + 3, ptCenter.y);
			}
		}


		// Drawing edges - But we have to increase the rect
		if(!rectLeft.IsRectEmpty())
		{
			dc.SelectObject(&PenInactiveBorder);
			rectLeft.InflateRect(2,1,0,0);
			dc.DrawEdge(rectLeft,EDGE_SUNKEN,BF_BOTTOMRIGHT);

			dc.MoveTo(rectLeft.TopLeft());
			dc.LineTo(rectLeft.left,rectLeft.bottom);

			rectLeft.DeflateRect(2,1,0,0);
		}

		if(!rectRight.IsRectEmpty())
		{
			rectRight.InflateRect(1,0,0,0);
			dc.DrawEdge(rectRight,EDGE_SUNKEN,BF_BOTTOM);
			rectRight.DeflateRect(1,0,0,0);
		}

		// Attribute name
		CString strName=ListItem.strName;
		CString strTxt;
		ListItem.Value.toString(strTxt);

		dc.SelectObject(&m_entryFont);
		dc.SetBkMode(TRANSPARENT);

		// Container
		if(ListItem.bIsContainer)
		{
			dc.SelectObject(&m_entryBoldFont);
		}
		else // Normal data types
		{
			dc.SetTextColor(ListItem.bIsDefault?crDisabledText:crNormText);

			if(ListItem.Value.dataType==ITEMDATA_COLOR) // Color
			{
				CRect rectColorBox(rectRight);
				rectColorBox.left+=INSP_COLORBOX_MARGIN;
				rectColorBox.right=rectColorBox.left+INSP_COLORBOX_SIZE;
				rectColorBox.top=rectColorBox.top+(rectColorBox.Height()-INSP_COLORBOX_SIZE)/2-1;
				rectColorBox.bottom=rectColorBox.top+INSP_COLORBOX_SIZE;

				dc.FillSolidRect(rectColorBox,ListItem.Value.colorVal);
				dc.FrameRect(rectColorBox,&Brush);

				CRect rectText(rectRight);
				rectText.left=rectColorBox.right+2*INSP_COLORBOX_MARGIN;

				dc.DrawText(strTxt,CRect(rectText.left,rectText.top,
								rectText.right,rectText.bottom),
				DT_LEFT | DT_SINGLELINE);
			}
			else if (ListItem.Value.dataType == ITEMDATA_BOOLEAN) {
				CRect rectColorBox(rectRight);
				rectColorBox.left += INSP_COLORBOX_MARGIN;
				rectColorBox.right = rectColorBox.left + INSP_CHECKBOX_SIZE;
				rectColorBox.top = rectColorBox.top + (rectColorBox.Height() - INSP_CHECKBOX_SIZE) / 2 - 1;
				rectColorBox.bottom = rectColorBox.top + INSP_CHECKBOX_SIZE;
				dc.DrawFrameControl(rectColorBox, DFC_BUTTON, DFCS_BUTTONCHECK | (ListItem.Value.boolVal ? DFCS_CHECKED : 0));

				CRect rectText(rectRight);
				rectText.left = rectColorBox.right + 2 * INSP_COLORBOX_MARGIN;

				dc.DrawText(strTxt, CRect(rectText.left, rectText.top,
					rectText.right, rectText.bottom),
					DT_LEFT | DT_SINGLELINE);
			}
			else if(ListItem.Value.dataType==ITEMDATA_STRING&&ListItem.Value.cLineNum>1)
			{
				// Multiline string
				CString strTxt;
				ListItem.Value.toString(strTxt);
				dc.DrawText(strTxt,CRect(rectRight.left,rectRight.top,
										rectRight.right,rectRight.bottom),DT_LEFT | DT_NOPREFIX);
				//DT_NOPREFIX by zolmol: draw string as is, do not interpret the & character
			}
			else
			{
				// Do not select the right hand side

				dc.DrawText(strTxt,CRect(rectRight.left,rectRight.top,
								rectRight.right,rectRight.bottom),
				DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
				//DT_NOPREFIX by zolmol: draw string as is, do not interpret the & character			
			}
		}
		dc.SetTextColor(crText);
		dc.DrawText(strName,CRect(rectLeft.left,rectLeft.top,
											rectLeft.right-2,rectLeft.bottom),
					DT_LEFT | DT_SINGLELINE);
	}

	dc.RestoreDC(savedDC);
	dc.Detach();


}

void CInspectorList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	int nIndex=lpMeasureItemStruct->itemID;
	if(m_ListItemArray[nIndex].Value.dataType==ITEMDATA_STRING)
	{
		int nLineNum=m_ListItemArray[nIndex].Value.cLineNum;
		if(nLineNum==1)
		{
			lpMeasureItemStruct->itemHeight = m_ItemHeight;
		}
		else
		{
			lpMeasureItemStruct->itemHeight = min(255, nLineNum * m_ItemHeight);
		}

	}
	else
	{
		lpMeasureItemStruct->itemHeight = m_ItemHeight;
	}

}

void CInspectorList::DoCollapseExpand(int nIndex)
{
	m_InPlaceManager.HideAllInPlace();

	CListItem &ListItem=m_ListItemArray.ElementAt(nIndex);
	if(!ListItem.bIsContainer) return;

	if(ListItem.bIsContainerOpen)
	{
		int nChildIndex=nIndex+1;
		ListItem.m_ContainedListItemArray.RemoveAll();

		while(nChildIndex<=m_ListItemArray.GetUpperBound())
		{
			CListItem ChildListItem=m_ListItemArray.GetAt(nChildIndex);

			if(ChildListItem.bIsContainer)break;

			ListItem.m_ContainedListItemArray.Add(ChildListItem);

			RemoveItem(nChildIndex);
		}

		// Setting the +/-sign
		ListItem.bIsContainerOpen=FALSE;
	}
	else
	{
		int nChildIndex=nIndex+1;
		for(int i=0;i<=ListItem.m_ContainedListItemArray.GetUpperBound();i++)
		{
			CListItem ChildListItem=ListItem.m_ContainedListItemArray.GetAt(i);
			m_ListItemArray.InsertAt(nChildIndex+i,ChildListItem);
			InsertString(nChildIndex+i,_T(""));

		}

		ListItem.m_ContainedListItemArray.RemoveAll();

		// Setting the +/-sign
		ListItem.bIsContainerOpen=TRUE;
	}
	Invalidate();

}


void CInspectorList::OnLButtonDown(UINT nFlags, CPoint point)
{
	if(GetCount()==0)return;

	if(m_bCategories)
	{
		OnPlusMinusClick(point);
	}
	if (OnRightSideClick(point))
		return;

	if(point.x>=(int)m_Settings.m_nDivider-INSP_MOUSE_RADIUS &&
		point.x<=(int)m_Settings.m_nDivider+INSP_MOUSE_RADIUS)
	{

		m_bIsDividerDrag=TRUE;

		::SetCursor(m_hCurSize);

		CRect rectWnd;
		GetWindowRect(rectWnd);
		rectWnd.left += INSP_LEFT_MARGIN+3; rectWnd.right -= 20;

		//Mouse within the listbox
		::ClipCursor(rectWnd);

		//Capture the mouse
		SetCapture();


		CRect rectClient;
		GetClientRect(rectClient);

		m_ptOldTop.y=rectClient.top;
		m_ptOldTop.x=point.x;

		m_ptOldBottom.y=rectClient.bottom;
		m_ptOldBottom.x=point.x;

		CClientDC dc(this);
		InvertLine(&dc,m_ptOldTop,m_ptOldBottom);

	}
	else
	{
		CListBox::OnLButtonDown(nFlags, point);
	}
}

void CInspectorList::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(GetCount()==0)return;

	if(m_bIsDividerDrag)
	{
		m_bIsDividerDrag=FALSE;
		m_Settings.m_nDivider = point.x;

		//Release the mouse
		if (this==GetCapture())
		{
			::ReleaseCapture();
		}

		// Allow the cursor to go outside
		::ClipCursor(NULL);

		// Arrow cursor
		::SetCursor(m_hCurSize);

		m_InPlaceManager.HideAllInPlace();
		// Redraw listbox
		Invalidate();
	}


	BOOL bOutside;
	int nIndex = ItemFromPoint(point, bOutside);

	if (!bOutside)
	{
		CRect rectItem;
		GetItemRect(nIndex, rectItem);

		CRect rectRight(rectItem);
		rectRight.left = m_Settings.m_nDivider;

		if (rectRight.PtInRect(point))
		{
			int cxDrag = GetSystemMetrics(SM_CXDRAG);
			int cyDrag = GetSystemMetrics(SM_CYDRAG);
			int nIndexMouseDown = ItemFromPoint(point, bOutside);
			if (!bOutside && nIndex == nIndexMouseDown && abs(m_ptOldTop.x - point.x) <= cxDrag && abs(m_ptOldTop.y - point.y) <= cyDrag)
			{
				CListItem& ListItem = m_ListItemArray[nIndex];
				if (ListItem.Value.dataType == ITEMDATA_BOOLEAN) {
					ListItem.Value.SetBoolValue(!ListItem.Value.boolVal);
					ListItem.SetDirty();
					NotifyParent(nIndex);
					SetFocus();
					return;
				}
			}
		}
	}

	CListBox::OnLButtonUp(nFlags, point);
}

void CInspectorList::OnMouseMove(UINT nFlags, CPoint point)
{
	if(GetCount()==0)return;

	if(m_bIsDividerDrag)
	{
		// Deleting the previous line
		CClientDC dc(this);
		InvertLine(&dc,m_ptOldTop,m_ptOldBottom);

		// Drawing a new one
		m_ptOldTop.x=point.x;
		m_ptOldBottom.x=point.x;
		InvertLine(&dc,m_ptOldTop,m_ptOldBottom);

	}
	else
	{
		if(point.x>=(int)m_Settings.m_nDivider-INSP_MOUSE_RADIUS &&
			point.x<=(int)m_Settings.m_nDivider+INSP_MOUSE_RADIUS)
		{
			::SetCursor(m_hCurSize);
		}
		else
		{
			::SetCursor(m_hCurArrow);
		}

	}

	CListBox::OnMouseMove(nFlags, point);
}

void CInspectorList::InvertLine(CDC *pDC, CPoint ptSrc, CPoint ptDst)
{

	int nOldMode = pDC->SetROP2(R2_NOT);

	pDC->MoveTo(ptSrc);
	pDC->LineTo(ptDst);

	pDC->SetROP2(nOldMode);
}

void CInspectorList::OnSelChange()
{
	int nSelCount=GetSelCount();

	if(nSelCount<1)
	{
		SetHelp(-1);
	}
	else
	{

		int nSelected;
		CListBox::GetSelItems(1,&nSelected);

		CRect rectItem;
		GetItemRect(nSelected,rectItem);

		SetHelp(nSelected);
		rectItem.left=m_Settings.m_nDivider;
		m_InPlaceManager.ShowInPlace(rectItem,nSelected);
	}

}


void CInspectorList::OnArrowClicked()
{

	m_InPlaceManager.OnClickArrowButton(false);

}

void CInspectorList::OnEditorClicked()
{

	m_InPlaceManager.OnClickEditorButton();

}


bool CInspectorList::InsertItem(CListItem ListItem, int nIndex)
{
	INT_PTR nUpperBound=m_ListItemArray.GetUpperBound();

	if (nIndex > nUpperBound)
	{
		return AddItem(ListItem);
	}

	if(!m_bCategories||nUpperBound==-1)
	{
		m_ListItemArray.InsertAt(nIndex,ListItem);
		InsertString(nIndex,_T(""));
	}
	else
	{
		int nLastCategoryIndex=nIndex;
		bool bIsOpen;
		do
		{
			const CListItem &LastCategoryListItem=m_ListItemArray.ElementAt(nLastCategoryIndex);
			if(LastCategoryListItem.bIsContainer)
			{
				bIsOpen=LastCategoryListItem.bIsContainerOpen;
				break;
			}
			nLastCategoryIndex--;
		}while(nLastCategoryIndex>=0);


		if(nLastCategoryIndex!=-1 && !bIsOpen)
		{
			CListItem & LastCategoryListItem =m_ListItemArray.ElementAt(nLastCategoryIndex);
			LastCategoryListItem.m_ContainedListItemArray.Add(ListItem);
		}
		else
		{
			m_ListItemArray.InsertAt(nIndex,ListItem);
			InsertString(nIndex,_T(""));

		}

	}


	// Checking value
	if(!m_bCategories)
	{
		ASSERT(ListItem.Value.dataType!=ITEMDATA_NULL);
		return false;
	}
	else
	{
		if(ListItem.Value.dataType==ITEMDATA_NULL && ListItem.bIsContainer==false)
		{
			// We should not add null items not containing any type
			ASSERT(("ITEMDATA_NULL type data was to be inserted in the list",false));
			return false;
		}
		else
		{
			return true;
		}
	}
}


void CInspectorList::GetSelItems(CArray<int,int>&IndexArray,CArray<CListItem,CListItem&> &ListItemArray)
{
	IndexArray.RemoveAll();
	ListItemArray.RemoveAll();

	int nSelCount=GetSelCount();

	if(nSelCount>0)
	{
		IndexArray.SetSize(nSelCount);
		CListBox::GetSelItems(nSelCount,IndexArray.GetData());
	}

	for(int i=0;i<=IndexArray.GetUpperBound();i++)
	{
		ListItemArray.Add(m_ListItemArray.ElementAt(IndexArray[i]));
	}
}


bool CInspectorList::AddItem(CListItem ListItem)
{

	INT_PTR nUpperBound=m_ListItemArray.GetUpperBound();

	if(!m_bCategories||nUpperBound==-1)
	{
		m_ListItemArray.Add(ListItem);
		AddString(_T(""));
	}
	else
	{
		int nLastCategoryIndex=nUpperBound;
		bool bIsOpen;
		do
		{
			const CListItem &LastCategoryListItem=m_ListItemArray.ElementAt(nLastCategoryIndex);
			if(LastCategoryListItem.bIsContainer)
			{
				bIsOpen=LastCategoryListItem.bIsContainerOpen;
				break;
			}
			nLastCategoryIndex--;
		}while(nLastCategoryIndex>=0);


		if(nLastCategoryIndex!=-1 && !bIsOpen)
		{
			CListItem & LastCategoryListItem =m_ListItemArray.ElementAt(nLastCategoryIndex);
			LastCategoryListItem.m_ContainedListItemArray.Add(ListItem);
		}
		else
		{
			m_ListItemArray.Add(ListItem);
			AddString(_T(""));
		}
	}

	RefreshState();
	// Checking value
	if(!m_bCategories)
	{
		ASSERT(ListItem.Value.dataType!=ITEMDATA_NULL);
		return false;
	}
	else
	{
		if(ListItem.Value.dataType==ITEMDATA_NULL && ListItem.bIsContainer==false)
		{
			// We should not add null items not containing any type
			ASSERT(("ITEMDATA_NULL type data was to be inserted in the list",false));
			return false;
		}
		else
		{
			return true;
		}
	}
}


void CInspectorList::UpdateItems(CArray<CListItem,CListItem&> &ListItemArray)
{
	INT_PTR nUpperBound=ListItemArray.GetUpperBound();
	int nIndex=0;

	for(int i=0;i<=m_ListItemArray.GetUpperBound();i++)
	{
		CListItem& ListItem=m_ListItemArray.ElementAt(i);

		if(nIndex>nUpperBound)
		{
			CInspectorList::RemoveItem(i);
			i--;
			continue;
		}
		else
		{
			UpdateItem(ListItemArray.ElementAt(nIndex),ListItem, nIndex);
			nIndex++;
		}

		if(ListItem.bIsContainer&&!ListItem.bIsContainerOpen)
		{
			for(int j=0;j<=ListItem.m_ContainedListItemArray.GetUpperBound();j++)
			{
				CListItem & InnerListItem=ListItem.m_ContainedListItemArray.ElementAt(j);
				if(nIndex>nUpperBound)
				{
					ListItem.m_ContainedListItemArray.RemoveAt(j);
					j--;
					continue;
				}
				else
				{
					UpdateItem(ListItemArray.ElementAt(nIndex),InnerListItem, nIndex);
					nIndex++;
				}
			}
		}
	}

	INT_PTR nLastElement=m_ListItemArray.GetUpperBound();
	if(nLastElement>=0)
	{
		const CListItem& LastListItem=m_ListItemArray.ElementAt(nLastElement);

		if(LastListItem.bIsContainer&& !LastListItem.bIsContainerOpen && nIndex<=nUpperBound)
		{
			DoCollapseExpand(nLastElement);
		}
	}
	for(int k=nIndex;k<=nUpperBound;k++)
	{
		CInspectorList::AddItem(ListItemArray.ElementAt(k));
	}
	if (m_InPlaceManager.m_ArrowButton.m_hWnd)
		m_InPlaceManager.m_ArrowButton.ShowWindow(SW_HIDE);
	if (m_InPlaceManager.m_EditorButton.GetSafeHwnd())
		m_InPlaceManager.m_EditorButton.ShowWindow(SW_HIDE);

	Invalidate();
}


void CInspectorList::UpdateItem(const CListItem &srcListItem, CListItem &dstListItem, int nIndex)
{
	ASSERT(nIndex < GetCount());
	dstListItem.CopyWithNoState(srcListItem);


	// Unfortunately, Windows sends MeasureItem message only once - Volgyesi
	if(srcListItem.Value.dataType==ITEMDATA_STRING)
	{
		int nLineNum=srcListItem.Value.cLineNum;
		if(nLineNum==1)
		{
			SetItemHeight(nIndex, m_ItemHeight);
		}
		else
		{
			VERIFY(SetItemHeight(nIndex, min(255, nLineNum * m_ItemHeight)) == LB_OKAY);
		}

	}
	else
	{
		SetItemHeight(nIndex, m_ItemHeight);
	}
	// Modification End - Volgyesi
}


bool CInspectorList::RemoveItem(int nIndex)
{
	if(nIndex<=m_ListItemArray.GetUpperBound())
	{
		m_InPlaceManager.HideAllInPlace();
		m_ListItemArray.RemoveAt(nIndex);
		VERIFY(DeleteString(nIndex)!=LB_ERR);
		return true;
	}
	return false;
}


void CInspectorList::RemoveAll()
{
	m_ListItemArray.RemoveAll();
	CInspectorList::ResetContent();
}


void CInspectorList::ResetContent()
{
	m_InPlaceManager.HideAllInPlace();
	CListBox::ResetContent();
}


void CInspectorList::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (!OnPlusMinusClick(point))
		OnLButtonDown(nFlags, point);

	//CListBox::OnLButtonDblClk(nFlags, point);
}


bool CInspectorList::OnPlusMinusClick(CPoint point)
{
	BOOL bOutside;
	int nIndex=ItemFromPoint(point,bOutside);
	if (nIndex==65535)
		return false;
	if (bOutside)
		return false;

	CListItem ListItem=m_ListItemArray.GetAt(nIndex);
	if(!ListItem.bIsContainer)
		return false;

	CRect rectItem;
	GetItemRect(nIndex,rectItem);
	CRect rectLeftMargin(rectItem);
	rectLeftMargin.right=INSP_LEFT_MARGIN;
	int nXPlusMinusOffset=(rectLeftMargin.Width()-INSP_PLUS_MINUS_BUTTON_SIZE)/2;
	int nYPlusMinusOffset=(rectLeftMargin.Height()-INSP_PLUS_MINUS_BUTTON_SIZE)/2;
	CRect rectPlusMinus(rectLeftMargin.left+nXPlusMinusOffset,
						rectLeftMargin.top+nYPlusMinusOffset,
						rectLeftMargin.left+nXPlusMinusOffset+INSP_PLUS_MINUS_BUTTON_SIZE,
						rectLeftMargin.top+nYPlusMinusOffset+INSP_PLUS_MINUS_BUTTON_SIZE);
	if(rectPlusMinus.PtInRect(point))
	{
		// If it was a click on the correct place
		DoCollapseExpand(nIndex);
		return true;
	}

	return false;
}


bool CInspectorList::OnRightSideClick(CPoint point)
{
	BOOL bOutside;
	int nIndex=ItemFromPoint(point,bOutside);

	if(!bOutside)
	{
		CRect rectItem;
		GetItemRect(nIndex,rectItem);

		CRect rectRight(rectItem);
		rectRight.left=m_Settings.m_nDivider;

		if(rectRight.PtInRect(point))
		{
			m_ptOldTop.y = point.y;
			m_ptOldTop.x = point.x;
			return m_InPlaceManager.OnRightItemClick(nIndex, rectRight);
		}
	}

	return false;
}


void CInspectorList::SetHelp(int nIndex)
{

	CWnd* pParent=GetParent();
	CInspectorDlg*pInspectorDlg=(CInspectorDlg*)pParent->GetParent();

	if(nIndex==-1)
	{
		pInspectorDlg->SetHelp(_T(""),_T(""));
		return;
	}


	int nSelCount=GetSelCount();
	if(nSelCount>1)
	{
		pInspectorDlg->SetHelp(_T("Multiple Selection"),_T("Select one item to edit its properties \r\nor press Ctrl+D to make selected items default."));
	}
	else
	{
		CListItem ListItem=m_ListItemArray.GetAt(nIndex);
		pInspectorDlg->SetHelp(ListItem.strName,ListItem.strToolTip);
	}
}


bool CInspectorList::SelectNextItem(BOOL reverse)
{
	int nCount = GetCount();
	if (nCount > 1) {
		int nCurSel = GetCurSel();	// In a multiple-selection list box, the index of the item that has the focus.
		nCurSel = nCurSel + (reverse ? -1 : 1);
		nCurSel = (nCurSel + nCount) % nCount;
		// Clear current selections
		// Get the indexes of all the selected items.
		int nSelCount = GetSelCount();
		CArray<int,int> arrListBoxSel;
		arrListBoxSel.SetSize(nSelCount);
		CListBox::GetSelItems(nSelCount, arrListBoxSel.GetData());
		for(int i = 0; i < nSelCount; i++)
			SetSel(arrListBoxSel.GetAt(i), FALSE);

		// Select the next focused
		SetSel(nCurSel, TRUE);
		OnSelChange();

		// TODO: pop up comboboxes (tricky because they're not really comboboxes)
		//CListItem& ListItem=m_ListItemArray.ElementAt(nCurSel);
		//if (ListItem.Value.dataType == ITEMDATA_BOOLEAN || ListItem.Value.dataType == ITEMDATA_FIXED_LIST) {
		//	m_InPlaceManager.OnClickArrowButton(false);
		//}
		return true;
	}
	return false;
}


LRESULT CInspectorList::OnEditEndOK(WPARAM /*lParam*/, LPARAM /*wParam*/)
{

	m_InPlaceManager.OnEditEnd();

	return TRUE;
}


void CInspectorList::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	m_InPlaceManager.HideAllInPlace();

	CListBox::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL CInspectorList::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	m_InPlaceManager.HideAllInPlace();
	return CListBox::OnMouseWheel(nFlags, zDelta, pt);
}



void CInspectorList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch(nChar)
	{
		case 'D':	// That's all abaout Microsoft VK_ codes.
			{
				if(::GetKeyState(VK_CONTROL) & 0x8000)
				{
					SetDefault();
					Invalidate();
					SetFocus();

				}
			}
			break;
		case VK_DELETE:
			{
				SetDefault();
				Invalidate();
				SetFocus();
			}
			break;
		case VK_TAB:	// JIRA GME-178
			{
				SelectNextItem(::GetKeyState(VK_SHIFT) & 0x8000);
			}
			break;
		case VK_SPACE:
		case VK_RETURN:
		{
			int nSelCount = GetSelCount();

			CArray<int, int> IndexArray;
			if (nSelCount > 0)
			{
				IndexArray.SetSize(nSelCount);
				CListBox::GetSelItems(nSelCount, IndexArray.GetData());
			}
			for (int i = 0; i < nSelCount; i++) {
				int nIndex = IndexArray[i];
				CListItem& ListItem = m_ListItemArray[nIndex];
				if (ListItem.Value.dataType == ITEMDATA_BOOLEAN) {
					ListItem.Value.SetBoolValue(!ListItem.Value.boolVal);
					ListItem.SetDirty();
					NotifyParent(nIndex);
					SetFocus();
				}
				else if (ListItem.Value.dataType == ITEMDATA_FIXED_LIST) {
				}
			}
		}
	}

	CListBox::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CInspectorList::SetDefault()
{
	int nSelCount=GetSelCount();

	CArray<int,int> SelItemArr;
	SelItemArr.SetSize(nSelCount);

	CListBox::GetSelItems(nSelCount,SelItemArr.GetData());

	for(int i=0;i<=SelItemArr.GetUpperBound();i++)
	{
		int nCurr=SelItemArr.GetAt(i);

		CListItem& ListItem=m_ListItemArray.ElementAt(nCurr);
		ListItem.RestoreDefault();

		NotifyParent(nCurr);
	}
}

void CInspectorList::OnSize(UINT nType, int cx, int cy)
{
	m_InPlaceManager.HideAllInPlace();
	CListBox::OnSize(nType, cx, cy);

}

bool CInspectorList::FindByKey(void* dwKey,CListItem& FoundListItem )
{
	for(int i=0;i<=m_ListItemArray.GetUpperBound();i++)
	{
		CListItem& ListItem=m_ListItemArray.ElementAt(i);
		if(ListItem.dwKeyValue==dwKey)
		{
			FoundListItem= ListItem;
			return true;
		}
		else
		{
			if(ListItem.bIsContainer&&!ListItem.bIsContainerOpen)
			{
				for(int j=0;j<=ListItem.m_ContainedListItemArray.GetUpperBound();j++)
				{
					CListItem & InnerListItem=ListItem.m_ContainedListItemArray.ElementAt(j);
					if(InnerListItem.dwKeyValue==dwKey)
					{
						FoundListItem= InnerListItem;
						return true;
					}
				}
			}
		}
	}
	return false;

}


bool CInspectorList::FindByKey(void* dwKey,DWORD dwUserData, CListItem& FoundListItem )
{
	for(int i=0;i<=m_ListItemArray.GetUpperBound();i++)
	{
		CListItem& ListItem=m_ListItemArray.ElementAt(i);
		if(ListItem.dwKeyValue==dwKey && ListItem.dwUserData==dwUserData)
		{
			FoundListItem= ListItem;
			return true;
		}
		else
		{
			if(ListItem.bIsContainer&&!ListItem.bIsContainerOpen)
			{
				for(int j=0;j<=ListItem.m_ContainedListItemArray.GetUpperBound();j++)
				{
					CListItem & InnerListItem=ListItem.m_ContainedListItemArray.ElementAt(j);
					if(InnerListItem.dwKeyValue==dwKey && InnerListItem.dwUserData==dwUserData)
					{
						FoundListItem= InnerListItem;
						return true;
					}
				}
			}
		}
	}
	return false;

}


void CInspectorList::DeleteByKey(void* dwKey, DWORD dwUserData)
{
	for(int i=0;i<=m_ListItemArray.GetUpperBound();i++)
	{
		CListItem& ListItem=m_ListItemArray.ElementAt(i);
		if(ListItem.dwKeyValue==dwKey && ListItem.dwUserData==dwUserData)
		{
			// Remove from array
			m_ListItemArray.RemoveAt(i);
			// Remove from listbox
			DeleteString(i);
			// Refresh listbox
			Invalidate();
			return;

		}
		else
		{
			if(ListItem.bIsContainer&&!ListItem.bIsContainerOpen)
			{
				for(int j=0;j<=ListItem.m_ContainedListItemArray.GetUpperBound();j++)
				{
					CListItem & InnerListItem=ListItem.m_ContainedListItemArray.ElementAt(j);
					if(InnerListItem.dwKeyValue==dwKey && InnerListItem.dwUserData==dwUserData)
					{
						// Remove from inner array
						ListItem.m_ContainedListItemArray.RemoveAt(j);
						return;
					}
				}
			}
		}
	}
}


void CInspectorList::DeleteByKey(void* dwKey)
{

	for(int i=0;i<=m_ListItemArray.GetUpperBound();i++)
	{
		CListItem& ListItem=m_ListItemArray.ElementAt(i);
		if(ListItem.dwKeyValue==dwKey)
		{
			// Remove from array
			m_ListItemArray.RemoveAt(i);
			// Remove from listbox
			DeleteString(i);
			// Refresh listbox
			Invalidate();
			return;

		}
		else
		{
			if(ListItem.bIsContainer&&!ListItem.bIsContainerOpen)
			{
				for(int j=0;j<=ListItem.m_ContainedListItemArray.GetUpperBound();j++)
				{
					CListItem & InnerListItem=ListItem.m_ContainedListItemArray.ElementAt(j);
					if(InnerListItem.dwKeyValue==dwKey)
					{
						// Remove from inner array
						ListItem.m_ContainedListItemArray.RemoveAt(j);
						return;
					}
				}
			}
		}
	}
}


void CInspectorList::NotifyParent(UINT nSelItem)
{
	GetParent()->SendMessage(LBN_ON_ITEM_CHANGED,nSelItem,0);
}

void CInspectorList::RefreshState()
{
	int nSelCount = GetSelCount();

	if (nSelCount<1)
	{
		SetHelp(-1);
	}
	else
	{
		int nSelected;
		CListBox::GetSelItems(1, &nSelected);
		SetHelp(nSelected);
	}
	// PETER: Removed beacuse of JIRA #GME-64
	// OnSelChange();
}


void CInspectorList::GetItem(int nIndex, CListItem &ListItem)
{
	ListItem=m_ListItemArray[nIndex];
}


void CInspectorList::OnRButtonDown(UINT nFlags, CPoint point)
{
	CWnd* pParent = GetParent();
	CInspectorDlg* pInspectorDlg = (CInspectorDlg*)pParent->GetParent();

	// if clicked item is not selected, select only it
	BOOL outside;
	auto clickedIndex = ItemFromPoint(point, outside);
	if (!outside) {
		if (GetSel(clickedIndex) <= 0) {
			for (int i = 0; i < GetCount(); i++)
			{
				SetSel(i, FALSE);
			}
			SetSel(clickedIndex);
		}
	}

	int nSelCount = GetSelCount();
	CArray<int, int> SelItemArr;
	SelItemArr.SetSize(nSelCount);

	CListBox::GetSelItems(nSelCount, SelItemArr.GetData());

	bool bIsCustom = false;

	for (int i = 0; i <= SelItemArr.GetUpperBound(); i++)
	{
		int nCurr = SelItemArr.GetAt(i);

		CListItem& ListItem = m_ListItemArray.ElementAt(nCurr);
		if (!ListItem.bIsDefault) {
			bIsCustom = true;
			break;
		}
	}

	if (this == pInspectorDlg->m_inspectorLists[0] || this == pInspectorDlg->m_inspectorLists[1]) {

		CMenu menu;
		menu.LoadMenu(IDR_LISTCNTX_MENU);
		if (!bIsCustom) {
			menu.GetSubMenu(0)->EnableMenuItem(ID_LISTCONTEXT_RESETTODEFAULT, MF_GRAYED);
		}
		if (nSelCount == 1 && m_ListItemArray.ElementAt(SelItemArr.GetAt(0)).Value.dataType == ITEMDATA_STRING) {
		}
		else {
			menu.GetSubMenu(0)->EnableMenuItem(ID_LISTCONTEXT_COPY, MF_GRAYED);
		}
		ClientToScreen(&point);
		menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON,
			point.x, point.y, this);
	}
	else if (this == pInspectorDlg->m_inspectorLists[2]) {
		if (nSelCount == 1) {
			CListItem& listItem = m_ListItemArray.ElementAt(SelItemArr.GetAt(0));
			if (listItem.strName == L"Type") {
				CMenu menu;
				menu.CreatePopupMenu();
				menu.AppendMenuW(MF_STRING, ID_LISTCONTEXT_RESETTODEFAULT, L"Detach from Archetype");

				const wchar_t* archetype = L" (Archetype)";
				auto value = listItem.Value.stringVal.GetAt(0);
				if ((size_t)value.GetLength() >= wcslen(archetype) && wcscmp(archetype, value.Right(wcslen(archetype))) == 0) {
					// FIXME: should disable for non-primary derived
					menu.EnableMenuItem(ID_LISTCONTEXT_RESETTODEFAULT, MF_GRAYED);
				}

				ClientToScreen(&point);
				menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
			}
			else if (listItem.strName == L"References") {
				CMenu menu;
				menu.CreatePopupMenu();
				menu.AppendMenuW(MF_STRING, ID_OPENREFERED, L"Open refered");

				const wchar_t* objectID = L"ObjectID=";
				auto& value = listItem.Value.stringVal.GetAt(0);
				if (value.Find(objectID) == -1) {
					menu.EnableMenuItem(ID_OPENREFERED, MF_GRAYED);
				}

				ClientToScreen(&point);
				menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
			}
			else if (listItem.strName == L"GUID" || listItem.strName == L"Object ID") {
				CMenu menu;
				menu.CreatePopupMenu();
				menu.AppendMenuW(MF_STRING, ID_LISTCONTEXT_COPY, L"Copy");

				ClientToScreen(&point);
				auto cmd = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
				if (cmd == ID_LISTCONTEXT_COPY)
				{
					const wchar_t* text = listItem.Value.stringVal[0];
					const size_t len = wcslen(text) + 1;
					HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
					memcpy(GlobalLock(hMem), text, len * sizeof(wchar_t));
					GlobalUnlock(hMem);
					if (OpenClipboard() == 0)
					{
						return;
					}
					EmptyClipboard();
					VERIFY(SetClipboardData(CF_UNICODETEXT, hMem));
					CloseClipboard();
				}
			}
		}

	}

	CListBox::OnRButtonDown(nFlags, point);
}

void CInspectorList::OnListContextResetToDefault()
{
	SetDefault();
	Invalidate();
}

void CInspectorList::OnOpenRefered()
{
	GetParent()->GetParent()->SendMessage(LBN_ON_OPEN_REFERED, 0, 0);
}

void SetClipboardText(const CString& szData)
{
	HGLOBAL h;
	LPTSTR arr;

	h = GlobalAlloc(GMEM_MOVEABLE, (szData.GetLength() + 1) * sizeof(TCHAR));
	arr = (LPTSTR)GlobalLock(h);
	_tcscpy_s((TCHAR*)arr, szData.GetLength() + 1, static_cast<const wchar_t*>(szData));
	GlobalUnlock(h);

	::OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, h);
	CloseClipboard();
}

void CInspectorList::OnListContextCopy()
{
	int nSelCount = GetSelCount();

	CArray<int, int> SelItemArr;
	SelItemArr.SetSize(nSelCount);

	CListBox::GetSelItems(nSelCount, SelItemArr.GetData());

	for (int i = 0; i <= SelItemArr.GetUpperBound(); i++)
	{
		int nCurr = SelItemArr.GetAt(i);

		CListItem& ListItem = m_ListItemArray.ElementAt(nCurr);
		if (ListItem.Value.dataType == ITEMDATA_STRING) {
			// FIXME: is this O(n**2)?
			CString concatenated;
			for (int j = 0; j <= ListItem.Value.stringVal.GetUpperBound(); j++) {
				concatenated += ListItem.Value.stringVal.GetAt(j) + "\r\n";
			}
			concatenated = concatenated.Left(concatenated.GetLength() - 2);
			SetClipboardText(concatenated);

		}
	}
	Invalidate();
}


int CInspectorList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Settings.m_nDivider = INSP_DIVIDER_POSITION * CClientDC(this).GetDeviceCaps(LOGPIXELSY) / 96;
	return 0;
}

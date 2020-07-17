// GMEView.h : interface of the CGMEView class
//
/////////////////////////////////////////////////////////////////////////////


#if !defined(AFX_GMEVIEW_H__BD235B55_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_)
#define AFX_GMEVIEW_H__BD235B55_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GMEStd.h"
#include "GMEOLEData.h"
#include "GuiObject.h"
#include "AspectSyncDlg.h"
#include "ScrollZoomView.h"
#include "GMEViewOverlay.h"
#include <list>
#include "CrashRpt.h"

class CViewDriver;
class CChildFrame;
class CGMEDoc;


static int __stdcall exceptionFilter(unsigned int code, struct _EXCEPTION_POINTERS* ep)
{
	crExceptionFilter(code, ep);
	return EXCEPTION_CONTINUE_SEARCH;
}

class CGMEView :	public CScrollZoomView
{
	friend class CGMEOLEModel;
	friend class CGMEOLEIt;
	friend class GMEViewOverlay;
	std::unique_ptr<GMEViewOverlay> m_overlay;
	void HighlightConnection(CGuiConnection* connection);
public:
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE) {
		m_overlay = nullptr;
		return __super::OnScrollBy(sizeScroll, bDoScroll);
	}
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) {
		__try {
			return __super::OnWndMsg(message, wParam, lParam, pResult);
		}
		__except (exceptionFilter(GetExceptionCode(), GetExceptionInformation()))
		{
		}
	}


protected: // create from serialization only
	CGMEView();
	DECLARE_DYNCREATE(CGMEView)

private:
	bool			m_preview;
	CGuiMetaAspect*	m_prevcurrasp;
	int				m_scalePrnPages;
	int				m_currPrnNumAsp;
	int				m_fullPrnAspNum;
	POSITION		m_prnpos;
	unsigned int	m_lastPrnPage;
	unsigned long	m_findNextAlreadyAchieved;
	bool			m_supressConnectionCheckAlert;

	void			SetZoomPoint(int curzoom, CPoint point);
	bool			GetNamePositionVal(CComPtr<IMgaFCO>& ccpMgaFCO, int *valRet);
	void			SetNamePositionVal(CComPtr<IMgaFCO>& ccpMgaFCO, int val);
	void			ChangeNamePosition(int w);
	void			UpdateNamePositionMenuItem(CCmdUI* pCmdUI, int this_val);
	bool			AskUserAndDetachIfNeeded(CComPtr<IMgaFCO>& mgafco);
	CGuiObject*		HelpMeFindNextObject(bool p_secondFind);
	bool IsLegalConnectionEnd(CGuiObject *connEnd, CGuiPort *port = NULL);

public:
	bool m_isActive;
	void DoPannWinRefresh();

public:
	void PrepareAspectPrn(CPrintInfo* pInfo);
	bool ChangePrnAspect(CString aspect);
	void SaveCurrAsp() { m_prevcurrasp = currentAspect; }
	CGuiMetaAspect* GetSavedAsp() { CGuiMetaAspect *ret = m_prevcurrasp; m_prevcurrasp = NULL; return ret; }
	bool IsPreview() { return m_preview; }
	void SetPreview(bool set) { m_preview = set; }

// Attributes
public:
	static int				inTransaction;
	static bool				inRWTransaction;
	static bool				inEventHandler;
	static int				instanceCount;
	static int				offScreenCreated;
	static CDC*				offScreen;
	static CBitmap*			ofsbmp;
	static HCURSOR			autoconnectCursor;
	static HCURSOR			autoconnect2Cursor;
	static HCURSOR			disconnectCursor;
	static HCURSOR			disconnect2Cursor;
	static HCURSOR			setCursor;
	static HCURSOR			set2Cursor;
	static HCURSOR			zoomCursor;
	static HCURSOR			visualCursor;
	static HCURSOR			editCursor;
	static bool				derivedDrop;
	static bool				instanceDrop;
	static bool				showConnectedPortsOnly;

	// === Start of decorator operation specific variables ===
private:
	CRect					originalRect;
	CGuiObject*				objectInDecoratorOperation;
	CGuiAnnotator*			annotatorInDecoratorOperation;
	bool					isCursorChangedByDecorator;
	bool					inElementDecoratorOperation;
	bool					inOpenedDecoratorTransaction;
	bool					isContextInitiatedOperation;
	bool					shouldCommitOperation;
	bool					decoratorOrAnnotator;
	bool m_dropRightClick;
public:
	bool m_bEnablePannWindowRefresh;
	struct ContextClickState {
		UINT		nFlags;
		CPoint		lpoint;
		CPoint		dpoint;
	};
	ContextClickState		ctxClkSt;
	CGuiObject*				selectedObjectOfContext;
	CGuiAnnotator*			selectedAnnotationOfContext;
	CGuiConnection*			selectedConnection;
	bool					isConnectionJustSelected;
	bool					isLeftMouseButtonDown;
	bool					doNotDeselectAfterInPlaceEdit;	// disable deselection code, if in-place edit took place (up was handled)
	// for connection customize operation
	bool					isInConnectionCustomizeOperation;
	PathCustomizationType	customizeConnectionType;
	int						customizeConnectionEdgeIndex;
	ConnectionPartMoveType	customizeConnectionPartMoveMethod;
	bool					customizeHorizontalOrVerticalEdge;
	CPoint					customizeConnectionEdgeStartPoint;
	CPoint					customizeConnectionEdgeEndPoint;
	CPoint					customizeConnectionEdgeThirdPoint;
	long						customizeConnectionEdgeXMinLimit;
	long						customizeConnectionEdgeXMaxLimit;
	long						customizeConnectionEdgeYMinLimit;
	long						customizeConnectionEdgeYMaxLimit;
	CPoint					customizeConnectionOrigCursor;
	CPoint					customizeConnectionCurrCursor;
	bool					isCursorChangedByEdgeCustomize;
	HCURSOR					customizeConnectionCursorBackup;
	// for connection context menu
	CGuiConnection*			selectedContextConnection;
	PathCustomizationType	contextConnectionCustomizationType;
	int						contextConnectionEdgeIndex;
	int						contextConnectionEdge2Index;
	ConnectionPartMoveType	contextConnectionPartMoveMethod;
	// === End of decorator operation specific variables ===
	COleDropTarget			dropTarget;
	bool					inDrag;
	CGMEDataDescriptor		dragDesc;
	DROPEFFECT				prevDropEffect;
	CRect					dragRect;
	CPoint					dragPoint;
	CPoint					dragOffset;
	CPoint					contextMenuLocation;
	std::set<CString> uncreatableDecorators;

	CComPtr<IMgaTerritory>	terry;
	CComPtr<IMgaModel>		currentModel;
	CComBSTR				currentModId;
	CComPtr<IMgaFCO>		baseType;			// currentModel's baseType (or type in case of an instance)
	CComPtr<IMgaModel>		parent;
	CComPtr<CViewDriver>	driver;
	CGuiMetaModel*			guiMeta;
	CGuiMetaAspect*			currentAspect;
	CGuiFco*				contextSelection;
	CGuiAnnotator*			contextAnnotation;
	CGuiConnection *last_Connection;
	CGuiSet*				currentSet;
	CGuiPort*				contextPort;
	CString					currentSetID;

	bool					tmpConnectMode;
	CGuiObject*				connSrc;
	CGuiPort*				connSrcPort;
	int						connSrcHotSide;
	CGuiObject*				connTmp;
	CGuiPort*				connTmpPort;
	int						connTmpHotSide;

	CGuiObject*				lastObject;
	CGuiPort*				lastPort;
	CGuiObject*				dragSource;

	bool					isType;
	bool					isSubType;
	bool					isLibObject;
	CString					name;
	CString					kindName;
	CString					kindDisplayedName;
	CString					path;

//	int						zoomIdx;
	int						m_zoomVal;

	bool					drawGrid;
	bool					alive;

	unsigned long			animRefCnt;
	UINT_PTR				timerID;


	CAutoRouter				router;
	CGuiFcoList				children;
	CGuiConnectionList		connections;
	CGuiAnnotatorList		annotators;
	CGuiObjectList			selected;
	CGuiAnnotatorList		selectedAnnotations;
	bool					validGuiObjects;
	CStringList				newObjectIDs;			// after paste(drop) keep track of new objects for selection after reset

	bool					needsReset;
	bool					needsConnConversion;
	bool					initDone;
	bool					isModelAutoRouted;
	COLORREF				bgColor;
	CChildFrame*			frame;

	CPendingRequestList		pendingRequests;
	bool					executingPendingRequests;
	CDC*					onScreen;

	static bool							m_bUseStretchBlt;		// Supposed workaround for Vista black view problem: use StretchBlt instead of BitBlt
	static Gdiplus::SmoothingMode		m_eEdgeAntiAlias;		// Edge smoothing mode
	static Gdiplus::TextRenderingHint	m_eFontAntiAlias;		// Text renndering hint mode

	CGMEDoc*				GetDocument();

// Operations
public:
	CComPtr<IMgaModel>&		GetCurrentModel() { return currentModel; }
	CGuiFco*				CreateGuiObject(CComPtr<IMgaFCO>& fco, CGuiFcoList* objList = NULL, CGuiConnectionList* connList = NULL);
	void					CreateGuiObjects(CComPtr<IMgaFCOs>& fcos, CGuiFcoList& objList, CGuiConnectionList& connList);
	bool					CreateGuiObjects();
	void					CreateAnnotators(CComPtr<IMgaRegNodes>& regNodes, CGuiAnnotatorList& annList);
	void					CreateAnnotators();
	bool					SendOpenModelEvent();
	bool					SendCloseModelEvent();
	bool					SendSelecEvent4Object(CGuiObject* selection);
	bool					SendUnselEvent4Object(CGuiObject* selection);
	bool					SendSelecEvent4List(CGuiObjectList* pSelectedList);
	bool					SendUnselEvent4List(CGuiObjectList* pSelectedList);
	bool					SendMouseOver4Object(CGuiObject* object);
	bool					SendNow(bool onlyDecoratorNotification = false);
	void					AddAnnotationToSelectionHead(CGuiAnnotator* ann);
	void					AddAnnotationToSelectionTail(CGuiAnnotator* ann);
	void					AddAnnotationToSelection(CGuiAnnotator* ann, bool headOrTail);
	void					RemoveAllAnnotationFromSelection(void);
	void					RemoveAnnotationFromSelectionHead(void);
	void					RemoveAnnotationFromSelection(POSITION annPos);
	void					ClearConnectionSelection(void);
	void					UpdateConnectionSelection(int aspect);
	bool					FollowLine(CGuiConnection* guiConn, bool reverse, bool tryPort);
	bool					FollowLine(CGuiObject* guiObj, bool reverse, bool tryPort);
	bool					FollowLine(CGuiPort* guiPort, bool reverse, bool tryPort);
	bool jumpToSelectedEnd( CGuiConnectionList& p_collOfConns, bool p_reverse, bool p_tryPort);
	std::list<CGuiObject*>	m_lstSelect;
	std::list<CGuiObject*>	m_lstUnselect;
	void					Reset(bool doInvalidate = false);
	void					ResetPartBrowser();
	void					ResetParent(bool doInvalidate = false);
	void					InitSets();
	void					CreateOffScreen(CDC* dc);
	void					DeleteOffScreen();
	void					Invalidate(bool thorough = false);
	void					SetProperties();
	void					SetTypeNameProperty();
	void					SetNameProperty();
	void					SetTitles(void);
	void					GetNameProperty(CString& txt);
	void					SetKindNameProperty();
	int						GetAspectProperty();
	void					SetAspectProperty(int ind);
	void					SetTypeProperty(bool type);
	void					ModeChange();
	void					IncrementalAutoRoute();
	void					AutoRoute();
	void					DrawConnections(HDC pDC, Gdiplus::Graphics* gdip);
	void					DrawTracker(CDC* pDC, const CRect& trackerRect, CRectTracker::StyleFlags styleFlags);
	void					DrawConnectionCustomizationTracker(CDC* pDC, Gdiplus::Graphics* gdip);
	void					InsertCustomEdge(CGuiConnection* selectedConn, PathCustomizationType custType,
											 int newPosX, int newPosY, int edgeIndex, bool horizontalOrVerticalEdge);
	void					UpdateCustomEdges(CGuiConnection* selectedConn, PathCustomizationType custType,
											  int newPosX, int newPosY, int edgeIndex, bool horizontalOrVerticalEdge);
	void					DeleteCustomEdges(CGuiConnection* selectedConn, PathCustomizationType custType,
											  int edgeIndex, bool horizontalOrVerticalEdge = false);
	void					ConvertPathToCustom(CComPtr<IUnknown>& pMgaObject);
	void					PrintHeader(CDC* pDC, CPrintInfo* pInfo);
	void					PrintHeaderRect(CDC* pDC, CRect& rectDraw);
	void					PrintMultiLineText(Gdiplus::Graphics* gdip, CDC* pDC, CString txt, int x, int& y, int ry, int xwidth);

	void					CoordinateTransfer(CPoint& point) const;
	CGuiObject*				FindFirstObject();
	CGuiObject*				FindNextObject();
	CGuiObject*				FindObject(CPoint& pt, bool lookNearToo = false, bool lookForLabel = false);
	CGuiAnnotator*			FindAnnotation(CPoint& pt);
	bool					FindObjects(CRect& rect, CGuiObjectList& objectList);
	bool					FindAnnotations(CRect& rect, CGuiAnnotatorList& annotatorList);
	bool					DeleteObjects(CGuiObjectList& objectList);
	void					DeleteAnnotations(CGuiAnnotatorList& annotatorList);
	bool					DeleteConnection(CGuiConnection* guiConn,bool checkAspect = true);
	bool					DisconnectAll(CGuiObject* end, CGuiPort* endPort, bool onlyVisible = true);
	void					FindConnections(CGuiObject* end, CGuiPort* endPort, CGuiConnectionList& guiConn);
	void					FindConnections(CGuiObject* end1, CGuiPort* end1Port, CGuiObject* end2, CGuiPort* end2Port,
											CGuiConnectionList& guiConn);
	void					GrayOutNonInternalConnections();
	void					FillModelGrid();
	void					SetObjectLocation(CComPtr<IMgaFCO>& child, CComPtr<IMgaMetaRole>& mmRole, CPoint pt);
	void					SetScroll();
	void					SetCenterObject(CComPtr<IMgaFCO> obj);
	void					InsertNewPart(const CString& roleName, const CPoint& pt);
	void					GetRefereeChain(IMgaFCOs* refChain, IMgaFCO* fco);
	bool					Connect(CGuiObject* src, CGuiPort* srcPort, int srcHotSide, CGuiObject* dst, CGuiPort* dstPort,
									int dstHotSide, bool nosticky);
	void					ResolveConnections();
	bool					IsConnectionConversionNeeded(void);
	void					ConvertNeededConnections(void);
	void					BeginTransaction(transactiontype_enum mode = TRANSACTION_GENERAL);
	void					CommitTransaction(); // throws hresult_exception
	void					__CommitTransaction(); // throws _com_error
	void					AbortTransaction(HRESULT hr);
	void					PasteAnnotations(CComPtr<IMgaModel>& targetModel, CComPtr<IMgaRegNodes>& regNodes,
											 CComPtr<IMgaRegNodes>& newRegNodes, bool isMove);
	void					CopyRegTree(CComPtr<IMgaRegNode>& regNode, CComPtr<IMgaRegNode>& newNode);
	bool					DoPasteItem(COleDataObject* pDataObject, bool drag = false, bool move = false, bool reference = false,
										bool derive = false, bool instance = false, bool closure = false, bool merge = false,
										CGuiObject* ref = 0, CPoint pt = CPoint(0, 0));
	bool					DoPasteNative(COleDataObject* pDataObject, bool drag, bool move, bool reference, bool derive,
										  bool instance, CGuiObject* ref, CPoint pt);
	void					MakeSureGUIDIsUniqueForSmartCopy(CComPtr<IMgaFCO>& fco);
	void					ChangeAspect(CString aspName, bool p_eraseStack = true);
	void					ChangeAspect(int ind);
	CString&				GetAspectName(int ind);
	CString&				GetCurrentAspectName(void);
	void					SetName();
	void					RetrievePath();
	void					SetBgColor();
	void					ShowHelp(CComPtr<IMgaFCO> fco);
	void					ShowModel(CComPtr<IMgaModel> model, const CString& aspect = CString(""));
	void					GetModelInContext(CComPtr<IMgaModel>& model);
	void					FindDerivedFrom(CComPtr<IMgaModel> model, CComPtr<IMgaModel>& type);
	void					ChangeAttrPrefObjs(CGuiObjectList& objlist);
	void					ChangeAttrPrefFco(CGuiFco* guiFco);
	void					ChangeAttrPrefFco();
	void					ShowProperties(CGuiFco* guiFco);
	void					ShowProperties();
	void					ShowAttributes(CGuiFco* guiFco);
	void					ShowAttributes();
	void					ShowPreferences(CGuiFco* guiFco);
	void					ShowPreferences();
	void					FillAttributeDescs(CComPtr<IMgaFCO> fco, CGuiMetaAttributeList& guiMetaAttrs, CGuiDepCtrlDescList& list);
	bool					CheckBeforeDeleteObjects(CGuiObjectList& objectList, CString& txt);
	void					ZoomOut(CPoint point);
	void					ZoomIn(CPoint point);
	void					ZoomToPoint(int scale, int newScale, CPoint point);
	void					ZoomRect(CRect srect);
	void					ZoomToFCOs(CRect rect);
	void					ShowRegistryBrowser(CComPtr<IMgaFCO> fco);
	bool					ShowAnnotationBrowser(CComPtr<IMgaFCO> fco, CComPtr<IMgaRegNode> focus);
	void					SyncAspects(CGuiMetaAspect* srcAspect, CGuiMetaAspectList& dstAspects, CGuiObjectList& movingObjects,
										CGuiObjectList& sedentaryObjects, bool priorityForSrcVisible, bool priorityForSelected);
	void					SyncOnGrid(CGuiObject *obj, int aspectIndexFrom, int aspectIndexTo);
	void					ClearConnSpecs();
	void					RunComponent(CString compname);
	void					SetEditCursor(void);
	void					StartDecoratorOperation(void);
	void					EndDecoratorOperation(void);
	void					CancelDecoratorOperation(bool notify = true);

	bool					IsCursorChangedByDecorator(void) const;
	void					SetIsCursorChangedByDecorator(bool isCurChanged);
	CRect					GetOriginalRect(void) const;
	void					SetOriginalRect(const CRect& rect);
	void					SetOriginalRectEmpty(void);
	bool					IsInElementDecoratorOperation(void) const;
	void					SetInElementDecoratorOperation(bool isIn);
	bool					IsInOpenedDecoratorTransaction(void) const;
	void					SetInOpenedDecoratorTransaction(bool inOpenedTr);
	bool					IsContextInitiatedOperation(void) const;
	void					SetIsContextInitiatedOperation(bool isCtxInit);
	bool					ShouldCommitOperation(void) const;
	void					SetShouldCommitOperation(bool shouldCommitOp);
	bool					IsDecoratorOrAnnotator(void) const;
	void					SetDecoratorOrAnnotator(bool decorOrAnnot);
	CGuiObject*				GetObjectInDecoratorOperation(void) const;
	void					SetObjectInDecoratorOperation(CGuiObject* obj);
	CGuiAnnotator*			GetAnnotatorInDecoratorOperation(void) const;
	void					SetAnnotatorInDecoratorOperation(CGuiAnnotator* ann);

	bool					ShouldSupressConnectionCheckAlert(void) const { return m_supressConnectionCheckAlert; };
	void					ClearSupressConnectionCheckAlert(void) { m_supressConnectionCheckAlert = false; };
	void					SupressConnectionCheckAlert(void) { m_supressConnectionCheckAlert = true; };
	HRESULT					DumpModelGeometryXML(LPCTSTR filePath);
	void					SetConnectionCustomizeCursor(const CPoint& point);
	bool					IsInstance(void) const;
	void					TryToExecutePendingRequests(void);

	virtual INT_PTR			OnToolHitTest(CPoint point, TOOLINFO* pTI) const;

	static bool				IsHugeModel()	{ return false; } // HACK
	static CGMEView*		GetActiveView();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGMEView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual void OnDragLeave();
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	virtual void OnActivateFrame( UINT nState, CFrameWnd* pFrameWnd );
	//}}AFX_VIRTUAL
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);

// Implementation
public:
	virtual ~CGMEView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void SwapAutoRouterPref(const CString& strPref);
	void SetAllAutoRouterPref(bool bSrc, bool bClear);

// Generated message map functions
public:
	void ShowGrid(bool show);
	void AttributepanelPage(long page);
	void ZoomPercent(long percent);
	void CycleAspect() { this->OnCycleAspect(); }
	void CycleAllAspects() { this->OnCycleAllAspects(); }
protected:
	//{{AFX_MSG(CGMEView)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnDropFiles(HDROP p_hDropInfo);
	afx_msg void OnKillfocusNameProp();
	afx_msg void OnSelChangeAspectProp();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT fFlags, short zDelta, CPoint point);
	afx_msg void OnAppCommand(CWnd* pWnd, UINT nCmd, UINT nDevice, UINT nKey);
	afx_msg void OnViewParent();
	afx_msg void OnUpdateViewParent(CCmdUI* pCmdUI);
	afx_msg void OnViewGrid();
	afx_msg void OnUpdateViewGrid(CCmdUI* pCmdUI);
	afx_msg void OnEditNudgedown();
	afx_msg void OnEditNudgeleft();
	afx_msg void OnEditNudgeright();
	afx_msg void OnEditNudgeup();
	afx_msg void OnEditDelete();
	afx_msg void OnUpdateEditDelete(CCmdUI* pCmdUI);
	afx_msg void OnContextProperties();
	afx_msg void OnCntxAttributes();
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCopyClosure();
	afx_msg void OnEditCopySmart();
	afx_msg void OnUpdateEditCopyClosure(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditCopySmart(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnCntxCopy();
	afx_msg void OnCntxCopyClosure();
	afx_msg void OnCntxCopySmart();
	afx_msg void OnCntxCut();
	afx_msg void OnCntxDelete();
	afx_msg void OnSelfcntxCopy();
	afx_msg void OnUpdateSelfcntxCopy(CCmdUI* pCmdUI);
	afx_msg void OnSelfcntxCopyClosure();
	afx_msg void OnSelfcntxCopySmart();
	afx_msg void OnUpdateSelfcntxCopyClosure(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSelfcntxCopySmart(CCmdUI* pCmdUI);
	afx_msg void OnSelfcntxCut();
	afx_msg void OnUpdateSelfcntxCut(CCmdUI* pCmdUI);
	afx_msg void OnSelfcntxDelete();
	afx_msg void OnUpdateSelfcntxDelete(CCmdUI* pCmdUI);
	afx_msg void OnSelfcntxPaste();
	afx_msg void OnUpdateSelfcntxPaste(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCntxAttributes(CCmdUI* pCmdUI);
	afx_msg void OnEditCancel();
	afx_msg void OnUpdateEditCancel(CCmdUI* pCmdUI);
	afx_msg void OnCntxDisconnectall();
	afx_msg void OnUpdateCntxDisconnectall(CCmdUI* pCmdUI);
	afx_msg void OnFileClose();
	afx_msg void OnConncntxProperties();
	afx_msg void OnConncntxDelete();
	afx_msg void OnConncntxReverse();
	afx_msg void OnUpdateConncntxReverse(CCmdUI* pCmdUI);
	afx_msg void OnConnCntxFollow();
	afx_msg void OnConnCntxRevfollow();
	afx_msg void OnCntxClear();
	afx_msg void OnUpdateCntxClear(CCmdUI* pCmdUI);
	afx_msg void OnCntxReset();
	afx_msg void OnUpdateCntxReset(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCntxDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateConncntxDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCntxCut(CCmdUI* pCmdUI);
	afx_msg void OnCntxPreferences();
	afx_msg void OnEditPreferences();
	afx_msg void OnHelpHelp();
	afx_msg void OnCntxHelp();
	afx_msg void OnEditShowtype();
	afx_msg void OnUpdateEditShowtype(CCmdUI* pCmdUI);
	afx_msg void OnEditShowbasetype();
	afx_msg void OnUpdateEditShowbasetype(CCmdUI* pCmdUI);
	afx_msg void OnCntxShowtype();
	afx_msg void OnUpdateCntxShowtype(CCmdUI* pCmdUI);
	afx_msg void OnCntxShowbasetype();
	afx_msg void OnUpdateCntxShowbasetype(CCmdUI* pCmdUI);
	afx_msg void OnFileInterpret();
	afx_msg void OnFileCheck();
	afx_msg void OnFileCheckSelected();
	afx_msg void OnCntxInterpret();
	afx_msg void OnCntxCheck();
	afx_msg void OnCntxLocate();
	afx_msg void OnUpdateCntxInterpret(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCntxCheck(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCntxLocate(CCmdUI* pCmdUI);
	afx_msg void OnCntxRegistry();
	afx_msg void OnEditRegistry();
	afx_msg void OnEditSync();
	afx_msg void OnUpdateEditSync(CCmdUI* pCmdUI);
	afx_msg void OnEditSelectall();
	afx_msg void OnUpdateFileCheck(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileCheckSelected(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSettings(CCmdUI* pCmdUI);
	afx_msg void OnEditPastespecialAsinstance();
	afx_msg void OnEditPastespecialAsreference();
	afx_msg void OnEditPastespecialAssubtype();
	afx_msg void OnEditPastespecialAsclosure();
	afx_msg void OnEditPastespecialAdditive();
	afx_msg void OnEditPastespecialMerge();
	afx_msg void OnUpdateEditPastespecialAsinstance(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPastespecialAsreference(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPastespecialAssubtype(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPastespecialAsclosure(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPastespecialAdditive(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditPastespecialMerge(CCmdUI* pCmdUI);
	afx_msg void OnCntxPastespecialAsinstance();
	afx_msg void OnUpdateCntxPastespecialAsinstance(CCmdUI* pCmdUI);
	afx_msg void OnCntxPastespecialAsreference();
	afx_msg void OnUpdateCntxPastespecialAsreference(CCmdUI* pCmdUI);
	afx_msg void OnCntxPastespecialAssubtype();
	afx_msg void OnUpdateCntxPastespecialAssubtype(CCmdUI* pCmdUI);
	afx_msg void OnCntxPastespecialAsclosure();
	afx_msg void OnUpdateCntxPastespecialAsclosure(CCmdUI* pCmdUI);
	afx_msg void OnCntxPastespecialAdditive();
	afx_msg void OnUpdateCntxPastespecialAdditive(CCmdUI* pCmdUI);
	afx_msg void OnCntxPastespecialMerge();
	afx_msg void OnUpdateCntxPastespecialMerge(CCmdUI* pCmdUI);
	afx_msg void OnCntxRedirectionpaste();
	afx_msg void OnUpdateCntxRedirectionpaste(CCmdUI* pCmdUI);
	afx_msg void OnCntxConnect();
	afx_msg void OnUpdateCntxConnect(CCmdUI* pCmdUI);
	afx_msg void OnResetSticky();
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnUpdateCntxInsertannotation(CCmdUI* pCmdUI);
	afx_msg void OnCntxInsertannotation();
	afx_msg void OnCntxAnnotations();
	afx_msg void OnEditAnnotations();
	afx_msg void OnUpdateCntxAutoRouters( CCmdUI* pCmdUI );
	afx_msg void OnCntxSrcarSouth();
	afx_msg void OnCntxSrcarNorth();
	afx_msg void OnCntxSrcarEast();
	afx_msg void OnCntxSrcarWest();
	afx_msg void OnCntxDstarEast();
	afx_msg void OnCntxDstarNorth();
	afx_msg void OnCntxDstarSouth();
	afx_msg void OnCntxDstarWest();
	afx_msg void OnCntxSrcarClear();
	afx_msg void OnCntxDstarClear();
	afx_msg void OnCntxSrcarSet();
	afx_msg void OnCntxDstarSet();
	afx_msg void OnPortCntxFollowConnection();
	afx_msg void OnPortCntxRevfollowConnection();
	afx_msg void OnCntxPortDelete();
	afx_msg void OnCntxFollowConnection();
	afx_msg void OnCntxRevfollowConnection();
	afx_msg void OnCntxPortShowInParent();
	afx_msg void OnCntxPortLocateInBrw();
	afx_msg void OnJumpAlongConnection();
	afx_msg void OnBackAlongConnection();
	afx_msg void OnJumpToFirstObject();
	afx_msg void OnJumpToNextObject();
	afx_msg void OnTryToSnapHorzVertPath();
	afx_msg void OnDeleteConnEdgeCustomData();
	afx_msg void OnDeleteConnPointCustomData();
	afx_msg void OnDeleteConnRouteCustomDataThisAspect();
	afx_msg void OnDeleteConnRouteCustomDataAllAspects();
	afx_msg void OnShowContextMenu();
	afx_msg void OnPrintMetafile();
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg LRESULT OnZoom(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPannScroll(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPannRefresh(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDecoratorViewRefreshRequest(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnExecutePendingRequests(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCntxNamePositionSouth();
	afx_msg void OnCntxNamePositionNorth();
	afx_msg void OnCntxNamePositionEast();
	afx_msg void OnCntxNamePositionWest();
	afx_msg void OnUpdateCntxNamePositionSouth( CCmdUI* pCmdUI );
	afx_msg void OnUpdateCntxNamePositionNorth( CCmdUI* pCmdUI );
	afx_msg void OnUpdateCntxNamePositionEast( CCmdUI* pCmdUI );
	afx_msg void OnUpdateCntxNamePositionWest( CCmdUI* pCmdUI );
	afx_msg void OnUpdatePortCntxFollowConnection( CCmdUI* pCmdUI );
	afx_msg void OnUpdatePortCntxRevfollowConnection( CCmdUI* pCmdUI );
	afx_msg void OnUpdateCntxFollowConnection( CCmdUI* pCmdUI );
	afx_msg void OnUpdateCntxRevfollowConnection( CCmdUI* pCmdUI );
	afx_msg void OnUpdateJumpAlongConnection( CCmdUI* pCmdUI );
	afx_msg void OnUpdateBackAlongConnection( CCmdUI* pCmdUI );
	afx_msg void OnUpdateTryToSnapHorzVertPath( CCmdUI* pCmdUI );
	afx_msg void OnUpdateDeleteConnEdgeCustomData( CCmdUI* pCmdUI );
	afx_msg void OnUpdateDeleteConnPointCustomData( CCmdUI* pCmdUI );
	afx_msg void OnUpdateDeleteConnRouteCustomDataThisAspect( CCmdUI* pCmdUI );
	afx_msg void OnUpdateDeleteConnRouteCustomDataAllAspects( CCmdUI* pCmdUI );
#if defined(ADDCRASHTESTMENU)
	afx_msg void OnCrashTestIllegalWrite();
	afx_msg void OnCrashTestIllegalRead();
	afx_msg void OnCrashTestIllegalReadInCRuntime();
	afx_msg void OnCrashTestIllegalCodeRead();
	afx_msg void OnCrashTestDivideByZero();
	afx_msg void OnCrashTestAbort();
	afx_msg void OnCrashTestTerminate();
	afx_msg void OnUpdateCrashTestMenu(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCrashTestIllegalWrite(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCrashTestIllegalRead(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCrashTestIllegalReadInCRuntime(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCrashTestIllegalCodeRead(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCrashTestDivideByZero(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCrashTestAbort(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCrashTestTerminate(CCmdUI* pCmdUI);
#endif
	afx_msg void OnShowSelectedModel();
	afx_msg void OnFocusBrowser();
	afx_msg void OnRename();
	afx_msg void OnFocusInspector();
	afx_msg void OnCycleAspect();
	afx_msg void OnCycleAspectBackwards();
	afx_msg void OnCycleAllAspects();
	afx_msg void OnHistoryBack();
	afx_msg void OnHistoryForw();
	afx_msg void OnKeyConnect();
	afx_msg void OnKeyCycleObjInspectorFrwd();
	afx_msg void OnKeyCycleObjInspectorBkwd();
	afx_msg void OnViewMultiUserShowObjectOwner();
	afx_msg void OnUpdateViewMultiUserShowObjectOwner( CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnViewShowconnectedportsonly();
	afx_msg void OnUpdateViewShowconnectedportsonly(CCmdUI *pCmdUI);
};

class ATL_NO_VTABLE CViewDriver :
		public CComObjectRootEx<CComSingleThreadModel>,
		public IDispatchImpl<IMgaEventSink, &__uuidof(IMgaEventSink), &__uuidof(__MGALib)>
{
public:
	CViewDriver() { }
	virtual ~CViewDriver() {}
public:
	CGMEView *view;
	static bool attrNeedsRefresh;

BEGIN_COM_MAP(CViewDriver)
	COM_INTERFACE_ENTRY(IMgaEventSink)
//	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ------- COM methods
public:
	STDMETHODIMP GlobalEvent(globalevent_enum event);
	STDMETHODIMP ObjectEvent(IMgaObject * obj, unsigned long eventmask,VARIANT v);



};


#ifndef _DEBUG  // debug version in GMEView.cpp
inline CGMEDoc* CGMEView::GetDocument()
   { return (CGMEDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GMEVIEW_H__BD235B55_BD3F_11D3_91E2_00104B98EAD9__INCLUDED_)

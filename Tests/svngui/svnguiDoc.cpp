
// svnguiDoc.cpp : implementation of the CsvnguiDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "svngui.h"
#endif

#include "svnguiDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CsvnguiDoc

IMPLEMENT_DYNCREATE(CsvnguiDoc, CDocument)

BEGIN_MESSAGE_MAP(CsvnguiDoc, CDocument)
END_MESSAGE_MAP()


// CsvnguiDoc construction/destruction

CsvnguiDoc::CsvnguiDoc() : svnFile(NULL)
{
	// TODO: add one-time construction code here

}

CsvnguiDoc::~CsvnguiDoc()
{
}

BOOL CsvnguiDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	if (!m_viewList.IsEmpty())
	{
		reinterpret_cast<CEditView*>(m_viewList.GetHead())->SetWindowText(NULL);
	}

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CsvnguiDoc serialization

void CsvnguiDoc::Serialize(CArchive& ar)
{
	// CEditView contains an edit control which handles all serialization
	if (!m_viewList.IsEmpty())
	{
		reinterpret_cast<CEditView*>(m_viewList.GetHead())->SerializeRaw(ar);
	}
#ifdef SHARED_HANDLERS

	if (m_viewList.IsEmpty() && ar.IsLoading())
	{
		CFile* pFile = ar.GetFile();
		pFile->Seek(0, FILE_BEGIN);
		ULONGLONG nFileSizeBytes = pFile->GetLength();
		ULONGLONG nFileSizeChars = nFileSizeBytes/sizeof(TCHAR);
		LPTSTR lpszText = (LPTSTR)malloc(((size_t)nFileSizeChars + 1) * sizeof(TCHAR));
		if (lpszText != NULL)
		{
			ar.Read(lpszText, (UINT)nFileSizeBytes);
			lpszText[nFileSizeChars] = '\0';
			m_strThumbnailContent = lpszText;
			m_strSearchContent = lpszText;
		}
	}
#endif
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CsvnguiDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(m_strThumbnailContent, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CsvnguiDoc::InitializeSearchContent()
{
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// Use the entire text file content as the search content.
	SetSearchContent(m_strSearchContent);
}

void CsvnguiDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CsvnguiDoc diagnostics

#ifdef _DEBUG
void CsvnguiDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CsvnguiDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CsvnguiDoc commands


BOOL CsvnguiDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	try {
		if (svnFile) {
			theApp.svn.forgetFile(svnFile);
			svnFile = NULL;
		}
		svnFile = theApp.svn.embraceFile(lpszPathName);
		CString logLine;
		logLine.Format(_T("File: %s, versioned: %s, tracked %s, owned: %s, latest: %s"), lpszPathName, 
			svnFile->isVersioned() ? _T("yes") : _T("no"),
			svnFile->isTracked() ? _T("yes") : _T("no"),
			svnFile->isOwned() ? _T("yes") : _T("no"),
			svnFile->isLatest() ? _T("yes") : _T("no") /*_T("undefined")*/);
		theApp.Log(logLine);
		if (svnFile->isTracked() && !svnFile->isOwned()) {
			if (AfxMessageBox(_T("This document is tracked in the repository.\nDo you want to lock it?"), MB_YESNO) == IDYES) {
				if (!svnFile->takeOwnership()) {
					if (AfxMessageBox(_T("Failed to lock.\nDo you want to update?"), MB_YESNO) == IDYES) {
						if (svnFile->update()) {
							if (!svnFile->takeOwnership()) {
								AfxMessageBox(_T("Failed to lock"), MB_ICONSTOP);
							}
						}
						else {
							AfxMessageBox(_T("Failed to update"), MB_ICONSTOP);
						}
					}
				}
			}
		}
		
	}
	catch (const CSVNError& e) {
		theApp.Log(e.msg());
	}
	return TRUE;
}


void CsvnguiDoc::OnCloseDocument()
{
	// TODO: Add your specialized code here and/or call the base class

	

	theApp.svn.forgetFile(svnFile);
	svnFile = NULL;
	CDocument::OnCloseDocument();
}


BOOL CsvnguiDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	// TODO: Add your specialized code here and/or call the base class

	return CDocument::OnSaveDocument(lpszPathName);
}


BOOL CsvnguiDoc::SaveModified()
{
	try {
		if (svnFile && svnFile->isOwned()) {
			svnFile->commit();
		}
	}
	catch (const CSVNError& e) {
		theApp.Log(e.msg());
	}
	// TODO: unlock file if not changed

	return CDocument::SaveModified();
}

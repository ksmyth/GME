// ScriptEdit.cpp: implementation of the CScriptEdit class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScriptEdit.h"
#include "ConsoleCtl.h"
#include "MgaUtil.h"
#include "GME.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CScriptEdit, CEdit)
	//{{AFX_MSG_MAP(CHtmlCtrl)
	ON_WM_KEYUP()
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnEnKillfocus)
	ON_CONTROL_REFLECT(EN_SETFOCUS, OnEnSetfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*static*/ const TCHAR* CScriptEdit::defPrompt = _T(">");
CScriptEdit::CScriptEdit()
{

}

CScriptEdit::~CScriptEdit()
{
	m_host->InitEngine(NULL, NULL);
}

bool CScriptEdit::Init(CConsoleCtrl *cons)
{
	try
	{
		m_console = cons;

		HRESULT hr;
		COMTHROW(hr = m_host.CreateInstance(CLSID_ScriptHost, 0, CLSCTX_ALL));

		_bstr_t engine(L"JScript");
//		_bstr_t engine(L"Python.AXScript.2");
		COMTHROW(m_host->InitEngine(m_console->GetIDispatch(false), engine));
		this->LimitText(1024 * 4);
		this->SetWindowText( defPrompt); // to attract user attention
	}
	catch(hresult_exception &e) 
	{ 
		TCHAR s[200];
		_stprintf_s(s, _T("Scripting Initialization Error: 0x%x"), (unsigned int)e.hr);
		m_console->Message(s, MSG_ERROR);
		_ftprintf(stderr, _T("%s"), s);
		return false;
	}

	return true;
}

void CScriptEdit::showPrev()
{
	OnKeyUp( VK_UP, 0, 0);
}

void CScriptEdit::showNext()
{
	OnKeyUp( VK_DOWN, 0, 0);
}

void CScriptEdit::returnHit()
{
	OnKeyUp( VK_RETURN, 0, 0);
}

void CScriptEdit::OnKeyUp( UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/ )
{
	static int lastup = -1;

	if (nChar == VK_ESCAPE)
	{
		lastup = -1;
		SetSel(0, -1);
		ReplaceSel(_T(""));
	}
	else if (nChar == VK_UP)
	{
		lastup++;
		if (lastup >= (int) m_inputlist.size())
			lastup = (m_inputlist.size()==0)? 0: m_inputlist.size()-1;
		if (!m_inputlist.empty())
		{
			SetSel(0, -1);
			ReplaceSel(m_inputlist[m_inputlist.size()-1-lastup]);
		}
	}
	else if (nChar == VK_DOWN)
	{
		lastup --;
		if (lastup < 0)
			lastup = 0;
		if (!m_inputlist.empty())
		{
			SetSel(0, -1);
			ReplaceSel(m_inputlist[m_inputlist.size()-1-lastup]);
		}
	}
	else if (nChar == VK_RETURN)
	{
		lastup = -1;
		CString inp = CEditGetLine(*this, 0);

		_bstr_t binp = inp;
		// echo
		m_console->Message((LPCTSTR)inp, MSG_NORMAL);
		// check if inp already exists
		int k;
		for (k=0; k<(int) m_inputlist.size(); k++)
		{
			if (m_inputlist[k] == inp)
			{
				// shift m_inputlist[k..size - 1] by 1 to the left, to have preserve 'recentness'
				int l;
				for( l  = k; l < (int) m_inputlist.size() - 1; ++l)
					m_inputlist[l] = m_inputlist[l + 1];
				m_inputlist[ l] = inp; // last element
				break;
			}
		}
		// add if inp is new
		if (k == m_inputlist.size())
		{
			m_inputlist.push_back(inp);
			if (m_inputlist.size() > 15) // do not store more than 15 items
			{
				Strings::iterator it = m_inputlist.begin();
				m_inputlist.erase(it);
			}
		}

		
		static const TCHAR *cls_c = _T("!cls");
		static const TCHAR *run_c = _T("!run");
		static const TCHAR *lod_c = _T("!load ");
		static const TCHAR *lor_c = _T("!loadr "); // load & run
		static const TCHAR *rel_c = _T("!rel"); // reload
		static const TCHAR *rlr_c = _T("!rlr"); // reload & run
		bool             handled = false;

		if(      inp == run_c) handled = true, m_console->RunScript();
		else if(wcsncmp(inp, lod_c, wcslen(lod_c)) == 0) handled = true, m_console->LoadScript( _bstr_t(inp.Mid( _tcslen( lod_c))));
		else if(wcsncmp(inp, lor_c, wcslen(lor_c)) == 0)
		{
			handled = true;
			m_console->LoadScript( _bstr_t(inp.Mid( _tcslen( lor_c))));
			m_console->RunScript();
		}
		else if( inp == rel_c) handled = true, m_console->LoadScript( _bstr_t(m_loadedFileName));
		else if( inp == rlr_c) handled = true, m_console->LoadScript( _bstr_t(m_loadedFileName)), m_console->RunScript();
		else if( inp == cls_c) handled = true, m_console->Clear();
		
		if( handled) {
			SetSel(0, -1);
			Clear();
			return;
		}
		else
			ExecuteScript( inp);
	}
}

void CScriptEdit::SetGMEApp(IDispatch *disp)
{
	try
	{
		COMTHROW(m_host->SetGMEApp(disp));
	}
	catch(hresult_exception &e) 
	{ 
		TCHAR s[1000];
		_stprintf_s(s, _T("Scripting Error: 0x%x"), (unsigned int)e.hr);
		m_console->Message((LPCTSTR)s, MSG_ERROR);
		_ftprintf(stderr, _T("%s"), s);
	}
	catch(_com_error &e) 
	{ 
		TCHAR s[1000];
		_stprintf_s(s, _T("Scripting Error: 0x%x"), (unsigned int)e.Error());
		m_console->Message(s, MSG_ERROR);
		_ftprintf(stderr, _T("%s"), s);
	}
}

void CScriptEdit::SetGMEProj(IDispatch *disp)
{
	try
	{
		COMTHROW(m_host->SetGMEProj(disp));
	}
	catch(hresult_exception &e) 
	{ 
		TCHAR s[1000];
		_stprintf_s(s, _T("Scripting Error: 0x%x"), (unsigned int)e.hr);
		m_console->Message(s, MSG_ERROR);
		_ftprintf(stderr, _T("%s"), s);
	}
	catch(_com_error &e) 
	{ 
		TCHAR s[1000];
		_stprintf_s(s, _T("Scripting Error: 0x%x"), (unsigned int)e.Error());
		m_console->Message(s, MSG_ERROR);
		_ftprintf(stderr, _T("%s"), s);
	}
}

void CScriptEdit::OnEnKillfocus()
{
	CString buff;
	this->GetWindowText( buff);
	if( buff.IsEmpty()) // if no user written command while leaving field
	{
		this->SetWindowText( defPrompt); // to attract user attention
	}
}

void CScriptEdit::OnEnSetfocus()
{
	CString buff;
	this->GetWindowText( buff);
	if( buff == defPrompt) // if found the default prompt
	{
		this->SetWindowText( _T("")); // when it has start typing remove '>'
	}
}

void CScriptEdit::ExecuteScript( CString& p_str)
{
	try {
		// load engine info from registry 
		CComPtr<IMgaRegistrar> registrar;
		COMTHROW(registrar.CoCreateInstance(L"Mga.MgaRegistrar"));
		ASSERT( registrar != NULL );
		_bstr_t eng;
		COMTHROW(registrar->get_ScriptEngine(REGACCESS_USER, eng.GetAddress()));
		_bstr_t engine(L"JScript");
		if (eng.length() != 0)
			engine = eng;

		_bstr_t input = p_str;
		COMTHROW(m_host->InitEngine(m_console->GetIDispatch(false), engine));
		COMTHROW(m_host->ProcessString(input));
	}
	catch(hresult_exception& e) {
		TCHAR s[1000];
		_stprintf_s(s, _T("Scripting Error: 0x%x"), (unsigned int)e.hr);
		m_console->Message(s, MSG_ERROR);
	}
	catch(_com_error& e) {
		TCHAR s[1000];
		COMTHROW(e.Error());
		_stprintf_s(s, _T("Execute Script Error: 0x%x"), (unsigned int)e.Error());
		m_console->Message(s, MSG_ERROR);
	}
	catch(...) {
		m_console->Message( _T("Exception handled."), MSG_ERROR);
	}

	SetSel(0, -1);
	Clear();
}


// ScriptHost.cpp : Implementation of CScriptHost
#include "stdafx.h"
#include "Console.h"
#include "ScriptHost.h"
#include "ConsoleCtl.h"

/////////////////////////////////////////////////////////////////////////////
// CScriptHost


STDMETHODIMP CScriptHost::InitEngine(IDispatch* console, BSTR engineProgid)
{
	// register app and project 
	try
	{
		if (console == NULL) {
			m_iscript = NULL;
			m_iscriptParse = NULL;
			m_console = NULL;
			return S_OK;
		}
		m_console = NULL;
		m_console = console;
		_bstr_t newprogid(engineProgid);
		if (m_enginePID == newprogid)
			return S_OK;
		if (m_iscript != NULL)
		{
			COMTHROW(m_iscript->Close());
			m_iscript.Release();
		}
		// FIXME: throw a better error message
		COMTHROW(m_iscript.CreateInstance((BSTR)newprogid, NULL, CLSCTX_ALL));
		m_enginePID = newprogid;
		m_iscriptParse = m_iscript;
		IActiveScriptSitePtr scs(this);
		COMTHROW(m_iscript->SetScriptSite(scs));
		COMTHROW(m_iscriptParse->InitNew());
		if (m_gmeptr != NULL)
		{
			_bstr_t project(L"project");
			_bstr_t gme(L"gme");
			_bstr_t this_model(L"it");
			COMTHROW(m_iscript->AddNamedItem(gme, SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE));
			if( m_mgaproj != NULL) COMTHROW(m_iscript->AddNamedItem(project, SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE));
			if( m_actMod  != NULL) COMTHROW(m_iscript->AddNamedItem(this_model, SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE));
		}
		COMTHROW(m_iscript->SetScriptState(SCRIPTSTATE_CONNECTED));
	}
	COMCATCH(
		// initialization failed; be sure to retry next time
		m_enginePID.Assign(nullptr);
	)

	return S_OK;
}

STDMETHODIMP CScriptHost::ProcessString(BSTR input)
{
// app = new ActiveXObject("Gme.Application"); app.ConsoleContents = "kaka maka";
//	_bstr_t str1("GMEapp = new ActiveXObject(\"Gme.Application\"); ");
//	_bstr_t str2("GMEapp.ConsoleContents = \"kaka maka\"");
//	_bstr_t str2("GMEapp.ConsoleClear();");
//	_bstr_t str2("GMEapp.CloseProject();");
//	_bstr_t strx("app = new ActiveXObject(\"GME.Application\"); app.CreateProject(\"MetaGME\", \"MGA=C:\\OCL_TEST\\kakamaka.mga\");");
//	_bstr_t str = str1 + str2;

	try
	{
		COMTHROW(m_iscriptParse->ParseScriptText(input, NULL, NULL, NULL, 0, 0, 
			SCRIPTTEXT_ISPERSISTENT, NULL, NULL));
	}
	catch(hresult_exception &e) 
	{ 
		Message( _bstr_t(_T("Input parsing failed!")), MSG_ERROR);
		return e.hr;
	}
	return S_OK;
}

void CScriptHost::Message(BSTR message, msgtype_enum level)
{
	static BYTE params[] = VTS_BSTR VTS_I2;
	COleDispatchDriver disp(m_console, false);
	disp.InvokeHelper(2, DISPATCH_METHOD, VT_EMPTY, 0, params, message, level);
}

STDMETHODIMP CScriptHost::GetLCID(/*[out]*/ LCID *plcid)
{
	*plcid = LANG_ENGLISH;
	return S_OK;
}

STDMETHODIMP CScriptHost::GetItemInfo(
                /*[in]*/  LPCOLESTR pstrName,
                /*[in]*/  DWORD     dwReturnMask,
                /*[out]*/ IUnknown  **ppiunkItem,
                /*[out]*/ ITypeInfo **ppti)
{
	if (!m_gmeptr)
		return E_FAIL;
	if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
	{
		if (m_gmeptr && wcscmp(pstrName, L"gme") == 0)
		{
			CComPtr<IUnknown> punk(m_gmeptr);
			*ppiunkItem = punk.Detach();
		}
		else if (m_mgaproj && wcscmp(pstrName, L"project") == 0)
		{
			CComPtr<IUnknown> punk(m_mgaproj);
			*ppiunkItem = punk.Detach();
		}
		else if (m_actMod && wcscmp(pstrName, L"it") == 0)
		{
			CComPtr<IUnknown> punk(m_actMod);
			*ppiunkItem = punk.Detach();
		}
		else
		{
			return TYPE_E_ELEMENTNOTFOUND;
		}

		return S_OK;
	}
	if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
	{
	}
	*ppti = NULL;
	return S_OK;
}

STDMETHODIMP CScriptHost::GetDocVersionString(/*[out]*/ BSTR      *pbstrVersion)
{
	return E_NOTIMPL;
}

STDMETHODIMP CScriptHost::OnScriptTerminate(
                /*[in]*/  const VARIANT     *pvarResult,
                /*[in]*/  const EXCEPINFO   *pexcepinfo)
{
	return S_OK;
}

STDMETHODIMP CScriptHost::OnStateChange(
                /*[in]*/  SCRIPTSTATE       ssScriptState)
{
	return S_OK;
}

STDMETHODIMP CScriptHost::OnScriptError(
                /*[in]*/  IActiveScriptError *pscripterror)
{
	try
	{
		IActiveScriptErrorPtr perr(pscripterror);

		EXCEPINFO expinfo;
		COMTHROW(perr->GetExceptionInfo(&expinfo));
		DWORD	sc;
		ULONG	line = 0;
		LONG	ch = 0;
		COMTHROW(perr->GetSourcePosition(&sc, &line, &ch));

		// write to console 
		CString desc = expinfo.bstrDescription;
		// make the error description html readable
		desc.Replace( _T("<"), _T("&lt;"));		// replacing <
		desc.Replace( _T(">"), _T("&gt;"));		// >
		desc.Replace(_T("\r\n"), _T("<br>"));	// 0d0a newlines
		desc.Replace(_T("\n"), _T("<br>"));		// 0a newlines

		TCHAR err[5000];
		_stprintf_s(err, _T("Scripting Error at Position: %ld Line: %lu<br>%s"), ch, line, (const TCHAR*)desc);
		Message(_bstr_t(err), MSG_ERROR);
	}
	catch(hresult_exception &e) 
	{ 
		return e.hr;
	}
	catch(_com_error &e) 
	{ 
		return e.Error();
	}

	return S_OK;
}

STDMETHODIMP CScriptHost::OnEnterScript(void)
{
	return S_OK;
}

STDMETHODIMP CScriptHost::OnLeaveScript(void)
{
	return S_OK;
}

STDMETHODIMP CScriptHost::SetGMEApp(IDispatch *gmeapp)
{

	if (m_gmeptr == NULL  &&  gmeapp == NULL)
		return S_OK;
	try
	{
		m_gmeptr.Release();
		if (gmeapp == NULL)
		{
			// release other pointers
			m_mgaproj.Release();
			m_actMod.Release();

			// delete names 
			if (m_iscript != NULL)
			{
				COMTHROW(m_iscript->Close());
				m_iscript.Release();
			}
			COMTHROW(m_iscript.CreateInstance((BSTR)m_enginePID));
			m_iscriptParse = m_iscript;
			IActiveScriptSitePtr scs(this);
			COMTHROW(m_iscript->SetScriptSite(scs));
			COMTHROW(m_iscriptParse->InitNew());
			COMTHROW(m_iscript->SetScriptState(SCRIPTSTATE_CONNECTED));
			return S_OK;
		}

		CComPtr<IDispatch> gip(gmeapp);
		COMTHROW(gip.QueryInterface(&m_gmeptr));

		_bstr_t gme(L"gme");
		COMTHROW(m_iscript->AddNamedItem(gme, SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE));
		COMTHROW(m_iscript->SetScriptState(SCRIPTSTATE_CONNECTED));
	}
	catch(hresult_exception &e) 
	{ 
		return e.hr;
	}
	catch(_com_error &e) 
	{ 
		return e.Error();
	}
	return S_OK;
}

STDMETHODIMP CScriptHost::SetGMEProj(IDispatch *gmeapp)
{

	if (m_gmeptr == NULL  &&  gmeapp == NULL)
		return S_OK;
	try
	{
		m_mgaproj.Release();
		m_actMod.Release();

		if (gmeapp == NULL) // means close project
		{
			return S_OK;
		}

		COMTHROW(m_gmeptr->get_MgaProject(&m_mgaproj));

		COMTHROW( m_gmeptr->get_OleIt( &m_actMod ));

		_bstr_t project(L"project");
		_bstr_t this_model(L"it");
		COMTHROW(m_iscript->AddNamedItem(project, SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE));
		COMTHROW(m_iscript->AddNamedItem(this_model, SCRIPTITEM_ISVISIBLE|SCRIPTITEM_ISSOURCE));
		COMTHROW(m_iscript->SetScriptState(SCRIPTSTATE_CONNECTED));
	}
	catch(hresult_exception &e) 
	{ 
		return e.hr;
	}

	return S_OK;
}

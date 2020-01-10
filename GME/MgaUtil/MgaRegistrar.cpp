#include "stdafx.h"
#include "MgaRegistrar.h"
#include "atlconv.h"
#include "CommonComponent.h"
#include "Parser.h"

#import "mscorlib.tlb"
#import "libid:5f45c5d6-4e11-42fc-a558-cfa0b6c7bea6" // MgaDotNetServices

#include "atlsafe.h"

#ifdef _DEBUG
void ERRTHROW(LONG err)
{
	VERIFYTHROW( err == ERROR_SUCCESS );
}
#else
#define ERRTHROW(FUNC) \
{ \
	VERIFYTHROW( (FUNC) == ERROR_SUCCESS ); \
}
#endif

void WIN32THROW(LONG err)
{
	if (err != ERROR_SUCCESS)
		HR_THROW(HRESULT_FROM_WIN32(err));
}

const CString rootreg(_T("SOFTWARE\\GME"));


CString QueryValue(CRegKey &key, const TCHAR *name)
{
	CString buffer;
	TCHAR *bufp = buffer.GetBuffer(4096);
	ULONG buflen = 4095;
	LONG err = key.QueryStringValue( name, bufp, &buflen);
	if( err != ERROR_SUCCESS ) {
		buflen = 0;
		if(err != ERROR_FILE_NOT_FOUND) ERRTHROW(err);
	}
	ASSERT( 0 <= buflen && buflen < 4096 );
	bufp[buflen] = _T('\0');
	buffer.ReleaseBuffer();

	return buffer;
}

// --------------------------- CMgaRegistrar

#define RM_USER 1 
#define RM_SYS  2
#define RM_SYS2 4
#define RM_BOTH 3
#define RM_TEST 8
#define RM_PRIO		 (RM_SYS2 | RM_USER)
#define RM_SYSDOREAD (RM_SYS2 | RM_SYS)

// For read operations: USER, SYS, PRIO
// For multi-return read operations: USER, SYS, BOTH, PRIO
// For write operations: USER, SYS, BOTH, TEST

void REVOKE_SYS2(regaccessmode_enum &mode) { *(int *)&mode &= ~RM_SYS2; }

enum Tristate_t {
	Tristate_Enabled,
	Tristate_Disabled,
	Tristate_Not_Specified,
};

bool Combine_Tristate(Tristate_t user, Tristate_t system, bool default_ = false) {
	if (user != Tristate_Not_Specified) {
		return user == Tristate_Enabled;
	}
	if (system != Tristate_Not_Specified) {
		return system == Tristate_Enabled;
	}
	return default_;
}

Tristate_t IsAssociated_hive(const CString& progidstr, const CString& paradigmstr, HKEY hive) {
	CRegKey acomp;

	if (acomp.Open(hive, rootreg + _T("\\Components\\") + progidstr + _T("\\Associated"), KEY_READ) != ERROR_SUCCESS) {
		return Tristate_Not_Specified;
	}
	ULONG count = 0;
	DWORD res = acomp.QueryValue(paradigmstr, NULL, NULL, &count);
	if (res != ERROR_SUCCESS) {
		return Tristate_Not_Specified;
	}
	CString val;
	if (acomp.QueryStringValue(paradigmstr, val.GetBufferSetLength(count), &count) == ERROR_SUCCESS) {
		val.ReleaseBuffer();
		if (val == _T("Disabled")) {
			return Tristate_Disabled;
		}
	}
	return Tristate_Enabled;
}

bool IsAssociated_regaccess(const CString& progidstr, const CString& paradigmstr, regaccessmode_enum mode) {
	if (mode & REGACCESS_BOTH) {
		return  Combine_Tristate(
			IsAssociated_hive(progidstr, paradigmstr, HKEY_CURRENT_USER),
			IsAssociated_hive(progidstr, paradigmstr, HKEY_LOCAL_MACHINE));
	} else if (mode & REGACCESS_USER) {
		return IsAssociated_hive(progidstr, paradigmstr, HKEY_CURRENT_USER) == Tristate_Enabled;
	} else if (mode & REGACCESS_SYSTEM) {
		return IsAssociated_hive(progidstr, paradigmstr, HKEY_LOCAL_MACHINE) == Tristate_Enabled;
	}
	return false;
}

STDMETHODIMP CMgaRegistrar::get_IconPath(regaccessmode_enum mode, BSTR *path)
{
	CHECK_OUT(path);

	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("IconPath"));
				str.TrimRight(_T(" ;,\t"));
				if(!str.IsEmpty()) REVOKE_SYS2(mode);
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			CString str2;
			if(res == ERROR_SUCCESS) {
				str2 = QueryValue(mga, _T("IconPath"));
			}
			if (str2 == "") {
				str2 = "$PARADIGMDIR\\icons;$PROJECTDIR\\icons";
			}
			str2.TrimLeft(_T(" ;,\t"));
			if(!str.IsEmpty() && !str2.IsEmpty())
			{
				str += _T(";");
				str	+= str2;
			}
			else
				str = str2;
		}
		CopyTo(str, path);
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_IconPath(regaccessmode_enum mode, BSTR path)
{
	COMTRY
	{
		CString str = path;
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("IconPath"), str ));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("IconPath"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_ShowMultipleView(regaccessmode_enum mode, VARIANT_BOOL *enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("ShowMultipleView"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("ShowMultipleView"));
			}
		}
		*enabled = (str == _T("1")) ? VARIANT_TRUE : VARIANT_FALSE; // Default value: false
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_ShowMultipleView(regaccessmode_enum mode, VARIANT_BOOL enabled)
{
	COMTRY
	{
		CString str = (enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("ShowMultipleView"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("ShowMultipleView"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_EventLoggingEnabled(regaccessmode_enum mode, VARIANT_BOOL *enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("EventLoggingEnabled"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("EventLoggingEnabled"));
			}
		}
		*enabled = (str == _T("1")) ? VARIANT_TRUE : VARIANT_FALSE; // Default value: false
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_EventLoggingEnabled(regaccessmode_enum mode, VARIANT_BOOL enabled)
{
	COMTRY
	{
		CString str = (enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("EventLoggingEnabled"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("EventLoggingEnabled"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_AutosaveEnabled(regaccessmode_enum mode, VARIANT_BOOL *enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("AutosaveEnabled"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("AutosaveEnabled"));
			}
		}
		*enabled = (str == _T("1")) ? VARIANT_TRUE : VARIANT_FALSE; // Default value: false
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_AutosaveEnabled(regaccessmode_enum mode, VARIANT_BOOL enabled)
{
	COMTRY
	{
		CString str = (enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("AutosaveEnabled"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("AutosaveEnabled"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_AutosaveFreq(regaccessmode_enum mode, long *secs)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("AutosaveFreq"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("AutosaveFreq"));
			}
		}
		if (_stscanf((LPCTSTR)str, _T("%ld"), secs) != 1) {
			*secs = 60;	// Default value: 1 minute
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_AutosaveFreq(regaccessmode_enum mode, long secs)
{
	COMTRY
	{
		CString str;
		str.Format(_T("%ld"), secs);
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("AutosaveFreq"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("AutosaveFreq"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_AutosaveUseDir(regaccessmode_enum mode, VARIANT_BOOL *enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("AutosaveUseDir"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("AutosaveUseDir"));
			}
		}
		*enabled = (str == _T("1")) ? VARIANT_TRUE : VARIANT_FALSE; // Default value: false
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_AutosaveUseDir(regaccessmode_enum mode, VARIANT_BOOL enabled)
{
	COMTRY
	{
		CString str = (enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("AutosaveUseDir"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("AutosaveUseDir"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_AutosaveDir(regaccessmode_enum mode, BSTR *dir)
{
	CHECK_OUT(dir);

	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("AutosaveDir"));
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("AutosaveDir"));
			}
		}
		CopyTo(str, dir);
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_AutosaveDir(regaccessmode_enum mode, BSTR dir)
{
	COMTRY
	{
		CString str = dir;
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("AutosaveDir"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("AutosaveDir"), str));
		}
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::get_ExternalEditorEnabled(regaccessmode_enum mode, VARIANT_BOOL *enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("ExternalEditorEnabled"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("ExternalEditorEnabled"));
			}
		}
		*enabled = (str == _T("1")) ? VARIANT_TRUE : VARIANT_FALSE; // Default value: false
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_ExternalEditorEnabled(regaccessmode_enum mode, VARIANT_BOOL enabled)
{
	COMTRY
	{
		CString str = (enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("ExternalEditorEnabled"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("ExternalEditorEnabled"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_ExternalEditor(regaccessmode_enum mode, BSTR *path)
{
	CHECK_OUT(path);

	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("ExternalEditor"));
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("ExternalEditor"));
			}
		}
		CopyTo(str, path);
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_ExternalEditor(regaccessmode_enum mode, BSTR path)
{
	COMTRY
	{
		CString str = path;
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("ExternalEditor"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("ExternalEditor"), str));
		}
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::get_UseAutoRouting(regaccessmode_enum mode, VARIANT_BOOL *enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("UseAutoRouting"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("UseAutoRouting"));
			}
		}
		*enabled = (str == _T("0")) ? VARIANT_FALSE : VARIANT_TRUE; // Default value: true
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::put_UseAutoRouting(regaccessmode_enum mode, VARIANT_BOOL enabled)
{
	COMTRY
	{
		CString str = (enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("UseAutoRouting"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("UseAutoRouting"), str));
		}
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::get_LabelAvoidance(regaccessmode_enum mode, VARIANT_BOOL *enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("LabelAvoidance"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("LabelAvoidance"));
			}
		}
		*enabled = (str == _T("1")) ? VARIANT_TRUE : VARIANT_FALSE; // Default value: false
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::put_LabelAvoidance(regaccessmode_enum mode, VARIANT_BOOL enabled)
{
	COMTRY
	{
		CString str = (enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("LabelAvoidance"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("LabelAvoidance"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_ScriptEngine(regaccessmode_enum mode, BSTR *path) {
	CHECK_OUT(path);

	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("ScriptEngine"));
				str.TrimRight(_T(" ;,\t"));
				if(!str.IsEmpty()) REVOKE_SYS2(mode);
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				CString str2 = QueryValue(mga, _T("ScriptEngine"));
				str2.TrimLeft(_T(" ;,\t"));
				if(!str.IsEmpty() && !str2.IsEmpty()) str += _T(";");
				str	+= str2;
			}
		}
		CopyTo(str, path);
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_ScriptEngine(regaccessmode_enum mode, BSTR path) {
	COMTRY
	{
		CString str = path;
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue(_T("ScriptEngine"), str) );//z7
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue(_T("ScriptEngine"), str) );//z7
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::GetDefZoomLevel(regaccessmode_enum p_mode, BSTR *p_zlev)
{
	CHECK_OUT(p_zlev);
	COMTRY
	{
		LONG res;
		CString str;
		if(p_mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("DefaultZoomLevel"));
				if(!str.IsEmpty()) REVOKE_SYS2(p_mode);
			}
		}
		if(p_mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("DefaultZoomLevel"));
			}
		}
		CopyTo(str, p_zlev);
	}
	COMCATCH(;)

}

STDMETHODIMP CMgaRegistrar::SetDefZoomLevel(regaccessmode_enum p_mode, BSTR p_zlev)
{
	COMTRY
	{
		CString str = p_zlev;
		if(p_mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("DefaultZoomLevel"), str));
		}
		if(p_mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(p_mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("DefaultZoomLevel"), str));
		}
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::GetMouseOverNotify(regaccessmode_enum mode, VARIANT_BOOL *enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("MouseOverNotify"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("MouseOverNotify"));
			}
		}
		*enabled = (str == _T("1")) ? VARIANT_TRUE : VARIANT_FALSE; // Default value: false
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::SetMouseOverNotify(regaccessmode_enum mode, VARIANT_BOOL enabled)
{
	COMTRY
	{
		CString str = (enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("MouseOverNotify"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("MouseOverNotify"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::GetRealNmbFmtStr(regaccessmode_enum p_mode, BSTR *p_fmtStr)
{
	CHECK_OUT(p_fmtStr);
	COMTRY
	{
		LONG res;
		CString str;
		if(p_mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("RealNmbFmtStr"));
				if(!str.IsEmpty()) REVOKE_SYS2(p_mode);
			}
		}
		if(p_mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("RealNmbFmtStr"));
			}
		}
		CopyTo(str, p_fmtStr);
	}
	COMCATCH(;)

}

STDMETHODIMP CMgaRegistrar::SetRealNmbFmtStr(regaccessmode_enum p_mode, BSTR p_fmtStr)
{
	COMTRY
	{
		CString str = p_fmtStr;
		if(p_mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("RealNmbFmtStr"), str));
		}
		if(p_mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(p_mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("RealNmbFmtStr"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::GetTimeStamping(regaccessmode_enum p_mode, VARIANT_BOOL *p_enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(p_mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("TimeStamping"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(p_mode);
				}
			}
		}
		if(p_mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("TimeStamping"));
			}
		}
		*p_enabled = (str == _T("1")) ? VARIANT_TRUE : VARIANT_FALSE; // Default value: false
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::SetTimeStamping(regaccessmode_enum p_mode, VARIANT_BOOL p_enabled)
{
	COMTRY
	{
		CString str = (p_enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(p_mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("TimeStamping"), str));
		}
		if(p_mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(p_mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("TimeStamping"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::GetNavigation(regaccessmode_enum p_mode, VARIANT_BOOL *p_enabled)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(p_mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("Navigation"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(p_mode);
				}
			}
		}
		if(p_mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("Navigation"));
			}
		}
		*p_enabled = (str == _T("0")) ? VARIANT_FALSE : VARIANT_TRUE; // Default value: true
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::SetNavigation(regaccessmode_enum p_mode, VARIANT_BOOL p_enabled)
{
	COMTRY
	{
		CString str = (p_enabled == VARIANT_FALSE) ? _T("0") : _T("1");
		if(p_mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("Navigation"), str));
		}
		if(p_mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(p_mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("Navigation"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::GetUndoQueueSize(regaccessmode_enum p_mode, BSTR *p_qusz)
{
	CHECK_OUT(p_qusz);
	COMTRY
	{
		LONG res;
		CString str;
		if(p_mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("UndoQueueSize"));
				if(!str.IsEmpty()) REVOKE_SYS2(p_mode);
			}
		}
		if(p_mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("UndoQueueSize"));
			}
		}
		CopyTo(str, p_qusz);
	}
	COMCATCH(;)

}

STDMETHODIMP CMgaRegistrar::SetUndoQueueSize(regaccessmode_enum p_mode, BSTR p_qusz)
{
	COMTRY
	{
		CString str = p_qusz;
		if(p_mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("UndoQueueSize"), str));
		}
		if(p_mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(p_mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("UndoQueueSize"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_EdgeSmoothMode(regaccessmode_enum mode, edgesmoothmode_enum* smoothMode)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("EdgeSmoothMode"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("EdgeSmoothMode"));
			}
		}
		*smoothMode = (edgesmoothmode_enum)(str.IsEmpty() ? 2 : _tcstol(str, NULL, 10));
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_EdgeSmoothMode(regaccessmode_enum mode, edgesmoothmode_enum smoothMode)
{
	COMTRY
	{
		CString str;
		str.Format(_T("%d"), smoothMode);
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("EdgeSmoothMode"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("EdgeSmoothMode"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_FontSmoothMode(regaccessmode_enum mode, fontsmoothmode_enum* smoothMode)
{
	COMTRY
	{
		LONG res;
		CString str;
		if(mode & RM_USER) {
			CRegKey mga;
			res = mga.Open(HKEY_CURRENT_USER, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str	= QueryValue(mga, _T("FontSmoothMode"));
				if(!str.IsEmpty()) {
					REVOKE_SYS2(mode);
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey mga;
			res = mga.Open(HKEY_LOCAL_MACHINE, rootreg, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				str = QueryValue(mga, _T("FontSmoothMode"));
			}
		}
		*smoothMode = (fontsmoothmode_enum)(str.IsEmpty() ? 4 : _tcstol(str, NULL, 10));
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_FontSmoothMode(regaccessmode_enum mode, fontsmoothmode_enum smoothMode)
{
	COMTRY
	{
		CString str;
		str.Format(_T("%d"), smoothMode);
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			ERRTHROW( mga.SetStringValue( _T("FontSmoothMode"), str));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			if(mode & RM_SYS) ERRTHROW( mga.SetStringValue( _T("FontSmoothMode"), str));
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_Paradigms(regaccessmode_enum mode, VARIANT *names)
{
	CHECK_OUT(names);

	COMTRY
	{
		CStringArray ret;
		if(mode & RM_USER) {
			CRegKey pars;
			LONG res = pars.Open(HKEY_CURRENT_USER, rootreg+_T("\\Paradigms"), KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				for(int index = 0;; ++index) {
					TCHAR name[1024];
					LONG err = RegEnumKey(pars, index, name, 1024);
					if( err == ERROR_NO_MORE_ITEMS )
						break;
					ERRTHROW( err );

					REVOKE_SYS2(mode);
					ret.Add(name);
				}

			}
		}
		int retlen = ret.GetSize();

		if(mode & RM_SYSDOREAD) {
			CRegKey pars;
			LONG res = pars.Open(HKEY_LOCAL_MACHINE, rootreg+_T("\\Paradigms"), KEY_READ | KEY_WOW64_32KEY);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				for(int index = 0;; ++index) {
					TCHAR name[1024];
					LONG err = RegEnumKey(pars, index, name, 1024);
					if( err == ERROR_NO_MORE_ITEMS )
						break;
					ERRTHROW( err );
					int j;
					for(j = 0; j < retlen; j++) {		// Make sure, name is not present already, if yes, system copy is ignored
						if(!ret[j].CompareNoCase(name)) break;
					}
					if(j != retlen) continue;
					ret.Add(name);
				}

			}
		}


		CopyTo(ret, names);
	}
	COMCATCH(;)
}


HRESULT GetMtaInfo(BSTR conn, BSTR *parname, BSTR *version, VARIANT *guid) {
	COMTRY {
		CComObjPtr<IMgaMetaProject> paradigm;
		COMTHROW( paradigm.CoCreateInstance(OLESTR("MGA.MgaMetaProject")) );
		ASSERT( paradigm != NULL );

		COMTHROW( paradigm->Open(conn) );

		COMTHROW( paradigm->get_Name(parname) );

		COMTHROW( paradigm->get_Version(version) );

		COMTHROW( paradigm->get_GUID(guid) );

		COMTHROW( paradigm->Close() );
	} COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::RegisterParadigmFromDataDisp(BSTR connstr, regaccessmode_enum mode, BSTR *newname)
{
	return RegisterParadigmFromData( connstr, newname, mode);
}

STDMETHODIMP CMgaRegistrar::RegisterParadigmFromData(BSTR connstr, BSTR *newname, regaccessmode_enum mode)
{
	CHECK_IN(connstr);
 	if(newname) CHECK_OUT(newname);
	COMTRY {
		CString conn = connstr;
		CString connrecent;
		// we have to parse it
		if( conn.Left(4) == _T("XML=") )
		{
			CString file = conn.Mid(4);

			conn = _T("MGA=");
			conn += file;

			if( conn.Right(4).CompareNoCase(_T(".xml")) == 0 || 
				conn.Right(4).CompareNoCase(_T(".xmp")) == 0 ) {
				conn.Delete(conn.GetLength() - 4, 4);
			}
			conn += _T(".mta");
#define FILEPART(x) (((LPCTSTR)x)+4)
			DWORD info = GetFileAttributes(FILEPART(conn));
			if(info != 0xFFFFFFFF ) {	// save old version of paradigm under another name
			  if(info & FILE_ATTRIBUTE_DIRECTORY) COMTHROW(E_FILEOPEN);
			  try {
				CComBstrObj name;	// Format: name-<GUID>
				CComVariant prevguid;
				CComBstrObj prevversion;
				COMTHROW(GetMtaInfo(PutInBstr(conn), PutOut(name), PutOut(prevversion), PutOut(prevguid)));
				CComBstrObj conn1;
				if(QueryParadigm(name, PutOut(conn1), &prevguid, REGACCESS_PRIORITY) == S_OK &&
					conn1 == CComBSTR(conn)) {  // if it was correctly registered under the previous nonextended name
					GUID  gg;
					CopyTo(prevguid,gg);
					CComBstrObj guidstr;
					CopyTo(gg, guidstr);
					connrecent = conn.Left(conn.GetLength()-4)+_T("-")+CString(PutInCString(guidstr))+_T(".mta");

					bool sysmove = false, usermove = false;
					conn1.Empty();
					if(QueryParadigm(name, PutOut(conn1), &prevguid, REGACCESS_SYSTEM) == S_OK &&
						conn1 == CComBSTR(conn)) {  // if it was correctly registered in system
						if(RegisterParadigm(name, PutInBstr(connrecent), prevversion, prevguid, REGACCESS_TEST) != S_OK) {
							AfxMessageBox(_T("Cannot register this paradigm file\n")
										  _T("an existing '.mta' file with the same name\n")
										  _T("is registered in the system registry\n")
										  _T("which you are not permitted to change.\n")
										  _T("You need to change the name or location\n")
										  _T("of the new paradigm file"));
							return E_INVALID_USAGE;
						}
						sysmove = true;;
					}
					conn1.Empty();
					if(QueryParadigm(name, PutOut(conn1), &prevguid, REGACCESS_USER) == S_OK &&
						conn1 == CComBSTR(conn)) {  // if it was correctly registered in user
						usermove = true;
					}

					if(!MoveFileEx(FILEPART(conn), FILEPART(connrecent), MOVEFILE_REPLACE_EXISTING)) {
						// n.b. can't do this because clients expect ParadigmName.mta
						// conn = conn.Left(conn.GetLength() - 4) + _T("-") + CTime::GetCurrentTime().Format("%Y%m%d_%H%M%S") + _T(".mta");
						COMTHROW(E_FILEOPEN);
					}
					else {
						if (sysmove) {
							COMTHROW(RegisterParadigm(name, PutInBstr(connrecent), prevversion, prevguid, REGACCESS_SYSTEM));
						}
						if (usermove) {
							COMTHROW(RegisterParadigm(name, PutInBstr(connrecent), prevversion, prevguid, REGACCESS_USER));
						}
					}
				}
			  } catch(hresult_exception(&e)) {
					AfxMessageBox(CString(_T("Failure saving previous version of paradigm\n")) + 
									e.what() + 
									(e.hr == E_BINFILE?_T(" [Binary paradigm file (.mta) incompatibility]\nPossible reason: GME version 6 changed the binary format of paradigms."):_T("")) +
									_T("\nOld version will be overwritten"));
					connrecent.Empty();
			  }
			}

			IMgaMetaParserPtr parser;
			COMTHROW(parser.CreateInstance(L"MGA.MgaMetaParser"));
			parser->__Parse(_bstr_t(file), _bstr_t(conn));
		}

		CComBstrObj name;
		CComVariant guid;
		CComBstrObj version;
		COMTHROW(GetMtaInfo(PutInBstr(conn), PutOut(name), PutOut(version), PutOut(guid)));
		if(!connrecent.IsEmpty()) {
			CComBstrObj namer;
			CComVariant guidr;
			CComBstrObj versionr;
			COMTHROW(GetMtaInfo(PutInBstr(connrecent), PutOut(namer), PutOut(versionr), PutOut(guidr)));
			// We should change existing registration only here, if the new GUID != the old
		}

		COMTHROW(RegisterParadigm( name, PutInBstr(conn), PutInBstr(version), guid, mode) );
		if(newname) MoveTo(name, newname);
	} COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::RegisterParadigm(BSTR name, BSTR connstr, BSTR version, VARIANT guid, regaccessmode_enum mode)
{
	CString cver(version); 

	COMTRY
	{
		GUID guid2;
		if (guid.vt == (VT_UI1 | VT_ARRAY) && GetArrayLength(guid) == sizeof(GUID))
		{
			CopyTo(guid, guid2);
		}
		else if (guid.vt == VT_BSTR)
		{
			CopyTo(guid.bstrVal, guid2);
		}
		else
			COMTHROW(E_INVALIDARG);

		CComBstrObj guid3;
		CopyTo(guid2, guid3);

		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW(mga.Create(HKEY_CURRENT_USER, rootreg) );

			CRegKey pars;
			ERRTHROW( pars.Create(mga, _T("Paradigms")) );

			CRegKey par;
			ERRTHROW( par.Create(pars, CString(name)) );


			ERRTHROW( par.SetStringValue( _T("CurrentVersion"), PutInCString(guid3)));
			if (!cver.IsEmpty()) {
				ERRTHROW( par.SetStringValue( cver, PutInCString(guid3)));
			}
			CRegKey parg;
			ERRTHROW( parg.Create(par, PutInCString(guid3)) );

			ERRTHROW( parg.SetStringValue( _T("ConnStr"), CString(connstr)));
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			WIN32THROW(mga.Create(HKEY_LOCAL_MACHINE, rootreg) );

			CRegKey pars;
			WIN32THROW( pars.Create(mga, _T("Paradigms"), 0, 0, KEY_READ | KEY_WRITE | KEY_WOW64_32KEY) );

			CRegKey par;

			if (mode & RM_SYS) {
				WIN32THROW( par.Create(pars, CString(name)) );
				CString gg	= QueryValue(par, _T("GUID"));
				CString gc	= QueryValue(par, _T("ConnStr"));
				par.DeleteValue(_T("GUID"));
				par.DeleteValue(_T("ConnStr"));
				if(!gc.IsEmpty() && !gg.IsEmpty()) {
					CRegKey parg2;
					WIN32THROW( parg2.Create(par, gg) );
					WIN32THROW( parg2.SetStringValue( _T("ConnStr"), gc) );
				}

				WIN32THROW( par.SetStringValue( _T("CurrentVersion"), PutInCString(guid3)));
				if (!cver.IsEmpty()) {
					WIN32THROW( par.SetStringValue( cver, PutInCString(guid3)));
				}
				CRegKey parg;
				WIN32THROW( parg.Create(par, PutInCString(guid3)) );

				WIN32THROW( parg.SetStringValue( _T("ConnStr"), CString(connstr)));
			} else {
				LONG res = par.Open(pars, CString(name));
				if(res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) WIN32THROW(res);
			}
		}
		if ((!(mode & RM_USER) || IsAssociated_hive(L"Mga.AddOn.ConstraintManager", name, HKEY_CURRENT_USER) != Tristate_Disabled) &&
			(!(mode & RM_SYS) || IsAssociated_hive(L"Mga.AddOn.ConstraintManager", name, HKEY_LOCAL_MACHINE) != Tristate_Disabled)) {
			Associate(CComBSTR(L"Mga.AddOn.ConstraintManager"), name, mode); // no error checking
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::QueryParadigmAllGUIDs(BSTR parname, VARIANT *guidstrs, regaccessmode_enum mode) {
	CHECK_OUT(guidstrs);
	COLE2CT parname2(parname);
	CString pname(parname2); 

	COMTRY
	{
		CStringArray ret;
		if(mode & RM_USER) {
			CRegKey par;
			LONG res = par.Open(HKEY_CURRENT_USER, rootreg+_T("\\Paradigms\\") + pname, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				for(int index = 0;; ++index) {
					TCHAR name[1024];
					LONG err = RegEnumKey(par, index, name, 1024);
					if( err == ERROR_NO_MORE_ITEMS )
						break;
					ERRTHROW( err );

					REVOKE_SYS2(mode);
					ret.Add(name);
				}

			}
		}
		int retlen = ret.GetSize();

		if(mode & RM_SYSDOREAD) {
			CRegKey par;
			LONG res = par.Open(HKEY_LOCAL_MACHINE, rootreg+_T("\\Paradigms\\") + pname, KEY_READ | KEY_WOW64_32KEY);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				CString cur = QueryValue(par, _T("CurrentVersion"));
				if(!cur.IsEmpty()) {  // New style: Connection strings are stored under GUID subkeys
				  for(int index = 0;; ++index) {
					TCHAR name[1024];
					LONG err = RegEnumKey(par, index, name, 1024);
					if( err == ERROR_NO_MORE_ITEMS )
						break;
					ERRTHROW( err );
					int j;
					for(j = 0; j < retlen; j++) {		// Make sure, name is not present already, if yes system copy is ignored
						if(!ret[j].CompareNoCase(name)) break;
					}
					if(j != retlen) continue;
					ret.Add(name);
				  }
				}
				else {
					CString name = QueryValue(par, _T("GUID"));
					int j;
					for(j = 0; j < retlen; j++) {		// Make sure, name is not present already, if yes system copy is ignored
						if(!ret[j].CompareNoCase(name)) break;
					}
					if(j == retlen) ret.Add(name);
				}
			}
		}


		CopyTo(ret, guidstrs);
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::QueryParadigm(BSTR parname, BSTR *connstr, VARIANT *guid, regaccessmode_enum mode)
{
	CHECK_OUT(connstr);
//	CHECK_OUT(guid);

	COMTRY
	{
		CString pname = parname;

		CRegKey subk;
		CString guidact;

		CComBstrObj inguidstr;
		if(guid->vt != VT_EMPTY && guid->vt != VT_NULL) {
			GUID g;
			CopyTo(*guid, g);
			CopyTo(g, inguidstr);
//			if(inguidstr == L"{00000000-0000-0000-0000-000000000000}") inguidstr = NULL;
		}

		LONG res;

		if(mode & RM_USER) {
			CRegKey par;
			res = par.Open(HKEY_CURRENT_USER, rootreg + _T("\\Paradigms\\") + pname, KEY_READ);
			if(res == ERROR_SUCCESS) {
				if(inguidstr == NULL) {
					guidact = QueryValue(par, _T("CurrentVersion"));
					if(guidact.IsEmpty()) res = ERROR_FILE_NOT_FOUND;
				}
				else
					guidact = inguidstr;
			}
			if(res == ERROR_SUCCESS) res = subk.Open(par, guidact, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS)   mode = REGACCESS_USER;
		}

		if(mode & RM_SYSDOREAD) {
			CRegKey par;
			res = par.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Paradigms\\") + pname, KEY_READ | KEY_WOW64_32KEY);
			if(res == ERROR_SUCCESS) {
				CString cur = QueryValue(par, _T("CurrentVersion"));
				if (cur.IsEmpty() && inguidstr == NULL) {
					guidact = QueryValue(par, _T("GUID"));
					if (inguidstr != NULL && inguidstr != CComBSTR(guidact))
						COMTHROW(E_NOTFOUND);
					subk.Attach(par.Detach());
				}
				else {
					if (inguidstr == NULL)
						guidact = cur;
					else
						guidact = inguidstr;
					res = subk.Open(par, guidact, KEY_READ);
					if (res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND)
						ERRTHROW(res);
				}
			}
		}
		if (subk == NULL)
		{
			CString error;
			if (inguidstr)
				error.Format(L"Paradigm '%s' with GUID '%s' is not registered.", static_cast<const wchar_t*>(pname),
					static_cast<const wchar_t*>(inguidstr));
			else
				error.Format(L"Paradigm '%s' is not registered.", static_cast<const wchar_t*>(pname));
			SetErrorInfo(error);
			return E_NOTFOUND;
		}


		CopyTo(QueryValue(subk, _T("ConnStr")), connstr);

		if(inguidstr == NULL) {
			GUID g;
			CopyTo(CComBSTR(guidact),g); 
			CopyTo(g, guid);
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_ParadigmGUIDString(regaccessmode_enum mode, BSTR parname, BSTR *guidstr)
{
	CHECK_OUT(guidstr);
	COMTRY
	{
		CComBSTR connstr;
		CComVariant guid;
		HRESULT hr = QueryParadigm(parname, &connstr, &guid, mode);
		if (FAILED(hr))
			return hr;
		GUID g;
		CopyTo(guid, g);
		CopyTo(g, guidstr);
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::VersionFromGUID(BSTR name, VARIANT guid, BSTR *ver, regaccessmode_enum mode)
{
	CHECK_OUT(ver);

	bool found = false;
	COMTRY
	{
		GUID gg;
		CopyTo(guid,gg);
		CComBstrObj guidbstr;
		CopyTo(gg, guidbstr);

		LONG res;

		if(mode & RM_USER) {
			CRegKey par;
			res = par.Open(HKEY_CURRENT_USER, rootreg + _T("\\Paradigms\\")+name, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				mode = REGACCESS_USER;
				for(int index = 0;; ++index) {
					TCHAR name[512];
					DWORD namesize = sizeof(name) / sizeof(name[0]);
					TCHAR value[512];
					DWORD valuesize = sizeof(value);
					DWORD valtype;

					LONG err = RegEnumValue(par, index, name, &namesize, NULL, &valtype, (LPBYTE)value, &valuesize);
					if( err == ERROR_NO_MORE_ITEMS )
						break;
					ERRTHROW( err );
					if (valtype == REG_SZ) {
						CString cver(value);
						if (cver.Compare(PutInCString(guidbstr)) == 0) {
							CString namestr(name);
							if (namestr.CompareNoCase(_T("CurrentVersion")) != 0) {
								found = true;
								CopyTo(namestr, ver);
							}
						}
					}
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey par;
			res = par.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Paradigms\\")+name, KEY_READ | KEY_WOW64_32KEY);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				for(int index = 0;; ++index) {
					TCHAR name[512];
					DWORD namesize = sizeof(name) / sizeof(name[0]);
					TCHAR value[512];
					DWORD valuesize = sizeof(value);
					DWORD valtype;

					LONG err = RegEnumValue(par, index, name, &namesize, NULL, &valtype, (BYTE*)value, &valuesize);
					if( err == ERROR_NO_MORE_ITEMS )
						break;
					ERRTHROW( err );
					if (valtype == REG_SZ) {
						CString cver(value);
						if (cver.Compare(PutInCString(guidbstr)) == 0) {
							CString namestr(name);
							if (namestr.CompareNoCase(_T("CurrentVersion")) != 0) {
								found = true;
								CopyTo(namestr, ver);
							}
						}
					}
				}
			}
		}
		if (!found)
		{
			SetErrorInfo(E_NOTFOUND);
			return(E_NOTFOUND);
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::GUIDFromVersion(BSTR name, BSTR ver, VARIANT* guid, regaccessmode_enum mode)
{
	CHECK_OUT(guid);

	CString verstr = ver;
	CString gstr;

	COMTRY
	{
		LONG res;

		if(mode & RM_USER) {
			CRegKey par;
			res = par.Open(HKEY_CURRENT_USER, rootreg + _T("\\Paradigms\\")+name, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) { 
				gstr = QueryValue(par, verstr);
				if (!gstr.IsEmpty()) {
					mode = REGACCESS_USER;
				}
			}
		}
		if(mode & (RM_SYSDOREAD)) {
			CRegKey par;
			res = par.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Paradigms\\")+name, KEY_READ | KEY_WOW64_32KEY);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) { 
				gstr = QueryValue(par, verstr);
			}
		}

		if (gstr.IsEmpty()) {
			CString error;
			if (SysStringLen(ver))
				error.Format(L"Paradigm '%s' with version '%s' is not registered.", name, ver);
			else
				error.Format(L"Paradigm '%s' is not registered.", name);
			SetErrorInfo(error);
			return E_NOTFOUND;
		}

		GUID g;
		CopyTo(CComBSTR(gstr),g); 
		CopyTo(g, guid);
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::UnregisterParadigmGUID(BSTR name, VARIANT v, regaccessmode_enum mode)
{				// it cannot be unregistered if it is the current version
	COMTRY
	{
		GUID gg;
		CopyTo(v,gg);
		CComBstrObj guidbstr;
		CopyTo(gg, guidbstr);

		if(mode & RM_USER) {
			CRegKey par;
			ERRTHROW( par.Open(HKEY_CURRENT_USER, rootreg + _T("\\Paradigms\\")+name) );
			CString cur = QueryValue(par, _T("CurrentVersion"));
			if(cur.Compare(PutInCString(guidbstr)) == 0) {
				COMTHROW(E_INVALID_USAGE);
			}
			
			for(int index = 0;; ++index) {
				TCHAR name[512];
				DWORD namesize = sizeof(name) / sizeof(name[0]);
				TCHAR value[512];
				DWORD valuesize = sizeof(value);
				DWORD valtype;

				LONG err = RegEnumValue(par, index, name, &namesize, NULL, &valtype, (BYTE*)value, &valuesize);
				if( err == ERROR_NO_MORE_ITEMS )
					break;
				ERRTHROW( err );
				if (valtype == REG_SZ) {
					CString cver(value);
					if (cver.Compare(PutInCString(guidbstr)) == 0) {
						RegDeleteValue(par, name);
					}
				}
			}

			ERRTHROW( par.RecurseDeleteKey(PutInCString(guidbstr)) );
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey par;
			ERRTHROW( par.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Paradigms\\")+name, KEY_READ | KEY_WRITE | KEY_WOW64_32KEY) );
			CString cur = QueryValue(par, _T("CurrentVersion"));
			if(cur.Compare(PutInCString(guidbstr)) == 0) {
				COMTHROW(E_INVALID_USAGE);
			}
			if(mode & RM_SYS) {
				ERRTHROW( par.RecurseDeleteKey(PutInCString(guidbstr)) );
				for(int index = 0;; ++index) {
					TCHAR name[512];
					DWORD namesize = sizeof(name) / sizeof(name[0]);
					TCHAR value[512];
					DWORD valuesize = sizeof(value);
					DWORD valtype;

					LONG err = RegEnumValue(par, index, name, &namesize, NULL, &valtype, (BYTE*)value, &valuesize);
					if( err == ERROR_NO_MORE_ITEMS )
						break;
					ERRTHROW( err );
					if (valtype == REG_SZ) {
						CString cver(value);
						if (cver.Compare(PutInCString(guidbstr)) == 0) {
							RegDeleteValue(par, name);
						}
					}
				}
			}
			if(mode & RM_TEST) ERRTHROW( par.Open(par, PutInCString(guidbstr)) );
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::UnregisterParadigm(BSTR name, regaccessmode_enum mode)
{
	COMTRY
	{
		if(mode & RM_USER) {
			CRegKey pars;
			LONG res = pars.Open(HKEY_CURRENT_USER, rootreg + _T("\\Paradigms"));
			if(!res) res = pars.RecurseDeleteKey(CString(name));
			if(res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
			ERRTHROW(res);
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey pars;
			LONG res = pars.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Paradigms"), KEY_READ | KEY_WRITE | KEY_WOW64_32KEY);
			if(!res) {
				if(mode & RM_SYS) res = pars.RecurseDeleteKey(PutInCString(name));
				if(mode & RM_TEST) res = pars.Open(pars, PutInCString(name));
			}
			if(res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
			ERRTHROW(res);
		}
	}
	COMCATCH(;)
}

// throws hresult_exception
void GetComponents_(HKEY hive, CStringArray& ret) {
	CRegKey comps;
	LONG res = comps.Open(hive, rootreg + _T("\\Components"), KEY_READ);
	if (res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND)
		COMTHROW(HRESULT_FROM_WIN32(res));
	if (res == ERROR_SUCCESS) {
		for (int index = 0;; ++index) {
			TCHAR name[1024];
			LONG err = RegEnumKey(comps, index, name, 1024);
			if (err == ERROR_NO_MORE_ITEMS)
				break;
			ERRTHROW(err);

			CRegKey comp;
			err = comp.Open(comps, name, KEY_READ);
			DWORD type2;
			if (err == ERROR_SUCCESS)
				err = comp.QueryDWORDValue(_T("Type"), type2);
			if (err != ERROR_SUCCESS)
				continue;
			if((type2 & COMPONENTTYPE_SYSREGREF) != 0) { 
				continue;
			}
			// Make sure name is not present already
			int j;
			for (j = 0; j < ret.GetSize(); j++) {
				if (!ret[j].CompareNoCase(name))
					break;
			}
			if (j != ret.GetSize())
				continue;
			ret.Add(name);
		}
	}
}

STDMETHODIMP CMgaRegistrar::get_Components(regaccessmode_enum mode, VARIANT *progids)
{
	CHECK_OUT(progids);

	COMTRY
	{
		CStringArray ret;
		if (mode & REGACCESS_USER)
			GetComponents_(HKEY_CURRENT_USER, ret);
		if (mode & REGACCESS_SYSTEM)
			GetComponents_(HKEY_LOCAL_MACHINE, ret);
		CopyTo(ret, progids);
	}
	COMCATCH(;)
}

template<typename Functor>
void TokenizeParadigmString(const CString& paradigms, Functor& x) {
	CStringList ret;
	int curPos = 0;
	CString token = paradigms.Tokenize(_T(" ;,\n\t"), curPos);
	while (token != _T("")) {
		x(token);
		token = paradigms.Tokenize(_T(" ;,\n\t"), curPos);
	}
}

STDMETHODIMP CMgaRegistrar::RegisterComponent(BSTR progid, componenttype_enum type, BSTR desc, regaccessmode_enum mode)
{
	COMTRY
	{
		CComBstrObj paradigms(OLESTR("*"));
		if(!(type & COMPONENTTYPE_PARADIGM_INDEPENDENT)) {
			paradigms.Empty();
			CComPtr<IMgaComponent> comp;
			HRESULT hr = CreateMgaComponent(comp, progid);
			if(!comp)
			{
				_bstr_t error;
				GetErrorInfo(hr, error.GetAddress());
				if (!static_cast<LPOLESTR>(error))
					error = L"Unknown error";
				throw_com_error(E_MGA_COMPONENT_ERROR, _bstr_t(L"Could not create: ") + progid + L": " + error);
			}
			COMTHROW(comp->get_Paradigm(PutOut(paradigms)));
		}
		if(mode & RM_USER) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_CURRENT_USER, rootreg) );
			CRegKey comps;
			ERRTHROW( comps.Create(mga, _T("Components")) );
			CRegKey comp;
			ERRTHROW( comp.Create(comps, PutInCString(progid)) );

			ERRTHROW( comp.SetDWORDValue( _T("Type"), (DWORD)type));

			ERRTHROW( comp.SetStringValue( _T("Description"), PutInCString(desc)));
			if(paradigms.Length()) {
				TokenizeParadigmString(PutInCString(paradigms), [&comp](const CString& par) {
					ERRTHROW(comp.SetStringValue(_T("Paradigm"), par));
				});
			}
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey mga;
			ERRTHROW( mga.Create(HKEY_LOCAL_MACHINE, rootreg) );
			CRegKey comps;
			ERRTHROW( comps.Create(mga, _T("Components")) );
			if(mode & RM_SYS) {
				CRegKey comp;
				ERRTHROW( comp.Create(comps, PutInCString(progid)) );
	
				ERRTHROW( comp.SetDWORDValue( _T("Type"), (DWORD)type));

				ERRTHROW( comp.SetStringValue( _T("Description"), PutInCString(desc)));
				TokenizeParadigmString(PutInCString(paradigms), [&comp](const CString& par) {
					ERRTHROW(comp.SetStringValue(_T("Paradigm"), par));
				});
			}
			else {
				CRegKey comp;
				LONG res = comp.Open(comps, PutInCString(progid));
				if(res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			}
		}
	}
	COMCATCH(;)
}

// FIXME: mode is ignored. It should be removed next interface-breaking change
STDMETHODIMP CMgaRegistrar::QueryComponent(BSTR progid, componenttype_enum *type, BSTR *desc, regaccessmode_enum mode)
{
	CHECK_OUT(type);
	if(desc) CHECK_OUT(desc);
	CString progidstr = PutInCString(progid);

	COMTRY
	{
		DWORD dwType = -1;
		CString strDesc;
		HKEY hives[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
		for (int i = 0; i < 2; i++) {
			CRegKey comp;
			LONG res = comp.Open(hives[i], rootreg+_T("\\Components\\")+progidstr, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				comp.QueryDWORDValue(_T("Type"), dwType);
				ULONG count = 0;
				res = comp.QueryStringValue(_T("Description"), NULL, &count);
				if (strDesc == _T("") && res == ERROR_SUCCESS) {
					CString ret;
					if (comp.QueryStringValue(_T("Description"), ret.GetBufferSetLength(count), &count) == ERROR_SUCCESS) {
						ret.ReleaseBuffer();
						strDesc = ret;
					}
				}
			}
		}
		if (dwType == -1 || !(dwType & COMPONENTTYPE_ALL))
			COMTHROW(E_NOTFOUND);
		
		if (type)
			*type = (componenttype_enum)dwType;
		if (desc && strDesc != _T(""))
			CopyTo(strDesc, desc);
		return S_OK;
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::UnregisterComponent(BSTR progid, regaccessmode_enum mode)
{
	COMTRY
	{
//		if(mode & RM_USER) {
		if(mode & (RM_USER | RM_SYS) ){
			CRegKey comps;
			LONG res = comps.Open(HKEY_CURRENT_USER, rootreg + _T("\\Components"));

			DWORD type2 = 0;
			if(!res) {
				CRegKey comp;
				res =  comp.Open(comps, PutInCString(progid), KEY_READ);
				if(!res) comp.QueryDWORDValue( _T("Type"), type2);
			}

			if((mode & RM_USER) || (type2 & COMPONENTTYPE_SYSREGREF) != 0 ) {
				if (!res)
					res = comps.RecurseDeleteKey(PutInCString(progid));
				if (res == ERROR_FILE_NOT_FOUND)
					res = ERROR_SUCCESS;
				if (res != ERROR_SUCCESS)
					WIN32THROW(res);
			}
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey comps;
			LONG res = comps.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Components"));
			if(!res) {
				if(mode & RM_SYS) res = comps.RecurseDeleteKey(PutInCString(progid));
				if(mode & RM_TEST) res = comps.Open(comps, PutInCString(progid));
			}
			if(res == ERROR_FILE_NOT_FOUND)
				res = ERROR_SUCCESS;
			if (res != ERROR_SUCCESS)
				WIN32THROW(res);
		}
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::put_ComponentExtraInfo(regaccessmode_enum mode, 
												   BSTR progid, BSTR name, BSTR newVal) {
	CString progidstr = PutInCString(progid);

	COMTRY
	{
		if(mode & RM_USER) {
			CRegKey comp;
			LONG res = comp.Open(HKEY_CURRENT_USER, rootreg+_T("\\Components\\")+progidstr);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				DWORD type2;
				ERRTHROW( comp.QueryDWORDValue( _T("Type"), type2));

				if((type2 & COMPONENTTYPE_ALL)) { 
					if(!newVal) { ERRTHROW( comp.DeleteValue(PutInCString(name)) );  }
					else		{ ERRTHROW( comp.SetStringValue( PutInCString(name), PutInCString(newVal))); }
				}
			}
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey comp;
			LONG res = comp.Open(HKEY_LOCAL_MACHINE, rootreg+_T("\\Components\\")+progidstr);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				DWORD type2;
				ERRTHROW( comp.QueryDWORDValue( _T("Type"), type2));

				if((mode & RM_SYS) && (type2 & COMPONENTTYPE_ALL)) { 
					if(!newVal) { ERRTHROW( comp.DeleteValue(PutInCString(name)) );  }
					else		{ ERRTHROW( comp.SetStringValue( PutInCString(name), PutInCString(newVal))); }
				}
			}
		}
	
	} COMCATCH(;)
}

// FIXME: mode is ignored. It should be removed next interface-breaking change
STDMETHODIMP CMgaRegistrar::get_ComponentExtraInfo(regaccessmode_enum mode, 
												   BSTR progid, BSTR name, BSTR* pVal) {
	CHECK_OUT(pVal);
	CString progidstr = PutInCString(progid);

	COMTRY
	{
		HKEY hives[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
		for (int i = 0; i < 2; i++) {
			CRegKey comp;
			LONG res = comp.Open(hives[i], rootreg+_T("\\Components\\")+progidstr, KEY_READ);
			if(res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if(res == ERROR_SUCCESS) {
				ULONG count = 0;
				res = comp.QueryStringValue(PutInCString(name), NULL, &count);
				if (res == ERROR_SUCCESS) {
					CString ret;
					if (comp.QueryStringValue(PutInCString(name), ret.GetBufferSetLength(count), &count) == ERROR_SUCCESS) {
						ret.ReleaseBuffer();
						CopyTo(ret, pVal);
						return S_OK;
					}
				}
			}
		}
		return E_NOTFOUND;
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaRegistrar::get_LocalDllPath(BSTR progid, BSTR* pVal) {
	CHECK_OUT(pVal);
	COMTRY
	{
		CString m_strProgId = PutInCString(progid);
		CRegKey comp;
		CString m_strClassId;
		for(int i = 0; i < 10; i++) {
			LONG res = comp.Open(HKEY_CLASSES_ROOT, m_strProgId + _T("\\CLSID"), KEY_READ);
			if(res == ERROR_SUCCESS) {
				m_strClassId = QueryValue(comp,_T("") );
				break;
			}
			else {
				res = comp.Open(HKEY_CLASSES_ROOT, m_strProgId + _T("\\CurVer"), KEY_READ);
				if(res != ERROR_SUCCESS) COMTHROW(E_NOTFOUND);
				m_strProgId = QueryValue(comp, _T("") );
				comp.Close();
			}
		}
		if(m_strClassId.IsEmpty()) COMTHROW(E_NOTFOUND);

		LONG res = comp.Open(HKEY_CLASSES_ROOT, _T("CLSID\\") + m_strClassId + _T("\\InprocServer32"), KEY_READ);
		CString m_strPath;
		if(res == ERROR_SUCCESS) {
			m_strPath = QueryValue(comp, _T("") );
			if (m_strPath == _T("mscoree.dll")) {
				CString data;
				ULONG count = 1024;
				if (comp.QueryStringValue(_T("CodeBase"), data.GetBufferSetLength(count), &count) == ERROR_SUCCESS) {
					data.ReleaseBuffer();
					m_strPath = data;
					m_strPath = m_strPath.Right(m_strPath.GetLength() - 8);
					m_strPath.Replace('/', '\\');
				} else {
					count = 1024;
					if (comp.QueryStringValue(_T("Assembly"), data.GetBufferSetLength(count), &count) == ERROR_SUCCESS) {
						data.ReleaseBuffer();
						m_strPath = _T("GAC: ");
						m_strPath += data;
					}
				}
			} else {
				// VS2012 puts quotes around the dll path
				if (m_strPath[0] == L'\"') {
					m_strPath = m_strPath.Right(m_strPath.GetLength() - 1);
				}
				if (m_strPath[m_strPath.GetLength() - 1] == L'\"') {
					m_strPath = m_strPath.Left(m_strPath.GetLength() - 1);
				}
			}
		}
		CopyTo(m_strPath, pVal);

	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_AssociatedComponents(BSTR paradigm, 
	componenttype_enum type, regaccessmode_enum mode, VARIANT *progids)
{
	CHECK_OUT(progids);
	type = (componenttype_enum)(type & COMPONENTTYPE_ALL);

	COMTRY
	{
		CStringArray ret;

		CString paradigmstr = paradigm;

		CStringArray components;
		if (mode & REGACCESS_USER)
			GetComponents_(HKEY_CURRENT_USER, components);
		if (mode & REGACCESS_SYSTEM)
			GetComponents_(HKEY_LOCAL_MACHINE, components);
		for (int i = 0; i < components.GetSize(); i++) {
			componenttype_enum comptype = COMPONENTTYPE_NONE;
			PutInBstr progid(components.GetAt(i));
			CComBSTR desc;
			COMTHROW(QueryComponent(progid, &comptype, &desc, REGACCESS_BOTH));
			if ((comptype & type) && IsAssociated_regaccess(components.GetAt(i), paradigmstr, mode)) {
				ret.Add(components.GetAt(i));
			}
		}
		CopyTo(ret, progids);
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_AssociatedParadigms(BSTR progid, regaccessmode_enum mode, VARIANT *paradigms)
{
	CHECK_OUT(paradigms);

	COMTRY
	{

		CStringArray ret;
		CString progidstr = PutInCString(progid);

		VARIANT all_paradigms_;
		VariantInit(&all_paradigms_);
		COMTHROW(get_Paradigms(REGACCESS_BOTH, &all_paradigms_));
		ATL::CComSafeArray<BSTR> all_paradigms;
		all_paradigms.Attach(all_paradigms_.parray);
		ASSERT(all_paradigms.GetDimensions() == 1);
		for (LONG i = all_paradigms.GetLowerBound(); i <= all_paradigms.GetUpperBound(); i++) {
			BSTR paradigm = all_paradigms.GetAt(i);
			if (IsAssociated_regaccess(progidstr, PutInCString(paradigm), mode)) {
				ret.Add(static_cast<const CString&>(PutInCString(paradigm)));
			}
		}

		CopyTo(ret, paradigms);
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::Associate(BSTR progid, BSTR paradigm, regaccessmode_enum mode)
{
	COLE2CT progid2(progid);
	CString pname(progid2);
	COMTRY
	{
		bool somethingdone = false;
		if(mode & RM_USER) {		
			CRegKey comp;
			LONG res = comp.Open(HKEY_CURRENT_USER, rootreg + _T("\\Components\\") + pname);
			CRegKey assocs;
			if(res == ERROR_FILE_NOT_FOUND) {   // try to create a shadow registry
				CRegKey comp1, mga;
				res = comp1.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Components\\") + pname, KEY_READ);
				if(!res) res = mga.Create(HKEY_CURRENT_USER, rootreg);
				CRegKey comps;
				if(!res) res = ( comps.Create(mga, _T("Components")) );
				if(!res) res = ( comp.Create(comps, pname) );
			}
			if(res == ERROR_SUCCESS) res = assocs.Create(comp, _T("Associated"));
			if(!res) assocs.SetStringValue( PutInCString(paradigm), _T(""));
			if(!res) somethingdone = true;
			if(res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
			ERRTHROW(res);
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey comp;
			LONG res = comp.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Components\\") + pname);
			if(mode & RM_SYS) {
				CRegKey assocs;
				if(!res) res = assocs.Create(comp, _T("Associated"));
				if(!res) res = assocs.SetStringValue( PutInCString(paradigm), _T(""));
			}
			else {
				CRegKey assocs;
				LONG res = assocs.Open(comp, _T("Associated"));
				if(res != ERROR_SUCCESS && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			}
			if(!res) somethingdone = true;
		}
		if(!somethingdone) COMTHROW(E_NOTFOUND);
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::Disassociate(BSTR progid, BSTR paradigm, regaccessmode_enum mode)
{
	COLE2CT progid2(progid);
	CString pname(progid2);
	COMTRY
	{
		if(mode & RM_USER) {		
			CRegKey comp;
			LONG res = comp.Open(HKEY_CURRENT_USER, rootreg + _T("\\Components\\") + pname);
			if (res == ERROR_FILE_NOT_FOUND) {
				CRegKey comp1, mga;
				res = comp1.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Components\\") + pname, KEY_READ);
				if(!res) res = mga.Create(HKEY_CURRENT_USER, rootreg);
				CRegKey comps;
				if(!res) res = ( comps.Create(mga, _T("Components")) );
				if(!res) res = ( comp.Create(comps, pname) );
			}
			CRegKey assocs;
			if(!res) res = assocs.Open(comp, _T("Associated"));
			if (res == ERROR_FILE_NOT_FOUND) 
				res = assocs.Create(comp, _T("Associated"));
			if (!res)
				res = assocs.SetStringValue(PutInCString(paradigm), _T("Disabled"));
			if(res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
			ERRTHROW(res);
		}
		if(mode & (RM_SYS | RM_TEST)) {
			CRegKey comp;
			LONG res = comp.Open(HKEY_LOCAL_MACHINE, rootreg + _T("\\Components\\") + pname);
			CRegKey assocs;
			if(!res) res = assocs.Open(comp, _T("Associated"));
			if(mode & RM_SYS) {
				if(!res) res = assocs.DeleteValue(PutInCString(paradigm));
			}
			if(res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
			ERRTHROW(res);
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::IsAssociated(BSTR progid, BSTR paradigm, 
										VARIANT_BOOL *is_ass, VARIANT_BOOL *can_ass, regaccessmode_enum mode){
	CHECK_IN(progid);
	CHECK_OUT(paradigm);
	CString progidstr = PutInCString(progid);
	CString parc = PutInCString(paradigm);

	COMTRY
	{
		componenttype_enum type;

		COMTHROW(QueryComponent(progid, &type, NULL, REGACCESS_PRIORITY));

		if (is_ass) *is_ass = IsAssociated_regaccess(progidstr, parc, mode) ? VARIANT_TRUE : VARIANT_FALSE;

		if (can_ass) *can_ass = VARIANT_FALSE;
		VARIANT_BOOL can = VARIANT_FALSE;
		CComBSTR pars;
		get_ComponentExtraInfo(mode,progid,CComBSTR(L"Paradigm"), &pars);
		if(!pars && !(type & COMPONENTTYPE_SCRIPT)) {
			CComPtr<IMgaComponent> comp;
			CreateMgaComponent(comp, progid);
			if(!comp) COMTHROW(E_NOTFOUND);
			COMTHROW(comp->get_Paradigm(&pars));
			put_ComponentExtraInfo(REGACCESS_BOTH, progid, CComBSTR(L"Paradigm"), pars);  // just try
		}
		if(!pars) {
			can = type & COMPONENTTYPE_PARADIGM_INDEPENDENT ? VARIANT_TRUE : VARIANT_FALSE;
		}
		else if(!wcscmp(pars, L"*")) {
			can = VARIANT_TRUE;
		}
		else {
			can = VARIANT_FALSE;
			TokenizeParadigmString(PutInCString(pars), [&paradigm, &can](const CString& paradigm2) {
				if (static_cast<const CString &>(PutInCString(paradigm)) == paradigm2)
					can = VARIANT_TRUE;
			});
		}	
		if(can_ass) *can_ass = can;
		
	}
	COMCATCH(;)
}


// --- Actions
typedef HRESULT (STDAPICALLTYPE *CTLREGPROC)();

STDMETHODIMP CMgaRegistrar::RegisterComponentLibrary(BSTR path, regaccessmode_enum mode)
{
	COMTRY
	{
		HMODULE hModule = LoadLibrary(CString(path));
		if( hModule == 0 ) {
			HR_THROW(HRESULT_FROM_WIN32(GetLastError()));
		}

		CTLREGPROC DLLRegisterServer =
			(CTLREGPROC)::GetProcAddress(hModule, "DllRegisterServer");
		
		if( DLLRegisterServer == NULL )
		{
			FreeLibrary(hModule);
		    using namespace MgaDotNetServices;
			_RegistrarPtr reg;
			COMTHROW(reg.CreateInstance(L"MGA.DotNetRegistrar"));
			//COMTHROW(reg.CreateInstance(L"{0BB0C371-6835-4F09-A156-0BD8E3DF8216}", NULL, CLSCTX_INPROC));
			reg->Register(_bstr_t(path));

			return S_OK;
		}
		else
		{
			//c++ dll:
			COMTHROW( DLLRegisterServer() );
			FreeLibrary(hModule);
		}


		CComObjPtr<ITypeLib> typelib;
		COMTHROW( LoadTypeLibEx(path, REGKIND_REGISTER, PutOut(typelib)) );

		UINT index = typelib->GetTypeInfoCount();
		while( index-- > 0 )
		{
			CComObjPtr<ITypeInfo> typeinfo;
			COMTHROW( typelib->GetTypeInfo(index, PutOut(typeinfo)) );

			TYPEATTR *typeattr = NULL;
			COMTHROW( typeinfo->GetTypeAttr(&typeattr) );
			ASSERT( typeattr != NULL );

			try
			{
				if( typeattr->typekind == TKIND_COCLASS )
				{
					CComPtr<IMgaComponent> component;
					// if( SUCCEEDED(typeinfo->CreateInstance(NULL, 
					//	__uuidof(IMgaComponent), (void**)PutOut(component))) )
					if (SUCCEEDED(CreateMgaComponent(component, typeattr->guid)))
					{
						CComBstrObj desc;
						COMTHROW( component->get_ComponentName(PutOut(desc)) );

						componenttype_enum type;
						COMTHROW( component->get_ComponentType(&type) );

						LPOLESTR olestr;
						COMTHROW( ProgIDFromCLSID(typeattr->guid, &olestr) );
						CComBstrObj progid(olestr);
						{
							LPOLESTR pp = olestr + wcslen(olestr) - 2;
							if(wcslen(olestr) > 2 && 
								pp[0] == '.' &&
								isdigit(pp[1]) ) {
								pp[0] = 0;
								CLSID tempGUID;
								HRESULT hr = CLSIDFromProgID(olestr, &tempGUID);
								if(hr == S_OK && tempGUID == typeattr->guid) {
									progid = CComBstrObj(olestr);
								}
							}
						}

						COMTHROW( RegisterComponent(progid, type, desc, mode) );
						CoTaskMemFree(olestr);

						CString paradigms;
						COMTHROW( component->get_Paradigm(PutOut(paradigms)) );

						TokenizeParadigmString(paradigms, [this,&mode,&progid](const CString& par){
							COMTHROW(Associate(progid, PutInBstr(par), mode));
						});
					}
				}

				typeinfo->ReleaseTypeAttr(typeattr);
			}
			catch(hresult_exception &)
			{
				typeinfo->ReleaseTypeAttr(typeattr);
				throw;
			}
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::UnregisterComponentLibrary(BSTR path, regaccessmode_enum mode)
{
	COMTRY
	{
		CComObjPtr<ITypeLib> typelib;
		COMTHROW( LoadTypeLibEx(path, REGKIND_NONE, PutOut(typelib)) );

		UINT index = typelib->GetTypeInfoCount();
		while( index) //WAS: while( --index >= 0 ) // was an infinite loop with UINT
		{
			--index;
			CComObjPtr<ITypeInfo> typeinfo;
			COMTHROW( typelib->GetTypeInfo(index, PutOut(typeinfo)) );// index parameter with the range of 0 to GetTypeInfoCount() 1.

			TYPEATTR *typeattr = NULL;
			COMTHROW( typeinfo->GetTypeAttr(&typeattr) );
			ASSERT( typeattr != NULL );

			try
			{
				if( typeattr->typekind == TKIND_COCLASS )
				{
					LPOLESTR olestr;
					if( SUCCEEDED(ProgIDFromCLSID(typeattr->guid, &olestr)) )
					{
						CComBstrObj progid(olestr);
						COMTHROW( UnregisterComponent(progid, mode) );
					}
				}

				typeinfo->ReleaseTypeAttr(typeattr);
			}
			catch(hresult_exception &)
			{
				typeinfo->ReleaseTypeAttr(typeattr);
				throw;
			}
		}

		// TODO: unregister the type library
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::put_ParadigmExtraInfo(regaccessmode_enum mode, BSTR paradigmName, BSTR paradigmVersionGuid, BSTR name, BSTR newVal)
{
	CString paradigmNameStr = PutInCString(paradigmName);
	CString paradigmGuidStr = PutInCString(paradigmVersionGuid);
	CString nameStr = PutInCString(name);

	COMTRY
	{
		if (mode & RM_SYS) {
			CRegKey para;
			LPCTSTR regpath = rootreg + _T("\\Paradigms\\") + paradigmNameStr + _T("\\") + paradigmGuidStr;
			LONG res = para.Open(HKEY_LOCAL_MACHINE, regpath, KEY_WRITE);
			if (res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if (res == ERROR_SUCCESS) {
				if (!newVal) { para.DeleteValue(nameStr); }
				else { para.SetStringValue(nameStr, PutInCString(newVal)); }
			}
		}
		if (mode & RM_USER) {
			CRegKey para;
			LPCTSTR regpath = rootreg + _T("\\Paradigms\\") + paradigmNameStr + _T("\\") + paradigmGuidStr;
			LONG res = para.Open(HKEY_CURRENT_USER, regpath, KEY_WRITE);
			if (res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if (res == ERROR_SUCCESS) {
				if (!newVal) { para.DeleteValue(nameStr); }
				else { para.SetStringValue(nameStr, PutInCString(newVal)); }
			}
		}
	} COMCATCH(;)
}

STDMETHODIMP CMgaRegistrar::get_ParadigmExtraInfo(regaccessmode_enum mode, BSTR paradigmName, BSTR paradigmVersionGuid, BSTR name, BSTR * pVal)
{
	CHECK_OUT(pVal);

	if ((mode & RM_BOTH) == RM_BOTH) {
		HRESULT res;
		res = this->get_ParadigmExtraInfo(REGACCESS_USER, paradigmName, paradigmVersionGuid, name, pVal);
		CString val = PutInCString(*pVal);
		if (val.IsEmpty())
			res = this->get_ParadigmExtraInfo(REGACCESS_SYSTEM, paradigmName, paradigmVersionGuid, name, pVal);
		return res;
	}

	CString paradigmNameStr = PutInCString(paradigmName);
	CString paradigmGuidStr = PutInCString(paradigmVersionGuid);
	CString nameStr = PutInCString(name);

	COMTRY
	{
		HKEY hive = (mode & RM_SYS ? HKEY_LOCAL_MACHINE : (mode & RM_USER ? HKEY_CURRENT_USER : NULL));
		if (hive != NULL)
		{
			CRegKey para;
			LPCTSTR regpath = rootreg + _T("\\Paradigms\\") + paradigmNameStr + _T("\\") + paradigmGuidStr;
			LONG res = para.Open(hive, regpath, KEY_READ);

			if (res != ERROR_SUCCESS && res != ERROR_ACCESS_DENIED && res != ERROR_FILE_NOT_FOUND) ERRTHROW(res);
			if (res == ERROR_SUCCESS) {
				ULONG count = 0;
				if (para.QueryStringValue(nameStr, NULL, &count) == ERROR_SUCCESS) {
					CString retVal;
					if (para.QueryStringValue(nameStr, retVal.GetBufferSetLength(count), &count) == ERROR_SUCCESS) {
						retVal.ReleaseBuffer();
						CopyTo(retVal, pVal);
						return S_OK;
					}
				}
			}
		}
	return E_NOTFOUND;
	}
	COMCATCH(;)
}

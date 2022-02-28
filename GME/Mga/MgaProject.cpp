// MgaProject.cpp : Implementation of CMgaProject
#include "stdafx.h"
#include "MgaUtil.h"
#include "CommonComponent.h"
#include "MgaFCO.h"
#include "MgaFilter.h"
#include "stdio.h"
#include "MgaComplexOps.h"
#include "MgaLibOps.h"

void LockingStart(CComPtr<ICoreProject> &tempproject)	{ 
		   CComPtr<ICoreTerritory> lm;
		   COMTHROW(tempproject->CreateTerritory( &lm));
		   COMTHROW(tempproject->PushTerritory( lm));
}

/////////////////////////////////////////////////////////////////////////////
// CMgaProject
/////////////////////////////////////////////////////////////////////////////

CMgaProject::CMgaProject()	{ 
			INITSIG('P');
#ifdef DEBUG
			MGA_TRACE("Constructed: %s - %08X\n", sig, this);
#endif
			opened_flag = OPENED_FLAG_CLOSED;
			dataproject = NULL;
			preferences = 0;
			opmask = 0x0026662A;
			mgaproject = this;
			checkoff = false;
			transactioncount = 0;
			autoaddons = false;
			inautoaddoncreate = false;
			mgaversion = 0;
			aurcnt = 0;
			guidstat = CLEAN;
}

STDMETHODIMP CMgaProject::FinalConstruct()	{
		COMTRY {
//			IDispatchImpl<IMgaProject, &IID_IMgaProject, &LIBID_MGALib> *xthis = this;
			COMTHROW( CoCreateInstance(__uuidof(CoreProject), (IMgaProject *)this, CLSCTX_ALL, IID_IUnknown, (LPVOID *) &inner) );
#ifndef _ATL_DEBUG_INTERFACES
			COMTHROW(inner.QueryInterface(&dataproject));
			dataproject->Release();   // release reference to ourselves
#else
			dataprojectNull = false;
#endif
		} COMCATCH(;);
}

CMgaProject::~CMgaProject()	{
#ifdef DEBUG
		MGA_TRACE("Destructed: %s - %08X\n", sig, this);
#endif
		MARKSIG('9'); 
		ASSERT(allterrs.empty());
		if(opened_flag != OPENED_FLAG_CLOSED) {
			Close();
		}
}

/////////////////////////////////////////////////////////////////////////////
///////////////////////// META CACHE FACILITY ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CMgaProject::get_MetaObj(metaref_type id, IMgaMetaBase **pVal) {
	COMTRY {
		COMTHROW(metapr != 0 ? S_OK : E_MGA_PROJECT_NOT_OPEN);
		SetNmspaceInMeta();
		COMTHROW(metapr->get_FindObject(id, pVal));
	}
	COMCATCH(;);	
}

// THROWS
CComPtr<IMgaMetaBase> CMgaProject::FindMetaRef(metaref_type l) {
		CComPtr<IMgaMetaBase> hh;
		COMTHROW(metapr != 0 ? S_OK : E_MGA_PROJECT_NOT_OPEN);
		SetNmspaceInMeta();
		COMTHROW(metapr->get_FindObject(l, &hh));
//		HRESULT hr = metapr->get_FindObject(l, &hh);
//		if(hr != S_OK && hr != E_NOTFOUND) COMTHROW(hr);
		return hh;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////    CREATE/OPEN PROJECT   //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


HRESULT CMgaProject::CreateSetupData(BSTR rootname, BSTR paradigmname, VARIANT guid) {
		COMTRY_IN_TRANSACTION {	

// check if meta is valid
			CoreObj  dataroot, rootfolder;
			CComBSTR parversion;

// set values in 			
			COMTHROW(dataproject->get_RootObject(&dataroot.ComPtr()));
			dataroot[ATTRID_CDATE] = dataroot[ATTRID_MDATE]  = Now();
			dataroot[ATTRID_PARADIGM] = paradigmname;
			dataroot[ATTRID_MGAVERSION] = mgaversion;
			COMTHROW(metapr->get_Version(&parversion));
			dataroot[ATTRID_PARVERSION] = parversion;
			dataroot[ATTRID_NAME]    = rootname;
			if( guid.vt != (VT_UI1 | VT_ARRAY) || GetArrayLength(guid) != sizeof(::GUID) )
					COMTHROW(E_INVALIDARG);
			dataroot[ATTRID_PARGUID] = guid;
			dataroot[ATTRID_VERSION] = CComBSTR(""); // default version string

// create data root things
		    COMTHROW(dataproject->CreateObject(DTID_FOLDER,&rootfolder.ComPtr()));
			assignGuid( this, rootfolder); // assign a guid to the root folder
			rootfolder[ATTRID_NAME]    = rootname;
			rootfolder[ATTRID_FPARENT] = dataroot;
			if(!(preferences & MGAPREF_MANUAL_RELIDS))  rootfolder[ATTRID_RELID] = 1;
			CComPtr<IMgaMetaFolder> mf;
			metaref_type mr;
			COMTHROW(metapr->get_RootFolder(&mf));
			COMTHROW(mf->get_MetaRef(&mr));
			rootfolder[ATTRID_META] = mr;
		} COMCATCH_IN_TRANSACTION(;);

}


static int guidcmp(VARIANT &qGUID, VARIANT &pGUID)	{
	::GUID g1, g2;
	CopyTo(qGUID, g1);
	CopyTo(pGUID, g2);
	return memcmp(&g1, &g2, sizeof(g1));
}


inline HRESULT EMAP(HRESULT hr, HRESULT from, HRESULT to) { HRESULT h = hr; if(h == from) h = to; return h; }
inline HRESULT EDEF(HRESULT hr, HRESULT to) { HRESULT h = hr; if(h != S_OK) h = to; return h; }


void CMgaProject::OpenParadigm(BSTR s, VARIANT *pGUID) {
	CComPtr<IMgaRegistrar> mgareg;
	COMTHROW(mgareg.CoCreateInstance(OLESTR("Mga.MgaRegistrar")));
	_bstr_t connstr;
	{
		HRESULT hr = mgareg->QueryParadigm(s, connstr.GetAddress(), pGUID, REGACCESS_PRIORITY);
		if (FAILED(hr))
		{
			CComPtr<IErrorInfo> info;
			GetErrorInfo(0, &info);
			throw _com_error(E_MGA_PARADIGM_NOTREG, info, true);
		}
	}
	ASSERT(connstr);
	COMTHROW(metapr.CoCreateInstance(OLESTR("Mga.MgaMetaProject")));
	// #ifdef _ATL_DEBUG_INTERFACES
	// COMTHROW(metapr.CoCreateInstance(OLESTR("Mga.MgaMetaProject"), NULL, CLSCTX_LOCAL_SERVER));
	HRESULT hr = metapr->Open(connstr);
	if (FAILED(hr))
	{
		_bstr_t err;
		if (GetErrorInfo(err.GetAddress()))
			throw_com_error(E_MGA_PARADIGM_INVALID, static_cast<const wchar_t*>(err)); // change HRESULT
		COMTHROW(E_MGA_PARADIGM_INVALID);
	}
	CComVariant metaGUID;
	COMTHROW(metapr->get_GUID(&metaGUID));
	if (guidcmp(metaGUID, *pGUID)) 
		throw_com_error(E_MGA_PARADIGM_INVALID, L".mta file paradigm GUID does not match registered GUID");
	parconn = connstr.GetBSTR();
}


STDMETHODIMP CMgaProject::OpenParadigm(BSTR s, BSTR ver) {
	COMTRY {
		CComVariant vguid;
		{
			CComPtr<IMgaRegistrar> mgareg;
			COMTHROW(mgareg.CoCreateInstance(OLESTR("Mga.MgaRegistrar")));
			{
				HRESULT hr = mgareg->GUIDFromVersion(s, ver, &vguid, REGACCESS_PRIORITY);
				if (FAILED(hr))
				{
					CComPtr<IErrorInfo> info;
					GetErrorInfo(0, &info);
					throw _com_error(E_MGA_PARADIGM_NOTREG, info, true);
				}
			}
		}
		OpenParadigm(s,&vguid);
	} COMCATCH(;);
}

STDMETHODIMP CMgaProject::CreateEx(BSTR projectname, BSTR paradigmname, VARIANT paradigmGUID) {
	COMTRY {
		if(opened_flag != OPENED_FLAG_CLOSED) COMTHROW(E_MGA_PROJECT_OPEN);

		mgaversion = 2;
			// Set generic for meta
		CComPtr<ICoreMetaProject> genericproject;
		CreateCoreMetaProject(genericproject, mgaversion > 1); // create version 2 from now on

		CComVariant connGUID;
		if(paradigmGUID.vt != VT_EMPTY) connGUID = paradigmGUID;
		OpenParadigm(paradigmname, &connGUID);
	
		int undosize = getMaxUndoSize();
		COMTHROW(dataproject->CreateProject2(projectname, undosize, genericproject));
		opened_flag = OPENED_FLAG_UNCHANGED;
		guidstat = DIRTY;

	    CComPtr<IMgaTerritory> lm;
	    COMTHROW(CreateTerritory(NULL, &lm));
		COMTHROW(BeginTransaction(lm, TRANSACTION_GENERAL));
		
		HRESULT suhr = S_OK;
		try {
			CComBSTR   fname("RootFolder");
            // BGY: commented out because xml backend has different connection string format
			/*LPCOLESTR p = wcsrchr(projectname, '\\');
			if(p) {
				p++;
				LPCOLESTR p2 = wcschr(p, '.');
				if(p2 == NULL) p2 = p + wcslen(p);
				fname = CComBSTR(p2-p, p);
			}*/
			suhr = CreateSetupData(fname, paradigmname, connGUID);
			COMTHROW(lm->Flush());
			COMTHROW(CommitTransaction());
		} catch(hresult_exception &e) {
			lm->Flush();
			AbortTransaction();
			throw e;
		}
		COMTHROW(dataproject->FlushUndoQueue());
		COMTHROW(suhr);
		projconn = projectname;
		StartAutoAddOns();
		try {
			COMTHROW(BeginTransaction(lm, TRANSACTION_READ_ONLY));
			GlobalNotify(GLOBALEVENT_OPEN_PROJECT);
			COMTHROW(lm->Flush());
			COMTHROW(CommitTransaction());
		} catch(hresult_exception &e) {
			lm->Flush();
			AbortTransaction();
			throw e;
		}
		MARKSIG('2');
	}
	COMCATCH(
		opened_flag = OPENED_FLAG_CLOSED;
		guidstat = CLEAN;
		if (dataproject) {
			dataproject->CloseProject(VARIANT_TRUE);
			dataproject->DeleteProject(projectname);
		}
		if (metapr)
			metapr->Close();
		metapr = 0;
		projconn.Empty(); 
		parconn.Empty();
		// We've already called SetErrorInfo, don't call it again
		if (e.hr == E_MGA_COMPONENT_ERROR) {
			return e.hr;
		}
	)
}


// if paradigname != "": 
//		change paradigmname if different
//		if paradigmguid == 0 update to the current version (even if no name change!);
// if paradigmguid != null and != oldguid update guid (use new paradigmname to locate paradigm in any case)
// if no guid change skip check
STDMETHODIMP CMgaProject::OpenEx(BSTR projectname, BSTR paradigmname, VARIANT paradigmGUID) {
	COMTRY {
		if(opened_flag != OPENED_FLAG_CLOSED) COMTHROW(E_MGA_PROJECT_OPEN);
		CComBSTR s;
		CComVariant pGUID;
		CComBSTR ver;
		CComPtr<ICoreMetaProject> genericproject;
		CreateCoreMetaProject(genericproject, true); // will upgrade if old version
		VARIANT_BOOL ro;

		int undosize = getMaxUndoSize();
		COMTHROW(dataproject->OpenProject2(projectname, undosize, genericproject, &ro));
		
		projconn = projectname;

		opened_flag = OPENED_FLAG_UNCHANGED;
		guidstat = CLEAN;
	    CComPtr<IMgaTerritory> lm;
	    COMTHROW(CreateTerritory(NULL, &lm));
		COMTHROW(BeginTransaction(lm, TRANSACTION_READ_ONLY));

		try {	

			CoreObj  dataroot;
			COMTHROW(dataproject->get_RootObject(&dataroot.ComPtr()));
		    s=dataroot[ATTRID_PARADIGM];
			mgaversion = dataroot[ATTRID_MGAVERSION];
			if (mgaversion <= 1L) // Core layer changed the project by adding ATTRID_GUID1..4 for CCoreBinFile
				opened_flag = OPENED_FLAG_CHANGED;

			pGUID=CComVariant(dataroot[ATTRID_PARGUID]);
			ver = dataroot[ATTRID_PARVERSION];
			COMTHROW(lm->Flush());
			COMTHROW(CommitTransaction());
		} catch(hresult_exception &e) {
			lm->Flush();
			AbortTransaction();
			throw e;
		}

		CComBSTR soldname = s;
		CComVariant soldguid = pGUID;

		if(SysStringLen(paradigmname) != 0) {
			ver.Empty();
			s = paradigmname;
		}
		if (paradigmGUID.vt != VT_EMPTY) {
			ver.Empty();
			if (CComVariant(true) == paradigmGUID)
				pGUID = NULLVARIANT; // use current version
			else
				pGUID = paradigmGUID;
		}
		if (s.Length()) {
			if (ver.Length()) {
				// Version string has precedence
				COMTHROW(OpenParadigm(s,ver));
				pGUID.Clear();
				COMTHROW(metapr->get_GUID(&pGUID));
			}
			else {
				OpenParadigm(s, &pGUID);
				ver.Empty();
				COMTHROW(metapr->get_Version(&ver));
			}
			bool guidchanged = false;
			if(guidcmp(soldguid, pGUID)) {
				COMTHROW(BeginTransaction(lm, TRANSACTION_READ_ONLY));
				try {
					CComPtr<IMgaFolder> rf;
					COMTHROW(get_RootFolder(&rf));
					HRESULT hr = ObjFor(rf)->CheckTree();
					if (FAILED(hr))
					{
						throw_last_com_error(hr);
					}
					rf = 0;
					COMTHROW(CommitTransaction());
				} catch(hresult_exception &e) {
					lm->Flush();
					AbortTransaction();
					throw e;
				} catch(_com_error&) {
					lm->Flush();
					AbortTransaction();
					throw;
				}

				guidchanged = true;
			}
			if(s != soldname || guidchanged) {
				if(ro) COMTHROW(E_MGA_READ_ONLY_ACCESS);
				COMTHROW(BeginTransaction(lm, TRANSACTION_GENERAL));
				try {
					CoreObj  dataroot;
					COMTHROW(dataproject->get_RootObject(&dataroot.ComPtr()));
					dataroot[ATTRID_PARADIGM] = s;
					dataroot[ATTRID_PARGUID] = pGUID;
					dataroot[ATTRID_PARVERSION] = ver;
					COMTHROW(CommitTransaction());
					opened_flag = OPENED_FLAG_CHANGED;
					guidstat = DIRTY;
				} catch(hresult_exception &e) {
					lm->Flush();
					AbortTransaction();
					throw e;
				}
			}
		}
		else COMTHROW(E_INVALID_USAGE);
		StartAutoAddOns();
		try {
			COMTHROW(BeginTransaction(lm, TRANSACTION_READ_ONLY));
			GlobalNotify(GLOBALEVENT_OPEN_PROJECT);
			COMTHROW(lm->Flush());
			COMTHROW(CommitTransaction());
		} catch(hresult_exception &e) {
			lm->Flush();
			AbortTransaction();
			throw e;
		}
		MARKSIG('2');
	} 
	COMCATCH(
		opened_flag = OPENED_FLAG_CLOSED;
		guidstat = CLEAN;
		if (dataproject)
			dataproject->CloseProject(VARIANT_TRUE);
		if (metapr)
			metapr->Close();
		metapr = 0;
		projconn.Empty(); 
		parconn.Empty();
		// We've already called SetErrorInfo, don't call it again
		if (e.hr == E_MGA_COMPONENT_ERROR) {
			return e.hr;
		}
	)
}

STDMETHODIMP CMgaProject::Open(BSTR projectname, VARIANT_BOOL *ro_mode)
{
	COMTRY {
		if (opened_flag != OPENED_FLAG_CLOSED) COMTHROW(E_MGA_PROJECT_OPEN);
		CComBSTR s;
		CComVariant pGUID;
		CComBSTR ver;
		CComPtr<ICoreMetaProject> genericproject;
		CreateCoreMetaProject(genericproject, true); // will upgrade if old version

		int undosize = getMaxUndoSize();
		COMTHROW(dataproject->OpenProject2(projectname, undosize, genericproject, ro_mode));
		
		projconn = projectname;

		opened_flag = OPENED_FLAG_UNCHANGED;
		guidstat = CLEAN;
	    CComPtr<IMgaTerritory> lm;
	    COMTHROW(CreateTerritory(NULL, &lm));
		COMTHROW(BeginTransaction(lm, TRANSACTION_READ_ONLY));

		try {	

			CoreObj  dataroot;
			COMTHROW(dataproject->get_RootObject(&dataroot.ComPtr()));
		    s=dataroot[ATTRID_PARADIGM];
			mgaversion = dataroot[ATTRID_MGAVERSION];
			if( mgaversion <= 1L) // Core layer changed the project by adding ATTRID_GUID1..4 for CCoreBinFile
				opened_flag = OPENED_FLAG_CHANGED;

			pGUID=CComVariant(dataroot[ATTRID_PARGUID]);
			ver=dataroot[ATTRID_PARVERSION];
			COMTHROW(lm->Flush());
			COMTHROW(CommitTransaction());
		} catch(hresult_exception &e) {
			lm->Flush();
			AbortTransaction();
			throw e;
		}

		if(s.Length()) {
			HRESULT hr = S_OK;
			if (ver.Length()) {
				// Version string has precedence
				hr = OpenParadigm(s,ver);
			}
			else {
				OpenParadigm(s, &pGUID);
			}
			if (FAILED(hr))
			{
				CComPtr<IErrorInfo> info;
				GetErrorInfo(0, &info);
				throw _com_error(hr, info, true);
			}
		}
		else COMTHROW(E_MGA_MODULE_INCOMPATIBILITY);

		CComVariant nGUID;
		COMTHROW(metapr->get_GUID(&nGUID));

		if(guidcmp(pGUID, nGUID)) {
			COMTHROW(BeginTransaction(lm, TRANSACTION_READ_ONLY));
			try {
				CComPtr<IMgaFolder> rf;
				COMTHROW(get_RootFolder(&rf));
				COMTHROW(ObjFor(rf)->CheckTree());
				rf = 0;
				COMTHROW(CommitTransaction());
			} catch(hresult_exception &e) {
				lm->Flush();
				AbortTransaction();
				throw e;
			}

			if (*ro_mode == VARIANT_FALSE) {
				COMTHROW(BeginTransaction(lm, TRANSACTION_GENERAL));
				try {
					CoreObj  dataroot;
					COMTHROW(dataproject->get_RootObject(&dataroot.ComPtr()));
					dataroot[ATTRID_PARGUID] = nGUID;
					COMTHROW(CommitTransaction());
					opened_flag = OPENED_FLAG_CHANGED;
					guidstat = DIRTY;
				} catch(hresult_exception &e) {
					lm->Flush();
					AbortTransaction();
					throw e;
				}
			}
		}

		StartAutoAddOns();

		try {
			COMTHROW(BeginTransaction(lm, TRANSACTION_READ_ONLY));
			GlobalNotify(GLOBALEVENT_OPEN_PROJECT);
			COMTHROW(CommitTransaction());
		} catch(hresult_exception &e) {
			lm->Flush();
			AbortTransaction();
			throw e;
		}

		MARKSIG('2');
	} 
	COMCATCH(
		opened_flag = OPENED_FLAG_CLOSED;
		if (dataproject)
			dataproject->CloseProject(VARIANT_TRUE);
		if (metapr)
			metapr->Close();
		metapr = 0;
		projconn.Empty(); 
		parconn.Empty();
		// We've already called SetErrorInfo, don't call it again
		if (e.hr == E_MGA_COMPONENT_ERROR) {
			return e.hr;
		}
	)
}

STDMETHODIMP CMgaProject::QueryProjectInfo(BSTR projectname, long *mgaversion, 
										   BSTR *paradigmname, 
										   BSTR *parversion,
										   VARIANT *paradigmGUID,
										   VARIANT_BOOL *ro_mode)
{
	CComPtr<ICoreProject> dp;
	COMTRY {
		CHECK_INSTRPAR(projectname);
		CHECK_OUTPAR(mgaversion);
		CHECK_OUTSTRPAR(paradigmname);
		CHECK_OUTSTRPAR(parversion);
		CHECK_OUTPAR(paradigmGUID);
		CHECK_OUTPAR(ro_mode);

		CComPtr<ICoreMetaProject> genericproject;
		CreateCoreMetaProject(genericproject); // use mgaversion = 1 project model 

		COMTHROW(dp.CoCreateInstance(__uuidof(CoreProject)));
		COMTHROW(dp->OpenProject(projectname, genericproject, ro_mode));
		
		CComPtr<ICoreTerritory> tt;
		COMTHROW(dp->CreateTerritory(&tt));
		COMTHROW(dp->BeginTransaction(TRANSTYPE_READFIRST));

		try {	
			COMTHROW(dp->PushTerritory(tt));

			CoreObj  dataroot;
			COMTHROW(dp->get_RootObject(&dataroot.ComPtr()));
		    *paradigmname = CComBSTR(dataroot[ATTRID_PARADIGM]).Detach();
		    *parversion = CComBSTR(dataroot[ATTRID_PARVERSION]).Detach();
		    CComVariant(dataroot[ATTRID_PARGUID]).Detach(paradigmGUID);
			*mgaversion = dataroot[ATTRID_MGAVERSION];
			COMTHROW(dp->PopTerritory());
		} catch(hresult_exception &) {
			// FIXME
			;
		}
		dp->AbortTransaction(TRANSTYPE_ANY);
		dp->CloseProject(VARIANT_FALSE);
	}
	COMCATCH(
		if(dp) dp->CloseProject(VARIANT_FALSE);
	)
}

STDMETHODIMP CMgaProject::Save(BSTR newname, VARIANT_BOOL keepoldname)
{
	COMTRY {
		if(baseterr)
			COMTHROW(E_MGA_ALREADY_IN_TRANSACTION);
		{
			CComPtr<IMgaTerritory> t;
			COMTHROW(CreateTerritory(NULL, &t));
			// if mga_ver<=1 the Core layer changed the project by adding ATTRID_GUID1..4
			// (mgaversion <= 1L) -> (opened >= CHANGED)
			ASSERT( !(mgaversion <= 1L) || opened_flag >= OPENED_FLAG_CHANGED);

			if (opened_flag >= OPENED_FLAG_CHANGED) {
				COMTHROW(BeginTransaction(t, TRANSACTION_GENERAL));
				try {
					CoreObj self;
					COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
					UpdateMGAVersion( self);
					FixupGUID();
					GlobalNotify(GLOBALEVENT_SAVE_PROJECT);
					COMTHROW(CommitTransaction());
				} catch(hresult_exception &) {
					t->Flush();
					AbortTransaction();
				}
			}
		}
		HRESULT hr = dataproject->SaveProject(newname, keepoldname);
		if (FAILED(hr))
			return hr;
		if(CComBSTR(newname).Length()) {
			if (keepoldname == VARIANT_FALSE) {
				projconn = newname;
				opened_flag = OPENED_FLAG_UNCHANGED;
				transactioncount = 0;
			}

		} else {
			opened_flag = OPENED_FLAG_UNCHANGED;
			transactioncount = 0;
		}
	}
	COMCATCH(;);
}

STDMETHODIMP CMgaProject::Close(VARIANT_BOOL abort)
{
	if (opened_flag == OPENED_FLAG_CLOSED) {
		ASSERT(("Project is closed but transaction is active", !baseterr));
		return S_OK;
	}
	
	COMTRY {
		if(baseterr) COMTHROW(AbortTransaction());
		{
			CComPtr<IMgaTerritory> t;
			COMTHROW(CreateTerritory(NULL, &t));
			bool write = !abort && opened_flag >= OPENED_FLAG_CHANGED;
			COMTHROW(BeginTransaction(t, write ? TRANSACTION_GENERAL : TRANSACTION_READ_ONLY));
			try {
				if(write) {
					CoreObj self;
					COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
					FixupGUID();
				}
				GlobalNotify(GLOBALEVENT_CLOSE_PROJECT);
				COMTHROW(CommitTransaction());
			} catch(hresult_exception &) {
				t->Flush();
				AbortTransaction();
			}
		}
		
		StopAutoAddOns();  // PETER: Moved these two lines here, otherwise addons won't receive notifications
		RemoveClients();   //

		if(dataproject) COMTHROW(dataproject->CloseProject(abort));

		if(metapr) {
			COMTHROW(metapr->Close());
			metapr = 0;
		}

	    MARKSIG('8');
		opened_flag = OPENED_FLAG_CLOSED;
		guidstat = CLEAN;
		projconn.Empty(); 
		parconn.Empty();
		transactioncount = 0;
	}
	COMCATCH( opened_flag = OPENED_FLAG_CLOSEERROR;);	//  You cannot rollback a failed Close completely, so I did not even try it.
}

#undef mgaproject


// ----------------------------------------
// Access the root nodes of data and meta
// ----------------------------------------
STDMETHODIMP CMgaProject::get_RootFolder(IMgaFolder **pVal)
{
	COMTRY {
		CHECK_OUTPTRPAR(pVal);
		CoreObj  dataroot;
		COMTHROW(dataproject->get_RootObject(&dataroot.ComPtr()));
		CoreObjs s = dataroot[ATTRID_FPARENT + ATTRID_COLLECTION];
		ITERATE_THROUGH(s) {
			CComPtr<ICoreMetaObject> m;
			metaid_type t;
			COMTHROW(ITER->get_MetaObject(&m));
			COMTHROW(m->get_MetaID(&t));
			if(t == DTID_FOLDER) {
				ObjForCore(ITER)->getinterface(pVal);
				break;
			}
		}
		if(*pVal == NULL) COMTHROW(E_MGA_MODULE_INCOMPATIBILITY);
    }
    COMCATCH(;);
}


STDMETHODIMP CMgaProject::get_RootMeta(IMgaMetaProject **pVal)
{
	COMTRY {
		if(metapr == 0) COMTHROW( E_MGA_PROJECT_NOT_OPEN );
		SetNmspaceInMeta();
		*pVal = metapr; (*pVal)->AddRef();
	} COMCATCH(;);
}

// ----------------------------------------
// Filter and search functions
// ----------------------------------------
HRESULT CMgaProject::CreateFilter(IMgaFilter **newfilter) {
	COMTRY {
		CHECK_OUTPTRPAR(newfilter);
		CComPtr<CMgaFilter> filter;
		CreateComObject(filter);
		filter->setproject(this);
		*newfilter = filter.Detach();
	} COMCATCH(;);
}

void recursefolders(CoreObj folder, CMgaFilter * filter, EXCOLLECTIONTYPE_FOR(MgaFCO) *q) {
	{
		ITERATE_THROUGH(folder[ATTRID_FCOPARENT+ATTRID_COLLECTION]) {
			if(ITER.IsFCO()) filter->searchFCOs(ITER, q);
			else recursefolders(ITER, filter, q);
		}
	}
}

STDMETHODIMP CMgaProject::AllFCOs(IMgaFilter *filter, IMgaFCOs ** fcos) {
	COMTRY {
		CHECK_OUTPTRPAR(fcos);
		CHECK_INPTRPAR(filter);   // project is checked in the following lines
		CComPtr<IMgaProject> p;
		COMTHROW(filter->get_Project(&p));
		if(!COM_EQUAL(p, (IMgaProject *)this)) COMTHROW(E_MGA_FOREIGN_PROJECT);

		CREATEEXCOLLECTION_FOR(MgaFCO, q);
		CComPtr<IMgaFolder> rootf;
		COMTHROW(get_RootFolder(&rootf));
		recursefolders(CoreObj(rootf), reinterpret_cast<CMgaFilter *>(filter),  q);
		*fcos = q.Detach();
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::GetFCOByID(BSTR id, IMgaFCO **pVal) {
	COMTRY {
		CComPtr<IMgaObject> p;
		COMTHROW(GetObjectByID(id,&p));
		COMTHROW(p.QueryInterface(pVal));
    } COMCATCH(;);
}

STDMETHODIMP CMgaProject::GetObjectByID(BSTR id, IMgaObject **pVal)
{

	COMTRY {
		CHECK_INSTRPAR(id);
		CHECK_OUTPTRPAR(pVal);
		metaid_type mm; // short
		objid_type ss;  // long
		if (SysStringLen(id) == 0)
		{
			COMTHROW(E_MGA_BAD_ID);
		}
		if( swscanf(id,OLESTR("id-%04hx-%08lx"), &mm, &ss) != 2 || 
			mm < DTID_MODEL || mm > DTID_FOLDER) COMTHROW(E_MGA_BAD_ID);
		CoreObj obj;
		COMTHROW(dataproject->get_Object(mm,ss,&obj.ComPtr()));
		if (obj)
		{
			IMgaObject *ret;
			ObjForCore(obj)->getinterface(&ret);
			if (ret == NULL)
			{
				// this should never happen
				ASSERT(false);
				obj->Delete();
				COMTHROW(E_NOTFOUND);
			}
			// get_Object will create an object if the ID does not exist
			// need to read from core storage to test if it exists
			_bstr_t name;
			HRESULT hr = ret->get_Name(name.GetAddress());
			if (SUCCEEDED(hr)) {
				*pVal = ret;
				return S_OK;
			}
			else
			{
				obj->Delete();
				ret->Release();
				COMTHROW(hr);
			}
		}
		else
			COMTHROW(E_MGA_BAD_ID);
    } COMCATCH(;);
}

STDMETHODIMP CMgaProject::GetFCOsByName(BSTR name, IMgaFCOs **pVal)
{

	COMTRY {
// ADD CODE
//				(*pVal)->Open()
		COMTHROW(E_MGA_NOT_IMPLEMENTED);
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::GetFolderByPath(BSTR path, IMgaFolder **pVal)
{

	COMTRY {
// ADD CODE
//				(*pVal)->Open()
		COMTHROW(E_MGA_NOT_IMPLEMENTED);
    }
    COMCATCH(;);
}

// see FCO::get_ObjectByPath for details in MgaFolder.cpp 
STDMETHODIMP CMgaProject::get_ObjectByPath(BSTR path, IMgaObject **pVal) {
	COMTRY {
		CHECK_INSTRPAR(path);
		CHECK_OUTPTRPAR(pVal);
		CComPtr<IMgaFolder> f;
		COMTHROW(get_RootFolder(&f));
		COMTHROW(f->get_ObjectByPath(path, pVal));
	} COMCATCH(;);
}

STDMETHODIMP CMgaProject::get_NthObjectByPath(long n_th, BSTR path, IMgaObject **pVal) {
	COMTRY {
		CHECK_INSTRPAR(path);
		CHECK_OUTPTRPAR(pVal);
		CComPtr<IMgaFolder> f;
		COMTHROW(get_RootFolder(&f));
		COMTHROW(f->get_NthObjectByPath(n_th, path, pVal));
	} COMCATCH(;);
}

// ----------------------------------------
// Access project properties
// ----------------------------------------
STDMETHODIMP CMgaProject::GetStatistics(BSTR *statstr)
{

	COMTRY {
// ADD CODE
    }
    COMCATCH(;);
}



STDMETHODIMP CMgaProject::get_CreateTime(BSTR *pVal)
{

	COMTRY {
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		*pVal = CComBSTR(self[ATTRID_CDATE]).Detach();
	}
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::get_ChangeTime(BSTR *pVal)
{

	COMTRY {
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		*pVal = CComBSTR(self[ATTRID_MDATE]).Detach();
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::get_Author(BSTR *pVal)
{

	COMTRY {
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		*pVal = CComBSTR(self[ATTRID_CREATOR]).Detach();
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::put_Author(BSTR newVal)
{

	COMTRY_IN_TRANSACTION {
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		self[ATTRID_CREATOR] = newVal;
		COMTHROW(GlobalNotify(GLOBALEVENT_PROJECT_PROPERTIES));
    }
    COMCATCH_IN_TRANSACTION(;);
}

STDMETHODIMP CMgaProject::get_MetaGUID(VARIANT *pVal)
{

	COMTRY
	{
		CHECK_OUTPAR(pVal);
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		CComVariant p = self[ATTRID_PARGUID];

		if( p.vt != (VT_UI1 | VT_ARRAY) || GetArrayLength(p) != sizeof(::GUID) )
		{
			::GUID guid;
			memset(&guid, 0, sizeof(::GUID));

			CopyTo(guid, p);
		}
		p.Detach(pVal);
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaProject::get_MetaName(BSTR *pVal)
{

	COMTRY
	{
		CHECK_OUTSTRPAR(pVal);
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		CComBSTR p = self[ATTRID_PARADIGM];

		*pVal=p.Detach();
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaProject::get_MetaVersion(BSTR *pVal)
{

	COMTRY
	{
		CHECK_OUTSTRPAR(pVal);
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		CComBSTR p = self[ATTRID_PARVERSION];

		*pVal=p.Detach();
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaProject::get_GUID(VARIANT *pVal)
{

	COMTRY
	{
		CHECK_OUTPAR(pVal);
		
		FixupGUID(false);

		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		CComVariant p = self[ATTRID_GUID];

		
		if (guidstat == PENDING) {
			p = pendingguid;
		}
		
		if( p.vt != (VT_UI1 | VT_ARRAY) || GetArrayLength(p) != sizeof(::GUID) )
		{
			::GUID guid;
			memset(&guid, 0, sizeof(::GUID));

			CopyTo(guid, p);
		}
		p.Detach(pVal);
	}
	COMCATCH(;)
}


STDMETHODIMP CMgaProject::put_GUID(VARIANT newVal)
{
	COMTRY_IN_TRANSACTION {
		CHECK_INPAR(newVal);
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));

		if( newVal.vt != (VT_UI1 | VT_ARRAY) || GetArrayLength(newVal) != sizeof(::GUID) )
			COMTHROW(E_INVALIDARG);

		self[ATTRID_GUID] = newVal;
		guidstat = MANUAL;
		COMTHROW(GlobalNotify(GLOBALEVENT_PROJECT_PROPERTIES));
    }
    COMCATCH_IN_TRANSACTION(;);
}


STDMETHODIMP CMgaProject::get_Comment(BSTR *pVal)
{

	COMTRY {
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		*pVal = CComBSTR(self[ATTRID_EXTDATA]).Detach();
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::put_Comment(BSTR newVal)
{

	COMTRY_IN_TRANSACTION {
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		self[ATTRID_EXTDATA] = newVal;
		COMTHROW(GlobalNotify(GLOBALEVENT_PROJECT_PROPERTIES));
    }
    COMCATCH_IN_TRANSACTION(;);
}

STDMETHODIMP CMgaProject::put_Name(BSTR newVal)
{

	COMTRY_IN_TRANSACTION {
		CHECK_INSTRPAR(newVal);
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		self[ATTRID_NAME] = newVal;
		COMTHROW(GlobalNotify(GLOBALEVENT_PROJECT_PROPERTIES));
    }
    COMCATCH_IN_TRANSACTION(;);
}

STDMETHODIMP CMgaProject::put_Version(BSTR newVal)
{

	COMTRY_IN_TRANSACTION {
		CHECK_INSTRPAR(newVal);
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		self[ATTRID_VERSION] = newVal;
		COMTHROW(GlobalNotify(GLOBALEVENT_PROJECT_PROPERTIES));
    }
    COMCATCH_IN_TRANSACTION(;);
}


STDMETHODIMP CMgaProject::get_Name(BSTR *pVal)
{

	COMTRY {
		CHECK_OUTSTRPAR(pVal);
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		CComBSTR s = self[ATTRID_NAME];
		*pVal = s.Detach();
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::get_Version(BSTR *pVal)
{

	COMTRY {
		CHECK_OUTSTRPAR(pVal);
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		CComBSTR s = self[ATTRID_VERSION];
		*pVal = s.Detach();
    }
    COMCATCH(;);
}


STDMETHODIMP CMgaProject::get_ProjectConnStr(BSTR *pVal)
{
	COMTRY {
		CHECK_OUTSTRPAR(pVal);
		CComBSTR s = projconn;
		*pVal = s.Detach();
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::get_ParadigmConnStr(BSTR *pVal)
{
	COMTRY {
		CHECK_OUTSTRPAR(pVal);
		CComBSTR s = parconn;
		*pVal = s.Detach();
    }
    COMCATCH(;);
}


////////////////////////////////////////////////////////////////////////////////////////////
// ----------------------------------------
// TRANSACTION HANDLING & NOTIFICATION 
// ----------------------------------------////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////



STDMETHODIMP CMgaProject::CreateTerritory(IMgaEventSink *sink, IMgaTerritory **pp, IMgaEventSink *rwsink) {
	COMTRY {
		CHECK_OUTPTRPAR(pp);
// MUST NOT RETURN
		CComPtr<ICoreTerritory> tt;
		COMTHROW(dataproject->CreateTerritory(&tt)); // terge ?? 
// MUST SUCCEED
		CComPtr< CMgaTerritory > ster;
		CreateComObject(ster);
#ifdef _ATL_DEBUG_INTERFACES
		tt->QueryInterface(&ster->coreterr);
#else
		ster->coreterr = tt;
#endif
		ster->mgaproject = this;; 
		allterrs.push_front(ster);
		ster->handler=sink;
		ster->rwhandler=rwsink;
#ifdef _ATL_DEBUG_INTERFACES
		ster.QueryInterface(pp);
#else
		*pp = ster.Detach();
#endif
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::CreateTerritoryWithoutSink(IMgaTerritory **pp) {
	return CreateTerritory(NULL, pp, NULL);
}

STDMETHODIMP CMgaProject::CreateAddOn(IMgaEventSink *sink, IMgaAddOn **pp) {
	COMTRY {
		CHECK_INPTRPAR(sink);
		CHECK_OUTPTRPAR(pp);
// MUST NOT RETURN

		
		if(!reserveterr) {
			CComPtr<IMgaTerritory> r;
			COMTHROW(CreateTerritory(NULL, &r));
			reserveterr = static_cast<CMgaTerritory *>(r.p);
		}
// MUST SUCCEED
		CComPtr< CMgaAddOn > saddon;
		CreateComObject(saddon);
		saddon->mgaproject = this;
		saddon->mgaproject->AddRef();
		saddon->progid = autoaddoncreate_progid;
		alladdons.push_front(saddon);
		saddon->handler=sink;
		if(inautoaddoncreate) {
			saddon->SetAutomatic();
		}
		*pp = saddon.Detach();
    }
    COMCATCH(;);

}



STDMETHODIMP CMgaProject::get_ActiveTerritory(IMgaTerritory **aterr) {
	COMTRY {
		CHECK_OUTPTRPAR(aterr);
		if((*aterr = baseterr) != NULL) (*aterr)->AddRef();
	} COMCATCH(;)
}

STDMETHODIMP CMgaProject::get_AddOns(IMgaAddOns **addons) {
	COMTRY {
		CHECK_OUTPTRPAR(addons);
		CREATECOLLECTION_FOR(IMgaAddOn, q);
		for(addoncoll::iterator j = alladdons.begin(); j != alladdons.end(); ++j) {
			q->Add(*j);
		}
		*addons = q.Detach();
	} COMCATCH(;)
}

STDMETHODIMP CMgaProject::get_Clients(IMgaClients **clients) {
	COMTRY {
		CHECK_OUTPTRPAR(clients);
		CREATECOLLECTION_FOR(IMgaClient, q);
		for(clientcoll::iterator j = allclients.begin(); j != allclients.end(); ++j) {
			q->Add(*j);
		}
		*clients = q.Detach();
	} COMCATCH(;)
}

STDMETHODIMP CMgaProject::RegisterClient(BSTR name, IDispatch *OLEServer, IMgaClient **pVal) {
	COMTRY {
		CHECK_INPTRPAR(OLEServer);
		CHECK_INSTRPAR(name);

		CComPtr< CMgaClient > client;
		CreateComObject(client);
		client->active = true;
		client->mgaproject = this;
		client->ole_server = OLEServer;
		client->name = name;
		allclients.push_front(client);
		*pVal = client.Detach();
		// TODO register only if not registered
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::GetClientByName(BSTR name, IMgaClient **pVal) {
	COMTRY {
		CHECK_INSTRPAR(name);
		clientcoll::iterator j;
		for(j = allclients.begin(); j != allclients.end(); ++j) {
			CComPtr< CMgaClient > client(*j);
			if (client->name == name) {
				*pVal = client.Detach();
				break;
			}
		}
		if(j == allclients.end()) 
			return E_MGA_NAME_NOT_FOUND;
	}
    COMCATCH(;);
}


STDMETHODIMP CMgaProject::get_Territories(IMgaTerritories **terrs) {
	COMTRY {
		CHECK_OUTPTRPAR(terrs);
		CREATECOLLECTION_FOR2(IMgaTerritory, IMgaTerritories, q);
		for(tercoll::iterator j = allterrs.begin(); j != allterrs.end(); ++j) {
			q->Add(*j);
		}
		*terrs = q.Detach();
	} COMCATCH(;)
}

STDMETHODIMP CMgaProject::get_AddOnComponents(IMgaComponents **comps) {
	COMTRY {
		CHECK_OUTPTRPAR(comps);
		CREATECOLLECTION_FOR(IMgaComponent, q);
		for(compcoll::iterator j = autocomps.begin(); j != autocomps.end(); ++j) {
			q->Add(*j);
		}
		*comps = q.Detach();
	} COMCATCH(;)
}

STDMETHODIMP CMgaProject::EnableAutoAddOns(VARIANT_BOOL bEnable) {
	COMTRY {
		if(baseterr) COMTHROW(E_MGA_ALREADY_IN_TRANSACTION);
		bool bnew = (bEnable == VARIANT_TRUE);
		if(bnew == autoaddons) return S_OK;
		autoaddons = bnew;
		if (opened_flag != OPENED_FLAG_CLOSED) {
			if(autoaddons) StartAutoAddOns();
			else StopAutoAddOns();
		}
	} COMCATCH(
		// We've already called SetErrorInfo, don't call it again
		if (e.hr == E_MGA_COMPONENT_ERROR) {
			return e.hr;
		}
	)
}


void CMgaProject::StartAutoAddOns() {
	if(!autoaddons) return;
#ifdef DEBUG
	for(addoncoll::iterator j = alladdons.begin(); j != alladdons.end(); ++j) {
		ASSERT(!(*j)->IsAutomatic());
	}
#endif
	CComPtr<IMgaRegistrar> reg;
	CComBSTR paradigm;
	COMTHROW(metapr->get_Name(&paradigm));
	CComVariant progids;
	COMTHROW(reg.CoCreateInstance(OLESTR("Mga.MgaRegistrar")));
	COMTHROW(reg->get_AssociatedComponents(paradigm, COMPONENTTYPE_ADDON, REGACCESS_BOTH, &progids));
	long p = GetArrayLength(progids);
	ASSERT(p >= 0);
	if(p)  {
		std::vector<CComBstrObj> vec;
		vec.resize(p);
		CopyTo(progids, &vec[0], (&vec[0])+vec.size());
		inautoaddoncreate = true;
		CComBSTR errs;
		for(std::vector<CComBstrObj>::iterator i = vec.begin(); i < vec.end(); ++i) {
			try {
				CComPtr<IMgaComponent> addon;
				autoaddoncreate_progid = *i;
				COMTHROW(CreateMgaComponent(addon, *i)); // Was: COMTHROW( addon.CoCreateInstance(*i) );
				ASSERT( addon != NULL );

				COMTHROW( addon->Initialize(this));
				autocomps.push_front(addon.Detach());
			} catch(hresult_exception &e) {
				BSTR err = NULL;
				GetErrorInfo(e.hr, &err);
				errs += " ";
				errs += i->p;
				errs += ": ";
				errs += err;
			}
		}
		inautoaddoncreate = false;
		autoaddoncreate_progid = L"";
		if (errs) {
			SetErrorInfo(_bstr_t(L"Could not create AddOn: ") + static_cast<const wchar_t*>(errs));
			COMTHROW(E_MGA_COMPONENT_ERROR); // change error type
		}
	}
//	SetAutomatic();
	
}


MIDL_INTERFACE("805D7A98-D4AF-3F0F-967F-E5CF45312D2C")
IDisposable : public IDispatch {
public:
    virtual VOID STDMETHODCALLTYPE Dispose() = 0;
};

void CMgaProject::StopAutoAddOns() {
	while(!autocomps.empty()) {
			CComPtr<IMgaComponent> addon;
			addon.Attach(autocomps.front());
			autocomps.pop_front();
			CComPtr<IDisposable> disposable;
			if (SUCCEEDED(addon->QueryInterface(&disposable)))
			{
				disposable->Dispose();
			}
			addon.Release();
	}
#ifdef DEBUG
		for(addoncoll::iterator j = alladdons.begin(); j != alladdons.end();++j) {
			ASSERT(!(*j)->IsAutomatic());
		}
#endif
}

void CMgaProject::RemoveClients() {
	while(!allclients.empty()) {
			CComPtr< CMgaClient > client;
			client = allclients.front();
			client->active = false;
			allclients.pop_front();
	}
}


STDMETHODIMP CMgaProject::get_ProjectStatus(long *status) {
	COMTRY {
		CHECK_OUTPAR(status);
		if (opened_flag == OPENED_FLAG_CLOSED) {
			*status  = 0;
		}	
		else if(opened_flag == OPENED_FLAG_CLOSEERROR) {
			*status = 0x80000000;
		}
		else {
			*status = 1 + (opened_flag == OPENED_FLAG_CHANGED ? 4 : 0) + (baseterr ? 8 : 0) + (read_only ? 16 : 0);
		}
	} COMCATCH(;);
}

STDMETHODIMP CMgaProject::BeginTransaction(IMgaTerritory *ter, transactiontype_enum mode)
{
	COMTRY {
		ASSERT(temporalobjs.empty());
		while (temporalobjs.size())
			temporalobjs.pop();
		ASSERT(changedobjs.empty());
		while (changedobjs.size())
			changedobjs.pop();
		ASSERT(notifyobjs.empty());
		while (notifyobjs.size())
			notifyobjs.pop();
		CComPtr<IMgaTerritory> ttemp;
		if(baseterr)
			COMTHROW(E_MGA_ALREADY_IN_TRANSACTION);
		if(!ter) {
			COMTHROW(CreateTerritory(NULL,&ttemp));
			ter = ttemp;
		}
		ASSERT(mode == TRANSACTION_GENERAL || mode == TRANSACTION_READ_ONLY  || mode == TRANSACTION_NON_NESTED);
		CComPtr<IMgaProject> p;
		COMTHROW(ter->get_Project(&p));
		if (p != this)
			COMTHROW(E_MGA_FOREIGN_OBJECT);
#ifdef _ATL_DEBUG_INTERFACES
		IUnknown* pUnk = ter;
		while (IsQIThunk(pUnk))
			pUnk = ((ATL::_QIThunk *)(pUnk))->m_pUnk;
		CMgaTerritory *t = static_cast<CMgaTerritory *>(pUnk);
#else
		CMgaTerritory *t = static_cast<CMgaTerritory *>(ter);
#endif
		if (!t->coreterr)
			COMTHROW(E_MGA_TARGET_DESTROYED);
		read_only = (mode == TRANSACTION_READ_ONLY);
		non_nestable = (mode == TRANSACTION_NON_NESTED);
		// this call fails if the project has been closed (maybe we're being called by an FCO destructor)
		HRESULT hr = dataproject->BeginTransaction(read_only? TRANSTYPE_READFIRST : TRANSTYPE_FIRST);
		if (FAILED(hr))
			COMRETURN(hr);
		checkofftemporary = false;
		in_nested = false;
		must_abort = false;
		hr = dataproject->PushTerritory(t->coreterr);
		if (FAILED(hr))
			COMRETURN(hr);
		baseterr = activeterr = t;
		MARKSIG('3');
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::BeginTransactionInNewTerr( transactiontype_enum mode, IMgaTerritory **ter)
{
	COMTRY {
		if(baseterr) COMTHROW(E_MGA_ALREADY_IN_TRANSACTION);
		// CHECK_OUTPTRPAR(ter)  // n.b. can't do this because of backwards compatibility

		// create a new territory
		CComPtr<IMgaTerritory> ttemp;
		COMTHROW(CreateTerritory(NULL,&ttemp));
		
		// begin transaction
		BeginTransaction( ttemp, mode);
		
		// [out, return] parameter
		*ter = ttemp.Detach();
	}
	COMCATCH(;);
}

STDMETHODIMP CMgaProject::CommitTransaction()
{
	COMTRY {
		ASSERT(!in_nested);
		if (!baseterr)
			COMTHROW(E_MGA_NOT_IN_TRANSACTION);
		if (checkoff)
			COMTHROW(CheckSupress(VARIANT_FALSE));
		while (!temporalobjs.empty()) {
			temporalobjs.front()->objrecordchange();
			temporalobjs.pop();
		}
		if(!notifyobjs.empty()) {
			CoreObj self;
			COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
//			self[ATTRID_MDATE] = Now();
		}
		HRESULT hr = CommitNotify();
		if (!temporalobjs.empty()) // CommitNotify may make changes. Notify only territories
		{
			while (!temporalobjs.empty()) {
				temporalobjs.front()->objrecordchange();
				temporalobjs.pop();
			}
			while (!changedobjs.empty()) {
				FCOPtr f = changedobjs.front();
				changedobjs.pop();
			}
			CommitNotify();
			ASSERT(temporalobjs.empty());
		}
		if (FAILED(hr))
			return hr;
		COMTHROW(dataproject->PopTerritory());
		short nestedCount;
		COMTHROW(dataproject->get_NestedTransactionCount(&nestedCount));
		if (nestedCount == 1 && !read_only)
			COMTHROW(GlobalNotify(GLOBALEVENT_COMMIT_TRANSACTION));


		short undosize;
		short undosizenew;
		COMTHROW(dataproject->get_UndoQueueSize(&undosize));
		COMTHROW(dataproject->CommitTransaction(read_only ? TRANSTYPE_READFIRST: TRANSTYPE_FIRST));
		COMTHROW(dataproject->get_UndoQueueSize(&undosizenew));
		baseterr = activeterr= NULL;
		read_only = false;

		if (undosize != undosizenew) {
			// this means there were changes to the core project, and we can undo this transaction
			transactioncount++;
			opened_flag = OPENED_FLAG_CHANGED;
			if (guidstat == MANUAL) {
				guidstat = CLEAN;
			}
			else {
				guidstat = DIRTY;
			}
		}
	    MARKSIG('7');
    }
    COMCATCH(;);
}



STDMETHODIMP CMgaProject::AbortTransaction() {
	COMTRY {
		ASSERT(!in_nested);
		if(!baseterr) COMTHROW(E_MGA_NOT_IN_TRANSACTION);
		aurcnt++;
		//COMTHROW(dataproject->PopTerritory()); BGY: see committransaction, it is already popped
		COMTHROW(dataproject->AbortTransaction(read_only ? TRANSTYPE_READFIRST:TRANSTYPE_FIRST));
		baseterr = activeterr= NULL;
		deferredchecks.clear();
		checkoff = false;
		while(!changedobjs.empty()) {
			changedobjs.front()->objforgetrwnotify();
			changedobjs.front()->apool.clear();
			changedobjs.pop();
		}
		while(!notifyobjs.empty()) {
			notifyobjs.front()->objforgetnotify();
			notifyobjs.pop();
		}
		while(!temporalobjs.empty()) {
			temporalobjs.front()->objforgetchange();
			temporalobjs.pop();
		}
		read_only = true;
		{
			CComPtr<IMgaTerritory> t;
			COMTHROW(CreateTerritory(NULL, &t));
			// If this fails, Mga likely didn't close a nested tx
			// ASSERT(dataproject->GetNestedTrCount == 1)
			COMTHROW(BeginTransaction(t, TRANSACTION_READ_ONLY));
			GlobalNotify(GLOBALEVENT_ABORT_TRANSACTION);
			COMTHROW(CommitTransaction());
		}
		read_only = false;
	    MARKSIG('6');
    }
    COMCATCH(read_only = false;);
}


STDMETHODIMP CMgaProject::CheckSupress(VARIANT_BOOL s) {
	COMTRY_IN_TRANSACTION {
		if (s)
			checkoff = true;
		else
		{ 
			for(objhash::iterator i = deferredchecks.begin(); i != deferredchecks.end(); ++i) {
				if (GetMetaID((*i)->self) == DTID_FOLDER)
					COMTHROW((*i)->Check());
				else
					(*i)->CheckRCS();
			}
			deferredchecks.clear();
			checkoff = false;
		}
	} COMCATCH_IN_TRANSACTION(;);
}


STDMETHODIMP CMgaProject::GlobalNotify(globalevent_enum msg) {
  COMTRY {
		{
			addoncoll::iterator i = alladdons.begin(), end = alladdons.end();
			if(i != end) {
				COMTHROW(pushterr(*reserveterr));
// The addon may Destroy() itself or release a reference while being notified
// But other addons are expected to remain in the list
				while(i != end) {
					CComPtr<CMgaAddOn> addon(*i++); // set the iterator to the next addon (since it may be deleted)
					if(msg == GLOBALEVENT_NOTIFICATION_READY && addon->notified == false) continue;
					if(addon->handler->GlobalEvent(msg)!= S_OK) {
						ASSERT(("Global notification failed", false));
					}
				}
				COMTHROW(popterr());
			}
		}
		tercoll::iterator i = allterrs.begin(), end = allterrs.end();
		for(;i != end; ++i) {
			if(msg == GLOBALEVENT_NOTIFICATION_READY && (*i)->notified == false) continue;
			else (*i)->notified = false;
			if((*i)->handler) {
				COMTHROW(pushterr(**i));
				(*i)->handler->GlobalEvent(msg); // Silently ignore failures
				COMTHROW(popterr());
			}
		}
  } COMCATCH(;);
}

STDMETHODIMP CMgaProject::Notify(globalevent_enum event) {
	COMTRY {
		if (baseterr)
			COMTHROW(E_MGA_ALREADY_IN_TRANSACTION);

		CComPtr<IMgaTerritory> t;
		COMTHROW(CreateTerritory(NULL, &t));
		COMTHROW(BeginTransaction(t, TRANSACTION_READ_ONLY));
		GlobalNotify(event);
		COMTHROW(CommitTransaction());
    }
    COMCATCH(;);
}


STDMETHODIMP CMgaProject::CommitNotify() {
  if(read_only) {
                ASSERT(changedobjs.empty() && notifyobjs.empty());
                return S_OK;
  }
  COMTRY {
                if(!baseterr)
					COMTHROW(E_MGA_NOT_IN_TRANSACTION);

                while(!changedobjs.empty()) {
                                FCOPtr f = changedobjs.front();
                                changedobjs.pop();

								HRESULT hr = f->objrwnotify();
								if (FAILED(hr))
								{
									read_only = false;
									return hr;
								}
                }

                read_only = true;
                while(!notifyobjs.empty()) {
                                FCOPtr f = notifyobjs.front();
                                notifyobjs.pop();
                                if(f->objnotify() != S_OK) {
                                        ASSERT(("Notification failed", false));
                                }
                }
                COMTHROW(GlobalNotify(GLOBALEVENT_NOTIFICATION_READY));
                read_only = false;
  } COMCATCH( read_only = false;);
}



bool CMgaProject::alreadynested() { return in_nested; }

HRESULT CMgaProject::beginnested() {
	ASSERT(!in_nested);

	ASSERT(objstocheck.empty());
	objstocheck.clear();

	HRESULT hr;
	if (non_nestable)
		hr = must_abort ? E_MGA_MUST_ABORT : S_OK;
	else
		hr = dataproject->BeginTransaction(TRANSTYPE_NESTED);

	MARKSIG('4');
	if (hr == S_OK)
		in_nested = true;
	return hr;
}

HRESULT CMgaProject::commitnested() {
	ASSERT(in_nested);

	ASSERT(objstocheck.empty());
	objstocheck.clear();

	HRESULT hr = S_OK;
	if (!non_nestable)
		hr = dataproject->CommitTransaction(TRANSTYPE_NESTED);
	MARKSIG('6');
	if (hr != S_OK)
		abortnested();
	else {
		in_nested = false;
		while (!temporalobjs.empty()) {
			temporalobjs.front()->objrecordchange();
			temporalobjs.pop();
		}
	}
	return hr;
}

HRESULT CMgaProject::abortnested() {
	ASSERT(in_nested);
	objstocheck.clear();
	in_nested = false;
	while (!temporalobjs.empty()) {
		temporalobjs.front()->objforgetchange();
		temporalobjs.pop();
	}
	MARKSIG('5');
	must_abort = true;
	if (non_nestable)
		return S_OK;
	else
		return dataproject->AbortTransaction(TRANSTYPE_NESTED);
}


HRESULT CMgaProject::pushterr(CMgaTerritory &ter) {
	COMTRY{
		ASSERT(("Territorys overwrite each other",activeterr == baseterr));
		activeterr = &ter;
		COMTHROW(dataproject->PushTerritory(ter.coreterr));
	} COMCATCH(;);
}

HRESULT CMgaProject::popterr() {
	COMTRY{
		activeterr = baseterr;
		COMTHROW(dataproject->PopTerritory());
	} COMCATCH(;);
}



STDMETHODIMP CMgaProject::Undo() {
	COMTRY{
		if (baseterr) COMTHROW(E_MGA_ALREADY_IN_TRANSACTION);
		COMTHROW(dataproject->UndoTransaction());
		transactioncount--;
		if (transactioncount == 0) {
			opened_flag = OPENED_FLAG_UNCHANGED;
			guidstat = CLEAN;
		}
		else {
			opened_flag = OPENED_FLAG_CHANGED;
			guidstat = DIRTY;
		}
		{
			aurcnt++;
			CComPtr<IMgaTerritory> t;
			COMTHROW(CreateTerritory(NULL, &t));
			COMTHROW(BeginTransaction(t, TRANSACTION_READ_ONLY));
			GlobalNotify(GLOBALEVENT_UNDO);
			COMTHROW(CommitTransaction());
		}
	}
	COMCATCH(;);
}

STDMETHODIMP CMgaProject::Redo() {

	COMTRY{
		if (baseterr)
			COMTHROW(E_MGA_ALREADY_IN_TRANSACTION);
		COMTHROW(dataproject->RedoTransaction());
		transactioncount++;
		if (transactioncount == 0) {
			opened_flag = OPENED_FLAG_UNCHANGED;
			guidstat = CLEAN;
		} else {
			opened_flag = OPENED_FLAG_CHANGED;
			guidstat = DIRTY;
		}

		{
			aurcnt++;
			CComPtr<IMgaTerritory> t;
			COMTHROW(CreateTerritory(NULL, &t));
			COMTHROW(BeginTransaction(t, TRANSACTION_READ_ONLY));
			GlobalNotify(GLOBALEVENT_REDO);
			COMTHROW(CommitTransaction());
		}
    }
    COMCATCH(;);
}

STDMETHODIMP CMgaProject::UndoRedoSize(short *undosize, short *redosize ) {
	COMTRY {
		COMTHROW(dataproject->get_UndoQueueSize(undosize));
		COMTHROW(dataproject->get_RedoQueueSize(redosize));
    }
    COMCATCH(;);
}


STDMETHODIMP CMgaProject::FlushUndoQueue() {

	if( dataproject == NULL )
		COMRETURN(E_FAIL);

	COMTRY {
		if(baseterr) COMTHROW(E_MGA_ALREADY_IN_TRANSACTION);
		COMTHROW( dataproject->FlushUndoQueue() );
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaProject::UpdateSourceControlInfo( BSTR param)
{
	CComPtr<IMgaTerritory> lm;
	COMTRY {
		COMTHROW(CreateTerritory(NULL, &lm));
		COMTHROW(BeginTransaction(lm, TRANSACTION_GENERAL));

		// hack: core will recognize this action as a source control status update
		CComBSTR hack_str = "UpdateSourceControlInfo";
		CComBSTR para_str( param);
		if( para_str.Length() > 0)
			COMTHROW(hack_str.Append( para_str));
		CoreObj  dataroot;
		COMTHROW(dataproject->get_RootObject(&dataroot.ComPtr()));
		dataroot[ATTRID_MDATE] = hack_str;

		COMTHROW(CommitTransaction());
	}
	COMCATCH(;)
}

STDMETHODIMP CMgaProject::SourceControlActiveUsers()
{
	CComPtr<IMgaTerritory> lm;
	COMTRY {
		COMTHROW(CreateTerritory(NULL, &lm));
		COMTHROW(BeginTransaction(lm, TRANSACTION_GENERAL));

		// hack: core will recognize this value as a command, will show a dlg with user names
		CComBSTR hack_str = "ShowActiveUsers";

		CoreObj  dataroot;
		COMTHROW(dataproject->get_RootObject(&dataroot.ComPtr()));
		dataroot[ATTRID_MDATE] = hack_str;

		COMTHROW(CommitTransaction());

	}
	COMCATCH(;)
}

STDMETHODIMP CMgaProject::SourceControlObjectOwner( BSTR p_optionalID)
{
	CComPtr<IMgaTerritory> lm;
	COMTRY {
		COMTHROW(CreateTerritory(NULL, &lm));
		COMTHROW(BeginTransaction(lm, TRANSACTION_GENERAL));

		// hack: core will recognize this value as a command, will show a dlg with owneruser
		CComBSTR hack_str = "WhoControlsThisObj";
		CComBSTR para_str( p_optionalID);
		if( para_str.Length() > 0)
			COMTHROW(hack_str.Append( para_str));
		CoreObj  dataroot;
		COMTHROW(dataproject->get_RootObject(&dataroot.ComPtr()));
		dataroot[ATTRID_MDATE] = hack_str;

		COMTHROW(CommitTransaction());
	}
	COMCATCH(;)
}

void CMgaProject::ObjMark(IMgaObject *s, long mask) {
	ObjFor(s)->SelfMark(mask);
}

void CMgaProject::FixupGUID(bool write) {
	if (guidstat == DIRTY) {
		::GUID newGUID;
		COMTHROW(CoCreateGuid(&newGUID));
		pendingguid.Clear();
		CopyTo(newGUID, pendingguid);
		guidstat = PENDING;
	}
	if ((guidstat == PENDING) && write) {
		ASSERT(baseterr);
		CoreObj self;
		COMTHROW(dataproject->get_RootObject(&self.ComPtr()));
		self[ATTRID_GUID] = pendingguid;
		guidstat = CLEAN;
	}
}

void CMgaProject::UpdateMGAVersion(CoreObj& p_dataroot)
{
	mgaversion = p_dataroot[ATTRID_MGAVERSION];
	if( mgaversion <= 1L)
	{
		// update done by CreateCoreMetaProject so update the ATTRID_MGAVERSION too
		mgaversion = p_dataroot[ATTRID_MGAVERSION] = 2L;
	}
}

void CMgaProject::SetNmspaceInMeta()
{
	try {
		ASSERT( metapr);
		if( metapr)
		{
			COMTHROW( metapr->SetNmspc( getNmspc()));
		}
	}
	catch( hresult_exception& ) {
		ASSERT(0);
	}
}


CComBSTR CMgaProject::getNmspc()
{
	CComBSTR bstr_nm;
	try {
		CComPtr<IMgaTerritory> terr;
		COMTHROW( get_ActiveTerritory( &terr));
		if( terr)
		{
			COMTHROW( terr->GetNamespace( &bstr_nm));
		}
	}
	catch( hresult_exception& ) {
		bstr_nm.Empty();
	}
	return bstr_nm;
}

CComBSTR CMgaProject::prefixWNmspc( CComBSTR pKindname)
{
	CComBSTR kindname_m;
	CComBSTR nm = getNmspc();
	if( nm.Length() > 0)// or if not found('::')
	{
		COMTHROW(kindname_m.AppendBSTR( nm));
		COMTHROW(kindname_m.Append( "::"));
	}

	COMTHROW(kindname_m.AppendBSTR( pKindname));

	return kindname_m;
}

STDMETHODIMP CMgaProject::CheckCollection(IMgaFCOs *coll) {
	COMTRY {
		MGACOLL_ITERATE(IMgaFCO, coll) {
			HRESULT s;
			if((s = MGACOLL_ITER->CheckProject(this)) != S_OK) return s;
		}
		MGACOLL_ITERATE_END;
	}
	COMCATCH(;)
}

// by ZolMol
STDMETHODIMP CMgaProject::CheckFolderCollection(IMgaFolders *coll) {
	COMTRY {
		MGACOLL_ITERATE(IMgaFolder, coll) {
			HRESULT s;
			if((s = MGACOLL_ITER->CheckProject(this)) != S_OK) return s;
		}
		MGACOLL_ITERATE_END;
	}
	COMCATCH(;)
}















// ----------------------------------------
// OBSOLETE stuff: Clearlocks
// ----------------------------------------

#include <set>
#include "../core/CoreUtilities.h"

bool
#if _MSC_VER >= 1700
constexpr
#endif
std::less<metaobjidpair_type>::operator ()(const metaobjidpair_type &a,const metaobjidpair_type &b) const {
	return (a.metaid == b.metaid) ? a.objid < b.objid : a.metaid < b.metaid;
}

void ClearLocks(ICoreStorage * storage, std::set<metaobjidpair_type> &mset, short mi, long oi, bool clear) {
	
	metaobjidpair_type mm; 
	mm.metaid = mi, mm.objid = oi;

	if(mset.find(mm) != mset.end()) return;
	mset.insert(mm);
	CComPtr<ICoreMetaProject> mp;
	COMTHROW(storage->get_MetaProject(&mp));
	CComPtr<ICoreMetaObject> mo;
	COMTHROW(mp->get_Object(mi, &mo));
	CComPtr<ICoreMetaAttributes> atts;
	COMTHROW(mo->get_Attributes(&atts));
	long count = 0;
	COMTHROW( atts->get_Count(&count) );
	std::unique_ptr<CComPtr<ICoreMetaAttribute>[]> array(new CComPtr<ICoreMetaAttribute>[count]);
	CComPtr<ICoreMetaAttribute> *arrptr, *arrend = array.get();
	if(count > 0) { 
		COMTHROW( atts->GetAll((unsigned long)count, &(*array.get())) ); 
	} 
	arrend = array.get()+count; 
	for(arrptr = array.get(); arrptr != arrend; arrptr++)  {
		unsigned char t;
		COMTHROW((*arrptr)->get_ValueType(&t)); 
		switch(t) {
		case VALTYPE_LOCK:
				COMTHROW(storage->put_MetaID(mi));
				COMTHROW(storage->OpenObject(oi));
				COMTHROW(storage->put_MetaAttribute((*arrptr)));
				{
				CComVariant vv;
				COMTHROW(storage->get_AttributeValue(&vv));
				ASSERT(vv.vt == VT_I2);
				if(vv.iVal) {
					COMTHROW(storage->put_AttributeValue(CComVariant(0)));
				}
				}
				break;
		case VALTYPE_COLLECTION:
				COMTHROW(storage->put_MetaID(mi));
				COMTHROW(storage->OpenObject(oi));
				COMTHROW(storage->put_MetaAttribute((*arrptr)));
				{
					CComVariant v;
					COMTHROW(storage->get_AttributeValue(&v));
					metaobjidpair_type *i = NULL;
					metaobjidpair_type *e = NULL;
					GetArrayBounds(v, i, e);
					while( i != e )	{
		 				ClearLocks(storage, mset, (short)(*i).metaid, (*i).objid, clear);
						i++;
					}
				}
		}
	}
}



STDMETHODIMP CMgaProject::CheckLocks(BSTR filename, VARIANT_BOOL clearlocks) {
  CComPtr<ICoreStorage> storage;
  CComPtr<ICoreMetaProject> genericproject;
  COMTRY {
	CreateCoreMetaProject(genericproject); // use mgaversion = 1 project model
	COMTHROW(storage.CoCreateInstance(	OLESTR("Mga.CoreRepository")));
	COMTHROW(storage->put_MetaProject(genericproject));
	COMTHROW(storage->OpenProject(filename, NULL));
	COMTHROW(storage->BeginTransaction());
	std::set<metaobjidpair_type> ccc;
	ClearLocks(storage, ccc, 1, 1, clearlocks ? true : false);
	COMTHROW(storage->CommitTransaction());
  } COMCATCH(;)
}

int CMgaProject::getMaxUndoSize()
{
	int retval = 10; // keep in sync with the default value in CoreProject.h
	try
	{
		CComPtr<IMgaRegistrar> mgareg;
		COMTHROW(mgareg.CoCreateInstance(OLESTR("Mga.MgaRegistrar")));
		CComBSTR undo_size;
		COMTHROW( mgareg->GetUndoQueueSize( REGACCESS_USER, &undo_size));
		if (undo_size != NULL && undo_size != L"") {
			int val = _wtoi(undo_size);
			if (val > 0 && val < 100) // requirement is to be above 0 and below 100
				retval = val;
		}
	}
	catch( hresult_exception&)
	{
		retval = 10;
	}

	return retval;
}

STDMETHODIMP CMgaProject::GetTopLibraries(BSTR pDispGuid, IMgaFolders **pVal)
{
	COMTRY {
		CHECK_INSTRPAR( pDispGuid);
		CREATECOLLECTION_FOR(IMgaFolder,q);

		CComPtr<IMgaFolder> rootf;
		COMTHROW( get_RootFolder( &rootf));
		CoreObj crf( rootf);
		CoreObjs children = crf[ATTRID_FPARENT+ATTRID_COLLECTION];
		ITERATE_THROUGH(children) {
			if( ITER.GetMetaID() != DTID_FOLDER) continue;
			long perm = ITER[ATTRID_PERMISSIONS];
			if( perm & LIBROOT_FLAG)
			{
				auto lib = ObjForCore(ITER);
				if( lib) 
				{
					CComBSTR gd;
					COMTHROW( lib->GetGuidDisp( &gd));

					if( gd == pDispGuid)
					{
						CComPtr<IMgaFolder> fld;
						lib->getinterface( &fld);
						q->Add( fld);
					}
				}
			}
		}
		*pVal = q.Detach();
	}
    COMCATCH(;);
}


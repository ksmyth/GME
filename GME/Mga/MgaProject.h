// MgaProject.h : Declaration of the CMgaProject

#ifndef __MGAPROJECT_H_
#define __MGAPROJECT_H_

#include "resource.h"       // main symbols
#include "MgaTrukk.h"
class CMgaTerritory;
class CMgaAddOn;
class CMgaClient;
class FCO;

#include "CommonVersionInfo.h"

/////////////////////////////////////////////////////////////////////////////
// CMgaProject
class ATL_NO_VTABLE CMgaProject : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMgaProject, &__uuidof(MgaProject)>,
	public IDispatchImpl<IMgaProject, &__uuidof(IMgaProject), &__uuidof(__MGALib)>,
	public ISupportErrorInfoImpl<&__uuidof(IMgaProject)>,
	public IGMEVersionInfoImpl
{

    DEFSIG;
	CComPtr<IUnknown> inner;
	CMgaProject *mgaproject;
public:
	CMgaProject();
	STDMETHOD(FinalConstruct)();

	~CMgaProject();

DECLARE_REGISTRY_RESOURCEID(IDR_MGAPROJECT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMgaProject)
	COM_INTERFACE_ENTRY(IMgaProject)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IID(IID_ISupportErrorInfo, IMyErrorInfoBase)
	COM_INTERFACE_ENTRY_IID(__uuidof(IGMEVersionInfo), IGMEVersionInfoImpl)
END_COM_MAP()


// IMgaProject
	HRESULT CreateSetupData(BSTR rootname, BSTR paradigmname, VARIANT guid);
public:
	STDMETHOD(get_MetaObj)(/**/ long id, /**/ IMgaMetaBase * *pVal);
	STDMETHOD(GetMetaObjDisp)(/**/ long id, /**/ IMgaMetaBase * *pVal) { return get_MetaObj( id, pVal); }

	void OpenParadigm(BSTR s, VARIANT *pGUID);
	STDMETHOD(OpenParadigm)(BSTR s, BSTR ver);
	STDMETHOD(Create)(BSTR projectname, BSTR paradigmname) { return CreateEx(projectname, paradigmname, NULLVARIANT); }
	STDMETHOD(CreateEx)(BSTR projectname, BSTR paradigmname, VARIANT parguid);

	STDMETHOD(OpenEx)(BSTR projectname, BSTR paradigmname, VARIANT paradigmGUID);
	STDMETHOD(Open)(BSTR projectname, VARIANT_BOOL *ro_mode = NULL);
	STDMETHOD(Close)(VARIANT_BOOL abort = VARIANT_FALSE);
	STDMETHOD(Save)(BSTR name = NULL, VARIANT_BOOL keepoldname = VARIANT_FALSE);
	STDMETHOD(put_Preferences)(long prefmask) {	preferences = prefmask; return S_OK; };
	STDMETHOD(put_OperationsMask)(long mask) {	opmask = mask; return S_OK; };
	STDMETHOD(get_Preferences)(long *prefmask) { 
		COMTRY {
			CHECK_OUTPAR(prefmask); 
			*prefmask = preferences;
		} COMCATCH(;);
	};
	STDMETHOD(get_OperationsMask)(long *mask) {
		COMTRY {
			CHECK_OUTPAR(mask); 
			*mask = opmask;
		} COMCATCH(;);
	};
	STDMETHOD(CheckLocks)(BSTR filename, VARIANT_BOOL clearlocks);

	STDMETHOD(CreateTerritory)(IMgaEventSink *handler, IMgaTerritory **, IMgaEventSink *h2=NULL);
	STDMETHOD(CreateTerritoryWithoutSink)(IMgaTerritory **);
	STDMETHOD(CreateAddOn)(IMgaEventSink *handler, IMgaAddOn **);
	STDMETHOD(get_ActiveTerritory)(IMgaTerritory **aterr);
	STDMETHOD(get_Territories)(IMgaTerritories **terrs);
	STDMETHOD(get_AddOns)(IMgaAddOns **addons);
	STDMETHOD(get_AddOnComponents)(IMgaComponents **comps);
	STDMETHOD(EnableAutoAddOns)(VARIANT_BOOL bEnable);
	STDMETHOD(BeginTransaction)(IMgaTerritory *, transactiontype_enum mode);
	STDMETHOD(BeginTransactionInNewTerr)(transactiontype_enum mode, IMgaTerritory **);
	STDMETHOD(AbortTransaction)();
	STDMETHOD(get_ProjectStatus)(long *stat);
	STDMETHOD(CommitTransaction)();
	STDMETHOD(CommitNotify)();
	STDMETHOD(CheckSupress)(VARIANT_BOOL);
	STDMETHOD(GlobalNotify)(globalevent_enum msg);
	STDMETHOD(Notify)(globalevent_enum event);
	STDMETHOD(Redo)();
	STDMETHOD(Undo)();
	STDMETHOD(UndoRedoSize)(short *undosize, short *redosize);
	STDMETHOD(FlushUndoQueue)();

	STDMETHOD(UpdateSourceControlInfo)( BSTR param);
	STDMETHOD(SourceControlActiveUsers)( );
	STDMETHOD(SourceControlObjectOwner)( BSTR p_optionalID);

	STDMETHOD(get_RootFolder)( IMgaFolder **pVal);
	STDMETHOD(get_RootMeta)(IMgaMetaProject **pVal);

	STDMETHOD(GetStatistics)(BSTR *statstr);
 	STDMETHOD(get_ObjectByPath)(BSTR path, IMgaObject **pVal);
 	STDMETHOD(get_NthObjectByPath)(long n_th, BSTR path, IMgaObject **pVal);
	STDMETHOD(GetFolderByPath)(BSTR path, IMgaFolder **pVal);
	STDMETHOD(GetFCOsByName)(BSTR name, IMgaFCOs **pVal);
	STDMETHOD(GetFCOByID)(BSTR id, IMgaFCO **pVal);
	STDMETHOD(GetObjectByID)(BSTR id, IMgaObject **pVal);
	STDMETHOD(EnumExtReferences)(IMgaFCOs *fcos, IMgaFCOs **pVal);

    STDMETHOD(CreateFilter)(IMgaFilter **);
	STDMETHOD(AllFCOs)(IMgaFilter *, IMgaFCOs **pVal);
	STDMETHOD(get_Name)(BSTR *pVal);
	STDMETHOD(put_Name)(BSTR newVal);
	STDMETHOD(get_Version)(BSTR *pVal);
	STDMETHOD(put_Version)(BSTR newVal);
	STDMETHOD(get_Comment)(/**/ BSTR *pVal);
	STDMETHOD(put_Comment)(/**/ BSTR newVal);
	STDMETHOD(get_Author)(/**/ BSTR *pVal);
	STDMETHOD(put_Author)(/**/ BSTR pVal);
	STDMETHOD(get_ChangeTime)( BSTR *pVal);
	STDMETHOD(get_CreateTime)( BSTR *pVal);
	STDMETHOD(get_MetaGUID)(VARIANT *pVal);
	STDMETHOD(get_MetaVersion)(BSTR *pVal);
	STDMETHOD(get_MetaName)(BSTR *pVal);
	STDMETHOD(get_GUID)(VARIANT *pVal);
	STDMETHOD(put_GUID)(VARIANT newVal);

	STDMETHOD(get_ProjectConnStr)(BSTR *pVal);
	STDMETHOD(get_ParadigmConnStr)(BSTR *pVal);
	
	STDMETHOD(QueryProjectInfo)(BSTR projectname, long *mgaversion, 
										   BSTR *paradigmname, 
										   BSTR *parversion,
										   VARIANT *paradigmGUID,
										   VARIANT_BOOL *ro_mode);
	STDMETHOD(CheckCollection)(IMgaFCOs *coll);
	STDMETHOD(CheckFolderCollection)(IMgaFolders *coll);
	STDMETHOD(get_Clients)(IMgaClients **clients);
	STDMETHOD(RegisterClient)(BSTR name, IDispatch *OLEServer, IMgaClient **client);
	STDMETHOD(GetClientByName)(BSTR name, IMgaClient **client);
	STDMETHOD(GetTopLibraries)(BSTR name, IMgaFolders **res);
#ifndef _ATL_DEBUG_INTERFACES
	ICoreProject* dataproject;
#else
	bool dataprojectNull;
	void put_dataproject(ICoreProject* dataproject)
	{
		dataprojectNull = !!dataproject;
	}
	CComPtr<ICoreProject> get_dataproject()
	{
		if (dataprojectNull)
			return NULL;
		CComPtr<ICoreProject> ret;
		COMTHROW(inner->QueryInterface(__uuidof(ICoreProject), (void**)&ret.p));
		return ret;
	}
	__declspec(property(get=get_dataproject, put=put_dataproject)) CComPtr<ICoreProject> dataproject;
#endif
	void ObjMark(IMgaObject *s, long mask);
	void FixupGUID(bool write = true);
	void UpdateMGAVersion(CoreObj& p_dataroot);
	void SetNmspaceInMeta();
	CComBSTR getNmspc();
	CComBSTR prefixWNmspc( CComBSTR pKindname);

	void StartAutoAddOns();												// start if enabled
	void StopAutoAddOns();												// stop them always
	void RemoveClients();

	CComPtr<IMgaMetaBase> FindMetaRef(metaref_type l);					// Does not add ref

private:
	CComPtr<IMgaMetaProject> metapr;

public:
	int mgaversion;

	typedef std::list<CMgaTerritory *> tercoll;    // elements do not hold a reference!! //slist
	tercoll allterrs;
	typedef std::list<CMgaAddOn *> addoncoll;	   // elements do not hold a reference!! //slist
	addoncoll alladdons;
	typedef std::list< IMgaComponent *> compcoll;  // this list holds a reference!!!! //slist
											   // (however, STL does not like smart pointers)
	typedef std::list<CMgaClient *>clientcoll;	   // elements do not hold a reference!! //slist
	clientcoll	allclients;

	compcoll	autocomps;
	CComPtr<CMgaTerritory> baseterr, activeterr, reserveterr;
	HRESULT pushterr(CMgaTerritory &terr);
	HRESULT popterr();

	typedef std::queue<FCOPtr>	objcoll;
	objcoll changedobjs, notifyobjs, temporalobjs;
	typedef stdext::hash_set<FCOPtr, FCOPtr_hashfunc> objhash;
	objhash deferredchecks;
	bool checkoff;
	bool checkofftemporary;

private:
	bool in_nested;
	bool non_nestable;
	bool must_abort;
	friend class Transaction;
	HRESULT beginnested();
	HRESULT  abortnested();
	HRESULT  commitnested();
	bool autoaddons;         // automatic addon instantiation enabled
	bool inautoaddoncreate;	 // addons are just being instantiated
	_bstr_t autoaddoncreate_progid;
#ifdef DEBUG
public:
#endif
	bool    alreadynested();

	int getMaxUndoSize();

public:
	long preferences;
	unsigned long opmask;
	bool read_only;
	
	int  guidstat;
	CComVariant pendingguid;
	enum guidmodes { CLEAN, DIRTY, PENDING, MANUAL};

	enum open_flags { OPENED_FLAG_CLOSED = 0, OPENED_FLAG_UNCHANGED = 1, OPENED_FLAG_CHANGED = 2, OPENED_FLAG_CLOSEERROR = 4};
	int transactioncount;

	int opened_flag;
	int aurcnt;   // Abort/undo/redo count
	coreobjhash objstocheck;

	CComBSTR projconn, parconn;
};


#define MODEMASK(type, mode) ((mgaproject->opmask >> (type + mode)) & 3)
#define MODEFLAG(type, flag) ((mgaproject->opmask >> (type)) & flag)



/////////////////////////////////////////////////////////////////////////////
// CMgaClient
class ATL_NO_VTABLE CMgaClient : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMgaClient, &__uuidof(MgaClient)>,
	public IDispatchImpl<IMgaClient, &__uuidof(IMgaClient), &__uuidof(__MGALib)>
{
	
public:

	CComBSTR name;
	CComPtr<IDispatch> ole_server;
	CMgaProject *mgaproject;
	bool active;

	CMgaClient()
	{
	}
	
	~CMgaClient()	{
		if(active) Destroy();
	}

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMgaClient)
	COM_INTERFACE_ENTRY(IMgaClient)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMgaClient
public:
	STDMETHOD(get_Name)( BSTR *pName ) {
		COMTRY {
			CHECK_OUTSTRPAR(pName);
			*pName = CComBSTR(name).Detach();
		} COMCATCH(;);
	}

	STDMETHOD(get_OLEServer)( IDispatch **pServer ) {
		COMTRY {
			CHECK_OUTPTRPAR(pServer);
			*pServer = ole_server;
			(*pServer)->AddRef();
		} COMCATCH(;);
	}

	STDMETHOD(get_Project)( IMgaProject **pVal ) {
		COMTRY {
			CHECK_OUTPTRPAR(pVal);
			*pVal = mgaproject;
			(*pVal)->AddRef();
		} COMCATCH(;);
	}

	STDMETHOD(Destroy)() {
		COMTRY {
			CMgaProject::clientcoll::iterator i = mgaproject->allclients.begin(), 
											  end = mgaproject->allclients.end();
			while (i != end) {
				if (*i == this) {
					i = mgaproject->allclients.erase(i);
					active = false;
					return S_OK;
				}
				else {
					i++;
				}
			}
		    ASSERT(("Client was not found among project clients",false));	
		} COMCATCH(;);
	}
};

#endif //__MGAPROJECT_H_

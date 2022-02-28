// ----------------------------------------
// Definition of MgaTerritory and MgaAddOn
// ----------------------------------------
#ifndef __MGATERRITORY_H_
#define __MGATERRITORY_H_

/////////////////////////////////////////////////////////////////////////////
// CMgaTerritory



class ATL_NO_VTABLE CMgaTerritory : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMgaTerritory, &__uuidof(MgaTerritory)>,
	public IDispatchImpl<IMgaTerritory, &__uuidof(IMgaTerritory), &__uuidof(__MGALib)>
	// TODO: ISupportErrorInfo
{
public:
    DEFSIG;
	typedef std::set<FCOPtr>	objsetcoll;
	objsetcoll inobjs;
	CMgaTerritory() {
		INITSIG('T');
#ifdef DEBUG
		MGA_TRACE("Constructed: %s - %08X\n", sig, this);
#endif
		notified = false;
		eventmask = 0xFFFFFFFF;
		rweventmask = 0;
		refcnt = 0;
	}

	~CMgaTerritory()	{ 
#ifdef DEBUG
		MGA_TRACE("Destructed: %s - %08X\n", sig, this);
#endif
		MARKSIG('9'); 
		if (coreterr) {
			Destroy();
		}
	}

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMgaTerritory)
	COM_INTERFACE_ENTRY(IMgaTerritory)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMgaTerritory
public:
	STDMETHOD(Flush)();
	STDMETHOD(Destroy)();
	STDMETHOD(put_EventMask)( unsigned long mask) { 
		COMTRY {
			if(!coreterr) COMTHROW(E_MGA_TARGET_DESTROYED);
			eventmask = mask;
		} COMCATCH(;);
	};
	STDMETHOD(put_RWEventMask)( unsigned long mask) { 
		COMTRY {
			if(!coreterr) COMTHROW(E_MGA_TARGET_DESTROYED);
			rweventmask = mask;
		} COMCATCH(;);
	};
	STDMETHOD(Associate)( IMgaObject *obj,  VARIANT userdata);
	STDMETHOD(get_Project)(IMgaProject **pVal) { 
		COMTRY {
			CHECK_OUTPTRPAR(pVal); 
			if(!coreterr) COMTHROW(E_MGA_TARGET_DESTROYED);
			*pVal = mgaproject; (*pVal)->AddRef(); 
		} COMCATCH(;)
	} 
	STDMETHOD(CheckProject)( IMgaProject *project);  
	STDMETHOD(OpenObj)(IMgaObject *oldptr, IMgaObject **newptr);
	STDMETHOD(OpenFCO)(IMgaFCO *oldptr, IMgaFCO **newptr);
	STDMETHOD(OpenFCOs)(IMgaFCOs *oldptr, IMgaFCOs **newptr);
	STDMETHOD(CloseObj)(IMgaObject *obj);
	STDMETHOD(SetNamespace)( BSTR pNmsp);
	STDMETHOD(GetNamespace)( BSTR * pNmsp);
	CMgaProject *mgaproject;
	bool notified;
	unsigned long eventmask, rweventmask;
	CComPtr<ICoreTerritory> coreterr;    // non-null if object active 
	CComPtr<IMgaEventSink> handler, rwhandler;
	int refcnt;
	CComBSTR m_bstrCurrNamespace;
};



/////////////////////////////////////////////////////////////////////////////
// CMgaAddOn
class ATL_NO_VTABLE CMgaAddOn : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMgaAddOn, &__uuidof(MgaAddOn)>,
	public IDispatchImpl<IMgaAddOn, &__uuidof(IMgaAddOn), &__uuidof(__MGALib)>
{
    DEFSIG;
	bool automatic;   // this addon was created automatically
public:
	CMgaAddOn()	{
		INITSIG('A');
#ifdef DEBUG
		MGA_TRACE("Constructed: %s - %08X\n", sig, this);
#endif
		notified = false;
		automatic = false;
	}
	~CMgaAddOn()	{
#ifdef DEBUG
		MGA_TRACE("Destructed: %s - %08X\n", sig, this);
#endif
		MARKSIG('9'); 
		if (handler)
			Destroy();
		mgaproject->Release();
	}

	void SetAutomatic() { automatic = true; }
	bool IsAutomatic() { return automatic; }

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMgaAddOn)
	COM_INTERFACE_ENTRY(IMgaAddOn)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMgaAddOn
public:
	STDMETHOD(get_Priority)( long *pVal);
	STDMETHOD(put_Priority)( long newVal);
	STDMETHOD(put_EventMask)( unsigned long mask) { 
		COMTRY {
			if (!handler)
				COMTHROW(E_MGA_TARGET_DESTROYED);
			eventmask = mask;
		} COMCATCH(;);
	};
	STDMETHOD(get_Project)(IMgaProject **pVal) { 
		COMTRY {
			if (!handler)
				COMTHROW(E_MGA_TARGET_DESTROYED);
			CHECK_OUTPTRPAR(pVal); 
			*pVal = mgaproject;
			(*pVal)->AddRef(); 
		} COMCATCH(;);
	};
	STDMETHOD(CheckProject)( IMgaProject *project);  
	STDMETHOD(Destroy)();

	unsigned long eventmask;
	bool notified;
	CMgaProject* mgaproject;
	_bstr_t progid;
	CComPtr<IMgaEventSink> handler;    // non-null if object active 
};

#endif // __MGATERRITORY_H_

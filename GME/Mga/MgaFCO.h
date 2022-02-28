// MgaFCO.h : Declaration of the CMgaFCO

#ifndef __MGAFCO_H_
#define __MGAFCO_H_

#include "resource.h"       // main symbols
#include "MgaAttribute.h"

// IMgaFCOIMPL

#define IMgaContainer IMgaObject

#define RELIDSPACE (RELID_BASE_MAX+1L)
#define RELIDSPACE_OVERFLOWED (RELID_VALUE_MAX+1UL)

// ----------------------------------------
// Generic template used in CMgaModel, CMgaAtom, CMgaFolder etc.
// most methods just dispatch to the corresponding method in the inFCO object
// ----------------------------------------
template <class T1, class T2>
class ATL_NO_VTABLE IMgaFCOImpl : public T1, public T2 {
protected:
 friend class FCO;
#ifdef _ATL_DEBUG_INTERFACES
 friend CComPtr<FCO> ObjFor(IMgaObject *s);
#else
 friend FCO *ObjFor(IMgaObject *s);
#endif
 DEFSIG;
#ifdef DEBUG
// in DEBUG, inFCO is transformed to a macro, which checks territory compatibility
 CComPtr<FCO> innFCO;
#define inFCO  (tcheck() ,innFCO)
 void tcheck() {
	 ASSERT(("You are working with an object from another territory, may ignore",\
							territory == innFCO->mgaproject->activeterr));
 }
#else
 CComPtr<FCO> inFCO;
#define innFCO inFCO
#endif
 CComPtr<CMgaTerritory> territory;
 CComVariant userdata;
public:

	IMgaFCOImpl()  { 	
		INITSIG('I'); 
#ifdef DEBUG
		MGA_TRACE("Constructed: %s - %08X\n", sig, this);
#endif
	}
 void Initialize(CMgaTerritory *terr, FCO *fco) {
	 MARKSIG('1');
	 ASSERT(terr);
	 ASSERT(fco);
	 innFCO = fco;
	 territory = terr;
	 innFCO->pubfcos.insert(FCO::pubfcohash::value_type(terr, reinterpret_cast<CMgaAtom*>(this)));
 }
 


 ~IMgaFCOImpl () { 
#ifdef DEBUG
	 MGA_TRACE("Destructed: %s - %08X\n", sig, this);
#endif
	 MARKSIG('9');

	 FCO::pubfcohash::iterator ii = innFCO->pubfcos.find(territory);
	 ASSERT(ii != innFCO->pubfcos.end());
	 innFCO->pubfcos.erase(ii);
#ifdef DEBUG
	 ii = innFCO->pubfcos.find(territory);
	 ASSERT(ii == innFCO->pubfcos.end());
#endif
 }

// methods dispatched to FCO  	 
 STDMETHOD(get_ID)( ID_type *pVal)					{ return inFCO->get_ID(pVal); }
 STDMETHOD(get_RelID)( long *pVal)					{ return inFCO->get_RelID(pVal); }
 STDMETHOD(put_RelID)( long newVal)					{ return inFCO->put_RelID(newVal); }
 STDMETHOD(get_Name)( BSTR *pVal)					{ return inFCO->get_Name(pVal); }
 STDMETHOD(put_Name)( BSTR newVal)					{ return inFCO->put_Name(newVal); }
 //by ZolMol
 STDMETHOD(get_AbsPath)( BSTR *pVal)				{ return inFCO->get_AbsPath(pVal); }
 STDMETHOD(get_Meta)( IMgaMetaFCO **pVal)			{ return inFCO->get_Meta(pVal); }
 //by ZolMol
 //STDMETHOD(get_FolderMeta)( IMgaMetaFolder **pVal)		{ return inFCO->get_Meta(pVal); }

 STDMETHOD(get_ParentModel)( IMgaModel **pVal)		{ return inFCO->get_ParentModel(pVal); }
 STDMETHOD(get_ParentFolder)( IMgaFolder **pVal)		{ return inFCO->get_ParentFolder(pVal); }
 STDMETHOD(get_MetaBase)( IMgaMetaBase **pVal)		{ return inFCO->get_MetaBase(pVal); }
 STDMETHOD(get_MetaRef)( metaref_type *pVal)		{ return inFCO->get_MetaRef(pVal); }
 STDMETHOD(get_ObjType)( objtype_enum *pVal)		{ return inFCO->get_ObjType(pVal); }
 STDMETHOD(GetAbsMetaPath)(BSTR *abspath)			{ return inFCO->GetAbsMetaPath(abspath); }
 STDMETHOD(GetRelMetaPath)(IMgaFCO *begfco, BSTR *relpath, IMgaFCOs *refs)
													{ return inFCO->GetRelMetaPath(begfco, relpath, refs); }
 STDMETHOD(get_Project)(IMgaProject **pVal)         { return innFCO->get_Project(pVal); }
 STDMETHOD(CheckProject)( IMgaProject *project)     { return innFCO->CheckProject(project); }
 STDMETHOD(SendEvent)(long mask)					{ return innFCO->SendEvent(mask); }
 STDMETHOD(get_IsLibObject)( VARIANT_BOOL *pVal)	{ return inFCO->get_IsLibObject(pVal); }

 STDMETHOD(get_Territory)(IMgaTerritory **pVal)         {   COMTRY {
															CHECK_OUTPTRPAR(pVal);
															CMgaTerritory *p;
															territory.CopyTo(&p);
															*pVal = p;
														} COMCATCH(;)
													}
 STDMETHOD(get_IsEqual)(IMgaObject *o, VARIANT_BOOL *pVal) { return inFCO->get_IsEqual(o, pVal); }
 STDMETHOD(GetIsEqualDisp)(IMgaObject *o, VARIANT_BOOL *pVal) { return inFCO->get_IsEqual(o, pVal); }
 
 STDMETHOD(DestroyObject)()							{ return inFCO->DeleteObject(); }
 STDMETHOD(get_Status) (long *p)					{ return inFCO->get_Status(p); }
 STDMETHOD(get_IsWritable) (VARIANT_BOOL *p)		{ return inFCO->get_IsWritable(p); }
		 
 STDMETHOD(Open)( openmode mode)					{ return inFCO->Open(mode); }
 STDMETHOD(Close)()									{ return inFCO->Close(); }

 STDMETHOD(Associate)( VARIANT ud)			{ userdata= ud; return S_OK; }
 STDMETHOD(get_CurrentAssociation)( VARIANT *udp)  { 
														COMTRY {
															CHECK_OUTPAR(udp);
															COMTHROW(VariantCopy(udp, &userdata)); 
														} COMCATCH(;)
													}
 STDMETHOD(GetParent)( IMgaContainer **pVal,  objtype_enum *l)  { return inFCO->GetParent(pVal,l); }


 STDMETHOD(Check)() { return inFCO->Check(); }
 STDMETHOD(CheckTree)() { return inFCO->CheckTree(); }
 STDMETHOD(put_Exempt)(VARIANT_BOOL newVal) { return inFCO->put_Exempt(newVal); }
 STDMETHOD(get_Exempt)(VARIANT_BOOL *pVal) { return inFCO->get_Exempt(pVal); }

 STDMETHOD(PutReadOnlyAccessWithPropagate)( VARIANT_BOOL pReadOnly)
								{ return inFCO->PutReadOnlyAccessWithPropagate( pReadOnly); }
 STDMETHOD(PutReadOnlyAccess)( VARIANT_BOOL pReadOnly)
								{ return inFCO->PutReadOnlyAccess( pReadOnly); }
 STDMETHOD(HasReadOnlyAccess)( VARIANT_BOOL *pReadOnly)
								{ return inFCO->HasReadOnlyAccess( pReadOnly); }

 STDMETHOD(GetGuid)( long* l1, long* l2, long* l3, long* l4)
								{ return inFCO->GetGuid( l1, l2, l3, l4); }
 STDMETHOD(PutGuid)( long l1, long l2, long l3, long l4)
								{ return inFCO->PutGuid( l1, l2, l3, l4); }
 STDMETHOD(GetGuidDisp)( BSTR *guid_str)
								{ return inFCO->GetGuidDisp( guid_str); }
 STDMETHOD(PutGuidDisp)( BSTR guid_str)
								{ return inFCO->PutGuidDisp( guid_str); }

 STDMETHOD(get_ChildObjects)(IMgaObjects **pVal) 
								{ return inFCO->get_ChildObjects(pVal); }
 STDMETHOD(get_ChildObjectByRelID)(long relid, IMgaObject **pVal) 
								{ return inFCO->get_ChildObjectByRelID(relid ,pVal); }
 STDMETHOD(GetChildObjectByRelIDDisp)(long relid, IMgaObject **pVal) 
								{ return inFCO->get_ChildObjectByRelID(relid ,pVal); }
 STDMETHOD(get_ObjectByPath)(BSTR path, IMgaObject **pVal) 
								{ return inFCO->get_ObjectByPath(path, pVal); }
 STDMETHOD(get_NthObjectByPath)(long n_th, BSTR path, IMgaObject **pVal) 
								{ return inFCO->get_NthObjectByPath(n_th, path, pVal); }
 STDMETHOD(GetObjectByPathDisp)(BSTR path, IMgaObject **pVal) 
								{ return inFCO->get_ObjectByPath(path, pVal); }
 STDMETHOD(GetNthObjectByPathDisp)(long n_th, BSTR path, IMgaObject **pVal) 
								{ return inFCO->get_NthObjectByPath(n_th, path, pVal); }
 
 STDMETHOD(get_MetaRole)( IMgaMetaRole **pVal)		{ return inFCO->get_MetaRole(pVal); }


 STDMETHOD(get_Parts)( IMgaParts **pVal)			{ return inFCO->get_Parts(pVal); }
 STDMETHOD(get_Part)( IMgaMetaAspect * asp,  IMgaPart **pVal)  { return inFCO->get_Part(asp, pVal); }
 STDMETHOD(GetPartDisp) ( IMgaMetaAspect * asp,  IMgaPart **pVal)  { return inFCO->get_Part(asp, pVal); }
 STDMETHOD(get_PartByMetaPart)( IMgaMetaPart *p,  IMgaPart **pVal)  { return inFCO->get_PartByMP(p, pVal); }
 STDMETHOD(GetPartByMetaPartDisp)( IMgaMetaPart *p,  IMgaPart **pVal)  { return inFCO->get_PartByMP(p, pVal); }
 STDMETHOD(get_Attributes)( IMgaAttributes **pVal)  { return inFCO->get_Attributes(pVal); }
 STDMETHOD(get_Attribute)( IMgaMetaAttribute *meta,  IMgaAttribute **pVal)  
													{ return inFCO->get_Attribute(meta,pVal); }
 STDMETHOD(GetAttributeDisp)( IMgaMetaAttribute *meta,  IMgaAttribute **pVal)  
													{ return inFCO->get_Attribute(meta,pVal); }
 STDMETHOD(get_AttributeByName)( BSTR name,  VARIANT *pVal) { return inFCO->get_AttributeByName(name, pVal); }
 STDMETHOD(put_AttributeByName)( BSTR name,  VARIANT newval) { return inFCO->put_AttributeByName( name, newval); }
 STDMETHOD(GetAttributeByNameDisp)( BSTR name,  VARIANT *pVal) { return inFCO->get_AttributeByName(name, pVal); }
 STDMETHOD(SetAttributeByNameDisp)( BSTR name,  VARIANT newval) { return inFCO->put_AttributeByName( name, newval); }
 STDMETHOD(get_StrAttrByName)( BSTR name,  BSTR *pVal)  { return inFCO->get_StrAttrByName(name, pVal); }
 STDMETHOD(put_StrAttrByName)( BSTR name,  BSTR newval)  { return inFCO->put_StrAttrByName(name, newval); }
 STDMETHOD(GetStrAttrByNameDisp)( BSTR name,  BSTR *pVal)  { return inFCO->get_StrAttrByName(name, pVal); }
 STDMETHOD(SetStrAttrByNameDisp)( BSTR name,  BSTR newval)  { return inFCO->put_StrAttrByName(name, newval); }
 STDMETHOD(get_IntAttrByName)( BSTR name,  long *pVal)  { return inFCO->get_IntAttrByName(name, pVal); }
 STDMETHOD(put_IntAttrByName)( BSTR name,  long newval)  { return inFCO->put_IntAttrByName(name, newval); }
 STDMETHOD(GetIntAttrByNameDisp)( BSTR name,  long *pVal)  { return inFCO->get_IntAttrByName(name, pVal); }
 STDMETHOD(SetIntAttrByNameDisp)( BSTR name,  long newval)  { return inFCO->put_IntAttrByName(name, newval); }
 STDMETHOD(get_FloatAttrByName)( BSTR name,  double *pVal)  { return inFCO->get_FloatAttrByName(name, pVal); }
 STDMETHOD(put_FloatAttrByName)( BSTR name,  double newval)  { return inFCO->put_FloatAttrByName(name, newval); }
 STDMETHOD(GetFloatAttrByNameDisp)( BSTR name,  double *pVal)  { return inFCO->get_FloatAttrByName(name, pVal); }
 STDMETHOD(SetFloatAttrByNameDisp)( BSTR name,  double newval)  { return inFCO->put_FloatAttrByName(name, newval); }
 STDMETHOD(get_BoolAttrByName)( BSTR name,  VARIANT_BOOL *pVal)  { return inFCO->get_BoolAttrByName(name, pVal); }
 STDMETHOD(put_BoolAttrByName)( BSTR name,  VARIANT_BOOL newval)  { return inFCO->put_BoolAttrByName(name, newval); }
 STDMETHOD(GetBoolAttrByNameDisp)( BSTR name,  VARIANT_BOOL *pVal)  { return inFCO->get_BoolAttrByName(name, pVal); }
 STDMETHOD(SetBoolAttrByNameDisp)( BSTR name,  VARIANT_BOOL newval)  { return inFCO->put_BoolAttrByName(name, newval); }
 STDMETHOD(get_RefAttrByName)( BSTR name,  IMgaFCO **pVal)  { return inFCO->get_RefAttrByName(name, pVal); }
 STDMETHOD(put_RefAttrByName)( BSTR name,  IMgaFCO * newval)  { return inFCO->put_RefAttrByName(name, newval); }
 STDMETHOD(GetRefAttrByNameDisp)( BSTR name,  IMgaFCO **pVal)  { return inFCO->get_RefAttrByName(name, pVal); }
 STDMETHOD(SetRefAttrByNameDisp)( BSTR name,  IMgaFCO * newval)  { return inFCO->put_RefAttrByName(name, newval); }
 STDMETHOD(ClearAttrByName)( BSTR name)			{ return inFCO->ClearAttrByName(name); }

 STDMETHOD(get_RegistryNode)( BSTR path,  IMgaRegNode **pVal)  { return inFCO->get_RegistryNode(path, pVal); }
 STDMETHOD(GetRegistryNodeDisp)( BSTR path,  IMgaRegNode **pVal)  { return inFCO->get_RegistryNode(path, pVal); }
 STDMETHOD(get_Registry)(VARIANT_BOOL virtuals, IMgaRegNodes **pVal)		{ return inFCO->get_Registry (virtuals, pVal); }
 STDMETHOD(GetRegistryDisp)(VARIANT_BOOL virtuals, IMgaRegNodes **pVal)		{ return inFCO->get_Registry (virtuals, pVal); }
 STDMETHOD(get_RegistryValue)( BSTR path,  BSTR *pVal)  { return inFCO->get_RegistryValue(path, pVal); }
 STDMETHOD(put_RegistryValue)( BSTR path,  BSTR newval)  { return inFCO->put_RegistryValue(path, newval); }
 STDMETHOD(GetRegistryValueDisp)( BSTR path,  BSTR *pVal)  { return inFCO->get_RegistryValue(path, pVal); }
 STDMETHOD(SetRegistryValueDisp)( BSTR path,  BSTR newval)  { return inFCO->put_RegistryValue(path, newval); }

 STDMETHOD(get_Constraints)( VARIANT_BOOL includemeta, IMgaConstraints **pVal)  
													{ return inFCO->get_Constraints (includemeta, pVal); }
 STDMETHOD(GetConstraintsDisp)( VARIANT_BOOL includemeta, IMgaConstraints **pVal)  
													{ return inFCO->get_Constraints (includemeta, pVal); }
 STDMETHOD(AddConstraint)( IMgaConstraint *pVal)  { return inFCO->AddConstraint (pVal); }
 STDMETHOD(DefineConstraint)(BSTR name, long mask, BSTR expr, IMgaConstraint **nc)
													{ return inFCO->DefineConstraint (name, mask, expr, nc); }

 STDMETHOD(CreateCollection)( IMgaFCOs **pVal) { return inFCO->CreateCollection(pVal); } 
 STDMETHOD(GetBaseType)(IMgaFCO ** basetype, IMgaFCO ** immbase, VARIANT_BOOL *isinst) {
						return inFCO->GetBaseType(basetype, immbase, isinst); }
 STDMETHOD(get_DerivedFrom)( IMgaFCO **pVal)		{ return inFCO->get_DerivedFrom( pVal); }
 STDMETHOD(get_Type)( IMgaFCO **pVal)			{ return inFCO->get_Type( pVal); }
 STDMETHOD(get_BaseType)( IMgaFCO **pVal) 		{ return inFCO->get_BaseType( pVal); }
 STDMETHOD(get_ArcheType)( IMgaFCO **pVal)		{ return inFCO->get_ArcheType( pVal); }
 STDMETHOD(get_IsInstance)( VARIANT_BOOL *pVal)	{ return inFCO->get_IsInstance( pVal); }
 STDMETHOD(get_IsPrimaryDerived)( VARIANT_BOOL *pVal) 	{ return inFCO->get_IsPrimaryDerived( pVal); }
 STDMETHOD(get_DerivedObjects)( IMgaFCOs **pVal) { return inFCO->get_DerivedObjects( pVal); }
 STDMETHOD(AttachToArcheType)( IMgaFCO *newtype,  VARIANT_BOOL instance) { 
													return inFCO->AttachToArcheType(newtype,instance); }
 STDMETHOD(DetachFromArcheType)()				{ return inFCO->DetachFromArcheType (); }
 STDMETHOD(get_RootFCO)(IMgaFCO **pVal) 		{ return inFCO->get_RootParent(pVal); }

 STDMETHOD(get_PartOfConns)(IMgaConnPoints **pVal)	{ return inFCO->get_PartOfConns(pVal); }
 STDMETHOD(get_MemberOfSets)(IMgaFCOs **pVal)	{ return inFCO->get_MemberOfSets(pVal); }
 STDMETHOD(get_ReferencedBy)(IMgaFCOs **pVal)	{ return inFCO->get_ReferencedBy(pVal); }


};

// ----------------------------------------
// MgaAtom is defined here (it is empty)
// ----------------------------------------
class ATL_NO_VTABLE CMgaAtom : 
	public CComCoClass<CMgaAtom, &__uuidof(MgaAtom)>,
	public IMgaFCOImpl< 
		CComObjectRootEx<CComSingleThreadModel>,
		IDispatchImpl<IMgaAtom, &__uuidof(IMgaAtom), &__uuidof(__MGALib)> >,
	public ISupportErrorInfoImpl<&__uuidof(IMgaAtom)>
{
public:
	CMgaAtom()	{}


DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMgaAtom)
	COM_INTERFACE_ENTRY(IMgaAtom)
	COM_INTERFACE_ENTRY_IID(__uuidof(IMgaObject),IMgaAtom)
	COM_INTERFACE_ENTRY_IID(__uuidof(IMgaFCO),IMgaAtom)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IID(IID_ISupportErrorInfo, IMyErrorInfoBase)
END_COM_MAP()
public:
};




// ----------------------------------------
// FCO
// ----------------------------------------
struct terr_hashfunc : public stdext::hash_compare<IMgaTerritory *> 
{
	size_t operator()(const IMgaTerritory *p_ob) const
	{
		return reinterpret_cast<size_t>( p_ob) % 5;//return (((int)p_ob) % 5);// 64 bt
	}
	bool operator()(const IMgaTerritory *p_ob1, const IMgaTerritory *p_ob2) const 
	{
		return p_ob1 < p_ob2;
	}
};

struct cp_hashfunc : public stdext::hash_compare<objid_type> 
{
	size_t operator()(const objid_type p_ob) const
	{
		return (((int)p_ob) % 5);
	}
	bool operator()(const objid_type p_ob1, const objid_type p_ob2) const 
	{
		return p_ob1 < p_ob2;
	}
};


class CMgaConnPoint;
class CMgaConstraint;

class ATL_NO_VTABLE FCO : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<FCO, &__uuidof(MgaO)>,
	public IDispatchImpl<IMgaO, &__uuidof(IMgaO), &__uuidof(__MGALib)>
{
public:
	DEFSIG;
   
DECLARE_REGISTRY_RESOURCEID(IDR_MGAO)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(FCO)
	COM_INTERFACE_ENTRY(IMgaO)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


	STDMETHOD(FinalConstruct)();

private:
//Notification
	unsigned long changemask, notifymask, temporalmask;
	// CMgaProject needs changemask
	friend class CMgaProject;

// PreDelete Notification by Tihamer for the PAMS SynchTool
	void PreDeleteNotify();

public:
#ifndef _ATL_DEBUG_INTERFACES
	class NoAddRefCoreObj : public CoreObj {	// CoreObj, but non-addref/release-ing
	public:
		void operator=(CoreObj &p) { Attach(p); } 
	   ~NoAddRefCoreObj() { Detach(); }
	}  self;
#else
	CoreObj get_self()
	{
		CoreObj myself;
		QueryInterface(__uuidof(ICoreObject), (void **)&myself);
		return myself;
	}
	__declspec(property(get = get_self)) CoreObj self;
#endif

	typedef stdext::hash_map<CMgaTerritory*, CMgaAtom*, terr_hashfunc> pubfcohash;
	pubfcohash pubfcos;
	CComPtr<CMgaProject> mgaproject;
	bool simpleconn();
	partpool ppool;
	attrpool apool;
	typedef stdext::hash_map<objid_type, CMgaConnPoint *, cp_hashfunc> cphash;
	cphash connpointhash;
	typedef stdext::hash_map<objid_type, CMgaConstraint *, cp_hashfunc> cshash;
	cshash constrainthash;

	FCO();
	~FCO();
//	void AddRef(void);
//	void Release(void);

	// these THROW
	long getstatus();
	void CheckRead();
	void CheckDeletedRead();
	void CheckWrite();

	HRESULT get_Project(IMgaProject **pVal);
	HRESULT CheckProject(IMgaProject *project);
	HRESULT get_IsEqual(IMgaObject *o, VARIANT_BOOL *pVal);
	HRESULT get_ID(ID_type *pVal);
	HRESULT get_RelID(long *pVal);
	HRESULT put_RelID(long newVal);

	HRESULT get_IsLibObject(VARIANT_BOOL *pVal);

	HRESULT get_Meta(IMgaMetaFCO **pVal);
	HRESULT get_ParentModel(IMgaModel **pVal);
	HRESULT get_ParentFolder(IMgaFolder **pVal);
	HRESULT get_MetaBase(IMgaMetaBase **pVal);
	HRESULT get_MetaRef(metaref_type *pVal);
	HRESULT get_Name(BSTR *pVal);
		void initialname();  // add inital name to object
	HRESULT put_Name(BSTR newVal);
	HRESULT get_AbsPath(BSTR *pVal);
	HRESULT get_Status(long *p);
	HRESULT get_IsWritable(VARIANT_BOOL *p);
	HRESULT get_ObjType(objtype_enum *pVal);
	HRESULT GetParent(IMgaContainer **pVal, objtype_enum *l = NULL);
	HRESULT GetAbsMetaPath(BSTR *abspath);
	HRESULT GetRelMetaPath(IMgaFCO *begfco, BSTR *relpath, IMgaFCOs *refs);

	HRESULT get_MetaRole(/**/ IMgaMetaRole **pVal);
	HRESULT Check();
	HRESULT CheckTree();

	// check the relations in refs, conns and sets
		void CheckRCS();
		// Get a list of basetypes this object is derived from
		HRESULT FCO::get_AllBaseTypes(IMgaFCOs **bases);
		// Get root parent of object
		HRESULT FCO::get_RootParent(IMgaFCO **parent, long *distance = NULL);
		// Get all parents starting from root (does not contain the object itself) 
		HRESULT FCO::get_ParentChain(IMgaFCOs **parents);
		// check if object is a child or any grandchild of parentobject (also true if itself)
		bool FCO::IsDescendantOf(IMgaFCO *rootobject, long *distance = NULL);


	HRESULT put_Exempt(VARIANT_BOOL newVal);
 	HRESULT get_Exempt(VARIANT_BOOL *pVal);
	HRESULT PutReadOnlyAccessWithPropagate( VARIANT_BOOL pReadOnly);
	HRESULT PutReadOnlyAccess( VARIANT_BOOL pReadOnly);
	HRESULT HasReadOnlyAccess( VARIANT_BOOL *pReadOnly);

	HRESULT GetGuid( long* l1, long* l2, long* l3, long* l4);
	HRESULT PutGuid( long l1, long l2, long l3, long l4);
	HRESULT GetGuidDisp( BSTR *guid_str);
	HRESULT PutGuidDisp( BSTR guid_str);

 	HRESULT get_ChildObjects(IMgaObjects **pVal);
 	HRESULT get_ChildObjectByRelID(long relid, IMgaObject **pVal); 
 	HRESULT get_ObjectByPath(BSTR path, IMgaObject **pVal);
	HRESULT get_NthObjectByPath(long n_th, BSTR path, IMgaObject **pVal);


	void assignnewchild(CoreObj &ss);
	void newrelidforchildren(CoreObj &ss);

	void inDeleteObject();
	HRESULT DeleteObject();

	HRESULT Open ( openmode mode = OPEN_READ);
	HRESULT Close ();

	HRESULT SendEvent(long mask);

//	HRESULT Associate(VARIANT userdata);
//	HRESULT get_CurrentAssociation(VARIANT *userdata);
	
	// FOLDER
    HRESULT get_ChildFolders(IMgaFolders **pVal);
	HRESULT CreateFolder(IMgaMetaFolder *meta, IMgaFolder **nf);
	HRESULT CreateRootObject(IMgaMetaFCO *meta, IMgaFCO **nobj);
	HRESULT get_MetaFolder(IMgaMetaFolder **pVal);
	// by ZolMol
	HRESULT CopyFolders(IMgaFolders *copylist, IMgaFolders **objs);
	HRESULT MoveFolders(IMgaFolders *copylist, IMgaFolders **objs);
	HRESULT CopyFolderDisp(IMgaFolder *copyfold, IMgaFolder **nobj);
	HRESULT MoveFolderDisp(IMgaFolder *movefold, IMgaFolder **nobj);
	HRESULT RefreshParent( IMgaFolder * f);
	

	// MODEL
	HRESULT CreateChildObject( IMgaMetaRole *role, IMgaFCO **newobj);
	HRESULT CreateReference( IMgaMetaRole *meta,  IMgaFCO *target,  IMgaFCO **newobj);
	HRESULT get_AspectParts( IMgaMetaAspect * asp, unsigned int filter, IMgaParts **pVal);
	HRESULT CreateSimpleConn( IMgaMetaRole *metar,  IMgaFCO *src,  IMgaFCO *dst, 
									IMgaFCOs *srcrefs, IMgaFCOs *dstrefs, IMgaFCO **newobj);
	HRESULT CreateSimpleConnDisp( IMgaMetaRole *metar,  IMgaFCO *src,  IMgaFCO *dst, 
									IMgaFCO *srcref, IMgaFCO *dstref, IMgaFCO **newobj);

// FOLDER & MODEL
	HRESULT ContainerCreateFCO(IMgaMetaFCO *meta, CoreObj &FCO);
	HRESULT GetChildrenOfKind(BSTR kn, IMgaFCOs **pVal);
	HRESULT GetDescendantFCOs(IMgaFilter* filt, IMgaFCOs **pVal); 
	HRESULT get_ChildFCOs(IMgaFCOs ** pVal);
	HRESULT get_ChildFCO(BSTR n,IMgaFCO**pVal);
	HRESULT CopyFCOs(IMgaFCOs *copylist, IMgaMetaRoles *rlist, IMgaFCOs **objs);
	HRESULT MoveFCOs(IMgaFCOs *copylist, IMgaMetaRoles *rlist, IMgaFCOs **objs);
	HRESULT CopyFCODisp(IMgaFCO *obj, IMgaMetaRole *role, IMgaFCO **nobj);
	HRESULT MoveFCODisp(IMgaFCO *obj, IMgaMetaRole *role, IMgaFCO **nobj);
	HRESULT DeriveFCO(IMgaFCO *base, IMgaMetaRole *role, VARIANT_BOOL instance, IMgaFCO** newobj);
	HRESULT AddInternalConnections(IMgaFCOs *inobjs, IMgaFCOs **objs);

	// CONNECTION
	static const CComBSTR SrcName, DstName;
	HRESULT get_NamedRoleTarget(BSTR rolename, IMgaFCO **pVal);
	HRESULT get_NamedRoleReferences(BSTR rolename, IMgaFCOs **pVal);
	HRESULT put_NamedRole(BSTR rolename, IMgaFCOs *rs, IMgaFCO * newVal);
		bool findroleobj(BSTR rolename, CoreObj &tt);
	HRESULT AddConnPoint(BSTR rolename, long maxinrole, IMgaFCO *target, IMgaFCOs *refs, IMgaConnPoint **pVal);
	HRESULT get_ConnPoints(IMgaConnPoints **pVal);


	// REFERENCE
	HRESULT get_Referred(IMgaFCO **pVal);
	HRESULT put_Referred(IMgaFCO * newVal);
	HRESULT get_RefAspect(IMgaMetaAspect **pVal);
	HRESULT put_RefAspect(IMgaMetaAspect * newVal);
	HRESULT get_UsedByConns(IMgaConnPoints **pVal);

	// SET
	HRESULT RemoveMember(IMgaFCO *member);
	HRESULT SetRemoveAll();
	HRESULT AddMember(IMgaFCO *member);
	HRESULT get_Members(IMgaFCOs * *pVal);
	HRESULT get_IsMember(IMgaFCO *obj, VARIANT_BOOL *pVal);


	// ALL
	HRESULT CreateCollection(IMgaFCOs **pVal);
	HRESULT get_Constraints(VARIANT_BOOL includemeta, IMgaConstraints **pVal);
	HRESULT CheckConstraints(long,int *,unsigned short ** );
	HRESULT AddConstraint(struct IMgaConstraint *p);
	HRESULT DefineConstraint(BSTR name, long eventmask, BSTR expression, IMgaConstraint **nconstraint);

	HRESULT GetBaseType(IMgaFCO **basetype, IMgaFCO ** immbase, VARIANT_BOOL *isinst);
	HRESULT get_DerivedFrom( IMgaFCO **pVal);
	HRESULT get_Type( IMgaFCO **pVal);
	HRESULT get_BaseType( IMgaFCO **pVal);
	HRESULT get_ArcheType( IMgaFCO **pVal);
	HRESULT get_IsInstance( VARIANT_BOOL *pVal);
	HRESULT get_IsPrimaryDerived( VARIANT_BOOL *pVal);
	HRESULT get_DerivedObjects( IMgaFCOs **pVal);
	HRESULT get_ChildDerivedFrom(IMgaFCO *baseobj, IMgaFCO **pVal);
	HRESULT DetachFromArcheType();
	HRESULT AttachToArcheType( IMgaFCO *newtype,  VARIANT_BOOL instance);

	HRESULT RefCompareToBase(short *status);				
	HRESULT RefRevertToBase();								
	HRESULT SetCompareToBase(IMgaFCO *p, short *status);				
	HRESULT SetRevertToBase(IMgaFCO *p);								
	HRESULT ConnCompareToBase(IMgaConnPoint *p, short *status);				
	HRESULT ConnRevertToBase(IMgaConnPoint *p);								

	HRESULT get_Parts(struct IMgaParts ** pVal);
	HRESULT get_Part(IMgaMetaAspect * asp, IMgaPart **pVal);
	HRESULT get_PartByMP(IMgaMetaPart *part, IMgaPart **pVal);
	HRESULT get_Attributes(IMgaAttributes **pVal);
	HRESULT get_Attribute(IMgaMetaAttribute *metaa,  IMgaAttribute **pVal);

	HRESULT get_PartOfConns(IMgaConnPoints **pVal);
	HRESULT get_MemberOfSets(IMgaFCOs **pVal);
	HRESULT get_ReferencedBy(IMgaFCOs **pVal);
	
	// THROWS!!
	CComPtr<IMgaAttribute> AbyN(BSTR name);
			CComPtr<IMgaMetaAttribute> metaattr;
	HRESULT get_AttributeByName(BSTR name, VARIANT *pVal);
	HRESULT put_AttributeByName(BSTR name, VARIANT newVal);
	HRESULT get_StrAttrByName( BSTR name,  BSTR *pVal);
	HRESULT put_StrAttrByName( BSTR name,  BSTR newVal);
	HRESULT get_IntAttrByName( BSTR name,  long *pVal);
	HRESULT put_IntAttrByName( BSTR name,  long newVal);
	HRESULT get_FloatAttrByName( BSTR name,  double *pVal);
	HRESULT put_FloatAttrByName( BSTR name,  double newVal);
	HRESULT get_BoolAttrByName( BSTR name,  VARIANT_BOOL *pVal);
	HRESULT put_BoolAttrByName( BSTR name,  VARIANT_BOOL newVal);
	HRESULT get_RefAttrByName( BSTR name,  IMgaFCO **pVal);
	HRESULT put_RefAttrByName( BSTR name,  IMgaFCO * newVal);
	HRESULT ClearAttrByName( BSTR name);
	HRESULT get_RegistryNode( BSTR path,  IMgaRegNode **pVal);
	HRESULT get_Registry( VARIANT_BOOL virtuals, IMgaRegNodes **pVal);
	HRESULT get_RegistryValue( BSTR path,  BSTR *pVal);
	HRESULT put_RegistryValue( BSTR path,  BSTR newval);

	HRESULT ChangeObject(IMgaMetaRole *r, IMgaMetaFCO *kind);

	// Library
	HRESULT doAttach(  BSTR libname, VARIANT_BOOL ungroup, IMgaFolder **nf);
	HRESULT doRefresh( BSTR libname, VARIANT_BOOL ungroup, long *numOfErrors);
	HRESULT AttachLibraryV3(BSTR libname, VARIANT_BOOL ungrouped, IMgaFolder **nf);
	HRESULT RefreshLibraryV3(BSTR libname, VARIANT_BOOL ungrouped, long *numOfErrors);

	HRESULT AttachLibrary(BSTR libname, IMgaFolder **nf);
	HRESULT RefreshLibrary(BSTR libname);
	HRESULT get_LibraryName(BSTR *libname);
	HRESULT put_LibraryName(BSTR libname);

	HRESULT GetVirtuallyIncludedBy( IMgaFolders** pDep);
	HRESULT SetVirtuallyIncludedBy( IMgaFolders*  pDep);
	HRESULT GetVirtuallyIncludes  ( IMgaFolders** pDep);
	HRESULT SetVirtuallyIncludes  ( IMgaFolders*  pDep);

	// Source control
	HRESULT GetSourceControlInfo( long* scInfo);

	void prepareDepInfo2Coll( CComBSTR& pInfo, IMgaFolders** p_resultColl);
	void prepareColl2DepInfo( IMgaFolders* pDep, CComBSTR& pResInfo);

	HRESULT get_ChildRelIDCounter(long *pVal);
	HRESULT put_ChildRelIDCounter(long pVal);

	void objsetuserdata(CMgaTerritory *t, VARIANT udata);

	void ObjMark(IMgaObject *s, long mask) { mgaproject->ObjMark(s, mask); }
	HRESULT objrwnotify();
	void objforgetrwnotify() { changemask = 0; };
	HRESULT objnotify();
	void objforgetnotify() { notifymask = 0; };
	void SelfMark(long newmask);  // not unsigned long, to avoid warnings
	void objrecordchange();
	void objforgetchange();
	void removeterrfromlist(CMgaTerritory *t);

	// lph: notification service for precursory object events (used by PRE_DESTROYED and PRE_STATUS)
	HRESULT PreNotify(unsigned long changemask, CComVariant param);

	template <class Q>
	void getinterface(Q **p, CMgaTerritory *terr = NULL) {
		CComPtr<IMgaObject> zz;
		getinterface(&zz, terr);
#if defined(DEBUG) || defined(_ATL_DEBUG_INTERFACES)
		CComQIPtr<Q> pp = zz;
		*p = pp.Detach();
#else
		* reinterpret_cast<IMgaObject **>(p) = zz.Detach();		
#endif
	}

	template <> // specialization for the template method above
	void getinterface<IMgaObject>(IMgaObject **p, CMgaTerritory *terr);

	//template <> //gives internal compiler error
	void getinterface(IMgaObject **p) // shorthand for the method above
	{ 
		getinterface<IMgaObject>(p, (CMgaTerritory *) NULL);
	}
};

class DeriveTreeTask  {
protected:
	int peercnt, internalpeercnt;
	int endreserve;   // the number of 'reserved' coreobjs at the end of the vector
	attrid_type masterattr;
private:
	std::vector<int> peerdepths;
	int selfdepth;
	virtual bool Do(CoreObj self, std::vector<CoreObj> *peers = NULL) = 0;
	void _DoWithDeriveds(CoreObj self, std::vector<CoreObj> *peers = NULL);
public:
	DeriveTreeTask() : endreserve(0), masterattr(0) { };
	void DoWithDeriveds(CoreObj self, std::vector<CoreObj> *peers = NULL);
};

class put_NameTask : public DeriveTreeTask {
	CComBSTR oldname, newname;

	bool Do(CoreObj self, std::vector<CoreObj> *peers = NULL);
public:
	put_NameTask(BSTR oname, BSTR nname);
};



#ifdef _ATL_DEBUG_INTERFACES
bool IsQIThunk(IUnknown *p);

inline FCOPtr ObjForCore(ICoreObject *s) {
#else
inline FCO *ObjForCore(ICoreObject *s) {
#endif
		CComQIPtr<IMgaO> p = s;
		if(!p) COMTHROW(E_MGA_MODULE_INCOMPATIBILITY);
#ifdef _ATL_DEBUG_INTERFACES
		IUnknown* pUnk = ((ATL::_QIThunk *)(p.p))->m_pUnk;
		while (IsQIThunk(pUnk))
			pUnk = ((ATL::_QIThunk *)(pUnk))->m_pUnk;
		return (FCO*)((ATL::CComContainedObject<FCO>*)pUnk);
#else
		return (FCO *)(p.p);
#endif
}

#ifdef _ATL_DEBUG_INTERFACES
inline CComPtr<FCO> ObjFor(IMgaObject *s) {
	IUnknown* pUnk = ((ATL::_QIThunk *)(s))->m_pUnk;
	return ((CMgaAtom*)pUnk)->innFCO;
}
#else
inline FCO *ObjFor(IMgaObject *s) {
		return (static_cast<CMgaAtom *>(s)->innFCO);
}
#endif

void CoreObjMark(CoreObj const &o, long mask);




struct CComBSTR_hashfunc : public stdext::hash_compare<CComBSTR> 
{
	size_t operator()(const CComBSTR &p_ob) const
	{
		size_t c = 0;
		for(unsigned int i = 0; i < p_ob.Length(); i+= 3) c += p_ob[i];
		return c;
	}
	bool operator()(const CComBSTR &p_ob1, const CComBSTR &p_ob2) const
	{
		return p_ob1 < p_ob2;
	}
};

struct metaref_hashfunc : public stdext::hash_compare<metaref_type>
{
	size_t operator()(const metaref_type &p_ob) const
	{
		return p_ob;
	}
	bool operator()(const metaref_type &p_ob1, const metaref_type &p_ob2) const
	{
		return p_ob1 < p_ob2;
	}
};

 
/* *******************  setcheck / docheck ******************************* */

#define CHK_NONE 0
#define CHK_NEW  1
#define CHK_CHANGED 3
#define CHK_DELETED 4
#define CHK_MOVED 7
#define CHK_ILLEGAL 8

#define CHK_REFEREES 4
#define CHK_REFCONNS 2
#define CHK_SELF 1


void setcheck(CMgaProject *mgaproject, CoreObj &ob, int code);
void docheck(CMgaProject *mgaproject);
void ObjTreeNotify(CMgaProject *mgaproject, CoreObj &self);



void ReDeriveNewObj(CMgaProject *mgaproject, CoreObj &orignobj, int targetlevel);
void ObjTreeDerive(CMgaProject *mgaproject, const CoreObj &origobj, CoreObj &newobj, coreobjpairhash &crealist, long instance);
bool ObjTreeReconnect(CoreObj self, coreobjpairhash &crealist, CoreObj const &derivtgt = NULLCOREOBJ);
void ObjDetachAndMerge( CMgaProject *mgaproject, CoreObj orig, CoreObj &nobj, coreobjpairhash& crealist, unsigned long nextrelid, bool prim);
void ObjAttach(CMgaProject *mgaproject, const CoreObj &origobj, CoreObj &newobj, coreobjpairhash &crealist, long instance, bool prim);


/* ******************* CollectionEx *************************************** */

typedef CCoreCollectionEx<IMgaFCOs, std::vector<IMgaFCO*>, IMgaFCO, IMgaFCO, &__uuidof(MgaFCOs), IDR_MGAFCOS> CMgaFCOs;
typedef CCoreCollectionEx<IMgaFolders, std::vector<IMgaFolder*>, IMgaFolder, IMgaFolder, &__uuidof(MgaFolders), IDR_MGAFOLDERS> CMgaFolders;
typedef CCoreCollectionEx<IMgaObjects, std::vector<IMgaObject*>, IMgaObject, IMgaObject, &__uuidof(MgaObjects), IDR_MGAOBJECTS> CMgaObjects;
typedef CCoreCollectionEx<IMgaRegNodes, std::vector<IMgaRegNode*>, IMgaRegNode, IMgaRegNode, &__uuidof(MgaRegNodes), IDR_MGAREGNODES> CMgaRegNodes;
#define CREATEEXCOLLECTION_FOR(x, q) CComPtr<C##x##s> q; CreateComObject(q);
#define EXCOLLECTIONTYPE_FOR(x) C##x##s 
#endif //__MGAFCO_H_

void CheckColl(IMgaProject *p, IMgaFCOs *f);

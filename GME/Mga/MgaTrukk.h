//
// Tricks, macros, definitions used throughout the Mga library
//

#pragma once

#include "MgaCoreObj.h"
#include "CommonMgaTrukk.h"

#define DIM(x) (sizeof(x)/ sizeof((x)[0]))

extern const CoreObj NULLCOREOBJ;
extern const CComVariant NULLVARIANT;


// ----------------------------------------
// Collection types
// ----------------------------------------

#define COLLECTIONTYPE_FOR2(t1, t2)  CCoreCollection<t2, std::vector<t1*>, t1, t1> 
#define CREATECOLLECTION_FOR2(t1, t2, q) \
   CComPtr< COLLECTIONTYPE_FOR2(t1, t2) > q; CreateComObject(q);

#define COLLECTIONTYPE_FOR(t)  COLLECTIONTYPE_FOR2(t, t##s)
#define CREATECOLLECTION_FOR(t,q) CREATECOLLECTION_FOR2(t, t##s, q)


// ----------------------------------------
// ITERATE through collections
// ----------------------------------------

// iterate through CoreObj colls

#define ITERATE_THROUGH(ci)  \
CoreObjs collifptr=ci; \
ASSERT( collifptr != NULL );				\
std::vector< CoreObj> vect;			\
GetAll2<ICoreObjects>(collifptr, vect);\
std::vector< CoreObj>::iterator it_i = vect.begin(), it_e = vect.end();	\
for(;it_i != it_e; ++it_i )

#define ITERATE_THROUGH_BACK(ci)  \
CoreObjs collifptr=ci; \
ASSERT( collifptr != NULL );				\
std::vector< CoreObj> vect;			\
GetAll2<ICoreObjects>(collifptr, vect);\
std::vector< CoreObj>::iterator it_b = vect.begin(), it_i = vect.end();	\
while(it_i-- != it_b) 

#define ITER  (*it_i)
#define ITER_BROKEN (it_i != it_e)

// ----------------------------------------
// CHECK PARAMETERS
// ----------------------------------------

#define COM_EQUAL(a, b) (static_cast<IUnknown *>(a) == b ? true : a.IsEqualObject(b))

#define CHECK_INPTRPAR(p)	 { if(p == NULL)				 COMTHROW( E_MGA_INPTR_NULL); }
#define CHECK_MYINPTRPARVALIDNULL(p)  { if(p != NULL) COMTHROW(p->CheckProject(mgaproject)); }
#define CHECK_MYINPTRPAR(p)	 { if(p == NULL)				 COMTHROW( E_MGA_INPTR_NULL); \
								COMTHROW(p->CheckProject(mgaproject)); }
#define CHECK_MYINPTRSPAR(p) { if(p == NULL)				 COMTHROW( E_MGA_INPTR_NULL); \
								COMTHROW( mgaproject->CheckCollection(p)); }
//by ZolMol
#define CHECK_MYINPTRSPARFOLDER(p) { if(p == NULL)				 COMTHROW( E_MGA_INPTR_NULL); \
								COMTHROW( mgaproject->CheckFolderCollection(p)); }
// KMS: was "if(p == NULL) p = NULLSTR", but starting with VS2003, that does nothing since NULLSTR.m_str==NULL
#define CHECK_INSTRPAR(p)	 { }
#define CHECK_OUTSTRPAR(p)   { if(p == NULL) COMTHROW(E_MGA_OUTPTR_NULL); if(*p != NULL) COMTHROW(E_MGA_OUTPTR_NONEMPTY); }
#define CHECK_OUTPTRPAR(p)   { if(p == NULL) COMTHROW(E_MGA_OUTPTR_NULL); if(*p != NULL) COMTHROW(E_MGA_OUTPTR_NONEMPTY); }
#define CHECK_OUTPTRPARVALIDNULL(p)  { if(p != NULL && *p != NULL) COMTHROW(E_MGA_OUTPTR_NONEMPTY); }
#define CHECK_INPAR(p)				
#define CHECK_INBOOLPAR(p)	 { if(p != VARIANT_FALSE && p != VARIANT_TRUE)	COMTHROW(E_MGA_BAD_ENUMVALUE);	}	
#define CHECK_OUTPAR(p)		 { if(p == NULL)				 COMTHROW(E_MGA_OUTPTR_NULL); }
#define CHECK_OUTVARIANTPAR(p)		 { if(p == NULL)				 COMTHROW(E_MGA_OUTPTR_NULL); }




//z! renamed to GetAll2 from GetAll (it was in conflict with GetAll defined in CommonCollection.h)
template<class COLLITF>
void GetAll2(COLLITF *coll, std::vector< CoreObj > &ret)
{
	ASSERT( coll != NULL );
	ASSERT( ret.empty() );

	long count = 0;
	COMTHROW( coll->get_Count(&count) );
	ASSERT( count >= 0 );

	ret.clear();
	ret.insert(ret.begin(), count, NULL);

	if(count > 0) COMTHROW( coll->GetAll(count, &ret.front().p) );
}



// ----------------------------------------
// STL compatible wrapped for CComBSTR
// ----------------------------------------
/*class CComBSTRNoAt : public CComBSTR {
public:
	CComBSTRNoAt() {;};
	CComBSTRNoAt(const CComBSTR &sss) : CComBSTR(sss) {;}
	CComBSTRNoAt* const operator&() {
		return this;
	}
};*/
typedef CComBSTR CComBSTRNoAt;	


// ----------------------------------------
// Operations and STL-derived datatypes defined on CoreObj
// ----------------------------------------

struct coreobj_hashfunc : public stdext::hash_compare<CoreObj> 
{
#ifndef _ATL_DEBUG_INTERFACES
	size_t operator()(const CoreObj &ob) const
	{
		return (size_t)ob.p;
	}
	bool operator()(const CoreObj &oba, const CoreObj &obb) const
	{
		// must be < logic, [ implemented based on COM_EQUAL's invokation ]
		return static_cast<IUnknown*>(const_cast<CoreObj&>(oba)) < static_cast<IUnknown*>(const_cast<CoreObj&>(obb));
	}
#else
	size_t operator()(const CoreObj &ob) const
	{
		return (size_t)(IUnknown*)getUnknown(ob);
	}
	bool operator()(const CoreObj &oba, const CoreObj &obb) const
	{
		// must be < logic, [ implemented based on COM_EQUAL's invokation ]
		return static_cast<IUnknown*>(getUnknown(oba)) < static_cast<IUnknown*>(getUnknown(obb));
	}

	IUnknownPtr getUnknown(const CoreObj &ob) const {
		auto obj = const_cast<CoreObj&>(ob);
		if (obj == nullptr) {
			return nullptr;
		}
		IUnknownPtr pUnk;
		obj->QueryInterface(&pUnk);
		return pUnk;
	}
#endif
};

typedef stdext::hash_map<CoreObj, CoreObj, coreobj_hashfunc> coreobjpairhash;
typedef stdext::hash_map<CoreObj,     int, coreobj_hashfunc> coreobjhash;



// ---------------------------------------
// sort a CoreObjs collection
// ----------------------------------------
template <class avt>
void Sort(CoreObjs &coll, attrid_type  ai) {
		CREATECOLLECTION_FOR(ICoreObject, q);
		std::vector< CoreObj> vect;			
		GetAll2<ICoreObjects>(coll, vect);
		std::vector< CoreObj>::iterator it_i = vect.begin(), it_e = vect.end();	
		std::vector< CoreObj>::iterator it_j, it_min;
		for(;it_i != it_e; ++it_i ) {
			it_j = it_i;
			avt min = (*it_j)[ai];
			it_min = it_j;
			while(++it_j != it_e) {
				avt l = (*it_j)[ai];
				if(l < min) {
					it_min = it_j;
					min = l;
				}
			}
			q->Add(*it_min);
			*it_min = *it_i;
		}
		coll.Attach(q.Detach());
}



// ----------------------------------------
// FCO SMART POINTER, compatible with STL collections
// ----------------------------------------
class FCO;

class FCOPtr {  //smart FCO pointer
	friend struct FCOPtr_hashfunc;
	FCO *p;
public:
	FCO * operator->() const { return  p; } 
	operator FCO* () const { return  p; } 
	void operator= (FCO *f);
	bool operator< (const FCOPtr &o) const;
	FCOPtr(FCOPtr const &o);
	FCOPtr(FCOPtr&& o);
	FCOPtr(FCO *f);
	FCOPtr();
	~FCOPtr();
};

struct FCOPtr_hashfunc : public stdext::hash_compare<FCOPtr>
{
	size_t operator()(const FCOPtr &ob) const
	{
		return reinterpret_cast<size_t>(ob.p) % 19;//return reinterpret_cast<size_t>( reinterpret_cast<size_t>(ob.p) % 19); // 64 bt
	}
	bool operator()(const FCOPtr &p_ob1, const FCOPtr &p_ob2) const
	{
		return p_ob1 < p_ob2;
	}
};


// ----------------------------------------
// UTILITIES FROM MGA.CPP 
// ----------------------------------------
/// THROW!!!
CComBSTR Now(void);  

// ----------------------------------------
// CLASS SIGNATURE MECHANISM USED FOR DEBUG 
// ----------------------------------------
#ifdef DEBUG
#define DEFSIG  char sig[5];
#define INITSIG(a)  { sig[0] = 'A'; sig[1] = 'B'; sig[2] = a; sig[3] = '0'; sig[4] = '\0'; }
#define MARKSIG(a)  sig[3] = a
#else
#define DEFSIG
#define INITSIG(a)
#define MARKSIG(a)
#endif


// ----------------------------------------
// ErrorInfoImpl
// ----------------------------------------
#define ISupportErrorInfoImpl IMyErrorInfoImpl


class ATL_NO_VTABLE IMyErrorInfoBase {
public:
	    BEGIN_INTERFACE
        virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject) = 0;
        
        virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;
        
        virtual ULONG STDMETHODCALLTYPE Release( void) = 0;
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid) = 0;
};

template <const IID* piid>
class ATL_NO_VTABLE IMyErrorInfoImpl : public IMyErrorInfoBase {
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
//		return (InlineIsEqualGUID(riid,*piid)) ? S_OK : S_FALSE;
		// ASSERT(InlineIsEqualGUID(riid,*piid)); not correct in many cases: e.g. riid=IMgaFCO on CMgaConnection, riid=IMgaSimpleConnection, riid=IDispatch, ....
		return S_OK;
	}
};


// ----------------------------------------
// Transaction class, {COMTRY,COMCATCH}_IN_TRANSACTION
// ----------------------------------------
#include "MgaProject.h"
#include "MgaFCO.h"
#include "MgaTerritory.h"

class Transaction {
	CMgaProject *pr;
public:
	HRESULT Begin(CMgaProject *ppr) {
		if(!ppr) { SetStandardOrGMEErrorInfo(E_MGA_ZOMBIE_NOPROJECT); return E_MGA_ZOMBIE_NOPROJECT; }
		if(!ppr->opened_flag) { SetStandardOrGMEErrorInfo(E_MGA_ZOMBIE_CLOSED_PROJECT); return E_MGA_ZOMBIE_CLOSED_PROJECT; }
		if(!ppr->activeterr) { SetStandardOrGMEErrorInfo(E_MGA_NOT_IN_TRANSACTION); return E_MGA_NOT_IN_TRANSACTION; }
		if(ppr->alreadynested()) {
			pr = NULL;
			return S_OK;
		}
		else {
			pr = ppr;
			return pr->beginnested();
		}
	}

	HRESULT Abort() {
		if(pr) return pr->abortnested();
		else return S_OK;
	}
	HRESULT Commit() {
		if(pr) 	return pr->commitnested();
		else return S_OK;
	}
};

#define COMTRY_IN_TRANSACTION { \
	Transaction ttt; \
	HRESULT hr = ttt.Begin(mgaproject); \
	if(hr != S_OK) { return hr; } \
	try

bool MgaSetErrorInfo(HRESULT hr);

#define COMRETURN_IN_TRANSACTION(hr) \
	if (FAILED(hr)) { \
		HRESULT hrTx = ttt.Abort(); \
		if (FAILED(hrTx)) return hrTx; \
		return hr; \
	} else { return ttt.Commit(); }

#define COMCATCH_IN_TRANSACTION( CLEANUP )  \
	catch(hresult_exception &e) \
	{ \
		ASSERT( FAILED(e.hr) ); \
		{ \
			if((hr = ttt.Abort()) != S_OK) return hr; \
			CLEANUP; \
		} \
		MgaSetErrorInfo(e.hr); \
		return e.hr; \
	} \
	catch(_com_error &e) \
	{ \
		ASSERT(FAILED(e.Error())); \
		if ((hr = ttt.Abort()) != S_OK) return hr; \
		{ CLEANUP; } \
		if (e.Description() != _bstr_t()) \
			SetErrorInfo(e.Description()); \
		else \
			SetStandardOrGMEErrorInfo(e.Error()); \
		return e.Error(); \
	} \
	catch(std::bad_alloc&) \
	{ \
		{ \
			if((hr = ttt.Abort()) != S_OK) return hr; \
			CLEANUP; \
		} \
		MgaSetErrorInfo(E_OUTOFMEMORY); \
		return E_OUTOFMEMORY; \
	} \
	return ttt.Commit(); }

#define MGAPREF_NO_NESTED_TX 0x00000080

#define COMTRY_IN_TRANSACTION_MAYBE \
Transaction ttt; \
if (!(this->mgaproject->preferences & MGAPREF_NO_NESTED_TX)) { \
  HRESULT hr = ttt.Begin(mgaproject); \
  if (hr != S_OK) { return hr; } \
} \
try

#define COMCATCH_IN_TRANSACTION_MAYBE( CLEANUP )  \
catch(hresult_exception &e) \
{ \
	ASSERT( FAILED(e.hr) ); \
	{ \
		HRESULT hr; \
		if(!(this->mgaproject->preferences & MGAPREF_NO_NESTED_TX) && ((hr = ttt.Abort()) != S_OK)) return hr; \
		CLEANUP; \
	} \
	MgaSetErrorInfo(e.hr); \
	return e.hr; \
} \
catch(_com_error &e) \
{ \
	ASSERT(FAILED(e.Error())); \
	HRESULT hr; \
	if(!(this->mgaproject->preferences & MGAPREF_NO_NESTED_TX) && ((hr = ttt.Abort()) != S_OK)) return hr; \
	{ CLEANUP; } \
	if (e.Description() != _bstr_t()) \
		SetErrorInfo(e.Description()); \
	else \
		SetStandardOrGMEErrorInfo(e.Error()); \
	return e.Error(); \
} \
catch(std::bad_alloc&) \
{ \
	{ \
		HRESULT hr; \
		if(!(this->mgaproject->preferences & MGAPREF_NO_NESTED_TX) && ((hr = ttt.Abort()) != S_OK)) return hr; \
		CLEANUP; \
	} \
	MgaSetErrorInfo(E_OUTOFMEMORY); \
	return E_OUTOFMEMORY; \
} \
if (!(this->mgaproject->preferences & MGAPREF_NO_NESTED_TX)) \
	return ttt.Commit(); \
else \
	return S_OK;

#define COMRETURN_IN_TRANSACTION_MAYBE(hr) \
  do { \
	HRESULT _hr = hr; \
	if (FAILED(_hr)) { \
		if (!(this->mgaproject->preferences & MGAPREF_NO_NESTED_TX)) \
		{ \
			HRESULT hrTx = ttt.Abort(); \
			if (FAILED(hrTx)) return hrTx; \
		} \
		return _hr; \
	} else { \
		if (!(this->mgaproject->preferences & MGAPREF_NO_NESTED_TX)) \
			return ttt.Commit(); \
		else \
			return S_OK; \
	} \
  } while(0);


#undef COMCATCH
#define COMCATCH( CLEANUP )  \
	catch(hresult_exception &e) \
	{ \
		ASSERT( FAILED(e.hr) ); \
		{ \
			CLEANUP; \
		} \
		SetStandardOrGMEErrorInfo(e.hr); \
		return e.hr; \
	} \
	catch(_com_error &err) \
	{ \
		struct { HRESULT hr; } e = { err.Error() }; \
		{ CLEANUP; } \
		if (err.Description() != _bstr_t()) \
			SetErrorInfo(err.Description()); \
		else \
			SetStandardOrGMEErrorInfo(err.Error()); \
		return e.hr; \
	} \
	catch(std::bad_alloc&) \
	{ \
		struct { HRESULT hr; } e = { E_OUTOFMEMORY }; \
		{ \
			CLEANUP; \
		} \
		SetStandardOrGMEErrorInfo(e.hr); \
		return e.hr; \
	} \
	return S_OK;



///////////////////////////////////////////////////////////////////////////////////
////////////////////////////// DERIVETREETASK /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

// Executes the virtual Do function for the object and all the derived types
// DoWithDeriveds is used to start the operation
//		- self : the CoreObj to work on (FCO or ConnRole, or SetNode)
//		- peers : the vector of the peer FCO-s (e.g. relation targets)
// Before starting with the initial object, it is determined 
// which targets are 'internal' i.e. to be adjusted for the subtypes
// 'false internals' are not detected, because in most cases they cannot occur.
// if they still may occur (e.g. RevertToBase), use the 'endreserve' parameter to
// explicitly indicate that they are not internals
/*class DeriveTreeTask  {
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
};*/


// ----------------------------------------
// BOOLTEMPFLAG
// keeps a bool flag set as long as it is on the stack
// ----------------------------------------
class booltempflag {
	bool *flagptr;
public:
	booltempflag(bool &a)  {	flagptr = a ? NULL : &a; set();}
	bool set() { if(flagptr) *flagptr = true; return flagptr != 0; }
	bool reset() { if(flagptr) *flagptr = false; return flagptr != 0; }
	~booltempflag() { reset(); }
};


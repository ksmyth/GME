#include "stdafx.h"
#include <stdio.h>
#include "MgaFCO.h"


#include "MgaFolder.h"
#include "MgaModel.h"
#include "MgaConnection.h"
#include "MgaReference.h"
#include "MgaSet.h"

// ----------------------------------------
// Constructor, destructor, FinalConstruct, getinterace
// ----------------------------------------

FCO::FCO()  { 
		INITSIG('F')
#ifdef DEBUG
		MGA_TRACE("Constructed: %s - %08X\n", sig, this);
#endif
		notifymask = changemask = temporalmask = 0;
}

STDMETHODIMP FCO::FinalConstruct() {
	COMTRY {
#ifndef _ATL_DEBUG_INTERFACES
			{
				CoreObj myself;
				QueryInterface(__uuidof(ICoreObject),(void **)&myself);
				self = myself;  // self does not hold reference!!!
			}
#endif
			CComPtr<ICoreProject> cp;
			COMTHROW(self->get_Project(&cp));
#ifdef _ATL_DEBUG_INTERFACES
			CComPtr<IMgaProject> proj;
			COMTHROW(cp->QueryInterface(__uuidof(IMgaProject),(void **)&proj));
			IUnknown* pUnk = ((ATL::_QIThunk *)(proj.p))->m_pUnk;
			pUnk->AddRef();
			mgaproject.Attach((CMgaProject*)(IDispatchImpl<IMgaProject, &__uuidof(IMgaProject), &__uuidof(__MGALib)>*)(pUnk));
#else
			COMTHROW(cp->QueryInterface(__uuidof(IMgaProject),(void **)&mgaproject));
#endif
	} COMCATCH(;);
}


FCO::~FCO() { 
#ifdef DEBUG
		MGA_TRACE("Destructed: %s - %08X\n", sig, this);
#endif
		MARKSIG('9');
		ASSERT(pubfcos.begin() == pubfcos.end());
}


// return an IMgaObject for this FCO. By default, use the active territory to create it
template <>
void FCO::getinterface<IMgaObject>(IMgaObject **p, CMgaTerritory *terr) {
	ASSERT(mgaproject->activeterr);
	if(terr == NULL) terr = mgaproject->activeterr;
	pubfcohash::iterator ii = pubfcos.find(terr);
	if(ii != pubfcos.end())  ((*p) = (*ii).second)->AddRef();
	else {
		switch(GetMetaID(self)) {
		case DTID_MODEL:		{ CComPtr< CMgaModel > np; CreateComObject(np); np->Initialize(terr, this); *p = np.Detach(); } break;
		case DTID_ATOM:			{ CComPtr< CMgaAtom > np; CreateComObject(np); np->Initialize(terr, this); *p = np.Detach(); } break;
		case DTID_REFERENCE:	{ CComPtr< CMgaReference > np; CreateComObject(np); np->Initialize(terr, this); *p = np.Detach(); } break;
		case DTID_CONNECTION:	{ CComPtr< CMgaConnection > np; CreateComObject(np); np->Initialize(terr, this); *p = np.Detach(); } break;
		case DTID_SET:			{ CComPtr< CMgaSet > np; CreateComObject(np); np->Initialize(terr, this); *p = np.Detach(); } break;
		case DTID_FOLDER:		{ CComPtr< CMgaFolder > np; CreateComObject(np); np->Initialize(terr, this); *p = np.Detach(); } break;
		}
	}
	ASSERT(("Could not create MGA object",p));
}

// ----------------------------------------
// FCO status information methods: readibility, writability, IsEqual
// ----------------------------------------
//throws!!
long FCO::getstatus() {
		if(!mgaproject) COMTHROW(E_MGA_ZOMBIE_NOPROJECT);
		if(!mgaproject->opened_flag) COMTHROW(E_MGA_ZOMBIE_CLOSED_PROJECT);
		if(!mgaproject->activeterr) COMTHROW(E_MGA_NOT_IN_TRANSACTION);
		VARIANT_BOOL pp;
		if(self->get_IsDeleted(&pp) != S_OK) return OBJECT_ZOMBIE;
		return pp ? OBJECT_DELETED  :  OBJECT_EXISTS;
}

//throws!!
void FCO::CheckDeletedRead() {
		if(getstatus() == OBJECT_ZOMBIE) COMTHROW(E_MGA_OBJECT_ZOMBIE);
}

//throws!!
void FCO::CheckRead() {
		if(getstatus() != OBJECT_EXISTS) COMTHROW(E_MGA_OBJECT_DELETED);
}

//throws!!
void FCO::CheckWrite() {
		CheckRead();
		if(mgaproject->read_only) COMTHROW(E_MGA_READ_ONLY_ACCESS);
		if(self[ATTRID_PERMISSIONS] & LIBRARY_FLAG) COMTHROW(E_MGA_LIBOBJECT);
		if(self[ATTRID_PERMISSIONS] & READONLY_FLAG) COMTHROW(E_MGA_READ_ONLY_ACCESS);
}

HRESULT FCO::get_IsEqual(IMgaObject *o, VARIANT_BOOL *pVal) {
	COMTRY {
		CHECK_OUTPAR(pVal);
		if(o == NULL) *pVal = VARIANT_FALSE;
		else {
			CHECK_MYINPTRPAR(o);
			*pVal = (ObjFor(o) == this) ? VARIANT_TRUE : VARIANT_FALSE;
		}
	} COMCATCH(;)
}


HRESULT FCO::get_Status(long *p) {
	COMTRY {
		CHECK_OUTPAR(p);
		*p = getstatus(); 
	} COMCATCH(;);
}

HRESULT FCO::get_IsWritable(VARIANT_BOOL *p) {
	COMTRY {
		CHECK_OUTPAR(p);
		*p = VARIANT_TRUE;
	} COMCATCH(;)
}


// is its metaobject a simpleconnection??
bool FCO::simpleconn() {
	CComPtr<IMgaMetaFCO> meta;
	if(get_Meta(&meta) != S_OK) return false;
	CComQIPtr<IMgaMetaConnection> metac = meta;
	if(!metac) return false;
	VARIANT_BOOL s;
	if(metac->get_IsSimple(&s) != S_OK) return false;
	return s ? true : false;
}


// ----------------------------------------
// Basic FCO info: project, ID, relID, meta, metarole, 
// ----------------------------------------

HRESULT FCO::get_Project(IMgaProject **pVal) { 
	COMTRY {
		CHECK_OUTPTRPAR(pVal); 
		*pVal = mgaproject; (*pVal)->AddRef(); 
	} COMCATCH(;)
} 


HRESULT FCO::CheckProject(IMgaProject *project) {
	return(project == mgaproject? S_OK : E_MGA_FOREIGN_PROJECT);
}


HRESULT FCO::get_ObjType(objtype_enum *pVal) { 
		COMTRY {
			CheckDeletedRead();
			CHECK_OUTPAR(pVal);
			*pVal = static_cast<objtype_enum>(GetMetaID(self) - DTID_BASE);
		} COMCATCH(;)
	};


HRESULT FCO::get_ID(ID_type *pVal) {
	COMTRY {
		CheckDeletedRead();
		CHECK_OUTSTRPAR(pVal);
		OLECHAR t[20];
		objid_type ss;
		COMTHROW(self->get_ObjID(&ss));
		swprintf(t, 20, OLESTR("id-%04lx-%08lx"), (unsigned long)GetMetaID(self), ss);
		*pVal = CComBSTR(t).Detach();
	} COMCATCH(;);
}

HRESULT FCO::get_RelID(long *pVal) {
	COMTRY {
		CheckRead();
		CHECK_OUTPAR(pVal);
		*pVal = self[ATTRID_RELID];
	} COMCATCH(;);
}

HRESULT FCO::put_RelID(long newVal) {
	COMTRY_IN_TRANSACTION {
		CheckWrite();
		if (newVal <= 0)
			COMTHROW(E_MGA_ARG_RANGE);
		self[ATTRID_RELID] = newVal;
	} COMCATCH_IN_TRANSACTION(;);
}


HRESULT FCO::get_MetaRole(/**/ IMgaMetaRole **pVal) {
		COMTRY {
			CheckDeletedRead();
			CHECK_OUTPTRPAR(pVal);
			metaref_type role = self[ATTRID_ROLEMETA];
			if(role) {
				mgaproject->SetNmspaceInMeta();
				*pVal = CComQIPtr<IMgaMetaRole>(mgaproject->FindMetaRef(role)).Detach();
				if(!(*pVal)) COMTHROW(E_MGA_META_INCOMPATIBILITY);
			}
			// FIXME: need to return an E_ here

		} COMCATCH(;);
}

HRESULT FCO::get_Meta(IMgaMetaFCO **pVal) { 
	COMTRY {
		CheckDeletedRead();
		CHECK_OUTPTRPAR(pVal);
		COMTRY {
			mgaproject->SetNmspaceInMeta();
			*pVal = CComQIPtr<IMgaMetaFCO>(mgaproject->FindMetaRef(self[ATTRID_META])).Detach();
		} COMCATCH(e.hr = E_MGA_META_INCOMPATIBILITY;)

		// unreachable code ? really?
		if(!(*pVal)) 
			COMTHROW(E_MGA_META_INCOMPATIBILITY);
	} COMCATCH(;)
}

HRESULT FCO::get_MetaBase(IMgaMetaBase **pVal) { 
	COMTRY {
		CheckDeletedRead();
		CHECK_OUTPTRPAR(pVal);
		mgaproject->SetNmspaceInMeta();
		*pVal = mgaproject->FindMetaRef(self[ATTRID_META]).Detach();
	} COMCATCH(;)
}

HRESULT FCO::get_MetaRef(metaref_type *pVal) { 
	COMTRY {
		CheckDeletedRead();
		CHECK_OUTPTRPAR(pVal);
		mgaproject->SetNmspaceInMeta();
		*pVal = self[ATTRID_META];
	} COMCATCH(;)
}

// ----------------------------------------
// Parent and grandparent access
// ----------------------------------------

HRESULT FCO::GetParent(IMgaContainer **pVal, objtype_enum *l) { 
		COMTRY {
			CheckRead();
			CHECK_OUTPTRPARVALIDNULL(pVal);
			CComPtr<IMgaContainer> pv;
			CoreObj par;
			par = self[ATTRID_PARENT];
			if(par.GetMetaID() != DTID_ROOT) {
				ObjForCore(par)->getinterface(&pv);
				if (l != NULL)
					COMTHROW(pv->get_ObjType(l));
				if(pVal != NULL) {
					*pVal = pv.Detach();
				}
			}
			else {
				if(l != NULL) *l = OBJTYPE_NULL;
				if(pVal != NULL) *pVal = NULL;
			}
		} COMCATCH(;)
	};


HRESULT FCO::get_ParentModel(IMgaModel **pVal) {
		COMTRY {
			CheckRead();
			CHECK_OUTPTRPAR(pVal);
			CoreObj par;
			par = self[ATTRID_FCOPARENT];
			if(par.GetMetaID() == DTID_MODEL) {
				CComPtr<IMgaModel> pv;
				ObjForCore(par)->getinterface(&pv);
				*pVal = pv.Detach();
				COMTHROW((*pVal)->Open(OPEN_READ));
			}
		} COMCATCH(;);
}

HRESULT FCO::get_ParentFolder(IMgaFolder **pVal) {
		COMTRY {
			CheckRead();
			CHECK_OUTPTRPAR(pVal);
			CoreObj par;
			par = self[ATTRID_FCOPARENT];
			if(par.GetMetaID() == DTID_FOLDER) {
				CComPtr<IMgaFolder> pv;
				ObjForCore(par)->getinterface(&pv);
				*pVal = pv.Detach();
				COMTHROW((*pVal)->Open(OPEN_READ));
			}
		} COMCATCH(;);
}



HRESULT FCO::get_RootParent(IMgaFCO **parent, long *distance) {       // retuns itself if root model
	COMTRY {
		long d = 0;
		CComPtr<IMgaFCO> cur;
		getinterface(&cur);
		while(true) {
			CComPtr<IMgaContainer> f;
			objtype_enum t;
			COMTHROW(cur->GetParent(&f, &t));
			if(t != OBJTYPE_MODEL) break;
			cur = NULL;
			f.QueryInterface(&cur);
			d++;
		}
		if(parent) {
			CHECK_OUTPTRPAR(parent);
			*parent = cur.Detach();
		}
		if(distance) *distance = d;
	} COMCATCH(;)
}

HRESULT FCO::get_ParentChain(IMgaFCOs **parents) {    // returns the real parents only (not self)
	COMTRY {
		CComPtr<IMgaFCO> cur;
		getinterface(&cur);
		CREATEEXCOLLECTION_FOR(MgaFCO, q);
		while(true) {
			CComPtr<IMgaContainer> f;
			objtype_enum t;
			COMTHROW(cur->GetParent(&f, &t));
			if(t != OBJTYPE_MODEL) break;
			f.QueryInterface(&cur);
			q->Add(cur);
		}
		*parents = q.Detach();
	} COMCATCH(;)
}

// check if object is a child or any grandchild of parentobject (also true if itself)
bool FCO::IsDescendantOf(IMgaFCO *object, long *distance) {
		int d;
		if(IsContained(self, CoreObj(object), &d)) {
			if(distance) *distance = d;
			return true;
		}
		else return false;

}

// ----------------------------------------
// Name access
// ----------------------------------------
HRESULT FCO::get_Name(BSTR *pVal)		{ 
	COMTRY {
		CheckDeletedRead();
		CHECK_OUTPAR(pVal);  
		*pVal = CComBSTR(self[ATTRID_NAME]).Detach();
	} COMCATCH(;);
};


/*class put_NameTask : public DeriveTreeTask {
	CComBSTR oldname, newname;

	bool Do(CoreObj self, std::vector<CoreObj> *peers = NULL) {
		if(!(oldname == CComBSTR(self[ATTRID_NAME]))) return false;
		if(ObjForCore(self)->mgaproject->preferences & MGAPREF_NODUPLICATENAMES) {
			CoreObj parent = self[ATTRID_PARENT];
			if(parent.GetMetaID() != DTID_ROOT) {  // Make sure it is not rootfolder (=parent is not rootobj)
				CoreObjs children = parent[ATTRID_PARENT+ATTRID_COLLECTION];
				ITERATE_THROUGH(children) {
					CComBSTR n = ITER[ATTRID_NAME];
					if(n == newname && !COM_EQUAL(self, ITER)) COMTHROW(E_MGA_NAME_DUPLICATE);
				}
			}		
		}
		self[ATTRID_NAME] = newname;
		ObjForCore(self)->SelfMark(OBJEVENT_PROPERTIES);
		if(self.IsFCO() && self.IsRootOfDeriv()) return false;   // Type renaming does not affect instances
		return true;
	}

public:
	put_NameTask(BSTR oname, BSTR nname) : oldname(oname), newname(nname) { ; }
};*/

put_NameTask::put_NameTask(BSTR oname, BSTR nname) : oldname(oname), newname(nname) { ; }

bool put_NameTask::Do(CoreObj self, std::vector<CoreObj> *peers /*= NULL*/) {
		if(!(oldname == CComBSTR(self[ATTRID_NAME]))) return false;
		if(ObjForCore(self)->mgaproject->preferences & MGAPREF_NODUPLICATENAMES) {
			CoreObj parent = self[ATTRID_PARENT];
			if(parent.GetMetaID() != DTID_ROOT) {  // Make sure it is not rootfolder (=parent is not rootobj)
				CoreObjs children = parent[ATTRID_PARENT+ATTRID_COLLECTION];
				ITERATE_THROUGH(children) {
					CComBSTR n = ITER[ATTRID_NAME];
					if(n == newname && !COM_EQUAL(self, ITER)) COMTHROW(E_MGA_NAME_DUPLICATE);
				}
			}		
		}
		self[ATTRID_NAME] = newname;
		ObjForCore(self)->SelfMark(OBJEVENT_PROPERTIES);
		if(self.IsFCO() && self.IsRootOfDeriv()) return false;   // Type renaming does not affect instances
		return true;
	}


void  FCO::initialname() { 
	if(mgaproject->preferences & MGAPREF_NAMENEWOBJECTS) {
		metaref_type mr;
		if(self.IsFCO() && !self.IsRootFCO()) mr = self[ATTRID_ROLEMETA];
		else mr = self[ATTRID_META];
		CComBSTR nname;
		COMTHROW(mgaproject->FindMetaRef(mr)->get_Name(&nname));
		if(mgaproject->preferences & MGAPREF_NODUPLICATENAMES) {
			size_t len = nname.Length();
			unsigned int freenum = 0;
			CoreObj parent = self[ATTRID_PARENT];
			if(parent.GetMetaID() != DTID_ROOT) {  // Make sure it is not rootfolder (=parent is not rootobj)
				CoreObjs children = parent[ATTRID_PARENT+ATTRID_COLLECTION];
				ITERATE_THROUGH(children) {
					CComBSTR n = ITER[ATTRID_NAME];
					if(!wcsncmp(nname, n, len)) {
						unsigned int f = 0;
						if (n.Length() == len)
							f = 1;
						else {
							swscanf(n+len,L"-%u", &f);
							f++;
							if(f > NEWNAME_MAXNUM) COMTHROW(E_MGA_GEN_OUT_OF_SPACE);
						}
						if(f > freenum) freenum = f;
					}
				}
			}
			if(freenum) {
				OLECHAR p[10];
				swprintf(p, 10, L"-%d",freenum);
				COMTHROW(nname.Append(p));
			}
		}
		self[ATTRID_NAME] = nname;
	}
}

HRESULT FCO::put_Name(BSTR newVal)    { 
	COMTRY_IN_TRANSACTION_MAYBE {
	CheckWrite();
		CHECK_INPAR(newVal);  
		if(self[ATTRID_PERMISSIONS] & LIBROOT_FLAG) COMTHROW(E_MGA_LIBOBJECT);
		if(CComBSTR(self[ATTRID_NAME]) != newVal) {
			put_NameTask(CComBSTR(self[ATTRID_NAME]), newVal).DoWithDeriveds(self);;
		}
	}
	COMCATCH_IN_TRANSACTION_MAYBE(;);	
};

void giveme( CMgaProject *mgaproject, CoreObj par, CoreObj cur, CComBSTR cur_kind, int *relpos)
{
	int count_lower_ids = 0;
	CComBSTR cur_name = cur[ATTRID_NAME];
	objid_type cur_id = cur.GetObjID();
	
	CoreObjs children = par[ATTRID_FCOPARENT+ATTRID_COLLECTION];
	ITERATE_THROUGH(children) {
		CComBSTR n = ITER[ATTRID_NAME];

		CComBSTR kind;
		COMTHROW( mgaproject->FindMetaRef( ITER[ATTRID_META])->get_Name( &kind));

		bool similar = n == cur_name;
		//similar = similar || ITER[ATTRID_PERMISSIONS] == LIBROOT_FLAG && libraryNameEqual(n, name_b);
		similar = similar && kind == cur_kind;
		if( similar) // similar name 
		{
			objid_type id = ITER.GetObjID();
			if( cur_id > id)
				++count_lower_ids;
		}
	}
	*relpos = count_lower_ids;
}

// gives back the path to an object starting from the rootfolder (does not include project name)
HRESULT FCO::get_AbsPath(BSTR *pVal)		{ 
	COMTRY {
		CheckRead();
		CComBSTR path("");

		CoreObj par = self[ATTRID_PARENT];
		CoreObj cur = self;
		while( !par.IsRootFolder())
		{
			CComBSTR tp("/@");
			COMTHROW(tp.Append( cur[ATTRID_NAME]));

			COMTHROW(tp.Append("|kind="));
			CComBSTR metakind;
			COMTHROW( mgaproject->FindMetaRef( cur[ATTRID_META])->get_Name( &metakind));
			COMTHROW(tp.Append( metakind));

			int n;
			giveme( mgaproject, par, cur, metakind, &n);
			char p[10]; sprintf( p, "%d", n);
			CComBSTR relative_pos;
			COMTHROW(tp.Append("|relpos="));
			COMTHROW(tp.Append( p));

			COMTHROW(tp.Append( path));
			path = tp;
			cur = par;
			par = par[ATTRID_PARENT];
		}
		CheckDeletedRead();
		CHECK_OUTPAR(pVal);  
		*pVal = path.Detach();
	} COMCATCH(;);
};

// ----------------------------------------
// Add FCO to a collection (create new coll if null)
// ----------------------------------------
HRESULT FCO::CreateCollection(IMgaFCOs **pVal) {
	COMTRY {
// check type of collection
		CComPtr<IMgaFCO> tthis;
		getinterface(&tthis);
		if(*pVal) {
			CComPtr< COLLECTIONTYPE_FOR(IMgaFCO)> q;
			q.Attach(static_cast< COLLECTIONTYPE_FOR(IMgaFCO) *>(*pVal));
			q->Add(tthis);
			/* *pVal = */q.Detach(); // modified by ZolMol
		}
		else {
			CREATEEXCOLLECTION_FOR(MgaFCO,q2)
			COMTHROW(q2->Append(tthis));
			*pVal = q2.Detach(); // added by ZolMol
		}
	} COMCATCH(;);
}


// ----------------------------------------
// Parts access
// ----------------------------------------
HRESULT FCO::get_Parts(struct IMgaParts ** pVal) {
		COMTRY {
			CheckRead();
			CHECK_OUTPTRPAR(pVal);

			CComPtr<IMgaMetaRole> metaro;					
			COMTHROW(get_MetaRole(&metaro));
			if(!metaro) COMTHROW( E_MGA_ROOTFCO);

			CComPtr<IMgaMetaParts> metaps;					
			COMTHROW(metaro->get_Parts(&metaps));

			CREATECOLLECTION_FOR(IMgaPart,q);
			MGACOLL_ITERATE(IMgaMetaPart, metaps) {
					metaref_type r;	            				
					COMTHROW(MGACOLL_ITER->get_MetaRef(&r));
					q->Add(ppool.getpoolobj(r, this, mgaproject));
			}
			MGACOLL_ITERATE_END;
			*pVal = q.Detach();

		} COMCATCH(;);
}

HRESULT FCO::get_Part(IMgaMetaAspect * asp, IMgaPart **pVal) {
		COMTRY {
			CheckRead();
			CHECK_INPTRPAR(asp);
			CHECK_OUTPTRPAR(pVal);

			CComPtr<IMgaPart> ret;

			CComPtr<IMgaMetaRole> metaro;					
			COMTHROW(get_MetaRole(&metaro));
			if(!metaro) COMTHROW( E_MGA_ROOTFCO);

			CComPtr<IMgaMetaParts> metaps;					
			COMTHROW(metaro->get_Parts(&metaps));
			MGACOLL_ITERATE(IMgaMetaPart, metaps) {
				CComPtr<IMgaMetaAspect> metaa;					
				COMTHROW(MGACOLL_ITER->get_ParentAspect(&metaa));
				if(COM_EQUAL(metaa, asp)) {
					metaref_type r;	            				
					COMTHROW(MGACOLL_ITER->get_MetaRef(&r));
					ret = ppool.getpoolobj(r, this, mgaproject);
					break;
				}
			}
			if (ret == nullptr)
			{
				SetStandardOrGMEErrorInfo(E_NOTFOUND);
				return E_NOTFOUND;
			}
			*pVal = ret.Detach();

			MGACOLL_ITERATE_END;
		} COMCATCH(;);
}

HRESULT FCO::get_PartByMP(IMgaMetaPart *part, IMgaPart **pVal) {
		COMTRY {
			CheckRead();
			CHECK_INPTRPAR(part);
			CHECK_OUTPTRPAR(pVal);

			CComPtr<IMgaMetaRole> metaro, metaro2;					
			COMTHROW(get_MetaRole(&metaro));
			if(!metaro) COMTHROW( E_MGA_ROOTFCO);

			COMTHROW(part->get_Role(&metaro2));
			if(!COM_EQUAL(metaro, metaro2)) COMTHROW( E_MGA_INVALID_ROLE);

			metaref_type r;	            				
			COMTHROW(part->get_MetaRef(&r));
			*pVal = ppool.getpoolobj(r, this, mgaproject).Detach();
		} COMCATCH(;);
}



// ----------------------------------------
// Attributes access
// ----------------------------------------
HRESULT FCO::get_Attributes(IMgaAttributes **pVal) { 
		COMTRY {
			CheckRead();
			CHECK_OUTPTRPAR(pVal);

			CComPtr<IMgaMetaFCO> metao;					
			COMTHROW(get_Meta(&metao));

			CComPtr<IMgaMetaAttributes> metaas;					
			COMTHROW(metao->get_Attributes(&metaas));

			CREATECOLLECTION_FOR(IMgaAttribute,q);
			MGACOLL_ITERATE(IMgaMetaAttribute, metaas) {
					metaref_type r;	            				
					COMTHROW(MGACOLL_ITER->get_MetaRef(&r));
					q->Add(apool.getpoolobj(r, this, mgaproject));
			}
			MGACOLL_ITERATE_END;
			*pVal = q.Detach();

		} COMCATCH(;);
}

HRESULT FCO::get_Attribute(IMgaMetaAttribute *metaa,  IMgaAttribute **pVal) { 
		COMTRY {
			CheckRead();
			CHECK_OUTPTRPAR(pVal);
			CHECK_INPTRPAR(metaa);
			CComPtr<IMgaMetaFCO> metao;					
			COMTHROW(get_Meta(&metao));
			metaref_type r;	            				
			COMTHROW(metaa->get_MetaRef(&r));
#ifdef OWN_META
			CComPtr<IMgaMetaFCO> meta;				    
			COMTHROW(metaa->get_Owner(&meta));
			if(!meta.IsEqualObject(metao)) return E_META_INVALIDATTR;
#else
			{
			CComPtr<IMgaMetaFCOs> metas;				    
			COMTHROW(metaa->get_UsedIn(&metas));
			MGACOLL_ITERATE(IMgaMetaFCO,metas) {
				if(MGACOLL_ITER.IsEqualObject(metao)) break;
			}
			if(MGACOLL_AT_END) return E_META_INVALIDATTR;
			MGACOLL_ITERATE_END;
			}
#endif

			*pVal = apool.getpoolobj(r, this, mgaproject).Detach();
		} COMCATCH(;);
	};

// THROWS!!, always returns a valid attr.
CComPtr<IMgaAttribute> FCO::AbyN(BSTR name) {
		CHECK_INSTRPAR(name);
		CComPtr<IMgaMetaAttribute> metaattr;
		CComPtr<IMgaMetaFCO> meta;
		metaref_type r;
		COMTHROW(get_Meta(&meta));
		COMTHROW(meta->get_AttributeByName(name, &metaattr));
		COMTHROW(metaattr->get_MetaRef(&r));
		return apool.getpoolobj(r, this, mgaproject);
}

HRESULT FCO::get_AttributeByName(BSTR name, VARIANT *pVal) { 
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->get_Value(pVal));
		} COMCATCH(;);
};
HRESULT FCO::put_AttributeByName(BSTR name, VARIANT newVal) { 
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->put_Value(newVal));
		} COMCATCH(;);
}
HRESULT FCO::get_StrAttrByName( BSTR name,  BSTR *pVal) {  
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->get_StringValue(pVal));
		} COMCATCH(;);
}
HRESULT FCO::put_StrAttrByName( BSTR name,  BSTR newVal) {
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->put_StringValue(newVal));
		} COMCATCH(;);
}
HRESULT FCO::get_IntAttrByName( BSTR name,  long *pVal) { 
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->get_IntValue(pVal));
		} COMCATCH(;);
}
HRESULT FCO::put_IntAttrByName( BSTR name,  long newVal) {
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->put_IntValue(newVal));
		} COMCATCH(;);
}
HRESULT FCO::get_FloatAttrByName( BSTR name,  double *pVal) { 
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->get_FloatValue(pVal));
		} COMCATCH(;);
}
HRESULT FCO::put_FloatAttrByName( BSTR name,  double newVal) { 
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->put_FloatValue(newVal));
		} COMCATCH(;);
}
HRESULT FCO::get_BoolAttrByName( BSTR name,  VARIANT_BOOL *pVal) { 
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->get_BoolValue(pVal));
		} COMCATCH(;);
}
HRESULT FCO::put_BoolAttrByName( BSTR name,  VARIANT_BOOL newVal) {
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->put_BoolValue(newVal));
		} COMCATCH(;);
}
HRESULT FCO::get_RefAttrByName( BSTR name,  IMgaFCO **pVal) {
		COMTRY {
			CheckRead();
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->get_FCOValue(pVal));
		} COMCATCH(;);
}
HRESULT FCO::put_RefAttrByName( BSTR name,  IMgaFCO * newVal) {
		COMTRY {
			CheckRead();
			if(newVal) CHECK_MYINPTRPAR(newVal);
			// parameter will be checked by attr. fun
			COMTHROW(AbyN(name)->put_FCOValue(newVal));
		} COMCATCH(;);
}

HRESULT FCO::ClearAttrByName( BSTR name) { 
		COMTRY_IN_TRANSACTION {
			CheckWrite();
			COMTHROW(AbyN(name)->Clear());
		} COMCATCH_IN_TRANSACTION(;);
}

HRESULT FCO::get_RegistryNode( BSTR path,  IMgaRegNode **pVal) {  
	COMTRY {
		CheckRead();
		CHECK_INSTRPAR(path);
		CHECK_OUTPTRPAR(pVal);

		CComPtr<CMgaRegNode> s;
		CreateComObject(s);

		s->Initialize(path, this, mgaproject);

		CComPtr<IMgaRegNode> retval = s;

		*pVal = s.Detach();
	} COMCATCH(;)
}


// ----------------------------------------
// Registry access
// ----------------------------------------
HRESULT FCO::get_Registry(VARIANT_BOOL virtuals, IMgaRegNodes **pVal) {  
	COMTRY {  
		CheckRead();
		CHECK_OUTPTRPAR(pVal);

		CComPtr<CMgaRegNode> regnode;
		CreateComObject(regnode);

		regnode->Initialize(CComBSTR(L""), this, mgaproject);

		COMTHROW(regnode->get_SubNodes(virtuals, pVal));
	} COMCATCH(;);
}
HRESULT FCO::get_RegistryValue( BSTR path,  BSTR *pVal) {  
	COMTRY {
		CheckRead();
		CHECK_OUTVARIANTPAR(pVal);
		MgaRegNode_get_Value(mgaproject, this, self, path, pVal);
	} COMCATCH(;)
}
HRESULT FCO::put_RegistryValue( BSTR path,  BSTR newval) {  
	COMTRY {  // no arg check, called methods will do that
		CheckRead();  // put_Value will check write
		CComPtr<IMgaRegNode> node;
		COMTHROW(get_RegistryNode(path, &node));
		COMTHROW(node->put_Value(newval));
	} COMCATCH(;)
}

//-------------------------------------------------------------------------------------
// lph: Change description for ATTR, REGISTRY and PROPERTIES notifications

typedef std::vector<CComVariant> ModificationsVector;

void getRegistryModifications(CoreObj &cobj, CComBSTR &path, ModificationsVector &mv) {
	// TODO
}

HRESULT get_Modifications(FCO *fco, unsigned long changemask, CComVariant *mods) {
	COMTRY {
	ModificationsVector modifications;
	if (changemask & OBJEVENT_REGISTRY) {
		// TODO
	}
	if (changemask & OBJEVENT_ATTR) {
		CComPtr<IMgaMetaFCO> mfco;
		COMTHROW(fco->get_Meta(&mfco));
		ITERATE_THROUGH(fco->self[ATTRID_ATTRPARENT+ATTRID_COLLECTION]) {
			CComPtr<IMgaMetaAttribute> ma;
			COMTHROW(mfco->get_AttributeByRef(ITER[ATTRID_META], &ma));
			attval_enum vt;
			COMTHROW(ma->get_ValueType(&vt));
			if (vt == ATTVAL_ENUM) vt = ATTVAL_STRING;
			attrid_type aid = ATTRID_ATTRTYPESBASE + vt;
			CComVariant current = ITER[aid];
			static const VARTYPE vartypes[] = { VT_NULL, VT_BSTR, VT_I4, VT_R8, VT_BOOL, VT_DISPATCH, VT_BSTR, VT_NULL };
			if(vartypes[vt] != current.vt) {
				COMTHROW(current.ChangeType(vartypes[vt]));
			}
			CComVariant previous;
			COMTHROW(ITER->get_PreviousAttrValue(aid, &previous));
			if(vartypes[vt] != previous.vt) {
				COMTHROW(previous.ChangeType(vartypes[vt]));
			}
			if (previous != current) {
				CComBSTR name;
				COMTHROW(ma->get_Name(&name));
				CComBSTR label = "ATTR:";
				COMTHROW(label.Append(name));
				CComVariant ident = label;
				modifications.push_back(ident);
				modifications.push_back(previous);
			}
		}
	}
	if (changemask & OBJEVENT_PROPERTIES) {
		CComVariant name = fco->self[ATTRID_NAME];
		CComVariant pname;
		COMTHROW(fco->self->get_PreviousAttrValue(ATTRID_NAME, &pname));
		if (pname != name) {
			CComVariant ident = "PROPERTIES:Name";
			modifications.push_back(ident);
			modifications.push_back(pname);
		}
/* lph: possibly necessary, but not yet
		CComVariant perm = fco->self[ATTRID_PERMISSIONS];
		CComVariant pperm;
		COMTHROW(fco->self->get_PreviousAttrValue(ATTRID_PERMISSIONS, &pperm));
		if (pperm != perm) {
			CComVariant ident = "PROPERTIES:Permissions";
			modifications.push_back(ident);
			modifications.push_back(pperm);
		}
*/
	}
	if (modifications.size() > 0) {
		SAFEARRAY *pVariantsArray = NULL;
		SAFEARRAYBOUND rgsabound[1];
		rgsabound[0].lLbound = 0;
		rgsabound[0].cElements = (ULONG)modifications.size();
		pVariantsArray = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
		for (LONG i=0; i<LONG(modifications.size()); i++) {
			COMTHROW(SafeArrayPutElement(pVariantsArray, &i, &modifications[i]));
		}
		CComVariant varOut;
		varOut.vt = VT_ARRAY | VT_VARIANT;
		varOut.parray = pVariantsArray;
		varOut.Detach(mods);
	}
	} COMCATCH(;)
	return S_OK;
}
//-------------------------------------------------------------------------------------

HRESULT FCO::objrwnotify() {
	COMTRY {
			unsigned long chmask = this->changemask;
			this->changemask = 0;
			// No other notification on deleted objects
			if(chmask & OBJEVENT_DESTROYED) chmask = OBJEVENT_DESTROYED;
			CMgaProject::addoncoll::iterator ai, abeg = mgaproject->alladdons.begin(), aend = mgaproject->alladdons.end();
			if(abeg != aend) {
				CComVariant mods;
				if(!(chmask & OBJEVENT_CREATED) && (chmask & (OBJEVENT_REGISTRY+OBJEVENT_ATTR+OBJEVENT_PROPERTIES)))
					COMTHROW(get_Modifications(this, chmask, &mods));
				COMTHROW(mgaproject->pushterr(*mgaproject->reserveterr));
				for(ai = abeg; ai != aend; ) {
					CComPtr<CMgaAddOn> t = *ai++;	// it is important to incr ii here, while obj 
												    // still holds a reference to the (*ai) object
					unsigned long mmask;
					if((mmask = (t->eventmask & chmask)) != 0) {
						CComPtr<IMgaObject> tt;
						getinterface(&tt);

						try
						{
							t->handler->__ObjectEvent(tt, mmask, mods);
						}
						catch (_com_error& e)
						{
							if (t->progid.length() == 0)
								throw;
							_bstr_t error = e.Description();
							if (!error.length())
							{
								GetErrorInfo(e.Error(), error.GetAddress());
								if (error.length() == 0)
								{
									error = L"Unknown error";
								}
							}
							error = _bstr_t(L"Error in Addon '") + t->progid + _bstr_t("'. Addon returned error: ") + error;
							SetErrorInfo(error);
							_com_issue_errorex(e.Error(), t->handler, __uuidof(t->handler));
						}
					    t->notified = true;
					}
				}
				COMTHROW(mgaproject->popterr());
			}
			if(chmask & OBJEVENT_CREATED) {
				// send message to all territories that contain parent, 
				CoreObj parent = self[ATTRID_PARENT];
				// n.b. parent may be null if an addon deleted self and parent
				if (parent && parent.IsContainer()) {
					auto objforcore = ObjForCore(parent);
					FCO &p = *objforcore;
					
					// if parent is also new, notify it first
					if(p.notifymask & OBJEVENT_CREATED) COMTHROW(p.objnotify());

					pubfcohash::iterator ii, pbeg = p.pubfcos.begin(), pend = p.pubfcos.end();
					for(ii = pbeg; ii != pend;) {  
						CMgaTerritory *t = (*ii).second->territory;
						CComPtr<IMgaObject> obj = (*ii).second;
						++ii;  // it is important to incr ii here, while obj 
							   // still holds a reference to the (*ii) object
						CComVariant dummy;
						if(t->rwhandler && (t->eventmask & OBJEVENT_CREATED) != 0) {
							COMTHROW(mgaproject->pushterr(*t));
							{
								CComPtr<IMgaObject> newoo;
								getinterface(&newoo, t);
								COMTHROW(t->rwhandler->ObjectEvent(newoo, (unsigned long)OBJEVENT_CREATED, dummy));
							}
							COMTHROW(mgaproject->popterr());
							t->notified = true;
						}
					}
				}
				chmask &= ~OBJEVENT_CREATED;
			}
			pubfcohash::iterator ii, beg = pubfcos.begin(), end = pubfcos.end();
			for(ii = beg; ii != end;) {  
				CMgaTerritory *t = (*ii).second->territory;
				CComVariant &ud = (*ii).second->userdata;
				CComPtr<IMgaObject> obj = (*ii).second;
				++ii;  // it is important to incr ii here, while obj 
					   // still holds a reference to the (*ii) object
				long mmask;
				if(t->rwhandler && (mmask = (t->rweventmask & chmask)) != 0) {
					COMTHROW(mgaproject->pushterr(*t));
					COMTHROW(t->rwhandler->ObjectEvent(obj, mmask,ud));
					t->notified = true;
					COMTHROW(mgaproject->popterr());  // this may release the territory!!!
				}
			}
			this->temporalmask = 0; // may have been changed during notification (but we won't notify about it)
	} COMCATCH(;)
}





HRESULT FCO::objnotify() {
	if(notifymask == 0) return S_OK;
	COMTRY {
			unsigned long chmask = notifymask;
			notifymask = 0;
			// No other notification on deleted objects
			if(chmask & OBJEVENT_DESTROYED) chmask = OBJEVENT_DESTROYED;
			if(chmask & OBJEVENT_CREATED) {
				// send message to all territories that contain parent, 
				CoreObj parent = self[ATTRID_PARENT];
				if(parent.IsContainer()) {
					auto objforcore = ObjForCore(parent);
					FCO &p = *objforcore;
					
					// if parent is also new, notify it first
					if(p.notifymask & OBJEVENT_CREATED) COMTHROW(p.objnotify());

					pubfcohash::iterator ii, pbeg = p.pubfcos.begin(), pend = p.pubfcos.end();
					for(ii = pbeg; ii != pend;) {  
						CMgaTerritory *t = (*ii).second->territory;
						CComPtr<IMgaObject> obj = (*ii).second;
						++ii;  // it is important to incr ii here, while obj 
							   // still holds a reference to the (*ii) object
						CComVariant dummy;
						if(t->handler && (t->eventmask & OBJEVENT_CREATED) != 0) {
							COMTHROW(mgaproject->pushterr(*t));
							{
								CComPtr<IMgaObject> newoo;
								getinterface(&newoo, t);
								if(t->handler->ObjectEvent(newoo, (unsigned long)OBJEVENT_CREATED, dummy) != S_OK) {
									ASSERT(("Notification failed", false));
								}
							}
							COMTHROW(mgaproject->popterr());
							t->notified = true;
						}
					}
				}
				chmask &= ~OBJEVENT_CREATED;
			}
			pubfcohash::iterator ii, beg = pubfcos.begin(), end = pubfcos.end();
			for(ii = beg; ii != end;) {  
				CMgaTerritory *t = (*ii).second->territory;
				CComVariant &ud = (*ii).second->userdata;
				CComPtr<IMgaObject> obj = (*ii).second;
				++ii;  // it is important to incr ii here, while obj 
					   // still holds a reference to the (*ii) object
				unsigned long mmask;
				if(t->handler && (mmask = (t->eventmask & chmask)) != 0) {
					COMTHROW(mgaproject->pushterr(*t));
					if(t->handler->ObjectEvent(obj, mmask, ud) != S_OK) {
						// FIXME: allow addons to return constraint error, don't silently swallow error
						ASSERT(("Notification failed", false));
					}
					COMTHROW(mgaproject->popterr());
					t->notified = true;
				}
			}
			ASSERT(notifymask == 0);
	} COMCATCH(;)
}

void FCO::objforgetchange() {
	temporalmask = 0;
}

void FCO::objrecordchange() {
	ASSERT(temporalmask != 0);
	if(!changemask) {
		mgaproject->changedobjs.push(this);
	}	
	changemask |= temporalmask;
// 
	if(!notifymask) {
		mgaproject->notifyobjs.push(this);
	}	
	notifymask |= temporalmask;
	temporalmask = 0;
}


HRESULT FCO::SendEvent(long mask) {
	COMTRY_IN_TRANSACTION {
		CheckRead();
		if((mask | OBJEVENT_USERBITS) != OBJEVENT_USERBITS) COMTHROW(E_MGA_BAD_MASKVALUE);
		SelfMark(mask);
	} COMCATCH_IN_TRANSACTION(;)
}

void FCO::SelfMark(long newmask) {
	ASSERT(("Error: event generated in read-only transaction",!mgaproject->read_only));
	ASSERT(newmask);
	if(!temporalmask) {
		mgaproject->temporalobjs.push(this);
	}	
	temporalmask |= (unsigned long)newmask;
}

void FCO::objsetuserdata(CMgaTerritory *t, VARIANT udata) {
		pubfcohash::iterator i = pubfcos.find(t); 
		if(i == pubfcos.end()) COMTHROW(E_MGA_NOT_IN_TERRITORY);
		(*i).second->userdata = udata;
}

/*
HRESULT FCO::Associate(VARIANT userdata) {
  COMTRY {
	CheckDeletedRead();
	objsetuserdata(mgaproject->activeterr, userdata);
  } COMCATCH(;);
}

HRESULT FCO::get_CurrentAssociation(VARIANT *userdata) {
  COMTRY {
	CheckDeletedRead();
		pubfcohash::iterator i = pubfcos.find(mgaproject->activeterr);
		if(i == pubfcos.end()) COMTHROW(E_MGA_NOT_IN_TERRITORY);
		VariantCopy(userdata, &((*i).second->userdata)); 
   } COMCATCH(;);
}
*/

HRESULT FCO::Open(openmode mode)  { 
	return S_OK;
};

HRESULT FCO::Close() {
	return S_OK;
}

FCOPtr::FCOPtr(FCOPtr const &o) {
	p = o.p;
	p->AddRef();
}

FCOPtr::FCOPtr(FCOPtr&& o) {
	p = o.p;
	o.p = NULL;
}

FCOPtr::FCOPtr(FCO *f) {
	p = f;
	p->AddRef();
}

FCOPtr::FCOPtr() {
	p = NULL;
}

FCOPtr::~FCOPtr() {
	if(p) p->Release();
}

void FCOPtr::operator= (FCO *f) {	
	if(p == f) return;
	if(p) p->Release();
	p = f;
	p->AddRef();
}

bool FCOPtr::operator< (const FCOPtr &o) const {
	return p < o.p;
}

void CoreObjMark(CoreObj const &ob, long mask) {
	ObjForCore(ob)->SelfMark(mask);
}
	
HRESULT FCO::GetGuid(long* pl1, long* pl2, long* pl3, long* pl4)
{
	COMTRY {
		CheckRead();
		CHECK_OUTPAR( pl1);
		CHECK_OUTPAR( pl2);
		CHECK_OUTPAR( pl3);
		CHECK_OUTPAR( pl4);

		*pl1 = self[ATTRID_GUID1];
		*pl2 = self[ATTRID_GUID2];
		*pl3 = self[ATTRID_GUID3];
		*pl4 = self[ATTRID_GUID4];

	} COMCATCH(;)
}

HRESULT FCO::PutGuid( long l1, long l2, long l3, long l4)
{
	COMTRY_IN_TRANSACTION {
		CheckWrite();
		//ASSERT( self.IsFCO());
		//if( self.IsFCO()) {
		self[ATTRID_GUID1] = l1;
		self[ATTRID_GUID2] = l2;
		self[ATTRID_GUID3] = l3;
		self[ATTRID_GUID4] = l4;
	}  COMCATCH_IN_TRANSACTION(;)
}

HRESULT FCO::GetGuidDisp( BSTR *p_pGuidStr)
{
	// keep logic in sync with LibImgHelper::GetItsGuid() in MgaLibOps.cpp
	COMTRY {
		CheckRead();
		CHECK_OUTPAR( p_pGuidStr);
		long v1(0), v2(0), v3(0), v4(0);

		COMTHROW( this->GetGuid( &v1, &v2, &v3, &v4));

		GUID t_guid;
		t_guid.Data1 = v1;
		t_guid.Data2 = (v2 >> 16);
		t_guid.Data3 = v2 & 0xFFFF;
		t_guid.Data4[0] = (v3 >> 24);
		t_guid.Data4[1] = (v3 >> 16) & 0xFF;
		t_guid.Data4[2] = (v3 >> 8) & 0xFF;
		t_guid.Data4[3] = v3 & 0xFF;

		t_guid.Data4[4] = (v4 >> 24);
		t_guid.Data4[5] = (v4 >> 16) & 0xFF;
		t_guid.Data4[6] = (v4 >> 8) & 0xFF;
		t_guid.Data4[7] = v4 & 0xFF;

		char buff[39];
		sprintf( buff, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
			t_guid.Data1, t_guid.Data2, t_guid.Data3,
			t_guid.Data4[0], t_guid.Data4[1], t_guid.Data4[2], t_guid.Data4[3],
			t_guid.Data4[4], t_guid.Data4[5], t_guid.Data4[6], t_guid.Data4[7]);

		*p_pGuidStr = CComBSTR( buff).Detach();

	} COMCATCH(;)
}

HRESULT FCO::PutGuidDisp( BSTR p_guidStr)
{
	COMTRY_IN_TRANSACTION {
		CheckWrite();
		CHECK_INSTRPAR( p_guidStr);
		//self[ATTRID_GUID1] = newVal;
		GUID t_guid;
		CopyTo( p_guidStr, t_guid);

		long v1 = t_guid.Data1; // Data1: 32 b, Data2, Data 3: 16 b, Data4: 64 bit
		long v2 = (t_guid.Data2 << 16) + t_guid.Data3;
		long v3 = (((((t_guid.Data4[0] << 8) + t_guid.Data4[1]) << 8) + t_guid.Data4[2]) << 8) + t_guid.Data4[3];
		long v4 = (((((t_guid.Data4[4] << 8) + t_guid.Data4[5]) << 8) + t_guid.Data4[6]) << 8) + t_guid.Data4[7];

		PutGuid( v1, v2, v3, v4);
	}  COMCATCH_IN_TRANSACTION(;)
}

#ifdef _ATL_DEBUG_INTERFACES
bool IsQIThunk(IUnknown *p) {
	ATL::_QIThunk dummy((IUnknown*)(void*)1, L"dummy", IID_IUnknown, 0, false);

	return *((int**)(void*)p) == *((int**)(void*)&dummy);
}
#endif

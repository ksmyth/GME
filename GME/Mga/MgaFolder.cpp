// MgaFolder.cpp : Implementation of FCO Folder and container operations
#include "stdafx.h"
#include <map>
#include <stdio.h>
#include "MgaFCO.h"
#include "MgaLibOps.h" // by ZolMol
#include "MgaComplexOps.h" // by ZolMol

/////////////////////////////////////////////////////////////////////////////
// CMgaFolder


HRESULT FCO::get_MetaFolder(IMgaMetaFolder **pVal) { 
		COMTRY {
				CheckRead();
				CHECK_OUTPTRPAR(pVal);
				*pVal = CComQIPtr<IMgaMetaFolder>(mgaproject->FindMetaRef(self[ATTRID_META])).Detach();
		} COMCATCH(;)
}



HRESULT FCO::CreateFolder(IMgaMetaFolder *meta, IMgaFolder **nf)
{
	COMTRY_IN_TRANSACTION_MAYBE {
		CheckWrite();
		if(self[ATTRID_PERMISSIONS] & LIBROOT_FLAG) COMTHROW(E_MGA_NOT_CHANGEABLE);
		CHECK_INPTRPAR(meta);
		CHECK_OUTPTRPAR(nf);
		CheckWrite();
		CoreObj  subfolder;
		COMTHROW(mgaproject->dataproject->CreateObject(DTID_FOLDER,&subfolder.ComPtr()));
		assignGuid( mgaproject, subfolder);
		assignnewchild(subfolder);
		metaref_type mr;
		COMTHROW(meta->get_MetaRef(&mr));
		subfolder[ATTRID_META]=mr;
		auto nff = ObjForCore(subfolder);
		nff->initialname();
		COMTHROW(nff->Check());
		nff->SelfMark(OBJEVENT_CREATED);
		SelfMark(OBJEVENT_NEWCHILD);
		nff->getinterface(nf);
	}
	COMCATCH_IN_TRANSACTION_MAYBE(;);	
}


//
// by ZolMol
// called from CMgaFolder::CopyFolders
//
HRESULT FCO::CopyFolders(IMgaFolders *copylist, IMgaFolders **objs)
{
	COMTRY_IN_TRANSACTION {
		CheckWrite();
		CHECK_MYINPTRSPARFOLDER(copylist);
		long cnt;
		COMTHROW(copylist->get_Count(&cnt));

		metaid_type targettype = GetMetaID(self);

// Pre check:
		if(targettype == DTID_FOLDER)
		{
			if(self[ATTRID_PERMISSIONS] & ~EXEMPT_FLAG) 
				COMTHROW(E_MGA_NOT_CHANGEABLE);
		}
		else
			ASSERT(0); // where else can a a folder be copied?

// Copy trees
		coreobjpairhash crealist;
		int i = 0;

		std::vector<CoreObj> nobjs(cnt);
		MGACOLL_ITERATE(IMgaFolder, copylist) {
			CoreObj oldobj = CoreObj(MGACOLL_ITER);
			ObjForCore(oldobj)->SelfMark(OBJEVENT_COPIED);
			ObjTreeCopyFoldersToo(mgaproject, oldobj, nobjs[i], crealist);  // copy
			assignnewchild(nobjs[i]);
			
			CComPtr<IMgaMetaFolder> meta_of_copied_f; 
			COMTHROW( MGACOLL_ITER->get_MetaFolder( &meta_of_copied_f));

			if( !meta_of_copied_f) 
				COMTHROW(E_MGA_META_INCOMPATIBILITY);

			metaref_type copied_kind = METAREF_NULL;
			COMTHROW( meta_of_copied_f->get_MetaRef( &copied_kind));

			if( copied_kind == METAREF_NULL)
				COMTHROW(E_MGA_INVALID_ROLE); //E_MGA_INVALID_KIND ? instead
			
			(nobjs[i])[ATTRID_META] = copied_kind;

			i++;

		} MGACOLL_ITERATE_END;

// Reroute references
		for(i = 0; i< cnt; i++) {
			ObjTreeReconnectFoldersToo(nobjs[i], crealist);			
		}

		coreobjhash newcrealist;
		shiftlist(crealist, newcrealist);

		for(i = 0; i< cnt; i++) {
			ObjTreeCheckRelationsFoldersToo(mgaproject, nobjs[i], newcrealist);			
		}

		docheck(mgaproject);

// Assemble return array:
		if(objs) {
			CREATEEXCOLLECTION_FOR(MgaFolder, q);
			for(i = 0; i< cnt; i++) {
				CComPtr<IMgaFolder> ff;
				ObjForCore(nobjs[i])->getinterface(&ff);
				q->Add(ff); 
			}
			*objs = q.Detach();
		}

		SelfMark(OBJEVENT_NEWCHILD);

	} COMCATCH_IN_TRANSACTION(;);
}

HRESULT FCO::MoveFolders( IMgaFolders *movelist, IMgaFolders **objs)
{
	COMTRY_IN_TRANSACTION {
		CheckWrite();
		CHECK_MYINPTRSPARFOLDER(movelist);
		long cnt;
		COMTHROW(movelist->get_Count(&cnt));

		bool valid = (cnt > 0);
		if ( cnt == 1) // check whether the only selected folder is drag-and-dropped onto itself (*this == movelist[0])
		{
			valid = false;
			CComPtr<IMgaFolder> mf;
			MGACOLL_ITERATE( IMgaFolder, movelist) {
				mf = MGACOLL_ITER;
			} MGACOLL_ITERATE_END;

			VARIANT_BOOL is_equal;
			IMgaFolder * thisptr;
			getinterface( &thisptr);
			COMTHROW( mf->get_IsEqual( thisptr, &is_equal));

			if (is_equal == VARIANT_FALSE) // not equal
				valid = true;
		}
		
		if ( valid)
		{

			metaid_type targettype = GetMetaID(self);
			int targetlevel = 0;
			CoreObj rootp;

// Pre check:
			if(targettype == DTID_FOLDER) 
			{
				if(self[ATTRID_PERMISSIONS]  & ~EXEMPT_FLAG) 
					COMTHROW(E_MGA_NOT_CHANGEABLE);
			}
			else
				ASSERT(0); // where else can a a folder be moved?


// move trees
			coreobjhash moveslist; // will contain the movements which are needed to proceed
			int i = 0;

			std::vector<CoreObj> nobjs(cnt);
			std::vector<int> moved_into(cnt);  // >= 0 for objects moved inside the tree, -1 for newcomers
			MGACOLL_ITERATE(IMgaFolder, movelist) {

				CoreObj cur = nobjs[i] = CoreObj(MGACOLL_ITER);

				CoreObj curr_parent = cur[ATTRID_FCOPARENT];
				if( IsFolderContained( self, cur)) COMTHROW(E_MGA_INVALID_ARG);// do not allow moving a parent (grandparent...) into its child

				if( !COM_EQUAL( curr_parent, self)) {
					CoreObjMark( curr_parent, OBJEVENT_LOSTCHILD); // the old parent will lose its child
					assignnewchild( cur);
					CoreObjMark( cur, OBJEVENT_PARENT); // obj has been moved
				}

				ObjTreeCollectFoldersToo( mgaproject, cur, moveslist, CHK_MOVED);
				
				CComPtr<IMgaMetaFolder> meta_of_moved_f; 
				COMTHROW( MGACOLL_ITER->get_MetaFolder( &meta_of_moved_f));

				if( !meta_of_moved_f) 
					COMTHROW(E_MGA_META_INCOMPATIBILITY);

				metaref_type moved_kind = METAREF_NULL;
				COMTHROW( meta_of_moved_f->get_MetaRef( &moved_kind));

				if(moved_kind == METAREF_NULL)
					COMTHROW(E_MGA_INVALID_ROLE); //E_MGA_INVALID_KIND ? instead
				
				cur[ATTRID_META] = moved_kind;

				i++;
			} MGACOLL_ITERATE_END;

			for(i = 0; i< cnt; i++) {
				ObjTreeCheckRelationsFoldersToo(mgaproject, nobjs[i], moveslist);
			}

			DeriveMoveds(mgaproject, nobjs, moved_into, cnt, targetlevel);

// Reroute references
			for(i = 0; i< cnt; i++) {
				ObjTreeCheckINTORelationsFoldersToo(mgaproject, nobjs[i], moveslist);			
			}

			docheck(mgaproject); // this method check whether the operations are valid

// Assemble return array:
			CREATEEXCOLLECTION_FOR(MgaFolder, q);
			for(i = 0; i< cnt; i++) {
				CComPtr<IMgaFolder> n;
				ObjForCore(nobjs[i])->getinterface(&n);
				q->Add(n); 
			}
			
			if(objs) {
				*objs = q.Detach();
			}
			
			//SelfMark(OBJEVENT_NEWCHILD); - omitted, done by separate method: RefreshParent

		} // if valid

	} COMCATCH_IN_TRANSACTION(;);
}

HRESULT FCO::CopyFolderDisp(IMgaFolder *copyfold, IMgaFolder **nobj)
{
	COMTRY_IN_TRANSACTION {
		CComPtr<IMgaFolder> new_fol;

		// copy in param to a folder coll
		CREATEEXCOLLECTION_FOR(MgaFolder, q);
		q->Add( CComPtr<IMgaFolder>( copyfold));

		CComPtr<IMgaFolders> newfols;
		COMTHROW(CopyFolders( q, &newfols));
		
		// extract ret value from returned coll
		long cnt = 0; 
		if( newfols)  COMTHROW( newfols->get_Count( &cnt));
		if( cnt == 1) COMTHROW( newfols->get_Item( 1, &new_fol));
		
		if( nobj) {
			*nobj = new_fol.Detach();
		}
	} COMCATCH_IN_TRANSACTION(;);
}

HRESULT FCO::MoveFolderDisp(IMgaFolder *movefold, IMgaFolder **nobj)
{
	COMTRY_IN_TRANSACTION {
		CComPtr<IMgaFolder> new_fol;

		// copy in param to a folder coll
		CREATEEXCOLLECTION_FOR(MgaFolder, q);
		q->Add( CComPtr<IMgaFolder>( movefold));

		CComPtr<IMgaFolders> newfols;
		COMTHROW(MoveFolders( q, &newfols));
		
		// extract ret value from returned coll
		long cnt = 0; 
		if( newfols)  COMTHROW( newfols->get_Count( &cnt));
		if( cnt == 1) COMTHROW( newfols->get_Item( 1, &new_fol));
		
		if( nobj) {
			*nobj = new_fol.Detach();
		}
	} COMCATCH_IN_TRANSACTION(;);
}

HRESULT FCO::RefreshParent( IMgaFolder * folder)
{
	COMTRY_IN_TRANSACTION {
		SelfMark(OBJEVENT_NEWCHILD);
	} COMCATCH_IN_TRANSACTION(;);
}

HRESULT FCO::get_ChildFolders(IMgaFolders **pVal) {
	COMTRY {
		CheckRead();
		
		CoreObjs children = self[ATTRID_FPARENT+ATTRID_COLLECTION];

		CREATECOLLECTION_FOR(IMgaFolder,q)

		ITERATE_THROUGH(children) {
			  if(ITER.IsFCO()) continue;
			  CComPtr<IMgaFolder> ff;
			  ObjForCore(ITER)->getinterface(&ff);
			  q->Add(ff);
		}
		*pVal = q.Detach();
	}
	COMCATCH(;);	
}


HRESULT FCO::CreateRootObject(IMgaMetaFCO *meta, IMgaFCO **nobj) {
	
	COMTRY_IN_TRANSACTION {
		CHECK_INPTRPAR(meta);
		CHECK_OUTPTRPAR(nobj);
		CheckWrite();
		if(self[ATTRID_PERMISSIONS] & LIBROOT_FLAG) COMTHROW(E_MGA_NOT_CHANGEABLE);
		CoreObj newobj;
		COMTHROW(ContainerCreateFCO(meta, newobj));
		auto nfco = ObjForCore(newobj);
		nfco->initialname();
		COMTHROW(nfco->Check());
		nfco->SelfMark(OBJEVENT_CREATED);
		SelfMark(OBJEVENT_NEWCHILD);
		nfco->getinterface(nobj);
	}
	COMCATCH_IN_TRANSACTION(;)
}





///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// IMGACONTAINER ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////



// relid: 
// on create: next
// on copy: next
// on move: next (only if moving to another parent)
// on derive: base object + RELIDSPACE
// detach: relid change!!!!!
// attach: 

#define RELIDSPACEMASK	(RELIDSPACE-1)

void FCO::assignnewchild(CoreObj &ss) {
	ASSERT(RELIDSPACE_OVERFLOWED%RELIDSPACE == 0);
	ss[ATTRID_PARENT] = self;
	if(mgaproject->mgaversion < 1) return;
	ss[ATTRID_RELID] = 0;
	if(mgaproject->preferences & MGAPREF_MANUAL_RELIDS) return;
	long candidate = self[ATTRID_LASTRELID] + 1;
	if((unsigned long)candidate < RELIDSPACE) {		// simple case: no overflow yet.
			ss[ATTRID_RELID] = candidate;
			self[ATTRID_LASTRELID] = candidate;
			return;   
	}
	if(candidate == RELIDSPACE) {  // this is the first time an overflow happens
			// TODO: Notify user that an overflow happened
	}
	candidate &= RELIDSPACEMASK;
	int incr = 1;
	CoreObjs children = ss[ATTRID_PARENT+ATTRID_COLLECTION];
	int tries = 0;
	while(true) {
		for(int i = 0; i < 9; i++) {
			ITERATE_THROUGH(children) {
				if(ITER[ATTRID_RELID] == candidate) break;
			}
			if(!ITER_BROKEN) {
				ss[ATTRID_RELID] = candidate;
				self[ATTRID_LASTRELID] = long(candidate | RELIDSPACE_OVERFLOWED);  
			}
			if(tries++ > RELIDSPACE) COMTHROW(E_MGA_GEN_OUT_OF_SPACE);		// infinite search
			candidate += incr;
			candidate &= RELIDSPACEMASK;
		}
		incr *= 9;
	}
}
void traversechildren( CoreObj& ss)
{
	ASSERT( DTID_MODEL == GetMetaID(ss));
	CoreObjs children = ss[ATTRID_PARENT+ATTRID_COLLECTION];
	long candidate = ss[ATTRID_LASTRELID] + 1;
	ITERATE_THROUGH(children) {
		if((unsigned long)candidate < RELIDSPACE) {		// simple case: no overflow yet.
				
			CoreObj base = ITER[ATTRID_DERIVED];
			if( !base)
			{
				ITER[ATTRID_RELID] = candidate++;
				if( DTID_MODEL == GetMetaID(ITER))
					traversechildren( ITER );
			} // else : still derived, no need for relid assignment
		}
		else ASSERT(0);
	}
	ss[ATTRID_LASTRELID] = candidate-1;
}

void FCO::newrelidforchildren(CoreObj &prnt) {
	ASSERT(RELIDSPACE_OVERFLOWED%RELIDSPACE == 0);
	// not tested for the cases when mgaversion<1, or manual relids:
	if(mgaproject->mgaversion < 1) return;
	if(mgaproject->preferences & MGAPREF_MANUAL_RELIDS) return;

	// for all children of prnt if an object is not derived any more (it was a secondary derived)
	// then assign a new relid in [1, RELIDSPACE) interval, because secondary derived objects
	// had special relids (greater then RELIDSPACE) which now need to be modified back to regular relids
	long candidate = self[ATTRID_LASTRELID] + 1;
	if((unsigned long)candidate < RELIDSPACE) {		// simple case: no overflow yet.
		prnt[ATTRID_RELID] = candidate;
		self[ATTRID_LASTRELID] = candidate;
		if( DTID_MODEL == GetMetaID(prnt)) 
			traversechildren( prnt);
	}
}

HRESULT FCO::ContainerCreateFCO(IMgaMetaFCO *meta, CoreObj &fco) {
	COMTRY_IN_TRANSACTION_MAYBE {
		metaref_type rr;
		objtype_enum tt;
		COMTHROW(meta->get_MetaRef(&rr));
		COMTHROW(meta->get_ObjType(&tt));

		COMTHROW(mgaproject->dataproject->CreateObject(tt+DTID_BASE,&fco.ComPtr()));
		assignGuid( mgaproject, fco);
		assignnewchild(fco);
		fco[ATTRID_META]=rr;
	}
	COMCATCH_IN_TRANSACTION_MAYBE(;);	

}

HRESULT FCO::get_ChildRelIDCounter(long *pVal) { 
	COMTRY {
		CheckRead();
		ASSERT(self.IsContainer());
		CHECK_OUTPAR(pVal);
		*pVal = self[ATTRID_LASTRELID];
	} COMCATCH(;);
}	

HRESULT FCO::put_ChildRelIDCounter(long pVal) { 
	COMTRY_IN_TRANSACTION {
		CheckWrite();
		ASSERT(self.IsContainer());
		self[ATTRID_LASTRELID] = pVal;
	} COMCATCH_IN_TRANSACTION(;);
}	

HRESULT FCO::get_ChildFCOs(IMgaFCOs ** pVal) {
	COMTRY {
		CheckRead();

		CoreObjs children = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];

		CREATEEXCOLLECTION_FOR(MgaFCO,q);

		ITERATE_THROUGH(children) {
			  if(!ITER.IsFCO()) continue;
			  CComPtr<IMgaFCO> ff;
		      ObjForCore(ITER)->getinterface(&ff);
			  q->Add(ff);
		}
		*pVal = q.Detach();
	}
	COMCATCH(;);	
}

HRESULT FCO::get_ChildObjects(IMgaObjects ** pVal) {
	COMTRY {
		CHECK_OUTPTRPAR(pVal);
		CheckRead();

		CoreObjs children = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];

		CREATEEXCOLLECTION_FOR(MgaObject,q);

		ITERATE_THROUGH(children) {
			  CComPtr<IMgaObject> ff;
		      ObjForCore(ITER)->getinterface(&ff);
			  q->Add(ff);
		}
		*pVal = q.Detach();
	}
	COMCATCH(;);	
}

HRESULT FCO::get_ChildObjectByRelID(long relid, IMgaObject ** pVal) {
	COMTRY {
		CHECK_OUTPTRPAR(pVal);
		CheckRead();

		CoreObjs children = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];
		ITERATE_THROUGH(children) {
			if(ITER[ATTRID_RELID] == relid) {
		      ObjForCore(ITER)->getinterface(pVal);
			  break;
			}
		}
	}
	COMCATCH(;);	
}


// returns true if the short file names are equal
// i.e. for the following strings 'true' is given back
// MGA=C:\Program Files\gme\paradigms\SF\tmp2.mga
// MGA=F:\Files\gm2e\paradigms2\mySF\tMp2.mGa
// MGA=F:tMp2.MGA
// MGA=tMp2.MGA
bool libraryNameEqual( CComBSTR n1, CComBSTR n2)
{
	COMTHROW(n1.ToUpper());
	COMTHROW(n2.ToUpper());
	std::string s1, s2;
	CopyTo( n1, s1); CopyTo( n2, s2);

	// return false if both are not libraries
	if ( s1.find("MGA=") == std::string::npos || s2.find("MGA=") == std::string::npos) return false;

	int i1 = s1.length() - 1;
	while ( i1 >= 0 && s1[i1] != '\\' && s1[i1] != ':' && s1[i1] != '=')
		--i1; //int i1 = s1.rfind( '\\' || '=');
	int i2 = s2.length() - 1;
	while ( i2 >= 0 && s2[i2] != '\\' && s2[i2] != ':' && s2[i2] != '=')
		--i2; //int i2 = s2.rfind( '\\' || '=');
	
	if ( i1 >= 0) s1 = s1.substr( i1 + 1, s1.length() - i1 - 1); 
	if ( i2 >= 0) s2 = s2.substr( i2 + 1, s2.length() - i2 - 1);

	return s1.compare( s2) == 0;
}

void str_scan_name( OLECHAR ** p_ptr, int len_of, CComBSTR& p_name, CComBSTR& p_kind, CComBSTR& p_relpos)
{
	wchar_t str_to_find1[] = L"|kind=";
	wchar_t str_to_find2[] = L"|relpos=";

	OLECHAR * pi = *p_ptr;
	std::wstring id;

	while( len_of > 0 && *pi != L'/') // goes to the next separator
	{
		id += *pi;
		++pi;--len_of;
	}

	std::string::size_type f = id.find( str_to_find1);
	if( std::string::npos == f) // kind not found
	{
		p_name = id.c_str(); // only name is present
		id = L"";
	}
	else
	{
		p_name = id.substr( 0, f).c_str(); // f might be 0, so p_name might be ""
		id = id.substr( f + wcslen( str_to_find1));

		std::string::size_type k = id.find( str_to_find2);
		if( std::string::npos == k) // relid not found
		{
			p_kind = id.c_str(); // the remaining part is the kind
			id = L"";
		}
		else
		{
			p_kind = id.substr( 0, k).c_str();
			id = id.substr( k + wcslen( str_to_find2));
			
			// the remaining part is the relpos
			p_relpos = id.c_str();
		}
	}	

	*p_ptr = pi; // the processing will go on from this OLECHAR* value
}


// Delimiter in the path is the slash character: '/'
// searches based on name and kind: "/@MyFolder|kind=OneFolder/@MySubFolder|kind=AnotherFolder" (the project or root folder name must not be included)
//       or based on plain name:    "/MyFolder/MySubFolder"
//       or based on relid:         "/#1/#3"
// these may be mixed like:         "/@MyFolder|kind=OneFolder|relpos=1/#2" which means that look for MyFolder, then select its child with Relid = 2
// incoming path may contain 'misleading' relpos tokens as well: /@MyFolder|kind=OneFolder|relpos=1/@MySubFolder|kind=AnotherFolder|relpos=2" these are disregarded
// If several objects are found, NULL is returned
//
// FCO::get_ObjectByPath ignores the leading '/'
HRESULT FCO::get_ObjectByPath(BSTR path, IMgaObject ** pVal) {
	COMTRY {
		CHECK_OUTPTRPAR(pVal);
		CheckRead();

		OLECHAR *p = path;
		int lenOf = SysStringLen( path);

		if(*p == '/') p++;
		CComPtr<IMgaObject> pp;
		if(*p == '@') { // implemented by ZolMol
			p++;

			CComBSTR name_b, kind_b, relpos_b;
			name_b.Attach(SysAllocString(L""));//prepare for empty names, name_b.p is not 0 anymore
			if (name_b.m_str == NULL)
				COMTHROW(E_OUTOFMEMORY);
			OLECHAR * p2 = p;

			str_scan_name( &p2, lenOf + path - p2, name_b, kind_b, relpos_b); // 2nd parameter = the # of characters left
			ASSERT( name_b != 0);

			int relpos = -1;
			if (relpos_b.Length())
			{
				if (swscanf(static_cast<const wchar_t*>(relpos_b), L"%d", &relpos) != 1)
					COMTHROW(E_INVALIDARG);
			}

			bool found = false;
			bool conflict = false;
			int n_found = 0;
			CoreObjs children = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];
			ITERATE_THROUGH(children) {
				CComBSTR n = ITER[ATTRID_NAME];//if name is empty then n = "" and n.p != 0 that is why we added to name_b the "" value
				CComBSTR kind;
				COMTHROW( mgaproject->FindMetaRef( ITER[ATTRID_META])->get_Name( &kind));

				bool similar = n == name_b;
				similar = similar || ITER[ATTRID_PERMISSIONS] == LIBROOT_FLAG && libraryNameEqual(n, name_b);
				similar = similar && (kind_b == L"" || kind == kind_b);
				if (similar && (!found || relpos != -1))
				{
					if (relpos == -1 || relpos == n_found)
					{
						if ( *p2 != 0)
						{
							ObjForCore(ITER)->get_ObjectByPath( CComBSTR(p2), pVal);
						}
						else
						{
							ObjForCore(ITER)->getinterface( pVal);
						}

						if ( *pVal)
							found = true;
					}
					n_found++;
				}
				else if( similar && found) // found at least two objects with similar names at this level (samename siblings) and the first sibling contains the needed object already
				{
					if ( *p2 != 0)
					{
						CComObjPtr<IMgaObject> pdummyVal;
						ObjForCore(ITER)->get_ObjectByPath( CComBSTR(p2), PutOut(pdummyVal));
						if( pdummyVal)
						{
							//COMTHROW(); //identical name found at this level and down below to the bottom
							conflict = true;
							*pVal = 0;
						}

					}
					else
					{
						//COMTHROW(); //identical name found
						conflict = true;
						*pVal = 0;

					}
				}
			}
		}
		else if(*p == '#')
		{
			p++;
			long relid = wcstol(p,&p,0);
			COMTHROW(get_ChildObjectByRelID(relid, &pp));
			
			if(pp) {
				if(*p != 0) COMTHROW(pp->get_ObjectByPath( CComBSTR(p), pVal)); // corr by ZolMol
				else *pVal = pp.Detach();
			}
		}
		else // regular name
		{
			CComBSTR name_b;
			name_b.Attach(SysAllocString(L""));//prepare for empty names, name_b.p is not 0 anymore
			if (name_b.m_str == NULL)
				COMTHROW(E_OUTOFMEMORY);
			OLECHAR * p2 = p;

			str_scan_name( &p2, lenOf + path - p2, name_b, CComBSTR(), CComBSTR()); // disregard kind and relpos
			ASSERT( name_b != 0);

			bool found = false;
			bool conflict = false;
			CoreObjs children = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];
			ITERATE_THROUGH(children) {
				CComBSTR n = ITER[ATTRID_NAME];//if name is empty then n = "" and n.p != 0 that is why we added to name_b the "" value

				bool similar = n == name_b;
				if( similar && !found)
				{
					if ( *p2 != 0)
					{
						ObjForCore(ITER)->get_ObjectByPath( CComBSTR(p2), pVal);
					}
					else
					{
						ObjForCore(ITER)->getinterface( pVal);
					}

					if ( *pVal)
						found = true;
				}
				else if( similar && found) // found at least two objects with similar names at this level (samename syblings) and the first sibling contains the needed object already
				{
					if ( *p2 != 0)
					{
						CComObjPtr<IMgaObject> pdummyVal;
						ObjForCore(ITER)->get_ObjectByPath( CComBSTR(p2), PutOut(pdummyVal));
						if( pdummyVal)
						{
							//identical name found at this level and down below to the bottom
							conflict = true;
							*pVal = 0;
						}

					}
					else
					{
						//identical name found
						conflict = true;
						*pVal = 0;

					}
				}
			}
		}
	} COMCATCH(;);
}

// main delimiter in the path is the slash character: '/'
// searches based on name, kind and relpos: "/@MyFolder|kind=OneFolder/@MySubFolder|kind=AnotherFolder" (the project name must not be included)
// if several objects are found with the same name and kind then relpos decides among them (relpos <==> the order of creation)
// if relpos indicates larger number then the existing samename children then the one with the largest id (the youngest) is selected
HRESULT FCO::get_NthObjectByPath(long n_th, BSTR path, IMgaObject ** pVal) {
	COMTRY {
		CHECK_OUTPTRPAR(pVal);
		CheckRead();

		OLECHAR *p = path;
		int lenOf = SysStringLen( path);

		if(*p == '/') p++;
		CComPtr<IMgaObject> pp;
		if(*p == '@') { // implemented by ZolMol
			p++;

			CComBSTR name_b, kind_b, relpos_b;
			name_b.Attach(SysAllocString(L""));//prepare for empty names, name_b.p is not 0 anymore
			if (name_b.m_str == NULL)
				COMTHROW(E_OUTOFMEMORY);
			OLECHAR * p2 = p;

			str_scan_name( &p2, lenOf + path - p2, name_b, kind_b, relpos_b);
			ASSERT( name_b != 0);

			bool found = false;
			std::map< objid_type, std::vector<CoreObj> > samename_objs;
			CoreObjs children = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];
			ITERATE_THROUGH(children) {
				CComBSTR n = ITER[ATTRID_NAME];//if name is empty then n = "" and n.p != 0 that is why we added to name_b the "" value
				CComBSTR kind;
				COMTHROW( mgaproject->FindMetaRef( ITER[ATTRID_META])->get_Name( &kind));

				bool similar = n == name_b;
				similar = similar || ITER[ATTRID_PERMISSIONS] == LIBROOT_FLAG && libraryNameEqual(n, name_b);
				similar = similar && kind == kind_b;
				if( similar)
				{
					objid_type id = ITER.GetObjID();
					ASSERT( samename_objs.find( id) == samename_objs.end());

					samename_objs[ id].push_back( ITER);
				}
			}

			if( samename_objs.empty()) return S_OK;

#ifdef DEBUG
			std::map< objid_type, std::vector<CoreObj> >::iterator i = samename_objs.begin();
			std::map< objid_type, std::vector<CoreObj> >::iterator e = samename_objs.end();
			for( ; i != e; ++i)
			{
				ASSERT( i->second.size() == 1); // the id is unique, one element in the vector
			}
#endif

			std::string relpos_str;
			CopyTo( relpos_b, relpos_str);

			int relpos;
			if (sscanf( relpos_str.c_str(), "%d", &relpos) != 1)
				COMTHROW(E_INVALIDARG);

			// take from the samename_objs map the element at relpos relative position
			std::map< objid_type, std::vector<CoreObj> >::iterator ii = samename_objs.begin();
			std::map< objid_type, std::vector<CoreObj> >::iterator ee = samename_objs.end();
			for( int count = 0; count < relpos && ii != ee; ++ii)
				++count;
			
			CoreObj the_right_one;
			if( ii == ee) // no samename objects are present in such number 
			{
				std::map< objid_type, std::vector<CoreObj> >::reverse_iterator rev_i = samename_objs.rbegin();
				the_right_one = *(rev_i->second.begin());
			}
			else if( ii->second.size() > 0)
				the_right_one = *(ii->second.begin());


			// continue the search for the remaining part of the string: p2
			if ( *p2 != 0)
			{
				ObjForCore(the_right_one)->get_NthObjectByPath(n_th, CComBSTR(p2), pVal);
			}
			else
			{
				ObjForCore(the_right_one)->getinterface( pVal);
			}

			if ( *pVal)
				found = true;
		}
	} COMCATCH(;);
}

HRESULT FCO::get_ChildFCO(BSTR name, IMgaFCO ** pVal) {
	COMTRY {
		CHECK_INSTRPAR(name);
		CHECK_OUTPTRPAR(pVal);
		CheckRead();

		CoreObjs children = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];


		ITERATE_THROUGH(children) {
			  if(!ITER.IsFCO()) continue;
			  if(CComBSTR(ITER[ATTRID_NAME]) == name) {
				ObjForCore(ITER)->getinterface(pVal);
				break;
			  }
		}
	}
	COMCATCH(;);	
}






//should be namespace aware
HRESULT FCO::GetChildrenOfKind(BSTR kindname, IMgaFCOs ** pVal) {
	COMTRY {
		CheckRead();
		metaref_type tgood = 0;
		CComBSTR kindname_m = mgaproject->prefixWNmspc( kindname);

		CoreObjs children = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];

		CREATEEXCOLLECTION_FOR(MgaFCO,q);

		ITERATE_THROUGH(children) {
			  metaref_type t = CoreObj(ITER)[ATTRID_META];
			  if(!tgood) {
				  CComQIPtr<IMgaMetaFCO> meta = mgaproject->FindMetaRef(t);
				  ASSERT(meta);
				  CComBSTR str;
				  meta->get_Name(&str);
				  if( str == kindname || str == kindname_m) tgood = t;	
			  }	
			  if(t == tgood)  {
				  CComPtr<IMgaFCO> ff;
				  ObjForCore(ITER)->getinterface(&ff);
				  q->Add(ff);
			  }
		}
		*pVal = q.Detach();
	}
	COMCATCH(;);	
}



HRESULT FCO::ChangeObject(IMgaMetaRole *newrole, IMgaMetaFCO *kind) { 
		CoreObj selfsave = self;
		COMTRY_IN_TRANSACTION {
			CheckWrite();
			objtype_enum nt; 
			COMTHROW(kind->get_ObjType(&nt));
			metaid_type currenttype = self.GetMetaID(), newtype = nt + DTID_BASE + nt - OBJTYPE_NULL;
// no changing real types yet:
			CoreObjs subtypes = self[ATTRID_DERIVED+ATTRID_COLLECTION];
			if(self[ATTRID_DERIVED] != NULL || subtypes.Count()) COMTHROW(E_MGA_OP_REFUSED);

			if(currenttype != newtype) {
				static const attrid_type collids[DTID_SET-DTID_BASE+1] = { 
					0, ATTRID_FCOPARENT, 0, ATTRID_SEGREF, ATTRID_SETMEMBER, ATTRID_CONNROLE };
				attrid_type collid = collids[currenttype];
				if(collid) {
					CoreObjs coll = self[collid+ATTRID_COLLECTION];
					if(coll.Count()) COMTHROW(E_MGA_META_VIOLATION);
				}
// now it is sure that the Masterobj collection of the reference is empty
				if(currenttype == DTID_REFERENCE && self[ATTRID_REFERENCE] != NULL) {
					COMTHROW(E_MGA_META_VIOLATION);
				}							
			}

			CComPtr<IMgaMetaFCO> okind, newkind = kind;
			COMTHROW(get_Meta(&okind));
			if(!newkind) COMTHROW(newrole->get_Kind(&newkind));
			if(okind != newkind) {
				if(currenttype == DTID_MODEL) { 
					CoreObjs chds = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];
					if(chds.Count()) COMTHROW(E_MGA_META_VIOLATION);
				}
				CoreObjs attrs = self[ATTRID_ATTRPARENT+ATTRID_COLLECTION];
				CComPtr<IMgaMetaAttributes> newattrs;
				COMTHROW(newkind->get_Attributes(&newattrs));
				ITERATE_THROUGH(attrs) {
					CComPtr<IMgaMetaBase> ameta = mgaproject->FindMetaRef(ITER[ATTRID_META]);
					MGACOLL_ITERATE(IMgaMetaAttribute, newattrs) {
						if(COM_EQUAL(ameta, MGACOLL_ITER)) break;
					}  
					if(MGACOLL_AT_END) COMTHROW(E_MGA_META_VIOLATION);
					MGACOLL_ITERATE_END;
				}
			}

			if(currenttype != newtype) {
				CoreObj nnode;
				COMTHROW(mgaproject->dataproject->CreateObject(newtype, &nnode.ComPtr()));
				nnode[ATTRID_FCOPARENT] = self[ATTRID_FCOPARENT];
				self[ATTRID_FCOPARENT] = NULLCOREOBJ;
				nnode[ATTRID_NAME] = self[ATTRID_NAME];
				nnode[ATTRID_RELID] = self[ATTRID_RELID];
				nnode[ATTRID_DERIVED] = NULL;
				nnode[ATTRID_PERMISSIONS] = 0;
				{ ITERATE_THROUGH(self[ATTRID_CONSTROWNER+ATTRID_COLLECTION]) ITER[ATTRID_CONSTROWNER] = nnode; }
				// FIXME: should copy registry
				{ ITERATE_THROUGH(self[ATTRID_XREF+ATTRID_COLLECTION]) ITER[ATTRID_XREF] = nnode; }
				{ ITERATE_THROUGH(self[ATTRID_XREF+ATTRID_REFERENCE]) ITER[ATTRID_REFERENCE] = nnode; }
				COMTHROW(self->Delete());
/* this has to be fixed!!!! */ ASSERT(false);
#ifndef _ATL_DEBUG_INTERFACES
				self = nnode;
#endif
			}
			metaref_type mr;
			COMTHROW(newkind->get_MetaRef(&mr));
			self[ATTRID_META] = mr;
			if(newrole) 
			{
				COMTHROW(newrole->get_MetaRef(&mr));
			}
			else mr = METAREF_NULL;
			self[ATTRID_ROLEMETA] = mr;

#ifndef _ATL_DEBUG_INTERFACES
		} COMCATCH_IN_TRANSACTION(self = selfsave;);
#else
		} COMCATCH_IN_TRANSACTION(;);
#endif
}

HRESULT FCO::GetSourceControlInfo( long* p_scInfo)
{
	COMTRY
	{
		if( p_scInfo)
		{
			CheckRead();
			*p_scInfo = self[ATTRID_FILESTATUS];
		}
	}
	COMCATCH(;)
}

/////////////////////////////////////////////////////////////////////////////////
///////////////////////// ENUM REFERRENCES //////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

/* Only the real reference ones, does not work for XREFS */


typedef stdext::hash_set<CoreObj, coreobj_hashfunc> coreobjset;

void EnumRefs(CoreObj &self, coreobjset fcoset, EXCOLLECTIONTYPE_FOR(MgaFCO) *q) {
	metaid_type n = GetMetaID(self);
	if(n == DTID_MODEL) {
		CoreObjs children = self[ATTRID_FCOPARENT+ATTRID_COLLECTION];
		ITERATE_THROUGH(children) {
			EnumRefs(ITER,fcoset,q);
		}
	}
	CoreObjs refs = self[DTID_REFERENCE+ATTRID_COLLECTION];
	ITERATE_THROUGH(refs) {
		CoreObj cur = ITER;
		do {
			coreobjset::iterator i;
			if((i = fcoset.find(ITER)) != fcoset.end()) return;  // internal ref found
			cur = cur[ATTRID_FCOPARENT];
		} while(GetMetaID(cur) == DTID_MODEL);				
		CComPtr<IMgaReference> r;
		ObjForCore(ITER)->getinterface(&r);
		q->Add(r);
	}
}	

STDMETHODIMP CMgaProject::EnumExtReferences(IMgaFCOs *fcos, IMgaFCOs **pVal) {
	COMTRY {
		coreobjset fcoobjs;
		CHECK_MYINPTRSPAR(fcos);
		CHECK_OUTPTRPAR(pVal);
		CREATEEXCOLLECTION_FOR(MgaFCO,q);

		MGACOLL_ITERATE(IMgaFCO, fcos) {
			fcoobjs.insert(CoreObj(MGACOLL_ITER));
		} MGACOLL_ITERATE_END;

		MGACOLL_ITERATE(IMgaFCO, fcos) {
			EnumRefs(CoreObj(MGACOLL_ITER), fcoobjs, q);
		} MGACOLL_ITERATE_END;
		*pVal = q.Detach();
	} COMCATCH(;);
}


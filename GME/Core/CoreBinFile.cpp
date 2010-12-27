#include "stdafx.h"
#include "CoreBinFile.h"
#include "CommonCollection.h"
#include "..\Mga\MgaGeneric.h"


/*
	examples:

		MGA=D:\Data\metamodel.mga
*/

// --------------------------- BinAttr

BinAttrBase *BinAttrBase::Create(BinAttrBase& attr, valtype_type valtype)
{
	ASSERT( valtype != VALTYPE_NONE );

	BinAttrBase *binattr = NULL;

	switch(valtype)
	{
	case VALTYPE_LONG:
		binattr = new ((void*)(&attr)) BinAttr<VALTYPE_LONG>();
		break;

	case VALTYPE_STRING:
		binattr = new ((void*)(&attr)) BinAttr<VALTYPE_STRING>();
		break;

	case VALTYPE_BINARY:
		binattr = new ((void*)(&attr)) BinAttr<VALTYPE_BINARY>;
		break;

	case VALTYPE_LOCK:
		binattr = new ((void*)(&attr)) BinAttr<VALTYPE_LOCK>;
		break;

	case VALTYPE_POINTER:
		binattr = new ((void*)(&attr)) BinAttr<VALTYPE_POINTER>;
		break;

	case VALTYPE_COLLECTION:
		binattr = new ((void*)(&attr)) BinAttr<VALTYPE_COLLECTION>;
		break;

	case VALTYPE_REAL:
		binattr = new ((void*)(&attr)) BinAttr<VALTYPE_REAL>;
		break;

	default:
		HR_THROW(E_METAPROJECT);
	}

	// FIXME: can't take this if branch (is nothrow new intended?)
	if( binattr == NULL )
		HR_THROW(E_OUTOFMEMORY);

	return binattr;
};

// --------------------------- BinObject
void getMeAGuid( long *p_l1, long *p_l2, long *p_l3, long *p_l4)
{
	GUID t_guid = GUID_NULL;
	COMTHROW(CoCreateGuid(&t_guid));
		
	ASSERT(t_guid != GUID_NULL);
	//char buff[39];
	//sprintf( buff, "{%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}",
	//	t_guid.Data1, t_guid.Data2, t_guid.Data3,
	//	t_guid.Data4[0], t_guid.Data4[1], t_guid.Data4[2], t_guid.Data4[3],
	//	t_guid.Data4[4], t_guid.Data4[5], t_guid.Data4[6], t_guid.Data4[7]);

	// thus replace the old guid with a new one
	*p_l1 = t_guid.Data1; // Data1: 32 b, Data2, Data 3: 16 b, Data4: 64 bit
	*p_l2 = (t_guid.Data2 << 16) + t_guid.Data3;
	*p_l3 = (((((t_guid.Data4[0] << 8) + t_guid.Data4[1]) << 8) + t_guid.Data4[2]) << 8) + t_guid.Data4[3];
	*p_l4 = (((((t_guid.Data4[4] << 8) + t_guid.Data4[5]) << 8) + t_guid.Data4[6]) << 8) + t_guid.Data4[7];
}

bool BinObject::HasGuidAndStatAttributes( bool* p_guidFound, bool* p_statusFound)
{
	int a1( 0), a2( 0), a3( 0), a4( 0);

	binattrs_iterator i = binattrs.begin();
	binattrs_iterator e = binattrs.end();
	while( i != e)
	{
		switch( (i)->attrid)
		{
			case ATTRID_GUID1: ++a1;break;
			case ATTRID_GUID2: ++a2;break;
			case ATTRID_GUID3: ++a3;break;
			case ATTRID_GUID4: ++a4;break;
			case ATTRID_FILESTATUS: *p_statusFound = true; break;
		};

		++i;
	}

	// a1, a2, a3, a4 should be equal & have the 0 or 1 value
	ASSERT( (a1 == 0 || a1 == 1) && a1 == a2 && a1 == a3 && a1 == a4);

	*p_guidFound = a1 && a2 && a3 && a4;
	return *p_guidFound;
}

// this method will create Guid attributes for mga objects
// loaded from MGA files saved with a previous version of gme
void BinObject::CreateGuidAttributes( CCoreBinFile* p_bf)
{
	// create a new guid
	CComVariant l1, l2, l3, l4;
	l4.vt = l3.vt = l2.vt = l1.vt = VT_I4;
	getMeAGuid( &l1.lVal, &l2.lVal, &l3.lVal, &l4.lVal);

	// create BinAttrs of LONG type
	BinAttrUnion binattr1space; 
	BinAttrBase* binattr1 = BinAttrBase::Create(binattr1space, VALTYPE_LONG);
	BinAttrUnion binattr2space;
	BinAttrBase* binattr2 = BinAttrBase::Create(binattr2space, VALTYPE_LONG);
	BinAttrUnion binattr3space;
	BinAttrBase* binattr3 = BinAttrBase::Create(binattr3space, VALTYPE_LONG);
	BinAttrUnion binattr4space;
	BinAttrBase* binattr4 = BinAttrBase::Create(binattr4space, VALTYPE_LONG);


	// fill the only public field
	binattr1->attrid = ATTRID_GUID1;
	binattr2->attrid = ATTRID_GUID2;
	binattr3->attrid = ATTRID_GUID3;
	binattr4->attrid = ATTRID_GUID4;

	// set the values
	binattr1->Set( p_bf, l1);
	binattr2->Set( p_bf, l2);
	binattr3->Set( p_bf, l3);
	binattr4->Set( p_bf, l4);

	// insert the objects into the container
	// these objects will be destructed later 
	// by BinObject::DestroyAttributes
	binattrs.push_back(std::move(binattr1space));
	binattrs.push_back(std::move(binattr1space));
	binattrs.push_back(std::move(binattr1space));
	binattrs.push_back(std::move(binattr1space));
}

// this method will create a status attribute for mga objects
// loaded from MGA files saved with a previous version of gme
void BinObject::CreateStatusAttribute( CCoreBinFile* p_bf)
{
	// create BinAttr of LONG type
	BinAttrUnion binattrspace;
	BinAttrBase* binattr = BinAttrBase::Create(binattrspace, VALTYPE_LONG);

	// fill the only public field
	binattr->attrid = ATTRID_FILESTATUS;

	// set the value
	binattr->Set( p_bf, CComVariant( 0L));

	// insert the objects into the container
	// these objects will be destructed later 
	// by BinObject::DestroyAttributes
	binattrs.push_back(std::move(binattrspace));
}

void BinObject::CreateAttributes(ICoreMetaObject *metaobject)
{
	ASSERT( metaobject != NULL );
	ASSERT( binattrs.empty() );

	CComObjPtr<ICoreMetaAttributes> metaattributes;
	COMTHROW( metaobject->get_Attributes(PutOut(metaattributes)) );
	ASSERT( metaattributes != NULL );

	typedef std::vector< CComObjPtr<ICoreMetaAttribute> > metaattributelist_type;
	metaattributelist_type metaattributelist;
	GetAll<ICoreMetaAttributes, ICoreMetaAttribute>(metaattributes, metaattributelist);

	binattrs.reserve(metaattributelist.size());

	metaattributelist_type::iterator i = metaattributelist.begin();
	metaattributelist_type::iterator e = metaattributelist.end();
	while( i != e )
	{
		valtype_type valtype;
		COMTHROW( (*i)->get_ValueType(&valtype) );

		attrid_type attrid = ATTRID_NONE;
		COMTHROW( (*i)->get_AttrID(&attrid) );

		BinAttrUnion binattrspace;
		BinAttrBase *binattr = BinAttrBase::Create(binattrspace, valtype);
		BinAttrBase::Create(binattrspace, valtype);

		ASSERT( attrid != ATTRID_NONE );
		binattr->attrid = attrid;

		binattrs.push_back(std::move(binattrspace));

		++i;
	}
}

void BinObject::DestroyAttributes()
{
	binattrs.clear();
}

void BinObject::Read(CCoreBinFile *binfile)
{
	ASSERT( binfile != NULL );
	ASSERT( binattrs.empty() );

	valtype_type valtype;

	// First count how many attributes this object has, so we can intelligently size this->binattrs
	size_t num_attrs = 0;
	char* cifs_save = binfile->cifs;
	for (;;)
	{
		binfile->read(valtype);
		if( valtype == VALTYPE_NONE )
			break;
		num_attrs++;

		attrid_type attrid;
		binfile->read(attrid);

		// These need to be the same as CCoreBinFile::Read()s, but without the expense
		switch(valtype)
		{
		case VALTYPE_LONG:
			{ long x; binfile->read(x); }
			break;

		case VALTYPE_STRING:
			{ int len; binfile->read(len); binfile->cifs += len; } // FIXME maybe cifs > cifs_eof
			break;

		case VALTYPE_BINARY:
			{ int len; binfile->read(len); binfile->cifs += len; } // FIXME maybe cifs > cifs_eof
			break;

		case VALTYPE_LOCK:
			break;

		case VALTYPE_POINTER:
			{
				metaid_type metaid;
				binfile->read(metaid);
				if( metaid != METAID_NONE )
				{
					objid_type objid;
					binfile->read(objid);
				}
			}

		case VALTYPE_COLLECTION:
			break;

		case VALTYPE_REAL:
			{ double x; binfile->read(x); }
			break;

		default:
			HR_THROW(E_METAPROJECT);
		}
	}
	binfile->cifs = cifs_save;
	binattrs.reserve(num_attrs);

	for(;;)
	{
		binfile->read(valtype);
		if( valtype == VALTYPE_NONE )
			break;

		BinAttrUnion binattrspace;
		BinAttrBase *binattr = BinAttrBase::Create(binattrspace, valtype);
		ASSERT( binattr != NULL );

		attrid_type attrid;
		binfile->read(attrid);
		ASSERT( attrid != ATTRID_NONE );

		binattr->attrid = attrid;

		// Possible pitfall: binattr == &binattrspace. It is possible the compiler will figure this out, and call BinAttrUnion::Read() (which we don't want)
		binattr->Read(binfile);

		// TODO: this move could be avoided
		binattrs.push_back(std::move(binattrspace));
	}
};

void BinObject::Write(CCoreBinFile *binfile)
{
	ASSERT( binfile != NULL );
	ASSERT( !deleted );

	binattrs_iterator i = binattrs.begin();
	binattrs_iterator e = binattrs.end();
	while( i != e )
	{
		ASSERT( (i)->GetValType() != VALTYPE_NONE );
		ASSERT( (i)->attrid != ATTRID_NONE );

		binfile->write( (i)->GetValType() );
		binfile->write( (i)->attrid );
		(i)->Write(binfile);

		++i;
	}

	binfile->write((valtype_type)VALTYPE_NONE);
}

bool BinObject::HasEmptyPointers() const
{
	if( deleted )
		return true;

	binattrs_type::const_iterator i = binattrs.begin();
	binattrs_type::const_iterator e = binattrs.end();
	while( i != e )
	{
		if( (i)->GetValType() == VALTYPE_POINTER )
		{
			if( !( ( ( BinAttr<VALTYPE_POINTER>*)(&*i))->isEmpty))
				return false;
		}
		++i;
	}

	return true;
}

// --------------------------- CCoreBinFile

CCoreBinFile::CCoreBinFile()
{
	metaid = METAID_NONE;
	attrid = ATTRID_NONE;
	intrans = false;
	modified = false;
	isEmpty = true;
}

CCoreBinFile::~CCoreBinFile()
{
}

// ------- MetaProject

STDMETHODIMP CCoreBinFile::get_MetaProject(ICoreMetaProject **p)
{
	CHECK_OUT(p);

	CopyTo(metaproject, p);

	return S_OK;
}

STDMETHODIMP CCoreBinFile::put_MetaProject(ICoreMetaProject *p)
{
	COMTRY
	{
		CloseMetaProject();

		metaproject = p;

		if( metaproject != NULL )
			OpenMetaProject();
	}
	COMCATCH( CloseMetaProject() )
}

void CCoreBinFile::OpenMetaProject()
{
	ASSERT( metaprojectid.empty() );
	ASSERT( metaproject != NULL );

	CComVariant tmp;
	metaproject->get_GUID(PutOut(tmp));
	CopyTo(tmp, metaprojectid);
}

void CCoreBinFile::CloseMetaProject()
{
	if( IsOpened() && metaproject != NULL )
		CloseProject();

	CloseMetaObject();

	metaproject = NULL;
	metaprojectid.clear();
}

// ------- MetaObject

STDMETHODIMP CCoreBinFile::get_MetaObject(ICoreMetaObject **p)
{
	CHECK_OUT(p);

	CopyTo(metaobject, p);

	return S_OK;
}

STDMETHODIMP CCoreBinFile::put_MetaObject(ICoreMetaObject *p)
{
	if( metaproject == NULL )
		COMRETURN(E_INVALID_USAGE);

	if( metaobject == p )
		return S_OK;

	COMTRY
	{
		if( p != NULL )
		{
			CComObjPtr<ICoreMetaProject> t;
			COMTHROW( p->get_Project(PutOut(t)) );
			if( !IsEqualObject(metaproject, t) )
				HR_THROW(E_SAMEPROJECT);
		}

		CloseMetaObject();
		metaobject = p;
		if( metaobject != NULL )
			OpenMetaObject();
	}
	COMCATCH( CloseMetaObject() )
}

STDMETHODIMP CCoreBinFile::get_MetaID(metaid_type *p)
{
	CHECK_OUT(p);

	*p = metaid;

	return S_OK;
}

STDMETHODIMP CCoreBinFile::put_MetaID(metaid_type metaid)
{
	if( metaproject == NULL )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		if( metaid != METAID_NONE )
		{
			CComObjPtr<ICoreMetaObject> p;
			COMTHROW( metaproject->get_Object(metaid, PutOut(p)) );
			ASSERT( p != NULL );

			if( metaobject != p )
			{
				CloseMetaObject();
				MoveTo(p, metaobject);
				OpenMetaObject();
			}
		}
		else
			CloseMetaObject();
	}
	COMCATCH( CloseMetaObject() )
}

void CCoreBinFile::OpenMetaObject()
{
	ASSERT( metaobject != NULL );

	COMTHROW( metaobject->get_MetaID(&metaid) );
}

void CCoreBinFile::CloseMetaObject()
{
	CloseObject();
	CloseMetaAttribute();
	metaobject = NULL;
	metaid = METAID_NONE;
}

// ------- MetaAttribute

STDMETHODIMP CCoreBinFile::get_MetaAttribute(ICoreMetaAttribute **p)
{
	CHECK_OUT(p);

	CopyTo(metaattribute, p);

	return S_OK;
}

STDMETHODIMP CCoreBinFile::put_MetaAttribute(ICoreMetaAttribute *p)
{
	if( metaobject == NULL )
		COMRETURN(E_INVALID_USAGE);
	ASSERT( metaproject != NULL );

	if( metaattribute == p )
		return S_OK;

	COMTRY
	{
		if( metaattribute != NULL )
		{
			CComObjPtr<ICoreMetaObject> t;
			COMTHROW( metaattribute->get_Object(PutOut(t)) );
			if( !IsEqualObject(metaobject, t) )
			{
				metaattribute = NULL;
				return E_INVALIDARG;
			}
		}

		CloseMetaAttribute();
		metaattribute = p;
		if( metaattribute != NULL )
			OpenMetaAttribute();
	}
	COMCATCH( CloseMetaAttribute() )
}

STDMETHODIMP CCoreBinFile::get_AttrID(attrid_type *p)
{
	CHECK_OUT(p);

	if( metaattribute )
		return metaattribute->get_AttrID(p);

	*p = 0;
	return S_OK;
}

STDMETHODIMP CCoreBinFile::put_AttrID(attrid_type attrid)
{
	if( metaobject == NULL )
		COMRETURN(E_INVALID_USAGE);
	ASSERT( metaproject != NULL );

	COMTRY
	{
		if( attrid != ATTRID_NONE )
		{
			CComObjPtr<ICoreMetaAttribute> p;
			COMTHROW( metaobject->get_Attribute(attrid, PutOut(p)) );
			ASSERT( p != NULL );

			if( metaattribute != p )
			{
				CloseMetaAttribute();
				metaattribute = p;
				OpenMetaAttribute();
			}
		}
		else
			CloseMetaAttribute();
	}
	COMCATCH( CloseMetaAttribute() )
}

void CCoreBinFile::OpenMetaAttribute()
{
	ASSERT( metaattribute != NULL );

	COMTHROW( metaattribute->get_AttrID(&attrid) );

	if( attrid == ATTRID_NONE )
		HR_THROW(E_METAPROJECT);
}

void CCoreBinFile::CloseMetaAttribute()
{
	metaattribute = NULL;
	attrid = ATTRID_NONE;
}

// ------- Ios

void CCoreBinFile::read(bindata &b)
{
	int len;
	
	read(len);
	ASSERT( len >= 0 );

	try {
		b.resize(len);
	} catch (std::bad_alloc&) {
		// KMS: could get here if the project is corrupt and len is incorrect
		COMTHROW(E_OUTOFMEMORY);
	}
	if( len > 0 ) {
		if (len > cifs_eof - cifs) {
			HR_THROW(E_FILEOPEN);
		}
		memcpy(&b[0], cifs, len);
		cifs += len;
	}
}

void CCoreBinFile::read(CComBstrObj &ss)
{
	std::string s;

	int len;

	read(len);
	ASSERT( len >= 0 );

	s.resize(len);
	if( len > 0 ) {
		if (len > cifs_eof - cifs) {
			HR_THROW(E_FILEOPEN);
		}
		// FIXME: why copy into std::string at all?
		memcpy(&s[0], cifs, len);
		cifs += len;
	}


	CopyTo(s, ss);
}

void CCoreBinFile::write(const bindata &b)
{
	ASSERT( ofs.is_open() );

	int len = b.size();
	ASSERT( len >= 0 );
	
	write(len);

	if( len > 0 )
		ofs.write( (const char *) &b[0], len);
}

void CCoreBinFile::write(const CComBstrObj &ss)
{
	ASSERT( ofs.is_open() );

	std::string s;
	CopyTo(ss, s);

	int len = s.size();
	ASSERT( len >= 0 );
	
	write(len);

	if( len > 0 )
		ofs.write( (const char *) &s[0], len);
}

// ------- Attribute

STDMETHODIMP CCoreBinFile::get_AttributeValue(VARIANT *p)
{
	CHECK_OUT(p);

	if( isEmpty || !InTransaction())
		COMRETURN(E_INVALID_USAGE);
	
	COMTRY
	{
		BinAttrBase *attr = opened_object->second.Find(attrid);
		ASSERT( attr != NULL );

		attr->Get(this, p);
	}
	COMCATCH(;)
}

STDMETHODIMP CCoreBinFile::put_AttributeValue(VARIANT p)
{
	if( isEmpty || !InTransaction())
		COMRETURN(E_INVALID_USAGE);
	
	COMTRY
	{
		BinAttrBase *attr = opened_object->second.Find(attrid);

	/*	std::pair<BinAttrBase *const, CComVariant> r(attr, CComVariant());
		std::pair<undos_iterator, bool> t = undos.insert(r);

		if( t.second )
			attr->Get(this, PutOut(t.first->second));
			*/
		attr->Set(this, p);
	}
	COMCATCH(;)
}

// ------- Object

void CCoreBinFile::InitMaxObjIDs()
{
	ASSERT( metaproject != NULL );

	maxobjids.clear();

	CComObjPtr<ICoreMetaObjects> metaobjects;
	COMTHROW( metaproject->get_Objects(PutOut(metaobjects)) );
	ASSERT( metaobjects != NULL );

	typedef std::vector< CComObjPtr<ICoreMetaObject> > metaobjectlist_type;
	metaobjectlist_type metaobjectlist;
	GetAll<ICoreMetaObjects, ICoreMetaObject>(metaobjects, metaobjectlist);

	metaobjectlist_type::iterator i = metaobjectlist.begin();
	metaobjectlist_type::iterator e = metaobjectlist.end();
	while( i != e )
	{
		ASSERT( *i != NULL );

		metaid_type metaid = METAID_NONE;
		COMTHROW( (*i)->get_MetaID(&metaid) );
		ASSERT( metaid != METAID_NONE );

		ASSERT( maxobjids.find(metaid) == maxobjids.end() );
		maxobjids.insert( std::pair<const metaid_type, objid_type>(metaid, OBJID_NONE) );

		++i;
	}
}

STDMETHODIMP CCoreBinFile::OpenObject(objid_type objid)
{
	if( metaobject == NULL || !InTransaction() )
		COMRETURN(E_INVALID_USAGE);

	ASSERT( metaid != METAID_NONE );

	metaobjidpair_type idpair;
	idpair.metaid = metaid;
	idpair.objid = objid;

	if( !isEmpty &&
		metaobjidpair_equalkey()(opened_object->first, idpair) )
		return S_OK;

	COMTRY
	{
		opened_object = objects.find(idpair);
		isEmpty = false;
		if( (opened_object == objects.end()) ||
			opened_object->second.deleted )
		{
			isEmpty = true;
			HR_THROW(E_NOTFOUND);
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CCoreBinFile::CreateObject(objid_type *objid)
{
	CHECK_OUT(objid);

	if( metaobject == NULL || !InTransaction() )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		modified = true;

		isEmpty = true;

		ASSERT( metaid != METAID_NONE );

		maxobjids_iterator i = maxobjids.find(metaid);
		ASSERT( i != maxobjids.end() );
		
		metaobjidpair_type idpair;
		idpair.metaid = metaid;
		idpair.objid = ++(i->second);

		ASSERT( metaobject != NULL );

		std::pair<objects_iterator, bool> t = objects.insert(
			objects_type::value_type(idpair,BinObject()));
		ASSERT( t.second );

		t.first->second.deleted = false;
		t.first->second.CreateAttributes(metaobject);

		created_objects.push_front(t.first);

		opened_object = t.first;
		isEmpty = false;
		*objid = idpair.objid;
	}
	COMCATCH(;)
}

STDMETHODIMP CCoreBinFile::CloseObject()
{
	isEmpty =  true;
	return S_OK;
}

STDMETHODIMP CCoreBinFile::LockObject()
{
	if( isEmpty || !InTransaction())
		COMRETURN(E_INVALID_USAGE);

	return S_OK;
}

STDMETHODIMP CCoreBinFile::DeleteObject()
{
	if( isEmpty || !InTransaction())
		COMRETURN(E_INVALID_USAGE);

	ASSERT( metaobject != NULL );

	ASSERT( opened_object->second.deleted == false );

	modified = true;

	deleted_objects.push_front(opened_object);

	opened_object->second.deleted = true;
	isEmpty = true;

	return S_OK;
}

// ------- Project

void CCoreBinFile::CancelProject()
{
	CloseMetaObject();

	cifs = 0;
	cifs_eof = 0;

	if( ofs.is_open() )
		ofs.close();

	filename.clear();
	intrans = false;
	modified = false;

	isEmpty = true;
	deleted_objects.clear();
	created_objects.clear();
	maxobjids.clear();
	resolvelist.clear();
	objects.clear();
}

void CCoreBinFile::SaveProject()
{
	ASSERT( !ofs.is_open() );
	ASSERT( metaprojectid.size() == 16 );

	ofs.clear();
	ofs.open(filename.c_str(), std::ios::out | std::ios::binary);
	if( ofs.fail() || !ofs.is_open() ) {
		ofs.close();
		ofs.clear();
		HR_THROW(E_FILEOPEN);
	}

	write(metaprojectid);

	objects_iterator i = objects.begin();
	objects_iterator e = objects.end();
	while( i != e )
	{
		ASSERT( (*i).first.metaid != METAID_NONE );
		ASSERT( (*i).first.objid != OBJID_NONE );

		if( ((*i).first.metaid == METAID_ROOT && (*i).first.objid == OBJID_ROOT)
			|| !(*i).second.HasEmptyPointers() )
		{
			write( (metaid_type)(*i).first.metaid );
			write( (objid_type)(*i).first.objid );

			(*i).second.Write(this);
		}

		++i;
	}

	write((metaid_type)METAID_NONE);

	if( ofs.fail() )
		HR_THROW(E_FILEOPEN);

	ofs.close();
}

void CCoreBinFile::LoadProject()
{
	InitMaxObjIDs();

	{
	membuf file_buffer;
	if (file_buffer.open(filename.c_str()) != 0) {
		HR_THROW(HRESULT_FROM_WIN32(GetLastError()));
	}
	cifs = file_buffer.getBegin();
	cifs_eof = file_buffer.getEnd();

	bindata guid;
	read(guid);

	if( !(guid == metaprojectid) )
		HR_THROW(E_PROJECT_MISMATCH);

	ASSERT( resolvelist.empty() );

	for(;;)
	{
		metaid_type metaid;
		read(metaid);
		if( metaid == METAID_NONE )
			break;

		objid_type objid;
		read(objid);
		ASSERT( objid != OBJID_NONE );

		maxobjids_iterator i = maxobjids.find(metaid);
		ASSERT( i != maxobjids.end() );

		if( i->second < objid )
			i->second = objid;

		metaobjidpair_type idpair;
		idpair.metaid = metaid;
		idpair.objid = objid;

		std::pair<objects_iterator, bool> t = objects.insert(
			objects_type::value_type(idpair,BinObject()));
		ASSERT( t.second );

		opened_object = t.first;
		isEmpty = false;
		opened_object->second.deleted = false;
		opened_object->second.Read(this);
		
		// if the object read is folder or fco and it does NOT have guid attributes (old version mga file)
		if( metaid >= DTID_MODEL && metaid <= DTID_FOLDER)	// 101 .. 106
		{
			bool stat_found( false), guid_found( false);

			opened_object->second.HasGuidAndStatAttributes( &guid_found, &stat_found);

			if( !guid_found) // we will create guid attributes for it
				opened_object->second.CreateGuidAttributes( this);

			if( !stat_found && ( metaid == DTID_MODEL || metaid == DTID_FOLDER)) // we will create status attribute for M and F
				opened_object->second.CreateStatusAttribute( this);
		}
	}

	resolvelist_type::iterator i = resolvelist.begin();
	resolvelist_type::iterator e = resolvelist.end();
	while( i != e )
	{
		opened_object = i->obj;
		isEmpty = false;
		
		// ASSERT( !isEmpty ); 

		BinAttrBase *base = opened_object->second.Find(i->attrid);
		ASSERT( base != NULL );

		ASSERT( base->GetValType() == VALTYPE_POINTER );

		objects_iterator j = objects.find( i->idpair );
		if( j == objects.end() )
			HR_THROW(E_BINFILE);

		((BinAttr<VALTYPE_POINTER>*)base)->Set(this, j);

		++i;
	}

	isEmpty = true;
	resolvelist.clear();
	}

	ofs.clear();
	ofs.open(filename.c_str(), std::ios::app | std::ios::binary);
	read_only = false;
	if( ofs.fail() || !ofs.is_open() )	read_only = true;
	else ofs.close();
}

STDMETHODIMP CCoreBinFile::OpenProject(BSTR connection, VARIANT_BOOL *ro_mode) {
	if( IsOpened() || metaproject == NULL )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		CopyTo(connection, filename);
		if( !(std::string(filename, 0, 4) == "MGA=") )
			HR_THROW(E_INVALID_USAGE);

		filename.erase(0, 4);

		LoadProject();


		modified = false;
		if(ro_mode) *ro_mode = read_only ? VARIANT_TRUE : VARIANT_FALSE;

		ASSERT( IsOpened() );
	}
	COMCATCH( CancelProject(); )
}

STDMETHODIMP CCoreBinFile::CreateProject(BSTR connection)
{
	if( IsOpened() || metaproject == NULL )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		CopyTo(connection, filename);
		if( !(std::string(filename, 0, 4) == "MGA=") )
			HR_THROW(E_INVALID_USAGE);

		filename.erase(0, 4);

		InitMaxObjIDs();

		CComObjPtr<ICoreMetaObject> mo;
		COMTHROW( metaproject->get_Object(METAID_ROOT, PutOut(mo)) );
		ASSERT( mo != NULL );

		maxobjids_iterator i = maxobjids.find(METAID_ROOT);
		i->second = OBJID_ROOT;
		
		metaobjidpair_type idpair;
		idpair.metaid = METAID_ROOT;
		idpair.objid = OBJID_ROOT;

		std::pair<objects_iterator, bool> t = objects.insert(
			objects_type::value_type(idpair,BinObject()));
		ASSERT( t.second );

		t.first->second.deleted = false;
		t.first->second.CreateAttributes(mo);

		if(filename.empty()) filename = ".";
		else SaveProject();

		modified = false;

		ASSERT( IsOpened() );
	}
	COMCATCH( CancelProject(); )
}

STDMETHODIMP CCoreBinFile::SaveProject(BSTR connection, VARIANT_BOOL keepoldname = VARIANT_TRUE) 
{
	std::string origfname = filename;
	COMTRY
	{
		std::string fn;
		CopyTo(connection, fn);

		if( !fn.empty() ) 
		{
			if( !(std::string(fn, 0, 4) == "MGA=") )
				HR_THROW(E_INVALID_USAGE);

			fn.erase(0, 4);
			filename = fn;
			if(filename.empty()) filename = ".";
		}
		if(filename == ".") COMTHROW(E_NAMEMISSING);
		SaveProject();
		if(keepoldname == VARIANT_TRUE) filename = origfname;
	}
	COMCATCH( filename = origfname;)
}

STDMETHODIMP CCoreBinFile::CloseProject( VARIANT_BOOL abort) {
	if( !IsOpened() || metaproject == NULL )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		if( abort == VARIANT_FALSE && modified && !(filename == ".")) 
			SaveProject(NULL);

		ASSERT( IsOpened() );

		CancelProject();
	}
	COMCATCH(;)
}

STDMETHODIMP CCoreBinFile::BeginTransaction()
{
	if( !IsOpened() || InTransaction() )
		COMRETURN(E_INVALID_USAGE);

	ASSERT( deleted_objects.empty() );
	ASSERT( created_objects.empty() );

	intrans = true;

	return S_OK;
}

STDMETHODIMP CCoreBinFile::CommitTransaction()
{
	if( !InTransaction() )
		COMRETURN(E_INVALID_USAGE);

	ASSERT( IsOpened() );

	CloseObject();

	std::list<objects_iterator>::iterator i = deleted_objects.begin();//slist
	std::list<objects_iterator>::iterator e = deleted_objects.end();//slist
	while( i != e )
	{
		ASSERT( (*i)->second.deleted );

		objects.erase(*i);

		++i;
	}

	undos.clear();
	deleted_objects.clear();
	created_objects.clear();

	intrans = false;

	return S_OK;
}

STDMETHODIMP CCoreBinFile::AbortTransaction()
{
	if( !InTransaction() )
		COMRETURN(E_INVALID_USAGE);

	ASSERT( IsOpened() );

	CloseObject();

	undos_iterator j = undos.begin();
	undos_iterator f = undos.end();
	while( j != f )
	{
		j->first->Set(this, j->second);

		++j;
	}

	std::list<objects_iterator>::iterator i = deleted_objects.begin();//slist
	std::list<objects_iterator>::iterator e = deleted_objects.end();//slist
	while( i != e )
	{
		ASSERT( (*i)->second.deleted );

		(*i)->second.deleted = false;

		++i;
	}

	i = created_objects.begin();
	e = created_objects.end();
	while( i != e )
	{
		ASSERT( !(*i)->second.deleted );

		objects.erase(*i);

		++i;
	}

	undos.clear();
	deleted_objects.clear();
	created_objects.clear();

	intrans = false;

	return S_OK;
}

STDMETHODIMP CCoreBinFile::get_StorageType(long *p)
{
	CHECK_OUT(p);

	*p = 0;
	return S_OK;
}


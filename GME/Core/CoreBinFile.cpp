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

	case VALTYPE_DICT:
		binattr = new ((void*)(&attr)) BinAttr<VALTYPE_DICT>;
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

void WalkRegistry(CCoreDictionaryAttributeValue::map_type& map, CComBSTR& path, CCoreBinFile* p_bf, BinObject& node)
{
	binattrs_iterator i = node.binattrs.begin();
	binattrs_iterator e = node.binattrs.end();
	while( i != e)
	{
		if( i->attrid == ATTRID_REGNOWNER + ATTRID_COLLECTION) {
			// Copy, since we're going to remove from it
			std::vector<objects_iterator> a = ((BinAttr<VALTYPE_COLLECTION>*)(void*)&(*i))->a;
			for (auto it = a.begin(); it != a.end(); it++)
			{
				objects_type::iterator regnode = p_bf->objects.find((*it)->first);
				if (regnode == p_bf->objects.end())
					throw E_BINFILE;

				CComVariant name;
				regnode->second.Find(ATTRID_NAME)->Get(p_bf, &name);
#define RFLAG_HASVALUE 1
#define RFLAG_OPAQUE 2
				CComVariant flag;
				regnode->second.Find(ATTRID_REGFLAGS)->Get(p_bf, &flag);

				CComBSTR newPath(path);
				if (path != L"")
					newPath += L"/";
				newPath += name.bstrVal;
				if (flag.intVal & RFLAG_HASVALUE) {
					CComVariant value;
					regnode->second.Find(ATTRID_REGNODEVALUE)->Get(p_bf, &value);
					map[newPath] = CComBSTR(value.bstrVal);
				}
				WalkRegistry(map, newPath, p_bf, regnode->second);
				p_bf->opened_object = regnode;
				regnode->second.Find(ATTRID_REGNOWNER)->Set(p_bf, CComVariant());
			}
			return;
		}
		++i;
	}
}

void BinObject::UpgradeRegistryIfNecessary(CCoreBinFile* p_bf)
{
	binattrs_iterator i = binattrs.begin();
	binattrs_iterator e = binattrs.end();
	while( i != e)
	{
		if( i->attrid == ATTRID_REGNOWNER + ATTRID_COLLECTION) {
			CCoreDictionaryAttributeValue::map_type map;
			
			WalkRegistry(map, CComBSTR(), p_bf, *this);

			BinAttrBase::Create(*i, VALTYPE_DICT);
			i->attrid = ATTRID_REGNODE;
			CComVariant dict;
			i->Get(p_bf, &dict);
			((CCoreDictionaryAttributeValue*)dict.pdispVal)->m_dict = map;
			return;
		}
		++i;
	}
}

void BinObject::HasGuidAndStatAttributes(GUID& t_guid, bool* p_statusFound, bool* p_oldRegFound)
{
	binattrs_iterator i = binattrs.begin();
	binattrs_iterator e = binattrs.end();
	while( i != e)
	{
		switch ((i)->attrid)
		{
			case ATTRID_GUID1:
				t_guid.Data1 = reinterpret_cast<BinAttr<VALTYPE_LONG>*>(&(*i))->a;
				break;
			case ATTRID_GUID2:
				t_guid.Data2 = (reinterpret_cast<BinAttr<VALTYPE_LONG>*>(&(*i))->a) >> 16;
				t_guid.Data3 = (reinterpret_cast<BinAttr<VALTYPE_LONG>*>(&(*i))->a) & 0xFFFF;
				break;

			case ATTRID_GUID3: {
				long v3 = reinterpret_cast<BinAttr<VALTYPE_LONG>*>(&(*i))->a;
				t_guid.Data4[0] = (v3 >> 24);
				t_guid.Data4[1] = (v3 >> 16) & 0xFF;
				t_guid.Data4[2] = (v3 >> 8) & 0xFF;
				t_guid.Data4[3] = v3 & 0xFF;
				} break;
			case ATTRID_GUID4: {
				long v4 = reinterpret_cast<BinAttr<VALTYPE_LONG>*>(&(*i))->a;
				t_guid.Data4[4] = (v4 >> 24);
				t_guid.Data4[5] = (v4 >> 16) & 0xFF;
				t_guid.Data4[6] = (v4 >> 8) & 0xFF;
				t_guid.Data4[7] = v4 & 0xFF;
				} break;
			case ATTRID_FILESTATUS: *p_statusFound = true; break;
			case ATTRID_REGNOWNER + ATTRID_COLLECTION:
				*p_oldRegFound = true; break;
		};

		++i;
	}
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
	binattrs.emplace_back(ATTRID_NONE);
	BinAttrUnion& binattr1space = binattrs.back(); 
	BinAttrBase* binattr1 = BinAttrBase::Create(binattr1space, VALTYPE_LONG);
	binattrs.emplace_back(ATTRID_NONE);
	BinAttrUnion& binattr2space = binattrs.back(); 
	BinAttrBase* binattr2 = BinAttrBase::Create(binattr2space, VALTYPE_LONG);
	binattrs.emplace_back(ATTRID_NONE);
	BinAttrUnion& binattr3space = binattrs.back(); 
	BinAttrBase* binattr3 = BinAttrBase::Create(binattr3space, VALTYPE_LONG);
	binattrs.emplace_back(ATTRID_NONE);
	BinAttrUnion& binattr4space = binattrs.back(); 
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

}

// this method will create a status attribute for mga objects
// loaded from MGA files saved with a previous version of gme
void BinObject::CreateStatusAttribute( CCoreBinFile* p_bf)
{
	// create BinAttr of LONG type
	binattrs.emplace_back(ATTRID_NONE);
	BinAttrUnion& binattrspace = binattrs.back(); 
	BinAttrBase* binattr = BinAttrBase::Create(binattrspace, VALTYPE_LONG);

	// fill the only public field
	binattr->attrid = ATTRID_FILESTATUS;

	// set the value
	binattr->Set( p_bf, CComVariant( 0L));
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

		binattrs.emplace_back(ATTRID_NONE);
		BinAttrUnion& binattrspace = binattrs.back(); 
		BinAttrBase *binattr = BinAttrBase::Create(binattrspace, valtype);

		ASSERT( attrid != ATTRID_NONE );
		binattr->attrid = attrid;

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

		case VALTYPE_DICT:
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

		binattrs.emplace_back(ATTRID_NONE);
		BinAttrUnion& binattrspace = binattrs.back();
		BinAttrBase *binattr = BinAttrBase::Create(binattrspace, valtype);
		ASSERT( binattr != NULL );

		attrid_type attrid;
		binfile->read(attrid);
		ASSERT( attrid != ATTRID_NONE );

		binattr->attrid = attrid;

		// Possible pitfall: binattr == &binattrspace. It is possible the compiler will figure this out, and call BinAttrUnion::Read() (which we don't want)
		binattr->Read(binfile);
	}
	ASSERT(binattrs.size() == num_attrs);
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
	COMTHROW(metaproject->get_GUID(PutOut(tmp)));
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

void CCoreBinFile::read(unsigned char*& b, int& len)
{
	read(len);
	ASSERT( len >= 0 );

	b = (unsigned char*)malloc(len);
	if (b == NULL) {
		// KMS: could get here if the project is corrupt and len is incorrect
		COMTHROW(E_OUTOFMEMORY);
	}
	if( len > 0 ) {
		if (len > cifs_eof - cifs) {
			HR_THROW(E_FILEOPEN);
		}
		memcpy(b, cifs, len);
		cifs += len;
	}
}

void CCoreBinFile::readstring(char*& pos) {
	pos = cifs;
	int len;

	read(len);
	cifs += len;
}

void CCoreBinFile::read(CComBstrObj &ss)
{
	read(ss, cifs);
}

void CCoreBinFile::read(CComBstrObj &ss, char*& cifs)
{
	int len;

	//read(len);
	// use local cifs
	CoreBinFile_read(len, 4);
	ASSERT( len >= 0 );

	if( len > 0 ) {
		if (len > cifs_eof - cifs) {
			HR_THROW(E_FILEOPEN);
		}
		CopyTo(cifs, len, &ss.p, CP_UTF8);
		cifs += len;
	} else {
		std::string s;
		CopyTo(s, ss);
	}
}

void CCoreBinFile::write(const bindata &b)
{
	ASSERT( ofs.is_open() );

	int len = (int)b.size();
	ASSERT( len >= 0 );
	
	write(len);

	if( len > 0 )
		ofs.write( (const char *) &b[0], len);
}

void CCoreBinFile::write(const unsigned char* b, int len)
{
	ASSERT( ofs.is_open() );

	ASSERT( len >= 0 );
	
	write(len);

	if( len > 0 )
		ofs.write( (const char *) b, len);
}

void CCoreBinFile::write(const CComBstrObj &ss)
{
	ASSERT( ofs.is_open() );

	int len = GetCharLength(ss, SysStringLen(ss.p), CP_UTF8);
	char* s = NULL;
	if (len) {
		s = new char[len];
		CopyTo(ss.p, s, len, CP_UTF8);
	}

	ASSERT( len >= 0 );
	
	write(len);

	if( len > 0 )
		ofs.write( (const char *) &s[0], len);
	delete[] s;
}

void CCoreBinFile::writestring(const char* pos)
{
	int len;
	memcpy(&len, pos, sizeof(len));
	write(len);
	ofs.write(pos+sizeof(int), len);
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
		maxobjids.insert( core::pair<const metaid_type, objid_type>(metaid, OBJID_NONE) );

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

static DWORD __stdcall prog(LARGE_INTEGER TotalFileSize, LARGE_INTEGER TotalBytesTransferred,LARGE_INTEGER StreamSize,
LARGE_INTEGER StreamBytesTransferred, DWORD dwStreamNumber, DWORD dwCallbackReason, HANDLE hSourceFile, HANDLE hDestinationFile, LPVOID lpData)
{
	return PROGRESS_STOP;
}


void CCoreBinFile::SaveProject(const std::wstring& origfname, bool keepoldname)
{
	ASSERT( !ofs.is_open() );
	ASSERT( metaprojectid.size() == 16 );


	std::wstring filenameout = filename;
	// origfname == filename => file_buffer has filename locked FILE_SHARE_READ
	// CopyFile because:
	// SetEndOfFileInformationFile
	// Preserves extended attributes, NTFS alternate streams, file attributes (and newer Windows: security attributes)
	if (origfname == filename && GetFileAttributesW(origfname.c_str()) != INVALID_FILE_ATTRIBUTES)
	{
		filenameout += L"tmp";
		BOOL cancel = FALSE;
		BOOL succ = CopyFileExW(origfname.c_str(), filenameout.c_str(), &prog, NULL, &cancel, 0);
		if (!succ && GetLastError() != ERROR_REQUEST_ABORTED)
		{
			HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
			_bstr_t err;
			GetErrorInfo(hr, err.GetAddress());
			throw_com_error(hr, _bstr_t(L"Error saving '") + filenameout.c_str() + L"': " + err);
		}
	}
	// TODO:
	// GetNamedSecurityInfo(source, GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION)
	// SetNamedSecurityInfo(target, GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION)

	ofs.clear();
	ofs.open(filenameout.c_str(), std::ios::out | std::ios::binary);
	if( ofs.fail() || !ofs.is_open() ) {
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		ofs.close();
		ofs.clear();
		_bstr_t err;
		GetErrorInfo(hr, err.GetAddress());
		throw_com_error(hr, _bstr_t(L"Error saving '") + filenameout.c_str() + L"': " + err);
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

	if( ofs.fail() )
		HR_THROW(E_FILEOPEN);

	file_buffer.~membuf();
	new ((void*)&file_buffer) membuf();

	if (origfname == filename)
	{
		BOOL succ = MoveFileExW(filenameout.c_str(), filename.c_str(), MOVEFILE_REPLACE_EXISTING);
		if (!succ)
		{
			HR_THROW(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	if (file_buffer.open((keepoldname ? origfname : filename).c_str()) != 0) {
		HR_THROW(HRESULT_FROM_WIN32(GetLastError()));
	}
	cifs = file_buffer.getBegin();
	cifs_eof = file_buffer.getEnd();
}

// KMS: due to a bug in MgaFolder::CopyFCOs (ObjTreeCopyFoldersToo) fixed in r2297, some mga files may have duplicate GUIDs
static void SetNewGuid(CCoreBinFile* p_bf, BinObject& o)
{
	CComVariant l1, l2, l3, l4;
	l4.vt = l3.vt = l2.vt = l1.vt = VT_I4;
	getMeAGuid( &l1.lVal, &l2.lVal, &l3.lVal, &l4.lVal);

	binattrs_iterator i = o.binattrs.begin();
	binattrs_iterator e = o.binattrs.end();
	while( i != e)
	{
		switch (i->attrid)
		{
		case ATTRID_GUID1:
			i->Set(p_bf, l1);
			break;
		case ATTRID_GUID2:
			i->Set(p_bf, l2);
			break;
		case ATTRID_GUID3:
			i->Set(p_bf, l3);
			break;
		case ATTRID_GUID4:
			i->Set(p_bf, l4);
			break;
		default:
			break;
		}
		i++;
	}
}

struct GUID_hash {
	size_t operator()(const GUID& guid) const
	{
		int* iGuid = (int*)(void*)&guid;
		return iGuid[0] ^ iGuid[1] ^ iGuid[2] ^ iGuid[3];
	}
};

void CCoreBinFile::LoadProject()
{
	InitMaxObjIDs();

	if (file_buffer.open(filename.c_str()) != 0) {
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		_bstr_t err;
		GetErrorInfo(hr, err.GetAddress());
		throw_com_error(hr, _bstr_t(L"Error opening '") + filename.c_str() + L"': " + err);
	}
	cifs = file_buffer.getBegin();
	cifs_eof = file_buffer.getEnd();

	std::unordered_map<GUID, bool, GUID_hash> guids;

	bindata guid;
	read(guid);

	if( !(guid == metaprojectid) )
		HR_THROW(E_PROJECT_MISMATCH);

	ASSERT( resolvelist.empty() );
	bool oldReg_found = false;

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
		if (i == maxobjids.end())
		{
			// e.g. 0x79==121: registry node from old-style registry
			i = maxobjids.insert(maxobjids_type::value_type(metaid, 1)).first;
		}

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
			bool stat_found( false);

			GUID zero_guid = {0};
			GUID guid = {0};
			opened_object->second.HasGuidAndStatAttributes(guid, &stat_found, &oldReg_found);

			if(guid == zero_guid) // we will create guid attributes for it
				opened_object->second.CreateGuidAttributes( this);

			std::pair<std::hash_map<GUID, bool>::iterator, bool> guid_insert = guids.emplace(std::make_pair(guid, true));
			if (guid_insert.second == false)
			{
				SetNewGuid(this, opened_object->second);
			}

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

	if (oldReg_found) {
		for (auto it = objects.begin(); it != objects.end(); it++)
		{
			if (it->first.metaid >= DTID_MODEL && it->first.metaid <= DTID_FOLDER)	// 101 .. 106
			{
				it->second.UpgradeRegistryIfNecessary(this);
			}
		}
		for (auto it = objects.begin(); it != objects.end(); )
		{
			if (it->first.metaid == DTID_REGNODE)
				objects.erase(it++);
			else
				it++;
		}
	}

	ofs.clear();
	  // FIXME: set read_only correctly
	read_only = false;
}

STDMETHODIMP CCoreBinFile::OpenProject(BSTR connection, VARIANT_BOOL *ro_mode) {
	if( IsOpened() || metaproject == NULL )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		if (SysStringLen(connection))
			filename = connection;
		if (!(std::wstring(filename, 0, 4) == L"MGA="))
			throw_com_error(E_INVALID_USAGE, L"Connection string must start with MGA=");

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
		if (SysStringLen(connection))
			filename = connection;
		if( !(std::wstring(filename, 0, 4) == L"MGA=") )
			throw_com_error(E_INVALID_USAGE, L"Connection string must start with MGA=");


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

		if (filename.empty())
			filename = L".";

		modified = false;

		ASSERT( IsOpened() );
	}
	COMCATCH( CancelProject(); )
}

STDMETHODIMP CCoreBinFile::SaveProject(BSTR connection, VARIANT_BOOL keepoldname = VARIANT_TRUE) 
{
	std::wstring origfname = filename;
	COMTRY
	{
		std::wstring fn;
		if (SysStringLen(connection))
			fn = connection;

		if( !fn.empty() ) 
		{
			if( !(std::wstring(fn, 0, 4) == L"MGA=") )
			{
				throw_com_error(E_INVALID_USAGE, L"Connection string must start with MGA=");
			}

			fn.erase(0, 4);
			filename = fn;
			if(filename.empty())
				filename = L".";
		}
		if (filename == L".")
			COMTHROW(E_NAMEMISSING);
		SaveProject(origfname, keepoldname != VARIANT_FALSE);
		if (keepoldname != VARIANT_FALSE)
			filename = origfname;
	}
	COMCATCH( filename = origfname;)
}

STDMETHODIMP CCoreBinFile::CloseProject( VARIANT_BOOL abort) {
	if( !IsOpened() || metaproject == NULL )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		if( abort == VARIANT_FALSE && modified && !(filename == L".")) 
		{
			HRESULT hr = SaveProject(NULL);
			if (FAILED(hr))
				HR_THROW(hr);
		}

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

	auto i = deleted_objects.begin();
	auto e = deleted_objects.end();
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

	auto i = deleted_objects.begin();
	auto e = deleted_objects.end();
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



#include "stdafx.h"

// --------------------------- CCoreObjectPtr

void CCoreObjectPtr::GetCollectionValue(attrid_type attrid, coreobjects_type &ret) const
{
	ASSERT( sizeof(CCoreObjectPtr) == sizeof(ICoreObject*) );

	ASSERT( p != NULL );
	ASSERT( ret.empty() );

	CComVariant v;
	COMTHROW( p->get_LoadedAttrValue(attrid + ATTRID_COLLECTION, PutOut(v)) );
	ASSERT( v.vt == VT_DISPATCH );

	CComObjPtr<ICoreObjects> coll;
	COMTHROW( ::QueryInterface(v.pdispVal, coll) );
	ASSERT( coll != NULL );

	long count = 0;
	COMTHROW( coll->get_Count(&count) );
	ASSERT( count >= 0 );

	if(count!=0)
	{
		// Used to be: ret.insert(ret.begin(), count, CCoreObjectPtr());
		ret.resize(count);
		COMTHROW( coll->GetAll(count, (ICoreObject**)&ret[0]) );
	}
}

void CCoreObjectPtr::GetStringValue(attrid_type attrid, BSTR *ret) const
{
	ASSERT( p != NULL );
	ASSERT( ret != NULL && *ret == NULL );

	CComVariant v;
	COMTHROW( p->get_LoadedAttrValue(attrid, PutOut(v)) );
	ASSERT( v.vt == VT_BSTR );

	MoveTo(&v.bstrVal, ret);
}

void CCoreObjectPtr::GetVariantValue(attrid_type attrid, VARIANT *ret) const
{
	ASSERT( p != NULL );
	ASSERT( ret != NULL && ret->vt == VT_EMPTY );

	COMTHROW( p->get_LoadedAttrValue(attrid, ret) );
}

long CCoreObjectPtr::GetLongValue(attrid_type attrid) const
{
	ASSERT( p != NULL );

	CComVariant v;
	COMTHROW( p->get_LoadedAttrValue(attrid, PutOut(v)) );
	ASSERT( v.vt == VT_I4 );

	return v.lVal;
}

VARIANT_BOOL CCoreObjectPtr::GetBoolValue(attrid_type attrid) const
{
	ASSERT( p != NULL );

	CComVariant v;
	COMTHROW( p->get_LoadedAttrValue(attrid, PutOut(v)) );
	ASSERT( v.vt == VT_I4 );

	return v.lVal ? VARIANT_TRUE : VARIANT_FALSE;
}

void CCoreObjectPtr::GetPointerValue(attrid_type attrid, CCoreObjectPtr &ret) const
{
	ASSERT( ret == NULL );

	CComVariant v;
	COMTHROW( p->get_LoadedAttrValue(attrid, PutOut(v)) );
	ASSERT( v.vt == VT_DISPATCH );

	if( v.pdispVal != NULL )
		COMTHROW( ::QueryInterface(v.pdispVal, PutOut(ret)) );
}

metaid_type CCoreObjectPtr::GetMetaID() const
{
	ASSERT( p != NULL );

	CComObjPtr<ICoreMetaObject> metaobject;
	COMTHROW( p->get_MetaObject(PutOut(metaobject)) );

	metaid_type metaid;
	COMTHROW( metaobject->get_MetaID(&metaid) );

	return metaid;
}

objid_type CCoreObjectPtr::GetObjID() const
{
	ASSERT( p != NULL );

	objid_type objid;
	COMTHROW( p->get_ObjID(&objid) );

	return objid;
}

// ------- Load

void CCoreObjectPtr::LoadCollectionValue(attrid_type attrid, coreobjects_type &ret)
{
	ASSERT( sizeof(CCoreObjectPtr) == sizeof(ICoreObject*) );

	ASSERT( p != NULL );
	ASSERT( ret.empty() );

	CComVariant v;
	COMTHROW( p->get_AttributeValue(attrid + ATTRID_COLLECTION, PutOut(v)) );
	ASSERT( v.vt == VT_DISPATCH );

	CComObjPtr<ICoreObjects> coll;
	COMTHROW( ::QueryInterface(v.pdispVal, coll) );
	ASSERT( coll != NULL );

	long count = 0;
	COMTHROW( coll->get_Count(&count) );
	ASSERT( count >= 0 );
	
	if(count!=0)
	{
		ret.insert(ret.begin(), count, CCoreObjectPtr());
		COMTHROW( coll->GetAll(count, (ICoreObject**)&ret[0]) );
	}
}

// ------- Put

void CCoreObjectPtr::PutStringValue(attrid_type attrid, BSTR a)
{
	ASSERT( p != NULL );

	COMTHROW( p->put_AttributeValue(attrid, PutInVariant(a)) );
}

void CCoreObjectPtr::PutVariantValue(attrid_type attrid, VARIANT a)
{
	ASSERT( p != NULL );

	COMTHROW( p->put_AttributeValue(attrid, a) );
}

void CCoreObjectPtr::PutLongValue(attrid_type attrid, long a)
{
	ASSERT( p != NULL );

	COMTHROW( p->put_AttributeValue(attrid, PutInVariant(a)) );
}

void CCoreObjectPtr::PutBoolValue(attrid_type attrid, VARIANT_BOOL a)
{
	ASSERT( p != NULL );

	COMTHROW( p->put_AttributeValue(attrid, PutInVariant(a)) );
}

void CCoreObjectPtr::PutPointerValue(attrid_type attrid, ICoreObject *a)
{
	ASSERT( p != NULL );

	COMTHROW( p->put_AttributeValue(attrid, PutInVariant(a)) );
}

void CCoreObjectPtr::PutLockValue(attrid_type attrid, locking_type locking)
{
	ASSERT( p != NULL );

	COMTHROW( p->put_AttributeValue(attrid, PutInVariant(locking)) );
}

// --------------------------- ComGetAttrValue

HRESULT ComGetAttrValue(IUnknown *self, attrid_type attrid, BSTR *p)
{
	CHECK_OUT(p);

	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);
		me.GetStringValue(attrid, p);
	}
	COMCATCH(;)
}

HRESULT ComGetAttrValue(IUnknown *self, attrid_type attrid, long *p)
{
	CHECK_OUT(p);

	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);
		*p = me.GetLongValue(attrid);
	}
	COMCATCH(;)
}

HRESULT ComGetAttrValue(IUnknown *self, attrid_type attrid, VARIANT *p)
{
	CHECK_OUT(p);

	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);
		me.GetVariantValue(attrid, p);
	}
	COMCATCH(;)
}

HRESULT ComGetDisplayedName(IUnknown *self, attrid_type dispattrid, attrid_type nameattrid, BSTR *p)
{
	CHECK_OUT(p);

	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);

		CComVariant v;
		COMTHROW( me->get_LoadedAttrValue(dispattrid, PutOut(v)) );
		ASSERT( v.vt == VT_BSTR );

		std::string dispname;
		CopyTo(v, dispname);

		if( dispname.empty() )
		{
			v.Clear();

			COMTHROW( me->get_LoadedAttrValue(nameattrid, PutOut(v)) );
			ASSERT( v.vt == VT_BSTR );
		}

		MoveTo(&v.bstrVal, p);
	}
	COMCATCH(;)
}

HRESULT ComGetObjType(IUnknown *self, objtype_enum *p)
{
	CHECK_OUT(p);

	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);

		metaid_type metaid = me.GetMetaID();

		if( metaid == METAID_METAPROJECT )
			*p = OBJTYPE_FOLDER;
		else
			*p = (objtype_enum)(metaid - METAID_METABASE);
	}
	COMCATCH(;)
}

// --------------------------- ComPutAttrValue

HRESULT ComPutAttrValue(IUnknown *self, attrid_type attrid, BSTR p)
{
	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);
		me.PutStringValue(attrid, p);
	}
	COMCATCH(;)
}

HRESULT ComPutAttrValue(IUnknown *self, attrid_type attrid, long p)
{
	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);
		me.PutLongValue(attrid, p);
	}
	COMCATCH(;)
}

HRESULT ComPutAttrValue(IUnknown *self, attrid_type attrid, VARIANT p)
{
	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);
		me.PutVariantValue(attrid, p);
	}
	COMCATCH(;)
}

HRESULT ComPutPointerValue(IUnknown *self, attrid_type attrid, IDispatch *p)
{
	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);

		CCoreObjectPtr other(p);

		me.PutPointerValue(attrid, other);
	}
	COMCATCH(;)
}

// --------------------------- Others

void ChangeAttrValueType(CComVariant &v, attval_enum attval)
{
	ASSERT( v.vt == VT_BSTR );

	switch(attval)
	{
	case ATTVAL_STRING:
	case ATTVAL_ENUM:
	case ATTVAL_DYNAMIC:
		return;

	case ATTVAL_INTEGER:
		if( FAILED(v.ChangeType(VT_I4, &v)) )
			v = (int)0;

		ASSERT( v.vt == VT_I4 );
		break;

	case ATTVAL_DOUBLE:
		if( FAILED(v.ChangeType(VT_R8, &v)) )
			v = (double)0.0;

		ASSERT( v.vt == VT_R8 );
		break;

	case ATTVAL_BOOLEAN:
		if( FAILED(v.ChangeType(VT_BOOL, &v)) )
			v = (VARIANT_BOOL)(VARIANT_FALSE);

		ASSERT( v.vt == VT_BOOL );
		break;

	default:
		HR_THROW(E_ATTVALTYPE);
	}
}

HRESULT ComDeleteObject(IUnknown *self)
{
	COMTRY
	{
		ASSERT( self != NULL );
		CCoreObjectPtr me(self);
		COMTHROW( me->Delete() );
	}
	COMCATCH(;)
}

CComBSTR truncateName( IUnknown *p_bs, CComBSTR pIn)
{
	CComBSTR truncated_name;
	CComBSTR nmspc;
	CComQIPtr<IMgaMetaBase> bs( p_bs);
	if( bs)
	{
		CComPtr<IMgaMetaProject> pr;
		COMTHROW( bs->get_MetaProject( &pr));
		COMTHROW( pr->GetNmspc( &nmspc)); // fetch namespace into m_nmspc
	}
	if( nmspc && nmspc.Length() > 0) // some namespace is set
	{
		COMTHROW(nmspc.Append( L"::"));
		unsigned int nms_len = nmspc.Length();
		unsigned int pin_len = pIn.Length();
		if( nms_len < pin_len
			&& 0 == wcsncmp( pIn.m_str, nmspc.m_str, (size_t) nms_len))
		{
			unsigned int i = nms_len;
			while( i < pin_len)
			{
				COMTHROW(truncated_name.Append( pIn[i]));
				++i;
			}

			return truncated_name;
		}
	}

	return pIn;
}

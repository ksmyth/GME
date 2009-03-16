#include "stdafx.h"
#include <algorithm>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <direct.h>
#include <fstream>
#include <time.h>
#include <sys/timeb.h>
#include "CoreXmlFile.h"
#include "CommonCollection.h"
#include "VSSLoginDlg.h"
#include "SvnLoginDlg.h"
#include "FilesInUseDlg.h"
#include "FilesInUseDetailsDlg.h"
#include "DomErrorPrinter.h"
#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp> 
#include "SvnExec.h"
#include "SvnTestDlg.h"
#include "FileHelp.h"
#include "DirSupplier.h"
#include "Transcoder.h"

using namespace XERCES_CPP_NAMESPACE;
using std::string;

#define DETAILS_ABOUT_XMLBACKEND 0
#define RESOLVE_PTRS_2ND_ATTEMPT 0

std::string g_userName;
std::string g_passWord;

const char* OpCodeStr[] =
{
	  "DELETED"
	, "MOVED"
	, "DERIVED"
	, "REFERRED"
	, "CONNECTED"
	, "TAKESPARTINCONN"
	, "REF_REDIRECTED"
	, "TOTAL"
}; // keep these in sync with OpCode

/*static*/ const char * OperatingOptions::m_usrConfName = "config.opt";
/*static*/ const char * OperatingOptions::m_sysConfName = "project.opt";
/*static*/ const char * OperatingOptions::m_sysConfDefContentsPlain = "\
# comments must be prefixed by a hashmark\n\
#UseCache=false\n\
#AutomaticLogin=true\n\
#UseAccountInfo=true\n\
#account=ya\n\
#phrase=\n\
\n\
# specifies maximum time of tolerance (in minutes) for old entries\n\
# if set to 0 then a max time-to-live is not imposed on entries\n\
PurgeDelayed=17.7777222\n\
\n\
# shows checkout status upon load\n\
OnLoadShowStatus = false\n\
\n\
# use this option to avoid seeing the 'Keep files checked out'\n\
DefCheckInOnSave = true\n\
\n\
# use this option to avoid seeing 'Some files need to be checked out'\n\
DefCheckOutOnAction = true\n\
\n\
# to overwrite these settings with user-specific preferences create a file\n\
# called config.opt, in your local checkout directory\n\
";

/*static*/ const char * OperatingOptions::m_sysConfDefContentsSvn = "\
# comments must be prefixed by a hashmark\n\
#UseCache=false\n\
#AutomaticLogin=true\n\
#UseAccountInfo=true\n\
#account=ya\n\
#phrase=\n\
\n\
# specifies maximum time of tolerance (in minutes) for old entries\n\
# if set to 0 then a max time-to-live is not imposed on entries\n\
PurgeDelayed=17.7777222\n\
\n\
# shows checkout status upon load\n\
OnLoadShowStatus = false\n\
\n\
# use this option to avoid seeing the 'Keep files checked out'\n\
DefCheckInOnSave = true\n\
\n\
# use this option to avoid seeing 'Some files need to be checked out'\n\
DefCheckOutOnAction = true\n\
\n\
# use this option to override the url or it's protocol part only thus you\n\
# may select from https and svn+ssh protocols and their corresponding urls\n\
#PreferredUrl = https://.... \n\
#PreferredUrl = svn+ssh://... \n\
\n\
# use this option to specify your preferred access method: CMD or API\n\
#AccessMethod = API # or CMD\n\
\n\
# Speed-up commits to the repository by aggregation.\
UseBulkCommit = true \n\
\n\
# to overwrite these settings with user-specific preferences create a file\n\
# called config.opt, in your local checkout directory\n\
";

/*static*/ const char * HelperFiles::signFileName = "sign.txt";
/*static*/ const char * HelperFiles::protFileName = "list_";
/*static*/ const char * HelperFiles::protFileExt  = ".txt";

/*static*/ const char * ParserLiterals::Main::deleted = "deleted";
/*static*/ const char * ParserLiterals::Main::metaId  = "MetaId";
/*static*/ const char * ParserLiterals::Main::id      = "Id";
/*static*/ const char * ParserLiterals::Main::parent  = "Parent";

/*static*/ const char * ParserLiterals::Signer::users = "users";
/*static*/ const char * ParserLiterals::Signer::user  = "user";
/*static*/ const char * ParserLiterals::Signer::name  = "name";
/*static*/ const char * ParserLiterals::Signer::since = "since";
/*static*/ const char * ParserLiterals::Signer::until = "until";

/*static*/ const char * ParserLiterals::Protector::item    = "item";
/*static*/ const char * ParserLiterals::Protector::when    = "when";
/*static*/ const char * ParserLiterals::Protector::oper    = "oper";
/*static*/ const char * ParserLiterals::Protector::gd      = "gd";
/*static*/ const char * ParserLiterals::Protector::objects = "objects";

/*static*/ const char * ParserLiterals::newln = "\n";
/*static*/ const char * ParserLiterals::empty = "";

/*static*/ const char * CCoreXmlFile::m_contentConst = "contents";

bool XmlObjComp::operator()( XmlObject * p1, XmlObject * p2) const
{
    if( !p1) return true;
    if( !p2) return false;
    return GUID_less()( p1->m_guid, p2->m_guid);
}

void bin2string( const unsigned char * bytes, int len, std::string& str )
{
    char hex[3];
    str.clear();
    for( int i=0; i<len; ++i )
    {
        sprintf( hex, "%02x", bytes[i] );
        str += hex;
    }
}

unsigned char hexCharToInt( char ch )
{
    if( ch>='0' && ch<='9' )
        return ch - '0';

    if( ch == 'a' || ch == 'A' )
        return 10;
    if( ch == 'b' || ch == 'B' )
        return 11;
    if( ch == 'c' || ch == 'C' )
        return 12;
    if( ch == 'd' || ch == 'D' )
        return 13;
    if( ch == 'e' || ch == 'E' )
        return 14;
    if( ch == 'f' || ch == 'F' )
        return 15;
    ASSERT(false);
    return 0;
}

void string2bin( const char * str, unsigned char * bytes )
{
    int l = strlen(str) / 2;
    for( int i=0; i<l; ++i )
    {        
        unsigned char c1 = hexCharToInt( str[i*2] );
        unsigned char c2 = hexCharToInt( str[i*2 + 1] );
        //unsigned char c;
        //sscanf( str + i*2, "%02x", &c );      // do not work in release veriosn...
        bytes[i] = 16 * c1 + c2;
    }
}

void guid2str( GUID guid, std::string& str )
{
    if( guid == GUID_NULL )
        str = "null";
    else
        bin2string( (unsigned char*)&guid, sizeof(GUID), str );
}

GUID str2guid( const char * str )
{    
    int len = strlen(str);
    if( len == 0 || strcmp( str, "null" ) == 0 )
        return GUID_NULL;
    else
    {
        GUID guid;
        string2bin( str, (unsigned char*)&guid );
        return guid;
    }
}

//
// obsolete since MgaGeneric.cpp does not contain any
// attribTokens (like 'MGA Version' was) with space inside
//
//void replaceSpaceWithUnderscore( std::string& str )
//{
//    for( unsigned int i=0; i < str.size(); ++i )
//        if( str[i] == ' ' )
//            str[i] = '_';
//}
//
//void replaceUnderscoreWithSpace( char * str )
//{
//    if( str == NULL )
//        return;
//
//    int n = strlen(str);
//    for( int i=0; i<n; ++i )
//        if( str[i] == '_' )
//            str[i] = ' ';
//}


////////////////////////////////////////////////////////////////////////////////
// XmlAttrBase class
////////////////////////////////////////////////////////////////////////////////

XmlAttrBase * XmlAttrBase::create(valtype_type valtype)
{
    ASSERT( valtype != VALTYPE_NONE );

    XmlAttrBase * xmlattr = NULL;

    switch(valtype)
    {
    case VALTYPE_LONG:
        xmlattr = new XmlAttrLong();
        break;

    case VALTYPE_STRING:
        xmlattr = new XmlAttrString();
        break;

    case VALTYPE_BINARY:
        xmlattr = new XmlAttrBinary();
        break;

    case VALTYPE_LOCK:
        xmlattr = new XmlAttrLock();
        break;

    case VALTYPE_POINTER:
        xmlattr = new XmlAttrPointer();
        break;

    case VALTYPE_COLLECTION:
        xmlattr = new XmlAttrCollection();
        break;

    case VALTYPE_REAL:
        xmlattr = new XmlAttrReal();
        break;

    default:
        HR_THROW(E_METAPROJECT);
    }

    if( xmlattr == NULL )
        HR_THROW(E_OUTOFMEMORY);

    return xmlattr;
}

XmlAttrBase::XmlAttrBase()
{ 
}
    
XmlAttrBase::~XmlAttrBase() 
{ 
}

////////////////////////////////////////////////////////////////////////////////
// XmlAttrLong class
////////////////////////////////////////////////////////////////////////////////

XmlAttrLong::XmlAttrLong()
{
    m_value = 0;
}

valtype_type XmlAttrLong::getType() const
{
    return VALTYPE_LONG;
}

void XmlAttrLong::fromVariant(VARIANT p)
{
    CopyTo(p, m_value);
}

void XmlAttrLong::toVariant(VARIANT *p) const
{
    CopyTo(m_value, p);
}

void XmlAttrLong::fromString(const char * str)
{
    if( str == NULL || strlen(str)==0 )
        m_value = 0;
    else
        m_value = atoi( str );
}

void XmlAttrLong::toString(std::string& str) const
{
    static char buf[100];
    sprintf( buf, "%ld", m_value );
    str = buf;
}

////////////////////////////////////////////////////////////////////////////////
// XmlAttrReal class
////////////////////////////////////////////////////////////////////////////////

XmlAttrReal::XmlAttrReal()
{
    m_value = 0;
}

valtype_type XmlAttrReal::getType() const
{
    return VALTYPE_REAL;
}

void XmlAttrReal::fromVariant(VARIANT p)
{
    CopyTo(p, m_value);
}

void XmlAttrReal::toVariant(VARIANT *p) const
{
    CopyTo(m_value, p);
}

void XmlAttrReal::fromString(const char * str)
{
    if( str == NULL || strlen(str)==0 )
        m_value = 0;
    else
        m_value = atof( str );
}

void XmlAttrReal::toString(std::string& str) const
{
    static char buf[100];
    sprintf( buf, "%.6f", m_value );
    str = buf;
}

////////////////////////////////////////////////////////////////////////////////
// XmlAttrString class
////////////////////////////////////////////////////////////////////////////////

valtype_type XmlAttrString::getType() const
{
    return VALTYPE_STRING;
}

void XmlAttrString::fromVariant(VARIANT p)
{
    CopyTo(p, m_value);
}

void XmlAttrString::toVariant(VARIANT *p) const
{
    CopyTo(m_value, p);
}

void XmlAttrString::fromString(const char * str)
{
    if( str == NULL )
        m_value = "";
    else
        m_value = str;
}

void XmlAttrString::toString(std::string& str) const
{
    str = m_value;
}

////////////////////////////////////////////////////////////////////////////////
// XmlAttrBinary class
////////////////////////////////////////////////////////////////////////////////

valtype_type XmlAttrBinary::getType() const
{
    return VALTYPE_BINARY;
}

void XmlAttrBinary::fromVariant(VARIANT p)
{
    CopyTo(p, m_value);
}

void XmlAttrBinary::toVariant(VARIANT *p) const
{
    CopyTo(m_value, p);
}

void XmlAttrBinary::fromString(const char * str)
{   
    if( str == NULL || strlen(str) == 0 )
        m_value.clear();
    else
    {
        // TODO: optimize this code
        int             len  = strlen(str)/2;
        unsigned char * buff = new unsigned char[len];
        string2bin( str, buff );
        m_value.resize( len );
        for( int i=0; i<len; ++i )
            m_value [i] = buff[i];
        delete [] buff;
    }
}

void XmlAttrBinary::toString(std::string& str) const
{
	if (m_value.empty()) {
		str.clear();
	}
	else {
		bin2string( &m_value[0], m_value.size(), str );
	}
}

////////////////////////////////////////////////////////////////////////////////
// XmlAttrLock class
////////////////////////////////////////////////////////////////////////////////

XmlAttrLock::XmlAttrLock()
{
    m_value = LOCKING_NONE;
}

valtype_type XmlAttrLock::getType() const
{
    return VALTYPE_LOCK;
}

void XmlAttrLock::fromVariant(VARIANT p)
{
    CopyTo(p, m_value);
}

void XmlAttrLock::toVariant(VARIANT *p) const
{
    CopyTo(m_value, p);
}

void XmlAttrLock::fromString(const char * str)
{   
    if( str == NULL || strlen(str) == 0 )
        m_value = LOCKING_NONE;
    else
        m_value = atoi( str );
}

void XmlAttrLock::toString(std::string& str) const
{
    static char buf[100];
    sprintf( buf, "%d", m_value );
    str = buf;
}

////////////////////////////////////////////////////////////////////////////////
// XmlAttrPointer class
////////////////////////////////////////////////////////////////////////////////

XmlAttrPointer::XmlAttrPointer()
{
    m_parent = NULL;
}

valtype_type XmlAttrPointer::getType() const
{
    return VALTYPE_POINTER;
}

////////////////////////////////////////////////////////////////////////////////
// XmlAttrCollection class
////////////////////////////////////////////////////////////////////////////////

valtype_type XmlAttrCollection::getType() const
{
    return VALTYPE_COLLECTION;
}

////////////////////////////////////////////////////////////////////////////////
// XmlObject class
////////////////////////////////////////////////////////////////////////////////

XmlObject::XmlObject(ICoreMetaObject *metaobject, bool createAllAttributes )
{
    m_deleted         = false;
    m_modified        = false;
    m_loaded          = createAllAttributes;
    
    COMTHROW( metaobject->get_MetaID( &m_metaid ) );
    if( m_loaded )
        createAttributes(metaobject,ATTR_ALL);
    else
        createAttributes(metaobject,ATTR_PRIMARY);
    CoCreateGuid(&m_guid);
}

XmlObject::~XmlObject()
{
    for( AttribMapIter it = m_attributes.begin(); it != m_attributes.end(); ++it )
        delete it->second;
}

bool XmlObject::isContainer()
{
    return( m_metaid==METAID_ROOT || m_metaid==DTID_MODEL || m_metaid==DTID_FOLDER );
}


void XmlObject::createAttributes(ICoreMetaObject *metaobject, int attrSet )
{
    ASSERT( metaobject != NULL );
    ASSERT( attrSet>=ATTR_PRIMARY && attrSet<=ATTR_ALL );

    CComObjPtr<ICoreMetaAttributes> metaattributes;
    COMTHROW( metaobject->get_Attributes(PutOut(metaattributes)) );
    ASSERT( metaattributes != NULL );

    typedef std::vector< CComObjPtr<ICoreMetaAttribute> > metaattributelist_type;
    metaattributelist_type metaattributelist;
    GetAll<ICoreMetaAttributes, ICoreMetaAttribute>(metaattributes, metaattributelist);

    for( metaattributelist_type::iterator i=metaattributelist.begin(); i!=metaattributelist.end(); ++i )
    {
        valtype_type valtype;
        COMTHROW( (*i)->get_ValueType(&valtype) );
        if( attrSet==ATTR_ALL ||
           (attrSet==ATTR_PRIMARY && (valtype==VALTYPE_POINTER || valtype==VALTYPE_COLLECTION || valtype==VALTYPE_LOCK)) ||
           (attrSet==ATTR_SECONDARY && (valtype!=VALTYPE_LOCK && valtype!=VALTYPE_POINTER && valtype!=VALTYPE_COLLECTION)))
        {
            attrid_type attrId = ATTRID_NONE;
            COMTHROW( (*i)->get_AttrID(&attrId) );
            ASSERT( attrId != ATTRID_NONE );
            m_attributes.insert( AttribMap::value_type(attrId,XmlAttrBase::create(valtype)) );
        }
    }
}

void XmlObject::deleteSecondaryAttribs()
{
    for( AttribMapIter it = m_attributes.begin(); it != m_attributes.end(); ++it )
    {
        valtype_type type =it->second->getType();
        if( type!=VALTYPE_LOCK && type!=VALTYPE_POINTER && type!=VALTYPE_COLLECTION )
        {
            delete it->second;
            m_attributes.erase(it);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// CCoreXmlFile class
////////////////////////////////////////////////////////////////////////////////

CCoreXmlFile::CCoreXmlFile()
	: m_console( true)
{
	m_opened                = false;
	m_inTransaction         = false;
	m_modified              = false;
	m_metaAttributeId       = ATTRID_NONE;
	m_metaAttributeValType  = VALTYPE_NONE;
	m_openedObject          = NULL;
	m_sourceControl         = SC_NONE;
	m_savedOnce             = false;
	m_svnByAPI              = false;
	m_hashFileNames         = false;
	m_hashInfoFound         = false;
	m_hashVal               = -1;
	m_comSvn                = CComPtr<ISvnExec>(0);
	m_cmdSvn                = 0;
	m_domImpl               = 0;
	m_domParser             = 0;
	m_domErrHandler         = 0;
	m_strategyShared        = false;
	fillParentMap();

	XMLPlatformUtils::Initialize();
}

CCoreXmlFile::~CCoreXmlFile()
{
	clearAll();
	XMLPlatformUtils::Terminate();
}

STDMETHODIMP CCoreXmlFile::get_MetaProject(ICoreMetaProject **p)
{
	CHECK_OUT(p);
	CopyTo(m_metaProject, p);
	return S_OK;
}

STDMETHODIMP CCoreXmlFile::put_MetaProject(ICoreMetaProject *p)
{
	COMTRY
	{
		closeMetaProject();
		m_metaProject = p;
	}
	COMCATCH( closeMetaProject() )
}

STDMETHODIMP CCoreXmlFile::get_MetaObject(ICoreMetaObject **p)
{
	CHECK_OUT(p);
	CopyTo(m_metaObject, p);
	return S_OK;
}

STDMETHODIMP CCoreXmlFile::put_MetaObject(ICoreMetaObject *p)
{
	if( m_metaProject == NULL )
		COMRETURN(E_INVALID_USAGE);

	if( m_metaObject == p )
		return S_OK;

	COMTRY
	{
		if( p != NULL )
		{
			// check the metaproject of the object, it must be the same as the metaproject
			// of the storage
			CComObjPtr<ICoreMetaProject> t;
			COMTHROW( p->get_Project(PutOut(t)) );
			if( !IsEqualObject(m_metaProject, t) )
				HR_THROW(E_SAMEPROJECT);
		}

		closeMetaObject();
		m_metaObject = p;
		if( m_metaObject != NULL )
			openMetaObject();
	}
	COMCATCH( closeMetaObject() )
}

STDMETHODIMP CCoreXmlFile::get_MetaID(metaid_type *p)
{
	CHECK_OUT(p);
	*p = m_metaObjectId;
	return S_OK;
}

STDMETHODIMP CCoreXmlFile::put_MetaID(metaid_type metaid)
{
	if( m_metaProject == NULL )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		if( metaid != METAID_NONE )
		{
			CComObjPtr<ICoreMetaObject> p;
			COMTHROW( m_metaProject->get_Object(metaid, PutOut(p)) );
			ASSERT( p != NULL );

			if( m_metaObject != p )
			{
				closeMetaObject();
				MoveTo(p, m_metaObject);
				openMetaObject();
			}
		}
		else
			closeMetaObject();
	}
	COMCATCH( closeMetaObject() )
}

STDMETHODIMP CCoreXmlFile::get_MetaAttribute(ICoreMetaAttribute **p)
{
	CHECK_OUT(p);
	CopyTo(m_metaAttribute, p);
	return S_OK;
}

STDMETHODIMP CCoreXmlFile::put_MetaAttribute(ICoreMetaAttribute *p)
{
	if( m_metaObject == NULL )
		COMRETURN(E_INVALID_USAGE);
	ASSERT( m_metaProject != NULL );

	if( m_metaAttribute == p )
		return S_OK;

	COMTRY
	{
		if( m_metaAttribute != NULL )
		{
			// the metaobject of the given metaattribute must be the same as the metaobject of 
			// the storage
			CComObjPtr<ICoreMetaObject> t;
			COMTHROW( m_metaAttribute->get_Object(PutOut(t)) );
			if( !IsEqualObject(m_metaObject, t) )
			{
				m_metaAttribute = NULL;
				return E_INVALIDARG;
			}
		}

		closeMetaAttribute();
		m_metaAttribute = p;
		if( m_metaAttribute != NULL )
			openMetaAttribute();
	}
	COMCATCH( closeMetaAttribute() )
}

STDMETHODIMP CCoreXmlFile::get_AttrID(attrid_type *p)
{
	CHECK_OUT(p);
	if( m_metaAttribute )
		return m_metaAttribute->get_AttrID(p);
	*p = 0;
	return S_OK;
}

STDMETHODIMP CCoreXmlFile::put_AttrID(attrid_type attrid)
{
	if( m_metaObject == NULL )
		COMRETURN(E_INVALID_USAGE);
	ASSERT( m_metaProject != NULL );

	COMTRY
	{
		if( attrid != ATTRID_NONE )
		{
			CComObjPtr<ICoreMetaAttribute> p;
			COMTHROW( m_metaObject->get_Attribute(attrid, PutOut(p)) );
			ASSERT( p != NULL );

			if( m_metaAttribute != p )
			{
				closeMetaAttribute();
				m_metaAttribute = p;
				openMetaAttribute();
			}
		}
		else
			closeMetaAttribute();
	}
	COMCATCH( closeMetaAttribute() )
}

STDMETHODIMP CCoreXmlFile::get_AttributeValue(VARIANT *p)
{
	CHECK_OUT(p);

	if( m_openedObject == NULL || !m_inTransaction )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		AttribMapIter it = m_openedObject->m_attributes.find( m_metaAttributeId );
		if( m_metaAttributeValType == VALTYPE_POINTER )
		{
			getPointer( (XmlAttrPointer*)it->second, p );
		}
		else if( m_metaAttributeValType == VALTYPE_COLLECTION )
		{
			getCollection( (XmlAttrCollection*)it->second, p );
		}
		else if( m_metaAttributeValType == VALTYPE_LOCK )
		{
			it->second->toVariant(p);
		}
		else
		{
			if( !m_openedObject->m_loaded )
			{
				fullReadContainer(getContainer(m_openedObject));
				it = m_openedObject->m_attributes.find( m_metaAttributeId );

				// if the attribute was not found -> throw
				if( m_openedObject->m_attributes.end() == it)
					COMTHROW(E_INVALID_USAGE);
			}

			it->second->toVariant(p);
		}
	}
	COMCATCH(;)
}

void CCoreXmlFile::applySmallChange( XmlObjSet& p_conts)
{
	for( XmlObjSet::const_iterator it = p_conts.begin(); it != p_conts.end(); ++it)
	{
		XmlObject * p = *it;
		AttribMapIter ait = p->m_attributes.find( ATTRID_RELID);
		if( ait != p->m_attributes.end())
		{
			long relid = ((XmlAttrLong*)(ait->second))->m_value;
			((XmlAttrLong*)(ait->second))->m_value = relid + 1;
			long reljd = ((XmlAttrLong*)(ait->second))->m_value;
			ASSERT( relid + 1 == reljd);
		}
	}
}

bool CCoreXmlFile::specialUserInquiryHandled( VARIANT p)
{
	static std::string magic_str = "UpdateSourceControlInfo";
	static std::string magi2_str = "WhoControlsThisObj";
	static std::string magi3_str = "ShowActiveUsers";

	bool ret = false;

	string str;
	CopyTo(p, str);

	bool b1 = str.find( magic_str) == 0;
	bool b2 = str.find( magi2_str) == 0;
	bool b3 = str.find( magi3_str) == 0;
	if( b1 || b2 || b3)
	{
		ret = true;

		if( b1 || b2)
		{
			//DOMBuilder * parser = NULL;
			//DOMImplementationLS * domimpl = DOMImplementationRegistry::getDOMImplementation( XMLString::transcode("XML 1.0"));//NULL
			//ASSERT( domimpl != NULL );

			//parser = domimpl->createDOMBuilder( DOMImplementationLS::MODE_SYNCHRONOUS, NULL );
			//ASSERT( parser != NULL );

			//XERCES_CPP_NAMESPACE::DOMDocument * doc = parser->parseURI( "F:\\t\\at\\an\\tesztam2\\f7bed9e2cbf62a418ec4e0df0ffe18a5.1xml");
			//if( 0 == doc->getDocumentElement())
			//{
			//	DOMNodeList* list = doc->getChildNodes();

			//	int c = (int) list->getLength();
			//	for( int i = c-1; i >= 0 ; --i)
			//	{
			//		DOMNode * node = list->item(i);
			//		if( node->getNodeType() == DOMNode::NodeType::TEXT_NODE)
			//		{
			//			DOMText * txt  = (DOMText*) node;
			//			const XMLCh* p = txt->getData();
			//		}
			//		if( node->getNodeType() == DOMNode::ELEMENT_NODE)
			//		{
			//			DOMElement* elem = (DOMElement*)node;
			//			const XMLCh* tn = elem->getTagName();
			//		}
			//	}
			//}

			std::string id = str.substr( b1?magic_str.length():magi2_str.length());
			if( id != "")
			{
				// must be in sync with the current id creation (format)
				metaobjidpair_type objid;
				sscanf( id.c_str(), "id-%04lx-%08lx", &objid.metaid, &objid.objid);

				XmlObject * obj = objectFromObjId( objid);
				if( obj && obj->isContainer()) b1?updateSourceControlInfo( obj) : whoControlsThis( obj);
			}
			else b1?updateSourceControlInfo():whoControlsThis(); // whole project
		}
		else if( b3)
		{
			std::string res( "The following users worked on the project since your log-in:\r\n");
			std::vector< LoggedIn> lus = allusers();
			for( std::vector< LoggedIn>::iterator it = lus.begin(), en = lus.end(); it != en; ++it)
			{
				res += std::string(1, '\t') + std::string(1, it->m_fl) + "\t\"" + it->m_nm + "\"\r\n";
			}

			if( lus.size() > 0)
			{
				//AfxMessageBox( res.c_str(), MB_ICONINFORMATION);
				CSvnTestDlg d;
				res.append( "Note: 'I' means Inactive, 'A' means Active users.");
				d.setContent( res.c_str());
				d.DoModal();
			}
			else
				AfxMessageBox( "No user login info found!");
		}
	}

	return ret;
}

STDMETHODIMP CCoreXmlFile::put_AttributeValue(VARIANT p)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if( m_openedObject == NULL || !m_inTransaction )
		COMRETURN(E_INVALID_USAGE);

	// root's guid changes evry time the project is modified we should igonre it
	// it casues some meta interpreter problems!
	if( m_metaAttributeId == ATTRID_GUID )
		return S_OK;

	if( 0&&m_metaAttributeId == ATTRID_LASTRELID) // never mind ATTRID_LASTRELID, thus
		return S_OK;                           // adding a child, won't affect parents

	// setting root's ATTRID_MDATE to "updatesourcecontrolinfo" means we have to update sourcecontrolinfo
	// and ignore attribute setting
	if( m_metaAttributeId == ATTRID_MDATE )
	{
		if( specialUserInquiryHandled( p))
			return S_OK;
	}

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
	time_t time1, time2;
	struct tm *tm1, *tm2;
	time( &time1); tm1 = localtime( &time1);
	//sendMsg( std::string( "CommitBegin ") + asctime( tm1 ), MSG_INFO);
#endif
#endif

	// ignore UpdateSourceControlInfo regnode
	if( m_openedObject && m_metaAttributeId == ATTRID_FILESTATUS)
		return S_OK;

	// TODO: return with specific error code
	//if( m_openedObject->m_readOnly )
	//  COMRETURN(E_INVALID_USAGE);
#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
	std::string iiid, mylo;
	std::string gd; guid2str( m_openedObject->m_guid, gd);
	char* kind[] = {"100", "M", "A", "R", "C", "S", "F", "connrole", "connseg"
		, "109", "110", "StrAttr", "IntAttr", "FloatAttr", "BoolAttr", "RrefAttr", "116", "117", "118", "119"
		, "Constraint", "RegNode", "SetNode", "123", "124", "125", "126", "127" };

	char buff[5];sprintf( buff, "%i", m_metaAttributeId);

	char *  mavt[] = { "VALTYPE_None", "VALTYPE_COLL", "VALTYPE_Pointer", "VALTYPE_LOCK", "VALTYPE_LONG"
		, "VALTYPE_STR", "VALTYPE_BIN", "VALTYPE_REAL", "VALTYPE_8", "VALTYPE_9", "VALTYPE_10", "VALTYPE_11"
		, "VALTYPE_12", "VALTYPE_13", "VALTYPE_14"};

	if( m_userOpts.m_createLog) iiid = gd + " {" + ((m_openedObject->m_metaid<=100)?"Root": kind[m_openedObject->m_metaid-100]) + "} [ " + buff + " ](" + mavt[m_metaAttributeValType] + ")";

	if( m_userOpts.m_createLog) mylo += iiid;
#endif
#endif

	COMTRY
	{
		AttribMapIter it = m_openedObject->m_attributes.find( m_metaAttributeId );
		if( m_metaAttributeValType == VALTYPE_POINTER )
		{   
			if( m_metaAttributeId == ATTRID_PARENT)
			{
				// --
				// this could be used to fine tune ATTRID_LASTRELID
				// m_userOpts.m_doModelParentLock
				// --
				// delete handled separately in cases of models and non-model fcos
				// if a, r, s, c is deleted the parent model is checked out
				// among these a, r, s can be ports
				// models are tricky because upon deleting a model its parent
				// does not get checked out
				// in addition a model also can be a port
				metaid_type &mt = m_openedObject->m_metaid;
				XmlObject * grand_parent = 0; // will be used only if it is determined to be a port
				XmlObject *       parent = 0; // will be used only if it is determined to be a port

				// simply get its father:
				AttribMapIter it3 = m_openedObject->m_attributes.find( ATTRID_PARENT );
				ASSERT( it3 != m_openedObject->m_attributes.end() );
				XmlObject * cont = parent = ((XmlAttrPointer*)(it3->second))->m_parent;

				// is removal (delete), is movement or is childbirth :) ?
				metaobjidpair_type idpair;CopyTo(p, idpair);
				bool is_removal = idpair.metaid == 0 && idpair.objid == 0; // VARIANT p is 0 => removal
				bool is_movemnt = !is_removal && parent != 0;

				if( is_removal || is_movemnt) // check if latent changes prevent this operation from happening
				{
					std::string scapegoat;
					if( findOnProtectedLists( m_openedObject->m_guid, scapegoat))
					{
						std::string msg = std::string( "Element ") + makelink( m_openedObject) +  " found on a protected list of user \"" + scapegoat + "\" thus it can't be moved/deleted!";
						sendMsg( msg, MSG_ERROR);
						AfxMessageBox( msg.c_str(), MB_ICONSTOP);
						throw hresult_exception(E_FAIL);
					}
					protect( m_openedObject, is_removal?ELEM_DELETED:ELEM_MOVED);

					ASSERT( m_userOpts.m_doConnEndPointLock); // currently we assume this especially
					ASSERT( m_userOpts.m_doConnSegmentLock);  // by not dealing too much with ports

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
					if( m_userOpts.m_createLog) {std::string nn,mm; parent->m_attributes.find( ATTRID_NAME )->second->toString( nn);
					m_openedObject->m_attributes.find(ATTRID_NAME)->second->toString(mm);
					mylo += (is_removal?" <br>-deleting ":" <br>-moving ") + mm + " in " + nn + "- <br>" ;}
#endif
#endif

				}

				if( is_removal) {
					m_deldObjs.push_back( m_openedObject);
					if( m_openedObject->m_metaid== DTID_REFERENCE) {
						//setPointer( m_openedObject, ATTRID_REFERENCE, 0);
					}
				}

				// when inserting Atoms, Refs, Sets in a archetype model, this gets down
				// to deriveds too, so they will be checked out too.
				// if models are inserted into archetype models, the subtypes of the archetype 
				// do not necessarily change. That's why we need to deal with these.
				// LAST_RELID modification is done only on the archetype parent
				// if this has been derived to SParent, and SSParent, ...
				// upon inserting a model into Parent LAST_RELID changes are not performed
				// on SParent, SSParent, because the newly inserted element has different relid (>RELIDSPACE)
				// so we need to make sure we slightly alter the subtyped parents too
				if( !is_removal && !is_movemnt && m_openedObject->m_metaid == DTID_MODEL) // a model is inserted
				{
					XmlObjSet derd_from_cont;
					if( cont)
						getDeriveds( cont, derd_from_cont); // or from its father
					else
						getDeriveds( objectFromObjId( idpair), derd_from_cont);  
					if( !derd_from_cont.empty())
						applySmallChange( derd_from_cont);
					for( XmlObjSet::const_iterator it = derd_from_cont.begin(); it != derd_from_cont.end(); ++it)
						if( m_modifiedObjects.end() == m_modifiedObjects.find( *it)) // not found
							m_modifiedObjects.insert( *it);

				}
			}
			//
			// we have alternatives as follows regarding consistency maintanance:
			// 1. We could require any NewTarget object (where the reference will point)
			//    to be neither locked, neither latent changed (aka modified already (by other users))
			//    thus we could be sure, that NewTarget version we see is still the latest one.
			//    BeforeSave: NewTarget has been added as "REFERRED" to the protectList
			//                so it is not possible for others to delete it anymore.
			//                NewTarget is not necessarily checked out (only if we decide so)
			//                Reference will change, so it is needed to be checked out, thus locked.
			//    AfterSave : Reference changed, checked back, for others became a latent change.
			//                NewTarget will remain unchanged, (even if we checked it out, upon checkin
			//                the same file contents are saved back, no new version is created.
			//    No deletion/removal can happen until this protection goes away from the list
			//    Pro:  - upon redirection no complex operations take place
			//    Con:  - NewTarget is required to be untouched by others
			// 2. We could check upon any redirection the state of the protectionlists,
			//    to see if NewTarget was deleted. If was moved elsewhere, that is not too relevant.
			//    BeforeSave: NewTarget has been added as "REFERRED" to the protectionlist, thus
			//                preventing later deletion by others.
			//                Reference will change, so it is needed to be checked out, thus locked.
			//                NewTarget would not be changed, its state is not required to be anything
			//                special.
			//    AfterSave:  Reference changed, checked back, for others became a latent change.
			//                NewTarget will remain unchanged, (unless something special in case it can be performed)
			//    No deletion can happen on NewTarget, because it has been referred, so its on the protection list.
			//    Pro: - NewTarget is not necessarily required to be available to us when referred, allows for better
			//           parralel work of the team.
			//    Con: - checking all users protection lists upon for REFERRED event could be slow
			//         - additional checks might be required to ensure that NewTarget's port children
			//           are not deleted by other users, while we would connect them through Reference's 
			//           reference ports, this might lead to inconsistent connections (src/dst could be non-existent)
			//  Let us implement #1 with the NewTarget checked out option, to make sure we see the latest version of it.
			//  Extension later: if our NewTarget is not the latest (we can't modify it), then we browse the 
			//                   protection list to make sure it was NOT deleted, and only then allow referral to it.
			//                   When the paradigm allows references to point to references, and if NewTarget is also
			//                   a reference, we must make sure, that it hasn't been redirected either.
			//  m_userOpts.m_doRefTargetLock=true must be used in this case
			if( m_metaAttributeId == ATTRID_REFERENCE) // 505
			{
				// redirection needs to be taken care of !!!
				// it affects connections, going through these refs
				metaid_type &mt = m_openedObject->m_metaid;

				// simply get its target, if filled
				AttribMapIter it3 = m_openedObject->m_attributes.find( ATTRID_REFERENCE );
				ASSERT( it3 != m_openedObject->m_attributes.end() );
				XmlObject * old_target = ((XmlAttrPointer*)(it3->second))->m_parent;

				// is removal (delete), is movement or is childbirth :) ?
				//metaobjidpair_type idpair;CopyTo(p, idpair);
				//bool is_removal = idpair.metaid == 0 && idpair.objid == 0; // VARIANT p is 0 => removal
				bool is_redirection = old_target != 0; // clear is also redirection
				if( is_redirection)
				{
					protect( m_openedObject, ELEM_REF_REDIRECTED); // might be useful for connections

					// commented on Sept.3.
					//XmlObjSet to_be_checked_out_containers;
					//XmlObjSet fdfdf;
					// 
					// find all references to this reference
					// to prevent breaking connections which
					//fdfdf.insert( m_openedObject);
					// upon a reference delete first the ATTRID_PARENT = 0 comes, later comes the
					// ATTRID_REFERENCE = 0, so this might confuse getMyDepObjConts, it will
					// find its container be 0
					if( std::find( m_deldObjs.begin(), m_deldObjs.end(), m_openedObject) != m_deldObjs.end()) // found
					{
						int l = 0;
						++l;
					}
					//getMyDepObjConts( fdfdf, to_be_checked_out_containers, true);
					//m_modifiedObjects.insert( to_be_checked_out_containers.begin(), to_be_checked_out_containers.end());
				}

				metaobjidpair_type idpair;CopyTo(p, idpair);
				XmlObject * new_target_obj = objectFromObjId( idpair);

				if( new_target_obj) protect( new_target_obj, ELEM_REFERRED);

				ASSERT( m_userOpts.m_doRefTargetLock);// currently we assume this

				if( m_userOpts.m_doRefTargetLock)
				{
					// new target also to be locked in this case (is it a model/or nonmodel?)
					if( new_target_obj) m_modifiedObjects.insert( new_target_obj);
				}
			}
			// DERIVATION
			// Let B be a base container, derived into D. B has C1 and C2 children. C2 has
			// C3 and C4 as children. B's parent is A.
			// When B is derived locks are required for B, C1, C2, C3, C4.
			// Small modification is made to all these objects (this will prevent
			// further standalone subtyping of Cx-s)
			// A's standalone subtyping will be prevented by the modified inner objects (B, Cx-s)
			// Before Save: objects are locked
			// After  Save: objects become RO (read-only)
			// After reopen: objects are already basetypes of D, DC1 (secondary derived peer of C1)
			// DC2, ..., DC4, thus conflicts are prevented by their presence.
			// Q: What happens if after reopen a user deleted D and derives A?
			if( m_metaAttributeId == ATTRID_DERIVED) // 510, 517?
			{
				metaobjidpair_type idpair;CopyTo(p, idpair);
				bool is_derivation = idpair.metaid != 0 && idpair.objid != 0; // VARIANT p is 0 => detach
				XmlObject * base_obj = objectFromObjId( idpair);
				if( is_derivation && base_obj != 0)
				{
					AttribMapIter it3 = m_openedObject->m_attributes.find( ATTRID_RELID );
					ASSERT( it3 != m_openedObject->m_attributes.end() );
					long relid = ((XmlAttrLong*)(it3->second))->m_value;
					bool secondary_deriv = relid > RELID_BASE_MAX;

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
					if( m_userOpts.m_createLog) {
						if( relid > RELID_BASE_MAX)
							mylo += " <br>- !!!! secondary derivation took place !!!! - <br>" ;
						else
							mylo += " <br>- !!!! primary derivation took place !!!! - <br>" ;
					}
#endif
#endif
					if( !secondary_deriv)
					{
						//
						// m_userOpts.m_doBaseTypeLock // this could be used too
						//
						// try to lock the hierarchy of base_obj to prevent concurrent derivations
						// of objects on different levels: e.g. parent derd by userA, child derd by userB
						XmlObjSet checkOutBases;
						XmlObjSet fdfdf;
						XmlObjSet to_be_checked_out_containers;
						// find all dependents of parent (? and m_openedObject)
						// where the deleted port can be seen
						//fdfdf.insert( parent); 
						//fdfdf.insert( m_openedObject);
						fdfdf.insert( base_obj);

						protect( base_obj, ELEM_DERIVED);

						getBasesOf( fdfdf, checkOutBases);
						getAllUpAndDown( fdfdf, to_be_checked_out_containers);to_be_checked_out_containers.insert( checkOutBases.begin(), checkOutBases.end());
						// perform a dummy operation on all containers
						for( XmlObjSet::const_iterator it = to_be_checked_out_containers.begin(); it != to_be_checked_out_containers.end(); ++it)
						{
							XmlObject * p = *it;
							AttribMapIter ait = p->m_attributes.find( ATTRID_RELID);
							if( ait != p->m_attributes.end())
							{
								// dummy operation
								long relid = ((XmlAttrLong*)(ait->second))->m_value;
								((XmlAttrLong*)(ait->second))->m_value = relid + 1;
								long reljd = ((XmlAttrLong*)(ait->second))->m_value;
								ASSERT( relid + 1 == reljd);
							}
						}
						m_modifiedObjects.insert( to_be_checked_out_containers.begin(), to_be_checked_out_containers.end());
					}
				}
			}
			// CONNECTION
			// Let S and D be source and destination.
			// Four scenarios exist regarding their connectibility:
			// Connection between:
			// 1.S and/or D as standalone objects
			// 2.S and/or D exposed as ports in their parents (MS and MD their parents)
			// 3.S and/or D exposed as ports of a reference to MS respectively MD 
			//       (Let RMS and RMD be these references.)
			// 4.S and/or D exposed as ports of a reference to RMS respectively RMD
			//       (Yes, references to references, which in turn reference a model, are also
			//        suitable for creating connections.)
			//
			// If S and D are checkoutable, that means we see their latest version
			//     (more specifically MS and MD except if S and D are models)
			//     For cases 3 and 4 add RMS and RRMS respectively RMD, RRMD.
			// If checkoutable is false, then protection lists could be analyzed for
			//     DEL/MOVE events.
			//     For cases 3 and 4 add REDIR event checkup on RMS/RRMS resp. RMD/RRMD.
			// Before Save: objects are locked, plus the parent of the connection.
			// After  Save: all objects become lockables again (checkoutable) except
			//              the connection parent.
			// CEnd, CSeq policy means that we lock S, D, RMS, RMD (in 3rd case), 
			// RRMS and RRMD (in 4th case)
			if( m_openedObject->m_metaid == DTID_CONNROLE && m_metaAttributeId == ATTRID_FCOREFATTR) // 525 or ATTRID_XREF
			{
				metaobjidpair_type idpair;CopyTo(p, idpair);
				bool is_conn = idpair.metaid != 0 && idpair.objid != 0; // VARIANT p is 0 => owner connection is being deleted
				XmlObject * end_obj = objectFromObjId( idpair);

				protect( end_obj, ELEM_CONNECTED);

				if( is_conn && end_obj != 0 && m_userOpts.m_doConnEndPointLock)
				{
					m_modifiedObjects.insert( end_obj);
#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
					if( m_userOpts.m_createLog) {
						std::string nn,mm; end_obj->m_attributes.find( ATTRID_NAME )->second->toString( nn);
						end_obj->m_attributes.find(ATTRID_NAME)->second->toString(mm);
						mylo += "<br>connecting through endpoint " + mm + " thus locking it <br>" ;
					}
#endif
#endif

				}
			}

			if( m_openedObject->m_metaid == DTID_CONNROLESEG && m_metaAttributeId == ATTRID_SEGREF) // 511, the Reference Involved
			{
				metaobjidpair_type idpair;CopyTo(p, idpair);
				bool is_filled = idpair.metaid != 0 && idpair.objid != 0; // VARIANT p is 0 => owner connection is being deleted
				bool is_a_ref  = idpair.metaid == DTID_REFERENCE; // a conn seg must point by its SEGREF to a Ref
				XmlObject * segment_ref = objectFromObjId( idpair);

				protect( segment_ref, ELEM_TAKESPARTINCONN);

				if( is_filled && is_a_ref && segment_ref != 0 && m_userOpts.m_doConnSegmentLock)
				{
					m_modifiedObjects.insert( segment_ref);
#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
					if( m_userOpts.m_createLog) {
						std::string nn,mm; segment_ref->m_attributes.find( ATTRID_NAME )->second->toString( nn);
						segment_ref->m_attributes.find(ATTRID_NAME)->second->toString(mm);
						mylo += "<br>connecting through segment ref " + mm + " thus locking it <br>" ;
					}
#endif
#endif

				}
			}

			m_modifiedObjects.insert( m_openedObject );

			// handle special case: if this is the containment parent and this object is an inner
			// we have to check out the parent too
			ParentMap::iterator it2 = m_parentMap.find( m_openedObject->m_metaid );
			ASSERT( it2 != m_parentMap.end() );
			if( it2->second == m_metaAttributeId && !m_openedObject->isContainer() )
			{
				XmlObject * cont = getContainer(m_openedObject);
				m_modifiedObjects.insert( cont );
			}

			XmlAttrPointer * pointerAttr = (XmlAttrPointer*)it->second;
			if( m_undoMap.find(it->second) == m_undoMap.end() )
			{
				std::pair<UndoMapIter, bool> t = m_undoMap.insert( UndoMap::value_type(it->second,UndoItem(m_openedObject, m_metaAttributeId, CComVariant()) ));
				getPointer( pointerAttr, PutOut(t.first->second.m_value) );
			}
			XmlObject * parnt = setPointer( p );
		}
		else if( m_metaAttributeValType == VALTYPE_COLLECTION )
		{
			ASSERT( false );
		}
		else if( m_metaAttributeValType == VALTYPE_LOCK )
		{
			if( m_undoMap.find(it->second) == m_undoMap.end() )
			{
				std::pair<UndoMapIter, bool> t = m_undoMap.insert( UndoMap::value_type(it->second,UndoItem(m_openedObject, m_metaAttributeId, CComVariant()) ));
				it->second->toVariant( PutOut(t.first->second.m_value) );
			}

			it->second->fromVariant(p);

			//TODO: if locks go down to 0 it could be written out to the file
		}
		else // VALTYPE_BIN, VALTYPE_STR, VALTYPE_LONG, VALTYPE_REAL
		{
#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
			if( m_userOpts.m_createLog) {
				mylo += " - valtype_StLoRe " ;
			}
#endif
#endif
			m_modifiedObjects.insert( m_openedObject );

			if( !m_openedObject->m_loaded )
			{
				fullReadContainer(getContainer(m_openedObject));
				it = m_openedObject->m_attributes.find( m_metaAttributeId );
			}

			// save previous value to m_undoMap (if this is the first modification)
			if( m_undoMap.find(it->second) == m_undoMap.end() )
			{
				std::pair<UndoMapIter, bool> t = m_undoMap.insert( UndoMap::value_type(it->second,UndoItem(m_openedObject, m_metaAttributeId, CComVariant()) ));
				it->second->toVariant( PutOut(t.first->second.m_value) );
			}
			it->second->fromVariant(p);
		}

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
		if( m_userOpts.m_createLog) {
			mylo += "\r\n";
			if( m_metaAttributeValType != VALTYPE_LOCK) //3
				mylog += mylo;
		}
//#endif
//#endif
		std::string gd_str; guid2str( m_openedObject->m_guid, gd_str);
		char* kind[] = {"100", "M", "A", "R", "C", "S", "F", "connrole", "connseg"
			, "109", "110", "StrAttr", "IntAttr", "FloatAttr", "BoolAttr", "RrefAttr", "116", "117", "118", "119"
			, "Constraint", "RegNode", "SetNode", "123", "124", "125", "126", "127" };

		char *  oper[] = { "VALTYPE_None", "VALTYPE_COLL", "VALTYPE_Pointer", "VALTYPE_LOCK", "VALTYPE_LONG"
			, "VALTYPE_STR", "VALTYPE_BIN", "VALTYPE_REAL", "VALTYPE_8", "VALTYPE_9", "VALTYPE_10", "VALTYPE_11"
			, "VALTYPE_12", "VALTYPE_13", "VALTYPE_14"};

		char mattrid[5];sprintf( mattrid, "%i", m_metaAttributeId);
		std::string kind_str = ((m_openedObject->m_metaid<=100)?"Root": kind[m_openedObject->m_metaid-100]);
		std::string oper_str = oper[m_metaAttributeValType];

		time( &time2); tm2 = localtime( &time2);
		double dur = difftime( time2, time1);
		char buff[100];
		sprintf( buff, "putAttribute [[%6.0f secs]] %s {%s} [ %s ] (%s)", dur, gd_str.c_str(), kind_str.c_str(), mattrid, oper_str.c_str());

		if( m_userOpts.m_measureTime)
			sendMsg( buff, MSG_INFO);
#endif
#endif

		m_openedObject->m_modified = true;
		m_modified = true;
	}
	COMCATCH(;)
}

STDMETHODIMP CCoreXmlFile::OpenObject(objid_type objid)
{
	if( m_metaObject == NULL || !m_inTransaction )
		COMRETURN(E_INVALID_USAGE);

	metaobjidpair_type idpair;
	idpair.metaid = m_metaObjectId;
	idpair.objid  = objid;

	COMTRY
	{
		m_openedObject = objectFromObjId(idpair);

		if( m_openedObject->m_deleted )
			m_openedObject = NULL;
	}
	COMCATCH(;)
}

STDMETHODIMP CCoreXmlFile::CreateObject(objid_type *objid)
{
	CHECK_OUT(objid);

	if( m_metaObject == NULL || !m_inTransaction )
		COMRETURN(E_INVALID_USAGE);

	ASSERT( m_metaObjectId != METAID_ROOT );

	COMTRY
	{                
		// create and add new object
		XmlObject * obj = new XmlObject(m_metaObject,true);
		addObject( obj );

		m_createdObjects.push_back(obj);

		m_modified = true;
		m_openedObject = obj;
		*objid = (long)obj;

		if( obj->m_metaid==DTID_MODEL || obj->m_metaid==DTID_FOLDER )
		{
			resetSourceControlInfo( obj );
			resetSourceControlStat( obj, true );
		}
	}
	COMCATCH(;)
}

STDMETHODIMP CCoreXmlFile::CloseObject()
{
	m_openedObject = NULL;
	return S_OK;
}

STDMETHODIMP CCoreXmlFile::LockObject()
{
	return S_OK;
}

STDMETHODIMP CCoreXmlFile::DeleteObject()
{
	if( m_openedObject == NULL || !m_inTransaction )
		COMRETURN(E_INVALID_USAGE);

	m_openedObject->m_deleted = true;
	m_deletedObjects.insert( m_openedObject );

	m_modified = true;

	//m_modifiedObjects.insert( m_openedObject );
	// TODO: add container 
	//try {
	//	if( m_openedObject->isContainer())
	//	{
	//		// obj has its own file
	//		std::string path;
	//		getSourceSafePath( m_openedObject, path);
	//		CComBSTR vssPath = path.c_str();

	//		// obtain file handle in VSS
	//		CComObjPtr<IVSSItem> item;
	//		COMTHROW( m_vssDatabase->get_VSSItem( vssPath, false, &(item.p)) );

	//		FILE * f = fopen( (m_folderPath + "\\tombstone.txt").c_str(), "a+b");
	//		if( f)
	//		{
	//			std::string fname;
	//			getContainerFileName( m_openedObject, fname, false);
	//			fprintf( f, "%s\n", fname.c_str());
	//			fclose( f);
	//		}
	//		// this solution is not good, since for all users but this the local .xml file
	//		// will be unaffected: if the vss entry is renamed, upon getLatest an obsolete
	//		// file won't be overwritten by its newer (which shows that it is deleted) version
	//		//CComBSTR nm;
	//		//COMTHROW( item->get_Name( &nm));
	//		//nm.Append( ".del"); // append this extension to the filename
	//		//COMTHROW( item->put_Name( nm));
	//	}
	//} catch( hresult_exception&) {
	//	int l = 0;
	//	++l;
	//}
	CloseObject();

	return S_OK;
}

void CCoreXmlFile::resetSettings()
{
	m_svnByAPI              = false;
	m_hashFileNames         = false;
	m_hashInfoFound         = false;
	m_hashVal               = -1;
	m_comSvn = CComPtr<ISvnExec>( 0);
	if( m_cmdSvn)
	{
		delete m_cmdSvn;
		m_cmdSvn = 0;
	}

}

STDMETHODIMP CCoreXmlFile::OpenProject(BSTR connection, VARIANT_BOOL *ro_mode)
{
	if( m_opened || m_metaProject == NULL )
		COMRETURN(E_INVALID_USAGE);

	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	resetSettings();

	parseConnectionString( connection );
	setFileNames();

	m_userOpts.reset();
	m_userOpts.load( m_folderPath);
	m_userOpts.display( this);

	readProjectFile();

	bool cache_loading_succeeded = false;
	if( m_userOpts.m_partialLoad)
		cache_loading_succeeded = readBinaryCache();
	// if usecache option is false or if cache not read succesfully

	//if( !m_userOpts.m_partialLoad)
	if( !m_userOpts.m_partialLoad || !cache_loading_succeeded) // part_load was not requested or it was, but failed
		//if( !m_userOpts.m_partialLoad || !readBinaryCache())
		//if( true )
	{                
		// binary cache is not found, get latest and read all
		if( m_sourceControl != SC_NONE )
			getLatestVersion();

		readAll( true );
		//if( m_userOpts.m_partialLoad) writeBinaryCache();
	}
	else
	{
		if( m_sourceControl != SC_NONE )
			getLatestVersion();        

		readAll( false );
		//// graph has been read  successfully, get latest and update graph
		//if( m_sourceControl != SC_NONE )
		//    getLatestAndLoad();
	}

	// m_sourceControl has to be filled for these methods below (setParent)
	m_signer.setParent( this);
	m_signer.in(); // signing on does username verification also
	m_protectList.setParent( this);

	// purge my protect list
	m_protectList.onLoad();

	m_opened    = true;
	m_modified  = false;
	m_savedOnce = true;

	if(ro_mode!=NULL) 
		*ro_mode = VARIANT_FALSE;

	return S_OK;
}

STDMETHODIMP CCoreXmlFile::CreateProject(BSTR connection)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	resetSettings();

	if( m_opened || m_metaProject == NULL )
		COMRETURN(E_INVALID_USAGE);

	parseConnectionString( connection );
	setFileNames();
	m_hashInfoFound = true;     // upon creation we select the hash/nonhash question
	m_svnAccessMethodFound = true; // and the access method too
	// user options can't be provided in this scenario
	//m_userOpts.load( m_folderPath, this);

	// clear data structures
	clearAll();

	// query the metaobject for the root
	CComObjPtr<ICoreMetaObject> mo;
	COMTHROW( m_metaProject->get_Object(METAID_ROOT, PutOut(mo)) );
	ASSERT( mo != NULL );

	// create the root
	m_root = new XmlObject(mo,true);
	addObject( m_root );

	if( m_clearCaseString.size() > 0 )
		createClearCaseProj();
	else if( m_svnUrl.size() > 0)
		createSubversionedFolder();
	else if( m_vssDatabaseStr.size() > 0 )
		createSourceSafeDatabase();
	else //AfxMessageBox( "Project has not been created under a source control system");
		createNonversioned();

	createProjectFile();

	// m_sourceControl has to be filled for these methods below (setParent)
	m_signer.setParent( this);
	m_signer.in(); // signing on does username verification also
	m_protectList.setParent( this);

	m_opened   = true;
	m_modified = false;

	return S_OK;
}

STDMETHODIMP CCoreXmlFile::SaveProject(BSTR connection, VARIANT_BOOL keepoldname = VARIANT_TRUE) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// reload options in order to allow 'change-of-mind' for users
	m_userOpts.reset();
	m_userOpts.load( m_folderPath);
	m_userOpts.display( this);

	if( m_userOpts.m_partialLoad) writeBinaryCache();
	writeAll();

	if( m_sourceControl != SC_NONE )
	{
		if( !m_savedOnce )
			checkInAll(true);
		else
			checkInAll();

	}

	m_modified = false;
	m_savedOnce = true;

	return S_OK;
}

STDMETHODIMP CCoreXmlFile::CloseProject( VARIANT_BOOL abort)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if( !m_opened || m_metaProject == NULL )
		COMRETURN(E_INVALID_USAGE);

	COMTRY
	{
		/*if( abort == VARIANT_FALSE && m_modified ) 
		SaveProject(NULL);

		if( m_sourceControl != SC_NONE )
		checkInAll();*/

		// purge my protect list
		m_protectList.onLoad();

		m_signer.off();
		clearAll();
		resetSettings();
		XMLPlatformUtils::Terminate();
	}
	COMCATCH(;)

		return S_OK;
}

STDMETHODIMP CCoreXmlFile::DeleteProject()
{ 
	return E_NOTIMPL;
}

STDMETHODIMP CCoreXmlFile::BeginTransaction()
{    
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if( !m_opened || m_inTransaction )
		COMRETURN(E_INVALID_USAGE);
	m_inTransaction = true;
	m_trivialChanges = true;
	m_fullLockNeeded = false;
	return S_OK;
}

STDMETHODIMP CCoreXmlFile::CommitTransaction()
{    
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if( !m_inTransaction )
		COMRETURN(E_INVALID_USAGE);

	ASSERT( m_opened );

	bool failed = false;

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
	time_t time1, time2, time3;
	struct tm *tm1, *tm2, *tm3;
	time( &time1); tm1 = localtime( &time1);
	//if( m_userOpts.m_measureTime) sendMsg( std::string( "CommitBegin ") + asctime( tm1 ), MSG_INFO);
#endif
#endif

	XmlObjSet to_be_checked_out_containers;
	getCheckOutContainers(m_modifiedObjects, to_be_checked_out_containers);

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
	time( &time2); tm2 = localtime( &time2);
	//if( m_userOpts.m_measureTime) sendMsg( std::string( "CommitMidle ") + asctime( tm2 ), MSG_INFO);
#endif
#endif

	CloseObject();

	//m_needClose = false;

	if( !checkOutFiles(to_be_checked_out_containers) )
	{
#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
		time( &time3);
		tm3 = localtime( &time3);

		double dur = difftime( time3, time1);
		char buff[100];
		sprintf( buff, " [Took total of  %6.0f secs]", dur );

		if( m_userOpts.m_measureTime) sendMsg( std::string( "CommitAbort ") + asctime( tm3) + buff, MSG_INFO);
#endif
#endif
		return E_FAIL;
	}
	else
	{
		m_trivialChanges = true;
		m_fullLockNeeded = false;
		m_createdObjects.clear();
		m_deletedObjects.clear();
		m_modifiedObjects.clear();
		m_undoMap.clear();
		m_inTransaction = false;

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)

		if( m_userOpts.m_createLog) {
			//while( -1 != mylog.find( "\r\n"))
			//{
			//    int p = mylog.find( "\r\n");
			//    mylog.replace( p, 2, "<br>");
			//}
			if( mylog.size() != 0 && mylog != "<br>")
				sendMsg( mylog, MSG_INFO);
			mylog.clear(); // clear the log
		}
#endif
#endif
		//if( m_needClose )
		//  m_gme->CloseProject( TRUE );

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
		time( &time3);
		tm3 = localtime( &time3);

		double dur = difftime( time3, time1);
		char buff[100];
		sprintf( buff, " [Took total of  %6.0f secs]", dur );

		if( m_userOpts.m_measureTime) sendMsg( std::string( "CommitSucce ") + asctime( tm3) + buff, MSG_INFO);
#endif
#endif
		m_protectList.onCommited();

		return S_OK;
	}
}

STDMETHODIMP CCoreXmlFile::AbortTransaction()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if( !m_inTransaction )
		COMRETURN(E_INVALID_USAGE);

	ASSERT( m_opened );

	// undelete deleted objects
	for( XmlObjSetIter it=m_deletedObjects.begin(); it!=m_deletedObjects.end(); ++it )
		(*it)->m_deleted = false;

	// delete created objects
	int minIndex = m_objects.size();
	for( XmlObjVecIter it2=m_createdObjects.begin(); it2!=m_createdObjects.end(); ++it2 )
	{
		XmlObject* obj = *it2;
		m_objectsByGUID.erase(obj->m_guid);
		if( obj->m_index < minIndex )
			minIndex = obj->m_index;
	}
	m_objects.resize( minIndex );

	// rollback attributum changes
	for( UndoMapIter it3=m_undoMap.begin(); it3!=m_undoMap.end(); ++it3 )
	{
		XmlObject * obj = it3->second.m_object;
		if( it3->first->getType() == VALTYPE_POINTER )
		{
			XmlAttrPointer * pointerAttr = (XmlAttrPointer*)it3->first;
			metaobjidpair_type idpair;
			CopyTo(it3->second.m_value, idpair);
			XmlObject * parent = objectFromObjId(idpair);
			setPointer( obj, it3->second.m_attrId, parent );
		}
		else
		{
			it3->first->fromVariant(it3->second.m_value);
		}
	}
	m_protectList.onAborted();
	m_undoMap.clear();

	m_deletedObjects.clear();
	m_createdObjects.clear();
	m_modifiedObjects.clear();
	m_trivialChanges = true;
	m_fullLockNeeded = false;
	CloseObject();    
	m_inTransaction = false;
	return S_OK;
}

STDMETHODIMP CCoreXmlFile::get_StorageType(long *p)
{
	CHECK_OUT(p);
	*p = 0;
	return S_OK;
}

void CCoreXmlFile::fillParentMap()
{
	m_parentMap.clear();

	m_parentMap.insert( ParentMap::value_type( DTID_CONSTRAINT , ATTRID_CONSTROWNER ) );
	m_parentMap.insert( ParentMap::value_type( DTID_REGNODE    , ATTRID_REGNOWNER   ) );
	m_parentMap.insert( ParentMap::value_type( DTID_FOLDER     , ATTRID_PARENT      ) );
	m_parentMap.insert( ParentMap::value_type( DTID_MODEL      , ATTRID_PARENT      ) );
	m_parentMap.insert( ParentMap::value_type( DTID_ATOM       , ATTRID_PARENT      ) );
	m_parentMap.insert( ParentMap::value_type( DTID_REFERENCE  , ATTRID_PARENT      ) );
	m_parentMap.insert( ParentMap::value_type( DTID_SET        , ATTRID_PARENT      ) );
	m_parentMap.insert( ParentMap::value_type( DTID_SETNODE    , ATTRID_SETMEMBER   ) );
	m_parentMap.insert( ParentMap::value_type( DTID_CONNECTION , ATTRID_PARENT      ) );
	m_parentMap.insert( ParentMap::value_type( DTID_CONNROLE   , ATTRID_CONNROLE    ) );
	m_parentMap.insert( ParentMap::value_type( DTID_CONNROLESEG, ATTRID_CONNSEG     ) );
	m_parentMap.insert( ParentMap::value_type( DTID_STRATTR    , ATTRID_ATTRPARENT  ) );
	m_parentMap.insert( ParentMap::value_type( DTID_INTATTR    , ATTRID_ATTRPARENT  ) );
	m_parentMap.insert( ParentMap::value_type( DTID_FLOATATTR  , ATTRID_ATTRPARENT  ) );
	m_parentMap.insert( ParentMap::value_type( DTID_BOOLATTR   , ATTRID_ATTRPARENT  ) );
	m_parentMap.insert( ParentMap::value_type( DTID_REFATTR    , ATTRID_ATTRPARENT  ) );
}

void CCoreXmlFile::closeMetaProject()
{
	closeMetaObject();
	CloseProject();
	m_metaProject = NULL;
}

void CCoreXmlFile::openMetaObject()
{
	ASSERT( m_metaObject != NULL );
	COMTHROW( m_metaObject->get_MetaID(&m_metaObjectId) );
}

void CCoreXmlFile::closeMetaObject()
{
	CloseObject();
	closeMetaAttribute();
	m_metaObject   = NULL;
	m_metaObjectId = METAID_NONE;
}

void CCoreXmlFile::openMetaAttribute()
{
	ASSERT( m_metaAttribute != NULL );
	COMTHROW( m_metaAttribute->get_AttrID(&m_metaAttributeId) );
	COMTHROW( m_metaAttribute->get_ValueType(&m_metaAttributeValType) );

	if( m_metaAttributeId == ATTRID_NONE )
		HR_THROW(E_METAPROJECT);
}

void CCoreXmlFile::closeMetaAttribute()
{
	m_metaAttribute        = NULL;
	m_metaAttributeId      = ATTRID_NONE;
	m_metaAttributeValType = VALTYPE_NONE;
}

void CCoreXmlFile::parseConnectionString( BSTR connection )
{   
	// connection string format:
	// START   = PAIRMGX | PAIR2
	// PAIR2   = PAIR PAIR2 | empty
	// PAIR    = KEY '=' VAL
	// PAIRMGX = 'MGX=' string
	// KEY     = string
	// VAL     = '"' string '"'
	//
	// valid key values: MGX, 
	//
	// example: 
	//   MGX="C:\temp\test1" vssdatabase="frfre" vsspath="dede" user="fdasfdsfds" password="defrefre"
	//   MGX="C:\temp\test1" svn="svn://localhost/"
	std::string conn;
	CopyTo(connection, conn);
	const char * connectionString = conn.c_str();

	if( strncmp( connectionString, "MGX=", 4 ) != 0 )
		HR_THROW(E_INVALID_USAGE);

	int          size = conn.size();
	std::string       key;
	std::string       val;
	bool         keyCollecting = true;
	bool         startedValue  = false;

	m_contentPath    = "";
	m_folderPath     = "";
	m_vssUser        = "";
	m_vssPassword    = "";
	m_vssDatabaseStr = "";
	m_vssParentPath  = "";
	m_svnUrl         = "";

	std::string to_hash = "";
	std::string to_hash_with_val = "";
	m_svnAccessMethodFound = false;

	for( int i=0; i<size; ++i )
	{
		char ch = connectionString[i];
		if( keyCollecting )
		{
			if( ch == '=' )
			{
				keyCollecting = false;
				startedValue  = false;
				val = "";
			}
			else
			{
				if( ch != ' ' && ch != '\t' )
					key += ch;
			}
		}
		else
		{
			if( startedValue )
			{
				if( ch != '\"' )
					val += ch;
				else
				{
					// key-val pair is finished
					if( stricmp( key.c_str(), "MGX" ) == 0 )
						m_folderPath = val;
					else if( stricmp( key.c_str(), "user" ) == 0 )
						m_vssUser = val;
					else if( stricmp( key.c_str(), "password" ) == 0 )
						m_vssPassword = val;
					else if( stricmp( key.c_str(), "vssDatabase" ) == 0 )
						m_vssDatabaseStr = val;
					else if( stricmp( key.c_str(), "vssPath" ) == 0 )
						m_vssParentPath = val;
					else if( stricmp( key.c_str(), "clearCase" ) == 0 )
						m_clearCaseString = val;
					else if( stricmp( key.c_str(), "svn" ) == 0 )
						m_svnUrl = val;
					else if( stricmp( key.c_str(), "hash") == 0 )
						to_hash = val;
					else if( stricmp( key.c_str(), "hval") == 0 )
						to_hash_with_val = val;
					else if( stricmp( key.c_str(), "svnaccess") == 0) {
						if( !m_svnAccessMethodFound) { 
							m_svnAccessMethodFound = true; 
							m_svnByAPI = val != "CMD";
						}
					}

					keyCollecting = true;
					key = "";
				}
			}
			else
			{
				if( ch == '\"' )
					startedValue = true;

			}
		}      
	}

	if( to_hash == "true")
	{
		m_contentPath = m_folderPath + "\\" + m_contentConst;
		m_hashFileNames = true;
		m_hashInfoFound = true;
		if( to_hash_with_val == "4096")
			m_hashVal = 5;
		else if( to_hash_with_val == "256")
			m_hashVal = 2;
		//else if( to_hash_with_val == "3")
		//	m_hashVal = 3;
		//else if( to_hash_with_val == "4")
		//	m_hashVal = 4;
		else
			HR_THROW( E_FILEOPEN);
	}
	else
		m_hashFileNames = false;
}

bool CCoreXmlFile::isUrlSvnSsh()
{
	return m_svnUrl.substr( 0, 10) == "svn+ssh://";
}

std::string CCoreXmlFile::userNameFromSvnSshUrl()
{
	size_t at_pos = m_svnUrl.find( '@');
	if( m_svnUrl.substr( 0, 10) == "svn+ssh://" && at_pos > 10 && at_pos != std::string::npos) // uname found
	{
		return m_svnUrl.substr( 10, at_pos - 10);
	}
	return "";
}

void CCoreXmlFile::svnSshHandling()
{
	if( m_svnUrl.substr( 0, 10) == "svn+ssh://" && m_svnUrl.find( '@') == std::string::npos) // uname not found
	{
		if( m_vssUser.empty())
		{
			AfxMessageBox( "Could not process further with \"svn+ssh://\" scheme if username is not provided\n\
either in the Credential Dialog or embedded in the url as \"svn+ssh://username@host.example.com\".");
			HR_THROW( E_UNKNOWN_STORAGE);
		}
		else
			m_svnUrl = std::string( "svn+ssh://") + m_vssUser + "@" + m_svnUrl.substr( 10);
	}
}

std::string CCoreXmlFile::svnSshMangling( const std::string& p_url)
{
	std::string ret;
	size_t at_pos = p_url.find( '@');
	if( p_url.substr( 0, 10) == "svn+ssh://" && at_pos != std::string::npos) // uname found
	{
		ret = std::string( "svn+ssh://") + p_url.substr( at_pos + 1);
		return ret;
	}
	return p_url;
}

void CCoreXmlFile::svnOptions()
{
	if( !m_svnUrl.empty())
	{
		if( !m_hashInfoFound)
		{
			m_hashFileNames = IDYES == AfxMessageBox( "Use hashed subdirectories?", MB_YESNO);
			if( m_hashFileNames)
			{
				m_hashVal = (IDYES == AfxMessageBox( "Does the project have 4096 subdirectories (1+2 digit hashing)?", MB_YESNO))?5:2;
				if( m_hashVal == 2) AfxMessageBox( "Defaulting to a project with 256 subdirectories (2 digit hashing).", MB_ICONINFORMATION);
				//if( m_hashVal == 2)
				//{
				//	m_hashVal = (IDYES == AfxMessageBox( "Use 4 digit hashed subdirectories?", MB_YESNO))?4:2;
				//	if( m_hashVal == 2)
				//		m_hashVal = (IDYES == AfxMessageBox( "Use 3 digit hashed subdirectories?", MB_YESNO))?3:2;
				//}
			}
		}

		if( !m_svnAccessMethodFound) // connection string might have contained it
		{
			m_svnByAPI = true;
			if( m_svnByAPI) m_svnByAPI = IDYES == AfxMessageBox( "Use Subversion through API?", MB_YESNO);
			else            m_svnByAPI = IDNO ==  AfxMessageBox( "Use Subversion through CMD.exe?", MB_YESNO);
		}

		if( !m_svnByAPI)
		{
			m_svnShowCmdLineWindows = IDYES == AfxMessageBox( "Show command line windows?", MB_YESNO);
			m_svnRedirectOutput     = !m_svnShowCmdLineWindows?true:IDYES == AfxMessageBox( "Redirect output to GME console?", MB_YESNO);
		}
	}
}

void CCoreXmlFile::setFileNames( bool p_reset /* = false */)
{  
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	// a conn string like 'c:\\t\\v\\p' comes in in m_folderPath
	// which is then divided in 'c' as drive, 't\\v' as dir
	// and 'p' as fname (although p is a directory) and '' as ext
	_splitpath( m_folderPath.c_str(), drive, dir, fname, ext );

	if( p_reset)
	{
		// m_projectFileName will be reset to a valid .mgx filename
		char                 buf[_MAX_PATH];
		_finddata_t          fileInfo;

		sprintf( buf, "%s\\*.mgx", m_folderPath.c_str() );

		long searchHandle = _findfirst( buf, &fileInfo ); // findfirst in project dir
		long ret = searchHandle;
		long count = 0;
		while( ret != -1 && ++count)
			ret = _findnext( searchHandle, &fileInfo );   // findnext

		if( count > 1)
			sendMsg( "Multiple .mgx files were found in directory " + m_folderPath, MSG_ERROR);
		else if( count == 0)
			sendMsg( "No .mgx file was found in directory " + m_folderPath, MSG_ERROR);
		else
		{
			// new m_projectFileName value set, a real one
			m_projectFileName = m_folderPath;
			m_projectFileName += "\\";
			m_projectFileName += fileInfo.name; // m_projectName or m_vssPath or m_parentFolderPath do NOT change

			sendMsg( "Loading project file: " + m_projectFileName, MSG_INFO);
			_findclose( searchHandle );
		}
		return;
	}

	m_parentFolderPath = drive;
	m_parentFolderPath += dir;
	if( m_parentFolderPath[m_parentFolderPath.size()-1] == '\\' )
		m_parentFolderPath.resize(m_parentFolderPath.size()-1);

	// this is the name of the project directory : 'p' in the example above
	m_projectName = fname;

	m_cacheFileName = m_folderPath + "\\project.bin";

	// the name of the project file siting in the project dir: 'c:\\t\\u\\p\\project.mgx'
	m_projectFileName = m_folderPath + "\\project.mgx";

	m_vssPath = m_vssParentPath;
	m_vssPath += "/";
	m_vssPath += m_projectName;
}

void CCoreXmlFile::getContainerFileName(XmlObject * obj, std::string& str, bool fullpath)
{
	ASSERT( obj->isContainer() );

	std::string guidStr;
	guid2str( obj->m_guid, guidStr );

	if( fullpath )
	{
		str = m_folderPath;
		str += "\\";
	}
	else
		str = "";

	if( m_hashFileNames && obj->m_metaid != METAID_ROOT) // hashing not applied for the root folder
	{
		str += m_contentConst;
		str += "\\";
		if( m_hashVal == 5) // 1+2 setup
		{
			str += guidStr.substr( 0, 1);
			str += "\\";
			str += guidStr.substr( 1, 2);
			str += "\\";
		}
		else if( m_hashVal == 2)
		{
			str += guidStr.substr( 0, 2);
			str += "\\";
		}
		else if( m_hashVal == 3)
		{
			str += guidStr.substr( 0, 1) + "\\" + guidStr.substr( 1, 1) + "\\" + guidStr.substr( 2, 1) + "\\";
		}
		else if( m_hashVal == 4)
		{
			str += guidStr.substr( 0, 1) + "\\" + guidStr.substr( 1, 1) + "\\" + guidStr.substr( 2, 1) + "\\" + guidStr.substr( 3, 1) + "\\";
		}
		//str += guidStr.substr( 0, 2);
		//str += "\\";
	}

	str += guidStr;
	str += ".xml";
}

void CCoreXmlFile::getContainerName(XmlObject * obj, string& name, string& type)
{
	CComObjPtr<ICoreMetaObject>     metaobject;
	CComBSTR                        metaToken;

	COMTHROW( m_metaProject->get_Object( obj->m_metaid, PutOut(metaobject) ) );
	COMTHROW( metaobject->get_Token( &metaToken ) );

	CopyTo(metaToken, type);

	name = "";
	AttribMapIter it;
	for( it=obj->m_attributes.begin(); it!=obj->m_attributes.end(); ++it )
	{
		XmlAttrBase                    * attr = it->second;
		CComObjPtr<ICoreMetaAttribute>   metaAttrib;
		CComBSTR                         attribToken;
		string                           attribToken2;
		string                           attrVal;

		COMTHROW( metaobject->get_Attribute( it->first, PutOut(metaAttrib) ) );
		metaAttrib->get_Token( &attribToken );

		CopyTo(attribToken, attribToken2);

		if( stricmp( attribToken2.c_str(), "name" ) == 0 && attr->getType() == VALTYPE_STRING )
		{
			XmlAttrBase * attr = it->second;
			attr->toString(name);
			break;
		}
	}
}

// method used to clear all objects before a new attempt to read a project.
// it is used by ~CCoreXmlFile, or ::CloseProject also.
void CCoreXmlFile::clearAll()
{
	for( XmlObjVecIter i=m_objects.begin(); i!=m_objects.end(); ++i )       
		delete (*i);
	m_objects.clear();
	m_objectsByGUID.clear();
	m_openedObject = NULL;
	m_root = NULL;
}

void CCoreXmlFile::addObject(XmlObject * obj)
{
	obj->m_index = m_objects.size();
	m_objects.push_back( obj );
	m_objectsByGUID.insert( GUIDToXmlObjectMap::value_type( obj->m_guid, obj ) );
}

void CCoreXmlFile::deleteObject(XmlObject * obj)
{
	// remove pointers
	for(AttribMapIter it=obj->m_attributes.begin(); it!=obj->m_attributes.end(); ++it)
	{
		if( it->second->getType() == VALTYPE_POINTER )
		{
			// remove it from parent object's collection
			XmlAttrPointer * pointer = (XmlAttrPointer*)it->second;
			if( pointer->m_parent != NULL )
			{
				AttribMapIter it2 = pointer->m_parent->m_attributes.find( it->first + ATTRID_COLLECTION );
				ASSERT( it2 != pointer->m_parent->m_attributes.end() );
				ASSERT( it2->second->getType() == VALTYPE_COLLECTION );
				((XmlAttrCollection*)it2->second)->m_children.erase(obj);
			}
		}
		else if( it->second->getType() == VALTYPE_COLLECTION )
		{
			XmlAttrCollection * collection = (XmlAttrCollection*)it->second;
			for( XmlObjSetIter it2=collection->m_children.begin(); it2!=collection->m_children.end(); ++it2 )
			{
				// set parent of this child to NULL
				XmlObject * obj2 = (*it2);
				AttribMapIter it3 = obj2->m_attributes.find( it->first - ATTRID_COLLECTION );
				ASSERT( it3 != obj2->m_attributes.end() );
				ASSERT( it3->second->getType() == VALTYPE_POINTER );
				((XmlAttrPointer *)(it3->second))->m_parent = NULL;
			}
		}
	}

	obj->m_deleted = true;
}

void CCoreXmlFile::setPointer(XmlObject * obj, attrid_type attribId, XmlObject * parent)
{
	ASSERT( obj!=NULL );

	AttribMapIter it = obj->m_attributes.find(attribId);
	ASSERT( it!=obj->m_attributes.end() );

	XmlAttrPointer * attr = (XmlAttrPointer*)it->second;

	// remove item from old parent's list
	if( attr->m_parent != NULL )
	{
		// find collection attribute of parent
		AttribMapIter it2 = attr->m_parent->m_attributes.find( attribId + ATTRID_COLLECTION );
		ASSERT( it2 != attr->m_parent->m_attributes.end() );
		ASSERT( it2->second->getType() == VALTYPE_COLLECTION );

		// remove this form the list
		((XmlAttrCollection *)it2->second)->m_children.erase(obj);
	}

	// set pointer attribute
	attr->m_parent = parent;

	// add item to new parent's list
	if( parent != NULL )
	{
		AttribMapIter it3 = parent->m_attributes.find( attribId + ATTRID_COLLECTION );
		ASSERT( it3 != parent->m_attributes.end() );
		ASSERT( it3->second->getType() == VALTYPE_COLLECTION );
		((XmlAttrCollection *)it3->second)->m_children.insert(obj);
	}
}

XmlObject * CCoreXmlFile::setPointer(VARIANT p)
{   
	metaobjidpair_type idpair;
	CopyTo(p, idpair);
	XmlObject * parent = objectFromObjId(idpair);
	setPointer( m_openedObject, m_metaAttributeId, parent );
	return parent;
}

void CCoreXmlFile::updateCollections()
{
	for( XmlObjVecIter it=m_objects.begin(); it!=m_objects.end(); ++it )
	{   
		XmlObject * obj = (*it);
		for( AttribMapIter j=obj->m_attributes.begin(); j!=obj->m_attributes.end(); ++j)
		{
			if( j->second->getType() == VALTYPE_POINTER )
			{
				XmlAttrPointer * pointer = (XmlAttrPointer*)j->second;
				if( pointer->m_parent != NULL )
				{
					AttribMapIter k = pointer->m_parent->m_attributes.find( j->first + ATTRID_COLLECTION );
					ASSERT( k != pointer->m_parent->m_attributes.end() );
					((XmlAttrCollection*)k->second)->m_children.insert( obj );
				}
			}
		}
	}
}

void CCoreXmlFile::resolvePointers(UnresolvedPointerVec& pointers)
{
#ifdef DEBUG
#if(RESOLVE_PTRS_2ND_ATTEMPT)
	UnresolvedPointerVec again_unresolved;
#endif
#endif

	for( UnresolvedPointerVecIt it=pointers.begin(); it!=pointers.end(); ++it )
	{
		if( it->m_pointedObjGuid == GUID_NULL )
		{
			setPointer( it->m_object, it->m_attrib, NULL );
		}
		else
		{
			GUIDToXmlObjectMapIter it2 = m_objectsByGUID.find( it->m_pointedObjGuid );
			if( it2 != m_objectsByGUID.end() )
			{
				setPointer( it->m_object, it->m_attrib, it2->second );
			}
			else
			{
				// TODO: invalid pointer, what to do?
#ifdef DEBUG
#if(RESOLVE_PTRS_2ND_ATTEMPT)
				again_unresolved.push_back( *it);
#endif
#endif
			}
		}
	}

	// analyze unresolved pointers in debug mode:
#ifdef DEBUG
#if(RESOLVE_PTRS_2ND_ATTEMPT)
	int still_unresolved = again_unresolved.size();
	ASSERT( still_unresolved == 0);                        // notify user!!!

	for( UnresolvedPointerVecIt it=again_unresolved.begin(); it!=again_unresolved.end(); ++it )
	{
		int k = 0;
		++k;
		if( it->m_object->m_metaid == DTID_CONNROLE && it->m_attrib == ATTRID_FCOREFATTR) // XREF, 525
		{
			// connEnd can't be resolved
			// find the connection it belongs to -> simply find its parent
			XmlObject * obj = it->m_object; int l = 0;
			while( obj != 0 && (obj->m_metaid != DTID_CONNECTION || obj->m_metaid == DTID_MODEL || obj->m_metaid == DTID_FOLDER))
			{
				ParentMap::iterator it1 = m_parentMap.find( obj->m_metaid );
				ASSERT( it1 != m_parentMap.end() );
				AttribMapIter it2 = obj->m_attributes.find( it1->second );
				ASSERT( it2 != obj->m_attributes.end() );
				obj = ((XmlAttrPointer*)(it2->second))->m_parent;
				++l;
			}
			ASSERT( l == 1);

			if( obj != 0 && obj->m_metaid == DTID_CONNECTION) // OK
			{
				int k = 0;
				++k;
				AttribMapIter pit = obj->m_attributes.find( ATTRID_PARENT);
				ASSERT( pit != obj->m_attributes.end());
				XmlObject * parent = ((XmlAttrPointer*)(pit->second))->m_parent;
				// delete it: set its parent to 0
				ASSERT(0); // notify user
				setPointer( obj, ATTRID_PARENT, 0);
			}
		}

		if( it->m_object->m_metaid == DTID_CONNROLESEG && it->m_attrib == ATTRID_SEGREF) // 511
		{
			XmlObject * obj = it->m_object; int l = 0;
			while( obj != 0 && (obj->m_metaid != DTID_CONNECTION || obj->m_metaid == DTID_MODEL || obj->m_metaid == DTID_FOLDER))
			{
				ParentMap::iterator it1 = m_parentMap.find( obj->m_metaid );
				ASSERT( it1 != m_parentMap.end() );
				AttribMapIter it2 = obj->m_attributes.find( it1->second );
				ASSERT( it2 != obj->m_attributes.end() );
				obj = ((XmlAttrPointer*)(it2->second))->m_parent;
				++l;
			}
			ASSERT( l == 2);

			if( obj != 0 && obj->m_metaid == DTID_CONNECTION) // OK
			{
				int k = 0;
				++k;
				// delete it: set its parent to 0
				ASSERT(0); // notify user
				setPointer( obj, ATTRID_PARENT, 0);
			}
		}

		if( it->m_object->m_metaid == DTID_REFERENCE && it->m_attrib == ATTRID_REFERENCE) // 505
		{
			// referred object is not found
			int k = 0;
			++k;
			ASSERT(0); // notify user
			// involved connections may have existed... :(
		}
		if( it->m_attrib == ATTRID_PARENT) // 602
		{
			// a child Y may have been lost its parent X, when another user deleted model X, not knowing 
			// that it has a Y child also
			// this refers to the case when children are added without locking parent (when not used ATTRID_LASTRELID)
			int k = 0;
			++k;
			ASSERT(0); // notify user
		}
	}
#endif
#endif
}

void CCoreXmlFile::resetSourceControlInfo( XmlObject * obj)
{
	ASSERT( obj->m_metaid==DTID_MODEL || obj->m_metaid==DTID_FOLDER );

	// set FILESTATUS attributes
	AttribMapIter fsit = obj->m_attributes.find( ATTRID_FILESTATUS );
	if( obj->m_attributes.end() != fsit)
	{
		XmlAttrLong * along = (XmlAttrLong*) fsit->second;
		along->m_value &= 0xFFFFFF00; // clear my bits (the lowest byte)
	}
}

void CCoreXmlFile::resetSourceControlForAll()
{
	// collect containers
	XmlObjVec containers;
	for( XmlObjVecIter it=m_objects.begin(); it!=m_objects.end(); ++it )
	{   
		XmlObject * obj = (*it);

		if( obj->m_metaid==DTID_MODEL || obj->m_metaid==DTID_FOLDER )
			containers.push_back( obj );
	}

	for( XmlObjVecIter it=containers.begin(); it!=containers.end(); ++it )
	{
		resetSourceControlInfo( *it );
		resetSourceControlStat( *it, false );
	}
}

void CCoreXmlFile::resetSourceControlStat(XmlObject * obj, bool p_freshObj)
{
	ASSERT( obj->m_metaid==DTID_MODEL || obj->m_metaid==DTID_FOLDER );

	// set FILESTATUS attributes
	AttribMapIter fsit = obj->m_attributes.find( ATTRID_FILESTATUS );
	if( obj->m_attributes.end() != fsit)
	{
		XmlAttrLong * along = (XmlAttrLong*) fsit->second;
		along->m_value &= 0xFFFF00FF; // clear my flags (the 2nd byte)

		// new objects get special status,
		// otherwise the default is 0 (only for the 2nd byte)
		int l = 1; 
		l |= p_freshObj ? 4:8;
		along->m_value |= ( p_freshObj ? FS_NOTYETSAVED : 0x0);
	}
}

void CCoreXmlFile::getPointer(XmlAttrPointer * attr, VARIANT * p)
{
	ASSERT( attr!= NULL );
	metaobjidpair_type id;
	objIdFromObject( attr->m_parent, id );
	CopyTo(id, p);
}

void CCoreXmlFile::getCollection(XmlAttrCollection * attr, VARIANT * p)
{
	ASSERT( attr!= NULL );
	std::vector<metaobjidpair_type> idpairs;
	for( XmlObjSetIter it = attr->m_children.begin(); it != attr->m_children.end(); ++it )
	{
		metaobjidpair_type id;
		objIdFromObject( *it, id );
		idpairs.push_back( id );
	}
	CopyTo(idpairs, p);
}

// TODO: test!
XmlObject * CCoreXmlFile::getContainer(XmlObject * obj)
{
	ASSERT( obj != NULL );
	XmlObject * container = obj;
	while( container!=NULL && !container->isContainer() )
	{
		ParentMap::iterator it = m_parentMap.find( container->m_metaid );
		ASSERT( it != m_parentMap.end() );
		AttribMapIter it2 = container->m_attributes.find( it->second );
		ASSERT( it2 != container->m_attributes.end() );
		container = ((XmlAttrPointer*)(it2->second))->m_parent;    
	}
	return container;
}

// TODO: test!
void CCoreXmlFile::getContainedObjects(XmlObject * obj, XmlObjVec& vec)
{
	ASSERT( obj != NULL );
	vec.push_back(obj);

	for(AttribMapIter it=obj->m_attributes.begin(); it!=obj->m_attributes.end(); ++it)
	{
		if( it->second->getType() == VALTYPE_COLLECTION )
		{
			XmlAttrCollection * coll = (XmlAttrCollection *)it->second;
			for( XmlObjSetIter it2=coll->m_children.begin(); it2!=coll->m_children.end(); ++it2 )
			{
				XmlObject * obj2 = (*it2);
				if( obj2->m_metaid != DTID_MODEL && obj2->m_metaid != DTID_FOLDER )
				{
					ParentMap::iterator it3 = m_parentMap.find( obj2->m_metaid );
					ASSERT( it3 != m_parentMap.end() );
					if( it3->first + ATTRID_COLLECTION == it->first )
						getContainedObjects(obj2,vec);
				}
			}
		}
	}
}

void CCoreXmlFile::getMyDepObjConts( XmlObjSet& objects, XmlObjSet& containers, bool thorough)
{
	XmlObjSet processedObjects;
	for( XmlObjSetIter it=objects.begin(); it!=objects.end(); ++it )
		getMyDepObj( *it, containers, processedObjects, thorough );

	for( XmlObjSetIter jt = processedObjects.begin(); jt != processedObjects.end(); ++jt)
	{
		XmlObject * cont = getContainer( *jt);
		//ASSERT( cont); 
		// cont can be 0, e.g. if an reference object is deleted, so its parent becomes 0, 
		// then its ATTR_REF pointer is set to 0, that's why end up in this method
		if( cont && containers.find( cont) == containers.end()) // not inserted yet
			containers.insert( cont);
	}
}

void CCoreXmlFile::getMyDepObj(XmlObject * obj, XmlObjSet& containers, XmlObjSet& processedObjects, bool thorough)
{
	if( obj!= NULL && processedObjects.find(obj)==processedObjects.end() )  // not processed yet
	{
		processedObjects.insert(obj);
		XmlObject * container = obj;//getContainer(obj);
		if( container != NULL )
		{
			//containers.insert( container );
			for(AttribMapIter it=container->m_attributes.begin(); thorough && it!=container->m_attributes.end(); ++it)
			{ 
				attrid_type at = it->first;

				// go through the collections only if non-trivial changes happened
				//
				// for a generic fco these collections are:
				//CREATE_COLLECTION(ATTRID_CONSTROWNER, "Constraints", "Constraints");
				//CREATE_COLLECTION(ATTRID_REGNOWNER, "RegNodes", "Registry Nodes");
				//CREATE_COLLECTION(ATTRID_ATTRPARENT,"Attributes", "Attributes");
				//CREATE_COLLECTION(ATTRID_REFERENCE, "References", "Referenced by");
				//CREATE_COLLECTION(ATTRID_XREF,		"XReferences", "Cross refs");
				//CREATE_COLLECTION(ATTRID_DERIVED, "SubTypes", "SubTypes/Instances"); 
				//
				// for a MODEL additonal collection is:
				//CREATE_COLLECTION(ATTRID_PARENT, "Children", "Child Objects");
				//
				// for a REFERENCE additional collection is:
				//CREATE_COLLECTION(ATTRID_SEGREF, "Segments", "Connection Segments");
				//CREATE_COLLECTION(ATTRID_MASTEROBJ, "MasterOf", "Master Of Objects");
				//
				// for a SET is
				//CREATE_COLLECTION(ATTRID_SETMEMBER, "Members", "Set Members");

				if(      at == ATTRID_PARENT      + ATTRID_COLLECTION) { } // this time children disregarded
				else if( at == ATTRID_CONSTROWNER + ATTRID_COLLECTION) { }
				else if( at == ATTRID_REGNOWNER   + ATTRID_COLLECTION) { }
				else if( at == ATTRID_ATTRPARENT  + ATTRID_COLLECTION) { }
				else
					if( it->second->getType() == VALTYPE_COLLECTION)
					{
						// XREF, DERIVED, REFERENCE, SEGREF, MASTEROBJ, SETMEMBER
						XmlAttrCollection * coll = (XmlAttrCollection*)it->second;
						for( XmlObjSetIter it2=coll->m_children.begin(); it2!=coll->m_children.end(); ++it2 )
						{
							XmlObject * obj2 = (*it2);
							if( obj2 != NULL )
								getMyDepObj( obj2, containers, processedObjects, thorough );
						}
					}
			}
		}
	}
}

void CCoreXmlFile::getBasesOfObj( XmlObject * obj, XmlObjSet& containers)
{
	if( obj!= NULL)
	{
		if( obj->m_metaid == DTID_MODEL && containers.find( obj) == containers.end())
			containers.insert( obj);

		AttribMapIter derattr = obj->m_attributes.find( ATTRID_DERIVED);
		if( derattr != obj->m_attributes.end())
		{
			XmlObject * base = ((XmlAttrPointer*)(derattr->second))->m_parent;
			if( base)
			{
				getBasesOfObj( base, containers);
			}
		}
	}
}

void CCoreXmlFile::getBasesOf( XmlObjSet& objects, XmlObjSet& containers)
{
	// collect baseobj, base of baseobj, ...
	for( XmlObjSetIter it=objects.begin(); it!=objects.end(); ++it )
		getBasesOfObj( *it, containers);
}

void CCoreXmlFile::getDeriveds( XmlObject * obj, XmlObjSet& containers)
{
	if( obj!= NULL)
	{
		for(AttribMapIter it=obj->m_attributes.begin(); it!=obj->m_attributes.end(); ++it)
		{ 
			attrid_type at = it->first;
			valtype_type vt = it->second->getType();

			if( at == ATTRID_DERIVED + ATTRID_COLLECTION && it->second->getType() == VALTYPE_COLLECTION) // all deriveds
			{
				XmlAttrCollection * coll = (XmlAttrCollection*)it->second;
				for( XmlObjSetIter it2=coll->m_children.begin(); it2!=coll->m_children.end(); ++it2 )
				{
					XmlObject * obj2 = (*it2);
					if( obj2 != NULL && obj2->m_metaid == DTID_MODEL) // folders cannot be derived
					{
						if( obj2->m_metaid == DTID_MODEL && containers.find( obj2) == containers.end())
							containers.insert( obj2);
						getDeriveds( obj2, containers); // find deriveds from deriveds also
					}
				}
			}
		}
	}
}

void CCoreXmlFile::getAllUpAndDown( XmlObjSet& objects, XmlObjSet& containers)
{
	// first go up
	// let's not go up
	// we don't go up anymore
	for( XmlObjSetIter it=objects.begin(); 0 && it!=objects.end(); ++it )
	{
		XmlObject * iobj = *it;
		XmlObject * cont = 0;
		AttribMapIter ait;
		while( iobj)
		{
			ait = iobj->m_attributes.find( ATTRID_PARENT );
			ASSERT( ait != iobj->m_attributes.end() );
			cont = ((XmlAttrPointer*)(ait->second))->m_parent;

			if( cont && cont->m_metaid == DTID_MODEL)
			{
				if( cont && containers.find( cont) == containers.end()) // not inserted yet
					containers.insert( cont);
				iobj = cont;
			}
			else // 0 or DTID_FOLDER
				iobj = 0;
		}
	}

	// then go down
	for( XmlObjSetIter it=objects.begin(); it!=objects.end(); ++it )
		getAllTheWayDown( *it, containers);
}

void CCoreXmlFile::getAllTheWayDown( XmlObject * obj, XmlObjSet& containers)
{
	if( obj!= NULL)
	{
		if( obj->m_metaid == DTID_MODEL && containers.find( obj) == containers.end())
			containers.insert( obj);
		for(AttribMapIter it=obj->m_attributes.begin(); it!=obj->m_attributes.end(); ++it)
		{ 
			attrid_type at = it->first;
			valtype_type vt = it->second->getType();

			if( at == ATTRID_PARENT + ATTRID_COLLECTION && it->second->getType() == VALTYPE_COLLECTION)// children
			{
				XmlAttrCollection * coll = (XmlAttrCollection*)it->second;
				for( XmlObjSetIter it2=coll->m_children.begin(); it2!=coll->m_children.end(); ++it2 )
				{
					XmlObject * obj2 = (*it2);
					if( obj2 != NULL && ( obj2->m_metaid == DTID_MODEL || obj2->m_metaid == DTID_FOLDER))
						getAllTheWayDown( obj2, containers);
				}
			}
		}
	}
}

void CCoreXmlFile::getCheckOutContainers(XmlObjSet& objects, XmlObjSet& containers, bool thorough)
{
	XmlObjSet processedObjects;
	for( XmlObjSetIter it=objects.begin(); it!=objects.end(); ++it )
		getDependentContainers( *it, containers, processedObjects, thorough );
}

void CCoreXmlFile::getDependentContainers(XmlObject * obj, XmlObjSet& containers, XmlObjSet& processedObjects, bool thorough)
{
	if( obj!= NULL && processedObjects.find(obj)==processedObjects.end() )  // not processed yet
	{
		processedObjects.insert(obj);
		XmlObject * container = getContainer(obj);
		if( container != NULL )
		{
			// TODO: to be though over thoroughly. E.g. if there is deletion
			// do the attributes change in such an order, that not all containers get into
			// the list (as the relationships cease to exist we lose the chance to find 
			// the container)
			// Do we need to store the container??? Think about it!
			containers.insert( container );
			for(AttribMapIter it=container->m_attributes.begin(); (m_userOpts.m_alwaysFullLock || m_fullLockNeeded || thorough) && it!=container->m_attributes.end(); ++it)
			{ 
				attrid_type at = it->first;
				valtype_type vt = it->second->getType();

				// this optimizes it:
				if(      at == ATTRID_CONSTROWNER + ATTRID_COLLECTION) { } // no need for all of these
				else if( at == ATTRID_REGNOWNER   + ATTRID_COLLECTION) { }
				else if( at == ATTRID_ATTRPARENT  + ATTRID_COLLECTION) { }
				else if( it->second->getType() == VALTYPE_COLLECTION) // go through the collections 
				{
					XmlAttrCollection * coll = (XmlAttrCollection*)it->second;
					for( XmlObjSetIter it2=coll->m_children.begin(); it2!=coll->m_children.end(); ++it2 )
					{
						XmlObject * obj2 = (*it2);
						if( obj2 != NULL )
							getDependentContainers( obj2, containers, processedObjects );
					}
				}
			}
		}
	}
}

/* 
Ask user if files need to be checked out.

Returns true if all the files has been checked out successfully.
*/
bool CCoreXmlFile::checkOutFiles(XmlObjSet& containers)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	if( containers.size() == 0 )
		return true;

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
	char buff[329];
	sprintf( buff, " ..--== size[ModifiedObjects] = %i, size[checkOutContainers] = %i ==--..", m_modifiedObjects.size(), containers.size());
	sendMsg( buff, MSG_INFO);
#endif
#endif

	XmlObjSetIter it;
	XmlObjSet     containersUsedByOthers;

	// count the files need to be checked out
	XmlObjSet readOnlyFiles;
	bool      checkdOutByOthers = false;
	for( it=containers.begin(); it!=containers.end(); ++it )
	{
		XmlObject * obj = *it;
		ASSERT( obj != NULL );

		if( m_sourceControl == SC_CLEARCASE )
		{
			string fileName;
			getContainerFileName( obj, fileName, true );

			FILE * f = fopen( fileName.c_str(), "r" );
			if( f!=NULL )
			{
				fclose(f);
				int checkoutState = getCheckOutStateCC( fileName.c_str() );

				if( checkoutState == CS_OTHER_USER )
				{
					checkdOutByOthers = true;
					readOnlyFiles.insert( obj );
					containersUsedByOthers.insert( obj );
				}
				else if( checkoutState == SC_NONE )
					readOnlyFiles.insert( obj );
			}
		}
		else if( isContainerReadOnly(obj) )
		{
			readOnlyFiles.insert( obj );
			if( m_sourceControl != SC_NONE )
			{
				// check if checked out by other users
				if( isContinerCheckedOut( obj ) )
				{
					containersUsedByOthers.insert( obj );
					checkdOutByOthers = true;
				}
			}
		}
	}

	if( readOnlyFiles.size() == 0 )
		return true;

	// if there is no source control nothing we can do, notify the user and roll back transaction
	if( m_sourceControl == SC_NONE )
	{
		AfxMessageBox( "Cannot perform this operation because some read only files need modification. Maybe the project is under source control. Try to open it again and login to the source control database!" );
		return false;
	}

	// if there are files checkd out by other we are done, roll back transaction
	if( checkdOutByOthers )
	{
		CFilesInUseDlg dlg; // plain dlg
		if( dlg.DoModal() == IDOK )
			showUsedFiles( containersUsedByOthers );
		return false;
	}

	// ask user for check out confirmation
	if( m_userOpts.m_defCheckOutOnAction || AfxMessageBox( "To perform this operation some files will be checked out. Do you want to continue?", MB_YESNO ) == IDYES )
	{
		if( m_userOpts.m_defCheckOutOnAction)
			sendMsg( std::string( "ACCELERATION: Files checked out automatically based on policy configured."), MSG_INFO);

		// don't create a mess by checking out modifications done by others
		// commented by zolmol
		//getLatestVersion();

		XmlObjSet latent_files;
		bool needClose = filesModifiedByOthersV3( readOnlyFiles, latent_files);
		if( needClose)
		{
			//sendMsg( "This part of the project has been modified by other users, it is highly recomended to close and reopen your project to synchronize it!", MSG_ERROR );
			//sendMsg( "Precise conflict, big chance of overlapping modifications. Not allowed!", MSG_ERROR );
			sendMsg( "Update with save operation by other users changed this part of the project. Your changes are not allowed unless you close and reopen the project!", MSG_ERROR );

			CFilesInUseDlg dlg( 0, true); // dlg with latent msg
			if( dlg.DoModal() == IDOK )
				showUsedFiles( latent_files, true );
			return false;
		}


		// check out files
		try
		{
			for( it=readOnlyFiles.begin(); it!=readOnlyFiles.end(); ++it )
				checkOutContainer( *it );

		}
		catch(...)
		{
			// couldn't checkout all things, what's next?
			// undocheckout or checkoutrollback needed for those which succesfully were checkedout
			// roll back transaction, and notify user about what happened
			sendMsg( "Could not check out all files needed to complete operation.", MSG_ERROR);
			sendMsg( "Rollback follows.", MSG_INFO);

			for( it=readOnlyFiles.begin(); it!=readOnlyFiles.end(); ++it )
				rollBackTheCheckOutContainer( *it ); // it has a try catch block inside, thus it is safe

			sendMsg( "Rollback finished. See details above.", MSG_INFO);
			return false;
		}


		return true;
	}
	else
		return false;
}

XmlObject * CCoreXmlFile::objectFromObjId(metaobjidpair_type idpair)
{
	if( idpair.metaid == METAID_NONE && idpair.objid == OBJID_NONE )
		return NULL;

	if( idpair.metaid == METAID_ROOT )
		return m_root;
	else
		return (XmlObject*)idpair.objid;
}

void CCoreXmlFile::objIdFromObject(XmlObject * obj, metaobjidpair_type& idpair)
{
	if( obj == NULL )
	{
		idpair.metaid = METAID_NONE;
		idpair.objid  = OBJID_NONE;
	}
	else
	{
		idpair.metaid = obj->m_metaid;
		if( idpair.metaid == METAID_ROOT )
			idpair.objid = 1;
		else
			idpair.objid = (long)obj; // pointer to long conversion, is a truncation on 64bit systems
	}
}

void CCoreXmlFile::timestampOfCache( FILETIME* p_fTime)
{
	WIN32_FILE_ATTRIBUTE_DATA attr;
	if( GetFileAttributesEx( m_cacheFileName.c_str(), GetFileExInfoStandard, &attr ) )
	{
		*p_fTime = attr.ftLastWriteTime;
	}
}

void CCoreXmlFile::writeBinaryCache()
{
	FILE * f = fopen( m_cacheFileName.c_str(), "wb" );
	if( f==NULL )
	{
		sendMsg( "Exception: Could not create binary cache file!", MSG_ERROR);
		HR_THROW(E_FILEOPEN);
	}

	XmlObjVecIter i;
	metaid_type   mid;

	// write out GUIDs and metaids
	int n = m_objects.size();
	fwrite( &n, sizeof(n), 1, f );
	for( i = m_objects.begin(); i != m_objects.end(); ++i )
	{
		mid = (*i)->m_metaid;
		fwrite( &mid, sizeof(mid), 1, f );
		fwrite( &((*i)->m_guid), sizeof(GUID), 1, f );
	}

	// write out pointers
	int x;
	for( i=m_objects.begin(); i!=m_objects.end(); ++i )
	{   
		XmlObject * obj = (*i);
		for( AttribMapIter j=obj->m_attributes.begin(); j!=obj->m_attributes.end(); ++j)
		{
			if( j->second->getType() == VALTYPE_POINTER )
			{
				XmlAttrPointer * pointer = (XmlAttrPointer*)j->second;
				if( pointer->m_parent == NULL )
					x = -1;
				else
					x = pointer->m_parent->m_index;
				fwrite( &x, sizeof(x), 1, f );
			}
		}
	}

	fclose(f);
}

bool CCoreXmlFile::readBinaryCache()
{    
	FILE * f = fopen( m_cacheFileName.c_str(), "rb" );
	if( f==NULL )
		return false;

	clearAll();

	// read GUIDs and metaids and create objects
	int n,i;
	fread( &n, sizeof(n), 1, f );
	for( i=0; i<n; ++i )
	{
		metaid_type  metaid;
		GUID         guid;

		fread( &metaid, sizeof(metaid), 1, f );
		fread( &guid, sizeof(GUID), 1, f );

		CComObjPtr<ICoreMetaObject> metaobject;
		COMTHROW( m_metaProject->get_Object(metaid, PutOut(metaobject)) );

		XmlObject * obj = new XmlObject(metaobject,false);
		obj->m_guid   = guid;
		addObject(obj);

		if( metaid == METAID_ROOT )
			m_root = obj;
	}

	UnresolvedPointerVec pointers;
	int x;
	for( XmlObjVecIter it=m_objects.begin(); it!=m_objects.end(); ++it )
	{   
		XmlObject * obj = (*it);
		for( AttribMapIter j=obj->m_attributes.begin(); j!=obj->m_attributes.end(); ++j)
		{
			if( j->second->getType() == VALTYPE_POINTER )
			{
				XmlAttrPointer * pointer = (XmlAttrPointer*)j->second;

				UnresolvedPointer p;
				p.m_object = obj;
				p.m_attrib = j->first;

				fread( &x, sizeof(x), 1, f );
				if( x == -1 )
					p.m_pointedObjGuid = GUID_NULL;
				else
					p.m_pointedObjGuid = m_objects[x]->m_guid;

				pointers.push_back( p );
			}
		}
	}

	resolvePointers( pointers );

	// read and set pointers
	/*int x;
	for( XmlObjVecIter it=m_objects.begin(); it!=m_objects.end(); ++it )
	{   
	XmlObject * obj = (*it);
	for( AttribMapIter j=obj->m_attributes.begin(); j!=obj->m_attributes.end(); ++j)
	{
	if( j->second->getType() == VALTYPE_POINTER )
	{
	XmlAttrPointer * pointer = (XmlAttrPointer*)j->second;

	fread( &x, sizeof(x), 1, f );
	if( x == 0 )
	pointer->m_parent = NULL;
	else
	pointer->m_parent = m_objects[x];
	}
	}
	}*/



	fclose(f);

	//updateCollections();

	return true;
}

void CCoreXmlFile::createProjectFile()
{    
	// create projet file
	FILE * f = fopen( m_projectFileName.c_str(), "wt" );
	if( f == NULL )
	{
		sendMsg( "Exception: Could not create project file '" + m_projectFileName + "'!", MSG_ERROR);
		AfxMessageBox( (std::string( "Could not create file ") + m_projectFileName).c_str());
		HR_THROW(E_FILEOPEN);
	}
	fprintf( f, "<GME " );
	if( m_vssDatabaseStr.size() > 0 )
		fprintf( f, "VSSDatabase=\"%s\" VSSPath=\"%s\"", m_vssDatabaseStr.c_str(), m_vssPath.c_str() );
	if( m_svnUrl.size() > 0)
		fprintf( f, "svn=\"%s\" access=\"%s\"", svnSshMangling( m_svnUrl).c_str(), m_svnByAPI?"API":"CMD" );
	if( m_hashFileNames)
		fprintf( f, " hash=\"true\" hval=\"%s\"", (m_hashVal == 5)?"1+2":(m_hashVal == 2)?"2": (m_hashVal == 3)? "3":"4");
	else
		fprintf( f, " hash=\"false\"" );
	fprintf( f, ">\n" );
	fprintf( f, "</GME>\n" );
	fclose( f );

	// if project under source control, the project file should be added to source control    
	if( isSS())
	{
		// get project item
		CComObjPtr<IVSSItem> projectItem;
		CComBSTR path = m_vssPath.c_str();
		COMTHROW( m_vssDatabase->get_VSSItem( path, false, &(projectItem.p)) );

		// add to source control projecct
		CComObjPtr<IVSSItem> item;
		CComBSTR fileName2 = m_projectFileName.c_str();

		projectItem->Add( fileName2, NULL, 0, &(item.p));   // TODO error handling
	}
	else if( isCC())
	{
		try
		{
			// add project file
			addFileToCC( m_projectFileName.c_str() );
			//checkInFileCC( m_projectFileName.c_str() );
			//checkInFileCC( m_folderPath.c_str() );
			//checkInFileCC( m_parentFolderPath.c_str() );
		}
		catch(...)
		{
			AfxMessageBox("ClearCase error! Cannot add project to ClearCase. Errocode=2");
			m_sourceControl = SC_NONE;
		}
	}
	else if( isSV())
	{
		bool s1 = addSVN( m_projectFileName);
		bool s2 = commitSVN( m_projectFileName, true);

		if( !s1 || !s2)
		{
			AfxMessageBox( "Subversion error! Cannot add project to Subversion. Errocode = 2!");
		}
	}

	makeSureFileExistsInVerSys( OperatingOptions::m_sysConfName, isSV() ? OperatingOptions::m_sysConfDefContentsSvn : OperatingOptions::m_sysConfDefContentsPlain);
}

void CCoreXmlFile::readProjectFile()
{
	// Project file is an xml file, with a GME tag.
	// possible attributes: VSSDatabase, VSSPath
	// example: <GME VSSDatabase="\\bogyom\GMEXMLBackEndTest\sorucesafedb\srcsafe.ini" VSSPath="$\test1"></GME>


	DOMBuilder * parser = getFreshParser( "ProjectFileReader");

	ASSERT( parser != NULL );
	if( !parser)
	{
		sendMsg( "Exception: Could not create parser!", MSG_ERROR);
		HR_THROW(E_FILEOPEN);
	}

	DOMErrorHandler* err_handler = new DOMErrorPrinter( &m_console);
	parser->setErrorHandler( err_handler);

	bool fexists   = FileHelp::fileExist( m_projectFileName);
	bool suc       = false;
	XERCES_CPP_NAMESPACE::DOMDocument * doc = 0;
	if( fexists) doc = enclosedParse( m_projectFileName, parser, &suc);
	if( !doc || !suc)
	{
		//sendMsg( "Could not find or parse project file " + m_projectFileName, MSG_ERROR);
		setFileNames( true); // reset the file name to a newly found one
		doc = enclosedParse( m_projectFileName, parser, &suc);
		if( !doc || !suc)
		{
			if( fexists) sendMsg( "Could not parse project file '" + m_projectFileName + "'!", MSG_ERROR);
			else         sendMsg( "Could not find project file '" + m_projectFileName + "' in directory " + m_folderPath, MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}
	}
	DOMElement * e = doc->getDocumentElement();
	if( e == NULL)
	{
		sendMsg( "Null document element error during parsing of '" + m_projectFileName + "'!", MSG_ERROR);
		HR_THROW(E_FILEOPEN);
	}

	XMLCh* x_vssDatabase = XMLString::transcode("VSSDatabase");
	XMLCh* x_svnLocator  = XMLString::transcode("svn");
	XMLCh* x_svnAccess   = XMLString::transcode("access");
	XMLCh* x_hashedDirs  = XMLString::transcode("hash");
	XMLCh* x_hashValue   = XMLString::transcode("hval");

	char * vssDatabase = XMLString::transcode(e->getAttribute( x_vssDatabase));
	char * svnLocator  = XMLString::transcode(e->getAttribute( x_svnLocator));
	char * svnAccess   = XMLString::transcode(e->getAttribute( x_svnAccess));
	char * hashedDirs  = XMLString::transcode(e->getAttribute( x_hashedDirs));
	char * hashValue   = XMLString::transcode(e->getAttribute( x_hashValue));

	if( !strcmp( hashedDirs, "true")) 
	{
		m_hashFileNames = true;
		m_hashInfoFound = true; // this will skip asking questions about hashing

		if     ( !strcmp( hashValue, "4096"))m_hashVal = 5;
		else if( !strcmp( hashValue, "256")) m_hashVal = 2;
		else if( !strcmp( hashValue, "1+2")) m_hashVal = 5;
		else if( !strcmp( hashValue, "2"))   m_hashVal = 2;
		else if( !strcmp( hashValue, "3"))   m_hashVal = 3;
		else if( !strcmp( hashValue, "4"))   m_hashVal = 4;
		else m_hashInfoFound = false;     // unkown value
	}
	else if( !strcmp( hashedDirs, "false"))
	{
		m_hashFileNames = false;
		m_hashInfoFound = true; // this will skip asking questions about hashing
	}

	if( strlen(vssDatabase) != 0 )
	{
		m_vssDatabaseStr = vssDatabase;
		XMLCh* x_vssPath     = XMLString::transcode("VSSPath");
		char * vssPath   = XMLString::transcode(e->getAttribute( x_vssPath));

		m_vssPath = vssPath;

		XMLString::release( &vssPath);//delete vssPath;
		XMLString::release( &x_vssPath);

		openSourceSafeDatabase();
	}
	else if( strlen( svnLocator) != 0 )
	{
		m_svnUrl = svnLocator;
		if( m_svnUrl != m_userOpts.m_prefUrl && !m_userOpts.m_prefUrl.empty()) // pref not empty and !equal
		{
			sendMsg( "Preferred url substitutes that loaded from the project file!", MSG_INFO);
			m_svnUrl = m_userOpts.m_prefUrl;
		}

		if( svnAccess != 0 && svnAccess != "")
		{
			m_svnAccessMethodFound = true;
			m_svnByAPI = true;  // default: API
			if( !strcmp( svnAccess, "CMD"))
				m_svnByAPI = false;
		}

		m_vssUser = m_userOpts.m_useAccountInfo? m_userOpts.m_defUserName.c_str(): userNameFromSvnSshUrl();
		m_vssPassword = m_userOpts.m_useAccountInfo? m_userOpts.m_defPassword.c_str(): "";

		m_sourceControl = SC_SUBVERSION;

		svnSetup( false); //false => won't throw if cancelled
		// fills m_vssUser, m_vssPassword

		// upon Open, the connection string might not have the full URL in it, so 
		// that information is only available after readProjectFile (m_svnUrl)

		_chdir( m_folderPath.c_str()); // change to the newly created local dir
	}
	else
	{        
		m_sourceControl = SC_NONE;
		m_vssDatabaseStr = "";
		m_vssPath        = "";

		// check if the project is under clearcase
		if( createClearCaseApp() )
		{
			if( isPathUnderClearCase( m_parentFolderPath.c_str() ) )
				m_sourceControl = SC_CLEARCASE;
		}
	}

	XMLString::release( &vssDatabase);//delete vssDatabase;
	XMLString::release( &x_vssDatabase);
	XMLString::release( &x_svnLocator);
	XMLString::release( &x_svnAccess);
	XMLString::release( &svnLocator);
	XMLString::release( &svnAccess);
	XMLString::release( &x_hashedDirs);
	XMLString::release( &x_hashValue);
	XMLString::release( &hashedDirs);
	XMLString::release( &hashValue);

	delete err_handler;
	delete parser;
}

void CCoreXmlFile::writeAll()
{
	chdir( m_folderPath.c_str()); // change to the local main dir (a file dialog may have changed the local dir)

	for( XmlObjVecIter it=m_objects.begin(); it!=m_objects.end(); ++it )
	{
		XmlObject * obj = (*it);
		if( obj->isContainer() && obj->m_loaded )
			writeXMLFile( obj );

		// we would need to write only those which have been modified, right?
		// yes! inside that method, files are open with 'w', which succeeds
		// only if the file is checked out, so non-modified containers are not
		// written out. Although not all checked out files are modified.
	}
}

void CCoreXmlFile::timeSync( const char * fileName, XmlObject * container)
{
	// get last write time of the recently updated and _closed_ (!) file
	WIN32_FILE_ATTRIBUTE_DATA attr;        
	if( GetFileAttributesEx( fileName, GetFileExInfoStandard, &attr ))
	{
		applyLastWrTime( container, true, CTime( attr.ftLastWriteTime ));
	}
}

void CCoreXmlFile::writeXMLFile(XmlObject * container)
{
	if( !container->isContainer() )
		HR_THROW(E_INVALID_USAGE);

	std::string fileName;
	getContainerFileName(container, fileName);

	// we wish we could predict the time the file will be closed
	// because that will become the file's 'Modified At' attribute
	// the obj->m_lastWriteTime needs to reflect exactly this time
	time_t        currentTime1 = time( &currentTime1 );
	CTime         currentTime2( currentTime1 + 1 );

	bool f_existed = false;
	if( FileHelp::isFileReadOnly2( fileName, &f_existed))
	{
		return; // file exists, is read-only, no chance of writing into it
		        // it also means there was no change
	}

	// open file
	// previously the "w" mode used by fopen guarranteed that only read-write files are opened
	// now with Transcoder, exceptions will be thrown whenever a file can't be opened because it is read-only.
	// That's why it is a good thing to check (above) for read-onlyness
	Transcoder ofs;
	try
	{
		bool deleted = false;
		// Is this object deleted according to its current state?
		// That's important to understand: if user does undo on a deleted object
		// its state might return to normal, thus the deleted object might become
		// undeleted at any time until the user closes the project.
		// During projectclose the deleted state objects -at that time- are really
		// discarded, see ::DeleteObject code for handling these.
		AttribMapIter parit = container->m_attributes.find( ATTRID_PARENT);
		if( parit != container->m_attributes.end() && container->m_metaid != DTID_ROOT)
		{
			// if a model has its parent 0, then it is deleted:
			deleted = 0 == ((XmlAttrPointer*)(parit->second))->m_parent;
		}

		if( !deleted)
		{
			ofs.init( fileName.c_str(), "UTF-8"); // create a transcoder fstream
			// Transcoder dumps by default the xml version and encoding

			// write objects recursively
			writeObject( container, ofs, true, "", currentTime2 );
			ofs.finalize();
		}
		// deleted containers will have a file of 0 size
		if( deleted) // really overwrite the file
		{
			std::ofstream zero; 
			zero.open( fileName.c_str(), std::ios_base::out | std::ios_base::trunc); // 'w'
			zero.close();
		}

		// now it's time to get file's exact 'Modified At' attribute
		// which can be set into the objects as the real m_lastWriteTime
		// alternative: if we could set the file's modification attribute
		// to a certain time, we'd save the effort-time to go over the
		// hierarchy (but the good news is that only the container's
		// m_lastWriteTime needs to be updated, subobjects don't matter)
		timeSync( fileName.c_str(), container);

		// add to source control if not added yet
		try
		{
			addToSourceControl( container, f_existed );
		}
		catch(...)
		{
			sendMsg( "Failed while adding to version control file: " + fileName, MSG_WARNING);
		}
	}
	catch( hresult_exception& e)
	{
		if( e.hr == E_INVALID_FILENAME) // file could not be opened, thrown by Transcoder::init()
			sendMsg( "Error while saving. Could not open file " + fileName, MSG_ERROR);
		else
			sendMsg( "Unknown error while saving file " + fileName, MSG_ERROR);
	}
	catch( ...)
	{
		sendMsg( "Generic error while saving file " + fileName, MSG_ERROR);
	}
}

void CCoreXmlFile::applyLastWrTime(XmlObject * obj, bool container, CTime lastWriteTime )
{
	obj->m_lastWriteTime = lastWriteTime;

	// apply the proper time for children too? currently only the container 
	// file times are compared (Versioning System's last checked in version vs.
	// the local file OR local file vs. the XmlObject's m_lastWriteTime)
	// so kids with no file associated need not have so precise time
}

void CCoreXmlFile::writeObject(XmlObject * obj, Transcoder& ofs, bool container, const char * prefix, CTime lastWriteTime )
{        
	std::string                     str;
	CComObjPtr<ICoreMetaObject>     metaobject;
	CComBSTR                        metaToken;

	obj->m_lastWriteTime = lastWriteTime;

	COMTHROW( m_metaProject->get_Object( obj->m_metaid, PutOut(metaobject) ) );
	COMTHROW( metaobject->get_Token( &metaToken ) );
	guid2str( obj->m_guid, str );

	bool spec_root = metaToken == "Root"; // or: m_metaid==METAID_ROOT

	// the multiline case? for details see Mga\MgaGeneric.cpp
	// and http://escher.isis.vanderbilt.edu/JIRA/browse/GME-151 
	// those VALTYPE_STRING type of attributes which are shown in the GME 
	// environment as multiline strings are affected: StrValue and RegNodeValue
	// the objects which own these kinds of attibutes are: StrAttr and RegNode
	bool spec_care = metaToken == "StrAttr" || metaToken == "RegNode" || spec_root;
	std::string spec_value;
	std::string spec_preamble( spec_root?"Comment=":"");
	// there might be multiple attrs of <Root> which will be multiline (in the future)
	// thus, we with regard to that we need to distinguish the CDATA sections somehow.
	// only when Root is the owner, we will use a preable in the CDATA section for Comment

	ofs << Transcoder::NoEscape << prefix << "<" << metaToken << " MetaId=\"" << obj->m_metaid << "\" Id=\"" << str << "\"";
	if( obj->m_deleted )
		ofs << " deleted=\"true\"";

	// write pointers, attributes
	AttribMapIter it;
	for( it=obj->m_attributes.begin(); it!=obj->m_attributes.end(); ++it )
	{
		XmlAttrBase                    * attr = it->second;
		CComObjPtr<ICoreMetaAttribute>   metaAttrib;
		CComBSTR                         attribToken;
		std::string                           attrVal;

		COMTHROW( metaobject->get_Attribute( it->first, PutOut(metaAttrib) ) );
		metaAttrib->get_Token( &attribToken );
		if( attribToken == "fstate")
		{   // do not dump file state attr into files
			continue;
		}

		if( attr->getType() == VALTYPE_POINTER )
		{
			ParentMap::iterator it2 = m_parentMap.find( obj->m_metaid );
			if( it2==m_parentMap.end() || it2->second!=it->first || container )
			{
				XmlAttrPointer * pointer = (XmlAttrPointer*)attr;
				if( pointer->m_parent != NULL )
					guid2str( pointer->m_parent->m_guid, attrVal );
			}
		}
		else if( attr->getType() != VALTYPE_COLLECTION && attr->getType() != VALTYPE_LOCK )
		{
			XmlAttrBase * attr = it->second;
			attr->toString(attrVal);
		}

		if( !attrVal.empty() )
		{
			// attribToken not converted any more with replaceSpaceWithUnderscore -- zolmol
#ifdef DEBUG
			std::string at; CopyTo( attribToken, at);
			ATLASSERT(("Check MgaGeneric.cpp for attribute tokens containing spaces", at.find(' ') == -1));
#endif

			bool spec_attr = spec_care && (attribToken == "StrValue" || attribToken == "RegNodeValue" || attribToken == "Comment");

			if( spec_attr) // spec_care is also true
				spec_value = attrVal; // store the (possibly multiline) original value, without encoding
			else // regular dump:
				ofs << " " << attribToken << "=\"" << Transcoder::StdEscape << attrVal << Transcoder::NoEscape << "\"";
		}
	}
	ofs << Transcoder::NoEscape << ">";
	if( spec_care)                                                    // right after the element, without whitespace, (in sync with readObject())
	{
		// only a little attention is paid to possbile
		// occurences of ']]>' in the data
		if( spec_value.find( "]]>") != std::string::npos)
		{
			sendMsg( "Special character string ']]>' found among one element's properties, will be replaced by ']] >'", MSG_INFO);
			spec_value.replace( spec_value.find( "]]>"), 3, "]] >");
		}
		// encoding not needed since it goes to CDATA
		ofs << "<![CDATA[" << spec_preamble << spec_value << "]]>";
	}
	ofs << "\n";


	// write out children
	// child is written if it is not the root, a model or a folder
	// and the parent of the child is a us according to m_parentMap
	std::string newPrefix = prefix;
	newPrefix += "\t";
	for( it=obj->m_attributes.begin(); it!=obj->m_attributes.end(); ++it )
	{
		XmlAttrBase * attr = it->second;
		if( attr->getType() == VALTYPE_COLLECTION )
		{
			XmlAttrCollection * coll = (XmlAttrCollection*)attr;
			for( XmlObjSetIter it2=coll->m_children.begin(); it2!=coll->m_children.end(); ++it2 )
			{
				XmlObject * obj2 = (*it2);
				if( obj2!=NULL && !obj2->isContainer() )
				{                    
					ParentMap::iterator it3 = m_parentMap.find( obj2->m_metaid );
					ASSERT( it3 != m_parentMap.end() );
					if( it3->second + ATTRID_COLLECTION == it->first )
						writeObject( obj2, ofs, false, newPrefix.c_str(), lastWriteTime );
				}
			}
		}
	}

	ofs << Transcoder::NoEscape << prefix << "</" << metaToken << ">\n";
}

/*
Moves to continer and all contained object to disk if possible (all lock values are 0)
It deletes secondary attributes from memory.

Returns true if the objects have been moved to disk successfully
*/
bool CCoreXmlFile::moveContainerToDisk(XmlObject * container)
{
	XmlObjVec     objs;
	XmlObjVecIter it;

	getContainedObjects( container, objs );
	for( it=objs.begin(); it!=objs.end(); ++it )
	{
		XmlObject * obj = *it;

		AttribMapIter it2 = obj->m_attributes.find(ATTRID_LOCK);
		ASSERT( it2 != obj->m_attributes.end() );

		XmlAttrLock * lock = (XmlAttrLock*)(it2->second);
		if( lock != 0 )
			return false;
	}

	// write all objects to disk
	writeXMLFile( container );

	// unload secondary attributes from memory
	for( it=objs.begin(); it!=objs.end(); ++it )
	{
		XmlObject * obj = *it;
		obj->deleteSecondaryAttribs();
	}

	return false;
}

void CCoreXmlFile::fullReadContainer(XmlObject * container)
{
	std::string fileName;
	getContainerFileName(container, fileName);

	UnresolvedPointerVec pointers;
	readXMLFile( fileName.c_str(), pointers, true );
	// shouldn't we call this? probably not, since pointer skeletons have been loaded already
	// and all pointers are considered unresolved after readXMLFile()
	//resolvePointers( pointers);
	//ASSERT( pointers.size() == 0);
}

void CCoreXmlFile::readXMLFile( const char * fileName, UnresolvedPointerVec& pointers, bool fullLoad )
{
	// get last write time
	WIN32_FILE_ATTRIBUTE_DATA attr;        
	BOOL res = GetFileAttributesEx( fileName, GetFileExInfoStandard, &attr );

	// attr is valid and filesize = 0
	if( res && attr.nFileSizeHigh == 0 && attr.nFileSizeLow == 0)
		return;

	CTime lastWriteTime( attr.ftLastWriteTime );

	DOMBuilder * parser = NULL;
	try
	{
		DOMImplementationLS * domimpl = 0;
		DOMErrorHandler*      err_handler = 0;
		giveDOMObjs( &domimpl, &parser, &err_handler);

		if( !domimpl || !parser)
		{
			sendMsg( std::string( "Could not create parser for file ") + fileName + "!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		bool suc;
		XERCES_CPP_NAMESPACE::DOMDocument * doc = enclosedParse( fileName, parser, &suc);
		if( !doc || !suc)
		{
			sendMsg( std::string( "Could not parse file ") + fileName + "!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		DOMElement * doc_e = doc->getDocumentElement();
		if( doc_e == NULL)
		{
			// todo linenumbers
			// this might be totally useless
			bool might_be_ok = false;
			DOMNodeList* list = doc->getChildNodes();

			if( list) for( int i = (int) list->getLength() - 1; i >= 0 ; --i)
			{
				might_be_ok = true;
				DOMNode * node = list->item(i);
				if( node->getNodeType() == DOMNode::TEXT_NODE)
				{
					DOMText * txt  = (DOMText*) node;
					const XMLCh* p = txt->getData();
				}
				if( node->getNodeType() == DOMNode::ELEMENT_NODE)
				{
					DOMElement* elem = (DOMElement*)node;
					const XMLCh* tn = elem->getTagName();
				}
			}
			if( !might_be_ok)
				sendMsg( std::string( "Null document element was found while parsing file ") + fileName + "!", MSG_ERROR);

			// reload it, might be nonzero now (only in fairy tales probably)
			doc_e = doc->getDocumentElement();
		}

		if( doc_e == NULL)
		{
			sendMsg( std::string( "Exception: Null document element during parsing of ") + fileName + "!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		readObject( doc_e, pointers, NULL, fullLoad, lastWriteTime );

		deleteParser( &parser);//delete parser;
	}
	catch(...)
	{
		if( parser != NULL )
			deleteParser( &parser);//delete parser;

		sendMsg( std::string( "Exception during reading ") + fileName + " file!", MSG_ERROR);
		HR_THROW(E_FILEOPEN);
	}
}

void CCoreXmlFile::readObject(DOMElement * e, UnresolvedPointerVec& pointers, XmlObject * parent, bool fullLoad, CTime lastWriteTime )
{   
	if( e == NULL)
	{
		sendMsg( "Exception: readObject invoked with null element!", MSG_ERROR);
		HR_THROW(E_FILEOPEN);
	}
	// what is the deleted container policy? if XML attribute is used code below is needed
#ifdef DEBUG
	// if the object is deleted do not deal with it
	// possible optimization: limit analysis to ( parent == 0) case only (when incoming parameter is 0)
	XMLCh* x_deleted = XMLString::transcode( ParserLiterals::Main::deleted);
	char * deletedStr = XMLString::transcode( e->getAttribute( x_deleted));
	bool deleted = (strcmp( deletedStr, "true" ) == 0);
	XMLString::release( &x_deleted);
	XMLString::release( &deletedStr);
	if( deleted)
	{
		ASSERT(0); // deleted XML attribute is no longer present in deleted XML files
		return;
	}
#endif

	XMLCh * x_metaId = XMLString::transcode( ParserLiterals::Main::metaId);
	XMLCh * x_id     = XMLString::transcode( ParserLiterals::Main::id);

	// get metaid, and id
	char * metaIdStr  = XMLString::transcode( e->getAttribute( x_metaId));
	char * objGUIDStr = XMLString::transcode( e->getAttribute( x_id));

	long metaid = atoi( metaIdStr);
	GUID   guid = str2guid( objGUIDStr );

	XMLString::release( &metaIdStr);
	XMLString::release( &objGUIDStr);
	XMLString::release( &x_metaId);
	XMLString::release( &x_id);


	// is container deleted/obsolete? 
	// if nonrootfolder and its explicit parent must be 0
	if( metaid != METAID_ROOT && parent == 0)
	{
		// is its XML Parent attribute empty
		XMLCh* x_parent       = XMLString::transcode( ParserLiterals::Main::parent);
		char* parent_xml_attr = XMLString::transcode( e->getAttribute( x_parent));

		// is object obsolete?
		bool obs = parent_xml_attr == 0 || strcmp( parent_xml_attr, "") == 0;

		XMLString::release( &x_parent);
		XMLString::release( &parent_xml_attr);

		if( obs) return;
	}

	// get meta object
	CComObjPtr<ICoreMetaObject> metaobject;
#pragma warning( disable: 4244) // conversion from 'long' to 'short', possible loss of data
	COMTHROW( m_metaProject->get_Object( metaid, PutOut(metaobject) ) );
#pragma warning( default: 4244) // conversion from 'long' to 'short', possible loss of data

	CComBSTR metaobj_token;
	COMTHROW( metaobject->get_Token( &metaobj_token));

	// the multiline case?  -----  for details see writeObject()
	bool          spec_care = metaobj_token == "StrAttr" || metaobj_token == "RegNode" || metaobj_token == "Root";
	std::string   spec_value;

	// find or create object
	XmlObject * obj = NULL;
	GUIDToXmlObjectMapIter it = m_objectsByGUID.find( guid );
	if( it != m_objectsByGUID.end() )
	{
		obj = it->second;
		if( !obj->m_loaded && fullLoad )
		{
			obj->createAttributes(metaobject,XmlObject::ATTR_SECONDARY);
			obj->m_loaded = true;
		}
	}
	else
	{
		obj = new XmlObject(metaobject,fullLoad);
		obj->m_guid = guid;
		addObject( obj );
		if( metaid == METAID_ROOT )
			m_root = obj;
	}

	obj->m_deleted = false;
	obj->m_lastWriteTime = lastWriteTime;

	// read attributes
	AttribMapIter it2;
	for( it2 = obj->m_attributes.begin(); it2 != obj->m_attributes.end(); ++it2 )
	{        
		CComObjPtr<ICoreMetaAttribute>  metaAttrib;
		CComBSTR                        attribToken;

		COMTHROW( metaobject->get_Attribute( it2->first, PutOut(metaAttrib) ) );
		COMTHROW( metaAttrib->get_Token( &attribToken ));        

		// see http://escher.isis.vanderbilt.edu/JIRA/browse/GME-152
		// it won't find 'MGA Version' of rootfolder if space2underscore and
		// underscore2space conversions are in effect
		char * attrVal = XMLString::transcode(e->getAttribute(attribToken));

		// multiline case?
		bool spec_value_found = false; // will ensure smooth upgrade from old style xmlbackend
		bool spec_attr = spec_care && ( attribToken == "StrValue" || attribToken == "RegNodeValue" || attribToken == "Comment");
		if( spec_attr)
		{
			// the implementation below with getChildNodes() is more tolerant of XML COMMENTs, XML Whitespaces...
			// than using the getFirstChild() method, which can be easily tricked by whitespaces
			DOMNodeList * children = e->getChildNodes();
			// find the first CDATA section among the kids: it should be the first node! (based on writeObject()'s impl)
			for( int i = 0; i < (int) children->getLength(); ++i )
			{
				DOMNode * node = children->item(i);
				if( node->getNodeType() == DOMNode::CDATA_SECTION_NODE ) // the first CDATA element is taken
				{
					char * sp_va = XMLString::transcode( ((DOMCDATASection*)node)->getTextContent());

					spec_value = sp_va;
					spec_value_found = true;

					XMLString::release( &sp_va);
					break;
				}
			}
		}

		// removed unnecessary conversion of attrVal with replaceUnderscoreWithSpace! -- zolmol

		XmlAttrBase * attr = it2->second;
		if( attr->getType() == VALTYPE_POINTER )
		{
			UnresolvedPointer p;
			p.m_object = obj;
			p.m_attrib = it2->first;          
			if( attrVal==NULL || strlen(attrVal)==0 )
				p.m_pointedObjGuid = GUID_NULL;
			else
				p.m_pointedObjGuid = str2guid( attrVal );
			pointers.push_back( p );
		}
		else if( attr->getType() != VALTYPE_LOCK && attr->getType() != VALTYPE_COLLECTION )
		{
			it2->second->fromString(attrVal);

			// use the spec_value only if really found the CDATA node
			if( spec_attr && spec_value_found) // spec_care is also true
			{
				if( attribToken == "Comment")
				{
					const std::string comm_spec = "Comment=";
					if( 0 == spec_value.find( comm_spec)) // CDATA section looks like this: <![CDATA[Comment=....]]>
						it2->second->fromString( spec_value.substr( comm_spec.length()).c_str());
					else
						ASSERT(0);
				}
				else
					it2->second->fromString( spec_value.c_str());
			}
		}

		XMLString::release( &attrVal);//delete [] attrVal;
	}

	// implicit parent pointer
	if( parent != NULL )
	{
		ParentMap::iterator it3 = m_parentMap.find( obj->m_metaid );
		UnresolvedPointer p;
		p.m_object = obj;
		p.m_attrib = it3->second;
		p.m_pointedObjGuid = parent->m_guid;
		pointers.push_back(p);
	}        

	// read children
	DOMNodeList * children = e->getChildNodes();
	for( int i=0; i< (int) children->getLength(); ++i )
	{
		DOMNode * node = children->item(i);
		//if spec_care was true the first child was CDATA, but we process anyway the ELEMENTs only
		if( node->getNodeType() == DOMNode::ELEMENT_NODE )
			readObject( (DOMElement*)node, pointers, obj, fullLoad, lastWriteTime );
	}
}

void CCoreXmlFile::loadFrom( const std::string& p_dir, UnresolvedPointerVec& p_pointers, bool p_fullLoad )
{
	char                 buf[_MAX_PATH];
	_finddata_t          fileInfo;

	sprintf( buf, "%s\\*.xml", p_dir.c_str() );

	long searchHandle = _findfirst( buf, &fileInfo );
	long ret = searchHandle;
	while( ret != -1 )
	{
		sprintf( buf, "%s\\%s", p_dir.c_str(), fileInfo.name );
		readXMLFile( buf, p_pointers, p_fullLoad );
		ret = _findnext( searchHandle, &fileInfo );
	}
	_findclose( searchHandle );
}

void CCoreXmlFile::loadDirs( const std::string& p_dir, UnresolvedPointerVec& p_pointers, bool p_fullLoad )
{
	char                 buf[_MAX_PATH];
	_finddata_t          item;

	loadFrom( p_dir, p_pointers, p_fullLoad); // load .xml files if any

	// find all subdirs
	sprintf( buf, "%s\\*", p_dir.c_str() );
	long searchHandle = _findfirst( buf, &item );
	long ret = searchHandle;
	while( ret != -1 )
	{
		if( (item.attrib & _A_SUBDIR) == _A_SUBDIR)
		{
			std::string f( item.name);
			if( f != ".." && f != ".")
			{
				sprintf( buf, "%s\\%s", p_dir.c_str(), item.name );
				//sendMsg( buf, MSG_INFO);
				//readXMLFile( buf, pointers, fullLoad );

				// invoke loadDirs for the subdir
				loadDirs( buf, p_pointers, p_fullLoad);
			}
		}
		ret = _findnext( searchHandle, &item );
	}
	_findclose( searchHandle );
}


void CCoreXmlFile::readAll( bool fullLoad )
{
	UnresolvedPointerVec pointers;

	// todo: preconditions
#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
	if( m_userOpts.m_measureTime) 
	{  
		sendMsg( std::string( "Loading ") + m_folderPath, MSG_INFO);
	}
#endif
#endif

	clearAll();

	m_strategyShared = m_userOpts.m_optimizedParsing;
	initParsers(); // shows a message and throws if can't create parser objects

	// load all dirs
	loadDirs( m_folderPath, pointers, fullLoad);

	finiParsers();

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
	if( m_userOpts.m_measureTime) { sendMsg( "resolvePointers", MSG_INFO); }
#endif
#endif

	resolvePointers( pointers );

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
	if( m_userOpts.m_measureTime) { sendMsg( "resetSourceControl", MSG_INFO); }

	_timeb b1, b2;
	_ftime( &b1);
#endif
#endif

	resetSourceControlForAll();

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
	_ftime( &b2);

	if( m_userOpts.m_measureTime) {
		char msg_buf[250];
		sprintf( msg_buf, "[createSourceControlInfoRegNodes took = %li secs %hi millisecs]", b2.time-b1.time, b2.millitm-b1.millitm);
		sendMsg( std::string( msg_buf), MSG_INFO);
	}
#endif
#endif

	if( m_userOpts.m_onLoadShowStatus)
	{
#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
		if( m_userOpts.m_measureTime) { sendMsg( "updateSourceControlRegnodes begn", MSG_INFO); }

		_timeb b3, b4; 

		_ftime( &b3);
#endif
#endif
		updateSourceControlInfo(); // show current status upon 'OpenProject'

#ifdef _DEBUG
#if(DETAILS_ABOUT_XMLBACKEND)
		_ftime( &b4);

		if( m_userOpts.m_measureTime) { sendMsg( "updateSourceControlRegnodes done", MSG_INFO); }

		if( m_userOpts.m_measureTime) { 
			char msg_buf[250];
			sprintf( msg_buf, "[UpdateSourceControlInfo took = %li secs %hi millisecs]", b4.time-b3.time, b4.millitm-b3.millitm);
			sendMsg( std::string( msg_buf), MSG_INFO);
		}
#endif
#endif
	}
}

void CCoreXmlFile::getLatestAndLoad()
{
	char          buf[_MAX_PATH];
	_finddata_t   fileInfo;
	time_t        currentTime1 = time( &currentTime1 );
	CTime         currentTime2( currentTime1 - 100 );

	FILETIME      last_cache_write_time;
	timestampOfCache( &last_cache_write_time);

	// get latest version from source control
	getLatestVersion();

	// itarete on files and read new and modified files
	sprintf( buf, "%s\\*.xml", m_folderPath.c_str() );
	long searchHandle = _findfirst( buf, &fileInfo );
	long ret = searchHandle;
	while( ret != -1 )
	{
		sprintf( buf, "%s\\%s", m_folderPath.c_str(), fileInfo.name );

		WIN32_FILE_ATTRIBUTE_DATA attr;        
		if( GetFileAttributesEx( buf, GetFileExInfoStandard, &attr ) )
		{
			// A GOOD TEST STILL NEEDED BELOW::::
			// this test is also not good, because the cache write time although is older 
			// then modelopen time (in which session the cache file was written out)
			// but the cache file reflects only the state of the project in modelopen time
			if( 1 == CompareFileTime( &attr.ftLastWriteTime, &last_cache_write_time)) // attr.ftLastWriteTime > last_cache_write_time
			{
				// this is a new or modified file, read it
				UnresolvedPointerVec pointers;
				readXMLFile( buf, pointers, false );
				resolvePointers( pointers);
			}
			// earlier this was used:
			// this might bave been a bad test:
			//CTime modTime( attr.ftLastWriteTime );
			//if( modTime >= currentTime2 ) { }
		}

		ret = _findnext( searchHandle, &fileInfo );
	}
	_findclose( searchHandle );
}

bool CCoreXmlFile::getUserCredentialInfo( int p_svnText, bool p_requireLogin)
{
	bool aborted = false;
	if( p_svnText != 0)
	{
		bool is_ssh_hinted = isUrlSvnSsh();
		CSvnLoginDlg dlg( is_ssh_hinted? 2 : 0);
		dlg.m_project  = m_projectFileName.c_str();
		dlg.m_database = m_svnUrl.c_str();
		dlg.m_user     = m_userOpts.m_useAccountInfo? m_userOpts.m_defUserName.c_str(): userNameFromSvnSshUrl().c_str();
		dlg.m_password = m_userOpts.m_useAccountInfo? m_userOpts.m_defPassword.c_str(): "";
		if( !is_ssh_hinted) // https etc.
			dlg.disableSshOption();

		if( m_userOpts.m_useAccountInfo && m_userOpts.m_automaticLogin || dlg.DoModal() == IDOK )
		{
			m_vssUser     = dlg.m_user;
			m_vssPassword = dlg.m_password;
			return true;
		}
		aborted = dlg.wasAborted();
	}
	else
	{
		CVSSLoginDlg dlg;
		dlg.m_project  = m_projectFileName.c_str();
		dlg.m_database = m_vssDatabaseStr.c_str();
		dlg.m_user     = m_userOpts.m_useAccountInfo? m_userOpts.m_defUserName.c_str(): "";
		dlg.m_password = m_userOpts.m_useAccountInfo? m_userOpts.m_defPassword.c_str(): "";

		// execute DoModal only if one of the booleans below is false
		if( m_userOpts.m_useAccountInfo && m_userOpts.m_automaticLogin || dlg.DoModal() == IDOK )
		{
			m_vssUser     = dlg.m_user;
			m_vssPassword = dlg.m_password;
			return true;
		}
		aborted = dlg.wasAborted();
	}

	// we are sure that not IDOK was pressed
	if( p_requireLogin)
		AfxMessageBox( "Could not process further without login information.", MB_ICONEXCLAMATION);

	if( aborted || p_requireLogin)
		HR_THROW( E_UNKNOWN_STORAGE); // this will imply a relatively silent abort, with no further assertions

	return false;
}

CComObjPtr<IVSSItem> CCoreXmlFile::createFolderSS( CComObjPtr<IVSSItem> p_parentItem, CComBSTR p_subProjName, CComBSTR p_localSpec)
{
	CComObjPtr<IVSSItem> item;
	COMTHROW( p_parentItem->NewSubproject( p_subProjName, NULL, &(item.p) ));

	COMTHROW( item->put_LocalSpec( p_localSpec) );

	return item;
}

void CCoreXmlFile::createSourceSafeDatabase()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	m_sourceControl = SC_NONE;

	std::string  msg;

	// create source control project if specified one
	if( m_vssDatabaseStr.size() > 0 )
	{
		if( m_vssUser.size() > 0 || getUserCredentialInfo( 0, true) )
		{
			// try to open the sorusafe database
			try
			{
				COMTHROW( CoCreateInstance(CLSID_VSSDatabase, NULL, CLSCTX_ALL, IID_IVSSDatabase, (void**)&(m_vssDatabase.p) ));

				CComBSTR path  = m_vssDatabaseStr.c_str();
				CComBSTR user2 = m_vssUser.c_str();
				CComBSTR pwd2  = m_vssPassword.c_str();

				COMTHROW( m_vssDatabase->Open( path, user2, pwd2 ) );

				CComObjPtr<IVSSItem> item;
				CComBSTR parentProject = m_vssParentPath.c_str();
				COMTHROW( m_vssDatabase->get_VSSItem( parentProject, false, &(item.p)) );

				// create project folder locally
				BOOL res;
				res = CreateDirectory( m_folderPath.c_str(), NULL);
				if( !res)  
				{
					sendMsg( "Exception: Could not create project folder '" + m_folderPath + "'!", MSG_ERROR);
					HR_THROW(E_FILEOPEN);
				}

				// create project folder on the server
				CComObjPtr<IVSSItem> proj_main_item = createFolderSS( item, m_projectName.c_str(), m_folderPath.c_str());
				if( !proj_main_item) 
				{
					sendMsg( "Exception: Could not create versioned folder '" + m_projectName + "'!", MSG_ERROR);
					HR_THROW(E_FILEOPEN);
				}

				//CComObjPtr<IVSSItem> item2;
				//CComBSTR subProjectName = m_projectName.c_str();
				//COMTHROW( item->NewSubproject( subProjectName, NULL, &(item2.p) ));

				//CComBSTR localSpec = m_folderPath.c_str();
				//COMTHROW( item2->put_LocalSpec(localSpec) );

				if( m_hashFileNames)
				{
					int succ = 0;
					// create folders locally
					succ = createHashedFolders();
					if( !succ) {
						
						sendMsg( "Exception: Could not create initial directory structure!", MSG_ERROR);
						AfxMessageBox( "Could not create initial directory structure.");
						HR_THROW(E_FILEOPEN);
					}

					// add/create folders to/in versioning system
					// create project folder on the server

					CComObjPtr<IVSSItem>      null_item(0);

					// first add the content folder
					CComObjPtr<IVSSItem> proj_cont_item = createFolderSS( proj_main_item, m_contentConst, m_contentPath.c_str());
					succ = null_item != proj_cont_item;

					// then the subfolders
					DirSupplier ds( m_hashFileNames, m_hashVal);

					if( m_hashVal == 2)
					{
						for( Dir256Iterator it = ds.begin256(); succ && it != ds.end256(); ++it)
							succ = null_item != createFolderSS( proj_cont_item, (*it).c_str(), (m_contentPath + "\\" + *it).c_str());
					}
					else if( m_hashVal == 5)
					{
						CComObjPtr<IVSSItem> proj_lev1_item;
						CComObjPtr<IVSSItem> proj_lev2_item;
						std::string          f_lev1, s_lev1,s_lev2;
						for( Dir16Iterator it = ds.begin16(); succ && it != ds.end16(); ++it)
						{
							s_lev1 = *it;
							f_lev1 = m_contentPath + "\\" + s_lev1;

							proj_lev1_item = createFolderSS( proj_cont_item, s_lev1.c_str(), f_lev1.c_str());
							succ = null_item != proj_lev1_item;

							for( Dir256Iterator jt = ds.begin256(); succ && jt != ds.end256(); ++jt)
							{
								s_lev2 = *jt;
								succ = null_item != createFolderSS( proj_lev1_item, s_lev2.c_str(), (f_lev1 + "\\" + s_lev2).c_str()); 
							}
						}
					}

					if( !succ) {
						sendMsg( "Exception: Could not create initial project structure on server!", MSG_ERROR);
						AfxMessageBox( "Could not create initial project structure on server.");
						HR_THROW(E_FILEOPEN);
					}
				}

				m_sourceControl = SC_SOURCESAFE;
			}
			catch(...)
			{
				// error during the sourcesafe project creation
				msg = "Exception during SourceSafe project creation.";
			}
		}
		else
		{
			// not logged into sourcesafe
			msg = "Not logged in.";
		}
	} else msg = "No SourceSafe Database specification found.";

	if( m_sourceControl == SC_NONE )
	{
		// TODO: error message
		AfxMessageBox( ("The project is not under any source control system!\nReason: " + msg).c_str(), MB_ICONEXCLAMATION);
	}

}

void CCoreXmlFile::openSourceSafeDatabase()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	if( m_vssUser.size() > 0 || getUserCredentialInfo( 0, false) )
	{
		// try to open the sorusafe database
		COMTHROW( CoCreateInstance(CLSID_VSSDatabase, NULL, CLSCTX_ALL, IID_IVSSDatabase, (void**)&(m_vssDatabase.p) ));

		CComBSTR path  = m_vssDatabaseStr.c_str();
		CComBSTR user2 = m_vssUser.c_str();
		CComBSTR pwd2  = m_vssPassword.c_str();

		HRESULT hr = m_vssDatabase->Open( path, user2, pwd2 );

		if( hr == 0 )
		{
			m_sourceControl = SC_SOURCESAFE;

			CComObjPtr<IVSSItem> item;
			CComBSTR path2 = m_vssPath.c_str();
			COMTHROW( m_vssDatabase->get_VSSItem( path2, false, &(item.p)) );

			CComBSTR localSpec = m_folderPath.c_str();
			COMTHROW( item->put_LocalSpec(localSpec) );
		}
		else
		{
			char buf[200];
			sprintf( buf, "Could not connect to SourceSafe database. Error code: %x (%d). You may lose synchronization with the version controlled project. You won't be able to modify read-only files.", hr, hr );
			AfxMessageBox( buf, MB_ICONEXCLAMATION );
			m_sourceControl = SC_NONE;
		}
	}
	else
	{
		AfxMessageBox( "You did not log in the SourceSafe database. You may lose synchronization with the version controlled project. You won't be able to modify read-only files.", MB_ICONEXCLAMATION );
		m_sourceControl = SC_NONE;
	}
}

void CCoreXmlFile::getSourceSafePath(XmlObject * obj, std::string& str)
{
	ASSERT( m_sourceControl == SC_SOURCESAFE );

	std::string fileName;
	getContainerFileName( obj, fileName, false );

	str = m_vssPath;
	str += "\\";
	str += fileName;
}

bool CCoreXmlFile::isContainerReadOnly(XmlObject * obj)
{
	ASSERT( obj != NULL );

	std::string fileName;
	getContainerFileName( obj, fileName );

	if( FileHelp::fileExist( fileName))
		return FileHelp::isFileReadOnly( fileName);
	else
	{
		// the file does not exiest (not written out yet)
		return false;
	}
}

bool CCoreXmlFile::isContinerCheckedOut(XmlObject * obj)
{
	ASSERT( m_sourceControl != SC_NONE );

	if( isSS())
	{
		std::string fullPath;
		getSourceSafePath( obj, fullPath );
		CComBSTR fullPath2 = fullPath.c_str();

		CComObjPtr<IVSSItem> item;        
		long checkOutState;
		COMTHROW( m_vssDatabase->get_VSSItem( fullPath2, false, &(item.p)) );
		COMTHROW( item->get_IsCheckedOut(&checkOutState) );

		return checkOutState == VSSFILE_CHECKEDOUT;
	}
	else if( isCC())
	{
		std::string fileName;
		getContainerFileName( obj, fileName, true );
		return isFileCheckedOutCC( fileName.c_str() );
	}
	else if( isSV())
	{
		// freshly added, test this. 98765
		std::string fileName;
		getContainerFileName( obj, fileName, true );
		return isCheckedOutByElseSVN( fileName);
	}

	return false;
}

void CCoreXmlFile::getSSLastCommiter(XmlObject * obj, std::string& user)
{
	ASSERT( m_sourceControl == SC_SOURCESAFE );
	ASSERT( obj->isContainer() );
	user = "<UnknownUser>";

	// currently does not work, because IVSSVersions is a special collection
	//char buf[300];

	//std::string fullPath;
	//getSourceSafePath( obj, fullPath );
	//CComBSTR fullPath2 = fullPath.c_str();

	//try { // by zolmol
	//	CComObjPtr<IVSSItem> item;
	//	COMTHROW( m_vssDatabase->get_VSSItem( fullPath2, false, &(item.p)));

	//	long ver_nmb = -1;
	//	COMTHROW( item->get_VersionNumber( &ver_nmb));

	//	CComObjPtr<IVSSVersions> vers;
	//	COMTHROW( item->get_Versions( 0, &(vers.p)));

	//	long cnt = 0;
	//	if( vers) COMTHROW( vers->get_Count( &cnt));
	//	if( cnt > 0 && ver_nmb >= 1 && ver_nmb <= cnt)
	//	{
	//		CComVariant vv; vv = ver_nmb;
	//		CComObjPtr<IVSSVersion> ver;
	//		COMTHROW( item->get_Version( vv, &(ver.p)));
	//		COMTHROW( vers->get_Item( ver_nmb, &(ver.p)));

	//		CComBSTR nm;
	//		if( ver) COMTHROW( ver->get_Username( &nm));

	//		sprintf( buf, "%S", nm );

	//		user = buf;
	//	}
	//	// else
	//	{
	//	}
	//} catch(hresult_exception&) {
	//	user = "";
	//}
}

void CCoreXmlFile::getSSCurrentOwner(XmlObject * obj, string& user, bool& newfile) // getSSCheckOutUser
{
	ASSERT( m_sourceControl == SC_SOURCESAFE );
	ASSERT( obj->isContainer() );

	char buf[300];
	newfile = false;

	string fullPath;
	getSourceSafePath( obj, fullPath );
	CComBSTR fullPath2 = fullPath.c_str();
	try { // by zolmol
		// get_VSSItem() may fail in case a new element has been created recently
		CComObjPtr<IVSSItem> item;
		if( S_OK == m_vssDatabase->get_VSSItem( fullPath2, false, &(item.p)))
		{
			CComObjPtr<IVSSCheckouts> checkouts;
			COMTHROW( item->get_Checkouts( &(checkouts.p) ));
			long checkoutNum;
			COMTHROW( checkouts->get_Count( &checkoutNum ) );
			if( checkoutNum > 0 )
			{
				CComObjPtr<IVSSCheckout> checkout;
				VARIANT index;
				index.vt = VT_INT;
				index.intVal = 1;
				COMTHROW( checkouts->get_Item(index, &(checkout.p)));

				CComBSTR userName;
				COMTHROW( checkout->get_Username( &userName ));

				string name, type;
				getContainerName( obj, name, type );

				sprintf( buf, "%S", userName );

				user = buf;
			}
			else
			{
				// not checked out
				user = "";
			}
		}
		else // by zolmol
		{
			// new element not yet saved 
			//ATLASSERT(( "New element not yet saved, thus not found in VSS", 0));
			user = ""; // as if not checked out
			newfile = true;
		}
	} catch(hresult_exception&) { // by zolmol
		// new element not yet saved 
		//ATLASSERT(( "New element not yet saved, thus not found in VSS", 0));
		user = ""; // as if not checked out
		newfile = true;
	}
}

void CCoreXmlFile::checkOutContainer(XmlObject * obj)
{
	ASSERT( m_sourceControl != SC_NONE );

	if( isSS())
	{
		std::string fullPath;
		getSourceSafePath( obj, fullPath );
		CComBSTR fullPath2 = fullPath.c_str();

		CComObjPtr<IVSSItem> item;
		COMTHROW( m_vssDatabase->get_VSSItem( fullPath2, false, &(item.p)) );
		//COMTHROW( item->Checkout( L"", NULL, VSSFLAG_TIMEMOD ) ); // previously was NULL // VSSFLAG_TIMEMOD, VSSFLAG_TIMENOW (default), or VSSFLAG_TIMEUPD
		std::string fileName;
		getContainerFileName( obj, fileName);
		CComBSTR loca = fileName.c_str();
		COMTHROW( item->Checkout( L"", loca, VSSFLAG_TIMEMOD ) ); // previously was NULL // VSSFLAG_TIMEMOD, VSSFLAG_TIMENOW (default), or VSSFLAG_TIMEUPD
	}
	else if( isCC())
	{
		std::string fileName;
		getContainerFileName( obj, fileName, true );
		checkOutFileCC( fileName.c_str() );
	}
	else if( isSV())
	{
		std::string file_name;
		getContainerFileName( obj, file_name, true);
		applyLockSVN( file_name);
	}
}

void CCoreXmlFile::rollBackTheCheckOutContainer(XmlObject * obj)
{
	ASSERT( m_sourceControl != SC_NONE );

	std::string fileName;
	getContainerFileName( obj, fileName);

	try
	{
		if( isSS())
		{
			std::string fullPath;
			getSourceSafePath( obj, fullPath );
			CComBSTR fullPath2 = fullPath.c_str();

			CComObjPtr<IVSSItem> item;
			COMTHROW( m_vssDatabase->get_VSSItem( fullPath2, false, &(item.p)) );
			CComBSTR loca = fileName.c_str();
			COMTHROW( item->UndoCheckout( loca, 0));
		}
		else if( isCC())
		{
			checkInFileCC( fileName.c_str() );
		}
		else if( isSV())
		{
			bool sc = removeLockSVN( fileName);
			if( !sc) HR_THROW( E_FAIL);
		}
	}
	catch(...)
	{
		sendMsg( "Could not rollback the lock for " + fileName, MSG_WARNING);
	}
}

void CCoreXmlFile::addToSourceControl(XmlObject * container, bool p_fileExisted)
{
	ASSERT( container->isContainer() );

	std::string fileName;
	getContainerFileName(container, fileName);

	if( isSS())
	{
		// get project item
		CComObjPtr<IVSSItem> projectItem;
		CComBSTR path;
		if( m_hashFileNames && ( m_hashVal == 2 || m_hashVal == 5))
			path = (m_vssPath + fileName.substr( m_folderPath.size(), fileName.size() - m_folderPath.size() - strlen("\\ce21dcacd8dced44896c385d004c6fa4.xml"))).c_str();
		else
			path = m_vssPath.c_str();

		COMTHROW( m_vssDatabase->get_VSSItem( path, false, &(projectItem.p)) );

		// add to source control projecct
		CComObjPtr<IVSSItem> item;
		CComBSTR fileName2 = fileName.c_str();
		if( projectItem->Add( fileName2, NULL, 0, &(item.p)) == 0 )
		{
			// check it out
			checkOutContainer( container );
		}
	}
	else if( isCC())
	{
		// parent folder must be checked out
		addFileToCC( fileName.c_str() );
	}
	else if( isSV())
	{
		bool sc_add = true;
		bool sc_pro = true;
		bool sc_com = true;
		if( !p_fileExisted) // if( !isVersionedInSVN( fileName)) // do add and lockable only for new files
		{
			sc_add = addSVN( fileName);
			sc_pro = lockablePropertySVN( fileName);
		}
		
		if( !m_userOpts.m_useBulkCommit)                         // if bulk commit then avoid individual commits
			sc_com = commitSVN( fileName, !p_fileExisted);

		if( !(sc_com && sc_pro && sc_add))
		{
			if(      !sc_add) sendMsg( "Could not add file " + fileName, MSG_ERROR);
			else if( !sc_pro) sendMsg( "Could not set lockable property for " + fileName, MSG_ERROR);
			else if( !sc_com) sendMsg( "Could not commit file " + fileName, MSG_ERROR);
			throw hresult_exception( -1);
		}
	}
}

/* ****************************************************************************** */
/*        C L A S S     P R O T E C T E N T R Y                                   */
/* ****************************************************************************** */
ProtectEntry::ProtectEntry( GUID p_gd, OpCode p_op, CTime p_time)
: m_guid( p_gd)
, m_op( p_op)
, m_time( p_time)
{ }

/* ****************************************************************************** */
/*        C L A S S     P U B L I C S T O R A G E                                 */
/* ****************************************************************************** */
PublicStorage::PublicStorage()
: m_parent( 0)
{
}

void PublicStorage::setParent( CCoreXmlFile* p_parent)
{
	m_parent = p_parent;
}

void PublicStorage::init( const std::string& p_initialContent)
{
	if( !m_parent)
		return;

	if( !m_parent->makeSureFileExistsInVerSys( m_fileName, p_initialContent))
	{
		m_parent->sendMsg( "Could not find files in Versioning System", MSG_ERROR);
		ASSERT(0);
		return;
	}

	if( isSS())
		// setting m_vssItem
		m_parent->getFileHandleSS( m_fileName, m_vssItem);
	else if( isCC())
		// setting m_ccsItem
		m_ccsItem = m_localFileName.c_str(); // ok, as long as m_localFileName doesn't change
	else if( isSV())
		m_ccsItem = m_localFileName.c_str(); // same as above
}

void PublicStorage::acquireSS( CComObjPtr<IVSSItem>& obj)
{
	long co_stat;
	COMTHROW( obj->get_IsCheckedOut( &co_stat));
	if( co_stat != VSSFILE_CHECKEDOUT_ME)
		COMTHROW( obj->Checkout( L"", 0, VSSFLAG_TIMEMOD));
}

void PublicStorage::releaseSS( CComObjPtr<IVSSItem>& obj)
{
	COMTHROW( obj->Checkin( L"", 0, 0));
}

void PublicStorage::acquireCC( const char * obj)
{
	m_parent->checkOutFileCC( obj);
}

void PublicStorage::releaseCC( const char * obj)
{
	m_parent->checkInFileCC( obj);
}

void PublicStorage::acquireSVN( const char * obj)
{
	if( FileHelp::isFileReadOnly( obj))
	{
		m_parent->updateSVN( obj); // SVN needs updating before any lock can be placed
		m_parent->applyLockSVN( obj);
		//ASSERT( !m_parent->isFileReadOnly( obj));
	}
}

void PublicStorage::releaseSVN( const char * obj)
{
	bool sc = m_parent->commitSVN( obj, false);
	ASSERT( sc);
	if( !sc)
	{
		m_parent->sendMsg( std::string( "Could not commit file ") + obj, MSG_ERROR);
	}

	if( !FileHelp::isFileReadOnly( obj)) // probably the file did not change, so that's why the commit did not remove the lock
	{
		bool sc = m_parent->removeLockSVN( obj);
		ASSERT( sc);
		ASSERT( FileHelp::isFileReadOnly( obj));
	}
}

void PublicStorage::acquireFile()
{
	if( isSS())
	{
		acquireSS( m_vssItem);
	}
	else if( isCC())
	{
		acquireCC( m_ccsItem);
	}
	else if( isSV())
	{
		acquireSVN( m_ccsItem);
	}
}

void PublicStorage::releaseFile()
{
	if( isSS())
	{
		releaseSS( m_vssItem);
	}
	else if( isCC())
	{
		releaseCC( m_ccsItem);
	}
	else if( isSV())
	{
		releaseSVN( m_ccsItem);
	}
}

std::string PublicStorage::userName() { return m_parent->userName(); }
bool        PublicStorage::isSS()     { return m_parent->isSS();     }
bool        PublicStorage::isCC()     { return m_parent->isCC();     }
bool        PublicStorage::isSV()     { return m_parent->isSV();     }


/* ****************************************************************************** */
/*        C L A S S     S I G N M A N A G E R                                     */
/* ****************************************************************************** */

void SignManager::setParent( CCoreXmlFile* p_parent)
{
	PublicStorage::setParent( p_parent);

	m_fileName = HelperFiles::signFileName;
	m_localFileName = m_parent->m_folderPath + "\\" + m_fileName;

	PublicStorage::init( "<users/>");
}

bool SignManager::anybodyElseHolding()
{
	if( isSS())
	{
		long checkOutState = VSSFILE_CHECKEDOUT;
		COMTHROW( m_vssItem->get_IsCheckedOut(&checkOutState));
		return checkOutState == VSSFILE_CHECKEDOUT; // checked out by somebody else
	}
	else if( isCC())
	{
		int checkOutState = m_parent->getCheckOutStateCC( m_ccsItem);
		return checkOutState == CCoreXmlFile::CS_OTHER_USER; // checked out by somebody else
	}
	else if( isSV())
	{
		return m_parent->isCheckedOutByElseSVN( m_ccsItem);
	}
	return false;
}

void SignManager::in_or_off( bool in)
{
	const char * msg_in = "Could not sign in yet. Press OK to try again.";
	const char * msg_out= "Could not sign out yet. Press OK to try again.";

	if( !isSS() && !isCC() && !isSV()) return;
	try
	{
		bool lost_patience          = false; // once this will turn true further attempts will cease
		bool successful_acquisition = false;

		while( !successful_acquisition && !lost_patience)
		{
			bool chance_for_acquisition = false;

			while( !chance_for_acquisition && !lost_patience)
			{
				chance_for_acquisition = !anybodyElseHolding();
				if( !chance_for_acquisition)
				{
					// others hold a lock to the signin file
					// notify user and retry if needed
					if( IDCANCEL == AfxMessageBox( in?msg_in:msg_out, MB_OKCANCEL))
						lost_patience = true; // quit, no further attempts
				}
			}

			if( chance_for_acquisition) // user did not give up, no one holding the file, chance to acquire it
			{
				try {
					acquireFile();
					successful_acquisition = true;
				}
				catch(hresult_exception&) {
					// somebody else acquired it in the meantime
					if( IDCANCEL == AfxMessageBox( in?msg_in:msg_out, MB_OKCANCEL))
						lost_patience = true; // quit, no further attempts
				}
			}
			// else: lost_patience turned true, so this loop will terminate too
		}

		if( successful_acquisition)
		{
			if( isSV()) {
				std::string owner;
				m_parent->infoSVN( this->m_ccsItem, false, std::string(), std::string(), owner);
				if( owner != userName()) {
					if( IDYES == AfxMessageBox( CString( "Username mismatch found. The signature file recently locked by you, reports to be locked by user: '") + owner.c_str() + "' while you have identified initially yourself as '" + userName().c_str() + "'.\nWould you like to continue with '" + owner.c_str() + "' username?", MB_YESNO)) {
						m_parent->replaceUserName( owner);
					}
				}
			}

			try {
				update( in, SignFileEntry( userName(), CTime::GetCurrentTime()));
			} catch(hresult_exception&) { // handled cases throw HRESULT from inside
				m_parent->sendMsg( "Could not update signature file! Exception happened.", MSG_ERROR);
			}

			releaseFile();
		}
	}
	catch(...)
	{
		AfxMessageBox( "Could not proceed with sign in/off. Exception happened.");
	}
}

void SignManager::update( bool p_in, const SignFileEntry& p_entry)
{
	DOMImplementationLS    * domimpl = NULL;
	DOMBuilder             * parser  = m_parent->getFreshParser( "SignatureFileUpdater", &domimpl);

	ASSERT( parser != NULL );
	if( !parser) {
		m_parent->sendMsg( "DOMBuilder pointer is NULL!", MSG_ERROR);
		HR_THROW(E_FILEOPEN);
	}

	//
	// Parsing
	XERCES_CPP_NAMESPACE::DOMDocument * doc = 0;
	try {

		doc = parser->parseURI( m_localFileName.c_str() );

	} 
	catch( const OutOfMemoryException&) {

		doc = 0;

		m_parent->sendMsg( "OutOfMemoryException during parsing.", MSG_ERROR);
	}
	catch (const SAXException&) { 

		doc = 0;

		m_parent->sendMsg( "SAXException during parsing.", MSG_ERROR);
	}
	catch (const XMLException& e) 
	{
		doc = 0;

		char* e_msg = XMLString::transcode( e.getMessage());
		m_parent->sendMsg( (std::string( "XMLException during parsing. Message: ") + e_msg).c_str(), MSG_ERROR);

		XMLString::release( &e_msg);
	}
	catch (const DOMException& e) 
	{
		doc = 0;

		char*              e_msg    = 0;
		const unsigned int maxChars = 2047;
		XMLCh              errText[maxChars + 1];

		if( DOMImplementation::loadDOMExceptionMsg( e.code, errText, maxChars))
		{
			e_msg = XMLString::transcode( errText);
			m_parent->sendMsg( (std::string( "DOMException during parsing. Message: ") + e_msg).c_str(), MSG_ERROR);
		}
		else
		{
			e_msg = XMLString::transcode( e.getMessage());
			m_parent->sendMsg( (std::string( "DOMException during parsing. Message: ") + e_msg).c_str(), MSG_ERROR);
		}
		XMLString::release( &e_msg);
	}
	catch (...) {

		doc = 0;

		m_parent->sendMsg( "GenException during parsing.", MSG_ERROR);
	}

	if( doc == NULL )
	{
		m_parent->sendMsg( "DOMDocument pointer is NULL. Parsing of signature file failed.", MSG_ERROR);
		HR_THROW(E_FILEOPEN);
	}

	//
	// The updating process itself:
	try
	{
		DOMElement * doc_e = 0;
		try {
			doc_e = doc->getDocumentElement();
		}
		catch( const XMLException& ) { doc_e = 0; }
		catch( const DOMException& ) { doc_e = 0; }
		catch( ... )                  { doc_e = 0; }

		if( doc_e == NULL)
		{
			m_parent->sendMsg( "Exception: Null document element in signature file!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		XMLCh * x_users = XMLString::transcode( ParserLiterals::Signer::users);
		ASSERT( XMLString::equals( x_users, doc_e->getTagName()));

		XMLCh* x_user = XMLString::transcode( ParserLiterals::Signer::user);

		DOMNodeList *uss = doc_e->getElementsByTagName( x_user);

		XMLCh* x_name = XMLString::transcode( ParserLiterals::Signer::name);
		XMLCh* x_since= XMLString::transcode( ParserLiterals::Signer::since);
		XMLCh* x_until= XMLString::transcode( ParserLiterals::Signer::until);
		XMLCh* x_empty= XMLString::transcode( ParserLiterals::empty);
		XMLCh* x_newln= XMLString::transcode( ParserLiterals::newln);

		XMLCh* x_time = XMLString::transcode( (LPCTSTR) p_entry.m_time.Format( _T("[%Y-%m-%d %H:%M:%S]")));
		XMLCh* x_username = XMLString::transcode( p_entry.m_username.c_str());

		bool found_already = false;
		int len = (int) uss->getLength();
		for( int i = 0; i < len; ++i)
		{
			DOMNode * node = uss->item(i);
			DOMElement* us = (DOMElement*) node; // user

			char * name = XMLString::transcode( us->getAttribute( x_name));

			if( 0 == stricmp( name, p_entry.m_username.c_str())) // user info found
			{
				if( found_already) // this is the 2nd entry with that username
				{
					doc_e->removeChild( node);
					continue;
				}

				// update accordingly
				if( p_in) // sign on
				{
					// found: already signed in
					// update this element
					us->setAttribute( x_since, x_time);
					us->setAttribute( x_until, x_empty);
					found_already = true; // no need for adding a new entry
				}
				else // sign off
				{
					// if entry is removed:
					//doc_e->removeChild( node);

					// update the logoff attribute
					us->setAttribute( x_until, x_time);
					found_already = true;
				}
			}

			XMLString::release( &name);
		}

		// if this is the first time the user logs in (entry not found yet):
		if( !found_already && p_in)
		{
			if( len == 0) // first ever user
			{
				DOMText* ntxt = doc->createTextNode( x_newln);
				doc_e->appendChild( ntxt);
			}

			DOMElement *nch = doc->createElement( x_user);
			nch->setAttribute( x_name , x_username);
			nch->setAttribute( x_since, x_time);
			nch->setAttribute( x_until, x_empty);
			doc_e->appendChild( nch);

			DOMText* ntxt = doc->createTextNode( x_newln);
			doc_e->appendChild( ntxt);
		}

		XMLString::release( &x_users);
		XMLString::release( &x_user);
		XMLString::release( &x_name);
		XMLString::release( &x_since);
		XMLString::release( &x_until);
		XMLString::release( &x_empty);
		XMLString::release( &x_newln);

		XMLString::release( &x_time);
		XMLString::release( &x_username);
	}
	catch(...)
	{
		if( parser) delete parser;
		m_parent->sendMsg( "Exception during signature file update!", MSG_ERROR);

		HR_THROW(E_FILEOPEN);
	}

	//
	// do a DOM save as follows:
	try
	{
		XMLCh* x_filenm = XMLString::transcode( m_localFileName.c_str());
		XMLFormatTarget* outfile = new LocalFileFormatTarget( x_filenm);
		XMLString::release( &x_filenm);

		DOMWriter* writer = domimpl->createDOMWriter();
		if( writer && writer->canSetFeature( XMLUni::fgDOMXMLDeclaration, false))
			writer->setFeature( XMLUni::fgDOMXMLDeclaration, false);

		writer->writeNode( outfile, *doc );
		delete outfile;
		delete writer;

		// delete the parser object
		delete parser;
	}
	catch(...)
	{
		if( parser) delete parser;
		m_parent->sendMsg( "DOMWriter exception during signature file update!", MSG_ERROR);
		
		HR_THROW(E_FILEOPEN);
	}
}

SignManager::SignFileDataVec SignManager::getUserData()
{
	SignFileDataVec res;

	DOMBuilder * parser = m_parent->getFreshParser( "SignatureFileAnalyzer");

	ASSERT( parser != NULL );
	if( !parser)
	{
		m_parent->sendMsg( "Exception: Could not create parser!", MSG_ERROR);
		return res;
	}

	try {
		XERCES_CPP_NAMESPACE::DOMDocument * doc = parser->parseURI( m_localFileName.c_str());
		if( doc == NULL )
		{
			m_parent->sendMsg( "Exception: Could not parse signature file!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		DOMElement * doc_e = doc->getDocumentElement();
		if( doc_e == NULL)
		{
			m_parent->sendMsg( "Exception: Null document element in signature file!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		XMLCh* x_users = XMLString::transcode( ParserLiterals::Signer::users);
		ASSERT( XMLString::equals( x_users, doc_e->getTagName()));
		XMLString::release( &x_users);

		XMLCh* x_user = XMLString::transcode( ParserLiterals::Signer::user);
		XMLCh* x_name = XMLString::transcode( ParserLiterals::Signer::name);
		XMLCh* x_since= XMLString::transcode( ParserLiterals::Signer::since);
		XMLCh* x_until= XMLString::transcode( ParserLiterals::Signer::until);

		DOMNodeList *uss = doc_e->getElementsByTagName( x_user);

		for( int i = 0; i < (int) uss->getLength(); ++i)
		{
			DOMNode * node = uss->item(i);
			DOMElement* us = (DOMElement*) node; // user

			char * name = XMLString::transcode( us->getAttribute( x_name));
			char * since= XMLString::transcode( us->getAttribute( x_since));
			char * until= XMLString::transcode( us->getAttribute( x_until));

			SignFileData data( name, since, until);
			if( std::find( res.begin(), res.end(), data) == res.end()) // not found
				res.push_back( data);

			XMLString::release( &name);
			XMLString::release( &since);
			XMLString::release( &until);
		}

		XMLString::release( &x_user);
		XMLString::release( &x_name);
		XMLString::release( &x_since);
		XMLString::release( &x_until);

		// delete the parser object
		delete parser;

	} catch(...) {
		if( parser) delete parser;
		m_parent->sendMsg( "Parser exception during singature file analysis!", MSG_ERROR);
	}

	return res;
}

/* ****************************************************************************** */
/*        C L A S S     P R O T E C T L I S T                                     */
/* ****************************************************************************** */

void ProtectList::setParent( CCoreXmlFile* p_parent)
{
	PublicStorage::setParent( p_parent);

	m_fileName = getProtListFileName( userName());
	m_localFileName = m_parent->m_folderPath + "\\" + m_fileName;

	PublicStorage::init( "<objects/>");
}

std::string ProtectList::getProtListFileName( const std::string& p_username)
{
	return std::string( HelperFiles::protFileName) + p_username + HelperFiles::protFileExt;
}

void ProtectList::onLoad()
{
	ASSERT( m_parent);
	if( !m_parent) return;
	if( !isSS() && !isCC() && !isSV()) return;

	try
	{
		acquireFile();
		// those items which are older than the last sync time for all users
		// can be removed from the list, the last sync time is the earliest of 
		// all currently active logins
		purgeProtList( m_parent->lastSyncTimeForAllUsers());
		releaseFile();
	} catch(hresult_exception&) {
		m_parent->sendMsg( "Could not purge old items from protection loglist.", MSG_ERROR);
	}
}

void ProtectList::onAborted()
{
	if( !isSS() && !isCC() && !isSV()) return;
	clearProtList();
}

void ProtectList::onCommited()
{
	if( !isSS() && !isCC() && !isSV()) return;
	if( !needed()) return;

	try {
		acquireFile(); // this file is better checkedin after modifications, because other
		// users strictly need to see the last (most up to date) version of it

		writeProtList(); // much costly than text based writeProtLisp()

		releaseFile();

		clearProtList();

	} catch(hresult_exception&) {
		m_parent->sendMsg( "Could not save <item> entries to my loglist.", MSG_ERROR);
	}
}

void ProtectList::addEntry( const ProtectEntry& p_pe)
{
	if( !isSS() && !isCC() && !isSV()) return;

	m_list.push_back( p_pe);
}

bool ProtectList::needed()
{
	return !m_list.empty();
}

void ProtectList::clearProtList()
{
	// called when Abort or Commit happens
	m_list.clear(); // clears the whole list
}

// let's just dump items into the file, without the enclosing 'objects' tag
// a text based, fast approach
void ProtectList::writeProtLisp()
{
	FILE * f = fopen( (m_localFileName+"3").c_str(), "a+b");
	if( !f)  return;

	// create <items> for all entries found in m_list
	for( unsigned int i = 0; i < m_list.size(); ++i)
	{
		string gd;
		guid2str( m_list[i].m_guid, gd );

		fprintf( f, "\r\n<item gd=\"%s\" oper=\"%s\" when=\"%s\"/>", gd.c_str(), OpCodeStr[ m_list[i].m_op], (LPCTSTR) m_list[i].m_time.Format( _T("[%Y-%m-%d %H:%M:%S]")));
	}

	fclose(f);
}

void ProtectList::writeProtList()
{
	DOMImplementationLS          * domimpl = NULL; 
	DOMBuilder                   * parser  = NULL;

	try {

		parser = m_parent->getFreshParser( "ProtectionListWriter", &domimpl);

		if( !domimpl || !parser)
		{
			m_parent->sendMsg( "Exception: Could not create parser!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		XERCES_CPP_NAMESPACE::DOMDocument * doc = parser->parseURI( m_localFileName.c_str() );
		if( doc == NULL )
		{
			m_parent->sendMsg( "Exception: Could not parse file '" + m_localFileName + "'!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		DOMElement * doc_e = doc->getDocumentElement();
		if( doc_e == NULL)
		{
			m_parent->sendMsg( "Exception: Null document element in file '" + m_localFileName + "'!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		XMLCh* ITEM_xiteral = XMLString::transcode( ParserLiterals::Protector::item);
		XMLCh* WHEN_xiteral = XMLString::transcode( ParserLiterals::Protector::when);
		XMLCh* OPER_xiteral = XMLString::transcode( ParserLiterals::Protector::oper);
		XMLCh* GUID_xiteral = XMLString::transcode( ParserLiterals::Protector::gd);
		XMLCh* OBJS_xiteral = XMLString::transcode( ParserLiterals::Protector::objects);

		ASSERT( XMLString::equals( OBJS_xiteral, doc_e->getTagName()));

		DOMNodeList *uss = doc_e->getElementsByTagName( ITEM_xiteral);

		// create <items> for all entries found in m_list
		for( unsigned int i = 0; i < m_list.size(); ++i)
		{
			string gd;
			guid2str( m_list[i].m_guid, gd );

			XMLCh* val_gd = XMLString::transcode( gd.c_str());
			XMLCh* val_tm = XMLString::transcode( (LPCTSTR) m_list[i].m_time.Format( _T("[%Y-%m-%d %H:%M:%S]")));
			XMLCh* val_op = XMLString::transcode( OpCodeStr[ m_list[i].m_op]);
			XMLCh* x_newln= XMLString::transcode( "\n");

			DOMElement *nch = doc->createElement( ITEM_xiteral);
			nch->setAttribute( GUID_xiteral, val_gd);
			nch->setAttribute( WHEN_xiteral, val_tm);
			nch->setAttribute( OPER_xiteral, val_op);

			doc_e->appendChild( nch);

			DOMText* ntxt = doc->createTextNode( x_newln); // as fprintf(f, "\n");
			doc_e->appendChild( ntxt);

			XMLString::release( &val_gd);
			XMLString::release( &val_tm);
			XMLString::release( &val_op);
			XMLString::release( &x_newln);
		}

		//do a DOM save as follows:
		XMLCh* x_fname = XMLString::transcode( m_localFileName.c_str());
		XMLFormatTarget* outfile = new LocalFileFormatTarget( x_fname);

		DOMWriter* writer = domimpl->createDOMWriter();

		if( writer->canSetFeature( XMLUni::fgDOMWRTDiscardDefaultContent, true))
			writer->setFeature( XMLUni::fgDOMWRTDiscardDefaultContent, true);
		if( writer->canSetFeature( XMLUni::fgDOMXMLDeclaration, false))
			writer->setFeature( XMLUni::fgDOMXMLDeclaration, false);

		doc->normalizeDocument();

		writer->writeNode( outfile, *doc );

		XMLString::release( &x_fname);
		XMLString::release( &ITEM_xiteral);
		XMLString::release( &WHEN_xiteral);
		XMLString::release( &OPER_xiteral);
		XMLString::release( &GUID_xiteral);
		XMLString::release( &OBJS_xiteral);

		delete outfile;
		delete writer;

		// delete the parser object
		delete parser;
	}
	catch(...)
	{
		m_parent->sendMsg( "Exception while writing protection file '" + m_localFileName + "'!", MSG_ERROR);
		if( parser) delete parser;
		return;
	}
}

void ProtectList::purgeProtList( CTime& p_lastSyncTime)
{
	DOMImplementationLS          * domimpl = NULL; 
	DOMBuilder                   * parser  = NULL;

	try {

		parser = m_parent->getFreshParser( "ProtectionListPurger", &domimpl);

		if( !domimpl || !parser)
		{
			m_parent->sendMsg( "Exception: Could not create parser!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		XERCES_CPP_NAMESPACE::DOMDocument * doc = parser->parseURI( m_localFileName.c_str() );
		if( doc == NULL )
		{
			m_parent->sendMsg( "Exception: Could not parse file '" + m_localFileName + "'!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		DOMElement * doc_e = doc->getDocumentElement();
		if( doc_e == NULL)
		{
			m_parent->sendMsg( "Exception: Null document element in file '" + m_localFileName + "'!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		XMLCh* ITEM_xiteral = XMLString::transcode( ParserLiterals::Protector::item);
		XMLCh* WHEN_xiteral = XMLString::transcode( ParserLiterals::Protector::when);
		XMLCh* OBJS_xiteral = XMLString::transcode( ParserLiterals::Protector::objects);
		XMLCh* x_newline    = XMLString::transcode( ParserLiterals::newln);

		ASSERT( XMLString::equals( OBJS_xiteral, doc_e->getTagName()));

		DOMNodeList *uss = doc_e->getElementsByTagName( ITEM_xiteral);

		// handle outdated items
		bool outdated;

		for( int i = (int) uss->getLength() - 1; i >= 0; --i)
		{
			DOMNode * node = uss->item(i);
			DOMElement* us = (DOMElement*) node; // user

			outdated = false;

			char * when = XMLString::transcode( us->getAttribute( WHEN_xiteral));

			int y(-1), M(-1), d(-1), h(-1), m(-1), s(-1);
			if( 6 == sscanf( when, "[%u-%u-%u %u:%u:%u]", &y, &M, &d, &h, &m, &s))
			{
				CTime whn(y, M, d, h, m, s); // when did happen that event
				if( whn < p_lastSyncTime)
				{
					outdated = true;
					doc_e->removeChild( node); // because of this we loop --i
				}
			}

			XMLString::release( &when);
		}

		doc->normalizeDocument();
		DOMElement * doc_f = doc->getDocumentElement();
		if( doc_f == NULL)
		{
			m_parent->sendMsg( "Exception: Null document element in file '" + m_localFileName + "' after normalization!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		// replace sequences of '\n' with just one
		DOMNodeList* list = doc_f->getChildNodes();

		for( int i = (int) list->getLength() - 1; i >= 0 ; --i)
		{
			DOMNode * node = list->item(i);
			if( node->getNodeType() == DOMNode::TEXT_NODE)
			{
				DOMText * txt  = (DOMText*) node;

				char* nlines = XMLString::transcode( txt->getData());
				std::string newlines( nlines);
				XMLString::release( &nlines);

				if( newlines.size() > 1 && std::string::npos == newlines.find_first_not_of( '\n')) // nothing else just '\n' characters
				{
					txt->setData( x_newline);
				}
			}
		}

		//do a DOM save as follows:
		XMLCh* x_fname = XMLString::transcode( m_localFileName.c_str());
		XMLFormatTarget* outfile = new LocalFileFormatTarget( x_fname);

		DOMWriter* writer = domimpl->createDOMWriter();
		if( writer == NULL)
		{
			m_parent->sendMsg( "Exception: Could not create DOM Writer for '" + m_localFileName + "'!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}

		if( writer->canSetFeature( XMLUni::fgDOMWRTDiscardDefaultContent, true))
			writer->setFeature( XMLUni::fgDOMWRTDiscardDefaultContent, true);
		if( writer->canSetFeature( XMLUni::fgDOMXMLDeclaration, false))
			writer->setFeature( XMLUni::fgDOMXMLDeclaration, false);

		doc->normalizeDocument();

		writer->writeNode( outfile, *doc );
		delete outfile;
		delete writer;

		// delete the parser object
		delete parser;

		XMLString::release( &x_fname);
		XMLString::release( &x_newline);
		XMLString::release( &ITEM_xiteral);
		XMLString::release( &WHEN_xiteral);
		XMLString::release( &OBJS_xiteral);
	}
	catch(...)
	{
		m_parent->sendMsg( "Exception while purging protection file '" + m_localFileName + "'!", MSG_ERROR);
		if( parser) delete parser;
		return;
	}
}

/* ****************************************************************************** */
/*        C L A S S                                                               */
/* ****************************************************************************** */

void CCoreXmlFile::protect( XmlObject * obj, OpCode oc)
{
	// the time stamp could be aqcuired only once, when commitToDisk happens
	if( obj) m_protectList.addEntry( ProtectEntry( obj->m_guid, oc, CTime::GetCurrentTime()));
}

bool CCoreXmlFile::findOnProtectedLists( GUID p_gd, std::string& p_scapegoatUser)
{
	if( !isSS() && !isCC() && !isSV()) return false;

	std::string str_gd; guid2str( p_gd, str_gd);

	std::vector< LoggedIn> ulist = allusers();

	bool found = false;
	for( std::vector< LoggedIn>::iterator it = ulist.begin()
		; !found && it != ulist.end()
		; ++it)
	{
		if( it->m_nm == userName()) continue; // ignore my file
		std::string fname = refreshProtectionFile( it->m_nm); // refresh my copy of this user's prot file

		if( found = findInFile( m_folderPath + "\\" + fname, str_gd))
			p_scapegoatUser = it->m_nm;
	}

	return found;
}

std::vector< LoggedIn> CCoreXmlFile::allusers()
{
	refreshSignFile();
	return getUsersFromSignFile();
}

bool CCoreXmlFile::refreshSignFile()
{
	return refreshOneFile( HelperFiles::signFileName);
}

void CCoreXmlFile::replaceUserName( const std::string& p_userName)
{
	m_vssUser = p_userName;
	m_comSvn->ReplaceUserName( CComBSTR( m_vssUser.c_str()));
}

std::string CCoreXmlFile::userName()
{
	if( isSS())
		return m_vssUser;
	else if( isCC())
	{
		// get windows (and clearcase) username
		char user_name[200];
		unsigned long user_name_sz = 200;
		GetUserName( user_name, &user_name_sz );

		return user_name;
	}
	else if( isSV())
	{
		return m_vssUser;
	}
	else
		return "";
}

bool CCoreXmlFile::isSS() { return m_sourceControl == CCoreXmlFile::SC_SOURCESAFE; }
bool CCoreXmlFile::isCC() { return m_sourceControl == CCoreXmlFile::SC_CLEARCASE; }
bool CCoreXmlFile::isSV() { return m_sourceControl == CCoreXmlFile::SC_SUBVERSION; }

bool CCoreXmlFile::userFilter( CTimeSpan& p_elapsed)
{
	//return p_elapsed.GetTotalMinutes() < 60*24*2; // less than 2 days ago
	return true;
}

std::vector< LoggedIn> CCoreXmlFile::getUsersFromSignFile()
{
	std::vector< LoggedIn>  res;
	SignManager::SignFileDataVec udata = m_signer.getUserData();
	for( SignManager::SignFileDataVec::const_iterator it = udata.begin();
		it != udata.end();
		++it)
	{
		//const char * date_last_logged_out = it->m_until.c_str();
		char status = it->m_until == ""?'A':'I'; // if logged in its logout date is ""

		const char * date_last_logged_in = it->m_since.c_str();
		int y(-1), M(-1), d(-1), h(-1), m(-1), s(-1);
		if( 6 == sscanf( date_last_logged_in, "[%u-%u-%u %u:%u:%u]", &y, &M, &d, &h, &m, &s))
		{
			CTime whn(y, M, d, h, m, s); // when did last login occur
			CTime now = CTime::GetCurrentTime();
			CTimeSpan elapsed = now - whn;
			if( userFilter( elapsed))
				if( std::find( res.begin(), res.end(), LoggedIn( it->m_user, status)) == res.end()) // not found
					res.push_back( LoggedIn( it->m_user, status)); // store its name
		}
	}
	return res;
}

CTime CCoreXmlFile::lastSyncTimeForAllUsers()
{
	// 
	//refreshSignFile(); // not needed now, we just signed on
	return findEarliestLogin( 0, 0, (int) ( m_userOpts.m_purgeDelayFactor * 60)); // days, hours, minutes
}

CTime CCoreXmlFile::findEarliestLogin( int p_nbOfDays, int p_nbOfHours, int p_nbOfMinutes)
{
	CTime earliest = CTime::GetCurrentTime();
	CTime earliest_at_most( earliest - CTimeSpan( p_nbOfDays, p_nbOfHours, p_nbOfMinutes, 0)); // no further back than p_nbOfDays days / p_nbOfHours
	bool avoid_limitation = earliest == earliest_at_most; // if no timespan provided we will not limit the earliest time

	SignManager::SignFileDataVec udata = m_signer.getUserData();
	for( std::vector< SignFileData>::const_iterator it = udata.begin();
		it != udata.end();
		++it)
	{
		const char * date_last_logged_in = it->m_since.c_str();
		const char * date_last_logged_out = it->m_until.c_str();
		int y(-1), M(-1), d(-1), h(-1), m(-1), s(-1);
		if( 6 == sscanf( date_last_logged_in, "[%u-%u-%u %u:%u:%u]", &y, &M, &d, &h, &m, &s))
		{
			CTime whn(y, M, d, h, m, s); // when did last login occur
			if( whn < earliest && "" == date_last_logged_out) // means user is currently active
				earliest = whn;
		}
	}

	// we have now the earliest login
	// but we might limit its range if we have been provided a proper timespan
	if( earliest < earliest_at_most && !avoid_limitation)
		earliest = earliest_at_most;

	return earliest;
}

bool CCoreXmlFile::refreshOneFile( const std::string& p_fname)
{
	if( isSS())
	{
		CComPtr<IVSSItem> one_file;
		if( S_OK != m_vssDatabase->get_VSSItem( CComBSTR( (m_vssPath + "\\" + p_fname).c_str()) , false, &(one_file.p))
			|| one_file == 0)
		{
			// handle error case
			return false;
		}

		ASSERT( one_file);
		return S_OK == one_file->Get( 0, VSSFLAG_TIMEMOD);
	}
	else if( isCC())
	{
		// get latest 
		//getLatestCC();
		return false;
	}
	else if( isSV())
	{
		updateSVN( p_fname);
	}
	return false;
}

std::string CCoreXmlFile::refreshProtectionFile( const std::string& p_username)
{
	std::string fname = ProtectList::getProtListFileName( p_username);
	refreshOneFile( fname);
	return fname;
}

bool CCoreXmlFile::findInFile( const std::string& fname, const std::string& str_gd)
{
	std::ifstream g;
	g.open( fname.c_str(), std::ios_base::in| std::ios_base::binary);
	if( !g.is_open()) return false;
	char buff[1024];
	const char * to_find = str_gd.c_str();// why won't work this? (" gd=\"" + str_gd + "\"").c_str();
	const char * to_fin2 = " oper=\"";
	char oper_buff[100];
	bool found = false;
	while( !g.eof() && !found)
	{
		g.getline( &buff[0], 1024, '\n');
		if( strlen( buff) < 1023)
		{
			//if( g.rdstate() != 0)
			//	g.clear();

			found = 0 != strstr( buff, to_find);
			if( found)
			{
				char * oper = strstr( buff, to_fin2);
				if( strlen(oper) < 100)
					sscanf( oper, " oper=\"%s\" ", &oper_buff);
			}
		}
		else
			ASSERT(0);
	}
	g.close();

	return found;
}

bool CCoreXmlFile::makeSureFileExistsInVerSys( const std::string& p_fname, const std::string& p_initialcontent, bool p_needsLock /*= true*/)
{
	std::string fulllocalfname = m_folderPath + "\\" + p_fname;
	bool found = false;

	try
	{
		if( isSS())
		{
			CComObjPtr<IVSSItem> it;
			found = S_OK == m_vssDatabase->get_VSSItem( CComBSTR( (m_vssPath + "\\" + p_fname).c_str()) , false, &(it.p));
		}
		else if( isCC())
		{
			try {
				bool ischdout = isFileCheckedOutCC( fulllocalfname.c_str());
				if( ischdout)
				{
					int l = 0;
					++l;
				}
				checkOutFileCC( fulllocalfname.c_str());
				found = true;
			} catch(...) {
				found = false;
			}
		}
		else if( isSV())
		{
			found = FileHelp::fileExist( fulllocalfname) && isVersionedInSVN( fulllocalfname);
		}

		if( !found)
		{
			FILE * f = fopen( fulllocalfname.c_str(), "w");
			if( !f) throw hresult_exception( -1);

			fprintf( f, "%s", p_initialcontent.c_str());
			fclose( f);

			// add newly created file
			if( isSS())
			{
				// acquire project handle
				CComObjPtr<IVSSItem> projectItem;
				COMTHROW( m_vssDatabase->get_VSSItem( CComBSTR( m_vssPath.c_str()), false, &(projectItem.p)) );

				// add a new element to the VSS
				CComObjPtr<IVSSItem> it;
				COMTHROW( projectItem->Add( CComBSTR( fulllocalfname.c_str()), L"", 0, &(it.p)));
			}
			else if( isCC())
			{
				checkOutFileCC( m_folderPath.c_str() ); // this is needed when adding new files, but is it's status somehow already checked out?
				addFileToCC( fulllocalfname.c_str());
			}
			else if( isSV())
			{
				//bool ok = isVersionedInSVN( fulllocalfname);
				bool sc_add = true;
				bool sc_pro = true;
				bool sc_com = true;
				sc_add = addSVN( fulllocalfname);
				if( p_needsLock) // apply lock attribute except if directed otherwise
					sc_pro = lockablePropertySVN( fulllocalfname);
				sc_com = commitSVN( fulllocalfname, true);

				if( !( sc_add && sc_pro && sc_com))
				{
					if(      !sc_add) sendMsg( "Could not add file " + fulllocalfname, MSG_ERROR);
					else if( !sc_pro) sendMsg( "Could not apply lockable property for file " + fulllocalfname, MSG_ERROR);
					else if( !sc_com) sendMsg( "Could not commit file " + fulllocalfname, MSG_ERROR);
					throw hresult_exception( -1);
				}
			}

			found = true;
		}
	}
	catch( hresult_exception& e)
	{
		char buff[200]; sprintf( buff, "Could not get \"%s\" file from source control. Exception code: 0x%x", fulllocalfname.c_str(), e.hr);
		sendMsg( buff, MSG_ERROR);
		AfxMessageBox( buff);
		return false;
	}
	return found;
}

bool CCoreXmlFile::getFileHandleSS( const std::string& p_fname, CComObjPtr<IVSSItem>& result_ptr)
{
	try
	{
		if( isSS())
		{
			COMTHROW( m_vssDatabase->get_VSSItem( CComBSTR( (m_vssPath + "\\" + p_fname).c_str()) , false, &(result_ptr.p)));
		}
	}
	catch( hresult_exception& e)
	{
		char buff[200]; sprintf( buff, "Could not get file from source control. Exception code: 0x%x", e.hr);
		sendMsg( buff, MSG_ERROR);
		AfxMessageBox( buff);
		return false;
	}
	return true;
}

void CCoreXmlFile::getLatestVersion()
{
	try
	{
		if( isSS())
		{
			CComObjPtr<IVSSItem> projectItem;
			CComBSTR path = m_vssPath.c_str();
			COMTHROW( m_vssDatabase->get_VSSItem( path, false, &(projectItem.p)) );
			//COMTHROW( projectItem->Get( NULL, VSSFLAG_TIMEMOD ) ); // previously was 0 // VSSFLAG_TIMEMOD, VSSFLAG_TIMENOW (default), or VSSFLAG_TIMEUPD
			CComBSTR loca = m_folderPath.c_str();
			COMTHROW( projectItem->Get( &loca, VSSFLAG_TIMEMOD ) ); // previously was 0 // VSSFLAG_TIMEMOD, VSSFLAG_TIMENOW (default), or VSSFLAG_TIMEUPD
		}
		else if( isCC())
		{
			// if we use dynamic views we do not need it
			//getLatestVerCC( m_folderPath.c_str() );
		}
		else if( isSV())
		{
	//		//getLatestSVN( m_folderPath);
			if( m_svnByAPI) {
				//m_svn->getLatest( m_folderPath);
				COMTHROW( m_comSvn->GetLatest( CComBSTR( m_folderPath.c_str())));
			}
			else          m_cmdSvn->getLatest( m_folderPath);
		}
	}
	catch( hresult_exception& )
	{
		AfxMessageBox( "Could not get latest version from server!");
		HR_THROW( E_UNKNOWN_STORAGE); // furthermore will be silently handled
	}
}

void CCoreXmlFile::checkInAll()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	if( !m_userOpts.m_defCheckInOnSave && AfxMessageBox( "Project saved. Keep modified files checked out, thus hold locks explicitly?", MB_YESNO ) == IDYES ) // easier question???
		checkInAll( true ); // keep files checked out
	else
	{
		if( m_userOpts.m_defCheckInOnSave)
			sendMsg( std::string( "ACCELERATION: No file is left checked out based on policy configured."), MSG_INFO);

		checkInAll( false ); // check in
	}
}

void CCoreXmlFile::checkInAll( bool keepCheckedOut )
{
	if( isSS())
	{
		// source safe
		CComObjPtr<IVSSItem> projectItem;
		CComBSTR path = m_vssPath.c_str();
		COMTHROW( m_vssDatabase->get_VSSItem( path, false, &(projectItem.p)) );
		if( keepCheckedOut )
			COMTHROW( projectItem->Checkin( NULL, NULL, VSSFLAG_KEEPYES ) );
		else
			COMTHROW( projectItem->Checkin( NULL, NULL, 0 ) );
	}
	else if( isCC())
	{
		// clear case

		//// get windows (and clearcase) username
		//char userName[200];
		//unsigned long userNameSize = 200;
		//GetUserName( userName, &userNameSize );

		// get list of checked out files
		CComObjPtr<ICCCheckedOutFileQuery> checkedOutFileQuery;

		COMTHROW( m_clearCase->CreateCheckedOutFileQuery( PutOut(checkedOutFileQuery) ));

		VARIANT v;
		v.vt = VT_ARRAY | VT_BSTR;
		SAFEARRAYBOUND rgsabound[1];  //Denotes number of dimensions
		rgsabound[0].lLbound   = 0;
		rgsabound[0].cElements = 1;
		v.parray = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
		long count = 0;

		CComVariant name = m_folderPath.c_str();
		COMTHROW( SafeArrayPutElement(v.parray, &count, (void *) &name) );

		// get list of checked out files
		CComBSTR user = userName().c_str();
		COMTHROW( checkedOutFileQuery->put_PathArray( v ) );
		COMTHROW( checkedOutFileQuery->put_User(user) );
		CComObjPtr<ICCCheckedOutFiles> files;
		COMTHROW( checkedOutFileQuery->Apply( PutOut(files) ));

		// checkin files
		long fileCount;
		COMTHROW( files->get_Count( &fileCount ) );
		for( int i=0; i<fileCount; ++i )
		{
			CComObjPtr<ICCVersion>    version;
			CComBSTR                  comment = "";

			CComObjPtr<ICCCheckedOutFile> checkedOutFile;
			COMTHROW( files->get_Item( i+1, PutOut(checkedOutFile) ) );
			checkedOutFile->CheckIn( comment, 1, comment, ccKeep, PutOut(version) );

			CComBSTR path;
			checkedOutFile->get_Path(&path);
			char buf[MAX_PATH];
			sprintf(buf, "%S", path);

			// check out if keepcheck out is true
			if( keepCheckedOut )
			{            
				checkOutFileCC(buf);
			}

			// sync time 
			if( strlen(buf) >= m_folderPath.length() + 1 && strncmp( buf, m_folderPath.c_str(), m_folderPath.length()) == 0)
			{
				std::string str_gd;
				str_gd = &buf[ m_folderPath.length() + 1]; // 1 for the '\'
				int pos = str_gd.rfind( ".xml");
				if( pos != std::string::npos)
				{
					str_gd = str_gd.substr( 0, pos);
					GUID gd = str2guid( str_gd.c_str());

					GUIDToXmlObjectMapIter it2 = m_objectsByGUID.find( gd );
					if( it2 != m_objectsByGUID.end() )
					{
						timeSync( buf, it2->second );
					}
				}
			}
		}
	}
	else if( isSV())
	{// sometimes there is nothing to commit here, let's try for a while to not execute this
		// if keepCheckedOut => use --no-unlock: won't unlock the targets
		//bool sc_com = commitSVN( m_folderPath, true, keepCheckedOut);
		//if( !sc_com)
		{
			//sendMsg( "Nothing committed or could not commit all in directory " + m_folderPath, MSG_WARNING);
		}
		if( m_userOpts.m_useBulkCommit)
		{
			bool sc_com = bulkCommitSVN( m_folderPath, keepCheckedOut);
			if( !sc_com)
			{
				sendMsg( "Nothing committed or could not commit all in directory " + m_folderPath, MSG_WARNING);
			}
		}
	}
}

bool CCoreXmlFile::createClearCaseApp()
{
	try
	{
		if( m_clearCase.p == NULL )
		{
			COMTHROW( m_clearCase.CoCreateInstance( CLSID_Application ) );
			COMTHROW( m_clearTool.CoCreateInstance( CLSID_ClearTool ) );
		}
		return true;
	}
	catch(...)
	{
		//AfxMessageBox( "Cannot connect to ClearCase. Check the ClearCase installation!" );
		return false;
	}
}

void CCoreXmlFile::createClearCaseProj()
{
	m_sourceControl = SC_CLEARCASE;

	// check if the folder under ClearCase source control
	bool is_app = createClearCaseApp();
	if( !is_app) {
		// since the user intends to create a ClearCase project we must notify that: 
		AfxMessageBox( "Cannot connect to ClearCase. Check the ClearCase installation!", MB_ICONSTOP);
		HR_THROW( E_FILEOPEN);
	}

	bool is_under_cc = isPathUnderClearCase( m_parentFolderPath.c_str());
	if( !is_under_cc) {
		AfxMessageBox( "This given project folder is not under ClearCase source control!" );
		HR_THROW( E_FILEOPEN);
	}

	try {
		// create and add project folder to source control
		checkOutFileCC( m_parentFolderPath.c_str() );   // maybe this not needed if we keep it checked out
		addDirToCC( m_folderPath.c_str() );
	} catch(...) {
		AfxMessageBox("ClearCase error! Cannot add project to ClearCase. Errocode=1");
		throw;
	}

	if( m_hashFileNames)
	{
		try {
			// create contents folder
			addDirToCC( m_contentPath.c_str());
			checkOutFileCC( m_contentPath.c_str());

			// create hashed folders
			DirSupplier     ds( m_hashFileNames, m_hashVal);
			if( m_hashVal == 2)
			{
				for( Dir256Iterator it = ds.begin256(); it != ds.end256(); ++it)
					addDirToCC( (m_contentPath + "\\" + *it).c_str());
			}
			else if( m_hashVal == 5)
			{
				for( Dir16Iterator it = ds.begin16(); it != ds.end16(); ++it)
				{
					addDirToCC( (m_contentPath + "\\" + *it).c_str());
					checkOutFileCC( (m_contentPath + "\\" + *it).c_str());

					for( Dir256Iterator jt = ds.begin256(); jt != ds.end256(); ++jt)
						addDirToCC( (m_contentPath + "\\" + *it + "\\" + *jt).c_str());
				}
			}
		} catch(...) {
			sendMsg( "Exception: Could not create initial directory structure!", MSG_ERROR);
			AfxMessageBox( "Could not create initial directory structure.");
			HR_THROW(E_FILEOPEN);
		}
	} // m_hashFileNames
}

bool CCoreXmlFile::isPathUnderClearCase( const char * path )
{   
	ASSERT( m_clearCase.p != NULL );

	CComObjPtr<ICCVersion> version;

	CComBSTR path2 = path;
	VARIANT  path3;

	path3.bstrVal = path2.m_str;
	path3.vt      = VT_BSTR;

	HRESULT hres = m_clearCase->get_Version( path3, PutOut(version) );

	return (hres==S_OK);
}

bool CCoreXmlFile::isFileCheckedOutCC( const char * path )
{
	CComObjPtr<ICCVersion>        version;
	CComBSTR                      path1 = path;
	VARIANT                       path2;
	//HRESULT                       hres;
	VARIANT_BOOL                  isCheckdOut = 0;

	path2.vt      = VT_BSTR;
	path2.bstrVal = path1;

	COMTHROW( m_clearCase->get_Version( path2, PutOut(version) ) );
	COMTHROW( version->get_IsCheckedOut( &isCheckdOut ) );

	return (isCheckdOut!=0);
}

int CCoreXmlFile::getCheckOutStateCC( const char * path )
{
	// get windows (and clearcase) username
	//char userName[200];
	//unsigned long userNameSize = 200;
	//GetUserName( userName, &userNameSize );

	// get list of checked out files by current user
	CComObjPtr<ICCCheckedOutFileQuery> checkedOutFileQuery;

	COMTHROW( m_clearCase->CreateCheckedOutFileQuery( PutOut(checkedOutFileQuery) ));

	VARIANT v;
	v.vt = VT_ARRAY | VT_BSTR;
	SAFEARRAYBOUND rgsabound[1];  //Denotes number of dimensions
	rgsabound[0].lLbound   = 0;
	rgsabound[0].cElements = 1;
	v.parray = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
	long count = 0;

	CComVariant name = m_folderPath.c_str();
	COMTHROW( SafeArrayPutElement(v.parray, &count, (void *) &name) );

	CComBSTR user = userName().c_str();
	COMTHROW( checkedOutFileQuery->put_PathArray( v ) );
	COMTHROW( checkedOutFileQuery->put_User(user) );
	CComObjPtr<ICCCheckedOutFiles> files;
	COMTHROW( checkedOutFileQuery->Apply( PutOut(files) ));

	long fileCount;
	COMTHROW( files->get_Count( &fileCount ) );
	for( int i=0; i<fileCount; ++i )
	{
		CComObjPtr<ICCVersion>    version;
		CComBSTR                  comment = "";

		CComObjPtr<ICCCheckedOutFile> checkedOutFile;
		COMTHROW( files->get_Item( i+1, PutOut(checkedOutFile) ) );

		CComBSTR path2;
		checkedOutFile->get_Path(&path2);
		char path3[MAX_PATH];
		sprintf(path3, "%S", path2);

		if( stricmp(path, path3) == 0 )
		{
			return CS_CURRENT_USER;
		}
	}

	if( isFileCheckedOutCC( path ) )
		return CS_OTHER_USER;

	return CS_NOT_CHECKEDOUT;
}

void CCoreXmlFile::checkOutFileCC( const char * path )
{
	CComObjPtr<ICCVersion>        version;
	CComObjPtr<ICCCheckedOutFile> checkedOutFile;
	CComBSTR                      path1 = path;
	CComBSTR                      comment = "";
	VARIANT                       path2;
	//HRESULT                       hres;

	path2.vt      = VT_BSTR;
	path2.bstrVal = path1;

	//COMTHROW( m_clearCase->get_Version( path2, PutOut(version) ) );
	HRESULT kr = m_clearCase->get_Version( path2, PutOut(version) );
	if( FAILED( kr)) throw hresult_exception( kr);
	version->CheckOut( ccTryReserved, comment, 0, ccVersion_Default, 0, 0, PutOut(checkedOutFile) );
	//COMTHROW( version->CheckOut( ccReserved, comment, 0, ccVersion_Default, 0, 0, PutOut(checkedOutFile) ) );
}

void CCoreXmlFile::checkInFileCC( const char * path )
{
	CComObjPtr<ICCVersion>        version;
	CComObjPtr<ICCCheckedOutFile> checkedOutFile;
	CComBSTR                      path1 = path;
	CComBSTR                      comment = "";
	CComBSTR                      empty = "";

	if( m_clearCase->get_CheckedOutFile( path1, PutOut(checkedOutFile) ) == S_OK )
	{
		if( checkedOutFile.p != NULL )
		{
			HRESULT hres = checkedOutFile->CheckIn( comment, 1, empty, ccKeep, PutOut(version) );
		}
	}
}

void CCoreXmlFile::addFileToCC( const char * path )
{
	// parent folder must be checked out!
	CComObjPtr<ICCCheckedOutFile> checkedOutFile;

	CComBSTR fileName         = path;
	CComBSTR comment          = "";
	CComBSTR elementTypeName1 = "text_file";

	VARIANT elmentTypeName;
	elmentTypeName.vt = VT_BSTR;        
	elmentTypeName.bstrVal = elementTypeName1;

	m_clearCase->CreateElement(fileName,comment,1,elmentTypeName,PutOut(checkedOutFile)); 
}

void CCoreXmlFile::addDirToCC( const char * path )
{
	// parent folder must be checked out!
	CComObjPtr<ICCCheckedOutFile> checkedOutFile;

	CComBSTR fileName         = path;
	CComBSTR comment          = "";
	CComBSTR elementTypeName1 = "directory";

	VARIANT elmentTypeName;
	elmentTypeName.vt = VT_BSTR;        
	elmentTypeName.bstrVal = elementTypeName1;

	m_clearCase->CreateElement(fileName, comment,1,elmentTypeName,PutOut(checkedOutFile));
}

void CCoreXmlFile::getLatestVerCC( const char * path )
{
	std::string cmd = "update ";
	cmd += path;
	CComBSTR cmd2 = cmd.c_str();
	BSTR output;
	COMTHROW( m_clearTool->CmdExec( cmd2, &output ) );
}

void CCoreXmlFile::createNonversioned()
{
	int  succ = CreateDirectory( m_folderPath.c_str(), NULL);

	if( succ)
		succ = createHashedFolders();

	if( !succ)
	{
		sendMsg( "Exception: Could not create initial directory structure!", MSG_ERROR);
		AfxMessageBox( "Could not create initial directory structure");
		HR_THROW(E_FILEOPEN);
	}
}

int CCoreXmlFile::createHashedFolders()
{
	if( !m_hashFileNames) return 1;

	int   succ = CreateDirectory( m_contentPath.c_str(), NULL);

	DirSupplier     ds( m_hashFileNames, m_hashVal);
	if( m_hashVal == 2)
	{
		for( Dir256Iterator it = ds.begin256(); succ && it != ds.end256(); ++it)
			succ = CreateDirectory( (m_contentPath + "\\" + *it).c_str(), NULL);
	}
	else if( m_hashVal == 5)
	{
		for( Dir16Iterator it = ds.begin16(); succ && it != ds.end16(); ++it)
		{
			succ = CreateDirectory( (m_contentPath + "\\" + *it).c_str(), NULL);
			for( Dir256Iterator jt = ds.begin256(); succ && jt != ds.end256(); ++jt)
				succ = CreateDirectory( (m_contentPath + "\\" + *it + "\\" + *jt).c_str(), NULL);
		}
	}

	return succ;
}

void CCoreXmlFile::commitHashedFolders()
{
	if( !m_hashFileNames) return;

	if( m_hashVal == 5)
	{
		// we will create 4096 dirs like 0/00, 0/01, ... 0/ff, 1/00, 1/01, ... f/ff
		const char ans[] = "0123456789abcdef";
		char       lev1[2] = { 'x', 0};
		char       lev2[5] = { 'y', '\\', 'z', 't', 0};
		int        err     = 0;

		for( short i = 0; !err && i != 16; ++i)
		{
			lev1[0] = ans[i]; // form a name 
			socoAdd( lev1, true /*=recursive*/); 
			socoCommit( lev1, true);
		}
	}
	else if( m_hashVal == 2)
	{
		// we will create 256 dirs like 00, 01, ..ff
		const char ans[] = "0123456789abcdef";
		char       lev[3] = { 'z', 't', 0};
		int        err     = 0;


		// add and commit
		for( short i = 0; !err && i != 16; ++i)
		{
			lev[0] = ans[i]; // form a name 
			for( short j = 0; !err && j != 16; ++j)
			{
				lev[1] = ans[j];
				socoAdd( lev, true /*=recursive*/); 
				socoCommit( lev, true);
			}
		}
	}
}

void CCoreXmlFile::socoAdd     ( const std::string& p_path, bool p_recursive)
{
	if( isSS())
	{
		//addDirToSS(
	}
	else if( isCC())
	{
		addDirToCC( p_path.c_str());//, p_recursive);
	}
	else if( isSV())
	{
		addSVN( p_path, p_recursive);
	}
	else
	{
		// nop
	}
}

void CCoreXmlFile::socoCommit( const std::string& p_path, bool p_initial)
{
	if( isSS())
	{
	}
	else if( isCC())
	{
		checkInFileCC( p_path.c_str());
	}
	else if( isSV())
	{
		bool sc = commitSVN( p_path, p_initial /*initial commit*/);
		if( !sc)
		{
			sendMsg( "Could not commit " + p_path + " into versioning system.", MSG_ERROR);
		}
	}
	else
	{
		// nop
	}
}


void CCoreXmlFile::testSubversion()
{
	//if( IDNO == AfxMessageBox( "Would you like to skip the test of svn connection?", MB_YESNO))
	//{
	//	std::string tst, last_author, curr_owner;
	//	bool sc = infoSVN( m_svnUrl, false /*no recursive*/, tst, last_author, std::string() /* =curr_owner*/);
	//	if( sc)
	//	{
	//		AfxMessageBox( tst.c_str());
	//		if( last_author != m_vssUser)
	//			AfxMessageBox( (std::string( "There is a username confusion here!\n'") + last_author + "' is told to be the last modifier of " + m_svnUrl + ", while the credentials provided indicate '" + m_vssUser + "'!").c_str());
	//	}
	//	else
	//		AfxMessageBox( "svn info command failed");
	//}
}

void CCoreXmlFile::createSubversionClientImpl()
{
	if( m_svnByAPI)
	{
		HRESULT hr = m_comSvn.CoCreateInstance( CLSID_SvnExec); // or "Mga.XmlBackEnd.SvnExec"
		if( SUCCEEDED( hr) && m_comSvn)
			hr = m_comSvn->Init( CComBSTR( m_vssUser.c_str()), CComBSTR( m_vssPassword.c_str()));
		if( FAILED( hr) || !m_comSvn) {
			if( hr == E_NOTIMPL) AfxMessageBox( "SVN client library is not implemented!"); // look for the #if(USESVN) macro
			else                 AfxMessageBox( "SVN implementation client object could not be created!");
			throw -1;
		}

		hr = m_comSvn->Logging( m_userOpts.m_createSvnLog? VARIANT_TRUE: VARIANT_FALSE, CComBSTR( m_userOpts.m_svnLogFileName.c_str()));
	}
	else
	{
		m_cmdSvn = new CmdClient( m_svnShowCmdLineWindows, m_svnRedirectOutput); 
	}
}

void CCoreXmlFile::svnSetup( bool p_createOrOpen)
{
	if( getUserCredentialInfo( 1, p_createOrOpen)) // fills m_vssUser, m_vssPassword, p_createOrOpen == requireLogin
	{
		svnSshHandling();
		svnOptions();
		createSubversionClientImpl();
	}
	else
	{
		AfxMessageBox( "You did not provide login data. You may work locally but might lose synchronization with the version controlled project. You won't be able to modify read-only files.", MB_ICONEXCLAMATION );
		m_sourceControl = SC_NONE;
	}
	//testSubversion();
}

void CCoreXmlFile::createSubversionedFolder()
{
	m_sourceControl = SC_SUBVERSION;
	//m_hashInfoFound = false;

	svnSetup( true); // true => strictly requires login data, throws if dlg is canceled
	// fills m_vssUser, m_vssPassword

	if( !isVersionedInSVN( m_svnUrl, /*isDir = */true))
	{
		sendMsg( "Exception: Location directory '" + m_svnUrl + "' does not exist on the SVN server!", MSG_ERROR);
		AfxMessageBox( (std::string( "Location directory '") + m_svnUrl + "' does not exist on the SVN server").c_str());
		HR_THROW(E_FILEOPEN);
	}
#if(USESERF) // serf does not like the test below
#else
	if( isVersionedInSVN( m_svnUrl + "/" + m_projectName, /*isDir = */true, /* suppressErrorMsg = */true))
	{
		sendMsg( "Exception: Project '" + m_projectName + "' already found at '" + m_svnUrl + "'. Project creation aborted.", MSG_ERROR);
		AfxMessageBox( (std::string( "Project '") + m_projectName + "' already found at '" + m_svnUrl + "'. Project creation aborted.").c_str());
		HR_THROW(E_FILEOPEN);
	}
#endif
	bool main_created = mkdirSVN( m_svnUrl, m_projectName, m_folderPath);
	if( !main_created)
	{
		sendMsg( "Exception: Could not create on server directory: " + m_svnUrl + "/" + m_projectName, MSG_ERROR);
		AfxMessageBox( (std::string( "Could not create on server directory: ") + m_svnUrl + "/" + m_projectName).c_str());
		HR_THROW(E_FILEOPEN);
	}

	chdir( m_folderPath.c_str()); // change to the newly created local dir

	if( m_hashFileNames)
	{
		// creation succeeded?
		int succ = createHashedFolders();
		if( !succ) {
			sendMsg( "Exception: Could not create initial directory structure.", MSG_ERROR);
			AfxMessageBox( "Could not create initial directory structure.");
			HR_THROW(E_FILEOPEN);
		}

		// add to server
		succ = addSVN( m_contentPath, true /*=recursive*/); 
		if( !succ) {
			sendMsg( "Exception: Could not add initial directory structure to server.", MSG_ERROR);
			AfxMessageBox( "Could not add initial directory structure to server.");
			HR_THROW(E_FILEOPEN);
		}

		// initial commit
		succ = commitSVN( m_contentPath, true);
		if( !succ) {
			sendMsg( "Exception: Could not commit initial directory structure.", MSG_ERROR);
			AfxMessageBox( "Could not commit initial directory structure.");
			HR_THROW(E_FILEOPEN);
		}
	}
}

void CCoreXmlFile::getSVLastCommiter(XmlObject * obj, string& user)
{
	ASSERT( m_sourceControl == SC_SUBVERSION );
	ASSERT( obj->isContainer() );
	ASSERT( m_svnByAPI);

	std::string fname;
	getContainerFileName( obj, fname, true);

	bool ret;
	if( m_svnByAPI) {
		//ret = m_svn->info( fname, false /*recursive*/, false /*info msg*/, std::string(), user /* = author*/, std::string() /* = holder*/);
		CComBSTR buser;
		COMTHROW( m_comSvn->Info( CComBSTR( fname.c_str()), VARIANT_FALSE /*rec?*/, VARIANT_FALSE /*infomessage?*/, &CComBSTR(), &buser, &CComBSTR()));
		CopyTo( buser, user);
	}
	else         ret = m_cmdSvn->info( fname, false /*recursive*/, false /*info msg*/, std::string(), user /* = author*/, std::string() /* = holder*/);

}

void CCoreXmlFile::getSVCurrentOwner(XmlObject * obj, string& user, bool& newfile) // getSVCheckOutUser
{
	ASSERT( m_sourceControl == SC_SUBVERSION );
	ASSERT( obj->isContainer() );
	ASSERT( m_svnByAPI);

	std::string fname;
	getContainerFileName( obj, fname, true);

	std::string holder;
	bool        ret = false;

	if( m_svnByAPI) {
		//ret = m_svn->isLockedByUser( fname, holder);
		VARIANT_BOOL is_verd;
		VARIANT_BOOL vb_locked;
		CComBSTR     bs_holder;
		COMTHROW( m_comSvn->IsVersioned( CComBSTR( fname.c_str()), VARIANT_FALSE /*isdir*/, VARIANT_TRUE /*suppress error*/, &is_verd)); 
		if( is_verd == VARIANT_TRUE)
		{
			newfile = false;
			COMTHROW( m_comSvn->IsLocked( CComBSTR( fname.c_str()), &vb_locked, &bs_holder));
			CopyTo( bs_holder, holder);
			ret = vb_locked == VARIANT_TRUE;
		}
		else
			newfile = true;
	}
	else
	{
		bool is_verd = m_cmdSvn->isVersioned( fname, false, true);
		if( is_verd)
		{
			newfile = false;
			ret = m_cmdSvn->isLockedByUser( fname, holder);
		}
		else
			newfile = true;
	}

	if( ret)
	{
		user    = holder;
	}

	long lInfo( 0x0), lStat( 0x0);
	if( !holder.empty())
	{
		if( holder != m_vssUser)
			lInfo = FS_OTHER; // checked out by other user
		else // readonly, but we could apply a lock onto it (then we released the lock)
			lInfo = FS_LOCAL; // checked out by local user
	}
	else
	{
		lInfo = 0x0;  // not checked out
	}


	bool ismodbyothers = fileModifiedByOthers( obj); // fileModifiedByOthers( fname, obj->m_lastWriteTime);

	if( ismodbyothers)
		lStat = FS_MODIFIEDBYOTHERS;
	else if( newfile)
		lStat = FS_NOTYETSAVED;
	else
		lStat = 0x0;

	setSourceControlNodes( obj, lInfo, lStat);
}

bool CCoreXmlFile::isCheckedOutByElseSVN( const std::string& p_file)
{
	if( m_svnByAPI)
	{
		VARIANT_BOOL vb_versioned;
		COMTHROW( m_comSvn->IsVersioned( CComBSTR( p_file.c_str()), VARIANT_FALSE /*isDir?*/, VARIANT_FALSE /*suppress?*/, &vb_versioned));
		ASSERT( vb_versioned == VARIANT_TRUE);
		//ASSERT( m_svn->isVersioned( p_file));
		VARIANT_BOOL vb_locked;
		CComBSTR bs_holder;
		COMTHROW( m_comSvn->IsLocked( CComBSTR( p_file.c_str()), &vb_locked, &bs_holder));
		if( vb_locked == VARIANT_TRUE)
		{
			std::string holder;
			CopyTo( bs_holder, holder);
			return holder != m_vssUser; // not us
		}
		return false;
		//return m_svn->isLockedByOthers( p_file);
	}
	else
	{
		return m_cmdSvn->isCheckedOutByElse( p_file);
	}
}

//void CCoreXmlFile::checkOutSVN( const std::string& p_file)
//{
//	// todo simplify, streamline this
//	if( applyLockSVN( p_file)) 
//	{
//		// not checked out by somebody else
//	}
//	else
//	{
//		ASSERT( 0);
//		throw "checked out file";
//	}
//}

bool CCoreXmlFile::applyLockSVN( const std::string& p_file) // throws
{
	bool succ = false;

	if( m_svnByAPI) { 
		//succ = m_svn->tryLock( p_file);
		VARIANT_BOOL vb_succ;
		COMTHROW( m_comSvn->TryLock( CComBSTR( p_file.c_str()), &vb_succ));
		succ = vb_succ == VARIANT_TRUE;
	}
	else         succ = m_cmdSvn->tryLock( p_file);

	if( !succ)
	{
		AfxMessageBox( (p_file + " lock() returned false in applyLockSVN").c_str());
		HR_THROW( E_FAIL);
	}

	return succ;
}

bool CCoreXmlFile::removeLockSVN( const std::string& p_file)
{
	if( m_svnByAPI) { 
		//return m_svn->unLock( p_file);
		VARIANT_BOOL vb_succ;
		COMTHROW( m_comSvn->UnLock( CComBSTR( p_file.c_str()), &vb_succ));
		return vb_succ == VARIANT_TRUE;
	}
	else         return m_cmdSvn->unLock( p_file);
}

bool CCoreXmlFile::mkdirSVN( const std::string& p_url, const std::string& p_dirName, const std::string& p_localDestPath)
{
	std::string dir = p_url + "/" + p_dirName;

	if( m_svnByAPI)
	{
		//bool sc = m_svn->mkDirOnServer( dir);
		//if( !sc) return false;
		HRESULT hr_m = m_comSvn->SrvMkDir( CComBSTR( dir.c_str()));
		if( FAILED( hr_m))
			return false;

		//getLatestSVN( 
		//sc = m_svn->lightCheckOut( dir, p_localDestPath);
		//return sc;
		HRESULT hr_c = m_comSvn->LightCheckOut( CComBSTR( dir.c_str()), CComBSTR( p_localDestPath.c_str()));
		return SUCCEEDED( hr_c);
	}
	else
	{
		bool sc = m_cmdSvn->mkdirOnServer( dir); //WAS: return m_cmdSvn->mkdirWithUpdate( p_url, p_localDestPath, p_dirName);
		if( !sc) return false;

		sc = m_cmdSvn->lightCheckOut( dir, p_localDestPath);

		// change to the local directory after creation and checkout
		int r;
		r = _chdir( p_localDestPath.c_str());

		return sc && !r;
	}
}

bool CCoreXmlFile::lockablePropertySVN( const std::string& p_file)
{
	if( m_svnByAPI) { 
		//return m_svn->lockableProp( p_file);
		HRESULT hr = m_comSvn->AddLockableProperty( CComBSTR( p_file.c_str()));
		return SUCCEEDED( hr);
	}
	else         return m_cmdSvn->lockableProp( p_file);
}

bool CCoreXmlFile::addSVN( const std::string& p_entity, bool p_recursive /*= false*/)
{
	if( m_svnByAPI) {
		//return m_svn->add( p_entity, p_recursive);
		HRESULT hr = m_comSvn->Add( CComBSTR( p_entity.c_str()), p_recursive?VARIANT_TRUE : VARIANT_FALSE);
		return SUCCEEDED( hr);
	}
	else         return m_cmdSvn->add( p_entity, p_recursive);
}

//
// IDL: UseTheseStrings( [in] short size, [in, out, size_is(size)] BSTR names[]);

HRESULT UseTheseStrings( short size, BSTR names[])
{
	for (int i = 0; i < size; ++i) {
		CW2A name(names[i]);
		MessageBox(NULL, name, "Msg", MB_OK);
	}
	return S_OK;
}

void fillUpVariantArray( const std::vector< std::string>& p_files, VARIANT *pStrings)
{
	if( p_files.size() == 0) return;

	VariantInit( pStrings);
	pStrings->vt = VT_ARRAY | VT_BSTR;
	SAFEARRAY *pSA;
	SAFEARRAYBOUND bounds;
	bounds.lLbound = 0;
	bounds.cElements = p_files.size();

	// create an array (client will free with SafeArrayDestroy())
	pSA = SafeArrayCreate( VT_BSTR, 1, &bounds);

	BSTR *theStrings;
	SafeArrayAccessData( pSA, (void**) &theStrings);
	for( std::vector< std::string>::size_type i = 0; i < p_files.size(); ++i)
	{
		theStrings[i] = CComBSTR( p_files[i].c_str());
	}

	SafeArrayUnaccessData( pSA);

	// set the ret value
	pStrings->parray = pSA;
}

void retAnArrayOfStrings( VARIANT *pStrings)
{
	// init and set the type of variant
	VariantInit( pStrings);
	pStrings->vt = VT_ARRAY | VT_BSTR;
	int nCount = 5;
	SAFEARRAY *pSA;
	SAFEARRAYBOUND bounds = { nCount, 0 };

	// create an array (client will free with SafeArrayDestroy())
	pSA = SafeArrayCreate( VT_BSTR, 1, &bounds);

	BSTR *theStrings;
	SafeArrayAccessData( pSA, (void**)& theStrings);
	theStrings[0] = SysAllocString( L"");
	theStrings[1] = SysAllocString( L"");

	SafeArrayUnaccessData( pSA);

	// set the ret value
	pStrings->parray = pSA;
}

void useArrayOfStrings( VARIANT strings)
{
	if(( strings.vt & VT_ARRAY) && (strings.vt & VT_BSTR))
	{
		// grab the array
		SAFEARRAY *pSA = strings.parray;
		BSTR *bstrArray;

		// lock it down
		SafeArrayAccessData( pSA, (void**)&bstrArray);

		// read each item
		for( ULONG i = 0; i < pSA->rgsabound->cElements; ++i)
		{
			CComBSTR temp = bstrArray[i];
		}

		// unlcok
		SafeArrayUnaccessData( pSA);
		SafeArrayDestroy( pSA);
	}
}

void CCoreXmlFile::findAllRwObjs( const std::string& p_folderPath, std::vector< std::string>& rw_file_vec)
{
	chdir( m_folderPath.c_str()); // change to the local main dir (a file dialog may have changed the local dir)

	for( XmlObjVecIter it=m_objects.begin(); it!=m_objects.end(); ++it )
	{
		XmlObject * obj = (*it);
		//if( obj->isContainer() && obj->m_loaded )
		//	writeXMLFile( obj );
		if( !obj->isContainer() )
			continue;

		std::string fileName;
		getContainerFileName( obj, fileName);

		// we wish we could predict the time the file will be closed
		// because that will become the file's 'Modified At' attribute
		// the obj->m_lastWriteTime needs to reflect exactly this time
		bool f_existed = false;
		if( FileHelp::isFileReadOnly2( fileName, &f_existed))
		{
			continue; // file exists, is read-only, no chance of writing into it
					// it also means there was no change
		}

		rw_file_vec.push_back( fileName);
	}
}

bool CCoreXmlFile::bulkCommitSVN( const std::string& p_dir, bool p_noUnlock /* = false*/) // noUnlock <==> keeplocked
{
	if( m_svnByAPI)
	{
		m_comSvn->Commit( CComBSTR( p_dir.c_str()), p_noUnlock? VARIANT_TRUE: VARIANT_FALSE);
		if( !p_noUnlock) // if noUnlock was not requested, then a file should be unlocked after commit
			// except, when it was not changed: in this case it needs a manual unlock
		{
			// find all 'rw' files, those need to be unlocked
			//std::vector< std::string> rwfiles;
			//findAllRwObjs( p_dir, rwfiles);

			//if( 0 < rwfiles.size())
			//{
			//	VARIANT var_arr;
			//	fillUpVariantArray( rwfiles, &var_arr);

			//	VARIANT_BOOL succ_vt;
			//	m_comSvn->BulkUnLock( var_arr, &succ_vt);
			//}
			// the approach above does not work, because Client::sub_unlock will
			// not unlock several files at once, even though its parameter Target 
			// would allow this.
			//
			// we will unlock files one by one:
			std::string fileName;
			for( XmlObjVecIter it=m_objects.begin(); it!=m_objects.end(); ++it )
			{
				if( !(*it)->isContainer() )
					continue;

				getContainerFileName( *it, fileName);

				bool f_existed = false;
				if( FileHelp::isFileReadOnly2( fileName, &f_existed))
					continue; // file exists, is read-only, means no lock on it

				// unlock one file at a time
				VARIANT_BOOL vb_succ;
				m_comSvn->UnLock( CComBSTR( fileName.c_str()), &vb_succ);
				if( vb_succ != VARIANT_TRUE)
					AfxMessageBox( "Commit/Unlock pair failed");
			}
		}
	}
	else  // cmd line implementation here
	{
		m_cmdSvn->commit( p_dir, false, p_noUnlock);
		if( !p_noUnlock) // if noUnlock was not requested, then a file should be unlocked after commit
			// except, when it was not changed: in this case it needs a manual unlock
		{
			std::vector< std::string> rwfiles;
			findAllRwObjs( p_dir, rwfiles);

			if( 0 < rwfiles.size())
			{
				m_cmdSvn->bulkUnLock( rwfiles);
			}
		}

	}
	return true;
}

bool CCoreXmlFile::commitSVN( const std::string& p_dirOrFile, bool p_initialCommit /* = false*/, bool p_noUnlock /* = false*/) // noUnlock <==> keeplocked
{
	if( m_svnByAPI)
	{
		//bool sc = m_svn->commitAll( p_dirOrFile, p_noUnlock);
		//if( !sc && !p_noUnlock) // if noUnlock was not requested, then a file should be unlocked after commit
			// except, when it was not changed: in this case it needs a manual unlock
		HRESULT hr_c = m_comSvn->Commit( CComBSTR( p_dirOrFile.c_str()), p_noUnlock?VARIANT_TRUE:VARIANT_FALSE);
		if( FAILED( hr_c) && !p_noUnlock) // if noUnlock was not requested, then a file should be unlocked after commit
		{                                 // except, when it was not changed: in this case it needs a manual unlock
			if( FileHelp::isFile( p_dirOrFile) && !FileHelp::isFileReadOnly( p_dirOrFile))
			{
				//sc = m_svn->unLock( p_dirOrFile);
				//if( !sc)
				VARIANT_BOOL vb_succ;
				COMTHROW( m_comSvn->UnLock( CComBSTR( p_dirOrFile.c_str()), &vb_succ));
				if( vb_succ != VARIANT_TRUE)
					AfxMessageBox( "Commit/Unlock pair failed");
				else
					hr_c = S_OK; // reset hr_c to success if unlock succeeded
			}
			//else if( 0) // a dir
			//{
			//	sc = m_svn->unLock( p_dirOrFile); // will it work?
			//	if( sc) AfxMessageBox( "Dir unlock worked");
			//	else    AfxMessageBox( "Dir Commit/Unlock pair failed");
			//}
		}
		//return sc;
		return SUCCEEDED( hr_c);
	}
	else
		return m_cmdSvn->commit( p_dirOrFile, p_initialCommit, p_noUnlock);
}

bool CCoreXmlFile::updateSVN( const std::string& p_dirOrFile)
{
	if( m_svnByAPI) {
		//return m_svn->getLatest( p_dirOrFile);
		HRESULT hr = m_comSvn->GetLatest( CComBSTR( p_dirOrFile.c_str()));
		return SUCCEEDED( hr);
	}
	else         return m_cmdSvn->getLatest( p_dirOrFile);
}

bool CCoreXmlFile::isVersionedInSVN( const std::string& p_file, bool p_isADir /*= false*/, bool p_suppressErrorMsg /*=false*/)
{
	if( m_svnByAPI) {
		//return m_svn->isVersioned( p_file, p_isADir, p_suppressErrorMsg);
		VARIANT_BOOL vb_versioned;
		COMTHROW( m_comSvn->IsVersioned( CComBSTR( p_file.c_str()), p_isADir?VARIANT_TRUE:VARIANT_FALSE, p_suppressErrorMsg?VARIANT_TRUE:VARIANT_FALSE, &vb_versioned));
		return vb_versioned == VARIANT_TRUE;
	}
	else         return m_cmdSvn->isVersioned( p_file, p_isADir, p_suppressErrorMsg);
}

bool CCoreXmlFile::infoSVN( const std::string& p_url, bool p_recursive, std::string& p_resultMsg, std::string& p_author, std::string& p_holder)
{
	if( m_svnByAPI) {
		//return m_svn->info( p_url, p_recursive, true /* = assemble_info_msg*/, p_resultMsg, p_author, p_holder);
		CComBSTR bs_infoResult;
		CComBSTR bs_author;
		CComBSTR bs_holder;
		HRESULT hr = m_comSvn->Info( CComBSTR( p_url.c_str()), p_recursive?VARIANT_TRUE:VARIANT_FALSE, VARIANT_TRUE, &bs_infoResult, &bs_author, &bs_holder);
		if( SUCCEEDED( hr)) {
			CopyTo( bs_infoResult, p_resultMsg);
			CopyTo( bs_author, p_author);
			CopyTo( bs_holder, p_holder);
			return true;
		}
		else return false;
	}
	else         return m_cmdSvn->info( p_url, p_recursive, true /* = assemble_info_msg*/, p_resultMsg, p_author, p_holder);
}

void CCoreXmlFile::showUsedFiles( XmlObjSet& containers, bool p_latentMessage /* = false */ )
{
	if( isSS() || isSV())
	{
		CFilesInUseDetailsDlg dlg( 0, p_latentMessage);
		char buf[300];
		XmlObjSet::iterator it;
		for( it=containers.begin(); it!=containers.end(); ++it )
		{
			string user; bool nfile;
			if( p_latentMessage)                    // info about latently changed files
			{
				if(      isSS()) getSSLastCommiter( *it, user);
				else if( isSV()) getSVLastCommiter( *it, user);
			}
			else                                    // info about owned files
			{
				if(      isSS()) getSSCurrentOwner( *it, user, nfile); // to handle when nfile is true
				else if( isSV()) getSVCurrentOwner( *it, user, nfile); // to handle when nfile is true
			}

			if( user.size() > 0 )
			{
				string name, type;
				getContainerName( *it, name, type );
				sprintf( buf, "%s\t%s (%s)", user.c_str(), name.c_str(), type.c_str() );
				dlg.m_fileList.push_back( buf );
			}
			else dlg.m_fileList.push_back( "A file not yet found in the Versioning System"); // by zolmol
		}
		dlg.DoModal();
	}
	else // = isCC()
	{
		AfxMessageBox( "No detailed information available." );
	}
}

void CCoreXmlFile::setSourceControlNodes( XmlObject * container, long lInfo, long lStat)
{
	ASSERT( container->isContainer() );

	AttribMapIter itfs = container->m_attributes.find( ATTRID_FILESTATUS);
	if( itfs != container->m_attributes.end())
	{
		ASSERT( itfs->second->getType() == VALTYPE_LONG);
		XmlAttrLong * along = (XmlAttrLong*)  itfs->second;
		along->m_value = lInfo + lStat;
	}
}

void CCoreXmlFile::whoControlsThis( XmlObject * container /*= 0*/)
{
	if( !container) return;
	ASSERT( container->isContainer() );

	if( isSS() || isSV())
	{
		std::string user, nm, msg;
		bool newfile = false;

		if(      isSS()) getSSCurrentOwner( container, user, newfile );
		else if( isSV()) getSVCurrentOwner( container, user, newfile ); // does a status info refresh too

		AttribMapIter itnm = container->m_attributes.find( ATTRID_NAME);
		if( itnm != container->m_attributes.end()) 
			itnm->second->toString( nm);
		//nm = makelink( container);

		if( newfile)
			msg = "Container \"" + nm + "\" is not yet saved into the repository.";
		else if( user.empty())
			msg = "Container \"" + nm + "\" is not held exclusively.";
		else
			msg = "Container \"" + nm + "\" is held exclusively by \"" + user + "\"";

		AfxMessageBox( msg.c_str(), MB_ICONINFORMATION);
		//bool ismodbyothers = fileModifiedByOthers( container);
	}
	else if( isCC())
	{
		ASSERT(0);
	}
	else 
		AfxMessageBox( "Container is not held exclusively since the project is not under sourcecontrol.");
}

void CCoreXmlFile::updateSourceControlInfo( XmlObject * container )
{
	ASSERT( container->isContainer() );

	if( isSS())
	{
		string user; bool newfile = false;
		getSSCurrentOwner( container, user, newfile );
		bool ismodbyothers = false;
		try {
			ismodbyothers = fileModifiedByOthers( container); // might throw in VSS case for orphans
		} catch( hresult_exception& )
		{
			ismodbyothers = false;
			newfile = true;
			std::string link = makelink( container);
			sendMsg( std::string( "Orphan found: ") + link, MSG_ERROR);
		}
		long lInfo( 0x0), lStat( 0x0);
		if( user.size() == 0 )
			lInfo = 0x0;  // not checked out
		else if( stricmp( user.c_str(), m_vssUser.c_str() ) == 0 )
			lInfo = FS_LOCAL; // checked out by local user
		else
			lInfo = FS_OTHER; // checked out by other user

		if( ismodbyothers)
			lStat = FS_MODIFIEDBYOTHERS;
		else if( newfile)
			lStat = FS_NOTYETSAVED;
		else
			lStat = 0x0;

		setSourceControlNodes( container, lInfo, lStat);
	}
	else if( isCC())
	{
		string fileName;
		getContainerFileName( container, fileName, true );
		int state = getCheckOutStateCC( fileName.c_str() );

		long lInfo( 0x0), lStat( 0x0);

		//if( ismodbyothers) // what about these?
		//    lStat = FS_MODIFIEDBYOTHERS;
		//else if( newfile)
		//    lStat = FS_NOTYETSAVED;
		//else
		//    lStat = 0x0;

		if( state == CS_NOT_CHECKEDOUT )
			lInfo = 0x0;
		else if( state == CS_CURRENT_USER )
			lInfo = FS_LOCAL;
		else
			lInfo = FS_OTHER;

		setSourceControlNodes( container, lInfo, lStat);
	}
	else if( isSV())
	{
		string file_name;
		getContainerFileName( container, file_name, true );

		bool fexists = FileHelp::fileExist( file_name);

		long lInfo( 0x0), lStat( 0x0);

		if( fexists && FileHelp::isFileReadOnly( file_name))
		{
			if( isCheckedOutByElseSVN( file_name))
			{
				lInfo = FS_OTHER; // checked out by other user
			}
			else // readonly, but we could apply a lock onto it (then we released the lock)
			{
				lInfo = 0x0;  // not checked out
			}
		}
		else if( fexists)
		{
			lInfo = FS_LOCAL; // checked out by local user
		}


		bool newfile = !fexists;
		bool ismodbyothers = fexists && fileModifiedByOthers( container);

		if( ismodbyothers)
			lStat = FS_MODIFIEDBYOTHERS;
		else if( newfile)
			lStat = FS_NOTYETSAVED;
		else
			lStat = 0x0;

		setSourceControlNodes( container, lInfo, lStat);
	}
}

void CCoreXmlFile::updateSourceControlInfo()
{
	for( XmlObjVecIter it=m_objects.begin(), en = m_objects.end(); it!=en; ++it )
	{   
		XmlObject * obj = (*it);
		if( obj->isContainer() )
			updateSourceControlInfo( obj );
	}
}

void CCoreXmlFile::dumpSourceControlInfo()
{
	FILE * f = fopen( "c:\\temp\\out.txt", "w" );

	for( XmlObjVecIter it=m_objects.begin(); it!=m_objects.end(); ++it )
	{   
		XmlObject * obj = (*it);
		if( obj->isContainer() )
		{
			string guid, str;
			guid2str( obj->m_guid, guid );
			//getSourceControlInfo( obj, str );

			string st2;
			//getSourceControlStat( obj, st2 );

			fprintf( f, "%s\t%s\t%s\n", guid.c_str(), str.c_str(), st2.c_str() );
		}
	}

	fclose(f);
}

bool CCoreXmlFile::filesModifiedByOthers()
{
	for( XmlObjVecIter it=m_objects.begin(); it!=m_objects.end(); ++it )
	{   
		XmlObject * obj = (*it);
		if( obj->isContainer() )
		{
			string filename;
			getContainerFileName(obj, filename);

			// get last write time
			WIN32_FILE_ATTRIBUTE_DATA attr;        
			BOOL res = GetFileAttributesEx( filename.c_str(), GetFileExInfoStandard, &attr );
			// inserted by zolmol:
			DWORD dwerror = res ? ERROR_SUCCESS : GetLastError(); // res == 0 in case of failure
			if( dwerror == ERROR_FILE_NOT_FOUND) // != ERROR_SUCCESS)
			{
				// in case of new objects have been introduced the respective files 
				// are not found (FILE_NOT_FOUND) and the obj->lastwritetime is 0
				ASSERT( obj->m_lastWriteTime == 0);
				continue;
			} // end of zolmol code

			CTime lastWriteTime( attr.ftLastWriteTime );

			if( lastWriteTime > obj->m_lastWriteTime )
				return true;           
		}
	}
	return false;
}

bool CCoreXmlFile::filesModifiedByOthersV3( XmlObjSet& p_readOnlyFiles, XmlObjSet& p_latentFiles)
{
	// the p_readOnlyFiles param indicates which files
	// are intended to be checked out at all (if any)
	// this will help determining more permissively
	// whether a safe checkout is to be allowed or not
	bool ret_prm = false;

	// the permissive return_value calculation
	for( XmlObjSetIter it=p_readOnlyFiles.begin(); it!=p_readOnlyFiles.end(); ++it )
	{   
		XmlObject * obj = (*it);
		if( obj->isContainer() )
		{
			bool ret = false;
			try {
				ret = fileModifiedByOthers( obj);
			} catch( hresult_exception& ) {
				ret = false;
				std::string link = makelink( obj);
				sendMsg( std::string( "Orphan found: ") + link, MSG_ERROR);
			}

			// collect latent changed files
			if( ret && p_latentFiles.end() == p_latentFiles.find( obj))
				p_latentFiles.insert( obj);

			ret_prm = ret_prm || ret; // once became true, should remain true
		}
	}

	return ret_prm;
}

bool CCoreXmlFile::fileModifiedByOthers( XmlObject * obj )
{
	ASSERT( obj);
	ASSERT( obj->isContainer());

	string filename;
	getContainerFileName(obj, filename);

	// get last write time
	WIN32_FILE_ATTRIBUTE_DATA attr;        
	BOOL res = GetFileAttributesEx( filename.c_str(), GetFileExInfoStandard, &attr );
	// inserted by zolmol:
	DWORD dwerror = res ? ERROR_SUCCESS : GetLastError(); // res == 0 in case of failure
	if( dwerror == ERROR_FILE_NOT_FOUND) // != ERROR_SUCCESS)
	{
		// in case of new objects have been introduced the respective files 
		// are not found (FILE_NOT_FOUND) and the obj->lastwritetime is 0
		ASSERT( obj->m_lastWriteTime == 0);
		return false;
	} // end of zolmol code
	bool mydec = false;
	CTime lastWriteTime( attr.ftLastWriteTime );
	bool rv = lastWriteTime > obj->m_lastWriteTime;
	if( isCC())
	{
		return rv;
	}
	else if( isSV())
	{
		VARIANT_BOOL is_utd = VARIANT_FALSE;
		if( m_svnByAPI)
		{
			m_comSvn->IsUpToDate( CComBSTR( filename.c_str()), &is_utd);
			rv = is_utd == VARIANT_TRUE;
		}
		else
		{
			bool out_of_date = false;
			m_cmdSvn->statusOnServer( filename, false, std::string(), &out_of_date);
			rv = out_of_date;
		}

	}
	else if( isSS())
	{
		{
			std::string fullPath;
			getSourceSafePath( obj, fullPath );
			CComBSTR vssFullPath2 = fullPath.c_str();

			std::string fileName12;
			getContainerFileName( obj, fileName12, false );

			CComObjPtr<IVSSItem> item;
			// obtain file handle in VSS
			COMTHROW( m_vssDatabase->get_VSSItem( vssFullPath2, false, &(item.p)) );

			long is_chout;
			COMTHROW( item->get_IsCheckedOut( &is_chout));

			VARIANT_BOOL is_dife;
			COMTHROW( item->get_IsDifferent( CComBSTR(( m_folderPath + "\\" + fileName12).c_str()), &is_dife));
			// if file is different->change happened. Who did the change?
			// Either we were the ones who changed it or somebody else did it.
			// If we did the change, then we should have our .xml file conforming to the VSS .xml file
			// indifferent whether the save operation was done with 'leaving checkouts' or not.
			// .xml file change occurs upon Save, and Save checks in files (think in VSS terms here)
			// even if it still holds them as checked out for further work
			if( is_dife == VARIANT_TRUE && ( is_chout == VSSFILE_CHECKEDOUT || is_chout == VSSFILE_NOTCHECKEDOUT)) //!= VSSFILE_CHECKEDOUT_ME) // either NOT CO, or CO by somebody else
			{
				int l( 0);
				++l; // server version can be only newer
				mydec = true;
			}
			// we could just return mydec's value here
			// without dealing further with .tmp files...
			return mydec;

			// construct temp file name:
			CComBSTR temppath( m_folderPath.c_str());
			//int dex = _mkdir( (m_folderPath + "\\temp").c_str());
			//if( dex == 0 || dex == EEXIST)
			//    temppath.Append( "\\temp");
			temppath.Append( "\\");
			temppath.Append( fileName12.c_str());

			// get version:
			long vnmb = 0;
			COMTHROW( item->get_VersionNumber( &vnmb));

			char buff[15];sprintf( buff, "%i", vnmb);
			temppath.Append( buff);
			temppath.Append( ".tmp"); // to distinguish from the xml backend files 

			// get file into tempdir:
			COMTHROW( item->Get( &temppath, VSSFLAG_TIMEMOD ) ); // previously was NULL // VSSFLAG_TIMEMOD, VSSFLAG_TIMENOW (default), or VSSFLAG_TIMEUPD

			VARIANT_BOOL is_diff;
			COMTHROW( item->get_IsDifferent( temppath, &is_diff));
			ASSERT( is_diff == VARIANT_FALSE);

			std::string temppath_str;
			CopyTo( temppath, temppath_str);
			BOOL res = GetFileAttributesEx( temppath_str.c_str(), GetFileExInfoStandard, &attr );
			DWORD dwerror = res ? ERROR_SUCCESS : GetLastError(); // res == 0 in case of failure
			if( dwerror == ERROR_FILE_NOT_FOUND) // != ERROR_SUCCESS)
			{
				// in case of new objects have been introduced the respective files 
				// are not found (FILE_NOT_FOUND) and the obj->lastwritetime is 0
				ASSERT( obj->m_lastWriteTime == 0);
				//return false;
			}else{
				CTime nlastWriteTime( attr.ftLastWriteTime );
				rv = nlastWriteTime > obj->m_lastWriteTime;
				int hDisk = nlastWriteTime.GetHour();
				int mDisk = nlastWriteTime.GetMinute();
				int sDisk = nlastWriteTime.GetSecond();
				int hObjt = obj->m_lastWriteTime.GetHour();
				int mObjt = obj->m_lastWriteTime.GetMinute();
				int sObjt = obj->m_lastWriteTime.GetSecond();
				CTimeSpan sp = nlastWriteTime - obj->m_lastWriteTime;
				int sSpan = sp.GetSeconds();
				++sSpan;
				//sp.Format(
			}
		}ASSERT( mydec == rv); // my decision could be the same as 'rv'
	}
	return rv;
}

void CCoreXmlFile::sendMsg( const std::string& p_msgStr, int p_msgType)
{
	m_console.sendMsg( p_msgStr, p_msgType);
}

std::string CCoreXmlFile::makelink( XmlObject * ptr)
{
	if( !ptr) return "nullobject";

	// name
	std::string nm;
	ptr->m_attributes.find( ATTRID_NAME)->second->toString( nm);

	// objid
	metaobjidpair_type idpr;
	objIdFromObject( ptr, idpr);

	// something similar to FCO::get_ID() found in MgaFCO.cpp
	char id[20];
	sprintf( id, "id-%04lx-%08lx", idpr.metaid, idpr.objid);

	return std::string( "<A HREF=\"mga:") + id + "\">" + (nm.size()>0?nm:"noname") + "</A>";
}

void CCoreXmlFile::initParsers()
{
	m_domImpl = 0;
	m_domParser = 0;
	m_domErrHandler = 0;

	if( m_strategyShared) // reusable objects are intended
	{
		newDOMObjs( &m_domImpl, &m_domParser, &m_domErrHandler);
		
		if( !m_domImpl || !m_domParser || !m_domErrHandler)
		{
			sendMsg( "Could not create parser and related objects!", MSG_ERROR);
			HR_THROW(E_FILEOPEN);
		}
	}
}

void CCoreXmlFile::finiParsers()
{
	if( m_domParser) 
	{
		delete m_domParser;
		m_domParser = 0;
	}

	if( m_domErrHandler)
	{
		delete m_domErrHandler;
		m_domErrHandler = 0;
	}
}

// pretends to delete a parser 
// but deletes it only in unique strategy case
void CCoreXmlFile::deleteParser( DOMBuilder* *p_parser)
{
	if( !m_strategyShared)
	{
		if( *p_parser) 
		{
			delete *p_parser;
			*p_parser = 0;
		}
	}
}

void CCoreXmlFile::newDOMObjs(  XERCES_CPP_NAMESPACE::DOMImplementationLS* *p_domImpl, XERCES_CPP_NAMESPACE::DOMBuilder* *p_domParser, XERCES_CPP_NAMESPACE::DOMErrorHandler* *p_domErrHandler)
{
	DOMImplementationLS * domimpl = DOMImplementationRegistry::getDOMImplementation( XMLString::transcode("XML 1.0"));//NULL
	ASSERT( domimpl != NULL );
	
	DOMBuilder * parser = !domimpl? 0: domimpl->createDOMBuilder( DOMImplementationLS::MODE_SYNCHRONOUS, NULL );
	ASSERT( parser != NULL );

	DOMErrorHandler* err_handler = new DOMErrorPrinter( &m_console);
	ASSERT( err_handler != NULL);
	if( parser && err_handler)
		parser->setErrorHandler( err_handler);

	*p_domImpl = domimpl;
	*p_domParser = parser;
	*p_domErrHandler = err_handler;
}

void CCoreXmlFile::giveDOMObjs( XERCES_CPP_NAMESPACE::DOMImplementationLS* *p_domImpl, XERCES_CPP_NAMESPACE::DOMBuilder* *p_domParser, XERCES_CPP_NAMESPACE::DOMErrorHandler* *p_domErrHandler)
{
	if( m_strategyShared)
	{
		// reuse the current set
		*p_domImpl = m_domImpl;
		*p_domParser = m_domParser;
		*p_domErrHandler = m_domErrHandler;
	}
	else
	{
		// create a new set of objects
		newDOMObjs( p_domImpl, p_domParser, p_domErrHandler);
	}
}

//DOMImplementationLS * CCoreXmlFile::giveDomImpl()
//{
//	if( m_strategyShared)
//		return m_domImpl;
//	else
//	{
//		DOMImplementationLS * domimpl = DOMImplementationRegistry::getDOMImplementation( XMLString::transcode("XML 1.0"));//NULL
//		ASSERT( domimpl != NULL );
//		return domimpl;
//	}
//}
//
//DOMBuilder * CCoreXmlFile::giveDomParser( DOMImplementationLS * domimpl)
//{
//	if( m_strategyShared)
//		return m_domParser;
//	else
//	{
//		DOMBuilder *parser = !domimpl? 0: domimpl->createDOMBuilder( DOMImplementationLS::MODE_SYNCHRONOUS, NULL );
//		ASSERT( parser != NULL );
//		return parser;
//	}
//}
//
XERCES_CPP_NAMESPACE::DOMDocument* CCoreXmlFile::enclosedParse( const std::string& p_fileName, DOMBuilder* p_parser, bool *p_success)
{
	ASSERT( p_parser);
	ASSERT( p_success);
	if( p_success) *p_success = p_parser != 0;
	if( !p_parser) return 0;

	XERCES_CPP_NAMESPACE::DOMDocument* doc = 0;

	try {
		doc = p_parser->parseURI( p_fileName.c_str());
	}
	catch( const OutOfMemoryException&) {
		sendMsg( "Exception: Out of memory during parsing of " + p_fileName + "!", MSG_ERROR);
		if( p_success) *p_success = false;
	}
	catch (const XMLException& e) {
		char* e_msg = XMLString::transcode( e.getMessage());
		sendMsg( "An error occurred during parsing of " + p_fileName + ". Message: " + e_msg, MSG_ERROR);
		XMLString::release( &e_msg);
		if( p_success) *p_success = false;
	}
	catch (const DOMException& e) {
		const unsigned int maxChars = 2047;
		XMLCh errText[maxChars + 1];
		if( DOMImplementation::loadDOMExceptionMsg( e.code, errText, maxChars))
		{
			char* e_msg = XMLString::transcode( errText);
			sendMsg( "An error occurred during parsing of " + p_fileName + ". Message: " + e_msg, MSG_ERROR);
			XMLString::release( &e_msg);
		}
		else
		{
			char* e_msg = XMLString::transcode( e.getMessage());
			sendMsg( "An error occurred during parsing of " + p_fileName + ". Message: " + e_msg, MSG_ERROR);
			XMLString::release( &e_msg);
		}
		
		if( p_success) *p_success = false;
	}
	catch (...)
	{
		sendMsg( "Unknown exception occurred during parsing of " + p_fileName + "!", MSG_ERROR);
		if( p_success) *p_success = false;
	}

	return doc;
}

DOMBuilder* CCoreXmlFile::getFreshParser( const std::string& p_whoIsTheUser, DOMImplementationLS ** p_ptrRetDomImpl /* = 0 */)
{
	DOMImplementationLS * domimpl = NULL;
	DOMBuilder*           parser  = NULL;
	
	
	//
	// DOM implementation factory creation
	//
	const char            di_msg[] = "Exception: Could not create DOMImplementation for ";
	try {

		domimpl = DOMImplementationRegistry::getDOMImplementation(NULL);

	} catch(...) {

		// was not initialized already? let's do our best
		XMLPlatformUtils::Initialize();

		// try again
		try {

			domimpl = DOMImplementationRegistry::getDOMImplementation(NULL); // 2nd attempt, hoping that XMLPlatformUtils::Initialize() was missing the time before
			
			if( domimpl) sendMsg( "Warning: DOMImplementation created in the second attempt only for " + p_whoIsTheUser, MSG_WARNING);

		} catch(...) {

			domimpl = 0;

			sendMsg( di_msg + p_whoIsTheUser, MSG_ERROR);

		}
	}

	ASSERT( domimpl != NULL );

	//
	// can the DOMBuilder/parser be created ?
	//
	try {

		parser = !domimpl? 0: domimpl->createDOMBuilder( DOMImplementationLS::MODE_SYNCHRONOUS, NULL );

	}
	catch(const DOMException& e) {

		parser = 0;

		char*              e_msg    = 0;
		const unsigned int maxChars = 2047;
		XMLCh errText[maxChars + 1];
		const char         s_msg[]  = "DOMException during parser object creation for ";
		const char         m_msg[]  = ". Message: ";

		if( DOMImplementation::loadDOMExceptionMsg( e.code, errText, maxChars))
		{
			e_msg = XMLString::transcode( errText);
			sendMsg( s_msg + p_whoIsTheUser + m_msg + e_msg, MSG_ERROR);
		}
		else
		{
			e_msg = XMLString::transcode( e.getMessage());
			sendMsg( s_msg + p_whoIsTheUser + m_msg + e_msg, MSG_ERROR);
		}
		XMLString::release( &e_msg);
	}
	catch(...) {

		parser = 0;

		sendMsg( "Exception: Could not create parser for " + p_whoIsTheUser, MSG_ERROR);
	}

	if( domimpl != 0 && parser != 0)
	{
		if( p_ptrRetDomImpl != 0)             // if user interested in the DomImpl
			*p_ptrRetDomImpl = domimpl;       // then fill the ptr

		return parser;
	}

	return 0;
}

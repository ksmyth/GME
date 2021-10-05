//###############################################################################################################################################
//
//	Object Constraint Language Generic Manager
//	OCLCommon_Extensions.cpp
//
//###############################################################################################################################################
#include "Solve4786.h"
#include "OCLCommonEx.h"

#include "TokenEx.h"
#include <algorithm>
#include <string>
#include <cctype>

namespace OclCommonEx {

//##############################################################################################################################################
//
//	F U N C T I O N S
//
//##############################################################################################################################################

	std::string Convert(const CStringW& strIn)
	{
		std::string strOut;

		int len = GetCharLength(strIn, strIn.GetLength(), CP_UTF8);
		if (len == 0)
			return strOut;
		char* out;
		char out_stack[1024];
		std::unique_ptr<char[]> out_heap;
		if (len > sizeof(out_stack) / sizeof(out_stack[0]))
		{
			out_heap = std::unique_ptr<char[]>(new char[len]);
			out = out_heap.get();
		}
		else
		{
			out = out_stack;
		}
		len = WideCharToMultiByte(CP_UTF8, 0, strIn, strIn.GetLength(), out, len, NULL, NULL);
		ASSERT(len);
		strOut = std::string(out, out + len);
		return strOut;
	}

	CStringW Convert(const std::string& strIn)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, strIn.c_str(), strIn.length(), nullptr, 0);

		if (len == 0)
			return CString();
		CString ret;
		len = MultiByteToWideChar(CP_UTF8, 0, strIn.c_str(), strIn.length(), ret.GetBuffer(len), len);
		ASSERT(len);
		ret.ReleaseBuffer(len);
		return ret;
	}

	bool ParseCardinality( const CString& strCardinalityIn )
	{
		CString strCardinality( strCardinalityIn );
		strCardinality.Remove( _T( '\t' ) );
		strCardinality.Remove( _T( '\n' ) );
		strCardinality.Remove( _T( ' ' ) );

		bool bError = false;
		bool bMultiple = false;
		CStringArray arrElements;
		CTokenEx	tokenizer;
		tokenizer.Split( strCardinality, _T(","), arrElements );

		for ( int i = 0 ; i < arrElements.GetSize() ; i++ ) {
			CStringArray arrBounds;
			tokenizer.Split( arrElements[i], _T(".."), arrBounds);

			if ( arrBounds.GetSize() == 0 ) {
				bError = true;
				break;
			}
			else if ( arrBounds.GetSize() == 1 ) {
				int iMultiplicity = 0;
				if ( _stscanf_s( (LPCTSTR) arrBounds[ 0 ], _T("%d"), &iMultiplicity ) != 1 ) {
					bError = true;
					break;
				}
				else {
					if ( iMultiplicity < 0 ) {
						bError = true;
						break;
					}
					if ( iMultiplicity > 1 )
						bMultiple = true;
				}
			}
			else if ( arrBounds.GetSize() == 2 ) {
				int iMultiplicityMin = 0;
				int iMultiplicityMax = 0;

				if( _stscanf_s( (LPCTSTR) arrBounds[ 0 ], _T("%d"), &iMultiplicityMin ) != 1 ) {
					bError = true;
					break;
				}
				else {
					if ( iMultiplicityMin < 0 ) {
						bError = true;
						break;
					}
					if ( iMultiplicityMin > 1 )
						bMultiple = true;
				}

				if( _stscanf_s( (LPCTSTR) arrBounds[ 1 ], _T("%d"), &iMultiplicityMax ) != 1 ) {
					if ( arrBounds[ 1 ] != _T("*") ) {
						bError = true;
						break;
					}
					bMultiple = true;
				}
				else {
					if ( iMultiplicityMax < 0 ) {
						bError = true;
						break;
					}
					if ( iMultiplicityMax < iMultiplicityMax ) {
						bError = true;
						break;
					}
					if ( iMultiplicityMax > 1 )
						bMultiple = true;
				}
			}
		}

		return bError || bMultiple;
	}

	std::string GetConnPointRole( CComPtr<IMgaConnPoint> spCP )
	{
		CString strRole;
		COMTHROW( spCP->get_ConnRole( PutOut( strRole ) ) );
		return Convert( strRole );
	}

	std::string GetObjectName( CComPtr<IMgaObject> spObject )
	{
		CString strName;
		COMTHROW( spObject->get_Name( PutOut( strName ) ) );
		return Convert( strName );
	}

	std::string GetObjectName( CComPtr<IMgaMetaBase> spObject )
	{
		CString strName;
		COMTHROW( spObject->get_Name( PutOut( strName ) ) );
		return Convert( strName );
	}

	std::string GetObjectKind( CComPtr<IMgaObject> spObject )
	{
		CString strKind;
		CComPtr<IMgaMetaBase> spMeta;
		COMTHROW( spObject->get_MetaBase( &spMeta ) );
		CComQIPtr<IMgaMetaRole> spRole = spMeta;
		if ( spRole.p ) {
			CComPtr<IMgaMetaFCO> spKind;
			COMTHROW( spRole->get_Kind( &spKind ) );
			return GetObjectName( spKind.p );
		}
		return GetObjectName( spMeta );
	}

	std::string GetObjectDisplayedName(CComPtr<IMgaObject>& spObject)
	{
		CComPtr<IMgaMetaBase> spMeta;
		COMTHROW( spObject->get_MetaBase( &spMeta ) );
		_bstr_t name;
		COMTHROW(spMeta->get_Name(name.GetAddress()));
		return static_cast<const char*>(name);
	}

	std::string GetFCORole( CComPtr<IMgaFCO> spFCO )
	{
		CComPtr<IMgaMetaRole> spMetaRole;
		COMTHROW( spFCO->get_MetaRole( &spMetaRole ) );
		return ( spMetaRole.p ) ? GetObjectName( spMetaRole.p ) : "";
	}

	objtype_enum GetObjectType( CComPtr<IMgaObject> spObject )
	{
		objtype_enum eType;
		COMTHROW( spObject->get_ObjType( &eType ) );
		return eType;
	}

	objtype_enum GetObjectType( CComPtr<IMgaMetaBase> spObject )
	{
		objtype_enum eType;
		COMTHROW( spObject->get_ObjType( &eType ) );
		return eType;
	}

	bool IsInLibrary( CComPtr<IMgaObject> spObject )
	{
		VARIANT_BOOL bInLib;
		COMTHROW( spObject->get_IsLibObject( &bInLib ) );
		return bInLib == VARIANT_TRUE;
	}

	std::string ParseLibraryName( const std::string& strFullName )
	{
		std::string strName;
		size_t iPos = strFullName.rfind( "\\" );
		if ( iPos != std::string::npos )
			strName = strFullName.substr( iPos + 1 );
		iPos = strName.rfind( "." );
		if ( iPos != std::string::npos )
			strName = strName.substr( 0, iPos );
		Trim( strName );
		return strName;
	}

	CComPtr<IMgaFolder> GetFirstLibrary( CComPtr<IMgaObject> spObject )
	{
		// If It is NULL

		if ( ! spObject.p )
			return NULL;

		// If this is the library, and it is not included in other library

		CComQIPtr<IMgaFolder> spFolder = spObject;
		if ( spFolder.p ) {
			CString strLibName;
			COMTHROW( spFolder->get_LibraryName( PutOut( strLibName ) ) );
			if ( ! strLibName.IsEmpty() )
				return spFolder.p;
		}

		// If it is included in other library

		if ( ! IsInLibrary( spObject ) )
			return NULL;
		if ( ! spFolder.p ) {
			CComQIPtr<IMgaFCO> spFCO = spObject;
			CComPtr<IMgaFCO> spRootFCO;
			COMTHROW( spFCO->get_RootFCO( &spRootFCO ) );
			spObject = spRootFCO;
		}
		CComPtr<IMgaObject> spParent;
		COMTHROW( spObject->GetParent( &spParent ) );
		return GetFirstLibrary( spParent );
	}

	StringVector GetLibraryPath( CComPtr<IMgaObject> spObject )
	{
		StringVector vecPath;

		CComPtr<IMgaFolder> spFolder = GetFirstLibrary( spObject );
		while ( spFolder.p ) {
			CString strFullName;
			COMTHROW( spFolder->get_LibraryName( PutOut( strFullName ) ) );
			vecPath.push_back( ParseLibraryName( Convert( strFullName ) ) );
			CComPtr<IMgaObject> spParent;
			COMTHROW( spFolder->GetParent( &spParent ) );
			spFolder = GetFirstLibrary( spParent );
		}
		std::reverse( vecPath.begin(), vecPath.end() );
		return vecPath;
	}

	std::string ObjectTypeToString( objtype_enum eType )
	{
		switch ( eType ) {
			case OBJTYPE_ATOM : return "Atom";
			case OBJTYPE_MODEL : return "Model";
			case OBJTYPE_CONNECTION : return "Connection";
			case OBJTYPE_REFERENCE : return "Reference";
			case OBJTYPE_SET : return "Set";
			case OBJTYPE_FOLDER : return "Folder";
			default : return "";
		}
	}

	objtype_enum StringToObjectType( const std::string& strType )
	{
		if ( strType == "Atom" )
			return OBJTYPE_ATOM;
		if ( strType == "Model" )
			return OBJTYPE_MODEL;
		if ( strType == "Connection" )
			return OBJTYPE_CONNECTION;
		if ( strType == "Reference" )
			return OBJTYPE_REFERENCE;
		if ( strType == "Set" )
			return OBJTYPE_SET;
		if ( strType == "Folder" )
			return OBJTYPE_FOLDER;
		return OBJTYPE_NULL;
	}

	std::string CreateType( const std::string& strKind, objtype_enum eType )
	{
		if ( eType == OBJTYPE_NULL )
			return "meta::" + strKind;
		else
			return "meta::" + LowerFirst( ObjectTypeToString( eType ) ) + "::" + strKind;
	}

	bool ResolveType( const std::string& strFullName, std::string& strKind, objtype_enum& eType )
	{
		eType = OBJTYPE_NULL;
		strKind = "";
		size_t iPos = strFullName.find( "::" );
		if ( iPos == std::string::npos ) {
			strKind = strFullName;
			return true;
		}
		if ( strFullName.substr( 0, iPos ) == "meta" ) {
			std::string strTemp = strFullName.substr( iPos + 2 );
			iPos = strTemp.find( "::" );
			if ( iPos == std::string::npos ) {
				strKind = strTemp;
				return true;
			}
			eType = StringToObjectType( UpperFirst( strTemp.substr( 0, iPos ) ) );
			if ( eType != OBJTYPE_NULL ) {
				strKind = strTemp.substr( iPos + 2 );
				return true;
			}
		}
		return false;
	}

	std::string LowerFirst( const std::string& strValue )
	{
		if ( strValue.empty() )
			return strValue;
		std::string strVal = strValue;
		std::transform(strVal.begin(), strVal.begin() + 1, strVal.begin(), std::tolower);
		return strVal;
	}

	std::string UpperFirst( const std::string& strValue )
	{
		if ( strValue.empty() )
			return strValue;
		std::string strVal = strValue;
		std::transform(strVal.begin(), strVal.begin() + 1, strVal.begin(), std::toupper);
		return strVal;
	}

	CString LowerFirst( const CString& strValue )
	{
		if ( strValue.IsEmpty() )
			return strValue;
		CString strTemp = strValue.Left( 1 );
		strTemp.MakeLower();
		return strTemp + strValue.Mid( 1 );
	}

	CString UpperFirst( const CString& strValue )
	{
		if ( strValue.IsEmpty() )
			return strValue;
		CString strTemp = strValue.Left( 1 );
		strTemp.MakeUpper();
		return strTemp + strValue.Mid( 1 );
	}

	void AddAssociation( OclMeta::TypeManager* pManager, OclMeta::Association* pAssociation, OclMeta::AssociationVector& vecAssociations, bool bCantBeSet )
	{
		OclMeta::Association* pAssociationFound = NULL;
		unsigned int i;
		for ( i = 0 ; i < vecAssociations.size() ; i++ )
			if ( pAssociation->IsIdentical( *vecAssociations[ i ] ) ) {
				pAssociationFound = vecAssociations[ i ];
				break;
			}
		if ( pAssociationFound ) {
			TypeSeq vecType = pAssociation->GetReturnTypeSeq();
			TypeSeq vecTypeFound = pAssociationFound->GetReturnTypeSeq();
			TypeSeq vecTypeNew;
			if ( ! bCantBeSet )
				vecTypeNew.push_back( "ocl::Set" );
			if ( vecType.size() == 1 )
				if ( vecTypeFound.size() == 1 )
					vecTypeNew.push_back( pManager->GetTypeBase( vecType[ 0 ], vecTypeFound[ 0 ] ) );
				else
					vecTypeNew.push_back( pManager->GetTypeBase( vecType[ 0 ], vecTypeFound[ 1 ] ) );
			else
				if ( vecTypeFound.size() == 1 )
					vecTypeNew.push_back( pManager->GetTypeBase( vecType[ 1 ], vecTypeFound[ 0 ] ) );
				else
					vecTypeNew.push_back( pManager->GetTypeBase( vecType[ 1 ], vecTypeFound[ 1 ] ) );
			vecAssociations[ i ]->SetReturnTypeSeq( vecTypeNew );
			delete pAssociation;
		}
		else
			vecAssociations.push_back( pAssociation );
	}

	void AddAssociations( OclMeta::AssociationVector& vecAssociationsToAdd, OclMeta::AssociationVector& vecAssociations )
	{
		for ( unsigned int i = 0 ; i < vecAssociationsToAdd.size() ; i++ )
			vecAssociations.push_back( vecAssociationsToAdd[ i ] );
		vecAssociationsToAdd.clear();
	}

//##############################################################################################################################################
//
//	F U N C T I O N S
//
//##############################################################################################################################################

	bool AddObject( CComPtr<IMgaFCO> spFCO, FCOVector& vecFCOs )
	{
		if ( ! spFCO.p )
			return false;
		if ( ContainsObject( spFCO, vecFCOs ) )
			return false;
		vecFCOs.push_back( spFCO.p );
		return true;
	}

	bool AddObject( CComPtr<IMgaObject> spObject, ObjectVector& vecObjects )
	{
		if ( ! spObject.p )
			return false;
		if ( ContainsObject( spObject, vecObjects ) )
			return false;
		vecObjects.push_back( spObject.p );
		return true;
	}

	bool ContainsObject( CComPtr<IMgaFCO> spFCO, FCOVector& vecFCOs )
	{
		for ( unsigned int i = 0 ; i < vecFCOs.size() ; i++ )
			if ( vecFCOs[ i ].p == spFCO.p )
				return true;
		return false;
	}

	bool ContainsObject( CComPtr<IMgaObject> spObject, ObjectVector& vecObjects )
	{
		for ( unsigned int i = 0 ; i < vecObjects.size() ; i++ )
			if ( vecObjects[ i ].p == spObject.p )
				return true;
		return false;
	}

	bool IsAbstract( CComPtr<IMgaFCO> spFCO )
	{
		std::string strKind = GetObjectKind( spFCO.p );
		if ( strKind == "Folder" )
			return false;
		if ( strKind == "FCO" )
			return true;
		short sAbstract;
		if ( ! SUCCEEDED( spFCO->get_BoolAttrByName( CComBSTR( L"IsAbstract" ), &sAbstract ) ) )
			return false;
		return ( sAbstract == 0 ) ? false : true;
	}

	bool GetAllObjects( CComPtr<IMgaFCO>& spFCO, CComPtr<IMgaFCOs>& spRefs )
	{
		spRefs = NULL;
		CComQIPtr<IMgaReference> spRef = spFCO;
		if ( spRef.p ) {
			spFCO = NULL;
			COMTHROW( spRef->get_Referred( &spFCO ) );
			if ( ! spFCO.p )
				return false;
		}
		COMTHROW( spFCO->get_ReferencedBy( &spRefs ) );
		return true;
	}

	void GetAssociationEnds( CComPtr<IMgaFCO> spFCO, const std::string& strRole, const std::string& strKind, FCOVector& vecFCOs )
	{
		std::string strCPRole, strCKind;
		CComPtr<IMgaConnPoints> spStartCPs;
		COMTHROW( spFCO->get_PartOfConns( &spStartCPs ) );
		MGACOLL_ITERATE( IMgaConnPoint , spStartCPs ) {
			CComPtr<IMgaConnPoint> spStartCP = MGACOLL_ITER;
			strCPRole = GetConnPointRole( spStartCP );
			if ( strRole.empty() || strRole == GetConnPointRole( spStartCP ) ) {
				CComPtr<IMgaConnection> spConnection;
				COMTHROW( spStartCP->get_Owner( &spConnection ) );
				strCKind = GetObjectKind( spConnection.p );
				if ( strKind.empty() || strKind == GetObjectKind( spConnection.p ) ) {
					CComPtr<IMgaConnPoints> spEndCPs;
					COMTHROW( spConnection->get_ConnPoints( &spEndCPs ) );
					MGACOLL_ITERATE( IMgaConnPoint, spEndCPs ) {
						CComPtr<IMgaConnPoint> spEndCP = MGACOLL_ITER;
						if ( spEndCP != spStartCP ) {
							CComPtr<IMgaFCO> spTarget;
							COMTHROW( spEndCP->get_Target( &spTarget ) );
							vecFCOs.push_back( spTarget.p );
						}
					} MGACOLL_ITERATE_END;
				}
			}
		} MGACOLL_ITERATE_END;
	}

	void GetAssociationEnds( CComPtr<IMgaFCO> spFCO, const std::string& strRole, const std::string& strKind, FCOVector& vecFCOs, FCOVector& vecConnections )
	{
		CComPtr<IMgaConnPoints> spStartCPs;
		COMTHROW( spFCO->get_PartOfConns( &spStartCPs ) );
		MGACOLL_ITERATE( IMgaConnPoint , spStartCPs ) {
			CComPtr<IMgaConnPoint> spStartCP = MGACOLL_ITER;
			if ( strRole.empty() || strRole == GetConnPointRole( spStartCP ) ) {
				CComPtr<IMgaConnection> spConnection;
				COMTHROW( spStartCP->get_Owner( &spConnection ) );
				if ( strKind.empty() || strKind == GetObjectKind( spConnection.p ) ) {
					CComPtr<IMgaConnPoints> spEndCPs;
					COMTHROW( spConnection->get_ConnPoints( &spEndCPs ) );
					MGACOLL_ITERATE( IMgaConnPoint, spEndCPs ) {
						CComPtr<IMgaConnPoint> spEndCP = MGACOLL_ITER;
						if ( spEndCP != spStartCP ) {
							CComPtr<IMgaFCO> spTarget;
							COMTHROW( spEndCP->get_Target( &spTarget ) );
							vecFCOs.push_back( spTarget.p );
							vecConnections.push_back( spConnection.p );
						}
					} MGACOLL_ITERATE_END;
				}
			}
		} MGACOLL_ITERATE_END;
	}

	void GetInheritances( CComPtr<IMgaFCO> spFCO, const std::string& strInheritanceType, bool bIsParent, FCOVector& vecFCOs )
	{
		AddObject( spFCO, vecFCOs );
		std::string strKind1;
		std::string strKind2;

		if ( strInheritanceType == "Interface" ) {
			strKind1 = ( bIsParent ) ? "DerivedIntInheritance" : "BaseIntInheritance";
			strKind2 = ( ! bIsParent ) ? "DerivedIntInheritance" : "BaseIntInheritance";
		}
		else if ( strInheritanceType == "Implementation" ) {
			strKind1 = ( bIsParent ) ? "DerivedImpInheritance" : "BaseImpInheritance";
			strKind2 = ( ! bIsParent ) ? "DerivedImpInheritance" : "BaseImpInheritance";
		}
		else {
			strKind1 = ( bIsParent ) ? "DerivedInheritance" : "BaseInheritance";
			strKind2 = ( ! bIsParent ) ? "DerivedInheritance" : "BaseInheritance";
		}

		FCOVector vecInheritances;
		GetAssociationEnds( spFCO, "", strKind1, vecInheritances );
		for ( unsigned int i = 0 ; i < vecInheritances.size() ; i++ )
			GetAssociationEnds( vecInheritances[ i ].p, "", strKind2, vecFCOs );
	}

	void GetEquivalences( CComPtr<IMgaFCO> spFCO, const std::string& strEquivalencesType, bool bToLeft, FCOVector& vecFCOs )
	{
		AddObject( spFCO, vecFCOs );
		std::string strKind1;
		std::string strKind2;

		if ( strEquivalencesType == "Aspect" ) {
			strKind1 = ( bToLeft ) ? "SameAspectLeft" : "SameAspectRight";
			strKind2 = ( ! bToLeft ) ? "SameAspectRight" : "SameAspectLeft";
		}
		else if ( strEquivalencesType == "Folder" ) {
			strKind1 = ( bToLeft ) ? "SameFolderLeft" : "SameFolderRight";
			strKind2 = ( ! bToLeft ) ? "SameFolderRight" : "SameFolderLeft";
		}
		else {
			strKind1 = ( bToLeft ) ? "EquivLeft" : "EquivRight";
			strKind2 = ( ! bToLeft ) ? "EquivRight" : "EquivLeft";
		}

		FCOVector vecEquivalences;
		GetAssociationEnds( spFCO, "", strKind1, vecEquivalences );
		for ( unsigned int i = 0 ; i < vecEquivalences.size() ; i++ )
			GetAssociationEnds( vecEquivalences[ i ].p, "", strKind2, vecFCOs );
	}

	void GetMetaObjectsR( CComPtr<IMgaMetaBase> spMeta, const std::string& strName, objtype_enum eType, MetaBaseVector& vecMetaBases )
	{
		if ( ( eType == OBJTYPE_NULL || eType == OclCommonEx::GetObjectType( spMeta ) ) && ( strName.empty() || strName == GetObjectName( spMeta ) ) )
			vecMetaBases.push_back( spMeta.p );

		CComPtr<IMgaMetaFCOs> spFCOs;
		CComQIPtr<IMgaMetaModel> spModel = spMeta;
		if ( spModel.p )
			COMTHROW( spModel->get_DefinedFCOs( &spFCOs ) );
		else {
			CComQIPtr<IMgaMetaFolder> spFolder = spMeta;
			if ( spFolder.p ) {
				CComPtr<IMgaMetaFolders> spFolders;
				COMTHROW( spFolder->get_DefinedFolders( &spFolders ) );
				MGACOLL_ITERATE( IMgaMetaFolder, spFolders ) {
					GetMetaObjectsR( MGACOLL_ITER.p, strName, eType, vecMetaBases );
				} MGACOLL_ITERATE_END;
				COMTHROW( spFolder->get_DefinedFCOs( &spFCOs ) );
			}
		}
		if ( spFCOs.p ) {
			MGACOLL_ITERATE( IMgaMetaFCO, spFCOs ) {
				GetMetaObjectsR( MGACOLL_ITER.p, strName, eType, vecMetaBases );
			} MGACOLL_ITERATE_END;
		}
	}

	void GetMetaObjects( CComPtr<IMgaMetaProject> spProject, const std::string& strName, objtype_enum eType, MetaBaseVector& vecMetaBases )
	{
		CComPtr<IMgaMetaFolder> spRootFolder;
		COMTHROW( spProject->get_RootFolder( &spRootFolder ) );
		GetMetaObjectsR( spRootFolder.p, strName, eType, vecMetaBases );
	};

	std::string GetPointerRole( CComPtr<IMgaMetaPointerSpec> spPointer )
	{
		CString strName;
		COMTHROW( spPointer->get_Name( PutOut( strName ) ) );
		return Convert( strName );
	}

	ModelMap GetModels( CComPtr<IMgaMetaProject> spProject )
	{
		MetaBaseVector vecModels;
		GetMetaObjects( spProject, "", OBJTYPE_MODEL, vecModels );
		ModelMap mapModels;
		for ( unsigned int i = 0 ; i < vecModels.size() ; i++ ) {
			CComQIPtr<IMgaMetaModel> spModel = vecModels[ i ].p;
			mapModels.insert( ModelMap::value_type( GetObjectName( vecModels[ i ].p ), spModel.p ) );
		}
		return mapModels;
	}

	StringVector GetPointerDescriptions( CComPtr<IMgaMetaPointerSpec> spPointer )
	{
		StringVector vecDescriptions;
		CComPtr<IMgaMetaPointerItems> spItems;
		COMTHROW( spPointer->get_Items( &spItems ) );
		MGACOLL_ITERATE( IMgaMetaPointerItem, spItems ) {
			CString strDesc;
			COMTHROW( MGACOLL_ITER->get_Desc( PutOut( strDesc ) ) );
			strDesc.TrimLeft();
			strDesc.TrimRight();
			vecDescriptions.push_back( Convert( strDesc ) );
		} MGACOLL_ITERATE_END;
		return vecDescriptions;
	}

	StringVector TranslateDescriptionsToKinds( ModelMap& mapModels, const StringVector& vecDescriptions )
	{
		StringVector vecKinds;
		for ( unsigned int i = 0 ; i < vecDescriptions.size() ; i++ ) {
			size_t iPos = vecDescriptions[ i ].find( ' ' );
			if ( iPos == std::string::npos ) {
				if ( std::find( vecKinds.begin(), vecKinds.end(), vecDescriptions[ i ] ) == vecKinds.end() )
					vecKinds.push_back( vecDescriptions[ i ] );
			}
			else {
				std::string strModel = vecDescriptions[ i ].substr( 0, iPos );
				std::string strRole = vecDescriptions[ i ].substr( iPos + 1 );
				ModelMap::iterator iModel = mapModels.find( strModel );
				CComPtr<IMgaMetaRole> spRole;
				COMTHROW( (*iModel).second->get_RoleByName( CComBSTR( Convert( strRole ) ), &spRole ) );
				CComPtr<IMgaMetaFCO> spFCO;
				COMTHROW( spRole->get_Kind( &spFCO ) );
				std::string strKind = GetObjectName( spFCO.p );
				if ( std::find( vecKinds.begin(), vecKinds.end(), strKind ) == vecKinds.end() )
					vecKinds.push_back( strKind );
			}
		}
		return vecKinds;
	}

	void GetDerivedObjectsR( CComPtr<IMgaFCO> spFCO, ObjectVector& vecObjects )
	{
		vecObjects.push_back( spFCO.p );
		CComPtr<IMgaFCOs> spFCOs;
		COMTHROW( spFCO->get_DerivedObjects( &spFCOs ) );
		if ( spFCOs.p ) {
			MGACOLL_ITERATE( IMgaFCO, spFCOs ) {
				GetDerivedObjectsR( MGACOLL_ITER, vecObjects );
			} MGACOLL_ITERATE_END;
		}
	}

	void GetDerivedObjects( CComPtr<IMgaProject> spProject, const std::string& strType, ObjectVector& vecObjects )
	{
		CComPtr<IMgaFCOs> spFCOs;
		CComPtr<IMgaFilter> spFilter;
		COMTHROW( spProject->CreateFilter( &spFilter ) );
		COMTHROW( spFilter->put_Name( CComBSTR( OclCommonEx::Convert( strType ) ) ) );
		COMTHROW( spProject->AllFCOs( spFilter, &spFCOs ) );
		MGACOLL_ITERATE( IMgaFCO, spFCOs ) {
			GetDerivedObjectsR( MGACOLL_ITER, vecObjects );
		} MGACOLL_ITERATE_END;
	}

	void GetKindFolders( CComPtr<IMgaFolder> spFolder, const std::string& strKind, bool bKind, ObjectVector& vecObjects )
	{
		if ( strKind.empty() || ( ( bKind ) ? GetObjectKind( spFolder.p ) : GetObjectName( spFolder.p ) ) == strKind )
			AddObject( spFolder.p, vecObjects );
		CComPtr<IMgaFolders> spFolders;
		COMTHROW( spFolder->get_ChildFolders( &spFolders ) );
		MGACOLL_ITERATE( IMgaFolder, spFolders ) {
			GetKindFolders( MGACOLL_ITER, strKind, bKind, vecObjects );
		} MGACOLL_ITERATE_END;
	}

	void GetKindObjects( CComPtr<IMgaProject> spProject, const std::string& strKind, ObjectVector& vecObjects )
	{
		CComPtr<IMgaMetaProject> spMetaProject;
		COMTHROW( spProject->get_RootMeta( &spMetaProject ) );
		MetaBaseVector vecMetas;
		GetMetaObjects( spMetaProject, strKind, OBJTYPE_NULL, vecMetas );

		for ( unsigned int i = 0 ; i < vecMetas.size() ; i++ ) {
			CComQIPtr<IMgaMetaFCO> spFCO = vecMetas[ i ].p;

			if ( spFCO.p ) {

				// Collect Roles and Create Metaref List

				CString strMetaRefs;
				metaref_type ref;
				COMTHROW( vecMetas[ i ]->get_MetaRef( &ref ) );
				strMetaRefs.Format( _T("#%d , "), ref );
				CComPtr<IMgaMetaRoles> spRoles;
				COMTHROW( spFCO->get_UsedInRoles( &spRoles ) );
				MGACOLL_ITERATE( IMgaMetaRole, spRoles ) {
					COMTHROW( MGACOLL_ITER->get_MetaRef( &ref ) );
					CString strTemp;
					strTemp.Format( _T("#%d , "), ref );
					strMetaRefs += strTemp;
				} MGACOLL_ITERATE_END;

				// Create a Filter and get All FCOs / Call Inheritance Chain Root with Types

				CComPtr<IMgaFilter> spFilter;
				COMTHROW( spProject->CreateFilter( &spFilter ) );
				COMTHROW( spFilter->put_Kind( CComBSTR( strMetaRefs ) ) );
				CComPtr<IMgaFCOs> spFCOs;
				COMTHROW( spProject->AllFCOs( spFilter, &spFCOs ) );
				MGACOLL_ITERATE( IMgaFCO, spFCOs ) {
					AddObject( MGACOLL_ITER.p, vecObjects );
				} MGACOLL_ITERATE_END;
			}

			CComPtr<IMgaFolder> spRootFolder;
			COMTHROW( spProject->get_RootFolder( &spRootFolder ) );
			GetKindFolders( spRootFolder, strKind, true, vecObjects );
		}
	}

	void GetObjectPathR( CComPtr<IMgaObject> spObject, std::string& strPath )
	{
		CComPtr<IMgaObject> spParent;
		COMTHROW( spObject->GetParent( &spParent ) );
		if ( spParent.p ) {
			strPath = "/" + GetObjectName( spParent ) + strPath;
			GetObjectPathR( spParent, strPath );
		}
	}

	std::string GetObjectPath( CComPtr<IMgaObject> spObject )
	{
		std::string strPath;
		GetObjectPathR( spObject, strPath );
		return ( strPath.empty() ) ? "/" : strPath;
	}

	std::string GetObjectDescription( CComPtr<IMgaObject> spObject )
	{
		if ( ! spObject.p )
			return "null";

		std::string strName = GetObjectName( spObject );
		std::string strKind = GetObjectKind( spObject );
		objtype_enum eType = GetObjectType( spObject );
		std::string strMetaKind = "gme::" + ObjectTypeToString( eType );

		std::string strContainmentRole;
		CComQIPtr<IMgaFCO> spFCO = spObject;
		if ( spFCO.p ) {
			strContainmentRole = GetFCORole( spFCO.p );
			if ( strContainmentRole == strKind )
				strContainmentRole = "";
			if ( eType == OBJTYPE_SET )
				strContainmentRole = "";
		}

		std::string strPath = GetObjectPath( spObject );

		std::string strOut = strName + " { ";

		switch ( eType ) {
			case OBJTYPE_FOLDER :
			case OBJTYPE_ATOM :
			case OBJTYPE_MODEL :
			case OBJTYPE_SET :
			{
				if ( ! strContainmentRole.empty() )
					strOut += "role: " + strContainmentRole + "; ";
				strOut += "kind: meta::" + strKind + "; ";
				return strOut + "path: " + strPath + "; }";
			}
			case OBJTYPE_REFERENCE :
			{
				strOut += "kind: meta::" + strKind + "; ";
				CComQIPtr<IMgaReference> spReference = spFCO;
				CComPtr<IMgaFCO> spTarget;
				COMTHROW( spReference->get_Referred( &spTarget ) );
				if ( spTarget.p )
					strOut += "referred: " + GetObjectName( spTarget.p ) + "; ";
				if ( ! strContainmentRole.empty() )
					strOut += "role: " + strContainmentRole + "; ";
				return strOut + "path: " + strPath + "; }";
			}
			case OBJTYPE_CONNECTION :
			{
				strOut += "kind: meta::" + strKind + "; ";
				CComQIPtr<IMgaConnection> spConnection = spFCO;
				CComPtr<IMgaConnPoints> spCPs;
				COMTHROW( spConnection->get_ConnPoints( &spCPs ) );
				MGACOLL_ITERATE( IMgaConnPoint, spCPs ) {
					strOut += GetConnPointRole( MGACOLL_ITER ) + ": ";
					CComPtr<IMgaFCO> spTarget;
					COMTHROW( MGACOLL_ITER->get_Target( &spTarget ) );
					strOut += GetObjectName( spTarget.p ) + "; ";
				} MGACOLL_ITERATE_END;
				if ( ! strContainmentRole.empty() )
					strOut += "role: " + strContainmentRole + "; ";
				return strOut + "path: " + strPath + "; }";
			}
			default :
				return strOut + "}";
		}
	}

	std::string GetConnectionPointDescription( CComPtr<IMgaConnPoint> spCP )
	{
		if ( ! spCP.p )
			return "null";

		std::string strOut = "gme::ConnectionPoint { ";

		std::string strRole = GetConnPointRole( spCP );
		if ( ! strRole.empty() )
			strOut += "role: " + strRole + "; ";

		CComPtr<IMgaFCO> spTarget;
		COMTHROW( spCP->get_Target( &spTarget ) );
		strOut += "target: " + GetObjectName( spTarget.p ) + "; ";

		CComPtr<IMgaConnection> spConnection;
		COMTHROW( spCP->get_Owner( &spConnection ) );
		strOut += "owner: " + GetObjectName( spConnection.p ) + "; ";

		return strOut + "path: " + GetObjectPath( spConnection.p ) + "; }";
	}

}; // namespace OclCommonEx

//###############################################################################################################################################
//
//	Object Constraint Language Generic Manager
//	OCLGMEECFacade.cpp
//
//###############################################################################################################################################
#include "Solve4786.h"
#include "OCLGMEECFacade.h"
#include "OCLTypeExGMEEC.h"
#include "OCLCommonEx.h"

namespace OclGmeEC
{
	void GetDerivedContext( CComPtr<IMgaFCO> spFCO, FCOVector& vecFCOs, FCOVector& vecExamined );

	void GetAllObjectsConsideredAsContext( CComPtr<IMgaFCO> spFCO, FCOVector& vecFCOs, FCOVector& vecExamined )
	{
		if ( OclCommonEx::ContainsObject( spFCO, vecExamined ) )
			return;
		vecExamined.push_back( spFCO.p );
		CComPtr<IMgaFCOs> spRefs;
		if ( OclCommonEx::GetAllObjects( spFCO, spRefs ) ) {
			if ( ! OclCommonEx::IsAbstract( spFCO ) && OclCommonEx::GetObjectType( spFCO.p ) != OBJTYPE_REFERENCE )
				OclCommonEx::AddObject( spFCO, vecFCOs );
			GetDerivedContext( spFCO, vecFCOs, vecExamined );
			if ( spRefs.p )
				MGACOLL_ITERATE( IMgaFCO, spRefs ) {
					GetDerivedContext( MGACOLL_ITER, vecFCOs, vecExamined );
				} MGACOLL_ITERATE_END;
		}
	}

	void GetDerivedContext( CComPtr<IMgaFCO> spFCO, FCOVector& vecFCOs, FCOVector& vecExamined )
	{
		FCOVector vecDeriveds;
		OclCommonEx::GetInheritances( spFCO, "Normal", false, vecDeriveds );
		OclCommonEx::GetInheritances( spFCO, "Implementation", false, vecDeriveds );
		OclCommonEx::GetInheritances( spFCO, "Interface", false, vecDeriveds );
		for ( unsigned int i = 0 ; i < vecDeriveds.size() ; i++ )
			GetAllObjectsConsideredAsContext( vecDeriveds[ i ].p, vecFCOs, vecExamined );
	}

//##############################################################################################################################################
//
//	C L A S S : OclGmeEC::ConstraintDefinitionFactory <<< + OclImplementation::ConstraintDefinitionFactory
//
//==============================================================================================================================================
//
//	D E S C R I P T I O N :
//
//##############################################################################################################################################

	class ConstraintDefinitionFactory
		: public OclImplementation::ConstraintDefinitionFactory
	{
		public :
			CComPtr<IMgaProject> 	m_spProject;
			OclTree::TreeManager* 	m_pTreeManager;

			virtual void GetFeatures( const OclSignature::Attribute& signature, OclMeta::AttributeVector& vecFeatures )
			{
				OclGme::ConstraintFunctionVector vecFunctions;
				GetConstraintFunctions( vecFunctions );
				for ( unsigned int i = 0 ; i < vecFunctions.size() ; i++ ) {
					if ( m_pTreeManager->GetTypeManager()->IsTypeA( signature.GetTypeName(), vecFunctions[ i ]->GetContextType() ) >= 0 )
						if ( 	vecFunctions[ i ]->GetName() == signature.GetName() && vecFunctions[ i ]->GetStereotype() == Ocl::Constraint::CS_ATTRIBUTEDEF )
							vecFeatures.push_back( new OclMeta::Attribute( signature.GetName(), CreateReturnType( vecFunctions[ i ]->GetReturnType() ), NULL, true ) );
				}
			}

			virtual void GetFeatures( const OclSignature::Method& signature, OclMeta::MethodVector& vecFeatures )
			{
				OclGme::ConstraintFunctionVector vecFunctions;
				GetConstraintFunctions( vecFunctions );
				for ( unsigned int i = 0 ; i < vecFunctions.size() ; i++ ) {
					if ( m_pTreeManager->GetTypeManager()->IsTypeA( signature.GetTypeName(), vecFunctions[ i ]->GetContextType() ) >= 0 ) {
						OclCommon::FormalParameterVector vecParamsIn = vecFunctions[ i ]->GetFormalParameters();
						if ( vecFunctions[ i ]->GetName() == signature.GetName() && vecFunctions[ i ]->GetStereotype() == Ocl::Constraint::CS_METHODDEF && (int) vecParamsIn.size() == signature.GetParameterCount() ) {
							OclCommon::FormalParameterVector vecParams;
							for ( unsigned int j = 0 ; j < vecParamsIn.size() ; j++ ) {
								TypeSeq vecType;
								OclCommon::Convert( vecParamsIn[ j ].GetTypeName(), vecType );
								vecParams.push_back( OclCommon::FormalParameter( vecParamsIn[ j ].GetName(), vecType[ 0 ], true ) );
							}
							vecFeatures.push_back( new OclMeta::Method( signature.GetName(), vecParams, CreateReturnType( vecFunctions[ i ]->GetReturnType() ), NULL, true ) );
						}
					}
				}
			}

		private :
			void GetConstraintFunctions( OclGme::ConstraintFunctionVector& vecFunctions )
			{
				CComPtr<IMgaFilter> spFilter;
				COMTHROW( m_spProject->CreateFilter( &spFilter ) );
				COMTHROW( spFilter->put_Kind( CComBSTR( "ConstraintFunc" ) ) );

				CComPtr<IMgaFCOs> spFCOs;
				COMTHROW( m_spProject->AllFCOs( spFilter, &spFCOs ) );

				MGACOLL_ITERATE( IMgaFCO, spFCOs ) {
					OclGme::SpConstraintFunction spFunction( new OclGme::ConstraintFunction( MGACOLL_ITER ) );
					spFunction->Register( m_pTreeManager );
					Ocl::Constraint::State eState = spFunction->ParseContext();
					if ( eState == Ocl::Constraint::CS_CTX_PARSE_SUCCEEDED ) {
						eState = spFunction->CheckContext();
						if ( eState == Ocl::Constraint::CS_CTX_CHECK_SUCCEEDED )
							vecFunctions.push_back( spFunction );
					}
				} MGACOLL_ITERATE_END;
			}

			TypeSeq CreateReturnType( const std::string& strType )
			{
				TypeSeq vecType;
				OclCommon::Convert( strType, vecType );
				return vecType;
			}
	};

//##############################################################################################################################################
//
//	C L A S S : OclGmeEC::Facade
//
//##############################################################################################################################################

	Facade::Facade()
		: m_pTreeManager( NULL ), m_spProject( NULL )
	{
	}

	Facade::~Facade()
	{
		Finalize();
		if ( m_pTreeManager )
			delete m_pTreeManager;
	}

	CComPtr<IMgaProject> Facade::GetProject() const
	{
		return m_spProject;
	}

	OclTree::TreeManager* Facade::GetTreeManager() const
	{
		return m_pTreeManager;
	}

	void Facade::Initialize( CComPtr<IMgaProject> spProject )
	{
		if ( m_pTreeManager )
			delete m_pTreeManager;

		m_spProject = spProject;

		m_bEnabled = true;
		m_bEnabledEvents = false;
		m_bEnabledInteractions = true;

		ConstraintDefinitionFactory* pCDFactory = new ConstraintDefinitionFactory();
		pCDFactory->m_spProject = spProject;

		OclMeta::TypeManager* pTypeManager = new OclMeta::TypeManager( new TypeFactory( spProject ), new OclBasic::OperatorFactory(), new OclImplementation::FunctionFactory(), pCDFactory );

		m_pTreeManager = new OclTree::TreeManager( pTypeManager, new OclTree::ObjectNodeAdaptor(), new OclTree::CollectionNodeAdaptor() );

		pCDFactory->m_pTreeManager = m_pTreeManager;
	}

	void Facade::Finalize()
	{
		if ( m_pTreeManager )
			m_pTreeManager->GetTypeManager()->ClearDynamicTypes();
	}

	bool Facade::IsConstraintDefined( CComPtr<IMgaFCO> spConstraint )
	{
		CString strField;

		COMTHROW( spConstraint->get_StrAttrByName( CComBSTR( "ConstraintEqn" ), PutOut( strField ) ) );
		if ( strField.IsEmpty() )
			return false;

		return true;
	}

	void Facade::CheckConstraint( CComPtr<IMgaFCO> spConstraint, OclGme::ConstraintVector& vecFaileds, bool bFieldsMandatory )
	{
		if ( ! IsConstraintDefined( spConstraint ) )
			return;

		FCOVector vecAssociated, vecFCOs, vecExamined;
		OclCommonEx::GetAssociationEnds( spConstraint, "src", "HasConstraint", vecAssociated );
		for ( unsigned int i = 0 ; i < vecAssociated.size() ; i++ )
			GetAllObjectsConsideredAsContext( vecAssociated[ i ].p, vecFCOs, vecExamined );

		for ( unsigned int i = 0 ; i < vecFCOs.size() ; i++ ) {
			std::string strType = OclCommonEx::GetObjectName( vecFCOs[ i ].p );
			if ( strType.empty() ) {
				char chNum[ 100 ];
				sprintf_s( chNum, "%u", i );
				strType = "UntitledClass_" + std::string( chNum );
			}
			OclGme::SpConstraint pConstraint( new OclGme::Constraint( strType, spConstraint, bFieldsMandatory ) );
			pConstraint->Register( m_pTreeManager );
			if ( pConstraint->GetState() == Ocl::Constraint::CS_DEFINED ) {
				Ocl::Constraint::State eState = pConstraint->Parse();
				if ( eState != Ocl::Constraint::CS_PARSE_SUCCEEDED )
					vecFaileds.push_back( pConstraint );
				else {
					OclTree::TypeContextStack context;
					context.AddVariable( "project", TypeSeq( 1, "gme::Project" ) );
					eState = pConstraint->Check( context );
					if ( eState != Ocl::Constraint::CS_CHECK_SUCCEEDED )
						vecFaileds.push_back( pConstraint );
				}
			}
		}

		if ( vecFCOs.empty() ) {
			OclGme::SpConstraint pConstraint( new OclGme::Constraint( "meta::RootFolder", spConstraint, bFieldsMandatory ) );
			pConstraint->Register( m_pTreeManager );
			if ( pConstraint->GetState() == Ocl::Constraint::CS_DEFINED ) {
				Ocl::Constraint::State eState = pConstraint->Parse();
				if ( eState != Ocl::Constraint::CS_PARSE_SUCCEEDED )
					vecFaileds.push_back( pConstraint );
				else {
					OclTree::TypeContextStack context;
					context.AddVariable( "project", TypeSeq( 1, "gme::Project" ) );
					eState = pConstraint->Check( context );
					if ( eState != Ocl::Constraint::CS_CHECK_SUCCEEDED )
						vecFaileds.push_back( pConstraint );
				}
			}
		}
	}

	void Facade::CheckAllConstraints( OclGme::ConstraintVector& vecFaileds )
	{
		CComPtr<IMgaFilter> spFilter;
		COMTHROW( m_spProject->CreateFilter( &spFilter ) );
		COMTHROW( spFilter->put_Kind( CComBSTR( "Constraint" ) ) );

		CComPtr<IMgaFCOs> spFCOs;
		COMTHROW( m_spProject->AllFCOs( spFilter, &spFCOs ) );

		MGACOLL_ITERATE( IMgaFCO, spFCOs ) {
			CheckConstraint( MGACOLL_ITER, vecFaileds, true );
		} MGACOLL_ITERATE_END;
	}

	bool Facade::IsConstraintFunctionDefined( CComPtr<IMgaFCO> spConstraintFunction )
	{
		CString strField;

		COMTHROW( spConstraintFunction->get_StrAttrByName( CComBSTR( "CFuncDefinition" ), PutOut( strField ) ) );
		if ( strField.IsEmpty() )
			return false;
		strField.Empty();

		COMTHROW( spConstraintFunction->get_StrAttrByName( CComBSTR( "CFuncContext" ), PutOut( strField ) ) );
		if ( strField.IsEmpty() )
			return false;
		strField.Empty();

		COMTHROW( spConstraintFunction->get_StrAttrByName( CComBSTR( "CFuncReturnType" ), PutOut( strField ) ) );
		if ( strField.IsEmpty() )
			return false;

		return true;
	}

	void Facade::CheckConstraintFunction( CComPtr<IMgaFCO> spConstraintFunction, OclGme::ConstraintFunctionVector& vecFaileds, bool bFieldsMandatory )
	{
		if ( ! IsConstraintFunctionDefined( spConstraintFunction ) )
			return;

		OclGme::SpConstraintFunction pConstraintFunction( new OclGme::ConstraintFunction( spConstraintFunction, bFieldsMandatory ) );
		pConstraintFunction->Register( m_pTreeManager );
		if ( pConstraintFunction->GetState() == Ocl::Constraint::CS_DEFINED ) {
			Ocl::Constraint::State eState = pConstraintFunction->Parse();
			if ( eState != Ocl::Constraint::CS_PARSE_SUCCEEDED )
				vecFaileds.push_back( pConstraintFunction );
			else {
				OclTree::TypeContextStack context;
				context.AddVariable( "project", TypeSeq( 1, "gme::Project" ) );
				eState = pConstraintFunction->Check( context );
				if ( eState != Ocl::Constraint::CS_CHECK_SUCCEEDED )
					vecFaileds.push_back( pConstraintFunction );
			}
		}
	}

	void Facade::CheckAllConstraintFunctions( OclGme::ConstraintFunctionVector& vecFaileds )
	{
		CComPtr<IMgaFilter> spFilter;
		COMTHROW( m_spProject->CreateFilter( &spFilter ) );
		COMTHROW( spFilter->put_Kind( CComBSTR( "ConstraintFunc" ) ) );

		CComPtr<IMgaFCOs> spFCOs;
		COMTHROW( m_spProject->AllFCOs( spFilter, &spFCOs ) );

		MGACOLL_ITERATE( IMgaFCO, spFCOs ) {
			CheckConstraintFunction( MGACOLL_ITER, vecFaileds, true );
		} MGACOLL_ITERATE_END;
	}

}; // namespace OclGmeEC
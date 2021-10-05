//###############################################################################################################################################
//
//	Object Constraint Language Generic Manager
//	OCLTree.cpp
//
//###############################################################################################################################################
#include "Solve4786.h"
#include "OCLTree.h"

#include "OCLObjectExBasic.h"
#include "OCLSignature.h"
#include "OCLFeature.h"
#include "OCLFeatureImplementation.h"
#include "OCLParserStatic.h"
#include "OCLException.h"

#define NILNAMESPACE ""
namespace OclTree
{
	#define EXCEPTION0( iCode, pos )	\
		OclCommon::Exception( OclCommon::Exception::ET_SEMANTIC, iCode, pos.iLine, pos.iColumn )

	#define EXCEPTION1( iCode, param1, pos ) \
		OclCommon::Exception( OclCommon::Exception::ET_SEMANTIC, iCode, param1, pos.iLine, pos.iColumn )

	#define EXCEPTION2( iCode, param1, param2, pos ) \
		OclCommon::Exception( OclCommon::Exception::ET_SEMANTIC, iCode, param1, param2, pos.iLine, pos.iColumn )

	#define ADDEX( ex ) \
	{ \
		OclCommon::Exception exp( ex ); \
		context.m_poolExceptions.Add( exp ); \
	}

	#define ADDEXP( ex ) \
		context.m_poolExceptions.Add( ex )

	#define POOLADDEX( ex_pool, ex ) \
	{ \
		OclCommon::Exception exp( ex ); \
		ex_pool.Add( exp ); \
	}

	#define SETEXPOS( ex, pos ) \
		ex.SetLine( pos.iLine ); ex.SetColumn( pos.iColumn );

	#define EVALTRY 	\
		try

	#define EVALCATCH( iLinee, strSigg ) 										\
		catch ( OclCommon::Exception ex ) {									\
			AddViolation( context, iLinee, strSigg, ex.GGetMessage() );		\
		}																		\
		catch ( char* ex ) {													\
			AddViolation( context, iLinee, strSigg, std::string( ex ) + " " ); 		\
		}																		\
		catch ( ... ) {															\
			AddViolation( context, iLinee, strSigg, "UNEX" ); 					\
		}

//##############################################################################################################################################
//
//	A B S T R A C T   C L A S S : OclTree::TreeNode
//
//==============================================================================================================================================
//
//	D E S C R I P T I O N :
//
//##############################################################################################################################################

	TreeNode::TreeNode( TreeManager* pManager, NodeKind eKind )
		: m_pManager( pManager ), m_eKind( eKind ), m_bTester( false ), m_bSelfTester( false )
	{
	}

	TreeNode::~TreeNode()
	{
	}

	TreeManager* TreeNode::GetTreeManager() const
	{
		return m_pManager;
	}

	TreeNode::NodeKind TreeNode::GetKind() const
	{
		return m_eKind;
	}

	bool TreeNode::ParseTypeSeq( TypeContext& context, const Position& position, std::string& strType, TypeSeq& vecType ) const
	{
		TypeSeq vecSavedType = vecType;
		int iRes = OclCommon::Convert( strType, vecType );
		if ( iRes > 0 ) {
			ADDEX( EXCEPTION1( EX_INVALID_TYPE, strType, position ) );
			return false;
		}
		bool bValid = true;
		for ( unsigned int i = 0 ; i < vecType.size() ; i++ ) {
			try {
				std::shared_ptr<OclMeta::Type> pType = m_pManager->GetTypeManager()->GetType( vecType[ i ], context.m_namespace);
				if ( i != vecType.size() - 1 ) {
					if ( ! pType->IsCompound() ) {
						ADDEX( EXCEPTION1( EX_TYPE_ISNT_COMPOUND, vecType[ i ], position ) );
						bValid = false;
					}
				}
				else {
					if ( pType->IsCompound() )
						vecType.push_back( "ocl::Any" );
				}
				if ( bValid )
					vecType[ i ] = pType->GetName();
			} catch ( OclCommon::Exception ex ) {
				SETEXPOS( ex, position );
				ADDEX( ex );
				bValid = false;
			}
		}
		if ( ! bValid )
			vecType = vecSavedType;
		else
			OclCommon::Convert( vecType, strType );
		return bValid;
	}

	bool TreeNode::CastType( TypeContext& context, const Position& position, const TypeSeq& vecTypeFrom, const TypeSeq& vecTypeTo ) const
	{
		OclMeta::TypeManager* pManager = m_pManager->GetTypeManager();
		if ( ! pManager->IsTypeA( vecTypeFrom, vecTypeTo ) && ! pManager ->IsTypeA( vecTypeTo, vecTypeFrom ) ) {
			std::string strCType, strSType;
			OclCommon::Convert( vecTypeFrom, strSType );
			OclCommon::Convert( vecTypeTo, strCType );
			ADDEX( EXCEPTION2( EX_CAST_TYPE_MISMATCH, strSType, strCType, position ) );
			return false;
		}
		return true;
	}

	TypeSeq TreeNode::GetParametralTypeSeq( const TypeSeq& vecType1, const TypeSeq& vecType2, const TypeSeq& vecTypeReturn )
	{
		m_vecType.clear();
		int i;

		for ( i = 0 ; i < (int) vecTypeReturn.size() - 1 ; i++ )
			m_vecType.push_back( vecTypeReturn[ i ] );

		std::string strLastType = vecTypeReturn[ vecTypeReturn.size() - 1 ];

		if ( strLastType == TYPE_AGGREGATED_OBJECT ) {
			for ( i = 1 ; i < (int) vecType1.size() ; i++ )
				m_vecType.push_back( vecType1[ i ] );
			return m_vecType;
		}

		if ( ! vecType2.empty() ) {

			if ( strLastType == TYPE_EXPRESSION_RETURN ) {
				for ( i = 0 ; i < (int) vecType2.size() ; i++ )
					m_vecType.push_back( vecType2[ i ] );
				return m_vecType;
			}

			if ( GetTreeManager()->GetTypeManager()->GetType( vecType1[ 0 ], NILNAMESPACE )->IsCompound() ) {

				if ( strLastType == TYPE_ARGUMENT_SELF_BASE || strLastType == TYPE_COMPOUNDARGUMENT_SELF_BASE ) {

					TypeSeq vecTypeS = vecType1;
					vecTypeS.erase( vecTypeS.begin() );
					TypeSeq vecTypeA = vecType2;

					if ( strLastType == TYPE_COMPOUNDARGUMENT_SELF_BASE && GetTreeManager()->GetTypeManager()->GetType( vecType2[ 0 ], NILNAMESPACE )->IsCompound() )
						vecTypeA.erase( vecTypeA.begin() );

					vecTypeS = GetTreeManager()->GetTypeManager()->GetTypeBase( vecTypeS, vecTypeA );
					for ( i = 0 ; i < (int) vecTypeS.size() ; i++ )
						m_vecType.push_back( vecTypeS[ i ] );

					return m_vecType;
				}
			}
		}

		m_vecType.push_back( strLastType );
		return m_vecType;
	}

	VariableNode* TreeNode::CreateThis( TypeContext& context, int iImplicitPos ) const
	{
		if ( iImplicitPos == -1 )
			iImplicitPos = context.m_vecImplicits.size() - 1;

		VariableNode* pVariableNode = GetTreeManager()->CreateVariable();
		pVariableNode->m_strName = context.m_vecImplicits[ iImplicitPos ];
		context.m_ctxTypes.GetVariable(  context.m_vecImplicits[ iImplicitPos ], pVariableNode->m_vecType );
		return pVariableNode;
	}

	int TreeNode::GetLastExceptionCode( TypeContext& context ) const
	{
		int iLastCode = context.m_poolExceptions.Size();
		return ( iLastCode == 0 ) ? -1 : context.m_poolExceptions.GetAt( context.m_poolExceptions.Size() - 1 ).GetCode();
	}

	OclMeta::Feature* TreeNode::CheckAmbiguity( const std::vector<OclMeta::Type*>& vecTypes, const std::vector<OclSignature::Feature*>& vecSignatures, std::vector<int>& vecAmbiguities, int& iPrecedence, OclCommon::ExceptionPool& exAmbiguity )
	{
		OclCommon::Exception exception;
		OclCommon::Exception exception2;
		OclMeta::Feature* feature1 = NULL;
		OclMeta::Feature* feature2 = NULL;
		try {
			switch ( vecSignatures[ 0 ]->GetKind() ) {
				case OclSignature::Feature::FK_OPERATOR 		: feature1 = m_pManager->GetTypeManager()->GetOperator( *( (OclSignature::Operator*)vecSignatures[ 0 ] ) ); break;
				case OclSignature::Feature::FK_FUNCTION 		: feature1 = m_pManager->GetTypeManager()->GetFunction( *( (OclSignature::Function*)vecSignatures[ 0 ] ) ); break;
				case OclSignature::Feature::FK_METHOD 			: feature1 = vecTypes[ 0 ]->GetMethod( *( (OclSignature::Method*)vecSignatures[ 0 ] ) ); break;
				case OclSignature::Feature::FK_ITERATOR 		: feature1 = ( ( OclMeta::CompoundType*) vecTypes[ 0 ] )->GetIterator(0, *( (OclSignature::Iterator*)vecSignatures[ 0 ] ) ); break;
				case OclSignature::Feature::FK_ATTRIBUTE 		: feature1 = vecTypes[ 0 ]->GetAttribute( *( (OclSignature::Attribute*)vecSignatures[ 0 ] ) ); break;
				case OclSignature::Feature::FK_ASSOCIATION 	: feature1 = vecTypes[ 0 ]->GetAssociation( *( (OclSignature::Association*)vecSignatures[ 0 ] ) ); break;
			}
			if ( iPrecedence == -1 )
				return feature1;
		}
		catch ( OclCommon::Exception ex ) {
			if ( iPrecedence == -1 )  {
				exAmbiguity.Add( ex );
				return NULL;
			}
			exception = ex;
		}
		try {
			switch ( vecSignatures[ 1 ]->GetKind() ) {
				case OclSignature::Feature::FK_OPERATOR 		: feature2 = m_pManager->GetTypeManager()->GetOperator( *( (OclSignature::Operator*)vecSignatures[ 1 ] ) ); break;
				case OclSignature::Feature::FK_FUNCTION 		: feature2 = m_pManager->GetTypeManager()->GetFunction( *( (OclSignature::Function*)vecSignatures[ 1 ] ) ); break;
				case OclSignature::Feature::FK_METHOD 			: feature2 = vecTypes[ 1 ]->GetMethod( *( (OclSignature::Method*)vecSignatures[ 1 ] ) ); break;
				case OclSignature::Feature::FK_ITERATOR 		: feature2 = ( ( OclMeta::CompoundType*) vecTypes[ 1 ] )->GetIterator(0, *( (OclSignature::Iterator*)vecSignatures[ 1 ] ) ); break;
				case OclSignature::Feature::FK_ATTRIBUTE 		: feature2 = vecTypes[ 1 ]->GetAttribute( *( (OclSignature::Attribute*)vecSignatures[ 1 ] ) ); break;
				case OclSignature::Feature::FK_ASSOCIATION 	: feature2 = vecTypes[ 1 ]->GetAssociation( *( (OclSignature::Association*)vecSignatures[ 1 ] ) ); break;
			}
			if ( iPrecedence == 1 )
				return feature2;

			vecAmbiguities[ 1 ] = 0;
			if ( feature1 || ! feature1 && exception.GetCode() == vecAmbiguities[ 0 ] ) {
				if ( feature1 )
					vecAmbiguities[ 0 ] = 0;
				POOLADDEX(exAmbiguity, OclCommon::Exception( OclCommon::Exception::ET_SEMANTIC, vecAmbiguities[ 2 ], vecSignatures[ 0 ]->Print(), vecSignatures[ 1 ]->Print() ) );
				return NULL;
			}
			iPrecedence = 1;
			return feature2;
		}
		catch ( OclCommon::Exception ex ) {
			if ( iPrecedence == 1 )  {
				exAmbiguity.Add( ex );
				return NULL;
			}

			if ( ex.GetCode() == vecAmbiguities[ 1 ] ) {
				if ( feature1 || ! feature1 && exception.GetCode() == vecAmbiguities[ 0 ] ) {
					if ( feature1 )
						vecAmbiguities[ 0 ] = 0;
					POOLADDEX(exAmbiguity, OclCommon::Exception( OclCommon::Exception::ET_SEMANTIC, vecAmbiguities[ 2 ], vecSignatures[ 0 ]->Print(), vecSignatures[ 1 ]->Print() ) );
					return NULL;
				}
				else {
					vecAmbiguities[ 0 ] = exception.GetCode();
					exAmbiguity.Add( ex );
					return NULL;
				}
			}
			else {
				vecAmbiguities[ 1 ] = ex.GetCode();
				if ( feature1 ) {
					vecAmbiguities[ 0 ] = 0;
					iPrecedence = -1;
					return feature1;
				}
				else {
					exAmbiguity.Add( exception );
					if ( exception.GetCode() != vecAmbiguities[ 0 ] )
						exAmbiguity.Add( ex );
					vecAmbiguities[ 0 ] = exception.GetCode();
					return NULL;
				}
			}
		}
	}

	void TreeNode::AddViolation( ObjectContext& context, int iLine, const std::string& strSignature, const std::string& strMessage )
	{
		if ( ! context.m_bEnableTracking )
			return;

		Violation violation;

		violation.bIsException = ! strMessage.empty();
		violation.position.iLine = iLine;
		if ( strMessage.empty() )
			violation.strMessage = "Expression evaluated to false.";
		else
			if ( strMessage == "UNEX" )
				violation.strMessage = "Unexpected exception occurred.";
			else
				violation.strMessage = strMessage;
		violation.strSignature = strSignature;

		ObjectContextStack::StateItemVector vecItems = context.oCtx.GetState();
		for ( unsigned int i = 0 ; i < vecItems.size() ; i++ ) {
//			ObjectContextStack::StateItem itemx = vecItems[i];
			string nam = vecItems[ i ].name;
			violation.vecVariables.push_back( nam );
			string str = vecItems[ i ].item.Print();
			violation.vecObjects.push_back( str );
			IUnknown* iu;
			iu = vecItems[ i ].item.GetObject();
			violation.vecObjectsPtr.push_back(iu);
		}
		context.vecViolations.push_back( violation );
		context.iViolationCount++;

		if ( violation.bIsException )
			context.m_bHasException;
	}

	// terge
	void TreeNode::AddViolation(ObjectContext& context, Violation &violation)
	{
		context.vecViolations.push_back( violation );
		context.iViolationCount++;

		if ( violation.bIsException )
			context.m_bHasException;
	}

	bool TreeNode::IsBooleanReturned()
	{
		return GetTreeManager()->GetTypeManager()->IsTypeA( m_vecType[ 0 ], "ocl::Boolean" ) >= 0;
	}

	OclMeta::Object TreeNode::CheckFalseResult( ObjectContext& context, OclMeta::Object spObject, int iLine, const std::string& strSignature )
	{
		if ( spObject.IsUndefined() )
			return spObject;
		if ( IsBooleanReturned() && context.bDoSnapshot ) {
			DECL_BOOLEAN( bResult, spObject );
			if ( ! bResult )
				AddViolation( context, iLine, strSignature );
		}
		return spObject;
	}

	OclMeta::Object TreeNode::EvaluateCast( ObjectContext& context, OclMeta::Object spObject, int iLine, const std::string& strSignature, const std::string& strTypeName, bool bOnlyTest )
	{
		std::string strTypeName2 = ( strTypeName.empty() ) ? m_vecType[ 0 ] : strTypeName;
		if ( spObject.IsUndefined() )
			return spObject;
		EVALTRY {
			if ( GetTreeManager()->GetTypeManager()->IsTypeA( spObject.GetTypeName(), strTypeName2 ) >= 0 ) {
				if ( ! bOnlyTest ) {
					OclMeta::Object spNewObject( spObject );
					spNewObject.SetStaticTypeName( strTypeName2 );
					return spNewObject;
				}
				return spObject;
			}
			AddViolation( context, iLine, strSignature, "Object is not instance of type [ " + strTypeName2 + " ]." );
			return OclMeta::Object::UNDEFINED;
		} EVALCATCH( iLine, "At casting object: " + strSignature );
		return OclMeta::Object::UNDEFINED;
	}

	void TreeNode::CheckInitialize()
	{
		m_vecType.clear();
		m_bSelfTester = false;
	}

	bool TreeNode::Check( TypeContext& context )
	{
		CheckInitialize();
		return CheckImplementation( context );
	}

//##############################################################################################################################################
//
//	C L A S S : OclNodes::ObjectNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################

	ObjectNode::ObjectNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_OBJECT ), m_strType( "" ), m_strValue( "" )
	{
	}

	std::string ObjectNode::Print( const std::string& strTabs ) const
	{
		return GetTreeManager()->m_pOAdaptor->Print( strTabs, (ObjectNode*)this );
	}

	bool ObjectNode::CheckImplementation( TypeContext& context )
	{
		return GetTreeManager()->m_pOAdaptor->Check( context, (ObjectNode*)this );
	}

	OclMeta::Object ObjectNode::Evaluate( ObjectContext& context )
	{
		OclMeta::Object spResult = GetTreeManager()->m_pOAdaptor->Evaluate( context, (ObjectNode*)this );
		spResult = EvaluateCast( context, spResult, m_mapPositions[ LID_NODE_START ].iLine, "At creating object: " + m_vecType[ 0 ] );
		return CheckFalseResult( context, spResult, m_mapPositions[ LID_NODE_START ].iLine, "At creating object: " + m_vecType[ 0 ] );
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::CollectionNode <<< + OclTree::TreeNode
//
//==============================================================================================================================================
//
//	D E S C R I P T I O N :
//
//##############################################################################################################################################

	CollectionNode::CollectionNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_COLLECTION ), m_strType( "" )
	{
	}

	CollectionNode::~CollectionNode()
	{
		if ( ! m_bTester && ! m_bSelfTester )
			for ( unsigned int i = 0; i < m_vecNodes.size() ; i++ )
				delete m_vecNodes[ i ];
	}

	std::string CollectionNode::Print( const std::string& strTabs ) const
	{
		return GetTreeManager()->m_pCAdaptor->Print( strTabs, (CollectionNode*)this );
	}

	bool CollectionNode::CheckImplementation( TypeContext& context )
	{
		return GetTreeManager()->m_pCAdaptor->Check( context, (CollectionNode*)this );
	}

	OclMeta::Object CollectionNode::Evaluate( ObjectContext& context )
	{
		OclMeta::Object spResult = GetTreeManager()->m_pCAdaptor->Evaluate( context, (CollectionNode*)this );
		spResult = EvaluateCast( context, spResult, m_mapPositions[ LID_NODE_START ].iLine, "At creating collection: " + m_vecType[ 0 ] );
		return CheckFalseResult( context, spResult, m_mapPositions[ LID_NODE_START ].iLine, "At creating collection: " + m_vecType[ 0 ] );
	}

//##############################################################################################################################################
//
//	C L A S S : OclNodes::DeclarationNode <<< + OclNodes::SyntaxTreeNode
//
//##############################################################################################################################################

	DeclarationNode::DeclarationNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_DECLARATION ), m_strName( "" ), m_strType( "" ), m_pValueNode( NULL ), m_pInnerNode( NULL )
	{
	}

	DeclarationNode::~DeclarationNode()
	{
		if ( ! m_bTester && ! m_bSelfTester ) {
			if ( m_pInnerNode )
				delete m_pInnerNode;
			if ( m_pValueNode )
				delete m_pValueNode;
		}
	}

	std::string DeclarationNode::Print( const std::string& strTabs ) const
	{
		std::string strOut = strTabs + "<Declaration name=\"" + m_strName + "\" type=\"" + m_strType + "\">\r\n";

		strOut += strTabs + TABSTR + "<Variable>\r\n";
		strOut += m_pValueNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</Variable>\r\n";

		strOut += strTabs + TABSTR + "<Expression>\r\n";
		strOut += m_pInnerNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</Expression>\r\n";

		strOut += strTabs + "</Declaration>\r\n";
		return strOut;
	}

	void DeclarationNode::CheckInitialize()
	{
		TreeNode::CheckInitialize();
		m_vecTypeDecl.clear();
	}

	bool DeclarationNode::CheckImplementation( TypeContext& context )
	{
		// Check Expression of Declaration

		if ( m_pValueNode->Check( context ) )
			m_vecTypeDecl = m_pValueNode->m_vecType;

		// Check explicit type of variable if it exists

		if ( ! m_strType.empty() )
			if ( ParseTypeSeq( context, m_mapPositions[ LID_VARIABLE_TYPE ], m_strType, m_vecTypeDecl ) )
				if ( ! m_pValueNode->m_vecType.empty() )
					if ( ! CastType( context, m_mapPositions[ LID_VARIABLE_TYPE ], m_pValueNode->m_vecType, m_vecTypeDecl ) )
						m_vecTypeDecl.clear();


		// Check if variable already exists , Add variable

		if ( context.m_ctxTypes.ExistsVariable( m_strName ) )
			ADDEX( EXCEPTION1( EX_VARIABLE_ALREADY_EXISTS, m_strName, m_mapPositions[ LID_VARIABLE_NAME ] ) )
		else
			if ( ! m_vecTypeDecl.empty() )
				context.m_ctxTypes.AddVariable( m_strName, m_vecTypeDecl, true );

		// Check Expression

		if ( m_pInnerNode->Check( context ) )
			m_vecType = m_pInnerNode->m_vecType;

		// Remove Variable

		if ( ! m_vecTypeDecl.empty() )
			context.m_ctxTypes.RemoveVariable( m_strName );

		return ! m_vecType.empty() && ! m_pValueNode->m_vecType.empty();
	}

	OclMeta::Object DeclarationNode::Evaluate( ObjectContext& context )
	{
		OclMeta::Object spObject = EvaluateCast( context, m_pValueNode->Evaluate( context ), m_mapPositions[ LID_NODE_START ].iLine, "let " + m_strName + " : " + m_vecTypeDecl[ 0 ] + " = ...", m_vecTypeDecl[ 0 ], m_strType.empty() );

		context.oCtx.AddVariable( m_strName, spObject, true );
		OclMeta::Object spResult = m_pInnerNode->Evaluate( context );
		context.oCtx.RemoveVariable( m_strName );

		return spResult;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::TypeCastNode <<< + OclTree::TreeNode
//
//==============================================================================================================================================
//
//	D E S C R I P T I O N :
//
//##############################################################################################################################################

	TypeCastNode::TypeCastNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_TYPECAST ), m_pThisNode( NULL ), m_strType( "" ), m_bIsDynamic( false )
	{
	}

	TypeCastNode::~TypeCastNode()
	{
		if ( ! m_bTester )
			if ( m_pThisNode )
				delete m_pThisNode;
	}

	std::string TypeCastNode::Print( const std::string& strTabs ) const
	{
		std::string strOut = strTabs + "<TypeCast type=\"" + m_strType + "\" dynamic=\"" + ( ( m_bIsDynamic ) ? "true" : "false" ) + "\">\r\n";

		strOut += strTabs + TABSTR + "<This>\r\n";
		if ( m_pThisNode )
			strOut += m_pThisNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</This>\r\n";

		strOut += strTabs + "</TypeCast>\r\n";
		return strOut;
	}

	void TypeCastNode::CheckInitialize()
	{
		if ( m_bSelfTester && m_pThisNode ) {
			delete m_pThisNode;
			m_pThisNode = NULL;
		}
		TreeNode::CheckInitialize();
	}

	bool TypeCastNode::CheckImplementation( TypeContext& context )
	{
		bool bThisValid = true;
		if ( ! m_pThisNode ) {
			if ( context.m_vecImplicits.empty() )
				return false;
			m_bSelfTester = true;
			m_pThisNode = CreateThis( context, -1 );
		}
		else
			bThisValid = m_pThisNode->Check( context );

		if ( bThisValid ) {

			// Check if Object Node is callable

			if ( m_pThisNode->GetKind() == TreeNode::NK_OBJECT ) {
				ObjectNode* pONode = (ObjectNode*) m_pThisNode;
				if ( ! pONode->m_bCallable )
					ADDEX( EXCEPTION1( EX_OBJECT_CALL_PROHIBITED, m_pThisNode->m_vecType[ 0 ], m_mapPositions[ LID_CALL_OPERATOR ] ) );
			}

			if ( ParseTypeSeq( context, m_mapPositions[ LID_FEATURE_NAME ], m_strType, m_vecType ) ) {
				if ( ! CastType( context, m_mapPositions[ LID_FEATURE_NAME ], m_pThisNode->m_vecType, m_vecType ) ) {
					m_vecType.clear();
					return false;
				}
				return true;
			}
		}
		return false;
	}

	OclMeta::Object TypeCastNode::Evaluate( ObjectContext& context )
	{
		return EvaluateCast( context, m_pThisNode->Evaluate( context ), m_mapPositions[ LID_FEATURE_NAME ].iLine, "ocl::Any::oclAsType( ocl::Type )" , m_vecType[ 0 ], false );
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::IfThenElseNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################

	IfThenElseNode::IfThenElseNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_IFTHENELSE ), m_pIfNode( NULL ), m_pThenNode( NULL ), m_pElseNode( NULL )
	{
	}

	IfThenElseNode::~IfThenElseNode()
	{
		if ( ! m_bTester && ! m_bSelfTester ) {
			if ( m_pIfNode )
				delete m_pIfNode;
			if ( m_pThenNode )
				delete m_pThenNode;
			if ( m_pElseNode )
				delete m_pElseNode;
		}
	}

	std::string IfThenElseNode::Print( const std::string& strTabs ) const
	{
		std::string strOut = strTabs + "<IfThenElse>\r\n";

		strOut += strTabs + TABSTR + "<IfExpression>\r\n";
		strOut += m_pIfNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</IfExpression>\r\n";

		strOut += strTabs + TABSTR + "<ThenExpression>\r\n";
		strOut += m_pThenNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</ThenExpression>\r\n";

		strOut += strTabs + TABSTR + "<ElseExpression>\r\n";
		strOut += m_pElseNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</ElseExpression>\r\n";

		strOut += strTabs + "</IfThenElse>\r\n";
		return strOut;
	}

	bool IfThenElseNode::CheckImplementation( TypeContext& context )
	{
		// Check if IF Expression returns kind of ocl::Boolean

		bool bIfValid = m_pIfNode->Check( context );
		if ( bIfValid ) {
			TypeSeq vecTypeB( 1, "ocl::Boolean" );
			bIfValid = GetTreeManager()->GetTypeManager()->IsTypeA( m_pIfNode->m_vecType, vecTypeB );
			if ( ! bIfValid )
				ADDEX( EXCEPTION0( EX_BOOLEAN_REQUIRED, m_pIfNode->m_mapPositions[ LID_NODE_START ] ) );
		}

		// Check THEN and ELSE Expressions

		bool bThenElseValid = m_pThenNode->Check( context );
		bThenElseValid = m_pElseNode->Check( context ) && bThenElseValid;

		// Check whether these Expression returns the same type

		if ( bThenElseValid ) {
			if ( ! ( m_pThenNode->m_vecType[ 0 ] == m_pElseNode->m_vecType[ 0 ] ) ) {
				std::string strType1, strType2;
				OclCommon::Convert( m_pThenNode->m_vecType, strType1 );
				OclCommon::Convert( m_pElseNode->m_vecType, strType2 );
				ADDEX( EXCEPTION2( EX_IF_TYPE_MISMATCH, strType1, strType2, m_mapPositions[ LID_NODE_START ] ) );
				return false;
			}
			if ( bIfValid ) {
				m_vecType = GetTreeManager()->GetTypeManager()->GetTypeBase( m_pThenNode->m_vecType, m_pElseNode->m_vecType );
				return true;
			}
		}
		return false;
	}

	OclMeta::Object IfThenElseNode::Evaluate( ObjectContext& context )
	{
		OclMeta::Object spCondition = m_pIfNode->Evaluate( context );
		if ( spCondition.IsUndefined() )
			return spCondition;

		DECL_BOOLEAN( bCondition, spCondition );
		if ( bCondition )
			return m_pThenNode->Evaluate( context );
		else
			return m_pElseNode->Evaluate( context );
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::OperatorNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################

	OperatorNode::OperatorNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_OPERATOR ), m_pOperandNode1( NULL ), m_pOperandNode2( NULL ), m_strName( "" )
	{
	}

	OperatorNode::~OperatorNode()
	{
		if ( ! m_bTester && ! m_bSelfTester ) {
			if ( m_pOperandNode1 )
				delete m_pOperandNode1;
			if ( m_pOperandNode2 )
				delete m_pOperandNode2;
		}
	}

	std::string OperatorNode::Print( const std::string& strTabs ) const
	{
		std::string strOut = strTabs + "<Operator name=\"" + m_strName + "\">\r\n";

		strOut += strTabs + TABSTR + "<Operand1>\r\n";
		strOut += m_pOperandNode1->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</Operand1>\r\n";

		if ( m_pOperandNode2 ) {
			strOut += strTabs + TABSTR + "<Operand2>\r\n";
			strOut += m_pOperandNode2->Print( strTabs + TABSTR + TABSTR );
			strOut += strTabs + TABSTR + "</Operand2>\r\n";
		}

		strOut += strTabs + "</Operator>\r\n";
		return strOut;
	}

	void OperatorNode::CheckInitialize()
	{
		TreeNode::CheckInitialize();
		m_strAssignVarName = "";
	}

	bool OperatorNode::CheckImplementation( TypeContext& context )
	{
		bool bIsValid = m_pOperandNode1->Check( context );
		if ( m_pOperandNode2 )
			bIsValid = m_pOperandNode2->Check( context ) && bIsValid;

		if ( bIsValid ) {

			/*
				This Feature is commented for the time being

			// Assignment operator check

			if ( m_strName == "=" && m_pOperandNode2 && m_pOperandNode1->GetKind() == TreeNode::NK_VARIABLE ) {
				VariableNode* pVariable = ( VariableNode* ) m_pOperandNode1;
				bool bAssignable = false;
				context.m_ctxTypes.IsAssignable( pVariable->m_strName, bAssignable );
				if ( bAssignable ) {
					m_strAssignVarName = pVariable->m_strName;
					if ( ! GetTreeManager()->GetTypeManager()->IsTypeA( m_pOperandNode2->m_vecType, m_pOperandNode1->m_vecType ) ) {
						std::string strVarType, strValType;
						OclCommon::Convert( m_pOperandNode1->m_vecType , strVarType );
						OclCommon::Convert( m_pOperandNode2->m_vecType , strValType );
						ADDEX( EXCEPTION2( EX_ASSIGNMENT_TYPE_MISMATCH, strVarType, strValType, GetPos( LID_NODE_START ) ) );
						return false;
					}
					m_vecType = m_pOperandNode1->m_vecType;
					return true;
				}
			}
			*/

			// Any other operator check

			OclMeta::Operator* pOperator = NULL;
			if ( ! m_pOperandNode2 ) {
				try {
					pOperator = GetTreeManager()->GetTypeManager()->GetOperator( OclSignature::Operator( m_strName, m_pOperandNode1->m_vecType[ 0 ] ) );
					GetParametralTypeSeq( m_pOperandNode1->m_vecType, TypeSeq(), pOperator->GetReturnTypeSeq() );
					return true;
				}
				catch ( OclCommon::Exception ex ) {
					SETEXPOS( ex, m_mapPositions[ LID_FEATURE_NAME ] );
					ADDEX( ex );
					return false;
				}
			}
			try {
				pOperator = GetTreeManager()->GetTypeManager()->GetOperator( OclSignature::Operator( m_strName, m_pOperandNode1->m_vecType[ 0 ], m_pOperandNode2->m_vecType[ 0 ] ) );
				GetParametralTypeSeq( m_pOperandNode1->m_vecType, m_pOperandNode1->m_vecType, pOperator->GetReturnTypeSeq() );
				return true;
			}
			catch ( OclCommon::Exception ex ) {
				SETEXPOS( ex, m_mapPositions[ LID_FEATURE_NAME ] );
				ADDEX( ex );
				return false;
			}
		}
		return false;
	}

	OclMeta::Object OperatorNode::Evaluate( ObjectContext& context )
	{
		OclSignature::Operator signature = ( ! m_pOperandNode2 ) ? OclSignature::Operator( m_strName, m_pOperandNode1->m_vecType[ 0 ] ) : OclSignature::Operator( m_strName, m_pOperandNode1->m_vecType[ 0 ], m_pOperandNode2->m_vecType[ 0 ] );
		int iLineFeatureName = m_mapPositions[ LID_FEATURE_NAME ].iLine;

		// Evaluation of Logical Operators

/*		if ( m_pOperandNode1->IsBooleanReturned() && m_pOperandNode2 && m_pOperandNode2->IsBooleanReturned() && m_vecType[ 0 ] == "ocl::Boolean" ) {

			if ( m_strName == "and" || m_strName == "&&" ) {
				OclMeta::Object spOperand1 = m_pOperandNode1->Evaluate( context );
				if ( spOperand1.IsUndefined() ) {
					if ( m_strName == "and" || ! context.m_bShortCircuitLogical )
						m_pOperandNode2->Evaluate( context );
					return spOperand1;
				}
				DECL_BOOLEAN( bOperand1, spOperand1 );
				if ( ! bOperand1 ) {
					if ( m_strName == "and" || ! context.m_bShortCircuitLogical ) {
						OclMeta::Object spOperand2 = m_pOperandNode2->Evaluate( context );
						if ( spOperand2.IsUndefined() )
							return spOperand2;
					}
					return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), false ), iLineFeatureName, signature.Print() );
				}
				OclMeta::Object spOperand2 = m_pOperandNode2->Evaluate( context );
				if ( spOperand2.IsUndefined() )
					return spOperand2;
				DECL_BOOLEAN( bOperand2, spOperand2 );
				return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), bOperand2 ), iLineFeatureName, signature.Print() );
			}*/

		// undef and/&& false = false and/&& undef = false
		// undef and/&& anything = undef
		if ( m_pOperandNode1->IsBooleanReturned() && m_pOperandNode2 && m_pOperandNode2->IsBooleanReturned() && m_vecType[ 0 ] == "ocl::Boolean" ) {

			if ( m_strName == "and" || m_strName == "&&" ) {
				OclMeta::Object spOperand1 = m_pOperandNode1->Evaluate( context );
				if ( ! spOperand1.IsUndefined() ) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					if ( ! bOperand1 ) {
						if ( m_strName == "&&" || context.m_bShortCircuitLogical )
							return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), false ), iLineFeatureName, signature.Print() );
					}
				}
				OclMeta::Object spOperand2 = m_pOperandNode2->Evaluate( context );
				if ( ! spOperand1.IsUndefined()  &&  ! spOperand2.IsUndefined()) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					DECL_BOOLEAN( bOperand2, spOperand2 );
					return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), bOperand1 && bOperand2 ), iLineFeatureName, signature.Print() );
				}
				// cannot create UNDEFINED result
				else if (spOperand1.IsUndefined()  &&  spOperand2.IsUndefined()) {
					return spOperand1;
				}
				else if (spOperand1.IsUndefined()) {
					DECL_BOOLEAN( bOperand2, spOperand2 );
					if (bOperand2)
						return spOperand1;
					else
						return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), false ), iLineFeatureName, signature.Print() );
				}
				else if (spOperand2.IsUndefined()) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					if (bOperand1)
						return spOperand2;
					else
						return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), false ), iLineFeatureName, signature.Print() );
				}
			}


/*			if ( m_strName == "or" || m_strName == "||" ) {
				OclMeta::Object spOperand1 = m_pOperandNode1->Evaluate( context );
				if ( ! spOperand1.IsUndefined() ) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					if ( bOperand1 ) {
						if ( m_strName == "or" || ! context.m_bShortCircuitLogical )
							m_pOperandNode2->Evaluate( context );
						return CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true );
					}
				}
				OclMeta::Object spOperand2 = m_pOperandNode2->Evaluate( context );
				if ( spOperand2.IsUndefined() ) {
					if ( spOperand1.IsUndefined() )
						return spOperand1;
					return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), false ), iLineFeatureName, signature.Print() );
				}
				DECL_BOOLEAN( bOperand2, spOperand2 );
				return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), bOperand2 ), iLineFeatureName, signature.Print() );
			}*/

			// true OR/|| UNDEF = UNDEF OR/|| true = true
			// UNDEF OR/|| anything = UNDEF
			if ( m_strName == "or" || m_strName == "||" ) {
				OclMeta::Object spOperand1 = m_pOperandNode1->Evaluate( context );
				if ( ! spOperand1.IsUndefined() ) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					if ( bOperand1 ) {
						if ( m_strName == "||"  ||  context.m_bShortCircuitLogical )
							return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true ), iLineFeatureName, signature.Print() );
					}
				}
				OclMeta::Object spOperand2 = m_pOperandNode2->Evaluate( context );
				if ( ! spOperand1.IsUndefined()  &&  ! spOperand2.IsUndefined()) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					DECL_BOOLEAN( bOperand2, spOperand2 );
					return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), bOperand1 || bOperand2 ), iLineFeatureName, signature.Print() );
				}
				else if (spOperand1.IsUndefined()  &&  spOperand2.IsUndefined() )
					return spOperand1;
				else if (spOperand1.IsUndefined()) {
					DECL_BOOLEAN( bOperand2, spOperand2 );
					if (!bOperand2)
						return spOperand1;
					else
						return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true ), iLineFeatureName, signature.Print() );
				}
				else if (spOperand2.IsUndefined()) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					if (!bOperand1)
						return spOperand2;
					else
						return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true ), iLineFeatureName, signature.Print() );
				}
			}



/*			if ( m_strName == "implies" || m_strName == "=>" ) {
				OclMeta::Object spOperand1 = m_pOperandNode1->Evaluate( context );
				if ( spOperand1.IsUndefined() )
					return spOperand1;
				DECL_BOOLEAN( bOperand1, spOperand1 );
				if ( ! bOperand1 ) {
					if ( m_strName == "implies" || ! context.m_bShortCircuitLogical )
						m_pOperandNode2->Evaluate( context );
					return CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true );
				}
				OclMeta::Object spOperand2 = m_pOperandNode2->Evaluate( context );
				if ( spOperand2.IsUndefined() )
					return spOperand2;
				DECL_BOOLEAN( bOperand2, spOperand2 );
				return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), bOperand2 ), iLineFeatureName, signature.Print() );
			}
		}*/

			// FALSE =>/implies anything - true
			// anything =>/implies TRUE - true
			if ( m_strName == "implies" || m_strName == "=>" ) {
				OclMeta::Object spOperand1 = m_pOperandNode1->Evaluate( context );
				if ( !spOperand1.IsUndefined() ) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					if (! bOperand1) {
						if ( m_strName == "=>" || context.m_bShortCircuitLogical )
							return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true ), iLineFeatureName, signature.Print() );
					}
				}
				OclMeta::Object spOperand2 = m_pOperandNode2->Evaluate( context );
				if ( ! spOperand1.IsUndefined()  &&  ! spOperand2.IsUndefined()) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					DECL_BOOLEAN( bOperand2, spOperand2 );
					return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), !bOperand1  ||  bOperand2), iLineFeatureName, signature.Print() );
				}
				else if (spOperand1.IsUndefined()  &&  spOperand2.IsUndefined() )
					return spOperand1;
				else if (spOperand1.IsUndefined()) {
					DECL_BOOLEAN( bOperand2, spOperand2 );
					if (!bOperand2)
						return spOperand1;
					else
						return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true), iLineFeatureName, signature.Print() );
				}
				else if (spOperand2.IsUndefined()) {
					DECL_BOOLEAN( bOperand1, spOperand1 );
					if (bOperand1)
						return spOperand2;
					else
						return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true ), iLineFeatureName, signature.Print() );
				}
			}
		}

		OclMeta::Object spOperand1 = m_pOperandNode1->Evaluate( context );
		OclMeta::Object spOperand2;

		/*
		// Evaluation of Assignment

		if ( ! m_strAssignVarName.empty() ) {
			operand2 = EvaluateAndCast( context, m_pOperandNode2, LID_NODE_START, signature.Print(), m_pOperandNode1->m_vecType[ 0 ] );
			context.oCtx.SetVariable( m_strAssignVarName, operand2 );
			if ( bIsBoolean && ! operand2.IsUndefined() && context.bDoSnapshot ) {
				DECL_BOOLEAN( bResult, operand2 );
				if ( ! bResult )
					AddViolation( context, false, LID_FEATURE_NAME, signature.Print() );
			}
			return operand2;
		}
		*/

		// Evaluation of Equality and non equality

		if ( m_pOperandNode2 )
			spOperand2 = m_pOperandNode2->Evaluate( context );

		if ( ( m_strName == "==" || m_strName == "=" ) && m_pOperandNode2 && m_vecType[ 0 ] == "ocl::Boolean" ) {
			if ( spOperand1.IsUndefined() )
				if ( spOperand2.IsUndefined() )
					return CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true );
				else
					CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), false ), iLineFeatureName, signature.Print() );
			else
				if ( spOperand2.IsUndefined() )
					CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), false ), iLineFeatureName, signature.Print() );
		}

		if ( ( m_strName == "!=" || m_strName == "<>" ) && m_pOperandNode2 && m_vecType[ 0 ] == "ocl::Boolean" ) {
			if ( spOperand1.IsUndefined() )
				if ( spOperand2.IsUndefined() )
					CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), false ), iLineFeatureName, signature.Print() );
				else
					return CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true );
			else
				if ( spOperand2.IsUndefined() )
					return CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), true );
		}

		// Evaluation of any kind of operators

		if ( spOperand1.IsUndefined() || m_pOperandNode2 && spOperand2.IsUndefined() )
			return OclMeta::Object::UNDEFINED;

		EVALTRY {
			OclMeta::Operator* pOperator = GetTreeManager()->GetTypeManager()->GetOperator( signature );
			OclImplementation::Operator* pIOperator = pOperator->GetImplementation();
			if ( ! pIOperator )
				THROWOCL0( ET_RUNTIME, EX_OPERATOR_NOT_IMPLEMENTED );

			OclMeta::ObjectVector vecOperands( 1, spOperand1 );
			if ( m_pOperandNode2 )
				vecOperands.push_back( spOperand2 );
			pIOperator->SetArguments( vecOperands );
			pIOperator->Initialize();

			(*pIOperator)();

			OclMeta::Object spResult = EvaluateCast( context, pIOperator->GetResult(), iLineFeatureName, signature.Print() );
			pIOperator->Finalize();

			return CheckFalseResult( context, spResult, iLineFeatureName, signature.Print() );
		} EVALCATCH( iLineFeatureName, signature.Print() );
		return OclMeta::Object::UNDEFINED;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::IteratorNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################
	int IteratorNode::m_iteratorLevel = 0;

	IteratorNode::IteratorNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_ITERATOR ), m_strName( "" ), m_pThisNode( NULL ), m_pArgumentNode( NULL ), m_strDeclType( "" ), m_strCallOperator( "" ), m_strAccuName( "" ), m_strAccuType( "" ), m_pAccuNode( NULL )
	{
	}

	IteratorNode::~IteratorNode()
	{
		if ( ! m_bTester ) {
			if ( m_pThisNode )
				delete m_pThisNode;
			if ( m_pArgumentNode )
				delete m_pArgumentNode;
			if ( m_pAccuNode )
				delete m_pAccuNode;
		}
	}

	std::string IteratorNode::Print( const std::string& strTabs ) const
	{
		std::string strOut = strTabs + "<Iterator name=\"" + m_strName + "\">\r\n";

		if ( m_vecDeclarators.size() > 0 ) {
			strOut += strTabs + TABSTR + "<Declarators type=\"" + m_strDeclType + "\">\r\n" + strTabs + TABSTR + TABSTR;
			for ( unsigned int i = 0 ; i < m_vecDeclarators.size() ; i++ )
				strOut += m_vecDeclarators[ i ] + " ";
			strOut += "\r\n" + strTabs + TABSTR + "</Declarators>\r\n";
		}

		if ( m_pAccuNode ) {
			strOut += strTabs + TABSTR + "<Accumulator type=\"" + m_strAccuType + "\">\r\n";
			strOut += m_pAccuNode->Print( strTabs + TABSTR + TABSTR );
			strOut += strTabs + TABSTR + "</Accumulator>\r\n";
		}

		strOut += strTabs + TABSTR + "<This>\r\n";
		if ( m_pThisNode )
			strOut += m_pThisNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</This>\r\n";

		strOut += strTabs + TABSTR + "<Argument>\r\n";
		strOut += m_pArgumentNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</Argument>\r\n";

		strOut += strTabs + "</Method>\r\n";

		return strOut;
	}

	void IteratorNode::CheckInitialize()
	{
		if ( m_bSelfTester && ! m_bTester && m_pThisNode ) {
			delete m_pThisNode;
			m_pThisNode = NULL;
		}
		TreeNode::CheckInitialize();
		if ( ! m_vecDeclarators.empty() && m_vecDeclarators[ 0 ].substr( 0, 1 ) == "!" )
			m_vecDeclarators.clear();
	}

	bool IteratorNode::CheckImplementation( TypeContext& context )
	{
		// Iterator cannot be called by .

		if ( m_strCallOperator == "." )
			ADDEX( EXCEPTION1( EX_INVALID_CALL, m_strCallOperator, m_mapPositions[ LID_CALL_OPERATOR ] ) );

		// Check This if exists

		if ( m_pThisNode ) {
			bool bThisValid = m_bTester || m_pThisNode->Check( context );
			if ( bThisValid )
				return CheckIterator( context );
			return false;
		}

		m_bSelfTester = true;

		// Check Implicits variables

		for ( int i = context.m_vecImplicits.size() - 1 ; i >= 0 ; i-- ) {

			TypeContext contextIterator( context.m_namespace);
			CheckInitialize();
			m_pThisNode = CreateThis( context, i );
			if ( CheckIterator( contextIterator ) )
				return true;
			else {
				delete m_pThisNode;
				m_pThisNode = NULL;
				int iLastCode = GetLastExceptionCode( contextIterator );
				if ( iLastCode != EX_ITERATOR_DOESNT_EXIST && iLastCode != EX_TYPE_NOT_COMPOUND_ITERATOR && iLastCode != -1 ) {
					ADDEXP( contextIterator.m_poolExceptions );
					return false;
				}
			}
		}
		ADDEX( EXCEPTION1( EX_IMPL_ITER_DOESNT_EXIST, m_strName + "( ... | ... )", m_mapPositions[ LID_FEATURE_NAME ] ) );
		return false;
	}

	bool IteratorNode::CheckIterator( TypeContext& context )
	{

		std::shared_ptr<OclMeta::Type> pType = GetTreeManager()->GetTypeManager()->GetType( m_pThisNode->m_vecType[ 0 ], context.m_namespace );

		// Not compound Type cannot have iterators

		if ( ! pType->IsCompound() ) {
			ADDEX( EXCEPTION1( EX_TYPE_NOT_COMPOUND_ITERATOR, pType->GetName(), m_mapPositions[ LID_NODE_START ] ) );
			return false;
		}
		m_vecTypeDecl = m_pThisNode->m_vecType;
		m_vecTypeDecl.erase( m_vecTypeDecl.begin() );

		// Check Type of Declaration if it exists

		if ( ! m_strDeclType.empty() ) {
			TypeSeq vecType;
			if ( ParseTypeSeq( context, m_mapPositions[ LID_DECLARATOR_TYPE ], m_strDeclType, vecType ) )
				if ( CastType( context, m_mapPositions[ LID_DECLARATOR_TYPE ], m_vecTypeDecl, vecType ) )
					m_vecTypeDecl = vecType;
		}

		// Check Accu expression and set accumulator type

		if ( m_pAccuNode && m_pAccuNode->Check( context ) ) {
			m_vecTypeAccu = m_pAccuNode->m_vecType;

			if ( ! m_strAccuType.empty() ) {
				TypeSeq vecType;
				if ( ParseTypeSeq( context, m_mapPositions[ LID_ACCUMULATOR_TYPE ], m_strAccuType, vecType ) )
					if ( CastType( context, m_mapPositions[ LID_ACCUMULATOR_TYPE ], m_vecTypeAccu, vecType ) )
						m_vecTypeAccu = vecType;
			}
		}

		// Create implicit iterator variable if it is necessary

		if ( m_vecDeclarators.empty() ) {
			char chBuffer[ 20 ];
			sprintf_s( chBuffer, sizeof(chBuffer), "!iter%zd", context.m_vecImplicits.size() );
			m_vecDeclarators.push_back( std::string( chBuffer ) );
			context.m_vecImplicits.push_back( std::string( chBuffer ) );
		}

		// Check and Add Declarators

		std::vector< bool > vecDeclDone( m_vecDeclarators.size(), true );

		if ( ! m_vecTypeDecl.empty() ) {
			for ( unsigned int i = 0 ; i < m_vecDeclarators.size() ; i++ ) {
				vecDeclDone[ i ] = context.m_ctxTypes.AddVariable( m_vecDeclarators[ i ], m_vecTypeDecl, true );
				if ( ! vecDeclDone[ i ] )
					ADDEX( EXCEPTION1( EX_VARIABLE_ALREADY_EXISTS, m_vecDeclarators[ i ], m_mapPositions[ LID_DECLARATOR_NAME + i ] ) );
			}
		}

		// Check and Add Accumulator

		bool bAccuAdded = false;
		if ( ! m_vecTypeAccu.empty() ) {
			if ( context.m_ctxTypes.AddVariable( m_strAccuName, m_vecTypeAccu, true ) )
				bAccuAdded = true;
			else
				ADDEX( EXCEPTION1( EX_VARIABLE_ALREADY_EXISTS, m_strAccuName, m_mapPositions[ LID_ACCUMULATOR_NAME ] ) );
		}

		// Check Argument

		bool bArgumentValid = m_bTester && ! m_pArgumentNode->m_vecType.empty() || ! m_vecTypeDecl.empty() && m_pArgumentNode->Check( context );

		// Check if Iterator exists

		if ( bArgumentValid ) {

			if ( ! m_pAccuNode ) {

				// It is predefined iterator

				try {
					OclMeta::CompoundType* pCType = (OclMeta::CompoundType*) pType.get();
					OclMeta::Iterator* pIterator = pCType->GetIterator(0, OclSignature::Iterator( m_strName, m_pThisNode->m_vecType[ 0 ], m_pArgumentNode->m_vecType[ 0 ] ) );
					GetParametralTypeSeq( m_pThisNode->m_vecType, m_pArgumentNode->m_vecType, pIterator->GetReturnTypeSeq() );
				}
				catch ( OclCommon::Exception ex ) {
					SETEXPOS( ex, m_mapPositions[ LID_FEATURE_NAME ] );
					ADDEX( ex );
				}
			}
			else {

				// It is generic iterator

				CastType( context, m_mapPositions[ LID_ACCUMULATOR_TYPE ], m_vecTypeAccu, m_pArgumentNode->m_vecType );
				m_vecType = m_vecTypeAccu;
			}
		}

		// Remove Declarators

		if ( ! m_vecTypeDecl.empty() ) {
			for ( unsigned int i = 0 ; i < m_vecDeclarators.size() ; i++ )
				if ( vecDeclDone[ i ] )
					context.m_ctxTypes.RemoveVariable( m_vecDeclarators[ i ] );
		}

		// Remove Accumulator

		if ( bAccuAdded )
			context.m_ctxTypes.RemoveVariable( m_strAccuName );

		// Remove Implicit variable if it was

		if ( m_vecDeclarators[ 0 ].substr( 0, 1 ) == "!" ) {
			context.m_vecImplicits.pop_back();
		}

		return ! m_vecType.empty();
	}

	OclMeta::Object IteratorNode::Evaluate( ObjectContext& context )
	{
		// recursion corrected: terge
		LevelCounter level(&m_iteratorLevel);

		int iLineFeatureName = m_mapPositions[ LID_FEATURE_NAME ].iLine;
		OclSignature::Iterator signature( m_strName, m_pThisNode->m_vecType[ 0 ], m_pArgumentNode->m_vecType[ 0 ] );

		OclMeta::Object objectThis = m_pThisNode->Evaluate( context );

		if ( objectThis.IsUndefined() )
			return OclMeta::Object::UNDEFINED;

		EVALTRY {
			OclImplementation::Iterator* pIIterator = NULL;
			if ( ! m_pAccuNode ) {
				OclMeta::CompoundType* pCType = (OclMeta::CompoundType*) GetTreeManager()->GetTypeManager()->GetType( objectThis.GetStaticTypeName(), NILNAMESPACE ).get();
				OclMeta::Iterator* pIterator = pCType->GetIterator(m_iteratorLevel, signature );
				pIIterator = pIterator->GetImplementation();
				if ( ! pIIterator )
					THROWOCL0( ET_RUNTIME, EX_ITERATOR_NOT_IMPLEMENTED );

				pIIterator->Initialize();
				pIIterator->SetThis( objectThis );
			}
			else {
				OclMeta::Object accumulator = EvaluateCast( context, m_pAccuNode->Evaluate( context ), m_mapPositions[ LID_ACCUMULATOR_TYPE ].iLine, m_strAccuName + " = ...", m_vecType[ 0 ], m_strAccuType.empty() );
				context.oCtx.AddVariable( m_strAccuName, accumulator, true );
			}

			OclMeta::ObjectVector vecArguments( m_vecDeclarators.size(), OclMeta::Object::UNDEFINED );

			Iterate( context, pIIterator, objectThis, vecArguments, 0 );

			OclMeta::Object spResult;
			if ( ! m_pAccuNode ) {
				spResult = pIIterator->GetResult();
				pIIterator->Finalize();
			}
			else {
				context.oCtx.GetVariable( m_strAccuName, spResult );
				context.oCtx.RemoveVariable( m_strAccuName );
			}
			spResult = EvaluateCast( context, spResult, iLineFeatureName, signature.Print() );
			return CheckFalseResult( context, spResult, iLineFeatureName, signature.Print() );
		} EVALCATCH( iLineFeatureName, signature.Print() );
		return OclMeta::Object::UNDEFINED;
	}

	bool IteratorNode::Iterate( ObjectContext& context, OclImplementation::Iterator* pIIterator, OclMeta::Object& objectThis, OclMeta::ObjectVector& vecArguments, int iDeclNum )
	{
		int iLineFeatureName = m_mapPositions[ LID_FEATURE_NAME ].iLine;
		OclSignature::Iterator signature( m_strName, m_pThisNode->m_vecType[ 0 ], m_pArgumentNode->m_vecType[ 0 ] );

		std::string strDecl = m_vecDeclarators[ iDeclNum ];
		context.oCtx.AddVariable( strDecl, OclMeta::Object::UNDEFINED, true );

		DECL_ITERATOR( iterator, objectThis );
		OclMeta::Object objectIter;

		bool bDoStop = false;

		while ( iterator->HasNext() && ( ! bDoStop || ! context.m_bShortCircuitIterator ) ) {

			objectIter = iterator->GetNext();
			if ( ! m_strDeclType.empty() )
				objectIter = EvaluateCast( context, objectIter, m_mapPositions[ LID_DECLARATOR_TYPE ].iLine, signature.Print(), m_vecTypeDecl[ 0 ], false );
			context.oCtx.SetVariable( strDecl, objectIter );
			vecArguments[ iDeclNum ] = objectIter;

			if ( iDeclNum == (int)m_vecDeclarators.size() - 1 ) {
				EVALTRY {
					OclMeta::Object spAccu = m_pArgumentNode->Evaluate( context ); // evaluate the argument
					if ( ! m_pAccuNode ) {
						pIIterator->SetArguments( vecArguments );
						pIIterator->SetSubResult( spAccu );
						pIIterator->SetSubOriResult( objectIter );
						(*pIIterator)();
						if ( pIIterator->DoStop() )
							bDoStop = true;
					}
					else
						context.oCtx.SetVariable( m_strAccuName, spAccu );
				} EVALCATCH( iLineFeatureName, signature.Print() );
			}
			else
				bDoStop = Iterate( context, pIIterator, objectThis, vecArguments, iDeclNum + 1 );
		}
		context.oCtx.RemoveVariable( strDecl );
		return bDoStop;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::MethodNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################
	int MethodNode::m_stackLevel = 0;

	MethodNode::MethodNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_METHOD ), m_strName( "" ), m_pThisNode( NULL ), m_strCallOperator( "" ), m_pIterator( NULL )
	{
	}

	MethodNode::~MethodNode()
	{
		if ( ! m_bTester && ! m_bSelfTester ) {
			delete m_pThisNode;
			for ( unsigned int i = 0 ; i < m_vecArguments.size() ; i++ )
				delete m_vecArguments[ i ];
		}
		if ( m_pIterator ) {
			delete m_pIterator;
			m_pIterator = NULL;
		}
	}

	std::string MethodNode::Print( const std::string& strTabs ) const
	{
		if ( m_pIterator )
			return m_pIterator->Print( strTabs );

		std::string strOut = strTabs + "<Method name=\"" + m_strName + "\">\r\n";

		strOut += strTabs + TABSTR + "<This>\r\n";
		strOut += m_pThisNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</This>\r\n";

		if ( ! m_vecArguments.empty() ) {
			for ( unsigned int i = 0 ; i < m_vecArguments.size() ; i++ ) {
				char chNum[ 100 ];
				sprintf_s( chNum, sizeof(chNum), "%u", i );
				strOut += strTabs + TABSTR + "<Argument_" + std::string( chNum ) + ">\r\n";
				strOut += m_vecArguments[ i ]->Print( strTabs + TABSTR + TABSTR );
				strOut += strTabs + TABSTR + "</Argument_" + std::string( chNum ) + ">\r\n";
			}
		}

		strOut += strTabs + "</Method>\r\n";
		return strOut;
	}

	void MethodNode::CheckInitialize()
	{
		TreeNode::CheckInitialize();
		if ( m_pIterator ) {
			m_pIterator->m_bTester = true;
			delete m_pIterator;
			m_pIterator = NULL;
		}
	}

	bool MethodNode::CheckImplementation( TypeContext& context )
	{
		// Check Self

		bool bThisValid = m_bTester || m_pThisNode->Check( context );

		// Check whether it is IsUndefined method

		if ( m_strName == "isUndefined" && m_vecArguments.empty() ) {
			m_vecType = TypeSeq( 1, "ocl::Boolean" );
			return true;
		}

		if ( bThisValid ) {

			OclMeta::Type* pType = GetTreeManager()->GetTypeManager()->GetType( m_pThisNode->m_vecType[ 0 ], context.m_namespace ).get();

			// Check if Object Node is callable

			if ( m_pThisNode->GetKind() == TreeNode::NK_OBJECT ) {
				ObjectNode* pONode = (ObjectNode*) m_pThisNode;
				if ( ! pONode->m_bCallable )
					ADDEX( EXCEPTION1( EX_OBJECT_CALL_PROHIBITED, m_pThisNode->m_vecType[ 0 ], m_mapPositions[ LID_CALL_OPERATOR ] ) );
			}

			// Check member selection operator

			if ( m_strCallOperator == "->" && ! pType->IsCompound() )
				ADDEX( EXCEPTION1( EX_TYPE_NOT_COMPOUND_ITERATOR, pType->GetName(), m_mapPositions[ LID_CALL_OPERATOR ] ) );

			// Check method exists

			if ( ! pType->IsCompound() || m_vecArguments.size() != 1 ) {

				bool bArgumentsValid = true;
				unsigned int i;
				for ( i = 0 ; i < m_vecArguments.size() ; i++ )
					bArgumentsValid = m_vecArguments[ i ]->Check( context ) && bArgumentsValid;

				if ( bArgumentsValid ) {
					StringVector vecTypes;
					for ( i = 0 ; i < m_vecArguments.size() ; i++ )
						vecTypes.push_back( m_vecArguments[ i ]->m_vecType[ 0 ] );
					OclSignature::Method sigMethod( m_strName, pType->GetName(), vecTypes );

					try {
						OclMeta::Method* pMethod = pType->GetMethod( sigMethod );
						GetParametralTypeSeq( m_pThisNode->m_vecType, ( m_vecArguments.empty() ) ? TypeSeq() : m_vecArguments[ 0 ]->m_vecType, pMethod->GetReturnTypeSeq() );
						if ( pMethod->IsDependence() )
							context.m_setDependencies.insert( OclMeta::Dependency( sigMethod.Print(), m_mapPositions[ LID_FEATURE_NAME ] ) );
						return true;
					}
					catch ( OclCommon::Exception ex ) {
						SETEXPOS( ex, m_mapPositions[ LID_FEATURE_NAME ] );
						ADDEX( ex );
						return false;
					}
				}
				return false;

			}

			// Try argument as method argument

			TypeContext contextMethod = context;
			contextMethod.m_poolExceptions.Clear();
			bool bMethodValid = m_vecArguments[ 0 ]->Check( contextMethod );
			TypeSeq vecTypeMethod = m_vecArguments[ 0 ]->m_vecType;

			// Try argument as iterator body expression

			TypeContext contextIterator = context;
			contextIterator.m_poolExceptions.Clear();

				// Add new implicit iterator variable

				TypeSeq vecType = m_pThisNode->m_vecType;
				vecType.erase( vecType.begin() );
				char chBuffer[ 20 ];
				sprintf_s( chBuffer, sizeof(chBuffer), "!iter%zu", contextIterator.m_vecImplicits.size() );
				contextIterator.m_ctxTypes.AddVariable( std::string( chBuffer ), vecType, true );
				contextIterator.m_vecImplicits.push_back( std::string( chBuffer ) );

				// Check the iterator

				bool bIteratorValid = m_vecArguments[ 0 ]->Check( contextIterator );

				// Remove the newly created implicit iterator

				contextIterator.m_ctxTypes.RemoveVariable( std::string( chBuffer ) );
				contextIterator.m_vecImplicits.pop_back();

			TypeSeq vecTypeIterator = m_vecArguments[ 0 ]->m_vecType;

			if ( bMethodValid ) {
				std::vector<OclMeta::Type*> vecTypes( 2, pType );

				OclSignature::Method sigMethod( m_strName, pType->GetName(), StringVector( 1, vecTypeMethod[ 0 ] ) );
				OclSignature::Iterator sigIterator( m_strName, pType->GetName(), vecTypeMethod[ 0 ] );
				std::vector<OclSignature::Feature*> vecSignatures( 2, &sigMethod ); vecSignatures[ 1 ] = &sigIterator;

				std::vector<int> vecCodes( 3, EX_METHOD_AMBIGUOUS ); vecCodes[ 1 ] = EX_ITERATOR_AMBIGUOUS; vecCodes[ 2 ] = EX_METHOD_ITERATOR_AMBIGUOUS;

				OclCommon::ExceptionPool poolResult;

				int iPrecedence = ( m_strCallOperator.empty() ) ? 0 : ( m_strCallOperator == "." ) ? -1 : 1;

				OclMeta::Feature* pFeature = CheckAmbiguity( vecTypes, vecSignatures, vecCodes, iPrecedence, poolResult );
				if ( ! pFeature ) {
					ADDEXP( poolResult );
					return false;
				}
				else {
					if ( iPrecedence == -1 ) {
						ADDEXP( contextMethod.m_poolExceptions );
						TypeContext contextTemp = context;
						m_vecArguments[ 0 ]->Check( contextTemp );
						GetParametralTypeSeq( m_pThisNode->m_vecType, vecTypeMethod, pFeature->GetReturnTypeSeq() );
						if ( pFeature->IsDependence() )
							context.m_setDependencies.insert( OclMeta::Dependency( sigMethod.Print(), m_mapPositions[ LID_FEATURE_NAME ] ) );
						return true;
					}
					else {
						ADDEXP( contextIterator.m_poolExceptions );
						if ( ! bIteratorValid )
							return false;
						if ( vecTypeMethod != vecTypeIterator )
							return false;
						m_pIterator= GetTreeManager()->CreateIterator();
						m_pIterator->m_bTester = true;
						m_pIterator->m_pThisNode = m_pThisNode;
						m_pIterator->m_strCallOperator = "->";
						m_pIterator->m_strName = m_strName;
						m_pIterator->m_vecDeclarators;
						m_pIterator->m_mapPositions = m_mapPositions;
						m_pIterator->m_pArgumentNode = m_vecArguments[ 0 ];

						m_pIterator->Check( context );
						m_pIterator->m_bTester = false;
						m_bSelfTester = true;
						m_vecType = m_pIterator->m_vecType;
						return true;
					}
				}
			}
			if ( bIteratorValid ) {
				std::vector<OclMeta::Type*> vecTypes( 2, pType );

				OclSignature::Iterator sigIterator( m_strName, pType->GetName(), vecTypeIterator[ 0 ] );
				OclSignature::Method sigMethod( m_strName, pType->GetName(), StringVector( 1, vecTypeIterator[ 0 ] ) );
				std::vector<OclSignature::Feature*> vecSignatures( 2, &sigIterator ); vecSignatures[ 1 ] = &sigMethod;

				std::vector<int> vecCodes( 3, EX_ITERATOR_AMBIGUOUS ); vecCodes[ 1 ] = EX_METHOD_AMBIGUOUS; vecCodes[ 2 ] = EX_METHOD_ITERATOR_AMBIGUOUS;

				OclCommon::ExceptionPool poolResult;

				int iPrecedence = 0;

				OclMeta::Feature* pFeature = CheckAmbiguity( vecTypes, vecSignatures, vecCodes, iPrecedence, poolResult );
				if ( ! pFeature ) {
					ADDEXP( poolResult );
					return false;
				}
				else {
					if ( iPrecedence == 1 ) {
						ADDEXP( contextMethod.m_poolExceptions );
						if ( pFeature->IsDependence() )
							context.m_setDependencies.insert( OclMeta::Dependency( sigMethod.Print(), m_mapPositions[ LID_FEATURE_NAME ] ) );
						return true;
					}
					else {
						ADDEXP( contextIterator.m_poolExceptions );
						m_pIterator= GetTreeManager()->CreateIterator();
						m_pIterator->m_bTester = true;
						m_pIterator->m_pThisNode = m_pThisNode;
						m_pIterator->m_strCallOperator = "->";
						m_pIterator->m_strName = m_strName;
						m_pIterator->m_vecDeclarators;
						m_pIterator->m_mapPositions = m_mapPositions;
						m_pIterator->m_pArgumentNode = m_vecArguments[ 0 ];

						m_pIterator->Check( context );
						m_pIterator->m_bTester = false;
						m_bSelfTester = true;
						m_vecType = m_pIterator->m_vecType;
						return true;
					}
				}
			}
			if ( m_strCallOperator == "->" )
				ADDEXP( contextIterator.m_poolExceptions );
			else
				ADDEXP( contextMethod.m_poolExceptions );
			return false;
		}
		return false;
	}

#define MAX_CONSTR_FUNC_LEVEL 100 // max level of constraint function encapsulation

	OclMeta::Object MethodNode::Evaluate( ObjectContext& context )
	{
		LevelCounter level(&m_stackLevel);
		std::string lastFuncName = "";
		static std::string currFuncName = "";
		std::string *szivas = &currFuncName;

		if (m_stackLevel <= 1)
			currFuncName = "";

		if ( m_pIterator )
			return m_pIterator->Evaluate( context );

		int iLineFeatureName = m_mapPositions[ LID_FEATURE_NAME ].iLine;

		StringVector vecTypes;
		unsigned int i;
		for ( i = 0 ; i < m_vecArguments.size() ; i++ )
			vecTypes.push_back( m_vecArguments[ i ]->m_vecType[ 0 ] );
		OclSignature::Method signature( m_strName, m_pThisNode->m_vecType[ 0 ], vecTypes );

		if (m_stackLevel >= MAX_CONSTR_FUNC_LEVEL)
		{
			std::string message = "possibly infinite loop broken";
			AddViolation(context, iLineFeatureName, signature.Print(), message);
			return OclMeta::Object::UNDEFINED;
		}

		OclMeta::Object objectThis = m_pThisNode->Evaluate( context );
		OclMeta::ObjectVector vecArguments;
		for ( i = 0 ; i < m_vecArguments.size() ; i++ )
			vecArguments.push_back( m_vecArguments[ i ]->Evaluate( context ) );

		if ( m_strName == "isUndefined" && m_vecArguments.empty() && m_vecType[ 0 ] == "ocl::Boolean" )
			return CheckFalseResult( context, CREATE_BOOLEAN( GetTreeManager()->GetTypeManager(), objectThis.IsUndefined() ), iLineFeatureName, signature.Print() );

		if ( objectThis.IsUndefined() )
			return OclMeta::Object::UNDEFINED;
		for ( i = 0 ; i < vecArguments.size() ; i++ )
			if ( vecArguments[ i ].IsUndefined() )
				return OclMeta::Object::UNDEFINED;

		EVALTRY {
			OclMeta::Type* pType = GetTreeManager()->GetTypeManager()->GetType( objectThis.GetStaticTypeName(), NILNAMESPACE ).get();
			OclMeta::Method* pMethod = pType->GetMethod( signature );
			OclImplementation::Method* pIMethod = pMethod->GetImplementation();
			if ( ! pIMethod )
				THROWOCL0( ET_RUNTIME, EX_METHOD_NOT_IMPLEMENTED );

			if (((OclMeta::Feature*)pMethod)->IsDynamic()) // userdef method
			{
				lastFuncName = currFuncName;
				currFuncName =  m_strName; // GetFullName();
			}

			pIMethod->Initialize();
			pIMethod->SetThis( objectThis );
			pIMethod->SetArguments( vecArguments );

			(*pIMethod)();

			OclMeta::Object spResult = EvaluateCast( context, pIMethod->GetResult(), iLineFeatureName, signature.Print() );

			pIMethod->Finalize();

			// all of the violations collected in pIMethod (((OclGmeCM::ConstraintMethod*)pIMethod)->m_spFunction)
			// will lose, one violation will be added to context depending on spResult
			// ?? !! terge
			OclTree::ViolationVector vec = pIMethod->GetViolations();
			pIMethod->ClearViolations();
			int num = vec.size();
			for (int kk=0; kk<num; kk++)
			{
				if (vec[kk].methodName.empty())
					vec[kk].methodName = currFuncName;
				AddViolation(context, vec[kk]);
			}
			if (!lastFuncName.empty())
				currFuncName = lastFuncName;

			return CheckFalseResult( context, spResult, iLineFeatureName, signature.Print() );
		} EVALCATCH( iLineFeatureName, signature.Print() );
		return OclMeta::Object::UNDEFINED;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::FunctionNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################

	FunctionNode::FunctionNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_FUNCTION ), m_strName( "" ), m_pMethod( NULL )
	{
	}

	FunctionNode::~FunctionNode()
	{
		if ( ! m_bTester && ! m_bSelfTester ) {
			for ( unsigned int i = 0 ; i < m_vecArguments.size() ; i++ )
				delete m_vecArguments[ i ];
		}
		CheckInitialize();
	}

	std::string FunctionNode::Print( const std::string& strTabs ) const
	{
		if ( m_pMethod )
			return m_pMethod->Print( strTabs );

		std::string strOut = strTabs + "<Function name=\"" + m_strName + "\">\r\n";

		if ( ! m_vecArguments.empty() ) {
			for ( unsigned int i = 0 ; i < m_vecArguments.size() ; i++ ) {
				char chNum[ 100 ];
				sprintf_s( chNum, sizeof(chNum), "%u", i );
				strOut += strTabs + TABSTR + "<Argument_" + std::string( chNum ) + ">\r\n";
				strOut += m_vecArguments[ i ]->Print( strTabs + TABSTR + TABSTR );
				strOut += strTabs + TABSTR + "</Argument_" + std::string( chNum ) + ">\r\n";
			}
		}

		strOut += strTabs + "</Function>\r\n";
		return strOut;
	}

	void FunctionNode::CheckInitialize()
	{
		TreeNode::CheckInitialize();
		if ( m_pMethod ) {
			m_pMethod->CheckInitialize();
			m_pMethod->m_bTester = true;
			delete m_pMethod->m_pThisNode;
			m_pMethod->m_pThisNode = NULL;
			delete m_pMethod;
			m_pMethod = NULL;
		}
	}

	bool FunctionNode::CheckImplementation( TypeContext& context )
	{
		TypeContext contextFunction = context;
		contextFunction.m_poolExceptions.Clear();

		bool bArgumentsValid = true;
		int i;
		for ( i = 0 ; i < (int) m_vecArguments.size() ; i++ )
			bArgumentsValid = m_vecArguments[ i ]->Check( contextFunction ) && bArgumentsValid;

		OclCommon::Exception exFunction;

		if ( bArgumentsValid ) {
			StringVector vecTypes;
			for ( i = 0 ; i < (int) m_vecArguments.size() ; i++ )
				vecTypes.push_back( m_vecArguments[ i ]->m_vecType[ 0 ] );
			OclSignature::Function sigFunction( m_strName, vecTypes );

			try {
				OclMeta::Function* pFunction = GetTreeManager()->GetTypeManager()->GetFunction( sigFunction );
				ADDEXP( contextFunction.m_poolExceptions );
				GetParametralTypeSeq( TypeSeq(), ( m_vecArguments.empty() ) ? TypeSeq() : m_vecArguments[ 0 ]->m_vecType, pFunction->GetReturnTypeSeq() );
				if ( pFunction->IsDependence() )
					context.m_setDependencies.insert( OclMeta::Dependency( sigFunction.Print(), m_mapPositions[ LID_FEATURE_NAME ] ) );
				return true;
			}
			catch ( OclCommon::Exception ex ) {
				SETEXPOS( ex, m_mapPositions[ LID_FEATURE_NAME ] );
				if ( ex.GetCode() != EX_FUNCTION_DOESNT_EXIST ) {
					ADDEXP( contextFunction.m_poolExceptions );
					ADDEX( ex );
					return false;
				}
				exFunction = ex;
			}
		}

		// Check implicit variables

		OclCommon::ExceptionPool poolExceptions;
		for ( i = context.m_vecImplicits.size() - 1 ; i >= 0 ; i-- ) {

			TypeContext contextMethod = context;
			contextMethod.m_poolExceptions.Clear();

			m_pMethod = GetTreeManager()->CreateMethod();
			m_pMethod->m_bTester = true;
			m_pMethod->m_pThisNode = CreateThis( context, i );
			m_pMethod->m_strCallOperator = "";
			m_pMethod->m_strName = m_strName;
			m_pMethod->m_mapPositions = m_mapPositions;
			m_pMethod->m_vecArguments = m_vecArguments;

			if ( m_pMethod->Check( contextMethod ) ) {
				ADDEXP( contextMethod.m_poolExceptions );
				m_pMethod->m_bTester = false;
				m_bSelfTester = true;
				m_vecType = m_pMethod->m_vecType;
				return true;
			}
			else {
				delete m_pMethod->m_pThisNode;
				m_pMethod->m_pThisNode = NULL;
				delete m_pMethod;
				m_pMethod = NULL;
				int iLastCode = GetLastExceptionCode( contextMethod);
				if ( iLastCode != EX_METHOD_DOESNT_EXIST && iLastCode != EX_ITERATOR_DOESNT_EXIST &&  iLastCode != EX_TYPE_NOT_COMPOUND_ITERATOR && iLastCode != -1 ) {
					ADDEXP( contextMethod.m_poolExceptions );
					return false;
				}
			}
		}
		ADDEX( EXCEPTION1( EX_FUNC_IMPL_METH_ITER_DOESNT_EXIST, m_strName + "( ... )", m_mapPositions[ LID_FEATURE_NAME ] ) );
		return false;
	}

	OclMeta::Object FunctionNode::Evaluate( ObjectContext& context )
	{
		if ( m_pMethod )
			return m_pMethod->Evaluate( context );

		int iLineFeatureName = m_mapPositions[ LID_FEATURE_NAME ].iLine;
		StringVector vecTypes;
		unsigned int i;
		for ( i = 0 ; i < m_vecArguments.size() ; i++ )
			vecTypes.push_back( m_vecArguments[ i ]->m_vecType[ 0 ] );
		OclSignature::Function signature( m_strName, vecTypes );

		OclMeta::ObjectVector vecArguments;
		for ( i = 0 ; i < m_vecArguments.size() ; i++ )
			vecArguments.push_back( m_vecArguments[ i ]->Evaluate( context ) );

		for ( i = 0 ; i < vecArguments.size() ; i++ )
			if ( vecArguments[ i ].IsUndefined() )
				return OclMeta::Object::UNDEFINED;

		EVALTRY {
			OclMeta::Function* pFunction = GetTreeManager()->GetTypeManager()->GetFunction( signature );
			OclImplementation::Function* pIFunction = pFunction->GetImplementation();
			if ( ! pIFunction )
				THROWOCL0( ET_RUNTIME, EX_FUNCTION_NOT_IMPLEMENTED );

			pIFunction->Initialize();
			pIFunction->SetArguments( vecArguments );

			(*pIFunction)();

			OclMeta::Object spResult = EvaluateCast( context, pIFunction->GetResult(), iLineFeatureName, signature.Print() );
			pIFunction->Finalize();

			return CheckFalseResult( context, spResult, iLineFeatureName, signature.Print() );
		} EVALCATCH( iLineFeatureName, signature.Print() );
		return OclMeta::Object::UNDEFINED;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::AssociationNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################

	AssociationNode::AssociationNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_ASSOCIATION), m_strName( "" ), m_strAcceptable( "" ), m_pThisNode( NULL )
	{
	}

	AssociationNode::~AssociationNode()
	{
		if ( ! m_bTester )
			if ( m_pThisNode )
				delete m_pThisNode;
	}

	std::string AssociationNode::Print( const std::string& strTabs ) const
	{
		std::string strOut = strTabs + "<Association name=\"" + m_strName + "\" class=\"" + m_strAcceptable + "\">\r\n";

		strOut += strTabs + TABSTR + "<This>\r\n";
		if ( m_pThisNode )
			strOut += m_pThisNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</This>\r\n";

		strOut += strTabs + "</Association>\r\n";
		return strOut;
	}

	void AssociationNode::CheckInitialize()
	{
		if ( m_bSelfTester && ! m_bTester && m_pThisNode ) {
			delete m_pThisNode;
			m_pThisNode = NULL;
		}
		TreeNode::CheckInitialize();
	}

	bool AssociationNode::CheckImplementation( TypeContext& context )
	{
		// Association cannot be called by ->

		if ( m_strCallOperator == "->" )
			ADDEX( EXCEPTION1( EX_INVALID_CALL, m_strCallOperator, m_mapPositions[ LID_CALL_OPERATOR ] ) );

		// If This exists

		if ( m_pThisNode ) {
			bool bThisValid = m_bTester || m_pThisNode->Check( context );
			if ( bThisValid )
				return CheckAssociation( context );
			return false;
		}

		// If This does not exist

		m_bSelfTester = true;

		// Check Implicits variables

		for ( int i = context.m_vecImplicits.size() - 1 ; i >= 0 ; i-- ) {

			TypeContext contextAssociation( context.m_namespace);
			CheckInitialize();
			m_pThisNode = CreateThis( context, i );
			if ( CheckAssociation( contextAssociation ) )
				return true;
			else {
				delete m_pThisNode;
				m_pThisNode = NULL;
				int iLastCode = GetLastExceptionCode( contextAssociation );
				if ( iLastCode != EX_ASSOCIATION_DOESNT_EXIST ) {
					ADDEXP( contextAssociation.m_poolExceptions );
					return false;
				}
			}
		}
		ADDEX( EXCEPTION1( EX_IMPL_ASSOC_DOESNT_EXIST, OclSignature::Association( m_strName, "", m_strAcceptable ).Print(), m_mapPositions[ LID_FEATURE_NAME ] ) );
		return false;
	}

	bool AssociationNode::CheckAssociation( TypeContext& context )
	{
		OclMeta::Type* pType = GetTreeManager()->GetTypeManager()->GetType( m_pThisNode->m_vecType[ 0 ], context.m_namespace ).get();

		// Check static object can be called

		if ( m_pThisNode->GetKind() == TreeNode::NK_OBJECT && ! m_bTester ) {
			ObjectNode* pONode = (ObjectNode*) m_pThisNode;
			if ( ! pONode->m_bCallable )
				ADDEX( EXCEPTION1( EX_OBJECT_CALL_PROHIBITED, m_pThisNode->m_vecType[ 0 ], m_mapPositions[ LID_CALL_OPERATOR ] ) );
		}

		// Compound Type cannot have associations

		if ( pType->IsCompound() ) {
			ADDEX( EXCEPTION1( EX_TYPE_COMPOUND_ASSOCIATION, pType->GetName(), m_mapPositions[ LID_FEATURE_NAME ] ) );
			return false;
		}

		// Check Association Exists

		try {
			OclMeta::Association* pAssociation = pType->GetAssociation( OclSignature::Association( m_strName, m_pThisNode->m_vecType[ 0 ], m_strAcceptable ) );
			m_vecType = pAssociation->GetReturnTypeSeq();
			return true;
		}
		catch ( OclCommon::Exception ex ) {
			SETEXPOS( ex, m_mapPositions[ LID_FEATURE_NAME ] );
			ADDEX( ex );
			return false;
		}
	}

	OclMeta::Object AssociationNode::Evaluate( ObjectContext& context )
	{
		int iLineFeatureName = m_mapPositions[ LID_FEATURE_NAME ].iLine;
		OclSignature::Association signature( m_strName, m_pThisNode->m_vecType[ 0 ], m_strAcceptable );

		OclMeta::Object objectThis = m_pThisNode->Evaluate( context );
		if ( objectThis.IsUndefined() )
			return objectThis;

		EVALTRY {
			OclMeta::Type* pType = GetTreeManager()->GetTypeManager()->GetType( objectThis.GetStaticTypeName(), NILNAMESPACE ).get();
			OclMeta::Association* pAssociation = pType->GetAssociation( signature );
			OclImplementation::Association* pIAssociation = pAssociation->GetImplementation();
			if ( ! pIAssociation )
				THROWOCL0( ET_RUNTIME, EX_ASSOCIATION_NOT_IMPLEMENTED );

			pIAssociation->Initialize();
			pIAssociation->SetThis( objectThis );

			(*pIAssociation)();

			OclMeta::Object spResult = EvaluateCast( context, pIAssociation->GetResult(), iLineFeatureName, signature.Print() );
			pIAssociation->Finalize();

			return CheckFalseResult( context, spResult, iLineFeatureName, signature.Print() );
		} EVALCATCH( iLineFeatureName, signature.Print() );
		return OclMeta::Object::UNDEFINED;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::AttributeNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################

	AttributeNode::AttributeNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_ATTRIBUTE ), m_strName( "" ), m_pThisNode( NULL ), m_pAssociation( NULL ), m_pCollector( NULL )
	{
	}

	AttributeNode::~AttributeNode()
	{
		if ( ! m_bTester && ! m_bSelfTester )
			if ( m_pThisNode )
				delete m_pThisNode;
		if ( m_pAssociation ) {
			delete m_pAssociation;
			m_pAssociation = NULL;
		}
		if ( m_pCollector ) {
			delete m_pCollector;
			m_pCollector = NULL;
		}
	}

	std::string AttributeNode::Print( const std::string& strTabs ) const
	{
		if ( m_pAssociation )
			return m_pAssociation->Print( strTabs );
		if ( m_pCollector )
			return m_pCollector->Print( strTabs );

		std::string strOut = strTabs + "<Attribute name=\"" + m_strName + "\">\r\n";

		strOut += strTabs + TABSTR + "<This>\r\n";
		strOut += m_pThisNode->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</This>\r\n";

		strOut += strTabs + "</Attribute>\r\n";
		return strOut;
	}

	void AttributeNode::CheckInitialize()
	{
		TreeNode::CheckInitialize();
		if ( m_pAssociation ) {
			m_pAssociation->m_bTester = true;
			delete m_pAssociation;
			m_pAssociation = NULL;
		}
		if ( m_pCollector ) {
			m_pCollector->m_bTester = true;
			delete m_pCollector->m_pArgumentNode;
			m_pCollector->m_pArgumentNode = NULL;
			delete m_pCollector;
			m_pCollector = NULL;
		}
	}

	bool AttributeNode::CheckImplementation( TypeContext& context )
	{
		// Check Self

		bool bThisValid = m_bTester || m_pThisNode->Check( context );

		if ( bThisValid ) {

			OclMeta::Type* pType = GetTreeManager()->GetTypeManager()->GetType( m_pThisNode->m_vecType[ 0 ], context.m_namespace ).get();

			// Type is not compound

			if ( ! pType->IsCompound() ) {

				// Check static object can be called

				if ( m_pThisNode->GetKind() == TreeNode::NK_OBJECT ) {
					ObjectNode* pONode = (ObjectNode*) m_pThisNode;
					if ( ! pONode->m_bCallable )
						ADDEX( EXCEPTION1( EX_OBJECT_CALL_PROHIBITED, m_pThisNode->m_vecType[ 0 ], m_mapPositions[ LID_CALL_OPERATOR ] ) );
				}

				// Check member selection operator

				if ( m_strCallOperator == "->" )
					ADDEX( EXCEPTION1( EX_INVALID_CALL, m_strCallOperator, m_mapPositions[ LID_CALL_OPERATOR ] ) );

				// Create Feature Call Check

				std::vector<OclMeta::Type*> vecTypes( 2, pType );

				OclSignature::Attribute sigAttribute( m_strName, pType->GetName() );
				OclSignature::Association sigAssociation( m_strName, pType->GetName(), "" );
				std::vector<OclSignature::Feature*> vecSignatures( 2, &sigAttribute ); vecSignatures[ 1 ] = &sigAssociation;

				std::vector<int> vecCodes( 3, EX_ATTRIBUTE_AMBIGUOUS ); vecCodes[ 1 ] = EX_ASSOCIATION_AMBIGUOUS; vecCodes[ 2 ] = EX_ATTRIBUTE_ASSOCIATION_AMBIGUOUS;

				OclCommon::ExceptionPool poolResult;

				int iPrecedence = 0;

				// Get Feature

				OclMeta::Feature* pFeature = CheckAmbiguity( vecTypes, vecSignatures, vecCodes, iPrecedence, poolResult );
				if ( ! pFeature ) {
					ADDEXP( poolResult );
					return false;
				}
				else {
					if ( iPrecedence == -1 ) {
						m_vecType = pFeature->GetReturnTypeSeq();
						if ( pFeature->IsDependence() )
							context.m_setDependencies.insert( OclMeta::Dependency( sigAttribute.Print(), m_mapPositions[ LID_FEATURE_NAME ] ) );
					}
					else {
						m_pAssociation = GetTreeManager()->CreateAssociation();
						m_pAssociation->m_bTester = true;
						m_pAssociation->m_pThisNode = m_pThisNode;
						m_pAssociation->m_strCallOperator = ".";
						m_pAssociation->m_strName = m_strName;
						m_pAssociation->m_strAcceptable = "";

						m_pAssociation->Check( context );
						m_pAssociation->m_bTester = false;
						m_bSelfTester = true;
						m_vecType = m_pAssociation->m_vecType;
					}
					return true;
				}
			}
			else {

			// Type is compound

				// Create Feature Call Check

				OclMeta::Type* pTypeInner = GetTreeManager()->GetTypeManager()->GetType( m_pThisNode->m_vecType[ 1 ], context.m_namespace ).get();

				std::vector<OclMeta::Type*> vecTypes( 2, pType ); vecTypes[ 1 ] = pTypeInner;

				OclSignature::Attribute sigAttribute( m_strName, pType->GetName() );
				OclSignature::Attribute sigAttributeInner( m_strName, pTypeInner->GetName() );
				std::vector<OclSignature::Feature*> vecSignatures( 2, &sigAttribute ); vecSignatures[ 1 ] = &sigAttributeInner;

				std::vector<int> vecCodes( 3, EX_ATTRIBUTE_AMBIGUOUS ); vecCodes[ 2 ] = EX_ATTRIBUTE_INNERATTRIBUTE_AMBIGUOUS;

				OclCommon::ExceptionPool poolResult;

				int iPrecedence = ( m_strCallOperator.empty() ) ? 0 : ( m_strCallOperator == "->" ) ? -1 : 1;

				// Get Feature

				OclMeta::Feature* pFeature = CheckAmbiguity( vecTypes, vecSignatures, vecCodes, iPrecedence, poolResult );
				if ( ! pFeature ) {
					ADDEXP( poolResult );
					return false;
				}
				else {
					if ( iPrecedence == -1 ) {
						m_vecType = pFeature->GetReturnTypeSeq();
						if ( pFeature->IsDependence() )
							context.m_setDependencies.insert( OclMeta::Dependency( sigAttribute.Print(), m_mapPositions[ LID_FEATURE_NAME ] ) );
					}
					else {
						if ( pFeature->IsDependence() )
							context.m_setDependencies.insert( OclMeta::Dependency( sigAttributeInner.Print(), m_mapPositions[ LID_FEATURE_NAME ] ) );
						int iPiece = 0; char chBuffer[ 500 ];
						std::string strDeclarator = "-" + m_strName;
						sprintf_s( chBuffer, sizeof(chBuffer), "%d", iPiece );
						while ( context.m_ctxTypes.ExistsVariable( strDeclarator + std::string( chBuffer ) ) )
							sprintf_s( chBuffer, sizeof(chBuffer), "%d", ++iPiece );

						VariableNode* pVariableNode = GetTreeManager()->CreateVariable();
						pVariableNode->m_strName = strDeclarator + std::string( chBuffer );
						pVariableNode->m_vecType = TypeSeq( 1, pTypeInner->GetName() );

						AttributeNode* pAttributeNode = GetTreeManager()->CreateAttribute();
						pAttributeNode->m_pThisNode = pVariableNode;
						pAttributeNode->m_strCallOperator = ( pTypeInner->IsCompound() ) ? "->" : ".";
						pAttributeNode->m_strName = m_strName;
						pAttributeNode->m_vecType = pFeature->GetReturnTypeSeq();

						m_pCollector = GetTreeManager()->CreateIterator();
						m_pCollector->m_bTester = true;
						m_pCollector->m_pThisNode = m_pThisNode;
						m_pCollector->m_strCallOperator = "->";
						m_pCollector->m_strName = "collect";
						m_pCollector->m_vecDeclarators = StringVector( 1, std::string( chBuffer ) );
						m_pCollector->m_mapPositions = m_mapPositions;
						m_pCollector->m_pArgumentNode = pAttributeNode;

						m_pCollector->Check( context );
						m_pCollector->m_bTester = false;
						m_bSelfTester = true;
						m_vecType = m_pCollector->m_vecType;
					}
					return true;
				}
			}
		}
		return false;
	}

	OclMeta::Object AttributeNode::Evaluate( ObjectContext& context )
	{
		if ( m_pAssociation )
			return m_pAssociation->Evaluate( context );
		if ( m_pCollector )
			return m_pCollector->Evaluate( context );

		int iLineFeatureName = m_mapPositions[ LID_FEATURE_NAME ].iLine;
		OclSignature::Attribute signature( m_strName, m_pThisNode->m_vecType[ 0 ] );

		OclMeta::Object objectThis = m_pThisNode->Evaluate( context );
		if ( objectThis.IsUndefined() )
			return objectThis;

		EVALTRY {
			OclMeta::Type* pType = GetTreeManager()->GetTypeManager()->GetType( objectThis.GetStaticTypeName(), NILNAMESPACE ).get();
			OclMeta::Attribute* pAttribute = pType->GetAttribute( signature );
			OclImplementation::Attribute* pIAttribute = pAttribute->GetImplementation();
			if ( ! pIAttribute )
				THROWOCL0( ET_RUNTIME, EX_ATTRIBUTE_NOT_IMPLEMENTED );

			pIAttribute->Initialize();
			pIAttribute->SetThis( objectThis );

			(*pIAttribute)();

			OclMeta::Object spResult = EvaluateCast( context, pIAttribute->GetResult(), iLineFeatureName, signature.Print() );
			pIAttribute->Finalize();

			return CheckFalseResult( context, spResult, iLineFeatureName, signature.Print() );
		} EVALCATCH( iLineFeatureName, signature.Print() );
		return OclMeta::Object::UNDEFINED;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::VariableNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################

	VariableNode::VariableNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_VARIABLE ), m_strName( "" ), m_pAttribute( NULL ), m_pType( NULL )
	{
	}

	VariableNode::~VariableNode()
	{
		CheckInitialize();
	}

	std::string VariableNode::Print( const std::string& strTabs ) const
	{
		if ( m_pAttribute )
			return m_pAttribute->Print( strTabs );

		std::string strOut = strTabs + "<Variable name=\"" + m_strName + "\">\r\n";
		strOut += strTabs + "</Variable>\r\n";
		return strOut;
	}

	void VariableNode::CheckInitialize()
	{
		TreeNode::CheckInitialize();
		if ( m_pAttribute ) {
			m_pAttribute->m_bTester = true;
			m_pAttribute->CheckInitialize();
			delete m_pAttribute->m_pThisNode;
			m_pAttribute->m_pThisNode = NULL;
			delete m_pAttribute;
			m_pAttribute = NULL;
		}
		if ( m_pType ) {
			delete m_pType;
			m_pType = NULL;
		}
	}

	bool VariableNode::CheckImplementation( TypeContext& context )
	{
		// Check if Type exists

		TypeContext contextType( context.m_namespace);
		m_pType = GetTreeManager()->CreateObject();
		m_pType->m_bTester = true;
		m_pType->m_strType = "ocl::Type";
		m_pType->m_strValue = m_strName;
		m_pType->m_bCallable = false;
		m_pType->m_mapPositions = m_mapPositions;
		if ( m_pType->Check( contextType ) ) {
			m_pType->m_bTester = false;
			m_bSelfTester = true;
			m_vecType = m_pType->m_vecType;
			return true;
		}
		else {
			delete m_pType;
			m_pType = NULL;
			if ( m_strName.find( ":" ) != std::string::npos ) {
				ADDEXP( contextType.m_poolExceptions );
				return false;
			}
		}

		// Check if Variable exists

		OclCommon::Exception exVariable = EXCEPTION1( EX_VARIABLE_DOESNT_EXIST, m_strName, m_mapPositions[ LID_NODE_START ] );
		if ( context.m_ctxTypes.GetVariable( m_strName, m_vecType ) )
			return true;
		else
			if ( m_strName == "self" || m_strName.substr( 0, 1 ) == "!" || m_strName.substr( 0, 1 ) == "-" ) {
				ADDEX( exVariable );
				return false;
			}

		// Check Self

		for ( int i = context.m_vecImplicits.size() - 1 ; i >= 0 ; i-- ) {

			TypeContext contextAttribute = context;
			contextAttribute.m_poolExceptions.Clear();

			m_pAttribute = GetTreeManager()->CreateAttribute();
			m_pAttribute->m_bTester = true;
			m_pAttribute->m_pThisNode = CreateThis( context, i );
			m_pAttribute->m_strCallOperator = "";
			m_pAttribute->m_strName = m_strName;

			if ( m_pAttribute->Check( contextAttribute ) ) {
				m_pAttribute->m_bTester = false;
				m_bSelfTester = true;
				m_vecType = m_pAttribute->m_vecType;
				return true;
			}
			else {
				delete m_pAttribute->m_pThisNode;
				m_pAttribute->m_pThisNode = NULL;
				delete m_pAttribute;
				m_pAttribute = NULL;
				int iLastCode = GetLastExceptionCode( contextAttribute );
				if ( iLastCode != EX_ATTRIBUTE_DOESNT_EXIST && iLastCode != EX_ASSOCIATION_DOESNT_EXIST ) {
					ADDEXP( contextAttribute.m_poolExceptions );
					return false;
				}
			}
		}
		ADDEX( EXCEPTION1( EX_VAR_IMPL_ATTRIB_ASSOC_DOESNT_EXIST, m_strName, m_mapPositions[ LID_NODE_START ] ) );
		return false;
	}

	OclMeta::Object VariableNode::Evaluate( ObjectContext& context )
	{
		if ( m_pType )
			return m_pType->Evaluate( context );
		if ( m_pAttribute )
			return m_pAttribute->Evaluate( context );

		OclMeta::Object spObject;
		context.oCtx.GetVariable( m_strName, spObject );
		return spObject;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::ContextNode <<< + OclTree::TreeNode
//
//##############################################################################################################################################

	ContextNode::ContextNode( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_CONTEXT ), m_strName( "" ), m_strType( "" ), m_bClassifier( false ), m_strStereotype( "" ), m_strReturnType( "" )
	{
	}

	std::string ContextNode::Print( const std::string& strTabs ) const
	{
		std::string strOut = strTabs + "<Context type=\"" +( ( m_bClassifier ) ? "Classifier" : "Operation" ) + "\" stereotype=\"" + m_strStereotype + "\">\r\n";
		if ( m_bClassifier )
			strOut += strTabs + TABSTR + ( ( ! m_strName.empty() ) ? m_strName : "self" ) + " : " + m_strType + "\r\n";
		else {
			strOut += strTabs + TABSTR + m_strType + "::" + m_strName + "( ";
			for ( unsigned int i = 0 ; i < m_vecParameters.size() ; i++ ) {
				strOut += m_vecParameters[ i ].GetName() + " : " + m_vecParameters[ i ].GetTypeName();
				if ( i != m_vecParameters.size() - 1 )
					strOut += ", ";
			}
			strOut += " )" + ( ( ! m_strReturnType.empty() ) ? " : " + m_strReturnType : "" ) + "\r\n";
		}
		strOut += strTabs + "</Context>\r\n";
		return strOut;
	}

	void ContextNode::CheckInitialize()
	{
		TreeNode::CheckInitialize();
		m_setDependencies.clear();
		m_vecTypeReturn.clear();
	}

	bool ContextNode::CheckImplementation( TypeContext& context )
	{
		context.m_strStereotype = m_strStereotype;

		// C L A S S I F I E R   C O N T E X T

		// Check type of "self" and Add "self"

		if ( m_strStereotype == "inv" ) {
			if ( ! ParseTypeSeq( context, m_mapPositions[ LID_CONTEXT_TYPE ], m_strType, m_vecType ) )
				return false;
			context.m_ctxTypes.AddVariable( ( m_strName == "" ) ? "self" : m_strName, m_vecType );
			context.m_vecImplicits.push_back( ( m_strName == "" ) ? "self" : m_strName );
			m_vecTypeReturn.push_back( "ocl::Boolean" );
			return true;
		}

		// O P E R A T I O N   C O N T E X T

		// Check type of "self" and Add "self"

		if ( ParseTypeSeq( context, m_mapPositions[ LID_CONTEXT_TYPE ], m_strType, m_vecType ) ) {
			context.m_ctxTypes.AddVariable( "self", m_vecType );
			context.m_vecImplicits.push_back( "self" );
		}

		// Check Parameters

		TypeSeq vecType;
		for ( unsigned int i = 0 ; i < m_vecParameters.size() ; i++ ) {
			vecType.clear();
			std::string strType = m_vecParameters[ i ].GetTypeName();
			std::string strName = m_vecParameters[ i ].GetName();
			if ( ParseTypeSeq( context, m_mapPositions[ LID_PARAMETER_TYPE + i ], strType, vecType ) )
				m_vecParameters[ i ] = OclCommon::FormalParameter( strName, strType, m_vecParameters[ i ].IsRequired() );
			if ( context.m_ctxTypes.ExistsVariable( strName ) )
				ADDEX( EXCEPTION1( EX_VARIABLE_ALREADY_EXISTS, strName, m_mapPositions[ LID_PARAMETER_NAME + i ] ) )
			else
				if ( ! vecType.empty() )
					context.m_ctxTypes.AddVariable( strName, vecType );
		}

		// Check Return Type if exists

		if ( ! m_strReturnType.empty() )
			ParseTypeSeq( context, m_mapPositions[ LID_CONTEXT_RETURNTYPE ], m_strReturnType, m_vecTypeReturn );
		if ( m_strStereotype == "defattribute" || m_strStereotype == "defmethod" ) {
			if ( m_strReturnType.empty() )
				ADDEX( EXCEPTION0( EX_CONSTRAINT_DEF_RETURN_MISSING, m_mapPositions[ LID_NODE_START ] ) );
		}
		else
			m_vecTypeReturn.push_back( "ocl::Boolean" );
		if ( ! m_vecTypeReturn.empty() )
			context.m_ctxTypes.AddVariable( "result", m_vecTypeReturn, m_strStereotype == "defattribute" || m_strStereotype == "defmethod" );

		return ! m_vecType.empty();
	}

	OclMeta::Object ContextNode::Evaluate( ObjectContext& context )
	{
		OclMeta::Object spResult;
		context.oCtx.GetVariable( ( m_strName == "" ) ? "self" : m_strName, spResult );
		return spResult;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::Constraint <<< + OclTree::TreeNode
//
//##############################################################################################################################################

	Constraint::Constraint( TreeManager* pManager )
		: TreeNode( pManager, TreeNode::NK_CONSTRAINT ), m_strName( "" ), m_pContext( NULL ), m_pExpression( NULL )
	{
	}

	Constraint::~Constraint()
	{
		if ( m_pContext )
			delete m_pContext;
		if ( m_pExpression )
			delete m_pExpression;
	}

	std::string Constraint::Print( const std::string& strTabs ) const
	{
		std::string strOut = strTabs + strTabs + "<Constraint name=\"" + m_strName + "\">\r\n";

		strOut += strTabs + TABSTR + "<Context>\r\n";
		strOut += m_pContext->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</Context>\r\n";

		strOut += strTabs + TABSTR + "<Expression>\r\n";
		strOut += m_pExpression->Print( strTabs + TABSTR + TABSTR );
		strOut += strTabs + TABSTR + "</Expression>\r\n";

		strOut += "</Constraint>\r\n";
		return strOut;
	}

	bool Constraint::CheckImplementation( TypeContext& context )
	{
		m_pContext->Check( context );
		bool bValidC = context.m_poolExceptions.IsEmpty();
		bool bValidE = true;
		if ( m_pExpression ) {
			bValidE = m_pExpression->Check( context );
			if ( bValidE && bValidC ) {
				m_pContext->m_setDependencies = context.m_setDependencies;
				if ( ! GetTreeManager()->GetTypeManager()->IsTypeA( m_pExpression->m_vecType, m_pContext->m_vecTypeReturn ) ) {
					if ( m_pContext->m_strStereotype == "defattribute" || m_pContext->m_strStereotype == "defmethod" ) {
						std::string strEType, strDType;
						OclCommon::Convert( m_pExpression->m_vecType, strEType );
						OclCommon::Convert( m_pContext->m_vecTypeReturn, strDType );
						ADDEX( EXCEPTION2( EX_CONSTRAINT_DEF_RETURN_MISMATCH, strDType, strEType, m_pExpression->m_mapPositions[ LID_NODE_START ] ) );
					}
					else
						ADDEX( EXCEPTION0( EX_BOOLEAN_REQUIRED, m_pExpression->m_mapPositions[ LID_NODE_START ]  ) );
					bValidE = false;
				}
				else
					m_vecType = m_pExpression->m_vecType;
			}
		}
		return bValidC && bValidE;
	}

	OclMeta::Object Constraint::Evaluate( ObjectContext& context )
	{
		int iLineNodeStart = m_mapPositions[ LID_NODE_START ].iLine;
		EVALTRY {
			OclMeta::Object spResult = EvaluateCast( context, m_pExpression->Evaluate( context ), iLineNodeStart, "At root expression" );
			context.m_bEnableTracking = true;
			if ( spResult.IsUndefined() ) {
				std::string strMyself = ( m_pContext->m_strStereotype == "defattribute" || m_pContext->m_strStereotype == "defmethod" ) ? "Definition " : "Constraint ";
				AddViolation( context, iLineNodeStart, "At root expression", strMyself + "evaluated to undefined object." );
			}

			return CheckFalseResult( context, spResult, iLineNodeStart, "At root expression" );
		} EVALCATCH( iLineNodeStart, "At root expression" );
		return OclMeta::Object::UNDEFINED;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::ObjectNodeAdaptor
//
//##############################################################################################################################################

	std::string ObjectNodeAdaptor::Print( const std::string& strTabs, ObjectNode* pNode ) const
	{
		std::string strOut = strTabs + "<Object type=\"" + pNode->m_strType + "\">\r\n";

		strOut += strTabs + TABSTR + "<Value>\r\n";
		strOut += strTabs + TABSTR + TABSTR;
		if ( pNode->m_strType == "ocl::String" )
			strOut += "\"" + pNode->m_strValue + "\"\r\n";
		else
			if ( pNode->m_strType == "ocl::Enumeration" )
				strOut += "#" + pNode->m_strValue + "\r\n";
			else
				strOut += pNode->m_strValue + "\r\n";
		strOut += strTabs + TABSTR + "</Value>\r\n";

		strOut += strTabs +  "</Object>\r\n";
		return strOut;
	}

	bool ObjectNodeAdaptor::Check( TypeContext& context, ObjectNode* pNode ) const
	{
		if ( pNode->ParseTypeSeq( context, pNode->m_mapPositions[ LID_NODE_START ], pNode->m_strType, pNode->m_vecType ) ) {

			// Check whether this adaptor is able to create object

			if ( pNode->m_strType == "ocl::String" || pNode->m_strType == "ocl::Enumeration" || pNode->m_strType == "ocl::Real" || pNode->m_strType == "ocl::Integer" || pNode->m_strType == "ocl::Boolean" )
				return true;
			else {
				if ( pNode->m_strType == "ocl::Type" ) {
					std::string strValue = pNode->m_strValue;
					TypeSeq vecType;
					bool bIsType = pNode->ParseTypeSeq( context, pNode->m_mapPositions[ LID_NODE_START ], strValue, vecType );
					if ( ! bIsType )
						pNode->m_vecType.clear();
					else
						pNode->m_strValue = strValue;
					return bIsType;
				}
				ADDEX( EXCEPTION2( EX_CANNOT_CREATE_OBJECT, pNode->m_strType, pNode->m_strValue, pNode->m_mapPositions[ LID_NODE_START ] ) );
				pNode->m_vecType.clear();
			}
		}
		return false;
	}

	OclMeta::Object ObjectNodeAdaptor::Evaluate( ObjectContext& context, ObjectNode* pNode ) const
	{
		if ( pNode->m_strType == "ocl::String" )
			return CREATE_STRING( pNode->GetTreeManager()->GetTypeManager(), pNode->m_strValue );
		if ( pNode->m_strType == "ocl::Enumeration" )
			return CREATE_ENUMERATION( pNode->GetTreeManager()->GetTypeManager(), pNode->m_strValue );
		if ( pNode->m_strType == "ocl::Boolean" ) {
			return CREATE_BOOLEAN( pNode->GetTreeManager()->GetTypeManager(), pNode->m_strValue == "true" );
		}
		if ( pNode->m_strType == "ocl::Real" ) {
			char* pchStop;
			double dValue = strtod( pNode->m_strValue.c_str(), &pchStop );
			return CREATE_REAL( pNode->GetTreeManager()->GetTypeManager(), dValue );
		}
		if ( pNode->m_strType == "ocl::Integer" ) {
			long lValue = atol( pNode->m_strValue.c_str() );
			return CREATE_INTEGER( pNode->GetTreeManager()->GetTypeManager(), lValue );
		}
		if ( pNode->m_strType == "ocl::Type" )
			return CREATE_TYPE( pNode->GetTreeManager()->GetTypeManager(), pNode->m_strValue );
		return OclMeta::Object::UNDEFINED;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::CollectionNodeAdaptor
//
//##############################################################################################################################################

	std::string CollectionNodeAdaptor::Print( const std::string& strTabs, CollectionNode* pNode ) const
	{
		std::string strOut = strTabs + "<Collection type=\"" + pNode->m_strType + "\">\r\n";

		for ( unsigned int i = 0 ; i < pNode->m_vecNodes.size() ; i++ ) {
			char chNum[ 100 ];
			sprintf_s( chNum, sizeof(chNum), "%u", i );
			strOut += strTabs + TABSTR + "<Element_" + std::string( chNum ) + ">\r\n";
			strOut += pNode->m_vecNodes[ i ]->Print( strTabs + TABSTR + TABSTR );
			strOut += strTabs + TABSTR + "</Element_" + std::string( chNum ) + ">\r\n";
		}

		strOut += strTabs + "</Collection>\r\n";
		return strOut;
	}

	bool CollectionNodeAdaptor::Check( TypeContext& context, CollectionNode* pNode ) const
	{
		if ( pNode->m_strType == "Set" || pNode->m_strType == "Sequence" || pNode->m_strType == "Bag" || pNode->m_strType == "OrderedSet" )
			pNode->m_strType = "ocl::" + pNode->m_strType;

		bool bValid = pNode->ParseTypeSeq( context, pNode->m_mapPositions[ LID_NODE_START ], pNode->m_strType, pNode->m_vecType );

		// Check if this object is Compound

		if ( bValid ) {
			OclMeta::Type* pType = pNode->GetTreeManager()->GetTypeManager()->GetType( pNode->m_vecType[ 0 ], context.m_namespace ).get();
			if ( ! pType->IsCompound() ) {
				ADDEX( EXCEPTION1( EX_TYPE_ISNT_COMPOUND, pNode->m_vecType[ 0 ], pNode->m_mapPositions[ LID_NODE_START ] ) );
				bValid = false;
			}

			// Check if this adaptor can create Compound Object

			if ( ! ( pNode->m_vecType[ 0 ] == "ocl::Set" || pNode->m_vecType[ 0 ] == "ocl::Sequence" || pNode->m_vecType[ 0 ] == "ocl::Bag"  || pNode->m_vecType[ 0 ] == "ocl::OrderedSet" ) ) {
				ADDEX( EXCEPTION1( EX_CANNOT_CREATE_COLLECTION, pNode->m_strType, pNode->m_mapPositions[ LID_NODE_START ] ) );
				bValid = false;
			}
		}

		// Determine the base of consisted Objects

		bool bArgumentsValid = true;
		TypeSeq vecAggregatedType;
		for ( unsigned int i = 0 ; i < pNode->m_vecNodes.size() ; i++ ) {
			bArgumentsValid = pNode->m_vecNodes[ i ]->Check( context )  && bArgumentsValid;
			if ( bArgumentsValid ) {
				TypeSeq vecType = pNode->m_vecNodes[ i ]->m_vecType;
				if ( vecAggregatedType.empty() )
					vecAggregatedType = vecType;
				else
					vecAggregatedType = pNode->GetTreeManager()->GetTypeManager()->GetTypeBase( vecAggregatedType, vecType );
			}
		}

		// Set return type

		if ( bValid && bArgumentsValid ) {
			if ( vecAggregatedType.empty() )
				vecAggregatedType.push_back( "ocl::Any" );
			vecAggregatedType.insert( vecAggregatedType.begin(), pNode->m_vecType[ 0 ] );
			pNode->m_vecType = vecAggregatedType;
		}
		return bValid && bArgumentsValid;
	}

	OclMeta::Object CollectionNodeAdaptor::Evaluate( ObjectContext& context, CollectionNode* pNode ) const
	{
		OclMeta::ObjectVector vecObjects;
		for ( unsigned int i = 0 ; i < pNode->m_vecNodes.size() ; i++ )
			vecObjects.push_back( pNode->m_vecNodes[ i ]->Evaluate( context ) );

		if ( pNode->m_vecType[ 0 ] == "ocl::Set" )
			return CREATE_SET( pNode->GetTreeManager()->GetTypeManager(), vecObjects );
		if ( pNode->m_vecType[ 0 ] == "ocl::Bag" )
			return CREATE_BAG( pNode->GetTreeManager()->GetTypeManager(), vecObjects );
		if ( pNode->m_vecType[ 0 ] == "ocl::Sequence" )
			return CREATE_SEQUENCE( pNode->GetTreeManager()->GetTypeManager(), vecObjects );
		if ( pNode->m_vecType[ 0 ] == "ocl::OrderedSet" )
			return CREATE_ORDEREDSET( pNode->GetTreeManager()->GetTypeManager(), vecObjects );

		return OclMeta::Object::UNDEFINED;
	}

//##############################################################################################################################################
//
//	C L A S S : OclTree::TreeManager
//
//##############################################################################################################################################

	TreeManager::TreeManager( OclMeta::TypeManager* pTypeManager, ObjectNodeAdaptor* pOAdaptor, CollectionNodeAdaptor* pCAdaptor )
		: m_pTypeManager( pTypeManager ), m_pOAdaptor( pOAdaptor ), m_pCAdaptor( pCAdaptor )
	{
	}

	TreeManager::~TreeManager()
	{
		delete m_pTypeManager;
		delete m_pOAdaptor;
		delete m_pCAdaptor;
	}

	OclMeta::TypeManager* TreeManager::GetTypeManager() const
	{
		return m_pTypeManager;
	}

	ObjectNode* TreeManager::CreateObject() const
	{
		return new ObjectNode( (TreeManager*) this );
	}

	CollectionNode* TreeManager::CreateCollection() const
	{
		return new CollectionNode( (TreeManager*) this );
	}

	DeclarationNode* TreeManager::CreateDeclaration() const
	{
		return new DeclarationNode( (TreeManager*) this );
	}

	IteratorNode* TreeManager::CreateIterator() const
	{
		return new IteratorNode( (TreeManager*) this );
	}

	MethodNode* TreeManager::CreateMethod() const
	{
		return new MethodNode( (TreeManager*) this );
	}

	FunctionNode* TreeManager::CreateFunction() const
	{
		return new FunctionNode( (TreeManager*) this );
	}

	AssociationNode* TreeManager::CreateAssociation() const
	{
		return new AssociationNode( (TreeManager*) this );
	}

	AttributeNode* TreeManager::CreateAttribute() const
	{
		return new AttributeNode( (TreeManager*) this );
	}

	VariableNode* TreeManager::CreateVariable() const
	{
		return new VariableNode( (TreeManager*) this );
	}

	ContextNode* TreeManager::CreateContext() const
	{
		return new ContextNode( (TreeManager*) this );
	}

	TypeCastNode* TreeManager::CreateTypeCast() const
	{
		return new TypeCastNode( (TreeManager*) this );
	}

	IfThenElseNode* TreeManager::CreateIfThenElse() const
	{
		return new IfThenElseNode( (TreeManager*) this );
	}

	OperatorNode* TreeManager::CreateOperator() const
	{
		return new OperatorNode( (TreeManager*) this );
	}

	Constraint* TreeManager::CreateConstraint() const
	{
		return new Constraint( (TreeManager*) this );
	}

//udmoclpat_ocltree_cpp_1 __please do not remove or change this line__

}; // namespace OclTree

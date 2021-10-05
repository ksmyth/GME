//###############################################################################################################################################
//
//	Object Constraint Language Generic Manager
//	OCLContext.h
//
//###############################################################################################################################################

#ifndef OCLContext_h
#define OCLContext_h

#include "OCLCommon.h"

namespace OclCommon
{
	class Context;
	template < class TItem > class RealContext;
	template < class TItem > class ContextStack;

//##############################################################################################################################################
//
//	C L A S S : Context
//
//==============================================================================================================================================
//
//	D E S C R I P T I O N :
//
//##############################################################################################################################################

	class Context
	{
		public :
			enum ContextType { CT_EXCLUSIVE = 0 , CT_INCLUSIVE = 1 };

		private :
			ContextType m_eType;

		protected :
			Context( ContextType eType )
				: m_eType( eType )
			{
			}

		public :
			virtual ~Context()
			{
			}

			ContextType GetType() const
			{
				return m_eType;
			}
	};

//##############################################################################################################################################
//
//	T E M P L A T E   C L A S S : RealContext
//
//==============================================================================================================================================
//
//	D E S C R I P T I O N :
//
//##############################################################################################################################################

	template< class TItem >
	class RealContext
		: public Context
	{
		public :
			typedef struct CItem_tag { std::string name; TItem item; } CItem;
			typedef std::vector< CItem > CItemVector;


		private :
			struct StoreItem{ TItem object; bool bAssignable; };
			typedef std::map< std::string, StoreItem > TMap;

		private :
			TMap		m_mapVariables;

		public :
			RealContext( Context::ContextType eType )
				: Context( eType )
			{
			}

			RealContext<TItem>& operator=( const RealContext<TItem>& object )
			{
				if ( this != &object )
					m_mapVariables = object.m_mapVariables;
				Context::operator=( object );
				return *this;
			}

			bool ExistsVariable( const std::string& strName ) const
			{
				TItem object;
				return GetVariable( strName, object );
			}

			bool GetVariable( const std::string& strName, TItem& object ) const
			{
				StoreItem item;
				bool bExists = GetVariable( strName, item );
				if ( bExists )
					object = item.object;
				return bExists;
			}

			bool AddVariable( const std::string& strName, const TItem& object, bool bAssignable = false )
			{
				StoreItem item;
				if ( GetVariable( strName, item ) )
					return false;
				item.object = object;
				item.bAssignable = bAssignable;
#if (_MSC_VER == 1200)
				m_mapVariables.insert( TMap::value_type( strName, item ) ); 
#else
				m_mapVariables.insert( typename TMap::value_type( strName, item ) ); 
#endif
				return true;
			}

			bool IsAssignable( const std::string& strName, bool& bAssignable ) const
			{
				StoreItem item;
				bool bExists = GetVariable( strName, item );
				if ( bExists )
					bAssignable = item.bAssignable;
				return bExists;
			}

			bool RemoveVariable( const std::string& strName )
			{
				if ( ExistsVariable( strName ) ) {
					m_mapVariables.erase( strName );
					return true;
				}
				return false;
			}

			bool SetVariable( const std::string& strName, const TItem& object )
			{
				StoreItem item;
				bool bExists = GetVariable( strName, item );
				if ( ! bExists || ! item.bAssignable )
					return false;
				m_mapVariables.erase( strName );
				item.object = object;
#if (_MSC_VER == 1200)
				m_mapVariables.insert( TMap::value_type( strName, item ) ); 
#else
				m_mapVariables.insert( typename TMap::value_type( strName, item ) ); 
#endif
				return true;
			}

			CItemVector GetState() const
			{
				CItemVector vecItems;
				CItem selfItem;
				bool bHasSelf = false;
				for ( typename TMap::const_iterator i = m_mapVariables.begin() ; i != m_mapVariables.end() ; ++i ) {
					if ( (*i).first == "self" ) {
						bHasSelf = true;
						selfItem.name = "self";
						selfItem.item = (*i).second.object;
					}
					else {
						CItem cItem;
						cItem.name = (*i).first;
						cItem.item = (*i).second.object;
						vecItems.push_back( cItem );
					}
				}
				if ( bHasSelf ) {
					if ( vecItems.empty() )
						vecItems.push_back( selfItem );
					else
						vecItems.insert( vecItems.begin(), selfItem );
				}
				return vecItems;
			}

		private :
			bool GetVariable( const std::string& strName, StoreItem& item ) const
			{
				typename TMap::const_iterator i = m_mapVariables.find( strName );
				if ( i == m_mapVariables.end() )
					return false;
				item = (*i).second;
				return true;
			}
	};

//##############################################################################################################################################
//
//	T E M P L A T E   C L A S S : ContextStack
//
//==============================================================================================================================================
//
//	D E S C R I P T I O N :
//
//##############################################################################################################################################

	template< class TItem >
	class ContextStack
	{
		public :
			typedef typename RealContext< TItem >::CItem StateItem;
			typedef std::vector< StateItem > StateItemVector;

		private :
			typedef RealContext< TItem > CTItem;
			typedef std::vector< CTItem > CTVector;

		private :
			CTVector 			m_vecContexts;

		public :
			ContextStack()
			{
				AddContext( Context::CT_EXCLUSIVE );
			}

			ContextStack<TItem>& operator=( const ContextStack<TItem>& object )
			{
				if ( this != &object )
					m_vecContexts = object.m_vecContexts;
				return *this;
			}

			void AddContext( Context::ContextType eType )
			{
				m_vecContexts.push_back( CTItem( eType ) );
			}

			bool RemoveContext()
			{
				if ( ! m_vecContexts.empty() ) {
					typename CTVector::iterator i = m_vecContexts.end();
					--i;
					m_vecContexts.erase( i );
					return true;
				}
				return false;
			}

			bool ExistsVariable( const std::string& strName ) const
			{
				for ( int i = m_vecContexts.size() - 1 ; i >= 0 ; i-- )
					if ( m_vecContexts[ i ].ExistsVariable( strName ) )
						return true;
					else
						if ( m_vecContexts[ i ].GetType() == Context::CT_EXCLUSIVE )
							break;
				return false;
			}

			bool GetVariable( const std::string& strName, TItem& object ) const
			{
				for ( int i = m_vecContexts.size() - 1 ; i >= 0 ; i-- )
					if ( m_vecContexts[ i ].GetVariable( strName, object ) )
						return true;
					else
						if ( m_vecContexts[ i ].GetType() == Context::CT_EXCLUSIVE )
							break;
				return false;
			}

			bool IsAssignable( const std::string& strName, bool& bAssignable ) const
			{
				for ( int i = m_vecContexts.size() - 1 ; i >= 0 ; i-- )
					if ( m_vecContexts[ i ].IsAssignable( strName, bAssignable ) )
						return true;
					else
						if ( m_vecContexts[ i ].GetType() == Context::CT_EXCLUSIVE )
							break;
				return false;
			}

			bool AddVariable( const std::string& strName, const TItem& object, bool bAssignable = false, bool bRedefine = false )
			{
				if ( bRedefine && ! m_vecContexts[ m_vecContexts.size() - 1 ].ExistsVariable( strName ) || ! ExistsVariable( strName ) ) {
					m_vecContexts[ m_vecContexts.size() - 1 ].AddVariable( strName, object, bAssignable );
					return true;
				}
				return false;
			}

			bool RemoveVariable( const std::string& strName )
			{
				for ( int i = m_vecContexts.size() - 1 ; i >= 0 ; i-- )
					if ( m_vecContexts[ i ].RemoveVariable( strName ) )
						return true;
					else
						if ( m_vecContexts[ i ].GetType() == Context::CT_EXCLUSIVE )
							break;
				return false;
			}

			bool SetVariable( const std::string& strName, const TItem& object )
			{
				for ( int i = m_vecContexts.size() - 1 ; i >= 0 ; i-- )
					if ( m_vecContexts[ i ].SetVariable( strName, object ) )
						return true;
					else
						if ( m_vecContexts[ i ].GetType() == Context::CT_EXCLUSIVE )
							break;
				return false;
			}

			StateItemVector GetState() const
			{
				StateItemVector vecItemsOut;
				for ( unsigned int i = 0 ; i < m_vecContexts.size() ; i++ ) {
					StateItemVector vecItems = m_vecContexts[ i ].GetState();
					if ( i != 0 && vecItems[ 0 ].name == "self" ) {
						char chBuffer[ 100 ];
						sprintf_s( chBuffer, sizeof(chBuffer), "%u", i + 1 );
						vecItems[ 0 ].name = "self - " + std::string( chBuffer );
					}
					for ( unsigned int j = 0 ; j < vecItems.size() ; j++ )
						vecItemsOut.push_back( vecItems[ j ] );
				}
				return vecItemsOut;
			}
	};

}; // namespace OclCommon

#endif // OclContext_h

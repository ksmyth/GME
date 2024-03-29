/**
 * @copyright
 * ====================================================================
 * Copyright (c) 2003 CollabNet.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 *
 * This software consists of voluntary contributions made by many
 * individuals.  For exact contribution history, see the revision
 * history and logs, available at http://subversion.tigris.org/.
 * ====================================================================
 * @endcopyright
 *
 */
/**
 * Modified by Zoltan Molnar, Vanderbilt University, 2008
 */

#include "../StdAfx.h"

#if(USESVN)

#include "Pool.h"
#include "Util.h"
//#include "JNIMutex.h"
//#include "JNICriticalSection.h"
#include "svn_pools.h"

/**
 * Constructor to create one apr pool as the subpool of the global pool
 * store this pool as the request pool.
 */ 
Pool::Pool()
{
	//JNICriticalSection criticalSection(*JNIUtil::getGlobalPoolMutex());
	m_pool = svn_pool_create(NULL);
}

Pool::Pool(Pool& parent)
{
	//JNICriticalSection criticalSection(*JNIUtil::getGlobalPoolMutex());
	m_pool = svn_pool_create(parent.pool());
}

/**
 * Destructor to destroy the apr pool and to clear the request pool pointer
 */
Pool::~Pool()
{
	//JNICriticalSection criticalSection(*JNIUtil::getGlobalPoolMutex());
	if(m_pool)
	{
		svn_pool_destroy(m_pool);
	}

}

/**
 * Returns the apr pool.
 * @return the apr pool
 */
apr_pool_t * Pool::pool() const
{
	return m_pool;
}

#endif
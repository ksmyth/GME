#include "StdAfx.h"

#ifndef _WIN64

#include "SVNClient.h"
#include "svn_dso.h"
#include "svn_utf.h"
#include "svn_nls.h"
#include "svn_fs.h"
#include "svn_hash.h"
#include "svn_props.h"

#include "SVNDialogCommit.h"
#include "SVNDialogPlaintext.h"
#include "SVNDialogSSLServerTrust.h"
#include "SVNDialogLogin.h"

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shfolder.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "psapi.lib")


#define SVNTHROW(FUNC) \
	do { \
	svn_error_t* _err = (FUNC); \
	if( _err ) { \
	throw CSVNError(_err); \
	} \
	} while(false)


///////////////////////////////////////////////////////////////////////
// SVN Error
///////////////////////////////////////////////////////////////////////
CSVNError::CSVNError(svn_error_t* e) : svnError(e)
{
}

CSVNError::~CSVNError()
{
	svn_error_clear(svnError);
}

CString CSVNError::msg() const
{
	char buf[SVN_ERROR_MSG_MAX];

	const char *ret = svn_err_best_message(svnError, buf, sizeof(buf));
	return CString(ret);
}

///////////////////////////////////////////////////////////////////////
// SVN Pool
///////////////////////////////////////////////////////////////////////
CSVNPool::CSVNPool(apr_pool_t* parentPool)
{
	pool = svn_pool_create(parentPool);
}

CSVNPool::~CSVNPool()
{
	if (pool) {
		svn_pool_destroy(pool);
		pool = NULL;
	}
}

CSVNPool::operator apr_pool_ptr() const
{
	return pool;
}


///////////////////////////////////////////////////////////////////////
// SVN Client
///////////////////////////////////////////////////////////////////////
CSVNClient::CSVNClient() 
	: isInitialized(false), ctx(NULL), pool(NULL)
{
}


CSVNClient::~CSVNClient(void)
{
	
	POSITION p = svnFiles.GetHeadPosition();
	while (p) {
		delete svnFiles.GetNext(p);
	}
	// NOTE: there is a memory allocation problem in serf 
	// it causes problems when SSL certificates are rejected during conn. setup
	// svn_pool_destroy mitigates this problem (memory leaks "only" instead of accessing freed memory)
	svn_pool_destroy(pool); 
	apr_terminate();
}

void CSVNClient::initialize(void)
{
	apr_status_t status;

	// TODO: subversion/libsvn_subr/cmdline.c contains a lot of esoteric stuff
	// such as "setvbuf", input/output encodings, exception handlers, locale settings, etc.

	/* Initialize the APR subsystem, and register an atexit() function
	to Uninitialize that subsystem at program exit. */
	status = apr_initialize();
	if (status)
	{ 
		// this is not a real svn error, but coming from apr
		throw CSVNError(svn_error_create(status, NULL, NULL));
	}

	/* DSO pool must be created before any other pools used by the
	application so that pool cleanup doesn't unload DSOs too
	early. See docstring of svn_dso_initialize2(). */
	SVNTHROW(svn_dso_initialize2());

	/* Create a pool for use by the UTF-8 routines.  It will be cleaned
	up by APR at exit time. */
	pool = svn_pool_create(NULL);
	svn_utf_initialize2(FALSE, pool);
	SVNTHROW(svn_nls_init());

	/* Create top-level memory pool. */
	pool = svn_pool_create(NULL);

	/* Initialize the FS library. */
	//SVNTHROW(svn_fs_initialize(pool));

	/* Initialize the RA library. */
	SVNTHROW(svn_ra_initialize(pool));

	/* Make sure the ~/.subversion run-time config files exist */
	SVNTHROW(svn_config_ensure(NULL, pool));


	/* All clients need to fill out a client_ctx object. */
	{
		apr_hash_t *cfg_hash;
		svn_config_t *cfg_config;

		/* Load the run-time config file into a hash */
		SVNTHROW(svn_config_get_config (&cfg_hash, NULL, pool));

		cfg_config = (svn_config_t *)svn_hash_gets(cfg_hash, SVN_CONFIG_CATEGORY_CONFIG);

		/* Initialize and allocate the client_ctx object. */
		SVNTHROW(svn_client_create_context2(&ctx, cfg_hash, pool));


		/* Set the working copy administrative directory name. */
		if (getenv ("SVN_ASP_DOT_NET_HACK"))
		{
			SVNTHROW(svn_wc_set_adm_dir ("_svn", pool));
		}

		/* Callbacks */

		/* A func (& context) which receives event signals during checkouts, updates, commits, etc.  */
		ctx->notify_func2 = cbNotify;
		ctx->notify_baton2 = this;

		/* A func (& context) which can receive log messages */
		ctx->log_msg_func3 = cbLog;
		ctx->log_msg_baton3 = this;

		/* A func (& context) which checks whether the user cancelled */
		ctx->cancel_func = cbCancel;
		ctx->cancel_baton = this;

		/* A func (& context) for network progress */
		ctx->progress_func = cbProgress;
		ctx->progress_baton = this;

		/* A func (& context) for conflict resolution */
		ctx->conflict_func2 = cbConflict;
		ctx->conflict_baton2 = this;

		/* Make the client_ctx capable of authenticating users */
		{
			svn_auth_provider_object_t *provider;
			apr_array_header_t *providers;

			/* For caching encrypted username/password and client cert passwords - no prompting */
			SVNTHROW(svn_auth_get_platform_specific_client_providers(&providers, cfg_config, pool));

			/* For caching unencrypted username/password (also from config file) - prompting only to confirm storing creds in cleartext */
			svn_auth_get_simple_provider2(&provider, cbAuthPlaintextPrompt, this, pool);
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			/* For guessing and optionally saving username - no prompting */
			svn_auth_get_username_provider(&provider, pool);
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			/* For validating server SSL certs from windows certificate store - no prompting (windows itself might show a dialog ?) */
			SVNTHROW(svn_auth_get_platform_specific_provider(&provider, "windows", "ssl_server_trust", pool));
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			/* For validating and/or storing SSL server certs (in custom files) - no prompting */
			svn_auth_get_ssl_server_trust_file_provider(&provider, pool);
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			/* For retrieving custom client certificate (file path) from server config - no prompting */
			svn_auth_get_ssl_client_cert_file_provider(&provider, pool);
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			svn_auth_get_ssl_client_cert_pw_file_provider2(&provider, cbAuthPlaintextPassphrasePrompt, this, pool);
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			svn_auth_get_simple_prompt_provider(&provider, cbAuthSimplePrompt, this, 2 /* retry limit */, pool);
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			svn_auth_get_username_prompt_provider(&provider, cbAuthUsernamePrompt, this, 2 /* retry limit */, pool);
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			svn_auth_get_ssl_server_trust_prompt_provider(&provider, cbAuthSSLServerTrustPrompt, this, pool);
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			svn_auth_get_ssl_client_cert_pw_prompt_provider(&provider, cbAuthSSLClientCertPWPrompt, this, 2 /* retry limit */, pool);
			APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;

			/* If configuration allows, add a provider for client-cert path
			prompting, too. */
			svn_boolean_t ssl_client_cert_file_prompt;
			SVNTHROW(svn_config_get_bool(cfg_config, &ssl_client_cert_file_prompt,
				SVN_CONFIG_SECTION_AUTH,
				SVN_CONFIG_OPTION_SSL_CLIENT_CERT_FILE_PROMPT,
				FALSE));
			if (ssl_client_cert_file_prompt)
			{
				svn_auth_get_ssl_client_cert_prompt_provider(&provider, cbAuthSSLClientCertPrompt, this, 2 /* retry limit */, pool);
				APR_ARRAY_PUSH(providers, svn_auth_provider_object_t *) = provider;
			}

			/* Register the auth-providers into the context's auth_baton. */
			svn_auth_open (&ctx->auth_baton, providers, pool);
		}
	}

	isInitialized = true;
}

CSVNFile* CSVNClient::embraceFile(const CString & filePath)
{
	CSVNFile* svnFile = new CSVNFile(this, filePath);
	if (svnFile) {
		svnFiles.AddTail(svnFile);
	}
	return svnFile;
}

void CSVNClient::forgetFile(CSVNFile* svnFile)
{
	POSITION p = svnFiles.Find(svnFile);
	while (p) {
		delete svnFiles.GetAt(p);
		svnFiles.RemoveAt(p);
		p = svnFiles.Find(svnFile);
	}
}

///////////////////////////////////////////////////////////////////////
// SVN Client Context Callbacks
///////////////////////////////////////////////////////////////////////
void CSVNClient::cbNotify(
	void *baton, 
	const svn_wc_notify_t *notify, 
	apr_pool_t *pool)
{
	CSVNClient *self = (CSVNClient*)baton;
	ASSERT(self);
	self->lastNotifyAction = notify->action;
}

svn_error_t* CSVNClient::cbLog(
	const char **log_msg, 
	const char **tmp_file, 
	const apr_array_header_t *commit_items, 
	void *baton, 
	apr_pool_t *pool)
{
	CSVNClient *self = (CSVNClient*)baton;
	CSVNDialogCommit dlg;
	*log_msg = NULL;

	ASSERT(self);

	if (apr_is_empty_array(commit_items)) {
		return SVN_NO_ERROR;
	}
	
	svn_client_commit_item3_t* item = APR_ARRAY_IDX(commit_items, 0, svn_client_commit_item3_t*);
	dlg.filename = item->path;
	dlg.repository = item->url;
	dlg.revision = item->revision;
	if (dlg.DoModal() == IDOK) {
		svn_string_t* logMsg = svn_string_create(CStringA(dlg.logMessage), pool);
		*log_msg = logMsg->data;
		self->canceledOperation = false;
	}
	else {
		self->canceledOperation = true;
	}
	
	return SVN_NO_ERROR;
}

svn_error_t* CSVNClient::cbCancel(void *cancel_baton)
{
	// not needed now (cancel button on user interfaces ?)
	return SVN_NO_ERROR;
}

void CSVNClient::cbProgress(
	apr_off_t progress, 
	apr_off_t total, 
	void *baton, 
	apr_pool_t *pool)
{
	//TODO: implement this
}

svn_error_t* CSVNClient::cbConflict(
	svn_wc_conflict_result_t **result, 
	const svn_wc_conflict_description2_t *description, 
	void *baton, apr_pool_t *result_pool, 
	apr_pool_t *scratch_pool)
{
	// too advanced to handle in this client
	return SVN_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////
// SVN Client Auth Callbacks
///////////////////////////////////////////////////////////////////////

//
// Auth callback function for asking whether storing a password to disk in plaintext is allowed.
// 
svn_error_t* CSVNClient::cbAuthPlaintextPrompt(
	svn_boolean_t *may_save_plaintext, 
	const char *realmstring, 
	void *baton, 
	apr_pool_t *pool)
{
	CSVNDialogPlaintext dlg;
	dlg.realm = realmstring;
	*may_save_plaintext = (dlg.DoModal() == IDOK) ? TRUE : FALSE;
	return SVN_NO_ERROR;
}

svn_error_t* CSVNClient::cbAuthPlaintextPassphrasePrompt(
	svn_boolean_t *may_save_plaintext, 
	const char *realmstring, 
	void *baton, 
	apr_pool_t *pool)
{
	CSVNDialogPlaintext dlg;
	dlg.realm = realmstring;
	*may_save_plaintext = (dlg.DoModal() == IDOK) ? TRUE : FALSE;
	return SVN_NO_ERROR;
}

svn_error_t* CSVNClient::cbAuthSimplePrompt(
	svn_auth_cred_simple_t **cred, 
	void *baton, 
	const char *realm, 
	const char *username, 
	svn_boolean_t may_save, 
	apr_pool_t *pool)
{
	CSVNDialogLogin dlg;
	dlg.realm = realm;
	dlg.passwordEnabled = TRUE;
	dlg.permanentEnabled = may_save;

	if (dlg.DoModal() == IDOK) {
		svn_auth_cred_simple_t *ret = (svn_auth_cred_simple_t *)apr_pcalloc(pool, sizeof(*ret));
		svn_string_t* username_s = svn_string_create(CStringA(dlg.username), pool);
		ret->username = username_s ? username_s->data : NULL;
		svn_string_t* password_s = svn_string_create(CStringA(dlg.password), pool);
		ret->password = password_s ? password_s->data : NULL;
		ret->may_save = dlg.permanent;
		*cred = ret;
	}
	else {
		*cred = NULL;
	}
	return SVN_NO_ERROR;
}

svn_error_t* CSVNClient::cbAuthUsernamePrompt(
	svn_auth_cred_username_t **cred, 
	void *baton, 
	const char *realm, 
	svn_boolean_t may_save, 
	apr_pool_t *pool)
{
	CSVNDialogLogin dlg;
	dlg.realm = realm;
	dlg.passwordEnabled = FALSE;
	dlg.permanentEnabled = may_save;

	if (dlg.DoModal() == IDOK) {
		svn_auth_cred_username_t *ret = (svn_auth_cred_username_t *)apr_pcalloc(pool, sizeof(*ret));
		svn_string_t* username_s = svn_string_create(CStringA(dlg.username), pool);
		ret->username = username_s ? username_s->data : NULL;
		ret->may_save = dlg.permanent;
		*cred = ret;
	}
	else {
		*cred = NULL;
	}
	return SVN_NO_ERROR;
}

svn_error_t* CSVNClient::cbAuthSSLServerTrustPrompt(
	svn_auth_cred_ssl_server_trust_t **cred, 
	void *baton, 
	const char *realm, 
	apr_uint32_t failures, 
	const svn_auth_ssl_server_cert_info_t *cert_info, 
	svn_boolean_t may_save, 
	apr_pool_t *pool)
{
	CSVNDialogSSLServerTrust dlg;
	dlg.host = cert_info->hostname;
	dlg.fingerprint = cert_info->fingerprint;
	dlg.issuer = cert_info->issuer_dname;
	dlg.permanentEnabled = may_save;

	if (failures & SVN_AUTH_SSL_NOTYETVALID) {
		dlg.problems += _T("Certificate is not yet valid.\r");
	}
	if (failures & SVN_AUTH_SSL_EXPIRED) {
		dlg.problems += _T("Certificate has expired.\r");
	}
	if (failures & SVN_AUTH_SSL_CNMISMATCH) {
		dlg.problems += _T("Certificate's CN does not match the remote hostname.\r");
	}
	if (failures & SVN_AUTH_SSL_UNKNOWNCA) {
		dlg.problems += _T("Certificate authority is unknown or not trusted.\r");
	}
	if (failures & SVN_AUTH_SSL_OTHER) {
		dlg.problems += _T("Other failure.\r");
	}

	if (dlg.DoModal() == IDOK) {
		svn_auth_cred_ssl_server_trust_t *ret = (svn_auth_cred_ssl_server_trust_t *)apr_pcalloc(pool, sizeof(*ret));
		ret->accepted_failures = failures; 
		ret->may_save = dlg.permanent;
		*cred = ret;
	}
	else {
		*cred = NULL;
	}

	return SVN_NO_ERROR;
}

svn_error_t* CSVNClient::cbAuthSSLClientCertPWPrompt(
	svn_auth_cred_ssl_client_cert_pw_t **cred, 
	void *baton, 
	const char *realm, 
	svn_boolean_t may_save, 
	apr_pool_t *pool)
{
	// Unsupported
	*cred = NULL;
	return SVN_NO_ERROR;
}

svn_error_t* CSVNClient::cbAuthSSLClientCertPrompt(
	svn_auth_cred_ssl_client_cert_t **cred, 
	void *baton, 
	const char *realm, 
	svn_boolean_t may_save, 
	apr_pool_t *pool)
{
	// Unsupported
	*cred = NULL;
	return SVN_NO_ERROR;
}


///////////////////////////////////////////////////////////////////////
// SVN File
///////////////////////////////////////////////////////////////////////
CSVNFile::CSVNFile(CSVNClient* client, const CString & filePath) 
	: client(client), filePath(filePath), versioned(false), tracked(false), owned(false), latest(false)
{
}

CSVNFile::~CSVNFile()
{
}

void CSVNFile::updateStatus(bool checkServer)
{
	svn_error_t* e;

	if (client->isInitialized) {
		CSVNPool scratch_pool(client->pool);

		svn_opt_revision_t revision = {svn_opt_revision_head, {0}};

		e = svn_client_status5(NULL, client->ctx, CStringA(filePath), 
			&revision, svn_depth_immediates, TRUE, checkServer ? TRUE : FALSE, 
			FALSE, FALSE, TRUE, NULL, cbStatus, this, scratch_pool);

		if (e && e->apr_err == SVN_ERR_WC_NOT_WORKING_COPY) {
			versioned = tracked = owned = latest = false;
			svn_error_clear(e);
		}
		else {
			SVNTHROW(e);
		}
	}
}

bool CSVNFile::isVersioned()
{
	updateStatus();
	return versioned;
}

bool CSVNFile::isTracked()
{
	updateStatus();
	return tracked;
}

bool CSVNFile::isOwned()
{
	updateStatus();
	return owned;
}

bool CSVNFile::isLatest()
{
	updateStatus(true);
	return latest;
}

bool CSVNFile::update()
{
	CStringA filePathA(filePath);
	const char* target = filePathA;
	CSVNPool scratch_pool(client->pool);
	apr_array_header_t* targets = apr_array_make(scratch_pool, 1, sizeof(target));
	APR_ARRAY_PUSH(targets, const char*) = target;

	svn_opt_revision_t revision = {svn_opt_revision_head, {0}};

	SVNTHROW(svn_client_update4(NULL, targets, &revision, svn_depth_files, FALSE, FALSE, FALSE,
				TRUE, FALSE, client->ctx, scratch_pool)); 

	return (client->lastNotifyAction == svn_wc_notify_update_completed);
}

bool CSVNFile::takeOwnership()
{
	CStringA filePathA(filePath);
	const char* target = filePathA;
	CSVNPool scratch_pool(client->pool);
	apr_array_header_t* targets = apr_array_make(scratch_pool, 1, sizeof(target));
	APR_ARRAY_PUSH(targets, const char*) = target;
	
	SVNTHROW(svn_client_lock(targets, "GME auto-locking", FALSE, client->ctx, scratch_pool));

	return (client->lastNotifyAction == svn_wc_notify_locked);
} 

bool CSVNFile::commit()
{
	CStringA filePathA(filePath);
	const char* target = filePathA;
	CSVNPool scratch_pool(client->pool);
	apr_array_header_t* targets = apr_array_make(scratch_pool, 1, sizeof(target));
	APR_ARRAY_PUSH(targets, const char*) = target;
	
	SVNTHROW(svn_client_commit6(targets, svn_depth_immediates, FALSE, FALSE, FALSE, 
		FALSE, FALSE, NULL, NULL, NULL, NULL, client->ctx, scratch_pool));
	
	// commit does not release the lock if the file was not changed (empty commit)
	if (!client->canceledOperation) {
		updateStatus();
		if (owned) {
			SVNTHROW(svn_client_unlock(targets, FALSE, client->ctx, scratch_pool));
			updateStatus();
		}
	}

	return !client->canceledOperation;
}

///////////////////////////////////////////////////////////////////////
// Operation Callbacks
///////////////////////////////////////////////////////////////////////
svn_error_t* CSVNFile::cbStatus(
	void *baton, 
	const char *path, 
	const svn_client_status_t *status, 
	apr_pool_t *scratch_pool)
{
	CSVNFile* self = (CSVNFile*)baton;

	if (status->versioned) {
		self->versioned = true;
		self->latest = (status->ood_changed_rev == SVN_INVALID_REVNUM);
		self->owned = (status->lock != NULL);

		self->tracked = false;
		apr_hash_t* props;
		svn_opt_revision_t revision = {svn_opt_revision_base, {0}};

		SVN_ERR(svn_client_propget(&props, SVN_PROP_NEEDS_LOCK, CStringA(self->filePath), 
			&revision, FALSE, self->client->ctx, scratch_pool));
		
		if (apr_hash_count(props)) {
			svn_string_t *hval;
			apr_hash_index_t* hi = apr_hash_first(scratch_pool, props);
			apr_hash_this(hi, NULL, 0, (void**)&hval);
			if (hval && !svn_string_isempty(hval)) {
				self->tracked = true;
			}
		}

	}

	return SVN_NO_ERROR;
}

#endif // #ifndef _WIN64

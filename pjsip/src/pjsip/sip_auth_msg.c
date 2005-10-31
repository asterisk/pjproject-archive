/* $Header: /pjproject/pjsip/src/pjsip/sip_auth_msg.c 9     8/31/05 9:05p Bennylp $ */
#include <pjsip/sip_auth_msg.h>
#include <pjsip/sip_auth_parser.h>
#include <pj/pool.h>
#include <pj/list.h>
#include <pj/string.h>
#include <pjsip/print.h>

///////////////////////////////////////////////////////////////////////////////
/*
 * Authorization and Proxy-Authorization header.
 */
static pjsip_authorization_hdr* pjsip_authorization_hdr_clone( pj_pool_t *pool,
							       const pjsip_authorization_hdr *hdr);
static pjsip_authorization_hdr* pjsip_authorization_hdr_shallow_clone( pj_pool_t *pool,
								       const pjsip_authorization_hdr *hdr);
static int pjsip_authorization_hdr_print( pjsip_authorization_hdr *hdr,
					  char *buf, pj_size_t size);

static pjsip_hdr_vptr authorization_hdr_vptr = 
{
    (pjsip_hdr_clone_fptr) &pjsip_authorization_hdr_clone,
    (pjsip_hdr_clone_fptr) &pjsip_authorization_hdr_shallow_clone,
    (pjsip_hdr_print_fptr) &pjsip_authorization_hdr_print,
};


PJ_DEF(pjsip_authorization_hdr*) pjsip_authorization_hdr_create(pj_pool_t *pool)
{
    pjsip_authorization_hdr *hdr = pj_pool_calloc(pool, 1, sizeof(*hdr));
    init_hdr(hdr, PJSIP_H_AUTHORIZATION, &authorization_hdr_vptr);
    return hdr;
}

PJ_DEF(pjsip_proxy_authorization_hdr*) pjsip_proxy_authorization_hdr_create(pj_pool_t *pool)
{
    pjsip_proxy_authorization_hdr *hdr = pj_pool_calloc(pool, 1, sizeof(*hdr));
    init_hdr(hdr, PJSIP_H_PROXY_AUTHORIZATION, &authorization_hdr_vptr);
    return hdr;
}

static int print_digest_credential(pjsip_digest_credential *cred, char *buf, pj_size_t size)
{
    int printed;
    char *startbuf = buf;
    char *endbuf = buf + size;
    
    copy_advance_pair_quote_cond(buf, "username=", 9, cred->username, '"', '"');
    copy_advance_pair_quote_cond(buf, ", realm=", 8, cred->realm, '"', '"');
    copy_advance_pair_quote_cond(buf, ", nonce=", 8, cred->nonce, '"', '"');
    copy_advance_pair_quote_cond(buf, ", uri=", 6, cred->uri, '"', '"');
    copy_advance_pair_quote_cond(buf, ", response=", 11, cred->response, '"', '"');
    copy_advance_pair(buf, ", algorithm=", 12, cred->algorithm);
    copy_advance_pair_quote_cond(buf, ", cnonce=", 9, cred->cnonce, '"', '"');
    copy_advance_pair_quote_cond(buf, ", opaque=", 9, cred->opaque, '"', '"');
    //Note: there's no dbl-quote in qop in Authorization header 
    // (unlike WWW-Authenticate)
    //copy_advance_pair_quote_cond(buf, ", qop=", 6, cred->qop, '"', '"');
    copy_advance_pair(buf, ", qop=", 6, cred->qop);
    copy_advance_pair(buf, ", nc=", 5, cred->nc);
    copy_advance(buf, cred->other_param);

    return (int) (buf-startbuf);
}

static int print_pgp_credential(pjsip_pgp_credential *cred, char *buf, pj_size_t size)
{
    PJ_UNUSED_ARG(cred)
    PJ_UNUSED_ARG(buf)
    PJ_UNUSED_ARG(size)
    return -1;
}

static int pjsip_authorization_hdr_print( pjsip_authorization_hdr *hdr,
					  char *buf, pj_size_t size)
{
    int printed;
    char *startbuf = buf;
    char *endbuf = buf + size;

    copy_advance(buf, hdr->name);
    *buf++ = ':';
    *buf++ = ' ';

    copy_advance(buf, hdr->scheme);
    *buf++ = ' ';

    if (pj_stricmp(&hdr->scheme, &pjsip_DIGEST_STR) == 0)
    {
	printed = print_digest_credential(&hdr->credential.digest, buf, endbuf - buf);
    } 
    else if (pj_stricmp(&hdr->scheme, &pjsip_PGP_STR) == 0)
    {
	printed = print_pgp_credential(&hdr->credential.pgp, buf, endbuf - buf);
    } 
    else {
	pj_assert(0);
	return -1;
    }

    if (printed == -1)
	return -1;

    buf += printed;
    *buf = '\0';
    return (int)(buf-startbuf);
}

static pjsip_authorization_hdr* pjsip_authorization_hdr_clone(  pj_pool_t *pool,
								const pjsip_authorization_hdr *rhs)
{
    /* This function also serves Proxy-Authorization header. */
    pjsip_authorization_hdr *hdr;
    if (rhs->type == PJSIP_H_AUTHORIZATION)
	hdr = pjsip_authorization_hdr_create(pool);
    else
	hdr = pjsip_proxy_authorization_hdr_create(pool);

    pj_strdup(pool, &hdr->scheme, &rhs->scheme);

    if (pj_stricmp2(&hdr->scheme, "digest") == 0) {
	pj_strdup(pool, &hdr->credential.digest.username, &rhs->credential.digest.username);
	pj_strdup(pool, &hdr->credential.digest.realm, &rhs->credential.digest.realm);
	pj_strdup(pool, &hdr->credential.digest.nonce, &rhs->credential.digest.nonce);
	pj_strdup(pool, &hdr->credential.digest.uri, &rhs->credential.digest.uri);
	pj_strdup(pool, &hdr->credential.digest.response, &rhs->credential.digest.response);
	pj_strdup(pool, &hdr->credential.digest.algorithm, &rhs->credential.digest.algorithm);
	pj_strdup(pool, &hdr->credential.digest.cnonce, &rhs->credential.digest.cnonce);
	pj_strdup(pool, &hdr->credential.digest.opaque, &rhs->credential.digest.opaque);
	pj_strdup(pool, &hdr->credential.digest.qop, &rhs->credential.digest.qop);
	pj_strdup(pool, &hdr->credential.digest.nc, &rhs->credential.digest.nc);
	pj_strdup(pool, &hdr->credential.digest.other_param, &rhs->credential.digest.other_param);
    } else if (pj_stricmp2(&hdr->scheme, "pgp") == 0) {
	pj_assert(0);
	return NULL;
    } else {
	pj_assert(0);
	return NULL;
    }

    return hdr;
}

static pjsip_authorization_hdr* pjsip_authorization_hdr_shallow_clone( pj_pool_t *pool,
								       const pjsip_authorization_hdr *rhs)
{
    /* This function also serves Proxy-Authorization header. */
    pjsip_authorization_hdr *hdr = pj_pool_alloc(pool, sizeof(*hdr));
    pj_memcpy(hdr, rhs, sizeof(*hdr));
    return hdr;
}


///////////////////////////////////////////////////////////////////////////////
/*
 * Proxy-Authenticate and WWW-Authenticate header.
 */
static int pjsip_www_authenticate_hdr_print( pjsip_www_authenticate_hdr *hdr,
					     char *buf, pj_size_t size);
static pjsip_www_authenticate_hdr* pjsip_www_authenticate_hdr_clone( pj_pool_t *pool,
								     const pjsip_www_authenticate_hdr *hdr);
static pjsip_www_authenticate_hdr* pjsip_www_authenticate_hdr_shallow_clone( pj_pool_t *pool,
									     const pjsip_www_authenticate_hdr *hdr);

static pjsip_hdr_vptr www_authenticate_hdr_vptr = 
{
    (pjsip_hdr_clone_fptr) &pjsip_www_authenticate_hdr_clone,
    (pjsip_hdr_clone_fptr) &pjsip_www_authenticate_hdr_shallow_clone,
    (pjsip_hdr_print_fptr) &pjsip_www_authenticate_hdr_print,
};


PJ_DEF(pjsip_www_authenticate_hdr*) pjsip_www_authenticate_hdr_create(pj_pool_t *pool)
{
    pjsip_www_authenticate_hdr *hdr = pj_pool_calloc(pool, 1, sizeof(*hdr));
    init_hdr(hdr, PJSIP_H_WWW_AUTHENTICATE, &www_authenticate_hdr_vptr);
    return hdr;
}


PJ_DEF(pjsip_proxy_authenticate_hdr*) pjsip_proxy_authenticate_hdr_create(pj_pool_t *pool)
{
    pjsip_proxy_authenticate_hdr *hdr = pj_pool_calloc(pool, 1, sizeof(*hdr));
    init_hdr(hdr, PJSIP_H_PROXY_AUTHENTICATE, &www_authenticate_hdr_vptr);
    return hdr;
}

static int print_digest_challenge( pjsip_digest_challenge *chal,
				   char *buf, pj_size_t size)
{
    int printed;
    char *startbuf = buf;
    char *endbuf = buf + size;

    copy_advance_pair_quote_cond(buf, " realm=", 7, chal->realm, '"', '"');
    copy_advance_pair_quote_cond(buf, ",domain=", 8, chal->domain, '"', '"');
    copy_advance_pair_quote_cond(buf, ",nonce=", 7, chal->nonce, '"', '"');
    copy_advance_pair_quote_cond(buf, ",opaque=", 8, chal->opaque, '"', '"');
    if (chal->stale) {
	pj_str_t true_str = { "true", 4 };
	copy_advance_pair(buf, ",stale=", 7, true_str);
    }
    copy_advance_pair(buf, ",algorithm=", 11, chal->algorithm);
    copy_advance_pair_quote_cond(buf, ",qop=", 5, chal->qop, '"', '"');
    copy_advance(buf, chal->other_param);

    return (int)(buf-startbuf);
}

static int print_pgp_challenge( pjsip_pgp_challenge *chal,
			        char *buf, pj_size_t size)
{
    PJ_UNUSED_ARG(chal)
    PJ_UNUSED_ARG(buf)
    PJ_UNUSED_ARG(size)
    return -1;
}

static int pjsip_www_authenticate_hdr_print( pjsip_www_authenticate_hdr *hdr,
					     char *buf, pj_size_t size)
{
    int printed;
    char *startbuf = buf;
    char *endbuf = buf + size;

    copy_advance(buf, hdr->name);
    *buf++ = ':';
    *buf++ = ' ';

    copy_advance(buf, hdr->scheme);
    *buf++ = ' ';

    if (pj_stricmp2(&hdr->scheme, "digest") == 0)
	printed = print_digest_challenge(&hdr->challenge.digest, buf, endbuf - buf);
    else if (pj_stricmp2(&hdr->scheme, "pgp") == 0)
	printed = print_pgp_challenge(&hdr->challenge.pgp, buf, endbuf - buf);
    else {
	pj_assert(0);
	return -1;
    }

    if (printed == -1)
	return -1;

    buf += printed;
    *buf = '\0';
    return (int)(buf-startbuf);
}

static pjsip_www_authenticate_hdr* pjsip_www_authenticate_hdr_clone( pj_pool_t *pool,
								     const pjsip_www_authenticate_hdr *rhs)
{
    /* This function also serves Proxy-Authenticate header. */
    pjsip_www_authenticate_hdr *hdr;
    if (rhs->type == PJSIP_H_WWW_AUTHENTICATE)
	hdr = pjsip_www_authenticate_hdr_create(pool);
    else
	hdr = pjsip_proxy_authenticate_hdr_create(pool);

    pj_strdup(pool, &hdr->scheme, &rhs->scheme);

    if (pj_stricmp2(&hdr->scheme, "digest") == 0) {
	pj_strdup(pool, &hdr->challenge.digest.realm, &rhs->challenge.digest.realm);
	pj_strdup(pool, &hdr->challenge.digest.domain, &rhs->challenge.digest.domain);
	pj_strdup(pool, &hdr->challenge.digest.nonce, &rhs->challenge.digest.nonce);
	pj_strdup(pool, &hdr->challenge.digest.opaque, &rhs->challenge.digest.opaque);
	hdr->challenge.digest.stale = rhs->challenge.digest.stale;
	pj_strdup(pool, &hdr->challenge.digest.algorithm, &rhs->challenge.digest.algorithm);
	pj_strdup(pool, &hdr->challenge.digest.qop, &rhs->challenge.digest.qop);
	pj_strdup(pool, &hdr->challenge.digest.other_param, &rhs->challenge.digest.other_param);
    } else if (pj_stricmp2(&hdr->scheme, "pgp") == 0) {
	pj_assert(0);
	return NULL;
    } else {
	pj_assert(0);
	return NULL;
    }

    return hdr;

}

static pjsip_www_authenticate_hdr* pjsip_www_authenticate_hdr_shallow_clone( pj_pool_t *pool,
									     const pjsip_www_authenticate_hdr *rhs)
{
    /* This function also serves Proxy-Authenticate header. */
    pjsip_www_authenticate_hdr *hdr = pj_pool_alloc(pool, sizeof(*hdr));
    pj_memcpy(hdr, rhs, sizeof(*hdr));
    return hdr;
}



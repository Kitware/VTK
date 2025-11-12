/*********************************************************************
*    Copyright 2018, UCAR/Unidata
*    See netcdf/COPYRIGHT file for copying and redistribution conditions.
* ********************************************************************/

/* The Curl code used here (nccurl_sha256.[ch] and nccurl_hmac.[ch]
   were taken from libcurl version 7.88.1. To upgrade this code,
   do a diff between that version of curl and the new one and transfer
   any relevant changes to this code.
*/

#ifndef NCCURL_SETUP_H
#define NCCURL_SETUP_H

#include "config.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include "ncexternl.h"

/* Please keep the SSL backend-specific #if branches in this order:
 *
 * 1. USE_OPENSSL
 * 2. USE_GNUTLS
 * 3. USE_MBEDTLS
 * 4. USE_COMMON_CRYPTO
 * 5. USE_WIN32_CRYPTO
 *
 * This ensures that the same SSL branch gets activated throughout this source
 * file even if multiple backends are enabled at the same time.
 */

#if defined(_WIN32)
#define USE_WIN32_CRYPTO
#elif ! defined(__APPLE__)
#define USE_OPENSSL
#endif

#define CURLX_FUNCTION_CAST(target_type, func) (target_type)(void (*) (void))(func)

#define DEBUGASSERT(expr)

#define CURL_MASK_UINT    ((unsigned int)~0)

extern uintmax_t strtoumax(const char *nptr, char **endptr, int base);
extern unsigned int nccurlx_uztoui(size_t uznum);

#endif /*NCCURL_SETUP_H*/

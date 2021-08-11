/*
 *	Copyright 2018, University Corporation for Atmospheric Research
 *      See netcdf/COPYRIGHT file for copying and redistribution conditions.
 */

#ifndef NCURI_H
#define NCURI_H

#include "ncexternl.h"

/* Define flags to control what is included by ncuribuild*/
#define NCURIPATH	    1
#define NCURIPWD	    2
#define NCURIQUERY	    4
#define NCURIFRAG	    8
#define NCURIENCODEPATH	    16 /* If output url path should be encoded */
#define NCURIENCODEQUERY    32 /* If output url query should be encoded */
#define NCURIENCODE	    (NCURIENCODEPATH|NCURIENCODEQUERY)
#define NCURIBASE	    (NCURIPWD|NCURIPATH)
#define NCURISVC	    (NCURIQUERY|NCURIBASE) /* for sending to server  */
#define NCURIALL	    (NCURIPATH|NCURIPWD|NCURIQUERY|NCURIFRAG) /* for rebuilding after changes */

/*! This is an open structure meaning
	it is ok to directly access its fields
*/
typedef struct NCURI {
    char* uri; /* copy of url as passed by the caller */
    char* protocol;
    char* user; /* from user:password@ */
    char* password; /* from user:password@ */
    char* host;	      /*!< host*/
    char* port;	      /*!< port */
    char* path;	      /*!< path */
    char* query;      /*!< query */
    char* fragment;   /*!< fragment */
    char** fraglist; /* envv style list of decomposed fragment*/
    char** querylist; /* envv style list of decomposed query*/
#if 0
    char* projection; /*!< without leading '?'*/
    char* selection;  /*!< with leading '&'*/
#endif
} NCURI;

#if 0
/* Declaration modifiers for DLL support (MSC et al) */
#if defined(DLL_NETCDF) /* define when library is a DLL */
#  if defined(DLL_EXPORT) /* define when building the library */
#   define MSC_EXTRA __declspec(dllexport)
#  else
#   define MSC_EXTRA __declspec(dllimport)
#  endif
#  include <io.h>
#else
#define MSC_EXTRA  /**< Needed for DLL build. */
#endif  /* defined(DLL_NETCDF) */

#define EXTERNL MSC_EXTRA extern /**< Needed for DLL build. */
#endif

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__) || defined(__CPLUSPLUS)
extern "C" {
#endif

EXTERNL int ncuriparse(const char* s, NCURI** ncuri);
EXTERNL void ncurifree(NCURI* ncuri);

/* Replace the protocol */
EXTERNL int ncurisetprotocol(NCURI*,const char* newprotocol);

/* Replace the path */
EXTERNL int ncurisetpath(NCURI*,const char* newpath);

/* Replace the constraints */
EXTERNL int ncurisetquery(NCURI*,const char* query);

/* Replace the fragment list */
EXTERNL int ncurisetfragments(NCURI*, const char* fragments);

/* Replace a specific &key=...& in uri fragment */
EXTERNL int ncurisetfragmentkey(NCURI* duri,const char* key, const char* value);

/* append a specific &key=...& in uri fragment */
EXTERNL int ncuriappendfragmentkey(NCURI* duri,const char* key, const char* value);

/* Construct a complete NC URI; caller frees returned string */
EXTERNL char* ncuribuild(NCURI*,const char* prefix, const char* suffix, int flags);

/*! Search the fragment for a given parameter
    Null result => entry not found; !NULL=>found;
    In any case, the result is imutable and should not be free'd.
*/
EXTERNL const char* ncurifragmentlookup(NCURI*, const char* param);

/*! Search the query for a given parameter
    Null result => entry not found; !NULL=>found;
    In any case, the result is imutable and should not be free'd.
*/
EXTERNL const char* ncuriquerylookup(NCURI*, const char* param);

/* Obtain the complete list of fragment pairs in envv format */
EXTERNL const char** ncurifragmentparams(NCURI*);

/* Obtain the complete list of query pairs in envv format */
EXTERNL const char** ncuriqueryparams(NCURI*);

/* URL Encode/Decode */
EXTERNL char* ncuridecode(const char* s);
/* Partial decode */
EXTERNL char* ncuridecodepartial(const char* s, const char* decodeset);
/* Encode using specified character set */
EXTERNL char* ncuriencodeonly(const char* s, const char* allowable);
/* Encode user or pwd */
EXTERNL char* ncuriencodeuserpwd(const char* s);

#if defined(_CPLUSPLUS_) || defined(__CPLUSPLUS__) || defined(__CPLUSPLUS)
}
#endif

#endif /*NCURI_H*/

/*
Copyright (c) 1998-2018 University Corporation for Atmospheric Research/Unidata
See COPYRIGHT for license information.
*/

/*
Common functionality for reading
and accessing rc files (e.g. .daprc).
*/

#ifndef NCRC_H
#define NCRC_H

/* Need these support includes */
#include "ncuri.h"
#include "nclist.h"
#include "ncbytes.h"

/* getenv() keys */
#define NCRCENVIGNORE "NCRCENV_IGNORE"
#define NCRCENVRC "NCRCENV_RC"


typedef struct NCTriple {
	char* host; /* combined host:port */
        char* key;
        char* value;
} NCTriple;

/* collect all the relevant info around the rc file */
typedef struct NCRCinfo {
	int ignore; /* if 1, then do not use any rc file */
	int loaded; /* 1 => already loaded */
        NClist* triples; /* the rc file triple store fields*/
        char* rcfile; /* specified rcfile; overrides anything else */
} NCRCinfo;

/* Collect global state info in one place */
typedef struct NCRCglobalstate {
    int initialized;
    char* tempdir; /* track a usable temp dir */
    char* home; /* track $HOME */
    char* cwd; /* track getcwd */
    NCRCinfo rcinfo; /* Currently only one rc file per session */
    struct GlobalZarr { /* Zarr specific parameters */
	char dimension_separator;
    } zarr;
} NCRCglobalstate;

/* From drc.c */
EXTERNL void ncrc_initialize(void);
EXTERNL void ncrc_freeglobalstate(void);
/* read and compile the rc file, if any */
EXTERNL int NC_rcload(void);
EXTERNL char* NC_rclookup(const char* key, const char* hostport);
EXTERNL int NC_rcfile_insert(const char* key, const char* value, const char* hostport);

/* Following are primarily for debugging */
/* Obtain the count of number of triples */
EXTERNL size_t NC_rcfile_length(NCRCinfo*);
/* Obtain the ith triple; return NULL if out of range */
EXTERNL NCTriple* NC_rcfile_ith(NCRCinfo*,size_t);

/* For internal use */
EXTERNL NCRCglobalstate* ncrc_getglobalstate(void);
EXTERNL void NC_rcclear(NCRCinfo* info);
EXTERNL void NC_rcclear(NCRCinfo* info);

/* From dutil.c (Might later move to e.g. nc.h */
EXTERNL int NC__testurl(const char* path, char** basenamep);
EXTERNL int NC_isLittleEndian(void);
EXTERNL char* NC_entityescape(const char* s);
EXTERNL int NC_readfile(const char* filename, NCbytes* content);
EXTERNL int NC_writefile(const char* filename, size_t size, void* content);
EXTERNL char* NC_mktmp(const char* base);
EXTERNL int NC_getmodelist(const char* url, NClist** modelistp);
EXTERNL int NC_testmode(const char* path, const char* tag);
#endif /*NCRC_H*/

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
    char* home; /* track $HOME for use in creating $HOME/.oc dir */
    NCRCinfo rcinfo; /* Currently only one rc file per session */
} NCRCglobalstate;

/* From drc.c */
extern NCRCglobalstate* ncrc_getglobalstate(void);
extern void ncrc_freeglobalstate(void);
/* read and compile the rc file, if any */
extern int NC_rcload(void);
extern char* NC_rclookup(const char* key, const char* hostport);
extern void NC_rcclear(NCRCinfo* info);
extern int NC_set_rcfile(const char* rcfile);
extern int NC_rcfile_insert(const char* key, const char* value, const char* hostport);
/* Obtain the count of number of triples */
extern size_t NC_rcfile_length(NCRCinfo*);
/* Obtain the ith triple; return NULL if out of range */
extern NCTriple* NC_rcfile_ith(NCRCinfo*,size_t);

/* From dutil.c (Might later move to e.g. nc.h */
extern int NC__testurl(const char* path, char** basenamep);
extern int NC_isLittleEndian(void);
extern char* NC_backslashEscape(const char* s);
extern char* NC_backslashUnescape(const char* esc);
extern char* NC_entityescape(const char* s);
extern int NC_readfile(const char* filename, NCbytes* content);
extern int NC_writefile(const char* filename, size_t size, void* content);
extern char* NC_mktmp(const char* base);
extern int NC_getmodelist(const char* url, NClist** modelistp);
extern int NC_testmode(const char* path, const char* tag);
#endif /*NCRC_H*/

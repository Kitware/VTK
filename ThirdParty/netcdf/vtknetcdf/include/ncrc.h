/*
Copyright (c) 1998-2017 University Corporation for Atmospheric Research/Unidata
See LICENSE.txt for license information.
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
    NCRCinfo rcinfo; /* Currenly only one rc file per session */
} NCRCglobalstate;

extern NCRCglobalstate ncrc_globalstate; /* singleton instance */

/* From drc.c */
/* read and compile the rc file, if any */
extern int NC_rcload(void);
extern char* NC_rclookup(const char* key, const char* hostport);
extern void NC_rcclear(NCRCinfo* info);
extern int NC_set_rcfile(const char* rcfile);

/* From dutil.c (Might later move to e.g. nc.h */
extern int NC__testurl(const char* path, char** basenamep);
extern int NC_isLittleEndian(void);
extern char* NC_backslashEscape(const char* s);
extern char* NC_backslashUnescape(const char* esc);
extern char* NC_entityescape(const char* s);
extern int NC_readfile(const char* filename, NCbytes* content);
extern char* NC_mktmp(const char* base);

#endif /*NCRC_H*/

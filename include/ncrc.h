/*********************************************************************
  *   Copyright 2016, UCAR/Unidata
  *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
  *********************************************************************/

#ifndef NCRC_H
#define NCRC_H

#include "nclist.h"

typedef struct NCTriple {
	char* tag;
        char* key;
        char* value;
} NCTriple;

typedef struct NCTripleStore {
    NClist* triples; /* list of NCTriple* */
} NCTripleStore;

/* read and compile the rc file, if any */
extern int ncrc_load(const char* filename);
extern char* ncrc_lookup(NCTripleStore*, char* key, char* hostport);
extern void ncrc_reset(NCTripleStore*);

#endif /*NCRC_H*/

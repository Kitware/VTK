/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/
/* $Id$ */
/* $Header$ */

#ifndef NC_URL_H
#define NC_URL_H

#include "nclist.h"

/*! This is an open structure meaning
        it is ok to directly access its fields*/
typedef struct NC_URL {
    char* url;        /* as passed by the caller */
    char* base;       /*!< without constraints*/
    char* protocol;
    char* constraint;
    char* projection; /*!< without leading '?'*/
    char* selection;  /*!< with leading '&'*/
    char* params;
    NClist* parammap;
} NC_URL;

extern int nc_urlparse(const char* s, NC_URL**);
extern void nc_urlfree(NC_URL* nc_url);

/* Replace the constraints */
extern void nc_urlsetconstraints(NC_URL*,const char* constraints);

/* Replace the protocol */
extern void nc_urlsetprotocol(NC_URL*,const char* newprotocol);

/* Construct a complete URL; caller frees returned string */
extern char* nc_urlgeturl(NC_URL*,const char* prefix, const char* suffix, int withconstraints);

extern int nc_urldecodeparams(NC_URL* nc_url);

/*! NULL result => entry not found.
    Empty value should be represented as a zero length list */
extern NClist* nc_urllookup(NC_URL*, const char* clientparam);

/* Convenience: search a list for a given string; NULL if not found */
extern const char* nc_urllookupvalue(NClist* list, const char* value);

#endif /*NC_URL_H*/

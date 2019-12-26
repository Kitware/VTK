/* Copyright 2018-2018 University Corporation for Atmospheric
   Research/Unidata. */

/**
 * Functions for inferring dataset model
 * @author Dennis Heimbigner
 */

#ifndef NCINFERMODEL_H
#define NCINFERMODEL_H

/* Track the information hat will help us
   infer how to access the data defined by
   path + omode + (sometimes) file content.
*/
typedef struct NCmodel {
    int impl; /* NC_FORMATX_XXX value */
    int format; /* NC_FORMAT_XXX value; Used to remember extra info; */
} NCmodel;

/* Keep compiler quiet */
struct NCURI;
struct NC_dispatch;

/* Infer model implementation */
EXTERNL int NC_infermodel(const char* path, int* omodep, int iscreate, int useparallel, void* params, NCmodel* model, char** newpathp);

/**
 * Provide a hidden interface to allow utilities
 * to check if a given path name is really a url.
 * If not, put null in basenamep, else put basename of the url
 * minus any extension into basenamep; caller frees.
 * Return 1 if it looks like a url, 0 otherwise.
 */
EXTERNL int nc__testurl(const char* path, char** basenamep);

#endif /*NCINFERMODEL_H*/

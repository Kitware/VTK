/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Id$
 *   $Header$
 *********************************************************************/

/*
 * In order to use any of the netcdf_XXX.h files, it is necessary
 * to include netcdf.h followed by any netcdf_XXX.h files.
 * Various things (like EXTERNL) are defined in netcdf.h
 * to make them available for use by the netcdf_XXX.h files.
*/

#ifndef NCAUX_H
#define NCAUX_H

#define NCAUX_ALIGN_C 0
#define NCAUX_ALIGN_UNIFORM 1

#if defined(__cplusplus)
extern "C" {
#endif


/**
Reclaim the output tree of data from a call
to e.g. nc_get_vara or the input to e.g. nc_put_vara.
This recursively walks the top-level instances to
reclaim any nested data such as vlen or strings or such.

Assumes it is passed a pointer to count instances of xtype.
Reclaims any nested data.
WARNING: does not reclaim the top-level memory because
we do not know how it was allocated.
Should work for any netcdf format.
*/

EXTERNL int ncaux_reclaim_data(int ncid, int xtype, void* memory, size_t count);


EXTERNL int ncaux_begin_compound(int ncid, const char *name, int alignmode, void** tag);

EXTERNL int ncaux_end_compound(void* tag, nc_type* typeid);

EXTERNL int ncaux_abort_compound(void* tag);

EXTERNL int ncaux_add_field(void* tag,  const char *name, nc_type field_type,
			   int ndims, const int* dimsizes);

/* Takes any type */
EXTERNL size_t ncaux_type_alignment(int xtype, int ncid);

/* Takes type classes only */
EXTERNL size_t ncaux_class_alignment(int ncclass);

#if defined(__cplusplus)
}
#endif

#endif /*NCAUX_H*/


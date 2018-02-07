/*********************************************************************
 *   Copyright 2010, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Id$
 *   $Header$
 *********************************************************************/

#ifndef NCAUX_H
#define NCAUX_H

#define NCAUX_ALIGN_C 0
#define NCAUX_ALIGN_UNIFORM 1

extern int ncaux_begin_compound(int ncid, const char *name, int alignmode,
				void** tag);

extern int ncaux_end_compound(void* tag, nc_type* typeid);

extern int ncaux_abort_compound(void* tag);

extern int ncaux_add_field(void* tag,  const char *name, nc_type field_type,
			   int ndims, const int* dimsizes);

#endif /*NCAUX_H*/


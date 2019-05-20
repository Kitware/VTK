/** \file
Attribute inquiry functions

These functions find out about attributes.

Copyright 2011 University Corporation for Atmospheric
Research/Unidata. See \ref copyright file for more info.  */

#include "ncdispatch.h"

/** \name Learning about Attributes

Functions to learn about the attributes in a file. */
/*! \{ */ /* All these functions are part of this named group... */

/**
\ingroup attributes
Return information about a netCDF attribute.

The function nc_inq_att returns the attribute's type and length.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as 
nc_inq_ncid().

\param varid Variable ID of the attribute's variable, or ::NC_GLOBAL
for a global attribute.

\param name Pointer to the location for the returned attribute \ref
object_name. \ref ignored_if_null.

\param xtypep Pointer to location for returned attribute \ref
data_type. \ref ignored_if_null.

\param lenp Pointer to location for returned number of values
currently stored in the attribute. For attributes of type ::NC_CHAR,
you should not assume that this includes a trailing zero byte; it
doesn't if the attribute was stored without a trailing zero byte, for
example from a FORTRAN program. Before using the value as a C string,
make sure it is null-terminated. \ref ignored_if_null.

\section nc_inq_att_example Example

Here is an example using nc_inq_att() to find out the type and length of
a variable attribute named valid_range for a netCDF variable named rh
and a global attribute named title in an existing netCDF dataset named
foo.nc:

\code
     #include <netcdf.h>
        ...
     int  status;             
     int  ncid;               
     int  rh_id;              
     nc_type vr_type, t_type; 
     size_t  vr_len, t_len;   
     
        ...
     status = nc_open("foo.nc", NC_NOWRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_inq_varid (ncid, "rh", &rh_id);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_inq_att (ncid, rh_id, "valid_range", &vr_type, &vr_len);
     if (status != NC_NOERR) handle_error(status);
     status = nc_inq_att (ncid, NC_GLOBAL, "title", &t_type, &t_len);
     if (status != NC_NOERR) handle_error(status);
\endcode
*/
int
nc_inq_att(int ncid, int varid, const char *name, nc_type *xtypep, 
	   size_t *lenp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_att(ncid, varid, name, xtypep, lenp);
}

/**
\ingroup attributes
Find an attribute ID.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as 
nc_inq_ncid().

\param varid Variable ID of the attribute's variable, or ::NC_GLOBAL for
a global attribute.

\param name Attribute \ref object_name. 

\param idp Pointer to location for returned attribute number that
specifies which attribute this is for this variable (or which global
attribute). If you already know the attribute name, knowing its number
is not very useful, because accessing information about an attribute
requires its name.
*/
int
nc_inq_attid(int ncid, int varid, const char *name, int *idp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_attid(ncid, varid, name, idp);
}

/**
\ingroup attributes
Find the name of an attribute.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as 
nc_inq_ncid().

\param varid Variable ID of the attribute's variable, or ::NC_GLOBAL
for a global attribute.

\param attnum Attribute number. The attributes for each variable are
numbered from 0 (the first attribute) to natts-1, where natts is the
number of attributes for the variable, as returned from a call to
nc_inq_varnatts().

\param name Pointer to the location for the returned attribute \ref
object_name.  
*/
int
nc_inq_attname(int ncid, int varid, int attnum, char *name)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_attname(ncid, varid, attnum, name);
}

/**
\ingroup attributes
Find number of global or group attributes.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as 
nc_inq_ncid().

\param nattsp Pointer where number of global or group attributes will be
written. \ref ignored_if_null.
*/
int
nc_inq_natts(int ncid, int *nattsp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   if(nattsp == NULL) return NC_NOERR;
   return ncp->dispatch->inq(ncid, NULL, NULL, nattsp, NULL);
}

/**
\ingroup attributes
Find the type of an attribute.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as 
nc_inq_ncid().

\param varid Variable ID of the attribute's variable, or ::NC_GLOBAL
for a global or group attribute.

\param name Attribute \ref object_name. 

\param xtypep Pointer to location for returned attribute \ref data_type.
*/
int
nc_inq_atttype(int ncid, int varid, const char *name, nc_type *xtypep)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_att(ncid, varid, name, xtypep, NULL);
}

/**
\ingroup attributes
Find the length of an attribute.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as 
nc_inq_ncid().

\param varid Variable ID of the attribute's variable, or ::NC_GLOBAL
for a global or group attribute.

\param name Attribute \ref object_name. 

\param lenp Pointer to location for returned number of values
currently stored in the attribute. Before using the value as a C
string, make sure it is null-terminated. \ref ignored_if_null.  
*/
int
nc_inq_attlen(int ncid, int varid, const char *name, size_t *lenp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_att(ncid, varid, name, NULL, lenp);
}

/*! \} */  /* End of named group ...*/

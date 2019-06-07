/** \file
Attribute functions

These functions read and write attributes.

Copyright 2018 University Corporation for Atmospheric
Research/Unidata. See \ref copyright file for more info.  */

#include "ncdispatch.h"

/** \name Getting Attributes

Functions to get the values of attributes.
 */
/*! \{ */

/*!
\ingroup attributes
Get an attribute of any type.

The nc_get_att() functions works for any type of attribute, and must
be used to get attributes of user-defined type. We recommend that they
type safe versions of this function be used for atomic data types.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID of the attribute's variable, or ::NC_GLOBAL
for a global attribute.

\param name Attribute name.

\param value Pointer to location for returned attribute value(s). All
elements of the vector of attribute values are returned, so you must
allocate enough space to hold them. Before using the value as a C
string, make sure it is null-terminated. Call nc_inq_attlen() first to
find out the length of the attribute.

\note See documentation for nc_get_att_string() regarding a special
case where memory must be explicitly released.

\returns ::NC_NOERR for success.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Bad varid.
\returns ::NC_EBADNAME Bad name. See \ref object_name.
\returns ::NC_EINVAL Invalid parameters.
\returns ::NC_ENOTATT Can't find attribute.
\returns ::NC_ECHAR Can't convert to or from NC_CHAR.
\returns ::NC_ENOMEM Out of memory.
\returns ::NC_ERANGE Data conversion went out of range.

<h1>Example</h1>

Here is an example using nc_get_att() from nc_test4/tst_vl.c creates a
VLEN attribute, then uses nc_get_att() to read it.

@code
#define FILE_NAME "tst_vl.nc"
#define VLEN_NAME "vlen_name"
#define ATT_NAME "att_name"

      int ncid, typeid;
      nc_vlen_t data[DIM_LEN], data_in[DIM_LEN];
      ...

      if (nc_create(FILE_NAME, NC_NETCDF4, &ncid)) ERR;
      if (nc_def_vlen(ncid, VLEN_NAME, NC_INT, &typeid)) ERR;
      ...
      if (nc_put_att(ncid, NC_GLOBAL, ATT_NAME, typeid, DIM_LEN, data)) ERR;
      if (nc_close(ncid)) ERR;

      ...
      if (nc_open(FILE_NAME, NC_NOWRITE, &ncid)) ERR;
      if (nc_get_att(ncid, NC_GLOBAL, ATT_NAME, data_in)) ERR;
      ...
      if (nc_close(ncid)) ERR;
@endcode

\author Glenn Davis, Ed Hartnett, Dennis Heimbigner
*/
int
nc_get_att(int ncid, int varid, const char *name, void *value)
{
   NC* ncp;
   int stat = NC_NOERR;
   nc_type xtype;

   if ((stat = NC_check_id(ncid, &ncp)))
      return stat;

   /* Need to get the type */
   if ((stat = nc_inq_atttype(ncid, varid, name, &xtype)))
      return stat;

   TRACE(nc_get_att);
   return ncp->dispatch->get_att(ncid, varid, name, value, xtype);
}
/*! \} */

/*!
\ingroup attributes
Get an attribute.

This function gets an attribute from the netCDF file. The nc_get_att()
function works with any type of data, including user defined types.

\note The netCDF library reads all attributes into memory when the
file is opened with nc_open(). Getting an attribute copies the value
from the in-memory store, and does not incur any file I/O penalties.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID of the attribute's variable, or ::NC_GLOBAL
for a global attribute.

\param name Attribute name.

\param value Pointer to location for returned attribute value(s). All
elements of the vector of attribute values are returned, so you must
allocate enough space to hold them. If you don't know how much
space to reserve, call nc_inq_attlen() first to find out the length of
the attribute.

\returns ::NC_NOERR for success.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Bad varid.
\returns ::NC_EBADNAME Bad name. See \ref object_name.
\returns ::NC_EINVAL Invalid parameters.
\returns ::NC_ENOTATT Can't find attribute.
\returns ::NC_ECHAR Can't convert to or from NC_CHAR.
\returns ::NC_ENOMEM Out of memory.
\returns ::NC_ERANGE Data conversion went out of range.

<h1>Example</h1>

Here is an example using nc_get_att_double() to determine the values
of a variable attribute named valid_range for a netCDF variable named
rh and using nc_get_att_text() to read a global attribute named title
in an existing netCDF dataset named foo.nc.

In this example, it is assumed that we don't know how many values will
be returned, but that we do know the types of the attributes. Hence,
to allocate enough space to store them, we must first inquire about
the length of the attributes.

\code
     #include <netcdf.h>
        ...
     int  status;
     int  ncid;
     int  rh_id;
     int  vr_len, t_len;
     double *vr_val;
     char *title;
     extern char *malloc()

        ...
     status = nc_open("foo.nc", NC_NOWRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_inq_varid (ncid, "rh", &rh_id);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_inq_attlen (ncid, rh_id, "valid_range", &vr_len);
     if (status != NC_NOERR) handle_error(status);
     status = nc_inq_attlen (ncid, NC_GLOBAL, "title", &t_len);
     if (status != NC_NOERR) handle_error(status);

     vr_val = (double *) malloc(vr_len * sizeof(double));
     title = (char *) malloc(t_len + 1);

     status = nc_get_att_double(ncid, rh_id, "valid_range", vr_val);
     if (status != NC_NOERR) handle_error(status);
     status = nc_get_att_text(ncid, NC_GLOBAL, "title", title);
     if (status != NC_NOERR) handle_error(status);
     title[t_len] = '\0';
        ...
\endcode
\author Glenn Davis, Ed Hartnett, Dennis Heimbigner
*/
/*! \{ */

int
nc_get_att_text(int ncid, int varid, const char *name, char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_text);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_CHAR);
}

int
nc_get_att_schar(int ncid, int varid, const char *name, signed char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_schar);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_BYTE);
}

int
nc_get_att_uchar(int ncid, int varid, const char *name, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_uchar);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_UBYTE);
}

int
nc_get_att_short(int ncid, int varid, const char *name, short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_short);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_SHORT);
}

int
nc_get_att_int(int ncid, int varid, const char *name, int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_int);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_INT);
}

int
nc_get_att_long(int ncid, int varid, const char *name, long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_long);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, longtype);
}

int
nc_get_att_float(int ncid, int varid, const char *name, float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_float);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_FLOAT);
}

int
nc_get_att_double(int ncid, int varid, const char *name, double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_double);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_DOUBLE);
}

int
nc_get_att_ubyte(int ncid, int varid, const char *name, unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_ubyte);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_UBYTE);
}

int
nc_get_att_ushort(int ncid, int varid, const char *name, unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_ushort);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_USHORT);
}

int
nc_get_att_uint(int ncid, int varid, const char *name, unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_uint);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_UINT);
}

int
nc_get_att_longlong(int ncid, int varid, const char *name, long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_longlong);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_INT64);
}

int
nc_get_att_ulonglong(int ncid, int varid, const char *name, unsigned long long *value)
{
   NC *ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_get_att_ulonglong);
   return ncp->dispatch->get_att(ncid, varid, name, (void *)value, NC_UINT64);
}
/*! \} */

/*!
\ingroup attributes
Get a variable-length string attribute.

This function gets an attribute from netCDF file. The nc_get_att()
function works with any type of data including user defined types, but
this function will retrieve attributes which are of type
variable-length string.

\note Note that unlike most other nc_get_att functions,
nc_get_att_string() allocates a chunk of memory which is returned to
the calling function.  This chunk of memory must be specifically
deallocated with nc_free_string() to avoid any memory leaks.  Also
note that you must still preallocate the memory needed for the array
of pointers passed to nc_get_att_string().

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID of the attribute's variable, or ::NC_GLOBAL
for a global attribute.

\param name Attribute name.

\param value Pointer to location for returned attribute value(s). All
elements of the vector of attribute values are returned, so you must
allocate enough space to hold them. If you don't know how much
space to reserve, call nc_inq_attlen() first to find out the length of
the attribute.

\returns ::NC_NOERR for success.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Bad varid.
\returns ::NC_EBADNAME Bad name. See \ref object_name.
\returns ::NC_EINVAL Invalid parameters.
\returns ::NC_ENOTATT Can't find attribute.
\returns ::NC_ECHAR Can't convert to or from NC_CHAR.
\returns ::NC_ENOMEM Out of memory.
\returns ::NC_ERANGE Data conversion went out of range.

\section nc_get_att_string_example Example

\code{.c}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <netcdf.h>

void check(int stat) {
  if (stat != NC_NOERR) {
    printf("NetCDF error: %s\n", nc_strerror(stat));
    exit(1);
  }
}

int main(int argc, char ** argv) {
  int stat = 0;

  int ncid = 0;
  stat = nc_open("test.nc", NC_NOWRITE, &ncid); check(stat);

  int varid = 0;
  stat = nc_inq_varid(ncid, "variable", &varid); check(stat);

  size_t attlen = 0;
  stat = nc_inq_attlen(ncid, varid, "attribute", &attlen); check(stat);

  char **string_attr = (char**)malloc(attlen * sizeof(char*));
  memset(string_attr, 0, attlen * sizeof(char*));

  stat = nc_get_att_string(ncid, varid, "attribute", string_attr); check(stat);

  for (size_t k = 0; k < attlen; ++k) {
    printf("variable:attribute[%d] = %s\n", k, string_attr[k]);
  }

  stat = nc_free_string(attlen, string_attr); check(stat);

  free(string_attr);

  stat = nc_close(ncid); check(stat);

  return 0;
}
\endcode
\author Glenn Davis, Ed Hartnett, Dennis Heimbigner
*/

int
nc_get_att_string(int ncid, int varid, const char *name, char **value)
{
    NC *ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    TRACE(nc_get_att_string);
    return ncp->dispatch->get_att(ncid,varid,name,(void*)value, NC_STRING);
}
/*! \} */

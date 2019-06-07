/** \file
Functions to write attributes.

These functions read and write attributes.

Copyright 2018 University Corporation for Atmospheric
Research/Unidata. See \ref copyright file for more info.  */

#include "ncdispatch.h"

/** \name Writing Attributes

Functions to write attributes. */
/*! \{ */

/*!
\ingroup attributes
Write a string attribute.

The function nc_put_att_string adds or changes a variable attribute or
global attribute of an open netCDF dataset. The string type is only
available in netCDF-4/HDF5 files, when ::NC_CLASSIC_MODEL has not been
used in nc_create().

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID of the variable to which the attribute will
be assigned or ::NC_GLOBAL for a global or group attribute.

\param name Attribute \ref object_name. \ref attribute_conventions may
apply.

\param len Number of values provided for the attribute.

\param value Pointer to one or more values.

\returns ::NC_NOERR No error.
\returns ::NC_EINVAL More than one value for _FillValue or trying to set global _FillValue.
\returns ::NC_ENOTVAR Couldn't find varid.
\returns ::NC_EBADTYPE Fill value and var must be same type.
\returns ::NC_ENOMEM Out of memory
\returns ::NC_ELATEFILL Fill values must be written while the file
is still in initial define mode.
*/


int
nc_put_att_string(int ncid, int varid, const char *name,
		  size_t len, const char** value)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->put_att(ncid, varid, name, NC_STRING,
				  len, (void*)value, NC_STRING);
}

/*!
\ingroup attributes
Write a text attribute.

Add or change a text attribute. If this attribute is new, or if the
space required to store the attribute is greater than before, the
netCDF dataset must be in define mode for classic formats (or
netCDF-4/HDF5 with NC_CLASSIC_MODEL).

Although it's possible to create attributes of all types, text and
double attributes are adequate for most purposes.

Use the nc_put_att function to create attributes of any type,
including user-defined types. We recommend using the type safe
versions of this function whenever possible.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID of the variable to which the attribute will
be assigned or ::NC_GLOBAL for a global attribute.

\param name Attribute \ref object_name. \ref attribute_conventions may
apply.

\param len Number of values provided for the attribute.

\param value Pointer to one or more values.

\returns ::NC_NOERR No error.
\returns ::NC_EINVAL More than one value for _FillValue or trying to set global _FillValue.
\returns ::NC_ENOTVAR Couldn't find varid.
\returns ::NC_EBADTYPE Fill value and var must be same type.
\returns ::NC_ENOMEM Out of memory
\returns ::NC_ELATEFILL Fill values must be written while the file
is still in initial define mode.

\note With netCDF-4 files, nc_put_att will notice if you are writing a
_Fill_Value_ attribute, and will tell the HDF5 layer to use the
specified fill value for that variable.

\section nc_put_att_text_example Example

Here is an example using nc_put_att_double() to add a variable
attribute named valid_range for a netCDF variable named rh and
nc_put_att_text() to add a global attribute named title to an existing
netCDF dataset named foo.nc:

\code
     #include <netcdf.h>
        ...
     int  status;
     int  ncid;
     int  rh_id;
     static double rh_range[] = {0.0, 100.0};
     static char title[] = "example netCDF dataset";
        ...
     status = nc_open("foo.nc", NC_WRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_redef(ncid);
     if (status != NC_NOERR) handle_error(status);
     status = nc_inq_varid (ncid, "rh", &rh_id);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_put_att_double (ncid, rh_id, "valid_range",
                                 NC_DOUBLE, 2, rh_range);
     if (status != NC_NOERR) handle_error(status);
     status = nc_put_att_text (ncid, NC_GLOBAL, "title",
                               strlen(title), title)
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_enddef(ncid);
     if (status != NC_NOERR) handle_error(status);
\endcode
*/


int nc_put_att_text(int ncid, int varid, const char *name,
		size_t len, const char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, NC_CHAR, len,
				 (void *)value, NC_CHAR);
}

/*! \} */
/*!
\ingroup attributes
Write an attribute.

The function nc_put_att_ type adds or changes a variable attribute or
global attribute of an open netCDF dataset. If this attribute is new,
or if the space required to store the attribute is greater than
before, the netCDF dataset must be in define mode for classic formats
(or netCDF-4/HDF5 with NC_CLASSIC_MODEL).

With netCDF-4 files, nc_put_att will notice if you are writing a
_FillValue attribute, and will tell the HDF5 layer to use the
specified fill value for that variable.  With either classic or
netCDF-4 files, a _FillValue attribute will be checked for validity,
to make sure it has only one value and that its type matches the type
of the associated variable.

Although it's possible to create attributes of all types, text and
double attributes are adequate for most purposes.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID of the variable to which the attribute will
be assigned or ::NC_GLOBAL for a global or group attribute.

\param name Attribute \ref object_name. \ref attribute_conventions may
apply.

\param xtype \ref data_type of the attribute.

\param len Number of values provided for the attribute.

\param value Pointer to one or more values.

\returns ::NC_NOERR No error.
\returns ::NC_EINVAL More than one value for _FillValue or trying to set global _FillValue.
\returns ::NC_ENOTVAR Couldn't find varid.
\returns ::NC_EBADTYPE Fill value and var must be same type.
\returns ::NC_ENOMEM Out of memory
\returns ::NC_ELATEFILL Fill values must be written while the file
is still in initial define mode.

\section nc_put_att_double_example Example

Here is an example using nc_put_att_double() to add a variable
attribute named valid_range for a netCDF variable named rh and
nc_put_att_text() to add a global attribute named title to an existing
netCDF dataset named foo.nc:

\code
     #include <netcdf.h>
        ...
     int  status;
     int  ncid;
     int  rh_id;
     static double rh_range[] = {0.0, 100.0};
     static char title[] = "example netCDF dataset";
        ...
     status = nc_open("foo.nc", NC_WRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_redef(ncid);
     if (status != NC_NOERR) handle_error(status);
     status = nc_inq_varid (ncid, "rh", &rh_id);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_put_att_double (ncid, rh_id, "valid_range",
                                 NC_DOUBLE, 2, rh_range);
     if (status != NC_NOERR) handle_error(status);
     status = nc_put_att_text (ncid, NC_GLOBAL, "title",
                               strlen(title), title)
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_enddef(ncid);
     if (status != NC_NOERR) handle_error(status);
\endcode
*/
/*! \{*/
int
nc_put_att(int ncid, int varid, const char *name, nc_type xtype,
	   size_t len, const void *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 value, xtype);
}

int
nc_put_att_schar(int ncid, int varid, const char *name,
		 nc_type xtype, size_t len, const signed char *value)
{
   NC *ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_BYTE);
}

int
nc_put_att_uchar(int ncid, int varid, const char *name,
		 nc_type xtype, size_t len, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_UBYTE);
}

int
nc_put_att_short(int ncid, int varid, const char *name,
		 nc_type xtype, size_t len, const short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_SHORT);
}

int
nc_put_att_int(int ncid, int varid, const char *name,
	       nc_type xtype, size_t len, const int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_INT);
}

int
nc_put_att_long(int ncid, int varid, const char *name,
		nc_type xtype, size_t len, const long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, longtype);
}

int
nc_put_att_float(int ncid, int varid, const char *name,
		 nc_type xtype, size_t len, const float *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_FLOAT);
}

int
nc_put_att_double(int ncid, int varid, const char *name,
		  nc_type xtype, size_t len, const double *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_DOUBLE);
}

int
nc_put_att_ubyte(int ncid, int varid, const char *name,
		 nc_type xtype, size_t len, const unsigned char *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_UBYTE);
}

int
nc_put_att_ushort(int ncid, int varid, const char *name,
		  nc_type xtype, size_t len, const unsigned short *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_USHORT);
}

int
nc_put_att_uint(int ncid, int varid, const char *name,
		nc_type xtype, size_t len, const unsigned int *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_UINT);
}

int
nc_put_att_longlong(int ncid, int varid, const char *name,
		    nc_type xtype, size_t len,
		    const long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_INT64);
}

int
nc_put_att_ulonglong(int ncid, int varid, const char *name,
		     nc_type xtype, size_t len,
		     const unsigned long long *value)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->put_att(ncid, varid, name, xtype, len,
				 (void *)value, NC_UINT64);
}

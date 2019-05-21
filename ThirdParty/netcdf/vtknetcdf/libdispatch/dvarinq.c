/*! \file
Functions for inquiring about variables.

Copyright 2010 University Corporation for Atmospheric
Research/Unidata. See COPYRIGHT file for more info.
*/

#include "ncdispatch.h"

/** \name Learning about Variables

Functions to learn about the variables in a file. */
/*! \{ */ /* All these functions are part of this named group... */

/**
\ingroup variables
Find the ID of a variable, from the name.

The function nc_inq_varid returns the ID of a netCDF variable, given
its name.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param name Name of the variable.

\param varidp Pointer to location for returned variable ID.  \ref
ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.

\section nc_inq_varid_example4 Example

Here is an example using nc_inq_varid to find out the ID of a variable
named rh in an existing netCDF dataset named foo.nc:

\code
     #include <netcdf.h>
        ...
     int  status, ncid, rh_id;
        ...
     status = nc_open("foo.nc", NC_NOWRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_inq_varid (ncid, "rh", &rh_id);
     if (status != NC_NOERR) handle_error(status);
\endcode
 */
int
nc_inq_varid(int ncid, const char *name, int *varidp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_varid(ncid, name, varidp);
}

/**
\ingroup variables
Learn about a variable.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param name Returned \ref object_name of variable. \ref
ignored_if_null.

\param xtypep Pointer where typeid will be stored. \ref ignored_if_null.

\param ndimsp Pointer where number of dimensions will be
stored. \ref ignored_if_null.

\param dimidsp Pointer where array of dimension IDs will be
stored. \ref ignored_if_null.

\param nattsp Pointer where number of attributes will be
stored. \ref ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.

\section nc_inq_var_example5 Example

Here is an example using nc_inq_var() to find out about a variable named
rh in an existing netCDF dataset named foo.nc:

\code
     #include <netcdf.h>
        ...
     int  status
     int  ncid;
     int  rh_id;
     nc_type rh_type;
     int rh_ndims;
     int  rh_dimids[NC_MAX_VAR_DIMS];
     int rh_natts
        ...
     status = nc_open ("foo.nc", NC_NOWRITE, &ncid);
     if (status != NC_NOERR) handle_error(status);
        ...
     status = nc_inq_varid (ncid, "rh", &rh_id);
     if (status != NC_NOERR) handle_error(status);
     status = nc_inq_var (ncid, rh_id, 0, &rh_type, &rh_ndims, rh_dimids,
                          &rh_natts);
     if (status != NC_NOERR) handle_error(status);
\endcode

 */
int
nc_inq_var(int ncid, int varid, char *name, nc_type *xtypep,
	   int *ndimsp, int *dimidsp, int *nattsp)
{
   NC* ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var);
   return ncp->dispatch->inq_var_all(ncid, varid, name, xtypep, ndimsp,
				     dimidsp, nattsp, NULL, NULL, NULL,
				     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}

/**
\ingroup variables
Learn the name of a variable.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param name Returned variable name. The caller must allocate space for
the returned name. The maximum length is ::NC_MAX_NAME. Ignored if
NULL.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
 */
int
nc_inq_varname(int ncid, int varid, char *name)
{
   return nc_inq_var(ncid, varid, name, NULL, NULL,
		     NULL, NULL);
}

/** Learn the type of a variable.
\ingroup variables

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param typep Pointer where typeid will be stored. \ref ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
 */
int
nc_inq_vartype(int ncid, int varid, nc_type *typep)
{
   return nc_inq_var(ncid, varid, NULL, typep, NULL,
		     NULL, NULL);
}

/** Learn how many dimensions are associated with a variable.
\ingroup variables

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param ndimsp Pointer where number of dimensions will be
stored. \ref ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
 */
int
nc_inq_varndims(int ncid, int varid, int *ndimsp)
{
   return nc_inq_var(ncid, varid, NULL, NULL, ndimsp, NULL, NULL);
}

/** Learn the dimension IDs associated with a variable.
\ingroup variables

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param dimidsp Pointer where array of dimension IDs will be
stored. \ref ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
 */
int
nc_inq_vardimid(int ncid, int varid, int *dimidsp)
{
   return nc_inq_var(ncid, varid, NULL, NULL, NULL,
		     dimidsp, NULL);
}

/** Learn how many attributes are associated with a variable.
\ingroup variables

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param nattsp Pointer where number of attributes will be
stored. \ref ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
 */
int
nc_inq_varnatts(int ncid, int varid, int *nattsp)
{
   if (varid == NC_GLOBAL)
      return nc_inq_natts(ncid,nattsp);
   /*else*/
   return nc_inq_var(ncid, varid, NULL, NULL, NULL, NULL,
		     nattsp);
}

#ifdef USE_NETCDF4
/** \ingroup variables
Learn the storage and deflate settings for a variable.

This is a wrapper for nc_inq_var_all().

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param shufflep A 1 will be written here if the shuffle filter is
turned on for this variable, and a 0 otherwise. \ref ignored_if_null.

\param deflatep If this pointer is non-NULL, the nc_inq_var_deflate
function will write a 1 if the deflate filter is turned on for this
variable, and a 0 otherwise. \ref ignored_if_null.

\param deflate_levelp If the deflate filter is in use for this
variable, the deflate_level will be writen here. \ref ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
*/
int
nc_inq_var_deflate(int ncid, int varid, int *shufflep, int *deflatep,
		   int *deflate_levelp)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var_deflate);
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      shufflep, /*shufflep*/
      deflatep, /*deflatep*/
      deflate_levelp, /*deflatelevelp*/
      NULL, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      NULL, /*nofillp*/
      NULL, /*fillvaluep*/
      NULL, /*endianp*/
      NULL, /*optionsmaskp*/
      NULL /*pixelsp*/
      );
}

/** \ingroup variables
Learn the szip settings of a variable.

This function returns the szip settings for a variable. NetCDF does
not allow variables to be created with szip (due to license problems
with the szip library), but we do enable read-only access of HDF5
files with szip compression.

This is a wrapper for nc_inq_var_all().

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param options_maskp The szip options mask will be copied to this
pointer. \ref ignored_if_null.

\param pixels_per_blockp The szip pixels per block will be copied
here. \ref ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_ENOTVAR Invalid variable ID.
*/
int
nc_inq_var_szip(int ncid, int varid, int *options_maskp, int *pixels_per_blockp)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var_szip);
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      NULL, /*shufflep*/
      NULL, /*deflatep*/
      NULL, /*deflatelevelp*/
      NULL, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      NULL, /*nofillp*/
      NULL, /*fillvaluep*/
      NULL, /*endianp*/
      options_maskp, /*optionsmaskp*/
      pixels_per_blockp /*pixelsp*/
      );
}

/** \ingroup variables
Learn the checksum settings for a variable.

This is a wrapper for nc_inq_var_all().

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param fletcher32p Will be set to ::NC_FLETCHER32 if the fletcher32
checksum filter is turned on for this variable, and ::NC_NOCHECKSUM if
it is not. \ref ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_ENOTVAR Invalid variable ID.
*/
int
nc_inq_var_fletcher32(int ncid, int varid, int *fletcher32p)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var_fletcher32);
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      NULL, /*shufflep*/
      NULL, /*deflatep*/
      NULL, /*deflatelevelp*/
      fletcher32p, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      NULL, /*nofillp*/
      NULL, /*fillvaluep*/
      NULL, /*endianp*/
      NULL, /*optionsmaskp*/
      NULL /*pixelsp*/
      );
}

/** \ingroup variables

This is a wrapper for nc_inq_var_all().

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param storagep Address of returned storage property, returned as
::NC_CONTIGUOUS if this variable uses contiguous storage, or
::NC_CHUNKED if it uses chunked storage. \ref ignored_if_null.

\param chunksizesp The chunksizes will be copied here. \ref
ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_ENOTVAR Invalid variable ID.


\section nc_inq_var_chunking_example Example

\code
        printf("**** testing contiguous storage...");
        {
     #define NDIMS6 1
     #define DIM6_NAME "D5"
     #define VAR_NAME6 "V5"
     #define DIM6_LEN 100

           int dimids[NDIMS6], dimids_in[NDIMS6];
           int varid;
           int ndims, nvars, natts, unlimdimid;
           nc_type xtype_in;
           char name_in[NC_MAX_NAME + 1];
           int data[DIM6_LEN], data_in[DIM6_LEN];
           size_t chunksize_in[NDIMS6];
           int storage_in;
           int i, d;

           for (i = 0; i < DIM6_LEN; i++)
              data[i] = i;


           if (nc_create(FILE_NAME, NC_NETCDF4, &ncid)) ERR;
           if (nc_def_dim(ncid, DIM6_NAME, DIM6_LEN, &dimids[0])) ERR;
           if (dimids[0] != 0) ERR;
           if (nc_def_var(ncid, VAR_NAME6, NC_INT, NDIMS6, dimids, &varid)) ERR;
           if (nc_def_var_chunking(ncid, varid, NC_CONTIGUOUS, NULL)) ERR;
           if (nc_put_var_int(ncid, varid, data)) ERR;


           if (nc_inq_var_chunking(ncid, 0, &storage_in, chunksize_in)) ERR;
           if (storage_in != NC_CONTIGUOUS) ERR;
\endcode

*/
int
nc_inq_var_chunking(int ncid, int varid, int *storagep, size_t *chunksizesp)
{
   NC *ncp;
   int stat = NC_check_id(ncid, &ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var_chunking);
   return ncp->dispatch->inq_var_all(ncid, varid, NULL, NULL, NULL, NULL,
				     NULL, NULL, NULL, NULL, NULL, storagep,
				     chunksizesp, NULL, NULL, NULL, NULL, NULL);
}

/** \ingroup variables
Learn the fill mode of a variable.

The fill mode of a variable is set by nc_def_var_fill().

This is a wrapper for nc_inq_var_all().

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param no_fill Pointer to an integer which will get a 1 if no_fill
mode is set for this variable. \ref ignored_if_null.

\param fill_valuep A pointer which will get the fill value for this
variable. \ref ignored_if_null.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
*/
int
nc_inq_var_fill(int ncid, int varid, int *no_fill, void *fill_valuep)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var_fill);
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      NULL, /*shufflep*/
      NULL, /*deflatep*/
      NULL, /*deflatelevelp*/
      NULL, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      no_fill, /*nofillp*/
      fill_valuep, /*fillvaluep*/
      NULL, /*endianp*/
      NULL, /*optionsmaskp*/
      NULL /*pixelsp*/
      );
}

/** \ingroup variables
Find the endianness of a variable.

This is a wrapper for nc_inq_var_all().

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param endianp Storage which will get ::NC_ENDIAN_LITTLE if this
variable is stored in little-endian format, ::NC_ENDIAN_BIG if it is
stored in big-endian format, and ::NC_ENDIAN_NATIVE if the endianness
is not set, and the variable is not created yet.

\returns ::NC_NOERR No error.
\returns ::NC_ENOTNC4 Not a netCDF-4 file.
\returns ::NC_EBADID Bad ncid.
\returns ::NC_ENOTVAR Invalid variable ID.
*/
int
nc_inq_var_endian(int ncid, int varid, int *endianp)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_var_endian);
   return ncp->dispatch->inq_var_all(
      ncid, varid,
      NULL, /*name*/
      NULL, /*xtypep*/
      NULL, /*ndimsp*/
      NULL, /*dimidsp*/
      NULL, /*nattsp*/
      NULL, /*shufflep*/
      NULL, /*deflatep*/
      NULL, /*deflatelevelp*/
      NULL, /*fletcher32p*/
      NULL, /*contiguousp*/
      NULL, /*chunksizep*/
      NULL, /*nofillp*/
      NULL, /*fillvaluep*/
      endianp, /*endianp*/
      NULL, /*optionsmaskp*/
      NULL /*pixelsp*/
      );
}

/*! Return number and list of unlimited dimensions.

In netCDF-4 files, it's possible to have multiple unlimited
dimensions. This function returns a list of the unlimited dimension
ids visible in a group.

Dimensions are visible in a group if they have been defined in that
group, or any ancestor group.

\param ncid NetCDF group ID, from a previous call to nc_open, nc_create, nc_def_grp, etc.
\param nunlimdimsp A pointer to an int which will get the number of visible unlimited dimensions. Ignored if NULL.
\param unlimdimidsp A pointer to an already allocated array of int which will get the ids of all visible unlimited dimensions. Ignored if NULL. To allocate the correct length for this array, call nc_inq_unlimdims with a NULL for this parameter and use the nunlimdimsp parameter to get the number of visible unlimited dimensions.

This function will return one of the following values.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Bad group id.
\returns ::NC_ENOTNC4 Attempting a netCDF-4 operation on a netCDF-3 file. NetCDF-4 operations can only be performed on files defined with a create mode which includes flag HDF5. (see nc_open).
\returns ::NC_ESTRICTNC3 This file was created with the strict netcdf-3 flag, therefore netcdf-4 operations are not allowed. (see nc_open).
\returns ::NC_EHDFERR An error was reported by the HDF5 layer.

 */
int
nc_inq_unlimdims(int ncid, int *nunlimdimsp, int *unlimdimidsp)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
   TRACE(nc_inq_unlimdims);
    return ncp->dispatch->inq_unlimdims(ncid, nunlimdimsp,
					unlimdimidsp);
}

#endif /* USE_NETCDF4 */

/*!

Used in libdap2 and libdap4.

@param[in] ncid               ncid for file.
@param[in] varid              varid for variable in question.
@param[out] name              Pointer to memory to contain the name of the variable.
@param[out] xtypep            Pointer to memory to contain the type of the variable.
@param[out] ndimsp            Pointer to memory to store the number of associated dimensions for the variable.
@param[out] dimidsp           Pointer to memory to store the dimids associated with the variable.
@param[out] nattsp            Pointer to memory to store the number of attributes associated with the variable.
@param[out] shufflep          Pointer to memory to store shuffle information associated with the variable.
@param[out] deflatep          Pointer to memory to store compression type associated with the variable.
@param[out] deflate_levelp    Pointer to memory to store compression level associated with the variable.
@param[out] fletcher32p       Pointer to memory to store compression information associated with the variable.
@param[out] contiguousp       Pointer to memory to store contiguous-data information associated with the variable.
@param[out] chunksizesp       Pointer to memory to store chunksize information associated with the variable.
@param[out] no_fill           Pointer to memory to store whether or not there is a fill value associated with the variable.
@param[out] fill_valuep       Pointer to memory to store the fill value (if one exists) for the variable.
@param[out] endiannessp       Pointer to memory to store endianness value. One of ::NC_ENDIAN_BIG ::NC_ENDIAN_LITTLE ::NC_ENDIAN_NATIVE
@param[out] options_maskp     Pointer to memory to store mask options information.
@param[out] pixels_per_blockp Pointer to memory to store pixels-per-block information for chunked data.

\note Expose access to nc_inq_var_all().

\internal
\ingroup variables


*/
int
NC_inq_var_all(int ncid, int varid, char *name, nc_type *xtypep,
               int *ndimsp, int *dimidsp, int *nattsp,
               int *shufflep, int *deflatep, int *deflate_levelp,
               int *fletcher32p, int *contiguousp, size_t *chunksizesp,
               int *no_fill, void *fill_valuep, int *endiannessp,
	       int *options_maskp, int *pixels_per_blockp)
{
   NC* ncp;
   int stat = NC_check_id(ncid,&ncp);
   if(stat != NC_NOERR) return stat;
   return ncp->dispatch->inq_var_all(
      ncid, varid, name, xtypep,
      ndimsp, dimidsp, nattsp,
      shufflep, deflatep, deflate_levelp, fletcher32p,
      contiguousp, chunksizesp,
      no_fill, fill_valuep,
      endiannessp,
      options_maskp,
      pixels_per_blockp);
}

/*! \} */  /* End of named group ...*/

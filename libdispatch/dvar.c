/* Copyright 2010-2018 University Corporation for Atmospheric
   Research/Unidata. See COPYRIGHT file for more info. */
/**
 * @file
 * Functions for defining and inquiring about variables. @note The
 * order of functions in this file affects the doxygen documentation.
 */

#include "ncdispatch.h"
#include "netcdf_f.h"

/**
   @defgroup variables Variables

   Variables hold multi-dimensional arrays of data.

   Variables for a netCDF dataset are defined when the dataset is
   created, while the netCDF dataset is in define mode. Other
   variables may be added later by reentering define mode. A netCDF
   variable has a name, a type, and a shape, which are specified when
   it is defined. A variable may also have values, which are
   established later in data mode.

   Ordinarily, the name, type, and shape are fixed when the variable
   is first defined. The name may be changed, but the type and shape
   of a variable cannot be changed. However, a variable defined in
   terms of the unlimited dimension can grow without bound in that
   dimension.

   A netCDF variable in an open netCDF dataset is referred to by a
   small integer called a variable ID.

   Variable IDs reflect the order in which variables were defined
   within a netCDF dataset. Variable IDs are 0, 1, 2,..., in the order
   in which the variables were defined. A function is available for
   getting the variable ID from the variable name and vice-versa.

   @ref attributes may be associated with a variable to specify such
   properties as units.

   Operations supported on variables are:
   - Create a variable, given its name, data type, and shape.
   - Get a variable ID from its name.
   - Get a variable's name, data type, shape, and number of attributes
   from its ID.
   - Put a data value into a variable, given variable ID, indices, and value.
   - Put an array of values into a variable, given variable ID, corner
   indices, edge lengths, and a block of values.
   - Put a subsampled or mapped array-section of values into a variable,
   given variable ID, corner indices, edge lengths, stride vector,
   index mapping vector, and a block of values.
   - Get a data value from a variable, given variable ID and indices.
   - Get an array of values from a variable, given variable ID, corner
   indices, and edge lengths.
   - Get a subsampled or mapped array-section of values from a variable,
   given variable ID, corner indices, edge lengths, stride vector, and
   index mapping vector.
   - Rename a variable.

   @section language_types Data Types

   NetCDF supported six atomic data types through version 3.6.0 (char,
   byte, short, int, float, and double). Starting with version 4.0, many
   new atomic and user defined data types are supported (unsigned int
   types, strings, compound types, variable length arrays, enums,
   opaque).

   The additional data types are only supported in netCDF-4/HDF5
   files. To create netCDF-4/HDF5 files, use the ::NC_NETCDF4 flag in
   nc_create().

   @section classic_types NetCDF-3 Classic and 64-Bit Offset Data Types

   NetCDF-3 classic and 64-bit offset files support 6 atomic data types,
   and none of the user defined datatype introduced in NetCDF-4.

   The following table gives the netCDF-3 external data types and the
   corresponding type constants for defining variables in the C
   interface:

   <table>
   <tr><td>Type</td><td>C define</td><td>Bits</td></tr>
   <tr><td>byte</td><td>::NC_BYTE</td><td>8</td></tr>
   <tr><td>char</td><td>::NC_CHAR</td><td>8</td></tr>
   <tr><td>short</td><td>::NC_SHORT</td><td>16</td></tr>
   <tr><td>int</td><td>::NC_INT</td><td>32</td></tr>
   <tr><td>float</td><td>::NC_FLOAT</td><td>32</td></tr>
   <tr><td>double</td><td>::NC_DOUBLE</td><td>64</td></tr>
   </table>

   The first column gives the netCDF external data type, which is the
   same as the CDL data type. The next column gives the corresponding C
   pre-processor macro for use in netCDF functions (the pre-processor
   macros are defined in the netCDF C header-file netcdf.h). The last
   column gives the number of bits used in the external representation of
   values of the corresponding type.

   @section netcdf_4_atomic NetCDF-4 Atomic Data Types

   NetCDF-4 files support all of the atomic data types from netCDF-3,
   plus additional unsigned integer types, 64-bit integer types, and a
   string type.

   <table>
   <tr><td>Type</td><td>C define</td><td>Bits

   <tr><td>byte</td><td>::NC_BYTE</td><td>8</td></tr>
   <tr><td>unsigned byte </td><td>::NC_UBYTE^</td><td> 8</td></tr>
   <tr><td>char </td><td>::NC_CHAR </td><td>8</td></tr>
   <tr><td>short </td><td>::NC_SHORT </td><td>16</td></tr>
   <tr><td>unsigned short </td><td>::NC_USHORT^ </td><td>16</td></tr>
   <tr><td>int </td><td>::NC_INT </td><td>32</td></tr>
   <tr><td>unsigned int </td><td>::NC_UINT^ </td><td>32</td></tr>
   <tr><td>unsigned long long </td><td>::NC_UINT64^ </td><td>64</td></tr>
   <tr><td>long long </td><td>::NC_INT64^ </td><td>64</td></tr>
   <tr><td>float </td><td>::NC_FLOAT </td><td>32</td></tr>
   <tr><td>double </td><td>::NC_DOUBLE </td><td>64</td></tr>
   <tr><td>char ** </td><td>::NC_STRING^ </td><td>string length + 1</td></tr>
   </table>

   ^This type was introduced in netCDF-4, and is not supported in netCDF
   classic or 64-bit offset format files, or in netCDF-4 files if they
   are created with the ::NC_CLASSIC_MODEL flags.
*/

/** @{ */
/**
   @name Defining Variables

   Use these functions to define variables.
*/
/*! @{ */

/**
   Define a new variable.

   This function adds a new variable to an open netCDF dataset or group.
   It returns (as an argument) a variable ID, given the netCDF ID,
   the variable name, the variable type, the number of dimensions, and a
   list of the dimension IDs.

   @param ncid NetCDF or group ID, from a previous call to nc_open(),
   nc_create(), nc_def_grp(), or associated inquiry functions such as
   nc_inq_ncid().
   @param name Variable @ref object_name.
   @param xtype @ref data_type of the variable.
   @param ndims Number of dimensions for the variable. For example, 2
   specifies a matrix, 1 specifies a vector, and 0 means the variable is
   a scalar with no dimensions. Must not be negative or greater than the
   predefined constant ::NC_MAX_VAR_DIMS. In netCDF-4/HDF5 files, may not
   exceed the HDF5 maximum number of dimensions (32).
   @param dimidsp Vector of ndims dimension IDs corresponding to the
   variable dimensions. For classic model netCDF files, if the ID of the
   unlimited dimension is included, it must be first. This argument is
   ignored if ndims is 0. For expanded model netCDF4/HDF5 files, there
   may be any number of unlimited dimensions, and they may be used in any
   element of the dimids array.
   @param varidp Pointer to location for the returned variable ID.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ncid.
   @return ::NC_ENOTINDEFINE Not in define mode.
   @return ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3 netcdf-4 file.
   @return ::NC_EMAXVARS NC_MAX_VARS exceeded [Not enforced after 4.5.0]
   @return ::NC_EBADTYPE Bad type.
   @return ::NC_EINVAL Invalid input.
   @return ::NC_ENAMEINUSE Name already in use.
   @return ::NC_EPERM Attempt to create object in read-only file.

   @section nc_def_var_example Example

   Here is an example using nc_def_var to create a variable named rh of
   type double with three dimensions, time, lat, and lon in a new netCDF
   dataset named foo.nc:

   @code
   #include <netcdf.h>
   ...
   int  status;
   int  ncid;
   int  lat_dim, lon_dim, time_dim;
   int  rh_id;
   int  rh_dimids[3];
   ...
   status = nc_create("foo.nc", NC_NOCLOBBER, &ncid);
   if (status != NC_NOERR) handle_error(status);
   ...

   status = nc_def_dim(ncid, "lat", 5L, &lat_dim);
   if (status != NC_NOERR) handle_error(status);
   status = nc_def_dim(ncid, "lon", 10L, &lon_dim);
   if (status != NC_NOERR) handle_error(status);
   status = nc_def_dim(ncid, "time", NC_UNLIMITED, &time_dim);
   if (status != NC_NOERR) handle_error(status);
   ...

   rh_dimids[0] = time_dim;
   rh_dimids[1] = lat_dim;
   rh_dimids[2] = lon_dim;
   status = nc_def_var (ncid, "rh", NC_DOUBLE, 3, rh_dimids, &rh_id);
   if (status != NC_NOERR) handle_error(status);
   @endcode

   @author Glenn Davis, Ed Hartnett, Dennis Heimbigner
*/
int
nc_def_var(int ncid, const char *name, nc_type xtype,
           int ndims,  const int *dimidsp, int *varidp)
{
    NC* ncp;
    int stat = NC_NOERR;

    if ((stat = NC_check_id(ncid, &ncp)))
        return stat;
    TRACE(nc_def_var);
    return ncp->dispatch->def_var(ncid, name, xtype, ndims,
                                  dimidsp, varidp);
}

/**
   Set the fill value for a variable.

   @note For netCDF classic, 64-bit offset, and CDF5 formats, it is
   allowed (but not good practice) to set the fill value after data
   have been written to the variable. In this case, unless the
   variable has been completely specified (without gaps in the data),
   any existing filled values will not be recognized as fill values by
   applications reading the data. Best practice is to set the fill
   value after the variable has been defined, but before any data have
   been written to that variable. In NetCDF-4 files, this is enforced
   by the HDF5 library. For netCDF-4 files, an error is returned if
   the user attempts to set the fill value after writing data to the
   variable.

   @param ncid NetCDF ID, from a previous call to nc_open() or
   nc_create().
   @param varid Variable ID.
   @param no_fill Set to ::NC_NOFILL to turn off fill mode for this
   variable. Set to ::NC_FILL (the default) to turn on fill mode for
   the variable.
   @param fill_value the fill value to be used for this variable. Must
   be the same type as the variable. This must point to enough free
   memory to hold one element of the data type of the variable. (For
   example, an ::NC_INT will require 4 bytes for it's fill value,
   which is also an ::NC_INT.)

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ID.
   @return ::NC_ENOTINDEFINE Not in define mode.  This is returned for
   netCDF classic, 64-bit offset, or 64-bit data files, or for
   netCDF-4 files, when they were created with ::NC_CLASSIC_MODEL flag by
   nc_creae().
   @return ::NC_EPERM Attempt to create object in read-only file.
   @return ::NC_ELATEDEF (NetCDF-4 only). Returned when user attempts
   to set fill value after data are written.
   @return ::NC_EGLOBAL Attempt to set fill value on NC_GLOBAL.

   @section nc_def_var_fill_example Example

   In this example from libsrc4/tst_vars.c, a variable is defined, and
   the fill mode turned off. Then nc_inq_fill() is used to check that
   the setting is correct. Then some data are written to the
   variable. Since the data that are written do not cover the full
   extent of the variable, the missing values will just be random. If
   fill value mode was turned on, the missing values would get the
   fill value.

   @code
   #define DIM7_LEN 2
   #define DIM7_NAME "dim_7_from_Indiana"
   #define VAR7_NAME "var_7_from_Idaho"
   #define NDIMS 1
   int dimids[NDIMS];
   size_t index[NDIMS];
   int varid;
   int no_fill;
   unsigned short ushort_data = 42, ushort_data_in, fill_value_in;

   if (nc_create(FILE_NAME, NC_NETCDF4, &ncid)) ERR;
   if (nc_def_dim(ncid, DIM7_NAME, DIM7_LEN, &dimids[0])) ERR;
   if (nc_def_var(ncid, VAR7_NAME, NC_USHORT, NDIMS, dimids,
   &varid)) ERR;
   if (nc_def_var_fill(ncid, varid, 1, NULL)) ERR;

   if (nc_inq_var_fill(ncid, varid, &no_fill, &fill_value_in)) ERR;
   if (!no_fill) ERR;

   index[0] = 1;
   if (nc_put_var1_ushort(ncid, varid, index, &ushort_data)) ERR;

   index[0] = 0;
   if (nc_get_var1_ushort(ncid, varid, index, &ushort_data_in)) ERR;

   if (nc_close(ncid)) ERR;
   @endcode
   @author Glenn Davis, Ed Hartnett, Dennis Heimbigner
*/
int
nc_def_var_fill(int ncid, int varid, int no_fill, const void *fill_value)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;

    /* Using NC_GLOBAL is illegal, as this API has no provision for
     * specifying the type of the fillvalue, it must of necessity be
     * using the type of the variable to interpret the bytes of the
     * fill_value argument. */
    if (varid == NC_GLOBAL) return NC_EGLOBAL;

    return ncp->dispatch->def_var_fill(ncid,varid,no_fill,fill_value);
}

/**
   Set the compression settings for a netCDF-4/HDF5 variable.

   This function must be called after nc_def_var and before nc_enddef
   or any functions which writes data to the file.

   Deflation and shuffline require chunked data. If this function is
   called on a variable with contiguous data, then the data is changed
   to chunked data, with default chunksizes. Use nc_def_var_chunking()
   to tune performance with user-defined chunksizes.

   If this function is called on a scalar variable, it is ignored.

   @param ncid NetCDF or group ID, from a previous call to nc_open(),
   nc_create(), nc_def_grp(), or associated inquiry functions such as
   nc_inq_ncid().
   @param varid Variable ID
   @param shuffle True to turn on the shuffle filter. The shuffle
   filter can assist with the compression of integer data by changing
   the byte order in the data stream. It makes no sense to use the
   shuffle filter without setting a deflate level, or to use shuffle
   on non-integer data.
   @param deflate True to turn on deflation for this variable.
   @param deflate_level A number between 0 (no compression) and 9
   (maximum compression).

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ncid.
   @return ::NC_ENOTVAR Invalid variable ID.
   @return ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
   not netCDF-4/HDF5.
   @return ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3
   netcdf-4 file.
   @return ::NC_ELATEDEF Too late to change settings for this variable.
   @return ::NC_ENOTINDEFINE Not in define mode.
   @return ::NC_EPERM File is read only.
   @return ::NC_EMAXDIMS Classic model file exceeds ::NC_MAX_VAR_DIMS.
   @return ::NC_ESTRICTNC3 Attempting to create netCDF-4 type var in
   classic model file
   @return ::NC_EBADTYPE Bad type.
   @return ::NC_ENOMEM Out of memory.
   @return ::NC_EHDFERR Error returned by HDF5 layer.
   @return ::NC_EINVAL Invalid input. Deflate can't be set unless
   variable storage is NC_CHUNK.

   @section nc_def_var_deflate_example Example

   Here is an example from /examples/C/simple_xy_nc4_wr.c using
   nc_def_var_deflate to create a variable and then turn on the shuffle
   filter and compression.

   @code
   #include <netcdf.h>
   #define NDIMS 2
   #define NX 6
   #define NY 12

   int ncid, x_dimid, y_dimid, varid;
   int dimids[NDIMS];
   int shuffle, deflate, deflate_level;
   int data_out[NX][NY];
   int x, y, retval;

   shuffle = NC_SHUFFLE;
   deflate = 1;
   deflate_level = 1;
   ...
   if ((retval = nc_create(FILE_NAME, NC_NETCDF4, &ncid)))
   ERR(retval);

   if ((retval = nc_def_dim(ncid, "x", NX, &x_dimid)))
   ERR(retval);
   if ((retval = nc_def_dim(ncid, "y", NY, &y_dimid)))
   ERR(retval);

   dimids[0] = x_dimid;
   dimids[1] = y_dimid;

   if ((retval = nc_def_var(ncid, "data", NC_INT, NDIMS,
   dimids, &varid)))
   ERR(retval);

   ...

   if ((retval = nc_def_var_deflate(ncid, varid, shuffle, deflate,
   deflate_level)))
   ERR(retval);
   ...
   @endcode
   @author Ed Hartnett, Dennis Heimbigner
*/
int
nc_def_var_deflate(int ncid, int varid, int shuffle, int deflate, int deflate_level)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_deflate(ncid,varid,shuffle,deflate,deflate_level);
}

/**
   Set checksum for a var.

   This function must be called after nc_def_var and before nc_enddef
   or any functions which writes data to the file.

   Checksums require chunked data. If this function is called on a
   variable with contiguous data, then the data is changed to chunked
   data, with default chunksizes. Use nc_def_var_chunking() to tune
   performance with user-defined chunksizes.

   @param ncid NetCDF or group ID, from a previous call to nc_open(),
   nc_create(), nc_def_grp(), or associated inquiry functions such as
   nc_inq_ncid().
   @param varid Variable ID
   @param fletcher32 True to turn on Fletcher32 checksums for this
   variable.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ncid.
   @return ::NC_ENOTVAR Invalid variable ID.
   @return ::NC_ENOTNC4 Attempting netcdf-4 operation on file that is
   not netCDF-4/HDF5.
   @return ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3
   netcdf-4 file.
   @return ::NC_ELATEDEF Too late to change settings for this variable.
   @return ::NC_EINVAL Invalid input
   @author Ed Hartnett, Dennis Heimbigner
*/
int
nc_def_var_fletcher32(int ncid, int varid, int fletcher32)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_fletcher32(ncid,varid,fletcher32);
}

/**
   Define chunking parameters for a variable

   The function nc_def_var_chunking sets the chunking parameters for a
   variable in a netCDF-4 file. It can set the chunk sizes to get chunked
   storage, or it can set the contiguous flag to get contiguous storage.

   The total size of a chunk must be less than 4 GiB. That is, the
   product of all chunksizes and the size of the data (or the size of
   nc_vlen_t for VLEN types) must be less than 4 GiB.

   This function may only be called after the variable is defined, but
   before nc_enddef is called. Once the chunking parameters are set for a
   variable, they cannot be changed.

   Note that this does not work for scalar variables. Only non-scalar
   variables can have chunking.

   @param[in] ncid NetCDF ID, from a previous call to nc_open or
   nc_create.

   @param[in] varid Variable ID.

   @param[in] storage If ::NC_CONTIGUOUS, then contiguous storage is used
   for this variable. Variables with one or more unlimited dimensions
   cannot use contiguous storage. If contiguous storage is turned on, the
   chunksizes parameter is ignored. If ::NC_CHUNKED, then chunked storage
   is used for this variable. Chunk sizes may be specified with the
   chunksizes parameter or default sizes will be used if that parameter
   is NULL.

   @param[in] chunksizesp A pointer to an array list of chunk sizes. The
   array must have one chunksize for each dimension of the variable. If
   ::NC_CONTIGUOUS storage is set, then the chunksizes parameter is
   ignored.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ID.
   @return ::NC_ENOTNC4 Not a netCDF-4 file.
   @return ::NC_ELATEDEF This variable has already been the subject of a
   nc_enddef call.  In netCDF-4 files nc_enddef will be called
   automatically for any data read or write. Once nc_enddef has been
   called after the nc_def_var call for a variable, it is impossible to
   set the chunking for that variable.
   @return ::NC_ENOTINDEFINE Not in define mode.  This is returned for
   netCDF classic or 64-bit offset files, or for netCDF-4 files, when
   they wwere created with ::NC_CLASSIC_MODEL flag by nc_create().
   @return ::NC_EPERM Attempt to create object in read-only file.
   @return ::NC_EBADCHUNK Returns if the chunk size specified for a
   variable is larger than the length of the dimensions associated with
   variable.

   @section nc_def_var_chunking_example Example

   In this example from libsrc4/tst_vars2.c, chunksizes are set with
   nc_var_def_chunking, and checked with nc_var_inq_chunking.

   @code
   printf("**** testing chunking...");
   {
   #define NDIMS5 1
   #define DIM5_NAME "D5"
   #define VAR_NAME5 "V5"
   #define DIM5_LEN 1000

   int dimids[NDIMS5], dimids_in[NDIMS5];
   int varid;
   int ndims, nvars, natts, unlimdimid;
   nc_type xtype_in;
   char name_in[NC_MAX_NAME + 1];
   int data[DIM5_LEN], data_in[DIM5_LEN];
   size_t chunksize[NDIMS5] = {5};
   size_t chunksize_in[NDIMS5];
   int storage_in;
   int i, d;

   for (i = 0; i < DIM5_LEN; i++)
   data[i] = i;

   if (nc_create(FILE_NAME, NC_NETCDF4, &ncid)) ERR;
   if (nc_def_dim(ncid, DIM5_NAME, DIM5_LEN, &dimids[0])) ERR;
   if (nc_def_var(ncid, VAR_NAME5, NC_INT, NDIMS5, dimids, &varid)) ERR;
   if (nc_def_var_chunking(ncid, varid, NC_CHUNKED, chunksize)) ERR;
   if (nc_put_var_int(ncid, varid, data)) ERR;

   if (nc_inq_var_chunking(ncid, varid, &storage_in, chunksize_in)) ERR;
   for (d = 0; d < NDIMS5; d++)
   if (chunksize[d] != chunksize_in[d]) ERR;
   if (storage_in != NC_CHUNKED) ERR;
   @endcode
*/
int
nc_def_var_chunking(int ncid, int varid, int storage,
                    const size_t *chunksizesp)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_chunking(ncid, varid, storage,
                                           chunksizesp);
}

/**
   Define endianness of a variable.

   With this function the endianness (i.e. order of bits in integers) can
   be changed on a per-variable basis. By default, the endianness is the
   same as the default endianness of the platform. But with
   nc_def_var_endianness the endianness can be explicitly set for a
   variable.

   Warning: this function is only defined if the type of the variable
   is an atomic integer or float type.

   This function may only be called after the variable is defined, but
   before nc_enddef is called.

   @param ncid NetCDF ID, from a previous call to nc_open() or
   nc_create().

   @param varid Variable ID.

   @param endian ::NC_ENDIAN_NATIVE to select the native endianness of
   the platform (the default), ::NC_ENDIAN_LITTLE to use
   little-endian, ::NC_ENDIAN_BIG to use big-endian.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ID.
   @return ::NC_ENOTNC4 Not a netCDF-4 file.
   @return ::NC_ELATEDEF This variable has already been the subject of a
   nc_enddef call. In netCDF-4 files nc_enddef will be called
   automatically for any data read or write. Once nc_enddef has been
   called after the nc_def_var call for a variable, it is impossible to
   set the chunking for that variable.
   @return ::NC_ENOTINDEFINE Not in define mode. This is returned for
   netCDF classic or 64-bit offset files, or for netCDF-4 files, when
   they wwere created with ::NC_CLASSIC_MODEL flag by nc_create().
   @return ::NC_EPERM Attempt to create object in read-only file.

   @section nc_def_var_endian_example Example

   In this example from libsrc4/tst_vars2.c, a variable is created, and
   the endianness set to ::NC_ENDIAN_BIG.

   @code
   #define NDIMS4 1
   #define DIM4_NAME "Joe"
   #define VAR_NAME4 "Ed"
   #define DIM4_LEN 10
   {
   int dimids[NDIMS4], dimids_in[NDIMS4];
   int varid;
   int ndims, nvars, natts, unlimdimid;
   nc_type xtype_in;
   char name_in[NC_MAX_NAME + 1];
   int data[DIM4_LEN], data_in[DIM4_LEN];
   int endian_in;
   int i;

   for (i = 0; i < DIM4_LEN; i++)
   data[i] = i;

   if (nc_create(FILE_NAME, NC_NETCDF4, &ncid)) ERR;
   if (nc_def_dim(ncid, DIM4_NAME, DIM4_LEN, &dimids[0])) ERR;
   if (dimids[0] != 0) ERR;
   if (nc_def_var(ncid, VAR_NAME4, NC_INT, NDIMS4, dimids, &varid)) ERR;
   if (nc_def_var_endian(ncid, varid, NC_ENDIAN_BIG)) ERR;
   @endcode
   @author Ed Hartnett
*/
int
nc_def_var_endian(int ncid, int varid, int endian)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_endian(ncid,varid,endian);
}

/**
   Define a new variable filter.

   @param ncid File and group ID.
   @param varid Variable ID.
   @param id
   @param nparams Number of filter parameters.
   @param parms Filter parameters.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ID.
   @author Dennis Heimbigner
*/
int
nc_def_var_filter(int ncid, int varid, unsigned int id,
                  size_t nparams, const unsigned int* parms)
{
    NC* ncp;
    int stat = NC_check_id(ncid,&ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->def_var_filter(ncid,varid,id,nparams,parms);
}

/** @} */

/**
   @name Rename a Variable

   Rename a variable.
*/
/** @{ */

/**
   Rename a variable.

   This function changes the name of a netCDF variable in an open netCDF
   file or group. You cannot rename a variable to have the name of any existing
   variable.

   For classic format, 64-bit offset format, and netCDF-4/HDF5 with
   classic mode, if the new name is longer than the old name, the netCDF
   dataset must be in define mode.

   For netCDF-4/HDF5 files, renaming the variable changes the order of
   the variables in the file. The renamed variable becomes the last
   variable in the file.

   @param ncid NetCDF or group ID, from a previous call to nc_open(),
   nc_create(), nc_def_grp(), or associated inquiry functions such as
   nc_inq_ncid().

   @param varid Variable ID

   @param name New name of the variable.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ncid.
   @return ::NC_ENOTVAR Invalid variable ID.
   @return ::NC_EBADNAME Bad name.
   @return ::NC_EMAXNAME Name is too long.
   @return ::NC_ENAMEINUSE Name in use.
   @return ::NC_ENOMEM Out of memory.

   @section nc_rename_var_example Example

   Here is an example using nc_rename_var to rename the variable rh to
   rel_hum in an existing netCDF dataset named foo.nc:

   @code
   #include <netcdf.h>
   ...
   int  status;
   int  ncid;
   int  rh_id;
   ...
   status = nc_open("foo.nc", NC_WRITE, &ncid);
   if (status != NC_NOERR) handle_error(status);
   ...
   status = nc_redef(ncid);
   if (status != NC_NOERR) handle_error(status);
   status = nc_inq_varid (ncid, "rh", &rh_id);
   if (status != NC_NOERR) handle_error(status);
   status = nc_rename_var (ncid, rh_id, "rel_hum");
   if (status != NC_NOERR) handle_error(status);
   status = nc_enddef(ncid);
   if (status != NC_NOERR) handle_error(status);
   @endcode
   @author Glenn Davis, Ed Hartnett, Dennis Heimbigner
*/
int
nc_rename_var(int ncid, int varid, const char *name)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    TRACE(nc_rename_var);
    return ncp->dispatch->rename_var(ncid, varid, name);
}
/** @} */

/**
   @internal Does a variable have a record dimension?

   @param ncid File ID.
   @param varid Variable ID.
   @param nrecs Pointer that gets number of records.

   @return 0 if not a record var, 1 if it is.
*/
int
NC_is_recvar(int ncid, int varid, size_t* nrecs)
{
    int status = NC_NOERR;
    int unlimid;
    int ndims;
    int dimset[NC_MAX_VAR_DIMS];

    status = nc_inq_unlimdim(ncid,&unlimid);
    if(status != NC_NOERR) return 0; /* no unlimited defined */
    status = nc_inq_varndims(ncid,varid,&ndims);
    if(status != NC_NOERR) return 0; /* no unlimited defined */
    if(ndims == 0) return 0; /* scalar */
    status = nc_inq_vardimid(ncid,varid,dimset);
    if(status != NC_NOERR) return 0; /* no unlimited defined */
    status = nc_inq_dim(ncid,dimset[0],NULL,nrecs);
    if(status != NC_NOERR) return 0;
    return (dimset[0] == unlimid ? 1: 0);
}

/**
   @internal Get the number of record dimensions for a variable and an
   array that identifies which of a variable's dimensions are record
   dimensions. Intended to be used instead of NC_is_recvar(), which
   doesn't work for netCDF-4 variables which have multiple unlimited
   dimensions or an unlimited dimension that is not the first of a
   variable's dimensions.

   @param ncid File ID.
   @param varid Variable ID.
   @param nrecdimsp Pointer that gets number of record dims.
   @param is_recdim Pointer that gets 1 if there is one or more record
   dimensions, 0 if not.

   @return 0 if not a record var, 1 if it is.

   Example use:
   @code
   int nrecdims;
   int is_recdim[NC_MAX_VAR_DIMS];
   ...
   status = NC_inq_recvar(ncid,varid,&nrecdims,is_recdim);
   isrecvar = (nrecdims > 0);
   @endcode
*/
int
NC_inq_recvar(int ncid, int varid, int* nrecdimsp, int *is_recdim)
{
    int status = NC_NOERR;
    int unlimid;
    int nvardims;
    int dimset[NC_MAX_VAR_DIMS];
    int dim;
    int nrecdims = 0;

    status = nc_inq_varndims(ncid,varid,&nvardims);
    if(status != NC_NOERR) return status;
    if(nvardims == 0) return NC_NOERR; /* scalars have no dims */
    for(dim = 0; dim < nvardims; dim++)
        is_recdim[dim] = 0;
    status = nc_inq_unlimdim(ncid, &unlimid);
    if(status != NC_NOERR) return status;
    if(unlimid == -1) return status; /* no unlimited dims for any variables */
#ifdef USE_NETCDF4
    {
        int nunlimdims;
        int *unlimids;
        int recdim;
        status = nc_inq_unlimdims(ncid, &nunlimdims, NULL); /* for group or file, not variable */
        if(status != NC_NOERR) return status;
        if(nunlimdims == 0) return status;

        if (!(unlimids = malloc(nunlimdims * sizeof(int))))
            return NC_ENOMEM;
        status = nc_inq_unlimdims(ncid, &nunlimdims, unlimids); /* for group or file, not variable */
        if(status != NC_NOERR) {
            free(unlimids);
            return status;
        }
        status = nc_inq_vardimid(ncid, varid, dimset);
        if(status != NC_NOERR) {
            free(unlimids);
            return status;
        }
        for (dim = 0; dim < nvardims; dim++) { /* netCDF-4 rec dims need not be first dim for a rec var */
            for(recdim = 0; recdim < nunlimdims; recdim++) {
                if(dimset[dim] == unlimids[recdim]) {
                    is_recdim[dim] = 1;
                    nrecdims++;
                }
            }
        }
        free(unlimids);
    }
#else
    status = nc_inq_vardimid(ncid, varid, dimset);
    if(status != NC_NOERR) return status;
    if(dimset[0] == unlimid) {
        is_recdim[0] = 1;
        nrecdims++;
    }
#endif /* USE_NETCDF4 */
    if(nrecdimsp) *nrecdimsp = nrecdims;
    return status;
}

/* Ok to use NC pointers because
   all IOSP's will use that structure,
   but not ok to use e.g. NC_Var pointers
   because they may be different structure
   entirely.
*/

/**
   @internal
   Find the length of a type. This is how much space is required by
   the in memory to hold one element of this type.

   @param type A netCDF atomic type.

   @return Length of the type in bytes, or -1 if type not found.
   @author Ed Hartnett
*/
int
nctypelen(nc_type type)
{
    switch(type){
    case NC_CHAR :
        return ((int)sizeof(char));
    case NC_BYTE :
        return ((int)sizeof(signed char));
    case NC_SHORT :
        return ((int)sizeof(short));
    case NC_INT :
        return ((int)sizeof(int));
    case NC_FLOAT :
        return ((int)sizeof(float));
    case NC_DOUBLE :
        return ((int)sizeof(double));

        /* These can occur in netcdf-3 code */
    case NC_UBYTE :
        return ((int)sizeof(unsigned char));
    case NC_USHORT :
        return ((int)(sizeof(unsigned short)));
    case NC_UINT :
        return ((int)sizeof(unsigned int));
    case NC_INT64 :
        return ((int)sizeof(signed long long));
    case NC_UINT64 :
        return ((int)sizeof(unsigned long long));
#ifdef USE_NETCDF4
    case NC_STRING :
        return ((int)sizeof(char*));
#endif /*USE_NETCDF4*/

    default:
        return -1;
    }
}

/**
    @internal
    Find the length of a type. Redundant over nctypelen() above.

    @param xtype an nc_type.

    @author Dennis Heimbigner
*/
size_t
NC_atomictypelen(nc_type xtype)
{
    size_t sz = 0;
    switch(xtype) {
    case NC_NAT: sz = 0; break;
    case NC_BYTE: sz = sizeof(signed char); break;
    case NC_CHAR: sz = sizeof(char); break;
    case NC_SHORT: sz = sizeof(short); break;
    case NC_INT: sz = sizeof(int); break;
    case NC_FLOAT: sz = sizeof(float); break;
    case NC_DOUBLE: sz = sizeof(double); break;
    case NC_INT64: sz = sizeof(signed long long); break;
    case NC_UBYTE: sz = sizeof(unsigned char); break;
    case NC_USHORT: sz = sizeof(unsigned short); break;
    case NC_UINT: sz = sizeof(unsigned int); break;
    case NC_UINT64: sz = sizeof(unsigned long long); break;
#ifdef USE_NETCDF4
    case NC_STRING: sz = sizeof(char*); break;
#endif
    default: break;
    }
    return sz;
}

/**
    @internal
    Get the type name.

    @param xtype an nc_type.

    @author Dennis Heimbigner
*/
char *
NC_atomictypename(nc_type xtype)
{
    char* nm = NULL;
    switch(xtype) {
    case NC_NAT: nm = "undefined"; break;
    case NC_BYTE: nm = "byte"; break;
    case NC_CHAR: nm = "char"; break;
    case NC_SHORT: nm = "short"; break;
    case NC_INT: nm = "int"; break;
    case NC_FLOAT: nm = "float"; break;
    case NC_DOUBLE: nm = "double"; break;
    case NC_INT64: nm = "int64"; break;
    case NC_UBYTE: nm = "ubyte"; break;
    case NC_USHORT: nm = "ushort"; break;
    case NC_UINT: nm = "uint"; break;
    case NC_UINT64: nm = "uint64"; break;
#ifdef USE_NETCDF4
    case NC_STRING: nm = "string"; break;
#endif
    default: break;
    }
    return nm;
}

/**
   @internal
   Get the shape of a variable.

   @param ncid NetCDF ID, from a previous call to nc_open() or
   nc_create().
   @param varid Variable ID.
   @param ndims Number of dimensions for this var.
   @param shape Pointer to pre-allocated array that gets the size of
   each dimension.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ncid.
   @return ::NC_ENOTVAR Bad varid.

   @author Dennis Heimbigner
*/
int
NC_getshape(int ncid, int varid, int ndims, size_t* shape)
{
    int dimids[NC_MAX_VAR_DIMS];
    int i;
    int status = NC_NOERR;

    if ((status = nc_inq_vardimid(ncid, varid, dimids)))
        return status;
    for(i = 0; i < ndims; i++)
        if ((status = nc_inq_dimlen(ncid, dimids[i], &shape[i])))
            break;

    return status;
}

/**
   @internal Check the start, count, and stride parameters for gets
   and puts, and handle NULLs.

   @param ncid The file ID.
   @param varid The variable ID.
   @param start Pointer to start array. If NULL ::NC_EINVALCOORDS will
   be returned for non-scalar variable.
   @param count Pointer to pointer to count array. If *count is NULL,
   an array of the correct size will be allocated, and filled with
   counts that represent the full extent of the variable. In this
   case, the memory must be freed by the caller.
   @param stride Pointer to pointer to stride array. If NULL, stide is
   ignored. If *stride is NULL an array of the correct size will be
   allocated, and filled with ones. In this case, the memory must be
   freed by the caller.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ncid.
   @return ::NC_ENOTVAR Variable not found.
   @return ::NC_ENOMEM Out of memory.
   @return ::NC_EINVALCOORDS Missing start array.
   @author Ed Hartnett
*/
int
NC_check_nulls(int ncid, int varid, const size_t *start, size_t **count,
               ptrdiff_t **stride)
{
    int varndims;
    int stat;

    if ((stat = nc_inq_varndims(ncid, varid, &varndims)))
        return stat;

    /* For non-scalar vars, start is required. */
    if (!start && varndims)
        return NC_EINVALCOORDS;

    /* If count is NULL, assume full extent of var. */
    if (!*count)
    {
        if (!(*count = malloc(varndims * sizeof(size_t))))
            return NC_ENOMEM;
        if ((stat = NC_getshape(ncid, varid, varndims, *count)))
        {
            free(*count);
            *count = NULL;
            return stat;
        }
    }

    /* If stride is NULL, do nothing, if *stride is NULL use all
     * 1s. */
    if (stride && !*stride)
    {
        int i;

        if (!(*stride = malloc(varndims * sizeof(ptrdiff_t))))
            return NC_ENOMEM;
        for (i = 0; i < varndims; i++)
            (*stride)[i] = 1;
    }

    return NC_NOERR;
}

/**
   @name Free String Resources

   Use this functions to free resources associated with ::NC_STRING
   data.
*/
/*! @{ */
/**
   Free string space allocated by the library.

   When you read string type the library will allocate the storage
   space for the data. This storage space must be freed, so pass the
   pointer back to this function, when you're done with the data, and
   it will free the string memory.

   @param len The number of character arrays in the array.
   @param data The pointer to the data array.

   @return ::NC_NOERR No error.
   @author Ed Hartnett
*/
int
nc_free_string(size_t len, char **data)
{
    int i;
    for (i = 0; i < len; i++)
        free(data[i]);
    return NC_NOERR;
}
/** @} */

/**
   @name Variables Chunk Caches

   Use these functions to change the variable chunk cache settings.
*/
/*! @{ */
/**
   Change the cache settings for a chunked variable. This function allows
   users to control the amount of memory used in the per-variable chunk
   cache at the HDF5 level. Changing the chunk cache only has effect
   until the file is closed. Once re-opened, the variable chunk cache
   returns to its default value.

   @param ncid NetCDF or group ID, from a previous call to nc_open(),
   nc_create(), nc_def_grp(), or associated inquiry functions such as
   nc_inq_ncid().
   @param varid Variable ID
   @param size The total size of the raw data chunk cache, in bytes.
   @param nelems The number of chunk slots in the raw data chunk cache.
   @param preemption The preemption, a value between 0 and 1 inclusive
   that indicates how much chunks that have been fully read are favored
   for preemption. A value of zero means fully read chunks are treated no
   differently than other chunks (the preemption is strictly LRU) while a
   value of one means fully read chunks are always preempted before other
   chunks.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ncid.
   @return ::NC_ENOTVAR Invalid variable ID.
   @return ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3
   netcdf-4 file.
   @return ::NC_EINVAL Invalid input

   @section nc_def_var_chunk_cache_example Example

   In this example from nc_test4/tst_coords.c, a variable is defined, and
   the chunk cache settings are changed for that variable.

   @code
   printf("**** testing setting cache values for coordinate variables...");
   {
   #define RANK_1 1
   #define DIM0_NAME "d0"
   #define CACHE_SIZE 1000000
   #define CACHE_NELEMS 1009
   #define CACHE_PREEMPTION .90

   int ncid, dimid, varid;
   char name_in[NC_MAX_NAME + 1];

   if (nc_create(FILE_NAME, NC_CLASSIC_MODEL|NC_NETCDF4, &ncid)) ERR;
   if (nc_def_dim(ncid, DIM0_NAME, NC_UNLIMITED, &dimid)) ERR;
   if (nc_def_var(ncid, DIM0_NAME, NC_DOUBLE, 1, &dimid, &varid)) ERR;
   if (nc_set_var_chunk_cache(ncid, varid, CACHE_SIZE, CACHE_NELEMS, CACHE_PREEMPTION)) ERR;
   if (nc_close(ncid)) ERR;

   ...
   }
   SUMMARIZE_ERR;
   @endcode
   @author Ed Hartnett
*/
int
nc_set_var_chunk_cache(int ncid, int varid, size_t size, size_t nelems,
                       float preemption)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->set_var_chunk_cache(ncid, varid, size,
                                              nelems, preemption);
}

/**
   Get the per-variable chunk cache settings from the HDF5 layer.

   @param ncid NetCDF or group ID, from a previous call to nc_open(),
   nc_create(), nc_def_grp(), or associated inquiry functions such as
   nc_inq_ncid().
   @param varid Variable ID
   @param sizep The total size of the raw data chunk cache, in bytes,
   will be put here. @ref ignored_if_null.
   @param nelemsp The number of chunk slots in the raw data chunk
   cache hash table will be put here. @ref ignored_if_null.
   @param preemptionp The preemption will be put here. The preemtion
   value is between 0 and 1 inclusive and indicates how much chunks
   that have been fully read are favored for preemption. A value of
   zero means fully read chunks are treated no differently than other
   chunks (the preemption is strictly LRU) while a value of one means
   fully read chunks are always preempted before other chunks. @ref
   ignored_if_null.

   @return ::NC_NOERR No error.
   @return ::NC_EBADID Bad ncid.
   @return ::NC_ENOTVAR Invalid variable ID.
   @return ::NC_ESTRICTNC3 Attempting netcdf-4 operation on strict nc3
   netcdf-4 file.
   @return ::NC_EINVAL Invalid input
   @author Ed Hartnett
*/
int
nc_get_var_chunk_cache(int ncid, int varid, size_t *sizep, size_t *nelemsp,
                       float *preemptionp)
{
    NC* ncp;
    int stat = NC_check_id(ncid, &ncp);
    if(stat != NC_NOERR) return stat;
    return ncp->dispatch->get_var_chunk_cache(ncid, varid, sizep,
                                              nelemsp, preemptionp);
}
/** @} */
/** @} */

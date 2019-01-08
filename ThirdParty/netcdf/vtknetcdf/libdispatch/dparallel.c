/** \file

This file has the parallel I/O functions which correspond to the
serial I/O functions.

Copyright 2010 University Corporation for Atmospheric
Research/Unidata. See COPYRIGHT file for more info.
*/
#include "config.h"
#include "ncdispatch.h"

/**
Create a netCDF file for parallel I/O.

This function creates a new netCDF file for parallel I/O access.

Parallel I/O access is only available in library build which support
parallel I/O. To support parallel I/O, netCDF must be built with
netCDF-4 enabled, and with a HDF5 library that supports parallel I/O,
or with support for the parallel-netcdf library via the
enable-pnetcdf option.

See nc_create() for a fuller discussion of file creation.

\note When opening a netCDF-4 file HDF5 error reporting is turned off,
if it is on. This doesn't stop the HDF5 error stack from recording the
errors, it simply stops their display to the user through stderr.

\param path The file name of the new netCDF dataset.

\param cmode The creation mode flag. The following flags are available:
  NC_NOCLOBBER (do not overwrite existing file),
  NC_NETCDF4 (create netCDF-4/HDF5 file),
  NC_CLASSIC_MODEL (enforce netCDF classic mode on netCDF-4/HDF5 files),
  NC_PNETCDF.

\param comm the MPI communicator to be used.

\param info MPI info or MPI_INFO_NULL.

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

\returns ::NC_NOERR No error.
\returns ::NC_ENOPAR Library was not built with parallel I/O features.
\returns ::NC_EINVAL Invalid input parameters.
\returns ::NC_ENOMEM System out of memory.
\returns ::NC_ENOTNC Binary format could not be determined.
\returns ::NC_EHDFERR HDF5 error (netCDF-4 files only).
\returns ::NC_EFILEMETA Error writing netCDF-4 file-level metadata in
HDF5 file. (netCDF-4 files only).

<h1>Example</h1>

In this example from nc_test4/tst_parallel.c, a file is create for
parallel I/O.

\code
    int mpi_size, mpi_rank;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Info info = MPI_INFO_NULL;
    int ncid, v1id, dimids[NDIMS];
    char file_name[NC_MAX_NAME + 1];

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    sprintf(file_name, "%s/%s", TEMP_LARGE, FILE);
    if ((res = nc_create_par(file_name, NC_NETCDF4|NC_MPIIO, comm,
			     info, &ncid))) ERR;

    if (nc_def_dim(ncid, "d1", DIMSIZE, dimids)) ERR;
    if (nc_def_dim(ncid, "d2", DIMSIZE, &dimids[1])) ERR;
    if (nc_def_dim(ncid, "d3", NUM_SLABS, &dimids[2])) ERR;

    if ((res = nc_def_var(ncid, "v1", NC_INT, NDIMS, dimids, &v1id))) ERR;

    if ((res = nc_enddef(ncid))) ERR;

    ...

    if ((res = nc_close(ncid)))	ERR;
\endcode
\author Ed Hartnett, Dennis Heimbigner
\ingroup datasets
*/
int nc_create_par(const char *path, int cmode, MPI_Comm comm,
	      MPI_Info info, int *ncidp)
{
#ifndef USE_PARALLEL
   return NC_ENOPAR;
#else
   NC_MPI_INFO data;

   /* One of these two parallel IO modes must be chosen by the user,
    * or else pnetcdf must be in use. */
   if (!(cmode & NC_MPIIO || cmode & NC_MPIPOSIX) &&
       !(cmode & NC_PNETCDF))
      return NC_EINVAL;

   data.comm = comm;
   data.info = info;
   return NC_create(path, cmode, 0, 0, NULL, 1, &data, ncidp);
#endif /* USE_PARALLEL */
}

/**
Open an existing netCDF file for parallel I/O.

This function opens an existing netCDF dataset for parallel I/O
access. It determines the underlying file format automatically. Use
the same call to open a netCDF classic, 64-bit offset, or netCDF-4
file.

Parallel I/O access is only available in library build which support
parallel I/O. To support parallel I/O, netCDF must be built with
netCDF-4 enabled, and with a HDF5 library that supports parallel I/O,
or with support for the parallel-netcdf library via the
enable-pnetcdf option.

It is not necessary to pass any information about the format of the
file being opened. The file type will be detected automatically by the
netCDF library.

As of version 4.3.1.2, multiple calls to nc_open_par() with the same
path will return the same ncid value.

\note When opening a netCDF-4 file HDF5 error reporting is turned off,
if it is on. This doesn't stop the HDF5 error stack from recording the
errors, it simply stops their display to the user through stderr.

\param path File name for netCDF dataset to be opened.

\param mode The mode flag may include NC_WRITE (for read/write
access), NC_MPIIO or NC_MPIPOSIX (not both) for parallel netCDF-4 I/O,
or NC_PNETCDF for parallel-netcdf parallel I/O access for a netCDF
classic or CDF5 file.

\param comm the MPI communicator to be used.

\param info MPI info or MPI_INFO_NULL.

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

nc_open_par() returns the value NC_NOERR if no errors
occurred. Otherwise, the returned status indicates an error. Possible
causes of errors include:

\returns ::NC_NOERR No error.
\returns ::NC_EINVAL Invalid parameters.
\returns ::NC_ENOTNC Not a netCDF file.
\returns ::NC_ENOMEM Out of memory.
\returns ::NC_EHDFERR HDF5 error. (NetCDF-4 files only.)
\returns ::NC_EDIMMETA Error in netCDF-4 dimension metadata. (NetCDF-4
files only.)

<h1>Examples</h1>

Here is an example using nc_open_par() from examples/C/parallel_vara.c.

\code
#include <mpi.h>
#include <netcdf.h>
#include <netcdf_par.h>
   ...
    int ncid, omode;
    char filename[128];
   ...
    omode = NC_PNETCDF | NC_NOWRITE;
    err = nc_open_par(filename, omode, MPI_COMM_WORLD, MPI_INFO_NULL, &ncid); FATAL_ERR
\endcode
\author Ed Hartnett, Dennis Heimbigner
\ingroup datasets
*/
int
nc_open_par(const char *path, int mode, MPI_Comm comm,
	    MPI_Info info, int *ncidp)
{
#ifndef USE_PARALLEL
   return NC_ENOPAR;
#else
   NC_MPI_INFO mpi_data;

   /* One of these two parallel IO modes must be chosen by the user,
    * or else pnetcdf must be in use. */
   if ((mode & NC_MPIIO) || (mode & NC_MPIPOSIX)) {
	/* ok */
   } else if(mode & NC_PNETCDF) {
	/* ok */
   } else
      return NC_EINVAL;

   mpi_data.comm = comm;
   mpi_data.info = info;

   return NC_open(path, mode, 0, NULL, 1, &mpi_data, ncidp);
#endif /* USE_PARALLEL */
}

/**
This is the same as nc_open_par(), but accepts the MPI comm/info as
integers.

\param path File name for netCDF dataset to be opened.

\param mode The mode flag may include NC_WRITE (for read/write
access), NC_MPIIO or NC_MPIPOSIX (not both) for parallel netCDF-4 I/O,
or NC_PNETCDF for parallel-netcdf parallel I/O access for a netCDF
classic or CDF5 file.

\param comm the MPI communicator to be used.

\param info MPI info or MPI_INFO_NULL.

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

nc_open_par() returns the value NC_NOERR if no errors
occurred. Otherwise, the returned status indicates an error. Possible
causes of errors include:

\returns ::NC_NOERR No error.
\returns ::NC_EINVAL Invalid parameters.
\returns ::NC_ENOTNC Not a netCDF file.
\returns ::NC_ENOMEM Out of memory.
\returns ::NC_EHDFERR HDF5 error. (NetCDF-4 files only.)
\returns ::NC_EDIMMETA Error in netCDF-4 dimension metadata. (NetCDF-4
files only.)

\author Ed Hartnett
\ingroup datasets
\internal
*/
int
nc_open_par_fortran(const char *path, int mode, int comm,
		    int info, int *ncidp)
{
#ifndef USE_PARALLEL
   return NC_ENOPAR;
#else
   MPI_Comm comm_c;
   MPI_Info info_c;

   /* Convert fortran comm and info to C comm and info, if there is a
    * function to do so. Otherwise just pass them. */
#ifdef HAVE_MPI_COMM_F2C
   comm_c = MPI_Comm_f2c(comm);
   info_c = MPI_Info_f2c(info);
#else
   comm_c = (MPI_Comm)comm;
   info_c = (MPI_Info)info;
#endif

   return nc_open_par(path, mode, comm_c, info_c, ncidp);
#endif
}

/**\ingroup datasets

This function will change the parallel access of a variable from
independent to collective.

\param ncid NetCDF or group ID, from a previous call to nc_open(),
nc_create(), nc_def_grp(), or associated inquiry functions such as
nc_inq_ncid().

\param varid Variable ID

\param par_access NC_COLLECTIVE or NC_INDEPENDENT.

\returns ::NC_NOERR No error.
\returns ::NC_EBADID Invalid ncid passed.
\returns ::NC_ENOTVAR Invalid varid passed.
\returns ::NC_ENOPAR LFile was not opened with nc_open_par/nc_create_var.
\returns ::NC_EINVAL Invalid par_access specified.

<h1>Example</h1>

Here is an example from examples/C/parallel_vara.c which changes the
parallel access of a variable and then writes to it.

\code
#define NY 10
#define NX 4

    #include <mpi.h>
    #include <netcdf.h>
    #include <netcdf_par.h>
        ...
    char filename[128];
    int err, ncid, cmode, varid, dimid, dimid[2], buf[NY][NX];
    size_t global_ny, global_nx, start[2], count[2];
        ...
    global_ny = NY;
    global_nx = NX * nprocs;
        ...
    cmode = NC_CLOBBER | NC_PNETCDF;
    err = nc_create_par(filename, cmode, MPI_COMM_WORLD, MPI_INFO_NULL, &ncid); FATAL_ERR
        ...
    err = nc_def_dim(ncid, "Y", global_ny, &dimid[0]); ERR
    err = nc_def_dim(ncid, "X", global_nx, &dimid[1]); ERR
    err = nc_def_var(ncid, "var", NC_INT, 2, dimid, &varid); ERR
        ...
    err = nc_enddef(ncid); ERR
        ...
    err = nc_var_par_access(ncid, varid, NC_COLLECTIVE); ERR
        ...
    start[0] = 0;
    start[1] = NX * rank;
    count[0] = NY;
    count[1] = NX;

    err = nc_put_vara_int(ncid, varid, start, count, &buf[0][0]); ERR

\endcode
\author Ed Hartnett, Dennis Heimbigner
\ingroup datasets
 */
int
nc_var_par_access(int ncid, int varid, int par_access)
{
    NC* ncp;

    int stat = NC_NOERR;

    if ((stat = NC_check_id(ncid, &ncp)))
       return stat;

#ifndef USE_PARALLEL
    return NC_ENOPAR;
#else
    return ncp->dispatch->var_par_access(ncid,varid,par_access);
#endif
}

/**
Create a netCDF file for parallel access from the Fortran API.

This function calls nc_create_par() after converting the MPI comm and
info from Fortran to C, if necessary.

\param path The file name of the new netCDF dataset.

\param cmode The creation mode flag. The following flags are available:
  NC_NOCLOBBER (do not overwrite existing file),
  NC_NETCDF4 (create netCDF-4/HDF5 file),
  NC_CLASSIC_MODEL (enforce netCDF classic mode on netCDF-4/HDF5 files),
  NC_PNETCDF.

\param comm the MPI communicator to be used.

\param info MPI info or MPI_INFO_NULL.

\param ncidp Pointer to location where returned netCDF ID is to be
stored.

\returns ::NC_NOERR No error.
\returns ::NC_ENOPAR Library was not built with parallel I/O features.
\returns ::NC_EINVAL Invalid input parameters.
\returns ::NC_ENOMEM System out of memory.
\returns ::NC_ENOTNC Binary format could not be determined.
\returns ::NC_EHDFERR HDF5 error (netCDF-4 files only).
\returns ::NC_EFILEMETA Error writing netCDF-4 file-level metadata in
HDF5 file. (netCDF-4 files only).

\author Ed Hartnett, Dennis Heimbigner
\ingroup datasets
\internal
*/
int
nc_create_par_fortran(const char *path, int cmode, int comm,
		      int info, int *ncidp)
{
#ifndef USE_PARALLEL
   return NC_ENOPAR;
#else
   MPI_Comm comm_c;
   MPI_Info info_c;

   /* Convert fortran comm and info to C comm and info, if there is a
    * function to do so. Otherwise just pass them. */
#ifdef HAVE_MPI_COMM_F2C
   comm_c = MPI_Comm_f2c(comm);
   info_c = MPI_Info_f2c(info);
#else
   comm_c = (MPI_Comm)comm;
   info_c = (MPI_Info)info;
#endif

   return nc_create_par(path, cmode, comm_c, info_c, ncidp);
#endif
}

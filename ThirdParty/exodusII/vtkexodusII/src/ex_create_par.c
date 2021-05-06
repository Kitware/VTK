/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

/*!
\ingroup Utilities

\note The ex_create_par_int() is an internal function called by
ex_create_par(). The user should call ex_create_par() and not ex_create_par_int().

The function ex_create() creates a new exodus file and returns an ID
that can subsequently be used to refer to the file.

All floating point values in an exodus file are stored as either
4-byte (float) or 8-byte (double) numbers; no mixing of 4- and
8-byte numbers in a single file is allowed. An application code can
compute either 4- or 8-byte values and can designate that the values
be stored in the exodus file as either 4- or 8-byte numbers;
conversion between the 4- and 8-byte values is performed automatically
by the API routines. Thus, there are four possible combinations of
compute word size and storage (or I/O) word size.

\return In case of an error, ex_create() returns a negative number. Possible
causes of errors include:
  -  Passing a file name that includes a directory that does not
 exist.
  -  Specifying a file name of a file that exists and also
 specifying a no clobber option.
  -  Attempting to create a file in a directory without permission
 to create files there.
  -  Passing an invalid file clobber mode.

\param path The file name of the new exodus file. This can be given as either an
            absolute path name (from the root of the file system) or a relative
            path name (from the current directory).

\param cmode Mode. Use one of the following predefined constants:
\arg #EX_NOCLOBBER  To create the new file only if the given file name does
not refer to a
                      file that already exists.

\arg #EX_CLOBBER    To create the new file, regardless of whether a file with
the same
                      name already exists. If a file with the same name does
exist, its
                      contents will be erased.

\arg #EX_64BIT_OFFSET To create a model that can store individual datasets
larger than
                        2 gigabytes. This modifies the internal storage used by
exodusII and
                        also puts the underlying NetCDF file into the \e 64-bit
offset'
                        mode. See largemodel for more details on this
                        mode. A large model file will also be created if the
                        environment variable EXODUS_LARGE_MODEL is defined
                        in the users environment. A message will be printed to
standard output
                        if this environment variable is found. #EX_LARGE_MODEL is
alias.

\arg #EX_NORMAL_MODEL Create a standard model.

\arg #EX_64BIT_DATA      To create a model using the CDF5 format which uses the
                        classic model but has 64-bit dimensions and sizes.
                        This type will also be created if the
                        environment variable EXODUS_NETCDF5 is defined in the
                        users environment. A message will be printed to standard
                        output if
                        this environment variable is found.

\arg #EX_NETCDF4 To create a model using the HDF5-based NetCDF-4
                        output. An HDF5-based NetCDF-4 file will also be created
if the
                        environment variable EXODUS_NETCDF4 is defined in the
                        users environment. A message will be printed to standard
output if
                        this environment variable is found.

\arg #EX_NOSHARE Do not open the underlying NetCDF file in \e share
mode. See the
                        NetCDF documentation for more details.

\arg #EX_SHARE   Do open the underlying NetCDF file in \e share mode. See
the NetCDF
                        documentation for more details.

\param[in,out] comp_ws  The word size in bytes (0, 4 or 8) of the floating point
variables
                        used in the application program. If 0 (zero) is passed,
the default
                        sizeof(float) will be used and returned in this
variable. WARNING: all
                        exodus functions requiring floats must be passed floats
declared with
                        this passed in or returned compute word size (4 or 8).}

\param io_ws            The word size in bytes (4 or 8) of the floating point
                        data as they are to be stored in the exodus file.

\param run_version (internally generated) used to verify compatibility of library
and include files.

The following code segment creates an exodus file called \file{test.exo}:

~~~{.c}
#include "exodusII.h"
int CPU_word_size, IO_word_size, exoid;
CPU_word_size = sizeof(float);      \comment{use float or double}
IO_word_size = 8;                   \comment{store variables as doubles}

\comment{create exodus file}
exoid = ex_create ("test.exo"       \comment{filename path}
                    EX_CLOBBER,     \comment{create mode}
                    &CPU_word_size, \comment{CPU float word size in bytes}
                    &IO_word_size); \comment{I/O float word size in bytes}
~~~

*/

/* Determine whether compiling against a parallel netcdf... */
#include "exodusII.h"
#if defined(PARALLEL_AWARE_EXODUS)

#include "exodusII_int.h"
#include <mpi.h>

/* NOTE: Do *not* call `ex_create_par_int()` directly.  The public API
 *       function name is `ex_create_par()` which is a wrapper that calls
 *       `ex_create_par_int` with an additional argument to make sure
 *       library and include file are consistent
 */
int ex_create_par_int(const char *path, int cmode, int *comp_ws, int *io_ws, MPI_Comm comm,
                      MPI_Info info, int run_version)
{
  int  exoid  = -1;
  int  status = 0;
  char errmsg[MAX_ERR_LENGTH];
  int  nc_mode = 0;

  unsigned int my_mode     = cmode;
  int          is_parallel = 1;

  EX_FUNC_ENTER();

#if !NC_HAS_PARALLEL
  /* Library does NOT support parallel output via netcdf-4 or pnetcdf */
  snprintf(errmsg, MAX_ERR_LENGTH,
           "EXODUS: ERROR: Parallel output requires the netcdf-4 and/or "
           "pnetcdf library format, but this netcdf library does not "
           "support either.\n");
  ex_err(__func__, errmsg, EX_BADPARAM);
  EX_FUNC_LEAVE(EX_FATAL);
#endif

  /* Verify that this file is not already open for read or write...
     In theory, should be ok for the file to be open multiple times
     for read, but bad things can happen if being read and written
     at the same time...
  */
  if (ex__check_multiple_open(path, EX_WRITE, __func__)) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  nc_mode = ex__handle_mode(my_mode, is_parallel, run_version);

  if ((status = nc_create_par(path, nc_mode, comm, info, &exoid)) != NC_NOERR) {
    if (my_mode & EX_NETCDF4) {
#if NC_HAS_PARALLEL4
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: file create failed for %s.", path);
#else
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: file create failed for %s in NetCDF-4 "
               "mode.\n\tThis library does not support parallel NetCDF-4 files (HDF5-based).",
               path);
#endif
    }
    else {
#if NC_HAS_PNETCDF
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: file create failed for %s", path);
#else
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: file create failed for %s in PnetCDF "
               "mode.\n\tThis library does not provide PnetCDF support.",
               path);
#endif
    }
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  status = ex__populate_header(exoid, path, my_mode, is_parallel, comp_ws, io_ws);
  if (status != EX_NOERR) {
    EX_FUNC_LEAVE(status);
  }
  EX_FUNC_LEAVE(exoid);
}
#else
/*
 * Prevent warning in some versions of ranlib(1) because the object
 * file has no symbols.
 */
const char exodus_unused_symbol_dummy_ex_create_par;
#endif

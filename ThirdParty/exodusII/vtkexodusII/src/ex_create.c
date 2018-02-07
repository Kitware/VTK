/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*!

\note The ex_create_int() is an internal function called by
ex_create(). The user should call ex_create() and not ex_create_int().

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
\arg EX_NOCLOBBER  To create the new file only if the given file name does
not refer to a
                      file that already exists.

\arg EX_CLOBBER    To create the new file, regardless of whether a file with
the same
                      name already exists. If a file with the same name does
exist, its
                      contents will be erased.

\arg EX_64BIT_OFFSET To create a model that can store individual datasets
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
                        if this environment variable is found. EX_LARGE_MODEL is
alias.

\arg EX_NORMAL_MODEL Create a standard model.

\arg EX_64BIT_DATA	To create a model using the CDF5 format which uses the
                        classic model but has 64-bit dimensions and sizes.
                        This type will also be created if the
                        environment variable EXODUS_NETCDF5 is defined in the
                        users environment. A message will be printed to standard
                        output if
                        this environment variable is found.

\arg EX_NETCDF4	To create a model using the HDF5-based NetCDF-4
                        output. An HDF5-based NetCDF-4 file will also be created
if the
                        environment variable EXODUS_NETCDF4 is defined in the
                        users environment. A message will be printed to standard
output if
                        this environment variable is found.

\arg EX_NOSHARE	Do not open the underlying NetCDF file in \e share
mode. See the
                        NetCDF documentation for more details.

\arg EX_SHARE	Do open the underlying NetCDF file in \e share mode. See
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

\param run_version (internally generated) used to verify compatability of libary
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
#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h>

static int warning_output = 0;

int ex_create_int(const char *path, int cmode, int *comp_ws, int *io_ws, int run_version)
{
  int   exoid;
  int   status;
  int   dimid;
  int   old_fill;
  int   lio_ws;
  int   filesiz = 1;
  float vers;
  char  errmsg[MAX_ERR_LENGTH];
  char *mode_name;
  int   nc_mode = 0;
#if NC_HAS_HDF5
  static int netcdf4_mode = -1;
#endif /* NC_NETCDF4 */
#if defined(NC_64BIT_DATA)
  static int netcdf5_mode = -1;
#endif
  int          int64_status;
  unsigned int my_mode = cmode;

  /* Contains a 1 in all bits corresponding to file modes */
  static unsigned int all_modes = EX_NORMAL_MODEL | EX_64BIT_OFFSET | EX_64BIT_DATA | EX_NETCDF4;

  EX_FUNC_ENTER();

  if (run_version != EX_API_VERS_NODOT && warning_output == 0) {
    int run_version_major = run_version / 100;
    int run_version_minor = run_version % 100;
    int lib_version_major = EX_API_VERS_NODOT / 100;
    int lib_version_minor = EX_API_VERS_NODOT % 100;
    fprintf(stderr,
            "EXODUS: Warning: This code was compiled with exodusII "
            "version %d.%02d,\n          but was linked with exodusII "
            "library version %d.%02d\n          This is probably an "
            "error in the build process of this code.\n",
            run_version_major, run_version_minor, lib_version_major, lib_version_minor);
    warning_output = 1;
  }

/*
 * See if specified mode is supported in the version of netcdf we
 * are using
 */
#if !NC_HAS_HDF5
  if (my_mode & EX_NETCDF4) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "EXODUS: ERROR: File format specified as netcdf-4, but the "
             "NetCDF library being used was not configured to enable "
             "this format\n");
    ex_err(__func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }
#endif

#if !defined(NC_64BIT_DATA)
  if (my_mode & EX_64BIT_DATA) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "EXODUS: ERROR: File format specified as 64bit_data, but "
             "the NetCDF library being used does not support this "
             "format\n");
    ex_err(__func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }
#endif

  /* Check that one and only one format mode is specified... */
  {
    unsigned int set_modes = all_modes & my_mode;

    if (set_modes == 0) {
      my_mode |= EX_64BIT_OFFSET; /* Default if nothing specified */
    }
    else {
      /* Checks that only a single bit is set */
      set_modes = set_modes && !(set_modes & (set_modes - 1));
      if (!set_modes) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "EXODUS: ERROR: More than 1 file format "
                 "(EX_NORMAL_MODEL, EX_LARGE_MODEL, EX_64BIT_OFFSET, "
                 "EX_64BIT_DATA, or EX_NETCDF4)\nwas specified in the "
                 "mode argument of the ex_create call. Only a single "
                 "format can be specified.\n");
        ex_err(__func__, errmsg, EX_BADPARAM);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

  /*
   * See if any integer data is to be stored as int64 (long long). If
   * so, then need to set NC_NETCDF4 and unset NC_CLASSIC_MODEL (or
   * set EX_NOCLASSIC.  Output meaningful error message if the library
   * is not NetCDF-4 enabled...
   *
   * As of netcdf-4.4.0, can also use NC_64BIT_DATA (CDF5) mode for this...
   */
  int64_status = my_mode & (EX_ALL_INT64_DB | EX_ALL_INT64_API);

  if ((int64_status & EX_ALL_INT64_DB) != 0) {
#if NC_HAS_HDF5 || defined(NC_64BIT_DATA)
    /* Library DOES support netcdf4 and/or cdf5 ... See if user
     * specified either of these and use that one; if not, pick
     * netcdf4, non-classic as default.
     */
    if (my_mode & EX_NETCDF4) {
      my_mode |= EX_NOCLASSIC;
    }
#if defined(NC_64BIT_DATA)
    else if (my_mode & EX_64BIT_DATA) {
      ; /* Do nothing, already set */
    }
#endif
    else {
      /* Unset the current mode so we don't have multiples specified */
      /* ~all_modes sets to 1 all bits not associated with file format */
      my_mode &= ~all_modes;
#if NC_HAS_HDF5
      /* Pick netcdf4 as default mode for 64-bit integers */
      my_mode |= EX_NOCLASSIC;
      my_mode |= EX_NETCDF4;
#else
      /* Pick 64bit_data as default mode for 64-bit integers */
      my_mode |= EX_64BIT_DATA;
#endif
    }
#else
    /* Library does NOT support netcdf4 or cdf5 */
    snprintf(errmsg, MAX_ERR_LENGTH,
             "EXODUS: ERROR: 64-bit integer storage requested, but the "
             "netcdf library does not support the required netcdf-4 or "
             "64BIT_DATA extensions.\n");
    ex_err(__func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
#endif
  }

#if NC_HAS_HDF5
  if (my_mode & EX_NETCDF4) {
    nc_mode |= (NC_NETCDF4);
  }
  else {
    if (netcdf4_mode == -1) {
      char *option = getenv("EXODUS_NETCDF4");
      if (option != NULL) {
        netcdf4_mode = NC_NETCDF4;
        if (option[0] != 'q') {
          fprintf(stderr, "EXODUS: Using netcdf version 4 selected via "
                          "EXODUS_NETCDF4 environment variable\n");
        }
      }
      else {
        netcdf4_mode = 0;
      }
    }
    nc_mode |= netcdf4_mode;
  }
  if (!(my_mode & EX_NOCLASSIC)) {
    nc_mode |= NC_CLASSIC_MODEL;
  }
#endif

#if defined(NC_64BIT_DATA)
  if (my_mode & EX_64BIT_DATA) {
    nc_mode |= (NC_64BIT_DATA);
  }
  else {
    if (netcdf5_mode == -1) {
      char *option = getenv("EXODUS_NETCDF5");
      if (option != NULL) {
        netcdf5_mode = NC_64BIT_DATA;
        if (option[0] != 'q') {
          fprintf(stderr, "EXODUS: Using netcdf version 5 (CDF5) selected via "
                          "EXODUS_NETCDF5 environment variable\n");
        }
      }
      else {
        netcdf5_mode = 0;
      }
    }
    nc_mode |= netcdf5_mode;
  }
#endif

  /*
   * Hardwire filesiz to 1 for all created files. Reduce complexity in nodal output routines.
   * has been default for a decade or so, but still support it on read...
   */
  if (
#if NC_HAS_HDF5
      !(nc_mode & NC_NETCDF4) &&
#endif
#if defined(NC_64BIT_DATA)
      !(nc_mode & NC_64BIT_DATA) &&
#endif
      filesiz == 1) {
    nc_mode |= NC_64BIT_OFFSET;
  }

  if (my_mode & EX_SHARE) {
    nc_mode |= NC_SHARE;
  }

  /*
   * set error handling mode to no messages, non-fatal errors
   */
  ex_opts(exoptval); /* call required to set ncopts first time through */

  if (my_mode & EX_CLOBBER) {
    nc_mode |= NC_CLOBBER;
    mode_name = "CLOBBER";
  }
  else {
    nc_mode |= NC_NOCLOBBER;
    mode_name = "NOCLOBBER";
  }

#if NC_HAS_DISKLESS
  if (my_mode & EX_DISKLESS) {
    nc_mode |= NC_DISKLESS;
  }
#endif

  if ((status = nc_create(path, nc_mode, &exoid)) != NC_NOERR) {
#if NC_HAS_HDF5
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: file create failed for %s, mode: %s", path, mode_name);
#else
    if (my_mode & EX_NETCDF4) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: file create failed for %s in NETCDF4 and %s "
               "mode.\n\tThis library does not support netcdf-4 files.",
               path, mode_name);
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: file create failed for %s, mode: %s", path,
               mode_name);
    }
#endif
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* turn off automatic filling of netCDF variables */
  if ((status = nc_set_fill(exoid, NC_NOFILL, &old_fill)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to set nofill mode in file id %d", exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Verify that there is not an existing file_item struct for this
     exoid This could happen (and has) when application calls
     ex_open(), but then closes file using nc_close() and then reopens
     file.  NetCDF will possibly reuse the exoid which results in
     internal corruption in exodus data structures since exodus does
     not know that file was closed and possibly new file opened for
     this exoid
  */
  if (ex_find_file_item(exoid) != NULL) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: There is an existing file already using the file "
             "id %d which was also assigned to file %s.\n\tWas "
             "nc_close() called instead of ex_close() on an open Exodus "
             "file?\n",
             exoid, path);
    ex_err(__func__, errmsg, EX_BADFILEID);
    nc_close(exoid);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* initialize floating point size conversion.  since creating new file,
   * i/o wordsize attribute from file is zero.
   */
  if (ex_conv_ini(exoid, comp_ws, io_ws, 0, int64_status, 0, 0, 0) != EX_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to init conversion routines in file id %d",
             exoid);
    ex_err(__func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* put the EXODUS version number, and i/o floating point word size as
   * netcdf global attributes
   */

  /* store Exodus API version # as an attribute */
  vers = EX_API_VERS;
  if ((status = nc_put_att_float(exoid, NC_GLOBAL, ATT_API_VERSION, NC_FLOAT, 1, &vers)) !=
      NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store Exodus II API version attribute in file id %d", exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* store Exodus file version # as an attribute */
  vers = EX_VERS;
  if ((status = nc_put_att_float(exoid, NC_GLOBAL, ATT_VERSION, NC_FLOAT, 1, &vers)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store Exodus II file version attribute in file id %d", exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* store Exodus file float word size  as an attribute */
  lio_ws = (*io_ws);
  if ((status = nc_put_att_int(exoid, NC_GLOBAL, ATT_FLT_WORDSIZE, NC_INT, 1, &lio_ws)) !=
      NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store Exodus II file float word size "
             "attribute in file id %d",
             exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* store Exodus file size (1=large, 0=normal) as an attribute */
  if ((status = nc_put_att_int(exoid, NC_GLOBAL, ATT_FILESIZE, NC_INT, 1, &filesiz)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store Exodus II file size attribute in file id %d", exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  {
    int max_so_far = 32;
    if ((status = nc_put_att_int(exoid, NC_GLOBAL, ATT_MAX_NAME_LENGTH, NC_INT, 1, &max_so_far)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to add maximum_name_length attribute in file id %d", exoid);
      ex_err(__func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* define some dimensions and variables */

  /* create string length dimension */
  if ((status = nc_def_dim(exoid, DIM_STR, (MAX_STR_LENGTH + 1), &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define string length in file id %d", exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* The name string length dimension is delayed until the ex_put_init function
   */

  /* create line length dimension */
  if ((status = nc_def_dim(exoid, DIM_LIN, (MAX_LINE_LENGTH + 1), &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define line length in file id %d", exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* create number "4" dimension; must be of type long */
  if ((status = nc_def_dim(exoid, DIM_N4, 4L, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number \"4\" dimension in file id %d",
             exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  {
    int int64_db_status = int64_status & EX_ALL_INT64_DB;
    if ((status = nc_put_att_int(exoid, NC_GLOBAL, ATT_INT64_STATUS, NC_INT, 1,
                                 &int64_db_status)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to add int64_status attribute in file id %d",
               exoid);
      ex_err(__func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if ((status = nc_enddef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to complete definition for file id %d", exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(exoid);
}

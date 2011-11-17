/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
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
 *     * Neither the name of Sandia Corporation nor the names of its
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
4-byte (\c float) or 8-byte (\c double) numbers; no mixing of 4- and
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
\arg \c EX_NOCLOBBER  To create the new file only if the given file name does not refer to a
          file that already exists.

\arg \c EX_CLOBBER    To create the new file, regardless of whether a file with the same
          name already exists. If a file with the same name does exist, its
          contents will be erased.

\arg \c EX_LARGE_MODEL  To create a model that can store individual datasets larger than
      2 gigabytes. This modifies the internal storage used by exodusII and
      also puts the underlying NetCDF file into the \e 64-bit offset'
      mode. See largemodel for more details on this
      mode. A large model file will also be created if the
      environment variable \c EXODUS_LARGE_MODEL is defined
      in the users environment. A message will be printed to standard output
      if this environment variable is found.

\arg \c EX_NORMAL_MODEL Create a standard model.

\arg \c EX_NETCDF4  To create a model using the HDF5-based NetCDF-4
      output. An HDF5-based NetCDF-4 file will also be created if the
      environment variable \c EXODUS_NETCDF4 is defined in the
      users environment. A message will be printed to standard output if
      this environment variable is found.

\arg \c EX_NOSHARE  Do not open the underlying NetCDF file in \e share mode. See the
                        NetCDF documentation for more details.

\arg \c EX_SHARE  Do open the underlying NetCDF file in \e share mode. See the NetCDF
      documentation for more details.

\param[in,out] comp_ws  The word size in bytes (0, 4 or 8) of the floating point variables
      used in the application program. If 0 (zero) is passed, the default
      sizeof(float) will be used and returned in this variable. WARNING: all
      exodus functions requiring floats must be passed floats declared with
      this passed in or returned compute word size (4 or 8).}

\param io_ws            The word size in bytes (4 or 8) of the floating point
      data as they are to be stored in the exodus file.

\param run_version (internally generated) used to verify compatability of libary and include files.

The following code segment creates an exodus file called \file{test.exo}:

\code
#include "exodusII.h"
int CPU_word_size, IO_word_size, exoid;
CPU_word_size = sizeof(float);      \comment{use float or double}
IO_word_size = 8;                   \comment{store variables as doubles}

\comment{create exodus file}
exoid = ex_create ("test.exo"       \comment{filename path}
        EX_CLOBBER,     \comment{create mode}
        &CPU_word_size, \comment{CPU float word size in bytes}
              &IO_word_size); \comment{I/O float word size in bytes}
\endcode

*/
#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h>

int ex_create_int (const char *path,
       int   cmode,
       int  *comp_ws,
       int  *io_ws,
       int   run_version)
{
  int exoid, dims[1];
  int status;
  int dimid, time_dim;
  int old_fill;
  int lio_ws;
  int filesiz;
  float vers;
  char errmsg[MAX_ERR_LENGTH];
  char *mode_name;
  int mode = 0;
#if defined(NC_NETCDF4)
  static int netcdf4_mode = -1;
  char *option;
#endif /* NC_NETCDF4 */
   
  exerrval = 0; /* clear error code */

  if (run_version != EX_API_VERS_NODOT) {
    int run_version_major = run_version / 100;
    int run_version_minor = run_version % 100;
    int lib_version_major = EX_API_VERS_NODOT / 100;
    int lib_version_minor = EX_API_VERS_NODOT % 100;
    fprintf(stderr, "EXODUSII: Warning: This code was compiled with exodusII version %d.%02d,\n          but was linked with exodusII library version %d.%02d\n          This is probably an error in the build process of this code.\n",
      run_version_major, run_version_minor, lib_version_major, lib_version_minor);
  }
#if defined(NC_NETCDF4)
  if (cmode & EX_NETCDF4) {
    mode |= (NC_NETCDF4|NC_CLASSIC_MODEL);
  } else {
    if (netcdf4_mode == -1) {
      option = getenv("EXODUS_NETCDF4");
      if (option != NULL) {
  fprintf(stderr, "EXODUSII: Using netcdf version 4 selected via EXODUS_NETCDF4 environment variable\n");
  netcdf4_mode = NC_NETCDF4|NC_CLASSIC_MODEL;
      } else {
  netcdf4_mode = 0;
      }
    }
    mode |= netcdf4_mode;
  }
#endif

  /*
   * See if "large file" mode was specified in a ex_create cmode. If
   * so, then pass the NC_64BIT_OFFSET flag down to netcdf.
   * If netcdf4 mode specified, don't use NC_64BIT_OFFSET mode.
   */
  if ( (cmode & EX_LARGE_MODEL) && (cmode & EX_NORMAL_MODEL)) {
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
      "Warning: conflicting mode specification for file %s, mode %d. Using normal",
      path, cmode);
    ex_err("ex_create",errmsg,exerrval);
  }
  if ((cmode & EX_NORMAL_MODEL) != 0)
    filesiz = 0;
#if defined(NC_NETCDF4)
  else if ((mode & NC_NETCDF4) != 0)
    filesiz = 1;
#endif
  else 
    filesiz = (int)(((cmode & EX_LARGE_MODEL) != 0) || (ex_large_model(-1) == 1));

  if (
#if defined(NC_NETCDF4)
      !(mode & NC_NETCDF4) &&
#endif
      filesiz == 1) {
    mode |= NC_64BIT_OFFSET;
  }

  if (cmode & EX_SHARE) {
    mode |= NC_SHARE;
  }

  /*
   * set error handling mode to no messages, non-fatal errors
   */
  ex_opts(exoptval);    /* call required to set ncopts first time through */

  if (cmode & EX_CLOBBER) {
    mode |= NC_CLOBBER;
    mode_name = "CLOBBER";
  } else {
    mode |= NC_NOCLOBBER;
    mode_name = "NOCLOBBER";
  }

  if ((status = nc_create (path, mode, &exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: file create failed for %s, mode: %s",
      path, mode_name);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* turn off automatic filling of netCDF variables
   */

  if ((status = nc_set_fill (exoid, NC_NOFILL, &old_fill)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to set nofill mode in file id %d",
      exoid);
    ex_err("ex_create", errmsg, exerrval);
    return (EX_FATAL);
  }

  /* initialize floating point size conversion.  since creating new file, 
   * i/o wordsize attribute from file is zero.
   */

  if (ex_conv_ini( exoid, comp_ws, io_ws, 0 ) != EX_NOERR) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: failed to init conversion routines in file id %d",
            exoid);
    ex_err("ex_create", errmsg, exerrval);
    return (EX_FATAL);
  }

  /* put the EXODUS version number, and i/o floating point word size as
   * netcdf global attributes
   */

  /* store Exodus API version # as an attribute */
  vers = EX_API_VERS;
  if ((status=nc_put_att_float(exoid, NC_GLOBAL, ATT_API_VERSION,
             NC_FLOAT, 1, &vers)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to store Exodus II API version attribute in file id %d",
      exoid);
    ex_err("ex_create",errmsg, exerrval);
    return (EX_FATAL);
  }
   
  /* store Exodus file version # as an attribute */
  vers = EX_VERS;
  if ((status=nc_put_att_float(exoid, NC_GLOBAL, ATT_VERSION, NC_FLOAT, 1, &vers)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to store Exodus II file version attribute in file id %d",
      exoid);
    ex_err("ex_create",errmsg, exerrval);
    return (EX_FATAL);
  }

  /* store Exodus file float word size  as an attribute */
  lio_ws = (int)(*io_ws);
  if ((status=nc_put_att_int (exoid, NC_GLOBAL, ATT_FLT_WORDSIZE, NC_INT, 1, &lio_ws)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to store Exodus II file float word size attribute in file id %d",
      exoid);
    ex_err("ex_create",errmsg, exerrval);
    return (EX_FATAL);
  }

  /* store Exodus file size (1=large, 0=normal) as an attribute */
  if ((status = nc_put_att_int (exoid, NC_GLOBAL, ATT_FILESIZE, NC_INT, 1, &filesiz)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to store Exodus II file size attribute in file id %d",
      exoid);
    ex_err("ex_create",errmsg, exerrval);
    return (EX_FATAL);
  }
  
  /* define some dimensions and variables
   */
  
  /* create string length dimension */
  if ((status=nc_def_dim (exoid, DIM_STR, (MAX_STR_LENGTH+1), &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to define string length in file id %d",exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* The name string length dimension is delayed until the ex_put_init function */

  /* create line length dimension */
  if ((status = nc_def_dim(exoid, DIM_LIN, (MAX_LINE_LENGTH+1), &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to define line length in file id %d",exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* create number "4" dimension; must be of type long */
  if ((status = nc_def_dim(exoid, DIM_N4, 4L, &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to define number \"4\" dimension in file id %d",exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }


  if ((status = nc_def_dim(exoid, DIM_TIME, NC_UNLIMITED, &time_dim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to define time dimension in file id %d", exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  dims[0] = time_dim;
  if ((status = nc_def_var(exoid, VAR_WHOLE_TIME, nc_flt_code(exoid), 1, dims, &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to define whole time step variable in file id %d",
      exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_enddef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to complete definition for file id %d", exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (exoid);
}

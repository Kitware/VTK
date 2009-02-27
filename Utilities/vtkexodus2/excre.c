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
/*****************************************************************************
*
* excre - ex_create
*
* entry conditions - 
*   input parameters:
*       char*   path                    file name including path
*       int     cmode                   access mode r/w
*       int*    comp_ws                 compute word size
*       int*    io_ws                   storage word size
*
* exit conditions - 
*       return value                    exodus file id
*
* revision history - 
*
*  Id
*
*****************************************************************************/
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * creates a new EXODUS II file and returns an id that can subsequently 
 * be used to refer to the file. This in an internal function; the external
 * name of this function ix ex_create()
 * \param      path         filename of file to create.
 * \param      cmode        access mode.  Any of the defines in the \ref FileVars group or'd together.
 * \param comp_ws The word size in bytes (0, 4 or 8) of the floating
 *                point variables used in the application program. If 0 (zero) is
 *                passed, the default sizeof(float) will be used and returned in this
 *                variable. WARNING: all EXODUS II functions requiring floats must be
 *                passed floats declared with this passed in or returned compute word
 *                size (4 or 8).
 * \param io_ws   The word size in bytes (4 or 8) of the floating point data as they
 *                are to be stored in the EXODUS II file. 
 * \param run_version (internally generated) used to verify compatability of libary and include files.
 */

int ex_create_int (const char *path,
		   int   cmode,
		   int  *comp_ws,
		   int  *io_ws,
		   int   run_version)
{
   int exoid, time_dim, dims[1];
   int status;
   int dimid;
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
     mode |= NC_NETCDF4;
   } else {
     if (netcdf4_mode == -1) {
       option = getenv("EXODUS_NETCDF4");
       if (option != NULL) {
	 fprintf(stderr, "EXODUSII: Using netcdf version 4 selected via EXODUS_NETCDF4 environment variable\n");
	 netcdf4_mode = NC_NETCDF4;
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

   if ((status = nc_set_fill (exoid, NC_NOFILL, &old_fill)) != NC_NOERR)
   {
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
  vers = (float) EX_API_VERS;
  if ((status=nc_put_att_float(exoid, NC_GLOBAL, ATT_API_VERSION,
			       NC_FLOAT, 1, &vers)) != NC_NOERR)
  {
    exerrval = status;
    sprintf(errmsg,
         "Error: failed to store Exodus II API version attribute in file id %d",
            exoid);
    ex_err("ex_create",errmsg, exerrval);
    return (EX_FATAL);
  }

/* store Exodus file version # as an attribute */
  vers = (float) EX_VERS;
  if ((status=nc_put_att_float(exoid, NC_GLOBAL, ATT_VERSION, NC_FLOAT, 1, &vers)) != NC_NOERR)
  {
    exerrval = status;
    sprintf(errmsg,
        "Error: failed to store Exodus II file version attribute in file id %d",
            exoid);
    ex_err("ex_create",errmsg, exerrval);
    return (EX_FATAL);
  }

/* store Exodus file float word size  as an attribute */
  lio_ws = (int)(*io_ws);
  if ((status=nc_put_att_int (exoid, NC_GLOBAL, ATT_FLT_WORDSIZE, NC_INT, 1, &lio_ws)) != NC_NOERR)
  {
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


  /* create line length dimension */
  if ((status = nc_def_dim(exoid, DIM_LIN, (MAX_LINE_LENGTH+1), &dimid)) != NC_NOERR)
  {
    exerrval = status;
    sprintf(errmsg,
           "Error: failed to define line length in file id %d",exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* create number "4" dimension; must be of type long */
  if ((status = nc_def_dim(exoid, DIM_N4, 4L, &dimid)) != NC_NOERR)
  {
    exerrval = status;
    sprintf(errmsg,
           "Error: failed to define number \"4\" dimension in file id %d",exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }


  if ((status = nc_def_dim(exoid, DIM_TIME, NC_UNLIMITED, &time_dim)) != NC_NOERR)
  {
    exerrval = status;
    sprintf(errmsg,
           "Error: failed to define time dimension in file id %d", exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  dims[0] = time_dim;
  if ((status = nc_def_var(exoid, VAR_WHOLE_TIME, nc_flt_code(exoid), 1, dims, &dimid)) != NC_NOERR)
  {
    exerrval = status;
    sprintf(errmsg,
           "Error: failed to define whole time step variable in file id %d",
            exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_enddef (exoid)) != NC_NOERR)
  {
    exerrval = status;
    sprintf(errmsg,
           "Error: failed to complete definition for file id %d", exoid);
    ex_err("ex_create",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (exoid);
}

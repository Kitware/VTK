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
* exopen - ex_open
*
* entry conditions - 
*   input parameters:
*       char*   path                    exodus filename path
*       int     mode                    access mode w/r
*
* exit conditions - 
*       int     exoid                   exodus file id
*       int*    comp_ws                 computer word size
*       int*    io_ws                   storage word size
*       float*  version                 EXODUSII interface version number
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdio.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * opens an existing EXODUS II file (or EXODUS II history file) and returns 
 * an id that can subsequently be used to refer to the file.  Multiple files 
 * may be open simultaneously. 
 * \param  path                    exodus filename path
 * \param  mode                    access mode w/r
 * \param[out]  comp_ws            computer word size
 * \param[out]  io_ws              storage word size
 * \param[out]  version            EXODUSII interface version number 
 * \param       run_version        EXODUSII version number of linked library
 * \return      exoid              exodus file id
 */

int ex_open_int (const char  *path,
                 int    mode,
                 int   *comp_ws,
                 int   *io_ws,
                 float *version,
                 int    run_version)
{
   int exoid;
   int status;
   int old_fill;
   int file_wordsize;
   char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */
 
/* set error handling mode to no messages, non-fatal errors */
  ex_opts(exoptval);    /* call required to set ncopts first time through */

  if (run_version != EX_API_VERS_NODOT) {
    int run_version_major = run_version / 100;
    int run_version_minor = run_version % 100;
    int lib_version_major = EX_API_VERS_NODOT / 100;
    int lib_version_minor = EX_API_VERS_NODOT % 100;
    fprintf(stderr, "EXODUSII: Warning: This code was compiled with exodusII version %d.%02d,\n          but was linked with exodusII library version %d.%02d\n          This is probably an error in the build process of this code.\n",
            run_version_major, run_version_minor, lib_version_major, lib_version_minor);
  }
  
  if (mode == EX_READ) { /* READ ONLY */
#if defined(__LIBCATAMOUNT__)
    if ((status = nc_open (path, NC_NOWRITE, &exoid)) != NC_NOERR)
#else
    if ((status = nc_open (path, NC_NOWRITE|NC_SHARE, &exoid)) != NC_NOERR)
#endif
    {
      /* NOTE: netCDF returns an id of -1 on an error - but no error code! */
      if (status == 0)
        exerrval = EX_FATAL;
      else
        exerrval = status;
      sprintf(errmsg,"Error: failed to open %s read only",path);
      ex_err("ex_open",errmsg,exerrval); 
      return(EX_FATAL);
    } 
  }

  else if (mode == EX_WRITE) /* READ/WRITE */
  {
#if defined(__LIBCATAMOUNT__)
    if ((status = nc_open (path, NC_WRITE, &exoid)) != NC_NOERR)
#else
    if ((status = nc_open (path, NC_WRITE|NC_SHARE, &exoid)) != NC_NOERR)
#endif
    {
      /* NOTE: netCDF returns an id of -1 on an error - but no error code! */
      if (status == 0)
        exerrval = EX_FATAL;
      else
        exerrval = status;
      sprintf(errmsg,"Error: failed to open %s write only",path);
      ex_err("ex_open",errmsg,exerrval); 
      return(EX_FATAL);
    } 

    /* turn off automatic filling of netCDF variables */

    if ((status = nc_set_fill (exoid, NC_NOFILL, &old_fill)) != NC_NOERR)
    {
      exerrval = status;
      sprintf(errmsg,
             "Error: failed to set nofill mode in file id %d",
              exoid);
      ex_err("ex_open", errmsg, exerrval);
      return (EX_FATAL);
    }
  }
  else 
  {
    exerrval = EX_BADFILEMODE;
    sprintf(errmsg,"Error: invalid file open mode: %d",mode);
    ex_err("ex_open",errmsg,exerrval); 
    return (EX_FATAL);
  }

/* determine version of EXODUS II file, and the word size of
 * floating point values stored in the file
 */

  if ((status = nc_get_att_float(exoid, NC_GLOBAL, ATT_VERSION, version)) != NC_NOERR) {
     exerrval  = status;
     sprintf(errmsg,"Error: failed to get database version for file id: %d",
             exoid);
     ex_err("ex_open",errmsg,exerrval);
     return(EX_FATAL);
   }
   
/* check ExodusII file version - old version 1.x files are not supported */
   if (*version < 2.0)
   {
     exerrval  = EX_FATAL;
     sprintf(errmsg,"Error: Unsupported file version %.2f in file id: %d",
             *version, exoid);
     ex_err("ex_open",errmsg,exerrval);
     return(EX_FATAL);
   }
   
   if (nc_get_att_int (exoid, NC_GLOBAL, ATT_FLT_WORDSIZE, &file_wordsize) != NC_NOERR)
   {  /* try old (prior to db version 2.02) attribute name */
     if (nc_get_att_int (exoid,NC_GLOBAL,ATT_FLT_WORDSIZE_BLANK,&file_wordsize) != NC_NOERR)
     {
       exerrval  = EX_FATAL;
       sprintf(errmsg,"Error: failed to get file wordsize from file id: %d",
             exoid);
       ex_err("ex_open",errmsg,exerrval);
       return(exerrval);
     }
   }

/* initialize floating point size conversion.
 */

   if (ex_conv_ini( exoid, comp_ws, io_ws, file_wordsize ) != EX_NOERR ) {
     exerrval = EX_FATAL;
     sprintf(errmsg,
           "Error: failed to init conversion routines in file id %d",
            exoid);
     ex_err("ex_open", errmsg, exerrval);
     return (EX_FATAL);
   }

   return (exoid);
}

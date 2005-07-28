/*
 * Copyright (c) 1994 Sandia Corporation. Under the terms of Contract
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
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
*          
* environment - UNIX
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

/*
 * opens an existing EXODUS II file (or EXODUS II history file) and returns 
 * an id that can subsequently be used to refer to the file.  Multiple files 
 * may be open simultaneously
 */

int ex_open (const char  *path,
             int    mode,
             int   *comp_ws,
             int   *io_ws,
             float *version)
{
   int exoid;
   nclong file_wordsize;
   char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */
 
/* set error handling mode to no messages, non-fatal errors */
  ex_opts(exoptval);    /* call required to set ncopts first time through */


  if (mode == EX_READ)  /* READ ONLY */
  {
#ifdef TFLOP
    if ((exoid = ncopen (path, NC_NOWRITE)) < 0)
#else
    if ((exoid = ncopen (path, NC_NOWRITE|NC_SHARE)) < 0)
#endif
    {
      /* NOTE: netCDF returns an id of -1 on an error - but no error code! */
      if (ncerr == 0)
        exerrval = EX_FATAL;
      else
        exerrval = ncerr;
      sprintf(errmsg,"Error: failed to open %s read only",path);
      ex_err("ex_open",errmsg,exerrval); 
      return(EX_FATAL);
    } 
  }

  else if (mode == EX_WRITE) /* READ/WRITE */
  {
#ifdef TFLOP
    if ((exoid = ncopen (path, NC_WRITE)) < 0)
#else
    if ((exoid = ncopen (path, NC_WRITE|NC_SHARE)) < 0)
#endif
    {
      /* NOTE: netCDF returns an id of -1 on an error - but no error code! */
      if (ncerr == 0)
        exerrval = EX_FATAL;
      else
        exerrval = ncerr;
      sprintf(errmsg,"Error: failed to open %s write only",path);
      ex_err("ex_open",errmsg,exerrval); 
      return(EX_FATAL);
    } 

    /* turn off automatic filling of netCDF variables */

    if (ncsetfill (exoid, NC_NOFILL) == -1)
    {
      exerrval = ncerr;
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

   if (ncattget (exoid, NC_GLOBAL, ATT_VERSION, version) == -1)
   {
     exerrval  = ncerr;
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
   
   if (ncattget (exoid, NC_GLOBAL, ATT_FLT_WORDSIZE, &file_wordsize) == -1)
   {  /* try old (prior to db version 2.02) attribute name */
     if (ncattget (exoid,NC_GLOBAL,ATT_FLT_WORDSIZE_BLANK,&file_wordsize) == -1)
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

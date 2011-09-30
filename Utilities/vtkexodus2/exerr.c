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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
\fn{void ex_err(const char *module_name, const char *message, int err_num)}

The function ex_err() logs an error to \c stderr. It is intended
to provide explanatory messages for error codes returned from other
exodus routines.

The passed in error codes and corresponding messages are listed in
???. The programmer may supplement the error message printed
for standard errors by providing an error message. If the error code
is provided with no error message, the predefined message will be
used. The error code \c EX_MSG is available to log application
specific messages.

\param[in]  module_name  This is a string containing the name of the calling function.
\param[in]  message      This is a string containing a message explaining the error 
                         or problem. If \c EX_VERBOSE (see ex_opts()) is true, 
                         this message will be printed to \c stderr. Otherwise, 
       nothing will be printed. Maximum length is \c MAX_ERR_LENGTH.

\param[in] err_num       This is an integer code identifying the error. exodus C functions
                         place an error code value in \c exerrval, an external int. Negative
       values are considered fatal errors while positive values are
       warnings. There is a set of predefined values defined in
       \file{exodusII.h}. The predefined constant \c EX_PRTLASTMSG will
       cause the last error message to be output, regardless of the setting
       of the error reporting level (see ex_opts()).

The following is an example of the use of this function:

\code
#include "exodusII.h"
int exoid, CPU_word_size, IO_word_size, errval;
float version;
char errmsg[MAX_ERR_LENGTH];

CPU_word_size = sizeof(float); 
IO_word_size = 0;

\comment{open exodus file}
if (exoid = ex_open ("test.exo", EX_READ, &CPU_word_size, 
                     &IO_word_size, &version)) {
   errval = 999;
   sprintf(errmsg,"Error: cannot open file test.exo");
   ex_err("prog_name", errmsg, errval);
}
\endcode

*/

int exerrval = 0;               /* clear initial global error code value */

static char last_pname[MAX_ERR_LENGTH];
static char last_errmsg[MAX_ERR_LENGTH];
static int last_err_num;

void ex_err(const char *module_name,      
            const char *message, 
            int err_num)            
{
  if (err_num == 0)             /* zero is no error, ignore and return */
    return;

  else if (err_num ==  EX_PRTLASTMSG)
  {
    fprintf(stderr, "[%s] %s\n",last_pname,last_errmsg);
    fprintf(stderr, "    exerrval = %d\n",last_err_num);
    return;
  }

  else if (exoptval & EX_VERBOSE) /* check see if we really want to hear this */
  {
    fprintf(stderr, "Exodus Library Error: [%s]\n\t%s\n",module_name,message);
    switch (err_num) {
    case NC_SYSERR:
      fprintf (stderr,"\t[%d] System error -- Usually disk full or filesystem issue\n", err_num);
      break;
    case NC_ESTS:
      fprintf (stderr,"\t[%d] In FORTRAN interface, string too small\n", err_num);
      break;
    case NC_EMAXNAME:
      fprintf (stderr,"\t[%d] length of name exceeds NC_MAX_NAME\n", err_num);
      break;
    case NC_EMAXDIMS:
      fprintf (stderr,"\t[%d] netcdf constraint NC_MAX_DIMS exceeded\n", err_num);
      break;
    case NC_EMAXVARS:
      fprintf (stderr,"\t[%d] netcdf constraint NC_MAX_VARS exceeded\n", err_num);
      break;
    case NC_EBADID:
      fprintf (stderr,"\t[%d] Not a netcdf id\n", err_num);
      break;
    case NC_ENFILE:
      fprintf (stderr,"\t[%d] Too many exodus (netcdf) files open\n", err_num);
      break;
    case NC_EEXIST:
      fprintf (stderr,"\t[%d] exodus (netcdf) file exists && NC_NOCLOBBER\n", err_num);
      break;
    case NC_EINVAL:
      fprintf (stderr,"\t[%d] Invalid Argument\n", err_num);
      break;
    case NC_EPERM:
      fprintf (stderr,"\t[%d] Write to read only\n", err_num);
      break;
    case NC_ENOTINDEFINE:
      fprintf (stderr,"\t[%d] Operation not allowed in data mode\n", err_num);
      break;
    case NC_EINDEFINE:
      fprintf (stderr,"\t[%d] Operation not allowed in define mode\n", err_num);
      break;
    case NC_EINVALCOORDS:
      fprintf (stderr,"\t[%d] Index exceeds dimension bound\n", err_num);
      break;
    case NC_ENAMEINUSE:
      fprintf (stderr,"\t[%d] String match to name in use\n", err_num);
      break;
    case NC_ENOTATT:
      fprintf (stderr,"\t[%d] Attribute not found\n", err_num);
      break;
    case NC_EMAXATTS:
      fprintf (stderr,"\t[%d] NC_MAX_ATTRS exceeded\n", err_num);
      break;
    case NC_EBADTYPE:
      fprintf (stderr,"\t[%d] Not a netcdf data type\n", err_num);
      break;
    case NC_EBADDIM:
      fprintf (stderr,"\t[%d] Invalid dimension id or name\n", err_num);
      break;
    case NC_EUNLIMPOS:
      fprintf (stderr,"\t[%d] NC_UNLIMITED in the wrong index\n", err_num);
      break;
    case NC_ENOTVAR:
      fprintf (stderr,"\t[%d] Variable not found\n", err_num);
      break;
    case NC_EGLOBAL:
      fprintf (stderr,"\t[%d] Action prohibited on NC_GLOBAL varid\n", err_num);
      break;
    case NC_ENOTNC:
      fprintf (stderr,"\t[%d] Not an exodus (netcdf) file\n", err_num);
      break;
    case NC_EUNLIMIT:
      fprintf (stderr,"\t[%d] NC_UNLIMITED size already in use\n", err_num);
      break;
    case NC_ENORECVARS:
      fprintf (stderr,"\t[%d] nc_rec op when there are no record vars\n", err_num);
      break;
    case NC_ECHAR:
      fprintf (stderr,"\t[%d] Attempt to convert between text & numbers\n", err_num);
      break;
    case NC_EEDGE:
      fprintf (stderr,"\t[%d] Start+count exceeds dimension bound\n", err_num);
      break;
    case NC_ESTRIDE:
      fprintf (stderr,"\t[%d] Illegal stride\n", err_num);
      break;
    case NC_EBADNAME:
      fprintf (stderr,"\t[%d] Attribute or variable name contains illegal characters\n", err_num);
      break;
    case NC_ERANGE:
      fprintf (stderr,"\t[%d] Math result not representable\n", err_num);
      break;
    case NC_ENOMEM:
      fprintf (stderr,"\t[%d] Memory allocation (malloc) failure\n", err_num);
      break;
    case NC_EVARSIZE:
      fprintf (stderr,"\t[%d] One or more variable sizes violate format constraints\n", err_num);
      break;
    case NC_EDIMSIZE:
      fprintf (stderr,"\t[%d] Invalid dimension size\n", err_num);
      break;
    case NC_ETRUNC:
      fprintf (stderr,"\t[%d] File likely truncated or possibly corrupted\n", err_num);
      break;
    case NC_EAXISTYPE:
      fprintf (stderr,"\t[%d] Unknown axis type.\n", err_num);
      break;
    case EX_MSG:
      break;
    default:
      if (exoptval & EX_VERBOSE)
  fprintf(stderr, "    exerrval = %d\n",err_num);
    }
  } 
  /* save the error message for replays */
  strcpy(last_errmsg, message);
  strcpy(last_pname, module_name);
  last_err_num = err_num;

  fflush(stderr);

  /* with netCDF 3.4, (fatal) system error codes are > 0; 
     so all EXODUS fatal error codes are > 0    */
  if ((err_num > 0) && (exoptval & EX_ABORT))
    exit (err_num);
}

void ex_get_err( const char** msg, const char** func, int* err_num )
 {
   (*msg) = last_errmsg;
   (*func) = last_pname;
   (*err_num) = last_err_num;
 }


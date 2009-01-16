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
* exerr - ex_err
*
* entry conditions - 
*   input parameters:
*       char*   pname                   procedure name
*       char*   err_string              error message string
*       int     errcode                 error code
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "netcdf.h"
#include "exodusII.h"
#include "exodusII_int.h"

int exerrval = 0;               /* clear initial global error code value */

static char last_pname[MAX_ERR_LENGTH];
static char last_errmsg[MAX_ERR_LENGTH];
static int last_errcode;

/**
 * Generalized error reporting function.
 * global integer used for
 * suppressing error messages and determining the fatality of errors.
 * \param pname      string containing the name of the calling function.
 * \param err_string string containing a message explaining the error or problem.
 *                   If EX_VERBOSE (see ex_opts()) is true, this message will be
 *                   printed to stderr. Otherwise, nothing will be printed.
 *                   Maximum length is #MAX_ERR_LENGTH.

 * \param errcode code identifying the error. EXODUS II C functions
 *                place an error code value in exerrval, an external int. Negative
 *                values are considered fatal errors while positive values are
 *                warnings. There is a set of predefined values defined in
 *                exodusII.h, see group \ref ErrorReturnCodes. 
 *                The predefined constant #EX_PRTLASTMSG will cause the
 *                last error message to be output, regardless of the setting of the
 *                error reporting level (see ex_opts()).
 */
void ex_err(const char *pname,      
            const char *err_string, 
            int errcode)            
{
  if (errcode == 0)             /* zero is no error, ignore and return */
    return;

  else if (errcode ==  EX_PRTLASTMSG)
  {
    fprintf(stderr, "[%s] %s\n",last_pname,last_errmsg);
    fprintf(stderr, "    exerrval = %d\n",last_errcode);
    return;
  }

  else if (exoptval & EX_VERBOSE) /* check see if we really want to hear this */
  {
    fprintf(stderr, "[%s] %s\n",pname,err_string);
    if (exoptval & EX_VERBOSE)
      fprintf(stderr, "    exerrval = %d\n",errcode);
    switch (errcode) 
    {
      case NC_ESTS:
        fprintf (stderr," In FORTRAN interface, string too small\n");
        break;
      case NC_EMAXNAME:
        fprintf (stderr," length of name exceeds NC_MAX_NAME\n");
        break;
      case EX_MSG:
        break;
    }
  } 
  /* save the error message for replays */
  strcpy(last_errmsg, err_string);
  strcpy(last_pname, pname);
  last_errcode = errcode;

  fflush(stderr);

  /* with netCDF 3.4, (fatal) system error codes are > 0; 
     so all EXODUS fatal error codes are > 0    */
  if ((errcode > 0) && (exoptval & EX_ABORT))
    exit (errcode);
}

void ex_get_err( const char** msg, const char** func, int* errcode )
 {
   (*msg) = last_errmsg;
   (*func) = last_pname;
   (*errcode) = last_errcode;
 }


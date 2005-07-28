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
* exerr - ex_err
*
* author - Sandia National Laboratories
*          Vic Yarberry    - Original
*
*          
* environment - UNIX
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

/* Generalized error reporting function
 * global integer used for suppressing error messages and determining
 * the fatality of errors.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "netcdf.h"
#include "exodusII.h"

int exerrval = 0;               /* clear initial global error code value */

static char last_pname[MAX_ERR_LENGTH];
static char last_errmsg[MAX_ERR_LENGTH];
static int last_errcode;

void ex_err( const char *pname, /* procedure name */
             const char *err_string,    /* error message string */
             int errcode)       /* error code */
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
      case NC_ENTOOL:
        fprintf (stderr," length of name exceeds MAX_NC_NAME\n");
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

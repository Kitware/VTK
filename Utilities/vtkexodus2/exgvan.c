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
* exgvan - ex_get_var_names
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
*       int     exoid                   exodus file id
*       char*   var_type                variable type: G,N, or E
*       int     num_vars                # of variables to read
*
* exit conditions - 
*       char*   var_names               ptr array of variable names
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

#include <ctype.h>

/*!
 * reads the names of the results variables from the database
 */

int ex_get_var_names (int   exoid,
                      const char *var_type,
                      int   num_vars,
                      char *var_names[])
{
  int i, varid, status;
  char errmsg[MAX_ERR_LENGTH];
  int vartyp;
  const char* tname;
  const char* vvarname;

  exerrval = 0; /* clear error code */

  vartyp = tolower( *var_type );

  switch (vartyp) {
  case 'g':
    tname = "global";
    vvarname = VAR_NAME_GLO_VAR;
    break;
  case 'n':
    tname = "nodal";
    vvarname = VAR_NAME_NOD_VAR;
    break;
  case 'l':
    tname = "edge block";
    vvarname = VAR_NAME_EDG_VAR;
    break;
  case 'f':
    tname = "face block";
    vvarname = VAR_NAME_FAC_VAR;
    break;
  case 'e':
    tname = "element block";
    vvarname = VAR_NAME_ELE_VAR;
    break;
  case 'm':
    tname = "node set";
    vvarname = VAR_NAME_NSET_VAR;
    break;
  case 'd':
    tname = "edge set";
    vvarname = VAR_NAME_ESET_VAR;
    break;
  case 'a':
    tname = "face set";
    vvarname = VAR_NAME_FSET_VAR;
    break;
  case 's':
    tname = "side set";
    vvarname = VAR_NAME_SSET_VAR;
    break;
  case 't':
    tname = "element set";
    vvarname = VAR_NAME_ELSET_VAR;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
      "Warning: invalid variable type %c requested from file id %d",
      *var_type, exoid);
    ex_err("ex_get_var_param",errmsg,exerrval);
    return (EX_WARN);
  }

  /* inquire previously defined variables  */
  if ((varid = ncvarid (exoid, vvarname)) == -1) {
    exerrval = ncerr;
    sprintf(errmsg, "Warning: no %s variables names stored in file id %d", tname,exoid);
    ex_err("ex_get_var_names",errmsg,exerrval);
    return (EX_WARN);
  }

  /* read the variable names */

  /*
   * See if reading into contiguous memory in which case we can load 
   * all values in one call.  If not, we must load each name individually.
   */
  if ((int)(&var_names[num_vars-1][0] - &var_names[0][0]) ==
      sizeof(char)*(MAX_STR_LENGTH+1)*(num_vars-1)) {
    status = nc_get_var_text(exoid, varid, &var_names[0][0]);
    if (status == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get results variable names from file id %d", exoid);
      ex_err("ex_get_var_names",errmsg,exerrval);
      return (EX_FATAL);
    }
  } else {
    for (i=0; i<num_vars; i++) {
      size_t start[2];
      size_t count[2];
      start[0] = i;  count[0] = 1;
      start[1] = 0;  count[1] = MAX_STR_LENGTH+1;
      status = nc_get_vara_text(exoid, varid, start, count, var_names[i]);
      if (status == -1) {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to get results variable names from file id %d", exoid);
        ex_err("ex_get_var_names",errmsg,exerrval);
        return (EX_FATAL);
      }
    }
  }
  return (EX_NOERR);
}

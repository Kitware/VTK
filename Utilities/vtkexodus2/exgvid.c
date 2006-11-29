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
* exgvid - ex_get_varid
*
* entry conditions - 
*   input parameters:
*       int     exoid    exodus file id
*
* exit conditions - 
*       int*    varid    variable id array
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * reads the EXODUS II variable varids from the database
 */

int ex_get_varid (int  exoid, const char *var_type,
                       int *varid_arr)
{
  int  varid, dimid, i, j;
  long num_entity = -1;
  long num_var = -1;
  char errmsg[MAX_ERR_LENGTH];
  const char* routine = "ex_get_elem_varid";

  /*
   * The ent_type and the var_name are used to build the netcdf
   * variables name.  Normally this is done via a macro defined in
   * exodusII_int.h
   */
  const char* ent_type = NULL;
  const char* var_name = NULL;

  exerrval = 0; /* clear error code */
 
  if (*var_type == 'n' || *var_type == 'N') {
    /* Handle nodal variables in a node-specific manner */
    return ex_get_nodal_varid(exoid, varid_arr);
  }
  else if (*var_type == 'e' || *var_type == 'E') {
    dimid = ex_get_dimension(exoid, DIM_NUM_EL_BLK,   "element", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_ELE_VAR,  "element variables", &num_var,    routine);
    var_name = "vals_elem_var";
    ent_type = "eb";
  }
  else if (*var_type == 'm' || *var_type == 'M') {
    dimid = ex_get_dimension(exoid, DIM_NUM_NS,       "nodeset", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_NSET_VAR, "nodeset variables", &num_var,    routine);
    var_name = "vals_nset_var";
    ent_type = "ns";
  }
  else if (*var_type == 's' || *var_type == 'S') {
    dimid = ex_get_dimension(exoid, DIM_NUM_SS,       "sideset", &num_entity, routine);
    varid = ex_get_dimension(exoid, DIM_NUM_SSET_VAR, "sideset variables", &num_var,    routine);
    var_name = "vals_sset_var";
    ent_type = "ss";
  }
  else {       /* invalid variable type */
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
      "Error: Invalid variable type %c specified in file id %d",
      *var_type, exoid);
    ex_err("ex_get_varid",errmsg,exerrval);
    return (EX_WARN);
  }
  
  if (dimid < 0 || varid < 0)
    return(EX_FATAL);
  
  if (num_entity == 0 || num_var == 0)
    return(EX_WARN);
  
  /* since truth table isn't stored in the data file, derive it dynamically */
  for (j=0; j<num_entity; j++) {
    for (i=0; i<num_var; i++) {
      /* NOTE: names are 1-based */
      if ((varid = ncvarid (exoid, ex_catstr2(var_name, i+1, ent_type, j+1))) == -1)
        /* variable doesn't exist; put a 0 in the varid_arr table */
        varid_arr[j*num_var+i] = 0;
      else
        /* variable exists; put varid in the table */
        varid_arr[j*num_var+i] = varid;
    }
  }
  return (EX_NOERR);
}


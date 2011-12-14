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
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * reads the EXODUS II variable varids from the database
 */

int ex_get_varid (int  exoid,
      ex_entity_type obj_type,
      int *varid_arr)
{
  int  status1, status2;
  int varid;
  size_t i, j;
  size_t num_entity = 0;
  size_t num_var = 0;
  char errmsg[MAX_ERR_LENGTH];
  const char* routine = "ex_get_varid";

  /*
   * The ent_type and the var_name are used to build the netcdf
   * variables name.  Normally this is done via a macro defined in
   * exodusII_int.h
   */
  const char* ent_type = NULL;
  const char* var_name = NULL;

  exerrval = 0; /* clear error code */
 
  if (obj_type == EX_NODAL){
    /* Handle nodal variables in a node-specific manner */
    return ex_get_nodal_varid(exoid, varid_arr);
  }
  else if (obj_type == EX_ELEM_BLOCK) {
    status1 = ex_get_dimension(exoid, DIM_NUM_ELE_VAR,  "element variables", &num_var, &varid, routine);
    var_name = "vals_elem_var";
    ent_type = "eb";
  }
  else if (obj_type == EX_NODE_SET) {
    status1 = ex_get_dimension(exoid, DIM_NUM_NSET_VAR, "nodeset variables", &num_var, &varid, routine);
    var_name = "vals_nset_var";
    ent_type = "ns";
  }
  else if (obj_type == EX_SIDE_SET) {
    status1 = ex_get_dimension(exoid, DIM_NUM_SSET_VAR, "sideset variables", &num_var, &varid, routine);
    var_name = "vals_sset_var";
    ent_type = "ss";
  }
  else if (obj_type == EX_EDGE_BLOCK) {
    status1 = ex_get_dimension(exoid, DIM_NUM_EDG_VAR,  "edge variables", &num_var,    &varid, routine);
    var_name = "vals_edge_var";
    ent_type = "eb";
  }
  else if (obj_type == EX_EDGE_SET) {
    status1 = ex_get_dimension(exoid, DIM_NUM_ESET_VAR, "edgeset variables", &num_var, &varid, routine);
    var_name = "vals_eset_var";
    ent_type = "es";
  }
  else if (obj_type == EX_FACE_BLOCK) {
    status1 = ex_get_dimension(exoid, DIM_NUM_FAC_VAR,  "face variables", &num_var,    &varid, routine);
    var_name = "vals_face_var";
    ent_type = "fb";
  }
  else if (obj_type == EX_FACE_SET) {
    status1 = ex_get_dimension(exoid, DIM_NUM_FSET_VAR, "faceset variables", &num_var, &varid, routine);
    var_name = "vals_fset_var";
    ent_type = "fs";
  }
  else if (obj_type == EX_ELEM_SET) {
    status1 = ex_get_dimension(exoid, DIM_NUM_ELSET_VAR, "elementset variables", &num_var, &varid, routine);
    var_name = "vals_elset_var";
    ent_type = "es";
  }
  else {       /* invalid variable type */
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
      "Error: Invalid object type %d specified in file id %d",
      obj_type, exoid);
    ex_err("ex_get_varid",errmsg,exerrval);
    return (EX_WARN);
  }
  
  status2 = ex_get_dimension(exoid, ex_dim_num_objects(obj_type),
           ex_name_of_object(obj_type), &num_entity, &varid, routine);
  if (status2 != NC_NOERR || status1 != NC_NOERR)
    return(EX_FATAL);
  
  if (num_entity == 0 || num_var == 0)
    return(EX_WARN);
  
  for (j=0; j<num_entity; j++) {
    for (i=0; i<num_var; i++) {
      /* NOTE: names are 1-based */
      if (nc_inq_varid(exoid, ex_catstr2(var_name, i+1, ent_type, j+1), &varid) == NC_NOERR)
        /* variable exists; put varid in the varid_arr table */
        varid_arr[j*num_var+i] = varid;
      else
        /* variable doesn't exist; put 0 in the table */
        varid_arr[j*num_var+i] = 0;
    }
  }
  return (EX_NOERR);
}


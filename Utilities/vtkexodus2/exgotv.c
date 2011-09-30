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
* exgotv - ex_get_object_truth_vector
*
* revision history - 
*
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * reads the EXODUS II specified variable truth vector from the database
 */

int ex_get_object_truth_vector (int  exoid,
        ex_entity_type obj_type,
        int  entity_id,
        int  num_var,
        int *var_vec)
{
  int statust;
  int varid, tabid, i, status, ent_ndx;
  size_t num_var_db = 0;
  size_t start[2], count[2]; 
  char errmsg[MAX_ERR_LENGTH];
  const char* routine = "ex_get_object_truth_vector";

  /*
   * The ent_type and the var_name are used to build the netcdf
   * variables name.  Normally this is done via a macro defined in
   * exodusII_int.h
   */
  const char* ent_type = NULL;
  const char* var_name = NULL;

  exerrval = 0; /* clear error code */
   
  switch (obj_type) {
  case EX_EDGE_BLOCK:
    status = ex_get_dimension(exoid, DIM_NUM_EDG_VAR,  "edge variables", &num_var_db, &varid, routine);
    statust = nc_inq_varid(exoid, VAR_EBLK_TAB, &tabid);
    var_name = "vals_edge_var";
    ent_type = "eb";
    break;
  case EX_FACE_BLOCK:
    status = ex_get_dimension(exoid, DIM_NUM_FAC_VAR,  "face variables", &num_var_db, &varid, routine);
    statust = nc_inq_varid (exoid, VAR_FBLK_TAB, &tabid);
    var_name = "vals_face_var";
    ent_type = "fb";
    break;
  case EX_ELEM_BLOCK:
    status = ex_get_dimension(exoid, DIM_NUM_ELE_VAR,  "element variables", &num_var_db, &varid, routine);
    statust = nc_inq_varid (exoid, VAR_ELEM_TAB, &tabid);
    var_name = "vals_elem_var";
    ent_type = "eb";
    break;
  case EX_NODE_SET:
    status = ex_get_dimension(exoid, DIM_NUM_NSET_VAR, "nodeset variables", &num_var_db, &varid, routine);
    statust = nc_inq_varid (exoid, VAR_NSET_TAB, &tabid);
    var_name = "vals_nset_var";
    ent_type = "ns";
    break;
  case EX_EDGE_SET:
    status = ex_get_dimension(exoid, DIM_NUM_ESET_VAR, "edgeset variables", &num_var_db, &varid, routine);
    statust = nc_inq_varid (exoid, VAR_ESET_TAB, &tabid);
    var_name = "vals_eset_var";
    ent_type = "es";
    break;
  case EX_FACE_SET:
    status = ex_get_dimension(exoid, DIM_NUM_FSET_VAR, "faceset variables", &num_var_db, &varid, routine);
    statust = nc_inq_varid (exoid, VAR_FSET_TAB, &tabid);
    var_name = "vals_fset_var";
    ent_type = "fs";
    break;
  case EX_SIDE_SET:
    status = ex_get_dimension(exoid, DIM_NUM_SSET_VAR, "sideset variables", &num_var_db, &varid, routine);
    statust = nc_inq_varid (exoid, VAR_SSET_TAB, &tabid);
    var_name = "vals_sset_var";
    ent_type = "ss";
    break;
  case EX_ELEM_SET:
    status = ex_get_dimension(exoid, DIM_NUM_ELSET_VAR, "elemset variables", &num_var_db, &varid, routine);
    statust = nc_inq_varid (exoid, VAR_ELSET_TAB, &tabid);
    var_name = "vals_elset_var";
    ent_type = "es";
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
      "Error: Invalid variable type %d specified in file id %d",
      obj_type, exoid);
    ex_err(routine,errmsg,exerrval);
    return (EX_WARN);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    return (EX_WARN);
  }

  /* Determine index of entity_id in id array */
  ent_ndx = ex_id_lkup(exoid,obj_type,entity_id);
  if (exerrval != 0) {
    if (exerrval != EX_NULLENTITY) {
      sprintf(errmsg,
        "Error: failed to locate %s id %d in id variable in file id %d",
        ex_name_of_object(obj_type), entity_id, exoid);
      ex_err(routine,errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* If this is a null entity, then 'ent_ndx' will be negative.
   * We don't care in this routine, so make it positive and continue...
   */
  if (ent_ndx < 0) ent_ndx = -ent_ndx;
  
  if ((int)num_var_db != num_var) {
    exerrval = EX_FATAL;
    sprintf(errmsg,
      "Error: # of variables doesn't match those defined in file id %d", exoid);
    ex_err("ex_get_object_truth_vector",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (statust != NC_NOERR) {
    /* since truth vector isn't stored in the data file, derive it dynamically */
    for (i=0; i<num_var; i++) {
      /* NOTE: names are 1-based */
      if (nc_inq_varid(exoid, ex_catstr2(var_name, i+1, ent_type, ent_ndx), &tabid) != NC_NOERR) {
  /* variable doesn't exist; put a 0 in the truth vector */
  var_vec[i] = 0;
      } else {
  /* variable exists; put a 1 in the truth vector */
  var_vec[i] = 1;
      }
    }
  } else {

    /* read in the truth vector */

    start[0] = ent_ndx-1;
    start[1] = 0;

    count[0] = 1;
    count[1] = num_var;

    status = nc_get_vara_int(exoid, tabid, start, count, var_vec);
     
    if (status != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to get truth vector from file id %d", exoid);
      ex_err("ex_get_object_truth_vector",errmsg,exerrval);
      return (EX_FATAL);
    }
  } 
  return (EX_NOERR);
}

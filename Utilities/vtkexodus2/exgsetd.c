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
* exgssd - ex_get_set_dist_fact
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     set_type                type of set
*       int     set_id                  set id
*
* exit conditions - 
*       float*  set_dist_fact           array of dist factors for set
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * reads the distribution factors for a single set
 */

int ex_get_set_dist_fact (int   exoid,
			  ex_entity_type set_type,
			  int   set_id,
			  void *set_dist_fact)
{

  int dimid, dist_id, set_id_ndx;
  int status;
  char errmsg[MAX_ERR_LENGTH];
  char* factptr = NULL;
 
  exerrval = 0; /* clear error code */

  /* first check if any sets are specified */
  if ((status = nc_inq_dimid (exoid, ex_dim_num_objects(set_type), &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Warning: no %s sets stored in file id %d",
	    ex_name_of_object(set_type), exoid);
    ex_err("ex_get_set_dist_fact",errmsg,exerrval);
    return (EX_WARN);
  }

  /* Lookup index of set id in VAR_*S_IDS array */
  set_id_ndx = ex_id_lkup(exoid,set_type,set_id);
  if (exerrval != 0)  {
    if (exerrval == EX_NULLENTITY) {
      sprintf(errmsg,
              "Warning: %s set %d is NULL in file id %d",
	      ex_name_of_object(set_type), set_id,exoid);
      ex_err("ex_get_set_dist_fact",errmsg,EX_MSG);
      return (EX_WARN);
    } else {
      sprintf(errmsg,
	      "Error: failed to locate %s set id %d in VAR_*S_IDS array in file id %d",
	      ex_name_of_object(set_type), set_id,exoid);
      ex_err("ex_get_set_dist_fact",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* setup more pointers based on set_type */
  if (set_type == EX_NODE_SET) {
    factptr = VAR_FACT_NS(set_id_ndx);
  }
  else if (set_type == EX_EDGE_SET) {
    factptr = VAR_FACT_ES(set_id_ndx);
  }
  else if (set_type == EX_FACE_SET) {
    factptr = VAR_FACT_FS(set_id_ndx);
  }
  else if (set_type == EX_SIDE_SET) {
    factptr = VAR_FACT_SS(set_id_ndx);
  }
  if (set_type == EX_ELEM_SET) {
    factptr = VAR_FACT_ELS(set_id_ndx);
  }

  /* inquire id's of previously defined dimensions and variables */
  if ((status = nc_inq_varid(exoid, factptr, &dist_id)) != NC_NOERR) {
    exerrval = status;
    /* not an error for node sets because this is how we check that df's exist */
    if (set_type == EX_NODE_SET) {
      sprintf(errmsg,
	      "Warning: dist factors not stored for %s set %d in file id %d",
	      ex_name_of_object(set_type), set_id, exoid);
      ex_err("ex_get_set_dist_fact",errmsg,exerrval);
      return (EX_WARN);         /* complain - but not too loud */
    }
    /* is an error for other sets */
    else  {
      sprintf(errmsg,
	      "Error: failed to locate dist factors list for %s set %d in file id %d",
	      ex_name_of_object(set_type), set_id,exoid);
      ex_err("ex_get_set_dist_fact",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* read in the distribution factors array */
  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_var_float(exoid, dist_id, set_dist_fact);
  } else {
    status = nc_get_var_double(exoid, dist_id, set_dist_fact);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to get dist factors list for %s set %d in file id %d",
	    ex_name_of_object(set_type), set_id,exoid);
    ex_err("ex_get_set_dist_fact",errmsg,exerrval);
    return (EX_FATAL);
  }
  return (EX_NOERR);
}

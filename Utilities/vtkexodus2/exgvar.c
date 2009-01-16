/*
 * Copyright (c) 2006 Sandia Corporation. Under the terms of Contract
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
* exgev - ex_get_var
*
* entry conditions - 
*   input parameters:
*       int     exoid                exodus file id
*       int     time_step            time step number
*       ex_entity_type var_type             block/variable type
*                                      node, edge/face/element block, or
*                                      node/edge/face/side/element set
*       int     var_index            variable index
*       int     obj_id               object id
*       int     num_entry_this_obj   number of entries in this object
*
*
* exit conditions - 
*       float*  var_vals                array of element variable values
*
*
* revision history - 
*   20061002 - David Thompson - Adapted from ex_get_elem_var
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the values of a single element variable for one element block at 
 * one time step in the database; assume the first time step and
 * element variable index is 1
 */

int ex_get_var( int   exoid,
                int   time_step,
                ex_entity_type var_type,
                int   var_index,
                int   obj_id, 
                int   num_entry_this_obj,
                void* var_vals )
{
  int status;
  int varid, obj_id_ndx;
  size_t start[2], count[2];
  char errmsg[MAX_ERR_LENGTH];

  if (var_type == EX_NODAL) {
    /* FIXME: Special case: ignore obj_id, possible large_file complications, etc. */
    return ex_get_nodal_var( exoid, time_step, var_index, num_entry_this_obj, var_vals );
  } else if (var_type == EX_GLOBAL) {
    /* FIXME: Special case: all vars stored in 2-D single array. */
    return ex_get_glob_vars( exoid, time_step, num_entry_this_obj, var_vals );
  }
   
  exerrval = 0; /* clear error code */

  /* Determine index of obj_id in VAR_ID_EL_BLK array */
  obj_id_ndx = ex_id_lkup(exoid,var_type,obj_id);
  if (exerrval != 0) {
    if (exerrval == EX_NULLENTITY) {
      sprintf(errmsg,
	      "Warning: no %s variables for NULL block %d in file id %d",
	      ex_name_of_object(var_type), obj_id,exoid);
      ex_err("ex_get_var",errmsg,EX_MSG);
      return (EX_WARN);
    } else {
      sprintf(errmsg,
	      "Error: failed to locate %s id %d in id variable in file id %d",
	      ex_name_of_object(var_type), obj_id, exoid);
      ex_err("ex_get_var",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  /* inquire previously defined variable */

  if((status = nc_inq_varid(exoid, ex_name_var_of_object(var_type,var_index,
							 obj_id_ndx), &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to locate %s %d var %d in file id %d",
	    ex_name_of_object(var_type),obj_id,var_index,exoid); 
    ex_err("ex_get_var",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* read values of element variable */
  start[0] = --time_step;
  start[1] = 0;

  count[0] = 1;
  count[1] = num_entry_this_obj;

  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, var_vals);
  } else {
    status = nc_get_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to get %s %d variable %d in file id %d", 
	    ex_name_of_object(var_type), obj_id, var_index,exoid);
    ex_err("ex_get_var",errmsg,exerrval);
    return (EX_FATAL);
  }
  return (EX_NOERR);
}

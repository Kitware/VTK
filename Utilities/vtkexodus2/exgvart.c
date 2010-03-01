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
* exgvart - ex_get_var_time
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     var_type                variable type global, nodal,
*                                         edge/face/elem block,
*                                         node/edge/face/side/elem set
*       int     var_index               element variable index
*       int     id                      entry number
*       int     beg_time_step           time step number
*       int     end_time_step           time step number
*
* exit conditions - 
*       float*  var_vals                array of element variable values
*
* revision history - 
*   20061002 - David Thompson - Adapted from ex_get_var_time
*
* Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the values of a variable for a single entry of an object (block or set) through a 
 * specified number of time steps in the database; assume the first
 * variable index, entry number, and time step are 1
 */

int ex_get_var_time( int   exoid,
                     ex_entity_type var_type,
                     int   var_index,
                     int   id,
                     int   beg_time_step, 
                     int   end_time_step,
                     void* var_vals )
{
  int dimid, varid, numel = 0, offset;
  int status;
  int *obj_ids, *stat_vals;
  size_t num_obj, i;
  size_t num_entries_this_obj = 0;
  size_t start[2], count[2];
  float fdum;
  char *cdum;
  char errmsg[MAX_ERR_LENGTH];
  const char* varobjids;
  const char* varobstat;

  switch (var_type) {
  case EX_GLOBAL:
    return ex_get_glob_var_time( exoid, var_index, beg_time_step, end_time_step, var_vals );
  case EX_NODAL:
    return ex_get_nodal_var_time( exoid, var_index, id, beg_time_step, end_time_step, var_vals );
  case EX_EDGE_BLOCK:
    varobjids =   VAR_ID_ED_BLK; 
    varobstat = VAR_STAT_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    varobjids =   VAR_ID_FA_BLK; 
    varobstat = VAR_STAT_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    varobjids =   VAR_ID_EL_BLK; 
    varobstat = VAR_STAT_EL_BLK;
    break;
  case EX_NODE_SET:
    varobjids =      VAR_NS_IDS; 
    varobstat =      VAR_NS_STAT;
    break;
  case EX_EDGE_SET:
    varobjids =      VAR_ES_IDS;
    varobstat =      VAR_ES_STAT;
    break;
  case EX_FACE_SET:
    varobjids =      VAR_FS_IDS; 
    varobstat =      VAR_FS_STAT;
    break;
  case EX_SIDE_SET:
    varobjids =      VAR_SS_IDS; 
    varobstat =      VAR_SS_STAT;
    break;
  case EX_ELEM_SET:
    varobjids =      VAR_ELS_IDS; 
    varobstat =      VAR_ELS_STAT;
    break;
  default:
    exerrval = EX_BADPARAM;
    sprintf( errmsg, "Error: Invalid variable type (%d) specified for file id %d", var_type, exoid );
    ex_err( "ex_get_var_time", errmsg, exerrval );
    return (EX_FATAL);
  }

  exerrval = 0; /* clear error code */

  cdum = 0; /* initialize even though it is not used */

  /* assume entry number is 1-based (the first entry of an object is 1, not 0);
   * adjust so it is 0-based
   */
  id--;

  /* find what object the entry is in */

  /* first, find out how many objects there are */
  status = ex_get_dimension(exoid, ex_dim_num_objects(var_type), ex_name_of_object(var_type),
                            &num_obj, &dimid, "ex_get_var_time");
  if (status != NC_NOERR) return status;

  /* get the array of object ids */
  /* don't think we need this anymore since the netcdf variable names 
     associated with objects don't contain the object ids */

  if (!(obj_ids = malloc(num_obj*sizeof(int)))) {
    exerrval = EX_MEMFAIL;
    sprintf(errmsg,
            "Error: failed to allocate memory for %s ids for file id %d",
            ex_name_of_object(var_type),exoid);
    ex_err("ex_get_var_time",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_inq_varid (exoid, varobjids, &varid )) != NC_NOERR) {
    exerrval = status;
    free(obj_ids);
    sprintf(errmsg,
            "Error: failed to locate %s ids in file id %d",
            ex_name_of_object(var_type),exoid);
    ex_err("ex_get_var_time",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_get_var_int(exoid, varid, obj_ids)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get %s ids from file id %d",
            ex_name_of_object(var_type),exoid);
    ex_err("ex_get_var_time",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* allocate space for stat array */
  if (!(stat_vals = malloc((int)num_obj*sizeof(int)))) {
    exerrval = EX_MEMFAIL;
    free (obj_ids);
    sprintf(errmsg,
            "Error: failed to allocate memory for %s status array for file id %d",
            ex_name_of_object(var_type),exoid);
    ex_err("ex_get_var_time",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* get variable id of status array */
  if (nc_inq_varid (exoid, varobstat, &varid) == NC_NOERR) {
    /* if status array exists, use it, otherwise assume, object exists
       to be backward compatible */

    if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) {
      exerrval = status;
      free (obj_ids);
      free(stat_vals);
      sprintf(errmsg,
              "Error: failed to get %s status array from file id %d",
              ex_name_of_object(var_type),exoid);
      ex_err("ex_get_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }
  }
  else { /* default: status is true */
    for(i=0;i<num_obj;i++)
      stat_vals[i]=1;
  }

  /* loop through each object until id is found;  since entry
   * numbers are sequential (beginning with 1) id is in obj_i
   * when id_first_i <= id <= id_last_i, where
   * id_first_i is the entry number of the first entry in 
   * obj_i and id_last_i is the entry number of the last
   * entry in obj_i
   */

  i = 0;
  if (stat_vals[i] != 0)  {
    if ((status = nc_inq_dimid(exoid, ex_dim_num_entries_in_object(var_type,i+1), &dimid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to locate number of entries in %s %d in file id %d",
              ex_name_of_object(var_type), obj_ids[i], exoid);
      ex_err("ex_get_var_time",errmsg,exerrval);
      free(stat_vals);
      free(obj_ids);
      return (EX_FATAL);
    }

    if ((status = nc_inq_dimlen(exoid, dimid, &num_entries_this_obj)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get number of entries in %s %d in file id %d",
              ex_name_of_object(var_type), obj_ids[i], exoid);
      ex_err("ex_get_var_time",errmsg,exerrval);
      free(stat_vals);
      free(obj_ids);
      return (EX_FATAL);
    }
  } /* End NULL object check */

  numel = num_entries_this_obj;

  while (numel <= id) {
    if (stat_vals[++i] != 0) {
      if ((status = nc_inq_dimid(exoid,ex_dim_num_entries_in_object(var_type,i+1), &dimid)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to locate number of entries in %s %d in file id %d",
                ex_name_of_object(var_type), obj_ids[i], exoid);
        ex_err("ex_get_var_time",errmsg,exerrval);
        free(stat_vals);
        free(obj_ids);
        return (EX_FATAL);
      }

      if ((status = nc_inq_dimlen(exoid, dimid, &num_entries_this_obj)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to get number of entries in %s %d in file id %d",
                ex_name_of_object(var_type), obj_ids[i], exoid);
        ex_err("ex_get_var_time",errmsg,exerrval);
        free(stat_vals);
        free(obj_ids);
        return (EX_FATAL);
      }
      numel += num_entries_this_obj;
    }
  }
  offset = id - (numel - num_entries_this_obj);

  /* inquire previously defined variable */

  if ((status = nc_inq_varid(exoid,ex_name_var_of_object(var_type,var_index,i+1), &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate variable %d for %s %d in file id %d",
            var_index,ex_name_of_object(var_type),obj_ids[i],exoid);
    ex_err("ex_get_var_time",errmsg,exerrval);
    free(stat_vals);
    free(obj_ids);
    return (EX_FATAL);
  }

  free(stat_vals);
  free(obj_ids);

  /* read values of object variable */
  start[0] = --beg_time_step;
  start[1] = offset;

  if (end_time_step < 0) {
    /* user is requesting the maximum time step;  we find this out using the
     * database inquire function to get the number of time steps;  the ending
     * time step number is 1 less due to 0 based array indexing in C
     */
    if ((status = ex_inquire (exoid, EX_INQ_TIME, &end_time_step, &fdum, cdum)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get maximum time step in file id %d",
              exoid);
      ex_err("ex_get_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  end_time_step--;

  count[0] = end_time_step - beg_time_step + 1;
  count[1] = 1;

  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, var_vals);
  } else {
    status = nc_get_vara_double(exoid, varid, start, count, var_vals);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get %s variable values in file id %d",
            ex_name_of_object(var_type),exoid);
    ex_err("ex_get_var_time",errmsg,exerrval);
    return (EX_FATAL);
  }
  return (EX_NOERR);
}

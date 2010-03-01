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
* exgnvt - ex_get_nodal_var_time
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     nodal_var_index         index of desired nodal variable
*       int     node_number             number of desired node
*       int     beg_time_step           starting time step
*       int     end_time_step           finishing time step
*
* exit conditions - 
*       float*  nodal_var_vals          array of nodal variable values
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the values of a nodal variable for a single node through a 
 * specified number of time steps in the database; assume the first time
 * step, nodal variable index, and node are numbered 1
 */

int ex_get_nodal_var_time (int   exoid,
                           int   nodal_var_index,
                           int   node_number,
                           int   beg_time_step, 
                           int   end_time_step,
                           void *nodal_var_vals)
{
  int status;
  int varid;
  size_t start[3], count[3];
  float fdum;
  char *cdum = 0; 
  char errmsg[MAX_ERR_LENGTH];

  /* inquire previously defined variable */
  if (end_time_step < 0) {

    /* user is requesting the maximum time step;  we find this out using the
     * database inquire function to get the number of time steps;  the ending
     * time step number is 1 less due to 0 based array indexing in C
     */
    if ((status = ex_inquire (exoid, EX_INQ_TIME, &end_time_step, &fdum, cdum)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get number of time steps in file id %d",
              exoid);
      ex_err("ex_get_nodal_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  end_time_step--;

  if (ex_large_model(exoid) == 0) {
    /* read values of the nodal variable;
     * assume node number is 1-based (first node is numbered 1);  adjust
     * so it is 0-based
     */
    if ((status = nc_inq_varid(exoid, VAR_NOD_VAR, &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Warning: could not find nodal variable %d in file id %d",
              nodal_var_index, exoid);
      ex_err("ex_get_nodal_var",errmsg,exerrval);
      return (EX_WARN);
    }

    start[0] = --beg_time_step;
    start[1] = --nodal_var_index;
    start[2] = --node_number;

    count[0] = end_time_step - beg_time_step + 1;
    count[1] = 1;
    count[2] = 1;
  } else {
    if ((status = nc_inq_varid(exoid, VAR_NOD_VAR_NEW(nodal_var_index), &varid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Warning: could not find nodal variable %d in file id %d",
              nodal_var_index, exoid);
      ex_err("ex_get_nodal_var",errmsg,exerrval);
      return (EX_WARN);
    }

    /* read values of the nodal variable;
     * assume node number is 1-based (first node is numbered 1);  adjust
     * so it is 0-based
     */

    start[0] = --beg_time_step;
    start[1] = --node_number;

    count[0] = end_time_step - beg_time_step + 1;
    count[1] = 1;
  }

  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_vara_float(exoid, varid, start, count, nodal_var_vals);
  } else {
    status = nc_get_vara_double(exoid, varid, start, count, nodal_var_vals);
  }

  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get nodal variables in file id %d",
            exoid);
    ex_err("ex_get_nodal_var_time",errmsg,exerrval);
    return (EX_FATAL);
  }
  return (EX_NOERR);
}

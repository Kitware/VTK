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
* exgev - ex_get_varid_var
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid           exodus file id
*       int     time_step       time step number
*       int     varid           id of variable on exodus database
*       int     num_entity      number of entities for this variable
*       
*
* exit conditions - 
*       float*  var_vals        array of variable values
*
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the values of a single variable at one time step in the
 * database; assume the first time index is 1. Access based on the
 * passed in 'varid'
 */

/* NOTE: If used for nodal variables, it must be an ex_large_model == 1 */

int ex_get_varid_var(int   exoid,
                     int   time_step,
                     int   varid,
                     int   num_entity,
                     void *var_vals)
{
  long start[2], count[2];
  char errmsg[MAX_ERR_LENGTH];
  void *array;
  
  exerrval = 0; /* clear error code */

  /* read values of element variable */

  start[0] = --time_step;
  start[1] = 0;
   
  count[0] = 1;
  count[1] = num_entity;

  array = ex_conv_array(exoid,RTN_ADDRESS,var_vals,num_entity);
  if (ncvarget (exoid, varid, start, count, array) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to get variable with variable id %d in file id %d",
            varid,exoid);/*this msg needs to be improved*/
    ex_err("ex_get_varid_var",errmsg,exerrval);
    return (EX_FATAL);
  }

  if (array != var_vals) {
    ex_conv_array(exoid, READ_CONVERT, var_vals, num_entity);
  }
  return (EX_NOERR);
}

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
* exgatm - get all time values
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*       float*  time_values             array of simulation time values
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * reads the time values for all time steps. Memory must be allocated
 * for the time values array before this function is invoked. The
 * storage requirements (equal to the number of time steps) can be
 * determined by using the ex_inquire() routine.
 * \param       exoid        exodus file id
 * \param[out]  time_values  Returned array of time values at all time steps. 
 */

int ex_get_all_times (int   exoid,
                      void *time_values)
{
   int varid;
   int status;
   char errmsg[MAX_ERR_LENGTH];

  exerrval = 0;

  if ((status = nc_inq_varid(exoid, VAR_WHOLE_TIME, &varid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,"Error: failed to locate time variable %s in file id %d",
            VAR_WHOLE_TIME, exoid);
    ex_err("ex_get_all_times",errmsg,exerrval);
    return(EX_FATAL);
  }

  /*read time values */
  if (ex_comp_ws(exoid) == 4) {
    status = nc_get_var_float(exoid, varid, time_values);
  } else {
    status = nc_get_var_double(exoid, varid, time_values);
  }
    
  if (status != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
           "Error: failed to get time values from file id %d",
            exoid);
    ex_err("ex_get_all_times",errmsg,exerrval);
    return(EX_FATAL);
  }

  return (EX_NOERR);
}

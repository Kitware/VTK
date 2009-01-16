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
* exggvt - get_glob_var_time
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     glob_var_index          global variable index
*       int     beg_time_step           starting time step
*       int     end_time_step           ending time step
*
* exit conditions - 
*       float*  glob_var_vals           global variable values
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the values of a single global variable through a specified number 
 * of time steps in the database; time step numbers and variable indices
 * are assumed to start at 1
 */

int ex_get_glob_var_time (int   exoid,
                          int   glob_var_index,
                          int   beg_time_step,
                          int   end_time_step,
                          void *glob_var_vals)
{
   int status;
   int varid;
   size_t start[2], count[2];
   float fdum;
   char *cdum = 0;
   char  errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   /* inquire previously defined variable */
   if ((status = nc_inq_varid (exoid, VAR_GLO_VAR, &varid)) != NC_NOERR) {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to locate global variables in file id %d",
             exoid);
     ex_err("ex_get_glob_var_time",errmsg,exerrval);
     return (EX_WARN);
   }

   /* read values of global variables */
   start[0] = --beg_time_step;
   start[1] = --glob_var_index;

   if (end_time_step < 0) {
     /* user is requesting the maximum time step;  we find this out using the
      * database inquire function to get the number of time steps
      */
     if ((status = ex_inquire (exoid, EX_INQ_TIME, &end_time_step, &fdum, cdum)) != NC_NOERR) {
       exerrval = status;
       sprintf(errmsg,
             "Error: failed to get number of time steps in file id %d",
               exoid);
       ex_err("ex_get_glob_var_time",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

   end_time_step--;

   count[0] = end_time_step - beg_time_step + 1;
   count[1] = 1;

   if (ex_comp_ws(exoid) == 4) {
     status = nc_get_vara_float(exoid, varid, start, count, glob_var_vals);
   } else {
     status = nc_get_vara_double(exoid, varid, start, count, glob_var_vals);
   }

   if (status != NC_NOERR) {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to get global variable %d values from file id %d",
            glob_var_index, exoid);
     ex_err("ex_get_glob_var_time",errmsg,exerrval);
     return (EX_FATAL);
   }
   return (EX_NOERR);
}

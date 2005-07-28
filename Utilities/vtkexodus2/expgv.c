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
* expgv - ex_put_glo_vars
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     time_step               time step number
*       int     num_glob_vars           number of global vars in file
*       float*  glob_var_vals           array of global variable values
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * writes the values of all the global variables for a single time step to 
 * the database; time step numbers are assumed to start at 1
 */

int ex_put_glob_vars (int   exoid,
                      int   time_step,
                      int   num_glob_vars,
                const void *glob_var_vals)
{
   int varid;
   long start[2], count[2];
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* if no variables are to be stored, return with warning */
   if (num_glob_vars == 0)
   {
     exerrval = EX_MSG;
     sprintf(errmsg,
            "Warning: no global variables specified for file id %d",
             exoid);
     ex_err("ex_put_glob_vars",errmsg,exerrval);

     return (EX_WARN);
   }


/* inquire previously defined variable */

   if ((varid = ncvarid (exoid, VAR_GLO_VAR)) == -1)
   {
     if (ncerr == NC_ENOTVAR)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: no global variables defined in file id %d",
               exoid);
       ex_err("ex_put_glob_vars",errmsg,exerrval);
     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to get global variables parameters in file id %d",
               exoid);
       ex_err("ex_put_glob_vars",errmsg,exerrval);
     }
     return (EX_FATAL);
   } 


/* write values of global variables */

   start[0] = --time_step;
   start[1] = 0;

   count[0] = 1;
   count[1] = num_glob_vars;

   if (ncvarput (exoid, varid, start, count,
         ex_conv_array(exoid,WRITE_CONVERT,glob_var_vals,num_glob_vars)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store global variables in file id %d", 
             exoid);
     ex_err("ex_put_glob_vars",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);
}

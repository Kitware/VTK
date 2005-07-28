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
* exgvp - ex_get_var_param
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
*       char*   var_type                variable type G,N, or E
*
* exit conditions - 
*       int*    num_vars                number of variables in database
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the number of global, nodal, or element variables that are 
 * stored in the database
 */

int ex_get_var_param (int   exoid,
                      const char *var_type,
                      int  *num_vars)
{
   int dimid;
   long lnum_vars;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   if (*var_type == 'g' || *var_type == 'G')
   {
     if ((dimid = ncdimid (exoid, DIM_NUM_GLO_VAR)) == -1) 
     {
       *num_vars = 0;
       if (ncerr == NC_EBADDIM)
         return(EX_NOERR);      /* no global variables defined */
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: failed to locate global variable names in file id %d",
                 exoid);
         ex_err("ex_get_var_param",errmsg,exerrval);
         return (EX_FATAL);
       }
     }

     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_vars) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to get number of global variables in file id %d",
               exoid);
       ex_err("ex_get_var_param",errmsg,exerrval);
       return (EX_FATAL);
     }
     *num_vars = lnum_vars;

   }

   else if (*var_type == 'n' || *var_type == 'N')
   {
     if ((dimid = ncdimid (exoid, DIM_NUM_NOD_VAR)) == -1)
     {
       *num_vars = 0;
       if (ncerr == NC_EBADDIM)
          return(EX_NOERR);     /* no nodal variables defined */
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: failed to locate nodal variable names in file id %d",
                 exoid);
         ex_err("ex_get_var_param",errmsg,exerrval);
         return (EX_FATAL);
       }

     }
     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_vars) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to get number of nodal variables in file id %d",
               exoid);
       ex_err("ex_get_var_param",errmsg,exerrval);
       return (EX_FATAL);
     }
     *num_vars = lnum_vars;

   }

   else if (*var_type == 'e' || *var_type == 'E')
   {
     if ((dimid = ncdimid (exoid, DIM_NUM_ELE_VAR)) == -1)
     {
       *num_vars = 0;
       if (ncerr == NC_EBADDIM)
         return(EX_NOERR);      /* no nodal variables defined */
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: failed to locate element variable names in file id %d",
                 exoid);
         ex_err("ex_get_var_param",errmsg,exerrval);
         return (EX_FATAL);
       }
     }

     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_vars) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to get number of element variables in file id %d",
               exoid);
       ex_err("ex_get_var_param",errmsg,exerrval);
       return (EX_FATAL);
     }
     *num_vars = lnum_vars;
   }

   else /* invalid variable type */
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Warning: invalid variable type %c requested from file id %d",
            *var_type, exoid);
     ex_err("ex_get_var_param",errmsg,exerrval);
     return (EX_WARN);
   }

   return(EX_NOERR);
}

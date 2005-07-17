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
* expvan - ex_put_var_names
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
*       char*   var_type                variable type: G,N, or E
*       int     num_vars                # of variables to read
*       char*   var_names               ptr array of variable names
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
#include <string.h>

/*
 * writes the names of the results variables to the database
 */

int ex_put_var_names (int   exoid,
                      const char *var_type,
                      int   num_vars,
                      char *var_names[])
{
   int i, varid; 
   long  start[2], count[2];
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   if (*var_type == 'g' || *var_type == 'G')
   {
     if ((ncdimid (exoid, DIM_NUM_GLO_VAR)) == -1)
     {
       if (ncerr == NC_EBADDIM)
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: no global variables defined in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
             "Error: failed to locate number of global variables in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       return(EX_FATAL); 
     }

     if ((varid = ncvarid (exoid, VAR_NAME_GLO_VAR)) == -1)
     {
       if (ncerr == NC_ENOTVAR)
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: no global variable names defined in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: global name variable names not found in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       return(EX_FATAL);
     }

   }

   else if (*var_type == 'n' || *var_type == 'N')
   {
     if ((ncdimid (exoid, DIM_NUM_NOD_VAR)) == -1)
     {
       if (ncerr == NC_EBADDIM)
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: no nodal variables defined in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
              "Error: failed to locate number of nodal variables in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       return(EX_FATAL);
     }

     if ((varid = ncvarid (exoid, VAR_NAME_NOD_VAR)) == -1)
     {
       if (ncerr == NC_ENOTVAR)
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: no nodal variable names defined in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: nodal name variable names not found in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       return(EX_FATAL);
     }
   }

   else if (*var_type == 'e' || *var_type == 'E')
   {
     if ((ncdimid (exoid, DIM_NUM_ELE_VAR)) == -1)
     {
       if (ncerr == NC_EBADDIM)
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: no element variables defined in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
            "Error: failed to locate number of element variables in file id %d",
                exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       return(EX_FATAL);
     }
     if ((varid = ncvarid (exoid, VAR_NAME_ELE_VAR)) == -1)
     {
       if (ncerr == NC_ENOTVAR)
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: no element variable names defined in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
                "Error: element name variable names not found in file id %d",
                 exoid);
         ex_err("ex_put_var_names",errmsg,exerrval);
       }
       return(EX_FATAL);

     }
   }

   else /* invalid variable type */
   {
     exerrval = EX_BADPARAM;
     sprintf(errmsg,
            "Error: Invalid variable type %c specified in file id %d",
             *var_type, exoid);
     ex_err("ex_put_var_names",errmsg,exerrval);
     return(EX_FATAL);
   }


/* write EXODUS variable names */

   for (i=0; i<num_vars; i++)
   {
     start[0] = i;
     start[1] = 0;

     count[0] = 1;
     count[1] = strlen(var_names[i]) + 1;

     if (ncvarput (exoid, varid, start, count, (void*) var_names[i]) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
               "Error: failed to store variable names in file id %d",
                exoid);
       ex_err("ex_put_var_names",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

   return(EX_NOERR);

}

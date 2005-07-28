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
* expcon - ex_put_coord_names
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
*       char*   coord_names             ptr array of coordinate names
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
 * writes the names of the coordinate arrays to the database
 */

int ex_put_coord_names (int   exoid,
                        char *coord_names[])
{
   int i, ndimdim, varid;
   long num_dim, start[2], count[2];
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* inquire previously defined dimensions  */

   if ((ndimdim = ncdimid (exoid, DIM_NUM_DIM)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
          "Error: failed to locate number of dimensions in file id %d",
             exoid);
     ex_err("ex_put_coord_names",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, ndimdim, (char *) 0, &num_dim) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: inquire failed to get number of dimensions in file id %d",
             exoid);
     ex_err("ex_put_coord_names",errmsg,exerrval);
     return (EX_FATAL);
   }

   if ((varid = ncvarid (exoid, VAR_NAME_COOR)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to locate coordinate names in file id %d",
             exoid);
     ex_err("ex_put_coord_names",errmsg,exerrval);
     return (EX_FATAL);
   }


/* write out coordinate names */

   for (i=0; i<num_dim; i++)
   {
     start[0] = i;
     start[1] = 0;

     count[0] = 1;
     count[1] = strlen(coord_names[i]) + 1;

     if (ncvarput (exoid, varid, start, count, (void*) coord_names[i]) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to store coordinate name %d in file id %d",
               i,exoid);
       ex_err("ex_put_coord_names",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

   return (EX_NOERR);

}

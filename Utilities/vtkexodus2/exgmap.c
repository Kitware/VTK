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
* exgmap - ex_get_map
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*       int*    elem_map                element order map array
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 *  reads the element order map from the database
 * \deprecated Use ex_get_num_map() instead.
 */

int ex_get_map (int  exoid,
                int *elem_map)
{
   int numelemdim, mapid, status;
   size_t i, num_elem;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* inquire id's of previously defined dimensions and variables  */

   /* See if file contains any elements...*/
   if ((status = nc_inq_dimid (exoid, DIM_NUM_ELEM, &numelemdim)) != NC_NOERR) {
     return (EX_NOERR);
   }

   if ((status = nc_inq_dimlen(exoid, numelemdim, &num_elem)) != NC_NOERR) {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to get number of elements in file id %d",
             exoid);
     ex_err("ex_get_map",errmsg,exerrval);
     return (EX_FATAL);
   }


   if (nc_inq_varid(exoid, VAR_MAP, &mapid) != NC_NOERR) {
     /* generate default map of 1..n, where n is num_elem */
     for (i=0; i<num_elem; i++)
       elem_map[i] = i+1;
     
     return (EX_NOERR);
   }

   /* read in the element order map  */
   status = nc_get_var_int(exoid, mapid, elem_map);

   if (status != NC_NOERR) {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to get element order map in file id %d",
             exoid);
     ex_err("ex_get_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   return(EX_NOERR);

}

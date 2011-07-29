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

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
\deprecated Use ex_get_num_map() instead.

The function ex_get_map() reads the element order mapelement order map
from the database. See #ElementOrderMap for a description of the
element order map. If an element order map is not stored in the data
file, a default array (1,2,3,. .. \c num_elem) is returned. Memory
must be allocated for the element map array ({num_elem} in length)
before this call is made.

\return In case of an error, ex_get_map() returns a negative number; a
warning will return a positive number. Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  if an element order map is not stored, a default map and a
     warning value are returned.

\param[in]  exoid      exodus file ID returned from a previous call to ex_create() or ex_open().
\param[out] elem_map   Returned element order map.

The following code will read an element order map from an open exodus
file :

\code
int *elem_map, error, exoid;

\comment{read element order map}
elem_map = (int *)calloc(num_elem, sizeof(int));

error = ex_get_map(exoid, elem_map);
\endcode

 */

int ex_get_map (int  exoid,
                int *elem_map)
{
   int numelemdim, mapid, status;
   size_t num_elem, i;
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

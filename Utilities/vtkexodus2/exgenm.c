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
* exgenm - ex_get_elem_num_map
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
*
* exit conditions - 
*       int*    elem_map                element number map array
*
* revision history - 
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 *  reads the element numbering map from the database; allows element numbers
 *  to be noncontiguous
 */

int ex_get_elem_num_map (int  exoid,
                int *elem_map)
{
   int numelemdim, mapid, i, iresult;
   long num_elem,  start[1], count[1]; 
   nclong *longs;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* inquire id's of previously defined dimensions and variables  */

   /* See if file contains any elements...*/
   if ((numelemdim = ncdimid (exoid, DIM_NUM_ELEM)) == -1)
   {
     return(EX_NOERR);
   }

   if (ncdiminq (exoid, numelemdim, (char *) 0, &num_elem) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of elements in file id %d",
             exoid);
     ex_err("ex_get_elem_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }


   if ((mapid = ncvarid (exoid, VAR_ELEM_NUM_MAP)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
"Warning: elem numbering map not stored in file id %d; returning default map",
             exoid);
     ex_err("ex_get_elem_num_map",errmsg,exerrval);

/* generate default map of 1..n, where n is num_elem */
     for (i=0; i<num_elem; i++)
        elem_map[i] = i+1;

     return (EX_WARN);
   }


/* read in the element numbering map  */

/* application code has allocated an array of ints but netcdf is expecting
   a pointer to nclongs;  if ints are different sizes than nclongs,
   we must allocate an array of nclongs then convert them to ints with ltoi */

   start[0] = 0;
   count[0] = num_elem;

   if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarget (exoid, mapid, start, count, elem_map);
   } else {
     if (!(longs = malloc(num_elem * sizeof(nclong)))) {
       exerrval = EX_MEMFAIL;
       sprintf(errmsg,
               "Error: failed to allocate memory for element number map for file id %d",
               exoid);
       ex_err("ex_get_elem_num_map",errmsg,exerrval);
       return (EX_FATAL);
     }
     iresult = ncvarget (exoid, mapid, start, count, longs);
   }

   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get element number map in file id %d",
             exoid);
     ex_err("ex_get_elem_num_map",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (sizeof(int) != sizeof(nclong)) {
      ltoi (longs, elem_map, num_elem);
      free (longs);
   }

   return(EX_NOERR);

}

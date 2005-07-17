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
* exgelc - exodusII read element block connectivity
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
* environment - UNIX
*
* entry conditions - 
*   expelb must be called first to establish element block parameters.
*   input parameters:
*       int     exoid           exodus file id
*       int     elem_blk_id     element block id
*
* exit conditions - 
*       int*    connect         connectivity array
*
* revision history - 
*
* Id
*/
#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the connectivity array for an element block
 */

int ex_get_elem_conn (int   exoid,
                      int   elem_blk_id,
                      int  *connect)
{
   int numelbdim, nelnoddim, connid, elem_blk_id_ndx, iresult;
   long num_elem_this_blk, num_nod_per_elem,  start[2], count[2]; 
   nclong *longs;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* Locate index of element block id in VAR_ID_EL_BLK array */

   elem_blk_id_ndx = ex_id_lkup(exoid,VAR_ID_EL_BLK,elem_blk_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)
     {
       sprintf(errmsg,
              "Warning: no connectivity array for NULL block %d in file id %d",
               elem_blk_id,exoid);
       ex_err("ex_get_elem_conn",errmsg,EX_MSG);
       return (EX_WARN); /* no connectivity array for this element block */
     }
     else
     {
       sprintf(errmsg,
        "Error: failed to locate element block id %d in %s array in file id %d",
               elem_blk_id,VAR_ID_EL_BLK,exoid);
       ex_err("ex_get_elem_conn",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

/* inquire id's of previously defined dimensions  */

   if ((numelbdim = ncdimid (exoid, DIM_NUM_EL_IN_BLK(elem_blk_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
     "Error: failed to locate number of elements in block %d in file id %d",
              elem_blk_id,exoid);
     ex_err("ex_get_elem_conn",errmsg, exerrval);
     return(EX_FATAL);
   }

   if (ncdiminq (exoid, numelbdim, (char *) 0, &num_elem_this_blk) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to get number of elements in block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_get_elem_conn",errmsg,exerrval);
     return(EX_FATAL);
   }


   if ((nelnoddim = ncdimid (exoid, DIM_NUM_NOD_PER_EL(elem_blk_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
      "Error: failed to locate number of nodes/elem for block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_get_elem_conn",errmsg,exerrval);
     return(EX_FATAL);
   }

   if (ncdiminq (exoid, nelnoddim, (char *) 0, &num_nod_per_elem) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to get number of nodes/elem for block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_get_elem_conn",errmsg, exerrval);
     return(EX_FATAL);
   }


   if ((connid = ncvarid (exoid, VAR_CONN(elem_blk_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
        "Error: failed to locate connectivity array for block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_get_elem_conn",errmsg, exerrval);
     return(EX_FATAL);
   }


/* read in the connectivity array */

/* application code has allocated an array of ints but netcdf is expecting
   a pointer to nclongs;  if ints are different sizes than nclongs,
   we must allocate an array of nclongs then convert them to ints with ltoi */

   start[0] = 0;
   start[1] = 0;

   count[0] = num_elem_this_blk;
   count[1] = num_nod_per_elem;

   if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarget (exoid, connid, start, count, connect);
   } else {
     if (!(longs = malloc (num_elem_this_blk*num_nod_per_elem * sizeof(nclong)))) {
       exerrval = EX_MEMFAIL;
       sprintf(errmsg,
               "Error: failed to allocate memory for element connectivity array for file id %d",
               exoid);
       ex_err("ex_get_elem_conn",errmsg,exerrval);
       return (EX_FATAL);
     }
     iresult = ncvarget (exoid, connid, start, count, longs);
   }
   
   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to get connectivity array for block %d in file id %d",
             elem_blk_id,exoid);
     ex_err("ex_get_elem_conn",errmsg, exerrval);
     return(EX_FATAL);
   }

   if (sizeof(int) != sizeof(nclong)) {
      ltoi (longs, connect, num_elem_this_blk*num_nod_per_elem);
      free (longs);
   }

   return (EX_NOERR);

}

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
* expss - ex_put_side_set
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
*       int     side_set_id             side set id
*       int*    side_set_elem_list      array of elements in side set
*       int*    side_set_side_list      array of sides in side set

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
 * writes the side set element list and side set side list for a single side set
 */

int ex_put_side_set (int   exoid,
                     int   side_set_id,
                     const int  *side_set_elem_list,
                     const int  *side_set_side_list)
{
   int dimid, iresult;
   int elem_list_id, side_list_id, side_set_id_ndx;
   long  num_side_in_set, start[1], count[1]; 
   nclong *lptr;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* first check if any side sets are specified */

   if ((dimid = ncdimid (exoid, DIM_NUM_SS)) < 0)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: no side sets defined in file id %d",
             exoid);
     ex_err("ex_put_side_set",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Lookup index of side set id in VAR_SS_IDS array */

   side_set_id_ndx = ex_id_lkup(exoid,VAR_SS_IDS,side_set_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)
     {
       sprintf(errmsg,
              "Warning: no data allowed for NULL side set %d in file id %d",
               side_set_id,exoid);
       ex_err("ex_put_side_set",errmsg,EX_MSG);
       return (EX_WARN);
     }
     else
     {
       sprintf(errmsg,
     "Error: failed to locate side set id %d in VAR_SS_IDS array in file id %d",
               side_set_id,exoid);
       ex_err("ex_put_side_set",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

/* inquire id's of previously defined dimensions  */

   if ((dimid = ncdimid (exoid, DIM_NUM_SIDE_SS(side_set_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
      "Error: failed to locate number of sides in side set %d in file id %d",
             side_set_id,exoid);
     ex_err("ex_put_side_set",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &num_side_in_set) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
         "Error: failed to get number of sides in side set %d in file id %d",
             side_set_id,exoid);
     ex_err("ex_put_side_set",errmsg,exerrval);
     return (EX_FATAL);
   }

/* inquire id's of previously defined variables  */

   if ((elem_list_id = ncvarid (exoid, VAR_ELEM_SS(side_set_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to locate element list for side set %d in file id %d",
               side_set_id,exoid);
     ex_err("ex_put_side_set",errmsg,exerrval);
     return (EX_FATAL);
   }

   if ((side_list_id = ncvarid (exoid, VAR_SIDE_SS(side_set_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
              "Error: failed to locate side list for side set %d in file id %d",
               side_set_id,exoid);
     ex_err("ex_put_side_set",errmsg,exerrval);
     return (EX_FATAL);
   }


/* write out the element list and side list arrays */

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

   start[0] = 0;
   count[0] = num_side_in_set;

   if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarput(exoid, elem_list_id, start, count, side_set_elem_list);
   } else {
      lptr = itol (side_set_elem_list, (int)num_side_in_set);
      iresult = ncvarput (exoid, elem_list_id, start, count, lptr);
      free(lptr);
   }

   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
      "Error: failed to store element list for side set %d in file id %d",
             side_set_id,exoid);
     ex_err("ex_put_side_set",errmsg,exerrval);
     return (EX_FATAL);
   }


/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

   if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarput(exoid, side_list_id, start, count, side_set_side_list);
   } else {
      lptr = itol (side_set_side_list, (int)num_side_in_set);
      iresult = ncvarput (exoid, side_list_id, start, count, lptr);
      free(lptr);
   }

   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
      "Error: failed to store side list for side set %d in file id %d",
             side_set_id,exoid);
     ex_err("ex_put_side_set",errmsg,exerrval);
     return (EX_FATAL);
   }


   return (EX_NOERR);

}

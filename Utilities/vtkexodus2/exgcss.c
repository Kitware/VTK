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
* exgcss - ex_get_concat_side_sets
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
*       int     *side_set_ids           array of side set ids
*       int     *num_elem_per_set       number of elements/sides/faces  per set
*       int     *num_dist_per_set       number of distribution factors per set
*       int     *side_sets_elem_index   index array of elements into elem list
*       int     *side_sets_dist_index   index array of df into df list
*       int     *side_sets_elem_list    array of elements
*       int     *side_sets_side_list    array of sides
*       void    *side_sets_dist_fact    array of distribution factors
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the side set ID's, side set element count array, side set node count 
 * array, side set element pointers array, side set node pointers array, side 
 * set element list, side set node list, and side set distribution factors
 */

int ex_get_concat_side_sets (int   exoid,
                             int  *side_set_ids,
                             int  *num_elem_per_set,
                             int  *num_dist_per_set,
                             int  *side_sets_elem_index,
                             int  *side_sets_dist_index,
                             int  *side_sets_elem_list,
                             int  *side_sets_side_list,
                             void *side_sets_dist_fact)
{
   char *cdum;
   int num_side_sets, i;
   float fdum;
   float  *flt_dist_fact;
   double *dbl_dist_fact;
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   cdum = 0; /* initialize even though it is not used */

/* first check if any side sets are specified */

   if (ncdimid (exoid, DIM_NUM_SS)  == -1)
   {
     if (ncerr == NC_EBADDIM)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Warning: no side sets defined for file id %d", exoid);
       ex_err("ex_get_concat_side_sets",errmsg,exerrval);
       return (EX_WARN);
     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to locate side sets defined in file id %d", exoid);
       ex_err("ex_get_concat_side_sets",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

/* inquire how many side sets have been stored */

   if (ex_inquire(exoid, EX_INQ_SIDE_SETS, &num_side_sets, &fdum, cdum) == -1)
   {
     sprintf(errmsg,
            "Error: failed to get number of side sets defined for file id %d",
             exoid);
     /* use error val from inquire */
     ex_err("ex_get_concat_side_sets",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ex_get_side_set_ids (exoid, side_set_ids) == -1)
   {
     sprintf(errmsg,
            "Error: failed to get side set ids for file id %d",
             exoid);
     /* use error val from inquire */
     ex_err("ex_get_concat_side_sets",errmsg,exerrval);
     return (EX_FATAL);
   }

   side_sets_elem_index[0] = 0;
   side_sets_dist_index[0] = 0;

   for (i=0; i<num_side_sets; i++)
   {
     if (ex_get_side_set_param(exoid, side_set_ids[i], 
                       &(num_elem_per_set[i]), &(num_dist_per_set[i])) == -1)
       return(EX_FATAL); /* error will be reported by sub */

     if (i < num_side_sets-1)
     {
       /* fill in element and dist factor index arrays */
       side_sets_elem_index[i+1] = side_sets_elem_index[i]+num_elem_per_set[i];
       side_sets_dist_index[i+1] = side_sets_dist_index[i]+num_dist_per_set[i];
     }

     if (num_elem_per_set[i] == 0) /* NULL side set? */
       continue;

     /* Now, use ExodusII call to get side sets */


     if (ex_comp_ws(exoid) == sizeof(float))
     {
       if (ex_get_side_set(exoid, side_set_ids[i],
                         &(side_sets_elem_list[side_sets_elem_index[i]]),
                         &(side_sets_side_list[side_sets_elem_index[i]])) == -1)
         return(EX_FATAL); /* error will be reported by subroutine */

       /* get distribution factors for this set */
       flt_dist_fact = side_sets_dist_fact;
       if (num_dist_per_set[i] > 0)       /* only get df if they exist */
       {
         if (ex_get_side_set_dist_fact(exoid, side_set_ids[i],
                               &(flt_dist_fact[side_sets_dist_index[i]])) == -1)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                  "Error: failed to get node set %d dist factors in file id %d",
                   side_set_ids[i], exoid);
           ex_err("ex_get_concat_side_sets",errmsg,exerrval);
           return(EX_FATAL);
         }
       } else {  /* fill distribution factor array with 1's */
       }
     }
     else if (ex_comp_ws(exoid) == sizeof(double))
     {
       if (ex_get_side_set(exoid, side_set_ids[i],
                         &(side_sets_elem_list[side_sets_elem_index[i]]),
                         &(side_sets_side_list[side_sets_elem_index[i]])) == -1)
         return(EX_FATAL); /* error will be reported by subroutine */

       /* get distribution factors for this set */
       dbl_dist_fact = side_sets_dist_fact;
       if (num_dist_per_set[i] > 0)       /* only get df if they exist */
       {
         if (ex_get_side_set_dist_fact(exoid, side_set_ids[i],
                               &(dbl_dist_fact[side_sets_dist_index[i]])) == -1)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                  "Error: failed to get node set %d dist factors in file id %d",
                     side_set_ids[i], exoid);
           ex_err("ex_get_concat_side_sets",errmsg,exerrval);
           return(EX_FATAL);
         }
       } else {  /* fill distribution factor array with 1's */
       }
     }
   }

   return(EX_NOERR);

}

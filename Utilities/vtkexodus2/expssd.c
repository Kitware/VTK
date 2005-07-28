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
* expssd - ex_put_side_set_dist_fact
*
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     side_set_id             side set id
*       void*   side_set_dist_fact      array of dist factors for side set

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
 * writes the distribution factors for a single side set
 */

int ex_put_side_set_dist_fact (int   exoid,
                               int   side_set_id,
                               const void *side_set_dist_fact)
{
   int dimid, side_set_id_ndx;
   int dist_id;
   long num_df_in_set,  start[1], count[1];
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

/* first check if any side sets are specified */

   if ((dimid = ncdimid (exoid, DIM_NUM_SS)) < 0)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: no side sets specified in file id %d",
             exoid);
     ex_err("ex_put_side_set_dist_fact",errmsg,exerrval);
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
       ex_err("ex_put_side_set_fact",errmsg,EX_MSG);
       return (EX_WARN);
     }
     else
     {
      sprintf(errmsg,
     "Error: failed to locate side set id %d in VAR_SS_IDS array in file id %d",
               side_set_id,exoid);
       ex_err("ex_put_side_set_dist_fact",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

/* inquire id's of previously defined dimension and variable */

   if ((dimid = ncdimid (exoid, DIM_NUM_DF_SS(side_set_id_ndx))) == -1)
   {
     if (ncerr == NC_EBADDIM)
     {
       exerrval = EX_BADPARAM;
       sprintf(errmsg,
              "Warning: no dist factors defined for side set %d in file id %d",
               side_set_id,exoid);
       ex_err("ex_put_side_set_dist_fact",errmsg,exerrval);
       return (EX_WARN);

     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
  "Error: failed to locate number of dist factors in side set %d in file id %d",
               side_set_id,exoid);
       ex_err("ex_put_side_set_dist_fact",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &num_df_in_set) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
     "Error: failed to get number of dist factors in side set %d in file id %d",
             side_set_id,exoid);
     ex_err("ex_put_side_set_dist_fact",errmsg,exerrval);
     return (EX_FATAL);
   }


   if ((dist_id = ncvarid (exoid, VAR_FACT_SS(side_set_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
      "Error: failed to locate dist factors list for side set %d in file id %d",
             side_set_id,exoid);
     ex_err("ex_put_side_set_dist_fact",errmsg,exerrval);
     return (EX_FATAL);
   }


/* write out the distribution factors array */

   start[0] = 0;

   count[0] = num_df_in_set;

   if (ncvarput (exoid, dist_id, start, count,
             ex_conv_array(exoid,WRITE_CONVERT,side_set_dist_fact,
                           (int)num_df_in_set)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store dist factors for side set %d in file id %d",
             side_set_id,exoid);
     ex_err("ex_put_side_set_dist_fact",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);

}

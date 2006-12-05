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
* expssd - ex_put_set_dist_fact
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
*       int     set_type                set type
*       int     set_id                  set id
*       void*   set_dist_fact           array of dist factors for set

* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the distribution factors for a single set
 */

int ex_put_set_dist_fact (int   exoid,
        int   set_type,
        int   set_id,
        const void *set_dist_fact)
{
   int dimid, set_id_ndx;
   int dist_id;
   long num_df_in_set,  start[1], count[1];
   char errmsg[MAX_ERR_LENGTH];
   char* typeName;
   char* dimptr;
   char* idsptr;
   char* numdfptr = 0;
   char* factptr = 0;

   exerrval = 0; /* clear error code */

   /* setup pointers based on set_type 
    NOTE: there is another block that sets more stuff later ... */
   if (set_type == EX_NODE_SET) {
     typeName = "node";
     dimptr = DIM_NUM_NS;
     idsptr = VAR_NS_IDS;
   }
   else if (set_type == EX_EDGE_SET) {
     typeName = "edge";
     dimptr = DIM_NUM_ES;
     idsptr = VAR_ES_IDS;
   }
   else if (set_type == EX_FACE_SET) {
     typeName = "face";
     dimptr = DIM_NUM_FS;
     idsptr = VAR_FS_IDS;
   }
   else if (set_type == EX_SIDE_SET) {
     typeName = "side";
     dimptr = DIM_NUM_SS;
     idsptr = VAR_SS_IDS;
   }
   else if (set_type == EX_ELEM_SET) {
     typeName = "elem";
     dimptr = DIM_NUM_ELS;
     idsptr = VAR_ELS_IDS;
   }
   else {
     exerrval = EX_FATAL;
     sprintf(errmsg,
       "Error: invalid set type (%d)", set_type);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

/* first check if any sets are specified */

   if ((dimid = ncdimid (exoid, dimptr)) < 0)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: no %s sets specified in file id %d",
             typeName, exoid);
     ex_err("ex_put_set_dist_fact",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Lookup index of set id in VAR_*S_IDS array */

   set_id_ndx = ex_id_lkup(exoid,idsptr,set_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)
     {
       sprintf(errmsg,
              "Warning: no data allowed for NULL %s set %d in file id %d",
               typeName, set_id,exoid);
       ex_err("ex_put_set_fact",errmsg,EX_MSG);
       return (EX_WARN);
     }
     else
     {
      sprintf(errmsg,
     "Error: failed to locate %s set id %d in VAR_*S_IDS array in file id %d",
        typeName, set_id,exoid);
       ex_err("ex_put_set_dist_fact",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

  /* setup more pointers based on set_type */
   if (set_type == EX_NODE_SET) {
     /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
     numdfptr = DIM_NUM_NOD_NS(set_id_ndx);
     factptr = VAR_FACT_NS(set_id_ndx);
   }
   else if (set_type == EX_EDGE_SET) {
     numdfptr = DIM_NUM_DF_ES(set_id_ndx);
     factptr = VAR_FACT_ES(set_id_ndx);
   }
   else if (set_type == EX_FACE_SET) {
     numdfptr = DIM_NUM_DF_FS(set_id_ndx);
     factptr = VAR_FACT_FS(set_id_ndx);
   }
   else if (set_type == EX_SIDE_SET) {
     numdfptr = DIM_NUM_DF_SS(set_id_ndx);
     factptr = VAR_FACT_SS(set_id_ndx);
   }
   if (set_type == EX_ELEM_SET) {
     numdfptr = DIM_NUM_DF_ELS(set_id_ndx);
     factptr = VAR_FACT_ELS(set_id_ndx);
   }

/* inquire id's of previously defined dimension and variable */

   if ((dimid = ncdimid (exoid, numdfptr)) == -1)
   {
     if (ncerr == NC_EBADDIM)
     {
       exerrval = EX_BADPARAM;
       sprintf(errmsg,
              "Warning: no dist factors defined for %s set %d in file id %d",
               typeName, set_id,exoid);
       ex_err("ex_put_set_dist_fact",errmsg,exerrval);
       return (EX_WARN);

     }
     else
     {
       exerrval = ncerr;
       sprintf(errmsg,
  "Error: failed to locate number of dist factors in %s set %d in file id %d",
               typeName, set_id,exoid);
       ex_err("ex_put_set_dist_fact",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &num_df_in_set) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
     "Error: failed to get number of dist factors in %s set %d in file id %d",
             typeName, set_id,exoid);
     ex_err("ex_put_set_dist_fact",errmsg,exerrval);
     return (EX_FATAL);
   }

/* find id of distribution factors variable
 */

   if ((dist_id = ncvarid (exoid, factptr)) == -1)
   {
     /* this test is only needed for node set because we're using
  DIM_NUM_NOD_NS instead of  DIM_NUM_DF_NS*/
     if (ncerr == NC_ENOTVAR)
       {
   exerrval = EX_BADPARAM;
   sprintf(errmsg,
     "Warning: no dist factors defined for %s set %d in file id %d",
     typeName, set_id, exoid);
   ex_err("ex_put_set_dist_fact",errmsg,exerrval);
   return (EX_WARN);
       }
     else 
       {
   exerrval = ncerr;
   sprintf(errmsg,
     "Error: failed to locate dist factors list for %s set %d in file id %d",
     typeName, set_id,exoid);
   ex_err("ex_put_set_dist_fact",errmsg,exerrval);
   return (EX_FATAL);
       }
   }


/* write out the distribution factors array */

   start[0] = 0;

   count[0] = num_df_in_set;

   if (ncvarput (exoid, dist_id, start, count,
             ex_conv_array(exoid,WRITE_CONVERT,set_dist_fact,
                           (int)num_df_in_set)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store dist factors for %s set %d in file id %d",
             typeName, set_id,exoid);
     ex_err("ex_put_set_dist_fact",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);

}

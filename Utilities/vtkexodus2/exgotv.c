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
* exgvtt - ex_get_object_truth_vector
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid              exodus file id
*       int     num_blk            number of blocks
*       int     num_var            number of variables
*
* exit conditions - 
*       int*    var_tab            element variable truth vector array
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
 * reads the EXODUS II specified variable truth vector from the database
 */

int ex_get_object_truth_vector (int  exoid,
        const char *obj_type,
        int  entity_id,
        int  num_var,
        int *var_vec)
{
   int varid, tabid, i, iresult, ent_ndx;
   long num_var_db = -1;
   long start[2], count[2]; 
   nclong *longs;
   char errmsg[MAX_ERR_LENGTH];
   const char* routine = "ex_get_object_truth_vector";

   /*
    * The ent_type and the var_name are used to build the netcdf
    * variables name.  Normally this is done via a macro defined in
    * exodusII_int.h
    */
   const char* ent_type = NULL;
   const char* var_name = NULL;

   exerrval = 0; /* clear error code */
   
   if (*obj_type == 'e' || *obj_type == 'E') {
     varid = ex_get_dimension(exoid, DIM_NUM_ELE_VAR,  "element variables", &num_var_db, routine);
     tabid = ncvarid (exoid, VAR_ELEM_TAB);
     var_name = "vals_elem_var";
     ent_ndx = ex_id_lkup(exoid, VAR_ID_EL_BLK, entity_id);
     ent_type = "eb";
   }
   else if (*obj_type == 'm' || *obj_type == 'M') {
     varid = ex_get_dimension(exoid, DIM_NUM_NSET_VAR, "nodeset variables", &num_var_db, routine);
     tabid = ncvarid (exoid, VAR_NSET_TAB);
     var_name = "vals_nset_var";
     ent_ndx = ex_id_lkup(exoid, VAR_NS_IDS, entity_id);
     ent_type = "ns";
   }
   else if (*obj_type == 's' || *obj_type == 'S') {
     varid = ex_get_dimension(exoid, DIM_NUM_SSET_VAR, "sideset variables", &num_var_db, routine);
     tabid = ncvarid (exoid, VAR_SSET_TAB);
     var_name = "vals_sset_var";
     ent_ndx = ex_id_lkup(exoid, VAR_SS_IDS, entity_id);
     ent_type = "ss";
   }
   else {       /* invalid variable type */
     exerrval = EX_BADPARAM;
     sprintf(errmsg,
       "Error: Invalid variable type %c specified in file id %d",
       *obj_type, exoid);
     ex_err("ex_get_varid",errmsg,exerrval);
     return (EX_WARN);
   }
   
   if (varid == -1) {
     exerrval = ncerr;
     return (EX_WARN);
   }


   if (num_var_db != num_var) {
     exerrval = EX_FATAL;
     sprintf(errmsg,
       "Error: # of variables doesn't match those defined in file id %d", exoid);
     ex_err("ex_get_object_truth_vector",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (tabid == -1) {
     /* since truth vector isn't stored in the data file, derive it dynamically */
     for (i=0; i<num_var; i++) {
       /* NOTE: names are 1-based */
       if ((tabid = ncvarid (exoid, ex_catstr2(var_name, i+1, ent_type, ent_ndx))) == -1) {
   
   /* variable doesn't exist; put a 0 in the truth vector */
   var_vec[i] = 0;
       } else {
   /* variable exists; put a 1 in the truth vector */
   var_vec[i] = 1;
       }
     }
   } else {

     /* read in the truth vector */

     /*
      * application code has allocated an array of ints but netcdf is
      * expecting a pointer to nclongs; if ints are different sizes
      * than nclongs, we must allocate an array of nclongs then
      * convert them to ints with ltoi
      */

     /* If this is a null entity, then 'ent_ndx' will be negative.
      * We don't care in this routine, so make it positive and continue...
      */
     if (ent_ndx < 0) ent_ndx = -ent_ndx;
     
     start[0] = ent_ndx-1;
     start[1] = 0;

     count[0] = 1;
     count[1] = num_var;

     if (sizeof(int) == sizeof(nclong)) {
        iresult = ncvarget (exoid, tabid, start, count, var_vec);
     } else {
       if (!(longs = malloc (num_var * sizeof(nclong)))) {
         exerrval = EX_MEMFAIL;
         sprintf(errmsg,
                 "Error: failed to allocate memory for truth vector for file id %d",
                 exoid);
         ex_err("ex_get_object_truth_vector",errmsg,exerrval);
         return (EX_FATAL);
       }
       iresult = ncvarget (exoid, tabid, start, count, longs);
     }
     
     if (iresult == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
               "Error: failed to get truth vector from file id %d", exoid);
       ex_err("ex_get_object_truth_vector",errmsg,exerrval);
       return (EX_FATAL);
     }

     if (sizeof(int) != sizeof(nclong)) {
       ltoi (longs, var_vec, num_var);
       free (longs);
     }

   } 


   return (EX_NOERR);

}

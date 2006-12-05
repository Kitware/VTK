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
* expsetp - ex_put_set_param
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
*       int     set_type                the type of set
*       int     set_id                  set id
*       int     num_entries_in_set       number of entries in the set
*       int     num_dist_fact_in_set    number of distribution factors in the
*                                       set
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

/*!
 * writes the set id and the number of entries
 * which describe a single set
 */

int ex_put_set_param (int exoid,
                      int set_type,
                      int set_id,
                      int num_entries_in_set,
                      int num_dist_fact_in_set)
{
   int dimid, varid, set_id_ndx, dims[1]; 
   long start[1], num_sets; 
   nclong ldum;
   int cur_num_sets, set_stat;
   char *cdum;
   char errmsg[MAX_ERR_LENGTH];
   char* typeName;
   char* dimptr;
   char* idsptr;
   char* statptr;
   char* numentryptr = 0;
   char* numdfptr = 0;
   char* factptr = 0;
   char* entryptr = 0;
   char* extraptr = 0;
   struct list_item** ctr_list_ptr;

   exerrval = 0; /* clear error code */

   cdum = 0;

   /* setup pointers based on set_type 
    NOTE: there is another block that sets more stuff later ... */
   if (set_type == EX_NODE_SET) {
     typeName = "node";
     dimptr = DIM_NUM_NS;
     idsptr = VAR_NS_IDS;
     statptr = VAR_NS_STAT;
     ctr_list_ptr = &ns_ctr_list; 
   }
   else if (set_type == EX_EDGE_SET) {
     typeName = "edge";
     dimptr = DIM_NUM_ES;
     idsptr = VAR_ES_IDS;
     statptr = VAR_ES_STAT;
     ctr_list_ptr = &es_ctr_list;
   }
   else if (set_type == EX_FACE_SET) {
     typeName = "face";
     dimptr = DIM_NUM_FS;
     idsptr = VAR_FS_IDS;
     statptr = VAR_FS_STAT;
     ctr_list_ptr = &fs_ctr_list;
   }
   else if (set_type == EX_SIDE_SET) {
     typeName = "side";
     dimptr = DIM_NUM_SS;
     idsptr = VAR_SS_IDS;
     statptr = VAR_SS_STAT;
     ctr_list_ptr = &ss_ctr_list;
   }
   else if (set_type == EX_ELEM_SET) {
     typeName = "elem";
     dimptr = DIM_NUM_ELS;
     idsptr = VAR_ELS_IDS;
     statptr = VAR_ELS_STAT;
     ctr_list_ptr = &els_ctr_list;
   }
   else {
     exerrval = EX_FATAL;
     sprintf(errmsg,
             "Error: invalid set type (%d)", set_type);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

/* first check if any of that set type is specified */

   if ((dimid = ncdimid (exoid, dimptr)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
             "Error: no %s sets specified in file id %d", typeName,
             exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

/* Check for duplicate set id entry */
   ex_id_lkup(exoid, idsptr, set_id);
   if (exerrval != EX_LOOKUPFAIL)   /* found the side set id */
   {
     exerrval = ncerr;
     sprintf(errmsg,
             "Error: %s set %d already defined in file id %d", typeName,
             set_id,exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return(EX_FATAL);
   }

/* Get number of sets specified for this file */
   if ((ncdiminq (exoid,dimid,cdum,&num_sets)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of %s sets in file id %d",
             typeName, exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }


/* Keep track of the total number of sets defined using a counter stored
   in a linked list keyed by exoid.
   NOTE: ex_get_file_item finds the maximum number of sets defined
         for a specific file and returns that value.
*/
   cur_num_sets=ex_get_file_item(exoid, ctr_list_ptr);
   if (cur_num_sets >= num_sets)
   {
     exerrval = EX_FATAL;
     sprintf(errmsg,
            "Error: exceeded number of %s sets (%ld) defined in file id %d",
             typeName, num_sets,exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

/*   NOTE: ex_inc_file_item finds the current number of sets defined
         for a specific file and returns that value incremented. */

   cur_num_sets=ex_inc_file_item(exoid, ctr_list_ptr);
   set_id_ndx = cur_num_sets + 1;

  /* setup more pointers based on set_type */
   if (set_type == EX_NODE_SET) {
     numentryptr = DIM_NUM_NOD_NS(set_id_ndx);
     entryptr = VAR_NODE_NS(set_id_ndx);
     extraptr = NULL;
     /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
     numdfptr = DIM_NUM_NOD_NS(set_id_ndx);
     factptr = VAR_FACT_NS(set_id_ndx);
   }
   else if (set_type == EX_EDGE_SET) {
     numentryptr = DIM_NUM_EDGE_ES(set_id_ndx);
     entryptr = VAR_EDGE_ES(set_id_ndx);
     extraptr = VAR_ORNT_ES(set_id_ndx);
     numdfptr = DIM_NUM_DF_ES(set_id_ndx);
     factptr = VAR_FACT_ES(set_id_ndx);
   }
   else if (set_type == EX_FACE_SET) {
     numentryptr = DIM_NUM_FACE_FS(set_id_ndx);
     entryptr = VAR_FACE_FS(set_id_ndx);
     extraptr = VAR_ORNT_FS(set_id_ndx);
     numdfptr = DIM_NUM_DF_FS(set_id_ndx);
     factptr = VAR_FACT_FS(set_id_ndx);
   }
   else if (set_type == EX_SIDE_SET) {
     numentryptr = DIM_NUM_SIDE_SS(set_id_ndx);
     entryptr = VAR_ELEM_SS(set_id_ndx);
     extraptr = VAR_SIDE_SS(set_id_ndx);
     numdfptr = DIM_NUM_DF_SS(set_id_ndx);
     factptr = VAR_FACT_SS(set_id_ndx);
   }
   if (set_type == EX_ELEM_SET) {
     numentryptr = DIM_NUM_ELE_ELS(set_id_ndx);
     entryptr = VAR_ELEM_ELS(set_id_ndx);
     extraptr = NULL;
     numdfptr = DIM_NUM_DF_ELS(set_id_ndx);
     factptr = VAR_FACT_ELS(set_id_ndx);
   }

/* write out information to previously defined variable */

   /* first: get id of set id variable */

   if ((varid = ncvarid (exoid, idsptr)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
             "Error: failed to locate %s set %d in file id %d", typeName,
             set_id, exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* write out set id */

   start[0] = cur_num_sets;

   ldum = (nclong)set_id;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
             "Error: failed to store %s set id %d in file id %d", typeName,
             set_id, exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (num_entries_in_set == 0) /* Is this a NULL  set? */
     set_stat = 0; /* change set status to NULL */
   else
     set_stat = 1; /* change set status to TRUE */

   if ((varid = ncvarid (exoid, statptr)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
             "Error: failed to locate %s set status in file id %d", typeName,
             exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   ldum = (nclong)set_stat;
   if (ncvarput1 (exoid, varid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
             "Error: failed to store %s set %d status to file id %d", typeName,
             set_id, exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (num_entries_in_set == 0) /* Is this a NULL set? */
   {
     return(EX_NOERR);
   }

/* put netcdf file into define mode  */

   if (ncredef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to put file id %d into define mode",
             exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }


/* define dimensions and variables */

   if ((dimid = ncdimdef(exoid, numentryptr,
                         (long)num_entries_in_set)) == -1)
     {
       if (ncerr == NC_ENAMEINUSE)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                   "Error: %s set %d size already defined in file id %d",
                   typeName, set_id,exoid);
           ex_err("ex_put_set_param",errmsg,exerrval);
         }
       else {
         exerrval = ncerr;
         sprintf(errmsg,
                 "Error: failed to define number of entries in %s set %d in file id %d",
                 typeName, set_id,exoid);
         ex_err("ex_put_set_param",errmsg,exerrval);
       }
       goto error_ret;
     }

/* create variable array in which to store the entry lists */

   dims[0] = dimid;
   
   if (ncvardef (exoid, entryptr, NC_LONG, 1, dims) == -1)
     {
       if (ncerr == NC_ENAMEINUSE)
         {
           exerrval = ncerr;
           sprintf(errmsg,
                   "Error: entry list already exists for %s set %d in file id %d",
                   typeName, set_id,exoid);
           ex_err("ex_put_set_param",errmsg,exerrval);
             }
       else
         {
           exerrval = ncerr;
           sprintf(errmsg,
                   "Error: failed to create entry list for %s set %d in file id %d",
                   typeName, set_id,exoid);
           ex_err("ex_put_set_param",errmsg,exerrval);
         }
       goto error_ret;            /* exit define mode and return */
     }

   if (extraptr) 
     {
       if (ncvardef (exoid, extraptr, NC_LONG, 1, dims) == -1)
         {
           if (ncerr == NC_ENAMEINUSE)
             {
               exerrval = ncerr;
               sprintf(errmsg,
                       "Error: extra list already exists for %s set %d in file id %d",
                       typeName, set_id, exoid);
               ex_err("ex_put_set_param",errmsg,exerrval);
             }
           else
             {
               exerrval = ncerr;
               sprintf(errmsg,
                       "Error: failed to create extra list for %s set %d in file id %d",
                       typeName, set_id,exoid);
               ex_err("ex_put_set_param",errmsg,exerrval);
             }
           goto error_ret;         /* exit define mode and return */
           
         }
     }

/* Create distribution factors variable if required */

   if (num_dist_fact_in_set > 0)
   {

     if (set_type == EX_NODE_SET) 
       {
         /* but num_dist_fact_in_set must equal number of nodes */
         if (num_dist_fact_in_set != num_entries_in_set)
           {
             exerrval = EX_FATAL;
             sprintf(errmsg,
                     "Error: # dist fact (%d) not equal to # nodes (%d) in node  set %d file id %d",
                     num_dist_fact_in_set, num_entries_in_set, set_id, exoid);
             ex_err("ex_put_set_param",errmsg,exerrval);
             goto error_ret;    /* exit define mode and return */
           }

         /* resuse dimid from entry lists */

       }
     else 
       {
         if ((dimid = ncdimdef (exoid, numdfptr, 
                                (long)num_dist_fact_in_set)) == -1)
           {
             exerrval = ncerr;
             sprintf(errmsg,
                     "Error: failed to define number of dist factors in %s set %d in file id %d",
                     typeName, set_id,exoid);
             ex_err("ex_put_set_param",errmsg,exerrval);
             goto error_ret;          /* exit define mode and return */
           }
       }

/* create variable array in which to store the set distribution factors
 */

     dims[0] = dimid;

     if (ncvardef (exoid, factptr,
                       nc_flt_code(exoid), 1, dims) == -1)
     {
       if (ncerr == NC_ENAMEINUSE)
       {
         exerrval = ncerr;
         sprintf(errmsg,
        "Error: dist factors list already exists for %s set %d in file id %d",
                 typeName, set_id,exoid);
         ex_err("ex_put_set_param",errmsg,exerrval);
       }
       else
       {
         exerrval = ncerr;
         sprintf(errmsg,
      "Error: failed to create dist factors list for %s set %d in file id %d",
                 typeName, set_id,exoid);
         ex_err("ex_put_set_param",errmsg,exerrval);
       }
       goto error_ret;            /* exit define mode and return */
     }

   }

/* leave define mode  */

   if (ncendef (exoid) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to complete definition in file id %d", exoid);
     ex_err("ex_put_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
       if (ncendef (exoid) == -1)     /* exit define mode */
       {
         sprintf(errmsg,
                "Error: failed to complete definition for file id %d",
                 exoid);
         ex_err("ex_put_set_param",errmsg,exerrval);
       }
       return (EX_FATAL);
}

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
* exgsetp - ex_get_set_param
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
*
* exit conditions - 
*       int*    num_entries_in_set      number of entries in the set
*       int*    num_dist_fact_in_set    number of distribution factors in the 
*                                       set
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the number of entries and the number of distribution factors which 
 * describe a single set
 */

int ex_get_set_param (int  exoid,
          int  set_type, 
          int  set_id,
          int *num_entry_in_set, 
          int *num_dist_fact_in_set)
{
   int dimid, set_id_ndx;
   long lnum_entry_in_set, lnum_dist_fact_in_set;
   char errmsg[MAX_ERR_LENGTH];
   char* typeName;
   char* dimptr;
   char* idsptr;
   char* numentryptr;
   char* numdfptr;

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

   if ((dimid = ncdimid (exoid, dimptr)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Warning: no %s sets stored in file id %d",
             typeName, exoid);
     ex_err("ex_get_set_param",errmsg,exerrval);
     return (EX_WARN);
   }

/* Lookup index of set id in VAR_*S_IDS array */

   set_id_ndx = ex_id_lkup(exoid,idsptr,set_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)     /* NULL set? */
     {
       *num_entry_in_set = 0;
       *num_dist_fact_in_set = 0;
       return (EX_NOERR);
     }
     else
     {
       sprintf(errmsg,
              "Error: failed to locate %s set id %d in %s in file id %d",
               typeName, set_id,idsptr,exoid);
       ex_err("ex_get_set_param",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

  /* setup more pointers based on set_type */

   if (set_type == EX_NODE_SET) {
     numentryptr = DIM_NUM_NOD_NS(set_id_ndx);
     /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
     numdfptr = DIM_NUM_NOD_NS(set_id_ndx);
   }
   else if (set_type == EX_EDGE_SET) {
     numentryptr = DIM_NUM_EDGE_ES(set_id_ndx);
     numdfptr = DIM_NUM_DF_ES(set_id_ndx);
   }
   else if (set_type == EX_FACE_SET) {
     numentryptr = DIM_NUM_FACE_FS(set_id_ndx);
     numdfptr = DIM_NUM_DF_FS(set_id_ndx);
   }
   else if (set_type == EX_SIDE_SET) {
     numentryptr = DIM_NUM_SIDE_SS(set_id_ndx);
     numdfptr = DIM_NUM_DF_SS(set_id_ndx);
   }
   if (set_type == EX_ELEM_SET) {
     numentryptr = DIM_NUM_ELE_ELS(set_id_ndx);
     numdfptr = DIM_NUM_DF_ELS(set_id_ndx);
   }

/* inquire values of dimension for number of entities in set */

   if ((dimid = ncdimid (exoid, numentryptr)) == -1)
   {
     *num_entry_in_set = 0;
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to locate number of entities in %s set %d in file id %d",
             typeName, set_id, exoid);
     ex_err("ex_get_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &lnum_entry_in_set) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to get number of entities in %s set %d in file id %d",
       typeName, set_id, exoid);
     ex_err("ex_get_set_param",errmsg,exerrval);
     return (EX_FATAL);
   }
   *num_entry_in_set = lnum_entry_in_set;

   /* Inquire value of dimension of number of dist factors for this set. 
      NOTE: For node sets, because DIM_NUM_DF_NS is not used, we check to see
      if the dist factor variable for a node set index exits. If it does not,
      the dist factor count is assumed to be zero, otherwise the dist factor 
      count will be the same as the number of nodes in the set. */

   if (set_type == EX_NODE_SET) 
   {
     if ((ncvarid (exoid, VAR_FACT_NS(set_id_ndx))) == -1)
     {
       *num_dist_fact_in_set = 0;        /* signal dist factor doesn't exist */
       if (ncerr == NC_ENOTVAR)
   return (EX_NOERR);
       else
       {
   exerrval = ncerr;
   sprintf(errmsg,
     "Error: failed to locate the dist factors for %s set %d in file id %d",
     typeName, set_id,exoid);
   ex_err("ex_get_set_param",errmsg,exerrval);
   return (EX_FATAL);
       }
     }
     *num_dist_fact_in_set = lnum_entry_in_set;   /* # of df = # of nodes */
   }
   else /* all other set types */
   {
     if ((dimid = ncdimid (exoid, numdfptr)) == -1)
     {
       *num_dist_fact_in_set = 0; /* no distribution factors for this set*/
       if (ncerr == NC_EBADDIM)
   return (EX_NOERR);
       else
       {
   exerrval = ncerr;
   sprintf(errmsg,
     "Error: failed to locate number of dist factors in %s set %d in file id %d",
     typeName, set_id, exoid);
   ex_err("ex_get_set_param",errmsg,exerrval);
   return (EX_FATAL);
       }
     }

     if (ncdiminq (exoid, dimid, (char *) 0, &lnum_dist_fact_in_set) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to get number of dist factors in %s set %d in file id %d",
         typeName, set_id, exoid);
       ex_err("ex_get_set_param",errmsg,exerrval);
       return (EX_FATAL);
     }
     *num_dist_fact_in_set = lnum_dist_fact_in_set;
   }

   return (EX_NOERR);
}

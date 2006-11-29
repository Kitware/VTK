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
* expss - ex_put_set
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
*       int     set_type                set type
*       int     set_id                  set id
*       int*    set_entry_list           array of entries in set
*       int*    set_extra_list          array of extras in set

* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdlib.h> /* for free() */

/*!
 * writes the set entry list and set extra list for a single set
 */

int ex_put_set (int   exoid,
    int   set_type,
    int   set_id,
    const int  *set_entry_list,
    const int  *set_extra_list)
{
   int dimid, iresult;
   int entry_list_id, extra_list_id, set_id_ndx;
   long  num_entries_in_set, start[1], count[1]; 
   nclong *lptr;
   char errmsg[MAX_ERR_LENGTH];
   char* typeName;
   char* dimptr;
   char* idsptr;
   char* numentryptr;
   char* entryptr;
   char* extraptr;

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
            "Error: no %s sets defined in file id %d",
             typeName, exoid);
     ex_err("ex_put_set",errmsg,exerrval);
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
               typeName,set_id,exoid);
       ex_err("ex_put_set",errmsg,EX_MSG);
       return (EX_WARN);
     }
     else
     {
       sprintf(errmsg,
     "Error: failed to locate %s set id %d in VAR_*S_IDS array in file id %d",
               typeName, set_id,exoid);
       ex_err("ex_put_set",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

  /* setup more pointers based on set_type */
   if (set_type == EX_NODE_SET) {
     numentryptr = DIM_NUM_NOD_NS(set_id_ndx);
     entryptr = VAR_NODE_NS(set_id_ndx);
     extraptr = NULL;
   }
   else if (set_type == EX_EDGE_SET) {
     numentryptr = DIM_NUM_EDGE_ES(set_id_ndx);
     entryptr = VAR_EDGE_ES(set_id_ndx);
     extraptr = VAR_ORNT_ES(set_id_ndx);
   }
   else if (set_type == EX_FACE_SET) {
     numentryptr = DIM_NUM_FACE_FS(set_id_ndx);
     entryptr = VAR_FACE_FS(set_id_ndx);
     extraptr = VAR_ORNT_FS(set_id_ndx);
   }
   else if (set_type == EX_SIDE_SET) {
     numentryptr = DIM_NUM_SIDE_SS(set_id_ndx);
     entryptr = VAR_ELEM_SS(set_id_ndx);
     extraptr = VAR_SIDE_SS(set_id_ndx);
   }
   if (set_type == EX_ELEM_SET) {
     numentryptr = DIM_NUM_ELE_ELS(set_id_ndx);
     entryptr = VAR_ELEM_ELS(set_id_ndx);
     extraptr = NULL;
   }

/* inquire id's of previously defined dimensions  */

   if ((dimid = ncdimid (exoid, numentryptr)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
      "Error: failed to locate number of entities in %s set %d in file id %d",
             typeName, set_id,exoid);
     ex_err("ex_put_set",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (ncdiminq (exoid, dimid, (char *) 0, &num_entries_in_set) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
         "Error: failed to get number of entities in %s set %d in file id %d",
             typeName, set_id,exoid);
     ex_err("ex_put_set",errmsg,exerrval);
     return (EX_FATAL);
   }

/* inquire id's of previously defined variables  */

   if ((entry_list_id = ncvarid (exoid, entryptr)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
           "Error: failed to locate entry list for %s set %d in file id %d",
       typeName, set_id,exoid);
     ex_err("ex_put_set",errmsg,exerrval);
     return (EX_FATAL);
   }

   /* only do for edge, face and side sets */
   if (extraptr) 
     {
       if ((extra_list_id = ncvarid (exoid, extraptr)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to locate extra list for %s set %d in file id %d",
       typeName, set_id,exoid);
     ex_err("ex_put_set",errmsg,exerrval);
     return (EX_FATAL);
   }
     }


/* write out the entry list and extra list arrays */

/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

   start[0] = 0;
   count[0] = num_entries_in_set;

   if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarput(exoid, entry_list_id, start, count, set_entry_list);
   } else {
      lptr = itol (set_entry_list, (int)num_entries_in_set);
      iresult = ncvarput (exoid, entry_list_id, start, count, lptr);
      free(lptr);
   }

   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
      "Error: failed to store entry list for %s set %d in file id %d",
             typeName, set_id,exoid);
     ex_err("ex_put_set",errmsg,exerrval);
     return (EX_FATAL);
   }


/* this contortion is necessary because netCDF is expecting nclongs; fortunately
   it's necessary only when ints and nclongs aren't the same size */

   /* only do for edge, face and side sets */
   if (extraptr)
     {
       if (sizeof(int) == sizeof(nclong)) {
   iresult = ncvarput(exoid, extra_list_id, start, count, set_extra_list);
       } else {
   lptr = itol (set_extra_list, (int)num_entries_in_set);
   iresult = ncvarput (exoid, extra_list_id, start, count, lptr);
   free(lptr);
       }

       if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to store extra list for %s set %d in file id %d",
       typeName, set_id,exoid);
     ex_err("ex_put_set",errmsg,exerrval);
     return (EX_FATAL);
   }
     }

   /* warn if extra data was sent in for node sets and elem sets */
   if ((set_type == EX_NODE_SET || set_type == EX_ELEM_SET) &&
       set_extra_list != NULL)
     {
       sprintf(errmsg,
         "Warning: extra list was ignored for %s set %d in file id %d",
         typeName, set_id, exoid);
       ex_err("ex_put_set",errmsg,EX_MSG);
       return(EX_WARN); 
     }

   return (EX_NOERR);

}

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
* expnam - ex_put_name
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid          exodus file id
*       int     obj_type       object type
*       int     entity_id      id of entity name to write
*       char*   name           ptr to entity name
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
#include <string.h>

/*!
 * writes the name of the specified entity to the database.
 */

int ex_put_name (int   exoid,
     int   obj_type,
     int   entity_id,
     const char *name)
{
   int varid, ent_ndx; 
   long  start[2], count[2];
   char errmsg[MAX_ERR_LENGTH];
   const char *routine = "ex_put_name";
   
   exerrval = 0; /* clear error code */

   if (obj_type == EX_ELEM_BLOCK) {
     if ((varid = ncvarid (exoid, VAR_NAME_EL_BLK)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate element block names in file id %d",
         exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     ent_ndx = ex_id_lkup(exoid, VAR_ID_EL_BLK, entity_id);
   }
   else if (obj_type == EX_NODE_SET) {
     if ((varid = ncvarid (exoid, VAR_NAME_NS)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate nodeset names in file id %d",
         exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
    ent_ndx = ex_id_lkup(exoid, VAR_NS_IDS, entity_id);
   }
   else if (obj_type == EX_SIDE_SET) {
     if ((varid = ncvarid (exoid, VAR_NAME_SS)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate sideset names in file id %d",
         exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
    ent_ndx = ex_id_lkup(exoid, VAR_SS_IDS, entity_id);
   }

   else if (obj_type == EX_NODE_MAP) {
     if ((varid = ncvarid (exoid, VAR_NAME_NM)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate node map names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     ent_ndx = ex_id_lkup(exoid, VAR_NM_PROP(1), entity_id);
   }

   else if (obj_type == EX_ELEM_MAP) {
     if ((varid = ncvarid (exoid, VAR_NAME_EM)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate element map names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     ent_ndx = ex_id_lkup(exoid, VAR_EM_PROP(1), entity_id);
   }

   else {/* invalid type */
     exerrval = EX_BADPARAM;
     sprintf(errmsg,
       "Error: Invalid type specified in file id %d", exoid);
     ex_err(routine,errmsg,exerrval);
     return(EX_FATAL);
   }
   
   /* If this is a null entity, then 'ent_ndx' will be negative.
    * We don't care in this routine, so make it positive and continue...
    */
   if (ent_ndx < 0) ent_ndx = -ent_ndx;
   
   /* write EXODUS entityname */
   start[0] = ent_ndx-1;
   start[1] = 0;
   
   count[0] = 1;
   count[1] = strlen(name) + 1;
   
   if (ncvarput (exoid, varid, start, count, (void*)name) == -1) {
     exerrval = ncerr;
     sprintf(errmsg,
       "Error: failed to store entity name for id %d in file id %d",
       ent_ndx, exoid);
     ex_err(routine,errmsg,exerrval);
     return (EX_FATAL);
   }
   return(EX_NOERR);
}

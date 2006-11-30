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
* expnam - ex_put_names
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid       exodus file id
*       int     obj_type    object type
*       char*   names       ptr array of entity names
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
 * writes the names of the results variables to the database
 */

int ex_put_names (int   exoid,
      int   obj_type,
      char* names[])
{
   int i, varid; 
   long num_entity;
   long  start[2], count[2];
   char errmsg[MAX_ERR_LENGTH];
   const char *routine = "ex_put_names";
   
   exerrval = 0; /* clear error code */

   switch (obj_type) {
     /*  ======== BLOCKS ========= */
   case EX_EDGE_BLOCK:
     ex_get_dimension(exoid, DIM_NUM_ED_BLK, "edge block", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_ED_BLK)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate edge block names in file id %d",
         exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;
   case EX_FACE_BLOCK:
     ex_get_dimension(exoid, DIM_NUM_FA_BLK, "face block", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_FA_BLK)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate face block names in file id %d",
         exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;
   case EX_ELEM_BLOCK:
     ex_get_dimension(exoid, DIM_NUM_EL_BLK, "element block", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_EL_BLK)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate element block names in file id %d",
         exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;

     /*  ======== SETS ========= */
   case EX_NODE_SET:
     ex_get_dimension(exoid, DIM_NUM_NS, "nodeset", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_NS)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate nodeset names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;
   case EX_EDGE_SET:
     ex_get_dimension(exoid, DIM_NUM_ES, "edgeset", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_ES)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate edgeset names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;
   case EX_FACE_SET:
     ex_get_dimension(exoid, DIM_NUM_FS, "faceset", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_FS)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate faceset names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;
   case EX_SIDE_SET:
     ex_get_dimension(exoid, DIM_NUM_SS, "sideset", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_SS)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate sideset names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;
   case EX_ELEM_SET:
     ex_get_dimension(exoid, DIM_NUM_ELS, "elemset", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_ELS)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate elemset names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;

     /*  ======== MAPS ========= */
   case EX_NODE_MAP:
     ex_get_dimension(exoid, DIM_NUM_NM, "node map", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_NM)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate node map names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;
   case EX_EDGE_MAP:
     ex_get_dimension(exoid, DIM_NUM_EDM, "edge map", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_EDM)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate edge map names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;
   case EX_FACE_MAP:
     ex_get_dimension(exoid, DIM_NUM_FAM, "face map", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_FAM)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate face map names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;
   case EX_ELEM_MAP:
     ex_get_dimension(exoid, DIM_NUM_EM, "element map", &num_entity, routine);

     if ((varid = ncvarid (exoid, VAR_NAME_EM)) == -1) {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to locate element map names in file id %d", exoid);
       ex_err(routine,errmsg,exerrval);
       return (EX_FATAL);
     }
     break;

     /*  ======== ERROR (Invalid type) ========= */
   default:
     exerrval = EX_BADPARAM;
     sprintf(errmsg,
       "Error: Invalid type specified in file id %d", exoid);
     ex_err(routine,errmsg,exerrval);
     return(EX_FATAL);
   }
   

   /* write EXODUS entity names */

   for ( i = 0; (long)i < num_entity; i++)
     {
     if ( names[i] != (char)'\0' ) {
       start[0] = i;
       start[1] = 0;

       count[0] = 1;
       count[1] = strlen(names[i]) + 1;

       if (ncvarput (exoid, varid, start, count, (void*) names[i]) == -1) {
         exerrval = ncerr;
         sprintf(errmsg,
           "Error: failed to store entity names in file id %d", exoid);
         ex_err(routine,errmsg,exerrval);
         return (EX_FATAL);
       }
     }
   }
   return(EX_NOERR);
}

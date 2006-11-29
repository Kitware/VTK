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
* exgnam - ex_get_names
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid          exodus file id
*       int    obj_type,
*
* exit conditions - 
*       char*   names[]           ptr array of names
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the entity names from the database
 */

int ex_get_names (int exoid,
                  int obj_type,
                  char **names)
{
   int i, j, varid;
   long num_entity, start[2];
   char *ptr;
   char errmsg[MAX_ERR_LENGTH];
   const char *routine = "ex_get_names";
   
   exerrval = 0; /* clear error code */

/* inquire previously defined dimensions and variables  */

   switch (obj_type) {
     /*  ======== BLOCKS ========= */
   case EX_EDGE_BLOCK:
     ex_get_dimension(exoid, DIM_NUM_ED_BLK, "edge block", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_ED_BLK);
     break;
   case EX_FACE_BLOCK:
     ex_get_dimension(exoid, DIM_NUM_FA_BLK, "face block", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_FA_BLK);
     break;
   case EX_ELEM_BLOCK:
     ex_get_dimension(exoid, DIM_NUM_EL_BLK, "element block", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_EL_BLK);
     break;

     /*  ======== SETS ========= */
   case EX_NODE_SET:
     ex_get_dimension(exoid, DIM_NUM_NS, "nodeset", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_NS);
     break;
   case EX_EDGE_SET:
     ex_get_dimension(exoid, DIM_NUM_ES, "edgeset", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_ES);
     break;
   case EX_FACE_SET:
     ex_get_dimension(exoid, DIM_NUM_FS, "faceset", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_FS);
     break;
   case EX_SIDE_SET:
     ex_get_dimension(exoid, DIM_NUM_SS, "sideset", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_SS);
     break;
   case EX_ELEM_SET:
     ex_get_dimension(exoid, DIM_NUM_ELS, "elemset", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_ELS);
     break;

     /*  ======== MAPS ========= */
   case EX_NODE_MAP:
     ex_get_dimension(exoid, DIM_NUM_NM, "node map", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_NM);
     break;
   case EX_EDGE_MAP:
     ex_get_dimension(exoid, DIM_NUM_EDM, "edge map", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_EDM);
     break;
   case EX_FACE_MAP:
     ex_get_dimension(exoid, DIM_NUM_FAM, "face map", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_FAM);
     break;
   case EX_ELEM_MAP:
     ex_get_dimension(exoid, DIM_NUM_EM, "element map", &num_entity, routine);
     varid = ncvarid (exoid, VAR_NAME_EM);
     break;

     /* invalid variable type */
   default:
     exerrval = EX_BADPARAM;
     sprintf(errmsg, "Error: Invalid type specified in file id %d",
             exoid);
     ex_err(routine,errmsg,exerrval);
     return(EX_FATAL);
   }
   
   if (varid != -1) {
     /* read the names */
     for (i=0; i<num_entity; i++) {
       start[0] = i;
       start[1] = 0;
       
       j = 0;
       ptr = names[i];
       
       if (ncvarget1 (exoid, varid, start, ptr) == -1) {
         exerrval = ncerr;
         sprintf(errmsg,
                 "Error: failed to get names in file id %d", exoid);
         ex_err("ex_get_names",errmsg,exerrval);
         return (EX_FATAL);
       }
       
       
       while ((*ptr++ != '\0') && (j < MAX_STR_LENGTH)) {
         start[1] = ++j;
         if (ncvarget1 (exoid, varid, start, ptr) == -1) {
           exerrval = ncerr;
           sprintf(errmsg,
                   "Error: failed to get names in file id %d", exoid);
           ex_err("ex_get_names",errmsg,exerrval);
           return (EX_FATAL);
         }
       }
       --ptr;
       if (ptr > names[i]) {
         /*    get rid of trailing blanks */
         while (*(--ptr) == ' ');
       }
       *(++ptr) = '\0';
     }
   } else {
     /* Names variable does not exist on the database; probably since this is an
      * older version of the database.  Return an empty array...
      */
     for (i=0; i<num_entity; i++) {
       names[i][0] = '\0';
     }
   }
   return (EX_NOERR);
}

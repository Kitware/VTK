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
* exgpa - ex_get_prop_array: read object property array
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                type of object (element block, node
*                                               set or side set)
*       char*   prop_name               name of the property for which the
*                                               values will be read
*       
* exit conditions - 
*       int*    values                  returned array of property values
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads an array of object properties
 */

int ex_get_prop_array (int   exoid,
                       ex_entity_type obj_type,
                       const char *prop_name,
                       int  *values)
{
   int num_props, i, propid, status;
   int found = FALSE;
   char name[MAX_VAR_NAME_LENGTH+1];
   char tmpstr[MAX_STR_LENGTH+1];

   char errmsg[MAX_ERR_LENGTH];

   exerrval  = 0; /* clear error code */

/* open appropriate variable, depending on obj_type and prop_name */

   num_props = ex_get_num_props(exoid, obj_type);

   for (i=1; i<=num_props; i++)
   {
     switch (obj_type){
       case EX_ELEM_BLOCK:
         strcpy (name, VAR_EB_PROP(i));
         break;
       case EX_EDGE_BLOCK:
         strcpy (name, VAR_ED_PROP(i));
         break;
       case EX_FACE_BLOCK:
         strcpy (name, VAR_FA_PROP(i));
         break;
       case EX_NODE_SET:
         strcpy (name, VAR_NS_PROP(i));
         break;
       case EX_EDGE_SET:
         strcpy (name, VAR_ES_PROP(i));
         break;
       case EX_FACE_SET:
         strcpy (name, VAR_FS_PROP(i));
         break;
       case EX_ELEM_SET:
         strcpy (name, VAR_ELS_PROP(i));
         break;
       case EX_SIDE_SET:
         strcpy (name, VAR_SS_PROP(i));
         break;
       case EX_ELEM_MAP:
         strcpy (name, VAR_EM_PROP(i));
         break;
       case EX_FACE_MAP:
         strcpy (name, VAR_FAM_PROP(i));
         break;
       case EX_EDGE_MAP:
         strcpy (name, VAR_EDM_PROP(i));
         break;
       case EX_NODE_MAP:
         strcpy (name, VAR_NM_PROP(i));
         break;
       default:
         exerrval = EX_BADPARAM;
         sprintf(errmsg, "Error: object type %d not supported; file id %d",
           obj_type, exoid);
         ex_err("ex_get_prop_array",errmsg,exerrval);
         return(EX_FATAL);
     }

     if ((status = nc_inq_varid(exoid, name, &propid)) != NC_NOERR) {
       exerrval = status;
       sprintf(errmsg,
          "Error: failed to locate property array %s in file id %d",
               name, exoid);
       ex_err("ex_get_prop_array",errmsg,exerrval);
       return (EX_FATAL);
     }

     /*   compare stored attribute name with passed property name   */
     memset(tmpstr, 0, MAX_STR_LENGTH+1);
     if ((status = nc_get_att_text(exoid, propid, ATT_PROP_NAME, tmpstr)) != NC_NOERR) {
       exerrval = status;
       sprintf(errmsg,
              "Error: failed to get property name in file id %d", exoid);
       ex_err("ex_get_prop_array",errmsg,exerrval);
       return (EX_FATAL);
     }

     if (strcmp(tmpstr, prop_name) == 0) {
       found = TRUE;
       break;
     }
   }

   /* if property is not found, return warning */
   if (!found) {
     exerrval = EX_BADPARAM;
     sprintf(errmsg,
       "Warning: object type %d, property %s not defined in file id %d",
        obj_type, prop_name, exoid);
     ex_err("ex_get_prop_array",errmsg,exerrval);
     return (EX_WARN);
   }

   /* read num_obj values from property variable */
   status = nc_get_var_int(exoid, propid, values);

   if (status != NC_NOERR) {
     exerrval = status;
     sprintf(errmsg,
            "Error: failed to read values in %s property array in file id %d",
             ex_name_of_object(obj_type), exoid);
     ex_err("ex_get_prop_array",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);
}

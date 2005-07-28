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
* expp - ex_put_prop: write object property 
*
* author - Larry A. Schoof, Sandia National Laboratories
*          Victor R. Yarberry, Sandia National Laboratories
*
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                type of object (element block, node
*                                               set or side set)
*       int     obj_id                  id of object to which property will
*                                               be assigned
*       char*   prop_name               name of the property for which the
*                                               value will be stored
*       int     value                   value of the property
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

/*
 * writes an object property 
 */

int ex_put_prop (int   exoid,
                 int   obj_type,
                 int   obj_id,
                 const char *prop_name,
                 int   value)
{
   int found = FALSE;
   int num_props, i, dimid, propid, dims[1];
   long start[1]; 
   nclong ldum;
   char name[MAX_VAR_NAME_LENGTH+1];
   char obj_stype[MAX_VAR_NAME_LENGTH+1];
   char obj_vtype[MAX_VAR_NAME_LENGTH+1];
   char tmpstr[MAX_VAR_NAME_LENGTH+1];
   char dim_name[MAX_VAR_NAME_LENGTH+1];

   char errmsg[MAX_ERR_LENGTH];

   exerrval  = 0; /* clear error code */

/* check if property has already been created */

   num_props = ex_get_num_props(exoid, obj_type);

   switch (obj_type)
   {
     case EX_ELEM_BLOCK:
       strcpy (obj_vtype, VAR_ID_EL_BLK);
       strcpy (obj_stype, "element block");
       break;
     case EX_NODE_SET:
       strcpy (obj_vtype, VAR_NS_IDS);
       strcpy (obj_stype, "node set");
       break;
     case EX_SIDE_SET:
       strcpy (obj_vtype, VAR_SS_IDS);
       strcpy (obj_stype, "side set");
       break;
     case EX_ELEM_MAP:
       strcpy (obj_vtype, VAR_EM_PROP(1));
       strcpy (obj_stype, "element map");
       break;
     case EX_NODE_MAP:
       strcpy (obj_vtype, VAR_NM_PROP(1));
       strcpy (obj_stype, "node map");
       break;
     default:
       exerrval = EX_BADPARAM;
       sprintf(errmsg, "Error: object type %d not supported; file id %d",
               obj_type, exoid);
       ex_err("ex_put_prop",errmsg,exerrval);
       return(EX_FATAL);
   }

   if (num_props > 1)   /* any properties other than the default 1? */
   {

   for (i=1; i<=num_props; i++)
   {
     switch (obj_type)
     {
       case EX_ELEM_BLOCK:
         strcpy (name, VAR_EB_PROP(i));
         break;
       case EX_NODE_SET:
         strcpy (name, VAR_NS_PROP(i));
         break;
       case EX_SIDE_SET:
         strcpy (name, VAR_SS_PROP(i));
         break;
       case EX_ELEM_MAP:
         strcpy (name, VAR_EM_PROP(i));
         break;
       case EX_NODE_MAP:
         strcpy (name, VAR_NM_PROP(i));
         break;
       default:
         exerrval = EX_BADPARAM;
         sprintf(errmsg, "Error: object type %d not supported; file id %d",
                 obj_type, exoid);
         ex_err("ex_put_prop",errmsg,exerrval);
         return(EX_FATAL);
     }

     if ((propid = ncvarid (exoid, name)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
          "Error: failed to get property array id in file id %d",
               exoid);
       ex_err("ex_put_prop",errmsg,exerrval);
       return (EX_FATAL);
     }

/*   compare stored attribute name with passed property name   */

     if ((ncattget (exoid, propid, ATT_PROP_NAME, tmpstr)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to get property name in file id %d", exoid);
       ex_err("ex_put_prop",errmsg,exerrval);
       return (EX_FATAL);
     }

     if (strcmp(tmpstr, prop_name) == 0) 
     {
       found = TRUE;
       break;
     }
   }
   }

/* if property array has not been created, create it */

   if (!found)
   {
/* put netcdf file into define mode  */

     if (ncredef (exoid) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,"Error: failed to place file id %d into define mode",exoid);
       ex_err("ex_put_prop",errmsg,exerrval);
       return (EX_FATAL);
     }

/*   create a variable with a name xx_prop#, where # is the new number   */
/*   of the property                                                     */

     switch (obj_type){
       case EX_ELEM_BLOCK:
         strcpy (name, VAR_EB_PROP(num_props+1));
         strcpy (dim_name, DIM_NUM_EL_BLK);
         break;
       case EX_NODE_SET:
         strcpy (name, VAR_NS_PROP(num_props+1));
         strcpy (dim_name, DIM_NUM_NS);
         break;
       case EX_SIDE_SET:
         strcpy (name, VAR_SS_PROP(num_props+1));
         strcpy (dim_name, DIM_NUM_SS);
         break;
       case EX_ELEM_MAP:
         strcpy (name, VAR_EM_PROP(num_props+1));
         strcpy (dim_name, DIM_NUM_EM);
         break;
       case EX_NODE_MAP:
         strcpy (name, VAR_NM_PROP(num_props+1));
         strcpy (dim_name, DIM_NUM_NM);
         break;
       default:
         exerrval = EX_BADPARAM;
         sprintf(errmsg, "Error: object type %d not supported; file id %d",
           obj_type, exoid);
         ex_err("ex_put_prop",errmsg,exerrval);
         goto error_ret;        /* Exit define mode and return */
     }

/*   inquire id of previously defined dimension (number of objects) */

     if ((dimid = ncdimid (exoid, dim_name)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
       "Error: failed to locate number of objects in file id %d",
                exoid);
       ex_err("ex_put_prop",errmsg, exerrval);
       goto error_ret;  /* Exit define mode and return */
     }

     dims[0] = dimid;
     ncsetfill(exoid, NC_FILL); /* fill with zeros per routine spec */

     if ((propid = ncvardef (exoid, name, NC_LONG, 1, dims)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
          "Error: failed to create property array variable in file id %d",
               exoid);
       ex_err("ex_put_prop",errmsg,exerrval);
       goto error_ret;  /* Exit define mode and return */
     }
     ncsetfill(exoid, NC_NOFILL); /* default: nofill */

/*   store property name as attribute of property array variable */

     if ((ncattput (exoid, propid, ATT_PROP_NAME, NC_CHAR,
                    strlen(prop_name)+1, prop_name)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to store property name %s in file id %d",
               prop_name,exoid);
       ex_err("ex_put_prop",errmsg,exerrval);
       goto error_ret;  /* Exit define mode and return */
     }

/* leave define mode  */

     if (ncendef (exoid) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
         "Error: failed to leave define mode in file id %d",
          exoid);
       ex_err("ex_put_prop",errmsg,exerrval);
       return (EX_FATAL);
     }

   }

/* find index into property array using obj_id; put value in property */
/* array at proper index; ex_id_lkup returns an index that is 1-based,*/
/* but netcdf expects 0-based arrays so subtract 1                    */

   /* special case: property name ID - check for duplicate ID assignment */
   if (strcmp("ID",prop_name) == 0)
   {
     start[0] = ex_id_lkup (exoid, obj_vtype, value);
     if (exerrval != EX_LOOKUPFAIL)   /* found the id */
     {
       exerrval = EX_BADPARAM;
       sprintf(errmsg,
              "Warning: attempt to assign duplicate %s ID %d in file id %d",
               obj_stype, value, exoid);
       ex_err("ex_put_prop",errmsg,exerrval);
       return (EX_WARN);
     }
   }

   start[0] = ex_id_lkup (exoid, obj_vtype, obj_id);
   if (exerrval != 0) 
   {
     if (exerrval == EX_NULLENTITY)
     {
       sprintf(errmsg,
              "Warning: no properties allowed for NULL %s id %d in file id %d",
               obj_stype, obj_id,exoid);
       ex_err("ex_put_prop",errmsg,EX_MSG);
       return (EX_WARN);
     }
     else
     {

       exerrval = ncerr;
       sprintf(errmsg,
            "Error: failed to find value %d in %s property array in file id %d",
               obj_id, obj_stype, exoid);
       ex_err("ex_put_prop",errmsg,exerrval);
       return (EX_FATAL);
     }
   }

   start[0] = start[0] - 1; 

   ldum = (nclong)value;
   if (ncvarput1 (exoid, propid, start, &ldum) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to store property value in file id %d",
             exoid);
     ex_err("ex_put_prop",errmsg,exerrval);
     return (EX_FATAL);
   }

   return (EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ncsetfill(exoid, NC_NOFILL); /* default: nofill */

  if (ncendef (exoid) == -1)     /* exit define mode */
  {
    sprintf(errmsg,
           "Error: failed to complete definition for file id %d",
            exoid);
    ex_err("ex_put_prop",errmsg,exerrval);
  }
  return (EX_FATAL);
}

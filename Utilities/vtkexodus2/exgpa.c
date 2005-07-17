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
* exgpa - ex_get_prop_array: read object property array
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
                       int   obj_type,
                       const char *prop_name,
                       int  *values)
{
   int num_props, i, propid, dimid, iresult;
   int found = FALSE;
   long start[1], count[1], num_obj; 
   nclong *longs;
   char name[MAX_VAR_NAME_LENGTH+1];
   char tmpstr[MAX_VAR_NAME_LENGTH+1];
   char obj_stype[MAX_VAR_NAME_LENGTH+1];
   char dim_name[MAX_VAR_NAME_LENGTH+1];

   char errmsg[MAX_ERR_LENGTH];

   exerrval  = 0; /* clear error code */

/* open appropriate variable, depending on obj_type and prop_name */

   num_props = ex_get_num_props(exoid, obj_type);

   switch (obj_type)
   {
     case EX_ELEM_BLOCK:
       strcpy (obj_stype, VAR_ID_EL_BLK);
       strcpy (dim_name, DIM_NUM_EL_BLK);
       break;
     case EX_NODE_SET:
       strcpy (obj_stype, VAR_NS_IDS);
       strcpy (dim_name, DIM_NUM_NS);
       break;
     case EX_SIDE_SET:
       strcpy (obj_stype, VAR_SS_IDS);
       strcpy (dim_name, DIM_NUM_SS);
       break;
     case EX_ELEM_MAP:
       strcpy (obj_stype, VAR_EM_PROP(1));
       strcpy (dim_name, DIM_NUM_EM);
       break;
     case EX_NODE_MAP:
       strcpy (obj_stype, VAR_NM_PROP(1));
       strcpy (dim_name, DIM_NUM_NM);
       break;
     default:
       exerrval = EX_BADPARAM;
       sprintf(errmsg, "Error: object type %d not supported; file id %d",
               obj_type, exoid);
       ex_err("ex_get_prop_array",errmsg,exerrval);
       return (EX_FATAL);
   }


   for (i=1; i<=num_props; i++)
   {
     switch (obj_type){
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
         ex_err("ex_get_prop_array",errmsg,exerrval);
         return(EX_FATAL);
     }

     if ((propid = ncvarid (exoid, name)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
          "Error: failed to locate property array %s in file id %d",
               name, exoid);
       ex_err("ex_get_prop_array",errmsg,exerrval);
       return (EX_FATAL);
     }

/*   compare stored attribute name with passed property name   */

     tmpstr[0] = '\0';
     if ((ncattget (exoid, propid, ATT_PROP_NAME, tmpstr)) == -1)
     {
       exerrval = ncerr;
       sprintf(errmsg,
              "Error: failed to get property name in file id %d", exoid);
       ex_err("ex_get_prop_array",errmsg,exerrval);
       return (EX_FATAL);
     }

     if (strcmp(tmpstr, prop_name) == 0) 
     {
       found = TRUE;
       break;
     }
   }

/* if property is not found, return warning */

   if (!found)
   {
     exerrval = EX_BADPARAM;
     sprintf(errmsg,
       "Warning: object type %d, property %s not defined in file id %d",
        obj_type, prop_name, exoid);
     ex_err("ex_get_prop_array",errmsg,exerrval);
     return (EX_WARN);
   }

   if ((dimid = ncdimid (exoid, dim_name)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
     "Error: failed to locate number of objects in file id %d",
              exoid);
     ex_err("ex_get_prop_array",errmsg, exerrval);
     return(EX_FATAL);
   }

/*   get number of objects */

   if (ncdiminq (exoid, dimid, dim_name, &num_obj) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to get number of %s objects in file id %d",
             obj_stype, exoid);
     ex_err("ex_get_prop_array",errmsg, exerrval);
     return (EX_FATAL);
   }

/* read num_obj values from property variable */

/* application code has allocated an array of ints but netcdf is expecting
   a pointer to nclongs;  if ints are different sizes than nclongs,
   we must allocate an array of nclongs then convert them to ints with ltoi */

   start[0] = 0;
   count[0] = num_obj;

   if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarget (exoid, propid, start, count, values);
   } else {
     if (!(longs = malloc(num_obj * sizeof(nclong)))) {
       exerrval = EX_MEMFAIL;
       sprintf(errmsg,
               "Error: failed to allocate memory for %s property array for file id %d",
               obj_stype, exoid);
       ex_err("ex_get_prop_array",errmsg,exerrval);
       return (EX_FATAL);
     }
     iresult = ncvarget (exoid, propid, start, count, longs);
   }

   if (iresult == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
            "Error: failed to read values in %s property array in file id %d",
             obj_stype, exoid);
     ex_err("ex_get_prop_array",errmsg,exerrval);
     return (EX_FATAL);
   }

   if (sizeof(int) != sizeof(nclong)) {
      ltoi (longs, values, num_obj);
      free (longs);
   }

   return (EX_NOERR);
}

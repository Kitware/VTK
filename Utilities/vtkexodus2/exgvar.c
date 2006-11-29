/*
 * Copyright (c) 2006 Sandia Corporation. Under the terms of Contract
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
* exgev - ex_get_var
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
*       int     exoid                exodus file id
*       int     time_step            time step number
*       int     var_type             block/variable type
*                                      node, edge/face/element block, or
*                                      node/edge/face/side/element set
*       int     var_index            variable index
*       int     obj_id               object id
*       int     num_entry_this_obj   number of entries in this object
*
*
* exit conditions - 
*       float*  var_vals                array of element variable values
*
*
* revision history - 
*   20061002 - David Thompson - Adapted from ex_get_elem_var
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the values of a single element variable for one element block at 
 * one time step in the database; assume the first time step and
 * element variable index is 1
 */

int ex_get_var( int   exoid,
                int   time_step,
                int   var_type,
                int   var_index,
                int   obj_id, 
                int   num_entry_this_obj,
                void* var_vals )
{
   int varid, obj_id_ndx;
   long start[2], count[2];
   char errmsg[MAX_ERR_LENGTH];
   const char* vblkids;
   const char* tname;

   switch (var_type) {
   case EX_NODAL:
     /* FIXME: Special case: ignore obj_id, possible large_file complications, etc. */
     return ex_get_nodal_var( exoid, time_step, var_index, num_entry_this_obj, var_vals );
     break;
   case EX_GLOBAL:
     /* FIXME: Special case: all vars stored in 2-D single array. */
     return ex_get_glob_vars( exoid, time_step, num_entry_this_obj, var_vals );
     break;
   case EX_EDGE_BLOCK:
     tname = "edge block";
     vblkids = VAR_ID_ED_BLK;
     break;
   case EX_FACE_BLOCK:
     tname = "face block";
     vblkids = VAR_ID_FA_BLK;
     break;
   case EX_ELEM_BLOCK:
     tname = "element block";
     vblkids = VAR_ID_EL_BLK;
     break;
   case EX_NODE_SET:
     tname = "node set";
     vblkids = VAR_NS_IDS;
     break;
   case EX_EDGE_SET:
     tname = "edge set";
     vblkids = VAR_ES_IDS;
     break;
   case EX_FACE_SET:
     tname = "face set";
     vblkids = VAR_FS_IDS;
     break;
   case EX_SIDE_SET:
     tname = "side set";
     vblkids = VAR_SS_IDS;
     break;
   case EX_ELEM_SET:
     tname = "element set";
     vblkids = VAR_ELS_IDS;
     break;
   default:
     exerrval = EX_BADPARAM;
     sprintf( errmsg, "Error: Invalid variable type (%d) given for file id %d", var_type, exoid );
     ex_err( "ex_get_var", errmsg, exerrval );
     return (EX_FATAL);
   }

   exerrval = 0; /* clear error code */

  /* Determine index of obj_id in VAR_ID_EL_BLK array */
  obj_id_ndx = ex_id_lkup(exoid,vblkids,obj_id);
  if (exerrval != 0) 
  {
    if (exerrval == EX_NULLENTITY)
    {
      sprintf(errmsg,
              "Warning: no element variables for NULL block %d in file id %d",
              obj_id,exoid);
      ex_err("ex_get_elem_var",errmsg,EX_MSG);
      return (EX_WARN);
    }
    else
    {
      sprintf(errmsg,
     "Error: failed to locate element block id %d in %s variable in file id %d",
              obj_id, vblkids, exoid);
      ex_err("ex_get_elem_var",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

/* inquire previously defined variable */

   if((varid=ncvarid(exoid,ex_name_var_of_object(var_type,var_index,obj_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
          "Error: failed to locate %s %d var %d in file id %d",
          tname,obj_id,var_index,exoid); /* this msg needs to be improved */
     ex_err("ex_get_elem_var",errmsg,exerrval);
     return (EX_FATAL);
   }

/* read values of element variable */

   start[0] = --time_step;
   start[1] = 0;

   count[0] = 1;
   count[1] = num_entry_this_obj;

   if (ncvarget (exoid, varid, start, count,
        ex_conv_array(exoid,RTN_ADDRESS,var_vals,num_entry_this_obj)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
        "Error: failed to get %s %d var %d in file id %d",
             tname, obj_id, var_index, exoid);/*this msg needs to be improved*/
     ex_err("ex_get_elem_var",errmsg,exerrval);
     return (EX_FATAL);
   }


   ex_conv_array( exoid, READ_CONVERT, var_vals, num_entry_this_obj );

   return (EX_NOERR);
}

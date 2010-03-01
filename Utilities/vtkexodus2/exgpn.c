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
* exgpn - ex_get_prop_names
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                type of object; element block, node
*                                       set, or side set
*
* exit conditions - 
*       char*   prop_names[]            ptr array of property names
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <string.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the names of the property arrays from the database
 */

int ex_get_prop_names (int    exoid,
                       ex_entity_type obj_type,
                       char **prop_names)
{
  int status;
  int i, num_props, propid;
  char var_name[12];

  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0;

  /* determine which type of object property names are desired for */

  num_props = ex_get_num_props (exoid, obj_type);

  for (i=0; i<num_props; i++) {
    switch (obj_type) {
    case EX_ELEM_BLOCK:
      strcpy (var_name, VAR_EB_PROP(i+1));
      break;
    case EX_FACE_BLOCK:
      strcpy (var_name, VAR_FA_PROP(i+1));
      break;
    case EX_EDGE_BLOCK:
      strcpy (var_name, VAR_ED_PROP(i+1));
      break;
    case EX_NODE_SET:
      strcpy (var_name, VAR_NS_PROP(i+1));
      break;
    case EX_SIDE_SET:
      strcpy (var_name, VAR_SS_PROP(i+1));
      break;
    case EX_EDGE_SET:
      strcpy (var_name, VAR_ES_PROP(i+1));
      break;
    case EX_FACE_SET:
      strcpy (var_name, VAR_FS_PROP(i+1));
      break;
    case EX_ELEM_SET:
      strcpy (var_name, VAR_ELS_PROP(i+1));
      break;
    case EX_ELEM_MAP:
      strcpy (var_name, VAR_EM_PROP(i+1));
      break;
    case EX_FACE_MAP:
      strcpy (var_name, VAR_FAM_PROP(i+1));
      break;
    case EX_EDGE_MAP:
      strcpy (var_name, VAR_EDM_PROP(i+1));
      break;
    case EX_NODE_MAP:
      strcpy (var_name, VAR_NM_PROP(i+1));
      break;
    default:
      exerrval = EX_BADPARAM;
      sprintf(errmsg, "Error: object type %d not supported; file id %d",
              obj_type, exoid);
      ex_err("ex_get_prop_names",errmsg,EX_BADPARAM);
      return(EX_FATAL);
    }

    if ((status = nc_inq_varid(exoid, var_name, &propid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to locate property array %s in file id %d",
              var_name, exoid);
      ex_err("ex_get_prop_names",errmsg,exerrval);
      return (EX_FATAL);
    }

    /*   for each property, read the "name" attribute of property array variable */
    if ((status = nc_get_att_text(exoid, propid, ATT_PROP_NAME, prop_names[i])) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to get property name in file id %d", exoid);
      ex_err("ex_get_prop_names",errmsg,exerrval);
      return (EX_FATAL);
    }
  }
  return (EX_NOERR);
}

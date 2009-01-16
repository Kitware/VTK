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
* exppn - ex_put_prop_names: write property arrays names
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                type of object (element block, node
*                                               set or side set)
*       int     num_props               number of properties to be assigned
*       char**  prop_names              array of num_props names
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
 * writes the parameters to set up property name arrays
 * \param    exoid                   exodus file id
 * \param    obj_type                type of object
 * \param    num_props               number of properties to be assigned
 * \param  **prop_names              array of num_props names
 */

int ex_put_prop_names (int   exoid,
                       ex_entity_type obj_type,
                       int   num_props,
                       char **prop_names)
{
  int status;
  int oldfill, temp;
  int i, propid, dimid, dims[1];
  char name[MAX_VAR_NAME_LENGTH+1];
  int vals[1];

  char errmsg[MAX_ERR_LENGTH];

  exerrval  = 0; /* clear error code */

  /* inquire id of previously defined dimension (number of objects) */
  if ((status = nc_inq_dimid(exoid, ex_dim_num_objects(obj_type), &dimid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to locate number of %s in file id %d",
	    ex_name_of_object(obj_type), exoid);
    ex_err("ex_put_prop_names",errmsg, exerrval);
    return(EX_FATAL);
  }

  nc_set_fill(exoid, NC_FILL, &oldfill); /* fill with zeros per routine spec */

  /* put netcdf file into define mode  */
  if ((status = nc_redef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,"Error: failed to place file id %d into define mode",exoid);
    ex_err("ex_put_prop_names",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* define num_props variables; we postpend the netcdf variable name with  */
  /* a counter starting at 2 because "xx_prop1" is reserved for the id array*/
  dims[0] = dimid;

  for (i=0; i<num_props; i++) {
    switch (obj_type) {
    case EX_ELEM_BLOCK:
      strcpy (name, VAR_EB_PROP(i+2));
      break;
    case EX_FACE_BLOCK:
      strcpy (name, VAR_FA_PROP(i+2));
      break;
    case EX_EDGE_BLOCK:
      strcpy (name, VAR_ED_PROP(i+2));
      break;
    case EX_NODE_SET:
      strcpy (name, VAR_NS_PROP(i+2));
      break;
    case EX_SIDE_SET:
      strcpy (name, VAR_SS_PROP(i+2));
      break;
    case EX_EDGE_SET:
      strcpy (name, VAR_ES_PROP(i+2));
      break;
    case EX_FACE_SET:
      strcpy (name, VAR_FS_PROP(i+2));
      break;
    case EX_ELEM_SET:
      strcpy (name, VAR_ELS_PROP(i+2));
      break;
    case EX_ELEM_MAP:
      strcpy (name, VAR_EM_PROP(i+2));
      break;
    case EX_FACE_MAP:
      strcpy (name, VAR_FAM_PROP(i+2));
      break;
    case EX_EDGE_MAP:
      strcpy (name, VAR_EDM_PROP(i+2));
      break;
    case EX_NODE_MAP:
      strcpy (name, VAR_NM_PROP(i+2));
      break;
    default:
      exerrval = EX_BADPARAM;
      sprintf(errmsg, "Error: object type %d not supported; file id %d",
	      obj_type, exoid);
      ex_err("ex_put_prop_names",errmsg,exerrval);
      goto error_ret;        /* Exit define mode and return */
    }

    if ((status = nc_def_var(exoid, name, NC_INT, 1, dims, &propid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to create property array variable in file id %d",
	      exoid);
      ex_err("ex_put_prop_names",errmsg,exerrval);
      goto error_ret;  /* Exit define mode and return */
    }

    vals[0] = 0; /* fill value */

    /*   create attribute to cause variable to fill with zeros per routine spec */
    if ((status = nc_put_att_int(exoid, propid, _FillValue, NC_INT, 1, vals)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
	      "Error: failed to create property name fill attribute in file id %d",
	      exoid);
      ex_err("ex_put_prop_names",errmsg,exerrval);
      goto error_ret;  /* Exit define mode and return */
    }

    /*   store property name as attribute of property array variable */
    if ((status = nc_put_att_text(exoid, propid, ATT_PROP_NAME,
				  strlen(prop_names[i])+1, prop_names[i])) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to store property name %s in file id %d",
	      prop_names[i],exoid);
      ex_err("ex_put_prop_names",errmsg,exerrval);
      goto error_ret;  /* Exit define mode and return */
    }
  }

  /* leave define mode  */
  if ((status = nc_enddef(exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
	    "Error: failed to leave define mode in file id %d", 
	    exoid);
    ex_err("ex_put_prop_names",errmsg,exerrval);
    return (EX_FATAL);
  }

  nc_set_fill(exoid, oldfill, &temp); /* default: turn off fill */
  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (nc_enddef (exoid) != NC_NOERR) {    /* exit define mode */
    sprintf(errmsg,
	    "Error: failed to complete definition for file id %d",
	    exoid);
    ex_err("ex_put_prop_names",errmsg,exerrval);
  }
  return (EX_FATAL);
}

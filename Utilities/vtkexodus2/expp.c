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

#include "exodusII.h"
#include "exodusII_int.h"
#include <string.h>

/*!
  
The function ex_put_prop() stores an integer object property value to
a single element block, node set, or side set. Although it is not
necessary to invoke ex_put_prop_names(), since ex_put_prop() will
allocate space within the data file if it hasn't been previously
allocated, it is more efficient to use ex_put_prop_names() if there is
more than one property to store. \see Efficiency for a discussion of
efficiency issues.

It should be noted that the interpretation of the values of the
integers stored as properties is left to the application code. In
general, a zero (0) means the object does not have the specified
property (or is not in the specified group); a nonzero value means the
object does have the specified property. When space is allocated for
the properties using ex_put_prop_names() or ex_put_prop(), the
properties are initialized to zero (0).

Because the ID of an element block, node set, or side set is just
another property (named \b ID), this routine can be used to change
the value of an ID. This feature must be used with caution, though,
because changing the ID of an object to the ID of another object of
the same type (element block, node set, or side set) would cause two
objects to have the same ID, and thus only the first would be
accessible. Therefore, ex_put_prop() issues a warning if a user
attempts to give two objects the same ID.

\return In case of an error, ex_put_prop() returns a negative number;
a warning will return a positive number.  Possible causes of errors
include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  data file not initialized properly with call to ex_put_init().
  -  invalid object type specified.
  -  a warning is issued if a user attempts to change the ID of an
     object to the ID of an existing object of the same type.

\param[in] exoid      exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in] obj_type   Type of object; use one of the options in the table below.
\param[in] obj_id     The element block, node set, or side set ID.
\param[in]  prop_name The name of the property for which the value will be stored. 
                      Maximum length of this string is \p MAX_STR_LENGTH .
\param[in] value      The value of the property.

<table>
<tr><td> \c EX_NODE_SET   </td><td>  Node Set entity type     </td></tr>
<tr><td> \c EX_EDGE_BLOCK </td><td>  Edge Block entity type   </td></tr>
<tr><td> \c EX_EDGE_SET   </td><td>  Edge Set entity type     </td></tr>
<tr><td> \c EX_FACE_BLOCK </td><td>  Face Block entity type   </td></tr>
<tr><td> \c EX_FACE_SET   </td><td>  Face Set entity type     </td></tr>
<tr><td> \c EX_ELEM_BLOCK </td><td>  Element Block entity type</td></tr>
<tr><td> \c EX_ELEM_SET   </td><td>  Element Set entity type  </td></tr>
<tr><td> \c EX_SIDE_SET   </td><td>  Side Set entity type     </td></tr>
<tr><td> \c EX_ELEM_MAP   </td><td>  Element Map entity type  </td></tr>
<tr><td> \c EX_NODE_MAP   </td><td>  Node Map entity type     </td></tr>
<tr><td> \c EX_EDGE_MAP   </td><td>  Edge Map entity type     </td></tr>
<tr><td> \c EX_FACE_MAP   </td><td>  Face Map entity type     </td></tr>
</table>

For an example of code to write out an object property, refer to the
description for ex_put_prop_names().
*/

int ex_put_prop (int   exoid,
                 ex_entity_type obj_type,
                 int   obj_id,
                 const char *prop_name,
                 int   value)
{
  int status;
  int oldfill, temp;
  int found = FALSE;
  int num_props, i, dimid, propid, dims[1];
  size_t start[1]; 
  size_t prop_name_len, name_length;
  int ldum;
  char name[MAX_VAR_NAME_LENGTH+1];
  char tmpstr[MAX_STR_LENGTH+1];
  char dim_name[MAX_VAR_NAME_LENGTH+1];
  int vals[1];

  char errmsg[MAX_ERR_LENGTH];

  exerrval  = 0; /* clear error code */

  /* check if property has already been created */

  num_props = ex_get_num_props(exoid, obj_type);

  if (num_props > 1) {  /* any properties other than the default 1? */

    for (i=1; i<=num_props; i++) {
      switch (obj_type) {
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
  ex_err("ex_put_prop",errmsg,exerrval);
  return(EX_FATAL);
      }

      if ((status = nc_inq_varid(exoid, name, &propid)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to get property array id in file id %d",
    exoid);
  ex_err("ex_put_prop",errmsg,exerrval);
  return (EX_FATAL);
      }

      /*   compare stored attribute name with passed property name   */
      memset(tmpstr, 0, MAX_STR_LENGTH+1);
      if ((status = nc_get_att_text(exoid, propid, ATT_PROP_NAME, tmpstr)) != NC_NOERR) {
  exerrval = status;
  sprintf(errmsg,
    "Error: failed to get property name in file id %d", exoid);
  ex_err("ex_put_prop",errmsg,exerrval);
  return (EX_FATAL);
      }

      if (strcmp(tmpstr, prop_name) == 0) {
  found = TRUE;
  break;
      }
    }
  }

  /* if property array has not been created, create it */
  if (!found) {

    name_length = ex_inquire_int(exoid, EX_INQ_DB_MAX_ALLOWED_NAME_LENGTH)+1;

    /* put netcdf file into define mode  */
    if ((status = nc_redef (exoid)) != NC_NOERR) {
      exerrval = status;
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
    case EX_FACE_BLOCK:
      strcpy (name, VAR_FA_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_FA_BLK);
      break;
    case EX_EDGE_BLOCK:
      strcpy (name, VAR_ED_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_ED_BLK);
      break;
    case EX_NODE_SET:
      strcpy (name, VAR_NS_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_NS);
      break;
    case EX_EDGE_SET:
      strcpy (name, VAR_ES_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_ES);
      break;
    case EX_FACE_SET:
      strcpy (name, VAR_FS_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_FS);
      break;
    case EX_ELEM_SET:
      strcpy (name, VAR_ELS_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_ELS);
      break;
    case EX_SIDE_SET:
      strcpy (name, VAR_SS_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_SS);
      break;
    case EX_ELEM_MAP:
      strcpy (name, VAR_EM_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_EM);
      break;
    case EX_FACE_MAP:
      strcpy (name, VAR_FAM_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_FAM);
      break;
    case EX_EDGE_MAP:
      strcpy (name, VAR_EDM_PROP(num_props+1));
      strcpy (dim_name, DIM_NUM_EDM);
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
    if ((status = nc_inq_dimid(exoid, dim_name, &dimid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to locate number of objects in file id %d",
        exoid);
      ex_err("ex_put_prop",errmsg, exerrval);
      goto error_ret;  /* Exit define mode and return */
    }

    dims[0] = dimid;
    nc_set_fill(exoid, NC_FILL, &oldfill); /* fill with zeros per routine spec */

    if ((status = nc_def_var(exoid, name, NC_INT, 1, dims, &propid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to create property array variable in file id %d",
        exoid);
      ex_err("ex_put_prop",errmsg,exerrval);
      goto error_ret;  /* Exit define mode and return */
    }

    vals[0] = 0; /* fill value */
    /*   create attribute to cause variable to fill with zeros per routine spec */
    if ((status = nc_put_att_int(exoid, propid, _FillValue, NC_INT, 1, vals)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to create property name fill attribute in file id %d",
        exoid);
      ex_err("ex_put_prop",errmsg,exerrval);
      goto error_ret;  /* Exit define mode and return */
    }

    /*   Check that the property name length is less than MAX_NAME_LENGTH */
    prop_name_len = strlen(prop_name)+1;
    if (prop_name_len > name_length) {
      fprintf(stderr,
        "Warning: The property name '%s' is too long.\n\tIt will be truncated from %d to %d characters\n",
        prop_name, (int)prop_name_len-1, (int)name_length-1);
      prop_name_len = name_length;
    }

    /*   store property name as attribute of property array variable */
    if ((status = nc_put_att_text(exoid, propid, ATT_PROP_NAME, 
          prop_name_len, (void*)prop_name)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to store property name %s in file id %d",
        prop_name,exoid);
      ex_err("ex_put_prop",errmsg,exerrval);
      goto error_ret;  /* Exit define mode and return */
    }

    ex_update_max_name_length(exoid, prop_name_len-1);
    
    /* leave define mode  */
    if ((status = nc_enddef (exoid)) != NC_NOERR) {
      exerrval = status;
      sprintf(errmsg,
        "Error: failed to leave define mode in file id %d",
        exoid);
      ex_err("ex_put_prop",errmsg,exerrval);
      return (EX_FATAL);
    }

    nc_set_fill(exoid, oldfill, &temp); /* default: nofill */
  }

  /* find index into property array using obj_id; put value in property */
  /* array at proper index; ex_id_lkup returns an index that is 1-based,*/
  /* but netcdf expects 0-based arrays so subtract 1                    */

  /* special case: property name ID - check for duplicate ID assignment */
  if (strcmp("ID",prop_name) == 0) {
    start[0] = ex_id_lkup (exoid, obj_type, value);
    if (exerrval != EX_LOOKUPFAIL)   /* found the id */
      {
  exerrval = EX_BADPARAM;
  sprintf(errmsg,
    "Warning: attempt to assign duplicate %s ID %d in file id %d",
    ex_name_of_object(obj_type), value, exoid);
  ex_err("ex_put_prop",errmsg,exerrval);
  return (EX_WARN);
      }
  }

  start[0] = ex_id_lkup (exoid, obj_type, obj_id);
  if (exerrval != 0) {
    if (exerrval == EX_NULLENTITY) {
      sprintf(errmsg,
        "Warning: no properties allowed for NULL %s id %d in file id %d",
        ex_name_of_object(obj_type), obj_id,exoid);
      ex_err("ex_put_prop",errmsg,EX_MSG);
      return (EX_WARN);
    } else {
      sprintf(errmsg,
        "Error: failed to find value %d in %s property array in file id %d",
        obj_id, ex_name_of_object(obj_type), exoid);
      ex_err("ex_put_prop",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  start[0] = start[0] - 1; 

  ldum = (int)value;
  if ((status = nc_put_var1_int(exoid, propid, start, &ldum)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
      "Error: failed to store property value in file id %d",
      exoid);
    ex_err("ex_put_prop",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  nc_set_fill(exoid, oldfill, &temp); /* default: nofill */

  if (nc_enddef (exoid) != NC_NOERR) {    /* exit define mode */
    sprintf(errmsg,
      "Error: failed to complete definition for file id %d",
      exoid);
    ex_err("ex_put_prop",errmsg,exerrval);
  }
  return (EX_FATAL);
}

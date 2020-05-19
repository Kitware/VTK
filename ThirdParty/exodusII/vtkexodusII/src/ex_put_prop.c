/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
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
 *     * Neither the name of NTESS nor the names of its
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

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ex__id_lkup, etc

/*!

The function ex_put_prop() stores an integer object property value to
a single element block, node set, or side set. Although it is not
necessary to invoke ex_put_prop_names(), since ex_put_prop() will
allocate space within the data file if it hasn't been previously
allocated, it is more efficient to use ex_put_prop_names() if there is
more than one property to store.

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

\param[in] exoid      exodus file ID returned from a previous call to
ex_create() or ex_open().
\param[in] obj_type   Type of object; use one of the options in the table below.
\param[in] obj_id     The element block, node set, or side set ID.
\param[in]  prop_name The name of the property for which the value will be
stored.
                      Maximum length of this string is \p MAX_STR_LENGTH .
\param[in] value      The value of the property.

| #ex_entity_type | description               |
| -------------- | ------------------------- |
|  #EX_NODE_SET   |  Node Set entity type     |
|  #EX_EDGE_BLOCK |  Edge Block entity type   |
|  #EX_EDGE_SET   |  Edge Set entity type     |
|  #EX_FACE_BLOCK |  Face Block entity type   |
|  #EX_FACE_SET   |  Face Set entity type     |
|  #EX_ELEM_BLOCK |  Element Block entity type|
|  #EX_ELEM_SET   |  Element Set entity type  |
|  #EX_SIDE_SET   |  Side Set entity type     |
|  #EX_ELEM_MAP   |  Element Map entity type  |
|  #EX_NODE_MAP   |  Node Map entity type     |
|  #EX_EDGE_MAP   |  Edge Map entity type     |
|  #EX_FACE_MAP   |  Face Map entity type     |

For an example of code to write out an object property, refer to the
description for ex_put_prop_names().
*/

int ex_put_prop(int exoid, ex_entity_type obj_type, ex_entity_id obj_id, const char *prop_name,
                ex_entity_id value)
{
  int       status;
  int       oldfill = 0;
  int       temp;
  int       found = EX_FALSE;
  int       num_props, i, dimid, propid, dims[1];
  int       int_type;
  size_t    start[1];
  size_t    prop_name_len, name_length;
  char *    name;
  char      tmpstr[MAX_STR_LENGTH + 1];
  char *    dim_name;
  long long vals[1];

  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* check if property has already been created */

  num_props = ex_get_num_props(exoid, obj_type);

  if (num_props > 1) { /* any properties other than the default 1? */

    for (i = 1; i <= num_props; i++) {
      switch (obj_type) {
      case EX_ELEM_BLOCK: name = VAR_EB_PROP(i); break;
      case EX_EDGE_BLOCK: name = VAR_ED_PROP(i); break;
      case EX_FACE_BLOCK: name = VAR_FA_PROP(i); break;
      case EX_NODE_SET: name = VAR_NS_PROP(i); break;
      case EX_EDGE_SET: name = VAR_ES_PROP(i); break;
      case EX_FACE_SET: name = VAR_FS_PROP(i); break;
      case EX_ELEM_SET: name = VAR_ELS_PROP(i); break;
      case EX_SIDE_SET: name = VAR_SS_PROP(i); break;
      case EX_ELEM_MAP: name = VAR_EM_PROP(i); break;
      case EX_FACE_MAP: name = VAR_FAM_PROP(i); break;
      case EX_EDGE_MAP: name = VAR_EDM_PROP(i); break;
      case EX_NODE_MAP: name = VAR_NM_PROP(i); break;
      default:
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: object type %d not supported; file id %d",
                 obj_type, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      if ((status = nc_inq_varid(exoid, name, &propid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get property array id in file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /*   compare stored attribute name with passed property name   */
      memset(tmpstr, 0, MAX_STR_LENGTH + 1);
      if ((status = nc_get_att_text(exoid, propid, ATT_PROP_NAME, tmpstr)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get property name in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      if (strcmp(tmpstr, prop_name) == 0) {
        found = EX_TRUE;
        break;
      }
    }
  }

  /* if property array has not been created, create it */
  if (!found) {

    name_length = ex_inquire_int(exoid, EX_INQ_DB_MAX_ALLOWED_NAME_LENGTH) + 1;

    /* put netcdf file into define mode  */
    if ((status = nc_redef(exoid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to place file id %d into define mode", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*   create a variable with a name xx_prop#, where # is the new number   */
    /*   of the property                                                     */

    switch (obj_type) {
    case EX_ELEM_BLOCK:
      name     = VAR_EB_PROP(num_props + 1);
      dim_name = DIM_NUM_EL_BLK;
      break;
    case EX_FACE_BLOCK:
      name     = VAR_FA_PROP(num_props + 1);
      dim_name = DIM_NUM_FA_BLK;
      break;
    case EX_EDGE_BLOCK:
      name     = VAR_ED_PROP(num_props + 1);
      dim_name = DIM_NUM_ED_BLK;
      break;
    case EX_NODE_SET:
      name     = VAR_NS_PROP(num_props + 1);
      dim_name = DIM_NUM_NS;
      break;
    case EX_EDGE_SET:
      name     = VAR_ES_PROP(num_props + 1);
      dim_name = DIM_NUM_ES;
      break;
    case EX_FACE_SET:
      name     = VAR_FS_PROP(num_props + 1);
      dim_name = DIM_NUM_FS;
      break;
    case EX_ELEM_SET:
      name     = VAR_ELS_PROP(num_props + 1);
      dim_name = DIM_NUM_ELS;
      break;
    case EX_SIDE_SET:
      name     = VAR_SS_PROP(num_props + 1);
      dim_name = DIM_NUM_SS;
      break;
    case EX_ELEM_MAP:
      name     = VAR_EM_PROP(num_props + 1);
      dim_name = DIM_NUM_EM;
      break;
    case EX_FACE_MAP:
      name     = VAR_FAM_PROP(num_props + 1);
      dim_name = DIM_NUM_FAM;
      break;
    case EX_EDGE_MAP:
      name     = VAR_EDM_PROP(num_props + 1);
      dim_name = DIM_NUM_EDM;
      break;
    case EX_NODE_MAP:
      name     = VAR_NM_PROP(num_props + 1);
      dim_name = DIM_NUM_NM;
      break;
    default:
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: object type %d not supported; file id %d", obj_type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret; /* Exit define mode and return */
    }

    /*   inquire id of previously defined dimension (number of objects) */
    if ((status = nc_inq_dimid(exoid, dim_name, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate number of objects in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* Exit define mode and return */
    }

    dims[0] = dimid;
    nc_set_fill(exoid, NC_FILL, &oldfill); /* fill with zeros per routine spec */

    int_type = NC_INT;
    if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
      int_type = NC_INT64;
    }

    if ((status = nc_def_var(exoid, name, int_type, 1, dims, &propid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to create property array variable in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* Exit define mode and return */
    }

    vals[0] = 0; /* fill value */
    /*   create attribute to cause variable to fill with zeros per routine spec
     */
    if ((status = nc_put_att_longlong(exoid, propid, _FillValue, int_type, 1, vals)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to create property name fill attribute in file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* Exit define mode and return */
    }

    /*   Check that the property name length is less than MAX_NAME_LENGTH */
    prop_name_len = strlen(prop_name) + 1;
    if (prop_name_len > name_length) {
      fprintf(stderr,
              "Warning: The property name '%s' is too long.\n\tIt will "
              "be truncated from %d to %d characters\n",
              prop_name, (int)prop_name_len - 1, (int)name_length - 1);
      prop_name_len = name_length;
    }

    /*   store property name as attribute of property array variable */
    if ((status = nc_put_att_text(exoid, propid, ATT_PROP_NAME, prop_name_len,
                                  (void *)prop_name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store property name %s in file id %d",
               prop_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* Exit define mode and return */
    }

    ex__update_max_name_length(exoid, prop_name_len - 1);

    /* leave define mode  */
    if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
      EX_FUNC_LEAVE(EX_FATAL);
    }

    nc_set_fill(exoid, oldfill, &temp); /* default: nofill */
  }

  /* find index into property array using obj_id; put value in property */
  /* array at proper index; ex__id_lkup returns an index that is 1-based,*/
  /* but netcdf expects 0-based arrays so subtract 1                    */

  /* special case: property name ID - check for duplicate ID assignment */
  if (strcmp("ID", prop_name) == 0) {
    int indx = ex__id_lkup(exoid, obj_type, value);
    if (indx != -EX_LOOKUPFAIL) { /* found the id */
      snprintf(errmsg, MAX_ERR_LENGTH,
               "Warning: attempt to assign duplicate %s ID %" PRId64 " in file id %d",
               ex_name_of_object(obj_type), value, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_DUPLICATEID);
      EX_FUNC_LEAVE(EX_WARN);
    }
  }

  status = ex__id_lkup(exoid, obj_type, obj_id);
  if (status > 0) {
    start[0] = status - 1;
  }
  else {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      if (status == EX_NULLENTITY) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: no properties allowed for NULL %s id %" PRId64 " in file id %d",
                 ex_name_of_object(obj_type), obj_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
        EX_FUNC_LEAVE(EX_WARN);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find value %" PRId64 " in %s property array in file id %d", obj_id,
               ex_name_of_object(obj_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* value is of type 'ex_entity_id' which is a typedef to int64_t or long long
   */
  status = nc_put_var1_longlong(exoid, propid, start, (long long *)&value);

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store property value in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  nc_set_fill(exoid, oldfill, &temp); /* default: nofill */

  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

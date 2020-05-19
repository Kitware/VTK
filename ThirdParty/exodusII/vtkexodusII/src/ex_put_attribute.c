/*
 * Copyright (c) 2019 National Technology & Engineering Solutions
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
#include "exodusII_int.h" // for EX_FATAL, etc

/* An assembly attribute is similar to an IOSS property consisting of
   a name, a type, and a value or values. It is not a value per entity
   in the assembly, but a value for the assembly. For now, they types
   will be limited to text, integer, and double to provide capability
   without the complexity of supporting the many types available in
   NetCDF-4 including user-defined types. Note that an attribute can
   have multiple values, for example if the attribute is a range, it
   could have the value {1.0, 100.0}

   NOTE: This type of attribute (value on entity instead of value per
   entities members, for example nodes in a nodeset) will also be added
   to the other entity types (blocks and sets) when implemented for
   assemblies.

   NOTE: Need a better name or way of distinguishing from the
   attributes which are currently supported in Exodus.
*/

static int ex__get_varid(int exoid, ex_entity_type obj_type, ex_entity_id id)
{
  const char *entryptr = NULL;
  char        errmsg[MAX_ERR_LENGTH];

  int id_ndx = 0;
  int status = 0;
  int varid  = 0;

  ex__check_valid_file_id(exoid, __func__);

  if (obj_type == EX_GLOBAL) {
    return NC_GLOBAL;
  }

  if (obj_type == EX_ASSEMBLY) {
    if ((status = nc_inq_varid(exoid, VAR_ENTITY_ASSEMBLY(id), &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id  %" PRId64 " in id array in file id %d",
               ex_name_of_object(obj_type), id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
    return varid;
  }

  if (obj_type == EX_BLOB) {
    if ((status = nc_inq_varid(exoid, VAR_ENTITY_BLOB(id), &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id  %" PRId64 " in id array in file id %d",
               ex_name_of_object(obj_type), id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
    return varid;
  }

  /* Everything else ... */
  /* First, locate index of this objects id `obj_type` id array */
  id_ndx = ex__id_lkup(exoid, obj_type, id);
  if (id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);
    if (status != 0) {
      if (status == EX_NULLENTITY) { /* NULL object?    */
        return EX_NOERR;
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id  %" PRId64 " in id array in file id %d",
               ex_name_of_object(obj_type), id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
  }

  switch (obj_type) {
  case EX_NODE_SET: entryptr = VAR_NODE_NS(id_ndx); break;
  case EX_EDGE_SET: entryptr = VAR_EDGE_ES(id_ndx); break;
  case EX_FACE_SET: entryptr = VAR_FACE_FS(id_ndx); break;
  case EX_SIDE_SET: entryptr = VAR_ELEM_SS(id_ndx); break;
  case EX_ELEM_SET: entryptr = VAR_ELEM_ELS(id_ndx); break;
  case EX_EDGE_BLOCK: entryptr = VAR_EBCONN(id_ndx); break;
  case EX_FACE_BLOCK: entryptr = VAR_FBCONN(id_ndx); break;
  case EX_ELEM_BLOCK: entryptr = VAR_CONN(id_ndx); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: object type %d not supported in call to %s", obj_type,
             __func__);
    ex_err(__func__, errmsg, EX_BADPARAM);
    return EX_FATAL;
  }

  if ((status = nc_inq_varid(exoid, entryptr, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate entity list array for %s %" PRId64 " in file id %d",
             ex_name_of_object(obj_type), id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    return EX_FATAL;
  }
  return varid;
}

/* define and output a double attribute */
int ex_put_double_attribute(int exoid, ex_entity_type obj_type, ex_entity_id id,
                            const char *atr_name, int num_values, double *values)
{
  int  status;
  char errmsg[MAX_ERR_LENGTH];
  int  varid;

  EX_FUNC_ENTER();
  varid = ex__get_varid(exoid, obj_type, id);
  if (varid <= 0 && obj_type != EX_GLOBAL) {
    /* Error message handled in ex__get_varid */
    EX_FUNC_LEAVE(varid);
  }

  /* put netcdf file into define mode  */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_put_att_double(exoid, varid, atr_name, NC_DOUBLE, num_values, values)) !=
      NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store double attribute %s on %s with id %" PRId64 " in file id %d",
             atr_name, ex_name_of_object(obj_type), id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  /* leave define mode  */
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

/* define and output an integer attribute */
int ex_put_integer_attribute(int exoid, ex_entity_type obj_type, ex_entity_id id,
                             const char *atr_name, int num_values, void_int *values)
{
  int  status;
  char errmsg[MAX_ERR_LENGTH];
  int  varid;

  EX_FUNC_ENTER();
  varid = ex__get_varid(exoid, obj_type, id);
  if (varid <= 0 && obj_type != EX_GLOBAL) {
    /* Error message handled in ex__get_varid */
    EX_FUNC_LEAVE(varid);
  }

  /* put netcdf file into define mode  */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
    status = nc_put_att_longlong(exoid, varid, atr_name, NC_INT64, num_values, values);
  }
  else {
    status = nc_put_att_int(exoid, varid, atr_name, NC_INT, num_values, values);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store integer attribute %s on %s with id %" PRId64 " in file id %d",
             atr_name, ex_name_of_object(obj_type), id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  /* leave define mode  */
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

/* define and output a text attribute.... */
int ex_put_text_attribute(int exoid, ex_entity_type obj_type, ex_entity_id id, const char *atr_name,
                          const char *value)
{
  int  status;
  char errmsg[MAX_ERR_LENGTH];
  int  varid;

  EX_FUNC_ENTER();

  varid = ex__get_varid(exoid, obj_type, id);
  if (varid <= 0 && obj_type != EX_GLOBAL) {
    /* Error message handled in ex__get_varid */
    EX_FUNC_LEAVE(varid);
  }

  /* put netcdf file into define mode  */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_put_att_text(exoid, varid, atr_name, strlen(value) + 1, value)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store text attribute %s on %s with id %" PRId64 " in file id %d",
             atr_name, ex_name_of_object(obj_type), id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  /* leave define mode  */
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

int ex_put_attribute(int exoid, ex_attribute attribute)
{
  char errmsg[MAX_ERR_LENGTH];

  switch (attribute.type) {
  case EX_INTEGER:
    return ex_put_integer_attribute(exoid, attribute.entity_type, attribute.entity_id,
                                    attribute.name, attribute.value_count, attribute.values);
  case EX_DOUBLE:
    return ex_put_double_attribute(exoid, attribute.entity_type, attribute.entity_id,
                                   attribute.name, attribute.value_count, attribute.values);
  case EX_CHAR:
    return ex_put_text_attribute(exoid, attribute.entity_type, attribute.entity_id, attribute.name,
                                 attribute.values);
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Unrecognized attribute type %d for attribute %s on %s with id %" PRId64
             " in file id %d",
             attribute.type, attribute.name, ex_name_of_object(attribute.entity_type),
             attribute.entity_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    return EX_FATAL;
  }
}

/*! Define and output the specified attributes. */
int ex_put_attributes(int exoid, size_t attr_count, ex_attribute *attr)
{
  for (size_t i = 0; i < attr_count; i++) {
    int status = ex_put_attribute(exoid, attr[i]);
    if (status != EX_NOERR) {
      return status;
    }
  }
  return EX_NOERR;
}

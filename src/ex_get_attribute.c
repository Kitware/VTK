/*
 * Copyright (c) 2019, 2020 National Technology & Engineering Solutions
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
#include <stdbool.h>

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

static bool ex__is_internal_attribute(const char *name, ex_entity_type obj_type)
{
  if (name[0] == '_') {
    return true;
  }
  else if ((strcmp(name, "elem_type") == 0) || (strcmp(name, "entity_type1") == 0) ||
           (strcmp(name, "entity_type2") == 0)) {
    return true;
  }
  else if (obj_type == EX_GLOBAL &&
           ((strcmp(name, "api_version") == 0) || (strcmp(name, "version") == 0) ||
            (strcmp(name, "floating_point_word_size") == 0) || (strcmp(name, "file_size") == 0) ||
            (strcmp(name, "maximum_name_length") == 0) || (strcmp(name, "int64_status") == 0) ||
            (strcmp(name, "title") == 0) || (strcmp(name, "nemesis_file_version") == 0) ||
            (strcmp(name, "nemesis_api_version") == 0) || (strcmp(name, "processor_info") == 0) ||
            (strcmp(name, "last_written_time") == 0))) {
    return true;
  }
  return false;
}

static int ex__get_varid(int exoid, ex_entity_type obj_type, ex_entity_id id)
{
  const char *entryptr = NULL;
  char        errmsg[MAX_ERR_LENGTH];

  int id_ndx = 0;
  int status = 0;
  int varid  = 0;

  ex__check_valid_file_id(exoid, __func__);

  /* First, locate index of this objects id `obj_type` id array */
  id_ndx = ex__id_lkup(exoid, obj_type, id);
  if (id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);
    if (status != 0) {
      if (status == EX_NULLENTITY) { /* NULL object?    */
        EX_FUNC_LEAVE(EX_NOERR);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id  %" PRId64 " in id array in file id %d",
               ex_name_of_object(obj_type), id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
  }

  switch (obj_type) {
  case EX_ASSEMBLY: entryptr = VAR_ENTITY_ASSEMBLY(id_ndx); break;
  case EX_BLOB: entryptr = VAR_ENTITY_BLOB(id_ndx); break;
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

static int ex__get_attribute_count(int exoid, ex_entity_type obj_type, ex_entity_id id, int *varid)
{
  int  att_count = 0;
  int  status;
  char errmsg[MAX_ERR_LENGTH];

  if (obj_type == EX_GLOBAL) {
    *varid = NC_GLOBAL;

    if ((status = nc_inq(exoid, NULL, NULL, &att_count, NULL)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get GLOBAL attribute count in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
  }
  else {
    *varid = ex__get_varid(exoid, obj_type, id);
    if (*varid <= 0) {
      /* Error message handled in ex__get_varid */
      return EX_FATAL;
    }

    if ((status = nc_inq_var(exoid, *varid, NULL, NULL, NULL, NULL, &att_count)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get attribute count on %s with id %" PRId64 " in file id %d",
               ex_name_of_object(obj_type), id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
  }
  return att_count;
}

/*! Get number of attributes defined on the specified entity
   type/entity id (EX_ASSEMBLY, 100).

   Filters out "internal" or "special" attributes defined by the
   NetCDF library or used by the exodus library internally.
*/
int ex_get_attribute_count(int exoid, ex_entity_type obj_type, ex_entity_id id)
{
  int status;
  int varid;
  int att_count, count;

  EX_FUNC_ENTER();

  att_count = ex__get_attribute_count(exoid, obj_type, id, &varid);
  if (att_count < 0) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get names of each attribute and see if it is an 'internal' name */
  count = att_count;
  for (int i = 0; i < count; i++) {
    char name[NC_MAX_NAME];
    if ((status = nc_inq_attname(exoid, varid, i, name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get attribute named %s on %s with id %" PRId64 " in file id %d",
               name, ex_name_of_object(obj_type), id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    if (ex__is_internal_attribute(name, obj_type)) {
      att_count--;
    }
  }
  EX_FUNC_LEAVE(att_count);
}

/*! Get the parameters for all attributes defined on the specified
    entity type/entity id (ASSEMBLY 100).

    Filters out "internal" or "special" attributes defined by the
    NetCDF library or used by the exodus library internally.

    Returns the name, type, value_count, and optionally values (if not
    null) for all attributes on this entity.  The `attr` argument must
    have enough space to hold all attributes defined on the specified
    entity.  The attribute count can be determined from
    `ex_get_attribute_count()`.  The `entity_type` and `entity_id` fields
    on `attr` will be populated with the `obj_type` and `id` function
    parameters.
*/
int ex_get_attribute_param(int exoid, ex_entity_type obj_type, ex_entity_id id, ex_attribute *attr)
{
  int  status;
  char errmsg[MAX_ERR_LENGTH];
  int  varid;
  int  att_count, count;

  EX_FUNC_ENTER();

  att_count = ex__get_attribute_count(exoid, obj_type, id, &varid);
  if (att_count < 0) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get names of each attribute and see if it is an 'internal' name; if not, copy to `attr` and
     get other parameters of the attribute
  */
  count = 0;
  for (int i = 0; i < att_count; i++) {
    char name[NC_MAX_NAME];
    if ((status = nc_inq_attname(exoid, varid, i, name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get attribute named %s on %s with id %" PRId64 " in file id %d",
               name, ex_name_of_object(obj_type), id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    if (!ex__is_internal_attribute(name, obj_type)) {
      nc_type type;
      size_t  val_count;

      if ((status = nc_inq_att(exoid, varid, name, &type, &val_count)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get parameters for attribute named %s on %s with id %" PRId64
                 " in file id %d",
                 name, ex_name_of_object(obj_type), id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
      ex_copy_string(attr[count].name, name, EX_MAX_NAME);
      attr[count].entity_type = obj_type;
      attr[count].entity_id   = id;
      attr[count].value_count = val_count;
      attr[count].type        = type;
      count++;
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

/*! Get the values for the specified attribute. */
int ex_get_attribute(int exoid, ex_attribute *attr)
{
  int  status;
  char errmsg[MAX_ERR_LENGTH];
  int  varid;

  EX_FUNC_ENTER();
  if (attr->entity_type == EX_GLOBAL) {
    varid = NC_GLOBAL;
  }
  else {
    varid = ex__get_varid(exoid, attr->entity_type, attr->entity_id);
    if (varid <= 0) {
      /* Error message handled in ex__get_varid */
      EX_FUNC_LEAVE(varid);
    }
  }

  /* If attr->values is NULL, then this routine should allocate memory
     (which needs to be freed by the client.
   */
  if (attr->values == NULL) {
    if (attr->type == EX_INTEGER) {
      attr->values = calloc(attr->value_count, sizeof(int));
    }
    else if (attr->type == EX_DOUBLE) {
      attr->values = calloc(attr->value_count, sizeof(double));
    }
    else if (attr->type == EX_CHAR) {
      attr->values = calloc(attr->value_count, sizeof(char));
    }
    if (attr->values == NULL) {
      snprintf(
          errmsg, MAX_ERR_LENGTH,
          "ERROR: failed allocate memory to store values for attribute %s on %s with id %" PRId64
          " in file id %d",
          attr->name, ex_name_of_object(attr->entity_type), attr->entity_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if ((status = nc_get_att(exoid, varid, attr->name, attr->values)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to read attribute %s on %s with id %" PRId64 " in file id %d",
             attr->name, ex_name_of_object(attr->entity_type), attr->entity_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

/*! Get the values for all of the specified attributes. */
int ex_get_attributes(int exoid, size_t attr_count, ex_attribute *attr)
{
  for (size_t i = 0; i < attr_count; i++) {
    int status = ex_get_attribute(exoid, &(attr[i]));
    if (status != EX_NOERR) {
      return status;
    }
  }
  return EX_NOERR;
}

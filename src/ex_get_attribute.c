/*
 * Copyright(C) 1999-2022, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc
#include <stdbool.h>

/* An entity attribute is similar to an IOSS property consisting of
   a name, a type, and a value or values. It is not a value per entity
   in the entity, but a value for the entity. For now, the types
   will be limited to text, integer, and double to provide capability
   without the complexity of supporting the many types available in
   NetCDF-4 including user-defined types. Note that an attribute can
   have multiple values, for example if the attribute is a range, it
   could have the value {1.0, 100.0}

   NOTE: Need a better name or way of distinguishing from the
   attributes which are currently supported in Exodus.
*/

static bool exi_is_internal_attribute(const char *name, ex_entity_type obj_type)
{
  if (name[0] == '_') {
    return true;
  }
  else if ((strcmp(name, ATT_NAME_ELB) == 0) || (strcmp(name, "entity_type1") == 0) ||
           (strcmp(name, "entity_type2") == 0)) {
    return true;
  }
  else if (obj_type == EX_GLOBAL &&
           ((strcmp(name, ATT_API_VERSION) == 0) || (strcmp(name, ATT_API_VERSION_BLANK) == 0) ||
            (strcmp(name, ATT_VERSION) == 0) || (strcmp(name, ATT_FLT_WORDSIZE) == 0) ||
            (strcmp(name, ATT_FLT_WORDSIZE_BLANK) == 0) || (strcmp(name, ATT_FILESIZE) == 0) ||
            (strcmp(name, ATT_MAX_NAME_LENGTH) == 0) || (strcmp(name, ATT_INT64_STATUS) == 0) ||
            (strcmp(name, ATT_TITLE) == 0) || (strcmp(name, ATT_NEM_FILE_VERSION) == 0) ||
            (strcmp(name, ATT_NEM_API_VERSION) == 0) || (strcmp(name, ATT_PROCESSOR_INFO) == 0) ||
            (strcmp(name, ATT_LAST_WRITTEN_TIME) == 0))) {
    return true;
  }
  else if (strncmp(name, "Field@", 6) == 0) {
    return true;
  }
  else if (strncmp(name, "Basis@", 6) == 0) {
    return true;
  }
  else if (strncmp(name, "Quad@", 5) == 0) {
    return true;
  }
  return false;
}

static int exi_get_attribute_count(int exoid, ex_entity_type obj_type, ex_entity_id id, int *varid)
{
  int att_count = 0;
  int status;

  if (obj_type == EX_GLOBAL) {
    *varid = NC_GLOBAL;

    if ((status = nc_inq(exoid, NULL, NULL, &att_count, NULL)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get GLOBAL attribute count in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
  }
  else {
    *varid = exi_get_varid(exoid, obj_type, id);
    if (*varid <= 0) {
      /* Error message handled in exi_get_varid */
      return 0;
    }

    if ((status = nc_inq_var(exoid, *varid, NULL, NULL, NULL, NULL, &att_count)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
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
  EX_FUNC_ENTER();

  int varid;
  int att_count = exi_get_attribute_count(exoid, obj_type, id, &varid);
  if (att_count < 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Negative attribute count (%d) on %s with id %" PRId64 " in file id %d",
             att_count, ex_name_of_object(obj_type), id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get names of each attribute and see if it is an 'internal' name */
  int count = att_count;
  for (int i = 0; i < count; i++) {
    char name[NC_MAX_NAME + 1];
    int  status;
    if ((status = nc_inq_attname(exoid, varid, i, name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get attribute named %s on %s with id %" PRId64 " in file id %d",
               name, ex_name_of_object(obj_type), id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    if (exi_is_internal_attribute(name, obj_type)) {
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
  char errmsg[MAX_ERR_LENGTH];
  int  varid;
  int  att_count, count;

  EX_FUNC_ENTER();

  att_count = exi_get_attribute_count(exoid, obj_type, id, &varid);
  if (att_count < 0) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get names of each attribute and see if it is an 'internal' name; if not, copy to `attr` and
     get other parameters of the attribute
  */
  count = 0;
  for (int i = 0; i < att_count; i++) {
    char name[NC_MAX_NAME + 1];
    int  status;
    if ((status = nc_inq_attname(exoid, varid, i, name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get attribute named %s on %s with id %" PRId64 " in file id %d",
               name, ex_name_of_object(obj_type), id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    if (!exi_is_internal_attribute(name, obj_type)) {
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
  EX_FUNC_ENTER();
  int varid;
  if (attr->entity_type == EX_GLOBAL) {
    varid = NC_GLOBAL;
  }
  else {
    varid = exi_get_varid(exoid, attr->entity_type, attr->entity_id);
    if (varid <= 0) {
      /* Error message handled in exi_get_varid */
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
      attr->values = calloc(attr->value_count + 1, sizeof(char));
    }
    if (attr->values == NULL) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(
          errmsg, MAX_ERR_LENGTH,
          "ERROR: failed allocate memory to store values for attribute %s on %s with id %" PRId64
          " in file id %d",
          attr->name, ex_name_of_object(attr->entity_type), attr->entity_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  int status;
  if ((status = nc_get_att(exoid, varid, attr->name, attr->values)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
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

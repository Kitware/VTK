/*
 * Copyright(C) 1999-2022, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/* An entity attribute is similar to an IOSS property consisting of
   a name, a type, and a value or values. It is not a value per entity
   in the entity, but a value for the entity itself. For now, the types
   will be limited to text, integer, and double to provide capability
   without the complexity of supporting the many types available in
   NetCDF-4 including user-defined types. Note that an attribute can
   have multiple values, for example if the attribute is a range, it
   could have the value {1.0, 100.0}

   NOTE: Need a better name or way of distinguishing from the
   attributes which are currently supported in Exodus.
*/

/* define and output a double attribute */
int ex_put_double_attribute(int exoid, ex_entity_type obj_type, ex_entity_id id,
                            const char *atr_name, int num_values, const double *values)
{
  int  status;
  char errmsg[MAX_ERR_LENGTH];
  int  varid;

  EX_FUNC_ENTER();
  varid = exi_get_varid(exoid, obj_type, id);
  if (varid <= 0 && obj_type != EX_GLOBAL) {
    /* Error message handled in exi_get_varid */
    EX_FUNC_LEAVE(varid);
  }

  /* put netcdf file into define mode  */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
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
  if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  exi_leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

/* define and output an integer attribute */
int ex_put_integer_attribute(int exoid, ex_entity_type obj_type, ex_entity_id id,
                             const char *atr_name, int num_values, const void_int *values)
{
  int  status;
  char errmsg[MAX_ERR_LENGTH];
  int  varid;

  EX_FUNC_ENTER();
  varid = exi_get_varid(exoid, obj_type, id);
  if (varid <= 0 && obj_type != EX_GLOBAL) {
    /* Error message handled in exi_get_varid */
    EX_FUNC_LEAVE(varid);
  }

  /* put netcdf file into define mode  */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
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
  if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  exi_leavedef(exoid, __func__);
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

  varid = exi_get_varid(exoid, obj_type, id);
  if (varid <= 0 && obj_type != EX_GLOBAL) {
    /* Error message handled in exi_get_varid */
    EX_FUNC_LEAVE(varid);
  }

  /* put netcdf file into define mode  */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
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
  if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  exi_leavedef(exoid, __func__);
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
int ex_put_attributes(int exoid, size_t attr_count, const ex_attribute *attr)
{
  for (size_t i = 0; i < attr_count; i++) {
    int status = ex_put_attribute(exoid, attr[i]);
    if (status != EX_NOERR) {
      return status;
    }
  }
  return EX_NOERR;
}

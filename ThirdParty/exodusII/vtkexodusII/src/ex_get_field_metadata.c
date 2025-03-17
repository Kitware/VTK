/*
 * Copyright(C) 1999-2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc
#include <assert.h>
#include <stdbool.h>

static const char *exi_get_metadata_attribute(const char *name, const char *prefix, int pre_len)
{
  /*
   * Each field or basis attribute metadata attribute consists of 2 or more attributes.
   * Return the string corresponding to {type} in an attribute of the form "Field@{name}@{type}"
   * or "Basis@{name}@{type}" or "Quad@{name}@{type}".
   */

  if (strncmp(name, prefix, pre_len) == 0) {
    /* Return the suffix (if any) following the last "@" */
    char *suffix = strrchr(name, '@');
    if (suffix != NULL) {
      suffix++;
      return suffix;
    }
  }
  return NULL;
}

static const char *exi_get_attribute_metadata_name(const char *attrib, int offset)
{
  /*
   * PRECONDITION: `attrib` is a basis or field metadata attribute of the form
   * "Basis@{name}@{type}" or "Field@{name}@{type}" `offset` is the length
   * of `Basis@` or `Field@` or `Quad@`
   *
   * Returns the `{name}` portion in `name`
   */
  static char name[EX_MAX_NAME + 1];
  memset(name, '\0', EX_MAX_NAME + 1);
  for (int i = 0; attrib[i + offset] != '@'; i++) {
    name[i] = attrib[i + offset];
  }
  return name;
}

static int exi_get_attribute_count(int exoid, ex_entity_type obj_type, ex_entity_id id, int *varid)
{
  int att_count = 0;
  int status;

  if (obj_type == EX_GLOBAL) {
    *varid = NC_GLOBAL;

    if ((status = nc_inq(exoid, NULL, NULL, &att_count, NULL)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get GLOBAL attribute count");
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
               "ERROR: failed to get attribute count on %s with id %" PRId64,
               ex_name_of_object(obj_type), id);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
  }
  return att_count;
}

int ex_get_field_metadata_count(int exoid, ex_entity_type obj_type, ex_entity_id id)
{
  EX_FUNC_ENTER();

  int varid;
  int att_count = exi_get_attribute_count(exoid, obj_type, id, &varid);
  if (att_count < 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Negative attribute count (%d) on %s with id %" PRId64,
             att_count, ex_name_of_object(obj_type), id);
    ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get names of each attribute and see if it is a 'Field metadata' name */
  int count = 0;
  for (int i = 0; i < att_count; i++) {
    char name[EX_MAX_NAME + 1];
    int  status;
    if ((status = nc_inq_attname(exoid, varid, i, name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get attribute named %s on %s with id %" PRId64, name,
               ex_name_of_object(obj_type), id);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    const char *type = exi_get_metadata_attribute(name, "Field@", 6);
    if (type != NULL && strcmp("type", type) == 0) {
      count++;
    }
  }
  EX_FUNC_LEAVE(count);
}

/*! Get the values for the specified attribute. */
int ex_get_field_metadata(int exoid, ex_field *field)
{
  EX_FUNC_ENTER();

  int varid;
  int att_count = exi_get_attribute_count(exoid, field[0].entity_type, field[0].entity_id, &varid);
  if (att_count < 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Negative attribute count (%d) on %s with id %" PRId64,
             att_count, ex_name_of_object(field[0].entity_type), field[0].entity_id);
    ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Iterate through each Field metadata field and populate `field` */
  int count = 0;
  for (int i = 0; i < att_count; i++) {
    char attr_name[EX_MAX_NAME + 1];
    int  status;
    if ((status = nc_inq_attname(exoid, varid, i, attr_name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get attribute named %s on %s with id %" PRId64, attr_name,
               ex_name_of_object(field[0].entity_type), field[0].entity_id);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    const char *fld_type = exi_get_metadata_attribute(attr_name, "Field@", 6);
    if (fld_type != NULL) {
      /* Get the field name.  We know that the `name` is of the form "Field@{name}@{item}" */
      const char *fld_name = exi_get_attribute_metadata_name(attr_name, 6);

      /* If this is the first time we have seen this `fld_name`, then increment count and
       * store the name */
      int found = -1;
      int which = 0;
      for (int ii = 0; ii < count; ii++) {
        if (strcmp(field[ii].name, fld_name) == 0) {
          found = ii;
          which = ii;
          break;
        }
      }

      if (found == -1) {
        which = count;
        strcpy(field[count].name, fld_name);
        /* Set default separator type... */
        field[count].component_separator[0] = '_';
        field[count].component_separator[1] = '\0';

        count++;
      }

      nc_type type;      /* integer, double, character, ... */
      size_t  val_count; /* how many `type` values */
      if ((status = nc_inq_att(exoid, varid, attr_name, &type, &val_count)) != NC_NOERR) {
        char errmsg[MAX_ERR_LENGTH];
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get parameters for attribute named %s on %s with id %" PRId64,
                 attr_name, ex_name_of_object(field[0].entity_type), field[0].entity_id);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      if (strcmp(fld_type, "type") == 0) {
        status = nc_get_att_int(exoid, varid, attr_name, (int *)field[which].type);
        if (field[which].nesting == 0) {
          field[which].nesting = val_count;
        }
      }
      else if (strcmp(fld_type, "separator") == 0) {
        status = nc_get_att_text(exoid, varid, attr_name, field[which].component_separator);
      }
      else if (strcmp(fld_type, "cardinality") == 0) {
        status = nc_get_att_int(exoid, varid, attr_name, field[which].cardinality);
        if (field[which].nesting == 0) {
          field[which].nesting = val_count;
        }
      }
      else if (strcmp(fld_type, "type_name") == 0) {
        status = nc_get_att_text(exoid, varid, attr_name, field[which].type_name);
      }
      else if (strcmp(fld_type, "suffices") == 0) {
        status = nc_get_att_text(exoid, varid, attr_name, field[which].suffices);
      }
      else {
        char errmsg[MAX_ERR_LENGTH];
        snprintf(
            errmsg, MAX_ERR_LENGTH,
            "ERROR: Invalid field metadata attribute type %s on field %s on %s with id %" PRId64,
            fld_type, fld_name, ex_name_of_object(field[0].entity_type), field[0].entity_id);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
      if (status != NC_NOERR) {
        char errmsg[MAX_ERR_LENGTH];
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to read field metadata attribute type %s on field %s on %s with id "
                 "%" PRId64,
                 fld_type, fld_name, ex_name_of_object(field[0].entity_type), field[0].entity_id);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

int exi_get_metadata_count(int exoid, const char *which)
{
  EX_FUNC_ENTER();

  int varid;
  int att_count = exi_get_attribute_count(exoid, EX_GLOBAL, 0, &varid);
  if (att_count < 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Negative attribute count (%d)", att_count);
    ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get names of each attribute and see if it is a `which` metadata name */
  size_t att_len = strlen(which);
  int    count   = 0;
  for (int i = 0; i < att_count; i++) {
    char name[EX_MAX_NAME + 1];
    int  status;
    if ((status = nc_inq_attname(exoid, varid, i, name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get attribute named %s", name);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    const char *type = exi_get_metadata_attribute(name, which, att_len);
    if (type != NULL && strcmp("cardinality", type) == 0) {
      count++;
    }
  }
  EX_FUNC_LEAVE(count);
}

int ex_get_basis_count(int exoid) { return exi_get_metadata_count(exoid, "Basis@"); }

int ex_get_quadrature_count(int exoid) { return exi_get_metadata_count(exoid, "Quad@"); }

int ex_get_basis(int exoid, ex_basis **pbasis, int *num_basis)
{
  /*
   * -- If this function is called and there is no basis metadata on the
   *    entity, it will return EX_NOTFOUND;
   *
   * -- If there are basis defined on the database, it will:
   *    - determine number of basis defined on database and return the count in `num_basis`
   *    - allocate `num_basis` copies of `ex_basis`
   *    - determine the cardinality of each basis and allocate the array members of each basis
   * struct.
   *    - populate the array members for each basis.
   *
   *    Upon return, the `pbasis` will contain `num_basis` structs fully populated.
   */

  // TODO: Should all array members of the struct be allocated and initialized to zero, or
  //       only the members that are actually defined on the database...
  EX_FUNC_ENTER();

  *num_basis = ex_get_basis_count(exoid);
  if (*num_basis == 0) {
    EX_FUNC_LEAVE(EX_NOTFOUND);
  }

  int varid;
  int att_count = exi_get_attribute_count(exoid, EX_GLOBAL, 0, &varid);
  if (att_count < 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Negative attribute count (%d)", att_count);
    ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (*pbasis == NULL) {
    struct ex_basis *basis = (ex_basis *)calloc(*num_basis, sizeof(struct ex_basis));
    *pbasis                = basis;
  }
  struct ex_basis *basis = *pbasis;

  // First, iterate through each attribute and get the basis name and cardinality
  int count = 0;
  for (int att = 0; att < att_count; att++) {
    char attr_name[EX_MAX_NAME + 1];
    int  status;
    if ((status = nc_inq_attname(exoid, varid, att, attr_name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get attribute named %s", attr_name);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (strncmp("Basis@", attr_name, 6) != 0) {
      continue;
    }

    const char *basis_type = exi_get_metadata_attribute(attr_name, "Basis@", 6);
    if (basis_type != NULL && strcmp(basis_type, "cardinality") == 0) {
      /* Get the basis name.  We know that the `name` is of the form "Basis@{name}@{item}" */
      const char *basis_name = exi_get_attribute_metadata_name(attr_name, 6);
      strcpy(basis[count].name, basis_name);

      int cardinality          = 0;
      status                   = nc_get_att_int(exoid, varid, attr_name, &cardinality);
      basis[count].cardinality = cardinality;
      count++;
      if (count == *num_basis) {
        break;
      }
      continue;
    }
  }

  if (count != *num_basis) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Internal error populating basis name and cardinality.  Did not find correct "
             "number of basis attributes.");
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  ex_initialize_basis_struct(basis, *num_basis, 1);

  // Now iterate the attributes again and fully populate the basis struct(s)
  for (int att = 0; att < att_count; att++) {
    char attr_name[EX_MAX_NAME + 1];
    int  status;
    if ((status = nc_inq_attname(exoid, varid, att, attr_name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get attribute named %s", attr_name);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (strncmp("Basis@", attr_name, 6) != 0) {
      continue;
    }

    const char *basis_type = exi_get_metadata_attribute(attr_name, "Basis@", 6);
    if (basis_type != NULL) {
      /* Get the basis name.  We know that the `name` is of the form "Basis@{name}@{item}" */
      const char *basis_name = exi_get_attribute_metadata_name(attr_name, 6);

      // There is no guarantee that we will be getting names in the same order as above since
      // attributes can be in any order. But, the name and cardinality for each basis will already
      // be set, so we just need to find the correct one...
      int which = -1;
      for (int i = 0; i < *num_basis; i++) {
        if (strcmp(basis[i].name, basis_name) == 0) {
          which = i;
          break;
        }
      }

      if (which == -1) {
        // Internal error...
      }

      if (strcmp(basis_type, "cardinality") == 0) {
        // Cardinality already set; skip
      }
      else if (strcmp(basis_type, "subc_dim") == 0) {
        status = nc_get_att_int(exoid, varid, attr_name, basis[which].subc_dim);
      }
      else if (status == NC_NOERR && strcmp(basis_type, "subc_ordinal") == 0) {
        status = nc_get_att_int(exoid, varid, attr_name, basis[which].subc_ordinal);
      }
      else if (status == NC_NOERR && strcmp(basis_type, "subc_dof_ordinal") == 0) {
        status = nc_get_att_int(exoid, varid, attr_name, basis[which].subc_dof_ordinal);
      }
      else if (status == NC_NOERR && strcmp(basis_type, "subc_num_dof") == 0) {
        status = nc_get_att_int(exoid, varid, attr_name, basis[which].subc_num_dof);
      }
      else if (status == NC_NOERR && strcmp(basis_type, "xi") == 0) {
        status = nc_get_att_double(exoid, varid, attr_name, basis[which].xi);
      }
      else if (status == NC_NOERR && strcmp(basis_type, "eta") == 0) {
        status = nc_get_att_double(exoid, varid, attr_name, basis[which].eta);
      }
      else if (status == NC_NOERR && strcmp(basis_type, "zeta") == 0) {
        status = nc_get_att_double(exoid, varid, attr_name, basis[which].zeta);
      }

      if (status != NC_NOERR) {
        char errmsg[MAX_ERR_LENGTH];
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to read Basis %s metadata",
                 basis[which].name);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

int ex_get_quadrature(int exoid, ex_quadrature **pquad, int *num_quad)
{
  /*
   * -- If this function is called and there is no quadrature metadata on the
   *    entity, it will return EX_NOTFOUND;
   *
   * -- If there are quadrature defined on the database, it will:
   *    - determine number of quadrature defined on database and return the count in `num_quad`
   *    - allocate `num_quad` copies of `ex_quad`
   *    - determine the cardinality of each quadrature and allocate the array members of each
   * quadrature struct.
   *    - populate the array members for each quadrature.
   *
   *    Upon return, the `pquad` will contain `num_basis` structs fully populated.
   */

  // TODO: Should all array members of the struct(xi, eta, zeta,
  //       weight) be allocated and initialized to zero, or only the
  //       members that are actually defined on the database...
  EX_FUNC_ENTER();

  *num_quad = ex_get_quadrature_count(exoid);
  if (*num_quad == 0) {
    EX_FUNC_LEAVE(EX_NOTFOUND);
  }

  int varid;
  int att_count = exi_get_attribute_count(exoid, EX_GLOBAL, 0, &varid);
  if (att_count < 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Negative attribute count (%d)", att_count);
    ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (*pquad == NULL) {
    struct ex_quadrature *quad = (ex_quadrature *)calloc(*num_quad, sizeof(struct ex_quadrature));
    *pquad                     = quad;
  }
  struct ex_quadrature *quad = *pquad;

  // First, iterate through each attribute and get the quadrature name and cardinality
  int count = 0;
  for (int att = 0; att < att_count; att++) {
    char attr_name[EX_MAX_NAME + 1];
    int  status;
    if ((status = nc_inq_attname(exoid, varid, att, attr_name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get attribute named %s", attr_name);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (strncmp("Quad@", attr_name, 5) != 0) {
      continue;
    }

    const char *quadrature_type = exi_get_metadata_attribute(attr_name, "Quad@", 5);
    if (quadrature_type != NULL && strcmp(quadrature_type, "cardinality") == 0) {
      /* Get the quadrature name.  We know that the `name` is of the form "Quad@{name}@{item}" */
      const char *quadrature_name = exi_get_attribute_metadata_name(attr_name, 5);
      strcpy(quad[count].name, quadrature_name);

      int cardinality         = 0;
      status                  = nc_get_att_int(exoid, varid, attr_name, &cardinality);
      quad[count].cardinality = cardinality;
      count++;
      if (count == *num_quad) {
        break;
      }
      continue;
    }
  }

  if (count != *num_quad) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Internal error populating quadrature name and cardinality.  Did not find "
             "correct number of quadrature attributes.");
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  ex_initialize_quadrature_struct(quad, *num_quad, 1);

  // Now iterate the attributes again and fully populate the quadrature struct(s)
  for (int att = 0; att < att_count; att++) {
    char attr_name[EX_MAX_NAME + 1];
    int  status;
    if ((status = nc_inq_attname(exoid, varid, att, attr_name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get attribute named %s", attr_name);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (strncmp("Quad@", attr_name, 5) != 0) {
      continue;
    }

    const char *quadrature_type = exi_get_metadata_attribute(attr_name, "Quad@", 5);
    if (quadrature_type != NULL) {

      /* Get the quadrature name.  We know that the `name` is of the form "Quad@{name}@{item}" */
      const char *quadrature_name = exi_get_attribute_metadata_name(attr_name, 5);

      // There is no guarantee that we will be getting names in the same order as above since
      // attributes can be in any order. But, the name and cardinality for each quadrature will
      // already be set, so we just need to find the correct one...
      int which = -1;
      for (int i = 0; i < *num_quad; i++) {
        if (strcmp(quad[i].name, quadrature_name) == 0) {
          which = i;
          break;
        }
      }

      if (which == -1) {
        // Internal error...
      }

      if (strcmp(quadrature_type, "cardinality") == 0) {
        // Cardinality already set; skip
      }
      else if (strcmp(quadrature_type, "xi") == 0) {
        status = nc_get_att_double(exoid, varid, attr_name, quad[which].xi);
      }
      else if (status == NC_NOERR && strcmp(quadrature_type, "eta") == 0) {
        status = nc_get_att_double(exoid, varid, attr_name, quad[which].eta);
      }
      else if (status == NC_NOERR && strcmp(quadrature_type, "zeta") == 0) {
        status = nc_get_att_double(exoid, varid, attr_name, quad[which].zeta);
      }
      else if (status == NC_NOERR && strcmp(quadrature_type, "weight") == 0) {
        status = nc_get_att_double(exoid, varid, attr_name, quad[which].weight);
      }
      // NOTE: Do not put an else since will fall through if the
      // arrays are NULL even though quadrature_type is valid.

      if (status != NC_NOERR) {
        char errmsg[MAX_ERR_LENGTH];
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to read Quadrature %s metadata",
                 quad[which].name);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

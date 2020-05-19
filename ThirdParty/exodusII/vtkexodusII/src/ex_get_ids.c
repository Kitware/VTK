/*
 * Copyright (c) 2005-2017, 2020 National Technology & Engineering Solutions
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
/*
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*
 *  reads the element block ids from the database
 */

struct ncvar
{ /* variable */
  char    name[MAX_VAR_NAME_LENGTH];
  nc_type type;
  int     ndims;
  int     dims[NC_MAX_VAR_DIMS];
  int     natts;
};

static int ex_get_nonstandard_ids(int exoid, ex_entity_type obj_type, void_int *ids)
{
  int  status;
  char errmsg[MAX_ERR_LENGTH];

  int64_t               count = 0;
  struct ex__file_item *file  = ex__find_file_item(exoid);
  if (file) {
    if (obj_type == EX_ASSEMBLY) {
      count = file->assembly_count;
    }
    else if (obj_type == EX_BLOB) {
      count = file->blob_count;
    }
  }

  if (count > 0) {
    /* For assemblies, we need to get the `assembly_entity` variables and read the ids from them
     * For blobs, we need to get the `blob_entity` variables and read the ids from them
     */
    int          num_found = 0;
    struct ncvar var;
    int          nvars;
    nc_inq(exoid, NULL, &nvars, NULL, NULL);
    char *type = NULL;
    if (obj_type == EX_ASSEMBLY) {
      type = "assembly_entity";
    }
    else if (obj_type == EX_BLOB) {
      type = "blob_entity";
    }

    for (int varid = 0; varid < nvars; varid++) {
      if ((status = nc_inq_var(exoid, varid, var.name, &var.type, &var.ndims, var.dims,
                               &var.natts)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable parameters in file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        return EX_FATAL;
      }
      if ((strncmp(var.name, type, strlen(type)) == 0)) {
        /* Query the "_id" attribute on this object type. */
        if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
          long long id                  = 0;
          status                        = nc_get_att_longlong(exoid, varid, EX_ATTRIBUTE_ID, &id);
          ((int64_t *)ids)[num_found++] = id;
        }
        else {
          int id                    = 0;
          status                    = nc_get_att_int(exoid, varid, EX_ATTRIBUTE_ID, &id);
          ((int *)ids)[num_found++] = id;
        }
        if (num_found == count) {
          break;
        }
        if (status != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s ids in file id %d",
                   ex_name_of_object(obj_type), exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          return EX_FATAL;
        }
      }
    }
  }
  return EX_NOERR;
}

int ex_get_ids(int exoid, ex_entity_type obj_type, void_int *ids)
{
  int  varid, status;
  char errmsg[MAX_ERR_LENGTH];

  const char *varidobj;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  if (obj_type == EX_ASSEMBLY || obj_type == EX_BLOB) {
    status = ex_get_nonstandard_ids(exoid, obj_type, ids);
    EX_FUNC_LEAVE(status);
  }

  /* Now handle the rest of the object types */
  switch (obj_type) {
  case EX_EDGE_BLOCK: varidobj = VAR_ID_ED_BLK; break;
  case EX_FACE_BLOCK: varidobj = VAR_ID_FA_BLK; break;
  case EX_ELEM_BLOCK: varidobj = VAR_ID_EL_BLK; break;
  case EX_NODE_SET: varidobj = VAR_NS_IDS; break;
  case EX_EDGE_SET: varidobj = VAR_ES_IDS; break;
  case EX_FACE_SET: varidobj = VAR_FS_IDS; break;
  case EX_SIDE_SET: varidobj = VAR_SS_IDS; break;
  case EX_ELEM_SET: varidobj = VAR_ELS_IDS; break;
  case EX_NODE_MAP: varidobj = VAR_NM_PROP(1); break;
  case EX_EDGE_MAP: varidobj = VAR_EDM_PROP(1); break;
  case EX_FACE_MAP: varidobj = VAR_FAM_PROP(1); break;
  case EX_ELEM_MAP: varidobj = VAR_EM_PROP(1); break;
  default: /* invalid variable type */
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid type specified in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Determine if there are any 'obj-type' objects */
  if ((status = nc_inq_dimid(exoid, ex__dim_num_objects(obj_type), &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no %s defined in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* inquire id's of previously defined dimensions and variables  */
  if ((status = nc_inq_varid(exoid, varidobj, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s ids variable in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* read in the element block ids  */
  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    status = nc_get_var_longlong(exoid, varid, ids);
  }
  else {
    status = nc_get_var_int(exoid, varid, ids);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to return %s ids in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

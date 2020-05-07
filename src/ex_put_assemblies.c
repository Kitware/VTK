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

#include "exodusII.h"
#include "exodusII_int.h"
#include <stdbool.h>
/*!
 * writes the assembly parameters and optionally assembly data for 1 or more assemblies
 * \param   exoid                   exodus file id
 * \param   count                   size of `assemblies` array
 * \param  *assemblies              array of ex_assembly structures
 */

int ex_put_assemblies(int exoid, size_t count, const struct ex_assembly *assemblies)
{
  int  dimid, status, dims[1];
  char errmsg[MAX_ERR_LENGTH];

  int int_type;

  EX_FUNC_ENTER();

  ex__check_valid_file_id(exoid, __func__);

  /* Note that this routine can be called:
     1) just define the assemblies
     2) just output the assembly data (after a previous call to define)
     3) define and output the assembly data in one call.
  */

  int *entlst_id = (int *)calloc(count, sizeof(int));

  bool in_define = false;
  for (size_t i = 0; i < count; i++) {
    /* See if an assembly with this id has already been defined or exists on file... */
    if (nc_inq_varid(exoid, VAR_ENTITY_ASSEMBLY(assemblies[i].id), &entlst_id[i]) != NC_NOERR) {
      /* Assembly has not already been defined */
      /* put netcdf file into define mode  */
      if (!in_define) {
        if ((status = nc_redef(exoid)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode",
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }
        in_define = true;
      }

      if (assemblies[i].entity_count <= 0) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: assembly %" PRId64
                 " -- has non-positive entity_count size %d which is not allowed in file id %d",
                 assemblies[i].id, assemblies[i].entity_count, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      }

      char *numentryptr = DIM_NUM_ENTITY_ASSEMBLY(assemblies[i].id);

      /* define dimensions and variables */
      if ((status = nc_def_dim(exoid, numentryptr, assemblies[i].entity_count, &dimid)) !=
          NC_NOERR) {
        if (status == NC_ENAMEINUSE) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: assembly %" PRId64 " -- size already defined in file id %d",
                   assemblies[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        else {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to define number of entries in assembly %" PRId64
                   " in file id %d",
                   assemblies[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        goto error_ret;
      }

      int_type = NC_INT;
      if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
        int_type = NC_INT64;
      }

      /* create variable array in which to store the entry lists */
      dims[0] = dimid;
      if ((status = nc_def_var(exoid, VAR_ENTITY_ASSEMBLY(assemblies[i].id), int_type, 1, dims,
                               &entlst_id[i])) != NC_NOERR) {
        if (status == NC_ENAMEINUSE) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: entry list already exists for assembly %" PRId64 " in file id %d",
                   assemblies[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        else {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to create entry list for assembly %" PRId64 " in file id %d",
                   assemblies[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        goto error_ret; /* exit define mode and return */
      }
      ex__compress_variable(exoid, entlst_id[i], 1);

      if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
        long long tmp = assemblies[i].id;
        status = nc_put_att_longlong(exoid, entlst_id[i], EX_ATTRIBUTE_ID, NC_INT64, 1, &tmp);
      }
      else {
        int id = assemblies[i].id;
        status = nc_put_att_int(exoid, entlst_id[i], EX_ATTRIBUTE_ID, NC_INT, 1, &id);
      }
      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to store assembly id %" PRId64 " in file id %d", assemblies[i].id,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      int type = assemblies[i].type;
      if ((status = nc_put_att_int(exoid, entlst_id[i], EX_ATTRIBUTE_TYPE, NC_INT, 1, &type)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store assembly type %d in file id %d",
                 assemblies[i].type, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      if ((status = nc_put_att_text(exoid, entlst_id[i], EX_ATTRIBUTE_NAME,
                                    strlen(assemblies[i].name) + 1, assemblies[i].name)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store assembly name %s in file id %d",
                 assemblies[i].name, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      {
        char *contains = ex_name_of_object(assemblies[i].type);
        if ((status = nc_put_att_text(exoid, entlst_id[i], EX_ATTRIBUTE_TYPENAME,
                                      strlen(contains) + 1, contains)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to store assembly type name %s in file id %d", assemblies[i].name,
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          goto error_ret; /* exit define mode and return */
        }
      }

      /* Increment assembly count */
      struct ex__file_item *file = ex__find_file_item(exoid);
      if (file) {
        file->assembly_count++;
      }
    }
  }
  /* leave define mode  */
  if (in_define) {
    if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
      free(entlst_id);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* Assembly are now all defined; see if any set data needs to be output... */
  for (size_t i = 0; i < count; i++) {
    status = EX_NOERR;
    if (assemblies[i].entity_list != NULL) {
      if ((status = nc_put_var_longlong(exoid, entlst_id[i],
                                        (long long *)assemblies[i].entity_list)) != EX_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to output entity list for assembly %" PRId64 " in file id %d",
                 assemblies[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        free(entlst_id);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }
  free(entlst_id);
  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  if (in_define) {
    ex__leavedef(exoid, __func__);
  }
  free(entlst_id);
  EX_FUNC_LEAVE(EX_FATAL);
}

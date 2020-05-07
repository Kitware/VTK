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

#include "exodusII.h"     // for ex_assembly, ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * reads the assembly parameters and optionally assembly data for one assembly
 * \param   exoid                   exodus file id
 * \param  *assembly                ex_assembly structure
 */

int ex_get_assembly(int exoid, ex_assembly *assembly)
{
  struct ex__file_item *file   = NULL;
  int                   status = 0;
  int                   dimid  = 0;
  size_t                len    = 0;
  char                  errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();

  file = ex__find_file_item(exoid);
  if (!file) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d.", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADFILEID);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* First, locate varid of assembly  */
  int entlst_id = 0;
  if ((status = nc_inq_varid(exoid, VAR_ENTITY_ASSEMBLY(assembly->id), &entlst_id)) != NC_NOERR) {
    ex_get_err(NULL, NULL, &status);
    if (status != 0) {
      if (assembly->name != NULL) {
        ex_copy_string(assembly->name, "NULL", MAX_STR_LENGTH + 1); /* NULL element type name */
      }
      assembly->entity_count = 0;
      assembly->type         = EX_INVALID;
      if (status == EX_NULLENTITY) { /* NULL element assembly?    */
        EX_FUNC_LEAVE(EX_NOERR);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate assembly id  %" PRId64 " in id array in file id %d",
               assembly->id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  char *numentryptr = DIM_NUM_ENTITY_ASSEMBLY(assembly->id);
  if ((status = nc_inq_dimid(exoid, numentryptr, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate number of entities in assembly %" PRId64 " in file id %d",
             assembly->id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimlen(exoid, dimid, &len)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get number of entities in assembly %" PRId64 " in file id %d",
             assembly->id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  assembly->entity_count = len;

  /* look up entity list array for this assembly id */
  if ((status = nc_inq_varid(exoid, VAR_ENTITY_ASSEMBLY(assembly->id), &entlst_id)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate entity list array for assembly %" PRId64 " in file id %d",
             assembly->id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the type of entities stored in the entity list... */
  int type;
  if ((status = nc_get_att_int(exoid, entlst_id, EX_ATTRIBUTE_TYPE, &type)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get assembly %" PRId64 " type in file id %d",
             assembly->id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  assembly->type = type;

  /* read the name */
  if (assembly->name != NULL) {
    int  name_size             = ex_inquire_int(exoid, EX_INQ_MAX_READ_NAME_LENGTH);
    char tmp_name[EX_MAX_NAME] = "";
    if ((status = nc_get_att_text(exoid, entlst_id, EX_ATTRIBUTE_NAME, tmp_name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to read assembly name for assembly %" PRId64 " in file id %d",
               assembly->id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    ex_copy_string(assembly->name, tmp_name, name_size + 1);
  }

  if (assembly->entity_list != NULL) {
    if ((status = nc_get_var_longlong(exoid, entlst_id, (long long int *)assembly->entity_list)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to read entity list for assembly %" PRId64 " in file id %d",
               assembly->id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

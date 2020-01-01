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
/*****************************************************************************
 *
 * exgnam - ex_get_name
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid          exodus file id
 *       const char *type       entity type - M, E, S
 *       int     entity_id      id of entity name to read
 *
 * exit conditions -
 *       char*   name           ptr to name
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_inquire_int, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*
 * reads the specified entity name from the database
 */

int ex_get_name(int exoid, ex_entity_type obj_type, ex_entity_id entity_id, char *name)
{
  int   status;
  int   varid, ent_ndx;
  char  errmsg[MAX_ERR_LENGTH];
  char *vobj = NULL;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  switch (obj_type) {
  case EX_ELEM_BLOCK: vobj = VAR_NAME_EL_BLK; break;
  case EX_EDGE_BLOCK: vobj = VAR_NAME_ED_BLK; break;
  case EX_FACE_BLOCK: vobj = VAR_NAME_FA_BLK; break;
  case EX_NODE_SET: vobj = VAR_NAME_NS; break;
  case EX_SIDE_SET: vobj = VAR_NAME_SS; break;
  case EX_EDGE_SET: vobj = VAR_NAME_ES; break;
  case EX_FACE_SET: vobj = VAR_NAME_FS; break;
  case EX_ELEM_SET: vobj = VAR_NAME_ELS; break;
  case EX_NODE_MAP: vobj = VAR_NAME_NM; break;
  case EX_EDGE_MAP: vobj = VAR_NAME_EDM; break;
  case EX_FACE_MAP: vobj = VAR_NAME_FAM; break;
  case EX_ELEM_MAP: vobj = VAR_NAME_EM; break;
  default:
    /* invalid variable type */
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid type specified in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, vobj, &varid)) == NC_NOERR) {
    /* If this is a null entity, then 'ent_ndx' will be negative.
     * We don't care in this __func__, so make it positive and continue...
     */
    ent_ndx = ex__id_lkup(exoid, obj_type, entity_id);
    if (ent_ndx < 0) {
      ent_ndx = -ent_ndx;
    }

    /* read the name */
    {
      int db_name_size  = ex_inquire_int(exoid, EX_INQ_DB_MAX_ALLOWED_NAME_LENGTH);
      int api_name_size = ex_inquire_int(exoid, EX_INQ_MAX_READ_NAME_LENGTH);
      int name_size     = db_name_size < api_name_size ? db_name_size : api_name_size;

      status = ex__get_name(exoid, varid, ent_ndx - 1, name, name_size, obj_type, __func__);
      if (status != NC_NOERR) {
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }
  else {
    /* Name variable does not exist on the database; probably since this is an
     * older version of the database.  Return an empty array...
     */
    name[0] = '\0';
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

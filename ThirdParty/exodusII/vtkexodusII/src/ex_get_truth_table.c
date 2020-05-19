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
 * exgvtt - ex_get_truth_table
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid              exodus file id
 *       int     num_blk            number of blocks
 *       int     num_var            number of variables
 *
 * exit conditions -
 *       int*    var_tab            element variable truth table array
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for ex__get_dimension, EX_FATAL, etc

/*!
 * \ingroup ResultsData
 * reads the EXODUS specified variable truth table from the database
 * \param[in]       exoid              exodus file id
 * \param[in]       obj_type           object type
 * \param[in]       num_blk            number of blocks or sets
 * \param[in]       num_var            number of variables
 * \param[out]     *var_tab            variable truth table array
 */

int ex_get_truth_table(int exoid, ex_entity_type obj_type, int num_blk, int num_var, int *var_tab)
{
  int    dimid, varid, tabid, i, j, status, status1;
  size_t num_entity = 0;
  size_t num_var_db = 0;
  char   errmsg[MAX_ERR_LENGTH];

  /*
   * The ent_type and the var_name are used to build the netcdf
   * variables name.  Normally this is done via a macro defined in
   * exodusII_int.h
   */
  const char *ent_type = NULL;
  const char *var_name = NULL;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  switch (obj_type) {
  case EX_EDGE_BLOCK:
    status =
        ex__get_dimension(exoid, DIM_NUM_EDG_VAR, "edge variables", &num_var_db, &varid, __func__);
    status1  = nc_inq_varid(exoid, VAR_EBLK_TAB, &tabid);
    var_name = "vals_edge_var";
    ent_type = "eb";
    break;
  case EX_FACE_BLOCK:
    status =
        ex__get_dimension(exoid, DIM_NUM_FAC_VAR, "face variables", &num_var_db, &varid, __func__);
    status1  = nc_inq_varid(exoid, VAR_FBLK_TAB, &tabid);
    var_name = "vals_face_var";
    ent_type = "fb";
    break;
  case EX_ELEM_BLOCK:
    status   = ex__get_dimension(exoid, DIM_NUM_ELE_VAR, "element variables", &num_var_db, &varid,
                               __func__);
    status1  = nc_inq_varid(exoid, VAR_ELEM_TAB, &tabid);
    var_name = "vals_elem_var";
    ent_type = "eb";
    break;
  case EX_NODE_SET:
    status   = ex__get_dimension(exoid, DIM_NUM_NSET_VAR, "nodeset variables", &num_var_db, &varid,
                               __func__);
    status1  = nc_inq_varid(exoid, VAR_NSET_TAB, &tabid);
    var_name = "vals_nset_var";
    ent_type = "ns";
    break;
  case EX_EDGE_SET:
    status   = ex__get_dimension(exoid, DIM_NUM_ESET_VAR, "edgeset variables", &num_var_db, &varid,
                               __func__);
    status1  = nc_inq_varid(exoid, VAR_ESET_TAB, &tabid);
    var_name = "vals_eset_var";
    ent_type = "es";
    break;
  case EX_FACE_SET:
    status   = ex__get_dimension(exoid, DIM_NUM_FSET_VAR, "faceset variables", &num_var_db, &varid,
                               __func__);
    status1  = nc_inq_varid(exoid, VAR_FSET_TAB, &tabid);
    var_name = "vals_fset_var";
    ent_type = "fs";
    break;
  case EX_SIDE_SET:
    status   = ex__get_dimension(exoid, DIM_NUM_SSET_VAR, "sideset variables", &num_var_db, &varid,
                               __func__);
    status1  = nc_inq_varid(exoid, VAR_SSET_TAB, &tabid);
    var_name = "vals_sset_var";
    ent_type = "ss";
    break;
  case EX_ELEM_SET:
    status   = ex__get_dimension(exoid, DIM_NUM_ELSET_VAR, "elemset variables", &num_var_db, &varid,
                               __func__);
    status1  = nc_inq_varid(exoid, VAR_ELSET_TAB, &tabid);
    var_name = "vals_elset_var";
    ent_type = "es";
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid variable type %d specified in file id %d",
             obj_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_WARN);
  }

  if (status != NC_NOERR) {
    EX_FUNC_LEAVE(EX_WARN);
  }

  status = ex__get_dimension(exoid, ex__dim_num_objects(obj_type), ex_name_of_object(obj_type),
                             &num_entity, &dimid, __func__);
  if (status != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (num_entity != (size_t)num_blk) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: # of %s doesn't match those defined in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (num_var_db != (size_t)num_var) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: # of %s variables doesn't match those defined in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (status1 != NC_NOERR) {
    /* since truth table isn't stored in the data file, derive it dynamically */
    for (j = 0; j < num_blk; j++) {

      for (i = 0; i < num_var; i++) {
        /* NOTE: names are 1-based */
        if (nc_inq_varid(exoid, ex__catstr2(var_name, i + 1, ent_type, j + 1), &tabid) ==
            NC_NOERR) {
          /* variable exists; put a 1 in the truth table */
          var_tab[j * num_var + i] = 1;
        }
        else {
          /* variable doesn't exist; put a 0 in the truth table */
          var_tab[j * num_var + i] = 0;
        }
      }
    }
  }
  else {
    /* read in the truth table */
    status = nc_get_var_int(exoid, tabid, var_tab);

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s truth table from file id %d",
               ex_name_of_object(obj_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

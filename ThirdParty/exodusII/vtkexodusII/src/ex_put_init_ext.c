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
 * expinix - ex_put_init_ext
 *
 * entry conditions -
 *   input parameters:
 *       int                   exoid     exodus file id
 *       const ex_init_params* params    finite element model parameters
 *
 * exit conditions -
 *
 * revision history -
 *          David Thompson  - Added edge/face blocks/sets
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_init_params, ex_err, etc
#include "exodusII_int.h" // for nc_flt_code, etc

static void write_dummy_names(int exoid, ex_entity_type obj_type, int num)
{
  if (num > 0) {
    size_t start[3], count[3];
    char * text = "";
    int    varid;
    size_t num_entity;
    size_t i;

    ex__get_dimension(exoid, ex__dim_num_objects(obj_type), ex_name_of_object(obj_type),
                      &num_entity, &varid, __func__);

    for (i = 0; i < num_entity; i++) {
      start[0] = i;
      count[0] = 1;

      start[1] = 0;
      count[1] = 1;

      start[2] = 0;
      count[2] = 1;

      nc_put_vara_text(exoid, varid, start, count, text);
    }
  }
}

static int ex_write_object_names(int exoid, const char *type, const char *dimension_name,
                                 int dimension_var, int string_dimension, int count)
{
  int  dim[2];
  int  status;
  int  varid;
  char errmsg[MAX_ERR_LENGTH];
#if NC_HAS_HDF5
  int fill = NC_FILL_CHAR;
#endif

  if (count > 0) {
    dim[0] = dimension_var;
    dim[1] = string_dimension;

    if ((status = nc_def_var(exoid, dimension_name, NC_CHAR, 2, dim, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s name array in file id %d", type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (status); /* exit define mode and return */
    }
#if NC_HAS_HDF5
    nc_def_var_fill(exoid, varid, 0, &fill);
#endif
  }
  return (NC_NOERR);
}

static int ex_write_object_params(int exoid, const char *type, const char *dimension_name,
                                  const char *status_dim_name, const char *id_array_dim_name,
                                  size_t count, int *dimension)
{
  int  dim[2];
  int  varid;
  int  status;
  int  int_type;
  char errmsg[MAX_ERR_LENGTH];

  /* Can have nonzero model->num_elem_blk even if model->num_elem == 0 */
  if (count > 0) {
    if ((status = nc_def_dim(exoid, dimension_name, count, dimension)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of %ss in file id %d", type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (status); /* exit define mode and return */
    }
    /* ...and some variables */
    /* element block id status array */
    dim[0] = *dimension;
    if ((status = nc_def_var(exoid, status_dim_name, NC_INT, 1, dim, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s status array in file id %d",
               type, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (status); /* exit define mode and return */
    }

    /* type id array */
    int_type = NC_INT;
    if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
      int_type = NC_INT64;
    }
    if ((status = nc_def_var(exoid, id_array_dim_name, int_type, 1, dim, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s id array in file id %d", type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (status); /* exit define mode and return */
    }

    /*   store property name as attribute of property array variable */
    if ((status = nc_put_att_text(exoid, varid, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s property name %s in file id %d",
               type, "ID", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (status);
    }
  }
  return (NC_NOERR);
}

static int ex_write_map_params(int exoid, const char *map_name, const char *map_dim_name,
                               const char *map_id_name, size_t map_count, int *map_dimension)
{
  int  dim[2];
  int  varid;
  int  status;
  char errmsg[MAX_ERR_LENGTH];

  int int_type = NC_INT;
  if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
    int_type = NC_INT64;
  }

  /* Can have nonzero model->num_XXXX_map even if model->num_XXXX == 0 */
  if ((map_count) > 0) {
    if ((status = nc_def_dim(exoid, map_dim_name, map_count, map_dimension)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of %ss in file id %d",
               map_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (status); /* exit define mode and return */
    }

    dim[0] = *map_dimension;

    /* map_name id array */
    if ((status = nc_def_var(exoid, map_id_name, int_type, 1, dim, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s id array in file id %d",
               map_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (status); /* exit define mode and return */
    }

    /*   store property name as attribute of property array variable */
    if ((status = nc_put_att_text(exoid, varid, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s property name %s in file id %d",
               map_name, "ID", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
    }
  }
  return (NC_NOERR);
}

static void invalidate_id_status(int exoid, const char *var_stat, const char *var_id, int count,
                                 int *ids)
{
  int i;
  int id_var, stat_var;

  if (count > 0) {
    if (var_id != 0) {
      for (i = 0; i < count; i++) {
        ids[i] = EX_INVALID_ID;
      }
      (void)nc_inq_varid(exoid, var_id, &id_var);
      (void)nc_put_var_int(exoid, id_var, ids);
    }

    if (var_stat != 0) {
      for (i = 0; i < count; i++) {
        ids[i] = 0;
      }

      (void)nc_inq_varid(exoid, var_stat, &stat_var);
      (void)nc_put_var_int(exoid, stat_var, ids);
    }
  }
}

/*!
\ingroup ModelDescription

 * writes the initialization parameters to the EXODUS file
 * \param     exoid     exodus file id
 * \param     model     finite element model parameters
 */

int ex_put_init_ext(int exoid, const ex_init_params *model)
{
  int numdimdim, numnoddim, elblkdim, edblkdim, fablkdim, esetdim, fsetdim, elsetdim, nsetdim,
      ssetdim, dim_str_name, dim[2], temp;
  int nmapdim, edmapdim, famapdim, emapdim, timedim;
  int status;
  int title_len;
#if 0
  /* used for header size calculations which are turned off for now */
  int header_size, fixed_var_size, iows;
#endif
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  int rootid = exoid & EX_FILE_ID_MASK;

  ex__check_valid_file_id(exoid, __func__);

  if (rootid == exoid && nc_inq_dimid(exoid, DIM_NUM_DIM, &temp) == NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: initialization already done for file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* put file into define mode */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* define some attributes... */
  title_len = strlen(model->title) < MAX_LINE_LENGTH ? strlen(model->title) : MAX_LINE_LENGTH;
  if ((status = nc_put_att_text(rootid, NC_GLOBAL, (const char *)ATT_TITLE, title_len + 1,
                                model->title)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define model->title attribute to file id %d",
             rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret; /* exit define mode and return */
  }

  /* ...and some dimensions... */

  /* create name string length dimension */
  if (nc_inq_dimid(rootid, DIM_STR_NAME, &dim_str_name) != NC_NOERR) {
    if ((status = nc_def_dim(rootid, DIM_STR_NAME, NC_MAX_NAME, &dim_str_name)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define name string length in file id %d",
               rootid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret;
    }
  }

  if ((status = nc_def_dim(exoid, DIM_TIME, NC_UNLIMITED, &timedim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define time dimension in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  dim[0] = timedim;
  if ((status = nc_def_var(exoid, VAR_WHOLE_TIME, nc_flt_code(exoid), 1, dim, &temp)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to define whole time step variable in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }
  {
    struct ex__file_item *file = ex__find_file_item(exoid);
    file->time_varid           = temp;
  }
  ex__compress_variable(exoid, temp, 2);

  if (model->num_dim > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_DIM, model->num_dim, &numdimdim)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of dimensions in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  /*
   * Need to handle "empty file" that may be the result of a strange
   * load balance or some other strange run.  Note that if num_node
   * == 0, then model->num_elem must be zero since you cannot have elements
   * with no nodes. It *is* permissible to have zero elements with
   * non-zero node count.
   */

  if (model->num_nodes > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_NODES, model->num_nodes, &numnoddim)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of nodes in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  if (model->num_elem > 0) {
    if (model->num_nodes <= 0) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Cannot have non-zero element count if node count "
               "is zero.in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret; /* exit define mode and return */
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_ELEM, model->num_elem, &temp)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of elements in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  if (model->num_edge > 0) {
    if (model->num_nodes <= 0) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Cannot have non-zero edge count if node count is "
               "zero.in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret; /* exit define mode and return */
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_EDGE, model->num_edge, &temp)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of edges in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  if (model->num_face > 0) {
    if (model->num_nodes <= 0) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Cannot have non-zero face count if node count is "
               "zero.in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret; /* exit define mode and return */
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_FACE, model->num_face, &temp)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of faces in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  if (ex_write_object_params(exoid, "element block", DIM_NUM_EL_BLK, VAR_STAT_EL_BLK, VAR_ID_EL_BLK,
                             model->num_elem_blk, &elblkdim)) {
    goto error_ret;
  }
  if (ex_write_object_params(exoid, "edge block", DIM_NUM_ED_BLK, VAR_STAT_ED_BLK, VAR_ID_ED_BLK,
                             model->num_edge_blk, &edblkdim)) {
    goto error_ret;
  }
  if (ex_write_object_params(exoid, "face block", DIM_NUM_FA_BLK, VAR_STAT_FA_BLK, VAR_ID_FA_BLK,
                             model->num_face_blk, &fablkdim)) {
    goto error_ret;
  }

  if (ex_write_object_params(exoid, "node set", DIM_NUM_NS, VAR_NS_STAT, VAR_NS_IDS,
                             model->num_node_sets, &nsetdim)) {
    goto error_ret;
  }
  if (ex_write_object_params(exoid, "edge set", DIM_NUM_ES, VAR_ES_STAT, VAR_ES_IDS,
                             model->num_edge_sets, &esetdim)) {
    goto error_ret;
  }
  if (ex_write_object_params(exoid, "face set", DIM_NUM_FS, VAR_FS_STAT, VAR_FS_IDS,
                             model->num_face_sets, &fsetdim)) {
    goto error_ret;
  }
  if (ex_write_object_params(exoid, "side set", DIM_NUM_SS, VAR_SS_STAT, VAR_SS_IDS,
                             model->num_side_sets, &ssetdim)) {
    goto error_ret;
  }
  if (ex_write_object_params(exoid, "elem set", DIM_NUM_ELS, VAR_ELS_STAT, VAR_ELS_IDS,
                             model->num_elem_sets, &elsetdim)) {
    goto error_ret;
  }

  if (ex_write_map_params(exoid, "node map", DIM_NUM_NM, VAR_NM_PROP(1), model->num_node_maps,
                          &nmapdim) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_map_params(exoid, "edge map", DIM_NUM_EDM, VAR_EDM_PROP(1), model->num_edge_maps,
                          &edmapdim) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_map_params(exoid, "face map", DIM_NUM_FAM, VAR_FAM_PROP(1), model->num_face_maps,
                          &famapdim) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_map_params(exoid, "element map", DIM_NUM_EM, VAR_EM_PROP(1), model->num_elem_maps,
                          &emapdim) != NC_NOERR) {
    goto error_ret;
  }

  if (model->num_nodes > 0) {
    dim[0] = numnoddim;
    if (model->num_dim > 0) {
      if ((status = nc_def_var(exoid, VAR_COORD_X, nc_flt_code(exoid), 1, dim, &temp)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define node x coordinate array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      ex__compress_variable(exoid, temp, 2);
    }

    if (model->num_dim > 1) {
      if ((status = nc_def_var(exoid, VAR_COORD_Y, nc_flt_code(exoid), 1, dim, &temp)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define node y coordinate array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      ex__compress_variable(exoid, temp, 2);
    }

    if (model->num_dim > 2) {
      if ((status = nc_def_var(exoid, VAR_COORD_Z, nc_flt_code(exoid), 1, dim, &temp)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define node z coordinate array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      ex__compress_variable(exoid, temp, 2);
    }
  }

  if (ex_write_object_names(exoid, "element block", VAR_NAME_EL_BLK, elblkdim, dim_str_name,
                            model->num_elem_blk) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "edge block", VAR_NAME_ED_BLK, edblkdim, dim_str_name,
                            model->num_edge_blk) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "face block", VAR_NAME_FA_BLK, fablkdim, dim_str_name,
                            model->num_face_blk) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "node set", VAR_NAME_NS, nsetdim, dim_str_name,
                            model->num_node_sets) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "edge set", VAR_NAME_ES, esetdim, dim_str_name,
                            model->num_edge_sets) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "face set", VAR_NAME_FS, fsetdim, dim_str_name,
                            model->num_face_sets) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "side set", VAR_NAME_SS, ssetdim, dim_str_name,
                            model->num_side_sets) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "element set", VAR_NAME_ELS, elsetdim, dim_str_name,
                            model->num_elem_sets) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "node map", VAR_NAME_NM, nmapdim, dim_str_name,
                            model->num_node_maps) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "edge map", VAR_NAME_EDM, edmapdim, dim_str_name,
                            model->num_edge_maps) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "face map", VAR_NAME_FAM, famapdim, dim_str_name,
                            model->num_face_maps) != NC_NOERR) {
    goto error_ret;
  }
  if (ex_write_object_names(exoid, "element map", VAR_NAME_EM, emapdim, dim_str_name,
                            model->num_elem_maps) != NC_NOERR) {
    goto error_ret;
  }
  if (model->num_dim > 0) {
    if (ex_write_object_names(exoid, "coordinate", VAR_NAME_COOR, numdimdim, dim_str_name,
                              model->num_dim) != NC_NOERR) {
      goto error_ret;
    }
  }

  /* leave define mode */
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Fill the id and status arrays with EX_INVALID_ID */
  {
    int *  invalid_ids = NULL;
    size_t maxset      = model->num_elem_blk;
    if (maxset < model->num_edge_blk) {
      maxset = model->num_edge_blk;
    }
    if (maxset < model->num_face_blk) {
      maxset = model->num_face_blk;
    }
    if (maxset < model->num_node_sets) {
      maxset = model->num_node_sets;
    }
    if (maxset < model->num_edge_sets) {
      maxset = model->num_edge_sets;
    }
    if (maxset < model->num_face_sets) {
      maxset = model->num_face_sets;
    }
    if (maxset < model->num_side_sets) {
      maxset = model->num_side_sets;
    }
    if (maxset < model->num_elem_sets) {
      maxset = model->num_elem_sets;
    }
    if (maxset < model->num_node_maps) {
      maxset = model->num_node_maps;
    }
    if (maxset < model->num_edge_maps) {
      maxset = model->num_edge_maps;
    }
    if (maxset < model->num_face_maps) {
      maxset = model->num_face_maps;
    }
    if (maxset < model->num_elem_maps) {
      maxset = model->num_elem_maps;
    }

    /* allocate space for id/status array */
    if (!(invalid_ids = malloc(maxset * sizeof(int)))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate memory for id/status array for file id %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    invalidate_id_status(exoid, VAR_STAT_EL_BLK, VAR_ID_EL_BLK, model->num_elem_blk, invalid_ids);
    invalidate_id_status(exoid, VAR_STAT_ED_BLK, VAR_ID_ED_BLK, model->num_edge_blk, invalid_ids);
    invalidate_id_status(exoid, VAR_STAT_FA_BLK, VAR_ID_FA_BLK, model->num_face_blk, invalid_ids);
    invalidate_id_status(exoid, VAR_NS_STAT, VAR_NS_IDS, model->num_node_sets, invalid_ids);
    invalidate_id_status(exoid, VAR_ES_STAT, VAR_ES_IDS, model->num_edge_sets, invalid_ids);
    invalidate_id_status(exoid, VAR_FS_STAT, VAR_FS_IDS, model->num_face_sets, invalid_ids);
    invalidate_id_status(exoid, VAR_SS_STAT, VAR_SS_IDS, model->num_side_sets, invalid_ids);
    invalidate_id_status(exoid, VAR_ELS_STAT, VAR_ELS_IDS, model->num_elem_sets, invalid_ids);

    invalidate_id_status(exoid, 0, VAR_NM_PROP(1), model->num_node_maps, invalid_ids);
    invalidate_id_status(exoid, 0, VAR_EDM_PROP(1), model->num_edge_maps, invalid_ids);
    invalidate_id_status(exoid, 0, VAR_FAM_PROP(1), model->num_face_maps, invalid_ids);
    invalidate_id_status(exoid, 0, VAR_EM_PROP(1), model->num_elem_maps, invalid_ids);

    free(invalid_ids);
    invalid_ids = NULL;
  }

  /* Write dummy values to the names arrays to avoid corruption issues on some
   * platforms */
  write_dummy_names(exoid, EX_ELEM_BLOCK, model->num_elem_blk);
  write_dummy_names(exoid, EX_EDGE_BLOCK, model->num_edge_blk);
  write_dummy_names(exoid, EX_FACE_BLOCK, model->num_face_blk);
  write_dummy_names(exoid, EX_NODE_SET, model->num_node_sets);
  write_dummy_names(exoid, EX_EDGE_SET, model->num_edge_sets);
  write_dummy_names(exoid, EX_FACE_SET, model->num_face_sets);
  write_dummy_names(exoid, EX_SIDE_SET, model->num_side_sets);
  write_dummy_names(exoid, EX_ELEM_SET, model->num_elem_sets);
  write_dummy_names(exoid, EX_NODE_MAP, model->num_node_maps);
  write_dummy_names(exoid, EX_EDGE_MAP, model->num_edge_maps);
  write_dummy_names(exoid, EX_FACE_MAP, model->num_face_maps);
  write_dummy_names(exoid, EX_ELEM_MAP, model->num_elem_maps);

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

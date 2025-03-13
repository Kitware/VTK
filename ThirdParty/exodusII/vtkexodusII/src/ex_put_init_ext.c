/*
 * Copyright(C) 1999-2025 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
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
    int    varid;
    size_t num_entity;
    exi_get_dimension(exoid, exi_dim_num_objects(obj_type), ex_name_of_object(obj_type),
                      &num_entity, &varid, __func__);

    char *text = "";
    for (size_t i = 0; i < num_entity; i++) {
      size_t start[] = {i, 0, 0};
      size_t count[] = {1, 1, 1};
      nc_put_vara_text(exoid, varid, start, count, text);
    }
  }
}

static int ex_write_object_names(int exoid, const char *type, const char *dimension_name,
                                 int dimension_var, int string_dimension, int count)
{

  if (count > 0) {
    int dim[] = {dimension_var, string_dimension};
    int status;
    int varid;
    if ((status = nc_def_var(exoid, dimension_name, NC_CHAR, 2, dim, &varid)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s name array in file id %d", type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return status; /* exit define mode and return */
    }
    exi_set_compact_storage(exoid, varid);
#if defined(EX_CAN_USE_NC_DEF_VAR_FILL)
    int fill = NC_FILL_CHAR;
    nc_def_var_fill(exoid, varid, 0, &fill);
#endif
  }
  return EX_NOERR;
}

static int ex_write_object_params(int exoid, const char *type, const char *dimension_name,
                                  const char *status_dim_name, const char *id_array_dim_name,
                                  size_t count, int *dimension)
{

  /* Can have nonzero model->num_elem_blk even if model->num_elem == 0 */
  if (count > 0) {
    int status;
    if ((status = nc_def_dim(exoid, dimension_name, count, dimension)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of %ss in file id %d", type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return status; /* exit define mode and return */
    }
    /* ...and some variables */
    /* element block id status array */
    int dim[2];
    int varid;
    dim[0] = *dimension;
    if ((status = nc_def_var(exoid, status_dim_name, NC_INT, 1, dim, &varid)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s status array in file id %d",
               type, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return status; /* exit define mode and return */
    }

    size_t sixty_four_kb = 64 * 1024; // Compact storage can only be used for < 64KiByte data sizes
    if (4 * count < sixty_four_kb) {
      exi_set_compact_storage(exoid, varid);
    }

    /* type id array */
    int int_type = NC_INT;
    int int_size = 4;
    if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
      int_type = NC_INT64;
      int_size = 8;
    }

    if ((status = nc_def_var(exoid, id_array_dim_name, int_type, 1, dim, &varid)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s id array in file id %d", type,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return status; /* exit define mode and return */
    }
    if (int_size * count < sixty_four_kb) {
      exi_set_compact_storage(exoid, varid);
    }

    /*   store property name as attribute of property array variable */
    if ((status = nc_put_att_text(exoid, varid, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s property name %s in file id %d",
               type, "ID", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return status;
    }
  }
  return EX_NOERR;
}

static int ex_write_map_params(int exoid, const char *map_name, const char *map_dim_name,
                               const char *map_id_name, size_t map_count, int *map_dimension)
{

  int int_type = NC_INT;
  if (ex_int64_status(exoid) & EX_IDS_INT64_DB) {
    int_type = NC_INT64;
  }

  /* Can have nonzero model->num_XXXX_map even if model->num_XXXX == 0 */
  if ((map_count) > 0) {
    int status;
    if ((status = nc_def_dim(exoid, map_dim_name, map_count, map_dimension)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of %ss in file id %d",
               map_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return status; /* exit define mode and return */
    }

    int dim[] = {*map_dimension};

    /* map_name id array */
    int varid;
    if ((status = nc_def_var(exoid, map_id_name, int_type, 1, dim, &varid)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s id array in file id %d",
               map_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return status; /* exit define mode and return */
    }

    /*   store property name as attribute of property array variable */
    if ((status = nc_put_att_text(exoid, varid, ATT_PROP_NAME, 3, "ID")) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s property name %s in file id %d",
               map_name, "ID", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return EX_FATAL;
    }
  }
  return EX_NOERR;
}

static void invalidate_id_status(int exoid, const char *var_stat, const char *var_id, int count,
                                 int *ids)
{
  if (count > 0) {
    if (var_id != NULL) {
      for (int i = 0; i < count; i++) {
        ids[i] = EX_INVALID_ID;
      }
      int id_var;
      (void)nc_inq_varid(exoid, var_id, &id_var);
      (void)nc_put_var_int(exoid, id_var, ids);
    }

    if (var_stat != NULL) {
      for (int i = 0; i < count; i++) {
        ids[i] = 0;
      }

      int stat_var;
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
  int numdimdim    = 0;
  int numnoddim    = 0;
  int elblkdim     = 0;
  int edblkdim     = 0;
  int fablkdim     = 0;
  int esetdim      = 0;
  int fsetdim      = 0;
  int elsetdim     = 0;
  int nsetdim      = 0;
  int ssetdim      = 0;
  int dim_str_name = 0;
  int dim[2];
  int temp      = 0;
  int nmapdim   = 0;
  int edmapdim  = 0;
  int famapdim  = 0;
  int emapdim   = 0;
  int timedim   = 0;
  int status    = 0;
  int title_len = 0;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  int rootid = exoid & EX_FILE_ID_MASK;

  if (rootid == exoid && nc_inq_dimid(exoid, DIM_NUM_DIM, &temp) == NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: initialization already done for file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* put file into define mode */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* define some attributes... */
  title_len = strlen(model->title) < MAX_LINE_LENGTH ? strlen(model->title) : MAX_LINE_LENGTH;
  if ((status = nc_put_att_text(rootid, NC_GLOBAL, (const char *)ATT_TITLE, title_len + 1,
                                model->title)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define model->title attribute to file id %d",
             rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret; /* exit define mode and return */
  }

  /* ...and some dimensions... */

  /* create name string length dimension */
  if (nc_inq_dimid(rootid, DIM_STR_NAME, &dim_str_name) != NC_NOERR) {
    if ((status = nc_def_dim(rootid, DIM_STR_NAME, EX_MAX_NAME, &dim_str_name)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define name string length in file id %d",
               rootid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret;
    }
  }

  if ((status = nc_def_dim(exoid, DIM_TIME, NC_UNLIMITED, &timedim)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define time dimension in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  dim[0] = timedim;
  if ((status = nc_def_var(exoid, VAR_WHOLE_TIME, nc_flt_code(exoid), 1, dim, &temp)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to define whole time step variable in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  {
    struct exi_file_item *file = exi_find_file_item(exoid);
    if (!file) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file id %d.", exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADFILEID);
      goto error_ret;
    }
    file->time_varid = temp;
  }

  exi_compress_variable(exoid, temp, -2); /* Don't compress, but do set collective io */

  if (model->num_dim > 0) {
    if ((status = nc_def_dim(exoid, DIM_NUM_DIM, model->num_dim, &numdimdim)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
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

  int    int_size = ex_int64_status(exoid) & EX_IDS_INT64_DB ? 8 : 4;
  size_t twoGiB   = 1ul << 31;

  if (model->num_nodes > 0) {
    // If file is using 32-bit integers, check what node count is in range...
    if (int_size == 4 && model->num_nodes >= twoGiB) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: File is using 32-bit integers, but the node count exceeds the integer "
               "capacity (%" PRId64 ") in file id %d",
               model->num_nodes, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret;
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_NODES, model->num_nodes, &numnoddim)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of nodes in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  if (model->num_elem > 0) {
    if (model->num_nodes <= 0) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Cannot have non-zero element count if node count "
               "is zero.in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret; /* exit define mode and return */
    }

    if (int_size == 4 && model->num_elem >= twoGiB) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: File is using 32-bit integers, but the element count exceeds the integer "
               "capacity (%" PRId64 ") in file id %d",
               model->num_elem, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret;
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_ELEM, model->num_elem, &temp)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of elements in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  if (model->num_edge > 0) {
    if (model->num_nodes <= 0) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Cannot have non-zero edge count if node count is "
               "zero.in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret; /* exit define mode and return */
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_EDGE, model->num_edge, &temp)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define number of edges in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  if (model->num_face > 0) {
    if (model->num_nodes <= 0) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Cannot have non-zero face count if node count is "
               "zero.in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret; /* exit define mode and return */
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_FACE, model->num_face, &temp)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
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
        char errmsg[MAX_ERR_LENGTH];
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define node x coordinate array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      exi_compress_variable(exoid, temp, 2);
    }

    if (model->num_dim > 1) {
      if ((status = nc_def_var(exoid, VAR_COORD_Y, nc_flt_code(exoid), 1, dim, &temp)) !=
          NC_NOERR) {
        char errmsg[MAX_ERR_LENGTH];
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define node y coordinate array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      exi_compress_variable(exoid, temp, 2);
    }

    if (model->num_dim > 2) {
      if ((status = nc_def_var(exoid, VAR_COORD_Z, nc_flt_code(exoid), 1, dim, &temp)) !=
          NC_NOERR) {
        char errmsg[MAX_ERR_LENGTH];
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define node z coordinate array in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      exi_compress_variable(exoid, temp, 2);
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
  if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Fill the id and status arrays with EX_INVALID_ID */
  {
    int    *invalid_ids = NULL;
    int64_t maxset      = model->num_elem_blk;
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
      char errmsg[MAX_ERR_LENGTH];
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
  exi_leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

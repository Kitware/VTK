/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgini - ex_get_init
 *
 * entry conditions -
 *   input parameters:
 *       int             exoid           exodus file id
 *
 * exit conditions -
 *       ex_init_params* info            parameter information
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_init_params, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

static void exi_get_entity_count(int exoid, ex_init_params *info)
{
  int ndims;
  nc_inq(exoid, &ndims, NULL, NULL, NULL);
  for (int dimid = 0; dimid < ndims; dimid++) {
    char   dim_nm[NC_MAX_NAME + 1] = {'\0'};
    size_t dim_sz;
    nc_inq_dim(exoid, dimid, dim_nm, &dim_sz);
    /* For assemblies, we check for a dim starting with "num_entity_assembly" */
    if (strncmp(dim_nm, "num_entity_assembly", 19) == 0) {
      info->num_assembly++;
    }
    else if (strncmp(dim_nm, "num_values_blob", 15) == 0) {
      info->num_blob++;
    }
  }
}

/* Used to reduce repeated code below */
static int ex_get_dim_value(int exoid, const char *name, const char *dimension_name, int dimension,
                            int64_t *value)
{
  if (nc_inq_dimid(exoid, dimension_name, &dimension) != NC_NOERR) {
    /* optional and default to zero. */
    *value = 0;
  }
  else {
    size_t tmp;
    int    status;
    if ((status = nc_inq_dimlen(exoid, dimension, &tmp)) != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %s in file id %d", name,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
    }
    *value = tmp;
  }
  return (EX_NOERR);
}

/*!
 * \ingroup ModelDescription
 * reads the initialization parameters from an opened EXODUS file
 * \param exoid exodus file id
 * \param[out] info #ex_init_params structure containing metadata for mesh.
 * \sa ex_get_init()
 */

int ex_get_init_ext(int exoid, ex_init_params *info)
{
  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  info->num_dim       = 0;
  info->num_nodes     = 0;
  info->num_edge      = 0;
  info->num_edge_blk  = 0;
  info->num_face      = 0;
  info->num_face_blk  = 0;
  info->num_elem      = 0;
  info->num_elem_blk  = 0;
  info->num_node_sets = 0;
  info->num_edge_sets = 0;
  info->num_face_sets = 0;
  info->num_side_sets = 0;
  info->num_elem_sets = 0;
  info->num_node_maps = 0;
  info->num_edge_maps = 0;
  info->num_face_maps = 0;
  info->num_elem_maps = 0;
  info->num_assembly  = 0;
  info->num_blob      = 0;

  int dimid = 0;
  if (ex_get_dim_value(exoid, "dimension count", DIM_NUM_DIM, dimid, &info->num_dim) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "nodes", DIM_NUM_NODES, dimid, &info->num_nodes) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Counts for assemblies and blobs */
  exi_get_entity_count(exoid, info);

  if (ex_get_dim_value(exoid, "edges", DIM_NUM_EDGE, dimid, &info->num_edge) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "faces", DIM_NUM_FACE, dimid, &info->num_face) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "elements", DIM_NUM_ELEM, dimid, &info->num_elem) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "element blocks", DIM_NUM_EL_BLK, dimid, &info->num_elem_blk) !=
      EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (info->num_elem_blk == 0 && info->num_elem > 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate number of element blocks in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "node sets", DIM_NUM_NS, dimid, &info->num_node_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "edge sets", DIM_NUM_ES, dimid, &info->num_edge_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "face sets", DIM_NUM_FS, dimid, &info->num_face_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "side sets", DIM_NUM_SS, dimid, &info->num_side_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "elem sets", DIM_NUM_ELS, dimid, &info->num_elem_sets) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "node maps", DIM_NUM_NM, dimid, &info->num_node_maps) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "edge maps", DIM_NUM_EDM, dimid, &info->num_edge_maps) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "face maps", DIM_NUM_FAM, dimid, &info->num_face_maps) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "elem maps", DIM_NUM_EM, dimid, &info->num_elem_maps) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Edge and face blocks are also optional (for backwards compatibility) */
  if (ex_get_dim_value(exoid, "edge blocks", DIM_NUM_ED_BLK, dimid, &info->num_edge_blk) !=
      EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ex_get_dim_value(exoid, "face blocks", DIM_NUM_FA_BLK, dimid, &info->num_face_blk) !=
      EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int     rootid = exoid & EX_FILE_ID_MASK;
  int     status;
  size_t  title_len  = 0;
  nc_type title_type = 0;
  if ((status = nc_inq_att(rootid, NC_GLOBAL, ATT_TITLE, &title_type, &title_len)) != NC_NOERR) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no title in file id %d", rootid);
    ex_err_fn(exoid, __func__, errmsg, status);
  }

  /* Check title length to avoid overrunning clients memory space; include
   * trailing null */
  if (title_len > 0) {
    if (title_len > MAX_LINE_LENGTH) {
      char *title = malloc(title_len + 1);
      if ((status = nc_get_att_text(rootid, NC_GLOBAL, ATT_TITLE, title)) == NC_NOERR) {
        ex_copy_string(info->title, title, MAX_LINE_LENGTH + 1);
        info->title[MAX_LINE_LENGTH] = '\0';
      }
      free(title);
    }
    else {
      status                 = nc_get_att_text(rootid, NC_GLOBAL, ATT_TITLE, info->title);
      info->title[title_len] = '\0';
    }
    if (status != NC_NOERR) {
      char errmsg[MAX_ERR_LENGTH];
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get title in file id %d", rootid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    info->title[0] = '\0';
  }

  /* Update settings in exi_file_item struct */
  {
    struct exi_file_item *file = exi_find_file_item(exoid);
    if (file) {
      file->has_nodes      = info->num_nodes > 0;
      file->has_edges      = info->num_edge > 0;
      file->has_faces      = info->num_face > 0;
      file->has_elems      = info->num_elem > 0;
      file->assembly_count = info->num_assembly;
      file->blob_count     = info->num_blob;
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

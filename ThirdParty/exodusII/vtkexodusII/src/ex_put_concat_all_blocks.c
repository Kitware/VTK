/*
 * Copyright(C) 1999-2020, 2022, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expclb - ex_put_concat_all_blocks: write elem, edge, & face block parameters
 *
 * entry conditions -
 *   input parameters:
 *       int                    exoid          exodus file id
 *       const ex_block_params* bparam         block parameters structure
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * writes the parameters used to describe all element, edge, and face blocks
 * \param     exoid          exodus file id
 * \param     param         block parameters structure
 */
int ex_put_concat_all_blocks(int exoid, const ex_block_params *param)
{
  int    varid, dimid, dims[2], strdim, *eb_stat, *ed_stat, *fa_stat;
  int    temp;
  size_t iblk;
  size_t i;
  int    status;
  size_t num_elem_blk = 0;
  size_t num_edge_blk = 0;
  size_t num_face_blk = 0;
  int    cur_num_elem_blk, nelnoddim, numelbdim, numattrdim, connid = -1;
  int    cur_num_edge_blk, numedbdim, nednoddim, cur_num_face_blk, numfabdim, nfanoddim;
  int    neledgdim = -1, nelfacdim = -1;
  char   errmsg[MAX_ERR_LENGTH];
  int    elem_work = 0; /* is DIM_NUM_EL_BLK defined? If so, there's work to do */
  int    edge_work = 0; /* is DIM_NUM_ED_BLK defined? If so, there's work to do */
  int    face_work = 0; /* is DIM_NUM_FA_BLK defined? If so, there's work to do */

  int *edge_id_int = NULL;
  int *face_id_int = NULL;
  int *elem_id_int = NULL;

  int64_t *edge_id_int64 = NULL;
  int64_t *face_id_int64 = NULL;
  int64_t *elem_id_int64 = NULL;

  int ids_int64 = ex_int64_status(exoid) & EX_IDS_INT64_API;

  static const char *dim_num_maps[] = {
      DIM_NUM_NM,
      DIM_NUM_EDM,
      DIM_NUM_FAM,
      DIM_NUM_EM,
  };
  static const char *dim_size_maps[] = {
      DIM_NUM_NODES,
      DIM_NUM_EDGE,
      DIM_NUM_FACE,
      DIM_NUM_ELEM,
  };
  static const ex_entity_type map_enums[] = {EX_NODE_MAP, EX_EDGE_MAP, EX_FACE_MAP, EX_ELEM_MAP};

  /* If param->define_maps is true, we must fill these with values from
     ex_put_init_ext
     before entering define mode */
  size_t num_maps[sizeof(dim_num_maps) / sizeof(dim_num_maps[0])];
  size_t num_map_dims = sizeof(dim_num_maps) / sizeof(dim_num_maps[0]);

  EX_FUNC_ENTER();

  if (ids_int64) {
    edge_id_int64 = param->edge_blk_id;
    face_id_int64 = param->face_blk_id;
    elem_id_int64 = param->elem_blk_id;
  }
  else {
    edge_id_int = param->edge_blk_id;
    face_id_int = param->face_blk_id;
    elem_id_int = param->elem_blk_id;
  }

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire previously defined dimensions  */
  if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &strdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get string length in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (param->define_maps) {
    for (i = 0; i < num_map_dims; ++i) {
      if ((status = nc_inq_dimid(exoid, dim_num_maps[i], &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find node map size of file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
      if ((status = nc_inq_dimlen(exoid, dimid, num_maps + i)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to retrieve node map size of file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

#define EX_PREPARE_BLOCK(TNAME, WNAME, DNUMNAME, VSTATNAME, VIDNAME, LNUMNAME, SNUMNAME, SIDNAME,  \
                         GSTAT)                                                                    \
  /* first check if any TNAME blocks are specified                                                 \
   * OK if zero...                                                                                 \
   */                                                                                              \
  if ((status = (nc_inq_dimid(exoid, DNUMNAME, &dimid))) == NC_NOERR) {                            \
    WNAME = 1;                                                                                     \
                                                                                                   \
    /* Get number of TNAME blocks defined for this file */                                         \
    if ((status = nc_inq_dimlen(exoid, dimid, &LNUMNAME)) != NC_NOERR) {                           \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to get number of " TNAME " blocks in file id %d", exoid);            \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      EX_FUNC_LEAVE(EX_FATAL);                                                                     \
    }                                                                                              \
                                                                                                   \
    /* Fill out the TNAME block status array */                                                    \
    if (!(GSTAT = malloc(LNUMNAME * sizeof(int)))) {                                               \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to allocate space for " TNAME " block status array in file id %d",   \
               exoid);                                                                             \
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);                                              \
      EX_FUNC_LEAVE(EX_FATAL);                                                                     \
    }                                                                                              \
                                                                                                   \
    for (i = 0; i < LNUMNAME; i++) {                                                               \
      if (SNUMNAME[i] == 0) /* Is this a NULL TNAME block? */                                      \
        GSTAT[i] = 0;       /* change TNAME block status to NULL */                                \
      else                                                                                         \
        GSTAT[i] = 1; /* change TNAME block status to TRUE */                                      \
    }                                                                                              \
                                                                                                   \
    /* Next, get variable id of status array */                                                    \
    if ((status = nc_inq_varid(exoid, VSTATNAME, &varid)) != NC_NOERR) {                           \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to locate " TNAME " block status in file id %d", exoid);             \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      free(GSTAT);                                                                                 \
      EX_FUNC_LEAVE(EX_FATAL);                                                                     \
    }                                                                                              \
                                                                                                   \
    status = nc_put_var_int(exoid, varid, GSTAT);                                                  \
                                                                                                   \
    if (status != NC_NOERR) {                                                                      \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to store " TNAME " block status array to file id %d", exoid);        \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      free(GSTAT);                                                                                 \
      EX_FUNC_LEAVE(EX_FATAL);                                                                     \
    }                                                                                              \
                                                                                                   \
    free(GSTAT);                                                                                   \
                                                                                                   \
    /* Next, fill out ids array */                                                                 \
    /* first get id of ids array variable */                                                       \
    if ((status = nc_inq_varid(exoid, VIDNAME, &varid)) != NC_NOERR) {                             \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to locate " TNAME " block ids array in file id %d", exoid);          \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      EX_FUNC_LEAVE(EX_FATAL);                                                                     \
    }                                                                                              \
                                                                                                   \
    /* then, write out id list */                                                                  \
    if (ids_int64) {                                                                               \
      status = nc_put_var_longlong(exoid, varid, SIDNAME);                                         \
    }                                                                                              \
    else {                                                                                         \
      status = nc_put_var_int(exoid, varid, SIDNAME);                                              \
    }                                                                                              \
                                                                                                   \
    if (status != NC_NOERR) {                                                                      \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to store " TNAME " block id array in file id %d", exoid);            \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      EX_FUNC_LEAVE(EX_FATAL);                                                                     \
    }                                                                                              \
  }

  EX_PREPARE_BLOCK("element", elem_work, DIM_NUM_EL_BLK, VAR_STAT_EL_BLK, VAR_ID_EL_BLK,
                   num_elem_blk, param->num_elem_this_blk, param->elem_blk_id, eb_stat);
  EX_PREPARE_BLOCK("edge", edge_work, DIM_NUM_ED_BLK, VAR_STAT_ED_BLK, VAR_ID_ED_BLK, num_edge_blk,
                   param->num_edge_this_blk, param->edge_blk_id, ed_stat);
  EX_PREPARE_BLOCK("face", face_work, DIM_NUM_FA_BLK, VAR_STAT_FA_BLK, VAR_ID_FA_BLK, num_face_blk,
                   param->num_face_this_blk, param->face_blk_id, fa_stat);

  if (elem_work == 0 && edge_work == 0 && face_work == 0 && param->define_maps == 0) {
    /* Nothing to do. This is not an error, but we can save
     * ourselves from entering define mode by returning here.
     */
    EX_FUNC_LEAVE(EX_NOERR);
  }
  /* put netcdf file into define mode  */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to place file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

#if defined(EX_CAN_USE_NC_DEF_VAR_FILL)
#define EX_PREPARE_ATTRIB_ARRAY(TNAME, CURBLK, DNAME, DVAL, ID, VANAME, VADIM0, VADIM1, VANNAME)   \
  if (DVAL[iblk] > 0) {                                                                            \
    if ((status = nc_def_dim(exoid, DNAME(CURBLK + 1), DVAL[iblk], &VADIM1)) != NC_NOERR) {        \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to define number of attributes in " TNAME " block %" PRId64          \
               " in file id %d",                                                                   \
               ID, exoid);                                                                         \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      goto error_ret; /* exit define mode and return */                                            \
    }                                                                                              \
                                                                                                   \
    dims[0] = VADIM0;                                                                              \
    dims[1] = VADIM1;                                                                              \
                                                                                                   \
    if ((status = nc_def_var(exoid, VANAME(CURBLK + 1), nc_flt_code(exoid), 2, dims, &temp)) !=    \
        NC_NOERR) {                                                                                \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR:  failed to define attributes for " TNAME " block %" PRId64                  \
               " in file id %d",                                                                   \
               ID, exoid);                                                                         \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      goto error_ret; /* exit define mode and return */                                            \
    }                                                                                              \
    exi_compress_variable(exoid, temp, 2);                                                         \
                                                                                                   \
    /* Attribute names... */                                                                       \
    dims[0] = VADIM1;                                                                              \
    dims[1] = strdim;                                                                              \
                                                                                                   \
    if ((status = nc_def_var(exoid, VANNAME(CURBLK + 1), NC_CHAR, 2, dims, &temp)) != NC_NOERR) {  \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to define " TNAME " attribute name array in file id %d", exoid);     \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      goto error_ret; /* exit define mode and return */                                            \
    }                                                                                              \
    int fill = NC_FILL_CHAR;                                                                       \
    nc_def_var_fill(exoid, temp, 0, &fill);                                                        \
  }
#else
#define EX_PREPARE_ATTRIB_ARRAY(TNAME, CURBLK, DNAME, DVAL, ID, VANAME, VADIM0, VADIM1, VANNAME)   \
  if (DVAL[iblk] > 0) {                                                                            \
    if ((status = nc_def_dim(exoid, DNAME(CURBLK + 1), DVAL[iblk], &VADIM1)) != NC_NOERR) {        \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to define number of attributes in " TNAME " block %" PRId64          \
               " in file id %d",                                                                   \
               ID, exoid);                                                                         \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      goto error_ret; /* exit define mode and return */                                            \
    }                                                                                              \
                                                                                                   \
    dims[0] = VADIM0;                                                                              \
    dims[1] = VADIM1;                                                                              \
                                                                                                   \
    if ((status = nc_def_var(exoid, VANAME(CURBLK + 1), nc_flt_code(exoid), 2, dims, &temp)) !=    \
        NC_NOERR) {                                                                                \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR:  failed to define attributes for " TNAME " block %" PRId64                  \
               " in file id %d",                                                                   \
               ID, exoid);                                                                         \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      goto error_ret; /* exit define mode and return */                                            \
    }                                                                                              \
    exi_compress_variable(exoid, temp, 2);                                                         \
                                                                                                   \
    /* Attribute names... */                                                                       \
    dims[0] = VADIM1;                                                                              \
    dims[1] = strdim;                                                                              \
                                                                                                   \
    if ((status = nc_def_var(exoid, VANNAME(CURBLK + 1), NC_CHAR, 2, dims, &temp)) != NC_NOERR) {  \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to define " TNAME " attribute name array in file id %d", exoid);     \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      goto error_ret; /* exit define mode and return */                                            \
    }                                                                                              \
  }
#endif

#define EX_PREPARE_CONN(TNAME, BLK, BLKID, BLKSZ, VNAME, DNAME)                                    \
  if (DNAME > 0) {                                                                                 \
    int conn_int_type = NC_INT;                                                                    \
    if (ex_int64_status(exoid) & EX_BULK_INT64_DB) {                                               \
      conn_int_type = NC_INT64;                                                                    \
    }                                                                                              \
    dims[0] = BLKSZ;                                                                               \
    dims[1] = DNAME;                                                                               \
                                                                                                   \
    if ((status = nc_def_var(exoid, VNAME(BLK + 1), conn_int_type, 2, dims, &connid)) !=           \
        NC_NOERR) {                                                                                \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to create " TNAME " connectivity array for block %" PRId64           \
               " in file id %d",                                                                   \
               BLKID, exoid);                                                                      \
      ex_err_fn(exoid, __func__, errmsg, status);                                                  \
      goto error_ret; /* exit define mode and return */                                            \
    }                                                                                              \
    exi_compress_variable(exoid, connid, 1);                                                       \
  }

  /* Iterate over edge blocks ... */
  for (iblk = 0; iblk < num_edge_blk; ++iblk) {
    ex_entity_id eb_id;
    if (ids_int64) {
      eb_id = edge_id_int64[iblk];
    }
    else {
      eb_id = edge_id_int[iblk];
    }

    cur_num_edge_blk = exi_get_file_item(exoid, exi_get_counter_list(EX_EDGE_BLOCK));
    if (cur_num_edge_blk >= (int)num_edge_blk) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: exceeded number of edge blocks (%ld) defined in file id %d",
               (long)num_edge_blk, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret;
    }

    /* NOTE: exi_inc_file_item  is used to find the number of edge blocks
       for a specific file and returns that value incremented. */
    cur_num_edge_blk = exi_inc_file_item(exoid, exi_get_counter_list(EX_EDGE_BLOCK));

    if (param->num_edge_this_blk[iblk] == 0) { /* Is this a NULL edge block? */
      continue;
    }

    /* define some dimensions and variables*/
    if ((status = nc_def_dim(exoid, DIM_NUM_ED_IN_EBLK(cur_num_edge_blk + 1),
                             param->num_edge_this_blk[iblk], &numedbdim)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) { /* duplicate entry */
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: edge block %" PRId64 " already defined in file id %d", eb_id, exoid);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of edges/block for block %" PRId64 " file id %d",
                 eb_id, exoid);
      }
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_NOD_PER_ED(cur_num_edge_blk + 1),
                             param->num_nodes_per_edge[iblk], &nednoddim)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to define number of nodes/edge for block %" PRId64 " in file id %d",
               eb_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    /* edge attribute array */
    EX_PREPARE_ATTRIB_ARRAY("edge", cur_num_edge_blk, DIM_NUM_ATT_IN_EBLK, param->num_attr_edge,
                            eb_id, VAR_EATTRIB, numedbdim, numattrdim, VAR_NAME_EATTRIB);

    EX_PREPARE_CONN("edge block", cur_num_edge_blk, eb_id, numedbdim, VAR_EBCONN, nednoddim);

    /* store edge type as attribute of connectivity variable */
    if ((status = nc_put_att_text(exoid, connid, ATT_NAME_ELB, strlen(param->edge_type[iblk]) + 1,
                                  (void *)param->edge_type[iblk])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store edge type name %s in file id %d",
               param->edge_type[iblk], exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  /* Iterate over face blocks ... */
  for (iblk = 0; iblk < num_face_blk; ++iblk) {
    ex_entity_id fb_id;
    if (ids_int64) {
      fb_id = face_id_int64[iblk];
    }
    else {
      fb_id = face_id_int[iblk];
    }

    cur_num_face_blk = exi_get_file_item(exoid, exi_get_counter_list(EX_FACE_BLOCK));
    if (cur_num_face_blk >= (int)num_face_blk) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: exceeded number of face blocks (%ld) defined in file id %d",
               (long)num_face_blk, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret;
    }

    /* NOTE: exi_inc_file_item  is used to find the number of edge blocks
       for a specific file and returns that value incremented. */
    cur_num_face_blk = exi_inc_file_item(exoid, exi_get_counter_list(EX_FACE_BLOCK));

    if (param->num_face_this_blk[iblk] == 0) { /* Is this a NULL face block? */
      continue;
    }

    /* define some dimensions and variables*/
    if ((status = nc_def_dim(exoid, DIM_NUM_FA_IN_FBLK(cur_num_face_blk + 1),
                             param->num_face_this_blk[iblk], &numfabdim)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) { /* duplicate entry */
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: face block %" PRId64 " already defined in file id %d", fb_id, exoid);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of faces/block for block %" PRId64 " file id %d",
                 fb_id, exoid);
      }
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_NOD_PER_FA(cur_num_face_blk + 1),
                             param->num_nodes_per_face[iblk], &nfanoddim)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to define number of nodes/face for block %" PRId64 " in file id %d",
               fb_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    /* face attribute array */
    EX_PREPARE_ATTRIB_ARRAY("face", cur_num_face_blk, DIM_NUM_ATT_IN_FBLK, param->num_attr_face,
                            fb_id, VAR_FATTRIB, numfabdim, numattrdim, VAR_NAME_FATTRIB);

    EX_PREPARE_CONN("face block", cur_num_face_blk, fb_id, numfabdim, VAR_FBCONN, nfanoddim);

    /* store face type as attribute of connectivity variable */
    if ((status = nc_put_att_text(exoid, connid, ATT_NAME_ELB, strlen(param->face_type[iblk]) + 1,
                                  (void *)param->face_type[iblk])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store face type name %s in file id %d",
               param->face_type[iblk], exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
  }

  /* Iterate over element blocks ... */
  for (iblk = 0; iblk < num_elem_blk; ++iblk) {
    ex_entity_id eb_id;
    if (ids_int64) {
      eb_id = elem_id_int64[iblk];
    }
    else {
      eb_id = elem_id_int[iblk];
    }

    cur_num_elem_blk = exi_get_file_item(exoid, exi_get_counter_list(EX_ELEM_BLOCK));
    if (cur_num_elem_blk >= (int)num_elem_blk) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: exceeded number of element blocks (%ld) defined "
               "in file id %d",
               (long)num_elem_blk, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret;
    }

    /* NOTE: exi_inc_file_item  is used to find the number of element blocks
       for a specific file and returns that value incremented. */
    cur_num_elem_blk = exi_inc_file_item(exoid, exi_get_counter_list(EX_ELEM_BLOCK));

    if (param->num_elem_this_blk[iblk] == 0) { /* Is this a NULL element block? */
      continue;
    }

    /* define some dimensions and variables*/
    if ((status = nc_def_dim(exoid, DIM_NUM_EL_IN_BLK(cur_num_elem_blk + 1),
                             param->num_elem_this_blk[iblk], &numelbdim)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) { /* duplicate entry */
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: element block %" PRId64 " already defined in file id %d", eb_id, exoid);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of elements/block for "
                 "block %" PRId64 " file id %d",
                 eb_id, exoid);
      }
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    /* Always define DIM_NUM_NOD_PER_EL, even if zero.
     * Do not define DIM_NUM_EDG_PER_EL or DIM_NUM_FAC_PER_EL unless > 0.
     */
    if ((status = nc_def_dim(exoid, DIM_NUM_NOD_PER_EL(cur_num_elem_blk + 1),
                             param->num_nodes_per_elem[iblk], &nelnoddim)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to define number of nodes/element for block %" PRId64
               " in file id %d",
               eb_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    if (param->num_edges_per_elem[iblk] > 0) {
      if ((status = nc_def_dim(exoid, DIM_NUM_EDG_PER_EL(cur_num_elem_blk + 1),
                               param->num_edges_per_elem[iblk], &neledgdim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of edges/element for block %" PRId64
                 " in file id %d",
                 eb_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }

    if (param->num_faces_per_elem[iblk] > 0) {
      if ((status = nc_def_dim(exoid, DIM_NUM_FAC_PER_EL(cur_num_elem_blk + 1),
                               param->num_faces_per_elem[iblk], &nelfacdim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of faces/element for block %" PRId64
                 " in file id %d",
                 eb_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }

    EX_PREPARE_ATTRIB_ARRAY("element", cur_num_elem_blk, DIM_NUM_ATT_IN_BLK, param->num_attr_elem,
                            eb_id, VAR_ATTRIB, numelbdim, numattrdim, VAR_NAME_ATTRIB);

    EX_PREPARE_CONN("nodal", cur_num_elem_blk, eb_id, numelbdim, VAR_CONN, nelnoddim);

    /* store element type as attribute of connectivity variable */
    if ((status = nc_put_att_text(exoid, connid, ATT_NAME_ELB, strlen(param->elem_type[iblk]) + 1,
                                  (void *)param->elem_type[iblk])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store element type name %s in file id %d",
               param->elem_type[iblk], exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    EX_PREPARE_CONN("edge", cur_num_elem_blk, eb_id, numelbdim, VAR_ECONN, neledgdim);
    EX_PREPARE_CONN("face", cur_num_elem_blk, eb_id, numelbdim, VAR_FCONN, nelfacdim);
  }

  /* Define the element map here to avoid a later redefine call */
  if (param->define_maps != 0) {
    size_t map_type;
    for (map_type = 0; map_type < num_map_dims; ++map_type) {
      if ((status = nc_inq_dimid(exoid, dim_size_maps[map_type], &dims[0])) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: could not find map size dimension %s in file id %d",
                 dim_size_maps[map_type], exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      for (i = 1; i <= num_maps[map_type]; ++i) {
        const char *mapname = exi_name_of_map(map_enums[map_type], i);
        if (nc_inq_varid(exoid, mapname, &temp) != NC_NOERR) {
          int map_int_type = NC_INT;
          if (ex_int64_status(exoid) & EX_MAPS_INT64_DB) {
            map_int_type = NC_INT64;
          }

          if ((status = nc_def_var(exoid, mapname, map_int_type, 1, dims, &temp)) != NC_NOERR) {
            if (status == NC_ENAMEINUSE) {
              snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: number map %s already exists in file id %d",
                       mapname, exoid);
            }
            else {
              snprintf(errmsg, MAX_ERR_LENGTH,
                       "ERROR: failed to create number map array %s in file id %d", mapname, exoid);
            }
            ex_err_fn(exoid, __func__, errmsg, status);
            goto error_ret; /* exit define mode and return */
          }
          exi_compress_variable(exoid, temp, 1);
        }
      }
    }
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

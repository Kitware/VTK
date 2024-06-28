/*
 * Copyright(C) 1999-2021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*!
 *
 * \undoc exgconn - exodusII read edge/face/element block connectivity
 *
 * entry conditions -
 *   expelb must be called first to establish element block parameters.
 *   input parameters:
 *       int     exoid           exodus file id
 *       int     blk_type        block type (edge, face, element)
 *       int     blk_id          block id
 *
 * exit conditions -
 *       int*    nodeconn        nodal connectivity array
 *       int*    edgeconn        edge connectivity array (where applicable)
 *       int*    faceconn        face connectivity array (where applicable)
 *
 *
 */
#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_id_lkup, etc

int ex_get_conn(int exoid, ex_entity_type blk_type, ex_entity_id blk_id, void_int *nodeconn,
                void_int *edgeconn, void_int *faceconn)
{
  int connid  = -1;
  int econnid = -1;
  int fconnid = -1;

  int blk_id_ndx, status;

  int numnodperentdim = -1;
  int numedgperentdim = -1;
  int numfacperentdim = -1;

  size_t num_nodes_per_entry = 0;
  size_t num_edges_per_entry = 0;
  size_t num_faces_per_entry = 0;

  char errmsg[MAX_ERR_LENGTH];

  const char *dnumnodent = NULL;
  const char *dnumedgent = NULL;
  const char *dnumfacent = NULL;
  const char *vnodeconn  = NULL;
  const char *vedgeconn  = NULL;
  const char *vfaceconn  = NULL;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Should we warn if edgeconn or faceconn are non-NULL?
   * No, fail silently so the same code can be used to read any type of block
   * info.
   * However, we will warn if edgeconn or faceconn are NULL but
   * num_edges_per_entry
   * or num_faces_per_entry (respectively) are positive.
   */

  /* Locate index of element block id in VAR_ID_EL_BLK array */

  blk_id_ndx = exi_id_lkup(exoid, blk_type, blk_id);
  if (blk_id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);
    if (status != 0) {
      if (status == EX_NULLENTITY) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: no connectivity array for NULL %s %" PRId64 " in file id %d",
                 ex_name_of_object(blk_type), blk_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
        EX_FUNC_LEAVE(EX_WARN); /* no connectivity array for this element block */
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in id array in file id %d",
               ex_name_of_object(blk_type), blk_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  switch (blk_type) {
  case EX_EDGE_BLOCK:
    dnumnodent = DIM_NUM_NOD_PER_ED(blk_id_ndx);
    dnumedgent = NULL;
    dnumfacent = NULL;
    vnodeconn  = VAR_EBCONN(blk_id_ndx);
    vedgeconn  = NULL;
    vfaceconn  = NULL;
    break;
  case EX_FACE_BLOCK:
    dnumnodent = DIM_NUM_NOD_PER_FA(blk_id_ndx);
    dnumedgent = NULL;
    dnumfacent = NULL;
    vnodeconn  = VAR_FBCONN(blk_id_ndx);
    vedgeconn  = NULL;
    vfaceconn  = NULL;
    break;
  case EX_ELEM_BLOCK:
    dnumnodent = DIM_NUM_NOD_PER_EL(blk_id_ndx);
    dnumedgent = DIM_NUM_EDG_PER_EL(blk_id_ndx);
    dnumfacent = DIM_NUM_FAC_PER_EL(blk_id_ndx);
    vnodeconn  = VAR_CONN(blk_id_ndx);
    vedgeconn  = VAR_ECONN(blk_id_ndx);
    vfaceconn  = VAR_FCONN(blk_id_ndx);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized block type in switch: %d in file id %d", blk_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL); /* number of attributes not defined */
  }
  /* inquire id's of previously defined dimensions  */

  num_nodes_per_entry = 0;
  if (nodeconn && dnumnodent) {
    if ((status = nc_inq_dimid(exoid, dnumnodent, &numnodperentdim)) != NC_NOERR) {
      numnodperentdim = -1;
    }
    else {
      if ((status = nc_inq_dimlen(exoid, numnodperentdim, &num_nodes_per_entry)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get number of nodes/entity for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blk_type), blk_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

  if (edgeconn && dnumedgent) {
    num_edges_per_entry = 0;
    if ((status = nc_inq_dimid(exoid, dnumedgent, &numedgperentdim)) != NC_NOERR) {
      numedgperentdim = -1;
    }
    else {
      if ((status = nc_inq_dimlen(exoid, numedgperentdim, &num_edges_per_entry)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get number of edges/entry for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blk_type), blk_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

  if (faceconn && dnumfacent) {
    num_faces_per_entry = 0;
    if ((status = nc_inq_dimid(exoid, dnumfacent, &numfacperentdim)) != NC_NOERR) {
      numfacperentdim = -1;
    }
    else {
      if ((status = nc_inq_dimlen(exoid, numfacperentdim, &num_faces_per_entry)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get number of faces/entry for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blk_type), blk_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

  status = 0;
  if (nodeconn && (numnodperentdim >= 0) &&
      ((status = nc_inq_varid(exoid, vnodeconn, &connid)) != NC_NOERR)) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate node connectivity array for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  status = 0;
  if (edgeconn && (numedgperentdim >= 0) &&
      ((status = nc_inq_varid(exoid, vedgeconn, &econnid)) != NC_NOERR)) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate edge connectivity array for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (faceconn && (numfacperentdim >= 0) &&
      ((status = nc_inq_varid(exoid, vfaceconn, &fconnid)) != NC_NOERR)) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate face connectivity array for %s %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* read in the connectivity array */
  if (edgeconn && num_edges_per_entry > 0) {
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      status = nc_get_var_longlong(exoid, econnid, edgeconn);
    }
    else {
      status = nc_get_var_int(exoid, econnid, edgeconn);
    }

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get edge connectivity array for %s %" PRId64 " in file id %d",
               ex_name_of_object(blk_type), blk_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if (faceconn && num_faces_per_entry > 0) {
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      status = nc_get_var_longlong(exoid, fconnid, faceconn);
    }
    else {
      status = nc_get_var_int(exoid, fconnid, faceconn);
    }

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get face connectivity array for %s %" PRId64 " in file id %d",
               ex_name_of_object(blk_type), blk_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  if (nodeconn && num_nodes_per_entry > 0) {
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      status = nc_get_var_longlong(exoid, connid, nodeconn);
    }
    else {
      status = nc_get_var_int(exoid, connid, nodeconn);
    }

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get node connectivity array for %s %" PRId64 " in file id %d",
               ex_name_of_object(blk_type), blk_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expenm - ex_put_partial_id_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       ex_entity_type obj_type
 *       int*    elem_map                element numbering map array
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/*!
 * writes out a portion of the entity numbering map to the database;
 * this allows the entity numbers to be non-contiguous.  This map is
 * used for mapping between local and global entity ids.
 * \param    exoid                   exodus file id
 * \param    map_type
 * \param    start_entity_num
 * \param    num_entities
 * \param    map                numbering map array
 */

int ex_put_partial_id_map(int exoid, ex_entity_type map_type, int64_t start_entity_num,
                          int64_t num_entities, const void_int *map)
{
  int         dimid = 0, mapid = 0, status, dims[1];
  int         map_int_type;
  size_t      start[1], count[1];
  char        errmsg[MAX_ERR_LENGTH];
  const char *tname       = NULL;
  const char *dnumentries = NULL;
  const char *vmap        = NULL;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (num_entities == 0 && !exi_is_parallel(exoid)) {
    EX_FUNC_LEAVE(EX_NOERR);
  }

  switch (map_type) {
  case EX_NODE_MAP:
    tname       = "node";
    dnumentries = DIM_NUM_NODES;
    vmap        = VAR_NODE_NUM_MAP;
    break;
  case EX_EDGE_MAP:
    tname       = "edge";
    dnumentries = DIM_NUM_EDGE;
    vmap        = VAR_EDGE_NUM_MAP;
    break;
  case EX_FACE_MAP:
    tname       = "face";
    dnumentries = DIM_NUM_FACE;
    vmap        = VAR_FACE_NUM_MAP;
    break;
  case EX_ELEM_MAP:
    tname       = "element";
    dnumentries = DIM_NUM_ELEM;
    vmap        = VAR_ELEM_NUM_MAP;
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Bad map type (%d) specified for file id %d", map_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Make sure the file contains entries */
  if (nc_inq_dimid(exoid, dnumentries, &dimid) != NC_NOERR) {
    /* Possible Error -- if we made it this far, num_entities is > 0,
     * or this is a parallel run but the dimension 'dnumentries' is
     * not defined which indicates that either the entity count is
     * zero, or there is an error in that the dimension has not yet
     * been defined. Assume that if num_entities == 0 and this is a
     * parallel run that there is no error; otherwise if in serial and
     * num_entities != 0, there is an error.
     */
    if (num_entities != 0) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: The %s count is %" PRId64
               ", but the %s dimension is not defined on file id %d.",
               tname, num_entities, dnumentries, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* define the map if it doesn't already exist... */
  if (nc_inq_varid(exoid, vmap, &mapid) != NC_NOERR) {
    if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* create a variable array in which to store the id map  */

    dims[0] = dimid;

    /* Check type to be used for maps... */
    map_int_type = NC_INT;
    if (ex_int64_status(exoid) & EX_MAPS_INT64_DB) {
      map_int_type = NC_INT64;
    }

    if ((status = nc_def_var(exoid, vmap, map_int_type, 1, dims, &mapid)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s numbering map already exists in file id %d",
                 tname, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to create %s id map in file id %d", tname,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      goto error_ret; /* exit define mode and return */
    }
    exi_compress_variable(exoid, mapid, 1);

    /* leave define mode  */
    if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* write out the entity numbering map  */
  start[0] = start_entity_num - 1;
  count[0] = num_entities;
  if (num_entities == 0) {
    start[0] = 0;
  }

  /* write out the entity numbering map  */
  if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
    status = nc_put_vara_longlong(exoid, mapid, start, count, map);
  }
  else {
    status = nc_put_vara_int(exoid, mapid, start, count, map);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s numbering map in file id %d", tname,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  exi_leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

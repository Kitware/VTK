/*
 * Copyright(C) 1999-2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expem - ex_put_num_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     map_type                type of map (node,edge,face,elem)
 *       int     map_id                  id to associate with new map
 *       int     *map_data               map set value array
 *
 * exit conditions -
 *
 * revision history -
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * writes a map; this is a vector of integers of the same length as the
 * number of entries in the source object (nodes, edges, faces, or elements
 * in the file).
 * \param   exoid                   exodus file id
 * \param   map_type                type of map (node,edge,face,elem)
 * \param   map_id                  id to associate with new map
 * \param   map                    map set value array
 */

int ex_put_num_map(int exoid, ex_entity_type map_type, ex_entity_id map_id, const void_int *map)
{
  size_t start[1];
  char   errmsg[MAX_ERR_LENGTH];
  int    status;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  const char *dnumentries;
  const char *dnummaps;
  const char *vmapids;

  switch (map_type) {
  case EX_NODE_MAP:
    dnumentries = DIM_NUM_NODES;
    dnummaps    = DIM_NUM_NM;
    vmapids     = VAR_NM_PROP(1);
    break;
  case EX_EDGE_MAP:
    dnumentries = DIM_NUM_EDGE;
    dnummaps    = DIM_NUM_EDM;
    vmapids     = VAR_EDM_PROP(1);
    break;
  case EX_FACE_MAP:
    dnumentries = DIM_NUM_FACE;
    dnummaps    = DIM_NUM_FAM;
    vmapids     = VAR_FAM_PROP(1);
    break;
  case EX_ELEM_MAP:
    dnumentries = DIM_NUM_ELEM;
    dnummaps    = DIM_NUM_EM;
    vmapids     = VAR_EM_PROP(1);
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Bad map type (%d) specified for file id %d", map_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Make sure the file contains entries */
  int dimid;
  if (nc_inq_dimid(exoid, dnumentries, &dimid) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_NOERR);
  }

  /* first check if any maps are specified */
  if ((status = nc_inq_dimid(exoid, dnummaps, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %ss specified in file id %d",
             ex_name_of_object(map_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* If the `map_id` is negative, then that specifies a specific location for that map */
  bool overwrite_map = false;
  bool id_is_index   = map_id < 0;
  if (id_is_index) {
    map_id = -map_id;
  }

  /* Check for duplicate map id entry */
  status = exi_id_lkup(exoid, map_type, map_id);
  if (status != -EX_LOOKUPFAIL) { /* found the map id */
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: %s %" PRId64 " already defined in file id %d",
             ex_name_of_object(map_type), map_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    id_is_index   = true;
    overwrite_map = true;
  }

  /* Get number of maps initialized for this file */
  size_t num_entries;
  if ((status = nc_inq_dimlen(exoid, dimid, &num_entries)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %ss in file id %d",
             ex_name_of_object(map_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Keep track of the total number of maps defined using a counter stored
     in a linked list keyed by exoid.
     NOTE: exi_get_file_item  is used to find the number of maps
     for a specific file and returns that value.
  */
  int cur_num_maps = exi_get_file_item(exoid, exi_get_counter_list(map_type));
  if (!overwrite_map) {
    int num_maps = num_entries;
    if (cur_num_maps >= num_maps) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: exceeded number of %ss (%d) specified in file id %d",
               ex_name_of_object(map_type), num_maps, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*   NOTE: exi_inc_file_item  is used to find the number of maps
         for a specific file and returns that value incremented. */
    cur_num_maps = exi_inc_file_item(exoid, exi_get_counter_list(map_type));
  }

  if (id_is_index) {
    cur_num_maps = map_id - 1;
  }

  /* write out information to previously defined variable */

  /* first get id of variable */
  int varid;
  if ((status = nc_inq_varid(exoid, vmapids, &varid)) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s ids in file id %d",
             ex_name_of_object(map_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* then, write out map id */
  start[0] = cur_num_maps;

  int ldum = (int)map_id;
  if ((status = nc_put_var1_int(exoid, varid, start, &ldum)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s id %" PRId64 " in file id %d",
             ex_name_of_object(map_type), map_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  const char *vmap;
  switch (map_type) {
  case EX_NODE_MAP: vmap = VAR_NODE_MAP(cur_num_maps + 1); break;
  case EX_EDGE_MAP: vmap = VAR_EDGE_MAP(cur_num_maps + 1); break;
  case EX_FACE_MAP: vmap = VAR_FACE_MAP(cur_num_maps + 1); break;
  case EX_ELEM_MAP: vmap = VAR_ELEM_MAP(cur_num_maps + 1); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized map type in switch: %d in file id %d", map_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* locate variable array in which to store the map */
  if (nc_inq_varid(exoid, vmap, &varid) != NC_NOERR) {
    int dims[2];

    /* determine number of entries */
    if ((status = nc_inq_dimid(exoid, dnumentries, &dimid)) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: couldn't determine number of %s entries in file id %d",
               ex_name_of_object(map_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to place file id %d into define mode", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Check type to be used for maps... */
    int map_int_type = NC_INT;
    if (ex_int64_status(exoid) & EX_MAPS_INT64_DB) {
      map_int_type = NC_INT64;
    }

    dims[0] = dimid;
    if ((status = nc_def_var(exoid, vmap, map_int_type, 1, dims, &varid)) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define map %s in file id %d", vmap, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    exi_compress_variable(exoid, varid, 1);

    if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) { /* exit define mode */
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
      ex_err_fn(exoid, __func__, errmsg, status);
      varid = -1; /* force early exit */
    }

    if (varid == -1) { /* we couldn't define variable and have prepared error message. */
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* write out the map  */

  if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
    status = nc_put_var_longlong(exoid, varid, map);
  }
  else {
    status = nc_put_var_int(exoid, varid, map);
  }
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s in file id %d",
             ex_name_of_object(map_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

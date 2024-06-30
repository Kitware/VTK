/*
 * Copyright(C) 1999-2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exppem - ex_put_partial_num_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     map_id                  element map id
 *       int     ent_start               first entry in map
 *       int     ent_count               number of entries in map
 *       int     *map                    map
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * writes a map; this is a vector of integers of length number of mesh
 * objects of that type (element, node, face, edge)
 */
int ex_put_partial_num_map(int exoid, ex_entity_type map_type, ex_entity_id map_id,
                           int64_t ent_start, int64_t ent_count, const void_int *map)
{
  int         status;
  int         dimid, varid, map_exists;
  size_t      start[1];
  size_t      num_maps, num_mobj, count[1];
  int         cur_num_maps;
  char        errmsg[MAX_ERR_LENGTH];
  const char *dnumentries;
  const char *dnummaps;
  const char *vmapids;
  const char *vmap;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

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
  bool id_is_index = map_id < 0;
  if (id_is_index) {
    map_id = -map_id;
  }

  /* Check for duplicate map id entry */
  status = exi_id_lkup(exoid, map_type, map_id);
  if (status == -EX_LOOKUPFAIL) { /* did not find the map id */
    map_exists = 0;               /* Map is being defined */
  }
  else {
    map_exists = 1; /* assume a portion of this map has already been written */
  }

  /* Check for duplicate map id entry */
  if (!map_exists) {
    /* Get number of maps initialized for this file */
    if ((status = nc_inq_dimlen(exoid, dimid, &num_maps)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %ss in file id %d",
               ex_name_of_object(map_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Keep track of the total number of maps defined using a
       counter stored in a linked list keyed by exoid.  NOTE:
       exi_get_file_item is used to find the number of element maps for a
       specific file and returns that value.
    */
    cur_num_maps = exi_get_file_item(exoid, exi_get_counter_list(map_type));
    if (cur_num_maps >= (int)num_maps) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: exceeded number of %ss (%zu) specified in file id %d",
               ex_name_of_object(map_type), num_maps, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*   NOTE: exi_inc_file_item  is used to find the number of element maps
         for a specific file and returns that value incremented. */
    cur_num_maps = exi_inc_file_item(exoid, exi_get_counter_list(map_type));

    if (id_is_index) {
      cur_num_maps = map_id - 1;
    }
  }
  else {
    int map_ndx  = exi_id_lkup(exoid, map_type, map_id);
    cur_num_maps = map_ndx - 1;
  }

  /* determine number of elements */
  if ((status = nc_inq_dimid(exoid, dnumentries, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: couldn't determine number of mesh objects in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimlen(exoid, dimid, &num_mobj)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of mesh objects in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Check input parameters for a valid range of numbers */
  if (ent_count == 0) {
    ent_start = 0;
  }

  if (ent_start < 0 || ent_start > num_mobj) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: start count is invalid in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ent_count < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid count value in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (ent_count > 0 && (ent_start + ent_count - 1 > num_mobj)) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: start+count-1 is larger than mesh object count in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out information to previously defined variable */

  /* first get id of variable */
  if ((status = nc_inq_varid(exoid, vmapids, &varid)) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s ids in file id %d",
             ex_name_of_object(map_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* then, write out map id */
  if (!map_exists) {
    start[0] = cur_num_maps;
    {
      if ((status = nc_put_var1_longlong(exoid, varid, start, (long long *)&map_id)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s id %" PRId64 " in file id %d",
                 ex_name_of_object(map_type), map_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

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
  if ((status = nc_inq_varid(exoid, vmap, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s %" PRId64 " in file id %d",
             ex_name_of_object(map_type), map_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out the map  */
  start[0] = ent_start - 1;
  count[0] = ent_count;

  if (count[0] == 0) {
    start[0] = 0;
  }

  if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
    status = nc_put_vara_longlong(exoid, varid, start, count, map);
  }
  else {
    status = nc_put_vara_int(exoid, varid, start, count, map);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s in file id %d",
             ex_name_of_object(map_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

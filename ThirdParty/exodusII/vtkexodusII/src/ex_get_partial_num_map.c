/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgpem - ex_get_partial_elem_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     map_id                  element map id
 *       int     ent_start               first entry in map
 *       int     ent_count               number of entries in map
 *
 * exit conditions -
 *       int*    elem_map                element map
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/*
 * reads the element map with specified ID
 */

int ex_get_partial_num_map(int exoid, ex_entity_type map_type, ex_entity_id map_id,
                           int64_t ent_start, int64_t ent_count, void_int *map)
{
  int         dimid, var_id, id_ndx, status;
  size_t      num_mobj, start[1], count[1];
  char        errmsg[MAX_ERR_LENGTH];
  const char *dim_map_size;
  const char *dim_num_maps;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  switch (map_type) {
  case EX_NODE_MAP:
    dim_map_size = DIM_NUM_NODES;
    dim_num_maps = DIM_NUM_NM;
    break;
  case EX_EDGE_MAP:
    dim_map_size = DIM_NUM_EDGE;
    dim_num_maps = DIM_NUM_EDM;
    break;
  case EX_FACE_MAP:
    dim_map_size = DIM_NUM_FACE;
    dim_num_maps = DIM_NUM_FAM;
    break;
  case EX_ELEM_MAP:
    dim_map_size = DIM_NUM_ELEM;
    dim_num_maps = DIM_NUM_EM;
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH, "Bad map type (%d) specified", map_type);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* See if file contains any elements...*/
  if (nc_inq_dimid(exoid, dim_map_size, &dimid) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_NOERR);
  }

  if ((status = nc_inq_dimlen(exoid, dimid, &num_mobj)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of mesh objects in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Check input parameters for a valid range of numbers */
  if (ent_start <= 0 || ent_start > num_mobj) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: start count is invalid in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ent_count < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid count value in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ent_start + ent_count - 1 > num_mobj) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: start+count-1 is larger than element count in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* first check if any maps have been defined */
  if ((status = nc_inq_dimid(exoid, dim_num_maps, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no %ss defined in file id %d",
             ex_name_of_object(map_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* Lookup index of element map id property array */
  id_ndx = exi_id_lkup(exoid, map_type, map_id);
  if (id_ndx <= 0) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate %s id %" PRId64 " in id variable in file id %d",
             ex_name_of_object(map_type), map_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire id's of previously defined dimensions and variables */
  if ((status = nc_inq_varid(exoid, exi_name_of_map(map_type, id_ndx), &var_id)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s %" PRId64 " in file id %d",
             ex_name_of_object(map_type), map_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* read in the map */
  start[0] = ent_start - 1;
  count[0] = ent_count;
  if (count[0] == 0) {
    start[0] = 0;
  }

  if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
    status = nc_get_vara_longlong(exoid, var_id, start, count, map);
  }
  else {
    status = nc_get_vara_int(exoid, var_id, start, count, map);
  }

  if (status == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s in file id %d",
             ex_name_of_object(map_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

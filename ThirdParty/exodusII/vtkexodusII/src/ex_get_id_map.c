/*
 * Copyright(C) 1999-2021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgnm - ex_get_id_map
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     map_type                type of map (node, edge, face, element)
 *
 * exit conditions -
 *       int*    map                     map
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/*
 * reads the id map
 */

int ex_get_id_map(int exoid, ex_entity_type map_type, void_int *map)
{
  int         status;
  char        errmsg[MAX_ERR_LENGTH];
  const char *dnumentries;
  const char *vmap;
  const char *tname;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
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

  /* See if any entries are stored in this file */
  int dimid;
  if (nc_inq_dimid(exoid, dnumentries, &dimid) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_NOERR);
  }

  int mapid;
  if (nc_inq_varid(exoid, vmap, &mapid) != NC_NOERR) {
    size_t num_entries;
    if ((status = nc_inq_dimlen(exoid, dimid, &num_entries)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %ss in file id %d", tname,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* generate default map of 1..n, where n is num_entries */
    if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
      int64_t *lmap = (int64_t *)map;
      for (size_t i = 0; i < num_entries; i++) {
        lmap[i] = i + 1;
      }
    }
    else {
      int *lmap = (int *)map;
      for (size_t i = 0; i < num_entries; i++) {
        lmap[i] = i + 1;
      }
    }

    EX_FUNC_LEAVE(EX_NOERR);
  }

  /* read in the id map  */
  if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
    status = nc_get_var_longlong(exoid, mapid, map);
  }
  else {
    status = nc_get_var_int(exoid, mapid, map);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s id map in file id %d", tname, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

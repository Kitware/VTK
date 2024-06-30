/*
 * Copyright(C) 1999-2021 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/*!
 * \ingroup ModelDescription
 *
 * reads the a portion of the values of the id map for the entity type specified by `map_type`
 * The beginning location of the read is a `start_entity_num` which is 1-based. The read will
 * return `num_entities` values starting at that location.  Requirements are:
 * - `start_entity_num > 0`
 * - `start_entity_num + num_entities - 1 <= num_entity` which is the number of entities of
 * specified type (e.g. elements)
 *
 * \param      exoid            exodus file id
 * \param      map_type         type (edge block, face block, edge set, ... )
 * \param      start_entity_num index of first entity in block to read (1-based)
 * \param      num_entities     number of entries to read in this block/set
 * \param      map              the values read are returned here.
 */

int ex_get_partial_id_map(int exoid, ex_entity_type map_type, int64_t start_entity_num,
                          int64_t num_entities, void_int *map)
{
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  const char *dnumentries;
  const char *vmap;
  const char *tname;
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

  size_t num_entries;
  int    status;
  if ((status = nc_inq_dimlen(exoid, dimid, &num_entries)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %ss in file id %d", tname,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (start_entity_num < 1) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: start index (%" PRId64 ") must be greater than 0 in file id %d",
             start_entity_num, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (start_entity_num + num_entities - 1 > num_entries) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: start index (%" PRId64 ") + entity count (%" PRId64
             ") is larger than total number of entities (%zu) in file id %d",
             start_entity_num, num_entities, num_entries, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int mapid;
  if (nc_inq_varid(exoid, vmap, &mapid) != NC_NOERR) {
    /* generate portion of the default map (1..num_entries) */
    if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
      int64_t *lmap = (int64_t *)map;
      for (int64_t i = 0; i < num_entities; i++) {
        lmap[i] = start_entity_num + i;
      }
    }
    else {
      int *lmap = (int *)map;
      for (int64_t i = 0; i < num_entities; i++) {
        lmap[i] = start_entity_num + i;
      }
    }

    EX_FUNC_LEAVE(EX_NOERR);
  }

  size_t start[] = {start_entity_num - 1};
  size_t count[] = {num_entities};
  if (count[0] == 0) {
    start[0] = 0;
  }

  /* read in the id map  */
  if (ex_int64_status(exoid) & EX_MAPS_INT64_API) {
    status = nc_get_vara_longlong(exoid, mapid, start, count, map);
  }
  else {
    status = nc_get_vara_int(exoid, mapid, start, count, map);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s id map in file id %d", tname, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

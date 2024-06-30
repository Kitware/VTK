/*
 * Copyright(C) 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * ex_get_block_id_map
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
 * reads the id map for the edge/face/element block with the specified id.
 */

int ex_get_block_id_map(int exoid, ex_entity_type obj_type, ex_entity_id entity_id, void_int *map)
{
  int            status;
  char           errmsg[MAX_ERR_LENGTH];
  const char    *dnument  = NULL;
  ex_entity_type map_type = EX_INVALID;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Determine index of entity_id in id array */
  int blk_id_ndx = exi_id_lkup(exoid, obj_type, entity_id);
  if (blk_id_ndx <= 0) { /* Empty block */
    ex_get_err(NULL, NULL, &status);
    if (status != 0) {
      if (status == EX_NULLENTITY) { /* NULL element block?    */
        EX_FUNC_LEAVE(EX_NOERR);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id  %" PRId64 " in id array in file id %d",
               ex_name_of_object(obj_type), entity_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  size_t offset = 1;
  size_t len    = 0;
  for (int i = 1; i <= blk_id_ndx; i++) {
    offset += len;
    /* inquire values of some dimensions */
    int dimid = 0;

    /* Determine the number of entities in all previous blocks. */
    switch (obj_type) {
    case EX_EDGE_BLOCK:
      dnument  = DIM_NUM_ED_IN_EBLK(i);
      map_type = EX_EDGE_MAP;
      break;
    case EX_FACE_BLOCK:
      dnument  = DIM_NUM_FA_IN_FBLK(i);
      map_type = EX_FACE_MAP;
      break;
    case EX_ELEM_BLOCK:
      dnument  = DIM_NUM_EL_IN_BLK(i);
      map_type = EX_ELEM_MAP;
      break;
    default:
      snprintf(errmsg, MAX_ERR_LENGTH, "Bad block type parameter (%d) specified for file id %d.",
               obj_type, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    if ((status = nc_inq_dimid(exoid, dnument, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate number of entities in %s  %" PRId64 " in file id %d",
               ex_name_of_object(obj_type), entity_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_inq_dimlen(exoid, dimid, &len)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get number of %ss in block  %" PRId64 " in file id %d",
               ex_name_of_object(obj_type), entity_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  EX_FUNC_UNLOCK();
  return ex_get_partial_id_map(exoid, map_type, offset, len, map);
}

/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgecpp - ex_get_entity_count_per_polyhedra
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_id_lkup, etc

/*!
 * reads in the number of entities (nodes/faces) per polyhedra
 * (nsided/nfaced) in this element block.
 * \param  exoid                exodus file id
 * \param  blk_type             type of block (face, or element)
 * \param  blk_id               block identifier
 * \param  entity_counts        entity_per_polyhedra count array
 */

int ex_get_entity_count_per_polyhedra(int exoid, ex_entity_type blk_type, ex_entity_id blk_id,
                                      int *entity_counts)
{
  int  npeid = -1, blk_id_ndx, status;
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  blk_id_ndx = exi_id_lkup(exoid, blk_type, blk_id);
  if (blk_id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      if (status == EX_NULLENTITY) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: entity_counts array not allowed for NULL %s block %" PRId64
                 " in file id %d",
                 ex_name_of_object(blk_type), blk_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
        EX_FUNC_LEAVE(EX_WARN);
      }

      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s block id %" PRId64 " in id array in file id %d",
               ex_name_of_object(blk_type), blk_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* inquire id's of previously defined dimensions  */
  switch (blk_type) {
  case EX_ELEM_BLOCK: status = nc_inq_varid(exoid, VAR_EBEPEC(blk_id_ndx), &npeid); break;
  case EX_FACE_BLOCK: status = nc_inq_varid(exoid, VAR_FBEPEC(blk_id_ndx), &npeid); break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "Internal ERROR: unrecognized block type in switch: %d in file id %d", blk_type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate entity_counts array for %s block %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  status = nc_get_var_int(exoid, npeid, entity_counts);
  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to read node counts array for %s block %" PRId64 " in file id %d",
             ex_name_of_object(blk_type), blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

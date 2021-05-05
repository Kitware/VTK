/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h" // for void_int, etc

/*!
 * \deprecated Use ex_get_partial_conn()(exoid, blk_type, blk_id, start_num, num_ent, nodeconn,
 * edgeconn, faceconn) instead.
 */

int ex_get_n_conn(int exoid, ex_entity_type blk_type, ex_entity_id blk_id, int64_t start_num,
                  int64_t num_ent, void_int *nodeconn, void_int *edgeconn, void_int *faceconn)
{
  return ex_get_partial_conn(exoid, blk_type, blk_id, start_num, num_ent, nodeconn, edgeconn,
                             faceconn);
}

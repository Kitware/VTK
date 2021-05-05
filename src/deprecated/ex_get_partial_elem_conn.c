/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *
 *      ex_get_partial_elem_conn()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      elem_blk_id        - The element block ID.
 *      start_elem_num     - The starting index of the elements to be
 *                           obtained.
 *      num_elems          - The number of FEM elements to read coords for.
 *      connect            - Pointer to the connectivity vector.
 *
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h" // for ex_get_partial_conn, etc

/*!
 * \deprecated use ex_get_partial_conn()(exoid, EX_ELEM_BLOCK, elem_blk_id, start_elem_num,
 num_elems, connect, NULL, NULL)
 * reads the connectivity array for an element block
 */

int ex_get_partial_elem_conn(int exoid, ex_entity_id elem_blk_id, int64_t start_elem_num,
                             int64_t num_elems, void_int *connect)
{
  return ex_get_partial_conn(exoid, EX_ELEM_BLOCK, elem_blk_id, start_elem_num, num_elems, connect,
                             NULL, NULL);
}

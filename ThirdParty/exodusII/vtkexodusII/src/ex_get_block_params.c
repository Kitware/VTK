/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#include "exodusII.h" // for EX_NOERR, etc
#include "exodusII_int.h"

/*!
 * Reads the parameters describing element/face/edge blocks
 * \param   exoid                   exodus file id
 * \param   block_count             number of blocks being queried
 * \param   blocks                  array of ex_block structures describing
 * block counts
 *
 * the id and type fields of the block(s) must be defined to specify which
 * blocks to access;
 * all other fields will be filled in based on data from the file
 */

int ex_get_block_params(int exoid, size_t block_count, struct ex_block **blocks)
{
  size_t i;
  EX_FUNC_ENTER();
  for (i = 0; i < block_count; i++) {
    int status = ex_get_block_param(exoid, blocks[i]);
    if (status != EX_NOERR) {
      EX_FUNC_LEAVE(status);
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

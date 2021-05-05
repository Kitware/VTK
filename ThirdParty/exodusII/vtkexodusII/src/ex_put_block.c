/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expblk - ex_put_block: write edge, face, or element block parameters
 *
 * entry conditions -
 *   input parameters:
 *       int     idexo                   exodus file id
 *       int     blk_type                type of block (edge, face, or element)
 *       int     blk_id                  block identifier
 *       char*   entry_descrip           string describing shape of entries in
 *the block
 *       int     num_entries_this_blk    number of entries(records) in the block
 *       int     num_nodes_per_entry     number of nodes per block entry
 *       int     num_edges_per_entry     number of edges per block entry
 *       int     num_faces_per_entry     number of faces per block entry
 *       int     num_attr_per_entry      number of attributes per block entry
 *
 * exit conditions -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_block, MAX_STR_LENGTH, etc
#include "exodusII_int.h"

/*!
 * writes the parameters used to describe an element/face/edge block
 * \param   exoid                   exodus file id
 * \param   blk_type                type of block (edge, face, or element)
 * \param   blk_id                  block identifier
 * \param   entry_descrip           string describing shape of entries in the
 * block
 * \param   num_entries_this_blk    number of entries(records) in the block
 * \param   num_nodes_per_entry     number of nodes per block entry
 * \param   num_edges_per_entry     number of edges per block entry
 * \param   num_faces_per_entry     number of faces per block entry
 * \param   num_attr_per_entry      number of attributes per block entry
 */

int ex_put_block(int exoid, ex_entity_type blk_type, ex_entity_id blk_id, const char *entry_descrip,
                 int64_t num_entries_this_blk, int64_t num_nodes_per_entry,
                 int64_t num_edges_per_entry, int64_t num_faces_per_entry,
                 int64_t num_attr_per_entry)
{
  ex_block block = {.type                = blk_type,
                    .id                  = blk_id,
                    .num_entry           = num_entries_this_blk,
                    .num_nodes_per_entry = num_nodes_per_entry,
                    .num_edges_per_entry = num_edges_per_entry,
                    .num_faces_per_entry = num_faces_per_entry,
                    .num_attribute       = num_attr_per_entry};

  ex_copy_string(block.topology, entry_descrip, MAX_STR_LENGTH + 1);
  block.topology[MAX_STR_LENGTH] = '\0';

  return ex_put_block_param(exoid, block);
}

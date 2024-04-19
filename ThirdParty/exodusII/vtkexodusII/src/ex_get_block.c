/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*!
 *
 * \undoc exgblk - read block parameters
 *
 * entry conditions -
 *   input parameters:
 *       int     idexo                   exodus file id
 *       int     blk_type                block type (edge,face,element)
 *       int     blk_id                  block id
 *
 * exit conditions -
 *       char*   elem_type               element type name
 *       int*    num_entries_this_blk    number of elements in this element block
 *       int*    num_nodes_per_entry     number of nodes per element block
 *       int*    num_attr_per_entry      number of attributes
 *
 * revision history -
 *
 *
 */

#include "exodusII.h" // for ex_block, void_int, etc
#include "exodusII_int.h"

/*
 * reads the parameters used to describe an edge, face, or element block
 */

int ex_get_block(int exoid, ex_entity_type blk_type, ex_entity_id blk_id, char *entity_descrip,
                 void_int *num_entries_this_blk, void_int *num_nodes_per_entry,
                 void_int *num_edges_per_entry, void_int *num_faces_per_entry,
                 void_int *num_attr_per_entry)
{
  int      err;
  ex_block block;

  EX_FUNC_ENTER();

  block.id   = blk_id;
  block.type = blk_type;

  err = ex_get_block_param(exoid, &block);

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    int64_t *n_entries_this_blk = num_entries_this_blk;
    int64_t *n_nodes_per_entry  = num_nodes_per_entry;
    int64_t *n_edges_per_entry  = num_edges_per_entry;
    int64_t *n_faces_per_entry  = num_faces_per_entry;
    int64_t *n_attr_per_entry   = num_attr_per_entry;

    if (n_entries_this_blk) {
      *n_entries_this_blk = block.num_entry;
    }
    if (n_nodes_per_entry) {
      *n_nodes_per_entry = block.num_nodes_per_entry;
    }
    if (n_edges_per_entry) {
      *n_edges_per_entry = block.num_edges_per_entry;
    }
    if (n_faces_per_entry) {
      *n_faces_per_entry = block.num_faces_per_entry;
    }
    if (n_attr_per_entry) {
      *n_attr_per_entry = block.num_attribute;
    }
  }
  else {
    int *n_entries_this_blk = num_entries_this_blk;
    int *n_nodes_per_entry  = num_nodes_per_entry;
    int *n_edges_per_entry  = num_edges_per_entry;
    int *n_faces_per_entry  = num_faces_per_entry;
    int *n_attr_per_entry   = num_attr_per_entry;

    if (n_entries_this_blk) {
      *n_entries_this_blk = block.num_entry;
    }
    if (n_nodes_per_entry) {
      *n_nodes_per_entry = block.num_nodes_per_entry;
    }
    if (n_edges_per_entry) {
      *n_edges_per_entry = block.num_edges_per_entry;
    }
    if (n_faces_per_entry) {
      *n_faces_per_entry = block.num_faces_per_entry;
    }
    if (n_attr_per_entry) {
      *n_attr_per_entry = block.num_attribute;
    }
  }

  if (entity_descrip) {
    ex_copy_string(entity_descrip, block.topology, MAX_STR_LENGTH + 1);
  }

  EX_FUNC_LEAVE(err);
}

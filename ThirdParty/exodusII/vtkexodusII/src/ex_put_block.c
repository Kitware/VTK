/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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
#include <stdint.h>   // for int64_t
#include <string.h>   // for strncpy

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
  ex_block block;
  block.type = blk_type;
  block.id   = blk_id;
  strncpy(block.topology, entry_descrip, MAX_STR_LENGTH + 1);
  block.topology[MAX_STR_LENGTH] = '\0';
  block.num_entry                = num_entries_this_blk;
  block.num_nodes_per_entry      = num_nodes_per_entry;
  block.num_edges_per_entry      = num_edges_per_entry;
  block.num_faces_per_entry      = num_faces_per_entry;
  block.num_attribute            = num_attr_per_entry;

  return ex_put_block_param(exoid, block);
}

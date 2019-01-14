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
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *
 *      ex_get_n_elem_attr()
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
 *      attrib             - Pointer to the attribute vector.
 *
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h" // for ex_get_partial_attr, etc
#include <stdint.h>   // for int64_t

/*!
 * \deprecated Use ex_get_partial_attr()(exoid, EX_ELEM_BLOCK, elem_blk_id, start_elem_num,
 * num_elems, attrib) instead.
 */

int ex_get_n_elem_attr(int exoid, ex_entity_id elem_blk_id, int64_t start_elem_num,
                       int64_t num_elems, void *attrib)

{
  return ex_get_partial_attr(exoid, EX_ELEM_BLOCK, elem_blk_id, start_elem_num, num_elems, attrib);
}

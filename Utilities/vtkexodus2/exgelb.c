/*
 * Copyright (c) 2006 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000 with Sandia Corporation, the U.S. Governement
 * retains certain rights in this software.
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
 *     * Neither the name of Sandia Corporation nor the names of its
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
* exgelb - read element block parameters
*
* entry conditions -
*   input parameters:
*       int     idexo                   exodus file id
*       int     elem_blk_id             element block id
*
* exit conditions -
*       char*   elem_type               element type name
*       int*    num_elem_this_blk       number of elements in this element block
*       int*    num_nodes_per_elem      number of nodes per element block
*       int*    num_attr                number of attributes
*
* revision history -
*
*  Id
*
*/

#include <string.h>
#include <stdio.h>
#include "exodusII.h"
#include "exodusII_int.h"

/**
 * reads the parameters used to describe an element block
 * \deprecated Use ex_get_block()(exoid, EX_ELEM_BLOCK, elem_blk_id, elem_type, num_elem_this_blk, num_nodes_per_elem, num_attr) instead
 */

int ex_get_elem_block (int   exoid,
                       int   elem_blk_id,
                       char *elem_type,
                       int  *num_elem_this_blk, 
                       int  *num_nodes_per_elem,
                       int  *num_attr)

{
  return ex_get_block( exoid, EX_ELEM_BLOCK, elem_blk_id, elem_type,
    num_elem_this_blk, num_nodes_per_elem, 0, 0, num_attr );
}

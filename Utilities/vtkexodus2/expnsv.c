/*
 * Copyright (c) 2005 Sandia Corporation. Under the terms of Contract
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
* expmv - ex_put_nset_var
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     time_step               time step number
*       int     nset_var_index          nodeset variable index
*       int     nset_id                 nodeset id
*       int     num_nodes_this_nset     number of nodes in this nodeset
*
* exit conditions -
*
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the values of a single nodeset variable for one nodeset at 
 * one time step to the database; assume the first time step and 
 * nodeset variable index are 1
 * \param      exoid                   exodus file id
 * \param      time_step               time step number
 * \param      nset_var_index          nodeset variable index
 * \param      nset_id                 nodeset id
 * \param      num_nodes_this_nset     number of nodes in this nodeset
 * \param      nset_var_vals           the values to be written
 * \deprecated Use ex_put_var()(exoid, time_step, EX_NODE_SET, nset_var_index, nset_id, num_nodes_this_nset, nset_var_vals)

 */

int ex_put_nset_var (int   exoid,
                     int   time_step,
                     int   nset_var_index,
                     int   nset_id,
                     int   num_nodes_this_nset,
                     const void *nset_var_vals)
{
  return ex_put_var(exoid, time_step, EX_NODE_SET, nset_var_index,
                    nset_id, num_nodes_this_nset, nset_var_vals);
}

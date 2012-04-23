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
* expvp - ex_put_all_var_param
*
* entry conditions - 
*   input parameters:
*       int     exoid           exodus file id
*       int     num_g           global variable count
*       int     num_n           nodal variable count
*       int     num_e           element variable count
*       int*    elem_var_tab    element variable truth table array
*       int     num_m           nodeset variable count
*       int*    nset_var_tab    nodeset variable truth table array
*       int     num_s           sideset variable count
*       int*    sset_var_tab    sideset variable truth table array
*
* exit conditions - 
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * defines the number of global, nodal, element, nodeset, and sideset variables 
 * that will be written to the database
*  \param     exoid           exodus file id
*  \param     num_g           global variable count
*  \param     num_n           nodal variable count
*  \param     num_e           element variable count
*  \param    *elem_var_tab    element variable truth table array
*  \param     num_m           nodeset variable count
*  \param    *nset_var_tab    nodeset variable truth table array
*  \param     num_s           sideset variable count
*  \param    *sset_var_tab    sideset variable truth table array
 */

int ex_put_all_var_param (int   exoid,
                          int   num_g,
                          int   num_n,
                          int   num_e,
                          int  *elem_var_tab,
                          int   num_m,
                          int  *nset_var_tab,
                          int   num_s,
                          int  *sset_var_tab)
{
  ex_var_params vparam;

  vparam.num_glob = num_g;
  vparam.num_node = num_n;
  vparam.num_edge = 0;
  vparam.edge_var_tab = 0;
  vparam.num_face = 0;
  vparam.face_var_tab = 0;
  vparam.num_elem = num_e;
  vparam.elem_var_tab = elem_var_tab;
  vparam.num_nset = num_m;
  vparam.nset_var_tab = nset_var_tab;
  vparam.num_eset = 0;
  vparam.eset_var_tab = 0;
  vparam.num_fset = 0;
  vparam.fset_var_tab = 0;
  vparam.num_sset = num_s;
  vparam.sset_var_tab = sset_var_tab;
  vparam.num_elset = 0;
  vparam.elset_var_tab = 0;

  return ex_put_all_var_param_ext( exoid, &vparam );
}

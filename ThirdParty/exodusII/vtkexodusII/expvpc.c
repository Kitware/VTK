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
* expvp - ex_put_concat_var_param
*
* entry conditions - 
*   input parameters:
*       int     exoid   exodus file id
*       int     num_g   global variable count
*       int     num_n   nodal variable count
*       int     num_e   element variable count
*       int     num_elem_blk            number of element blocks (unused)
*       int*    elem_var_tab            element variable truth table array
*
* exit conditions - 
*
* revision history - 
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

#include <ctype.h>
/*!
 * writes the number of global, nodal, and element variables 
 * that will be written to the database
 * \param      exoid           int             exodus file id
 * \param      num_g           int             global variable count
 * \param      num_n           int             nodal variable count
 * \param      num_e           int             element variable count
 * \param      num_elem_blk    int             number of element blocks
 * \param      elem_var_tab    int*            element variable truth table array
 * \deprecated Use ex_put_all_var_param()(exoid, num_g, num_n, num_e, elem_var_tab, 0, 0, 0, 0)
 */

int ex_put_concat_var_param (int   exoid,
                             int   num_g,
                             int   num_n,
                             int   num_e,
                             int   num_elem_blk, /* unused */
                             int  *elem_var_tab)
{
  return ex_put_all_var_param(exoid, num_g, num_n, num_e, elem_var_tab, 0, 0, 0, 0);
}

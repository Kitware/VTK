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
* expsstt - ex_put_sset_var_tab
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     num_sset                number of sidesets
*       int     num_sset_var            number of sideset variables
*       int*    sset_var_tab            sideset variable truth table array
*
* exit conditions - 
*
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*!
 * writes the EXODUS II sideset variable truth table to the database; 
 * also, creates netCDF variables in which to store EXODUS II sideset
 * variable values; although this table isn't required (because the
 * netCDF variables can also be created in ex_put_sset_var), this call
 * will save tremendous time because all of the variables are defined
 * at once while the file is in define mode, rather than going in and out
 * of define mode (causing the entire file to be copied over and over)
 * which is what occurs when the sideset variable values variables are
 * defined in ex_put_sset_var
 * \param      exoid                   exodus file id
 * \param      num_sset                number of sidesets
 * \param      num_sset_var            number of sideset variables
 * \param     *sset_var_tab            sideset variable truth table array
 * \deprecated Use ex_put_truth_table()(exoid, EX_SIDE_SET, num_sset, num_sset_var, sset_var_tab)
 */

int ex_put_sset_var_tab (int  exoid,
                         int  num_sset,
                         int  num_sset_var,
                         int *sset_var_tab)
{
  return ex_put_truth_table(exoid, EX_SIDE_SET, num_sset, num_sset_var, sset_var_tab);
}


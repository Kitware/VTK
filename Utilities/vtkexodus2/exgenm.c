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
* exgenm - ex_get_elem_num_map
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*       int*    elem_map                element number map array
*
* revision history - 
*
*****************************************************************************/

#include "exodusII.h"

/*!
\deprecated Use ex_get_id_map()(exoid, EX_ELEM_MAP, elem_map)

The function ex_get_elem_num_map() reads the optional element number
map from the database. See Section LocalElementIds for a description of
the element number map. If an element number map is not stored in the
data file, a default array (1,2,3,. .. \c num_elem) is
returned. Memory must be allocated for the element number map array
({num_elem} in length) before this call is made.

\return In case of an error, ex_get_elem_num_map() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  if an element number map is not stored, a default map and a warning value are returned.

\param[in]   exoid     exodus file ID returned from a previous call to ex_create() or ex_open().
\param[out]  elem_map  Returned element number map.

The following code will read an element number map from an 
open exodus file :
\code
int *elem_map, error, exoid;

\comment{read element number map}
elem_map = (int *) calloc(num_elem, sizeof(int));
error = ex_get_elem_num_map (exoid, elem_map);

\comment{Equivalent using non-deprecated function}
error = ex_get_id_map(exoid, EX_ELEM_MAP, elem_map);
\endcode
 */

int ex_get_elem_num_map (int  exoid,
       int *elem_map)
{
  return ex_get_id_map(exoid, EX_ELEM_MAP, elem_map);
}

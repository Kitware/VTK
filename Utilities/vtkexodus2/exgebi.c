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

#include "exodusII.h"

/*!
\deprecated Use ex_get_ids()(exoid, EX_ELEM_BLOCK, ids) instead

The function ex_get_elem_blk_ids() reads the IDs of all of the element
blocks. Memory must be allocated for the returned array of
({num_elem_blk}) IDs before this function is invoked. The required
size(\c num_elem_blk) can be determined via a call to ex_inquire() or
ex_inquire_int().

\return In case of an error, ex_get_elem_blk_ids() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()


\param[in]   exoid         exodus file ID returned from a previous call to ex_create() or ex_open().
\param[out]  ids           Returned array of the element blocks IDs. The order of the IDs in this
                           array reflects the sequence that the element blocks were introduced
                           into the file.

The following code segment reads all the element block IDs:

\code
int error, exoid, *idelbs, num_elem_blk;
idelbs = (int *) calloc(num_elem_blk, sizeof(int));

error = ex_get_elem_blk_ids (exoid, idelbs);

\comment{Same result using non-deprecated functions}
error = ex_get_ids (exoid, EX_ELEM_BLOCK, idelbs);

\endcode


 */

int ex_get_elem_blk_ids (int  exoid,
                         int *ids)
{
  /* ex_get_elem_blk_ids should be deprecated. */
  return ex_get_ids( exoid, EX_ELEM_BLOCK, ids );
}

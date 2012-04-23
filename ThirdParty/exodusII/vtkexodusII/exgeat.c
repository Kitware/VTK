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

#include "exodusII.h"

/*!
\deprecated Use ex_get_attr()(exoid, EX_ELEM_BLOCK, elem_blk_id, attrib) instead

The function ex_get_elem_attr() reads the attributes for an element
block. Memory must be allocated for(\c num_attr x \c num_elem_this_blk)
attributes before this routine is called.

Because the attributes are floating point values, the application code
must declare the array passed to be the appropriate type (\c float or
\c double) to match the compute word size passed in ex_create() or
ex_open().

\return In case of an error, ex_get_elem_attr() returns a negative
number; a warning will return a positive number.  Possible causes of
errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  invalid element block ID.
  -  a warning value is returned if no attributes are stored in the file.

\param[in]  exoid         exodus file ID returned from a previous call to ex_create() or ex_open().
\param[in]  elem_blk_id   The element block ID.
\param[out] attrib        Size [num_elem_this_blk*num_attr].
                          Returned list of(\c num_attr x num_elem_this_blk) attributes for
        the element block, with the \c num_attr index cycling faster.


Refer to the code example in ex_get_elem_block() for an example
of reading the element attributes for an element block.
 */

int ex_get_elem_attr (int   exoid,
                      int   elem_blk_id,
                      void *attrib)
{
  return ex_get_attr( exoid, EX_ELEM_BLOCK, elem_blk_id, attrib );
}

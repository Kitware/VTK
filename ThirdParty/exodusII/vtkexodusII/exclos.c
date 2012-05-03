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
* exclos - ex_close
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*
* revision history - 
*
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

extern char *ret_string;      /* cf ex_utils.c */

/*!

The function ex_close() updates and then closes an open exodus file.

\return In case of an error, ex_close() returns a negative number; a
        warning will return a positive number. Possible causes of errors
  include:
 - data file not properly opened with call to ex_create() or ex_open()

 \param exoid      exodus file ID returned from a previous call to ex_create() or ex_open().

The following code segment closes an open exodus file:

\code
int error,exoid;
error = ex_close (exoid);
\endcode

 */
int ex_close (int exoid)
{
   char errmsg[MAX_ERR_LENGTH];
   int status;
   
   exerrval = 0; /* clear error code */

   if ((status = nc_sync(exoid)) != NC_NOERR) {
     exerrval = status;
     sprintf(errmsg,"Error: failed to update file id %d",exoid);
     ex_err("ex_close",errmsg,exerrval);
     return(EX_FATAL);
   }
   if ((status = nc_close (exoid)) == NC_NOERR) {
     ex_conv_exit(exoid);

     ex_rm_file_item(exoid, ex_get_counter_list(EX_ELEM_BLOCK));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_FACE_BLOCK));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_EDGE_BLOCK));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_NODE_SET));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_EDGE_SET));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_FACE_SET));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_SIDE_SET));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_ELEM_SET));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_NODE_MAP));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_EDGE_MAP));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_FACE_MAP));
     ex_rm_file_item(exoid, ex_get_counter_list(EX_ELEM_MAP));

     ex_rm_stat_ptr (exoid, &exoII_ed);
     ex_rm_stat_ptr (exoid, &exoII_fa);
     ex_rm_stat_ptr (exoid, &exoII_eb);
     ex_rm_stat_ptr (exoid, &exoII_ns);
     ex_rm_stat_ptr (exoid, &exoII_es);
     ex_rm_stat_ptr (exoid, &exoII_fs);
     ex_rm_stat_ptr (exoid, &exoII_ss);
     ex_rm_stat_ptr (exoid, &exoII_els);
     ex_rm_stat_ptr (exoid, &exoII_nm);
     ex_rm_stat_ptr (exoid, &exoII_edm);
     ex_rm_stat_ptr (exoid, &exoII_fam);
     ex_rm_stat_ptr (exoid, &exoII_em);
   }
   else {
     exerrval = status;
     sprintf(errmsg, "Error: failed to close file id %d",exoid);
     ex_err("ex_close",errmsg, status);
     return(EX_FATAL);
   }
   return(EX_NOERR);
}

/*
 * Copyright (c) 1994 Sandia Corporation. Under the terms of Contract
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
* author - Sandia National Laboratories
*          Larry A. Schoof - Original
*          James A. Schutt - 8 byte float and standard C definitions
*          Vic Yarberry    - Added headers and error logging
*
*          
* environment - UNIX
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

struct list_item* eb_ctr_list = 0;
extern char *ret_string;      /* cf ex_utils.c */

/*
 * updates and then closes an open EXODUS II file
 */

int ex_close (int exoid)
{
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

   if (ncsync (exoid) == -1) 
   {
     exerrval = ncerr;
     sprintf(errmsg,"Error: failed to update file id %d",exoid);
     ex_err("ex_close",errmsg,exerrval);
     return(EX_FATAL);
   }
   if (ncclose (exoid) >= 0 )
   {
     ex_conv_exit(exoid);
     ex_rm_file_item(exoid, &eb_ctr_list);
     ex_rm_file_item(exoid, &ns_ctr_list);
     ex_rm_file_item(exoid, &ss_ctr_list);
     ex_rm_file_item(exoid, &em_ctr_list);
     ex_rm_file_item(exoid, &nm_ctr_list);

     rm_stat_ptr (exoid, &eb);
     rm_stat_ptr (exoid, &ns);
     rm_stat_ptr (exoid, &ss);
     rm_stat_ptr (exoid, &em);
     rm_stat_ptr (exoid, &nm);
   }
   else
   {
     exerrval = ncerr;
     sprintf(errmsg, "Error: failed to close file id %d",exoid);
     ex_err("ex_close",errmsg,ncerr);
     return(EX_FATAL);
   }

   return(EX_NOERR);
}

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
* exgssv - ex_get_nset_var
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
*       int     time_step               time step number
*       int     nset_var_index          nodeset variable index
*       int     nset_blk_id             nodeset id
*       int     num_node_this_nset      number of nodes in this nodeset
*       
*
* exit conditions - 
*       float*  nset_var_vals           array of nodeset variable values
*
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the values of a single nodeset variable for one nodeset at 
 * one time step in the database; assume the first time step and
 * nodeset variable index is 1
 */

int ex_get_nset_var (int   exoid,
                     int   time_step,
                     int   nset_var_index,
                     int   nset_id, 
                     int   num_node_this_nset,
                     void *nset_var_vals)
{
   int varid, nset_id_ndx;
   long start[2], count[2];
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

  /* Determine index of nset_id in VAR_NS_IDS array */
  nset_id_ndx = ex_id_lkup(exoid,VAR_NS_IDS,nset_id);
  if (exerrval != 0) 
  {
    if (exerrval == EX_NULLENTITY)
    {
      sprintf(errmsg,
              "Warning: no nodeset variables for NULL nodeset %d in file id %d",
              nset_id,exoid);
      ex_err("ex_get_nset_var",errmsg,EX_MSG);
      return (EX_WARN);
    }
    else
    {
      sprintf(errmsg,
     "Error: failed to locate nodeset id %d in %s variable in file id %d",
              nset_id, VAR_ID_EL_BLK, exoid);
      ex_err("ex_get_nset_var",errmsg,exerrval);
      return (EX_FATAL);
    }
  }


/* inquire previously defined variable */

   if((varid=ncvarid(exoid,VAR_NS_VAR(nset_var_index,nset_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
          "Error: failed to locate nodeset variable %d for nodeset %d in file id %d",
          nset_var_index,nset_id,exoid); /* this msg needs to be improved */
     ex_err("ex_get_nset_var",errmsg,exerrval);
     return (EX_FATAL);
   }

/* read values of nodeset variable */

   start[0] = --time_step;
   start[1] = 0;

   count[0] = 1;
   count[1] = num_node_this_nset;

   if (ncvarget (exoid, varid, start, count,
        ex_conv_array(exoid,RTN_ADDRESS,nset_var_vals,num_node_this_nset)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
        "Error: failed to get nodeset variable %d for nodeset %d in file id %d",
             nset_var_index,nset_id,exoid);/*this msg needs to be improved*/
     ex_err("ex_get_nset_var",errmsg,exerrval);
     return (EX_FATAL);
   }


   ex_conv_array( exoid, READ_CONVERT, nset_var_vals, num_node_this_nset );

   return (EX_NOERR);
}

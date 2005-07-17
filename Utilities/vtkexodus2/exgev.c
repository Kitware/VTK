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
* exgev - ex_get_elem_var
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
*       int     elem_var_index          element variable index
*       int     elem_blk_id             element block id
*       int     num_elem_this_blk       number of elements in this block
*       
*
* exit conditions - 
*       float*  elem_var_vals           array of element variable values
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
 * reads the values of a single element variable for one element block at 
 * one time step in the database; assume the first time step and
 * element variable index is 1
 */

int ex_get_elem_var (int   exoid,
                     int   time_step,
                     int   elem_var_index,
                     int   elem_blk_id, 
                     int   num_elem_this_blk,
                     void *elem_var_vals)
{
   int varid, elem_blk_id_ndx;
   long start[2], count[2];
   char errmsg[MAX_ERR_LENGTH];

   exerrval = 0; /* clear error code */

  /* Determine index of elem_blk_id in VAR_ID_EL_BLK array */
  elem_blk_id_ndx = ex_id_lkup(exoid,VAR_ID_EL_BLK,elem_blk_id);
  if (exerrval != 0) 
  {
    if (exerrval == EX_NULLENTITY)
    {
      sprintf(errmsg,
              "Warning: no element variables for NULL block %d in file id %d",
              elem_blk_id,exoid);
      ex_err("ex_get_elem_var",errmsg,EX_MSG);
      return (EX_WARN);
    }
    else
    {
      sprintf(errmsg,
     "Error: failed to locate element block id %d in %s variable in file id %d",
              elem_blk_id, VAR_ID_EL_BLK, exoid);
      ex_err("ex_get_elem_var",errmsg,exerrval);
      return (EX_FATAL);
    }
  }


/* inquire previously defined variable */

   if((varid=ncvarid(exoid,VAR_ELEM_VAR(elem_var_index,elem_blk_id_ndx))) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
          "Error: failed to locate elem var %d for elem block %d in file id %d",
          elem_var_index,elem_blk_id,exoid); /* this msg needs to be improved */
     ex_err("ex_get_elem_var",errmsg,exerrval);
     return (EX_FATAL);
   }

/* read values of element variable */

   start[0] = --time_step;
   start[1] = 0;

   count[0] = 1;
   count[1] = num_elem_this_blk;

   if (ncvarget (exoid, varid, start, count,
        ex_conv_array(exoid,RTN_ADDRESS,elem_var_vals,num_elem_this_blk)) == -1)
   {
     exerrval = ncerr;
     sprintf(errmsg,
        "Error: failed to get elem var %d for block %d in file id %d",
             elem_var_index,elem_blk_id,exoid);/*this msg needs to be improved*/
     ex_err("ex_get_elem_var",errmsg,exerrval);
     return (EX_FATAL);
   }


   ex_conv_array( exoid, READ_CONVERT, elem_var_vals, num_elem_this_blk );

   return (EX_NOERR);
}

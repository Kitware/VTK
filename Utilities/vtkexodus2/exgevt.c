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
* exgevt - ex_get_elem_var_time
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
*       int     elem_var_index          element variable index
*       int     elem_number             element number
*       int     beg_time_step           time step number
*       int     end_time_step           time step number
*
* exit conditions - 
*
* revision history - 
*       float*  elem_var_vals           array of element variable values
*
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * reads the values of an element variable for a single element through a 
 * specified number of time steps in the database; assume the first element
 * variable index, element number, and time step are 1
 */

int ex_get_elem_var_time (int   exoid,
                          int   elem_var_index,
                          int   elem_number,
                          int   beg_time_step, 
                          int   end_time_step,
                          void *elem_var_vals)
{
  int i, dimid, varid, numel = 0, offset;
  nclong *elem_blk_ids, *stat_vals;
  long num_elem_blocks, num_el_this_blk = 0, start[2], count[2];
  float fdum;
  char *cdum;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  cdum = 0; /* initialize even though it is not used */

  /* assume element number is 1-based (the first element number is 1);
   * adjust so it is 0-based
   */
  elem_number--;

  /* find what element block the element is in */

  /* first, find out how many element blocks there are */

  if ((dimid = ncdimid (exoid, DIM_NUM_EL_BLK)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate number of element blocks in file id %d", 
              exoid);
      ex_err("ex_get_elem_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }

  if (ncdiminq (exoid, dimid, (char *) 0, &num_elem_blocks) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get number of element blocks in file id %d",
              exoid);
      ex_err("ex_get_elem_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* get the array of element block ids */
  /* don't think we need this anymore since the netcdf variable names 
     associated with element blocks don't contain the element block ids */

  if (!(elem_blk_ids = malloc(num_elem_blocks*sizeof(nclong))))
    {
      exerrval = EX_MEMFAIL;
      sprintf(errmsg,
              "Error: failed to allocate memory for element block ids for file id %d",
              exoid);
      ex_err("ex_get_elem_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }


  if ((varid = ncvarid (exoid, VAR_ID_EL_BLK)) == -1)
    {
      exerrval = ncerr;
      free(elem_blk_ids);
      sprintf(errmsg,
              "Error: failed to locate element block ids in file id %d", exoid);
      ex_err("ex_get_elem_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }


  start[0] = 0;
  count[0] = num_elem_blocks;
  if (ncvarget (exoid, varid, start, count, elem_blk_ids) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get element block ids from file id %d", exoid);
      ex_err("ex_get_elem_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* allocate space for stat array */
  if (!(stat_vals = malloc((int)num_elem_blocks*sizeof(nclong))))
    {
      exerrval = EX_MEMFAIL;
      free (elem_blk_ids);
      sprintf(errmsg,
              "Error: failed to allocate memory for element block status array for file id %d",
              exoid);
      ex_err("ex_get_elem_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* get variable id of status array */
  if ((varid = ncvarid (exoid, VAR_STAT_EL_BLK)) != -1)
    {
      /* if status array exists, use it, otherwise assume, object exists
         to be backward compatible */

      start[0] = 0;
      start[1] = 0;
      count[0] = num_elem_blocks;
      count[1] = 0;

      if (ncvarget (exoid, varid, start, count, (void *)stat_vals) == -1)
        {
          exerrval = ncerr;
          free (elem_blk_ids);
          free(stat_vals);
          sprintf(errmsg,
                  "Error: failed to get element block status array from file id %d",
                  exoid);
          ex_err("ex_get_elem_var_time",errmsg,exerrval);
          return (EX_FATAL);
        }
    }
  else /* default: status is true */
    for(i=0;i<num_elem_blocks;i++)
      stat_vals[i]=1;




  /* loop through each element block until elem_number is found;  since element
   * numbers are sequential (beginning with 1) elem_number is in element_block_i
   * when elem_number_first_i <= elem_number <= elem_number_last_i, where
   * elem_number_first_i is the element number of the first element in 
   * element_block_i and elem_number_last_i is the element number of the last
   * element in element_block_i
   */

  i = 0;
  if (stat_vals[i] != 0)  {
    if ((dimid = ncdimid (exoid, DIM_NUM_EL_IN_BLK(i+1))) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate number of elements in block %d in file id %d",
              elem_blk_ids[i], exoid);
      ex_err("ex_get_elem_var_time",errmsg,exerrval);
      free(stat_vals);
      free(elem_blk_ids);
      return (EX_FATAL);
    }

    if (ncdiminq (exoid, dimid, (char *) 0, &num_el_this_blk) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get number of elements in block %d in file id %d",
              elem_blk_ids[i], exoid);
      ex_err("ex_get_elem_var_time",errmsg,exerrval);
      free(stat_vals);
      free(elem_blk_ids);
      return (EX_FATAL);
    }

  } /* End NULL element block check */

  numel = num_el_this_blk;

  while (numel <= elem_number) {
    if (stat_vals[++i] != 0) {
      if ((dimid = ncdimid(exoid,DIM_NUM_EL_IN_BLK(i+1))) == -1) {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to locate number of elements in block %d in file id %d",
                elem_blk_ids[i], exoid);
        ex_err("ex_get_elem_var_time",errmsg,exerrval);
        free(stat_vals);
        free(elem_blk_ids);
        return (EX_FATAL);
      }

      if (ncdiminq (exoid, dimid, (char *) 0, &num_el_this_blk) == -1) {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to get number of elements in block %d in file id %d",
                elem_blk_ids[i], exoid);
        ex_err("ex_get_elem_var_time",errmsg,exerrval);
        free(stat_vals);
        free(elem_blk_ids);
        return (EX_FATAL);
      }

      numel += num_el_this_blk;
    }
  }

  offset = elem_number - (numel - num_el_this_blk);

  /* inquire previously defined variable */

  if((varid=ncvarid(exoid,VAR_ELEM_VAR(elem_var_index+1,i+1))) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to locate elem variable %d for elem block %d in file id %d",
            elem_var_index,elem_blk_ids[i],exoid);
    ex_err("ex_get_elem_var_time",errmsg,exerrval);
    free(stat_vals);
    free(elem_blk_ids);
    return (EX_FATAL);
  }

  free(stat_vals);
  free(elem_blk_ids);

  /* read values of element variable */

  start[0] = --beg_time_step;
  start[1] = offset;

  if (end_time_step < 0) {

    /* user is requesting the maximum time step;  we find this out using the
     * database inquire function to get the number of time steps;  the ending
     * time step number is 1 less due to 0 based array indexing in C
     */
    if (ex_inquire (exoid, EX_INQ_TIME, &end_time_step, &fdum, cdum) == -1) {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get maximum time step in file id %d",
              exoid);
      ex_err("ex_get_elem_var_time",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  end_time_step--;

  count[0] = end_time_step - beg_time_step + 1;
  count[1] = 1;

  if (ncvarget (exoid, varid, start, count,
                ex_conv_array(exoid,RTN_ADDRESS,elem_var_vals,count[0])) == -1) {
    exerrval = ncerr;
    sprintf(errmsg,
            "Error: failed to get elem variable values in file id %d", exoid);
    ex_err("ex_get_elem_var_time",errmsg,exerrval);
    return (EX_FATAL);
  }

  ex_conv_array( exoid, READ_CONVERT, elem_var_vals, count[0] );

  return (EX_NOERR);
}

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
* expev - ex_put_elem_var
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
* exit conditions -
*
*
* exit conditions - 
*
* revision history - 
*
*  Id
*
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

/*
 * writes the values of a single element variable for one element block at 
 * one time step to the database; assume the first time step and 
 * element variable index are 1
 */

int ex_put_elem_var (int   exoid,
                     int   time_step,
                     int   elem_var_index,
                     int   elem_blk_id,
                     int   num_elem_this_blk,
                     const void *elem_var_vals)
{
  int varid, dimid,time_dim, numelbdim, dims[2], elem_blk_id_ndx;
  long num_elem_blk, num_elem_var, start[2], count[2];
  nclong *elem_var_tab;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* Determine index of elem_blk_id in VAR_ID_EL_BLK array */
  elem_blk_id_ndx = ex_id_lkup(exoid,VAR_ID_EL_BLK,elem_blk_id);
  if (exerrval != 0) 
  {
    if (exerrval == EX_NULLENTITY)
    {
      sprintf(errmsg,
              "Warning: no variables allowed for NULL block %d in file id %d",
              elem_blk_id,exoid);
      ex_err("ex_put_elem_var",errmsg,EX_MSG);
      return (EX_WARN);
    }
    else
    {
    sprintf(errmsg,
        "Error: failed to locate element block id %d in %s array in file id %d",
            elem_blk_id, VAR_ID_EL_BLK, exoid);
    ex_err("ex_put_elem_var",errmsg,exerrval);
    return (EX_FATAL);
    }
  }

  if ((varid = ncvarid (exoid,
                        VAR_ELEM_VAR(elem_var_index,elem_blk_id_ndx))) == -1)
  {
    if (ncerr == NC_ENOTVAR) /* variable doesn't exist, create it! */
    {

/*    inquire previously defined dimensions */

      /* check for the existance of an element variable truth table */
      if ((varid = ncvarid (exoid, VAR_ELEM_TAB)) != -1)
      {
        /* find out number of element blocks and element variables */
        if ((dimid = ncdimid (exoid, DIM_NUM_EL_BLK)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
               "Error: failed to locate number of element blocks in file id %d",
                  exoid);
          ex_err("ex_put_elem_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if (ncdiminq (exoid, dimid, (char *) 0, &num_elem_blk) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                 "Error: failed to get number of element blocks in file id %d",
                  exoid);
          ex_err("ex_put_elem_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if ((dimid = ncdimid (exoid, DIM_NUM_ELE_VAR)) == -1)
        {
          exerrval = EX_BADPARAM;
          sprintf(errmsg,
               "Error: no element variables stored in file id %d",
                  exoid);
          ex_err("ex_put_elem_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if (ncdiminq (exoid, dimid, (char *) 0, &num_elem_var) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
               "Error: failed to get number of element variables in file id %d",
                  exoid);
          ex_err("ex_put_elem_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if (!(elem_var_tab = malloc(num_elem_blk*num_elem_var*sizeof(nclong))))
        {
          exerrval = EX_MEMFAIL;
          sprintf(errmsg,
                 "Error: failed to allocate memory for element variable truth table in file id %d",
                  exoid);
          ex_err("ex_put_elem_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        /*   read in the element variable truth table */

        start[0] = 0;
        start[1] = 0;

        count[0] = num_elem_blk;
        count[1] = num_elem_var;

        if (ncvarget (exoid, varid, start, count, elem_var_tab) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                 "Error: failed to get truth table from file id %d", exoid);
          ex_err("ex_put_elem_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if(elem_var_tab[num_elem_var*(elem_blk_id_ndx-1)+elem_var_index-1] 
           == 0L)
        {
          free(elem_var_tab);
          exerrval = EX_BADPARAM;
          sprintf(errmsg,
              "Error: Invalid element variable %d, block %d in file id %d",
                  elem_var_index, elem_blk_id, exoid);
          ex_err("ex_put_elem_var",errmsg,exerrval);
          return (EX_FATAL);
        }
        free(elem_var_tab);
      }

      if ((time_dim = ncdimid (exoid, DIM_TIME)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
               "Error: failed to locate time dimension in file id %d", exoid);
        ex_err("ex_put_elem_var",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }

      if ((numelbdim=ncdimid(exoid, DIM_NUM_EL_IN_BLK(elem_blk_id_ndx))) == -1)
      {
        if (ncerr == NC_EBADDIM)
        {
          exerrval = ncerr;
          sprintf(errmsg,
      "Error: number of elements in element block %d not defined in file id %d",
                  elem_blk_id, exoid);
          ex_err("ex_put_elem_var",errmsg,exerrval);
        }
        else
        {
          exerrval = ncerr;
          sprintf(errmsg,
 "Error: failed to locate number of elements in element block %d in file id %d",
                  elem_blk_id, exoid);
          ex_err("ex_put_elem_var",errmsg,exerrval);
        }
        goto error_ret;
      }

/*    variable doesn't exist so put file into define mode  */

      if (ncredef (exoid) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
               "Error: failed to put file id %d into define mode", exoid);
        ex_err("ex_put_elem_var",errmsg,exerrval);
        return (EX_FATAL);
      }


/*    define netCDF variable to store element variable values */

      dims[0] = time_dim;
      dims[1] = numelbdim;
      if ((varid = ncvardef(exoid,VAR_ELEM_VAR(elem_var_index,elem_blk_id_ndx),
                            nc_flt_code(exoid), 2, dims)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
               "Error: failed to define element variable %d in file id %d",
                elem_var_index,exoid);
        ex_err("ex_put_elem_var",errmsg,exerrval);
        goto error_ret;
      }


/*    leave define mode  */

      if (ncendef (exoid) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
       "Error: failed to complete element variable %s definition to file id %d",
                VAR_ELEM_VAR(elem_var_index,elem_blk_id_ndx), exoid);
        ex_err("ex_put_elem_var",errmsg,exerrval);
        return (EX_FATAL);
      }
    }
    else
    {
      exerrval = ncerr;
      sprintf(errmsg,
             "Error: failed to locate element variable %s in file id %d",
              VAR_ELEM_VAR(elem_var_index,elem_blk_id_ndx),exoid);
      ex_err("ex_put_elem_var",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

/* store element variable values */

  start[0] = --time_step;
  start[1] = 0;

  count[0] = 1;
  count[1] = num_elem_this_blk;

  if (ncvarput (exoid, varid, start, count, 
                ex_conv_array(exoid,WRITE_CONVERT,elem_var_vals,
                num_elem_this_blk)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to store element variable %d in file id %d", 
            elem_var_index,exoid);
    ex_err("ex_put_elem_var",errmsg,exerrval);
    return (EX_FATAL);
  }

  return (EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  if (ncendef (exoid) == -1)     /* exit define mode */
  {
    sprintf(errmsg,
           "Error: failed to complete definition for file id %d",
            exoid);
    ex_err("ex_put_elem_var",errmsg,exerrval);
  }
  return (EX_FATAL);
}

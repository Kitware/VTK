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
* expev - ex_put_sset_var
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
*       int     sset_var_index          sideset variable index
*       int     sset_id                 sideset id
*       int     num_faces_this_sset     number of faces in this sideset
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

/*!
 * writes the values of a single sideset variable for one sideset at 
 * one time step to the database; assume the first time step and 
 * sideset variable index are 1
 */

int ex_put_sset_var (int   exoid,
                     int   time_step,
                     int   sset_var_index,
                     int   sset_id,
                     int   num_faces_this_sset,
                     const void *sset_var_vals)
{
  int varid, dimid,time_dim, numelbdim, dims[2], sset_id_ndx;
  long num_ssets, num_sset_var, start[2], count[2];
  nclong *sset_var_tab;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* Determine index of sset_id in VAR_SS_ID array */
  sset_id_ndx = ex_id_lkup(exoid,VAR_SS_IDS,sset_id);
  if (exerrval != 0) 
  {
    if (exerrval == EX_NULLENTITY)
    {
      sprintf(errmsg,
              "Warning: no variables allowed for NULL sideset %d in file id %d",
              sset_id,exoid);
      ex_err("ex_put_sset_var",errmsg,EX_MSG);
      return (EX_WARN);
    }
    else
    {
    sprintf(errmsg,
        "Error: failed to locate sideset id %d in %s array in file id %d",
            sset_id, VAR_SS_IDS, exoid);
    ex_err("ex_put_sset_var",errmsg,exerrval);
    return (EX_FATAL);
    }
  }

  if ((varid = ncvarid (exoid,
                        VAR_SS_VAR(sset_var_index,sset_id_ndx))) == -1)
  {
    if (ncerr == NC_ENOTVAR) /* variable doesn't exist, create it! */
    {

/*    inquire previously defined dimensions */

      /* check for the existance of an sideset variable truth table */
      if ((varid = ncvarid (exoid, VAR_SSET_TAB)) != -1)
      {
        /* find out number of sidesets and sideset variables */
        if ((dimid = ncdimid (exoid, DIM_NUM_SS)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
               "Error: failed to locate number of sidesets in file id %d",
                  exoid);
          ex_err("ex_put_sset_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if (ncdiminq (exoid, dimid, (char *) 0, &num_ssets) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                 "Error: failed to get number of sidesets in file id %d",
                  exoid);
          ex_err("ex_put_sset_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if ((dimid = ncdimid (exoid, DIM_NUM_SSET_VAR)) == -1)
        {
          exerrval = EX_BADPARAM;
          sprintf(errmsg,
               "Error: no sideset variables stored in file id %d",
                  exoid);
          ex_err("ex_put_sset_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if (ncdiminq (exoid, dimid, (char *) 0, &num_sset_var) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
               "Error: failed to get number of sideset variables in file id %d",
                  exoid);
          ex_err("ex_put_sset_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if (!(sset_var_tab = malloc(num_ssets*num_sset_var*sizeof(nclong))))
        {
          exerrval = EX_MEMFAIL;
          sprintf(errmsg,
                 "Error: failed to allocate memory for sideset variable truth table in file id %d",
                  exoid);
          ex_err("ex_put_sset_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        /*   read in the sideset variable truth table */

        start[0] = 0;
        start[1] = 0;

        count[0] = num_ssets;
        count[1] = num_sset_var;

        if (ncvarget (exoid, varid, start, count, sset_var_tab) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                 "Error: failed to get truth table from file id %d", exoid);
          ex_err("ex_put_sset_var",errmsg,exerrval);
          return (EX_FATAL);
        }

        if(sset_var_tab[num_sset_var*(sset_id_ndx-1)+sset_var_index-1] 
           == 0L)
        {
          free(sset_var_tab);
          exerrval = EX_BADPARAM;
          sprintf(errmsg,
              "Error: Invalid sideset variable %d, sideset %d in file id %d",
                  sset_var_index, sset_id, exoid);
          ex_err("ex_put_sset_var",errmsg,exerrval);
          return (EX_FATAL);
        }
        free(sset_var_tab);
      }

      if ((time_dim = ncdimid (exoid, DIM_TIME)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
               "Error: failed to locate time dimension in file id %d", exoid);
        ex_err("ex_put_sset_var",errmsg,exerrval);
        goto error_ret;         /* exit define mode and return */
      }

      if ((numelbdim=ncdimid(exoid, DIM_NUM_SIDE_SS(sset_id_ndx))) == -1)
      {
        if (ncerr == NC_EBADDIM)
        {
          exerrval = ncerr;
          sprintf(errmsg,
      "Error: number of faces in sideset %d not defined in file id %d",
                  sset_id, exoid);
          ex_err("ex_put_sset_var",errmsg,exerrval);
        }
        else
        {
          exerrval = ncerr;
          sprintf(errmsg,
 "Error: failed to locate number of sides in sideset %d in file id %d",
                  sset_id, exoid);
          ex_err("ex_put_sset_var",errmsg,exerrval);
        }
        goto error_ret;
      }

/*    variable doesn't exist so put file into define mode  */

      if (ncredef (exoid) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
               "Error: failed to put file id %d into define mode", exoid);
        ex_err("ex_put_sset_var",errmsg,exerrval);
        return (EX_FATAL);
      }


/*    define netCDF variable to store sideset variable values */

      dims[0] = time_dim;
      dims[1] = numelbdim;
      if ((varid = ncvardef(exoid,VAR_SS_VAR(sset_var_index,sset_id_ndx),
                            nc_flt_code(exoid), 2, dims)) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
               "Error: failed to define sideset variable %d in file id %d",
                sset_var_index,exoid);
        ex_err("ex_put_sset_var",errmsg,exerrval);
        goto error_ret;
      }


/*    leave define mode  */

      if (ncendef (exoid) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
       "Error: failed to complete sideset variable %s definition to file id %d",
                VAR_SS_VAR(sset_var_index,sset_id_ndx), exoid);
        ex_err("ex_put_sset_var",errmsg,exerrval);
        return (EX_FATAL);
      }
    }
    else
    {
      exerrval = ncerr;
      sprintf(errmsg,
             "Error: failed to locate sideset variable %s in file id %d",
              VAR_SS_VAR(sset_var_index,sset_id_ndx),exoid);
      ex_err("ex_put_sset_var",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

/* store sideset variable values */

  start[0] = --time_step;
  start[1] = 0;

  count[0] = 1;
  count[1] = num_faces_this_sset;

  if (ncvarput (exoid, varid, start, count, 
                ex_conv_array(exoid,WRITE_CONVERT,sset_var_vals,
                num_faces_this_sset)) == -1)
  {
    exerrval = ncerr;
    sprintf(errmsg,
           "Error: failed to store sideset variable %d in file id %d", 
            sset_var_index,exoid);
    ex_err("ex_put_sset_var",errmsg,exerrval);
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
    ex_err("ex_put_sset_var",errmsg,exerrval);
  }
  return (EX_FATAL);
}

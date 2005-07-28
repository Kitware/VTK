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
* expvp - ex_put_concat_var_param
*
* author - Sandia National Laboratories
*          
* entry conditions - 
*   input parameters:
*       int     exoid   exodus file id
*       int     num_g   global variable count
*       int     num_n   nodal variable count
*       int     num_e   element variable count
*       int     num_elem_blk            number of element blocks
*       int*    elem_var_tab            element variable truth table array
*
* exit conditions - 
*
* revision history - 
*
*  Id
*****************************************************************************/

#include <stdlib.h>
#include "exodusII.h"
#include "exodusII_int.h"

#include <ctype.h>
/*
 * writes the number of global, nodal, and element variables 
 * that will be written to the database
 */

int ex_put_concat_var_param (int   exoid,
                             int   num_g,
                             int   num_n,
                             int   num_e,
                             int   num_elem_blk,
                             int  *elem_var_tab)
{
  int time_dim, num_nod_dim, dimid, strdim, varid, iresult;
  long idum, start[2], count[2]; 
  int numelblkdim, numelvardim;
  nclong *stat_vals, *lptr;
  int i, j, k, id, *ids;
  int dims[3];
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* inquire previously defined dimensions  */

  if ((time_dim = ncdimid (exoid, DIM_TIME)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate time dimension in file id %d", exoid);
      ex_err("ex_put_concat_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }

  if ((num_nod_dim = ncdimid (exoid, DIM_NUM_NODES)) == -1)
    {
      if (num_n > 0) {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to locate number of nodes in file id %d",
                exoid);
        ex_err("ex_put_concat_var_param",errmsg,exerrval);
        return (EX_FATAL);
      }
    }

  if ((strdim = ncdimid (exoid, DIM_STR)) < 0)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get string length in file id %d",exoid);
      ex_err("ex_put_concat_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* put file into define mode  */

  if (num_e > 0) {
    if ((numelblkdim = ncdimid (exoid, DIM_NUM_EL_BLK)) == -1)
      {
        if (ncerr == NC_EBADDIM)
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: no element blocks defined in file id %d",
                    exoid);
            ex_err("ex_put_concat_var_param",errmsg,exerrval);
          }
        else
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: failed to locate number of element blocks in file id %d",
                    exoid);
            ex_err("ex_put_concat_var_param",errmsg,exerrval);
          }
        return (EX_FATAL);
      }

    if (ncdiminq (exoid, numelblkdim, (char *) 0, &idum) == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to get number of element blocks in file id %d",
                exoid);
        ex_err("ex_put_concat_var_param",errmsg,exerrval);
        return (EX_FATAL);
      }


    if (idum != num_elem_blk)
      {
        exerrval = EX_FATAL;
        sprintf(errmsg,
                "Error: # of element blocks doesn't match those specified in file id %d",
                exoid);
        ex_err("ex_put_concat_var_param",errmsg,exerrval);
        return (EX_FATAL);
      }

    /* get element block IDs */
    if (!(ids = malloc(num_elem_blk*sizeof(int))))
      {
        exerrval = EX_MEMFAIL;
        sprintf(errmsg,
                "Error: failed to allocate memory for element block id array for file id %d",
                exoid);
        ex_err("ex_put_concat_var_param",errmsg,exerrval);
        return (EX_FATAL);
      }
    ex_get_elem_blk_ids (exoid, ids);

    /* Get element block status array for later use */

    if (!(stat_vals = malloc(num_elem_blk*sizeof(nclong))))
      {
        exerrval = EX_MEMFAIL;
        free(ids);
        sprintf(errmsg,
                "Error: failed to allocate memory for element block status array for file id %d",
                exoid);
        ex_err("ex_put_concat_var_param",errmsg,exerrval);
        return (EX_FATAL);
      }

    /* get variable id of status array */
    if ((varid = ncvarid (exoid, VAR_STAT_EL_BLK)) != -1)
      {
        /* if status array exists (V 2.01+), use it, otherwise assume
           object exists to be backward compatible */

        start[0] = 0;
        start[1] = 0;
        count[0] = num_elem_blk;
        count[1] = 0;

        if (ncvarget (exoid, varid, start, count, (void *)stat_vals) == -1)
          {
            exerrval = ncerr;
            free(stat_vals);
            sprintf(errmsg,
                    "Error: failed to get element block status array from file id %d",
                    exoid);
            ex_err("ex_put_concat_var_param",errmsg,exerrval);
            return (EX_FATAL);
          }
      }
    else
      {
        /* status array doesn't exist (V2.00), dummy one up for later checking */
        for(i=0;i<num_elem_blk;i++)
          stat_vals[i] = 1;
      }
  }

  if (ncredef (exoid) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to put file id %d into define mode", exoid);
      ex_err("ex_put_concat_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }


  /* define dimensions and variables */

  if (num_g > 0) 
    {
      if ((dimid = ncdimdef (exoid, DIM_NUM_GLO_VAR, (long)num_g)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: global variable name parameters are already defined in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define number of global variables in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }


      dims[0] = time_dim;
      dims[1] = dimid;
      if ((ncvardef (exoid, VAR_GLO_VAR, 
                     nc_flt_code(exoid), 2, dims)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                  "Error: failed to define global variables in file id %d",
                  exoid);
          ex_err("ex_put_concat_var_param",errmsg,exerrval);
          goto error_ret;          /* exit define mode and return */
        }

      /* Now define global variable name variable */
      dims[0] = dimid;
      dims[1] = strdim;
      if ((ncvardef (exoid, VAR_NAME_GLO_VAR, NC_CHAR, 2, dims)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: global variable names are already defined in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define global variable names in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }

    }

  if (num_n > 0) 
    {
      /*
       * There are two ways to store the nodal variables. The old way *
       * was a blob (#times,#vars,#nodes), but that was exceeding the
       * netcdf maximum dataset size for large models. The new way is
       * to store #vars separate datasets each of size (#times,#nodes)
       *
       * We want this routine to be capable of storing both formats
       * based on some external flag.  Since the storage format of the
       * coordinates have also been changed, we key off of their
       * storage type to decide which method to use for nodal
       * variables. If the variable 'coord' is defined, then store old
       * way; otherwise store new.
       */
      if ((dimid = ncdimdef (exoid, DIM_NUM_NOD_VAR, (long)num_n)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: nodal variable name parameters are already defined in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define number of nodal variables in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }

      if (ex_large_model(exoid) == 0) { /* Old way */
        dims[0] = time_dim;
        dims[1] = dimid;
        dims[2] = num_nod_dim;
        if ((ncvardef (exoid, VAR_NOD_VAR,
                       nc_flt_code(exoid), 3, dims)) == -1)
          {
            exerrval = ncerr;
            sprintf(errmsg,
                    "Error: failed to define nodal variables in file id %d",
                    exoid);
            ex_err("ex_put_concat_var_param",errmsg,exerrval);
            goto error_ret;          /* exit define mode and return */
          }
      } else { /* Store new way */
        for (i = 1; i <= num_n; i++) {
          dims[0] = time_dim;
          dims[1] = num_nod_dim;
          if ((ncvardef (exoid, VAR_NOD_VAR_NEW(i),
                         nc_flt_code(exoid), 2, dims)) == -1)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define nodal variable %d in file id %d",
                      i, exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
              goto error_ret;          /* exit define mode and return */
            }
        }
      }
      /* Now define nodal variable name variable */
      dims[0] = dimid;
      dims[1] = strdim;
      if ((ncvardef (exoid, VAR_NAME_NOD_VAR, NC_CHAR, 2, dims)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: nodal variable names are already defined in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define nodal variable names in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }

    }

  if (num_e > 0) 
    {
      if ((numelvardim = ncdimdef (exoid, DIM_NUM_ELE_VAR, (long)num_e)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: element variable name parameters are already defined in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define number of element variables in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }

      /* Now define element variable name variable */
      dims[0] = numelvardim;
      dims[1] = strdim;
      if ((ncvardef (exoid, VAR_NAME_ELE_VAR, NC_CHAR, 2, dims)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: element variable names are already defined in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define element variable names in file id %d",
                      exoid);
              ex_err("ex_put_concat_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }

      k = 0;
      for (i=0; i<num_elem_blk; i++)
        {
          for (j=1; j<=num_e; j++)
            {

              /* check if element variables are to be put out for this element block */
              if (elem_var_tab[k] != 0)
                {
                  if (stat_vals[i] == 0) /* check for NULL element block */
                    {
                      exerrval = EX_NULLENTITY;
                      elem_var_tab[k] = 0;
                      sprintf(errmsg,
                              "Warning: Element variable truth table specifies invalid entry for NULL element block %d, variable %d in file id %d",
                              ids[i], j, exoid);
                      ex_err("ex_put_concat_var_param",errmsg,exerrval);
                    }
                  else
                    {

                      /* inquire previously defined dimensions */

                      if ((dims[0] = ncdimid (exoid, DIM_TIME)) == -1)
                        {
                          exerrval = ncerr;
                          free(stat_vals);
                          free (ids);
                          sprintf(errmsg,
                                  "Error: failed to locate time variable in file id %d",
                                  exoid);
                          ex_err("ex_put_concat_var_param",errmsg,exerrval);
                          goto error_ret;          /* exit define mode and return */
                        }

                      /* Determine number of elements in block */
                      if ((dims[1] = ncdimid (exoid, DIM_NUM_EL_IN_BLK(i+1))) == -1)
                        {
                          exerrval = ncerr;
                          id=ids[i];
                          free(stat_vals);
                          free (ids);
                          sprintf(errmsg,
                                  "Error: failed to locate number of elements in element block %d in file id %d",
                                  id,exoid);
                          ex_err("ex_put_concat_var_param",errmsg,exerrval);
                          goto error_ret;          /* exit define mode and return */
                        }


                      /* define netCDF variable to store element variable values;
                       * the j index cycles from 1 through the number of element variables so 
                       * that the index of the EXODUS II element variable (which is part of 
                       * the name of the netCDF variable) will begin at 1 instead of 0
                       */

                      if ((varid = ncvardef (exoid, VAR_ELEM_VAR(j,i+1),
                                             nc_flt_code(exoid), 2, dims)) == -1)
                        {
                          if (ncerr != NC_ENAMEINUSE)
                            {
                              exerrval = ncerr;
                              id=ids[i];
                              free(stat_vals);
                              free (ids);
                              sprintf(errmsg,
                                      "Error: failed to define elem variable for element block %d in file id %d",
                                      id,exoid);
                              ex_err("ex_put_concat_var_param",errmsg,exerrval);
                              goto error_ret;   /* exit define mode and return */
                            }
                        }
                    }
                }  /* if */
              k++; /* increment element truth table pointer */
            }  /* for j */
        }  /* for i */

      free (stat_vals);
      free (ids);

      /* create a variable array in which to store the element variable truth
       * table
       */

      dims[0] = numelblkdim;
      dims[1] = numelvardim;

      if ((varid = ncvardef (exoid, VAR_ELEM_TAB, NC_LONG, 2, dims)) == -1)
        {
          exerrval = ncerr;
          sprintf(errmsg,
                  "Error: failed to define element variable truth table in file id %d",
                  exoid);
          ex_err("ex_put_concat_var_param",errmsg,exerrval);
          goto error_ret;          /* exit define mode and return */
        }

    }

  /* leave define mode  */

  if (ncendef (exoid) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to complete definition in file id %d",
              exoid);
      ex_err("ex_put_concat_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* write out the element variable truth table */
  if (num_e > 0) {
    /* this contortion is necessary because netCDF is expecting nclongs; fortunately
       it's necessary only when ints and nclongs aren't the same size */

    start[0] = 0;
    start[1] = 0;
    
    count[0] = num_elem_blk;
    count[1] = num_e;
    
    if (sizeof(int) == sizeof(nclong)) {
      iresult = ncvarput (exoid, varid, start, count, elem_var_tab);
    } else {
      lptr = itol (elem_var_tab, (int)(num_elem_blk*num_e));
      iresult = ncvarput (exoid, varid, start, count, lptr);
      free(lptr);
    }
    
    if (iresult == -1)
      {
        exerrval = ncerr;
        sprintf(errmsg,
                "Error: failed to store element variable truth table in file id %d",
                exoid);
        ex_err("ex_put_concat_var_param",errmsg,exerrval);
        return (EX_FATAL);
      }
  }
  return(EX_NOERR);
  
  /* Fatal error: exit definition mode and return */
 error_ret:
  if (ncendef (exoid) == -1)     /* exit define mode */
    {
      sprintf(errmsg,
              "Error: failed to complete definition for file id %d",
              exoid);
      ex_err("ex_put_concat_var_param",errmsg,exerrval);
    }
  return (EX_FATAL);
}

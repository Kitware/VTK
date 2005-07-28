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
* expvp - ex_put_var_param
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
*       char*   var_type                variable type G,N, or E
*       int*    num_vars                number of variables in database
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

#include <ctype.h>
/*
 * writes the number and names of global, nodal, or element variables 
 * that will be written to the database
 */

int ex_put_var_param (int   exoid,
                      const char *var_type,
                      int   num_vars)
{
  int time_dim, num_nod_dim, dimid, strdim;
  int dims[3];
  char *vptr;
  char errmsg[MAX_ERR_LENGTH];

  exerrval = 0; /* clear error code */

  /* if no variables are to be stored, return with warning */
  if (num_vars == 0)
    {
      exerrval = EX_MSG;
      if (tolower(*var_type) == 'e')
        vptr="element";
      else if (tolower(*var_type) == 'g')
        vptr="global";
      else if (tolower(*var_type) == 'n')
        vptr="nodal";
      else
        vptr="invalid type"; 

      sprintf(errmsg,
              "Warning: zero %s variables specified for file id %d",
              vptr,exoid);
      ex_err("ex_put_var_param",errmsg,exerrval);

      return (EX_WARN);
    }

  /* inquire previously defined dimensions  */

  if ((time_dim = ncdimid (exoid, DIM_TIME)) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate time dimension in file id %d", exoid);
      ex_err("ex_put_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }

  if ((num_nod_dim = ncdimid (exoid, DIM_NUM_NODES)) == -1) {
    if (tolower(*var_type) == 'n') {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to locate number of nodes in file id %d", exoid);
      ex_err("ex_put_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }
  }
  

  if ((strdim = ncdimid (exoid, DIM_STR)) < 0)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to get string length in file id %d",exoid);
      ex_err("ex_put_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }

  /* put file into define mode  */

  if (ncredef (exoid) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to put file id %d into define mode", exoid);
      ex_err("ex_get_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }


  /* define dimensions and variables */

  if (tolower(*var_type) == 'g')
    {
      if ((dimid = ncdimdef (exoid, DIM_NUM_GLO_VAR, (long)num_vars)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: global variable name parameters are already defined in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define number of global variables in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
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
          ex_err("ex_put_var_param",errmsg,exerrval);
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
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define global variable names in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }

    }

  else if (tolower(*var_type) == 'n')
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
      if ((dimid = ncdimdef (exoid, DIM_NUM_NOD_VAR, (long)num_vars)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: nodal variable name parameters are already defined in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define number of nodal variables in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
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
            ex_err("ex_put_var_param",errmsg,exerrval);
            goto error_ret;          /* exit define mode and return */
          }
      } else { /* New way */
        int i;
        for (i = 1; i <= num_vars; i++) {
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
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define nodal variable names in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }

    }

  else if (tolower(*var_type) == 'e')
    {
      if ((dimid = ncdimdef (exoid, DIM_NUM_ELE_VAR, (long)num_vars)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: element variable name parameters are already defined in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define number of element variables in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }

      /* Now define element variable name variable */
      dims[0] = dimid;
      dims[1] = strdim;
      if ((ncvardef (exoid, VAR_NAME_ELE_VAR, NC_CHAR, 2, dims)) == -1)
        {
          if (ncerr == NC_ENAMEINUSE)
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: element variable names are already defined in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          else
            {
              exerrval = ncerr;
              sprintf(errmsg,
                      "Error: failed to define element variable names in file id %d",
                      exoid);
              ex_err("ex_put_var_param",errmsg,exerrval);
            }
          goto error_ret;          /* exit define mode and return */
        }


      /* netCDF variables in which to store the EXODUS element variable values will
       * be defined in ex_put_elem_var_tab or ex_put_elem_var; at this point, we 
       * don't know what element variables are valid for which element blocks 
       * (the info that is stored in the element variable truth table)
       */
    }

  /* leave define mode  */

  if (ncendef (exoid) == -1)
    {
      exerrval = ncerr;
      sprintf(errmsg,
              "Error: failed to complete definition in file id %d",
              exoid);
      ex_err("ex_put_var_param",errmsg,exerrval);
      return (EX_FATAL);
    }

  return(EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if (ncendef (exoid) == -1)     /* exit define mode */
    {
      sprintf(errmsg,
              "Error: failed to complete definition for file id %d",
              exoid);
      ex_err("ex_put_var_param",errmsg,exerrval);
    }
  return (EX_FATAL);
}

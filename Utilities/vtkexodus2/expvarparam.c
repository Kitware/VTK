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
* expvp - ex_put_var_param
*
* entry conditions - 
*   input parameters:
*       int     exoid                   exodus file id
*       int     obj_type                
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

#define EX_PREPARE_RESULT_VAR(TNAME,DIMNAME,VARNAMEVAR) \
  if ((status = nc_def_dim(exoid, DIMNAME, num_vars, &dimid)) != NC_NOERR) { \
          if (status == NC_ENAMEINUSE) { \
              exerrval = status; \
              sprintf(errmsg, \
                      "Error: " TNAME " variable name parameters are already defined in file id %d", \
                      exoid); \
              ex_err("ex_put_var_param",errmsg,exerrval); \
            } else { \
              exerrval = status; \
              sprintf(errmsg, \
                      "Error: failed to define number of " TNAME " variables in file id %d", \
                      exoid); \
              ex_err("ex_put_var_param",errmsg,exerrval); \
            } \
          goto error_ret;          /* exit define mode and return */ \
        } \
      /* Now define TNAME variable name variable */ \
      dims[0] = dimid; \
      dims[1] = strdim; \
  if ((status = nc_def_var (exoid, VARNAMEVAR, NC_CHAR, 2, dims, &varid)) != NC_NOERR) { \
          if (status == NC_ENAMEINUSE) { \
              exerrval = status; \
              sprintf(errmsg, \
                      "Error: " TNAME " variable names are already defined in file id %d", \
                      exoid); \
              ex_err("ex_put_variable_param",errmsg,exerrval); \
            } else { \
              exerrval = status; \
              sprintf(errmsg, \
                      "Error: failed to define " TNAME " variable names in file id %d", \
                      exoid); \
              ex_err("ex_put_variable_param",errmsg,exerrval); \
            } \
          goto error_ret;          /* exit define mode and return */ \
        }

/*!
 * writes the number and names of global, nodal, or element variables 
 * that will be written to the database
 * \param      exoid                   exodus file id
 * \param      obj_type                object type
 * \param     *num_vars                number of variables in database
 */

int ex_put_variable_param (int exoid,
                           ex_entity_type obj_type,
                           int num_vars)
{
  int time_dim, num_nod_dim, dimid, strdim, varid;
  int dims[3];
  char errmsg[MAX_ERR_LENGTH];
  int status;
  
  exerrval = 0; /* clear error code */

  /* if no variables are to be stored, return with warning */
  if (num_vars == 0) {
    exerrval = EX_MSG;
    sprintf(errmsg,
            "Warning: zero %s variables specified for file id %d",
            ex_name_of_object(obj_type),exoid);
    ex_err("ex_put_variable_param",errmsg,exerrval);

    return (EX_WARN);
  }
  
  if ( obj_type != EX_NODAL      &&
       obj_type != EX_NODE_SET   &&
       obj_type != EX_EDGE_BLOCK &&
       obj_type != EX_EDGE_SET   &&
       obj_type != EX_FACE_BLOCK &&
       obj_type != EX_FACE_SET   &&
       obj_type != EX_ELEM_BLOCK &&
       obj_type != EX_ELEM_SET   &&
       obj_type != EX_SIDE_SET   &&
       obj_type != EX_GLOBAL) {
    exerrval = EX_BADPARAM;
    sprintf(errmsg,
            "Error: Invalid variable type %d specified in file id %d",
            obj_type, exoid);
    ex_err("ex_put_variable_param",errmsg,exerrval);
    return (EX_WARN);
  }

  /* inquire previously defined dimensions  */
  if ((status = nc_inq_dimid (exoid, DIM_TIME, &time_dim)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to locate time dimension in file id %d", exoid);
    ex_err("ex_put_variable_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  if ((status = nc_inq_dimid (exoid, DIM_NUM_NODES, &num_nod_dim)) != NC_NOERR) {
    if (obj_type == EX_NODAL) {
      exerrval = status;
      sprintf(errmsg,
              "Error: failed to locate number of nodes in file id %d", exoid);
      ex_err("ex_put_variable_param",errmsg,exerrval);
      return (EX_FATAL);
    }
  }

  if ((status = nc_inq_dimid (exoid, DIM_STR, &strdim)) < 0) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to get string length in file id %d",exoid);
    ex_err("ex_put_variable_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  /* put file into define mode  */
  if ((status = nc_redef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to put file id %d into define mode", exoid);
    ex_err("ex_get_var_param",errmsg,exerrval);
    return (EX_FATAL);
  }


  /* define dimensions and variables */
  if (obj_type == EX_GLOBAL) {
    EX_PREPARE_RESULT_VAR("global",DIM_NUM_GLO_VAR,VAR_NAME_GLO_VAR);

    dims[0] = time_dim;
    dims[1] = dimid;
    if ((status = nc_def_var (exoid, VAR_GLO_VAR, 
                              nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR)
      {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define global variables in file id %d",
                exoid);
        ex_err("ex_put_variable_param",errmsg,exerrval);
        goto error_ret;          /* exit define mode and return */
      }
  }

  else if (obj_type == EX_NODAL) {
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
    if ((status = nc_def_dim(exoid, DIM_NUM_NOD_VAR, num_vars, &dimid)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
        exerrval = status;
        sprintf(errmsg,
                "Error: nodal variable name parameters are already defined in file id %d",
                exoid);
        ex_err("ex_put_variable_param",errmsg,exerrval);
      } else {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define number of nodal variables in file id %d",
                exoid);
        ex_err("ex_put_variable_param",errmsg,exerrval);
      }
      goto error_ret;          /* exit define mode and return */
    }

    if (ex_large_model(exoid) == 0) { /* Old way */
      dims[0] = time_dim;
      dims[1] = dimid;
      dims[2] = num_nod_dim;
      if ((status = nc_def_var(exoid, VAR_NOD_VAR,
                               nc_flt_code(exoid), 3, dims, &varid)) != NC_NOERR) {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define nodal variables in file id %d",
                exoid);
        ex_err("ex_put_variable_param",errmsg,exerrval);
        goto error_ret;          /* exit define mode and return */
      }
    } else { /* New way */
      int i;
      for (i = 1; i <= num_vars; i++) {
        dims[0] = time_dim;
        dims[1] = num_nod_dim;
        if ((status = nc_def_var (exoid, VAR_NOD_VAR_NEW(i),
                                  nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR) {
          exerrval = status;
          sprintf(errmsg,
                  "Error: failed to define nodal variable %d in file id %d",
                  i, exoid);
          ex_err("ex_put_variable_param",errmsg,exerrval);
          goto error_ret;          /* exit define mode and return */
        }
      }
    }

    /* Now define nodal variable name variable */
    dims[0] = dimid;
    dims[1] = strdim;
    if ((status = nc_def_var(exoid, VAR_NAME_NOD_VAR, NC_CHAR, 2, dims, &varid)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
        exerrval = status;
        sprintf(errmsg,
                "Error: nodal variable names are already defined in file id %d",
                exoid);
        ex_err("ex_put_variable_param",errmsg,exerrval);
      } else {
        exerrval = status;
        sprintf(errmsg,
                "Error: failed to define nodal variable names in file id %d",
                exoid);
        ex_err("ex_put_variable_param",errmsg,exerrval);
      }
      goto error_ret;          /* exit define mode and return */
    }
  }

  /* netCDF variables in which to store the EXODUS obj_type variable values will
   * be defined in ex_put_*_var_tab or ex_put_*_var; at this point, we 
   * don't know what obj_type variables are valid for which obj_type blocks 
   * (the info that is stored in the obj_type variable truth table)
   */
  else if (obj_type == EX_ELEM_BLOCK)  {
    EX_PREPARE_RESULT_VAR("element",DIM_NUM_ELE_VAR,VAR_NAME_ELE_VAR);
  }
  else if (obj_type == EX_NODE_SET) {
    EX_PREPARE_RESULT_VAR("nodeset",DIM_NUM_NSET_VAR,VAR_NAME_NSET_VAR);
  }
  else if (obj_type == EX_SIDE_SET) {
    EX_PREPARE_RESULT_VAR("sideset",DIM_NUM_SSET_VAR,VAR_NAME_SSET_VAR);
  }
  else if (obj_type == EX_EDGE_BLOCK) {
    EX_PREPARE_RESULT_VAR("edge",DIM_NUM_EDG_VAR,VAR_NAME_EDG_VAR);
  }
  else if (obj_type == EX_FACE_BLOCK) {
    EX_PREPARE_RESULT_VAR("face",DIM_NUM_FAC_VAR,VAR_NAME_FAC_VAR);
  }
  else if (obj_type == EX_EDGE_SET) {
    EX_PREPARE_RESULT_VAR("edgeset",DIM_NUM_ESET_VAR,VAR_NAME_ESET_VAR);
  }
  else if (obj_type == EX_FACE_SET) {
    EX_PREPARE_RESULT_VAR("faceset",DIM_NUM_FSET_VAR,VAR_NAME_FSET_VAR);
  }
  else if (obj_type == EX_ELEM_SET) {
    EX_PREPARE_RESULT_VAR("elementset",DIM_NUM_ELSET_VAR,VAR_NAME_ELSET_VAR);
  }

  /* leave define mode  */
  if ((status = nc_enddef (exoid)) != NC_NOERR) {
    exerrval = status;
    sprintf(errmsg,
            "Error: failed to complete definition in file id %d",
            exoid);
    ex_err("ex_put_variable_param",errmsg,exerrval);
    return (EX_FATAL);
  }

  return(EX_NOERR);

  /* Fatal error: exit definition mode and return */
 error_ret:
  if ((status = nc_enddef(exoid)) != NC_NOERR) {    /* exit define mode */
    sprintf(errmsg,
            "Error: failed to complete definition for file id %d",
            exoid);
    ex_err("ex_put_variable_param",errmsg,exerrval);
  }
  return (EX_FATAL);
}

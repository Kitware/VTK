/*
 * Copyright (c) 2005-2017 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
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
 *     * Neither the name of NTESS nor the names of its
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

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for ex__compress_variable, etc

/*! \cond INTERNAL */
static int ex_prepare_result_var(int exoid, int num_vars, char *type_name, char *dim_name,
                                 char *variable_name)
{
  int status;
  int dimid;
  int varid;
  int dims[2];
  int dim_str_name;
#if NC_HAS_HDF5
  int fill = NC_FILL_CHAR;
#endif

  char errmsg[MAX_ERR_LENGTH];

  if ((status = nc_def_dim(exoid, dim_name, num_vars, &dimid)) != NC_NOERR) {
    if (status == NC_ENAMEINUSE) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: %s variable name parameters are already defined "
               "in file id %d",
               type_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to define number of %s variables in file id %d", type_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    return (EX_FATAL); /* exit define mode and return */
  }

  /* Now define type_name variable name variable */
  if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &dim_str_name)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get string length in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    return (EX_FATAL);
  }

  dims[0] = dimid;
  dims[1] = dim_str_name;
  if ((status = nc_def_var(exoid, variable_name, NC_CHAR, 2, dims, &varid)) != NC_NOERR) {
    if (status == NC_ENAMEINUSE) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s variable names are already defined in file id %d",
               type_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s variable names in file id %d",
               type_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    return (EX_FATAL); /* exit define mode and return */
  }
#if NC_HAS_HDF5
  nc_def_var_fill(exoid, varid, 0, &fill);
#endif
  return (EX_NOERR);
}
/*! \endcond */

/*!
\ingroup ResultsData

The function ex_put_variable_param() writes the number of global,
nodal, nodeset, sideset, edge, face, or element variables that will be
written to the database.

\return In case of an error, ex_put_variable_param() returns a negative
        number; a warning will return a positive number. Possible causes of
        errors include:
  -  data file not properly opened with call to ex_create() or ex_open()
  -  data file opened for read only.
  -  invalid variable type specified.
  -  data file not initialized properly with call to ex_put_init().
  -  this routine has already been called with the same variable
     type; redefining the number of variables is not allowed.
  -  a warning value is returned if the number of variables
     is specified as zero.

\param[in] exoid     exodus file ID returned from a previous call to ex_create()
or ex_open().
\param[in] obj_type  Variable indicating the type of variable which is
described. Use one
                     of the #ex_entity_type types specified in the table below.
\param[in] num_vars  The number of var_type variables that will be written to the database.

| ex_entity_type|  description              |
|---------------|---------------------------|
| #EX_GLOBAL     |  Global entity type       |
| #EX_NODAL      |  Nodal entity type        |
| #EX_NODE_SET   |  Node Set entity type     |
| #EX_EDGE_BLOCK |  Edge Block entity type   |
| #EX_EDGE_SET   |  Edge Set entity type     |
| #EX_FACE_BLOCK |  Face Block entity type   |
| #EX_FACE_SET   |  Face Set entity type     |
| #EX_ELEM_BLOCK |  Element Block entity type|
| #EX_ELEM_SET   |  Element Set entity type  |
| #EX_SIDE_SET   |  Side Set entity type     |

For example, the following code segment initializes the data file to
store global variables:

~~~{.c}
int num_glo_vars, error, exoid;

\comment{write results variables parameters}
num_glo_vars = 3;

error = ex_put_variable_param (exoid, EX_GLOBAL, num_glo_vars);
~~~

 */

int ex_put_variable_param(int exoid, ex_entity_type obj_type, int num_vars)
{
  int  time_dim, num_nod_dim, dimid, dim_str_name, varid;
  int  dims[3];
  char errmsg[MAX_ERR_LENGTH];
  int  status;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* if no variables are to be stored, return with warning */
  if (num_vars == 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: zero %s variables specified for file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);

    EX_FUNC_LEAVE(EX_WARN);
  }

  if (obj_type != EX_NODAL && obj_type != EX_NODE_SET && obj_type != EX_EDGE_BLOCK &&
      obj_type != EX_EDGE_SET && obj_type != EX_FACE_BLOCK && obj_type != EX_FACE_SET &&
      obj_type != EX_ELEM_BLOCK && obj_type != EX_ELEM_SET && obj_type != EX_SIDE_SET &&
      obj_type != EX_GLOBAL) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid variable type %d specified in file id %d",
             obj_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* inquire previously defined dimensions  */
  if ((status = nc_inq_dimid(exoid, DIM_TIME, &time_dim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time dimension in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_dimid(exoid, DIM_NUM_NODES, &num_nod_dim)) != NC_NOERR) {
    if (obj_type == EX_NODAL) {
      EX_FUNC_LEAVE(EX_NOERR); /* Probably no nodes on database (e.g., badly
                            load-balanced parallel run) */
    }
  }

  if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &dim_str_name)) < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get name string length in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* put file into define mode  */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* define dimensions and variables */
  if (obj_type == EX_GLOBAL) {
    if ((status = ex_prepare_result_var(exoid, num_vars, "global", DIM_NUM_GLO_VAR,
                                        VAR_NAME_GLO_VAR)) != EX_NOERR) {
      goto error_ret;
    }

    if ((status = nc_inq_dimid(exoid, DIM_NUM_GLO_VAR, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get global variable count in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    dims[0] = time_dim;
    dims[1] = dimid;
    if ((status = nc_def_var(exoid, VAR_GLO_VAR, nc_flt_code(exoid), 2, dims, &varid)) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define global variables in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
    ex__compress_variable(exoid, varid, 2);
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
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: nodal variable name parameters are already "
                 "defined in file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of nodal variables in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      goto error_ret; /* exit define mode and return */
    }

    int i;
    for (i = 1; i <= num_vars; i++) {
      dims[0] = time_dim;
      dims[1] = num_nod_dim;
      if ((status = nc_def_var(exoid, VAR_NOD_VAR_NEW(i), nc_flt_code(exoid), 2, dims, &varid)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define nodal variable %d in file id %d",
                 i, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      ex__compress_variable(exoid, varid, 2);
    }

    /* Now define nodal variable name variable */
    dims[0] = dimid;
    dims[1] = dim_str_name;
    if ((status = nc_def_var(exoid, VAR_NAME_NOD_VAR, NC_CHAR, 2, dims, &varid)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: nodal variable names are already defined in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define nodal variable names in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      goto error_ret; /* exit define mode and return */
    }
  }

  /* netCDF variables in which to store the EXODUS obj_type variable values will
   * be defined in ex_put_*_var_tab or ex_put_*_var; at this point, we
   * don't know what obj_type variables are valid for which obj_type blocks
   * (the info that is stored in the obj_type variable truth table)
   */
  else if (obj_type == EX_ELEM_BLOCK) {
    if ((status = ex_prepare_result_var(exoid, num_vars, "element", DIM_NUM_ELE_VAR,
                                        VAR_NAME_ELE_VAR)) != EX_NOERR) {
      goto error_ret;
    }
  }
  else if (obj_type == EX_NODE_SET) {
    if ((status = ex_prepare_result_var(exoid, num_vars, "nodeset", DIM_NUM_NSET_VAR,
                                        VAR_NAME_NSET_VAR)) != EX_NOERR) {
      goto error_ret;
    }
  }
  else if (obj_type == EX_SIDE_SET) {
    if ((status = ex_prepare_result_var(exoid, num_vars, "sideset", DIM_NUM_SSET_VAR,
                                        VAR_NAME_SSET_VAR)) != EX_NOERR) {
      goto error_ret;
    }
  }
  else if (obj_type == EX_EDGE_BLOCK) {
    if ((status = ex_prepare_result_var(exoid, num_vars, "edge", DIM_NUM_EDG_VAR,
                                        VAR_NAME_EDG_VAR)) != EX_NOERR) {
      goto error_ret;
    }
  }
  else if (obj_type == EX_FACE_BLOCK) {
    if ((status = ex_prepare_result_var(exoid, num_vars, "face", DIM_NUM_FAC_VAR,
                                        VAR_NAME_FAC_VAR)) != EX_NOERR) {
      goto error_ret;
    }
  }
  else if (obj_type == EX_EDGE_SET) {
    if ((status = ex_prepare_result_var(exoid, num_vars, "edgeset", DIM_NUM_ESET_VAR,
                                        VAR_NAME_ESET_VAR)) != EX_NOERR) {
      goto error_ret;
    }
  }
  else if (obj_type == EX_FACE_SET) {
    if ((status = ex_prepare_result_var(exoid, num_vars, "faceset", DIM_NUM_FSET_VAR,
                                        VAR_NAME_FSET_VAR)) != EX_NOERR) {
      goto error_ret;
    }
  }
  else if (obj_type == EX_ELEM_SET) {
    if ((status = ex_prepare_result_var(exoid, num_vars, "elementset", DIM_NUM_ELSET_VAR,
                                        VAR_NAME_ELSET_VAR)) != EX_NOERR) {
      goto error_ret;
    }
  }

  /* leave define mode  */
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

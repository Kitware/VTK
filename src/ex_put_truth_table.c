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
/*****************************************************************************
 *
 * expvtt - ex_put_truth_table
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid              exodus file id
 *       int     obj_type           object type
 *       int     num_blk            number of blocks
 *       int     num_var            number of variables
 *       int*    variable_table            variable truth table array
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for ex__get_dimension, EX_FATAL, etc

/*!
\ingroup ResultsData

 * writes the EXODUS variable truth table to the database; also,
 * creates netCDF variables in which to store EXODUS variable
 * values; although this table isn't required (because the netCDF
 * variables can also be created in ex_put_var()), this call will save
 * tremendous time because all of the variables are defined at once
 * while the file is in define mode, rather than going in and out of
 * define mode (causing the entire file to be copied over and over)
 * which is what occurs when the variables are defined in ex_put_var()
 * \param       exoid              exodus file id
 * \param       obj_type           object type
 * \param       num_blk            number of blocks or sets
 * \param       num_var            number of variables
 * \param      *var_tab            variable truth table array
 *
The following coding will create, populate, and write an element
variable truth table to an opened exodus file (NOTE: all element
variables are valid for all element blocks in this example.):

~~~{.c}
int *truth_tab, num_elem_blk, num_ele_vars, error, exoid;

\comment{write element variable truth table}
truth_tab = (int *)calloc((num_elem_blk*num_ele_vars), sizeof(int));

for (i=0, k=0; i < num_elem_blk; i++) {
   for (j=0; j < num_ele_vars; j++) {
      truth_tab[k++] = 1;
   }
}
error = ex_put_truth_table(exoid, EX_ELEM_BLOCK, num_elem_blk, num_ele_vars,
                            truth_tab);
~~~
 */

int ex_put_truth_table(int exoid, ex_entity_type obj_type, int num_blk, int num_var, int *var_tab)
{
  int    numelblkdim, numelvardim, timedim, dims[2], varid;
  char * sta_type, *tab_type;
  size_t num_entity = 0;
  size_t num_var_db = 0;
  int *  stat_vals;
  int    i, j, k;
  int    status;
  char   errmsg[MAX_ERR_LENGTH];

  /*
   * The ent_type and the var_name are used to build the netcdf
   * variables name.  Normally this is done via a macro defined in
   * exodusII_int.h
   */
  const char *ent_type = NULL;
  const char *var_name = NULL;
  const char *ent_size = NULL;

  EX_FUNC_ENTER();

  ex__check_valid_file_id(exoid, __func__);

  ex__get_dimension(exoid, ex__dim_num_objects(obj_type), ex_name_of_object(obj_type), &num_entity,
                    &numelblkdim, __func__);

  if (obj_type == EX_ELEM_BLOCK) {
    ex__get_dimension(exoid, DIM_NUM_ELE_VAR, "element variables", &num_var_db, &numelvardim,
                      __func__);
    var_name = "vals_elem_var";
    ent_type = "eb";
    ent_size = "num_el_in_blk";
    sta_type = VAR_STAT_EL_BLK;
    tab_type = VAR_ELEM_TAB;
  }
  else if (obj_type == EX_EDGE_BLOCK) {
    ex__get_dimension(exoid, DIM_NUM_EDG_VAR, "edge block variables", &num_var_db, &numelvardim,
                      __func__);
    var_name = "vals_edge_var";
    ent_type = "eb";
    ent_size = "num_ed_in_blk";
    sta_type = VAR_STAT_ED_BLK;
    tab_type = VAR_EBLK_TAB;
  }
  else if (obj_type == EX_FACE_BLOCK) {
    ex__get_dimension(exoid, DIM_NUM_FAC_VAR, "face block variables", &num_var_db, &numelvardim,
                      __func__);
    var_name = "vals_face_var";
    ent_type = "fb";
    ent_size = "num_fa_in_blk";
    sta_type = VAR_STAT_FA_BLK;
    tab_type = VAR_FBLK_TAB;
  }
  else if (obj_type == EX_SIDE_SET) {
    ex__get_dimension(exoid, DIM_NUM_SSET_VAR, "sideset variables", &num_var_db, &numelvardim,
                      __func__);
    var_name = "vals_sset_var";
    ent_type = "ss";
    ent_size = "num_side_ss";
    sta_type = VAR_SS_STAT;
    tab_type = VAR_SSET_TAB;
  }
  else if (obj_type == EX_NODE_SET) {
    ex__get_dimension(exoid, DIM_NUM_NSET_VAR, "nodeset variables", &num_var_db, &numelvardim,
                      __func__);
    var_name = "vals_nset_var";
    ent_type = "ns";
    ent_size = "num_nod_ns";
    sta_type = VAR_NS_STAT;
    tab_type = VAR_NSET_TAB;
  }
  else if (obj_type == EX_EDGE_SET) {
    ex__get_dimension(exoid, DIM_NUM_ESET_VAR, "edge set variables", &num_var_db, &numelvardim,
                      __func__);
    var_name = "vals_eset_var";
    ent_type = "es";
    ent_size = "num_edge_es";
    sta_type = VAR_ES_STAT;
    tab_type = VAR_ESET_TAB;
  }
  else if (obj_type == EX_FACE_SET) {
    ex__get_dimension(exoid, DIM_NUM_FSET_VAR, "face set variables", &num_var_db, &numelvardim,
                      __func__);
    var_name = "vals_fset_var";
    ent_type = "fs";
    ent_size = "num_face_fs";
    sta_type = VAR_FS_STAT;
    tab_type = VAR_FSET_TAB;
  }
  else if (obj_type == EX_ELEM_SET) {
    ex__get_dimension(exoid, DIM_NUM_ELSET_VAR, "element set variables", &num_var_db, &numelvardim,
                      __func__);
    var_name = "vals_elset_var";
    ent_type = "es";
    ent_size = "num_ele_els";
    sta_type = VAR_ELS_STAT;
    tab_type = VAR_ELSET_TAB;
  }

  else { /* invalid variable type */
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Invalid variable type %d specified in file id %d",
             obj_type, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_WARN);
  }

  if ((int)num_entity != num_blk) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: # of %s doesn't match those defined in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((int)num_var_db != num_var) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: # of %s variables doesn't match those defined in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get status array for later use */
  if (!(stat_vals = malloc(num_blk * sizeof(int)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate memory for %s status array for file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  status = nc_inq_varid(exoid, sta_type, &varid);

  /* get variable id of status array */
  if (status == NC_NOERR) {
    /* if status array exists (V 2.01+), use it, otherwise assume
       object exists to be backward compatible */

    if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) {
      free(stat_vals);
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s status array from file id %d",
               ex_name_of_object(obj_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    /* status array doesn't exist (V2.00), dummy one up for later checking */
    for (i = 0; i < num_blk; i++) {
      stat_vals[i] = 1;
    }
  }

  /* put netcdf file into define mode  */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    free(stat_vals);
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire previously defined dimensions */
  if ((status = nc_inq_dimid(exoid, DIM_TIME, &timedim)) != NC_NOERR) {
    free(stat_vals);
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time variable in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret; /* exit define mode and return */
  }

  /* define netCDF variables in which to store EXODUS element
   * variable values
   */

  k = 0;
  for (i = 0; i < num_blk; i++) {
    for (j = 1; j <= num_var; j++) {

      /* check if variables are to be put out for this entity */
      if (var_tab[k] != 0) {
        if (stat_vals[i] != 0) { /* check for NULL entity */
          /* NOTE: This code used to zero out the var_tab entry
             if the stat_vals[i] value was zero. However, in some
             cases it is good to know that a variable was assigned to
             an entity even if that entity is empty. The code was
             changed to not modify the truth table.
          */
          dims[0] = timedim;

          /* Determine number of entities in block */
          if ((status = nc_inq_dimid(exoid, ex__catstr(ent_size, (i + 1)), &dims[1])) != NC_NOERR) {
            free(stat_vals);
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to locate number of entities in "
                     "%d'th %s in file id %d",
                     i + 1, ex_name_of_object(obj_type), exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
            goto error_ret; /* exit define mode and return */
          }

          /* define netCDF variable to store variable values; the j
           * index cycles from 1 through the number of variables so
           * that the index of the EXODUS variable (which is part
           * of the name of the netCDF variable) will begin at 1
           * instead of 0
           */

          if ((status = nc_def_var(exoid, ex__catstr2(var_name, j, ent_type, i + 1),
                                   nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR) {
            if (status != NC_ENAMEINUSE) {
              free(stat_vals);
              snprintf(errmsg, MAX_ERR_LENGTH,
                       "ERROR: failed to define variable for %d'th %s in file id %d", i + 1,
                       ex_name_of_object(obj_type), exoid);
              ex_err_fn(exoid, __func__, errmsg, status);
              goto error_ret; /* exit define mode and return */
            }
            ex__compress_variable(exoid, varid, 2);
          }
        }
      }    /* if */
      k++; /* increment element truth table pointer */
    }      /* for j */
  }        /* for i */

  free(stat_vals);

  /* create a variable array in which to store the truth table
   */

  dims[0] = numelblkdim;
  dims[1] = numelvardim;
  status  = nc_def_var(exoid, tab_type, NC_INT, 2, dims, &varid);

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to define %s variable truth table in file id %d",
             ex_name_of_object(obj_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret; /* exit define mode and return */
  }

  /* leave define mode  */
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out the element variable truth table */
  status = nc_put_var_int(exoid, varid, var_tab);

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store variable truth table in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

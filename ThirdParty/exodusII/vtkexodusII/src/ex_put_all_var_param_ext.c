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
 * expvp - ex_put_all_var_param_ext
 *
 * entry conditions -
 *   input parameters:
 *       int                  exoid    exodus file id
 *       const ex_var_params* vp       pointer to variable parameter info
 *
 * exit conditions -
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for ex__get_dimension, etc

static int  define_dimension(int exoid, const char *DIMENSION, int count, const char *label,
                             int *dimid);
static int  define_variable_name_variable(int exoid, const char *VARIABLE, int dimension,
                                          const char *label);
static int *get_status_array(int exoid, int var_count, const char *VARIABLE, const char *label);
static int  put_truth_table(int exoid, int varid, int *table, const char *label);
static int  define_truth_table(ex_entity_type obj_type, int exoid, int num_ent, int num_var,
                               int *var_tab, int *status_tab, void_int *ids, const char *label);
static int  ex_define_vars(int exoid, ex_entity_type obj_type, const char *entity_name,
                           const char *entity_blk_name, int numvar, const char *DNAME, int dimid1,
                           int dimid2, int DVAL, void_int **entity_ids, const char *VNOV,
                           const char *VTV, int **status_var, int *truth_table,
                           int *truth_table_var);

#define EX_GET_IDS_STATUS(TNAME, NUMVAR, DNAME, DID, DVAL, VIDS, EIDS, VSTAT, VSTATVAL)            \
  if (NUMVAR > 0) {                                                                                \
    status = ex__get_dimension(exoid, DNAME, TNAME "s", &DVAL, &DID, __func__);                    \
    if (status != NC_NOERR)                                                                        \
      goto error_ret;                                                                              \
                                                                                                   \
    /* get element block IDs */                                                                    \
    if (!(VIDS = malloc(sizeof(int64_t) * DVAL))) {                                                \
      snprintf(errmsg, MAX_ERR_LENGTH,                                                             \
               "ERROR: failed to allocate memory for " TNAME " id array for file id %d", exoid);   \
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);                                              \
      goto error_ret;                                                                              \
    }                                                                                              \
    ex_get_ids(exoid, EIDS, VIDS);                                                                 \
                                                                                                   \
    /* Get element block status array for later use (allocates memory) */                          \
    VSTATVAL = get_status_array(exoid, DVAL, VSTAT, TNAME);                                        \
    if (VSTATVAL == NULL) {                                                                        \
      goto error_ret;                                                                              \
    }                                                                                              \
  }

/*!
 * writes the number of global, nodal, element, nodeset, and sideset variables
 * that will be written to the database
 * \param      exoid    exodus file id
 * \param      *vp       pointer to variable parameter info
 */

int ex_put_all_var_param_ext(int exoid, const ex_var_params *vp)
{
  int    in_define = 0;
  int    status    = 0;
  int    temp      = 0;
  int    time_dim = 0, num_nod_dim = 0, dimid = 0;
  size_t num_elem_blk = 0, num_edge_blk = 0, num_face_blk = 0;
  size_t num_nset = 0, num_eset = 0, num_fset = 0, num_sset = 0, num_elset = 0;
  int    numelblkdim = 0, numelvardim = 0, numedvardim = 0, numedblkdim = 0, numfavardim = 0,
      numfablkdim = 0, numnsetdim = 0, nsetvardim = 0, numesetdim = 0, esetvardim = 0,
      numfsetdim = 0, fsetvardim = 0, numssetdim = 0, ssetvardim = 0, numelsetdim = 0,
      elsetvardim = 0;
  int i           = 0;

  int edblk_varid = 0, fablk_varid = 0, eblk_varid = 0, nset_varid = 0, eset_varid = 0,
      fset_varid = 0, sset_varid = 0, elset_varid = 0, varid = 0;

  void_int *eblk_ids  = NULL;
  void_int *edblk_ids = NULL;
  void_int *fablk_ids = NULL;
  void_int *nset_ids  = NULL;
  void_int *eset_ids  = NULL;
  void_int *fset_ids  = NULL;
  void_int *sset_ids  = NULL;
  void_int *elset_ids = NULL;

  int *eblk_stat  = NULL;
  int *edblk_stat = NULL;
  int *fablk_stat = NULL;
  int *nset_stat  = NULL;
  int *eset_stat  = NULL;
  int *fset_stat  = NULL;
  int *sset_stat  = NULL;
  int *elset_stat = NULL;

  int  dims[3];
  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* inquire previously defined dimensions  */

  if ((status = nc_inq_dimid(exoid, DIM_TIME, &time_dim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time dimension in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  if ((status = nc_inq_dimid(exoid, DIM_NUM_NODES, &num_nod_dim)) != NC_NOERR) {
    num_nod_dim = -1; /* There is probably no nodes on this file */
  }

  /* Check this now so we can use it later without checking for errors */
  if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &temp)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get string length in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  EX_GET_IDS_STATUS("edge block", vp->num_edge, DIM_NUM_ED_BLK, numedblkdim, num_edge_blk,
                    edblk_ids, EX_EDGE_BLOCK, VAR_STAT_ED_BLK, edblk_stat);
  EX_GET_IDS_STATUS("face block", vp->num_face, DIM_NUM_FA_BLK, numfablkdim, num_face_blk,
                    fablk_ids, EX_FACE_BLOCK, VAR_STAT_FA_BLK, fablk_stat);
  EX_GET_IDS_STATUS("element block", vp->num_elem, DIM_NUM_EL_BLK, numelblkdim, num_elem_blk,
                    eblk_ids, EX_ELEM_BLOCK, VAR_STAT_EL_BLK, eblk_stat);
  EX_GET_IDS_STATUS("node set", vp->num_nset, DIM_NUM_NS, numnsetdim, num_nset, nset_ids,
                    EX_NODE_SET, VAR_NS_STAT, nset_stat);
  EX_GET_IDS_STATUS("edge set", vp->num_eset, DIM_NUM_ES, numesetdim, num_eset, eset_ids,
                    EX_EDGE_SET, VAR_ES_STAT, eset_stat);
  EX_GET_IDS_STATUS("face set", vp->num_fset, DIM_NUM_FS, numfsetdim, num_fset, fset_ids,
                    EX_FACE_SET, VAR_FS_STAT, fset_stat);
  EX_GET_IDS_STATUS("side set", vp->num_sset, DIM_NUM_SS, numssetdim, num_sset, sset_ids,
                    EX_SIDE_SET, VAR_SS_STAT, sset_stat);
  EX_GET_IDS_STATUS("element set", vp->num_elset, DIM_NUM_ELS, numelsetdim, num_elset, elset_ids,
                    EX_ELEM_SET, VAR_ELS_STAT, elset_stat);

  /* put file into define mode  */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }
  in_define = 1;

  /* define dimensions and variables */
  if (vp->num_glob > 0) {
    if (define_dimension(exoid, DIM_NUM_GLO_VAR, vp->num_glob, "global", &dimid) != NC_NOERR) {
      goto error_ret;
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

    /* Now define global variable name variable */
    if (define_variable_name_variable(exoid, VAR_NAME_GLO_VAR, dimid, "global") != NC_NOERR) {
      goto error_ret;
    }
  }

  if (vp->num_node > 0) {
    if (define_dimension(exoid, DIM_NUM_NOD_VAR, vp->num_node, "nodal", &dimid) != NC_NOERR) {
      goto error_ret;
    }

    if (num_nod_dim > 0) {
      for (i = 1; i <= vp->num_node; i++) {
        dims[0] = time_dim;
        dims[1] = num_nod_dim;
        if ((status = nc_def_var(exoid, VAR_NOD_VAR_NEW(i), nc_flt_code(exoid), 2, dims, &varid)) !=
            NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to define nodal variable %d in file id %d", i, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          goto error_ret; /* exit define mode and return */
        }
        ex__compress_variable(exoid, varid, 2);
      }
    }

    /* Now define nodal variable name variable */
    if (define_variable_name_variable(exoid, VAR_NAME_NOD_VAR, dimid, "nodal") != NC_NOERR) {
      goto error_ret;
    }
  }

  if ((status =
           ex_define_vars(exoid, EX_EDGE_BLOCK, "edge", "edge block", vp->num_edge, DIM_NUM_EDG_VAR,
                          numedblkdim, numedvardim, num_edge_blk, &edblk_ids, VAR_NAME_EDG_VAR,
                          VAR_EBLK_TAB, &edblk_stat, vp->edge_var_tab, &edblk_varid)) != EX_NOERR) {
    goto error_ret;
  }

  if ((status =
           ex_define_vars(exoid, EX_FACE_BLOCK, "face", "face block", vp->num_face, DIM_NUM_FAC_VAR,
                          numfablkdim, numfavardim, num_face_blk, &fablk_ids, VAR_NAME_FAC_VAR,
                          VAR_FBLK_TAB, &fablk_stat, vp->face_var_tab, &fablk_varid)) != EX_NOERR) {
    goto error_ret;
  }

  if ((status = ex_define_vars(exoid, EX_ELEM_BLOCK, "element", "element block", vp->num_elem,
                               DIM_NUM_ELE_VAR, numelblkdim, numelvardim, num_elem_blk, &eblk_ids,
                               VAR_NAME_ELE_VAR, VAR_ELEM_TAB, &eblk_stat, vp->elem_var_tab,
                               &eblk_varid)) != EX_NOERR) {
    goto error_ret;
  }

  if ((status =
           ex_define_vars(exoid, EX_NODE_SET, "nodeset", "node set", vp->num_nset, DIM_NUM_NSET_VAR,
                          numnsetdim, nsetvardim, num_nset, &nset_ids, VAR_NAME_NSET_VAR,
                          VAR_NSET_TAB, &nset_stat, vp->nset_var_tab, &nset_varid)) != EX_NOERR) {
    goto error_ret;
  }

  if ((status =
           ex_define_vars(exoid, EX_EDGE_SET, "edgeset", "edge set", vp->num_eset, DIM_NUM_ESET_VAR,
                          numesetdim, esetvardim, num_eset, &eset_ids, VAR_NAME_ESET_VAR,
                          VAR_ESET_TAB, &eset_stat, vp->eset_var_tab, &eset_varid)) != EX_NOERR) {
    goto error_ret;
  }

  if ((status =
           ex_define_vars(exoid, EX_FACE_SET, "faceset", "face set", vp->num_fset, DIM_NUM_FSET_VAR,
                          numfsetdim, fsetvardim, num_fset, &fset_ids, VAR_NAME_FSET_VAR,
                          VAR_FSET_TAB, &fset_stat, vp->fset_var_tab, &fset_varid)) != EX_NOERR) {
    goto error_ret;
  }

  if ((status =
           ex_define_vars(exoid, EX_SIDE_SET, "sideset", "side set", vp->num_sset, DIM_NUM_SSET_VAR,
                          numssetdim, ssetvardim, num_sset, &sset_ids, VAR_NAME_SSET_VAR,
                          VAR_SSET_TAB, &sset_stat, vp->sset_var_tab, &sset_varid)) != EX_NOERR) {
    goto error_ret;
  }

  if ((status = ex_define_vars(exoid, EX_ELEM_SET, "elemset", "element set", vp->num_elset,
                               DIM_NUM_ELSET_VAR, numelsetdim, elsetvardim, num_elset, &elset_ids,
                               VAR_NAME_ELSET_VAR, VAR_ELSET_TAB, &elset_stat, vp->elset_var_tab,
                               &elset_varid)) != EX_NOERR) {
    goto error_ret;
  }

  /* leave define mode  */

  in_define = 0;
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    goto error_ret;
  }

  /* write out the variable truth tables */
  if (vp->num_edge > 0) {
    if (put_truth_table(exoid, edblk_varid, vp->edge_var_tab, "edge") != NC_NOERR) {
      goto error_ret;
    }
  }

  if (vp->num_face > 0) {
    if (put_truth_table(exoid, fablk_varid, vp->face_var_tab, "face") != NC_NOERR) {
      goto error_ret;
    }
  }

  if (vp->num_elem > 0) {
    if (put_truth_table(exoid, eblk_varid, vp->elem_var_tab, "element") != NC_NOERR) {
      goto error_ret;
    }
  }

  if (vp->num_nset > 0) {
    if (put_truth_table(exoid, nset_varid, vp->nset_var_tab, "nodeset") != NC_NOERR) {
      goto error_ret;
    }
  }

  if (vp->num_eset > 0) {
    if (put_truth_table(exoid, eset_varid, vp->eset_var_tab, "edgeset") != NC_NOERR) {
      goto error_ret;
    }
  }

  if (vp->num_fset > 0) {
    if (put_truth_table(exoid, fset_varid, vp->fset_var_tab, "faceset") != NC_NOERR) {
      goto error_ret;
    }
  }

  if (vp->num_sset > 0) {
    if (put_truth_table(exoid, sset_varid, vp->sset_var_tab, "sideset") != NC_NOERR) {
      goto error_ret;
    }
  }

  if (vp->num_elset > 0) {
    if (put_truth_table(exoid, elset_varid, vp->elset_var_tab, "elemset") != NC_NOERR) {
      goto error_ret;
    }
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  if (in_define == 1) {
    ex__leavedef(exoid, __func__);
  }
  free(eblk_ids);
  free(edblk_ids);
  free(fablk_ids);
  free(nset_ids);
  free(eset_ids);
  free(fset_ids);
  free(sset_ids);
  free(elset_ids);

  free(eblk_stat);
  free(edblk_stat);
  free(fablk_stat);
  free(nset_stat);
  free(eset_stat);
  free(fset_stat);
  free(sset_stat);
  free(elset_stat);

  EX_FUNC_LEAVE(EX_FATAL);
}

static int define_dimension(int exoid, const char *DIMENSION, int count, const char *label,
                            int *dimid)
{
  char errmsg[MAX_ERR_LENGTH];
  int  status;
  if ((status = nc_def_dim(exoid, DIMENSION, count, dimid)) != NC_NOERR) {
    if (status == NC_ENAMEINUSE) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: %s variable name parameters are already defined "
               "in file id %d",
               label, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to define number of %s variables in file id %d", label, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
  }
  return (status);
}

static int define_variable_name_variable(int exoid, const char *VARIABLE, int dimension,
                                         const char *label)
{
  char errmsg[MAX_ERR_LENGTH];
  int  dims[2];
  int  variable;
  int  status;
#if NC_HAS_HDF5
  int fill = NC_FILL_CHAR;
#endif

  dims[0] = dimension;
  (void)nc_inq_dimid(exoid, DIM_STR_NAME, &dims[1]); /* Checked earlier, so known to exist */

  if ((status = nc_def_var(exoid, VARIABLE, NC_CHAR, 2, dims, &variable)) != NC_NOERR) {
    if (status == NC_ENAMEINUSE) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s variable names are already defined in file id %d",
               label, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define %s variable names in file id %d",
               label, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
  }
#if NC_HAS_HDF5
  nc_def_var_fill(exoid, variable, 0, &fill);
#endif
  return (status);
}

static int *get_status_array(int exoid, int var_count, const char *VARIABLE, const char *label)
{
  char errmsg[MAX_ERR_LENGTH];
  int  varid;
  int  status;
  int *stat_vals = NULL;

  if (!(stat_vals = malloc(var_count * sizeof(int)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate memory for %s status array for file id %d", label, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    return (NULL);
  }

  /* get variable id of status array */
  if ((nc_inq_varid(exoid, VARIABLE, &varid)) == NC_NOERR) {
    /* if status array exists (V 2.01+), use it, otherwise assume
       object exists to be backward compatible */

    if ((status = nc_get_var_int(exoid, varid, stat_vals)) != NC_NOERR) {
      free(stat_vals);
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s status array from file id %d",
               label, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (NULL);
    }
  }
  else {
    /* status array doesn't exist (V2.00), dummy one up for later checking */
    int i;
    for (i = 0; i < var_count; i++) {
      stat_vals[i] = 1;
    }
  }
  return stat_vals;
}

static int put_truth_table(int exoid, int varid, int *table, const char *label)
{
  int  iresult = 0;
  char errmsg[MAX_ERR_LENGTH];

  iresult = nc_put_var_int(exoid, varid, table);

  if (iresult != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s variable truth table in file id %d",
             label, exoid);
    ex_err_fn(exoid, __func__, errmsg, iresult);
  }
  return iresult;
}

static int define_truth_table(ex_entity_type obj_type, int exoid, int num_ent, int num_var,
                              int *var_tab, int *status_tab, void_int *ids, const char *label)
{
  char errmsg[MAX_ERR_LENGTH];
  int  k = 0;
  int  i, j;
  int  time_dim;
  int  dims[2];
  int  varid;
  int  status;

  if ((status = nc_inq_dimid(exoid, DIM_TIME, &time_dim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate time dimension in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    return -1;
  }

  if (var_tab == NULL) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s variable truth table is NULL in file id %d", label,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    return -1;
  }

  for (i = 0; i < num_ent; i++) {
    int64_t id;
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      id = ((int64_t *)ids)[i];
    }
    else {
      id = ((int *)ids)[i];
    }

    for (j = 1; j <= num_var; j++) {

      /* check if variables are to be put out for this block */
      if (var_tab[k] != 0) {
        if (status_tab[i] != 0) { /* only define variable if active */
          dims[0] = time_dim;

          /* Determine number of entities in entity */
          /* Need way to make this more generic... */
          status = nc_inq_dimid(exoid, ex__dim_num_entries_in_object(obj_type, i + 1), &dims[1]);
          if (status != NC_NOERR) {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to locate number of entities in %s %" PRId64 " in file id %d",
                     label, id, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
            return (status);
          }

          /* define netCDF variable to store variable values;
           * the j index cycles from 1 through the number of variables so
           * that the index of the EXODUS variable (which is part of
           * the name of the netCDF variable) will begin at 1 instead of 0
           */
          status = nc_def_var(exoid, ex__name_var_of_object(obj_type, j, i + 1), nc_flt_code(exoid),
                              2, dims, &varid);
          if (status != NC_NOERR) {
            if (status != NC_ENAMEINUSE) {
              snprintf(errmsg, MAX_ERR_LENGTH,
                       "ERROR: failed to define %s variable for %s %" PRId64 " in file id %d",
                       label, label, id, exoid);
              ex_err_fn(exoid, __func__, errmsg, status);
              return (status);
            }
          }
          ex__compress_variable(exoid, varid, 2);
        }
      }    /* if */
      k++; /* increment truth table pointer */
    }      /* for j */
  }        /* for i */
  return NC_NOERR;
}

static int ex_define_vars(int exoid, ex_entity_type obj_type, const char *entity_name,
                          const char *entity_blk_name, int numvar, const char *DNAME, int dimid1,
                          int dimid2, int DVAL, void_int **entity_ids, const char *VNOV,
                          const char *VTV, int **status_var, int *truth_table, int *truth_table_var)
{
  int  status = 0;
  int  dims[2];
  char errmsg[MAX_ERR_LENGTH];

  if (numvar > 0) {
    if ((status = define_dimension(exoid, DNAME, numvar, entity_name, &dimid2)) != NC_NOERR) {
      return status;
    }

    /* Now define entity_name variable name variable */
    if ((status = define_variable_name_variable(exoid, VNOV, dimid2, entity_name)) != NC_NOERR) {
      return status;
    }

    if ((status = define_truth_table(obj_type, exoid, DVAL, numvar, truth_table, *status_var,
                                     *entity_ids, entity_blk_name)) != NC_NOERR) {
      return status;
    }

    free(*status_var);
    *status_var = NULL;
    free(*entity_ids);
    *entity_ids = NULL;

    /* create a variable array in which to store the entity_name variable truth
     * table
     */

    dims[0] = dimid1;
    dims[1] = dimid2;

    if ((status = nc_def_var(exoid, VTV, NC_INT, 2, dims, truth_table_var)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to define %s variable truth table in file id %d", entity_name, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return status;
    }
  }
  return NC_NOERR;
}

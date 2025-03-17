/*
 * Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expcss - ex_put_concat_sets
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     set_type                type of set
 *       struct ex_set_specs* set_specs  set specs structure
 *
 * exit conditions -
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, exi_comp_ws, etc

/*!
 * writes the set ID's, set entry count array, set entry pointers array,
 * set entry list, set extra list, and distribution factors list for
 * all the sets of the specified type.
 * \param  exoid      exodus file id
 * \param  set_type   type of set
 * \param  set_specs  set specs structure
 */

int ex_put_concat_sets(int exoid, ex_entity_type set_type, const struct ex_set_specs *set_specs)
{
  int             status;
  int             temp;
  const void_int *num_entries_per_set = set_specs->num_entries_per_set;
  const void_int *num_dist_per_set    = set_specs->num_dist_per_set;
  const void_int *sets_entry_index    = set_specs->sets_entry_index;
  const void_int *sets_dist_index     = set_specs->sets_dist_index;
  const void     *sets_dist_fact      = set_specs->sets_dist_fact;
  size_t          num_df, num_entry;
  int             i, cur_num_sets, num_sets;
  int             dimid, varid, set_id_ndx, dims[1];
  int            *set_stat = NULL;
  int             set_int_type, int_size;

  const float  *flt_dist_fact = NULL;
  const double *dbl_dist_fact = NULL;
  char          errmsg[MAX_ERR_LENGTH];
  char         *idsptr   = NULL;
  char         *statptr  = NULL;
  char         *numdfptr = NULL;
  char         *factptr  = NULL;
  char         *elemptr  = NULL;
  char         *extraptr = NULL;
  ex_inquiry    ex_inq_val;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int_size = sizeof(int);
  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    int_size = sizeof(int64_t);
  }

  /* setup pointers based on set_type
     NOTE: there is another block that sets more stuff later ... */

  if (set_type == EX_NODE_SET) {
    ex_inq_val = EX_INQ_NODE_SETS;
    idsptr     = VAR_NS_IDS;
    statptr    = VAR_NS_STAT;
  }
  else if (set_type == EX_EDGE_SET) {
    ex_inq_val = EX_INQ_EDGE_SETS;
    idsptr     = VAR_ES_IDS;
    statptr    = VAR_ES_STAT;
  }
  else if (set_type == EX_FACE_SET) {
    ex_inq_val = EX_INQ_FACE_SETS;
    idsptr     = VAR_FS_IDS;
    statptr    = VAR_FS_STAT;
  }
  else if (set_type == EX_SIDE_SET) {
    ex_inq_val = EX_INQ_SIDE_SETS;
    idsptr     = VAR_SS_IDS;
    statptr    = VAR_SS_STAT;
  }
  else if (set_type == EX_ELEM_SET) {
    ex_inq_val = EX_INQ_ELEM_SETS;
    idsptr     = VAR_ELS_IDS;
    statptr    = VAR_ELS_STAT;
  }
  else {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: invalid set type (%d)", set_type);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* first check if any sets are specified */
  if ((status = nc_inq_dimid(exoid, exi_dim_num_objects(set_type), &temp)) != NC_NOERR) {
    if (status == NC_EBADDIM) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %ss defined for file id %d",
               ex_name_of_object(set_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %ss defined in file id %d",
               ex_name_of_object(set_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
    }
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire how many sets are to be stored */
  num_sets = ex_inquire_int(exoid, ex_inq_val);
  if (num_sets < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %ss defined for file id %d",
             ex_name_of_object(set_type), exoid);
    /* use error val from inquire */
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Fill out set status array */

  /* First, allocate space for the status list */
  if (!(set_stat = malloc(num_sets * sizeof(int)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for %s status array in file id %d",
             ex_name_of_object(set_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (int_size == sizeof(int64_t)) {
    for (i = 0; i < num_sets; i++) {
      set_stat[i] = (((int64_t *)num_entries_per_set)[i] == 0) ? 0 : 1;
    }
  }
  else {
    for (i = 0; i < num_sets; i++) {
      set_stat[i] = (((int *)num_entries_per_set)[i] == 0) ? 0 : 1;
    }
  }

  /* Next, get variable id of status array */
  if ((status = nc_inq_varid(exoid, statptr, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s status in file id %d",
             ex_name_of_object(set_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(set_stat);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  status = nc_put_var_int(exoid, varid, set_stat);

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s status array to file id %d",
             ex_name_of_object(set_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(set_stat);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* put netcdf file into define mode  */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(set_stat);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* create set definitions */
  for (i = 0; i < num_sets; i++) {
    int64_t set_id;
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      set_id = ((int64_t *)set_specs->sets_ids)[i];
    }
    else {
      set_id = ((int *)set_specs->sets_ids)[i];
    }

    /* Keep track of the total number of sets defined using a counter stored
       in a linked list keyed by exoid.
       NOTE: exi_get_file_item  is used to find the number of sets of type
       for a specific file and returns that value.
    */
    cur_num_sets = exi_get_file_item(exoid, exi_get_counter_list(set_type));
    if (cur_num_sets >= num_sets) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: exceeded number of %ss (%d) defined in file id %d",
               ex_name_of_object(set_type), num_sets, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret;
    }

    /*   NOTE: exi_inc_file_item  is used to find the number of sets
         for a specific file and returns that value incremented. */

    cur_num_sets = exi_inc_file_item(exoid, exi_get_counter_list(set_type));
    set_id_ndx   = cur_num_sets + 1;

    /* setup more pointers based on set_type */
    if (set_type == EX_NODE_SET) {
      elemptr  = VAR_NODE_NS(set_id_ndx);
      extraptr = NULL;
      /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
      numdfptr = DIM_NUM_NOD_NS(set_id_ndx);
      factptr  = VAR_FACT_NS(set_id_ndx);
    }
    else if (set_type == EX_EDGE_SET) {
      elemptr  = VAR_EDGE_ES(set_id_ndx);
      extraptr = VAR_ORNT_ES(set_id_ndx);
      numdfptr = DIM_NUM_DF_ES(set_id_ndx);
      factptr  = VAR_FACT_ES(set_id_ndx);
    }
    else if (set_type == EX_FACE_SET) {
      elemptr  = VAR_FACE_FS(set_id_ndx);
      extraptr = VAR_ORNT_FS(set_id_ndx);
      numdfptr = DIM_NUM_DF_FS(set_id_ndx);
      factptr  = VAR_FACT_FS(set_id_ndx);
    }
    else if (set_type == EX_SIDE_SET) {
      elemptr  = VAR_ELEM_SS(set_id_ndx);
      extraptr = VAR_SIDE_SS(set_id_ndx);
      numdfptr = DIM_NUM_DF_SS(set_id_ndx);
      factptr  = VAR_FACT_SS(set_id_ndx);
    }
    if (set_type == EX_ELEM_SET) {
      elemptr  = VAR_ELEM_ELS(set_id_ndx);
      extraptr = NULL;
      numdfptr = DIM_NUM_DF_ELS(set_id_ndx);
      factptr  = VAR_FACT_ELS(set_id_ndx);
    }

    /*  define dimension for number of entries per set */
    if (set_stat[i] == 0) { /* Is this a NULL set? */
      continue;             /* Do not create anything for NULL sets! */
    }

    if (int_size == sizeof(int)) {
      status = nc_def_dim(exoid, exi_dim_num_entries_in_object(set_type, set_id_ndx),
                          ((int *)num_entries_per_set)[i], &dimid);
    }
    else {
      status = nc_def_dim(exoid, exi_dim_num_entries_in_object(set_type, set_id_ndx),
                          ((int64_t *)num_entries_per_set)[i], &dimid);
    }

    if (status != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: %s entry count %" PRId64 " already defined in file id %d",
                 ex_name_of_object(set_type), set_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of entries for %s %" PRId64 " in file id %d",
                 ex_name_of_object(set_type), set_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      goto error_ret;
    }

    /* create element list variable for set */
    set_int_type = NC_INT;
    if (ex_int64_status(exoid) & EX_BULK_INT64_DB) {
      set_int_type = NC_INT64;
    }

    dims[0] = dimid;
    if ((status = nc_def_var(exoid, elemptr, set_int_type, 1, dims, &temp)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: element list already exists for %s %" PRId64 " in file id %d",
                 ex_name_of_object(set_type), set_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to create element list for %s %" PRId64 " in file id %d",
                 ex_name_of_object(set_type), set_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      goto error_ret; /* exit define mode and return */
    }
    exi_compress_variable(exoid, temp, 1);

    /* create extra list variable for set  (only for edge, face and side sets)
     */
    if (extraptr) {
      if ((status = nc_def_var(exoid, extraptr, set_int_type, 1, dims, &temp)) != NC_NOERR) {
        if (status == NC_ENAMEINUSE) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: extra list already exists for %s %" PRId64 " in file id %d",
                   ex_name_of_object(set_type), set_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        else {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to create extra list for %s %" PRId64 " in file id %d",
                   ex_name_of_object(set_type), set_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        goto error_ret; /* exit define mode and return */
      }
      exi_compress_variable(exoid, temp, 1);
    }

    /*  define dimension for number of dist factors per set */
    /*  NOTE: only define df count if the dist factors exist! */
    if (int_size == sizeof(int64_t)) {
      num_df    = ((int64_t *)num_dist_per_set)[i];
      num_entry = ((int64_t *)num_entries_per_set)[i];
    }
    else {
      num_df    = ((int *)num_dist_per_set)[i];
      num_entry = ((int *)num_entries_per_set)[i];
    }

    if (num_df > 0) {

      if (set_type == EX_NODE_SET) {
        if (num_df != num_entry) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: # dist fact (%zu) not equal to # nodes (%zu) in node set %" PRId64
                   " file id %d",
                   num_df, num_entry, set_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
          goto error_ret; /* exit define mode and return */
        }

        /* reuse dimid from entry lists */
      }
      else {
        if ((status = nc_def_dim(exoid, numdfptr, num_df, &dimid)) != NC_NOERR) {
          if (status == NC_ENAMEINUSE) {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: %s df count %" PRId64 " already defined in file id %d",
                     ex_name_of_object(set_type), set_id, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to define %s df count for set %" PRId64 " in file id %d",
                     ex_name_of_object(set_type), set_id, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          goto error_ret;
        }
      }

      /* create distribution factor list variable for set */
      dims[0] = dimid;
      if ((status = nc_def_var(exoid, factptr, nc_flt_code(exoid), 1, dims, &temp)) != NC_NOERR) {
        if (status == NC_ENAMEINUSE) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: dist factor list already exists for %s %" PRId64 " in file id %d",
                   ex_name_of_object(set_type), set_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        else {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to create dist factor list for %s %" PRId64 " in file id %d",
                   ex_name_of_object(set_type), set_id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        goto error_ret; /* exit define mode and return */
      }
      exi_compress_variable(exoid, temp, 2);
    } /* end define dist factors */
  }

  /* leave define mode  */
  if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
    ex_err_fn(exoid, __func__, errmsg, status);
    free(set_stat);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Next, fill out set ids array */

  /* first get id of set ids array variable */
  if ((status = nc_inq_varid(exoid, idsptr, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s ids array in file id %d",
             ex_name_of_object(set_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(set_stat);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* then, write out set id list */
  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    status = nc_put_var_longlong(exoid, varid, set_specs->sets_ids);
  }
  else {
    status = nc_put_var_int(exoid, varid, set_specs->sets_ids);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s id array in file id %d",
             ex_name_of_object(set_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(set_stat);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* If the sets_entry_index is passed in as a NULL pointer, then
   *  the user only wants us to define the sets and not populate
   *  the data structures.
   */
  if (sets_entry_index == 0) {
    free(set_stat);
    EX_FUNC_LEAVE(EX_NOERR);
  }

  /* Now, use ExodusII call to store sets */
  for (i = 0; i < num_sets; i++) {
    int64_t set_id;
    size_t  df_ndx;

    if (set_stat[i] == 0) { /* Is this a NULL set? */
      continue;             /* Do not create anything for NULL sets! */
    }

    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      set_id = ((int64_t *)set_specs->sets_ids)[i];
    }
    else {
      set_id = ((int *)set_specs->sets_ids)[i];
    }

    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      int64_t *extra_list = NULL;
      /* set extra list */
      if (set_type == EX_EDGE_SET || set_type == EX_FACE_SET || set_type == EX_SIDE_SET) {
        extra_list = &(((int64_t *)set_specs->sets_extra_list)[((int64_t *)sets_entry_index)[i]]);
      }

      status = ex_put_set(
          exoid, set_type, set_id,
          &(((int64_t *)set_specs->sets_entry_list)[((int64_t *)sets_entry_index)[i]]), extra_list);
    }
    else {
      int *extra_list = NULL;
      /* set extra list */
      if (set_type == EX_EDGE_SET || set_type == EX_FACE_SET || set_type == EX_SIDE_SET) {
        extra_list = &(((int *)set_specs->sets_extra_list)[((int *)sets_entry_index)[i]]);
      }

      status = ex_put_set(exoid, set_type, set_id,
                          &(((int *)set_specs->sets_entry_list)[((int *)sets_entry_index)[i]]),
                          extra_list);
    }
    if (status != NC_NOERR) {
      free(set_stat);
      EX_FUNC_LEAVE(EX_FATAL); /* error will be reported by subroutine */
    }

    if (int_size == sizeof(int)) {
      num_df = ((int *)num_dist_per_set)[i];
      df_ndx = ((int *)sets_dist_index)[i];
    }
    else {
      num_df = ((int64_t *)num_dist_per_set)[i];
      df_ndx = ((int64_t *)sets_dist_index)[i];
    }

    if (exi_comp_ws(exoid) == sizeof(float)) {
      flt_dist_fact = sets_dist_fact;
      if (num_df > 0) { /* store dist factors if required */
        if (ex_put_set_dist_fact(exoid, set_type, set_id, &(flt_dist_fact[df_ndx])) == -1) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to store %s %" PRId64 " dist factors for file id %d",
                   ex_name_of_object(set_type), set_id, exoid);
          /* use error val from exodusII routine */
          ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
          free(set_stat);
          EX_FUNC_LEAVE(EX_FATAL);
        }
      }
    }
    else if (exi_comp_ws(exoid) == sizeof(double)) {
      dbl_dist_fact = sets_dist_fact;
      if (num_df) { /* only store if they exist */
        if (ex_put_set_dist_fact(exoid, set_type, set_id, &(dbl_dist_fact[df_ndx])) == -1) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to store %s %" PRId64 " dist factors for file id %d",
                   ex_name_of_object(set_type), set_id, exoid);
          /* use error val from exodusII routine */
          ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
          free(set_stat);
          EX_FUNC_LEAVE(EX_FATAL);
        }
      }
    }
    else {
      /* unknown floating point word size */
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: unsupported floating point word size %d for file id %d", exi_comp_ws(exoid),
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      free(set_stat);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  free(set_stat);
  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  free(set_stat);

  exi_leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

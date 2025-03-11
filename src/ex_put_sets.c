/*
 * Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_set, ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * writes the set parameters and optionally set data for 1 or more sets
 * \param   exoid                   exodus file id
 * \param   set_count               number of sets to write
 * \param  *sets                    array of ex_set structures
 */

int ex_put_sets(int exoid, size_t set_count, const struct ex_set *sets)
{
  size_t i;
  int    needs_define = 0;
  int    set_stat;
  int    dimid, varid, status, dims[1];
  int    set_id_ndx;
  size_t start[1];
  int    cur_num_sets;
  char   errmsg[MAX_ERR_LENGTH];
  int   *sets_to_define = NULL;
  char  *numentryptr    = NULL;
  char  *entryptr       = NULL;
  char  *extraptr       = NULL;
  char  *idsptr         = NULL;
  char  *statptr        = NULL;
  char  *numdfptr       = NULL;
  char  *factptr        = NULL;

  int int_type;

  EX_FUNC_ENTER();

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (!(sets_to_define = malloc(set_count * sizeof(int)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate memory for internal sets_to_define "
             "array in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Note that this routine can be called:
     1) just define the sets
     2) just output the set data (after a previous call to define)
     3) define and output the set data in one call.
  */
  for (i = 0; i < set_count; i++) {
    /* first check if any sets are specified */
    if ((status = nc_inq_dimid(exoid, exi_dim_num_objects(sets[i].type), &dimid)) != NC_NOERR) {
      if (status == NC_EBADDIM) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %ss defined for file id %d",
                 ex_name_of_object(sets[i].type), exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %ss defined in file id %d",
                 ex_name_of_object(sets[i].type), exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      free(sets_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (sets[i].id < 0) {
      /* We are adding a set with id = -sets[i].id. We want to define
       * everything, but we don't
       * want to increment the number of sets...  Major kluge / proof of concept
       */
      needs_define++;
      sets_to_define[i] = -1;
    }
    else {
      status = exi_id_lkup(exoid, sets[i].type, sets[i].id);
      if (status != -EX_LOOKUPFAIL) { /* found the side set id, so set is already defined... */
        sets_to_define[i] = 0;
      }
      else {
        needs_define++;
        sets_to_define[i] = 1;
      }
    }
  }

  if (needs_define > 0) {
    /* put netcdf file into define mode  */
    if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file id %d into define mode", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(sets_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    for (i = 0; i < set_count; i++) {
      if (sets_to_define[i] == 0) {
        continue;
      }

      if (sets_to_define[i] > 0) {
        /*   NOTE: exi_inc_file_item finds the current number of sets defined
             for a specific file and returns that value incremented. */
        cur_num_sets      = exi_inc_file_item(exoid, exi_get_counter_list(sets[i].type));
        set_id_ndx        = cur_num_sets + 1;
        sets_to_define[i] = set_id_ndx;
      }
      else {
        cur_num_sets      = exi_get_file_item(exoid, exi_get_counter_list(sets[i].type));
        set_id_ndx        = cur_num_sets - set_count + i + 1;
        sets_to_define[i] = set_id_ndx;
      }

      if (sets[i].num_entry == 0) {
        continue;
      }

      /* setup pointers based on set_type */
      if (sets[i].type == EX_NODE_SET) {
        numentryptr = DIM_NUM_NOD_NS(set_id_ndx);
        entryptr    = VAR_NODE_NS(set_id_ndx);
        extraptr    = NULL;
        /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
        numdfptr = DIM_NUM_NOD_NS(set_id_ndx);
        factptr  = VAR_FACT_NS(set_id_ndx);
      }
      else if (sets[i].type == EX_EDGE_SET) {
        numentryptr = DIM_NUM_EDGE_ES(set_id_ndx);
        entryptr    = VAR_EDGE_ES(set_id_ndx);
        extraptr    = VAR_ORNT_ES(set_id_ndx);
        numdfptr    = DIM_NUM_DF_ES(set_id_ndx);
        factptr     = VAR_FACT_ES(set_id_ndx);
      }
      else if (sets[i].type == EX_FACE_SET) {
        numentryptr = DIM_NUM_FACE_FS(set_id_ndx);
        entryptr    = VAR_FACE_FS(set_id_ndx);
        extraptr    = VAR_ORNT_FS(set_id_ndx);
        numdfptr    = DIM_NUM_DF_FS(set_id_ndx);
        factptr     = VAR_FACT_FS(set_id_ndx);
      }
      else if (sets[i].type == EX_SIDE_SET) {
        numentryptr = DIM_NUM_SIDE_SS(set_id_ndx);
        entryptr    = VAR_ELEM_SS(set_id_ndx);
        extraptr    = VAR_SIDE_SS(set_id_ndx);
        numdfptr    = DIM_NUM_DF_SS(set_id_ndx);
        factptr     = VAR_FACT_SS(set_id_ndx);
      }
      else if (sets[i].type == EX_ELEM_SET) {
        numentryptr = DIM_NUM_ELE_ELS(set_id_ndx);
        entryptr    = VAR_ELEM_ELS(set_id_ndx);
        extraptr    = NULL;
        numdfptr    = DIM_NUM_DF_ELS(set_id_ndx);
        factptr     = VAR_FACT_ELS(set_id_ndx);
      }

      /* define dimensions and variables */
      if ((status = nc_def_dim(exoid, numentryptr, sets[i].num_entry, &dimid)) != NC_NOERR) {
        if (status == NC_ENAMEINUSE) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: %s %" PRId64 " -- size already defined in file id %d",
                   ex_name_of_object(sets[i].type), sets[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        else {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to define number of entries in %s %" PRId64 " in file id %d",
                   ex_name_of_object(sets[i].type), sets[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        goto error_ret;
      }

      int_type = NC_INT;
      if (ex_int64_status(exoid) & EX_BULK_INT64_DB) {
        int_type = NC_INT64;
      }

      /* create variable array in which to store the entry lists */
      dims[0] = dimid;
      if ((status = nc_def_var(exoid, entryptr, int_type, 1, dims, &varid)) != NC_NOERR) {
        if (status == NC_ENAMEINUSE) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: entry list already exists for %s %" PRId64 " in file id %d",
                   ex_name_of_object(sets[i].type), sets[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        else {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to create entry list for %s %" PRId64 " in file id %d",
                   ex_name_of_object(sets[i].type), sets[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
        }
        goto error_ret; /* exit define mode and return */
      }
      exi_compress_variable(exoid, varid, 1);

      if (extraptr) {
        if ((status = nc_def_var(exoid, extraptr, int_type, 1, dims, &varid)) != NC_NOERR) {
          if (status == NC_ENAMEINUSE) {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: extra list already exists for %s %" PRId64 " in file id %d",
                     ex_name_of_object(sets[i].type), sets[i].id, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to create extra list for %s %" PRId64 " in file id %d",
                     ex_name_of_object(sets[i].type), sets[i].id, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          goto error_ret; /* exit define mode and return */
        }
        exi_compress_variable(exoid, varid, 1);
      }

      /* Create distribution factors variable if required */
      if (sets[i].num_distribution_factor > 0) {
        if (sets[i].type != EX_SIDE_SET) {
          /* but sets[i].num_distribution_factor must equal number of nodes */
          if (sets[i].num_distribution_factor != sets[i].num_entry) {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: # dist fact (%" PRId64 ") not equal to # nodes (%" PRId64
                     ") in node  set %" PRId64 " file id %d",
                     sets[i].num_distribution_factor, sets[i].num_entry, sets[i].id, exoid);
            ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
            goto error_ret; /* exit define mode and return */
          }
        }
        else {
          /* reuse dimid from entry lists */
          if ((status = nc_def_dim(exoid, numdfptr, sets[i].num_distribution_factor, &dimid)) !=
              NC_NOERR) {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to define number of dist factors in %s %" PRId64
                     " in file id %d",
                     ex_name_of_object(sets[i].type), sets[i].id, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
            goto error_ret; /* exit define mode and return */
          }
        }

        /* create variable array in which to store the set distribution factors
         */
        dims[0] = dimid;
        if ((status = nc_def_var(exoid, factptr, nc_flt_code(exoid), 1, dims, &varid)) !=
            NC_NOERR) {
          if (status == NC_ENAMEINUSE) {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: dist factors list already exists for %s %" PRId64 " in file id %d",
                     ex_name_of_object(sets[i].type), sets[i].id, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to create dist factors list for %s %" PRId64 " in file id %d",
                     ex_name_of_object(sets[i].type), sets[i].id, exoid);
            ex_err_fn(exoid, __func__, errmsg, status);
          }
          goto error_ret; /* exit define mode and return */
        }
        exi_compress_variable(exoid, varid, 2);
      }
    }

    /* leave define mode  */
    if ((status = exi_leavedef(exoid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to exit define mode");
      ex_err_fn(exoid, __func__, errmsg, status);
      free(sets_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Output the set ids and status... */
    for (i = 0; i < set_count; i++) {
      /* setup pointers based on sets[i].type */
      if (sets[i].type == EX_NODE_SET) {
        idsptr  = VAR_NS_IDS;
        statptr = VAR_NS_STAT;
      }
      else if (sets[i].type == EX_EDGE_SET) {
        idsptr  = VAR_ES_IDS;
        statptr = VAR_ES_STAT;
      }
      else if (sets[i].type == EX_FACE_SET) {
        idsptr  = VAR_FS_IDS;
        statptr = VAR_FS_STAT;
      }
      else if (sets[i].type == EX_SIDE_SET) {
        idsptr  = VAR_SS_IDS;
        statptr = VAR_SS_STAT;
      }
      else if (sets[i].type == EX_ELEM_SET) {
        idsptr  = VAR_ELS_IDS;
        statptr = VAR_ELS_STAT;
      }

      /* first: get id of set id variable */
      if ((status = nc_inq_varid(exoid, idsptr, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s %" PRId64 " in file id %d",
                 ex_name_of_object(sets[i].type), sets[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        free(sets_to_define);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* write out set id */
      start[0]     = sets_to_define[i] - 1;
      long long id = sets[i].id;
      if (id < 0) {
        id = -id;
      }
      status = nc_put_var1_longlong(exoid, varid, start, &id);

      if (status != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s id %" PRId64 " in file id %d",
                 ex_name_of_object(sets[i].type), sets[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        free(sets_to_define);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      set_stat = (sets[i].num_entry == 0) ? 0 : 1;

      if ((status = nc_inq_varid(exoid, statptr, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s status in file id %d",
                 ex_name_of_object(sets[i].type), exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        free(sets_to_define);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      if ((status = nc_put_var1_int(exoid, varid, start, &set_stat)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to store %s %" PRId64 " status to file id %d",
                 ex_name_of_object(sets[i].type), sets[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        free(sets_to_define);
        EX_FUNC_LEAVE(EX_FATAL);
      }
    }
  }

  free(sets_to_define);

  /* Sets are now all defined; see if any set data needs to be output... */
  status = EX_NOERR;
  for (i = 0; i < set_count; i++) {
    int       stat;
    long long id = sets[i].id;
    if (id < 0) {
      id = -id;
    }
    if (sets[i].entry_list != NULL || sets[i].extra_list != NULL) {
      /* NOTE: ex_put_set will write the warning/error message... */
      stat = ex_put_set(exoid, sets[i].type, id, sets[i].entry_list, sets[i].extra_list);
      if (stat != EX_NOERR) {
        status = EX_FATAL;
      }
    }
    if (sets[i].num_distribution_factor > 0 && sets[i].distribution_factor_list != NULL) {
      /* NOTE: ex_put_set_dist_fact will write the warning/error message... */
      stat = ex_put_set_dist_fact(exoid, sets[i].type, id, sets[i].distribution_factor_list);
      if (stat != EX_NOERR) {
        status = EX_FATAL;
      }
    }
  }
  EX_FUNC_LEAVE(status);

/* Fatal error: exit definition mode and return */
error_ret:
  free(sets_to_define);

  exi_leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

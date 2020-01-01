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
/*!
 *
 * \undoc exgcss - ex_get_concat_sets
 *
 * reads the set ID's, set entry count array, set entry pointers
 * array, set entry list, set extra list, and set distribution factors
 * for all sets of the specified type.
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       ex_entity_type set_type                type of set
 *
 * exit conditions -
 *       struct ex_set_specs* set_specs  set specs structure
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_set_specs, ex_err, etc
#include "exodusII_int.h" // for ex__check_valid_file_id, etc

int ex_get_concat_sets(int exoid, ex_entity_type set_type, struct ex_set_specs *set_specs)
{
  int       status, dimid;
  void_int *num_entries_per_set = set_specs->num_entries_per_set;
  void_int *num_dist_per_set    = set_specs->num_dist_per_set;
  void_int *sets_entry_index    = set_specs->sets_entry_index;
  void_int *sets_dist_index     = set_specs->sets_dist_index;

  void *sets_dist_fact = set_specs->sets_dist_fact;

  int        num_sets, i;
  float *    flt_dist_fact;
  double *   dbl_dist_fact;
  char       errmsg[MAX_ERR_LENGTH];
  ex_inquiry ex_inq_val;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* setup pointers based on set_type
     NOTE: there is another block that sets more stuff later ... */

  if (set_type == EX_NODE_SET) {
    ex_inq_val = EX_INQ_NODE_SETS;
  }
  else if (set_type == EX_EDGE_SET) {
    ex_inq_val = EX_INQ_EDGE_SETS;
  }
  else if (set_type == EX_FACE_SET) {
    ex_inq_val = EX_INQ_FACE_SETS;
  }
  else if (set_type == EX_SIDE_SET) {
    ex_inq_val = EX_INQ_SIDE_SETS;
  }
  else if (set_type == EX_ELEM_SET) {
    ex_inq_val = EX_INQ_ELEM_SETS;
  }
  else {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: invalid set type (%d)", set_type);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* first check if any sets are specified */

  if ((status = nc_inq_dimid(exoid, ex__dim_num_objects(set_type), &dimid)) != NC_NOERR) {
    if (status == NC_EBADDIM) {
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no %ss defined for file id %d",
               ex_name_of_object(set_type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_WARN);
    }
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %ss defined in file id %d",
             ex_name_of_object(set_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire how many sets have been stored */

  num_sets = ex_inquire_int(exoid, ex_inq_val);
  if (num_sets < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %ss defined for file id %d",
             ex_name_of_object(set_type), exoid);
    /* use error val from inquire */
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_ids(exoid, set_type, set_specs->sets_ids) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get %s ids for file id %d",
             ex_name_of_object(set_type), exoid);
    /* use error val from inquire */
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    ((int64_t *)sets_entry_index)[0] = 0;
    ((int64_t *)sets_dist_index)[0]  = 0;
  }
  else {
    ((int *)sets_entry_index)[0] = 0;
    ((int *)sets_dist_index)[0]  = 0;
  }

  for (i = 0; i < num_sets; i++) {
    int set_id;
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      set_id = ((int64_t *)set_specs->sets_ids)[i];
    }
    else {
      set_id = ((int *)set_specs->sets_ids)[i];
    }

    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      if (ex_get_set_param(exoid, set_type, set_id, &(((int64_t *)num_entries_per_set)[i]),
                           &(((int64_t *)num_dist_per_set)[i])) != NC_NOERR) {
        EX_FUNC_LEAVE(EX_FATAL); /* error will be reported by sub */
      }

      if (i < num_sets - 1) {
        /* fill in entry and dist factor index arrays */
        ((int64_t *)sets_entry_index)[i + 1] =
            ((int64_t *)sets_entry_index)[i] + ((int64_t *)num_entries_per_set)[i];
        ((int64_t *)sets_dist_index)[i + 1] =
            ((int64_t *)sets_dist_index)[i] + ((int64_t *)num_dist_per_set)[i];
      }

      if (((int64_t *)num_entries_per_set)[i] == 0) { /* NULL  set? */
        continue;
      }

      {
        /* Now, use ExodusII call to get sets */
        int64_t *sets_entry_list = set_specs->sets_entry_list;
        int64_t *sets_extra_list = set_specs->sets_extra_list;
        int64_t *sets_extra =
            sets_extra_list ? &(sets_extra_list)[((int64_t *)sets_entry_index)[i]] : NULL;
        status = ex_get_set(exoid, set_type, set_id,
                            &(sets_entry_list[((int64_t *)sets_entry_index)[i]]), sets_extra);
      }
    }
    else {
      if (ex_get_set_param(exoid, set_type, set_id, &(((int *)num_entries_per_set)[i]),
                           &(((int *)num_dist_per_set)[i])) != NC_NOERR) {
        EX_FUNC_LEAVE(EX_FATAL); /* error will be reported by sub */
      }

      if (i < num_sets - 1) {
        /* fill in entry and dist factor index arrays */
        ((int *)sets_entry_index)[i + 1] =
            ((int *)sets_entry_index)[i] + ((int *)num_entries_per_set)[i];
        ((int *)sets_dist_index)[i + 1] =
            ((int *)sets_dist_index)[i] + ((int *)num_dist_per_set)[i];
      }

      if (((int *)num_entries_per_set)[i] == 0) { /* NULL  set? */
        continue;
      }

      {
        /* Now, use ExodusII call to get sets */
        int *sets_entry_list = set_specs->sets_entry_list;
        int *sets_extra_list = set_specs->sets_extra_list;
        int *sets_extra = sets_extra_list ? &(sets_extra_list)[((int *)sets_entry_index)[i]] : NULL;
        status          = ex_get_set(exoid, set_type, set_id,
                            &(sets_entry_list[((int *)sets_entry_index)[i]]), sets_extra);
      }
    }

    if (status != NC_NOERR) {
      EX_FUNC_LEAVE(EX_FATAL); /* error will be reported by subroutine */
    }

    /* get distribution factors for this set */
    if (sets_dist_fact != 0) {
      size_t df_idx;
      size_t num_dist;
      if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
        df_idx   = ((int64_t *)sets_dist_index)[i];
        num_dist = ((int64_t *)num_dist_per_set)[i];
      }
      else {
        df_idx   = ((int *)sets_dist_index)[i];
        num_dist = ((int *)num_dist_per_set)[i];
      }
      if (num_dist > 0) { /* only get df if they exist */
        if (ex__comp_ws(exoid) == sizeof(float)) {
          flt_dist_fact = sets_dist_fact;
          status        = ex_get_set_dist_fact(exoid, set_type, set_id, &(flt_dist_fact[df_idx]));
        }
        else {
          dbl_dist_fact = sets_dist_fact;
          status        = ex_get_set_dist_fact(exoid, set_type, set_id, &(dbl_dist_fact[df_idx]));
        }
        if (status != NC_NOERR) {
          EX_FUNC_LEAVE(EX_FATAL); /* error will be reported by subroutine */
        }
      }
    }
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

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
 * exgsetp - ex_get_set_param
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     set_type                the type of set
 *       int     set_id                  set id
 *
 * exit conditions -
 *       int*    num_entries_in_set      number of entries in the set
 *       int*    num_dist_fact_in_set    number of distribution factors in the
 *                                       set
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, EX_NOERR, etc

/*
 * reads the number of entries and the number of distribution factors which
 * describe a single set
 */

int ex_get_set_param(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                     void_int *num_entry_in_set, void_int *num_dist_fact_in_set)
{
  int    status;
  int    varid, dimid, set_id_ndx;
  size_t lnum_entry_in_set;
  size_t lnum_dist_fact_in_set;
  char   errmsg[MAX_ERR_LENGTH];
  char * numentryptr = NULL;
  char * numdfptr    = NULL;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    if (num_entry_in_set) {
      *(int64_t *)num_entry_in_set = 0;
    }
    if (num_dist_fact_in_set) {
      *(int64_t *)num_dist_fact_in_set = 0;
    }
  }
  else {
    if (num_entry_in_set) {
      *(int *)num_entry_in_set = 0;
    }
    if (num_dist_fact_in_set) {
      *(int *)num_dist_fact_in_set = 0;
    }
  }
  /* first check if any sets are specified */
  if ((status = nc_inq_dimid(exoid, ex__dim_num_objects(set_type), &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no %ss stored in file id %d",
             ex_name_of_object(set_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* Lookup index of set id in VAR_*S_IDS array */
  set_id_ndx = ex__id_lkup(exoid, set_type, set_id);
  if (set_id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      if (status == EX_NULLENTITY) {
        EX_FUNC_LEAVE(EX_NOERR);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in id array in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* setup more pointers based on set_type */
  if (set_type == EX_NODE_SET) {
    numentryptr = DIM_NUM_NOD_NS(set_id_ndx);
    /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
    numdfptr = DIM_NUM_NOD_NS(set_id_ndx);
  }
  else if (set_type == EX_EDGE_SET) {
    numentryptr = DIM_NUM_EDGE_ES(set_id_ndx);
    numdfptr    = DIM_NUM_DF_ES(set_id_ndx);
  }
  else if (set_type == EX_FACE_SET) {
    numentryptr = DIM_NUM_FACE_FS(set_id_ndx);
    numdfptr    = DIM_NUM_DF_FS(set_id_ndx);
  }
  else if (set_type == EX_SIDE_SET) {
    numentryptr = DIM_NUM_SIDE_SS(set_id_ndx);
    numdfptr    = DIM_NUM_DF_SS(set_id_ndx);
  }
  if (set_type == EX_ELEM_SET) {
    numentryptr = DIM_NUM_ELE_ELS(set_id_ndx);
    numdfptr    = DIM_NUM_DF_ELS(set_id_ndx);
  }

  /* inquire values of dimension for number of entities in set */
  if (ex__get_dimension(exoid, numentryptr, "entries", &lnum_entry_in_set, &dimid, __func__) !=
      NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    if (num_entry_in_set) {
      *(int64_t *)num_entry_in_set = lnum_entry_in_set;
    }
  }
  else {
    if (num_entry_in_set) {
      *(int *)num_entry_in_set = lnum_entry_in_set;
    }
  }

  /* Inquire value of dimension of number of dist factors for this set.
     NOTE: For node sets, because DIM_NUM_DF_NS is not used, we check to see
     if the dist factor variable for a node set index exits. If it does not,
     the dist factor count is assumed to be zero, otherwise the dist factor
     count will be the same as the number of nodes in the set. */

  if (set_type == EX_NODE_SET) {
    if ((status = nc_inq_varid(exoid, VAR_FACT_NS(set_id_ndx), &varid)) != NC_NOERR) {
      if (status == NC_ENOTVAR) {
        EX_FUNC_LEAVE(EX_NOERR);
      }

      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate the dist factors for %s %" PRId64 " in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      if (num_dist_fact_in_set) {
        *(int64_t *)num_dist_fact_in_set = lnum_entry_in_set;
      }
    }
    else {
      if (num_dist_fact_in_set) {
        *(int *)num_dist_fact_in_set = lnum_entry_in_set;
      }
    }
  }
  else { /* all other set types */
    if ((status = nc_inq_dimid(exoid, numdfptr, &dimid)) != NC_NOERR) {
      if (status == NC_EBADDIM) {
        EX_FUNC_LEAVE(EX_NOERR);
      }

      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate number of dist factors in %s %" PRId64 " in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_inq_dimlen(exoid, dimid, &lnum_dist_fact_in_set)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get number of dist factors in %s %" PRId64 " in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      if (num_dist_fact_in_set) {
        *(int64_t *)num_dist_fact_in_set = lnum_dist_fact_in_set;
      }
    }
    else {
      if (num_dist_fact_in_set) {
        *(int *)num_dist_fact_in_set = lnum_dist_fact_in_set;
      }
    }
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

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
 * exgssc - ex_get_side_set_node_count
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     side_set_id             side set id
 *
 * exit conditions -
 *       int     *side_set_node_cnt_list returned array of number of nodes for
 *                                       side or face
 * revision history -
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, ex_block, etc
#include "exodusII_int.h" // for elem_blk_parm, EX_FATAL, etc
#include <assert.h>       // for assert
#include <ctype.h>        // for toupper
#include <inttypes.h>     // for PRId64
#include <stddef.h>       // for size_t
#include <stdio.h>
#include <stdlib.h>    // for free, malloc
#include <string.h>    // for strncmp, strlen
#include <sys/types.h> // for int64_t

int ex_get_side_set_node_count(int exoid, ex_entity_id side_set_id, int *side_set_node_cnt_list)
{
  int       status;
  size_t    ii, i, j;
  int       num_side_sets, num_elem_blks, ndim;
  size_t    tot_num_ss_elem = 0;
  int64_t   side, elem;
  void_int *elem_blk_ids       = NULL;
  void_int *ss_elem_ndx        = NULL;
  void_int *side_set_elem_list = NULL;
  void_int *side_set_side_list = NULL;
  size_t    elem_ctr;

  struct elem_blk_parm *elem_blk_parms = NULL;

  char errmsg[MAX_ERR_LENGTH];
  int  err_stat = EX_NOERR;

  EX_FUNC_ENTER();
  ex_check_valid_file_id(exoid, __func__);

  /* first check if any side sets are specified */
  /* inquire how many side sets have been stored */
  num_side_sets = ex_inquire_int(exoid, EX_INQ_SIDE_SETS);
  if (num_side_sets < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of side sets in file id %d",
             exoid);
    ex_err(__func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (num_side_sets == 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no side sets defined in file id %d", exoid);
    ex_err(__func__, errmsg, EX_WARN);
    EX_FUNC_LEAVE(EX_WARN);
  }

  /* Lookup index of side set id in VAR_SS_IDS array */
  if (ex_id_lkup(exoid, EX_SIDE_SET, side_set_id) <= 0) {
    ;
  }
  ex_get_err(NULL, NULL, &status);

  if (status != 0) {
    if (status == EX_NULLENTITY) {
      snprintf(errmsg, MAX_ERR_LENGTH, "Warning: side set %" PRId64 " is NULL in file id %d",
               side_set_id, exoid);
      ex_err(__func__, errmsg, EX_NULLENTITY);
      EX_FUNC_LEAVE(EX_WARN);
    }

    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate side set %" PRId64 " in VAR_SS_IDS array in file id %d",
             side_set_id, exoid);
    ex_err(__func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  num_elem_blks = ex_inquire_int(exoid, EX_INQ_ELEM_BLK);
  if (num_elem_blks < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of element blocks in file id %d",
             exoid);
    ex_err(__func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* get the dimensionality of the coordinates;  this is necessary to
     distinguish between 2d TRIs and 3d TRIs */
  ndim = ex_inquire_int(exoid, EX_INQ_DIM);
  if (ndim < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get dimensionality in file id %d", exoid);
    ex_err(__func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int int_size = sizeof(int);
  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    int_size = sizeof(int64_t);
  }

  /* First determine the  # of elements in the side set*/
  int err;
  if (int_size == sizeof(int64_t)) {
    int64_t ss_elem = 0;
    int64_t ss_df   = 0;
    err             = ex_get_set_param(exoid, EX_SIDE_SET, side_set_id, &ss_elem, &ss_df);
    tot_num_ss_elem = ss_elem;
  }
  else {
    int ss_elem     = 0;
    int ss_df       = 0;
    err             = ex_get_set_param(exoid, EX_SIDE_SET, side_set_id, &ss_elem, &ss_df);
    tot_num_ss_elem = ss_elem;
  }

  if (err == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get number of elements in side set %" PRId64 " in file id %d",
             side_set_id, exoid);
    ex_err(__func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Allocate space for the side set element list */
  {
    if (!(side_set_elem_list = malloc(tot_num_ss_elem * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for side set element "
               "list for file id %d",
               exoid);
      ex_err(__func__, errmsg, EX_MEMFAIL);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Allocate space for the side set side list */
    if (!(side_set_side_list = malloc(tot_num_ss_elem * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for side set side list "
               "for file id %d",
               exoid);
      ex_err(__func__, errmsg, EX_MEMFAIL);
      err_stat = EX_FATAL;
      goto cleanup;
    }

    if (ex_get_set(exoid, EX_SIDE_SET, side_set_id, side_set_elem_list, side_set_side_list) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get side set %" PRId64 " in file id %d",
               side_set_id, exoid);
      ex_err(__func__, errmsg, EX_LASTERR);
      err_stat = EX_FATAL;
      goto cleanup;
    }

    /* Allocate space for the ss element index array */
    if (!(ss_elem_ndx = malloc(tot_num_ss_elem * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for side set elem sort "
               "array for file id %d",
               exoid);
      ex_err(__func__, errmsg, EX_MEMFAIL);
      err_stat = EX_FATAL;
      goto cleanup;
    }

    if (int_size == sizeof(int64_t)) {
      /* Sort side set element list into index array  - non-destructive */
      int64_t *elems = (int64_t *)ss_elem_ndx;
      for (i = 0; i < tot_num_ss_elem; i++) {
        elems[i] = i; /* init index array to current position */
      }
      ex_iqsort64(side_set_elem_list, elems, tot_num_ss_elem);
    }
    else {
      /* Sort side set element list into index array  - non-destructive */
      int *elems = (int *)ss_elem_ndx;
      for (i = 0; i < tot_num_ss_elem; i++) {
        elems[i] = i; /* init index array to current position */
      }
      ex_iqsort(side_set_elem_list, elems, tot_num_ss_elem);
    }
  }

  /* Allocate space for the element block ids */
  {
    int_size = sizeof(int);
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      int_size = sizeof(int64_t);
    }

    if (!(elem_blk_ids = malloc(num_elem_blks * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for element block ids "
               "for file id %d",
               exoid);
      ex_err(__func__, errmsg, EX_MEMFAIL);
      err_stat = EX_FATAL;
      goto cleanup;
    }

    if (ex_get_ids(exoid, EX_ELEM_BLOCK, elem_blk_ids) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get element block ids in file id %d",
               exoid);
      ex_err(__func__, errmsg, EX_MSG);
      err_stat = EX_FATAL;
      goto cleanup;
    }
  }

  /* Allocate space for the element block params */
  if (!(elem_blk_parms = malloc(num_elem_blks * sizeof(struct elem_blk_parm)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for element block params "
             "for file id %d",
             exoid);
    ex_err(__func__, errmsg, EX_MEMFAIL);
    err_stat = EX_FATAL;
    goto cleanup;
  }

  elem_ctr = 0;
  for (i = 0; i < num_elem_blks; i++) {
    ex_entity_id id;
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      id = ((int64_t *)elem_blk_ids)[i];
    }
    else {
      id = ((int *)elem_blk_ids)[i];
    }

    err_stat = ex_int_get_block_param(exoid, id, ndim, &elem_blk_parms[i]);
    if (err_stat != EX_NOERR) {
      goto cleanup;
    }

    elem_ctr += elem_blk_parms[i].num_elem_in_blk;
    elem_blk_parms[i].elem_ctr = elem_ctr; /* save elem number max */
  }

  /* Finally... Create the list of node counts for each face in the
   * side set.
   */

  j = 0; /* The current element block... */
  for (ii = 0; ii < tot_num_ss_elem; ii++) {

    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      i    = ((int64_t *)ss_elem_ndx)[ii];
      elem = ((int64_t *)side_set_elem_list)[i];
      side = ((int64_t *)side_set_side_list)[i] - 1; /* Convert to 0-based sides */
    }
    else {
      i    = ((int *)ss_elem_ndx)[ii];
      elem = ((int *)side_set_elem_list)[i];
      side = ((int *)side_set_side_list)[i] - 1; /* Convert to 0-based sides */
    }

    /*
     * Since the elements are being accessed in sorted, order, the
     * block that contains the elements must progress sequentially
     * from block 0 to block[num_elem_blks-1]. Once we find an element
     * not in this block, find a following block that contains it...
     */
    for (; j < num_elem_blks; j++) {
      if (elem <= elem_blk_parms[j].elem_ctr) {
        break;
      }
    }

    if (j < num_elem_blks) {
      assert(side < elem_blk_parms[j].num_sides);
      side_set_node_cnt_list[i] = elem_blk_parms[j].num_nodes_per_side[side];
    }
    else {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Invalid element number %" PRId64 " found in side set %" PRId64 " in file %d",
               elem, side_set_id, exoid);
      ex_err(__func__, errmsg, EX_BADPARAM);
      err_stat = EX_FATAL;
      goto cleanup;
    }
  }

/* All done: release connectivity array space, element block ids
 * array, element block parameters array, and side set element index
 * array
 */
cleanup:
  free(elem_blk_ids);
  free(elem_blk_parms);
  free(ss_elem_ndx);
  free(side_set_side_list);
  free(side_set_elem_list);

  EX_FUNC_LEAVE(err_stat);
}

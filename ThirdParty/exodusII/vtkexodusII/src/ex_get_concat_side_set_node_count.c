/*
 * Copyright(C) 1999-2020, 2022, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

/*****************************************************************************
 *
 * exgcssc - ex_get_concat_side_set_node_count
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *
 * exit conditions -
 *       int     *side_set_node_cnt_list returned array of number of nodes for
 *                                       side or face for all sidesets
 * revision history -
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, EX_MSG, etc
#include "exodusII_int.h" // for elem_blk_parm, EX_FATAL, etc

/*! \undoc */
int ex_get_concat_side_set_node_count(int exoid, int *side_set_node_cnt_list)
{
  int          ii, i, j, iss, ioff;
  ex_entity_id side_set_id;
  int64_t      tot_num_ss_elem = 0, num_df = 0;
  void_int    *elem_blk_ids       = NULL;
  void_int    *side_set_ids       = NULL;
  void_int    *ss_elem_ndx        = NULL;
  void_int    *side_set_elem_list = NULL;
  void_int    *side_set_side_list = NULL;
  size_t       elem_ctr;
  int          status;

  struct exi_elem_blk_parm *elem_blk_parms = NULL;

  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* first check if any side sets are specified */
  /* inquire how many side sets have been stored */
  int num_side_sets = (int)ex_inquire_int(exoid, EX_INQ_SIDE_SETS);
  if (num_side_sets < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of side sets in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (num_side_sets == 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "Warning: no side sets defined in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_NOENTITY);
    EX_FUNC_LEAVE(EX_WARN);
  }

  int num_elem_blks = (int)ex_inquire_int(exoid, EX_INQ_ELEM_BLK);
  if (num_elem_blks < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of element blocks in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  int64_t tot_num_elem = ex_inquire_int(exoid, EX_INQ_ELEM);
  if (tot_num_elem < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get total number of elements in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* get the dimensionality of the coordinates;  this is necessary to
     distinguish between 2d TRIs and 3d TRIs */
  int ndim = (int)ex_inquire_int(exoid, EX_INQ_DIM);
  if (ndim < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get dimensionality in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  bool ints_64  = ex_int64_status(exoid) & EX_BULK_INT64_API;
  int  int_size = ints_64 ? sizeof(int64_t) : sizeof(int);

  /* Allocate space for the element block ids */
  int ids_size = sizeof(int);
  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    ids_size = sizeof(int64_t);
  }

  if (!(elem_blk_ids = malloc(num_elem_blks * ids_size))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for element block ids for file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    goto error_ret;
  }

  if (ex_get_ids(exoid, EX_ELEM_BLOCK, elem_blk_ids) != EX_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get element block ids in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    goto error_ret;
  }

  /* Allocate space for the element block params */
  if (!(elem_blk_parms = malloc(num_elem_blks * sizeof(struct exi_elem_blk_parm)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for element block params "
             "for file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    goto error_ret;
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

    if (exi_get_block_param(exoid, id, ndim, &elem_blk_parms[i]) != EX_NOERR) {
      goto error_ret;
    }

    elem_ctr += elem_blk_parms[i].num_elem_in_blk;
    elem_blk_parms[i].elem_ctr = elem_ctr; /* save elem number max */
  }

  /* Finally... Create the list of node counts for each face in the
   * side set.
   */
  /* Allocate space for the sideset ids */
  if (!(side_set_ids = malloc(num_side_sets * ids_size))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for side set ids for file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    goto error_ret;
  }

  if (ex_get_ids(exoid, EX_SIDE_SET, side_set_ids) != EX_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get side set ids in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    goto error_ret;
  }

  /* Lookup index of side set id in VAR_SS_IDS array */
  ioff = 0;
  for (iss = 0; iss < num_side_sets; iss++) {
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      side_set_id = ((int64_t *)side_set_ids)[iss];
    }
    else {
      side_set_id = ((int *)side_set_ids)[iss];
    }

    /* First determine the  # of elements in the side set*/
    if (int_size == sizeof(int64_t)) {
      status = ex_get_set_param(exoid, EX_SIDE_SET, side_set_id, &tot_num_ss_elem, &num_df);
    }
    else {
      int tot, df;
      status          = ex_get_set_param(exoid, EX_SIDE_SET, side_set_id, &tot, &df);
      tot_num_ss_elem = tot;
      num_df          = df;
    }

    if (status != EX_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to get number of elements in side set  %" PRId64 " in file id %d",
               side_set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
      goto error_ret;
    }

    if (tot_num_ss_elem == 0) {
      continue;
    }

    /* Allocate space for the side set element list */
    if (!(side_set_elem_list = malloc(tot_num_ss_elem * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for side set element "
               "list for file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      goto error_ret;
    }

    /* Allocate space for the side set side list */
    if (!(side_set_side_list = malloc(tot_num_ss_elem * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for side set side list "
               "for file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      goto error_ret;
    }

    if (ex_get_set(exoid, EX_SIDE_SET, side_set_id, side_set_elem_list, side_set_side_list) !=
        EX_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get side set  %" PRId64 " in file id %d",
               side_set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
      goto error_ret;
    }

    /* Allocate space for the ss element index array */
    if (!(ss_elem_ndx = malloc(tot_num_ss_elem * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for side set elem sort "
               "array for file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      goto error_ret;
    }

    /* Sort side set element list into index array  - non-destructive */
    if (int_size == sizeof(int64_t)) {
      /* Sort side set element list into index array  - non-destructive */
      int64_t *elems = (int64_t *)ss_elem_ndx;
      for (i = 0; i < tot_num_ss_elem; i++) {
        elems[i] = i; /* init index array to current position */
      }
      exi_iqsort64(side_set_elem_list, ss_elem_ndx, tot_num_ss_elem);
    }
    else {
      /* Sort side set element list into index array  - non-destructive */
      int *elems = (int *)ss_elem_ndx;
      for (i = 0; i < tot_num_ss_elem; i++) {
        elems[i] = i; /* init index array to current position */
      }
      exi_iqsort(side_set_elem_list, ss_elem_ndx, tot_num_ss_elem);
    }

    j = 0; /* The current element block... */
    for (ii = 0; ii < tot_num_ss_elem; ii++) {

      int64_t elem_ndx;
      int64_t elem;
      int64_t side;
      if (ints_64) {
        elem_ndx = ((int64_t *)ss_elem_ndx)[ii];
        elem     = ((int64_t *)side_set_elem_list)[elem_ndx];
        side     = ((int64_t *)side_set_side_list)[elem_ndx] - 1;
      }
      else {
        elem_ndx = ((int *)ss_elem_ndx)[ii];
        elem     = ((int *)side_set_elem_list)[elem_ndx];
        side     = ((int *)side_set_side_list)[elem_ndx] - 1;
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
        side_set_node_cnt_list[elem_ndx + ioff] = elem_blk_parms[j].num_nodes_per_side[side];
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: Invalid element number  %" PRId64 " found in side set  %" PRId64
                 " in file %d",
                 elem, side_set_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
        goto error_ret;
      }
    }
    free(ss_elem_ndx);
    ss_elem_ndx = NULL;
    free(side_set_elem_list);
    side_set_elem_list = NULL;
    free(side_set_side_list);
    side_set_side_list = NULL;
    ioff += tot_num_ss_elem;
  }

  /* All done: release allocated memory */
  free(elem_blk_ids);
  free(side_set_ids);
  free(ss_elem_ndx);
  free(side_set_elem_list);
  free(side_set_side_list);
  free(elem_blk_parms);
  EX_FUNC_LEAVE(EX_NOERR);

error_ret:
  free(elem_blk_ids);
  free(side_set_ids);
  free(ss_elem_ndx);
  free(side_set_elem_list);
  free(side_set_side_list);
  free(elem_blk_parms);
  EX_FUNC_LEAVE(EX_FATAL);
}

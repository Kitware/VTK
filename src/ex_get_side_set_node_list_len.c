/*
 * Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgsnl - ex_get_side_set_node_list_len
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *       int     side_set_id             side set id
 *
 * exit conditions -
 *       int     *side_set_node_list_len length of node list
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for elem_blk_parm, EX_FATAL, etc

/*!
 * This routine is designed to read the Exodus II V 2.0 side set side
 * definition  and return the length of a ExodusI style side set node list.
 * \param           exoid                   exodus file id
 * \param           side_set_id             side set id
 * \param[out]     *side_set_node_list_len length of node list
 */

int ex_get_side_set_node_list_len(int exoid, ex_entity_id side_set_id,
                                  void_int *side_set_node_list_len)
{
  size_t    ii, i, j;
  int64_t   num_side_sets, num_elem_blks, num_df, ndim;
  size_t    list_len     = 0;
  int64_t   tot_num_elem = 0, tot_num_ss_elem = 0;
  void_int *elem_blk_ids   = NULL;
  int      *ss_elem_ndx    = NULL;
  int64_t  *ss_elem_ndx_64 = NULL;

  void_int *side_set_elem_list = NULL;
  void_int *side_set_side_list = NULL;
  size_t    elem_ctr;

  int err_stat = EX_NOERR;
  int status;

  struct exi_elem_blk_parm *elem_blk_parms = NULL;

  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  bool ints_64 = ex_int64_status(exoid) & EX_BULK_INT64_API;
  if (ints_64) {
    *(int64_t *)side_set_node_list_len = 0; /* default value */
  }
  else {
    *(int *)side_set_node_list_len = 0; /* default value */
  }

  /* first check if any side sets are specified */
  /* inquire how many side sets have been stored */

  /* get the dimensionality of the coordinates;  this is necessary to
     distinguish between 2d TRIs and 3d TRIs */

  ndim = ex_inquire_int(exoid, EX_INQ_DIM);
  if (ndim < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get dimensionality in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  tot_num_elem = ex_inquire_int(exoid, EX_INQ_ELEM);
  if (tot_num_elem < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get total number of elements in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  num_elem_blks = ex_inquire_int(exoid, EX_INQ_ELEM_BLK);
  if (num_elem_blks < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of element blocks in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  num_side_sets = ex_inquire_int(exoid, EX_INQ_SIDE_SETS);
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

  /* First determine the  # of elements in the side set*/
  if (ints_64) {
    status = ex_get_set_param(exoid, EX_SIDE_SET, side_set_id, &tot_num_ss_elem, &num_df);
  }
  else {
    int tot;
    int df;
    status          = ex_get_set_param(exoid, EX_SIDE_SET, side_set_id, &tot, &df);
    tot_num_ss_elem = tot;
    num_df          = df;
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to get number of elements in side set %" PRId64 " in file id %d",
             side_set_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (tot_num_ss_elem == 0) { /* NULL side set? */
    EX_FUNC_LEAVE(EX_NOERR);
  }

  /* Allocate space for the side set element list */
  {
    int int_size = ints_64 ? sizeof(int64_t) : sizeof(int);

    if (!(side_set_elem_list = malloc(tot_num_ss_elem * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for side set element "
               "list for file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Allocate space for the side set side list */
    if (!(side_set_side_list = malloc(tot_num_ss_elem * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for side set side list "
               "for file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      err_stat = EX_FATAL;
      goto cleanup;
    }

    if (ex_get_set(exoid, EX_SIDE_SET, side_set_id, side_set_elem_list, side_set_side_list) !=
        NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get side set %" PRId64 " in file id %d",
               side_set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
      err_stat = EX_FATAL;
      goto cleanup;
    }

    /* Allocate space for the ss element index array */
    if (int_size == sizeof(int64_t)) {
      ss_elem_ndx_64 = malloc(tot_num_ss_elem * int_size);
    }
    else {
      ss_elem_ndx = malloc(tot_num_ss_elem * int_size);
    }

    if (ss_elem_ndx_64 == NULL && ss_elem_ndx == NULL) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for side set elem sort "
               "array for file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      err_stat = EX_FATAL;
      goto cleanup;
    }
  }

  /* Sort side set element list into index array  - non-destructive */
  if (ints_64) {
    for (i = 0; i < tot_num_ss_elem; i++) {
      ss_elem_ndx_64[i] = i; /* init index array to current position */
    }
    exi_iqsort64(side_set_elem_list, ss_elem_ndx_64, tot_num_ss_elem);
  }
  else {
    for (i = 0; i < tot_num_ss_elem; i++) {
      ss_elem_ndx[i] = i; /* init index array to current position */
    }
    exi_iqsort(side_set_elem_list, ss_elem_ndx, tot_num_ss_elem);
  }

  /* Allocate space for the element block ids */
  {
    int int_size = sizeof(int);
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      int_size = sizeof(int64_t);
    }

    if (!(elem_blk_ids = malloc(num_elem_blks * int_size))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate space for element block ids "
               "for file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      err_stat = EX_FATAL;
      goto cleanup;
    }
  }

  if (ex_get_ids(exoid, EX_ELEM_BLOCK, elem_blk_ids)) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get element block ids in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    err_stat = EX_FATAL;
    goto cleanup;
  }

  /* Allocate space for the element block params */
  if (!(elem_blk_parms = calloc(num_elem_blks, sizeof(struct exi_elem_blk_parm)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for element block params "
             "for file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
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

    err_stat = exi_get_block_param(exoid, id, ndim, &elem_blk_parms[i]);
    if (err_stat != EX_NOERR) {
      goto cleanup;
    }

    elem_ctr += elem_blk_parms[i].num_elem_in_blk;
    elem_blk_parms[i].elem_ctr = elem_ctr; /* save elem number max */
  }

  /* Walk through element list and keep a running count of the node length */

  list_len = 0;
  j        = 0; /* The current element block... */
  for (ii = 0; ii < tot_num_ss_elem; ii++) {
    size_t elem;
    size_t side;
    if (ints_64) {
      i    = ss_elem_ndx_64[ii];
      elem = ((int64_t *)side_set_elem_list)[i];
      side = ((int64_t *)side_set_side_list)[i];
    }
    else {
      i    = ss_elem_ndx[ii];
      elem = ((int *)side_set_elem_list)[i];
      side = ((int *)side_set_side_list)[i];
    }

    /*
     * Since the elements are being accessed in sorted, order, the
     * block that contains the elements must progress sequentially
     * from block 0 to block[num_elem_blks-1]. Once we find an element
     * not in this block, find a following block that contains it...
     */
    for (; j < num_elem_blks; j++) {
      if (elem_blk_parms[j].elem_type_val != EX_EL_NULL_ELEMENT) {
        if (elem <= elem_blk_parms[j].elem_ctr) {
          break;
        }
      }
    }

    if (j >= num_elem_blks) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Invalid element number %zu found in side set %" PRId64 " in file %d", elem,
               side_set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      err_stat = EX_FATAL;
      goto cleanup;
    }
    list_len += elem_blk_parms[j].num_nodes_per_side[side - 1];
  }

  if (ints_64) {
    *(int64_t *)side_set_node_list_len = list_len;
  }
  else {
    *(int *)side_set_node_list_len = list_len;
  }

  if (num_df > 0 && num_df != tot_num_ss_elem) {
    if (list_len != num_df) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "Warning: In side set %" PRId64 " the distribution factor count (%" PRId64
               ") does not match the side set node list length (%zu). These should match and this "
               "may indicate a corrupt database in file %d",
               side_set_id, num_df, list_len, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MSG);
      err_stat = EX_WARN;
      goto cleanup;
    }
  }

/* All done: release element block ids array,
   element block parameters array, and side set element index array */
cleanup:
  free(elem_blk_ids);
  free(elem_blk_parms);
  free(ss_elem_ndx);
  free(ss_elem_ndx_64);
  free(side_set_side_list);
  free(side_set_elem_list);

  EX_FUNC_LEAVE(err_stat);
}

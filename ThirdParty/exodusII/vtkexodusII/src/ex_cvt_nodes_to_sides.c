/*
 * Copyright(C) 1999-2020, 2023 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_err, ex_block, etc
#include "exodusII_int.h" // for elem_blk_parm, EX_FATAL, etc
#include <assert.h>
#include <stdbool.h>

static int64_t get_node(void_int *connect, size_t index, size_t int_size)
{
  if (int_size == sizeof(int64_t)) {
    return ((int64_t *)connect)[index];
  }
  return ((int *)connect)[index];
}

static void put_side(void_int *side_list, size_t index, size_t value, size_t int_size)
{
  if (int_size == sizeof(int64_t)) {
    ((int64_t *)side_list)[index] = value;
  }
  else {
    ((int *)side_list)[index] = value;
  }
}

/*!
\ingroup Utilities
The function ex_cvt_nodes_to_sides() is used to convert a side set
node list to a side set side list. This routine is provided for
application programs that utilize side sets defined by nodes (as was
done previous to release 2.0) rather than local faces or edges. The
application program must allocate memory for the returned array of
sides. The length of this array is the same as the length of the
concatenated side sets element list, which can be determined with a
call to ex_inquire() or ex_inquire_int().

\return In case of an error, ex_cvt_nodes_to_sides() returns a
negative number; a warning will return a positive number. Possible
causes of errors include:
  -  a warning value is returned if no side sets are stored in the file.
  -  because the faces of a wedge require a different number of
     nodes to describe them (quadrilateral vs. triangular faces), the
     function will abort with a fatal return code if a wedge is
     encountered in the side set element list.

\param[in] exoid                  exodus file ID returned from a previous call
to ex_create()
                                  or ex_open().

\param[in]  num_elem_per_set      Array containing the number of sides for each
set. The number
                                  of sides is equal to the number of elements
for each set.

\param[in]  num_nodes_per_set     Array containing the number of nodes for each
set.

\param[in]  side_sets_elem_index  Array containing indices into the \c
side_sets_elem_list which are
                                  the locations of the first element for each
set. These indices are
                                  0-based.  Unused.

\param[in]  side_sets_node_index  Array containing indices into the \c
side_sets_node_list
                                  which are the locations of the first node for
each set. These
                                  indices are 0-based. Unused.

\param[in]  side_sets_elem_list   Array containing the elements for all side
sets. Internal element IDs
                                  are used in this list (see Section
LocalElementIds).

\param[in]  side_sets_node_list   Array containing the nodes for all side sets.
Internal node
                                  IDs are used in this list (see  Section
LocalNodeIds).

\param[out]  side_sets_side_list  Returned array containing the sides for all
side sets.

The following code segment will convert side sets described
by nodes to side sets described by local side numbers:

~~~{.c}
int error, exoid, ids[2], num_side_per_set[2],
    num_nodes_per_set[2], elem_ind[2], node_ind[2],
    elem_list[4], node_list[8], el_lst_len, *side_list;

ids[0] = 30             ; ids[1]  = 31;
num_side_per_set[0]  = 2; num_side_per_set[1] = 2;
num_nodes_per_set[0] = 4; num_nodes_per_set[1] = 4;

elem_ind[0] = 0; elem_ind[1] = 2;
node_ind[0] = 0; node_ind[1] = 4;

\comment{side set #1}
elem_list[0] = 2; elem_list[1] = 2;
node_list[0] = 8; node_list[1] = 5;
node_list[2] = 6; node_list[3] = 7;

\comment{side set #2}
elem_list[2] = 1; elem_list[3] = 2;
node_list[4] = 2; node_list[5] = 3;
node_list[6] = 7; node_list[7] = 8;

el_lst_len = ex_inquire_int(exoid, EX_INQ_SS_ELEM_LEN);

\comment{side set element list is same length as side list}
side_list = (int *) calloc (el_lst_len, sizeof(int));

ex_cvt_nodes_to_sides(exoid, num_side_per_set, num_nodes_per_set,
                      elem_ind, node_ind, elem_list,
                      node_list, side_list);
~~~

 <b>Algorithm:</b>

\verbatim
  Read elem_block_ids --> elem_blk_id[array]

  Read element block parameters --> elem_blk_parms[array]

  Determine total number of elements in side set by summing num_elem_per_set

  Build side set element to side set node list index --> ss_elem_node_ndx[array]

  For each element in the side_set_elem_list  {
    If Jth element is not in current element block (e.g. J>elem_ctr) {
      get element block parameters (num_elem_in_blk, ...)
      elem_ctr += num_elem_in_blk

      free old connectity array space
      allocate connectivity array: size=num_elem_in_blk*num_nodes_per_elem
      get connectivity array
    }

    If Jth element is in current element block (e.g. J<=elem_ctr) {
      For each node in element (linear search of up to num_nodes_per_elem) {
        If side set element node[1] == element node[i] {
          Case element type = Hex {
            If side set element node[2] == element node[Hex_table[i,1]]
              Jth side = Hex_table[i,2]

             break
          }
          Case element type = Wedge {
            If side set element node[2] == element node[Wedge_table[i,1]]
              Jth side = Wedge_table[i,2]

            break
          }
        }
      }
    }
  }
\endverbatim

 */

int ex_cvt_nodes_to_sides(int exoid, void_int *num_elem_per_set, void_int *num_nodes_per_set,
                          void_int *side_sets_elem_index, /* unused */
                          void_int *side_sets_node_index, /* unused */
                          void_int *side_sets_elem_list, void_int *side_sets_node_list,
                          void_int *side_sets_side_list)
{
  size_t    i, j, k, n;
  int       num_side_sets, num_elem_blks;
  int64_t   tot_num_elem = 0, tot_num_ss_elem = 0, elem_num = 0, ndim;
  void_int *elem_blk_ids     = NULL;
  void_int *connect          = NULL;
  void_int *ss_elem_ndx      = NULL;
  void_int *ss_elem_node_ndx = NULL;
  void_int *ss_parm_ndx      = NULL;
  size_t    elem_ctr, node_ctr, elem_num_pos;
  int       num_nodes_per_elem, num_node_per_side;

  int *same_elem_type = NULL;
  int  el_type        = 0;

  int int_size;
  int ids_size;

  struct exi_elem_blk_parm *elem_blk_parms = NULL;

  int err_stat = EX_NOERR;

  /* node to side translation tables -
     These tables are used to look up the side number based on the
     first and second node in the side/face list. The side node order
     is found in the original Exodus document, SAND87-2997. The element
     node order is found in the ExodusII document, SAND92-2137. These
     tables were generated by following the right-hand rule for determining
     the outward normal. Note: Only the more complex 3-D shapes require
     these tables, the simple shapes are trivial - the first node found
     is also the side number.
  */

  char errmsg[MAX_ERR_LENGTH];

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_UNUSED(side_sets_elem_index);
  EX_UNUSED(side_sets_node_index);

  /* first check if any side sets are specified */
  /* inquire how many side sets have been stored */

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

  num_elem_blks = ex_inquire_int(exoid, EX_INQ_ELEM_BLK);
  if (num_elem_blks < 0) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of element blocks in file id %d",
             exoid);
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

  /* get the dimensionality of the coordinates;  this is necessary to
     distinguish between 2d TRIs and 3d TRIs */
  ndim = ex_inquire_int(exoid, EX_INQ_DIM);

  bool ints_64 = ex_int64_status(exoid) & EX_BULK_INT64_API;
  int_size     = ints_64 ? sizeof(int64_t) : sizeof(int);

  /* First count up # of elements in the side sets*/
  if (ints_64) {
    for (i = 0; i < num_side_sets; i++) {
      tot_num_ss_elem += ((int64_t *)num_elem_per_set)[i];
    }
  }
  else {
    for (i = 0; i < num_side_sets; i++) {
      tot_num_ss_elem += ((int *)num_elem_per_set)[i];
    }
  }

  /* Allocate space for the ss element index array */
  if (!(ss_elem_ndx = malloc(tot_num_ss_elem * int_size))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for side set elem sort "
             "array for file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    err_stat = EX_FATAL;
    goto cleanup;
  }

  if (int_size == sizeof(int64_t)) {
    /* Sort side set element list into index array  - non-destructive */
    int64_t *elems = (int64_t *)ss_elem_ndx;
    for (i = 0; i < tot_num_ss_elem; i++) {
      elems[i] = i; /* init index array to current position */
    }
    exi_iqsort64(side_sets_elem_list, elems, tot_num_ss_elem);
  }
  else {
    /* Sort side set element list into index array  - non-destructive */
    int *elems = (int *)ss_elem_ndx;
    for (i = 0; i < tot_num_ss_elem; i++) {
      elems[i] = i; /* init index array to current position */
    }
    exi_iqsort(side_sets_elem_list, elems, tot_num_ss_elem);
  }

  /* Allocate space for the element block ids */
  ids_size = sizeof(int);
  if (ints_64) {
    ids_size = sizeof(int64_t);
  }

  if (!(elem_blk_ids = malloc(num_elem_blks * ids_size))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for element block ids for file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    err_stat = EX_FATAL;
    goto cleanup;
  }

  if (ex_get_ids(exoid, EX_ELEM_BLOCK, elem_blk_ids)) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get element block ids in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MSG);
    err_stat = EX_FATAL;
    goto cleanup;
  }

  /* Allocate space for the element block params */
  if (!(elem_blk_parms = malloc(num_elem_blks * sizeof(struct exi_elem_blk_parm)))) {
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
    if (ints_64) {
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

  /* Allocate space for the ss element to element block parameter index array */
  if (!(ss_parm_ndx = malloc(tot_num_ss_elem * int_size))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for side set elem params "
             "index for file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    err_stat = EX_FATAL;
    goto cleanup;
  }

  /* Allocate space for the ss element to node list index array */
  if (!(ss_elem_node_ndx = malloc((tot_num_ss_elem + 1) * int_size))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for side set elem to node "
             "index for file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    err_stat = EX_FATAL;
    goto cleanup;
  }

  /* determine if each side set has uniform element types; this will
     be used to help determine the stride through the node list
  */

  /* Allocate space for same element type flag array*/
  if (!(same_elem_type = calloc(num_side_sets, sizeof(int)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for element type flag "
             "array for file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    err_stat = EX_FATAL;
    goto cleanup;
  }

  same_elem_type[0] = true;
  if (ints_64) {
    elem_ctr = ((int64_t *)num_elem_per_set)[0];
    for (i = 0, k = 0; i < tot_num_ss_elem; i++) {
      int64_t elem = ((int64_t *)side_sets_elem_list)[i];
      for (j = 0; j < num_elem_blks; j++) {
        if (elem <= elem_blk_parms[j].elem_ctr) {
          break;
        }
      }

      if (j >= num_elem_blks) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: internal logic error for file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
        err_stat = EX_FATAL;
        goto cleanup;
      }

      if (i == 0) {
        el_type = elem_blk_parms[j].elem_type_val;
      }

      /* determine which side set this element is in; assign to kth side set */
      if (i >= elem_ctr) {
        elem_ctr += ((int64_t *)num_elem_per_set)[++k];

        el_type           = elem_blk_parms[j].elem_type_val;
        same_elem_type[k] = true;
      }

      if (el_type != elem_blk_parms[j].elem_type_val) {
        same_elem_type[k] = false;
      }
    }

    /* Build side set element to node list index and side set element
       parameter index.
    */
    node_ctr = 0;
    elem_ctr = ((int64_t *)num_elem_per_set)[0];
    for (i = 0, k = 0; i < tot_num_ss_elem; i++) {
      int64_t elem = ((int64_t *)side_sets_elem_list)[i];

      for (j = 0; j < num_elem_blks; j++) {
        if (elem <= elem_blk_parms[j].elem_ctr) {
          break;
        }
      }
      if (j >= num_elem_blks) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: internal logic error for file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
        err_stat = EX_FATAL;
        goto cleanup;
      }

      ((int64_t *)ss_parm_ndx)[i]      = j;        /* assign parameter block index */
      ((int64_t *)ss_elem_node_ndx)[i] = node_ctr; /* assign node list index */

      /* determine which side set this element is in; assign to kth side set */
      if (i >= elem_ctr) {
        /* skip over NULL side sets */
        while (((int64_t *)num_elem_per_set)[++k] == 0) {
          ;
        }
        elem_ctr += ((int64_t *)num_elem_per_set)[k];
      }

      /* determine number of nodes per side */
      if (((((int64_t *)num_nodes_per_set)[k] % ((int64_t *)num_elem_per_set)[k]) == 0) &&
          (same_elem_type[k] == true)) { /* all side set elements are same type */
        node_ctr += ((int64_t *)num_nodes_per_set)[k] / ((int64_t *)num_elem_per_set)[k];
      }
      else {
        node_ctr += elem_blk_parms[j].num_nodes_per_side[0];
      }
    }
    ((int64_t *)ss_elem_node_ndx)[i] = node_ctr; /* assign node list index */
  }
  else {
    elem_ctr = ((int *)num_elem_per_set)[0];
    for (i = 0, k = 0; i < tot_num_ss_elem; i++) {
      int elem = ((int *)side_sets_elem_list)[i];

      for (j = 0; j < num_elem_blks; j++) {
        if (elem <= elem_blk_parms[j].elem_ctr) {
          break;
        }
      }

      if (j >= num_elem_blks) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: internal logic error for file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
        err_stat = EX_FATAL;
        goto cleanup;
      }

      if (i == 0) {
        el_type = elem_blk_parms[j].elem_type_val;
      }

      /* determine which side set this element is in; assign to kth side set */
      if (i >= elem_ctr) {
        elem_ctr += ((int *)num_elem_per_set)[++k];

        el_type           = elem_blk_parms[j].elem_type_val;
        same_elem_type[k] = true;
      }

      if (el_type != elem_blk_parms[j].elem_type_val) {
        same_elem_type[k] = false;
      }
    }

    /* Build side set element to node list index and side set element
       parameter index.
    */
    node_ctr = 0;
    elem_ctr = ((int *)num_elem_per_set)[0];
    for (i = 0, k = 0; i < tot_num_ss_elem; i++) {
      int elem = ((int *)side_sets_elem_list)[i];

      for (j = 0; j < num_elem_blks; j++) {
        if (elem <= elem_blk_parms[j].elem_ctr) {
          break;
        }
      }
      if (j >= num_elem_blks) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: internal logic error for file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_INTERNAL);
        err_stat = EX_FATAL;
        goto cleanup;
      }

      ((int *)ss_parm_ndx)[i]      = j;        /* assign parameter block index */
      ((int *)ss_elem_node_ndx)[i] = node_ctr; /* assign node list index */

      /* determine which side set this element is in; assign to kth side set */
      if (i >= elem_ctr) {
        /* skip over NULL side sets */
        while (((int *)num_elem_per_set)[++k] == 0) {
          ;
        }
        elem_ctr += ((int *)num_elem_per_set)[k];
      }

      /* determine number of nodes per side */
      if (((((int *)num_nodes_per_set)[k] % ((int *)num_elem_per_set)[k]) == 0) &&
          (same_elem_type[k])) { /* all side set elements are same type */
        node_ctr += ((int *)num_nodes_per_set)[k] / ((int *)num_elem_per_set)[k];
      }
      else {
        node_ctr += elem_blk_parms[j].num_nodes_per_side[0];
      }
    }
    ((int *)ss_elem_node_ndx)[i] = node_ctr; /* assign node list index */
  }

  /* All setup, ready to go ... */

  elem_ctr = 0;

  for (j = 0; j < tot_num_ss_elem; j++) {
    int64_t elem;
    int64_t idx;
    int64_t ss_node0, ss_node1;
    int64_t p_ndx;
    if (int_size == sizeof(int64_t)) {
      idx      = ((int64_t *)ss_elem_ndx)[j];
      elem     = ((int64_t *)side_sets_elem_list)[idx];
      ss_node0 = ((int64_t *)side_sets_node_list)[((int64_t *)ss_elem_node_ndx)[idx]];
      ss_node1 = ((int64_t *)side_sets_node_list)[((int64_t *)ss_elem_node_ndx)[idx] + 1];
      p_ndx    = ((int64_t *)ss_parm_ndx)[idx];
    }
    else {
      idx      = ((int *)ss_elem_ndx)[j];
      elem     = ((int *)side_sets_elem_list)[idx];
      ss_node0 = ((int *)side_sets_node_list)[((int *)ss_elem_node_ndx)[idx]];
      ss_node1 = ((int *)side_sets_node_list)[((int *)ss_elem_node_ndx)[idx] + 1];
      p_ndx    = ((int *)ss_parm_ndx)[idx];
    }
    elem_num = elem - 1;

    if (elem > elem_ctr) {
      /* release connectivity array space and get next one */
      if (elem_ctr > 0) {
        free(connect);
      }

      /* Allocate space for the connectivity array for new element block */
      if (!(connect = malloc(elem_blk_parms[p_ndx].num_elem_in_blk *
                             elem_blk_parms[p_ndx].num_nodes_per_elem * int_size))) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to allocate space for connectivity "
                 "array for file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
        err_stat = EX_FATAL;
        goto cleanup;
      }

      /* get connectivity array */
      if (ex_get_conn(exoid, EX_ELEM_BLOCK, elem_blk_parms[p_ndx].elem_blk_id, connect, NULL,
                      NULL) == EX_FATAL) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get connectivity array for elem blk %" PRId64 " for file id %d",
                 elem_blk_parms[p_ndx].elem_blk_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
        err_stat = EX_FATAL;
        goto cleanup;
      }
      elem_ctr = elem_blk_parms[p_ndx].elem_ctr;
    }
    /*  For the first node of each side in side set, using a linear search
        (of up to num_nodes_per_elem) of the connectivity array,
        locate the node position in the element. The first node position
        and the second node position are used with a element type specific
        table to determine the side. */

    if (connect == NULL) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: logic error. Connect pointer is null for elem blk %" PRId64
               " for file id %d",
               elem_blk_parms[p_ndx].elem_blk_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
      err_stat = EX_FATAL;
      goto cleanup;
    }

    /* calculate the relative element number position in it's block*/
    elem_num_pos =
        elem_num - (elem_blk_parms[p_ndx].elem_ctr - elem_blk_parms[p_ndx].num_elem_in_blk);
    /* calculate the beginning of the node list for this element by
       using the ss_elem_node_ndx index into the side_sets_node_index
       and adding the element number position * number of nodes per elem */

    num_nodes_per_elem = elem_blk_parms[p_ndx].num_nodes_per_elem;

    for (n = 0; n < num_nodes_per_elem; n++) {
      /* find node in connectivity array that matches first node in side set */
      if (((int_size == sizeof(int64_t)) &&
           (ss_node0 == ((int64_t *)connect)[num_nodes_per_elem * (elem_num_pos) + n])) ||
          ((int_size == sizeof(int)) &&
           (ss_node0 == ((int *)connect)[num_nodes_per_elem * (elem_num_pos) + n]))) {
        switch (elem_blk_parms[p_ndx].elem_type_val) {
        case EX_EL_CIRCLE:
        case EX_EL_SPHERE: {
          /* simple case: 1st node number is same as side # */
          put_side(side_sets_side_list, idx, n + 1, int_size);
          break;
        }
        case EX_EL_QUAD:
        case EX_EL_TRIANGLE:
        case EX_EL_TRUSS:
        case EX_EL_BEAM: {
          /* simple case: 1st node number is same as side # */
          put_side(side_sets_side_list, idx, n + 1, int_size);
          break;
        }
        case EX_EL_TRISHELL: {
          /* use table to find which node to compare to next */
          /*   1     2     3                                               node 1 */
          static const int trishell_table[2][6] = {
              {2, 3, 3, 1, 1, 2}, /* node 2 */
              {1, 2, 1, 2, 1, 2}  /* side # */
          };

          assert(n < 3);
          if (ss_node1 ==
              get_node(connect,
                       num_nodes_per_elem * (elem_num_pos) + (trishell_table[0][2 * n] - 1),
                       int_size)) {
            /* Assume only front or back, no edges... */
            put_side(side_sets_side_list, idx, trishell_table[1][2 * n], int_size);
          }
          else if (ss_node1 == get_node(connect,
                                        num_nodes_per_elem * (elem_num_pos) +
                                            (trishell_table[0][2 * n + 1] - 1),
                                        int_size)) {
            /* Assume only front or back, no edges... */
            put_side(side_sets_side_list, idx, trishell_table[1][2 * n + 1], int_size);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to find TRIANGULAR SHELL element %" PRId64 ", node %" PRId64
                     " in connectivity array %" PRId64 " for file id %d",
                     elem_num + 1, ss_node1, elem_blk_parms[p_ndx].elem_blk_id, exoid);
            ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
            err_stat = EX_FATAL;
            goto cleanup;
          }
          break;
        }
        case EX_EL_SHELL: {
          /* use table to find which node to compare to next */
          assert(n < 4);

          if (ints_64) {
            num_node_per_side =
                ((int64_t *)ss_elem_node_ndx)[idx + 1] - ((int64_t *)ss_elem_node_ndx)[idx];
          }
          else {
            num_node_per_side = ((int *)ss_elem_node_ndx)[idx + 1] - ((int *)ss_elem_node_ndx)[idx];
          }

          if (num_node_per_side >= 4) {
            /*   1     2     3     4                                          node 1 */
            static const int shell_table[2][8] = {
                {2, 4, 3, 1, 4, 2, 1, 3}, /* node 2 */
                {1, 2, 1, 2, 1, 2, 1, 2}  /* side # */
            };

            /* Front or Back side of shell */
            if (ss_node1 ==
                get_node(connect, num_nodes_per_elem * (elem_num_pos) + (shell_table[0][2 * n] - 1),
                         int_size)) {
              /* 4- or 8-node side (front or back face) */
              put_side(side_sets_side_list, idx, shell_table[1][2 * n], int_size);
            }
            else if (ss_node1 ==
                     get_node(connect,
                              num_nodes_per_elem * (elem_num_pos) + (shell_table[0][2 * n + 1] - 1),
                              int_size)) {
              /* 4- or 8-node side (front or back face) */
              put_side(side_sets_side_list, idx, shell_table[1][2 * n + 1], int_size);
            }
            else {
              snprintf(errmsg, MAX_ERR_LENGTH,
                       "ERROR: failed to find SHELL element %" PRId64 ", node %" PRId64
                       " in connectivity array %" PRId64 " for file id %d",
                       elem_num + 1, ss_node1, elem_blk_parms[p_ndx].elem_blk_id, exoid);
              ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
              err_stat = EX_FATAL;
              goto cleanup;
            }
          }
          else {
            /* Edge sides of shell */
            /*    1     2   3    4                                          node 1 */
            static const int shell_edge_table[2][8] = {
                {2, 4, 3, 1, 4, 2, 1, 3}, /* node 2 */
                {3, 6, 4, 3, 5, 4, 6, 5}  /* side # */
            };

            if (ss_node1 ==
                get_node(connect,
                         num_nodes_per_elem * (elem_num_pos) + (shell_edge_table[0][2 * n] - 1),
                         int_size)) {
              /* 2- or 3-node side (edge of shell) */
              put_side(side_sets_side_list, idx, shell_edge_table[1][2 * n], int_size);
            }
            else if (ss_node1 == get_node(connect,
                                          num_nodes_per_elem * (elem_num_pos) +
                                              (shell_edge_table[0][2 * n + 1] - 1),
                                          int_size)) {
              /* 2- or 3-node side (edge of shell) */
              put_side(side_sets_side_list, idx, shell_edge_table[1][2 * n + 1], int_size);
            }
            else {
              snprintf(errmsg, MAX_ERR_LENGTH,
                       "ERROR: failed to find SHELL element %" PRId64 ", node %" PRId64
                       " in connectivity array %" PRId64 " for file id %d",
                       elem_num + 1, ss_node1, elem_blk_parms[p_ndx].elem_blk_id, exoid);
              ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
              err_stat = EX_FATAL;
              goto cleanup;
            }
          }
          break;
        }
        case EX_EL_HEX: {
          /* use table to find which node to compare to next */
          static const int hex_table[2][24] = {
              /*     1        2        3        4        5        6        7        8       node 1
               */
              {4, 2, 5, 1, 3, 6, 7, 4, 2, 3, 1, 8, 6, 8, 1, 5, 2, 7, 8, 6, 3, 7, 5, 4}, /* node 2 */
              {5, 1, 4, 5, 2, 1, 2, 3, 5, 5, 4, 3, 6, 4, 1, 1, 2, 6, 6, 2, 3, 3, 6, 4}  /* side # */
          };

          assert(n < 8);

          if (ss_node1 == get_node(connect,
                                   num_nodes_per_elem * (elem_num_pos) + (hex_table[0][3 * n] - 1),
                                   int_size)) {
            put_side(side_sets_side_list, idx, hex_table[1][3 * n], int_size);
          }
          else if (ss_node1 ==
                   get_node(connect,
                            num_nodes_per_elem * (elem_num_pos) + (hex_table[0][3 * n + 1] - 1),
                            int_size)) {
            put_side(side_sets_side_list, idx, hex_table[1][3 * n + 1], int_size);
          }
          else if (ss_node1 ==
                   get_node(connect,
                            num_nodes_per_elem * (elem_num_pos) + (hex_table[0][3 * n + 2] - 1),
                            int_size)) {
            put_side(side_sets_side_list, idx, hex_table[1][3 * n + 2], int_size);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to find HEX element %" PRId64 ", node %" PRId64
                     " in connectivity array %" PRId64 " for file id %d",
                     elem_num + 1, ss_node1, elem_blk_parms[p_ndx].elem_blk_id, exoid);
            ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
            err_stat = EX_FATAL;
            goto cleanup;
          }
          break;
        }
        case EX_EL_TETRA: {
          /* use table to find which node to compare to next */
          /*  1         2          3          4            node 1 */
          static const int tetra_table[2][12] = {
              {2, 3, 4, 1, 3, 4, 4, 1, 2, 1, 2, 3}, /* node 2 */
              {1, 4, 3, 4, 2, 1, 2, 3, 4, 1, 2, 3}  /* side # */
          };

          assert(n < 4);

          if (ss_node1 ==
              get_node(connect, num_nodes_per_elem * (elem_num_pos) + (tetra_table[0][3 * n] - 1),
                       int_size)) {
            put_side(side_sets_side_list, idx, tetra_table[1][3 * n], int_size);
          }
          else if (ss_node1 ==
                   get_node(connect,
                            num_nodes_per_elem * (elem_num_pos) + (tetra_table[0][3 * n + 1] - 1),
                            int_size)) {
            put_side(side_sets_side_list, idx, tetra_table[1][3 * n + 1], int_size);
          }
          else if (ss_node1 ==
                   get_node(connect,
                            num_nodes_per_elem * (elem_num_pos) + (tetra_table[0][3 * n + 2] - 1),
                            int_size)) {
            put_side(side_sets_side_list, idx, tetra_table[1][3 * n + 2], int_size);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to find TETRA element %" PRId64 ", node %" PRId64
                     " in connectivity array %" PRId64 " for file id %d",
                     elem_num + 1, ss_node1, elem_blk_parms[p_ndx].elem_blk_id, exoid);
            ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
            err_stat = EX_FATAL;
            goto cleanup;
          }
          break;
        }
        case EX_EL_PYRAMID: {
          /* NOTE: PYRAMID elements in side set node lists are currently not
           * supported */
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: unsupported PYRAMID element found in side "
                   "set node list in file id %d",
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
          err_stat = EX_FATAL;
          goto cleanup;
        }
        case EX_EL_WEDGE: {
          /* NOTE: WEDGE elements in side set node lists are currently not
           * supported */
#if 0
	  static const int wedge_table[2][18]  = {
	    /*     1      2      3      4      5      6                     node 1 */
	    {2,4,3, 5,1,3, 6,1,2, 1,6,5, 6,2,4, 4,3,5},              /* node 2 */
	    {1,3,4, 1,4,2, 2,3,4, 1,3,5, 5,2,1, 5,3,2}               /* side # */
	  };
#endif

          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: unsupported WEDGE element found in side set "
                   "node list in file id %d",
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
          err_stat = EX_FATAL;
          goto cleanup;
        }
        default: {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s is an unsupported element type",
                   elem_blk_parms[p_ndx].elem_type);
          ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
          err_stat = EX_FATAL;
          goto cleanup;
        }
        }
        break; /* done with this element */
      }
    }
    if (n >= num_nodes_per_elem) /* did we find the node? */
    {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find element %" PRId64 ", node %" PRId64
               " in element block %" PRId64 " for file id %d",
               elem_num + 1, ss_node0, elem_blk_parms[p_ndx].elem_blk_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      err_stat = EX_FATAL;
      goto cleanup;
    }
  }

/* All done: release connectivity array space, element block ids array,
   element block parameters array, and side set element index array */
cleanup:
  free(connect);
  free(ss_elem_node_ndx);
  free(ss_parm_ndx);
  free(elem_blk_parms);
  free(elem_blk_ids);
  free(ss_elem_ndx);
  free(same_elem_type);

  EX_FUNC_LEAVE(err_stat);
}

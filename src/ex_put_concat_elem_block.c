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
 * expclb - ex_put_concat_elem_block: write element block parameters
 *
 * entry conditions -
 *   input parameters:
 *       int     idexo                   exodus file id
 *       char**  elem_type               element type string
 *       int*    num_elem_this_blk       number of elements in the element blk
 *       int*    num_nodes_per_elem      number of nodes per element block
 *       int*    num_attr_this_blk       number of attributes
 *       int     define_maps             if != 0, write maps, else don't
 *
 *****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * writes the parameters used to describe an element block
 * \param    exoid                   exodus file id
 * \param    elem_blk_id             element block id
 * \param    elem_type               element type string
 * \param    num_elem_this_blk       number of elements in the element blk
 * \param    num_nodes_per_elem      number of nodes per element block
 * \param    num_attr_this_blk       number of attributes
 * \param    define_maps             if != 0, write maps, else don't
 */
int ex_put_concat_elem_block(int exoid, const void_int *elem_blk_id, char *elem_type[],
                             const void_int *num_elem_this_blk, const void_int *num_nodes_per_elem,
                             const void_int *num_attr_this_blk, int define_maps)
{
  int    i, varid, dimid, dims[2], strdim, *eb_array;
  int    temp;
  int    iblk;
  int    status;
  int    num_elem_blk;
  int    map_int_type, conn_int_type;
  size_t length;
  int    cur_num_elem_blk, nelnoddim, numelbdim, numattrdim, connid, numelemdim, numnodedim;
  char   errmsg[MAX_ERR_LENGTH];
#if NC_HAS_HDF5
  int fill = NC_FILL_CHAR;
#endif

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* first check if any element blocks are specified
   * OK if zero...
   */
  if (nc_inq_dimid(exoid, DIM_NUM_EL_BLK, &dimid) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_NOERR);
  }

  /* Get number of element blocks defined for this file */
  if ((status = nc_inq_dimlen(exoid, dimid, &length)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of element blocks in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }
  num_elem_blk = length;

  /* Fill out the element block status array */
  if (!(eb_array = malloc(num_elem_blk * sizeof(int)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate space for element block status "
             "array in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    for (i = 0; i < num_elem_blk; i++) {
      eb_array[i] = (((int64_t *)num_elem_this_blk)[i] == 0) ? 0 : 1;
    }
  }
  else {
    for (i = 0; i < num_elem_blk; i++) {
      eb_array[i] = (((int *)num_elem_this_blk)[i] == 0) ? 0 : 1;
    }
  }

  /* Next, get variable id of status array */
  if ((status = nc_inq_varid(exoid, VAR_STAT_EL_BLK, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate element block status in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(eb_array);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  status = nc_put_var_int(exoid, varid, eb_array);

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store element block status array to file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(eb_array);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Next, fill out ids array */
  /* first get id of ids array variable */
  if ((status = nc_inq_varid(exoid, VAR_ID_EL_BLK, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate element block ids array in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(eb_array);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* then, write out id list */
  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    status = nc_put_var_longlong(exoid, varid, elem_blk_id);
  }
  else {
    status = nc_put_var_int(exoid, varid, elem_blk_id);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store element block id array in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(eb_array);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire previously defined dimensions  */
  if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &strdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get string length in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(eb_array);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* put netcdf file into define mode  */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to place file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(eb_array);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  conn_int_type = NC_INT;
  if (ex_int64_status(exoid) & EX_BULK_INT64_DB) {
    conn_int_type = NC_INT64;
  }

  map_int_type = NC_INT;
  if (ex_int64_status(exoid) & EX_MAPS_INT64_DB) {
    map_int_type = NC_INT64;
  }

  /* Iterate over element blocks ... */
  for (iblk = 0; iblk < num_elem_blk; iblk++) {
    ex_entity_id eb_id;
    size_t       num_elem;
    size_t       num_npe;
    size_t       num_attr;
    if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
      eb_id    = ((int64_t *)elem_blk_id)[iblk];
      num_elem = ((int64_t *)num_elem_this_blk)[iblk];
      num_npe  = ((int64_t *)num_nodes_per_elem)[iblk];
      num_attr = ((int64_t *)num_attr_this_blk)[iblk];
    }
    else {
      eb_id    = ((int *)elem_blk_id)[iblk];
      num_elem = ((int *)num_elem_this_blk)[iblk];
      num_npe  = ((int *)num_nodes_per_elem)[iblk];
      num_attr = ((int *)num_attr_this_blk)[iblk];
    }

    cur_num_elem_blk = ex__get_file_item(exoid, ex__get_counter_list(EX_ELEM_BLOCK));
    if (cur_num_elem_blk >= num_elem_blk) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: exceeded number of element blocks (%d) defined in file id %d", num_elem_blk,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      goto error_ret;
    }

    /* NOTE: ex__inc_file_item  is used to find the number of element blocks
       for a specific file and returns that value incremented. */
    cur_num_elem_blk = ex__inc_file_item(exoid, ex__get_counter_list(EX_ELEM_BLOCK));

    if (eb_array[iblk] == 0) { /* Is this a NULL element block? */
      continue;
    }

    /* define some dimensions and variables*/
    if ((status = nc_def_dim(exoid, DIM_NUM_EL_IN_BLK(cur_num_elem_blk + 1), num_elem,
                             &numelbdim)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) { /* duplicate entry */
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: element block %" PRId64 " already defined in file id %d", eb_id, exoid);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of elements/block for "
                 "block %" PRId64 " file id %d",
                 eb_id, exoid);
      }
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    if ((status = nc_def_dim(exoid, DIM_NUM_NOD_PER_EL(cur_num_elem_blk + 1), num_npe,
                             &nelnoddim)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to define number of nodes/element for block %" PRId64
               " in file id %d",
               eb_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    /* element connectivity array */
    dims[0] = numelbdim;
    dims[1] = nelnoddim;

    if ((status = nc_def_var(exoid, VAR_CONN(cur_num_elem_blk + 1), conn_int_type, 2, dims,
                             &connid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to create connectivity array for block %" PRId64 " in file id %d",
               eb_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }
    ex__compress_variable(exoid, connid, 1);

    /* store element type as attribute of connectivity variable */
    if ((status = nc_put_att_text(exoid, connid, ATT_NAME_ELB, strlen(elem_type[iblk]) + 1,
                                  (void *)elem_type[iblk])) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store element type name %s in file id %d",
               elem_type[iblk], exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      goto error_ret; /* exit define mode and return */
    }

    /* element attribute array */
    if (num_attr > 0) {
      if ((status = nc_def_dim(exoid, DIM_NUM_ATT_IN_BLK(cur_num_elem_blk + 1), num_attr,
                               &numattrdim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of attributes in block %" PRId64 " in file id %d",
                 eb_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /* Attribute names... */
      dims[0] = numattrdim;
      dims[1] = strdim;

      if ((status = nc_def_var(exoid, VAR_NAME_ATTRIB(cur_num_elem_blk + 1), NC_CHAR, 2, dims,
                               &temp)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define element attribute name array "
                 "in file id %d",
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
#if NC_HAS_HDF5
      nc_def_var_fill(exoid, temp, 0, &fill);
#endif
      eb_array[iblk] = temp;

      dims[0] = numelbdim;
      dims[1] = numattrdim;

      if ((status = nc_def_var(exoid, VAR_ATTRIB(cur_num_elem_blk + 1), nc_flt_code(exoid), 2, dims,
                               &temp)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR:  failed to define attributes for element block %" PRId64 " in file id %d",
                 eb_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }
  }

  /* Define the element map here to avoid a later redefine call */
  if (define_maps != 0) {
    if (nc_inq_varid(exoid, VAR_ELEM_NUM_MAP, &temp) != NC_NOERR) {
      /* Map does not exist */
      /* Possible to have zero elements but >0 element blocks.
       * Only define map if there are nonzero elements
       */
      if (nc_inq_dimid(exoid, DIM_NUM_ELEM, &numelemdim) == NC_NOERR) {
        dims[0] = numelemdim;

        if ((status = nc_def_var(exoid, VAR_ELEM_NUM_MAP, map_int_type, 1, dims, &temp)) !=
            NC_NOERR) {
          if (status == NC_ENAMEINUSE) {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: element numbering map already exists in file id %d", exoid);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to create element numbering map in file id %d", exoid);
          }
          ex_err_fn(exoid, __func__, errmsg, status);
          goto error_ret; /* exit define mode and return */
        }
        ex__compress_variable(exoid, temp, 1);
      }
    }

    /* Do the same for the node numbering map */
    if (nc_inq_varid(exoid, VAR_NODE_NUM_MAP, &temp) != NC_NOERR) {
      /* Map does not exist */
      if ((nc_inq_dimid(exoid, DIM_NUM_NODES, &numnodedim)) == NC_NOERR) {
        dims[0] = numnodedim;
        if ((status = nc_def_var(exoid, VAR_NODE_NUM_MAP, map_int_type, 1, dims, &temp)) !=
            NC_NOERR) {
          if (status == NC_ENAMEINUSE) {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: node numbering map already exists in file id %d", exoid);
          }
          else {
            snprintf(errmsg, MAX_ERR_LENGTH,
                     "ERROR: failed to create node numbering map array "
                     "in file id %d",
                     exoid);
          }
          ex_err_fn(exoid, __func__, errmsg, status);
          goto error_ret; /* exit define mode and return */
        }
        ex__compress_variable(exoid, temp, 1);
      }
    }
  }

  /* leave define mode  */
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    free(eb_array);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  {
    /* Write dummy attribute name. Without this we get corruption in the
     * attribute name.
     */
    size_t start[2], count[2];
    char * text = "";
    count[0]    = 1;
    start[1]    = 0;
    count[1]    = strlen(text) + 1;

    for (iblk = 0; iblk < num_elem_blk; iblk++) {
      size_t num_attr;
      if (eb_array[iblk] == 0) { /* Is this a NULL element block? */
        continue;
      }
      if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
        num_attr = ((int64_t *)num_attr_this_blk)[iblk];
      }
      else {
        num_attr = ((int *)num_attr_this_blk)[iblk];
      }
      for (i = 0; i < num_attr; i++) {
        start[0] = i;
        nc_put_vara_text(exoid, eb_array[iblk], start, count, text);
      }
    }
  }
  free(eb_array);

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  free(eb_array);
  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

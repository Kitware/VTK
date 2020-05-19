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
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *     ex_get_init_global()
 *****************************************************************************
 * This function reads the global initial information.
 *****************************************************************************
 *  Variable Index:
 *      exoid            - The NetCDF ID of an already open NemesisI file.
 *      num_nodes_g     - The number of global FEM nodes. This is output as
 *                        a NetCDF variable.
 *      num_elems_g     - The number of global FEM elements. This is output
 *                        as a NetCDF variable.
 *      num_elem_blks_g - The number of global element blocks. This is output
 *                        as a NetCDF dimension.
 *      num_node_sets_g - The number of global node sets. This is output as
 *                        a NetCDF dimension.
 *      num_side_sets_g - The number of global side sets. This is output as
 *                        a NetCDF dimension.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include "exodusII.h"     // for ex_err, void_int, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/* Used to reduce repeated code below */
static int ex_get_dim_value(int exoid, const char *name, const char *dimension_name, int dimension,
                            size_t *value)
{
  char errmsg[MAX_ERR_LENGTH];
  int  status;

  if (nc_inq_dimid(exoid, dimension_name, &dimension) != NC_NOERR) {
    /* optional and default to zero. */
    *value = 0;
  }
  else {
    size_t tmp;
    if ((status = nc_inq_dimlen(exoid, dimension, &tmp)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get number of %s in file id %d", name,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      return (EX_FATAL);
    }
    *value = tmp;
  }
  return (EX_NOERR);
}

/*!
 * \ingroup ModelDescription
 * \undoc
 */
int ex_get_init_global(int exoid, void_int *num_nodes_g, void_int *num_elems_g,
                       void_int *num_elem_blks_g, void_int *num_node_sets_g,
                       void_int *num_side_sets_g)
{
  int    dimid;
  size_t nng, neg, nebg, nnsg, nssg;

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* Check the file version information */
  if ((dimid = ne__check_file_version(exoid)) != EX_NOERR) {
    EX_FUNC_LEAVE(dimid);
  }

  if (ex_get_dim_value(exoid, "global nodes", DIM_NUM_NODES_GLOBAL, dimid, &nng) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "global elements", DIM_NUM_ELEMS_GLOBAL, dimid, &neg) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "global element blocks", DIM_NUM_ELBLK_GLOBAL, dimid, &nebg) !=
      EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "global node sets", DIM_NUM_NS_GLOBAL, dimid, &nnsg) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_get_dim_value(exoid, "global side sets", DIM_NUM_SS_GLOBAL, dimid, &nssg) != EX_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    *(int64_t *)num_nodes_g     = nng;
    *(int64_t *)num_elems_g     = neg;
    *(int64_t *)num_elem_blks_g = nebg;
    *(int64_t *)num_node_sets_g = nnsg;
    *(int64_t *)num_side_sets_g = nssg;
  }
  else {
    *(int *)num_nodes_g     = nng;
    *(int *)num_elems_g     = neg;
    *(int *)num_elem_blks_g = nebg;
    *(int *)num_node_sets_g = nnsg;
    *(int *)num_side_sets_g = nssg;
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

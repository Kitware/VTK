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

#include "exodusII.h"     // for ex_block, ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * writes the parameters used to describe an element/face/edge block
 * \param   exoid                   exodus file id
 * \param   block_count             number of blocks being defined
 * \param   blocks                  array of ex_block structures describing
 * block counts
 */

int ex_put_block_params(int exoid, size_t block_count, const struct ex_block *blocks)
{
  size_t i;
  int    conn_int_type;
  int    status;
  int    arbitrary_polyhedra = 0; /* 1 if block is arbitrary 2d polyhedra type; 2 if 3d polyhedra */
  int    att_name_varid      = -1;
  int    varid, dimid, dims[2], blk_id_ndx, blk_stat, strdim;
  size_t start[2];
  size_t num_blk;
  int    cur_num_blk, numblkdim, numattrdim;
  int    nnodperentdim = -1;
  int    nedgperentdim = -1;
  int    nfacperentdim = -1;
  int    connid        = 0;
  int    npeid;
  char   errmsg[MAX_ERR_LENGTH];
  char * entity_type1     = NULL;
  char * entity_type2     = NULL;
  int *  blocks_to_define = NULL;
  const char *dnumblk     = NULL;
  const char *vblkids     = NULL;
  const char *vblksta     = NULL;
  const char *vnodcon     = NULL;
  const char *vnpecnt     = NULL;
  const char *vedgcon     = NULL;
  const char *vfaccon     = NULL;
  const char *vconn       = NULL;
  const char *vattnam     = NULL;
  const char *vblkatt     = NULL;
  const char *dneblk      = NULL;
  const char *dnape       = NULL;
  const char *dnnpe       = NULL;
  const char *dnepe       = NULL;
  const char *dnfpe       = NULL;
#if NC_HAS_HDF5
  int fill = NC_FILL_CHAR;
#endif

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  if (!(blocks_to_define = malloc(block_count * sizeof(int)))) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to allocate memory for internal blocks_to_define "
             "array in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  for (i = 0; i < block_count; i++) {
    switch (blocks[i].type) {
    case EX_EDGE_BLOCK:
      dnumblk = DIM_NUM_ED_BLK;
      vblkids = VAR_ID_ED_BLK;
      vblksta = VAR_STAT_ED_BLK;
      break;
    case EX_FACE_BLOCK:
      dnumblk = DIM_NUM_FA_BLK;
      vblkids = VAR_ID_FA_BLK;
      vblksta = VAR_STAT_FA_BLK;
      break;
    case EX_ELEM_BLOCK:
      dnumblk = DIM_NUM_EL_BLK;
      vblkids = VAR_ID_EL_BLK;
      vblksta = VAR_STAT_EL_BLK;
      break;
    default:
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: Bad block type (%d) specified for entry %d file id %d", blocks[i].type,
               (int)i, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      free(blocks_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* first check if any blocks of that type are specified */
    if ((status = ex__get_dimension(exoid, dnumblk, ex_name_of_object(blocks[i].type), &num_blk,
                                    &dimid, __func__)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: No %ss defined in file id %d",
               ex_name_of_object(blocks[i].type), exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
      free(blocks_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Next: Make sure that there are not any duplicate block ids by
       searching the vblkids array.
       WARNING: This must be done outside of define mode because id_lkup
       accesses
       the database to determine the position
    */

    if ((status = nc_inq_varid(exoid, vblkids, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s ids in file id %d",
               ex_name_of_object(blocks[i].type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(blocks_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    status = ex__id_lkup(exoid, blocks[i].type, blocks[i].id);
    if (-status != EX_LOOKUPFAIL) { /* found the element block id */
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s id %" PRId64 " already exists in file id %d",
               ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_DUPLICATEID);
      free(blocks_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* Keep track of the total number of element blocks defined using a counter
       stored in a linked list keyed by exoid.
       NOTE: ex__get_file_item  is a function that finds the number of element
       blocks for a specific file and returns that value.
    */
    cur_num_blk = ex__get_file_item(exoid, ex__get_counter_list(blocks[i].type));
    if (cur_num_blk >= (int)num_blk) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: exceeded number of %ss (%d) defined in file id %d",
               ex_name_of_object(blocks[i].type), (int)num_blk, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      free(blocks_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    /*   NOTE: ex__inc_file_item  is a function that finds the number of element
         blocks for a specific file and returns that value incremented. */
    cur_num_blk = ex__inc_file_item(exoid, ex__get_counter_list(blocks[i].type));
    start[0]    = cur_num_blk;

    /* write out block id to previously defined id array variable*/
    status = nc_put_var1_longlong(exoid, varid, start, (long long *)&blocks[i].id);

    if (status != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s id to file id %d",
               ex_name_of_object(blocks[i].type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(blocks_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    blocks_to_define[i] = start[0] + 1; /* element id index into vblkids array*/

    if (blocks[i].num_entry == 0) { /* Is this a NULL element block? */
      blk_stat = 0;                 /* change element block status to NULL */
    }
    else {
      blk_stat = 1; /* change element block status to EX_EX_TRUE */
    }

    if ((status = nc_inq_varid(exoid, vblksta, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s status in file id %d",
               ex_name_of_object(blocks[i].type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(blocks_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_put_var1_int(exoid, varid, start, &blk_stat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to store %s id %" PRId64 " status to file id %d",
               ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(blocks_to_define);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* put netcdf file into define mode  */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to place file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    free(blocks_to_define);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  for (i = 0; i < block_count; i++) {
    if (blocks[i].num_entry == 0) { /* Is this a NULL element block? */
      continue;
    }

    blk_id_ndx = blocks_to_define[i];

    switch (blocks[i].type) {
    case EX_EDGE_BLOCK:
      dneblk  = DIM_NUM_ED_IN_EBLK(blk_id_ndx);
      dnnpe   = DIM_NUM_NOD_PER_ED(blk_id_ndx);
      dnepe   = 0;
      dnfpe   = 0;
      dnape   = DIM_NUM_ATT_IN_EBLK(blk_id_ndx);
      vblkatt = VAR_EATTRIB(blk_id_ndx);
      vattnam = VAR_NAME_EATTRIB(blk_id_ndx);
      vnodcon = VAR_EBCONN(blk_id_ndx);
      vedgcon = 0;
      vfaccon = 0;
      break;
    case EX_FACE_BLOCK:
      dneblk  = DIM_NUM_FA_IN_FBLK(blk_id_ndx);
      dnnpe   = DIM_NUM_NOD_PER_FA(blk_id_ndx);
      dnepe   = 0;
      dnfpe   = 0;
      dnape   = DIM_NUM_ATT_IN_FBLK(blk_id_ndx);
      vblkatt = VAR_FATTRIB(blk_id_ndx);
      vattnam = VAR_NAME_FATTRIB(blk_id_ndx);
      vnodcon = VAR_FBCONN(blk_id_ndx);
      vnpecnt = VAR_FBEPEC(blk_id_ndx);
      vedgcon = 0;
      vfaccon = 0;
      break;
    case EX_ELEM_BLOCK:
      dneblk  = DIM_NUM_EL_IN_BLK(blk_id_ndx);
      dnnpe   = DIM_NUM_NOD_PER_EL(blk_id_ndx);
      dnepe   = DIM_NUM_EDG_PER_EL(blk_id_ndx);
      dnfpe   = DIM_NUM_FAC_PER_EL(blk_id_ndx);
      dnape   = DIM_NUM_ATT_IN_BLK(blk_id_ndx);
      vblkatt = VAR_ATTRIB(blk_id_ndx);
      vattnam = VAR_NAME_ATTRIB(blk_id_ndx);
      vnodcon = VAR_CONN(blk_id_ndx);
      vnpecnt = VAR_EBEPEC(blk_id_ndx);
      vedgcon = VAR_ECONN(blk_id_ndx);
      vfaccon = VAR_FCONN(blk_id_ndx);
      break;
    default: goto error_ret;
    }

    /* define some dimensions and variables*/
    if ((status = nc_def_dim(exoid, dneblk, blocks[i].num_entry, &numblkdim)) != NC_NOERR) {
      if (status == NC_ENAMEINUSE) { /* duplicate entry */
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: %s %" PRId64 " already defined in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      else {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of entities/block for %s %" PRId64 " file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
      }
      goto error_ret; /* exit define mode and return */
    }

    if (dnnpe && blocks[i].num_nodes_per_entry > 0) {
      /* A nfaced block would not have any nodes defined... */
      if ((status = nc_def_dim(exoid, dnnpe, blocks[i].num_nodes_per_entry, &nnodperentdim)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of nodes/entity for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }

    if (dnepe && blocks[i].num_edges_per_entry > 0) {
      if ((status = nc_def_dim(exoid, dnepe, blocks[i].num_edges_per_entry, &nedgperentdim)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of edges/entity for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }

    if (dnfpe && blocks[i].num_faces_per_entry > 0) {
      if ((status = nc_def_dim(exoid, dnfpe, blocks[i].num_faces_per_entry, &nfacperentdim)) !=
          NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of faces/entity for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }

    /* element attribute array */
    if (blocks[i].num_attribute > 0) {

      if ((status = nc_def_dim(exoid, dnape, blocks[i].num_attribute, &numattrdim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define number of attributes in %s %" PRId64 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      dims[0] = numblkdim;
      dims[1] = numattrdim;

      if ((status = nc_def_var(exoid, vblkatt, nc_flt_code(exoid), 2, dims, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR:  failed to define attributes for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      ex__compress_variable(exoid, varid, 2);

#if defined(PARALLEL_AWARE_EXODUS)
      /*
       * There is currently a bug in netcdf-4.5.1-devel and earlier
       * for partial parallel output of strided arrays in collective
       * mode for netcdf-4-based output.  If the number of attributes >
       * 1 and in parallel mode, set the mode to independent.
       */
      if (blocks[i].num_attribute > 1) {
        struct ex__file_item *file = ex__find_file_item(exoid);
        if (file->is_parallel && file->is_hdf5) {
          nc_var_par_access(exoid, varid, NC_INDEPENDENT);
        }
      }
#endif

      /* inquire previously defined dimensions  */
      if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &strdim)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get string length in file id %d", exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret;
      }

      /* Attribute names... */
      dims[0] = numattrdim;
      dims[1] = strdim;

      if ((status = nc_def_var(exoid, vattnam, NC_CHAR, 2, dims, &att_name_varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define %s attribute name array in file id %d",
                 ex_name_of_object(blocks[i].type), exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
#if NC_HAS_HDF5
      nc_def_var_fill(exoid, att_name_varid, 0, &fill);
#endif
    }

    conn_int_type = NC_INT;
    if (ex_int64_status(exoid) & EX_BULK_INT64_DB) {
      conn_int_type = NC_INT64;
    }

    /* See if storing an 'nsided' element block (arbitrary 2d polyhedra or super
     * element) */
    if (strlen(blocks[i].topology) >= 3) {
      if ((blocks[i].topology[0] == 'n' || blocks[i].topology[0] == 'N') &&
          (blocks[i].topology[1] == 's' || blocks[i].topology[1] == 'S') &&
          (blocks[i].topology[2] == 'i' || blocks[i].topology[2] == 'I')) {
        arbitrary_polyhedra = 1;
      }
      else if ((blocks[i].topology[0] == 'n' || blocks[i].topology[0] == 'N') &&
               (blocks[i].topology[1] == 'f' || blocks[i].topology[1] == 'F') &&
               (blocks[i].topology[2] == 'a' || blocks[i].topology[2] == 'A')) {
        /* If a FACE_BLOCK, then we are dealing with the faces of the nfaced
         * blocks[i]. */
        arbitrary_polyhedra = blocks[i].type == EX_FACE_BLOCK ? 1 : 2;
      }
    }

    /* element connectivity array */
    if (arbitrary_polyhedra > 0) {
      if (blocks[i].type != EX_FACE_BLOCK && blocks[i].type != EX_ELEM_BLOCK) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: Bad block type (%d) for nsided/nfaced block in file id %d", blocks[i].type,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
        goto error_ret;
      }

      if (arbitrary_polyhedra == 1) {
        dims[0] = nnodperentdim;
        vconn   = vnodcon;

        /* store entity types as attribute of npeid variable -- node/elem,
         * node/face, face/elem*/
        entity_type1 = "NODE";
        if (blocks[i].type == EX_ELEM_BLOCK) {
          entity_type2 = "ELEM";
        }
        else {
          entity_type2 = "FACE";
        }
      }
      else if (arbitrary_polyhedra == 2) {
        dims[0] = nfacperentdim;
        vconn   = vfaccon;

        /* store entity types as attribute of npeid variable -- node/elem,
         * node/face, face/elem*/
        entity_type1 = "FACE";
        entity_type2 = "ELEM";
      }

      if ((status = nc_def_var(exoid, vconn, conn_int_type, 1, dims, &connid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to create connectivity array for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      /* element face-per-element or node-per-element count array */
      dims[0] = numblkdim;

      if ((status = nc_def_var(exoid, vnpecnt, conn_int_type, 1, dims, &npeid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to create face- or node- per-entity "
                 "count array for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }

      if ((status = nc_put_att_text(exoid, npeid, "entity_type1", strlen(entity_type1) + 1,
                                    entity_type1)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to store entity type attribute text for %s %" PRId64
                 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      if ((status = nc_put_att_text(exoid, npeid, "entity_type2", strlen(entity_type2) + 1,
                                    entity_type2)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to store entity type attribute text for %s %" PRId64
                 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }
    else {
      if (blocks[i].num_nodes_per_entry > 0) {
        /* "Normal" (non-polyhedra) element block type */
        dims[0] = numblkdim;
        dims[1] = nnodperentdim;

        if ((status = nc_def_var(exoid, vnodcon, conn_int_type, 2, dims, &connid)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to create connectivity array for %s %" PRId64 " in file id %d",
                   ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          goto error_ret; /* exit define mode and return */
        }
        ex__compress_variable(exoid, connid, 1);
      }
    }
    /* store element type as attribute of connectivity variable */
    if (connid > 0) {
      if ((status = nc_put_att_text(exoid, connid, ATT_NAME_ELB, strlen(blocks[i].topology) + 1,
                                    blocks[i].topology)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s type name %s in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].topology, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }

    if (arbitrary_polyhedra == 0) {
      if (vedgcon && blocks[i].num_edges_per_entry) {
        dims[0] = numblkdim;
        dims[1] = nedgperentdim;

        if ((status = nc_def_var(exoid, vedgcon, conn_int_type, 2, dims, &varid)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to create edge connectivity array for %s %" PRId64
                   " in file id %d",
                   ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          goto error_ret; /* exit define mode and return */
        }
      }

      if (vfaccon && blocks[i].num_faces_per_entry) {
        dims[0] = numblkdim;
        dims[1] = nfacperentdim;

        if ((status = nc_def_var(exoid, vfaccon, conn_int_type, 2, dims, &varid)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to create face connectivity array for %s %" PRId64
                   " in file id %d",
                   ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          goto error_ret; /* exit define mode and return */
        }
      }
    }
  }

  free(blocks_to_define);

  /* leave define mode  */
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  for (i = 0; i < block_count; i++) {
    switch (blocks[i].type) {
    case EX_EDGE_BLOCK: vblkids = VAR_ID_ED_BLK; break;
    case EX_FACE_BLOCK: vblkids = VAR_ID_FA_BLK; break;
    case EX_ELEM_BLOCK: vblkids = VAR_ID_EL_BLK; break;
    default: EX_FUNC_LEAVE(EX_FATAL); /* should have been handled earlier; quiet compiler here */
    }

    nc_inq_varid(exoid, vblkids, &att_name_varid);

    if (blocks[i].num_attribute > 0 && att_name_varid >= 0) {
      /* Output a dummy empty attribute name in case client code doesn't
         write anything; avoids corruption in some cases.
      */
      size_t count[2];
      char * text = "";
      size_t j;

      count[0] = 1;
      start[1] = 0;
      count[1] = strlen(text) + 1;

      for (j = 0; j < blocks[i].num_attribute; j++) {
        start[0] = j;
        nc_put_vara_text(exoid, att_name_varid, start, count, text);
      }
    }
  }

  EX_FUNC_LEAVE(EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  free(blocks_to_define);

  ex__leavedef(exoid, __func__);
  EX_FUNC_LEAVE(EX_FATAL);
}

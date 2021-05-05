/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_block, ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * Internal function used to put a homogeneous `blocks` array that
 * contains all blocks of that specified type that will be defined.
 * Permits some optimizations and safer for N->1 parallel.
 * Arbitrary  polyhedra are handled in more general routine; not here.
 */
int ex__put_homogenous_block_params(int exoid, size_t block_count, const struct ex_block *blocks)
{

  int  status;
  int  varid, dims[2];
  char errmsg[MAX_ERR_LENGTH];

  if (ex__check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  const char *vblkids = NULL;
  const char *vblksta = NULL;

  switch (blocks[0].type) {
  case EX_EDGE_BLOCK:
    vblkids = VAR_ID_ED_BLK;
    vblksta = VAR_STAT_ED_BLK;
    break;
  case EX_FACE_BLOCK:
    vblkids = VAR_ID_FA_BLK;
    vblksta = VAR_STAT_FA_BLK;
    break;
  case EX_ELEM_BLOCK:
    vblkids = VAR_ID_EL_BLK;
    vblksta = VAR_STAT_EL_BLK;
    break;
  default:
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: Bad block type (%d) specified for all blocks file id %d", blocks[0].type,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
    return (EX_FATAL);
  }

  { /* Output ids for this block */
    long long *ids = NULL;
    if (!(ids = malloc(block_count * sizeof(long long)))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate memory for block ids "
               "array in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      return (EX_FATAL);
    }

    for (size_t i = 0; i < block_count; i++) {
      ids[i] = (long long)blocks[i].id;
    }
    /* write out block id to previously defined id array variable*/
    if ((status = nc_inq_varid(exoid, vblkids, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s ids in file id %d",
               ex_name_of_object(blocks[0].type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(ids);
      return (EX_FATAL);
    }

    if ((status = nc_put_var_longlong(exoid, varid, ids)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s ids to file id %d",
               ex_name_of_object(blocks[0].type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(ids);
      return (EX_FATAL);
    }
    free(ids);
  }

  { /* Output the block status array */
    int *stat = NULL;
    if (!(stat = malloc(block_count * sizeof(int)))) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to allocate memory for status array "
               "array in file id %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_MEMFAIL);
      return (EX_FATAL);
    }

    for (size_t i = 0; i < block_count; i++) {
      stat[i] = (blocks[i].num_entry == 0) ? 0 : 1;
    }

    if ((status = nc_inq_varid(exoid, vblksta, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to locate %s status in file id %d",
               ex_name_of_object(blocks[0].type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(stat);
      return (EX_FATAL);
    }

    if ((status = nc_put_var_int(exoid, varid, stat)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s status to file id %d",
               ex_name_of_object(blocks[0].type), exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      free(stat);
      return (EX_FATAL);
    }
    free(stat);
  }

  /* ======================================================================== */
  /* put netcdf file into define mode  */
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to place file id %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    return (EX_FATAL);
  }

  /* inquire previously defined dimensions  */
  int strdim = 0;
  if ((status = nc_inq_dimid(exoid, DIM_STR_NAME, &strdim)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get string length in file id %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    goto error_ret;
  }

  for (size_t i = 0; i < block_count; i++) {
    int blk_id_ndx = 1 + ex__inc_file_item(exoid, ex__get_counter_list(blocks[i].type));

    if (blocks[i].num_entry == 0) { /* Is this a NULL element block? */
      continue;
    }

    const char *vnodcon = NULL;
    const char *vedgcon = NULL;
    const char *vfaccon = NULL;
    const char *vattnam = NULL;
    const char *vblkatt = NULL;
    const char *dneblk  = NULL;
    const char *dnape   = NULL;
    const char *dnnpe   = NULL;
    const char *dnepe   = NULL;
    const char *dnfpe   = NULL;

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
      vedgcon = VAR_ECONN(blk_id_ndx);
      vfaccon = VAR_FCONN(blk_id_ndx);
      break;
    default: goto error_ret;
    }

    /* define some dimensions and variables*/
    int numblkdim = 0;
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

    int nnodperentdim = -1;
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

    int nedgperentdim = -1;
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

    int nfacperentdim = -1;
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

      int numattrdim = 0;
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

      /* Attribute names... */
      dims[0] = numattrdim;
      dims[1] = strdim;

      int att_name_varid = -1;
      if ((status = nc_def_var(exoid, vattnam, NC_CHAR, 2, dims, &att_name_varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to define %s attribute name array in file id %d",
                 ex_name_of_object(blocks[i].type), exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
#if NC_HAS_HDF5
      int fill = NC_FILL_CHAR;
      nc_def_var_fill(exoid, att_name_varid, 0, &fill);
#endif
    }

    int conn_int_type = NC_INT;
    if (ex_int64_status(exoid) & EX_BULK_INT64_DB) {
      conn_int_type = NC_INT64;
    }

    /* element connectivity array */
    if (blocks[i].num_nodes_per_entry > 0) {
      /* "Normal" (non-polyhedra) element block type */
      dims[0] = numblkdim;
      dims[1] = nnodperentdim;

      int connid = 0;
      if ((status = nc_def_var(exoid, vnodcon, conn_int_type, 2, dims, &connid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to create connectivity array for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
      ex__compress_variable(exoid, connid, 1);

      /* store element type as attribute of connectivity variable */
      if ((status = nc_put_att_text(exoid, connid, ATT_NAME_ELB, strlen(blocks[i].topology) + 1,
                                    blocks[i].topology)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to store %s type name %s in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].topology, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }

    if (vedgcon && blocks[i].num_edges_per_entry) {
      dims[0] = numblkdim;
      dims[1] = nedgperentdim;

      if ((status = nc_def_var(exoid, vedgcon, conn_int_type, 2, dims, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to create edge connectivity array for %s %" PRId64 " in file id %d",
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
                 "ERROR: failed to create face connectivity array for %s %" PRId64 " in file id %d",
                 ex_name_of_object(blocks[i].type), blocks[i].id, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        goto error_ret; /* exit define mode and return */
      }
    }
  }

  /* leave define mode  */
  if ((status = ex__leavedef(exoid, __func__)) != NC_NOERR) {
    return (EX_FATAL);
  }

  /* ======================================================================== */
  for (size_t i = 0; i < block_count; i++) {
    switch (blocks[i].type) {
    case EX_EDGE_BLOCK: vblkids = VAR_ID_ED_BLK; break;
    case EX_FACE_BLOCK: vblkids = VAR_ID_FA_BLK; break;
    case EX_ELEM_BLOCK: vblkids = VAR_ID_EL_BLK; break;
    default: return (EX_FATAL); /* should have been handled earlier; quiet compiler here */
    }

    int att_name_varid = -1;
    nc_inq_varid(exoid, vblkids, &att_name_varid);

    if (blocks[i].num_attribute > 0 && att_name_varid >= 0) {
      /* Output a dummy empty attribute name in case client code doesn't
         write anything; avoids corruption in some cases.
      */
      size_t count[2];
      size_t start[2];
      char * text = "";

      count[0] = 1;
      start[1] = 0;
      count[1] = strlen(text) + 1;

      for (size_t j = 0; j < blocks[i].num_attribute; j++) {
        start[0] = j;
        nc_put_vara_text(exoid, att_name_varid, start, count, text);
      }
    }
  }

  return (EX_NOERR);

/* Fatal error: exit definition mode and return */
error_ret:
  ex__leavedef(exoid, __func__);
  return (EX_FATAL);
}

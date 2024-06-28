/*
 * Copyright(C) 1999-2020, 2022 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *      ex_get_loadbal_param()
 *****************************************************************************
 * This function retrieves the load balance parameters.
 *****************************************************************************
 *  Variable Index:
 *      exoid             - The NetCDF ID of an already open NemesisI file.
 *      num_int_nodes    - The number of internal FEM nodes.
 *      num_bor_nodes    - The number of border FEM nodes.
 *      num_ext_nodes    - The number of external FEM nodes.
 *      num_int_elems    - The number of internal FEM elements.
 *      num_bor_elems    - The number of border FEM elements.
 *      num_node_cmaps   - The number of nodal communication maps.
 *      num_elem_cmaps   - The number of elemental communication maps.
 *      processor        - The processor the file being read was written for.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, DIM_NUM_BOR_ELEMS, etc

/*!
 * \ingroup ModelDescription
 * \undoc
 */
int ex_get_loadbal_param(int exoid, void_int *num_int_nodes, void_int *num_bor_nodes,
                         void_int *num_ext_nodes, void_int *num_int_elems, void_int *num_bor_elems,
                         void_int *num_node_cmaps, void_int *num_elem_cmaps, int processor)
{
  int     dimid, varid, status;
  size_t  start[1];
  size_t  nin, nbn, nen, nie, nbe, nncm, necm;
  int64_t varidx[2];
  char    ftype[2];
  int     nmstat;

  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/
  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    *(int64_t *)num_int_nodes  = 0;
    *(int64_t *)num_bor_nodes  = 0;
    *(int64_t *)num_ext_nodes  = 0;
    *(int64_t *)num_int_elems  = 0;
    *(int64_t *)num_bor_elems  = 0;
    *(int64_t *)num_node_cmaps = 0;
    *(int64_t *)num_elem_cmaps = 0;
  }
  else {
    *(int *)num_int_nodes  = 0;
    *(int *)num_bor_nodes  = 0;
    *(int *)num_ext_nodes  = 0;
    *(int *)num_int_elems  = 0;
    *(int *)num_bor_elems  = 0;
    *(int *)num_node_cmaps = 0;
    *(int *)num_elem_cmaps = 0;
  }

  /* Check the file version information */
  if ((dimid = nei_check_file_version(exoid)) != EX_NOERR) {
    EX_FUNC_LEAVE(dimid);
  }

  /* Get the file type */
  if (exi_get_file_type(exoid, ftype) != EX_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unable to find file type for file ID %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the status for this node map */
  if ((status = nc_inq_varid(exoid, VAR_INT_N_STAT, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" from file ID %d",
             VAR_INT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ftype[0] == 's') {
    start[0] = processor;
  }
  else {
    start[0] = 0;
  }

  if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get status for \"%s\" from file ID %d",
             VAR_INT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (nmstat == 1) {
    if (ex_get_idx(exoid, VAR_NODE_MAP_INT_IDX, varidx, processor) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find index variable, \"%s\", in file ID %d", VAR_NODE_MAP_INT_IDX,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* check if I need to get the dimension of the internal node map */
    if (varidx[1] == -1) {
      /* Get the dimension ID for the number of internal nodes */
      if ((status = nc_inq_dimid(exoid, DIM_NUM_INT_NODES, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find dimension ID for \"%s\" in file ID %d", DIM_NUM_INT_NODES,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);

        EX_FUNC_LEAVE(EX_FATAL);
      }

      /*
       * Get the value of the dimension representing the total number of
       * internal FEM nodes.
       */
      if ((status = nc_inq_dimlen(exoid, dimid, &nin)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d",
                 DIM_NUM_INT_NODES, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);

        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* set the end value for the node map */
      varidx[1] = nin;
    } /* End "if (varidx[1] = -1)" */

    /* now get the number of nodes */
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      *(int64_t *)num_int_nodes = varidx[1] - varidx[0];
    }
    else {
      *(int *)num_int_nodes = varidx[1] - varidx[0];
    }
  }

  /* Get the status for this node map */
  if ((status = nc_inq_varid(exoid, VAR_BOR_N_STAT, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" from file ID %d",
             VAR_BOR_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ftype[0] == 's') {
    start[0] = processor;
  }
  else {
    start[0] = 0;
  }

  if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get status for \"%s\" from file ID %d",
             VAR_BOR_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (nmstat == 1) {
    if (ex_get_idx(exoid, VAR_NODE_MAP_BOR_IDX, varidx, processor) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find index variable, \"%s\", in file ID %d", VAR_NODE_MAP_BOR_IDX,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* check if I need to get the dimension of the border node map */
    if (varidx[1] == -1) {
      /* Get the dimension ID for the number of border nodes */
      if ((status = nc_inq_dimid(exoid, DIM_NUM_BOR_NODES, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find dimension ID for \"%s\" in file ID %d", DIM_NUM_BOR_NODES,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);

        EX_FUNC_LEAVE(EX_FATAL);
      }

      /*
       * Get the value of the dimension representing the number of border
       * FEM nodes.
       */
      if ((status = nc_inq_dimlen(exoid, dimid, &nbn)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d",
                 DIM_NUM_BOR_NODES, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);

        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* set the end value for the node map */
      varidx[1] = nbn;
    } /* End "if (varidx[1] == -1)" */

    /* now calculate the number of nodes */
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      *(int64_t *)num_bor_nodes = varidx[1] - varidx[0];
    }
    else {
      *(int *)num_bor_nodes = varidx[1] - varidx[0];
    }
  }

  /* Get the status for this node map */
  if ((status = nc_inq_varid(exoid, VAR_EXT_N_STAT, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" from file ID %d",
             VAR_EXT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ftype[0] == 's') {
    start[0] = processor;
  }
  else {
    start[0] = 0;
  }

  if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get status for \"%s\" from file ID %d",
             VAR_EXT_N_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (nmstat == 1) {
    if (ex_get_idx(exoid, VAR_NODE_MAP_EXT_IDX, varidx, processor) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find index variable, \"%s\", in file ID %d", VAR_NODE_MAP_EXT_IDX,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* check if I need to get the dimension of the external node map */
    if (varidx[1] == -1) {
      /* Get the dimension ID for the number of external nodes */
      if ((status = nc_inq_dimid(exoid, DIM_NUM_EXT_NODES, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find dimension ID for \"%s\" in file ID %d", DIM_NUM_EXT_NODES,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);

        EX_FUNC_LEAVE(EX_FATAL);
      }

      /*
       * Get the value of the dimension representing the number of external
       * FEM nodes.
       */
      if ((status = nc_inq_dimlen(exoid, dimid, &nen)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d",
                 DIM_NUM_EXT_NODES, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);

        EX_FUNC_LEAVE(EX_FATAL);
      }
      /* set the end value for the node map */
      varidx[1] = nen;
    } /* End "if (varidx[1] == -1)" */

    /* now get the number of nodes */
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      *(int64_t *)num_ext_nodes = varidx[1] - varidx[0];
    }
    else {
      *(int *)num_ext_nodes = varidx[1] - varidx[0];
    }
  }

  /* Get the status for this element map */
  if ((status = nc_inq_varid(exoid, VAR_INT_E_STAT, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" from file ID %d",
             VAR_INT_E_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ftype[0] == 's') {
    start[0] = processor;
  }
  else {
    start[0] = 0;
  }

  if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get status for \"%s\" from file ID %d",
             VAR_INT_E_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (nmstat == 1) {
    if (ex_get_idx(exoid, VAR_ELEM_MAP_INT_IDX, varidx, processor) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find index variable, \"%s\", in file ID %d", VAR_ELEM_MAP_INT_IDX,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* check if I need to get the dimension of the internal element map */
    if (varidx[1] == -1) {
      /* Get the dimension ID for the number of internal elements */
      if ((status = nc_inq_dimid(exoid, DIM_NUM_INT_ELEMS, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find dimension ID for \"%s\" from file ID %d", DIM_NUM_INT_ELEMS,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);

        EX_FUNC_LEAVE(EX_FATAL);
      }

      /*
       * Get the value of the dimension representing the number of internal
       * FEM elements.
       */
      if ((status = nc_inq_dimlen(exoid, dimid, &nie)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d",
                 DIM_NUM_INT_ELEMS, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* set the end value for the node map */
      varidx[1] = nie;
    } /* End "if (varidx[1] == -1)" */

    /* now get the number of elements */
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      *(int64_t *)num_int_elems = varidx[1] - varidx[0];
    }
    else {
      *(int *)num_int_elems = varidx[1] - varidx[0];
    }
  }

  /* Get the status for this element map */
  if ((status = nc_inq_varid(exoid, VAR_BOR_E_STAT, &varid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find variable ID for \"%s\" from file ID %d",
             VAR_BOR_E_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (ftype[0] == 's') {
    start[0] = processor;
  }
  else {
    start[0] = 0;
  }

  if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get status for \"%s\" from file ID %d",
             VAR_BOR_E_STAT, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (nmstat == 1) {
    if (ex_get_idx(exoid, VAR_ELEM_MAP_BOR_IDX, varidx, processor) == -1) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to find index variable, \"%s\", in file ID %d", VAR_ELEM_MAP_BOR_IDX,
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    /* check if I need to get the dimension of the border element map */
    if (varidx[1] == -1) {
      /* Get the dimension ID for the number of border elements */
      if ((status = nc_inq_dimid(exoid, DIM_NUM_BOR_ELEMS, &dimid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find dimension ID for \"%s\" from file ID %d", DIM_NUM_BOR_ELEMS,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);

        EX_FUNC_LEAVE(EX_FATAL);
      }

      /*
       * Get the value of the dimension representing the number of border
       * FEM elements.
       */
      if ((status = nc_inq_dimlen(exoid, dimid, &nbe)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d",
                 DIM_NUM_BOR_ELEMS, exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* set the end value for the node map */
      varidx[1] = nbe;
    } /* End "if (varidx[1] == -1)" */

    /* now get the number of nodes */
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      *(int64_t *)num_bor_elems = varidx[1] - varidx[0];
    }
    else {
      *(int *)num_bor_elems = varidx[1] - varidx[0];
    }
  } /* End "if (nmstat == 1)" */

  if (ex_get_idx(exoid, VAR_N_COMM_INFO_IDX, varidx, processor) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find index variable, \"%s\", in file ID %d",
             VAR_N_COMM_INFO_IDX, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* check if I need to get the dimension of the nodal comm map */
  if (varidx[1] == -1) {
    /* Get the nodal comm map information */
    if ((status = nc_inq_dimid(exoid, DIM_NUM_N_CMAPS, &dimid)) != NC_NOERR) {
      varidx[1] = 0;
    }
    else {
      if ((status = nc_inq_dimlen(exoid, dimid, &nncm)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NUM_N_CMAPS,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }
      /* set the end value for the node map */
      varidx[1] = nncm;
    }
  } /* End "if (varidx[1] == -1)" */

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    *(int64_t *)num_node_cmaps = varidx[1] - varidx[0];
  }
  else {
    *(int *)num_node_cmaps = varidx[1] - varidx[0];
  }

  if (ex_get_idx(exoid, VAR_E_COMM_INFO_IDX, varidx, processor) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find index variable, \"%s\", in file ID %d",
             VAR_E_COMM_INFO_IDX, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* check if I need to get the dimension of the elemental comm map */
  if (varidx[1] == -1) {
    /* Get the elemental comm map information */
    if (nc_inq_dimid(exoid, DIM_NUM_E_CMAPS, &dimid) != NC_NOERR) {
      varidx[1] = 0;
    }
    else {
      if ((status = nc_inq_dimlen(exoid, dimid, &necm)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NUM_E_CMAPS,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* set the end value for the node map */
      varidx[1] = necm;
    }
  } /* End "if (varidx[1] == -1)" */

  if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
    *(int64_t *)num_elem_cmaps = varidx[1] - varidx[0];
  }
  else {
    *(int *)num_elem_cmaps = varidx[1] - varidx[0];
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

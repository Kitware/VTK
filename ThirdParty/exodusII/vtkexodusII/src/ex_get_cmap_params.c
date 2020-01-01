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
/*****************************************************************************/
/* Function(s) contained in this file:
 *     ex_get_cmap_params()
 *****************************************************************************
 * This function outputs the communication map parameters.
 *****************************************************************************
 *  Variable Index:
 *      exoid                - The NetCDF ID of an already open NemesisI file.
 *      node_cmap_ids       - Pointer to vector of nodal communication
 *                            set IDs.
 *      node_cmap_node_cnts - Pointer to a vector which contains a count of
 *                            the number of FEM nodes for each nodal
 *                            communication map.
 *      elem_cmap_ids       - Pointer to vector for retrieval of elemental
 *                            communication set IDs.
 *      elem_cmap_elem_cnts - Pointer to a vector which contains a count of
 *                            the number of FEM elements for each elemental
 *                            communication map.
 *      processor           - The processor the file being read was written
 *                            for.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>     // for ex_err, etc
#include <exodusII_int.h> // for EX_FATAL, VAR_E_COMM_IDS, etc

int ex_get_cmap_params(int exoid, void_int *node_cmap_ids, void_int *node_cmap_node_cnts,
                       void_int *elem_cmap_ids, void_int *elem_cmap_elem_cnts, int processor)
{
  size_t  cnt, num_n_comm_maps, num_e_comm_maps, start[1], count[1];
  int64_t cmap_info_idx[2], cmap_data_idx[2];
  int     nmstat;
  int     status, map_idx, varid, dimid;

  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /*****************************************************************************/
  /*****************************************************************************/
  /*                    Nodal communication map(s) */
  /*****************************************************************************/
  /*****************************************************************************/

  /* get the cmap information variables index */
  if (ex_get_idx(exoid, VAR_N_COMM_INFO_IDX, cmap_info_idx, processor) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find index variable, \"%s\", in file ID %d",
             VAR_N_COMM_INFO_IDX, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the number of nodal communications maps in the file */
  if ((status = nc_inq_dimid(exoid, DIM_NUM_N_CMAPS, &dimid)) == NC_NOERR) {
    /* check if I need to get the dimension of the nodal comm map */
    if (cmap_info_idx[1] == -1) {
      if ((status = nc_inq_dimlen(exoid, dimid, count)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NUM_N_CMAPS,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* set the end value for the node map */
      cmap_info_idx[1] = count[0];
    } /* End "if (cmap_info_idx[1] == -1) */

    num_n_comm_maps = cmap_info_idx[1] - cmap_info_idx[0];

    if (num_n_comm_maps > 0) {
      count[0] = num_n_comm_maps;

      /* Get the variable ID for the vector of nodal comm map IDs */
      if ((status = nc_inq_varid(exoid, VAR_N_COMM_IDS, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get variable ID for \"%s\" in file ID %d", VAR_N_COMM_IDS,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* Get the vector of nodal communication map IDs */
      if (node_cmap_ids != NULL) {
        start[0] = cmap_info_idx[0];
        if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
          status = nc_get_vara_longlong(exoid, varid, start, count, node_cmap_ids);
        }
        else {
          status = nc_get_vara_int(exoid, varid, start, count, node_cmap_ids);
        }

        if (status != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable \"%s\" from file ID %d",
                   VAR_N_COMM_IDS, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }

        if ((status = nc_inq_varid(exoid, VAR_N_COMM_STAT, &varid)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to find variable ID for \"%s\" from file ID %d", VAR_N_COMM_STAT,
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }

        if (node_cmap_node_cnts != NULL) {

          /* Get the node counts in each of the nodal communication maps */
          for (cnt = 0; cnt < num_n_comm_maps; cnt++) {
            int64_t cmap_id;
            if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
              cmap_id = ((int64_t *)node_cmap_ids)[cnt];
            }
            else {
              cmap_id = ((int *)node_cmap_ids)[cnt];
            }

            if ((map_idx = ne__id_lkup(exoid, VAR_N_COMM_IDS, cmap_info_idx, cmap_id)) < 0) {
              snprintf(errmsg, MAX_ERR_LENGTH,
                       "ERROR: failed to find nodal comm map with ID %" PRId64 " in file ID %d",
                       cmap_id, exoid);
              ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
              EX_FUNC_LEAVE(EX_FATAL);
            }

            /* Check the status of the node map */
            start[0] = map_idx;
            if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
              snprintf(errmsg, MAX_ERR_LENGTH,
                       "ERROR: failed to get status for \"%s\" from file ID %d", VAR_N_COMM_STAT,
                       exoid);
              ex_err_fn(exoid, __func__, errmsg, status);
              EX_FUNC_LEAVE(EX_FATAL);
            }

            if (nmstat == 1) {

              /* get the cmap information variables index */
              if (ex_get_idx(exoid, VAR_N_COMM_DATA_IDX, cmap_data_idx, map_idx) == -1) {
                snprintf(errmsg, MAX_ERR_LENGTH,
                         "ERROR: failed to find index variable, \"%s\", "
                         "in file ID %d",
                         VAR_N_COMM_DATA_IDX, exoid);
                ex_err_fn(exoid, __func__, errmsg, status);

                EX_FUNC_LEAVE(EX_FATAL);
              }

              if (cmap_data_idx[1] == -1) {
                /*
                 * Find the dimension ID of the variable containing the
                 * node count
                 */
                if ((status = nc_inq_dimid(exoid, DIM_NCNT_CMAP, &dimid)) != NC_NOERR) {
                  snprintf(errmsg, MAX_ERR_LENGTH,
                           "ERROR: failed to find dimension ID for "
                           "\"%s\" in file ID %d",
                           DIM_NCNT_CMAP, exoid);
                  ex_err_fn(exoid, __func__, errmsg, status);
                  EX_FUNC_LEAVE(EX_FATAL);
                }

                /* Find the value of the number of nodes in this nodal comm map
                 */
                if ((status = nc_inq_dimlen(exoid, dimid, count)) != NC_NOERR) {
                  snprintf(errmsg, MAX_ERR_LENGTH,
                           "ERROR: failed to find length of dimension "
                           "\"%s\" in file ID %d",
                           DIM_NCNT_CMAP, exoid);
                  ex_err_fn(exoid, __func__, errmsg, status);
                  EX_FUNC_LEAVE(EX_FATAL);
                }

                cmap_data_idx[1] = count[0];
              }

              if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
                ((int64_t *)node_cmap_node_cnts)[cnt] = cmap_data_idx[1] - cmap_data_idx[0];
              }
              else {
                ((int *)node_cmap_node_cnts)[cnt] = cmap_data_idx[1] - cmap_data_idx[0];
              }
            }
            else if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
              ((int64_t *)node_cmap_node_cnts)[cnt] = 0;
            }
            else {
              ((int *)node_cmap_node_cnts)[cnt] = 0;
            }
          } /* "for(cnt=0; cnt < num_n_comm_maps; cnt++)" */
        }   /* "if (node_cmap_node_cnts != NULL)" */
      }     /* "if (node_cmap_ids != NULL)" */
    }       /* "if (num_n_comm_maps > 0)" */
  }         /* End "if ((dimid = nc_inq_dimid(exoid, DIM_NUM_N_CMAPS)) != -1)" */

  /*****************************************************************************/
  /*****************************************************************************/
  /*                Elemental communication map(s) */
  /*****************************************************************************/
  /*****************************************************************************/

  /* get the cmap information variables index */
  if (ex_get_idx(exoid, VAR_E_COMM_INFO_IDX, cmap_info_idx, processor) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find index variable, \"%s\", in file ID %d",
             VAR_E_COMM_INFO_IDX, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the number of elemental communications maps in the file */
  if ((status = nc_inq_dimid(exoid, DIM_NUM_E_CMAPS, &dimid)) == NC_NOERR) {
    /* check if I need to get the dimension of the nodal comm map */
    if (cmap_info_idx[1] == -1) {
      if ((status = nc_inq_dimlen(exoid, dimid, count)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NUM_E_CMAPS,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* set the end value for the node map */
      cmap_info_idx[1] = count[0];
    } /* End "if (cmap_info_idx[1] == -1) */

    num_e_comm_maps = cmap_info_idx[1] - cmap_info_idx[0];

    if (num_e_comm_maps > 0) {
      count[0] = num_e_comm_maps;

      /* Get the variable ID for the vector of nodal comm map IDs */
      if ((status = nc_inq_varid(exoid, VAR_E_COMM_IDS, &varid)) != NC_NOERR) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "ERROR: failed to get variable ID for \"%s\" in file ID %d", VAR_E_COMM_IDS,
                 exoid);
        ex_err_fn(exoid, __func__, errmsg, status);
        EX_FUNC_LEAVE(EX_FATAL);
      }

      /* Get the vector of elemental communication map IDs */
      if (elem_cmap_ids != NULL) {
        start[0] = cmap_info_idx[0];
        if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
          status = nc_get_vara_longlong(exoid, varid, start, count, elem_cmap_ids);
        }
        else {
          status = nc_get_vara_int(exoid, varid, start, count, elem_cmap_ids);
        }
        if (status != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get variable \"%s\" from file ID %d",
                   VAR_E_COMM_IDS, exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }

        if ((status = nc_inq_varid(exoid, VAR_E_COMM_STAT, &varid)) != NC_NOERR) {
          snprintf(errmsg, MAX_ERR_LENGTH,
                   "ERROR: failed to find variable ID for \"%s\" from file ID %d", VAR_E_COMM_STAT,
                   exoid);
          ex_err_fn(exoid, __func__, errmsg, status);
          EX_FUNC_LEAVE(EX_FATAL);
        }

        if (elem_cmap_elem_cnts != NULL) {
          /*
           * Get the element counts in each of the elemental
           * communication maps
           */
          for (cnt = 0; cnt < num_e_comm_maps; cnt++) {
            int64_t cmap_id;
            if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
              cmap_id = ((int64_t *)elem_cmap_ids)[cnt];
            }
            else {
              cmap_id = ((int *)elem_cmap_ids)[cnt];
            }

            if ((map_idx = ne__id_lkup(exoid, VAR_E_COMM_IDS, cmap_info_idx, cmap_id)) < 0) {
              snprintf(errmsg, MAX_ERR_LENGTH,
                       "ERROR: failed to find elemental comm map with ID %" PRId64 " in file ID %d",
                       cmap_id, exoid);
              ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
              EX_FUNC_LEAVE(EX_FATAL);
            }

            /* Check the status of the requested elemental map */
            start[0] = map_idx;
            if ((status = nc_get_var1_int(exoid, varid, start, &nmstat)) != NC_NOERR) {
              snprintf(errmsg, MAX_ERR_LENGTH,
                       "ERROR: failed to get status for \"%s\" from file ID %d", VAR_E_COMM_STAT,
                       exoid);
              ex_err_fn(exoid, __func__, errmsg, status);
              EX_FUNC_LEAVE(EX_FATAL);
            }

            if (nmstat == 1) {

              /* get the cmap information variables index */
              if (ex_get_idx(exoid, VAR_E_COMM_DATA_IDX, cmap_data_idx, map_idx) == -1) {
                snprintf(errmsg, MAX_ERR_LENGTH,
                         "ERROR: failed to find index variable, \"%s\", "
                         "in file ID %d",
                         VAR_E_COMM_DATA_IDX, exoid);
                ex_err_fn(exoid, __func__, errmsg, status);

                EX_FUNC_LEAVE(EX_FATAL);
              }

              if (cmap_data_idx[1] == -1) {
                /*
                 * Find the dimension ID of the variable containing the
                 * element count
                 */
                if ((status = nc_inq_dimid(exoid, DIM_ECNT_CMAP, &dimid)) != NC_NOERR) {
                  snprintf(errmsg, MAX_ERR_LENGTH,
                           "ERROR: failed to find dimension ID for "
                           "\"%s\" in file ID %d",
                           DIM_ECNT_CMAP, exoid);
                  ex_err_fn(exoid, __func__, errmsg, status);
                  EX_FUNC_LEAVE(EX_FATAL);
                }

                /*
                 * Find the value of the number of elements in this elemental
                 * comm map
                 */
                if ((status = nc_inq_dimlen(exoid, dimid, count)) != NC_NOERR) {
                  snprintf(errmsg, MAX_ERR_LENGTH,
                           "ERROR: failed to find length of dimension "
                           "\"%s\" in file ID %d",
                           DIM_ECNT_CMAP, exoid);
                  ex_err_fn(exoid, __func__, errmsg, status);
                  EX_FUNC_LEAVE(EX_FATAL);
                }
                cmap_data_idx[1] = count[0];
              }
              if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
                ((int64_t *)elem_cmap_elem_cnts)[cnt] = cmap_data_idx[1] - cmap_data_idx[0];
              }
              else {
                ((int *)elem_cmap_elem_cnts)[cnt] = cmap_data_idx[1] - cmap_data_idx[0];
              }
            }
            else if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
              ((int64_t *)elem_cmap_elem_cnts)[cnt] = 0;
            }
            else {
              ((int *)elem_cmap_elem_cnts)[cnt] = 0;
            }
          } /* "for(cnt=0; cnt < num_e_comm_maps; cnt++)" */
        }   /* "if (elem_cmap_elem_cnts != NULL)" */
      }     /* "if (elem_cmap_ids != NULL)" */
    }       /* "if (num_e_comm_maps > 0)" */
  }         /* End "if ((dimid = nc_inq_dimid(exoid, DIM_NUM_E_CMAPS(processor))) !=
               -1)" */
  EX_FUNC_LEAVE(EX_NOERR);
}

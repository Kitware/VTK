/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
*
* expssd - ex_put_set_dist_fact
*
* entry conditions -
*   input parameters:
*       int     exoid                   exodus file id
*       int     set_type                set type
*       int     set_id                  set id
*       void*   set_dist_fact           array of dist factors for set

* exit conditions -
*
* revision history -
*
*
*****************************************************************************/

#include "exodusII.h"     // for ex_err, ex_name_of_object, etc
#include "exodusII_int.h" // for EX_FATAL, EX_WARN, etc

/*!
 * writes the distribution factors for a single set
 * \param  exoid                   exodus file id
 * \param  set_type                set type
 * \param  set_id                  set id
 * \param *set_dist_fact           array of dist factors for set
 */

int ex_put_set_dist_fact(int exoid, ex_entity_type set_type, ex_entity_id set_id,
                         const void *set_dist_fact)
{
  int   status;
  int   dimid, set_id_ndx;
  int   dist_id;
  char  errmsg[MAX_ERR_LENGTH];
  char *factptr = NULL;

  EX_FUNC_ENTER();

  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* first check if any sets are specified */
  if ((status = nc_inq_dimid(exoid, exi_dim_num_objects(set_type), &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: no %ss specified in file id %d",
             ex_name_of_object(set_type), exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Lookup index of set id in VAR_*S_IDS array */
  set_id_ndx = exi_id_lkup(exoid, set_type, set_id);
  if (set_id_ndx <= 0) {
    ex_get_err(NULL, NULL, &status);

    if (status != 0) {
      if (status == EX_NULLENTITY) {
        snprintf(errmsg, MAX_ERR_LENGTH,
                 "Warning: no data allowed for NULL %s %" PRId64 " in file id %d",
                 ex_name_of_object(set_type), set_id, exoid);
        ex_err_fn(exoid, __func__, errmsg, EX_NULLENTITY);
        EX_FUNC_LEAVE(EX_WARN);
      }
      snprintf(errmsg, MAX_ERR_LENGTH,
               "ERROR: failed to locate %s id %" PRId64 " in VAR_*S_IDS array in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* setup more pointers based on set_type */
  if (set_type == EX_NODE_SET) {
    /* note we are using DIM_NUM_NODE_NS instead of DIM_NUM_DF_NS */
    factptr = VAR_FACT_NS(set_id_ndx);
  }
  else if (set_type == EX_EDGE_SET) {
    factptr = VAR_FACT_ES(set_id_ndx);
  }
  else if (set_type == EX_FACE_SET) {
    factptr = VAR_FACT_FS(set_id_ndx);
  }
  else if (set_type == EX_SIDE_SET) {
    factptr = VAR_FACT_SS(set_id_ndx);
  }
  if (set_type == EX_ELEM_SET) {
    factptr = VAR_FACT_ELS(set_id_ndx);
  }

  /* find id of distribution factors variable
   */

  if ((status = nc_inq_varid(exoid, factptr, &dist_id)) != NC_NOERR) {
    /* this test is only needed for node set because we're using
       DIM_NUM_NOD_NS instead of  DIM_NUM_DF_NS*/
    if (status == NC_ENOTVAR) {
      snprintf(errmsg, MAX_ERR_LENGTH,
               "Warning: no dist factors defined for %s %" PRId64 " in file id %d",
               ex_name_of_object(set_type), set_id, exoid);
      ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);
      EX_FUNC_LEAVE(EX_WARN);
    }
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to locate dist factors list for %s %" PRId64 " in file id %d",
             ex_name_of_object(set_type), set_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* write out the distribution factors array */
  if (exi_comp_ws(exoid) == 4) {
    status = nc_put_var_float(exoid, dist_id, set_dist_fact);
  }
  else {
    status = nc_put_var_double(exoid, dist_id, set_dist_fact);
  }

  if (status != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to store dist factors for %s %" PRId64 " in file id %d",
             ex_name_of_object(set_type), set_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

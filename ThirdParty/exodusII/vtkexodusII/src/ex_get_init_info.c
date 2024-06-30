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
 * 	ex_get_init_info()
 *****************************************************************************
 * This function reads information about the processors for which the
 * decomposition was performed.
 *****************************************************************************
 * Variable Index:
 *	exoid		  - The NetCDF ID of an already open NemesisI file.
 *	num_proc	  - The number of processors in the decomposition.
 *	num_proc_in_f	  - The number of processors the file contains
 *			    information for.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>     // for ex_err, EX_MSG, etc
#include <exodusII_int.h> // for EX_FATAL, DIM_NUM_PROCS, etc

/*!
 * \ingroup ModelDescription
 * \undoc
 */
int ex_get_init_info(int exoid, int *num_proc, int *num_proc_in_f, char *ftype)
{
  int    dimid, status;
  size_t ltempsv;

  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* In case file isn't parallel, set the values here... */
  *num_proc      = 1;
  *num_proc_in_f = 1;

  /* Get the file type */
  if (exi_get_file_type(exoid, ftype) != EX_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get file type for file ID %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (nc_inq_dimid(exoid, DIM_NUM_PROCS, &dimid) != NC_NOERR) {
    /* This isn't a parallel file.  Just return now with no error, but with num_proc and
     * num_proc_in_f set to 1 */
    EX_FUNC_LEAVE(EX_NOERR);
  }

  /* Get the value of the number of processors */
  if ((status = nc_inq_dimlen(exoid, dimid, &ltempsv)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NUM_PROCS,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }
  *num_proc = ltempsv;

  /* Get the dimension ID of processors that have info in this file */
  if ((status = nc_inq_dimid(exoid, DIM_NUM_PROCS_F, &dimid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find dimension ID for \"%s\" in file ID %d",
             DIM_NUM_PROCS_F, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Get the value of the number of processors that have info in this file */
  if ((status = nc_inq_dimlen(exoid, dimid, &ltempsv)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to find length of dimension \"%s\" in file ID %d", DIM_NUM_PROCS_F,
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }
  *num_proc_in_f = ltempsv;

  EX_FUNC_LEAVE(EX_NOERR);
}

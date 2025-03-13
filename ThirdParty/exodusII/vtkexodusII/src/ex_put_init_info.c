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
 *      ex_put_init_info()
 *****************************************************************************
 * This function writes information about the processors for which the
 * decomposition was performed.
 *****************************************************************************
 * Variable Index:
 *      exoid             - The NetCDF ID of an already open NemesisI file.
 *      num_proc          - The number of processors in the decomposition.
 *      num_proc_in_f     - The number of processors the file contains
 *                          information for.
 *      ftype             - The type of Nemesis file.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#include <exodusII.h>
#include <exodusII_int.h>

/*!
 * \ingroup ModelDescription
 * \undoc
 */
int ex_put_init_info(int exoid, int num_proc, int num_proc_in_f, const char *ftype)
{
  int  dimid, varid;
  int  ltempsv;
  int  lftype;
  int  status;
  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Check the file type */
  if (!ftype) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: NULL file type input for file ID %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Set the file type */
  if (ftype[0] == 'p' || ftype[0] == 'P') {
    lftype = 0;
  }
  else if (ftype[0] == 's' || ftype[0] == 'S') {
    lftype = 1;
  }
  else {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unknown file type requested for file ID %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Put file into define mode */
  if ((status = exi_redef(exoid, __func__)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file ID %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Define dimension for the number of processors */
  if (nc_inq_dimid(exoid, DIM_NUM_PROCS, &dimid) != NC_NOERR) {
    ltempsv = num_proc;
    if ((status = nc_def_dim(exoid, DIM_NUM_PROCS, ltempsv, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file ID %d",
               DIM_NUM_PROCS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* If this is a parallel file then the status vectors are size 1 */
  if (nc_inq_dimid(exoid, DIM_NUM_PROCS_F, &dimid) != NC_NOERR) {
    ltempsv = num_proc_in_f;
    if ((status = nc_def_dim(exoid, DIM_NUM_PROCS_F, ltempsv, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file ID %d",
               DIM_NUM_PROCS_F, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* Output the file type */
  if (nc_inq_varid(exoid, VAR_FILE_TYPE, &varid) != NC_NOERR) {
    if ((status = nc_def_var(exoid, VAR_FILE_TYPE, NC_INT, 0, NULL, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define file type in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      /* Leave define mode before returning */
      exi_leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (exi_leavedef(exoid, __func__) != EX_NOERR) {
      EX_FUNC_LEAVE(EX_FATAL);
    }

    if ((status = nc_put_var1_int(exoid, varid, NULL, &lftype)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: unable to output file type variable in file ID %d",
               exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }
  else {
    if (exi_leavedef(exoid, __func__) != EX_NOERR) {
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

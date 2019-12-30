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
int ex_put_init_info(int exoid, int num_proc, int num_proc_in_f, char *ftype)
{
  int  dimid, varid;
  int  ltempsv;
  int  lftype;
  int  status;
  char errmsg[MAX_ERR_LENGTH];
  /*-----------------------------Execution begins-----------------------------*/

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

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
  if ((status = nc_redef(exoid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to put file ID %d into define mode", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Define dimension for the number of processors */
  if ((status = nc_inq_dimid(exoid, DIM_NUM_PROCS, &dimid)) != NC_NOERR) {
    ltempsv = num_proc;
    if ((status = nc_def_dim(exoid, DIM_NUM_PROCS, ltempsv, &dimid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to dimension \"%s\" in file ID %d",
               DIM_NUM_PROCS, exoid);
      ex_err_fn(exoid, __func__, errmsg, status);
      /* Leave define mode before returning */
      ex__leavedef(exoid, __func__);

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
      ex__leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  /* Output the file type */
  if (nc_inq_varid(exoid, VAR_FILE_TYPE, &varid) != NC_NOERR) {
    if ((status = nc_def_var(exoid, VAR_FILE_TYPE, NC_INT, 0, NULL, &varid)) != NC_NOERR) {
      snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to define file type in file ID %d", exoid);
      ex_err_fn(exoid, __func__, errmsg, status);

      /* Leave define mode before returning */
      ex__leavedef(exoid, __func__);

      EX_FUNC_LEAVE(EX_FATAL);
    }

    if (ex__leavedef(exoid, __func__) != EX_NOERR) {
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
    if (ex__leavedef(exoid, __func__) != EX_NOERR) {
      EX_FUNC_LEAVE(EX_FATAL);
    }
  }

  EX_FUNC_LEAVE(EX_NOERR);
}

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
 *
 *      ex_get_elem_type()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      elem_blk_id        - The element block ID to find the element type
 *                           for.
 *      elem_type          - The returned name of the element.
 *
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h"     // for ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, ATT_NAME_ELB, etc

int ex_get_elem_type(int exoid, ex_entity_id elem_blk_id, char *elem_type)
/*
 *      Reads the element type for a specific element block
 *           elem_type is assumed to have a length of MAX_STR_LENGTH+1
 */
{
  int    connid, el_blk_id_ndx, status;
  size_t len;
  char   errmsg[MAX_ERR_LENGTH];

  /*****************************************************************************/

  EX_FUNC_ENTER();
  ex__check_valid_file_id(exoid, __func__);

  /* inquire id's of previously defined dimensions */
  if ((el_blk_id_ndx = ex__id_lkup(exoid, EX_ELEM_BLOCK, elem_blk_id)) == -1) {
    snprintf(errmsg, MAX_ERR_LENGTH,
             "ERROR: failed to find element block ID %" PRId64 " in file %d", elem_blk_id, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_LASTERR);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  if ((status = nc_inq_varid(exoid, VAR_CONN(el_blk_id_ndx), &connid)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find connectivity variable in file ID %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, status);
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* get the element type name */
  if ((status = nc_inq_attlen(exoid, connid, ATT_NAME_ELB, &len)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to find attribute in file ID %d", exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  if (len > (MAX_STR_LENGTH + 1)) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: Element type must be of length %d in file ID %d",
             (int)len, exoid);
    ex_err_fn(exoid, __func__, errmsg, EX_BADPARAM);

    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* Make sure the end of the string is terminated with a null character */
  elem_type[MAX_STR_LENGTH] = '\0';

  if ((status = nc_get_att_text(exoid, connid, ATT_NAME_ELB, elem_type)) != NC_NOERR) {
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to get attribute \"%s\" in file ID %d",
             ATT_NAME_ELB, exoid);
    ex_err_fn(exoid, __func__, errmsg, status);

    EX_FUNC_LEAVE(EX_FATAL);
  }
  EX_FUNC_LEAVE(EX_NOERR);
}

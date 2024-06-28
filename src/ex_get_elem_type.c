/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
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
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

  /* inquire id's of previously defined dimensions */
  if ((el_blk_id_ndx = exi_id_lkup(exoid, EX_ELEM_BLOCK, elem_blk_id)) == -1) {
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

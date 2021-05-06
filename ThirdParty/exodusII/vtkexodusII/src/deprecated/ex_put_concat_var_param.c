/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expvp - ex_put_concat_var_param
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid   exodus file id
 *       int     num_g   global variable count
 *       int     num_n   nodal variable count
 *       int     num_e   element variable count
 *       int     num_elem_blk            number of element blocks (unused)
 *       int*    elem_var_tab            element variable truth table array
 *
 * exit conditions -
 *
 * revision history -
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_put_all_var_param
#include "exodusII_int.h"

/*!
 * writes the number of global, nodal, and element variables
 * that will be written to the database
 * \param      exoid           int             exodus file id
 * \param      num_g           int             global variable count
 * \param      num_n           int             nodal variable count
 * \param      num_e           int             element variable count
 * \param      num_elem_blk    int             number of element blocks
 * \param      elem_var_tab    int*            element variable truth table
 * array
 * \deprecated Use ex_put_all_var_param()(exoid, num_g, num_n, num_e,
 * elem_var_tab, 0, 0, 0, 0)
 */

int ex_put_concat_var_param(int exoid, int num_g, int num_n, int num_e,
                            int  num_elem_blk, /* unused */
                            int *elem_var_tab)
{
  EX_UNUSED(num_elem_blk);
  return ex_put_all_var_param(exoid, num_g, num_n, num_e, elem_var_tab, 0, 0, 0, 0);
}

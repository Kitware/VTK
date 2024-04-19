/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* Function(s) contained in this file:
 *
 *      ex_put_nodal_var_slab()
 *
 *****************************************************************************
 *
 *  Variable Index:
 *
 *      exoid               - The NetCDF ID of an already open NemesisI file.
 *      time_step          - The time step to write this data to.
 *      nodal_var_index    - The index of this nodal variable.
 *      start_pos          - The start point for outputting data. The first
 *                           value is 0.
 *      num_vals           - The number of values to be output.
 *      nodal_var_vals     - Pointer to the vector of values to be output.
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
#include "exodusII.h" // for ex_put_partial_var, etc

/*
 * writes the values of a single nodal variable for a single time step to
 * the database; assume the first time step and nodal variable index
 * is 1
 */

/*!
 * \ingroup ResultsData
 * \deprecated Use ex_put_partial_var()(exoid, time_step, EX_NODAL, nodal_var_index, 1, start_pos,
 num_vals, nodal_var_vals) instead.
 */
int ex_put_nodal_var_slab(int exoid, int time_step, int nodal_var_index, int64_t start_pos,
                          int64_t num_vals, void *nodal_var_vals)

{
  return ex_put_partial_var(exoid, time_step, EX_NODAL, nodal_var_index, 1, start_pos, num_vals,
                            nodal_var_vals);
}

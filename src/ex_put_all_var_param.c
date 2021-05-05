/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * expvp - ex_put_all_var_param
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid           exodus file id
 *       int     num_g           global variable count
 *       int     num_n           nodal variable count
 *       int     num_e           element variable count
 *       int*    elem_var_tab    element variable truth table array
 *       int     num_m           nodeset variable count
 *       int*    nset_var_tab    nodeset variable truth table array
 *       int     num_s           sideset variable count
 *       int*    sset_var_tab    sideset variable truth table array
 *
 * exit conditions -
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_var_params, etc

/*!
\ingroup ResultsData
 * defines the number of global, nodal, element, nodeset, and sideset variables
 * that will be written to the database
 *  \param     exoid           exodus file id
 *  \param     num_g           global variable count
 *  \param     num_n           nodal variable count
 *  \param     num_e           element variable count
 *  \param    *elem_var_tab    element variable truth table array
 *  \param     num_m           nodeset variable count
 *  \param    *nset_var_tab    nodeset variable truth table array
 *  \param     num_s           sideset variable count
 *  \param    *sset_var_tab    sideset variable truth table array
 */

int ex_put_all_var_param(int exoid, int num_g, int num_n, int num_e, int *elem_var_tab, int num_m,
                         int *nset_var_tab, int num_s, int *sset_var_tab)
{
  ex_var_params vparam;

  vparam.num_glob      = num_g;
  vparam.num_node      = num_n;
  vparam.num_edge      = 0;
  vparam.edge_var_tab  = 0;
  vparam.num_face      = 0;
  vparam.face_var_tab  = 0;
  vparam.num_elem      = num_e;
  vparam.elem_var_tab  = elem_var_tab;
  vparam.num_nset      = num_m;
  vparam.nset_var_tab  = nset_var_tab;
  vparam.num_eset      = 0;
  vparam.eset_var_tab  = 0;
  vparam.num_fset      = 0;
  vparam.fset_var_tab  = 0;
  vparam.num_sset      = num_s;
  vparam.sset_var_tab  = sset_var_tab;
  vparam.num_elset     = 0;
  vparam.elset_var_tab = 0;

  return ex_put_all_var_param_ext(exoid, &vparam);
}

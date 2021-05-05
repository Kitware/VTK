/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
/*****************************************************************************
 *
 * exgssi - ex_get_side_set_ids
 *
 * entry conditions -
 *   input parameters:
 *       int     exoid                   exodus file id
 *
 * exit conditions -
 *       int*    size_set_ids            array of side set IDs
 *
 * revision history -
 *
 *
 *****************************************************************************/

#include "exodusII.h" // for ex_get_ids, etc

/*!
 *  reads the side set ids from the database
 * \deprecated Use ex_get_ids()(exoid, EX_SIDE_SET, ids)
 */

int ex_get_side_set_ids(int exoid, void_int *ids) { return ex_get_ids(exoid, EX_SIDE_SET, ids); }

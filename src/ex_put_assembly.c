/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_assembly, ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

#include <stdbool.h>

/*!
 * writes the assembly parameters and optionally assembly data for one assembly
 * \param   exoid                   exodus file id
 * \param  *assembly                ex_assembly structure
 */

int ex_put_assembly(int exoid, const struct ex_assembly assembly)
{
  return ex_put_assemblies(exoid, 1, &assembly);
}

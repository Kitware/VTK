/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_blob, ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

#include <stdbool.h>

/*!
 * writes the blob parameters and optionally blob data for one blob
 * \param   exoid                   exodus file id
 * \param  *blob                ex_blob structure
 */

int ex_put_blob(int exoid, const struct ex_blob blob) { return ex_put_blobs(exoid, 1, &blob); }

/*
 * Copyright (c) 2005-2017, 2020 National Technology & Engineering Solutions
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

#include "exodusII.h"     // for ex_assembly, ex_err, etc
#include "exodusII_int.h" // for EX_FATAL, etc

/*!
 * writes the assembly parameters and optionally assembly data for all assemblies
 * assumes that `assembly` is large enough to contain all assemblies.
 * \param   exoid                   exodus file id
 * \param  *assembly                array of ex_assembly structures
 */
int ex_get_assemblies(int exoid, ex_assembly *assembly)
{
  /* Determine number of assemblies on database */
  int num_assembly        = ex_inquire_int(exoid, EX_INQ_ASSEMBLY);
  int max_use_name_length = ex_inquire_int(exoid, EX_INQ_DB_MAX_USED_NAME_LENGTH);

  if (num_assembly < 0) {
    char errmsg[MAX_ERR_LENGTH];
    snprintf(errmsg, MAX_ERR_LENGTH, "ERROR: failed to inquire ASSEMBLY count in file id %d",
             exoid);
    ex_err_fn(exoid, __func__, errmsg, num_assembly);
    return (EX_FATAL);
  }

  if (ex_int64_status(exoid) & EX_IDS_INT64_API) {
    int64_t *ids = calloc(num_assembly, sizeof(int64_t));
    ex_get_ids(exoid, EX_ASSEMBLY, ids);
    for (int i = 0; i < num_assembly; i++) {
      assembly[i].id = ids[i];
    }
    free(ids);
  }
  else {
    int *ids = calloc(num_assembly, sizeof(int));
    ex_get_ids(exoid, EX_ASSEMBLY, ids);
    for (int i = 0; i < num_assembly; i++) {
      assembly[i].id = ids[i];
    }
    free(ids);
  }

  for (int i = 0; i < num_assembly; i++) {
    if (assembly[i].name == NULL) {
      assembly[i].name = calloc(max_use_name_length + 1, sizeof(char));
    }

    int status = ex_get_assembly(exoid, &assembly[i]);
    if (status != EX_NOERR) {
      return status;
    }
  }
  return EX_NOERR;
}

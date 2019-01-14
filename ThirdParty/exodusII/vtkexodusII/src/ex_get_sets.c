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

#include "exodusII.h"     // for ex_set, EX_FATAL, EX_NOERR, etc
#include "exodusII_int.h" // for ex_check_valid_file_id
#include <stddef.h>       // for NULL, size_t

int ex_get_sets(int exoid, size_t set_count, struct ex_set *sets)
{
  size_t i;
  int    status = EX_NOERR;
  int    stat;

  EX_FUNC_ENTER();
  ex_check_valid_file_id(exoid, __func__);

  for (i = 0; i < set_count; i++) {
    if (ex_int64_status(exoid) & EX_BULK_INT64_API) {
      stat = ex_get_set_param(exoid, sets[i].type, sets[i].id, &sets[i].num_entry,
                              &sets[i].num_distribution_factor);
    }
    else {
      /* API expecting 32-bit ints; ex_set structure has 64-bit ints. */
      int num_entry;
      int num_dist;
      stat              = ex_get_set_param(exoid, sets[i].type, sets[i].id, &num_entry, &num_dist);
      sets[i].num_entry = num_entry;
      sets[i].num_distribution_factor = num_dist;
    }
    if (stat != EX_NOERR) {
      status = (status == EX_FATAL) ? EX_FATAL : stat;
    }

    if (stat == EX_NOERR && (sets[i].entry_list != NULL || sets[i].extra_list != NULL)) {
      stat = ex_get_set(exoid, sets[i].type, sets[i].id, sets[i].entry_list, sets[i].extra_list);
      if (stat != EX_NOERR) {
        status = (status == EX_FATAL) ? EX_FATAL : stat;
      }
    }

    if (stat == EX_NOERR && sets[i].distribution_factor_list != NULL) {
      stat =
          ex_get_set_dist_fact(exoid, sets[i].type, sets[i].id, sets[i].distribution_factor_list);
      if (stat != EX_NOERR) {
        status = (status == EX_FATAL) ? EX_FATAL : stat;
      }
    }
  }
  EX_FUNC_LEAVE(status);
}

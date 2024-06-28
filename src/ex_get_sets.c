/*
 * Copyright(C) 1999-2020 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include "exodusII.h"     // for ex_set, EX_FATAL, EX_NOERR, etc
#include "exodusII_int.h" // for exi_check_valid_file_id

int ex_get_sets(int exoid, size_t set_count, struct ex_set *sets)
{
  size_t i;
  int    status = EX_NOERR;
  int    stat;

  EX_FUNC_ENTER();
  if (exi_check_valid_file_id(exoid, __func__) == EX_FATAL) {
    EX_FUNC_LEAVE(EX_FATAL);
  }

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

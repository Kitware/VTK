/* Author:  Lisandro Dalcin   */
/* Contact: dalcinl@gmail.com */

#ifndef MPI4PY_H
#define MPI4PY_H

#include "mpi.h"

#include "mpi4py.MPI_api.h"

static int import_mpi4py(void) {
  if (import_mpi4py__MPI() < 0) goto bad;
  return 0;
 bad:
  return -1;
}

#endif /* MPI4PY_H */

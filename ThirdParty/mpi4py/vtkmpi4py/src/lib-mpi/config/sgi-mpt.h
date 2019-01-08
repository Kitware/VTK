#ifndef PyMPI_CONFIG_SGI_MPT_H
#define PyMPI_CONFIG_SGI_MPT_H

/* ------------------------------------------------------------------------- */

#include "mpi-11.h"
#include "mpi-12.h"
#include "mpi-20.h"
#include "mpi-22.h"
#include "mpi-30.h"
#include "mpi-31.h"

/* ------------------------------------------------------------------------- */

/* These types are difficult to implement portably */
#undef PyMPI_HAVE_MPI_REAL2
#undef PyMPI_HAVE_MPI_COMPLEX4

/* SGI MPT doesn't have this function. */
#undef PyMPI_HAVE_MPI_CONVERSION_FN_NULL

/* ------------------------------------------------------------------------- */

#endif /* !PyMPI_CONFIG_SGI_MPT_H */

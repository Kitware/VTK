#ifndef PyMPI_CONFIG_SGI_MPT_H
#define PyMPI_CONFIG_SGI_MPT_H

/* ------------------------------------------------------------------------- */

#include "mpiapi.h"

/* ------------------------------------------------------------------------- */

/* These types are difficult to implement portably */
#undef PyMPI_HAVE_MPI_REAL2
#undef PyMPI_HAVE_MPI_COMPLEX4

/* SGI MPT doesn't have this function. */
#undef PyMPI_HAVE_MPI_CONVERSION_FN_NULL

/* ------------------------------------------------------------------------- */

#endif /* !PyMPI_CONFIG_SGI_MPT_H */

#ifndef PyMPI_CONFIG_UNKNOWN_H
#define PyMPI_CONFIG_UNKNOWN_H

#include "mpiapi.h"

/* These types are difficult to implement portably */
#ifndef MPI_REAL2
#undef PyMPI_HAVE_MPI_REAL2
#endif
#ifndef MPI_COMPLEX4
#undef PyMPI_HAVE_MPI_COMPLEX4
#endif
#ifndef MPI_INTEGER16
#undef PyMPI_HAVE_MPI_INTEGER16
#endif

#endif /* !PyMPI_CONFIG_UNKNOWN_H */

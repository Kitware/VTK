#ifndef PyMPI_CONFIG_MPICH3_H
#define PyMPI_CONFIG_MPICH3_H

#include "mpiapi.h"

/* These types may not be available */
#ifndef MPI_REAL2
#undef PyMPI_HAVE_MPI_REAL2
#endif
#ifndef MPI_MPI_COMPLEX4
#undef PyMPI_HAVE_MPI_COMPLEX4
#endif

/* MPI I/O may not be available */
#ifndef ROMIO_VERSION
#include "mpi-io.h"
#endif

#endif /* !PyMPI_CONFIG_MPICH3_H */

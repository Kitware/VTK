#ifndef PyMPI_COMPAT_MPICH2_H
#define PyMPI_COMPAT_MPICH2_H

#if defined(__SICORTEX__)
#include "sicortex.h"
#endif

#if defined(MS_MPI)
#undef  MPI_File_c2f
#define MPI_File_c2f PMPI_File_c2f
#undef  MPI_File_f2c
#define MPI_File_f2c PMPI_File_f2c
#endif

#endif /* !PyMPI_COMPAT_MPICH2_H */

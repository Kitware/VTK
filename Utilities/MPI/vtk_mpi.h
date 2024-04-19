// Skip MPICH's C++ support.
#ifndef MPICH_SKIP_MPICXX
#define MPICH_SKIP_MPICXX
#define _vtk_mpi_mpich
#endif

// Skip OpenMPI's C++ support.
#ifndef OMPI_SKIP_MPICXX
#define OMPI_SKIP_MPICXX
#define _vtk_mpi_openmpi
#endif

// Skip the IBM's MPI C++ support.
#ifndef _MPICC_H
#define _MPICC_H
#define _vtk_mpi_ibm
#endif

// Skip SGI MPT's C++ support.
#ifndef MPI_NO_CPPBIND
#define MPI_NO_CPPBIND
#define _vtk_mpi_sgi
#endif

// Include the MPI header.
#include <mpi.h>

// Cleanup what we set up.
#ifdef _vtk_mpi_mpich
#undef MPICH_SKIP_MPICXX
#undef _vtk_mpi_mpich
#endif

#ifdef _vtk_mpi_openmpi
#undef OMPI_SKIP_MPICXX
#undef _vtk_mpi_openmpi
#endif

#ifdef _vtk_mpi_ibm
#undef _MPICC_H
#undef _vtk_mpi_ibm
#endif

#ifdef _vtk_mpi_sgi
#undef MPI_NO_CPPBIND
#undef _vtk_mpi_sgi
#endif

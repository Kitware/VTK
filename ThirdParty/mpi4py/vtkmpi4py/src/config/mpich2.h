#ifndef PyMPI_CONFIG_MPICH2_H
#define PyMPI_CONFIG_MPICH2_H

#if MPI_VERSION==2 && MPI_SUBVERSION<2
#define PyMPI_MISSING_MPI_AINT 1
#define PyMPI_MISSING_MPI_OFFSET 1
#define PyMPI_MISSING_MPI_C_BOOL 1
#define PyMPI_MISSING_MPI_INT8_T 1
#define PyMPI_MISSING_MPI_INT16_T 1
#define PyMPI_MISSING_MPI_INT32_T 1
#define PyMPI_MISSING_MPI_INT64_T 1
#define PyMPI_MISSING_MPI_UINT8_T 1
#define PyMPI_MISSING_MPI_UINT16_T 1
#define PyMPI_MISSING_MPI_UINT32_T 1
#define PyMPI_MISSING_MPI_UINT64_T 1
#define PyMPI_MISSING_MPI_C_COMPLEX 1
#define PyMPI_MISSING_MPI_C_FLOAT_COMPLEX 1
#define PyMPI_MISSING_MPI_C_DOUBLE_COMPLEX 1
#define PyMPI_MISSING_MPI_C_LONG_DOUBLE_COMPLEX 1
#endif

#define PyMPI_MISSING_MPI_LOGICAL1 1
#define PyMPI_MISSING_MPI_LOGICAL2 1
#define PyMPI_MISSING_MPI_LOGICAL4 1
#define PyMPI_MISSING_MPI_LOGICAL8 1

#define PyMPI_MISSING_MPI_REAL2 1
#define PyMPI_MISSING_MPI_COMPLEX4 1

#if MPI_VERSION==2 && MPI_SUBVERSION<2
#define PyMPI_MISSING_MPI_Op_commutative 1
#define PyMPI_MISSING_MPI_Reduce_local 1
#define PyMPI_MISSING_MPI_Reduce_scatter_block 1
#endif

#if MPI_VERSION==2 && MPI_SUBVERSION<2
#define PyMPI_MISSING_MPI_DIST_GRAPH 1
#define PyMPI_MISSING_MPI_UNWEIGHTED 1
#define PyMPI_MISSING_MPI_Dist_graph_create_adjacent 1
#define PyMPI_MISSING_MPI_Dist_graph_create 1
#define PyMPI_MISSING_MPI_Dist_graph_neighbors_count 1
#define PyMPI_MISSING_MPI_Dist_graph_neighbors 1
#endif

#if MPI_VERSION==2 && MPI_SUBVERSION<2
#define PyMPI_MISSING_MPI_Comm_errhandler_function 1
#define PyMPI_MISSING_MPI_Win_errhandler_function 1
#define PyMPI_MISSING_MPI_File_errhandler_function 1
#endif

#if defined(MPI_UNWEIGHTED) && (MPICH2_NUMVERSION < 10300000)
#undef  MPI_UNWEIGHTED
#define MPI_UNWEIGHTED ((int *)0)
#endif

#if !defined(MPICH2_NUMVERSION) || (MPICH2_NUMVERSION < 10100000)
#define PyMPI_MISSING_MPI_Type_create_f90_integer 1
#define PyMPI_MISSING_MPI_Type_create_f90_real 1
#define PyMPI_MISSING_MPI_Type_create_f90_complex 1
#endif /* MPICH2 < 1.1.0 */

#ifndef ROMIO_VERSION
#include "mpich2io.h"
#endif /* !ROMIO_VERSION */

#endif /* !PyMPI_CONFIG_MPICH2_H */

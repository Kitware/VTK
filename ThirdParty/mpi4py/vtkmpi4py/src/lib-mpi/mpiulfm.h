#ifndef PyMPI_MPIULFM_H
#define PyMPI_MPIULFM_H

#ifndef PyMPI_SKIP_MPIULFM
#define PyMPI_SKIP_MPIULFM 0
#endif

#if MPI_VERSION < 5 && !PyMPI_SKIP_MPIULFM

#if defined(MPICH_NAME) && (MPICH_NAME >= 3)
#if MPICH_NUMVERSION >= 30200000 && 0
#define PyMPI_HAVE_MPIX_ERR_REVOKED 1
#define PyMPI_HAVE_MPIX_ERR_PROC_FAILED 1
#define PyMPI_HAVE_MPIX_ERR_PROC_FAILED_PENDING 1
#define PyMPI_HAVE_MPIX_Comm_revoke 1
#define PyMPI_HAVE_MPIX_Comm_agree 1
#define PyMPI_HAVE_MPIX_Comm_shrink 1
#endif
#if MPICH_NUMVERSION >= 40200000 && 0
#define PyMPI_HAVE_MPIX_Comm_get_failed 1
#define PyMPI_HAVE_MPIX_Comm_ack_failed 1
#define PyMPI_HAVE_MPIX_Comm_iagree 1
#endif
#endif

#if defined(OPEN_MPI)
#include <mpi-ext.h>
#ifdef OMPI_HAVE_MPI_EXT_FTMPI
#define PyMPI_HAVE_MPIX_ERR_REVOKED 1
#define PyMPI_HAVE_MPIX_ERR_PROC_FAILED 1
#define PyMPI_HAVE_MPIX_ERR_PROC_FAILED_PENDING 1
#define PyMPI_HAVE_MPIX_Comm_revoke 1
#define PyMPI_HAVE_MPIX_Comm_is_revoked 1
#define PyMPI_HAVE_MPIX_Comm_agree 1
#define PyMPI_HAVE_MPIX_Comm_iagree 1
#define PyMPI_HAVE_MPIX_Comm_shrink 1
#ifdef  OMPI_HAVE_MPIX_COMM_GET_FAILED
#define PyMPI_HAVE_MPIX_Comm_get_failed 1
#endif
#ifdef  OMPI_HAVE_MPIX_COMM_ACK_FAILED
#define PyMPI_HAVE_MPIX_Comm_ack_failed 1
#endif
#ifdef  OMPI_HAVE_MPIX_COMM_ISHRINK
#define PyMPI_HAVE_MPIX_Comm_ishrink 1
#endif
#endif
#endif

#endif


#ifndef PyMPI_HAVE_MPI_ERR_REVOKED
#ifdef  PyMPI_HAVE_MPIX_ERR_REVOKED
#undef  MPI_ERR_REVOKED
#define MPI_ERR_REVOKED MPIX_ERR_REVOKED
#endif
#endif

#ifndef PyMPI_HAVE_MPI_ERR_PROC_FAILED
#ifdef  PyMPI_HAVE_MPIX_ERR_PROC_FAILED
#undef  MPI_ERR_PROC_FAILED
#define MPI_ERR_PROC_FAILED MPIX_ERR_PROC_FAILED
#endif
#endif

#ifndef PyMPI_HAVE_MPI_ERR_PROC_FAILED_PENDING
#ifdef  PyMPI_HAVE_MPIX_ERR_PROC_FAILED_PENDING
#undef  MPI_ERR_PROC_FAILED_PENDING
#define MPI_ERR_PROC_FAILED_PENDING MPIX_ERR_PROC_FAILED_PENDING
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_revoke
#ifdef  PyMPI_HAVE_MPIX_Comm_revoke
#undef  MPI_Comm_revoke
#define MPI_Comm_revoke MPIX_Comm_revoke
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_is_revoked
#ifdef  PyMPI_HAVE_MPIX_Comm_is_revoked
#undef  MPI_Comm_is_revoked
#define MPI_Comm_is_revoked MPIX_Comm_is_revoked
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_get_failed
#ifdef  PyMPI_HAVE_MPIX_Comm_get_failed
#undef  MPI_Comm_get_failed
#define MPI_Comm_get_failed MPIX_Comm_get_failed
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_ack_failed
#ifdef  PyMPI_HAVE_MPIX_Comm_ack_failed
#undef  MPI_Comm_ack_failed
#define MPI_Comm_ack_failed MPIX_Comm_ack_failed
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_agree
#ifdef  PyMPI_HAVE_MPIX_Comm_agree
#undef  MPI_Comm_agree
#define MPI_Comm_agree MPIX_Comm_agree
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_iagree
#ifdef  PyMPI_HAVE_MPIX_Comm_iagree
#undef  MPI_Comm_iagree
#define MPI_Comm_iagree MPIX_Comm_iagree
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_shrink
#ifdef  PyMPI_HAVE_MPIX_Comm_shrink
#undef  MPI_Comm_shrink
#define MPI_Comm_shrink MPIX_Comm_shrink
#endif
#endif

#ifndef PyMPI_HAVE_MPI_Comm_ishrink
#ifdef  PyMPI_HAVE_MPIX_Comm_ishrink
#undef  MPI_Comm_ishrink
#define MPI_Comm_ishrink MPIX_Comm_ishrink
#endif
#endif


#endif /* !PyMPI_MPIULFM_H */

#if defined(MPI_VERSION)
#if (MPI_VERSION >= 5)

#define PyMPI_HAVE_MPI_ERR_REVOKED 1
#define PyMPI_HAVE_MPI_ERR_PROC_FAILED 1
#define PyMPI_HAVE_MPI_ERR_PROC_FAILED_PENDING 1
#define PyMPI_HAVE_MPI_Comm_revoke 1
#define PyMPI_HAVE_MPI_Comm_is_revoked 1
#define PyMPI_HAVE_MPI_Comm_get_failed 1
#define PyMPI_HAVE_MPI_Comm_ack_failed 1
#define PyMPI_HAVE_MPI_Comm_agree 1
#define PyMPI_HAVE_MPI_Comm_iagree 1
#define PyMPI_HAVE_MPI_Comm_shrink 1

#endif
#endif

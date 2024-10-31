#ifndef PyMPI_COMPAT_IMPI_H
#define PyMPI_COMPAT_IMPI_H

/* -------------------------------------------------------------------------- */

static int PyMPI_IMPI_MPI_Initialized(int *flag)
{
  int ierr;
  ierr = MPI_Initialized(flag); if (ierr) return ierr;
  if (!flag || *flag) return MPI_SUCCESS;
  ierr = MPI_Finalized(flag); if (ierr) return ierr;
  return MPI_SUCCESS;
}
#define MPI_Initialized PyMPI_IMPI_MPI_Initialized

/* -------------------------------------------------------------------------- */

/* https://github.com/mpi4py/mpi4py/issues/418#issuecomment-2026805886 */

#if I_MPI_NUMVERSION == 20211200300
#undef  MPI_Status_c2f
#define MPI_Status_c2f PMPI_Status_c2f
#undef  MPI_Status_f2c
#define MPI_Status_f2c PMPI_Status_f2c
#endif

/* -------------------------------------------------------------------------- */

#endif /* !PyMPI_COMPAT_IMPI_H */

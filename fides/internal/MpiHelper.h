#ifndef fides_internal_MpiHelper_h
#define fides_internal_MpiHelper_h

/// @brief The FIDES_SAFE_MPI macro safely executes an MPI call only if the provided
/// communicator is valid.
///
/// WARNING: If Fides is built without MPI, OR if the communicator is MPI_COMM_NULL,
/// the MPI call is elided. In that case, any output variables passed to the function
/// will not be updated. To prevent undefined behavior, all output variables passed
/// to this macro must be initialized to appropriate serial fallback values prior to
/// invocation.

#if FIDES_USE_MPI
#include <fides/fides_mpi.h>
#define FIDES_SAFE_MPI(comm, cmd) \
  do                              \
  {                               \
    if ((comm) != MPI_COMM_NULL)  \
    {                             \
      cmd;                        \
    }                             \
  } while (false)

#else
#define FIDES_SAFE_MPI(comm, cmd) /* no-op */
#endif

#endif // fides_internal_MpiHelper_h

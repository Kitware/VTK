#ifndef DIY_MPI_HPP
#define DIY_MPI_HPP

#ifndef DIY_NO_MPI
#include <vtk_mpi.h>
#else
#include "mpi/no-mpi.hpp"
#endif

#include "mpi/constants.hpp"
#include "mpi/datatypes.hpp"
#include "mpi/optional.hpp"
#include "mpi/status.hpp"
#include "mpi/request.hpp"
#include "mpi/point-to-point.hpp"
#include "mpi/communicator.hpp"
#include "mpi/collectives.hpp"
#include "mpi/io.hpp"
#include "mpi/window.hpp"

namespace diy
{
namespace mpi
{

//! \ingroup MPI
struct environment
{
  inline environment(int threading = MPI_THREAD_FUNNELED);
  inline environment(int argc, char* argv[], int threading = MPI_THREAD_FUNNELED);
  inline ~environment();

  int   threading() const           { return provided_threading; }

  int   provided_threading;
};

}
}

diy::mpi::environment::
environment(int threading)
{
#ifndef DIY_NO_MPI
  int argc = 0; char** argv;
  MPI_Init_thread(&argc, &argv, threading, &provided_threading);
#else
  provided_threading = threading;
#endif
}

diy::mpi::environment::
environment(int argc, char* argv[], int threading)
{
#ifndef DIY_NO_MPI
  MPI_Init_thread(&argc, &argv, threading, &provided_threading);
#else
  (void) argc; (void) argv;
  provided_threading = threading;
#endif
}

diy::mpi::environment::
~environment()
{
#ifndef DIY_NO_MPI
  MPI_Finalize();
#endif
}

#endif

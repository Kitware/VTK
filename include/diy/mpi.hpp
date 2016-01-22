#ifndef DIY_MPI_HPP
#define DIY_MPI_HPP

#include <mpi.h>

#include "mpi/constants.hpp"
#include "mpi/datatypes.hpp"
#include "mpi/optional.hpp"
#include "mpi/status.hpp"
#include "mpi/request.hpp"
#include "mpi/point-to-point.hpp"
#include "mpi/communicator.hpp"
#include "mpi/collectives.hpp"
#include "mpi/io.hpp"

namespace diy
{
namespace mpi
{

//! \ingroup MPI
struct environment
{
  environment()                           { int argc = 0; char** argv; MPI_Init(&argc, &argv); }
  environment(int argc, char* argv[])     { MPI_Init(&argc, &argv); }
  ~environment()                          { MPI_Finalize(); }
};

}
}

#endif

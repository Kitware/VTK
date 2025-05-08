#ifndef VISKORESDIY_MPI_ENVIRONMENT_HPP
#define VISKORESDIY_MPI_ENVIRONMENT_HPP

#include "config.hpp"

namespace diy
{
namespace mpi
{

//! \ingroup MPI
struct environment
{
  VISKORESDIY_MPI_EXPORT_FUNCTION static bool initialized();

  VISKORESDIY_MPI_EXPORT_FUNCTION environment();
  VISKORESDIY_MPI_EXPORT_FUNCTION environment(int requested_threading);
  VISKORESDIY_MPI_EXPORT_FUNCTION environment(int argc, char* argv[]);
  VISKORESDIY_MPI_EXPORT_FUNCTION environment(int argc, char* argv[], int requested_threading);

  VISKORESDIY_MPI_EXPORT_FUNCTION  ~environment();

  int   threading() const           { return provided_threading; }

  int   provided_threading;
};

}
} // diy::mpi

#ifndef VISKORESDIY_MPI_AS_LIB
#include "environment.cpp"
#endif

#endif // VISKORESDIY_MPI_ENVIRONMENT_HPP

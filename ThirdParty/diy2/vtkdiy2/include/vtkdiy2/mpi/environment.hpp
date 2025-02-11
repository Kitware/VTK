#ifndef DIY_MPI_ENVIRONMENT_HPP
#define DIY_MPI_ENVIRONMENT_HPP

#include "config.hpp"

namespace diy
{
namespace mpi
{

//! \ingroup MPI
struct environment
{
  DIY_MPI_EXPORT_FUNCTION static bool initialized();

  DIY_MPI_EXPORT_FUNCTION environment();
  DIY_MPI_EXPORT_FUNCTION environment(int requested_threading);
  DIY_MPI_EXPORT_FUNCTION environment(int argc, char* argv[]);
  DIY_MPI_EXPORT_FUNCTION environment(int argc, char* argv[], int requested_threading);

  DIY_MPI_EXPORT_FUNCTION  ~environment();

  int   threading() const           { return provided_threading; }

  int   provided_threading;
};

}
} // diy::mpi

#ifndef DIY_MPI_AS_LIB
#include "environment.cpp"
#endif

#endif // DIY_MPI_ENVIRONMENT_HPP

#ifndef VISKORESDIY_MPI_REQUEST_HPP
#define VISKORESDIY_MPI_REQUEST_HPP

#include "config.hpp"
#include "status.hpp"
#include "optional.hpp"

namespace diy
{
namespace mpi
{
  struct request
  {
    VISKORESDIY_MPI_EXPORT_FUNCTION                  request();
    VISKORESDIY_MPI_EXPORT_FUNCTION status           wait();
    VISKORESDIY_MPI_EXPORT_FUNCTION optional<status> test();
    VISKORESDIY_MPI_EXPORT_FUNCTION void             cancel();

    DIY_MPI_Request handle;
  };

}
} // diy::mpi

#ifndef VISKORESDIY_MPI_AS_LIB
#include "request.cpp"
#endif

#endif // VISKORESDIY_MPI_REQUEST_HPP

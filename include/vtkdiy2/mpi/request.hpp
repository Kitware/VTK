#ifndef DIY_MPI_REQUEST_HPP
#define DIY_MPI_REQUEST_HPP

#include "config.hpp"
#include "status.hpp"
#include "optional.hpp"

namespace diy
{
namespace mpi
{
  struct request
  {
    DIY_MPI_EXPORT_FUNCTION                  request();
    DIY_MPI_EXPORT_FUNCTION status           wait();
    DIY_MPI_EXPORT_FUNCTION optional<status> test();
    DIY_MPI_EXPORT_FUNCTION void             cancel();

    DIY_MPI_Request handle;
  };

}
} // diy::mpi

#ifndef DIY_MPI_AS_LIB
#include "request.cpp"
#endif

#endif // DIY_MPI_REQUEST_HPP

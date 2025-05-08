#ifndef VISKORESDIY_MPI_STATUS_HPP
#define VISKORESDIY_MPI_STATUS_HPP

#include "config.hpp"
#include "datatypes.hpp"

namespace diy
{
namespace mpi
{
  struct status
  {
    status() = default;
    status(const DIY_MPI_Status& s) : handle(s) {}

#ifndef VISKORESDIY_MPI_AS_LIB // only available in header-only mode
    status(const MPI_Status& s) : handle(s) {}
    operator MPI_Status() { return handle; }
#endif

    VISKORESDIY_MPI_EXPORT_FUNCTION int  source() const;
    VISKORESDIY_MPI_EXPORT_FUNCTION int  tag() const;
    VISKORESDIY_MPI_EXPORT_FUNCTION int  error() const;
    VISKORESDIY_MPI_EXPORT_FUNCTION bool cancelled() const;
    VISKORESDIY_MPI_EXPORT_FUNCTION int  count(const datatype& type) const;

    template<class T>       int count() const
    {
      return this->count(detail::get_mpi_datatype<T>());
    }

    DIY_MPI_Status handle;
  };

}
} // diy::mpi

#ifndef VISKORESDIY_MPI_AS_LIB
#include "status.cpp"
#endif

#endif // VISKORESDIY_MPI_STATUS_HPP

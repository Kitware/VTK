#ifdef DIY_MPI_AS_LIB
#include "request.hpp"
#endif

#include <algorithm>
#include <iterator>

#if defined(DIY_MPI_AS_LIB) && !DIY_HAS_MPI
diy::mpi::request::request()
{
  std::fill(std::begin(this->handle.data), std::end(this->handle.data), nullptr);
}
#else
diy::mpi::request::request() = default;
#endif

diy::mpi::status diy::mpi::request::wait()
{
#if DIY_HAS_MPI
  status s;
  MPI_Wait(&mpi_cast(handle), &mpi_cast(s.handle));
  return s;
#else
  DIY_UNSUPPORTED_MPI_CALL(diy::mpi::request::wait);
#endif
}

diy::mpi::optional<diy::mpi::status> diy::mpi::request::test()
{
#if DIY_HAS_MPI
  status s;
  int flag;
  MPI_Test(&mpi_cast(handle), &flag, &mpi_cast(s.handle));
  if (flag)
    return s;
#endif
  return optional<status>();
}

void diy::mpi::request::cancel()
{
#if DIY_HAS_MPI
  MPI_Cancel(&mpi_cast(handle));
#endif
}

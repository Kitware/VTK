#ifdef DIY_MPI_AS_LIB
#include "status.hpp"
#endif

int diy::mpi::status::source() const { return mpi_cast(handle).MPI_SOURCE; }
int diy::mpi::status::tag() const    { return mpi_cast(handle).MPI_TAG; }
int diy::mpi::status::error() const  { return mpi_cast(handle).MPI_ERROR; }

bool diy::mpi::status::cancelled() const
{
#if DIY_HAS_MPI
  int flag;
  MPI_Test_cancelled(&mpi_cast(handle), &flag);
  return flag;
#else
  DIY_UNSUPPORTED_MPI_CALL(diy::mpi::status::cancelled);
#endif
}

int diy::mpi::status::count(const diy::mpi::datatype& type) const
{
#if DIY_HAS_MPI
  int c;
  MPI_Get_count(&mpi_cast(handle), mpi_cast(type.handle), &c);
  return c;
#else
  (void) type;
  DIY_UNSUPPORTED_MPI_CALL(diy::mpi::status::count);
#endif
}

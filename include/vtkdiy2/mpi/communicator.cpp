#ifdef DIY_MPI_AS_LIB
#include "communicator.hpp"
#endif

diy::mpi::communicator::communicator()
  : comm_(make_DIY_MPI_Comm(MPI_COMM_WORLD)), rank_(0), size_(1), owner_(false)
{
#if DIY_HAS_MPI
  MPI_Comm_rank(mpi_cast(comm_), &rank_);
  MPI_Comm_size(mpi_cast(comm_), &size_);
#endif
}

diy::mpi::communicator::
communicator(DIY_MPI_Comm comm, bool owner):
    comm_(comm), rank_(0), size_(1), owner_(owner)
{
#if DIY_HAS_MPI
  if (mpi_cast(comm_) != MPI_COMM_NULL)
  {
    MPI_Comm_rank(mpi_cast(comm_), &rank_);
    MPI_Comm_size(mpi_cast(comm_), &size_);
  }
#endif
}

#ifndef DIY_MPI_AS_LIB // only available in header-only mode
diy::mpi::communicator::
communicator(MPI_Comm comm, bool owner):
    comm_(comm), rank_(0), size_(1), owner_(owner)
{
#if DIY_HAS_MPI
  if (comm_ != MPI_COMM_NULL)
  {
    MPI_Comm_rank(comm_, &rank_);
    MPI_Comm_size(comm_, &size_);
  }
#endif
}
#endif

void
diy::mpi::communicator::
destroy()
{
#if DIY_HAS_MPI
    if (owner_)
        MPI_Comm_free(&mpi_cast(comm_));
#endif
}

diy::mpi::status
diy::mpi::communicator::
probe(int source, int tag) const
{
#if DIY_HAS_MPI
  status s;
  MPI_Probe(source, tag, mpi_cast(comm_), &mpi_cast(s.handle));
  return s;
#else
  (void) source; (void) tag;
  DIY_UNSUPPORTED_MPI_CALL(MPI_Probe);
#endif
}

diy::mpi::optional<diy::mpi::status>
diy::mpi::communicator::
iprobe(int source, int tag) const
{
  (void) source; (void) tag;
#if DIY_HAS_MPI
  status s;
  int flag;
  MPI_Iprobe(source, tag, mpi_cast(comm_), &flag, &mpi_cast(s.handle));
  if (flag)
    return s;
#endif
  return optional<status>();
}

void
diy::mpi::communicator::
barrier() const
{
#if DIY_HAS_MPI
  MPI_Barrier(mpi_cast(comm_));
#endif
}

diy::mpi::communicator
diy::mpi::communicator::
split(int color, int key) const
{
#if DIY_HAS_MPI
    DIY_MPI_Comm newcomm;
    MPI_Comm_split(mpi_cast(comm_), color, key, &mpi_cast(newcomm));
    return communicator(newcomm, true);
#else
    (void) color; (void) key;
    return communicator();
#endif
}

diy::mpi::request
diy::mpi::communicator::
ibarrier() const
{
#if DIY_HAS_MPI
    request r;
    MPI_Ibarrier(mpi_cast(comm_), &mpi_cast(r.handle));
    return r;
#else
    // this is not the ideal fix; in principle we should just return a status
    // that tests true, but this requires redesigning some parts of our no-mpi
    // handling
    DIY_UNSUPPORTED_MPI_CALL(MPI_Ibarrier);
#endif
}

void
diy::mpi::communicator::
duplicate(const communicator& other)
{
#if DIY_HAS_MPI
    DIY_MPI_Comm newcomm;
    MPI_Comm_dup(mpi_cast(other.comm_), &mpi_cast(newcomm));
    (*this) = communicator(newcomm,true);
#endif
    (void) other;
}

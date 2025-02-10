#ifdef DIY_MPI_AS_LIB
#include "collectives.hpp"
#endif

namespace diy
{
namespace mpi
{
namespace detail
{

inline void copy_buffer(const void* src, void* dst, size_t size, int count)
{
  if (src != dst)
  {
    std::copy_n(static_cast<const int8_t*>(src),
                size * static_cast<size_t>(count),
                static_cast<int8_t*>(dst));
  }
}

void broadcast(const communicator& comm, void* data, int count, const datatype& type, int root)
{
#if DIY_HAS_MPI
  MPI_Bcast(data, count, mpi_cast(type.handle), root, mpi_cast(comm.handle()));
#else
  (void) comm; (void) data; (void) count; (void) type; (void) root;
#endif
}

request ibroadcast(const communicator& comm, void* data, int count, const datatype& type, int root)
{
  request r;
#if DIY_HAS_MPI
  MPI_Ibcast(data, count, mpi_cast(type.handle), root, mpi_cast(comm.handle()), &mpi_cast(r.handle));
#else
  (void) comm; (void) data; (void) count; (void) type; (void) root;
#endif
  return r;
}

void gather(const communicator& comm,
            const void* dataIn, int count, const datatype& type, void* dataOut,
            int root)
{
#if DIY_HAS_MPI
  MPI_Gather(dataIn, count, mpi_cast(type.handle),
             dataOut, count, mpi_cast(type.handle),
             root, mpi_cast(comm.handle()));
#else
  copy_buffer(dataIn, dataOut, mpi_cast(type.handle), count);
  (void)comm; (void)root;
#endif
}

void gather_v(const communicator& comm,
              const void* dataIn, int countIn, const datatype& type,
              void* dataOut, const int counts[], const int offsets[],
              int root)
{
#if DIY_HAS_MPI
  MPI_Gatherv(dataIn, countIn, mpi_cast(type.handle),
              dataOut, counts, offsets, mpi_cast(type.handle),
              root, mpi_cast(comm.handle()));
#else
  copy_buffer(dataIn, dataOut, mpi_cast(type.handle), countIn);
  (void)comm; (void)counts, (void)offsets, (void)root;
#endif
}

void all_gather(const communicator& comm,
                const void* dataIn, int count, const datatype& type, void* dataOut)
{
#if DIY_HAS_MPI
  MPI_Allgather(dataIn, count, mpi_cast(type.handle),
                dataOut, count, mpi_cast(type.handle),
                mpi_cast(comm.handle()));
#else
  copy_buffer(dataIn, dataOut, mpi_cast(type.handle), count);
  (void)comm;
#endif
}

void all_gather_v(const communicator& comm,
                  const void* dataIn, int countIn, const datatype& type,
                  void* dataOut, const int counts[], const int offsets[])
{
#if DIY_HAS_MPI
  MPI_Allgatherv(dataIn, countIn, mpi_cast(type.handle),
                 dataOut, counts, offsets, mpi_cast(type.handle),
                 mpi_cast(comm.handle()));
#else
  copy_buffer(dataIn, dataOut, mpi_cast(type.handle), countIn);
  (void)comm; (void)counts; (void)offsets;
#endif
}

void reduce(const communicator& comm,
            const void* dataIn, int count, const datatype& type, void* dataOut,
            const operation& op, int root)
{
#if DIY_HAS_MPI
  MPI_Reduce(dataIn, dataOut, count, mpi_cast(type.handle), mpi_cast(op.handle), root, mpi_cast(comm.handle()));
#else
  copy_buffer(dataIn, dataOut, mpi_cast(type.handle), count);
  (void)comm; (void)op; (void)root;
#endif
}

void all_reduce(const communicator& comm,
                const void* dataIn, void* dataOut, int count, const datatype& type,
                const operation& op)
{
#if DIY_HAS_MPI
  MPI_Allreduce(dataIn, dataOut, count, mpi_cast(type.handle), mpi_cast(op.handle), mpi_cast(comm.handle()));
#else
  copy_buffer(dataIn, dataOut, mpi_cast(type.handle), count);
  (void)comm; (void)op;
#endif
}

request iall_reduce(const communicator& comm,
                    const void* dataIn, void* dataOut, int count, const datatype& type,
                    const operation& op)
{
  request r;
#if DIY_HAS_MPI
  MPI_Iallreduce(dataIn, dataOut, count, mpi_cast(type.handle), mpi_cast(op.handle), mpi_cast(comm.handle()), &mpi_cast(r.handle));
#else
  copy_buffer(dataIn, dataOut, mpi_cast(type.handle), count);
  (void)comm; (void)op;
#endif
  return r;
}

void scan(const communicator& comm,
          const void* dataIn, void* dataOut, int count, const datatype& type,
          const operation& op)
{
#if DIY_HAS_MPI
  MPI_Scan(dataIn, dataOut, count, mpi_cast(type.handle), mpi_cast(op.handle), mpi_cast(comm.handle()));
#else
  copy_buffer(dataIn, dataOut, mpi_cast(type.handle), count);
  (void)comm; (void)op;
#endif
}

void all_to_all(const communicator& comm,
                const void* dataIn, int count, const datatype& type, void* dataOut)
{
#if DIY_HAS_MPI
  MPI_Alltoall(dataIn, count, mpi_cast(type.handle), dataOut, count, mpi_cast(type.handle), mpi_cast(comm.handle()));
#else
  copy_buffer(dataIn, dataOut, mpi_cast(type.handle), count);
  (void)comm;
#endif
}

}
}
} // diy::mpi::detail

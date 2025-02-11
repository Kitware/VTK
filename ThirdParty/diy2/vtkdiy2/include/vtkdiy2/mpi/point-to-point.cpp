#ifdef DIY_MPI_AS_LIB
#include "point-to-point.hpp"
#endif

namespace diy
{
namespace mpi
{

#ifdef DIY_MPI_AS_LIB
#  ifdef _MSC_VER
#    define EXPORT_MACRO DIY_MPI_EXPORT
#  else
#    define EXPORT_MACRO
#  endif
EXPORT_MACRO const int any_source  = MPI_ANY_SOURCE;
EXPORT_MACRO const int any_tag     = MPI_ANY_TAG;
#  undef EXPORT_MACRO
#endif

namespace detail
{

void send(DIY_MPI_Comm comm, int dest, int tag, const void* data, int count, const datatype& type)
{
#if DIY_HAS_MPI
  MPI_Send(data, count, mpi_cast(type.handle), dest, tag, mpi_cast(comm));
#else
  (void) comm; (void) dest; (void) tag; (void) data; (void) count; (void) type;
  DIY_UNSUPPORTED_MPI_CALL(MPI_Send);
#endif
}

void ssend(DIY_MPI_Comm comm, int dest, int tag, const void* data, int count, const datatype& type)
{
#if DIY_HAS_MPI
  MPI_Ssend(data, count, mpi_cast(type.handle), dest, tag, mpi_cast(comm));
#else
  (void) comm; (void) dest; (void) tag; (void) data; (void) count; (void) type;
  DIY_UNSUPPORTED_MPI_CALL(MPI_Ssend);
#endif
}

status probe(DIY_MPI_Comm comm, int source, int tag)
{
#if DIY_HAS_MPI
  status s;
  MPI_Probe(source, tag, mpi_cast(comm), &mpi_cast(s.handle));
  return s;
#else
  (void) comm; (void) source; (void) tag;
  DIY_UNSUPPORTED_MPI_CALL(MPI_Probe);
#endif
}

status recv(DIY_MPI_Comm comm, int source, int tag, void* data, int count, const datatype& type)
{
#if DIY_HAS_MPI
  status s;
  MPI_Recv(data, count, mpi_cast(type.handle), source, tag, mpi_cast(comm), &mpi_cast(s.handle));
  return s;
#else
  (void) comm; (void) source; (void) tag; (void) data; (void) count; (void) type;
  DIY_UNSUPPORTED_MPI_CALL(MPI_Recv);
#endif
}

request isend(DIY_MPI_Comm comm, int dest, int tag, const void* data, int count, const datatype& type)
{
#if DIY_HAS_MPI
  request r;
  MPI_Isend(data, count, mpi_cast(type.handle), dest, tag, mpi_cast(comm), &mpi_cast(r.handle));
  return r;
#else
  (void) comm; (void) dest; (void) tag; (void) data; (void) count; (void) type;
  DIY_UNSUPPORTED_MPI_CALL(MPI_Isend);
#endif
}

request issend(DIY_MPI_Comm comm, int dest, int tag, const void* data, int count, const datatype& type)
{
#if DIY_HAS_MPI
  request r;
  MPI_Issend(data, count, mpi_cast(type.handle), dest, tag, mpi_cast(comm), &mpi_cast(r.handle));
  return r;
#else
  (void) comm; (void) dest; (void) tag; (void) data; (void) count; (void) type;
  DIY_UNSUPPORTED_MPI_CALL(MPI_Issend);
#endif
}

request irecv(DIY_MPI_Comm comm, int source, int tag, void* data, int count, const datatype& type)
{
#if DIY_HAS_MPI
  request r;
  MPI_Irecv(data, count, mpi_cast(type.handle), source, tag, mpi_cast(comm), &mpi_cast(r.handle));
  return r;
#else
  (void) comm; (void) source; (void) tag; (void) data; (void) count; (void) type;
  DIY_UNSUPPORTED_MPI_CALL(MPI_Irecv);
#endif
}

}
}
} // diy::mpi::detail

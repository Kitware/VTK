#ifndef DIY_MPI_POINT_TO_POINT_HPP
#define DIY_MPI_POINT_TO_POINT_HPP

#include "config.hpp"
#include "datatypes.hpp"
#include "request.hpp"
#include "status.hpp"

#include <vector>

namespace diy
{
namespace mpi
{

#ifndef DIY_MPI_AS_LIB
constexpr int any_source  = MPI_ANY_SOURCE;
constexpr int any_tag     = MPI_ANY_TAG;
#else
DIY_MPI_EXPORT extern const int any_source;
DIY_MPI_EXPORT extern const int any_tag;
#endif

namespace detail
{
  DIY_MPI_EXPORT_FUNCTION void send(DIY_MPI_Comm comm, int dest, int tag, const void* data, int count, const datatype& type);
  DIY_MPI_EXPORT_FUNCTION void ssend(DIY_MPI_Comm comm, int dest, int tag, const void* data, int count, const datatype& type);
  DIY_MPI_EXPORT_FUNCTION request isend(DIY_MPI_Comm comm, int dest, int tag, const void* data, int count, const datatype& type);
  DIY_MPI_EXPORT_FUNCTION request issend(DIY_MPI_Comm comm, int dest, int tag, const void* data, int count, const datatype& type);
  DIY_MPI_EXPORT_FUNCTION status probe(DIY_MPI_Comm comm, int source, int tag);
  DIY_MPI_EXPORT_FUNCTION status recv(DIY_MPI_Comm comm, int source, int tag, void* data, int count, const datatype& type);
  DIY_MPI_EXPORT_FUNCTION request irecv(DIY_MPI_Comm comm, int source, int tag, void* data, int count, const datatype& type);

  template <class T>
  inline void send(DIY_MPI_Comm comm, int dest, int tag, const T& x)
  {
    static_assert(std::is_same<typename is_mpi_datatype<T>::type, true_type>::value, "is_mpi_datatype<T>::type must be true_type");
    send(comm, dest, tag, address(x), count(x), datatype_of(x));
  }

  template <class T>
  inline void ssend(DIY_MPI_Comm comm, int dest, int tag, const T& x)
  {
    static_assert(std::is_same<typename is_mpi_datatype<T>::type, true_type>::value, "is_mpi_datatype<T>::type must be true_type");
    ssend(comm, dest, tag, address(x), count(x), datatype_of(x));
  }

  template <class T>
  status recv(DIY_MPI_Comm comm, int source, int tag, T& x)
  {
    static_assert(std::is_same<typename is_mpi_datatype<T>::type, true_type>::value, "is_mpi_datatype<T>::type must be true_type");
    return recv(comm, source, tag, address(x), count(x), datatype_of(x));
  }

  template <class T>
  status recv(DIY_MPI_Comm comm, int source, int tag, std::vector<T>& x)
  {
    auto s = probe(comm, source, tag);
    x.resize(static_cast<size_t>(s.count<T>()));
    return recv(comm, source, tag, address(x), count(x), datatype_of(x));
  }

  template <class T>
  request isend(DIY_MPI_Comm comm, int dest, int tag, const T& x)
  {
    static_assert(std::is_same<typename is_mpi_datatype<T>::type, true_type>::value, "is_mpi_datatype<T>::type must be true_type");
    return isend(comm, dest, tag, address(x), count(x), datatype_of(x));
  }

  template <class T>
  request issend(DIY_MPI_Comm comm, int dest, int tag, const T& x)
  {
    static_assert(std::is_same<typename is_mpi_datatype<T>::type, true_type>::value, "is_mpi_datatype<T>::type must be true_type");
    return issend(comm, dest, tag, address(x), count(x), datatype_of(x));
  }

  template <class T>
  request irecv(DIY_MPI_Comm comm, int source, int tag, T& x)
  {
    static_assert(std::is_same<typename is_mpi_datatype<T>::type, true_type>::value, "is_mpi_datatype<T>::type must be true_type");
    return irecv(comm, source, tag, address(x), count(x), datatype_of(x));
  }

}
}
} // diy::mpi::detail

#ifndef DIY_MPI_AS_LIB
#include "point-to-point.cpp"
#endif

#endif // DIY_MPI_POINT_TO_POINT_HPP

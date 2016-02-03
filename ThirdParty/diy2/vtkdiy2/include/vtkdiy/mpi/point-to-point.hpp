#include <vector>

namespace diy
{
namespace mpi
{
namespace detail
{
  // send
  template< class T, class is_mpi_datatype_ = typename is_mpi_datatype<T>::type >
  struct send;

  template<class T>
  struct send<T, true_type>
  {
    void operator()(MPI_Comm comm, int dest, int tag, const T& x) const
    {
      typedef       mpi_datatype<T>     Datatype;
      MPI_Send((void*) Datatype::address(x),
               Datatype::count(x),
               Datatype::datatype(),
               dest, tag, comm);
    }
  };

  // recv
  template< class T, class is_mpi_datatype_ = typename is_mpi_datatype<T>::type >
  struct recv;

  template<class T>
  struct recv<T, true_type>
  {
    status operator()(MPI_Comm comm, int source, int tag, T& x) const
    {
      typedef       mpi_datatype<T>     Datatype;
      status s(Datatype::datatype());
      MPI_Recv(&x, 1, get_mpi_datatype<T>(), source, tag, comm, &s.s);
      return s;
    }
  };

  template<class U>
  struct recv<std::vector<U>, true_type>
  {
    status operator()(MPI_Comm comm, int source, int tag, std::vector<U>& x) const
    {
      status s;

      MPI_Probe(source, tag, comm, &s.s);
      x.resize(s.count<U>());
      MPI_Recv(&x[0], x.size(), get_mpi_datatype<U>(), source, tag, comm, &s.s);
      return s;
    }
  };

  // isend
  template< class T, class is_mpi_datatype_ = typename is_mpi_datatype<T>::type >
  struct isend;

  template<class T>
  struct isend<T, true_type>
  {
    request operator()(MPI_Comm comm, int dest, int tag, const T& x) const
    {
      request r;
      typedef       mpi_datatype<T>     Datatype;
      MPI_Isend((void*) Datatype::address(x),
                Datatype::count(x),
                Datatype::datatype(),
                dest, tag, comm, &r.r);
      return r;
    }
  };

  // irecv
  template< class T, class is_mpi_datatype_ = typename is_mpi_datatype<T>::type >
  struct irecv;

  template<class T>
  struct irecv<T, true_type>
  {
    request operator()(MPI_Comm comm, int source, int tag, T& x) const
    {
      request r;
      typedef       mpi_datatype<T>     Datatype;
      MPI_Irecv(Datatype::address(x),
                Datatype::count(x),
                Datatype::datatype(),
                source, tag, comm, &r.r);
      return r;
    }
  };
}
}
}

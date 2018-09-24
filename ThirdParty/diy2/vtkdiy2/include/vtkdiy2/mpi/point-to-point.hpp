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
#ifndef DIY_NO_MPI
      typedef       mpi_datatype<T>     Datatype;
      MPI_Send((void*) Datatype::address(x),
               Datatype::count(x),
               Datatype::datatype(),
               dest, tag, comm);
#else
      (void) comm; (void) dest; (void) tag; (void) x;
      DIY_UNSUPPORTED_MPI_CALL(MPI_Send);
#endif
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
#ifndef DIY_NO_MPI
      typedef       mpi_datatype<T>     Datatype;
      status s;
      MPI_Recv((void*) Datatype::address(x),
                Datatype::count(x),
                Datatype::datatype(),
                source, tag, comm, &s.s);
      return s;
#else
      (void) comm; (void) source; (void) tag; (void) x;
      DIY_UNSUPPORTED_MPI_CALL(MPI_Recv);
#endif
    }
  };

  template<class U>
  struct recv<std::vector<U>, true_type>
  {
    status operator()(MPI_Comm comm, int source, int tag, std::vector<U>& x) const
    {
#ifndef DIY_NO_MPI
      status s;

      MPI_Probe(source, tag, comm, &s.s);
      x.resize(s.count<U>());
      MPI_Recv(&x[0], static_cast<int>(x.size()), get_mpi_datatype<U>(), source, tag, comm, &s.s);
      return s;
#else
      (void) comm; (void) source; (void) tag; (void) x;
      DIY_UNSUPPORTED_MPI_CALL(MPI_Recv);
#endif
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
#ifndef DIY_NO_MPI
      request r;
      typedef       mpi_datatype<T>     Datatype;
      MPI_Isend((void*) Datatype::address(x),
                Datatype::count(x),
                Datatype::datatype(),
                dest, tag, comm, &r.r);
      return r;
#else
      (void) comm; (void) dest; (void) tag; (void) x;
      DIY_UNSUPPORTED_MPI_CALL(MPI_Isend);
#endif
    }
  };

  // issend
  template< class T, class is_mpi_datatype_ = typename is_mpi_datatype<T>::type >
  struct issend;

  template<class T>
  struct issend<T, true_type>
  {
    request operator()(MPI_Comm comm, int dest, int tag, const T& x) const
    {
#ifndef DIY_NO_MPI
      request r;
      typedef       mpi_datatype<T>     Datatype;
      MPI_Issend((void*) Datatype::address(x),
                Datatype::count(x),
                Datatype::datatype(),
                dest, tag, comm, &r.r);
      return r;
#else
      (void) comm; (void) dest; (void) tag; (void) x;
      DIY_UNSUPPORTED_MPI_CALL(MPI_Issend);
#endif
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
#ifndef DIY_NO_MPI
      request r;
      typedef       mpi_datatype<T>     Datatype;
      MPI_Irecv(Datatype::address(x),
                Datatype::count(x),
                Datatype::datatype(),
                source, tag, comm, &r.r);
      return r;
#else
      (void) comm; (void) source; (void) tag; (void) x;
      DIY_UNSUPPORTED_MPI_CALL(MPI_Irecv);
#endif
    }
  };
}
}
}

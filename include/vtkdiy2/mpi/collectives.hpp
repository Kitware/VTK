#include <vector>

#include "../constants.h" // for DIY_UNUSED.
#include "operations.hpp"

namespace diy
{
namespace mpi
{
  //!\addtogroup MPI
  //!@{

  template<class T, class Op>
  struct Collectives
  {
    static void broadcast(const communicator& comm, T& x, int root)
    {
#ifndef DIY_NO_MPI
      MPI_Bcast(address(x), count(x), datatype(x), root, comm);
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(x);
      DIY_UNUSED(root);
#endif
    }

    static void broadcast(const communicator& comm, std::vector<T>& x, int root)
    {
#ifndef DIY_NO_MPI
      size_t sz = x.size();
      Collectives<size_t, void*>::broadcast(comm, sz, root);

      if (comm.rank() != root)
          x.resize(sz);

      MPI_Bcast(address(x), count(x), datatype(x), root, comm);
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(x);
      DIY_UNUSED(root);
#endif
    }

    static request ibroadcast(const communicator& comm, T& x, int root)
    {
#ifndef DIY_NO_MPI
      request r;
      MPI_Ibcast(address(x), count(x), datatype(x), root, comm, &r.r);
      return r;
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(x);
      DIY_UNUSED(root);
      DIY_UNSUPPORTED_MPI_CALL(MPI_Ibcast);
#endif
    }

    static void gather(const communicator& comm, const T& in, std::vector<T>& out, int root)
    {
      out.resize(comm.size());
#ifndef DIY_NO_MPI
      MPI_Gather(address(in), count(in), datatype(in), address(out), count(in), datatype(out), root, comm);
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(root);
      out[0] = in;
#endif
    }

    static void gather(const communicator& comm, const std::vector<T>& in, std::vector< std::vector<T> >& out, int root)
    {
#ifndef DIY_NO_MPI
      std::vector<int>  counts(comm.size());
      Collectives<int,void*>::gather(comm, count(in), counts, root);

      std::vector<int>  offsets(comm.size(), 0);
      for (unsigned i = 1; i < offsets.size(); ++i)
        offsets[i] = offsets[i-1] + counts[i-1];

      int elem_size = count(in[0]);     // size of 1 vector element in units of mpi datatype
      std::vector<T> buffer((offsets.back() + counts.back()) / elem_size);
      MPI_Gatherv(address(in), count(in), datatype(in),
                  address(buffer),
                  &counts[0],
                  &offsets[0],
                  datatype(buffer),
                  root, comm);

      out.resize(comm.size());
      size_t cur = 0;
      for (unsigned i = 0; i < (unsigned)comm.size(); ++i)
      {
          out[i].reserve(counts[i] / elem_size);
          for (unsigned j = 0; j < (unsigned)(counts[i] / elem_size); ++j)
              out[i].push_back(buffer[cur++]);
      }
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(root);
      out.resize(1);
      out[0] = in;
#endif
    }

    static void gather(const communicator& comm, const T& in, int root)
    {
#ifndef DIY_NO_MPI
      MPI_Gather(address(in), count(in), datatype(in), address(in), count(in), datatype(in), root, comm);
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(in);
      DIY_UNUSED(root);
      DIY_UNSUPPORTED_MPI_CALL("MPI_Gather");
#endif
    }

    static void gather(const communicator& comm, const std::vector<T>& in, int root)
    {
#ifndef DIY_NO_MPI
      Collectives<int,void*>::gather(comm, count(in), root);

      MPI_Gatherv(address(in), count(in), datatype(in),
                  0, 0, 0,
                  datatype(in),
                  root, comm);
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(in);
      DIY_UNUSED(root);
      DIY_UNSUPPORTED_MPI_CALL("MPI_Gatherv");
#endif
    }

    static void all_gather(const communicator& comm, const T& in, std::vector<T>& out)
    {
      out.resize(comm.size());
#ifndef DIY_NO_MPI
      MPI_Allgather(address(in), count(in), datatype(in),
                    address(out), count(in), datatype(in),
                    comm);
#else
      DIY_UNUSED(comm);
      out[0] = in;
#endif
    }

    static void all_gather(const communicator& comm, const std::vector<T>& in, std::vector< std::vector<T> >& out)
    {
#ifndef DIY_NO_MPI
      std::vector<int>  counts(comm.size());
      Collectives<int,void*>::all_gather(comm, count(in), counts);

      std::vector<int>  offsets(comm.size(), 0);
      for (unsigned i = 1; i < offsets.size(); ++i)
        offsets[i] = offsets[i-1] + counts[i-1];

      int elem_size = count(in[0]);     // size of 1 vector element in units of mpi datatype
      std::vector<T> buffer((offsets.back() + counts.back()) / elem_size);
      MPI_Allgatherv(address(in), count(in), datatype(in),
                     address(buffer),
                     &counts[0],
                     &offsets[0],
                     datatype(buffer),
                     comm);

      out.resize(comm.size());
      size_t cur = 0;
      for (int i = 0; i < comm.size(); ++i)
      {
          out[i].reserve(counts[i] / elem_size);
          for (int j = 0; j < (int)(counts[i] / elem_size); ++j)
              out[i].push_back(buffer[cur++]);
      }
#else
      DIY_UNUSED(comm);
      out.resize(1);
      out[0] = in;
#endif
    }

    static void reduce(const communicator& comm, const T& in, T& out, int root, const Op&)
    {
#ifndef DIY_NO_MPI
      MPI_Reduce(address(in), address(out), count(in), datatype(in),
                 detail::mpi_op<Op>::get(),
                 root, comm);
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(root);
      out = in;
#endif
    }

    static void reduce(const communicator& comm, const T& in, int root, const Op&)
    {
#ifndef DIY_NO_MPI
      MPI_Reduce(address(in), address(in), count(in), datatype(in),
                 detail::mpi_op<Op>::get(),
                 root, comm);
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(in);
      DIY_UNUSED(root);
      DIY_UNSUPPORTED_MPI_CALL("MPI_Reduce");
#endif
    }

    static void all_reduce(const communicator& comm, const T& in, T& out, const Op&)
    {
#ifndef DIY_NO_MPI
      MPI_Allreduce(address(in), address(out), count(in), datatype(in),
                    detail::mpi_op<Op>::get(),
                    comm);
#else
      DIY_UNUSED(comm);
      out = in;
#endif
    }

    static void all_reduce(const communicator& comm, const std::vector<T>& in, std::vector<T>& out, const Op&)
    {
#ifndef DIY_NO_MPI
      out.resize(in.size());
      MPI_Allreduce(address(in), address(out), count(in),
                    datatype(in),
                    detail::mpi_op<Op>::get(),
                    comm);
#else
      DIY_UNUSED(comm);
      out = in;
#endif
    }

    static request iall_reduce(const communicator& comm, const T& in, T& out, const Op&)
    {
#ifndef DIY_NO_MPI
      request r;
      MPI_Iallreduce(address(in), address(out), count(in), datatype(in),
                     detail::mpi_op<Op>::get(),
                     comm, &r.r);
      return r;
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(in);
      DIY_UNUSED(out);
      DIY_UNSUPPORTED_MPI_CALL(MPI_Iallreduce);
#endif
    }

    static request iall_reduce(const communicator& comm, const std::vector<T>& in, std::vector<T>& out, const Op&)
    {
#ifndef DIY_NO_MPI
      request r;
      out.resize(in.size());
      MPI_Iallreduce(address(in), address(out), count(in),
                     datatype(in),
                     detail::mpi_op<Op>::get(),
                     comm, &r.r);
      return r;
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(in);
      DIY_UNUSED(out);
      DIY_UNSUPPORTED_MPI_CALL(MPI_Iallreduce);
#endif
    }

    static void scan(const communicator& comm, const T& in, T& out, const Op&)
    {
#ifndef DIY_NO_MPI
      MPI_Scan(address(in), address(out), count(in), datatype(in),
               detail::mpi_op<Op>::get(),
               comm);
#else
      DIY_UNUSED(comm);
      out = in;
#endif
    }

    static void all_to_all(const communicator& comm, const std::vector<T>& in, std::vector<T>& out, int n = 1)
    {
#ifndef DIY_NO_MPI
      // n specifies how many elements go to/from every process from every process;
      // the sizes of in and out are expected to be n * comm.size()

      int elem_size = count(in[0]);               // size of 1 vector element in units of mpi datatype
      // NB: this will fail if T is a vector
      MPI_Alltoall(address(in),
                   elem_size * n,
                   datatype(in),
                   address(out),
                   elem_size * n,
                   datatype(out),
                   comm);
#else
      DIY_UNUSED(comm);
      DIY_UNUSED(n);
      out = in;
#endif
    }
  };

  //! iBarrier; standalone function version for completeness
  inline request   ibarrier(const communicator& comm)
  {
    return comm.ibarrier();
  }

  //! Broadcast to all processes in `comm`.
  template<class T>
  inline
  void      broadcast(const communicator& comm, T& x, int root)
  {
    Collectives<T,void*>::broadcast(comm, x, root);
  }

  //! Broadcast for vectors
  template<class T>
  inline
  void      broadcast(const communicator& comm, std::vector<T>& x, int root)
  {
    Collectives<T,void*>::broadcast(comm, x, root);
  }

  //! iBroadcast to all processes in `comm`.
  template<class T>
  request   ibroadcast(const communicator& comm, T& x, int root)
  {
    return Collectives<T,void*>::ibroadcast(comm, x, root);
  }

  //! Gather from all processes in `comm`.
  //!  On `root` process, `out` is resized to `comm.size()` and filled with
  //! elements from the respective ranks.
  template<class T>
  inline
  void      gather(const communicator& comm, const T& in, std::vector<T>& out, int root)
  {
    Collectives<T,void*>::gather(comm, in, out, root);
  }

  //! Same as above, but for vectors.
  template<class T>
  inline
  void      gather(const communicator& comm, const std::vector<T>& in, std::vector< std::vector<T> >& out, int root)
  {
    Collectives<T,void*>::gather(comm, in, out, root);
  }

  //! Simplified version (without `out`) for use on non-root processes.
  template<class T>
  inline
  void      gather(const communicator& comm, const T& in, int root)
  {
    Collectives<T,void*>::gather(comm, in, root);
  }

  //! Simplified version (without `out`) for use on non-root processes.
  template<class T>
  inline
  void      gather(const communicator& comm, const std::vector<T>& in, int root)
  {
    Collectives<T,void*>::gather(comm, in, root);
  }

  //! all_gather from all processes in `comm`.
  //! `out` is resized to `comm.size()` and filled with
  //! elements from the respective ranks.
  template<class T>
  inline
  void      all_gather(const communicator& comm, const T& in, std::vector<T>& out)
  {
    Collectives<T,void*>::all_gather(comm, in, out);
  }

  //! Same as above, but for vectors.
  template<class T>
  inline
  void      all_gather(const communicator& comm, const std::vector<T>& in, std::vector< std::vector<T> >& out)
  {
    Collectives<T,void*>::all_gather(comm, in, out);
  }

  //! reduce
  template<class T, class Op>
  inline
  void      reduce(const communicator& comm, const T& in, T& out, int root, const Op& op)
  {
    Collectives<T, Op>::reduce(comm, in, out, root, op);
  }

  //! Simplified version (without `out`) for use on non-root processes.
  template<class T, class Op>
  inline
  void      reduce(const communicator& comm, const T& in, int root, const Op& op)
  {
    Collectives<T, Op>::reduce(comm, in, root, op);
  }

  //! all_reduce
  template<class T, class Op>
  inline
  void      all_reduce(const communicator& comm, const T& in, T& out, const Op& op)
  {
    Collectives<T, Op>::all_reduce(comm, in, out, op);
  }

  //! Same as above, but for vectors.
  template<class T, class Op>
  inline
  void      all_reduce(const communicator& comm, const std::vector<T>& in, std::vector<T>& out, const Op& op)
  {
    Collectives<T, Op>::all_reduce(comm, in, out, op);
  }

  //! iall_reduce
  template<class T, class Op>
  inline
  request   iall_reduce(const communicator& comm, const T& in, T& out, const Op& op)
  {
    return Collectives<T, Op>::iall_reduce(comm, in, out, op);
  }

  //! Same as above, but for vectors.
  template<class T, class Op>
  inline
  request   iall_reduce(const communicator& comm, const std::vector<T>& in, std::vector<T>& out, const Op& op)
  {
    return Collectives<T, Op>::iall_reduce(comm, in, out, op);
  }


  //! scan
  template<class T, class Op>
  inline
  void      scan(const communicator& comm, const T& in, T& out, const Op& op)
  {
    Collectives<T, Op>::scan(comm, in, out, op);
  }

  //! all_to_all
  template<class T>
  inline
  void      all_to_all(const communicator& comm, const std::vector<T>& in, std::vector<T>& out, int n = 1)
  {
    Collectives<T, void*>::all_to_all(comm, in, out, n);
  }

  //!@}
}
}

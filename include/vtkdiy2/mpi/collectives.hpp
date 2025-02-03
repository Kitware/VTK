#ifndef DIY_MPI_COLLECTIVES_HPP
#define DIY_MPI_COLLECTIVES_HPP

#include "config.hpp"
#include "communicator.hpp"
#include "datatypes.hpp"
#include "operations.hpp"
#include "request.hpp"

#include <algorithm>
#include <vector>
#include <numeric>

namespace diy
{
namespace mpi
{

namespace detail
{

DIY_MPI_EXPORT_FUNCTION
void broadcast(const communicator& comm,
               void* data, int count, const datatype& type,
               int root);

DIY_MPI_EXPORT_FUNCTION
request ibroadcast(const communicator& comm,
                   void* data, int count, const datatype& type,
                   int root);

DIY_MPI_EXPORT_FUNCTION
void gather(const communicator& comm,
            const void* dataIn, int count, const datatype& type, void* dataOut,
            int root);

DIY_MPI_EXPORT_FUNCTION
void gather_v(const communicator& comm,
              const void* dataIn, int countIn, const datatype& type,
              void* dataOut, const int counts[], const int offsets[],
              int root);

DIY_MPI_EXPORT_FUNCTION
void all_gather(const communicator& comm,
                const void* dataIn, int count, const datatype& type, void* dataOut);

DIY_MPI_EXPORT_FUNCTION
void all_gather_v(const communicator& comm,
                  const void* dataIn, int countIn, const datatype& type,
                  void* dataOut, const int counts[], const int offsets[]);

DIY_MPI_EXPORT_FUNCTION
void reduce(const communicator& comm,
            const void* dataIn, int count, const datatype& type, void* dataOut,
            const operation& op, int root);

DIY_MPI_EXPORT_FUNCTION
void all_reduce(const communicator& comm,
                const void* dataIn, void* dataOut, int count, const datatype& type,
                const operation& op);

DIY_MPI_EXPORT_FUNCTION
request iall_reduce(const communicator& comm,
                    const void* dataIn, void* dataOut, int count, const datatype& type,
                    const operation& op);

DIY_MPI_EXPORT_FUNCTION
void scan(const communicator& comm,
          const void* dataIn, void* dataOut, int count, const datatype& type,
          const operation& op);

DIY_MPI_EXPORT_FUNCTION
void all_to_all(const communicator& comm,
                const void* dataIn, int count, const datatype& type, void* dataOut);

} // detail

  //!\addtogroup MPI
  //!@{

  template<class T, class Op>
  struct Collectives
  {
    static void broadcast(const communicator& comm, T& x, int root)
    {
      detail::broadcast(comm, address(x), count(x), datatype_of(x), root);
    }

    static void broadcast(const communicator& comm, std::vector<T>& x, int root)
    {
      size_t sz = x.size();
      detail::broadcast(comm, &sz, 1, datatype_of(sz), root);

      if (comm.rank() != root)
          x.resize(sz);

      detail::broadcast(comm, address(x), count(x), datatype_of(x), root);
    }

    static request ibroadcast(const communicator& comm, T& x, int root)
    {
      return detail::ibroadcast(comm, address(x), count(x), datatype_of(x), root);
    }

    static void gather(const communicator& comm, const T& in, std::vector<T>& out, int root)
    {
      out.resize(comm.size());
      detail::gather(comm, address(in), count(in), datatype_of(in), address(out), root);
    }

    static void gather(const communicator& comm, const std::vector<T>& in, std::vector< std::vector<T> >& out, int root)
    {
      std::vector<int> counts;
      if (comm.rank() == root)
      {
        counts.resize(static_cast<size_t>(comm.size()));
      }

      Collectives<int,void*>::gather(comm, count(in), counts, root);

      std::vector<int> offsets;
      if (comm.rank() == root)
      {
        offsets.resize(counts.size());
        offsets[0] = 0;
        std::partial_sum(counts.begin(), counts.end() - 1, offsets.begin() + 1);
      }

      int elem_size = count(in[0]);     // size of 1 vector element in units of mpi datatype
      std::vector<T> buffer;
      if (comm.rank() == root)
      {
        buffer.resize((offsets.back() + counts.back()) / elem_size);
      }

      detail::gather_v(comm, address(in), count(in), datatype_of(in),
                       address(buffer), counts.data(), offsets.data(),
                       root);

      if (comm.rank() == root)
      {
          out.resize(static_cast<size_t>(comm.size()));
          size_t offset = 0;
          for (size_t i = 0; i < out.size(); ++i)
          {
            auto count = static_cast<size_t>(counts[i] / elem_size);
            out[i].insert(out[i].end(), buffer.data() + offset, buffer.data() + offset + count);
            offset += count;
          }
      }
    }

    static void gather(const communicator& comm, const T& in, int root)
    {
      detail::gather(comm, address(in), count(in), datatype_of(in), address(in), root);
    }

    static void gather(const communicator& comm, const std::vector<T>& in, int root)
    {
      Collectives<int,void*>::gather(comm, count(in), root);
      detail::gather_v(comm, address(in), count(in), datatype_of(in), 0, 0, 0, root);
    }

    static void all_gather(const communicator& comm, const T& in, std::vector<T>& out)
    {
      out.resize(comm.size());
      detail::all_gather(comm, address(in), count(in), datatype_of(in), address(out));
    }

    static void all_gather(const communicator& comm, const std::vector<T>& in, std::vector< std::vector<T> >& out)
    {
      std::vector<int>  counts(static_cast<size_t>(comm.size()));
      Collectives<int,void*>::all_gather(comm, count(in), counts);

      std::vector<int>  offsets(counts.size());
      offsets[0] = 0;
      std::partial_sum(counts.begin(), counts.end() - 1, offsets.begin() + 1);

      int elem_size = count(in[0]);     // size of 1 vector element in units of mpi datatype
      std::vector<T> buffer((offsets.back() + counts.back()) / elem_size);
      detail::all_gather_v(comm,
                           address(in), count(in), datatype_of(in),
                           address(buffer),
                           &counts[0],
                           &offsets[0]);

      out.resize(static_cast<size_t>(comm.size()));
      size_t offset = 0;
      for (size_t i = 0; i < out.size(); ++i)
      {
          auto count = static_cast<size_t>(counts[i] / elem_size);
          out[i].insert(out[i].end(), buffer.data() + offset, buffer.data() + offset + count);
          offset += count;
      }
    }

    static void reduce(const communicator& comm, const T& in, T& out, int root, const Op&)
    {
      auto op = detail::mpi_op<Op>::get();
      detail::reduce(comm, address(in), count(in), datatype_of(in), address(out), op, root);
    }

    static void reduce(const communicator& comm, const T& in, int root, const Op&)
    {
      auto op = detail::mpi_op<Op>::get();
      detail::reduce(comm, address(in), count(in), datatype_of(in), address(in), op, root);
    }

    static void all_reduce(const communicator& comm, const T& in, T& out, const Op&)
    {
      auto op = detail::mpi_op<Op>::get();
      detail::all_reduce(comm, address(in), address(out), count(in), datatype_of(in), op);
    }

    static void all_reduce(const communicator& comm, const std::vector<T>& in, std::vector<T>& out, const Op&)
    {
      auto op = detail::mpi_op<Op>::get();
      out.resize(in.size());
      detail::all_reduce(comm, address(in), address(out), count(in), datatype_of(in), op);
    }

    static request iall_reduce(const communicator& comm, const T& in, T& out, const Op&)
    {
      auto op = detail::mpi_op<Op>::get();
      return detail::iall_reduce(comm, address(in), address(out), count(in), datatype_of(in), op);
    }

    static request iall_reduce(const communicator& comm, const std::vector<T>& in, std::vector<T>& out, const Op&)
    {
      auto op = detail::mpi_op<Op>::get();
      out.resize(in.size());
      return detail::iall_reduce(comm, address(in), address(out), count(in), datatype_of(in), op);
    }

    static void scan(const communicator& comm, const T& in, T& out, const Op&)
    {
      auto op = detail::mpi_op<Op>::get();
      detail::scan(comm, address(in), address(out), count(in), datatype_of(in), op);
    }

    static void all_to_all(const communicator& comm, const std::vector<T>& in, std::vector<T>& out, int n = 1)
    {
      // n specifies how many elements go to/from every process from every process;
      // the sizes of in and out are expected to be n * comm.size()

      int elem_size = count(in[0]);               // size of 1 vector element in units of mpi datatype
      // NB: this will fail if T is a vector
      detail::all_to_all(comm, address(in), elem_size * n, datatype_of(in), address(out));
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

#ifndef DIY_MPI_AS_LIB
#include "collectives.cpp"
#endif

#endif // DIY_MPI_COLLECTIVES_HPP

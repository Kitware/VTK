#ifndef DIY_MPI_COMMUNICATOR_HPP
#define DIY_MPI_COMMUNICATOR_HPP

#include "config.hpp"
#include "optional.hpp"
#include "point-to-point.hpp"
#include "request.hpp"
#include "status.hpp"

namespace diy
{
namespace mpi
{

  //! \ingroup MPI
  //! Simple wrapper around `MPI_Comm`.
  class communicator
  {
    public:
                DIY_MPI_EXPORT_FUNCTION
                communicator();

                communicator(DIY_MPI_Comm comm):
                  communicator(comm, false) {}

                DIY_MPI_EXPORT_FUNCTION
                communicator(DIY_MPI_Comm comm, bool owner);

                ~communicator()                     { destroy(); }

                communicator(const communicator& other):
                    comm_(other.comm_),
                    rank_(other.rank_),
                    size_(other.size_),
                    owner_(false)                   {}

                communicator(communicator&& other):
                    comm_(other.comm_),
                    rank_(other.rank_),
                    size_(other.size_),
                    owner_(other.owner_)                    { other.owner_ = false; }

#ifndef DIY_MPI_AS_LIB // only available in header-only mode
                communicator(MPI_Comm comm):
                  communicator(comm, false) {}

                DIY_MPI_EXPORT_FUNCTION
                communicator(MPI_Comm comm, bool owner);

                operator MPI_Comm() { return comm_; }
#endif

      communicator&
                operator=(const communicator& other)        { destroy(); comm_ = other.comm_; rank_ = other.rank_; size_ = other.size_; owner_ = false; return *this; }
      communicator&
                operator=(communicator&& other)             { destroy(); comm_ = other.comm_; rank_ = other.rank_; size_ = other.size_; owner_ = other.owner_; other.owner_ = false; return *this; }

      int       rank() const                        { return rank_; }
      int       size() const                        { return size_; }

      //! Send `x` to processor `dest` using `tag` (blocking).
      template<class T>
      void      send(int dest, int tag, const T& x) const   { detail::send(comm_, dest, tag, x); }

      template<class T>
      void      ssend(int dest, int tag, const T& x) const   { detail::ssend(comm_, dest, tag, x); }

      //! Receive `x` from `dest` using `tag` (blocking).
      //! If `T` is an `std::vector<...>`, `recv` will resize it to fit exactly the sent number of values.
      template<class T>
      status    recv(int source, int tag, T& x) const       { return detail::recv(comm_, source, tag, x); }

      //! Non-blocking version of `send()`.
      template<class T>
      request   isend(int dest, int tag, const T& x) const  { return detail::isend(comm_, dest, tag, x); }

      //! Non-blocking version of `ssend()`.
      template<class T>
      request   issend(int dest, int tag, const T& x) const  { return detail::issend(comm_, dest, tag, x); }

      //! Non-blocking version of `recv()`.
      //! If `T` is an `std::vector<...>`, its size must be big enough to accommodate the sent values.
      template<class T>
      request   irecv(int source, int tag, T& x) const      { return detail::irecv(comm_, source, tag, x); }

      //! probe
      DIY_MPI_EXPORT_FUNCTION
      status    probe(int source, int tag) const;

      //! iprobe
      DIY_MPI_EXPORT_FUNCTION
      optional<status>
                iprobe(int source, int tag) const;

      //! barrier
      DIY_MPI_EXPORT_FUNCTION
      void      barrier() const;

      //! Nonblocking version of barrier
      DIY_MPI_EXPORT_FUNCTION
      request   ibarrier() const;

      //! split
      //! When keys are the same, the ties are broken by the rank in the original comm.
      DIY_MPI_EXPORT_FUNCTION
      communicator
                split(int color, int key = 0) const;

      //! duplicate
      DIY_MPI_EXPORT_FUNCTION
      void      duplicate(const communicator& other);

      DIY_MPI_Comm handle() const { return comm_; }

    private:
      DIY_MPI_EXPORT_FUNCTION
      void      destroy();

    private:
      DIY_MPI_Comm  comm_;
      int           rank_;
      int           size_;
      bool          owner_;
  };

}
} // diy::mpi

#ifndef DIY_MPI_AS_LIB
#include "communicator.cpp"
#endif

#endif // DIY_MPI_COMMUNICATOR_HPP

namespace diy
{
namespace mpi
{

  //! \ingroup MPI
  //! Simple wrapper around `MPI_Comm`.
  class communicator
  {
    public:
                communicator(MPI_Comm comm = MPI_COMM_WORLD):
                  comm_(comm)                       { MPI_Comm_rank(comm_, &rank_); MPI_Comm_size(comm_, &size_); }

      int       rank() const                        { return rank_; }
      int       size() const                        { return size_; }

      //void      send(int dest,
      //               int tag,
      //               const void* buf,
      //               MPI_Datatype datatype) const   { }

      //! Send `x` to processor `dest` using `tag` (blocking).
      template<class T>
      void      send(int dest, int tag, const T& x) const   { detail::send<T>()(comm_, dest, tag, x); }

      //! Receive `x` from `dest` using `tag` (blocking).
      //! If `T` is an `std::vector<...>`, `recv` will resize it to fit exactly the sent number of values.
      template<class T>
      status    recv(int source, int tag, T& x) const       { return detail::recv<T>()(comm_, source, tag, x); }

      //! Non-blocking version of `send()`.
      template<class T>
      request   isend(int dest, int tag, const T& x) const  { return detail::isend<T>()(comm_, dest, tag, x); }

      //! Non-blocking version of `recv()`.
      //! If `T` is an `std::vector<...>`, its size must be big enough to accomodate the sent values.
      template<class T>
      request   irecv(int source, int tag, T& x) const      { return detail::irecv<T>()(comm_, source, tag, x); }

      //! probe
      status    probe(int source, int tag) const            { status s; MPI_Probe(source, tag, comm_, &s.s); return s; }

      //! iprobe
      inline
      optional<status>
                iprobe(int source, int tag) const;

      //! barrier
      void      barrier() const                             { MPI_Barrier(comm_); }

                operator MPI_Comm() const                   { return comm_; }

    private:
      MPI_Comm  comm_;
      int       rank_;
      int       size_;
  };
}
}

diy::mpi::optional<diy::mpi::status>
diy::mpi::communicator::
iprobe(int source, int tag) const
{
  status s;
  int flag;
  MPI_Iprobe(source, tag, comm_, &flag, &s.s);
  if (flag)
    return s;
  return optional<status>();
}


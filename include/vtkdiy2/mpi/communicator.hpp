namespace diy
{
namespace mpi
{

  //! \ingroup MPI
  //! Simple wrapper around `MPI_Comm`.
  class communicator
  {
    public:
                inline
                communicator(MPI_Comm comm = MPI_COMM_WORLD, bool owner = false);

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

    communicator&
                operator=(const communicator& other)        { destroy(); comm_ = other.comm_; rank_ = other.rank_; size_ = other.size_; owner_ = false; return *this; }
    communicator&
                operator=(communicator&& other)             { destroy(); comm_ = other.comm_; rank_ = other.rank_; size_ = other.size_; owner_ = other.owner_; other.owner_ = false; return *this; }

      int       rank() const                        { return rank_; }
      int       size() const                        { return size_; }

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

      //! Non-blocking version of `ssend()`.
      template<class T>
      request   issend(int dest, int tag, const T& x) const  { return detail::issend<T>()(comm_, dest, tag, x); }

      //! Non-blocking version of `recv()`.
      //! If `T` is an `std::vector<...>`, its size must be big enough to accommodate the sent values.
      template<class T>
      request   irecv(int source, int tag, T& x) const      { return detail::irecv<T>()(comm_, source, tag, x); }

      //! probe
      inline
      status    probe(int source, int tag) const;

      //! iprobe
      inline
      optional<status>
                iprobe(int source, int tag) const;

      //! barrier
      inline
      void      barrier() const;

      //! Nonblocking version of barrier
      inline
      request   ibarrier() const;

                operator MPI_Comm() const                   { return comm_; }

      //! split
      //! When keys are the same, the ties are broken by the rank in the original comm.
      inline
      communicator
                split(int color, int key = 0) const;

      //! duplicate
      inline
      void      duplicate(const communicator& other);

    private:
      inline
      void      destroy();

    private:
      MPI_Comm  comm_;
      int       rank_;
      int       size_;
      bool      owner_;
  };
}
}

diy::mpi::communicator::
communicator(MPI_Comm comm, bool owner):
    comm_(comm), rank_(0), size_(1), owner_(owner)
{
#ifndef DIY_NO_MPI
  if (comm != MPI_COMM_NULL)
  {
    MPI_Comm_rank(comm_, &rank_);
    MPI_Comm_size(comm_, &size_);
  }
#endif
}

void
diy::mpi::communicator::
destroy()
{
#ifndef DIY_NO_MPI
    if (owner_)
        MPI_Comm_free(&comm_);
#endif
}

diy::mpi::status
diy::mpi::communicator::
probe(int source, int tag) const
{
  (void) source;
  (void) tag;

#ifndef DIY_NO_MPI
  status s;
  MPI_Probe(source, tag, comm_, &s.s);
  return s;
#else
  DIY_UNSUPPORTED_MPI_CALL(MPI_Probe);
#endif
}

diy::mpi::optional<diy::mpi::status>
diy::mpi::communicator::
iprobe(int source, int tag) const
{
  (void) source;
  (void) tag;
#ifndef DIY_NO_MPI
  status s;
  int flag;
  MPI_Iprobe(source, tag, comm_, &flag, &s.s);
  if (flag)
    return s;
#endif
  return optional<status>();
}

void
diy::mpi::communicator::
barrier() const
{
#ifndef DIY_NO_MPI
  MPI_Barrier(comm_);
#endif
}

diy::mpi::communicator
diy::mpi::communicator::
split(int color, int key) const
{
#ifndef DIY_NO_MPI
    MPI_Comm newcomm;
    MPI_Comm_split(comm_, color, key, &newcomm);
    return communicator(newcomm, true);
#else
    return communicator();
#endif
}

diy::mpi::request
diy::mpi::communicator::
ibarrier() const
{
#ifndef DIY_NO_MPI
    request r_;
    MPI_Ibarrier(comm_, &r_.r);
    return r_;
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
#ifndef DIY_NO_MPI
    MPI_Comm newcomm;
    MPI_Comm_dup(other.comm_, &newcomm);
    (*this) = communicator(newcomm,true);
#endif
}

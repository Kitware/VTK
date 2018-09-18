namespace diy
{
namespace mpi
{
  struct status
  {
    int             source() const          { return s.MPI_SOURCE; }
    int             tag() const             { return s.MPI_TAG; }
    int             error() const           { return s.MPI_ERROR; }

    inline
    bool            cancelled() const;

    template<class T>
    int             count() const;

                    operator MPI_Status&()              { return s; }
                    operator const MPI_Status&() const  { return s; }

    MPI_Status      s;
  };
}
}


bool
diy::mpi::status::cancelled() const
{
#ifndef DIY_NO_MPI
  int flag;
  MPI_Test_cancelled(const_cast<MPI_Status*>(&s), &flag);
  return flag;
#else
  DIY_UNSUPPORTED_MPI_CALL(diy::mpi::status::cancelled);
#endif
}

template<class T>
int
diy::mpi::status::count() const
{
#ifndef DIY_NO_MPI
  int c;
  MPI_Get_count(const_cast<MPI_Status*>(&s), detail::get_mpi_datatype<T>(), &c);
  return c;
#else
  DIY_UNSUPPORTED_MPI_CALL(diy::mpi::status::count);
#endif
}

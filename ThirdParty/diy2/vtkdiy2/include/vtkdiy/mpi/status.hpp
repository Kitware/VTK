namespace diy
{
namespace mpi
{
  struct status
  {
    int             source() const          { return s.MPI_SOURCE; }
    int             tag() const             { return s.MPI_TAG; }
    int             error() const           { return s.MPI_ERROR; }
    bool            cancelled() const       { int flag; MPI_Test_cancelled(const_cast<MPI_Status*>(&s), &flag); return flag; }

    template<class T>
    int             count() const;

                    operator MPI_Status&()              { return s; }
                    operator const MPI_Status&() const  { return s; }

    MPI_Status      s;
  };
}
}

template<class T>
int
diy::mpi::status::count() const
{
  int c;
  MPI_Get_count(const_cast<MPI_Status*>(&s), detail::get_mpi_datatype<T>(), &c);
  return c;
}

namespace diy
{
namespace mpi
{
  struct request
  {
    inline
    status              wait();
    inline
    optional<status>    test();
    inline
    void                cancel();

    MPI_Request         r;
  };
}
}

diy::mpi::status
diy::mpi::request::wait()
{
#ifndef DIY_NO_MPI
  status s;
  MPI_Wait(&r, &s.s);
  return s;
#else
  DIY_UNSUPPORTED_MPI_CALL(diy::mpi::request::wait);
#endif
}

diy::mpi::optional<diy::mpi::status>
diy::mpi::request::test()
{
#ifndef DIY_NO_MPI
  status s;
  int flag;
  MPI_Test(&r, &flag, &s.s);
  if (flag)
    return s;
#endif
  return optional<status>();
}

void
diy::mpi::request::cancel()
{
#ifndef DIY_NO_MPI
  MPI_Cancel(&r);
#endif
}

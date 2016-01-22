namespace diy
{
namespace mpi
{
  struct request
  {
    status              wait()              { status s; MPI_Wait(&r, &s.s); return s; }
    inline
    optional<status>    test();
    void                cancel()            { MPI_Cancel(&r); }

    MPI_Request         r;
  };
}
}

diy::mpi::optional<diy::mpi::status>
diy::mpi::request::test()
{
  status s;
  int flag;
  MPI_Test(&r, &flag, &s.s);
  if (flag)
    return s;
  return optional<status>();
}

#ifdef DIY_MPI_AS_LIB
#include "environment.hpp"
#endif

bool diy::mpi::environment::initialized()
{
#if DIY_HAS_MPI
  int flag;
  MPI_Initialized(&flag);
  return flag != 0;
#else
  return true;
#endif
}

diy::mpi::environment::environment()
{
#if DIY_HAS_MPI
  int argc = 0; char** argv = nullptr;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided_threading);
#else
  provided_threading = MPI_THREAD_FUNNELED;
#endif
}

diy::mpi::environment::environment(int requested_threading)
{
#if DIY_HAS_MPI
  int argc = 0; char** argv = nullptr;
  MPI_Init_thread(&argc, &argv, requested_threading, &provided_threading);
#else
  provided_threading = requested_threading;
#endif
}

diy::mpi::environment::environment(int argc, char* argv[])
{
#if DIY_HAS_MPI
  MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided_threading);
#else
  (void) argc; (void) argv;
  provided_threading = MPI_THREAD_FUNNELED;
#endif
}

diy::mpi::environment::environment(int argc, char* argv[], int requested_threading)
{
#if DIY_HAS_MPI
  MPI_Init_thread(&argc, &argv, requested_threading, &provided_threading);
#else
  (void) argc; (void) argv;
  provided_threading = requested_threading;
#endif
}

diy::mpi::environment::
~environment()
{
#if DIY_HAS_MPI
  MPI_Finalize();
#endif
}

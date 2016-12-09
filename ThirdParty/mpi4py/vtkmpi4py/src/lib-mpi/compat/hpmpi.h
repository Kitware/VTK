#ifndef PyMPI_COMPAT_HPMPI_H
#define PyMPI_COMPAT_HPMPI_H

/* ---------------------------------------------------------------- */

#ifndef MPI_INTEGER1
#define MPI_INTEGER1 ((MPI_Datatype)MPI_Type_f2c(MPIF_INTEGER1))
#endif
#ifndef MPI_INTEGER2
#define MPI_INTEGER2 ((MPI_Datatype)MPI_Type_f2c(MPIF_INTEGER2))
#endif
#ifndef MPI_INTEGER4
#define MPI_INTEGER4 ((MPI_Datatype)MPI_Type_f2c(MPIF_INTEGER4))
#endif
#ifndef MPI_REAL4
#define MPI_REAL4    ((MPI_Datatype)MPI_Type_f2c(MPIF_REAL4))
#endif
#ifndef MPI_REAL8
#define MPI_REAL8    ((MPI_Datatype)MPI_Type_f2c(MPIF_REAL8))
#endif

/* ---------------------------------------------------------------- */

#ifndef HPMPI_DLOPEN_LIBMPI
#define HPMPI_DLOPEN_LIBMPI 1
#endif

#if HPMPI_DLOPEN_LIBMPI
#if HAVE_DLOPEN

#include "../../dynload.h"

static void HPMPI_dlopen_libmpi(void)
{
  void *handle = 0;
  int mode = RTLD_NOW | RTLD_GLOBAL;
  #ifdef RTLD_NOLOAD
  mode |= RTLD_NOLOAD;
  #endif
#if defined(__APPLE__)
  /* Mac OS X */
  if (!handle) handle = dlopen("libhpmpi.3.dylib", mode);
  if (!handle) handle = dlopen("libhpmpi.2.dylib", mode);
  if (!handle) handle = dlopen("libhpmpi.1.dylib", mode);
  if (!handle) handle = dlopen("libhpmpi.0.dylib", mode);
  if (!handle) handle = dlopen("libhpmpi.dylib",   mode);
#else
  /* GNU/Linux and others*/
  if (!handle) handle = dlopen("libhpmpi.so.3", mode);
  if (!handle) handle = dlopen("libhpmpi.so.2", mode);
  if (!handle) handle = dlopen("libhpmpi.so.1", mode);
  if (!handle) handle = dlopen("libhpmpi.so.0", mode);
  if (!handle) handle = dlopen("libhpmpi.so",   mode);
#endif
}

static int PyMPI_HPMPI_MPI_Init(int *argc, char ***argv)
{
  HPMPI_dlopen_libmpi();
  return MPI_Init(argc, argv);
}
#undef  MPI_Init
#define MPI_Init PyMPI_HPMPI_MPI_Init

static int PyMPI_HPMPI_MPI_Init_thread(int *argc, char ***argv,
                                         int required, int *provided)
{
  HPMPI_dlopen_libmpi();
  return MPI_Init_thread(argc, argv, required, provided);
}
#undef  MPI_Init_thread
#define MPI_Init_thread PyMPI_HPMPI_MPI_Init_thread

#endif /* !HAVE_DLOPEN */
#endif /* !HPMPI_DLOPEN_LIBMPI */

/* ---------------------------------------------------------------- */

#endif /* !PyMPI_COMPAT_HPMPI_H */

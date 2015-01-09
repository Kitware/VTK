#ifndef PyMPI_COMPAT_OPENMPI_H
#define PyMPI_COMPAT_OPENMPI_H

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/*
 * The hackery below redefines the actuall calls to 'MPI_Init()' and
 * 'MPI_Init_thread()' in order to preload the main MPI dynamic
 * library with appropriate flags to 'dlopen()' ensuring global
 * availability of library symbols.
 */

#ifndef OPENMPI_DLOPEN_LIBMPI
#define OPENMPI_DLOPEN_LIBMPI 1
#endif

#if OPENMPI_DLOPEN_LIBMPI
#if HAVE_DLOPEN

#include "../dynload.h"

/*
static void * my_dlopen(const char *name, int mode) {
  void *handle;
  static int called = 0;
  if (!called) {
    called = 1;
    #if HAVE_DLFCN_H
    printf("HAVE_DLFCN_H: yes\n");
    #else
    printf("HAVE_DLFCN_H: no\n");
    #endif
    printf("\n");
    printf("RTLD_LAZY:    0x%X\n", RTLD_LAZY   );
    printf("RTLD_NOW:     0x%X\n", RTLD_NOW    );
    printf("RTLD_LOCAL:   0x%X\n", RTLD_LOCAL  );
    printf("RTLD_GLOBAL:  0x%X\n", RTLD_GLOBAL );
    #ifdef RTLD_NOLOAD
    printf("RTLD_NOLOAD:  0x%X\n", RTLD_NOLOAD );
    #endif
    printf("\n");
  }
  handle = dlopen(name, mode);
  printf("dlopen(\"%s\",0x%X) -> %p\n", name, mode, handle);
  printf("dlerror() -> %s\n\n", dlerror());
  return handle;
}
#define dlopen my_dlopen
*/

static void OPENMPI_dlopen_libmpi(void)
{
  void *handle = 0;
  int mode = RTLD_NOW | RTLD_GLOBAL;
  #ifdef RTLD_NOLOAD
  mode |= RTLD_NOLOAD;
  #endif
#if defined(__CYGWIN__)
  if (!handle) handle = dlopen("cygmpi.dll", mode);
  if (!handle) handle = dlopen("mpi.dll",    mode);
#elif defined(__APPLE__)
  /* Mac OS X */
  if (!handle) handle = dlopen("libmpi.3.dylib", mode);
  if (!handle) handle = dlopen("libmpi.2.dylib", mode);
  if (!handle) handle = dlopen("libmpi.1.dylib", mode);
  if (!handle) handle = dlopen("libmpi.0.dylib", mode);
  if (!handle) handle = dlopen("libmpi.dylib",   mode);
#else
  /* GNU/Linux and others*/
  if (!handle) handle = dlopen("libmpi.so.3", mode);
  if (!handle) handle = dlopen("libmpi.so.2", mode);
  if (!handle) handle = dlopen("libmpi.so.1", mode);
  if (!handle) handle = dlopen("libmpi.so.0", mode);
  if (!handle) handle = dlopen("libmpi.so",   mode);
#endif
}

static int PyMPI_OPENMPI_MPI_Init(int *argc, char ***argv)
{
  OPENMPI_dlopen_libmpi();
  return MPI_Init(argc, argv);
}
#undef  MPI_Init
#define MPI_Init PyMPI_OPENMPI_MPI_Init

static int PyMPI_OPENMPI_MPI_Init_thread(int *argc, char ***argv,
                                         int required, int *provided)
{
  OPENMPI_dlopen_libmpi();
  return MPI_Init_thread(argc, argv, required, provided);
}
#undef  MPI_Init_thread
#define MPI_Init_thread PyMPI_OPENMPI_MPI_Init_thread

#endif /* !HAVE_DLOPEN */
#endif /* !OPENMPI_DLOPEN_LIBMPI */

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */

#if (defined(OMPI_MAJOR_VERSION) && \
     defined(OMPI_MINOR_VERSION) && \
     defined(OMPI_RELEASE_VERSION))
#define PyMPI_OPENMPI_VERSION ((OMPI_MAJOR_VERSION   * 10000) + \
                               (OMPI_MINOR_VERSION   * 100)   + \
                               (OMPI_RELEASE_VERSION * 1))
#else
#define PyMPI_OPENMPI_VERSION 10000
#endif

/* ------------------------------------------------------------------------- */

/*
 * Open MPI < 1.1.3 generates an error when MPI_File_get_errhandler()
 * is called with the predefined error handlers MPI_ERRORS_RETURN and
 * MPI_ERRORS_ARE_FATAL.
 */

#if PyMPI_OPENMPI_VERSION < 10103

static int PyMPI_OPENMPI_Errhandler_free(MPI_Errhandler *errhandler)
{
  if (errhandler && ((*errhandler == MPI_ERRORS_RETURN) ||
                     (*errhandler == MPI_ERRORS_ARE_FATAL))) {
    *errhandler = MPI_ERRHANDLER_NULL;
    return MPI_SUCCESS;
  }
  return MPI_Errhandler_free(errhandler);
}
#undef  MPI_Errhandler_free
#define MPI_Errhandler_free PyMPI_OPENMPI_Errhandler_free

#endif /* !(PyMPI_OPENMPI_VERSION < 10103) */

/* ------------------------------------------------------------------------- */

/*
 * Open MPI 1.1 generates an error when MPI_File_get_errhandler() is
 * called with the MPI_FILE_NULL handle.  The code below try to fix
 * this bug by intercepting the calls to the functions setting and
 * getting the error handlers for MPI_File's.
 */

#if PyMPI_OPENMPI_VERSION < 10200

static MPI_Errhandler PyMPI_OPENMPI_FILE_NULL_ERRHANDLER = (MPI_Errhandler)0;

static int PyMPI_OPENMPI_File_get_errhandler(MPI_File file,
                                             MPI_Errhandler *errhandler)
{
  if (file == MPI_FILE_NULL) {
    if (PyMPI_OPENMPI_FILE_NULL_ERRHANDLER == (MPI_Errhandler)0) {
      PyMPI_OPENMPI_FILE_NULL_ERRHANDLER = MPI_ERRORS_RETURN;
    }
    *errhandler = PyMPI_OPENMPI_FILE_NULL_ERRHANDLER;
    return MPI_SUCCESS;
  }
  return MPI_File_get_errhandler(file, errhandler);
}
#undef  MPI_File_get_errhandler
#define MPI_File_get_errhandler PyMPI_OPENMPI_File_get_errhandler

static int PyMPI_OPENMPI_File_set_errhandler(MPI_File file,
                                             MPI_Errhandler errhandler)
{
  int ierr = MPI_File_set_errhandler(file, errhandler);
  if (ierr != MPI_SUCCESS) return ierr;
  if (file == MPI_FILE_NULL) {
    PyMPI_OPENMPI_FILE_NULL_ERRHANDLER = errhandler;
  }
  return ierr;
}
#undef  MPI_File_set_errhandler
#define MPI_File_set_errhandler PyMPI_OPENMPI_File_set_errhandler

#endif /* !(PyMPI_OPENMPI_VERSION < 10200) */

/* ---------------------------------------------------------------- */

#if PyMPI_OPENMPI_VERSION < 10301

static MPI_Fint PyMPI_OPENMPI_File_c2f(MPI_File file)
{
  if (file == MPI_FILE_NULL) return (MPI_Fint)0;
  return MPI_File_c2f(file);
}
#define MPI_File_c2f PyMPI_OPENMPI_File_c2f

#endif /* !(PyMPI_OPENMPI_VERSION < 10301) */

/* ------------------------------------------------------------------------- */

#if PyMPI_OPENMPI_VERSION < 10402

static int PyMPI_OPENMPI_MPI_Cancel(MPI_Request *request)
{
  if (request && *request == MPI_REQUEST_NULL) {
    MPI_Comm_call_errhandler(MPI_COMM_WORLD, MPI_ERR_REQUEST);
    return MPI_ERR_REQUEST;
  }
  return MPI_Cancel(request);
}
#undef  MPI_Cancel
#define MPI_Cancel PyMPI_OPENMPI_MPI_Cancel

static int PyMPI_OPENMPI_MPI_Request_free(MPI_Request *request)
{
  if (request && *request == MPI_REQUEST_NULL) {
    MPI_Comm_call_errhandler(MPI_COMM_WORLD, MPI_ERR_REQUEST);
    return MPI_ERR_REQUEST;
  }
  return MPI_Request_free(request);
}
#undef  MPI_Request_free
#define MPI_Request_free PyMPI_OPENMPI_MPI_Request_free

static int PyMPI_OPENMPI_MPI_Win_get_errhandler(MPI_Win win,
                                                MPI_Errhandler *errhandler)
{
  if (win == MPI_WIN_NULL) {
    MPI_Comm_call_errhandler(MPI_COMM_WORLD, MPI_ERR_WIN);
    return MPI_ERR_WIN;
  }
  return MPI_Win_get_errhandler(win, errhandler);
}
#undef  MPI_Win_get_errhandler
#define MPI_Win_get_errhandler PyMPI_OPENMPI_MPI_Win_get_errhandler

static int PyMPI_OPENMPI_MPI_Win_set_errhandler(MPI_Win win,
                                                MPI_Errhandler errhandler)
{
  if (win == MPI_WIN_NULL) {
    MPI_Comm_call_errhandler(MPI_COMM_WORLD, MPI_ERR_WIN);
    return MPI_ERR_WIN;
  }
  return MPI_Win_set_errhandler(win, errhandler);
}
#undef  MPI_Win_set_errhandler
#define MPI_Win_set_errhandler PyMPI_OPENMPI_MPI_Win_set_errhandler

#endif /* !(PyMPI_OPENMPI_VERSION < 10402) */

/* ------------------------------------------------------------------------- */

#endif /* !PyMPI_COMPAT_OPENMPI_H */

/*
  Local Variables:
  c-basic-offset: 2
  indent-tabs-mode: nil
  End:
*/

/* Author:  Lisandro Dalcin   */
/* Contact: dalcinl@gmail.com */

#ifndef MPI4PY_H
#define MPI4PY_H

#include "vtk_mpi.h"

#if defined(MSMPI_VER) && !defined(PyMPI_HAVE_MPI_Message)
#  if defined(MPI_MESSAGE_NULL)
#    define PyMPI_HAVE_MPI_Message 1
#  endif
#endif

#if defined(MSMPI_VER) && !defined(PyMPI_HAVE_MPI_Session)
#  if defined(MPI_SESSION_NULL)
#    define PyMPI_HAVE_MPI_Session 1
#  endif
#endif

#if (MPI_VERSION < 3) && !defined(PyMPI_HAVE_MPI_Message)
typedef void *PyMPI_MPI_Message;
#define MPI_Message PyMPI_MPI_Message
#endif

#if (MPI_VERSION < 4) && !defined(PyMPI_HAVE_MPI_Session)
typedef void *PyMPI_MPI_Session;
#define MPI_Session PyMPI_MPI_Session
#endif

#if defined(MPI4PY_LIMITED_API)
#include "pycapi.h"
#else
#include "api/MPI_api.h"
#endif

static int import_mpi4py(void) {
  return import_mpi4py__MPI();
}

#endif /* MPI4PY_H */

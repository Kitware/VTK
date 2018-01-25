/* Author:  Lisandro Dalcin   */
/* Contact: dalcinl@gmail.com */

/* ------------------------------------------------------------------------- */

#include "Python.h"
#include "mpi.h"

/* ------------------------------------------------------------------------- */

#include "lib-mpi/config.h"
#include "lib-mpi/missing.h"
#include "lib-mpi/fallback.h"
#include "lib-mpi/compat.h"

#include "pympivendor.h"
#include "pympicommctx.h"

/* ------------------------------------------------------------------------- */

#include "pycompat.h"

#ifdef PYPY_VERSION
  #define PyMPI_RUNTIME_PYPY    1
  #define PyMPI_RUNTIME_CPYTHON 0
#else
  #define PyMPI_RUNTIME_PYPY    0
  #define PyMPI_RUNTIME_CPYTHON 1
#endif

/* ------------------------------------------------------------------------- */

#if !defined(PyMPI_USE_MATCHED_RECV)
  #if defined(PyMPI_HAVE_MPI_Mprobe) && \
      defined(PyMPI_HAVE_MPI_Mrecv)  && \
      MPI_VERSION >= 3
    #define PyMPI_USE_MATCHED_RECV 1
  #else
    #define PyMPI_USE_MATCHED_RECV 0
  #endif
#endif

/* ------------------------------------------------------------------------- */

static PyObject *
PyMPIString_AsStringAndSize(PyObject *ob, const char **s, Py_ssize_t *n)
{
  PyObject *b = NULL;
  if (PyUnicode_Check(ob)) {
#if PY_MAJOR_VERSION >= 3
    b = PyUnicode_AsUTF8String(ob);
#else
    b = PyUnicode_AsASCIIString(ob);
#endif
    if (!b) return NULL;
  } else {
    b = ob; Py_INCREF(ob);
  }
#if PY_MAJOR_VERSION >= 3
  if (PyBytes_AsStringAndSize(b, (char **)s, n) < 0) {
#else
  if (PyString_AsStringAndSize(b, (char **)s, n) < 0) {
#endif
    Py_DECREF(b);
    return NULL;
  }
  return b;
}

#if PY_MAJOR_VERSION >= 3
#define PyMPIString_FromString        PyUnicode_FromString
#define PyMPIString_FromStringAndSize PyUnicode_FromStringAndSize
#else
#define PyMPIString_FromString        PyString_FromString
#define PyMPIString_FromStringAndSize PyString_FromStringAndSize
#endif

/* ------------------------------------------------------------------------- */

/*
  Local variables:
  c-basic-offset: 2
  indent-tabs-mode: nil
  End:
*/

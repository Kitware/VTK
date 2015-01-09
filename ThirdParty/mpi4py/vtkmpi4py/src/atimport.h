/* ------------------------------------------------------------------------- */

#include "Python.h"

#include "mpi.h"

/* ------------------------------------------------------------------------- */

#if defined(MPICH_NAME) && (MPICH_NAME==1)
#define MPICH1 1
#endif

#if defined(MS_WINDOWS) && defined(MPICH2) && defined(MPIAPI)
#define MS_MPI 1
#endif

#if defined(MS_WINDOWS) && defined(DEINO_MPI) && !defined(MPICH2)
#define MPICH2 1
#endif

/* XXX describe */
#if defined(HAVE_CONFIG_H)
#include "config.h"
#elif defined(MPICH2)
#include "config/mpich2.h"
#elif defined(OPEN_MPI)
#include "config/openmpi.h"
#elif defined(MPICH1)
#include "config/mpich1.h"
#elif defined(LAM_MPI)
#include "config/lammpi.h"
#else /* Unknown MPI*/
#include "config/unknown.h"
#endif

/* XXX describe */
#include "missing.h"
#include "fallback.h"

/* XXX describe */
#if   defined(MPICH2)
#include "compat/mpich2.h"
#elif defined(OPEN_MPI)
#include "compat/openmpi.h"
#elif defined(MPICH1)
#include "compat/mpich1.h"
#elif defined(LAM_MPI)
#include "compat/lammpi.h"
#elif defined(HP_MPI)
#include "compat/hpmpi.h"
#endif

/* ------------------------------------------------------------------------- */

#if defined(MS_WINDOWS) && !defined(PyMPIAPI)
  #if defined(MPI_CALL)   /* DeinoMPI */
    #define PyMPIAPI MPI_CALL
  #elif defined(MPIAPI)   /* Microsoft MPI */
    #define PyMPIAPI MPIAPI
  #endif
#endif
#if !defined(PyMPIAPI)
  #define PyMPIAPI
#endif

/* ------------------------------------------------------------------------- */

/*
  It could be a good idea to implement the startup and cleanup phases
  employing PMPI_Xxx calls, thus MPI profilers would not notice.

  1) The MPI calls at the startup phase could be (a bit of initial)
     junk for users trying to profile the calls for their own code.

  2) Some (naive?) MPI profilers could get confused if MPI_Xxx routines
     are called inside MPI_Finalize during the cleanup phase.

  If for whatever reason you need it, just change the values of the
  defines below to the corresponding PMPI_Xxx symbols.
*/

#define P_MPI_Comm_get_errhandler MPI_Comm_get_errhandler
#define P_MPI_Comm_set_errhandler MPI_Comm_set_errhandler
#define P_MPI_Errhandler_free     MPI_Errhandler_free
#define P_MPI_Comm_create_keyval  MPI_Comm_create_keyval
#define P_MPI_Comm_free_keyval    MPI_Comm_free_keyval
#define P_MPI_Comm_set_attr       MPI_Comm_set_attr
#define P_MPI_Win_free_keyval     MPI_Win_free_keyval

static MPI_Errhandler PyMPI_ERRHDL_COMM_WORLD = (MPI_Errhandler)0;
static MPI_Errhandler PyMPI_ERRHDL_COMM_SELF  = (MPI_Errhandler)0;
static int PyMPI_KEYVAL_MPI_ATEXIT = MPI_KEYVAL_INVALID;
static int PyMPI_KEYVAL_WIN_MEMORY = MPI_KEYVAL_INVALID;

static int PyMPI_StartUp(void);
static int PyMPI_CleanUp(void);
static int PyMPIAPI PyMPI_AtExitMPI(MPI_Comm,int,void*,void*);

static int PyMPI_STARTUP_DONE = 0;
static int PyMPI_StartUp(void)
{
  if (PyMPI_STARTUP_DONE) return MPI_SUCCESS;
  PyMPI_STARTUP_DONE = 1;
  /* change error handlers for predefined communicators */
  if (PyMPI_ERRHDL_COMM_WORLD == (MPI_Errhandler)0)
    PyMPI_ERRHDL_COMM_WORLD = MPI_ERRHANDLER_NULL;
  if (PyMPI_ERRHDL_COMM_WORLD == MPI_ERRHANDLER_NULL) {
    (void)P_MPI_Comm_get_errhandler(MPI_COMM_WORLD, &PyMPI_ERRHDL_COMM_WORLD);
    (void)P_MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
  }
  if (PyMPI_ERRHDL_COMM_SELF == (MPI_Errhandler)0)
    PyMPI_ERRHDL_COMM_SELF = MPI_ERRHANDLER_NULL;
  if (PyMPI_ERRHDL_COMM_SELF == MPI_ERRHANDLER_NULL) {
    (void)P_MPI_Comm_get_errhandler(MPI_COMM_SELF, &PyMPI_ERRHDL_COMM_SELF);
    (void)P_MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
  }
  /* make the call to MPI_Finalize() run a cleanup function */
  if (PyMPI_KEYVAL_MPI_ATEXIT == MPI_KEYVAL_INVALID) {
    int keyval = MPI_KEYVAL_INVALID;
    (void)P_MPI_Comm_create_keyval(MPI_COMM_NULL_COPY_FN,
                                    PyMPI_AtExitMPI, &keyval, 0);
    (void)P_MPI_Comm_set_attr(MPI_COMM_SELF, keyval, 0);
    PyMPI_KEYVAL_MPI_ATEXIT = keyval;
  }
  return MPI_SUCCESS;
}

static int PyMPI_CLEANUP_DONE = 0;
static int PyMPI_CleanUp(void)
{
  if (PyMPI_CLEANUP_DONE) return MPI_SUCCESS;
  PyMPI_CLEANUP_DONE = 1;
  /* free atexit keyval */
  if (PyMPI_KEYVAL_MPI_ATEXIT != MPI_KEYVAL_INVALID) {
    (void)P_MPI_Comm_free_keyval(&PyMPI_KEYVAL_MPI_ATEXIT);
    PyMPI_KEYVAL_MPI_ATEXIT = MPI_KEYVAL_INVALID;
  }
  /* free windows keyval */
  if (PyMPI_KEYVAL_WIN_MEMORY != MPI_KEYVAL_INVALID) {
    (void)P_MPI_Win_free_keyval(&PyMPI_KEYVAL_WIN_MEMORY);
    PyMPI_KEYVAL_WIN_MEMORY = MPI_KEYVAL_INVALID;
  }
  /* restore default error handlers for predefined communicators */
  if (PyMPI_ERRHDL_COMM_SELF != MPI_ERRHANDLER_NULL) {
    (void)P_MPI_Comm_set_errhandler(MPI_COMM_SELF, PyMPI_ERRHDL_COMM_SELF);
    (void)P_MPI_Errhandler_free(&PyMPI_ERRHDL_COMM_SELF);
    PyMPI_ERRHDL_COMM_SELF = MPI_ERRHANDLER_NULL;
  }
  if (PyMPI_ERRHDL_COMM_WORLD != MPI_ERRHANDLER_NULL) {
    (void)P_MPI_Comm_set_errhandler(MPI_COMM_WORLD, PyMPI_ERRHDL_COMM_WORLD);
    (void)P_MPI_Errhandler_free(&PyMPI_ERRHDL_COMM_WORLD);
    PyMPI_ERRHDL_COMM_WORLD = MPI_ERRHANDLER_NULL;
  }
  return MPI_SUCCESS;
}

static int PyMPIAPI
PyMPI_AtExitMPI(MPI_Comm comm, int k, void *v, void *xs)
{
  comm=MPI_COMM_NULL; k=0; v=0; xs=0;
  return PyMPI_CleanUp();
}

/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */

static PyObject *
PyMPI_Allocate(Py_ssize_t m, size_t b, void **pp)
{
  PyObject *ob;
  Py_ssize_t n = m * (Py_ssize_t)b;
  if (n > PY_SSIZE_T_MAX)
    return PyErr_NoMemory();
  else if (n < 0) {
    PyErr_SetString(PyExc_RuntimeError,
                    "memory allocation with negative size");
    return NULL;
  }
#if PY_VERSION_HEX >= 0x02070000
  ob = PyByteArray_FromStringAndSize(NULL, (n==0) ? 1 : n);
  if (ob && n==0 && (PyByteArray_Resize(ob, 0) < 0)) {
    Py_DECREF(ob);
    return NULL;
  }
  if (ob && pp)
    *pp = (void *)PyByteArray_AS_STRING(ob);
#else
  {
    void *p = PyMem_Malloc((size_t)n);
    if (!p)
      return PyErr_NoMemory();
    ob = PyCObject_FromVoidPtr(p, PyMem_Free);
    if (!ob)
      PyMem_Free(p);
    else if (pp)
      *pp = p;
  }
#endif
  return ob;
}

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

#if PY_VERSION_HEX < 0x02040000
#ifndef Py_CLEAR
#define Py_CLEAR(op)                        \
  do {                                      \
    if (op) {                               \
      PyObject *_py_tmp = (PyObject *)(op); \
      (op) = NULL;                          \
      Py_DECREF(_py_tmp);                   \
    }                                       \
  } while (0)
#endif
#endif

#if PY_VERSION_HEX < 0x02060000

#ifndef PyExc_BufferError
#define PyExc_BufferError PyExc_TypeError
#endif/*PyExc_BufferError*/

#ifndef PyBuffer_FillInfo
static int
PyBuffer_FillInfo(Py_buffer *view, PyObject *obj,
                  void *buf, Py_ssize_t len,
                  int readonly, int flags)
{
  if (view == NULL) return 0;
  if (((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE) &&
      (readonly == 1)) {
    PyErr_SetString(PyExc_BufferError,
                    "Object is not writable.");
    return -1;
  }

  view->obj = obj;
  if (obj)
    Py_INCREF(obj);

  view->buf = buf;
  view->len = len;
  view->itemsize = 1;
  view->readonly = readonly;
  view->format = NULL;
  if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
    view->format = "B";

  view->ndim = 1;
  view->shape = NULL;
  if ((flags & PyBUF_ND) == PyBUF_ND)
    view->shape = &(view->len);
  view->strides = NULL;
  if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES)
    view->strides = &(view->itemsize);
  view->suboffsets = NULL;

  view->internal = NULL;
  return 0;
}
#endif/*PyBuffer_FillInfo*/

#ifndef PyBuffer_Release
static void
PyBuffer_Release(Py_buffer *view)
{
  PyObject *obj = view->obj;
  Py_XDECREF(obj);
  view->obj = NULL;
}
#endif/*PyBuffer_Release*/

#ifndef PyObject_CheckBuffer
#define PyObject_CheckBuffer(ob) (0)
#endif/*PyObject_CheckBuffer*/

#ifndef PyObject_GetBuffer
#define PyObject_GetBuffer(ob,view,flags) \
        (PyErr_SetString(PyExc_NotImplementedError, \
                        "new buffer interface is not available"), -1)
#endif/*PyObject_GetBuffer*/

#endif

#if PY_VERSION_HEX < 0x02070000
static PyObject *
PyMemoryView_FromBuffer(Py_buffer *view)
{
  if (view->obj) {
    if (view->readonly)
      return PyBuffer_FromObject(view->obj, 0, view->len);
    else
      return PyBuffer_FromReadWriteObject(view->obj, 0, view->len);
  } else {
    if (view->readonly)
      return PyBuffer_FromMemory(view->buf,view->len);
    else
      return PyBuffer_FromReadWriteMemory(view->buf,view->len);
  }
}
#endif

/* ------------------------------------------------------------------------- */

#if !defined(WITH_THREAD)
#undef  PyGILState_Ensure
#define PyGILState_Ensure() ((PyGILState_STATE)0)
#undef  PyGILState_Release
#define PyGILState_Release(state) (state)=((PyGILState_STATE)0)
#undef  Py_BLOCK_THREADS
#define Py_BLOCK_THREADS
#undef  Py_UNBLOCK_THREADS
#define Py_UNBLOCK_THREADS
#endif

/* ------------------------------------------------------------------------- */

/*
  Local variables:
  c-basic-offset: 2
  indent-tabs-mode: nil
  End:
*/

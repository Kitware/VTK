/* Author:  Lisandro Dalcin   */
/* Contact: dalcinl@gmail.com */

/* ------------------------------------------------------------------------- */

#ifdef PYPY_VERSION

#ifndef PyByteArray_Check
#define PyByteArray_Check(self) PyObject_TypeCheck(self, &PyByteArray_Type)
#endif

#ifndef PyByteArray_AsString
static char* PyByteArray_AsString(PyObject* o)
{
  PyErr_SetString(PyExc_RuntimeError,
                  "PyPy: PyByteArray_AsString() not available");
  (void)o; return NULL;
}
#endif

#ifndef PyByteArray_Size
static Py_ssize_t PyByteArray_Size(PyObject* o)
{
  PyErr_SetString(PyExc_RuntimeError,
                  "PyPy: PyByteArray_Size() not available");
  (void)o; return -1;
}
#endif

#endif/*PYPY_VERSION*/

/* ------------------------------------------------------------------------- */

/* Legacy Python 2 buffer interface */

static int _Py2_IsBuffer(PyObject *obj)
{
#if PY_VERSION_HEX < 0x03000000
  return PyObject_CheckReadBuffer(obj);
#else
  (void)obj;
  return 0;
#endif
}

static int _Py2_AsBuffer(PyObject *obj, int readonly,
                         void **buf, Py_ssize_t *size)
{
#if defined(PYPY_VERSION) || PY_VERSION_HEX < 0x03000000
  if (readonly)
    return PyObject_AsReadBuffer(obj, (const void**)buf, size);
  else
    return PyObject_AsWriteBuffer(obj, buf, size);
#else
  (void)obj; (void)readonly;
  (void)buf; (void)size;
  PyErr_SetString(PyExc_SystemError,
                  "Legacy buffer interface not available in Python 3");
  return -1;
#endif
}

/* ------------------------------------------------------------------------- */

#ifdef PYPY_VERSION
#if PY_VERSION_HEX < 0x03030000
#ifdef PySlice_GetIndicesEx
#undef PySlice_GetIndicesEx
#define PySlice_GetIndicesEx(s, n, start, stop, step, length) \
PyPySlice_GetIndicesEx((PySliceObject *)(s), n, start, stop, step, length)
#else
#define PySlice_GetIndicesEx(s, n, start, stop, step, length) \
PySlice_GetIndicesEx((PySliceObject *)(s), n, start, stop, step, length)
#endif
#endif
#else
#if PY_VERSION_HEX < 0x03020000
#define PySlice_GetIndicesEx(s, n, start, stop, step, length) \
PySlice_GetIndicesEx((PySliceObject *)(s), n, start, stop, step, length)
#endif
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

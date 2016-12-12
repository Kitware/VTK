/* Author:  Lisandro Dalcin   */
/* Contact: dalcinl@gmail.com */

/* ------------------------------------------------------------------------- */

#ifdef PYPY_VERSION

#ifndef PyByteArray_Check
#define PyByteArray_Check(self) PyObject_TypeCheck(self, &PyByteArray_Type)
#endif
#ifndef PyByteArray_AS_STRING
#define PyByteArray_GET_SIZE(self)  0
#define PyByteArray_AS_STRING(self) NULL
#endif

#ifndef _PyLong_AsByteArray
static int
_PyLong_AsByteArray(PyLongObject* v,
                    unsigned char* bytes, size_t n,
                    int little_endian, int is_signed)
{
  (void)_PyLong_AsByteArray; /* unused */
  PyErr_SetString(PyExc_RuntimeError,
                  "PyPy: _PyLong_AsByteArray() not available");
  return -1;
}
#endif

#endif/*PYPY_VERSION*/

/* ------------------------------------------------------------------------- */

#if PY_VERSION_HEX < 0x03030000 || \
    (defined(PYPY_VERSION) && !defined(PyMemoryView_FromMemory))
static PyObject *
PyMemoryView_FromMemory(char *mem, Py_ssize_t size, int flags)
{
  int readonly = (flags == PyBUF_WRITE) ? 0 : 1;
#if PY_VERSION_HEX < 0x02070000 || \
    (defined(PYPY_VERSION) && !defined(PyMemoryView_FromBuffer))
  if (readonly)
    return PyBuffer_FromMemory((void*)mem, size);
  else
    return PyBuffer_FromReadWriteMemory((void*)mem, size);
#else
  Py_buffer info;
  if (PyBuffer_FillInfo(&info, NULL, mem, size, readonly,
                        PyBUF_FULL_RO) < 0) return NULL;
  if (info.buf == NULL) {
    PyErr_SetString(PyExc_ValueError,
                    "PyMemoryView_FromBuffer(): info->buf must not be NULL");
    return NULL;
  }
  return PyMemoryView_FromBuffer(&info);
#endif
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

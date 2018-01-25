#------------------------------------------------------------------------------

cdef extern from "Python.h":
    enum: PY_SSIZE_T_MAX
    void *PyMem_Malloc(size_t)
    void *PyMem_Realloc(void*, size_t)
    void PyMem_Free(void*)

cdef extern from "Python.h":
    object PyLong_FromVoidPtr(void*)
    void*  PyLong_AsVoidPtr(object)

cdef extern from "Python.h":
    enum: PyBUF_READ
    enum: PyBUF_WRITE
    object PyMemoryView_FromMemory(char*,Py_ssize_t,int)

#------------------------------------------------------------------------------

cdef extern from *:
    void *emptymemory '((void*)"")'

cdef inline object asmemory(object ob, void **base, MPI_Aint *size):
    cdef _p_buffer buf = getbuffer_w(ob, base, size)
    return buf

cdef inline object tomemory(void *base, MPI_Aint size):
    if base == NULL and size == 0: base = emptymemory
    if PYPY and PY3: return tobuffer(base, size)
    return PyMemoryView_FromMemory(<char*>base, size, PyBUF_WRITE)

#------------------------------------------------------------------------------

@cython.final
@cython.internal
cdef class _p_mem:
    cdef void *buf
    def __cinit__(self):
        self.buf = NULL
    def __dealloc__(self):
        PyMem_Free(self.buf)

cdef inline _p_mem allocate(Py_ssize_t m, size_t b, void **buf):
  if m > PY_SSIZE_T_MAX/<Py_ssize_t>b:
      raise MemoryError("memory allocation size too large")
  if m < 0:
      raise RuntimeError("memory allocation with negative size")
  cdef size_t n = <size_t>m * b
  cdef _p_mem ob = <_p_mem>_p_mem.__new__(_p_mem)
  ob.buf = PyMem_Malloc(n)
  if ob.buf == NULL: raise MemoryError
  if buf != NULL: buf[0] = ob.buf
  return ob

cdef inline _p_mem allocate_int(int n, int **p):
     cdef _p_mem ob = allocate(n, sizeof(int), NULL)
     p[0] = <int*>ob.buf
     return ob

#------------------------------------------------------------------------------

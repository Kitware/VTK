#------------------------------------------------------------------------------

cdef extern from "Python.h":
    void *PyMem_Malloc(size_t)
    void *PyMem_Realloc(void *, size_t)
    void PyMem_Free(void *)

cdef extern from "Python.h":
    object PyLong_FromVoidPtr(void *)
    void*  PyLong_AsVoidPtr(object)

#------------------------------------------------------------------------------

cdef extern from "Python.h":
    object PyMemoryView_FromBuffer(Py_buffer *)

cdef inline object asmemory(object ob, void **base, MPI_Aint *size):
    cdef _p_buffer buf = getbuffer_w(ob, base, size)
    return buf

cdef inline object tomemory(void *base, MPI_Aint size):
    cdef _p_buffer buf = tobuffer(base, size, 0)
    return PyMemoryView_FromBuffer(&buf.view)

#------------------------------------------------------------------------------

cdef extern from *:
    object allocate"PyMPI_Allocate"(Py_ssize_t, size_t, void **)

cdef inline object allocate_int(int n, int **p):
     cdef int *array = NULL
     cdef object ob = allocate(n, sizeof(int), <void**>&array)
     p[0] = array
     return ob

#------------------------------------------------------------------------------

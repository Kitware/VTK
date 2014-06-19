# -----------------------------------------------------------------------------

cdef type arraytype
from array import array as arraytype

cdef extern from "Python.h":
    int PySequence_DelItem(object, Py_ssize_t) except -1
    object PySequence_InPlaceRepeat(object, Py_ssize_t)

cdef inline object newarray_int(Py_ssize_t n, int **p):
    cdef object ary = arraytype('i', [0])
    if n <= 0:
        PySequence_DelItem(ary, 0)
    elif n > 1:
        ary = PySequence_InPlaceRepeat(ary, n)
    cdef int *base = NULL
    cdef Py_ssize_t size = 0
    PyObject_AsWriteBuffer(ary, <void**>&base, &size)
    if p != NULL: p[0] = base
    return ary

cdef inline object getarray_int(object ob, int *n, int **p):
    cdef int *base = NULL
    cdef Py_ssize_t i = 0, size = len(ob)
    cdef object ary = newarray_int(size, &base)
    for i from 0 <= i < size:
        base[i] = ob[i]
    if n != NULL: n[0] = <int> size # XXX overflow?
    if p != NULL: p[0] = base
    return ary

cdef inline object chkarray_int(object ob, Py_ssize_t size, int **p):
    cdef int n = 0
    cdef object ary = getarray_int(ob, &n, p)
    if size != <Py_ssize_t>n: raise ValueError(
        "expecting %d items, got %d" % (size, n))
    return ary

# -----------------------------------------------------------------------------

cdef inline object asarray_int(object sequence,
                               Py_ssize_t size, int **p):
     cdef int *array = NULL
     cdef Py_ssize_t i = 0, n = len(sequence)
     if size != n: raise ValueError(
         "expecting %d items, got %d" % (size, n))
     cdef object ob = allocate(n, sizeof(int), <void**>&array)
     for i from 0 <= i < n:
         array[i] = sequence[i]
     p[0] = array
     return ob

cdef inline object asarray_Aint(object sequence,
                                Py_ssize_t size, MPI_Aint **p):
     cdef MPI_Aint *array = NULL
     cdef Py_ssize_t i = 0, n = len(sequence)
     if size != n: raise ValueError(
         "expecting %d items, got %d" % (size, n))
     cdef object ob = allocate(n, sizeof(MPI_Aint), <void**>&array)
     for i from 0 <= i < n:
         array[i] = sequence[i]
     p[0] = array
     return ob

cdef inline object asarray_Datatype(object sequence,
                                    Py_ssize_t size, MPI_Datatype **p):
     cdef MPI_Datatype *array = NULL
     cdef Py_ssize_t i = 0, n = len(sequence)
     if size != n: raise ValueError(
         "expecting %d items, got %d" % (size, n))
     cdef object ob = allocate(n, sizeof(MPI_Datatype), <void**>&array)
     for i from 0 <= i < n:
         array[i] = (<Datatype?>sequence[i]).ob_mpi
     p[0] = array
     return ob

cdef inline object asarray_Info(object sequence,
                                Py_ssize_t size, MPI_Info **p):
     cdef MPI_Info *array = NULL
     cdef Py_ssize_t i = 0, n = size
     cdef MPI_Info info = MPI_INFO_NULL
     cdef object ob
     if sequence is None or isinstance(sequence, Info):
         if sequence is not None:
             info = (<Info?>sequence).ob_mpi
         ob = allocate(size, sizeof(MPI_Info), <void**>&array)
         for i from 0 <= i < size:
             array[i] = info
     else:
         n = len(sequence)
         if size != n: raise ValueError(
             "expecting %d items, got %d" % (size, n))
         ob = allocate(size, sizeof(MPI_Datatype), <void**>&array)
         for i from 0 <= i < size:
             array[i] = (<Info?>sequence[i]).ob_mpi
     p[0] = array
     return ob

# -----------------------------------------------------------------------------

cdef inline object asarray_str(object sequence,
                               Py_ssize_t size, char ***p):
     cdef Py_ssize_t i = 0, n = len(sequence)
     if size != n: raise ValueError(
         "expecting %d items, got %d" % (size, n))
     cdef char** array = NULL
     cdef object ob = allocate((n+1), sizeof(char*), <void**>&array)
     for i from 0 <= i < n:
         sequence[i] = asmpistr(sequence[i], &array[i], NULL)
     array[n] = NULL
     p[0] = array
     return (sequence, ob)

cdef inline object asarray_argv(object sequence, char ***p):
     if sequence is None:
         p[0] = MPI_ARGV_NULL
         return None
     cdef Py_ssize_t size = len(sequence)
     return asarray_str(sequence, size, p)

cdef inline object asarray_argvs(object sequence,
                                 Py_ssize_t size, char ****p):
     if sequence is None:
         p[0] = MPI_ARGVS_NULL
         return None
     cdef Py_ssize_t i = 0, n = len(sequence)
     if size != n: raise ValueError(
         "expecting %d items, got %d" % (size, n))
     cdef char*** array = NULL
     cdef object ob = allocate((n+1), sizeof(char**), <void**>&array)
     for i from 0 <= i < n:
         sequence[i] = asarray_argv(sequence[i], &array[i])
     array[n] = NULL
     p[0] = array
     return (sequence, ob)

cdef inline object asarray_nprocs(object sequence,
                                  Py_ssize_t size, int **p):
     cdef Py_ssize_t i = 0
     cdef int value = 1
     cdef int *array = NULL
     cdef object ob
     if sequence is None or is_int(sequence):
         if sequence is None:
             value = 1
         else:
             value = sequence
         ob = newarray_int(size, &array)
         for i from 0 <= i < size:
             array[i] = value
     else:
         ob = asarray_int(sequence, <int>size, &array)
     p[0] = array
     return ob

# -----------------------------------------------------------------------------

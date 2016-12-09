# -----------------------------------------------------------------------------

cdef inline object newarray_int(Py_ssize_t n, int **p):
    return allocate(n, sizeof(int), <void**>p)

cdef inline object getarray_int(object ob, int *n, int **p):
    cdef int *base = NULL
    cdef Py_ssize_t i = 0, size = len(ob)
    cdef object mem = newarray_int(size, &base)
    for i from 0 <= i < size: base[i] = ob[i]
    p[0] = base
    n[0] = downcast(size)
    return mem

cdef inline object chkarray_int(object ob, int n, int **p):
    cdef int size = 0
    cdef object mem = getarray_int(ob, &size, p)
    if n != size: raise ValueError(
        "expecting %d items, got %d" % (n, size))
    return mem

# -----------------------------------------------------------------------------

cdef inline object mkarray_int(Py_ssize_t size, int **p):
     return allocate(size, sizeof(int), <void**>p)

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
     cdef Py_ssize_t i = 0
     cdef MPI_Info info = MPI_INFO_NULL
     cdef object ob
     if sequence is None or isinstance(sequence, Info):
         if sequence is not None:
             info = (<Info?>sequence).ob_mpi
         ob = allocate(size, sizeof(MPI_Info), <void**>&array)
         for i from 0 <= i < size:
             array[i] = info
     else:
         if size != len(sequence): raise ValueError(
             "expecting %d items, got %d" % (size, len(sequence)))
         ob = allocate(size, sizeof(MPI_Datatype), <void**>&array)
         for i from 0 <= i < size:
             array[i] = (<Info?>sequence[i]).ob_mpi
     p[0] = array
     return ob

# -----------------------------------------------------------------------------

cdef inline int is_string(object obj):
     return (isinstance(obj, str) or
             isinstance(obj, bytes) or
             isinstance(obj, unicode))

cdef inline object asarray_str(object sequence, char ***p):
     if is_string(sequence):
         raise ValueError("expecting a sequence of strings")
     sequence = list(sequence)
     cdef Py_ssize_t i = 0, size = len(sequence)
     cdef char** array = NULL
     cdef object ob = allocate(size+1, sizeof(char*), <void**>&array)
     for i from 0 <= i < size:
         sequence[i] = asmpistr(sequence[i], &array[i])
     array[size] = NULL
     p[0] = array
     return (sequence, ob)

cdef inline object asarray_argv(object sequence, char ***p):
     if sequence is None:
         p[0] = MPI_ARGV_NULL
         return None
     if is_string(sequence):
         sequence = [sequence]
     return asarray_str(sequence, p)

cdef inline object asarray_cmds(object sequence,
                               int *count, char ***p):
     if is_string(sequence):
         raise ValueError("expecting a sequence of strings")
     count[0] = <int>len(sequence)
     return asarray_str(sequence, p)

cdef inline object asarray_argvs(object sequence,
                                 Py_ssize_t size, char ****p):
     if sequence is None:
         p[0] = MPI_ARGVS_NULL
         return None
     if is_string(sequence):
         sequence = [sequence] * size
     else:
         sequence = list(sequence)
         if size != len(sequence): raise ValueError(
             "expecting %d items, got %d" % (size, len(sequence)))
     cdef Py_ssize_t i = 0
     cdef char*** array = NULL
     cdef object ob = allocate(size+1, sizeof(char**), <void**>&array)
     cdef object argv
     for i from 0 <= i < size:
         argv = sequence[i]
         if argv is None: argv = []
         sequence[i] = asarray_argv(argv, &array[i])
     array[size] = NULL
     p[0] = array
     return (sequence, ob)

cdef inline object asarray_nprocs(object sequence,
                                  Py_ssize_t size, int **p):
     cdef Py_ssize_t i = 0
     cdef int value = 1
     cdef int *array = NULL
     cdef object ob
     if sequence is None or is_integral(sequence):
         if sequence is not None:
             value = sequence
         ob = newarray_int(size, &array)
         for i from 0 <= i < size:
             array[i] = value
     else:
         ob = asarray_int(sequence, size, &array)
     p[0] = array
     return ob

# -----------------------------------------------------------------------------

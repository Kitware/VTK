# -----------------------------------------------------------------------------

cdef dict comm_keyval = {}

cdef int comm_keyval_new(int keyval,
                         object copy_fn,object delete_fn) except -1:
    comm_keyval[keyval] = (copy_fn, delete_fn)
    return 0

cdef int comm_keyval_del(int keyval) except -1:
    try: del comm_keyval[keyval]
    except KeyError: pass
    return 0

cdef int comm_attr_copy(MPI_Comm comm,
                        int keyval,
                        void *extra_state,
                        void *attrval_in,
                        void *attrval_out,
                        int *flag) except -1:
    cdef tuple entry = comm_keyval.get(keyval)
    cdef object copy_fn = None
    if entry is not None: copy_fn = entry[0]
    if copy_fn is None or copy_fn is False:
        flag[0] = 0
        return 0
    cdef object attrval = <object>attrval_in
    cdef void **aptr = <void **>attrval_out
    if copy_fn is not True:
        attrval = copy_fn(attrval)
    Py_INCREF(attrval)
    aptr[0] = <void*>attrval
    flag[0] = 1
    return 0

cdef int comm_attr_delete(MPI_Comm comm,
                          int keyval,
                          void *attrval,
                          void *extra_state) except -1:
    cdef tuple entry = comm_keyval.get(keyval)
    cdef object delete_fn = None
    if entry is not None: delete_fn = entry[1]
    if delete_fn is not None:
        delete_fn(<object>attrval)
    Py_DECREF(<object>attrval)
    return 0

@cython.callspec("PyMPIAPI")
cdef int comm_attr_copy_fn(MPI_Comm comm,
                           int keyval,
                           void *extra_state,
                           void *attrval_in,
                           void *attrval_out,
                           int *flag) with gil:
    if not Py_IsInitialized():
        return MPI_SUCCESS
    if attrval_in == NULL:
        return MPI_ERR_INTERN
    if attrval_out == NULL:
        return MPI_ERR_INTERN
    cdef object exc
    try:
        comm_attr_copy(comm, keyval, extra_state,
                       attrval_in, attrval_out, flag)
    except MPIException, exc:
        print_traceback()
        return exc.Get_error_code()
    except:
        print_traceback()
        return MPI_ERR_OTHER
    return MPI_SUCCESS

@cython.callspec("PyMPIAPI")
cdef int comm_attr_delete_fn(MPI_Comm comm,
                             int keyval,
                             void *attrval,
                             void *extra_state) with gil:
    if not Py_IsInitialized():
        return MPI_SUCCESS
    if attrval == NULL:
        return MPI_ERR_INTERN
    cdef object exc
    try:
        comm_attr_delete(comm, keyval, attrval, extra_state)
    except MPIException, exc:
        print_traceback()
        return exc.Get_error_code()
    except:
        print_traceback()
        return MPI_ERR_OTHER
    return MPI_SUCCESS

# -----------------------------------------------------------------------------

cdef _p_buffer _buffer = None

cdef inline int attach_buffer(ob, void **p, int *n) except -1:
    global _buffer
    cdef void *bptr = NULL
    cdef MPI_Aint blen = 0
    _buffer = getbuffer_w(ob, &bptr, &blen)
    p[0] = bptr
    n[0] = <int>blen # XXX Overflow ?
    return 0

cdef inline object detach_buffer(void *p, int n):
    global _buffer
    cdef object ob = None
    try:
        if (_buffer is not None and
            _buffer.view.buf == p and
            _buffer.view.len == <Py_ssize_t>n and
            _buffer.view.obj != NULL):
            ob = <object>_buffer.view.obj
        else:
            ob = tomemory(p, <MPI_Aint>n)
    finally:
        _buffer = None
    return ob

# -----------------------------------------------------------------------------

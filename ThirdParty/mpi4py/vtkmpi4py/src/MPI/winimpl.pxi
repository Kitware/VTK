# -----------------------------------------------------------------------------

cdef extern from *:
    int PyMPI_KEYVAL_WIN_MEMORY

cdef void win_memory_decref(void *ob) with gil:
    Py_DECREF(<object>ob)

@cython.callspec("PyMPIAPI")
cdef int win_memory_del(MPI_Win w, int k, void *v, void *xs) nogil:
    if  v != NULL:
        if Py_IsInitialized():
            win_memory_decref(v)
    return MPI_SUCCESS

cdef int PyMPI_Win_setup(MPI_Win win, object memory):
    cdef int ierr = MPI_SUCCESS
    # hold a reference to memory
    global PyMPI_KEYVAL_WIN_MEMORY
    if memory is not None:
        if PyMPI_KEYVAL_WIN_MEMORY == MPI_KEYVAL_INVALID:
            ierr = MPI_Win_create_keyval(MPI_WIN_NULL_COPY_FN, win_memory_del,
                                         &PyMPI_KEYVAL_WIN_MEMORY, NULL)
            if ierr: return ierr
        ierr = MPI_Win_set_attr(win, PyMPI_KEYVAL_WIN_MEMORY, <void*>memory)
        if ierr: return ierr
        Py_INCREF(memory)
    #
    return MPI_SUCCESS

# -----------------------------------------------------------------------------

cdef dict win_keyval = {}

cdef inline int win_keyval_new(int keyval,
                               object copy_fn,object delete_fn) except -1:
    win_keyval[keyval] = (copy_fn, delete_fn)
    return 0

cdef inline int win_keyval_del(int keyval) except -1:
    try: del win_keyval[keyval]
    except KeyError: pass
    return 0

cdef inline int win_attr_copy(MPI_Win win,
                              int keyval,
                              void *extra_state,
                              void *attrval_in,
                              void *attrval_out,
                              int *flag) except -1:
    cdef tuple entry = win_keyval.get(keyval)
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

cdef inline int win_attr_delete(MPI_Win win,
                                int keyval,
                                void *attrval,
                                void *extra_state) except -1:
    cdef tuple entry = win_keyval.get(keyval)
    cdef object delete_fn = None
    if entry is not None: delete_fn = entry[1]
    if delete_fn is not None:
        delete_fn(<object>attrval)
    Py_DECREF(<object>attrval)
    return 0

@cython.callspec("PyMPIAPI")
cdef int win_attr_copy_fn(MPI_Win win,
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
        win_attr_copy(win, keyval, extra_state,
                       attrval_in, attrval_out, flag)
    except MPIException, exc:
        print_traceback()
        return exc.Get_error_code()
    except:
        print_traceback()
        return MPI_ERR_OTHER
    return MPI_SUCCESS

@cython.callspec("PyMPIAPI")
cdef int win_attr_delete_fn(MPI_Win win,
                             int keyval,
                             void *attrval,
                             void *extra_state) with gil:
    if not Py_IsInitialized():
        return MPI_SUCCESS
    if attrval == NULL:
        return MPI_ERR_INTERN
    cdef object exc
    try:
        win_attr_delete(win, keyval, attrval, extra_state)
    except MPIException, exc:
        print_traceback()
        return exc.Get_error_code()
    except:
        print_traceback()
        return MPI_ERR_OTHER
    return MPI_SUCCESS

# -----------------------------------------------------------------------------

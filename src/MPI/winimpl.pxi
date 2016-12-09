# -----------------------------------------------------------------------------

cdef dict win_keyval = {}

cdef inline Win newwin(MPI_Win ob):
    cdef Win win = <Win>Win.__new__(Win)
    win.ob_mpi = ob
    return win

cdef int win_attr_copy(
    MPI_Win win,
    int keyval,
    void *extra_state,
    void *attrval_in,
    void *attrval_out,
    int *flag) except -1:
    cdef _p_keyval state = <_p_keyval>extra_state
    cdef object copy_fn = state.copy_fn
    if copy_fn is None:
        flag[0] = 0
        return 0
    cdef object attrval = <object>attrval_in
    cdef void **aptr = <void **>attrval_out
    cdef Win handle
    if copy_fn is not True:
        handle = newwin(win)
        try: attrval = copy_fn(handle, keyval, attrval)
        finally: handle.ob_mpi = MPI_WIN_NULL
    Py_INCREF(attrval)
    Py_INCREF(state)
    aptr[0] = <void*>attrval
    flag[0] = 1
    return 0

cdef int win_attr_copy_cb(
    MPI_Win win,
    int keyval,
    void *extra_state,
    void *attrval_in,
    void *attrval_out,
    int *flag) with gil:
    cdef object exc
    try:
        win_attr_copy(win, keyval, extra_state,
                       attrval_in, attrval_out, flag)
    except MPIException as exc:
        print_traceback()
        return exc.Get_error_code()
    except:
        print_traceback()
        return MPI_ERR_OTHER
    return MPI_SUCCESS

cdef int win_attr_delete(
    MPI_Win win,
    int keyval,
    void *attrval,
    void *extra_state) except -1:
    cdef _p_keyval state = <_p_keyval>extra_state
    cdef object delete_fn = state.delete_fn
    cdef Win handle
    if delete_fn is not None:
        handle = newwin(win)
        try: delete_fn(handle, keyval, <object>attrval)
        finally: handle.ob_mpi = MPI_WIN_NULL
    Py_DECREF(<object>attrval)
    Py_DECREF(<object>extra_state)
    return 0

cdef int win_attr_delete_cb(
    MPI_Win win,
    int keyval,
    void *attrval,
    void *extra_state) with gil:
    cdef object exc
    try:
        win_attr_delete(win, keyval, attrval, extra_state)
    except MPIException as exc:
        print_traceback()
        return exc.Get_error_code()
    except:
        print_traceback()
        return MPI_ERR_OTHER
    return MPI_SUCCESS

@cython.callspec("MPIAPI")
cdef int win_attr_copy_fn(MPI_Win win,
                          int keyval,
                          void *extra_state,
                          void *attrval_in,
                          void *attrval_out,
                          int *flag) nogil:
    if attrval_in  == NULL: return MPI_ERR_INTERN
    if attrval_out == NULL: return MPI_ERR_INTERN
    if attrval_out == NULL: return MPI_ERR_INTERN
    flag[0] = 0
    if not Py_IsInitialized(): return MPI_SUCCESS
    return win_attr_copy_cb(win, keyval, extra_state,
                            attrval_in, attrval_out, flag)

@cython.callspec("MPIAPI")
cdef int win_attr_delete_fn(MPI_Win win,
                            int keyval,
                            void *attrval,
                            void *extra_state) nogil:
    if extra_state == NULL: return MPI_ERR_INTERN
    if attrval     == NULL: return MPI_ERR_INTERN
    if not Py_IsInitialized(): return MPI_SUCCESS
    return win_attr_delete_cb(win, keyval, attrval, extra_state)

# -----------------------------------------------------------------------------

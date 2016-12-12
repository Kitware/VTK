# -----------------------------------------------------------------------------

cdef dict type_keyval = {}

cdef inline Datatype newtype(MPI_Datatype ob):
    cdef Datatype datatype = <Datatype>Datatype.__new__(Datatype)
    datatype.ob_mpi = ob
    return datatype

cdef inline int type_attr_copy(
    MPI_Datatype datatype,
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
    cdef Datatype handle
    if copy_fn is not True:
        handle = newtype(datatype)
        try: attrval = copy_fn(handle, keyval, attrval)
        finally: handle.ob_mpi = MPI_DATATYPE_NULL
    Py_INCREF(attrval)
    Py_INCREF(state)
    aptr[0] = <void*>attrval
    flag[0] = 1
    return 0

cdef int type_attr_copy_cb(
    MPI_Datatype datatype,
    int keyval,
    void *extra_state,
    void *attrval_in,
    void *attrval_out,
    int *flag) with gil:
    cdef object exc
    try:
        type_attr_copy(datatype, keyval, extra_state,
                       attrval_in, attrval_out, flag)
    except MPIException, exc:
        print_traceback()
        return exc.Get_error_code()
    except:
        print_traceback()
        return MPI_ERR_OTHER
    return MPI_SUCCESS

cdef inline int type_attr_delete(
    MPI_Datatype datatype,
    int keyval,
    void *attrval,
    void *extra_state) except -1:
    cdef _p_keyval state = <_p_keyval>extra_state
    cdef object delete_fn = state.delete_fn
    cdef Datatype handle
    if delete_fn is not None:
        handle = newtype(datatype)
        try: delete_fn(handle, keyval, <object>attrval)
        finally: handle.ob_mpi = MPI_DATATYPE_NULL
    Py_DECREF(<object>attrval)
    Py_DECREF(<object>extra_state)
    return 0

cdef int type_attr_delete_cb(
    MPI_Datatype datatype,
    int keyval,
    void *attrval,
    void *extra_state) with gil:
    cdef object exc
    try:
        type_attr_delete(datatype, keyval, attrval, extra_state)
    except MPIException, exc:
        print_traceback()
        return exc.Get_error_code()
    except:
        print_traceback()
        return MPI_ERR_OTHER
    return MPI_SUCCESS

@cython.callspec("MPIAPI")
cdef int type_attr_copy_fn(MPI_Datatype datatype,
                           int keyval,
                           void *extra_state,
                           void *attrval_in,
                           void *attrval_out,
                           int *flag) nogil:
    if extra_state == NULL: return MPI_ERR_INTERN
    if attrval_in  == NULL: return MPI_ERR_INTERN
    if attrval_out == NULL: return MPI_ERR_INTERN
    flag[0] = 0
    if not Py_IsInitialized(): return MPI_SUCCESS
    return type_attr_copy_cb(datatype, keyval, extra_state,
                             attrval_in, attrval_out, flag)

@cython.callspec("MPIAPI")
cdef int type_attr_delete_fn(MPI_Datatype datatype,
                             int keyval,
                             void *attrval,
                             void *extra_state) nogil:
    if extra_state == NULL: return MPI_ERR_INTERN
    if attrval     == NULL: return MPI_ERR_INTERN
    if not Py_IsInitialized(): return MPI_SUCCESS
    return type_attr_delete_cb(datatype, keyval, attrval, extra_state)

# -----------------------------------------------------------------------------

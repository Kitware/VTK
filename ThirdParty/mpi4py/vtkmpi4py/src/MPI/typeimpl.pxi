# -----------------------------------------------------------------------------

cdef dict type_keyval = {}

cdef inline int type_keyval_new(int keyval,
                                object copy_fn,object delete_fn) except -1:
    type_keyval[keyval] = (copy_fn, delete_fn)
    return 0

cdef inline int type_keyval_del(int keyval) except -1:
    try: del type_keyval[keyval]
    except KeyError: pass
    return 0

cdef inline int type_attr_copy(MPI_Datatype datatype,
                               int keyval,
                               void *extra_state,
                               void *attrval_in,
                               void *attrval_out,
                               int *flag) except -1:
    cdef tuple entry = type_keyval.get(keyval)
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

cdef inline int type_attr_delete(MPI_Datatype datatype,
                                 int keyval,
                                 void *attrval,
                                 void *extra_state) except -1:
    cdef tuple entry = type_keyval.get(keyval)
    cdef object delete_fn = None
    if entry is not None: delete_fn = entry[1]
    if delete_fn is not None:
        delete_fn(<object>attrval)
    Py_DECREF(<object>attrval)
    return 0

@cython.callspec("PyMPIAPI")
cdef int type_attr_copy_fn(MPI_Datatype datatype,
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
        type_attr_copy(datatype, keyval, extra_state,
                       attrval_in, attrval_out, flag)
    except MPIException, exc:
        print_traceback()
        return exc.Get_error_code()
    except:
        print_traceback()
        return MPI_ERR_OTHER
    return MPI_SUCCESS

@cython.callspec("PyMPIAPI")
cdef int type_attr_delete_fn(MPI_Datatype datatype,
                             int keyval,
                             void *attrval,
                             void *extra_state) with gil:
    if not Py_IsInitialized():
        return MPI_SUCCESS
    if attrval == NULL:
        return MPI_ERR_INTERN
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

# -----------------------------------------------------------------------------

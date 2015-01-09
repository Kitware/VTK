# -----------------------------------------------------------------------------

cdef MPI_Status empty_status
empty_status.MPI_SOURCE = MPI_ANY_SOURCE
empty_status.MPI_TAG = MPI_ANY_TAG
empty_status.MPI_ERROR  = MPI_SUCCESS

cdef object acquire_rs(object requests,
                       object statuses,
                       int         *count,
                       MPI_Request *rp[],
                       MPI_Status  *sp[]):
     cdef MPI_Request *array_r = NULL
     cdef MPI_Status  *array_s = NULL
     cdef object ob_r = None, ob_s = None
     cdef Py_ssize_t i = 0, n = len(requests)
     count[0] = <int>n # XXX overflow ?
     ob_r = allocate(n, sizeof(MPI_Request), <void**>&array_r)
     for i from 0 <= i < n:
         array_r[i] = (<Request?>requests[i]).ob_mpi
     rp[0] = array_r
     if statuses is not None:
         ob_s = allocate(n, sizeof(MPI_Status), <void**>&array_s)
         for i from 0 <= i < n:
             array_s[i] = empty_status
         sp[0] = array_s
     return (ob_r, ob_s)

cdef int release_rs(object requests,
                    object statuses,
                    int         count,
                    MPI_Request rp[],
                    MPI_Status  sp[]) except -1:
    cdef Py_ssize_t i = 0, nr = count, ns = 0
    cdef Request req = None
    for i from 0 <= i < nr:
        req = <Request>requests[i]
        req.ob_mpi = rp[i]
        if rp[i] == MPI_REQUEST_NULL:
            req.ob_buf = None
    if statuses is not None:
        ns = len(statuses)
        if nr > ns :
            if isinstance(statuses, list):
                statuses += [Status.__new__(Status)
                             for i from ns <= i < nr]
                ns = nr
        for i from 0 <= i < ns:
            (<Status?>statuses[i]).ob_mpi = sp[i]
    return 0

# -----------------------------------------------------------------------------

cdef class _p_greq:

    cdef object query_fn
    cdef object free_fn
    cdef object cancel_fn
    cdef tuple args
    cdef dict  kargs

    def __cinit__(self, query_fn, free_fn, cancel_fn, args, kargs):
        self.query_fn  = query_fn
        self.free_fn   = free_fn
        self.cancel_fn = cancel_fn
        self.args  = tuple(args) if args  is not None else ()
        self.kargs = dict(kargs) if kargs is not None else {}

    cdef int query(self, MPI_Status *status) except -1:
        status.MPI_SOURCE = MPI_ANY_SOURCE
        status.MPI_TAG = MPI_ANY_TAG
        MPI_Status_set_elements(status, MPI_BYTE, 0)
        MPI_Status_set_cancelled(status, 0)
        cdef Status sts = <Status>Status.__new__(Status)
        if self.query_fn is not None:
            sts.ob_mpi = status[0]
            self.query_fn(sts, *self.args, **self.kargs)
            status[0] = sts.ob_mpi
            if self.cancel_fn is None:
                MPI_Status_set_cancelled(status, 0)
        return MPI_SUCCESS

    cdef int free(self) except -1:
        if self.free_fn is not None:
            self.free_fn(*self.args, **self.kargs)
        return MPI_SUCCESS

    cdef int cancel(self, bint completed) except -1:
        if self.cancel_fn is not None:
            self.cancel_fn(completed, *self.args, **self.kargs)
        return MPI_SUCCESS

# ---

cdef int greq_query(void *extra_state, MPI_Status *status) with gil:
    cdef _p_greq state = <_p_greq>extra_state
    cdef int ierr = MPI_SUCCESS
    cdef object exc
    try:
        ierr = state.query(status)
    except MPIException, exc:
        print_traceback()
        ierr = exc.Get_error_code()
    except:
        print_traceback()
        ierr = MPI_ERR_OTHER
    return ierr

cdef int greq_free(void *extra_state) with gil:
    cdef _p_greq state = <_p_greq>extra_state
    cdef int ierr = MPI_SUCCESS
    cdef object exc
    try:
        ierr = state.free()
    except MPIException, exc:
        print_traceback()
        ierr = exc.Get_error_code()
    except:
        print_traceback()
        ierr = MPI_ERR_OTHER
    Py_DECREF(<object>extra_state)
    return ierr

cdef int greq_cancel(void *extra_state, int completed) with gil:
    cdef _p_greq state = <_p_greq>extra_state
    cdef int ierr = MPI_SUCCESS
    cdef object exc
    try:
        ierr = state.cancel(completed)
    except MPIException, exc:
        print_traceback()
        ierr = exc.Get_error_code()
    except:
        print_traceback()
        ierr = MPI_ERR_OTHER
    return ierr

# ---

@cython.callspec("PyMPIAPI")
cdef int greq_query_fn(void *extra_state, MPI_Status *status) nogil:
    if extra_state == NULL:
        return MPI_ERR_INTERN
    if status == NULL:
        return MPI_ERR_INTERN
    if not Py_IsInitialized():
        return MPI_ERR_INTERN
    return greq_query(extra_state, status)

@cython.callspec("PyMPIAPI")
cdef int greq_free_fn(void *extra_state) nogil:
    if extra_state == NULL:
        return MPI_ERR_INTERN
    if not Py_IsInitialized():
        return MPI_ERR_INTERN
    return greq_free(extra_state)

@cython.callspec("PyMPIAPI")
cdef int greq_cancel_fn(void *extra_state, int completed) nogil:
    if extra_state == NULL:
        return MPI_ERR_INTERN
    if not Py_IsInitialized():
        return MPI_ERR_INTERN
    return greq_cancel(extra_state, completed)

# -----------------------------------------------------------------------------

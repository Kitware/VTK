cdef class Request:

    """
    Request
    """

    def __cinit__(self, Request request=None):
        self.ob_mpi = MPI_REQUEST_NULL
        if request is not None:
            self.ob_mpi = request.ob_mpi
            self.ob_buf = request.ob_buf

    def __dealloc__(self):
        if not (self.flags & PyMPI_OWNED): return
        CHKERR( del_Request(&self.ob_mpi) )

    def __richcmp__(self, other, int op):
        if not isinstance(self,  Request): return NotImplemented
        if not isinstance(other, Request): return NotImplemented
        cdef Request s = <Request>self, o = <Request>other
        if   op == Py_EQ: return (s.ob_mpi == o.ob_mpi)
        elif op == Py_NE: return (s.ob_mpi != o.ob_mpi)
        else: raise TypeError("only '==' and '!='")

    def __bool__(self):
        return self.ob_mpi != MPI_REQUEST_NULL

    # Completion Operations
    # ---------------------

    def Wait(self, Status status=None):
        """
        Wait for a send or receive to complete
        """
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_Wait(
            &self.ob_mpi, statusp) )
        if self.ob_mpi == MPI_REQUEST_NULL:
            self.ob_buf = None

    def Test(self, Status status=None):
        """
        Test for the completion of a send or receive
        """
        cdef int flag = 0
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_Test(
            &self.ob_mpi, &flag, statusp) )
        if self.ob_mpi == MPI_REQUEST_NULL:
            self.ob_buf = None
        return <bint>flag

    def Free(self):
        """
        Free a communication request
        """
        with nogil: CHKERR( MPI_Request_free(&self.ob_mpi) )

    def Get_status(self, Status status=None):
        """
        Non-destructive test for the completion of a request
        """
        cdef int flag = 0
        cdef MPI_Status *statusp = arg_Status(status)
        with nogil: CHKERR( MPI_Request_get_status(
            self.ob_mpi, &flag, statusp) )
        return <bint>flag

    # Multiple Completions
    # --------------------

    @classmethod
    def Waitany(cls, requests, Status status=None):
        """
        Wait for any previously initiated request to complete
        """
        cdef int count = 0
        cdef MPI_Request *irequests = NULL
        cdef int index = MPI_UNDEFINED
        cdef MPI_Status *statusp = arg_Status(status)
        #
        cdef tmp = acquire_rs(requests, None, &count, &irequests, NULL)
        try:
            with nogil: CHKERR( MPI_Waitany(
                count, irequests, &index, statusp) )
        finally:
            release_rs(requests, None, count, irequests, NULL)
        return index

    @classmethod
    def Testany(cls, requests, Status status=None):
        """
        Test for completion of any previously initiated request
        """
        cdef int count = 0
        cdef MPI_Request *irequests = NULL
        cdef int index = MPI_UNDEFINED
        cdef int flag = 0
        cdef MPI_Status *statusp = arg_Status(status)
        #
        cdef tmp = acquire_rs(requests, None, &count, &irequests, NULL)
        try:
            with nogil: CHKERR( MPI_Testany(
                count, irequests, &index, &flag, statusp) )
        finally:
            release_rs(requests, None, count, irequests, NULL)
        #
        return (index, <bint>flag)

    @classmethod
    def Waitall(cls, requests, statuses=None):
        """
        Wait for all previously initiated requests to complete
        """
        cdef int count = 0
        cdef MPI_Request *irequests = NULL
        cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
        #
        cdef tmp = acquire_rs(requests, statuses,
                              &count, &irequests, &istatuses)
        try:
            with nogil: CHKERR( MPI_Waitall(
                count, irequests, istatuses) )
        finally:
            release_rs(requests, statuses, count, irequests, istatuses)
        return None

    @classmethod
    def Testall(cls, requests, statuses=None):
        """
        Test for completion of all previously initiated requests
        """
        cdef int count = 0
        cdef MPI_Request *irequests = NULL
        cdef int flag = 0
        cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
        #
        cdef tmp = acquire_rs(requests, statuses,
                              &count, &irequests, &istatuses)
        try:
            with nogil: CHKERR( MPI_Testall(
                count, irequests, &flag, istatuses) )
        finally:
            release_rs(requests, statuses, count, irequests, istatuses)
        return <bint>flag

    @classmethod
    def Waitsome(cls, requests, statuses=None):
        """
        Wait for some previously initiated requests to complete
        """
        cdef int incount = 0
        cdef MPI_Request *irequests = NULL
        cdef int outcount = MPI_UNDEFINED, *iindices = NULL
        cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
        #
        cdef tmp = acquire_rs(requests, statuses,
                              &incount, &irequests, &istatuses)
        cdef object indices = newarray_int(incount, &iindices)
        try:
            with nogil: CHKERR( MPI_Waitsome(
                incount, irequests, &outcount, iindices, istatuses) )
        finally:
            release_rs(requests, statuses, incount, irequests, istatuses)
        #
        if outcount == MPI_UNDEFINED:
            del indices[:]
        else:
            del indices[outcount:]
        return (outcount, indices)

    @classmethod
    def Testsome(cls, requests, statuses=None):
        """
        Test for completion of some previously initiated requests
        """
        cdef int incount = <int>len(requests)
        cdef MPI_Request *irequests = NULL
        cdef int outcount = MPI_UNDEFINED, *iindices = NULL
        cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
        #
        cdef tmp = acquire_rs(requests, statuses,
                              &incount, &irequests, &istatuses)
        cdef object indices = newarray_int(incount, &iindices)
        try:
            with nogil: CHKERR( MPI_Testsome(
                incount, irequests, &outcount, iindices, istatuses) )
        finally:
            release_rs(requests, statuses, incount, irequests, istatuses)
        #
        if outcount == MPI_UNDEFINED:
            del indices[:]
        else:
            del indices[outcount:]
        return (outcount, indices)

    # Cancel
    # ------

    def Cancel(self):
        """
        Cancel a communication request
        """
        with nogil: CHKERR( MPI_Cancel(&self.ob_mpi) )

    # Fortran Handle
    # --------------

    def py2f(self):
        """
        """
        return MPI_Request_c2f(self.ob_mpi)

    @classmethod
    def f2py(cls, arg):
        """
        """
        cdef Request request = <Request>cls()
        request.ob_mpi = MPI_Request_f2c(arg)
        return request

    # Python Communication
    # --------------------
    #
    def wait(self, Status status=None):
        """
        Wait for a send or receive to complete
        """
        cdef msg = PyMPI_wait(self, status)
        return msg
    #
    def test(self, Status status=None):
        """
        Test for the completion of a send or receive
        """
        cdef int flag = 0
        cdef msg = PyMPI_test(self, &flag, status)
        return (<bint>flag, msg)
    #
    @classmethod
    def waitany(cls, requests, Status status=None):
        """
        Wait for any previously initiated request to complete
        """
        cdef int index = MPI_UNDEFINED
        cdef msg = PyMPI_waitany(requests, &index, status)
        return (index, msg)
    #
    @classmethod
    def testany(cls, requests, Status status=None):
        """
        Test for completion of any previously initiated request
        """
        cdef int index = MPI_UNDEFINED
        cdef int flag  = 0
        cdef msg = PyMPI_testany(requests, &index, &flag, status)
        return (index, <bint>flag, msg)
    #
    @classmethod
    def waitall(cls, requests, statuses=None):
        """
        Wait for all previously initiated requests to complete
        """
        cdef msg = PyMPI_waitall(requests, statuses)
        return msg
    #
    @classmethod
    def testall(cls, requests, statuses=None):
        """
        Test for completion of all previously initiated requests
        """
        cdef int flag = 0
        cdef msg = PyMPI_testall(requests, &flag, statuses)
        return (<bint>flag, msg)


cdef class Prequest(Request):

    """
    Persistent request
    """

    def __cinit__(self, Request request=None):
        if self.ob_mpi != MPI_REQUEST_NULL:
            <void>(<Prequest?>request)

    def Start(self):
        """
        Initiate a communication with a persistent request
        """
        with nogil: CHKERR( MPI_Start(&self.ob_mpi) )

    @classmethod
    def Startall(cls, requests):
        """
        Start a collection of persistent requests
        """
        cdef int count = 0
        cdef MPI_Request *irequests = NULL
        cdef tmp = acquire_rs(requests, None, &count, &irequests, NULL)
        #
        try:
            with nogil: CHKERR( MPI_Startall(count, irequests) )
        finally:
            release_rs(requests, None, count, irequests, NULL)



cdef class Grequest(Request):

    """
    Generalized request
    """

    def __cinit__(self, Request request=None):
        self.ob_grequest = self.ob_mpi
        if self.ob_mpi != MPI_REQUEST_NULL:
            <void>(<Grequest?>request)

    @classmethod
    def Start(cls, query_fn, free_fn, cancel_fn,
              args=None, kargs=None):
        """
        Create and return a user-defined request
        """
        cdef Grequest request = <Grequest>cls()
        cdef _p_greq state = \
             _p_greq(query_fn, free_fn, cancel_fn, args, kargs)
        with nogil: CHKERR( MPI_Grequest_start(
            greq_query_fn, greq_free_fn, greq_cancel_fn,
            <void*>state, &request.ob_mpi) )
        Py_INCREF(state)
        request.ob_grequest = request.ob_mpi
        return request

    def Complete(self):
        """
        Notify that a user-defined request is complete
        """
        if self.ob_mpi != MPI_REQUEST_NULL:
            if self.ob_mpi != self.ob_grequest:
                raise MPIException(MPI_ERR_REQUEST)
        cdef MPI_Request grequest = self.ob_grequest
        self.ob_grequest = self.ob_mpi ## or MPI_REQUEST_NULL ??
        with nogil: CHKERR( MPI_Grequest_complete(grequest) )
        self.ob_grequest = self.ob_mpi ## or MPI_REQUEST_NULL ??



cdef Request __REQUEST_NULL__ = new_Request(MPI_REQUEST_NULL)


# Predefined request handles
# --------------------------

REQUEST_NULL = __REQUEST_NULL__  #: Null request handle

cdef class Status:

    """
    Status
    """

    def __cinit__(self, Status status=None):
        self.ob_mpi.MPI_SOURCE = MPI_ANY_SOURCE
        self.ob_mpi.MPI_TAG    = MPI_ANY_TAG
        self.ob_mpi.MPI_ERROR  = MPI_SUCCESS
        if status is None: return
        copy_Status(&status.ob_mpi, &self.ob_mpi)

    def __richcmp__(self, other, int op):
        if not isinstance(other, Status): return NotImplemented
        cdef Status s = <Status>self, o = <Status>other
        cdef int r = equal_Status(&s.ob_mpi, &o.ob_mpi)
        if   op == Py_EQ: return  r != 0
        elif op == Py_NE: return  r == 0
        cdef str mod = type(self).__module__
        cdef str cls = type(self).__name__
        raise TypeError("unorderable type: '%s.%s'" % (mod, cls))

    def Get_source(self):
        """
        Get message source
        """
        return self.ob_mpi.MPI_SOURCE

    def Set_source(self, int source):
        """
        Set message source
        """
        self.ob_mpi.MPI_SOURCE = source

    property source:
        """source"""
        def __get__(self):
            return self.Get_source()
        def __set__(self, value):
            self.Set_source(value)

    def Get_tag(self):
        """
        Get message tag
        """
        return self.ob_mpi.MPI_TAG

    def Set_tag(self, int tag):
        """
        Set message tag
        """
        self.ob_mpi.MPI_TAG = tag

    property tag:
        """tag"""
        def __get__(self):
            return self.Get_tag()
        def __set__(self, value):
            self.Set_tag(value)

    def Get_error(self):
        """
        Get message error
        """
        return self.ob_mpi.MPI_ERROR

    def Set_error(self, int error):
        """
        Set message error
        """
        self.ob_mpi.MPI_ERROR = error

    property error:
        """error"""
        def __get__(self):
            return self.Get_error()
        def __set__(self, value):
            self.Set_error(value)

    def Get_count(self, Datatype datatype not None=BYTE):
        """
        Get the number of *top level* elements
        """
        cdef MPI_Datatype dtype = datatype.ob_mpi
        cdef int count = MPI_UNDEFINED
        CHKERR( MPI_Get_count(&self.ob_mpi, dtype, &count) )
        return count

    property count:
        """byte count"""
        def __get__(self):
            return self.Get_count(__BYTE__)

    def Get_elements(self, Datatype datatype not None):
        """
        Get the number of basic elements in a datatype
        """
        cdef MPI_Datatype dtype = datatype.ob_mpi
        cdef MPI_Count elements = MPI_UNDEFINED
        CHKERR( MPI_Get_elements_x(&self.ob_mpi, dtype, &elements) )
        return elements

    def Set_elements(self, Datatype datatype not None, Count count):
        """
        Set the number of elements in a status

        .. note:: This should be only used when implementing
           query callback functions for generalized requests
        """
        cdef MPI_Datatype dtype = datatype.ob_mpi
        CHKERR( MPI_Status_set_elements_x(&self.ob_mpi, dtype, count) )

    def Is_cancelled(self):
        """
        Test to see if a request was cancelled
        """
        cdef int flag = 0
        CHKERR( MPI_Test_cancelled(&self.ob_mpi, &flag) )
        return <bint>flag

    def Set_cancelled(self, bint flag):
        """
        Set the cancelled state associated with a status

        .. note:: This should be only used when implementing
           query callback functions for generalized requests
        """
        CHKERR( MPI_Status_set_cancelled(&self.ob_mpi, flag) )

    property cancelled:
        """
        cancelled state
        """
        def __get__(self):
            return self.Is_cancelled()
        def __set__(self, value):
            self.Set_cancelled(value)

    # Fortran Handle
    # --------------

    def py2f(self):
        """
        """
        cdef Status status = <Status> self
        cdef Py_ssize_t i = 0
        cdef Py_ssize_t n = <int>(sizeof(MPI_Status)/sizeof(int))
        cdef MPI_Status *c_status = &status.ob_mpi
        cdef MPI_Fint *f_status = NULL
        cdef tmp = allocate(n+1, sizeof(MPI_Fint), <void**>&f_status)
        CHKERR( MPI_Status_c2f(c_status, f_status) )
        return [f_status[i] for i from 0 <= i < n]

    @classmethod
    def f2py(cls, arg):
        """
        """
        cdef Status status = <Status> Status.__new__(Status)
        cdef Py_ssize_t i = 0
        cdef Py_ssize_t n = <int>(sizeof(MPI_Status)/sizeof(int))
        cdef MPI_Status *c_status = &status.ob_mpi
        cdef MPI_Fint *f_status = NULL
        cdef tmp = allocate(n+1, sizeof(MPI_Fint), <void**>&f_status)
        for i from 0 <= i < n: f_status[i] = arg[i]
        CHKERR( MPI_Status_f2c(f_status, c_status) )
        return status

cdef class Status:

    """
    Status
    """

    def __cinit__(self):
        self.ob_mpi.MPI_SOURCE = MPI_ANY_SOURCE
        self.ob_mpi.MPI_TAG    = MPI_ANY_TAG
        self.ob_mpi.MPI_ERROR  = MPI_SUCCESS

    def __richcmp__(self, other, int op):
        if not isinstance(self,  Status): return NotImplemented
        if not isinstance(other, Status): return NotImplemented
        cdef Status s = <Status>self, o = <Status>other
        cdef int r = equal_Status(&s.ob_mpi, &o.ob_mpi)
        if   op == Py_EQ: return  r == 0
        elif op == Py_NE: return  r != 0
        else: raise TypeError("only '==' and '!='")


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
        cdef int count = MPI_UNDEFINED
        CHKERR( MPI_Get_count(&self.ob_mpi, datatype.ob_mpi, &count) )
        return count

    property count:
        """byte count"""
        def __get__(self):
            return self.Get_count(__BYTE__)

    def Get_elements(self, Datatype datatype not None):
        """
        Get the number of basic elements in a datatype
        """
        cdef int elements = MPI_UNDEFINED
        CHKERR( MPI_Get_elements(&self.ob_mpi, datatype.ob_mpi, &elements) )
        return elements

    def Set_elements(self, Datatype datatype not None, int count):
        """
        Set the number of elements in a status

        .. note:: This should be only used when implementing
           query callback functions for generalized requests
        """
        CHKERR( MPI_Status_set_elements(&self.ob_mpi, datatype.ob_mpi, count) )

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

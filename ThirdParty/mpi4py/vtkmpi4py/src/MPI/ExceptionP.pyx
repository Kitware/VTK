class Exception(RuntimeError):

    """
    Exception
    """

    def __init__(self, int ierr=0):
        if ierr < MPI_SUCCESS:      ierr = MPI_ERR_UNKNOWN
        if ierr > MPI_ERR_LASTCODE: ierr = MPI_ERR_UNKNOWN
        self.ob_mpi = ierr
        RuntimeError.__init__(self, self.ob_mpi)

    def __eq__(self, int error):
        cdef int ierr = self.ob_mpi
        return <bint> (ierr == error)
    def __ne__(self, int error):
        cdef int ierr = self.ob_mpi
        return <bint> (ierr != error)
    def __lt__(self, int error):
        cdef int ierr = self.ob_mpi
        return <bint> (ierr < error)
    def __le__(self, int error):
        cdef int ierr = self.ob_mpi
        return <bint> (ierr <= error)
    def __gt__(self, int error):
        cdef int ierr = self.ob_mpi
        return <bint> (ierr > error)
    def __ge__(self, int error):
        cdef int ierr = self.ob_mpi
        return <bint> (ierr >= error)

    def __bool__(self):
        cdef int ierr = self.ob_mpi
        return ierr != MPI_SUCCESS

    def __int__(self):
        if not mpi_active():
            return self.ob_mpi
        return self.Get_error_code()

    def __repr__(self):
        return "MPI.Exception(%d)" % self.ob_mpi

    def __str__(self):
        if not mpi_active():
            return "error code: %d" % self.ob_mpi
        return self.Get_error_string()

    def Get_error_code(self):
        """
        Error code
        """
        cdef int errorcode = MPI_SUCCESS
        errorcode = self.ob_mpi
        return errorcode

    error_code = property(Get_error_code, doc="error code")

    def Get_error_class(self):
        """
        Error class
        """
        cdef int errorclass = MPI_SUCCESS
        CHKERR( MPI_Error_class(self.ob_mpi, &errorclass) )
        return errorclass

    error_class = property(Get_error_class, doc="error class")

    def Get_error_string(self):
        """
        Error string
        """
        cdef char string[MPI_MAX_ERROR_STRING+1]
        cdef int resultlen = 0
        CHKERR( MPI_Error_string(self.ob_mpi, string, &resultlen) )
        return tompistr(string, resultlen)

    error_string = property(Get_error_string, doc="error string")

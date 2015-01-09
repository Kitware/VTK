cdef class Op:

    """
    Op
    """

    def __cinit__(self):
        self.ob_mpi = MPI_OP_NULL

    def __dealloc__(self):
        if not (self.flags & PyMPI_OWNED): return
        CHKERR( del_Op(&self.ob_mpi) )
        op_user_del(&self.ob_usrid)

    def __richcmp__(self, other, int op):
        if not isinstance(self,  Op): return NotImplemented
        if not isinstance(other, Op): return NotImplemented
        cdef Op s = <Op>self, o = <Op>other
        if   op == Py_EQ: return (s.ob_mpi == o.ob_mpi)
        elif op == Py_NE: return (s.ob_mpi != o.ob_mpi)
        else: raise TypeError("only '==' and '!='")

    def __bool__(self):
        return self.ob_mpi != MPI_OP_NULL

    def __call__(self, x, y):
        if self.ob_func != NULL:
            return self.ob_func(x, y)
        else:
            return op_user_py(self.ob_usrid, x, y, None)

    @classmethod
    def Create(cls, function, bint commute=False):
        """
        Create a user-defined operation
        """
        cdef Op op = <Op>cls()
        cdef MPI_User_function *cfunction = NULL
        op.ob_usrid = op_user_new(function, &cfunction)
        CHKERR( MPI_Op_create(cfunction, commute, &op.ob_mpi) )
        return op

    def Free(self):
        """
        Free the operation
        """
        CHKERR( MPI_Op_free(&self.ob_mpi) )
        op_user_del(&self.ob_usrid)

    # Process-local reduction
    # -----------------------

    def Is_commutative(self):
        """
        Query reduction operations for their commutativity
        """
        cdef int flag = 0
        CHKERR( MPI_Op_commutative(self.ob_mpi, &flag) )
        return <bint>flag

    property is_commutative:
        """is commutative"""
        def __get__(self):
            return self.Is_commutative()

    def Reduce_local(self, inbuf, inoutbuf):
        """
        Apply a reduction operator to local data
        """
        # get *in* and *inout* buffers
        cdef _p_msg_cco m = message_cco()
        m.for_cro_send(inbuf, 0)
        m.for_cro_recv(inoutbuf, 0)
        # check counts and datatypes
        if self.scount != self.rcount:
            raise ValueError(
                "mismatch in inbuf count %d and inoutbuf count %d" %
                (self.scount, self.rcount))
        if (self.stype != self.rtype):
            raise ValueError(
                "mismatch in inbuf and inoutbuf MPI datatypes")
        # do local reduction
        with nogil: CHKERR( MPI_Reduce_local(
            m.sbuf, m.rbuf, m.rcount, m.rtype, self.ob_mpi) )

    # Fortran Handle
    # --------------

    def py2f(self):
        """
        """
        return MPI_Op_c2f(self.ob_mpi)

    @classmethod
    def f2py(cls, arg):
        """
        """
        cdef Op op = <Op>cls()
        op.ob_mpi = MPI_Op_f2c(arg)
        return op



cdef Op __OP_NULL__ = new_Op( MPI_OP_NULL )
cdef Op __MAX__     = new_Op( MPI_MAX     )
cdef Op __MIN__     = new_Op( MPI_MIN     )
cdef Op __SUM__     = new_Op( MPI_SUM     )
cdef Op __PROD__    = new_Op( MPI_PROD    )
cdef Op __LAND__    = new_Op( MPI_LAND    )
cdef Op __BAND__    = new_Op( MPI_BAND    )
cdef Op __LOR__     = new_Op( MPI_LOR     )
cdef Op __BOR__     = new_Op( MPI_BOR     )
cdef Op __LXOR__    = new_Op( MPI_LXOR    )
cdef Op __BXOR__    = new_Op( MPI_BXOR    )
cdef Op __MAXLOC__  = new_Op( MPI_MAXLOC  )
cdef Op __MINLOC__  = new_Op( MPI_MINLOC  )
cdef Op __REPLACE__ = new_Op( MPI_REPLACE )


# Predefined operation handles
# ----------------------------

OP_NULL = __OP_NULL__  #: Null
MAX     = __MAX__      #: Maximum
MIN     = __MIN__      #: Minimum
SUM     = __SUM__      #: Sum
PROD    = __PROD__     #: Product
LAND    = __LAND__     #: Logical and
BAND    = __BAND__     #: Bit-wise and
LOR     = __LOR__      #: Logical or
BOR     = __BOR__      #: Bit-wise or
LXOR    = __LXOR__     #: Logical xor
BXOR    = __BXOR__     #: Bit-wise xor
MAXLOC  = __MAXLOC__   #: Maximum and location
MINLOC  = __MINLOC__   #: Minimum and location
REPLACE = __REPLACE__  #: Replace (for RMA)

# Assertion modes
# ---------------

MODE_NOCHECK   = MPI_MODE_NOCHECK
MODE_NOSTORE   = MPI_MODE_NOSTORE
MODE_NOPUT     = MPI_MODE_NOPUT
MODE_NOPRECEDE = MPI_MODE_NOPRECEDE
MODE_NOSUCCEED = MPI_MODE_NOSUCCEED

# Lock types
# ----------

LOCK_EXCLUSIVE = MPI_LOCK_EXCLUSIVE
LOCK_SHARED    = MPI_LOCK_SHARED


cdef class Win:

    """
    Window
    """

    def __cinit__(self, Win win=None):
        self.ob_mpi = MPI_WIN_NULL
        if win is not None:
            self.ob_mpi =  win.ob_mpi

    def __dealloc__(self):
        if not (self.flags & PyMPI_OWNED): return
        CHKERR( del_Win(&self.ob_mpi) )

    def __richcmp__(self, other, int op):
        if not isinstance(self,  Win): return NotImplemented
        if not isinstance(other, Win): return NotImplemented
        cdef Win s = <Win>self, o = <Win>other
        if   op == Py_EQ: return (s.ob_mpi == o.ob_mpi)
        elif op == Py_NE: return (s.ob_mpi != o.ob_mpi)
        else: raise TypeError("only '==' and '!='")

    def __bool__(self):
        return self.ob_mpi != MPI_WIN_NULL

    # [6.2] Initialization
    # --------------------

    # [6.2.1] Window Creation
    # -----------------------

    @classmethod
    def Create(cls, memory, int disp_unit=1,
               Info info=INFO_NULL, Intracomm comm not None=COMM_SELF):
        """
        Create an window object for one-sided communication
        """
        cdef void *base = NULL
        cdef MPI_Aint size = 0
        if memory is __BOTTOM__:
            base = MPI_BOTTOM
            memory = None
        elif memory is not None:
            memory = getbuffer_w(memory, &base, &size)
        cdef MPI_Info cinfo = arg_Info(info)
        cdef Win win = <Win>cls()
        with nogil:
            CHKERR( MPI_Win_create(
                    base, size, disp_unit,
                    cinfo, comm.ob_mpi, &win.ob_mpi) )
            CHKERR( MPI_Win_set_errhandler(
                    win.ob_mpi, MPI_ERRORS_RETURN) )
        CHKERR( PyMPI_Win_setup(win.ob_mpi, memory) )
        return win

    def Free(self):
        """
        Free a window
        """
        with nogil: CHKERR( MPI_Win_free(&self.ob_mpi) )

    # [6.2.2] Window Attributes
    # -------------------------

    def Get_group(self):
        """
        Return a duplicate of the group of the
        communicator used to create the window
        """
        cdef Group group = Group()
        with nogil: CHKERR( MPI_Win_get_group(self.ob_mpi, &group.ob_mpi) )
        return group

    property group:
        """window group"""
        def __get__(self):
            return self.Get_group()

    def Get_attr(self, int keyval):
        """
        Retrieve attribute value by key
        """
        cdef void *attrval = NULL
        cdef int flag = 0
        CHKERR( MPI_Win_get_attr(self.ob_mpi, keyval, &attrval, &flag) )
        if not flag: return None
        if not attrval: return 0
        # handle predefined keyvals
        if (keyval == <int>MPI_WIN_BASE):
            return <MPI_Aint>attrval
        elif (keyval == <int>MPI_WIN_SIZE):
            return (<MPI_Aint*>attrval)[0]
        elif (keyval == <int>MPI_WIN_DISP_UNIT):
            return (<int*>attrval)[0]
        # likely be a user-defined keyval
        elif keyval in win_keyval:
            return <object>attrval
        else:
            return PyLong_FromVoidPtr(attrval)

    def Set_attr(self, int keyval, object attrval):
        """
        Store attribute value associated with a key
        """
        cdef void *ptrval = NULL
        cdef int incref = 0
        if keyval in win_keyval:
            ptrval = <void*>attrval
            incref = 1
        else:
            ptrval = PyLong_AsVoidPtr(attrval)
            incref = 0
        CHKERR(MPI_Win_set_attr(self.ob_mpi, keyval, ptrval) )
        if incref: Py_INCREF(attrval)

    def Delete_attr(self, int keyval):
        """
        Delete attribute value associated with a key
        """
        CHKERR(MPI_Win_delete_attr(self.ob_mpi, keyval) )

    @classmethod
    def Create_keyval(cls, copy_fn=None, delete_fn=None):
        """
        Create a new attribute key for windows
        """
        cdef int keyval = MPI_KEYVAL_INVALID
        cdef MPI_Win_copy_attr_function *_copy = win_attr_copy_fn
        cdef MPI_Win_delete_attr_function *_del = win_attr_delete_fn
        cdef void *extra_state = NULL
        CHKERR( MPI_Win_create_keyval(_copy, _del, &keyval, extra_state) )
        win_keyval_new(keyval, copy_fn, delete_fn)
        return keyval

    @classmethod
    def Free_keyval(cls, int keyval):
        """
        Free and attribute key for windows
        """
        cdef int keyval_save = keyval
        CHKERR( MPI_Win_free_keyval (&keyval) )
        win_keyval_del(keyval_save)
        return keyval

    property attrs:
        "window attributes"
        def __get__(self):
            cdef MPI_Win win = self.ob_mpi
            cdef void *base = NULL, *pbase = NULL
            cdef MPI_Aint size = 0, *psize = NULL
            cdef int      disp = 0, *pdisp = NULL
            cdef int attr = MPI_KEYVAL_INVALID
            cdef int flag = 0
            #
            attr = MPI_WIN_BASE
            CHKERR( MPI_Win_get_attr(win, attr, &pbase, &flag) )
            if flag and pbase != NULL: base = pbase
            #
            attr = MPI_WIN_SIZE
            CHKERR( MPI_Win_get_attr(win, attr, &psize, &flag) )
            if flag and psize != NULL: size = psize[0]
            #
            attr = MPI_WIN_DISP_UNIT
            CHKERR( MPI_Win_get_attr(win, attr, &pdisp, &flag) )
            if flag and pdisp != NULL: disp = pdisp[0]
            #
            return (<MPI_Aint>base, size, disp)

    property memory:
        """window memory buffer"""
        def __get__(self):
            cdef MPI_Win win = self.ob_mpi
            cdef void *base = NULL, *pbase = NULL
            cdef MPI_Aint size = 0, *psize = NULL
            cdef int attr = MPI_KEYVAL_INVALID
            cdef int flag = 0
            #
            attr = MPI_WIN_BASE
            CHKERR( MPI_Win_get_attr(win, attr, &pbase, &flag) )
            if flag and pbase != NULL: base = pbase
            #
            attr = MPI_WIN_SIZE
            CHKERR( MPI_Win_get_attr(win, attr, &psize, &flag) )
            if flag and psize != NULL: size = psize[0]
            #
            return tomemory(base, size)

    # [6.3] Communication Calls
    # -------------------------

    # [6.3.1] Put
    # -----------

    def Put(self, origin, int target_rank, target=None):
        """
        Put data into a memory window on a remote process.
        """
        cdef _p_msg_rma msg = message_rma()
        msg.for_put(origin, target_rank, target)
        with nogil: CHKERR( MPI_Put(
            msg.oaddr, msg.ocount, msg.otype,
            target_rank,
            msg.tdisp, msg.tcount, msg.ttype,
            self.ob_mpi) )

    # [6.3.2] Get
    # -----------

    def Get(self, origin, int target_rank, target=None):
        """
        Get data from a memory window on a remote process.
        """
        cdef _p_msg_rma msg = message_rma()
        msg.for_get(origin, target_rank, target)
        with nogil: CHKERR( MPI_Get(
            msg.oaddr, msg.ocount, msg.otype,
            target_rank,
            msg.tdisp, msg.tcount, msg.ttype,
            self.ob_mpi) )

    # [6.3.4] Accumulate Functions
    # ----------------------------

    def Accumulate(self, origin, int target_rank,
                   target=None, Op op not None=SUM):
        """
        Accumulate data into the target process
        using remote memory access.
        """
        cdef _p_msg_rma msg = message_rma()
        msg.for_acc(origin, target_rank, target)
        with nogil: CHKERR( MPI_Accumulate(
            msg.oaddr, msg.ocount, msg.otype,
            target_rank,
            msg.tdisp, msg.tcount, msg.ttype,
            op.ob_mpi, self.ob_mpi) )

    # [6.4] Synchronization Calls
    # ---------------------------

    # [6.4.1] Fence
    # -------------

    def Fence(self, int assertion=0):
        """
        Perform an MPI fence synchronization on a window
        """
        with nogil: CHKERR( MPI_Win_fence(assertion, self.ob_mpi) )

    # [6.4.2] General Active Target Synchronization
    # ---------------------------------------------

    def Start(self, Group group not None, int assertion=0):
        """
        Start an RMA access epoch for MPI
        """
        with nogil: CHKERR( MPI_Win_start(
            group.ob_mpi, assertion, self.ob_mpi) )

    def Complete(self):
        """
        Completes an RMA operations begun after an `Win.Start()`
        """
        with nogil: CHKERR( MPI_Win_complete(self.ob_mpi) )

    def Post(self, Group group not None, int assertion=0):
        """
        Start an RMA exposure epoch
        """
        with nogil: CHKERR( MPI_Win_post(
            group.ob_mpi, assertion, self.ob_mpi) )

    def Wait(self):
        """
        Complete an RMA exposure epoch begun with `Win.Post()`
        """
        with nogil: CHKERR( MPI_Win_wait(self.ob_mpi) )

    def Test(self):
        """
        Test whether an RMA exposure epoch has completed
        """
        cdef bint flag = 0
        with nogil: CHKERR( MPI_Win_test(self.ob_mpi, &flag) )
        return flag

    # [6.4.3] Lock
    # ------------

    def Lock(self, int lock_type, int rank, int assertion=0):
        """
        Begin an RMA access epoch at the target process
        """
        with nogil: CHKERR( MPI_Win_lock(
            lock_type, rank, assertion, self.ob_mpi) )

    def Unlock(self, int rank):
        """
        Complete an RMA access epoch at the target process
        """
        with nogil: CHKERR( MPI_Win_unlock(rank, self.ob_mpi) )

    # [6.6] Error Handling
    # --------------------

    def Get_errhandler(self):
        """
        Get the error handler for a window
        """
        cdef Errhandler errhandler = <Errhandler>Errhandler.__new__(Errhandler)
        CHKERR( MPI_Win_get_errhandler(self.ob_mpi, &errhandler.ob_mpi) )
        return errhandler

    def Set_errhandler(self, Errhandler errhandler not None):
        """
        Set the error handler for a window
        """
        CHKERR( MPI_Win_set_errhandler(self.ob_mpi, errhandler.ob_mpi) )

    def Call_errhandler(self, int errorcode):
        """
        Call the error handler installed on a window
        """
        CHKERR( MPI_Win_call_errhandler(self.ob_mpi, errorcode) )


    # [8.4] Naming Objects
    # --------------------

    def Get_name(self):
        """
        Get the print name associated with the window
        """
        cdef char name[MPI_MAX_OBJECT_NAME+1]
        cdef int nlen = 0
        CHKERR( MPI_Win_get_name(self.ob_mpi, name, &nlen) )
        return tompistr(name, nlen)

    def Set_name(self, name):
        """
        Set the print name associated with the window
        """
        cdef char *cname = NULL
        name = asmpistr(name, &cname, NULL)
        CHKERR( MPI_Win_set_name(self.ob_mpi, cname) )

    property name:
        """window name"""
        def __get__(self):
            return self.Get_name()
        def __set__(self, value):
            self.Set_name(value)

    # Fortran Handle
    # --------------

    def py2f(self):
        """
        """
        return MPI_Win_c2f(self.ob_mpi)

    @classmethod
    def f2py(cls, arg):
        """
        """
        cdef Win win = <Win>cls()
        win.ob_mpi = MPI_Win_f2c(arg)
        return win



cdef Win __WIN_NULL__ = new_Win(MPI_WIN_NULL)


# Predefined window handles
# -------------------------

WIN_NULL = __WIN_NULL__  #: Null window handle

#------------------------------------------------------------------------------

cdef extern from "Python.h":
    int is_int    "PyInt_Check" (object)
    int is_list   "PyList_Check" (object)
    int is_tuple  "PyTuple_Check" (object)

cdef inline bint is_buffer(object ob):
    return (PyObject_CheckBuffer(ob) or
            PyObject_CheckReadBuffer(ob))

#------------------------------------------------------------------------------

cdef object __BOTTOM__ = <MPI_Aint>MPI_BOTTOM

cdef object __IN_PLACE__ = <MPI_Aint>MPI_IN_PLACE

#------------------------------------------------------------------------------

cdef class _p_message:
    cdef _p_buffer buf
    cdef object count
    cdef object displ
    cdef Datatype type

cdef _p_message message_basic(object o_buf,
                              object o_type,
                              bint readonly,
                              #
                              void        **baddr,
                              MPI_Aint     *bsize,
                              MPI_Datatype *btype,
                              ):
    global TypeDict
    cdef _p_message m = <_p_message>_p_message.__new__(_p_message)
    cdef int f = (o_type is None)
    # special-case for BOTTOM or None,
    # an explicit MPI datatype is required
    if o_buf is __BOTTOM__ or o_buf is None:
        if isinstance(o_type, Datatype):
            m.type = <Datatype>o_type
        else:
            m.type = TypeDict[o_type]
        m.buf = newbuffer()
        baddr[0] = MPI_BOTTOM
        bsize[0] = 0
        btype[0] = m.type.ob_mpi
        return m
    #elif obuf
    # get buffer base address and length
    m.buf = getbuffer(o_buf, readonly, f)
    baddr[0] = <void*>    m.buf.view.buf
    bsize[0] = <MPI_Aint> m.buf.view.len
    # lookup datatype if not provided or not a Datatype
    if isinstance(o_type, Datatype):
        m.type = <Datatype>o_type
    elif o_type is None:
        m.type = TypeDict[getformat(m.buf)]
    else:
        m.type = TypeDict[o_type]
    btype[0] = m.type.ob_mpi
    # and we are done ...
    return m

cdef _p_message message_simple(object msg,
                               int readonly,
                               int rank,
                               int blocks,
                               #
                               void         **_addr,
                               int          *_count,
                               MPI_Datatype *_type,
                               ):
    # special-case PROC_NULL target rank
    if rank == MPI_PROC_NULL:
        _addr[0]  = NULL
        _count[0] = 0
        _type[0]  = MPI_BYTE
        return None
    # unpack message list/tuple
    cdef Py_ssize_t nargs = 0
    cdef object o_buf   = None
    cdef object o_count = None
    cdef object o_displ = None
    cdef object o_type  = None
    if is_buffer(msg):
        o_buf = msg
    elif is_list(msg) or is_tuple(msg):
        nargs = len(msg)
        if nargs == 2:
            (o_buf, o_count) = msg
            if (isinstance(o_count, Datatype) or
                isinstance(o_count, str)):
                (o_count, o_type) = None, o_count
            elif is_tuple(o_count) or is_list(o_count):
                (o_count, o_displ) = o_count
        elif nargs == 3:
            (o_buf, o_count, o_type) = msg
            if is_tuple(o_count) or is_list(o_count):
                (o_count, o_displ) = o_count
        elif nargs == 4:
            (o_buf, o_count, o_displ, o_type) = msg
        else:
            raise ValueError("message: expecting 2 to 4 items")
    else:
        raise TypeError("message: expecting buffer or list/tuple")
    # buffer: address, length, and datatype
    cdef void *baddr = NULL
    cdef MPI_Aint bsize = 0
    cdef MPI_Datatype btype = MPI_DATATYPE_NULL
    cdef _p_message m = message_basic(o_buf, o_type, readonly,
                                      &baddr, &bsize, &btype)
    # buffer: count and displacement
    cdef int count = 0 # number of datatype entries
    cdef int displ = 0 # from base buffer, in datatype entries
    cdef MPI_Aint offset = 0 # from base buffer, in bytes
    cdef MPI_Aint extent = 0, lb = 0
    if o_displ is not None:
        if o_count is None: raise ValueError(
            "message: cannot handle displacement, "
            "explicit count required")
        count = <int> o_count
        if count < 0: raise ValueError(
            "message: negative count %d" % count)
        displ = <int> o_displ
        if displ < 0: raise ValueError(
            "message: negative diplacement %d" % displ)
        if displ != 0:
            if btype == MPI_DATATYPE_NULL: raise ValueError(
                "message: cannot handle diplacement, "
                "datatype is null")
            CHKERR( MPI_Type_get_extent(btype, &lb, &extent) )
            offset = displ*extent # XXX overflow?
    elif o_count is not None:
        count = <int> o_count
        if count < 0:
            raise ValueError(
                "message: negative count %d" % count)
    elif bsize > 0:
        if btype == MPI_DATATYPE_NULL: raise ValueError(
            "message: cannot guess count, "
            "datatype is null")
        CHKERR( MPI_Type_get_extent(btype, &lb, &extent) )
        if extent <= 0: raise ValueError(
            ("message: cannot guess count, "
             "datatype extent %d (lb:%d, ub:%d)"
             ) % (extent, lb, lb+extent))
        if (bsize % extent) != 0: raise ValueError(
            ("message: cannot guess count, "
             "buffer length %d is not a multiple of "
             "datatype extent %d (lb:%d, ub:%d)"
             ) % (bsize, extent, lb, lb+extent))
        if blocks < 1: blocks = 1
        if ((bsize // extent) % blocks) != 0: raise ValueError(
            ("message: cannot guess count, "
             "number of datatype items %d is not a multiple of"
             "the required number of blocks %d"
             ) %  (bsize//extent, blocks))
        count = <int> ((bsize // extent) // blocks) # XXX overflow?
    if o_count is None: o_count = count
    if o_displ is None: o_displ = displ
    m.count = o_count
    m.displ = o_displ
    # sanity-check zero-sized messages
    if o_buf is None:
        if count != 0:
            raise ValueError(
                "message: buffer is None but count is %d" % count)
        if displ != 0:
            raise ValueError(
                "message: buffer is None but displacement is %d" % displ)
    # return collected message data
    _addr[0]  = <void*>(<char*>baddr + offset)
    _count[0] = count
    _type[0]  = btype
    return m

cdef _p_message message_vector(object msg,
                               int readonly,
                               int rank,
                               int blocks,
                               #
                               void        **_addr,
                               int         **_counts,
                               int         **_displs,
                               MPI_Datatype *_type,
                               ):
    # special-case PROC_NULL target rank
    if rank == MPI_PROC_NULL:
        _addr[0]   = NULL
        _counts[0] = NULL
        _displs[0] = NULL
        _type[0]   = MPI_BYTE
        return None
    # unpack message list/tuple
    cdef Py_ssize_t nargs = 0
    cdef object o_buf    = None
    cdef object o_counts = None
    cdef object o_displs = None
    cdef object o_type   = None
    if is_buffer(msg):
        o_buf = msg
    elif is_list(msg) or is_tuple(msg):
        nargs = len(msg)
        if nargs == 2:
            (o_buf, o_counts) = msg
            if (isinstance(o_counts, Datatype) or
                isinstance(o_counts, str)):
                (o_counts, o_type) = None, o_counts
            elif is_tuple(o_counts):
                (o_counts, o_displs) = o_counts
        elif nargs == 3:
            (o_buf, o_counts, o_type) = msg
            if is_tuple(o_counts):
                (o_counts, o_displs) = o_counts
        elif nargs == 4:
            (o_buf, o_counts, o_displs, o_type) = msg
        else:
            raise ValueError("message: expecting 2 to 4 items")
    else:
        raise TypeError("message: expecting buffer or list/tuple")
    # buffer: address, length, and datatype
    cdef void *baddr = NULL
    cdef MPI_Aint bsize = 0
    cdef MPI_Datatype btype = MPI_DATATYPE_NULL
    cdef _p_message m = message_basic(o_buf, o_type, readonly,
                                      &baddr, &bsize, &btype)
    # counts and displacements
    cdef int *counts = NULL
    cdef int *displs = NULL
    cdef int i=0, val=0
    cdef MPI_Aint extent=0, lb=0
    cdef MPI_Aint asize=0, aval=0
    if o_counts is None:
        if bsize > 0:
            if btype == MPI_DATATYPE_NULL:
                raise ValueError(
                    "message: cannot guess count, "
                    "datatype is null")
            CHKERR( MPI_Type_get_extent(btype, &lb, &extent) )
            if extent <= 0: raise ValueError(
                ("message: cannot guess count, "
                 "datatype extent %d (lb:%d, ub:%d)"
                 ) % (extent, lb, lb+extent))
            if (bsize % extent) != 0: raise ValueError(
                ("message: cannot guess count, "
                 "buffer length %d is not a multiple of "
                 "datatype extent %d (lb:%d, ub:%d)"
                 ) % (bsize, extent, lb, lb+extent))
            asize = bsize // extent
        o_counts = newarray_int(blocks, &counts)
        for i from 0 <= i < blocks:
            aval = (asize // blocks) + (asize % blocks > i)
            counts[i] = <int> aval # XXX overflow?
    elif is_int(o_counts):
        val = <int> o_counts
        o_counts = newarray_int(blocks, &counts)
        for i from 0 <= i < blocks:
            counts[i] = val
    else:
        o_counts = chkarray_int(o_counts, blocks, &counts)
    if o_displs is None: # contiguous
        val = 0
        o_displs = newarray_int(blocks, &displs)
        for i from 0 <= i < blocks:
            displs[i] = val
            val += counts[i]
    elif is_int(o_displs): # strided
        val = <int> o_displs
        o_displs = newarray_int(blocks, &displs)
        for i from 0 <= i < blocks:
            displs[i] = val * i
    else: # general
        o_displs = chkarray_int(o_displs, blocks, &displs)
    m.count = o_counts
    m.displ = o_displs
    # return collected message data
    _addr[0]   = baddr
    _counts[0] = counts
    _displs[0] = displs
    _type[0]   = btype
    return m

#------------------------------------------------------------------------------

cdef class _p_msg_p2p:

    # raw C-side arguments
    cdef void         *buf
    cdef int          count
    cdef MPI_Datatype dtype
    # python-side argument
    cdef object _msg

    def __cinit__(self):
        self.buf   = NULL
        self.count = 0
        self.dtype = MPI_DATATYPE_NULL

    cdef int for_send(self, object msg, int rank) except -1:
        self._msg = message_simple(msg, 1, # readonly
                                   rank, 0,
                                   &self.buf,
                                   &self.count,
                                   &self.dtype)
        return 0

    cdef int for_recv(self, object msg, int rank) except -1:
        self._msg = message_simple(msg, 0, # writable
                                   rank, 0,
                                   &self.buf,
                                   &self.count,
                                   &self.dtype)
        return 0

cdef inline _p_msg_p2p message_p2p_send(object sendbuf, int dest):
    cdef _p_msg_p2p msg = <_p_msg_p2p>_p_msg_p2p.__new__(_p_msg_p2p)
    msg.for_send(sendbuf, dest)
    return msg

cdef inline _p_msg_p2p message_p2p_recv(object recvbuf, int source):
    cdef _p_msg_p2p msg = <_p_msg_p2p>_p_msg_p2p.__new__(_p_msg_p2p)
    msg.for_recv(recvbuf, source)
    return msg

#------------------------------------------------------------------------------

cdef class _p_msg_cco:

    # raw C-side arguments
    cdef void *sbuf, *rbuf
    cdef int  scount, rcount
    cdef int *scounts, *rcounts
    cdef int *sdispls, *rdispls
    cdef MPI_Datatype stype, rtype
    # python-side arguments
    cdef object _smsg, _rmsg
    cdef object _rcnt

    def __cinit__(self):
        self.sbuf    = self.rbuf    = NULL
        self.scount  = self.rcount  = 0
        self.scounts = self.rcounts = NULL
        self.sdispls = self.rdispls = NULL
        self.stype   = self.rtype   = MPI_DATATYPE_NULL

    # Collective Communication Operations
    # -----------------------------------

    # sendbuf arguments
    cdef int for_cco_send(self, int vector,
                          object amsg,
                          int root, int size) except -1:
        if not vector: # block variant
            self._smsg = message_simple(
                amsg, 1, root, size,
                &self.sbuf, &self.scount, &self.stype)
        else: # vector variant
            self._smsg = message_vector(
                amsg, 1, root, size,
                &self.sbuf, &self.scounts, &self.sdispls, &self.stype)
        return 0

    # recvbuf arguments
    cdef int for_cco_recv(self, int vector,
                          object amsg,
                          int root, int size) except -1:
        if not vector: # block variant
            self._rmsg = message_simple(
                amsg, 0, root, size,
                &self.rbuf, &self.rcount, &self.rtype)
        else: # vector variant
            self._rmsg = message_vector(
                amsg, 0, root, size,
                &self.rbuf, &self.rcounts, &self.rdispls, &self.rtype)
        return 0

    # bcast
    cdef int for_bcast(self,
                       object msg, int root,
                       MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        cdef int inter=0, rank=0, sending=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_rank(comm, &rank) )
            if root == rank:
                self.for_cco_send(0, msg, root, 0)
                sending = 1
            else:
                self.for_cco_recv(0, msg, root, 0)
        else: # inter-communication
            if ((root == <int>MPI_ROOT) or
                (root == <int>MPI_PROC_NULL)):
                self.for_cco_send(0, msg, root, 0)
                sending = 1
            else:
                self.for_cco_recv(0, msg, root, 0)
        if sending:
            self.rbuf   = self.sbuf
            self.rcount = self.scount
            self.rtype  = self.stype
        else:
            self.sbuf   = self.rbuf
            self.scount = self.rcount
            self.stype  = self.rtype
        return 0

    # gather/gatherv
    cdef int for_gather(self, int v,
                        object smsg, object rmsg,
                        int root, MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        cdef int inter=0, size=0, rank=0, null=MPI_PROC_NULL
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_size(comm, &size) )
            CHKERR( MPI_Comm_rank(comm, &rank) )
            if root == rank:
                self.for_cco_recv(v, rmsg, root, size)
                if smsg is __IN_PLACE__:
                    self.sbuf   = MPI_IN_PLACE
                    self.scount = self.rcount
                    self.stype  = self.rtype
                else:
                    self.for_cco_send(0, smsg, 0, 0)
            else:
                self.for_cco_recv(v, rmsg, null, size)
                self.for_cco_send(0, smsg, root, 0)
        else: # inter-communication
            CHKERR( MPI_Comm_remote_size(comm, &size) )
            if ((root == <int>MPI_ROOT) or
                (root == <int>MPI_PROC_NULL)):
                self.for_cco_recv(v, rmsg, root, size)
                self.for_cco_send(0, smsg, null, 0)
            else:
                self.for_cco_recv(v, rmsg, null, size)
                self.for_cco_send(0, smsg, root, 0)
        return 0

    # scatter/scatterv
    cdef int for_scatter(self, int v,
                         object smsg, object rmsg,
                         int root, MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        cdef int inter=0, size=0, rank=0, null=MPI_PROC_NULL
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_size(comm, &size) )
            CHKERR( MPI_Comm_rank(comm, &rank) )
            if root == rank:
                self.for_cco_send(v, smsg, root, size)
                if rmsg is __IN_PLACE__:
                    self.rbuf   = MPI_IN_PLACE
                    self.rcount = self.scount
                    self.rtype  = self.stype
                else:
                    self.for_cco_recv(0, rmsg, root, 0)
            else:
                self.for_cco_send(v, smsg, null, size)
                self.for_cco_recv(0, rmsg, root, 0)
        else: # inter-communication
            CHKERR( MPI_Comm_remote_size(comm, &size) )
            if ((root == <int>MPI_ROOT) or
                (root == <int>MPI_PROC_NULL)):
                self.for_cco_send(v, smsg, root, size)
                self.for_cco_recv(0, rmsg, null,  0)
            else:
                self.for_cco_send(v, smsg, null, size)
                self.for_cco_recv(0, rmsg, root, 0)
        return 0

    # allgather/allgatherv
    cdef int for_allgather(self, int v,
                           object smsg, object rmsg,
                           MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        cdef int inter=0, size=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_size(comm, &size) )
        else: # inter-communication
            CHKERR( MPI_Comm_remote_size(comm, &size) )
        #
        self.for_cco_recv(v, rmsg, 0, size)
        if not inter and smsg is __IN_PLACE__:
            self.sbuf   = MPI_IN_PLACE
            self.scount = self.rcount
            self.stype  = self.rtype
        else:
            self.for_cco_send(0, smsg, 0, 0)
        return 0

    # alltoall/alltoallv
    cdef int for_alltoall(self, int v,
                          object smsg, object rmsg,
                          MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        cdef int inter=0, size=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_size(comm, &size) )
        else: # inter-communication
            CHKERR( MPI_Comm_remote_size(comm, &size) )
        #
        self.for_cco_recv(v, rmsg, 0, size)
        if not inter and smsg is __IN_PLACE__:
            self.sbuf    = MPI_IN_PLACE
            self.scount  = self.rcount
            self.scounts = self.rcounts
            self.sdispls = self.rdispls
            self.stype   = self.rtype
        else:
            self.for_cco_send(v, smsg, 0, size)
        return 0


    # Collective Reductions Operations
    # --------------------------------

    # sendbuf
    cdef int for_cro_send(self, object amsg, int root) except -1:
        self._smsg = message_simple(amsg, 1, # readonly
                                    root, 0,
                                    &self.sbuf,
                                    &self.scount,
                                    &self.stype)
        return 0

    # recvbuf
    cdef int for_cro_recv(self, object amsg, int root) except -1:
        self._rmsg = message_simple(amsg, 0, # writable
                                    root, 0,
                                    &self.rbuf,
                                    &self.rcount,
                                    &self.rtype)
        return 0

    cdef int for_reduce(self,
                        object smsg, object rmsg,
                        int root, MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        cdef int inter=0, rank=0, null=MPI_PROC_NULL
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        if not inter: # intra-communication
            CHKERR( MPI_Comm_rank(comm, &rank) )
            if root == rank:
                self.for_cro_recv(rmsg, root)
                if smsg is __IN_PLACE__:
                    self.sbuf   = MPI_IN_PLACE
                    self.scount = self.rcount
                    self.stype  = self.rtype
                else:
                    self.for_cro_send(smsg, root)
            else:
                self.for_cro_recv(rmsg, null)
                self.for_cro_send(smsg, root)
                self.rcount = self.scount
                self.rtype  = self.stype
        else: # inter-communication
            if ((root == <int>MPI_ROOT) or
                (root == <int>MPI_PROC_NULL)):
                self.for_cro_recv(rmsg, root)
                self.scount = self.rcount
                self.stype  = self.rtype
            else:
                self.for_cro_send(smsg, root)
                self.rcount = self.scount
                self.rtype  = self.stype
        return 0

    cdef int for_allreduce(self,
                           object smsg, object rmsg,
                           MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        cdef int inter=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        # get send and recv buffers
        self.for_cro_recv(rmsg, 0)
        if not inter and smsg is __IN_PLACE__:
            self.sbuf   = MPI_IN_PLACE
            self.scount = self.rcount
            self.stype  = self.rtype
        else:
            self.for_cro_send(smsg, 0)
        # check counts and datatypes
        if (self.sbuf   != MPI_IN_PLACE and
            self.scount != self.rcount):
            raise ValueError(
                "mismatch in send count %d and receive count %d" %
                (self.scount, self.rcount))
        if (self.sbuf  != MPI_IN_PLACE and
            self.stype != self.rtype):
            raise ValueError(
                "mismatch in send and receive MPI datatypes")
        return 0

    cdef int for_reduce_scatter_block(self,
                                      object smsg, object rmsg,
                                      MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        cdef int inter=0, size=0
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        CHKERR( MPI_Comm_size(comm, &size) )
        # get send and recv buffers
        if not inter and smsg is __IN_PLACE__:
            self.for_cco_recv(0, rmsg, 0, size)
            self.sbuf = MPI_IN_PLACE
        else:
            self.for_cco_recv(0, rmsg, 0, 0)
            self.for_cco_send(0, smsg, 0, size)
        # check counts and datatypes
        if self.sbuf != MPI_IN_PLACE:
            if self.stype != self.rtype:
                raise ValueError(
                    "mismatch in send and receive MPI datatypes")
            if self.scount != self.rcount:
                raise ValueError(
                    "mismatch in send count %d receive count %d" %
                    (self.scount, self.rcount*size))
        return 0

    cdef int for_reduce_scatter(self,
                                object smsg, object rmsg, object rcnt,
                                MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        cdef int inter=0, size=0, rank=MPI_PROC_NULL
        CHKERR( MPI_Comm_test_inter(comm, &inter) )
        CHKERR( MPI_Comm_size(comm, &size) )
        CHKERR( MPI_Comm_rank(comm, &rank) )
        # get send and recv buffers
        self.for_cro_recv(rmsg, 0)
        if not inter and smsg is __IN_PLACE__:
            self.sbuf = MPI_IN_PLACE
        else:
            self.for_cro_send(smsg, 0)
        # get receive counts
        if rcnt is None and not inter and self.sbuf != MPI_IN_PLACE:
            self._rcnt = newarray_int(size, &self.rcounts)
            CHKERR( MPI_Allgather(&self.rcount, 1, MPI_INT,
                                  self.rcounts, 1, MPI_INT, comm) )
        else:
            self._rcnt = chkarray_int(rcnt, size, &self.rcounts)
        # total sum or receive counts
        cdef int i=0, sumrcounts=0
        for i from 0 <= i < size:
            sumrcounts += self.rcounts[i]
        # check counts and datatypes
        if self.sbuf != MPI_IN_PLACE:
            if self.stype != self.rtype:
                raise ValueError(
                    "mismatch in send and receive MPI datatypes")
            if self.scount != sumrcounts:
                raise ValueError(
                    "mismatch in send count %d and sum(counts) %d" %
                    (self.scount, sumrcounts))
            if self.rcount != self.rcounts[rank]:
                raise ValueError(
                    "mismatch in receive count %d and counts[%d] %d" %
                    (self.rcount, rank, self.rcounts[rank]))
        else:
            if self.rcount != sumrcounts:
                raise ValueError(
                    "mismatch in receive count %d and sum(counts) %d" %
                    (self.rcount, sumrcounts))
        return 0

    cdef int for_scan(self,
                      object smsg, object rmsg,
                      MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        # get send and recv buffers
        self.for_cro_recv(rmsg, 0)
        if smsg is __IN_PLACE__:
            self.sbuf   = MPI_IN_PLACE
            self.scount = self.rcount
            self.stype  = self.rtype
        else:
            self.for_cro_send(smsg, 0)
        # check counts and datatypes
        if (self.sbuf   != MPI_IN_PLACE and
            self.scount != self.rcount):
            raise ValueError(
                "mismatch in send count %d and receive count %d" %
                (self.scount, self.rcount))
        if (self.sbuf  != MPI_IN_PLACE and
            self.stype != self.rtype):
            raise ValueError(
                "mismatch in send and receive MPI datatypes")
        return 0

    cdef int for_exscan(self,
                        object smsg, object rmsg,
                        MPI_Comm comm) except -1:
        if comm == MPI_COMM_NULL: return 0
        # get send and recv buffers
        self.for_cro_recv(rmsg, 0)
        if smsg is __IN_PLACE__:
            self.sbuf   = MPI_IN_PLACE
            self.scount = self.rcount
            self.stype  = self.rtype
        else:
            self.for_cro_send(smsg, 0)
        # check counts and datatypes
        if self.scount != self.rcount:
            raise ValueError(
                "mismatch in send count %d and receive count %d" %
                (self.scount, self.rcount))
        if self.stype != self.rtype:
            raise ValueError(
                "mismatch in send and receive MPI datatypes")
        return 0


cdef inline _p_msg_cco message_cco():
    cdef _p_msg_cco msg = <_p_msg_cco>_p_msg_cco.__new__(_p_msg_cco)
    return msg

#------------------------------------------------------------------------------

cdef class _p_msg_rma:

    # raw origin arguments
    cdef void*        oaddr
    cdef int          ocount
    cdef MPI_Datatype otype
    # raw target arguments
    cdef MPI_Aint     tdisp
    cdef int          tcount
    cdef MPI_Datatype ttype
    # python-side arguments
    cdef object _origin
    cdef object _target

    def __cinit__(self):
        self.oaddr  = NULL
        self.ocount = 0
        self.otype  = MPI_DATATYPE_NULL
        self.tdisp  = 0
        self.tcount = 0
        self.ttype  = MPI_DATATYPE_NULL

    cdef int for_rma(self, int readonly,
                     object origin, int rank, object target) except -1:
        # ORIGIN
        self._origin = message_simple(
            origin, readonly, rank, 0,
            &self.oaddr,  &self.ocount,  &self.otype)
        if ((rank == MPI_PROC_NULL) and
            (origin is not None) and
            (is_list(origin) or is_tuple(origin)) and
            (len(origin) > 0 and isinstance(origin[-1], Datatype))):
            self.otype  = (<Datatype?>origin[-1]).ob_mpi
            self._origin = origin
        # TARGET
        if target is None:
            self.tdisp  = 0
            self.tcount = self.ocount
            self.ttype  = self.otype
        elif is_int(target):
            self.tdisp  = <MPI_Aint>target
            self.tcount = self.ocount
            self.ttype  = self.otype
        elif is_list(target) or is_tuple(target):
            if len(target) != 3:
                raise ValueError("target: expecting 3 items")
            self.tdisp  = <MPI_Aint>target[0]
            self.tcount = <int>target[1]
            self.ttype  = (<Datatype?>target[2]).ob_mpi
        else:
            raise ValueError("target: expecting integral or list/tuple")
        self._target = target
        return 0

    cdef int for_put(self, object origin, int rank, object target) except -1:
        self.for_rma(1, origin, rank, target)
        return 0

    cdef int for_get(self, object origin, int rank, object target) except -1:
        self.for_rma(0, origin, rank, target)
        return 0

    cdef int for_acc(self, object origin, int rank, object target) except -1:
        self.for_rma(1, origin, rank, target)
        return 0

cdef inline _p_msg_rma message_rma():
    cdef _p_msg_rma msg = <_p_msg_rma>_p_msg_rma.__new__(_p_msg_rma)
    return msg

#------------------------------------------------------------------------------

cdef class _p_msg_io:

    # raw C-side data
    cdef void         *buf
    cdef int          count
    cdef MPI_Datatype dtype
    # python-side data
    cdef object _msg

    def __cinit__(self):
        self.buf   = NULL
        self.count = 0
        self.dtype = MPI_DATATYPE_NULL

    cdef int for_read(self, object msg) except -1:
        self._msg = message_simple(msg, 0, # writable
                                   0, 0,
                                   &self.buf,
                                   &self.count,
                                   &self.dtype)
        return 0

    cdef int for_write(self, object msg) except -1:
        self._msg = message_simple(msg, 1, # readonly
                                   0, 0,
                                   &self.buf,
                                   &self.count,
                                   &self.dtype)
        return 0

cdef inline _p_msg_io message_io_read(object buf):
    cdef _p_msg_io msg = <_p_msg_io>_p_msg_io.__new__(_p_msg_io)
    msg.for_read(buf)
    return msg

cdef inline _p_msg_io message_io_write(object buf):
    cdef _p_msg_io msg = <_p_msg_io>_p_msg_io.__new__(_p_msg_io)
    msg.for_write(buf)
    return msg

#------------------------------------------------------------------------------

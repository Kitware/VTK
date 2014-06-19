# -----------------------------------------------------------------------------

cdef extern from "Python.h":
    enum:      PY_MAJOR_VERSION
    bint       PyBytes_CheckExact(object)
    char*      PyBytes_AsString(object) except NULL
    Py_ssize_t PyBytes_Size(object) except -1
    object     PyBytes_FromStringAndSize(char*,Py_ssize_t)

# -----------------------------------------------------------------------------

cdef object PyPickle_dumps = None
cdef object PyPickle_loads = None
cdef object PyPickle_PROTOCOL = -1

try:
    from cPickle import dumps as PyPickle_dumps
    from cPickle import loads as PyPickle_loads
    from cPickle import HIGHEST_PROTOCOL as PyPickle_PROTOCOL
except ImportError:
    from pickle import dumps as PyPickle_dumps
    from pickle import loads as PyPickle_loads
    from pickle import HIGHEST_PROTOCOL as PyPickle_PROTOCOL

cdef object PyStringIO_New = None
cdef object PyPickle_loadf  = None
try:
    from cStringIO import StringIO as PyStringIO_New
    from cPickle import load as PyPickle_loadf
except ImportError:
    pass

cdef class _p_Pickle:

    cdef object ob_dumps
    cdef object ob_loads
    cdef object ob_PROTOCOL

    def __cinit__(self):
        self.ob_dumps = None
        self.ob_loads = None
        self.ob_PROTOCOL = PyPickle_PROTOCOL

    property dumps:
        def __get__(self):
            if self.ob_dumps is None:
                return PyPickle_dumps
            else:
                return self.ob_dumps
        def __set__(self, dumps):
            if dumps is PyPickle_dumps:
                self.ob_dumps = None
            else:
                self.ob_dumps = dumps

    property loads:
        def __get__(self):
            if self.ob_loads is None:
                return PyPickle_loads
            else:
                return self.ob_loads
        def __set__(self, loads):
            if loads is PyPickle_loads:
                self.ob_loads = None
            else:
                self.ob_loads = loads

    property PROTOCOL:
        def __get__(self):
            return self.ob_PROTOCOL
        def __set__(self, PROTOCOL):
            self.ob_PROTOCOL = PROTOCOL

    cdef object dump(self, object obj, void **p, int *n):
        if obj is None:
            p[0] = NULL
            n[0] = 0
            return None
        cdef object buf
        if self.ob_dumps is None:
            buf = PyPickle_dumps(obj, self.ob_PROTOCOL)
        else:
            buf = self.ob_dumps(obj, self.ob_PROTOCOL)
        p[0] = <void*> PyBytes_AsString(buf)
        n[0] = <int>   PyBytes_Size(buf) # XXX overflow?
        return buf

    cdef object alloc(self, void **p, int n):
        if n == 0:
            p[0] = NULL
            return None
        cdef object buf
        buf = PyBytes_FromStringAndSize(NULL, n)
        p[0] = PyBytes_AsString(buf)
        return buf

    cdef object load(self, object buf):
        if buf is None: return None
        cdef bint use_StringIO = \
            (PY_MAJOR_VERSION == 2 and
             not PyBytes_CheckExact(buf) and
             PyStringIO_New is not None)
        if self.ob_loads is None:
            if use_StringIO:
                buf = PyStringIO_New(buf)
                if PyPickle_loadf is not None:
                    return PyPickle_loadf(buf)
                buf = buf.read()
            return PyPickle_loads(buf)
        else:
            if use_StringIO:
                buf = PyStringIO_New(buf)
                buf = buf.read()
            return self.ob_loads(buf)

    cdef object dumpv(self, object obj, void **p,
                      int n, int cnt[], int dsp[]):
        cdef Py_ssize_t i=0, m=n
        if obj is None:
            p[0] = NULL
            for i from 0 <= i < m:
                cnt[i] = 0
                dsp[i] = 0
            return None
        cdef object items = list(obj)
        m = len(items)
        if m != n: raise ValueError(
            "expecting %d items, got %d" % (n, m))
        cdef int d=0, c=0
        for i from 0 <= i < m:
            items[i] = self.dump(items[i], p, &c)
            if c == 0: items[i] = b''
            cnt[i] = c; dsp[i] = d; d += c
        cdef object buf = b''.join(items) # XXX use _PyBytes_Join() ?
        p[0] = PyBytes_AsString(buf)
        return buf

    cdef object allocv(self, void **p,
                       int n, int cnt[], int dsp[]):
        cdef int i=0, d=0
        for i from 0 <= i < n:
            dsp[i] = d
            d += cnt[i]
        return self.alloc(p, d)

    cdef object loadv(self, object obj,
                      int n, int cnt[], int dsp[]):
        cdef Py_ssize_t i=0, m=n
        cdef object items = [None] * m
        if obj is None: return items
        cdef char *p = PyBytes_AsString(obj)
        cdef object buf = None
        for i from 0 <= i < m:
            if cnt[i] == 0: continue
            buf = PyBytes_FromStringAndSize(p+dsp[i], cnt[i])
            items[i] = self.load(buf)
        return items


cdef _p_Pickle PyMPI_PICKLE = _p_Pickle()

cdef inline _p_Pickle PyMPI_pickle():
    return PyMPI_PICKLE

_p_pickle = PyMPI_PICKLE

# -----------------------------------------------------------------------------

cdef object PyMPI_send(object obj, int dest, int tag,
                       MPI_Comm comm):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    #
    cdef int dosend = (dest != MPI_PROC_NULL)
    #
    cdef object smsg = None
    if dosend: smsg = pickle.dump(obj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Send(sbuf, scount, stype,
                                 dest, tag, comm) )
    return None


cdef object PyMPI_bsend(object obj, int dest, int tag,
                        MPI_Comm comm):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    #
    cdef int dosend = (dest != MPI_PROC_NULL)
    #
    cdef object smsg = None
    if dosend: smsg = pickle.dump(obj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Bsend(sbuf, scount, stype,
                                  dest, tag, comm) )
    return None


cdef object PyMPI_ssend(object obj, int dest, int tag,
                        MPI_Comm comm):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    #
    cdef int dosend = (dest != MPI_PROC_NULL)
    #
    cdef object smsg = None
    if dosend: smsg = pickle.dump(obj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Ssend(sbuf, scount, stype,
                                  dest, tag, comm) )
    return None


cdef object PyMPI_recv(object obj, int source, int tag,
                       MPI_Comm comm, MPI_Status *status):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *rbuf = NULL
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int dorecv = (source != MPI_PROC_NULL)
    #
    cdef object rmsg = None
    cdef MPI_Status rsts
    cdef _p_buffer m
    if dorecv:
        if obj is None:
            with nogil:
                CHKERR( MPI_Probe(source, tag, comm, &rsts) )
                CHKERR( MPI_Get_count(&rsts, rtype, &rcount) )
            rmsg = pickle.alloc(&rbuf, rcount)
            source = rsts.MPI_SOURCE
            tag = rsts.MPI_TAG
        else:
            rmsg = m = getbuffer(obj, 0, 0)
            rbuf = m.view.buf
            rcount = <int> m.view.len # XXX overflow?
    #
    with nogil: CHKERR( MPI_Recv(rbuf, rcount, rtype,
                                 source, tag, comm, status) )
    if dorecv: rmsg = pickle.load(rmsg)
    return rmsg


cdef object PyMPI_sendrecv(object sobj, int dest,   int sendtag,
                           object robj, int source, int recvtag,
                           MPI_Comm comm, MPI_Status *status):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int dosend = (dest   != MPI_PROC_NULL)
    cdef int dorecv = (source != MPI_PROC_NULL)
    #
    cdef object smsg = None
    if dosend: smsg = pickle.dump(sobj, &sbuf, &scount)
    cdef MPI_Request sreq = MPI_REQUEST_NULL
    with nogil: CHKERR( MPI_Isend(sbuf, scount, stype,
                                  dest, sendtag, comm, &sreq) )
    #
    cdef object rmsg = None
    cdef MPI_Status rsts
    cdef _p_buffer m
    if dorecv:
        if robj is None:
            with nogil:
                CHKERR( MPI_Probe(source, recvtag, comm, &rsts) )
                CHKERR( MPI_Get_count(&rsts, rtype, &rcount) )
            rmsg = pickle.alloc(&rbuf, rcount)
            source = rsts.MPI_SOURCE
            recvtag = rsts.MPI_TAG
        else:
            rmsg = m = getbuffer(robj, 0, 0)
            rbuf = m.view.buf
            rcount = <int> m.view.len # XXX overflow?
    #
    with nogil:
        CHKERR( MPI_Recv(rbuf, rcount, rtype,
                         source, recvtag, comm, status) )
        CHKERR( MPI_Wait(&sreq, MPI_STATUS_IGNORE) )
    if dorecv: rmsg = pickle.load(rmsg)
    return rmsg

# -----------------------------------------------------------------------------

cdef object PyMPI_isend(object obj, int dest, int tag,
                        MPI_Comm comm, MPI_Request *request):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    #
    cdef object smsg = None
    cdef int dosend = (dest != MPI_PROC_NULL)
    if dosend: smsg = pickle.dump(obj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Isend(sbuf, scount, stype,
                                  dest, tag, comm, request) )
    return smsg


cdef object PyMPI_ibsend(object obj, int dest, int tag,
                         MPI_Comm comm, MPI_Request *request):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    #
    cdef object smsg = None
    cdef int dosend = (dest != MPI_PROC_NULL)
    if dosend: smsg = pickle.dump(obj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Ibsend(sbuf, scount, stype,
                                   dest, tag, comm, request) )
    return smsg


cdef object PyMPI_issend(object obj, int dest, int tag,
                         MPI_Comm comm, MPI_Request *request):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    #
    cdef object smsg = None
    cdef int dosend = (dest != MPI_PROC_NULL)
    if dosend: smsg = pickle.dump(obj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Issend(sbuf, scount, stype,
                                   dest, tag, comm, request) )
    return smsg


cdef object PyMPI_irecv(object obj, int dest, int tag,
                        MPI_Comm comm, MPI_Request *request):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *rbuf = NULL
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef _p_buffer rmsg = None
    cdef int dorecv = (dest != MPI_PROC_NULL)
    if dorecv:
        if obj is None:
            rcount = <int>(1<<15)
            obj = pickle.alloc(&rbuf, rcount)
            rmsg = getbuffer(obj, 1, 0)
        #elif is_int(obj):
        #    rcount = <int> obj
        #    obj = pickle.alloc(&rbuf, rcount)
        #    rmsg = getbuffer(obj, 1, 0)
        else:
            rmsg = getbuffer(obj, 0, 0)
            rbuf = rmsg.view.buf
            rcount = <int> rmsg.view.len # XXX overflow?
    with nogil: CHKERR( MPI_Irecv(rbuf, rcount, rtype,
                                  dest, tag, comm, request) )
    return rmsg


cdef object PyMPI_wait(Request request, Status status):
    cdef _p_Pickle pickle = PyMPI_pickle()
    cdef object buf
    #
    cdef MPI_Status rsts
    with nogil: CHKERR( MPI_Wait(&request.ob_mpi, &rsts) )
    buf = request.ob_buf
    if status is not None:
        status.ob_mpi = rsts
    if request.ob_mpi == MPI_REQUEST_NULL:
        request.ob_buf = None
    #
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    if type(buf) is not _p_buffer: return None
    CHKERR( MPI_Get_count(&rsts, rtype, &rcount) )
    if rcount <= 0: return None
    return pickle.load(buf)


cdef object PyMPI_test(Request request, int *flag, Status status):
    cdef _p_Pickle pickle = PyMPI_pickle()
    cdef object buf
    #
    cdef MPI_Status rsts
    with nogil: CHKERR( MPI_Test(&request.ob_mpi, flag, &rsts) )
    if flag[0]:
        buf = request.ob_buf
    if status is not None:
        status.ob_mpi = rsts
    if request.ob_mpi == MPI_REQUEST_NULL:
        request.ob_buf = None
    #
    if not flag[0]: return None
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    if type(buf) is not _p_buffer: return None
    CHKERR( MPI_Get_count(&rsts, rtype, &rcount) )
    if rcount <= 0: return None
    return pickle.load(buf)


cdef object PyMPI_waitany(requests, int *index, Status status):
    cdef _p_Pickle pickle = PyMPI_pickle()
    cdef object buf
    #
    cdef int count = 0
    cdef MPI_Request *irequests = NULL
    cdef MPI_Status rsts
    #
    cdef tmp = acquire_rs(requests, None, &count, &irequests, NULL)
    try:
        with nogil: CHKERR( MPI_Waitany(count, irequests, index, &rsts) )
        if index[0] != MPI_UNDEFINED:
            buf = (<Request>requests[<Py_ssize_t>index[0]]).ob_buf
        if status is not None:
            status.ob_mpi = rsts
    finally:
        release_rs(requests, None, count, irequests, NULL)
    #
    if index[0] == MPI_UNDEFINED: return None
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    if type(buf) is not _p_buffer: return None
    CHKERR( MPI_Get_count(&rsts, rtype, &rcount) )
    if rcount <= 0: return None
    return pickle.load(buf)


cdef object PyMPI_testany(requests, int *index, int *flag, Status status):
    cdef _p_Pickle pickle = PyMPI_pickle()
    cdef object buf
    #
    cdef int count = 0
    cdef MPI_Request *irequests = NULL
    cdef MPI_Status rsts
    #
    cdef tmp = acquire_rs(requests, None, &count, &irequests, NULL)
    try:
        with nogil: CHKERR( MPI_Testany(count, irequests, index, flag, &rsts) )
        if index[0] != MPI_UNDEFINED:
            buf = (<Request>requests[<Py_ssize_t>index[0]]).ob_buf
        if status is not None:
            status.ob_mpi = rsts
    finally:
        release_rs(requests, None, count, irequests, NULL)
    #
    if index[0] == MPI_UNDEFINED: return None
    if not flag[0]: return None
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    if type(buf) is not _p_buffer: return None
    CHKERR( MPI_Get_count(&rsts, rtype, &rcount) )
    if rcount <= 0: return None
    return pickle.load(buf)


cdef object PyMPI_waitall(requests, statuses):
    cdef _p_Pickle pickle = PyMPI_pickle()
    cdef object buf, bufs
    #
    cdef Py_ssize_t i = 0
    cdef int count = 0
    cdef MPI_Request *irequests = NULL
    cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
    #
    cdef tmp = acquire_rs(requests, True, &count, &irequests, &istatuses)
    try:
        with nogil: CHKERR( MPI_Waitall(count, irequests, istatuses) )
        bufs = [(<Request>requests[i]).ob_buf for i from 0 <= i < count]
    finally:
        release_rs(requests, statuses, count, irequests, NULL)
    #
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    for i from 0 <= i < count:
        if type(bufs[i]) is not _p_buffer:
            bufs[i] = None; continue
        rcount = 0
        CHKERR( MPI_Get_count(&istatuses[i], rtype, &rcount) )
        if rcount <= 0: bufs[i] = None
    return [pickle.load(buf) for buf in bufs]


cdef object PyMPI_testall(requests, int *flag, statuses):
    cdef _p_Pickle pickle = PyMPI_pickle()
    cdef object buf, bufs
    #
    cdef Py_ssize_t i = 0
    cdef int count = 0
    cdef MPI_Request *irequests = NULL
    cdef MPI_Status *istatuses = MPI_STATUSES_IGNORE
    #
    cdef tmp = acquire_rs(requests, True, &count, &irequests, &istatuses)
    try:
        with nogil: CHKERR( MPI_Testall(count, irequests, flag, istatuses) )
        if flag[0]:
            bufs = [(<Request>requests[i]).ob_buf for i from 0 <= i < count]
    finally:
        release_rs(requests, statuses, count, irequests, NULL)
    #
    if not flag[0]: return None
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    for i from 0 <= i < count:
        if type(bufs[i]) is not _p_buffer:
            bufs[i] = None; continue
        rcount = 0
        CHKERR( MPI_Get_count(&istatuses[i], rtype, &rcount) )
        if rcount <= 0: bufs[i] = None
    return [pickle.load(buf) for buf in bufs]

# -----------------------------------------------------------------------------

cdef object PyMPI_barrier(MPI_Comm comm):
    with nogil: CHKERR( MPI_Barrier(comm) )
    return None


cdef object PyMPI_bcast(object obj,
                        int root, MPI_Comm comm):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *buf = NULL
    cdef int count = 0
    cdef MPI_Datatype dtype = MPI_BYTE
    #
    cdef int dosend=0, dorecv=0
    cdef int inter=0, rank=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        if root == <int>MPI_PROC_NULL:
            dosend=0; dorecv=0;
        elif root == <int>MPI_ROOT:
            dosend=1; dorecv=0;
        else:
            dosend=0; dorecv=1;
    else:
        CHKERR( MPI_Comm_rank(comm, &rank) )
        if root == rank:
            dosend=1; dorecv=1;
        else:
            dosend=0; dorecv=1;
    #
    cdef object smsg = None
    if dosend: smsg = pickle.dump(obj, &buf, &count)
    with nogil: CHKERR( MPI_Bcast(&count, 1, MPI_INT,
                                  root, comm) )
    cdef object rmsg = None
    if dorecv and dosend: rmsg = smsg
    elif dorecv: rmsg = pickle.alloc(&buf, count)
    with nogil: CHKERR( MPI_Bcast(buf, count, dtype,
                                  root, comm) )
    if dorecv: rmsg = pickle.load(rmsg)
    return rmsg


cdef object PyMPI_gather(object sendobj, object recvobj,
                         int root, MPI_Comm comm):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int *rcounts = NULL
    cdef int *rdispls = NULL
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int dosend=0, dorecv=0
    cdef int inter=0, size=0, rank=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        CHKERR( MPI_Comm_remote_size(comm, &size) )
        if root == <int>MPI_PROC_NULL:
            dosend=0; dorecv=0;
        elif root == <int>MPI_ROOT:
            dosend=0; dorecv=1;
        else:
            dosend=1; dorecv=0;
    else:
        CHKERR( MPI_Comm_size(comm, &size) )
        CHKERR( MPI_Comm_rank(comm, &rank) )
        if root == rank:
            dosend=1; dorecv=1;
        else:
            dosend=1; dorecv=0;
    #
    cdef object tmp1=None, tmp2=None
    if dorecv: tmp1 = allocate_int(size, &rcounts)
    if dorecv: tmp2 = allocate_int(size, &rdispls)
    #
    cdef object smsg = None
    if dosend: smsg = pickle.dump(sendobj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Gather(&scount, 1, MPI_INT,
                                   rcounts, 1, MPI_INT,
                                   root, comm) )
    cdef object rmsg = None
    if dorecv: rmsg = pickle.allocv(&rbuf, size, rcounts, rdispls)
    with nogil: CHKERR( MPI_Gatherv(sbuf, scount,           stype,
                                    rbuf, rcounts, rdispls, rtype,
                                    root, comm) )
    if dorecv: rmsg = pickle.loadv(rmsg, size, rcounts, rdispls)
    return rmsg


cdef object PyMPI_scatter(object sendobj, object recvobj,
                          int root, MPI_Comm comm):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int *scounts = NULL
    cdef int *sdispls = NULL
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int rcount = 0
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int dosend=0, dorecv=0
    cdef int inter=0, size=0, rank=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        CHKERR( MPI_Comm_remote_size(comm, &size) )
        if root == <int>MPI_PROC_NULL:
            dosend=1; dorecv=0;
        elif root == <int>MPI_ROOT:
            dosend=1; dorecv=0;
        else:
            dosend=0; dorecv=1;
    else:
        CHKERR( MPI_Comm_size(comm, &size) )
        CHKERR( MPI_Comm_rank(comm, &rank) )
        if root == rank:
            dosend=1; dorecv=1;
        else:
            dosend=0; dorecv=1;
    #
    cdef object tmp1=None, tmp2=None
    if dosend: tmp1 = allocate_int(size, &scounts)
    if dosend: tmp2 = allocate_int(size, &sdispls)
    #
    cdef object smsg = None
    if dosend: smsg = pickle.dumpv(sendobj, &sbuf, size, scounts, sdispls)
    with nogil: CHKERR( MPI_Scatter(scounts, 1, MPI_INT,
                                    &rcount, 1, MPI_INT,
                                    root, comm) )
    cdef object rmsg = None
    if dorecv: rmsg = pickle.alloc(&rbuf, rcount)
    with nogil: CHKERR( MPI_Scatterv(sbuf, scounts, sdispls, stype,
                                     rbuf, rcount,           rtype,
                                     root, comm) )
    if dorecv: rmsg = pickle.load(rmsg)
    return rmsg


cdef object PyMPI_allgather(object sendobj, object recvobj,
                            MPI_Comm comm):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int scount = 0
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int *rcounts = NULL
    cdef int *rdispls = NULL
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int inter=0, size=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        CHKERR( MPI_Comm_remote_size(comm, &size) )
    else:
        CHKERR( MPI_Comm_size(comm, &size) )
    #
    cdef object tmp1 = allocate_int(size, &rcounts)
    cdef object tmp2 = allocate_int(size, &rdispls)
    #
    cdef object smsg = pickle.dump(sendobj, &sbuf, &scount)
    with nogil: CHKERR( MPI_Allgather(&scount, 1, MPI_INT,
                                      rcounts, 1, MPI_INT,
                                      comm) )
    cdef object rmsg = pickle.allocv(&rbuf, size, rcounts, rdispls)
    with nogil: CHKERR( MPI_Allgatherv(sbuf, scount,           stype,
                                       rbuf, rcounts, rdispls, rtype,
                                       comm) )
    rmsg = pickle.loadv(rmsg, size, rcounts, rdispls)
    return rmsg


cdef object PyMPI_alltoall(object sendobj, object recvobj,
                           MPI_Comm comm):
    cdef _p_Pickle pickle = PyMPI_pickle()
    #
    cdef void *sbuf = NULL
    cdef int *scounts = NULL
    cdef int *sdispls = NULL
    cdef MPI_Datatype stype = MPI_BYTE
    cdef void *rbuf = NULL
    cdef int *rcounts = NULL
    cdef int *rdispls = NULL
    cdef MPI_Datatype rtype = MPI_BYTE
    #
    cdef int inter=0, size=0
    CHKERR( MPI_Comm_test_inter(comm, &inter) )
    if inter:
        CHKERR( MPI_Comm_remote_size(comm, &size) )
    else:
        CHKERR( MPI_Comm_size(comm, &size) )
    #
    cdef object stmp1 = allocate_int(size, &scounts)
    cdef object stmp2 = allocate_int(size, &sdispls)
    cdef object rtmp1 = allocate_int(size, &rcounts)
    cdef object rtmp2 = allocate_int(size, &rdispls)
    #
    cdef object smsg = pickle.dumpv(sendobj, &sbuf, size, scounts, sdispls)
    with nogil: CHKERR( MPI_Alltoall(scounts, 1, MPI_INT,
                                     rcounts, 1, MPI_INT,
                                     comm) )
    cdef object rmsg = pickle.allocv(&rbuf, size, rcounts, rdispls)
    with nogil: CHKERR( MPI_Alltoallv(sbuf, scounts, sdispls, stype,
                                      rbuf, rcounts, rdispls, rtype,
                                      comm) )
    rmsg = pickle.loadv(rmsg, size, rcounts, rdispls)
    return rmsg

# -----------------------------------------------------------------------------

cdef inline object _py_reduce(object seq, object op):
    if seq is None: return None
    cdef object res
    cdef Py_ssize_t i=0, n=len(seq)
    if op is __MAXLOC__ or op is __MINLOC__:
        res = (seq[0], 0)
        for i from 1 <= i < n:
            res = op(res, (seq[i], i))
    else:
        res = seq[0]
        for i from 1 <= i < n:
            res = op(res, seq[i])
    return res

cdef inline object _py_scan(object seq, object op):
    if seq is None: return None
    cdef Py_ssize_t i=0, n=len(seq)
    if op is __MAXLOC__ or op is __MINLOC__:
        seq[0] = (seq[0], 0)
        for i from 1 <= i < n:
            seq[i] = op(seq[i-1], (seq[i], i))
    else:
        for i from 1 <= i < n:
            seq[i] = op(seq[i-1], seq[i])
    return seq

cdef inline object _py_exscan(object seq, object op):
    if seq is None: return None
    seq = _py_scan(seq, op)
    seq.pop(-1)
    seq.insert(0, None)
    return seq


cdef object PyMPI_reduce(object sendobj, object recvobj,
                         object op, int root, MPI_Comm comm):
    cdef object items = PyMPI_gather(sendobj, recvobj, root, comm)
    return _py_reduce(items, op)


cdef object PyMPI_allreduce(object sendobj, object recvobj,
                            object op, MPI_Comm comm):
    cdef object items = PyMPI_allgather(sendobj, recvobj, comm)
    return _py_reduce(items, op)


cdef object PyMPI_scan(object sendobj, object recvobj,
                       object op, MPI_Comm comm):
    cdef object items = PyMPI_gather(sendobj, None, 0, comm)
    items = _py_scan(items, op)
    return PyMPI_scatter(items, None, 0, comm)


cdef object PyMPI_exscan(object sendobj, object recvobj,
                         object op, MPI_Comm comm):
    cdef object items = PyMPI_gather(sendobj, None, 0, comm)
    items = _py_exscan(items, op)
    return PyMPI_scatter(items, None, 0, comm)

# -----------------------------------------------------------------------------

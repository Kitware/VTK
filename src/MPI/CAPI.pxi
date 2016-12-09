# -----------------------------------------------------------------------------

#  Datatype

cdef api object PyMPIDatatype_New(MPI_Datatype arg):
    cdef Datatype obj = <Datatype>Datatype.__new__(Datatype)
    obj.ob_mpi = arg
    return obj

cdef api MPI_Datatype* PyMPIDatatype_Get(object arg) except NULL:
    return &(<Datatype?>arg).ob_mpi

# -----------------------------------------------------------------------------

#  Status

cdef api object PyMPIStatus_New(MPI_Status *arg):
    cdef Status obj = <Status>Status.__new__(Status)
    if (arg != NULL and
        arg != MPI_STATUS_IGNORE and
        arg != MPI_STATUSES_IGNORE):
        obj.ob_mpi = arg[0]
    else: pass  # XXX should fail ?
    return obj

cdef api MPI_Status* PyMPIStatus_Get(object arg) except? NULL:
    if arg is None: return MPI_STATUS_IGNORE
    return &(<Status?>arg).ob_mpi

# -----------------------------------------------------------------------------

#  Request

cdef api object PyMPIRequest_New(MPI_Request arg):
    cdef Request obj = <Request>Request.__new__(Request)
    obj.ob_mpi = arg
    return obj

cdef api MPI_Request* PyMPIRequest_Get(object arg) except NULL:
    return &(<Request?>arg).ob_mpi

# -----------------------------------------------------------------------------

#  Message

cdef api object PyMPIMessage_New(MPI_Message arg):
    cdef Message obj = <Message>Message.__new__(Message)
    obj.ob_mpi = arg
    return obj

cdef api MPI_Message* PyMPIMessage_Get(object arg) except NULL:
    return &(<Message?>arg).ob_mpi

# -----------------------------------------------------------------------------

#  Op

cdef api object PyMPIOp_New(MPI_Op arg):
    cdef Op obj = <Op>Op.__new__(Op)
    obj.ob_mpi = arg
    return obj

cdef api MPI_Op* PyMPIOp_Get(object arg) except NULL:
    return &(<Op?>arg).ob_mpi

# -----------------------------------------------------------------------------

#  Info

cdef api object PyMPIInfo_New(MPI_Info arg):
    cdef Info obj = <Info>Info.__new__(Info)
    obj.ob_mpi = arg
    return obj

cdef api MPI_Info* PyMPIInfo_Get(object arg) except NULL:
    return &(<Info?>arg).ob_mpi

# -----------------------------------------------------------------------------

#  Group

cdef api object PyMPIGroup_New(MPI_Group arg):
    cdef Group obj = <Group>Group.__new__(Group)
    obj.ob_mpi = arg
    return obj

cdef api MPI_Group* PyMPIGroup_Get(object arg) except NULL:
    return &(<Group?>arg).ob_mpi

# -----------------------------------------------------------------------------

# Comm

cdef api object PyMPIComm_New(MPI_Comm arg):
    cdef type cls   = Comm
    cdef int  inter = 0
    cdef int  topo  = MPI_UNDEFINED
    if arg != MPI_COMM_NULL:
        CHKERR( MPI_Comm_test_inter(arg, &inter) )
        if inter:
            cls = Intercomm
        else:
            CHKERR( MPI_Topo_test(arg, &topo) )
            if topo == MPI_UNDEFINED:
                cls = Intracomm
            elif topo == MPI_CART:
                cls = Cartcomm
            elif topo == MPI_GRAPH:
                cls = Graphcomm
            elif topo == MPI_DIST_GRAPH:
                cls = Distgraphcomm
            else:
                cls = Intracomm
    cdef Comm obj = <Comm>cls.__new__(cls)
    obj.ob_mpi = arg
    return obj

cdef api MPI_Comm* PyMPIComm_Get(object arg) except NULL:
    return &(<Comm?>arg).ob_mpi

# -----------------------------------------------------------------------------

# Win

cdef api object PyMPIWin_New(MPI_Win arg):
    cdef Win obj = <Win>Win.__new__(Win)
    obj.ob_mpi = arg
    return obj

cdef api MPI_Win* PyMPIWin_Get(object arg) except NULL:
    return &(<Win?>arg).ob_mpi

# -----------------------------------------------------------------------------

# File

cdef api object PyMPIFile_New(MPI_File arg):
    cdef File obj = <File>File.__new__(File)
    obj.ob_mpi = arg
    return obj

cdef api MPI_File* PyMPIFile_Get(object arg) except NULL:
    return &(<File?>arg).ob_mpi

# -----------------------------------------------------------------------------

# Errhandler

cdef api object PyMPIErrhandler_New(MPI_Errhandler arg):
    cdef Errhandler obj = <Errhandler>Errhandler.__new__(Errhandler)
    obj.ob_mpi = arg
    return obj

cdef api MPI_Errhandler* PyMPIErrhandler_Get(object arg) except NULL:
    return &(<Errhandler?>arg).ob_mpi

# -----------------------------------------------------------------------------

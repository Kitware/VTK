#------------------------------------------------------------------------------

cdef extern from "Python.h":
    enum: Py_LT
    enum: Py_LE
    enum: Py_EQ
    enum: Py_NE
    enum: Py_GT
    enum: Py_GE

#------------------------------------------------------------------------------

cdef enum PyMPI_OBJECT_FLAGS:
    PyMPI_OWNED = 1<<1

#------------------------------------------------------------------------------
# Status

cdef inline MPI_Status *arg_Status(object status):
    if status is None: return MPI_STATUS_IGNORE
    return &((<Status>status).ob_mpi)

cdef inline int equal_Status(MPI_Status* s1, MPI_Status* s2) nogil:
   cdef size_t i=0, n=sizeof(MPI_Status)
   cdef unsigned char* a = <unsigned char*>s1
   cdef unsigned char* b = <unsigned char*>s2
   for i from 0 <= i < n:
       if a[i] != b[i]:
           return 0
   return 1

#------------------------------------------------------------------------------
# Datatype

include "typeimpl.pxi"

cdef inline Datatype new_Datatype(MPI_Datatype ob):
    cdef Datatype datatype = <Datatype>Datatype.__new__(Datatype)
    datatype.ob_mpi = ob
    return datatype

cdef inline int del_Datatype(MPI_Datatype* ob):
    #
    if ob    == NULL              : return 0
    if ob[0] == MPI_DATATYPE_NULL : return 0
    if not mpi_active()           : return 0
    #
    return MPI_Type_free(ob)

cdef inline int named_Datatype(MPI_Datatype ob):
    if ob == MPI_DATATYPE_NULL: return 0
    cdef int ni = 0, na = 0, nt = 0, combiner = MPI_UNDEFINED
    cdef int ierr = MPI_SUCCESS
    ierr = MPI_Type_get_envelope(ob, &ni, &na, &nt, &combiner)
    if ierr: return 0 # XXX
    return combiner == MPI_COMBINER_NAMED

cdef void fix_fileview_Datatype(Datatype datatype):
    cdef MPI_Datatype ob = datatype.ob_mpi
    if ob == MPI_DATATYPE_NULL: return
    if named_Datatype(ob): pass

#------------------------------------------------------------------------------
# Request

include "reqimpl.pxi"

cdef inline Request new_Request(MPI_Request ob):
    cdef Request request = <Request>Request.__new__(Request)
    request.ob_mpi = ob
    return request

cdef inline int del_Request(MPI_Request* ob):
    #
    if ob    == NULL             : return 0
    if ob[0] == MPI_REQUEST_NULL : return 0
    if not mpi_active()          : return 0
    #
    return MPI_Request_free(ob)

#------------------------------------------------------------------------------
# Op

include "opimpl.pxi"

cdef inline Op new_Op(MPI_Op ob):
    cdef Op op = <Op>Op.__new__(Op)
    op.ob_mpi = ob
    if   ob == MPI_OP_NULL : op.ob_func = NULL
    elif ob == MPI_MAX     : op.ob_func = _op_MAX
    elif ob == MPI_MIN     : op.ob_func = _op_MIN
    elif ob == MPI_SUM     : op.ob_func = _op_SUM
    elif ob == MPI_PROD    : op.ob_func = _op_PROD
    elif ob == MPI_LAND    : op.ob_func = _op_LAND
    elif ob == MPI_BAND    : op.ob_func = _op_BAND
    elif ob == MPI_LOR     : op.ob_func = _op_LOR
    elif ob == MPI_BOR     : op.ob_func = _op_BOR
    elif ob == MPI_LXOR    : op.ob_func = _op_LXOR
    elif ob == MPI_BXOR    : op.ob_func = _op_BXOR
    elif ob == MPI_MAXLOC  : op.ob_func = _op_MAXLOC
    elif ob == MPI_MINLOC  : op.ob_func = _op_MINLOC
    elif ob == MPI_REPLACE : op.ob_func = _op_REPLACE
    return op

cdef inline int del_Op(MPI_Op* ob):
    #
    if ob    == NULL        : return 0
    if ob[0] == MPI_OP_NULL : return 0
    if ob[0] == MPI_MAX     : return 0
    if ob[0] == MPI_MIN     : return 0
    if ob[0] == MPI_SUM     : return 0
    if ob[0] == MPI_PROD    : return 0
    if ob[0] == MPI_LAND    : return 0
    if ob[0] == MPI_BAND    : return 0
    if ob[0] == MPI_LOR     : return 0
    if ob[0] == MPI_BOR     : return 0
    if ob[0] == MPI_LXOR    : return 0
    if ob[0] == MPI_BXOR    : return 0
    if ob[0] == MPI_MAXLOC  : return 0
    if ob[0] == MPI_MINLOC  : return 0
    if ob[0] == MPI_REPLACE : return 0
    if not mpi_active()     : return 0
    #
    return MPI_Op_free(ob)

#------------------------------------------------------------------------------
# Info

cdef inline Info new_Info(MPI_Info ob):
    cdef Info info = <Info>Info.__new__(Info)
    info.ob_mpi = ob
    return info

cdef inline int del_Info(MPI_Info* ob):
    #
    if ob    == NULL          : return 0
    if ob[0] == MPI_INFO_NULL : return 0
    if not mpi_active()       : return 0
    #
    return MPI_Info_free(ob)

cdef inline MPI_Info arg_Info(object info):
    if info is None: return MPI_INFO_NULL
    return (<Info>info).ob_mpi

#------------------------------------------------------------------------------
# Group

cdef inline Group new_Group(MPI_Group ob):
    cdef Group group = <Group>Group.__new__(Group)
    group.ob_mpi = ob
    return group


cdef inline int del_Group(MPI_Group* ob):
     #
     if ob    == NULL            : return 0
     if ob[0] == MPI_GROUP_NULL  : return 0
     if ob[0] == MPI_GROUP_EMPTY : return 0
     if not mpi_active()         : return 0
     #
     return MPI_Group_free(ob)

#------------------------------------------------------------------------------
# Comm

include "commimpl.pxi"

cdef inline Comm new_Comm(MPI_Comm ob):
    cdef Comm comm = <Comm>Comm.__new__(Comm)
    comm.ob_mpi = ob
    return comm

cdef inline Intracomm new_Intracomm(MPI_Comm ob):
    cdef Intracomm comm = <Intracomm>Intracomm.__new__(Intracomm)
    comm.ob_mpi = ob
    return comm

cdef inline Intercomm new_Intercomm(MPI_Comm ob):
    cdef Intercomm comm = <Intercomm>Intercomm.__new__(Intercomm)
    comm.ob_mpi = ob
    return comm

cdef inline int del_Comm(MPI_Comm* ob):
    #
    if ob    == NULL           : return 0
    if ob[0] == MPI_COMM_NULL  : return 0
    if ob[0] == MPI_COMM_SELF  : return 0
    if ob[0] == MPI_COMM_WORLD : return 0
    if not mpi_active()        : return 0
    #
    return MPI_Comm_free(ob)

#------------------------------------------------------------------------------
# Win

include "winimpl.pxi"

cdef inline Win new_Win(MPI_Win ob):
    cdef Win win = <Win>Win.__new__(Win)
    win.ob_mpi = ob
    return win

cdef inline int del_Win(MPI_Win* ob):
    #
    if ob    == NULL         : return 0
    if ob[0] == MPI_WIN_NULL : return 0
    if not mpi_active()      : return 0
    #
    return MPI_Win_free(ob)

#------------------------------------------------------------------------------
# File

cdef inline File new_File(MPI_File ob):
    cdef File file = <File>File.__new__(File)
    file.ob_mpi = ob
    return file

cdef inline int del_File(MPI_File* ob):
    #
    if ob    == NULL          : return 0
    if ob[0] == MPI_FILE_NULL : return 0
    if not mpi_active()       : return 0
    #
    return MPI_File_close(ob)

#------------------------------------------------------------------------------
# Errhandler

cdef inline Errhandler new_Errhandler(MPI_Errhandler ob):
    cdef Errhandler errhandler = <Errhandler>Errhandler.__new__(Errhandler)
    errhandler.ob_mpi = ob
    return errhandler

cdef inline int del_Errhandler(MPI_Errhandler* ob):
    #
    if ob    == NULL                : return 0
    if ob[0] == MPI_ERRHANDLER_NULL : return 0
    if not mpi_active()             : return 0
    #
    return MPI_Errhandler_free(ob)

#------------------------------------------------------------------------------

# Author:  Lisandro Dalcin
# Contact: dalcinl@gmail.com

# --

cdef extern from "mpi.h":

    ctypedef long      MPI_Aint
    ctypedef long long MPI_Offset

    ctypedef struct MPI_Status:
        int MPI_SOURCE
        int MPI_TAG
        int MPI_ERROR

    ctypedef struct _mpi_datatype_t
    ctypedef _mpi_datatype_t* MPI_Datatype

    ctypedef struct _mpi_request_t
    ctypedef _mpi_request_t* MPI_Request

    ctypedef struct _mpi_op_t
    ctypedef _mpi_op_t* MPI_Op

    ctypedef struct _mpi_group_t
    ctypedef _mpi_group_t* MPI_Group

    ctypedef struct _mpi_info_t
    ctypedef _mpi_info_t* MPI_Info

    ctypedef struct _mpi_errhandler_t
    ctypedef _mpi_errhandler_t* MPI_Errhandler

    cdef struct _mpi_comm_t
    ctypedef _mpi_comm_t* MPI_Comm

    ctypedef struct _mpi_win_t
    ctypedef _mpi_win_t* MPI_Win

    ctypedef struct _mpi_file_t
    ctypedef _mpi_file_t* MPI_File

# --

cdef extern from *:
    ctypedef MPI_Aint   Aint   "MPI_Aint"
    ctypedef MPI_Offset Offset "MPI_Offset"

ctypedef public api class Status [
    type   PyMPIStatus_Type,
    object PyMPIStatusObject,
    ]:
    cdef MPI_Status ob_mpi
    cdef int        flags

ctypedef public api class Datatype [
    type   PyMPIDatatype_Type,
    object PyMPIDatatypeObject,
    ]:
    cdef MPI_Datatype ob_mpi
    cdef int          flags

ctypedef public api class Request [
    type   PyMPIRequest_Type,
    object PyMPIRequestObject,
    ]:
    cdef MPI_Request ob_mpi
    cdef int         flags
    cdef object      ob_buf

ctypedef public api class Prequest(Request) [
    type   PyMPIPrequest_Type,
    object PyMPIPrequestObject,
    ]:
    pass

ctypedef public api class Grequest(Request) [
    type   PyMPIGrequest_Type,
    object PyMPIGrequestObject,
    ]:
    cdef MPI_Request ob_grequest

ctypedef public api class Op [
    type   PyMPIOp_Type,
    object PyMPIOpObject,
    ]:
    cdef MPI_Op ob_mpi
    cdef int    flags
    cdef object (*ob_func)(object, object)
    cdef int    ob_usrid

ctypedef public api class Group [
    type   PyMPIGroup_Type,
    object PyMPIGroupObject,
    ]:
    cdef MPI_Group ob_mpi
    cdef int       flags

ctypedef public api class Info [
    type   PyMPIInfo_Type,
    object PyMPIInfoObject,
    ]:
    cdef MPI_Info ob_mpi
    cdef int      flags

ctypedef public api class Errhandler [
    type   PyMPIErrhandler_Type,
    object PyMPIErrhandlerObject,
    ]:
    cdef MPI_Errhandler ob_mpi
    cdef int            flags

ctypedef public api class Comm [
    type   PyMPIComm_Type,
    object PyMPICommObject,
    ]:
    cdef MPI_Comm ob_mpi
    cdef int      flags

ctypedef public api class Intracomm(Comm) [
    type   PyMPIIntracomm_Type,
    object PyMPIIntracommObject,
    ]:
    pass

ctypedef public api class Cartcomm(Intracomm) [
    type   PyMPICartcomm_Type,
    object PyMPICartcommObject,
    ]:
    pass

ctypedef public api class Graphcomm(Intracomm) [
    type   PyMPIGraphcomm_Type,
    object PyMPIGraphcommObject,
    ]:
    pass

ctypedef public api class Distgraphcomm(Intracomm) [
    type   PyMPIDistgraphcomm_Type,
    object PyMPIDistgraphcommObject,
    ]:
    pass

ctypedef public api class Intercomm(Comm) [
    type   PyMPIIntercomm_Type,
    object PyMPIIntercommObject,
    ]:
    pass

ctypedef public api class Win [
    type   PyMPIWin_Type,
    object PyMPIWinObject,
    ]:
    cdef MPI_Win ob_mpi
    cdef int     flags

ctypedef public api class File [
    type   PyMPIFile_Type,
    object PyMPIFileObject,
    ]:
    cdef MPI_File ob_mpi
    cdef int      flags

# --
